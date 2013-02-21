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

#include "windows.h"
#include "debug.hpp"

extern HINSTANCE g_hInstance;

STDAPI_(BOOL) WINAPI DllMain(HANDLE hInst,DWORD Reason,LPVOID)
{
    switch(Reason) 
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls((HINSTANCE)hInst);
#ifndef SHIP_BUILD
            RETAILREGISTERZONES((HINSTANCE)hInst);
#endif
            g_hInstance = (HINSTANCE)hInst;
            DEBUGMSG(ZONE_FUNCTION,(TEXT("XamlRenderPlugin.DLL(OpenGL): DLL_PROCESS_ATTACH")));
            break;

        case DLL_PROCESS_DETACH:
            DEBUGMSG(ZONE_FUNCTION,(TEXT("XamlRenderPlugin.DLL(OpenGL): DLL_PROCESS_DETACH")));
            break;
    }

    return TRUE;
} 
