/*
 * fmd_stub.cpp
 */

extern "C"	// hmseo-061028
{
#include <WMRConfig.h>
#include <WMRTypes.h>

#include <OSLessWMROAM.h>
#include <FTL.h>
#include <VFLBuffer.h>
#include <VFL.h>
#include <FIL.h>
#include <WMR_Bootpart.h>

#include <fmd.h>
}


#define SpinForever()   while (1)

/*****************************************************************************/
/* Debug Print #defines                                                      */
/*****************************************************************************/
#define	EXAM_INF_MSG_ON
#define	EXAM_LOG_MSG_ON

#define     EXAM_ERR_PRINT(x)            WMR_RTL_PRINT(x)
#define     EXAM_RTL_PRINT(x)            WMR_RTL_PRINT(x)

#if defined(EXAM_LOG_MSG_ON)
#define     EXAM_LOG_PRINT(x)            WMR_DBG_PRINT(x)
#else
#define     EXAM_LOG_PRINT(x)
#endif

#if defined(EXAM_INF_MSG_ON)
#define     EXAM_INF_PRINT(x)            WMR_DBG_PRINT(x)
#else
#define     EXAM_INF_PRINT(x)
#endif


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      WMR_Init			                                                 */
/* DESCRIPTION                                                               */
/*      This function inits whimory & format & open.						 */
/* PARAMETERS                                                                */
/*      pTotalScts 	[OUT] 	the count of sectors which user can use.         */
/* RETURN VALUES                                                             */
/*		WMR_SUCCESS															 */
/*				FTL_Open is completed										 */
/*		WMR_CRITICAL_ERROR													 */
/*				FTL_Open is failed											 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
WMR_Init()
{
	Buffer *pBuf;
	INT32 	nRet;
	LowFuncTbl *pLowFuncTbl;
	UINT32	nSig;
	UINT32  nSyncRet;
//	UINT32  pTotalScts;

	nRet = FIL_Init();

	if (nRet != FIL_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	EXAM_RTL_PRINT((TEXT("[FTL:MSG] FIL_Init			[OK]\r\n")));

	nRet = BUF_Init();

	if (nRet != BUF_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	EXAM_RTL_PRINT((TEXT("[FTL:MSG] BUF_Init			[OK]\r\n")));

	nRet = VFL_Init();

	if (nRet != VFL_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	EXAM_RTL_PRINT((TEXT("[FTL:MSG] VFL_Init			[OK]\r\n")));

#if 0
	nRet = FTL_Init();

	if (nRet != FTL_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	EXAM_RTL_PRINT((TEXT("[FTL:MSG] FTL_Init			[OK]\r\n")));
#endif

	pBuf = BUF_Get(BUF_MAIN_AND_SPARE);

	if (pBuf == NULL)
	{
		return WMR_CRITICAL_ERROR;
	}

	pLowFuncTbl = FIL_GetFuncTbl();

	nRet = pLowFuncTbl->Read(0, Signature_Ppn, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP,
							 pBuf->pData, pBuf->pSpare, FALSE32, TRUE32);

#if	0
	if ( nRet == FIL_U_ECC_ERROR )		// hmseo-061028
	{
		pLowFuncTbl->Erase(0, 0, enuBOTH_PLANE_BITMAP);
		nRet = pLowFuncTbl->Read(0, Signature_Ppn, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP,
								 pBuf->pData, pBuf->pSpare, FALSE32, TRUE32);
	}	// hmseo-061028
#endif

	nSig = *(UINT32 *)(pBuf->pData);

	BUF_Release(pBuf);

	if (nRet == FIL_SUCCESS_CLEAN)
	{
		UINT32	nRetTemp;
		UINT32 *pTemp;

		EXAM_RTL_PRINT((TEXT("[FTL:MSG] Not Formated !\n")));

		nRetTemp = VFL_Format();

		if (nRetTemp != VFL_SUCCESS)
		{
			return WMR_CRITICAL_ERROR;
		}

		EXAM_RTL_PRINT((TEXT("[FTL:MSG] VFL_Format			[OK]\r\n")));
#if 0
		nRetTemp = FTL_Format();

		if (nRetTemp != FTL_SUCCESS)
		{
			return WMR_CRITICAL_ERROR;
		}

		EXAM_RTL_PRINT((TEXT("[FTL:MSG] FTL_Format			[OK]\r\n")));
#endif
		pBuf = BUF_Get(BUF_MAIN_AND_SPARE);

		if (pBuf == NULL)
		{
			return WMR_CRITICAL_ERROR;
		}

		pTemp= (UINT32 *)(pBuf->pData);

		*pTemp= WMR_SIGNATURE;

		pLowFuncTbl->Write(0, Signature_Ppn, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, pBuf->pData, pBuf->pSpare);

		BUF_Release(pBuf);

		nRetTemp = pLowFuncTbl->Sync(0, &nSyncRet);

		if (nRetTemp != FIL_SUCCESS)
		{
			return WMR_CRITICAL_ERROR;
		}

		nSig = WMR_SIGNATURE;

		EXAM_RTL_PRINT((TEXT("[FTL:MSG] Write Signature		[OK]\r\n")));

	}
	else if (nRet != FIL_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	if (nSig != WMR_SIGNATURE)
	{
		EXAM_ERR_PRINT((TEXT("[FTL:ERR] Version Fail			[OK]\r\n")));
		return WMR_CRITICAL_ERROR;
	}

	nRet = VFL_Open();

	if (nRet != VFL_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	EXAM_RTL_PRINT((TEXT("[FTL:MSG] VFL_Open			[OK]\r\n")));
#if 0
	nRet = FTL_Open(&pTotalScts);

	if (nRet != FTL_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	EXAM_RTL_PRINT((TEXT("[FTL:MSG] FTL_Open			[OK]\r\n")));

	#if (WMR_READ_RECLAIM)
	nRet = FTL_ReadReclaim();

	if (nRet != FTL_SUCCESS)
	{
		return WMR_CRITICAL_ERROR;
	}

	EXAM_RTL_PRINT((TEXT("[FTL:MSG] FTL_ReadReclaim		[OK]\r\n")));
	#endif
#endif
	return WMR_SUCCESS;
}


PVOID  FMD_Init(LPCTSTR lpActiveReg, PPCI_REG_INFO pRegIn, PPCI_REG_INFO pRegOut)
{
	if(WMR_Init()!=WMR_SUCCESS) {
	    RETAILMSG(1, (L"BP_ReadLogicalSector : WMR_Init is failed.\r\n"));
//	    SpinForever();
	}

	return((PVOID) 1);
}

BOOL  FMD_GetInfo(PFlashInfo pFlashInfo)
{
	if (pFlashInfo == NULL)
		return(FALSE);

	pFlashInfo->flashType = NAND;
	pFlashInfo->dwNumBlocks = SUBLKS_TOTAL;
	pFlashInfo->dwBytesPerBlock = PAGES_PER_SUBLK*SECTORS_PER_SUPAGE*BYTES_PER_SECTOR;
	pFlashInfo->wSectorsPerBlock = SECTORS_PER_SUBLK;
	pFlashInfo->wDataBytesPerSector = BYTES_PER_SECTOR;
	return(TRUE);
}

ULONG nCount=0;
BOOL  FMD_ReadSector (SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
	Buffer 		stBuf;
	UINT32          nVsnAlign;
	UINT32          nVsnCnt     = dwNumSectors;
	UINT32          nSctToRead;
	INT32            nRet;
	UINT32		nStartPageAddr;

//	RETAILMSG(1, (L"FMD_ReadSector: %x, %x, %x, %x\r\n", startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors));

	if ((nCount %0x100) == 0 )
	{
		RETAILMSG(1,(L"."));
	}
	nCount++;

	if ((pSectorBuff == NULL) || (pSectorInfoBuff != NULL)){
		RETAILMSG(1, (L"FMD_ReadSector : pSetorBuff null or pSectorInfoBuff not null\r\n"));
		return(FALSE);
	}

	while(nVsnCnt)
	{
		nVsnAlign = startSectorAddr % SECTORS_PER_SUPAGE;
		nStartPageAddr= startSectorAddr/SECTORS_PER_SUPAGE;

		if (nVsnAlign != 0)
		{
			if (nVsnCnt >= (SECTORS_PER_SUPAGE - nVsnAlign))
			{
				nSctToRead = SECTORS_PER_SUPAGE - nVsnAlign;
			}
			else
			{
				nSctToRead = nVsnCnt;
			}
		}
		else
		{
			if (nVsnCnt >= SECTORS_PER_SUPAGE)
			{
				nSctToRead = SECTORS_PER_SUPAGE;
			}
			else
			{
				nSctToRead = nVsnCnt;
			}
		}

		stBuf.eStatus = BUF_AUX;
		stBuf.nBank = 0;
		stBuf.pData = (UINT8 *)pSectorBuff;
		stBuf.pSpare = NULL;

		stBuf.nBitmap = ((((FULL_SECTOR_BITMAP_PAGE >> (SECTORS_PER_SUPAGE - nSctToRead))&FULL_SECTOR_BITMAP_PAGE)<<nVsnAlign)&FULL_SECTOR_BITMAP_PAGE);

		nRet = VFL_Read(nStartPageAddr,        /* Vpn              */
						&stBuf,				/* Buffer   */
						FALSE32);


		if (nRet != VFL_SUCCESS)
		{
			RETAILMSG(1, (TEXT("[BOOTPART:ERR]  VFL_Read (Vsn=%d) Error = 0x%x\r\n"), startSectorAddr, nRet));
			return FALSE;
		}

		nVsnCnt   -= nSctToRead;
		startSectorAddr += nSctToRead;
		pSectorBuff  += nSctToRead * BYTES_PER_SECTOR;
	}

	return(TRUE);
}

BOOL  FMD_WriteSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
	Buffer 		stBuf;
	UINT32          nVsnAlign;
	UINT32          nVsnCnt     = dwNumSectors;
	UINT32          nSctToWrite;
	INT32            nRet;
	UINT32		nStartPageAddr;

//	RETAILMSG(1, (L"FMD_WriteSector: %x, %x, %x, %x\r\n", startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors));

	if ((pSectorBuff == NULL) || (pSectorInfoBuff != NULL)){
		RETAILMSG(1, (L"FMD_WriteSector : pSetorBuff null or pSectorInfoBuff not null\r\n"));
		return(FALSE);
	}

	while(nVsnCnt)
	{
		nVsnAlign = startSectorAddr % SECTORS_PER_SUPAGE;

		nStartPageAddr= startSectorAddr/SECTORS_PER_SUPAGE;

		if (nVsnAlign != 0)
		{
			if (nVsnCnt >= (SECTORS_PER_SUPAGE - nVsnAlign))
			{
				nSctToWrite = SECTORS_PER_SUPAGE - nVsnAlign;
			}
			else
			{
				nSctToWrite = nVsnCnt;
			}
		}
		else
		{
			if (nVsnCnt >= SECTORS_PER_SUPAGE)
			{
				nSctToWrite = SECTORS_PER_SUPAGE;
			}
			else
			{
				nSctToWrite = nVsnCnt;
			}
		}

		stBuf.eStatus = BUF_AUX;
		stBuf.nBank = 0;
		stBuf.pData = (UINT8 *)pSectorBuff;
		stBuf.pSpare = NULL;

		stBuf.nBitmap = ((((FULL_SECTOR_BITMAP_PAGE >> (SECTORS_PER_SUPAGE - nSctToWrite))&FULL_SECTOR_BITMAP_PAGE)<<nVsnAlign)&FULL_SECTOR_BITMAP_PAGE);

		nRet = VFL_Write(nStartPageAddr,        /* Vpn              */
						&stBuf);


		if (nRet != VFL_SUCCESS)
		{
			RETAILMSG(1, (TEXT("[BOOTPART:ERR]  VFL_Write (Vsn=%d) Error = 0x%x\r\n"), startSectorAddr, nRet));
			return FALSE;
		}

		nVsnCnt   -= nSctToWrite;
		startSectorAddr += nSctToWrite;
		pSectorBuff  += nSctToWrite * BYTES_PER_SECTOR;
	}

	return(TRUE);
}

BOOL  FMD_Deinit(PVOID)
{
	return(TRUE);
}

BOOL  FMD_EraseBlock(BLOCK_ID blockID)
{
	if (VFL_Erase(blockID) != VFL_SUCCESS)
	{
		RETAILMSG(1, (TEXT("[EBOOT:ERR] VFL_Erase Error (Blk=%d:nECfgVbn)\r\n"), blockID));
		return (FALSE);
	}

	return(TRUE);
}

#if 0
BOOL  FMD_GetInfoEx(PFlashInfoEx pFlashInfo, PDWORD pdwNumRegions)
{
	return(FALSE);
}

DWORD FMD_GetBlockStatus(BLOCK_ID blockID)
{
	return(0);
}

BOOL FMD_SetBlockStatus(BLOCK_ID blockID, DWORD dwStatus)
{
	return(FALSE);
}

void  FMD_PowerUp(VOID)
{
}

VOID  FMD_PowerDown(VOID)
{
}

BOOL  FMD_OEMIoControl(DWORD dwIoControlCode, PBYTE pInBuf, DWORD nInBufSize,
                       PBYTE pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned)
{
	return(FALSE);
}
#endif
