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

/** $Id: SsbSipVideoDivXmp4_dpart.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
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
// SsbSipVideoDivXmp4_dpart.h //

/**
 *
**/
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_DPART__
#define ___SSBSIPMP4_DPART__
#ifdef __cplusplus
extern "C"
{
#endif
/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/

#define DC_MARKER            0x0006b001
#define MOTION_MARKER    0x0001f001
/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
#ifdef ENABLE_DATA_PARTITION        

EXPORT int SsbSipVideoDivXdata_partitioned_i_vop(REFERENCE * ref);
EXPORT int SsbSipVideoDivXdata_partitioned_p_vop(REFERENCE * ref);
#endif

#ifdef __cplusplus
extern "C"
}
#endif

#endif // ___SSBSIPMP4_DPART__

