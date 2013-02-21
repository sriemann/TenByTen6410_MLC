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

    Serial PDD for SamSang 6410 Development Board.

Notes: 
--*/
#include <windows.h>
#include <types.h>
#include <ceddk.h>

#include <ddkreg.h>
#include <serhw.h>
#include <Serdbg.h>
#include <bsp.h>
#include <pdds3c6410_ser.h>
#include <s3c6410_base_regs.h>
#include <s3c6410_gpio.h>

// CPdd6410Serial0 is only use for UART0 which 
// RxD0 & TxD0 uses GPA0 & GPA1 respectively
// RTS0 & CTS0 uses GPA3 & GPA2 respectively
// DTR0 & DSR0 uses GPN6 & GPN7 respectively

class CPdd6410Serial0 : public CPdd6410Uart
{
public:
    CPdd6410Serial0(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj)
        : CPdd6410Uart(lpActivePath, pMdd, pHwObj)
    {
        m_pIOPregs = NULL;
        m_pSysconRegs = NULL;
        //m_fIsDSRSet = FALSE;
    }
    ~CPdd6410Serial0()
    {
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART0;        // UART0
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        if (m_pIOPregs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pIOPregs, sizeof(S3C6410_GPIO_REG));
        }
        if (m_pSysconRegs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pSysconRegs, sizeof(S3C6410_SYSCON_REG));
        }
    }
    virtual BOOL Init()
    {
        PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
        ioPhysicalBase.HighPart = 0;
        m_pSysconRegs = (S3C6410_SYSCON_REG *) MmMapIoSpace(ioPhysicalBase,sizeof(S3C6410_SYSCON_REG),FALSE);

        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART0;        // UART0
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        else
        {
            return FALSE;
        }

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
        ioPhysicalBase.HighPart = 0;
        m_pIOPregs = (S3C6410_GPIO_REG *) MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG),FALSE);

        if (m_pIOPregs)
        {
            DDKISRINFO ddi;
            if (GetIsrInfo(&ddi)== ERROR_SUCCESS && 
                KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &ddi.dwIrq, sizeof(UINT32), &ddi.dwSysintr, sizeof(UINT32), NULL))
            {   
                RETAILMSG( FALSE, (TEXT("DEBUG: Serial0 SYSINTR : %d\r\n"), (PBYTE)&ddi.dwSysintr)); 
                RegSetValueEx(DEVLOAD_SYSINTR_VALNAME,REG_DWORD,(PBYTE)&ddi.dwSysintr, sizeof(UINT32));
            }
            else
            {
                return FALSE;
            }

			// DTR0(GPN6), DSR0(GPN7)
            // DTR and DSR are used for ActiveSync connection.
            //m_pDTRPort = NULL; //(volatile ULONG *)&(m_pIOPregs->GPNDAT);
            //m_pDSRPort = NULL; //(volatile ULONG *)&(m_pIOPregs->GPNDAT);
            //m_dwDTRPortNum = DTR_PORT_NUMBER;
            //m_dwDSRPortNum = DSR_PORT_NUMBER;

            // CTS0(GPA2), RTS0(GPA3), TXD0(GPA1), RXD0(GPA0)
            m_pIOPregs->GPACON &= ~(0xf<<0 | 0xf<<4 | 0xf<<8 | 0xf<<12 );    ///< Clear Bit
            m_pIOPregs->GPACON |=  (0x2<<0 | 0x2<<4 | 0x2<<8 | 0x2<<12 );     ///< Select UART IP                
            m_pIOPregs->GPAPUD &= ~(0x3<<0 | 0x3<<2 | 0x3<<4 | 0x3<<6 );    ///< Pull-Up/Down Disable  

			// DTR0(GPN6), DSR0(GPN7)
            // If you want to use COM1 port for ActiveSync, use these statements.   
            /*  
            m_pIOPregs->GPNCON &= ~(0x3<<12);    ///< DTR0 Clear Bit
            m_pIOPregs->GPNCON |= (0x1<<12);    ///< Output
            m_pIOPregs->GPNPUD &= ~(0x3<<12);    ///< Pull-Up/Down Disable 
            m_pIOPregs->GPNCON &= ~(0x3<<14);    ///< DSR0 Clear Bit
            m_pIOPregs->GPNCON |= (0x0<<14);    ///< Input
            m_pIOPregs->GPNPUD &= ~(0x3<<14);    ///< Pull-Up/Down Disable 
            */

            return CPdd6410Uart::Init();
        }
        return FALSE;
    }
    virtual BOOL    PowerOff()
    {
        //SetDTR(FALSE);
        CSerialPDD::PowerOff();
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART0;        // UART0
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        return TRUE;
    }
    virtual BOOL    PowerOn()
    {
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART0;        // UART0
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        CSerialPDD::PowerOn();
        //SetDTR(TRUE);
        return TRUE;
    }
    virtual BOOL    InitModem(BOOL bInit)
    {
        return CPdd6410Uart::InitModem(bInit);
    }

    virtual ULONG GetModemStatus()
    {
		return CPdd6410Uart::GetModemStatus();
    }
   virtual void    SetDTR(BOOL bSet) {}

    virtual void    SetDefaultConfiguration()
    {
        // Default Value. Can be altered.
        m_CommPorp.wPacketLength       = 0xffff;
        m_CommPorp.wPacketVersion      = 0xffff;
        m_CommPorp.dwServiceMask       = SP_SERIALCOMM;
        m_CommPorp.dwReserved1         = 0;
        m_CommPorp.dwMaxTxQueue        = 16;
        m_CommPorp.dwMaxRxQueue        = 16;
        m_CommPorp.dwMaxBaud           = BAUD_115200;
        m_CommPorp.dwProvSubType       = PST_RS232;
        m_CommPorp.dwProvCapabilities  =
            PCF_DTRDSR | PCF_RLSD | PCF_RTSCTS |
            PCF_SETXCHAR |
            PCF_INTTIMEOUTS |
            PCF_PARITY_CHECK |
            PCF_SPECIALCHARS |
            PCF_TOTALTIMEOUTS |
            PCF_XONXOFF;
        m_CommPorp.dwSettableBaud      =
            BAUD_075 | BAUD_110 | BAUD_150 | BAUD_300 | BAUD_600 |
            BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
            BAUD_7200 | BAUD_9600 | BAUD_14400 |
            BAUD_19200 | BAUD_38400 | BAUD_56K | BAUD_128K |
            BAUD_115200 | BAUD_57600 | BAUD_USER;
        m_CommPorp.dwSettableParams    =
            SP_BAUD | SP_DATABITS | SP_HANDSHAKING | SP_PARITY |
            SP_PARITY_CHECK | SP_RLSD | SP_STOPBITS;
        m_CommPorp.wSettableData       =
            DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8;
        m_CommPorp.wSettableStopParity =
            STOPBITS_10 | STOPBITS_20 |
            PARITY_NONE | PARITY_ODD | PARITY_EVEN | PARITY_SPACE |
            PARITY_MARK;
        // Setup m_DCB.

        // Set Detault Parameter.
        SetOutputMode(FALSE, TRUE );    // No IR.
        // For DCB. The PDD only need to take care BaudRate, ByteSize Parity & StopBit
        m_DCB.DCBlength  = sizeof(DCB);
        SetBaudRate(m_DCB.BaudRate   = 9600,FALSE);
        SetByteSize(m_DCB.ByteSize   = 8);
        SetParity(m_DCB.Parity     = NOPARITY);
        SetStopBits(m_DCB.StopBits   = ONESTOPBIT);            
    }
    
private:
    volatile S3C6410_GPIO_REG * m_pIOPregs;
    volatile S3C6410_SYSCON_REG * m_pSysconRegs;
    //volatile ULONG *    m_pDTRPort;
    //DWORD               m_dwDTRPortNum;
    //volatile ULONG *    m_pDSRPort;
    //DWORD               m_dwDSRPortNum;
    //BOOL                m_fIsDSRSet;
};

/// CPdd6410Serial1 is used for UART1
/// enabling UART1 is dependent to board's jumper setting.
/// We assume that jumper setting is correct.
/// RTS1 & CTS1 uses GPA7 & GPA6 respectively
/// RxD1 & TxD1 uses GPA4 & GPA5 respectively

class CPdd6410Serial1 : public CPdd6410Uart
{
public:
    CPdd6410Serial1(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj)
        : CPdd6410Uart(lpActivePath, pMdd, pHwObj)
    {
        m_pIOPregs = NULL;
        m_pSysconRegs = NULL;
    }
    ~CPdd6410Serial1()
    {
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART1;        // UART1
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        if (m_pIOPregs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pIOPregs, sizeof(S3C6410_GPIO_REG));
        }
        if (m_pSysconRegs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pSysconRegs, sizeof(S3C6410_SYSCON_REG));
        }
    }
    virtual BOOL Init()
    {
        PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
        ioPhysicalBase.HighPart = 0;
        m_pSysconRegs = (S3C6410_SYSCON_REG *) MmMapIoSpace(ioPhysicalBase,sizeof(S3C6410_SYSCON_REG),FALSE);

        if(m_pSysconRegs)
        {        
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART1;        // UART1
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        else
        {
            return FALSE;
        }


        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
        ioPhysicalBase.HighPart = 0;
        m_pIOPregs = (S3C6410_GPIO_REG *) MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG),FALSE);

        if (m_pIOPregs)
        {
            DDKISRINFO ddi;
            if (GetIsrInfo(&ddi)== ERROR_SUCCESS && 
                KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &ddi.dwIrq, sizeof(UINT32), &ddi.dwSysintr, sizeof(UINT32), NULL))
            {   
                RETAILMSG( FALSE, (TEXT("DEBUG: Serial1 SYSINTR : %d\r\n"), (PBYTE)&ddi.dwSysintr)); 
                RegSetValueEx(DEVLOAD_SYSINTR_VALNAME,REG_DWORD,(PBYTE)&ddi.dwSysintr, sizeof(UINT32));
            }
            else
            {
                return FALSE;
            }

            // TXD1(GPA5), RXD1(GPA4), RTS1(GPA7), CTS1(GPA6)
            m_pIOPregs->GPACON &= ~(0xf<<16 | 0xf<<20 | 0xf<<24 | 0xf<<28); 
            m_pIOPregs->GPACON |= (0x2<<16 | 0x2<<20 | 0x2<<24 | 0x2<<28); 
            m_pIOPregs->GPAPUD &= ~(0x3<<8  | 0x3<<10 | 0x3<<12 | 0x3<<14);

            /* switch UART1 clock to EPLL to get access to higher baud rates */
                    
            /* allow UART1 to use auto RTS/CTS flow control when requests by applications */

            return CPdd6410Uart::Init();
        }
        return FALSE;
    }
    virtual BOOL    PowerOff()
    {
        CSerialPDD::PowerOff();
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART1;        // UART1
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        return TRUE;
    }
    virtual BOOL    PowerOn()
    {
        if(m_pSysconRegs)
        {        
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART1;        // UART1
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        CSerialPDD::PowerOn();
        return TRUE;
    }
    virtual ULONG   GetModemStatus()
    {
        // return (CPdd6410Uart::GetModemStatus() | MS_CTS_ON);
        // return TRUE modem status
        return CPdd6410Uart::GetModemStatus();
    }
    virtual void    SetDefaultConfiguration()
    {
        // Default Value. Can be altered.
        m_CommPorp.wPacketLength       = 0xffff;
        m_CommPorp.wPacketVersion      = 0xffff;
        m_CommPorp.dwServiceMask       = SP_SERIALCOMM;
        m_CommPorp.dwReserved1         = 0;
        m_CommPorp.dwMaxTxQueue        = 16;
        m_CommPorp.dwMaxRxQueue        = 16;
        m_CommPorp.dwMaxBaud           = BAUD_115200;
        m_CommPorp.dwProvSubType       = PST_RS232;
        m_CommPorp.dwProvCapabilities  =
            PCF_DTRDSR | PCF_RLSD | PCF_RTSCTS |
            PCF_SETXCHAR |
            PCF_INTTIMEOUTS |
            PCF_PARITY_CHECK |
            PCF_SPECIALCHARS |
            PCF_TOTALTIMEOUTS |
            PCF_XONXOFF;
        m_CommPorp.dwSettableBaud      =
            BAUD_075 | BAUD_110 | BAUD_150 | BAUD_300 | BAUD_600 |
            BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
            BAUD_7200 | BAUD_9600 | BAUD_14400 |
            BAUD_19200 | BAUD_38400 | BAUD_56K | BAUD_128K |
            BAUD_115200 | BAUD_57600 | BAUD_USER;
        m_CommPorp.dwSettableParams    =
            SP_BAUD | SP_DATABITS | SP_HANDSHAKING | SP_PARITY |
            SP_PARITY_CHECK | SP_RLSD | SP_STOPBITS;
        m_CommPorp.wSettableData       =
            DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8;
        m_CommPorp.wSettableStopParity =
            STOPBITS_10 | STOPBITS_20 |
            PARITY_NONE | PARITY_ODD | PARITY_EVEN | PARITY_SPACE |
            PARITY_MARK;
        // Setup m_DCB.

        // Set Detault Parameter.
        SetOutputMode(FALSE, TRUE );    // No IR.
        // For DCB. The PDD only need to take care BaudRate, ByteSize Parity & StopBit
        m_DCB.DCBlength  = sizeof(DCB);
        SetBaudRate(m_DCB.BaudRate   = 9600,FALSE);
        SetByteSize(m_DCB.ByteSize   = 8);
        SetParity(m_DCB.Parity     = NOPARITY);
        SetStopBits(m_DCB.StopBits   = ONESTOPBIT);        
    }

    
    volatile S3C6410_GPIO_REG * m_pIOPregs;
    volatile S3C6410_SYSCON_REG * m_pSysconRegs;
};

/// CPdd6410Serial2 is used for UART2
/// enabling UART2 is dependent to board jumper setting.
/// We assume that jumper setting is correct.
/// UART2 has no RTS&CTS signal
/// RxD2 & TxD2 uses GPB0 & GPB1 respectively

class CPdd6410Serial2 : public CPdd6410Uart
{
public:
    CPdd6410Serial2(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj)
        : CPdd6410Uart(lpActivePath, pMdd, pHwObj)
    {
        m_pIOPregs = NULL;
        m_pSysconRegs = NULL;
    }
    ~CPdd6410Serial2()
    {
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART2;        // UART2;    
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        if (m_pIOPregs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pIOPregs, sizeof(S3C6410_GPIO_REG));
        }
        if (m_pSysconRegs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pSysconRegs, sizeof(S3C6410_SYSCON_REG));
        }
    }
    virtual BOOL Init()
    {
        PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
        ioPhysicalBase.HighPart = 0;
        m_pSysconRegs = (S3C6410_SYSCON_REG *) MmMapIoSpace(ioPhysicalBase,sizeof(S3C6410_SYSCON_REG),FALSE);

        if(m_pSysconRegs)
        {        
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART2;        // UART2
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        else
        {
            return FALSE;
        }        

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
        ioPhysicalBase.HighPart = 0;
        m_pIOPregs = (S3C6410_GPIO_REG *) MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG),FALSE);

        if (m_pIOPregs)
        {
            DDKISRINFO ddi;
            if (GetIsrInfo(&ddi)== ERROR_SUCCESS && 
                KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &ddi.dwIrq, sizeof(UINT32), &ddi.dwSysintr, sizeof(UINT32), NULL))
            {   
                RETAILMSG( FALSE, (TEXT("DEBUG: Serial2 SYSINTR : %d\r\n"), (PBYTE)&ddi.dwSysintr)); 
                RegSetValueEx(DEVLOAD_SYSINTR_VALNAME,REG_DWORD,(PBYTE)&ddi.dwSysintr, sizeof(UINT32));
            }
            else
            {
                return FALSE;
            }

            // TXD2(GPB1), RXD2(GPB0)
            m_pIOPregs->GPBCON &= ~(0xf<<0 | 0xf<<4);    ///< Clear Bit  
            m_pIOPregs->GPBCON |= (0x2<<0 | 0x2<<4);    ///< Select RXD2, TXD2
            m_pIOPregs->GPBPUD &= ~(0x3<<0 | 0x3<<2);    ///< Pull-Up/Down Disable 

            return CPdd6410Uart::Init();
        }
        return FALSE;
    }
    virtual BOOL    PowerOff()
    {
        CSerialPDD::PowerOff();
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART2;        // UART2;    
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        return TRUE;
    }
    virtual BOOL    PowerOn()
    {
        if(m_pSysconRegs)
        {        
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART2;        // UART2
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        CSerialPDD::PowerOn();
        return TRUE;
    }
    virtual ULONG   GetModemStatus()
    {
        return (CPdd6410Uart::GetModemStatus() | MS_CTS_ON);
    }
    virtual void    SetDefaultConfiguration()
    {
        // Default Value. Can be altered.
        m_CommPorp.wPacketLength       = 0xffff;
        m_CommPorp.wPacketVersion      = 0xffff;
        m_CommPorp.dwServiceMask       = SP_SERIALCOMM;
        m_CommPorp.dwReserved1         = 0;
        m_CommPorp.dwMaxTxQueue        = 16;
        m_CommPorp.dwMaxRxQueue        = 16;
        m_CommPorp.dwMaxBaud           = BAUD_115200;
        m_CommPorp.dwProvSubType       = PST_RS232;
        m_CommPorp.dwProvCapabilities  =
            PCF_DTRDSR | PCF_RLSD | PCF_RTSCTS |
            PCF_SETXCHAR |
            PCF_INTTIMEOUTS |
            PCF_PARITY_CHECK |
            PCF_SPECIALCHARS |
            PCF_TOTALTIMEOUTS |
            PCF_XONXOFF;
        m_CommPorp.dwSettableBaud      =
            BAUD_075 | BAUD_110 | BAUD_150 | BAUD_300 | BAUD_600 |
            BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
            BAUD_7200 | BAUD_9600 | BAUD_14400 |
            BAUD_19200 | BAUD_38400 | BAUD_56K | BAUD_128K |
            BAUD_115200 | BAUD_57600 | BAUD_USER;
        m_CommPorp.dwSettableParams    =
            SP_BAUD | SP_DATABITS | SP_HANDSHAKING | SP_PARITY |
            SP_PARITY_CHECK | SP_RLSD | SP_STOPBITS;
        m_CommPorp.wSettableData       =
            DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8;
        m_CommPorp.wSettableStopParity =
            STOPBITS_10 | STOPBITS_20 |
            PARITY_NONE | PARITY_ODD | PARITY_EVEN | PARITY_SPACE |
            PARITY_MARK;
        // Setup m_DCB.

        // Set Detault Parameter.
        SetOutputMode(FALSE, TRUE );    // No IR.
        // For DCB. The PDD only need to take care BaudRate, ByteSize Parity & StopBit
        m_DCB.DCBlength  = sizeof(DCB);
        SetBaudRate(m_DCB.BaudRate   = 9600,FALSE);
        SetByteSize(m_DCB.ByteSize   = 8);
        SetParity(m_DCB.Parity     = NOPARITY);
        SetStopBits(m_DCB.StopBits   = ONESTOPBIT);        
    }
    /// change GPIO between RXD2 and Input
#ifndef    BSP_NOIRDA2
    // This function is needed when we use IrDA2.
    // If use IrDA. When data is transmitted, data cannot be received. 
    virtual void    Rx_Pause(BOOL bSet)
    {
        if(bSet)
        {        ///< from RXD2 to Input
            m_pIOPregs->GPBCON = (m_pIOPregs->GPBCON & ~(0xf<<0)) | 0x0<<0;
        }
        else
        {            ///< from Input to RXD2
            m_pIOPregs->GPBCON = (m_pIOPregs->GPBCON & ~(0xf<<0)) | 0x2<<0;
        }
    }
#endif    

    volatile S3C6410_GPIO_REG * m_pIOPregs;
    volatile S3C6410_SYSCON_REG * m_pSysconRegs;
};

/// CPdd6410Serial3 is used for UART3
/// enabling UART3 is dependent to board's jumper setting.
/// We assume that jumper setting is correct.
/// UART3 has no RTS&CTS signal
/// RxD3 & TxD3 uses GPB2 & GPB3 respectively

class CPdd6410Serial3 : public CPdd6410Uart
{
public:
    CPdd6410Serial3(LPTSTR lpActivePath, PVOID pMdd, PHWOBJ pHwObj)
        : CPdd6410Uart(lpActivePath, pMdd, pHwObj)
    {
        m_pIOPregs = NULL;
        m_pSysconRegs = NULL;
    }
    ~CPdd6410Serial3()
    {
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART3;        // UART3
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        if (m_pIOPregs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pIOPregs, sizeof(S3C6410_GPIO_REG));
        }
        if (m_pSysconRegs!=NULL)
        {
            MmUnmapIoSpace((PVOID)m_pSysconRegs, sizeof(S3C6410_SYSCON_REG));
        }
    }
    virtual BOOL Init()
    {
        PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
        ioPhysicalBase.HighPart = 0;
        m_pSysconRegs = (S3C6410_SYSCON_REG *) MmMapIoSpace(ioPhysicalBase,sizeof(S3C6410_SYSCON_REG),FALSE);

        if(m_pSysconRegs)
        {        
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART3;        // UART3
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        else
        {
            return FALSE;
        }

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
        ioPhysicalBase.HighPart = 0;
        m_pIOPregs = (S3C6410_GPIO_REG *) MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG),FALSE);

        if (m_pIOPregs)
        {
            DDKISRINFO ddi;
            if (GetIsrInfo(&ddi)== ERROR_SUCCESS && 
                KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &ddi.dwIrq, sizeof(UINT32), &ddi.dwSysintr, sizeof(UINT32), NULL))
            {   
                RETAILMSG( FALSE, (TEXT("DEBUG: Serial3 SYSINTR : %d\r\n"), (PBYTE)&ddi.dwSysintr)); 
                RegSetValueEx(DEVLOAD_SYSINTR_VALNAME,REG_DWORD,(PBYTE)&ddi.dwSysintr, sizeof(UINT32));
            }
            else
            {
                return FALSE;
            }

            // TXD3(GPB3), RXD3(GPB2)
            m_pIOPregs->GPBCON &= ~(0xf<<8 | 0xf<<12); 
            m_pIOPregs->GPBCON |= (0x2<<8 | 0x2<<12); 
            m_pIOPregs->GPBPUD &= ~(0x3<<4 | 0x3<<6);

            return CPdd6410Uart::Init();
        }
        return FALSE;
    }
    virtual BOOL    PowerOff()
    {
        CSerialPDD::PowerOff();
        if(m_pSysconRegs)
        {
            m_pSysconRegs->PCLK_GATE  &= ~PCLK_UART3;        // UART3
            m_pSysconRegs->SCLK_GATE  &= ~SCLK_UART;        // UART0~3
        }
        return TRUE;
    }
    virtual BOOL    PowerOn()
    {
        if(m_pSysconRegs)
        {        
            m_pSysconRegs->PCLK_GATE  |= PCLK_UART3;        // UART3
            m_pSysconRegs->SCLK_GATE  |= SCLK_UART;        // UART0~3    
        }
        CSerialPDD::PowerOn();
        return TRUE;
    }
    virtual ULONG   GetModemStatus()
    {
        return (CPdd6410Uart::GetModemStatus() | MS_CTS_ON);
    }
    virtual void    SetDefaultConfiguration()
    {
        // Default Value. Can be altered.
        m_CommPorp.wPacketLength       = 0xffff;
        m_CommPorp.wPacketVersion      = 0xffff;
        m_CommPorp.dwServiceMask       = SP_SERIALCOMM;
        m_CommPorp.dwReserved1         = 0;
        m_CommPorp.dwMaxTxQueue        = 16;
        m_CommPorp.dwMaxRxQueue        = 16;
        m_CommPorp.dwMaxBaud           = BAUD_115200;
        m_CommPorp.dwProvSubType       = PST_RS232;
        m_CommPorp.dwProvCapabilities  =
            PCF_DTRDSR | PCF_RLSD | PCF_RTSCTS |
            PCF_SETXCHAR |
            PCF_INTTIMEOUTS |
            PCF_PARITY_CHECK |
            PCF_SPECIALCHARS |
            PCF_TOTALTIMEOUTS |
            PCF_XONXOFF;
        m_CommPorp.dwSettableBaud      =
            BAUD_075 | BAUD_110 | BAUD_150 | BAUD_300 | BAUD_600 |
            BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
            BAUD_7200 | BAUD_9600 | BAUD_14400 |
            BAUD_19200 | BAUD_38400 | BAUD_56K | BAUD_128K |
            BAUD_115200 | BAUD_57600 | BAUD_USER;
        m_CommPorp.dwSettableParams    =
            SP_BAUD | SP_DATABITS | SP_HANDSHAKING | SP_PARITY |
            SP_PARITY_CHECK | SP_RLSD | SP_STOPBITS;
        m_CommPorp.wSettableData       =
            DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8;
        m_CommPorp.wSettableStopParity =
            STOPBITS_10 | STOPBITS_20 |
            PARITY_NONE | PARITY_ODD | PARITY_EVEN | PARITY_SPACE |
            PARITY_MARK;
        // Setup m_DCB.

        // Set Detault Parameter.
        SetOutputMode(FALSE, TRUE );    // No IR.
        // For DCB. The PDD only need to take care BaudRate, ByteSize Parity & StopBit
        m_DCB.DCBlength  = sizeof(DCB);
        SetBaudRate(m_DCB.BaudRate   = 9600,FALSE);
        SetByteSize(m_DCB.ByteSize   = 8);
        SetParity(m_DCB.Parity     = NOPARITY);
        SetStopBits(m_DCB.StopBits   = ONESTOPBIT);        
    }
    /// change GPIO between RXD3 and Input
#ifndef    BSP_NOIRDA3
    // This function is needed when we use IrDA2.
    // If use IrDA. When data is transmitted, data cannot be received. 
    virtual void    Rx_Pause(BOOL bSet)
    {
        if(bSet)
        {        ///< from RXD3 to Input
            m_pIOPregs->GPBCON = (m_pIOPregs->GPBCON & ~(0xf<<8)) | 0x0<<8;
        }
        else
        {            ///< from Input to RXD3
            m_pIOPregs->GPBCON = (m_pIOPregs->GPBCON & ~(0xf<<8)) | 0x2<<8;
        }
    }
#endif
    
    volatile S3C6410_GPIO_REG * m_pIOPregs;
    volatile S3C6410_SYSCON_REG * m_pSysconRegs;
};

CSerialPDD * CreateSerialObject(LPTSTR lpActivePath, PVOID pMdd,PHWOBJ pHwObj, DWORD DeviceArrayIndex)
{
    CSerialPDD * pSerialPDD = NULL;
    RETAILMSG( TRUE, (TEXT("DEBUG: CreateSerialObject %d\r\n"), DeviceArrayIndex)); 
    switch (DeviceArrayIndex)
    {
    case 0:        ///< UART0
        pSerialPDD = new CPdd6410Serial0(lpActivePath,pMdd, pHwObj);
        break;
    case 1:        ///< UART1
        pSerialPDD = new CPdd6410Serial1(lpActivePath,pMdd, pHwObj);
        break;
    case 2:        ///< UART2(IrDA)
        pSerialPDD = new CPdd6410Serial2(lpActivePath, pMdd, pHwObj);
        break;
    //case 3:        ///< UART3(IrDA)
    //    pSerialPDD = new CPdd6410Serial3(lpActivePath, pMdd, pHwObj);
    //    break;
    }
    if (pSerialPDD && !pSerialPDD->Init())
    {
        delete pSerialPDD;
        pSerialPDD = NULL;
    }    
    return pSerialPDD;
}
void DeleteSerialObject(CSerialPDD * pSerialPDD)
{
    if (pSerialPDD)
    {
        delete pSerialPDD;
    }
}

