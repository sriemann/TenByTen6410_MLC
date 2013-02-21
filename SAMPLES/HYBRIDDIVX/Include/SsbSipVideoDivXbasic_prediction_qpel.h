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

/** $Id: basic_prediction_qpel.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
  Copyright (C) 2001 - DivXNetworks
  DivX Advanced Research Center <darc@projectmayo.com>
**/
// basic_prediction_qpel.h //

/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
#ifndef ___SSBSIPBASIC_PREDICTION_QPEL_H__
#define ___SSBSIPBASIC_PREDICTION_QPEL_H__
#ifdef __cplusplus
extern "C"
{
#endif
/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
#ifdef _REDUNDANTCODE_
void SsbSipVideoDivXqpel_EmptyBlock(unsigned char *Dst, int stridedst);
void SsbSipVideoDivXqpel_EmptyMBlock(unsigned char *Dst, int stridedst);


// block routines
void SsbSipVideoDivXqpel_CopyBlock(unsigned char *src, unsigned char *dst, int stridesrc, int stridedst, int rows);
void SsbSipVideoDivXqpel_CopyBlockHor(unsigned char *Src, unsigned char *Dst, int stridesrc, int stridedst, int rounding_control);
void SsbSipVideoDivXqpel_CopyBlockVer(unsigned char *Src, unsigned char *Dst, int stridesrc, int stridedst, int rounding_control);
void SsbSipVideoDivXqpel_CopyBlockHorVer(unsigned char *Src, unsigned char *Dst, int stridesrc, int stridedst, int rounding_control);


// SsbSipVideoDivXmacroblock routines
void SsbSipVideoDivXqpel_CopyMBlock(unsigned char *src, unsigned char *dst, int stridesrc, int stridedst, int rows);
void SsbSipVideoDivXqpel_CopyMBlockHor(unsigned char *Src, unsigned char *Dst, int stridesrc, int stridedst, int rounding_control);
void SsbSipVideoDivXqpel_CopyMBlockVer(unsigned char *Src, unsigned char *Dst, int stridesrc, int stridedst, int rounding_control);
void SsbSipVideoDivXqpel_CopyMBlockHorVer(unsigned char *Src, unsigned char *Dst, int stridesrc, int stridedst, int rounding_control);

// block and SsbSipVideoDivXmacroblock copy routine using 2 different source blocks and interpolating them

void SsbSipVideoDivXqpel_CopyBlock_2_dest(unsigned char *Src1, unsigned char *Src2, unsigned char *Dst, int stride1, int stride2, int stridedst, int rounding_control, int rows);
void SsbSipVideoDivXqpel_CopyBlock_4_dest(unsigned char *Src1, unsigned char *Src2, unsigned char *Src3, unsigned char *Src4, unsigned char *Dst, int stride1, int stride2, int stride3, int stride4, int stridedst, int rounding_control, int rows);
void SsbSipVideoDivXqpel_CopyMBlock_2_dest(unsigned char *Src1, unsigned char *Src2, unsigned char *Dst, int stride1, int stride2, int stridedst, int rounding_control, int rows);
void SsbSipVideoDivXqpel_CopyMBlock_4_dest(unsigned char *Src1, unsigned char *Src2, unsigned char *Src3, unsigned char *Src4, unsigned char *Dst, int stride1, int stride2, int stride3, int stride4, int stridedst, int rounding_control, int rows);
#endif

#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPBASIC_PREDICTION_QPEL_H__
