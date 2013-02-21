#ifndef __BSP_ARGS_H
#define __BSP_ARGS_H

//------------------------------------------------------------------------------
//
// File:        bsp_args.h
//
// Description: This header file defines device structures and constant related
//              to boot configuration. BOOT_CFG structure defines layout of
//              persistent device information. It is used to control boot
//              process. BSP_ARGS structure defines information passed from
//              boot loader to kernel HAL/OAL. Each structure has version
//              field which should be updated each time when structure layout
//              change.
//
//------------------------------------------------------------------------------

#include "oal_args.h"
#include "oal_kitl.h"

#define BSP_ARGS_VERSION    1

#define BSP_ARGS_QUERY_HIVECLEAN        (BSP_ARGS_QUERY)        // Query hive clean flag.
#define BSP_ARGS_QUERY_CLEANBOOT        (BSP_ARGS_QUERY+1)        // Query clean boot flag.
#define BSP_ARGS_QUERY_FORMATPART    (BSP_ARGS_QUERY+2)        // Query format partition flag.

#define BSP_ARGS_QUERY_POWERCTL		  (BSP_ARGS_QUERY+3)		// Query PowerCtl Flag
#define BSP_ARGS_QUERY_DISPLAYTYPE	   (BSP_ARGS_QUERY+4)
#define BSP_ARGS_QUERY_FRAMEBUFFERDEPTH  (BSP_ARGS_QUERY+5)
#define BSP_ARGS_QUERY_BACKGROUNDCOLOR  (BSP_ARGS_QUERY+6)
#define BSP_ARGS_QUERY_LOGOHW			(BSP_ARGS_QUERY+7)
#define BSP_ARGS_QUERY_DEBUGUART		(BSP_ARGS_QUERY+8)
typedef struct
{
    OAL_ARGS_HEADER    header;
    UINT8               deviceId[16];            // Device identification
    OAL_KITL_ARGS       kitl;
    UINT8               uuid[16];
    BOOL                bUpdateMode;            // TRUE = Enter update mode on reboot.
    BOOL                bHiveCleanFlag;            // TRUE = Clean hive at boot
    BOOL                bCleanBootFlag;            // TRUE = Clear RAM, user objectstore at boot
    BOOL                bFormatPartFlag;        // TRUE = Format partion when mounted at boot
    DWORD               nfsblk;                    // for NAND Lock Tight
    HANDLE              g_SDCardDetectEvent;    //For USB MSF , check SD Card insert & remove.
    DWORD               g_SDCardState;            //For USB MSF , check SD Card insert & remove.
	DWORD				powerCTL;
	DWORD				displayType;
	DWORD				framebufferDepth;
	DWORD				backgroundColor;
	DWORD				logoHW;
	DWORD				debugUART;
} BSP_ARGS;

//------------------------------------------------------------------------------
//
//  Function:  OALArgsInit
//
//  This function is called by other OAL modules to intialize value of argument
//  structure.
//
VOID OALArgsInit(BSP_ARGS *pBSPArgs);

//------------------------------------------------------------------------------

#endif    // __BSP_ARGS_H