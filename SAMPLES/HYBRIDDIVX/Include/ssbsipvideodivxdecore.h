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

/** $Id: SsbSipVideoDivXdecore.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Adam Li
 * Andrea Graziani
 * Jonathan White
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// SsbSipVideoDivXdecore.h //

// This is the header file describing 
// the entrance function of the encoder core
// or the encore ...
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPDECORE_H__
#define ___SSBSIPDECORE_H__

#include "CMMAPI.h"

#ifdef __cplusplus
//EXPORT "C" {
extern "C" {
#endif 

#ifdef WIN32
#define STDCALL _stdcall
#else
#define STDCALL
#endif // WIN32

/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/

// SsbSipVideoDivXdecore options
#define DEC_OPT_INIT            1
#define DEC_OPT_RELEASE         2
#define DEC_OPT_SETOUT          3
#define DEC_OPT_ADJUST          4 
#define DEC_OPT_FRAME        5
#define DEC_OPT_INIT_VOL       6
#define DEC_OPT_FLUSH        7
#define DEC_OPT_VERSION        8
#define DEC_OPT_SETDEBUG     9
#define DEC_OPT_CONVERTYUV  10
#define DEC_OPT_CONVERTYV12 11
#define DEC_OPT_ALLOC_DECODER 12
#define DEC_OPT_UPDATEFRAME        13
#define DEC_OPT_UPDATEVOPTIMERES  14
#define DEC_OPT_UPDATE_REFFRAME   15



// SsbSipVideoDivXdecore return values
#define DEC_OK            0
#define DEC_MEMORY        1
#define DEC_BAD_FORMAT    2
#define DEC_INVALID_ARGUMENT 3
#define DEC_NOT_IMPLEMENTED 4

#define DECORE_VERSION        20021112

#define DEC_ADJ_POSTPROCESSING 0
#define DEC_ADJ_BRIGHTNESS 1
#define DEC_ADJ_CONTRAST 2
#define DEC_ADJ_SATURATION 3
#define DEC_ADJ_WARMTH 4
#define DEC_ADJ_SET 0
#define DEC_ADJ_RETRIEVE 0x80000000


// fixme: add watermarking to the API doc
#include "SsbSipVideoDivXportab.h"

    
typedef struct
{
    uint32_t codec_version;
    uint32_t smooth_playback;
    uint32_t flag_logo;
    void* (*alloc) (uint32_t);
    void (*free) (void*);
}DEC_INIT;

#ifndef  DivXBitmapInfoHeader_defined
#define DivXBitmapInfoHeader_defined
typedef struct tagDivXBitmapInfoHeader 
{
    uint32_t biSize; 
    int32_t  biWidth; 
    int32_t  biHeight; 
    uint16_t biPlanes; 
    uint16_t biBitCount;
    uint32_t biCompression; 
    uint32_t biSizeImage; 
    int32_t  biXPelsPerMeter; 
    int32_t  biYPelsPerMeter; 
    uint32_t biClrUsed; 
    uint32_t biClrImportant; 
} DivXBitmapInfoHeader; 
#endif

typedef struct
{
    void *bmp; // decoded bitmap 
    void *phy_buff; // decoded bitmap 
    void *bitstream; // decoder buffer
    uint32_t length; // length of the decoder stream
    uint32_t render_flag;    // 1: the frame is going to be rendered
    uint32_t stride; // decoded bitmap stride, in pixels ( not bytes! is it good? )
    uint32_t skip_decoding;
    unsigned int is_packed;
    unsigned int file_format;
} DEC_FRAME;

#ifdef _REDUNDANTCODE_
typedef struct
{
    const char *quant_store;
    uint32_t quant_stride;
    uint32_t prediction_type;
    uint32_t frame_length;
    uint32_t frame_num;
    uint32_t vop_coded;

    void *y;
    void *u;
    void *v;
    uint32_t stride_y;
    uint32_t stride_uv;

} DEC_FRAME_INFO;


typedef struct
{
    uint32_t x_dim; 
    uint32_t y_dim;
    uint32_t time_incr;
    uint32_t codec_version;
    uint32_t build_number;
    uint32_t prefixed;
} DEC_VOL_INFO;
#endif




/**
 *
**/
/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

// the prototype of the SsbSipVideoDivXdecore() - main SsbSipVideoDivXdecore engine entrance
//
int STDCALL SsbSipVideoDivXdecore(void* handle, void *cmm_handle, int dec_opt, void *param1, void *param2);

#ifdef __cplusplus
}
#endif 

#endif // ___SSBSIPDECORE_H__

