/*****************************************************************************/
/*                                                                           */
/* PROJECT : Rainbow	                                                     */
/* MODULE  : Whimory configuration definition heade file                     */
/* NAME    : Whimory configuration definition                                */
/* FILE    : WMRConfig.h                                                     */
/* PURPOSE : Configuation definition for Whimory                             */
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
/*   03-NOV-2005 [Yangsup Lee ] : Add wear-leveling algorithm				 */
/*                                                                           */
/*****************************************************************************/

#ifndef _WMR_CONFIG_H_
#define _WMR_CONFIG_H_

/*****************************************************************************/
/* Build Config Definition which should be shared by FTL, VFL, FIL           */
/* Must edit this configurations for your target							 */
/*****************************************************************************/

/* [Supported Device Selection]								*/
/* can reduce code size with editing this configurations	*/
#ifdef SUPPORTSLC
#define		WMR_SLC_SUPPORT		(1)		/* support SLC large block nand		 */
#define		WMR_MLC_SUPPORT		(0)		/* support MLC large block nand		 */
#endif
#ifdef SUPPORTMLC
#define		WMR_SLC_SUPPORT		(0)		/* support SLC large block nand		 */
#define		WMR_MLC_SUPPORT		(1)		/* support MLC large block nand		 */
#endif

#if ((!WMR_SLC_SUPPORT) && (!WMR_MLC_SUPPORT))
#error Configuration Error! Must Select Supported Device
#endif

#define		WMR_STDLIB_SUPPORT	(1)		/* support divide operation			 */

#define		WMR_ALIGN_CHECK		(1)		/* support 4 bytes align check		 */

#define		WMR_BIG_ENDIAN		(0)		/* support big endian 				 */

#define		WMR_READ_RECLAIM	(1)		/* support read 1bit ecc error reclaim */

#define		WMR_SUPPORT_META_WEAR_LEVEL  (1) /* support FTL meta block wear level algorithm      */

#ifdef SUPPORTSLC
#define		WMR_MLC_LSB_RECOVERY	(0)				/* support reovery LSB corruption of MLC 	*/
#endif
#ifdef SUPPORTMLC
#define		WMR_MLC_LSB_RECOVERY	(1)				/* support reovery LSB corruption of MLC 	*/
#endif

#define		WMR_MLC_LSB_SQA			(0)		/* support SQA */

#if ((WMR_SLC_SUPPORT) && (WMR_MLC_LSB_RECOVERY))
#error Configuration Error! WMR_SLC_SUPPORT and WMR_MLC_LSB_RECOVERY can not define together.
#endif

#define		WMR_SIMULATION      (0)     /* support simulation                */

/*****************************************************************************/
/* Global Config Definition which should be shared by FTL, VFL, FIL          */
/*****************************************************************************/
#define		WMR_MAX_DEVICE		(4)					/* the maximum number of bank		 */

#define		WMR_SECTOR_SIZE		(512)	/* the size of sector				 */

#define		WMR_SPARE_SIZE		(16)	/* the size of spare				 */

#define		WMR_NUM_BUFFERS		(1)		/* the number of buffers			 */

#define		WMR_SECTORS_PER_PAGE_MAX (16)			/* the maximum size of sectors per page */

#define		WMR_MAX_VB			(16384)	/* the maximum number of virtual block*/

#define		WMR_MAX_RESERVED_SIZE	(400)
									/* the maximum count of reserved blocks */

#define		WMR_MAPS_PER_BLOCK	(WMR_SECTOR_SIZE / 2)
										/* the count of map index per block  */

#define		WMR_NUM_MAPS_MAX	(WMR_MAX_VB / WMR_MAPS_PER_BLOCK)
										/* the maximum number of map blocks  */

#if (WMR_READ_RECLAIM)
#define		DEF_WMR_MAX_RECLAIM		(2)		/* the maximum number of reclaim count*/
#endif

/* reserved block ratio for whimory context & reserved section    */
/* you can change this value to change whimory total sector count */
#define     WMR_RESERVED_SUBLKS_RATIO		(5)		// by dodan2-061129 (% Percent Unit)

#define     WMR_SPECIAL_AREA_SIZE           (600)   // (in case of 1MB per Block, 100 = 100x1MB = 100MB)

#define		WMR_WEAR_LEVEL_FREQUENCY	    (20)
#define		WMR_WEAR_LEVEL_MAX_DIFF		    (10)
						/* maximum different between min and max erase count */
#define		WMR_META_WEAR_LEVEL_FREQUENCY	(200)

#define		Signature_Ppn					(PAGES_PER_BLOCK - 1)	// modified value... hmseo-061122 for MLC

/////////////////////////////////////////////////////////////
// Use Global variables instead of allocated memory
#define NO_MALLOC	// by dodan2 061215

#endif /* _WMR_CONFIG_H_ */
