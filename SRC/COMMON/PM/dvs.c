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
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    dvs.c

Abstract:       Dynamic Voltage and Frequency Scaling Implementation

Functions:


Notes:


--*/

#include <bsp.h>

// Power Transition Table
#include <pmplatform.h>
#include "dvs.h"
#include "s3c6410_pm.h"

extern void System_EnableIRQ(void);
extern void System_DisableIRQ(void);
extern void ChangeDivider(UINT32);
extern void ChangeToL1(UINT32);
extern void ChangeToL0(UINT32);
static volatile DWORD g_dwCurrentIdleTime;
static SYSTEM_ACTIVE_LEVEL g_CurrentLevel = SYS_L0;
static volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;
static volatile unsigned int *g_pOTGLinkReg = NULL;
#ifdef DVS_EN
static BOOL g_bBSP_DVSEN=TRUE;
#else
static BOOL g_bBSP_DVSEN=FALSE;
#endif

static BOOL g_bDVSEN=FALSE;

// ProfileTable[Lv][0] = Idle Rate
// ProfileTable[Lv][1] = Active Rate
// ProfileTable[Lv][2] = Shift Down count
// ProfileTable[Lv][3] = Shift Up count
static DWORD g_aDVFSProfileTable[SYS_LEVEL_MAX][4];
static BOOL g_bProfileDVS = FALSE;
static DWORD g_dwLastTickCount_NonDVS = 0;    // for Profile Non-DVS Idle rate
static DWORD g_dwLastIdleCount_NonDVS = 0;    // for Profile Non-DVS Idle rate

#define Steady_LockTime        0xE16
#define Transit_LockTime    0x2
#define MPS_1332GHz    ((1<<31)|(333<<16)|(3<<8)|(0<<0))
#define MPS_667GHz    ((1<<31)|(333<<16)|(3<<8)|(1<<0))

static BOOL CheckUSBinUse(void);

void InitializeDVS(void)
{
    if(!g_bBSP_DVSEN)
    {
        return;
    }
    g_pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);
    g_pOTGLinkReg = (unsigned int *)OALPAtoVA(S3C6410_BASE_REG_PA_USBOTG_LINK, FALSE);
    g_CurrentLevel = SYS_L0;    // Initial System Level

#ifdef    ENABLE_VOLTAGE_CONTROL
    PMIC_Init();

    // This is caused by SMDK board bug. You can erase this line if you use the other board.
//    PMIC_VoltageSet(SETVOLTAGE_BOTH, VoltageARM(g_CurrentLevel), 0);
#endif
}

void SetCurrentIdleTime(DWORD dwIdleTime)
{
    g_dwCurrentIdleTime = dwIdleTime;

}

static BOOL CheckUSBinUse(void)
{
    // Check USB Device in Use
    if (*g_pOTGLinkReg & (0x3<<18))
    {
        if (g_bDVSEN == TRUE)
        {
            g_bDVSEN = FALSE;     // Disable DVS
            if (g_CurrentLevel != SYS_L0)
            {
                ChangeDVSLevel(SYS_L0);
            }
            OALMSG(OAL_POWER && OAL_FUNC, (L"[DVS] DVS disabled by USB\r\n"));
        }
        return TRUE;    // Do not apply DVS, when USB is in Use
    }
    else
    {
        if (g_bDVSEN == FALSE)
        {
            // Enable DVS after USB operation finished
            g_bDVSEN = TRUE;
            OALMSG(OAL_POWER && OAL_FUNC, (L"[DVS] DVS enabled\r\n"));
        }
    }
    return FALSE;
}

void UpdateDVS(void)
{
    static IDLE_LOG tLastLogFine;   //< This value must be retained to next Update
    static IDLE_LOG tLastLogCoarse; //< This value must be retained to next Update

    IDLE_LOG tCurLog;
    DWORD dwCurrentMSec;
    DWORD dwCurrentIdleMSec;
    DWORD dwIdleTick, dwActiveTick; //< This variables is used for profile

    if(!g_bBSP_DVSEN)
    {
        return;
    }

    if(CheckUSBinUse())
    {
        return;
    }

    dwCurrentMSec = CurMSec;
    dwCurrentIdleMSec = g_dwCurrentIdleTime;

    if ( (dwCurrentMSec - tLastLogFine.dwTickCount) >=  DVS_UPDATE_PERIOD_FINE )
    {
        tCurLog.dwTickCount = dwCurrentMSec;
        tCurLog.dwIdleCount = g_dwCurrentIdleTime;
        tCurLog.dwIdleRate = (100*(dwCurrentIdleMSec-tLastLogFine.dwIdleCount))/(dwCurrentMSec-tLastLogFine.dwTickCount);

        dwIdleTick = dwCurrentIdleMSec-tLastLogFine.dwIdleCount;
        dwActiveTick = (dwCurrentMSec-tLastLogFine.dwTickCount)-dwIdleTick;

        tLastLogFine.dwTickCount = tCurLog.dwTickCount;
        tLastLogFine.dwIdleCount = tCurLog.dwIdleCount;
        tLastLogFine.dwIdleRate = tCurLog.dwIdleRate;

        if (tCurLog.dwIdleRate < ShiftUpRate)    // Pump Up the Clock
        {
            if (g_bProfileDVS)
            {
                g_aDVFSProfileTable[g_CurrentLevel][0] += dwIdleTick;        // Idle
                g_aDVFSProfileTable[g_CurrentLevel][1] += dwActiveTick;    // Active
                g_aDVFSProfileTable[g_CurrentLevel][3] ++ ; // Shift up
            }
            OALMSG(OAL_POWER && OAL_FUNC, ((L"U[%d:%d]\r\n"), g_CurrentLevel, tCurLog.dwIdleRate));
            ChangeDVSLevel(ShiftUpLevel);
        }
        else
        {
            if ( (dwCurrentMSec - tLastLogCoarse.dwTickCount) >=  DVS_UPDATE_PERIOD_COARSE )
            {
                tCurLog.dwIdleRate = (100*(dwCurrentIdleMSec-tLastLogCoarse.dwIdleCount))/(dwCurrentMSec-tLastLogCoarse.dwTickCount);
                if (g_bProfileDVS)
                {
                    dwIdleTick = dwCurrentIdleMSec-tLastLogCoarse.dwIdleCount;
                    dwActiveTick = (dwCurrentMSec-tLastLogCoarse.dwTickCount)-dwIdleTick;

                    g_aDVFSProfileTable[g_CurrentLevel][0] += dwIdleTick;        // Idle
                    g_aDVFSProfileTable[g_CurrentLevel][1] += dwActiveTick;    // Active
                    g_aDVFSProfileTable[g_CurrentLevel][2] ++; // Shift down
                }
                tLastLogCoarse.dwTickCount = tCurLog.dwTickCount;
                tLastLogCoarse.dwIdleCount = tCurLog.dwIdleCount;
                tLastLogCoarse.dwIdleRate = tCurLog.dwIdleRate;

                OALMSG(OAL_POWER && OAL_FUNC, ((L"D[%d:%d]\r\n"), g_CurrentLevel, tCurLog.dwIdleRate));

                if (tCurLog.dwIdleRate > ShiftDownRate)    // Pump Down the Clock
                {
                    ChangeDVSLevel(ShiftDownLevel);
                }

            }
        }
    }
}

void ChangeDVSLevel(SYSTEM_ACTIVE_LEVEL NewLevel)
{
    if(!g_bDVSEN)
    {
        return;
    }
    if (g_CurrentLevel == NewLevel)
    {
        // There is no need to change
        return;
    }
    else
    {
#ifdef    ENABLE_VOLTAGE_CONTROL
        if(g_CurrentLevel > NewLevel)    // Clock Speed of New Level is Slower, Voltage of New Level is Lower
        {
            PMIC_VoltageSet(SETVOLTAGE_ARM, VoltageARM(NewLevel), 100);
            PMIC_VoltageSet(SETVOLTAGE_INTERNAL, VoltageInternal(NewLevel), 100);
        }
#endif

        ChangeClockDivider(NewLevel);

#ifdef    ENABLE_VOLTAGE_CONTROL
        if(g_CurrentLevel < NewLevel)    // Clock Speed of New Level is Faster, Voltage of New Level is Higher
        {
            PMIC_VoltageSet(SETVOLTAGE_ARM, VoltageARM(NewLevel), 100);
            PMIC_VoltageSet(SETVOLTAGE_INTERNAL, VoltageInternal(NewLevel), 100);
        }
#endif
        OALMSG(OAL_POWER && OAL_FUNC, (L"[DVS] System Level Changed [%d -> %d]\r\n", g_CurrentLevel, NewLevel));

        // Update System Level Variable
        g_CurrentLevel = NewLevel;
    }
}

void ChangeClockDivider(SYSTEM_ACTIVE_LEVEL NewLevel)
{
    DWORD dwNewCLKDIV = 0;

    // Change System Clock Divider
    dwNewCLKDIV = (g_pSysConReg->CLK_DIV0 & ~((0xf<<28)|(0xf<<12)|(0x7<<9)|(0xf)))
                    | ((SysMFCLKDiv(NewLevel)-1)<<28)
                    | ((SysPCLKDiv(NewLevel)-1)<<12)
                    | ((SysHCLKx2Div(NewLevel)-1)<<9)
                    | (SysARMCLKDiv(NewLevel)-1);
    //OALMSG(OAL_POWER && OAL_FUNC, ((L"0x%x CV[%d,%d,%d,%d,%d] 0x%x\r\n"), g_pSysConReg, g_CurrentLevel, SysMFCLKDiv(NewLevel), SysPCLKDiv(NewLevel), SysHCLKx2Div(NewLevel), SysARMCLKDiv(NewLevel), dwNewCLKDIV));

    System_DisableIRQ();

    if ((NewLevel == SYS_L0) && (g_CurrentLevel != SYS_L0))         // L1~5 --> L0
    {
        #if ((TARGET_ARM_CLK == CLK_666MHz) && SYNCMODE)
        g_pSysConReg->APLL_LOCK = Transit_LockTime;
        ChangeToL0(dwNewCLKDIV);
        g_pSysConReg->APLL_LOCK = Steady_LockTime;

        #else
        ChangeDivider(dwNewCLKDIV);

        #endif
    }
    else if ((NewLevel != SYS_L0) && (g_CurrentLevel == SYS_L0))    // L0 --> L1~5
    {
        #if ((TARGET_ARM_CLK == CLK_666MHz) && SYNCMODE)
        g_pSysConReg->APLL_LOCK = Transit_LockTime;
        ChangeToL1(dwNewCLKDIV);
        g_pSysConReg->APLL_LOCK = Steady_LockTime;

        #else
        ChangeDivider(dwNewCLKDIV);

        #endif
    }
    else                                                            // L1~5 --> L1~5
    {
        ChangeDivider(dwNewCLKDIV);
    }

    System_EnableIRQ();

}


// for Profiling DVS transition and Measuring CPU idle and active rate
void ProfileDVSOnOff(BOOL bOnOff)
{
    int i;

    if(!g_bDVSEN)
    {
        return;
    }

    if (bOnOff)
    {
        for (i=0; i<SYS_LEVEL_MAX; i++)
        {
            g_aDVFSProfileTable[i][0] = 0;
            g_aDVFSProfileTable[i][1] = 0;
            g_aDVFSProfileTable[i][2] = 0;
            g_aDVFSProfileTable[i][3] = 0;
        }

        g_bProfileDVS = TRUE;

        // Profile Idle Rate in Non DVS Mode when USB cable is plugged
        if (*g_pOTGLinkReg & (0x3<<18))        // Check USB Device in Use
        {
            g_dwLastTickCount_NonDVS = CurMSec;
            g_dwLastIdleCount_NonDVS = g_dwCurrentIdleTime;

            OALMSG(TRUE, (L"[Non DVS] Profile Start...\r\n"));
        }
        else
        {
            OALMSG(TRUE, (L"[DVS] Profile Start...\r\n"));
        }
    }
    else
    {
        g_bProfileDVS = FALSE;

        // Profile Idle Rate in Non DVS Mode when USB cable is plugged
        if (*g_pOTGLinkReg & (0x3<<18))        // Check USB Device in Use
        {
            DWORD dwTick, dwIdle;

            dwTick = CurMSec - g_dwLastTickCount_NonDVS;
            dwIdle = g_dwCurrentIdleTime - g_dwLastIdleCount_NonDVS;
            g_dwLastTickCount_NonDVS = 0;
            g_dwLastIdleCount_NonDVS = 0;

            OALMSG(TRUE, (L"[Non DVS] Profile Stop...   Idle[%d ms] Active[%d ms]\r\n", dwIdle, dwTick-dwIdle));
        }
        else
        {
            OALMSG(TRUE, (L"[DVS] Profile Stop...\r\n"));
            OALMSG(TRUE, (L"Level(ARM,Internal)  Div(A,Hx2,P,MFC)  Idle/Active    Down/Up \r\n"));

            for (i=0; i<SYS_LEVEL_MAX; i++)
            {
                OALMSG(TRUE, ((L" %d  (%dmV,%dmV)   (%d,%d,%d,%d)    [%d ms]/[%d ms]   [%d]/[%d]\r\n"), i,
                           VoltageARM(i), VoltageInternal(i),
                           SysARMCLKDiv(i), SysHCLKx2Div(i), SysPCLKDiv(i), SysMFCLKDiv(i),
                           g_aDVFSProfileTable[i][0], g_aDVFSProfileTable[i][1],
                           g_aDVFSProfileTable[i][2], g_aDVFSProfileTable[i][3]));
            }
        }
    }
}

