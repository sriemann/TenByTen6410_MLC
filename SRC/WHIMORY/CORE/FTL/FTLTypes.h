/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : FTL				                                         */
/* NAME    	   : FTL types definition header                                 */
/* FILE        : FTLTypes.h	                                                 */
/* PURPOSE 	   : This header defines Data types which are shared             */
/*               by all FTL submodules                                       */
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
/*   22-JUL-2005 [Jaesung Jung] : first writing                              */
/*   03-NOV-2005 [Yangsup Lee ] : Add wear-leveling algorithm				 */
/*   14-MAR-2006 [Yangsup Lee ] : Don't use bad mark area                    */
/*   29-MAR-2006 [Yangsup Lee ] : modify wear-leveling structure             */
/*   31-MAR-2006 [Yangsup Lee ] : support ftl meta block wear leveling       */
/*   01-JAN-2007 [Inhwan Choi ] : 4k page nand support                       */
/*                                                                           */
/*****************************************************************************/
#ifndef _FTL_TYPES_H_
#define	_FTL_TYPES_H_


/**
 *  Data structure for storing the FTL log block context definition
 */
#define LOG_EMPTY_SLOT  (0xffff)

typedef	struct {
    UINT32 nAge;            // Log block age
    UINT32 nVbn;            // the virtual block number of log block
    UINT32 nLbn;            // the logical block number of log block
    UINT16 *paPOffsetL2P;   // L2P page offset mapping table
    UINT16 *paSecBitmap;    // sector allocation bitmap list
                            // +---------------+
                            // + x + x + x + o +    --> 0x7 (0111)
                            // +---------------+
    UINT16 nFreePOffset;    // free page offset in log block
    UINT16 nNumOfValidP;    // the number of valid page
    BOOL32 bCopyMerge;      // can be copymerged or not
    UINT32 nEC;             // erase count of log block
} LOGCxt;


/**
 *  Data structure for log block spare area
 */

// run mark definitions
#define LOG_CLEAN           (0xff)
#define LOG_MIDDLE_OF_RUN   (0x0f)
#define LOG_END_OF_RUN      (0x00)


// spare layout for SLC, MLC
typedef struct {
    UINT8 cBadMark;
    UINT8 aReserved[3]; // 1 byte CleanMark, 2 byte Reserved
    UINT32 nLPOffset;   // ECCc coverage is just this 4 bytes of spare area
    UINT8 aRunMark[WMR_SECTORS_PER_PAGE_MAX];
} LOGSpare;


/**
 *  Data structure for storing the FTL context definition
 */
// NOTICE !!
// this structure is used directly to load FTL context by WMR_MEMCPY
// so the byte pad of this structure must be 0
typedef struct
{
    UINT32 nAge;                            // Age of FTL context
    UINT32 nIdleVbn;                        // Virtual block number of Idle block
    UINT32 nIdleVbnEC;                      // Erase Count of Idle block
    UINT16 nCachedMapIdx;                   // Cached Map Index

    UINT16 nNumOfFreeVb;                    // Number of Free Virtual block
    UINT16 nFreeVbListTail;                 // Start point of Free Virtual block

    UINT16 nWearLevelCounter;               // WearLevel frequency	(20)
    UINT16 nMetaWearLevelCounter;           // MetaWearLevel frequency	(20)

    UINT16 aFreeVbList[FREE_SECTION_SIZE];  // Free Virtual block List
    UINT32 aFreeVbEC[FREE_SECTION_SIZE];    // Erase Count of Virtual block

    UINT16 aLogVbn[LOG_SECTION_SIZE];       // Virtual block number array of log block
    UINT16 aLogLbn[LOG_SECTION_SIZE];       // Logical block number array of log block
    UINT32 aLogEC[LOG_SECTION_SIZE];        // Erase Count array of log block

    #if (WMR_MLC_LSB_RECOVERY)
    UINT16 aBakLogVbn[LOG_SECTION_SIZE];
    UINT32 aBakLogEC[LOG_SECTION_SIZE];
    #endif

    #if (WMR_SUPPORT_META_WEAR_LEVEL)
    UINT16 aMapVbn[WMR_NUM_MAPS_MAX + 1];   // Virtual block map array (64+1)
    UINT16 nPadding;
    UINT16 aMapOffset[WMR_NUM_MAPS_MAX + 1];// Map page offset
    #else
    UINT16 aMapPosition[WMR_NUM_MAPS_MAX + 1];
    #endif
    UINT32 nOSScanCnt;                      // OS Scan Count Value
    UINT32 nFSScanCnt;                      // FS Scan Count Value
} FTLCxt;


/**
 *  Data structure of the FTL context & virtual block mapping table definition
 */
// NOTICE !!
// this structure is used directly to load FTL context by WMR_MEMCPY
// so the byte pad of this structure must be 0
typedef struct
{
    FTLCxt stFTLCxt;                        // FTL context struct
    UINT8 aReserved[BYTES_PER_SECTOR - sizeof(FTLCxt)];

    UINT16 aMapTbl[WMR_MAPS_PER_BLOCK];     // Logial to Virtual mapping table

    UINT32 nMinEC;                          // minimum erase count of minimum erased block
    UINT32 nMetaEC;                         // erase count of meta block
    UINT8 aReclaimMapTbl[WMR_MAPS_PER_BLOCK / 8];
    UINT8 aReserved3[(BYTES_PER_SECTOR / 2) - (sizeof(UINT32) * 2) - (WMR_MAPS_PER_BLOCK / 8)];

    UINT8 aECTbl[WMR_MAPS_PER_BLOCK * 3];   // Erase Count for warelevel 3Byte
} FTLMeta;


/**
 *  Data structure for FTL context spare area
 */
// FTL context spare confirm mark value
#define FTLCxt_CONFIRM_FREE     (0xff)
#define FTLCxt_CONFIRM_ALLOC    (0x00)

// spare layout for SLC & MLC
typedef struct {
    UINT8 cBadMark;
    UINT8 aReserved[3];     // 1 byte CleanMark, 2 byte Reserved
    UINT32 nAge;            // context age 0xFFFFFFFF --> 0x0
    UINT8 cStatusMark;      // status (confirm) mark
} FTLCxtSpare;


#endif  // !_FTL_TYPES_H_

