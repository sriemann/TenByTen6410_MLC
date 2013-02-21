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

Module Name:    video_driver.c

Abstract:       Implementation of Video Driver. 
                This driver can handle DisplayController, TV Encoder/Scaler, PostProcessor, Image Rotator
                This driver support many IOCTLs for above HW IPs
                Application can use HW IPs with DeviceIoCtl

Functions:


Notes:


--*/

#include <bsp.h>
#include "SVEngine.h"


DBGPARAM dpCurSettings =                                \
{                                                       \
    TEXT(__MODULE__),                                   \
    {                                                   \
        TEXT("Errors"),                 /* 0  */        \
        TEXT("Warnings"),               /* 1  */        \
        TEXT("Performance"),            /* 2  */        \
        TEXT("Temporary tests"),        /* 3  */        \
        TEXT("Enter,Exit"),             /* 4  */        \
        TEXT("Initialize"),             /* 5  */        \
        TEXT("IOCTL : Block Power On"), /* 6  */        \
        TEXT("IOCTL : Block Power Off"),/* 7  */        \
        TEXT("IOCTL : DVS Profile"),    /* 8  */        \
        TEXT("IOCTL : DVS Change"),     /* 9  */        \
    },                                                  \
    (VDE_ZONES)                               \
};

BOOL
WINAPI
DllEntry(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DEBUGREGISTER(hinstDll);
        RETAILREGISTERZONES(hinstDll);
        //DisableThreadLibraryCalls ((HMODULE)hinstDll);
        VDE_MSG((_T("[VDE] DllEntry() : Process Attach\r\n")));
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        VDE_MSG((_T("[VDE] DllEntry() : Process Detach\r\n")));
    }

    return TRUE;
}

DWORD
VDE_Init(
    LPCTSTR pContext
    )
{
    VDE_MSG((_T("[VDE] ++VDE_Init(0x%08x)\r\n"), pContext));

    if (SVE_initialize_video_engine() == FALSE)
    {
        VDE_ERR((_T("[VDE:ERR] VDE_Init() : SVE_initialize_video_engine() Failed \n\r")));
        goto CleanUp;
    }

    VDE_MSG((_T("[VDE] --VDE_Init()\r\n")));

    return SVE_get_driver_signature();    // hDeviceContext

CleanUp:

    VDE_ERR((_T("[VDE:ERR] --VDE_Init() : Failed\r\n")));

    return 0;
}

BOOL
VDE_Deinit(
    DWORD hDeviceContext
    )
{
    VDE_MSG((_T("[VDE] ++VDE_Deinit(0x%08x)\r\n"), hDeviceContext));

    if (hDeviceContext != SVE_get_driver_signature())
    {
        VDE_ERR((_T("[VDE:ERR] VDE_Deinit() : Invalid Driver Handle[0x%08x]\r\n"), hDeviceContext));
        return FALSE;
    }

    SVE_deinitialize_video_engine();
    VDE_MSG((_T("[VDE] --VDE_Deinit()\r\n")));

    return TRUE;
}

DWORD
VDE_Open(
    DWORD hDeviceContext,
    DWORD AccessCode,
    DWORD ShareMode
    )
{
    DWORD dwOpenContext;

    VDE_MSG((_T("[VDE] ++VDE_Open(0x%08x, 0x%08x, 0x%08x)\r\n"), hDeviceContext, AccessCode, ShareMode));

    if (hDeviceContext != SVE_get_driver_signature())
    {
        VDE_ERR((_T("[VDE:ERR] VDE_Open() : Invalid Driver Handle[0x%08x]\r\n"), hDeviceContext));
        goto CleanUp;
    }

    dwOpenContext = SVE_add_open_context();
    if (dwOpenContext == 0)
    {
        VDE_ERR((_T("[VDE:ERR] VDE_Open() : Allocating Open Context Failed\r\n")));
        goto CleanUp;
    }

    VDE_MSG((_T("[VDE] --VDE_Open()\r\n")));

    return dwOpenContext;

CleanUp:

    VDE_ERR((_T("[VDE:ERR] --VDE_Init() : Failed\r\n")));

    return 0;
}

BOOL
VDE_Close(
    DWORD hOpenContext
    )
{
    BOOL bRet;

    VDE_MSG((_T("[VDE] ++VDE_Close(0x%08x)\r\n"), hOpenContext));

    bRet = SVE_remove_open_context(hOpenContext);
    if (!bRet)
    {
        VDE_ERR((_T("[VDE:ERR] VDE_Close() : Invalid Open Context !!!\r\n")));
    }

    VDE_MSG((_T("[VDE] --VDE_Close()\r\n")));

    return bRet;
}

DWORD
VDE_Read(
    DWORD hOpenContext,
    LPVOID pBuffer,
    DWORD Count
    )
{
    VDE_MSG((_T("[VDE] VDE_Read(0x%08x, 0x%08x, 0x%08x)\r\n"), hOpenContext, pBuffer, Count));

    return (0);    // End of File
}

DWORD
VDE_Write(
    DWORD hOpenContext,
    LPCVOID pBuffer,
    DWORD Count
    )
{
    VDE_MSG((_T("[VDE] VDE_Write(0x%08x, 0x%08x, 0x%08x)\r\n"), hOpenContext, pBuffer, Count));

    return (0);    // Number of Byte
}

DWORD
VDE_Seek(
    DWORD hOpenContext,
    long Amount,
    WORD Type
    )
{
    VDE_MSG((_T("[VDE] VDE_Seek(0x%08x, 0x%08x, 0x%08x)\r\n"), hOpenContext, Amount, Type));

    return (DWORD)-1;    // Failure
}

BOOL
VDE_IOControl(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    )
{
    SVEngineContext *pCtxt;
    BOOL bRet = TRUE;
    BYTE LocalBuffer[SVEARG_MAX_SIZE];
    PBYTE pBufInLocal = (PBYTE)&LocalBuffer;

    pCtxt = SVE_get_context();

    DEBUGMSG(VDE_ZONE_ENTER, (_T("[VDE] VDE_IOControl(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n"),
                hOpenContext, dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut));

    memset(pBufInLocal, 0, SVEARG_MAX_SIZE);
    if (dwLenIn > SVEARG_MAX_SIZE ||
        !CeSafeCopyMemory(pBufInLocal, pBufIn, dwLenIn))
    {
        RETAILMSG(ZONEMASK_ERROR, (_T("VDE_IOControl: Failed to create a local copy of parameters.\r\n")) );
        return FALSE;        
    }

    EnterCriticalSection(&pCtxt->csProc);

    switch(dwCode)
    {
    case IOCTL_POWER_CAPABILITIES:
    case IOCTL_POWER_GET:
    case IOCTL_POWER_QUERY:
    case IOCTL_POWER_SET:
    case IOCTL_REGISTER_POWER_RELATIONSHIP:
        break;
    case IOCTL_SVE_PM_SET_POWER_ON:
        SVE_video_engine_power_on();
        break;

    case IOCTL_SVE_PM_SET_POWER_OFF:
        //if caller is not kernel mode, do not allow setting power state to off
        if (GetDirectCallerProcessId() != GetCurrentProcessId()){
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        SVE_video_engine_power_off();
        break;
        
    case IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE:
    case IOCTL_SVE_RSC_RELEASE_FIMD_INTERFACE:
    case IOCTL_SVE_RSC_REQUEST_FIMD_WIN0:
    case IOCTL_SVE_RSC_RELEASE_FIMD_WIN0:
    case IOCTL_SVE_RSC_REQUEST_FIMD_WIN1:
    case IOCTL_SVE_RSC_RELEASE_FIMD_WIN1:
    case IOCTL_SVE_RSC_REQUEST_FIMD_WIN2:
    case IOCTL_SVE_RSC_RELEASE_FIMD_WIN2:
    case IOCTL_SVE_RSC_REQUEST_FIMD_WIN3:
    case IOCTL_SVE_RSC_RELEASE_FIMD_WIN3:
    case IOCTL_SVE_RSC_REQUEST_FIMD_WIN4:
    case IOCTL_SVE_RSC_RELEASE_FIMD_WIN4:
    case IOCTL_SVE_RSC_REQUEST_POST:
    case IOCTL_SVE_RSC_RELEASE_POST:
    case IOCTL_SVE_RSC_REQUEST_ROTATOR:
    case IOCTL_SVE_RSC_RELEASE_ROTATOR:
    case IOCTL_SVE_RSC_REQUEST_TVSCALER_TVENCODER:
    case IOCTL_SVE_RSC_RELEASE_TVSCALER_TVENCODER:
        __try
        {
            bRet = SVE_Resource_API_Proc(hOpenContext, SVE_get_api_function_code(dwCode), pBufInLocal, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;

    case IOCTL_SVE_FIMD_SET_INTERFACE_PARAM:
    case IOCTL_SVE_FIMD_SET_OUTPUT_RGBIF:
    case IOCTL_SVE_FIMD_SET_OUTPUT_TV:
    case IOCTL_SVE_FIMD_SET_OUTPUT_ENABLE:
    case IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE:
    case IOCTL_SVE_FIMD_SET_WINDOW_MODE:
    case IOCTL_SVE_FIMD_SET_WINDOW_POSITION:
    case IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER:
    case IOCTL_SVE_FIMD_SET_WINDOW_COLORMAP:
    case IOCTL_SVE_FIMD_SET_WINDOW_ENABLE:
    case IOCTL_SVE_FIMD_SET_WINDOW_DISABLE:
    case IOCTL_SVE_FIMD_SET_WINDOW_BLEND_DISABLE:
    case IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY:
    case IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA:
    case IOCTL_SVE_FIMD_WAIT_FRAME_INTERRUPT:
    case IOCTL_SVE_FIMD_GET_OUTPUT_STATUS:
    case IOCTL_SVE_FIMD_GET_WINDOW_STATUS:
        __try
        {
            bRet = SVE_DispCon_API_Proc(hOpenContext, SVE_get_api_function_code(dwCode), pBufInLocal, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;

    case IOCTL_SVE_POST_SET_PROCESSING_PARAM:
    case IOCTL_SVE_POST_SET_SOURCE_BUFFER:
    case IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER:
    case IOCTL_SVE_POST_SET_DESTINATION_BUFFER:
    case IOCTL_SVE_POST_SET_NEXT_DESTINATION_BUFFER:
    case IOCTL_SVE_POST_SET_PROCESSING_START:
    case IOCTL_SVE_POST_SET_PROCESSING_STOP:
    case IOCTL_SVE_POST_WAIT_PROCESSING_DONE:
    case IOCTL_SVE_POST_GET_PROCESSING_STATUS:
        __try
        {
            bRet = SVE_Post_API_Proc(hOpenContext, SVE_get_api_function_code(dwCode), pBufInLocal, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;


    case IOCTL_SVE_LOCALPATH_SET_WIN0_START:
    case IOCTL_SVE_LOCALPATH_SET_WIN0_STOP:
    case IOCTL_SVE_LOCALPATH_SET_WIN1_START:
    case IOCTL_SVE_LOCALPATH_SET_WIN1_STOP:
    case IOCTL_SVE_LOCALPATH_SET_WIN2_START:
    case IOCTL_SVE_LOCALPATH_SET_WIN2_STOP:
        __try
        {
            bRet = SVE_LocalPath_API_Proc(hOpenContext, SVE_get_api_function_code(dwCode), pBufInLocal, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;

    case IOCTL_SVE_ROTATOR_SET_OPERATION_PARAM:
    case IOCTL_SVE_ROTATOR_SET_SOURCE_BUFFER:
    case IOCTL_SVE_ROTATOR_SET_DESTINATION_BUFFER:
    case IOCTL_SVE_ROTATOR_SET_OPERATION_START:
    case IOCTL_SVE_ROTATOR_SET_OPERATION_STOP:
    case IOCTL_SVE_ROTATOR_WAIT_OPERATION_DONE:
    case IOCTL_SVE_ROTATOR_GET_STATUS:
        __try
        {
            bRet = SVE_Rotator_API_Proc(hOpenContext, SVE_get_api_function_code(dwCode), pBufInLocal, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;

    case IOCTL_SVE_TVSC_SET_PROCESSING_PARAM:
    case IOCTL_SVE_TVSC_SET_SOURCE_BUFFER:
    case IOCTL_SVE_TVSC_SET_NEXT_SOURCE_BUFFER:
    case IOCTL_SVE_TVSC_SET_DESTINATION_BUFFER:
    case IOCTL_SVE_TVSC_SET_NEXT_DESTINATION_BUFFER:
    case IOCTL_SVE_TVSC_SET_PROCESSING_START:
    case IOCTL_SVE_TVSC_SET_PROCESSING_STOP:
    case IOCTL_SVE_TVSC_WAIT_PROCESSING_DONE:
    case IOCTL_SVE_TVSC_GET_PROCESSING_STATUS:
        __try
        {
            bRet = SVE_TVScaler_API_Proc(hOpenContext, SVE_get_api_function_code(dwCode), pBufInLocal, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;

    case IOCTL_SVE_TVENC_SET_INTERFACE_PARAM:
    case IOCTL_SVE_TVENC_SET_ENCODER_ON:
    case IOCTL_SVE_TVENC_SET_ENCODER_OFF:
    case IOCTL_SVE_TVENC_GET_INTERFACE_STATUS:
        __try
        {
            bRet = SVE_TVEncoder_API_Proc(hOpenContext, SVE_get_api_function_code(dwCode), pBufInLocal, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;
    case IOCTL_SVE_FIMD_VSYNC_ENABLE:

        __try
        {
            Disp_VSync_Enable();
            bRet=TRUE;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_FIMD_VSYNC_ENABLE\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;


    case IOCTL_SVE_FIMD_GET_FLIPSTATUS:

        __try
        {
            if(Disp_GetFlipStatus())
            {
                bRet=TRUE;
            }
            else
            {
                bRet=FALSE;
            }
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RETAILMSG( 1, ( _T("VDE_IOControl: exception in IOCTL_SVE_FIMD_GET_FLIPSTATUS\n")) );
            LeaveCriticalSection(&pCtxt->csProc);
            return FALSE;
        }
        break;


    case IOCTL_SVE_PM_GET_POWER_STATUS:

    default:
        VDE_ERR((_T("[VDE:ERR] VDE_IOControl() : Unknown IOCTL [0x%08x]\r\n"), dwCode));
        SetLastError (ERROR_INVALID_ACCESS);
        bRet = FALSE;
        break;
    }

    LeaveCriticalSection(&pCtxt->csProc);

    DEBUGMSG(VDE_ZONE_ENTER, (_T("[VDE] --VDE_IOControl()\r\n")));

    return bRet;
}

