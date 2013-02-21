//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install mediaS3C6410
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
#ifndef __PDDS3C6410_SER_DMA_H_
#define __PDDS3C6410_SER_DMA_H_
#include <cmthread.h>
#include <S3C6410_intr.h>
#include <bsp_cfg.h>
#include <S3C6410_dma_controller.h>


#define UART1_TX_DATA_PHY_ADDR 	0x7F005420
#define UART1_RX_DATA_PHY_ADDR 	0x7F005424
/* DMA Related Start*/
#define TX_DMA_THREAD_PRIORITY 106

//DMA Buffer Address Alloc
#define Buffer_Mem_Size 4096//10240    //4K fit for BT


typedef struct {
    volatile S3C6410_DMAC_REG       *pDMAC0regs;
    volatile S3C6410_DMAC_REG       *pDMAC1regs;
// MDMA    volatile S3C6410_DMAC_REG       *pDMACregs;
    volatile S3C6410_SYSCON_REG		*pSYSCONregs;

//
    DWORD m_chnum;

//RX   
    DWORD   dwRxDmaDoneThreadId;
    DWORD   dwRxDmaDoneThreadPrio;
    DWORD   dwRxDmaDoneSysIntr;
    HANDLE  hRxDmaDoneEvent;
    HANDLE  hRxDmaDoneDoneEvent;
    HANDLE  hRxDmaDoneThread;

//TX    
    DWORD   dwTxDmaDoneThreadId;
    DWORD   dwTxDmaThreadPrio;
    DWORD   dwTxDmaDoneThreadPrio;
    DWORD   dwTxDmaDoneSysIntr;
    HANDLE  hTxDmaDoneEvent;
    HANDLE  hTxDmaDoneDoneEvent;
    HANDLE  hTxDmaDoneThread;
    
} DMA_PUBLIC_CONTEXT, *PDMA_PUBLIC_CONTEXT;


extern DMA_CH_CONTEXT   g_OutputDMA;
extern DMA_CH_CONTEXT   g_InputDMA;

extern PBYTE pVirtDmaDstBufferAddr;
extern PBYTE pVirtDmaSrcBufferAddr;

extern PDMA_PUBLIC_CONTEXT pPublicUart;

extern UINT DmaDstAddress;
extern UINT DmaSrcAddress;


#define WRITE_TIME_OUT_CONSTANT     5000
#define WRITE_TIME_OUT_MULTIPLIER   1

#define READ_TIME_OUT_CONSTANT      5000
#define READ_TIME_OUT_MULTIPLIER    1

PVOID InitializeDMA(DWORD nCH);
BOOL DmaPowerUp();
BOOL DmaPowerDown();

/* DMA Related End */

#endif
