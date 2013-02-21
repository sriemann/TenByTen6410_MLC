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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
//
//------------------------------------------------------------------------------
//
//  File: bkldrvmain.c
//
//  Backlight driver source code
//
#include <windows.h>
#include <windev.h>
#include <pnp.h>
#include <pm.h>
#include <strsafe.h>
#include "BKLi.h"
#include "BKLPDD.h"


DBGPARAM dpCurSettings = {
    TEXT("Backlight"), {
        TEXT("Backlight"), TEXT("Function"), TEXT("Misc"), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""), TEXT("Warning"), TEXT("Error"),
    },
    0xC003
};


#define BACKLIGHT_REGKEY TEXT("ControlPanel\\Backlight")

#define BKL_EVENT_REG            0       // registry change
#define BKL_EVENT_POWER_MSG      1       // power status change
#define BKL_EVENT_EXIT           2       // we're exiting
#define BKL_EVENT_DISPLAY_MSG    3       // display device notification
#define BKL_NUM_EVENTS           4

#define TURNOFFIMMEDIATELY -1

// device notification queue parameters
#define PNP_QUEUE_ENTRIES       1       // assumes we have only 1 display driver interface being advertised
//"The notifications sent to hMsgQ are a sequence of DEVDETAIL structures 
//with extra TCHAR types appended to account for the instance names":
#define PNP_QUEUE_SIZE          (PNP_QUEUE_ENTRIES * (sizeof(DEVDETAIL) + (MAX_NAMELEN * sizeof(TCHAR))))


static const UCHAR   DeviceStateMasks[5]={
    DX_MASK(D0),
    DX_MASK(D1),
    DX_MASK(D2),
    DX_MASK(D3),
    DX_MASK(D4),
};

BOOL ConvertStringToGuid(LPCTSTR GuidString, GUID *Guid )
{
  // ConvertStringToGuid
  // this routine converts a string into a GUID and returns TRUE if the
  // conversion was successful.

  // Local variables.
  UINT Data4[8];
  int  Count;
  BOOL Ok = FALSE;
  LPWSTR GuidFormat = L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";

  DEBUGCHK(Guid != NULL && GuidString != NULL);

  __try 
  {

  if (_stscanf(GuidString, GuidFormat, &Guid->Data1, 
        &Guid->Data2, &Guid->Data3, &Data4[0], &Data4[1], &Data4[2], &Data4[3], 
        &Data4[4], &Data4[5], &Data4[6], &Data4[7]) == 11) {

      for (Count = 0; Count < (sizeof(Data4) / sizeof(Data4[0])); Count++) {

        Guid->Data4[Count] = (UCHAR) Data4[Count];
      }
    }

    Ok = TRUE;
  }
  __except(EXCEPTION_EXECUTE_HANDLER) 
  {
        RETAILMSG(ZONE_ERROR, (TEXT("exception in convertstringtoguid\r\n")));
  }

  return Ok;
}




void UpdateACStatus(BKL_MDD_INFO *pBKLinfo)
{
    SYSTEM_POWER_STATUS_EX2 SysPower;

    static fFirstTime = TRUE;

    // make sure that GWES APIs ready before calling: 
    if (WAIT_OBJECT_0 != WaitForAPIReady(SH_GDI, INFINITE))
    {
        RETAILMSG(1, (TEXT("Backlight driver: WaitForAPIReady failed.\r\n")));
        return;
    }

    if (pBKLinfo->pfnGetSystemPowerStatusEx2) 
    {
        if ((*pBKLinfo->pfnGetSystemPowerStatusEx2)(&SysPower, sizeof(SysPower), FALSE))
        {
            if (SysPower.ACLineStatus & AC_LINE_ONLINE)
            {
                pBKLinfo->fOnAC = TRUE;
            }
            else
            {
                pBKLinfo->fOnAC = FALSE;
            }
        }
        else
        {
            RETAILMSG(1, (TEXT("GetSystemPowerStstusEx2 failed with error 0x%x.\r\n"), GetLastError()));
        }
    }
    else 
    {
        // There are no battery APIs so assume that we are always on AC power.
        pBKLinfo->fOnAC = TRUE;
		RETAILMSG(1, (TEXT("AC is on.\r\n")));
    }
    
    return;    
    
}
/* 
    A driver that is issued a request to enter a power state not supported by its device 
    enters the next available power state supported. 
    For example, if the Power Manager requests that it enter D2 and does not support D2, 
    the device can enter D3 or D4 instead. 
    If a device is requested to enter D3 and cannot wake up the system then 
    it should enter D4 and power off rather than staying in standby

    All drivers must support at least D0

    Note: since the default PDA power manager will never ask a driver to go to a state
    that the driver has reported it supports, most of this code will never be called.
    The code could be made more efficient (no branching) if deemed necessary

*/
BOOL GetBestSupportedState(BKL_MDD_INFO *pBKLinfo, CEDEVICE_POWER_STATE ReqDx, CEDEVICE_POWER_STATE* SetDx)
{
    BOOL fRet = TRUE;   // assume there's a suitable state we can go to
    
    switch(ReqDx)
    {
        case D0:
            *SetDx = D0;
            break;
        case D1:
            if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D1])
            {
                *SetDx = D1;
            }            
            else if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D2])
            {
                *SetDx = D2;
            }
            else if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D3])
            {
                *SetDx = D3;
            }
            else if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D4])
            {
                *SetDx = D4;
            }
            else
            {
                fRet = FALSE;
            }
            break;
        
        case D2:
            if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D2])
            {
                *SetDx = D2;
            }
            else if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D3])
            {
                *SetDx = D3;
            }
            else if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D4])
            {
                *SetDx = D4;
            }                       
            else
            {
                fRet = FALSE;
            }
            break;

        case D3:
            if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D3])
            {
                *SetDx = D3;
            }
            else if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D4])
            {
                *SetDx = D4;
            }
            else
            {
                fRet = FALSE;
            }
            break;
        case D4:
            if(pBKLinfo->ucSupportedStatesMask & DeviceStateMasks[D4])
            {
                *SetDx = D4;
            }
            else
            {
                fRet = FALSE;
            }
            break;  
        default:
            ASSERT(FALSE);
            break;
            
    }   

    return fRet;
}


/*
    Has the user checked the 'turn on when key pressed...' option for the current power status?
*/
BOOL IsTapOn(BKL_MDD_INFO *pBKLinfo)
{
    if(pBKLinfo->fOnAC)
    {
        return (pBKLinfo->fExternalTapOn? TRUE : FALSE);
    }
    else
    {
        return (pBKLinfo->fBatteryTapOn? TRUE : FALSE);
    }
    
}

DWORD GetTimeout(BKL_MDD_INFO *pBKLinfo)
{
    if(pBKLinfo->fOnAC)
    {
        return pBKLinfo->dwACTimeout;        
    }
    else
    {
        return pBKLinfo->dwBattTimeout;        
    }

}

/*
    Reads the 'turn on when key pressed...' registry settings from the registry
*/
void BacklightUpdateMDDRegSettings(BKL_MDD_INFO *pBKLinfo)
{
    DWORD   retCode;
    BYTE    ValueData[MAX_PATH];
    DWORD    dwType;
    void    *bData = ValueData;
    DWORD   cbData;
    HKEY    hKey;

    DEBUGMSG(ZONE_BACKLIGHT,(TEXT("+BacklightReadMDDReg\r\n")));

    retCode = RegOpenKeyEx (HKEY_CURRENT_USER, BACKLIGHT_REGKEY, 0, KEY_ALL_ACCESS, &hKey);
    if (retCode == ERROR_SUCCESS)
    {
        //Battery Tap
        dwType=REG_DWORD;
        cbData = MAX_PATH;
        retCode = RegQueryValueEx(hKey, TEXT("BacklightOnTap"), NULL, &dwType, (LPBYTE) bData, (LPDWORD)&cbData);
        if (retCode == ERROR_SUCCESS)
        {
            pBKLinfo->fBatteryTapOn = (*(DWORD *)bData );
        }
        //External Tap
        dwType=REG_DWORD;
        cbData = MAX_PATH;
        retCode = RegQueryValueEx(hKey, TEXT("ACBacklightOnTap"), NULL, &dwType, (LPBYTE) bData, (LPDWORD)&cbData);
        if (retCode == ERROR_SUCCESS)
        {
            pBKLinfo->fExternalTapOn = (*(DWORD *)bData );
        }
        //Backlight on battery timeout (we may need to turn the backlight off)
        dwType=REG_DWORD;
        cbData = MAX_PATH;
        retCode = RegQueryValueEx(hKey, TEXT("BatteryTimeout"), NULL, &dwType, (LPBYTE) bData, (LPDWORD)&cbData);
        if (retCode == ERROR_SUCCESS)
        {
            pBKLinfo->dwBattTimeout = (*(DWORD *)bData );
        }
        //Backlight on AC timeout
        dwType=REG_DWORD;
        cbData = MAX_PATH;        
        retCode = RegQueryValueEx(hKey, TEXT("ACTimeout"), NULL, &dwType, (LPBYTE) bData, (LPDWORD)&cbData);
        if (retCode == ERROR_SUCCESS)
        {
            pBKLinfo->dwACTimeout = (*(DWORD *)bData );
        }

        //Backlight brightness for battery
        dwType=REG_DWORD;
        cbData = MAX_PATH;          
        retCode = RegQueryValueEx(hKey, TEXT("BrightNess"), NULL, &dwType, (LPBYTE) bData, (LPDWORD)&cbData);
        if (retCode == ERROR_SUCCESS)
        {
            pBKLinfo->dwBrightness= (*(DWORD *)bData );
        }

        //Backlight brightness for AC
        dwType=REG_DWORD;
        cbData = MAX_PATH;          
        retCode = RegQueryValueEx(hKey, TEXT("ACBrightNess"), NULL, &dwType, (LPBYTE) bData, (LPDWORD)&cbData);
        if (retCode == ERROR_SUCCESS)
        {
            pBKLinfo->dwACBrightness= (*(DWORD *)bData );
        }

        
    }

    goto exit;
    
exit:
    if(hKey)
    {
        RegCloseKey(hKey);
    }

    DEBUGMSG(ZONE_BACKLIGHT,(TEXT("-BacklightReadMDDReg\r\n")));
    
}

/*
    Ask power manager to set the BKL power state.
    PM will not ask BKL to change states again until
    called with PwrDeviceUnspecified
*/
void BKL_SetDevicePower(BKL_MDD_INFO *pBKLinfo, CEDEVICE_POWER_STATE bklPowerState)
{
    SetDevicePower(pBKLinfo->szName, POWER_NAME, bklPowerState);
    
    return;
}

// Update the backlight reg settings or power status
// Set power requirment for backlight to D4 (off) if 'Tap On' setting unchecked
// and unspecified if 'Tap On' setting just checked. Similarly if 'Turn Off Backlight' timeout option
// has just been selected/deselected:
void UpdateBacklight(BKL_MDD_INFO *pBKLinfo, DWORD dwReason)
{
    BOOL    fTapOnPrev, fTapOnNew;
    DWORD   dwTimeoutPrev, dwTimeoutNew;
    BOOL    fSetPowerOff = FALSE;
    BOOL    fReleasePwrOff = FALSE;
    
    fTapOnPrev = IsTapOn(pBKLinfo);
    dwTimeoutPrev = GetTimeout(pBKLinfo);

    switch(dwReason)
    {
        case BKL_EVENT_REG:
            // Have registry keys changed?
            BacklightUpdateMDDRegSettings(pBKLinfo);  
            break;

        case BKL_EVENT_POWER_MSG:
            // AC Status changed?
            UpdateACStatus(pBKLinfo);
            break;
    };
    
    dwTimeoutNew = GetTimeout(pBKLinfo);
    fTapOnNew = IsTapOn(pBKLinfo);
    
    // 'Tap On' settings just unchecked or 'Turn off backlight' timer option just selected:
    fSetPowerOff = ( ((dwTimeoutPrev != TURNOFFIMMEDIATELY) && (dwTimeoutNew == TURNOFFIMMEDIATELY)) \
                    || ((fTapOnPrev && !fTapOnNew)));
    // 'Tap On' setting just checked or 'Turn off backlight' timer option just deselected:
    fReleasePwrOff = (((dwTimeoutPrev == TURNOFFIMMEDIATELY) && (dwTimeoutNew != TURNOFFIMMEDIATELY) ) \
                    || (!fTapOnPrev && fTapOnNew));
    
    if(fSetPowerOff)   
    {
        BKL_SetDevicePower(pBKLinfo, D4);
    }
    else if (fReleasePwrOff) 
    {
        BKL_SetDevicePower(pBKLinfo, PwrDeviceUnspecified);
    }
    

}
/*

Monitors changes in reg keys 'BacklightOnTap' and 'ACBacklightOnTap'
and changes in power status (AC->DC / DC->AC)

*/
DWORD fnBackLightThread(PVOID pvArgument)
{ 
    DWORD    dwResult;
    MSGQUEUEOPTIONS msgopts;
    HANDLE  hPwrNotification = NULL;
    HANDLE  hPowerNotificationMsgs = NULL;    
    HKEY    hKey = NULL;
    HANDLE  WaitEvents[BKL_NUM_EVENTS];    
    HANDLE  hEventRegistryChange = NULL;
    DWORD   dwSize;    
    DWORD   dwFlags;
    HANDLE  hDisplayNotifications = NULL;
    HANDLE  hDisplayNotificationMsgs = NULL;
    GUID    idInterface;
    BKL_MDD_INFO *pBKLinfo = NULL;
    
    DEBUGMSG(ZONE_BACKLIGHT,(TEXT("+fnBackLightRegThread\r\n")));

    // Verify context
    if(! pvArgument)
    {
        RETAILMSG(ZONE_ERROR, (L"ERROR: BacklightThread: "
            L"Incorrect context paramer\r\n" ));

        return FALSE;
    }

    pBKLinfo = (BKL_MDD_INFO*) pvArgument;


    // create msg queue for power status change notification (AC->Battery and vice versa)
    memset(&msgopts, 0, sizeof(msgopts));
    msgopts.dwSize = sizeof(msgopts);
    msgopts.dwFlags = 0;
    msgopts.dwMaxMessages = 0;                  // no max number of messages
    msgopts.cbMaxMessage = sizeof(POWER_BROADCAST);   // max size of each msg
    msgopts.bReadAccess = TRUE;
    hPowerNotificationMsgs = CreateMsgQueue(NULL, &msgopts);

    if (!hPowerNotificationMsgs) 
    {
        RETAILMSG(ZONE_ERROR, (TEXT("BKL: Create message queue failed\r\n")));
        goto exit;
    }
    // request notification of power status changes:
    hPwrNotification = RequestPowerNotifications(hPowerNotificationMsgs, PBT_POWERSTATUSCHANGE);
    if (!hPwrNotification) 
    {
        RETAILMSG(ZONE_ERROR, (TEXT("BKL: register power notification failed\r\n")));
        goto exit;
    }

    // create msg queue for display device interface notifications:
    msgopts.cbMaxMessage = PNP_QUEUE_SIZE;
    hDisplayNotificationMsgs = CreateMsgQueue(NULL, &msgopts);
    if (!hDisplayNotificationMsgs) 
    {
        RETAILMSG(ZONE_ERROR, (TEXT("BKL: Create message queue failed\r\n")));
        goto exit;
    }

    if(!ConvertStringToGuid(PMCLASS_DISPLAY, &idInterface)) 
    {
        RETAILMSG(ZONE_ERROR, (TEXT("can't convert PMCLASS_DISPLAY string to GUID\r\n")));  
        goto exit;
    }

    // get display driver name (required to keep display driver on with SetPowerRequirement):
    if(!(hDisplayNotifications = RequestDeviceNotifications(&idInterface, hDisplayNotificationMsgs, TRUE)))
    {
        RETAILMSG(ZONE_ERROR, (TEXT("RequestDeviceNotifications failed\r\n")));
        goto exit;
    }    
    
    dwResult = RegOpenKeyEx(HKEY_CURRENT_USER, BACKLIGHT_REGKEY, 0, KEY_NOTIFY, &hKey);
    if(ERROR_SUCCESS  != dwResult)
    {
        goto exit;
    }
    // Request notification of backlight registry changes:
    hEventRegistryChange = CeFindFirstRegChange(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET);
    if(INVALID_HANDLE_VALUE == hEventRegistryChange)
    {   
        RETAILMSG(ZONE_ERROR, (TEXT("BKL: CeFindFirstRegChange failed\r\n")));        
        goto exit;
    }
    RegCloseKey(hKey);
    hKey = NULL;

    BacklightUpdateMDDRegSettings(pBKLinfo);
    UpdateACStatus(pBKLinfo);

    //call BacklightRegChanged() is just to initialize the brightness setting for the backlight
    if(IsAcOn(pBKLinfo))
        BacklightRegChanged(pBKLinfo->dwACBrightness);
    else
        BacklightRegChanged(pBKLinfo->dwBrightness);

    WaitEvents[BKL_EVENT_REG] = hEventRegistryChange;
    WaitEvents[BKL_EVENT_POWER_MSG] = hPowerNotificationMsgs;
    WaitEvents[BKL_EVENT_EXIT] = pBKLinfo->hExitEvent;
    WaitEvents[BKL_EVENT_DISPLAY_MSG] = hDisplayNotificationMsgs;

    pBKLinfo->fExit = FALSE;
    
    while(!pBKLinfo->fExit)
    {
        dwResult = WaitForMultipleObjects(BKL_NUM_EVENTS, &WaitEvents[0], FALSE, INFINITE);
        switch(dwResult)
        {
            case(WAIT_OBJECT_0 + BKL_EVENT_REG):
            {
                DEBUGMSG(ZONE_BACKLIGHT,(TEXT("Backlight mdd registry change\r\n")));

                UpdateBacklight(pBKLinfo, BKL_EVENT_REG);

                CeFindNextRegChange(hEventRegistryChange);      

                // Tell PDD that the backlight reg settings changed:
                if(IsAcOn(pBKLinfo))
                    BacklightRegChanged(pBKLinfo->dwACBrightness);
                else
                    BacklightRegChanged(pBKLinfo->dwBrightness);
            }
            break;
            
            case (WAIT_OBJECT_0 + BKL_EVENT_POWER_MSG):
            {
                POWER_BROADCAST PwrMsgBuf;
                
                DEBUGMSG(ZONE_BACKLIGHT,(TEXT("Power status change to/from AC\r\n")));
                if (!ReadMsgQueue(hPowerNotificationMsgs, &PwrMsgBuf, sizeof(PwrMsgBuf), &dwSize, 0, &dwFlags)) 
                {
                    DEBUGMSG(ZONE_BACKLIGHT,(TEXT("ReadMsgQueue failed\r\n")));  
                    ASSERT(FALSE);
                }

                UpdateBacklight(pBKLinfo, BKL_EVENT_POWER_MSG);
                
                // Tell PDD that the backlight reg settings changed:
                if(IsAcOn(pBKLinfo))
                    BacklightRegChanged(pBKLinfo->dwACBrightness);
                else
                    BacklightRegChanged(pBKLinfo->dwBrightness);

                
            }
            break;

            case (WAIT_OBJECT_0 + BKL_EVENT_DISPLAY_MSG):
            {
                union 
                {
                    UCHAR deviceBuf[PNP_QUEUE_SIZE];
                    DEVDETAIL devDetail;
                } u;
                
                DEBUGMSG(ZONE_BACKLIGHT,(TEXT("Display driver interface notification\r\n")));
                if (!ReadMsgQueue(hDisplayNotificationMsgs, u.deviceBuf, PNP_QUEUE_SIZE, &dwSize, 0, &dwFlags)) 
                {
                    DEBUGMSG(ZONE_BACKLIGHT,(TEXT("ReadMsgQueue failed\r\n")));  
                    ASSERT(FALSE);
                }
                else if(dwSize >= sizeof(DEVDETAIL)) 
                {
                    PDEVDETAIL pDevDetail = &u.devDetail;

                    if(( (pDevDetail->cbName + sizeof(TCHAR) < sizeof(pBKLinfo->szDisplayInterface) ) 
                        // Check overflow AND underflow
                        && ( (int)(pDevDetail->cbName + sizeof(TCHAR)) > pDevDetail->cbName)
                        && (int)(pDevDetail->cbName) >= 0) )
                    {
                        memcpy(pBKLinfo->szDisplayInterface, pDevDetail->szName, pDevDetail->cbName);
                        // make sure it's null terminated:
                        pBKLinfo->szDisplayInterface[pDevDetail->cbName] = '\0';
                    } 
                }
                else
                {
                    DEBUGMSG(ZONE_BACKLIGHT,(TEXT("insufficient buffer for device message\r\n")));  
                }
            }
            break;

            case(WAIT_OBJECT_0 + BKL_EVENT_EXIT):
            {
                DEBUGMSG(ZONE_BACKLIGHT,(TEXT("Backlight exiting\r\n")));                
            }
            break;
  
            default:
            {
                ASSERT(FALSE);
            }
            
        }
        
    }

    exit:
        
    if (hPwrNotification)
    {
        StopPowerNotifications(hPwrNotification);
    }
    if (hPowerNotificationMsgs)
    {
        CloseMsgQueue(hPowerNotificationMsgs);
    }
    if (hDisplayNotifications)
    {
        StopDeviceNotifications(hDisplayNotifications);
    }    
    if (hDisplayNotificationMsgs)
    {
        CloseMsgQueue(hDisplayNotificationMsgs);
    }
    if(hEventRegistryChange)
    {
        CeFindCloseRegChange(hEventRegistryChange);
    }
    if(hKey)
    {
        RegCloseKey(hKey);
    }
    
    DEBUGMSG(ZONE_BACKLIGHT,(TEXT("-fnBackLightRegThread\r\n")));

    return 0;
}


BOOL IsAcOn(BKL_MDD_INFO *pBKLinfo)
{
    return (pBKLinfo->fOnAC?TRUE:FALSE);

}

