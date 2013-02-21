//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/



#include <bsp.h>
#include "pmplatform.h"


static BOOL mfc_power_on_off(int on_off)
{
    DWORD    dwIPIndex = PWR_IP_MFC;
    DWORD    dwIOCTL;

    HANDLE   hPwrControl;

    hPwrControl = CreateFile( L"PWC0:",
                              GENERIC_READ|GENERIC_WRITE,
                              FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0, 0);
    if (INVALID_HANDLE_VALUE == hPwrControl )
    {
        RETAILMSG(1, (L"[MFC] PWC0 Open Device Failed.\r\n"));
        return FALSE;
    }

    if (on_off == 0)
        dwIOCTL = IOCTL_PWRCON_SET_POWER_OFF;
    else
        dwIOCTL = IOCTL_PWRCON_SET_POWER_ON;

    if ( !DeviceIoControl(hPwrControl, dwIOCTL, &dwIPIndex, sizeof(DWORD), NULL, 0, NULL, NULL) )
    {
        RETAILMSG(1,(TEXT("[MFC] MFC Power(%d) : MFC Power On/Off Failed\r\n")));
        CloseHandle(hPwrControl);
        return FALSE;
    }

    CloseHandle(hPwrControl);

    return TRUE;
}


static BOOL mfc_clock_on_off(int on_off)
{
    static volatile S3C6410_SYSCON_REG * pSysConReg = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    // SYSCON_REG register address mapping
    //   Once it is mapped, it keeps the mapping until the OS is rebooted.
    if (pSysConReg == NULL) {
        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
        pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
        if (pSysConReg == NULL)
        {
            RETAILMSG(1, (L"[[MFC Driver] MFC_Init() : g_pSysConReg MmMapIoSpace() Failed\n\r"));
            return FALSE;
        }
    }

    if (on_off == 0) {
        pSysConReg->PCLK_GATE &= ~(1<<0);    // MFC
        pSysConReg->SCLK_GATE &= ~(1<<3);    // MFC
    }
    else {
        pSysConReg->HCLK_GATE |= (1<<0);    // MFC
        pSysConReg->PCLK_GATE |= (1<<0);    // MFC
        pSysConReg->SCLK_GATE |= (1<<3);    // MFC
    }

    return TRUE;
}



BOOL Mfc_Pwr_On()
{
    RETAILMSG(0, (L"\n[MFC POWER] MFC_POWER_ON."));

    if (mfc_power_on_off(1) == FALSE)
        return FALSE;

    RETAILMSG(0, (L"\n[MFC POWER] Power is up."));

    return TRUE;
}



BOOL Mfc_Pwr_Off()
{
    RETAILMSG(0, (L"\n[MFC POWER] MFC_POWER_OFF."));

    if (mfc_power_on_off(0) == FALSE)
        return FALSE;

    RETAILMSG(0, (L"\n[MFC POWER] Power is down"));

    return TRUE;
}


BOOL Mfc_Clk_On()
{
    return mfc_clock_on_off(1);
}



BOOL Mfc_Clk_Off()
{
    return mfc_clock_on_off(0);
}

