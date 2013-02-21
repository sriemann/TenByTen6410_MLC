/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : FIL				                                         */
/* NAME    	   : Flash Interface Layer (6410 version)                        */
/* FILE        : FIL.c		                                                 */
/* PURPOSE 	   : 															 */
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
/*   18-AUG-2005 [Yangsup Lee] : first writing	                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/
#include <WMRConfig.h>
#include <WMRTypes.h>
#include <OSLessWMROAM.h>
#include "S3C6410_FIL.h"
#include "FIL.h"
#include <bibdrvinfo.h>

#include <string.h>

/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/
PRIVATE LowFuncTbl	stLowFuncTbl;

/*****************************************************************************/
/* Code Implementation                                                       */
/*****************************************************************************/
INT32
FIL_Init(VOID)
{
	INT32 nFILRet;

	nFILRet = NAND_Init();

	if (nFILRet != FIL_SUCCESS)
	{
		return nFILRet;
	}

	stLowFuncTbl.Init = NAND_Init;
	stLowFuncTbl.Reset = NAND_Reset;
	stLowFuncTbl.Sync = NAND_Sync;
//	stLowFuncTbl.Read = NAND_Read;
	stLowFuncTbl.Read = NAND_Read_Retry;
	stLowFuncTbl.Write = NAND_Write;
	stLowFuncTbl.Erase = NAND_Erase;
	stLowFuncTbl.Steploader_Write = NAND_Steploader_Write;
	stLowFuncTbl.GetPlatformInfo = NAND_GetPlatformInfo;

	return FIL_SUCCESS;
}

LowFuncTbl*
FIL_GetFuncTbl(VOID)
{
	return &stLowFuncTbl;
}

VOID GetNandInfo(NAND_INFO *pNandInfo)
{
	pNandInfo->dwPagesPerSuBlk = PAGES_PER_SUBLK;
	pNandInfo->dwSectorsPerSuPage = SECTORS_PER_SUPAGE;
	pNandInfo->dwSpecialAreaSize = SPECIAL_AREA_SIZE;
	pNandInfo->dwSpecialAreaStart = SPECIAL_AREA_START;
}
