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
#include "s3c6410_display_con.h"
#include "S3c6410_post_proc.h"
#include "SVE_API.h"
#include "JPGDrvHandler.h"

void DisplayJPEG (int, INT32, INT32, INT32, INT32, int);
UINT JPEGDec(void);
UINT JPEGCom(void);

///////////////////////////////////////////////////////////////////////////////
//
// @doc SAMPLESTRESSDLL
//
//
// @topic Dll Modules for CE Stress |
//
//	The simplest way to implement a CE Stress module is by creating a DLL
//  that can be managed by the stress harness.  Each stress DLL is loaded and 
//	run by a unique instance of the container: stressmod.exe.  The container
//  manages the duration of your module, collects and reports results to the
//  harness, and manages multiple test threads.
//
//	Your moudle is required to export an initialization function (<f InitializeStressModule>)
//	and a termination function (<f TerminateStressModule>).  Between these calls 
//  your module's main test function (<f DoStressIteration>) will be called in a 
//  loop for the duration of the module's run.  The harness will aggragate and report
//  the results of your module's run based on this function's return values.
//
//	You may wish to run several concurrent threads in your module.  <f DoStressIteration>
//	will be called in a loop on each thread.  In addition, you may implement per-thread
//  initialization and cleanup functions (<f InitializeTestThread> and <f CleanupTestThread>).
//	  
//	<nl>
//	Required functions:
//
//    <f InitializeStressModule> <nl>
//    <f DoStressIteration> <nl>
//    <f TerminateStressModule>
//
//  Optional functions:
//
//    <f InitializeTestThread> <nl>
//    <f CleanupTestThread>
//

//
// @topic Stress utilities |
//
//	Documentation for the utility functions in StressUtils.Dll can be
//  found on:
//
//     \\\\cestress\\docs\\stresss <nl>
//     %_WINCEROOT%\\private\\test\\stress\\stress\\docs
//
//
// @topic Sample code |
//
//	Sample code can be found at: 
//       <nl>\t%_WINCEROOT%\\private\\test\\stress\\stress\\samples <nl>
//
//	Actual module examples can be found at: 
//       <nl>%_WINCEROOT%\\private\\test\\stress\\stress\\modules
//

HANDLE g_hInst = NULL;
#define LCD_X		800
#define LCD_Y		480
typedef struct para_TUX_JPEG
{
	char *infilename;
	char *outfilename;
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
UINT JPEGDecExe(TUX_JPEG_PARA *fdin_str)
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
	fp = fopen(fdin_str->infilename, "rb");
	if(fp == NULL){
		LogFail(_T("file open error!\n"));
		return JPG_TC_FAIL;
	}
	handle = GetJPGDrvPtrDec();
	if(handle == NULL){
		LogFail(_T("Get JPEG driver handle pointer failed\n"));
		fclose(fp);
		return JPG_TC_FAIL;
	}
	
	//===== 2. figure out file size =====	
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	//===== 3. Buffer control, move from file to memory =====
	InBuf = (char*)GetDecInBuf(handle,fileSize);
	if(InBuf == NULL){
		//printf("Input buffer is NULL\n");
		fdin_str->may_oversize = TRUE;
		LogFail(_T("Input buffer is NULL"));
		fclose(fp);
		DeinitJPEGDec(handle);
		return JPG_TC_FAIL;
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
		LogFail(_T("Decoding failed\n"));
		fdin_str->may_oversize = TRUE;
		DeinitJPEGDec(handle);
		return JPG_TC_FAIL;
	}
	//===== 5. Get out buffer =====
	OutBuf = (char*)GetDecOutBuf(handle,&decSize);
	GetJPEGInfo(JPEG_GET_DECODE_WIDTH, &width);
	GetJPEGInfo(JPEG_GET_DECODE_HEIGHT, &height);
	GetJPEGInfo(JPEG_GET_SAMPING_MODE, &samplemode);
	LogWarn2(_T("Decode image info: width : %d height : %d samplemode : %d\n\n"), width, height, samplemode);
	OutPhyBuf = (char*)GetJPEGDecOutPhyBuf(handle);
	OutRGBPhyBuf = (char*)GetJPEGRGBPhyBuf(handle,LCD_X, LCD_Y);
	if(ConvertYCBYCRToRGB((int)OutPhyBuf, width, height, 
							  POST_SRC_YUV422_CRYCBY,
							  (int)OutRGBPhyBuf, LCD_X, LCD_Y,
							  POST_DST_RGB16) == FALSE){
			LogFail(_T("ConvertYCBYCRToRGB error\n"));
			DeinitJPEGDec(handle);
			return JPG_TC_FAIL;
	}
	// Show on LCD
	DisplayJPEG((int)OutRGBPhyBuf, LCD_X, LCD_Y, LCD_X, LCD_Y, 3);
	DeinitJPEGDec(handle);

	return JPG_TC_PASS;
}
// Compress YUV file into JPEG file.
UINT JPEGComExe(TUX_JPEG_PARA *fcin_str)
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

	handle = GetJPGDrvPtrEnc();	// Get JPEG driver handler
	if(handle == NULL) {
		LogFail(_T("Get JPEG driver handle pointer failed\n"));	
		DeInitJPEGEnc(handle);
		return JPG_TC_FAIL;
	}
	
	//===== 1. Open YUV file =====	
	fp = fopen(fcin_str->infilename, "rb");
	if(fp == NULL){
		LogFail(_T("file open error : %s\n"), fcin_str->infilename);
		DeInitJPEGEnc(handle);
		return JPG_TC_FAIL;
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
		LogFail(_T("Fail in seting JPG Compress Config\n"));
		fcin_str->may_oversize = TRUE;
		fclose(fp);
		DeInitJPEGEnc(handle);
		return JPG_TC_FAIL;
	}
	
	//===== 2. figure out file size =====
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	InBuf = (char*)GetEncInBuf(handle, fileSize);
	if(InBuf == NULL){
		LogFail(_T("In Buffer is NULL, failed!\n"));
		fclose(fp);
		DeInitJPEGEnc(handle);
		return JPG_TC_FAIL;
	}
	fread(InBuf, 1, fileSize, fp);

	fclose(fp);
	LogVerbose(_T("InBuf get OK!!!\n"));
	
	ExifInfo = (ExifFileInfo *)malloc(sizeof(ExifFileInfo));
	if(ExifInfo == NULL) {
		LogFail(_T("Allocate ExifInfo memory fail!\n"));
		DeInitJPEGEnc(handle);
		return JPG_TC_FAIL;
	}
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

	ret = JPEG_OK;

	if(ret != JPEG_OK){
		LogFail(_T("Execute JPEG Compress failed!\n"));
		DeInitJPEGEnc(handle);
		if(ExifInfo != NULL)
			free(ExifInfo);				
		return JPG_TC_FAIL;
	}
	//===== 5. Get out buffer  =====
	OutBuf = (char*)GetEncOutBuf(handle, &frameSize);
	if(OutBuf == NULL){
		LogFail(_T("Failed, Out buffer is NULL!\n"));
		DeInitJPEGEnc(handle);		
		if(ExifInfo != NULL)
			free(ExifInfo);
		return JPG_TC_FAIL;
	}

	fp = fopen(fcin_str->outfilename, "wb");
	fwrite(OutBuf, 1, frameSize, fp);
	fclose(fp);	
	DeInitJPEGEnc(handle);
	if(ExifInfo != NULL)
		free(ExifInfo);
		
	
	return JPG_TC_PASS;
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
	*pnThreads = 1;
	
	// !! You must call this before using any stress utils !!

	LogVerbose(_T("Start InitializeStressModule\n"));
	InitializeStressUtils (
							_T("JPEG_STRESS"),									// Module name to be used in logging
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

	srand(GetTickCount());

	// (Optional) 
	// Get the number of modules of this type (i.e. same exe or dll)
	// that are currently running.  This allows you to bail (return FALSE)
	// if your module can not be run with more than a certain number of
	// instances.
	
	LONG count = GetRunningModuleCount(g_hInst);
    if (count > 1)
        return FALSE;

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


	if ( 0 == (rand() & 1) )
		st_result = JPEGCom();
	else
		st_result = JPEGDec();		
		
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
	// It is not currently used by the harness.

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

#define TJPEGDEC_FI_NUM	4
// Decompressed a JPEG image and show on LCD
UINT JPEGDec(void)
{
	TUX_JPEG_PARA *tdfile01_str;
	char* list[TJPEGDEC_FI_NUM] = {"\\Storage Card\\t420_640_480.jpg", "\\Storage Card\\t422_1280_960.jpg", 
                                   "\\Storage Card\\t444_2048_1024.jpg", "\\Storage Card\\tgray_1600_1200.jpg"};
	unsigned char fail_log = 0;
	UINT i;
	
    LogComment(_T("This test will decode JPEG image and show on LCD."));
	tdfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tdfile01_str == NULL ){
		LogFail(_T("Fail in test case init!\n."));
		return CESTRESS_FAIL;
	}
	
	tdfile01_str->outfilename = NULL;
	tdfile01_str->fpt = FALSE;	//for performance test
	tdfile01_str->do_thumb = FALSE;
	i = (rand() & 3); // Random i from 0 ~ 3
	LogWarn2(_T("i number is [%d]\n."),i);
	tdfile01_str->infilename = list[i];
	LogWarn2(_T("JPEG Decompress filename %s\n"),tdfile01_str->infilename);
	if ( JPEGDecExe(tdfile01_str) != JPG_TC_PASS ) {
		LogFail(_T("JPEG Decompress image failed!\n."));
		fail_log++;
	}

	free(tdfile01_str);
    return ((fail_log == 0)? CESTRESS_PASS:CESTRESS_FAIL);
}

#define TJPEGCOM_FI_NUM	4
UINT JPEGCom(void)
{
	TUX_JPEG_PARA *tcfile01_str;
	UINT i;
	unsigned char fail_log = 0;
	char* ilist[TJPEGCOM_FI_NUM] = {"\\Storage Card\\t640_480.yuv", "\\Storage Card\\t1280_960.yuv", "\\Storage Card\\t1600_1200.yuv", "\\Storage Card\\t2048_1024.yuv"};
	char* olist[TJPEGCOM_FI_NUM] = {"\\Storage Card\\to002_A.jpg", "\\Storage Card\\to002_B.jpg", "\\Storage Card\\to002_C.jpg", "\\Storage Card\\to002_D.jpg"};
	INT32 inrawwith[TJPEGCOM_FI_NUM] = {640,1280,1600,2048};
	INT32 inrawheigh[TJPEGCOM_FI_NUM] = {480,960,1200,1024};
	IMAGE_QUALITY_TYPE_T outquality[TJPEGCOM_FI_NUM] = {JPG_QUALITY_LEVEL_2,JPG_QUALITY_LEVEL_2,JPG_QUALITY_LEVEL_1,JPG_QUALITY_LEVEL_3};
	SAMPLE_MODE_T outsampmod[TJPEGCOM_FI_NUM] = {JPG_422,JPG_422,JPG_422,JPG_422};
	
    LogComment(_T("This test will compress YUV files into JPEG"));
	tcfile01_str = (TUX_JPEG_PARA*)malloc(sizeof(TUX_JPEG_PARA));
	if( tcfile01_str == NULL ){
		LogFail(_T("Fail in test case init!\n."));
		return CESTRESS_FAIL;
	}
    
	tcfile01_str->fpt = FALSE;	//not for performance test
	tcfile01_str->do_thumb = FALSE;
	i = (rand() & 3); // Random i from 0 ~ 3

		tcfile01_str->infilename = ilist[i];
		tcfile01_str->outfilename = olist[i];
		tcfile01_str->inwidth = inrawwith[i];
		tcfile01_str->inheight = inrawheigh[i];
		tcfile01_str->samplemode = outsampmod[i];
		tcfile01_str->imgquality = outquality[i];
		LogWarn2(_T("JPEG Compress filename %s\n"),tcfile01_str->infilename);
		if( JPEGComExe(tcfile01_str) != JPG_TC_PASS )	{
			LogFail(_T("JPEG Compress image failed!\n."));
			fail_log++;
		}
		else
			LogWarn2(_T("JPEG Compress output filename %s\n"),tcfile01_str->outfilename);

	free(tcfile01_str);
	return ((fail_log == 0)? CESTRESS_PASS:CESTRESS_FAIL);	
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
		LogFail(_T("[VDE:ERR] VDE0 Open Device Failed\n"));
		return;
	}

	// Request FIMD Win0 H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		LogFail(_T("[VDE:ERR] IOCTL_SVE_RSC_REQUEST_FIMD_WIN0 Failed\n"));
		return;
	}

	// Request Post Processor H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{ 
		LogFail(_T("[VDE:ERR] IOCTL_SVE_RSC_REQUEST_POST Failed\n"));
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
				LogFail(_T("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n"));
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
				LogFail(_T("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n"));
				return;
			}

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
			{
				LogFail(_T("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n"));
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
				LogFail(_T("[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n"));
				return;
			}

			tParamBuffer.dwBufferRGBY = srcAddr;
			tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+srcwidth*srcheight;
			tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+srcwidth*srcheight/4;
			tParamBuffer.bWaitForVSync = FALSE;

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
			{
				LogFail(_T("[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
				return;
			}

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
			{
				LogFail(_T("[VDE:ERR] IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER Failed\n"));
				return;
			}

			if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
			{
				LogFail(_T("[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_START Failed\n"));
				return;
			}

	Sleep(displayTime*1000);

	//--------------------------------------------
	// close surface
	//---------------------------------------------
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_STOP, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		LogFail(_T("[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_STOP Failed\n"));
		return;
	}
	// Release FIMD Win0 H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		LogFail(_T("[VDE:ERR] IOCTL_SVE_RSC_RELEASE_FIMD_WIN0 Failed\n"));
		return;
	}

	// Release Post Processor H/W Resource to Video Engine Driver for Local Path
	if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
	{
		LogFail(_T("[VDE:ERR] IOCTL_SVE_RSC_RELEASE_POST Failed\n"));
		return;
	}

	if ( CloseHandle(hVideoDrv) == 0 ) {
		LogFail(_T("[VDE:ERR] CloseHandle Failed\n"));
		CloseHandle(hVideoDrv);
	}


}