/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : MkPIExt.h                                                       */
/* PURPOSE : This file contains making Partition Information Exention for    */
/*           Region Information of BINFS Partition                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003,2004 SAMSUNG ELECTRONICS CO., LTD.                */
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
/*   01-APR-2004 [Janghwan Kim]: first writing                               */
/*                                                                           */
/*****************************************************************************/

#ifndef _POCKETSTORE_II_MKPIEXT_H_
#define _POCKETSTORE_II_MKPIEXT_H_

/*****************************************************************************/
/* Common Constant Definition                                                */
/*****************************************************************************/
#define RGN_ENTRY_MBR           0x01
#define RGN_ENTRY_XIPKERNEL     0x02
#define RGN_ENTRY_KERNEL        0x03
#define RGN_ENTRY_OS            0x04
#define RGN_ENTRY_SHELL         0x05
#define RGN_ENTRY_BROWSING      0x06
#define RGN_ENTRY_COREAPPS      0x08
#define RGN_ENTRY_EXAPPS        0x0A
#define RGN_ENTRY_MISC          0x0B
#define RGN_ENTRY_XIPCHAIN      0x0C


/*****************************************************************************/
/* Data Structure                                                            */
/*****************************************************************************/
typedef struct {
    UINT32 nID;
    UINT32 n1stVsn;
    UINT32 nNumOfScts;
} PartRgnEntry;



#define MAX_EXTENTION_NUM       9 
#define MAX_EXT_NAME_LEN        32
#define POCKETSTORE_II_EXT      0x55



typedef struct {
    UINT32  nVirAddr;                   /* 4 Bytes */
    UINT32  nExtLen;                    /* 4 Bytes */
    UINT8   aszExtName[XIP_NAMELEN];    /* 32 Bytes */
    UINT32  n1stVsn;                    /* 4 Bytes */
    UINT32  nNumOfScts;                 /* 4 Bytes */
} stBINPIExt;




/*****************************************************************************/
/* FIL External Functions                                                    */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


stBINPIExt* FindPIExt   (CHAR *pStr);
BOOL32      MakeBINPIExt(UINT32 nVol, UINT32 nBAddr, PXIPCHAIN_SUMMARY pChainSum);
VOID        PrintPIExt  (UINT32 nVol);


#ifdef __cplusplus
}
#endif // __cplusplus


#endif /* _POCKETSTORE_II_MKPIEXT_H_ */
