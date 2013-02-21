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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

@doc EX_TOUCH_DDI INTERNAL DRIVERS MDD TOUCH_PANEL

Module Name:  

@module tchbasic.c

Abstract:  
    This module contains the touch panel DLL entry point.<nl>


Functions:
TouchPanelDllEntry
Notes: 


--*/

#include    <windows.h>
#include    <types.h>
#include    <memory.h>
#include    <nkintr.h>
#include    <tchddi.h>
#include    <tchddsi.h>

PFN_TOUCH_PANEL_CALLBACK v_pfnCgrPointCallback;
PFN_TOUCH_PANEL_CALLBACK v_pfnCgrCallback = NULL;
extern ULONG   culReferenceCount;              //@globalvar ULONG | culReferenceCount | Count of attached threads
extern PFN_TOUCH_PANEL_CALLBACK v_pfnPointCallback;
extern HANDLE hThread;


/*++
Autodoc Information:

    @func BOOL | TouchPanelDllEntry |
    Dll entry point.

    @rdesc
    TRUE if the function succeeds. Otherwise, FALSE.

--*/
BOOL
TouchPanelDllEntry(
    HANDLE  hinstDll,    //@parm Process handle.
    DWORD   fdwReason,   //@parm Reason for calling the function.
    LPVOID  lpvReserved  //@parm Reserved, not used.
    )
{

    BOOL ReturnCode = TRUE;

    switch ( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER(hinstDll);
            DEBUGMSG( ZONE_FUNCTION, (TEXT("Dll Process Attach\r\n")) );
            DisableThreadLibraryCalls((HMODULE) hinstDll);
             //
             // Process is attaching.  We allow only 1 process to be attached.
             // If our global counter (maintained by the PDD) is greater than 0,
             //   error.
             //

            if ( DdsiTouchPanelAttach() > 1 )
            {
                DEBUGMSG( ZONE_FUNCTION, (TEXT("DdsiTouchPanelAttach > 1\r\n")) );
                DdsiTouchPanelDetach(); // if a process attach fails, the detach is
                 // never called. So adjust the count here.
                ReturnCode = FALSE;
            }

            break;
        case DLL_PROCESS_DETACH:
            DEBUGMSG( ZONE_FUNCTION,
                      (TEXT("Dll Process Detach\r\n")) );

             //
             // Process is detaching.
             // If the detaching process is the process that was allowed
             // to attach, we reset the callback functions,
             // reference count, disable the touch panel, and disconnect from the
             // logical interrupt.
             //
             //
            ASSERT(hThread==NULL);
            DdsiTouchPanelDetach();
            break;
    }
    return ( ReturnCode );
}

BOOL
WINAPI
DllMain (
    HANDLE hinstDLL,
    DWORD dwReason,
    LPVOID lpvReserved
    )
{
    return TouchPanelDllEntry(hinstDLL, dwReason, lpvReserved);
}

