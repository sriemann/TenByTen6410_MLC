//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//  JPEG_TUX TUX DLL
//
//  Module: JPEG_TUX.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>
#include <tux.h>
#include <kato.h>
#include "main.h"
#include "globals.h"
#include "s3c6410_display_con.h"
#include "S3c6410_post_proc.h"
#include "SVE_API.h"
#include "JPGDrvHandler.h"


#define LCD_X		800
#define LCD_Y		480
#define TIMEOUT_CONSTANT	10000
#define IOCTL_JPG_GET_STRBUF		CTL_CODE( 0, 0x812, 0, 0 )

void* GetJPGDrvPtrDec(void);
void* GetJPGDrvPtrEnc(void);
void DisplayJPEG (int , INT32 , INT32 , INT32 , INT32 ,int );
int MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType); 


typedef struct
{
   HANDLE hWaitInvalidTest;
   bool bTestResult;
}CMD_DATA,*PCMD_DATA;

typedef struct para_TUX_JPEG
{
	wchar_t *infilename;
	wchar_t *outfilename;
	BOOL fpt;	//for performance test
	BOOL do_thumb;
	BOOL may_oversize;
	UINT32 ptime;	//performance time
	INT32 inwidth;
	INT32 inheight;
	SAMPLE_MODE_T samplemode;
	IMAGE_QUALITY_TYPE_T imgquality;
}TUX_JPEG_PARA;

// Examine whole buffer value, if whole buffer is zero
// this function will return TRUE.
// Parameter: ptr, pointer of buffer
//			  prtsz, pointer size
// Return: TRUE, whole buffer is zero, 
//		   FALSE, buffer content non-zero value
unsigned char ZeroChecker(char *ptr, int prtsz)
{
	int i = 0;
	
	while( i < prtsz  ) {
		if ( (char*)(ptr+i) != 0	) {
			return FALSE; //Safe, non-zero
		}
		i++;
	}
	return TRUE;
}
// makeExifParam:
// Fill up EXIF content, just a fixed value
void makeExifParam(ExifFileInfo *exifFileInfo)
{
	strcpy(exifFileInfo->Make,"Samsung SYS.LSI make");;
	strcpy(exifFileInfo->Model,"Samsung 2007 model");
	strcpy(exifFileInfo->Version,"version 1.0.2.0");
	strcpy(exifFileInfo->DateTime,"2007:05:16 12:32:54");
	strcpy(exifFileInfo->CopyRight,"Samsung Electronics@2007:All rights reserved");

	exifFileInfo->Height					= 320;
	exifFileInfo->Width						= 240;
	exifFileInfo->Orientation				= 1; // top-left
	exifFileInfo->ColorSpace				= 1;
	exifFileInfo->Process					= 1;
	exifFileInfo->Flash						= 0;
	exifFileInfo->FocalLengthNum			= 1;
	exifFileInfo->FocalLengthDen			= 4;
	exifFileInfo->ExposureTimeNum			= 1;
	exifFileInfo->ExposureTimeDen			= 20;
	exifFileInfo->FNumberNum				= 1;
	exifFileInfo->FNumberDen				= 35;
	exifFileInfo->ApertureFNumber			= 1;
	exifFileInfo->SubjectDistanceNum		= -20;
	exifFileInfo->SubjectDistanceDen		= -7;
	exifFileInfo->CCDWidth					= 1;
	exifFileInfo->ExposureBiasNum			= -16;
	exifFileInfo->ExposureBiasDen			= -2;
	exifFileInfo->WhiteBalance				= 6;
	exifFileInfo->MeteringMode				= 3;
	exifFileInfo->ExposureProgram			= 1;
	exifFileInfo->ISOSpeedRatings[0]		= 1;
	exifFileInfo->ISOSpeedRatings[1]		= 2;
	exifFileInfo->FocalPlaneXResolutionNum	= 65;
	exifFileInfo->FocalPlaneXResolutionDen	= 66;
	exifFileInfo->FocalPlaneYResolutionNum	= 70;
	exifFileInfo->FocalPlaneYResolutionDen	= 71;
	exifFileInfo->FocalPlaneResolutionUnit	= 3;
	exifFileInfo->XResolutionNum			= 48;
	exifFileInfo->XResolutionDen			= 20;
	exifFileInfo->YResolutionNum			= 48;
	exifFileInfo->YResolutionDen			= 20;
	exifFileInfo->RUnit						= 2;
	exifFileInfo->BrightnessNum				= -7;
	exifFileInfo->BrightnessDen				= 1;

	strcpy(exifFileInfo->UserComments,"Usercomments");
}
// Decompress JPEG files and show on LCD.
TESTPROCAPI JPEGDecExe(TUX_JPEG_PARA *fdin_str)
{
	FILE *fp;
	void *handle;
	UINT32 fileSize;
	char *InBuf = NULL;
	char *OutBuf = NULL;
	char *OutPhyBuf = NULL;
	char *OutRGBPhyBuf = NULL;
	long decSize;
	JPEG_ERRORTYPE ret;
	UINT32 width, height, samplemode;
	UINT32	decodeTime = 0;

	//===== 1. Open JPEG file =====		
	fp = _wfopen(fdin_str->infilename, L"rb");
	if(fp == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("file open error!\n"));
		return TPR_FAIL;
	}
	handle = GetJPGDrvPtrDec();
	if(handle == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("Get JPEG driver handle pointer failed\n"));
		fclose(fp);
		return TPR_FAIL;
	}
	
	//===== 2. figure out file size =====	
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	//===== 3. Buffer control, move from file to memory =====
	InBuf = (char*)GetDecInBuf(handle,fileSize);
	if(InBuf == NULL){
		fdin_str->may_oversize = TRUE;
		g_pKato->Log(LOG_FAIL, TEXT("Input buffer is NULL"));
		fclose(fp);
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}
	fread(InBuf, 1, fileSize, fp);
	fclose(fp);
	//===== 4. Decompress JPEG =====
	if( fdin_str->fpt == TRUE )	{ // log system tick for performance tset
		fdin_str->ptime = GetTickCount();		
	}	
	ret = ExeJPEGDec(handle);
	if( fdin_str->fpt == TRUE )	{ // log system tick for performance tset
		decodeTime = GetTickCount();
		fdin_str->ptime = decodeTime - fdin_str->ptime;
	}	
	if(ret != JPEG_OK){
		g_pKato->Log(LOG_FAIL, TEXT("Decoding failed\n"));
		fdin_str->may_oversize = TRUE;
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}
	//===== 5. Get out buffer =====
	OutBuf = (char*)GetDecOutBuf(handle,&decSize);
	GetJPEGInfo(JPEG_GET_DECODE_WIDTH, &width);
	GetJPEGInfo(JPEG_GET_DECODE_HEIGHT, &height);
	GetJPEGInfo(JPEG_GET_SAMPING_MODE, &samplemode);
	g_pKato->Log(LOG_COMMENT, TEXT("Decode image info: width : %d height : %d samplemode : %d\n\n"), width, height, samplemode);
	OutPhyBuf = (char*)GetJPEGDecOutPhyBuf(handle);
	OutRGBPhyBuf = (char*)GetJPEGRGBPhyBuf(handle,LCD_X, LCD_Y);
	if(ConvertYCBYCRToRGB((int)OutPhyBuf, width, height, 
							  POST_SRC_YUV422_CRYCBY,
							  (int)OutRGBPhyBuf, LCD_X, LCD_Y,
							  POST_DST_RGB16) == FALSE){
			g_pKato->Log(LOG_FAIL, TEXT("ConvertYCBYCRToRGB error\n"));
			DeinitJPEGDec(handle);
			return TPR_FAIL;
	}
	// Show on LCD
	DisplayJPEG((int)OutRGBPhyBuf, LCD_X, LCD_Y, LCD_X, LCD_Y, 5);
	DeinitJPEGDec(handle);

	return TPR_PASS;
}
// Compress YUV file into JPEG file.
TESTPROCAPI JPEGComExe(TUX_JPEG_PARA *fcin_str)
{
	void *handle;
	JPEG_ERRORTYPE ret;
	BOOL result = TRUE;
	FILE *fp;
	char *InBuf = NULL;
	char *OutBuf = NULL;
	long fileSize;
	long frameSize;
	ExifFileInfo *ExifInfo;
	INT32	encodeTime = 0;

	//===== 1. Open YUV file =====	
	fp = _wfopen(fcin_str->infilename, L"rb");
	if(fp == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("file open error : %s\n"), fcin_str->infilename);
		return TPR_FAIL;
	}	
	handle = GetJPGDrvPtrEnc();	// Get JPEG driver handler
	if ( NULL == handle ) {
		g_pKato->Log(LOG_FAIL, TEXT("Get JPEG driver handle pointer failed\n"));
		fclose(fp);
		return TPR_FAIL;
	}
	if((ret = SetJPGConfig(JPEG_SET_SAMPING_MODE, fcin_str->samplemode)) != JPEG_OK){
		result = FALSE;
	}
	if((ret = SetJPGConfig(JPEG_SET_ENCODE_WIDTH, fcin_str->inwidth)) != JPEG_OK){
		result = FALSE;
	}
	if((ret = SetJPGConfig(JPEG_SET_ENCODE_HEIGHT, fcin_str->inheight)) != JPEG_OK){
		result = FALSE;
	}
	if((ret = SetJPGConfig(JPEG_SET_ENCODE_QUALITY, fcin_str->imgquality)) != JPEG_OK){
		result = FALSE;
	}
	if( result == FALSE ) {
		g_pKato->Log(LOG_FAIL, TEXT("Fail in seting JPG Compress Config\n"));
		fcin_str->may_oversize = TRUE;
		fclose(fp);
		return TPR_FAIL;
	}
	//===== 2. figure out file size =====
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	InBuf = (char*)GetEncInBuf(handle, fileSize);
	if(InBuf == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("In Buffer is NULL, failed!\n"));
		fclose(fp);
		DeInitJPEGEnc(handle);
		return TPR_FAIL;
	}
	fread(InBuf, 1, fileSize, fp);
	fclose(fp);
	
	ExifInfo = (ExifFileInfo *)malloc(sizeof(ExifFileInfo));
	memset(ExifInfo, 0x00, sizeof(ExifFileInfo));
	makeExifParam(ExifInfo);	// Create EXIF information 
	if( fcin_str->fpt == TRUE )	{ // log system tick for performance tset
		fcin_str->ptime = GetTickCount();
	}
	//===== 4. Compress YUV =====
	ret = ExeJPEGEnc(handle, ExifInfo);
	if( fcin_str->fpt == TRUE )	{ // log system tick for performance tset
		encodeTime = GetTickCount();
		fcin_str->ptime = encodeTime - fcin_str->ptime;
	}	
	if(ret != JPEG_OK){
		g_pKato->Log(LOG_FAIL, TEXT("Execute JPEG Compress failed!\n"));
		free(ExifInfo);
		DeInitJPEGEnc(handle);
		return TPR_FAIL;
	}
	//===== 5. Get out buffer  =====
	OutBuf = (char*)GetEncOutBuf(handle, &frameSize);
	if(OutBuf == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("Failed, Out buffer is NULL!\n"));
		free(ExifInfo);
		DeInitJPEGEnc(handle);
		return TPR_FAIL;
	}
	fp = _wfopen(fcin_str->outfilename, L"wb");
	fwrite(OutBuf, 1, frameSize, fp);
	fclose(fp);
	
	DeInitJPEGEnc(handle);
		if(ExifInfo != NULL)
			free(ExifInfo);
	
	if(result == FALSE){
		g_pKato->Log(LOG_FAIL, TEXT("Return will be FALSE!\n"));	
		
		return TPR_FAIL;	
	}
	return TPR_PASS;
}
// Thread of test case 1104, this thread try to open JPEG driver with 
// negative value, maximum value.
#define INVALID_API_NEG	-1
#define INVALID_API_MAX	0xFFFFFFFF
DWORD  ThreadProc_InvalidOpen(LPVOID lpParameter)
{
    HANDLE hDriver = INVALID_HANDLE_VALUE;    
	BOOL bResult = FALSE;
    PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
    
    hDriver = CreateFile( JPG_DRIVER_NAME,  INVALID_API_NEG, 0, NULL,  OPEN_EXISTING, 0, NULL );

    if ( hDriver != NULL && hDriver != INVALID_HANDLE_VALUE ) {
        g_pKato->Log(LOG_FAIL, TEXT("JPG open with negative value for access code test failed"));
        pCmdData->bTestResult=FALSE;

        bResult=CloseHandle(hDriver);
	    if ( bResult == FALSE ) {
            g_pKato->Log(LOG_FAIL, TEXT("JPG close failed"));
            pCmdData->bTestResult=FALSE;
        }
    }

	// Large value for access code test     
    hDriver = CreateFile( JPG_DRIVER_NAME,  INVALID_API_MAX, 0, NULL,  OPEN_EXISTING, 0, NULL );
    
    if ( hDriver != NULL && hDriver != INVALID_HANDLE_VALUE ) {
        g_pKato->Log(LOG_FAIL, TEXT("JPG open with large value for access code test failed"));
        pCmdData->bTestResult=FALSE;
    
        bResult=CloseHandle(hDriver);
	    if ( bResult == FALSE ) {
            g_pKato->Log(LOG_FAIL, TEXT("JPG close failed"));
            pCmdData->bTestResult=FALSE;
        }
    }
	SetEvent(pCmdData->hWaitInvalidTest);
	
    return 0;    
}
// Thread of test case 1105, this thread try to operate JPEG IO control with 
// negative value, maximum value.
DWORD  ThreadProc_InvalidIOCtl(LPVOID lpParameter)
{
  	char testbuf;
    HANDLE hDriver = INVALID_HANDLE_VALUE;    
	BOOL bResult = FALSE;
    PCMD_DATA pCmdData = (PCMD_DATA) lpParameter;
	
    hDriver = CreateFile(JPG_DRIVER_NAME,GENERIC_READ|GENERIC_WRITE,
							0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if( (hDriver == INVALID_HANDLE_VALUE) || (NULL == hDriver) ){
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Driver CreateFile get failed! \r\n"));
		pCmdData->bTestResult=FALSE;
		return 0;
	}

	// Negative value for access code test     
	DeviceIoControl(hDriver, IOCTL_JPG_GET_STRBUF, NULL, 
					INVALID_API_NEG, &(testbuf), sizeof(testbuf), NULL, NULL);
					
    if ( hDriver != NULL && hDriver != INVALID_HANDLE_VALUE ) {
        g_pKato->Log(LOG_FAIL, TEXT("JPG IOCTL with negative value for access code test failed"));
        pCmdData->bTestResult=FALSE;
    }

	// Large value for access code test     
    DeviceIoControl(hDriver, IOCTL_JPG_GET_STRBUF, NULL, 
					INVALID_API_MAX, &(testbuf), sizeof(testbuf), NULL, NULL);
    if ( hDriver != NULL && hDriver != INVALID_HANDLE_VALUE ) {
        g_pKato->Log(LOG_FAIL, TEXT("JPG IOCTL with large value for access code test failed"));
        pCmdData->bTestResult=FALSE;
    
        bResult=CloseHandle(hDriver);
	    if ( bResult == FALSE ) {
            g_pKato->Log(LOG_FAIL, TEXT("JPG close failed"));
            pCmdData->bTestResult=FALSE;
        }
    } 
    
    if ( NULL != hDriver )
    	CloseHandle(hDriver);
	SetEvent(pCmdData->hWaitInvalidTest);
	
    return 0;
}

//Total file input for this test case
#define T1001_FI_NUM	5
// Test Case 1001: Decompressed a JPEG image and show on LCD
TESTPROCAPI JPEGDec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *tdfile01_str;
	wchar_t* list[T1001_FI_NUM] = {L"\\Storage Card\\t420_640_480.jpg", L"\\Storage Card\\t422_1280_960.jpg",
								   L"\\Storage Card\\t444_2048_1024.jpg", L"\\Storage Card\\t640_480.jpg", 
                                   L"\\Storage Card\\tgray_1600_1200.jpg"};
	HWND hWnd1;
	int user_ret;
	unsigned char fail_log = 0;
	unsigned char i;
	
    if(uMsg != TPM_EXECUTE)
        return TPR_NOT_HANDLED;

    g_pKato->Log(LOG_COMMENT, TEXT("This test will decode JPEG image and show on LCD."));
	tdfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tdfile01_str == NULL )
    {
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n"));
		return TPR_FAIL;
	}

    hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) 
    {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		free(tdfile01_str);
		return TPR_FAIL;
	}
	tdfile01_str->outfilename = NULL;
	tdfile01_str->fpt = FALSE;	//for performance test
	tdfile01_str->do_thumb = FALSE;
	for(i = 0; i < T1001_FI_NUM; i++)	
    {
		tdfile01_str->infilename = list[i];
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Decompress filename %s\n"),tdfile01_str->infilename);
		if ( JPEGDecExe(tdfile01_str) != TPR_PASS ) 
        {
			g_pKato->Log(LOG_FAIL, TEXT("JPEG Decompress image failed!\n"));
			fail_log++;
		}
		else 
        {
			user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this image correctly!", 
								(LPCWSTR)L"JPEG TUX Result", MB_YESNO);		
			if( user_ret != IDYES )
            {
				g_pKato->Log(LOG_FAIL, TEXT("User reported decompress image failed!\n"));
				fail_log ++;
			}
			g_pKato->Log(LOG_PASS, TEXT("User reported decompress image successful!\n"));
		}
	}
	free(tdfile01_str);
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}

//Total file input for this test case
#define T1002_FI_NUM	4
// Test Case 1002: Compressed a raw image into JPEG format. 
//				   And manually use other JPEG photo viewer to check it.
TESTPROCAPI JPEGCom(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *tcfile01_str;
	HWND hWnd1;
	int user_ret;
	unsigned char i;
	unsigned char fail_log = 0;
	wchar_t* ilist[T1002_FI_NUM] = {L"\\Storage Card\\t640_480.yuv", L"\\Storage Card\\t1280_960.yuv", 
                                    L"\\Storage Card\\t1600_1200.yuv", L"\\Storage Card\\t2048_1024.yuv"};
	wchar_t* olist[T1002_FI_NUM] = {L"\\Storage Card\\to002_A.jpg", L"\\Storage Card\\to002_B.jpg", 
                                    L"\\Storage Card\\to002_C.jpg", L"\\Storage Card\\to002_D.jpg"};
	INT32 inrawwith[T1002_FI_NUM] = {640,1280,1600,2048};
	INT32 inrawheigh[T1002_FI_NUM] = {480,960,1200,1024};
	IMAGE_QUALITY_TYPE_T outquality[T1002_FI_NUM] = {JPG_QUALITY_LEVEL_2,JPG_QUALITY_LEVEL_2,JPG_QUALITY_LEVEL_1,JPG_QUALITY_LEVEL_3};
	SAMPLE_MODE_T outsampmod[T1002_FI_NUM] = {JPG_422,JPG_422,JPG_422,JPG_422};
	
    if(uMsg != TPM_EXECUTE)
        return TPR_NOT_HANDLED;

    hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) 
    {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		return TPR_FAIL;
	}
    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress YUV files into JPEG"));
	tcfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tcfile01_str == NULL )
    {
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    
	tcfile01_str->fpt = FALSE;	//for performance test
	tcfile01_str->do_thumb = FALSE;
	
	for(i = 0; i < T1002_FI_NUM; i++)	
    {
		tcfile01_str->infilename = ilist[i];
		tcfile01_str->outfilename = olist[i];
		tcfile01_str->inwidth = inrawwith[i];
		tcfile01_str->inheight = inrawheigh[i];
		tcfile01_str->samplemode = outsampmod[i];
		tcfile01_str->imgquality = outquality[i];
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress filename %s\n"),tcfile01_str->infilename);
		if( JPEGComExe(tcfile01_str) != TPR_PASS )	
        {
			g_pKato->Log(LOG_FAIL, TEXT("JPEG Compress image failed!\n."));
			fail_log++;
			break;
		}
	    g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress output filename %s\n"),tcfile01_str->outfilename);
	}
	user_ret = MessageBox(hWnd1, (LPCWSTR)L"Remove the SD card and view compressed images on the PC.  Do they display correctly?", 
						  (LPCWSTR)L"JPEG TUX Result", MB_YESNO);		
	if( user_ret != IDYES )
    {
		g_pKato->Log(LOG_FAIL, TEXT("User reported compress image failed!\n."));
		fail_log ++;
	}
	else
		g_pKato->Log(LOG_PASS, TEXT("User reported compress image successful!\n."));

	free(tcfile01_str);
	return ((fail_log == 0)? TPR_PASS:TPR_FAIL);	
}

// Test Case 1101: Basic Stream Driver Test
TESTPROCAPI JPEGStream(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	void *JPGHandle;
	
	JPGHandle = CreateFile(JPG_DRIVER_NAME,
							GENERIC_READ|GENERIC_WRITE, 0,
							NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(JPGHandle == INVALID_HANDLE_VALUE)
    {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Stream Interface :: CreateFile failed\r\n"));
		return TPR_FAIL;
	}
	g_pKato->Log(LOG_PASS, TEXT("JPEG Driver Successfully Opened\n"));
	if( CloseHandle(JPGHandle) == 0 ) 
    {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Stream Interface :: Close failed\r\n"));
		return TPR_FAIL;
	}
	
	return TPR_PASS;
} 

// Test Case 1102: Decompress I/O control function test
TESTPROCAPI JPEGIOCtlDec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    FILE *fp;
    void *handle;	
	UINT32 fileSize;
	char *InBuf = NULL;
	char *OutBuf = NULL;
	char *OutPhyBuf = NULL;
	char *OutRGBPhyBuf = NULL;
	char *OutRGBBuf = NULL;
	long decSize;
	JPEG_ERRORTYPE ret;
	UINT32 width, height, samplemode;
    RESULT_IOCL gb_res_ioctl;
    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will test some IO controls by running Decompress JPEG image"));
	gb_res_ioctl.get_strbuf = JPG_TC_WONT;
	gb_res_ioctl.get_frmbuf = JPG_TC_SKIP;    
	gb_res_ioctl.get_rgbbuf = JPG_TC_SKIP;
	gb_res_ioctl.get_phy_frmbuf = JPG_TC_SKIP;
	gb_res_ioctl.get_phy_rgbbuf = JPG_TC_SKIP;
	gb_res_ioctl.get_thumb_strbuf = JPG_TC_WONT;
	gb_res_ioctl.get_thumb_frmbuf = JPG_TC_WONT;
    /*==========================================================================*/
    handle = GetJPGDrvPtrDec();

	if(handle == NULL)
    {
		g_pKato->Log(LOG_COMMENT, TEXT("Get JPEG driver handle pointer failed\n"));
		return TPR_FAIL;
	}
	
	fp = _wfopen(L"\\Storage Card\\t420_640_480.jpg", L"rb");
	if(fp == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("JPEG File open error!\n No Target IOCTL be tested!\n"));
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	InBuf = (char*)GetDecInBuf(handle,fileSize);
	if(InBuf == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get input buffer is NULL\n No Target IOCTL be tested!\n"));
		DeinitJPEGDec(handle);
		fclose(fp);
		return TPR_FAIL;
	}
	fread(InBuf, 1, fileSize, fp);
	fclose(fp);

	ret = ExeJPEGDec(handle);
	if(ret != JPEG_OK){
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Decoding failed\n No Target IOCTL be tested!\n"));
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}

	OutBuf = (char*)GetDecOutBuf(handle,&decSize);
	if( ZeroChecker(OutBuf,sizeof(OutBuf)) == TRUE )	{ //Whole buffer is zero.
		gb_res_ioctl.get_frmbuf = JPG_TC_FAIL;
		g_pKato->Log(LOG_FAIL, TEXT("JPEG IO Control test, GET_FRMBUF Failed!\n"));		
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}
	gb_res_ioctl.get_frmbuf = JPG_TC_PASS;
	g_pKato->Log(LOG_PASS, TEXT("JPEG IO Control test, GET_FRMBUF Passed!\n"));
	GetJPEGInfo(JPEG_GET_DECODE_WIDTH, &width);
	GetJPEGInfo(JPEG_GET_DECODE_HEIGHT, &height);
	GetJPEGInfo(JPEG_GET_SAMPING_MODE, &samplemode);
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG decode image info: width : %d height : %d samplemode : %d\n\n"), width, height, samplemode);
	OutPhyBuf = (char*)GetJPEGDecOutPhyBuf(handle);
	if( ZeroChecker(OutPhyBuf,sizeof(OutPhyBuf)) == TRUE )	{ //Whole buffer is zero.
		gb_res_ioctl.get_phy_frmbuf = JPG_TC_FAIL;
		g_pKato->Log(LOG_FAIL, TEXT("JPEG IO Control test, GET_PHY_FRMBUF Failed!\n"));
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}
	gb_res_ioctl.get_phy_frmbuf = JPG_TC_PASS;
	g_pKato->Log(LOG_PASS, TEXT("JPEG IO Control test, GET_PHY_FRMBUF Passed!\n"));
	OutRGBBuf = (char*)GetJPEGRGBBuf(handle,LCD_X, LCD_Y);
	if( ZeroChecker(OutRGBBuf,sizeof(OutRGBBuf)) == TRUE )	{ //Whole buffer is zero.
		gb_res_ioctl.get_rgbbuf = JPG_TC_FAIL;
		g_pKato->Log(LOG_FAIL, TEXT("JPEG IO Control test, GET_RGBBUF Failed!\n"));
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}
	gb_res_ioctl.get_rgbbuf = JPG_TC_PASS;
	g_pKato->Log(LOG_PASS, TEXT("JPEG IO Control test, GET_RGBBUF Passed!\n"));
	OutRGBPhyBuf = (char*)GetJPEGRGBPhyBuf(handle,LCD_X, LCD_Y);
	if( ZeroChecker(OutRGBPhyBuf,sizeof(OutRGBPhyBuf)) == TRUE )	{ //Whole buffer is zero.
		gb_res_ioctl.get_phy_rgbbuf = JPG_TC_FAIL;
		g_pKato->Log(LOG_FAIL, TEXT("JPEG IO Control test, GET_PHY_RGBBUF Failed!\n"));
		DeinitJPEGDec(handle);
		return TPR_FAIL;
	}
	gb_res_ioctl.get_phy_rgbbuf = JPG_TC_PASS;
	g_pKato->Log(LOG_PASS, TEXT("JPEG IO Control test, GET_PHY_RGBBUF Passed!\n"));
	DeinitJPEGDec(handle);
	
	g_pKato->Log(LOG_PASS, TEXT("JPEG Decode IO Control test finished."));
    
    return TPR_PASS;
}

// Test Case 1103: Compress I/O control function test
TESTPROCAPI JPEGIOCtlEnc(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    void *handle;
	BOOL result = TRUE;
	FILE *fp;
	char *InBuf = NULL;
	char *OutBuf = NULL;
	long fileSize;
	long frameSize;
	RESULT_IOCL gb_res_ioctl;
	ExifFileInfo *ExifInfo;	
	
	if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

	g_pKato->Log(LOG_COMMENT, TEXT("This test will test some IO controls by running Compress JPEG image"));
    
	handle = GetJPGDrvPtrEnc();
	if(handle == NULL) {
		g_pKato->Log(LOG_FAIL, TEXT("Get JPEG driver handle pointer failed\n"));
		result = FALSE;
	}
	if( SetJPGConfig(JPEG_SET_SAMPING_MODE, JPG_422) != JPEG_OK ){
		result = FALSE;
	}
	if( SetJPGConfig(JPEG_SET_ENCODE_WIDTH, 640) != JPEG_OK ){
		result = FALSE;
	}
	if( SetJPGConfig(JPEG_SET_ENCODE_HEIGHT, 480) != JPEG_OK ){
		result = FALSE;
	}
	if( SetJPGConfig(JPEG_SET_ENCODE_QUALITY, JPG_QUALITY_LEVEL_2) != JPEG_OK ){
		result = FALSE;
	}
	
	if ( result == FALSE ) {
		g_pKato->Log(LOG_FAIL, TEXT("Init test setting failed!\n"));
		DeInitJPEGEnc(handle);
		return TPR_FAIL;	
	}
	
	fp = _wfopen(L"\\Storage Card\\t640_480.yuv", L"rb");
	if(fp == NULL){	
		g_pKato->Log(LOG_FAIL, TEXT("JPEG File open error!\n No Target IOCTL be tested!\n"));
		DeInitJPEGEnc(handle);
		return TPR_FAIL;
	}
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	InBuf = (char*)GetEncInBuf(handle, fileSize);
	if(InBuf == NULL){
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get InBuf is NULL\n"));
		DeInitJPEGEnc(handle);
		fclose(fp);
		return TPR_FAIL;
	}
	
	fread(InBuf, 1, fileSize, fp);
	fclose(fp);
	
	ExifInfo = (ExifFileInfo *)malloc(sizeof(ExifFileInfo));
	memset(ExifInfo, 0x00, sizeof(ExifFileInfo));
	makeExifParam(ExifInfo);
	
	if( ExeJPEGEnc(handle, ExifInfo) != JPEG_OK){
		DeInitJPEGEnc(handle);
		g_pKato->Log(LOG_FAIL, TEXT("Execute compress failed!\n"));
		free(ExifInfo);
		return TPR_FAIL;
	}
	OutBuf = (char*)GetEncOutBuf(handle, &frameSize);
	if(OutBuf == NULL){
		DeInitJPEGEnc(handle);
		g_pKato->Log(LOG_FAIL, TEXT("Get output buffer failed!\n"));
		free(ExifInfo);
		return TPR_FAIL;
	}
	if( ZeroChecker(OutBuf,sizeof(OutBuf)) == TRUE )	{ //Whole buffer is zero.
		g_pKato->Log(LOG_FAIL, TEXT("JPEG IO Control test, GET_STRBUF Failed!\n"));
		gb_res_ioctl.get_strbuf = JPG_TC_FAIL;
		DeInitJPEGEnc(handle);
		free(ExifInfo);
		return TPR_FAIL;
	}
	gb_res_ioctl.get_strbuf = JPG_TC_PASS;
	g_pKato->Log(LOG_FAIL, TEXT("JPEG IO Control test, GET_STRBUF Passed!\n"));
	
	DeInitJPEGEnc(handle);
	free(ExifInfo);
	
	return TPR_PASS;
}

//Total file input for this test case
#define T1301_FI_NUM	2
// Test Case 1301: Compressed image performance test.
TESTPROCAPI JPEGComPerf(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *tcpfile01_str;
	unsigned char i;
    unsigned char fail_log = 0;
	wchar_t* pilist[T1301_FI_NUM] = {L"\\Storage Card\\t1600_1200.yuv", L"\\Storage Card\\t640_480.yuv"};
	wchar_t* polist[T1301_FI_NUM] = {L"\\Storage Card\\to301_A.jpg", L"\\Storage Card\\to301_B.jpg"};
	UINT32 inrawwith[T1301_FI_NUM] = {1600,640};
	UINT32 inrawheigh[T1301_FI_NUM] = {1200,480};
	UINT32 pt_range[T1301_FI_NUM] = {252,133};
	 
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will test JPEG compress performance"));
    
	tcpfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tcpfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
	tcpfile01_str->fpt = TRUE;
	tcpfile01_str->do_thumb = FALSE;
	tcpfile01_str->samplemode = JPG_420;
	tcpfile01_str->imgquality = JPG_QUALITY_LEVEL_2;
	for(i = 0; i < T1301_FI_NUM; i++){
    	tcpfile01_str->infilename = pilist[i];
		tcpfile01_str->outfilename = polist[i];
		tcpfile01_str->inwidth = inrawwith[i];
		tcpfile01_str->inheight = inrawheigh[i];
		if( JPEGComExe(tcpfile01_str) != TPR_PASS ){
			g_pKato->Log(LOG_FAIL, TEXT("Compress image performance test failed!\n"));
			return TPR_FAIL;
		}		
		if( tcpfile01_str->ptime > pt_range[i] ) {
			g_pKato->Log(LOG_FAIL, TEXT("Compress image spend too much time!\n"));
			g_pKato->Log(LOG_FAIL, TEXT("This test should finish in [%d] ticks, but it actually spend [%d] ticks"),
                                         pt_range[i], tcpfile01_str->ptime);
			fail_log++;
		}
		else
			g_pKato->Log(LOG_COMMENT, TEXT("Performance time for %s is %d ms\n"),tcpfile01_str->infilename, tcpfile01_str->ptime);
	}
	free(tcpfile01_str);
	return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}

//Total file input for this test case
#define T1302_FI_NUM	2
// Test Case 1302: Decompressed image performance test.
TESTPROCAPI JPEGDecPerf(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *tdpfile01_str;
	unsigned char i;
    unsigned char fail_log = 0;
	wchar_t* list[T1302_FI_NUM] = {L"\\Storage Card\\t422_1280_960.jpg", L"\\Storage Card\\t420_640_480.jpg"};
	UINT32 pt_range[T1302_FI_NUM] = {359,225};
	
	if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will test JPEG decompress performace"));
	
	tdpfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tdpfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
	tdpfile01_str->outfilename = NULL;
	tdpfile01_str->fpt = TRUE;
	tdpfile01_str->do_thumb = FALSE;
	for(i = 0; i < T1302_FI_NUM; i++ )	{
    	tdpfile01_str->infilename = list[i];
	
		if( JPEGDecExe(tdpfile01_str) != TPR_PASS )	{
			g_pKato->Log(LOG_FAIL, TEXT("Decompress image performance test failed!\n"));
			return TPR_FAIL;
		}
		if( tdpfile01_str->ptime > pt_range[i] ) {
			g_pKato->Log(LOG_FAIL, TEXT("Decompress image spend too much time!\n"));
			g_pKato->Log(LOG_FAIL, TEXT("This test should finish in [%d] ticks, but it actually spend [%d] ticks"),
                         pt_range[i],tdpfile01_str->ptime);
			fail_log++;
		}
		else
			g_pKato->Log(LOG_COMMENT, TEXT("Performance time for %s is %d ms\n"),tdpfile01_str->infilename, tdpfile01_str->ptime);
	}
	free(tdpfile01_str);
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}

// Test Case 1501: Oversize image Compressed test.
#define T501_W	2160
#define T501_H	1562
TESTPROCAPI JPEGOverCom(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
   	wchar_t* ilist = L"\\t2160_1562.yuv";
   	wchar_t* olist = L"\\t501_A.jpg";
	TUX_JPEG_PARA *tocfile01_str;
	unsigned char fail_log = 0;
	    
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress an oversized image.\n"));
	tocfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tocfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    tocfile01_str->infilename = ilist;
	tocfile01_str->outfilename = olist;
	tocfile01_str->fpt = FALSE;	
	tocfile01_str->do_thumb = FALSE;
	tocfile01_str->may_oversize = FALSE;
	tocfile01_str->inwidth = T501_W;
	tocfile01_str->inheight = T501_H;
	tocfile01_str->samplemode = JPG_420;
	tocfile01_str->imgquality = JPG_QUALITY_LEVEL_2;

	if( (JPEGComExe(tocfile01_str) == TPR_FAIL) && (tocfile01_str->may_oversize == TRUE) ){
		g_pKato->Log(LOG_COMMENT, TEXT("Compress oversize image test finish with oversize flag set.\n"));
	}
	else {
		fail_log++;
	}
	free(tocfile01_str);
	return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}

// Test Case 1502: Oversize image Decompressed test.
TESTPROCAPI JPEGOverDec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *todfile01_str;
	wchar_t* ilist = L"\\Storage Card\\t2164_1564.jpg";
	unsigned char fail_log = 0;
	
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will decompress an oversized image"));
	todfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( todfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    todfile01_str->infilename = ilist;
	todfile01_str->outfilename = NULL;
	todfile01_str->fpt = FALSE;	//for performance test
	todfile01_str->may_oversize = FALSE;
	todfile01_str->do_thumb = FALSE;
	if( (JPEGDecExe(todfile01_str) == TPR_FAIL) && (todfile01_str->may_oversize == TRUE) ){
		g_pKato->Log(LOG_COMMENT, TEXT("Decompress oversize image test finish with oversize flag set.\n"));		
	}
	else {
		fail_log++;
	}
	free(todfile01_str);
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}

// Test Case 1503: Exactly image size compressed and decompressed.
#define TI503_W	2048
#define TI503_H	1536
TESTPROCAPI JPEGExactCodec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *tefile01_str;
	wchar_t* ielist = L"\\Storage Card\\t2048_1536.yuv";
	wchar_t* oelist = L"\\Storage Card\\to503_A.jpg";
	HWND hWnd1;
	int user_ret;
	unsigned char fail_log = 0;
	
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress/decompress 2048x1536 pixel JPEG image\n"));
	tefile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tefile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		fail_log++;
		goto LEAVEREAD;
	}
	
    hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		fail_log ++;
		goto LEAVEREAD;
	}
	tefile01_str->infilename = ielist;
	tefile01_str->outfilename = oelist;
	tefile01_str->fpt = FALSE;	//for performance test
	tefile01_str->do_thumb = FALSE;
	tefile01_str->inwidth = TI503_W;
	tefile01_str->inheight = TI503_H;
	tefile01_str->samplemode = JPG_422;
	tefile01_str->imgquality = JPG_QUALITY_LEVEL_2;
	
	if ( JPEGComExe(tefile01_str) != TPR_PASS ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in compress image!\n."));
		fail_log ++;
		goto LEAVEREAD;
	}
	tefile01_str->infilename = oelist;

	if( JPEGDecExe(tefile01_str) != TPR_PASS ) {
		g_pKato->Log(LOG_FAIL, TEXT("Fail in decompress image!\n."));
		fail_log ++;
		goto LEAVEREAD;
	}
	
	user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this image correctly!", 
								(LPCWSTR)L"JPEG TUX Result", MB_YESNO);		
	if( user_ret != IDYES ){
		g_pKato->Log(LOG_FAIL, TEXT("User reported decompress image failed!\n."));
		fail_log ++;
	}		
LEAVEREAD:	
	if( tefile01_str != NULL )	
		free(tefile01_str);
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}
//Total file input for this test case
#define T1504_FI_NUM	2
// Test Case 1504: Small image size compressed and decompressed.
TESTPROCAPI JPEGSmallCodec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *tsfile01_str;
	wchar_t* ielist[T1504_FI_NUM] = {L"\\Storage Card\\small_16x16.yuv", L"\\Storage Card\\small_8x8.yuv"};
	wchar_t* oelist[T1504_FI_NUM] = {L"\\Storage Card\\to504_A.jpg", L"\\Storage Card\\to504_B.jpg"};
	INT32 inrawwith[T1002_FI_NUM] = {16,8};
	INT32 inrawheigh[T1002_FI_NUM] = {16,8};
	HWND hWnd1;
	unsigned char i;
	unsigned char fail_log = 0;
	int user_ret;
	
    if(uMsg != TPM_EXECUTE)
        return TPR_NOT_HANDLED;

    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress/decompress 16x16, 8x8 pixel images\n"));
	tsfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tsfile01_str == NULL )
    {
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		fail_log++;
		goto LEAVEREAD;
	}
    hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) 
    {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		fail_log++;
		goto LEAVEREAD;
	}
	
	tsfile01_str->fpt = FALSE;	//for performance test
	tsfile01_str->do_thumb = FALSE;	
	tsfile01_str->samplemode = JPG_422;
	tsfile01_str->imgquality = JPG_QUALITY_LEVEL_2;
	for(i = 0; i < T1001_FI_NUM; i++)	
    {
		tsfile01_str->inwidth = inrawwith[i];
		tsfile01_str->inheight = inrawheigh[i];
		tsfile01_str->infilename = ielist[i];
		tsfile01_str->outfilename = oelist[i];
		printf("JPEG Compress small image filename %s\n",tsfile01_str->infilename);
		if ( JPEGComExe(tsfile01_str) != TPR_PASS )
        {
			g_pKato->Log(LOG_FAIL, TEXT("Fail in compress small image!\n."));
			fail_log++;
			break;
		}
		tsfile01_str->infilename = oelist[0];
		printf("JPEG Decompress small image filename %s\n",tsfile01_str->infilename);
		if( JPEGDecExe(tsfile01_str) != TPR_PASS ) 
        {
			g_pKato->Log(LOG_FAIL, TEXT("Fail in decompress small image!\n."));
			fail_log++;
		}
		else 
        {
			user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this image correctly!", 
										(LPCWSTR)L"JPEG TUX Result", MB_YESNO);		
			if( user_ret != IDYES )
            {
				g_pKato->Log(LOG_FAIL, TEXT("User reported decompress image failed!\n."));
				fail_log ++;
			}
		}
	}
LEAVEREAD:	
	if( tsfile01_str != NULL )
		free(tsfile01_str);
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}
//Total file input for this test case
#define T1505_FI_NUM	2
#define TO505_A1W	1599
#define TO505_A2W	799
#define TO505_A1H	1198
#define TO505_A2H	598
// Test Case 1505: Odd image size compressed.
TESTPROCAPI JPEGOddCom(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    TUX_JPEG_PARA *todcfile01_str;
    HWND hWnd1;
	int user_ret;	
    unsigned char i;
    unsigned char fail_log = 0;
	wchar_t* ilist[T1505_FI_NUM] = {L"\\Storage Card\\t1599_1198.yuv", L"\\Storage Card\\t799_598.yuv"};
    wchar_t* olist[T1505_FI_NUM] = {L"\\Storage Card\\to505_A.jpg", L"\\Storage Card\\to505_B.jpg"};
    INT32 inrawwith[T1505_FI_NUM] = {TO505_A1W,TO505_A2W};
	INT32 inrawheigh[T1505_FI_NUM] = {TO505_A1H,TO505_A2H};
	IMAGE_QUALITY_TYPE_T outquality[T1505_FI_NUM] = {JPG_QUALITY_LEVEL_2,JPG_QUALITY_LEVEL_2};
	SAMPLE_MODE_T outsampmod[T1505_FI_NUM] = {JPG_422,JPG_422};
	
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress an oversize image.\n"));
	todcfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( todcfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		free(todcfile01_str);
		return TPR_FAIL;
	}
	for(i = 0; i < T1505_FI_NUM; i++)	
    {
		todcfile01_str->infilename = ilist[i];
		todcfile01_str->outfilename = olist[i];
		todcfile01_str->inwidth = inrawwith[i];
		todcfile01_str->inheight = inrawheigh[i];
		todcfile01_str->samplemode = outsampmod[i];
		todcfile01_str->imgquality = outquality[i];
		printf("JPEG Compress filename %s\n",todcfile01_str->infilename);
		if( JPEGComExe(todcfile01_str) != TPR_PASS )	{
			g_pKato->Log(LOG_FAIL, TEXT("JPEG Compress image failed!\n."));
			fail_log++;
			break;
		}
		printf("JPEG Compress output filename %s\n",todcfile01_str->outfilename);
		user_ret = MessageBox(hWnd1, (LPCWSTR)L"Does this image can show on PC correctly!", 
								(LPCWSTR)L"JPEG TUX Result", MB_YESNO);		
		if( user_ret != IDYES ){
			g_pKato->Log(LOG_FAIL, TEXT("User reported compress image failed!\n."));
			fail_log ++;
		}
		g_pKato->Log(LOG_COMMENT, TEXT("User reported compress image successful!\n."));
	}
	free(todcfile01_str);
	return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}

//Total file input for this test case
#define T1506_FI_NUM	3
// Test Case 1506: Odd image size decompressed.
TESTPROCAPI JPEGOddDec(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	TUX_JPEG_PARA *toddfile01_str;
	wchar_t* list[T1506_FI_NUM] = {L"\\Storage Card\\t420_1599_1198.jpg", L"\\Storage Card\\t422_799_598.jpg", 
                                    L"\\Storage Card\\t444_1599_1198.jpg"};
	HWND hWnd1;
	int user_ret;
	unsigned char fail_log = 0;
	unsigned char i;
	
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    g_pKato->Log(LOG_COMMENT, TEXT("This test will decompress an odd size image!\n"));
	toddfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( toddfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		free(toddfile01_str);
		return TPR_FAIL;
	}
	toddfile01_str->outfilename = NULL;
	toddfile01_str->fpt = FALSE;	//for performance test
	toddfile01_str->do_thumb = FALSE;
	for(i = 0; i < T1506_FI_NUM; i++)	{
		toddfile01_str->infilename = list[i];
		printf("JPEG Decompress filename %s\n",toddfile01_str->infilename);
		if ( JPEGDecExe(toddfile01_str) != TPR_PASS ) {
			g_pKato->Log(LOG_FAIL, TEXT("JPEG Decompress image failed!\n."));
			fail_log++;
		}
		else {
			user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this image correctly!", 
								(LPCWSTR)L"JPEG TUX Result", MB_YESNO);		
			if( user_ret != IDYES ){
				g_pKato->Log(LOG_FAIL, TEXT("User reported decompress image failed!\n."));
				fail_log ++;
			}
			g_pKato->Log(LOG_PASS, TEXT("User reported decompress image successful!\n."));
		}
	}
	free(toddfile01_str);
	
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);
}

void DisplayJPEG (int srcAddr, 
				  INT32 srcwidth, INT32 srcheight, 
				  INT32 dstwidth, INT32 dstheight,
				  int displayTime)
{
	HANDLE						hVideoDrv = INVALID_HANDLE_VALUE;
	SVEARG_FIMD_WIN_MODE		tParamMode;
	SVEARG_FIMD_WIN_COLORKEY	tParamCKey;
	SVEARG_FIMD_WIN_ALPHA		tParamAlpha;
	SVEARG_POST_PARAMETER		tParamPost;
	SVEARG_POST_BUFFER			tParamBuffer;
	DWORD						dwBytes;	

	//-----------------------
	// Open Video Driver
	//-----------------------
	hVideoDrv = CreateFile( L"VDE0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (hVideoDrv == INVALID_HANDLE_VALUE)
	{
		g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] VDE0 Open Device Failed\n"));
		return;
	}

	// Request FIMD Win0 H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_REQUEST_FIMD_WIN0 Failed\n"));
		return;
	}

	// Request Post Processor H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{ 
		g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_REQUEST_POST Failed\n"));
		return;
	}

		tParamMode.dwWinMode = DISP_WIN0_POST_RGB;
		tParamMode.dwBPP = DISP_24BPP_888;
			tParamMode.dwWidth = dstwidth;
			tParamMode.dwHeight = dstheight;
			tParamMode.dwOffsetX = 0;
			tParamMode.dwOffsetY = 0;

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParamMode, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
			{
				g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n"));
				return;
			}

			// Color Key Disable
			tParamCKey.dwWinNum = DISP_WIN1;
			tParamCKey.bOnOff = FALSE;
			tParamCKey.dwDirection = DISP_FG_MATCH_BG_DISPLAY;
			tParamCKey.dwColorKey = 0;
			tParamCKey.dwCompareKey = 0;

			// Alpha Set to 0x0 (Show Window0)
			tParamAlpha.dwWinNum = DISP_WIN1;
			tParamAlpha.dwMethod = DISP_ALPHA_PER_PLANE;
			tParamAlpha.dwAlpha0 = 0x0;
			tParamAlpha.dwAlpha1 = 0x0;

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY, &tParamCKey, sizeof(SVEARG_FIMD_WIN_COLORKEY), NULL, 0, &dwBytes, NULL) )
			{
				g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n"));
				return;
			}

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
			{
				g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n"));
				return;
			}

			tParamPost.dwOpMode = POST_FREE_RUN_MODE;
			tParamPost.dwScanMode = POST_PROGRESSIVE;
			tParamPost.dwSrcType = POST_SRC_RGB16;
		 	tParamPost.dwSrcBaseWidth = srcwidth;
			tParamPost.dwSrcBaseHeight = srcheight;
			tParamPost.dwSrcWidth = tParamPost.dwSrcBaseWidth;
			tParamPost.dwSrcHeight = tParamPost.dwSrcBaseHeight;
			tParamPost.dwSrcOffsetX = 0;
			tParamPost.dwSrcOffsetY = 0;
			tParamPost.dwDstType = POST_DST_FIFO_RGB888;
		 	tParamPost.dwDstBaseWidth = dstwidth;
			tParamPost.dwDstBaseHeight = dstheight;
			tParamPost.dwDstWidth = dstwidth;
			tParamPost.dwDstHeight = dstheight;
			tParamPost.dwDstOffsetX = 0;
			tParamPost.dwDstOffsetY = 0;

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_PARAM, &tParamPost, sizeof(SVEARG_POST_PARAMETER), NULL, 0, &dwBytes, NULL) )
			{
				g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n"));
				return;
			}

			tParamBuffer.dwBufferRGBY = srcAddr;
			tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+srcwidth*srcheight;
			tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+srcwidth*srcheight/4;
			tParamBuffer.bWaitForVSync = FALSE;

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
			{
				g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
				return;
			}

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
			{
				g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER Failed\n"));
				return;
			}

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
			{
				g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_START Failed\n"));
				return;
			}

	Sleep(displayTime*1000);

	//--------------------------------------------
	// close surface
	//---------------------------------------------
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_STOP, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_STOP Failed\n"));
		return;
	}
	// Release FIMD Win0 H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_RELEASE_FIMD_WIN0 Failed\n"));
		return;
	}

	// Release Post Processor H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_RELEASE_POST Failed\n"));
		return;
	}

	if ( CloseHandle(hVideoDrv) == 0 ) {
		g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] CloseHandle Failed\n"));
		CloseHandle(hVideoDrv);
	}


}

////////////////////////////////////////////////////////////////////////////////
