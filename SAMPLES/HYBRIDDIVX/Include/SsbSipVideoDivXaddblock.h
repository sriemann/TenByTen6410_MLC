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

/** $Id: SsbSipVideoDivXaddblock.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// SsbSipVideoDivXaddblock.h //

/**
 *
**/
/*******************************************************************************
                        Samsung India Software Operations Pvt. Ltd. (SISO)
                            Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPADDBLOCK_H__
#define ___SSBSIPADDBLOCK_H__
#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
EXPORT void SsbSipVideoDivXaddblockIntra (REFERENCE * ref, short bp[64], int comp, int bx, int by);
EXPORT void SsbSipVideoDivXaddblockInter (REFERENCE * ref, short bp[64], int comp, int bx, int by);


#ifdef __cplusplus
extern "C"
}
#endif
#endif //___SSBSIPADDBLOCK_H__

