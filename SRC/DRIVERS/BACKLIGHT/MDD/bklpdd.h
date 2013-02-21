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
//  File: backlight.c
//
//  Backlight driver source code
//

#ifndef __BKLPDD_H
#define __BKLPDD_H

#include <pm.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
    Initialize hardware etc
    Returned DWORD will be passed to BacklightDeInit and should be used to store context if necessary
    pDeviceState should be set to the start state of the backlight (usually D0)
*/
DWORD BacklightInit(LPCTSTR pContext, LPCVOID lpvBusContext, CEDEVICE_POWER_STATE *pDeviceState);

/* 
Deinit platform and free any resources
*/
void BacklightDeInit(DWORD dwContext);

/*
    sets the backlight state (turns the backlight on and off)
    returns TRUE if successful
*/
BOOL BackLightSetState(DWORD dwContext, CEDEVICE_POWER_STATE SetState);

/*
    returns the supported device states
*/
UCHAR BacklightGetSupportedStates();


/* 
    Called when the MDD gets a backlight registry changed event
*/
void BacklightRegChanged(DWORD dwBrightness);


/* 
    For IOCTls that MDD doesn't know. ie non-pm IOCTLs
*/
DWORD BacklightIOControl(DWORD dwOpenContext, DWORD dwIoControlCode, LPBYTE lpInBuf, 
                          DWORD nInBufSize, LPBYTE lpOutBuf, DWORD nOutBufSize, 
                          LPDWORD lpBytesReturned);

#ifdef __cplusplus
}
#endif

#endif 
