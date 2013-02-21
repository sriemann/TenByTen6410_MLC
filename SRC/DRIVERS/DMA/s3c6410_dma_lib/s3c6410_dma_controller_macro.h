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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------

//
// s3c6410_dma_controller_macro.h

#ifndef _S3C6410_DMA_CONTROLLER_MACRO_H_
#define _S3C6410_DMA_CONTROLLER_MACRO_H_

#if __cplusplus
extern "C"
{
#endif

#define DMA_CH_REG_OFFSET       (0x100)
#define DMA_CH_REG_SIZE         (0x20)

typedef struct
{
    UINT32 SrcAddr;             // 0x00
    UINT32 DestAddr;            // 0x04
    UINT32 LLI;                 // 0x08
    UINT32 Control0;            // 0x0C

    UINT32 Control1;            // 0x10
    UINT32 Configuration;       // 0x14
    UINT32 PAD[2];              // 0x18~0x1F
} S3C6410_DMA_CH_REG, *PS3C6410_DMA_CH_REG;

typedef struct
{
    UINT32 SrcAddr;
    UINT32 DestAddr;
    UINT32 LLI;
    UINT32 Control0;
    UINT32 Control1;
} DMA_LLI_ENTRY, *PDMA_LLI_ENTRY;

// DMACIntStatus
// DMACIntTCStatus
// DMACIntTCClear
// DMACIntErrStatus
// DMACIntErrClear;
// DMACRawIntTCStatus
// DMACRawIntErrStatus
// DMACEnbldChns
// DMACSoftBReq
// DMACSoftSReq
// DMACSoftLBReq
// DMACSoftLSReq

// DMACConfiguration
#define M2_LITTLE_ENDIAN            (0<<2)
#define M2_BIG_ENDIAN               (1<<2)
#define M1_LITTLE_ENDIAN            (0<<1)
#define M1_BIG_ENDIAN               (1<<1)
#define DMAC_ENABLE                 (1<<0)

// DMACSync

// DMACCxSrcAddr
// DMACCxDestAddr

// DMACCxLLI
#define NEXT_LLI_ITEM(n)            ((n)&0xFFFFFFFC)    // Remove LSB 2 bit
#define LM_AHB_M1                   (0<<0)
#define LM_AHB_M2                   (1<<0)

// DMACCxControl0
#define TCINT_ENABLE                (1<<31)
#define PROT_NOT_CACHEABLE          (0<<30)
#define PROT_CACHEABLE              (1<<30)
#define PROT_NOT_BUFFERABLE         (0<<29)
#define PROT_BUFFERABLE             (1<<29)
#define PROT_USER_MODE              (0<<28)
#define PROT_PREVILEGED_MODE        (1<<28)
#define DEST_FIXED                  (0<<27)
#define DEST_INCREMENT              (1<<27)
#define SRC_FIXED                   (0<<26)
#define SRC_INCREMENT               (1<<26)
#define DEST_AHB_M1                 (0<<25)
#define DEST_AHB_M2                 (1<<25)
#define SRC_AHB_M1                  (0<<24)
#define SRC_AHB_M2                  (1<<24)
#define DEST_UNIT_BYTE              (0<<21)
#define DEST_UNIT_HWORD             (1<<21)
#define DEST_UNIT_WORD              (2<<21)
#define DEST_UNIT_MASK              (7<<21)
#define SRC_UNIT_BYTE               (0<<18)
#define SRC_UNIT_HWORD              (1<<18)
#define SRC_UNIT_WORD               (2<<18)
#define SRC_UNIT_MASK               (7<<18)
#define DEST_BURST_1                (0<<15)
#define DEST_BURST_4                (1<<15)
#define DEST_BURST_8                (2<<15)
#define DEST_BURST_16               (3<<15)
#define DEST_BURST_32               (4<<15)
#define DEST_BURST_64               (5<<15)
#define DEST_BURST_128              (6<<15)
#define DEST_BURST_256              (7<<15)
#define DEST_BURST_MASK             (7<<15)
#define SRC_BURST_1                 (0<<12)
#define SRC_BURST_4                 (1<<12)
#define SRC_BURST_8                 (2<<12)
#define SRC_BURST_16                (3<<12)
#define SRC_BURST_32                (4<<12)
#define SRC_BURST_64                (5<<12)
#define SRC_BURST_128               (6<<12)
#define SRC_BURST_256               (7<<12)
#define SRC_BURST_MASK              (7<<12)

// DMACCxControl1
#define TRANSFERCOUNT(n)        ((n)&0xFFFFFF)

// DMACCxConfiguration
#define ALLOW_REQUEST               (0<<18)
#define HALT                        (1<<18)
#define ACTIVE                      (1<<17)
#define UNLOCK                      (0<<16)
#define LOCK                        (1<<16)
#define TCINT_UNMASK                (1<<15)
#define ERRINT_UNMASK               (1<<14)
#define FLOWCTRL(n)                 (((n)&0x7)<<11)
#define FLOWCTRL_MEM2MEM            (0<<11)
#define FLOWCTRL_MEM2PERI           (1<<11)
#define FLOWCTRL_PERI2MEM           (2<<11)
#define FLOWCTRL_PERI2PERI          (3<<11)
#define FLOWCTRL_MASK               (7<<11)
#define ONENANDMODEDST              (1<<10)
#define DEST_PERI(n)                (((n)&0xf)<<6)
#define ONENANDMODESRC              (1<<5)
#define SRC_PERI(n)                 (((n)&0xf)<<1)
#define CHANNEL_ENABLE              (1<<0)

#if __cplusplus
}
#endif

#endif    // _S3C6410_DMA_CONTROLLER_MACRO_H_
