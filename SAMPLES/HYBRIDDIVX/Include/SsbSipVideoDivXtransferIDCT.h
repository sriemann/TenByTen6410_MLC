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

/** $Id: SsbSipVideoDivXtransferIDCT.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * John Funnell
 * James Leiterman
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// SsbSipVideoDivXtransferIDCT.h //

/* Functions to add or copy the result of the iDCT into the output       */
/* frame buffer.  The "clear block" function could be absorbed into this */
/* loop (set *ouputPtr = 0 after the copy/add).              */

/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPTRANSFERIDCT_H__
#define ___SSBSIPTRANSFERIDCT_H__

/*
// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
*/
#include "SsbSipVideoDivXportab.h"

#ifdef __cplusplus
EXPORT "C" {
#endif // __cplusplus


#ifndef POWERPC

typedef void (transferIDCTProc)(int16_t *sourceS16, uint8_t *destU8, int stride);
typedef transferIDCTProc* transferIDCTProcPtr;

EXPORT transferIDCTProcPtr transferIDCT_add;
EXPORT transferIDCTProcPtr transferIDCT_copy;

EXPORT transferIDCTProc transferIDCT_add_generic;
EXPORT transferIDCTProc transferIDCT_copy_generic;

#else

typedef void (*transferIDCTProc)(int16_t *sourceS16, uint8_t *destU8, int stride);

EXPORT transferIDCTProc transferIDCT_add;
EXPORT transferIDCTProc transferIDCT_copy;

#endif // !POWERPC



#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ___SSBSIPTRANSFERIDCT_H__

