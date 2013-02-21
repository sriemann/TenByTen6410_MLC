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
//  CMM_Test TUX DLL
//
//  Module: CodecMemTest.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"

// Platform Dependent
#include "s3c6410_display_con.h"
#include "S3c6410_post_proc.h"
#include "SVE_API.h"
#include "CMMAPI.h"


#define DEBUG               1
#define MAX_BUFFER_NUM      3
#define IMG_WIDTH           320
#define IMG_HEIGHT          240
#define LCD_WIDTH           800
#define LCD_HEIGHT          480
#define PADDING_SIZE        0
#define BUFF_SIZE           (((IMG_WIDTH + 2*PADDING_SIZE)*(IMG_HEIGHT + 2*PADDING_SIZE)*3)>>1)
#define NUM_OF_ITERATION    3

/////////////////////////////////////////////////////////////////
// Variables                                                   //
/////////////////////////////////////////////////////////////////

HANDLE    displayThread;
UINT8     *buff[3];
HANDLE    hVideoDrv = INVALID_HANDLE_VALUE;
BOOL      bFirstRender;


/////////////////////////////////////////////////////////////////
// function prototypes                                         //
/////////////////////////////////////////////////////////////////

static int DisplayInit();
static int DisplayEnd();
static int StartDisplay(UINT32 buffNo, void *CodecMemHandle);

/////////////////////////////////////////////////////////////////
// functions                                                   //
/////////////////////////////////////////////////////////////////

TESTPROCAPI CMMTest_CodecMemTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    int                  ret;
    FILE                 *fp;
    DWORD                dwRead;
    DWORD                displayTime = 0;
    DWORD                totalTime = 0;
    UINT32               count = 0, i, loop = 0;
    CMM_ALLOC_PRAM_T     inParam;
    void                 *CodecMemHandle;
    DWORD                dwBytes;

	// The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    if (!DisplayInit()) 
        return TPR_FAIL;

    do 
    {
        g_pKato->Log(LOG_COMMENT, TEXT("CodecMemTest start(%d)\n"), loop);
        g_pKato->Log(LOG_COMMENT, TEXT("--------------------------------\n"));
        CodecMemHandle = CreateFile(CODEC_MEM_DRIVER_NAME,
                                    GENERIC_READ|GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,NULL);

        if (CodecMemHandle == INVALID_HANDLE_VALUE)
        {
            g_pKato->Log(LOG_FAIL, TEXT("CreateFile(CODEC_MEM_DRIVER_NAME) failed\r\n"));
            return TPR_FAIL;
        }

        for (i=0; i < MAX_BUFFER_NUM; i++)
        {
            inParam.size = (unsigned int)BUFF_SIZE;
            inParam.cacheFlag = 1;
            ret = DeviceIoControl(CodecMemHandle, IOCTL_CODEC_MEM_ALLOC, (PBYTE)&inParam, 
                                  sizeof(CMM_ALLOC_PRAM_T), &buff[i], sizeof(buff[i]), 
                                  NULL, NULL);
            if(ret == NULL)
            {
                g_pKato->Log(LOG_FAIL, TEXT("IOCTL_CODEC_MEM_GET failed\r\n"));
                return TPR_FAIL;
            }
        }

        fp = fopen("\\Storage Card\\cmmtest.yuv", "rb");
        if (fp == NULL)
        {
            g_pKato->Log(LOG_FAIL, TEXT("File open error: cmmtest.yuv \r\n"));
            return TPR_FAIL;
        }

        i = 0;
        bFirstRender = TRUE;
        
        do 
        {
            dwRead = fread(buff[i], 1, BUFF_SIZE, fp);

            // Instead of Sleep, your decoding function should be here...........
            //
                Sleep(20);
            //
            if(!(StartDisplay(i, CodecMemHandle)))    
                return TPR_FAIL;

            i = (++i) % MAX_BUFFER_NUM;
        } while(dwRead == BUFF_SIZE);

        fclose(fp);

        // Wait for Post Processing Finished
        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_WAIT_PROCESSING_DONE, NULL, 0, NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_WAIT_PROCESSING_DONE Failed 1111\n"));
            return TPR_FAIL;
        }

        CloseHandle(CodecMemHandle);
        Sleep(1000);
    } while(++loop < NUM_OF_ITERATION);

    DisplayEnd();
    return TPR_PASS;
}


static BOOL DisplayInit()
{
    DWORD   dwBytes;


    hVideoDrv = CreateFile(L"VDE0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
                            NULL, OPEN_EXISTING, 0, 0);
    if (hVideoDrv == INVALID_HANDLE_VALUE)
    {
        g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] VDE0 Open Device Failed\n"));
        return FALSE;
    }

    // Request FIMD Win0 H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_REQUEST_FIMD_WIN0 Failed\n"));
        return FALSE;
    }

    // Request Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_REQUEST_POST Failed\n"));
        return FALSE;
    }

    return TRUE;
}

static BOOL DisplayEnd()
{
    DWORD   dwBytes;
    DWORD   dwWinNum;

    dwWinNum = DISP_WIN0;
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_DISABLE, &dwWinNum, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_DISABLE Failed\n"));
        return FALSE;
    }
    // Release FIMD Win0 H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_RELEASE_FIMD_WIN0 Failed\n"));
        return FALSE;
    }

    // Release Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_RSC_RELEASE_POST Failed\n"));
        return FALSE;
    }

    CloseHandle(hVideoDrv);
    g_pKato->Log(LOG_COMMENT, TEXT("Display End\n"));

    return TRUE;
}


static int StartDisplay(UINT32 buffNo, void *CodecMemHandle)
{
    int                         ret;
    SVEARG_FIMD_WIN_MODE        tParamMode;
    SVEARG_FIMD_WIN_FRAMEBUFFER tParamFB;
    SVEARG_FIMD_WIN_COLORKEY    tParamCKey;
    SVEARG_FIMD_WIN_ALPHA       tParamAlpha;
    SVEARG_POST_PARAMETER       tParamPost;
    SVEARG_POST_BUFFER          tParamBuffer;
    DWORD                       dwBytes;
    UINT                        phy_addr;
    DWORD                       dwWinNum;


    ret = DeviceIoControl(CodecMemHandle, IOCTL_CODEC_CACHE_CLEAN_INVALIDATE, (PBYTE)buff[buffNo], 
                          sizeof(buff[buffNo]), NULL, 0, NULL, NULL);
    if(ret == 0) 
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_CODEC_CACHE_FLUSH failed(buffno : %ld)\r\n"), buffNo);
        return 0;
    }

    /*
    ret = DeviceIoControl(CodecMemHandle, IOCTL_CODEC_CACHE_CLEAN, (PBYTE)buff[buffNo], 
                        sizeof(buff[buffNo]), NULL, 0, 
                        NULL, NULL);
    if(ret == 0) {
        RETAILMSG(1, (TEXT("IOCTL_CODEC_CACHE_FLUSH failed(buffno : %ld)\r\n", buffNo)));
        return 0;
    }

    ret = DeviceIoControl(CodecMemHandle, IOCTL_CODEC_CACHE_INVALIDATE, (PBYTE)buff[buffNo], 
                        sizeof(buff[buffNo]), NULL, 0, 
                        NULL, NULL);
    if(ret == 0) {
        RETAILMSG(1, (TEXT("IOCTL_CODEC_CACHE_FLUSH failed(buffno : %ld)\r\n", buffNo)));
        return 0;
    }
    */

    ret = DeviceIoControl(CodecMemHandle, IOCTL_CODEC_GET_PHY_ADDR, (PBYTE)buff[buffNo], 
                        sizeof(buff[buffNo]), (PBYTE)&phy_addr, sizeof(phy_addr), 
                        NULL, NULL);
    if(ret == 0) 
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_CODEC_GET_PHY_ADDR failed\r\n"));
        return 0;
    }


    if (bFirstRender)
    {
        bFirstRender = FALSE;

        tParamMode.dwWinMode = DISP_WIN0_DMA;
        tParamMode.dwBPP = DISP_16BPP_565;
        tParamMode.dwWidth = LCD_WIDTH;
        tParamMode.dwHeight = LCD_HEIGHT;
        tParamMode.dwOffsetX = (800-tParamMode.dwWidth)/2;
        tParamMode.dwOffsetY = (480-tParamMode.dwHeight)/2;

        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParamMode, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n"));
            return 0;
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

        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY, &tParamCKey, 
                              sizeof(SVEARG_FIMD_WIN_COLORKEY), NULL, 0, &dwBytes, NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n"));
            return 0;
        }

        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, 
                              sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n"));
            return 0;
        }

        // Set Window0 Framebuffer
        tParamFB.dwWinNum = DISP_WIN0;
        tParamFB.dwFrameBuffer = 0x57400000;
        tParamFB.bWaitForVSync = FALSE;
        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER, &tParamFB, 
                              sizeof(SVEARG_FIMD_WIN_FRAMEBUFFER), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER Failed\n"));
            return 0;
        }

        // Window0 Enable
        dwWinNum = DISP_WIN0;
        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_ENABLE, &dwWinNum, 
                              sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_ENABLE Failed\n"));
            return 0;
        }

        tParamPost.dwOpMode = POST_PER_FRAME_MODE;
        tParamPost.dwScanMode = POST_PROGRESSIVE;
        tParamPost.dwSrcType = POST_SRC_YUV420;
        tParamPost.dwSrcBaseWidth = IMG_WIDTH + 2*PADDING_SIZE;
        tParamPost.dwSrcBaseHeight = IMG_HEIGHT + 2*PADDING_SIZE;
        tParamPost.dwSrcOffsetX = PADDING_SIZE;
        tParamPost.dwSrcOffsetY = PADDING_SIZE;
        tParamPost.dwSrcWidth = tParamPost.dwSrcBaseWidth -2* tParamPost.dwSrcOffsetX;
        tParamPost.dwSrcHeight = tParamPost.dwSrcBaseHeight -2*tParamPost.dwSrcOffsetY ;
        tParamPost.dwDstType = POST_DST_RGB16;
        tParamPost.dwDstBaseWidth = LCD_WIDTH;
        tParamPost.dwDstBaseHeight = LCD_HEIGHT;
        tParamPost.dwDstWidth = LCD_WIDTH;
        tParamPost.dwDstHeight = LCD_HEIGHT;
        tParamPost.dwDstOffsetX = 0;
        tParamPost.dwDstOffsetY = 0;

        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_PARAM, &tParamPost, 
                              sizeof(SVEARG_POST_PARAMETER), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n"));
            return 0;
        }

        // Source Address
        tParamBuffer.dwBufferRGBY = phy_addr;
        tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight;
        tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight/4;
        tParamBuffer.bWaitForVSync = FALSE;

        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, 
                              sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
            return 0;
        }

        // Destination Address
        tParamBuffer.dwBufferRGBY = 0x57400000;
        tParamBuffer.dwBufferCb = 0;
        tParamBuffer.dwBufferCr = 0;
        tParamBuffer.bWaitForVSync = FALSE;

        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_DESTINATION_BUFFER, &tParamBuffer, 
                              sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_DESTINATION_BUFFER Failed\n"));
            return 0;
        }

        // Post Processing Start
        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_START, NULL, 0, 
                              NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_START Failed\n"));
            return 0;
        }
    }
    else
    {
        // Wait for Post Processing Finished
        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_WAIT_PROCESSING_DONE, NULL, 0, 
                              NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_WAIT_PROCESSING_DONE Failed 2222\n"));
            return 0;
        }

        // Source Address
        tParamBuffer.dwBufferRGBY = phy_addr;
        tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+(IMG_WIDTH + 2*PADDING_SIZE)*(IMG_HEIGHT + 2*PADDING_SIZE);
        tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+(IMG_WIDTH + 2*PADDING_SIZE)*(IMG_HEIGHT + 2*PADDING_SIZE)/4;
        tParamBuffer.bWaitForVSync = FALSE;

        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, 
                              sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
            return 0;
        }

        // Post Processing Start
        if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_START, NULL, 0, 
                              NULL, 0, &dwBytes, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_START Failed\n"));
            return 0;
        }
    }

    return 1;
}

