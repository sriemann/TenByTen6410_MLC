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

/** $Id: SsbSipVideoDivXclearblock.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
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
// SsbSipVideoDivXclearblock.h //

/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
#ifndef ___SSBSIPCLEARBLOCK_H__
#define ___SSBSIPCLEARBLOCK_H__
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
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/
typedef void (SsbSipVideoDivXclearblockProc) (int16_t*);

typedef SsbSipVideoDivXclearblockProc* SsbSipVideoDivXclearblockProcPtr;

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

EXPORT SsbSipVideoDivXclearblockProc SsbSipVideoDivXclearblock_generic;
EXPORT SsbSipVideoDivXclearblockProcPtr SsbSipVideoDivXclearblock;
#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPCLEARBLOCK_H__

