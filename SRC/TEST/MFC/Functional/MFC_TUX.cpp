//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//  MFC_TUX TUX DLL
//
//  Module: MFC_TUX.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdio.h>
#include "MfcDrvParams.h"
#include "mfc_decode.h"
#include "mfc_encode.h"
#include "mfc_render.h"
#include "main.h"
#include "globals.h"

int mfcdec_basic(HWND, LPCTSTR, CODEC_MODE, int, int*);
int mfcenc_basic(ENC_PARAINFO, LPCTSTR, CODEC_MODE, int, int*);

#define NUM_PIP_VIDEOS      1
#define TIMEOUT_CONSTANT    10000

typedef struct
{
   HANDLE hWaitInvalidTest;
   bool bTestResult;
}CMD_DATA,*PCMD_DATA;

#define INVALID_OPEN_NEGATIVE   -1
#define INVALID_OPEN_MAX        0xFFFFFFFF
// Thread for Invalid Open test.
DWORD  ThreadProc_InvalidOpen(LPVOID lpParameter)
{
    HANDLE hDriver = INVALID_HANDLE_VALUE;    
    BOOL bResult = FALSE;
    PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
    
    // Negative value for access code test     
    hDriver = CreateFile( L"MFC1:",  INVALID_OPEN_NEGATIVE, 0, NULL, OPEN_ALWAYS, 0, NULL );

    if ( hDriver != NULL && hDriver != INVALID_HANDLE_VALUE ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC open with negative value for access code test failed"));
        pCmdData->bTestResult=FALSE;

        bResult=CloseHandle(hDriver);
        if ( bResult == FALSE ) {
            g_pKato->Log(LOG_FAIL, TEXT("MFC close failed"));
            pCmdData->bTestResult=FALSE;
        }
    }

    // Large value for access code test     
    hDriver = CreateFile( L"MFC1:",  INVALID_OPEN_MAX, 0, NULL,  CREATE_NEW, 0, NULL );
    
    if ( hDriver != NULL && hDriver != INVALID_HANDLE_VALUE ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC open with large value for access code test failed"));
        pCmdData->bTestResult=FALSE;
    
        bResult=CloseHandle(hDriver);
        if ( bResult == FALSE ) {
            g_pKato->Log(LOG_FAIL, TEXT("MFC close failed"));
            pCmdData->bTestResult=FALSE;
        }
    }
    SetEvent(pCmdData->hWaitInvalidTest);
    
    return 0;    
}
// Thread for Invalid IO Control test.
DWORD  ThreadProc_InvalidIOCtl(LPVOID lpParameter)
{
    HANDLE hDriver = INVALID_HANDLE_VALUE;    
    BOOL bResult = FALSE;
    PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
    MFC_ARGS    mfc_args;
    
    hDriver = CreateFile(L"MFC1:", GENERIC_READ|GENERIC_WRITE,
                         0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hDriver == NULL && hDriver == INVALID_HANDLE_VALUE){
        RETAILMSG(1, (TEXT("MFC Driver CreateFile get failed! \r\n")));
        pCmdData->bTestResult=FALSE;
        return 0;
    }
    mfc_args.get_config.in_config_param     = MFC_GET_CONFIG_DEC_FRAME_NEED_COUNT;
    //init the out value
    mfc_args.get_config.out_config_value[0]  = 0;
    mfc_args.get_config.out_config_value[1]  = 0;
    // Negative value for access code test     
    if( FALSE != DeviceIoControl(hDriver, IOCTL_MFC_GET_CONFIG, &mfc_args, INVALID_OPEN_NEGATIVE, 
                    NULL, 0, NULL, NULL) ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC IOCTL with negative value for access code test failed"));
        pCmdData->bTestResult=FALSE;

        bResult=CloseHandle(hDriver);
        if ( bResult == FALSE ) {
            g_pKato->Log(LOG_FAIL, TEXT("MFC close failed"));
            pCmdData->bTestResult=FALSE;
        }
    }

    // Large value for access code test     
    if( FALSE != DeviceIoControl(hDriver, IOCTL_MFC_GET_CONFIG, &mfc_args, INVALID_OPEN_MAX, 
                    NULL, 0, NULL, NULL) ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC IOCTL with large value for access code test failed"));
        pCmdData->bTestResult=FALSE;
    
        bResult=CloseHandle(hDriver);
        if ( bResult == FALSE ) {
            g_pKato->Log(LOG_FAIL, TEXT("MFC close failed"));
            pCmdData->bTestResult=FALSE;
        }
    } 
    
    bResult=CloseHandle(hDriver);
    SetEvent(pCmdData->hWaitInvalidTest);
    
    return 0;
}
//Properites of tryit.yuv file
#define TRYIT_YUV_W     320
#define TRYIT_YUV_H     240
#define TRYIT_YUV_FR    15
#define TRYIT_YUV_BR    1000
#define TRYIT_YUV_GOP   15
#define TRYIT_YUV_IQ    10
#define TRYIT_YUV_QP    10
#define TRYIT_YUV_GA    0.75

// Thread to do Encode H.263.
DWORD  ThreadProc_Enc263(LPVOID lpParameter)
{    
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit1.yuv"};
    CODEC_MODE codec_modes[] = {CODEC_H263};
    PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
    int forceExit;
    ENC_PARAINFO video_para;
    TUX_MFC_PARA *t601par;
    char* olist[] = {"\\Storage Card\\output.263"};
    // Fill-up properites of target video
    video_para.width =  TRYIT_YUV_W;    
    video_para.height = TRYIT_YUV_H;
    video_para.frame_rate = TRYIT_YUV_FR;
    video_para.bitrate = TRYIT_YUV_BR;
    video_para.gop_num = TRYIT_YUV_GOP;
    video_para.intraqp = TRYIT_YUV_IQ;
    video_para.qpmax = TRYIT_YUV_QP;
    video_para.gamma = TRYIT_YUV_GA;
    // Encode/Decode setting configuration
    t601par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t601par->infilename = pszFileNames;
    t601par->outfilename = olist[0];
    t601par->codec_mode = codec_modes;
    t601par->outDISP = FALSE;
    // Execute and get result
    if (mfcenc_basic(video_para,t601par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Encode H263 thread faild\n"));
        pCmdData->bTestResult = FALSE;
    }
    else {
        g_pKato->Log(LOG_PASS, TEXT("MFC Encode H263 thread finished\n"));
        pCmdData->bTestResult = TRUE;
    }
    
    SetEvent(pCmdData->hWaitInvalidTest);
    free(t601par);
    return 0;
}
// Thread to do Encode MPEG-4.
DWORD  ThreadProc_EncMPEG4(LPVOID lpParameter)
{    
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit2.yuv"};
    CODEC_MODE codec_modes[] = {CODEC_MPEG4};
    PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
    int forceExit;
    ENC_PARAINFO video_para;
    TUX_MFC_PARA *t601par;
    char* olist[] = {"\\Storage Card\\output.m4v"};
    
    video_para.width =  TRYIT_YUV_W;    
    video_para.height = TRYIT_YUV_H;
    video_para.frame_rate = TRYIT_YUV_FR;
    video_para.bitrate = TRYIT_YUV_BR;
    video_para.gop_num = TRYIT_YUV_GOP;
    video_para.intraqp = TRYIT_YUV_IQ;
    video_para.qpmax = TRYIT_YUV_QP;
    video_para.gamma = TRYIT_YUV_GA;
    t601par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t601par->infilename = pszFileNames;
    t601par->outfilename = olist[0];
    t601par->codec_mode = codec_modes;
    t601par->outDISP = FALSE;
    
    if (mfcenc_basic(video_para,t601par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Encode MPEG4 thread faild\n"));
        pCmdData->bTestResult = FALSE;
    }
    else {
        g_pKato->Log(LOG_PASS, TEXT("MFC Encode MPEG4 thread finished\n"));
        pCmdData->bTestResult = TRUE;
    }
    
    SetEvent(pCmdData->hWaitInvalidTest);
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
    char* olist[] = {"\\Storage Card\\output.264"};
    
    video_para.width =  TRYIT_YUV_W;    
    video_para.height = TRYIT_YUV_H;
    video_para.frame_rate = TRYIT_YUV_FR;
    video_para.bitrate = TRYIT_YUV_BR;
    video_para.gop_num = TRYIT_YUV_GOP;
    video_para.intraqp = TRYIT_YUV_IQ;
    video_para.qpmax = TRYIT_YUV_QP;
    video_para.gamma = TRYIT_YUV_GA;
    t601par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t601par->infilename = pszFileNames;
    t601par->outfilename = olist[0];
    t601par->codec_mode = codec_modes;
    t601par->outDISP = FALSE;
    
    if (mfcenc_basic(video_para,t601par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Encode 264 thread faild\n"));
        pCmdData->bTestResult = FALSE;
    }
    else {
        g_pKato->Log(LOG_PASS, TEXT("MFC Encode 264 thread finished\n"));
        pCmdData->bTestResult = TRUE;
    }
    
    SetEvent(pCmdData->hWaitInvalidTest);
    free(t601par);
    
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
    char* olist[] = {"\\Storage Card\\output602a.yuv"};
    
    t602apar = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t602apar->infilename = pszFileNames;
    t602apar->outfilename = olist[0];
    t602apar->codec_mode = codec_modes;
    t602apar->outDISP = FALSE;
    t602apar->num_pips = NUM_PIP_VIDEOS;
    if (mfcdec_basic(t602apar,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Decode MPEG4 thread faild\n"));
        pCmdData->bTestResult = FALSE;
    }
    else {
        g_pKato->Log(LOG_PASS, TEXT("MFC Decode MPEG4 thread finished\n"));
        pCmdData->bTestResult = TRUE;
    }
    
    SetEvent(pCmdData->hWaitInvalidTest);
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
    int user_ret;
    HWND hWnd1;
        
    t602bpar = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t602bpar->infilename = pszFileNames;
    t602bpar->outfilename = NULL;
    t602bpar->codec_mode = codec_modes;
    t602bpar->outDISP = TRUE;
    t602bpar->num_pips = NUM_PIP_VIDEOS;
    hWnd1 = GetForegroundWindow();
    if (mfcdec_basic(t602bpar,&forceExit) < 0 ) {
        mfc_render_final();
        g_pKato->Log(LOG_FAIL, TEXT("MFC Decode VC-1 thread faild\n"));
        pCmdData->bTestResult = FALSE;
    }
    else {
        mfc_render_final();
        g_pKato->Log(LOG_COMMENT, TEXT("MFC Decode VC-1 thread finished\n"));
        user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this video correctly!", 
                                (LPCWSTR)L"MFC TUX Result", MB_YESNO);
        if( user_ret != IDYES ){
            g_pKato->Log(LOG_FAIL, TEXT("User reported decode video failed!\n."));
            pCmdData->bTestResult = FALSE;
        }
        pCmdData->bTestResult = TRUE;
    }
    
    SetEvent(pCmdData->hWaitInvalidTest);
    free(t602bpar);
    
    return 0;
}
// Test Case 1001: Encode an H.263 format video from file.
TESTPROCAPI MFC263Enc(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit1.yuv"};
    CODEC_MODE codec_modes[] = {CODEC_H263};
    int forceExit;
    ENC_PARAINFO video_para;
    TUX_MFC_PARA *t001par;
    char* olist[] = {"\\Storage Card\\output001.263"};
    HWND hWnd1;
        BOOL test_fail = FALSE;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    // Encode / Decode setting configuration
    t001par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t001par->infilename = pszFileNames;
    t001par->outfilename = olist[0];
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
    hWnd1 = GetForegroundWindow();
    if( hWnd1 == NULL  ) {
        g_pKato->Log(LOG_FAIL, TEXT("Init MFC Encode test case faild\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    // Execute and get result
    if (mfcenc_basic(video_para,t001par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Encode H263 test case faild\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_PASS, TEXT("MFC Encode H263 test case finished\n"));
        //Binary compare against an expected file output needs to be checked, this will be implemented in Chelan test code.
    }
LEAVEREAD:
    free(t001par);

        return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}
// Test Case 1002: Encode an MPEG-4 format video  from file.
TESTPROCAPI MFCMPEG4Enc(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit2.yuv"};
    CODEC_MODE codec_modes[] = {CODEC_MPEG4};
    int forceExit;
    ENC_PARAINFO video_para;
    TUX_MFC_PARA *t002par;
    char* olist[] = {"\\Storage Card\\output002.m4v"};
    HWND hWnd1;
        BOOL test_fail = FALSE;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    t002par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t002par->infilename = pszFileNames;
    t002par->outfilename = olist[0];
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
    hWnd1 = GetForegroundWindow();
    if( hWnd1 == NULL  ) {
        g_pKato->Log(LOG_FAIL, TEXT("Init MFC Encode test case faild\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    g_pKato->Log(LOG_COMMENT, TEXT("This test will encode MPEG4 video and save as file."));
    if (mfcenc_basic(video_para,t002par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Encode MPEG4 test case faild\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_COMMENT, TEXT("MFC Encode MPEG4 test case finished\n"));
        //Binary compare against an expected file output needs to be checked, this will be implemented in Chelan test code.
    
    }
LEAVEREAD:  
    free(t002par);

    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}
// Test Case 1003: Encode an H.264 format video  from file.
TESTPROCAPI MFC264Enc(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\tryit3.yuv"};
    CODEC_MODE codec_modes[] = {CODEC_H264};
    int forceExit;
    ENC_PARAINFO video_para;
    TUX_MFC_PARA *t003par;
    char* olist[] = {"\\Storage Card\\output003.264"};
    HWND hWnd1;
     BOOL test_fail = FALSE;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    t003par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t003par->infilename = pszFileNames;
    t003par->outfilename = olist[0];
    t003par->codec_mode = codec_modes;
    t003par->outDISP = FALSE;
    t003par->num_pips = NUM_PIP_VIDEOS;
    video_para.width =  TRYIT_YUV_W;    
    video_para.height = TRYIT_YUV_H;
    video_para.frame_rate = TRYIT_YUV_FR;
    video_para.bitrate = TRYIT_YUV_BR;
    video_para.gop_num = TRYIT_YUV_GOP;
    video_para.intraqp = TRYIT_YUV_IQ;
    video_para.qpmax = TRYIT_YUV_QP;
    video_para.gamma = TRYIT_YUV_GA;
    hWnd1 = GetForegroundWindow();
    if( hWnd1 == NULL  ) {
        g_pKato->Log(LOG_FAIL, TEXT("Init MFC Encode test case faild\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    g_pKato->Log(LOG_COMMENT, TEXT("This test will encode H.264 video and save as file."));
    if (mfcenc_basic(video_para,t003par,&forceExit) < 0 ) {    
        g_pKato->Log(LOG_FAIL, TEXT("MFC Encode H.264 test case faild\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_COMMENT, TEXT("MFC Encode H.264 test case finished\n"));
        //Binary compare against an expected file output needs to be checked, this will be implemented in Chelan test code.
    
    }
LEAVEREAD:      
    free(t003par);

    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}
// Test Case 1004: Decode an H.263 format video and show on LCD. 
TESTPROCAPI MFC263Dec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HWND hWnd1;
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.263"};
    CODEC_MODE codec_modes[] = {CODEC_H263};
    int forceExit;
    TUX_MFC_PARA *t004par;
    int user_ret;
    BOOL test_fail = FALSE;
    
    forceExit = FALSE;
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will decode H.263 video and show on LCD."));
    
    t004par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t004par->infilename = pszFileNames;
    t004par->outfilename = NULL;
    t004par->codec_mode = codec_modes;
    t004par->outDISP = TRUE;
    t004par->num_pips = NUM_PIP_VIDEOS;
    hWnd1 = GetForegroundWindow();
    if( (hWnd1 == NULL) || (mfc_render_init(hWnd1) == FALSE) ) {
        g_pKato->Log(LOG_FAIL, TEXT("Init MFC Decode test case faild\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    if (mfcdec_basic(t004par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Decode H.263 test case faild\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_COMMENT, TEXT("MFC Decode H.263 test case finished\n"));
        user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this video correctly!", 
                                (LPCWSTR)L"MFC TUX Result", MB_YESNO);
        if( user_ret != IDYES ){
            g_pKato->Log(LOG_FAIL, TEXT("User reported decode video failed!\n."));
            test_fail = TRUE;
        }
    }
    mfc_render_final();
LEAVEREAD:  
    free(t004par);
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

// Test Case 1005: Decode an MPEG-4 format video and show on LCD.
TESTPROCAPI MFCMPEG4Dec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HWND hWnd1;
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.m4v"};
    CODEC_MODE codec_modes[] = {CODEC_MPEG4};
    int forceExit;
    TUX_MFC_PARA *t005par;
    int user_ret;
    BOOL test_fail = FALSE;
    
    forceExit = FALSE;
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will decode MPEG4 video and show on LCD."));
    
    t005par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t005par->infilename = pszFileNames;
    t005par->outfilename = NULL;
    t005par->codec_mode = codec_modes;
    t005par->outDISP = TRUE;
    t005par->num_pips = NUM_PIP_VIDEOS;
    hWnd1 = GetForegroundWindow();
    if( (hWnd1 == NULL) || (mfc_render_init(hWnd1) == FALSE) ) {
        g_pKato->Log(LOG_FAIL, TEXT("Init MFC Decode test case faild\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    if (mfcdec_basic(t005par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Decode MPEG4 test case faild\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_COMMENT, TEXT("MFC Decode MPEG4 test case finished\n"));
        user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this video correctly!", 
                                (LPCWSTR)L"MFC TUX Result", MB_YESNO);
        if( user_ret != IDYES ){
            g_pKato->Log(LOG_FAIL, TEXT("User reported decode video failed!\n."));
            test_fail = TRUE;
        }
    }
    mfc_render_final();
LEAVEREAD:
    free(t005par);
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

// Test Case 1006: Decode an H.264 format video and show on LCD.
TESTPROCAPI MFC264Dec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HWND hWnd1;
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.264"};
    CODEC_MODE codec_modes[] = {CODEC_H264};
    int forceExit;
    TUX_MFC_PARA *t006par;
    int user_ret;
    BOOL test_fail = FALSE;
    
    forceExit = FALSE;
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will decode H.264 video and show on LCD."));
    
    t006par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t006par->infilename = pszFileNames;
    t006par->outfilename = NULL;
    t006par->codec_mode = codec_modes;
    t006par->outDISP = TRUE;
    t006par->num_pips = NUM_PIP_VIDEOS;
    hWnd1 = GetForegroundWindow();
    if( (hWnd1 == NULL) || (mfc_render_init(hWnd1) == FALSE) ) {
        g_pKato->Log(LOG_FAIL, TEXT("Init MFC Decode test case faild\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    if (mfcdec_basic(t006par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Decode H.264 test case faild\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_COMMENT, TEXT("MFC Decode H.264 test case finished\n"));
        user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this video correctly!", 
                                (LPCWSTR)L"MFC TUX Result", MB_YESNO);
        if( user_ret != IDYES ){
            g_pKato->Log(LOG_FAIL, TEXT("User reported decode video failed!\n."));
            test_fail = TRUE;
        }
    }
    mfc_render_final();
LEAVEREAD:  
    free(t006par);
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

// Test Case 1007: Decode a VC-1 format video and show on LCD.
TESTPROCAPI MFCVC1Dec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HWND hWnd1;
    LPCTSTR pszFileNames[]   = {L"\\Storage Card\\sample.rcv"};
    CODEC_MODE codec_modes[] = {CODEC_VC1};
    int forceExit;
    TUX_MFC_PARA *t007par;
    int user_ret;
    BOOL test_fail = FALSE;
    
    forceExit = FALSE;
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will decode VC-1 video and show on LCD."));
    
    t007par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
    t007par->infilename = pszFileNames;
    t007par->outfilename = NULL;
    t007par->codec_mode = codec_modes;
    t007par->outDISP = TRUE;
    t007par->num_pips = NUM_PIP_VIDEOS;
    hWnd1 = GetForegroundWindow();
    if( (hWnd1 == NULL) || (mfc_render_init(hWnd1) == FALSE) ) {
        g_pKato->Log(LOG_FAIL, TEXT("Init MFC Decode test case faild\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    if (mfcdec_basic(t007par,&forceExit) < 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC Decode VC-1 test case faild\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_COMMENT, TEXT("MFC Decode VC-1  test case finished\n"));
        user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this video correctly!", 
                                (LPCWSTR)L"MFC TUX Result", MB_YESNO);
        if( user_ret != IDYES ){
            g_pKato->Log(LOG_FAIL, TEXT("User reported decode video failed!\n."));
            test_fail = TRUE;
        }
    }
    mfc_render_final();
LEAVEREAD:  
    free(t007par);
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

// Test Case 1101: Basic Stream Driver Test
// Just open and close this driver. See if any problem in the operation.
TESTPROCAPI MFCStream(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{   
    void *MFCHandle;
    BOOL test_fail = FALSE;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    MFCHandle = CreateFile(L"MFC1:",
                            GENERIC_READ|GENERIC_WRITE, 0,
                            NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if( NULL == MFCHandle || INVALID_HANDLE_VALUE == MFCHandle ){
        g_pKato->Log(LOG_FAIL, TEXT("MFC Stream Interface :: CreateFile failed\r\n"));
        test_fail = TRUE;
    }
    else {
        g_pKato->Log(LOG_PASS, TEXT("MFC Driver Successfully Opened\n"));
        if( CloseHandle(MFCHandle) == 0 ) {
            g_pKato->Log(LOG_FAIL, TEXT("MFC Stream Interface :: Close failed\r\n"));
            test_fail = TRUE;
        }
    }

    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
} 

// Test Case 1102: Reentring Driver Test
// Create two MFC driver handlers and use IO control operation to verify those handlers
// are both work fine.
TESTPROCAPI MFCReentring(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    void *MFCHandle1, *MFCHandle2;
    BOOL test_fail = FALSE;
    MFC_ARGS            mfc_args;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    MFCHandle1 = CreateFile(L"MFC1:",
                            GENERIC_READ|GENERIC_WRITE, 0,
                            NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if( NULL == MFCHandle1 || INVALID_HANDLE_VALUE == MFCHandle1 ){
        g_pKato->Log(LOG_FAIL, TEXT("MFC Stream Interface[1] :: CreateFile failed\r\n"));
        return TPR_FAIL;
    }
    g_pKato->Log(LOG_PASS, TEXT("First MFC Driver Successfully Opened\n"));
    
    MFCHandle2 = CreateFile(L"MFC1:",
                            GENERIC_READ|GENERIC_WRITE, 0,
                            NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if( NULL == MFCHandle2 || INVALID_HANDLE_VALUE == MFCHandle2 ){
        g_pKato->Log(LOG_FAIL, TEXT("MFC Stream Interface[2] :: CreateFile failed\r\n"));
        if ( CloseHandle(MFCHandle1) != 0 ) {
            g_pKato->Log(LOG_FAIL, TEXT("Close Handle: MFCHandle1 failed!\n"));
        }
        return TPR_FAIL;
    }
    g_pKato->Log(LOG_PASS, TEXT("Second MFC Driver Successfully Opened\n"));
    
    if (DeviceIoControl(MFCHandle1, IOCTL_MFC_GET_CONFIG,
                            &mfc_args, sizeof(MFC_ARGS),
                            NULL, 0,
                            NULL,
                            NULL) == FALSE ) {
        g_pKato->Log(LOG_FAIL, TEXT("First MFC IOCTL call failed\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    g_pKato->Log(LOG_PASS, TEXT("First MFC IOCTL call passed\n"));
    if (DeviceIoControl(MFCHandle2, IOCTL_MFC_GET_CONFIG,
                            &mfc_args, sizeof(MFC_ARGS),
                            NULL, 0,
                            NULL,
                            NULL) == FALSE ) {
        g_pKato->Log(LOG_FAIL, TEXT("Second MFC IOCTL call failed\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    g_pKato->Log(LOG_PASS, TEXT("Second MFC IOCTL call passed\n"));
LEAVEREAD:     
    if ( CloseHandle(MFCHandle1) == 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: MFCHandle1 failed!\n"));
        test_fail = TRUE;
    }
    if ( CloseHandle(MFCHandle2) == 0 ) {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: MFCHandle2 failed!\n"));
        test_fail = TRUE;
    }
    
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

// Test Case 1103: IOCTL Test - IOCTL_MFC_GET_CONFIG.
// Test IO Control function via executing IOCTL_MFC_GET_CONFIG.
TESTPROCAPI MFCIOGet(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    void *MFCHandle;
    BOOL test_fail = FALSE;
    MFC_ARGS    mfc_args;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    MFCHandle = CreateFile(L"MFC1:",
                            GENERIC_READ|GENERIC_WRITE, 0,
                            NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if( NULL == MFCHandle || INVALID_HANDLE_VALUE == MFCHandle ){
        g_pKato->Log(LOG_FAIL, TEXT("MFC Stream Interface :: CreateFile failed\r\n"));
        return TPR_FAIL;
    }
    g_pKato->Log(LOG_PASS, TEXT("MFC Driver Successfully Opened\n"));
    mfc_args.get_config.in_config_param     = MFC_GET_CONFIG_DEC_FRAME_NEED_COUNT;
    mfc_args.get_config.out_config_value[0]  = 0;
    mfc_args.get_config.out_config_value[1]  = 0;
    if ( (DeviceIoControl(MFCHandle, IOCTL_MFC_GET_CONFIG,
                            &mfc_args, sizeof(MFC_ARGS),
                            NULL, 0,
                            NULL,
                            NULL) == FALSE) || (mfc_args.set_config.ret_code != 0) ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC TUX IOCTL GET_CONFIG failed\n"));
        test_fail = TRUE;
    }
    if ( CloseHandle(MFCHandle) == 0 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: MFCHandle failed!\n"));
        test_fail = TRUE;
    }
    
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

// Test Case 1104: IOCTL Test - IOCTL_MFC_SET_CONFIG.
// Test IO Control function via executing IOCTL_MFC_SET_CONFIG.
TESTPROCAPI MFCIOSet(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{ 
    void *MFCHandle;
    BOOL test_fail = FALSE;
    MFC_ARGS    mfc_args;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    MFCHandle = CreateFile(L"MFC1:",
                            GENERIC_READ|GENERIC_WRITE, 0,
                            NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if( ( NULL == MFCHandle || INVALID_HANDLE_VALUE == MFCHandle ) ){
        g_pKato->Log(LOG_FAIL, TEXT("MFC Stream Interface :: CreateFile failed\r\n"));
        return TPR_FAIL;
    }
    g_pKato->Log(LOG_PASS, TEXT("MFC Driver Successfully Opened\n"));
    mfc_args.set_config.in_config_param     = MFC_SET_CONFIG_DEC_OPTION;
    mfc_args.set_config.in_config_value[0]  = 0;
    mfc_args.set_config.in_config_value[1]  = 0;
    if ( (DeviceIoControl(MFCHandle, IOCTL_MFC_SET_CONFIG,
                            &mfc_args, sizeof(MFC_ARGS),
                            NULL, 0,
                            NULL,
                            NULL) == FALSE) || (mfc_args.set_config.ret_code != 0) ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC TUX IOCTL SET_CONFIG failed\n"));
        test_fail = TRUE;
    }
    if ( CloseHandle(MFCHandle) == 0 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: MFCHandle failed!\n"));
        test_fail = TRUE;
    }
    
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}


// Test Case 1601: Simultaneous encoding H.263, H.264 and MPEG-4
// Create three threads and encoding H.263, H.264 and MPEG-4 simultancously.
TESTPROCAPI MFCSimEnc(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
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
    
    if(uMsg != TPM_EXECUTE) {
        return TPR_NOT_HANDLED;
    }

    if( (pCmdData1 == NULL) && (pCmdData2 == NULL) && (pCmdData3 == NULL) ) {
        g_pKato->Log(LOG_FAIL, TEXT("Can not create new command data!\n"));
        return TPR_FAIL;
    }
    g_pKato->Log(LOG_COMMENT, TEXT("This test case simultaneously executes MFC encode!\n"));
    //Create and run threads
    pCmdData1->bTestResult=TRUE;
    pCmdData1->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("MFC_SimEnc263_Event"));
    if( NULL == pCmdData1->hWaitInvalidTest ) {
        g_pKato->Log(LOG_FAIL, TEXT("Create Event: Create Event1 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    hThread1 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_Enc263, (LPVOID)pCmdData1, 0,&IDThreadKey1);
    if( NULL == hThread1 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Create Thread: Create Thread1 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    dwWaitRet1 = WaitForSingleObject(pCmdData1->hWaitInvalidTest, TIMEOUT_CONSTANT);
    
    pCmdData2->bTestResult=TRUE;
    pCmdData2->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("MFC_SimEncMPEG4_Event"));
    if( NULL == pCmdData2->hWaitInvalidTest ) {
        g_pKato->Log(LOG_FAIL, TEXT("Create Event: Create Event2 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    hThread2 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_EncMPEG4, (LPVOID)pCmdData2, 0,&IDThreadKey2);
    if( NULL == hThread2 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Create Thread: Create Thread2 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    dwWaitRet2 = WaitForSingleObject(pCmdData2->hWaitInvalidTest, TIMEOUT_CONSTANT);
    
    pCmdData3->bTestResult=TRUE;
    pCmdData3->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("MFC_SimEnc264_Event"));
    if( NULL == pCmdData3->hWaitInvalidTest ) {
        g_pKato->Log(LOG_FAIL, TEXT("Create Event: Create Event3 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    hThread3 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_Enc264, (LPVOID)pCmdData3, 0,&IDThreadKey3);
    if( NULL == hThread3 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Create Thread: Create Thread3 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    dwWaitRet3 = WaitForSingleObject(pCmdData3->hWaitInvalidTest, TIMEOUT_CONSTANT);
    // see if timeout
    if ( WAIT_TIMEOUT == dwWaitRet1 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously encode H.263 timeout."));
        pCmdData1->bTestResult = FALSE;
        goto LEAVEREAD;
    } 
    if ( WAIT_TIMEOUT == dwWaitRet2 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously encode MPEG4 timeout."));
        pCmdData2->bTestResult = FALSE;
        goto LEAVEREAD;
    }
    if ( WAIT_TIMEOUT == dwWaitRet3 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously encode H.264 timeout."));
        pCmdData3->bTestResult = FALSE;
    }

LEAVEREAD: 
    if ( (NULL != pCmdData1->hWaitInvalidTest) && (CloseHandle(pCmdData1->hWaitInvalidTest) == 0)  )    {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: pCmdData1 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != hThread1) && (CloseHandle(hThread1) == 0) )   {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: Thread1 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != pCmdData2->hWaitInvalidTest) && (CloseHandle(pCmdData2->hWaitInvalidTest) == 0)  )    {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: pCmdData2 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != hThread2) && (CloseHandle(hThread2) == 0) )   {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: Thread2 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != pCmdData3->hWaitInvalidTest) && (CloseHandle(pCmdData3->hWaitInvalidTest) == 0)  )    {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: pCmdData3 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != hThread3) && (CloseHandle(hThread3) == 0) )   {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: Thread3 failed!\n"));
        test_fail = TRUE;
    }
    if ( ((pCmdData1->bTestResult == TRUE) && (pCmdData2->bTestResult == TRUE)) && (pCmdData3->bTestResult == TRUE) ) {
        g_pKato->Log(LOG_PASS, TEXT("MFC simultaneously encode test succeeded"));
    } 
    else {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously encode test failed"));
        test_fail = TRUE;
    }
    delete pCmdData1;
    delete pCmdData2;
    delete pCmdData3;
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

// Test Case 1602: Simultaneous encoding H.263, H.264 and decoding MPEG-4, VC-1.
// Create three threads and run Encode H.263, Decode MPEG4 and Encode H.264 simulataneously.
TESTPROCAPI MFCSimCODECA(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
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
    
    if(uMsg != TPM_EXECUTE) {
        return TPR_NOT_HANDLED;
    }

    if( (pCmdData1 == NULL) && (pCmdData2 == NULL) && (pCmdData3 == NULL) ) {
        g_pKato->Log(LOG_FAIL, TEXT("Can not create new command data!\n"));
        return TPR_FAIL;
    }
    g_pKato->Log(LOG_COMMENT, TEXT("This test case simultaneously executes MFC encode/decode - A suit!\n"));
    // Create threads and run
    pCmdData1->bTestResult=TRUE;
    pCmdData1->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("MFC_SimEnc263_Event"));
    if( NULL == pCmdData1->hWaitInvalidTest ) {     
        g_pKato->Log(LOG_FAIL, TEXT("Create Event: Create Event1 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    hThread1 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_Enc263, (LPVOID)pCmdData1, 0,&IDThreadKey1);    
    if( NULL == hThread1 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Create Thread: Create Thread1 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    dwWaitRet1 = WaitForSingleObject(pCmdData1->hWaitInvalidTest, TIMEOUT_CONSTANT); 
    
    pCmdData2->bTestResult=TRUE;
    pCmdData2->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("MFC_SimDecMPEG4_Event"));
    if( NULL == pCmdData2->hWaitInvalidTest ) {
        g_pKato->Log(LOG_FAIL, TEXT("Create Event: Create Event2 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    hThread2 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_DecMPEG4, (LPVOID)pCmdData2, 0,&IDThreadKey2);
    if( NULL == hThread2 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Create Thread: Create Thread2 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    dwWaitRet2 = WaitForSingleObject(pCmdData2->hWaitInvalidTest, TIMEOUT_CONSTANT);
    
    pCmdData3->bTestResult=TRUE;
    pCmdData3->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("MFC_SimEnc264_Event"));
    if( NULL == pCmdData3->hWaitInvalidTest ) {
        g_pKato->Log(LOG_FAIL, TEXT("Create Event: Create Event3 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    hThread3 = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_Enc264, (LPVOID)pCmdData3, 0,&IDThreadKey3);
    if( NULL == hThread3 )  {
        g_pKato->Log(LOG_FAIL, TEXT("Create Thread: Create Thread3 failed!\n"));
        test_fail = TRUE;
        goto LEAVEREAD;
    }
    dwWaitRet3 = WaitForSingleObject(pCmdData3->hWaitInvalidTest, TIMEOUT_CONSTANT);
    // see if timeout
    if ( WAIT_TIMEOUT == dwWaitRet1 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously encode H.263 timeout."));
        pCmdData1->bTestResult = FALSE;
        goto LEAVEREAD;
    } 
    if ( WAIT_TIMEOUT == dwWaitRet2 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously decode MPEG4 timeout."));
        pCmdData2->bTestResult = FALSE;
        goto LEAVEREAD;
    }
    if ( WAIT_TIMEOUT == dwWaitRet3 ) {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously encode H.264 timeout."));
        pCmdData3->bTestResult = FALSE;
    }

LEAVEREAD: 
    if ( (NULL != pCmdData1->hWaitInvalidTest) && (CloseHandle(pCmdData1->hWaitInvalidTest) == 0)  )    {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: pCmdData1 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != hThread1) && (CloseHandle(hThread1) == 0) )   {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: Thread1 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != pCmdData2->hWaitInvalidTest) && (CloseHandle(pCmdData2->hWaitInvalidTest) == 0)  )    {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: pCmdData2 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != hThread2) && (CloseHandle(hThread2) == 0) )   {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: Thread2 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != pCmdData3->hWaitInvalidTest) && (CloseHandle(pCmdData3->hWaitInvalidTest) == 0)  )    {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: pCmdData3 failed!\n"));
        test_fail = TRUE;
    }
    if ( (NULL != hThread3) && (CloseHandle(hThread3) == 0) )   {
        g_pKato->Log(LOG_FAIL, TEXT("Close Handle: Thread3 failed!\n"));
        test_fail = TRUE;
    }
    if ( ((pCmdData1->bTestResult == TRUE) && (pCmdData2->bTestResult == TRUE)) && 
            (pCmdData3->bTestResult == TRUE)  ) {       
        g_pKato->Log(LOG_PASS, TEXT("MFC simultaneously encode/decode test succeeded - suit A"));
    } 
    else {
        g_pKato->Log(LOG_FAIL, TEXT("MFC simultaneously encode/decode test failed - suit A"));
        test_fail = TRUE;
    }
    delete pCmdData1;
    delete pCmdData2;
    delete pCmdData3;
    return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}
