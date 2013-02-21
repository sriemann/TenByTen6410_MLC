//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//------------------------------------------------------------------------------
//
//  File:  image_cfg.h
//
//  Defines configuration parameters used to create the NK and Bootloader
//  program images.
//
#ifndef __IMAGE_CFG_H
#define __IMAGE_CFG_H

#if __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//  RESTRICTION
//
//  This file is a configuration file. It should ONLY contain simple #define
//  directives defining constants. This file is included by other files that
//  only support simple substitutions.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  NAMING CONVENTION
//
//  The IMAGE_ naming convention ...
//
//  IMAGE_<NAME>_<SECTION>_<MEMORY_DEVICE>_[OFFSET|SIZE|START|END]
//
//      <NAME>          - WINCE, BOOT, SHARE
//      <SECTION>       - section name: user defined
//      <MEMORY_DEVICE> - the memory device the block resides on
//      OFFSET          - number of bytes from memory device start address
//      SIZE            - maximum size of the block
//      START           - start address of block    (device address + offset)
//      END             - end address of block      (start address  + size - 1)
//
//------------------------------------------------------------------------------

// DRAM Base Address
#define DRAM_BASE_PA_START            (0x50000000)
#define DRAM_BASE_CA_START            (0x80000000)
#define DRAM_BASE_UA_START            (0xA0000000)

#if (BSP_TYPE == BSP_DRAM256)
#define DRAM_SIZE                     (0x10000000)
#elif (BSP_TYPE == BSP_DRAM128)
#define DRAM_SIZE                     (0x08000000)
#endif

//------------------------------------------------------------------------------

// Steploader Area
#define IMAGE_STEPLOADER_PA_START       (0x00000000)
#define IMAGE_STEPLOADER_SIZE           (0x00002000)

//------------------------------------------------------------------------------

// Eboot Area
#define IMAGE_EBOOT_OFFSET              (0x00030000)
#define IMAGE_EBOOT_PA_START            (DRAM_BASE_PA_START+IMAGE_EBOOT_OFFSET)
#define IMAGE_EBOOT_CA_START            (DRAM_BASE_CA_START+IMAGE_EBOOT_OFFSET)
#define IMAGE_EBOOT_UA_START            (DRAM_BASE_UA_START+IMAGE_EBOOT_OFFSET)
#define IMAGE_EBOOT_SIZE                (0x00080000)

#define EBOOT_BINFS_BUFFER_OFFSET       (0x06C00000)
#define EBOOT_BINFS_BUFFER_PA_START     (DRAM_BASE_PA_START+EBOOT_BINFS_BUFFER_OFFSET)
#define EBOOT_BINFS_BUFFER_CA_START     (DRAM_BASE_CA_START+EBOOT_BINFS_BUFFER_OFFSET)
#define EBOOT_BINFS_BUFFER_UA_START     (DRAM_BASE_UA_START+EBOOT_BINFS_BUFFER_OFFSET)
#define EBOOT_BINFS_BUFFER_SIZE         (0x00480000)

#define EBOOT_USB_BUFFER_OFFSET         (0x03000000)
#define EBOOT_USB_BUFFER_PA_START       (DRAM_BASE_PA_START+EBOOT_USB_BUFFER_OFFSET)
#define EBOOT_USB_BUFFER_CA_START       (DRAM_BASE_CA_START+EBOOT_USB_BUFFER_OFFSET)
#define EBOOT_USB_BUFFER_UA_START       (DRAM_BASE_UA_START+EBOOT_USB_BUFFER_OFFSET)

// Eboot Display Frame Buffer
// 2MB
#define EBOOT_FRAMEBUFFER_OFFSET        (0x07E00000)
#define EBOOT_FRAMEBUFFER_PA_START      (DRAM_BASE_PA_START+EBOOT_FRAMEBUFFER_OFFSET)
#define EBOOT_FRAMEBUFFER_UA_START      (DRAM_BASE_UA_START+EBOOT_FRAMEBUFFER_OFFSET)
#define EBOOT_FRAMEBUFFER_SIZE          (0x00200000)

//------------------------------------------------------------------------------

// NK Area
#define IMAGE_NK_OFFSET                 (0x00100000)
#define IMAGE_NK_PA_START               (DRAM_BASE_PA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_CA_START               (DRAM_BASE_CA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_UA_START               (DRAM_BASE_UA_START+IMAGE_NK_OFFSET)
#define IMAGE_NK_SIZE                   (0x04F00000)  // Set Max Size, This will be tailored automatically

//------------------------------------------------------------------------------

// BSP ARGs Area
#define IMAGE_SHARE_ARGS_OFFSET         (0x00020800)
#define IMAGE_SHARE_ARGS_PA_START       (DRAM_BASE_PA_START+IMAGE_SHARE_ARGS_OFFSET)
#define IMAGE_SHARE_ARGS_CA_START       (DRAM_BASE_CA_START+IMAGE_SHARE_ARGS_OFFSET)
#define IMAGE_SHARE_ARGS_UA_START       (DRAM_BASE_UA_START+IMAGE_SHARE_ARGS_OFFSET)
#define IMAGE_SHARE_ARGS_SIZE           (0x00000800)

//------------------------------------------------------------------------------

// Sleep Data Area
#define IMAGE_SLEEP_DATA_OFFSET         (0x00028000)
#define IMAGE_SLEEP_DATA_PA_START       (DRAM_BASE_PA_START+IMAGE_SLEEP_DATA_OFFSET)
#define IMAGE_SLEEP_DATA_CA_START       (DRAM_BASE_CA_START+IMAGE_SLEEP_DATA_OFFSET)
#define IMAGE_SLEEP_DATA_UA_START       (DRAM_BASE_UA_START+IMAGE_SLEEP_DATA_OFFSET)
#define IMAGE_SLEEP_DATA_SIZE           (0x00002000)

//------------------------------------------------------------------------------

// Display Frame Buffer
// 12MB
#define IMAGE_FRAMEBUFFER_OFFSET        (0x06900000)
#define IMAGE_FRAMEBUFFER_PA_START      (DRAM_BASE_PA_START+IMAGE_FRAMEBUFFER_OFFSET)
#define IMAGE_FRAMEBUFFER_UA_START      (DRAM_BASE_UA_START+IMAGE_FRAMEBUFFER_OFFSET)
#define IMAGE_FRAMEBUFFER_SIZE          (0x00C00000)

//------------------------------------------------------------------------------

// MFC Video Process Buffer
// 12MB
#define IMAGE_MFC_BUFFER_OFFSET         (0x07500000)
#define IMAGE_MFC_BUFFER_PA_START       (DRAM_BASE_PA_START+IMAGE_MFC_BUFFER_OFFSET)
#define IMAGE_MFC_BUFFER_UA_START       (DRAM_BASE_UA_START+IMAGE_MFC_BUFFER_OFFSET)
#define IMAGE_MFC_BUFFER_SIZE           (0x00800000)

//------------------------------------------------------------------------------

// CMM memory
#define IMAGE_CMM_BUFFER_OFFSET         (0x07D00000)
#define IMAGE_CMM_BUFFER_PA_START       (DRAM_BASE_PA_START+IMAGE_CMM_BUFFER_OFFSET)
#define IMAGE_CMM_BUFFER_UA_START       (DRAM_BASE_UA_START+IMAGE_CMM_BUFFER_OFFSET)
#define IMAGE_CMM_BUFFER_SIZE           (0x00300000)

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
