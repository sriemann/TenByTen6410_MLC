//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// TestCode.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <stdio.h>


#include "SsbSipMpeg4Decode.h"
#include "SsbSipMpeg4Encode.h"
#include "SsbSipH264Decode.h"
#include "SsbSipH264Encode.h"
#include "SsbSipVC1Decode.h"
#include "SsbSipMfcDecode.h"
#include "FrameExtractor.h"
#include "MPEG4Frames.h"
#include "H263Frames.h"
#include "H264Frames.h"
#include "VC1Frames.h"
#include "FileRead.h"
#include "mfc_encode.h"

#if (ENCODED_OUTPUT == ENCODED_OUT_DISP)
#include "mfc_render.h"
#include "number_img.h"
// PROPERTY WINDOW : Video image size, fps information window
#define PROPERTY_WINDOW		0
#define PROPERTY_WINDOW_WIDTH	200
#define PROPERTY_WINDOW_HEIGHT	40
static unsigned short g_property_window[PROPERTY_WINDOW_WIDTH * PROPERTY_WINDOW_HEIGHT];
extern "C" unsigned char character_img[];
extern "C" unsigned char size_img[];
#endif

//#define INPUT_BUFFER_SIZE		(1024 * 256)
//#define NUM_PIPS				1
#define NO_OUTPUT	1

#ifndef DATAREAD_TYPE
#error "DATAREAD_TYPE is not defined."
#endif
#if (DATAREAD_TYPE == DATA_FILE)
	#define FRAMEX_IN_TYPE_SEL	(FRAMEX_IN_TYPE_FILE)
#else
	#define FRAMEX_IN_TYPE_SEL	(FRAMEX_IN_TYPE_MEM)
#endif

static unsigned char delimiter_mpeg4[3] = {0x00, 0x00, 0x01};
static unsigned char delimiter_h263[3]  = {0x00, 0x00, 0x80};
static unsigned char delimiter_h264[4]  = {0x00, 0x00, 0x00, 0x01};

static char msg[256];
static void LogMsg()
{
	FILE *fp;

	fp = fopen("\\NandFlash\\logmsg.txt", "at");
	if (fp == NULL)
		return;
	fprintf(fp, msg);
	fclose(fp);
}


/// MFC API related function pointers
typedef void *(*EFP_SsbSipEncodeInit)(unsigned int, unsigned int,unsigned int,
                            unsigned int, unsigned int, unsigned int, unsigned int,unsigned int, float);
typedef int   (*EFP_SsbSipEncodeExe)(void *);
typedef int   (*EFP_SsbSipEncodeDeInit)(void *);
typedef void *(*EFP_SsbSipEncodeEncodeGetInBuf)(void *, long);
typedef void *(*EFP_SsbSipEncodeEncodeGetOutBuf)(void *, long *);
typedef int   (*EFP_SsbSipEncodeGetConfig)(void *, unsigned int, void *);
typedef int   (*EFP_SsbSipEncodeSetConfig)(void *, unsigned int, void *);

/// Frame Extractor API related function pointers
typedef int  (*EFP_NextFrame)(FRAMEX_CTX *, void *, unsigned char [], int, unsigned int *);

typedef struct
{
	EFP_SsbSipEncodeInit              efp_SsbSipEncodeInit;
	EFP_SsbSipEncodeExe               efp_SsbSipEncodeExe;
	EFP_SsbSipEncodeDeInit            efp_SsbSipEncodeDeInit;
	EFP_SsbSipEncodeEncodeGetInBuf    efp_SsbSipEncodeEncodeGetInBuf;
	EFP_SsbSipEncodeEncodeGetOutBuf   efp_SsbSipEncodeEncodeGetOutBuf;
	//EFP_SsbSipEncodeGetConfig         efp_SsbSipEncodeGetConfig;
	EFP_SsbSipEncodeSetConfig         efp_SsbSipEncodeSetConfig;
} EFP_LIST;


static EFP_LIST	efp_list[3] =
{
	// fp_list[0] : MPEG4 decode API functions
	{
		SsbSipMPEG4EncodeInit,
		SsbSipMPEG4EncodeExe,
		SsbSipMPEG4EncodeDeInit,
		SsbSipMPEG4EncodeGetInBuf,
		SsbSipMPEG4EncodeGetOutBuf,
		//SsbSipMPEG4EncodeGetConfig,
		SsbSipMPEG4EncodeSetConfig,
	},
	// fp_list[1] : H.263 decode API functions
	{
		SsbSipMPEG4EncodeInit,
		SsbSipMPEG4EncodeExe,
		SsbSipMPEG4EncodeDeInit,
		SsbSipMPEG4EncodeGetInBuf,
		SsbSipMPEG4EncodeGetOutBuf,
		//SsbSipMPEG4EncodeGetConfig,
		SsbSipMPEG4EncodeSetConfig,
	},
	// fp_list[2] : H.264 decode API functions
	{
		SsbSipH264EncodeInit,
		SsbSipH264EncodeExe,
		SsbSipH264EncodeDeInit,
		SsbSipH264EncodeGetInBuf,
		SsbSipH264EncodeGetOutBuf,
		//SsbSipH264EncodeGetConfig,
		SsbSipH264EncodeSetConfig,
	}
};


int mfcenc_basic(ENC_PARAINFO video_para, TUX_MFC_PARA *enc_para, int *force_exit)
{
	int      i=0;	// Loop counter
	void    *handle;
	void    *fp;
	int      nLoop;
	int 	ret_val = 0;
	DWORD    t1;	// tick for performance measurement
	unsigned char     *pStrmBuf;
	unsigned char *p_outbuf;
	unsigned int   num_slices, h263_annex;
	long size;
	unsigned int read_cnt;
	int ret;
	EFP_LIST           *pEFP_LIST;		// MFC APIÀÇ function pointers
	unsigned int   change_param[2];
#if ( (!NO_OUTPUT) && (ENCODED_OUTPUT == ENCODED_OUT_FILE) )
	FILE    *fp_out;
#elif ((!NO_OUTPUT) && (ENCODED_OUTPUT == ENCODED_OUT_DISP))
	void    *hRender_property=NULL;
	void    *hRender;
	int      render_wd=0, render_hi=0;
	SIZE_IMG_IDX   size_img_idx;
#endif


//RETAILMSG(1,(L"\t ------------- \n"));

	fp = SSB_FILE_OPEN(enc_para->infilename[0]);
	if (fp == NULL) {
		RETAILMSG(1,(L"File not found\n"));
		return -1;
	}		
#if ( (!NO_OUTPUT) && (ENCODED_OUTPUT == ENCODED_OUT_FILE) )
	fp_out = fopen(enc_para->outfilename, "wb");
	if (fp_out == NULL) {
		RETAILMSG(1,(L"Cannot open the output file.\n"));
		SSB_FILE_CLOSE(fp);
		return -1;
	}
#endif
	switch (enc_para->codec_mode[0]) {
		case CODEC_MPEG4:
			pEFP_LIST = &(efp_list[0]);
			handle = pEFP_LIST->efp_SsbSipEncodeInit(SSBSIPMFCENC_MPEG4, video_para.width, video_para.height,
													video_para.frame_rate, video_para.bitrate, video_para.gop_num,
													video_para.intraqp, video_para.qpmax, video_para.gamma);
		break;
		case CODEC_H263:
			pEFP_LIST = &(efp_list[1]);
			handle = pEFP_LIST->efp_SsbSipEncodeInit(SSBSIPMFCENC_H263,video_para.width,video_para.height,video_para.frame_rate,video_para.bitrate,video_para.gop_num,video_para.intraqp,video_para.qpmax,video_para.gamma); 
		break;
		case CODEC_H264:		
			//RETAILMSG(1,(L"[mfcdec_init] Our codec mode is H.264\n"));
			pEFP_LIST = &(efp_list[2]);
			handle = pEFP_LIST->efp_SsbSipEncodeInit(video_para.width,video_para.height,video_para.frame_rate,video_para.bitrate,video_para.gop_num,video_para.intraqp,video_para.qpmax,0,video_para.gamma); 
		break;
		default:
			//RETAILMSG(1,(L"[mfcdec_init] Undefined codec mode"));
			SSB_FILE_CLOSE(fp);
#if ( (!NO_OUTPUT) && (ENCODED_OUTPUT == ENCODED_OUT_FILE) )
			fclose(fp_out);
#endif
			return -1;
	}

		//////////////////////////////////////
		///    1. Create new instance      ///
		//////////////////////////////////////
		
	if (handle == NULL) {
		RETAILMSG(1,(L"Encoder Init Failed.\n"));
		ret_val = -1;
		goto LEAVEREAD;
	}
		////////////////////////////////////////////////////////////////////
		///    2. Set some configuration parameter for initialization    ///
		///       (SsbSipH264EncodeExe)                                  ///
		////////////////////////////////////////////////////////////////////
	num_slices = 4;
	if ( enc_para->codec_mode[0] == CODEC_H263 ) {
		h263_annex = ANNEX_K_ON;
		SsbSipMPEG4EncodeSetConfig(handle, MPEG4_ENC_SETCONF_H263_ANNEX,      &h263_annex);
	}
		
		////////////////////////////////////////////////////////////////
		///    3. Configuring the instance with the config stream    ///
		///       (SsbSipH264DecodeExe)                             ///
		////////////////////////////////////////////////////////////////

	if (pEFP_LIST->efp_SsbSipEncodeExe(handle) != SSBSIP_MFC_ENC_RET_OK) {
		RETAILMSG(1,(L"H.264 Encoder Configuration Failed.\n"));
		ret_val = -1;
		goto LEAVEREAD;
	}
		//RETAILMSG(1,(L"\t Before step4 \n"));
		/////////////////////////////////////////////
		///    4. Obtaining the Input Buffer      ///
		///      (SsbSipH264DecodeGetInBuf)       ///
		/////////////////////////////////////////////
	pStrmBuf = (unsigned char *) pEFP_LIST->efp_SsbSipEncodeEncodeGetInBuf(handle, 0);
	if (pStrmBuf == NULL) {
		RETAILMSG(1,(L"SsbSipMFCEncodeGetInBuf Failed.\n"));
		ret_val = -1;
		goto LEAVEREAD;
	}	
	
	t1 = GetTickCount();
	for (nLoop=0; nLoop < 650 ; nLoop++) {
		ret = SSB_FILE_READ(fp,pStrmBuf,(video_para.width * video_para.height * 3) >> 1,&read_cnt);
		//RETAILMSG(1,(L"\nRet result [%d]\n", ret));
		if ( ret == SSB_FILE_EOF ) {
			//RETAILMSG(1,(L"EOF\n"));
			break;
		}
		if ( read_cnt != ((video_para.width * video_para.height * 3) >> 1) ) {
			break;
		}
			
		if ( (i == 27 ) && (enc_para->codec_mode[0] != CODEC_H264)) {
			change_param[0] = MPEG4_ENC_PARAM_GOP_NUM;
			change_param[1] = 20;
			SsbSipMPEG4EncodeSetConfig(handle, MPEG4_ENC_SETCONF_PARAM_CHANGE, &change_param);
			change_param[0] = MPEG4_ENC_PARAM_F_RATE;
			change_param[1] = (2000 << 16) | 40000;
			SsbSipMPEG4EncodeSetConfig(handle, MPEG4_ENC_SETCONF_PARAM_CHANGE, &change_param);
		}
		
		ret = pEFP_LIST->efp_SsbSipEncodeExe(handle);

		p_outbuf = (unsigned char*)pEFP_LIST->efp_SsbSipEncodeEncodeGetOutBuf(handle, &size);
		//RETAILMSG(1,(L"\n Output Buf = 0x%X,  size = %d\n", (DWORD) p_outbuf, size));
#if ( (!NO_OUTPUT) && (ENCODED_OUTPUT == ENCODED_OUT_FILE) )
		fwrite(p_outbuf, 1, size, fp_out);
#endif 
	}

	//RETAILMSG(1,(L"\n\n@@@ Program ends.\n"));
LEAVEREAD:	
	SSB_FILE_CLOSE(fp);
#if ( (!NO_OUTPUT) && (ENCODED_OUTPUT == ENCODED_OUT_FILE) )			
	fclose(fp_out);
#endif	
	if (handle != NULL) 
		pEFP_LIST->efp_SsbSipEncodeDeInit(handle);

	return ret_val;
}

