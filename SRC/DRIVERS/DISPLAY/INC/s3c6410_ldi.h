//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) Hiteg Ltd.	All rights reserved.
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s3c6410_ldi.h

Abstract:       Function prototypes and enumrate value of LDI control library

Functions:


Notes:


--*/

#ifndef __S3C6410_LDI_H__
#define __S3C6410_LDI_H__

#include <bsp.h>
#include "s3c6410_display_con.h"

#if __cplusplus
extern "C"
{
#endif

typedef enum backlight_type_t
{
	FIXED_BKLGHT=0,
	PWM_BKLGHT=1,
	ATMEL_BKLGHT=2,
	PCA9530=3,
} BKLTYPE;

typedef struct backlight_boost_t
{	char *name;
	unsigned min_duty;			// on reach switch backlight off with zero PWM
	unsigned max_duty;			// on reach switch backlight on with full PWM
	unsigned polarity;			// sets the polarity of the PWM signal, negativ = 0 or positive = 1
} BACKLIGHT_BOOST;

typedef struct hiteg_display
{
    char *name;
	BKLTYPE backlight;
	DISP_VIDOUT_MODE mode;
    DISP_RGBIFOUT_MODE intrfc;
    unsigned int width;
    unsigned int height;
    unsigned int VBPD;
    unsigned int VFPD;
    unsigned int VSPW;
    unsigned int HBPD;
    unsigned int HFPD;
    unsigned int HSPW;
    unsigned int VCLK_POL;
    unsigned int HSYNC_POL;
    unsigned int VSYNC_POL;
    unsigned int VDEN_POL;
    unsigned int PNR_MODE;
    unsigned int VCLK_SRC;
    unsigned int VCLK_DIR;
    unsigned int FRAME_RATE;
	BACKLIGHT_BOOST *boost;

} HITEG_DISPLAY;


typedef enum hiteg_display_type
{
    HITEG_MEGADISPLAY3  = 0,
    HITEG_MEGADISPLAY4,
    HITEG_MEGADISPLAY7,
    HITEG_MEGADISPLAY5SD,
    HITEG_MEGADISPLAY5HD,
    HITEG_VGA640,
    HITEG_VGA800,
    HITEG_VGA1024,
    HITEG_TV,
    HITEG_MEGADISPLAY3A,		
    HITEG_MEGADISPLAY4A,		
    HITEG_MEGADISPLAY7A,		
    MAX_DISPLAYS
} HITEG_DISPLAY_TYPE ;


typedef enum
{
    LDI_SUCCESS,
    LDI_ERROR_NULL_PARAMETER,
    LDI_ERROR_ILLEGAL_PARAMETER,
    LDI_ERROR_NOT_INITIALIZED,
    LDI_ERROR_NOT_IMPLEMENTED,
    LDI_ERROR_XXX
} LDI_ERROR;



LDI_ERROR LDI_initialize_register_address(volatile void *pSPIReg,volatile void *pDispConReg,volatile void *pGPIOReg);
LDI_ERROR LDI_set_LCD_module_type(HITEG_DISPLAY_TYPE ModuleType);
LDI_ERROR LDI_initialize_LCD_module(void);
LDI_ERROR LDI_deinitialize_LCD_module(void);
LDI_ERROR LDI_fill_output_device_information(void *pDevInfo);

extern HITEG_DISPLAY_TYPE display;
unsigned int getDisplayWidth(HITEG_DISPLAY_TYPE type);
unsigned int getDisplayHeight(HITEG_DISPLAY_TYPE type);
char *getAttachedDisplayName(HITEG_DISPLAY_TYPE type);

void LDI_setClock(int TV);
void DelayLoop_1ms(int msec);
void DelayLoop(int delay);
BOOL LDI_initDisplay(HITEG_DISPLAY_TYPE type, volatile void*syscon, volatile void *dispcon, volatile void *GPIO);
unsigned int LDI_GetDisplayWidth(HITEG_DISPLAY_TYPE type);
unsigned int LDI_GetDisplayHeight(HITEG_DISPLAY_TYPE type);
char *LDI_getDisplayName(HITEG_DISPLAY_TYPE type);
HITEG_DISPLAY_TYPE LDI_getDisplayType();
void LDI_setBPP(DWORD bpp);
DWORD LDI_getBPP();
void LDI_clearScreen(int framebuffer, DWORD color);
void LDI_setBacklight(unsigned char rate);
DISP_VIDOUT_MODE LDI_getDisplayMode();
void LDI_copyDisplay(HITEG_DISPLAY *tp,HITEG_DISPLAY_TYPE type );
BACKLIGHT_BOOST *LDI_getBoost(HITEG_DISPLAY_TYPE type);


// Macros to handle GPIOs

#define TFT_LCD_nRESET_BIT  (5)

#if __cplusplus
}
#endif

#endif    // __S3C6410_LDI_H__
