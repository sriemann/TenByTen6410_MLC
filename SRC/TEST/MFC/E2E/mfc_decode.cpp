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


static unsigned char delimiter_mpeg4[3] = {0x00, 0x00, 0x01};
static unsigned char delimiter_h263[3]  = {0x00, 0x00, 0x80};
static unsigned char delimiter_h264[4]  = {0x00, 0x00, 0x00, 0x01};


#define INPUT_BUFFER_SIZE		(1024 * 256)
#define NUM_PIPS				2


#include "FileRead.h"
#ifndef DATAREAD_TYPE
#error "DATAREAD_TYPE is not defined."
#endif
#if (DATAREAD_TYPE == DATA_FILE)
	#define FRAMEX_IN_TYPE_SEL	(FRAMEX_IN_TYPE_FILE)
#else
	#define FRAMEX_IN_TYPE_SEL	(FRAMEX_IN_TYPE_MEM)
#endif


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


#include "mfc_decode.h"
#if (DECODED_OUTPUT == DECODED_OUT_DISP)
#include "mfc_render.h"

// PROPERTY WINDOW : Video image size, fps information window
#define PROPERTY_WINDOW		0

#define PROPERTY_WINDOW_WIDTH	200
#define PROPERTY_WINDOW_HEIGHT	40

static unsigned short g_property_window[PROPERTY_WINDOW_WIDTH * PROPERTY_WINDOW_HEIGHT];

#include "number_img.h"

extern "C" unsigned char character_img[];
extern "C" unsigned char size_img[];

#endif


/// MFC API related function pointers
typedef void *(*FP_SsbSipDecodeInit)(void);
typedef int   (*FP_SsbSipDecodeExe)(void *, long);
typedef int   (*FP_SsbSipDecodeDeInit)(void *);
typedef void *(*FP_SsbSipDecodeDecodeGetInBuf)(void *, long);
typedef void *(*FP_SsbSipDecodeDecodeGetOutBuf)(void *, long *);
typedef int   (*FP_SsbSipDecodeGetConfig)(void *, unsigned int, void *);
typedef int   (*FP_SsbSipDecodeSetConfig)(void *, unsigned int, void *);

/// Frame Extractor API related function pointers
typedef int  (*FP_NextFrame)(FRAMEX_CTX *, void *, unsigned char [], int, unsigned int *);

typedef struct
{
	FP_SsbSipDecodeInit              fp_SsbSipDecodeInit;
	FP_SsbSipDecodeExe               fp_SsbSipDecodeExe;
	FP_SsbSipDecodeDeInit            fp_SsbSipDecodeDeInit;
	FP_SsbSipDecodeDecodeGetInBuf    fp_SsbSipDecodeDecodeGetInBuf;
	FP_SsbSipDecodeDecodeGetOutBuf   fp_SsbSipDecodeDecodeGetOutBuf;
	FP_SsbSipDecodeGetConfig         fp_SsbSipDecodeGetConfig;
	FP_SsbSipDecodeSetConfig         fp_SsbSipDecodeSetConfig;

	FP_NextFrame                     fp_NextFrame;
} FP_LIST;


static FP_LIST	fp_list[4] =
{
	// fp_list[0] : MPEG4 decode API functions
	{
		SsbSipMPEG4DecodeInit,
		SsbSipMPEG4DecodeExe,
		SsbSipMPEG4DecodeDeInit,
		SsbSipMPEG4DecodeGetInBuf,
		SsbSipMPEG4DecodeGetOutBuf,
		SsbSipMPEG4DecodeGetConfig,
		SsbSipMPEG4DecodeSetConfig,

		NextFrameMpeg4
	},
	// fp_list[1] : H.263 decode API functions
	{
		SsbSipMPEG4DecodeInit,
		SsbSipMPEG4DecodeExe,
		SsbSipMPEG4DecodeDeInit,
		SsbSipMPEG4DecodeGetInBuf,
		SsbSipMPEG4DecodeGetOutBuf,
		SsbSipMPEG4DecodeGetConfig,
		SsbSipMPEG4DecodeSetConfig,

		NULL
	},
	// fp_list[2] : H.264 decode API functions
	{
		SsbSipH264DecodeInit,
		SsbSipH264DecodeExe,
		SsbSipH264DecodeDeInit,
		SsbSipH264DecodeGetInBuf,
		SsbSipH264DecodeGetOutBuf,
		SsbSipH264DecodeGetConfig,
		SsbSipH264DecodeSetConfig,

		NextFrameH264
	},
	// fp_list[3] : VC-1 decode
	{
		SsbSipVC1DecodeInit,
		SsbSipVC1DecodeExe,
		SsbSipVC1DecodeDeInit,
		SsbSipVC1DecodeGetInBuf,
		SsbSipVC1DecodeGetOutBuf,
		SsbSipVC1DecodeGetConfig,
		SsbSipVC1DecodeSetConfig,

		NULL
	}
};


int mfcdec_basic(TUX_MFC_PARA *dec_para, int *force_exit)
{
	int      i=0;	// Loop counter
	int      pip;	// Loop counter
	void    *handle[NUM_PIPS];
	void    *fp[NUM_PIPS];
	int      nLoop;
	DWORD    t1, t2, t_diff;	// tick for performance measurement
	int      perf_disp_period = 100;
	wchar_t  str_performance[8];
	float    fps;
	BOOL 	eof_flag = FALSE;
	unsigned char     *pStrmBuf[NUM_PIPS];
	int                nFrameLeng[NUM_PIPS];
	unsigned char     *pYUVBuf[NUM_PIPS];
	int                nYUVLeng[NUM_PIPS];
	FRAMEX_CTX        *pFrameExCtx[NUM_PIPS];
	unsigned char     *delimiter;
	int                delimiter_leng;
	FP_LIST           *pFP_LIST[NUM_PIPS];		// MFC APIÀÇ function pointers
	SSBSIP_H264_STREAM_INFO stream_info[NUM_PIPS];
	//For output file!
	FILE    *fp_out;
	//For output display!
	void    *hRender_property=NULL;
	void    *hRender[NUM_PIPS];
	int      render_wd=0, render_hi=0;
	SIZE_IMG_IDX   size_img_idx;

	if (dec_para->num_pips > NUM_PIPS) {
		RETAILMSG(1,(L"num_pips must be less than or equal to %d.\n", NUM_PIPS));
		return -1;
	}

	if( !dec_para->outDISP ) {
		fp_out = fopen(dec_para->outfilename, "wb");
		if (fp_out == NULL) {
			RETAILMSG(1,(L"Cannot open the output file.\n"));
			return 0;
		}
	}


RETAILMSG(1,(L"\t ------------- \n"));

	////////////////////////
	// Opening Input File //
	////////////////////////
	for (pip=0; pip < dec_para->num_pips; pip++) {
		//fp[pip] = SSB_FILE_OPEN(pszFileName[pip]);
		fp[pip] = SSB_FILE_OPEN(dec_para->infilename[pip]);
		if (fp[pip] == NULL) {
			RETAILMSG(1,(L"File not found\n"));
			return 0;
		}

		switch (dec_para->codec_mode[pip]) {
		case CODEC_MPEG4:
			delimiter      = delimiter_mpeg4;
			delimiter_leng = sizeof(delimiter_mpeg4);

			pFP_LIST[pip] = &(fp_list[0]);
			break;

		case CODEC_H263:
			delimiter      = NULL;
			delimiter_leng = 0;

			pFP_LIST[pip] = &(fp_list[1]);
			break;

		case CODEC_H264:
			delimiter      = delimiter_h264;
			delimiter_leng = sizeof(delimiter_h264);

			pFP_LIST[pip] = &(fp_list[2]);
			break;

		case CODEC_VC1:
			delimiter      = NULL;
			delimiter_leng = 0;

			pFP_LIST[pip] = &(fp_list[3]);
			break;

		default:
			RETAILMSG(1,(L"[mfcdec_init] Undefined codec mode"));
			return -1;
		}
		///////////////////////////////////
		// FrameExtractor Initialization //
		///////////////////////////////////
		pFrameExCtx[pip] = FrameExtractorInit(FRAMEX_IN_TYPE_SEL, delimiter, delimiter_leng, 1);
		FrameExtractorFirst(pFrameExCtx[pip], fp[pip]);

		//////////////////////////////////////
		///    1. Create new instance      ///
		//////////////////////////////////////
		handle[pip] = pFP_LIST[pip]->fp_SsbSipDecodeInit();
		if (handle[pip] == NULL) {
			RETAILMSG(1,(L"Decoder Init Failed.\n"));
			return 0;
		}

		/////////////////////////////////////////////
		///    2. Obtaining the Input Buffer      ///
		///      (SsbSipH264DecodeGetInBuf)       ///
		/////////////////////////////////////////////
		pStrmBuf[pip] = (unsigned char *) pFP_LIST[pip]->fp_SsbSipDecodeDecodeGetInBuf(handle[pip], 204800);
		if (pStrmBuf[pip] == NULL) {
			RETAILMSG(1,(L"SsbSipDecodeGetInBuf Failed.\n"));
			pFP_LIST[pip]->fp_SsbSipDecodeDeInit(handle[pip]);
			return 0;
		}

		//////////////////////////////
		// CONFIG stream extraction //
		//////////////////////////////
		switch (dec_para->codec_mode[pip]) {
			case CODEC_MPEG4:
				nFrameLeng[pip] = ExtractConfigStreamMpeg4(pFrameExCtx[pip], fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
				break;
			case CODEC_H263:
				nFrameLeng[pip] = ExtractConfigStreamH263(fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
				break;
			case CODEC_H264:
				nFrameLeng[pip] = ExtractConfigStreamH264(pFrameExCtx[pip], fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
				break;
			case CODEC_VC1:
				nFrameLeng[pip] = ExtractConfigStreamVC1(fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
				break;
		}
		if (nFrameLeng[pip] == 0) {
			RETAILMSG(1, (L"\nFailed in extracting the CONFIG stream."));
			return 0;
		}

		////////////////////////////////////////////////////////////////
		///    3. Configuring the instance with the config stream    ///
		///       (SsbSipH264DecodeExe)                             ///
		////////////////////////////////////////////////////////////////
		if (pFP_LIST[pip]->fp_SsbSipDecodeExe(handle[pip], nFrameLeng[pip]) != SSBSIP_H264_DEC_RET_OK) {
			RETAILMSG(1,(L"H.264 Decoder Configuration Failed.\n"));
			return 0;
		}
	
		/////////////////////////////////////
		///   4. Get stream information   ///
		/////////////////////////////////////
		switch (dec_para->codec_mode[pip]) {
			case CODEC_H264:
				pFP_LIST[pip]->fp_SsbSipDecodeGetConfig(handle[pip], H264_DEC_GETCONF_STREAMINFO, &(stream_info[pip]));
				break;
			case CODEC_VC1:
				pFP_LIST[pip]->fp_SsbSipDecodeGetConfig(handle[pip], VC1_DEC_GETCONF_STREAMINFO, &(stream_info[pip]));
				break;
		}
		pFP_LIST[pip]->fp_SsbSipDecodeGetConfig(handle[pip], H264_DEC_GETCONF_STREAMINFO, &(stream_info[pip]));
		RETAILMSG(1,(L"\n\t<STREAMINFO> width=%d   height=%d.", stream_info[pip].width, stream_info[pip].height));

		if( dec_para->outDISP ) {
			// Direct Draw surface creation
			// The surface size is determined by the input video stream
			// Because only one YV12 type surface is supported,
			// 0-th surface is assigned as YV12 surface and the others are asssigned as RGB565.
			if (pip == 0) {
				hRender[pip] = mfc_render_create_overlay(MFC_RENDER_SURFACE_TYPE_YV12,
				                                    0, 0,
				                                    stream_info[pip].width, stream_info[pip].height,
				                                    800, 480);
			}
			else {
				hRender[pip] = mfc_render_create_overlay(MFC_RENDER_SURFACE_TYPE_RGB565,
				                                    0, 0,
				                                    stream_info[pip].width, stream_info[pip].height,
				                                    stream_info[pip].width, stream_info[pip].height);
			}
			if (hRender[pip] == NULL) {
				RETAILMSG(1, (L"\nMFC RENDER overlay surface [%d] creation failed.", pip));
				return -1;
			}
		}
	}

	if( dec_para->outDISP ) {
	#if (PROPERTY_WINDOW == 1)
		hRender_property = mfc_render_create_overlay(MFC_RENDER_SURFACE_TYPE_RGB565,0, 0,
			                                             PROPERTY_WINDOW_WIDTH, PROPERTY_WINDOW_HEIGHT,
			                                             PROPERTY_WINDOW_WIDTH, PROPERTY_WINDOW_HEIGHT);
		if (hRender_property == NULL) {
			RETAILMSG(1, (L"\nMFC RENDER overlay property surface creation failed."));
			return -1;
		}
		
		if (NumImg_MemLoad(character_img) == FALSE) {
			RETAILMSG(1, (L"\nMFC character image file cannot be found."));
			return -1;
		}
		if (SizeImg_MemLoad(size_img) == FALSE) {
			RETAILMSG(1, (L"\nMFC size image file cannot be found."));
			return -1;
		}
		memset(g_property_window, 0xFF, sizeof(g_property_window));
	#endif
		// Performance display interval setting
		if ((stream_info[0].width == 320) && (stream_info[0].height == 240)) {
			size_img_idx = SIZE_IMG_QVGA;
			perf_disp_period = 200;
		}
		else if ((stream_info[0].width == 640) && (stream_info[0].height == 480)) {
			size_img_idx = SIZE_IMG_VGA;
			perf_disp_period = 100;
		}
		else if ((stream_info[0].width == 720) && (stream_info[0].height == 480)) {
			size_img_idx = SIZE_IMG_SD;
			perf_disp_period = 60;
		}
		else if ((stream_info[0].width == 720) && (stream_info[0].height == 576)) {
			size_img_idx = SIZE_IMG_SD;
			perf_disp_period = 60;
		}
		else {
			size_img_idx = SIZE_IMG_UNDEF;
			perf_disp_period = 100;
		}
	}
	else {	//out File
		// Print Inteval of Performance value into colsole  (Regardless of image size, it is set to fixed value.)
		perf_disp_period = 60;
	}

	RETAILMSG(1,(L"\nSTARTING MFC decoding (PIP=%d)\n", dec_para->num_pips));
	t1 = GetTickCount();
	nLoop=0;
	while( !eof_flag ) {
		nLoop++;
		if (*force_exit) {
			RETAILMSG(1,(L"\nFORCE EXIT\n"));
			break;
		}

		for (pip=0; pip < dec_para->num_pips; pip++) {
			//////////////////////////////////
			///       5. DECODE            ///
			///    (SsbSipH264DecodeExe)   ///
			//////////////////////////////////
			if (pFP_LIST[pip]->fp_SsbSipDecodeExe(handle[pip], nFrameLeng[pip]) != SSBSIP_H264_DEC_RET_OK) {
				sprintf(msg, "\n\t Error in decoding %d-th video, %d-th frame\n", pip, nLoop);
				LogMsg();

			RETAILMSG(1,(L"\n-----------------------------\n"));
			RETAILMSG(1,(L"DECODE ERROR\n"));
			RETAILMSG(1,(L"DECODE ERROR\n"));
			RETAILMSG(1,(L"DECODE ERROR\n"));
			RETAILMSG(1,(L"DECODE ERROR\n"));
			RETAILMSG(1,(L"-----------------------------\n"));

				continue;
			}

			//////////////////////////////////////////////
			///    6. Obtaining the Output Buffer      ///
			///      (SsbSipH264DecodeGetOutBuf)       ///
			//////////////////////////////////////////////
			pYUVBuf[pip] = (unsigned char *) pFP_LIST[pip]->fp_SsbSipDecodeDecodeGetOutBuf(handle[pip], (long *) &(nYUVLeng[pip]));

			RETAILMSG(0,(L"\t [%d]  decoded.\n", nLoop));

			///////////////////////
			// Next VIDEO stream //
			///////////////////////
			switch (dec_para->codec_mode[pip]) {
			case CODEC_VC1:
				nFrameLeng[pip] = NextFrameVC1(fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
				break;
			case CODEC_H263:
				nFrameLeng[pip] = NextFrameH263(fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
				break;
			default:
				nFrameLeng[pip] = pFP_LIST[pip]->fp_NextFrame(pFrameExCtx[pip], fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
			}

			if (nFrameLeng[pip] < 4) {
				SSB_FILE_REWIND(fp[pip]);
				RETAILMSG(1,(L"\n### REWIND.\n"));
				switch (dec_para->codec_mode[pip]) {
				case CODEC_VC1:
					nFrameLeng[pip] = ExtractConfigStreamVC1(fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
					break;
				case CODEC_H263:
					nFrameLeng[pip] = ExtractConfigStreamH263(fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
					break;
				default:
					FrameExtractorFirst(pFrameExCtx[pip], fp[pip]);
					nFrameLeng[pip] = pFP_LIST[pip]->fp_NextFrame(pFrameExCtx[pip], fp[pip], pStrmBuf[pip], INPUT_BUFFER_SIZE, NULL);
				}
				eof_flag = TRUE;
			}


			if( !dec_para->outDISP ) {
				fwrite(pYUVBuf[pip], 1, (size_t)nYUVLeng[pip], fp_out);
				//RETAILMSG(1,(L"\n Write File.\n"));
			}
			else {
				mfc_render_do(hRender[pip], pYUVBuf[pip], stream_info[pip].width, stream_info[pip].height, MFC_RENDER_IMAGE_TYPE_YUV420);
				mfc_render_flip(hRender[pip]);
			}
		}

		if (nLoop == perf_disp_period) {
			t2 = GetTickCount();
			t_diff = t2 - t1;
			fps = (float)(1000 * nLoop) / t_diff;
			wsprintf(str_performance, L"%3.1f", fps);
			RETAILMSG(1,(L"Disp. Perf. =%s fps\n", str_performance));
			nLoop = 0;
			t1 = GetTickCount();

			if( dec_para->outDISP ) {
			#if (PROPERTY_WINDOW == 1)	
				SizeImg_Write(size_img_idx, (unsigned char *) g_property_window, PROPERTY_WINDOW_WIDTH, PROPERTY_WINDOW_HEIGHT, 10, 10);
				NumImg_Write_FPS(fps, (unsigned char *) g_property_window, PROPERTY_WINDOW_WIDTH, PROPERTY_WINDOW_HEIGHT, 90, 10);
				mfc_render_do(hRender_property, (unsigned char *) g_property_window, PROPERTY_WINDOW_WIDTH, PROPERTY_WINDOW_HEIGHT, MFC_RENDER_IMAGE_TYPE_RGB565);
				mfc_render_flip(hRender_property);
			#endif
			}
		}

	}

	for (pip=0; pip < dec_para->num_pips; pip++) {

		///////////////////////////////////////
		///    7. SsbSipH264DecodeDeInit    ///
		///////////////////////////////////////
		pFP_LIST[pip]->fp_SsbSipDecodeDeInit(handle[pip]);
	
		SSB_FILE_CLOSE(fp[pip]);

		if( !dec_para->outDISP ) {
			fclose(fp_out);
			RETAILMSG(1,(L"\n Close File.\n"));
		}
		else	{
			mfc_render_delete_overlay(hRender[pip]);
		#if (PROPERTY_WINDOW == 1)
			mfc_render_delete_overlay(hRender_property);
		#endif
		}
	}

	RETAILMSG(1,(L"\n\n@@@ Decode Program ends.\n"));

	return 0;
}

