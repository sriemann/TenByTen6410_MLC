//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
////////////////////////////////////////////////////////////////////////////////
//
//  MfcE2E TUX DLL
//
//  Module: test.cpp
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



#define RAW_VIDEO_NAND_PATH    "\\NandFlash\\MfcE2E.yuv"
#define RAW_VIDEO_USB_PATH      "\\Hard Disk\\MfcE2E.yuv"
#define RAW_VIDEO_SD_PATH       "\\Storage Card\\MfcE2E.yuv"

#define H264_VIDEO_NAND_PATH   "\\NandFlash\\MfcE2E.264"
#define H263_VIDEO_USB_PATH     "\\Hard Disk\\MfcE2E.263"
#define MP4_VIDEO_SD_PATH       "\\Storage Card\\MfcE2E.m4v"
#define VC1_VIDEO_SD_PATH       "\\Storage Card\\MfcE2E.rcv"
#define H264_VIDEO_NAND_PATH2   "\\NandFlash\\MfcE2E2.264"
#define H263_VIDEO_USB_PATH2     "\\Hard Disk\\MfcE2E2.263"
#define MP4_VIDEO_SD_PATH2       "\\Storage Card\\MfcE2E2.m4v"


#define VIDEO_YUV_W		320
#define VIDEO_YUV_H		240
#define VIDEO_YUV_FR	15
#define VIDEO_YUV_BR	1000
#define VIDEO_YUV_GOP	15
#define VIDEO_YUV_IQ	10
#define VIDEO_YUV_QP	10
#define VIDEO_YUV_GA	0.75

#define NUM_PIP_VIDEOS		1
#define TIMEOUT_CONSTANT	30000

typedef struct
{
   HANDLE hEvent;
   LPCTSTR *pszFileNames;
   CODEC_MODE *codec_modes;
   char* olist;
}ENCODE_PARA,*PENCODE_PARA;

BOOL Encode(LPCTSTR *pszFileNames,CODEC_MODE *codec_modes,char* olist);


BOOL  ThreadProc_EncodeThread(LPVOID lpParameter)
{

   PENCODE_PARA pEncodePara = (PENCODE_PARA) lpParameter;
   Encode(pEncodePara->pszFileNames,pEncodePara->codec_modes,pEncodePara->olist);
   SetEvent(pEncodePara->hEvent);
   return 0; 
}



BOOL Encode(LPCTSTR *pszFileNames,CODEC_MODE *codec_modes,char* olist)
{
	int forceExit=0;
	ENC_PARAINFO video_para;
	TUX_MFC_PARA *t003par;

	HWND hWnd;
	
	BOOL test_fail = FALSE;
	

	t003par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	t003par->infilename   = pszFileNames;
	t003par->outfilename  = olist;
	t003par->codec_mode   = codec_modes;
	t003par->outDISP      = FALSE;
	t003par->num_pips     = NUM_PIP_VIDEOS;
    video_para.width      =  VIDEO_YUV_W;    
	video_para.height     = VIDEO_YUV_H;
	video_para.frame_rate = VIDEO_YUV_FR;
	video_para.bitrate    = VIDEO_YUV_BR;
	video_para.gop_num    = VIDEO_YUV_GOP;
	video_para.intraqp    = VIDEO_YUV_IQ;
	video_para.qpmax      = VIDEO_YUV_QP;
	video_para.gamma      = VIDEO_YUV_GA;

	hWnd = GetForegroundWindow();
	if( hWnd == NULL  ) {
		g_pKato->Log(LOG_COMMENT, TEXT("Init MFC Encode test case faild\n"));
		free(t003par);
		test_fail = TRUE;
		goto Exit;
	}
	g_pKato->Log(LOG_COMMENT, TEXT("Encode video and save as file."));
	if (mfcenc_basic(video_para,t003par,&forceExit) < 0 ) {    
	    g_pKato->Log(LOG_COMMENT, TEXT("MFC Encode test case faild\n"));
	    test_fail = TRUE;
		goto Exit;
	}
	else 
	{
		g_pKato->Log(LOG_COMMENT, TEXT("MFC Encode test case finished\n"));
		
	}	
     

Exit:
	free(t003par);
	return !test_fail;
}

BOOL Decode(LPCTSTR *pszOutputFileNames,CODEC_MODE *codec_modes)
{
		

	int forceExit2=0;
	
	TUX_MFC_PARA *t006par;
	HWND hWnd;
	int user_ret;
	BOOL test_fail = FALSE;

	
    g_pKato->Log(LOG_COMMENT, TEXT("Decode video and show on LCD."));
	
    t006par = (TUX_MFC_PARA*)malloc(sizeof(TUX_MFC_PARA));
	t006par->infilename = pszOutputFileNames;
	t006par->outfilename = NULL;
	t006par->codec_mode = codec_modes;
	t006par->outDISP = TRUE;
	t006par->num_pips = NUM_PIP_VIDEOS;
	 
	hWnd = GetForegroundWindow();
	if( (hWnd == NULL) || (mfc_render_init(hWnd) == FALSE) ) {
		g_pKato->Log(LOG_COMMENT, TEXT("Init MFC Decode test case faild\n"));
		free(t006par);
		test_fail = TRUE;
		goto Exit;
	}
	if (mfcdec_basic(t006par,&forceExit2) < 0 ) {
	    g_pKato->Log(LOG_COMMENT, TEXT("MFC Decode test case faild\n"));
	    test_fail = TRUE;
	}
	else {
		g_pKato->Log(LOG_COMMENT, TEXT("MFC Decode test case finished\n"));
		user_ret = MessageBox(hWnd, (LPCWSTR)L"Do you see this video correctly!", 
								(LPCWSTR)L"MFC TUX Result", MB_YESNO);
		if( user_ret != IDYES ){
			g_pKato->Log(LOG_COMMENT, TEXT("User reported decode video failed!\n."));
			test_fail = TRUE;
		}
	}
	mfc_render_final();

	

Exit:
	free(t006par);

	return !test_fail;
}



////////////////////////////////////////////////////////////////////////////////
// MFC_H264_NAND_E2E
//  MFC H.264 and NAND flash E2E test
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI MFC_H264_NAND_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	BOOL test_fail = FALSE;
	LPCTSTR pszFileNames[] ={_T(RAW_VIDEO_NAND_PATH)};
	LPCTSTR pszOutputFileNames[]={_T(H264_VIDEO_NAND_PATH)};
	LPCTSTR pszOutputFileNames2[]={_T(H264_VIDEO_NAND_PATH2)};
    CODEC_MODE codec_modes[] = {CODEC_H264};
	char* olist[] = {H264_VIDEO_NAND_PATH};
	char* olist2[] = {H264_VIDEO_NAND_PATH2};
    ENCODE_PARA *pENCODE_PARA=new ENCODE_PARA;
	DWORD IDThreadKey;
	HANDLE hThread;
	DWORD dwWaitRet;
    FILE *fp;
	HWND hWnd;

	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
	if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    
	g_pKato->Log(LOG_COMMENT, TEXT("MFC H.264 and NAND flash E2E test"));
	
	hWnd = GetForegroundWindow();
	if( hWnd == NULL  ) {
		g_pKato->Log(LOG_COMMENT, TEXT("Init MFC E2E test case faild. Can not find Foreground window."));
		test_fail = TRUE;
		goto Exit;
	}
	
	MessageBox(hWnd,TEXT("Please copy raw data file to NAND flash. We will start MFC H264 and NAND flash E2E test."),TEXT("MFC E2E test"),MB_OK);
	
	fp = fopen(RAW_VIDEO_NAND_PATH, "rb");
	if(fp == NULL){
		MessageBox(hWnd,TEXT("File not found! Please copy raw data file to NAND flash."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}

	if(Encode((LPCTSTR *)pszFileNames,codec_modes,olist[0]))
	{
		
		if(Decode((LPCTSTR *)pszOutputFileNames,codec_modes))
		{
            pENCODE_PARA->pszFileNames=pszFileNames;
	        pENCODE_PARA->codec_modes=codec_modes;
            pENCODE_PARA->olist=olist2[0];
			pENCODE_PARA->hEvent=CreateEvent(0,FALSE,FALSE,_T("EncodeDone"));

            hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_EncodeThread, (LPVOID)pENCODE_PARA, 0,&IDThreadKey);
            if(!Decode((LPCTSTR *)pszOutputFileNames,codec_modes))
		    {
                test_fail=TRUE;
			}
			dwWaitRet=WaitForSingleObject(pENCODE_PARA->hEvent, TIMEOUT_CONSTANT);
            if ( WAIT_TIMEOUT == dwWaitRet )
            {
                g_pKato->Log(LOG_COMMENT, TEXT("Encode timeout."));
		        test_fail=TRUE;
             
            } 
			else if(!test_fail)
			{
                 if(!Decode((LPCTSTR *)pszOutputFileNames2,codec_modes))
		         {
                     test_fail=TRUE;
			     }
			}
 
		}
		else
		{
            test_fail=TRUE;
		}
	}
	else
	{
       test_fail=TRUE;
	}

Exit:
	if(test_fail == TRUE)
	{
        MessageBox(hWnd,TEXT("MFC H.264 and NAND flash E2E test failed."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_FAIL, TEXT("MFC H.264 and NAND flash E2E test failed."));
	}
	else
	{
        MessageBox(hWnd,TEXT("MFC H.264 and NAND flash E2E test succeeded."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_PASS, TEXT("MFC H.264 and NAND flash E2E test succeeded."));
	}
	if(hThread !=NULL)
	{
		CloseHandle(hThread);
	}
	if(pENCODE_PARA->hEvent!=NULL)
	{
        CloseHandle(pENCODE_PARA->hEvent);
	}

    delete pENCODE_PARA;

	return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;

	
}

////////////////////////////////////////////////////////////////////////////////
// MFC_H263_USB_E2E
//  MFC H.263 and USB Mass Storage E2E test
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI MFC_H263_USB_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	BOOL test_fail = FALSE;
	LPCTSTR pszFileNames[] ={_T(RAW_VIDEO_USB_PATH)};
	LPCTSTR pszOutputFileNames[]={_T(H263_VIDEO_USB_PATH)};
	LPCTSTR pszOutputFileNames2[]={_T(H263_VIDEO_USB_PATH2)};
    CODEC_MODE codec_modes[] = {CODEC_H263};
	char* olist[] = {H263_VIDEO_USB_PATH};
	char* olist2[] = {H263_VIDEO_USB_PATH2};
    ENCODE_PARA *pENCODE_PARA=new ENCODE_PARA;
	DWORD IDThreadKey;
	HANDLE hThread;
	DWORD dwWaitRet;
    FILE *fp;
	HWND hWnd;

	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
	if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    
	g_pKato->Log(LOG_COMMENT, TEXT("MFC H.263 and USB mass storage E2E test"));
	
	hWnd = GetForegroundWindow();
	if( hWnd == NULL  ) {
		g_pKato->Log(LOG_COMMENT, TEXT("Init MFC E2E test case faild. Can not find foreground window."));
		test_fail = TRUE;
		goto Exit;
	}
	
	MessageBox(hWnd,TEXT("Please insert USB storage with raw data file. We will start MFC H263 and USB mass storage E2E test."),TEXT("MFC E2E test"),MB_OK);
	
	fp = fopen(RAW_VIDEO_USB_PATH, "rb");
	if(fp == NULL){
		MessageBox(hWnd,TEXT("File not found! Please insert USB storage with raw data file."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}

	if(Encode((LPCTSTR *)pszFileNames,codec_modes,olist[0]))
	{
		
		if(Decode((LPCTSTR *)pszOutputFileNames,codec_modes))
		{
            pENCODE_PARA->pszFileNames=pszFileNames;
	        pENCODE_PARA->codec_modes=codec_modes;
            pENCODE_PARA->olist=olist2[0];
			pENCODE_PARA->hEvent=CreateEvent(0,FALSE,FALSE,_T("EncodeDone"));

            hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_EncodeThread, (LPVOID)pENCODE_PARA, 0,&IDThreadKey);
            if(!Decode((LPCTSTR *)pszOutputFileNames,codec_modes))
		    {
                test_fail=TRUE;
			}
			dwWaitRet=WaitForSingleObject(pENCODE_PARA->hEvent, TIMEOUT_CONSTANT);
            if ( WAIT_TIMEOUT == dwWaitRet )
            {
                g_pKato->Log(LOG_COMMENT, TEXT("Encode timeout."));
		        test_fail=TRUE;
             
            } 
			else if(!test_fail)
			{
                 if(!Decode((LPCTSTR *)pszOutputFileNames2,codec_modes))
		         {
                     test_fail=TRUE;
			     }
			}
 
		}
		else
		{
            test_fail=TRUE;
		}
	}
	else
	{
       test_fail=TRUE;
	}

Exit:
	if(test_fail == TRUE)
	{
        MessageBox(hWnd,TEXT("MFC H.263 and USB mass storage E2E test failed."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_FAIL, TEXT("MFC H.263 and USB mass storage E2E test failed."));
	}
	else
	{
        MessageBox(hWnd,TEXT("MFC H.263 and USB mass storage E2E test succeeded."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_PASS, TEXT("MFC H.263 and USB mass storage E2E test succeeded."));
	}
	if(hThread !=NULL)
	{
		CloseHandle(hThread);
	}
	if(pENCODE_PARA->hEvent!=NULL)
	{
        CloseHandle(pENCODE_PARA->hEvent);
	}

    delete pENCODE_PARA;

	return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;


}


////////////////////////////////////////////////////////////////////////////////
// MFC_MPEG4_SD_E2E
//  MFC MPEG4 and SD card E2E test
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI MFC_MPEG4_SD_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	BOOL test_fail = FALSE;
	LPCTSTR pszFileNames[] ={_T(RAW_VIDEO_SD_PATH)};
	LPCTSTR pszOutputFileNames[]={_T(MP4_VIDEO_SD_PATH)};
	LPCTSTR pszOutputFileNames2[]={_T(MP4_VIDEO_SD_PATH2)};
    CODEC_MODE codec_modes[] = {CODEC_MPEG4};
	char* olist[] = {MP4_VIDEO_SD_PATH};
	char* olist2[] = {MP4_VIDEO_SD_PATH2};
    ENCODE_PARA *pENCODE_PARA=new ENCODE_PARA;
	DWORD IDThreadKey;
	HANDLE hThread;
	DWORD dwWaitRet;
    FILE *fp;
	HWND hWnd;

	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
	if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    
	g_pKato->Log(LOG_COMMENT, TEXT("MFC MPEG4 and SD card E2E test"));
	
	hWnd = GetForegroundWindow();
	if( hWnd == NULL  ) {
		g_pKato->Log(LOG_COMMENT, TEXT("Init MFC E2E test case faild. Can not find Foreground window."));
		test_fail = TRUE;
		goto Exit;
	}
	
	MessageBox(hWnd,TEXT("Please insert SD card with raw data file. We will start MFC MPEG4 and SD card E2E test."),TEXT("MFC E2E test"),MB_OK);
	
	fp = fopen(RAW_VIDEO_SD_PATH, "rb");
	if(fp == NULL){
		MessageBox(hWnd,TEXT("File not found! Please insert SD card with raw data file."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}

	if(Encode((LPCTSTR *)pszFileNames,codec_modes,olist[0]))
	{
		
		if(Decode((LPCTSTR *)pszOutputFileNames,codec_modes))
		{
            pENCODE_PARA->pszFileNames=pszFileNames;
	        pENCODE_PARA->codec_modes=codec_modes;
            pENCODE_PARA->olist=olist2[0];
			pENCODE_PARA->hEvent=CreateEvent(0,FALSE,FALSE,_T("EncodeDone"));

            hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_EncodeThread, (LPVOID)pENCODE_PARA, 0,&IDThreadKey);
            if(!Decode((LPCTSTR *)pszOutputFileNames,codec_modes))
		    {
                test_fail=TRUE;
			}
			dwWaitRet=WaitForSingleObject(pENCODE_PARA->hEvent, TIMEOUT_CONSTANT);
            if ( WAIT_TIMEOUT == dwWaitRet )
            {
                g_pKato->Log(LOG_COMMENT, TEXT("Encode timeout."));
		        test_fail=TRUE;
             
            } 
			else if(!test_fail)
			{
                 if(!Decode((LPCTSTR *)pszOutputFileNames2,codec_modes))
		         {
                     test_fail=TRUE;
			     }
			}
 
		}
		else
		{
            test_fail=TRUE;
		}
	}
	else
	{
       test_fail=TRUE;
	}

Exit:
	if(test_fail == TRUE)
	{
        MessageBox(hWnd,TEXT("MFC MPEG4 and SD card E2E test failed."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_FAIL, TEXT("MFC MPEG4 and SD card E2E test failed."));
	}
	else
	{
        MessageBox(hWnd,TEXT("MFC MPEG4 and SD card E2E test succeeded."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_PASS, TEXT("MFC MPEG4 and SD card E2E test succeeded."));
	}
	if(hThread !=NULL)
	{
		CloseHandle(hThread);
	}
	if(pENCODE_PARA->hEvent!=NULL)
	{
        CloseHandle(pENCODE_PARA->hEvent);
	}

    delete pENCODE_PARA;

	return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

////////////////////////////////////////////////////////////////////////////////
// MFC_VC1_SD_E2E
//  MFC VC1 and SD card E2E test
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI MFC_VC1_SD_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	BOOL test_fail = FALSE;
	LPCTSTR pszFileNames[] ={_T(RAW_VIDEO_SD_PATH)};
	LPCTSTR pszOutputFileNames[]={_T(VC1_VIDEO_SD_PATH)};
    CODEC_MODE codec_modes[] = {CODEC_VC1};
	char* olist[] = {VC1_VIDEO_SD_PATH};
	HWND hWnd;
	FILE *fp;
    

	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
	if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
    g_pKato->Log(LOG_COMMENT, TEXT("MFC VC1 and SD card E2E test"));

	hWnd = GetForegroundWindow();
	if( hWnd == NULL  ) {
		g_pKato->Log(LOG_COMMENT, TEXT("Init MFC E2E test case faild. Can not find Foreground window."));
		test_fail = TRUE;
		goto Exit;
	}
	
	MessageBox(hWnd,TEXT("Please insert SD card with VC1 file. We will start MFC VC1 and SD card E2E test."),TEXT("MFC E2E test"),MB_OK);
	
	fp = fopen(RAW_VIDEO_SD_PATH, "rb");
	if(fp == NULL){
		MessageBox(hWnd,TEXT("File not found! Please insert SD card with VC1 file."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}
	//test_fail=!EncodeDecode((LPCTSTR *)pszFileNames,(LPCTSTR *)pszOutputFileNames,codec_modes,olist[0]);
	test_fail=!Decode((LPCTSTR *)pszOutputFileNames,codec_modes);

Exit:
	if(test_fail == TRUE)
	{
        MessageBox(hWnd,TEXT("MFC VC1 and SD card E2E test failed."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_FAIL, TEXT("MFC VC1 and SD card E2E test failed."));
	}
	else
	{
        MessageBox(hWnd,TEXT("MFC VC1 and SD card E2E test succeeded."),TEXT("MFC E2E test"),MB_OK);
		g_pKato->Log(LOG_PASS, TEXT("MFC VC1 and SD card E2E test succeeded."));
	}

	return (test_fail == TRUE) ? TPR_FAIL:TPR_PASS;
}

////////////////////////////////////////////////////////////////////////////////
