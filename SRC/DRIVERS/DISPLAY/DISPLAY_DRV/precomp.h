//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
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

    The precompiled header for Display Class

Notes:

--*/

#include <bsp.h>
#include <types.h>
#include <winddi.h>
#include <ddrawi.h>
#include <ddgpe.h>
#include <emul.h>
#include <ddhfuncs.h>
#include <CRegEdit.h>
#include <dvp.h>
#include <image_cfg.h>
#include <dispperf.h>

#include "mediatype.h"
#include <display_main.h>
#include "s3c6410_ldi.h"
#include "s3c6410_display_con.h"
#include "s3c6410_post_proc.h"
#include "s3c6410_tv_scaler.h"
#include "s3c6410_tv_encoder.h"
#include "SVE_API.h"    // Display Driver Do not include "SVEDriverAPI.h"
#include <fimgse2d.h>
#include "priv_context.h"       // For common data structure, This has dependency to S3C6410Surf

#define __MODULE__  "S3C6410 Display Driver"

extern DBGPARAM dpCurSettings;
#ifdef DEBUG

#define GPE_ZONEMASK_ERROR              (1<<0)
#define GPE_ZONEMASK_WARNING            (1<<1)
#define GPE_ZONEMASK_PERF               (1<<2)
#define GPE_ZONEMASK_TEMP               (1<<3)

#define GPE_ZONEMASK_ENTER              (1<<4)
#define GPE_ZONEMASK_INIT               (1<<5)
#define GPE_ZONEMASK_BLT_HI             (1<<6)
#define GPE_ZONEMASK_BLT_LO             (1<<7)

#define GPE_ZONEMASK_CREATE             (1<<8)
#define GPE_ZONEMASK_FLIP               (1<<9)
#define GPE_ZONEMASK_LINE               (1<<10)
#define GPE_ZONEMASK_HW                 (1<<11)

#define GPE_ZONEMASK_POLY               (1<<12)
#define GPE_ZONEMASK_CURSOR             (1<<13)
// #define GPE_ZONE_               DEBUGZONE(14)
// #define GPE_ZONE_               DEBUGZONE(15)

#ifndef DISPDRV_DEBUGZONES
#define DISPDRV_DEBUGZONES         (GPE_ZONEMASK_ERROR | GPE_ZONEMASK_WARNING) // | GPE_ZONEMASK_TEMP)
#endif
#endif  //DEBUG

// Even on Release Mode, We don't care about GPE message
#define ZONEID_ERROR                0
#define ZONEID_WARNING              1
#define ZONEID_PERF                 2
#define ZONEID_TEMP                 3
#define ZONEID_ENTER                4
#define ZONEID_INIT                 5
#define ZONEID_BLT_HI               6
#define ZONEID_BLT_LO               7
#define ZONEID_CREATE               8
#define ZONEID_FLIP                 9
#define ZONEID_LINE                 10
#define ZONEID_POST                 11
#define ZONEID_ROTATOR              12
#define ZONEID_TVSC                 13
#define ZONEID_TVENC                14
#define ZONEID_2D                   15

#define DISP_ZONE_ERROR             DEBUGZONE(ZONEID_ERROR)
#define DISP_ZONE_WARNING           DEBUGZONE(ZONEID_WARNING)
#define DISP_ZONE_PERF              DEBUGZONE(ZONEID_PERF)
#define DISP_ZONE_TEMP              DEBUGZONE(ZONEID_TEMP)
#define DISP_ZONE_ENTER             DEBUGZONE(ZONEID_ENTER)
#define DISP_ZONE_INIT              DEBUGZONE(ZONEID_INIT)
#define DISP_ZONE_BLT_HI            DEBUGZONE(ZONEID_BLT_HI)
#define DISP_ZONE_BLT_LO            DEBUGZONE(ZONEID_BLT_LO)
#define DISP_ZONE_CREATE            DEBUGZONE(ZONEID_CREATE)
#define DISP_ZONE_FLIP              DEBUGZONE(ZONEID_FLIP)
#define DISP_ZONE_LINE              DEBUGZONE(ZONEID_LINE)
#define DISP_ZONE_POST              DEBUGZONE(ZONEID_POST)
#define DISP_ZONE_ROTATOR           DEBUGZONE(ZONEID_ROTATOR)
#define DISP_ZONE_TVSC              DEBUGZONE(ZONEID_TVSC)
#define DISP_ZONE_TVENC             DEBUGZONE(ZONEID_TVENC)
#define DISP_ZONE_2D                DEBUGZONE(ZONEID_2D)

#define ZONEMASK_ERROR         (1 << ZONEID_ERROR)
#define ZONEMASK_WARNING       (1 << ZONEID_WARNING)
#define ZONEMASK_PERF          (1 << ZONEID_PERF)
#define ZONEMASK_TEMP          (1 << ZONEID_TEMP)
#define ZONEMASK_ENTER         (1 << ZONEID_ENTER)
#define ZONEMASK_INIT          (1 << ZONEID_INIT)
#define ZONEMASK_BLT_HI        (1 << ZONEID_BLT_HI)
#define ZONEMASK_BLT_LO        (1 << ZONEID_BLT_LO)
#define ZONEMASK_CREATE        (1 << ZONEID_CREATE)
#define ZONEMASK_FLIP          (1 << ZONEID_FLIP)
#define ZONEMASK_LINE          (1 << ZONEID_LINE)
#define ZONEMASK_POST          (1 << ZONEID_POST)
#define ZONEMASK_ROTATOR       (1 << ZONEID_ROTATOR)
#define ZONEMASK_TVSC          (1 << ZONEID_TVSC)
#define ZONEMASK_TVENC         (1 << ZONEID_TVENC)
#define ZONEMASK_2D            (1 << ZONEID_2D)


#ifndef DISPDRV_RETAILZONES
#define DISPDRV_RETAILZONES         (ZONEMASK_ERROR)// | ZONEMASK_TEMP | ZONEMASK_BLT_LO| ZONEMASK_2D) 
#endif

// This Definition is used to initialize the main class of display driver
#define ESC_SUCCESS             (0x00000001)
#define ESC_FAILED              (0xFFFFFFFF)
#define ESC_NOT_SUPPORTED       (0x00000000)

#define TV_DST_WIDTH            (640)
#define TV_DST_HEIGHT           (448)

#define MAX_SUPPORT_MODE        (1)

#define PC_REG_DISPLAY_CONFIG   _T("\\Drivers\\Display\\S3C6410\\Config\\")
#define PC_REG_PRIMARY_WINDOW   _T("PrimaryWin")
#define PC_REG_DDOVERLAY_WINDOW _T("DDOverlayWin")
// This is Default Setting
#define PRIMARY_WINDOW          (DISP_WIN1)
#define PRIMARY_WINDOW_MODE     (DISP_WIN1_DMA)
#define OVERLAY_WINDOW          (DISP_WIN0)
#define OVERLAY_WINDOW_DMA      (DISP_WIN0_DMA)
#define OVERLAY_WINDOW_FIFO     (DISP_WIN0_POST_RGB)

// This calculation is different from CE 5.0, In CE5.0 Surface class can not support BytesPerPixel()
//#define BYTES_PER_PIXEL(_surf)      (EGPEFormatToBpp[(_surf)->Format()])>>3)     //< This is for WCE5.0
#define BYTES_PER_PIXEL(_surf)      ((_surf)->BytesPerPixel())
#define SURFACE_WIDTH(_surf)     ((_surf)->Stride()/BYTES_PER_PIXEL(_surf))
#define RECT_WIDTH(_rect)        ((_rect)->right - (_rect)->left)
#define RECT_HEIGHT(_rect)      ((_rect)->bottom - (_rect)->top)
#define MIN(a, b)                  ((a) < (b)? a : b)

/// Graphic Optimizaiton Options
//
#define G2D_ACCELERATE    (TRUE)            //< If you want to use 2D HW for GDI, set this to "TRUE", if not, set to "FALSE"
/// if USE_G2D_ACCELERATE is TRUE then these condition will work
/// Try to bitblt from cached source surface to non cached destinatino surface, this do cache flush
#define G2D_TRY_CBLT    (TRUE)        
/// if use 2DHW CETK GDI Test case 218(StretchBlt),219(TransparentBlt) will fail.
/// This is why 2DHW's stretching algorithm differ from SW stretching algorithm.
#define G2D_BYPASS_HW_STRETCHBLT    (FALSE)
/// 2D HW cannot process properly Non-Alphablend Blt with ARGB color format
/// If make this FALSE, CETK GDI test case 200,218,219, and DDraw test case 102 can fail.
/// MS code assumes this case as just forwarding pixel to destination except for TranslateBlt.
#define G2D_BYPASS_ALPHADIBTEST     (FALSE)
/// If use 2DHW Alphablended Bitblt, CETK GDI test case 200, 218, 219, 231 and DDraw test case 102 can fail.
/// Due to SW conformance issue
#define G2D_BYPASS_HW_ALPHABLEND    (FALSE)
    /// Below is suboption
    /// In our HW PPA feature has different bleding equation to SW.
    #define G2D_BYPASS_PERPIXEL_ALPHABLEND          (FALSE)
    /// In our HW SCA feature does not change alphavalue, so this can lead to test fail when repetive alphablending.
    #define G2D_BYPASS_SOURCECONSTANT_ALPHABLEND    (FALSE)
    /// This will run HW Bitblt twice, one for SCA and other for PPA, The result has incorrect alphabit
    #define G2D_BYPASS_2STEP_PROCESS_PPA_AFTER_SCA  (TRUE)
/// whether use HW fill or not
#define G2D_BYPASS_HW_FILLRECT      (FALSE)
/// whether override SW Emulattion
/// SW Emulation Code Selection is prior to HW Accelration Code Selection
#define G2D_OVERRIDE_EMULSEL        (FALSE)

/**
*    Define G2D StretchBlt SW Workaround
*    2D HW's Stretch Algorithm is Nearest Stretch and it calculate reference source pixel with round off(+0.5)
**/
#define G2D_ROUNDOFF_REFERENCE      (1)     //< HW Stretchblt algorithm differs from MS'SW Stretching BLT algorithms,  So, CETK 218, 219 can fails.
#define G2D_QUARTERED_ADJUSTING     (2)     //< This will decrease fail case. but still fail is occured
                                            //< This is just experimental adjusting and slower than the deafult G2D_ROUNDOFF_REFERENCE
#define G2D_STRETCH_ALGORITHM       (G2D_ROUNDOFF_REFERENCE)

/// For using Physically Linear Surface on System Memory to wide 2D HW usage.
/// 2D HW need physically contiguous memory, and its address. 
/// This will consume System Memory and allocate Physically and Virtually contiguous memory.
/// So if system has small memory, allocation may fail.
/// Then 2D HW will not work for that memory.
#define USE_PACSURF        (TRUE)        
///  if USE_PACSURF is TRUE then these condition will work
#define G2D_BLT_OPTIMIZE    (TRUE)                //< This option will enable above two optimization method. This can increase 2D processing overhead.
#define PAC_ALLOCATION_BOUNDARY    (160*120*2)    //(320*240*2)        //<  PACSurf creation request is processed only for the surface has over QVGA 16bpp size        
#define G2D_COMPROMISE_LIMIT    (28800)        //< Transferring below this size(byte) using HW will be poor than using SW. so we will use software 2D flow under this size transfer request.

#define USE_SECEMUL_LIBRARY    (TRUE)

#define HWBLIT_CLIPPING_ISSUE_WORKAROUND    (TRUE)  // Workaround for the clipping region calculation issue when creating scratch surfaces

