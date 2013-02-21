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

/** $Id: noise_adder.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/* noise_adder.h */

/* John Funnell, March 2002 */
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPNOISE_ADDER_H__
#define ___SSBSIPNOISE_ADDER_H__

/*
// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
*/

#include "SsbSipVideoDivXportab.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/


#define MAX_IMAGE_WIDTH 1920
#define MAX_RECT_SIZE_Y 16

#ifdef _REDUNDANTCODE_
typedef struct noise_adder_s {
    /* this is a PRIVATE structure, members should only be accessed by calls to the functions below */
    int image_stride;
    int rect_size_x;
    int rect_size_y;
    int level;
    int min_value;
    uint8_t shift_register_8bit[16][32];
    int     shift_register_pos[16];

    int virgin;
    uint8_t previous_byte[MAX_RECT_SIZE_Y];
    int8_t  previous_line[MAX_IMAGE_WIDTH];


    /* MMX things, could also be used by another 64-bit implementation ... */
    int      mmx_initialised;
    uint8_t  mmx_shift_registers_space[32*32+31];
    uint8_t* mmx_shift_registers; /* this needs to be 32-byte aligned */
    int      mmx_shift_register_pos;
    /* ...MMX things */
//#endif
} noise_adder_t;

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/


void SsbSipVideoDivXnoise_adder_init(noise_adder_t* this_na, int image_width, int image_height, int image_stride, int rect_size_x, int rect_size_y);

typedef void (SsbSipVideoDivXProc_noise_adder_set)(noise_adder_t* this_na, int level);
SsbSipVideoDivXProc_noise_adder_set SsbSipVideoDivXnoise_adder_generic_set;
EXPORT SsbSipVideoDivXProc_noise_adder_set *SsbSipVideoDivXnoise_adder_set;

uint8_t SsbSipVideoDivXnoise_adder_shift_it(noise_adder_t* this_na, int n);

typedef void (SsbSipVideoDivXProc_noise_adder_go)(noise_adder_t* this_na, uint8_t* rect, int xpos, int ypos);
SsbSipVideoDivXProc_noise_adder_go SsbSipVideoDivXnoise_adder_generic_go;
EXPORT SsbSipVideoDivXProc_noise_adder_go *SsbSipVideoDivXnoise_adder_go;


#endif //_REDUNDANTCODE
#ifdef __cplusplus
}
#endif
#endif
