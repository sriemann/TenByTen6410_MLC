//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#ifndef __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_ENCCODE_H__
#define __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_ENCODE_H__
#include <windows.h>	// Because of HWND
#include "mfc_decode.h"

// Display config
#define ENCODED_OUT_NOTHING		(0)
#define ENCODED_OUT_FILE		(1)
#define ENCODED_OUT_DISP		(2)
#define ENCODED_OUTPUT	(ENCODED_OUT_FILE)

typedef struct
{
	unsigned int width;
	unsigned int height;
	unsigned int frame_rate;
	unsigned int bitrate;
	unsigned int gop_num;
    unsigned int intraqp;
    unsigned int qpmax;
    float gamma;
} ENC_PARAINFO;


#define ANNEX_T_OFF						(0<<0)
#define ANNEX_T_ON						(1<<0)
#define ANNEX_K_OFF						(0<<1)
#define ANNEX_K_ON						(1<<1)
#define ANNEX_J_OFF						(0<<2)
#define ANNEX_J_ON						(1<<2)
#define ANNEX_I_OFF						(0<<3)
#define ANNEX_I_ON						(1<<3)

// Error codes
#define SSBSIP_MFC_ENC_RET_OK						(0)
#define SSBSIP_MFC_ENC_RET_ERR_INVALID_HANDLE		(-1)
#define SSBSIP_MFC_ENC_RET_ERR_INVALID_PARAM		(-2)

#define SSBSIP_MFC_ENC_RET_ERR_CONFIG_FAIL		(-100)
#define SSBSIP_MFC_ENC_RET_ERR_ENCODE_FAIL		(-101)
#define SSBSIP_MFC_ENC_RET_ERR_GETCONF_FAIL		(-102)
#define SSBSIP_MFC_ENC_RET_ERR_SETCONF_FAIL		(-103)



//int mfcenc_basic(ENC_PARAINFO video_para, LPCTSTR pszFileName[], CODEC_MODE codec_mode[], int num_pips, int *force_exit);
int mfcenc_basic(ENC_PARAINFO video_para, TUX_MFC_PARA *enc_para, int *force_exit);

#endif /* __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_ENCODE_H__ */
