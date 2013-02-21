//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#ifndef __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_DECODE_H__
#define __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_DECODE_H__

#include <windows.h>	// Because of HWND

// Display config
#define DECODED_OUT_NOTHING		(0)
#define DECODED_OUT_FILE		(1)
#define DECODED_OUT_DISP		(2)
#define DECODED_OUTPUT	(DECODED_OUT_DISP)

typedef enum {
	CODEC_MPEG4,
	CODEC_H263,
	CODEC_H264,
	CODEC_VC1
} CODEC_MODE;

typedef struct para_TUX_MFC
{
	LPCTSTR *infilename;
	char *outfilename;
	CODEC_MODE *codec_mode;
	unsigned char num_pips;
	BOOL outDISP;
}TUX_MFC_PARA;


int mfcdec_basic(TUX_MFC_PARA *dec_para, int *force_exit);

#endif /* __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_DECODE_H__ */
