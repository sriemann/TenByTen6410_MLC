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
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    priv_context.h

Abstract:

    The definition of HW context
    
Functions:


Notes:

--*/

#ifndef __S3C6410_DISP_PRIVATE_CONTEXT_H__
#define __S3C6410_DISP_PRIVATE_CONTEXT_H__

typedef enum
{
    VB_FRONTPORCH =0,
    VB_VSYNC,
    VB_BACKPORCH
} VB_STATUS;

typedef enum
{
    OUTPUT_IF_RGB,
    OUTPUT_IF_TV
} OUTPUT_INTERFACE;

typedef enum
{
    TV_DMA_DISABLE,
    TV_DMA_PRIMARY,
    TV_DMA_OVERLAY
} TV_DMA_MODE;

typedef enum
{
    TV_DMA_RGB565,
    TV_DMA_RGBX888,
    TV_DMA_I420,
    TV_DMA_YV12,
    TV_DMA_YUYV,
    TV_DMA_YVYU,
    TV_DMA_UYVY,
    TV_DMA_VYUY
} TV_DMA_SOURCE_FORMAT;

typedef struct _OverlayContext
{
    S3C6410Surf *pSurface;        // Current Overlay Surface
    S3C6410Surf *pPrevSurface;    // Previous Flipped Overlay Surface

    DWORD       dwWinMode;
    DWORD       dwBPPMode;
    DWORD       dwPostSrcType;
    BOOL        bLocalPath;

    unsigned int    uiSrcWidth;
    unsigned int    uiSrcHeight;
    unsigned int    uiSrcOffsetX;
    unsigned int    uiSrcOffsetY;
    unsigned int    uiDstWidth;
    unsigned int    uiDstHeight;
    unsigned int    uiDstOffsetX;
    unsigned int    uiDstOffsetY;
    BOOL        bEnabled;        // TRUE menas FIMD window for Overlay is enabled
    BOOL        bShow;            // TRUE menas Overlay set to SHOW in HalUpdateOverlay()

    // Blending Information
    BOOL        bBlendOn;
    BOOL        bColorKey;        // TRUE measn Color Key, FALSE means Alpha Blending

    // Color Key Information
    BOOL        bSrcCKey;        // TRUE measn SrcCKey, FALSE means DestCKey
    unsigned int    CompareKey;    // Compare Key (Mask)
    unsigned int    ColorKey;        // Color Key (Value)

    // Alpha Blending Information
    BOOL        bUsePixelBlend;    // TRUE means PerPixel Blending, FALSE means PerPlane Blending
    unsigned int    Alpha;            // Alpha Value
} OverlayContext;

typedef struct _TVDMAContext
{
    DWORD        dwSourceFormat;
    unsigned int    uiSrcBaseWidth;
    unsigned int    uiSrcBaseHeight;
    unsigned int    uiSrcWidth;
    unsigned int    uiSrcHeight;
    unsigned int    uiSrcOffsetX;
    unsigned int    uiSrcOffsetY;
    unsigned int    uiBufferRGBY;
    unsigned int    uiBufferCb;
    unsigned int    uiBufferCr;
} TVDMAContext;

// This definition is used for G2D_ACCEL DrvEscape to control 2DHW Acceleration grade
typedef struct _G2D_ACCEL_CONTROL_ARGS{
    unsigned int BltLimitSize;
    unsigned int AllocBoundSize;
    unsigned int AccelLevel;
    unsigned int UseSWAccel;
    unsigned int SetBltLimitSize;
    unsigned int SetAllocBound;
    unsigned int UsePACSurf;
    unsigned int UseStretchBlt;
    unsigned int CachedBlt;
    unsigned int HWOnOff;
    unsigned int UseAlphaBlend;
    unsigned int UseBitBlt;
    unsigned int UseLineDraw;
    unsigned int UseFillRect;
    unsigned int OverrideEmulFunc;
} G2D_ACCEL_CONTROL_ARGS;

#define DRVESC_OUTPUT_BASE              (0x00020100)
#define DRVESC_OUTPUT_RGB               (DRVESC_OUTPUT_BASE+0)
#define DRVESC_OUTPUT_TV                (DRVESC_OUTPUT_BASE+1)
#define DRVESC_OUTPUT_SWITCH            (DRVESC_OUTPUT_BASE+2)
#define DRVESC_TV_DMA_DISABLE           (DRVESC_OUTPUT_BASE+10)
#define DRVESC_TV_DMA_PRIMARY           (DRVESC_OUTPUT_BASE+11)
#define DRVESC_TV_DMA_OVERLAY           (DRVESC_OUTPUT_BASE+12)

#ifndef DRVESC_G2D_ACCEL_SET
#define DRVESC_G2D_ACCEL_SET        (0x00020200)
#endif
#ifndef DRVESC_G2D_ACCEL_GET
#define DRVESC_G2D_ACCEL_GET        (0x00020201)
#endif

#ifndef GETRAWFRAMEBUFFER
#define GETRAWFRAMEBUFFER       (0x00020001)
#define RAW_FORMAT_565          (1)
#define RAW_FORMAT_555          (2)
#define RAW_FORMAT_OTHER        (3)
typedef struct _RawFrameBufferInfo
{
    WORD    wFormat;
    WORD    wBPP;
    VOID    *pFramePointer;
    int        cxStride;
    int        cyStride;
    int        cxPixels;
    int        cyPixels;
} RawFrameBufferInfo;
#endif

#endif __S3C6410_DISP_PRIVATE_CONTEXT_H__