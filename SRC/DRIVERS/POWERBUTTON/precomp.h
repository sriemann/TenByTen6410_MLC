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
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

   precomp.h

Abstract:

   Precompiled header to have definition and common structure

Functions:

    

Notes:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <bsp.h>
#include <pmplatform.h>
#include <assert.h>
#include "smdk6410_button.h"



#define __MODULE__  "S3C6410 Power Button Driver"

#define ZONEID_ERROR                0
#define ZONEID_WARNING              1
#define ZONEID_PERF                 2
#define ZONEID_TEMP                 3
#define ZONEID_ENTER                4
#define ZONEID_INIT                 5
#define ZONEID_POWER_UP             6
#define ZONEID_POWER_DOWN           7
#define ZONEID_EVENT_HOOK           8

#define PWR_ZONE_ERROR             DEBUGZONE(ZONEID_ERROR)
#define PWR_ZONE_WARNING           DEBUGZONE(ZONEID_WARNING)
#define PWR_ZONE_PERF              DEBUGZONE(ZONEID_PERF)
#define PWR_ZONE_TEMP              DEBUGZONE(ZONEID_TEMP)
#define PWR_ZONE_ENTER             DEBUGZONE(ZONEID_ENTER)
#define PWR_ZONE_INIT              DEBUGZONE(ZONEID_INIT)
#define PWR_ZONE_POWER_UP          DEBUGZONE(ZONEID_POWER_UP)
#define PWR_ZONE_POWER_DOWN        DEBUGZONE(ZONEID_POWER_DOWN)
#define PWR_ZONE_EVENT_HOOK        DEBUGZONE(ZONEID_EVENT_HOOK)

#define ZONEMASK_ERROR         (1 << ZONEID_ERROR)
#define ZONEMASK_WARNING       (1 << ZONEID_WARNING)
#define ZONEMASK_PERF          (1 << ZONEID_PERF)
#define ZONEMASK_TEMP          (1 << ZONEID_TEMP)
#define ZONEMASK_ENTER         (1 << ZONEID_ENTER)
#define ZONEMASK_INIT          (1 << ZONEID_INIT)
#define ZONEMASK_POWER_UP      (1 << ZONEID_POWER_UP)
#define ZONEMASK_POWER_DOWN    (1 << ZONEID_POWER_DOWN)
#define ZONEMASK_EVENT_HOOK    (1 << ZONEID_EVENT_HOOK)

#ifndef PWRBTN_DEBUGZONES
#define PWRBTN_DEBUGZONES          (ZONEMASK_ERROR | ZONEMASK_WARNING | ZONEMASK_POWER_UP | \
                                    ZONEMASK_POWER_DOWN | ZONEMASK_EVENT_HOOK)
#endif
#ifndef PWRBTN_RETAILZONES
#define PWRBTN_RETAILZONES          (ZONEMASK_ERROR | ZONEMASK_WARNING)
#endif
#ifdef  DEBUG
#define PWRBTN_ZONES PWRBTN_DEBUGZONES
#else
#define PWRBTN_ZONES PWRBTN_RETAILZONES
#endif


//------------------------------------------------------------------------------
// Local macros

#define CLEARBIT32(d,b)   ((d) &= ~(1<<b))
#define SETBIT32(d,b)     ((d) |= ((d) & ~(1<<b)) | (1<<b))
#define GETBIT32(d,b)     ((d) & (1<<b))

#endif // _PRECOMP_H_