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
//------------------------------------------------------------------------------
//
//  File:  bsp_base_reg_cfg.h
//
//  This header file defines location for BSP on-board devices. It usually
//  should contain only physical addresses. Virtual addresses should be obtain
//  via OALPAtoVA function call. Base addresses for SoC are defined in similar
//  file s3c6410_base_reg_cfg.h.
//
#ifndef __BSP_BASE_REG_CFG_H
#define __BSP_BASE_REG_CFG_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//  NAMING CONVENTIONS
//
//  BSP_BASE_REG_ is the standard prefix for BSP device base registers.
//
//  Memory ranges are accessed using physical, uncached, or cached addresses,
//  depending on the system state. The following abbreviations are used for
//  each addressing type:
//
//      PA - physical address
//      CA - cached virtual address
//      UA - uncached virtual address
//
//  The naming convention for base registers is:
//
//      xxx_BASE_REG_<ADDRTYPE>_<SUBSYSTEM>
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Define:  BSP_BASE_REG_PA_CS8900A
//
//  Locates CS8900A Ethernet chip. On S3C6410 Board memory bank 5 is used
//  to connect this device. Note that there must exist memory mapping in
//  memory_cfg.h for this memory area.
//

#define BSP_BASE_REG_PA_CS8900A_IOBASE        (0x18800300)
#define BSP_BASE_REG_PA_CS8900A_MEMBASE        (0x18000000)
#define BSP_BASE_REG_PA_DM9000A_IOBASE        (0x18800300)
#define BSP_BASE_REG_PA_DM9000A_MEMBASE        (0x18000000)
//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
