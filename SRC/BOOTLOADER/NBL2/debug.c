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
//
VOID OEMInitDebugSerial()
{
    UINT32 DivSlot;
    UINT32 uPCLK;
    float Div;

    // Map SFR Address
    //
    if (g_pUARTReg == NULL)
    {
        // UART0
        g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_UART0, FALSE);
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
    // UART0 Clock Enable
    g_pSysConReg->PCLK_GATE |= (1<<1);        // UART0
    g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
    // UART0 Port Initialize (RXD0 : GPA0, TXD0: GPA1)
    g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<0)) | (0x22<<0);        // GPA0->RXD0, GPA1->TXD0
    g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<0)) | (0x1<<0);            // RXD0: Pull-down, TXD0: pull up/down disable


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
}

//------------------------------------------------------------------------------
//
//  Function: OEMWriteDebugByte
//
//  Transmits a character out the debug serial port.
//
VOID OEMWriteDebugByte(UINT8 ch)
{
    // Wait for TX Buffer Empty
    //
    while (!(g_pUARTReg->UTRSTAT & 0x2));

    // TX Character
    //
    g_pUARTReg->UTXH = ch;
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
    int ch;

    if (g_pUARTReg->UTRSTAT & 0x1)        // There is received data
    {
        ch = (int)(g_pUARTReg->URXH);
    }
    else        // There no data in RX Buffer;
    {
        ch = OEM_DEBUG_READ_NODATA;
    }

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
    while (*string != L'\0') OEMWriteDebugByte((UINT8)*string++);
}


//------------------------------------------------------------------------------
