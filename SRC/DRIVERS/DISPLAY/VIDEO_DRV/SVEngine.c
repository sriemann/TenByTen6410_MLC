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

Module Name:    SVEngine.c

Abstract:       Implementation of Video Driver. 
                This module handle context data, and Video Engine On/Off

Functions:


Notes:


--*/

#include <bsp.h>
#include "SVEngine.h"

static SVEngineContext SVECtxt;
static SVEnginePowerContext SVEPMCtxt;

BOOL SVE_initialize_video_engine(void)
{
	DWORD dwDisplayType[2] = {123,16};
    DWORD dwBytesRet = 0;
    SVEngineContext *pCtxt;

    RETAILMSG(1,(_T("[VDE] ++SVE_initialize_video_engine()\r\n")));

    pCtxt = SVE_get_context();

    // Clear Context
    SVE_initialize_context();
    SVE_initialize_power_context();

    // map device SFR address
    if (SVE_map_device_address() == FALSE)
    {
        VDE_ERR((_T("[VDE:ERR] SVE_initialize_video_engine() : SVE_map_device_address() Failed\r\n")));
        goto CleanUp;
    }

    // Intialize interrupt
    if (SVE_initialize_interrupt() == FALSE)
    {
        VDE_ERR((_T("[VDE:ERR] SVE_initialize_video_engine() : SVE_intialize_interrupt() Failed\r\n")));
        goto CleanUp;
    }

    // Create Interrupt Thread
    if (SVE_initialize_thread() == FALSE)
    {
        VDE_ERR((_T("[VDE:ERR] SVE_initialize_video_engine() : SVE_initialize_thread() Failed\r\n")));
        goto CleanUp;
    }

    // Open Power Control Driver
    if (SVE_initialize_power_control() == FALSE)
    {
        VDE_ERR((_T("[VDE:ERR] SVE_initialize_video_engine() : SVE_initialize_power_control() Failed\r\n")));
        goto CleanUp;
    }

    // Initialize SFR Address of Sub Module
    LDI_initialize_register_address((void *)pCtxt->pSPIReg, (void *)pCtxt->pDispConReg, (void *)pCtxt->pGPIOReg);
    Disp_initialize_register_address((void *)pCtxt->pDispConReg, (void *)pCtxt->pMSMIFReg, (void *)pCtxt->pGPIOReg);
    Post_initialize_register_address((void *)pCtxt->pPostReg);
    Rotator_initialize_register_address((void *)pCtxt->pRotatorReg);
    TVSC_initialize_register_address((void *)pCtxt->pTVSCReg);
    TVEnc_initialize_register_address((void *)pCtxt->pTVEncReg, (void *)pCtxt->pGPIOReg);


	if (   KernelIoControl (IOCTL_HAL_QUERY_DISPLAYSETTINGS, NULL, 0, dwDisplayType, sizeof(DWORD)*2, &dwBytesRet)  // get data from BSP_ARGS via KernelIOCtl
                        && (dwBytesRet == (sizeof(DWORD)*2)))
	{
		RETAILMSG(1,(TEXT("[DISPDRV1] display driver display: %s\r\n"),LDI_getDisplayName((HITEG_DISPLAY_TYPE)dwDisplayType[0])));
		LDI_set_LCD_module_type((HITEG_DISPLAY_TYPE)dwDisplayType[0]);
	}
	else
	{
		RETAILMSG(1,(TEXT("[DISPDRV1] Error getting Display type from args section via Kernel IOCTL!!!\r\n")));
	}

    SVE_hw_power_control(HWPWR_DISPLAY_ON);
    SVE_hw_power_control(HWPWR_2D_ON);

    SVE_hw_clock_control(HWCLK_DISPLAY_ON);
    SVE_hw_clock_control(HWCLK_2D_ON);

    RETAILMSG(1,(_T("[VDE] --SVE_initialize_video_engine()\r\n")));

    return TRUE;

CleanUp:

    SVE_deinitialize_video_engine();

    VDE_ERR((_T("[VDE:ERR] --SVE_initialize_video_engine() : Failed\r\n")));


    return FALSE;
}


void SVE_deinitialize_video_engine(void)
{
    VDE_MSG((_T("[VDE] ++SVE_deinitialize_video_engine()\r\n")));

    SVE_deinitialize_context();
    SVE_deinitialize_power_context();
    SVE_unmap_device_address();
    SVE_deinitialize_interrupt();    
    SVE_deinitialize_thread();
    SVE_deinitialize_power_control();

    VDE_MSG((_T("[VDE] --SVE_deinitialize_video_engine()\r\n")));
}

SVEngineContext* SVE_get_context(void)
{
	RETAILMSG( 1, ( _T("VDE_SVEEINGINEcontext: Context 0x%X, 0x%X\n"), SVECtxt, &SVECtxt) );
    return &SVECtxt;
}

SVEnginePowerContext* SVE_get_power_context(void)
{
    return &SVEPMCtxt;
}

BOOL SVE_map_device_address(void)
{
    SVEngineContext *pCtxt;
    PHYSICAL_ADDRESS    ioPhysicalBase = { 0,0};

    VDE_MSG((_T("[VDE] ++%s\r\n"), _T(__FUNCTION__)));

    pCtxt = SVE_get_context();

    // Translate to System Address.
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DISPLAY;
    pCtxt->pDispConReg = (S3C6410_DISPLAY_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DISPLAY_REG), FALSE);
    if (pCtxt->pDispConReg == NULL)
    {
        goto CleanUp;  
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_POST;
    pCtxt->pPostReg = (S3C6410_POST_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_POST_REG), FALSE);
    if (pCtxt->pPostReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_ROTATOR;
    pCtxt->pRotatorReg = (S3C6410_ROTATOR_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_ROTATOR_REG), FALSE);
    if (pCtxt->pRotatorReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_TVSC;
    pCtxt->pTVSCReg = (S3C6410_TVSC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_TVSC_REG), FALSE);
    if (pCtxt->pTVSCReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_TVENC;
    pCtxt->pTVEncReg = (S3C6410_TVENC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_TVENC_REG), FALSE);
    if (pCtxt->pTVEncReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_MSMIF_SFR;
    pCtxt->pMSMIFReg = (S3C6410_MSMIF_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_MSMIF_REG), FALSE);
    if (pCtxt->pMSMIFReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
    pCtxt->pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (pCtxt->pSysConReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    pCtxt->pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (pCtxt->pGPIOReg == NULL)
    {
        goto CleanUp;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SPI0;
    pCtxt->pSPIReg = (S3C6410_SPI_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SPI_REG), FALSE);
    if (pCtxt->pSPIReg == NULL)
    {
        goto CleanUp;
    }

    VDE_MSG((_T("[VDE] --%s\r\n"), _T(__FUNCTION__)));

    return TRUE;

CleanUp:

    VDE_ERR((_T("[VDE:ERR] --%s : Failed, ioPhysicalBase(0x%x,0x%x)\r\n"), _T(__FUNCTION__), ioPhysicalBase.LowPart, ioPhysicalBase.HighPart));

    return FALSE;
}

void SVE_unmap_device_address(void)
{
    SVEngineContext *pCtxt;

    VDE_MSG((_T("[VDE] ++SVE_unmap_device_address()\r\n")));

    pCtxt = SVE_get_context();

    if (pCtxt->pDispConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pDispConReg, sizeof(S3C6410_DISPLAY_REG));
        pCtxt->pDispConReg = NULL;
    }

    if (pCtxt->pPostReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pPostReg, sizeof(S3C6410_POST_REG));
        pCtxt->pPostReg = NULL;
    }

    if (pCtxt->pRotatorReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pRotatorReg, sizeof(S3C6410_ROTATOR_REG));
        pCtxt->pRotatorReg = NULL;
    }

    if (pCtxt->pTVSCReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pTVSCReg, sizeof(S3C6410_TVSC_REG));
        pCtxt->pTVSCReg = NULL;
    }

    if (pCtxt->pTVEncReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pTVEncReg, sizeof(S3C6410_TVENC_REG));
        pCtxt->pTVEncReg = NULL;
    }

    if (pCtxt->pMSMIFReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pMSMIFReg, sizeof(S3C6410_MSMIF_REG));
        pCtxt->pMSMIFReg = NULL;
    }

    if (pCtxt->pSysConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pSysConReg, sizeof(S3C6410_SYSCON_REG));
        pCtxt->pSysConReg = NULL;
    }

    if (pCtxt->pGPIOReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pGPIOReg, sizeof(S3C6410_GPIO_REG));
        pCtxt->pGPIOReg = NULL;
    }

    if (pCtxt->pSPIReg != NULL)
    {
        MmUnmapIoSpace((PVOID)pCtxt->pSPIReg, sizeof(S3C6410_SPI_REG));
        pCtxt->pSPIReg = NULL;
    }

    VDE_MSG((_T("[VDE] --SVE_unmap_device_address()\r\n")));
}

void SVE_initialize_context(void)
{
    SVEngineContext *pCtxt;
    int iWinNum;

    VDE_MSG((_T("[VDE] ++SVE_initialize_context()\r\n")));

    pCtxt = SVE_get_context();

    // Clear Context
    memset(pCtxt, 0x0, sizeof(SVEngineContext));

    pCtxt->dwSignature = SVE_DRIVER_SIGNITURE;
    pCtxt->dwLastOpenContext = SVE_DRIVER_SIGNITURE;

    pCtxt->dwSysIntrDisp = SYSINTR_UNDEFINED;
    pCtxt->dwSysIntrPost = SYSINTR_UNDEFINED;
    pCtxt->dwSysIntrTVSC = SYSINTR_UNDEFINED;
    pCtxt->dwSysIntrRotator = SYSINTR_UNDEFINED;
    pCtxt->hPowerControl = INVALID_HANDLE_VALUE;

    // Critical Section for IOCTL
    InitializeCriticalSection(&pCtxt->csProc);

    // Display Controller Command Context
    for (iWinNum = DISP_WIN0; iWinNum < DISP_WIN_MAX; iWinNum ++)
    {
        InitializeCriticalSection(&pCtxt->DispCmdCtxt[iWinNum].csCmd);
        pCtxt->DispCmdCtxt[iWinNum].bCmdSetBuffer = FALSE;
        pCtxt->DispCmdCtxt[iWinNum].bCmdSetPosition = FALSE;
    }

    // Post Processor Command Context
    InitializeCriticalSection(&pCtxt->PostCmdCtxt.csCmd);
    pCtxt->PostCmdCtxt.bCmdSetSrcBuffer = FALSE;
    pCtxt->PostCmdCtxt.bCmdSetDstBuffer = FALSE;

    // Local Path Command Context
    InitializeCriticalSection(&pCtxt->LocalPathCmdCtxt.csCmd);
    pCtxt->LocalPathCmdCtxt.bCmdSetWin0Enable = FALSE;        // Depricated
    pCtxt->LocalPathCmdCtxt.bCmdSetWin0Disable = FALSE;
    pCtxt->LocalPathCmdCtxt.bCmdSetWin1Enable = FALSE;        // Depricated
    pCtxt->LocalPathCmdCtxt.bCmdSetWin1Disable = FALSE;
    pCtxt->LocalPathCmdCtxt.bCmdSetWin2Enable = FALSE;        // Depricated
    pCtxt->LocalPathCmdCtxt.bCmdSetWin2Disable = FALSE;

    // TV Scaler Command Context
    InitializeCriticalSection(&pCtxt->TVSCCmdCtxt.csCmd);
    pCtxt->TVSCCmdCtxt.bCmdSetSrcBuffer = FALSE;
    pCtxt->TVSCCmdCtxt.bCmdSetDstBuffer = FALSE;

    // Command Event
    pCtxt->hDispCmdDone = CreateEvent(NULL, TRUE, FALSE, NULL);    // Manual Reset, You should call ResetEvent() or use PulseEvent() Only
    pCtxt->hPostCmdDone = CreateEvent(NULL, TRUE, FALSE, NULL);    // Manual Reset, You should call ResetEvent() or use PulseEvent() Only
    pCtxt->hTVSCCmdDone = CreateEvent(NULL, TRUE, FALSE, NULL);    // Manual Reset, You should call ResetEvent() or use PulseEvent() Only

    // Operation Finish Event
    pCtxt->hRotatorFinish = CreateEvent(NULL, TRUE, FALSE, NULL);    // Manual Reset, You should call ResetEvent() or use PulseEvent() Only

    VDE_MSG((_T("[VDE] --SVE_initialize_context()\r\n")));
}


void SVE_deinitialize_context(void)
{
    SVEngineContext *pCtxt;
    int iWinNum;

    VDE_MSG((_T("[VDE] ++SVE_deinitialize_context()\r\n")));

    pCtxt = SVE_get_context();

    // Display Controller Command Context
    for (iWinNum = DISP_WIN0; iWinNum < DISP_WIN_MAX; iWinNum ++)
    {
        DeleteCriticalSection(&pCtxt->DispCmdCtxt[iWinNum].csCmd);
    }

    // Post Processor Command Context
    DeleteCriticalSection(&pCtxt->PostCmdCtxt.csCmd);

    // Local Path Command Context
    DeleteCriticalSection(&pCtxt->LocalPathCmdCtxt.csCmd);

    // TV Scaler Command Context
    DeleteCriticalSection(&pCtxt->TVSCCmdCtxt.csCmd);

    // Critical Section for IOCTL
    DeleteCriticalSection(&pCtxt->csProc);

    if (pCtxt->hDispCmdDone != NULL)
    {
        CloseHandle(pCtxt->hDispCmdDone);
    }

    if (pCtxt->hPostCmdDone != NULL)
    {
        CloseHandle(pCtxt->hPostCmdDone);
    }

    if (pCtxt->hTVSCCmdDone != NULL)
    {
        CloseHandle(pCtxt->hTVSCCmdDone);
    }

    if (pCtxt->hRotatorFinish != NULL)
    {
        CloseHandle(pCtxt->hRotatorFinish);
    }

    VDE_MSG((_T("[VDE] --SVE_deinitialize_context()\r\n")));
}


DWORD SVE_get_api_function_code(DWORD dwCode)
{
    //#define CTL_CODE( DeviceType, Function, Method, Access ) ( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )
    return ((dwCode>>2)&0xFFF);
}

DWORD SVE_get_driver_signature(void)
{
    return (SVE_get_context()->dwSignature);
}

DWORD SVE_add_open_context(void)
{
    SVEngineContext *pCtxt = SVE_get_context();

    pCtxt->dwOpenCount++;
    pCtxt->dwLastOpenContext++;

    VDE_MSG((_T("[VDE] SVE_add_open_context() : OpenCount = %d, OpenContext = 0x%08x\r\n"), pCtxt->dwOpenCount, pCtxt->dwLastOpenContext));

    return (pCtxt->dwLastOpenContext);
}

BOOL SVE_remove_open_context(DWORD dwOpenContext)
{
    SVEngineContext *pCtxt = SVE_get_context();

    if (pCtxt->dwOpenCount > 0)
    {
        // Release H/W Resource
        if (pCtxt->dwOccupantFIMD == dwOpenContext) pCtxt->dwOccupantFIMD = 0;
        if (pCtxt->dwOccupantFIMDWindow[0] == dwOpenContext) pCtxt->dwOccupantFIMDWindow[0] = 0;
        if (pCtxt->dwOccupantFIMDWindow[1] == dwOpenContext) pCtxt->dwOccupantFIMDWindow[1] = 0;
        if (pCtxt->dwOccupantFIMDWindow[2] == dwOpenContext) pCtxt->dwOccupantFIMDWindow[2] = 0;
        if (pCtxt->dwOccupantFIMDWindow[3] == dwOpenContext) pCtxt->dwOccupantFIMDWindow[3] = 0;
        if (pCtxt->dwOccupantFIMDWindow[4] == dwOpenContext) pCtxt->dwOccupantFIMDWindow[4] = 0;
        if (pCtxt->dwOccupantPost == dwOpenContext) pCtxt->dwOccupantPost = 0;
        if (pCtxt->dwOccupantRotator == dwOpenContext) pCtxt->dwOccupantRotator = 0;
        if (pCtxt->dwOccupantTVScalerTVEncoder == dwOpenContext) pCtxt->dwOccupantTVScalerTVEncoder = 0;

        pCtxt->dwOpenCount--;

        VDE_MSG((_T("[VDE] SVE_remove_open_context(0x%08x) : OpenCount = %d\r\n"), pCtxt->dwLastOpenContext, pCtxt->dwOpenCount));

        return TRUE;
    }
    else
    {
        VDE_ERR((_T("[VDE:ERR] SVE_remove_open_context() : Current Open Count is 0 !!!\r\n")));

        return FALSE;
    }
}

DWORD SVE_get_current_open_count(void)
{
    return (SVE_get_context()->dwOpenCount);
}

