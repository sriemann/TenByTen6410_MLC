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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
//
//------------------------------------------------------------------------------
//
//  File: Backlight_pdd.c
//
//  Backlight PDD driver source code, S3C6410 
//
#include <windows.h>
#include <ceddk.h>
#include "backlight_pdd.h"
#include <s3c6410.h>
#include "bsp_cfg.h"

#ifdef DEBUG
#define ZONE_BACKLIGHT      DEBUGZONE(0)
#define ZONE_FUNCTION       DEBUGZONE(1)
#define ZONE_ERROR          DEBUGZONE(15)
#else
#define ZONE_BACKLIGHT      1
#define ZONE_FUNCTION       1
#define ZONE_ERROR          1
#endif


#define PWM0_1_PRESCALER 0x10
#define PWM1_DIVIDER 0x3
#define PWM_TCNTB1 5000
#define PWM_TCMPB1_UNIT 50
#define LOW_BACKLIGHT 5

volatile static S3C6410_GPIO_REG *v_pGPIORegs=NULL;
volatile static S3C6410_PWM_REG *v_pPWMRegs=NULL;

DWORD g_dwD0Brightness=0;
//-----------------------------------------------------------------------------
//  Initialize hardware etc
//  Returned DWORD will be passed to BacklightDeInit and should be used to store context if necessary
//  pDeviceState should be set to the start state of the backlight (usually D0)
//
extern "C"
void BL_InitPWM()
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("[BKL_PDD]BL_InitPWM Enter\r\n")));
    v_pPWMRegs->TCFG0 = v_pPWMRegs->TCFG0 & (~(0xff<<0)|(PWM0_1_PRESCALER<<0));
	 v_pPWMRegs->TCFG1 = v_pPWMRegs->TCFG1&(~(0xf<<0))|(PWM1_DIVIDER<<0);     // Timer0 Devider set 
    v_pPWMRegs->TCON = v_pPWMRegs->TCON&(~(1<<3))|(1<<3);                  // Enable Timer0 auto reload 
    v_pPWMRegs->TCON = v_pPWMRegs->TCON&(~(1<<1))|(1<<1);                    // Timer0 manual update clear (TCNTB0 & TCMPB0)
}


//
// turn on/off the backlight
//
extern "C"
void BL_Set(BOOL bOn)
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("[BKL_PDD]BL_Set(%d)\r\n"),(int)bOn));
   int i = 0;
    DEBUGMSG(ZONE_FUNCTION,(TEXT("[BKL_PDD]BL_Set(%d)\r\n"),(int)bOn));

    if(bOn) 
    {

    //BackLight on
	  v_pGPIORegs->GPFCON &= ~(0x3<<28); 
	  v_pGPIORegs->GPFCON |= (0x2<<28);
      v_pGPIORegs->GPFPUD &= ~(0x3<<28);
      v_pGPIORegs->GPFDAT |= (0x1<<14);  // GPF14=OUT high
    }
    else 
    {
      v_pGPIORegs->GPFDAT = v_pGPIORegs->GPFDAT&(~(1<<14))|(0<<14);  // Back Light power off
      v_pGPIORegs->GPFCON = v_pGPIORegs->GPFCON&(~(3<<28))|(1<<28);  // GPF14 = OUTPUT  


      // Delay
		for (i = 0;  i < 1000; i++)
		{
			;
		}
   }
}


void BL_SetBrightness(DWORD dwValue)
{
    UINT32 u32BrightnessSet=0;
    u32BrightnessSet = PWM_TCMPB1_UNIT*dwValue;
    if(u32BrightnessSet<0)
        u32BrightnessSet=0;
    else if(u32BrightnessSet> (PWM_TCNTB1-1) )
        u32BrightnessSet= (PWM_TCNTB1-1);
    v_pPWMRegs->TCNTB0 = PWM_TCNTB1;
    v_pPWMRegs->TCMPB0 =  u32BrightnessSet;
    DEBUGMSG(ZONE_FUNCTION,(TEXT("[BKL_PDD] BacklightRegChanged: BrightNess=%d TCMPB1=%d\r\n"), dwValue, v_pPWMRegs->TCMPB1));

    v_pPWMRegs->TCON = v_pPWMRegs->TCON&(~(1<<0))|(1<<0);    //Timer0 start
    v_pPWMRegs->TCON = v_pPWMRegs->TCON&(~(1<<1))|(0<<1);    //Timer0̄ manual update set (TCNTB0 & TCMPB0)

}



extern "C"
DWORD BacklightInit(LPCTSTR pContext, LPCVOID lpvBusContext, CEDEVICE_POWER_STATE *pDeviceState)
{

    BOOL bRet = TRUE;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    DEBUGMSG(ZONE_FUNCTION, (TEXT("[BKL_PDD] Init\r\n")));

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    v_pGPIORegs = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (v_pGPIORegs == NULL)
    {
        RETAILMSG(ZONE_ERROR, (TEXT("[BKL_PDD] v_pGPIORegs MmMapIoSpace() Failed \n\r")));
        return FALSE;
    }

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_PWM;
    v_pPWMRegs = (volatile S3C6410_PWM_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_PWM_REG), FALSE);
    if (v_pPWMRegs == NULL)
    {
      RETAILMSG(ZONE_ERROR, (TEXT("[BKL_PDD] v_pPWMRegs MmMapIoSpace() Failed \n\r")));
      return FALSE;
    }

    if (v_pGPIORegs)
    {
      v_pGPIORegs->GPFPUD = v_pGPIORegs->GPFPUD&(~(0x3<<28)); //GPF14 Pull-up/down disabled
    }

    BL_InitPWM();
    BL_Set(TRUE);
    *pDeviceState = D0;

    return TRUE;
}


extern "C"
void BacklightDeInit(DWORD dwContext)
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("[BKL_PDD] DeInit\r\n")));

    if(v_pGPIORegs != NULL)
    {
        MmUnmapIoSpace((PVOID)v_pGPIORegs, sizeof(S3C6410_GPIO_REG));
        v_pGPIORegs = NULL;
    }
    if(v_pPWMRegs != NULL)
    {
        MmUnmapIoSpace((PVOID)v_pPWMRegs, sizeof(S3C6410_PWM_REG));
        v_pPWMRegs = NULL;
    }

}


extern "C"
BOOL BackLightSetState(DWORD dwContext, CEDEVICE_POWER_STATE state)
{
    // sets the backlight state (turns the backlight on and off)
    DEBUGMSG(ZONE_FUNCTION,(TEXT("[BKL_PDD] BackLightSetState (0x%08x)\r\n"),(DWORD)state));
    BOOL bRet=TRUE;
    switch (state)
    {
        case D0:
            BL_InitPWM();
            BL_SetBrightness(g_dwD0Brightness);
            BL_Set(TRUE);
            break;
        case D1:
            BL_SetBrightness(LOW_BACKLIGHT);
            break;
        case D2:
        case D3:
        case D4:
            BL_Set(FALSE);
			// BL_Set(TRUE);
            break;
        default:
            RETAILMSG(ZONE_ERROR, (L"+BackLightSetState - Unsupported power state!\r\n"));
            bRet=FALSE;
    }
    return bRet;
}

extern "C"
UCHAR BacklightGetSupportedStates()
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("[BKL_PDD] BacklightGetSupportedStates()\r\n")));
    return DX_MASK(D0) |DX_MASK(D1)|DX_MASK(D4);     //support D0,D1, D4 (ON, LOW,OFF)


}

extern "C"
DWORD BacklightIOControl(DWORD dwOpenContext, DWORD dwIoControlCode, LPBYTE lpInBuf, 
                   DWORD nInBufSize, LPBYTE lpOutBuf, DWORD nOutBufSize, 
                   LPDWORD lpBytesReturned)
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("[BKL_PDD] BacklightIOControl()\r\n")));

    // For IOCTls that MDD doesn't know. ie non-pm IOCTLs
    return ERROR_NOT_SUPPORTED;
}

extern "C"
void BacklightRegChanged(DWORD dwBrightness)
{
    // Called when the MDD gets a backlight registry changed event
    // eg: read brightness settings from registry and update backlight accordingly
    g_dwD0Brightness = dwBrightness;
    BL_SetBrightness(dwBrightness);
}



