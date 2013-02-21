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

/** $Id: mp4_recon.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
        Copyright (C) 2001 - DivXNetworks
        DivX Advanced Research Center <darc@projectmayo.com>
**/
// mp4_recon.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef _MP4_RECON_H_
#define ___SSBSIPMP4_RECON_H__
/*
// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
*/


#include "SsbSipVideoDivXmp4_vars.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
void SsbSipVideoDivXreconstruct_skip (REFERENCE * ref);



void SsbSipVideoDivXreconstruct (REFERENCE * ref,MotionVector*, int inter4vFlag);




void SsbSipVideoDivXreconstruct_bvop (REFERENCE * ref, int mode,MotionVector*);

SsbSipVideoDivXReconBlockFunc SsbSipVideoDivXrecon_comp_block;
SsbSipVideoDivXReconBlockFunc SsbSipVideoDivXrecon_clear_block;

SsbSipVideoDivXReconFunc SsbSipVideoDivXrecon_comp_16x16;
SsbSipVideoDivXReconFunc SsbSipVideoDivXrecon_clear_16x16;
//ReconFunc recon_comp_16x16_accurate;
//ReconFunc recon_comp_block_accurate;
void SsbSipVideoDivXrecon_comp_16x16_accurate(MP4_STATE* mp4_state, unsigned char *src, 
                                unsigned char *dst,    int lx, int lx_dst, int x, int y, int dx, int dy);
void SsbSipVideoDivXrecon_comp_block_accurate(MP4_STATE* mp4_state, unsigned char *src, 
                               unsigned char *dst, int lx, int lx_dst, int x, int y, int dx, int dy);

void SsbSipVideoDivXrecon_comp_8x8_chr_interlaced(MP4_STATE* mp4_state,
           unsigned char* src[3], int lx,
           unsigned char* dst[3], int lx_dst,
           int top_field_reference, 
           int bottom_field_reference,
           int dx_top, int dy_top, int dx_bottom, int dy_bottom,
           int px, int py );
void SsbSipVideoDivXrecon_comp_16x16_interlaced(MP4_STATE* mp4_state,
           unsigned char* src, int lx,
           unsigned char* dst, int lx_dst,
           int top_field_reference, 
           int bottom_field_reference,
           int dx_top, int dy_top, int dx_bottom, int dy_bottom,
           int px, int py );

typedef void (SsbSipVideoDivXreconCompAccurateProc) (unsigned char * src, unsigned char * dst, int stride, int stride_dst, int xh, int yh, int rounding);
typedef SsbSipVideoDivXreconCompAccurateProc* SsbSipVideoDivXreconCompAccurateProcPtr;

EXPORT SsbSipVideoDivXreconCompAccurateProc SsbSipVideoDivXrecon_comp_accurate_internal_generic;
EXPORT SsbSipVideoDivXreconCompAccurateProcPtr SsbSipVideoDivXrecon_comp_accurate_internal;
typedef void (SsbSipVideoDivXreconCompAffineProc) (unsigned char *src, unsigned char *dst, int lx, int lx_dst, int px, int py, AFFINE_TRANSFORM* ptrans,
                                    int warping_accuracy, int rounding, int sx, int sy);
typedef SsbSipVideoDivXreconCompAffineProc* SsbSipVideoDivXreconCompAffineProcPtr;

EXPORT SsbSipVideoDivXreconCompAffineProc SsbSipVideoDivXrecon_comp_block_affine_generic;
EXPORT SsbSipVideoDivXreconCompAffineProc SsbSipVideoDivXrecon_comp_16x16_affine_generic;

EXPORT SsbSipVideoDivXreconCompAffineProcPtr SsbSipVideoDivXrecon_comp_block_affine;
EXPORT SsbSipVideoDivXreconCompAffineProcPtr SsbSipVideoDivXrecon_comp_16x16_affine;

EXPORT void  (*const SsbSipVideoDivXset_gmc_mv_pointers[4])(MP4_STATE*);
EXPORT void (*const SsbSipVideoDivXreconstruct_gmc_pointers[4])(REFERENCE*);

#ifdef _ARM_ASSEMBLY_
void SsbSipVideoDivXrecon_bvop_average (unsigned char *src_1[3], unsigned char *src_2[3], REFERENCE * ref);
void SsbSipVideoDivXrecon_bvop_average_clear (unsigned char *src_1[3], unsigned char *src_2[3], REFERENCE * ref);
#else
void SsbSipVideoDivXrecon_bvop_average (unsigned char *src_1[3], unsigned char *src_2[3], int src_stride, int bx, int by, unsigned char *dst[3], int dst_stride);
void SsbSipVideoDivXrecon_bvop_average_clear (unsigned char *src_1[3], unsigned char *src_2[3], int src_stride, int bx, int by, unsigned char *dst[3], int dst_stride);
#endif
void SsbLibVideoDivXReconBvopAverageASM(unsigned char *src_1[3], unsigned char *src_2[3],
                        REFERENCE * ref);


#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPMP4_RECON_H__
