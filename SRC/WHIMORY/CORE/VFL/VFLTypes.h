/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : Virtual Flash Layer                                         */
/* NAME    	   : VFL types definition header                                 */
/* FILE        : VFLTypes.h	                                                 */
/* PURPOSE 	   : This header defines Data types which are shared             */
/*               by all VFL submodules                                       */
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
/*   04-AUG-2003 [HM Jung]		: first writing								 */
/*   14-JUL-2005 [Jaesung Jung] : reorganize code                            */
/*	 06-SEP-2005 [Jaesung Jung] : fix from code inspection					 */
/*   24-JAN-2006 [Yangsup Lee ] : support un-pair bad block management       */
/*   31-MAR-2006 [Yangsup Lee ] : support ftl meta block wear leveling       */
/*                                                                           */
/*****************************************************************************/
#ifndef _VFL_TYPES_H_
#define _VFL_TYPES_H_


/**
 *  VFL constant definitions
 */
// bad mark position in spare
#define INIT_BAD_MARK_POS       (0)
// bad mark position in next plane spare  (2 plane programming)
#define INIT_BAD_MARK_2ND_POS   (16) * (SECTORS_PER_PAGE)
#define INIT_GOOD_MARK          (0xff)

typedef enum
{
    enuBOTH_PLANE_INIT_BAD  = 0x0,      // both plane
    enuLEFT_PLANE_INIT_BAD  = 0x00ff,   // only left plane (1x plane)
    enuRIGHT_PLANE_INIT_BAD	= 0xff00,   // only right plane
    enuNONE_PLANE_INIT_BAD  = 0xffff    // none plane

} t_init_bad_bitmap;

#define BAD_MARK_COMPRESS_SIZE  (8)


/**
 *  VFL context status(confirm) mark definition
 */
#define PAGE_INCOMPLETE (0xff)
#define PAGE_VALID      (0x00)


/**
 *  Data structure for storing the VFL context definition.
 *
 *
 * NOTICE !!
 * this structure is used directly to load VFL context by WMR_MEMCPY
 * so the byte pad of this structure must be 0 !!
 */
typedef struct {
    #if (WMR_SUPPORT_META_WEAR_LEVEL)   // Support FTL META block wear-leveling
    UINT32 nGlobalCxtAge;               // age for FTL meta information search
    UINT16 aFTLCxtVbn[WMR_NUM_MAPS_MAX + 1];
    UINT16 nPadding;
    #endif

    UINT32 nCxtAge;                     // context age 0xFFFFFFFF --> 0x0
    UINT32 nCxtLocation;                // page offset, context is located

    // this data is used for summary
    UINT16 nNumOfInitBadBlk;            // the number of initial bad blocks
    UINT16 nNumOfWriteFail;             // the number of write fail
    UINT16 nNumOfEraseFail;             // the number of erase fail

    // bad blocks management table & good block pointer
    UINT16 nBadMapTableMaxIdx;
    UINT16 aBadMapTable[WMR_MAX_RESERVED_SIZE * 2];
    UINT8 aBadMark[((WMR_MAX_VB / 8) / BAD_MARK_COMPRESS_SIZE)];

    // bad blocks management table within VFL info area
    UINT8 aBadInfoBlk[VFL_INFO_SECTION_SIZE];
} VFLCxt;


/**
 *  VFLMeta size is 2048 bytes(4 sector)
 */
typedef struct {
    VFLCxt stVFLCxt;

    UINT8 aReserved[((BYTES_PER_SECTOR * 4) - sizeof(VFLCxt))];
} VFLMeta;


/**
 *  Data structure for VFL context spare area
 */

/**
 *  spare layout for SLC & MLC
 */
typedef struct {
    UINT8 cBadMark;         // bad mark
    UINT8 aReserved[3];     // 1 byte CleanMark, 2 byte Reserved
    UINT32 nCxtAge;         // context age 0xFFFFFFFF --> 0x0
    UINT8 cStatusMark;      // status (confirm) mark
    // reserved for main ECC
} VFLSpare;


/**
 *  Asynchronous operation management structure & enum definition
 */
typedef enum {
    FLASH_OPERATION_NONE        = 0x10000000,
    FLASH_OPERATION_WRITE       = 0x10000001,
    FLASH_OPERATION_ERASE       = 0x10000002,
    FLASH_OPERATION_COPYBACK    = 0x10000003
} OpType;


typedef struct {
    UINT32 nDestBlkOriginal;
    UINT32 nDestLBlkRemapped;
    UINT32 nDestRBlkRemapped;
    UINT32 nSrcBlkRemapped;
    Buffer *pBuf;
    UINT32 nDestPageOffset;
    UINT32 nSrcPageOffset;
    OpType eOpType;
} AsyncOp;


#endif  // !_VFL_TYPES_H_

