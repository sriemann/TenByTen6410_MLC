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
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

   s3c6410_power_control.c   Power Controller Library

Abstract:

   Low Level HW Block Power control Librrary

Functions:

    

Notes:

--*/

#include "precomp.h"

static volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;

/**
*    @fn    PwrCon_initialize_register_address(void *pSysConReg)
*    @param pSysConReg    System Controller Register Block's Start Address(Virtual)
*    @note  This funciton just save System Controller Register Block's Start address into global variable
*/
BOOL PwrCon_initialize_register_address(void *pSysConReg)
{
    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]++%s(0x%08x)\n"), _T(__FUNCTION__), pSysConReg));

    if (pSysConReg == NULL)
    {
        RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] PwrCon_initialize_register_address() : NULL pointer parameter\n")));
        return FALSE;
    }
    else
    {
        g_pSysConReg = (S3C6410_SYSCON_REG *)pSysConReg;
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] g_pSysConReg    = 0x%08x\n"), g_pSysConReg));
    }

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]--%s()\n"), _T(__FUNCTION__)));

    return TRUE;
}

/**
*    @fn    PwrCon_set_block_power_on(BLKPWR_DOMAIN eDomain)
*    @param eDomain    The IP Block number in NORMAL_CFG Register to turn on
*    @note  This funciton turn on specific IP Block's power
*           when the system is in NORMAL Mode, not in other Power mode
*/
BOOL PwrCon_set_block_power_on(BLKPWR_DOMAIN eDomain)
{
    UINT32 TimeOut;

    assert(g_pSysConReg);

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]++%s(%d)\n"), _T(__FUNCTION__), eDomain));
    
    TimeOut = 10000000L;   // This is big value.

    switch(eDomain)
    {
    case BLKPWR_DOMAIN_IROM:    // Internal ROM
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_IROM);
        break;
    case BLKPWR_DOMAIN_G:       // 3D
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_DOMAIN_G);
        while(!(GETBIT32(g_pSysConReg->BLK_PWR_STAT, BIT_BLK_G)) || (TimeOut-- == 0));
        if(TimeOut == 0) goto Block_Pwr_ON_FAIL;
        break;
    case BLKPWR_DOMAIN_ETM:    // ETM
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_DOMAIN_ETM);
        while(!(GETBIT32(g_pSysConReg->BLK_PWR_STAT, BIT_BLK_ETM)) || (TimeOut-- == 0));
        if(TimeOut == 0) goto Block_Pwr_ON_FAIL;
        break;
    case BLKPWR_DOMAIN_S:        // SDMA, Security System
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_DOMAIN_S);
        while(!(GETBIT32(g_pSysConReg->BLK_PWR_STAT, BIT_BLK_S)) || (TimeOut-- == 0));
        if(TimeOut == 0) goto Block_Pwr_ON_FAIL;
        break;
    case BLKPWR_DOMAIN_F:        // Display Controller, Post Processor, Rotator
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_DOMAIN_F);
        while(!(GETBIT32(g_pSysConReg->BLK_PWR_STAT, BIT_BLK_F)) || (TimeOut-- == 0));
        if(TimeOut == 0) goto Block_Pwr_ON_FAIL;
        break;
    case BLKPWR_DOMAIN_P:        // FIMG-2D, TV Encoder, TV Scaler
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_DOMAIN_P);
        while(!(GETBIT32(g_pSysConReg->BLK_PWR_STAT, BIT_BLK_P)) || (TimeOut-- == 0));
        if(TimeOut == 0) goto Block_Pwr_ON_FAIL;
        break;
    case BLKPWR_DOMAIN_I:        // Camera interface, Jpeg
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_DOMAIN_I);
        while(!(GETBIT32(g_pSysConReg->BLK_PWR_STAT, BIT_BLK_I)) || (TimeOut-- == 0));
        if(TimeOut == 0) goto Block_Pwr_ON_FAIL;        
        break;
    case BLKPWR_DOMAIN_V:        // MFC
        SETBIT32(g_pSysConReg->NORMAL_CFG, BIT_DOMAIN_V);
        while(!(GETBIT32(g_pSysConReg->BLK_PWR_STAT, BIT_BLK_V)) || (TimeOut-- == 0));
        if(TimeOut == 0) goto Block_Pwr_ON_FAIL;        
        break;
    default:
        RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s() : Unknown Domain = %d\n"), _T(__FUNCTION__), eDomain));
        return FALSE;
    }

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]--%s()\n"), _T(__FUNCTION__)));

    return TRUE;

Block_Pwr_ON_FAIL:
    RETAILMSG(TRUE, (_T("[PWRCON:ERR] %s() : Time OUT!! Domain = %d\n"), _T(__FUNCTION__), eDomain));
    
    return FALSE;
}

/**
*    @fn    PwrCon_set_block_power_off(BLKPWR_DOMAIN eDomain)
*    @param eDomain    The IP Block number in NORMAL_CFG Register to turn off
*    @note  This funciton turn off specific IP Block's power
*           when the system is in NORMAL Mode, not in other Power mode
*/
BOOL PwrCon_set_block_power_off(BLKPWR_DOMAIN eDomain)
{
    assert(g_pSysConReg);
    
    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]++%s(%d)\n"), _T(__FUNCTION__), eDomain));

    switch(eDomain)
    {
    case BLKPWR_DOMAIN_IROM:    // Internal ROM
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_IROM);
        break;
    case BLKPWR_DOMAIN_G:       // 3D 
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_BLK_G);
        break;
    case BLKPWR_DOMAIN_ETM:    // ETM
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_BLK_ETM);
        break;
    case BLKPWR_DOMAIN_S:        // SDMA, Security System
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_BLK_S);
        break;
    case BLKPWR_DOMAIN_F:        // Display Controller, Post Processor, Rotator
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_BLK_F);
        break;
    case BLKPWR_DOMAIN_P:        // FIMG-2D, TV Encoder, TV Scaler
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_BLK_P);
        break;
    case BLKPWR_DOMAIN_I:        // Camera interface, Jpeg
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_BLK_I);
        break;
    case BLKPWR_DOMAIN_V:        // MFC
        CLEARBIT32(g_pSysConReg->NORMAL_CFG, BIT_BLK_V);
        break;
    default:
        RETAILMSG(PWC_ZONE_ERROR, (_T("[PWRCON:ERR] %s() : Unknown Domain = %d\n"), _T(__FUNCTION__), eDomain));
        return FALSE;
    }

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]--%s()\n"), _T(__FUNCTION__)));

    return TRUE;
}

/**
*    @fn    PwrCon_set_USB_phy(BOOL bOn)
*    @param bOn    TRUE(turn on) or FALSE(turn off)
*    @note  This funciton turn on and off USB phy module's clock
*/
void PwrCon_set_USB_phy(BOOL bOn)
{
    assert(g_pSysConReg);
    
    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]++%s(%d)\n"), _T(__FUNCTION__), bOn));

    if (bOn)
    {
        SETBIT32(g_pSysConReg->OTHERS, BIT_USB_SIG_MASK);
    }
    else
    {
        CLEARBIT32(g_pSysConReg->OTHERS, BIT_USB_SIG_MASK);
    }

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON]--%s()\n"), _T(__FUNCTION__)));
}

/**
*    @fn    PwrCon_get_reset_statys(void)
*    @note  This funciton read the reset status which means what trigger reset the system in the last time
*/
RESET_STATUS PwrCon_get_reset_status(void)
{
    RESET_STATUS RstStatus = RST_UNKNOWN;
    
    assert(g_pSysConReg->NORMAL_CFG);    
    
    RstStatus = g_pSysConReg->RST_STAT;
    
    switch(RstStatus)
    {
    case RST_HW_RESET:
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] %s() : H/W Reset [0x%08x]\n"), _T(__FUNCTION__), g_pSysConReg->RST_STAT));
        break;
    case RST_WARM_RESET:
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] %s() : Warm Reset [0x%08x]\n"), _T(__FUNCTION__), g_pSysConReg->RST_STAT));
        break;
    case RST_WDT_RESET:
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] %s() : Watch Dog Timer Reset [0x%08x]\n"), _T(__FUNCTION__), g_pSysConReg->RST_STAT));
        break;
    case RST_SLEEP_WAKEUP:
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] %s() : Sleep/Wakeup Reset [0x%08x]\n"), _T(__FUNCTION__), g_pSysConReg->RST_STAT));
        break;
    case RST_ESLEEP_WAKEUP:
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] %s() : eSleep/Wakeup Reset [0x%08x]\n"), _T(__FUNCTION__), g_pSysConReg->RST_STAT));
        break;
    case RST_SW_RESET:
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:INF] %s() : S/W Reset [0x%08x]\n"), _T(__FUNCTION__), g_pSysConReg->RST_STAT));
        break;
    default:
        RETAILMSG(PWC_ZONE_TEMP, (_T("[PWRCON:ERR] %s() : Unknown RST_STAT [0x%08x]\n"), _T(__FUNCTION__), g_pSysConReg->RST_STAT));
        break;
    }

    RETAILMSG(PWC_ZONE_ENTER, (_T("[PWRCON] %s() = %d\n"), _T(__FUNCTION__), RstStatus));

    return RstStatus;
}

