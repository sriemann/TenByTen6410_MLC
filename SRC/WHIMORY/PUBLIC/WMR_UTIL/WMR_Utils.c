#include <WMRConfig.h>
#include <WMRTypes.h>
#include <OSLessWMROAM.h>
#include <FTL.h>
#include <VFLBuffer.h>
#include <VFL.h>
#include <FIL.h>

#define WMR_UTIL_LOG(printf_exp)	WMR_RTL_PRINT(printf_exp)
#define WMR_UTIL_INF(printf_exp)	WMR_RTL_PRINT(printf_exp)
#define WMR_UTIL_ERR(printf_exp)	WMR_RTL_PRINT(printf_exp)

// Erase All Block except Initial Bad Block
// You must close FTL and VFL layer (FTL_Close(), VFL_Close())
BOOL32 WMR_Format_FIL(void)
{
	UINT32 nBank, nBlock, nPage = 0;
	UINT8 pSBuf[512];
	LowFuncTbl *pLowFuncTbl;
	UINT32 nSyncRet;

	WMR_UTIL_LOG((TEXT("[WMR    ] ++WMR_Format_FIL()\r\n")));

#if	0
	if (FTL_Close() == FTL_SUCCESS)
	{
		WMR_UTIL_LOG((TEXT("[WMR    ] WMR_Format_FIL() : FTL_Close() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_FIL() : FTL_Close() Failure\r\n")));
		return FALSE32;
	}

	if (VFL_Close() == VFL_SUCCESS)
	{
		WMR_UTIL_LOG((TEXT("[WMR    ] WMR_Format_FIL() : VFL_Close() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_FIL() : VFL_Close() Failure\r\n")));
		return FALSE32;
	}
#endif

	pLowFuncTbl = FIL_GetFuncTbl();

	for (nBank=0; nBank<BANKS_TOTAL; nBank++)
	{
		for (nBlock=0; nBlock<BLOCKS_PER_BANK; nBlock++)
		{
			if ( IS_MLC )
			{
				nPage = nBlock*PAGES_PER_BLOCK+(PAGES_PER_BLOCK - 1);
			}
			else if ( IS_SLC )
			{
				nPage = nBlock*PAGES_PER_BLOCK;
			}
			IS_CHECK_SPARE_ECC = FALSE32;
			pLowFuncTbl->Read(nBank, nPage, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, NULL, pSBuf, TRUE32, FALSE32);
			IS_CHECK_SPARE_ECC = TRUE32;

			if (TWO_PLANE_PROGRAM == TRUE32)
			{
				if ((pSBuf[0] == 0xff) && (pSBuf[BYTES_PER_SPARE_PAGE] == 0xff))
				{
					pLowFuncTbl->Erase(nBank, nBlock, enuBOTH_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
				}
				else if ((pSBuf[0] != 0xff) && (pSBuf[BYTES_PER_SPARE_PAGE] == 0xff))
				{
					WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FIL() : Initial Bad @ 1st plane of Block %d\r\n"), nBlock));
					pLowFuncTbl->Erase(nBank, nBlock, enuRIGHT_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
				}
				else if ((pSBuf[0] == 0xff) && (pSBuf[BYTES_PER_SPARE_PAGE] != 0xff))
				{
					WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FIL() : Initial Bad @ 2nd plane of Block %d\r\n"), nBlock));
					pLowFuncTbl->Erase(nBank, nBlock, enuLEFT_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
				}
				else
				{
					WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FIL() : Initial Bad @ Both plane of Block %d\r\n"), nBlock));
#if 0 // ksk dbg
					pLowFuncTbl->Erase(nBank, nBlock, enuBOTH_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
#endif
				}
			}
			else
			{
#if 0  // ksk dbg
				if (pSBuf[0] != 0xff)
				{
					WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FIL() : Initial Bad @ Block %d\r\n"), nBlock));
				}
				else
#endif
				{
					pLowFuncTbl->Erase(nBank, nBlock, enuBOTH_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
				}
			}
		}

		WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FIL() : All Block in the Bank %d Erased\r\n"), nBank));
	}

	WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FIL() : All Block Erased including Block 0 !!!\r\n")));

	WMR_UTIL_LOG((TEXT("[WMR    ] --WMR_Format_FIL()\r\n")));

	return TRUE32;
}

// Erase All Block except Initial Bad Block
// You must close FTL and VFL layer (FTL_Close(), VFL_Close())
BOOL32 WMR_ERASE_VFLAREA(void)
{
	UINT32 nBank, nBlock, nPage;
	UINT8 pSBuf[512];
	LowFuncTbl *pLowFuncTbl;
	UINT32 nSyncRet;

	WMR_UTIL_LOG((TEXT("[WMR    ] ++WMR_ERASE_VFLAREA()\r\n")));

#if	0
	if (FTL_Close() == FTL_SUCCESS)
	{
		WMR_UTIL_LOG((TEXT("[WMR    ] WMR_ERASE_VFLAREA() : FTL_Close() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_ERASE_VFLAREA() : FTL_Close() Failure\r\n")));
		return FALSE32;
	}

	if (VFL_Close() == VFL_SUCCESS)
	{
		WMR_UTIL_LOG((TEXT("[WMR    ] WMR_ERASE_VFLAREA() : VFL_Close() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_ERASE_VFLAREA() : VFL_Close() Failure\r\n")));
		return FALSE32;
	}
#endif

	pLowFuncTbl = FIL_GetFuncTbl();

	for (nBank=0; nBank<BANKS_TOTAL; nBank++)
	{
		for (nBlock=SPECIAL_AREA_START; nBlock<BLOCKS_PER_BANK; nBlock++)
		{
			nPage = nBlock*PAGES_PER_BLOCK+(PAGES_PER_BLOCK - 1);
			IS_CHECK_SPARE_ECC = FALSE32;
			pLowFuncTbl->Read(nBank, nPage, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, NULL, pSBuf, TRUE32, FALSE32);
			IS_CHECK_SPARE_ECC = TRUE32;

			if (TWO_PLANE_PROGRAM == TRUE32)
			{
				if ((pSBuf[0] == 0xff) && (pSBuf[BYTES_PER_SPARE_PAGE] == 0xff))
				{
					pLowFuncTbl->Erase(nBank, nBlock, enuBOTH_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
				}
				else if ((pSBuf[0] != 0xff) && (pSBuf[BYTES_PER_SPARE_PAGE] == 0xff))
				{
					WMR_UTIL_INF((TEXT("[WMR:INF] WMR_ERASE_VFLAREA() : Initial Bad @ 1st plane of Block %d\r\n"), nBlock));
					pLowFuncTbl->Erase(nBank, nBlock, enuRIGHT_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);

				}
				else if ((pSBuf[0] == 0xff) && (pSBuf[BYTES_PER_SPARE_PAGE] != 0xff))
				{
					WMR_UTIL_INF((TEXT("[WMR:INF] WMR_ERASE_VFLAREA() : Initial Bad @ 2nd plane of Block %d\r\n"), nBlock));
					pLowFuncTbl->Erase(nBank, nBlock, enuLEFT_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
				}
				else
				{
					WMR_UTIL_INF((TEXT("[WMR:INF] WMR_ERASE_VFLAREA() : Initial Bad @ Both plane of Block %d\r\n"), nBlock));
				}
			}
			else
			{
					pLowFuncTbl->Erase(nBank, nBlock, enuBOTH_PLANE_BITMAP);
					pLowFuncTbl->Sync(nBank, &nSyncRet);
			}
		}

		WMR_UTIL_INF((TEXT("[WMR:INF] WMR_ERASE_VFLAREA() : All Block in the Bank %d Erased\r\n"), nBank));
	}

	WMR_UTIL_INF((TEXT("[WMR:INF] WMR_ERASE_VFLAREA() : All Block Erased including Block 0 !!!\r\n")));

	WMR_UTIL_LOG((TEXT("[WMR    ] --WMR_ERASE_VFLAREA()\r\n")));

	return TRUE32;
}

// You must close FTL and VFL layer (FTL_Close(), VFL_Close())
// and initialize VFL (VFL_Init())
BOOL32 WMR_Format_VFL(void)
{
	LowFuncTbl *pLowFuncTbl = NULL;
	UINT32 *pTemp = NULL;
	UINT32 nSyncRet = 0;
#ifdef	NO_MALLOC	// by dodan2 061216
	UINT8 pDBuf[8192];
	//UINT8 pSBuf[256]; // spare size has to be bigger to 512 for 8Bit ECC
	UINT8 pSBuf[512];
#else
	UINT8 *pDBuf = NULL;
	UINT8 *pSBuf = NULL;
#endif

	WMR_UTIL_LOG((TEXT("[WMR    ] ++WMR_Format_VFL()\r\n")));

#if	0
	if (FTL_Close() == FTL_SUCCESS)
	{
		WMR_UTIL_LOG((TEXT("[WMR    ] WMR_Format_VFL() : FTL_Close() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_VFL() : FTL_Close() Failure\r\n")));
		return FALSE32;
	}

	if (VFL_Close() == VFL_SUCCESS)
	{
		WMR_UTIL_LOG((TEXT("[WMR    ] WMR_Format_VFL() : VFL_Close() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_VFL() : VFL_Close() Failure\r\n")));
		return FALSE32;
	}
#endif

	pLowFuncTbl = FIL_GetFuncTbl();

	WMR_ERASE_VFLAREA();

	if (VFL_Init() == VFL_SUCCESS)
	{
		WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_VFL() : VFL_Init() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_VFL() : VFL_Init() Failure\r\n")));
		return FALSE32;
	}

	if (VFL_Format() == VFL_SUCCESS)
	{
		WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_VFL() : VFL_Format() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_VFL() : VFL_Format() Failure\r\n")));
		return FALSE32;
	}

#ifndef	NO_MALLOC	// by dodan2 061216
	pDBuf = (UINT8 *)WMR_MALLOC(BYTES_PER_MAIN_SUPAGE);
	pSBuf = (UINT8 *)WMR_MALLOC(BYTES_PER_SPARE_SUPAGE);
	if (pDBuf == NULL || pSBuf == NULL)
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_VFL() : malloc() Failure\r\n")));
		return FALSE;
	}
#endif

	WMR_MEMSET((void *)pDBuf, 0xff, BYTES_PER_MAIN_SUPAGE);
	WMR_MEMSET((void *)pSBuf, 0xff, BYTES_PER_SPARE_SUPAGE);

	pTemp = (UINT32 *)pDBuf;

	*pTemp= WMR_SIGNATURE;

	pLowFuncTbl->Write(0, PAGES_PER_BLOCK-1, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, pDBuf, pSBuf);

#ifndef	NO_MALLOC	// by dodan2 061216
	WMR_FREE(pDBuf);
	WMR_FREE(pSBuf);
#endif

	if (pLowFuncTbl->Sync(0, &nSyncRet) != FIL_SUCCESS)
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_VFL() : WMR_SIGNATURE write Failure]\r\n")));
		return FALSE32;
	}

	WMR_UTIL_LOG((TEXT("[WMR    ] --WMR_Format_VFL()\r\n")));

	return TRUE32;
}

// Format FTL layer
// You must close FTL layer FTL_Close()
BOOL32 WMR_Format_FTL(void)
{
	UINT32 nBlock = 0;
	UINT32 pTotalScts = 0;

	WMR_UTIL_LOG((TEXT("[WMR    ] ++WMR_Format_FTL()\r\n")));

#if	0
	if (FTL_Close() == FTL_SUCCESS)
	{
		WMR_UTIL_LOG((TEXT("[WMR    ] WMR_Format_FTL() : FTL_Close() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_FTL() : FTL_Close() Failure\r\n")));
		return FALSE32;
	}
#endif

	for (nBlock=FTL_INFO_SECTION_START; nBlock<SUBLKS_TOTAL; nBlock++)
	{
		if (VFL_Erase(nBlock) != VFL_SUCCESS)
		{
			WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_FTL() : VFL_Erase()	Failure @ Virtual Block %d\r\n"), nBlock));
		}
	}

	WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FTL() : All Virtual Block Erased (VFL) !!!\r\n")));

	if (FTL_Init() == FTL_SUCCESS)
	{
		WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FTL() : FTL_Init() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_FTL() : FTL_Init() Failure\r\n")));
		return FALSE32;
	}

	if (FTL_Format() == FTL_SUCCESS)
	{
		WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FTL() : FTL_Format() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_FTL() : FTL_Format() Failure\r\n")));
		return FALSE32;
	}

	WMR_UTIL_LOG((TEXT("[WMR    ] --WMR_Format_FTL()\r\n")));

	if (FTL_Open(&pTotalScts) == FTL_SUCCESS)
	{
		WMR_UTIL_INF((TEXT("[WMR:INF] WMR_Format_FTL() : FTL_Open() Success\r\n")));
	}
	else
	{
		WMR_UTIL_ERR((TEXT("[WMR:ERR] WMR_Format_FTL() : FTL_Open() Failure\r\n")));
		return FALSE32;
	}

	WMR_UTIL_LOG((TEXT("[WMR    ] --WMR_Format_FTL()\r\n")));

	return TRUE32;
}

