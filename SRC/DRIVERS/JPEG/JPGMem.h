//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __JPG_MEM_H__
#define __JPG_MEM_H__

#include <windows.h>
#include "image_cfg.h"
#include <ceddk.h>

#define JPG_REG_BASE_ADDR       (0x78800000)
#define JPG_DATA_BASE_ADDR      (IMAGE_MFC_BUFFER_PA_START) 
#define IMAGE_JPG_BUFFER_SIZE   (IMAGE_MFC_BUFFER_SIZE)

#define MAX_JPG_WIDTH        2048
#define MAX_JPG_HEIGHT       1536

#define MAX_JPG_THUMBNAIL_WIDTH     160
#define MAX_JPG_THUMBNAIL_HEIGHT 120

#define MAX_RGB_WIDTH        800
#define MAX_RGB_HEIGHT       480

// memory area is 4k(PAGE_SIZE) aligned because of VirtualCopyEx()
#define JPG_STREAM_BUF_SIZE        ((MAX_JPG_WIDTH * MAX_JPG_HEIGHT )/PAGE_SIZE + 1)*PAGE_SIZE
#define JPG_STREAM_THUMB_BUF_SIZE  ((MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT )/PAGE_SIZE + 1)*PAGE_SIZE
#define JPG_FRAME_BUF_SIZE         ((MAX_JPG_WIDTH * MAX_JPG_HEIGHT * 2)/PAGE_SIZE + 1)*PAGE_SIZE
#define JPG_FRAME_THUMB_BUF_SIZE   ((MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT * 2)/PAGE_SIZE + 1)*PAGE_SIZE
#define JPG_RGB_BUF_SIZE           ((MAX_RGB_WIDTH * MAX_RGB_HEIGHT*4)/PAGE_SIZE + 1)*PAGE_SIZE

#define JPG_TOTAL_BUF_SIZE         (JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE \
                                        + JPG_FRAME_BUF_SIZE + JPG_FRAME_THUMB_BUF_SIZE + JPG_RGB_BUF_SIZE)

#define COEF1_RGB_2_YUV    0x4d971e
#define COEF2_RGB_2_YUV    0x2c5783
#define COEF3_RGB_2_YUV    0x836e13

/*
 * JPEG HW Register Macro Definition
 */
#define JPG_1BIT_MASK           1
#define JPG_4BIT_MASK           0xF

#define JPG_SMPL_MODE_MASK    0x07    // SubSampling_Mode Mask is JPGMOD Register [2:0] bits mask

#define JPG_RESTART_INTRAVEL    2    // Restart Interval value in JPGDRI Register is 2
#define JPG_MODESEL_YCBCR       1    // Mode Sel in JPGMISC Register : YCbCr422
#define JPG_MODESEL_RGB         2    // Mode Sel in JPGMISC Register : RGB565

#define JPG_JPEG_RATIO_BIT      24    // JPEG_RATIO is CLK_DIV0 Register 24th bit
#define JPG_HCLK_JPEG_BIT       11    // HCLK_JPEG is HCLK_GATE Register 11th bit
#define JPG_SCLK_JPEG_BIT       1    // SCLK_JPEG is SCLK_GATE Register 1th bit
#define JPG_SMPL_MODE_BIT       0    // SubSampling_Mode is JPGMOD Register 0th bit
#define JPG_QUANT_TABLE1_BIT    8    // Quantization Table #1 is JPGQHNO Register 8th bit
#define JPG_QUANT_TABLE2_BIT    10    // Quantization Table #2 is JPGQHNO Register 10th bit
#define JPG_QUANT_TABLE3_BIT    12    // Quantization Table #3 is JPGQHNO Register 12th bit
#define JPG_MODE_SEL_BIT        5    // Mode Sel is JPGMISC Register 5th bit

#define JPG_DECODE              (0x1 << 3)
#define JPG_ENCODE              (0x0 << 3)

#define JPG_RESERVE_ZERO        (0b000 << 2)

#define ENABLE_MOTION_ENC  (0x1<<3)
#define DISABLE_MOTION_ENC (0x0<<3)

#define ENABLE_MOTION_DEC  (0x1<<0)
#define DISABLE_MOTION_DEC (0x0<<0)

#define ENABLE_HW_DEC      (0x1<<2)
#define DISABLE_HW_DEC     (0x0<<2)

#define INCREMENTAL_DEC    (0x1<<3)
#define NORMAL_DEC         (0x0<<3)
#define YCBCR_MEMORY       (0x1<<5)

#define ENABLE_IRQ         (0xf<<3)

/*
** S3C6410 JPEG Register for Host Interfacing
**  TYPE          RegName        // RegAddress
*/
typedef struct tagS3C6410_JPG_HOSTIF_REG
{
    UINT32        JPGMod;         //0x000
    UINT32        JPGStatus;      //0x004
    UINT32        JPGQTblNo;      //0x008
    UINT32        JPGRSTPos;      //0x00C
    UINT32        JPGY;           //0x010
    UINT32        JPGX;           //0x014
    UINT32        JPGDataSize;    //0x018
    UINT32        JPGIRQ;         //0x01C
    UINT32        JPGIRQStatus;   //0x020
    UINT32        dummy0[247];    //Not Defined

    UINT32        JQTBL0[64];     //0x400
    UINT32        JQTBL1[64];     //0x500
    UINT32        JQTBL2[64];     //0x600
    UINT32        JQTBL3[64];     //0x700
    UINT32        JHDCTBL0[16];   //0x800
    UINT32        JHDCTBLG0[12];  //0x840
    UINT32        dummy1[4];      //Not Defined
    UINT32        JHACTBL0[16];   //0x880
    UINT32        JHACTBLG0[162]; //0x8c0
    UINT32        dummy2[46];     //Not Defined
    UINT32        JHDCTBL1[16];   //0xc00
    UINT32        JHDCTBLG1[12];  //0xc40
    UINT32        dummy3[4];      //Not Defined
    UINT32        JHACTBL1[16];   //0xc80
    UINT32        JHACTBLG1[162]; //0xcc0
    UINT32        dummy4[46];

    UINT32        JPGYUVAddr0;    //0x1000
    UINT32        JPGYUVAddr1;    //0x1004
    UINT32        JPGFileAddr0;   //0x1008
    UINT32        JPGFileAddr1;   //0x100c
    UINT32        JPGStart;       //0x1010
    UINT32        JPGReStart;     //0x1014
    UINT32        JPGSoftReset;   //0x1018
    UINT32        JPGCntl;        //0x101c
    UINT32        JPGCOEF1;       //0x1020
    UINT32        JPGCOEF2;       //0x1024
    UINT32        JPGCOEF3;       //0x1028
    UINT32        JPGMISC;        //0x102c
    UINT32        JPGFrameIntv;   //0x1030
}S3C6410_JPG_HOSTIF_REG;

typedef struct tagS3C6410_JPG_CTX
{
    volatile S3C6410_JPG_HOSTIF_REG  *v_pJPG_REG;
    volatile UINT8                   *v_pJPGData_Buff;
    HANDLE                           callerProcess;
    unsigned char                    *strUserBuf;
    unsigned char                    *frmUserBuf;
    unsigned char                    *strUserThumbBuf;
    unsigned char                    *frmUserThumbBuf;
    unsigned char                    *rgbBuf;
}S3C6410_JPG_CTX;

void *Phy2VirAddr(UINT32 phy_addr, int mem_size);
void FreeVirAddr(void * vir_addr, int mem_size);

BOOL JPGMemMapping(S3C6410_JPG_CTX *base);
void JPGMemFree(S3C6410_JPG_CTX *base);
BOOL JPGBuffMapping(S3C6410_JPG_CTX *base);
void JPGBuffFree(S3C6410_JPG_CTX *base);
BOOL HWPostMemMapping(void);
void HWPostMemFree(void);

#endif
