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
//  JpegE2E TUX DLL
//
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"
#include "s3c6410_display_con.h"
#include "S3c6410_post_proc.h"
#include "SVE_API.h"
#include "JPGDrvHandler.h"

#define RAW_IMAGE_FLASH_PATH    L"\\NandFlash\\t640_480.yuv"
#define RAW_IMAGE_USB_PATH      L"\\Hard Disk\\t640_480.yuv"
#define RAW_IMAGE_SD_PATH       L"\\Storage Card\\t640_480.yuv"

#define JPEG_IMAGE_FLASH_PATH   L"\\NandFlash\\t640_480.jpg"
#define JPEG_IMAGE_USB_PATH     L"\\Hard Disk\\t640_480.jpg"
#define JPEG_IMAGE_SD_PATH      L"\\Storage Card\\t640_480.jpg"
#define IMAGE_WIDTH             640
#define IMAGE_HEIGHT            480
#define SAMPLE_MODE             JPG_422
#define IMAGE_QUALITY_TYPE      JPG_QUALITY_LEVEL_2
#define LCD_X		            800
#define LCD_Y		            480

void* GetJPGDrvPtrDec(void);
void* GetJPGDrvPtrEnc(void);
void DisplayJPEG (int , INT32 , INT32 , INT32 , INT32 ,int );


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
		g_pKato->Log(LOG_COMMENT, TEXT("file open error!"));
		return TPR_FAIL;
	}
	handle = GetJPGDrvPtrDec();
	if(handle == NULL){
		g_pKato->Log(LOG_COMMENT, TEXT("Get JPEG driver handle pointer failed"));
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
		g_pKato->Log(LOG_COMMENT, TEXT("Input buffer is NULL"));
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
		g_pKato->Log(LOG_COMMENT, TEXT("Decoding failed\n"));
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
			g_pKato->Log(LOG_COMMENT, TEXT("ConvertYCBYCRToRGB error\n"));
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
		g_pKato->Log(LOG_COMMENT, TEXT("file open error : %s\n"), fcin_str->infilename);
		return TPR_FAIL;
	}
	handle = GetJPGDrvPtrEnc();	// Get JPEG driver handler
	if ( NULL == handle ) {
		g_pKato->Log(LOG_COMMENT, TEXT("Get JPEG driver handle pointer failed\n"));
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
		g_pKato->Log(LOG_COMMENT, TEXT("Fail in seting JPG Compress Config\n"));
		fcin_str->may_oversize = TRUE;
		fclose(fp);
		DeInitJPEGEnc(handle);
		return TPR_FAIL;
	}
	//===== 2. figure out file size =====
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	InBuf = (char*)GetEncInBuf(handle, fileSize);
	if(InBuf == NULL){
		g_pKato->Log(LOG_COMMENT, TEXT("In Buffer is NULL, failed!\n"));
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
		g_pKato->Log(LOG_COMMENT, TEXT("Execute JPEG Compress failed!\n"));
		free(ExifInfo);
		DeInitJPEGEnc(handle);
		return TPR_FAIL;
	}
	//===== 5. Get out buffer  =====
	OutBuf = (char*)GetEncOutBuf(handle, &frameSize);
	if(OutBuf == NULL){
		g_pKato->Log(LOG_COMMENT, TEXT("Failed, Out buffer is NULL!\n"));
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
		g_pKato->Log(LOG_COMMENT, TEXT("Return will be FALSE!\n"));		
		return TPR_FAIL;	
	}
	return TPR_PASS;
}


////////////////////////////////////////////////////////////////////////////////
// JpegFileSystemNAND_E2E
//    JPEG and file system (NAND) E2E test
//
// Parameters:
//    uMsg            Message code.
//    tpParam         Additional message-dependent data.
//    lpFTE           Function table entry that generated this call.
//
// Return value:
//    TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//    special conditions.

TESTPROCAPI JpegFileSystemNAND_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    TUX_JPEG_PARA *tcfile01_str;
	TUX_JPEG_PARA *tdfile01_str;
	HWND hWnd1;
	int user_ret;
	
	unsigned char fail_log = 0;

	FILE *fp;
    
	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
	
	hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Get hWnd faild\n"));
		return TPR_FAIL;
	}

	fp = _wfopen(RAW_IMAGE_FLASH_PATH, L"rb");
	if(fp == NULL){
		MessageBox(hWnd1,TEXT("File not found! Please copy raw image data file to NAND flash."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}

    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress YUV files into JPEG"));
	tcfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tcfile01_str == NULL ){
		g_pKato->Log(LOG_COMMENT, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    
	tcfile01_str->fpt = FALSE;	//for performance test
	tcfile01_str->do_thumb = FALSE;
	
	tcfile01_str->infilename  = RAW_IMAGE_FLASH_PATH;
	tcfile01_str->outfilename = JPEG_IMAGE_FLASH_PATH;
	tcfile01_str->inwidth     = IMAGE_WIDTH;
	tcfile01_str->inheight    = IMAGE_HEIGHT;
	tcfile01_str->samplemode  = SAMPLE_MODE;
	tcfile01_str->imgquality  = JPG_QUALITY_LEVEL_2;
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress filename %s\n"), tcfile01_str->infilename);
	if( JPEGComExe(tcfile01_str) != TPR_PASS )	{
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress image failed!\n."));
		fail_log++;
		goto exit;
	}
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress output filename %s\n"),tcfile01_str->outfilename);


	g_pKato->Log(LOG_COMMENT, TEXT("This test will decode JPEG image and show on LCD."));
	tdfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tdfile01_str == NULL ){
		g_pKato->Log(LOG_COMMENT, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
	hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Get hWnd faild\n"));
		return TPR_FAIL;
	}
	tdfile01_str->outfilename = NULL;
	tdfile01_str->fpt = FALSE;	//for performance test
	tdfile01_str->do_thumb = FALSE;
	
	tdfile01_str->infilename = JPEG_IMAGE_FLASH_PATH;
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Decompress filename %s\n"),tdfile01_str->infilename);
	if ( JPEGDecExe(tdfile01_str) != TPR_PASS ) {
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Decompress image failed!\n."));
		fail_log++;
	}
	else {
		user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you see this image correctly!", 
					(LPCWSTR)L"JPEG TUX Result", MB_YESNO);		
		if( user_ret != IDYES ){
			g_pKato->Log(LOG_COMMENT, TEXT("User reported decompress image failed!\n."));
			fail_log ++;
		}
		g_pKato->Log(LOG_COMMENT, TEXT("User reported decompress image successful!\n."));
	}
	
exit:
	free(tcfile01_str);
	return ((fail_log == 0)? TPR_PASS:TPR_FAIL);	
}

////////////////////////////////////////////////////////////////////////////////
// JpegFileSystemUSB_E2E
//    JPEG and file system (USB Mass Storage) E2E test
//
// Parameters:
//    uMsg            Message code.
//    tpParam         Additional message-dependent data.
//    lpFTE           Function table entry that generated this call.
//
// Return value:
//    TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//    special conditions.

TESTPROCAPI JpegFileSystemUSB_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    TUX_JPEG_PARA *tcfile01_str;
	TUX_JPEG_PARA *tdfile01_str;
	HWND hWnd1;
	int user_ret;
	
	unsigned char fail_log = 0;
	
	FILE *fp;
    

	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }
	
	hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Get hWnd faild\n"));
		return TPR_FAIL;
	}

	fp = _wfopen(RAW_IMAGE_USB_PATH, L"rb");
	if(fp == NULL){
		MessageBox(hWnd1,TEXT("File not found! Please copy raw image data file to USB storage."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}

    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress YUV files into JPEG"));
	tcfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tcfile01_str == NULL ){
		g_pKato->Log(LOG_COMMENT, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    
	tcfile01_str->fpt = FALSE;	//for performance test
	tcfile01_str->do_thumb = FALSE;
	
	tcfile01_str->infilename  = RAW_IMAGE_USB_PATH;
	tcfile01_str->outfilename = JPEG_IMAGE_USB_PATH;
	tcfile01_str->inwidth     = IMAGE_WIDTH;
	tcfile01_str->inheight    = IMAGE_HEIGHT;
	tcfile01_str->samplemode  = SAMPLE_MODE;
	tcfile01_str->imgquality  = JPG_QUALITY_LEVEL_2;
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress filename %s\n"),tcfile01_str->infilename);
	if( JPEGComExe(tcfile01_str) != TPR_PASS )	{
		g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress image failed!\n."));
		fail_log++;
		goto exit;
	}
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress output filename %s\n"),tcfile01_str->outfilename);


	g_pKato->Log(LOG_COMMENT, TEXT("This test will decode JPEG image and show on LCD."));
	tdfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tdfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		free(tcfile01_str);
		return TPR_FAIL;
	}
	hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		free(tcfile01_str);
		free(tdfile01_str);
		return TPR_FAIL;
	}
	tdfile01_str->outfilename = NULL;
	tdfile01_str->fpt = FALSE;	//for performance test
	tdfile01_str->do_thumb = FALSE;
	
	tdfile01_str->infilename = JPEG_IMAGE_USB_PATH;
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Decompress filename %s\n"),tdfile01_str->infilename);
	if ( JPEGDecExe(tdfile01_str) != TPR_PASS ) {
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
		g_pKato->Log(LOG_COMMENT, TEXT("User reported decompress image successful!\n."));
	}
	
exit:
	free(tcfile01_str);
	free(tdfile01_str);
	return ((fail_log == 0)? TPR_PASS:TPR_FAIL);	
}

////////////////////////////////////////////////////////////////////////////////
// JpegFileSystemSD_E2E
//    JPEG and file system (SD Card) E2E test
//
// Parameters:
//    uMsg            Message code.
//    tpParam         Additional message-dependent data.
//    lpFTE           Function table entry that generated this call.
//
// Return value:
//    TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//    special conditions.

TESTPROCAPI JpegFileSystemSD_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    TUX_JPEG_PARA *tcfile01_str;
	TUX_JPEG_PARA *tdfile01_str;
	HWND hWnd1;
	int user_ret;
	
	unsigned char fail_log = 0;

	FILE *fp;
    
	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

	hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		return TPR_FAIL;
	}

	fp = _wfopen(RAW_IMAGE_SD_PATH, L"rb");
	if(fp == NULL){
		MessageBox(hWnd1,TEXT("File not found! Please copy raw image data file to SD card."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}

    g_pKato->Log(LOG_COMMENT, TEXT("This test will compress YUV files into JPEG"));
	tcfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tcfile01_str == NULL ){
		g_pKato->Log(LOG_COMMENT, TEXT("Fail in test case init!\n."));
		return TPR_FAIL;
	}
    
	tcfile01_str->fpt = FALSE;	//for performance test
	tcfile01_str->do_thumb = FALSE;
	
	tcfile01_str->infilename  = RAW_IMAGE_SD_PATH;
	tcfile01_str->outfilename = JPEG_IMAGE_SD_PATH;
	tcfile01_str->inwidth     = IMAGE_WIDTH;
	tcfile01_str->inheight    = IMAGE_HEIGHT;
	tcfile01_str->samplemode  = SAMPLE_MODE;
	tcfile01_str->imgquality  = JPG_QUALITY_LEVEL_2;
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress filename %s\n"),tcfile01_str->infilename);
	if( JPEGComExe(tcfile01_str) != TPR_PASS )	{
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Compress image failed!\n."));
		fail_log++;
		goto exit;
	}
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Compress output filename %s\n"),tcfile01_str->outfilename);


	g_pKato->Log(LOG_COMMENT, TEXT("This test will decode JPEG image and show on LCD."));
	tdfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tdfile01_str == NULL ){
		g_pKato->Log(LOG_FAIL, TEXT("Fail in test case init!\n."));
		free(tdfile01_str);
		return TPR_FAIL;
	}
	hWnd1 = GetForegroundWindow();
	if( hWnd1 == NULL ) {
		g_pKato->Log(LOG_FAIL, TEXT("JPEG Get hWnd faild\n"));
		free(tcfile01_str);
		free(tdfile01_str);
		return TPR_FAIL;
	}
	tdfile01_str->outfilename = NULL;
	tdfile01_str->fpt = FALSE;	//for performance test
	tdfile01_str->do_thumb = FALSE;
	
	tdfile01_str->infilename = JPEG_IMAGE_SD_PATH;
	g_pKato->Log(LOG_COMMENT, TEXT("JPEG Decompress filename %s\n"), tdfile01_str->infilename);
	if ( JPEGDecExe(tdfile01_str) != TPR_PASS ) {
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
		g_pKato->Log(LOG_COMMENT, TEXT("User reported decompress image successful!\n."));
	}

exit:
	free(tcfile01_str);
	free(tdfile01_str);
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
