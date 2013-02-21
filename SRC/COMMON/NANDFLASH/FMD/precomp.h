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

    The precompiled header for FMD PDD library

Notes:

--*/

#include <bsp.h>
#include <fmd.h>
#include "cfnand.h" // predefined Configuration data and parameter for each nand

extern volatile S3C6410_NAND_REG *g_pNFConReg;
extern volatile S3C6410_SYSCON_REG *g_pSysConReg;

// Defined in nand.s
// This functions are implemented by assembly for performance
extern "C"
{
    void RdPage512(unsigned char *bufPt);
    void RdPage512Unalign(unsigned char *bufPt);
    void WrPage512(unsigned char *bufPt);
    void WrPage512Unalign(unsigned char *bufPt);
    void WrPageInfo(PBYTE pBuff);
    void RdPageInfo(PBYTE pBuff);
}

#ifdef _IROMBOOT_
typedef struct
{
    UINT32  n8MECC0;        // 8MECC0
    UINT32  n8MECC1;        // 8MECC1
    UINT32  n8MECC2;        // 8MECC2
    UINT8   n8MECC3;        // 8MECC3
} MECC8;

#endif  // !_IROMBOOT_

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

extern DBGPARAM dpCurSettings;

extern BOOL g_bNeedExtAddr;

#define NAND_READ_TIMEOUT       (10)    //< this is heuristic value from experience, there is tAR time to get ID, refer to NAND Datasheet

// Prototypes not defined in fmd.h in public interface
BOOL ECC_CorrectData(SECTOR_ADDR sectoraddr, LPBYTE pData, UINT32 nRetEcc, ECC_CORRECT_TYPE nType);
void Init_NandController();



//------------------------------------------------------------------------------
// Local macros

#define CLEARBIT32(d,b)   ((d) &= ~(1<<b))
#define SETBIT32(d,b)     ((d) |= ((d) & ~(1<<b)) | (1<<b))
#define GETBIT32(d,b)     ((d) & (1<<b))

