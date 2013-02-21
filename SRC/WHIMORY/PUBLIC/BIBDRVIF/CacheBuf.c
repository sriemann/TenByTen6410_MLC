/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : CacheBuf.c                                                      */
/* PURPOSE : This file implements Cache-Buffer for improving read-speed      */
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
/*                                                                           */
/*****************************************************************************/

#include <windows.h>

#include <CacheBuf.h>

//#include <OEMFlashIO.h>

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Imported variable declarations                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Imported function declarations                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local #define                                                             */
/*****************************************************************************/

#undef DEBUG_NFLAT_CACHEBUF
#ifdef DEBUG_NFLAT_CACHEBUF

	#define	DEBUG_WINCE300

	#if defined(DEBUG_WINCE300)

		#include <windows.h>

		#define	CBUF_DBG_PRINT(x)		RETAILMSG(1, x)

	#endif	//DEBUG_WINCE300

#else  /* DEBUG_NFLAT_CACHEBUF */

	#define CBUF_DBG_PRINT(x)

#endif /* DEBUG_NFLAT_CACHEBUF */

#undef	HEAP_CONSUMPTION

#undef	NFLAT_CACHE_ALGORITHM_1
#define	NFLAT_CACHE_ALGORITHM_2

/*****************************************************************************/
// Local constant definitions
/*****************************************************************************/

/*****************************************************************************/
// Local typedefs
/*****************************************************************************/

/*****************************************************************************/
// Local variables
/*****************************************************************************/

/*****************************************************************************/
// Local function prototypes
/*****************************************************************************/

/*****************************************************************************/
// Function definitions
/*****************************************************************************/

#ifdef HEAP_CONSUMPTION
static UINT giUsedHeap = 0;
#define INC_USED_HEAP(n) giUsedHeap += (n)
#else /* HEAP_COMSUMPTION */
#define INC_USED_HEAP(n)
#endif /* HEAP_COMSUMPTION */

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		NFLATCache_Init                                                      */
/* DESCRIPTION                                                               */
/*		This function opens the driver key specified by the active key       */
/* PARAMETERS                                                                */
/*		ActiveKey		Handle to a currently open key or any of the         */
/*						following predefined reserved handle values          */
/* RETURN VALUES                                                             */
/*		Return values is HKEY value of "[ActiveKey]\[Key]", The caller is    */
/*		responsible for closing the returned HKEY                            */
/*                                                                           */
/*****************************************************************************/
NFLAT_CACHEBUF *
NFLATCache_Init(
	UINT nNumOfSecCache
	)
{
	NFLAT_SEC_CACHE *pSecCache;
	NFLAT_CACHEBUF  *pCacheBuf;
	UINT nIdx;

	CBUF_DBG_PRINT((TEXT("++CBUF: NFLATCache_Init nNumOfSecCache=%d\r\n"), nNumOfSecCache));

	
	INC_USED_HEAP(sizeof(NFLAT_CACHEBUF));
	
	pCacheBuf = (NFLAT_CACHEBUF *) LocalAlloc(LPTR, sizeof(NFLAT_CACHEBUF));
	if (NULL == pCacheBuf)
	{
		CBUF_DBG_PRINT((TEXT("CBUF: ERROR: LocalAlloc pCacheBuf=0x%X"), pCacheBuf));
		return NULL;
	}
		
	pCacheBuf->nNumOfSecCache = nNumOfSecCache;

	INC_USED_HEAP(nNumOfSecCache * sizeof(NFLAT_SEC_CACHE));

	pCacheBuf->pSecCache = (NFLAT_SEC_CACHE *) LocalAlloc(LPTR,
							nNumOfSecCache * sizeof(NFLAT_SEC_CACHE));
	if (NULL == pCacheBuf->pSecCache)
	{
		CBUF_DBG_PRINT((TEXT("CBUF: ERROR: LocalAlloc pSecCache=0x%X\r\n"), pCacheBuf->pSecCache));
		LocalFree(pCacheBuf);
		return NULL;
	}

	// Initialize pSecCache[x] of pCacheBuf[].
	pSecCache = pCacheBuf->pSecCache;
	for (nIdx = 0; nIdx < nNumOfSecCache; nIdx ++)
	{
		pSecCache[nIdx].bValid = FALSE;
		pSecCache[nIdx].nSecNum = 0;
		pSecCache[nIdx].nSecHit = 0;
	}

#if defined(HEAP_CONSUMPTION)
	CBUF_DBG_PRINT((TEXT("--CBUF: Allocated memory = %d bytes\r\n"), giUsedHeap));
#endif	//HEAP_CONSUMPTION

	CBUF_DBG_PRINT((TEXT("--CBUF: NFLATCache_Init nNumOfSecCache=%d\r\n"), nNumOfSecCache));

	return pCacheBuf;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		NFLATCache_Deinit                                                    */
/* DESCRIPTION                                                               */
/*		This function opens the driver key specified by the active key       */
/* PARAMETERS                                                                */
/*		ActiveKey		Handle to a currently open key or any of the         */
/*						following predefined reserved handle values          */
/* RETURN VALUES                                                             */
/*		Return values is HKEY value of "[ActiveKey]\[Key]", The caller is    */
/*		responsible for closing the returned HKEY                            */
/*                                                                           */
/*****************************************************************************/
VOID
NFLATCache_Deinit(
	NFLAT_CACHEBUF *pCacheBuf
	)
{
	CBUF_DBG_PRINT((TEXT("++CBUF: NFLATCache_Deinit(0x%x)"), pCacheBuf));
	LocalFree(pCacheBuf->pSecCache);
	LocalFree(pCacheBuf);
	CBUF_DBG_PRINT((TEXT("--CBUF: NFLATCache_Deinit")));
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		NFLATCache_GetSecCache                                               */
/* DESCRIPTION                                                               */
/*		This function opens the driver key specified by the active key       */
/* PARAMETERS                                                                */
/*		ActiveKey		Handle to a currently open key or any of the         */
/*						following predefined reserved handle values          */
/* RETURN VALUES                                                             */
/*		Return values is HKEY value of "[ActiveKey]\[Key]", The caller is    */
/*		responsible for closing the returned HKEY                            */
/*                                                                           */
/*****************************************************************************/
NFLAT_SEC_CACHE *
NFLATCache_GetSecCache(
	NFLAT_CACHEBUF *pCacheBuf,
	UINT nSecNum
	)
{
#if defined(NFLAT_CACHE_ALGORITHM_1)
            
	register UINT nIdx;
	register UINT nNumOfSecCache;
	register NFLAT_SEC_CACHE *pSecCache;
	static UINT nAccessCnt = 0;
	static UINT nTotalHit  = 0;


	nNumOfSecCache = pCacheBuf->nNumOfSecCache;
	pSecCache      = pCacheBuf->pSecCache;
	
	nAccessCnt ++;
	
	CBUF_DBG_PRINT((TEXT("++CBUF: SECTOR(%8d), ACCESS COUNT(%8d), HIT COUNT(%4d)\r\n"),
			nSecNum, nAccessCnt, nTotalHit));
	
	for (nIdx = 0; nIdx < nNumOfSecCache; nIdx ++)
	{
		if ((TRUE == pSecCache[nIdx].bValid) &&
			(nSecNum == pSecCache[nIdx].nSecNum))
		{
			pSecCache[nIdx].nSecHit ++;
			
//			CBUF_DBG_PRINT((TEXT("++CBUF: IDX(%3d) SEC(%8d), HIT(%8d), ACC(%8d)\r\n"),
//					nIdx, nSecNum, pSecCache[nIdx].nSecHit, nAccessCnt));

			nTotalHit ++;

			return &(pSecCache[nIdx]);
		}
	}
	
//	CBUF_DBG_PRINT((TEXT("++CBUF:          SEC(%8d), NOHIT        , ACC(%8d)\r\n"),
//			nSecNum, nAccessCnt));

	return NULL;
	
#elif defined(NFLAT_CACHE_ALGORITHM_2)
	register UINT nIdx, nMaxIdx;
	register UINT nNumOfSecCache;
	register NFLAT_SEC_CACHE *pSecCache;
	register UINT nSelectedIdx;


	nNumOfSecCache = pCacheBuf->nNumOfSecCache;
	pSecCache      = pCacheBuf->pSecCache;
	nMaxIdx        = NUM_OF_SEC_CACHE_COLUMN;

	for (nIdx = 0; nIdx < nMaxIdx; nIdx ++)
	{
		nSelectedIdx = NUM_OF_SEC_CACHE_ROW * nIdx + nSecNum % NUM_OF_SEC_CACHE_ROW;
		if ((TRUE == pSecCache[nSelectedIdx].bValid) &&
			(nSecNum == pSecCache[nSelectedIdx].nSecNum))
		{
			pSecCache[nSelectedIdx].nSecHit ++;
			
			CBUF_DBG_PRINT((TEXT("++CBUF: IDX(%3d/%3d) SEC(%8d), HIT(%8d)\r\n"),
					nSelectedIdx, nSecNum % NUM_OF_SEC_CACHE_ROW, pSecCache[nSelectedIdx].nSecHit));
	
			return &(pSecCache[nSelectedIdx]);
		}
	}

	return NULL;
#endif
}

VOID
NFLATCache_AddSecCache(
	NFLAT_CACHEBUF *pCacheBuf,
	UINT nSecNum,
//	UCHAR aBuf[MAINPAGE_SIZE]
	UCHAR aBuf[PSII_SECTOR_SIZE]
	)
{
#if defined(NFLAT_CACHE_ALGORITHM_1)

	register UINT nIdx1, nIdx2;
	register UINT nNumOfSecCache;
	register NFLAT_SEC_CACHE *pSecCache;
	UINT nSelectedIdx;
	UINT nLowestSecHit;


	nNumOfSecCache = pCacheBuf->nNumOfSecCache;
	pSecCache      = pCacheBuf->pSecCache;

	nSelectedIdx = 0xFFFFFFFF;
	for (nIdx1 = 0; nIdx1 < nNumOfSecCache; nIdx1 ++)
	{
		if (FALSE == pSecCache->bValid)
		{
			nSelectedIdx = nIdx1;
			break;
		}

		pSecCache ++;
	}

	// If there is no free slot, search for SecCache[], which has the lowest nSecHit.
	if (0xFFFFFFFF == nSelectedIdx)
	{
		nSelectedIdx  = nNumOfSecCache - 1;
		nLowestSecHit = pSecCache[nSelectedIdx].nSecHit;
		nIdx2         = nNumOfSecCache - 1;
		pSecCache     = &(pCacheBuf->pSecCache[nSelectedIdx]);

		for (nIdx1 = 0; nIdx1 < nNumOfSecCache; nIdx1 ++)
		{
			if (nLowestSecHit > pSecCache->nSecHit)
			{
				nSelectedIdx = nIdx2;
				nLowestSecHit = pSecCache->nSecHit;
			}

			pSecCache --;
			nIdx2 --;
		}
	}

	pSecCache[nSelectedIdx].bValid  = TRUE;
	pSecCache[nSelectedIdx].nSecNum = nSecNum;
	pSecCache[nSelectedIdx].nSecHit = 1;
//	memcpy(pSecCache[nSelectedIdx].aBuf, aBuf, MAINPAGE_SIZE);
	memcpy(pSecCache[nSelectedIdx].aBuf, aBuf, PSII_SECTOR_SIZE);

#elif defined(NFLAT_CACHE_ALGORITHM_2)

	register UINT nIdx, nMaxIdx;
	register UINT nNumOfSecCache;
	register NFLAT_SEC_CACHE *pSecCache;
	UINT nSelectedIdx1, nSelectedIdx2;
	UINT nLowestSecHit;


	nNumOfSecCache = pCacheBuf->nNumOfSecCache;
	pSecCache      = pCacheBuf->pSecCache;
	nMaxIdx        = NUM_OF_SEC_CACHE_COLUMN;

	// search for free slot.
	nSelectedIdx1 = 0xFFFFFFFF;
	for (nIdx = 0; nIdx < nMaxIdx; nIdx ++)
	{
		nSelectedIdx2 = NUM_OF_SEC_CACHE_ROW * nIdx + nSecNum % NUM_OF_SEC_CACHE_ROW;
		if (FALSE == pSecCache[nSelectedIdx2].bValid)
		{
			nSelectedIdx1 = nSelectedIdx2;
			break;
		}
	}

	// If there is no free slot, search for SecCache[], which has the lowest nSecHit.
	if (0xFFFFFFFF == nSelectedIdx1)
	{
		nSelectedIdx1 = NUM_OF_SEC_CACHE_ROW * 0 + nSecNum % NUM_OF_SEC_CACHE_ROW;
		nLowestSecHit = pSecCache[nSelectedIdx1].nSecHit;

		for (nIdx = 1; nIdx < nMaxIdx; nIdx ++)
		{
			nSelectedIdx2 = NUM_OF_SEC_CACHE_ROW * nIdx + nSecNum % NUM_OF_SEC_CACHE_ROW;
			if (nLowestSecHit > pSecCache[nSelectedIdx2].nSecHit)
			{
				nSelectedIdx1 = nSelectedIdx2;
				nLowestSecHit = pSecCache[nSelectedIdx2].nSecHit;
			}
		}
	}

	pSecCache[nSelectedIdx1].bValid  = TRUE;
	pSecCache[nSelectedIdx1].nSecNum = nSecNum;
	pSecCache[nSelectedIdx1].nSecHit = 1;
//	memcpy(pSecCache[nSelectedIdx1].aBuf, aBuf, MAINPAGE_SIZE);
	memcpy(pSecCache[nSelectedIdx1].aBuf, aBuf, PSII_SECTOR_SIZE);

#endif
}