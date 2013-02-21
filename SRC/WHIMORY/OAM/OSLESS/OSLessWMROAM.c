/*****************************************************************************/
/*                                                                           */
/* PROJECT : Rainbow	                                                     */
/* MODULE  : OS Adaptation Module                                            */
/* NAME    : OSLess OAM                                                      */
/* FILE    : WMROAM.c                                                        */
/* PURPOSE : This file contain the OS Adaptation Modules for OSless platform */
/*           such as BootLoader                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2005 SAMSUNG ELECTRONICS CO., LTD.                */
/*                          ALL RIGHTS RESERVED                              */
/*                                                                           */
/*   Permission is hereby granted to licenses of Samsung Electronics         */
/*   Co., Ltd. products to use or abstract this computer program only in     */
/*   accordance with the terms of the NAND FLASH MEMORY SOFTWARE LICENSE     */
/*   AGREEMENT for the sole purpose of implementing a product based on       */
/*   Samsung Electronics Co., Ltd. products. No other rights to reproduce,   */
/*   use, or disseminate this computer program, whether in part or in whole, */
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
/*   18-AUG-2005 [Yangsup Lee]  : first writing                              */
/*                                                                           */
/*****************************************************************************/
#include <windows.h>
#include "WMRConfig.h"
#include "WMRTypes.h"
#include "OSLessWMROAM.h"

//#include <stdarg.h>
//#include <stdio.h>

//#include "S5L8700.h"

/*****************************************************************************/
/* Global variables definitions                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Local #defines                                                            */
/*****************************************************************************/
#define		OSLESS_MALLOC_POOL_SIZE_BY_KB	50
#define		WMR_LOCAL_MEM_SIZE		((OSLESS_MALLOC_POOL_SIZE_BY_KB * 1024) / sizeof(UINT32))

/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local constant definitions                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/
static UINT32  aMemBuf[WMR_LOCAL_MEM_SIZE];
static UINT32  nMallocPtr = 0;

static	BYTE	memAddr[OSLESS_MALLOC_POOL_SIZE_BY_KB];
static	BOOL	firstMemalloc = TRUE;

typedef struct _MEMARRAY {
    DWORD startAddress;     // starting sector number
	DWORD size;   // number of sectors
} MEMARRAY, * PMEMARRAY;

static	MEMARRAY	memarray[OSLESS_MALLOC_POOL_SIZE_BY_KB];

#define USED	1
#define	FREE	0

/* for Blues LLD */
//UINT8 	aTempSBuf[512];
//UINT8 	aMakeSECCBuf[512];
//UINT8 	aTempEccBuf[12];
//BOOL32	aNeedSync[WMR_MAX_DEVICE * 2];

//UINT8 TestMBuf[2];
//UINT8 TestMBuf[2048];
//UINT8 TestSBuf[512];


// for TFS4
#if 0
UINT8 szBuff[512];

t_char sBuff[2];
t_uint8	usBuffer[2];
t_char      *pArg[2];
t_kfat_string       sTmp;
t_kfat_string           strUnicode;
t_kfat_string strChild, strChildShort, strRead;
t_dir_entry             stDE[2];
t_file_table_entry stParent, stChild;
#endif

#if 0
extern UINT8 aChecksumTable[];
#else
/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/
PRIVATE const UINT8 aChecksumTable[256]  =
   { 8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,
      7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
      7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
      7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
      5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
      7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
      5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
      6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
      5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
      5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
      4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0
    };

#endif

/*****************************************************************************/
/* Code Implementation                                                       */
/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      WMR_GetChkSum                                                        */
/* DESCRIPTION                                                               */
/*      This function returns the sum of 0 count of pBuf.				     */
/* PARAMETERS                                                                */
/*      pBuf    [IN] 	the pointer of buffer	                             */
/*      nSize	[IN] 	the size of buffer		                             */
/* RETURN VALUES                                                             */
/*		the sum of 0 count													 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
UINT16
WMR_GetChkSum(UINT8 *pBuf, UINT32 nSize)
{
	register UINT32 nIdx;
	register const UINT8 *pD8;
	register const UINT8 *pCheckSum;
	register UINT32 nSum = 0;

	pD8 = pBuf;

	pCheckSum = aChecksumTable;

	if(nSize < 512)
	{
		for (nIdx = 0; nIdx < nSize; nIdx++)
		{
			nSum += *(pCheckSum + *(pD8++));
		}
	}
	else
	{
		for (nIdx = 0; nIdx < nSize / 8; nIdx++)
		{
			nSum += *(pCheckSum + *(pD8++));
			nSum += *(pCheckSum + *(pD8++));
			nSum += *(pCheckSum + *(pD8++));
			nSum += *(pCheckSum + *(pD8++));
			nSum += *(pCheckSum + *(pD8++));
			nSum += *(pCheckSum + *(pD8++));
			nSum += *(pCheckSum + *(pD8++));
			nSum += *(pCheckSum + *(pD8++));
		}
	}

	return (UINT16)nSum;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      OAM_Malloc                                                           */
/* DESCRIPTION                                                               */
/*      This function allocates memory for Whimory                           */
/*                                                                           */
/* PARAMETERS                                                                */
/*      nSize       [IN]                                                     */
/*            Size to be allocated                                           */
/*                                                                           */
/* RETURN VALUES                                                             */
/*      Pointer of allocated memory                                          */
/*                                                                           */
/* NOTES                                                                     */
/*      This function is called by function that wants to use memory         */
/*                                                                           */
/*****************************************************************************/
VOID *
OAM_Malloc(UINT32 nSize)
{
#ifdef NO_MALLOC	// by dodan2 061216
	WMR_RTL_PRINT((TEXT("[FTL:ERR]  OAM_Malloc() : NO_MALLOC defined\r\n")));
	while(1);
#else
	UINT32  nAlignSize;
	UINT32	nAlignArrayStart;
	UINT32	nAlignArrayNumber;
	UINT32	i;
	UINT32	j;

//	RETAILMSG(1, (L"OAM_Malloc : nSize=0x%x \n", nSize));

	if ( firstMemalloc == TRUE )
	{
		for ( i = 0; i < OSLESS_MALLOC_POOL_SIZE_BY_KB; i++ )
		{
			memAddr[i] = FREE;
			memarray[i].startAddress = 0;
			memarray[i].size = 0;
		}
		firstMemalloc = FALSE;
	}

	nAlignArrayNumber = nSize/1024+(nSize%1024==0?0:1);
//	RETAILMSG(1, (L"OAM_Malloc : nAlignArrayNumber=%d \n", nAlignArrayNumber));
	nAlignSize = nAlignArrayNumber*1024;
//	RETAILMSG(1, (L"OAM_Malloc : nAlignSize=%d \n", nAlignSize));

	for ( i = 0; i < OSLESS_MALLOC_POOL_SIZE_BY_KB; i++ )
	{
//		RETAILMSG(1, (L"OAM_Malloc : i=%d, memAddr[%d]=0x%x \n", i, i, memAddr[i]));
		if ( memAddr[i] == USED ) continue;
		for (j = 0; j < nAlignArrayNumber; j++)
		{
//			RETAILMSG(1, (L"OAM_Malloc : i+j=%d, memAddr[%d]=0x%x \n", i+j, i+j, memAddr[i+j]));
			if ( memAddr[i+j] == FREE )
				continue;
			else
				break;
		}
//		RETAILMSG(1, (L"OAM_Malloc : j=%d, nAlignArrayNumber=%d \n", j, nAlignArrayNumber));
		if ( j == nAlignArrayNumber )	// found
		{
			nAlignArrayStart = i;
			nMallocPtr = i*1024/sizeof(UINT32);
			break;
		}
	}
//	RETAILMSG(1, (L"OAM_Malloc : nMallocPtr=0x%x \n", nMallocPtr));

//	RETAILMSG(1, (L"OAM_Malloc : nAlignArrayStart=0x%x \n", nAlignArrayStart));
//	RETAILMSG(1, (L"OAM_Malloc : nAlignArrayNumber=0x%x \n", nAlignArrayNumber));
	for ( i = nAlignArrayStart; i < nAlignArrayStart+nAlignArrayNumber; i++ )
	{
		memAddr[i] = USED;
	}

 	if (nMallocPtr+nAlignSize > WMR_LOCAL_MEM_SIZE*4)
	{
		RETAILMSG(1, (L"aMemBuf=0x%x\n", &(aMemBuf[0])));
		RETAILMSG(1, (L"nAlignSize=0x%x nMallocPtr=0x%x WMR_LOCAL_MEM_SIZE=0x%x\n", nAlignSize, nMallocPtr, WMR_LOCAL_MEM_SIZE));
 		return NULL;
	}

	for ( i = 0; i < OSLESS_MALLOC_POOL_SIZE_BY_KB; i ++ )
	{
//		RETAILMSG(1, (L"OAM_Malloc : memarray[%d].startAddress=0x%x \n", i, memarray[i].startAddress));
		if ( memarray[i].startAddress == 0 )
		{
			memarray[i].startAddress	= (UINT32)(&(aMemBuf[nMallocPtr]));
			memarray[i].size			= nAlignSize;
			break;
		}
	}
//	RETAILMSG(1, (L"OAM_Malloc : memarray[%d].startAddress=0x%x \n", i, memarray[i].startAddress));
//	RETAILMSG(1, (L"OAM_Malloc : memarray[%d].size=0x%x \r\n", i, memarray[i].size));
//	RETAILMSG(1, (L"OAM_Malloc : &(aMemBuf[0x%x])=0x%x \r\n", nMallocPtr, &(aMemBuf[nMallocPtr])));
	return (VOID *) &(aMemBuf[nMallocPtr]);
#endif
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*  OAM_Free                                                                 */
/* DESCRIPTION                                                               */
/*      This function free memory that XSR allocated                         */
/*                                                                           */
/* PARAMETERS                                                                */
/*  pMem        Pointer to be free                                           */
/*                                                                           */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/*                                                                           */
/* NOTES                                                                     */
/*      This function is called by function that wants to free memory        */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
VOID
OAM_Free(VOID  *pMem)
{
#ifdef NO_MALLOC	// by dodan2 061216
	WMR_RTL_PRINT((TEXT("[FTL:ERR]  OAM_Free() : NO_MALLOC defined\r\n")));
	while(1);
#else
	UINT32	i, j;
	UINT32	arraynum;

//	RETAILMSG(1, (L"OAM_Free : (UINT32)(pMem)=0x%x \n", (UINT32)(pMem)));
	if ( (UINT32)(pMem) != 0xFFFFFFFF )
	{
		for ( i = 0; i < OSLESS_MALLOC_POOL_SIZE_BY_KB; i ++ )
		{
//			RETAILMSG(1, (L"OAM_Free : i=%d, memarray[%d].startAddress=0x%x \n", i, i, memarray[i].startAddress));
			if ( memarray[i].startAddress == (UINT32)(pMem) )
			{
//				RETAILMSG(1, (L"OAM_Free : memarray[%d].startAddress=0x%x, memarray[%d].size=0x%x \r\n", i, memarray[i].startAddress, i, memarray[i].size));
//				RETAILMSG(1, (L"OAM_Free : memarray[%d].size/1024 = 0x%x \n", i, memarray[i].size/1024));

				arraynum = ((UINT32)(pMem) - (UINT32)(&(aMemBuf[0])))/1024;
				for ( j = 0; j < memarray[i].size/1024; j++ )
				{
//					RETAILMSG(1, (L"OAM_Free : j=%d, memAddr[%d]=0x%x \n", j, arraynum, memAddr[arraynum]));
					memAddr[arraynum] = FREE;
					arraynum++;
				}

				memarray[i].startAddress	= 0;
				memarray[i].size			= 0;

//				RETAILMSG(1, (L"OAM_Free : i=%d \r\n"));

				break;
			}
		}
	}

//	_OAM_ASSERT(0);
#endif
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*  OAM_Memcpy                                                               */
/* DESCRIPTION                                                               */
/*      This function copies data from source to destination.                */
/*                                                                           */
/* PARAMETERS                                                                */
/*  pDst        Destination array Pointer to be copied                       */
/*  pSrc        Source data allocated Pointer                                */
/*  nLen        length to be copied                                          */
/*                                                                           */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/*                                                                           */
/* NOTES                                                                     */
/*      This function is called by function that wants to copy source buffer */
/*  to destination buffer.                                                   */
/*                                                                           */
/*****************************************************************************/
VOID
OAM_Memcpy(VOID *pDst, VOID *pSrc, UINT32 nLen)
{
	register INT32 	nCnt;
	register UINT8	*pD8, *pS8;
	register INT32	nL = nLen;
	register UINT32	*pD32, *pS32;

	pD8 = (UINT8*)pDst;
	pS8 = (UINT8*)pSrc;

	if ( ((INT32)pD8 % sizeof(UINT32)) == ((INT32)pS8 % sizeof(UINT32)) )
	{
		while ( (INT32)pD8 % sizeof(UINT32) )
		{
			*pD8++ = *pS8++;
			nL--;

			if( nL <= 0 )
			    return;
		}

		pD32 = (UINT32*)pD8;
		pS32 = (UINT32*)pS8;

		for (nCnt = 0; nCnt <(INT32)(nL / sizeof(UINT32)); nCnt++)
			*pD32++ = *pS32++;

		pD8 = (UINT8*)pD32;
		pS8 = (UINT8*)pS32;

		while( nL % sizeof(UINT32) )
		{
			*pD8++ = *pS8++;
			nL--;
		}
	}
	else
	{
		for( nCnt = 0; nCnt < nL; nCnt++)
			*pD8++ = *pS8++;
	}
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*  OAM_Memset                                                               */
/* DESCRIPTION                                                               */
/*      This function set data of specific buffer.                           */
/*                                                                           */
/* PARAMETERS                                                                */
/*  pSrc        Source data allocated Pointer                                */
/*  nV          Value to be setted                                           */
/*  nLen        length to be setted                                          */
/*                                                                           */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/*                                                                           */
/* NOTES                                                                     */
/*      This function is called by function that wants to set source buffer  */
/*  own data.                                                                */
/*                                                                           */
/*****************************************************************************/
VOID
OAM_Memset(VOID *pDst, UINT8 nV, UINT32 nLen)
{
	register UINT8	*pD8;
	register UINT32	*pD32;
	register UINT8	nV8 = nV;
	register UINT32	nV32 = (UINT32)( nV << 24 | nV << 16 | nV << 8 | nV );
	register INT32	nL = (INT32)nLen;
	register UINT32 nCnt;

	pD8 = (UINT8*)pDst;

	while ( (INT32)pDst % sizeof(UINT32) )
	{
		*pD8++ = nV8;
		nL--;

		if( nL <= 0 )
		    return;
	}

	pD32 = (UINT32*)pD8;
	for (nCnt = 0; nCnt <(INT32)(nL / sizeof(UINT32)); nCnt++)
		*pD32++ = nV32;

	pD8 = (UINT8*)pD32;
	while( nL % sizeof(UINT32) )
	{
		*pD8++ = nV8;
		nL--;
	}
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*  OAM_Assert                                                               */
/* DESCRIPTION                                                               */
/*     assert check									 	                     */
/*                                                                           */
/* PARAMETERS                                                                */
/*  bVal    check boolean value									             */
/*  szFile  file name												         */
/*  nLine   line count											             */
/*                                                                           */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/*                                                                           */
/* NOTES                                                                     */
/*      This function is used when system can not support debug              */
/*  print function                                                           */
/*                                                                           */
/*****************************************************************************/
VOID
OAM_Assert(BOOL32 bVal, const TCHAR *szFile, UINT32 nLine)
{
	if(!bVal)
	{
        WMR_RTL_PRINT((TEXT("[ERR] OAM_Assert Error [%s:%d]!\r\n"), szFile, nLine));
        WMR_RTL_PRINT((TEXT("\r\n<log P1=\"100\" P2=\"OSLessWMROAM\" P3=\"\" P4=\"OAM_Assert(%s:%d)\" />\r\n"), szFile, nLine));
		while(1);
	}

}


VOID
MMT_Assert(BOOL32 bVal, const TCHAR *szFile, UINT32 nLine)
{
    if(!bVal)
    {
        WMR_RTL_PRINT((TEXT("[MMT:ERR] MMT_Assert Error [%s:%d]!\r\n"), szFile, nLine));
        WMR_RTL_PRINT((TEXT("\r\n<log P1=\"100\" P2=\"POR\" P3=\"\" P4=\"MMT_Assert(%s:%d)\" />\r\n"), szFile, nLine));

        while(1);
	}

}


void
OAM_Init()
{
	UINT32 nIdx;

	for (nIdx = 0; nIdx < WMR_LOCAL_MEM_SIZE; nIdx++)
	{
		aMemBuf[nIdx]=0;
	}

	nMallocPtr = 0;
}
