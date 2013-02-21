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
//  File: bkli.h
//
//  Backlight driver source code
//
#ifndef __BKLI_H
#define __BKLI_H

#pragma once


#ifdef DEBUG
#define ZONE_BACKLIGHT      DEBUGZONE(0)
#define ZONE_FUNCTION       DEBUGZONE(1)
#define ZONE_MISC           DEBUGZONE(2)
#define ZONE_WARN           DEBUGZONE(14)
#define ZONE_ERROR          DEBUGZONE(15)
#else
#define ZONE_BACKLIGHT      1
#define ZONE_ERROR          1
#endif


#define MAX_NAMELEN         128


typedef DWORD (WINAPI *PFN_GetSystemPowerStatusEx2) (
    PSYSTEM_POWER_STATUS_EX2 pSystemPowerStatusEx2,
    DWORD dwLen,
        BOOL fUpdate
    );


typedef struct
{
    TCHAR                   szName[MAX_NAMELEN];                // device name, eg "BKL1:"
    TCHAR                   szDisplayInterface[MAX_NAMELEN];   
    BOOL                    fBatteryTapOn;      // reg setting - do we turn on when screen/button tapped?
    BOOL                    fExternalTapOn;      // reg setting - do we turn on when screen/button tapped? 
    DWORD                   dwBattTimeout;  // reg setting - we only want this to deal with special cases 
                                                     // ('backlight off' currently, which PM doesn't know about)
    DWORD                   dwACTimeout; 
    BOOL                    fOnAC;              // are we currently on ac power?
    CEDEVICE_POWER_STATE    dwCurState;      // actual status (0=Off, 1=On)
    UCHAR                   ucSupportedStatesMask;  // which of D0-D4 driver supports
    HANDLE                  hExitEvent;
    HANDLE                  hBklThread;
    HANDLE                  hDDIPowerReq; // handle from PM for relasing display driver power requirement
    BOOL                    fExit; 
    DWORD                   dwPddContext; // Context for device specific PDD 

    PFN_GetSystemPowerStatusEx2 pfnGetSystemPowerStatusEx2;
    HINSTANCE                   hCoreDll;

    DWORD                    dwBrightness;
    DWORD                    dwACBrightness;
} BKL_MDD_INFO;




DWORD fnBackLightThread(PVOID pvArgument);
BOOL IsTapOn(BKL_MDD_INFO *pBKLinfo);
void UpdateACStatus(BKL_MDD_INFO *pBKLinfo);
BOOL GetBestSupportedState(BKL_MDD_INFO *pBKLinfo, CEDEVICE_POWER_STATE ReqDx, CEDEVICE_POWER_STATE* SetDx);
BOOL IsAcOn(BKL_MDD_INFO *pBKLinfo);


#endif 
