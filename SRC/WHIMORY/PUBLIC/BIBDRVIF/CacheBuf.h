/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : CacheBuf.h                                                      */
/* PURPOSE : This file contains head files of CacheBuf.c                     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                COPYRIGHT 2003 SAMSUNG ELECTRONICS CO., LTD.               */
/*                      ALL RIGHTS RESERVED                                  */
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
/* - 20-MAR-2003 [SongHo Yoon]: first writing                                */
/* - 19-APR-2004 [Janghwan Kim]: PocketStoreII migration                     */
/*                                                                           */
/*****************************************************************************/

#ifndef _POCKETSTORE_NFLAT_CACHEBUF_H_
#define _POCKETSTORE_NFLAT_CACHEBUF_H_

#include <windows.h>
//#include <OEMFlashIO.h>
#include <WMRConfig.h>

#define	NUM_OF_SEC_CACHE_ROW		128
#define	NUM_OF_SEC_CACHE_COLUMN		8

#define PSII_SECTOR_SIZE            WMR_SECTOR_SIZE


typedef struct _NFLAT_SEC_CACHE
{
	BOOL  bValid;
	UINT  nSecNum;
	UINT  nSecHit;
//	UCHAR aBuf[MAINPAGE_SIZE];
	UCHAR aBuf[PSII_SECTOR_SIZE];
} NFLAT_SEC_CACHE;

typedef struct _NFLAT_CACHEBUF
{
	UINT                nNumOfSecCache;
	NFLAT_SEC_CACHE    *pSecCache;
} NFLAT_CACHEBUF;

NFLAT_CACHEBUF  *NFLATCache_Init        (UINT nNumOfSecCache);
VOID             NFLATCache_Deinit      (NFLAT_CACHEBUF *pCacheBuf);
NFLAT_SEC_CACHE *NFLATCache_GetSecCache (NFLAT_CACHEBUF *pCacheBuf, UINT nSecNum);
//VOID             NFLATCache_AddSecCache (NFLAT_CACHEBUF *pCacheBuf, UINT nSecNum, UCHAR aBuf[MAINPAGE_SIZE]);
VOID             NFLATCache_AddSecCache (NFLAT_CACHEBUF *pCacheBuf, UINT nSecNum, UCHAR aBuf[PSII_SECTOR_SIZE]);

#endif //_POCKETSTORE_NFLAT_CACHEBUF_H_