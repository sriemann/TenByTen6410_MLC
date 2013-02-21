/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStore II                                                  */
/* FILE    : HALWrapper.h                                                    */
/* PURPOSE : This file contains a wrapper interface of WinCE HAL             */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2006 SAMSUNG ELECTRONICS CO., LTD.                */
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
/*   17-OCT-2006 [Seungkyu Kim]: first writing                               */
/*                                                                           */
/*****************************************************************************/

#include <windows.h>
#include <WMRTypes.h>
#include <config.h>
#include <WMRConfig.h>
#include <VFLBuffer.h>
#include <WMRTypes.h>
#include <VFL.h>
#include <HALWrapper.h>
#include <FTL.h>
#include <FIL.h>
#include <WMR_Utils.h>
#include <bibdrvinfo.h>

static BOOL		bAllreadyFTLOpen = FALSE;
static UINT32	TotalScts = 0;

extern VOID GetNandInfo(NAND_INFO *pNandInfo);

/*****************************************************************************/
/* Debug Definitions                                                         */
/*****************************************************************************/
#define HALWP_RTL_PRINT(x)           PSII_RTL_PRINT(x)

#if HALWP_ERR_MSG_ON
#define HALWP_ERR_PRINT(x)           PSII_RTL_PRINT(x)
#else
#define HALWP_ERR_PRINT(x)
#endif /* #if HALWP_ERR_MSG_ON */

#if HALWP_LOG_MSG_ON
#define HALWP_LOG_PRINT(x)           PSII_RTL_PRINT(x)
#else
#define HALWP_LOG_PRINT(x)
#endif  /* #if HALWP_LOG_MSG_ON */

#if HALWP_INF_MSG_ON
#define HALWP_INF_PRINT(x)           PSII_DBG_PRINT(x)
#else
#define HALWP_INF_PRINT(x)
#endif  /* #if HALWP_INF_MSG_ON */

/*****************************************************************************/
/* Code Implementation                                                       */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      PSII_HALWrapper                                                      */
/* DESCRIPTION                                                               */
/*      free all resources associated with the specified disk                */
/* PARAMETERS                                                                */
/*      pPacket                                                              */
/*          Pointer to packet                                                */
/*      pInOutBuf                                                            */
/*          Pointer to Input/Output buffer                                   */
/*      pResult                                                              */
/*          Pointer to result                                                */
/* RETURN VALUES                                                             */
/*      If it successes, it returns TRUE. otherwize it returns FALSE         */
/*                                                                           */
/*****************************************************************************/
UINT32
PSII_HALWrapper(VOID *pPacket, VOID *pInOutBuf, UINT32 *pResult)
{
	FTLPacket *pstInPacketFTL = NULL;
	VFLPacket *pstInPacketVFL = NULL;
	UINT32     nCtrlCode;
	UINT32     nLsn = 0;
	UINT32     nNumOfScts = 0;
	UINT8     *pBuf = 0;
	UINT32    *pTotalScts = 0;

	UINT32     nVbn;
	UINT32     nVpn;
	UINT32     nSrcVpn;
	UINT32     nDesVpn;
	BOOL32     bCleanChk;
	BUFType    eType;
	Buffer    *pbBuf;
/*
	UINT16     *pFTLCxtVbn = NULL;
*/

	HALWP_LOG_PRINT((TEXT("[HALWP: IN] ++PSII_HALWrapper()\r\n")));

	pstInPacketFTL = (FTLPacket*)pPacket;

	if (pstInPacketFTL->nCtrlCode <= PM_HAL_FIL_INIT)
	{
		HALWP_LOG_PRINT((TEXT("[HALWP:LOG]  VFL packet\r\n")));
		pstInPacketFTL = NULL;
		pstInPacketVFL = (VFLPacket*)pPacket;

		nCtrlCode = pstInPacketVFL->nCtrlCode;
		nVbn      = pstInPacketVFL->nVbn;
		nVpn      = pstInPacketVFL->nVpn;
		pbBuf      = pstInPacketVFL->pBuf;
		nSrcVpn   = pstInPacketVFL->nSrcVpn;
		nDesVpn   = pstInPacketVFL->nDesVpn;
		bCleanChk = pstInPacketVFL->bCleanCheck;
		eType     = pstInPacketVFL->eType;
	}
	else
	{
		HALWP_LOG_PRINT((TEXT("[HALWP:LOG]  FTL packet\r\n")));
		nCtrlCode  = pstInPacketFTL->nCtrlCode;
		nLsn       = pstInPacketFTL->nLsn;
		nNumOfScts = pstInPacketFTL->nNumOfScts;
		pBuf       = pstInPacketFTL->pBuf;
		pTotalScts = pstInPacketFTL->pTotalScts;
	}

	HALWP_LOG_PRINT((TEXT("[HALWP:LOG]  nCtrlCode   = %d\r\n"), nCtrlCode));
	HALWP_LOG_PRINT((TEXT("[HALWP:LOG]  nLsn        = %d(0x%x)\r\n"), nLsn, nLsn));
	HALWP_LOG_PRINT((TEXT("[HALWP:LOG]  nNumOfScts  = %d\r\n"), nNumOfScts));
	HALWP_LOG_PRINT((TEXT("[HALWP:LOG]  pBuf        = 0x%x\r\n"), (UINT32)pBuf));
	//HALWP_LOG_PRINT((TEXT("[HALWP:LOG]  pTotalScts  = %d(0x%x)\r\n"), *pTotalScts,(UINT32)pTotalScts));

	switch (nCtrlCode)
	{
	case PM_HAL_VFL_INIT:
		HALWP_RTL_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_VFL_INIT\r\n")));
		*pResult = (UINT32)VFL_Init();
		pInOutBuf = NULL;
		break;

	case PM_HAL_VFL_OPEN:
		HALWP_RTL_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_VFL_OPEN\r\n")));
		*pResult = (UINT32)VFL_Open();
		pInOutBuf = NULL;
		break;

	case PM_HAL_FIL_INIT:
		HALWP_RTL_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FIL_INIT\r\n")));
		*pResult = (UINT32)FIL_Init();
		pInOutBuf = NULL;
		break;

	case PM_HAL_FIL_GET_NANDINFO:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_INIT\r\n")));
		GetNandInfo((NAND_INFO *)pInOutBuf);
		break;

	case PM_HAL_FTL_INIT:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_INIT\r\n")));
		*pResult = (UINT32)FTL_Init();
		pInOutBuf = NULL;
		break;

	case PM_HAL_FTL_OPEN:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode   = PM_HAL_FTL_OPEN\r\n")));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  pTotalScts  = %d\r\n"), *pTotalScts));
		if ( pTotalScts != NULL )
		{
    		if ( bAllreadyFTLOpen == FALSE )
    		{
    			*pResult = (UINT32)FTL_Open(pTotalScts);
    			if ( *pResult == FTL_SUCCESS )
    			{
    				bAllreadyFTLOpen = TRUE;
    				TotalScts = *pTotalScts;
    			}
    		}
    		else
    		{
    			*pTotalScts = TotalScts;
    			*pResult = FTL_SUCCESS;
    			HALWP_ERR_PRINT((TEXT("[HALWP:INF]  TotalScts  = %d\r\n"), TotalScts));
    		}
    	}
    	else
    	{
   			HALWP_ERR_PRINT((TEXT("[HALWP:INF]  pTotalScts is NULL\r\n")));
   			*pResult = FTL_CRITICAL_ERROR;
    	}
		pInOutBuf = NULL;
		break;

	case PM_HAL_FTL_FORMAT:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_FORMAT\r\n")));
		*pResult = (UINT32)FTL_Format();
		pInOutBuf = NULL;
		break;

	case PM_HAL_VFL_READ:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_VFL_READ\r\n")));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nVpn      = %d(0x%x)\r\n"), nVpn));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  pBuf      = 0x%x\r\n"), (UINT32)pBuf));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  bCleanChk = 0x%x\r\n"), (UINT32)bCleanChk));
		*pResult = (UINT32)VFL_Read(nVpn,(Buffer*)pbBuf,bCleanChk);
		break;


	case PM_HAL_FTL_READ:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_READ\r\n")));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nLsn      = %d\r\n"), nLsn));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nNumOfScts= %d\r\n"), nNumOfScts));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  pBuf      = 0x%x\r\n"), (UINT32)pBuf));
		*pResult = (UINT32)FTL_Read(nLsn,nNumOfScts,(UINT8*)pBuf);
		pInOutBuf = NULL;
		break;


#if (WMR_READ_RECLAIM)
	case PM_HAL_FTL_SCAN:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_SCAN\r\n")));
		// nLsn number is SCAN_FS or SCAN_OS
		*pResult = (UINT32)FTL_Scan(nLsn);
		pInOutBuf = NULL;
		break;


	case PM_HAL_FTL_READRECLAIM:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_READRECLAIM\r\n")));
		*pResult = (UINT32)FTL_ReadReclaim();
		pInOutBuf = NULL;
		break;
#endif

	case PM_HAL_FTL_WRITE:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_WRITE\r\n")));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nLsn      = %d\r\n"), nLsn));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nNumOfScts= %d\r\n"), nNumOfScts));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  pBuf      = 0x%x\r\n"), (UINT32)pBuf));
		*pResult = (UINT32)FTL_Write(nLsn,nNumOfScts,(UINT8*)pBuf);
		pInOutBuf = NULL;
		break;

	case PM_HAL_FTL_DELETE:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_DELETE\r\n")));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nLsn      = %d\r\n"), nLsn));
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nNumOfScts= %d\r\n"), nNumOfScts));
		*pResult = (UINT32)FTL_Delete(nLsn,nNumOfScts);
		pInOutBuf = NULL;
		break;

	case PM_HAL_FTL_CLOSE:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_CLOSE\r\n")));
		*pResult = (UINT32)FTL_Close();
		pInOutBuf = NULL;
		bAllreadyFTLOpen = FALSE;
		TotalScts = 0;
		break;

	case PM_HAL_FTL_GARBAGECOLLECT:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_GARBAGECOLLECT\r\n")));
		*pResult = (UINT32)FTL_GarbageCollect();
		pInOutBuf = NULL;
		break;

	case PM_HAL_FTL_FORMATPROC:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_FTL_FORMATPROC\r\n")));
		//        *pResult = (UINT32)FTL_FormatProc();
		pInOutBuf = NULL;
		break;

	case PM_HAL_WMR_FORMAT_FIL:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_WMR_FORMAT_FIL\r\n")));
		*pResult = (UINT32)WMR_Format_FIL();
		pInOutBuf = NULL;
		break;

	case PM_HAL_WMR_FORMAT_VFL:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_WMR_FORMAT_VFL\r\n")));
		*pResult = (UINT32)WMR_Format_VFL();
		pInOutBuf = NULL;
		break;

	case PM_HAL_WMR_FORMAT_FTL:
		HALWP_INF_PRINT((TEXT("[HALWP:INF]  nCtrlCode = PM_HAL_WMR_FORMAT_FTL\r\n")));
		*pResult = (UINT32)WMR_Format_FTL();
		pInOutBuf = NULL;
		break;

	default:
		return FALSE;
	}

	HALWP_LOG_PRINT((TEXT("[HALWP:OUT] --PSII_HALWrapper()\r\n")));

	return TRUE;
}
