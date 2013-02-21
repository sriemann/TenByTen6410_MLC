//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

   s3c6410_button.c   

Abstract:

   Low Level HW Button control Library, this will contorl HW register with External Interrupt and GPIOs

Functions:

    

Notes:

--*/

#include "precomp.h"

static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;

BOOL Button_initialize_register_address(void *pGPIOReg)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN]++%s(0x%08x)\n\r"), _T(__FUNCTION__), pGPIOReg));

    if (pGPIOReg == NULL)
    {
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : NULL pointer parameter\n\r"), _T(__FUNCTION__)));
        return FALSE;
    }
    else
    {
        g_pGPIOReg = (S3C6410_GPIO_REG *)pGPIOReg;
        RETAILMSG(PWR_ZONE_TEMP, (_T("[BTN:INF] g_pGPIOReg    = 0x%08x\n\r"), g_pGPIOReg));
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN]--%s()\n\r"), _T(__FUNCTION__)));

    return TRUE;
}

// In SMDK6410 Eval. Board, Button is mapped to this GPIOs
// Reset Button -> GPN[11] : EINT11
// Power Button -> GPN[9] : EINT9 / ADDR_CF[1]
//
void Button_pwrbtn_port_initialize(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));
    
    // GPN[0] to EINT0
    SET_GPIO(g_pGPIOReg, GPNCON, 0, GPNCON_EXTINT);
    // GPN[0] set Pull-up Enable
    SET_GPIO(g_pGPIOReg, GPNPUD, 0, GPNPUD_PULLUP);
}

BOOL Button_pwrbtn_set_interrupt_method(EINT_SIGNAL_METHOD eMethod)
{
    BOOL bRet = TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s(%d)\n\r"), _T(__FUNCTION__), eMethod));

    switch(eMethod)
    {
    case EINT_SIGNAL_LOW_LEVEL:
    case EINT_SIGNAL_HIGH_LEVEL:        
    case EINT_SIGNAL_FALL_EDGE:        
    case EINT_SIGNAL_RISE_EDGE:        
    case EINT_SIGNAL_BOTH_EDGE:        
        g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT0)) | (eMethod<<EINT0CON_EINT0);        
        break;
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

BOOL Button_pwrbtn_set_filter_method(EINT_FILTER_METHOD eMethod, unsigned int uiFilterWidth)
{
    BOOL bRet =TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s(%d, %d)\n\r"), _T(__FUNCTION__), eMethod, uiFilterWidth));

    switch(eMethod)
    {
    case EINT_FILTER_DISABLE:
        g_pGPIOReg->EINT0FLTCON1 &= ~(0x1<<FLTSEL_0);
        break;
    case EINT_FILTER_DELAY:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(0x1<<FLTSEL_0)) | (0x1<<FLTEN_0);
        break;
    case EINT_FILTER_DIGITAL:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(EINT0FILTERCON_MASK<<FLTWIDTH_0))
                                    | ((0x1<<FLTSEL_0) | (0x1<<FLTEN_0)
                                    | (uiFilterWidth & (EINT0FILTER_WIDTH_MASK<<FLTWIDTH_0)));
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

void Button_pwrbtn_enable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

    g_pGPIOReg->EINT0MASK &= ~(0x1<<0);    // Unmask EINT0
}

void Button_pwrbtn_disable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

    g_pGPIOReg->EINT0MASK |= (0x1<<0);    // Mask EINT0
}

void Button_pwrbtn_clear_interrupt_pending(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

    g_pGPIOReg->EINT0PEND = (0x1<<0);        // Clear pending EINT0
}

BOOL Button_pwrbtn_is_pushed(void)
{
    RETAILMSG(PWR_ZONE_ENTER,(_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));

    if (g_pGPIOReg->GPNDAT & (0x1<<0))        // We can read GPDAT pin level when configured as EINT
    {
        return FALSE;    // Low Active Switch (Pull-up switch)
    }
    else
    {
        return TRUE;
    }
}


void Button_rstbtn_port_initialize(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s()\n\r"), _T(__FUNCTION__)));
    
    // GPN[1] to EINT1
    SET_GPIO(g_pGPIOReg, GPNCON, 1, GPNCON_EXTINT);
    // GPN[1] set Pull-up Enable
    SET_GPIO(g_pGPIOReg, GPNPUD, 1, GPNPUD_PULLUP);
}


BOOL Button_rstbtn_set_interrupt_method(EINT_SIGNAL_METHOD eMethod)
{
    BOOL bRet =TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] %s(%d)\n\r"), _T(__FUNCTION__), eMethod));

    switch(eMethod)
    {
    case EINT_SIGNAL_LOW_LEVEL:
    case EINT_SIGNAL_HIGH_LEVEL:
    case EINT_SIGNAL_FALL_EDGE:
    case EINT_SIGNAL_RISE_EDGE:
    case EINT_SIGNAL_BOTH_EDGE:
        g_pGPIOReg->EINT0CON0 = (g_pGPIOReg->EINT0CON0 & ~(EINT0CON0_BITMASK<<EINT0CON_EINT1)) | (eMethod<<EINT0CON_EINT1);
        break;
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

BOOL Button_rstbtn_set_filter_method(EINT_FILTER_METHOD eMethod, unsigned int uiFilterWidth)
{
    BOOL bRet =TRUE;

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s(%d, %d)\n\r"), _T(__FUNCTION__), eMethod, uiFilterWidth));

    switch(eMethod)
    {
    case EINT_FILTER_DISABLE:
        g_pGPIOReg->EINT0FLTCON1 &= ~(0x1<<FLTSEL_1);
        break;
    case EINT_FILTER_DELAY:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(0x1<<FLTSEL_1)) | (0x1<<FLTEN_1);
        break;
    case EINT_FILTER_DIGITAL:
        g_pGPIOReg->EINT0FLTCON1 = (g_pGPIOReg->EINT0FLTCON1 & ~(EINT0FILTERCON_MASK<<FLTWIDTH_1))
                                    | ((0x1<<FLTSEL_1) | (0x1<<FLTEN_1)
                                    | (uiFilterWidth & (EINT0FILTER_WIDTH_MASK<<FLTWIDTH_1)));
        break;
    default:
        RETAILMSG(PWR_ZONE_ERROR, (_T("[BTN:ERR] %s() : Unknown Method = %d\n\r"), _T(__FUNCTION__), eMethod));
        bRet = FALSE;
        break;
    }

    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] --%s() = %d\n\r"), _T(__FUNCTION__), bRet));

    return bRet;
}

void Button_rstbtn_enable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

    g_pGPIOReg->EINT0MASK &= ~(0x1<<1);    // Unmask EINT1
}

void Button_rstbtn_disable_interrupt(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

    g_pGPIOReg->EINT0MASK |= (0x1<<1);        // Mask EINT1
}

void Button_rstbtn_clear_interrupt_pending(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

    g_pGPIOReg->EINT0PEND = (0x1<<1);        // Clear pending EINT1
}

BOOL Button_rstbtn_is_pushed(void)
{
    RETAILMSG(PWR_ZONE_ENTER, (_T("[BTN] ++%s()\n\r"), _T(__FUNCTION__)));

    if (GET_GPIO(g_pGPIOReg, GPNDAT, 1))        // We can read GPDAT pin level when configured as EINT
    {
        return FALSE;    // Low Active Switch (Pull-up switch)
    }
    else
    {
        return TRUE;
    }
}

