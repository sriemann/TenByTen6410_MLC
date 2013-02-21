//-------------------------------------------------------------------------------------------------------------------------
// Copyright (c) Samsung Electronics Co., Ltd.  All rights reserved.
//-------------------------------------------------------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
//
// Module Name :    S3C6410_UART_DMA.C
//
// Abstract     :   UART Interface Routines for Samsung S3C6410 CPU
//
// Environment :    Samsung S3C6410 / WM 6.1
//
// 2008/10/
//
//-------------------------------------------------------------------------------------------------------------------------

#include <windows.h>
#include <nkintr.h>
#include <ceddk.h>    // For DMA Buffer Alloc
#include <memory.h>

#include <bsp.h>
#include <oal_intr.h>
#include <S3C6410_dma_controller.h>
#include <pddS3C6410_ser_dma.h>

#define UART_DMA_INIT 0
#define UART_MSG 0

PDMA_PUBLIC_CONTEXT  pPublicUart = NULL;

DWORD DMA_Init(PDMA_PUBLIC_CONTEXT pPublicUart);
BOOL InitializeBuffer();
PVOID InitializeDMA(DWORD nCH);

DWORD ThreadForRxDmaDone(PDMA_PUBLIC_CONTEXT pPublicUart);
DWORD ThreadForTxDmaDone(PDMA_PUBLIC_CONTEXT pPublicUart);

DMA_CH_CONTEXT   g_OutputDMA;
DMA_CH_CONTEXT   g_InputDMA;

//DMA
UINT DmaDstAddress;
UINT DmaSrcAddress;

//Memory Physical Address
PHYSICAL_ADDRESS PhysDmaDstBufferAddr;
PHYSICAL_ADDRESS PhysDmaSrcBufferAddr;

// DMA Buffer Address Alloc
PBYTE pVirtDmaDstBufferAddr = NULL;
PBYTE pVirtDmaSrcBufferAddr = NULL;


DWORD DMA_Init( PDMA_PUBLIC_CONTEXT pPublicUart)
{
    BOOL bResult = TRUE;
    PHYSICAL_ADDRESS ioPhysicalBase = {0, 0};
    
    // Syscon Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;      
    pPublicUart->pSYSCONregs = (volatile S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (pPublicUart->pSYSCONregs == NULL)
    {
        RETAILMSG(UART_DMA_INIT,(TEXT("[UART] For pSYSCONregs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

/*    // MDMA
    // MDMAC Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_MDMA;
    pPublicUart->pDMACregs = (volatile S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (pPublicUart->pDMACregs == NULL)
    {
        RETAILMSG(UART_DMA_INIT,(TEXT("[UART] For pDMACregs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }
*/

    // DMAC0 Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA0;      
    pPublicUart->pDMAC0regs = (volatile S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (pPublicUart->pDMAC0regs == NULL)
    {
        RETAILMSG(UART_DMA_INIT,(TEXT("[UART] For pDMAC0regs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    // DMAC1 Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA1;      
    pPublicUart->pDMAC1regs = (volatile S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (pPublicUart->pDMAC1regs == NULL)
    {
        RETAILMSG(UART_DMA_INIT,(TEXT("[UART] For pDMAC1regs: MmMapIoSpace failed!\r\n")));
        bResult = FALSE;
        goto CleanUp;
    }

CleanUp:

    if (!bResult)
    {
        return bResult;
    }


    DMA_initialize_register_address((void *)pPublicUart->pDMAC0regs,
                                    (void *)pPublicUart->pDMAC1regs,
                                    (void *)pPublicUart->pSYSCONregs);

/*
    DMA_initialize_register_address((void *)pPublicUart->pDMAC0regs,
                                    (void *)pPublicUart->pDMAC1regs,
                                    (void *)pPublicUart->pDMACregs,
                                    (void *)pPublicUart->pSYSCONregs);
*/

    return bResult;
}

BOOL InitializeBuffer()
{
    BOOL bResult = TRUE;

    DMA_ADAPTER_OBJECT Adapter1, Adapter2;

    RETAILMSG(UART_DMA_INIT,(TEXT("+++[UART] InitializeBuffer\n")));
    memset(&Adapter1, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter1.InterfaceType = Internal;
    Adapter1.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    memset(&Adapter2, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter2.InterfaceType = Internal;
    Adapter2.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    // Allocate a block of virtual memory (physically contiguous) for the DMA buffers.
    //
    pVirtDmaDstBufferAddr = (PBYTE)HalAllocateCommonBuffer(&Adapter1, Buffer_Mem_Size, &PhysDmaDstBufferAddr, FALSE);
    RETAILMSG(FALSE, (TEXT("[UART] InitializeBuffer() - pVirtDmaDstBufferAddr %x\r\n"),pVirtDmaDstBufferAddr));

    if (pVirtDmaDstBufferAddr == NULL)
    {
        RETAILMSG(UART_DMA_INIT, (TEXT("[UART] InitializeBuffer() - Failed to allocate DMA buffer for UART.\r\n")));
        HalFreeCommonBuffer(0, 0, PhysDmaDstBufferAddr, pVirtDmaDstBufferAddr, FALSE);
        bResult = FALSE;
        goto CleanUp;
    }

    pVirtDmaSrcBufferAddr = (PBYTE)HalAllocateCommonBuffer(&Adapter2, Buffer_Mem_Size, &PhysDmaSrcBufferAddr, FALSE);
    RETAILMSG(FALSE, (TEXT("[UART] InitializeBuffer() - pVirtDmaSrcBufferAddr %x\r\n"),pVirtDmaSrcBufferAddr));

    if (pVirtDmaSrcBufferAddr == NULL)
    {
        RETAILMSG(UART_DMA_INIT, (TEXT("[UART] InitializeBuffer() - Failed to allocate DMA buffer for UART\r\n")));
        HalFreeCommonBuffer(0, 0, PhysDmaSrcBufferAddr, pVirtDmaSrcBufferAddr, FALSE);
        bResult = FALSE;
        goto CleanUp;
    }

    //DMA Address
    DmaDstAddress = (UINT)(PhysDmaDstBufferAddr.LowPart);
    DmaSrcAddress = (UINT)(PhysDmaSrcBufferAddr.LowPart);

    RETAILMSG(UART_DMA_INIT, (TEXT("[UART] pVirtDmaSrcBufferAddr 0x%x   DmaSrcAddress 0x%x \r\n"),pVirtDmaSrcBufferAddr, DmaSrcAddress));
    RETAILMSG(UART_DMA_INIT, (TEXT("[UART] pVirtDmaDstBufferAddr 0x%x   DmaDstAddress 0x%x \r\n"),pVirtDmaDstBufferAddr, DmaDstAddress));

    RETAILMSG(UART_DMA_INIT,(TEXT("-[UART] InitializeBuffer\n")));

CleanUp:
    return bResult;
}

BOOL InitializeChannel(PDMA_PUBLIC_CONTEXT pPublicUart)
{
    BOOL bResult=TRUE;
    int  DMASrc=DMA_UART0_TX + (pPublicUart->m_chnum*2);

    //TX DMA Init    
    if( DMA_request_channel(&g_OutputDMA, (DMA_SOURCE)DMASrc) != TRUE )
    {
        RETAILMSG(1,(TEXT("[UART] DMA UART TX channel request is failed\n")));
        bResult = FALSE;   
    }

    /*RETAILMSG(UART_DMA_INIT,(TEXT("[UART] pUartPrivate->pTxDMABuffer : 0x%X \r\n"), DmaSrcAddress));
    dma_error_value = DMA_initialize_channel(&g_OutputDMA, TRUE);
    RETAILMSG(UART_DMA_INIT,(TEXT("[UART] 1 %d \r\n"),dma_error_value));
    dma_error_value = DMA_set_channel_source(&g_OutputDMA, (UINT)DmaSrcAddress, BYTE_UNIT, BURST_1, INCREASE);
    RETAILMSG(UART_DMA_INIT,(TEXT("[UART] 2 %d \r\n"),dma_error_value));
    dma_error_value = DMA_set_channel_destination(&g_OutputDMA, (UINT)UART1_TX_DATA_PHY_ADDR, BYTE_UNIT, BURST_1, FIXED);
    RETAILMSG(UART_DMA_INIT,(TEXT("[UART] 3 %d \r\n"),dma_error_value));
    DMA_initialize_LLI(&g_OutputDMA, 1);                     
    DMA_set_LLI_entry(&g_OutputDMA, 0, LLI_FIRST_ENTRY, (UINT)DmaSrcAddress,
            (UINT)UART1_TX_DATA_PHY_ADDR, UART_BUF_SIZE);*/    

    return bResult;    

}

PVOID InitializeDMA(DWORD nCH)
{

    BOOL  bResult = TRUE;
    DWORD dwHwIntr=0;

    RETAILMSG(UART_DMA_INIT,(TEXT("++[UART] InitializeDMA Function\r\n")));
  
    if ( !(pPublicUart = (PDMA_PUBLIC_CONTEXT)LocalAlloc( LPTR, sizeof(DMA_PUBLIC_CONTEXT) )) )
    {
        RETAILMSG(1,(TEXT("[UART] Can't not allocate for UART Context\n")));
        bResult = FALSE;
        goto CleanUp;
    }

    pPublicUart->m_chnum = nCH;
    pPublicUart->dwTxDmaThreadPrio = TX_DMA_THREAD_PRIORITY;

    if(!InitializeBuffer())
    {
        RETAILMSG(1,(TEXT("[UART] InitializeBuffer is failed\n")));
        bResult = FALSE;
        goto CleanUp;
    }
    

    DMA_Init(pPublicUart);
    InitializeChannel(pPublicUart);

    do
    {
        //Tx DMA Done ISR
        pPublicUart->dwTxDmaDoneSysIntr = SYSINTR_NOP;
        dwHwIntr = g_OutputDMA.dwIRQ;

        pPublicUart->hTxDmaDoneDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        pPublicUart->hTxDmaDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwHwIntr, sizeof(DWORD), &pPublicUart->dwTxDmaDoneSysIntr, sizeof(DWORD), NULL))
        {
            RETAILMSG(1,(TEXT("[UART] Failed to request the UART_DMA sysintr.\n")));
            pPublicUart->dwTxDmaDoneSysIntr = SYSINTR_UNDEFINED;
            bResult = FALSE;
            break;
        }

        if (!InterruptInitialize(pPublicUart->dwTxDmaDoneSysIntr, pPublicUart->hTxDmaDoneEvent, NULL, 0))
        {
            RETAILMSG(1,(TEXT("[UART] DMA Interrupt Initialization failed!!!\n")));
            bResult = FALSE;
            break;
        }

        pPublicUart->hTxDmaDoneThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadForTxDmaDone, (LPVOID)pPublicUart, 0, (LPDWORD)&pPublicUart->dwTxDmaDoneThreadId);
        if (pPublicUart->hTxDmaDoneThread == NULL)
        {
            RETAILMSG(1,(TEXT("[UART] UART Dma Thread creation error!!!\n")));
            bResult = FALSE;
            break;
        }

    } while (0);

CleanUp:
    RETAILMSG(UART_DMA_INIT,(TEXT("--[UART] InitializeDMA  Function\r\n")));
    if(bResult)
    {
        return pPublicUart;
    }
    else
    {
        return NULL;
    }
}




#ifdef USE_RX_DMA 
DWORD ThreadForRxDmaDone(PDMA_PUBLIC_CONTEXT pUartPublic)
{
    do
    {
        WaitForSingleObject(pUartPublic->hRxDmaDoneEvent, INFINITE);

        RETAILMSG(UART_MSG,(TEXT("[UART] ThreadForRxDmaDone \r\n")));

        DMA_clear_interrupt_pending(&g_InputDMA);
        DMA_set_interrupt_mask(&g_InputDMA);

        SetEvent(pUartPublic->hRxDmaDoneDoneEvent);
        InterruptDone(pUartPublic->dwRxDmaDoneSysIntr);

        DMA_clear_interrupt_mask(&g_InputDMA);

    } while(TRUE);
    return 0;
}
#endif 

DWORD ThreadForTxDmaDone(PDMA_PUBLIC_CONTEXT pUartPublic)
{
    if(CeSetThreadPriority( pPublicUart->hTxDmaDoneThread, 99))
    {
        RETAILMSG(UART_MSG,(TEXT("[UART] Priority setting sucess \r\n")));
    }    
    do
    {
        RETAILMSG(0,(TEXT("[UART] ThreadForTxDmaDone Waiting\r\n")));
        WaitForSingleObject(pUartPublic->hTxDmaDoneEvent, INFINITE);

        RETAILMSG(UART_MSG,(TEXT("[UART] ThreadForTxDmaDoneDone \r\n")));

        DMA_clear_interrupt_pending(&g_OutputDMA);
        DMA_set_interrupt_mask(&g_OutputDMA);

        SetEvent(pUartPublic->hTxDmaDoneDoneEvent);
        InterruptDone(pUartPublic->dwTxDmaDoneSysIntr);

        DMA_clear_interrupt_mask(&g_OutputDMA);

    } while(TRUE);
    return 0;
}


BOOL DmaPowerUp()
{
    BOOL bResult =TRUE;
    return bResult;
}

BOOL DmaPowerDown()
{
    BOOL bResult =TRUE;
    return bResult;

}

