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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2008. Samsung Electronics, co. ltd  All rights reserved.

Module Name:  

Abstract:

    This file implements the S3C6410 Keyboard function

Notes: 
--*/

#ifndef __KEYMATRIX_HPP_INCLUDED__
#define __KEYMATRIX_HPP_INCLUDED__

#include <windows.h>
#include <bsp_cfg.h>

#define MATRIX_PDD 0x08

#ifndef FIN
#define FIN        12000000
#endif

class KeyMatrix
{
public:
    BOOL IsrThreadStart(void);
    BOOL IsrThreadProc(void);
    BOOL KeybdPowerOff(void);
    BOOL KeybdPowerOn(void);

   friend void KeybdPdd_PowerHandler(BOOL bOff);
   friend int WINAPI KeybdPdd_GetEventEx( UINT32 VKeyBuf[16], UINT32 ScanCodeBuf[16], KEY_STATE_FLAGS    KeyStateFlagsBuf[16]);
   friend void WINAPI KeybdPdd_ToggleKeyNotification( KEY_STATE_FLAGS KeyStateFlags);
};

/////////////////////////////////////////////////////////////////////////////////////////
//// S3C6410 KEYIF Register Bit Definition

// KEYIFCON
#define    FC_EN_DIS            (0<<3)
#define    FC_EN_EN             (1<<3)
#define    DF_EN_DIS            (0<<2)
#define    DF_EN_EN             (1<<2)
#define    INT_R_ENABLE         (0<<1)
#define    INT_R_DISABLE        (1<<1)
#define    INT_F_ENABLE         (0<<0)
#define    INT_F_DISABLE        (1<<0)

// KEYIFSTSCLR
#define    CLEAR_P_INT          (0xFF<<0)
#define    CLEAR_R_INT          (0xFF<<8)

// KEYIFCOL
#define    KEYIFCOLEN_ALL_EN    (0x0<<8)
#define    KEYIFCOLEN_ALL_DIS   (0xFF<<8)

// KEYIFFC
#define    FC_DIV_VAL(n)        (((n)&0x3ff)<<0)

#endif

