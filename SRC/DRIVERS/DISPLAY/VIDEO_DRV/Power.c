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

Module Name:    Power.c

Abstract:       Implementation of Video Driver
                This module handle Power context data

Functions:


Notes:


--*/

#include <bsp.h>
#include "pmplatform.h"
#include "SVEngine.h"

void SVE_initialize_power_context(void)
{
    SVEnginePowerContext *pPMCtxt;

    VDE_MSG((_T("[VDE] SVE_initialize_context()\r\n")));

    pPMCtxt = SVE_get_power_context();

    memset(pPMCtxt, 0x0, sizeof(SVEnginePowerContext));

    pPMCtxt->bPowerOn = TRUE;    // Initially Power On

    // Invalidate All Stored Values
    pPMCtxt->bFIMDOutputParam = FALSE;
    pPMCtxt->bFIMDOutputTV = FALSE;
    pPMCtxt->bFIMDOutputEnable = FALSE;
    pPMCtxt->bFIMDWinMode[DISP_WIN0] = FALSE;
    pPMCtxt->bFIMDWinMode[DISP_WIN1] = FALSE;
    pPMCtxt->bFIMDWinMode[DISP_WIN2] = FALSE;
    pPMCtxt->bFIMDWinMode[DISP_WIN3] = FALSE;
    pPMCtxt->bFIMDWinMode[DISP_WIN4] = FALSE;
    pPMCtxt->bFIMDWinFBuffer[DISP_WIN0] = FALSE;
    pPMCtxt->bFIMDWinFBuffer[DISP_WIN1] = FALSE;
    pPMCtxt->bFIMDWinFBuffer[DISP_WIN2] = FALSE;
    pPMCtxt->bFIMDWinFBuffer[DISP_WIN3] = FALSE;
    pPMCtxt->bFIMDWinFBuffer[DISP_WIN4] = FALSE;
    pPMCtxt->bFIMDColorKey[DISP_WIN0] = FALSE;    // Never Used
    pPMCtxt->bFIMDColorKey[DISP_WIN1] = FALSE;
    pPMCtxt->bFIMDColorKey[DISP_WIN2] = FALSE;
    pPMCtxt->bFIMDColorKey[DISP_WIN3] = FALSE;
    pPMCtxt->bFIMDColorKey[DISP_WIN4] = FALSE;
    pPMCtxt->bFIMDAlpha[DISP_WIN0] = FALSE;        // Never Used
    pPMCtxt->bFIMDAlpha[DISP_WIN1] = FALSE;
    pPMCtxt->bFIMDAlpha[DISP_WIN2] = FALSE;
    pPMCtxt->bFIMDAlpha[DISP_WIN3] = FALSE;
    pPMCtxt->bFIMDAlpha[DISP_WIN4] = FALSE;
    pPMCtxt->bFIMDWinEnable[DISP_WIN0] = FALSE;
    pPMCtxt->bFIMDWinEnable[DISP_WIN1] = FALSE;
    pPMCtxt->bFIMDWinEnable[DISP_WIN2] = FALSE;
    pPMCtxt->bFIMDWinEnable[DISP_WIN3] = FALSE;
    pPMCtxt->bFIMDWinEnable[DISP_WIN4] = FALSE;
    pPMCtxt->bPostParam = FALSE;
    pPMCtxt->bPostSrcBuffer = FALSE;
    pPMCtxt->bPostDstBuffer = FALSE;
    pPMCtxt->bPostStart = FALSE;
    pPMCtxt->bLocalPathWin0Enable = FALSE;
    pPMCtxt->bLocalPathWin1Enable = FALSE;
    pPMCtxt->bLocalPathWin2Enable = FALSE;
    pPMCtxt->bRotatorParam = FALSE;
    pPMCtxt->bRotatorSrcBuffer = FALSE;
    pPMCtxt->bRotatorDstBuffer = FALSE;
    pPMCtxt->bRotatorStart = FALSE;
    pPMCtxt->bTVSCParam = FALSE;
    pPMCtxt->bTVSCSrcBuffer = FALSE;
    pPMCtxt->bTVSCDstBuffer = FALSE;
    pPMCtxt->bTVSCStart = FALSE;
    pPMCtxt->bTVEncParam = FALSE;
    pPMCtxt->bTVEncEnable = FALSE;
}

void SVE_deinitialize_power_context(void)
{
    // Currently, Power Context is static global variable
    // So, we don't need to any deinit procedure
    DEBUGMSG(VDE_ZONE_TEMP, (_T("[VDE] SVE_deinitialize_power_context()\r\n")));
    return;
}

void SVE_video_engine_power_on(void)
{
    SVEngineContext *pCtxt;
    SVEnginePowerContext *pPMCtxt;
    int iWinCnt;

    DEBUGMSG(VDE_ZONE_ENTER, (_T("[VDE] ++SVE_video_engine_power_on()\r\n")));

    pCtxt = SVE_get_context();
    pPMCtxt = SVE_get_power_context();

    if (pPMCtxt->bPowerOn)
    {
        DEBUGMSG(VDE_ZONE_TEMP,(_T("[VDE:INF] SVE_video_engine_power_on() : Video Engine is Already Power On\r\n")));
        goto CleanUp;
    }

    //-------------------------------
    // HW Block Power On and Clock On
    //-------------------------------
    SVE_hw_power_control(HWPWR_DISPLAY_ON);
    SVE_hw_power_control(HWPWR_2D_ON);
    SVE_hw_clock_control(HWCLK_DISPLAY_ON);
    SVE_hw_clock_control(HWCLK_2D_ON);

    //-------------------
    // FIMD Reconfiguration
    //-------------------
    if (pPMCtxt->bFIMDOutputParam)
    {
        Disp_set_output_device_information(&pPMCtxt->tFIMDOutputParam.tRGBDevInfo);
        Disp_set_output_TV_information(pPMCtxt->tFIMDOutputParam.dwTVOutScreenWidth, pPMCtxt->tFIMDOutputParam.dwTVOutScreenHeight);

        if (pPMCtxt->bFIMDOutputTV)
        {
			DEBUGMSG(VDE_ZONE_ENTER, (_T("[VDE] bFIMDOutputTV == TRUE\r\n")));
            //Disp_initialize_output_interface(DISP_VIDOUT_TVENCODER);
        }
        else
        {
			DEBUGMSG(VDE_ZONE_ENTER, (_T("[VDE] bFIMDOutputTV==FALSE\r\n")));
            Disp_initialize_output_interface(DISP_VIDOUT_RGBIF);
        }
    }

    //---------------------------
    // FIMD Window Reconfiguration
    //---------------------------

    for (iWinCnt=DISP_WIN0; iWinCnt< DISP_WIN_MAX; iWinCnt++)
    {
        if (pPMCtxt->bFIMDWinMode[iWinCnt])
        {
            Disp_set_window_mode(pPMCtxt->tFIMDWinMode[iWinCnt].dwWinMode, pPMCtxt->tFIMDWinMode[iWinCnt].dwBPP,
                                    pPMCtxt->tFIMDWinMode[iWinCnt].dwWidth, pPMCtxt->tFIMDWinMode[iWinCnt].dwHeight,
                                    pPMCtxt->tFIMDWinMode[iWinCnt].dwOffsetX, pPMCtxt->tFIMDWinMode[iWinCnt].dwOffsetY);
        }

        if (pPMCtxt->bFIMDWinFBuffer[iWinCnt])
        {
            Disp_set_framebuffer(iWinCnt, pPMCtxt->tFIMDWinFBuffer[iWinCnt].dwFrameBuffer);
        }

        if (pPMCtxt->bFIMDColorKey[iWinCnt])
        {
            Disp_set_color_key(iWinCnt, pPMCtxt->tFIMDColorKey[iWinCnt].bOnOff, pPMCtxt->tFIMDColorKey[iWinCnt].dwDirection,
                                pPMCtxt->tFIMDColorKey[iWinCnt].dwColorKey, pPMCtxt->tFIMDColorKey[iWinCnt].dwCompareKey);
        }

        if (pPMCtxt->bFIMDAlpha[iWinCnt])
        {
            Disp_set_alpha_blending(iWinCnt, pPMCtxt->tFIMDAlpha[iWinCnt].dwMethod,
                                    pPMCtxt->tFIMDAlpha[iWinCnt].dwAlpha0, pPMCtxt->tFIMDAlpha[iWinCnt].dwAlpha1);
        }

        if (pPMCtxt->bFIMDWinEnable[iWinCnt])
        {
            Disp_window_onfoff(iWinCnt, DISP_WINDOW_ON);
            pCtxt->bWindowEnable[iWinCnt] = TRUE;
        }
    }

    //---------------------------
    // Post Processor Reconfiguration
    //---------------------------

    if (pPMCtxt->bPostParam)
    {
        SVE_hw_power_control(HWPWR_POST_ON);
        SVE_hw_clock_control(HWCLK_POST_ON);
        Post_initialize(pPMCtxt->tPostParam.dwOpMode, pPMCtxt->tPostParam.dwScanMode,
                    pPMCtxt->tPostParam.dwSrcType,
                    pPMCtxt->tPostParam.dwSrcBaseWidth, pPMCtxt->tPostParam.dwSrcBaseHeight,
                    pPMCtxt->tPostParam.dwSrcWidth, pPMCtxt->tPostParam.dwSrcHeight,
                    pPMCtxt->tPostParam.dwSrcOffsetX, pPMCtxt->tPostParam.dwSrcOffsetY,
                    pPMCtxt->tPostParam.dwDstType,
                    pPMCtxt->tPostParam.dwDstBaseWidth, pPMCtxt->tPostParam.dwDstBaseHeight,
                    pPMCtxt->tPostParam.dwDstWidth, pPMCtxt->tPostParam.dwDstHeight,
                    pPMCtxt->tPostParam.dwDstOffsetX, pPMCtxt->tPostParam.dwDstOffsetY);

        if (pPMCtxt->bPostSrcBuffer)
        {
            Post_set_source_buffer(pPMCtxt->tPostSrcBuffer.dwBufferRGBY,
                                        pPMCtxt->tPostSrcBuffer.dwBufferCb,
                                        pPMCtxt->tPostSrcBuffer.dwBufferCr);

            Post_set_next_source_buffer(pPMCtxt->tPostSrcBuffer.dwBufferRGBY,
                                        pPMCtxt->tPostSrcBuffer.dwBufferCb,
                                        pPMCtxt->tPostSrcBuffer.dwBufferCr);
        }

        if (pPMCtxt->bPostDstBuffer)
        {

            Post_set_destination_buffer(pPMCtxt->tPostDstBuffer.dwBufferRGBY,
                                        pPMCtxt->tPostDstBuffer.dwBufferCb,
                                        pPMCtxt->tPostDstBuffer.dwBufferCr);

            Post_set_next_destination_buffer(pPMCtxt->tPostDstBuffer.dwBufferRGBY,
                                        pPMCtxt->tPostDstBuffer.dwBufferCb,
                                        pPMCtxt->tPostDstBuffer.dwBufferCr);
        }

        if (pPMCtxt->bPostStart)
        {
            pPMCtxt->bTVSCStart  = FALSE;    // for Trigger Once
            Post_enable_interrupt();
            Post_processing_start();
        }
    }

    //-----------------------
    // TV Scaler Reconfiguration
    //-----------------------

    if (pPMCtxt->bTVSCParam)
    {
        SVE_hw_power_control(HWPWR_TV_ON);
        SVE_hw_clock_control(HWCLK_TV_ON);
        TVSC_initialize(pPMCtxt->tTVSCParam.dwOpMode, pPMCtxt->tTVSCParam.dwScanMode,
                    pPMCtxt->tTVSCParam.dwSrcType,
                    pPMCtxt->tTVSCParam.dwSrcBaseWidth, pPMCtxt->tTVSCParam.dwSrcBaseHeight,
                    pPMCtxt->tTVSCParam.dwSrcWidth, pPMCtxt->tTVSCParam.dwSrcHeight,
                    pPMCtxt->tTVSCParam.dwSrcOffsetX, pPMCtxt->tTVSCParam.dwSrcOffsetY,
                    pPMCtxt->tTVSCParam.dwDstType,
                    pPMCtxt->tTVSCParam.dwDstBaseWidth, pPMCtxt->tTVSCParam.dwDstBaseHeight,
                    pPMCtxt->tTVSCParam.dwDstWidth, pPMCtxt->tTVSCParam.dwDstHeight,
                    pPMCtxt->tTVSCParam.dwDstOffsetX, pPMCtxt->tTVSCParam.dwDstOffsetY);

        if (pPMCtxt->bTVSCSrcBuffer)
        {
            TVSC_set_source_buffer(pPMCtxt->tTVSCSrcBuffer.dwBufferRGBY,
                                        pPMCtxt->tTVSCSrcBuffer.dwBufferCb,
                                        pPMCtxt->tTVSCSrcBuffer.dwBufferCr);

            TVSC_set_next_source_buffer(pPMCtxt->tTVSCSrcBuffer.dwBufferRGBY,
                                        pPMCtxt->tTVSCSrcBuffer.dwBufferCb,
                                        pPMCtxt->tTVSCSrcBuffer.dwBufferCr);
        }

        if (pPMCtxt->bTVSCDstBuffer)
        {
            TVSC_set_destination_buffer(pPMCtxt->tTVSCDstBuffer.dwBufferRGBY,
                                        pPMCtxt->tTVSCDstBuffer.dwBufferCb,
                                        pPMCtxt->tTVSCDstBuffer.dwBufferCr);

            TVSC_set_next_destination_buffer(pPMCtxt->tTVSCDstBuffer.dwBufferRGBY,
                                        pPMCtxt->tTVSCDstBuffer.dwBufferCb,
                                        pPMCtxt->tTVSCDstBuffer.dwBufferCr);
        }

        if (pPMCtxt->bTVSCStart)
        {
            pPMCtxt->bTVSCStart  = FALSE;
            TVSC_enable_interrupt();
            TVSC_processing_start();
        }
    }

    //---------------------------
    // Local Path Reconfiguration
    //---------------------------

    if (pPMCtxt->bLocalPathWin0Enable)
    {
        Post_enable_interrupt();
        Post_processing_start();
        Disp_set_framebuffer(DISP_WIN0, IMAGE_FRAMEBUFFER_PA_START);    // Safe Frame Bufer Address for Local Path
        Disp_window_onfoff(DISP_WIN0, DISP_WINDOW_ON);

        pCtxt->bWindowEnable[DISP_WIN0] = TRUE;
    }

    if (pPMCtxt->bLocalPathWin1Enable)
    {
        // This require enabling CamIF MSDMA or TV Scaler
        // attatched to Display controller through LocalPath FIFO in caller process
        pCtxt->bWindowEnable[DISP_WIN1] = TRUE;
    }

    if (pPMCtxt->bLocalPathWin2Enable)
    {
        // This require enabling CamIF MSDMA or TV Scaler
        // attatched to Display controller through LocalPath FIFO in caller process
        pCtxt->bWindowEnable[DISP_WIN2] = TRUE;
    }

    //---------------------------
    // Image Rotator Reconfiguration
    //---------------------------

    if (pPMCtxt->bRotatorParam)
    {
        SVE_hw_power_control(HWPWR_ROTATOR_ON);
        SVE_hw_clock_control(HWCLK_ROTATOR_ON);
        Rotator_initialize(pPMCtxt->tRotatorParam.dwImgFormat,
                        pPMCtxt->tRotatorParam.dwOpType,
                        pPMCtxt->tRotatorParam.dwSrcWidth,
                        pPMCtxt->tRotatorParam.dwSrcHeight);

        if (pPMCtxt->bRotatorSrcBuffer)
        {
            Rotator_set_source_buffer(pPMCtxt->tRotatorSrcBuffer.dwBufferRGBY,
                                    pPMCtxt->tRotatorSrcBuffer.dwBufferCb,
                                    pPMCtxt->tRotatorSrcBuffer.dwBufferCr);
        }

        if (pPMCtxt->bRotatorDstBuffer)
        {
            Rotator_set_destination_buffer(pPMCtxt->tRotatorDstBuffer.dwBufferRGBY,
                                    pPMCtxt->tRotatorDstBuffer.dwBufferCb,
                                    pPMCtxt->tRotatorDstBuffer.dwBufferCr);
        }

        if (pPMCtxt->bRotatorStart)
        {
            pPMCtxt->bRotatorStart = FALSE;
            pCtxt->bRotatorBusy = TRUE;
            Rotator_enable_interrupt();
            Rotator_start();
        }
    }

    //---------------------------
    // TV Encoder Reconfiguration
    //---------------------------

    if (pPMCtxt->bTVEncParam)
    {
        TVEnc_initialize(pPMCtxt->tTVEncParam.dwOutputType,
                        pPMCtxt->tTVEncParam.dwOutputStandard, pPMCtxt->tTVEncParam.dwMVisionPattern,
                        pPMCtxt->tTVEncParam.dwSrcWidth, pPMCtxt->tTVEncParam.dwSrcHeight);
    }

    //-------------------
    // Enable Video Output
    //-------------------

    if (pPMCtxt->bTVEncEnable)
    {
        // TV Encoder On
        TVEnc_output_onoff(TVENC_ENCODER_ON);

        // TV Scaler Start
        TVSC_enable_interrupt();
        TVSC_processing_start();
    }

    if (pPMCtxt->bFIMDOutputEnable)
    {
        // Enable Interrupt
        Disp_set_frame_interrupt(DISP_FRMINT_FRONTPORCH);
        Disp_enable_frame_interrupt();

        // Video Output Enable
        Disp_envid_onoff(DISP_ENVID_ON);
        pCtxt->bVideoEnable = TRUE;
    }

    //-------------------
    // Update Power State
    //-------------------
    pPMCtxt->bPowerOn = TRUE;

CleanUp:

    VDE_MSG((_T("[VDE] --SVE_video_engine_power_on()\r\n")));
}

void SVE_video_engine_power_off(void)
{
    SVEngineContext *pCtxt;
    SVEnginePowerContext *pPMCtxt;

    DEBUGMSG(VDE_ZONE_ENTER, (_T("[VDE] ++SVE_video_engine_power_off()\r\n")));

    pCtxt = SVE_get_context();
    pPMCtxt = SVE_get_power_context();

    if (!pPMCtxt->bPowerOn)
    {
        DEBUGMSG(VDE_ZONE_TEMP,(_T("[VDE:INF] SVE_video_engine_power_off() : Video Engine is Already Power Off\r\n")));
        goto CleanUp;
    }

    //-----------------------
    // Disable Video Engine HW
    //-----------------------

    // Disable All Interrupt
    Disp_disable_frame_interrupt();
    Post_disable_interrupt();
    Rotator_disable_interrupt();
    TVSC_disable_interrupt();

    // Clear All Interrupt Pending
    Disp_clear_interrupt_pending();
    Post_clear_interrupt_pending();
    Rotator_clear_interrupt_pending();
    TVSC_clear_interrupt_pending();

    // Disable FIMD output
    Disp_envid_onoff(DISP_ENVID_DIRECT_OFF);
    pCtxt->bVideoEnable = FALSE;
    pCtxt->bWindowEnable[DISP_WIN0] = FALSE;
    pCtxt->bWindowEnable[DISP_WIN1] = FALSE;
    pCtxt->bWindowEnable[DISP_WIN2] = FALSE;
    pCtxt->bWindowEnable[DISP_WIN3] = FALSE;
    pCtxt->bWindowEnable[DISP_WIN4] = FALSE;

    // Disable TV Scaler Operation
    TVSC_processing_stop();

    // Disable TV Encoder Operation
    TVEnc_output_onoff(TVENC_ENCODER_OFF);

    // Disable Post Processor Operation
    Post_processing_stop();

    // Disable Rotator Operation
    Rotator_stop();
    pCtxt->bRotatorBusy = FALSE;

    //----------------------
    // Clear Command Context
    //----------------------
    pCtxt->DispCmdCtxt[DISP_WIN0].bCmdSetBuffer = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN0].bCmdSetPosition = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN1].bCmdSetBuffer = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN1].bCmdSetPosition = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN2].bCmdSetBuffer = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN2].bCmdSetPosition = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN3].bCmdSetBuffer = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN3].bCmdSetPosition = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN4].bCmdSetBuffer = FALSE;
    pCtxt->DispCmdCtxt[DISP_WIN4].bCmdSetPosition = FALSE;

    pCtxt->PostCmdCtxt.bCmdSetSrcBuffer = FALSE;
    pCtxt->PostCmdCtxt.bCmdSetDstBuffer = FALSE;

    pCtxt->LocalPathCmdCtxt.bCmdSetWin0Disable = FALSE;
    pCtxt->LocalPathCmdCtxt.bCmdSetWin1Disable = FALSE;
    pCtxt->LocalPathCmdCtxt.bCmdSetWin2Disable = FALSE;

    pCtxt->TVSCCmdCtxt.bCmdSetSrcBuffer = FALSE;
    pCtxt->TVSCCmdCtxt.bCmdSetDstBuffer = FALSE;

    //-------------------------------
    // HW Clock Off and Block Power Off
    //-------------------------------
    SVE_hw_clock_control(HWCLK_ALL_OFF);
    SVE_hw_power_control(HWPWR_ALL_OFF);

    //-------------------
    // Update Power State
    //-------------------
    pPMCtxt->bPowerOn = FALSE;

CleanUp:

    VDE_MSG((_T("[VDE] --SVE_video_engine_power_off()\r\n")));
}

BOOL SVE_initialize_power_control(void)
{
    SVEngineContext *pCtxt;

    VDE_MSG((_T("[VDE] ++SVE_initialize_power_control()\r\n")));

    pCtxt = SVE_get_context();

    // Open Device Power Control Driver
    pCtxt->hPowerControl = CreateFile( L"PWC0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (pCtxt->hPowerControl == INVALID_HANDLE_VALUE)
    {
        VDE_ERR((_T("[VDE:ERR] SVE_initialize_video_engine() : PWC0 Open Device Failed\r\n")));
        goto CleanUp;
    }

    VDE_MSG((_T("[VDE] ++SVE_initialize_power_control()\r\n")));

    return TRUE;

CleanUp:

    VDE_ERR((_T("[VDE:ERR] --SVE_initialize_power_control() : Failed\r\n")));

    return FALSE;
}

void SVE_deinitialize_power_control(void)
{
    SVEngineContext *pCtxt;

    VDE_MSG((_T("[VDE] ++SVE_deinitialize_power_control()\r\n")));

    pCtxt = SVE_get_context();

    if (pCtxt->hPowerControl !=INVALID_HANDLE_VALUE)
    {
        CloseHandle(pCtxt->hPowerControl);
    }

    VDE_MSG((_T("[VDE] ++SVE_deinitialize_power_control()\r\n")));
}

BOOL SVE_hw_clock_control(HWCLK_GATING eParams)
{
    SVEngineContext *pCtxt;
    BOOL bRet = TRUE;

    RETAILMSG(FALSE,(_T("[VDE] ++SVE_hw_clock_control(%d)\r\n"), eParams));

    pCtxt = SVE_get_context();

    switch(eParams)
    {
    case HWCLK_ALL_ON:
        pCtxt->pSysConReg->HCLK_GATE |= (1<<15)        // MSM I/F
                                    |(0<<9)        // TV Scaler
                                    |(1<<8)        // 2D
                                    |(0<<7)        // TV Encoder
                                    |(1<<5)        // Post Processor
                                    |(1<<4)        // Image Rotator
                                    |(1<<3);        // Display Controller
        pCtxt->pSysConReg->SCLK_GATE |= (0<<19)        // DAC 27MHz
                                    |(0<<18)    // TV Encoder 27MHz
                                    //|(1<<17)    // TV Scaler 27MHz
                                    |(0<<16)    // TV Scaler
                                    |(0<<15)    // Display Controller 27MHz
                                    |(1<<14)    // Display Controller
                                    //|(1<<13)    // Post Processor 1 27MHz
                                    //|(1<<12)    // Post Processor 0 27MHz
                                    //|(1<<11)    // Post Processor 1
                                    |(1<<10);    // Post Processor 0
        break;

    case HWCLK_ALL_OFF:
        pCtxt->pSysConReg->HCLK_GATE &= ~((1<<15)|/*(1<<9)| */(1<<8)|(1<<7)|(1<<5)|(1<<4)|(1<<3));
        pCtxt->pSysConReg->SCLK_GATE &= ~((1<<19)|(1<<18)|(1<<16)|(1<<15)|(1<<14)|(1<<10));
        break;

    case HWCLK_DISPLAY_ON:
        pCtxt->pSysConReg->HCLK_GATE |= (1<<15)        // MSM I/F
                                    |(1<<3);        // Display Controller
        pCtxt->pSysConReg->SCLK_GATE |= (1<<14);    // Display Controller
        break;

    case HWCLK_DISPLAY_OFF:
        pCtxt->pSysConReg->HCLK_GATE &= ~((1<<15)|(1<<3));
        pCtxt->pSysConReg->SCLK_GATE &= ~(1<<14);
        break;

    case HWCLK_MSMIF_ON:
        pCtxt->pSysConReg->HCLK_GATE |= (1<<15);    // MSM I/F
        break;

    case HWCLK_MSMIF_OFF:
        pCtxt->pSysConReg->HCLK_GATE &= ~(1<<15);    // MSM I/F
        break;

    case HWCLK_POST_ON:
        pCtxt->pSysConReg->HCLK_GATE |= (1<<5);        // Post Processor
        pCtxt->pSysConReg->SCLK_GATE |= (1<<10);    // Post Processor 0
        break;

    case HWCLK_POST_OFF:
        pCtxt->pSysConReg->HCLK_GATE &= ~(1<<5);    // Post Processor
        pCtxt->pSysConReg->SCLK_GATE &= ~(1<<10);    // Post Processor 0
        break;

    case HWCLK_ROTATOR_ON:
        pCtxt->pSysConReg->HCLK_GATE |= (1<<4);        // Image Rotator
        break;

    case HWCLK_ROTATOR_OFF:
        pCtxt->pSysConReg->HCLK_GATE &= ~(1<<4);    // Image Rotator
        break;

    case HWCLK_TV_ON:
		/*
        pCtxt->pSysConReg->HCLK_GATE |= (1<<9)        // TV Scaler
                                    |(1<<7);        // TV Encoder
        pCtxt->pSysConReg->SCLK_GATE |= (1<<19)        // DAC 27MHz
                                    |(1<<18)    // TV Encoder 27MHz
                                    |(1<<16)    // TV Scaler
                                    |(1<<15);    // Display Controller 27MHz
		*/
        break;

    case HWCLK_TV_OFF:
        pCtxt->pSysConReg->HCLK_GATE &= ~((1<<9)|(1<<7));
        pCtxt->pSysConReg->SCLK_GATE &= ~((1<<19)|(1<<18)|(1<<16)|(1<<15));
        break;

    case HWCLK_2D_ON:
        pCtxt->pSysConReg->HCLK_GATE |= (1<<8);        // FIMG 2D
        break;

    case HWCLK_2D_OFF:
        pCtxt->pSysConReg->HCLK_GATE &= ~(1<<8);    // FIMG 2D
        break;

    default:
        VDE_ERR((_T("[VDE:ERR] SVE_hw_clock_control() : Unknown Parameter = %d\n\r"), eParams));
        bRet = FALSE;
        break;
    }

    VDE_MSG((_T("[VDE] --SVE_hw_clock_control()\n\r")));

    return bRet;
}

BOOL SVE_hw_power_control(HWPWR_GATING eParams)
{
    SVEngineContext *pCtxt;
    DWORD dwIPIndex, dwBytes;
    BOOL bRet = TRUE;

    VDE_MSG((_T("[VDE] ++SVE_hw_power_control(%d)\r\n"), eParams));

    pCtxt = SVE_get_context();

    switch(eParams)
    {
    case HWPWR_ALL_ON:
        dwIPIndex = PWR_IP_DISPCON;    // Power On Display Controller
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }

        dwIPIndex = PWR_IP_POST;        // Power On Post Processor
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }

        dwIPIndex = PWR_IP_ROTATOR;    // Power On Image Rotator
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }

        dwIPIndex = PWR_IP_TVENC;    // Power On TV Encoder
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        dwIPIndex = PWR_IP_TVSC;        // Power On TV Scaler
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_ALL_OFF:
        dwIPIndex = PWR_IP_DISPCON;    // Power Off Display Controller
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }

        dwIPIndex = PWR_IP_POST;        // Power Off Post Processor
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }

        dwIPIndex = PWR_IP_ROTATOR;    // Power Off Image Rotator
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }

        dwIPIndex = PWR_IP_TVENC;    // Power Off TV Encoder
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }

        dwIPIndex = PWR_IP_TVSC;        // Power Off TV Scaler
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_DISPLAY_ON:
        dwIPIndex = PWR_IP_DISPCON;    // Power On Display Controller
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_DISPLAY_OFF:
        dwIPIndex = PWR_IP_DISPCON;    // Power Off Display Controller
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_POST_ON:
        dwIPIndex = PWR_IP_POST;        // Power On Post Processor
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_POST_OFF:
        dwIPIndex = PWR_IP_POST;        // Power Off Post Processor
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_ROTATOR_ON:
        dwIPIndex = PWR_IP_ROTATOR;    // Power On Image Rotator
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_ROTATOR_OFF:
        dwIPIndex = PWR_IP_ROTATOR;    // Power Off Image Rotator
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_TV_ON:
        dwIPIndex = PWR_IP_TVENC;    // Power On TV Encoder
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        dwIPIndex = PWR_IP_TVSC;        // Power On TV Scaler
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_TV_OFF:
        dwIPIndex = PWR_IP_TVENC;    // Power Off TV Encoder
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }
        dwIPIndex = PWR_IP_TVSC;        // Power Off TV Scaler
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_2D_ON:
        dwIPIndex = PWR_IP_2D;        // Power On FIMG 2D
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_ON Failed\n\r")));
            bRet = FALSE;
        }
        break;

    case HWPWR_2D_OFF:
        dwIPIndex = PWR_IP_2D;        // Power Off FIMG 2D
        if ( !DeviceIoControl(pCtxt->hPowerControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
        {
            VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : IOCTL_PWRCON_SET_POWER_OFF Failed\n\r")));
            bRet = FALSE;
        }
        break;

    default:
        VDE_ERR((_T("[VDE:ERR] SVE_hw_power_control() : Unknown Parameter = %d\n\r"), eParams));
        bRet = FALSE;
        break;
    }

    VDE_MSG((_T("[VDE] --SVE_hw_power_control()\n\r")));

    return bRet;
}

