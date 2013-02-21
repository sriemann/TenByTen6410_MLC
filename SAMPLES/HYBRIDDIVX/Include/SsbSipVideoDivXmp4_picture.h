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

/** $Id: SsbSipVideoDivXmp4_picture.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
  Copyright (C) 2001 - DivXNetworks
  DivX Advanced Research Center <darc@projectmayo.com>
**/
// SsbSipVideoDivXmp4_picture.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_PICTURE_H__
#define ___SSBSIPMP4_PICTURE_H__

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

#ifdef DERING
void SsbSipVideoDivXMacroblockDisplay_dering(REFERENCE * ref, int x, int y);
#endif
#ifdef DEBLOCK
void SsbSipVideoDivXMacroblockDisplay_deblock(REFERENCE * ref, int x, int y);
#endif
void SsbSipVideoDivXMacroblockDisplay_nopostproc(REFERENCE * ref, int x, int y);
void SsbSipVideoDivXMacroblockDisplay_do_nothing(REFERENCE * ref, int x, int y);
void SsbSipVideoDivXMacroblockDisplay_Init(REFERENCE * ref);
#ifdef DERING
void SsbSipVideoDivXMacroblockDisplayFinish_dering(REFERENCE * ref, unsigned char *bmp, unsigned int stride);
#endif
#ifdef DEBLOCK
void SsbSipVideoDivXMacroblockDisplayFinish_deblock(REFERENCE * ref, unsigned char *bmp, unsigned int stride);
#endif
void SsbSipVideoDivXMacroblockDisplayFinish_nopostproc(REFERENCE * ref, unsigned char *bmp, unsigned int stride);
void SsbSipVideoDivXMacroblockDisplayFinish_entire_frame(REFERENCE * ref, unsigned char *bmp, unsigned int stride);
void SsbSipVideoDivXMacroblockDisplay_Deinit(REFERENCE * ref);
void SsbSipVideoDivXcopymacroblockBmp(REFERENCE * ref, unsigned char** frame, unsigned char * bmp,
               int stride_out, int mb_xpos, int mb_ypos);
void SsbLibVideoDivXMemcpy_16_ASM(unsigned char *,unsigned char *);               
void SsbLibVideoDivXMemcpyLT_8Bytes_TwoBlocksASM(unsigned char *,unsigned char *,unsigned char *,unsigned char *,int);
void SsbLibVideoDivXMemcpy_8_TwoBlocksASM(unsigned char *,unsigned char *,unsigned char *,unsigned char *);
#ifdef __cplusplus
extern "C"
}
#endif

#endif
