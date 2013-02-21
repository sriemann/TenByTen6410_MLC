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

Module Name:    TVScalerAPI.c

Abstract:       Implementation of Video Driver
                This module handle TVScaler

Functions:


Notes:


--*/

#include <bsp.h>
#include "SVEngine.h"

BOOL SVE_TVScaler_API_Proc(
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
    SVEnginePowerContext *pPMCtxt;
    BOOL bRet = TRUE;

    //VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\r\n"),
    //            hOpenContext, dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut));
    VDE_MSG((_T("[VDE] ++SVE_TVScaler_API_Proc()\n\r")));

    pCtxt = SVE_get_context();
    pPMCtxt = SVE_get_power_context();

    //-------------------------------------------------------------
    // Check OpenContext of Caller have the right to access to H/W Resource
    //-------------------------------------------------------------
    switch(dwCode)
    {
    case SVE_TVSC_SET_PROCESSING_PARAM:
    case SVE_TVSC_SET_SOURCE_BUFFER:
    case SVE_TVSC_SET_NEXT_SOURCE_BUFFER:
    case SVE_TVSC_SET_DESTINATION_BUFFER:
    case SVE_TVSC_SET_NEXT_DESTINATION_BUFFER:
    case SVE_TVSC_SET_PROCESSING_START:
    case SVE_TVSC_SET_PROCESSING_STOP:
    case SVE_TVSC_WAIT_PROCESSING_DONE:
    case SVE_TVSC_GET_PROCESSING_STATUS:
        bRet = SVE_resource_compare_TVScaler_TVEncoder(hOpenContext);
        if (!bRet)
        {
            VDE_ERR((_T("[VDE:ERR] SVE_TVScaler_API_Proc(0x%08x) : No Right to Access to H/W Resource\r\n"), dwCode));
            goto CleanUp;
        }
        break;
    default:
        bRet = FALSE;
        DEBUGMSG(VDE_ZONE_ERROR, (TEXT("[VDE:ERR] Invalid IOCTL code\n")));        
        goto CleanUp;
        break;
    }

    //--------------------------------
    // Processing IOCTL for TV Scaler
    //--------------------------------

    switch(dwCode)
    {
        case SVE_TVSC_SET_PROCESSING_PARAM:
        {
            SVEARG_TVSC_PARAMETER *pArg;

            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_PROCESSING_PARAM\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_TVSC_PARAMETER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            }            

            pArg = (SVEARG_TVSC_PARAMETER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (TVSC_get_processing_state() == TVSC_BUSY)        // TV Scaler Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_PROCESSING_START : TV Scaler is Running\n\r")));
                    bRet = FALSE;
                    break;
                }

                TVSC_initialize(pArg->dwOpMode, pArg->dwScanMode,
                            pArg->dwSrcType, pArg->dwSrcBaseWidth, pArg->dwSrcBaseHeight, pArg->dwSrcWidth, pArg->dwSrcHeight, pArg->dwSrcOffsetX, pArg->dwSrcOffsetY,
                            pArg->dwDstType, pArg->dwDstBaseWidth, pArg->dwDstBaseHeight, pArg->dwDstWidth, pArg->dwDstHeight, pArg->dwDstOffsetX, pArg->dwDstOffsetY);
            }

            // Backup for PM
            memcpy(&pPMCtxt->tTVSCParam, pArg, sizeof(SVEARG_TVSC_PARAMETER));

            break;
        }

        case SVE_TVSC_SET_SOURCE_BUFFER:
        {
            SVEARG_TVSC_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_SOURCE_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_TVSC_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            }            

            pArg = (SVEARG_TVSC_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (TVSC_get_processing_state() == TVSC_BUSY)        // TV Scaler Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_SOURCE_BUFFER : TV Scaler is Running\n\r")));
                    bRet = FALSE;
                    break;
                }

                TVSC_set_source_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
            }

            // Backup for PM
            memcpy(&pPMCtxt->tTVSCSrcBuffer, pArg, sizeof(SVEARG_TVSC_BUFFER));
            pPMCtxt->bTVSCSrcBuffer = TRUE;

            break;
        }

        case SVE_TVSC_SET_NEXT_SOURCE_BUFFER:
        {
            SVEARG_TVSC_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_NEXT_SOURCE_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_TVSC_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            }            

            pArg = (SVEARG_TVSC_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (TVSC_get_processing_state() == TVSC_BUSY)        // TV Scaler Running
                {
                    BOOL bRetry = TRUE;

                    while(bRetry)
                    {
                        if (pCtxt->TVSCCmdCtxt.bCmdSetSrcBuffer == FALSE)
                        {
                            EnterCriticalSection(&pCtxt->TVSCCmdCtxt.csCmd);                        
                            // We can change buffer address here...
                            // But, after interrupt occurs, It can take effect..
                            TVSC_set_next_source_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);

                            pCtxt->TVSCCmdCtxt.bCmdSetSrcBuffer = TRUE;
                            bRetry = FALSE;
                            LeaveCriticalSection(&pCtxt->TVSCCmdCtxt.csCmd);                            
                        }

                        if (bRetry || pArg->bWaitForVSync)
                        {
                            if (WAIT_TIMEOUT == SVE_wait_tvsc_cmd_done())
                            {
                                VDE_ERR((_T("[VDE:ERR] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_NEXT_SOURCE_BUFFER : SVE_wait_tvsc_cmd_done() TimeOut\n\r")));
                                bRetry = FALSE;
                                bRet = FALSE;
                            }
                        }
                    }
                }
                else
                {
                    TVSC_set_next_source_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
                }
            }

            // Backup for PM
            memcpy(&pPMCtxt->tTVSCSrcBuffer, pArg, sizeof(SVEARG_TVSC_BUFFER));
            pPMCtxt->bTVSCSrcBuffer = TRUE;

            break;
        }

        case SVE_TVSC_SET_DESTINATION_BUFFER:
        {
            SVEARG_TVSC_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_DESTINATION_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_TVSC_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            }            

            pArg = (SVEARG_TVSC_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (TVSC_get_processing_state() == TVSC_BUSY)        // TV Scaler Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_DESTINATION_BUFFER : TV Scaler is Running\n\r")));
                    bRet = FALSE;
                    break;
                }

                TVSC_set_destination_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
            }

            // Backup for PM
            memcpy(&pPMCtxt->tTVSCDstBuffer, pArg, sizeof(SVEARG_TVSC_BUFFER));
            pPMCtxt->bTVSCDstBuffer = TRUE;

            break;
        }

        case SVE_TVSC_SET_NEXT_DESTINATION_BUFFER:
        {
            SVEARG_TVSC_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_NEXT_DESTINATION_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_TVSC_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            }            

            pArg = (SVEARG_TVSC_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (TVSC_get_processing_state() == TVSC_BUSY)        // TV Scaler Running
                {
                    BOOL bRetry = TRUE;

                    while(bRetry)
                    {
                        if (pCtxt->TVSCCmdCtxt.bCmdSetDstBuffer == FALSE)
                        {
                            EnterCriticalSection(&pCtxt->TVSCCmdCtxt.csCmd);
                        
                            // We can change buffer address here...
                            // But, after interrupt occurs, It can take effect..
                            TVSC_set_next_destination_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);

                            pCtxt->TVSCCmdCtxt.bCmdSetDstBuffer = TRUE;
                            bRetry = FALSE;

                            LeaveCriticalSection(&pCtxt->TVSCCmdCtxt.csCmd);                            
                        }

                        if (bRetry || pArg->bWaitForVSync)
                        {
                            if (WAIT_TIMEOUT == SVE_wait_tvsc_cmd_done())
                            {
                                VDE_ERR((_T("[VDE:ERR] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_NEXT_DESTINATION_BUFFER : SVE_wait_tvsc_cmd_done() TimeOut\n\r")));
                                bRetry = FALSE;
                                bRet = FALSE;
                            }
                        }
                    }
                }
                else
                {
                    TVSC_set_next_destination_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
                }
            }

            // Backup for PM
            memcpy(&pPMCtxt->tTVSCDstBuffer, pArg, sizeof(SVEARG_TVSC_BUFFER));
            pPMCtxt->bTVSCDstBuffer = TRUE;

            break;
        }

        case SVE_TVSC_SET_PROCESSING_START:
        {
            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_PROCESSING_START\n\r")));

            if (pPMCtxt->bPowerOn)
            {
                if (TVSC_get_processing_state() == TVSC_BUSY)        // TV Scaler Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_PROCESSING_START : TV Scaler is Running\n\r")));
                    bRet = FALSE;
                    break;
                }

                TVSC_enable_interrupt();
                TVSC_processing_start();
            }

            // Backup for PM
            pPMCtxt->bTVSCStart = TRUE;

            break;
        }

        case SVE_TVSC_SET_PROCESSING_STOP:
        {
            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_PROCESSING_STOP\n\r")));

            if (pPMCtxt->bPowerOn)
            {
                TVSC_autoload_disable();

                if (TVSC_get_processing_state() == TVSC_BUSY)    // TV Scaler Running
                {
                    // Wait for Current Frame Finished
                    if (WAIT_TIMEOUT == SVE_wait_tvsc_cmd_done())
                    {
                        if (TVSC_get_processing_state() == TVSC_IDLE)
                        {
                            // Time Out, But TV Scaler Finished
                            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_PROCESSING_STOP : SVE_wait_tvsc_cmd_done() TimeOut, But TV Scaler Finished\n\r")));
                        }
                        else
                        {
                            VDE_INF((_T("[VDE:INF] SVE_TVScaler_API_Proc() : SVE_TVSC_SET_PROCESSING_STOP : SVE_wait_tvsc_cmd_done() TimeOut\n\r")));
                            TVSC_processing_stop();
                        }
                    }
                }
            }

            // Backup for PM
            pPMCtxt->bTVSCStart = FALSE;

            break;
        }

        case SVE_TVSC_WAIT_PROCESSING_DONE:
        {
            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_WAIT_PROCESSING_DONE\n\r")));

            if (pPMCtxt->bPowerOn)
            {
                if (TVSC_get_processing_state() == TVSC_BUSY)        // TV Scaler Running
                {
                    if (WAIT_TIMEOUT == SVE_wait_tvsc_cmd_done())
                    {
                        if (TVSC_get_processing_state() == TVSC_IDLE)
                        {
                            // Time Out, But TV Scaler Finished
                            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_WAIT_PROCESSING_DONE : SVE_wait_tvsc_cmd_done() TimeOut, But TV Scaler Finished\n\r")));
                        }
                        else
                        {
                            VDE_INF((_T("[VDE:INF] SVE_TVScaler_API_Proc() : SVE_TVSC_WAIT_PROCESSING_DONE : SVE_wait_tvsc_cmd_done() TimeOut\n\r")));
                            bRet = FALSE;
                        }
                    }
                }
            }

            // Backup for PM
            pPMCtxt->bTVSCStart = FALSE;

            break;
        }

        case SVE_TVSC_GET_PROCESSING_STATUS:
        {
            DWORD *pArg;

            VDE_MSG((_T("[VDE] SVE_TVScaler_API_Proc() : SVE_TVSC_GET_PROCESSING_STATUS\n\r")));

            if(!pBufOut || dwLenOut < sizeof(DWORD))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufOut:0x%x dwLenOut:%d\n"), pBufOut, dwLenOut));
                bRet = FALSE;
                break;
            } 
            
            pArg = (DWORD *)pBufOut;

            if (pPMCtxt->bPowerOn)
            {
                *pArg = TVSC_get_processing_state();
            }
            else
            {
                *pArg = TVSC_IDLE;
            }

            if (pdwActualOut)
            {
                *pdwActualOut = sizeof(DWORD);
            }

            break;
        }
        default:    // This is filtered above switch statement
            bRet = FALSE;
            break;            
        
    }

CleanUp:

    VDE_MSG((_T("[VDE] --SVE_TVScaler_API_Proc()\n\r")));

    return bRet;
}


