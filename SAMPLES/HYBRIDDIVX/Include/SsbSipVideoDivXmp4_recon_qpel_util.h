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

/** $Id: mp4_recon_qpel_util.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
  Copyright (C) 2001 - DivXNetworks
  DivX Advanced Research Center <darc@projectmayo.com>
**/
// mp4_recon_qpel_util.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
#ifdef _REDUNDANTCODE_
#ifndef ___SSBSIPMP4_RECON_QPEL_UTIL_H__
#define ___SSBSIPMP4_RECON_QPEL_UTIL_H__
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
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

EXPORT void recon_qpel_prepare(const unsigned char *src, unsigned char *dst, int src_stride, int dst_stride, int block_dim);
EXPORT void recon_qpel_halfsamplevalues(const unsigned char *dst, int rounding_control, int block_dim);

//
// optimized routines
//

EXPORT void construct_mirror_matrix(const unsigned char *matrix_source, unsigned char *mirrored_matrix, int width, int height, int src_stride);
EXPORT void construct_hor_matrix(const unsigned char *src_mirrored, unsigned char *dst, int width, int height, int rounding_control);
EXPORT void construct_ver_matrix(const unsigned char *src_mirrored, unsigned char *dst, int width, int height, int rounding_control);
EXPORT void construct_horver_matrix(const unsigned char *src_mirrored, unsigned char *dst, int width, int height, int rounding_control);
EXPORT void construct_e_matrix(const unsigned char *src_mirrored, const unsigned char *hor_matrix, unsigned char *dst, int width, int height, int rouding_control);
EXPORT void construct_k_matrix(const unsigned char *src, unsigned char *dst, int width, int height, int rounding_control);

typedef void (qpel_filtering_8tap_hor_proc)(const unsigned char *src, int rounding_control, unsigned char *dst);
typedef void (qpel_filtering_8tap_ver_proc)(const unsigned char *src, int rounding_control, int src_stride, unsigned char *dst);

typedef qpel_filtering_8tap_hor_proc* qpel_filtering_8tap_hor_procptr;
typedef qpel_filtering_8tap_ver_proc* qpel_filtering_8tap_ver_procptr;

EXPORT qpel_filtering_8tap_hor_procptr qpel_filtering_8tap_hor;
EXPORT qpel_filtering_8tap_ver_procptr qpel_filtering_8tap_ver;

EXPORT qpel_filtering_8tap_hor_proc qpel_filtering_8tap_hor_generic;
EXPORT qpel_filtering_8tap_ver_proc qpel_filtering_8tap_ver_generic;


#ifdef WIN32
EXPORT qpel_filtering_8tap_hor_proc qpel_filtering_8tap_hor_mmx;
EXPORT qpel_filtering_8tap_ver_proc qpel_filtering_8tap_ver_mmx;
#endif
#endif
#ifdef __cplusplus
extern "C"
}
#endif
#endif // ___SSBSIPMP4_RECON_QPEL_UTIL_H__
