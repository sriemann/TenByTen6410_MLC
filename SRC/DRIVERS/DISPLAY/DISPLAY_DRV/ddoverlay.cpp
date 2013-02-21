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

    ddoverlay.cpp

Abstract:

    This module implements the main class that derived from DDGPE of display driver to support DirectDraw
    In this part, there are codes that implement base logical HW control functions to use DirectDraw Overlay

Functions:

    Overlay Resource Control, Overlay Enable/Disable

Notes:

--*/

#include "precomp.h"

BOOL
S3C6410Disp::OverlayAllocResource(BOOL bLocalPath)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s(%d)\n\r"), _T(__FUNCTION__), bLocalPath));

    DWORD dwBytes;

    // Request FIMD Win0 H/W Resource to Video Engine Driver for Overlay Window
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_REQUEST_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s() : IOCTL_SVE_RSC_REQUEST_FIMD_WIN0 Failed\n\r"), _T(__FUNCTION__)));
        goto AllocFail;
    }

    if (bLocalPath)
    {
        // Request Post Processor H/W Resource to Video Engine Driver for Overlay Window
        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_REQUEST_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s() : IOCTL_SVE_RSC_REQUEST_POST Failed\n\r"), _T(__FUNCTION__)));
            goto AllocFail;
        }
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return TRUE;

AllocFail:

    // Release Partially Allocated Resource
    OverlayReleaseResource(bLocalPath);

    RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] --%s() : Failed\n\r"), _T(__FUNCTION__)));

    return FALSE;
}


BOOL
S3C6410Disp::OverlayReleaseResource(BOOL bLocalPath)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s(%d)\n\r"), _T(__FUNCTION__), bLocalPath));

    BOOL bRet = TRUE;
    DWORD dwBytes;

    // Release FIMD Win0 H/W Resource to Video Engine Driver for Overlay Window
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_RELEASE_FIMD_WIN0, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s() : IOCTL_SVE_RSC_RELEASE_FIMD_WIN0 Failed\n\r"), _T(__FUNCTION__)));
        bRet = FALSE;
    }

    if (bLocalPath)
    {
        // Release Post Processor H/W Resource to Video Engine Driver for Overlay Window
        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_RSC_RELEASE_POST, NULL, 0, NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s() : IOCTL_SVE_RSC_RELEASE_POST Failed\n\r"), _T(__FUNCTION__)));
            bRet = FALSE;
        }
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));

    return bRet;
}


BOOL
S3C6410Disp::OverlayInitialize(S3C6410Surf* pOverlaySurface, RECT *pSrc, RECT *pDest)
{
    BOOL bRet = TRUE;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DDHAL:INF] OverlayInitialize() (%d,%d) (%d,%d,%d,%d) (%d,%d,%d,%d)\n\r"),
                    pOverlaySurface->Width(), pOverlaySurface->Height(),
                    pSrc->left, pSrc->top, pSrc->right, pSrc->bottom,
                    pDest->left, pDest->top, pDest->right, pDest->bottom));

    EnterCriticalSection(&m_csDevice);

    m_OverlayCtxt.pSurface = pOverlaySurface;

    switch(m_iRotate)
    {
    case DMDO_0:
    default:
        // Driver support Overlay Source Clipping
        m_OverlayCtxt.uiSrcWidth = pSrc->right - pSrc->left;
        m_OverlayCtxt.uiSrcHeight = pSrc->bottom - pSrc->top;
        m_OverlayCtxt.uiSrcOffsetX = pSrc->left;
        m_OverlayCtxt.uiSrcOffsetY = pSrc->top;

        //  Driver support Overlay Destination Stretch
        m_OverlayCtxt.uiDstWidth = pDest->right - pDest->left;
        m_OverlayCtxt.uiDstHeight = pDest->bottom - pDest->top;
        m_OverlayCtxt.uiDstOffsetX = pDest->left;
        m_OverlayCtxt.uiDstOffsetY = pDest->top;
        break;
    case DMDO_90:
        // Driver support Overlay Source Clipping
        m_OverlayCtxt.uiSrcWidth = pSrc->bottom - pSrc->top;
        m_OverlayCtxt.uiSrcHeight = pSrc->right - pSrc->left;
        m_OverlayCtxt.uiSrcOffsetX = pSrc->top;
        m_OverlayCtxt.uiSrcOffsetY = pOverlaySurface->Height()
                                        - m_OverlayCtxt.uiSrcHeight - pSrc->left;

        //  Driver support Overlay Destination Stretch
        m_OverlayCtxt.uiDstWidth = pDest->bottom - pDest->top;
        m_OverlayCtxt.uiDstHeight = pDest->right - pDest->left;
        m_OverlayCtxt.uiDstOffsetX = pDest->top;
        m_OverlayCtxt.uiDstOffsetY = m_pPrimarySurface->ScreenHeight()
                                        - m_OverlayCtxt.uiDstHeight - pDest->left;
        break;
    case DMDO_180:
        // Driver support Overlay Source Clipping
        m_OverlayCtxt.uiSrcWidth = pSrc->right - pSrc->left;
        m_OverlayCtxt.uiSrcHeight = pSrc->bottom - pSrc->top;
        m_OverlayCtxt.uiSrcOffsetX = pOverlaySurface->Width()
                                        - m_OverlayCtxt.uiSrcWidth - pSrc->left;
        m_OverlayCtxt.uiSrcOffsetY = pOverlaySurface->Height()
                                        - m_OverlayCtxt.uiSrcHeight - pSrc->top;

        //  Driver support Overlay Destination Stretch
        m_OverlayCtxt.uiDstWidth = pDest->right - pDest->left;
        m_OverlayCtxt.uiDstHeight = pDest->bottom - pDest->top;
        m_OverlayCtxt.uiDstOffsetX = m_pPrimarySurface->ScreenWidth()
                                        - m_OverlayCtxt.uiDstWidth - pDest->left;
        m_OverlayCtxt.uiDstOffsetY = m_pPrimarySurface->ScreenHeight()
                                        - m_OverlayCtxt.uiDstHeight - pDest->top;
        break;
    case DMDO_270:
        // Driver support Overlay Source Clipping
        m_OverlayCtxt.uiSrcHeight = pSrc->right - pSrc->left;
        m_OverlayCtxt.uiSrcWidth = pSrc->bottom - pSrc->top;
        m_OverlayCtxt.uiSrcOffsetX = pOverlaySurface->Width()
                                        - m_OverlayCtxt.uiSrcWidth - pSrc->top;;
        m_OverlayCtxt.uiSrcOffsetY = pSrc->left;

        //  Driver support Overlay Destination Stretch
        m_OverlayCtxt.uiDstHeight = pDest->right - pDest->left;
        m_OverlayCtxt.uiDstWidth = pDest->bottom - pDest->top;
        m_OverlayCtxt.uiDstOffsetX = m_pPrimarySurface->ScreenWidth()
                                        - m_OverlayCtxt.uiDstWidth - pDest->top;
        m_OverlayCtxt.uiDstOffsetY = pDest->left;
        break;
    }

    switch(m_OverlayCtxt.pSurface->PixelFormat())
    {
    case ddgpePixelFormat_I420:    // YUV420
    case ddgpePixelFormat_YV12:    // YVU420
        m_OverlayCtxt.bLocalPath = TRUE;
        m_OverlayCtxt.dwWinMode = DISP_WIN0_POST_RGB;
        m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
        m_OverlayCtxt.dwPostSrcType = POST_SRC_YUV420;
        break;
    case ddgpePixelFormat_YUYV:    // YUV422 (YCbYCr)
    case ddgpePixelFormat_YUY2:    // YUV422 (YCbYCr)
        m_OverlayCtxt.bLocalPath = TRUE;
        m_OverlayCtxt.dwWinMode = DISP_WIN0_POST_RGB;
        m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
        m_OverlayCtxt.dwPostSrcType = POST_SRC_YUV422_CRYCBY;
        break;
    case ddgpePixelFormat_UYVY:    // YUV422 (CbYCrY)
        m_OverlayCtxt.bLocalPath = TRUE;
        m_OverlayCtxt.dwWinMode = DISP_WIN0_POST_RGB;
        m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
        m_OverlayCtxt.dwPostSrcType = POST_SRC_YUV422_YCRYCB;
        break;
    case ddgpePixelFormat_YVYU:    // YUV422 (YCrYCb)
        m_OverlayCtxt.bLocalPath = TRUE;
        m_OverlayCtxt.dwWinMode = DISP_WIN0_POST_RGB;
        m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
        m_OverlayCtxt.dwPostSrcType = POST_SRC_YUV422_CBYCRY;
        break;
    case ddgpePixelFormat_VYUY:    // YUV422 (CrYCbY)
        m_OverlayCtxt.bLocalPath = TRUE;
        m_OverlayCtxt.dwWinMode = DISP_WIN0_POST_RGB;
        m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
        m_OverlayCtxt.dwPostSrcType = POST_SRC_YUV422_YCBYCR;
        break;
    case ddgpePixelFormat_565:
        if (    (m_OverlayCtxt.uiSrcWidth == pOverlaySurface->Width())
            && (m_OverlayCtxt.uiSrcHeight == pOverlaySurface->Height())
            && (m_OverlayCtxt.uiDstWidth == pOverlaySurface->Width())
            && (m_OverlayCtxt.uiDstHeight == pOverlaySurface->Height()))
        {
            // No Clipping and No Stretch, Don't Use Local Path for RGB
            m_OverlayCtxt.bLocalPath = FALSE;
            m_OverlayCtxt.dwWinMode = DISP_WIN0_DMA;
            m_OverlayCtxt.dwBPPMode = DISP_16BPP_565;
        }
        else
        {
            // Use Local Path for RGB
            m_OverlayCtxt.bLocalPath = TRUE;
            m_OverlayCtxt.dwWinMode = DISP_WIN0_POST_RGB;
            m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
            m_OverlayCtxt.dwPostSrcType = POST_SRC_RGB16;
        }
        break;
    //case ddgpePixelFormat_8880:    // FIMD can not support Packed RGB888
    case ddgpePixelFormat_8888:
        if (    (m_OverlayCtxt.uiSrcWidth == pOverlaySurface->Width())
            && (m_OverlayCtxt.uiSrcHeight == pOverlaySurface->Height())
            && (m_OverlayCtxt.uiDstWidth == pOverlaySurface->Width())
            && (m_OverlayCtxt.uiDstHeight == pOverlaySurface->Height()))
        {
            // No Clipping and No Stretch, Don't Use Local Path for RGB
            m_OverlayCtxt.bLocalPath = FALSE;
            m_OverlayCtxt.dwWinMode = DISP_WIN0_DMA;
            m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
        }
        else
        {
            // Use Local Path for RGB
            m_OverlayCtxt.bLocalPath = TRUE;
            m_OverlayCtxt.dwWinMode = DISP_WIN0_POST_RGB;
            m_OverlayCtxt.dwBPPMode = DISP_24BPP_888;
            m_OverlayCtxt.dwPostSrcType = POST_SRC_RGB24;
        }
        break;
    }

    // Request H/W Resource for Overlay to Video Engine Driver
    if (OverlayAllocResource(m_OverlayCtxt.bLocalPath) == FALSE)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] OverlayInitialize() : OverlayAllocResource() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Adjust for Post Processor and FIMD restriction
    switch(m_OverlayCtxt.pSurface->PixelFormat())
    {
    case ddgpePixelFormat_I420:
    case ddgpePixelFormat_YV12:
        m_OverlayCtxt.uiSrcWidth = m_OverlayCtxt.uiSrcWidth-m_OverlayCtxt.uiSrcWidth%8;
        m_OverlayCtxt.uiSrcHeight = m_OverlayCtxt.uiSrcHeight-m_OverlayCtxt.uiSrcHeight%2;
        m_OverlayCtxt.uiSrcOffsetX = m_OverlayCtxt.uiSrcOffsetX-m_OverlayCtxt.uiSrcOffsetX%8;

        if (m_OverlayCtxt.uiSrcWidth < 8) m_OverlayCtxt.uiSrcWidth = 8;
        if (m_OverlayCtxt.uiSrcHeight < 4) m_OverlayCtxt.uiSrcHeight = 4;
        if (m_OverlayCtxt.uiDstHeight < 3) m_OverlayCtxt.uiDstHeight = 3;
        break;
    case ddgpePixelFormat_YUYV:
    case ddgpePixelFormat_YUY2:
    case ddgpePixelFormat_UYVY:
    case ddgpePixelFormat_YVYU:
    case ddgpePixelFormat_VYUY:
        m_OverlayCtxt.uiSrcWidth = m_OverlayCtxt.uiSrcWidth-m_OverlayCtxt.uiSrcWidth%2;
        m_OverlayCtxt.uiSrcHeight = m_OverlayCtxt.uiSrcHeight-m_OverlayCtxt.uiSrcHeight%2;
        m_OverlayCtxt.uiSrcOffsetX = m_OverlayCtxt.uiSrcOffsetX-m_OverlayCtxt.uiSrcOffsetX%2;

        if (m_OverlayCtxt.uiSrcWidth < 2) m_OverlayCtxt.uiSrcWidth = 2;
        if (m_OverlayCtxt.uiSrcHeight < 2) m_OverlayCtxt.uiSrcHeight = 2;
        break;
    case ddgpePixelFormat_565:
        m_OverlayCtxt.uiSrcWidth = m_OverlayCtxt.uiSrcWidth-m_OverlayCtxt.uiSrcWidth%2;
        m_OverlayCtxt.uiSrcHeight = m_OverlayCtxt.uiSrcHeight-m_OverlayCtxt.uiSrcHeight%2;
        m_OverlayCtxt.uiSrcOffsetX = m_OverlayCtxt.uiSrcOffsetX-m_OverlayCtxt.uiSrcOffsetX%2;

        m_OverlayCtxt.uiDstWidth = m_OverlayCtxt.uiDstWidth-m_OverlayCtxt.uiDstWidth%2;
        m_OverlayCtxt.uiDstOffsetX = m_OverlayCtxt.uiDstOffsetX-m_OverlayCtxt.uiDstOffsetX%2;

        if (m_OverlayCtxt.uiSrcWidth < 2) m_OverlayCtxt.uiSrcWidth = 2;
        if (m_OverlayCtxt.uiSrcHeight < 2) m_OverlayCtxt.uiSrcHeight = 2;
        if (m_OverlayCtxt.uiDstWidth < 2) m_OverlayCtxt.uiDstWidth = 2;
        break;
    }

    // Adjust for Overlay Window Position
    if (m_OverlayCtxt.uiDstWidth+m_OverlayCtxt.uiDstOffsetX > (unsigned int)m_dwDeviceScreenWidth)
    {
        //m_OverlayCtxt.uiDstWidth = m_dwDeviceScreenWidth - m_OverlayCtxt.uiDstOffsetX;    // Adjust Width
        m_OverlayCtxt.uiDstOffsetX = m_dwDeviceScreenWidth - m_OverlayCtxt.uiDstWidth;        // Adjust Offset
    }

    if (m_OverlayCtxt.uiDstHeight+m_OverlayCtxt.uiDstOffsetY  > (unsigned int)m_dwDeviceScreenHeight)
    {
        //m_OverlayCtxt.uiDstHeight = m_dwDeviceScreenHeight - m_OverlayCtxt.uiDstOffsetY;    // Adjust Height
        m_OverlayCtxt.uiDstOffsetY  = m_dwDeviceScreenHeight - m_OverlayCtxt.uiDstHeight;    // Adjust Offset
    }

    // Update TV DMA Context
    if (m_eTVDMAMode == TV_DMA_OVERLAY)
    {
        DevUpdateTVDMAContext();
    }

    DevOverlayInitialize();

CleanUp:

    LeaveCriticalSection(&m_csDevice);

    return bRet;
}


void
S3C6410Disp::OverlaySetPosition(UINT32 uiOffsetX, UINT32 uiOffsetY)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("OverlaySetPosition(%d,%d)\n\r"), uiOffsetX, uiOffsetY));

    EnterCriticalSection(&m_csDevice);

    switch(m_iRotate)
    {
    case DMDO_0:
    default:
        m_OverlayCtxt.uiDstOffsetX = uiOffsetX;
        m_OverlayCtxt.uiDstOffsetY = uiOffsetY;
        break;
    case DMDO_90:
        m_OverlayCtxt.uiDstOffsetX = uiOffsetY;
        m_OverlayCtxt.uiDstOffsetY = m_pPrimarySurface->ScreenHeight()
                                        - m_OverlayCtxt.uiDstHeight - uiOffsetX;
        break;
    case DMDO_180:
        m_OverlayCtxt.uiDstOffsetX = m_pPrimarySurface->ScreenWidth()
                                        - m_OverlayCtxt.uiDstWidth - uiOffsetX;
        m_OverlayCtxt.uiDstOffsetY = m_pPrimarySurface->ScreenHeight()
                                        - m_OverlayCtxt.uiDstHeight - uiOffsetY;
        break;
    case DMDO_270:
        m_OverlayCtxt.uiDstOffsetX = m_pPrimarySurface->ScreenWidth()
                                        - m_OverlayCtxt.uiDstWidth - uiOffsetY;
        m_OverlayCtxt.uiDstOffsetY = uiOffsetX;
        break;
    }

    // Adjust for Post Processor and FIMD restriction
    if (m_OverlayCtxt.pSurface->PixelFormat() == ddgpePixelFormat_565)
    {
        m_OverlayCtxt.uiDstOffsetX = m_OverlayCtxt.uiDstOffsetX-m_OverlayCtxt.uiDstOffsetX%2;
    }

    // Adjust for Overlay Window Position
    if (m_OverlayCtxt.uiDstWidth+m_OverlayCtxt.uiDstOffsetX > (unsigned int)m_dwDeviceScreenWidth)
    {
        //m_OverlayCtxt.uiDstWidth = m_dwDeviceScreenWidth - m_OverlayCtxt.uiDstOffsetX;    // Adjust Width
        m_OverlayCtxt.uiDstOffsetX = m_dwDeviceScreenWidth - m_OverlayCtxt.uiDstWidth;        // Adjust Offset
    }

    if (m_OverlayCtxt.uiDstHeight+m_OverlayCtxt.uiDstOffsetY  > (unsigned int)m_dwDeviceScreenHeight)
    {
        //m_OverlayCtxt.uiDstHeight = m_dwDeviceScreenHeight - m_OverlayCtxt.uiDstOffsetY;    // Adjust Height
        m_OverlayCtxt.uiDstOffsetY  = m_dwDeviceScreenHeight - m_OverlayCtxt.uiDstHeight;    // Adjust Offset
    }

    DevOverlaySetPosition();

    LeaveCriticalSection(&m_csDevice);
}


void
S3C6410Disp::OverlayEnable(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] %s()\n\r"), _T(__FUNCTION__)));

    EnterCriticalSection(&m_csDevice);

    m_OverlayCtxt.bShow = TRUE;

    DevOverlayEnable();

    if (m_eTVDMAMode == TV_DMA_OVERLAY)
    {
        DevOutputEnableTVDMA();
    }

    LeaveCriticalSection(&m_csDevice);
}


void
S3C6410Disp::OverlayDisable(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] %s()\n\r"), _T(__FUNCTION__)));

    m_OverlayCtxt.pSurface = NULL;
    m_OverlayCtxt.pPrevSurface = NULL;

    m_OverlayCtxt.bShow = FALSE;

    EnterCriticalSection(&m_csDevice);

    DevOverlayDisable();

    // Update TV DMA Context
    if (m_eTVDMAMode == TV_DMA_OVERLAY)
    {
        DevOutputDisableTVDMA();
    }

    LeaveCriticalSection(&m_csDevice);

    // Release H/W Resource for Overlay to Video Engine Driver
    OverlayReleaseResource(m_OverlayCtxt.bLocalPath);
}


void
S3C6410Disp::OverlayBlendDisable()
{
    m_OverlayCtxt.bBlendOn = FALSE;
    DevOverlayBlendDisable();
}


void
S3C6410Disp::OverlaySetColorKey(BOOL bSrcCKey, EDDGPEPixelFormat Format, DWORD ColorKey)
{
    m_OverlayCtxt.bBlendOn = TRUE;
    m_OverlayCtxt.bColorKey = TRUE;
    m_OverlayCtxt.bSrcCKey = bSrcCKey;

    if (Format == ddgpePixelFormat_565)    // RGB565
    {
        m_OverlayCtxt.ColorKey =
                    (((ColorKey&0xF800)>>11)<<19) |    // R bit
                    (((ColorKey&0x07E0)>>5)<<10) |    // G bit
                    ((ColorKey&0x001F)<<3);            // B bit
        m_OverlayCtxt.CompareKey = 0x00070307;
    }
    else    // if (Format == ddgpePixelFormat_8888)    // RGB888
    {
        m_OverlayCtxt.ColorKey = ColorKey;
        m_OverlayCtxt.CompareKey = 0x00000000;
    }

    DevOverlaySetColorKey();
}

void
S3C6410Disp::OverlaySetAlpha(BOOL bUsePixelBlend, DWORD Alpha)
{
    m_OverlayCtxt.bBlendOn = TRUE;
    m_OverlayCtxt.bColorKey = FALSE;
    m_OverlayCtxt.bUsePixelBlend = bUsePixelBlend;
    m_OverlayCtxt.Alpha = Alpha;

    DevOverlaySetAlpha();
}


void
S3C6410Disp::InitalizeOverlayContext(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++InitalizeOverlayContext()\n\r")));

    m_OverlayCtxt.pSurface = NULL;            // Current Overlay Surface
    m_OverlayCtxt.pPrevSurface = NULL;

    m_OverlayCtxt.bLocalPath = FALSE;
    m_OverlayCtxt.uiSrcWidth = 0;
    m_OverlayCtxt.uiSrcHeight = 0;
    m_OverlayCtxt.uiSrcOffsetX = 0;
    m_OverlayCtxt.uiSrcOffsetY = 0;
    m_OverlayCtxt.uiDstWidth = 0;
    m_OverlayCtxt.uiDstHeight = 0;
    m_OverlayCtxt.uiDstOffsetX = 0;
    m_OverlayCtxt.uiDstOffsetY = 0;
    m_OverlayCtxt.bEnabled = FALSE;
    m_OverlayCtxt.bShow = FALSE;

    m_OverlayCtxt.bBlendOn = FALSE;
    m_OverlayCtxt.bColorKey = 0x0;
    m_OverlayCtxt.bSrcCKey = FALSE;
    m_OverlayCtxt.CompareKey = 0x0;
    m_OverlayCtxt.ColorKey = 0x0;
    m_OverlayCtxt.bUsePixelBlend = FALSE;
    m_OverlayCtxt.Alpha = 0x0;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --InitalizeOverlayContext()\n\r")));
}


S3C6410Surf*
S3C6410Disp::GetCurrentOverlaySurf(void)
{
    return m_OverlayCtxt.pSurface;
}


S3C6410Surf*
S3C6410Disp::GetPreviousOverlaySurf(void)
{
    return m_OverlayCtxt.pPrevSurface;
}

void
S3C6410Disp::DevSetVisibleSurface(S3C6410Surf *pSurf, BOOL bWaitForVBlank)
{
    unsigned int uiBuffer;
    BOOL bRetry = TRUE;
    DWORD dwBytes;

    if (!pSurf)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s() : pSurf is NULL\r\n"),_T(__FUNCTION__)));
        return;
    }

    if (pSurf->IsOverlay() == TRUE)
    {
        if (m_OverlayCtxt.bLocalPath)
        {
            SVEARG_POST_BUFFER tParam;

            tParam.dwBufferRGBY = m_OverlayCtxt.pSurface->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase;
            tParam.dwBufferCb = tParam.dwBufferRGBY+m_OverlayCtxt.pSurface->m_uiOffsetCb;
            tParam.dwBufferCr = tParam.dwBufferRGBY+m_OverlayCtxt.pSurface->m_uiOffsetCr;
            tParam.bWaitForVSync = bWaitForVBlank;

            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER, &tParam, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayInitialize() : IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER Failed\n\r")));
            }
        }
        else
        {
            SVEARG_FIMD_WIN_FRAMEBUFFER tParam;

            // Change Frame Buffer
            tParam.dwWinNum = OVERLAY_WINDOW;
            tParam.dwFrameBuffer = pSurf->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase;
            tParam.bWaitForVSync = bWaitForVBlank;
            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER, &tParam, sizeof(SVEARG_FIMD_WIN_FRAMEBUFFER), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s() : IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER Failed\n\r"), _T(__FUNCTION__)));
            }
        }

        //---------------
        // For TV Out DMA
        //---------------
        if (m_eTVDMAMode == TV_DMA_OVERLAY)
        {
            uiBuffer = (unsigned int)(pSurf->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase);
            DevSetTVDMABuffer(uiBuffer, uiBuffer+pSurf->m_uiOffsetCb, uiBuffer+pSurf->m_uiOffsetCr, bWaitForVBlank);
        }
    }
    else
    {
        SVEARG_FIMD_WIN_FRAMEBUFFER tParam;

        // Change Frame Buffer
        tParam.dwWinNum = PRIMARY_WINDOW;
        tParam.dwFrameBuffer = pSurf->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase;
        tParam.bWaitForVSync = bWaitForVBlank;
        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER, &tParam, sizeof(SVEARG_FIMD_WIN_FRAMEBUFFER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevSetVisibleSurface() : IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER Failed\n\r")));
        }

        //---------------
        // For TV Out DMA
        //---------------
        if (m_eTVDMAMode == TV_DMA_PRIMARY)
        {
            uiBuffer = (unsigned int)(pSurf->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase);
            DevSetTVDMABuffer(uiBuffer, uiBuffer+pSurf->m_uiOffsetCb, uiBuffer+pSurf->m_uiOffsetCr, bWaitForVBlank);
        }
    }
}


void
S3C6410Disp::DevOverlayInitialize(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++DevOverlayInitialize()\n\r")));

    DWORD dwBytes;

    // Display Overlay Window
    if (m_OverlayCtxt.bEnabled)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DDHAL] DevOverlayInitialize() : Disable Overlay\n\r")));
        DevOverlayDisable();
    }

    // Disable Overlay TV DMA
    if (m_eTVDMAMode == TV_DMA_OVERLAY)
    {
        RETAILMSG(DISP_ZONE_TEMP, (_T("[DDHAL] DevOverlayInitialize() : Disable TV DMA\n\r")));
        DevOutputDisableTVDMA();
    }

    // Initialize Overlay Widnow
    if (m_OverlayCtxt.bLocalPath)
    {
        SVEARG_FIMD_WIN_MODE tParamMode;
        SVEARG_POST_PARAMETER tParamPost;
        SVEARG_POST_BUFFER tParamBuffer;

        tParamMode.dwWinMode = m_OverlayCtxt.dwWinMode;
        tParamMode.dwBPP = m_OverlayCtxt.dwBPPMode;
        tParamMode.dwWidth = m_OverlayCtxt.uiDstWidth;
        tParamMode.dwHeight = m_OverlayCtxt.uiDstHeight;
        tParamMode.dwOffsetX = m_OverlayCtxt.uiDstOffsetX;
        tParamMode.dwOffsetY = m_OverlayCtxt.uiDstOffsetY;

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParamMode, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayInitialize() : IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n\r")));
        }

        tParamPost.dwOpMode = POST_FREE_RUN_MODE;
        tParamPost.dwScanMode = POST_PROGRESSIVE;
        tParamPost.dwSrcType = m_OverlayCtxt.dwPostSrcType;
        tParamPost.dwSrcBaseWidth = m_OverlayCtxt.pSurface->Width();
        tParamPost.dwSrcBaseHeight = m_OverlayCtxt.pSurface->Height();
        tParamPost.dwSrcWidth = m_OverlayCtxt.uiSrcWidth;
        tParamPost.dwSrcHeight = m_OverlayCtxt.uiSrcHeight;
        tParamPost.dwSrcOffsetX = m_OverlayCtxt.uiSrcOffsetX;
        tParamPost.dwSrcOffsetY = m_OverlayCtxt.uiSrcOffsetY;
        tParamPost.dwDstType = POST_DST_FIFO_RGB888;
        tParamPost.dwDstBaseWidth = m_OverlayCtxt.uiDstWidth;
        tParamPost.dwDstBaseHeight = m_OverlayCtxt.uiDstHeight;
        tParamPost.dwDstWidth = m_OverlayCtxt.uiDstWidth;
        tParamPost.dwDstHeight = m_OverlayCtxt.uiDstHeight;
        tParamPost.dwDstOffsetX = 0;
        tParamPost.dwDstOffsetY = 0;

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_POST_SET_PROCESSING_PARAM, &tParamPost, sizeof(SVEARG_POST_PARAMETER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayInitialize() : IOCTL_SVE_POST_SET_PROCESSING_PARAM Failed\n\r")));
        }

        tParamBuffer.dwBufferRGBY = m_OverlayCtxt.pSurface->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase;
        tParamBuffer.dwBufferCb = tParamBuffer.dwBufferRGBY+m_OverlayCtxt.pSurface->m_uiOffsetCb;
        tParamBuffer.dwBufferCr = tParamBuffer.dwBufferRGBY+m_OverlayCtxt.pSurface->m_uiOffsetCr;
        tParamBuffer.bWaitForVSync = FALSE;

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_POST_SET_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayInitialize() : IOCTL_SVE_POST_SET_SOURCE_BUFFER Failed\n\r")));
        }

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER, &tParamBuffer, sizeof(SVEARG_POST_BUFFER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayInitialize() : IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER Failed\n\r")));
        }
    }
    else
    {
        SVEARG_FIMD_WIN_MODE tParamMode;
        SVEARG_FIMD_WIN_FRAMEBUFFER tParamFB;

        tParamMode.dwWinMode = m_OverlayCtxt.dwWinMode;
        tParamMode.dwBPP = m_OverlayCtxt.dwBPPMode;
        tParamMode.dwWidth = m_OverlayCtxt.uiDstWidth;
        tParamMode.dwHeight = m_OverlayCtxt.uiDstHeight;
        tParamMode.dwOffsetX = m_OverlayCtxt.uiDstOffsetX;
        tParamMode.dwOffsetY = m_OverlayCtxt.uiDstOffsetY;

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_MODE, &tParamMode, sizeof(SVEARG_FIMD_WIN_MODE), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayInitialize() : IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n\r")));
        }

        tParamFB.dwWinNum = OVERLAY_WINDOW;
        tParamFB.dwFrameBuffer = m_OverlayCtxt.pSurface->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase;
        tParamFB.bWaitForVSync = FALSE;

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER, &tParamFB, sizeof(SVEARG_FIMD_WIN_FRAMEBUFFER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayInitialize() : IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER Failed\n\r")));
        }
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --DevOverlayInitialize()\n\r")));
}


void
S3C6410Disp::DevOverlaySetPosition(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++DevOverlaySetPosition()\n\r")));

    DWORD dwBytes;

    SVEARG_FIMD_WIN_POS tParam;

    tParam.dwWinNum = OVERLAY_WINDOW;
    tParam.dwOffsetX = m_OverlayCtxt.uiDstOffsetX;
    tParam.dwOffsetY = m_OverlayCtxt.uiDstOffsetY;

    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_POSITION, &tParam, sizeof(SVEARG_FIMD_WIN_POS), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlaySetPosition() : IOCTL_SVE_FIMD_SET_WINDOW_POSITION Failed\n\r")));
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --DevOverlaySetPosition()\n\r")));
}


void
S3C6410Disp::DevOverlayEnable(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    DWORD dwBytes;

    if (!m_OverlayCtxt.bEnabled)
    {
        if (m_OverlayCtxt.bLocalPath)
        {
            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_START, NULL, 0, NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlaySetPosition() : IOCTL_SVE_LOCALPATH_SET_WIN0_START Failed\n\r")));
            }
        }
        else
        {
            DWORD dwParam;
            dwParam = OVERLAY_WINDOW;
            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_ENABLE, &dwParam, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] %s() : IOCTL_SVE_FIMD_SET_WINDOW_ENABLE Failed\n\r"), _T(__FUNCTION__)));
            }
        }

        m_OverlayCtxt.bEnabled = TRUE;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --%s()\n\r"), _T(__FUNCTION__)));
}


void
S3C6410Disp::DevOverlayDisable(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++%s()\n\r"), _T(__FUNCTION__)));

    DWORD dwBytes;

    if (m_OverlayCtxt.bEnabled)
    {
        if (m_OverlayCtxt.bLocalPath)
        {
            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_LOCALPATH_SET_WIN0_STOP, NULL, 0, NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlaySetPosition() : IOCTL_SVE_LOCALPATH_SET_WIN0_STOP Failed\n\r")));
            }
        }
        else
        {
            DWORD dwParam;
            dwParam = OVERLAY_WINDOW;
            if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_DISABLE, &dwParam, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayDisable() : IOCTL_SVE_FIMD_SET_WINDOW_DISABLE Failed\n\r")));
            }
        }

        m_OverlayCtxt.bEnabled = FALSE;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --DevOverlayDisable()\n\r")));
}

void
S3C6410Disp::DevOverlayBlendDisable(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] DevOverlayBlendDisable()\n\r")));

    SVEARG_FIMD_WIN_COLORKEY tParamCKey;
    SVEARG_FIMD_WIN_ALPHA tParamAlpha;
    DWORD dwBytes;

    // Color Key Disable
    tParamCKey.dwWinNum = PRIMARY_WINDOW;
    tParamCKey.bOnOff = FALSE;
    tParamCKey.dwDirection = DISP_FG_MATCH_BG_DISPLAY;
    tParamCKey.dwColorKey = 0;
    tParamCKey.dwCompareKey = 0;

    // Alpha Set to 0x0 (Show Window0)
    tParamAlpha.dwWinNum = PRIMARY_WINDOW;
    tParamAlpha.dwMethod = DISP_ALPHA_PER_PLANE;
    tParamAlpha.dwAlpha0 = 0x0;
    tParamAlpha.dwAlpha1 = 0x0;

    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY, &tParamCKey, sizeof(SVEARG_FIMD_WIN_COLORKEY), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayBlendDisable() : IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n\r")));
    }

    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlayBlendDisable() : IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n\r")));
    }

}


void
S3C6410Disp::DevOverlaySetColorKey(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] DevOverlaySetColorKey()\n\r")));

    SVEARG_FIMD_WIN_COLORKEY tParamCKey;
    DWORD dwBytes;

    if (m_OverlayCtxt.bSrcCKey)
    {
        tParamCKey.dwWinNum = PRIMARY_WINDOW;
        tParamCKey.bOnOff = TRUE;
        tParamCKey.dwDirection = DISP_BG_MATCH_FG_DISPLAY;
        tParamCKey.dwColorKey = m_OverlayCtxt.ColorKey;
        tParamCKey.dwCompareKey = m_OverlayCtxt.CompareKey;
    }
    else
    {
        tParamCKey.dwWinNum = PRIMARY_WINDOW;
        tParamCKey.bOnOff = TRUE;
        tParamCKey.dwDirection = DISP_FG_MATCH_BG_DISPLAY;
        tParamCKey.dwColorKey = m_OverlayCtxt.ColorKey;
        tParamCKey.dwCompareKey = m_OverlayCtxt.CompareKey;
    }

    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY, &tParamCKey, sizeof(SVEARG_FIMD_WIN_COLORKEY), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlaySetColorKey() : IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY Failed\n\r")));
    }

}


void
S3C6410Disp::DevOverlaySetAlpha(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] DevOverlaySetAlpha()\n\r")));

    SVEARG_FIMD_WIN_ALPHA tParamAlpha;
    DWORD dwBytes;

    if (m_OverlayCtxt.bUsePixelBlend)
    {
        tParamAlpha.dwWinNum = PRIMARY_WINDOW;
        tParamAlpha.dwMethod = DISP_ALPHA_PER_PIXEL;
        tParamAlpha.dwAlpha0 = m_OverlayCtxt.Alpha;
        tParamAlpha.dwAlpha1 = 0x0;
    }
    else
    {
        tParamAlpha.dwWinNum = PRIMARY_WINDOW;
        tParamAlpha.dwMethod = DISP_ALPHA_PER_PLANE;
        tParamAlpha.dwAlpha0 = m_OverlayCtxt.Alpha;
        tParamAlpha.dwAlpha1 = m_OverlayCtxt.Alpha;
    }

    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA, &tParamAlpha, sizeof(SVEARG_FIMD_WIN_ALPHA), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOverlaySetAlpha() : IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA Failed\n\r")));
    }

}


void
S3C6410Disp::DevRecoverOverlay(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++DevRecoverOverlay()\n\r")));

    if (m_OverlayCtxt.bShow)
    {
        // Initialize Overlay Window
        DevOverlayInitialize();

        // Configure Blending
        if (m_OverlayCtxt.bBlendOn)
        {
            if (m_OverlayCtxt.bColorKey)
            {
                DevOverlaySetColorKey();
            }
            else
            {
                DevOverlaySetAlpha();
            }
        }
        else
        {
            DevOverlayBlendDisable();
        }

        DevOverlayEnable();
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --DevRecoverOverlay()\n\r")));
}