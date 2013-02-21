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

/** $Id: SsbSipVideoDivXmp4_recon_qpel.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
  Copyright (C) 2001 - DivXNetworks
  DivX Advanced Research Center <darc@projectmayo.com>
**/
// SsbSipVideoDivXmp4_recon_qpel.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_RECON_QPEL_H__
#define ___SSBSIPMP4_RECON_QPEL_H__
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

#ifdef ENABLE_QPEL
SsbSipVideoDivXReconFunc SsbSipVideoDivXrecon_comp_qpel_16x16;
SsbSipVideoDivXReconBlockFunc SsbSipVideoDivXrecon_comp_qpel_block;
/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/


void SsbSipVideoDivXqpel_x2y0_get(unsigned char *src, unsigned char *dst, int s_stride, int d_stride, int width , int height, int rc);
void SsbSipVideoDivXqpel_x0y2_get(unsigned char *src, unsigned char *dst, int s_stride, int d_stride, int width , int height, int rc);
void SsbSipVideoDivXqpel_x1y0_get(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width , int height, int rc);
void SsbSipVideoDivXqpel_x0y1_get(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width , int height, int rc);

#ifdef _ARM_ASSEMBLY_


void SsbLibVideoDivXQpelx1y0GetRc0ASM(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width , int height);
void SsbLibVideoDivXQpelx1y0GetRc1ASM(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width , int height);
void SsbLibVideoDivXQpelx0y1GetRc0ASM(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width , int height);
void SsbLibVideoDivXQpelx0y1GetRc1ASM(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width , int height);
void SsbLibVideoDivXQpelx0y2GetRc0ASM(unsigned char *src, unsigned char *dst, int s_stride, int d_stride, int width , int height);
void SsbLibVideoDivXQpelx0y2GetRc1ASM(unsigned char *src, unsigned char *dst, int s_stride, int d_stride, int width , int height);
void SsbLibVideoDivXQpelx2y0GetRc0ASM(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width);                                           
void SsbLibVideoDivXQpelx2y0GetRc1ASM(unsigned char *src, unsigned char *dst, unsigned char *s, int s_stride, int d_stride, int width);                                           


#endif
#endif

#endif // ___SSBSIPMP4_RECON_QPEL_H__
