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

#include <windows.h>
#include <nkintr.h>
#include <ceddk.h>
#include <s3c6410.h>
#include <cebuscfg.h>
#include "s3c6410_hsmmc.h"

#define _SRCCLK_48MHZ_    // from USB PHY (Keep sync with "sdhcslot.cpp")

// Global Variables
LPCTSTR HostControllerName = TEXT("HSMMCCh0");
/*
static volatile S3C6410_GPIO_REG *pIOPreg = NULL;

#ifdef _SMDK6410_CH0_EXTCD_
// New Constructor for card detect of HSMMC ch0 on SMDK6410.
CSDHControllerCh0::CSDHControllerCh0() : CSDHCBase() {
    m_htCardDetectThread = NULL;
    m_hevCardDetectEvent = NULL;
    m_dwSDDetectSysIntr = SYSINTR_UNDEFINED;
}
#endif
*/
BOOL CSDHControllerCh0::Init(LPCTSTR pszActiveKey) {
    RETAILMSG(TRUE,(TEXT("[HSMMC0] Initializing the HSMMC Host Controller\n")));

    // HSMMC Ch0 initialization
    if (!InitCh()) return FALSE;
    return CSDHCBase::Init(pszActiveKey);
}

VOID CSDHControllerCh0::PowerUp() {
    RETAILMSG(TRUE,(TEXT("[HSMMC0] Power Up the HSMMC Host Controller\n")));

    // HSMMC Ch0 initialization for "WakeUp"
    if (!InitCh()) return;
    CSDHCBase::PowerUp();
}

extern "C" PCSDHCBase CreateHSMMCHCCh0Object() {
    return new CSDHControllerCh0;
}

VOID CSDHControllerCh0::DestroyHSMMCHCCh0Object(PCSDHCBase pSDHC) {
    DEBUGCHK(pSDHC);
    delete pSDHC;
}

// The function that initilize SYSCON for a clock gating.
BOOL CSDHControllerCh0::InitClkPwr() {
    volatile S3C6410_SYSCON_REG *pCLKPWR = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
    pCLKPWR = (volatile S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (pCLKPWR == NULL) {
        RETAILMSG(TRUE, (TEXT("[HSMMC0] Clock & Power Management Special Register is *NOT* mapped.\n")));
        return FALSE;
    }

#ifdef _SRCCLK_48MHZ_
    RETAILMSG(TRUE, (TEXT("[HSMMC0] Setting registers for the USB48MHz (EXTCLK for SDCLK) : SYSCon.\n")));
    // SCLK_HSMMC#_48 : CLK48M_PHY(OTH PHY 48MHz Clock Source from SYSCON block)
    // To use the USB clock, must be set the "USB_SIG_MASK" bit in the syscon register.
    pCLKPWR->OTHERS    |= (0x1<<16);  // set USB_SIG_MASK
    pCLKPWR->HCLK_GATE |= (0x1<<17);    // Gating HCLK for HSMMC0
    pCLKPWR->SCLK_GATE |= (0x1<<27);    // Gating special clock for HSMMC0 (SCLK_MMC0_48)
#else
    RETAILMSG(TRUE, (TEXT("[HSMMC0] Setting registers for the EPLL (for SDCLK) : SYSCon.\n")));
    // SCLK_HSMMC#  : EPLLout, MPLLout, PLL_source_clk or CLK27 clock
    // (from SYSCON block, can be selected by MMC#_SEL[1:0] fields of the CLK_SRC register in SYSCON block)
    // Set the clock source to EPLL out for CLKMMC1
    pCLKPWR->CLK_SRC   = (pCLKPWR->CLK_SRC & ~(0x3<<18) & ~(0x1<<2)) |  // Control MUX(MMC0:MOUT EPLL)
        (0x1<<2); // Control MUX(EPLL:FOUT EPLL)
    pCLKPWR->HCLK_GATE |= (0x1<<17);  // Gating HCLK for HSMMC0
    pCLKPWR->SCLK_GATE  = (pCLKPWR->SCLK_GATE) | (0x1<<24);  // Gating special clock for HSMMC1 (SCLK_MMC0)
#endif

    MmUnmapIoSpace((PVOID)pCLKPWR, sizeof(S3C6410_SYSCON_REG));

    return TRUE;
}

// The function that initilize GPIO for DAT, CD and WP lines.
BOOL CSDHControllerCh0::InitGPIO() {
    volatile S3C6410_GPIO_REG *pIOPreg = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    pIOPreg = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (pIOPreg == NULL) {
        RETAILMSG(TRUE, (TEXT("[HSMMC0] GPIO registers is *NOT* mapped.\n")));
        return FALSE;
    }
    RETAILMSG(TRUE, (TEXT("[HSMMC0] Setting registers for the GPIO.\n")));
    pIOPreg->GPGCON  = (pIOPreg->GPGCON & ~(0xFFFFFF)) | (0x222222);  // 4'b0010 for the MMC 0
    pIOPreg->GPGPUD &= ~(0xFFF); // Pull-up/down disabled
/*

#ifdef _SMDK6410_CH0_WP_
    pIOPreg->GPNCON &= ~(0x3<<24);  // WP_SD0
    pIOPreg->GPNPUD &= ~(0x3<<24);  // Pull-up/down disabled
#endif
*/

//#ifndef _SMDK6410_CH0_EXTCD_
    pIOPreg->GPGCON = (pIOPreg->GPGCON & ~(0xF<<24)) | (0x2<<24); // SD CD0
    pIOPreg->GPGPUD &= ~(0x3<<12); // Pull-up/down disabled
//#endif
/*

#ifdef _SMDK6410_CH0_EXTCD_
    // Setting for card detect pin of HSMMC ch0 on SMDK6410.
    pIOPreg->GPNCON    = ( pIOPreg->GPNCON & ~(0x3<<26) ) | (0x2<<26);    // SD_CD0 by EINT13
    pIOPreg->GPNPUD         = ( pIOPreg->GPNPUD & ~(0x3<<26) ) | (0x0<<26);  // pull-up/down disabled

    pIOPreg->EINT0CON0 = ( pIOPreg->EINT0CON0 & ~(0x7<<24)) | (0x7<<24);    // Both edge triggered
    pIOPreg->EINT0PEND = ( pIOPreg->EINT0PEND | (0x1<<13) );     //clear EINT19 pending bit
    pIOPreg->EINT0MASK = ( pIOPreg->EINT0MASK & ~(0x1<<13));     //enable EINT19
#endif
*/
    MmUnmapIoSpace((PVOID)pIOPreg, sizeof(S3C6410_GPIO_REG));
    return TRUE;
}

// The function that initilize the register for HSMMC Control.
BOOL CSDHControllerCh0::InitHSMMC() {
    volatile S3C6410_HSMMC_REG *pHSMMC = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_HSMMC0;
    pHSMMC = (volatile S3C6410_HSMMC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_HSMMC_REG), FALSE);
    if (pHSMMC == NULL) {
        RETAILMSG(TRUE, (TEXT("[HSMMC0] HSMMC Special Register is *NOT* mapped.\n")));
        return FALSE;
    }

#ifdef _SRCCLK_48MHZ_
    RETAILMSG(TRUE, (TEXT("[HSMMC0] Setting registers for the USB48MHz (EXTCLK) : HSMMCCon.\n")));
    // Set the clock source to USB_PHY for CLKMMC0
    pHSMMC->CONTROL2 = (pHSMMC->CONTROL2 & ~(0xffffffff)) |
        (0x3<<9) |  // Debounce Filter Count 0x3=64 iSDCLK
        (0x1<<8) |  // SDCLK Hold Enable
        (0x3<<4);   // Base Clock Source = External Clock
#else
    RETAILMSG(TRUE, (TEXT("[HSMMC0] Setting registers for the EPLL : HSMMCCon.\n")));
    // Set the clock source to EPLL out for CLKMMC0
    pHSMMC->CONTROL2 = (pHSMMC->CONTROL2 & ~(0xffffffff)) |
        (0x3<<9) |  // Debounce Filter Count 0x3=64 iSDCLK
        (0x1<<8) |  // SDCLK Hold Enable
        (0x2<<4);   // Base Clock Source = EPLL out
#endif

    MmUnmapIoSpace((PVOID)pHSMMC, sizeof(S3C6410_HSMMC_REG));
    return TRUE;
}

BOOL CSDHControllerCh0::InitCh() {
    if (!InitClkPwr()) return FALSE;
    if (!InitGPIO()) return FALSE;
    if (!InitHSMMC()) return FALSE;
    return TRUE;
}
/*
#ifdef _SMDK6410_CH0_EXTCD_
// New function to Card detect thread of HSMMC ch0 on SMDK6410.
DWORD CSDHControllerCh0::CardDetectThread() {
    BOOL  bSlotStateChanged = FALSE;
    DWORD dwWaitResult  = WAIT_TIMEOUT;
    PCSDHCSlotBase pSlotZero = GetSlot(0);

    CeSetThreadPriority(GetCurrentThread(), 100);

    while(1) {
        // Wait for the next insertion/removal interrupt
        dwWaitResult = WaitForSingleObject(m_hevCardDetectEvent, INFINITE);

        Lock();
        pSlotZero->HandleInterrupt(SDSLOT_INT_CARD_DETECTED);
        Unlock();
        InterruptDone(m_dwSDDetectSysIntr);

        EnableCardDetectInterrupt();
    }

    return TRUE;
}

// New function to request a SYSINTR for Card detect interrupt of HSMMC ch0 on SMDK6410.
BOOL CSDHControllerCh0::InitializeHardware() {
    m_dwSDDetectIrq = SD_CD0_IRQ;

    // convert the SDI hardware IRQ into a logical SYSINTR value
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &m_dwSDDetectIrq, sizeof(DWORD), &m_dwSDDetectSysIntr, sizeof(DWORD), NULL)) {
        // invalid SDDetect SYSINTR value!
        RETAILMSG(TRUE, (TEXT("[HSMMC0] invalid SD detect SYSINTR value!\n")));
        m_dwSDDetectSysIntr = SYSINTR_UNDEFINED;
        return FALSE;
    }

    return CSDHCBase::InitializeHardware();
}

// New Start function for Card detect of HSMMC ch0 on SMDK6410.
SD_API_STATUS CSDHControllerCh0::Start() {
    SD_API_STATUS status = SD_API_STATUS_INSUFFICIENT_RESOURCES;

    m_fDriverShutdown = FALSE;

    // allocate the interrupt event
    m_hevInterrupt = CreateEvent(NULL, FALSE, FALSE,NULL);

    if (NULL == m_hevInterrupt)    {
        goto EXIT;
    }

    // initialize the interrupt event
    if (!InterruptInitialize (m_dwSysIntr, m_hevInterrupt, NULL, 0)) {
        goto EXIT;
    }

    m_fInterruptInitialized = TRUE;

    // create the interrupt thread for controller interrupts
    m_htIST = CreateThread(NULL, 0, ISTStub, this, 0, NULL);
    if (NULL == m_htIST) {
        goto EXIT;
    }

    // allocate the card detect event
    m_hevCardDetectEvent = CreateEvent(NULL, FALSE, FALSE,NULL);

    if (NULL == m_hevCardDetectEvent) {
        goto EXIT;
    }

    // initialize the interrupt event
    if (!InterruptInitialize (m_dwSDDetectSysIntr, m_hevCardDetectEvent, NULL, 0)) {
        goto EXIT;
    }

    // create the card detect interrupt thread
    m_htCardDetectThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SD_CardDetectThread, this, 0, NULL);
    if (NULL == m_htCardDetectThread)    {
        goto EXIT;
    }

    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PCSDHCSlotBase pSlot = GetSlot(dwSlot);
        status = pSlot->Start();

        if (!SD_API_SUCCESS(status)) {
            goto EXIT;
        }
    }

    // wake up the interrupt thread to check the slot
    ::SetInterruptEvent(m_dwSDDetectSysIntr);

    status = SD_API_STATUS_SUCCESS;

EXIT:
    if (!SD_API_SUCCESS(status)) {
        // Clean up
        Stop();
    }

    return status;
}

// New function for enabling the Card detect interrupt of HSMMC ch0 on SMDK6410.
BOOL CSDHControllerCh0::EnableCardDetectInterrupt() {
    pIOPreg->EINT0PEND = ( pIOPreg->EINT0PEND |  (0x1<<13));     //clear EINT19 pending bit
    pIOPreg->EINT0MASK = ( pIOPreg->EINT0MASK & ~(0x1<<13));     //enable EINT19
    return TRUE;
}
#endif    // The end of BSP_SMDK6410
*/
