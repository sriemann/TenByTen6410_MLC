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

    fmd.cpp

Abstract:

    This module implements main functions of FMD common PDD

Functions:

    FMD_Init,  

Notes:

--*/

#include <windows.h>
#include <dbgapi.h>

// This definition is come from CETK test code
#define ZONEID_ERROR            10
#define ZONEID_FUNCTION         11
#define ZONEID_STATUS           12
#define ZONEID_ERASE            13
#define ZONEID_WRITE            14
#define ZONEID_READ             15

#define FMD_ZONE_ERROR              DEBUGZONE(ZONEID_ERROR)
#define FMD_ZONE_FUNCTION           DEBUGZONE(ZONEID_FUNCTION)
#define FMD_ZONE_STATUS             DEBUGZONE(ZONEID_STATUS)
#define FMD_ZONE_ERASE              DEBUGZONE(ZONEID_ERASE)
#define FMD_ZONE_WRITE              DEBUGZONE(ZONEID_WRITE)
#define FMD_ZONE_READ               DEBUGZONE(ZONEID_READ)

#ifdef ZONEMASK_ERROR
#undef ZONEMASK_ERROR
#endif
#define ZONEMASK_ERROR          (1 << ZONEID_ERROR)
#ifdef ZONEMASK_FUNCTION
#undef ZONEMASK_FUNCTION
#endif
#define ZONEMASK_FUNCTION       (1 << ZONEID_FUNCTION)
#define ZONEMASK_STATUS         (1 << ZONEID_STATUS)
#define ZONEMASK_ERASE          (1 << ZONEID_ERASE)
#define ZONEMASK_WRITE          (1 << ZONEID_WRITE)
#define ZONEMASK_READ           (1 << ZONEID_READ)

// Debugzone 0~8 is already defined in FAL
#ifdef DEBUG
#define DBZID_ERROR                     0
#define DBZID_OPS                       1
#define DBZID_STATUS                    2
#define DBZID_PARAMS                    3
#define DBZID_DEVINFO                   4
#define DBZID_CALC                      5
#define DBZID_HAL                       6
#define DBZID_VERBOSE                   7
#define DBZID_INIT                      8       // defined by FAL
#define DBZID_MAX                       16

#define DBZMSK_ERROR                    (1<<DBZID_ERROR)
#define DBZMSK_OPS                      (1<<DBZID_OPS)
#define DBZMSK_STATUS                   (1<<DBZID_STATUS)
#define DBZMSK_PARAMS                   (1<<DBZID_PARAMS)
#define DBZMSK_DEVINFO                  (1<<DBZID_DEVINFO)
#define DBZMSK_CALC                     (1<<DBZID_CALC)
#define DBZMSK_HAL                      (1<<DBZID_HAL)
#define DBZMSK_VERBOSE                  (1<<DBZID_VERBOSE)
#define DBZMSK_INIT                     (1<<DBZID_INIT)

#define DBZ_DEFAULT                     DBZMSK_ERROR

#define ZONE_FASLFMD_ERROR              DEBUGZONE(DBZID_ERROR)
#define ZONE_FASLFMD_OPS                DEBUGZONE(DBZID_OPS)
#define ZONE_FASLFMD_STATUS             DEBUGZONE(DBZID_STATUS)
#define ZONE_FASLFMD_PARAMS             DEBUGZONE(DBZID_PARAMS)
#define ZONE_FASLFMD_DEVINFO            DEBUGZONE(DBZID_DEVINFO)
#define ZONE_FASLFMD_CALC               DEBUGZONE(DBZID_CALC)
#define ZONE_FASLFMD_HAL                DEBUGZONE(DBZID_HAL)
#define ZONE_FASLFMD_VERBOSE            DEBUGZONE(DBZID_VERBOSE)
#define ZONE_FASLFMD_INIT               DEBUGZONE(DBZID_INIT)

#define MAXSTRINGLEN                    32
#else

#define ZONE_FASLFMD_ERROR              0
#define ZONE_FASLFMD_OPS                0
#define ZONE_FASLFMD_STATUS             0
#define ZONE_FASLFMD_PARAMS             0
#define ZONE_FASLFMD_DEVINFO            0
#define ZONE_FASLFMD_CALC               0
#define ZONE_FASLFMD_HAL                0
#define ZONE_FASLFMD_VERBOSE            0
#define ZONE_FASLFMD_INIT               0
#endif

#ifndef DEBUG
// This is already defined in FAL
DBGPARAM dpCurSettings = { TEXT("FMDPDD"), {
    TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), 
    TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"),
    TEXT("Error"), TEXT("Function"), TEXT("Status"), 
    TEXT("Erase"), TEXT("Write"), TEXT("Read"),
    },
    ZONEMASK_ERROR //| ZONEMASK_FUNCTION | ZONEMASK_STATUS
};
#endif