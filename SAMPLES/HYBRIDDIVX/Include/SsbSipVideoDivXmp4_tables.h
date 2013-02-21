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

/** $Id: SsbSipVideoDivXmp4_tables.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_TABLES_H___
#define ___SSBSIPMP4_TABLES_H___
#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
EXPORT    const unsigned int SsbSipVideoDivXzig_zag_scan[64];
EXPORT    const     unsigned int SsbSipVideoDivXalternate_horizontal_scan[64];
EXPORT    const     unsigned int SsbSipVideoDivXalternate_vertical_scan[64];
EXPORT    const     unsigned int SsbSipVideoDivXintra_quant_matrix[64];
EXPORT    const     unsigned int SsbSipVideoDivXnonintra_quant_matrix[64];
EXPORT    const     int SsbSipVideoroundtab[16];
EXPORT    const      int SsbSipVideoMCBPCtabIntra[32];
EXPORT    const  int  SsbSipVideoMCBPCtabInter[256];
EXPORT    const      int  SsbSipVideoCBPYtab[48];
EXPORT    const     int SsbSipVideoMVtab0[14];
EXPORT    const     int SsbSipVideoMVtab1[96];
EXPORT    const     int SsbSipVideoMVtab2[124];

EXPORT    const     unsigned int  SsbSipVideotableB16_1[112];
EXPORT    const     unsigned int  SsbSipVideotableB16_2[96];
EXPORT    const     unsigned int  SsbSipVideotableB16_3[120];

EXPORT    const     unsigned int SsbSipVideotableB17_1[112];
EXPORT    const     unsigned int SsbSipVideotableB17_2[96];
EXPORT    const     unsigned int SsbSipVideotableB17_3[120];



#ifdef __cplusplus
extern "C"
}
#endif

#endif
