//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#include <windows.h>
#include <tchar.h>
#include <stressutils.h>
#include "MfcDrvParams.h"
#include "mfc_decode.h"
#include "mfc_encode.h"
#include "mfc_render.h"


#define MEM_LEAK_DETC	1


//Properites of tryit.yuv file
#define TRYIT_YUV_W		320
#define TRYIT_YUV_H		240
#define TRYIT_YUV_FR	15
#define TRYIT_YUV_BR	1000
#define TRYIT_YUV_GOP	15
#define TRYIT_YUV_IQ	10
#define TRYIT_YUV_QP	10
#define TRYIT_YUV_GA	0.75
#define NUM_PIP_VIDEOS		1
#define T_RUNNING	1
#define T_REST	0
#define TIMEOUT_CONSTANT	10000

typedef struct
{
   HANDLE hWaitSimCodec;
   bool bTestResult;
}CMD_DATA,*PCMD_DATA;


DWORD  ThreadProc_Enc263(LPVOID lpParameter);
DWORD  ThreadProc_EncMPEG4(LPVOID lpParameter);
DWORD  ThreadProc_Enc264(LPVOID lpParameter);
DWORD  ThreadProc_Dec263(LPVOID lpParameter);
DWORD  ThreadProc_DecMPEG4(LPVOID lpParameter);
DWORD  ThreadProc_Dec264(LPVOID lpParameter);
DWORD  ThreadProc_DecVC1(LPVOID lpParameter);
//Thread Function List 
DWORD  (*TFL_MFC[7])(LPVOID lpParameter) = {ThreadProc_Enc263,
											ThreadProc_EncMPEG4,
											ThreadProc_Enc264,
											ThreadProc_Dec263,
											ThreadProc_DecMPEG4,
											ThreadProc_Dec264,
											ThreadProc_DecVC1};


HANDLE g_hInst = NULL;
unsigned char running_codec[] = {T_REST,T_REST,T_REST,T_REST,T_REST,T_REST,T_REST};

// Thread to do Encode H.263.
DWORD  ThreadProc_Enc263(LPVOID lpParameter)
{    
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit1.yuv"};
	CODEC_MODE codec_modes[] = {CODEC_H263};
	PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	int forceExit = FALSE;
	ENC_PARAINFO video_para;
	TUX_MFC_PARA *t601par;

	// Encode/Decode setting configuration
	LogComment(_T("This thread will encode H.263 video."));
	t601par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t601par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		pCmdData->bTestResult = FALSE;
		return 0;
	}
	t601par->infilename = pszFileNames;
	t601par->outfilename = NULL;
	t601par->codec_mode = codec_modes;
	t601par->outDISP = FALSE;
	
	// Fill-up properites of target video
    video_para.width =  TRYIT_YUV_W;    
	video_para.height = TRYIT_YUV_H;
	video_para.frame_rate = TRYIT_YUV_FR;
	video_para.bitrate = TRYIT_YUV_BR;
	video_para.gop_num = TRYIT_YUV_GOP;
	video_para.intraqp = TRYIT_YUV_IQ;
	video_para.qpmax = TRYIT_YUV_QP;
	video_para.gamma = TRYIT_YUV_GA;
	
	// Execute and get result
	if (mfcenc_basic(video_para,t601par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Encode H263 thread faild\n"));
	    pCmdData->bTestResult = FALSE;
	}
	else {
		LogComment(_T("MFC Encode H263 thread finished\n"));
	    pCmdData->bTestResult = TRUE;
	}
	SetEvent(pCmdData->hWaitSimCodec);
	free(t601par);

	return 0;
}
// Thread to do Encode MPEG-4.
DWORD  ThreadProc_EncMPEG4(LPVOID lpParameter)
{    
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit2.yuv"};
	CODEC_MODE codec_modes[] = {CODEC_MPEG4};
	PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	int forceExit = FALSE;
	ENC_PARAINFO video_para;
	TUX_MFC_PARA *t601par;
	
	LogComment(_T("This thread will encode MPEG4 video."));
	t601par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t601par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		pCmdData->bTestResult = FALSE;
		return 0;
	}
	t601par->infilename = pszFileNames;
	t601par->outfilename = NULL;
	t601par->codec_mode = codec_modes;
	t601par->outDISP = FALSE;
	
    video_para.width =  TRYIT_YUV_W;    
	video_para.height = TRYIT_YUV_H;
	video_para.frame_rate = TRYIT_YUV_FR;
	video_para.bitrate = TRYIT_YUV_BR;
	video_para.gop_num = TRYIT_YUV_GOP;
	video_para.intraqp = TRYIT_YUV_IQ;
	video_para.qpmax = TRYIT_YUV_QP;
	video_para.gamma = TRYIT_YUV_GA;
	
	if (mfcenc_basic(video_para,t601par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Encode MPEG4 thread faild\n"));
	    pCmdData->bTestResult = FALSE;
	}
	else {
		LogComment(_T("MFC Encode MPEG4 thread finished\n"));
	    pCmdData->bTestResult = TRUE;
	}
	SetEvent(pCmdData->hWaitSimCodec);
	free(t601par);
	
    return 0;
}
// Thread to do Encode H.264.
DWORD  ThreadProc_Enc264(LPVOID lpParameter)
{    
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit3.yuv"};
	CODEC_MODE codec_modes[] = {CODEC_H264};
	PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	int forceExit = FALSE;
	ENC_PARAINFO video_para;
	TUX_MFC_PARA *t601par;
	
	LogComment(_T("This thread will encode H.264 video."));
	t601par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t601par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		pCmdData->bTestResult = FALSE;
		return 0;
	}
	t601par->infilename = pszFileNames;
	t601par->outfilename = NULL;
	t601par->codec_mode = codec_modes;
	t601par->outDISP = FALSE;
	
    video_para.width =  TRYIT_YUV_W;    
	video_para.height = TRYIT_YUV_H;
	video_para.frame_rate = TRYIT_YUV_FR;
	video_para.bitrate = TRYIT_YUV_BR;
	video_para.gop_num = TRYIT_YUV_GOP;
	video_para.intraqp = TRYIT_YUV_IQ;
	video_para.qpmax = TRYIT_YUV_QP;
	video_para.gamma = TRYIT_YUV_GA;
	
	if (mfcenc_basic(video_para,t601par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Encode 264 thread faild\n"));
	    pCmdData->bTestResult = FALSE;
	}
	else {
		LogComment(_T("MFC Encode 264 thread finished\n"));
	    pCmdData->bTestResult = TRUE;
	}
	SetEvent(pCmdData->hWaitSimCodec);
	free(t601par);
	
    return 0;
}
// Thread to do Decode H.263.
DWORD  ThreadProc_Dec263(LPVOID lpParameter)
{    
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.263"};
	CODEC_MODE codec_modes[] = {CODEC_H263};
	PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	int forceExit = FALSE;
	TUX_MFC_PARA *t602apar;
	
	LogComment(_T("This thread will decode H.263 video."));
	t602apar = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t602apar){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		pCmdData->bTestResult = FALSE;
		return 0;
	}
	t602apar->infilename = pszFileNames;
	t602apar->outfilename = NULL;
	t602apar->codec_mode = codec_modes;
	t602apar->outDISP = FALSE;
	t602apar->num_pips = NUM_PIP_VIDEOS;
	if (mfcdec_basic(t602apar,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode H.263 thread faild\n"));
	    pCmdData->bTestResult = FALSE;
	}
	else {
		LogComment(_T("MFC Decode H.263 thread finished\n"));
	    pCmdData->bTestResult = TRUE;
	}
	SetEvent(pCmdData->hWaitSimCodec);
	free(t602apar);
	
    return 0;
}
// Thread to do Decode H.264.
DWORD ThreadProc_Dec264(LPVOID lpParameter)
{    
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.264"};
	CODEC_MODE codec_modes[] = {CODEC_H264};
	PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	int forceExit = FALSE;
	TUX_MFC_PARA *t602apar;
	
	LogComment(_T("This thread will decode H.264 video."));
	t602apar = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t602apar){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		pCmdData->bTestResult = FALSE;
		return 0;
	}
	t602apar->infilename = pszFileNames;
	t602apar->outfilename = NULL;
	t602apar->codec_mode = codec_modes;
	t602apar->outDISP = FALSE;
	t602apar->num_pips = NUM_PIP_VIDEOS;
	if (mfcdec_basic(t602apar,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode H.264 thread faild\n"));
	    pCmdData->bTestResult = FALSE;
	}
	else {
		LogComment(_T("MFC Decode H.264 thread finished\n"));
	    pCmdData->bTestResult = TRUE;
	}
	SetEvent(pCmdData->hWaitSimCodec);
	free(t602apar);
	
    return 0;
}

// Thread to do Decode MPEG-4.
DWORD  ThreadProc_DecMPEG4(LPVOID lpParameter)
{    
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.m4v"};
	CODEC_MODE codec_modes[] = {CODEC_MPEG4};
	PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	int forceExit = FALSE;
	TUX_MFC_PARA *t602apar;
		
	LogComment(_T("This thread will decode MPEG4 video."));
	t602apar = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t602apar){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		pCmdData->bTestResult = FALSE;
		return 0;
	}
	t602apar->infilename = pszFileNames;
	t602apar->outfilename = NULL;
	t602apar->codec_mode = codec_modes;
	t602apar->outDISP = FALSE;
	t602apar->num_pips = NUM_PIP_VIDEOS;
	if (mfcdec_basic(t602apar,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode MPEG4 thread faild\n"));
	    pCmdData->bTestResult = FALSE;
	}
	else {
		LogComment(_T("MFC Decode MPEG4 thread finished\n"));
	    pCmdData->bTestResult = TRUE;
	}
	SetEvent(pCmdData->hWaitSimCodec);
	free(t602apar);
	
    return 0;
}
// Thread to do Decode VC-1.
DWORD  ThreadProc_DecVC1(LPVOID lpParameter)
{    
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.rcv"};
	CODEC_MODE codec_modes[] = {CODEC_VC1};
	PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	int forceExit = FALSE;
	TUX_MFC_PARA *t602bpar;
	    
	LogComment(_T("This thread will decode VC-1 video."));
	t602bpar = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t602bpar){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		pCmdData->bTestResult = FALSE;
		return 0;
	}
	t602bpar->infilename = pszFileNames;
	t602bpar->outfilename = NULL;
	t602bpar->codec_mode = codec_modes;
	t602bpar->outDISP = TRUE;
	t602bpar->num_pips = NUM_PIP_VIDEOS;
	if (mfcdec_basic(t602bpar,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode VC-1 thread faild\n"));
	    pCmdData->bTestResult = FALSE;
	}
	else {		
		LogComment(_T("MFC Decode VC-1 thread finished\n"));		
	    pCmdData->bTestResult = TRUE;
	}
	SetEvent(pCmdData->hWaitSimCodec);
	free(t602bpar);
	
    return 0;
}
// Encode an H.263 format video  from file.
UINT MFC263Enc(void)
{
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit1.yuv"};
	CODEC_MODE codec_modes[] = {CODEC_H263};
	int forceExit = FALSE;
	ENC_PARAINFO video_para;
	TUX_MFC_PARA *t001par;
	BOOL test_fail = FALSE;

	LogComment(_T("This test will encode H.263 video."));
    // Encode / Decode setting configuration
	t001par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t001par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		return CESTRESS_FAIL;
	}
	t001par->infilename = pszFileNames;
	t001par->outfilename = NULL;
	t001par->codec_mode = codec_modes;
	t001par->outDISP = FALSE;
	// Fill-up properites of target video
    video_para.width =  TRYIT_YUV_W;    
	video_para.height = TRYIT_YUV_H;
	video_para.frame_rate = TRYIT_YUV_FR;
	video_para.bitrate = TRYIT_YUV_BR;
	video_para.gop_num = TRYIT_YUV_GOP;
	video_para.intraqp = TRYIT_YUV_IQ;
	video_para.qpmax = TRYIT_YUV_QP;
	video_para.gamma = TRYIT_YUV_GA;
	
	// Execute and get result
    if (mfcenc_basic(video_para,t001par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Encode H263 test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		LogComment(_T("MFC Encode H263 test case finished\n"));		
	}
	free(t001par);

		return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}

// Encode a MPEG-4 format video  from file.
UINT MFCMPEG4Enc(void)
{
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit1.yuv"};
	CODEC_MODE codec_modes[] = {CODEC_MPEG4};
	int forceExit = FALSE;
	ENC_PARAINFO video_para;
	TUX_MFC_PARA *t002par;
	BOOL test_fail = FALSE;

	LogComment(_T("This test will encode MPEG4 video."));
	t002par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t002par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		return CESTRESS_FAIL;
	}
	t002par->infilename = pszFileNames;
	t002par->outfilename = NULL;
	t002par->codec_mode = codec_modes;
	t002par->outDISP = FALSE;
    video_para.width =  TRYIT_YUV_W;    
	video_para.height = TRYIT_YUV_H;
	video_para.frame_rate = TRYIT_YUV_FR;
	video_para.bitrate = TRYIT_YUV_BR;
	video_para.gop_num = TRYIT_YUV_GOP;
	video_para.intraqp = TRYIT_YUV_IQ;
	video_para.qpmax = TRYIT_YUV_QP;
	video_para.gamma = TRYIT_YUV_GA;
		
	if (mfcenc_basic(video_para,t002par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Encode MPEG4 test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		LogComment(_T("MFC Encode MPEG4 test case finished\n"));
	}
	free(t002par);

	return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}

// Encode an H.264 format video  from file.
UINT MFC264Enc(void)
{
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit1.yuv"};
	CODEC_MODE codec_modes[] = {CODEC_H264};
	int forceExit = FALSE;
	ENC_PARAINFO video_para;
	TUX_MFC_PARA *t003par;
	//char* olist[] = {"\\output003.264"};
	BOOL test_fail = FALSE;
	
	LogComment(_T("This test will encode H.264 video."));
	t003par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	if (NULL == t003par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		return CESTRESS_FAIL;
	}
	t003par->infilename = pszFileNames;
	t003par->outfilename = NULL;
	t003par->codec_mode = codec_modes;
	t003par->outDISP = FALSE;
    video_para.width =  TRYIT_YUV_W;    
	video_para.height = TRYIT_YUV_H;
	video_para.frame_rate = TRYIT_YUV_FR;
	video_para.bitrate = TRYIT_YUV_BR;
	video_para.gop_num = TRYIT_YUV_GOP;
	video_para.intraqp = TRYIT_YUV_IQ;
	video_para.qpmax = TRYIT_YUV_QP;
	video_para.gamma = TRYIT_YUV_GA;
	
	if (mfcenc_basic(video_para,t003par,&forceExit) < 0 ) {    
	    LogFail(_T("MFC Encode H.264 test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		LogComment(_T("MFC Encode H.264 test case finished\n"));		
	}
	free(t003par);

	return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}
// Decode an H.263 format video. 
UINT MFC263Dec(void)
{
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.263"};
	CODEC_MODE codec_modes[] = {CODEC_H263};
	int forceExit = FALSE;
	TUX_MFC_PARA *t004par;
	BOOL test_fail = FALSE;

    LogComment(_T("This test will decode H.263 video."));	
    t004par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    if (NULL == t004par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		return CESTRESS_FAIL;
	}
	t004par->infilename = pszFileNames;
	t004par->outfilename = NULL;
	t004par->codec_mode = codec_modes;
	t004par->outDISP = FALSE;
	t004par->num_pips = NUM_PIP_VIDEOS;
	
	if (mfcdec_basic(t004par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode H.263 test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		LogComment(_T("MFC Decode H.263 test case finished\n"));
	}
	
	free(t004par);
	return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}

// Decode an MPEG-4 format video.
UINT MFCMPEG4Dec(void)
{
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.m4v"};
	CODEC_MODE codec_modes[] = {CODEC_MPEG4};
	int forceExit = FALSE;
	TUX_MFC_PARA *t005par;
	BOOL test_fail = FALSE;
		
    LogComment(_T("This test will decode MPEG4 video."));
    t005par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    if (NULL == t005par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		return CESTRESS_FAIL;
	}
	t005par->infilename = pszFileNames;
	t005par->outfilename = NULL;
	t005par->codec_mode = codec_modes;
	t005par->outDISP = FALSE;
	t005par->num_pips = NUM_PIP_VIDEOS;
    
	if (mfcdec_basic(t005par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode MPEG4 test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		LogComment(_T("MFC Decode MPEG4 test case finished\n"));	
	}
	free(t005par);
	
	return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}

// Decode an H.264 format video.
UINT MFC264Dec(void)
{
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.264"};
	CODEC_MODE codec_modes[] = {CODEC_H264};
	int forceExit = FALSE;
	TUX_MFC_PARA *t006par;
	BOOL test_fail = FALSE;
	
	LogComment(_T("This test will decode H.264 video."));
    t006par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    if (NULL == t006par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		return CESTRESS_FAIL;
	}
	t006par->infilename = pszFileNames;
	t006par->outfilename = NULL;
	t006par->codec_mode = codec_modes;
	t006par->outDISP = FALSE;
	t006par->num_pips = NUM_PIP_VIDEOS;
    
	if (mfcdec_basic(t006par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode H.264 test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		LogComment(_T("MFC Decode H.264 test case finished\n"));
	}
	free(t006par);
	
	return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}

// Decode a VC-1 format video.
UINT MFCVC1Dec(void)
{
	LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.rcv"};
	CODEC_MODE codec_modes[] = {CODEC_VC1};
	int forceExit = FALSE;
	TUX_MFC_PARA *t007par;
	BOOL test_fail = FALSE;

    LogComment(_T("This test will decode VC-1 video."));
    t007par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    if (NULL == t007par){
		LogFail(_T("Allocate memory for TUX_MFC_PARA fail!\n"));
		return CESTRESS_FAIL;
	}
	t007par->infilename = pszFileNames;
	t007par->outfilename = NULL;
	t007par->codec_mode = codec_modes;
	t007par->outDISP = TRUE;
	t007par->num_pips = NUM_PIP_VIDEOS;

	if (mfcdec_basic(t007par,&forceExit) < 0 ) {
	    LogFail(_T("MFC Decode VC-1 test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		LogComment(_T("MFC Decode VC-1  test case finished\n"));
	}
	free(t007par);
	
	return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}
// Simultaneous encoding H.263, H.264 and MPEG-4 or decode H.263, H.264, MPEG-4 and VC-1.
// Create three threads and do encode/decode randomly.
UINT MFCSimCodec(void)
{
	CMD_DATA *pCmdData1=new CMD_DATA;
	HANDLE hThread1;
	DWORD dwWaitRet1;
	DWORD   IDThreadKey1;
	CMD_DATA *pCmdData2=new CMD_DATA;
	HANDLE hThread2;
	DWORD dwWaitRet2;
	DWORD   IDThreadKey2;
	CMD_DATA *pCmdData3=new CMD_DATA;
	HANDLE hThread3;
	DWORD dwWaitRet3;
	DWORD   IDThreadKey3;	
	BOOL test_fail = FALSE;
	int th1, th2, th3;
  
    LogComment(_T("This test case simultaneously executes MFC encode/decode!\n"));
	if( (pCmdData1 == NULL) || (pCmdData2 == NULL) || (pCmdData3 == NULL) )	{
		if(pCmdData1 != NULL) 
			delete pCmdData1;
		if(pCmdData2 != NULL)
			delete pCmdData2;
		if(pCmdData3 != NULL)
			delete pCmdData3;
	    LogFail(_T("Can not create new command data!\n"));
	    return CESTRESS_FAIL;
    }
    
    th1 = rand() %7;
    LogVerbose(_T("Random th1 [%d]\n"),th1);
    running_codec[th1] = T_RUNNING;
    th2 = rand() %7;
    LogVerbose(_T("Random th2 [%d]\n"),th2);
    while( T_RUNNING == running_codec[th2]) {
		th2 = rand() %7;
		LogVerbose(_T("Random th2 [%d]\n"),th2);
	}
	running_codec[th2] = T_RUNNING;
	th3 = rand() %7;
    LogVerbose(_T("Random th3 [%d]\n"),th3);
    while( T_RUNNING == running_codec[th3]) {
		th3 = rand() %7;
		LogVerbose(_T("Random th3 [%d]\n"),th3);
	}
	running_codec[th3] = T_RUNNING;
	
	//Create and run threads
    pCmdData1->bTestResult=TRUE;
	pCmdData1->hWaitSimCodec = CreateEvent(0,FALSE,FALSE,_T("MFC_SimThread1_Event"));
	if( NULL == pCmdData1->hWaitSimCodec ) {
		LogFail(_T("Create Event: Create Event1 failed!\n"));
		test_fail = TRUE;
		goto LEAVEREAD;
	}
    
    hThread1 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TFL_MFC[th1], (LPVOID)pCmdData1, 0,&IDThreadKey1);
    if( NULL == hThread1 )	{
	    LogFail(_T("Create Thread: Create Thread1 failed!\n"));
		test_fail = TRUE;
		goto LEAVEREAD;
	}
    dwWaitRet1 = WaitForSingleObject(pCmdData1->hWaitSimCodec, TIMEOUT_CONSTANT);
   
    pCmdData2->bTestResult=TRUE;
	pCmdData2->hWaitSimCodec = CreateEvent(0,FALSE,FALSE,_T("MFC_SimThread2_Event"));
	if( NULL == pCmdData2->hWaitSimCodec ) {
		LogFail(_T("Create Event: Create Event2 failed!\n"));
		test_fail = TRUE;
		goto LEAVEREAD;
	}
    
    hThread2 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TFL_MFC[th2], (LPVOID)pCmdData2, 0,&IDThreadKey2);
    if( NULL == hThread2 )	{
	    LogFail(_T("Create Thread: Create Thread2 failed!\n"));
		test_fail = TRUE;
		goto LEAVEREAD;
	}
    dwWaitRet2 = WaitForSingleObject(pCmdData2->hWaitSimCodec, TIMEOUT_CONSTANT);
    
	pCmdData3->bTestResult=TRUE;
	pCmdData3->hWaitSimCodec = CreateEvent(0,FALSE,FALSE,_T("MFC_SimThread3_Event"));
	if( NULL == pCmdData3->hWaitSimCodec ) {
		LogFail(_T("Create Event: Create Event3 failed!\n"));
		test_fail = TRUE;
		goto LEAVEREAD;
	}
    
    hThread3 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TFL_MFC[th3], (LPVOID)pCmdData3, 0,&IDThreadKey3);
    if( NULL == hThread3 )	{
	    LogFail(_T("Create Thread: Create Thread3 failed!\n"));
		test_fail = TRUE;
		goto LEAVEREAD;
	}
    dwWaitRet3 = WaitForSingleObject(pCmdData3->hWaitSimCodec, TIMEOUT_CONSTANT);
    // see if timeout
    if ( WAIT_TIMEOUT == dwWaitRet1 ) {
        LogFail(_T("MFC simultaneously thread 1 timeout."));
		pCmdData1->bTestResult = FALSE;
        goto LEAVEREAD;
    } 
    if ( WAIT_TIMEOUT == dwWaitRet2 ) {
        LogFail(_T("MFC simultaneously thread 2 timeout."));
		pCmdData2->bTestResult = FALSE;
        goto LEAVEREAD;
    }
    if ( WAIT_TIMEOUT == dwWaitRet3 ) {
        LogFail(_T("MFC simultaneously thread 3 timeout."));
		pCmdData3->bTestResult = FALSE;
    }

LEAVEREAD: 
    if ( (NULL != pCmdData1->hWaitSimCodec) && (CloseHandle(pCmdData1->hWaitSimCodec) == 0)  )	{
		LogFail(_T("Close Handle: pCmdData1 failed!\n"));
		test_fail = TRUE;
	}
	if ( (NULL != hThread1) && (CloseHandle(hThread1) == 0) )	{
	    LogFail(_T("Close Handle: Thread1 failed!\n"));
	    test_fail = TRUE;
    }
    if ( (NULL != pCmdData2->hWaitSimCodec) && (CloseHandle(pCmdData2->hWaitSimCodec) == 0)  )	{
		LogFail(_T("Close Handle: pCmdData2 failed!\n"));
		test_fail = TRUE;
	}
	if ( (NULL != hThread2) && (CloseHandle(hThread2) == 0) )	{
	    LogFail(_T("Close Handle: Thread2 failed!\n"));
	    test_fail = TRUE;
    }
    if ( (NULL != pCmdData3->hWaitSimCodec) && (CloseHandle(pCmdData3->hWaitSimCodec) == 0)  )	{
		LogFail(_T("Close Handle: pCmdData3 failed!\n"));
		test_fail = TRUE;
	}
	if ( (NULL != hThread3) && (CloseHandle(hThread3) == 0) )	{
	    LogFail(_T("Close Handle: Thread3 failed!\n"));
	    test_fail = TRUE;
    }

    if ( (pCmdData1->bTestResult == TRUE) && (pCmdData2->bTestResult == TRUE) && (pCmdData3->bTestResult == TRUE) ) {
        LogComment(_T("MFC simultaneously encode/decode test succeeded"));
    } 
	else {
		LogFail(_T("MFC simultaneously encode/decode test failed"));
		test_fail = TRUE;
	}
	running_codec[th1] = T_REST;
	running_codec[th2] = T_REST;
	running_codec[th3] = T_REST;
	delete pCmdData1;
    delete pCmdData2;
    delete pCmdData3;

    return (test_fail == TRUE) ? CESTRESS_FAIL:CESTRESS_PASS;
}

///////////////////////////////////////////////////////////////////////////////
//
// @func	(Required) BOOL | InitializeStressModule |
//			Called by the harness after your DLL is loaded.
// 
// @rdesc	Return TRUE if successful.  If you return FALSE, CEStress will 
//			terminate your module.
// 
// @parm	[in] <t MODULE_PARAMS>* | pmp | Pointer to the module params info 
//			passed in by the stress harness.  Most of these params are handled 
//			by the harness, but you can retrieve the module's user command
//          line from this structure.
// 
// @parm	[out] UINT* | pnThreads | Set the value pointed to by this param 
//			to the number of test threads you want your module to run.  The 
//			module container that loads this DLL will manage the life-time of 
//			these threads.  Each thread will call your <f DoStressIteration>
//			function in a loop.
// 		
// @comm    You must call InitializeStressUtils( ) (see \\\\cestress\\docs\\stress\\stressutils.hlp) and 
//			pass it the <t MODULE_PARAMS>* that was passed in to this function by
//			the harness.  You may also do any test-specific initialization here.
//
//
//
BOOL InitializeStressModule (
							/*[in]*/ MODULE_PARAMS* pmp, 
							/*[out]*/ UINT* pnThreads
							)
{
	int i;
	*pnThreads = 1;
	
	// !! You must call this before using any stress utils !!

	LogVerbose(_T("Start InitializeStressModule\n"));
	InitializeStressUtils (
							_T("MFC_STRESS"),									// Module name to be used in logging
							LOGZONE(SLOG_SPACE_DRIVERS, SLOG_DEFAULT),	// Logging zones used by default
							pmp												// Forward the Module params passed on the cmd line
							);

	// Note on Logging Zones: 
	//
	// Logging is filtered at two different levels: by "logging space" and
	// by "logging zones".  The 16 logging spaces roughly corresponds to the major
	// feature areas in the system (including apps).  Each space has 16 sub-zones
	// for a finer filtering granularity.
	//
	// Use the LOGZONE(space, zones) macro to indicate the logging space
	// that your module will log under (only one allowed) and the zones
	// in that space that you will log under when the space is enabled
	// (may be multiple OR'd values).
	//
	// See \test\stress\stress\inc\logging.h for a list of available
	// logging spaces and the zones that are defined under each space.
	
	// Reset all running status.
	for(i=0; i <7; i++) {
		running_codec[i] = T_REST;
	}
	
	srand(GetTickCount());

	// (Optional) 
	// Get the number of modules of this type (i.e. same exe or dll)
	// that are currently running.  This allows you to bail (return FALSE)
	// if your module can not be run with more than a certain number of
	// instances.
	
	LONG count = GetRunningModuleCount(g_hInst);
    if (count > 1)
    {
        LogWarn1(_T("Another instance of MFC_STRESS module already running."));
        return FALSE;
    }

	LogVerbose(_T("Running Modules = %d"), count);


	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// @func	(Required) UINT | DoStressIteration | 
//			Called once at the start of each test thread.  
// 
// @rdesc	Return a <t CESTRESS return value>.  
// 
// @parm	[in] HANDLE | hThread | A pseudohandle for the current thread. 
//			A pseudohandle is a special constant that is interpreted as the 
//			current thread handle. The calling thread can use this handle to 
//			specify itself whenever a thread handle is required. 
//
// @parm	[in] DWORD | dwThreadId | This thread's identifier.
//
// @parm	[in] LPVOID | pv | This can be cast to a pointer to an <t ITERATION_INFO>
//			structure.
// 		
// @comm    This is the main worker function for your test.  A test iteration should 
//			begin and end in this function (though you will likely call other test 
//			functions along the way).  The return value represents the result for a  
//			single iteration, or test case.  
//
//
//

UINT DoStressIteration (
						/*[in]*/ HANDLE hThread, 
						/*[in]*/ DWORD dwThreadId, 
						/*[in]*/ LPVOID pv) 						
{
	ITERATION_INFO* pIter = (ITERATION_INFO*) pv;
	UINT st_result;
	int i;

	LogVerbose(_T("Thread %i, iteration %i"), pIter->index, pIter->iteration);	
	for(i=0; i <7; i++) {
		running_codec[i] = T_REST;
	}
	
	switch ( rand() & 7 ) {
		case 0:
			st_result = MFC263Enc();
		break;
		case 1:
			st_result = MFCMPEG4Enc();
		break;
		case 2:
			st_result = MFC264Enc();
		break;
		case 3:
			st_result = MFC263Dec();
		break;
		case 4:
			st_result = MFCMPEG4Dec();
		break;
		case 5:
			st_result = MFC264Dec();
		break;
		case 6:
			st_result = MFCVC1Dec();
		break;
		case 7:
			st_result = MFCSimCodec();
		break;
		default:
			LogFail(_T("Should not run here!\n"));
		break;
	}

	return st_result;	
}


///////////////////////////////////////////////////////////////////////////////
//
// @func	(Required) DWORD | TerminateStressModule | 
//			Called once before your module exits.  
// 
// @rdesc	Unused.
// 		
// @comm    There is no required action.  This is provided for test-specific 
//			clean-up only.
//

DWORD TerminateStressModule (void)
{

	// This will be used as the process exit code.

	return ((DWORD) -1);
}



///////////////////////////////////////////////////////////
BOOL WINAPI DllMain(
					HANDLE hInstance, 
					ULONG dwReason, 
					LPVOID lpReserved
					)
{
	g_hInst = hInstance;
    return TRUE;
}
////////////////////////////////////////////////////////////

