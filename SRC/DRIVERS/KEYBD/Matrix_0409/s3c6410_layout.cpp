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

#include    <windows.h>
#include    <keybddr.h>
#include    <laymgr.h>
#include    <devicelayout.h>
#include    <keybddbg.h>

#include    "winuserm.h"

#include    "KeyMatrix.hpp"

#define    VK_MATRIX_FN    0xC1

#if (MATRIX_LAYOUT == LAYOUT0)
#define    ScanCodeTableFirst    0x00
#define    ScanCodeTableLast    0x3F

UINT8 ScanCodeToVKeyTable[] =
{
    VK_ESCAPE,            // scan code 0,     ESC
    'A',                 // scan code 1,     
    'B',                 // scan code 2,
    'C',                // scan code 3
    'D',                // scan code 4
    VK_T1,                // scan code 5
    VK_T2,                // scan code 6
    VK_T3,                // scan code 7

    VK_RETURN,            // scan code 8,        Enter
    'E',                // scan code 9
    'F',                // scan code 10
    'G',                // scan code 11
    'H',                // scan code 12
    VK_T4,                // scan code 13
    VK_T5,                // scan code 14
    VK_T6,                // scan code 15

    VK_TAB,                // scan code 16,    TAB
    'I',                // scan code 17,
    'J',                // scan code 18,
    'K',                // scan code 19,
    'L',                // scan code 20,
    VK_T7,                 // scan code 21,
    VK_T8,                // scan code 22,
    VK_T9,                // scan code 23,

    VK_SHIFT,            // scan code 24,    Shift 
    'M',                // scan code 25,
    'N',                // scan code 26,
    'O',                // scan code 27,
    'P',                // scan code 28,
    VK_MULTIPLY,        // scan code 29,
    VK_T0,                // scan code 30,    
    VK_PERIOD,            // scan code 31,

    VK_CONTROL,            // scan code 32,    CTRL
    'Q',                 // scan code 33,    
    'R',                // scan code 34,    
    'S',                // scan code 35,    
    'T',                // scan code 36,    
    VK_HYPHEN,            // scan code 37,
    VK_APOSTROPHE,        // scan code 38,    
    VK_COMMA,            // scan code 39,    

    VK_MENU,            // scan code 40,    ALT
    'U',                // scan code 41,    
    'V',                // scan code 42,
    'W',                // scan code 43,
    'X',                // scan code 44,
    VK_SLASH,            // scan code 45,     
    VK_SEMICOLON,        // scan code 46,        
    VK_CAPITAL,            // scan code 47,        

    VK_HANGUL,            // scan code 48,    Korean/English
    'Y',                // scan code 49,    
    'Z',                // scan code 50,
    VK_SUBTRACT,        // scan code 51,    -
    VK_EQUAL,            // scan code 52,    =
    VK_LBRACKET,        // scan code 53,    [        
    VK_UP,                // scan code 54,    
    VK_RBRACKET,        // scan code 55,     ]
    
    VK_LWIN,            // scan code 56,     Windows launcher
    VK_DELETE,            // scan code 57,     
    VK_SPACE,            // scan code 58,    
    VK_BACK,            // scan code 59,    
    VK_BACKSLASH,        // scan code 60,
    VK_LEFT,            // scan code 61,
    VK_DOWN,            // scan code 62,
    VK_RIGHT,            // scan code 63,
};
#elif (MATRIX_LAYOUT == LAYOUT1)
#define ScanCodeTableFirst  0x00
#define ScanCodeTableLast   0x09

UINT8 ScanCodeToVKeyTable[] =
{ 
    VK_UP,                // scan code 0
    VK_LWIN,            // scan code 1 
    VK_RETURN,             // scan code 2
    VK_CONTROL,            // scan code 3
    VK_DOWN,            // scan code 4
    VK_DELETE,            // scan code 5
    VK_RIGHT,            // scan code 6
    VK_ESCAPE,            // scan code 7

    VK_LEFT,            // scan code 8
    VK_MENU,            // scan code 9
};
#elif (MATRIX_LAYOUT == LAYOUT2)
#define    ScanCodeTableFirst    0x00
#define    ScanCodeTableLast    0x3F

UINT8 ScanCodeToVKeyTable[] =
{
    0,                    // scan code 0,
    0,                     // scan code 1,
    VK_T1,                 // scan code 2,
    'Q',                // scan code 3
    'A',                // scan code 4
    VK_CONTROL,            // scan code 5
    VK_TACTION,            // scan code 6
    VK_LEFT,            // scan code 7

    0,                    // scan code 8,
    0,                    // scan code 9
    VK_T2,                // scan code 10
    'W',                // scan code 11
    'S',                // scan code 12
    'Z',                // scan code 13
    VK_RIGHT,            // scan code 14
    VK_F2,                // scan code 15

    0,                    // scan code 16,
    0,                    // scan code 17,
    VK_T3,                // scan code 18,
    'E',                // scan code 19,
    'D',                // scan code 20,
    'X',                 // scan code 21,
    VK_DELETE,            // scan code 22,
    VK_UP,                // scan code 23,

    0,                    // scan code 24,
    0,                    // scan code 25,
    VK_T4,                // scan code 26,
    'R',                // scan code 27,
    'F',                // scan code 28,
    'C',                // scan code 29,
    VK_CAPITAL,            // scan code 30,    
    VK_F1,                // scan code 31,

    0,                    // scan code 32,
    'O',                 // scan code 33,    
    VK_T5,                // scan code 34,    
    'T',                // scan code 35,    
    'G',                // scan code 36,    
    'V',                // scan code 37,
    VK_DOWN,            // scan code 38,    
    VK_BACK,            // scan code 39,    

    'P',                // scan code 40,
    VK_T0,                // scan code 41,
    VK_T6,                // scan code 42,
    'Y',                // scan code 43,
    'H',                // scan code 44,
    VK_SPACE,            // scan code 45,
    VK_LWIN,            // scan code 46,
    VK_MULTIPLY,        // scan code 47,

    'M',                // scan code 48,
    'L',                // scan code 49,
    VK_T7,                // scan code 50,
    'U',                // scan code 51,
    'J',                // scan code 52,
    'N',                // scan code 53,        
    VK_ESCAPE,            // scan code 54,
    VK_RETURN,            // scan code 55,
    
    VK_SHIFT,            // scan code 56,
    VK_T9,                // scan code 57,
    VK_T8,                // scan code 58,
    'I',                // scan code 59,
    'K',                // scan code 60,
    'B',                // scan code 61,
    VK_TAB,                // scan code 62,
    VK_MENU,            // scan code 63,
};
#endif

static ScanCodeToVKeyData scvkEngUS = 
{
    0,
    ScanCodeTableFirst,
    ScanCodeTableLast,
    ScanCodeToVKeyTable
};

#if (MATRIX_LAYOUT == LAYOUT0)
#define ScanCodeTableExtFirst  0xE000
#define ScanCodeTableExtLast   0xE03F

UINT8 ScanCodeToVKeyExtTable[] =
{
       0,
    VK_T1,
       VK_T2,
    VK_T3,
    0,
    0,
    0,
    0,

    0,
    VK_T4,
    VK_T5,
    VK_T6,
    0,
    0,
    0,
    0,

    0,
    VK_T7,
    VK_T8,
    VK_T9,
    0,
    0,
    0,
    0,

    0,
    VK_TTALK,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    VK_TEND,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    VK_TVOLUMEUP,
    VK_TVOLUMEDOWN,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
#elif (MATRIX_LAYOUT == LAYOUT1)
#define ScanCodeTableExtFirst  0xE000
#define ScanCodeTableExtLast   0xE009

UINT8 ScanCodeToVKeyExtTable[] =
{ 
    VK_T1,
    VK_T2,
    VK_T3,
    VK_T4,
    VK_T5,
    VK_T6,
    VK_TVOLUMEUP,
    VK_TVOLUMEDOWN,

    0,
    0,
};

#elif (MATRIX_LAYOUT == LAYOUT2)
#define ScanCodeTableExtFirst  0xE000
#define ScanCodeTableExtLast   0xE03F

UINT8 ScanCodeToVKeyExtTable[] =
{
       0,
    0,
       0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
#endif

static ScanCodeToVKeyData scvkEngExtUS = 
{  
    0xe000,
    ScanCodeTableExtFirst,
    ScanCodeTableExtLast,
    ScanCodeToVKeyExtTable
};

static ScanCodeToVKeyData *rgscvkMatrixEngUSTables[] =
{
    &scvkEngUS,
    &scvkEngExtUS
};


struct VirtualKeyMapping
{
    UINT32    uiVk;
    UINT32    uiVkGenerated;
};

static const VirtualKeyMapping g_rgvkMapFn[] =
{
    {  '1', VK_F1 },
    {  '2', VK_F2 },
    {  '3', VK_F3 },
    {  '4', VK_F4 },
    {  '5', VK_F5 },
    {  '6', VK_F6 },
    {  '7', VK_F7 },
    {  '8', VK_F8 },
    {  '9', VK_F9 },
    {  '0', VK_F10 },
    { VK_HYPHEN, VK_NUMLOCK },
    { VK_EQUAL, VK_CANCEL },
    {  'P', VK_INSERT },
    { VK_LBRACKET, VK_PAUSE },
    { VK_RBRACKET, VK_SCROLL },
    { VK_SEMICOLON, VK_SNAPSHOT },
    { VK_APOSTROPHE, VK_SNAPSHOT },
    {  VK_LEFT, VK_HOME },
    {  VK_UP, VK_PRIOR},
    {  VK_DOWN, VK_NEXT },
    {  VK_RIGHT, VK_END },
};

static const VirtualKeyMapping g_rgvkMapNumLock[] =
{
    {  '7', VK_NUMPAD7 },
    {  '8', VK_NUMPAD8 },
    {  '9', VK_NUMPAD9 },
    {  '0', VK_MULTIPLY },
    {  'U', VK_NUMPAD4 },
    {  'I', VK_NUMPAD5 },
    {  'O', VK_NUMPAD6 },
    {  'P', VK_SUBTRACT },
    {  'J', VK_NUMPAD1 },
    {  'K', VK_NUMPAD2 },
    {  'L', VK_NUMPAD3 },
    {  VK_SEMICOLON, VK_ADD },
    {  'M', VK_NUMPAD0 },
    {  VK_PERIOD, VK_DECIMAL },
    {  VK_SLASH, VK_DIVIDE },
};


// Find a virtual key mapping in the given array.
static
const VirtualKeyMapping * 
FindRemappedKey(
    UINT32 uiVk,
    const VirtualKeyMapping *pvkMap,
    DWORD cvkMap
    )
{
    const VirtualKeyMapping *pvkMapMatch = NULL;
    UINT ui;

    if(pvkMap == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (_T("FindRemappedKey: pvkMap error\r\n")));
        return NULL;
    }

    for (ui = 0; ui < cvkMap; ++ui)
    {
        if (pvkMap[ui].uiVk == uiVk)
        {
            pvkMapMatch = &pvkMap[ui];
            break;
        }
    }

    return pvkMapMatch;
}


#define IS_NUMLOCK_ON(ksf) (ksf & KeyShiftNumLockFlag)

// Remapping function for the matrix keyboard
static
UINT
WINAPI
MatrixUsRemapVKey(
    const KEYBD_EVENT *pKbdEvents,
    UINT               cKbdEvents,
    KEYBD_EVENT       *pRmpKbdEvents,
    UINT               cMaxRmpKbdEvents
    )
{
    SETFNAME(_T("MatrixUsRemapVKey"));

    static BOOL fFnDown = FALSE;

    UINT cRmpKbdEvents = 0;
    UINT ui;

    if (pRmpKbdEvents == NULL)
    {
        // 1 to 1 mapping
        if (cMaxRmpKbdEvents != 0)
        {
            DEBUGMSG(ZONE_ERROR, (_T("%s: cMaxRmpKbdEvents error!\r\n"), pszFname));
        }
        return cKbdEvents;
    }

    if (pKbdEvents == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s: pKbdEvents error!\r\n"), pszFname));
    }

    if (cMaxRmpKbdEvents < cKbdEvents)
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Buffer is not large enough!\r\n"), pszFname));
        return 0;
    }

    for (ui = 0; ui < cKbdEvents; ++ui)
    {
        const KEYBD_EVENT *pKbdEventCurr = &pKbdEvents[ui];
        KEYBD_EVENT *pKbdEventRmpCurr = &pRmpKbdEvents[cRmpKbdEvents];

        // Copy the input key event to our remapped list
        pKbdEventRmpCurr->uiVk = pKbdEventCurr->uiVk;
        pKbdEventRmpCurr->uiSc = pKbdEventCurr->uiSc;
        pKbdEventRmpCurr->KeyStateFlags = pKbdEventCurr->KeyStateFlags;

        const VirtualKeyMapping *pvkMap = NULL;
        BOOL fKeyDown = (pKbdEventCurr->KeyStateFlags & KeyStateDownFlag) != 0;
        UINT32 uiVkCurr = pKbdEventCurr->uiVk;

        if (uiVkCurr == VK_MATRIX_FN)
        {
            fFnDown = fKeyDown;
            // Fn virtual key does not get sent to the system so
            // do not increment cRmpKbdEvents.
            DEBUGMSG(ZONE_DEVICELAYOUT, (_T("%s: Fn key is now %s\r\n"),
                pszFname, (fFnDown ? _T("DOWN") : _T("UP"))));
        }
        else
        {
            // We have one key event
            ++cRmpKbdEvents;

            if (fKeyDown)
            {
                // Handle key down
                if (fFnDown)
                {
                    // Fn key is on
                    if (IS_NUMLOCK_ON(pKbdEventCurr->KeyStateFlags))
                    {
                        pvkMap = FindRemappedKey(uiVkCurr, g_rgvkMapNumLock, dim(g_rgvkMapNumLock));
                    }

                    if (pvkMap == NULL)
                    {
                        // NumLock did not effect this key. See if the
                        // Fn key by itself does.                        
                        pvkMap = FindRemappedKey(uiVkCurr, g_rgvkMapFn, dim(g_rgvkMapFn));
                    }
                }
            }
            else
            {
                // Handle key up
                if (fFnDown)
                {
                    // Fn key is on
                    if (IS_NUMLOCK_ON(pKbdEventCurr->KeyStateFlags))
                    {
                        pvkMap = FindRemappedKey(uiVkCurr, g_rgvkMapNumLock, dim(g_rgvkMapNumLock));
                    }

                    if (pvkMap == NULL)
                    {
                        // NumLock did not effect this key. See if the
                        // Fn key by itself does.                        
                        pvkMap = FindRemappedKey(uiVkCurr, g_rgvkMapFn, dim(g_rgvkMapFn));
                    }
                }
            }

            if (pvkMap != NULL)
            {
                // This combination generates a different virtual key
                if (pvkMap->uiVkGenerated == NULL)
                {
                    DEBUGMSG(ZONE_ERROR, (_T("%s: pvkMap->uiVkGenerated error!\r\n"), pszFname));
                }
                pKbdEventRmpCurr->uiVk = pvkMap->uiVkGenerated;
            }
        }
    }

    return cRmpKbdEvents;    
}
    

static DEVICE_LAYOUT dlMatrixEngUs =
{
    sizeof(DEVICE_LAYOUT),
    MATRIX_PDD,
    rgscvkMatrixEngUSTables,
    dim(rgscvkMatrixEngUSTables),
    MatrixUsRemapVKey,
};

extern "C"
BOOL
Matrix(
    PDEVICE_LAYOUT pDeviceLayout
    )
{
    SETFNAME(_T("Matrix"));

    BOOL fRet = FALSE;

    if (pDeviceLayout == NULL)
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s: pDeviceLayout error!\r\n"), pszFname));
        return FALSE;
    }

    if (pDeviceLayout->dwSize != sizeof(DEVICE_LAYOUT))
    {
        DEBUGMSG(ZONE_ERROR, (_T("Matrix: data structure size mismatch\r\n")));
        goto leave;
    }

    // Make sure that the Sc->Vk tables are the sizes that we expect
    if (dim(ScanCodeToVKeyTable) != (1 + ScanCodeTableLast - ScanCodeTableFirst))
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s: ScanCodeToVKeyTable error!\r\n"), pszFname));
    }

    *pDeviceLayout = dlMatrixEngUs;

    fRet = TRUE;

leave:
    return fRet;
}

#ifdef DEBUG
// Verify function declaration against the typedef.
static PFN_DEVICE_LAYOUT_ENTRY v_pfnDeviceLayout = Matrix;
#endif

