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

/** $Id: mp4_vld_311.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
 * Copyright (C) 2001 - DivXNetworks
 *
 * Eugene Kuznetsov
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
// mp4_vld_311.h //
#ifndef  ___SSBSIPMP4_VLD_311_H__  
#define  ___SSBSIPMP4_VLD_311_H__   

/*
// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
*/
#include "SsbSipVideoDivXdecore.h"
#include "SsbSipVideoDivXmp4_header.h"
#include "SsbSipVideoDivXmp4_vld.h"
#include "SsbSipVideoDivXtypes_311.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/
EXPORT event_t SsbSipVideoDivXvld_intra_dct_311_0_lum(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_intra_dct_311_10_lum(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_intra_dct_311_11_lum(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_intra_dct_311_0_chrom(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_intra_dct_311_10_chrom(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_intra_dct_311_11_chrom(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_inter_dct_311_0(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_inter_dct_311_10(MP4_STREAM* ld);
EXPORT event_t SsbSipVideoDivXvld_inter_dct_311_11(MP4_STREAM* ld);

EXPORT void SsbSipVideoDivXgetMVdata_311_0(MP4_STREAM* ld, int* mv_x, int* mv_y);
EXPORT void SsbSipVideoDivXgetMVdata_311_1(MP4_STREAM* ld, int* mv_x, int* mv_y);

EXPORT short SsbSipVideoDivXgetDC_311_0_lum(MP4_STREAM* ld);
EXPORT short SsbSipVideoDivXgetDC_311_1_lum(MP4_STREAM* ld);
EXPORT short SsbSipVideoDivXgetDC_311_0_chrom(MP4_STREAM* ld);
EXPORT short SsbSipVideoDivXgetDC_311_1_chrom(MP4_STREAM* ld);

EXPORT const int SsbSipVideoDivXcbp_intra_indices[];
EXPORT const int SsbSipVideoDivXcbp_inter_indices[];

EXPORT const int SsbSipVideoDivXintra_0_indices[];
EXPORT const item_t SsbSipVideoDivXintra_0_items[];
EXPORT const int SsbSipVideoDivXintra_10_indices[];
EXPORT const item_t SsbSipVideoDivXintra_10_items[];
EXPORT const int SsbSipVideoDivXintra_11_indices[];
EXPORT const item_t SsbSipVideoDivXintra_11_items[];

EXPORT const int SsbSipVideoDivXinter_0_indices[];
EXPORT const item_t SsbSipVideoDivXinter_0_items[];
EXPORT const int SsbSipVideoDivXinter_10_indices[];
EXPORT const item_t SsbSipVideoDivXinter_10_items[];
EXPORT const int SsbSipVideoDivXinter_11_indices[];
EXPORT const item_t SsbSipVideoDivXinter_11_items[];


EXPORT const mv_item_t SsbSipVideoDivXmv_tree0_items[];
EXPORT const int SsbSipVideoDivXmv_tree0_indices[];
EXPORT const mv_item_t SsbSipVideoDivXmv_tree1_items[];
EXPORT const int SsbSipVideoDivXmv_tree1_indices[];


EXPORT const int SsbSipVideoDivXdc_chrom0_indices[];
EXPORT const int SsbSipVideoDivXdc_chrom1_indices[];
EXPORT const int SsbSipVideoDivXdc_lum0_indices[];
EXPORT const int SsbSipVideoDivXdc_lum1_indices[];



STATIC __inline short StaDivXGetCBP311_I(MP4_STREAM* ld)
{
    short cbp;
    if(StaDivXGetBits1(ld))
    cbp=0;
    else
    cbp=StaDivXGetShort311(ld, SsbSipVideoDivXcbp_intra_indices);
    return cbp;
}

STATIC __inline short StaDivXGetCBP311_P(MP4_STREAM* ld)
{
    short cbp;
    cbp=StaDivXGetShort311(ld, SsbSipVideoDivXcbp_inter_indices);
    return cbp;
}

#ifdef __cplusplus
extern "C"
}
#endif

#endif
