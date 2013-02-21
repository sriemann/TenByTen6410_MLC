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

    tvcontrol.cpp

Abstract:

    This module implements the main class that derived from DDGPE of display driver to support DirectDraw
    In this part, there are codes related with TV-Out, This is special feature of S3C6410

Functions:

    Overlay Resource Control, Overlay Enable/Disable

Notes:

--*/

#include "precomp.h"

void
S3C6410Disp::DevOutputEnableTV(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++S3C6410Disp::DevOutputEnableTV()\n\r")));

    // Video Output Disable
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE Failed\n\r")));
    }

    // Display Controller initialize to TV Out
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_OUTPUT_TV, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_FIMD_SET_OUTPUT_TV Failed\n\r")));
    }

    // TV Scaler Initialize
    SVEARG_TVSC_PARAMETER tParamTVSC;
    tParamTVSC.dwOpMode = TVSC_FREE_RUN_MODE;
    tParamTVSC.dwScanMode = TVSC_INTERLACE;
    tParamTVSC.dwSrcType = TVSC_SRC_FIFO;
    tParamTVSC.dwSrcBaseWidth = m_dwDeviceScreenWidth;
    tParamTVSC.dwSrcBaseHeight = m_dwDeviceScreenHeight;
    tParamTVSC.dwSrcWidth = m_dwDeviceScreenWidth;
    tParamTVSC.dwSrcHeight = m_dwDeviceScreenHeight;
    tParamTVSC.dwSrcOffsetX = 0;
    tParamTVSC.dwSrcOffsetY = 0;
    tParamTVSC.dwDstType = TVSC_DST_FIFO_YUV444;
    tParamTVSC.dwDstBaseWidth = TV_DST_WIDTH*2;
    tParamTVSC.dwDstBaseHeight = TV_DST_HEIGHT;
    tParamTVSC.dwDstWidth = TV_DST_WIDTH*2;
    tParamTVSC.dwDstHeight = TV_DST_HEIGHT;
    tParamTVSC.dwDstOffsetX = 0;
    tParamTVSC.dwDstOffsetY = 0;
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVSC_SET_PROCESSING_PARAM, &tParamTVSC, sizeof(SVEARG_TVSC_PARAMETER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_TVSC_SET_PROCESSING_PARAM Failed\n\r")));
    }

    // TV Encoder Initialize
    SVEARG_TVENC_PARAMETER tParamTVEnc;
    tParamTVEnc.dwOutputType = TVENC_COMPOSITE;
    tParamTVEnc.dwOutputStandard = TVENC_NTSC_M;
    tParamTVEnc.dwMVisionPattern = TVENC_MACROVISION_OFF;
    tParamTVEnc.dwSrcWidth = TV_DST_WIDTH;
    tParamTVEnc.dwSrcHeight = TV_DST_HEIGHT;
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVENC_SET_INTERFACE_PARAM, &tParamTVEnc, sizeof(SVEARG_TVENC_PARAMETER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_TVENC_SET_INTERFACE_PARAM Failed\n\r")));
    }

    // TV Encoder On & TV Scaler Start
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVENC_SET_ENCODER_ON, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_TVENC_SET_ENCODER_ON Failed\n\r")));
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
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n\r")));
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
                RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_FIMD_SET_WINDOW_MODE Failed\n\r")));
            }

            break;
        }
    default:
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : Unknown PixelFormat %d \n\r"), m_pModeEx->ePixelFormat));
        break;
    }

    // Set Primary Window Framebuffer
    SVEARG_FIMD_WIN_FRAMEBUFFER tParamFB;
    tParamFB.dwWinNum = PRIMARY_WINDOW;
    tParamFB.dwFrameBuffer = m_VideoMemoryPhysicalBase;
    tParamFB.bWaitForVSync = FALSE;
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER, &tParamFB, sizeof(SVEARG_FIMD_WIN_FRAMEBUFFER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER Failed\n\r")));
    }

    // Primary Window Enable
    DWORD dwParam;
    dwParam = PRIMARY_WINDOW;
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_WINDOW_ENABLE, &dwParam, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_FIMD_SET_WINDOW_ENABLE Failed\n\r")));
    }

    // Video Output Enable
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_OUTPUT_ENABLE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_LCD_SET_MODULE_ENABLE Failed\n\r")));
    }

    // Enable Overlay Window and Blending
    DevRecoverOverlay();

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --S3C6410Disp::DevOutputEnableTV()\n\r")));
}


void
S3C6410Disp::DevOutputDisableTV(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++S3C6410Disp::DevOutputDisableTV()\n\r")));

    // Disable Overlay
    DevOverlayDisable();

    // Video Output Disable
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputDisableTV() : IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE Failed\n\r")));
    }

    // TV Scaler Stop & TV Encoder Off
    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVENC_SET_ENCODER_OFF, NULL, 0, NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputDisableTV() : IOCTL_SVE_TVENC_SET_ENCODER_OFF Failed\n\r")));
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --S3C6410Disp::DevOutputDisableTV()\n\r")));
}


BOOL
S3C6410Disp::DevSetTVDMAMode(TV_DMA_MODE eType)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++S3C6410Disp::DevSetTVDMAMode(%d)\n\r"), eType));

    if (m_eOutputInterface == OUTPUT_IF_TV)
    {
        // In case of OUTPUT_IF_TV, we can not use TV out DMA mode
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevSetTVDMAMode() : Current output I/F is TV out. TV DMA is not available.\n\r")));
        return FALSE;
    }
    else if (m_eTVDMAMode != TV_DMA_DISABLE)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevSetTVDMAMode() : TV DMA is already in use [%d] mode\n\r"), m_eTVDMAMode));
        return FALSE;
    }

    // Update TV DMA mode
    m_eTVDMAMode = eType;

    if (eType == TV_DMA_PRIMARY)
    {
        DevUpdateTVDMAContext();
        DevOutputEnableTVDMA();
    }
    else if (eType == TV_DMA_OVERLAY)
    {
        if (m_OverlayCtxt.bShow)
        {
            DevUpdateTVDMAContext();
            DevOutputEnableTVDMA();
        }
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --S3C6410Disp::DevSetTVDMAMode()\n\r")));

    return TRUE;
}


void
S3C6410Disp::DevUpdateTVDMAContext(void)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++S3C6410Disp::DevUpdateTVDMAContext()\n\r")));

    if (m_eTVDMAMode == TV_DMA_PRIMARY)
    {
        if (m_pModeEx->ePixelFormat == ddgpePixelFormat_565)
        {
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_RGB16;
        }
#if    0    // FIMD can not support Packed RGB888
        else if (m_pModeEx->ePixelFormat == ddgpePixelFormat_8880)
        {
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_RGB24;
        }
#endif
        else if (m_pModeEx->ePixelFormat == ddgpePixelFormat_8888)
        {
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_RGB24;
        }

        m_TVDMACtxt.uiSrcBaseWidth = m_dwDeviceScreenWidth;
        m_TVDMACtxt.uiSrcBaseHeight = m_dwDeviceScreenHeight;
        m_TVDMACtxt.uiSrcWidth = m_dwDeviceScreenWidth;
        m_TVDMACtxt.uiSrcHeight = m_dwDeviceScreenHeight;
        m_TVDMACtxt.uiSrcOffsetX = 0;
        m_TVDMACtxt.uiSrcOffsetY = 0;
        m_TVDMACtxt.uiBufferRGBY = m_VideoMemoryPhysicalBase;
        m_TVDMACtxt.uiBufferCb = 0;
        m_TVDMACtxt.uiBufferCr = 0;
    }
    else if (m_eTVDMAMode == TV_DMA_OVERLAY)
    {
        switch(m_OverlayCtxt.pSurface->PixelFormat())
        {
        case ddgpePixelFormat_565:
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_RGB16;
            break;
        //case ddgpePixelFormat_8880:    // FIMD can not support Packed RGB888
        case ddgpePixelFormat_8888:
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_RGB24;
            break;
        case ddgpePixelFormat_I420:
        case ddgpePixelFormat_YV12:    // Cb and Cr is swapped in S3C6410Surf()
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_YUV420;
            break;
        case ddgpePixelFormat_YUYV:
        case ddgpePixelFormat_YUY2:
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_YUV422_CRYCBY;
            break;
        case ddgpePixelFormat_UYVY:
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_YUV422_YCRYCB;
            break;
        case ddgpePixelFormat_YVYU:
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_YUV422_CBYCRY;
            break;
        case ddgpePixelFormat_VYUY:
            m_TVDMACtxt.dwSourceFormat = TVSC_SRC_YUV422_YCBYCR;
            break;
        default:
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] S3C6410Disp::DevUpdateTVDMAContext() : Unknown PixelFormat %d\n\r"), m_OverlayCtxt.pSurface->PixelFormat()));
            break;
        }

        m_TVDMACtxt.uiSrcBaseWidth = m_OverlayCtxt.pSurface->Width();
        m_TVDMACtxt.uiSrcBaseHeight = m_OverlayCtxt.pSurface->Height();
        m_TVDMACtxt.uiSrcWidth = m_OverlayCtxt.pSurface->Width();
        m_TVDMACtxt.uiSrcHeight = m_OverlayCtxt.pSurface->Height();
        m_TVDMACtxt.uiSrcOffsetX = 0;
        m_TVDMACtxt.uiSrcOffsetY = 0;
        m_TVDMACtxt.uiBufferRGBY = (unsigned int)(m_OverlayCtxt.pSurface->OffsetInVideoMemory() + m_VideoMemoryPhysicalBase);
        m_TVDMACtxt.uiBufferCb = m_TVDMACtxt.uiBufferRGBY+m_OverlayCtxt.pSurface->m_uiOffsetCb;
        m_TVDMACtxt.uiBufferCr = m_TVDMACtxt.uiBufferRGBY+m_OverlayCtxt.pSurface->m_uiOffsetCr;
    }

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --S3C6410Disp::DevUpdateTVDMAContext()\n\r")));
}


void
S3C6410Disp::DevOutputEnableTVDMA(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++S3C6410Disp::DevOutputEnableTVDMA()\n\r")));

    if (m_TVDMACtxt.uiSrcBaseWidth > 800)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTVDMA() : For TV DMA, Source width[%d] should be lower than 800 pixel\n\r"), m_TVDMACtxt.uiSrcBaseWidth));
        m_eTVDMAMode = TV_DMA_DISABLE;
        return;
    }

    if (!m_bTVDMARunning && (m_eTVDMAMode != TV_DMA_DISABLE))
    {
        // TV Scaler Initialize
        SVEARG_TVSC_PARAMETER tParamTVSC;
        tParamTVSC.dwOpMode = TVSC_FREE_RUN_MODE;
        tParamTVSC.dwScanMode = TVSC_INTERLACE;
        tParamTVSC.dwSrcType = m_TVDMACtxt.dwSourceFormat;
        tParamTVSC.dwSrcBaseWidth = m_TVDMACtxt.uiSrcBaseWidth;
        tParamTVSC.dwSrcBaseHeight = m_TVDMACtxt.uiSrcBaseHeight;
        tParamTVSC.dwSrcWidth = m_TVDMACtxt.uiSrcBaseWidth;
        tParamTVSC.dwSrcHeight = m_TVDMACtxt.uiSrcBaseHeight;
        tParamTVSC.dwSrcOffsetX = 0;
        tParamTVSC.dwSrcOffsetY = 0;
        tParamTVSC.dwDstType = TVSC_DST_FIFO_YUV444;
        tParamTVSC.dwDstBaseWidth = TV_DST_WIDTH*2;
        tParamTVSC.dwDstBaseHeight = TV_DST_HEIGHT;
        tParamTVSC.dwDstWidth = TV_DST_WIDTH*2;
        tParamTVSC.dwDstHeight = TV_DST_HEIGHT;
        tParamTVSC.dwDstOffsetX = 0;
        tParamTVSC.dwDstOffsetY = 0;
        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVSC_SET_PROCESSING_PARAM, &tParamTVSC, sizeof(SVEARG_TVSC_PARAMETER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTVDMA() : IOCTL_SVE_TVSC_SET_PROCESSING_PARAM Failed\n\r")));
        }

        // TV Scaler Source Buffer Initialize
        SVEARG_TVSC_BUFFER tParamFB;
        tParamFB.dwBufferRGBY = m_TVDMACtxt.uiBufferRGBY;
        tParamFB.dwBufferCb = m_TVDMACtxt.uiBufferCb;
        tParamFB.dwBufferCr = m_TVDMACtxt.uiBufferCr;
        tParamFB.bWaitForVSync = FALSE;

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVSC_SET_SOURCE_BUFFER, &tParamFB, sizeof(SVEARG_TVSC_BUFFER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTVDMA() : IOCTL_SVE_TVSC_SET_SOURCE_BUFFER Failed\n\r")));
        }

        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVSC_SET_NEXT_SOURCE_BUFFER, &tParamFB, sizeof(SVEARG_TVSC_BUFFER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTVDMA() : IOCTL_SVE_TVSC_SET_NEXT_SOURCE_BUFFER Failed\n\r")));
        }

        // TV Encoder Initialize
        SVEARG_TVENC_PARAMETER tParamTVEnc;
        tParamTVEnc.dwOutputType = TVENC_COMPOSITE;
        tParamTVEnc.dwOutputStandard = TVENC_NTSC_M;
        tParamTVEnc.dwMVisionPattern = TVENC_MACROVISION_OFF;
        tParamTVEnc.dwSrcWidth = TV_DST_WIDTH;
        tParamTVEnc.dwSrcHeight = TV_DST_HEIGHT;
        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVENC_SET_INTERFACE_PARAM, &tParamTVEnc, sizeof(SVEARG_TVENC_PARAMETER), NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_TVENC_SET_INTERFACE_PARAM Failed\n\r")));
        }

        // TV Encoder On & TV Scaler Start
        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVENC_SET_ENCODER_ON, NULL, 0, NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputEnableTV() : IOCTL_SVE_TVENC_SET_ENCODER_ON Failed\n\r")));
        }
    }

    m_bTVDMARunning = TRUE;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --S3C6410Disp::DevOutputEnableTVDMA()\n\r")));
}


void
S3C6410Disp::DevOutputDisableTVDMA(void)
{
    DWORD dwBytes;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] ++S3C6410Disp::DevOutputDisableTVDMA()\n\r")));

    if (m_bTVDMARunning)
    {
        // TV Scaler Stop & TV Encoder Off
        if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVENC_SET_ENCODER_OFF, NULL, 0, NULL, 0, &dwBytes, NULL) )
        {
            RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevOutputDisableTVDMA() : IOCTL_SVE_TVENC_SET_ENCODER_OFF Failed\n\r")));
        }
    }

    m_bTVDMARunning = FALSE;

    RETAILMSG(DISP_ZONE_ENTER, (_T("[DISPDRV] --S3C6410Disp::DevOutputDisableTVDMA()\n\r")));
}


void
S3C6410Disp::DevSetTVDMABuffer(unsigned int uiAddrRGBY, unsigned int uiAddrCb, unsigned int uiAddrCr, BOOL bWaitForVBlank)
{
    // Update to Context
    m_TVDMACtxt.uiBufferRGBY = uiAddrRGBY;
    m_TVDMACtxt.uiBufferCb = uiAddrCb;
    m_TVDMACtxt.uiBufferCr = uiAddrCr;

    // TV Scaler Next Source Buffer Initialize
    DWORD dwBytes;
    SVEARG_TVSC_BUFFER tParamFB;
    tParamFB.dwBufferRGBY = m_TVDMACtxt.uiBufferRGBY;
    tParamFB.dwBufferCb = m_TVDMACtxt.uiBufferCb;
    tParamFB.dwBufferCr = m_TVDMACtxt.uiBufferCr;
    tParamFB.bWaitForVSync = bWaitForVBlank;

    if ( !DeviceIoControl(m_hVideoDrv, IOCTL_SVE_TVSC_SET_NEXT_SOURCE_BUFFER, &tParamFB, sizeof(SVEARG_TVSC_BUFFER), NULL, 0, &dwBytes, NULL) )
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] DevSetTVDMABuffer() : IOCTL_SVE_TVSC_SET_NEXT_SOURCE_BUFFER Failed\n\r")));
    }
}

