/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : FIL				                                         */
/* NAME    	   : Flash Interface Layer header file						     */
/* FILE        : FIL.h			                                             */
/* PURPOSE 	   : This file contains the definition and protypes of exported  */
/*           	 functions for FIL of Whimory.							     */
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

#ifndef _FIL_H_
#define	_FIL_H_

/*****************************************************************************/
/* Return value of FIL_XXX() & physical interface function pointer           */
/*****************************************************************************/
#define     FIL_SUCCESS                     WMR_RETURN_VALUE(0, 0x0000, 0x0000)
#define     FIL_SUCCESS_CLEAN               WMR_RETURN_VALUE(0, 0x0000, 0x0001)
#define     FIL_CRITICAL_ERROR              WMR_RETURN_VALUE(1, 0x0001, 0x0000)
#define     FIL_U_ECC_ERROR		    WMR_RETURN_VALUE(1, 0x0002, 0x0000)
#define     FIL_SYNC_FAIL		    WMR_RETURN_VALUE(1, 0x0003, 0x0000)

/*****************************************************************************/
/* FIL Function Table Data Structures                                        */
/*****************************************************************************/
typedef struct {
    UINT32  nType;              /* 0 : OneNAND, 1 : read ID from NAND directly  */
                                /* 2 : read ID from register of NAND controller */
    UINT32  nAddrOfCmdReg;      /* Address of command register                  */
    UINT32  nAddrOfAdrReg;      /* Address of address register                  */
    UINT32  nAddrOfReadIDReg;   /* Address of register for reading ID           */
    UINT32  nAddrOfStatusReg;   /* Address of status register                   */
    UINT32  nCmdOfReadID;       /* Command of reading Device ID                 */
    UINT32  nCmdOfReadPage;     /* Command of read page                         */ 
    UINT32  nCmdOfReadStatus;   /* Command of read status                       */
    UINT32  nMaskOfRnB;         /* Mask value for Ready or Busy status          */
} FILPlatformInfo;

typedef struct {

	INT32 (*Init)		(VOID);
	VOID  (*Reset)	(UINT32 nBank);
	INT32 (*Read)	(UINT32 nBank, UINT32 nPpn,    UINT32 nSctBitmap, UINT32 nPlaneBitmap,
					 UINT8 *pDBuf, UINT8 *pSBuf,   BOOL32 bECCIn,	BOOL32 bCleanCheck);
	VOID  (*Write)	(UINT32 nBank, UINT32 nPpn,    UINT32 nSctBitmap, UINT32 nPlaneBitmap,
					 UINT8 *pDBuf, UINT8 *pSBuf);
	VOID  (*Erase)	(UINT32 nBank, UINT32 nPbn,    UINT32 nPlaneBitmap);
	INT32 (*Sync)	(UINT32 nBank, UINT32 *nPlaneBitmap);

#if (WMR_SUPPORT_META_WEAR_LEVEL && WMR_SIMULATION)
	VOID  (*Swap)           (UINT32 nBank, UINT32 nMetaPbn, UINT32 nDataPbn, UINT32 nMetaBlkBitmap, UINT32 nDataBlkBitmap);
#endif

	VOID  (*Steploader_Write)	(UINT32 nBank, UINT32 nPpn,    UINT32 nSctBitmap, UINT32 nPlaneBitmap,
					 UINT8 *pDBuf, UINT8 *pSBuf);

	VOID  (*GetPlatformInfo)    (FILPlatformInfo* pstFILPlatformInfo);

} LowFuncTbl;

/*****************************************************************************/
/* Exported Function Prototype of FIL                                        */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32    	 FIL_Init         (VOID);
LowFuncTbl	*FIL_GetFuncTbl	  (VOID);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FIL_H_ */
