/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : OS Adaptation Module                                        */
/* FILE        : WMROAM.h                                                    */
/* PURPOSE     : This file contains the definition and protypes of exported  */
/*              functions for OS Adaptation Module.                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2005 SAMSUNG ELECTRONICS CO., LTD.                */
/*                          ALL RIGHTS RESERVED                              */
/*                                                                           */
/*   Permission is hereby granted to licensees of Samsung Electronics        */
/*   Co., Ltd. products to use or abstract this computer program for the     */
/*   sole purpose of implementing a product based on Samsung                 */
/*   Electronics Co., Ltd. products. No other rights to reproduce, use,      */
/*   or disseminate this computer program, whether in part or in whole,      */
/*   are granted.                                                            */
/*                                                                           */
/*   Samsung Electronics Co., Ltd. makes no representation or warranties     */
/*   with respect to the performance of this computer program, and           */
/*   specifically disclaims any responsibility for any damages,              */
/*   special or consequential, connected with the use of this program.       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* REVISION HISTORY                                                          */
/*                                                                           */
/*   18-AUG-2005 [Yangsup Lee] : first writing                               */
/*                                                                           */
/*****************************************************************************/

#ifndef _WMR_OAM_H_
#define _WMR_OAM_H_

#define WHIMORY_PORT_BLUES  0
#define MMT_TEST            0

#if (WHIMORY_PORT_BLUES)
#include "Uart_api.h"
#include "Basic_typedefs.h"
#define	WMR_DBG_PRINT(x)	Uart_Printf x
#define	WMR_RTL_PRINT(x)	Uart_Printf x
#else
#include <windows.h>
#define	WMR_DBG_PRINT(x)	RETAILMSG(1, x)
#define	WMR_RTL_PRINT(x)	RETAILMSG(1, x)
#endif

/*****************************************************************************/
/* OS dependent memory management functions definition                       */
/* if OS is changed, edit this MACROs									     */
/*****************************************************************************/
#define		WMR_MEMSET(x, y, z)				OAM_Memset(x, y, z)
#define		WMR_MEMCPY(x, y, z)				OAM_Memcpy(x, y, z)
#define		WMR_MALLOC(x)					OAM_Malloc(x)
#define		WMR_FREE(x)					OAM_Free(x)


#if 0
#undef	TEXT
#define	TEXT(x)					(VOID *) (x)
#endif

#undef	INLINE
#define	INLINE					__inline

/*****************************************************************************/
/* ASSERT MACRO #define                                                      */
/*****************************************************************************/
#define	WMR_ASSERT(x)   OAM_Assert(x, (TEXT(__FILE__)), (UINT32)__LINE__)

#if (MMT_TEST)
#define	MMT_ASSERT(x)   MMT_Assert(x, (TEXT(__FILE__)), (UINT32)__LINE__)
#else
#define	MMT_ASSERT(x)
#endif

/*****************************************************************************/
/* NULL #defines                                                             */
/*****************************************************************************/
#ifndef		NULL
#ifdef		__cplusplus
#define		NULL				0
#else
#define		NULL				((void *)0)
#endif
#endif

/*****************************************************************************/
/* PRIVATE #defines                                                          */
/*****************************************************************************/
#define		PRIVATE				static

/*****************************************************************************/
/* exported function prototype of OAM                                        */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

UINT16  WMR_GetChkSum(UINT8 *pBuf, UINT32 nSize);

VOID*   OAM_Malloc(UINT32 nSize);
VOID    OAM_Free(VOID *pMem);
VOID    OAM_Memcpy(VOID *pDst, VOID *pSrc, UINT32 nLen);
VOID    OAM_Memset(VOID *pDst, UINT8 nV, UINT32 nLen);
VOID    OAM_Debug(VOID *pFmt, ...);
VOID    OAM_Assert(BOOL32 bVal, const TCHAR *szFile, UINT32 nLine);
VOID    MMT_Assert(BOOL32 bVal, const TCHAR *szFile, UINT32 nLine);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _WMR_OAM_H_ */
