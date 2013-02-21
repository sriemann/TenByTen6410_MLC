//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Module Name :    SPI.C
//
// Abstract     :   SPI Interface Routines for Samsung S3C6410 CPU
//
// Environment :    Samsung S3C6410 / WinCE6.0
//
// 2008/10/
//
//-------------------------------------------------------------------------------------------------------------------------

#include <bsp.h>
#include <memory.h>
#include "spi.h"
#include "s3c6410_dma_controller.h"

#define MASTER_CS_ENABLE      pSPIregs->SLAVE_SEL = 0
#define MASTER_CS_DISABLE     pSPIregs->SLAVE_SEL = 1

#define TRAIL_CNT(n)          (((n)&0x3FF)<<19)

#define SPI_POWER_ON          (1<<21)
#define SPI_SCLK_ON           (1<<20)
#define SPI_USBHOST_ON        (1<<22)

#define PCLOCK                (0)
#define USB_HOST_CLOCK        (1)
#define EPLL_CLOCK            (2)
#define SPI_CLOCK             EPLL_CLOCK

#if (SPI_CLOCK == EPLL_CLOCK)
#define CLKSEL                CLKSEL_EPLL
#elif (SPI_CLOCK == USB_HOST_CLOCK)
#define CLKSEL                CLKSEL_USBCLK
#elif (SPI_CLOCK == PCLOCK)
#define CLKSEL                CLKSEL_PCLK
#endif

//#define TEST_MODE 

// MSG
#define    SPI_MSG      0
#define    SPI_INIT     1
#define    SPI_ERR      1

S3C6410_SPI_REG     *pRestoreSPIregs    =    NULL;

#define    WRITE_TIME_OUT_CONSTANT      5000
#define    WRITE_TIME_OUT_MULTIPLIER    1

#define    READ_TIME_OUT_CONSTANT       5000
#define    READ_TIME_OUT_MULTIPLIER     1

#define    TXDONE_TIMEOUT_COUNT         1000
#define    SLAVE_TXWAIT_SLEEP_INTERVAL  5   // Actual wait time will be this multiplied by TXDONE_TIMEOUT_COUNT in milliseconds

DWORD HW_Init(PSPI_PUBLIC_CONTEXT pPublicSpi);

DWORD ThreadForTx(PSPI_PUBLIC_CONTEXT pPublicSpi);
DWORD ThreadForRx(PSPI_PUBLIC_CONTEXT pPublicSpi);
DWORD ThreadForSpi(PSPI_PUBLIC_CONTEXT pPublicSpi);
DWORD ThreadForRxDmaDone(PSPI_PUBLIC_CONTEXT pPublicSpi);
DWORD ThreadForTxDmaDone(PSPI_PUBLIC_CONTEXT pPublicSpi);


static DMA_CH_CONTEXT    g_OutputDMA;
static DMA_CH_CONTEXT    g_InputDMA;

//DMA
UINT DmaDstAddress;
UINT DmaSrcAddress;

//DMA Buffer Address Alloc
#define Buffer_Mem_Size   1504

//Memory Physical Address
PHYSICAL_ADDRESS    PhysDmaDstBufferAddr;
PHYSICAL_ADDRESS    PhysDmaSrcBufferAddr;

// DMA Buffer Address Alloc
PBYTE pVirtDmaDstBufferAddr = NULL;
PBYTE pVirtDmaSrcBufferAddr = NULL;

BOOL
DllEntry(
    HINSTANCE   hinstDll,
    DWORD       dwReason,
    LPVOID      lpReserved
    )
{
    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        DEBUGMSG (1, (TEXT("[SPI] Process Attach\r\n")));
    }

    if ( dwReason == DLL_PROCESS_DETACH )
    {
        DEBUGMSG (1, (TEXT("[SPI] Process Detach\r\n")));
    }

    return(TRUE);
}

DWORD
HW_Init(
    PSPI_PUBLIC_CONTEXT pPublicSpi
    )
{
    BOOL bResult = TRUE;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    if ( !pPublicSpi )
    {
        bResult = FALSE;
        goto CleanUp;
    }

    // GPIO Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    pPublicSpi->pGPIOregs = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (pPublicSpi->pGPIOregs == NULL)
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] For pGPIOregs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    // HS-SPI Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SPI0;    
    pPublicSpi->pSPIregs = (volatile S3C6410_SPI_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SPI_REG), FALSE);
    if (pPublicSpi->pSPIregs == NULL)
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] For pSPIregs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    // Syscon Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;      
    pPublicSpi->pSYSCONregs = (volatile S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (pPublicSpi->pSYSCONregs == NULL)
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] For pSYSCONregs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    // DMAC0 Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA0;      
    pPublicSpi->pDMAC0regs = (volatile S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (pPublicSpi->pDMAC0regs == NULL)
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] For pDMAC0regs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    // DMAC1 Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA1;      
    pPublicSpi->pDMAC1regs = (volatile S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (pPublicSpi->pDMAC1regs == NULL)
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] For pDMAC1regs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

CleanUp:

    if (!bResult)
    {
        if (pPublicSpi->pGPIOregs)
        {
            MmUnmapIoSpace((PVOID)pPublicSpi->pGPIOregs, sizeof(S3C6410_GPIO_REG));
            pPublicSpi->pGPIOregs = NULL;
        }

        if (pPublicSpi->pSPIregs)
        {
            MmUnmapIoSpace((PVOID)pPublicSpi->pSPIregs, sizeof(S3C6410_SPI_REG));
            pPublicSpi->pSPIregs = NULL;
        }

        if (pPublicSpi->pDMAC0regs)
        {
            MmUnmapIoSpace((PVOID)pPublicSpi->pDMAC0regs, sizeof(S3C6410_DMAC_REG));
            pPublicSpi->pDMAC0regs = NULL;
        }

        if (pPublicSpi->pDMAC1regs)
        {
            MmUnmapIoSpace((PVOID)pPublicSpi->pDMAC1regs, sizeof(S3C6410_DMAC_REG));
            pPublicSpi->pDMAC1regs = NULL;
        }

        if (pPublicSpi->pSYSCONregs)
        {
            MmUnmapIoSpace((PVOID)pPublicSpi->pSYSCONregs, sizeof(S3C6410_SYSCON_REG));
            pPublicSpi->pSYSCONregs = NULL;
        }

        bResult = FALSE;
    }

    //Configure HS-SPI Port Drive Strength
    pPublicSpi->pGPIOregs->SPCON = pPublicSpi->pGPIOregs->SPCON & ~(0x3<<28) | (2<<28);
    //Set GPIO for MISO, MOSI, SPICLK, SS
    pPublicSpi->pGPIOregs->GPCPUD = pPublicSpi->pGPIOregs->GPCPUD & ~(0xFF<<0);
    pPublicSpi->pGPIOregs->GPCCON = pPublicSpi->pGPIOregs->GPCCON & ~(0xFFFF<<0) | (2<<0) | (2<<4) | (2<<8) |(2<<12);

    // Clock On
    pPublicSpi->pSYSCONregs->PCLK_GATE |= SPI_POWER_ON;
#if    (SPI_CLOCK == EPLL_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE |= SPI_SCLK_ON;
#elif (SPI_CLOCK == USB_HOST_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE |= SPI_USBHOST_ON;
#endif

    DMA_initialize_register_address((void *)pPublicSpi->pDMAC0regs, (void *)pPublicSpi->pDMAC1regs, (void *)pPublicSpi->pSYSCONregs);

    return bResult;
}

BOOL InitializeBuffer()
{
    BOOL bResult = TRUE;

    DMA_ADAPTER_OBJECT Adapter1, Adapter2;

    RETAILMSG(SPI_INIT,(TEXT("+[SPI] InitializeBuffer\n")));
    memset(&Adapter1, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter1.InterfaceType = Internal;
    Adapter1.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    memset(&Adapter2, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter2.InterfaceType = Internal;
    Adapter2.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    // Allocate a block of virtual memory (physically contiguous) for the DMA buffers.
    //
    pVirtDmaDstBufferAddr = (PBYTE)HalAllocateCommonBuffer(&Adapter1, Buffer_Mem_Size, &PhysDmaDstBufferAddr, FALSE);
    RETAILMSG(FALSE, (TEXT("[SPIDD] InitializeBuffer() - pVirtDmaDstBufferAddr %x\r\n"),pVirtDmaDstBufferAddr));
    if (pVirtDmaDstBufferAddr == NULL)
    {
        RETAILMSG(SPI_INIT, (TEXT("[SPI] InitializeBuffer() - Failed to allocate DMA buffer for SPI.\r\n")));
        HalFreeCommonBuffer(0, 0, PhysDmaDstBufferAddr, pVirtDmaDstBufferAddr, FALSE);
        bResult = FALSE;
        goto CleanUp;
    }

    pVirtDmaSrcBufferAddr = (PBYTE)HalAllocateCommonBuffer(&Adapter2, Buffer_Mem_Size, &PhysDmaSrcBufferAddr, FALSE);
    RETAILMSG(FALSE, (TEXT("[SPIDD] InitializeBuffer() - pVirtDmaSrcBufferAddr %x\r\n"),pVirtDmaSrcBufferAddr));
    if (pVirtDmaSrcBufferAddr == NULL)
    {
        RETAILMSG(SPI_INIT, (TEXT("[SPI] InitializeBuffer() - Failed to allocate DMA buffer for SPI.\r\n")));
        HalFreeCommonBuffer(0, 0, PhysDmaSrcBufferAddr, pVirtDmaSrcBufferAddr, FALSE);
        bResult = FALSE;
        goto CleanUp;
    }

    //DMA Address
    DmaDstAddress = (UINT)(PhysDmaDstBufferAddr.LowPart);
    DmaSrcAddress = (UINT)(PhysDmaSrcBufferAddr.LowPart);

    RETAILMSG(SPI_MSG, (TEXT("[SPI] pVirtDmaSrcBufferAddr 0x%x   DmaSrcAddress 0x%x \r\n"),pVirtDmaSrcBufferAddr, DmaSrcAddress));
    RETAILMSG(SPI_MSG, (TEXT("[SPI] pVirtDmaDstBufferAddr 0x%x   DmaDstAddress 0x%x \r\n"),pVirtDmaDstBufferAddr, DmaDstAddress));

    RETAILMSG(SPI_INIT,(TEXT("-[SPI] InitializeBuffer\n")));

CleanUp:
    return bResult;
}


PSPI_PUBLIC_CONTEXT SPI_Init(PVOID Context)
{
    LPTSTR                          ActivePath = (LPTSTR) Context;
    PSPI_PUBLIC_CONTEXT             pPublicSpi = NULL;
    BOOL                            bResult = TRUE;
    DWORD                           dwHwIntr=0;

    RETAILMSG(SPI_INIT,(TEXT("++[SPI] HSP_Init Function\r\n")));
    RETAILMSG(SPI_MSG,(TEXT("[SPI] Active Path : %s\n"), ActivePath));

    if ( !(pPublicSpi = (PSPI_PUBLIC_CONTEXT)LocalAlloc( LPTR, sizeof(SPI_PUBLIC_CONTEXT) )) )
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] Can't not allocate for SPI Context\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    if ( !(pRestoreSPIregs = (PS3C6410_SPI_REG)LocalAlloc( LPTR, sizeof(S3C6410_SPI_REG) )) )
    {
        bResult = FALSE;
        goto CleanUp;
    }


    if(!HW_Init(pPublicSpi))
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] HW_Init is failed\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    if(!InitializeBuffer())
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] InitializeBuffer is failed\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    // Request DMA Channel
    // DMA context have Virtual IRQ Number of Allocated DMA Channel
    // You Should initialize DMA Interrupt Thread after "Request DMA Channel"
    if( DMA_request_channel(&g_OutputDMA, DMA_SPI0_TX) != TRUE )
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] DMA SPI TX channel request is failed\n")));
        bResult = FALSE;
        goto CleanUp;   
    }
    if( DMA_request_channel(&g_InputDMA, DMA_SPI0_RX) != TRUE )
    {
        RETAILMSG(SPI_INIT,(TEXT("[SPI] DMA SPI RX channel request is failed\n")));
        bResult = FALSE;
        goto CleanUp;   
    }

    do
    {
        InitializeCriticalSection(&(pPublicSpi->CsRxAccess));
        InitializeCriticalSection(&(pPublicSpi->CsTxAccess));

        //Rx Thread
        pPublicSpi->hRxEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPublicSpi->hRxEvent == NULL)
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI Rx Event creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicSpi->hRxThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadForRx, (LPVOID)pPublicSpi, 0, (LPDWORD)&pPublicSpi->dwRxThreadId);
        if (pPublicSpi->hRxThread == NULL)
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI Rx Thread creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicSpi->hRxDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPublicSpi->hRxDoneEvent == NULL)
        {
            RETAILMSG(SPI_INIT, (TEXT("[SPI] SPI Rx Done Event creation error!!!\n")));
            bResult = FALSE;
            break;
        }
        
        pPublicSpi->hRxIntrDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPublicSpi->hRxIntrDoneEvent == NULL)
        {
            RETAILMSG(SPI_INIT, (TEXT("[SPI] SPI Rx Interrupt Event creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        //Tx Thread
        pPublicSpi->hTxEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPublicSpi->hTxEvent == NULL)
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI Tx Event creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicSpi->hTxThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadForTx, (LPVOID)pPublicSpi, 0, (LPDWORD)&pPublicSpi->dwTxThreadId);
        if (pPublicSpi->hTxThread == NULL)
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI Tx Thread creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicSpi->hTxDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPublicSpi->hTxDoneEvent == NULL)
        {
            RETAILMSG(SPI_INIT, (TEXT("[SPI] SPI Tx Done Event creation error!!!\n")));
            bResult = FALSE;
            break;
        }        

        pPublicSpi->hTxIntrDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPublicSpi->hTxIntrDoneEvent == NULL)
        {
            RETAILMSG(SPI_INIT, (TEXT("[SPI] SPI Tx Interrupt Event creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        //Spi ISR
        pPublicSpi->dwSpiSysIntr = SYSINTR_NOP;
        dwHwIntr = IRQ_SPI0;        //HS-SPI

        pPublicSpi->hSpiEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwHwIntr, sizeof(DWORD), &pPublicSpi->dwSpiSysIntr, sizeof(DWORD), NULL))
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] Failed to request the SPI sysintr.\n")));
            pPublicSpi->dwSpiSysIntr = SYSINTR_UNDEFINED;
            bResult = FALSE;
            break;
        }

        if (!InterruptInitialize(pPublicSpi->dwSpiSysIntr, pPublicSpi->hSpiEvent, NULL, 0))
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI Interrupt Initialization failed!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicSpi->hSpiThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadForSpi, (LPVOID)pPublicSpi, 0, (LPDWORD)&pPublicSpi->dwSpiThreadId);
        if (pPublicSpi->hSpiThread == NULL)
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI ISR Thread creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        //Tx DMA Done ISR
        pPublicSpi->dwTxDmaDoneSysIntr = SYSINTR_NOP;
        dwHwIntr = g_OutputDMA.dwIRQ;

        pPublicSpi->hTxDmaDoneDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        pPublicSpi->hTxDmaDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwHwIntr, sizeof(DWORD), &pPublicSpi->dwTxDmaDoneSysIntr, sizeof(DWORD), NULL))
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] Failed to request the SPI_DMA sysintr.\n")));
            pPublicSpi->dwTxDmaDoneSysIntr = SYSINTR_UNDEFINED;
            bResult = FALSE;
            break;
        }

        if (!InterruptInitialize(pPublicSpi->dwTxDmaDoneSysIntr, pPublicSpi->hTxDmaDoneEvent, NULL, 0))
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] DMA Interrupt Initialization failed!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicSpi->hTxDmaDoneThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadForTxDmaDone, (LPVOID)pPublicSpi, 0, (LPDWORD)&pPublicSpi->dwTxDmaDoneThreadId);
        if (pPublicSpi->hTxDmaDoneThread == NULL)
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI Dma Thread creation error!!!\n")));
            bResult = FALSE;
            break;
        }

        //Rx DMA Done ISR
        pPublicSpi->dwRxDmaDoneSysIntr = SYSINTR_NOP;
        dwHwIntr = g_InputDMA.dwIRQ;

        pPublicSpi->hRxDmaDoneDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        pPublicSpi->hRxDmaDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwHwIntr, sizeof(DWORD), &pPublicSpi->dwRxDmaDoneSysIntr, sizeof(DWORD), NULL))
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] Failed to request the SPI_DMA sysintr.\n")));
            pPublicSpi->dwRxDmaDoneSysIntr = SYSINTR_UNDEFINED;
            bResult = FALSE;
            break;
        }

        if (!InterruptInitialize(pPublicSpi->dwRxDmaDoneSysIntr, pPublicSpi->hRxDmaDoneEvent, NULL, 0))
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] DMA Interrupt Initialization failed!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicSpi->hRxDmaDoneThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadForRxDmaDone, (LPVOID)pPublicSpi, 0, (LPDWORD)&pPublicSpi->dwRxDmaDoneThreadId);
        if (pPublicSpi->hRxDmaDoneThread == NULL)
        {
            RETAILMSG(SPI_INIT,(TEXT("[SPI] SPI Dma Thread creation error!!!\n")));
            bResult = FALSE;
            break;
        }

    } while (0);


CleanUp:
    RETAILMSG(SPI_INIT,(TEXT("--[SPI] HSP_Init Function\r\n")));
    if(bResult)
    {
        return pPublicSpi;
    }
    else
    {
        return NULL;
    }
}

DWORD
SPI_Open(
   DWORD        pContext,
   DWORD        AccessCode,
   DWORD        ShareMode)
{
    PSPI_PUBLIC_CONTEXT         pSpiPublic  = NULL;
    PSPI_PRIVATE_CONTEXT        pSpiPrivate = NULL;
    BOOL                        bResult     = TRUE;

    if(pContext != 0)
    {
        pSpiPublic  = (PSPI_PUBLIC_CONTEXT) pContext;
    }
    else
    {
        bResult = FALSE;
        goto CleanUp;
    }

    if ( !(pSpiPrivate = (PSPI_PRIVATE_CONTEXT)LocalAlloc( LPTR, sizeof(SPI_PRIVATE_CONTEXT) )) )
    {
        RETAILMSG(SPI_ERR,(TEXT("[SPI] Can't not allocate for SPI Context\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    pSpiPrivate->State      = STATE_INIT;

    pSpiPrivate->pSpiPublic = pSpiPublic;

    pSpiPrivate->bUseRxDMA  = FALSE;
    pSpiPrivate->bUseRxIntr = FALSE;
    pSpiPrivate->bUseTxDMA  = FALSE;
    pSpiPrivate->bUseTxIntr = FALSE;

CleanUp:
    if(bResult)
    {
        return (DWORD) pSpiPrivate;
    }
    else
    {
        return (DWORD) NULL;
    }
}

DWORD
SPI_Read(
    DWORD     hOpenContext,
    LPVOID    pBuffer,
    DWORD     Count)
{
    PSPI_PRIVATE_CONTEXT pSpiPrivate    = (PSPI_PRIVATE_CONTEXT)hOpenContext;
    PSPI_PUBLIC_CONTEXT  pSpiPublic     = pSpiPrivate->pSpiPublic;
    BOOL                 bResult        = TRUE;
    ULONG                BytesRead      = 0;
    ULONG                RxCount        = 0;
    
    HRESULT hr;
    PBYTE     MappedEmbedded;
    PBYTE     Marshalled;

    if((PSPI_PRIVATE_CONTEXT)hOpenContext == NULL)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    //param check
    if(pSpiPrivate->State != STATE_IDLE)
    {
        RETAILMSG(SPI_ERR,(TEXT("[SPI] READ ERROR : STATE IS NOT IDLE\n")));
        bResult = FALSE;
        goto CleanUp;
    }
    RETAILMSG(SPI_MSG,(TEXT("[SPI] pBuffer : 0x%X, Count : %d\n"), pBuffer, Count));

    hr = CeOpenCallerBuffer((PVOID*) &MappedEmbedded, pBuffer, Count, ARG_IO_PTR, FALSE);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    hr = CeAllocAsynchronousBuffer((PVOID*) &Marshalled, MappedEmbedded, Count, ARG_IO_PTR);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    if(pSpiPrivate->bUseRxDMA)
    {
        pSpiPrivate->pRxBuffer         = pVirtDmaDstBufferAddr;
        pSpiPrivate->pRxDMABuffer     = (LPVOID)DmaDstAddress;
    }
    else
    {
        pSpiPrivate->pRxBuffer = Marshalled;
    }
    RETAILMSG(SPI_MSG,(TEXT("[SPI] MappedEmbedded 0x%x\n"),MappedEmbedded));
    RETAILMSG(SPI_MSG,(TEXT("[SPI] Marshalled 0x%x\n"),Marshalled));

    pSpiPrivate->dwRxCount = Count;
    pSpiPublic->pSpiPrivate = pSpiPrivate;

    SetEvent(pSpiPublic->hRxEvent);

    //Thread call

    WaitForSingleObject(pSpiPublic->hRxDoneEvent, INFINITE);
    pSpiPrivate->State = STATE_IDLE;

    if(pSpiPrivate->bUseRxDMA)
    {
        memcpy(Marshalled, pVirtDmaDstBufferAddr,Count);
    }

    hr = CeFreeAsynchronousBuffer((PVOID)Marshalled, MappedEmbedded, Count, ARG_IO_PTR);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    hr = CeCloseCallerBuffer((PVOID)  MappedEmbedded,   pBuffer, Count, ARG_IO_PTR);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    // pSpiPrivate->dwRxCount is set to Count and decremented in a bounded way to 0. 
    // So it is guaranteed to be less than count. Explicitly checking it here makes prefast happy
    RxCount = pSpiPrivate->dwRxCount;
    if(RxCount <= Count)
    {    
        BytesRead = Count - RxCount;
    }
    else
    {
        BytesRead = 0;
        bResult = FALSE;
    }

CleanUp:
    
    RETAILMSG(SPI_MSG,(TEXT("[SPI] SPI_Read : Return Value : %d\n\n"), BytesRead));

    if(bResult)
    {
        return BytesRead;
    }
    else
    {
        return 0;
    }
}

DWORD
SPI_Write(
    DWORD     hOpenContext,
    LPVOID    pBuffer,
    DWORD     Count)
{
    PSPI_PRIVATE_CONTEXT pSpiPrivate    = (PSPI_PRIVATE_CONTEXT)hOpenContext;
    PSPI_PUBLIC_CONTEXT  pSpiPublic     = pSpiPrivate->pSpiPublic;
    BOOL                 bResult        = TRUE;
    ULONG                BytesWritten   = 0;
    ULONG                TxCount        = 0;

    HRESULT hr;
    PBYTE     MappedEmbedded;
    PBYTE     Marshalled;

    if((PSPI_PRIVATE_CONTEXT)hOpenContext == NULL)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    //param check
    if(pSpiPrivate->State != STATE_IDLE)
    {
        RETAILMSG(SPI_ERR,(TEXT("[SPI] WRITE ERROR : STATE IS NOT IDLE\n")));
        bResult = FALSE;
        goto CleanUp;
    }
    RETAILMSG(SPI_MSG,(TEXT("[SPI] pBuffer : 0x%X, Count : %d\n"), pBuffer, Count));

    hr = CeOpenCallerBuffer((PVOID*) &MappedEmbedded, pBuffer, Count, ARG_IO_PTR, FALSE);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    hr = CeAllocAsynchronousBuffer((PVOID*) &Marshalled, MappedEmbedded, Count, ARG_IO_PTR);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    if(pSpiPrivate->bUseTxDMA)
    {
        memcpy(pVirtDmaSrcBufferAddr,Marshalled, Count);
        pSpiPrivate->pTxBuffer         = pVirtDmaSrcBufferAddr;
        pSpiPrivate->pTxDMABuffer     = (LPVOID)DmaSrcAddress;
    }
    else
    {
        pSpiPrivate->pTxBuffer = (LPVOID)Marshalled;
    }
    RETAILMSG(SPI_MSG,(TEXT("[SPI] MappedEmbedded 0x%x\n"),MappedEmbedded));
    RETAILMSG(SPI_MSG,(TEXT("[SPI] Marshalled 0x%x\n"),Marshalled));

    pSpiPrivate->dwTxCount = Count;
    pSpiPublic->pSpiPrivate = pSpiPrivate;

    SetEvent(pSpiPublic->hTxEvent);

    //Thread call

    WaitForSingleObject(pSpiPublic->hTxDoneEvent, INFINITE);
    pSpiPrivate->State = STATE_IDLE;

    hr = CeFreeAsynchronousBuffer((PVOID)Marshalled, MappedEmbedded, Count, ARG_IO_PTR);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    hr = CeCloseCallerBuffer((PVOID)  MappedEmbedded,   pBuffer, Count, ARG_IO_PTR);
    if(hr != S_OK)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    // dwTxCount is set to Count and decremented in a bounded way to 0. 
    // So it is guaranteed to be less than count. Explicitly checking it here makes prefast happy
    TxCount = pSpiPrivate->dwTxCount;
    if(TxCount <= Count)
    {    
        BytesWritten = Count - TxCount;
    }
    else
    {
        BytesWritten = 0;
        bResult = FALSE;
    }
    
CleanUp:
    RETAILMSG(SPI_MSG,(TEXT("[SPI] SPI_Write : Return Value : %d\n"), BytesWritten));

    if(bResult)
    {
        return BytesWritten;
    }
    else
    {
        return 0;
    } 
}

BOOL
SPI_IOControl(
    DWORD dwInst,
    DWORD dwIoControlCode,
    PBYTE lpInBuf,
    DWORD nInBufSize,
    PBYTE lpOutBuf,
    DWORD nOutBufSize,
    LPDWORD lpBytesRetruned)
{
    PSPI_PRIVATE_CONTEXT        pSpiPrivate = (PSPI_PRIVATE_CONTEXT)dwInst;
    PSPI_PUBLIC_CONTEXT         pSpiPublic  = pSpiPrivate->pSpiPublic;
    volatile S3C6410_SPI_REG    *pSPIregs   = pSpiPublic->pSPIregs;
    volatile S3C6410_SPI_REG    *pRxSPIregs = &pSpiPrivate->RxSPIregs;
    volatile S3C6410_SPI_REG    *pTxSPIregs = &pSpiPrivate->TxSPIregs;

    SET_CONFIG              SetConfig;
    BOOL                    bResult = TRUE;

#ifndef ALLOW_USER_MODE_ACCESS
    //if caller is not kernel mode, do not allow setting power state
    if (GetDirectCallerProcessId() != GetCurrentProcessId()){
        return FALSE;
    }
#endif

    if((PSPI_PRIVATE_CONTEXT)dwInst == NULL)
    {
        bResult = FALSE;
        goto CleanUp;
    }

    switch(dwIoControlCode)
    {
    case SPI_IOCTL_SET_CONFIG:
        __try
        {
            if( lpInBuf == NULL || nInBufSize != sizeof(SET_CONFIG) )
            {
                bResult = FALSE;
                break;
            }

            if (!CeSafeCopyMemory((LPVOID)&SetConfig, lpInBuf, nInBufSize)) 
            {
                bResult = FALSE;
                break;
            }

            pSpiPrivate->dwTimeOutVal       = SetConfig.dwTimeOutVal;
            pSpiPrivate->dwMode             = SetConfig.dwMode;
            pSpiPrivate->dwPrescaler        = SetConfig.dwPrescaler;

            pSpiPrivate->bUseRxDMA          = SetConfig.bUseRxDMA;
            pSpiPrivate->bUseRxIntr         = SetConfig.bUseRxIntr;

            pSpiPrivate->bUseTxDMA          = SetConfig.bUseTxDMA;
            pSpiPrivate->bUseTxIntr         = SetConfig.bUseTxIntr;

            pRxSPIregs->CH_CFG              = CPOL_RISING|CPHA_FORMAT_A;

            //Slave TX output time control
            pRxSPIregs->CH_CFG              &= ~(HIGH_SPEED_MASK);
            pRxSPIregs->CH_CFG              |= (HIGH_SPEED_EN);

            pRxSPIregs->CLK_CFG             = CLKSEL|(pSpiPrivate->dwPrescaler);
            pRxSPIregs->MODE_CFG            = MODE_DEFAULT;

            pTxSPIregs->CH_CFG              = pRxSPIregs->CH_CFG;
            pTxSPIregs->CLK_CFG             = pRxSPIregs->CLK_CFG;
            pTxSPIregs->MODE_CFG            = pRxSPIregs->MODE_CFG;

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                pRxSPIregs->CH_CFG      |= SPI_MASTER;
                pRxSPIregs->CLK_CFG     |= ENCLK_ENABLE;

                pTxSPIregs->CH_CFG      |= SPI_MASTER;
                pTxSPIregs->CLK_CFG     |= ENCLK_ENABLE;
            }
            else if(pSpiPrivate->dwMode == SPI_SLAVE_MODE)
            {
                pRxSPIregs->CH_CFG      |= SPI_SLAVE;
                pRxSPIregs->CLK_CFG     |= ENCLK_DISABLE;

                pTxSPIregs->CH_CFG      |= SPI_SLAVE;
                pTxSPIregs->CLK_CFG     |= ENCLK_DISABLE;
            }
            else
            {
                RETAILMSG(SPI_ERR,(TEXT("[SPI] SPI_IOCTL_SET_CONFIG: unsupported MODE\n")));
                pSpiPrivate->State = STATE_ERROR;
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( SPI_ERR, ( _T("SPI_IOControl: exception occurred\n")) );
            return FALSE;
        }
        break;

    case SPI_IOCTL_START:
        if(pSpiPrivate->State == STATE_ERROR)
        {
            RETAILMSG(SPI_ERR,(TEXT("[SPI] SPI_IOCTL_START ERROR\n")));
            bResult = FALSE;
            break;
        }
        pSpiPrivate->State = STATE_IDLE;
        RETAILMSG(SPI_MSG,(TEXT("[SPI] SPI STATE : SPI_IOCTL_START\n")));
        break;

    default:
        RETAILMSG(SPI_ERR,(TEXT("[SPI_IOCTL] ERROR. Invalid IOCTL\n")));
        bResult = FALSE;
        break;
    }


CleanUp:
    return bResult;
}


DWORD ThreadForTx(PSPI_PUBLIC_CONTEXT pSpiPublic)
{
    volatile S3C6410_SPI_REG    *pSPIregs  = NULL;
    PSPI_PRIVATE_CONTEXT        pSpiPrivate;

    DWORD     dwTxCount;
    PBYTE     pTxBuffer;

    PBYTE     pTestBuffer;
    DWORD     dwTestCount;

    ULONG    TimeOut;
    ULONG    TotalTimeOut;
    ULONG    WaitReturn;

    INT      TimeOutCount;

    if(pSpiPublic != NULL)
    {
        pSPIregs       = pSpiPublic->pSPIregs;
    }

    do
    {
        WaitForSingleObject(pSpiPublic->hTxEvent, INFINITE);

        pSpiPrivate     = (PSPI_PRIVATE_CONTEXT) pSpiPublic->pSpiPrivate;

        if(pSpiPrivate->dwTxCount != 0)
        {
            dwTestCount     = dwTxCount = pSpiPrivate->dwTxCount;
        }

        pTestBuffer     = pTxBuffer = pSpiPrivate->pTxBuffer;

        RETAILMSG(SPI_MSG,(TEXT("[SPI] pTxBuffer : 0x%X, dwTxCount : %d \r\n"), pTxBuffer, dwTxCount));

        //Reset
        pSPIregs->CH_CFG |= SW_RST;
        while(!(pSPIregs->CH_CFG & SW_RST));
        RETAILMSG(SPI_MSG,(TEXT("[SPI] HS SPI reset\n")));
        pSPIregs->CH_CFG &= ~SW_RST;

        if(pSpiPrivate->bUseTxIntr)
        // INT  + TX
        {
            RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : USE INT \r\n")));
            pSpiPrivate->State = STATE_TXINTR;

            pSPIregs->CH_CFG     = pSpiPrivate->TxSPIregs.CH_CFG;
            pSPIregs->CLK_CFG      = pSpiPrivate->TxSPIregs.CLK_CFG;
            pSPIregs->MODE_CFG    = pSpiPrivate->TxSPIregs.MODE_CFG|(TX_TRIG_LEVEL<<5);

            pSPIregs->SPI_INT_EN        =    TX_FIFORDY;
            pSPIregs->PENDING_CLEAR        =    TX_UNDERRUN_CLR|TX_OVERRUN_CLR|RX_UNDERRUN_CLR|RX_OVERRUN_CLR|TRAILING_CLR;
            pSPIregs->CH_CFG            |=    TX_CH_ON;

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : MASTER MODE \r\n")));
                MASTER_CS_ENABLE;
            }
            else
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : SLAVE MODE \r\n")));
            }

            //Timeout value setting
            TotalTimeOut = WRITE_TIME_OUT_CONSTANT + (WRITE_TIME_OUT_MULTIPLIER/1000)*dwTxCount;
            TimeOut    =    TotalTimeOut;
            WaitReturn = WaitForSingleObject(pSpiPublic->hTxIntrDoneEvent, TimeOut);

            if ( WAIT_TIMEOUT == WaitReturn )
            {
                // Timeout
                RETAILMSG (SPI_ERR, (TEXT("SPI: ThreadForTx: Write Event timeout!!!\r\n")));
                goto LEAVEWRITE;
            }

            TimeOutCount = TXDONE_TIMEOUT_COUNT;
            while((((pSPIregs ->SPI_STATUS>>6) & 0x7f) && !(pSPIregs ->SPI_STATUS & TX_DONE)) &&
                TimeOutCount > 0)
            {
                if(pSpiPrivate->dwMode == SPI_SLAVE_MODE)
                {
                    // In SLAVE mode, TX could take longer because the master has to initiate the read
                    // So a busy wait with a large TimeoutCount is inefficient. Adding a small Sleep and 
                    // keeping the same TimeoutCount as master mode is better.
                    Sleep(SLAVE_TXWAIT_SLEEP_INTERVAL);
                }
                TimeOutCount--;
            }

            if (TimeOutCount == 0)
            {
                RETAILMSG (SPI_ERR, (TEXT("SPI: ThreadForTx: Timeout waiting for TXDONE status.\r\n")));
            }
        }
        else if(pSpiPrivate->bUseTxDMA)
        // DMA + TX
        {
            DWORD dwDmaLen            = dwTxCount & 0xFFFFF ;

            RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : USE DMA (TxCount : %d) \r\n"),dwDmaLen));

            pSpiPrivate->State = STATE_TXDMA;

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                pSPIregs->CH_CFG     = pSpiPrivate->TxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG      = pSpiPrivate->TxSPIregs.CLK_CFG;
                pSPIregs->MODE_CFG  = pSpiPrivate->TxSPIregs.MODE_CFG;
            }
            else
            {
                pSPIregs->CH_CFG     = pSpiPrivate->TxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG      = pSpiPrivate->TxSPIregs.CLK_CFG;
                pSPIregs->MODE_CFG  = pSpiPrivate->TxSPIregs.MODE_CFG;
            }

            if(dwDmaLen > 0)
            {
                pSPIregs->MODE_CFG        |=    TX_DMA_ON|DMA_SINGLE;
                pSPIregs->CH_CFG         |=    TX_CH_ON;

                {
                    RETAILMSG(SPI_MSG,(TEXT("[SPI] pSpiPrivate->pTxDMABuffer : 0x%X \r\n"), pSpiPrivate->pTxDMABuffer));
                    DMA_initialize_channel(&g_OutputDMA, TRUE);
                    DMA_set_channel_source(&g_OutputDMA, (UINT)pSpiPrivate->pTxDMABuffer, BYTE_UNIT, BURST_1, INCREASE);
                    DMA_set_channel_destination(&g_OutputDMA, (UINT)SPI_TX_DATA_PHY_ADDR, BYTE_UNIT, BURST_1, FIXED);
                    DMA_set_channel_transfer_size(&g_OutputDMA, dwDmaLen);
                    DMA_initialize_LLI(&g_OutputDMA, 0);
                }

                if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
                {
                    RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : MASTER MODE \r\n")));
                    MASTER_CS_ENABLE;
                }
                else
                {
                    RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : SLAVE MODE \r\n")));
                }

                DMA_channel_start(&g_OutputDMA);

                //Timeout value setting
                TotalTimeOut = WRITE_TIME_OUT_CONSTANT + (WRITE_TIME_OUT_MULTIPLIER/1000)*dwTxCount;
                TimeOut    =    TotalTimeOut;
                WaitReturn = WaitForSingleObject(pSpiPublic->hTxDmaDoneDoneEvent, TimeOut);

                if ( WAIT_TIMEOUT == WaitReturn )
                {
                    // Timeout
                    RETAILMSG (TRUE, (TEXT("Write timeout!!!\r\n")));
                    // Stop output DMA
                    DMA_channel_stop(&g_OutputDMA);
                    goto LEAVEWRITE;
                }

                pSpiPrivate->dwTxCount -= dwDmaLen;
                pSpiPrivate->pTxBuffer = (((PUINT) pSpiPrivate->pTxBuffer) + dwDmaLen);
            }

            TimeOutCount = TXDONE_TIMEOUT_COUNT;
            while((((pSPIregs ->SPI_STATUS>>6) & 0x7f) && !(pSPIregs ->SPI_STATUS & TX_DONE)) &&
                TimeOutCount > 0)
            {
                if(pSpiPrivate->dwMode == SPI_SLAVE_MODE)
                {
                    // In SLAVE mode, TX could take longer because the master has to initiate the read
                    // So a busy wait with a large TimeoutCount is inefficient. Adding a small Sleep and 
                    // keeping the same TimeoutCount as master mode is better.
                    Sleep(SLAVE_TXWAIT_SLEEP_INTERVAL);
                }
                TimeOutCount--;
            }

            if (TimeOutCount == 0)
            {
                RETAILMSG (SPI_ERR, (TEXT("SPI: ThreadForTx: Timeout waiting for TXDONE status.\r\n")));
            }
        }
        else
        // POLLING + TX
        {
            ULONG Count;
            RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : USE Polling (TxCount : %d) \r\n"), dwTxCount));

            pSPIregs->CH_CFG     = pSpiPrivate->TxSPIregs.CH_CFG;
            pSPIregs->CLK_CFG     = pSpiPrivate->TxSPIregs.CLK_CFG;
            pSPIregs->MODE_CFG  = pSpiPrivate->TxSPIregs.MODE_CFG;

            pSPIregs->CH_CFG         |=    TX_CH_ON;

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : MASTER MODE \r\n")));
                MASTER_CS_ENABLE;
            }
            else
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for TX : SLAVE MODE \r\n")));
            }

            TotalTimeOut = WRITE_TIME_OUT_CONSTANT*100*50 + (WRITE_TIME_OUT_MULTIPLIER)*dwTxCount;
            Count = 0;
            do
            {
                while(((pSPIregs ->SPI_STATUS>>6) & 0x7f)==FIFO_FULL)
                {
                    Count++;
                    if(TotalTimeOut == Count)
                    {
                        // Timeout
                        RETAILMSG (TRUE, (TEXT("Write timeout!!!\r\n")));
                        goto LEAVEWRITE;
                    }
                }
                pSPIregs->SPI_TX_DATA = *(PBYTE)pSpiPrivate->pTxBuffer;
            } while(--pSpiPrivate->dwTxCount > 0 && ++(PBYTE)pSpiPrivate->pTxBuffer);

            TimeOutCount = TXDONE_TIMEOUT_COUNT;
            while((((pSPIregs ->SPI_STATUS>>6) & 0x7f) && !(pSPIregs ->SPI_STATUS & TX_DONE)) &&
                TimeOutCount > 0)
            {
                if(pSpiPrivate->dwMode == SPI_SLAVE_MODE)
                {
                    // In SLAVE mode, TX could take longer because the master has to initiate the read
                    // So a busy wait with a large TimeoutCount is inefficient. Adding a small Sleep and 
                    // keeping the same TimeoutCount as master mode is better.
                    Sleep(SLAVE_TXWAIT_SLEEP_INTERVAL);
                }
                TimeOutCount--;
            }

            if (TimeOutCount == 0)
            {
                RETAILMSG (SPI_ERR, (TEXT("SPI: ThreadForTx: Timeout waiting for TXDONE status.\r\n")));
            }
        }

        LEAVEWRITE:

#ifdef TEST_MODE
        do
        {
            RETAILMSG(SPI_MSG,(TEXT("[SPI] WRITE BYTE : %02X(dwTxCount : %d)\n"), *pTestBuffer, dwTestCount));
        } while( (--dwTestCount > 0) && ++pTestBuffer);
#endif

        RETAILMSG(FALSE,(TEXT("[SPI] TX_CH_OFF \n")));
        pSPIregs->CH_CFG     &= ~TX_CH_ON;    
        if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
        {
            MASTER_CS_DISABLE;
        }

        SetEvent(pSpiPublic->hTxDoneEvent);
    } while(TRUE);

    return 0;
}

DWORD ThreadForRx(PSPI_PUBLIC_CONTEXT pSpiPublic)
{
    volatile S3C6410_SPI_REG    *pSPIregs   = NULL;
    PSPI_PRIVATE_CONTEXT        pSpiPrivate;

    DWORD     dwRxCount;
    PBYTE     pRxBuffer;

    PBYTE     pTestBuffer;
    DWORD     dwTestCount;

    ULONG    TimeOut;
    ULONG    TotalTimeOut;
    ULONG    WaitReturn;

    if(pSpiPublic != NULL)
    {
        pSPIregs       = pSpiPublic->pSPIregs;
    }

    do
    {
        WaitForSingleObject(pSpiPublic->hRxEvent, INFINITE);

        pSpiPrivate = (PSPI_PRIVATE_CONTEXT) pSpiPublic->pSpiPrivate;

        if(pSpiPrivate->dwRxCount != 0)
        {
            dwTestCount = dwRxCount = pSpiPrivate->dwRxCount;
        }

        pTestBuffer = pRxBuffer = pSpiPrivate->pRxBuffer;

        RETAILMSG(SPI_MSG,(TEXT("[SPI] pRxBuffer : 0x%X, dwRxCount : %d \r\n"), pRxBuffer, dwRxCount));

        //Reset
        pSPIregs->CH_CFG |= SW_RST;
        while(!(pSPIregs->CH_CFG & SW_RST));
        RETAILMSG(SPI_MSG,(TEXT("[SPI] HS SPI reset\n")));
        pSPIregs->CH_CFG &= ~SW_RST;

        if(pSpiPrivate->bUseRxIntr)
        //INT Mode + RX
        {
            RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : USE INT \r\n")));
            pSpiPrivate->State = STATE_RXINTR;

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                pSPIregs->CH_CFG     = pSpiPrivate->RxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG      = pSpiPrivate->RxSPIregs.CLK_CFG;
                pSPIregs->MODE_CFG  = pSpiPrivate->RxSPIregs.MODE_CFG|TRAIL_CNT(0x3FF)|(RX_TRIG_LEVEL<<11);
                pSPIregs->PACKET_COUNT = PACKET_CNT_EN | (pSpiPrivate->dwRxCount) ;
            }
            else
            {
                pSPIregs->CH_CFG     = pSpiPrivate->RxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG      = pSpiPrivate->RxSPIregs.CLK_CFG; 
                pSPIregs->MODE_CFG  = pSpiPrivate->RxSPIregs.MODE_CFG|TRAIL_CNT(0x3FF)|(RX_TRIG_LEVEL<<11); 
            }

            pSPIregs->SPI_INT_EN        =    RX_FIFORDY;
            pSPIregs->PENDING_CLEAR        =    TX_UNDERRUN_CLR|TX_OVERRUN_CLR|RX_UNDERRUN_CLR|RX_OVERRUN_CLR|TRAILING_CLR;
            pSPIregs->CH_CFG            |=     RX_CH_ON;

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : MASTER MODE \r\n")));
                MASTER_CS_ENABLE;
            }
            else
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : SLAVE MODE \r\n")));
            }

            //Timeout value setting
            TotalTimeOut = READ_TIME_OUT_CONSTANT + (READ_TIME_OUT_MULTIPLIER/1000)*dwRxCount;
            TimeOut    =    TotalTimeOut;
            WaitReturn = WaitForSingleObject(pSpiPublic->hRxIntrDoneEvent, TimeOut);

            if ( WAIT_TIMEOUT == WaitReturn )
            {
                // Timeout
                RETAILMSG (TRUE, (TEXT("Read timeout!!!\r\n")));
                goto LEAVEREAD;
            }
        }

        else if(pSpiPrivate->bUseRxDMA)
        //DMA Mode + Rx
        {
            DWORD dwDmaLen    = (pSpiPrivate->dwRxCount & 0xFFFFF);

            RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : USE DMA \r\n")));

            pSpiPrivate->State = STATE_RXDMA;

            //Reset
            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                pSPIregs->CH_CFG     = pSpiPrivate->RxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG      = pSpiPrivate->RxSPIregs.CLK_CFG;
                pSPIregs->MODE_CFG  = pSpiPrivate->RxSPIregs.MODE_CFG;
                pSPIregs->PACKET_COUNT = PACKET_CNT_EN | (pSpiPrivate->dwRxCount) ;
            }
            else
            {
                pSPIregs->CH_CFG     = pSpiPrivate->RxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG      = pSpiPrivate->RxSPIregs.CLK_CFG;
                pSPIregs->MODE_CFG  = pSpiPrivate->RxSPIregs.MODE_CFG;
            }

            if(dwDmaLen > 0)
            {
                pSPIregs->MODE_CFG        |=    RX_DMA_ON|DMA_SINGLE;
                pSPIregs->CH_CFG         |=    RX_CH_ON;

                {
                    DMA_initialize_channel(&g_InputDMA, TRUE);
                    DMA_set_channel_source(&g_InputDMA, (UINT)SPI_RX_DATA_PHY_ADDR, BYTE_UNIT, BURST_1, FIXED);
                    DMA_set_channel_destination(&g_InputDMA, (UINT)pSpiPrivate->pRxDMABuffer, BYTE_UNIT, BURST_1, INCREASE);
                    DMA_set_channel_transfer_size(&g_InputDMA, dwDmaLen);
                    DMA_initialize_LLI(&g_InputDMA, 0);
                }

                if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
                {
                    RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : MASTER MODE \r\n")));
                    MASTER_CS_ENABLE;
                }
                else
                {
                    RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : SLAVE MODE \r\n")));
                }

                DMA_channel_start(&g_InputDMA);

                //Timeout value setting
                TotalTimeOut = READ_TIME_OUT_CONSTANT + (READ_TIME_OUT_MULTIPLIER/1000)*dwRxCount;
                TimeOut    =    TotalTimeOut;
                WaitReturn = WaitForSingleObject(pSpiPublic->hRxDmaDoneDoneEvent, TimeOut);

                if ( WAIT_TIMEOUT == WaitReturn )
                {
                    // Timeout
                    RETAILMSG (TRUE, (TEXT("Read timeout!!!\r\n")));
                    // Stop input DMA
                    DMA_channel_stop(&g_InputDMA);
                    goto LEAVEREAD;
                }

                pSpiPrivate->dwRxCount -= dwDmaLen;
                pSpiPrivate->pRxBuffer = (PBYTE) (((PUINT) pSpiPrivate->pRxBuffer) + dwDmaLen);
            }
        }
        else
        {
        //POLLING Mode + RX
            ULONG Count;
            RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : USE Polling (RxCount : %d) \r\n"), dwRxCount));

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                pSPIregs->CH_CFG     = pSpiPrivate->RxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG     = pSpiPrivate->RxSPIregs.CLK_CFG;
                pSPIregs->MODE_CFG  = pSpiPrivate->RxSPIregs.MODE_CFG;
                pSPIregs->PACKET_COUNT = PACKET_CNT_EN | (pSpiPrivate->dwRxCount) ;
            }
            else
            {
                pSPIregs->CH_CFG     = pSpiPrivate->RxSPIregs.CH_CFG;
                pSPIregs->CLK_CFG     = pSpiPrivate->RxSPIregs.CLK_CFG;
                pSPIregs->MODE_CFG  = pSpiPrivate->RxSPIregs.MODE_CFG;
            }

            pSPIregs->CH_CFG     |= RX_CH_ON;

            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : MASTER MODE \r\n")));
                MASTER_CS_ENABLE;
            }
            else
            {
                RETAILMSG(SPI_MSG,(TEXT("[SPI] Thread for RX : SLAVE MODE \r\n")));
            }

            TotalTimeOut = READ_TIME_OUT_CONSTANT*100*50 + (READ_TIME_OUT_MULTIPLIER)*dwRxCount;
            Count = 0;
            do
            {
                while (((pSPIregs ->SPI_STATUS>>13)&0x7f)==FIFO_EMPTY)
                {
                    Count++;
                    if(TotalTimeOut == Count)
                    {
                        // Timeout
                        RETAILMSG (TRUE, (TEXT("Read timeout!!!\r\n")));
                        goto LEAVEREAD;
                    }
                }
                *(PBYTE)pSpiPrivate->pRxBuffer = pSPIregs->SPI_RX_DATA;
            } while(--pSpiPrivate->dwRxCount > 0 && ++(PBYTE)pSpiPrivate->pRxBuffer);
        }

        LEAVEREAD:

#ifdef TEST_MODE
        do
        {
            RETAILMSG(SPI_MSG,(TEXT("[SPI] READ BYTE : %02X(dwRxCount %d)\n"), *pTestBuffer, dwTestCount));
        } while((--dwTestCount > 0) && ++pTestBuffer);
#endif

        RETAILMSG(FALSE,(TEXT("[SPI] RX_CH_OFF \n")));
        pSPIregs->CH_CFG     &= ~RX_CH_ON;
        if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
        {
            MASTER_CS_DISABLE;
        }

        SetEvent(pSpiPublic->hRxDoneEvent);
    } while(TRUE);

    return 0;
}

DWORD ThreadForSpi(PSPI_PUBLIC_CONTEXT pSpiPublic)
{
    volatile S3C6410_SPI_REG *pSPIregs  = pSpiPublic->pSPIregs;        // for HS-SPI
    PSPI_PRIVATE_CONTEXT pSpiPrivate;
    DWORD     dwRxCount;
    DWORD     dwTxCount;

    RETAILMSG(SPI_MSG,(TEXT("[SPI] ThreadForSpi thread is created \r\n")));
    do
    {
        WaitForSingleObject(pSpiPublic->hSpiEvent, INFINITE);

        pSpiPrivate = (PSPI_PRIVATE_CONTEXT) pSpiPublic->pSpiPrivate;

        if(pSpiPrivate->State == STATE_RXINTR)
        {
            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                do
                {
                    while (((pSPIregs ->SPI_STATUS>>13)&0x7f)==FIFO_EMPTY);
                    *(PBYTE)pSpiPrivate->pRxBuffer = pSPIregs->SPI_RX_DATA;
                } while(--pSpiPrivate->dwRxCount > 0 && ++(PBYTE)pSpiPrivate->pRxBuffer);

                if(pSpiPrivate->dwRxCount ==0)
                {
                    pSPIregs->SPI_INT_EN    &=    ~(RX_FIFORDY|TX_FIFORDY);
                    SetEvent(pSpiPublic->hRxIntrDoneEvent);
                }
            }
            else
            { 
                dwRxCount = ((pSPIregs ->SPI_STATUS>>13) & 0x7f) ;
                do
                {
                    *(PBYTE)pSpiPrivate->pRxBuffer = pSPIregs->SPI_RX_DATA;
                } while(--pSpiPrivate->dwRxCount > 0 && ++(PBYTE)pSpiPrivate->pRxBuffer && --dwRxCount > 0);

                if(pSpiPrivate->dwRxCount ==0)
                {
                    pSPIregs->SPI_INT_EN    &=    ~(RX_FIFORDY|TX_FIFORDY);
                    SetEvent(pSpiPublic->hRxIntrDoneEvent);
                } 
            }
        }

        else if(pSpiPrivate->State == STATE_TXINTR)
        {
            if(pSpiPrivate->dwMode == SPI_MASTER_MODE)
            {
                dwTxCount = FIFO_SIZE -((pSPIregs ->SPI_STATUS>>6) & 0x7f) ;
                do
                {
                    pSPIregs->SPI_TX_DATA = *(PBYTE)pSpiPrivate->pTxBuffer;
                } while(--pSpiPrivate->dwTxCount > 0 && ++(PBYTE)pSpiPrivate->pTxBuffer && --dwTxCount > 0);

                if(pSpiPrivate->dwTxCount ==0)
                {
                    pSPIregs->SPI_INT_EN    &=    ~(RX_FIFORDY|TX_FIFORDY);
                    SetEvent(pSpiPublic->hTxIntrDoneEvent);
                }
            }
            else
            { 
                dwTxCount = FIFO_SIZE -((pSPIregs ->SPI_STATUS>>6) & 0x7f) ;
                do
                {
                    pSPIregs->SPI_TX_DATA = *(PBYTE)pSpiPrivate->pTxBuffer;
                } while(--pSpiPrivate->dwTxCount > 0 && ++(PBYTE)pSpiPrivate->pTxBuffer && --dwTxCount > 0);

                if(pSpiPrivate->dwTxCount ==0)
                {
                    pSPIregs->SPI_INT_EN    &=    ~(RX_FIFORDY|TX_FIFORDY);
                    SetEvent(pSpiPublic->hTxIntrDoneEvent);
                } 
            }
        }
        else
        {
            RETAILMSG(SPI_MSG,(TEXT("[SPI] UNSOLVED OPERATION\n")));
        }

//END_POINT:
        InterruptDone(pSpiPublic->dwSpiSysIntr);
    } while(TRUE);
    return 0;
}


DWORD ThreadForRxDmaDone(PSPI_PUBLIC_CONTEXT pSpiPublic)
{
    do
    {
        WaitForSingleObject(pSpiPublic->hRxDmaDoneEvent, INFINITE);

        RETAILMSG(SPI_MSG,(TEXT("[SPI] ThreadForRxDmaDone \r\n")));

        DMA_set_interrupt_mask(&g_InputDMA);
        DMA_clear_interrupt_pending(&g_InputDMA);

        SetEvent(pSpiPublic->hRxDmaDoneDoneEvent);
        InterruptDone(pSpiPublic->dwRxDmaDoneSysIntr);

        DMA_clear_interrupt_mask(&g_InputDMA);

    } while(TRUE);
    return 0;
}

DWORD ThreadForTxDmaDone(PSPI_PUBLIC_CONTEXT pSpiPublic)
{
    do
    {
        WaitForSingleObject(pSpiPublic->hTxDmaDoneEvent, INFINITE);

        RETAILMSG(SPI_MSG,(TEXT("[SPI] ThreadForTxDmaDone \r\n")));

        DMA_set_interrupt_mask(&g_OutputDMA);
        DMA_clear_interrupt_pending(&g_OutputDMA);

        SetEvent(pSpiPublic->hTxDmaDoneDoneEvent);
        InterruptDone(pSpiPublic->dwTxDmaDoneSysIntr);

        DMA_clear_interrupt_mask(&g_OutputDMA);

    } while(TRUE);
    return 0;
}

void SPI_PowerDown (PSPI_PUBLIC_CONTEXT pPublicSpi)
{
    RETAILMSG(SPI_MSG,(TEXT("[SPI] ++PowerDown()\n\r")));

    if((PSPI_PUBLIC_CONTEXT)pPublicSpi == NULL)
    {
        goto CleanUp;
    }

    //Save SPI Reg
    pRestoreSPIregs->CH_CFG     = pPublicSpi->pSPIregs->CH_CFG;
    pRestoreSPIregs->CLK_CFG    = pPublicSpi->pSPIregs->CLK_CFG;
    pRestoreSPIregs->MODE_CFG   = pPublicSpi->pSPIregs->MODE_CFG;
    pRestoreSPIregs->SPI_INT_EN = pPublicSpi->pSPIregs->SPI_INT_EN;
    pRestoreSPIregs->SWAP_CFG   = pPublicSpi->pSPIregs->SWAP_CFG;
    pRestoreSPIregs->FB_CLK_SEL = pPublicSpi->pSPIregs->FB_CLK_SEL;

    // Clock Off
    pPublicSpi->pSYSCONregs->PCLK_GATE &= ~SPI_POWER_ON;
#if    (SPI_CLOCK == EPLL_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE &= ~SPI_SCLK_ON;
#elif (SPI_CLOCK == USB_HOST_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE &= ~SPI_USBHOST_ON;
#endif

    RETAILMSG(SPI_MSG,(TEXT("[SPI] --PowerDown()\n\r")));

CleanUp:
    return;
}

void SPI_Deinit (PSPI_PUBLIC_CONTEXT pPublicSpi)
{
    if((PSPI_PUBLIC_CONTEXT)pPublicSpi == NULL)
    {
        goto CleanUp;
    }

    // Delete CS
    DeleteCriticalSection(&(pPublicSpi->CsRxAccess));
    DeleteCriticalSection(&(pPublicSpi->CsTxAccess));

    // Deinitialize Buffer
    HalFreeCommonBuffer(0, 0, PhysDmaDstBufferAddr, pVirtDmaDstBufferAddr, FALSE);
    HalFreeCommonBuffer(0, 0, PhysDmaSrcBufferAddr, pVirtDmaSrcBufferAddr, FALSE);

    // Clear value assigned to DMA physical Address
    DmaDstAddress = 0;
    DmaSrcAddress = 0;

    // DMA Channel Stop
    DMA_channel_stop(&g_OutputDMA);
    DMA_channel_stop(&g_InputDMA);
    DMA_release_channel(&g_OutputDMA);
    DMA_release_channel(&g_InputDMA);

    // Clock Off
    pPublicSpi->pSYSCONregs->PCLK_GATE &= ~SPI_POWER_ON;
#if    (SPI_CLOCK == EPLL_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE &= ~SPI_SCLK_ON;
#elif (SPI_CLOCK == USB_HOST_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE &= ~SPI_USBHOST_ON;
#endif

    // Assign SYSINTR_NOP value
    pPublicSpi->dwSpiSysIntr = SYSINTR_NOP;

    // Close Handle
    if (pPublicSpi->hRxEvent != NULL)
    {
        CloseHandle(pPublicSpi->hRxEvent);
    }
    if (pPublicSpi->hRxDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hRxDoneEvent);
    }
    if (pPublicSpi->hRxIntrDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hRxIntrDoneEvent);
    }
    if (pPublicSpi->hTxEvent != NULL)
    {
        CloseHandle(pPublicSpi->hTxEvent);
    }
    if (pPublicSpi->hTxDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hTxDoneEvent);
    }
    if (pPublicSpi->hTxIntrDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hTxIntrDoneEvent);
    }
    if (pPublicSpi->hSpiEvent != NULL)
    {
        CloseHandle(pPublicSpi->hSpiEvent);
    }
    if (pPublicSpi->hTxDmaDoneDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hTxDmaDoneDoneEvent);
    }
    if (pPublicSpi->hTxDmaDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hTxDmaDoneEvent);
    }
    if (pPublicSpi->hRxDmaDoneDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hRxDmaDoneDoneEvent);
    }
    if (pPublicSpi->hRxDmaDoneEvent != NULL)
    {
        CloseHandle(pPublicSpi->hRxDmaDoneEvent);
    }
    if (pPublicSpi->hRxThread != NULL)
    {
        CloseHandle(pPublicSpi->hRxThread);
    }
    if (pPublicSpi->hTxThread != NULL)
    {
        CloseHandle(pPublicSpi->hTxThread);
    }
    if (pPublicSpi->hSpiThread != NULL)
    {
        CloseHandle(pPublicSpi->hSpiThread);
    }
    if (pPublicSpi->hTxDmaDoneThread != NULL)
    {
        CloseHandle(pPublicSpi->hTxDmaDoneThread);
    }
    if (pPublicSpi->hRxDmaDoneThread != NULL)
    {
        CloseHandle(pPublicSpi->hRxDmaDoneThread);
    }
   
    // VirtualFree
    if (pPublicSpi->pGPIOregs)
    {
        MmUnmapIoSpace((PVOID)pPublicSpi->pGPIOregs, sizeof(S3C6410_GPIO_REG));
        pPublicSpi->pGPIOregs = NULL;
    }

    if (pPublicSpi->pSPIregs)
    {
        MmUnmapIoSpace((PVOID)pPublicSpi->pSPIregs, sizeof(S3C6410_SPI_REG));
        pPublicSpi->pSPIregs = NULL;
    }

    if (pPublicSpi->pDMAC0regs)
    {
        MmUnmapIoSpace((PVOID)pPublicSpi->pDMAC0regs, sizeof(S3C6410_DMAC_REG));
        pPublicSpi->pDMAC0regs = NULL;
    }

    if (pPublicSpi->pDMAC1regs)
    {
        MmUnmapIoSpace((PVOID)pPublicSpi->pDMAC1regs, sizeof(S3C6410_DMAC_REG));
        pPublicSpi->pDMAC1regs = NULL;
    }

    if (pPublicSpi->pSYSCONregs)
    {
        MmUnmapIoSpace((PVOID)pPublicSpi->pSYSCONregs, sizeof(S3C6410_SYSCON_REG));
        pPublicSpi->pSYSCONregs = NULL;
    }

    // Local Free
    LocalFree(pRestoreSPIregs);
    LocalFree(pPublicSpi);

CleanUp:
    return;
}

void SPI_PowerUp (PSPI_PUBLIC_CONTEXT pPublicSpi)
{
    RETAILMSG(SPI_MSG,(TEXT("[SPI] ++PowerUp()\n\r")));

    if((PSPI_PUBLIC_CONTEXT)pPublicSpi == NULL)
    {
        goto CleanUp;
    }

    // Clock On
    pPublicSpi->pSYSCONregs->PCLK_GATE |= SPI_POWER_ON;
#if    (SPI_CLOCK == EPLL_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE |= SPI_SCLK_ON;
#elif (SPI_CLOCK == USB_HOST_CLOCK)
    pPublicSpi->pSYSCONregs->SCLK_GATE |= SPI_USBHOST_ON;
#endif

    //Restore SPI Reg
    pPublicSpi->pSPIregs->CH_CFG     = pRestoreSPIregs->CH_CFG;
    pPublicSpi->pSPIregs->CLK_CFG    = pRestoreSPIregs->CLK_CFG;
    pPublicSpi->pSPIregs->MODE_CFG   = pRestoreSPIregs->MODE_CFG;
    pPublicSpi->pSPIregs->SPI_INT_EN = pRestoreSPIregs->SPI_INT_EN;
    pPublicSpi->pSPIregs->SWAP_CFG   = pRestoreSPIregs->SWAP_CFG;
    pPublicSpi->pSPIregs->FB_CLK_SEL = pRestoreSPIregs->FB_CLK_SEL;

    RETAILMSG(SPI_MSG,(TEXT("[SPI] --PowerUp()\n\r")));

CleanUp:
    return;
}

BOOL SPI_Close (DWORD dwOpen)
{
    PSPI_PUBLIC_CONTEXT         pSpiPublic  = NULL;
    BOOL bResult = TRUE;

    if(dwOpen != 0)
    {
        pSpiPublic  = (PSPI_PUBLIC_CONTEXT) dwOpen;
    }
    else
    {
        bResult = FALSE;
        goto CleanUp;
    }

    // Free all data allocated in open
    LocalFree( pSpiPublic );

CleanUp:
    return bResult;
}

