//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

   power_control.c   Power Controller Driver

Abstract:

   This module control Power state of system and HW Block Power
   Especially, this module concerns about sleep/wakeup changes
   IOCTL serves HW Block Power on/off

Functions:

    

Notes:

--*/

#include "precomp.h"

static volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;
static HANDLE g_hThreadPowerMon = NULL;
static HANDLE g_hMsgQueue = NULL;
static BOOL g_aIPPowerStatus[PWR_IP_MAX] = {FALSE, };
static BOOL g_bExitThread = FALSE;
static CRITICAL_SECTION csPowerCon;

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
    (PWRCTL_ZONES)                               \
};

/**
*    @fn    PowerMonitorThread(void)
*    @note  This thread handle Power notification message
*/
INT
WINAPI
PowerMonitorThread(void)
{
    HANDLE hPowerNotification = NULL;
    MSGQUEUEOPTIONS msgOptions;
    PPOWER_BROADCAST pB;
//    RESET_STATUS eRstStat;
    UCHAR msgBuf[QUEUE_SIZE];
    DWORD iBytesInQueue = 0;
    DWORD dwFlags;
    int iSleepCount = 0;

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON:INF] ++%s\n"), _T(__FUNCTION__)));

//    eRstStat = PwrCon_get_reset_status();

    //-------------------------------------------------
    // Set Power Monitor Thread Priority
    //-------------------------------------------------
    CeSetThreadPriority(g_hThreadPowerMon, POWER_MONITOR_THREAD_PRIODITY);

    //-------------------------------------------------
    // Create a message queue for Power Manager notifications.
    //-------------------------------------------------
    memset((void *)&msgOptions, 0x0, sizeof(msgOptions));
    msgOptions.dwSize = sizeof(MSGQUEUEOPTIONS);
    msgOptions.dwFlags = 0;
    msgOptions.dwMaxMessages = QUEUE_ENTRIES;
    msgOptions.cbMaxMessage = sizeof(POWER_BROADCAST) + MAX_NAMELEN;
    msgOptions.bReadAccess = TRUE;

    g_hMsgQueue = CreateMsgQueue(NULL, &msgOptions);
    if (g_hMsgQueue == NULL )
    {
        RETAILMSG(PWC_ZONE_ERROR,(_T("[PWRCON:ERR] %s->CreateMsgQueue() Failed : Err %d\n"), _T(__FUNCTION__), GetLastError()));
        goto Thread_CleanUp;
    }

    // Request Power notifications
    hPowerNotification = RequestPowerNotifications(g_hMsgQueue, POWER_NOTIFY_ALL);
    if (!hPowerNotification)
    {
        RETAILMSG(PWC_ZONE_ERROR,(_T("[PWRCON:ERR] %s->RequestPowerNotifications() Failed : Err %d\n"), _T(__FUNCTION__), GetLastError()));
        goto Thread_CleanUp;
    }

    while(!g_bExitThread)
    {
        memset(msgBuf, 0x0, QUEUE_SIZE);
        pB = (PPOWER_BROADCAST)msgBuf;

        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWR:INF] PowerMonitorThread() : Wait for PM Notification\n")));

        // Read message from queue.
        if (!ReadMsgQueue(g_hMsgQueue, msgBuf, QUEUE_SIZE, &iBytesInQueue, INFINITE, &dwFlags))
        {
            if (g_bExitThread)
            {
                break;
            }

            RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s->ReadMsgQueue() Failed : Err %d\n"), _T(__FUNCTION__), GetLastError()));
        }
        else if (iBytesInQueue < sizeof(POWER_BROADCAST))
        {
            RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s->Receive Insufficient Message (Size is %d, Expected %d)\n"), _T(__FUNCTION__), iBytesInQueue, sizeof(POWER_BROADCAST)));
        }
        else
        {
            switch (pB->Message)
            {
            //-----------------------
            // Notified State Transition
            //-----------------------
            case PBT_TRANSITION:

                RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Notified [PBT_TRANSITION : %s (0x%08x)]\n"), pB->SystemPowerState, pB->Flags));
                break;

            //-----------------------
            // Notified Resume State
            //-----------------------
            case PBT_RESUME:

                RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Notified [PBT_RESUME]\n")));
                {
                    DWORD dwWakeSrc = SYSWAKE_UNKNOWN;
                    DWORD dwBytesRet = 0;

                    if (KernelIoControl(IOCTL_HAL_GET_WAKE_SOURCE, NULL, 0, &dwWakeSrc, sizeof(dwWakeSrc), &dwBytesRet)
                        && (dwBytesRet == sizeof(dwWakeSrc)))
                    {
                        switch(dwWakeSrc)
                        {
                        case SYSWAKE_POWER_BUTTON:    // Power Button
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by Power Button\n")));
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            SetSystemPowerState(NULL, POWER_STATE_ON, POWER_FORCE);
                            break;
                        case OEMWAKE_RTC_ALARM:
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by RTC Alarm\n")));
                            //PWRCON_INF((_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            // Do not change Power State to POWER_STATE_ON
                            break;
                        case OEMWAKE_RTC_TICK:
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by RTC Tick\n")));
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            SetSystemPowerState(NULL, POWER_STATE_ON, POWER_FORCE);
                            break;
                        case OEMWAKE_KEYPAD:
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by Keypad\n")));
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            SetSystemPowerState(NULL, POWER_STATE_ON, POWER_FORCE);
                            break;
                        case OEMWAKE_MSM:
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by MSM I/F\n")));
                            //PWRCON_INF((_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            //SetSystemPowerState(NULL, POWER_STATE_ON, POWER_FORCE);
                            break;
                        case OEMWAKE_BATTERY_FAULT:
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by Battery Fault\n")));
                            //PWRCON_INF((_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            //SetSystemPowerState(NULL, POWER_STATE_ON, POWER_FORCE);
                            break;
                        case OEMWAKE_WARM_RESET:
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by Warm Reset\n")));
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            SetSystemPowerState(NULL, POWER_STATE_ON, POWER_FORCE);
                            break;
                        case OEMWAKE_HSI:
                            RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Wake Up by HSI I/F\n")));
                            //PWRCON_INF((_T("[PWRCON:INF] SetSystemPowerState(POWER_STATE_ON)\n")));
                            //SetSystemPowerState(NULL, POWER_STATE_ON, POWER_FORCE);
                            break;
                        default:
                            RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] WakeUp Source = 0x%08x\n"), dwWakeSrc));
                            break;
                        }


                    }
                    else
                    {
                        NKDbgPrintfW(L"PWRCON: Error getting wake source\n");
                        RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] IOCTL_HAL_GET_WAKE_SOURCE Failed\n")));
                        RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] System is Still in [PBT_RESUME] State\n")));
                    }
                }
                break;

            //-----------------------------------
            // Notified Power Supply changed (AC/DC)
            //-----------------------------------
            case PBT_POWERSTATUSCHANGE:
                RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] Notified [PBT_POWERSTATUSCHANGE]\n")));
                break;

            //---------------------------------
            // Notified Power Information changed
            //---------------------------------
            case PBT_POWERINFOCHANGE:
            {
                PPOWER_BROADCAST_POWER_INFO ppbpi;

                ppbpi = (PPOWER_BROADCAST_POWER_INFO)pB->SystemPowerState;

                RETAILMSG(PWC_ZONE_TEMP,(_T("[PWRCON:INF] Notified [PBT_POWERINFOCHANGE]\n")));
                RETAILMSG(PWC_ZONE_TEMP,(_T("[PWRCON:INF]     ACLine Status : %d\n"), ppbpi->bACLineStatus));
                RETAILMSG(PWC_ZONE_TEMP,(_T("[PWRCON:INF]     Battery Flag  : %d\n"), ppbpi->bBatteryFlag));
                RETAILMSG(PWC_ZONE_TEMP,(_T("[PWRCON:INF]     Backup Flag   : %d\n"), ppbpi->bBackupBatteryFlag));
                RETAILMSG(PWC_ZONE_TEMP,(_T("[PWRCON:INF]     Level         : %d\n"), ppbpi->dwNumLevels));
                break;
            }

            default:
                RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] Notified Unknown Message [0x%08x]\n"), pB->Message));
                break;
            }
        }
    }

Thread_CleanUp:
    if (hPowerNotification)
    {
        StopPowerNotifications(hPowerNotification);
        hPowerNotification = NULL;
    }

    if (g_hMsgQueue)
    {
        CloseMsgQueue(g_hMsgQueue);
        g_hMsgQueue = NULL;
    }

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON:INF] --%s\n"), _T(__FUNCTION__)));

    return 0;
}

/**
*    @fn    AllocResource(void)
*    @note  This function maps System Controller Register Block to virtual address space.
*/
static BOOL
AllocResources(void)
{
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
    DEBUGMSG(PWC_ZONE_ENTER, (_T("[PWRCON] ++%s\n"), _T(__FUNCTION__)));

    //--------------------
    // System Controller SFR
    //--------------------
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
    g_pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (g_pSysConReg == NULL)
    {
        RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s->g_pSysConReg MmMapIoSpace() Failed \n"), _T(__FUNCTION__)));
        return FALSE;
    }

    //--------------------
    // Critical Section
    //--------------------
    InitializeCriticalSection(&csPowerCon);

    DEBUGMSG(PWC_ZONE_ENTER, (_T("[PWRCON] --%s\n"), _T(__FUNCTION__)));

    return TRUE;
}

/**
*    @fn    ReleaseResource(void)
*    @note  This function unmaps System Controller Register Block' virtual address space.
*/
static void
ReleaseResources(void)
{
    DEBUGMSG(PWC_ZONE_ENTER, (_T("[PWRCON] ++%s\n"), _T(__FUNCTION__)));

    if (g_pSysConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pSysConReg, sizeof(S3C6410_SYSCON_REG));
        g_pSysConReg = NULL;
    }

    DeleteCriticalSection(&csPowerCon);

    DEBUGMSG(PWC_ZONE_ENTER, (_T("[PWRCON] --%s\n"), _T(__FUNCTION__)));
}

BOOL
WINAPI
DllEntry(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DEBUGREGISTER(hinstDll);
        DisableThreadLibraryCalls ((HMODULE)hinstDll);
        DEBUGMSG(PWC_ZONE_INIT,(_T("[PWRCON] %s : Process Attach\n"), _T(__FUNCTION__)));
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DEBUGMSG(PWC_ZONE_INIT,(_T("[PWRCON] %s : Process Detach\n"), _T(__FUNCTION__)));
    }

    return TRUE;
}

BOOL PWC_Deinit(DWORD pContext)
{
    DEBUGMSG(PWC_ZONE_INIT,(_T("[PWRCON] ++%s(0x%08x)\n"), _T(__FUNCTION__), pContext));

    g_bExitThread = TRUE;

    if(g_hThreadPowerMon)        // Make Sure if thread is exist
    {
        // Signal Thread to Finish
        if (g_hMsgQueue)
        {
            CloseMsgQueue(g_hMsgQueue);    // Closing the MsgQueue will force ReadMsgQueue to return
            g_hMsgQueue = NULL;
        }

        // Wait for Thread to Finish
        WaitForSingleObject(g_hThreadPowerMon, INFINITE);
        CloseHandle(g_hThreadPowerMon);
        g_hThreadPowerMon = NULL;
    }
    ReleaseResources();

    DEBUGMSG(PWC_ZONE_INIT,(_T("[PWRCON] --%s\n"), _T(__FUNCTION__) ));

    return TRUE;
}

DWORD
PWC_Init(DWORD dwContext)
{
    DEBUGMSG(PWC_ZONE_INIT,(_T("[PWRCON:INF] ++%s(0x%08x)\n"), _T(__FUNCTION__), dwContext));

    if (AllocResources() == FALSE)
    {
        RETAILMSG(PWC_ZONE_ERROR,(_T("[PWRCON:ERR] %s->AllocResources() Failed \n"), _T(__FUNCTION__)));
        goto CleanUp;
    }

    PwrCon_initialize_register_address((void *)g_pSysConReg);

    // Create power Monitor Thread, Singleton
    g_hThreadPowerMon = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PowerMonitorThread, NULL, 0, NULL);
    if (g_hThreadPowerMon == NULL )
    {
        RETAILMSG(PWC_ZONE_ERROR,(_T("[PWRCON:ERR] %s->CreateThread() Power Monitor Failed \n"), _T(__FUNCTION__)));
        goto CleanUp;
    }
    DEBUGMSG(PWC_ZONE_INIT,(_T("[PWRCON:INF] --%s()\n"), _T(__FUNCTION__)));

    return TRUE;

CleanUp:

    RETAILMSG(PWC_ZONE_ERROR,(_T("[PWRCON:ERR] --%s : Failed\n"), _T(__FUNCTION__)));

    PWC_Deinit(0);

    return FALSE;
}

DWORD
PWC_Open(DWORD pContext, DWORD dwAccess, DWORD dwShareMode)
{
    DEBUGMSG(PWC_ZONE_ENTER,(_T("[PWRCON] %s(0x%08x, 0x%08x, 0x%08x)\n"), _T(__FUNCTION__), pContext, dwAccess, dwShareMode));
    return TRUE;
}

BOOL
PWC_Close(DWORD pContext)
{
    DEBUGMSG(PWC_ZONE_ENTER,(_T("[PWRCON] %s(0x%08x)\n"), _T(__FUNCTION__), pContext));
    return TRUE;
}

DWORD
PWC_Read (DWORD pContext,  LPVOID pBuf, DWORD Len)
{
    DEBUGMSG(PWC_ZONE_ENTER,(_T("[PWRCON] %s(0x%08x, 0x%08x, 0x%08x)\n"), _T(__FUNCTION__), pContext, pBuf, Len));
    return (0);    // End of File
}

DWORD
PWC_Write(DWORD pContext, LPCVOID pBuf, DWORD Len)
{
    DEBUGMSG(PWC_ZONE_ENTER,(_T("[PWRCON] %s(0x%08x, 0x%08x, 0x%08x)\n"), _T(__FUNCTION__), pContext, pBuf, Len));
    return (0);    // Number of Byte
}

DWORD
PWC_Seek (DWORD pContext, long pos, DWORD type)
{
    DEBUGMSG(PWC_ZONE_ENTER,(_T("[PWRCON] %s(0x%08x, 0x%08x, 0x%08x)\n"), _T(__FUNCTION__), pContext, pos, type));
    return (DWORD)-1;    // Failure
}

BOOL
PWC_PowerUp(DWORD pContext)
{
    DEBUGMSG(PWC_ZONE_ENTER,(_T("[PWRCON] %s(0x%08x)\n"), _T(__FUNCTION__), pContext));

    return TRUE;
}

BOOL
PWC_PowerDown(DWORD pContext)
{
    DEBUGMSG(PWC_ZONE_ENTER,(_T("[PWRCON] %s(0x%08x)\n"), _T(__FUNCTION__), pContext));

    return TRUE;
}

BOOL
PWC_IOControl(DWORD pContext, DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    BOOL bRet = TRUE;
    DWORD dwIndex;

    //if caller is not kernel mode, do not allow setting power state
    if (GetDirectCallerProcessId() != GetCurrentProcessId()){
        return ERROR_ACCESS_DENIED;
    }

    if ( !( (dwCode == IOCTL_PWRCON_SET_POWER_ON)
        || (dwCode == IOCTL_PWRCON_SET_POWER_OFF)
        || (dwCode == IOCTL_PWRCON_SET_SYSTEM_LEVEL)
        || (dwCode == IOCTL_PWRCON_QUERY_SYSTEM_LEVEL)        
        || (dwCode == IOCTL_PWRCON_PROFILE_DVS)
        ))
        
    {
        RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s : Unknown IOCTL [0x%08x]\n"), _T(__FUNCTION__), dwCode));
        SetLastError (ERROR_INVALID_ACCESS);
        return FALSE;
    }

    switch(dwCode)
    {
    case IOCTL_PWRCON_SET_POWER_ON:

        __try
        {
            if ((dwLenIn < sizeof(DWORD)) || 
                (NULL == pBufIn))
            {
                bRet = FALSE;
            }
            else
            {
                dwIndex = *(DWORD*)(pBufIn);
                if (dwIndex > PWR_IP_MAX - 1) // Validate dwIndex since this indexes into the g_aIPPowerStatus array
                {
                    bRet = FALSE;
                }
            }

            if (!bRet)
            {
                RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s(IOCTL_PWRCON_SET_POWER_ON) : Invalid Parameter\n"),_T(__FUNCTION__)));
                SetLastError (ERROR_INVALID_PARAMETER);
                break;
            }
        }
        
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s(IOCTL_PWRCON_SET_POWER_ON) : Exception\n"),_T(__FUNCTION__)));
            SetLastError (ERROR_INVALID_PARAMETER);
            bRet = FALSE;
            break;
        }

        EnterCriticalSection(&csPowerCon);

        switch(dwIndex)
        {
        case PWR_IP_IROM:
            g_aIPPowerStatus[dwIndex] = TRUE;
            RETAILMSG(PWC_ZONE_BLK_PWR_ON,(_T("[PWRCON:INF] %s(SET_POWER_ON, %d) : IROM\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_on(BLKPWR_DOMAIN_IROM);
            break;
        case PWR_IP_ETM:
            g_aIPPowerStatus[dwIndex] = TRUE;
            RETAILMSG(PWC_ZONE_BLK_PWR_ON,(_T("[PWRCON:INF] %s(SET_POWER_ON, %d) : ETM\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_on(BLKPWR_DOMAIN_ETM);
            break;
        case PWR_IP_SDMA0:        // Domain S
        case PWR_IP_SDMA1:
        case PWR_IP_SECURITY:
            g_aIPPowerStatus[dwIndex] = TRUE;
            RETAILMSG(PWC_ZONE_BLK_PWR_ON,(_T("[PWRCON:INF] %s(SET_POWER_ON, %d) : S\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_on(BLKPWR_DOMAIN_S);
            break;
        case PWR_IP_ROTATOR:    // Domain F
        case PWR_IP_POST:
        case PWR_IP_DISPCON:
            g_aIPPowerStatus[dwIndex] = TRUE;
            RETAILMSG(PWC_ZONE_BLK_PWR_ON,(_T("[PWRCON:INF] %s(SET_POWER_ON, %d) : F\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_on(BLKPWR_DOMAIN_F);
            break;
        case PWR_IP_2D:            // Domain P
        case PWR_IP_TVENC:
        case PWR_IP_TVSC:
            g_aIPPowerStatus[dwIndex] = TRUE;
            RETAILMSG(PWC_ZONE_BLK_PWR_ON,(_T("[PWRCON:INF] %s(SET_POWER_ON, %d) : P\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_on(BLKPWR_DOMAIN_P);
            break;
        case PWR_IP_JPEG:        // Domain I
        case PWR_IP_CAMIF:
            g_aIPPowerStatus[dwIndex] = TRUE;
            RETAILMSG(PWC_ZONE_BLK_PWR_ON,(_T("[PWRCON:INF] %s(SET_POWER_ON, %d) : I\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_on(BLKPWR_DOMAIN_I);
            break;
        case PWR_IP_MFC:        // Domain V
            g_aIPPowerStatus[dwIndex] = TRUE;
            RETAILMSG(PWC_ZONE_BLK_PWR_ON,(_T("[PWRCON:INF] %s(SET_POWER_ON, %d) : V\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_on(BLKPWR_DOMAIN_V);
            break;
        default:
            RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s(IOCTL_PWRCON_SET_POWER_ON, %d) : Invalid Parameter\n"), _T(__FUNCTION__), dwIndex));
            SetLastError (ERROR_INVALID_PARAMETER);
            bRet = FALSE;
        }

        LeaveCriticalSection(&csPowerCon);

        break;

    case IOCTL_PWRCON_SET_POWER_OFF:

        __try
        {
            if ((dwLenIn < sizeof(DWORD)) || 
                (NULL == pBufIn))
            {
                bRet = FALSE;
            }
            else
            {
                dwIndex = *(DWORD*)(pBufIn);
                if (dwIndex > PWR_IP_MAX - 1) // Validate dwIndex since this indexes into the g_aIPPowerStatus array
                {
                    bRet = FALSE;
                }
            }

            if (!bRet)
            {
                RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s(IOCTL_PWRCON_SET_POWER_OFF) : Invalid Parameter\n"),_T(__FUNCTION__)));
                SetLastError (ERROR_INVALID_PARAMETER);
                break;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s(IOCTL_PWRCON_SET_POWER_OFF) : Exception\n"),_T(__FUNCTION__)));
            SetLastError (ERROR_INVALID_PARAMETER);
            bRet = FALSE;
            break;
        }
        
        EnterCriticalSection(&csPowerCon);

        switch(dwIndex)
        {
        case PWR_IP_IROM:
            g_aIPPowerStatus[dwIndex] = FALSE;
            RETAILMSG(PWC_ZONE_BLK_PWR_OFF,(_T("[PWRCON:INF] %s(SET_POWER_OFF, %d) : IROM\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_off(BLKPWR_DOMAIN_IROM);
            break;
        case PWR_IP_ETM:
            g_aIPPowerStatus[dwIndex] = FALSE;
            RETAILMSG(PWC_ZONE_BLK_PWR_OFF,(_T("[PWRCON:INF] %s(SET_POWER_OFF, %d) : ETM\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_off(BLKPWR_DOMAIN_ETM);
            break;
        case PWR_IP_SDMA0:        // Domain S
        case PWR_IP_SDMA1:
        case PWR_IP_SECURITY:
            g_aIPPowerStatus[dwIndex] = FALSE;
            if ( (g_aIPPowerStatus[PWR_IP_SDMA0] == FALSE)
                && (g_aIPPowerStatus[PWR_IP_SDMA1] == FALSE)
                && (g_aIPPowerStatus[PWR_IP_SECURITY] == FALSE) )
            {
                RETAILMSG(PWC_ZONE_BLK_PWR_OFF, (_T("[PWRCON:INF] %s(SET_POWER_OFF, %d) : S\n"), _T(__FUNCTION__), dwIndex));
                PwrCon_set_block_power_off(BLKPWR_DOMAIN_S);
            }
            break;
        case PWR_IP_ROTATOR:    // Domain F
        case PWR_IP_POST:
        case PWR_IP_DISPCON:
            g_aIPPowerStatus[dwIndex] = FALSE;
            if ( (g_aIPPowerStatus[PWR_IP_ROTATOR] == FALSE)
                && (g_aIPPowerStatus[PWR_IP_POST] == FALSE)
                && (g_aIPPowerStatus[PWR_IP_DISPCON] == FALSE) )
            {
                RETAILMSG(PWC_ZONE_BLK_PWR_OFF, (_T("[PWRCON:INF] %s(SET_POWER_OFF, %d) : F\n"), _T(__FUNCTION__), dwIndex));
                PwrCon_set_block_power_off(BLKPWR_DOMAIN_F);
            }
            break;
        case PWR_IP_2D:            // Domain P
        case PWR_IP_TVENC:
        case PWR_IP_TVSC:
            g_aIPPowerStatus[dwIndex] = FALSE;
            if ( (g_aIPPowerStatus[PWR_IP_2D] == FALSE)
                && (g_aIPPowerStatus[PWR_IP_TVENC] == FALSE)
                && (g_aIPPowerStatus[PWR_IP_TVSC] == FALSE) )
            {
                RETAILMSG(PWC_ZONE_BLK_PWR_OFF, (_T("[PWRCON:INF] %s(SET_POWER_OFF, %d) : P\n"), _T(__FUNCTION__), dwIndex));
                PwrCon_set_block_power_off(BLKPWR_DOMAIN_P);
            }
            break;
        case PWR_IP_JPEG:        // Domain I
        case PWR_IP_CAMIF:
            g_aIPPowerStatus[dwIndex] = FALSE;
            if ( (g_aIPPowerStatus[PWR_IP_JPEG] == FALSE)
                && (g_aIPPowerStatus[PWR_IP_CAMIF] == FALSE) )
            {
                RETAILMSG(PWC_ZONE_BLK_PWR_OFF, (_T("[PWRCON:INF] %s(SET_POWER_OFF, %d) : I\n"), _T(__FUNCTION__), dwIndex));
                PwrCon_set_block_power_off(BLKPWR_DOMAIN_I);
            }
            break;
        case PWR_IP_MFC:        // Domain V
            g_aIPPowerStatus[dwIndex] = FALSE;
            RETAILMSG(PWC_ZONE_BLK_PWR_OFF, (_T("[PWRCON:INF] %s(SET_POWER_OFF, %d) : V\n"), _T(__FUNCTION__), dwIndex));
            PwrCon_set_block_power_off(BLKPWR_DOMAIN_V);
            break;
        default:
            RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s(SET_POWER_OFF, %d) : Invalid Parameter\n"), _T(__FUNCTION__), dwIndex));
            SetLastError (ERROR_INVALID_PARAMETER);
            bRet = FALSE;
        }

        LeaveCriticalSection(&csPowerCon);

        break;
    case IOCTL_PWRCON_SET_SYSTEM_LEVEL:
        RETAILMSG(PWC_ZONE_DVS_CHANGE, (_T("Set System Level\n")));
        return KernelIoControl(IOCTL_HAL_SET_SYSTEM_LEVEL, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);
        break;
    case IOCTL_PWRCON_PROFILE_DVS:
        RETAILMSG(PWC_ZONE_DVS_PROFILE, (_T("Call HAL Profiler DVS Io Control\n")));        
        return KernelIoControl(IOCTL_HAL_PROFILE_DVS, pBufIn, dwLenIn, pBufOut, dwLenOut, pdwActualOut);        
        break;
    }

    return bRet;
}

// EOF
