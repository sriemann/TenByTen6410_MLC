//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#include <bsp.h>
#include "SVEngine.h"

BOOL SVE_Post_API_Proc(
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

    VDE_MSG((_T("[VDE] ++SVE_Post_API_Proc()\n\r")));

    pCtxt = SVE_get_context();
    pPMCtxt = SVE_get_power_context();


    //-------------------------------------------------------------
    // Check OpenContext of Caller have the right to access to H/W Resource
    //-------------------------------------------------------------
    switch(dwCode)
    {
    case SVE_POST_SET_PROCESSING_PARAM:
    case SVE_POST_SET_SOURCE_BUFFER:
    case SVE_POST_SET_NEXT_SOURCE_BUFFER:
    case SVE_POST_SET_DESTINATION_BUFFER:
    case SVE_POST_SET_NEXT_DESTINATION_BUFFER:
    case SVE_POST_SET_PROCESSING_START:
    case SVE_POST_SET_PROCESSING_STOP:
    case SVE_POST_WAIT_PROCESSING_DONE:
    case SVE_POST_GET_PROCESSING_STATUS:
        bRet = SVE_resource_compare_Post(hOpenContext);
        if (!bRet)
        {
            VDE_ERR((_T("[VDE:ERR] SVE_Post_API_Proc(0x%08x) : No Right to Access to H/W Resource\r\n"), dwCode));
            goto CleanUp;
        }
        break;
    default:
        DEBUGMSG(VDE_ZONE_ERROR, (TEXT("[VDE:ERR] Invalid IOCTL code\n")));
        bRet = FALSE;
        goto CleanUp;        
    }

    //--------------------------------
    // Processing IOCTL for post Processor
    //--------------------------------

    switch(dwCode)
    {
        case SVE_POST_SET_PROCESSING_PARAM:
        {
            SVEARG_POST_PARAMETER *pArg;

            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_SET_PROCESSING_PARAM\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_POST_PARAMETER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            }                   

            pArg = (SVEARG_POST_PARAMETER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (Post_get_processing_state() == POST_BUSY)        // Post Processor Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_Post_API_Proc() : SVE_POST_SET_PROCESSING_START : Post Processor is Running\n\r")));
                    bRet = FALSE;
                    break;
                }


                Post_initialize(pArg->dwOpMode, pArg->dwScanMode,
                            pArg->dwSrcType, pArg->dwSrcBaseWidth, pArg->dwSrcBaseHeight, pArg->dwSrcWidth, pArg->dwSrcHeight, pArg->dwSrcOffsetX, pArg->dwSrcOffsetY,
                            pArg->dwDstType, pArg->dwDstBaseWidth, pArg->dwDstBaseHeight, pArg->dwDstWidth, pArg->dwDstHeight, pArg->dwDstOffsetX, pArg->dwDstOffsetY);
            }

            // Backup for PM
            memcpy(&pPMCtxt->tPostParam, pArg, sizeof(SVEARG_POST_PARAMETER));

            break;
        }

        case SVE_POST_SET_SOURCE_BUFFER:
        {
            SVEARG_POST_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_SET_SOURCE_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_POST_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            } 

            pArg = (SVEARG_POST_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (Post_get_processing_state() == POST_BUSY)        // Post Processor Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_Post_API_Proc() : SVE_POST_SET_SOURCE_BUFFER : Post Processor is Running\n\r")));
                    bRet = FALSE;
                    break;
                }

                Post_set_source_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
            }

            // Backup for PM
            memcpy(&pPMCtxt->tPostSrcBuffer, pArg, sizeof(SVEARG_POST_BUFFER));
            pPMCtxt->bPostSrcBuffer = TRUE;

            break;
        }

        case SVE_POST_SET_NEXT_SOURCE_BUFFER:
        {
            SVEARG_POST_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_SET_NEXT_SOURCE_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_POST_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            } 

            pArg = (SVEARG_POST_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (Post_get_processing_state() == POST_BUSY)
                {
                    BOOL bRetry = TRUE;

                    while(bRetry)
                    {
                        if (pCtxt->PostCmdCtxt.bCmdSetSrcBuffer == FALSE)
                        {
                            EnterCriticalSection(&pCtxt->PostCmdCtxt.csCmd);
                        
                            // We can change buffer address here...
                            // But, after interrupt occurs, It can take effect..
                            Post_set_next_source_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);

                            pCtxt->PostCmdCtxt.bCmdSetSrcBuffer = TRUE;
                            bRetry = FALSE;
                            
                            LeaveCriticalSection(&pCtxt->PostCmdCtxt.csCmd);                            
                        }

                        if (bRetry || pArg->bWaitForVSync)
                        {
                            if (WAIT_TIMEOUT == SVE_wait_post_cmd_done())
                            {
                                VDE_ERR((_T("[VDE:ERR] SVE_Post_API_Proc() : SVE_POST_SET_NEXT_SOURCE_BUFFER : SVE_wait_post_cmd_done() TimeOut\n\r")));
                                bRetry = FALSE;
                                bRet = FALSE;
                            }
                        }
                    }
                }
                else
                {
                    Post_set_next_source_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
                }
            }

            // Backup for PM
            memcpy(&pPMCtxt->tPostSrcBuffer, pArg, sizeof(SVEARG_POST_BUFFER));
            pPMCtxt->bPostSrcBuffer = TRUE;

            break;
        }

        case SVE_POST_SET_DESTINATION_BUFFER:
        {
            SVEARG_POST_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_SET_DESTINATION_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_POST_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            } 

            pArg = (SVEARG_POST_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (Post_get_processing_state() == POST_BUSY)        // Post Processor Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_Post_API_Proc() : SVE_POST_SET_DESTINATION_BUFFER : Post Processor is Running\n\r")));
                    bRet = FALSE;
                    break;
                }

                Post_set_destination_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
            }

            // Backup for PM
            memcpy(&pPMCtxt->tPostDstBuffer, pArg, sizeof(SVEARG_POST_BUFFER));
            pPMCtxt->bPostDstBuffer = TRUE;

            break;
        }

        case SVE_POST_SET_NEXT_DESTINATION_BUFFER:
        {
            SVEARG_POST_BUFFER *pArg;

            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_SET_NEXT_DESTINATION_BUFFER\n\r")));

            if(!pBufIn || dwLenIn < sizeof(SVEARG_POST_BUFFER))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufIn:0x%x Length:%d\n"), pBufIn, dwLenIn));
                bRet = FALSE;
                break;
            } 

            pArg = (SVEARG_POST_BUFFER *)pBufIn;

            if (pPMCtxt->bPowerOn)
            {
                if (Post_get_processing_state() == POST_BUSY)
                {
                    BOOL bRetry = TRUE;

                    while(bRetry)
                    {
                        if (pCtxt->PostCmdCtxt.bCmdSetDstBuffer == FALSE)
                        {
                            EnterCriticalSection(&pCtxt->PostCmdCtxt.csCmd);
                        
                            // We can change buffer address here...
                            // But, after interrupt occurs, It can take effect..
                            Post_set_next_destination_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);

                            pCtxt->PostCmdCtxt.bCmdSetDstBuffer = TRUE;
                            bRetry = FALSE;
                            LeaveCriticalSection(&pCtxt->PostCmdCtxt.csCmd);                            
                            
                        }

                        if (bRetry || pArg->bWaitForVSync)
                        {
                            if (WAIT_TIMEOUT == SVE_wait_post_cmd_done())
                            {
                                VDE_ERR((_T("[VDE:ERR] SVE_Post_API_Proc() : SVE_POST_SET_NEXT_DESTINATION_BUFFER : SVE_wait_post_cmd_done() TimeOut\n\r")));
                                bRetry = FALSE;
                                bRet = FALSE;
                            }
                        }
                    }
                }
                else
                {
                    Post_set_next_destination_buffer(pArg->dwBufferRGBY, pArg->dwBufferCb, pArg->dwBufferCr);
                }
            }

            // Backup for PM
            memcpy(&pPMCtxt->tPostDstBuffer, pArg, sizeof(SVEARG_POST_BUFFER));
            pPMCtxt->bPostDstBuffer = TRUE;

            break;
        }

        case SVE_POST_SET_PROCESSING_START:
        {
            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_SET_PROCESSING_START\n\r")));

            if (pPMCtxt->bPowerOn)
            {
                if (Post_get_processing_state() == POST_BUSY)        // Post Processor Running
                {
                    VDE_ERR((_T("[VDE:ERR] SVE_Post_API_Proc() : SVE_POST_SET_PROCESSING_START : Post Processor is Running\n\r")));
                    bRet = FALSE;
                    break;
                }

                Post_enable_interrupt();
                Post_processing_start();
            }

            // Backup for PM
            pPMCtxt->bPostStart = TRUE;

            break;
        }

        case SVE_POST_SET_PROCESSING_STOP:
        {
            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_SET_PROCESSING_STOP\n\r")));

            if (pPMCtxt->bPowerOn)
            {
                Post_autoload_disable();

                if (Post_get_processing_state() == POST_BUSY)        // Post Processor Running
                {
                    // Wait for Current Frame Finished
                    if (WAIT_TIMEOUT == SVE_wait_post_cmd_done())
                    {
                        if (Post_get_processing_state() == POST_IDLE)
                        {
                            // Time Out, But Post Processor Finished
                            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_WAIT_PROCESSING_DONE : SVE_wait_post_cmd_done() TimeOut, But Post Processor Finished\n\r")));
                        }
                        else
                        {
                            VDE_INF((_T("[VDE:INF] SVE_Post_API_Proc() : SVE_POST_WAIT_PROCESSING_DONE : SVE_wait_post_cmd_done() TimeOut\n\r")));
                            Post_processing_stop();
                        }
                    }
                }
            }

            // Backup for PM
            pPMCtxt->bPostStart = FALSE;

            break;
        }

        case SVE_POST_WAIT_PROCESSING_DONE:
        {
            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_WAIT_PROCESSING_DONE\n\r")));

            if (pPMCtxt->bPowerOn)
            {
                if (Post_get_processing_state() == POST_BUSY)        // Post Processor Running
                {
                    if (WAIT_TIMEOUT == SVE_wait_post_cmd_done())
                    {
                        if (Post_get_processing_state() == POST_IDLE)
                        {
                            // Time Out, But Post Processor Finished
                            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_WAIT_PROCESSING_DONE : SVE_wait_post_cmd_done() TimeOut, But Post Processor Finished\n\r")));
                        }
                        else
                        {
                            VDE_INF((_T("[VDE:INF] SVE_Post_API_Proc() : SVE_POST_WAIT_PROCESSING_DONE : SVE_wait_post_cmd_done() TimeOut\n\r")));
                            bRet = FALSE;
                        }
                    }
                }
            }

            // Backup for PM
            pPMCtxt->bPostStart = FALSE;

            break;
        }

        case SVE_POST_GET_PROCESSING_STATUS:
        {
            DWORD *pArg;

            VDE_MSG((_T("[VDE] SVE_Post_API_Proc() : SVE_POST_GET_PROCESSING_STATUS\n\r")));

            if(!pBufOut || dwLenOut < sizeof(DWORD))
            {
                DEBUGMSG(VDE_ZONE_ERROR,(TEXT("Invalid Parameter : pBufOut:0x%x dwLenOut:%d\n"), pBufOut, dwLenOut));
                bRet = FALSE;
                break;
            } 

            pArg = (DWORD *)pBufOut;

            if (pPMCtxt->bPowerOn)
            {
                *pArg = Post_get_processing_state();
            }
            else
            {
                *pArg = POST_IDLE;
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

    VDE_MSG((_T("[VDE] --SVE_Post_API_Proc()\n\r")));

    return bRet;
}


