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

   s3c6410_power_control.h

Abstract:

   Low Level HW Block Power control interface

Functions:

    

Notes:

--*/

#ifndef _S3C6410_POWER_CONTROL_H_
#define _S3C6410_POWER_CONTROL_H_


// Each value is same to Bit number of BLK_PWR_STAT, except for IROM
typedef enum
{
    BLKPWR_DOMAIN_TOP = 0,
    BLKPWR_DOMAIN_V = 1,            // MFC    
    BLKPWR_DOMAIN_I = 2,            // Camera interface, Jpeg    
    BLKPWR_DOMAIN_P = 3,            // FIMG-2D, TV Encoder, TV Scaler    
    BLKPWR_DOMAIN_F = 4,            // Display Controller, Post Processor, Rotator    
    BLKPWR_DOMAIN_S = 5,            // SDMA, Security System    
    BLKPWR_DOMAIN_ETM = 6,          // ETM    
    BLKPWR_DOMAIN_G = 7, 
    BLKPWR_DOMAIN_IROM = 30             // Internal ROM
} BLKPWR_DOMAIN;

typedef enum
{
    RST_HW_RESET = 0x1,        // External reset by XnRESET
    RST_WARM_RESET = 0x2,        // Warm reset by XnWRESET
    RST_WDT_RESET = 0x4,        // Watch dog timer reset
    RST_SLEEP_WAKEUP = 0x8,        // Reset by Wake up from Sleep
    RST_ESLEEP_WAKEUP = 0x10,    // Reset by Wake up from eSleep
    RST_SW_RESET = 0x20,            // Software reset
    RST_UNKNOWN = 0xFF           // Undefined status
} RESET_STATUS;

BOOL PwrCon_initialize_register_address(void *pSysConReg);
BOOL PwrCon_set_block_power_on(BLKPWR_DOMAIN eDomain);
BOOL PwrCon_set_block_power_off(BLKPWR_DOMAIN eDomain);
void PwrCon_set_USB_phy(BOOL bOn);
RESET_STATUS PwrCon_get_reset_status(void);

#endif    // _S3C6410_POWER_CONTROL_H_

