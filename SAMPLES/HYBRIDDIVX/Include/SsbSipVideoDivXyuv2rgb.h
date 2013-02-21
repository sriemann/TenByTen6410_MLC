/**
 ** Copyright (C) 2002 DivXNetworks, all rights reserved.
 **
 ** DivXNetworks, Inc. Proprietary & Confidential
 **
 ** This source code and the algorithms implemented therein constitute
 ** confidential information and may comprise trade secrets of DivXNetworks
 ** or its associates, and any use thereof is subject to the terms and
 ** conditions of the Non-Disclosure Agreement pursuant to which this
 ** source code was originally received.
 **
 **/

/** $Id: SsbSipVideoDivXyuv2rgb.h,v 1.1.1.1 2003/04/23 23:24:26 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// yuvrgb.h //

// 13.Feb.01 John Funnell: a negative height value now signifies that the output should be flipped
// 18 Apr.01 James Leiterman: added function pointers and platform init prototypes for multi-skew platforms

/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
#ifdef YUV2RGB
#include "SsbSipVideoDivXportab.h"
#include "SsbSipVideoDivXmp4_vars.h"

#ifndef ___SSBSIPYUVRGB_H__
#define ___SSBSIPYUVRGB_H__


#ifdef __cplusplus
EXPORT "C" {
#endif 
/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/

#ifndef POWERPC

#define MAX_WIDTH  720
#define MAX_RINDEX 1024
#define DITHER_DIM MAX_RINDEX+MAX_WIDTH
#define DITHER_MAX 9


void init_yuv2rgb(void);
void init_dither(void);

typedef void (yuv2rgbProc)(const uint8_t *puc_y, int stride_y, 
    const uint8_t *puc_u, const uint8_t *puc_v, int stride_uv, 
    uint8_t *puc_out,  int width_y, int height_y, 
    unsigned int stride_out, const GAMMA_ADJUSTMENT* pG);
typedef yuv2rgbProc* yuv2rgbProcPtr;    

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

EXPORT yuv2rgbProcPtr YV12toRGB24;
EXPORT yuv2rgbProcPtr YV12toRGB24_mb;
EXPORT yuv2rgbProcPtr YV12toRGB32;
EXPORT yuv2rgbProcPtr YV12toRGB32_mb;
EXPORT yuv2rgbProcPtr YV12toBGR24; // bgr
EXPORT yuv2rgbProcPtr YV12toBGR24_mb; // bgr
EXPORT yuv2rgbProcPtr YV12toBGR32; // bgr
EXPORT yuv2rgbProcPtr YV12toBGR32_mb; // bgr
EXPORT yuv2rgbProcPtr YV12toRGB555;
EXPORT yuv2rgbProcPtr YV12toRGB565;
EXPORT yuv2rgbProcPtr YV12toRGB24;
EXPORT yuv2rgbProcPtr YV12toRGB8;
EXPORT yuv2rgbProcPtr YV12toYUY2;
EXPORT yuv2rgbProcPtr YV12toYUY2_mb;
EXPORT yuv2rgbProcPtr YV12toYV12;
EXPORT yuv2rgbProcPtr YV12toUYVY;

EXPORT yuv2rgbProc YV12toRGB24_generic;
EXPORT yuv2rgbProc YV12toRGB32_generic;
EXPORT yuv2rgbProc YV12toBGR24_generic; // bgr
EXPORT yuv2rgbProc YV12toBGR32_generic; // bgr
EXPORT yuv2rgbProc YV12toRGB555_generic;
EXPORT yuv2rgbProc YV12toRGB565_generic;
EXPORT yuv2rgbProc YV12toRGB24_generic;
EXPORT yuv2rgbProc YV12toYUY2_generic;
EXPORT yuv2rgbProc YV12toYV12_generic;
EXPORT yuv2rgbProc YV12toUYVY_generic;
EXPORT yuv2rgbProc YV12toRGB8_generic;

EXPORT yuv2rgbProc YV12toRGB24_reference;
EXPORT yuv2rgbProc YV12toRGB32_reference;

#else


typedef void (*yuv2rgbProc)(uint8_t *puc_y, int stride_y, 
    uint8_t *puc_u, uint8_t *puc_v, int stride_uv, 
    uint8_t *puc_out,  int width_y, int height_y, 
    unsigned int stride_out, GAMMA_ADJUSTMENT* pG);


EXPORT yuv2rgbProc yuv2rgb_32;    
EXPORT yuv2rgbProc yuv2rgb_24;    
EXPORT yuv2rgbProc yuv2rgb_555;    
EXPORT yuv2rgbProc yuv2rgb_565;
EXPORT yuv2rgbProc yuv12_out;
EXPORT yuv2rgbProc yuy2_out;
EXPORT yuv2rgbProc uyvy_out;


#endif // !POWERPC

#ifdef __cplusplus
}
#endif

#endif // ___SSBSIPYUVRGB_H__
#endif//YUV2RGB
    
    
    


