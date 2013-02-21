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

Module Name:    ResourceAPI.c

Abstract:       Implementation of Video Driver
                This module handle Resource Context IOCTLs

Functions:


Notes:


--*/

#include <bsp.h>
#include "SVEngine.h"

BOOL SVE_Resource_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    )
{
    BOOL bRet = TRUE;

    VDE_MSG((_T("[VDE] ++SVE_Resource_API_Proc()\n\r")));

    switch(dwCode)
    {
    case SVE_RSC_REQUEST_FIMD_INTERFACE:
        bRet = SVE_resource_request_FIMD_interface(hOpenContext);
        break;
    case SVE_RSC_RELEASE_FIMD_INTERFACE:
        bRet = SVE_resource_release_FIMD_interface(hOpenContext);
        break;
    case SVE_RSC_REQUEST_FIMD_WIN0:
        bRet = SVE_resource_request_FIMD_window(DISP_WIN0, hOpenContext);
        break;
    case SVE_RSC_RELEASE_FIMD_WIN0:
        bRet = SVE_resource_release_FIMD_window(DISP_WIN0, hOpenContext);
        break;
    case SVE_RSC_REQUEST_FIMD_WIN1:
        bRet = SVE_resource_request_FIMD_window(DISP_WIN1, hOpenContext);
        break;
    case SVE_RSC_RELEASE_FIMD_WIN1:
        bRet = SVE_resource_release_FIMD_window(DISP_WIN1, hOpenContext);
        break;
    case SVE_RSC_REQUEST_FIMD_WIN2:
        bRet = SVE_resource_request_FIMD_window(DISP_WIN2, hOpenContext);
        break;
    case SVE_RSC_RELEASE_FIMD_WIN2:
        bRet = SVE_resource_release_FIMD_window(DISP_WIN2, hOpenContext);
        break;
    case SVE_RSC_REQUEST_FIMD_WIN3:
        bRet = SVE_resource_request_FIMD_window(DISP_WIN3, hOpenContext);
        break;
    case SVE_RSC_RELEASE_FIMD_WIN3:
        bRet = SVE_resource_release_FIMD_window(DISP_WIN3, hOpenContext);
        break;
    case SVE_RSC_REQUEST_FIMD_WIN4:
        bRet = SVE_resource_request_FIMD_window(DISP_WIN4, hOpenContext);
        break;
    case SVE_RSC_RELEASE_FIMD_WIN4:
        bRet = SVE_resource_release_FIMD_window(DISP_WIN4, hOpenContext);
        break;
    case SVE_RSC_REQUEST_POST:
        bRet = SVE_resource_request_Post(hOpenContext);
        break;
    case SVE_RSC_RELEASE_POST:
        bRet = SVE_resource_release_Post(hOpenContext);
        break;
    case SVE_RSC_REQUEST_ROTATOR:
        bRet = SVE_resource_request_Rotator(hOpenContext);
        break;
    case SVE_RSC_RELEASE_ROTATOR:
        bRet = SVE_resource_release_Rotator(hOpenContext);
        break;
    case SVE_RSC_REQUEST_TVSCALER_TVENCODER:
        bRet = SVE_resource_request_TVScaler_TVEncoder(hOpenContext);
        break;
    case SVE_RSC_RELEASE_TVSCALER_TVENCODER:
        bRet = SVE_resource_release_TVScaler_TVEncoder(hOpenContext);
        break;
    }

    if (bRet == FALSE)
    {
        VDE_ERR((_T("[VDE:ERR] SVE_Resource_API_Proc() : dwCode[0x%08x] Failed\n\r")));
    }

    return bRet;
}

BOOL SVE_resource_request_FIMD_interface(DWORD dwOpenContext)
{

	

    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

	DEBUGMSG(VDE_ZONE_ERROR, (_T("[VDE] +++SVE_resource_request_FIMD_interface()\r\n"), dwOpenContext));
    if (pCtxt->dwOccupantFIMD == 0)
    {
        pCtxt->dwOccupantFIMD = dwOpenContext;
        SVE_hw_power_control(HWPWR_DISPLAY_ON);
        SVE_hw_clock_control(HWCLK_DISPLAY_ON);
        pPMCtxt->bFIMDOutputParam = TRUE;
        VDE_MSG((_T("[VDE] SVE_resource_request_FIMD_interface() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else if (pCtxt->dwOccupantFIMD == dwOpenContext)
    {
        DEBUGMSG(VDE_ZONE_ERROR, (_T("[VDE:ERR] SVE_resource_request_FIMD_interface() : OpenContext[0x%08x] already have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_request_FIMD_interface() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
	DEBUGMSG(VDE_ZONE_ERROR, (_T("[VDE] ---SVE_resource_request_FIMD_interface()\r\n"), dwOpenContext));
}

BOOL SVE_resource_release_FIMD_interface(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

    if (pCtxt->dwOccupantFIMD == dwOpenContext)
    {
        pCtxt->dwOccupantFIMD = 0;
        SVE_hw_power_control(HWPWR_DISPLAY_OFF);
        SVE_hw_clock_control(HWCLK_DISPLAY_OFF);
        pPMCtxt->bFIMDOutputParam = FALSE;
        VDE_MSG((_T("[VDE] SVE_resource_request_FIMD_interface() : OpenContext[0x%08x] release resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_request_FIMD_interface() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
}

BOOL SVE_resource_compare_FIMD_interface(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
	
	VDE_ERR( ( _T("VDE_Ressource: +++SVE_resource_compare_FIMD_interface\n")) );
	VDE_ERR(( _T("VDE_Ressource: openContext 0x%X\n"),dwOpenContext ) );
	if(!pCtxt) 
	{
		VDE_ERR( ( _T("VDE_Ressource: SVE context bad\n")) );
		return FALSE;}
    if (((DWORD)(pCtxt->dwOccupantFIMD)) == dwOpenContext)
    {
        VDE_ERR( (_T("[VDE] SVE_resource_compare_FIMD_interface() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else 
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_request_FIMD_interface() : Resource was occupied by OpenContext: 0x%x\r\n"), pCtxt->dwOccupantFIMD));
        return FALSE;
    }
	VDE_ERR(( _T("VDE_Ressource: ---SVE_resource_compare_FIMD_interface\n")) );
}

BOOL SVE_resource_request_FIMD_window(DWORD dwWinNum, DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();

    if (pCtxt->dwOccupantFIMDWindow[dwWinNum] == 0)
    {
        pCtxt->dwOccupantFIMDWindow[dwWinNum] = dwOpenContext;
        VDE_MSG((_T("[VDE] SVE_resource_request_FIMD_window() : OpenContext[0x%08x] have resource Win[%d]\r\n"), dwOpenContext, dwWinNum));
        return TRUE;
    }
    else if (pCtxt->dwOccupantFIMDWindow[dwWinNum] == dwOpenContext)
    {
        DEBUGMSG(VDE_ZONE_TEMP,(_T("[VDE:ERR] SVE_resource_request_FIMD_window() : OpenContext[0x%08x] already have resource Win[%d]\r\n"), dwOpenContext, dwWinNum));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_request_FIMD_window() : Resource Win[%d] was occupied by other OpenContext\r\n"), dwWinNum));
        return FALSE;
    }
}

BOOL SVE_resource_release_FIMD_window(DWORD dwWinNum, DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();

    if (pCtxt->dwOccupantFIMDWindow[dwWinNum] == dwOpenContext)
    {
        pCtxt->dwOccupantFIMDWindow[dwWinNum] = 0;
        DEBUGMSG(VDE_ZONE_TEMP,(_T("[VDE] SVE_resource_release_FIMD_window() : OpenContext[0x%08x] release resource Win[%d]\r\n"), dwOpenContext, dwWinNum));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_release_FIMD_window() : Resource Win[%d] was occupied by other OpenContext\r\n"), dwWinNum));
        return FALSE;
    }
}

BOOL SVE_resource_compare_FIMD_window(DWORD dwWinNum, DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();

    if (pCtxt->dwOccupantFIMDWindow[dwWinNum] == dwOpenContext)
    {
        DEBUGMSG(VDE_ZONE_TEMP,(_T("[VDE] SVE_resource_compare_FIMD_window() : OpenContext[0x%08x] have resource Win[%d]\r\n"), dwOpenContext, dwWinNum));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_compare_FIMD_window() : Resource Win[%d]:0x%x was occupied by other OpenContext\r\n"), dwWinNum, pCtxt->dwOccupantFIMDWindow[dwWinNum]));
        return FALSE;
    }
}

BOOL SVE_resource_request_Post(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

    if (pCtxt->dwOccupantPost == 0)
    {
        pCtxt->dwOccupantPost = dwOpenContext;
        SVE_hw_power_control(HWPWR_POST_ON);
        SVE_hw_clock_control(HWCLK_POST_ON);
        pPMCtxt->bPostParam = TRUE;
        VDE_MSG((_T("[VDE] SVE_resource_request_Post() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else if (pCtxt->dwOccupantPost == dwOpenContext)
    {
        DEBUGMSG(VDE_ZONE_TEMP,(_T("[VDE:ERR] SVE_resource_request_Post() : OpenContext[0x%08x] already have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_request_Post() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
}

BOOL SVE_resource_release_Post(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

    if (pCtxt->dwOccupantPost == dwOpenContext)
    {
        pCtxt->dwOccupantPost = 0;
        SVE_hw_power_control(HWPWR_POST_OFF);
        SVE_hw_clock_control(HWCLK_POST_OFF);
        pPMCtxt->bPostParam = FALSE;
        VDE_MSG((_T("[VDE] SVE_resource_release_Post() : OpenContext[0x%08x] release resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_release_Post() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
}

BOOL SVE_resource_compare_Post(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();

    if (pCtxt->dwOccupantPost == dwOpenContext)
    {
        VDE_MSG((_T("[VDE] SVE_resource_compare_Post() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else 
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_compare_Post() : Resource was occupied by other OpenContext:0x%x\r\n"),pCtxt->dwOccupantPost ));
        return FALSE;
    }
}

BOOL SVE_resource_request_Rotator(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

    if (pCtxt->dwOccupantRotator == 0)
    {
        pCtxt->dwOccupantRotator = dwOpenContext;
        SVE_hw_power_control(HWPWR_ROTATOR_ON);
        SVE_hw_clock_control(HWCLK_ROTATOR_ON);
        pPMCtxt->bRotatorParam = TRUE;
        VDE_MSG((_T("[VDE] SVE_resource_request_Rotator() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else if (pCtxt->dwOccupantRotator == dwOpenContext)
    {
        DEBUGMSG(VDE_ZONE_TEMP,(_T("[VDE:ERR] SVE_resource_request_Rotator() : OpenContext[0x%08x] already have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_request_Rotator() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
}

BOOL SVE_resource_release_Rotator(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

    if (pCtxt->dwOccupantRotator == dwOpenContext)
    {
        pCtxt->dwOccupantRotator = 0;
        SVE_hw_power_control(HWPWR_ROTATOR_OFF);
        SVE_hw_clock_control(HWCLK_ROTATOR_OFF);
        pPMCtxt->bRotatorParam = FALSE;
        VDE_MSG((_T("[VDE] SVE_resource_release_Rotator() : OpenContext[0x%08x] release resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_release_Rotator() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
}

BOOL SVE_resource_compare_Rotator(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();

    if (pCtxt->dwOccupantRotator == dwOpenContext)
    {
        VDE_MSG((_T("[VDE] SVE_resource_compare_Rotator() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_compare_Rotator() : Resource was occupied by other OpenContext:0x%x\r\n"),pCtxt->dwOccupantRotator));
        return FALSE;
    }
}

BOOL SVE_resource_request_TVScaler_TVEncoder(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

    if (pCtxt->dwOccupantTVScalerTVEncoder == 0)
    {
        pCtxt->dwOccupantTVScalerTVEncoder = dwOpenContext;
        SVE_hw_power_control(HWPWR_TV_ON);
        SVE_hw_clock_control(HWCLK_TV_ON);
        pPMCtxt->bTVSCParam = TRUE;
        pPMCtxt->bTVEncParam = TRUE;
        VDE_MSG((_T("[VDE] SVE_resource_request_TVScaler_TVEncoder() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else if (pCtxt->dwOccupantTVScalerTVEncoder == dwOpenContext)
    {
        DEBUGMSG(VDE_ZONE_TEMP, (_T("[VDE:ERR] SVE_resource_request_TVScaler_TVEncoder() : OpenContext[0x%08x] already have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_request_TVScaler_TVEncoder() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
}

BOOL SVE_resource_release_TVScaler_TVEncoder(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();
    SVEnginePowerContext *pPMCtxt = SVE_get_power_context();

    if (pCtxt->dwOccupantTVScalerTVEncoder == dwOpenContext)
    {
        pCtxt->dwOccupantTVScalerTVEncoder = 0;
        SVE_hw_power_control(HWPWR_TV_OFF);
        SVE_hw_clock_control(HWCLK_TV_OFF);
        pPMCtxt->bTVSCParam = FALSE;
        pPMCtxt->bTVEncParam = FALSE;
        VDE_MSG((_T("[VDE] SVE_resource_release_TVScaler_TVEncoder() : OpenContext[0x%08x] release resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_release_TVScaler_TVEncoder() : Resource was occupied by other OpenContext\r\n")));
        return FALSE;
    }
}

BOOL SVE_resource_compare_TVScaler_TVEncoder(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();

    if (pCtxt->dwOccupantTVScalerTVEncoder == dwOpenContext)
    {
        VDE_MSG((_T("[VDE] SVE_resource_compare_TVScaler_TVEncoder() : OpenContext[0x%08x] have resource\r\n"), dwOpenContext));
        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_resource_compare_TVScaler_TVEncoder() : Resource was occupied by other OpenContext:0x%x\r\n"), pCtxt->dwOccupantTVScalerTVEncoder));
        return FALSE;
    }
}

