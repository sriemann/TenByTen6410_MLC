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
#include <windows.h>
#include "debug.hpp"


//------------------------------------------------------------------------------
//
//  Global:  dpCurSettings
//

DBGPARAM dpCurSettings =
{
    L"XRRendererOpenGL", 
        {
        L"Error",       L"Warning",     L"Info",        L"Function",
        L"Surface",     L"Render",      L"Memory",      L"Verbose",
        L"Perf",        L"Shaders",     L"Configs",     L"Mutex",
        L"Undefined",   L"Undefined",   L"Undefined",   L"Undefined" 
        },
        ZONE_ID_ERROR | ZONE_ID_WARNING | ZONE_ID_INFO 
};
