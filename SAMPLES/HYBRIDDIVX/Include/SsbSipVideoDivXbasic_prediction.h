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

/** $Id: SsbSipVideoDivXbasic_prediction.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
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
// SsbSipVideoDivXbasic_prediction.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPBASIC_PREDICTION_H__
#define ___SSBSIPBASIC_PREDICTION_H__
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
typedef void (SsbSipVideoDivXCopyAreaProc)(unsigned char * Src, unsigned char * Dst, int Stride, int StrideDst);
typedef SsbSipVideoDivXCopyAreaProc* SsbSipVideoDivXCopyAreaProcPtr;



/**/
/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlock;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHor;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockVer;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHorVer;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHorRound;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockVerRound;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyBlockHorVerRound;

EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlock;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlock_nt;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHor;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockVer;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHorVer;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHorRound;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockVerRound;
EXPORT SsbSipVideoDivXCopyAreaProcPtr CopyMBlockHorVerRound;

/**/

EXPORT SsbSipVideoDivXCopyAreaProc CopyBlock_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHor_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockVer_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHorVer_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHorRound_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockVerRound_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyBlockHorVerRound_generic;
#ifdef _ARM_ASSEMBLY_
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyBlockUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyBlockHorUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyBlockVerUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyBlockHorVerASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyBlockHorRndUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyBlockVerRndUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyBlockHorVerRndASM;

EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyMBlockUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyMBlockHorUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyMBlockVerUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyMBlockHorRndUnalignedASM;
EXPORT SsbSipVideoDivXCopyAreaProc SsbLibVideoDivXBasicPredCopyMBlockVerRndUnalignedASM;

#endif
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlock_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHor_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockVer_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHorVer_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHorRound_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockVerRound_generic;
EXPORT SsbSipVideoDivXCopyAreaProc CopyMBlockHorVerRound_generic;
/**/

#ifdef __cplusplus
}
#endif
#endif // ___SSBSIPBASIC_PREDICTION_H__
