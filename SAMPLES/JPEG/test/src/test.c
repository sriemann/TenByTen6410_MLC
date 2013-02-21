//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    jpg_app.c

Abstract:

    This file implements JPEG Test Application.

Functions:


Notes:

--*/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
// Platform Dependent
#include <SVEDriverAPI.h>
#include "JPGApi.h"

 

#define FPS            1
#define DEBUG        0    


#define TEST_DECODE                    1  // enable decoder test
#define TEST_DECODE_OUTPUT_YUV422    0  // output file format is YUV422(non-interleaved)
#define TEST_DECODE_OUTPUT_YCBYCR    0  // output file format is YCBYCR(interleaved)
#define TEST_DECODE_OUTPUT_RGB16    0  // output file format is RGB16
#define TEST_DECODE_OUTPUT_LCD        1  // output will display to LCD during DISPLAY_TIME

#define TEST_ENCODE                    0 // enable encoder test
#define TEST_ENCODE_WITH_EXIF        1 // encoded jpg file will include Exif info
#define TEST_ENCODE_WITH_THUMBNAIL    0 // enable thumbnail encoding

#if (TEST_DECODE == 1)
    #define CTRL_FILE_NAME    "fname_dec.txt"
#elif (TEST_ENCODE == 1)
    #define CTRL_FILE_NAME    "fname_enc.txt"
#endif


#define PHY_ADDR_FRAME_BUFFER 0x57400000
#define LCD_X        800
#define LCD_Y        480
#define DISPLAY_TIME    5//sec

#define R_RGB565(x)        (unsigned char) (((x) >> 8) & 0xF8)
#define G_RGB565(x)        (unsigned char) (((x) >> 3) & 0xFC)
#define B_RGB565(x)        (unsigned char) ((x) << 3)

void TestDecoder();
void TestEncoder();
void DecodeFileOutYCBYCR(char *OutBuf, UINT32 streamSize, char *filename);
void DecodeFileOutYUV422(char *OutBuf, UINT32 streamSize, char *filename);
void makeExifParam(ExifFileInfo *exifFileInfo);
void DisplayJPEG (int srcAddr, 
                  INT32 srcwidth, INT32 srcheight, 
                  INT32 dstwidth, INT32 dstheight,
                  int displayTime);
BOOL ConvertYCBYCRToRGB(int inBuf, INT32 srcwidth, INT32 srcheight, 
                        unsigned long srcType,
                        int outBuf, INT32 dstwidth, INT32 dstheight,
                        unsigned long dstType);
void DecodeFileOutRGB16ToPPM(unsigned char *p_img, int wd, int hi, char *filename);
void printD(char* fmt, ...);

/*
*******************************************************************************
Name            : main
Description     : Main function
Parameter       :
Return Value    : SUCCESS or FAILURE
*******************************************************************************
*/

int WINAPI WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow)
{
#if (TEST_DECODE == 1)
    TestDecoder();
#elif (TEST_ENCODE == 1)
    TestEncoder();
#endif

    return 1;
}

/*
*******************************************************************************
Name            : TestDecoder
Description     : To test Decoder
Parameter       : imageType - JPG_YCBYCR or JPG_RGB16
Return Value    : void
*******************************************************************************
*/
void TestDecoder()
{
    char *InBuf = NULL;
    char *OutBuf = NULL;
    char *OutPhyBuf = NULL;
    char *OutRGBBuf = NULL;
    char *OutRGBPhyBuf = NULL;
    FILE *fp;
    FILE *CTRfp;
    UINT32 fileSize;
    UINT32 streamSize;
    void *handle;
    INT32 width, height, samplemode;
    JPEG_ERRORTYPE ret;
    char outFilename[128];
    char inFilename[128];
    BOOL result = TRUE;
#if (FPS == 1)
    INT32    decodeTime;
#endif
    DWORD    startTime;


    RETAILMSG(1,(L"------------------------Decoder Test Start ---------------------\n"));

    //////////////////////////////////////////////////////////////
    // 0. Get input/output file name                            //
    //////////////////////////////////////////////////////////////
    CTRfp = fopen(CTRL_FILE_NAME, "rb");
    if(CTRfp == NULL){
        RETAILMSG(1,(L"file open error : %s\n", CTRL_FILE_NAME));
        return;
    }

    do{
        memset(outFilename, 0x00, sizeof(outFilename));
        memset(inFilename, 0x00, sizeof(inFilename));

        fscanf(CTRfp, "%s", inFilename);

        if(inFilename[0] == '#'){
            RETAILMSG(1,(L"------------------------Decoder Test Done ---------------------\n"));
            fclose(CTRfp);
            return;
        }

        fscanf(CTRfp, "%s", outFilename);

        if(inFilename == NULL || outFilename == NULL){
            RETAILMSG(1,(L"read file error\n"));
            RETAILMSG(1,(L"------------------------Decoder Test Done ---------------------\n"));
            fclose(CTRfp);
            return;
        }

        RETAILMSG(1,(L"inFilename : %s \noutFilename : %s\n", inFilename, outFilename));
        //////////////////////////////////////////////////////////////
        // 1. handle Init                                           //
        //////////////////////////////////////////////////////////////
        #if (FPS == 1)
            decodeTime = GetTickCount();
        #endif
        handle = SsbSipJPEGDecodeInit();
        #if (FPS == 1)        
            decodeTime = GetTickCount() - decodeTime;
            RETAILMSG(1,(L"Initialization Time : %d \n", decodeTime));
        #endif
        if(handle == NULL){
            RETAILMSG(1,(L"Decoder Init failed\n"));
            break;
        }

        //////////////////////////////////////////////////////////////
        // 2. open JPEG file to decode                              //
        //////////////////////////////////////////////////////////////
        fp = fopen(inFilename, "rb");
        if(fp == NULL){
            result = FALSE;
            RETAILMSG(1,(L"file open error : %s\n", inFilename));
            break;
        }
        fseek(fp, 0, SEEK_END);
        fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        RETAILMSG(1,(L"filesize : %d\n", fileSize));

        //////////////////////////////////////////////////////////////
        // 3. get Input buffer address                              //
        //////////////////////////////////////////////////////////////
        InBuf = SsbSipJPEGGetDecodeInBuf(handle, fileSize);
        if(InBuf == NULL){
            RETAILMSG(1,(L"Input buffer is NULL\n"));
            result = FALSE;
            break;
        }

        RETAILMSG(1,(L"inBuf : 0x%x\n", InBuf));

        //////////////////////////////////////////////////////////////
        // 4. put JPEG frame to Input buffer                        //
        //////////////////////////////////////////////////////////////
        fread(InBuf, 1, fileSize, fp);
        fclose(fp);

        
        //////////////////////////////////////////////////////////////
        // 5. Decode JPEG frame                                     //
        //////////////////////////////////////////////////////////////
        #if (FPS == 1)
            decodeTime = GetTickCount();
        #endif

        ret = SsbSipJPEGDecodeExe(handle);

        #if (FPS == 1)        
            decodeTime = GetTickCount() - decodeTime;
            RETAILMSG(1,(L"decodeTime : %d \n", decodeTime));
        #endif

        if(ret != JPEG_OK){
            RETAILMSG(1,(L"Decoding failed\n"));
            result = FALSE;
            break;
        }

        //////////////////////////////////////////////////////////////
        // 6. get Output buffer address                             //
        //////////////////////////////////////////////////////////////
        OutBuf = SsbSipJPEGGetDecodeOutBuf(handle, &streamSize);
        if(OutBuf == NULL){
            RETAILMSG(1,(L"Output buffer is NULL\n"));
            result = FALSE;
            break;
        }
        printD("OutBuf : 0x%x streamsize : %d\n", OutBuf, streamSize);

        //////////////////////////////////////////////////////////////
        // 7. get decode config.                                    //
        //////////////////////////////////////////////////////////////
        SsbSipJPEGGetConfig(JPEG_GET_DECODE_WIDTH, &width);
        SsbSipJPEGGetConfig(JPEG_GET_DECODE_HEIGHT, &height);
        SsbSipJPEGGetConfig(JPEG_GET_SAMPING_MODE, &samplemode);

        RETAILMSG(1,(L"width : %d height : %d samplemode : %d\n\n", width, height, samplemode));

        //////////////////////////////////////////////////////////////
        // 8. wirte output file                                     //
        //////////////////////////////////////////////////////////////

#if (TEST_DECODE_OUTPUT_YCBYCR == 1)
        DecodeFileOutYCBYCR(OutBuf, streamSize, outFilename);    // YCBYCR interleaved
#elif (TEST_DECODE_OUTPUT_YUV422 == 1)
        DecodeFileOutYUV422(OutBuf, streamSize, outFilename);    // yuv422 non-interleaved
#elif (TEST_DECODE_OUTPUT_RGB16 == 1)                            //RGB16
        OutPhyBuf = SsbSipJPEGGetDecodeOutPhyBuf(handle);
        OutRGBPhyBuf = SsbSipJPEGGetRGBPhyBuf(handle, LCD_X, LCD_Y);
        if(ConvertYCBYCRToRGB((int)OutPhyBuf, width, height, 
                              POST_SRC_YUV422_CRYCBY,
                              (int)OutRGBPhyBuf, LCD_X, LCD_Y,
                              POST_DST_RGB16) == FALSE){
            RETAILMSG(1,(L"ConvertYCBYCRToRGB error\n"));
            result = FALSE;
            break;
        }
        OutRGBBuf = SsbSipJPEGGetRGBBuf(handle, LCD_X, LCD_Y);
        DecodeFileOutRGB16ToPPM(OutRGBBuf, LCD_X, LCD_Y, outFilename); 
#elif(TEST_DECODE_OUTPUT_LCD == 1)
        OutPhyBuf = SsbSipJPEGGetDecodeOutPhyBuf(handle);
        OutRGBPhyBuf = SsbSipJPEGGetRGBPhyBuf(handle, LCD_X, LCD_Y);
        startTime = GetTickCount();
        if(ConvertYCBYCRToRGB((int)OutPhyBuf, width, height, 
                              POST_SRC_YUV422_CRYCBY,
                              (int)OutRGBPhyBuf, LCD_X, LCD_Y,
                              POST_DST_RGB16) == FALSE){
            RETAILMSG(1,(L"ConvertYCBYCRToRGB error\n"));
            result = FALSE;
            break;
        }

        RETAILMSG(1,(L"converting time : %d\n\n", GetTickCount() - startTime));
        RETAILMSG(1,(L"\n\n This image will be disappeared after %d seconds......\n\n", DISPLAY_TIME));
        DisplayJPEG((int)OutRGBPhyBuf, LCD_X, LCD_Y, LCD_X, LCD_Y, DISPLAY_TIME);
#endif

        //////////////////////////////////////////////////////////////
        // 9. finalize handle                                      //
        //////////////////////////////////////////////////////////////
        SsbSipJPEGDecodeDeInit(handle);
        Sleep(5);
    }while(1);

    if(result == FALSE){
        SsbSipJPEGDecodeDeInit(handle);
    }

    fclose(CTRfp);
    RETAILMSG(1,(L"------------------------Decoder Test Done ---------------------\n"));
}
/*
*******************************************************************************
Name            : TestEncoder
Description     : To test Encoder
Parameter       : imageType - JPG_YCBYCR or JPG_RGB16
Return Value    : void
*******************************************************************************
*/
void TestEncoder()
{
    char *InBuf = NULL;
    char *InThumbBuf = NULL;
    char *OutBuf = NULL;
    FILE *fp;
    FILE *CTRfp;
    JPEG_ERRORTYPE ret;
    UINT32 fileSize;
    UINT32 frameSize;
    void *handle;
    ExifFileInfo *ExifInfo;
    char outFilename[128];
    char inFilename[128];
    char widthstr[8], heightstr[8];
    INT32 width, height;
    BOOL result = TRUE;
#if (FPS == 1)
    INT32    encodeTime;
#endif


    RETAILMSG(1,(L"------------------------Encoder Test start---------------------\n"));
    //////////////////////////////////////////////////////////////
    // 0. Get input/output file name                            //
    //////////////////////////////////////////////////////////////
    CTRfp = fopen(CTRL_FILE_NAME, "rb");
    if(CTRfp == NULL){
        RETAILMSG(1,(L"file open error : %s\n", CTRL_FILE_NAME));
        return;
    }

    do{
        memset(outFilename, 0x00, sizeof(outFilename));
        memset(inFilename, 0x00, sizeof(inFilename));
        memset(widthstr, 0x00, sizeof(widthstr));
        memset(heightstr, 0x00, sizeof(heightstr));

        fscanf(CTRfp, "%s", inFilename);
        if(inFilename[0] == '#'){
            RETAILMSG(1,(L"------------------------Encoder Test Done---------------------\n"));
            fclose(CTRfp);
            return;
        }

        fscanf(CTRfp, "%s", outFilename);
        fscanf(CTRfp, "%s", widthstr);
        fscanf(CTRfp, "%s", heightstr);
        width = (INT32)atoi(widthstr);
        height = (INT32)atoi(heightstr);

        if(inFilename == NULL || outFilename == NULL){
            RETAILMSG(1,(L"read file error\n"));
            RETAILMSG(1,(L"------------------------Encoder Test Done---------------------\n"));
            fclose(CTRfp);
            return;
        }


        RETAILMSG(1,(L"inFilename : %s \noutFilename : %s width : %d height : %d\n", 
                inFilename, outFilename, width, height));
        //////////////////////////////////////////////////////////////
        // 1. handle Init                                           //
        //////////////////////////////////////////////////////////////
        #if (FPS == 1)
            encodeTime = GetTickCount();
        #endif
        handle = SsbSipJPEGEncodeInit();
        #if (FPS == 1)        
            encodeTime = GetTickCount() - encodeTime;
            RETAILMSG(1,(L"Initialization Time : %d \n", encodeTime));
        #endif
        if(handle == NULL)
            break;

        //////////////////////////////////////////////////////////////
        // 2. set decode config.                                    //
        //////////////////////////////////////////////////////////////
        if((ret = SsbSipJPEGSetConfig(JPEG_SET_SAMPING_MODE, JPG_422)) != JPEG_OK){
            result = FALSE;
            break;
        }
        if((ret = SsbSipJPEGSetConfig(JPEG_SET_ENCODE_WIDTH, width)) != JPEG_OK){
            result = FALSE;
            break;
        }
        if((ret = SsbSipJPEGSetConfig(JPEG_SET_ENCODE_HEIGHT, height)) != JPEG_OK){
            result = FALSE;
            break;
        }
        if((ret = SsbSipJPEGSetConfig(JPEG_SET_ENCODE_QUALITY, JPG_QUALITY_LEVEL_2)) != JPEG_OK){
            result = FALSE;
            break;
        }
#if (TEST_ENCODE_WITH_THUMBNAIL == 1)
        if((ret = SsbSipJPEGSetConfig(JPEG_SET_ENCODE_THUMBNAIL, TRUE)) != JPEG_OK){
            result = FALSE;
            break;
        }
        if((ret = SsbSipJPEGSetConfig(JPEG_SET_THUMBNAIL_WIDTH, 160)) != JPEG_OK){
            result = FALSE;
            break;
        }
        if((ret = SsbSipJPEGSetConfig(JPEG_SET_THUMBNAIL_HEIGHT, 120)) != JPEG_OK){
            result = FALSE;
            break;
        }
#endif

        //////////////////////////////////////////////////////////////
        // 3. open JPEG file to decode                              //
        //////////////////////////////////////////////////////////////
        fp = fopen(inFilename, "rb");
        if(fp == NULL){
            RETAILMSG(1,(L"file open error : %s\n", inFilename));
            result = FALSE;
            break;
        }
        fseek(fp, 0, SEEK_END);
        fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        //////////////////////////////////////////////////////////////
        // 4. get Input buffer address                              //
        //////////////////////////////////////////////////////////////
        printD("filesize : %d\n", fileSize);
        InBuf = SsbSipJPEGGetEncodeInBuf(handle, fileSize);
        if(InBuf == NULL){
            result = FALSE;
            break;
        }
        printD("inBuf : 0x%x\n", InBuf);

        //////////////////////////////////////////////////////////////
        // 5. put YUV stream to Input buffer                        //
        //////////////////////////////////////////////////////////////
        fread(InBuf, 1, fileSize, fp);
        fclose(fp);

    
        //////////////////////////////////////////////////////////////
        // 6. Make Exif info parameters                             //
        //////////////////////////////////////////////////////////////
        ExifInfo = (ExifFileInfo *)malloc(sizeof(ExifFileInfo));
        memset(ExifInfo, 0x00, sizeof(ExifFileInfo));
        makeExifParam(ExifInfo);

        //////////////////////////////////////////////////////////////
        // 7. Encode YUV stream                                     //
        //////////////////////////////////////////////////////////////
        #if (FPS == 1)
            encodeTime = GetTickCount();
        #endif

        #if (TEST_ENCODE_WITH_EXIF == 1)
        ret = SsbSipJPEGEncodeExe(handle, ExifInfo);    //with Exif
        #else
        ret = SsbSipJPEGEncodeExe(handle, NULL);         //No Exif
        #endif

        #if (FPS == 1)        
            encodeTime = GetTickCount() - encodeTime;
            RETAILMSG(1,(L"encodeTime : %d \n", encodeTime));
        #endif
        if(ret != JPEG_OK){
            result = FALSE;
            break;
        }
    
        //////////////////////////////////////////////////////////////
        // 8. get output buffer address                             //
        //////////////////////////////////////////////////////////////
        OutBuf = SsbSipJPEGGetEncodeOutBuf(handle, &frameSize);
        if(OutBuf == NULL){
            result = FALSE;
            break;
        }
        
        printD("OutBuf : 0x%x freamsize : %d\n", OutBuf, frameSize);

        //////////////////////////////////////////////////////////////
        // 9. write JPEG result file                                //
        //////////////////////////////////////////////////////////////
        fp = fopen(outFilename, "wb");
        fwrite(OutBuf, 1, frameSize, fp);
        fclose(fp);

        //////////////////////////////////////////////////////////////
        // 10. finalize handle                                      //
        //////////////////////////////////////////////////////////////
        SsbSipJPEGEncodeDeInit(handle);
        free(ExifInfo);
        Sleep(5);
    }while(1);

    if(result == FALSE){
        SsbSipJPEGEncodeDeInit(handle);
        if(ExifInfo != NULL)
            free(ExifInfo);
    }
    fclose(CTRfp);
    RETAILMSG(1,(L"------------------------Encoder Test Done---------------------\n"));
}
/*
*******************************************************************************
Name            : makeExifParam
Description     : To make exif input parameter
Parameter       : 
Return Value    : exifFileInfo - exif input parameter
*******************************************************************************
*/
void makeExifParam(ExifFileInfo *exifFileInfo)
{
    strcpy(exifFileInfo->Make,"Samsung SYS.LSI make");;
    strcpy(exifFileInfo->Model,"Samsung 2007 model");
    strcpy(exifFileInfo->Version,"version 1.0.2.0");
    strcpy(exifFileInfo->DateTime,"2007:05:16 12:32:54");
    strcpy(exifFileInfo->CopyRight,"Samsung Electronics@2007:All rights reserved");

    exifFileInfo->Height                    = 320;
    exifFileInfo->Width                        = 240;
    exifFileInfo->Orientation                = 1; // top-left
    exifFileInfo->ColorSpace                = 1;
    exifFileInfo->Process                    = 1;
    exifFileInfo->Flash                        = 0;
    exifFileInfo->FocalLengthNum            = 1;
    exifFileInfo->FocalLengthDen            = 4;
    exifFileInfo->ExposureTimeNum            = 1;
    exifFileInfo->ExposureTimeDen            = 20;
    exifFileInfo->FNumberNum                = 1;
    exifFileInfo->FNumberDen                = 35;
    exifFileInfo->ApertureFNumber            = 1;
    exifFileInfo->SubjectDistanceNum        = -20;
    exifFileInfo->SubjectDistanceDen        = -7;
    exifFileInfo->CCDWidth                    = 1;
    exifFileInfo->ExposureBiasNum            = -16;
    exifFileInfo->ExposureBiasDen            = -2;
    exifFileInfo->WhiteBalance                = 6;
    exifFileInfo->MeteringMode                = 3;
    exifFileInfo->ExposureProgram            = 1;
    exifFileInfo->ISOSpeedRatings[0]        = 1;
    exifFileInfo->ISOSpeedRatings[1]        = 2;
    exifFileInfo->FocalPlaneXResolutionNum    = 65;
    exifFileInfo->FocalPlaneXResolutionDen    = 66;
    exifFileInfo->FocalPlaneYResolutionNum    = 70;
    exifFileInfo->FocalPlaneYResolutionDen    = 71;
    exifFileInfo->FocalPlaneResolutionUnit    = 3;
    exifFileInfo->XResolutionNum            = 48;
    exifFileInfo->XResolutionDen            = 20;
    exifFileInfo->YResolutionNum            = 48;
    exifFileInfo->YResolutionDen            = 20;
    exifFileInfo->RUnit                        = 2;
    exifFileInfo->BrightnessNum                = -7;
    exifFileInfo->BrightnessDen                = 1;

    strcpy(exifFileInfo->UserComments,"Usercomments");
}
/*
*******************************************************************************
Name            : DecodeFileOutYUV422
Description     : To change YCBYCR to YUV422, and write result file.
Parameter       :
Return Value    : void
*******************************************************************************
*/
void DecodeFileOutYUV422(char *OutBuf, UINT32 streamSize, char *filename)
{
    UINT32    i;
    UINT32  indexY, indexCB, indexCR;
    char *Buf;
    FILE *fp;

    Buf = (char *)malloc(MAX_YUV_SIZE);
    memset(Buf, 0x00, MAX_YUV_SIZE);

    printD("convertyuvformat\n");
    indexY = 0;
    indexCB = streamSize >> 1;
    indexCR = indexCB+(streamSize >> 2);

    printD("indexY(%ld), indexCB(%ld), indexCR(%ld)\n", indexY, indexCB, indexCR);

    for(i = 0; i < streamSize; i++)
    {
        if((i%2) == 0)
            Buf[indexY++] = OutBuf[i];

        if((i%4) == 1) 
            Buf[indexCB++] = OutBuf[i];

        if((i%4) == 3) 
            Buf[indexCR++] = OutBuf[i];
    }

    fp = fopen(filename, "wb");
    fwrite(Buf, 1, streamSize, fp);
    fclose(fp);
    free(Buf);

}
/*
*******************************************************************************
Name            : DecodeFileOutYCBYCR
Description     : To write result YCBYCR file.
Parameter       :
Return Value    : void
*******************************************************************************
*/
void DecodeFileOutYCBYCR(char *OutBuf, UINT32 streamSize, char *filename)
{
    FILE *fp;

    fp = fopen(filename, "wb");
    fwrite(OutBuf, 1, streamSize, fp);
    fclose(fp);

}

#if (DEBUG == 1)
void printD(char* fmt, ...) 
{

    char str[512];
    wchar_t wstr[512];
    int    alen;
    int wlen;

    vsprintf(str, fmt, (char *)(&fmt+1));
    alen = strlen(str);
    wlen = MultiByteToWideChar(CP_ACP, 0, str, alen, 0, 0);
    MultiByteToWideChar(CP_ACP, 0, str, alen, wstr, wlen);
    wstr[wlen] = 0;
    NKDbgPrintfW(wstr);
}
#else
void printD(char* fmd, ...)
{
}
#endif


/*
*******************************************************************************
Name            : DecodeFileOutYCBYCR
Description     : To write result YCBYCR file.
Parameter       :
Return Value    : void
*******************************************************************************
*/
void DisplayJPEG (int srcAddr, 
                  INT32 srcwidth, INT32 srcheight, 
                  INT32 dstwidth, INT32 dstheight,
                  int displayTime)
{
    HANDLE                        hVideoDrv = INVALID_HANDLE_VALUE;
    SVEARG_FIMD_WIN_MODE        tParamMode;
    SVEARG_FIMD_WIN_FRAMEBUFFER tParamFB;
    SVEARG_FIMD_WIN_COLORKEY    tParamCKey;
    SVEARG_FIMD_WIN_ALPHA        tParamAlpha;
    SVEARG_POST_PARAMETER        tParamPost;
    SVEARG_POST_BUFFER            tParamBuffer;
    DWORD                        dwWinNum;
    DWORD                        dwBytes;    

    //-----------------------
    // Open Video Driver
    //-----------------------
    hVideoDrv = CreateFile( L"VDE0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hVideoDrv == INVALID_HANDLE_VALUE)
    {
        RETAILMSG(1,(L"[VDE:ERR] VDE0 Open Device Failed\n"));
        return;
    }

    // Request FIMD Win0 H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_REQUEST_FIMD_WIN0 Failed\n"));
        return;
    }

    // Request Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_REQUEST_POST Failed\n"));
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
                RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n"));
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
                RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n"));
                return;
            }

            if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n"));
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
                RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n"));
                return;
            }

            tParamBuffer.dwBufferRGBY = srcAddr;
            tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+srcwidth*srcheight;
            tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+srcwidth*srcheight/4;
            tParamBuffer.bWaitForVSync = FALSE;

            if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
                return;
            }

            if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER Failed\n"));
                return;
            }

            if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_START Failed\n"));
                return;
            }

    Sleep(displayTime*1000);

    //--------------------------------------------
    // close surface
    //---------------------------------------------
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_STOP, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_STOP Failed\n"));
        return;
    }
    // Release FIMD Win0 H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_RELEASE_FIMD_WIN0 Failed\n"));
        return;
    }

    // Release Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_RELEASE_POST Failed\n"));
        return;
    }

    CloseHandle(hVideoDrv);


}

BOOL ConvertYCBYCRToRGB(int inBuf, INT32 srcwidth, INT32 srcheight, 
                        unsigned long srcType,
                        int outBuf, INT32 dstwidth, INT32 dstheight,
                        unsigned long dstType)
{
    HANDLE hPostDrv = INVALID_HANDLE_VALUE;
    SVEARG_POST_PARAMETER tParamPost;
    SVEARG_POST_BUFFER tParamBuffer;
    DWORD dwBytes;

    hPostDrv = CreateFile( L"VDE0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hPostDrv == INVALID_HANDLE_VALUE)
    {
        RETAILMSG(1,(L"[VDE:ERR] VDE0 Open Device Failed\n"));
        return FALSE;
    }
    
    // Request Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hPostDrv, IOCTL_SVE_RSC_REQUEST_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_REQUEST_POST Failed\n"));
        return    FALSE;
    }

    tParamPost.dwOpMode = POST_PER_FRAME_MODE;
    tParamPost.dwScanMode = POST_PROGRESSIVE;
    tParamPost.dwSrcType = srcType;
    tParamPost.dwSrcBaseWidth = srcwidth;
    tParamPost.dwSrcBaseHeight = srcheight;
    tParamPost.dwSrcWidth = srcwidth;
    tParamPost.dwSrcHeight = srcheight;
    tParamPost.dwSrcOffsetX = 0;
    tParamPost.dwSrcOffsetY = 0;
    tParamPost.dwDstType = dstType;
    tParamPost.dwDstBaseWidth = dstwidth;
    tParamPost.dwDstBaseHeight = dstheight;
    tParamPost.dwDstWidth = dstwidth;
    tParamPost.dwDstHeight = dstheight;
    tParamPost.dwDstOffsetX = 0;
    tParamPost.dwDstOffsetY = 0;

    if ( !DeviceIoControl(hPostDrv, IOCTL_SVE_POST_SET_PROCESSING_PARAM, &tParamPost, sizeof(SVEARG_POST_PARAMETER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n"));
        return FALSE;
    }

    // Source Address
    tParamBuffer.dwBufferRGBY = (DWORD)inBuf;
    tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+srcwidth*srcheight;
    tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+srcwidth*srcheight/4;
    tParamBuffer.bWaitForVSync = FALSE;

    if ( !DeviceIoControl(hPostDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
        return FALSE;
    }

    // Destination Address
    tParamBuffer.dwBufferRGBY = (DWORD)outBuf;
    tParamBuffer.dwBufferCb = 0;
    tParamBuffer.dwBufferCr = 0;
    tParamBuffer.bWaitForVSync = FALSE;

    if ( !DeviceIoControl(hPostDrv, IOCTL_SVE_POST_SET_DESTINATION_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_DESTINATION_BUFFER Failed\n"));
        return FALSE;
    }

    // Post Processing Start
    if ( !DeviceIoControl(hPostDrv, IOCTL_SVE_POST_SET_PROCESSING_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_START Failed\n"));
        return FALSE;
    }

    // Wait for Post Processing Finished
    if ( !DeviceIoControl(hPostDrv, IOCTL_SVE_POST_WAIT_PROCESSING_DONE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_WAIT_PROCESSING_DONE Failed\n"));
        return FALSE;
    }

    // Release Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hPostDrv, IOCTL_SVE_RSC_RELEASE_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_RELEASE_POST Failed\n"));
        return FALSE;
    }

    CloseHandle(hPostDrv);

    return TRUE;


}

/*
*******************************************************************************
Name            : DecodeFileOutRGB16ToPPM
Description     : To change RGB16 to PPM, and write result file.
Parameter       :
Return Value    : void
*******************************************************************************
*/
void DecodeFileOutRGB16ToPPM(unsigned char *p_img, int wd, int hi,  char *filename)
{
    int   i, size;
    FILE *fp_write;
    unsigned short rgb565;
    unsigned char  rgb[3];

    fp_write = fopen(filename, "wb");

    fprintf(fp_write, "P6\n");
    fprintf(fp_write, "# Samsung Electronics\n");
    fprintf(fp_write, "%d %d\n255\n", wd, hi);

    size = wd * hi;
    for (i=0; i<size; i++) {
        rgb565 = (*p_img) | (*(++p_img));
        rgb[0] = *p_img++;
        rgb[1] = *p_img;
        rgb565 = (rgb[0]<<8) | rgb[1];

        rgb[0] = R_RGB565(rgb565);
        rgb[1] = G_RGB565(rgb565);
        rgb[2] = B_RGB565(rgb565);

        fwrite(rgb, 1, 3, fp_write);
    }

    fclose(fp_write);
}
