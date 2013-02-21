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

/** $Id: mp4_obmc.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
  Copyright (C) 2001 - DivXNetworks
  DivX Advanced Research Center <darc@projectmayo.com>
**/
// mp4_obmc.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
#ifdef _REDUNDANTCODE_
#ifndef ___SSBSIPMP4_OBMC_H__
#define ___SSBSIPMP4_OBMC_H__
#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

extern void SsbSipVideoDivXrecon_comp_obmc (REFERENCE * ref, unsigned char *src, unsigned char *dst, 
    int stride, int xp, int xy, int mv_index_x, int mv_index_y);
#endif
#ifdef __cplusplus
extern "C"
}


#endif // ___SSBSIPMP4_OBMC_H__
#endif