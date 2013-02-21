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

/** $Id: basic_prediction_field.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/


/**
        Copyright (C) 2001 - DivXNetworks
        DivX Advanced Research Center <darc@projectmayo.com>
**/
// basic_prediction_field.h //

/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
#ifndef ___SSBSIPBASIC_PREDICTION_FIELD_H__
#define ___SSBSIPBASIC_PREDICTION_FIELD_H__
/*
// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
*/

#include "SsbSipVideoDivXbasic_prediction.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/


/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlock_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHor_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockVer_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHorVer_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHorRound_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockVerRound_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHorVerRound_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlock_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHor_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockVer_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHorVer_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHorRound_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockVerRound_field;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHorVerRound_field;

EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlock_16x8_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHor_16x8_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockVer_16x8_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHorVer_16x8_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHorRound_16x8_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockVerRound_16x8_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHorVerRound_16x8_generic;

EXPORT SsbSipVideoDivXCopyAreaProc CopyBlock_8x4_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHor_8x4_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockVer_8x4_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHorVer_8x4_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHorRound_8x4_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockVerRound_8x4_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHorVerRound_8x4_generic;
#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPBASIC_PREDICTION_FIELD_H__
