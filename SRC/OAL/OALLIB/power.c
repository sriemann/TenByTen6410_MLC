//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include <windows.h>
#include <bsp.h>
#include <pmplatform.h>
#ifdef _IROM_SDMMC_
#include <DrvLib.h>
#endif
#include "ExtPowerCTL_LIB.h"


#define OEM_IMPLEMENT   (FALSE)
static UINT32 g_LastWakeupStatus = 0;

extern INT32 NAND_Init(VOID);

static void BSPConfigGPIOforPowerOff(void);
static void S3C6410_WakeUpSource_Configure(void);
static void S3C6410_WakeUpSource_Detect(void);

VOID BSPPowerOff()
{
    volatile S3C6410_GPIO_REG *pGPIOReg;
    volatile S3C6410_ADC_REG *pADCReg;
    volatile S3C6410_RTC_REG *pRTCReg;
    volatile S3C6410_SYSCON_REG *pSysConReg;

    OALMSG(OAL_FUNC, (TEXT("++BSPPowerOff()\n")));
    OALMSG(1, (TEXT("++BSPPowerOff()\n")));

    pGPIOReg = (S3C6410_GPIO_REG*)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    pADCReg = (S3C6410_ADC_REG*)OALPAtoVA(S3C6410_BASE_REG_PA_ADC, FALSE);
    pRTCReg = (S3C6410_RTC_REG*)OALPAtoVA(S3C6410_BASE_REG_PA_RTC, FALSE);
    pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    //-----------------------------
    // Wait till NAND Erase/Write operation is finished
    //-----------------------------
    VFL_Sync();

    //-----------------------------
    // Disable DVS and Set to Full Speed
    //-----------------------------
    ChangeDVSLevel(SYS_L0);

    // RTC Control Disable
    pRTCReg->RTCCON = 0x0;            // Subclk 32768 Hz, No Reset, Merged BCD counter, XTAL 2^-15, Control Disable

    //-------------------------------
    // GPIO Configuration for Sleep State
    //-------------------------------
    BSPConfigGPIOforPowerOff();
	PWRCTL_Sleep();
    //
    //CLRPORT32(&pIOPort->GPGDAT, 1 << 4);
    //----------------------------
    // Wake Up Source Configuration
    //----------------------------
    S3C6410_WakeUpSource_Configure();

    OALMSG(OAL_FUNC, (TEXT("--BSPPowerOff()\n")));
    OALMSG(1, (TEXT("--BSPPowerOff()\n")));
}


VOID BSPPowerOn()
{
    OALMSG(OAL_FUNC, (TEXT("++BSPPowerOn()\n")));

// The OEM can add BSP specific procedure here when system power up
    //----------------------------
    // Wake Up Source Determine
    //----------------------------
    S3C6410_WakeUpSource_Detect();

#ifdef _IROM_SDMMC_
    if (!BootDeviceInit())
    {
        OALMSG(OAL_ERROR, (TEXT("[OAL:ERR] BootDeviceInit() returned FALSE\r\n")));
    }
#endif

	// NAND Controller Initialize
	NAND_Init();
	PWRCTL_Awake();
    OALMSG(OAL_FUNC, (TEXT("--BSPPowerOn()\n")));
}

static void S3C6410_WakeUpSource_Configure(void)
{
    volatile S3C6410_SYSCON_REG *pSysConReg;
    volatile S3C6410_GPIO_REG *pGPIOReg;
#ifdef    SLEEP_AGING_TEST
    volatile S3C6410_RTC_REG *pRTCReg;
#endif

    pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);
    pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
#ifdef    SLEEP_AGING_TEST
    pRTCReg = (S3C6410_RTC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_RTC, FALSE);
#endif

    // Wake Up Source Mask
    pSysConReg->PWR_CFG = (pSysConReg->PWR_CFG & ~(0x1FF80))
                            |(1<<16)    // MMC2        (Disabled, Only for Stop)
                            |(1<<15)    // MMC1        (Disabled, Only for Stop)
                            |(1<<14)    // MMC0        (Disabled, Only for Stop)
                            |(1<<13)    // HSI        (Disabled)
                            |(1<<12)    // Touch        (Disabled, Only for Stop)
#ifdef    SLEEP_AGING_TEST
                            |(0<<11)    // RTC Tick    (Enabled)
#else
                            |(1<<11)    // RTC Tick    (Disabled)
#endif
                            |(0<<10)    // RTC ALARM    (Enabled)
                            |(1<<9)        // MSM        (Disabled)
#ifdef    REMOVE_KBD_WAKEUP_SRC        // Remove Keypad I/F from wake up source. if keyboard driver is not loaded
                            |(1<<8)        // Keypad        (Disabled)
#else
                            |(0<<8)        // Keypad        (Enabled)
#endif
                            |(1<<7);        // Battery Fault    (Disabled)

    //-----------------
    // External Interrupt
    //-----------------
    // Power Button EINT[9] (GPN[9] is Retention Port)
    //pGPIOReg->GPNCON = (pGPIOReg->GPNCON & ~(0x3<<22)) | (0x2<<22);    // GPN[11] as EINT[11]

    pSysConReg->EINT_MASK = 0x0FFFFFFF;        // Mask All EINT Wake Up Source at Sleep
    //pSysConReg->EINT_MASK &= ~(1<<11);        // Enable EINT[11] as Wake Up Source at Sleep

    //-----------------
    // RTC Tick
    //-----------------
#ifdef    SLEEP_AGING_TEST
    pRTCReg->TICCNT = 0x4000;    // 0.5 sec @ 32.768KHz
    pRTCReg->RTCCON |= (1<<8);    // Tick Timer Enable
#endif

    // Clear All Wake Up Status bits
    pSysConReg->WAKEUP_STAT = 0xfff;

}

static void S3C6410_WakeUpSource_Detect(void)
{
    volatile S3C6410_SYSCON_REG *pSysConReg;
    volatile S3C6410_GPIO_REG *pGPIOReg;
#ifdef    SLEEP_AGING_TEST
    volatile S3C6410_RTC_REG *pRTCReg;
#endif

    pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);
    pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
#ifdef    SLEEP_AGING_TEST
    pRTCReg = (S3C6410_RTC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_RTC, FALSE);
#endif

    g_LastWakeupStatus = pSysConReg->WAKEUP_STAT;

    switch(g_LastWakeupStatus)
    {
    case 0x1:    // External Interrupt
        if (pGPIOReg->EINT0PEND&(1<<9))        // Power Button : EINT[9]
        {
            g_oalWakeSource = SYSWAKE_POWER_BUTTON;    // OEMWAKE_EINT9;
            pGPIOReg->EINT0PEND = (1<<9);    // Clear Pending (Power Button Driver No Need to Handle Wake Up Interrupt)
        }
        else
        {
            // The else case here will not happen because all other external interrupts were
            // maksed off in Configure function.
            // This is can be expended for other external interrupt by device maker
            g_oalWakeSource = SYSWAKE_UNKNOWN;
        }
        break;
    case 0x2:    // RTC Alarm
        g_oalWakeSource = OEMWAKE_RTC_ALARM;
        break;
    case 0x4:    // RTC Tick
        g_oalWakeSource = OEMWAKE_RTC_TICK;
#ifdef    SLEEP_AGING_TEST
            pRTCReg->RTCCON &= ~(1<<8);    // Tick Timer Disable
            pRTCReg->INTP = 0x1;                // Clear RTC Tick Interrupt Pending
#endif
        break;
    case 0x10:    // Keypad
        g_oalWakeSource = OEMWAKE_KEYPAD;
        break;
    case 0x20:    // MSM
        g_oalWakeSource = OEMWAKE_MSM;
        break;
    case 0x40:    // BATFLT
        g_oalWakeSource = OEMWAKE_BATTERY_FAULT;
        break;
    case 0x80:    // WRESET
        g_oalWakeSource = OEMWAKE_WARM_RESET;
        break;
    case 0x100:    // HSI
        g_oalWakeSource = OEMWAKE_HSI;
        break;
    default:        // Unknown or Multiple Wakeup Source ???
        g_oalWakeSource = SYSWAKE_UNKNOWN;
        OALMSG(OAL_ERROR, (L"[OEM:ERR] OEMPowerOff() : SYSWAKE_UNKNOWN , WAKEUP_STAT = 0x%08x", g_LastWakeupStatus));
        break;
    }

    // Clear All Wake Up Status bits
    pSysConReg->WAKEUP_STAT = 0xfff;

    // EINT_MASK in SysCon Also Effective in Normal State
    // Unmask all bit for External Interrupt at Normal Mode
    pSysConReg->EINT_MASK = 0x0;
}

static void BSPConfigGPIOforPowerOff(void)
{
#if OEM_IMPLEMENT
    volatile S3C6410_GPIO_REG *pGPIOReg;
#endif

    OALMSG(OAL_FUNC, (TEXT("++BSPConfigGPIOforPowerOff()\n")));
    OALMSG(1, (TEXT("++BSPConfigGPIOforPowerOff()\n")));

    // The OEM can implement this function to cut GPIO when system falls into sleep mode.
#if OEM_IMPLEMENT
    pGPIOReg = (S3C6410_GPIO_REG*)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // VDD_OP1 IO
    // GPF[7:0], GPG[7:0], GPE[15:14], GPE[4:0]

    //GPF
    pIOPort->GPFCON &=~(0xffff<<0);
    pIOPort->GPFCON |= ((0x1<<14)|(0x1<<12)|(0x1<<10)|(0x1<<8)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x2<<0));
    pIOPort->EXTINT0 = (READEXTINT0(pIOPort->EXTINT0) & ~((1<<31) | (1<<27) | (1<<23) | (1<<19) | (1<<15) | (1<<11) | (1<<7) | (1<<3) ));
    pIOPort->EXTINT0 = (READEXTINT0(pIOPort->EXTINT0)  | ((1<<31) | (1<<27) | (1<<23) | (1<<19) | (0<<15) | (1<<11) | (1<<7) | (1<<3) )); //rEXTINT0[11] = PD_dis(Because of VBUS_DET)
    pIOPort->GPFDAT &=~ (0xff<<0);
    pIOPort->GPFDAT |= ((0x0<<7)|(0x0<<6)|(0x0<<5)|(0x0<<4)|(0x0<<3)|(0x0<<2)|(0x0<<1)|(0x0<<0));

    //GPG
    pIOPort->GPGCON &=~(0xffff<<0);
    pIOPort->GPGCON |= ((0x0<<14)|(0x0<<12)|(0x0<<10)|(0x0<<8)|(0x2<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));

    pIOPort->GPGUDP &=~(0xffff<<0);
    pIOPort->EXTINT1 = (READEXTINT1(pIOPort->EXTINT1) | (0<<31) | (0<<27) | (0<<23) | (0<<19) | (1<<15) | (0<<11) | (0<<7)| (0<<0) );

    pIOPort->GPGDAT &=~ (0xff<<0);
    pIOPort->GPGDAT |= ((0x1<<7)|(0x0<<6)|(0x1<<5)|(0x1<<4)|(0x0<<3)|(0x1<<2)|(0x1<<1)|(0x1<<0));

    //GPA
    pIOPort->GPACDH = (READGPACDH(pIOPort->GPACDL,pIOPort->GPACDH) | (1<<16) | (1<<5));    //GPACDH[16] is exclusive use of nRSTOUT and '1' is PRESET


    // DP0 / DN0
    pIOPort->MISCCR |= (1<<12); //Set USB port as suspend mode


    ///////////////////////////////////////////////////////////////////////////////////////////
    // VDD_OP2 IO
    // GPB[10:0], GPH[12:0], GPE[15:14], GPE[4:0]


    //GPB
    pIOPort->GPBCON &=~(0x3fffff<<0);
    pIOPort->GPBCON |= ((0x0<<20)|(0x0<<18)|(0x0<<16)|(0x0<<14)|(0x0<<12)|(0x0<<10)|(0x0<<8)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x3<<0));

    pIOPort->GPBUDP = (pIOPort->GPBUDP &=~(0x3fffff<<0));
    pIOPort->GPBUDP |= ((0x3<<20)|(0x2<<18)|(0x2<<16)|(0x2<<14)|(0x2<<12)|(0x2<<10)|(0x3<<8)|(0x3<<6)|(0x3<<4)|(0x0<<2)|(0x3<<0));


    //GPE
    pIOPort->GPECON &=~((0xf<<28)|(0x3ff<<0));
    pIOPort->GPECON |= ((0x0<<30)|(0x0<<28)|(0x0<<8)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));

    pIOPort->GPEUDP &=~((0xf<<28)|(0x3ff<<0));
    pIOPort->GPEUDP |= ((0x0<<30)|(0x0<<28)|(0x0<<8)|(0x3<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));

    pIOPort->GPEDAT &=~ (0xffff<<0);
    pIOPort->GPEDAT |= ((0x1<<15)|(0x1<<14)|(0x1<<4)|(0x1<<3)|(0x1<<2)|(0x1<<1)|(0x1<<0));
    //printf("rgPEcon=%08x, rgPEUDP=%08x, rGPeDAT=%08x\n", rGPECON,rGPEUDP,rGPEDAT);


    //GPG
    pIOPort->GPGCON &=~(0xffff<<16);
    pIOPort->GPGCON |= ((0x0<<30)|(0x0<<28)|(0x0<<26)|(0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x0<<16));

    pIOPort->GPGUDP &=~(0xffff<<16);
    pIOPort->GPGUDP |= ((0x2<<30)|(0x0<<28)|(0x3<<26)|(0x3<<24)|(0x0<<22)|(0x3<<20)|(0x3<<18)|(0x3<<16));

    pIOPort->GPGDAT &=~ (0xff<<8);
    pIOPort->GPGDAT |= ((0x0<<15)|(0x0<<14)|(0x0<<13)|(0x0<<12)|(0x0<<11)|(0x0<<10)|(0x0<<9)|(0x0<<8));



    //GPH
    pIOPort->GPHCON &=~(0x3fffffff<<0);
    pIOPort->GPHCON |= ((0x0<<28)|(0x0<<26)|(0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x0<<16)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));

    pIOPort->GPHUDP &=~(0x3fffffff<<0);
    pIOPort->GPHUDP |= ((0x0<<28)|(0x0<<26)|(0x0<<24)|(0x0<<22)|(0x3<<20)|(0x0<<18)|(0x3<<16)|(0x3<<6)|(0x0<<4)|(0x3<<2)|(0x0<<0));
    pIOPort->GPHDAT &=~ (0x7fff<<0);
    pIOPort->GPHDAT |= ((0x0<<14)|(0x1<<13)|(0x0<<12)|(0x1<<11)|(0x0<<10)|(0x1<<9)|(0x1<<8)|(0x0<<3)|(0x1<<2)|(0x1<<1)|(0x1<<0));



    ///////////////////////////////////////////////////////////////////////////////////////////
    // VDD_SD IO
    // GPE[13:5], GPL[14:0], GPJ[15:13]

    //GPE
    pIOPort->GPECON &=~(0x3ffff<<10);
    pIOPort->GPECON |= ((0x0<<26)|(0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x0<<16)|(0x0<<14)|(0x0<<12)|(0x0<<10));//Set Input All

    pIOPort->GPEUDP &=~(0x3ffff<<10);
    pIOPort->GPEUDP |= ((0x0<<26)|(0x0<<24)|(0x0<<22)|(0x3<<20)|(0x3<<18)|(0x3<<16)|(0x3<<14)|(0x3<<12)|(0x0<<10));

    pIOPort->GPEDAT &=~ (0x1ff<<5);
    pIOPort->GPEDAT |= ((0x0<<13)|(0x0<<12)|(0x0<<11)|(0x0<<10)|(0x1<<9)|(0x1<<8)|(0x0<<7)|(0x1<<6)|(0x1<<5));


    //GPL
    pIOPort->GPLCON &=~(0x3fffffff<<0);
    pIOPort->GPLCON |= ((0x0<<28)|(0x0<<26)|(0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x0<<16)|(0x0<<14)|(0x0<<12)|(0x0<<10)|(0x0<<8)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));
    pIOPort->GPLUDP &=~(0x3fffffff<<0);
    pIOPort->GPLUDP |= ((0x0<<28)|(0x0<<26)|(0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x3<<16)|(0x3<<14)|(0x3<<12)|(0x3<<10)|(0x3<<8)|(0x3<<6)|(0x3<<4)|(0x3<<2)|(0x3<<0));
    pIOPort->GPLDAT &=~ (0x1fff<<0);
    pIOPort->GPLDAT |= ((0x0<<14)|(0x0<<13)|(0x0<<12)|(0x0<<11)|(0x0<<10)|(0x1<<9)|(0x1<<8)|(0x1<<3)|(0x1<<2)|(0x1<<1)|(0x1<<0));

    //GPJ
    pIOPort->GPJCON &=~(0x3f<<26);
    pIOPort->GPJCON |= ((0x0<<30)|(0x0<<28)|(0x0<<26));

    pIOPort->GPJUDP &=~(0x3f<<26);
    pIOPort->GPJUDP |= ((0x0<<30)|(0x0<<28)|(0x0<<26));

    pIOPort->GPJDAT &=~ (0x7<<13);
    pIOPort->GPJDAT |= ((0x1<<15)|(0x1<<14)|(0x1<<13));

    //GPH - This configuration is setting for CFG3(UART) set as [off:off:off:off]
    pIOPort->GPHCON &= ~((0x3<<14)|(0x3<<12)|(0x3<<10)|(0x3<<8));
    pIOPort->GPHUDP &= ~((0x3<<14)|(0x3<<12)|(0x3<<10)|(0x3<<8));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // VDD_LCD IO
    // GPC[15:0], GPD[15:0]

    //GPC
    pIOPort->GPCCON &=~(0xffffffff<<0);
    pIOPort->GPCCON |= ((0x0<<30)|(0x0<<28)|(0x0<<26)|(0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x0<<16)\
    |(0x0<<14)|(0x0<<12)|(0x0<<10)|(0x0<<8)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));

    pIOPort->GPCUDP &=~(0xffffffff<<0);
    pIOPort->GPCUDP |= ((0x2<<30)|(0x2<<28)|(0x2<<26)|(0x2<<24)|(0x2<<22)|(0x2<<20)|(0x2<<18)|(0x2<<16)
    |(0x2<<14)|(0x2<<12)|(0x2<<10)|(0x2<<8)|(0x2<<6)|(0x2<<4)|(0x2<<2)|(0x2<<0));

    pIOPort->GPCDAT &=~ (0x1fff<<0);
    pIOPort->GPCDAT |= ((0x0<<14)|(0x1<<13)|(0x0<<12)|(0x1<<11)|(0x0<<10)|(0x1<<9)|(0x1<<8)|(0x0<<3)|(0x1<<2)|(0x1<<1)|(0x1<<0));

    //GPD
    pIOPort->GPDCON &=~(0xffffffff<<0);
    pIOPort->GPDCON |= ((0x0<<30)|(0x0<<28)|(0x0<<26)|(0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x0<<16)\
    |(0x0<<14)|(0x0<<12)|(0x0<<10)|(0x0<<8)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));


    pIOPort->GPDUDP &=~(0xffffffff<<0);
    pIOPort->GPDUDP |= ((0x2<<30)|(0x2<<28)|(0x2<<26)|(0x2<<24)|(0x2<<22)|(0x2<<20)|(0x2<<18)|(0x2<<16)\
    |(0x2<<14)|(0x2<<12)|(0x2<<10)|(0x2<<8)|(0x2<<6)|(0x2<<4)|(0x2<<2)|(0x2<<0));

    pIOPort->GPDDAT &=~ (0xffff<<0);
    pIOPort->GPDDAT |= ((0x0<<14)|(0x1<<13)|(0x0<<12)|(0x1<<11)|(0x0<<10)|(0x1<<9)|(0x1<<8)|(0x0<<3)|(0x1<<2)|(0x1<<1)|(0x1<<0));


    ///////////////////////////////////////////////////////////////////////////////////////////
    // VDD_CAM IO
    // GPJ[15:0],

    //GPJ
    pIOPort->GPJCON &=~(0x3ffffff<<0);
    pIOPort->GPJCON |= ((0x0<<24)|(0x0<<22)|(0x0<<20)|(0x0<<18)|(0x0<<16)\
    |(0x0<<14)|(0x0<<12)|(0x0<<10)|(0x0<<8)|(0x0<<6)|(0x0<<4)|(0x0<<2)|(0x0<<0));

    pIOPort->GPJUDP &=~(0x3ffffff<<0);
    pIOPort->GPJUDP |= ((0x2<<24)|(0x2<<22)|(0x2<<20)|(0x2<<18)|(0x2<<16)\
    |(0x2<<14)|(0x2<<12)|(0x2<<10)|(0x2<<8)|(0x2<<6)|(0x2<<4)|(0x2<<2)|(0x2<<0));

    pIOPort->GPJDAT &=~ (0x1fff<<0);
    pIOPort->GPJDAT |= ((0x0<<12)|(0x1<<11)|(0x0<<10)|(0x1<<9)|(0x1<<8)\
    |(0x0<<7)|(0x1<<6)|(0x1<<5)|(0x1<<4)|(0x0<<3)|(0x1<<2)|(0x1<<1)|(0x1<<0));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // VDD_RMOP
    pIOPort->MSLCON = 0x0;
    pIOPort->GPMCON &=~((0x3<<2) | (0x3<<0));
    pIOPort->GPMUDP &=~((0x3<<4) |(0x3<<2) | (0x3<<0));
    pIOPort->GPMUDP |= ((0x3<<4) | (0x3<<2) | (0x3<<0));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // VDD_SMOP : sleep wakeup iteration fail or not?
    pIOPort->MSLCON = 0x0;
    pIOPort->DATAPDEN &=~((0x1<<0)|(0x1<<1)|(0x1<<2)|(0x1<<3)|(0x1<<4)|(0x1<<5)); // reset value = 0x3f; --> 0x30 = 2uA
    pIOPort->DATAPDEN = (0x3<<4);

#endif
    OALMSG(OAL_FUNC, (TEXT("--BSPConfigGPIOforPowerOff()\n")));
    OALMSG(1, (TEXT("--BSPConfigGPIOforPowerOff()\n")));
}

