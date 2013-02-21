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

   PowerButton.c   Power Controller Driver

Abstract:

   Stream interface of Power Button Driver (MDD).
   In SMDK6410, Powerbutton is mapped to EINT11, SW9 at default

Functions:



Notes:

--*/

#include "precomp.h"

#define SLEEP_AGING_TEST    (FALSE)     //< OEMs can enable this definition to do aging test for sleep/wakeup

static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static DWORD g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
static DWORD g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;
static HANDLE g_hEventPowerBtn = NULL;
static HANDLE g_hEventResetBtn = NULL;
static HANDLE g_hThreadPowerBtn = NULL;
static HANDLE g_hThreadResetBtn = NULL;
static BOOL g_bExitThread = FALSE;

BOOL g_bResetButtonEnabled = TRUE;

#define ETHKITL_VALNAME     TEXT("EthernetKITLInterruptEnabled") // Value name to look for in the active key
#define DEVKEY_VALNAME      TEXT("Key")                          // Device key in \Drivers\Built-In

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
        TEXT("Power Up"),               /* 6  */        \
        TEXT("Power Down"),             /* 7  */        \
        TEXT("Event Hook"),             /* 8  */        \
    },                                                  \
    (PWRBTN_ZONES)                               \
};


HKEY GetDeviceRegKey(LPTSTR pszActiveKey)
{
    HKEY    hActiveKey = NULL;  // handle to driver's active key
    DWORD   dwValueType;        
    DWORD   dwValueLength = 0;  
    LPTSTR   szDeviceKey = NULL; // name of device key; value associated with "Key"
    HKEY    hDeviceKey = NULL;   // handle to device key; handle to "Key"

    if ((ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszActiveKey, 0, 0, &hActiveKey)) || (!hActiveKey)) 
    {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T("PWR_Init: Failed to open active key(%s)\r\n"), pszActiveKey));
        goto Exit;
    }

    // query the value of "Key" with @dwValueLength=0, to determine the actual
    // length of the value (so as to allocate the exact amount of memory)
    //
    if (ERROR_SUCCESS == RegQueryValueEx(hActiveKey, DEVKEY_VALNAME, NULL, &dwValueType, NULL, &dwValueLength)) 
    {
        // allocate just enough memory to store the value of "Key"
        szDeviceKey = (LPTSTR)LocalAlloc(LPTR, dwValueLength);
        if (szDeviceKey) {

            // read the actual value of "Key" and null terminate the target buffer
            if ( ERROR_SUCCESS == RegQueryValueEx(hActiveKey, DEVKEY_VALNAME, NULL, &dwValueType, (PBYTE)szDeviceKey, &dwValueLength)
                && (dwValueLength != 0)) 
            {            
                szDeviceKey[(dwValueLength / sizeof(TCHAR)) - 1] = 0;
    
                // open the device key
                if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDeviceKey, 0, 0, &hDeviceKey)) 
                {
                    DEBUGMSG(ZONE_INIT, (_T("AtaLoadRegyKey> Failed to open %s\r\n"), szDeviceKey));
                    hDeviceKey = NULL;
                }
            }
        }
    }

Exit:
    if (hActiveKey)
    {
        RegCloseKey(hActiveKey);
    }
    if (szDeviceKey) 
    {
        LocalFree(szDeviceKey);
    }
    return hDeviceKey;
}


INT WINAPI PowerButtonThread(void)
{
    DWORD nBtnCount = 0;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] ++%s()\r\n"), _T(__FUNCTION__)));

    while(!g_bExitThread)
    {
        WaitForSingleObject(g_hEventPowerBtn, INFINITE);

        if(g_bExitThread)
        {
            break;
        }

        Button_pwrbtn_disable_interrupt();              // Mask EINT
        Button_pwrbtn_clear_interrupt_pending();        // Clear Interrupt Pending

        InterruptDone(g_dwSysIntrPowerBtn);

#if !(SLEEP_AGING_TEST)
        // Normal Button Push/Release Operation
        // Enter in when the power button is pushed
        // Hold in loop
        // Loop out when the power button is released
        while(Button_pwrbtn_is_pushed())
        {
            // Wait for Button Released...
            Sleep(10);
        }
#endif

        nBtnCount++;
        RETAILMSG(PWR_ZONE_EVENT_HOOK, (_T("[PWR] Power Button Event [%d]\r\n"), nBtnCount));

        // In the Windows Mobile, "PowerPolicyNotify(PPN_POWERBUTTONPRESSED, 0);" can be used
        SetSystemPowerState(NULL, POWER_STATE_SUSPEND, POWER_FORCE);

        Button_pwrbtn_enable_interrupt();            // UnMask EINT
#if (SLEEP_AGING_TEST)
        // To do Sleep/Wakeup aging test 
        SetEvent(g_hEventPowerBtn);
#endif
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] --%s()\r\n"), _T(__FUNCTION__)));

    return 0;
}

INT WINAPI ResetButtonThread(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] ++%s()\r\n"), _T(__FUNCTION__)));

    while(!g_bExitThread)
    {
        WaitForSingleObject(g_hEventResetBtn, INFINITE);

        if(g_bExitThread)
        {
            break;
        }

        Button_rstbtn_disable_interrupt();              // Mask EINT
        Button_rstbtn_clear_interrupt_pending();        // Clear Interrupt Pending

        InterruptDone(g_dwSysIntrResetBtn);

        RETAILMSG(PWR_ZONE_EVENT_HOOK, (_T("[PWR] Reset Button Event\r\n")));

        SetSystemPowerState(NULL, POWER_STATE_RESET, POWER_FORCE);
        //KernelIoControl(IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL);

        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] Soft Reset Failed\r\n")));

        Button_rstbtn_enable_interrupt();                // UnMask EINT
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR:INF] --%s()\r\n"), _T(__FUNCTION__)));

    return 0;
}

static BOOL
AllocResources(void)
{
    DWORD dwIRQ;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s()\r\n"), _T(__FUNCTION__)));

    //------------------
    // GPIO Controller SFR
    //------------------
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    g_pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (g_pGPIOReg == NULL)
    {
        RETAILMSG(PWR_ZONE_ERROR,(_T("[PWR:ERR] %s() : pGPIOReg MmMapIoSpace() Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }

    //--------------------
    // Power Button Interrupt
    //--------------------
    dwIRQ = IRQ_EINT9;
    g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
    g_hEventPowerBtn = NULL;

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &g_dwSysIntrPowerBtn, sizeof(DWORD), NULL))
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : IOCTL_HAL_REQUEST_SYSINTR Power Button Failed \n\r"), _T(__FUNCTION__)));
        g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
        return FALSE;
    }

    g_hEventPowerBtn = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == g_hEventPowerBtn)
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateEvent() Power Button Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }

    if (!(InterruptInitialize(g_dwSysIntrPowerBtn, g_hEventPowerBtn, 0, 0)))
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : InterruptInitialize() Power Button Failed \n\r"), _T(__FUNCTION__)));
        return FALSE;
    }

    if (g_bResetButtonEnabled)
    {
        //--------------------
        // Reset Button Interrupt
        //--------------------
        dwIRQ = IRQ_EINT11;
        g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;
        g_hEventResetBtn = NULL;

        if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &g_dwSysIntrResetBtn, sizeof(DWORD), NULL))
        {
            RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : IOCTL_HAL_REQUEST_SYSINTR Reset Button Failed \n\r"), _T(__FUNCTION__)));
            g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;
            return FALSE;
        }

        g_hEventResetBtn = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(NULL == g_hEventResetBtn)
        {
            RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateEvent() Reset Button Failed \n\r"), _T(__FUNCTION__)));
            return FALSE;
        }

        if (!(InterruptInitialize(g_dwSysIntrResetBtn, g_hEventResetBtn, 0, 0)))
        {
            RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : InterruptInitialize() Reset Button Failed \n\r"), _T(__FUNCTION__)));
            return FALSE;
        }
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));

    return TRUE;
}

static void
ReleaseResources(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s()\r\n"), _T(__FUNCTION__)));

    if (g_pGPIOReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
        g_pGPIOReg = NULL;
    }

    if (g_dwSysIntrPowerBtn != SYSINTR_UNDEFINED)
    {
        InterruptDisable(g_dwSysIntrPowerBtn);
    }

    if (g_hEventPowerBtn != NULL)
    {
        CloseHandle(g_hEventPowerBtn);
    }

    if (g_dwSysIntrPowerBtn != SYSINTR_UNDEFINED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_dwSysIntrPowerBtn, sizeof(DWORD), NULL, 0, NULL);
    }

    if (g_dwSysIntrResetBtn != SYSINTR_UNDEFINED)
    {
        InterruptDisable(g_dwSysIntrResetBtn);
    }

    if (g_hEventResetBtn != NULL)
    {
        CloseHandle(g_hEventResetBtn);
    }

    if (g_dwSysIntrResetBtn != SYSINTR_UNDEFINED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &g_dwSysIntrResetBtn, sizeof(DWORD), NULL, 0, NULL);
    }

    g_pGPIOReg = NULL;

    g_dwSysIntrPowerBtn = SYSINTR_UNDEFINED;
    g_dwSysIntrResetBtn = SYSINTR_UNDEFINED;

    g_hEventPowerBtn = NULL;
    g_hEventResetBtn = NULL;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));
}

static void InitInterrupt(void)
{
    // Interrupt Disable and Clear Pending
    Button_pwrbtn_disable_interrupt();

    // Initialize Port as External Interrupt
    Button_pwrbtn_port_initialize();

    // Interrupt Siganl Method and Filtering
    Button_pwrbtn_set_interrupt_method(EINT_SIGNAL_FALL_EDGE);
    Button_pwrbtn_set_filter_method(EINT_FILTER_DELAY, 0);

    // Clear Interrupt Pending
    Button_pwrbtn_clear_interrupt_pending();

    // Enable Interrupt
    Button_pwrbtn_enable_interrupt();

    if (g_bResetButtonEnabled)
    {
        // Interrupt Disable and Clear Pending
        Button_rstbtn_disable_interrupt();

        // Initialize Port as External Interrupt
        Button_rstbtn_port_initialize();

        // Interrupt Siganl Method and Filtering
        Button_rstbtn_set_interrupt_method(EINT_SIGNAL_FALL_EDGE);
        Button_rstbtn_set_filter_method(EINT_FILTER_DELAY, 0);

        // Clear Interrupt Pending
        Button_rstbtn_clear_interrupt_pending();

        // Enable Interrupt
        Button_rstbtn_enable_interrupt();
    }
}


BOOL
DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DEBUGREGISTER(hinstDll);
        DEBUGMSG(PWR_ZONE_INIT, (_T("[PWR] %s() : Process Attach\r\n"), _T(__FUNCTION__)));
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DEBUGMSG(PWR_ZONE_INIT, (_T("[PWR] %s() : Process Detach\r\n"), _T(__FUNCTION__)));
    }

    return TRUE;
}

// After Wake Up, This code will be called in single-threaded stage.
// Button Driver Monitor thread should get interrupt from external interrupt source in multi-threaded stage.
// then set System Power State to Resume in IST
// So, there are no need to disable interrupt for Power Button, and to clear interrupt pending bit in ISR handler.
BOOL
PWR_PowerUp(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));

    Button_rstbtn_clear_interrupt_pending();
    Button_rstbtn_enable_interrupt();

    return TRUE;
}

BOOL
PWR_PowerDown(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));

    // Interrupt Disable and Clear Pending
    Button_pwrbtn_disable_interrupt();
    Button_pwrbtn_clear_interrupt_pending();
    Button_rstbtn_disable_interrupt();
    Button_rstbtn_clear_interrupt_pending();

    return TRUE;
}

BOOL PWR_Deinit(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));

    g_bExitThread = TRUE;

    if (g_hThreadPowerBtn)        // Make Sure if thread is exist
    {
        Button_pwrbtn_disable_interrupt();
        Button_pwrbtn_clear_interrupt_pending();

        // Signal Thread to Finish
        SetEvent(g_hEventPowerBtn);
        // Wait for Thread to Finish
        WaitForSingleObject(g_hThreadPowerBtn, INFINITE);
        CloseHandle(g_hThreadPowerBtn);
        g_hThreadPowerBtn = NULL;
    }

    if (g_hThreadResetBtn)        // Make Sure if thread is exist
    {
        Button_rstbtn_disable_interrupt();
        Button_rstbtn_clear_interrupt_pending();

        // Signal Thread to Finish
        SetEvent(g_hEventResetBtn);
        // Wait for Thread to Finish
        WaitForSingleObject(g_hThreadResetBtn, INFINITE);
        CloseHandle(g_hThreadResetBtn);
        g_hThreadResetBtn = NULL;
    }

    ReleaseResources();

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));

    return TRUE;
}

DWORD
PWR_Init(DWORD dwContext)
{
    LPTSTR   szActiveKey = (LPTSTR)dwContext; // name of driver's active key
    HKEY     hDeviceKey = NULL;               // handle to driver's reg key
    DWORD    dwEthKitlIntEnabled;
    DWORD    dwValueLength;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] ++%s(0x%08x)\r\n"), _T(__FUNCTION__), dwContext));

    // open the driver's registry key and look if Ethernet KITL interrupt is enabled
    // This key is set by KITL during its initialization.
    // if yes, it clashes with reset button interrupt and so disable reset button
    //
    hDeviceKey = GetDeviceRegKey(szActiveKey);
    if (!hDeviceKey) 
    {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T("PWR_Init: Failed to open active key(%s)\r\n"), szActiveKey));
        goto CleanUp;
    }

    dwValueLength = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueEx(hDeviceKey, ETHKITL_VALNAME, NULL, NULL, (LPBYTE)&dwEthKitlIntEnabled, &dwValueLength))
    {
        if (dwEthKitlIntEnabled == 1)
        {
            g_bResetButtonEnabled = FALSE;
        }
    }
    RegCloseKey(hDeviceKey);
    
    if (AllocResources() == FALSE)
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : AllocResources() Failed \n\r"), _T(__FUNCTION__)));
        goto CleanUp;
    }

    Button_initialize_register_address((void *)g_pGPIOReg);

    // Enable Interrupt
    InitInterrupt();
    
    // Create Power Button Thread
    g_hThreadPowerBtn = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PowerButtonThread, NULL, 0, NULL);
    if (g_hThreadPowerBtn == NULL )
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateThread() Power Button Failed \n\r"), _T(__FUNCTION__)));
        goto CleanUp;
    }

    if (g_bResetButtonEnabled)
    {
        // Create Reset Button Thread
        g_hThreadResetBtn = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ResetButtonThread, NULL, 0, NULL);
        if (g_hThreadResetBtn == NULL )
        {
            RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] %s() : CreateThread() Reset Button Failed \n\r"), _T(__FUNCTION__)));
            goto CleanUp;
        }
    }
    
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] --%s()\r\n"), _T(__FUNCTION__)));

    return TRUE;

CleanUp:

    RETAILMSG(PWR_ZONE_ERROR, (_T("[PWR:ERR] --%s() : Failed\r\n"), _T(__FUNCTION__)));

    PWR_Deinit(0);

    return FALSE;
}

DWORD
PWR_Open(DWORD pContext, DWORD dwAccess, DWORD dwShareMode)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, dwAccess, dwShareMode));

    return TRUE;
}

BOOL
PWR_Close(DWORD pContext)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x)\r\n"), _T(__FUNCTION__), pContext));

    return TRUE;
}

DWORD
PWR_Read (DWORD pContext,  LPVOID pBuf, DWORD Len)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, pBuf, Len));

    return (0);    // End of File
}

DWORD
PWR_Write(DWORD pContext, LPCVOID pBuf, DWORD Len)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, pBuf, Len));

    return (0);    // Number of Byte
}

DWORD
PWR_Seek (DWORD pContext, long pos, DWORD type)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, pos, type));

    return (DWORD)-1;    // Failure
}

BOOL
PWR_IOControl(DWORD pContext, DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[PWR] %s(0x%08x, 0x%08x)\r\n"), _T(__FUNCTION__), pContext, dwCode));

    return FALSE;    // Failure
}

