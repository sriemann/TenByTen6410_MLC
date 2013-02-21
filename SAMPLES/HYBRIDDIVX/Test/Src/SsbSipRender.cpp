/*********************************************************************************
*                                                                               *
* Copyright (c) 2008 Samsung System LSI                                            *
* All rights reserved.                                                          *
*                                                                               *
* This software is test sample code for Hybrid Divx Decoder                        *
*                                                                                *
* Author : Jiyoung Shin                                                            *
* Last Changed : 2008.06.10                                                     *
*********************************************************************************/

#include <windows.h>
// Platform Dependent
#include <SVEDriverAPI.h>
#include "SsbSipRender.h"
#include "SsbSipMpeg4Decode.h"

HANDLE            hEvent = INVALID_HANDLE_VALUE;
HANDLE            displayThread = INVALID_HANDLE_VALUE;
HANDLE             hVideoDrv = INVALID_HANDLE_VALUE;
static UINT        DisplayBuffPhyAddr;
static unsigned int    DisplayWidth, DisplayHeight;
static unsigned int    DisplayBufWidth, DisplayBufHeight;
static unsigned int    DisplayOffsetX, DisplayOffsetY;

static int SsbSipDisplayInit()
{
    DWORD             dwBytes;


    hVideoDrv = CreateFile( L"VDE0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hVideoDrv == INVALID_HANDLE_VALUE)
    {
        RETAILMSG(1,(L"[VDE:ERR] VDE0 Open Device Failed\n"));
        return 0;
    }

    // Request FIMD Win0 H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_REQUEST_FIMD_WIN0 Failed\n"));
        return 0;
    }

    // Request Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_REQUEST_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_REQUEST_POST Failed\n"));
        return 0;
    }

    return    1;

}

int SsbSipDisplayDeInit()
{
    DWORD             dwBytes;
    DWORD            dwWinNum;

#ifdef MEM_TO_MEM
    dwWinNum = DISP_WIN0;
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_DISABLE, &dwWinNum, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_DISABLE Failed\n"));
        return 0;
    }
#else //LOCAL_PATH

    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_STOP, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_STOP Failed\n"));
        return 0;
    }


#endif

    // Release FIMD Win0 H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_RELEASE_FIMD_WIN0 Failed\n"));
        return 0;
    }

    // Release Post Processor H/W Resource to Video Engine Driver for Local Path
    if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_RSC_RELEASE_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_RSC_RELEASE_POST Failed\n"));
        return 0;
    }

    CloseHandle(hVideoDrv);
    return 0;
}


static DWORD WINAPI SsbSipDisplayThread()
{
    int                            ret;
    BOOL                        bFirstRender;
    SVEARG_FIMD_WIN_MODE        tParamMode;
    SVEARG_FIMD_WIN_FRAMEBUFFER tParamFB;
    SVEARG_FIMD_WIN_COLORKEY    tParamCKey;
    SVEARG_FIMD_WIN_ALPHA        tParamAlpha;
    SVEARG_POST_PARAMETER        tParamPost;
    SVEARG_POST_BUFFER            tParamBuffer;
    DWORD                         dwBytes;
    DWORD                        dwWinNum;


    printf("StartDisplay\n");
    
    if(!SsbSipDisplayInit()) ExitThread(0);

    bFirstRender = TRUE;

    while(TRUE){

        WaitForSingleObject(hEvent, INFINITE);

#ifdef MEM_TO_MEM
        if (bFirstRender)
        {
                bFirstRender = FALSE;

                tParamMode.dwWinMode = DISP_WIN0_DMA;
                tParamMode.dwBPP = DISP_16BPP_565;
                tParamMode.dwWidth = DISPLAY_X;
                tParamMode.dwHeight = DISPLAY_Y;
                tParamMode.dwOffsetX = (LCD_X - tParamMode.dwWidth)/2;
                tParamMode.dwOffsetY = (LCD_Y - tParamMode.dwHeight)/2;

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParamMode, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n"));
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

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY, &tParamCKey, sizeof(SVEARG_FIMD_WIN_COLORKEY), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n"));
                    return 0;
                }

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n"));
                    return 0;
                }

                // Set Window0 Framebuffer
                tParamFB.dwWinNum = DISP_WIN0;
                tParamFB.dwFrameBuffer = 0x57400000;
                tParamFB.bWaitForVSync = FALSE;
                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER, &tParamFB, sizeof(SVEARG_FIMD_WIN_FRAMEBUFFER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER Failed\n"));
                    return 0;
                }

                // Window0 Enable
                dwWinNum = DISP_WIN0;
                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_ENABLE, &dwWinNum, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_ENABLE Failed\n"));
                    return 0;
                }

                tParamPost.dwOpMode = POST_PER_FRAME_MODE;
                tParamPost.dwScanMode = POST_PROGRESSIVE;
                tParamPost.dwSrcType = POST_SRC_YUV420;
                 tParamPost.dwSrcBaseWidth = DisplayBufWidth;
                tParamPost.dwSrcBaseHeight = DisplayBufHeight;
                tParamPost.dwSrcOffsetX = DisplayOffsetX;
                tParamPost.dwSrcOffsetY = DisplayOffsetY;
                tParamPost.dwSrcWidth = DisplayWidth;
                tParamPost.dwSrcHeight = DisplayHeight ;
                tParamPost.dwDstType = POST_DST_RGB16;
                 tParamPost.dwDstBaseWidth = DISPLAY_X;
                tParamPost.dwDstBaseHeight = DISPLAY_Y;
                tParamPost.dwDstWidth = DISPLAY_X;
                tParamPost.dwDstHeight = DISPLAY_Y;
                tParamPost.dwDstOffsetX = (tParamPost.dwDstBaseWidth - tParamPost.dwDstWidth)/2;
                tParamPost.dwDstOffsetY = (tParamPost.dwDstBaseHeight - tParamPost.dwDstHeight)/2;



                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_PARAM, &tParamPost, sizeof(SVEARG_POST_PARAMETER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n"));
                    return 0;
                }

                tParamBuffer.dwBufferRGBY = DisplayBuffPhyAddr;
                tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight;
                tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight/4;
                tParamBuffer.bWaitForVSync = FALSE;

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
                    return 0;
                }

                // Destination Address
                tParamBuffer.dwBufferRGBY = 0x57400000;
                tParamBuffer.dwBufferCb = 0;
                tParamBuffer.dwBufferCr = 0;
                tParamBuffer.bWaitForVSync = FALSE;

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_DESTINATION_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_DESTINATION_BUFFER Failed\n"));
                    return 0;
                }

                // Post Processing Start
                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_START Failed\n"));
                    return 0;
                }
            }
            else
            {
                // Wait for Post Processing Finished
                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_WAIT_PROCESSING_DONE, NULL, 0, NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_WAIT_PROCESSING_DONE Failed\n"));
                    return 0;
                }

                // Source Address
                tParamBuffer.dwBufferRGBY = DisplayBuffPhyAddr;
                tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight;
                tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight/4;
                tParamBuffer.bWaitForVSync = FALSE;

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
                    return 0;
                }

                // Post Processing Start
                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_START Failed\n"));
                    return 0;
                }
            }
#else // LOCAL_PATH

            if (bFirstRender)
            {
                bFirstRender = FALSE;

                tParamMode.dwWinMode = DISP_WIN0_POST_RGB;
                tParamMode.dwBPP = DISP_24BPP_888;
                tParamMode.dwWidth = DISPLAY_X;
                tParamMode.dwHeight = DISPLAY_Y;
                tParamMode.dwOffsetX = (LCD_X - tParamMode.dwWidth)/2;
                tParamMode.dwOffsetY = (LCD_Y - tParamMode.dwHeight)/2;

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParamMode, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n"));
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

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY, &tParamCKey, sizeof(SVEARG_FIMD_WIN_COLORKEY), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n"));
                    return 0;
                }

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n"));
                    return 0;
                }

                tParamPost.dwOpMode = POST_FREE_RUN_MODE;
                tParamPost.dwScanMode = POST_PROGRESSIVE;
                tParamPost.dwSrcType = POST_SRC_YUV420;
                 tParamPost.dwSrcBaseWidth = DisplayBufWidth;
                tParamPost.dwSrcBaseHeight = DisplayBufHeight;
                tParamPost.dwSrcOffsetX = DisplayOffsetX;
                tParamPost.dwSrcOffsetY = DisplayOffsetY;
                tParamPost.dwSrcWidth = DisplayWidth;
                tParamPost.dwSrcHeight = DisplayHeight ;
                tParamPost.dwDstType = POST_DST_FIFO_RGB888;
                 tParamPost.dwDstBaseWidth = DISPLAY_X;
                tParamPost.dwDstBaseHeight = DISPLAY_Y;
                tParamPost.dwDstWidth = DISPLAY_X;
                tParamPost.dwDstHeight = DISPLAY_Y;
                tParamPost.dwDstOffsetX = (tParamPost.dwDstBaseWidth - tParamPost.dwDstWidth)/2;
                tParamPost.dwDstOffsetY = (tParamPost.dwDstBaseHeight - tParamPost.dwDstHeight)/2;



                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_PARAM, &tParamPost, sizeof(SVEARG_POST_PARAMETER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n"));
                    return 0;
                }

                tParamBuffer.dwBufferRGBY =DisplayBuffPhyAddr;
                tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight;
                tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight/4;
                tParamBuffer.bWaitForVSync = FALSE;

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n"));
                    return 0;
                }

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER Failed\n"));
                    return 0;
                }

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_LOCALPATH_SET_WIN0_START Failed\n"));
                    return 0;
                }
            }
            else
            {
                tParamBuffer.dwBufferRGBY = DisplayBuffPhyAddr;
                tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight;
                tParamBuffer.dwBufferCr = tParamBuffer.dwBufferCb+tParamPost.dwSrcBaseWidth*tParamPost.dwSrcBaseHeight/4;
                tParamBuffer.bWaitForVSync = TRUE;

                if ( !DeviceIoControl(hVideoDrv, IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
                {
                    RETAILMSG(1,(L"[VDE:ERR] IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER Failed\n"));
                    return 0;
                }
            }



#endif

        ResetEvent(hEvent);
        
    }

    ExitThread(0);
}

void SsbSipDisplayEventSet()
{
        SetEvent(hEvent);
}

void SsbSipDisplayEventCreate()
{
        hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void SsbSipDisplayParamSet(UINT    phy_addr, unsigned int width, unsigned int height, unsigned int buf_width, unsigned int buf_height, unsigned int offset_x, unsigned int offset_y)
{
        DisplayBuffPhyAddr = phy_addr;
        DisplayWidth = (width/16)*16;
        DisplayHeight = (height/16)*16;
        DisplayBufWidth = buf_width;
        DisplayBufHeight = buf_height;
        DisplayOffsetX = offset_x;
        DisplayOffsetY = offset_y;

        //printf("DisplayParamSet : buff(0x%08x) width(%d) height(%d)(buf_width:%d)(buf_height : %d)(offset_x : %d)(offset_y : %d)\n", 
        //    DisplayBuffPhyAddr, DisplayWidth, DisplayHeight, DisplayBufWidth, DisplayBufHeight, DisplayOffsetX, DisplayOffsetY);
}

int SsbSipDisplayThreadCreate()
{
    displayThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)SsbSipDisplayThread,
                                    NULL,
                                    0,
                                    NULL);
    if (!displayThread) {
        printf("Unable to create display thread");
        return FALSE;
    }

    return TRUE;

}

void SsbSipDisplayThreadClose()
{
    SsbSipDisplayDeInit();
    CloseHandle(displayThread);

}


