//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

/**
 * @file    fglconfig.h
 * @brief    This is the configuration header file for fgl.
 * @version    1.5
 */

#if !defined(__FIMG_CONFIG_H__)
#define __FIMG_CONFIG_H__

/*----------------------------------------------------------------------------*/

#define FIMG_MAJOR         1
#define FIMG_MINOR         0
#define FIMG_BUILD         00

/*----------------------------------------------------------------------------*/
/*
#define FIMG_DEBUG
#define VIP_WORKAROUND
#define FPGA_WORKAROUND

#define HOST_RENDER_STATUS        0x00000100
#define HOST_EXCUTE_FUNCTION    0x00000104
#define HOST_ERROR_CODE            0x00000108
*/
#define _FIMG3DSE_VER_1_1       /* Cancel */

#define _FIMG3DSE_VER_1_2       1
#define _FIMG3DSE_VER_1_2_1     2

#define _FIMG3DSE_VER_2_0     4
#define __STRICT_CHECK_CORRECT__    1
#define TARGET_FIMG_VERSION     _FIMG3DSE_VER_1_2_1
#define _FIMG_PIPELINE_SINGLE

//#define _FGL_VALUE_CHECK

#define WIN32_VIP         1
#define FPGA_BOARD         2
#define EVAL_BOARD      3

#define TARGET_PLATFORM FPGA_BOARD

#if TARGET_PLATFORM == FPGA_BOARD
#define DUMP_FRAME_BUFFER
#endif


/*
#if TARGET_PLATFORM == WIN32_VIP
#define MemAlloc malloc
#define MemFree free
#define AcMemcpy memcpy
    typedef HDC NativeDisplayType;
    typedef HWND NativeWindowType;
    typedef HBITMAP NativePixmapType;
#elif TARGET_PLATFORM == FPGA_BOARD
#define MemAlloc malloc
#define MemFree free
#define AcMemcpy memcpy
#include "2460addr.h"
#include "lcdlib.h"
    typedef unsigned char NativeDisplayType;
    typedef unsigned char NativeWindowType;
    typedef unsigned char NativePixmapType;
#elif TARGET_PLATFORM == EVAL_BOARD
#include "WmCrayon.h"
#include "WmGrpInterface.h"
#define malloc MemAlloc
#define free(x) MemFree(x)
    typedef unsigned char NativeWindowType;
    typedef WmScreen NativeDisplayType;
    typedef unsigned char NativePixmapType;
#endif
*/
/*----------------------------------------------------------------------------*/

#if defined(FIMG_DEBUG)
#define FIMG_PDUMP
#define FIMG_TRACE
#endif

/*
#if defined(FIMG_SOLENG)
#if !defined(SOLENG)
#define SOLENG
#endif
#endif
*/




#endif    /* __FIMG_CONFIG_H__ */


