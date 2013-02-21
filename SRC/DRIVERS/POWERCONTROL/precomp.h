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
#include "s3c6410_power_control.h"

#define QUEUE_ENTRIES   (16)
#define MAX_NAMELEN     (64)
#define QUEUE_SIZE      (QUEUE_ENTRIES*(sizeof(POWER_BROADCAST)+MAX_NAMELEN))

#define POWER_MONITOR_THREAD_PRIODITY    (98)

#define __MODULE__  "S3C6410 Power Control Driver"

#define ZONEID_ERROR                0
#define ZONEID_WARNING              1
#define ZONEID_PERF                 2
#define ZONEID_TEMP                 3
#define ZONEID_ENTER                4
#define ZONEID_INIT                 5
#define ZONEID_BLK_PWR_ON           6
#define ZONEID_BLK_PWR_OFF          7
#define ZONEID_DVS_PROFILE          8
#define ZONEID_DVS_CHANGE           9

#define PWC_ZONE_ERROR             DEBUGZONE(ZONEID_ERROR)
#define PWC_ZONE_WARNING           DEBUGZONE(ZONEID_WARNING)
#define PWC_ZONE_PERF              DEBUGZONE(ZONEID_PERF)
#define PWC_ZONE_TEMP              DEBUGZONE(ZONEID_TEMP)
#define PWC_ZONE_ENTER             DEBUGZONE(ZONEID_ENTER)
#define PWC_ZONE_INIT              DEBUGZONE(ZONEID_INIT)
#define PWC_ZONE_BLK_PWR_ON        DEBUGZONE(ZONEID_BLK_PWR_ON)
#define PWC_ZONE_BLK_PWR_OFF       DEBUGZONE(ZONEID_BLK_PWR_OFF)
#define PWC_ZONE_DVS_PROFILE       DEBUGZONE(ZONEID_DVS_PROFILE)
#define PWC_ZONE_DVS_CHANGE        DEBUGZONE(ZONEID_DVS_CHANGE)

#define ZONEMASK_ERROR         (1 << ZONEID_ERROR)
#define ZONEMASK_WARNING       (1 << ZONEID_WARNING)
#define ZONEMASK_PERF          (1 << ZONEID_PERF)
#define ZONEMASK_TEMP          (1 << ZONEID_TEMP)
#define ZONEMASK_ENTER         (1 << ZONEID_ENTER)
#define ZONEMASK_INIT          (1 << ZONEID_INIT)
#define ZONEMASK_BLK_PWR_ON    (1 << ZONEID_BLK_PWR_ON)
#define ZONEMASK_BLK_PWR_OFF   (1 << ZONEID_BLK_PWR_OFF)
#define ZONEMASK_DVS_PROFILE   (1 << ZONEID_DVS_PROFILE)
#define ZONEMASK_DVS_CHANGE    (1 << ZONEID_DVS_CHANGE)

#ifndef PWRCTL_DEBUGZONES
#define PWRCTL_DEBUGZONES          (ZONEMASK_ERROR | ZONEMASK_WARNING | ZONEMASK_BLK_PWR_ON | \
                                    ZONEMASK_BLK_PWR_OFF | ZONEMASK_DVS_CHANGE)
#endif
#ifndef PWRCTL_RETAILZONES
#define PWRCTL_RETAILZONES          (ZONEMASK_ERROR | ZONEMASK_WARNING | ZONEMASK_DVS_CHANGE)
#endif
#ifdef  DEBUG
#define PWRCTL_ZONES PWRCTL_DEBUGZONES
#else
#define PWRCTL_ZONES PWRCTL_RETAILZONES
#endif


//------------------------------------------------------------------------------
// Local macros

#define CLEARBIT32(d,b)   ((d) &= ~(1<<b))
#define SETBIT32(d,b)     ((d) |= ((d) & ~(1<<b)) | (1<<b))
#define GETBIT32(d,b)     ((d) & (1<<b))


#endif // _PRECOMP_H_