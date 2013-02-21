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

/** $Id: SsbSipVideoDivXmp4_mblock.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
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
// SsbSipVideoDivXmp4_mblock.h //
/*******************************************************************************
                        Samsung India Software Operations Pvt. Ltd. (SISO)
                        Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_MBLOCK_H__
#define ___SSBSIPMP4_MBLOCK_H__

#ifdef __cplusplus
extern "C"
{
#endif


/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/
typedef struct {
  int val, len;
} VLCtabMb;

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

EXPORT int SsbSipVideoDivXmacroblock(REFERENCE * ref);
EXPORT int SsbSipVideoDivXmacroblock_i_vop(REFERENCE * ref);
EXPORT int SsbSipVideoDivXmacroblock_p_vop(REFERENCE * ref);
EXPORT int SsbSipVideoDivXgetgobhdr(REFERENCE * ref);
EXPORT int block(void);

#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPMP4_MBLOCK_H__



