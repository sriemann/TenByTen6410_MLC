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

/** $Id: SsbSipVideoDivXmp4_mblock_util.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
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
// SsbSipVideoDivXmp4_mblock_util.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_MBLOCK_UTIL_H__
#define ___SSBSIPMP4_MBLOCK_UTIL_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/


EXPORT int SsbSipVideoDivXgetMCBPC_i_vop(MP4_STREAM * _ld);
EXPORT int SsbSipVideoDivXgetMCBPC_p_vop(MP4_STREAM * _ld);
EXPORT int SsbSipVideoDivXgetCBPY(MP4_STREAM * _ld, int intraFlag);

EXPORT int SsbSipVideoDivXsetMV(REFERENCE *ref,MotionVector *MV, int block_num);
EXPORT int SsbSipVideoDivXgetMVdata(MP4_STREAM * _ld);

EXPORT int SsbSipVideoDivXsetMV_interlaced(MP4_STREAM * ld, MP4_STATE * mp4_state, int mb_xpos, int mb_ypos, int field);

#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPMP4_MBLOCK_UTIL_H__
