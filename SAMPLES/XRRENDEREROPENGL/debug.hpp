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

#define ZONE_ID_ERROR     0x0001
#define ZONE_ID_WARNING   0x0002
#define ZONE_ID_INFO      0x0004
#define ZONE_ID_FUNCTION  0x0008
#define ZONE_ID_SURFACE   0x0010
#define ZONE_ID_RENDER    0x0020
#define ZONE_ID_MEMORY    0x0040
#define ZONE_ID_PERF      0x0080
#define ZONE_ID_SHADERS   0x0100
#define ZONE_ID_SERIALIZE 0x0200
#define ZONE_ID_CONFIGS   0x0400
#define ZONE_ID_VERBOSE   0x0800

#define ZONE_ERROR      DEBUGZONE(0)
#define ZONE_WARNING    DEBUGZONE(1)
#define ZONE_INFO       DEBUGZONE(2)
#define ZONE_FUNCTION   DEBUGZONE(3)
#define ZONE_SURFACE    DEBUGZONE(4)
#define ZONE_RENDER     DEBUGZONE(5)
#define ZONE_MEMORY     DEBUGZONE(6)
#define ZONE_PERF       DEBUGZONE(7)
#define ZONE_SHADERS    DEBUGZONE(8)
#define ZONE_SERIALIZE  DEBUGZONE(9)
#define ZONE_CONFIGS    DEBUGZONE(10)
#define ZONE_VERBOSE    DEBUGZONE(11)

extern DBGPARAM dpCurSettings;
