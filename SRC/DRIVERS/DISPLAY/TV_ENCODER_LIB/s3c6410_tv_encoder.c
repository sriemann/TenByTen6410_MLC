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

Module Name:    s3c6410_tv_encoder.c

Abstract:       Implementation of TV Encoder Control Library
                This module implements Low Level HW control 

Functions:


Notes:


--*/

#include <windows.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "s3c6410_tv_encoder_macro.h"
#include "s3c6410_tv_encoder.h"

#define TVENC_MSG(x)
#define TVENC_INF(x)
#define TVENC_ERR(x)        RETAILMSG(TRUE, x)

static volatile S3C6410_TVENC_REG *g_pTVEncReg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static tTVEncConfig g_TVEncConfig;

TVENC_ERROR TVEnc_initialize_register_address(void *pTVEncReg, void *pGPIOReg)
{
    TVENC_ERROR error = TVENC_SUCCESS;

    TVENC_MSG((_T("[TVEnc]++TVEnc_initialize_register_address(0x%08x, 0x%08x)\n\r"), pTVEncReg, pGPIOReg));

    if (pTVEncReg == NULL || pGPIOReg == NULL)
    {
        TVENC_ERR((_T("[TVEnc:ERR] TVEnc_initialize_register_address() : NULL pointer parameter\n\r")));
        error = TVENC_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pTVEncReg = (S3C6410_TVENC_REG *)pTVEncReg;
        g_pGPIOReg = (S3C6410_GPIO_REG *)pGPIOReg;
        TVENC_INF((_T("[TVEnc:INF] g_pTVEncReg = 0x%08x\n\r"), g_pTVEncReg));
        TVENC_INF((_T("[TVEnc:INF] g_pGPIOReg = 0x%08x\n\r"), g_pGPIOReg));
    }

    memset((void *)(&g_TVEncConfig), 0x0, sizeof(tTVEncConfig));

    TVENC_MSG((_T("[TVEnc]--TVEnc_initialize_register_address() : %d\n\r"), error));

    return error;
}

TVENC_ERROR TVEnc_initialize(TVENC_OUTPUT_TYPE OutputType, TVENC_OUTPUT_STANDARD OutputStandard, TVENC_MACROVISION_PATTERN Macrovision, unsigned int Width, unsigned int Height)
{
    TVENC_ERROR error = TVENC_SUCCESS;

    TVENC_MSG((_T("[TVEnc]++TVEnc_initialize(%d, %d, %d, %d)\n\r"), OutputType, OutputStandard, Width, Height));

    g_TVEncConfig.OutputType = OutputType;
    g_TVEncConfig.OutputStandard = OutputStandard;
    g_TVEncConfig.MacrovisionPattern = Macrovision;
    g_TVEncConfig.Width = Width;
    g_TVEncConfig.Height = Height;

    TVEnc_initialize_port(OutputType);
    error = TVEnc_set_output_mode(OutputType, OutputStandard);
    if(error == TVENC_SUCCESS)
    {
        error = TVEnc_set_video_size(Width, Height);    
    }
    // In this time, Macrovision setting can be conducted.
    // But this hold patent. so contact us via e-mail on usermanual.
    
    TVENC_MSG((_T("[TVEnc]--TVEnc_initialize() : %d\n\r"), error));

    return error;
}

void TVEnc_output_onoff(TVENC_ENCODER_ONOFF OnOff)
{
    TVENC_MSG((_T("[TVEnc]++TVEnc_output_onoff(%d)\n\r"), OnOff));

    if (OnOff == TVENC_ENCODER_ON)
    {
        g_pTVEncReg->TVCTRL |= TVOUT_ON;
    }
    else
    {
        g_pTVEncReg->TVCTRL &= ~TVOUT_ON;
    }

    TVENC_MSG((_T("[TVEnc]--TVEnc_output_onoff()\n\r")));
}

TVENC_ENCODER_ONOFF TVEnc_get_output_state(void)
{
    if (g_pTVEncReg->TVCTRL & TVOUT_ON)
    {
        TVENC_MSG((_T("[TVEnc] TVEnc_get_output_state() = TVENC_ENCODER_ON\n\r")));
        return TVENC_ENCODER_ON;
    }
    else
    {
        TVENC_MSG((_T("[TVEnc] TVEnc_get_output_state() = TVENC_ENCODER_OFF\n\r")));
        return TVENC_ENCODER_OFF;
    }
}

void TVEnc_enable_interrupt(void)
{
    TVENC_MSG((_T("[TVEnc]++TVEnc_enable_interrupt()\n\r")));

    g_pTVEncReg->TVCTRL |= INTFIFO_URUN_EN;

    TVENC_MSG((_T("[TVEnc]--TVEnc_enable_interrupt()\n\r")));
}

void TVEnc_disable_interrupt(void)
{
    TVENC_MSG((_T("[TVEnc]++TVEnc_disable_interrupt()\n\r")));

    g_pTVEncReg->TVCTRL &= ~INTFIFO_URUN_EN;

    TVENC_MSG((_T("[TVEnc]--TVEnc_disable_interrupt()\n\r")));
}

BOOL TVEnc_clear_interrupt_pending(void)
{
    BOOL bRet = FALSE;

    //TVENC_MSG((_T("[TVEnc]++TVEnc_clear_interrupt_pending()\n\r")));

    if (g_pTVEncReg->TVCTRL & INTFIFO_PEND)
    {
        g_pTVEncReg->TVCTRL |= INTFIFO_PEND;
        bRet = TRUE;
    }

    //TVENC_MSG((_T("[TVEnc]--TVEnc_clear_interrupt_pending()\n\r")));

    return bRet;
}

static void TVEnc_initialize_port(TVENC_OUTPUT_TYPE Type)
{
    TVENC_MSG((_T("[TVEnc]++TVEnc_initialize_port()\n\r")));

    // There's no Initializetion procedure for S3C64xx

    TVENC_MSG((_T("[TVEnc]--TVEnc_initialize_port()\n\r")));
}

static TVENC_ERROR TVEnc_set_output_mode(TVENC_OUTPUT_TYPE Type, TVENC_OUTPUT_STANDARD Standard)
{
    TVENC_ERROR error = TVENC_SUCCESS;

    TVENC_MSG((_T("[TVEnc]++TVEnc_set_output_mode(%d, %d)\n\r"), Type, Standard));

    if (Type == TVENC_COMPOSITE)
    {
        g_pTVEncReg->TVCTRL &= ~TVOUTTYPE_S_VIDEO;

        switch(Standard)
        {
        case TVENC_NTSC_M:
        case TVENC_PAL_M:
        case TVENC_NTSC_J:
            g_pTVEncReg->YCFILTERBW = YBW_2_1MHZ | CBW_0_6MHZ;
            break;
        case TVENC_PAL_BDGHI:
        case TVENC_PAL_NC:
            g_pTVEncReg->YCFILTERBW = YBW_2_6MHZ | CBW_0_6MHZ;
            break;
        default:
            error = TVENC_ERROR_ILLEGAL_PARAMETER;
            g_pTVEncReg->YCFILTERBW = YBW_2_1MHZ | CBW_0_6MHZ;
            break;
        }
    }
    else if (Type == TVENC_S_VIDEO)
    {
        g_pTVEncReg->TVCTRL |= TVOUTTYPE_S_VIDEO;
        g_pTVEncReg->YCFILTERBW = YBW_6_0MHZ | CBW_0_6MHZ;
    }
    else
    {
        error = TVENC_ERROR_ILLEGAL_PARAMETER;
        TVENC_ERR((_T("[TVEnc:ERR] TVEnc_set_output_mode() : Unknown Type [%d]\n\r"), Type));
    }

    g_pTVEncReg->TVCTRL = (g_pTVEncReg->TVCTRL & ~(TVOUTFMT_MASK)) | (Standard<<4);

    switch(Standard)
    {
    case TVENC_NTSC_M:
    case TVENC_PAL_M:
        g_pTVEncReg->VBPORCH = VEFBPD_NTSC | VOFBPD_NTSC;
        g_pTVEncReg->HBPORCH = HSPW_NTSC | HBPD_NTSC;
        g_pTVEncReg->HENHOFFSET = DTOffset_NTSC | HEOV_NTSC;
        g_pTVEncReg->PEDCTRL = PEDOn;
        g_pTVEncReg->SYNCSIZECTRL = Sy_Size_NTSC;
        g_pTVEncReg->BURSTCTRL = Bu_End_NTSC | Bu_St_NTSC;
        g_pTVEncReg->MACROBURSTCTRL = Bumav_St_NTSC;
        g_pTVEncReg->ACTVIDPOSCTRL = Avon_End_NTSC | Avon_St_NTSC;
        break;
    case TVENC_NTSC_J:
        g_pTVEncReg->VBPORCH = VEFBPD_NTSC | VOFBPD_NTSC;
        g_pTVEncReg->HBPORCH = HSPW_NTSC | HBPD_NTSC;
        g_pTVEncReg->HENHOFFSET = DTOffset_NTSC | HEOV_NTSC;
        g_pTVEncReg->PEDCTRL = PEDOff;
        g_pTVEncReg->SYNCSIZECTRL = Sy_Size_NTSC;
        g_pTVEncReg->BURSTCTRL = Bu_End_NTSC | Bu_St_NTSC;
        g_pTVEncReg->MACROBURSTCTRL = Bumav_St_NTSC;
        g_pTVEncReg->ACTVIDPOSCTRL = Avon_End_NTSC | Avon_St_NTSC;
        break;
    case TVENC_PAL_BDGHI:
    case TVENC_PAL_NC:
        g_pTVEncReg->VBPORCH = VEFBPD_PAL | VOFBPD_PAL;
        g_pTVEncReg->HBPORCH = HSPW_PAL | HBPD_PAL;
        g_pTVEncReg->HENHOFFSET = DTOffset_PAL | HEOV_PAL;
        g_pTVEncReg->PEDCTRL = PEDOff;
        g_pTVEncReg->SYNCSIZECTRL = Sy_Size_PAL;
        g_pTVEncReg->BURSTCTRL = Bu_End_PAL | Bu_St_PAL;
        g_pTVEncReg->MACROBURSTCTRL = Bumav_St_PAL;
        g_pTVEncReg->ACTVIDPOSCTRL = Avon_End_PAL | Avon_St_PAL;
        break;
    default:
        error = TVENC_ERROR_ILLEGAL_PARAMETER;
        TVENC_ERR((_T("[TVEnc:ERR] TVEnc_set_output_mode() : Unknown Standard [%d]\n\r"), Standard));
    }

    // Sub_carrier reset enable
    g_pTVEncReg->FSCAUXCTRL = Fdrst_RESET;

    TVENC_MSG((_T("[TVEnc]--TVEnc_set_output_mode() = %d\n\r"), error));

    return error;
}

static TVENC_ERROR TVEnc_set_video_size(unsigned int uiWidth, unsigned int uiHeight)
{
    TVENC_ERROR error = TVENC_SUCCESS;

    TVENC_MSG((_T("[TVEnc]++TVEnc_set_video_size(%d, %d)\n\r"), uiWidth, uiHeight));

    if (uiWidth*2 < TV_MAXWIDTH && uiHeight < TV_MAXHEIGHT)
    {
        g_pTVEncReg->INIMAGESIZE = ImageHehgit(uiHeight) | ImageWidth(uiWidth*2);    // Width is double of horizontal pixel
    }
    else
    {
        TVENC_ERR((_T("[TVEnc:ERR] TVEnc_set_video_size() : Out of range %d x %d\n\r"), uiWidth, uiHeight));
        error = TVENC_ERROR_ILLEGAL_PARAMETER;
    }

    TVENC_MSG((_T("[TVEnc]--TVEnc_set_output_mode() = %d\n\r"), error));

    return error;
}
