//
// Copyright (c) Hiteg Ltd.  All rights reserved.
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
// Copyright (c) Hiteg LTD.  All rights reserved.
// based on work from
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s3c6410_ldi.c

Abstract:       Libray to control MegaDisplay and TV

Functions:
				Heavily improved version of SAMSUNG's original code
				removed support for SAMSUNGS LDI based displays
				added support for RGB_P interface and TV
				Upcoming version will add support for I80 IF Displays
				MegaDisplay<x>C series

Notes:


--*/

#include <bsp.h>
#include "s3c6410_ldi.h"
#include "s3c6410_display_con.h"
#include "s3c6410_display_con_macro.h"

#ifdef DEBUG
#define LDI_MSG(x)   RETAILMSG(FALSE, x)
#define LDI_INF(x)    RETAILMSG(FALSE, x)
#define LDI_ERR(x)   RETAILMSG(TRUE, x)
#else
#define LDI_MSG(x)   
#define LDI_INF(x)    
#define LDI_ERR(x)   RETAILMSG(FALSE, x)
#endif

#define LCD_DELAY_1MS    30000          // Sufficient under 1Ghz

DWORD Bpp;
DWORD BGcolor;
DWORD LogoHW;


static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static volatile S3C6410_DISPLAY_REG *g_pDispConReg = NULL;
static volatile S3C6410_SYSCON_REG *pSysConReg;

static BACKLIGHT_BOOST boost[]=
{
	{"SM8121AH",10,90,1},
	{"direct", 0, 100,1},
	{"LP3303", 10, 90,1},
	{"NONE",0,0,0},
};
static HITEG_DISPLAY_TYPE g_Type;

static HITEG_DISPLAY_TYPE display;

static HITEG_DISPLAY HitegDisplays[(int) MAX_DISPLAYS]=
{
{"MegaDisplay 3",PWM_BKLGHT,DISP_VIDOUT_RGBIF, DISP_24BIT_RGB888_P, 320,240,15,4,3,38,20,30,IVCLK_RISE_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_LCDCLK, CLKDIR_DIVIDED, 70, &boost[0]},

{"MegaDisplay 4", PWM_BKLGHT,DISP_VIDOUT_RGBIF, DISP_24BIT_RGB888_P, 480, 272, 2, 2, 10, 2, 2, 41, IVCLK_FALL_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_LCDCLK, CLKDIR_DIVIDED, 60, &boost[0]},

{"MegaDisplay 7",PCA9530,DISP_VIDOUT_RGBIF, DISP_16BIT_RGB565_P, 800, 480, 2,2,4,2,2,29, IVCLK_FALL_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_HCLK, CLKDIR_DIVIDED, 60, &boost[1]},

{"MegaDisplay 5SD", PWM_BKLGHT,DISP_VIDOUT_RGBIF, DISP_24BIT_RGB888_P, 480, 272, 2, 2, 10, 2, 2, 41, IVCLK_FALL_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_HCLK, CLKDIR_DIVIDED, 60, &boost[2]},

{"MegaDisplay 5HD", PWM_BKLGHT,DISP_VIDOUT_RGBIF, DISP_16BIT_RGB565_P, 800, 480, 29,13,3,40,40,48, IVCLK_FALL_EDGE, IHSYNC_HIGH_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_BGR_P, CLKSEL_F_HCLK, CLKDIR_DIVIDED, 60, &boost[2]},

{"VGA_640x480" ,PWM_BKLGHT,DISP_VIDOUT_RGBIF, DISP_18BIT_RGB666_P, 640, 480, 33,6,3,37,19,29, IVCLK_FALL_EDGE, IHSYNC_HIGH_ACTIVE, IVSYNC_HIGH_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_HCLK, CLKDIR_DIVIDED, 60, &boost[3]},

{"VGA_800x600" ,PWM_BKLGHT,DISP_VIDOUT_RGBIF, DISP_18BIT_RGB666_P, 800, 600, 17,8,6,17,24,29, IVCLK_FALL_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_LCDCLK, CLKDIR_DIVIDED, 70, &boost[1]},

{"VGA_1024x768" ,PWM_BKLGHT,DISP_VIDOUT_RGBIF, DISP_18BIT_RGB666_P, 1024, 768, 33,6,3,37,19,29, IVCLK_FALL_EDGE, IHSYNC_HIGH_ACTIVE, IVSYNC_HIGH_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_HCLK, CLKDIR_DIVIDED, 60, &boost[3]},

{"TV", FIXED_BKLGHT,DISP_VIDOUT_TVENCODER, DISP_18BIT_RGB666_S, 640, 480, 3,5,3,13,8,3, IVCLK_FALL_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_EXT27M, CLKDIR_DIRECT, 60, &boost[3]},

{"MegaDisplay 3A",ATMEL_BKLGHT,DISP_VIDOUT_RGBIF, DISP_24BIT_RGB888_P, 320,240,15,4,3,38,20,30,IVCLK_RISE_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_LCDCLK, CLKDIR_DIVIDED, 70, &boost[0]},

{"MegaDisplay 4A",ATMEL_BKLGHT,DISP_VIDOUT_RGBIF, DISP_24BIT_RGB888_P, 480, 272, 2, 2, 10, 2, 2, 41, IVCLK_FALL_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_LCDCLK, CLKDIR_DIVIDED, 60, &boost[0]},

{"MegaDisplay 7A",ATMEL_BKLGHT,DISP_VIDOUT_RGBIF, DISP_18BIT_RGB666_P, 800, 480, 2,2,4,2,2,29, IVCLK_FALL_EDGE, IHSYNC_LOW_ACTIVE, IVSYNC_LOW_ACTIVE, IVDEN_HIGH_ACTIVE, PNRMODE_RGB_P, CLKSEL_F_LCDCLK, CLKDIR_DIVIDED, 50, &boost[1]},

};
void LDI_copyDisplay(HITEG_DISPLAY *tp,HITEG_DISPLAY_TYPE type )
{
	if(tp==NULL) return;
	memcpy(tp, &HitegDisplays[type], sizeof(HITEG_DISPLAY));
}
unsigned int LDI_GetDisplayWidth(HITEG_DISPLAY_TYPE type)
{
    if (type>=MAX_DISPLAYS) return( (unsigned int )-1);
    return HitegDisplays[type].width;
}
unsigned int LDI_GetDisplayHeight(HITEG_DISPLAY_TYPE type)
{
    if (type>=MAX_DISPLAYS) return( (unsigned int )-1);
    return HitegDisplays[type].height;
}
char *LDI_getDisplayName(HITEG_DISPLAY_TYPE type) // strange format in NK.bin, can't have both?
{
    if(type>=MAX_DISPLAYS) return "NONE";
    
    return (HitegDisplays[type].name);
}
//Set PWM GPIO to control Back-light  Regulator  Shotdown Pin (GPF[14])
#define BACKLIGHT_ON(gpioreg)  {  (gpioreg)##->GPFDAT |= (1<<14); \
                                    (gpioreg)##->GPFCON = ((gpioreg)##->GPFCON & ~(0x3<<28)) | (1<<28); }        // set GPF[14] as Output

#define BACKLIGHT_OFF(gpioreg)  {  (gpioreg)##->GPFDAT &= ~(1<<14);  \
                                    (gpioreg)##->GPFCON = ((gpioreg)##->GPFCON & ~(0x3<<28)) | (1<<28); }
void LDI_setBacklight(unsigned char rate)
{
	//if(HitegDisplays[g_Type].backlight==FIXED_BKLGHT) return;
	if(rate==0) {
	  g_pGPIOReg->GPFCON &= ~(0x3<<28); 
	  g_pGPIOReg->GPFCON |= (0x2<<28);
      g_pGPIOReg->GPFPUD &= ~(0x3<<28);
      g_pGPIOReg->GPFDAT &= ~(0x1<<14);  // GPF14=OUT low
		return;
	}
	  g_pGPIOReg->GPFCON &= ~(0x3<<28); 
	  g_pGPIOReg->GPFCON |= (0x2<<28);
      g_pGPIOReg->GPFPUD &= ~(0x3<<28);
      g_pGPIOReg->GPFDAT |= (0x1<<14);  // GPF14=OUT high

	  g_pGPIOReg->SPCON &=~(3<<24); // better pictures on displays connected with LONG cables....
}

BACKLIGHT_BOOST *LDI_getBoost(HITEG_DISPLAY_TYPE type)
{
	return HitegDisplays[type].boost;
}
void LDI_setClock(int TV)
{
		pSysConReg->CLK_SRC = (pSysConReg->CLK_SRC & ~(0xFFFFFFF0))
			|(0<<31)				// TV27_SEL    -> 27MHz
            |(0<<30)				// DAC27        -> 27MHz
            |(0<<28)				// SCALER_SEL    -> MOUT_EPLL
			|(1<<26)				// LCD_SEL    -> Mout_MPLL
            |(0<<24)				// IRDA_SEL    -> MOUT_EPLL
            |(0<<22)				// MMC2_SEL    -> MOUT_EPLL
            |(0<<20)				// MMC1_SEL    -> MOUT_EPLL
            |(0<<18)				// MMC0_SEL    -> MOUT_EPLL
            |(0<<16)				// SPI1_SEL    -> MOUT_EPLL
            |(0<<14)				// SPI0_SEL    -> MOUT_EPLL
            |(0<<13)				// UART_SEL    -> MOUT_EPLL
            |(0<<10)				// AUDIO1_SEL    -> MOUT_EPLL
            |(0<<7)					// AUDIO0_SEL    -> MOUT_EPLL
            |(0<<5)					// UHOST_SEL    -> 48MHz
            |(0<<4);				// MFCCLK_SEL    -> HCLKx2 (0:HCLKx2, 1:MoutEPLL)
}


void LDI_setBPP(DWORD bpp)
{
	Bpp=16;
	if(bpp==16 || bpp==24 || bpp==32)
		Bpp=bpp;
}
DWORD LDI_getBPP()
{
	return Bpp;
}
void LDI_clearScreen(int framebuffer, DWORD color) // we use ARGB layout, thus A will not be used....
{
	unsigned short rgb16;
	unsigned char r,g,b;
	unsigned long pixels;
	r=(unsigned char)((color & 0xFF0000)>>16);
	g=(unsigned char)((color & 0x00FF00)>>8);
	b=(unsigned char)((color & 0x0000FF));
	rgb16=(unsigned short)(((r>>3)<<11) | ((g>>2)<<5) | (b>>3));
	LDI_INF((_T("[LDI:INF] RGB16: 0x%X\n\r"),rgb16 ));
	pixels=LDI_GetDisplayWidth(g_Type) * LDI_GetDisplayHeight(g_Type);

	LDI_INF((_T("[LDI:INF] framebuffer at: 0x%X\n\r"),framebuffer ));
	if(Bpp==16)
	{	
		unsigned short *fb;
		fb=(unsigned short *)framebuffer;
		for( ;pixels>0 ; pixels--)
			*fb++=rgb16;
	}
	else
	{
		DWORD *fb;
		fb=( DWORD *)framebuffer;
		for( ;pixels>0 ; pixels--)
			*fb++=color;	
	}
}
BOOL LDI_initDisplay(HITEG_DISPLAY_TYPE type, volatile void*syscon, volatile void *dispcon, volatile void *GPIO)
{

	if(type>=MAX_DISPLAYS)
	{
		LDI_MSG((_T("[LDI]++LDI_initDisplay error type(%d)\n\r"),type));
		return 0;
	}
	LDI_MSG((_T("[LDI]++LDI_initDisplay type(%d)\n\r"),type));
	LDI_set_LCD_module_type(type);
	pSysConReg = (S3C6410_SYSCON_REG *)syscon;

	// Initialize Display Power Gating
    if(!(pSysConReg->BLK_PWR_STAT & (1<<4))) {
        pSysConReg->NORMAL_CFG |= (1<<14);
        while(!(pSysConReg->BLK_PWR_STAT & (1<<4)));
        }
	
	LDI_initialize_register_address(NULL, dispcon, GPIO);


	LDI_setClock(0);
	

	LDI_MSG((_T("[LDI]--LDI_fill_output_device_information()\n\r")));
	return 1;
}


LDI_ERROR LDI_initialize_register_address(void *pSPIReg, void *pDispConReg, void *pGPIOReg)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_initialize_register_address(0x%08x, 0x%08x, 0x%08x)\n\r"), pSPIReg, pDispConReg, pGPIOReg));

    if (pDispConReg == NULL || pGPIOReg == NULL)
    {
        LDI_ERR((_T("[LDI:ERR] LDI_initialize_register_address() : NULL pointer parameter\n\r")));
        error = LDI_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pDispConReg = (S3C6410_DISPLAY_REG *)pDispConReg;
        g_pGPIOReg = (S3C6410_GPIO_REG *)pGPIOReg;
        LDI_INF((_T("[LDI:INF] g_pDispConReg = 0x%08x\n\r"), g_pDispConReg));
        LDI_INF((_T("[LDI:INF] g_pGPIOReg    = 0x%08x\n\r"), g_pGPIOReg));
    }

	

    LDI_MSG((_T("[LDI]--LDI_initialize_register_address() : %d\n\r"), error));

    return error;
}

LDI_ERROR LDI_set_LCD_module_type(HITEG_DISPLAY_TYPE ModuleType)
{
    LDI_ERROR error = LDI_SUCCESS;

    g_Type = ModuleType;

    LDI_MSG((_T("[LDI]++LDI_set_LCD_module_type(%d)\n\r"),g_Type));


    return error;
}

HITEG_DISPLAY_TYPE LDI_getDisplayType()
{
	return g_Type;
}
LDI_ERROR LDI_initialize_LCD_module(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]--LDI_initialize_LCD_module() : %d\n\r"), error));

    return error;
}

LDI_ERROR LDI_deinitialize_LCD_module(void)
{
    LDI_ERROR error = LDI_SUCCESS;

    return error;
}
DISP_VIDOUT_MODE LDI_getDisplayMode()
{
	return HitegDisplays[g_Type].mode;
}

LDI_ERROR LDI_fill_output_device_information(void *pDevInfo)
{
	tDevInfo *pDeviceInfo;
	DWORD g_Type;
    LDI_ERROR error = LDI_SUCCESS;

    LDI_MSG((_T("[LDI]++LDI_fill_output_device_information()\n\r")));
	
	g_Type=LDI_getDisplayType();

    if (pDevInfo == NULL)
    {
        LDI_ERR((_T("[LDI:ERR] LDI_fill_output_device_information() : Null Parameter\n\r")));
        error = DISP_ERROR_NULL_PARAMETER;
        return error;
    }

    pDeviceInfo = (tDevInfo *)pDevInfo;
	LDI_INF((_T("[LDI:INF] Output Devce Type [%s] \r\n"), LDI_getDisplayName(g_Type)));
	pDeviceInfo->VideoOutMode = HitegDisplays[g_Type].mode;
	pDeviceInfo->RGBOutMode = HitegDisplays[g_Type].intrfc;
	pDeviceInfo->uiWidth = HitegDisplays[g_Type].width;
	pDeviceInfo->uiHeight = HitegDisplays[g_Type].height;
	pDeviceInfo->VBPD_Value = HitegDisplays[g_Type].VBPD;
	pDeviceInfo->VFPD_Value = HitegDisplays[g_Type].VFPD;
	pDeviceInfo->VSPW_Value = HitegDisplays[g_Type].VSPW;
	pDeviceInfo->HBPD_Value = HitegDisplays[g_Type].HBPD;
	pDeviceInfo->HFPD_Value = HitegDisplays[g_Type].HFPD;
	pDeviceInfo->HSPW_Value = HitegDisplays[g_Type].HSPW;
	pDeviceInfo->VCLK_Polarity = HitegDisplays[g_Type].VCLK_POL;
	pDeviceInfo->HSYNC_Polarity = HitegDisplays[g_Type].HSYNC_POL;
	pDeviceInfo->VSYNC_Polarity = HitegDisplays[g_Type].VSYNC_POL;
	pDeviceInfo->VDEN_Polarity = HitegDisplays[g_Type].VDEN_POL;
	pDeviceInfo->PNR_Mode = HitegDisplays[g_Type].PNR_MODE;
	pDeviceInfo->VCLK_Source = HitegDisplays[g_Type].VCLK_SRC;
	//pDeviceInfo->VCLK_Source = CLKSEL_F_LCDCLK;
	LDI_ERR((_T("[LDI:ERR] VLCK_source: %d\n\r"), (pDeviceInfo->VCLK_Source)?S3C6410_DoutMPLL:S3C6410_HCLK) );
	pDeviceInfo->VCLK_Direction = HitegDisplays[g_Type].VCLK_DIR;
	pDeviceInfo->Frame_Rate = HitegDisplays[g_Type].FRAME_RATE;



    LDI_MSG((_T("[LDI]--LDI_fill_output_device_information()\n\r")));

    return error;
}



static void DelayLoop_1ms(int msec)
{
    volatile int j;
    for(j = 0; j < LCD_DELAY_1MS*msec; j++)  ;
}

static void DelayLoop(int delay)
{
    volatile int j;
    for(j = 0; j < delay; j++)  ;
}

