//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
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

*/


#include "JPGDriver.h"
#include "JPGMem.h"
#include "JPGMisc.h"
#include "JPGOpr.h"
#include <s3c6410_syscon.h>
#include <s3c6410_base_regs.h>
#include <pmplatform.h>
#include <bsp_cfg.h>


HANDLE      hPwrControl;
S3C6410_SYSCON_REG *s6410PWR = NULL;
UINT8       instanceNo = 0;
BOOL        PowerChange = FALSE;

#if DEBUG
#define ZONE_INIT              DEBUGZONE(0)

DBGPARAM dpCurSettings =                            \
{                                                   \
    TEXT("JPEG_Driver"),                            \
    {                                               \
        TEXT("Init"),                 /* 0  */      \
    },                                              \
    (0x0001)                                        \
};
#endif

static void JPGPowerControl(BOOL bOnOff);
static void Delay(UINT32 count);
static BOOL JPGSetClkDiv(int divider);
static BOOL AllocateAndMapToUserMemory( HANDLE hCallerProcess,
                                        LPVOID pAddrKernelSpace,
                                        DWORD dwSize,
                                        LPVOID *ppAddrUserSpace);

/*----------------------------------------------------------------------------
*Function: JPG_Init

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize JPEG Hardware
-----------------------------------------------------------------------------*/
DWORD
JPG_Init(
    DWORD dwContext
    )
{
    S3C6410_JPG_CTX *JPGRegCtx;
    HANDLE            h_Mutex;
    DWORD            ret;

    printD("DD::JPG_Init\n");

    // PWM clock Virtual alloc
    s6410PWR = (S3C6410_SYSCON_REG *)Phy2VirAddr(S3C6410_BASE_REG_PA_SYSCON, sizeof(S3C6410_SYSCON_REG));
    if (s6410PWR == NULL)
    {
        RETAILMSG(1,(TEXT("DD:: For s6410PWR: MmMapIoSpace failed!\r\n")));
        return FALSE;;
    }

    hPwrControl = CreateFile( L"PWC0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == hPwrControl )
    {
        RETAILMSG(1, (TEXT("DD:: JPEG - PWC0 Open Device Failed\r\n")));
        FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));
        return FALSE;
    }

    if(JPGSetClkDiv(JPG_CLOCK_DIVIDER_RATIO_QUARTER) == FALSE)
    {
        RETAILMSG(1, (TEXT("DD:: JPEG - Set Clock Divider Failed\r\n")));
        CloseHandle(hPwrControl);
        FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));
        return FALSE;
    }

    // Mutex initialization
    h_Mutex = CreateJPGmutex();
    if (h_Mutex == NULL)
    {
        RETAILMSG(1, (TEXT("DD::JPG Mutex Initialize error : %d \r\n"),GetLastError()));
        CloseHandle(hPwrControl);
        FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));
        return FALSE;
    }

    ret = LockJPGMutex();
    if(!ret)
    {
        RETAILMSG(1, (TEXT("DD::JPG Mutex Lock Fail\r\n")));
        DeleteJPGMutex();
        CloseHandle(hPwrControl);
        FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));
        return FALSE;
    }

    // Register/Memory initialization
    JPGRegCtx = (S3C6410_JPG_CTX *)malloc(sizeof(S3C6410_JPG_CTX));
    if (!JPGRegCtx)
    {
        RETAILMSG(1, (TEXT("DD::JPG Memory Context Allocation Fail\r\n")));
        UnlockJPGMutex();
        DeleteJPGMutex();
        CloseHandle(hPwrControl);
        FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));
        return FALSE;
    }
    memset(JPGRegCtx, 0x00, sizeof(S3C6410_JPG_CTX));

    if( !JPGMemMapping(JPGRegCtx) )
    {
        RETAILMSG(1, (TEXT("DD::JPEG-HOST-MEMORY Initialize error : %d \r\n"),GetLastError()));
        free(JPGRegCtx);
        UnlockJPGMutex();
        DeleteJPGMutex();
        CloseHandle(hPwrControl);
        FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));
        return FALSE;
    }
    else
    {
        if(!JPGBuffMapping(JPGRegCtx))
        {
            RETAILMSG(1, (TEXT("DD::JPEG-DATA-MEMORY Initialize error : %d \r\n"),GetLastError()));
            JPGMemFree(JPGRegCtx);
            free(JPGRegCtx);
            UnlockJPGMutex();
            DeleteJPGMutex();
            CloseHandle(hPwrControl);
            FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));
            return FALSE;
        }
    }

    instanceNo = 0;

    UnlockJPGMutex();
    return (DWORD)JPGRegCtx;
}


/*----------------------------------------------------------------------------
*Function: JPG_DeInit

*Parameters:         InitHandle        :
*Return Value:        True/False
*Implementation Notes: Deinitialize JPEG Hardware
-----------------------------------------------------------------------------*/
BOOL
JPG_Deinit(
    DWORD InitHandle
    )
{
    DWORD    ret;
    S3C6410_JPG_CTX *JPGRegCtx;

    printD("DD::JPG_Deinit\n");

    JPGRegCtx = (S3C6410_JPG_CTX *)InitHandle;
    if(!JPGRegCtx)
    {
        RETAILMSG(1, (TEXT("DD::JPG Invalid Input Handle\r\n")));
        return FALSE;
    }

    ret = LockJPGMutex();
    if(!ret)
    {
        RETAILMSG(1, (TEXT("DD::JPG Mutex Lock Fail\r\n")));
        return FALSE;
    }

    JPGBuffFree(JPGRegCtx);
    JPGMemFree(JPGRegCtx);
    free(JPGRegCtx);
    UnlockJPGMutex();

    DeleteJPGMutex();
    if (hPwrControl)
        CloseHandle(hPwrControl);
    if (s6410PWR)
        FreeVirAddr(s6410PWR, sizeof(S3C6410_SYSCON_REG));

    return TRUE;
}


/*----------------------------------------------------------------------------
*Function: JPG_Open

*Parameters:         InitHandle        :Handle to JPEG  context
                    dwAccess        :
                    dwShareMode        :File share mode of JPEG
*Return Value:        This function returns a handle that identifies the
                    open context of JPEG  to the calling application.
*Implementation Notes: Opens JPEG CODEC device for reading, writing, or both
-----------------------------------------------------------------------------*/
DWORD
JPG_Open(
    DWORD InitHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    S3C6410_JPG_CTX *JPGRegCtx;
    DWORD    ret;

    printD("DD::JPG_open \r\n");

    JPGPowerControl(TRUE);

    JPGRegCtx = (S3C6410_JPG_CTX *)InitHandle;
    if(!JPGRegCtx)
    {
        RETAILMSG(1, (TEXT("DD::JPG Invalid Input Handle\r\n")));
        return FALSE;
    }

    ret = LockJPGMutex();
    if(!ret)
    {
        RETAILMSG(1, (TEXT("DD::JPG Mutex Lock Fail\r\n")));
        return FALSE;
    }

    // check the number of instance
    if(instanceNo > MAX_INSTANCE_NUM)
    {
        RETAILMSG(1, (TEXT("DD::Instance Number error-JPEG is running\r\n")));
        UnlockJPGMutex();
        return FALSE;
    }

    instanceNo++;
    PowerChange = FALSE;

    printD("instanceNo : %d\n", instanceNo);

    UnlockJPGMutex();
    return (DWORD)JPGRegCtx;
}


/*----------------------------------------------------------------------------
*Function: JPG_Close

*Parameters:         OpenHandle        :
*Return Value:        True/False
*Implementation Notes: This function closes the device context identified by
                        OpenHandle
-----------------------------------------------------------------------------*/
BOOL
JPG_Close(
    DWORD OpenHandle
    )
{
    DWORD    ret;
    S3C6410_JPG_CTX *JPGRegCtx;
    BOOL    result = TRUE;

    printD("DD::JPG_Close\n");
    
    if(PowerChange == TRUE)
    {
        RETAILMSG(1, (TEXT("DD::Power state is changed after open\r\n")));
        return FALSE;
    }

    JPGRegCtx = (S3C6410_JPG_CTX *)OpenHandle;
    if(!JPGRegCtx)
    {
        RETAILMSG(1, (TEXT("DD::JPG Invalid Input Handle\r\n")));
        return FALSE;
    }

    ret = LockJPGMutex();
    if(!ret)
    {
        RETAILMSG(1, (TEXT("DD::JPG Mutex Lock Fail\r\n")));
        return FALSE;
    }
    
    if((--instanceNo) < 0)
        instanceNo = 0;

    printD("JPGRegCtx->callerProcess : 0x%x\n", JPGRegCtx->callerProcess);

    if(JPGRegCtx->strUserBuf != NULL)
    {
        printD("decommit strUserBuf\n");
        if(!VirtualFreeEx(JPGRegCtx->callerProcess,    // HANDLE hProcess
                          JPGRegCtx->strUserBuf,
                          0, 
                          MEM_RELEASE))
        {
            RETAILMSG(1, (L"DD::JPG VirtualFreeEx(strUserBuf) returns FALSE.\n"));
        }
        JPGRegCtx->strUserBuf = NULL;
    }

    if(JPGRegCtx->strUserThumbBuf != NULL)
    {
        printD("decommit strUserThumbBuf\n");
        if(!VirtualFreeEx(JPGRegCtx->callerProcess,    // HANDLE hProcess
                          JPGRegCtx->strUserThumbBuf,
                          0, 
                          MEM_RELEASE))
        {
            RETAILMSG(1, (L"DD::JPG  VirtualFreeEx(strUserThumbBuf) returns FALSE.\n"));
        }
        JPGRegCtx->strUserThumbBuf = NULL;
    }

    if(JPGRegCtx->frmUserBuf != NULL)
    {
        printD("decommit frmUserBuf\n");
        if(!VirtualFreeEx(JPGRegCtx->callerProcess,    // HANDLE hProcess
                          JPGRegCtx->frmUserBuf,
                          0, 
                          MEM_RELEASE))
        {
            RETAILMSG(1, (L"DD::JPG  VirtualFreeEx(STRM_BUF) returns FALSE.\n"));
        }
        JPGRegCtx->frmUserBuf = NULL;
    }

    if(JPGRegCtx->frmUserThumbBuf != NULL)
    {
        printD("decommit frmUserThumbBuf\n");
        if(!VirtualFreeEx(JPGRegCtx->callerProcess,    // HANDLE hProcess
                          JPGRegCtx->frmUserThumbBuf,
                          0, 
                          MEM_RELEASE))
        {
            RETAILMSG(1, (L"DD::JPG  VirtualFreeEx(frmUserThumbBuf) returns FALSE.\n"));
        }
        JPGRegCtx->frmUserThumbBuf = NULL;
    }

    if(JPGRegCtx->rgbBuf != NULL)
    {
        printD("decommit rgbBuf\n");
        if(!VirtualFreeEx(JPGRegCtx->callerProcess,    // HANDLE hProcess
                                  JPGRegCtx->rgbBuf,
                                  0, 
                                  MEM_RELEASE))
        {
            RETAILMSG(1, (L"DD::JPG  VirtualFreeEx(rgbBuf) returns FALSE.\n"));
        }
        JPGRegCtx->rgbBuf = NULL;
    }

    UnlockJPGMutex();

    JPGPowerControl(FALSE);
    return TRUE;
}


/*----------------------------------------------------------------------------
*Function: JPG_IOControl

*Parameters:         OpenHandle        :
                    dwIoControlCode    :
*Return Value:        True/False
*Implementation Notes: JPEG_IOControl sends commands to initiate different
*                       operations like Init,Decode and Deinit.The test
*                       application uses the DeviceIOControl function to
*                       specify an operation to perform
-----------------------------------------------------------------------------*/
BOOL
JPG_IOControl(
    DWORD OpenHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned
    )
{
    S3C6410_JPG_CTX *JPGRegCtx;
    JPG_DEC_PROC_PARAM *DecReturn;
    JPG_ENC_PROC_PARAM *EncParam;
    
    BOOL    result = TRUE;
    DWORD   ret;
    DWORD   dwSize = 0;
    PVOID   pMarshalledBuf = NULL;

    printD("DD::IOCTL\n");
    
    if(PowerChange == TRUE)
    {
        RETAILMSG(1, (TEXT("DD::Power state is changed after open\r\n")));
        return FALSE;
    }

    JPGRegCtx = (S3C6410_JPG_CTX *)OpenHandle;
    if(!JPGRegCtx)
    {
        RETAILMSG(1, (TEXT("DD::JPG Invalid Input Handle\r\n")));
        return FALSE;
    }

    ret = LockJPGMutex();
    if(!ret)
    {
        RETAILMSG(1, (TEXT("DD::JPG Mutex Lock Fail\r\n")));
        return FALSE;
    }

    switch ( dwIoControlCode )
    {
    case IOCTL_JPG_DECODE:
        printD("DD::IOCTL_JPEG_DECODE\n");

        // NOTE: Using pBytesReturned to pass in parameters is incorrect
        // This is not being changed right now due to the problem of breaking existing callers
        //
        // For now, at least check if the caller has access to the passed in buffer 
        // and duplicate it for use
        if(FAILED(CeOpenCallerBuffer(&pMarshalledBuf, pBytesReturned, sizeof(JPG_DEC_PROC_PARAM), ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("JPG_IOControl: CeOpenCallerBuffer failed in IOCTL_JPG_DECODE.\r\n")));
            result = FALSE;
            break;
        }

        DecReturn = (JPG_DEC_PROC_PARAM *)pMarshalledBuf;

        result = decodeJPG(JPGRegCtx, DecReturn);
        printD("DD::width : %d hegiht : %d size : %d\n", DecReturn->width, DecReturn->height, DecReturn->dataSize);

        if(FAILED(CeCloseCallerBuffer(pMarshalledBuf, pBytesReturned, sizeof(JPG_DEC_PROC_PARAM), ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("JPG_IOControl: CeCloseCallerBuffer failed in IOCTL_JPG_DECODE.\r\n")));
            result = FALSE;
            break;
        }

        break;

    case IOCTL_JPG_ENCODE:
        printD("DD::IOCTL_JPEG_ENCODE\n");

        // NOTE: Using pBytesReturned to pass in parameters is incorrect
        // This is not being changed right now due to the problem of breaking existing callers
        //
        // For now, at least check if the caller has access to the passed in buffer 
        // and duplicate it for use
        if(FAILED(CeOpenCallerBuffer(&pMarshalledBuf, pBytesReturned, sizeof(JPG_ENC_PROC_PARAM), ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("JPG_IOControl: CeOpenCallerBuffer failed in IOCTL_JPG_ENCODE.\r\n")));
            result = FALSE;
            break;
        }
        
        EncParam = (JPG_ENC_PROC_PARAM *)pMarshalledBuf;
        
        printD("DD::width : %d hegiht : %d enctype : %d quality : %d\n",
            EncParam->width, EncParam->height, EncParam->encType, EncParam->quality);

        result = encodeJPG(JPGRegCtx, EncParam);
        
        printD("DD::encoded file size : %d\n", EncParam->fileSize);

        if(FAILED(CeCloseCallerBuffer(pMarshalledBuf, pBytesReturned, sizeof(JPG_ENC_PROC_PARAM), ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("JPG_IOControl: CeCloseCallerBuffer failed in IOCTL_JPG_ENCODE.\r\n")));
            result = FALSE;
            break;
        }
        
        break;

    case IOCTL_JPG_GET_STRBUF:
        printD("DD::JPG IOCTL_JPG_GET_STRBUF\n");

        if (!pOutBuf || nOutBufSize < sizeof(UINT*))
        {
            RETAILMSG(1, (TEXT("DD::JPG IOCTL_JPG_GET_STRBUF Error: invalid parameter.\r\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            result = FALSE;
            break;
        }

        if(JPGRegCtx->strUserBuf == NULL)
        {
            JPGRegCtx->callerProcess = (HANDLE) GetDirectCallerProcessId();
            dwSize = JPG_STREAM_BUF_SIZE;
            if (dwSize > IMAGE_JPG_BUFFER_SIZE) // Upper bound the size
            {
                JPGRegCtx->strUserBuf = NULL;
            }
            else
            {
                result = AllocateAndMapToUserMemory(JPGRegCtx->callerProcess, 
                                                    (LPVOID)JPGRegCtx->v_pJPGData_Buff, 
                                                    dwSize, 
                                                    (LPVOID*)&JPGRegCtx->strUserBuf);
            }
            
            if (JPGRegCtx->strUserBuf == NULL)
            {
                RETAILMSG(1, (TEXT("DD::JPG Memory Allocation Fail\r\n")));
                result = FALSE;
            }
        }

        printD("DD::strUserBuf : 0x%x CallerProcessID : 0x%x\n", JPGRegCtx->strUserBuf, JPGRegCtx->callerProcess);
        
        __try
        {
            *((UINT *)pOutBuf) = (UINT) JPGRegCtx->strUserBuf;
        }
         __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            RETAILMSG(1, (TEXT("DD:JPG IOCTL_JPG_GET_STRBUF exception\r\n")));
            result = FALSE;
        }
         
        break;

    case IOCTL_JPG_GET_THUMB_STRBUF:
        printD("DD::IOCTL_JPG_GET_THUMB_STRBUF\n");

        if (!pOutBuf || nOutBufSize < sizeof(UINT*))
        {
            RETAILMSG(1, (TEXT("DD::JPG IOCTL_JPG_GET_THUMB_STRBUF Error: invalid parameter.\r\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            result = FALSE;
            break;
        }

        if(JPGRegCtx->strUserThumbBuf == NULL)
        {
            JPGRegCtx->callerProcess = (HANDLE) GetDirectCallerProcessId();
            dwSize = JPG_STREAM_THUMB_BUF_SIZE;
            if (dwSize > IMAGE_JPG_BUFFER_SIZE) // Upper bound the size
            {
                JPGRegCtx->strUserBuf = NULL;
            }
            else
            {
                result = AllocateAndMapToUserMemory(JPGRegCtx->callerProcess, 
                                                    (LPVOID)(JPGRegCtx->v_pJPGData_Buff+ JPG_STREAM_BUF_SIZE), 
                                                    dwSize, 
                                                    (LPVOID*)&JPGRegCtx->strUserThumbBuf);
            }
            if (JPGRegCtx->strUserThumbBuf == NULL)
            {
                RETAILMSG(1, (TEXT("DD::JPG Memory Allocation Fail\r\n")));
                result = FALSE;
            }
        }

        printD("DD::strUserThumbBuf : 0x%x CallerProcessID : 0x%x\n", JPGRegCtx->strUserThumbBuf, JPGRegCtx->callerProcess);
        
        __try
        {
            *((UINT *)pOutBuf) = (UINT) JPGRegCtx->strUserThumbBuf;
        }
         __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            RETAILMSG(1, (TEXT("DD:JPG IOCTL_JPG_GET_THUMB_STRBUF exception\r\n")));
            result = FALSE;
        }

        break;

    case IOCTL_JPG_GET_FRMBUF:
        printD("DD::IOCTL_JPG_GET_FRMBUF\n");

        if (!pOutBuf || nOutBufSize < sizeof(UINT*))
        {
            RETAILMSG(1, (TEXT("DD::JPG IOCTL_JPG_GET_FRMBUF Error: invalid parameter.\r\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            result = FALSE;
            break;
        }

        if(JPGRegCtx->frmUserBuf == NULL)
        {
            JPGRegCtx->callerProcess = (HANDLE) GetDirectCallerProcessId();
            dwSize = JPG_FRAME_BUF_SIZE;
            if (dwSize > IMAGE_JPG_BUFFER_SIZE) // Upper bound the size
            {
                JPGRegCtx->strUserBuf = NULL;
            }
            else
            {
                result = AllocateAndMapToUserMemory(JPGRegCtx->callerProcess, 
                                                    (LPVOID)(JPGRegCtx->v_pJPGData_Buff + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE), 
                                                    dwSize, 
                                                    (LPVOID*)&JPGRegCtx->frmUserBuf);
            }
            if (JPGRegCtx->frmUserBuf == NULL)
            {
                RETAILMSG(1, (TEXT("DD::JPG Memory Allocation Fail\r\n")));
                result = FALSE;
            }
        }

        printD("DD::frmUserBuf : 0x%x CallerProcessID : 0x%x\n", JPGRegCtx->frmUserBuf, JPGRegCtx->callerProcess);
        
        __try
        {
            *((UINT *)pOutBuf) = (UINT) JPGRegCtx->frmUserBuf;
        }
         __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            RETAILMSG(1, (TEXT("DD:JPG IOCTL_JPG_GET_FRMBUF exception\r\n")));
            result = FALSE;
        }
         
        break;

    case IOCTL_JPG_GET_PHY_FRMBUF:

        if (!pOutBuf || nOutBufSize < sizeof(UINT*))
        {
            RETAILMSG(1, (TEXT("DD::JPG IOCTL_JPG_GET_PHY_FRMBUF Error: invalid parameter.\r\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            result = FALSE;
            break;
        }
        
        __try
        {
            *((UINT *)pOutBuf) = (UINT)JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE;
            printD("IOCTL_JPG_GET_PHY_FRMBUF : 0x%x\n", JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE);
        }
         __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            RETAILMSG(1, (TEXT("DD:JPG IOCTL_JPG_GET_PHY_FRMBUF exception\r\n")));
            result = FALSE;
        }

        break;

    case IOCTL_JPG_GET_THUMB_FRMBUF:
        printD("DD::IOCTL_JPG_GET_THUMB_FRMBUF\n");

        if (!pOutBuf || nOutBufSize < sizeof(UINT*))
        {
            RETAILMSG(1, (TEXT("DD::JPG IOCTL_JPG_GET_THUMB_FRMBUF Error: invalid parameter.\r\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            result = FALSE;
            break;
        }        

        if(JPGRegCtx->frmUserThumbBuf == NULL)
        {
            JPGRegCtx->callerProcess = (HANDLE) GetDirectCallerProcessId();
            dwSize = JPG_FRAME_THUMB_BUF_SIZE;
            if (dwSize > IMAGE_JPG_BUFFER_SIZE) // Upper bound the size
            {
                JPGRegCtx->strUserBuf = NULL;
            }
            else
            {
                result = AllocateAndMapToUserMemory(JPGRegCtx->callerProcess, 
                                                    (LPVOID)(JPGRegCtx->v_pJPGData_Buff+ JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE + JPG_FRAME_BUF_SIZE),
                                                    dwSize, 
                                                    (LPVOID*)&JPGRegCtx->frmUserThumbBuf);
            }
            if (JPGRegCtx->frmUserThumbBuf == NULL)
            {
                RETAILMSG(1, (TEXT("DD::JPG Memory Allocation Fail\r\n")));
                result = FALSE;
            }
        }

        printD("DD::frmUserBuf : 0x%x CallerProcessID : 0x%x\n", JPGRegCtx->frmUserThumbBuf, JPGRegCtx->callerProcess);
        
        __try
        {
            *((UINT *)pOutBuf) = (UINT) JPGRegCtx->frmUserThumbBuf;
        }
         __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            RETAILMSG(1, (TEXT("DD:JPG IOCTL_JPG_GET_THUMB_FRMBUF exception\r\n")));
            result = FALSE;
        }
         
        break;

    case IOCTL_JPG_GET_RGBBUF:
        printD("DD::IOCTL_JPG_GET_RGBBUF\n");

        if (!pOutBuf || nOutBufSize < sizeof(UINT*))
        {
            RETAILMSG(1, (TEXT("DD::JPG IOCTL_JPG_GET_RGBBUF Error: invalid parameter.\r\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            result = FALSE;
            break;
        }

        if(JPGRegCtx->rgbBuf == NULL)
        {
            JPGRegCtx->callerProcess = (HANDLE) GetDirectCallerProcessId();
            dwSize = JPG_RGB_BUF_SIZE;
            if (dwSize > IMAGE_JPG_BUFFER_SIZE) // Upper bound the size
            {
                JPGRegCtx->strUserBuf = NULL;
            }
            else
            {
                result = AllocateAndMapToUserMemory(JPGRegCtx->callerProcess, 
                                                    (LPVOID)(JPGRegCtx->v_pJPGData_Buff+ JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE+ JPG_FRAME_BUF_SIZE + JPG_FRAME_THUMB_BUF_SIZE),
                                                    dwSize, 
                                                    (LPVOID*)&JPGRegCtx->rgbBuf);
            }
            if (JPGRegCtx->rgbBuf == NULL)
            {
                RETAILMSG(1, (TEXT("DD::JPG Memory Allocation Fail\r\n")));
                result = FALSE;
            }
        }

        printD("DD::frmUserBuf : 0x%x CallerProcessID : 0x%x\n", JPGRegCtx->rgbBuf, JPGRegCtx->callerProcess);
        
        __try
        {
            *((UINT *)pOutBuf) = (UINT) JPGRegCtx->rgbBuf;
        }
         __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            RETAILMSG(1, (TEXT("DD:JPG IOCTL_JPG_GET_RGBBUF exception\r\n")));
            result = FALSE;
        }
         
        break;

    case IOCTL_JPG_GET_PHY_RGBBUF:

        if (!pOutBuf || nOutBufSize < sizeof(UINT*))
        {
            RETAILMSG(1, (TEXT("DD::JPG IOCTL_JPG_GET_PHY_RGBBUF Error: invalid parameter.\r\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            result = FALSE;
            break;
        }
        
        __try
        {
            *((UINT *)pOutBuf) = (UINT)JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE+ JPG_FRAME_BUF_SIZE + JPG_FRAME_THUMB_BUF_SIZE;
            printD("IOCTL_JPG_GET_PHY_RGBBUF : 0x%x\n", JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE + JPG_FRAME_BUF_SIZE + JPG_FRAME_THUMB_BUF_SIZE);
        }
         __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            RETAILMSG(1, (TEXT("DD:JPG IOCTL_JPG_GET_PHY_RGBBUF exception\r\n")));
            result = FALSE;
        }

        break;

    default :
        RETAILMSG(1, (TEXT("DD::JPG Invalid IOControl\r\n")));
    }


    UnlockJPGMutex();
    return result;
}

/*----------------------------------------------------------------------------
*Function: JPG_Write

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize JPEG Hardware
-----------------------------------------------------------------------------*/
DWORD
JPG_Write(
    DWORD OpenHandle,
    LPCVOID pBuffer,
    DWORD dwNumBytes
    )
{
    printD("DD::JPEG_Write \n");
    return 1;
}

/*----------------------------------------------------------------------------
*Function: JPEG_PowerUp

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize JPEG Hardware
-----------------------------------------------------------------------------*/
void
JPG_PowerUp(
    DWORD InitHandle
    )
{
    printD("DD::JPEG_PowerUp \n");
}

/*----------------------------------------------------------------------------
*Function: JPEG_PowerDown

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize JPEG Hardware
-----------------------------------------------------------------------------*/
void
JPG_PowerDown(
    DWORD InitHandle
    )
{
    printD("DD::JPEG_PowerDown(instanceNo : %d \n", instanceNo);

    if(instanceNo > 0)
    {
        PowerChange = TRUE;
        Delay(MAX_PROCESSING_THRESHOLD);
    }

    JPGPowerControl(FALSE);
}


BOOL AllocateAndMapToUserMemory( HANDLE hCallerProcess,
                                            LPVOID pAddrKernelSpace,
                                            DWORD dwSize,
                                            LPVOID *ppAddrUserSpace)
{
    BOOL fResult = FALSE;
    LPVOID pUserAddress = NULL;

    pUserAddress = VirtualAllocEx(hCallerProcess, 
                                    NULL, 
                                    dwSize, 
                                    MEM_RESERVE, 
                                    PAGE_NOACCESS);
    if (pUserAddress == NULL)
    {
        RETAILMSG(1, (TEXT("DD::JPG Memory VirtualAlloc Fail.  Error = %d\r\n"), GetLastError()));
        goto Exit;
    }
    if (!VirtualCopyEx(hCallerProcess,           
                        pUserAddress,
                        (HANDLE) GetCurrentProcessId(),
                        pAddrKernelSpace,    
                        dwSize,
                        PAGE_READWRITE | PAGE_NOCACHE))
    {
        RETAILMSG(1, (TEXT("DD::JPG Memory VirtualCopyEx Fail. Error = %d\r\n"), GetLastError()));
        goto Exit;
    }                    

    *(ppAddrUserSpace) = pUserAddress;
    pUserAddress = NULL;
    fResult = TRUE;

Exit:
    if (pUserAddress)
    {
        if(!VirtualFreeEx(hCallerProcess,
                          pUserAddress,
                          0, 
                          MEM_RELEASE))
        {
            RETAILMSG(1, (L"DD::JPG VirtualFreeEx(error case) returns FALSE.\n"));
        }
    }
    
    return fResult;
}

/*----------------------------------------------------------------------------
*Function: JPGPowerControl
*Implementation Notes: JPEG Power on/off
-----------------------------------------------------------------------------*/
static void
JPGPowerControl(
    BOOL bOnOff
    )
{
    DWORD dwIPIndex = PWR_IP_JPEG;
    DWORD dwBytes;
    static int isOn = 0;

    printD("DD::JPEG Power Control\n");
    
    // JPEG clock
    if (!bOnOff)
    {
        if(isOn == 1)
        {
            printD("JPEG powerOFF\n");
            isOn = 0;

            s6410PWR->HCLK_GATE &= ~(JPG_1BIT_MASK << JPG_HCLK_JPEG_BIT); // JPEG clock disable
            s6410PWR->SCLK_GATE &= ~(JPG_1BIT_MASK << JPG_SCLK_JPEG_BIT); // JPEG clock disable

            if ( !DeviceIoControl(hPwrControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(1, (TEXT("DD::JPG IOCTL_PWRCON_SET_POWER_OFF Failed\r\n")));
            }
        }
    }
    else
    {
        if(isOn == 0)
        {
            printD("JPEG powerON\n");
            isOn = 1;

            if ( !DeviceIoControl(hPwrControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(1, (TEXT("DD::JPG IOCTL_PWRCON_SET_POWER_ON Failed\r\n")));
            }

            s6410PWR->HCLK_GATE |= (JPG_1BIT_MASK << JPG_HCLK_JPEG_BIT); // JPEG clock enable
            s6410PWR->SCLK_GATE |= (JPG_1BIT_MASK << JPG_SCLK_JPEG_BIT); // JPEG clock enable
        }
    }
}


/*----------------------------------------------------------------------------
*Function: JPGSetClkDiv
*Implementation Notes: set JPG clock
-----------------------------------------------------------------------------*/
static BOOL
JPGSetClkDiv(
    int divider
    )
{
    if ((divider < 1) || (divider > 16))
    {
        RETAILMSG(1, (L"JPGSetClkDiv: JPG clock divider must be 1 ~ 16.\n\r"));
        return FALSE;
    }

    // S3C6410 JPEG clock ratio must be odd value.
    // If you want to set quater clock(1/4) to CLKJPG, you have to set 3(odd value) to JPEG_RATIO field.
    // As below calcuration, this function's parameter must be 4.
    s6410PWR->CLK_DIV0 = (s6410PWR->CLK_DIV0 & ~(JPG_4BIT_MASK << JPG_JPEG_RATIO_BIT)) | ((divider - 1) << JPG_JPEG_RATIO_BIT);

    return TRUE;
}

/*----------------------------------------------------------------------------
*Function: Delay
*Implementation Notes: delay during count milisecond
-----------------------------------------------------------------------------*/
static void
Delay(
    UINT32 count
    )
{
    volatile int i, j = 0;
    volatile static int loop = APLL_CLK/100000;

    for(;count > 0;count--)
        for(i=0;i < loop; i++)
        {
            j++;
        }
}

/*----------------------------------------------------------------------------
*Function: JPEG_DllMain

*Parameters:         DllInstance        :
                    Reason            :
                    Reserved        :
*Return Value:        True/False
*Implementation Notes: Entry point for JPEG.dll
-----------------------------------------------------------------------------*/
BOOL WINAPI
JPG_DllMain(
    HINSTANCE DllInstance,
    DWORD Reason,
    LPVOID Reserved
    )
{
    switch(Reason)
    {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER(DllInstance);
            break;
    }

    return TRUE;
}

