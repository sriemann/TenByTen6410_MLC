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

/** $Id: SsbSipVideoDivXlogo_adder.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

// SsbSipVideoDivXlogo_adder.h //
/*******************************************************************************
                        Samsung India Software Operations Pvt. Ltd. (SISO)
                                    Copyright 2006
;*******************************************************************************/

#ifndef _LOGO_ADDER_H_
#define ___SSBSIPLOGO_ADDER_H__
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
typedef struct logo_adder_s {
    int logo_start_pos_x;
    int logo_start_pos_y;

    int logo_width;
    int logo_height;
    uint8_t* logo_matrix[3]; // b&w yuv 420

    // other... timer and shine information
} logo_adder_t;

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

void SsbSipVideoDivXlogo_adder_init(logo_adder_t * this_la, int image_width, int image_height);
void SsbSipVideoDivXlogo_adder_release(logo_adder_t * this_la);

typedef void (SsbSipVideoDivXProc_logo_adder_go)(uint8_t* image_bmp, int image_stride, const uint8_t* logo_matrix, int logo_width, int magnitude);
SsbSipVideoDivXProc_logo_adder_go logo_adder_go_generic;
EXPORT SsbSipVideoDivXProc_logo_adder_go * logo_adder_go;

void SsbSipVideoDivXadd_logo(logo_adder_t * this_la, uint8_t* image_bmp, int image_stride, int pos_x, int pos_y, int magnitude);

#ifdef __cplusplus
}
#endif
#endif // ___SSBSIPLOGO_ADDER_H__

/**
    SsbSipVideoDivXlogo_adder_init - Specify the width and height of the image (this permits to decide where
    the logo has to be displayed, logo_start_pos_x and logo_start_pos_y), allocate space for 
    the logo_matrix and initialize the logo matrix. 

    SsbSipVideoDivXlogo_adder_release - Release the data allocated for the logo adder.

    logo_adder_go - Draw the logo in the specified bitmap (magnitude indicates the 
    required visibility 0 - 100). 
**/
