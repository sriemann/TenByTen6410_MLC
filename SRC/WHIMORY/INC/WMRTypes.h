/*****************************************************************************/
/*                                                                           */
/* PROJECT : Rainbow	                                                     */
/* MODULE  : Whimory types definition heade file	                         */
/* NAME    : Whimory types definition                                        */
/* FILE    : WMRTypes.h		                                                 */
/* PURPOSE : Types definition for Whimory                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*        COPYRIGHT 2003-2005, SAMSUNG ELECTRONICS CO., LTD.                 */
/*                      ALL RIGHTS RESERVED                                  */
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
/*   12-JUL-2005 [Jaesung Jung] : first writing                              */
/*   01-JAN-2007 [Inhwan Choi ] : 4k page nand support                       */
/*                                                                           */
/*****************************************************************************/

#ifndef _WMR_TYPES_H_
#define _WMR_TYPES_H_

/*****************************************************************************/
/* Basic Types                                                               */
/*****************************************************************************/
#if !defined(WMR_NO_BASIC_TYPES)

typedef		unsigned int        	UINT32;
typedef		signed int          	INT32;
typedef		unsigned short      	UINT16;
typedef		signed short        	INT16;
typedef		unsigned char       	UINT8;
typedef		signed char         	INT8;

#endif /* WMR_NO_BASIC_TYPES */

#ifndef		VOID
typedef		void                	VOID;
#endif		//VOID
typedef		UINT32              	BOOL32;

/*****************************************************************************/
/* Basic Constants                                                           */
/*****************************************************************************/
#define		FALSE32					(BOOL32) 0
#define		TRUE32					(BOOL32) 1

/*****************************************************************************/
/* Global Definition which should be shared by FTL, VFL, FIL                 */
/*****************************************************************************/
/* [NAND Device Type]									*/
/* these constants are used for WMRDeviceI.nDeviceType	*/
#define		WMR_SLC				1		/* SLC device type					 */
#define		WMR_MLC				2		/* MLC device type					 */

#define		IS_SLC				(GET_DevType() == WMR_SLC)
#define		IS_MLC				(GET_DevType() == WMR_MLC)

/*  Sort of Paired Page Address Translation (MLC)	*/
/* #if (WMR_MLC_LSB_RECOVERY) */
#define		WMR_MLC_NO				0
#define		WMR_MLC_LSB_CLASS1		1
#define		WMR_MLC_LSB_CLASS2		2
/* #endif */

/* whimory version signature							*/
#define     WMR_SIGNATURE       (('2' << 24) | ('2' << 16) | ('0' << 8) | ('W' << 0))

/*****************************************************************************/
/* NAND SECTOR BITMAP shared by FTL, VFL, FIL   				             */
/*****************************************************************************/
/*
case 1. SECTORS_PER_PAGE == 4, TWO_PLANE_PROGRAM = FALSE32
		FULL_SECTOR_BITMAP_PAGE = 	0xF (1111)
		LEFT_SECTOR_BITMAP_PAGE = 	0xF (1111)
		RIGHT_SECTOR_BITMAP_PAGE = 	0x0 (0000)

case 2. SECTORS_PER_PAGE == 4, TWO_PLANE_PROGRAM = TRUE32
		FULL_SECTOR_BITMAP_PAGE = 	0xFF (1111 1111)
		LEFT_SECTOR_BITMAP_PAGE = 	0x0F (0000 1111)
		RIGHT_SECTOR_BITMAP_PAGE = 	0xF0 (1111 0000)

case 3. SECTORS_PER_PAGE == 8, TWO_PLANE_PROGRAM = FALSE32
		FULL_SECTOR_BITMAP_PAGE = 	0xFF (1111 1111)
		LEFT_SECTOR_BITMAP_PAGE = 	0xFF (1111 1111)
		RIGHT_SECTOR_BITMAP_PAGE = 	0x00 (0000 0000)

case 4. SECTORS_PER_PAGE == 8, TWO_PLANE_PROGRAM = TRUE32
	 	FULL_SECTOR_BITMAP_PAGE = 	0xFFFF (1111 1111 1111 1111)
		LEFT_SECTOR_BITMAP_PAGE = 	0x00FF (0000 0000 1111 1111)
		RIGHT_SECTOR_BITMAP_PAGE = 	0xFF00 (1111 1111 0000 0000)
*/

#define 	FULL_SECTOR_BITMAP_PAGE 	((1 << (SECTORS_PER_PAGE * (TWO_PLANE_PROGRAM + 1))) - 1)
/*
	in case 1: 	FULL_SECTOR_BITMAP_PAGE = 	((1 << (4 * (0 + 1)))-1) = 0xF
	in case 2: 	FULL_SECTOR_BITMAP_PAGE = 	((1 << (4 * (1 + 1)))-1) = 0xFF
	in case 3: 	FULL_SECTOR_BITMAP_PAGE = 	((1 << (8 * (0 + 1)))-1) = 0xFF
	in case 4: 	FULL_SECTOR_BITMAP_PAGE = 	((1 << (8 * (1 + 1)))-1) = 0xFFFF
*/

#define 	LEFT_SECTOR_BITMAP_PAGE	\
	((FULL_SECTOR_BITMAP_PAGE) & ((FULL_SECTOR_BITMAP_PAGE) \
	>> (((SECTORS_PER_PAGE / 2) * TWO_PLANE_PROGRAM) * (TWO_PLANE_PROGRAM + 1))))
/*
	in case 1: ((0xF) & ((0xF) >> (((4/2)*0) * (0 + 1))) = 0xF
	in case 2: ((0xFF) & ((0xFF) >> (((4/2)*1) * (1 + 1))) = 0x0F
	in case 3: ((0xFF) & ((0xFF) >> (((8/2)*0) * (0 + 1))) = 0xFF
	in case 4: ((0xFFFF) & ((0xFFFF) >> (((8/2)*1) * (1 + 1))) = 0x00FF
*/

#define 	RIGHT_SECTOR_BITMAP_PAGE \
	((FULL_SECTOR_BITMAP_PAGE) & ((FULL_SECTOR_BITMAP_PAGE) \
	<< ((SECTORS_PER_PAGE / (1 + TWO_PLANE_PROGRAM)) * (TWO_PLANE_PROGRAM + 1))))
/*
	in case 1: ((0xF) & ((0xF) << (((4/(1+0))*0) * (0 + 1))) = 0x0
	in case 2: ((0xFF) & ((0xFF) << (((4/(1+1))*1) * (1 + 1))) = 0xF0
	in case 3: ((0xFF) & ((0xFF) << (((8/(1+0))*0) * (0 + 1))) = 0x00
	in case 4: ((0xFFFF) & ((0xFFFF) << (((8/(1+1))*1) * (1 + 1))) = 0xFF00
*/

typedef enum
{
	enuNONE_PLANE_BITMAP		= 0x0,	/*	non plane  					*/
	enuBOTH_PLANE_BITMAP		= 0x3,	/*	both plane  					*/
	enuLEFT_PLANE_BITMAP		= 0x1,	/*	only left plane(one plane)  	*/
	enuRIGHT_PLANE_BITMAP	= 0x2	/*	only right plane     				*/
} t_plane_bitmap;


/*****************************************************************************/
/* Divide & remnant operation definitions					                 */
/*****************************************************************************/
#if (!WMR_STDLIB_SUPPORT)
#define		DIV(x, y)			((x) >> (y))
#define		REM(x, y)			((x) & ((1 << (y)) - 1))
#endif


/*****************************************************************************/
/* Global variables	definitions (Sector size for System)		             */
/*****************************************************************************/
#define     OND_SECTOR_SHIFT            (2)			// 0 : sector=512B,  2 : sector=2KB,  3 : sector=4KB


/*****************************************************************************/
/* Return value MACRO definition                                             */
/*****************************************************************************/
#define		WMR_RETURN_MAJOR(err)			(INT32)((err) & 0xFFFF0000)
#define		WMR_RETURN_MINOR(err)			(INT32)((err) & 0x0000FFFF)
#define		WMR_RETURN_VALUE(err, maj, min)	(INT32)(((UINT32)((err) & 0x00000001) << 31) | \
											        ((UINT32)((maj) & 0x00007FFF) << 16) | \
											         (UINT32)((min) & 0x0000FFFF))



/*****************************************************************************/
/* Global Structure Types which should be shared by FTL, VFL, FIL            */
/*****************************************************************************/

//  Structures for further Debug message
typedef struct {
    BOOL32  bFlagErrInfoDBGMSG;         /* Flag used in core to enable/disable printing the further DBGMSG */
    UINT32  nPbnForFlag;                /* the number of virtual block has the flag indicated enable/disable the further DBGMSG from OEM */
    UINT32  nPpnInPbnForFlag;           /* the number of page in the block from OEM*/
} WMRDBGFlagPosition;

typedef struct {
    UINT32  nLsn;                       /* the number of Logical sector used in FTL */
    UINT32  nLbn;                       /* the number of Logical block  used in FTL */
    UINT32  nVbn;                       /* the number of Virtual block  used in FTL */
    UINT32  nVpn;                       /* the number of Virtual page   used in VFL */
    UINT32  nPbn;                       /* the number of Physical block used in VFL */
    UINT32  nPpn;                       /* the number of Physical page  used in VFL */
} WMRDBGLPVNumber;


typedef struct {

	UINT32	nDeviceType;				/* the type of device SLC [1], MLC [2]				 */
	UINT32	nAddrCycle;					/* device address cycle				 */
	UINT32	nSecPerPage;				/* the number of sectors per page	 */
	UINT32	nSecPerVPage;				/* the number of sectors per super page	 */
	BOOL32	b2XProgram;					/* 2plane program support			 	 */
	BOOL32	b2XRead;					/* 2plane read support			 		 */
	BOOL32	b2XReadStatus;					/* 2plane read status support			 */
	UINT32	nPagesPerBlock;				/* the count of pages per block		 */
	UINT32	nSecPerVb;					/* the number of sectors per virtual block */
	UINT32	nPagesPerVb;				/* the count of pages per virtual block */
	UINT32	nPagesPerBank;				/* the count of pages per bank		 */
	UINT32	nPagesTotal;				/* the total number of pages		 */
	UINT32	nVbTotal;					/* the total number of virtual block */
	UINT32	nUserVbTotal;				/* the total number of data virtual block */
	UINT32	nUserSecTotal;				/* the total number of data sector   */

	UINT32	nBlocksPerBank;				/* the count of blocks per bank		 */

	UINT32	nBytesPerPage;				/* bytes per page (main)			 */
	UINT32	nBytesPerSpare;				/* bytes per spare					 */
	UINT32	nBytesPerVPage;				/* bytes per super page (main)			 */
	UINT32	nBytesPerVSpare;				/* bytes per super spare					 */

	#if (!WMR_STDLIB_SUPPORT)
	UINT32	nSecPerPageShift;
	UINT32	nSecPerVPageShift;
	UINT32	nSecPerVbShift;
	UINT32	nPagesPerBankShift;
	UINT32	nPagesPerBlockShift;
	UINT32	nPagesPerVbShift;

	UINT32	nBlocksPerBankShift;
	#endif

	BOOL32	bReadErrFlag;
    BOOL32  bScanProgressFlag;

	#if (WMR_MLC_LSB_RECOVERY)
	UINT32	nMLCLSBClass;				/* Sort of Paired Address Translation (MLC) */
	#endif

    UINT32  nFATSize;

	BOOL32  b8BitECC;

} WMRDeviceInfo;


typedef struct {

	UINT32	nNumOfBank;					/* the number of banks				 */
	UINT32	nNumOfMaps;					/* the number of map blocks			 */

	BOOL32	bInterLeaving;				/* support interleaving or not		 */
    BOOL32  bInternalInterLeaving;      /* support internal interleaving or not */
	BOOL32	bCheckSpareECC;				/* check spare ecc or not (for bad check)*/

	#if (!WMR_STDLIB_SUPPORT)
	UINT32	nNumOfBankShift;
	UINT32	nNumOfMapsShift;
	#endif

    UINT32  nFSScanRatio;                 // scan percentage of total FTL area
    UINT32  nOSScanRatio;                 // scan percentage of total FTL area
    BOOL32  bBootingScan;

    UINT32  nCriticalReadCountValue;      // scan percentage of total FTL area

    // Structures for further debug message
    WMRDBGFlagPosition WMRDBGFlagPos;       // Flag and flag position for further Debug message
    WMRDBGLPVNumber WMRLPVNum;              // L,P,V block, page, sector number of the latest read operation
} WMRConfig;

typedef struct {
	UINT32	nWMRAreaStart;				/* WMR_AREA_SIZE */

	UINT32	nSPAreaSize;				/* the block number of Special area size */

	UINT32	nVFLAreaSize;				/* the block number of VFL area size */
	UINT32	nReservedSecSize;			/* the size of reserved section		 */

	UINT32	nFTLInfoStart;				/* FTL info section start			 */
	UINT32	nFTLInfoSize;				/* FTL info section size			 */

	UINT32	nFreeSecStart;				/* free section start				 */

	UINT32	nDataSecStart;				/* data section start				 */
	UINT32 	nDataSecSize;				/* data section size				 */

#if (WMR_READ_RECLAIM)
	UINT32  nReclaimCnt;
#endif

} WMRLayout;

/*****************************************************************************/
/* Global variables	extern										             */
/*****************************************************************************/
extern 		WMRDeviceInfo			stDeviceInfo;
extern		WMRConfig				stConfig;
extern		WMRLayout				stLayout;

/*****************************************************************************/
/* Global variables	redefinitions (WMRDeviceInfo)							 */
/*****************************************************************************/
#define		GET_DevType()			(stDeviceInfo.nDeviceType)
#define		SET_DevType(x)			(stDeviceInfo.nDeviceType = x)

#define		DEV_ADDR_CYCLE			(stDeviceInfo.nAddrCycle)

#define		SECTORS_PER_PAGE		(stDeviceInfo.nSecPerPage)
#define		SECTORS_PER_SUPAGE		(stDeviceInfo.nSecPerVPage)
#define 	TWO_PLANE_PROGRAM		(stDeviceInfo.b2XProgram)
#define 	TWO_PLANE_READ			(stDeviceInfo.b2XRead)
#define 	TWO_PLANE_READ_STATUS	(stDeviceInfo.b2XReadStatus)
#define		PAGES_PER_BLOCK			(stDeviceInfo.nPagesPerBlock)
#define		SECTORS_PER_SUBLK		(stDeviceInfo.nSecPerVb)
#define		PAGES_PER_SUBLK			(stDeviceInfo.nPagesPerVb)
#define		PAGES_PER_BANK			(stDeviceInfo.nPagesPerBank)
#define		PAGES_TOTAL			(stDeviceInfo.nPagesTotal)

#define 	ECC_8BIT_SUPPORT		(stDeviceInfo.b8BitECC) // to support 8Bit ecc hsjang 080923

#define		PAGES_PER_METADATA		(1)

#define		SUBLKS_TOTAL			(stDeviceInfo.nVbTotal)

#define		USER_SUBLKS_TOTAL		(stDeviceInfo.nUserVbTotal)
#define		USER_SECTORS_TOTAL		(stDeviceInfo.nUserSecTotal)

#if (!WMR_STDLIB_SUPPORT)
#define		SECTORS_PER_PAGE_SHIFT	(stDeviceInfo.nSecPerPageShift)
#define		SECTORS_PER_SUPAGE_SHIFT	(stDeviceInfo.nSecPerVPageShift)
#define		SECTORS_PER_SUBLK_SHIFT (stDeviceInfo.nSecPerVbShift)

#define		PAGES_PER_BLOCK_SHIFT	(stDeviceInfo.nPagesPerBlockShift)
#define 	PAGES_PER_SUBLK_SHIFT	(stDeviceInfo.nPagesPerVbShift)
#define		PAGES_PER_BANK_SHIFT	(stDeviceInfo.nPagesPerBankShift)

#define		BLOCKS_PER_BANK_SHIFT	(stDeviceInfo.nBlocksPerBankShift)
#endif

#define		BLOCKS_PER_BANK			(stDeviceInfo.nBlocksPerBank)

#define		BYTES_PER_SECTOR		(WMR_SECTOR_SIZE)
#define		BYTES_PER_SPARE		(WMR_SPARE_SIZE)
#define		BYTES_PER_MAIN_PAGE			(stDeviceInfo.nBytesPerPage)
#define		BYTES_PER_SPARE_PAGE			(stDeviceInfo.nBytesPerSpare)
#define		BYTES_PER_MAIN_SUPAGE			(stDeviceInfo.nBytesPerVPage)
#define		BYTES_PER_SPARE_SUPAGE			(stDeviceInfo.nBytesPerVSpare)

#define		READ_ERR_FLAG			(stDeviceInfo.bReadErrFlag)
#define     SCAN_PROGRESS_FLAG      (stDeviceInfo.bScanProgressFlag)

#if (WMR_MLC_LSB_RECOVERY)
#define		MLC_LSB_CLASS			(stDeviceInfo.nMLCLSBClass)
#endif

#define     FAT_SIZE                (stDeviceInfo.nFATSize)

/*****************************************************************************/
/* Global variables	redefinitions (WMRConfig)			     */
/*****************************************************************************/
#define		BANKS_TOTAL				(stConfig.nNumOfBank)
#define		NUM_MAPS				(stConfig.nNumOfMaps)
#define		IS_SUPPORT_INTERLEAVING (stConfig.bInterLeaving)
#define     IS_SUPPORT_INTERNAL_INTERLEAVING (stConfig.bInternalInterLeaving)

#define		IS_CHECK_SPARE_ECC		(stConfig.bCheckSpareECC)

#if (!WMR_STDLIB_SUPPORT)
#define		BANKS_TOTAL_SHIFT		(stConfig.nNumOfBankShift)
#define		NUM_MAPS_SHIFT			(stConfig.nNumOfMapsShift)
#endif

#define     FS_SCAN_RATIO           (stConfig.nFSScanRatio)
#define     OS_SCAN_RATIO           (stConfig.nOSScanRatio)
#define     BOOTING_SCAN            (stConfig.bBootingScan)

#define     CRITICAL_READ_CNT       (stConfig.nCriticalReadCountValue)

// Flag and variables for further debug message
#define     WMR_ERR_INFO_DBGMSG     (stConfig.WMRDBGFlagPos.bFlagErrInfoDBGMSG)
#define     WMR_ERR_INFO_FLAG_POS_BN    (stConfig.WMRDBGFlagPos.nPbnForFlag)
#define     WMR_ERR_INFO_FLAG_POS_PN    (stConfig.WMRDBGFlagPos.nPpnInPbnForFlag)

#define     WMR_LLSN                (stConfig.WMRLPVNum.nLsn)
#define     WMR_LLBN                (stConfig.WMRLPVNum.nLbn)
#define     WMR_LVBN                (stConfig.WMRLPVNum.nVbn)
#define     WMR_LVPN                (stConfig.WMRLPVNum.nVpn)
#define     WMR_LPBN                (stConfig.WMRLPVNum.nPbn)
#define     WMR_LPPN                (stConfig.WMRLPVNum.nPpn)

#define     SCAN_OS            (1<<1)
#define     SCAN_FS            (1<<2)

/*****************************************************************************/
/* Global variables	redefinitions (WMRLayout)					             */
/*
	+---------------------------+
	| WMR_AREA (1)				|
	+---------------------------+
	| SPECIAL_AREA (0)			|	User specific area
	+---------------------------+
	| VFL_INFO_SECTION (4)		|
	+---------------------------+
	| RESERVED_SECTION			| = SUBLKS_TOTAL - (FTL_AREA_SIZE + WMR_AREA_SIZE + VFL_INFO_SECTION_SIZE);
	+---------------------------+
	| FTL_INFO_SECTION 			| = (SUBLKS_TOTAL / WMR_MAPS_PER_BLOCK) + 2
	+---------------------------+
	| FREE_SECTION (10)			|
	| +	LOG_SECTION (7)			|
	| +	FREE_LIST_SIZE (3)		|
	+---------------------------+
	| DATA_SECTION				| = SUBLKS_TOTAL * WMR_USER_SUBLKS_RATIO / 256;
	+---------------------------+
*/
/*****************************************************************************/

#define	WMR_AREA_DEF_SIZE		(10)		// WMR(1) + TOC(2) + Eboot(7)
#define	WMR_AREA_SIZE			(stLayout.nWMRAreaStart)		// WMR(1) + TOC(2) + Eboot(7) 

#define   SPECIAL_AREA_START		(WMR_AREA_SIZE)
#define	  SPECIAL_AREA_SIZE		(stLayout.nSPAreaSize)		// dodan2-061129

#define	VFL_AREA_START			(SPECIAL_AREA_START + SPECIAL_AREA_SIZE)
#define	VFL_AREA_SIZE			(stLayout.nVFLAreaSize)

#define	VFL_INFO_SECTION_START		(VFL_AREA_START)
#define	VFL_INFO_SECTION_SIZE		(4)

#define	RESERVED_SECTION_START		(VFL_INFO_SECTION_START + VFL_INFO_SECTION_SIZE)
#define	RESERVED_SECTION_SIZE		(stLayout.nReservedSecSize)

#define	FTL_INFO_SECTION_START		(stLayout.nFTLInfoStart)
#define	FTL_INFO_SECTION_SIZE		(stLayout.nFTLInfoSize)

#if (!WMR_MLC_LSB_RECOVERY)
#define	LOG_SECTION_SIZE		(FREE_SECTION_SIZE - FREE_LIST_SIZE)

#define	FREE_SECTION_START		(stLayout.nFreeSecStart)
#define	FREE_SECTION_SIZE			(10)
#define   FREE_LIST_SIZE          		(3)
#else
#define		LOG_SECTION_SIZE		((FREE_SECTION_SIZE - FREE_LIST_SIZE)/2)

#define		FREE_SECTION_START		(stLayout.nFreeSecStart)
#define		FREE_SECTION_SIZE		(17)
#define     FREE_LIST_SIZE          (3)
#endif

#define	DATA_SECTION_START		(stLayout.nDataSecStart)
#define	DATA_SECTION_SIZE			(stLayout.nDataSecSize)

#define	FTL_AREA_START			(VFL_AREA_START + VFL_AREA_SIZE)
#define	FTL_AREA_SIZE				(FTL_INFO_SECTION_SIZE + FREE_SECTION_SIZE + USER_SUBLKS_TOTAL)

#if (WMR_READ_RECLAIM)
#define WMR_MAX_RECLAIM         (stLayout.nReclaimCnt)
#endif

/*****************************************************************************/
/* exported function prototype 		                                         */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

VOID 		CalcGlobal				(BOOL32 bInternalInterleaving);

#if (WMR_MLC_LSB_RECOVERY)
UINT32		GetMlcClass				(UINT8 nDID, UINT8 nHID);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WMR_TYPES_H_ */
