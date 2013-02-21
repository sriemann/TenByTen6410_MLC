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

/** $Id: SsbSipVideoDivXmp4_predict.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
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
// SsbSipVideoDivXmp4_predict.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_PREDICT_H__
#define ___SSBSIPMP4_PREDICT_H__

#ifdef __cplusplus
extern "C"
{
#endif
/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/
#define TOP 1
#define LEFT 0


/*** aritmetic operators ***/


/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

EXPORT void SsbSipVideoDivXset_prediction_direction(MP4_STATE * mp4_state, int block_num);
EXPORT void SsbSipVideoDivXset_prediction_direction_intradc(MP4_STATE * mp4_state, int block_num, short* dc_value);

EXPORT void SsbSipVideoDivXdc_recon(MP4_STATE * mp4_state, int block_num, short * dc_value);
EXPORT int SsbSipVideoDivXac_recon(MP4_STATE * mp4_state, int block_num, short * psBlock,short * psBlock1);
EXPORT int SsbSipVideoDivXac_rescaling(MP4_STATE * mp4_state, int block_num, short * psBlock,short * psBlock1);
EXPORT void SsbSipVideoDivXac_store(MP4_STATE * mp4_state, int block_num, short * psBlock);

EXPORT void SsbSipVideoDivXrescue_predict(MP4_STATE * _mp4_state);
#ifdef __cplusplus
extern "C"
}
#endif
#endif // ___SSBSIPMP4_PREDICT_H__
