//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
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

#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <Pmpolicy.h>

#include <keybddbg.h>
#include <keybddr.h>
#include <keybdpdd.h>
#include <keybdist.h>

#include <oal.h>
#include <s3c6410.h>
#include <bsp.h>

#include "keymatrix.hpp"

#define KEY_POWER_ON        (1<<11)                      // PCLKCON
#define FT_CLK_DIV          (FIN/32000 - 1)
#define KCODE_TYPE_NORMAL  0x0001
#define KCODE_TYPE_SL      0x0002
#define DEFAULT_PRIORITY    145
#define SIZE_KEY        SIZE_COLS * SIZE_ROWS
#define CNT_VALIDKEY    1
#define CNT_LONGKEY     30
#define TIME_KEYSCAN    10
#define SCAN_EXT        0xe000

#if ((MATRIX_LAYOUT == LAYOUT0)||(MATRIX_LAYOUT == LAYOUT2))
#define SIZE_BITS   8
#define SIZE_COLS   8
#define SIZE_ROWS   8
#elif (MATRIX_LAYOUT == LAYOUT1)
#define SIZE_BITS   2
#define SIZE_COLS   5
#define SIZE_ROWS   2
#endif

// Pointer to device control registers
volatile S3C6410_GPIO_REG *pGPIOReg = NULL;
volatile S3C6410_KEYPAD_REG *pKeyPadReg = NULL;
volatile S3C6410_SYSCON_REG *pSysConReg = NULL;

DWORD ChangeState[SIZE_COLS];
DWORD KeyState[SIZE_COLS];
DWORD FaultKey;

// There is really only one physical keyboard supported by the system.
KeyMatrix *Keyboard;

typedef enum {
    ENUM_INPUT = 0,
    ENUM_OUTPUT,
    ENUM_AUXFUNC,
    ENUM_RESERVED
} ENUM_GPIO_FUNC;

typedef enum {
    ENUM_ROW,
    ENUM_COL
} ENUM_COL_ROW;

struct KSTATE
{
    WORD Mask;
    WORD Cnt;
};

struct KCODE
{
    DWORD Type;
    DWORD Scan;
    DWORD TimeoutCnt;    // used by KCODE_TYPE_SL
    BOOL Fin;
};

#if (MATRIX_LAYOUT == LAYOUT0) 
struct KSTATE KeyChange[SIZE_KEY];
struct KCODE KeyCode[SIZE_KEY] =
{
    {KCODE_TYPE_NORMAL , 0x0000 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0001 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0002 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0003 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0004 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0005 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0006 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0007 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0008 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0009 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000f , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0010 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0011 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0012 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0013 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0014 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0015 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0016 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0017 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0018 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0019 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001f , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0020 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0021 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0022 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0023 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0024 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0025 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0026 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0027 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0028 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0029 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002f , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0030 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0031 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0032 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0033 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0034 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0035 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0036 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0037 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0038 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0039 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003f , 0, 0},
};
#elif (MATRIX_LAYOUT == LAYOUT1)
struct KSTATE KeyChange[SIZE_KEY];
struct KCODE KeyCode[SIZE_KEY] =
{
    {KCODE_TYPE_NORMAL , 0x0000 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0001 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0002 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0003 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0004 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0005 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0006 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0007 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0008 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0009 , 0, 0}
};
#elif (MATRIX_LAYOUT == LAYOUT2) 
struct KSTATE KeyChange[SIZE_KEY];
struct KCODE KeyCode[SIZE_KEY] =
{
    {KCODE_TYPE_NORMAL , 0x0000 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0001 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0002 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0003 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0004 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0005 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0006 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0007 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0008 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0009 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x000f , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0010 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0011 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0012 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0013 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0014 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0015 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0016 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0017 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0018 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0019 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x001f , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0020 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0021 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0022 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0023 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0024 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0025 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0026 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0027 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0028 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0029 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x002f , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0030 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0031 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0032 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0033 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0034 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0035 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0036 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0037 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0038 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x0039 , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003a , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003b , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003c , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003d , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003e , 0, 0},
    {KCODE_TYPE_NORMAL , 0x003f , 0, 0},
};
#endif

extern void ReadRegDWORD( LPCWSTR szKeyName, LPCWSTR szValueName, LPDWORD pdwValue );
static void GPIO_PuEnable(ENUM_COL_ROW iClass, bool bFlag);
static void GPIO_CtrlHandler(ENUM_COL_ROW iClass, ENUM_GPIO_FUNC iLevel);
static void KEYIF_Column_Set(DWORD dVal);
static void KEYIF_Column_Bitset(bool bVal, int cIdx);
static DWORD KEYIF_Row_Read(void);
static void KEYIF_Status_Clear(void);
void Keypad_Clock_On(BOOL bOn);


/*****************************************************************************/

/*****************************************************************************
*    Function Name : KeybdPdd_PowerHandler
*    Function Desc  : Power Handler
*
*/
void WINAPI KeybdPdd_PowerHandler(BOOL bOff)
{
    if (!bOff)
    {
        Keyboard->KeybdPowerOn();
    }
    else
    {
        Keyboard->KeybdPowerOff();
    }
    return;
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : KeybdDriverInitializeAddresses
*    Function Desc  : KeyBoard Driver Initialization
*                     Read Registry
*
*/
BOOL KeybdDriverInitializeAddresses(void)
{
    bool RetValue = TRUE;

    DWORD dwSYSCONBase;
    DWORD dwIOBase;
    DWORD dwIOCTRLBase;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    DEBUGMSG(ZONE_INIT,(TEXT("++KeybdDriverInitializeAddresses\r\n")));

    ReadRegDWORD(TEXT("HARDWARE\\DEVICEMAP\\KEYBD"), _T("SYSCONBase"), &dwSYSCONBase );
    if(dwSYSCONBase == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("Can't fount registry entry : HARDWARE\\DEVICEMAP\\KEYBD\\SYSCONBase\r\n")));
        goto error_return;
    }
    DEBUGMSG(ZONE_INIT, (TEXT("HARDWARE\\DEVICEMAP\\KEYBD\\SYSCONBase:%x\r\n"), dwSYSCONBase));

    ReadRegDWORD(TEXT("HARDWARE\\DEVICEMAP\\KEYBD"), _T("IOBase"), &dwIOBase );
    if(dwIOBase == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("Can't fount registry entry : HARDWARE\\DEVICEMAP\\KEYBD\\IOBase\r\n")));
        goto error_return;
    }
    DEBUGMSG(ZONE_INIT, (TEXT("HARDWARE\\DEVICEMAP\\KEYBD\\IOBase:%x\r\n"), dwIOBase));

    ReadRegDWORD(TEXT("HARDWARE\\DEVICEMAP\\KEYBD"), _T("IOCTRLBase"), &dwIOCTRLBase );
    if(dwIOCTRLBase == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("Can't fount registry entry : HARDWARE\\DEVICEMAP\\KEYBD\\IOCTRLBase\r\n")));
        goto error_return;
    }
    DEBUGMSG(1, (TEXT("HARDWARE\\DEVICEMAP\\KEYBD\\IOCTRLBase:%x\r\n"), dwIOCTRLBase));

    // Syscon Virtual alloc
    ioPhysicalBase.LowPart = dwSYSCONBase;
    pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (pSysConReg == NULL)
    {
        DEBUGMSG(ZONE_INIT,(TEXT("[KBD] pSysConReg : MmMapIoSpace failed!\r\n")));
        goto error_return;
    }
    DEBUGMSG(ZONE_INIT, (TEXT("[KBD] pSysConReg mapped at %x\r\n"), pSysConReg));

    // GPIO Virtual alloc
    ioPhysicalBase.LowPart = dwIOBase;
    pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (pGPIOReg == NULL)
    {
        DEBUGMSG(ZONE_ERROR,(TEXT("[KBD] pGPIOReg : MmMapIoSpace failed!\r\n")));
        goto error_return;
    }
    DEBUGMSG(ZONE_INIT, (TEXT("[KBD] pGPIOReg mapped at %x\r\n"), pGPIOReg));

    // Keypad Virtual alloc
    ioPhysicalBase.LowPart = dwIOCTRLBase;    
    pKeyPadReg = (S3C6410_KEYPAD_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_KEYPAD_REG), FALSE);
    if (pKeyPadReg == NULL)
    {
        DEBUGMSG(ZONE_ERROR,(TEXT("[KBD] pKeyPadReg : MmMapIoSpace failed!\r\n")));
        goto error_return;
    }
    DEBUGMSG(ZONE_INIT, (TEXT("[KBD] pKeyPadReg mapped at %x\r\n"), pKeyPadReg));

    DEBUGMSG(ZONE_INIT,(TEXT("--KeybdDriverInitializeAddresses\r\n")));

    return TRUE;

error_return:

    if ( pSysConReg )
    {
        MmUnmapIoSpace((PVOID)pSysConReg, sizeof(S3C6410_SYSCON_REG));
        pSysConReg = NULL;
    }

    if ( pGPIOReg )
    {
        MmUnmapIoSpace((PVOID)pGPIOReg, sizeof(S3C6410_GPIO_REG));
        pGPIOReg = NULL;
    }

    if ( pKeyPadReg )
    {
        MmUnmapIoSpace((PVOID)pKeyPadReg, sizeof(S3C6410_KEYPAD_REG));
        pKeyPadReg = NULL;
    }

    DEBUGMSG(ZONE_INIT,(TEXT("--KeybdDriverInitializeAddresses[FAILED!!!]\r\n")));
    return FALSE;
}
/****************************************************************************/


/*****************************************************************************
*    Function Name : VarInit
*    Function Desc  : Initialization for variables about Key Change. 
*
*/
void VarInit(void)
{
    int i;

    for( i=0; i<SIZE_KEY; i++)
    {
        KeyChange[i].Mask = 0;
        KeyChange[i].Cnt  = 0;
    }
}
/****************************************************************************/


/*****************************************************************************
*    Function Name : AreAllKeysUp
*    Function Desc  : Check that all keys are released. 
*
*/
BOOL AreAllKeysUp(void)
{
    DWORD tmp = 0, tmp1 = 0, i;

    for(i=0;i<SIZE_COLS;i++)
    {
        tmp |= KeyState[i];
        tmp1 |= ChangeState[i];
    }

    return (tmp || tmp1)? FALSE:TRUE;
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : KScan_ProcState
*    Function Desc  : Handle KeyMatrix State. Check which keys are changed.
*
*/
DWORD KScan_ProcState(int kidx, int idx, PDWORD key, PBOOL press)
{
    int val, i;
    DWORD count = 0;
    int mask;

    val = ChangeState[kidx];
    for(i=0;i<SIZE_BITS;i++)
    {
        mask = 1<<i;

        if( (KeyState[kidx] & mask) && (KeyCode[idx].Type == KCODE_TYPE_SL) && (KeyCode[idx].Fin == 0) )
        {
            if( (KeyCode[idx].TimeoutCnt == CNT_LONGKEY) )
            {
                // key down
                *key++ = KeyCode[idx].Scan | SCAN_EXT;
                *press++ = FALSE;
                // key up
                *key++ = KeyCode[idx].Scan | SCAN_EXT;
                *press++ = TRUE;
                count+=2;

                KeyCode[idx].TimeoutCnt = 0;
                KeyCode[idx].Fin = TRUE;
                RETAILMSG(FALSE, (TEXT(">>> KSCAN:LONG[%d] - %d, key - 0x%x\r\n"), idx, KeyCode[idx].Scan,*key));
            }
            else
            {
                KeyCode[idx].TimeoutCnt++;
            }
        }

        // state changed
        if( val & mask )
        {
            // Need to check whether the key changed really
            if( KeyChange[idx].Cnt == 0 ) // in counting
            {
                KeyChange[idx].Cnt = CNT_VALIDKEY;
                KeyChange[idx].Mask = ( KeyState[kidx] & mask )? 0: mask;
            }
            else
            {
            // checked key state
                if( KeyChange[idx].Mask != (KeyState[kidx] & mask) )
                {
                    if( --KeyChange[idx].Cnt == 0 )
                    {
                        if( KeyChange[idx].Mask == 0 )  // Key UP
                        {
                            if( KeyCode[idx].Type == KCODE_TYPE_NORMAL )
                            {
                                *key++ = KeyCode[idx].Scan;
                                *press++ = TRUE;
                                count++;

                                RETAILMSG(FALSE, (TEXT(">>> KSCAN:UP  [%d] - %d\r\n"), idx, KeyCode[idx].Scan));
                            }
                            else    // KCODE_TYPE_SL
                            {
                                if( KeyCode[idx].Fin == FALSE )
                                {
                                    // key down
                                    *key++ = KeyCode[idx].Scan;
                                    *press++ = FALSE;
                                    // key up
                                    *key++ = KeyCode[idx].Scan;
                                    *press++ = TRUE;
                                    count+=2;

                                    KeyCode[idx].Fin = TRUE;

                                    RETAILMSG(FALSE, (TEXT(">>> KSCAN:SHORT[%d] - %d\r\n"), idx, KeyCode[idx].Scan));
                                }
                            }

                            KeyState[kidx] &= ~mask;
                        }
                        else    // Key Down
                        {
                            if( KeyCode[idx].Type == KCODE_TYPE_NORMAL )
                            {
                                *key++ = KeyCode[idx].Scan;
                                *press++ = FALSE;
                                count++;

                                RETAILMSG(FALSE, (TEXT(">>> KSCAN:DOWN[%d] - %d\r\n"), idx, KeyCode[idx].Scan));
                            }
                            else    // KCODE_TYPE_SL
                            {
                                KeyCode[idx].TimeoutCnt = 0;
                                KeyCode[idx].Fin = FALSE;
                            }

                            KeyState[kidx] |= mask;
                        }
                    }
                }
                else
                {
                    KeyChange[idx].Cnt = 0;
                    RETAILMSG(0, (TEXT(">>> KSCAN:Must be not occurred[%d] %x, %llx, %llx\r\n"), idx,KeyChange[idx].Mask,KeyState,mask));
                }
            }
        }
        else
        {
            if( KeyChange[idx].Cnt ) // in counting
            {
                KeyChange[idx].Cnt = 0;
                RETAILMSG(0, (TEXT(">>> KSCAN:Canceled [%d]\r\n"), idx));
            }
        }
        idx++;
    }

    return count;
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : KScan_ProcIO
*    Function Desc  : Scan code
*                     Read KEYIF
*
*/
void KScan_ProcIO(void)
{
    int i;

    for(i = 0 ; i < SIZE_COLS; i++)
    {
        KEYIF_Column_Set(0x0);
        // select a column
        KEYIF_Column_Bitset(false,i);
        Sleep(1);
        ChangeState[i] = KeyState[i]^((~ KEYIF_Row_Read())&0xff);
        RETAILMSG(FALSE, (TEXT("[KSCAN](%d)-%x\r\n"), i, ChangeState[i]));
        Sleep(1);
    }
    KEYIF_Column_Set(0x0);
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : KScan_SetINTMode
*    Function Desc  : Initialize the H/W
*
*/
void KScan_SetINTMode(void)
{
    RETAILMSG(FALSE,(TEXT("+Select all column\r\n")));
    // select all column - Set Keypad column GPIO to [Key PAD COL]
    GPIO_CtrlHandler(ENUM_COL, ENUM_AUXFUNC);

    KEYIF_Column_Set(0x0);

    // configure - Set Keypad row GPIO to [Key PAD ROW]
    GPIO_CtrlHandler(ENUM_ROW, ENUM_AUXFUNC);

    // unmask the key interrupt
    KEYIF_Status_Clear();
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : KeybdPdd_ToggleKeyNotification
*    Function Desc  : Toggle Key
*
*/
void WINAPI KeybdPdd_ToggleKeyNotification(KEY_STATE_FLAGS  KeyStateFlags)
{
    unsigned int fLights;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("KeybdPdd_ToggleKeyNotification\r\n")));

    fLights = 0;
    if (KeyStateFlags & KeyShiftCapitalFlag)
    {
        fLights |= 0x04;
    }

    if (KeyStateFlags & KeyShiftNumLockFlag)
    {
        fLights |= 0x2;
    }
    /*
    Keyboard lights is disabled once driver is installed because the
    PS2 controller sends back a response which goes to the IST and corrupts
    the interface.  When we figure out how to disable the PS2 response we
    can re-enable the lights routine below
    */

    return;
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : IsrThreadProc
*    Function Desc  : IST of KeyBD D/D
*        Create IST
*        Wait for Event according to the KEYPAD INT
*        KScan_SetIOMode() : Handle H/W
*        KScan_ProcIO() :
*        KScan_ProcState() :
*
*/
extern UINT v_uiPddId;
extern PFN_KEYBD_EVENT v_pfnKeybdEvent;

BOOL KeyMatrix::IsrThreadProc()
{
    DWORD dwPriority;
    DWORD i, step;

    DWORD   rguiScanCode[SIZE_KEY];
    BOOL    rgfKeyUp[SIZE_KEY];
    UINT    cEvents;
    DWORD ret;
    DWORD timeout;
    HANDLE gEventIntr;
    DWORD irq, sysintr;

    ReadRegDWORD( TEXT("HARDWARE\\DEVICEMAP\\KEYBD"), _T("Priority256"), &dwPriority );
    if(dwPriority == 0)
    {
        dwPriority = DEFAULT_PRIORITY;
    }

    DEBUGMSG(ZONE_INIT, (TEXT("+[KEYBD]IsrThreadProc\r\n")));
    // update the IST priority
    CeSetThreadPriority(GetCurrentThread(), (int)dwPriority);

    irq = IRQ_KEYPAD;
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &irq, sizeof(UINT32), &sysintr, sizeof(UINT32), NULL))
    {
        ERRORMSG( 1, (TEXT("ERROR: Failed to request the IRQ_KEY sysintr.\r\n")));
        sysintr = SYSINTR_UNDEFINED;
        return(FALSE);
    }

    gEventIntr = CreateEvent(NULL, FALSE, FALSE, NULL);
    if( NULL == gEventIntr )
    {
        ERRORMSG( 1, (TEXT("Event is not created\r\n")));
        return(FALSE);
    }

    if( InterruptInitialize(sysintr, gEventIntr, NULL, 0) == FALSE )
    {
        ERRORMSG( 1, (TEXT("interrupt is not initialized\n\r")));
        return(FALSE);
    }


    timeout = INFINITE;
    DEBUGMSG(ZONE_INIT, (TEXT("+[KEYBD]Enter Infinite Loop\r\n")));
    while(1)    // INFINITE LOOP ____________________________________________________________________
    {
        ret = WaitForSingleObject(gEventIntr, timeout); // Wait for Interrupt Event ________________________

        if( ret == WAIT_OBJECT_0 )
        {
            RETAILMSG( FALSE,(TEXT("Object : WAIT_OBJECT_0\r\n")));
            timeout = TIME_KEYSCAN;
        }

        // Clear Pressed/Released Interrupt
        KEYIF_Status_Clear();
        // Read the Matrix
        KScan_ProcIO();

        for( i=0, step=0; i< SIZE_COLS; i++, step+=SIZE_ROWS)
        {
            cEvents = KScan_ProcState( i, step, rguiScanCode, rgfKeyUp);

            if( cEvents )
            {
                for (UINT iEvent = 0; iEvent < cEvents; ++iEvent)
                {
                    v_pfnKeybdEvent(v_uiPddId, rguiScanCode[iEvent], rgfKeyUp[iEvent]);
                    RETAILMSG(FALSE,(TEXT("PddID : %x, ScanCode : %x, KeyUp : %d\r\n"),v_uiPddId, rguiScanCode[iEvent], rgfKeyUp[iEvent]));
                }
            }
        }

        if( TRUE == AreAllKeysUp() )
        {
            RETAILMSG(0,(TEXT("Key all up\r\n")));
            timeout = INFINITE;
        }
        InterruptDone(sysintr);
    }// INFINITE LOOP ____________________________________________________________________
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : KBDISRThread
*    Function Desc  : Keybd IST Wrapper
*        Call KeyMatrix.IsrThreadProc()
*
*/
DWORD KBDISRThread(KeyMatrix *pp2k)
{
    DEBUGMSG(ZONE_INIT,(TEXT("[KEYBD]KBDISRThread:\r\n")));
    pp2k->IsrThreadProc();
    return 0;
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : IsrThreadStart
*    Function Desc  : IST start function
*
*/
BOOL KeyMatrix::IsrThreadStart()
{
    HANDLE   hthrd;
    BOOL     bRet = TRUE;

    DEBUGMSG(ZONE_INIT,(TEXT("+[KEYBD]IsrThreadStart:\r\n")));
    hthrd = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)KBDISRThread,this,0,NULL);
    if(hthrd == NULL)
    {
        bRet = FALSE;
        DEBUGMSG(ZONE_ERROR,(TEXT("[KEYBD] IsrThreadStart. Error starting thread. Err = %d\r\n"), GetLastError()));
    }
    else
    {
        // Since we don't need the handle, close it now.
        CloseHandle(hthrd);
    }
    DEBUGMSG(ZONE_INIT,(TEXT("-[KEYBD]IsrThreadStart:\r\n")));
    return bRet;
}
/****************************************************************************/


/*****************************************************************************
*    Function Name : KeybdPowerOn
*    Function Desc  : Power on function
*        Key array (mask, cnt) initialization, GPIO, Interrupt Initialization
*
*/
BOOL KeyMatrix::KeybdPowerOn()
{
    DEBUGMSG(ZONE_INIT,(TEXT("++[KEYBD]KeyMatrix::KeybdPowerOn\r\n")));
    //
    // enable the Keypad Clock (PCLK)
    //
    Keypad_Clock_On(TRUE);

    pKeyPadReg->KEYIFCON = INT_F_DISABLE|INT_R_ENABLE|DF_EN_EN|FC_EN_EN;

    //Keypad interfae debouncing filter clock division register
    pKeyPadReg->KEYIFFC = FC_DIV_VAL(FT_CLK_DIV);

#if ((MATRIX_LAYOUT == LAYOUT0)||(MATRIX_LAYOUT == LAYOUT2))
    pKeyPadReg->KEYIFCOL = (0x00<<8);
#elif (MATRIX_LAYOUT == LAYOUT1)
    pKeyPadReg->KEYIFCOL = (0x7<<8);
#endif

    VarInit();
    KScan_SetINTMode();

    DEBUGMSG(ZONE_INIT,(TEXT("--[KEYBD]KeyMatrix::KeybdPowerOn\r\n")));
    return(TRUE);
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : KeybdPowerOff
*    Function Desc  : Power off function
*        Mask KEYIF INT, Column Low
*
*/
BOOL KeyMatrix::KeybdPowerOff()
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("++KeyMatrix::KeybdPowerOff\r\n")));

    // Clear Pressed/Released Interrupt
    KEYIF_Status_Clear();

    // select all column - Set Keypad column GPIO to [Key PAD COL]
    GPIO_CtrlHandler(ENUM_COL, ENUM_AUXFUNC);

    KEYIF_Column_Set(0x0);

    // configure - Set Keypad row GPIO to [Key PAD ROW]
    GPIO_CtrlHandler(ENUM_ROW, ENUM_AUXFUNC);

    //Clock Off
    Keypad_Clock_On(FALSE);

    DEBUGMSG(ZONE_FUNCTION,(TEXT("--KeyMatrix::KeybdPowerOff\r\n")));
    return(TRUE);
}
/****************************************************************************/

/*****************************************************************************
*    Function Name : GPIO Handling function
*    Function Desc  : GPIO_PuEnable, GPIO_CtrlHandler
*            Pull up Enable/Disable
*            GPIO Control register configuration only related to KEYPAD
*            GPIO Data register setting only related to KEYPAD
*
*/
/**
    [iClass] 0: Column, 1: Row
    [bFlag] 0: Pull up Enable, 1 : Pull up Disable
*/
static void GPIO_PuEnable(ENUM_COL_ROW iClass, bool bFlag)
{

    if(iClass == ENUM_COL)    // Column setting
    {
        if(bFlag)        // Pull up Enable
        {
            pGPIOReg->GPLPUD =  pGPIOReg->GPLPUD  | (0xaaaa<<0);    // KBC_0~7
        }
        else         // Pull up Disable
        {
            pGPIOReg->GPLPUD =  pGPIOReg->GPLPUD & ~ (0xffff<<0);    // KBC_0~7
        }
    }
    else         // Row Setting
    {
        if(bFlag)        // Pull up Enable
        {
            pGPIOReg->GPKPUD =  pGPIOReg->GPKPUD | (0xaaaa<<16);    // KBR_0~7
        }
        else         // Pull up Disable
        {
            pGPIOReg->GPKPUD =  pGPIOReg->GPKPUD & ~ (0xffff<<16);    // KBR_0~7
        }
    }

}

/**
    [iClass] 0: Column, 1: Row
    [iLevel] 0: INPUT, 1 : OUTPUT, 2 : Aux. Function, 3 : Reserved
*/

static void GPIO_CtrlHandler(ENUM_COL_ROW iClass, ENUM_GPIO_FUNC iLevel)
{
#if ((MATRIX_LAYOUT == LAYOUT0)||(MATRIX_LAYOUT == LAYOUT2))
    if(iClass == ENUM_COL)    // Column setting
    {
        switch(iLevel)
        {
        case ENUM_INPUT :
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xffffffff<<0)) | (0x0<<0);    //KBC_0(GPL0)~ KBC_8(GPL7)
            break;
        case ENUM_OUTPUT :
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xffffffff<<0)) | (0x11111111<<0);    //KBC_0(GPL0)~ KBC_8(GPL7)
            break;
        case ENUM_AUXFUNC :
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xffffffff<<0)) | (0x33333333<<0);    //KBC_0(GPL0)~ KBC_8(GPL7)
            break;
        default :    //ENUM_RESERVED
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xffffffff<<0)) | (0x44444444<<0);    //KBC_0(GPL0)~ KBC_8(GPL7)
            break;
        }
    }
    else if(iClass == ENUM_ROW)        // Row Setting
    {
        switch(iLevel)
        {
        case ENUM_INPUT :
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xffffffff<<0)) | (0x0<<0);     //row            break;
            break;
        case ENUM_OUTPUT :
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xffffffff<<0)) | (0x11111111<<0);     //row            break;
            break;
        case ENUM_AUXFUNC :
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xffffffff<<0)) | (0x33333333<<0);    //KBR_0(GPK8)~ KBR_7(GPK15)
            break;
        default :    //ENUM_RESERVED
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xffffffff<<0)) | (0x44444444<<0);    //KBR_0(GPK8)~ KBR_7(GPK15)
            break;
        }
    }
    else
    {
        DEBUGMSG(ZONE_ERROR,(TEXT("Invalid Parameter\r\n")));
    }
#elif (MATRIX_LAYOUT == LAYOUT1)
    if(iClass == ENUM_COL)    // Column setting
    {
        switch(iLevel)
        {
        case ENUM_INPUT :
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xfffff<<12)) | (0x00000<<12);    //KBC_3(GPL3)~ KBC_7(GPL7)
            break;
        case ENUM_OUTPUT :
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xfffff<<12)) | (0x11111<<12);    //KBC_3(GPL3)~ KBC_7(GPL7)
            break;
        case ENUM_AUXFUNC :
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xfffff<<12)) | (0x33333<<12);    //KBC_3(GPL3)~ KBC_7(GPL7)
            break;
        default :    //ENUM_RESERVED
            pGPIOReg->GPLCON0=
                (pGPIOReg->GPLCON0 & ~(0xfffff<<12)) | (0x44444<<12);    //KBC_3(GPL3)~ KBC_7(GPL7)
            break;
        }
    }
    else if(iClass == ENUM_ROW)        // Row Setting
    {
        switch(iLevel)
        {
        case ENUM_INPUT :
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xff<<0)) | (0x00<<0);    //KBR_0(GPK8)~ KBR_1(GPK9)
            break;
        case ENUM_OUTPUT :
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xff<<0)) | (0x11<<0);    //KBR_0(GPK8)~ KBR_1(GPK9)
            break;
        case ENUM_AUXFUNC :
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xff<<0)) | (0x33<<0);    //KBR_0(GPK8)~ KBR_1(GPK9)
            break;
        default :    //ENUM_RESERVED
            pGPIOReg->GPKCON1=
                (pGPIOReg->GPKCON1 & ~(0xff<<0)) | (0x44<<0);    //KBR_0(GPK8)~ KBR_1(GPK9)
            break;
        }
    }
    else
    {
        DEBUGMSG(ZONE_ERROR,(TEXT("Invalid Parameter\r\n")));
    }
#endif
}

/**
    [dVal] Value of KBC
*/
static void KEYIF_Column_Set(DWORD dVal)
{
    pKeyPadReg->KEYIFCOL = (dVal & 0xff);
}

static void KEYIF_Column_Bitset(bool bVal, int cIdx)
{
#if ((MATRIX_LAYOUT == LAYOUT0)||(MATRIX_LAYOUT == LAYOUT2))
    if(bVal)
    {
        pKeyPadReg->KEYIFCOL = pKeyPadReg->KEYIFCOL | (0x1 << cIdx);
    }
    else
    {
        pKeyPadReg->KEYIFCOL = pKeyPadReg->KEYIFCOL | (0xff & ~(0x1 << cIdx));
    }
#elif (MATRIX_LAYOUT == LAYOUT1)
    if(bVal)
    {
        pKeyPadReg->KEYIFCOL = pKeyPadReg->KEYIFCOL | (0x1 << (cIdx+3));
    }
    else
    {
        pKeyPadReg->KEYIFCOL = pKeyPadReg->KEYIFCOL | (0xff & ~(0x1 << (cIdx+3)));
    }
#endif
}

static DWORD KEYIF_Row_Read(void)
{
    return pKeyPadReg->KEYIFROW;
}

static void KEYIF_Status_Clear(void)
{
    pKeyPadReg->KEYIFSTSCLR = CLEAR_P_INT|CLEAR_R_INT;    // Clear Pressed/Released Interrupt
}

void Keypad_Clock_On(BOOL bOn)
{
    if (bOn)
    {
        pSysConReg->PCLK_GATE |= KEY_POWER_ON;
    }
    else
    {
        pSysConReg->PCLK_GATE &= ~KEY_POWER_ON;
    }
    return;
}

