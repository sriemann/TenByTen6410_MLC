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

/** $Id: idct.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Andrea Graziani (Ag)
 * James Leiterman
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// idct.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPIDCT_H__
#define ___SSBSIPIDCT_H__

#ifdef __cplusplus
EXPORT "C" {
#endif // __cplusplus


#ifndef POWERPC
/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/
    
typedef void (SsbSipVideoDivXidctIntraProc)(short *block, char *dest, int stride);
typedef void (SsbSipVideoDivXidctInterProc)(short *block);
typedef SsbSipVideoDivXidctIntraProc* SsbSipVideoDivXidctIntraProcPtr;
typedef SsbSipVideoDivXidctInterProc* SsbSipVideoDivXidctInterProcPtr;

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

EXPORT SsbSipVideoDivXidctIntraProc idct_intra_generic;
//EXPORT SsbSipVideoDivXidctInterProc idct_inter_generic;
EXPORT SsbSipVideoDivXidctInterProc idct_inter_generic_311;
EXPORT SsbSipVideoDivXidctIntraProcPtr idct_intra;
EXPORT SsbSipVideoDivXidctInterProcPtr idct_inter;
EXPORT SsbSipVideoDivXidctInterProcPtr idct_inter_311;



#else
/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/

typedef void (*SsbSipVideoDivXidctProc)(short *block); // function pointer prototypes
/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
EXPORT SsbSipVideoDivXidctProc idct;

#endif // !POWERPC

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // ___SSBSIPIDCT_H__
