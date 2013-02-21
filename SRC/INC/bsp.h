//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//  Use of this source code is subject to the terms of the Microsoft end-user
//  license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
//  If you did not accept the terms of the EULA, you are not authorized to use
//  this source code. For a copy of the EULA, please see the LICENSE.RTF on your
//  install media.
//
//------------------------------------------------------------------------------
//
//  File: bsp.h
//
//  This header file is comprised of component header files that defines
//  the standard include hierarchy for the board support packege.
//
#ifndef __BSP_H
#define __BSP_H

//------------------------------------------------------------------------------

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <oal.h>

// BSP Configuration Files
#include "bsp_cfg.h"
#include "bsp_base_reg_cfg.h"
#include "ioctl_cfg.h"
#include "image_cfg.h"
#include "bsp_args.h"

// Processor Specific Definitions
#include "s3c6410.h"



//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
