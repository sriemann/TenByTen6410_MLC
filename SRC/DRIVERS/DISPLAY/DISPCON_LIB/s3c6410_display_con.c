//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s3c6410_display_con.c

Abstract:       Implementation of Display Controller Control Library
                This module implements Low Level HW control 

Functions:


Notes:


--*/

#include <windows.h>
#include <bsp_cfg.h>    // for reference S3C6410_HCLK, S3C6410_ECLK
#include <s3c6410.h>
#include "s3c6410_display_con.h"
#include "s3c6410_display_con_macro.h"

#define DISP_MSG(x)    RETAILMSG(TRUE, x)
#define DISP_INF(x)    RETAILMSG(TRUE, x)
#define DISP_ERR(x)    RETAILMSG(TRUE, x)

#define ITU601OUTPUT    (FALSE)

BOOL g_flipCompleted=TRUE;
static volatile S3C6410_DISPLAY_REG *g_pDispConReg = NULL;
static volatile S3C6410_MSMIF_REG *g_pMSMIFReg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;

static tDevInfo g_DevInfoRGB;
static tDevInfo g_DevInfoTV;
static tDispWindow0Config g_Win0Config;
static tDispWindow12Config g_Win1Config;
static tDispWindow12Config g_Win2Config;
static tDispWindow34Config g_Win3Config;
static tDispWindow34Config g_Win4Config;

DISP_ERROR Disp_initialize_register_address(volatile S3C6410_DISPLAY_REG *pDispConReg,volatile S3C6410_MSMIF_REG *pMSMIFReg,volatile S3C6410_GPIO_REG *pGPIOReg)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_initialize_register_address(0x%08x, 0x%08x, 0x%08x)\n\r"), pDispConReg, pMSMIFReg, pGPIOReg));

    if (pDispConReg == NULL || pMSMIFReg == NULL || pGPIOReg == NULL)
    {
        DISP_ERR((_T("[DISP:ERR] Disp_initialize_register_address() : NULL pointer parameter\n\r")));
        error = DISP_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pDispConReg = (volatile S3C6410_DISPLAY_REG *)pDispConReg;
        g_pMSMIFReg = (volatile S3C6410_MSMIF_REG *)pMSMIFReg;
        g_pGPIOReg = (volatile S3C6410_GPIO_REG *)pGPIOReg;
        DISP_INF((_T("[DISP:INF] g_pDispConReg = 0x%08x\n\r"), g_pDispConReg));
        DISP_INF((_T("[DISP:INF] g_pMSMIFReg = 0x%08x\n\r"), g_pMSMIFReg));
        DISP_INF((_T("[DISP:INF] g_pGPIOReg    = 0x%08x\n\r"), g_pGPIOReg));
    }

    DISP_MSG((_T("[DISP]--Disp_initialize_register_address() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_output_device_information(void *pInfo)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_output_device_information(%0x08x)\n\r"), pInfo));

    if (pInfo == NULL)
    {
        DISP_ERR((_T("[DISP:ERR] Disp_set_output_device_information() : NULL pointer parameter\n\r")));
        error = DISP_ERROR_NULL_PARAMETER;
    }
    else
    {
		LDI_fill_output_device_information(&g_DevInfoRGB);
        //memcpy(&g_DevInfoRGB, pInfo, sizeof(tDevInfo));
    }

    DISP_MSG((_T("[DISP]--Disp_set_output_device_information()\n\r")));

    return error;
}

#if    0    // Depricated
void* Disp_get_output_device_information_buffer(void)
{
    DISP_MSG((_T("[DISP]++Disp_get_output_device_information_buffer()\n\r")));

    // Device Information is Filled by LDI_fill_output_device_information()
    return (void *)&g_DevInfoRGB;
}
#endif

DISP_ERROR Disp_set_output_TV_information(unsigned int uiWidth, unsigned int uiHeight)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_output_TV_information(%d, %d)\n\r"), uiWidth, uiHeight));

    if (uiWidth > 800)
    {
        DISP_ERR((_T("[DISP:ERR] Disp_set_output_TV_information() : Horizontal Resolution[%d] should lower than 800 pixel\n\r"), uiWidth));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        g_DevInfoTV.RGBOutMode = DISP_18BIT_RGB666_S;
        g_DevInfoTV.uiWidth = uiWidth;
        g_DevInfoTV.uiHeight = uiHeight;
        g_DevInfoTV.VBPD_Value = 3;
        g_DevInfoTV.VFPD_Value = 5;
        g_DevInfoTV.VSPW_Value = 5;
        g_DevInfoTV.HBPD_Value = 13;
        g_DevInfoTV.HFPD_Value = 8;
        g_DevInfoTV.HSPW_Value = 3;
        g_DevInfoTV.VCLK_Polarity = IVCLK_FALL_EDGE;
        g_DevInfoTV.HSYNC_Polarity = IHSYNC_LOW_ACTIVE;
        g_DevInfoTV.VSYNC_Polarity = IVSYNC_LOW_ACTIVE;
        g_DevInfoTV.VDEN_Polarity = IVDEN_HIGH_ACTIVE;
        g_DevInfoTV.PNR_Mode = PNRMODE_RGB_P;
        g_DevInfoTV.VCLK_Source = CLKSEL_F_EXT27M;
        g_DevInfoTV.VCLK_Direction = CLKDIR_DIRECT;
        g_DevInfoTV.Frame_Rate = 60;
    }

    DISP_MSG((_T("[DISP]--Disp_set_output_TV_information()\n\r")));

    return error;
}

DISP_ERROR Disp_initialize_output_interface(DISP_VIDOUT_MODE VidoutMode)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_initialize_output_interface(%d)\n\r"), VidoutMode));

    if (g_pDispConReg == NULL || g_pGPIOReg == NULL)
    {
        DISP_ERR((_T("[DISP:ERR] Disp_initialize_output_interface() : Register Address Not Initialized\n\r")));
        error = DISP_ERROR_NOT_INITIALIZED;
    }

    g_DevInfoRGB.VideoOutMode = VidoutMode;

    switch(VidoutMode)
    {
    case DISP_VIDOUT_RGBIF:
		DISP_INF((_T("[DISP:INF] Disp_initialize_port_RGBIF\n\r")));
        Disp_initialize_port_RGBIF(g_DevInfoRGB.RGBOutMode);
		DISP_INF((_T("[DISP:INF] Disp_initialize_RGBIF\n\r")));
        Disp_initialize_RGBIF();
        break;
    case DISP_VIDOUT_TVENCODER:
        // TODO: Port Close ???? Disp_initialize_port_RGBIF(g_DevInfoRGB.RGBOutMode);
        Disp_initialize_TVEnc();
        break;
    case DISP_VIDOUT_RGBIF_TVENCODER:
        Disp_initialize_port_RGBIF(g_DevInfoRGB.RGBOutMode);
        Disp_initialize_RGBIF_withTVEnc();
        break;
    case DISP_VIDOUT_I80IF_LDI0:
    case DISP_VIDOUT_I80IF_LDI1:
    case DISP_VIDOUT_I80IF_LDI0_TVENCODER:
    case DISP_VIDOUT_I80IF_LDI1_TVENCODER:
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_initialize_output_interface() : Not Implemented Video Output Mode [%d]\n\r"), VidoutMode));
        error = DISP_ERROR_NOT_IMPLEMENTED;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_initialize_output_interface() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_window_mode(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_window_mode(%d)\n\r"), Mode));

    switch(Mode)
    {
    case DISP_WIN0_DMA:
    case DISP_WIN0_POST_RGB:
    case DISP_WIN0_POST_YUV:
        Disp_window0_initialize(Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY);
        break;
    case DISP_WIN1_DMA:
    case DISP_WIN1_TVSCALER_RGB:
    case DISP_WIN1_TVSCALER_YUV:
    case DISP_WIN1_CIPREVIEW_RGB:
    case DISP_WIN1_CIPREVIEW_YUV:
        Disp_window1_initialize(Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY);
        break;
    case DISP_WIN2_DMA:
    case DISP_WIN2_TVSCALER_RGB:
    case DISP_WIN2_TVSCALER_YUV:
    case DISP_WIN2_CICODEC_RGB:
    case DISP_WIN2_CICODEC_YUV:
        Disp_window2_initialize(Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY);
        break;
    case DISP_WIN3_DMA:
        Disp_window3_initialize(Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY);
        break;
    case DISP_WIN4_DMA:
        Disp_window4_initialize(Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY);
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_window_mode() : Unsupported Window Mode [%d]\n\r"), Mode));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_set_window_mode() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_window_position(DISP_WINDOW Win, unsigned int uiOffsetX, unsigned int uiOffsetY)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_window_position(%d, %d, %d)\n\r"), Win, uiOffsetX, uiOffsetY));

    switch(Win)
    {
    case DISP_WIN0:
        g_Win0Config.uiOffsetX = uiOffsetX;
        g_Win0Config.uiOffsetY = uiOffsetY;

        g_pDispConReg->VIDOSD0A = OSD_LEFTTOPX_F(g_Win0Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win0Config.uiOffsetY);

        g_pDispConReg->VIDOSD0B = OSD_RIGHTBOTX_F(g_Win0Config.uiWidth+g_Win0Config.uiOffsetX-1) |
                                    OSD_RIGHTBOTY_F(g_Win0Config.uiHeight+g_Win0Config.uiOffsetY-1);
        break;
    case DISP_WIN1:
        g_Win1Config.uiOffsetX = uiOffsetX;
        g_Win1Config.uiOffsetY = uiOffsetY;

        g_pDispConReg->VIDOSD1A = OSD_LEFTTOPX_F(g_Win1Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win1Config.uiOffsetY);

        g_pDispConReg->VIDOSD1B = OSD_RIGHTBOTX_F(g_Win1Config.uiWidth+g_Win1Config.uiOffsetX-1) |
                                    OSD_RIGHTBOTY_F(g_Win1Config.uiHeight+g_Win1Config.uiOffsetY-1);
        break;
    case DISP_WIN2:
        g_Win2Config.uiOffsetX = uiOffsetX;
        g_Win2Config.uiOffsetY = uiOffsetY;

        g_pDispConReg->VIDOSD2A = OSD_LEFTTOPX_F(g_Win2Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win2Config.uiOffsetY);

        g_pDispConReg->VIDOSD2B = OSD_RIGHTBOTX_F(g_Win2Config.uiWidth+g_Win2Config.uiOffsetX-1) |
                                    OSD_RIGHTBOTY_F(g_Win2Config.uiHeight+g_Win2Config.uiOffsetY-1);
        break;
    case DISP_WIN3:
        g_Win3Config.uiOffsetX = uiOffsetX;
        g_Win3Config.uiOffsetY = uiOffsetY;

        g_pDispConReg->VIDOSD3A = OSD_LEFTTOPX_F(g_Win3Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win3Config.uiOffsetY);

        g_pDispConReg->VIDOSD3B = OSD_RIGHTBOTX_F(g_Win3Config.uiWidth+g_Win3Config.uiOffsetX-1) |
                                    OSD_RIGHTBOTY_F(g_Win3Config.uiHeight+g_Win3Config.uiOffsetY-1);
        break;
    case DISP_WIN4:
        g_Win4Config.uiOffsetX = uiOffsetX;
        g_Win4Config.uiOffsetY = uiOffsetY;

        g_pDispConReg->VIDOSD4A = OSD_LEFTTOPX_F(g_Win4Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win4Config.uiOffsetY);

        g_pDispConReg->VIDOSD4B = OSD_RIGHTBOTX_F(g_Win4Config.uiWidth+g_Win4Config.uiOffsetX-1) |
                                    OSD_RIGHTBOTY_F(g_Win4Config.uiHeight+g_Win4Config.uiOffsetY-1);
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_window_position() : Unknown Window Number [%d]\n\r"), Win));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    //DISP_MSG((_T("[DISP]--Disp_set_window_position() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_framebuffer(DISP_WINDOW Win, unsigned int uiFrameBufferAddress)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_framebuffer(%d, 0x%08x)\n\r"), Win, uiFrameBufferAddress));

    switch(Win)
    {
    case DISP_WIN0:
        if (g_Win0Config.LocalPathEnable == LOCAL_PATH_ENABLE)
        {
#if    0
            DISP_ERR((_T("[DISP:ERR] Disp_set_framebuffer() : Window0 Local Path is Enabled\n\r")));
            error = DISP_ERROR_ILLEGAL_PARAMETER;
#else
            // Safe Frame buffer Address for Local Path
            g_pDispConReg->VIDW00ADD0B0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
            g_pDispConReg->VIDW00ADD1B0 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+0x10);    // 4 Words
            g_pDispConReg->VIDW00ADD2 = PAGEWIDTH_F(0x10);                                    // 4 Words
            g_pDispConReg->WINCON0 &= ~(BUFSEL_BUF1);    // Buffer set to Buf0
#endif
        }
        else
        {
            g_pDispConReg->VIDW00ADD0B0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
            g_pDispConReg->VIDW00ADD1B0 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+g_Win0Config.uiPageWidth*g_Win0Config.uiHeight);
            g_pDispConReg->VIDW00ADD2 = PAGEWIDTH_F(g_Win0Config.uiPageWidth);
            g_pDispConReg->WINCON0 &= ~(BUFSEL_BUF1);    // Buffer set to Buf0
        }
        break;
    case DISP_WIN1:
        if (g_Win1Config.LocalPathEnable == LOCAL_PATH_ENABLE)
        {
#if    0
            DISP_ERR((_T("[DISP:ERR] Disp_set_framebuffer() : Window1 Local Path is Enabled\n\r")));
            error = DISP_ERROR_ILLEGAL_PARAMETER;
#else
            // Safe Frame buffer Address for Local Path
            g_pDispConReg->VIDW01ADD0B0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
            g_pDispConReg->VIDW01ADD1B0 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+0x10);    // 4 Words
            g_pDispConReg->VIDW01ADD2 = PAGEWIDTH_F(0x10);                                    // 4 Words
            g_pDispConReg->WINCON1 &= ~(BUFSEL_BUF1);    // Buffer set to Buf0
#endif
        }
        else
        {
            g_pDispConReg->VIDW01ADD0B0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
            g_pDispConReg->VIDW01ADD1B0 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+g_Win1Config.uiPageWidth*g_Win1Config.uiHeight);
            g_pDispConReg->VIDW01ADD2 = PAGEWIDTH_F(g_Win1Config.uiPageWidth);
            g_pDispConReg->WINCON1 &= ~(BUFSEL_BUF1);    // Buffer set to Buf0
        }
        break;
    case DISP_WIN2:
        if (g_Win2Config.LocalPathEnable == LOCAL_PATH_ENABLE)
        {
#if    0
            DISP_ERR((_T("[DISP:ERR] Disp_set_framebuffer() : Window2 Local Path is Enabled\n\r")));
            error = DISP_ERROR_ILLEGAL_PARAMETER;
#else
            // Safe Frame buffer Address for Local Path
            g_pDispConReg->VIDW02ADD0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
            g_pDispConReg->VIDW02ADD1 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+0x10);        // 4 Words
            g_pDispConReg->VIDW02ADD2 = PAGEWIDTH_F(0x10);                                    // 4 Words
#endif
        }
        else
        {
            g_pDispConReg->VIDW02ADD0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
            g_pDispConReg->VIDW02ADD1 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+g_Win2Config.uiPageWidth*g_Win2Config.uiHeight);
            g_pDispConReg->VIDW02ADD2 = PAGEWIDTH_F(g_Win2Config.uiPageWidth);
        }
        break;
    case DISP_WIN3:
        g_pDispConReg->VIDW03ADD0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
        g_pDispConReg->VIDW03ADD1 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+g_Win3Config.uiPageWidth*g_Win3Config.uiHeight);
        g_pDispConReg->VIDW03ADD2 = PAGEWIDTH_F(g_Win3Config.uiPageWidth);
        break;
    case DISP_WIN4:
        g_pDispConReg->VIDW04ADD0 = VBANK_F(uiFrameBufferAddress>>24) | VBASEU_F(uiFrameBufferAddress);
        g_pDispConReg->VIDW04ADD1 = VBASEL_F(VBASEU_F(uiFrameBufferAddress)+g_Win4Config.uiPageWidth*g_Win4Config.uiHeight);
        g_pDispConReg->VIDW04ADD2 = PAGEWIDTH_F(g_Win4Config.uiPageWidth);
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_framebuffer() : Unknown Window Number [%d]\n\r"), Win));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    //DISP_MSG((_T("[DISP]--Disp_set_framebuffer() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_envid_onoff(DISP_ENVID_ONOFF EnvidOnOff)
{
    DISP_ERROR error = DISP_SUCCESS;

	DISP_MSG((_T("[DISP]++Disp_envid_onoff(%s)\n\r"), (EnvidOnOff==DISP_ENVID_ON)?"ON":"OFF"));

    if (EnvidOnOff == DISP_ENVID_ON)
    {
        g_pDispConReg->VIDCON0 |= (ENVID_ENABLE | ENVID_F_ENABLE);
    }
    else if (EnvidOnOff == DISP_ENVID_OFF)
    {
        g_pDispConReg->VIDCON0 &= ~(ENVID_F_ENABLE);    // Per Frame Off
    }
    else if (EnvidOnOff == DISP_ENVID_DIRECT_OFF)
    {
        g_pDispConReg->VIDCON0 &= ~(ENVID_ENABLE | ENVID_F_ENABLE);    // Direct Off
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] Disp_envid_onoff() : Unknown Parameter [%d]\n\r"), EnvidOnOff));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
    }

    DISP_MSG((_T("[DISP]--Disp_envid_onoff() : %d\n\r"), error));

    return error;
}

BOOL Disp_get_envid_status(void)
{
    BOOL bRet = FALSE;

    DISP_MSG((_T("[DISP]++Disp_get_envid_status()\n\r")));

    if (g_pDispConReg->VIDCON0 & ENVID_ENABLE)
    {
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    DISP_MSG((_T("[DISP]--Disp_envid_onoff() : %d\n\r"), bRet));

    return bRet;
}

DISP_ERROR Disp_window_onfoff(DISP_WINDOW Win, DISP_WINDOW_ONOFF WinOnOff)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_window_onfoff(%d, %d)\n\r"), Win, WinOnOff));

    if (WinOnOff == DISP_WINDOW_ON || WinOnOff == DISP_WINDOW_OFF)
    {
        switch(Win)
        {
        case DISP_WIN0:
            if (WinOnOff == DISP_WINDOW_ON)
            {
                g_pDispConReg->WINCON0 |= (ENWIN_F_ENABLE | g_Win0Config.LocalPathEnable);
            }
            else
            {
                // ENLOCAL bit must be disabled after ENWIN_F has been disabled
                //g_pDispConReg->WINCON0 &= ~(ENWIN_F_ENABLE | g_Win0Config.LocalPathEnable);
                g_pDispConReg->WINCON0 &= ~ENWIN_F_ENABLE;
            }
            break;
        case DISP_WIN1:
            if (WinOnOff == DISP_WINDOW_ON)
            {
                g_pDispConReg->WINCON1 |= (ENWIN_F_ENABLE | g_Win1Config.LocalPathEnable);
            }
            else
            {
                // ENLOCAL bit must be disabled after ENWIN_F has been disabled
                //g_pDispConReg->WINCON1 &= ~(ENWIN_F_ENABLE | g_Win1Config.LocalPathEnable);
                g_pDispConReg->WINCON1 &= ~ENWIN_F_ENABLE;
            }
            break;
        case DISP_WIN2:
            if (WinOnOff == DISP_WINDOW_ON)
            {
                g_pDispConReg->WINCON2 |= (ENWIN_F_ENABLE | g_Win2Config.LocalPathEnable);
            }
            else
            {
                // ENLOCAL bit must be disabled after ENWIN_F has been disabled
                //g_pDispConReg->WINCON2 &= ~(ENWIN_F_ENABLE | g_Win2Config.LocalPathEnable);
                g_pDispConReg->WINCON2 &= ~ENWIN_F_ENABLE;
            }
            break;
        case DISP_WIN3:
            if (WinOnOff == DISP_WINDOW_ON)
            {
                g_pDispConReg->WINCON3 |= ENWIN_F_ENABLE;
            }
            else
            {
                g_pDispConReg->WINCON3 &= ~ENWIN_F_ENABLE;
            }
            break;
        case DISP_WIN4:
            if (WinOnOff == DISP_WINDOW_ON)
            {
                g_pDispConReg->WINCON4 |= ENWIN_F_ENABLE;
            }
            else
            {
                g_pDispConReg->WINCON4 &= ~ENWIN_F_ENABLE;
            }
            break;
        default:
            DISP_ERR((_T("[DISP:ERR] Disp_window_onfoff() : Unknown Window Number [%d]\n\r"), Win));
            error = DISP_ERROR_ILLEGAL_PARAMETER;
            break;
        }
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] Disp_window_onfoff() : Unknown OnOff Parameter [%d]\n\r"), WinOnOff));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
    }

    DISP_MSG((_T("[DISP]--Disp_window_onfoff() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_local_path_off(DISP_WINDOW Win)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_local_path_off(%d)\n\r"), Win));

    switch(Win)
    {
    case DISP_WIN0:
        // The precondition is that ENLOCAL bit must be disabled after ENWIN_F has been disabled
        g_pDispConReg->WINCON0 &= ~(g_Win0Config.LocalPathEnable);
        break;
    case DISP_WIN1:
        // The precondition is that ENLOCAL bit must be disabled after ENWIN_F has been disabled
        g_pDispConReg->WINCON1 &= ~(g_Win1Config.LocalPathEnable);
        break;
    case DISP_WIN2:
        // The precondition is that ENLOCAL bit must be disabled after ENWIN_F has been disabled
        g_pDispConReg->WINCON2 &= ~(g_Win2Config.LocalPathEnable);
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_local_path_off() : Unknown Window Number [%d]\n\r"), Win));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_local_path_off() : %d\n\r"), error));

    return error;
}


DISP_WINDOW_ONOFF Disp_get_window_status(DISP_WINDOW Win)
{
    unsigned int uiWinConReg;

    switch(Win)
    {
    case DISP_WIN0:
        uiWinConReg = g_pDispConReg->WINCON0;
        break;
    case DISP_WIN1:
        uiWinConReg = g_pDispConReg->WINCON1;
        break;
    case DISP_WIN2:
        uiWinConReg = g_pDispConReg->WINCON2;
        break;
    case DISP_WIN3:
        uiWinConReg = g_pDispConReg->WINCON3;
        break;
    case DISP_WIN4:
        uiWinConReg = g_pDispConReg->WINCON4;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_get_window_status() : Unknown Window Number [%d]\n\r"), Win));
        return -1;
    }

    if (uiWinConReg & ENWIN_F_ENABLE)
    {
        return DISP_WINDOW_ON;
    }
    else
    {
        return DISP_WINDOW_OFF;
    }
}

DISP_ERROR Disp_set_dithering_mode(DISP_DITHER_MODE Mode)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_dithering_mode(%d)\n\r"), Mode));

    switch(Mode)
    {
    case DISP_DITHER_OFF:
        g_pDispConReg->DITHMODE = DITHEN_F_DISABLE;
        break;
    case DISP_DITHER_565:
        g_pDispConReg->DITHMODE = RDITHPOS_5BIT | GDITHPOS_6BIT | BDITHPOS_5BIT | DITHEN_F_ENABLE;
        break;
    case DISP_DITHER_666:
        g_pDispConReg->DITHMODE = RDITHPOS_6BIT | GDITHPOS_6BIT | BDITHPOS_6BIT | DITHEN_F_ENABLE;
        break;
    case DISP_DITHER_888:
        g_pDispConReg->DITHMODE = RDITHPOS_8BIT | GDITHPOS_8BIT | BDITHPOS_8BIT | DITHEN_F_ENABLE;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_dithering_mode() : Unknown Mode [%d]\n\r"), Mode));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        g_pDispConReg->DITHMODE = DITHEN_F_DISABLE;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_set_dithering_mode() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_window_color_map(DISP_WINDOW Win, BOOL bOnOff, unsigned int uiColorValue)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_window_color_map(%d, %d, 0x%08x)\n\r"), Win, bOnOff, uiColorValue));

    switch(Win)
    {
    case DISP_WIN0:
        if (bOnOff)
        {
            g_pDispConReg->WIN0MAP = MAPCOLEN_F_ENABLE | MAPCOLOR(uiColorValue);
        }
        else
        {
            g_pDispConReg->WIN0MAP = MAPCOLEN_F_DISABLE;
        }
        break;
    case DISP_WIN1:
        if (bOnOff)
        {
            g_pDispConReg->WIN1MAP = MAPCOLEN_F_ENABLE | MAPCOLOR(uiColorValue);
        }
        else
        {
            g_pDispConReg->WIN1MAP = MAPCOLEN_F_DISABLE;
        }
        break;
    case DISP_WIN2:
        if (bOnOff)
        {
            g_pDispConReg->WIN2MAP = MAPCOLEN_F_ENABLE | MAPCOLOR(uiColorValue);
        }
        else
        {
            g_pDispConReg->WIN2MAP = MAPCOLEN_F_DISABLE;
        }
        break;
    case DISP_WIN3:
        if (bOnOff)
        {
            g_pDispConReg->WIN3MAP = MAPCOLEN_F_ENABLE | MAPCOLOR(uiColorValue);
        }
        else
        {
            g_pDispConReg->WIN3MAP = MAPCOLEN_F_DISABLE;
        }
        break;
    case DISP_WIN4:
        if (bOnOff)
        {
            g_pDispConReg->WIN4MAP = MAPCOLEN_F_ENABLE | MAPCOLOR(uiColorValue);
        }
        else
        {
            g_pDispConReg->WIN4MAP = MAPCOLEN_F_DISABLE;
        }
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_window_color_map() : Unknown Window Number [%d]\n\r"), Win));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_set_window_color_map() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_color_key(DISP_WINDOW Win, BOOL bOnOff, DISP_COLOR_KEY_DIRECTION Direction, unsigned int uiColorKey, unsigned int uiComparekey)
{
    DISP_ERROR error = DISP_SUCCESS;

    unsigned int uiKeyDir;

    DISP_MSG((_T("[DISP]++Disp_set_color_key(%d, %d, %d, 0x%08x, 0x%08x)\n\r"), Win, bOnOff, Direction, uiColorKey, uiComparekey));

    if (Direction == DISP_FG_MATCH_BG_DISPLAY)
    {
        uiKeyDir = DIRCON_FG_MATCH_BG_DISPLAY;
    }
    else
    {
        uiKeyDir = DIRCON_BG_MATCH_FG_DISPLAY;
    }

    switch(Win)
    {
    case DISP_WIN1:
        if (bOnOff)
        {
            g_pDispConReg->W1KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_ENABLE | uiKeyDir | COMPKEY(uiComparekey);
            g_pDispConReg->W1KEYCON1 = COLVAL(uiColorKey);
        }
        else
        {
            g_pDispConReg->W1KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_DISABLE;
        }
        break;
    case DISP_WIN2:
        if (bOnOff)
        {
            g_pDispConReg->W2KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_ENABLE | uiKeyDir | COMPKEY(uiComparekey);
            g_pDispConReg->W2KEYCON1 = COLVAL(uiColorKey);
        }
        else
        {
            g_pDispConReg->W2KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_DISABLE;
        }
        break;
    case DISP_WIN3:
        if (bOnOff)
        {
            g_pDispConReg->W3KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_ENABLE | uiKeyDir | COMPKEY(uiComparekey);
            g_pDispConReg->W3KEYCON1 = COLVAL(uiColorKey);
        }
        else
        {
            g_pDispConReg->W3KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_DISABLE;
        }
        break;
    case DISP_WIN4:
        if (bOnOff)
        {
            g_pDispConReg->W4KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_ENABLE | uiKeyDir | COMPKEY(uiComparekey);
            g_pDispConReg->W4KEYCON1 = COLVAL(uiColorKey);
        }
        else
        {
            g_pDispConReg->W4KEYCON0 = KEYBLEN_DISABLE | KEYEN_F_DISABLE;
        }
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_color_key() : Unknown Window Number [%d]\n\r"), Win));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_set_color_key() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_alpha_blending(DISP_WINDOW Win, DISP_ALPHA_BLEND_METHOD Method, unsigned int uiAlphaSet0, unsigned int uiAlphaSet1)
{
    DISP_ERROR error = DISP_SUCCESS;
    unsigned int uiMethod;

    DISP_MSG((_T("[DISP]++Disp_set_alpha_blending(%d, %d, 0x%08x, 0x%08x)\n\r"), Win, Method, uiAlphaSet0, uiAlphaSet1));

    if (Method == DISP_ALPHA_PER_PLANE)
    {
        uiMethod = BLEND_PER_PLANE;
    }
    else
    {
        uiMethod = BLEND_PER_PIXEL;
    }

    switch(Win)
    {
    case DISP_WIN1:
        g_pDispConReg->WINCON1 =  (g_pDispConReg->WINCON1 & ~(BLEND_PER_PIXEL|ALPHASEL_ALPHA1)) | uiMethod;
        g_pDispConReg->VIDOSD1C = ALPHA0_R(uiAlphaSet0) | ALPHA0_G(uiAlphaSet0) | ALPHA0_B(uiAlphaSet0)
                                    | ALPHA1_R(uiAlphaSet1) | ALPHA1_G(uiAlphaSet1) | ALPHA1_B(uiAlphaSet1);
        break;
    case DISP_WIN2:
        g_pDispConReg->WINCON2 =  (g_pDispConReg->WINCON2 & ~(BLEND_PER_PIXEL|ALPHASEL_ALPHA1)) | uiMethod;
        g_pDispConReg->VIDOSD2C = ALPHA0_R(uiAlphaSet0) | ALPHA0_G(uiAlphaSet0) | ALPHA0_B(uiAlphaSet0)
                                    | ALPHA1_R(uiAlphaSet1) | ALPHA1_G(uiAlphaSet1) | ALPHA1_B(uiAlphaSet1);
        break;
    case DISP_WIN3:
        g_pDispConReg->WINCON3 =  (g_pDispConReg->WINCON3 & ~(BLEND_PER_PIXEL|ALPHASEL_ALPHA1)) | uiMethod;
        g_pDispConReg->VIDOSD3C = ALPHA0_R(uiAlphaSet0) | ALPHA0_G(uiAlphaSet0) | ALPHA0_B(uiAlphaSet0)
                                    | ALPHA1_R(uiAlphaSet1) | ALPHA1_G(uiAlphaSet1) | ALPHA1_B(uiAlphaSet1);
        break;
    case DISP_WIN4:
        g_pDispConReg->WINCON4 =  (g_pDispConReg->WINCON4 & ~(BLEND_PER_PIXEL|ALPHASEL_ALPHA1)) | uiMethod;
        g_pDispConReg->VIDOSD4C = ALPHA0_R(uiAlphaSet0) | ALPHA0_G(uiAlphaSet0) | ALPHA0_B(uiAlphaSet0)
                                    | ALPHA1_R(uiAlphaSet1) | ALPHA1_G(uiAlphaSet1) | ALPHA1_B(uiAlphaSet1);
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_alpha_blending() : Unknown Window Number [%d]\n\r"), Win));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_set_alpha_blending() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_frame_interrupt(DISP_FRAME_INTERRUPT FrameInt)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_frame_interrupt(%d)\n\r"), FrameInt));

    g_pDispConReg->VIDINTCON0 &= ~FRAMEINT_MASK;    // Clear Al Frame Interrupt Bit

    switch(FrameInt)
    {
    case DISP_FRMINT_BACKPORCH:
        g_pDispConReg->VIDINTCON0 |= FRAMESEL0_BACK;
        break;
    case DISP_FRMINT_VSYNC:
        g_pDispConReg->VIDINTCON0 |= FRAMESEL0_VSYNC;
        break;
    case DISP_FRMINT_ACTIVE:
        g_pDispConReg->VIDINTCON0 |= FRAMESEL0_ACTIVE;
        break;
    case DISP_FRMINT_FRONTPORCH:
        g_pDispConReg->VIDINTCON0 |= FRAMESEL0_FRONT;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_frame_interrupt() : Unknown Frame Interrupt [%d]\n\r"), FrameInt));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_set_frame_interrupt() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_enable_frame_interrupt(void)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_enable_frame_interrupt()\n\r")));

    g_pDispConReg->VIDINTCON0 |= (INTFRMEN_ENABLE | INTEN_ENABLE);

    DISP_MSG((_T("[DISP]--Disp_enable_frame_interrupt() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_disable_frame_interrupt(void)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_disable_frame_interrupt()\n\r")));

    if (g_pDispConReg->VIDINTCON0 & INTFIFOEN_ENABLE)
    {
        // If FIFO Interrupt is enabled, Clear only Frame Interrupt Enable Bit
        g_pDispConReg->VIDINTCON0 &= ~INTFRMEN_ENABLE;
    }
    else
    {
        g_pDispConReg->VIDINTCON0 &= ~(INTFRMEN_ENABLE | INTEN_ENABLE);
    }

    DISP_MSG((_T("[DISP]--Disp_disable_frame_interrupt() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_set_fifo_interrupt(DISP_WINDOW Win, DISP_FIFO_INTERRUPT_LEVEL Level)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_set_fifo_interrupt(%d, %d)\n\r"), Win, Level));

    g_pDispConReg->VIDINTCON0 &= ~FIFOINT_MASK;    // Clear Al FIFO Interrupt Bit

    switch(Win)
    {
    case DISP_WIN0:
        g_pDispConReg->VIDINTCON0 |= FIFOSEL_WIN0;
        break;
    case DISP_WIN1:
        g_pDispConReg->VIDINTCON0 |= FIFOSEL_WIN1;
        break;
    case DISP_WIN2:
        g_pDispConReg->VIDINTCON0 |= FIFOSEL_WIN2;
        break;
    case DISP_WIN3:
        g_pDispConReg->VIDINTCON0 |= FIFOSEL_WIN3;
        break;
    case DISP_WIN4:
        g_pDispConReg->VIDINTCON0 |= FIFOSEL_WIN4;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_fifo_interrupt() : Unknown Window Number [%d]\n\r"), Win));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    switch(Level)
    {
    case DISP_FIFO_0_25:
        g_pDispConReg->VIDINTCON0 |= FIFOLEVEL_25;
        break;
    case DISP_FIFO_0_50:
        g_pDispConReg->VIDINTCON0 |= FIFOLEVEL_50;
        break;
    case DISP_FIFO_0_75:
        g_pDispConReg->VIDINTCON0 |= FIFOLEVEL_75;
        break;
    case DISP_FIFO_EMPTY:
        g_pDispConReg->VIDINTCON0 |= FIFOLEVEL_EMPTY;
        break;
    case DISP_FIFO_FULL:
        g_pDispConReg->VIDINTCON0 |= FIFOLEVEL_FULL;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_set_fifo_interrupt() : Unknown FIFO Level [%d]\n\r"), Level));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    DISP_MSG((_T("[DISP]--Disp_set_fifo_interrupt() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_enable_fifo_interrupt(void)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_enable_fifo_interrupt()\n\r")));

    g_pDispConReg->VIDINTCON0 |= (INTFIFOEN_ENABLE | INTEN_ENABLE);

    DISP_MSG((_T("[DISP]--Disp_enable_fifo_interrupt() : %d\n\r"), error));

    return error;
}

DISP_ERROR Disp_disable_fifo_interrupt(void)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_disable_fifo_interrupt()\n\r")));

    if (g_pDispConReg->VIDINTCON0 & INTFRMEN_ENABLE)
    {
        // If Frame Interrupt is enabled, Clear only FIFO Interrupt Enable Bit
        g_pDispConReg->VIDINTCON0 &= ~INTFIFOEN_ENABLE;
    }
    else
    {
        g_pDispConReg->VIDINTCON0 &= ~(INTFIFOEN_ENABLE | INTEN_ENABLE);
    }

    DISP_MSG((_T("[DISP]--Disp_disable_fifo_interrupt() : %d\n\r"), error));

    return error;
}

DISP_INTERRUPT Disp_clear_interrupt_pending(void)
{
    DISP_INTERRUPT Interrupt = DISP_INT_NONE;

    // Check Interrupt Source and Clear Pending
    if (g_pDispConReg->VIDINTCON1 & INTFIFO_PEND)
    {
        g_pDispConReg->VIDINTCON1 = INTFIFO_PEND;
        Interrupt = DISP_INT_FIFO;
    }
    else if (g_pDispConReg->VIDINTCON1 & INTFRM_PEND)
    {
        g_pDispConReg->VIDINTCON1 = INTFRM_PEND;
        Interrupt = DISP_INT_FRAME;
    }
    else if (g_pDispConReg->VIDINTCON1 & INTSYSIF_PEND)
    {
        g_pDispConReg->VIDINTCON1 = INTSYSIF_PEND;
        Interrupt = DISP_INT_I80;
    }

    return Interrupt;
}

unsigned int Disp_get_line_count(void)
{
    unsigned int uiLineCnt;

    DISP_MSG((_T("[DISP]++Disp_get_line_count()\n\r")));

    uiLineCnt = ((g_pDispConReg->VIDCON1)>>16)&0x7ff;

    DISP_MSG((_T("[DISP]--Disp_get_line_count() : Line Count = %d\n\r"), uiLineCnt));

    return uiLineCnt;
}

DISP_VERTICAL_STATUS Disp_get_vertical_status(void)
{
    unsigned int status;

    //DISP_MSG((_T("[DISP]++Disp_get_vertical_status()\n\r")));

    status = ((g_pDispConReg->VIDCON1)>>13)&0x3;

    //DISP_MSG((_T("[DISP]--Disp_get_vertical_status() : Status = %d\n\r"), status));

    return (DISP_VERTICAL_STATUS)status;
}

DISP_WINDOW Disp_get_win_num_from_win_mode(DISP_WINDOW_MODE Mode)
{
    DISP_WINDOW win;

    switch(Mode)
    {
    case DISP_WIN0_DMA:
    case DISP_WIN0_POST_RGB:
    case DISP_WIN0_POST_YUV:
        win = DISP_WIN0;
        break;
    case DISP_WIN1_DMA:
    case DISP_WIN1_TVSCALER_RGB:
    case DISP_WIN1_TVSCALER_YUV:
    case DISP_WIN1_CIPREVIEW_RGB:
    case DISP_WIN1_CIPREVIEW_YUV:
        win = DISP_WIN1;
        break;
    case DISP_WIN2_DMA:
    case DISP_WIN2_TVSCALER_RGB:
    case DISP_WIN2_TVSCALER_YUV:
    case DISP_WIN2_CICODEC_RGB:
    case DISP_WIN2_CICODEC_YUV:
        win = DISP_WIN2;
        break;
    case DISP_WIN3_DMA:
        win = DISP_WIN3;
        break;
    case DISP_WIN4_DMA:
        win = DISP_WIN4;
        break;
    default:
        win = -1;
        break;
    }

    return win;
}

static DISP_ERROR Disp_initialize_port_RGBIF(DISP_RGBIFOUT_MODE RGBOutMode)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_initialize_port_RGBIF(%d)\n\r"), RGBOutMode));

    // This code enable or disable VideoSignalLine mapped with GPIO
    // Open all signal, There are no 16BPP LCD panel, we can represent full color range
    // Even it's OS's limitation.
/*          
    switch(RGBOutMode)
    {
    case DISP_16BIT_RGB565_P:    // GPJ[7:3]->R[4:0], GPI[15:10]->G[5:0], GPI[7:3] -> B[4:0]
        g_pGPIOReg->GPIPUD &= ~(0xfff0ffc0);                                        // Pull up/down disable
        g_pGPIOReg->GPICON = (g_pGPIOReg->GPICON & ~(0xfff0ffc0)) | (0xaaa0aa80);    // G[5:0], B[4:0]
        g_pGPIOReg->GPJPUD &= ~(0xffc0);                                            // Pull up/down disable
        g_pGPIOReg->GPJCON = (g_pGPIOReg->GPJCON & ~(0xffc0)) | (0xaa80);            // R[4:0]
        break;
    case DISP_18BIT_RGB666_P:    // GPJ[7:2]->R[5:0], GPI[15:10]->G[5:0], GPI[7:2] -> B[5:0]
        g_pGPIOReg->GPIPUD &= ~(0xfff0fff0);                                        // Pull up/down disable
        g_pGPIOReg->GPICON = (g_pGPIOReg->GPICON & ~(0xfff0fff0)) | (0xaaa0aaa0);    // G[5:0], B[5:0]
        g_pGPIOReg->GPJPUD &= ~(0xfff0);                                            // Pull up/down disable
        g_pGPIOReg->GPJCON = (g_pGPIOReg->GPJCON & ~(0xfff0)) | (0xaaa0);            // R[5:0]
        break;
    case DISP_24BIT_RGB888_P:    // GPI[15:0] -> Data[15:0], GPJ[7:0]->Data[23:16]
    */
        g_pGPIOReg->GPIPUD = 0;                                            // Pull up/down disable
        g_pGPIOReg->GPICON = 0xaaaaaaaa;                                    // Data[15:0]
        g_pGPIOReg->GPJPUD &= ~0xffff;                                        // Pull up/down disable
        g_pGPIOReg->GPJCON = (g_pGPIOReg->GPJCON & ~0xffff) | 0xaaaa;        // Data[23:16]
        /*
        break;
    case DISP_16BIT_RGB565_S:    // GPJ[7:2] -> Data[5:0]
    case DISP_18BIT_RGB666_S:    // GPJ[7:2] -> Data[5:0]
        g_pGPIOReg->GPJPUD &= ~(0xfff0);                                    // Pull up/down disable
        g_pGPIOReg->GPJCON = (g_pGPIOReg->GPJCON & ~(0xfff0)) | (0xaaa0);    // Data[5:0]
        break;
    case DISP_24BIT_RGB888_S:    // GPJ[7:0] -> Data[7:0]

        g_pGPIOReg->GPJPUD &= ~0xffff;                                        // Pull up/down disable
        g_pGPIOReg->GPJCON = (g_pGPIOReg->GPJCON & ~(0xffff)) | 0xaaaa;    // Data[7:0]
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_initialize_port_RGBIF() : Unsupported RGB I/F Output Mode [%d]\n\r"), RGBOutMode));
        error = DISP_ERROR_NOT_INITIALIZED;
        break;
    }
        */    

    // Control Signal (HSync, VSync, VDEN, VClk)
    g_pGPIOReg->GPJPUD &= ~(0xf<<16);    // Pull up/down disable
    g_pGPIOReg->GPJCON = (g_pGPIOReg->GPJCON & ~(0xff<<16)) | (0xaa<<16);    // HSync, VSync, VDEN, VCLK

    // MSM I/F Bypass Mode Disable
    g_pMSMIFReg->MIFPCON &= ~(0x1<<3);    // SEL_BYPASS -> Normal Mode

    // LCD Interface Type
    g_pGPIOReg->SPCON = (g_pGPIOReg->SPCON & ~(0x3)) | (0x1); // Select RGBIF style PIN
	g_pGPIOReg->SPCON &=~(3<<24); // better pictures on displays connected with LONG cables....
    DISP_MSG((_T("[DISP]--Disp_initialize_port_RGBIF() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_initialize_RGBIF(void)
{
    DISP_ERROR error = DISP_SUCCESS;
    unsigned int VCLKDivider;

    DISP_MSG((_T("[DISP]++Disp_initialize_RGBIF()\n\r")));

    if (Disp_get_vclk_direction_divider(g_DevInfoRGB.VCLK_Source, &g_DevInfoRGB.VCLK_Direction, &VCLKDivider))
    {
		DISP_MSG((_T("[DISP] VCLKDivider (%d)\n\r"),CLKVAL_F(VCLKDivider)));
/*
        g_pDispConReg->VIDCON0 = INTERLACE | VIDOUT_TVENC | g_DevInfoRGB.PNR_Mode | CLKVALUP_ALWAYS |
                                CLKVAL_F(VCLKDivider) | VCLK_NORMAL | g_DevInfoRGB.VCLK_Direction |
                                g_DevInfoRGB.VCLK_Source | ENVID_DISABLE | ENVID_F_DISABLE;
*/    
        g_pDispConReg->VIDCON0 = PROGRESSIVE | VIDOUT_RGBIF | g_DevInfoRGB.PNR_Mode | CLKVALUP_ALWAYS |
                                CLKVAL_F(VCLKDivider) | VCLK_NORMAL | g_DevInfoRGB.VCLK_Direction |
                                g_DevInfoRGB.VCLK_Source | ENVID_DISABLE | ENVID_F_DISABLE;

		g_pDispConReg->VIDCON1 = g_DevInfoRGB.VCLK_Polarity | g_DevInfoRGB.HSYNC_Polarity |
                                g_DevInfoRGB.VSYNC_Polarity | g_DevInfoRGB.VDEN_Polarity;

/*
    g_pDispConReg->VIDCON2 = EN601 | TVIF_FMT_YUV422;
    // Default Field Mode, polarity all normal(HREF, VSYNC, HSYNC, FIELD[Even=High], CLK)
    // User can adjust the VSYNC delay clock with changing DLYVSYNC[23:16], SELVSYNC[24]
    g_pDispConReg->ITUIFCON0 = 0;
    g_pGPIOReg->SPCON = (g_pGPIOReg->SPCON & ~(0x3<<0))|(2<<0);     // rechange to ITU601/656 style
*/
//        g_pDispConReg->VIDCON2 = TVIF_FMT_YUV444;    // Should be this value

        g_pDispConReg->VIDTCON0 = VBPDE(1) | VBPD(g_DevInfoRGB.VBPD_Value) |
                                VFPD(g_DevInfoRGB.VFPD_Value) | VSPW(g_DevInfoRGB.VSPW_Value);

        g_pDispConReg->VIDTCON1 = VFPDE(1) | HBPD(g_DevInfoRGB.HBPD_Value) |
                                HFPD(g_DevInfoRGB.HFPD_Value) | HSPW(g_DevInfoRGB.HSPW_Value);

        g_pDispConReg->VIDTCON2 = LINEVAL(g_DevInfoRGB.uiHeight) | HOZVAL(g_DevInfoRGB.uiWidth);
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] Disp_initialize_RGBIF() : Clock Source Decision Failed\n\r")));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
    }

    DISP_MSG((_T("[DISP]--Disp_initialize_RGBIF() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_initialize_RGBIF_withTVEnc(void)
{
    DISP_ERROR error = DISP_SUCCESS;
    unsigned int VCLKDivider;

    DISP_MSG((_T("[DISP]++Disp_initialize_RGBIF_withTVEnc()\n\r")));

    if (Disp_get_vclk_direction_divider_forTVEnc(g_DevInfoRGB.VCLK_Source, &g_DevInfoRGB.VCLK_Direction, &VCLKDivider))
    {
        g_pDispConReg->VIDCON0 = PROGRESSIVE | VIDOUT_TV_RGBIF | g_DevInfoRGB.PNR_Mode | CLKVALUP_ALWAYS |
                                CLKVAL_F(VCLKDivider) | VCLK_NORMAL | g_DevInfoRGB.VCLK_Direction |
                                g_DevInfoRGB.VCLK_Source | ENVID_DISABLE | ENVID_F_DISABLE;

        g_pDispConReg->VIDCON1 = g_DevInfoRGB.VCLK_Polarity | g_DevInfoRGB.HSYNC_Polarity |
                                g_DevInfoRGB.VSYNC_Polarity | g_DevInfoRGB.VDEN_Polarity;

        g_pDispConReg->VIDCON2 = TVIF_FMT_YUV444;    // Should be this value

        g_pDispConReg->VIDTCON0 = VBPDE(1) | VBPD(g_DevInfoRGB.VBPD_Value) |
                                VFPD(g_DevInfoRGB.VFPD_Value) | VSPW(g_DevInfoRGB.VSPW_Value);

        g_pDispConReg->VIDTCON1 = VFPDE(1) | HBPD(g_DevInfoRGB.HBPD_Value) |
                                HFPD(g_DevInfoRGB.HFPD_Value) | HSPW(g_DevInfoRGB.HSPW_Value);

        g_pDispConReg->VIDTCON2 = LINEVAL(g_DevInfoRGB.uiHeight) | HOZVAL(g_DevInfoRGB.uiWidth);
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] Disp_initialize_RGBIF_withTVEnc() : Clock Source Decision Failed\n\r")));
        error = DISP_ERROR_ILLEGAL_PARAMETER;
    }

    DISP_MSG((_T("[DISP]--Disp_initialize_RGBIF_withTVEnc() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_initialize_TVEnc(void)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_initialize_TVEnc()\n\r")));

#if 0  // TV Encoder FIFO underrun at WVGA(800x480) size Frame Buffer
    g_pDispConReg->VIDCON0 = PROGRESSIVE | VIDOUT_TVENC | g_DevInfoTV.PNR_Mode | CLKVALUP_ALWAYS |
                            CLKVAL_F(2) | VCLK_NORMAL | CLKDIR_DIRECT |
                            CLKSEL_F_EXT27M | ENVID_DISABLE | ENVID_F_DISABLE;
#else
    g_pDispConReg->VIDCON0 = PROGRESSIVE | VIDOUT_TVENC | g_DevInfoTV.PNR_Mode | CLKVALUP_ALWAYS |
                            CLKVAL_F(4) | VCLK_NORMAL | CLKDIR_DIVIDED |
                            CLKSEL_F_HCLK | ENVID_DISABLE | ENVID_F_DISABLE;
#endif

    g_pDispConReg->VIDCON1 = g_DevInfoTV.VCLK_Polarity | g_DevInfoTV.HSYNC_Polarity |
                            g_DevInfoTV.VSYNC_Polarity | g_DevInfoTV.VDEN_Polarity;

    g_pDispConReg->VIDCON2 = TVIF_FMT_YUV444;    // Should be this value

    g_pDispConReg->VIDTCON0 = VBPDE(1) | VBPD(g_DevInfoTV.VBPD_Value) |
                            VFPD(g_DevInfoTV.VFPD_Value) | VSPW(g_DevInfoTV.VSPW_Value);

    g_pDispConReg->VIDTCON1 = VFPDE(1) | HBPD(g_DevInfoTV.HBPD_Value) |
                            HFPD(g_DevInfoTV.HFPD_Value) | HSPW(g_DevInfoTV.HSPW_Value);

    g_pDispConReg->VIDTCON2 = LINEVAL(g_DevInfoTV.uiHeight) | HOZVAL(g_DevInfoTV.uiWidth);

    DISP_MSG((_T("[DISP]--Disp_initialize_TVEnc() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_window0_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_window0_initialize(%d, %d, %d, %d, %d, %d)\n\r"), Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY));

    switch(Mode)
    {
    case DISP_WIN0_DMA:
        g_Win0Config.LocalPathEnable = LOCAL_PATH_DISABLE;
        //g_Win0Config.LocaPathSourceFormat;    // Don't care when Local Path is disabled
        break;
    case DISP_WIN0_POST_RGB:
        g_Win0Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win0Config.LocaPathSourceFormat = LOCAL_IN_RGB888;
        break;
    case DISP_WIN0_POST_YUV:
        g_Win0Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win0Config.LocaPathSourceFormat = LOCAL_IN_YUV444;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window0_initialize() : Unsupported Window Mode [%d]\n\r"), Mode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win0Config.BitSwapping = BITSWP_DISABLE;
    g_Win0Config.ByteSwapping = BYTSWP_DISABLE;
    g_Win0Config.HalfWordSwapping = HAWSWP_DISABLE;

    switch(BPPMode)
    {
    //case DISP_1BPP:
    //case DISP_2BPP:
    //case DISP_4BPP:
    //case DISP_8BPP_PAL:
    //case DISP_16BPP_I555:
    case DISP_16BPP_565:
        g_Win0Config.uiPageWidth = uiWidth*2;    // 2 byte per pixel
        g_Win0Config.HalfWordSwapping = HAWSWP_ENABLE;    // 16BPP need Halfword Swapping
        break;
    case DISP_18BPP_666:
    case DISP_24BPP_888:
        g_Win0Config.uiPageWidth = uiWidth*4;    // 4 byte per pixel
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window0_initialize() : Unsupported BPP Mode [%d]\n\r"), BPPMode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win0Config.BPP_Mode = BPPMode;
    g_Win0Config.BufferSelect = BUFSEL_BUF0;
    g_Win0Config.BufferAutoControl = BUFAUTO_DISABLE;
    g_Win0Config.uiWidth = uiWidth;
    g_Win0Config.uiHeight = uiHeight;
    g_Win0Config.uiOffsetX = uiOffsetX;
    g_Win0Config.uiOffsetY = uiOffsetY;

    if (g_Win0Config.LocalPathEnable == LOCAL_PATH_ENABLE)
    {
        g_Win0Config.BurstLength = BURSTLEN_4WORD;    // 4 Words Burst
    }
    else if ((g_Win0Config.uiPageWidth%64) == 0)        // 16 words burst case
    {
        g_Win0Config.BurstLength = BURSTLEN_16WORD;
    }
    else if ((g_Win0Config.uiPageWidth%32) == 0)    // 8 words burst case
    {
        g_Win0Config.BurstLength = BURSTLEN_8WORD;
    }
    else if ((g_Win0Config.uiPageWidth%16) == 0)    // 4 words burst case
    {
        g_Win0Config.BurstLength = BURSTLEN_4WORD;
		
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] Disp_window0_initialize() : uiPageWidth[%d] is not Aligned with Minimum Burst Size (4 Words)\n\r"), g_Win0Config.uiPageWidth));
        return DISP_ERROR_ILLEGAL_PARAMETER;
    }
	DISP_MSG((_T("[DISP] Disp_window0_burst width : %d\n\r"), g_Win0Config.BurstLength));

    g_pDispConReg->WINCON0 = CSC_WIDE_RANGE | //g_Win0Config.LocalPathEnable |
                            g_Win0Config.BufferSelect | g_Win0Config.BufferAutoControl | g_Win0Config.BitSwapping |
                            g_Win0Config.ByteSwapping | g_Win0Config.HalfWordSwapping | g_Win0Config.LocaPathSourceFormat |
                            g_Win0Config.BurstLength | BPPMODE_F(g_Win0Config.BPP_Mode);

    g_pDispConReg->VIDOSD0A = OSD_LEFTTOPX_F(g_Win0Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win0Config.uiOffsetY);

    g_pDispConReg->VIDOSD0B = OSD_RIGHTBOTX_F(g_Win0Config.uiWidth+g_Win0Config.uiOffsetX-1) |
                                OSD_RIGHTBOTY_F(g_Win0Config.uiHeight+g_Win0Config.uiOffsetY-1);

    g_pDispConReg->VIDOSD0C = OSD_SIZE(g_Win0Config.uiWidth*g_Win0Config.uiHeight);

    DISP_MSG((_T("[DISP]--Disp_window0_initialize() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_window1_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_window1_initialize(%d, %d, %d, %d, %d, %d)\n\r"), Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY));

    switch(Mode)
    {
    case DISP_WIN1_DMA:
        g_Win1Config.LocalPathEnable = LOCAL_PATH_DISABLE;
        //g_Win1Config.LocalPathSelect;            // Don't care when Local Path is disabled
        //g_Win1Config.LocaPathSourceFormat;    // Don't care when Local Path is disabled
        break;
    case DISP_WIN1_TVSCALER_RGB:
        g_Win1Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win1Config.LocalPathSelect = LOCALSEL_TVSCALER;
        g_Win1Config.LocaPathSourceFormat = LOCAL_IN_RGB888;
        break;
    case DISP_WIN1_TVSCALER_YUV:
        g_Win1Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win1Config.LocalPathSelect = LOCALSEL_TVSCALER;
        g_Win1Config.LocaPathSourceFormat = LOCAL_IN_YUV444;
        break;
    case DISP_WIN1_CIPREVIEW_RGB:
        g_Win1Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win1Config.LocalPathSelect = LOCALSEL_CIPREVIEW;
        g_Win1Config.LocaPathSourceFormat = LOCAL_IN_RGB888;
        break;
    case DISP_WIN1_CIPREVIEW_YUV:
        g_Win1Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win1Config.LocalPathSelect = LOCALSEL_CIPREVIEW;
        g_Win1Config.LocaPathSourceFormat = LOCAL_IN_YUV444;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window1_initialize() : Unsupported Window Mode [%d]\n\r"), Mode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win1Config.BitSwapping = BITSWP_DISABLE;
    g_Win1Config.ByteSwapping = BYTSWP_DISABLE;
    g_Win1Config.HalfWordSwapping = HAWSWP_DISABLE;

    switch(BPPMode)
    {
    //case DISP_1BPP:
    //case DISP_2BPP:
    //case DISP_4BPP:
    //case DISP_8BPP_PAL:
    //case DISP_8BPP_NOPAL:
    //case DISP_16BPP_A555:
    //case DISP_16BPP_I555:
    //case DISP_18BPP_A665:
    //case DISP_19BPP_A666:
    //case DISP_24BPP_A887:
    //case DISP_25BPP_A888:
    case DISP_16BPP_565:
        g_Win1Config.uiPageWidth = uiWidth*2;    // 2 byte per pixel
        g_Win1Config.HalfWordSwapping = HAWSWP_ENABLE;    // 16BPP need Halfword Swapping
        break;
    case DISP_18BPP_666:
    case DISP_24BPP_888:
        g_Win1Config.uiPageWidth = uiWidth*4;    // 4 byte per pixel
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window1_initialize() : Unsupported BPP Mode [%d]\n\r"), BPPMode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win1Config.BPP_Mode = BPPMode;
    g_Win1Config.BufferSelect = BUFSEL_BUF0;
    g_Win1Config.BufferAutoControl = BUFAUTO_DISABLE;
    g_Win1Config.uiWidth = uiWidth;
    g_Win1Config.uiHeight = uiHeight;
    g_Win1Config.uiOffsetX = uiOffsetX;
    g_Win1Config.uiOffsetY = uiOffsetY;

    if (g_Win1Config.LocalPathEnable == LOCAL_PATH_ENABLE)
    {
        g_Win1Config.BurstLength = BURSTLEN_4WORD;    // 4 Words Burst
    }
    else if ((g_Win1Config.uiPageWidth%64) == 0)        // 16 words burst case
    {
        g_Win1Config.BurstLength = BURSTLEN_16WORD;
    }
    else if ((g_Win1Config.uiPageWidth%32) == 0)    // 8 words burst case
    {
        g_Win1Config.BurstLength = BURSTLEN_8WORD;
    }
    else if ((g_Win1Config.uiPageWidth%16) == 0)    // 4 words burst case
    {
        g_Win1Config.BurstLength = BURSTLEN_4WORD;
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] Disp_window1_initialize() : uiPageWidth is not Word Aligned [%d]\n\r"), g_Win0Config.uiPageWidth));
        return DISP_ERROR_ILLEGAL_PARAMETER;
    }

    g_pDispConReg->WINCON1 = CSC_WIDE_RANGE | g_Win1Config.LocalPathSelect | //g_Win1Config.LocalPathEnable |
                            g_Win1Config.BufferSelect | g_Win1Config.BufferAutoControl | g_Win1Config.BitSwapping |
                            g_Win1Config.ByteSwapping | g_Win1Config.HalfWordSwapping | g_Win1Config.LocaPathSourceFormat |
                            g_Win1Config.BurstLength | BLEND_PER_PLANE | BPPMODE_F(g_Win1Config.BPP_Mode) |
                            ALPHASEL_ALPHA0;

    g_pDispConReg->VIDOSD1A = OSD_LEFTTOPX_F(g_Win1Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win1Config.uiOffsetY);

    g_pDispConReg->VIDOSD1B = OSD_RIGHTBOTX_F(g_Win1Config.uiWidth+g_Win1Config.uiOffsetX-1) |
                                OSD_RIGHTBOTY_F(g_Win1Config.uiHeight+g_Win1Config.uiOffsetY-1);

    g_pDispConReg->VIDOSD1C = 0x0;

    g_pDispConReg->VIDOSD1D = OSD_SIZE(g_Win1Config.uiWidth*g_Win1Config.uiHeight);

    DISP_MSG((_T("[DISP]--Disp_window1_initialize() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_window2_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_window2_initialize(%d, %d, %d, %d, %d, %d)\n\r"), Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY));

    switch(Mode)
    {
    case DISP_WIN2_DMA:
        g_Win2Config.LocalPathEnable = LOCAL_PATH_DISABLE;
        //g_Win2Config.LocalPathSelect;            // Don't care when Local Path is disabled
        //g_Win2Config.LocaPathSourceFormat;    // Don't care when Local Path is disabled
        break;
    case DISP_WIN2_TVSCALER_RGB:
        g_Win2Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win2Config.LocalPathSelect = LOCALSEL_TVSCALER;
        g_Win2Config.LocaPathSourceFormat = LOCAL_IN_RGB888;
        break;
    case DISP_WIN2_TVSCALER_YUV:
        g_Win2Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win2Config.LocalPathSelect = LOCALSEL_TVSCALER;
        g_Win2Config.LocaPathSourceFormat = LOCAL_IN_YUV444;
        break;
    case DISP_WIN2_CICODEC_RGB:
        g_Win2Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win2Config.LocalPathSelect = LOCALSEL_CIPREVIEW;
        g_Win2Config.LocaPathSourceFormat = LOCAL_IN_RGB888;
        break;
    case DISP_WIN2_CICODEC_YUV:
        g_Win2Config.LocalPathEnable = LOCAL_PATH_ENABLE;
        g_Win2Config.LocalPathSelect = LOCALSEL_CIPREVIEW;
        g_Win2Config.LocaPathSourceFormat = LOCAL_IN_YUV444;
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window2_initialize() : Unsupported Window Mode [%d]\n\r"), Mode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win2Config.BitSwapping = BITSWP_DISABLE;
    g_Win2Config.ByteSwapping = BYTSWP_DISABLE;
    g_Win2Config.HalfWordSwapping = HAWSWP_DISABLE;

    switch(BPPMode)
    {
    //case DISP_1BPP:
    //case DISP_2BPP:
    //case DISP_4BPP:
    //case DISP_8BPP_NOPAL:
    //case DISP_16BPP_A555:
    //case DISP_16BPP_I555:
    //case DISP_18BPP_A665:
    //case DISP_19BPP_A666:
    //case DISP_24BPP_A887:
    //case DISP_25BPP_A888:
    case DISP_16BPP_565:
        g_Win2Config.uiPageWidth = uiWidth*2;    // 2 byte per pixel
        g_Win2Config.HalfWordSwapping = HAWSWP_ENABLE;    // 16BPP need Halfword Swapping
        break;
    case DISP_18BPP_666:
    case DISP_24BPP_888:
        g_Win2Config.uiPageWidth = uiWidth*4;    // 4 byte per pixel
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window2_initialize() : Unsupported BPP Mode [%d]\n\r"), BPPMode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win2Config.BPP_Mode = BPPMode;
    g_Win2Config.BufferSelect = BUFSEL_BUF0;
    g_Win2Config.BufferAutoControl = BUFAUTO_DISABLE;
    g_Win2Config.uiWidth = uiWidth;
    g_Win2Config.uiHeight = uiHeight;
    g_Win2Config.uiOffsetX = uiOffsetX;
    g_Win2Config.uiOffsetY = uiOffsetY;

    if (g_Win2Config.LocalPathEnable == LOCAL_PATH_ENABLE)
    {
        g_Win2Config.BurstLength = BURSTLEN_4WORD;    // 4 Words Burst
    }
    else if ((g_Win2Config.uiPageWidth%64) == 0)        // 16 words burst case
    {
        g_Win2Config.BurstLength = BURSTLEN_16WORD;
    }
    else if ((g_Win2Config.uiPageWidth%32) == 0)    // 8 words burst case
    {
        g_Win2Config.BurstLength = BURSTLEN_8WORD;
    }
    else if ((g_Win2Config.uiPageWidth%16) == 0)    // 4 words burst case
    {
        g_Win2Config.BurstLength = BURSTLEN_4WORD;
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] Disp_window2_initialize() : uiPageWidth is not Word Aligned [%d]\n\r"), g_Win0Config.uiPageWidth));
        return DISP_ERROR_ILLEGAL_PARAMETER;
    }

    g_pDispConReg->WINCON2 = CSC_WIDE_RANGE | g_Win2Config.LocalPathSelect | //g_Win2Config.LocalPathEnable |
                            g_Win2Config.BufferSelect | g_Win2Config.BufferAutoControl | g_Win2Config.BitSwapping |
                            g_Win2Config.ByteSwapping | g_Win2Config.HalfWordSwapping | g_Win2Config.LocaPathSourceFormat |
                            g_Win2Config.BurstLength | BLEND_PER_PLANE | BPPMODE_F(g_Win2Config.BPP_Mode) |
                            ALPHASEL_ALPHA0;

    g_pDispConReg->VIDOSD2A = OSD_LEFTTOPX_F(g_Win2Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win2Config.uiOffsetY);

    g_pDispConReg->VIDOSD2B = OSD_RIGHTBOTX_F(g_Win2Config.uiWidth+g_Win2Config.uiOffsetX-1) |
                                OSD_RIGHTBOTY_F(g_Win2Config.uiHeight+g_Win2Config.uiOffsetY-1);

    g_pDispConReg->VIDOSD2C = 0x0;

    g_pDispConReg->VIDOSD2D = OSD_SIZE(g_Win2Config.uiWidth*g_Win2Config.uiHeight);

    DISP_MSG((_T("[DISP]--Disp_window2_initialize() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_window3_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_window3_initialize(%d, %d, %d, %d, %d, %d)\n\r"), Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY));

    g_Win3Config.BitSwapping = BITSWP_DISABLE;
    g_Win3Config.ByteSwapping = BYTSWP_DISABLE;
    g_Win3Config.HalfWordSwapping = HAWSWP_DISABLE;

    switch(BPPMode)
    {
    //case DISP_1BPP:
    //case DISP_2BPP:
    //case DISP_4BPP:
    //case DISP_16BPP_A555:
    //case DISP_16BPP_I555:
    //case DISP_18BPP_A665:
    //case DISP_19BPP_A666:
    //case DISP_24BPP_A887:
    //case DISP_25BPP_A888:
    case DISP_16BPP_565:
        g_Win3Config.uiPageWidth = uiWidth*2;    // 2 byte per pixel
        g_Win3Config.HalfWordSwapping = HAWSWP_ENABLE;    // 16BPP need Halfword Swapping
        break;
    case DISP_18BPP_666:
    case DISP_24BPP_888:
        g_Win3Config.uiPageWidth = uiWidth*4;    // 4 byte per pixel
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window3_initialize() : Unsupported BPP Mode [%d]\n\r"), BPPMode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win3Config.BPP_Mode = BPPMode;
    g_Win3Config.uiWidth = uiWidth;
    g_Win3Config.uiHeight = uiHeight;
    g_Win3Config.uiOffsetX = uiOffsetX;
    g_Win3Config.uiOffsetY = uiOffsetY;

    if (g_Win3Config.uiPageWidth%4)
    {
        DISP_ERR((_T("[DISP:ERR] Disp_window3_initialize() : uiPageWidth is not Word Aligned [%d]\n\r"), g_Win0Config.uiPageWidth));
        return DISP_ERROR_ILLEGAL_PARAMETER;
    }
    else if (g_Win3Config.uiPageWidth >= 16*4)
    {
        g_Win3Config.BurstLength = BURSTLEN_16WORD;
    }
    else if (g_Win3Config.uiPageWidth >= 8*4)
    {
        g_Win3Config.BurstLength = BURSTLEN_8WORD;
    }
    else
    {
        g_Win3Config.BurstLength = BURSTLEN_4WORD;
    }

    g_pDispConReg->WINCON3 = g_Win3Config.BitSwapping | g_Win3Config.ByteSwapping | g_Win3Config.HalfWordSwapping |
                            g_Win3Config.BurstLength | BLEND_PER_PLANE | BPPMODE_F(g_Win3Config.BPP_Mode) |
                            ALPHASEL_ALPHA0;

    g_pDispConReg->VIDOSD3A = OSD_LEFTTOPX_F(g_Win3Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win3Config.uiOffsetY);

    g_pDispConReg->VIDOSD3B = OSD_RIGHTBOTX_F(g_Win3Config.uiWidth+g_Win3Config.uiOffsetX-1) |
                                OSD_RIGHTBOTY_F(g_Win3Config.uiHeight+g_Win3Config.uiOffsetY-1);

    g_pDispConReg->VIDOSD3C = 0x0;

    DISP_MSG((_T("[DISP]--Disp_window3_initialize() : %d\n\r"), error));

    return error;
}

static DISP_ERROR Disp_window4_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY)
{
    DISP_ERROR error = DISP_SUCCESS;

    DISP_MSG((_T("[DISP]++Disp_window4_initialize(%d, %d, %d, %d, %d, %d)\n\r"), Mode, BPPMode, uiWidth, uiHeight, uiOffsetX, uiOffsetY));

    g_Win4Config.BitSwapping = BITSWP_DISABLE;
    g_Win4Config.ByteSwapping = BYTSWP_DISABLE;
    g_Win4Config.HalfWordSwapping = HAWSWP_DISABLE;

    switch(BPPMode)
    {
    //case DISP_1BPP:
    //case DISP_2BPP:
    //case DISP_16BPP_A555:
    //case DISP_16BPP_I555:
    //case DISP_18BPP_A665:
    //case DISP_19BPP_A666:
    //case DISP_24BPP_A887:
    //case DISP_25BPP_A888:
    case DISP_16BPP_565:
        g_Win4Config.uiPageWidth = uiWidth*2;    // 2 byte per pixel
        g_Win4Config.HalfWordSwapping = HAWSWP_ENABLE;    // 16BPP need Halfword Swapping
        break;
    case DISP_18BPP_666:
    case DISP_24BPP_888:
        g_Win4Config.uiPageWidth = uiWidth*4;    // 4 byte per pixel
        break;
    default:
        DISP_ERR((_T("[DISP:ERR] Disp_window4_initialize() : Unsupported BPP Mode [%d]\n\r"), BPPMode));
        return DISP_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_Win4Config.BPP_Mode = BPPMode;
    g_Win4Config.uiWidth = uiWidth;
    g_Win4Config.uiHeight = uiHeight;
    g_Win4Config.uiOffsetX = uiOffsetX;
    g_Win4Config.uiOffsetY = uiOffsetY;

    if (g_Win4Config.uiPageWidth%4)
    {
        DISP_ERR((_T("[DISP:ERR] Disp_window4_initialize() : uiPageWidth is not Word Aligned [%d]\n\r"), g_Win0Config.uiPageWidth));
        return DISP_ERROR_ILLEGAL_PARAMETER;
    }
    else if (g_Win4Config.uiPageWidth >= 16*4)
    {
        g_Win4Config.BurstLength = BURSTLEN_16WORD;
    }
    else if (g_Win4Config.uiPageWidth >= 8*4)
    {
        g_Win4Config.BurstLength = BURSTLEN_8WORD;
    }
    else
    {
        g_Win4Config.BurstLength = BURSTLEN_4WORD;
    }

    g_pDispConReg->WINCON4 = g_Win4Config.BitSwapping | g_Win4Config.ByteSwapping | g_Win4Config.HalfWordSwapping |
                            g_Win4Config.BurstLength | BLEND_PER_PLANE | BPPMODE_F(g_Win4Config.BPP_Mode) |
                            ALPHASEL_ALPHA0;

    g_pDispConReg->VIDOSD4A = OSD_LEFTTOPX_F(g_Win4Config.uiOffsetX) | OSD_LEFTTOPY_F(g_Win4Config.uiOffsetY);

    g_pDispConReg->VIDOSD4B = OSD_RIGHTBOTX_F(g_Win4Config.uiWidth+g_Win4Config.uiOffsetX-1) |
                                OSD_RIGHTBOTY_F(g_Win4Config.uiHeight+g_Win4Config.uiOffsetY-1);

    g_pDispConReg->VIDOSD4C = 0x0;

    DISP_MSG((_T("[DISP]--Disp_window4_initialize() : %d\n\r"), error));

    return error;
}

static BOOL Disp_get_vclk_direction_divider(unsigned int CLKSrc, unsigned int *ClkDir, unsigned int *ClkDiv)
{
    DWORD dwVCLKSrc = 0;
    DWORD dwHozTime = 0;
    DWORD dwVerTime = 0;
    DWORD dwVCLK = 0;
    DWORD dwDiv = 0;

    DISP_MSG((_T("[DISP]++Disp_get_vclk_direction_divider(%d)\n\r"), CLKSrc));

	LDI_fill_output_device_information(&g_DevInfoRGB);

    if (CLKSrc == CLKSEL_F_HCLK)
    {
        dwVCLKSrc = S3C6410_HCLK;
        DISP_INF((_T("[DISP:INF] VCLK Source = HCLK (%d Hz)\n\r"), dwVCLKSrc));
    }
    else if (CLKSrc == CLKSEL_F_LCDCLK)
    {
        dwVCLKSrc = S3C6410_DoutMPLL;    // LCD_CLK is set to DoutMPLL by CLK_SRC in SysCon
        DISP_INF((_T("[DISP:INF] VCLK Source = LCDCLK (%d Hz)\n\r"), dwVCLKSrc));
    }
    else if (CLKSrc == CLKSEL_F_EXT27M)
    {
        dwVCLKSrc = 27000000;    // 27MHz
        DISP_INF((_T("[DISP:INF] VCLK Source = EXT27M (%d Hz)\n\r"), dwVCLKSrc));
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] --Disp_get_vclk_direction_divider() : Unknown CLKSrc = %d\n\r"), CLKSrc));
        return FALSE;
    }
	DISP_ERR((_T("[DISP:ERR] b4 switch\n\r")));
    switch(g_DevInfoRGB.RGBOutMode)
    {
        case DISP_16BIT_RGB565_P:
        case DISP_18BIT_RGB666_P:
        case DISP_24BIT_RGB888_P:
            dwHozTime = g_DevInfoRGB.uiWidth+g_DevInfoRGB.HBPD_Value+g_DevInfoRGB.HFPD_Value+g_DevInfoRGB.HSPW_Value;
            break;
        case DISP_16BIT_RGB565_S:
        case DISP_18BIT_RGB666_S:
        case DISP_24BIT_RGB888_S:
            dwHozTime = g_DevInfoRGB.uiWidth*3+g_DevInfoRGB.HBPD_Value+g_DevInfoRGB.HFPD_Value+g_DevInfoRGB.HSPW_Value;
            break;
    }
	DISP_INF((_T("[DISP:ERR]  HozTime : (%d)\n\r"), dwHozTime));
    dwVerTime = g_DevInfoRGB.uiHeight+g_DevInfoRGB.VBPD_Value+g_DevInfoRGB.VFPD_Value+g_DevInfoRGB.VSPW_Value;
	DISP_INF((_T("[DISP:ERR]  vertTime : (%d)\n\r"), dwVerTime));
    dwVCLK = dwHozTime*dwVerTime*g_DevInfoRGB.Frame_Rate;
	DISP_INF((_T("[DISP:ERR]  VCLK : (%d)\n\r"), dwVCLK));
	dwDiv = dwVCLKSrc/((dwVCLK==0)?1:dwVCLK);
	DISP_INF((_T("[DISP:ERR]  Div : (%d)\n\r"), dwDiv));

    DISP_INF((_T("[DISP:INF] VCLKSrc = %d, VCLK = %d, Div = %d\n\r"), dwVCLKSrc, dwVCLK, dwDiv));

    if (dwDiv < 1)
    {
        DISP_ERR((_T("[DISP:ERR] --Disp_get_vclk_direction_divider() : VCLK Source is Too Slow\n\r")));
        return FALSE;
    }
    else     if (dwDiv == 1)
    {
        // No Divide, Direct Clock
        *ClkDir = CLKDIR_DIRECT;
        *ClkDiv = 1;
        DISP_INF((_T("[DISP:INF] VCLK Direction = Direct, VCLK = %d Hz\n\r"), dwVCLKSrc));
    }
    else
    {
        // Divide by more than 1, Divided Clock
        *ClkDir = CLKDIR_DIVIDED;
        *ClkDiv = dwDiv;
        DISP_INF((_T("[DISP:INF] VCLK Direction = Divided, Divider = %d, VCLK = %d Hz\n\r"), *ClkDiv,  (unsigned int)((double)dwVCLKSrc/(double)(*ClkDiv))));
    }

    DISP_MSG((_T("[DISP] --Disp_get_vclk_direction_divider()\n\r")));

    return TRUE;
}

void Disp_VSync_Disable(void)
{
    g_pDispConReg->VIDINTCON0= g_pDispConReg->VIDINTCON0 & (0xFFFF7FFF);
}


void Disp_Check_Vsync_Value(void)
{
    if(Disp_get_vertical_status()==DISP_V_VSYNC)
    {
        g_flipCompleted=TRUE;
        Disp_VSync_Disable();
    }
}

void Disp_VSync_Enable(void)
{
    g_flipCompleted=FALSE;
    g_pDispConReg->VIDINTCON0= g_pDispConReg->VIDINTCON0 & (0xFFFEFFFF);
    g_pDispConReg->VIDINTCON0= g_pDispConReg->VIDINTCON0 | (0x00008000);
}



BOOL Disp_GetFlipStatus(void)
{
    return(g_flipCompleted);
}


static BOOL Disp_get_vclk_direction_divider_forTVEnc(unsigned int CLKSrc, unsigned int *ClkDir, unsigned int *ClkDiv)
{
    BOOL bRet;
    unsigned int uiVclkSrc;

    DISP_ERR((_T("[DISP]++Disp_get_vclk_direction_divider_forTVEnc(%d)\n\r"), CLKSrc));

    bRet = Disp_get_vclk_direction_divider(CLKSrc, ClkDir, ClkDiv);

    if (CLKSrc == CLKSEL_F_HCLK)
    {
        uiVclkSrc = S3C6410_HCLK;
    }
    else if (CLKSrc == CLKSEL_F_LCDCLK)
    {
        uiVclkSrc = S3C6410_DoutMPLL;
    }
    else if (CLKSrc == CLKSEL_F_EXT27M)
    {
        uiVclkSrc = 27000000;    // 27MHz
    }
    else
    {
        DISP_ERR((_T("[DISP:ERR] --Disp_get_vclk_direction_divider_forTVEnc() : Unknown CLKSrc = %d\n\r"), CLKSrc));
        return FALSE;
    }

    if ((*ClkDir) == CLKDIR_DIVIDED)
    {
        if (uiVclkSrc/(*ClkDiv) < MINIMUM_VCLK_FOR_TV)
        {
            *ClkDiv = uiVclkSrc/MINIMUM_VCLK_FOR_TV;
            DISP_INF((_T("[DISP:INF] Disp_get_vclk_direction_divider_forTVEnc() : Force Divider for Minimum VCLK of TV Encoder -> %d (%d Hz)\n\r"), *ClkDiv, uiVclkSrc/(*ClkDiv)));
        }
    }

    DISP_ERR((_T("[DISP] --Disp_get_vclk_direction_divider_forTVEnc()\n\r")));

    return TRUE;

}

