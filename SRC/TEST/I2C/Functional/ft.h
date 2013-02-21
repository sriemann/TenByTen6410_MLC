//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
////////////////////////////////////////////////////////////////////////////////
//
//  I2CTUX TUX DLL
//
//  Module: ft.h
//          Declares the TUX function table and test function prototypes EXCEPT
//          when included by globals.cpp, in which case it DEFINES the function
//          table.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#if (!defined(__FT_H__) || defined(__GLOBALS_CPP__))
#ifndef __FT_H__
#define __FT_H__
#endif

////////////////////////////////////////////////////////////////////////////////
// Local macros

#ifdef __DEFINE_FTE__
#undef BEGIN_FTE
#undef FTE
#undef FTH
#undef END_FTE
#define BEGIN_FTE FUNCTION_TABLE_ENTRY g_lpFTE[] = {
#define FTH(a, b) { TEXT(b), a, 0, 0, NULL },
#define FTE(a, b, c, d, e) { TEXT(b), a, d, c, e },
#define END_FTE { NULL, 0, 0, 0, NULL } };
#else // __DEFINE_FTE__
#ifdef __GLOBALS_CPP__
#define BEGIN_FTE
#else // __GLOBALS_CPP__
#define BEGIN_FTE extern FUNCTION_TABLE_ENTRY g_lpFTE[];
#endif // __GLOBALS_CPP__
#define FTH(a, b)
#define FTE(a, b, c, d, e) TESTPROCAPI e(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
#define END_FTE
#endif // __DEFINE_FTE__

////////////////////////////////////////////////////////////////////////////////
// TUX Function table
//  To create the function table and function prototypes, two macros are
//  available:
//
//      FTH(level, description)
//          (Function Table Header) Used for entries that don't have functions,
//          entered only as headers (or comments) into the function table.
//
//      FTE(level, description, code, param, function)
//          (Function Table Entry) Used for all functions. DON'T use this macro
//          if the "function" field is NULL. In that case, use the FTH macro.
//
//  You must not use the TEXT or _T macros here. This is done by the FTH and FTE
//  macros.
//
//  In addition, the table must be enclosed by the BEGIN_FTE and END_FTE macros.

BEGIN_FTE
    FTH(0, "Stream Interface API test cases")
    FTE(1,    "Open/Close test",                                          1001, 0, I2CTUX_OpenCloseVerify                  )
    FTE(1,    "IOCTL_POWER_CAPABILITIES test",                            1002, 0, I2CTUX_IoctlPowerCapabilitiesVerify     )
    FTE(1,    "IOCTL_IIC_SET_CLOCK and IOCTL_IIC_GET_CLOCK test",         1003, 0, I2CTUX_IoctlClockVerify                 )
    FTE(1,    "IOCTL_IIC_SET_FILTER and IOCTL_IIC_GET_FILTER test",       1004, 0, I2CTUX_IoctlFilterVerify                )
    FTE(1,    "IOCTL_IIC_SET_DELAY and IOCTL_IIC_GET_DELAY test",         1005, 0, I2CTUX_IoctlDelayVerify                 )
    FTH(0, "Stream Interface API Invalid parameters test cases"                                                            )
    FTE(1,    "I2C IOCTL_POWER_CAPABILITIES with invalid parameter test", 1101, 0, I2CTUX_Invalid_IOCTL_POWER_CAPABILITIES )
    FTE(1,    "I2C IOCTL_IIC_WRITE with invalid parameter test",          1102, 0, I2CTUX_Invalid_IOCTL_IIC_WRITE          )
    FTE(1,    "I2C IOCTL_IIC_READ with invalid parameter test",           1103, 0, I2CTUX_Invalid_IOCTL_IIC_READ           )
    FTE(1,    "I2C IOCTL_IIC_SET_CLOCK with invalid parameter test",      1104, 0, I2CTUX_Invalid_IOCTL_IIC_SET_CLOCK      )
    FTE(1,    "I2C IOCTL_IIC_GET_CLOCK with invalid parameter test",      1105, 0, I2CTUX_Invalid_IOCTL_IIC_GET_CLOCK      )
    FTE(1,    "I2C IOCTL_IIC_SET_FILTER with invalid parameter test",     1106, 0, I2CTUX_Invalid_IOCTL_IIC_SET_FILTER     )
    FTE(1,    "I2C IOCTL_IIC_GET_FILTER with invalid parameter test",     1107, 0, I2CTUX_Invalid_IOCTL_IIC_GET_FILTER     )
    FTE(1,    "I2C IOCTL_IIC_SET_DELAY with invalid parameter test",      1108, 0, I2CTUX_Invalid_IOCTL_IIC_SET_DELAY      )
    FTE(1,    "I2C IOCTL_IIC_GET_DELAY with invalid parameter test",      1109, 0, I2CTUX_Invalid_IOCTL_IIC_GET_DELAY      )
    FTH(0, "Transfer performance test cases"                                                                               )
    FTE(1,    "IIC read performance test",                                1201, 0, I2CTUX_ReadPerformance                  )
    FTE(1,    "IIC write performance test",                               1202, 0, I2CTUX_WritePerformance                 )
END_FTE

////////////////////////////////////////////////////////////////////////////////

#endif // !__FT_H__ || __GLOBALS_CPP__
////////////////////////////////////////////////////////////////////////////////
//
//  I2CTUX TUX DLL
//  Copyright (c) Microsoft Corporation
//
//  Module: globals.h
//          Declares all global variables and test function prototypes EXCEPT
//          when included by globals.cpp, in which case it DEFINES global
//          variables, including the function table.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

////////////////////////////////////////////////////////////////////////////////
// Local macros

#ifdef __GLOBALS_CPP__
#define GLOBAL
#define INIT(x) = x
#else // __GLOBALS_CPP__
#define GLOBAL  extern
#define INIT(x)
#endif // __GLOBALS_CPP__

////////////////////////////////////////////////////////////////////////////////
// Global macros

#define countof(x)  (sizeof(x)/sizeof(*(x)))

////////////////////////////////////////////////////////////////////////////////
// Global function prototypes

void            Debug(LPCTSTR, ...);
SHELLPROCAPI    ShellProc(UINT, SPPARAM);

////////////////////////////////////////////////////////////////////////////////
// TUX Function table

#include "ft.h"

////////////////////////////////////////////////////////////////////////////////
// Globals

// Global CKato logging object. Set while processing SPM_LOAD_DLL message.
GLOBAL CKato            *g_pKato INIT(NULL);

// Global shell info structure. Set while processing SPM_SHELL_INFO message.
GLOBAL SPS_SHELL_INFO   *g_pShellInfo;

// Add more globals of your own here. There are two macros available for this:
//  GLOBAL  Precede each declaration/definition with this macro.
//  INIT    Use this macro to initialize globals, instead of typing "= ..."
//
// For example, to declare two DWORDs, one uninitialized and the other
// initialized to 0x80000000, you could enter the following code:
//
//  GLOBAL DWORD        g_dwUninit,
//                      g_dwInit INIT(0x80000000);
////////////////////////////////////////////////////////////////////////////////

#endif // __GLOBALS_H__
////////////////////////////////////////////////////////////////////////////////
//
//  I2CTUX TUX DLL
//  Copyright (c) Microsoft Corporation
//
//  Module: main.h
//          Header for all files in the project.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MAIN_H__
#define __MAIN_H__

////////////////////////////////////////////////////////////////////////////////
// Included files

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <tux.h>
#include <kato.h>

////////////////////////////////////////////////////////////////////////////////
// Suggested log verbosities

#define LOG_EXCEPTION          0
#define LOG_FAIL               2
#define LOG_ABORT              4
#define LOG_SKIP               6
#define LOG_NOT_IMPLEMENTED    8
#define LOG_PASS              10
#define LOG_DETAIL            12
#define LOG_COMMENT           14

////////////////////////////////////////////////////////////////////////////////

#endif // __MAIN_H__
