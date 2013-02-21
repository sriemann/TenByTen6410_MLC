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

/** $Id: SsbSipVideoDivXmp4_mblock_bvop.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
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
// SsbSipVideoDivXmp4_mblock_bvop.h //
#ifndef  ___SSBSIPMP4_MBLOCK_BVOP__  
#define  ___SSBSIPMP4_MBLOCK_BVOP__   
#ifdef __cplusplus
extern "C"
{
#endif


/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

#ifdef __cplusplus
extern "C"
}
#endif

EXPORT int SsbSipVideoDivXmacroblock_b_vop(REFERENCE * ref);
#ifdef OPT_MEM
EXPORT int SsbSipVideoDivXmacroblock_b_vop_finish(REFERENCE *ref,int mba);
#else
EXPORT int SsbSipVideoDivXmacroblock_b_vop_finish(REFERENCE *ref);
#endif

#endif    /*___SSBSIPMP4_MBLOCK_BVOP__*/
