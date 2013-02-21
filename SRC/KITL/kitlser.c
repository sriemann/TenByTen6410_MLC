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
    kitlser.c

Abstract:

    Platform specific code for serial KITL services.

Functions:


Notes:

--*/

#include <windows.h>
#include <bsp.h>
#include <kitl_cfg.h>

static DWORD KitlIoPortBase;
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

/* S3C6410UART_Init
 *
 *  Called by PQOAL KITL framework to initialize the serial port
 *
 *  Return Value:
 */
BOOL S3C6410UART_Init (KITL_SERIAL_INFO *pSerInfo)
{
    UINT32 DivSlot;
    float Div;

    KITLOutputDebugString ("[KITL] ++S3C6410UART_Init()\r\n");

    KITLOutputDebugString ("[KITL]    pAddress = 0x%x\n", pSerInfo->pAddress);
    KITLOutputDebugString ("[KITL]    BaudRate = 0x%x\n", pSerInfo->baudRate);
    KITLOutputDebugString ("[KITL]    DataBits = 0x%x\n", pSerInfo->dataBits);
    KITLOutputDebugString ("[KITL]    StopBits = 0x%x\n", pSerInfo->stopBits);
    KITLOutputDebugString ("[KITL]    Parity   = 0x%x\n", pSerInfo->parity);

    KitlIoPortBase = (DWORD)pSerInfo->pAddress;

    if (KitlIoPortBase == 0)
    {
        KITLOutputDebugString ("[KITL:ERR] KitlIoPortBase is NULL\r\n");
        return FALSE;
    }

    g_pUARTReg = (S3C6410_UART_REG *)OALPAtoVA(KitlIoPortBase, FALSE);
    g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    g_pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART0)
    {
        // UART0 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<1);        // UART0
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART0 Port Initialize (RXD0 : GPA0, TXD0: GPA1)
        g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<0)) | (0x22<<0);        // GPA0->RXD0, GPA1->TXD0
        g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<0)) | (0x1<<0);            // RXD0: Pull-down, TXD0: pull up/down disable
    }
    else if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART1)
    {
        // UART1 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<2);        // UART1
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART1 Port Initialize (RXD1 : GPA4, TXD1: GPA5)
        g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<16)) | (0x22<<16);    // GPA4->RXD1, GPA5->TXD1
        g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<8)) | (0x1<<8);            // RXD1: Pull-down, TXD1: pull up/down disable
    }
    else if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART2)
    {
        // UART2 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<3);        // UART2
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART2 Port Initialize (RXD2 : GPB0, TXD2: GPB1)
        g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<0)) | (0x22<<0);        // GPB0->RXD2, GPB1->TXD2
        g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<0)) | (0x1<<0);            // RXD2: Pull-down, TXD2: pull up/down disable
    }
    else if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART3)
    {
        // UART3 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<4);        // UART3
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART3 Port Initialize (RXD3 : GPB2, TXD3: GPB3)
        g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<8)) | (0x22<<8);        // GPB2->RXD3, GPB3->TXD3
        g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<4)) | (0x1<<4);            // RXD3: Pull-down, TXD3: pull up/down disable
    }

    // Initialize UART
    //
    g_pUARTReg->ULCON = (0<<6)|(0<<3)|(0<<2)|(3<<0);                    // Normal Mode, No Parity, 1 Stop Bit, 8 Bit Data
    g_pUARTReg->UCON = (0<<10)|(1<<9)|(1<<8)|(0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0);    // PCLK divide, Polling Mode
    g_pUARTReg->UFCON = (0<<6)|(0<<4)|(0<<2)|(0<<1)|(0<<0);            // Disable FIFO
    g_pUARTReg->UMCON = (0<<5)|(0<<4)|(0<<0);                        // Disable Auto Flow Control

    Div = (float)((float)S3C6410_PCLK/(16.0*(float)CBR_115200)) - 1;
    DivSlot = (UINT32)((Div-(int)Div)*16);

    g_pUARTReg->UBRDIV = (UINT32)Div;
    g_pUARTReg->UDIVSLOT = aSlotTable[DivSlot];

    pSerInfo->bestSize = 1;        // read it one by one

    KITLOutputDebugString ("[KITL] --S3C6410UART_Init()\r\n");

    return TRUE;
}

/* S3C6410UART_WriteData
 *
 *  Block until the byte is sent
 *
 *  Return Value: TRUE on success, FALSE otherwise
 */
UINT16 S3C6410UART_WriteData (UINT8 *pch, UINT16 length)
{
    if (!KitlIoPortBase)
    {
        length = 0;
    }
    else
    {
        DEBUGCHK (length == 1);

        // Wait for transmit buffer to be empty
        while ((g_pUARTReg->UTRSTAT & 0x02) == 0);

        // Send character
        g_pUARTReg->UTXH = *pch;
    }

    return length;
}


/* S3C6410UART_ReadData
 *
 *  Called from PQOAL KITL to read a byte from serial port
 *
 *  Return Value: TRUE on success, FALSE otherwise
 */
UINT16 S3C6410UART_ReadData (UINT8 *pch, UINT16 length)
{
    UCHAR uStatus;
    UINT16 count = 0;

    if (KitlIoPortBase)
    {
        uStatus = g_pUARTReg->UTRSTAT;
        if ((uStatus & 0x01) != 0)
        {
            *pch = (UINT8) g_pUARTReg->URXH;
            count = 1;
        }
    }

    return count;
}

VOID S3C6410UART_EnableInt (void)
{
    // polling, no interrupt
}

VOID S3C6410UART_DisableInt (void)
{
    // polling, no interrupt
}

void S3C6410UART_PowerOff(void)
{
    KITLOutputDebugString ("[KITL] S3C6410UART_PowerOff()\r\n");

    return;
}

void S3C6410UART_PowerOn(void)
{
    UINT32 DivSlot;
    float Div;

    OEMInitDebugSerial();    // for What ???

    KITLOutputDebugString ("[KITL] S3C6410UART_PowerOn()\r\n");

    if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART0)
    {
        // UART0 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<1);        // UART0
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART0 Port Initialize (RXD0 : GPA0, TXD0: GPA1)
        g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<0)) | (0x22<<0);        // GPA0->RXD0, GPA1->TXD0
        g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<0)) | (0x1<<0);            // RXD0: Pull-down, TXD0: pull up/down disable
    }
    else if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART1)
    {
        // UART1 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<2);        // UART1
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART1 Port Initialize (RXD1 : GPA4, TXD1: GPA5)
        g_pGPIOReg->GPACON = (g_pGPIOReg->GPACON & ~(0xff<<16)) | (0x22<<16);    // GPA4->RXD1, GPA5->TXD1
        g_pGPIOReg->GPAPUD = (g_pGPIOReg->GPAPUD & ~(0xf<<8)) | (0x1<<8);            // RXD1: Pull-down, TXD1: pull up/down disable
    }
    else if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART2)
    {
        // UART2 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<3);        // UART2
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART2 Port Initialize (RXD2 : GPB0, TXD2: GPB1)
        g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<0)) | (0x22<<0);        // GPB0->RXD2, GPB1->TXD2
        g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<0)) | (0x1<<0);            // RXD2: Pull-down, TXD2: pull up/down disable
    }
    else if (KitlIoPortBase == S3C6410_BASE_REG_PA_UART3)
    {
        // UART3 Clock Enable
        g_pSysConReg->PCLK_GATE |= (1<<4);        // UART3
        g_pSysConReg->SCLK_GATE |= (1<<5);        // UART0~3
        // UART3 Port Initialize (RXD3 : GPB2, TXD3: GPB3)
        g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xff<<8)) | (0x22<<8);        // GPB2->RXD3, GPB3->TXD3
        g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xf<<4)) | (0x1<<4);            // RXD3: Pull-down, TXD3: pull up/down disable
    }

    // Initialize UART
    //
    g_pUARTReg->ULCON = (0<<6)|(0<<3)|(0<<2)|(3<<0);                    // Normal Mode, No Parity, 1 Stop Bit, 8 Bit Data
    g_pUARTReg->UCON = (0<<10)|(1<<9)|(1<<8)|(0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0);    // PCLK divide, Polling Mode
    g_pUARTReg->UFCON = (0<<6)|(0<<4)|(0<<2)|(0<<1)|(0<<0);            // Disable FIFO
    g_pUARTReg->UMCON = (0<<5)|(0<<4)|(0<<0);                        // Disable Auto Flow Control

    Div = (float)((float)S3C6410_PCLK/(16.0*(float)CBR_115200)) - 1;
    DivSlot = (UINT32)((Div-(int)Div)*16);

    g_pUARTReg->UBRDIV = (UINT32)Div;
    g_pUARTReg->UDIVSLOT = aSlotTable[DivSlot];
}

// KITL Serial Driver function pointer
OAL_KITL_SERIAL_DRIVER DrvSerial =
{
    S3C6410UART_Init,            // pfnInit
    NULL,                        // pfnDeinit
    S3C6410UART_WriteData,        // pfnSend
    NULL,                        // pfnSendComplete
    S3C6410UART_ReadData,        // pfnRecv
    S3C6410UART_EnableInt,        // pfnEnableInts
    S3C6410UART_DisableInt,    // pfnDisableInts
    S3C6410UART_PowerOff,        // pfnPowerOff
    S3C6410UART_PowerOn,        // pfnPowerOn
    NULL,                        // pfnFlowControl
};

const OAL_KITL_SERIAL_DRIVER *GetKitlSerialDriver (void)
{
    return &DrvSerial;
}
