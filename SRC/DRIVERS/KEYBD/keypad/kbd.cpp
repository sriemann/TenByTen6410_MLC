//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2008. Samsung Electronics, co. ltd  All rights reserved.

Module Name:  

Abstract:

    This file implements the S3C6410 Keyboard function

Notes: 
--*/

#include <windows.h>
#include <ceddk.h>
#include <keybddbg.h>
#include <keybdpdd.h>
#include <laymgr.h>

#include "keymatrix.hpp"

extern KeyMatrix    *Keyboard;

UINT v_uiPddId;
PFN_KEYBD_EVENT v_pfnKeybdEvent;

void WINAPI KeybdPdd_PowerHandler(BOOL    bOff);
BOOL KeybdDriverInitializeAddresses(void);
static void WINAPI Matrix_PowerHandler(UINT uiPddId, BOOL fTurnOff);
static void WINAPI Matrix_ToggleLights( UINT uiPddId, KEY_STATE_FLAGS KeyStateFlags);

static KEYBD_PDD MatrixPdd = 
{
    MATRIX_PDD,
    _T("Matrix"),
    Matrix_PowerHandler,
    Matrix_ToggleLights
};


void
ReadRegDWORD(
    LPCWSTR    szKeyName,
    LPCWSTR szValueName,
    LPDWORD pdwValue
    )
{
    HKEY hKeybd;
    DWORD ValType;
    DWORD ValLen;
    DWORD status;

    //
    // Get the device key from the active device registry key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                szKeyName,
                0,
                0,
                &hKeybd);
    if (status!= ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ReadRegDWORD: RegOpenKeyEx(HLM\\%s) returned %d!!!\r\n"),
                  szKeyName, status));
        *pdwValue = 0;   // Fail
        return;
    }

    ValLen = sizeof(DWORD);
    status = RegQueryValueEx(       // Retrieve the value
                hKeybd,
                szValueName,
                NULL,
                &ValType,
                (PUCHAR)pdwValue,
                &ValLen);
    if (status != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("ReadRegDWORD: RegQueryValueEx(%s) returned %d\r\n"),
                  szValueName, status));
        *pdwValue = 0;   // Fail
    }

    DEBUGMSG(ZONE_ERROR, (_T("ReadRegDWORD(): %s -> %s is 0x%x\r\n"), szKeyName, szValueName, *pdwValue));

    RegCloseKey(hKeybd);
}   // OpenDeviceKey


static void WINAPI Matrix_PowerHandler(UINT uiPddId, BOOL fTurnOff)
{
   KeybdPdd_PowerHandler(fTurnOff);
}

static void WINAPI Matrix_ToggleLights( UINT uiPddId, KEY_STATE_FLAGS KeyStateFlags)
{
    SETFNAME(_T("Matrix_ToggleLights"));

    static const KEY_STATE_FLAGS ksfLightMask = KeyShiftCapitalFlag | 
    KeyShiftNumLockFlag | KeyShiftScrollLockFlag; 
    static KEY_STATE_FLAGS ksfCurr;

    KEY_STATE_FLAGS ksfNewState = (ksfLightMask & KeyStateFlags);

    if (ksfNewState != ksfCurr) 
    {
      DEBUGMSG(ZONE_PDD, (_T("%s: PDD %u: Changing light state\r\n"), pszFname, uiPddId));
      KeybdPdd_ToggleKeyNotification(ksfNewState);
      ksfCurr = ksfNewState;
    }

    return;
}


BOOL
WINAPI
Matrix_Entry(
    UINT uiPddId,
    PFN_KEYBD_EVENT pfnKeybdEvent,
    PKEYBD_PDD *ppKeybdPdd
    )
{
    SETFNAME(_T("Matrix_Entry"));

    BOOL fRet = FALSE;

    v_uiPddId = uiPddId;
    v_pfnKeybdEvent = pfnKeybdEvent;

    DEBUGMSG(ZONE_INIT, (_T("%s: Initialize Matrix ID %u\r\n"), pszFname, uiPddId));

    if(ppKeybdPdd == NULL)
    {
        goto leave;
    }

    *ppKeybdPdd = &MatrixPdd;

    if (Keyboard) 
    {
        fRet = TRUE;
        goto leave;
    }

    //    We always assume that there is a keyboard.
    Keyboard = new KeyMatrix;

    if (!KeybdDriverInitializeAddresses()) 
    {
        goto leave;
    }

    if (Keyboard)
    {
        Keyboard->KeybdPowerOn();
        Keyboard->IsrThreadStart();
    }

    fRet = TRUE;
        
leave:
    DEBUGMSG(ZONE_INIT, (_T("%s: Initialization complete\r\n"), pszFname));
    return fRet;
}

#ifdef DEBUG
// Verify function declaration against the typedef.
static PFN_KEYBD_PDD_ENTRY v_pfnKeybdEntry = Matrix_Entry;
#endif


