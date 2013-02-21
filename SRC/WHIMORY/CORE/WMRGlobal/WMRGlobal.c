/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : Global				                                         */
/* NAME    	   : 							                                 */
/* FILE        : WMRGlobal.c	                                             */
/* PURPOSE 	   : This file contains the exported global variable & function. */
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
/*   11-AUG-2005 [Jaesung Jung] : first writing                              */
/*                                                                           */
/*****************************************************************************/
#include <WMRConfig.h>
#include <WMRTypes.h>
#include <OSLessWMROAM.h>


/*****************************************************************************/
/* Variables definitions                                              		 */
/*****************************************************************************/
WMRDeviceInfo			stDeviceInfo = {0,};
WMRConfig				stConfig = {0,};
WMRLayout				stLayout = {0,};

/*****************************************************************************/
/* Static function prototypes                                                */
/*****************************************************************************/
#if (!WMR_STDLIB_SUPPORT)
PRIVATE UINT32		_CalcShift			(UINT32	 nVal);
#endif

/*****************************************************************************/
/* Code Implementation                                                       */
/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _CalcShift	                                                         */
/* DESCRIPTION                                                               */
/*      This function calculate the shift value of global variables.	     */
/* PARAMETERS                                                                */
/*      nVal   [IN] 	global variable										 */
/* RETURN VALUES                                                             */
/*		UINT32																 */
/*				shift value													 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
#if (!WMR_STDLIB_SUPPORT)
PRIVATE UINT32
_CalcShift (UINT32 nVal)
{
	UINT32 nRet = 0;
	UINT32 nTemp = nVal;

	do
	{
		if (nTemp & 0x1)
			break;
		nTemp = (nTemp >> 1);
		nRet++;

	}while(1);

	return nRet;
}
#endif

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      CalcGlobal	                                                         */
/* DESCRIPTION                                                               */
/*      This function calculate global variables.						     */
/* PARAMETERS                                                                */
/*      bInternalInterleaving   [IN] 	support internal interleaving or not */
/* RETURN VALUES                                                             */
/*		none																 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
CalcGlobal(BOOL32 bInternalInterleaving)
{
	WMR_AREA_SIZE = WMR_AREA_DEF_SIZE;

#if (WMR_READ_RECLAIM)
	WMR_MAX_RECLAIM = DEF_WMR_MAX_RECLAIM;
#endif

	IS_CHECK_SPARE_ECC = TRUE32;

    IS_SUPPORT_INTERNAL_INTERLEAVING = bInternalInterleaving;

	/****************************/
	/* Basic device information */
	/****************************/
	if ((BANKS_TOTAL > 1) || (IS_SUPPORT_INTERNAL_INTERLEAVING == TRUE32))
	{
		IS_SUPPORT_INTERLEAVING = TRUE32;
	}
	else
	{
		IS_SUPPORT_INTERLEAVING = FALSE32;
	}

	if ((BLOCKS_PER_BANK > 1024) || (TWO_PLANE_PROGRAM == TRUE32))
	{
		DEV_ADDR_CYCLE = 5;
	}
	else
	{
		DEV_ADDR_CYCLE = 4;
	}

	PAGES_PER_BANK = BLOCKS_PER_BANK * PAGES_PER_BLOCK;
	PAGES_TOTAL = PAGES_PER_BANK * BANKS_TOTAL;
	PAGES_PER_SUBLK = PAGES_PER_BLOCK * BANKS_TOTAL;

	BYTES_PER_MAIN_PAGE = BYTES_PER_SECTOR * SECTORS_PER_PAGE;

	if (ECC_8BIT_SUPPORT)
	{
		BYTES_PER_SPARE_PAGE = 256;					
	}
	else
	{
		BYTES_PER_SPARE_PAGE = BYTES_PER_SPARE * SECTORS_PER_PAGE;			
	}
	

	SECTORS_PER_SUBLK = PAGES_PER_SUBLK * SECTORS_PER_PAGE;
	SECTORS_PER_SUPAGE = SECTORS_PER_PAGE;

	BYTES_PER_MAIN_SUPAGE = BYTES_PER_MAIN_PAGE;
	BYTES_PER_SPARE_SUPAGE = BYTES_PER_SPARE_PAGE;

	if (TWO_PLANE_PROGRAM == TRUE32)
	{
		SECTORS_PER_SUBLK *= 2;
		SECTORS_PER_SUPAGE *= 2;
		BYTES_PER_MAIN_SUPAGE *=2;
		BYTES_PER_SPARE_SUPAGE *= 2;
	}

	SUBLKS_TOTAL = BLOCKS_PER_BANK;

#if (!WMR_STDLIB_SUPPORT)
	BANKS_TOTAL_SHIFT = _CalcShift(BANKS_TOTAL);
#endif

	NUM_MAPS = SUBLKS_TOTAL / WMR_MAPS_PER_BLOCK;

	/****************************/
	/* Whimory VFL, FTL layout information */
	/****************************/

	/* EACH SECTION SIZE */
	FTL_INFO_SECTION_SIZE = NUM_MAPS + 2;
	SPECIAL_AREA_SIZE = WMR_SPECIAL_AREA_SIZE/BANKS_TOTAL/(SECTORS_PER_SUPAGE/SECTORS_PER_PAGE);
	RESERVED_SECTION_SIZE = (SUBLKS_TOTAL*WMR_RESERVED_SUBLKS_RATIO)/100;
	VFL_AREA_SIZE = VFL_INFO_SECTION_SIZE + RESERVED_SECTION_SIZE;

	USER_SUBLKS_TOTAL = SUBLKS_TOTAL - (WMR_AREA_SIZE + SPECIAL_AREA_SIZE + RESERVED_SECTION_SIZE + VFL_INFO_SECTION_SIZE + FTL_INFO_SECTION_SIZE + FREE_SECTION_SIZE);

#if 0  // it is needless because the method for calculation of USER_SUBLKS_TOTAL is changed
	#if(WMR_MLC_LSB_RECOVERY)
	if(IS_MLC)
	{
		USER_SUBLKS_TOTAL -= LOG_SECTION_SIZE;
	}
	#endif
#endif

	USER_SECTORS_TOTAL = USER_SUBLKS_TOTAL * SECTORS_PER_SUBLK;
	DATA_SECTION_SIZE = USER_SUBLKS_TOTAL;

	/* EACH SECTION START ADDRESS */
	FTL_INFO_SECTION_START = FTL_AREA_START;
	FREE_SECTION_START = FTL_INFO_SECTION_START + FTL_INFO_SECTION_SIZE;
	DATA_SECTION_START = FREE_SECTION_START + FREE_SECTION_SIZE;

	/* fat area size */
	if (LOG_SECTION_SIZE > 4)
	{
		if (IS_MLC)
		{
			FAT_SIZE = 2;
		}
		else
		{
			FAT_SIZE = 3;
		}
	}
	else
	{
		FAT_SIZE = 0;
	}

	/****************************/
	#if (!WMR_STDLIB_SUPPORT)
	PAGES_PER_SUBLK_SHIFT = _CalcShift(PAGES_PER_SUBLK);
  	NUM_MAPS_SHIFT = _CalcShift(NUM_MAPS);
  	SECTORS_PER_SUBLK_SHIFT = _CalcShift(SECTORS_PER_SUBLK);
  	SECTORS_PER_PAGE_SHIFT = _CalcShift(SECTORS_PER_PAGE);
  	SECTORS_PER_SUPAGE_SHIFT = _CalcShift(SECTORS_PER_SUPAGE);
  	PAGES_PER_SUBLK_SHIFT = _CalcShift(PAGES_PER_SUBLK);
  	PAGES_PER_BLOCK_SHIFT = _CalcShift(PAGES_PER_BLOCK);
  	PAGES_PER_BANK_SHIFT = _CalcShift(PAGES_PER_BANK);
   	BLOCKS_PER_BANK_SHIFT = _CalcShift(BLOCKS_PER_BANK);
  	#endif

	return;

}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      GetMlcClass	                                                         */
/* DESCRIPTION                                                               */
/*      This function return TYPE of MLC ( Pair of LSB and MSB ).	   	     */
/* PARAMETERS                                                                */
/*      nDID   [IN] 	Device ID											 */
/*      nHID   [IN] 	Hidden ID											 */
/* RETURN VALUES                                                             */
/*		Type of MLC															 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
#if (WMR_MLC_LSB_RECOVERY)
UINT32 
GetMlcClass(UINT8 nDID, UINT8 nHID)
{
#if 0
	struct 
	{
		UINT8	nDevID;
		UINT8	nHidID;
		UINT32	nClass;
	} stMlc[6] = { 
					{0xDC, 0x14, WMR_MLC_LSB_CLASS1 },	/* 4G08 MLC(K9G4G08) Mono */
					{0xD3, 0x55, WMR_MLC_LSB_CLASS1 },	/* 8G08 MLC(K9L8G08) DDP  */
					{0xD3, 0x14, WMR_MLC_LSB_CLASS1 },	/* 8G08 MLC(K9G8G08) Mono */
					{0xD5, 0x55, WMR_MLC_LSB_CLASS1 },	/* 16Gb MLC(K9LAG08) DDP  */
					{0xD5, 0x14, WMR_MLC_LSB_CLASS2 },	/* 16Gb MLC(K9GAG08) Mono */			
					{0xD7, 0x55, WMR_MLC_LSB_CLASS2 }	/* 32Gb MLC(K9LBG08) DDP  */	
		  		};
	UINT32 nIdx;

	for( nIdx=0; nIdx<6; nIdx++)
	{
		if ( stMlc[nIdx].nDevID == nDID && stMlc[nIdx].nHidID == nHID )
		{
			return stMlc[nIdx].nClass;
		}
	}

	/* return (WMR_MLC_NO); */
	return (WMR_MLC_LSB_CLASS1);
#else
    // Every device use Class 2 paired page mapping.
	return (WMR_MLC_LSB_CLASS2);
#endif
}
#endif

