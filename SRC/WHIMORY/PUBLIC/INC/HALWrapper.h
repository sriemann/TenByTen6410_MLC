/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : HALWrapper.h                                                    */
/* PURPOSE : This file contains a wrapper interface of WinCE HAL             */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2002,2003 SAMSUNG ELECTRONICS CO., LTD.                */
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

//#include <VFLBuffer.h>
//#include <WMRTypes.h>
//#include <VFL.h>

#ifndef _POCKETSTOREII_HAL_WRAPPER_H_
#define _POCKETSTOREII_HAL_WRAPPER_H_

typedef struct {
    UINT    nCtrlCode;
    UINT    nLsn;
    UINT    nNumOfScts;
    UINT8  *pBuf;
    UINT32 *pTotalScts;
} FTLPacket;

typedef struct {
	UINT    	nCtrlCode;
	UINT    	nVbn;
	UINT    	nVpn;
	Buffer  	*pBuf;
	UINT    	nSrcVpn;
	UINT    	nDesVpn;
	BOOL32 	bCleanCheck;
	BUFType	eType;
} VFLPacket;


/*****************************************************************************/
/* nControlCode of PM_hal_wrapper                                            */
/*****************************************************************************/

#define PM_HAL_VFL_INIT					1
#define PM_HAL_VFL_OPEN				2
#define PM_HAL_VFL_FORMAT				3
#define PM_HAL_VFL_READ				4
#define PM_HAL_VFL_WRITE				5
#define PM_HAL_VFL_ERASE				6
#define PM_HAL_VFL_CPBACK				7
#define PM_HAL_VFL_SYNC				8
#define PM_HAL_VFL_CLOSE				9
#define PM_HAL_VFL_GETFTLCXT			10
#define PM_HAL_VFL_CHANGEFTLCXT		11
#define PM_HAL_VFL_BUF_GET			12
#define PM_HAL_VFL_BUF_REGET			13
#define PM_HAL_VFL_BUF_RELEASE		14
#define PM_HAL_FIL_GET_NANDINFO		15
#define PM_HAL_FIL_INIT					16

#define PM_HAL_FTL_INIT					20
#define PM_HAL_FTL_OPEN				21
#define PM_HAL_FTL_FORMAT				22
#define PM_HAL_FTL_READ				23
#define PM_HAL_FTL_WRITE				24
#define PM_HAL_FTL_CLOSE				25
#define PM_HAL_FTL_FORMATPROC			26
#define PM_HAL_FTL_READRECLAIM		27
#define PM_HAL_FTL_GARBAGECOLLECT	28
#define PM_HAL_FTL_SCAN				29

#define PM_HAL_WMR_FORMAT_FIL		30
#define PM_HAL_WMR_FORMAT_VFL		31
#define PM_HAL_WMR_FORMAT_FTL		32
#define PM_HAL_FTL_DELETE			33

#define IOCTL_POCKETSTORE_CMD	CTL_CODE(FILE_DEVICE_HAL, 4070, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_POCKETSTOREII_CMD	CTL_CODE(FILE_DEVICE_HAL, 4080, METHOD_BUFFERED, FILE_ANY_ACCESS)

UINT32 PSII_HALWrapper(VOID *pPacket, VOID *pInOutBuf, UINT32 *pResult);

#endif //_POCKETSTOREII_HAL_WRAPPER_H_
