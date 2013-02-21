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

/** $Id: mp4_header.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
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
/*******************************************************************************
                Samsung India Software Operations Pvt. Ltd. (SISO)
                        Copyright 2006
;*******************************************************************************/
// mp4_header.h //

#ifndef ___MP4_HEADER_H__
#define ___MP4_HEADER_H__

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

EXPORT int log2ceil(int arg);

/* All get*hdr functions return 0 if successful, -1 if header is not found, positive error code if an error happens */
EXPORT int SsbSipVideoDivXgetvoshdr(MP4_STREAM * _ld, MP4_STATE * _mp4_state, int just_vol_init);
EXPORT int SsbSipVideoDivXgetvsohdr(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
EXPORT int SsbSipVideoDivXgetvolhdr(MP4_STREAM * _ld, MP4_STATE * _mp4_state, int just_vol_init);
EXPORT int SsbSipVideoDivXgetshvhdr(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
EXPORT int SsbSipVideoDivXgetgophdr(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
EXPORT int SsbSipVideoDivXgetvophdr(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
EXPORT int SsbSipVideoDivXgetvophdr_311(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
EXPORT int SsbSipVideoDivXgetpackethdr(MP4_STREAM * _ld, MP4_STATE * _mp4_state);

EXPORT int SsbSipVideoDivXget_use_intra_dc_vlc(int quantizer, int intra_dc_vlc_thr);

EXPORT int SsbSipVideoDivXnextbits(MP4_STREAM * _ld, int nbits);
EXPORT int SsbSipVideoDivXbytealign(MP4_STREAM * _ld);
EXPORT int SsbSipVideoDivXbytealigned(MP4_STREAM * _ld, int nbits);
EXPORT void SsbSipVideoDivXnext_start_code(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
EXPORT void SsbSipVideoDivXnext_resync_marker(MP4_STREAM * ld, MP4_STATE * mp4_state);
EXPORT int SsbSipVideoDivXnextbits_bytealigned(MP4_STREAM * _ld, int nbit, int short_video_header);

EXPORT int SsbSipVideoDivXnextbits_resync_marker(MP4_STREAM * ld, MP4_STATE * mp4_state);
EXPORT int SsbSipVideoDivXget_resync_marker(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
EXPORT int SsbSipVideoDivXcheck_sync_marker(MP4_STREAM* ld);
void getusrhdr(MP4_STREAM * ld, MP4_STATE * mp4_state, int just_vol_init);
int blockInterNew(REFERENCE * ref, int block_num, int coded, int *dc_coeff);


#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___MP4_HEADER_H__
