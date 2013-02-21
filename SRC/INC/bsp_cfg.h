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
//  File:  bsp_cfg.h
//
//  This file contains system constant specific for SMDK6410 board.
//
#ifndef __BSP_CFG_H
#define __BSP_CFG_H

#include <soc_cfg.h>

//------------------------------------------------------------------------------
//
//  Define:  BSP_DEVICE_PREFIX
//
//  Prefix used to generate device name for bootload/KITL
//
#define BSP_DEVICE_PREFIX       "SMDK6410"        // Device name prefix

#define PWRCTL_HITEG_FACTORY_DEFAULTS ((1<<0) | (1<<1) | (1<<4))

//------------------------------------------------------------------------------
// SMDK6410 Display Dimension, is going to be replaced by extended LDI module
// We currently use these values int bsp_args.(c|h) for init values...
//------------------------------------------------------------------------------
#define MEGADISPLAY3		(0)   	//CHIMEI lq035nc111_v05
#define MEGADISPLAY4		(1)   	//INNOLUX AT043
#define MEGADISPLAY7        	(2)   	//INNOLUX AT070 	
#define TV_OUT			(3)	// TV_OUT only
#define MEGADISPLAY3A		(4)   	//CHIMEI lq035nc111_v05 with ATMEL 48pa
#define MEGADISPLAY4A		(5)   	//INNOLUX AT043 with ATMEL 48pa
#define MEGADISPLAY7A       	(6)   	//INNOLUX AT070 with ATMEL 48pa	
#define NO_DISPLAY	      (123) 	// No Display or unknown

#define SMDK6410_LCD_MODULE       	(NO_DISPLAY)
#define DEFAULT_FRAMEBUFFER_COLOR	(0x00101010)
#define DEFAULT_LOGO_POSITION		(~0)

#if (SMDK6410_LCD_MODULE == MEGADISPLAY3 || SMDK6410_LCD_MODULE == MEGADISPLAY3A )
#define LCD_WIDTH            320
#define LCD_HEIGHT           240
#define LCD_BPP              24
#elif (SMDK6410_LCD_MODULE == MEGADISPLAY4 || SMDK6410_LCD_MODULE == MEGADISPLAY4A)
#define LCD_WIDTH		     480
#define LCD_HEIGHT		     272
#define LCD_BPP			      24
#elif (SMDK6410_LCD_MODULE == MEGADISPLAY7 || SMDK6410_LCD_MODULE == MEGADISPLAY7A)
#define LCD_WIDTH            800
#define LCD_HEIGHT           480
#define LCD_BPP              16		// fluid
#elif (SMDK6410_LCD_MODULE == TV_OUT)
#define LCD_WIDTH            640
#define LCD_HEIGHT           480
#define LCD_BPP               16	// conservative
#elif (SMDK6410_LCD_MODULE == NO_DISPLAY)
#define LCD_WIDTH            0
#define LCD_HEIGHT           0
#define LCD_BPP              16
#else
#error LCD_MODULE_UNDEFINED_ERROR
#endif

// We define the maximum values for the S3C6410 as reported by SAMSUNG
// However we suggest using in the max resolution 8Bit depth for the framebuffer to get 
// fluid visuals....no OpenGL on that one though....but check out the TFT controller and 2D Accel. !!!!

#define LCD_MAX_WIDTH	1024
#define LCD_MAX_hEIGHT	 768
#define LCD_MAX_BPP	  32
 

//------------------------------------------------------------------------------
// SMDK6410 Audio Sampling Rate
//------------------------------------------------------------------------------
#define AUDIO_44_1KHz        (44100)
#define AUDIO_48KHz            (48000)
#define AUDIO_SAMPLE_RATE    (AUDIO_44_1KHz)        // Keep sync with EPLL Fout

//------------------------------------------------------------------------------
// SMDK6410 UART Debug Port Baudrate
//------------------------------------------------------------------------------
#define DEBUG_UART0         (0)
#define DEBUG_UART1         (1)
#define DEBUG_UART2         (2)
#define DEBUG_UART3         (3)
#define DEBUG_BAUDRATE      (115200)

//------------------------------------------------------------------------------
// SMDK6410 NAND Flash Timing Parameter
// Need to optimize for each HClock value.
// Default vaule is 7.7.7. this value has maximum margin. 
// so stable but performance cannot be max.
//------------------------------------------------------------------------------
#define NAND_TACLS         (7)
#define NAND_TWRPH0        (7)
#define NAND_TWRPH1        (7)

//------------------------------------------------------------------------------
// SMDK6410 CF Interface Mode
//------------------------------------------------------------------------------
#define CF_INDIRECT_MODE    (0)        // MemPort0 Shared by EBI
#define CF_DIRECT_MODE      (1)        // Independent CF Interface
#define CF_INTERFACE_MODE   (CF_DIRECT_MODE)

// Added for DM9000
#ifndef SYSINTR_DM9000A1
#define SYSINTR_DM9000A1     (SYSINTR_FIRMWARE+2)	// for DM9000 Adp1, nCS4, 18
#endif
//------------------------------------------------------------------------------
// SMDK6410 Keypad Layout
//------------------------------------------------------------------------------
#define LAYOUT0                (0)        // 8*8 Keypad board
#define LAYOUT1                (1)        // On-Board Key
#define LAYOUT2                (2)        // Qwerty Key board
#define MATRIX_LAYOUT        (LAYOUT1)

#endif // __BSP_CFG_H

