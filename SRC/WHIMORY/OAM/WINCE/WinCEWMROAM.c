/*****************************************************************************/
/*                                                                           */
/* PROJECT : Rainbow	                                                     */
/* MODULE  : OS Adaptation Module                                            */
/* NAME    : WinCE OAM                                                       */
/* FILE    : WinCEWMROAM.c                                                   */
/* PURPOSE : This file contain the OS Adaptation Modules for WinCE platform  */
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
#include "WinCEWMROAM.h"

#include <stdarg.h>
#include <stdio.h>

//#include "S5L8700.h"

/*****************************************************************************/
/* Global variables definitions                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Local #defines                                                            */
/*****************************************************************************/
#define		WMR_LOCAL_MEM_SIZE		((19 * 1024) / sizeof(UINT32))

/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local constant definitions                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/
//static UINT32  aMemBuf[WMR_LOCAL_MEM_SIZE];
//static UINT32  nMallocPtr = 0;

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
// hmseo-061017 start
    if (nSize < 64) nSize = 64;     /* I don't know.. Why this is needed */
    return (void *)malloc(nSize);
// hmseo-061017 end
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
	if (pMem != NULL) free(pMem);
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
// hmseo-061017 start
	memcpy((void *) pDst,(void *) pSrc, nLen);
// hmseo-061017 end
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
// hmseo-061017 start
	memset((void *) pDst, (int) nV, nLen);
// hmseo-061017 end
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
#define RETAILMSG(cond, printf_exp)	((cond)?(NKDbgPrintfW printf_exp),1:0)

VOID
OAM_Assert(BOOL32 bVal, const TCHAR *szFile, UINT32 nLine)
{
	if(!bVal)
	{
        RETAILMSG(TRUE, (_T("[ERR] OAM_Assert Error [%s:%d]!\r\n"), szFile, nLine));
        WMR_RTL_PRINT((TEXT("\r\n<log P1=\"100\" P2=\"WinCEWMROAM\" P3=\"\" P4=\"OAM_Assert(%s:%d)\" />\r\n"), szFile, nLine));

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
// hmseo-061017 start
//	UINT32 nIdx;

//	for (nIdx = 0; nIdx < WMR_LOCAL_MEM_SIZE; nIdx++)
//	{
//		aMemBuf[i]=0;
//	}

//	nMallocPtr = 0;
// hmseo-061017 end
}
