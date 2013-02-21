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

/** $Id: SsbSipVideoDivXmp4_block.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// SsbSipVideoDivXmp4_block.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_BLOCK_H__
#define ___SSBSIPMP4_BLOCK_H__
#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

// EXPORT int SsbSipVideoDivXblockIntra(MP4_STREAM * ld, MP4_STATE * mp4_state, short * block, int block_num, int coded);
EXPORT int SsbSipVideoDivXblockIntra(REFERENCE *ref,unsigned char * frame_back[] ,int block_num, int coded);


EXPORT int SsbSipVideoDivXblockIntra_311(unsigned char* frame_ref[], MP4_STREAM * _ld, MP4_STATE * _mp4_state, short * block, int block_num, int coded);
EXPORT int SsbSipVideoDivXblockInter_311(MP4_STREAM * _ld, MP4_STATE * _mp4_state, short * block, int block_num);

EXPORT int SsbSipVideoDivXgetDCsizeLum(MP4_STREAM * _ld);
EXPORT int SsbSipVideoDivXgetDCsizeChr(MP4_STREAM * _ld);
EXPORT int SsbSipVideoDivXgetDCdiff(MP4_STREAM * _ld, int dct_dc_size);

EXPORT int SsbSipVideoDivXDCscaler(int quantizer, int block_num);
//EXPORT int SsbSipVideoDivXintra_ac_vld(MP4_STREAM * _ld, int reversible_vlc_flag);
//EXPORT __inline unsigned char *block2pointer_interlaced (REFERENCE * ref, int comp, int bx, int by, int* stride);

#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPMP4_BLOCK_H__

