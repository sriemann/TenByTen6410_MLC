//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

Abstract:

    Serial PDD for SamSang 6410 UART Common Code.

Notes: 
--*/
#include <windows.h>
#include <types.h>
#include <ceddk.h>

#include <bsp.h>
#include <ddkreg.h>
#include <serhw.h>
#include <Serdbg.h>
#include <pdds3c6410_ser.h>
#include <s3c6410_base_regs.h>
#ifdef USE_DMA
#include <s3c6410_dma_controller.h>
#include "pdds3c6410_ser_dma.h"
#endif 

#define    EPLL_CLK    0

#ifdef USE_DMA
#define UART_DMA_MSG         0
#define UART_TX_FIFO_LEN    16
#endif 

CReg6410Uart::CReg6410Uart(PULONG pRegAddr)
:   m_pReg(pRegAddr)
{
    m_fIsBackedUp = FALSE;
    m_ULCONBackup = NULL;
    m_UCONBackup = NULL;
    m_UFCONBackup = NULL;
    m_UMCOMBackup = NULL;
    m_UBRDIVBackup = NULL;
    m_UDIVSLOTBackup = NULL;
    m_UINTMBackup = NULL;

    m_BaudRate = NULL;
    m_s3c6410_pclk = S3C6410_PCLK;
}
BOOL   CReg6410Uart::Init() 
{
    BOOL bRet = TRUE;

    if (m_pReg)
    { // Set Value to default.
        Write_ULCON(0);
        Write_UCON(0);
        Write_UFCON(0);
        Write_UMCON(0);
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

void CReg6410Uart::Backup()
{
#ifdef USE_DMA
    DmaPowerDown();
#endif    
    m_ULCONBackup = Read_ULCON();
    m_UCONBackup = Read_UCON();
    m_UFCONBackup = Read_UFCON();
    m_UMCOMBackup = Read_UMCON();
    m_UBRDIVBackup = Read_UBRDIV();    
    m_UDIVSLOTBackup = Read_UDIVSLOT();
    m_UINTMBackup = Read_UINTM();
    m_fIsBackedUp = TRUE;
}
void CReg6410Uart::Restore()
{
    if (m_fIsBackedUp) {
        Write_ULCON(m_ULCONBackup );
       //================================[
       // RxD FIFO Reset
        DWORD dwBit =Read_UFCON();
        // Reset RxD Fifo.
        dwBit |= (1<<1);
        dwBit &= ~(1<<0);
        Write_UFCON( dwBit);

        // Enable RxD FIFO.
        dwBit &= ~(1<<1);
        dwBit |= (1<<0);
        Write_UFCON(dwBit); // RxD Fifo Reset Done..
       //================================]
       
        Write_UFCON( m_UFCONBackup );
        Write_UMCON( m_UMCOMBackup );
        Write_UBRDIV( m_UBRDIVBackup);
        Write_UDIVSLOT( m_UDIVSLOTBackup );
    Write_UCON( m_UCONBackup );
        m_fIsBackedUp = FALSE;
#ifdef USE_DMA
    DmaPowerUp();
#endif            
    
    }
}
CReg6410Uart::Write_BaudRate(ULONG BaudRate)
{
    DOUBLE Div_val;
    UINT UDIVSLOTn = 0;
    UINT UBRDIV = 0;
    BOOL bRet = TRUE;

    DEBUGCHK(BaudRate!= NULL);

    DEBUGMSG(ZONE_INIT, (TEXT("SetBaudRate -> %d\r\n"), BaudRate));

    if ( (Read_UCON() & UART_CS_MASK) == UART_CS_PCLK )
    {
        Div_val = (m_s3c6410_pclk/16.0/BaudRate);
        UBRDIV = (int)Div_val - 1;
        Write_UBRDIV( UBRDIV );
        UDIVSLOTn = (int)( (Div_val - (int)Div_val) * 16);
        Write_UDIVSLOT( UDIVSLOT_TABLE[UDIVSLOTn] );
        RETAILMSG( FALSE , (TEXT("CLK:%d, BaudRate:%d, UBRDIV:%d, UDIVSLOTn:%d\r\n"), m_s3c6410_pclk, BaudRate, UBRDIV, UDIVSLOTn));
        bRet = TRUE;
    }
    else if( (Read_UCON() & UART_CS_MASK) == UART_CS_EPLLCLK )
    {
        Div_val = (S3C6410_ECLK/16.0/BaudRate);
        UBRDIV = (int)Div_val - 1;
        Write_UBRDIV( UBRDIV );
        UDIVSLOTn = (int)( (Div_val - (int)Div_val) * 16);
        Write_UDIVSLOT( UDIVSLOT_TABLE[UDIVSLOTn] );
        RETAILMSG( FALSE , (TEXT("CLK:%d, BaudRate:%d, UBRDIV:%d, UDIVSLOTn:%d\r\n"), S3C6410_ECLK, BaudRate, UBRDIV, UDIVSLOTn));
        bRet = TRUE;
    }
    else
    {
        RETAILMSG(TRUE, (TEXT("ERROR: The s3c6410a serial driver doesn't support an external UART clock.\r\n")));
        ASSERT(FALSE);
        bRet =FALSE;
    }

    return bRet;
}
#ifdef DEBUG
void CReg6410Uart::DumpRegister()
{
    NKDbgPrintfW(TEXT("DumpRegister (ULCON=%x, UCON=%x, UFCON=%x, UMCOM = %x, UBDIV =%x)\r\n"),
        Read_ULCON(),Read_UCON(),Read_UFCON(),Read_UMCON(),Read_UBRDIV());
}
#endif

CPdd6410Uart::CPdd6410Uart (LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj )
:   CSerialPDD(lpActivePath,pMdd, pHwObj)
,   m_ActiveReg(HKEY_LOCAL_MACHINE,lpActivePath)
,   CMiniThread (0, TRUE)   
{
    m_pReg6410Uart = NULL;
    m_dwSysIntr = MAXDWORD;
    m_hISTEvent = NULL;
    m_dwDevIndex = 0;
    m_pRegVirtualAddr = NULL;
    m_XmitFlushDone =  CreateEvent(0, FALSE, FALSE, NULL);
    m_XmitFifoEnable = FALSE;
    m_dwWaterMark = NULL ;
    m_ClockSelectValid = FALSE;
    m_AutoFlowEnabled = FALSE;
    m_UseAutoFlow = FALSE;
}
CPdd6410Uart::~CPdd6410Uart()
{
    InitModem(FALSE);
    if (m_hISTEvent)
    {
        m_bTerminated=TRUE;
        ThreadStart();
        SetEvent(m_hISTEvent);
        ThreadTerminated(1000);
        InterruptDisable( m_dwSysIntr );         
        CloseHandle(m_hISTEvent);
    };
    if (m_pReg6410Uart)
    {
        delete m_pReg6410Uart;
    }
    if (m_XmitFlushDone)
    {
        CloseHandle(m_XmitFlushDone);
    }
    if (m_pRegVirtualAddr != NULL)
    {
        MmUnmapIoSpace((PVOID)m_pRegVirtualAddr,sizeof(S3C6410_UART_REG));
    }        
}
BOOL CPdd6410Uart::Init()
{
    BOOL bRet = TRUE;

    if ( CSerialPDD::Init() && IsKeyOpened() && m_XmitFlushDone!=NULL)
    { 
        // IST Setup .
        DDKISRINFO ddi;
        if (GetIsrInfo(&ddi)!=ERROR_SUCCESS)
        {
            bRet = FALSE;
            goto CleanUp;
        }
        m_dwSysIntr = ddi.dwSysintr;
        if (m_dwSysIntr !=  MAXDWORD && m_dwSysIntr!=0 )
        {
            m_hISTEvent= CreateEvent(0,FALSE,FALSE,NULL);
        }

        if (m_hISTEvent!=NULL)
        {
            InterruptInitialize(m_dwSysIntr,m_hISTEvent,0,0);
        }
        else
        {
            bRet = FALSE;
            goto CleanUp;
        }

        // Get Device Index.
        if (!GetRegValue(PC_REG_DEVINDEX_VAL_NAME, (PBYTE)&m_dwDevIndex, PC_REG_DEVINDEX_VAL_LEN))
        {
            m_dwDevIndex = 0;
        }
        if (!GetRegValue(PC_REG_SERIALWATERMARK_VAL_NAME,(PBYTE)&m_dwWaterMark,PC_REG_SERIALWATERMARKER_VAL_LEN))
        {
            m_dwWaterMark = DEFAULT_VALUE_WATER_MARK;
        }
        if (!GetRegValue(PC_REG_6410UART_IST_TIMEOUTS_VAL_NAME,(PBYTE)&m_dwISTTimeout, PC_REG_6410UART_IST_TIMEOUTS_VAL_LEN))
        {
            m_dwISTTimeout = INFINITE;
        }
        if (!GetRegValue(PC_REG_6410UART_MEM_LENGTH_VAL_NAME, (PBYTE)&m_dwMemLen, PC_REG_6410UART_MEM_LENGTH_VAL_LEN))
        {
            m_dwMemLen = DEFAULT_VALUE_MEM_LENGH;
        }
        if (!MapHardware()  || !CreateHardwareAccess())
        {
            bRet = FALSE;
            goto CleanUp;
        }
#ifdef USE_DMA
        if (!GetRegValue(PC_REG_TX_DMA_EN_NAME,(PBYTE)&m_dwTXDMAEnable, sizeof(DWORD)))
        {
            m_dwTXDMAEnable = FALSE;
        }  
        if(m_dwTXDMAEnable)
        {
            RETAILMSG(1, (L"[UART] DMA init CH:%d \r\n", m_dwDevIndex));
            InitializeDMA(m_dwDevIndex);
        }    
#endif 
        bRet = TRUE;
        goto CleanUp;
    }
    bRet = FALSE;
CleanUp:
    return bRet;
}
BOOL CPdd6410Uart::MapHardware() 
{
    if (m_pRegVirtualAddr !=NULL)
    {
        return TRUE;
    }

    // Get IO Window From Registry
    DDKWINDOWINFO dwi;
    if ( GetWindowInfo( &dwi)!=ERROR_SUCCESS || 
    dwi.dwNumMemWindows < 1 || 
    dwi.memWindows[0].dwBase == 0 || 
    dwi.memWindows[0].dwLen < m_dwMemLen)
    {
        return FALSE;
    }

    DWORD dwInterfaceType;
    if (m_ActiveReg.IsKeyOpened() && 
    m_ActiveReg.GetRegValue( DEVLOAD_INTERFACETYPE_VALNAME, (PBYTE)&dwInterfaceType,sizeof(DWORD)))
    {
        dwi.dwInterfaceType = dwInterfaceType;
    }

    // Translate to System Address.
    PHYSICAL_ADDRESS    ioPhysicalBase = { dwi.memWindows[0].dwBase, 0};
    ULONG                inIoSpace = 0;
    if (TranslateBusAddr(m_hParent,(INTERFACE_TYPE)dwi.dwInterfaceType,dwi.dwBusNumber, ioPhysicalBase,&inIoSpace,&ioPhysicalBase))
    {
        // Map it if it is Memeory Mapped IO.
        m_pRegVirtualAddr = MmMapIoSpace(ioPhysicalBase, dwi.memWindows[0].dwLen,FALSE);
    }

    return (m_pRegVirtualAddr!=NULL );
}

BOOL CPdd6410Uart::CreateHardwareAccess()
{
    BOOL bRet = TRUE;

    if (m_pReg6410Uart)
    {
        bRet = TRUE;
        goto CleanUp;
    }
    if (m_pRegVirtualAddr!=NULL)
    {
        m_pReg6410Uart = new CReg6410Uart((PULONG)m_pRegVirtualAddr);
        if (m_pReg6410Uart && !m_pReg6410Uart->Init())
        {
            delete m_pReg6410Uart ;
            m_pReg6410Uart = NULL;
        }    
    }
    bRet = (m_pReg6410Uart!=NULL);

CleanUp:
    return bRet;
}
#define MAX_RETRY 0x1000
void CPdd6410Uart::PostInit()
{
    DWORD dwCount=0;
    m_HardwareLock.Lock();
    m_pReg6410Uart->Write_UCON(0); // Set to Default;
    DisableInterrupt(S6410UART_INT_RXD | S6410UART_INT_TXD | S6410UART_INT_ERR | S6410UART_INT_MODEM);
    // Mask all interrupt.
    while ((GetInterruptStatus() & (S6410UART_INT_RXD | S6410UART_INT_TXD | S6410UART_INT_ERR | S6410UART_INT_MODEM))!=0 && dwCount <MAX_RETRY)
    {
        InitReceive(TRUE);
        InitLine(TRUE);
        ClearInterrupt(S6410UART_INT_RXD | S6410UART_INT_TXD | S6410UART_INT_ERR | S6410UART_INT_MODEM);
        dwCount++;
    } 
    ASSERT((GetInterruptStatus() & (S6410UART_INT_RXD | S6410UART_INT_TXD | S6410UART_INT_ERR | S6410UART_INT_MODEM))==0);
    m_HardwareLock.Unlock();
    CSerialPDD::PostInit();
    CeSetPriority(m_dwPriority256);
#ifdef DEBUG
    if ( ZONE_INIT )
    {
        m_pReg6410Uart->DumpRegister();
    }
#endif
    ThreadStart();  // Start IST.
}
DWORD CPdd6410Uart::ThreadRun()
{
    DWORD dwData;
    DWORD interrupts;
    
    while ( m_hISTEvent!=NULL && !IsTerminated() )
    {
        if ( WaitForSingleObject( m_hISTEvent, m_dwISTTimeout) == WAIT_OBJECT_0)
        {
            m_HardwareLock.Lock();    
        
            while ( !IsTerminated() )
            {
                dwData = ( GetInterruptStatus() & (S6410UART_INT_RXD | S6410UART_INT_TXD | S6410UART_INT_ERR | S6410UART_INT_MODEM) );
                if (dwData)
                {
                    DEBUGMSG(ZONE_THREAD, (TEXT(" CPdd6410Uart::ThreadRun Active INT=%x\r\n"), dwData));

                    // Clear the interrupt value to notify to MDD
                    interrupts=NULL;
                    DEBUGCHK(interrupts==NULL);

                    if ((dwData & S6410UART_INT_RXD)!=0)
                    {
                        interrupts |= INTR_RX;
                    }
                    if ((dwData & S6410UART_INT_TXD)!=0)
                    {
                        interrupts |= INTR_TX;
                    }
                    if ((dwData & S6410UART_INT_ERR)!=0)
                    {
                        interrupts |= INTR_LINE | INTR_RX;
                    }
                    if ((dwData & S6410UART_INT_MODEM)!=0)
                    {
                        interrupts |=INTR_MODEM;
                    }
                    
                    NotifyPDDInterrupt( (INTERRUPT_TYPE)interrupts );
                    
                    ClearInterrupt(dwData);
                }
                else
                {
                    break;
                }
            }

            m_HardwareLock.Unlock();   

            InterruptDone(m_dwSysIntr);
        }
        else
        {
            DEBUGMSG(ZONE_THREAD,(TEXT(" CPdd6410Uart::ThreadRun timeout INT=%x,MASK=%d\r\n"),m_pReg6410Uart->Read_UINTP()/*m_pReg6410Uart->Read_UINTSP()*/, m_pReg6410Uart->Read_UINTM()) );
            ASSERT(FALSE);
        }
    }

    return 1;
}
BOOL CPdd6410Uart::InitialEnableInterrupt(BOOL bEnable )
{
    m_HardwareLock.Lock();
    if (bEnable)
    {
        EnableInterrupt(S6410UART_INT_RXD | S6410UART_INT_ERR | S6410UART_INT_MODEM);
    }
    else
    {
        DisableInterrupt(S6410UART_INT_RXD | S6410UART_INT_ERR | S6410UART_INT_MODEM);
    }
    m_HardwareLock.Unlock();
    return TRUE;
}

BOOL  CPdd6410Uart::InitXmit(BOOL bInit)
{
    DWORD dwTicks = 0;
    DWORD dwUTRState;

    if (bInit)
    { 
        m_HardwareLock.Lock();    
        DWORD dwBit = m_pReg6410Uart->Read_UCON();
#if EPLL_CLK
        // Set UART CLK.
        dwBit &= ~(UART_CS_MASK);
        dwBit |= (UART_CS_EPLLCLK);
#endif
        if (m_ClockSelectValid) {
            dwBit &= ~(UART_CS_MASK);
            dwBit |= (m_ClockSelect);
        }

 
        // Set Tx Inerrupt Request Type, Tx Interrupt/Polling Mode.
        dwBit &= ~(UART_TX_INT_TYPE_MASK|UART_TX_MODE_MASK);
        dwBit |= (UART_TX_INT_TYPE_LEVEL|UART_TX_INT_POLL);
        m_pReg6410Uart->Write_UCON(dwBit );

        dwBit = m_pReg6410Uart->Read_UFCON();
        // Reset Xmit Fifo.
        dwBit |= (UART_FCR_TX_FIFO_RESET);
        dwBit &= ~(UART_FCR_FIFO_ENABLE);
        m_pReg6410Uart->Write_UFCON( dwBit);
        // Set Trigger level to 16. 
        dwBit &= ~(UART_FCR_TX_FIFO_TRIG_MASK);
        dwBit |= (UART_FCR_TX_FIFO_TRIG_16);//16
        m_pReg6410Uart->Write_UFCON(dwBit); 
        // Enable Xmit FIFO.
        dwBit &= ~(UART_FCR_TX_FIFO_RESET);
        dwBit |= (UART_FCR_FIFO_ENABLE);
        m_pReg6410Uart->Write_UFCON(dwBit); // Xmit Fifo Reset Done..
        m_HardwareLock.Unlock();
#ifdef USE_DMA
        if(m_dwTXDMAEnable)
        {
            InitXmitDMA();
        }
#endif 
    }
    else 
    { // Make Sure data has been trasmit out.
        // We have to make sure the xmit is complete because MDD will shut down the device after this return
        while (dwTicks < TIMEOUT_TX_EMPTY && ((dwUTRState = m_pReg6410Uart->Read_UTRSTAT()) & (UART_TRANSMITTER_STATUS_MASK | UART_TX_BUFFER_STATUS_MASK))
            != (UART_TRANSMITTER_EMPTY | UART_TX_BUFFER_EMPTY))
        { // Transmitter empty is not true
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE, (TEXT("CPdd6410Uart::InitXmit! Wait for UTRSTAT=%x clear.\r\n"), dwUTRState));
            Sleep(5);
            dwTicks +=5;
        }
    }
    return TRUE;
}
DWORD   CPdd6410Uart::GetWriteableSize()
{
    DWORD dwWriteSize = 0;
    DWORD dwUfState = m_pReg6410Uart->Read_UFSTAT() ;
    if ((dwUfState & UART_FSR_TX_FIFO_MASK)==UART_FSR_TX_FIFO_NOT_FULL)
    { // It is not full.
        dwUfState = ((dwUfState & UART_FSR_TX_FIFO_COUNT_MASK) >> UART_FSR_TX_FIFO_COUNT_SHFIT); // It is fifo count.
        if (dwUfState < S3C6410_UART_FIFO_DEPTH-1)
        {
            dwWriteSize = S3C6410_UART_FIFO_DEPTH-1 - dwUfState;
        }
    }
    return dwWriteSize;
}

#ifdef USE_DMA
void    CPdd6410Uart::XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen)
{
    BOOL bRet;
    PREFAST_DEBUGCHK(pBuffLen!=NULL);
    m_HardwareLock.Lock();

    //wait until TX FIFO empty 
    // 6ms : 38400 baudrate/64Byte
    // 85us : 3M baudrate/64Byte    
    // It affects when BT power on and off at low baudrate    
    if((m_dwTXDMAEnable==1) && (*pBuffLen < UART_TX_FIFO_LEN))
    {
        while(((m_pReg6410Uart->Read_UFSTAT()>>8) & 0x3f))
        {
                    RETAILMSG(FALSE, (TEXT("[UART] Wait \r\n")));
        }
    } 
    
    if (*pBuffLen == 0)
    {
        EnableXmitInterrupt(FALSE);
    }
    else
    {
        DEBUGCHK(pTxBuffer);
        PulseEvent(m_XmitFlushDone);
        DWORD dwDataAvaiable = *pBuffLen;
        *pBuffLen = 0;
        DMA_ERROR dma_error_value = DMA_SUCCESS;

        Rx_Pause(TRUE);
        if ((m_DCB.fOutxCtsFlow && IsCTSOff()) ||(m_DCB.fOutxDsrFlow && IsDSROff()))
        { // We are in flow off
            RETAILMSG(FALSE, (TEXT("CPddS3CXUart::XmitInterruptHandler! Flow Off, Data Discard.\r\n")));
            EnableXmitInterrupt(FALSE);
        }
        else
        {
            if((m_dwTXDMAEnable==1)&&(dwDataAvaiable> UART_TX_FIFO_LEN/*UART_TX_FIFO_LEN*/))            
            {
                DWORD dwDmaLen = (dwDataAvaiable > Buffer_Mem_Size)? Buffer_Mem_Size:dwDataAvaiable;
                
                RETAILMSG(0,(TEXT("[UART] Thread for TX : USE DMA (TxCount : %d) \r\n"),dwDmaLen));

                if(dwDmaLen > 0)
                {
                    bRet = StartXmitDMA(pTxBuffer, dwDmaLen);
                    if(!bRet)
                    {
                        goto LEAVEWRITE;
                    }
                                dwDataAvaiable -= dwDmaLen;
                    pTxBuffer = (PUCHAR)(((PUINT) pTxBuffer) + dwDmaLen);
                }

                *pBuffLen = dwDmaLen;
                        EnableXmitInterrupt(TRUE); 

                
   }
            
   else
   {
                DWORD dwWriteSize = GetWriteableSize();

                RETAILMSG(0,(TEXT("[UART] XmitInterruptHandler! WriteableSize=%x to FIFO,dwDataAvaiable=%x\r\n"),
                    dwWriteSize,dwDataAvaiable));

                for (DWORD dwByteWrite=0; dwByteWrite<dwWriteSize && dwDataAvaiable!=0;dwByteWrite++)
                {
                    m_pReg6410Uart->Write_UTXH(*pTxBuffer);
                    pTxBuffer ++;
                    dwDataAvaiable--;
                }

                RETAILMSG(0,(TEXT("[UART] XmitInterruptHandler! Write %d byte to FIFO\r\n"),dwByteWrite));

                *pBuffLen = dwByteWrite;
                EnableXmitInterrupt(TRUE);        
            }                

        }
LEAVEWRITE:
        ClearInterrupt(S6410UART_INT_TXD);
        
        if (m_pReg6410Uart->Read_ULCON() & (1 << 6))
        {
            while((m_pReg6410Uart->Read_UFSTAT() & (0x3F << 8)) >> (8));
        }
        Rx_Pause(FALSE);
    }
    m_HardwareLock.Unlock();
   }
#else//#ifdef USE_DMA
void    CPdd6410Uart::XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen)
{
    PREFAST_DEBUGCHK(pBuffLen!=NULL);
    m_HardwareLock.Lock();    
    if (*pBuffLen == 0)
    { 
        EnableXmitInterrupt(FALSE);
    }
    else
    {
        DEBUGCHK(pTxBuffer);
        PulseEvent(m_XmitFlushDone);
        DWORD dwDataAvaiable = *pBuffLen;
        *pBuffLen = 0;

        Rx_Pause(TRUE);
        if (!m_AutoFlowEnabled && ((m_DCB.fOutxCtsFlow && IsCTSOff()) ||(m_DCB.fOutxDsrFlow && IsDSROff())))
        { // We are in flow off
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE, (TEXT("CPdd6410Uart::XmitInterruptHandler! Flow Off, Data Discard.\r\n")));
            EnableXmitInterrupt(FALSE);
        }
        else
        {
            DWORD dwWriteSize = GetWriteableSize();
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE,(TEXT("CPdd6410Uart::XmitInterruptHandler! WriteableSize=%x to FIFO,dwDataAvaiable=%x\r\n"),
                dwWriteSize,dwDataAvaiable));
            for (DWORD dwByteWrite=0; dwByteWrite<dwWriteSize && dwDataAvaiable!=0;dwByteWrite++)
            {
                m_pReg6410Uart->Write_UTXH(*pTxBuffer);
                pTxBuffer ++;
                dwDataAvaiable--;
            }
            DEBUGMSG(ZONE_THREAD|ZONE_WRITE,(TEXT("CPdd6410Uart::XmitInterruptHandler! Write %d byte to FIFO\r\n"),dwByteWrite));
            *pBuffLen = dwByteWrite;
            EnableXmitInterrupt(TRUE);        
        }
        ClearInterrupt(S6410UART_INT_TXD);

        if (m_pReg6410Uart->Read_ULCON() & UART_LCR_IR_MODE)
        {
            while((m_pReg6410Uart->Read_UFSTAT() & UART_FSR_TX_FIFO_COUNT_MASK) >> UART_FSR_TX_FIFO_COUNT_SHFIT);
        }
        Rx_Pause(FALSE);
    }
    m_HardwareLock.Unlock();
}
#endif //#ifdef USE_DMA

void    CPdd6410Uart::XmitComChar(UCHAR ComChar)
{
    // This function has to poll until the Data can be sent out.
    BOOL bDone = FALSE;
    do
    {
        m_HardwareLock.Lock(); 
        if ( GetWriteableSize()!=0 )
        {  // If not full 
            m_pReg6410Uart->Write_UTXH(ComChar);
            bDone = TRUE;
        }
        else
        {
            EnableXmitInterrupt(TRUE);
        }
            m_HardwareLock.Unlock();
        if (!bDone)
        {
            WaitForSingleObject(m_XmitFlushDone, (ULONG)1000);
        }
    }while (!bDone);
}
BOOL    CPdd6410Uart::EnableXmitInterrupt(BOOL fEnable)
{
    m_HardwareLock.Lock();
    if (fEnable)
    {
        EnableInterrupt(S6410UART_INT_TXD);
    }
    else
    {
        DisableInterrupt(S6410UART_INT_TXD);
    }
    m_HardwareLock.Unlock();
    return TRUE; 
}
BOOL  CPdd6410Uart::CancelXmit()
{
    return InitXmit(TRUE);     
}
static PAIRS s_HighWaterPairs[] =
{
    {0, 1 },
    {1, 8 },
    {2, 16 },
    {3, 32 }
};

BYTE  CPdd6410Uart::GetWaterMarkBit()
{
    BYTE bReturnKey = (BYTE)s_HighWaterPairs[0].Key;
    for (DWORD dwIndex = dim(s_HighWaterPairs)-1 ; dwIndex != 0; dwIndex --)
    {
        if (m_dwWaterMark >= s_HighWaterPairs[dwIndex].AssociatedValue)
        {
            bReturnKey = (BYTE)s_HighWaterPairs[dwIndex].Key;
            break;
        }
    }
    return bReturnKey;
}
DWORD   CPdd6410Uart::GetWaterMark()
{
    BYTE bReturnValue = (BYTE)s_HighWaterPairs[0].AssociatedValue;
    for (DWORD dwIndex = dim(s_HighWaterPairs)-1 ; dwIndex != 0; dwIndex --)
    {
        if (m_dwWaterMark >= s_HighWaterPairs[dwIndex].AssociatedValue)
        {
            bReturnValue = (BYTE)s_HighWaterPairs[dwIndex].AssociatedValue;
            break;
        }
    }
    return bReturnValue;
}

// Receive
BOOL    CPdd6410Uart::InitReceive(BOOL bInit)
{
    m_HardwareLock.Lock();    
    if (bInit)
    {         
        BYTE uWarterMarkBit = GetWaterMarkBit();
        if (uWarterMarkBit> 3)
        {
            uWarterMarkBit = 3;
        }
        // Setup Receive FIFO.
        // Reset Receive Fifo.
        DWORD dwBit = m_pReg6410Uart->Read_UFCON();
        dwBit |= (UART_FCR_RX_FIFO_RESET);
        dwBit &= ~(UART_FCR_FIFO_ENABLE);
        m_pReg6410Uart->Write_UFCON( dwBit);
        // Set Trigger level to WaterMark.
        dwBit &= ~(UART_FCR_RX_FIFO_TRIG_MASK);
        dwBit |= (uWarterMarkBit<<4);
        m_pReg6410Uart->Write_UFCON(dwBit); 
        // Enable Receive FIFO.
        dwBit &= ~(UART_FCR_RX_FIFO_RESET);
        dwBit |= (UART_FCR_FIFO_ENABLE);
        m_pReg6410Uart->Write_UFCON(dwBit); // Receive FIFO Reset Done..
        m_pReg6410Uart->Read_UERSTAT(); // Clean Line Interrupt.
        dwBit = m_pReg6410Uart->Read_UCON();
#if EPLL_CLK
        // Set UART CLK.
        dwBit &= ~(UART_CS_MASK);
        dwBit |= (UART_CS_EPLLCLK);
#endif
        if (m_ClockSelectValid) {
            dwBit &= ~(UART_CS_MASK);
            dwBit |= (m_ClockSelect);
        }
        // Set Rx Inerrupt Request Type, Rx Timeout, Rx Interrupt/Polling Mode.
        dwBit &= ~(UART_RX_INT_TYPE_MASK|UART_RX_TIMEOUT_MASK|UART_RX_MODE_MASK);
        dwBit |= (UART_RX_INT_TYPE_LEVEL|UART_RX_TIMEOUT_ENABLE|UART_RX_INT_POLL);
        m_pReg6410Uart->Write_UCON(dwBit);
        EnableInterrupt(S6410UART_INT_RXD | S6410UART_INT_ERR );
    }
    else
    {
        DisableInterrupt(S6410UART_INT_RXD | S6410UART_INT_ERR );
    }
    m_HardwareLock.Unlock();
    return TRUE;
}
ULONG   CPdd6410Uart::ReceiveInterruptHandler(PUCHAR pRxBuffer,ULONG *pBufflen)
{
    DEBUGMSG(ZONE_THREAD|ZONE_READ,(TEXT("+CPdd6410Uart::ReceiveInterruptHandler pRxBuffer=%x,*pBufflen=%x\r\n"),
        pRxBuffer,pBufflen!=NULL?*pBufflen:0));
    DWORD dwBytesDropped = 0;
    if (pRxBuffer && pBufflen )
    {
        DWORD dwBytesStored = 0 ;
        DWORD dwRoomLeft = *pBufflen;
        m_bReceivedCanceled = FALSE;
        m_HardwareLock.Lock();

        while (dwRoomLeft && !m_bReceivedCanceled)
        {
            ULONG ulUFSTATE = m_pReg6410Uart->Read_UFSTAT();
            DWORD dwNumRxInFifo = ((ulUFSTATE & UART_FSR_RX_FIFO_COUNT_MASK) >> UART_FSR_RX_FIFO_COUNT_SHIFT);
            if ((ulUFSTATE & (UART_FSR_RX_FIFO_MASK))== UART_FSR_RX_FIFO_FULL) // Overflow.;
            {
                dwNumRxInFifo = S3C6410_UART_FIFO_DEPTH;
            }
            DEBUGMSG(ZONE_THREAD|ZONE_READ,(TEXT("CPdd6410Uart::ReceiveInterruptHandler ulUFSTATE=%x,UTRSTAT=%x, dwNumRxInFifo=%X\r\n"),
                ulUFSTATE, m_pReg6410Uart->Read_UTRSTAT(), dwNumRxInFifo));
            if (dwNumRxInFifo)
            {
                ASSERT((m_pReg6410Uart->Read_UTRSTAT() & UART_RX_BUFFER_STATUS_MASK)!= UART_RX_BUFFER_EMPTY);
                while (dwNumRxInFifo && dwRoomLeft)
                {
                    UCHAR uLineStatus = GetLineStatus();
                    UCHAR uData = m_pReg6410Uart->Read_URXH();
                    if (DataReplaced(&uData,(uLineStatus & UART_LSR_PE)!=0))
                    {
                        *pRxBuffer++ = uData;
                        dwRoomLeft--;
                        dwBytesStored++;
                    }
                    dwNumRxInFifo --;
                }
            }
            else
            {
                break;
            }
        }
        if (m_bReceivedCanceled)
        {
            dwBytesStored = 0;
        }

        m_HardwareLock.Unlock();
        *pBufflen = dwBytesStored;
    }
    else
    {
        ASSERT(FALSE);
    }
    DEBUGMSG(ZONE_THREAD|ZONE_READ,(TEXT("-CPdd6410Uart::ReceiveInterruptHandler pRxBuffer=%x,*pBufflen=%x,dwBytesDropped=%x\r\n"),
        pRxBuffer,pBufflen!=NULL?*pBufflen:0,dwBytesDropped));
    return dwBytesDropped;
}
ULONG   CPdd6410Uart::CancelReceive()
{
    m_bReceivedCanceled = TRUE;
    m_HardwareLock.Lock();   
    InitReceive(TRUE);
    m_HardwareLock.Unlock();
    return 0;
}
BOOL    CPdd6410Uart::InitModem(BOOL bInit)
{
    m_HardwareLock.Lock();
    m_pReg6410Uart->Write_UMCON( m_pReg6410Uart->Read_UMCON()
        |UART_MCR_AUTO_FLOW_DISABLE
        |UART_MCR_MODEM_INTERRUPT_ENABLE
        |UART_MCR_SET_RTS); 
    m_HardwareLock.Unlock();
    return TRUE;
}

ULONG   CPdd6410Uart::GetModemStatus()
{
    m_HardwareLock.Lock();    
    ULONG ulReturn =0 ;
    ULONG Events = 0;
    UINT8 ubModemStatus = (UINT8) m_pReg6410Uart->Read_UMSTAT();
    m_HardwareLock.Unlock();

    // Event Notification.
    if (ubModemStatus & (UART_MSR_DCTS))
    {
        Events |= EV_CTS;
    }
    if (Events!=0)
    {
        EventCallback(Events);
    }

    // Report Modem Status.
    if ( ubModemStatus & (UART_MSR_CTS))
    {
        ulReturn |= MS_CTS_ON;
    }
    return ulReturn;
}
void    CPdd6410Uart::SetRTS(BOOL bSet)
{
    if (m_AutoFlowEnabled) {
        return;
    }

    m_HardwareLock.Lock();
    ULONG ulData = m_pReg6410Uart->Read_UMCON();
    if (bSet)
    {
        ulData |= (UART_MCR_SET_RTS);
    }
    else
    {
        ulData &= ~(UART_MCR_SET_RTS);
    }
    m_pReg6410Uart->Write_UMCON(ulData);
    m_HardwareLock.Unlock();

}
BOOL CPdd6410Uart::InitLine(BOOL bInit)
{
    m_HardwareLock.Lock();
    if  (bInit)
    {
        EnableInterrupt( S6410UART_INT_ERR );
    }
    else
    {
        DisableInterrupt(S6410UART_INT_ERR );
    }
    m_HardwareLock.Unlock();
    return TRUE;
}
BYTE CPdd6410Uart::GetLineStatus()
{
    m_HardwareLock.Lock();
    ULONG ulData = m_pReg6410Uart->Read_UERSTAT();
    m_HardwareLock.Unlock();  
    ULONG ulError = 0;
    if (ulData & (UART_LSR_OE) )
    {
        ulError |=  CE_OVERRUN;
    }
    if (ulData & (UART_LSR_PE))
    {
        ulError |= CE_RXPARITY;
    }
    if (ulData & (UART_LSR_FE))
    {
        ulError |=  CE_FRAME;
    }
    if (ulError)
    {
        SetReceiveError(ulError);
    }
    if (ulData & (UART_LSR_BI))
    {
        EventCallback(EV_BREAK);
    }
    return (UINT8)ulData;

}
void    CPdd6410Uart::SetBreak(BOOL bSet)
{
    m_HardwareLock.Lock();
    ULONG ulData = m_pReg6410Uart->Read_UCON();
    if (bSet)
    {
        ulData |= (UART_BREAK_SIGNAL_ENABLE);
    }
    else
    {
        ulData &= ~(UART_BREAK_SIGNAL_ENABLE);
    }
    m_pReg6410Uart->Write_UCON(ulData);
    m_HardwareLock.Unlock();      
}
BOOL    CPdd6410Uart::SetBaudRate(ULONG BaudRate,BOOL /*bIrModule*/)
{
    DWORD dwBit;

    m_HardwareLock.Lock();

    if (m_ClockSelectValid) {

        dwBit = m_pReg6410Uart->Read_UCON();

        dwBit &= ~(UART_CS_MASK);

        if (BaudRate < 10*115200) {
            m_ClockSelect = UART_CS_PCLK;
        } else {
            m_ClockSelect = UART_CS_EPLLCLK;
        }

        dwBit |= (m_ClockSelect);

        m_pReg6410Uart->Write_UCON(dwBit);
    }

    BOOL bReturn = m_pReg6410Uart->Write_BaudRate(BaudRate);
    m_HardwareLock.Unlock();      
    return TRUE;
}
BOOL    CPdd6410Uart::SetByteSize(ULONG ByteSize)
{
    BOOL bRet = TRUE;
    m_HardwareLock.Lock();
    ULONG ulData = (m_pReg6410Uart->Read_ULCON() & (~(UART_LCR_DATA_LENGTH_MASK)));
    switch ( ByteSize )
    {
    case 5:
        ulData |= (UART_LCR_DATA_LENGTH_5BIT);
        break;
    case 6:
        ulData |= (UART_LCR_DATA_LENGTH_6BIT);
        break;
    case 7:
        ulData |= (UART_LCR_DATA_LENGTH_7BIT);
        break;
    case 8:
        ulData |= (UART_LCR_DATA_LENGTH_8BIT);
        break;
    default:
        bRet = FALSE;
        break;
    }
    if (bRet)
    {
        m_pReg6410Uart->Write_ULCON(ulData);
    }
    m_HardwareLock.Unlock();
    return bRet;
}
BOOL    CPdd6410Uart::SetParity(ULONG Parity)
{
    BOOL bRet = TRUE;
    m_HardwareLock.Lock();
    ULONG ulData = (m_pReg6410Uart->Read_ULCON() & (~(UART_LCR_PARITY_MASK)));
    switch ( Parity )
    {
    case ODDPARITY:
        ulData |= (UART_LCR_PARITY_ODD);
        break;
    case EVENPARITY:
        ulData |= (UART_LCR_PARITY_EVEN);
        break;
    case MARKPARITY:
        ulData |= (UART_LCR_PARITY_FORCE_1);
        break;
    case SPACEPARITY:
        ulData |= (UART_LCR_PARITY_FORCE_0);
        break;
    case NOPARITY:
        break;
    default:
        bRet = FALSE;
        break;
    }
    if (bRet)
    {
        m_pReg6410Uart->Write_ULCON(ulData);
    }
    m_HardwareLock.Unlock();
    return bRet;
}
BOOL    CPdd6410Uart::SetStopBits(ULONG StopBits)
{
    BOOL bRet = TRUE;
    m_HardwareLock.Lock();
    ULONG ulData = (m_pReg6410Uart->Read_ULCON() & (~(UART_LCR_STOPBIT_MASK)));

    switch ( StopBits )
    {
    case ONESTOPBIT :
        ulData |= (UART_LCR_1_STOPBIT);
        break;
    case TWOSTOPBITS :
        ulData |= (UART_LCR_2_STOPBITS);
        break;
    default:
        bRet = FALSE;
        break;
    }
    if (bRet)
    {
        m_pReg6410Uart->Write_ULCON(ulData);
    }
    m_HardwareLock.Unlock();
    return bRet;
}
void    CPdd6410Uart::SetOutputMode(BOOL UseIR, BOOL Use9Pin)
{
    m_HardwareLock.Lock();
    CSerialPDD::SetOutputMode(UseIR, Use9Pin);
    ULONG ulData = (m_pReg6410Uart->Read_ULCON() & (~(UART_LCR_MODE_MASK)));
    ulData |= (UseIR?(UART_LCR_IR_MODE):(UART_LCR_NORMAL_MODE));
    m_pReg6410Uart->Write_ULCON(ulData);
    m_HardwareLock.Unlock();
}


#ifdef USE_DMA

BOOL CPdd6410Uart::InitXmitDMA(void)
{
    UINT UART_TXH = 0;
    UART_TXH = S3C6410_BASE_REG_PA_UART0 + (m_dwDevIndex*S3C6410_BASE_REG_OFFSET_UART)+0x20;
      
    DMA_initialize_channel(&g_OutputDMA, TRUE);
    if(DMA_set_channel_destination(&g_OutputDMA, UART_TXH, BYTE_UNIT, BURST_1, FIXED))
    {
        return FALSE;
    }

    return TRUE; 
}

BOOL    CPdd6410Uart::EnableTxDMA(BOOL fEnable)
{
    RETAILMSG(UART_DMA_MSG, (TEXT("CPddS3CXUart::EnableDMA(%d)\r\n"), fEnable));

    m_HardwareLock.Lock();
    DWORD dwBit = m_pReg6410Uart->Read_UCON();
    if (fEnable)
    {
    // for UART1 - DMA1
        dwBit &= ~(3<<2);
        dwBit |= (2<<2);  
    }
    else
    {
         // Set Interrupt Tx Mode.
        dwBit &= ~(3<<2);
        dwBit |= (1<<2);
    }
    m_pReg6410Uart->Write_UCON(dwBit );

    //
    dwBit = m_pReg6410Uart->Read_UFCON();
    if (fEnable)
    {
        dwBit &= ~(3<<6);//8
    }
    else
    {
         // Set Trigger level to 16.
        dwBit &= ~(3<<6);//empty
        dwBit |= (UART_FCR_TX_FIFO_TRIG_16);//16
    }
    m_pReg6410Uart->Write_UFCON(dwBit );    

    m_HardwareLock.Unlock();
    return TRUE;
}

BOOL CPdd6410Uart::StopXmitDMA(void)
{
   BOOL ret = FALSE;
//   UARTMSG(0, (TEXT("StopXmitDMA\r\n")));
    DMA_channel_stop(&g_OutputDMA);
   return ret;
}

BOOL CPdd6410Uart::StartXmitDMA(PUCHAR pTxBuffer, ULONG BuffLen)
{
    DWORD dwDmaLen = BuffLen;    
    DMA_ERROR dma_error_value = DMA_SUCCESS;
    ULONG    WaitReturn;
    DWORD interrupts=0;
    UINT UART_TXH = 0;
    UART_TXH = S3C6410_BASE_REG_PA_UART0 + (m_dwDevIndex*S3C6410_BASE_REG_OFFSET_UART)+0x20;

    RETAILMSG(UART_DMA_MSG,(TEXT("[UART] +StartXmitDMA (%d) \r\n"),dwDmaLen));
    EnableTxDMA(TRUE);

    memcpy(pVirtDmaSrcBufferAddr,pTxBuffer,dwDmaLen); 
      
    DMA_set_channel_source(&g_OutputDMA, (UINT)DmaSrcAddress, BYTE_UNIT, BURST_1, INCREASE);
    DMA_set_channel_destination(&g_OutputDMA, (UINT)UART1_TX_DATA_PHY_ADDR, BYTE_UNIT, BURST_1, FIXED);
    DMA_set_channel_transfer_size(&g_OutputDMA, dwDmaLen);                                                  
    DMA_channel_start(&g_OutputDMA); 
                            
    WaitReturn = WaitForSingleObject(pPublicUart->hTxDmaDoneDoneEvent, WRITE_TIME_OUT_CONSTANT);
    DMA_channel_stop(&g_OutputDMA);

    if ( WAIT_TIMEOUT == WaitReturn )
    {
        // Timeout
        RETAILMSG (1, (TEXT("[UART] TX DMA timeout!!!\r\n")));
        RETAILMSG (1, (TEXT("[UART] LEN:%d, TX:%d\r\n"), dwDmaLen, m_pReg6410Uart->Read_UFSTAT()>>8));                                         
        return FALSE;
    }
    
    EnableTxDMA(FALSE);
    RETAILMSG(UART_DMA_MSG,(TEXT("[UART] -StartXmitDMA\r\n")));
    return TRUE;
}

#endif //USE_DMA

