//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
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

Module Name:    module.h

Abstract:       common Camera module descriptor

Functions:


Notes:


--*/

#ifndef _MODULE_H
#define _MODULE_H

typedef struct _MODULE_DESCRIPTOR
{
    BYTE ITUXXX;            // if ITU-601 8bit, set 1. if ITU-656 8bit, set 0
    BYTE UVOffset;          // Cb, Cr value offset. 1: +128 , 0: 0
    BYTE Order422;          // 0x00:YCbYCr, 0x01:YCrYCb, 0x10:CbYCrY, 0x11:CrYCbY
    BYTE Codec;             // 422: 1   , 420: 0
    BYTE HighRst;           // Reset is    Low->High: 0   High->Low: 1 
    BYTE InvPCLK;           // 1: inverse the polarity of PCLK    0 : normal
    BYTE InvVSYNC;          // 1: inverse the polarity of VSYNC   0 : normal
    BYTE InvHREF;           // 1: inverse the polarity of HREF      0 : normal
    UINT32 SourceHSize;     // Horizontal size
    UINT32 SourceVSize;     // Vertical size
    UINT32 SourceHOffset;   // Horizontal size
    UINT32 SourceVOffset;   // Vertical size    
    UINT32 Clock;           // clock
} MODULE_DESCRIPTOR;

#define CAM_ITU601              (1)
#define CAM_ITU656              (0)

#define CAM_ORDER_YCBYCR        (0)
#define CAM_ORDER_YCRYCB        (1)
#define CAM_ORDER_CBYCRY        (2)
#define CAM_ORDER_CRYCBY        (3)

#define CAM_UVOFFSET_0          (0)
#define CAM_UVOFFSET_128        (1)

#define CAM_CODEC_422           (1)
#define CAM_CODEC_420           (0)

typedef enum
{
    QCIF, CIF/*352x288*/, 
    QQVGA, QVGA, VGA, SVGA/*800x600*/, SXGA/*1280x1024*/, UXGA/*1600x1200*/, QXGA/*2048x1536*/,
    WVGA/*854x480*/, HD720/*1280x720*/, HD1080/*1920x1080*/,
    SUB_SAMPLING2/*800x600*/, SUB_SAMPLING4/*400x300*/
} IMG_SIZE;


int     ModuleInit();
void    ModuleDeinit();
void    ModuleGetFormat(MODULE_DESCRIPTOR &outModuleDesc);
int     ModuleWriteBlock();
int     ModuleSetImageSize(int imageSize);

#endif