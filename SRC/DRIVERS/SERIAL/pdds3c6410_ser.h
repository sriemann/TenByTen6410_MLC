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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

Abstract:

    Platform dependent Serial definitions for S3C6410 UART  controller.

Notes: 
--*/
#ifndef __PDDS3C6410_SER_H_
#define __PDDS3C6410_SER_H_
#include <cserpdd.h>
#include <cmthread.h>
#include <S3c6410_intr.h>
#include <bsp_cfg.h>

#define    DEFAULT_S3C6410_PCLK    (S3C6410_PCLK)
#define    S3C6410_UART_FIFO_DEPTH  64

#define    DEFAULT_VALUE_WATER_MARK 8
#define    DEFAULT_VALUE_MEM_LENGH  0x40
#define    TIMEOUT_TX_EMPTY         1000

#define    PCLK_UART0               (1<<1)
#define    PCLK_UART1               (1<<2)
#define    PCLK_UART2               (1<<3)
#define    PCLK_UART3               (1<<4)
#define    SCLK_UART                (1<<5)

#define    DTR_PORT_NUMBER          6
#define    DSR_PORT_NUMBER          7

#ifdef USE_DMA
#define S3C6410_BASE_REG_OFFSET_UART 0x400
#endif     

/////////////////////////////////////////////////////////////////////////////////////////
//// S3C6410 UART Register Bit Definition

//UART Line Control Register
//Mode
#define    UART_LCR_NORMAL_MODE                     (0 << 6)
#define    UART_LCR_IR_MODE                         (1 << 6)
#define    UART_LCR_MODE_MASK                       (1 << 6)
//Parity
#define    UART_LCR_NO_PARITY                       (0 << 3)
#define    UART_LCR_PARITY_ODD                      (4 << 3)
#define    UART_LCR_PARITY_EVEN                     (5 << 3)
#define    UART_LCR_PARITY_FORCE_1                  (6 << 3)
#define    UART_LCR_PARITY_FORCE_0                  (7 << 3)
#define    UART_LCR_PARITY_MASK                     (7 << 3)
//Stop Bit
#define    UART_LCR_1_STOPBIT                       (0 << 2)
#define    UART_LCR_2_STOPBITS                      (1 << 2)
#define    UART_LCR_STOPBIT_MASK                    (1 << 2)
//Data Length
#define    UART_LCR_DATA_LENGTH_5BIT                (0 << 0)
#define    UART_LCR_DATA_LENGTH_6BIT                (1 << 0)
#define    UART_LCR_DATA_LENGTH_7BIT                (2 << 0)
#define    UART_LCR_DATA_LENGTH_8BIT                (3 << 0)
#define    UART_LCR_DATA_LENGTH_MASK                (3 << 0)

//UART Control Register
//Clock Selection
#define    UART_CS_PCLK                             (0 << 10)
#define    UART_CS_EXTCLK                           (1 << 10)
#define    UART_CS_EPLLCLK                          (3 << 10)
#define    UART_CS_MASK                             (3 << 10)
//Tx Interrupt Type
#define    UART_TX_INT_TYPE_PUSE                    (0 << 9)
#define    UART_TX_INT_TYPE_LEVEL                   (1 << 9)
#define    UART_TX_INT_TYPE_MASK                    (1 << 9)
//Rx Interrupt Type
#define    UART_RX_INT_TYPE_PUSE                    (0 << 8)
#define    UART_RX_INT_TYPE_LEVEL                   (1 << 8)
#define    UART_RX_INT_TYPE_MASK                    (1 << 8)
//Rx Time Out Enable
#define    UART_RX_TIMEOUT_DISABLE                  (0 << 7)
#define    UART_RX_TIMEOUT_ENABLE                   (1 << 7)
#define    UART_RX_TIMEOUT_MASK                     (1 << 7)
//Rx Error Status Interrupt Enable
#define    UART_RX_ERR_INT_DISABLE                  (0 << 6)
#define    UART_RX_ERR_INT_ENABLE                   (1 << 6)
#define    UART_RX_ERR_INT_MASK                     (1 << 6)
//Loop-back Mode
#define    UART_LOOPBACK_DISABLE                    (0 << 5)
#define    UART_LOOPBACK_ENABLE                     (1 << 5)
#define    UART_LOOPBACK_MASK                       (1 << 5)
//Send Break Signal
#define    UART_BREAK_SIGNAL_DISABLE                (0 << 4)
#define    UART_BREAK_SIGNAL_ENABLE                 (1 << 4)
#define    UART_BREAK_SIGNAL_MASK                   (1 << 4)
//Transmit Mode
#define    UART_TX_DISABLE                          (0 << 2)
#define    UART_TX_INT_POLL                         (1 << 2)
#define    UART_TX_DMA_REQEUST_0                    (2 << 2)
#define    UART_TX_DMA_REQEUST_1                    (3 << 2)
#define    UART_TX_MODE_MASK                        (3 << 2)
//Receive Mode
#define    UART_RX_DISABLE                          (0 << 0)
#define    UART_RX_INT_POLL                         (1 << 0)
#define    UART_RX_DMA_REQEUST_0                    (2 << 0)
#define    UART_RX_DMA_REQEUST_1                    (3 << 0)
#define    UART_RX_MODE_MASK                        (3 << 0)

//UART FIFO Control Register
//Tx FIFO Trigger Level
#define    UART_FCR_TX_FIFO_TRIG_EMPTY              (0 << 6)
#define    UART_FCR_TX_FIFO_TRIG_16                 (1 << 6)
#define    UART_FCR_TX_FIFO_TRIG_32                 (2 << 6)
#define    UART_FCR_TX_FIFO_TRIG_48                 (3 << 6)
#define    UART_FCR_TX_FIFO_TRIG_MASK               (3 << 6)
//Rx FIFO Trigger Level
#define    UART_FCR_RX_FIFO_TRIG_1                  (0 << 4)
#define    UART_FCR_RX_FIFO_TRIG_8                  (1 << 4)
#define    UART_FCR_RX_FIFO_TRIG_16                 (2 << 4)
#define    UART_FCR_RX_FIFO_TRIG_32                 (3 << 4)
#define    UART_FCR_RX_FIFO_TRIG_MASK               (3 << 4)
//Tx FIFO Reset
#define    UART_FCR_TX_FIFO_RESET                   (1 << 2)
//Rx FIFO Reset
#define    UART_FCR_RX_FIFO_RESET                   (1 << 1)
//FIFO Enable
#define    UART_FCR_FIFO_DISABLE                    (0 << 0)
#define    UART_FCR_FIFO_ENABLE                     (1 << 0)

//UART Modem Control Register
//RTS Trigger Level
#define    UART_MCR_RTS_RX_FIFO_TRIG_63             (0 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_56             (1 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_48             (2 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_40             (3 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_32             (4 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_24             (5 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_16             (6 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_8              (7 << 5)
#define    UART_MCR_RTS_RX_FIFO_TRIG_MASK           (7 << 5)
//Auto Flow Control (AFC)
#define    UART_MCR_AUTO_FLOW_DISABLE               (0 << 4)
#define    UART_MCR_AUTO_FLOW_ENABLE                (1 << 4)
#define    UART_MCR_AUTO_FLOW_MASK                  (1 << 4)
//Modem Interrupt Enable
#define    UART_MCR_MODEM_INTERRUPT_DISABLE         (0 << 3)
#define    UART_MCR_MODEM_INTERRUPT_ENABLE          (1 << 3)
#define    UART_MCR_MODEM_INTERRUPT_MASK            (1 << 3)
//Request To Send (RTS)
#define    UART_MCR_CLEAR_RTS                       (0 << 0)
#define    UART_MCR_SET_RTS                         (1 << 0)
#define    UART_MCR_RTS_MASK                        (1 << 0)

//UART TX/RX Status Register
//Transmitter Empty
#define    UART_TRANSMITTER_NOT_EMPTY               (0 << 2)
#define    UART_TRANSMITTER_EMPTY                   (1 << 2)
#define    UART_TRANSMITTER_STATUS_MASK             (1 << 2)
//Transmitter Buffer Empty
#define    UART_TX_BUFFER_NOT_EMPTY                 (0 << 1)
#define    UART_TX_BUFFER_EMPTY                     (1 << 1)
#define    UART_TX_BUFFER_STATUS_MASK               (1 << 1)
//Receive Buffer Data Ready
#define    UART_RX_BUFFER_EMPTY                     (0 << 0)
#define    UART_RX_BUFFER_NOT_EMPTY                 (1 << 0)
#define    UART_RX_BUFFER_STATUS_MASK               (1 << 0)

//UART Error Status Register
//Break Detect
#define    UART_LSR_BI                              (1 << 3)
//Frame Error
#define    UART_LSR_FE                              (1 << 2)
//Parity Error
#define    UART_LSR_PE                              (1 << 1)
//Overrun Error
#define    UART_LSR_OE                              (1 << 0)

//UART FIFO Status Register
//Tx FIFO Full
#define    UART_FSR_TX_FIFO_NOT_FULL                (0 << 14)
#define    UART_FSR_TX_FIFO_FULL                    (1 << 14)
#define    UART_FSR_TX_FIFO_MASK                    (1 << 14)
//Tx FIFO Count
#define    UART_FSR_TX_FIFO_COUNT_MASK              (0x3F << 8)    // [13:8] 6 bits
#define    UART_FSR_TX_FIFO_COUNT_SHFIT             (8)
//Rx FIFO Full
#define    UART_FSR_RX_FIFO_NOT_FULL                (0 << 6)
#define    UART_FSR_RX_FIFO_FULL                    (1 << 6)
#define    UART_FSR_RX_FIFO_MASK                    (1 << 6)
//Rx FIFO Count
#define    UART_FSR_RX_FIFO_COUNT_MASK              (0x3F << 0)    // [5:0] 6 bits
#define    UART_FSR_RX_FIFO_COUNT_SHIFT             (0)

//UART Modem Status Register
//Delta CTS
#define    UART_MSR_DCTS                            (1 << 4)
//Clear to Send (CTS)
#define    UART_MSR_CTS                             (1 << 0)

//UART Interrupt Peding Register, UART Interrupt Source Pending Register, UART Interrupt Mask Register
#define    S6410UART_INT_RXD                        (1 << 0)
#define    S6410UART_INT_ERR                        (1 << 1)
#define    S6410UART_INT_TXD                        (1 << 2)
#define    S6410UART_INT_MODEM                      (1 << 3)
////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// Required Registry Setting.
#define    PC_REG_6410UART_IST_TIMEOUTS_VAL_NAME    TEXT("ISTTimeouts")
#define    PC_REG_6410UART_IST_TIMEOUTS_VAL_LEN     sizeof(DWORD)
#define    PC_REG_6410UART_MEM_LENGTH_VAL_NAME      TEXT("MemLen")
#define    PC_REG_6410UART_MEM_LENGTH_VAL_LEN       sizeof(DWORD)
#ifdef USE_DMA
#define PC_REG_TX_DMA_EN_NAME                        TEXT("TXDMAEnable") 
#endif 
/////////////////////////////////////////////////////////////////////////////////////////

// WaterMarker Pairs.
typedef struct  __PAIRS
{
    ULONG   Key;
    ULONG   AssociatedValue;
} PAIRS, *PPAIRS;

static const UINT UDIVSLOT_TABLE[16] = 
{
    0x0000, 0x0080, 0x0808, 0x0888, 0x2222, 0x4924, 0x4A52, 0x54AA,
    0x5555, 0xD555, 0xD5D5, 0xDDD5, 0xDDDD, 0xDFDD, 0xDFDF, 0xFFDF
};

class CReg6410Uart
{
public:
    CReg6410Uart(PULONG pRegAddr);
    virtual ~CReg6410Uart() { ; };
    virtual BOOL    Init() ;
    // We do not virtual Read & Write data because of Performance Concern.
    void    Write_ULCON(ULONG uData) { WRITE_REGISTER_ULONG( m_pReg, (uData)); };
    ULONG   Read_ULCON() { return (READ_REGISTER_ULONG(m_pReg)); } ;
    void    Write_UCON (ULONG uData) { WRITE_REGISTER_ULONG(m_pReg+1 , uData); };
    ULONG   Read_UCON() { return READ_REGISTER_ULONG(m_pReg+1 ); };
    void    Write_UFCON(ULONG uData) { WRITE_REGISTER_ULONG( m_pReg+2, uData);};
    ULONG   Read_UFCON() { return READ_REGISTER_ULONG(m_pReg + 2); };
    void    Write_UMCON(ULONG uData) { WRITE_REGISTER_ULONG(m_pReg + 3, uData);};
    ULONG   Read_UMCON() { return READ_REGISTER_ULONG(m_pReg + 3);};
    ULONG   Read_UTRSTAT() { return READ_REGISTER_ULONG(m_pReg + 4);};
    ULONG   Read_UERSTAT() { return READ_REGISTER_ULONG(m_pReg + 5);};
    ULONG   Read_UFSTAT() { return READ_REGISTER_ULONG(m_pReg + 6);};
    ULONG   Read_UMSTAT() { return READ_REGISTER_ULONG(m_pReg + 7);};
    void    Write_UTXH (UINT8 uData) { WRITE_REGISTER_ULONG( (m_pReg + 8), uData) ; };
    UINT8   Read_URXH() { return (UINT8) READ_REGISTER_ULONG(m_pReg + 9); };
    void    Write_UBRDIV(ULONG uData) { WRITE_REGISTER_ULONG( m_pReg + 10, uData );};
    ULONG   Read_UBRDIV() { return READ_REGISTER_ULONG(m_pReg + 10); };
    void    Write_UDIVSLOT(ULONG uData) { WRITE_REGISTER_ULONG( (m_pReg + 11), uData) ; };
    ULONG    Read_UDIVSLOT() { return READ_REGISTER_ULONG(m_pReg + 11); };
    void    Write_UINTP(ULONG uData) { WRITE_REGISTER_ULONG( (m_pReg + 12), uData) ; };
    ULONG    Read_UINTP() { return READ_REGISTER_ULONG(m_pReg + 12); };
    void    Write_UINTSP(ULONG uData) { WRITE_REGISTER_ULONG( (m_pReg + 13), uData) ; };
    ULONG    Read_UINTSP() { return READ_REGISTER_ULONG(m_pReg + 13); };
    void    Write_UINTM(ULONG uData) { WRITE_REGISTER_ULONG( (m_pReg + 14), uData) ; };
    ULONG    Read_UINTM() { return READ_REGISTER_ULONG(m_pReg + 14); };    

    virtual BOOL    Write_BaudRate(ULONG uData);
    PULONG  GetRegisterVirtualAddr() { return m_pReg; };
    virtual void    Backup();
    virtual void    Restore();
#ifdef DEBUG
    virtual void    DumpRegister();
#endif
protected:
    volatile PULONG const  m_pReg;
    BOOL    m_fIsBackedUp;
private:
    ULONG    m_ULCONBackup;
    ULONG    m_UCONBackup;
    ULONG    m_UFCONBackup;
    ULONG    m_UMCOMBackup;
    ULONG    m_UBRDIVBackup;
    ULONG    m_UDIVSLOTBackup;
    ULONG    m_UINTMBackup;

    ULONG    m_BaudRate;
    ULONG    m_s3c6410_pclk;
};
class CPdd6410Uart: public CSerialPDD, public CMiniThread
{
public:
    CPdd6410Uart (LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj);
    virtual ~CPdd6410Uart();
    virtual BOOL Init();
    virtual void PostInit();
    virtual BOOL MapHardware();
    virtual BOOL CreateHardwareAccess();
    //  Power Manager Required Function.
    virtual void    SerialRegisterBackup() { m_pReg6410Uart->Backup(); };
    virtual void    SerialRegisterRestore() { m_pReg6410Uart->Restore(); };

// Implement CPddSerial Function.
    virtual BOOL SetDCB(LPDCB lpDCB) {
        BOOL success;

        success = CSerialPDD::SetDCB(lpDCB);

        if (success) {  
            if (m_UseAutoFlow) {

                m_HardwareLock.Lock();

                DWORD dwBit = m_pReg6410Uart->Read_UMCON();

                dwBit &= ~UART_MCR_AUTO_FLOW_MASK;
                dwBit &= ~UART_MCR_RTS_RX_FIFO_TRIG_MASK;


                if (m_DCB.fOutxCtsFlow && (m_DCB.fRtsControl == RTS_CONTROL_HANDSHAKE)) {

                    dwBit |= UART_MCR_AUTO_FLOW_ENABLE | UART_MCR_RTS_RX_FIFO_TRIG_48;
                    m_AutoFlowEnabled = TRUE;          
                    RETAILMSG(1, (TEXT(" *** UART AutoFlow enabled: 0x%X  \r\n"),dwBit));
                } else {
                    m_AutoFlowEnabled = FALSE;
                }

                m_pReg6410Uart->Write_UMCON(dwBit); 

                m_HardwareLock.Unlock();
            }
        }

        return success;
    }

// Interrupt
    virtual BOOL    InitialEnableInterrupt(BOOL bEnable ) ; // Enable All the interrupt may include Xmit Interrupt.
private:
    virtual DWORD ThreadRun();   // IST
//
//  Tx Function.
public:
    virtual BOOL    InitXmit(BOOL bInit);
    virtual void    XmitInterruptHandler(PUCHAR pTxBuffer, ULONG *pBuffLen);
    virtual void    XmitComChar(UCHAR ComChar);
    virtual BOOL    EnableXmitInterrupt(BOOL bEnable);
    virtual BOOL    CancelXmit();
    virtual DWORD   GetWriteableSize();
protected:
    BOOL    m_XmitFifoEnable;
    HANDLE  m_XmitFlushDone;
//
//  Rx Function.
public:
    virtual BOOL    InitReceive(BOOL bInit);
    virtual ULONG   ReceiveInterruptHandler(PUCHAR pRxBuffer,ULONG *pBufflen);
    virtual ULONG   CancelReceive();
    virtual DWORD   GetWaterMark();
    virtual BYTE    GetWaterMarkBit();
    virtual void    Rx_Pause(BOOL bSet) {;};
protected:
    BOOL    m_bReceivedCanceled;
    DWORD   m_dwWaterMark;
//
//  Modem Function
public:
    virtual BOOL    InitModem(BOOL bInit);
    virtual void    ModemInterruptHandler() { GetModemStatus();};
    virtual ULONG   GetModemStatus();
    virtual void    SetDTR(BOOL bSet) {;};
    virtual void    SetRTS(BOOL bSet);
//
// Line Function.
    virtual BOOL    InitLine(BOOL bInit) ;
    virtual void    LineInterruptHandler() { GetLineStatus();};
    virtual void    SetBreak(BOOL bSet) ;
    virtual BOOL    SetBaudRate(ULONG BaudRate,BOOL bIrModule) ;
    virtual BOOL    SetByteSize(ULONG ByteSize);
    virtual BOOL    SetParity(ULONG Parity);
    virtual BOOL    SetStopBits(ULONG StopBits);
//
// Line Internal Function
    BYTE            GetLineStatus();
    virtual void    SetOutputMode(BOOL UseIR, BOOL Use9Pin) ;

protected:
    CReg6410Uart *  m_pReg6410Uart;
    PVOID           m_pRegVirtualAddr;
// Interrupt Function
public:
    void    DisableInterrupt(DWORD dwInt)
    { 
        m_pReg6410Uart->Write_UINTM(m_pReg6410Uart->Read_UINTM() | dwInt);
        m_pReg6410Uart->Write_UINTSP(dwInt);
    }
    void    EnableInterrupt(DWORD dwInt)
    { 
        m_pReg6410Uart->Write_UINTSP(dwInt);
        m_pReg6410Uart->Write_UINTM(m_pReg6410Uart->Read_UINTM() & ~dwInt);
    }
    void    ClearInterrupt(DWORD dwInt)
    {
        m_pReg6410Uart->Write_UINTSP(dwInt);
        m_pReg6410Uart->Write_UINTP(dwInt);
    }
    DWORD   GetInterruptStatus()
    { 
        return (m_pReg6410Uart->Read_UINTP()); 
    };
    DWORD   GetIntrruptMask ()
    { 
        return (~(m_pReg6410Uart->Read_UINTM()));
    };
    VOID SetClockSelect(DWORD ClockSelect) {
        m_ClockSelect = ClockSelect;
        m_ClockSelectValid = TRUE;
    }

    VOID AllowAutoFlow(BOOL Allow) {
        m_UseAutoFlow = Allow;
    }

protected:
    CRegistryEdit    m_ActiveReg;
    //  Interrupt Handler
    DWORD    m_dwSysIntr;
    HANDLE   m_hISTEvent;
    // Optional Parameter
    DWORD    m_dwDevIndex;
    DWORD    m_dwISTTimeout;
    DWORD    m_dwMemLen;
    BOOL    m_ClockSelectValid;
    DWORD    m_ClockSelect;
    BOOL    m_UseAutoFlow;
    BOOL    m_AutoFlowEnabled;
#ifdef USE_DMA
    DWORD         m_dwTXDMAEnable;
    BOOL             InitXmitDMA(void); 
    BOOL            EnableTxDMA(BOOL fEnable);
    BOOL             StopXmitDMA(void);
    BOOL             StartXmitDMA(PUCHAR pTxBuffer, ULONG BuffLen);   
#endif
};

#endif
