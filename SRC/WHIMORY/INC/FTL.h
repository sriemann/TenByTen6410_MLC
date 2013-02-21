/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : FTL				                                         */
/* NAME    	   : FTL header file			                                 */
/* FILE        : FTL.h		                                                 */
/* PURPOSE 	   : This file contains the definition and protypes of exported  */
/*           	 functions for FTL. 				                         */
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
/*   19-JUL-2005 [Jaesung Jung] : first writing                              */
/*                                                                           */
/*****************************************************************************/

#ifndef _FTL_H_
#define _FTL_H_

/*****************************************************************************/
/* Return value of FTL_XXX()                                                 */
/*****************************************************************************/
#define     FTL_SUCCESS                     WMR_RETURN_VALUE(0, 0x0000, 0x0000)
#define     FTL_CRITICAL_ERROR              WMR_RETURN_VALUE(1, 0x0001, 0x0000)
#define 	FTL_USERDATA_ERROR              WMR_RETURN_VALUE(1, 0x0002, 0x0000)
#define		FTL_OUT_OF_RANGE				WMR_RETURN_VALUE(1, 0x0003, 0x0000)

/*****************************************************************************/
/* exported function prototype of FTL                                        */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32	FTL_Init				(VOID);
INT32	FTL_Format				(VOID);
INT32	FTL_Open				(UINT32 *pTotalScts);

INT32	FTL_Read				(UINT32	 nLsn,	UINT32  nNumOfScts,	UINT8 *pBuf);
INT32	FTL_Write				(UINT32	 nLsn,	UINT32  nNumOfScts,	UINT8 *pBuf);
INT32	FTL_Delete				(UINT32	 nLsn,	UINT32  nNumOfScts);

#if (WMR_READ_RECLAIM)
INT32   FTL_Scan                (UINT32  mode);
INT32	FTL_ReadReclaim		    (VOID);
#endif

VOID	FTL_AutoWearLevel		(BOOL32 bEnable);
VOID	FTL_RunSync            (BOOL32 bEnable);
BOOL32	FTL_GarbageCollect		(VOID);
INT32	FTL_Close				(VOID);

#if (WMR_MLC_LSB_RECOVERY && WMR_MLC_LSB_SQA)
VOID	 FTL_Write_LSB_SQA		(UINT32	 nLsn,	UINT32  nNumOfScts,	UINT8 *pBuf);
#endif

INT32	FTL_FormatProc(VOID);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FTL_H_ */
