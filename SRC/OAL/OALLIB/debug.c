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
//------------------------------------------------------------------------------
//
//  File:  debug.c
//
//  This module is provides the interface to the serial port.
//
#include <bsp.h>
#include <nkintr.h>
#include <devload.h>
//------------------------------------------------------------------------------
// Defines

//------------------------------------------------------------------------------
// Externs

//------------------------------------------------------------------------------
// Global Variables

//------------------------------------------------------------------------------
// Local Variables

static volatile S3C6410_UART_REG *g_pUARTReg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;

static const UINT32 aSlotTable[16] =
{
    0x0000, 0x0080, 0x0808, 0x0888,
    0x2222, 0x4924, 0x4a52, 0x54aa,
    0x5555, 0xd555, 0xd5d5, 0xddd5,
    0xdddd, 0xdfdd, 0xdfdf, 0xffdf
};

//------------------------------------------------------------------------------
// Local Functions

//------------------------------------------------------------------------------
//
//  Function: OEMInitDebugSerial
//
//  Initializes the debug serial port
// HKEY_LOCAL_MACHINE\Drivers\BuiltIn\Serial0
//------------------------------------------------------------------------------
//
//  Function:  OALKitlInitRegistry
//
//  This function is called during the initialization process to allow the
//  OAL to denote devices which are being used by the KITL connection
//  and thus shouldn't be touched during the OS initialization process.  The
//  OAL provides this information via the registry.
//

VOID OALDebugRemoveFromRegistry(unsigned port)
{
    HKEY Key;
    DWORD Status;
    DWORD Disposition;
    DWORD Value;
    DWORD Flags;
    DEVICE_LOCATION devLoc;



        // Disable the UART driver since it is used for Debug
        //
		switch(port)
		{
		case 0:
			Status = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\Serial0", 0, NULL, 0, 0, NULL, &Key, &Disposition);
			break;
			
		case 1:
			Status = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\Serial1", 0, NULL, 0, 0, NULL, &Key, &Disposition);
			break;
			
		case 2:
			Status = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\Serial2", 0, NULL, 0, 0, NULL, &Key, &Disposition);
			break;
			
		case 3:
			Status = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\Serial3", 0, NULL, 0, 0, NULL, &Key, &Disposition);
			break;
		default:

			break;
		}
        if (Status == ERROR_SUCCESS)
        {
            Disposition = DEVFLAGS_NOLOAD;
            // Set Flags value to indicate no loading of driver for this device
            Status = NKRegSetValueEx(Key, DEVLOAD_FLAGS_VALNAME, 0, DEVLOAD_FLAGS_VALTYPE, (PBYTE)&Disposition, sizeof(Disposition));
        }

        // Close the registry key.
        NKRegCloseKey(Key);

        if (Status != ERROR_SUCCESS)
        {
            
            goto CleanUp;
        }

CleanUp:
    return;
}

VOID OEMInitDebugSerial()
{
    UINT32 DivSlot;
    UINT32 uPCLK;
    float Div;
#if defined(DEBUG_ON)
    // Map SFR Address
    //
    if (g_pUARTReg == NULL)
    {
#if    (DEBUG_PORT == DEBUG_UART0)
        // UART0
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART0, FALSE);
#elif (DEBUG_PORT == DEBUG_UART1)
        // UART1
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART1, FALSE);
#elif (DEBUG_PORT == DEBUG_UART2)
        // UART2
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART2, FALSE);
#elif (DEBUG_PORT == DEBUG_UART3)
        // UART3
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART3, FALSE);
#else
        INVALID_DEBUG_PORT        // Error
#endif
    }

    if (g_pGPIOReg == NULL)
    {
        g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    }

    if (g_pSysConReg == NULL)
    {
        g_pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);
    }

    // UART I/O port initialize
#if    (DEBUG_PORT == DEBUG_UART0)
    // UART0 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<1);        // UART0
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART0 Port Initialize (RXD0 : GPA0, TXD0: GPA1)
    g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<0)) | (0x22<<0);        // GPA0->RXD0, GPA1->TXD0
    g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<0)) | (0x1<<0);            // RXD0: Pull-down, TXD0: pull up/down disable
#elif (DEBUG_PORT == DEBUG_UART1)
    // UART1 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<2);        // UART1
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART1 Port Initialize (RXD1 : GPA4, TXD1: GPA5)
    g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<16)) | (0x22<<16);    // GPA4->RXD1, GPA5->TXD1
    g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<8)) | (0x1<<8);            // RXD1: Pull-down, TXD1: pull up/down disable
#elif (DEBUG_PORT == DEBUG_UART2)
    // UART2 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<3);        // UART2
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART2 Port Initialize (RXD2 : GPAB0, TXD2: GPB1)
    g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<0)) | (0x22<<0);        // GPB0->RXD2, GPB1->TXD2
    g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<0)) | (0x1<<0);            // RXD2: Pull-down, TXD2: pull up/down disable
#elif (DEBUG_PORT == DEBUG_UART3)
    // UART3 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<4);        // UART3
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART3 Port Initialize (RXD3 : GPB2, TXD3: GPB3)
    g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<8)) | (0x22<<8);        // GPB2->RXD3, GPB3->TXD3
    g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<4)) | (0x1<<4);            // RXD3: Pull-down, TXD3: pull up/down disable
#endif

    // Initialize UART
    //
    g_pUARTReg->ULCON = (0<<6)|(0<<3)|(0<<2)|(3<<0);                    // Normal Mode, No Parity, 1 Stop Bit, 8 Bit Data
    g_pUARTReg->UCON = (0<<10)|(1<<9)|(1<<8)|(0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0);    // PCLK divide, Polling Mode
    g_pUARTReg->UFCON = (0<<6)|(0<<4)|(0<<2)|(0<<1)|(0<<0);            // Disable FIFO
    g_pUARTReg->UMCON = (0<<5)|(0<<4)|(0<<0);                        // Disable Auto Flow Control

    uPCLK = System_GetPCLK();

    Div = (float)((float)uPCLK/(16.0*(float)DEBUG_BAUDRATE)) - 1;        //< S3C6410_PCLK is macro code defined in soc_cfg.h
    DivSlot = (UINT32)((Div-(int)Div)*16);

    g_pUARTReg->UBRDIV = (UINT32)Div;                                    // Baud rate
    g_pUARTReg->UDIVSLOT = aSlotTable[DivSlot];
#endif
}

//------------------------------------------------------------------------------
//
//  Function: OEMWriteDebugByte
//
//  Transmits a character out the debug serial port.
//
VOID OEMWriteDebugByte(UINT8 ch)
{
#if defined(DEBUG_ON)
    // Wait for TX Buffer Empty
    //
    while (!(g_pUARTReg->UTRSTAT & 0x2));

    // TX Character
    //
    g_pUARTReg->UTXH = ch;
#endif
}



//------------------------------------------------------------------------------
//
//  Function: OEMReadDebugByte
//
//  Reads a byte from the debug serial port. Does not wait for a character.
//  If a character is not available function returns "OEM_DEBUG_READ_NODATA".
//

int OEMReadDebugByte()
{
    int ch= OEM_DEBUG_READ_NODATA;
#if defined(DEBUG_ON)
    if (g_pUARTReg->UTRSTAT & 0x1)        // There is received data
    {
        ch = (int)(g_pUARTReg->URXH);
    }
#endif
    return ch;
}

#define MAX_OEM_LEDINDEX    ((1<<4) - 1)
// The SMDK6410 Evaluation Platform supports 4 LEDs
void OEMWriteDebugLED(UINT16 Index, DWORD Pattern)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugString
//
//  Output unicode string to debug serial port
//
VOID OEMWriteDebugString(LPWSTR string)
{
#if defined(DEBUG_ON)
    while (*string != L'\0') OEMWriteDebugByte((UINT8)*string++);
#endif
}



//------------------------------------------------------------------------------
