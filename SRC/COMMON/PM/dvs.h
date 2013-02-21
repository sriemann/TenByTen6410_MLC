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

Module Name:    dvs.h

Abstract:       Dynamic Voltage and Frequency Scaling Interface & defines

Functions:


Notes:


--*/

#ifndef _DVS__H_
#define _DVS__H_

// DVS Control Definition
//#define ENABLE_VOLTAGE_CONTROL        // Not yet Tested
//#define DVS_LEVEL_PROFILE

#define DVS_UPDATE_PERIOD_FINE      (100)
#define DVS_UPDATE_PERIOD_COARSE    (1000)

typedef struct
{
    DWORD dwTickCount;
    DWORD dwIdleCount;
    DWORD dwIdleRate;
} IDLE_LOG;

//-----------------------------------------------------------------------------------
// Following System Clock configuration Level Transition Table has definition of 6 state
// and system can be changed adjacent state.
// for example,
//    {    SYS_L2,        means that current definition is from System Level 2.
//        50, SYS_L3,    means that if system idle rate reaches over 50%(idle), system level will be changed to level 3
//        10, SYS_L1,    means that if system idle rate is downto 10%(hardworking), system level will be changed to level 1
//        3, 1, 8    }    means current system level's(Lv 2) system clock configuration
//-----------------------------------------------------------------------------------
// Current System Level,
//                Shift Down Idle Rate,
//                    Shift Down Level,
//                            Shift Up Idle Rate,
//                                Shift Up Level,
//                                        ARMCLK Divider,
//                                            HCLKx2 Divider,
//                                                PCLK Divider
//                                                    MFCCLK Divider (MFCCLK = HCLKx2/Div)
//                                                            VddARM(mV)
//                                                                VddInternal(mV)
//-----------------------------------------------------------------------------------
// OEM can change this table as own device attributes.
// Each Idle Rate to get in other level must be optimized by OEMs
// g_aTransitionTable[x][1], g_aTransitionTable[3]
// Voltage is not yet confirmed on beta release, Refer to Final Chip manual.
DWORD g_aTransitionTable[SYS_LEVEL_MAX][11] =
{
#if (SYNCMODE)

#if (TARGET_ARM_CLK == CLK_532MHz)// || (TARGET_ARM_CLK == CLK_266MHz) || (TARGET_ARM_CLK == CLK_133MHz) || (TARGET_ARM_CLK == CLK_66_5MHz)
    {SYS_L0,         50, SYS_L1,         0, SYS_L0,    1,    2,    4,    2,    1100, 1200},    // 532, 133, 66, 133
    {SYS_L1,         40, SYS_L2,        10, SYS_L0,    2,    2,    4,    2,    1050, 1200},    // 266, 133, 66, 133
    {SYS_L2,         40, SYS_L3,        20, SYS_L0,    2,    2,    4,    2,    1050, 1200},    // 266, 133, 66, 133
    {SYS_L3,        100, SYS_L4,        20, SYS_L0,    4,    2,    4,    2,    1050, 1200},    // 133, 133, 66, 133
    {SYS_L4,         60, SYS_L5,        20, SYS_L0,    4,    4,    2,    1,    1050, 1000},    // 133,  66, 66, 133
    {SYS_L5,        100, SYS_L5,        20, SYS_L0,    8,    4,    2,    1,    1050, 1000},    //  66,  66, 66, 133

#elif (TARGET_ARM_CLK == CLK_666MHz)
    {SYS_L0,         50, SYS_L1,         0, SYS_L0,    2,    5,    4,    2,    1200, 1300},    // 666, 133, 66, 133 (Sync)
    {SYS_L1,         40, SYS_L2,        10, SYS_L0,    2,    1,    4,    2,    1100, 1200},    // 333, 133, 66, 133 (Async)
    {SYS_L2,         40, SYS_L3,        20, SYS_L0,    3,    1,    4,    2,    1050, 1200},    // 222, 133, 66, 133 (Async)
    {SYS_L3,        100, SYS_L4,        20, SYS_L0,    5,    1,    4,    2,    1050, 1200},    // 133, 133, 66, 133 (Async)
    {SYS_L4,         60, SYS_L5,        20, SYS_L0,    5,    2,    2,    1,    1050, 1000},    // 133,  66, 66, 133 (Async)
    {SYS_L5,        100, SYS_L5,        20, SYS_L0,   10,    2,    2,    1,    1050, 1000},    //  66,  66, 66, 133 (Async)

#elif (TARGET_ARM_CLK == CLK_800MHz)
    {SYS_L0,         50, SYS_L1,         0, SYS_L0,    1,    3,    4,    2,    1300, 1200},    // 800, 133, 66, 133
    {SYS_L1,         40, SYS_L2,        10, SYS_L0,    2,    3,    4,    2,    1100, 1200},    // 400, 133, 66, 133
    {SYS_L2,         40, SYS_L3,        20, SYS_L0,    3,    3,    4,    2,    1050, 1200},    // 266, 133, 66, 133
    {SYS_L3,        100, SYS_L4,        20, SYS_L0,    6,    3,    4,    2,    1050, 1200},    // 133, 133, 66, 133
    {SYS_L4,         60, SYS_L5,        20, SYS_L0,    6,    6,    2,    1,    1050, 1000},    // 133,  66, 66, 133
    {SYS_L5,        100, SYS_L5,        20, SYS_L0,   12,    6,    2,    1,    1050, 1000},    //  66,  66, 66, 133

#else
#error ARMCLK_UNDEFINED_ERROR_ON_SYNCMODE
#endif

#else   // Aynchronous bus clock, This is not yet confirmed. Do not use DVS on Asynchronous bus clock !!

#if    (TARGET_ARM_CLK == CLK_532MHz)
    {SYS_L0,         50, SYS_L1,         0, SYS_L0,    1,    1,    8,    2,    1200, 1200},    // 532, 133, 66.5, 133
    {SYS_L1,         40, SYS_L2,        10, SYS_L0,    2,    1,    8,    2,    1100, 1200},    // 266, 133, 66.5, 133
    {SYS_L2,         40, SYS_L3,        10, SYS_L1,    3,    1,    8,    2,    1100, 1200},    // 177, 133, 66.5, 133
    {SYS_L3,         50, SYS_L4,        20, SYS_L2,    4,    1,    8,    2,    1000, 1000},    // 133, 133, 66.5, 133
    {SYS_L4,         60, SYS_L5,        20, SYS_L2,    4,    2,    4,    1,    1000, 1000},    // 133, 66.5, 66.5, 133
    {SYS_L5,        100, SYS_L5,        20, SYS_L2,    8,    2,    4,    1,    1000, 1000},    // 66.5, 66.5, 33.25, 133

#elif    (TARGET_ARM_CLK == CLK_666MHz)
    {SYS_L0,         50, SYS_L1,         0, SYS_L0,    1,    1,    4,    2,    1200, 1300},    // 666, 133, 66, 133
    {SYS_L1,         40, SYS_L2,        10, SYS_L0,    2,    1,    4,    2,    1100, 1200},    // 333, 133, 66, 133
    {SYS_L2,         40, SYS_L3,        20, SYS_L0,    3,    1,    4,    2,    1050, 1200},    // 222, 133, 66, 133
    {SYS_L3,        100, SYS_L4,        20, SYS_L0,    5,    1,    4,    2,    1050, 1200},    // 133, 133, 66, 133
    {SYS_L4,         60, SYS_L5,        20, SYS_L0,    5,    2,    2,    1,    1050, 1000},    // 133,  66, 66, 133
    {SYS_L5,        100, SYS_L5,        20, SYS_L0,   10,    2,    2,    1,    1050, 1000},    //  66,  66, 66, 133

#elif    (TARGET_ARM_CLK == CLK_800MHz)
    {SYS_L0,         50, SYS_L1,         0, SYS_L0,    1,    1,    8,    2,    1300, 975},    // 634, 133, 33, 133
    {SYS_L1,         40, SYS_L2,        10, SYS_L0,    2,    1,    8,    2,     975, 975},    // 317, 133, 33, 133
    {SYS_L2,         40, SYS_L3,        10, SYS_L1,    3,    1,    8,    2,     900, 975},    // 211, 133, 33, 133
    {SYS_L3,         50, SYS_L4,        20, SYS_L2,    4,    1,    8,    2,     900, 975},    // 158, 133, 33, 133
    {SYS_L4,         60, SYS_L5,        20, SYS_L2,    4,    2,    4,    1,     900, 900},    // 158, 66, 33, 133
    {SYS_L5,        100, SYS_L5,        20, SYS_L2,    8,    2,    4,    1,     900, 900},    // 79, 66, 33, 133

#else
#error ARMCLK_UNDEFINED_ERROR_ON_ASYNCMODE
#endif
#endif
};

// Transition Table Data field macro
// SYSTEM_ACTIVE_LEVEL is defined in pmplatform.h
#define ShiftDownRate       (g_aTransitionTable[g_CurrentLevel][1])
#define ShiftDownLevel      ((SYSTEM_ACTIVE_LEVEL)(g_aTransitionTable[g_CurrentLevel][2]))
#define ShiftUpRate         (g_aTransitionTable[g_CurrentLevel][3])
#define ShiftUpLevel        ((SYSTEM_ACTIVE_LEVEL)(g_aTransitionTable[g_CurrentLevel][4]))
#define SysARMCLKDiv(x)     (g_aTransitionTable[x][5])
#define SysHCLKx2Div(x)     (g_aTransitionTable[x][6])
#define SysPCLKDiv(x)       (g_aTransitionTable[x][7])
#define SysMFCLKDiv(x)      (g_aTransitionTable[x][8])
#define VoltageARM(x)       (g_aTransitionTable[x][9])
#define VoltageInternal(x)  (g_aTransitionTable[x][10])

#endif _DVS__H_
