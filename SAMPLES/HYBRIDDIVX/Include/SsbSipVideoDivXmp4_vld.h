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

/** $Id: SsbSipVideoDivXmp4_vld.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * John Funnell
 * Andrea Graziani
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// SsbSipVideoDivXmp4_vld.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_VLD_H__
#define ___SSBSIPMP4_VLD_H__
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
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/
#define ESCAPE 7167
#define ESCAPE_INTRAB16 47
#define ESCAPE_INTERB17 191
/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

EXPORT    const     unsigned int  SsbSipVideotableB16_1[112];
EXPORT    const     unsigned int  SsbSipVideotableB16_2[96];
EXPORT    const     unsigned int  SsbSipVideotableB16_3[120];

EXPORT    const     unsigned int SsbSipVideotableB17_1[112];
EXPORT    const     unsigned int SsbSipVideotableB17_2[96];
EXPORT    const     unsigned int SsbSipVideotableB17_3[120];





STATIC __inline StaDivXVldTableB19(int last, int run);
STATIC __inline StaDivXVldTableB20(int last, int run);
STATIC __inline StaDivXVldTableB21(int last, int level);
STATIC __inline StaDivXVldTableB22(int last, int level);

EXPORT int vld_intra_dct(REFERENCE * ref,unsigned int *zigzag,int *i,short *m)  ;
EXPORT int vld_inter_dct(REFERENCE * ref,unsigned int *zigzag,int *i,int *m);
EXPORT int vld_shv_dct(REFERENCE * ref,unsigned int *zigzag,int *i,int *m);
EXPORT int vld_inter_dct_quanttypefirst(REFERENCE * ref,unsigned int *zigzag,int *i,int *m);
EXPORT int vld_intra_dct_QuanttypeFirst(REFERENCE * ref,unsigned int *zigzag,int *i,short *m) ;
/***/
#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPMP4_VLD_H__
