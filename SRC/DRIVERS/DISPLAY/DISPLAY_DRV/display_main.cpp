//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    display_main.cpp

Abstract:

    This module implement the main class that derived from DDGPE of display driver to support DirectDraw
    The class S3C6410Disp has many method and member variables, so implementation code is split to other module

Functions:

    Common Initalization, Deinitialization,
Notes:

--*/
#include <bsp.h>
#include "precomp.h"
#include "pmplatform.h"
#include <syspal.h>    // for 8Bpp we use the natural palette
#include <gxinfo.h>
//#define DISPPERF_DECLARE    //< This is defined in ddi_if.cpp in GPE component in CE6R2
#include "dispperf.h"

DDGPE * gGPE = (DDGPE *)NULL;

#define DBG_MSG_HEADER      _T("[DISPDRV]")

#ifdef DEBUG
// NOTE:  One file should use INSTANTIATE_GPE_ZONES.  This allows it to be pre-compiled
// initialZones should typically be 0x0003 (refer to "gpe.h")
INSTANTIATE_GPE_ZONES(DISPDRV_DEBUGZONES, __MODULE__, "", "")
#else


DBGPARAM dpCurSettings =                                \
{                                                       \
    TEXT(__MODULE__),                                      \
    {                                                   \
        TEXT("Errors"),                 /* 0  */        \
        TEXT("Warnings"),               /* 1  */        \
        TEXT("Performance"),            /* 2  */        \
        TEXT("Temporary tests"),        /* 3  */        \
        TEXT("Enter,Exit"),             /* 4  */        \
        TEXT("Initialize"),             /* 5  */        \
        TEXT("Blt Calls"),              /* 6  */        \
        TEXT("Blt Verbose"),            /* 7  */        \
        TEXT("Surface Create"),         /* 8  */        \
        TEXT("Flip"),                   /* 9  */        \
        TEXT("Line"),                   /* 10 */        \
        TEXT("Post"),                   /* 11 */        \
        TEXT("Rotator"),                /* 12 */        \
        TEXT("TV Scaler"),              /* 13 */        \
        TEXT("TV Encoder"),             /* 14 */        \
        TEXT("2D"),                     /* 15 */        \
    },                                                  \
    (DISPDRV_RETAILZONES)                               \
};
#endif

// This prototype avoids problems exporting from .lib
BOOL APIENTRY GPEEnableDriver(ULONG engineVersion, ULONG cj, DRVENABLEDATA * data, PENGCALLBACKS engineCallbacks);

BOOL AdvertisePowerInterface(HMODULE hInst);
BOOL ConvertStringToGuid (LPCTSTR pszGuid, GUID *pGuid);
CEDEVICE_POWER_STATE VideoToPmPowerState(VIDEO_POWER_STATE vps);
VIDEO_POWER_STATE PmToVideoPowerState(CEDEVICE_POWER_STATE pmDx);

BOOL
APIENTRY
DrvEnableDriver(
    ULONG           engineVersion,
    ULONG           cj,
    DRVENABLEDATA * data,
    PENGCALLBACKS   engineCallbacks
    )
{
    return GPEEnableDriver(engineVersion, cj, data, engineCallbacks);
}

GPE *
GetGPE()
{
    if (!gGPE)
    {
        gGPE = new S3C6410Disp();
    }

    return gGPE;
}


S3C6410Disp::S3C6410Disp()
{
    DWORD dwSplashFrameBufferSize;
	DWORD dwDisplayType[2] = {123,16};
    DWORD dwBytesRet = 0;
	if (   KernelIoControl (IOCTL_HAL_QUERY_DISPLAYSETTINGS, NULL, 0, dwDisplayType, sizeof(DWORD)*2, &dwBytesRet)  // get data from BSP_ARGS via KernelIOCtl
                        && (dwBytesRet == (sizeof(DWORD)*2)))
	{
		RETAILMSG(DISP_ZONE_ERROR,(TEXT("[DISPDRV1] display driver display: %s\r\n"),LDI_getDisplayName((HITEG_DISPLAY_TYPE)dwDisplayType[0])));
		LDI_set_LCD_module_type((HITEG_DISPLAY_TYPE)dwDisplayType[0]);
	}
	else
	{
		RETAILMSG(DISP_ZONE_ERROR,(TEXT("[DISPDRV1] Error getting Display type from args section via Kernel IOCTL!!!\r\n")));
	}
    RETAILMSG(DISP_ZONE_ENTER,(_T("[DISPDRV1] ++%s()\n\r"),_T(__FUNCTION__)));

    m_pDispConReg = NULL;
    m_pGPIOReg = NULL;
    m_pSYSReg = NULL;

    m_VideoMemoryPhysicalBase = NULL;
    m_VideoMemoryVirtualBase = NULL;
    m_VideoMemorySize = 0;
    m_pVideoMemoryHeap = NULL;

    m_hVideoDrv = NULL;

    m_CursorVisible = FALSE;
    m_CursorDisabled = TRUE;
    m_CursorForcedOff = FALSE;
    memset (&m_CursorRect, 0x0, sizeof(m_CursorRect));

    m_InDDraw = FALSE;

    InitalizeOverlayContext();

    m_bTVDMARunning = FALSE;

    m_TVDMACtxt.dwSourceFormat = 0;
    m_TVDMACtxt.uiSrcBaseWidth = 0;
    m_TVDMACtxt.uiSrcBaseHeight = 0;
    m_TVDMACtxt.uiSrcWidth = 0;
    m_TVDMACtxt.uiSrcHeight = 0;
    m_TVDMACtxt.uiSrcOffsetX = 0;
    m_TVDMACtxt.uiSrcOffsetY = 0;

    m_pLastSrcSurfUsingHW = NULL;
    m_pLastDstSurfUsingHW = NULL;

//---------------------
// Setup Mode Information
//---------------------

    // Initial Output Interface
	if(LDI_getDisplayMode()==DISP_VIDOUT_RGBIF)
	{
		m_eOutputInterface = OUTPUT_IF_RGB;
	}

	if(LDI_getDisplayMode()==DISP_VIDOUT_TVENCODER)
	{
		m_eOutputInterface = OUTPUT_IF_TV;
	}
	if(OUTPUT_IF_RGB)
    	m_eTVDMAMode = TV_DMA_DISABLE;

    // Initialize Screen Dimensions


	m_nScreenWidthSave = m_dwDeviceScreenWidth = LDI_GetDisplayWidth( LDI_getDisplayType() );
	m_nScreenHeightSave = m_dwDeviceScreenHeight = LDI_GetDisplayHeight( LDI_getDisplayType() );   
	
    m_iRotate = GetRotateModeFromReg();
    SetRotateParams();

    //----------------------------------------
    // Initialize ModeInfoEX, modeInfo Data Structure
    //----------------------------------------

    InitializeDisplayMode();

    // Initialize Power State
    m_VideoPowerState = VideoPowerOn;

    // Mapping Virtual Address (SFR, VideoMemory)
    if (AllocResource() == FALSE)
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : AllocResource() Fail\n\r"), _T(__FUNCTION__)));
        return;
    }

    // Clear Video Memory (Leave frame buffer for splash image)
    dwSplashFrameBufferSize = m_dwDeviceScreenWidth*m_dwDeviceScreenHeight*m_pMode->Bpp/8;
    //memset ((void *)(m_VideoMemoryVirtualBase+dwSplashFrameBufferSize), 0x0, m_VideoMemorySize-dwSplashFrameBufferSize);

    InitAcceleration();
    
    if(m_G2DControlArgs.HWOnOff)
    {
        m_oG2D = new FIMGSE2D;
        if(m_oG2D == NULL)
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : 2D Accelerator Initialization Fail\n\r"), _T(__FUNCTION__)));
        }

        // Initialize Interrupt (Event, Interrupt)
        if (m_oG2D)
        {
            BOOL bResult;

            bResult = m_oG2D->InitializeInterrupt();
            if(bResult==FALSE)
            {
                RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] InitializeInterrupt() 2D Object is failed.\n\r")));
            }
        }
        else
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] InitializeInterrupt() : 2D Object is not created.\n\r")));
        }
    }
    // Initialize Critical Section
    InitializeCriticalSection(&m_csDevice);
    InitializeCriticalSection(&m_cs2D);

    // Initialize Display Controller
    if (DevInitialize() == FALSE)
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : InitializeDevice() Fail\n\r"), _T(__FUNCTION__)));
        return;
    }

    //Allocate Primary Surface
    if (NULL == m_pPrimarySurface)
    {
        if (FAILED(AllocSurface((DDGPESurf **)&m_pPrimarySurface,
                            m_nScreenWidthSave, m_nScreenHeightSave,
                            m_pMode->format, m_pModeEx->ePixelFormat,
                            GPE_REQUIRE_VIDEO_MEMORY)))
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : m_pPrimarySurface AllocSurface() Fail\n\r"), _T(__FUNCTION__)));
         }
        else
        {
            m_pVisibleSurface = (S3C6410Surf*)m_pPrimarySurface;
        }
    }
    RETAILMSG(TRUE,(TEXT("m_nScreenWidth:%d, m_nScreenHeight:%d, m_nWS:%d, m_nHS:%d, m_iRotate:%d, PriRot:%d\r\n"),
        m_nScreenWidth, m_nScreenHeight, m_nScreenWidthSave, m_nScreenHeightSave, m_iRotate, m_pPrimarySurface->Rotate()));
    
    m_pPrimarySurface->SetRotation(m_nScreenWidth, m_nScreenHeight, m_iRotate);

    RETAILMSG(TRUE,(TEXT("Primary ScreenWidth:%d, ScreenHeight:%d, m_iRotate:%d\r\n"),
        m_pPrimarySurface->ScreenWidth(), m_pPrimarySurface->ScreenHeight(), m_pPrimarySurface->Rotate()));
    

    AdvertisePowerInterface(g_hmodDisplayDll);

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));
}

S3C6410Disp::~S3C6410Disp()
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    if (m_oG2D)
    {
        m_oG2D->DeinitInterrupt();
        delete m_oG2D;
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] DeinitInterrupt() : 2D Object is not created.\n\r")));
    }

    ReleaseResource();

    RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));
}

void
S3C6410Disp::InitAcceleration(void)
{
#if G2D_ACCELERATE
    m_G2DControlArgs.HWOnOff = 1;
    m_G2DControlArgs.UseBitBlt = 1;
    m_G2DControlArgs.UseLineDraw = 1;
#else
    m_G2DControlArgs.HWOnOff = 0;    
    m_G2DControlArgs.UseBitBlt = 0;
    m_G2DControlArgs.UseLineDraw = 0;
#endif
    // Determine AccelLevel, Currently not used strictly.
    m_G2DControlArgs.AccelLevel = 0x8;
    
    m_G2DControlArgs.UseSWAccel = USE_SECEMUL_LIBRARY;
    m_G2DControlArgs.CachedBlt = G2D_TRY_CBLT;
    m_G2DControlArgs.UseAlphaBlend = (G2D_BYPASS_HW_ALPHABLEND) ? (0) : (1);
    m_G2DControlArgs.UseStretchBlt = (G2D_BYPASS_HW_STRETCHBLT) ? (0) : (1);
    m_G2DControlArgs.UseFillRect = (G2D_BYPASS_HW_FILLRECT) ? (0) : (1);    
    m_G2DControlArgs.UsePACSurf = USE_PACSURF; 
    m_G2DControlArgs.SetBltLimitSize = G2D_BLT_OPTIMIZE;
    m_G2DControlArgs.AllocBoundSize = PAC_ALLOCATION_BOUNDARY;
    m_G2DControlArgs.BltLimitSize = G2D_COMPROMISE_LIMIT;
    m_G2DControlArgs.OverrideEmulFunc = G2D_OVERRIDE_EMULSEL;

    RETAILMSG(DISP_ZONE_INIT, (TEXT("LV:%d, HW:%d, BitBlt:%d, Line:%d, Alpha:%d, Fill:%d, SW:%d, Cached:%d, Stretch:%d, PAC:%d, Limit:%d, Alloc:%d, BltLimit:%d\r\n"),
    m_G2DControlArgs.AccelLevel,
    m_G2DControlArgs.HWOnOff,
    m_G2DControlArgs.UseBitBlt,
    m_G2DControlArgs.UseLineDraw,
    m_G2DControlArgs.UseAlphaBlend,
    m_G2DControlArgs.UseFillRect,
    m_G2DControlArgs.UseSWAccel,
    m_G2DControlArgs.CachedBlt,
    m_G2DControlArgs.UseStretchBlt,
    m_G2DControlArgs.UsePACSurf,
    m_G2DControlArgs.SetBltLimitSize,
    m_G2DControlArgs.AllocBoundSize,
    m_G2DControlArgs.BltLimitSize));
    
}

void 
S3C6410Disp::CheckAndWaitForHWIdle(GPESurf* pSurf)
{
    if (m_VideoPowerState != VideoPowerOff &&
        m_oG2D &&
        (m_pLastSrcSurfUsingHW == pSurf || m_pLastDstSurfUsingHW == pSurf))
    {
        // The surface passed in was being used in the last H/W blit.
        // Need to wait until that blit has completed
        m_oG2D->WaitForIdle();

        // Since the previous H/W blit completed, we can reset the Last* surface pointers
        m_pLastSrcSurfUsingHW = NULL;
        m_pLastDstSurfUsingHW = NULL;
    }
}

void
S3C6410Disp::WaitForNotBusy(void)
{
    if( m_VideoPowerState != VideoPowerOff && // to avoid hanging while bring up display H/W
        m_oG2D)
    {
        m_oG2D->WaitForIdle();

        // Since the previous H/W blit completed, we can reset the Last* surface pointers
        m_pLastSrcSurfUsingHW = NULL;
        m_pLastDstSurfUsingHW = NULL;        
    }
}

int
S3C6410Disp::IsBusy(void)
{
    if( m_VideoPowerState != VideoPowerOff && // to avoid hanging while bring up display H/W
        m_oG2D)
    {
        return m_oG2D->CheckFifo();
    }
    return    0;        //< 2D HW not yet start
}

bool
S3C6410Disp::SurfaceBusyBlitting(DDGPESurf *pSurf)
{
    bool retval = FALSE;
    
    if (m_VideoPowerState != VideoPowerOff &&
        m_oG2D &&
        (m_pLastSrcSurfUsingHW == pSurf || m_pLastDstSurfUsingHW == pSurf))
    {
        // The surface passed in was being used in the last H/W blit.
        // Check the status
        if (!m_oG2D->IsIdle())
        {
            retval = TRUE;
        }
        else
        {
            // Since the previous H/W blit completed, we can reset the Last* surface pointers
            m_pLastSrcSurfUsingHW = NULL;
            m_pLastDstSurfUsingHW = NULL;
        }
    }

    return retval;
}


void
S3C6410Disp::GetPhysicalVideoMemory(unsigned long *physicalMemoryBase, unsigned long *videoMemorySize)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] %s()\n\r"), _T(__FUNCTION__)));

    *physicalMemoryBase = m_VideoMemoryPhysicalBase;
    *videoMemorySize = m_VideoMemorySize;
}

void
S3C6410Disp::GetVirtualVideoMemory(unsigned long *virtualMemoryBase, unsigned long *videoMemorySize)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] %s()\n\r"), _T(__FUNCTION__)));

    *virtualMemoryBase = m_VideoMemoryVirtualBase;
    *videoMemorySize = m_VideoMemorySize;
}


int
S3C6410Disp::InDisplay(void)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("%s()\r\n"), _T(__FUNCTION__)));

    if (DevGetVerticalStatus() == DISP_V_ACTIVE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

int
S3C6410Disp::InVBlank(void)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("%s()\r\n"), _T(__FUNCTION__)));

    switch(DevGetVerticalStatus())
    {
        case DISP_V_VSYNC:
        case DISP_V_BACKPORCH:
            return TRUE;
            break;
        case DISP_V_ACTIVE:
        case DISP_V_FRONTPORCH:    
        default:
            return FALSE;
            break;
    }
}

SCODE
S3C6410Disp::SetPalette(const PALETTEENTRY *source, USHORT firstEntry, USHORT numEntries)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("%s()\r\n"), _T(__FUNCTION__)));

    if (firstEntry + numEntries > 256 || source == NULL)
    {
        return  E_INVALIDARG;
    }

    return S_OK;
}

int
S3C6410Disp::GetGameXInfo(ULONG iEsc, ULONG cjIn, PVOID pvIn, ULONG cjOut, PVOID pvOut)
{
    int     RetVal = 0;     // Default not supported
    GXDeviceInfo * pgxoi;
    void *pCheckBufOut = NULL;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    // GAPI only support P8, RGB444, RGB555, RGB565, and RGB888
    if ((cjOut >= sizeof(GXDeviceInfo)) && (pvOut != NULL)
        && (m_pMode->Bpp == 8 || m_pMode->Bpp == 16 || m_pMode->Bpp == 24 || m_pMode->Bpp == 32))
    {
        if(FAILED(CeOpenCallerBuffer(&pCheckBufOut, pvOut, cjOut, ARG_IO_PTR, FALSE)))
        {
            DEBUGMSG( DISP_ZONE_ERROR, (_T("Display GetGameXInfo: CeOpenCallerBuffer failed.\r\n")) );
            return ESC_FAILED;
        }
        if (((GXDeviceInfo *) pCheckBufOut)->idVersion == kidVersion100)
        {
            pgxoi = (GXDeviceInfo *) pCheckBufOut;
            pgxoi->idVersion = kidVersion100;
            pgxoi->pvFrameBuffer = (void *)m_pPrimarySurface->Buffer();
            pgxoi->cbStride = m_pPrimarySurface->Stride();
            pgxoi->cxWidth = m_pPrimarySurface->Width();
            pgxoi->cyHeight = m_pPrimarySurface->Height();

            if (m_pMode->Bpp == 8)
            {
                pgxoi->cBPP = 8;
                pgxoi->ffFormat = kfPalette;
            }
            else if (m_pMode->Bpp == 16)
            {
                pgxoi->cBPP = 16;
                pgxoi->ffFormat= kfDirect | kfDirect565;
            }
            else if (m_pMode->Bpp == 24)
            {
                pgxoi->cBPP = 24;
                pgxoi->ffFormat = kfDirect | kfDirect888;
            }
            else
            {
                pgxoi->cBPP = 32;
                pgxoi->ffFormat = kfDirect | kfDirect888;
            }

            if (m_iRotate == DMDO_90 || m_iRotate == DMDO_270 )
                pgxoi->ffFormat |= kfLandscape;  // Rotated

            pgxoi->vkButtonUpPortrait = VK_UP;
            pgxoi->vkButtonUpLandscape = VK_LEFT;
            pgxoi->vkButtonDownPortrait = VK_DOWN;
            pgxoi->vkButtonDownLandscape = VK_RIGHT;
            pgxoi->vkButtonLeftPortrait = VK_LEFT;
            pgxoi->vkButtonLeftLandscape = VK_DOWN;
            pgxoi->vkButtonRightPortrait = VK_RIGHT;
            pgxoi->vkButtonRightLandscape = VK_UP;
            pgxoi->vkButtonAPortrait = 0xC3;            // far right button
            pgxoi->vkButtonALandscape = 0xC5;            // record button on side
            pgxoi->vkButtonBPortrait = 0xC4;            // second from right button
            pgxoi->vkButtonBLandscape = 0xC1;
            pgxoi->vkButtonCPortrait = 0xC5;            // far left button
            pgxoi->vkButtonCLandscape = 0xC2;            // far left button
            pgxoi->vkButtonStartPortrait = 134;            // action button
            pgxoi->vkButtonStartLandscape = 134;
            pgxoi->ptButtonUp.x = 120;
            pgxoi->ptButtonUp.y = 330;
            pgxoi->ptButtonDown.x = 120;
            pgxoi->ptButtonDown.y = 390;
            pgxoi->ptButtonLeft.x = 90;
            pgxoi->ptButtonLeft.y = 360;
            pgxoi->ptButtonRight.x = 150;
            pgxoi->ptButtonRight.y = 360;
            pgxoi->ptButtonA.x = 180;
            pgxoi->ptButtonA.y = 330;
            pgxoi->ptButtonB.x = 210;
            pgxoi->ptButtonB.y = 345;
            pgxoi->ptButtonC.x = -50;
            pgxoi->ptButtonC.y = 0;
            pgxoi->ptButtonStart.x = 120;
            pgxoi->ptButtonStart.y = 360;
            pgxoi->pvReserved1 = (void *) 0;
            pgxoi->pvReserved2 = (void *) 0;
            RetVal = ESC_SUCCESS;
        }
        else
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] GetGameXInfo() : Invalid Parameter\n\r")));
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = ESC_FAILED;
        }
        if(FAILED(CeCloseCallerBuffer(pCheckBufOut, pvOut, cjOut, ARG_IO_PTR)))
        {
            DEBUGMSG( DISP_ZONE_ERROR, (_T("Display GetGameXInfo: CeCloseCallerBuffer failed.\r\n")) );
            return ESC_FAILED;
        }
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] GetGameXInfo() : Invalid Parameter\n\r")));
        SetLastError (ERROR_INVALID_PARAMETER);
        RetVal = ESC_FAILED;
    }

    RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return(RetVal);
}


int
S3C6410Disp::GetRawFrameBuffer(ULONG iEsc, ULONG cjIn, PVOID pvIn, ULONG cjOut, PVOID pvOut)
{
    int RetVal = ESC_SUCCESS;     // Default not supported
    RawFrameBufferInfo *pRawFB;
    void *pCheckBufOut = NULL;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    if ((cjOut >= sizeof(RawFrameBufferInfo)) && (pvOut != NULL))
    {
        if(FAILED(CeOpenCallerBuffer(&pCheckBufOut, pvOut, cjOut, ARG_IO_PTR, FALSE)))
        {
            DEBUGMSG( DISP_ZONE_ERROR, (_T("Display GetRawFrameBuffer: CeOpenCallerBuffer failed.\r\n")) );
            return ESC_FAILED;
        }

        pRawFB = (RawFrameBufferInfo *)pCheckBufOut;

        pRawFB->wBPP = m_pMode->Bpp;

        if (pRawFB->wBPP == gpe32Bpp)
        {
            pRawFB->wFormat= RAW_FORMAT_OTHER;
            pRawFB->cxStride = m_pPrimarySurface->Stride();
            pRawFB->cyStride = sizeof(DWORD);
        }
        else
        {
            pRawFB->wFormat= RAW_FORMAT_565;
            pRawFB->cxStride = m_pPrimarySurface->Stride();
            pRawFB->cyStride = sizeof(WORD);
        }

        pRawFB->cxPixels = m_pPrimarySurface->Width();
        pRawFB->cyPixels = m_pPrimarySurface->Height();
        pRawFB->pFramePointer = (void *)m_pPrimarySurface->Buffer();

        RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV:INF] GetRawFrameBuffer() : Stride=%d, xPixel=%d, yPixel=%d, FramePointer=0x%08x\r\n"), 
                                pRawFB->cyStride, pRawFB->cxPixels, pRawFB->cyPixels, pRawFB->pFramePointer));

        RetVal = ESC_SUCCESS;

        if(FAILED(CeCloseCallerBuffer(pCheckBufOut, pvOut, cjOut, ARG_IO_PTR)))
        {
            DEBUGMSG( DISP_ZONE_ERROR, (_T("Display GetGameXInfo: CeCloseCallerBuffer failed.\r\n")) );
            return ESC_FAILED;
        }
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] GetRawFrameBuffer() : Invalid Parameter\n\r")));
        SetLastError (ERROR_INVALID_PARAMETER);
        RetVal = ESC_FAILED;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return(RetVal);
}


ULONG
S3C6410Disp::DrvEscape(SURFOBJ * pso, ULONG iEsc, ULONG cjIn, void *pvIn, ULONG cjOut, void *pvOut)
{
    ULONG Result = 0;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s(0x%08x)\n\r"), _T(__FUNCTION__), iEsc));

    if (iEsc == QUERYESCSUPPORT)
    {
        // tell the power manager about ourselves
        if (pvIn != NULL && cjIn == sizeof(DWORD))
        {
            __try
            {
                if ((*(DWORD*)pvIn == GETGXINFO)
                    || (*(DWORD*)pvIn == GETRAWFRAMEBUFFER)
                    || (*(DWORD*)pvIn == DRVESC_GETSCREENROTATION)
                    || (*(DWORD*)pvIn == DRVESC_SETSCREENROTATION)
                    || (*(DWORD*)pvIn == SETPOWERMANAGEMENT)
                    || (*(DWORD*)pvIn == GETPOWERMANAGEMENT)
                    || (*(DWORD*)pvIn == IOCTL_POWER_CAPABILITIES)
                    || (*(DWORD*)pvIn == IOCTL_POWER_QUERY)
                    || (*(DWORD*)pvIn == IOCTL_POWER_SET)
                    || (*(DWORD*)pvIn == IOCTL_POWER_GET)
                    || (*(DWORD*)pvIn == DRVESC_OUTPUT_RGB)
                    || (*(DWORD*)pvIn == DRVESC_OUTPUT_TV)
                    || (*(DWORD*)pvIn == DRVESC_OUTPUT_SWITCH)
                    || (*(DWORD*)pvIn == DRVESC_TV_DMA_DISABLE)
                    || (*(DWORD*)pvIn == DRVESC_TV_DMA_PRIMARY)
                    || (*(DWORD*)pvIn == DRVESC_TV_DMA_OVERLAY)
                    || (*(DWORD*)pvIn == DRVESC_G2D_ACCEL_SET))
                {
                    // The escape is supported.
                    return ESC_SUCCESS;
                }
                else
                {
                    // The escape isn't supported.
#if DO_DISPPERF
                    return DispPerfQueryEsc(*(DWORD*)pvIn);;
#else
                    return ESC_NOT_SUPPORTED;
#endif
                }
            }
            __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : QUERYESCSUPPORT Exception Occurs\n\r")));
                Result = ESC_FAILED;
            }
        }
    }
    else if (iEsc == DRVESC_GETSCREENROTATION)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : DRVESC_GETSCREENROTATION\n\r")));

        // This API is called only by GDI. Access to applications is restricted by GDI
        *(int *)pvOut = ((DMDO_0 | DMDO_90 | DMDO_180 | DMDO_270) << 8) | ((BYTE)m_iRotate);
        
        
        return DISP_CHANGE_SUCCESSFUL;
    }
    else if (iEsc == DRVESC_SETSCREENROTATION)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : DRVESC_SETSCREENROTATION\n\r")));

        if ((cjIn == DMDO_0)   ||
            (cjIn == DMDO_90)  ||
            (cjIn == DMDO_180) ||
            (cjIn == DMDO_270) )
        {
            return DynRotate(cjIn);
        }

        return DISP_CHANGE_BADMODE;
    }
    else if (iEsc == GETGXINFO)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : GETGXINFO\n\r")));

        return GetGameXInfo(iEsc, cjIn, pvIn, cjOut, pvOut);
    }
    else if (iEsc == GETRAWFRAMEBUFFER)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : GETRAWFRAMEBUFFER\n\r")));

        return GetRawFrameBuffer(iEsc, cjIn, pvIn, cjOut, pvOut);
    }
    else if (iEsc == SETPOWERMANAGEMENT)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : SETPOWERMANAGEMENT\n\r")));

        __try
        {
            if ((cjIn >= sizeof (VIDEO_POWER_MANAGEMENT)) && (pvIn != NULL))
            {
                PVIDEO_POWER_MANAGEMENT pvpm = (PVIDEO_POWER_MANAGEMENT)pvIn;

                if (pvpm->Length >= sizeof (VIDEO_POWER_MANAGEMENT))
                {
                    SetDisplayPowerState((VIDEO_POWER_STATE)(pvpm->PowerState));
                }
            }
        }
        __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
           EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : SETPOWERMANAGEMENT Exception Occurs\n\r")));
            Result = ESC_FAILED;
        }

        if (Result != ESC_SUCCESS)
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : SETPOWERMANAGEMENT Fail\n\r")));

            // Shouldn't get here if everything was ok.
            SetLastError(ERROR_INVALID_PARAMETER);
            Result = ESC_FAILED;
        }

        return Result;
    }
    else if (iEsc == GETPOWERMANAGEMENT)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : GETPOWERMANAGEMENT\n\r")));

        __try
        {
            if ((cjOut >= sizeof (VIDEO_POWER_MANAGEMENT)) && (pvOut != NULL))
            {
                PVIDEO_POWER_MANAGEMENT pvpm = (PVIDEO_POWER_MANAGEMENT)pvOut;

                pvpm->Length = sizeof (VIDEO_POWER_MANAGEMENT);
                pvpm->DPMSVersion = 0;

                pvpm->PowerState = m_VideoPowerState;

                Result = ESC_SUCCESS;
            }
        }
        __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
           EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : SETPOWERMANAGEMENT Exception Occurs\n\r")));
            Result = ESC_FAILED;
        }

        if (Result != ESC_SUCCESS)
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : GETPOWERMANAGEMENT Fail\n\r")));

            // Shouldn't get here if everything was ok.
            SetLastError(ERROR_INVALID_PARAMETER);
            Result = ESC_FAILED;
        }

        return Result;
    }
    else if (iEsc == IOCTL_POWER_CAPABILITIES)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : IOCTL_POWER_CAPABILITIES\n\r")));

        // tell the power manager about ourselves
        if (pvOut != NULL && cjOut == sizeof(POWER_CAPABILITIES))
        {
            __try
            {
                PPOWER_CAPABILITIES ppc = (PPOWER_CAPABILITIES) pvOut;
                memset(ppc, 0, sizeof(*ppc));
                ppc->DeviceDx = DX_MASK(D0) | DX_MASK(D4);
                Result = ESC_SUCCESS;
            }
            __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : IOCTL_POWER_CAPABILITIES Exception Occurs\n\r")));
                Result = ESC_FAILED;
            }
        }

        return Result;
    }
    else if(iEsc == IOCTL_POWER_QUERY)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : IOCTL_POWER_QUERY\n\r")));

        if(pvOut != NULL && cjOut == sizeof(CEDEVICE_POWER_STATE))
        {
            // return a good status on any valid query, since we are always ready to
            // change power states.
            __try
            {
                CEDEVICE_POWER_STATE NewDx = *(PCEDEVICE_POWER_STATE) pvOut;
                if(VALID_DX(NewDx))
                {
                    // this is a valid Dx state so return a good status
                    Result = ESC_SUCCESS;
                }
                else
                {
                    RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : IOCTL_POWER_QUERY Fail\n\r")));
                    Result = ESC_FAILED;
                }
            }
            __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : IOCTL_POWER_QUERY Exception Occurs\n\r")));
                Result = ESC_FAILED;
            }
        }

        return Result;
    }
    else if(iEsc == IOCTL_POWER_SET)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : IOCTL_POWER_SET\n\r")));

        if(pvOut != NULL && cjOut == sizeof(CEDEVICE_POWER_STATE))
        {
            __try
            {
                CEDEVICE_POWER_STATE NewDx = *(PCEDEVICE_POWER_STATE) pvOut;
                if(VALID_DX(NewDx))
                {
                    RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : IOCTL_POWER_SET(D%d)\n\r"), NewDx));
                    SetDisplayPowerState(PmToVideoPowerState(NewDx));
                    Result = ESC_SUCCESS;
                }
                else
                {
                    RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : IOCTL_POWER_SET Fail\n\r")));
                    Result = ESC_FAILED;
                }
            }
            __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : IOCTL_POWER_SET Exception Occurs\n\r")));
                Result = ESC_FAILED;
            }
        }

        return Result;
    }
    else if(iEsc == IOCTL_POWER_GET)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : IOCTL_POWER_GET\n\r")));

        if(pvOut != NULL && cjOut == sizeof(CEDEVICE_POWER_STATE))
        {
            __try
            {
                *(PCEDEVICE_POWER_STATE) pvOut = VideoToPmPowerState(GetDisplayPowerState());

                Result = ESC_SUCCESS;
            }
            __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : IOCTL_POWER_GET Exception Occurs\n\r")));
                Result = ESC_FAILED;
            }
        }

        return Result;
    }
    else if(iEsc == DRVESC_OUTPUT_RGB)
    {
        if (DevChangeOutputInterface(OUTPUT_IF_RGB))
        {
            return ESC_SUCCESS;
        }
        else
        {
            return ESC_FAILED;
        }
    }
    else if(iEsc == DRVESC_OUTPUT_TV)
    {
        if (DevChangeOutputInterface(OUTPUT_IF_TV))
        {
            return ESC_SUCCESS;
        }
        else
        {
            return ESC_FAILED;
        }
    }
    else if (iEsc == DRVESC_OUTPUT_SWITCH)
    {
        if (m_eOutputInterface == OUTPUT_IF_RGB)
        {
            if (DevChangeOutputInterface(OUTPUT_IF_TV))
            {
                return ESC_SUCCESS;
            }
            else
            {
                return ESC_FAILED;
            }
        }
        else
        {
            if (DevChangeOutputInterface(OUTPUT_IF_RGB))
            {
                return ESC_SUCCESS;
            }
            else
            {
                return ESC_FAILED;
            }
        }
    }
    else if(iEsc == DRVESC_TV_DMA_DISABLE)
    {
        // TODO: We need to implement wrapper OutputDisableTVDMA()...
        EnterCriticalSection(&m_csDevice);
        DevOutputDisableTVDMA();
        m_eTVDMAMode = TV_DMA_DISABLE;
        TVOutReleaseResource();
        LeaveCriticalSection(&m_csDevice);

        return ESC_SUCCESS;
    }
    else if(iEsc == DRVESC_TV_DMA_PRIMARY)
    {
        if (TVOutAllocResource())
        {
            EnterCriticalSection(&m_csDevice);
            DevSetTVDMAMode(TV_DMA_PRIMARY);
            LeaveCriticalSection(&m_csDevice);

            return ESC_SUCCESS;
        }
        else
        {
            return ESC_FAILED;
        }
    }
    else if(iEsc == DRVESC_TV_DMA_OVERLAY)
    {
        if (TVOutAllocResource())
        {
            EnterCriticalSection(&m_csDevice);
            DevSetTVDMAMode(TV_DMA_OVERLAY);
            LeaveCriticalSection(&m_csDevice);

            return ESC_SUCCESS;
        }
        else
        {
            return ESC_FAILED;
        }
    }
    else if(iEsc == DRVESC_G2D_ACCEL_SET)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : DRVESC_G2D_ACCEL\n\r")));
        /// Control Accelerator On/Off
        /// Adjust Acceleration Level (0~0xF0) : Predefined configuration, (0xF1~0xFE) : Reserved, (0xFF) : Force Setting
        ///     L0   0x00      No Accelration and Optimization
        ///     L1   0x01      Only SW Acceleratoin
        ///     L2   0x02      HW BitBlt
        ///     L3   0x04      HW BitBlt + HW Line
        ///     L4   0x08      HW Line + HW BitBlt + HW FillRect

        /// Return Value : Acceleration Level
        ///        Succeed : Accerleration Level ( 1 ~ 3 )
        ///        Fail : 0 (default no accleration)
        if(pvIn != NULL && cjIn == sizeof(G2D_ACCEL_CONTROL_ARGS) && pvOut != NULL && cjOut == sizeof(DWORD))
        {
            __try
            {
                G2D_ACCEL_CONTROL_ARGS NewSetting = *(G2D_ACCEL_CONTROL_ARGS *) pvIn;
                RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] DrvEscape() : IOCTL_POWER_SET(D%d)\n\r"), NewSetting));
                
                // Detailed Setting.
                if(NewSetting.AccelLevel == 0xFF)
                {
                    m_G2DControlArgs = NewSetting;
                }
                else
                {
                    // Decode Accleration LEVEL
                    switch(NewSetting.AccelLevel & 0xF)
                    {
                    case 0x0:
                        m_G2DControlArgs.AccelLevel = NewSetting.AccelLevel;
                        m_G2DControlArgs.UseSWAccel = 0;
                        m_G2DControlArgs.HWOnOff = 0;
                        m_G2DControlArgs.UseBitBlt = 0;
                        m_G2DControlArgs.UseLineDraw = 0;
                        m_G2DControlArgs.UseAlphaBlend = 0;
                        m_G2DControlArgs.UseFillRect = 0;
                        break;
                    case 0x1:
                        m_G2DControlArgs.AccelLevel = NewSetting.AccelLevel;
                        m_G2DControlArgs.UseSWAccel = 1;
                        m_G2DControlArgs.HWOnOff = 0;
                        m_G2DControlArgs.UseBitBlt = 0;
                        m_G2DControlArgs.UseLineDraw = 0;
                        m_G2DControlArgs.UseAlphaBlend = 0;
                        m_G2DControlArgs.UseFillRect = 0;
                        break;
                    case 0x2:
                        m_G2DControlArgs.AccelLevel = NewSetting.AccelLevel;
                        m_G2DControlArgs.UseSWAccel = 1;
                        m_G2DControlArgs.HWOnOff = 1;
                        m_G2DControlArgs.UseBitBlt = 1;
                        m_G2DControlArgs.UseLineDraw = 0;
                        m_G2DControlArgs.UseAlphaBlend = 0;
                        m_G2DControlArgs.UseFillRect = 0;
                        break;
                    case 0x4:
                        m_G2DControlArgs.AccelLevel = NewSetting.AccelLevel;
                        m_G2DControlArgs.UseSWAccel = 1;
                        m_G2DControlArgs.HWOnOff = 1;
                        m_G2DControlArgs.UseBitBlt = 1;
                        m_G2DControlArgs.UseLineDraw = 1;
                        m_G2DControlArgs.UseAlphaBlend = 1;
                        m_G2DControlArgs.UseFillRect = 0;
                        break;
                    case 0x8:
                        m_G2DControlArgs.AccelLevel = NewSetting.AccelLevel;
                        m_G2DControlArgs.UseSWAccel = 1;
                        m_G2DControlArgs.HWOnOff = 1;
                        m_G2DControlArgs.UseBitBlt = 1;
                        m_G2DControlArgs.UseLineDraw = 1;
                        m_G2DControlArgs.UseAlphaBlend = 1;
                        m_G2DControlArgs.UseFillRect = 1;
                        break;
                    default:
                        //This is invalid level, so we ignore it.
                        Result = ESC_FAILED;
                        break;
                    }

                }
                Result = ESC_SUCCESS;
            }
            __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ? 
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DrvEscape() : DRVESC_G2D_ACCEL_SET Exception Occurs\n\r")));
                Result = ESC_FAILED;
            }
        }
        
        return ESC_FAILED;
    }
#if DO_DISPPERF
    else
    {
        return DispPerfDrvEscape(iEsc, cjIn, pvIn, cjOut,pvOut);
    }
#endif

    return 0;
}

int
S3C6410Disp::GetRotateModeFromReg()
{
    HKEY hKey;
    int iRet = DMDO_0;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\GDI\\ROTATION"), 0, 0, &hKey))
    {
        DWORD dwSize, dwAngle, dwType = REG_DWORD;

        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("ANGLE"), NULL, &dwType, (LPBYTE)&dwAngle, &dwSize))
        {
            switch (dwAngle)
            {
            case 0:
                iRet = DMDO_0;
                break;

            case 90:
                iRet = DMDO_90;
                break;

            case 180:
                iRet = DMDO_180;
                break;

            case 270:
                iRet = DMDO_270;
                break;

            default:
                iRet = DMDO_0;
                break;
            }
        }

        RegCloseKey(hKey);
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] GetRotateModeFromReg() : RegOpenKeyEx() Fail\n\r")));
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s() = %d\n\r"), _T(__FUNCTION__),iRet));

    return iRet;
}

void
S3C6410Disp::SetRotateParams()
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV:INF] %s() : Angle = %d\n\r"), _T(__FUNCTION__), m_iRotate));

    switch(m_iRotate)
    {
    case DMDO_90:
    case DMDO_270:
        m_nScreenHeight = m_nScreenWidthSave;
        m_nScreenWidth = m_nScreenHeightSave;
        break;

    case DMDO_0:
    case DMDO_180:
    default:
        m_nScreenWidth = m_nScreenWidthSave;
        m_nScreenHeight = m_nScreenHeightSave;
        break;
    }

    return;
}

LONG
S3C6410Disp::DynRotate(int angle)
{
    GPESurfRotate * pSurf = (GPESurfRotate *)m_pPrimarySurface;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s(%d)\n\r"), _T(__FUNCTION__), angle));

    // DirectDraw and rotation can't co-exist.
    if (m_InDDraw)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DynRotate() : Can NOT Rotate in DirectDraw Mode\n\r")));
        return DISP_CHANGE_BADMODE;
    }

    CursorOff();

    // ensure that the angle is not negative and does not exceed 360 degree rotation
    m_iRotate = abs(angle) % 360;

    SetRotateParams();

    m_pMode->width  = m_nScreenWidthSave;
    m_pMode->height = m_nScreenHeightSave;

    pSurf->SetRotation(m_nScreenWidth, m_nScreenHeight, angle);

    CursorOn();

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return DISP_CHANGE_SUCCESSFUL;
}


BOOL
S3C6410Disp::AllocResource(void)
{
    PHYSICAL_ADDRESS    ioPhysicalBase = { 0, 0};

    DWORD dwBytes;
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s\n\r"),_T(__FUNCTION__)));


    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DISPLAY;
    m_pDispConReg = (S3C6410_DISPLAY_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DISPLAY_REG), FALSE);
    if (m_pDispConReg == NULL)
    {
        goto CleanUp;
    }
    
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    m_pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (m_pGPIOReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
    m_pSYSReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (m_pSYSReg == NULL)
    {
        goto CleanUp;
    }

    // Open Video Engine Driver
    m_hVideoDrv = CreateFile( L"VDE0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (m_hVideoDrv == INVALID_HANDLE_VALUE)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] AllocResource() : VDE0 Open Device Failed\n\r")));
        return FALSE;
    }

    // Request FIMD H/W Resource to Video Engine Driver
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevInitialize() : IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE Failed\n\r")));
        return FALSE;
    }

    // Request FIMD Win1 H/W Resource to Video Engine Driver for Primary Window
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_WIN1, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevInitialize() : IOCTL_SVE_RSC_REQUEST_FIMD_WIN1 Failed\n\r")));
        return FALSE;
    }

    // Frame Buffer
    m_VideoMemoryPhysicalBase = IMAGE_FRAMEBUFFER_PA_START;
    m_VideoMemorySize = IMAGE_FRAMEBUFFER_SIZE;

    m_VideoMemoryVirtualBase = (DWORD)VirtualAlloc(NULL, m_VideoMemorySize, MEM_RESERVE, PAGE_NOACCESS);

    if (NULL == VirtualCopy((void *)m_VideoMemoryVirtualBase, (void *)(m_VideoMemoryPhysicalBase>>8), m_VideoMemorySize, PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] AllocResource() : m_VideoMemoryVirtualBase VirtualCopy() Failed : %x\n\r"),    GetLastError()));
        return FALSE;
    }

#if 1
    // Platform Independent
    if(CeSetMemoryAttributes((void *)m_VideoMemoryVirtualBase, (void *)(m_VideoMemoryPhysicalBase>>8), m_VideoMemorySize, PAGE_WRITECOMBINE) != TRUE)
    {
        RETAILMSG(DISP_ZONE_WARNING, (_T("[DISPDRV:ERR] AllocResource() : m_VideoMemoryVirtualBase CeSetMemoryAttributes() Failed : %x\n\r"), GetLastError()));
    } 
#else
    // Platform Dependent, change TLB directly, as NonCachable and Bufferable attributes for ARM9, Shared Device attribute for ARM11 and Cortex
    if(VirtualSetAttributes((void *)m_VideoMemoryVirtualBase, m_VideoMemorySize, 0x4, 0xc, NULL) != TRUE)
    {
        RETAILMSG(DISP_ZONE_WARNING, (_T("[DISPDRV:ERR] AllocResource() : m_VideoMemoryVirtualBase VirtualSetAttributes() Failed : %x\n\r"), GetLastError()));
    }
#endif
    RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV:INF] m_VideoMemoryPhysicalBase = 0x%08x\n\r"), m_VideoMemoryPhysicalBase));
    RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV:INF] m_VideoMemoryVirtualBase = 0x%08x\n\r"), m_VideoMemoryVirtualBase));
    RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV:INF] m_VideoMemorySize = 0x%08x\n\r"), m_VideoMemorySize));

    // Allocate SurfaceHeap
    m_pVideoMemoryHeap = new SurfaceHeap(m_VideoMemorySize, m_VideoMemoryVirtualBase, NULL, NULL);
    if(!m_pVideoMemoryHeap)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s : SurfaceHeap() allocate Fail\n\r"),_T(__FUNCTION__)));
        return FALSE;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s\n\r"), _T(__FUNCTION__)));

    return TRUE;

CleanUp:

    RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] --%s : Failed, ioPhysicalBase(0x%x,0x%x)\r\n"), _T(__FUNCTION__), ioPhysicalBase.LowPart, ioPhysicalBase.HighPart));


    return FALSE;
}


void
S3C6410Disp::ReleaseResource(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    DWORD dwBytes;

    if (m_pVideoMemoryHeap != NULL)
    {
        delete m_pVideoMemoryHeap;
    }

    if (m_VideoMemoryVirtualBase != NULL)
    {
        VirtualFree((LPVOID)m_VideoMemoryVirtualBase, 0, MEM_RELEASE);
    }

    if (m_pDispConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)m_pDispConReg, sizeof(S3C6410_DISPLAY_REG));
        m_pDispConReg = NULL;
    }

    if (m_pGPIOReg != NULL)
    {
        MmUnmapIoSpace((PVOID)m_pGPIOReg, sizeof(S3C6410_GPIO_REG));
        m_pGPIOReg = NULL;
    }

    if (m_pSYSReg != NULL)
    {
        MmUnmapIoSpace((PVOID)m_pSYSReg, sizeof(S3C6410_SYSCON_REG));
        m_pSYSReg = NULL;
    }

    // Release FIMD Win1 H/W Resource to Video Engine Driver for Primary Window
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_WIN1, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] ReleaseResource() : IOCTL_SVE_RSC_RELEASE_FIMD_WIN1 Failed\n\r")));
    }

    // Release FIMD H/W Resource to Video Engine Driver
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_INTERFACE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] ReleaseResource() : IOCTL_SVE_RSC_RELEASE_FIMD_INTERFACE Failed\n\r")));
    }

    // Close Video Engine Driver
    if (m_hVideoDrv!= INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hVideoDrv);
        m_hVideoDrv = INVALID_HANDLE_VALUE;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));
}


BOOL
S3C6410Disp::TVOutAllocResource(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    BOOL bRet = TRUE;
    DWORD dwBytes;

    // Request TV Scaler & TV Encoder H/W Resource to Video Engine Driver for TV Out
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_REQUEST_TVSCALER_TVENCODER, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] TVOutAllocResource() : IOCTL_SVE_RSC_REQUEST_TVSCALER_TVENCODER Failed\n\r")));
        bRet = FALSE;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return bRet;
}


BOOL
S3C6410Disp::TVOutReleaseResource(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    BOOL bRet = TRUE;
    DWORD dwBytes;

    // Release TV Scaler & TV Encoder H/W Resource to Video Engine Driver
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_RELEASE_TVSCALER_TVENCODER, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] TVOutReleaseResource() : IOCTL_SVE_RSC_RELEASE_TVSCALER_TVENCODER Failed\n\r")));
        bRet = FALSE;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return bRet;
}

// returns DD_OK for OK otherwise error code
// populates *pFormat, and *pPixelFormat
SCODE
S3C6410Disp::DetectPixelFormat(
                DWORD                dwCaps,            // in: DDSCAPS_xxx flags
                DDPIXELFORMAT*        pDDPF,            // in: Explicit pixel format or current mode
                EGPEFormat*            pFormat,
                EDDGPEPixelFormat*    pPixelFormat
                )
{
    SCODE rv = DDGPE::DetectPixelFormat(dwCaps, pDDPF, pFormat, pPixelFormat);

    if (rv == DDERR_UNSUPPORTEDFORMAT)
    {
        if(pDDPF->dwFlags & DDPF_FOURCC)
        {
            if( pDDPF->dwFourCC == FOURCC_I420 )
            {
                *pPixelFormat = (EDDGPEPixelFormat)ddgpePixelFormat_I420;
                *pFormat = gpe16Bpp;    // 12Bpp is not defined in GPE
                return DD_OK;
            }
            else     if( pDDPF->dwFourCC == FOURCC_YVYU )
            {
                *pPixelFormat = (EDDGPEPixelFormat)ddgpePixelFormat_YVYU;
                *pFormat = gpe16Bpp;
                return DD_OK;
            }
            else     if( pDDPF->dwFourCC == FOURCC_VYUY )
            {
                *pPixelFormat = (EDDGPEPixelFormat)ddgpePixelFormat_VYUY;
                *pFormat = gpe16Bpp;
                return DD_OK;
            }
        }

        return DDERR_UNSUPPORTEDFORMAT;
    }
    else
    {
        return rv;
    }
}

void
S3C6410Disp::SetDisplayPowerState(VIDEO_POWER_STATE PowerState)
{
    static BYTE * pVideoMemory = NULL;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s(%d)\n\r"), _T(__FUNCTION__), PowerState));

    // If we're already in the appropriate state, just return
    if (m_VideoPowerState == PowerState)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] SetDisplayPowerState() : Same as current State [%d]\n\r"), m_VideoPowerState));
        return;
    }

    if (PowerState == VideoPowerOn)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] SetDisplayPowerState() : VideoPowerOn\n\r")));

        if (m_VideoPowerState == VideoPowerOn)
        {
            // Do Nothing
        }
        else        // from VideoPowerOff or VideoPowerSuspend
        {
            DevPowerOn();

            DynRotate(m_iRotate);

            m_CursorDisabled = FALSE;
            //CursorOn();    // in DynRotate()
            //m_CursorVisible = TRUE;    // in CursorOn()
        }

        m_VideoPowerState = VideoPowerOn;

    }
    else if (PowerState == VideoPowerOff)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV] SetDisplayPowerState() : VideoPowerOff\n\r")));

        if (m_VideoPowerState == VideoPowerOff)
        {
            // Do Nothing
        }
        else        // from VideoPowerOn or VideoPowerStandby
        {
            m_CursorDisabled = TRUE;
            CursorOff();

            // Turn Off Display Controller
            DevPowerOff();
        }

        m_VideoPowerState = VideoPowerOff;
    }
}


VIDEO_POWER_STATE
S3C6410Disp::GetDisplayPowerState(void)
{
    return m_VideoPowerState;
}


BOOL
S3C6410Disp::WaitForVerticalBlank(VB_STATUS Status)
{
    BOOL bRet = FALSE;

    bRet = DevWaitForVerticalBlank();

    return bRet;
}


DWORD
S3C6410Disp::GetScanLine(void)
{
    DWORD dwRet = 0;

    dwRet = DevGetScanLine();

    return dwRet;
}



BOOL
S3C6410Disp::DevInitialize(void)
{
    SVEARG_FIMD_OUTPUT_IF tParam;
    DWORD dwBytes;
	DWORD dwDisplayType[2] = {123,16};
	DWORD dwBytesRet = 0;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV_init] ++%s()\n\r"), _T(__FUNCTION__)));

    EnterCriticalSection(&m_csDevice);

	// we did this in the constructor already
	if (KernelIoControl(IOCTL_HAL_QUERY_DISPLAYSETTINGS, NULL, 0, dwDisplayType, sizeof(DWORD)*2, &dwBytesRet)  // get data from BSP_ARGS via KernelIOCtl
                        && (dwBytesRet == (sizeof(DWORD)*2)))
	{
		RETAILMSG(DISP_ZONE_ERROR,(TEXT("[DISPDRV_INIT] display driver display: %s\r\n"),LDI_getDisplayName((HITEG_DISPLAY_TYPE)dwDisplayType[0])));
		LDI_set_LCD_module_type((HITEG_DISPLAY_TYPE)dwDisplayType[0]);
	}
	else
	{
		RETAILMSG(DISP_ZONE_ERROR,(TEXT("[DISPDRV_INIT] Error getting Display type from args section via Kernel IOCTL!!!\r\n")));
	}
    // Initialize SFR Address of Sub Module
    LDI_initDisplay(LDI_getDisplayType(), m_pSYSReg, m_pDispConReg, m_pGPIOReg);

	m_pGPIOReg->SPCON &=~(3<<24);
    // Get RGB Interface Information from LDI Library
    LDI_fill_output_device_information(&tParam.tRGBDevInfo);

	//if(LDI_getDisplayMode()==DISP_VIDOUT_RGBIF)
	//{
		m_eOutputInterface = OUTPUT_IF_RGB;
	//}
/*
	if(LDI_getDisplayMode()==DISP_VIDOUT_TVENCODER)
	{
		m_eOutputInterface = OUTPUT_IF_TV;
	}
	
    tParam.dwTVOutScreenWidth = m_dwDeviceScreenWidth 	= LDI_GetDisplayWidth( LDI_getDisplayType() );
    tParam.dwTVOutScreenHeight = m_dwDeviceScreenHeight = LDI_GetDisplayHeight( LDI_getDisplayType() );

    // Initialize Display Interface Parameter (SVE)
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_INTERFACE_PARAM, &tParam, sizeof(SVEARG_FIMD_OUTPUT_IF), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevInitialize() : IOCTL_SVE_FIMD_SET_INTERFACE_PARAM Failed\n\r")));
    }

    // Check TV Scaler Limitation
    if ((m_eOutputInterface == OUTPUT_IF_TV) && (m_dwDeviceScreenWidth > 800))
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevInitialize() : For TVout, Screen width[%d] should be lower than 800 pixel\n\r"), m_dwDeviceScreenWidth));
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevInitialize() : Output Interface forced to RGB I/F\n\r")));
        m_eOutputInterface = OUTPUT_IF_RGB;
    }
*/
    if (m_eOutputInterface == OUTPUT_IF_RGB)
    {
        DevOutputEnableRGBIF();
    }
    else if (m_eOutputInterface == OUTPUT_IF_TV)
    {
        // Backlight Off
        //DevicePowerNotify(_T("BKL1:"),(_CEDEVICE_POWER_STATE)D4, POWER_NAME);        

        //DevOutputEnableTV();
    }

    LeaveCriticalSection(&m_csDevice);

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return TRUE;
}


BOOL
S3C6410Disp::DevPowerOn(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    EnterCriticalSection(&m_csDevice);


    ///////////////////////////////////////////////////////////////
    // Initialize LCD Module
    LDI_initialize_LCD_module();

    ///////////////////////////////////////////////////////////////
    // Power On Video Driver
    // All of HW State is restored by Video Driver
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_PM_SET_POWER_ON, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevPowerOn() : IOCTL_SVE_PM_SET_POWER_ON Failed\n\r")));
    }

    if (m_eOutputInterface == OUTPUT_IF_RGB)
    {

		///////////////////////////////////////////////////////////////
        // Backlight On
        DevicePowerNotify(_T("BKL1:"),(_CEDEVICE_POWER_STATE)D0, POWER_NAME);
    }

    LeaveCriticalSection(&m_csDevice);

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return TRUE;
}


BOOL
S3C6410Disp::DevPowerOff(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    EnterCriticalSection(&m_csDevice);

    if (m_eOutputInterface == OUTPUT_IF_RGB)
    {
        // Backlight Off
        DevicePowerNotify(_T("BKL1:"),(_CEDEVICE_POWER_STATE)D4, POWER_NAME);    
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Power Off Video Driver
    // All of HW State will be restored by Video Driver
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_PM_SET_POWER_OFF, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevPowerOff() : IOCTL_SVE_PM_SET_POWER_OFF Failed : m_hVideoDrv:0x%x\n\r"),m_hVideoDrv));
    }

    // Deinitialize LCD Module
    LDI_deinitialize_LCD_module();

    // Power Off Video Driver
    // All of HW State will be restored by Video Driver
    LeaveCriticalSection(&m_csDevice);

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return TRUE;
}


BOOL
S3C6410Disp::DevChangeOutputInterface(OUTPUT_INTERFACE eNewOutputIF)
{
    BOOL bRet = TRUE;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s(%d)\n\r"), _T(__FUNCTION__), eNewOutputIF));

    EnterCriticalSection(&m_csDevice);

    if (m_eOutputInterface == eNewOutputIF)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV:INF] DevChangeOutputInterface(%d) : Same Inteface... Not Changed\n\r"), eNewOutputIF));
        bRet = FALSE;
        goto CleanUp;
    }

    if (eNewOutputIF == OUTPUT_IF_TV)
    {
        // Check TV Scaler Limitation
        if (m_dwDeviceScreenWidth > 800)
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevChangeOutputInterface() : For TVout, Screen width[%d] should be lower than 800 pixel\n\r"), m_dwDeviceScreenWidth));
            bRet = FALSE;
            goto CleanUp;
        }

        // Request Video Driver HW Resource
        if (!TVOutAllocResource())
        {
            RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV:INF] DevChangeOutputInterface(%d) : TVOutAllocResource() Failed\n\r"), eNewOutputIF));
            bRet = FALSE;
            goto CleanUp;
        }
    }

    //---------------------------
    // Disable Old Output Interface
    //---------------------------
    if (m_eOutputInterface == OUTPUT_IF_RGB)    // Previous interface is RGB I/F
    {
        if (m_eTVDMAMode !=     TV_DMA_DISABLE)
        {
            DevOutputDisableTVDMA();
            m_eTVDMAMode = TV_DMA_DISABLE;
            RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV:INF] DevChangeOutputInterface() : TV DMA is disabled for TV Out\n\r")));
        }

        DevOutputDisableRGBIF();
    }
    else        // Previous interface is TV Out
    {
        DevOutputDisableTV();
    }

    //---------------------------
    // Update Output Interface State
    //---------------------------
    m_eOutputInterface = eNewOutputIF;

    if (m_eOutputInterface == OUTPUT_IF_RGB)
    {
        // Release Video Driver HW Resource
        TVOutReleaseResource();
    }

    //---------------------------
    // Enable New Output Interface
    //---------------------------
    RETAILMSG(DISP_ZONE_TEMP, (_T("[DISPDRV:INF] Enable New Output Interface(%d)\n\r"), m_eOutputInterface));

    if (m_eOutputInterface == OUTPUT_IF_RGB)
    {
        DevOutputEnableRGBIF();
    }
    else if (m_eOutputInterface == OUTPUT_IF_TV)
    {
        DevOutputEnableTV();
    }

CleanUp:

    LeaveCriticalSection(&m_csDevice);

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return bRet;
}


void
S3C6410Disp::DevOutputEnableRGBIF(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    // Display initialize to RGB I/F
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_OUTPUT_RGBIF, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableRGBIF() : IOCTL_SVE_FIMD_SET_OUTPUT_RGBIF Failed\n\r")));
    }

    switch(m_pModeEx->ePixelFormat)
    {
    case ddgpePixelFormat_565:
        {
            SVEARG_FIMD_WIN_MODE tParam;

            tParam.dwWinMode = PRIMARY_WINDOW_MODE;
            tParam.dwBPP = DISP_16BPP_565;
            tParam.dwWidth = m_dwDeviceScreenWidth;
            tParam.dwHeight = m_dwDeviceScreenHeight;
            tParam.dwOffsetX = 0;
            tParam.dwOffsetY = 0;

            // Primary Window Initialize
            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParam, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableRGBIF() : IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n\r")));
            }

            break;
        }
    //case ddgpePixelFormat_8880:    // FIMD can not support Packed RGB888
    case ddgpePixelFormat_8888:
        {
            SVEARG_FIMD_WIN_MODE tParam;

            tParam.dwWinMode = PRIMARY_WINDOW_MODE;
            tParam.dwBPP = DISP_24BPP_888;
            tParam.dwWidth = m_dwDeviceScreenWidth;
            tParam.dwHeight = m_dwDeviceScreenHeight;
            tParam.dwOffsetX = 0;
            tParam.dwOffsetY = 0;

            // Primary Window Initialize
            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParam, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableRGBIF() : IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n\r")));
            }

            break;
        }
    default:
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableRGBIF() : Unknown PixelFormat %d \n\r"), m_pModeEx->ePixelFormat));
        break;
    }

    // Set Primary Window Framebuffer
    SVEARG_FIMD_WIN_FRAMEBUFFER tParamFB;
    tParamFB.dwWinNum = PRIMARY_WINDOW;
    tParamFB.dwFrameBuffer = m_VideoMemoryPhysicalBase;
    tParamFB.bWaitForVSync = FALSE;
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER, &tParamFB, sizeof(SVEARG_FIMD_WIN_FRAMEBUFFER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableRGBIF() : IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER Failed\n\r")));
    }

    // Primary Window Enable
    DWORD dwParam;
    dwParam = PRIMARY_WINDOW;
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_ENABLE, &dwParam, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableRGBIF() : IOCTL_SVE_FIMD_SET_WINDOW_ENABLE Failed\n\r")));
    }

    // Initialize LCD Module
    LDI_initialize_LCD_module();

    // Video Output Enable
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_OUTPUT_ENABLE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableRGBIF() : IOCTL_SVE_LCD_SET_MODULE_ENABLE Failed\n\r")));
    }

    // Enable Overlay Window and Blending
    DevRecoverOverlay();

    // Backlight On
    DevicePowerNotify(_T("BKL1:"),(_CEDEVICE_POWER_STATE)D0, POWER_NAME);

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));
}


void
S3C6410Disp::DevOutputDisableRGBIF(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    // Backlight Off
    DevicePowerNotify(_T("BKL1:"),(_CEDEVICE_POWER_STATE)D4, POWER_NAME);    

    // Disable Overlay Window
    DevOverlayDisable();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Video Output Disable
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputDisableRGBIF() : IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE Failed\n\r")));
    }
    // Deinitialize LCD Module
    LDI_deinitialize_LCD_module();

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --S3C6410Disp::DevOutputDisableRGBIF()\n\r")));
}

BOOL
S3C6410Disp::DevWaitForVerticalBlank(void)
{
    BOOL bRet = TRUE;
    DWORD dwBytes;

    // Wait for Frame Interrupt
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_WAIT_FRAME_INTERRUPT, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevGetVerticalStatus() : IOCTL_SVE_FIMD_WAIT_FRAME_INTERRUPT Failed\n\r")));
        bRet = FALSE;
    }

    return bRet;
}


int
S3C6410Disp::DevGetVerticalStatus(void)
{
    SVEARG_FIMD_OUTPUT_STAT tParam;
    DWORD dwBytes;

    // Get Output Status
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_GET_OUTPUT_STATUS, NULL, 0, &tParam, sizeof(SVEARG_FIMD_OUTPUT_STAT), &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevGetVerticalStatus() : IOCTL_SVE_FIMD_GET_OUTPUT_STATUS Failed\n\r")));
    }

    return (int)tParam.dwVerticalStatus;
}


DWORD
S3C6410Disp::DevGetScanLine(void)
{
    SVEARG_FIMD_OUTPUT_STAT tParam;
    DWORD dwBytes;

    // Get Output Status
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_GET_OUTPUT_STATUS, NULL, 0, &tParam, sizeof(SVEARG_FIMD_OUTPUT_STAT), &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevGetScanLine() : IOCTL_SVE_FIMD_GET_OUTPUT_STATUS Failed\n\r")));
    }

    return tParam.dwLineCnt;
}

BOOL S3C6410Disp::DevEnableVsyncInterrupt(void)
{
    return(DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_VSYNC_ENABLE, NULL, 0, NULL, 0, NULL, NULL) );
}

BOOL S3C6410Disp::DevGetFlipStatus(void)
{
    return(DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_GET_FLIPSTATUS, NULL, 0, NULL, 0, NULL, NULL) );
}


// this routine converts a string into a GUID and returns TRUE if the
// conversion was successful.
BOOL
ConvertStringToGuid (LPCTSTR pszGuid, GUID *pGuid)
{
    UINT Data4[8];
    int  Count;
    BOOL fOk = FALSE;
    TCHAR *pszGuidFormat = _T("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

    DEBUGCHK(pGuid != NULL && pszGuid != NULL);
    __try
    {
        if (_stscanf(pszGuid, pszGuidFormat, &pGuid->Data1,
            &pGuid->Data2, &pGuid->Data3, &Data4[0], &Data4[1], &Data4[2], &Data4[3],
            &Data4[4], &Data4[5], &Data4[6], &Data4[7]) == 11)
        {
            for(Count = 0; Count < (sizeof(Data4) / sizeof(Data4[0])); Count++)
            {
                pGuid->Data4[Count] = (UCHAR) Data4[Count];
            }
        }
        fOk = TRUE;
    }
    __except(GetExceptionCode()==STATUS_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        DEBUGMSG(DISP_ZONE_ERROR, (TEXT("%s: Exception occured\n")));
        fOk=FALSE;
    }

    return fOk;
}

// This routine notifies the OS that we support the Power Manager IOCTLs (through
// ExtEscape(), which calls DrvEscape()).
BOOL
AdvertisePowerInterface(HMODULE hInst)
{
    BOOL fOk = FALSE;
    HKEY hk;
    DWORD dwStatus;
    TCHAR szTemp[MAX_PATH];
    GUID gClass;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++AdvertisePowerInterface()\n\r")));

    // assume we are advertising the default class
    fOk = ConvertStringToGuid(PMCLASS_DISPLAY, &gClass);
    DEBUGCHK(fOk);

    // check for an override in the registry
    dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\GDI\\Drivers"), 0, 0, &hk);
    if(dwStatus == ERROR_SUCCESS)
    {
        DWORD dwType, dwSize;

        dwSize = sizeof(szTemp);
        dwStatus = RegQueryValueEx(hk, _T("DisplayPowerClass"), NULL, &dwType, (LPBYTE) szTemp, &dwSize);
        szTemp[MAX_PATH-1] = 0;

        if(dwStatus == ERROR_SUCCESS && dwType == REG_SZ)
        {
            // got a guid string, convert it to a guid
            GUID gTemp;

            fOk = ConvertStringToGuid(szTemp, &gTemp);
            DEBUGCHK(fOk);
            if(fOk)
            {
                gClass = gTemp;
            }
        }

        // release the registry key
        RegCloseKey(hk);
    }

    // figure out what device name to advertise
    if(fOk)
    {
        fOk = GetModuleFileName(hInst, szTemp, sizeof(szTemp) / sizeof(szTemp[0]));
        DEBUGCHK(fOk);
    }

    // now advertise the interface
    if(fOk)
    {
        fOk = AdvertiseInterface(&gClass, szTemp, TRUE);
        DEBUGCHK(fOk);
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --AdvertisePowerInterface() : %d\n\r"), fOk));

    return fOk;
}

VIDEO_POWER_STATE
PmToVideoPowerState(CEDEVICE_POWER_STATE pmDx)
{
    VIDEO_POWER_STATE vps;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++PmToVideoPowerState(%d)\n\r"), pmDx));

    switch(pmDx)
    {
    case D0:        // Display On
        vps = VideoPowerOn;
        break;
    case D1:        // Display Standby
        //vps = VideoPowerStandBy;
        //break;
    case D2:        // Display Suspend
        //vps = VideoPowerSuspend;
        //break;
    case D3:        // Display Off
    case D4:        // Display Off
        vps = VideoPowerOff;
        break;
    default:
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] PmToVideoPowerState() : Unknown PM State %d\n\r"), pmDx));
        vps = VideoPowerOn;
        break;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --PmToVideoPowerState() : %d\n\r"), vps));

    return vps;
}

// this routine maps video power states to PM power states.
CEDEVICE_POWER_STATE
VideoToPmPowerState(VIDEO_POWER_STATE vps)
{
    CEDEVICE_POWER_STATE pmDx;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++VideoToPmPowerState(%d)\n\r"), vps));

    switch(vps)
    {
    case VideoPowerOn:
        pmDx = D0;
        break;
    case VideoPowerStandBy:
        pmDx = D1;
        break;
    case VideoPowerSuspend:
        pmDx = (CEDEVICE_POWER_STATE)D2;
        break;
    case VideoPowerOff:
        pmDx = (CEDEVICE_POWER_STATE)D4;
        break;
    default:
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] VideoToPmPowerState() : Unknown Video State %d\n\r"), vps));
        pmDx = D0;
        break;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --VideoToPmPowerState() : %d\n\r"), pmDx));

    return pmDx;
}

