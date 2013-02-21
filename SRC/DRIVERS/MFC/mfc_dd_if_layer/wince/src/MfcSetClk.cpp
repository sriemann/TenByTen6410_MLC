//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include <windows.h>
#include <s3c6410.h>


static BOOL mfc_set_MFC_CLKDIV0(int mfc_ratio)
{
    volatile S3C6410_SYSCON_REG * pSysConReg = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
    pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (pSysConReg == NULL)
    {
        RETAILMSG(1, (L"[MFC Driver] MFC_Init() : g_pSysConReg MmMapIoSpace() Failed\n\r"));
        return FALSE;
    }

    pSysConReg->CLK_DIV0 = (pSysConReg->CLK_DIV0 & ~(0xF << 28)) | (mfc_ratio << 28);

    MmUnmapIoSpace((PVOID)pSysConReg, sizeof(S3C6410_SYSCON_REG));
    pSysConReg = NULL;


    return TRUE;
}

BOOL Mfc_Set_ClkDiv(int divider)
{
    if ((divider < 1) || (divider > 16)) {
        RETAILMSG(1, (L"[[MFC Driver] Mfc_Set_ClkDiv() : MFC clock divider must be 1 ~ 16.\n\r"));
        return FALSE;
    }


    mfc_set_MFC_CLKDIV0(divider - 1);

    return TRUE;
}

