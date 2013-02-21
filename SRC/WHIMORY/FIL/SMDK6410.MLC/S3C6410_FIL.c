///////////////////////////////////////////////////////////////
//
//	MODULE		: FIL
//	NAME		: S3C6410X Flash Interface Layer
//	FILE			: S3C6410_FIL.c
//	PURPOSE		:
//
///////////////////////////////////////////////////////////////
//
//		COPYRIGHT 2003-2006 SAMSUNG ELECTRONICS CO., LTD.
//					ALL RIGHTS RESERVED
//
//	Permission is hereby granted to licensees of Samsung Electronics
//	Co., Ltd. products to use or abstract this computer program for the
//	sole purpose of implementing a product based on Samsung
//	Electronics Co., Ltd. products. No other rights to reproduce, use,
//	or disseminate this computer program, whether in part or in whole,
//	are granted.
//
//	Samsung Electronics Co., Ltd. makes no representation or warranties
//	with respect to the performance of this computer program, and
//	specifically disclaims any responsibility for any damages,
//	special or consequential, connected with the use of this program.
//
///////////////////////////////////////////////////////////////
//
//	REVISION HISTORY
//
//	2006.10.19	dodan2(gabjoo.lim@samsung.com)
//				Draft Version
//	2007.03.25	ksk
//				Support 4KByte/Page NAND Flash Device
//
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////
// Header File
///////////////////////////////////////////////
#include <windows.h>
#include <WMRConfig.h>
#include <WMRTypes.h>
#include <OSLessWMROAM.h>
#include <FIL.h>
#include <string.h>
#include "S3C6410_FIL.h"
#include "S3C6410.h"
//#include "s3c6410_dma.h"
//#include "s3c6410_nand.h"
//#include "s3c6410_syscon.h"
//#include "s3c6410_matrix.h"

//#define USE_SETKMODE
#define USE_2CE_NAND

///////////////////////////////////////////////
// Transfer Mode
///////////////////////////////////////////////
#define ASM					(0)		// Assembly Code
#define DMA					(1)		// DMA Transfer

///////////////////////////////////////////////
// NAND DMA Buffer
///////////////////////////////////////////////
//#define NAND_DMA_BUFFER_UA	(0xA0100000+0x25800)		// LCD Frame Buffer
//#define NAND_DMA_BUFFER_PA	(0x30100000+0x25800)
#ifdef USING_DMA
#define NAND_DMA_BUFFER_UA	(0xB0700000)		// Stepping Stone (Check Oemaddrtab_cgf.inc !!!!)
#define NAND_DMA_BUFFER_PA	(0x40000000)
#endif
///////////////////////////////////////////////
// Debug Print Macro
///////////////////////////////////////////////
#define RETAILMSG(cond, printf_exp)	((cond)?(NKDbgPrintfW printf_exp),1:0)
#define NAND_ERR(x)	RETAILMSG(TRUE, x)
//#define NAND_ERR(x)
//#define NAND_MSG(x)	RETAILMSG(TRUE, x)
#define NAND_MSG(x)
#define NAND_LOG(x)	RETAILMSG(TRUE, x)
//#define NAND_LOG(x)

///////////////////////////////////////////////
// Device type context definitions
///////////////////////////////////////////////
typedef struct
{
	UINT8	nDevID;				// Device ID
	UINT8	nHidID;				// Hidden ID
	UINT32	nNumOfBlocks;		// Number of Blocks
	UINT32	nPagesPerBlock;		// Pages per block
	UINT32	nSectorsPerPage;	// Sectors per page
	BOOL32	b2XProgram;		// 2 plane program
	BOOL32	b2XRead;						/* 2 plane read	 	 */
	BOOL32	b2XReadStatus;						/* 2 plane read status	 	 */
	BOOL32	bDualDie;			// internal interleaving
	BOOL32	bMLC;				// MLC
	BOOL32  b8BitECC;
} DEVInfo;

typedef struct
{
	UINT32	n8MECC0;				// 8MECC0
	UINT32	n8MECC1;				// 8MECC1
	UINT32	n8MECC2;				// 8MECC2
	UINT8	n8MECC3;				// 8MECC3
} MECC8;


typedef struct
{
	UINT8	nBadBlock;				// bad block marker;
	MECC8	t8MECC[8];				// 8MECC0 ~ 8
} SECCInfo;


PRIVATE const DEVInfo stDEVInfo[] = {
		/*****************************************************************************/
		/* Device ID																 */
		/*	   Hidden ID															 */
		/*			 Blocks															 */
		/*				   Pages per block											 */
		/*						Sectors per page									 */
		/*						   2X program										 */
		/*						   			2X read									 */
		/*											 2x status 						 */
		/*											 		  internal Interleaving	 */
		/*												               MLC			 */
		/*												               		   8bit ECC */
		/*****************************************************************************/
		/* MLC NAND ID TABLE */
		{0xDC, 0x14, 2048, 128, 4, TRUE32,  FALSE32, FALSE32, FALSE32, TRUE32, FALSE32},	/* 4Gb MLC(K9G4G08) Mono */
		{0xD3, 0x55, 4096, 128, 4, TRUE32,  FALSE32, FALSE32, TRUE32,  TRUE32, FALSE32},	/* 8Gb MLC(K9L8G08) DDP  */
		{0xD3, 0x14, 4096, 128, 4, TRUE32,  FALSE32, FALSE32, FALSE32, TRUE32, FALSE32},	/* 8Gb MLC(K9G8G08) Mono */
		{0xD5, 0x55, 8192, 128, 4, TRUE32,  FALSE32, FALSE32, TRUE32,  TRUE32, FALSE32},	/* 16Gb MLC(K9LAG08) DDP  */
		{0xD5, 0x14, 4096, 128, 8, TRUE32,  TRUE32,  TRUE32,  FALSE32, TRUE32, FALSE32},	/* 16Gb MLC(K9GAG08) Mono */
		{0xD5, 0x94, 4096, 128, 8, TRUE32,  TRUE32,  TRUE32,  FALSE32, TRUE32, TRUE32},		/* 16Gb MLC(K9GAG08) Mono */ // hsjang 080922
		{0xD7, 0x55, 8192, 128, 8, TRUE32,  TRUE32,  TRUE32,  TRUE32,  TRUE32, FALSE32},	/* 32Gb MLC(K9LBG08) DDP  */
		{0xD7, 0xD5, 8192, 128, 8, TRUE32,  TRUE32,  TRUE32,  TRUE32,  TRUE32, TRUE32},	    /* 32Gb MLC(K9LBG08) DDP  */
};

///////////////////////////////////////////////
// PRIVATE variables definitions
///////////////////////////////////////////////
PRIVATE BOOL32 bInternalInterleaving	= FALSE32;
PRIVATE 	BOOL32 aNeedSync[WMR_MAX_DEVICE * 2];
BOOL32 bReadSafeMode = FALSE32;
UINT32 gnPpn;
BOOL32 bECCEngineError = FALSE32;

PRIVATE volatile S3C6410_NAND_REG *pNANDFConReg = NULL;
PRIVATE volatile S3C6410_SYSCON_REG *pSYSConReg = NULL;
//PRIVATE volatile S3C6410_MATRIX_REG *pMatrixConReg = NULL;
PRIVATE volatile S3C6410_WATCHDOG_REG *pWTDogReg = NULL;      // VA for Watchdog base

PRIVATE UINT8 aTempSBuf[NAND_SPAREPAGE_SIZE] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

///////////////////////////////////////////////
// Private Function Prototype
///////////////////////////////////////////////
PRIVATE INT32	Read_DeviceID(UINT32 nBank, UINT8 *pDID, UINT8 *pHID);
PRIVATE UINT32	Read_Sector(UINT32 nBank, UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf, UINT32* pSpareCxt, BOOL32 bCheckAllFF);
PRIVATE UINT32	Read_Sector_8BitECC(UINT32 nBank, UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf, UINT32* pSpareCxt, BOOL32 bCheckAllFF);
PRIVATE UINT32	Read_Spare(UINT32 nBank, UINT32 nPpn, UINT32* pSpareCxt);
PRIVATE UINT32	Read_Spare_Separate(UINT32 nBank, UINT32 nPpn, UINT32* pSpareCxt);
PRIVATE UINT32	Read_Spare_8BitECC(UINT32 nBank, UINT32 nPpn, UINT32* pSpareCxt);
PRIVATE VOID	Write_Sector(UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf);
PRIVATE VOID	Write_Sector_8BitECC(UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf);
PRIVATE VOID	Write_Spare(UINT32 nBank, UINT32 nPpn, pSECCCxt pSpareCxt);
PRIVATE VOID	Write_Spare_Separate(UINT32 nBank, UINT32 nPpn, pSECCCxt pSpareCxt);
PRIVATE VOID	Write_Spare_8BitECC(UINT32 nBank, UINT32 nPpn, pSECCCxt pSpareCxt);
PRIVATE UINT32	Decoding_MainECC(UINT8* pBuf);
PRIVATE UINT32	Decoding_Main8BitECC(UINT8* pBuf);
PRIVATE UINT32	Decoding_SpareECC(UINT8* pBuf);
PRIVATE UINT32	Decoding_Spare8BitECC(UINT8* pBuf, UINT32 nEffectiveByte);
PRIVATE UINT32	_IsAllFF(UINT8* pBuf, UINT32 nSize);
PRIVATE UINT32	_TRDelay(UINT32 nNum);
PRIVATE UINT32	_TRDelay2(UINT32 nNum);

PRIVATE VOID    MLC_Print_Page_Data(unsigned char *pMBuf, unsigned char *pSBuf);
PRIVATE VOID    MLC_Print_SFR(VOID);

///////////////////////////////////////////////
// ECC Decoding Function Return Value
///////////////////////////////////////////////
#define ECC_CORRECTABLE_ERROR					(0x1)
#define ECC_UNCORRECTABLE_ERROR				(0x2)
#define ECC_NEED_DECODING_AGAIN					(0x4)
//#define PAGE_CORRECTABLE_ERROR_MASK			(0x11111)  // for 2KByte/Page
//#define PAGE_UNCORRECTABLE_ERROR_MASK		(0x22222)  // for 2KByte/Page
#define PAGE_CORRECTABLE_ERROR_MASK			(0x15555)  // for 4KByte/Page
#define PAGE_UNCORRECTABLE_ERROR_MASK		(0x2AAAA)  // for 4KByte/Page
#define ECCVAL_ALLFF0  (0xFFFFFFFF)
#define ECCVAL_ALLFF1  (0xFFFFFFFF)
#define ECCVAL_ALLFF2  (0xFFFFFFFF)
#define ECCVAL_ALLFF3  (0xFFFFFFFF)

extern void _Read_512Byte(UINT8* pBuf);
extern void _Read_512Byte_Unaligned(UINT8* pBuf);
extern void _Write_512Byte(UINT8* pBuf);
extern void _Write_512Byte_Unaligned(UINT8* pBuf);
extern void _Write_Dummy_500Byte_AllFF(void);
extern void _Write_Dummy_492Byte_AllFF(void);
extern void _Write_Dummy_480Byte_AllFF(void);
extern void _Write_Dummy_468Byte_AllFF(void);
extern void _Write_Dummy_448Byte_AllFF(void);
extern void _Write_Dummy_428Byte_AllFF(void);
extern void _Write_Dummy_364Byte_AllFF(void);
extern void _STOP_FOR_BREAK(void);

// WTCON - control register, bit specifications
#define WTCON_PRESCALE(x)        (((x)&0xff)<<8)    // bit 15:8, prescale value, 0 <= (x) <= 27
#define WTCON_ENABLE            (1<<5)            // bit 5, enable watchdog timer
#define WTCON_CLK_DIV16        (0<<3)
#define WTCON_CLK_DIV32        (1<<3)
#define WTCON_CLK_DIV64        (2<<3)
#define WTCON_CLK_DIV128        (3<<3)
#define WTCON_INT_ENABLE        (1<<2)
#define WTCON_RESET            (1<<0)


// WTCNT - watchdog counter register
#define WTCNT_CNT(x)            ((x)&0xffff)
// WTDAT - watchdog reload value register
#define WTDAT_CNT(x)            ((x)&0xffff)
// WTCLRINT - watchdog interrupt clear register
#define WTCLRINT_CLEAR            (1<<0)
// Watchdog Clock
// PCLK : 25MHz
// PCLK/PRESCALER : 25/25 = 1MHz
// PCLK/PRESCALER/DIVIDER : 1MHz/128 = 7.812 KHz
// MAX Counter = 0xffff = 65535
// Period = 65535/7812 =~ 8.4 sec
#define WD_PRESCALER            (25-1)
#define WD_REFRESH_PERIOD        3000    // tell the OS to refresh watchdog every 3 second.


///////////////////////////////////////////////
// Code Implementation
///////////////////////////////////////////////

#pragma optimize ("",off)

BOOL32 NF_WAIT_ECC_DEC_DONE(volatile S3C6410_NAND_REG *pNANDFConReg)
{
    volatile int timeout;

	for ( timeout = 0; timeout < 1000; timeout++ )   // It need 155 cycle, but set it to enough value.
	{
	    if ( (pNANDFConReg->NFSTAT&NF_ECC_DEC_DONE) == NF_ECC_DEC_DONE )
	    {
	        return TRUE32;
	    }
	}

    bECCEngineError = TRUE32;

    return FALSE32;
}

#pragma optimize ("",on)


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_Init		                                                     */
/* DESCRIPTION                                                               */
/*      This function inits NAND device.							 		 */
/* PARAMETERS                                                                */
/*      None													             */
/* RETURN VALUES                                                             */
/*		FIL_SUCCESS															 */
/*					NAND_Init is success.									 */
/*		FIL_CRITICAL_ERROR													 */
/*					NAND_Init is failed.									 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
NAND_Init(VOID)
{
	UINT32 nDevIdx;
	UINT32 nScanIdx = 0, nCompIdx = 0;
	UINT8 nDevID, nHiddenID;
	UINT32 nDevCnt = 0;
	BOOL32 bComp = FALSE32;

	NAND_MSG((_T("[FIL]++NAND_MLC_Init()\r\n")));

	pNANDFConReg = (volatile S3C6410_NAND_REG *)0xB0200000;			// 0x4E000000 : 0x70200000
	//pSYSConReg   = (volatile S3C6410_SYSCON_REG *)0x92A00000;                // 0x7E00F000
	pSYSConReg   = (volatile S3C6410_SYSCON_REG *)0xB2A0F000;
	pWTDogReg    = (volatile S3C6410_WATCHDOG_REG *)0xB2A04000;

	// Configure BUS Width and Chip Select for NAND Flash
    pSYSConReg->MEM_SYS_CFG = (pSYSConReg->MEM_SYS_CFG & ~(0x1000)); // 8-bit data width
    pSYSConReg->MEM_SYS_CFG = (pSYSConReg->MEM_SYS_CFG & ~(0x3F)) | (0x00); // Xm0CSn[2] = NFCON CS0

	// Initialize NAND Flash Controller for MLC NAND Flash
	pNANDFConReg->NFCONF = NF_4BIT_ECC | NF_TACLS(DEFAULT_TACLS) | NF_TWRPH0(DEFAULT_TWRPH0) | NF_TWRPH1(DEFAULT_TWRPH1);
	pNANDFConReg->NFCONT = NF_MAIN_ECC_LOCK | NF_SPARE_ECC_LOCK | NF_INIT_MECC | NF_INIT_SECC | NF_NFCE1 | NF_NFCE0 | NF_NFCON_EN;
	pNANDFConReg->NFSTAT = NF_RNB_READY;	// Clear RnB Transition Detect Bit

	// Device's ID must be available and equal to each other
	for(nDevIdx = 0; nDevIdx < WMR_MAX_DEVICE; nDevIdx++)
	{
		nScanIdx = Read_DeviceID(nDevIdx, &nDevID, &nHiddenID);

		if (nScanIdx == FIL_CRITICAL_ERROR)
		{
			nScanIdx = nCompIdx;
			break;
		}
		if ((nCompIdx != nScanIdx) && (bComp))
		{
			return FIL_CRITICAL_ERROR;
		}

		nCompIdx = nScanIdx;
		bComp = TRUE32;
		nDevCnt++;
	}

	if(stDEVInfo[nScanIdx].bMLC)
	{
		SET_DevType(WMR_MLC);
	}
	else
	{
		SET_DevType(WMR_SLC);
	}

	if(stDEVInfo[nScanIdx].bDualDie && nDevCnt <= 2)
	{
		/* multi chip dual die (DDP) */
		BLOCKS_PER_BANK = stDEVInfo[nScanIdx].nNumOfBlocks >> 1;
		BANKS_TOTAL = nDevCnt * 2;
		bInternalInterleaving = TRUE32;
	}
	else
	{
		BLOCKS_PER_BANK = stDEVInfo[nScanIdx].nNumOfBlocks;
		BANKS_TOTAL = nDevCnt;
		bInternalInterleaving = FALSE32;
	}

	SECTORS_PER_PAGE = stDEVInfo[nScanIdx].nSectorsPerPage;

	TWO_PLANE_PROGRAM = stDEVInfo[nScanIdx].b2XProgram;

	if (TWO_PLANE_PROGRAM == TRUE32)
	{
		BLOCKS_PER_BANK /= 2;
	}

	TWO_PLANE_READ = stDEVInfo[nScanIdx].b2XRead;

	TWO_PLANE_READ_STATUS = stDEVInfo[nScanIdx].b2XReadStatus;

	PAGES_PER_BLOCK = stDEVInfo[nScanIdx].nPagesPerBlock;

	ECC_8BIT_SUPPORT = stDEVInfo[nScanIdx].b8BitECC; // hsjang 080923 to support 8Bit ecc

	if (ECC_8BIT_SUPPORT == TRUE32)
	{
        NF_SETREG_8BITECC();
    }

	// FS_SCAN_RATIO means scan percentage of Total FTL size.
	// If FS_SCAN_RATIO value is 0, skip Scan.
	// OS_SCAN_RATIO means scan percentage of Total SPECIAL AREA size.
	// If OS_SCAN_RATIO value is 0, skip Scan.
    OS_SCAN_RATIO = 10;    // 10 means (100/10)% => 10%
	if ( stDEVInfo[nScanIdx].nNumOfBlocks <= 1024 )
	{
	    FS_SCAN_RATIO = 20;    // 10 means (100/10)% => 10%
	}
	else if ( stDEVInfo[nScanIdx].nNumOfBlocks <= 2048 )
	{
	    FS_SCAN_RATIO = 50;    // 20 means (100/20)% => 5%
	}
	else if ( stDEVInfo[nScanIdx].nNumOfBlocks <= 4096 )
	{
	    FS_SCAN_RATIO = 100;    // 50 means (100/50)% => 2%
	}
	else if ( stDEVInfo[nScanIdx].nNumOfBlocks <= 8192 )
	{
	    FS_SCAN_RATIO = 200;    // 100 means (100/100)% => 1%
	}
	else
	{
	    FS_SCAN_RATIO = 400;    // 200 means (100/200)% => 0.5%
	}

	// CRITICAL_READ_CNT value is used to avoid read disturbance problem.
	// This value is critical count value to determine the count of read for reclaim.
	// Every block have each read count value, this value is reset to 0 when boot up the system.
	// If each block is read more than CRITICAL_READ_CNT value after boot up, this block is enter to reclaim list.
	// This value is useful if the system is running long times without reboot.
	// But system have to calls FTL_Reclaim function during running the system.
	// If this CRITICAL_READ_CNT value is set to 0. It doesn't support Reclaim function using read count value.
	// If you support periodic scan and reclaim by OnDisk or other application, you don't need to use this function.
	CRITICAL_READ_CNT = 0;

	// This flag determin the scan function during boot up.
	BOOTING_SCAN = FALSE32;

    // Set to enable/disable printing the further DBGMSG.
    WMR_ERR_INFO_FLAG_POS_BN = 0;       // the number of virtual block in bank 0 has the flag indicated enable/disable the further DBGMSG from OEM
                                        // Don't set WMR_ERR_INFO_FLAG_POS_BN with "0", if you want to enable further DBGMSG.
    WMR_ERR_INFO_FLAG_POS_PN = 0;       // the number of page in the block from OEM

	/* DDP */
	if (bInternalInterleaving)
	{
		for (nDevIdx = 0; nDevIdx < nDevCnt; nDevIdx++)
		{
			aNeedSync[nDevIdx * 2] = FALSE32;
			aNeedSync[nDevIdx * 2 + 1] = FALSE32;
		}
	}

	#if (WMR_MLC_LSB_RECOVERY)
	MLC_LSB_CLASS = GetMlcClass( stDEVInfo[nScanIdx].nDevID,
								 stDEVInfo[nScanIdx].nHidID);
	#endif


	CalcGlobal(bInternalInterleaving);

	NAND_LOG((_T("[FIL] ##############################\r\n")));
	NAND_LOG((_T("[FIL]  FIL Global Information\r\n")));
	NAND_LOG((_T("[FIL]  BANKS_TOTAL = %d\r\n"), BANKS_TOTAL));
	NAND_LOG((_T("[FIL]  BLOCKS_PER_BANK = %d\r\n"), BLOCKS_PER_BANK));
	NAND_LOG((_T("[FIL]  TWO_PLANE_PROGRAM = %d\r\n"), TWO_PLANE_PROGRAM));
	NAND_LOG((_T("[FIL]  SUPPORT_INTERLEAVING = %d\r\n"), IS_SUPPORT_INTERLEAVING));
	NAND_LOG((_T("[FIL]  SUBLKS_TOTAL = %d\r\n"), SUBLKS_TOTAL));
	NAND_LOG((_T("[FIL]  PAGES_PER_SUBLK = %d\r\n"), PAGES_PER_SUBLK));
	NAND_LOG((_T("[FIL]  PAGES_PER_BANK = %d\r\n"), PAGES_PER_BANK));
	NAND_LOG((_T("[FIL]  SECTORS_PER_PAGE = %d\r\n"), SECTORS_PER_PAGE));
	NAND_LOG((_T("[FIL]  SECTORS_PER_SUPAGE = %d\r\n"), SECTORS_PER_SUPAGE));
	NAND_LOG((_T("[FIL]  SECTORS_PER_SUBLK = %d\r\n"), SECTORS_PER_SUBLK));
	NAND_LOG((_T("[FIL]  BYTES_PER_MAIN_PAGE = %d\r\n"), BYTES_PER_MAIN_PAGE));
	NAND_LOG((_T("[FIL]  BYTES_PER_SPARE_PAGE = %d\r\n"), BYTES_PER_SPARE_PAGE));
	NAND_LOG((_T("[FIL]  BYTES_PER_SPARE_SUPAGE = %d\r\n"), BYTES_PER_SPARE_SUPAGE));
	NAND_LOG((_T("[FIL]  USER_SECTORS_TOTAL = %d\r\n"), USER_SECTORS_TOTAL));
	NAND_LOG((_T("[FIL]  ADDRESS_CYCLE = %d\r\n"), DEV_ADDR_CYCLE));
	NAND_LOG((_T("[FIL] ##############################\r\n\r\n")));
	NAND_LOG((_T("[INFO] WMR_AREA_SIZE = %d\n"), WMR_AREA_SIZE));
	NAND_LOG((_T("[INFO] SPECIAL_AREA_START = %d\n"), SPECIAL_AREA_START));
	NAND_LOG((_T("[INFO] SPECIAL_AREA_SIZE = %d\n"), SPECIAL_AREA_SIZE));
	NAND_LOG((_T("[INFO] VFL_AREA_START = %d\n"), VFL_AREA_START));
	NAND_LOG((_T("[INFO] VFL_AREA_SIZE = %d\n"), VFL_AREA_SIZE));
	NAND_LOG((_T("[INFO] VFL_INFO_SECTION_START = %d\n"), VFL_INFO_SECTION_START));
	NAND_LOG((_T("[INFO] VFL_INFO_SECTION_SIZE = %d\n"), VFL_INFO_SECTION_SIZE));
	NAND_LOG((_T("[INFO] RESERVED_SECTION_START = %d\n"), RESERVED_SECTION_START));
	NAND_LOG((_T("[INFO] RESERVED_SECTION_SIZE = %d\n"), RESERVED_SECTION_SIZE));
	NAND_LOG((_T("[INFO] FTL_INFO_SECTION_START = %d\n"), FTL_INFO_SECTION_START));
	NAND_LOG((_T("[INFO] FTL_INFO_SECTION_SIZE = %d\n"), FTL_INFO_SECTION_SIZE));
	NAND_LOG((_T("[INFO] LOG_SECTION_SIZE = %d\n"), LOG_SECTION_SIZE));
	NAND_LOG((_T("[INFO] FREE_SECTION_START = %d\n"), FREE_SECTION_START));
	NAND_LOG((_T("[INFO] FREE_SECTION_SIZE = %d\n"), FREE_SECTION_SIZE));
	NAND_LOG((_T("[INFO] FREE_LIST_SIZE = %d\n"), FREE_LIST_SIZE));
	NAND_LOG((_T("[INFO] DATA_SECTION_START = %d\n"), DATA_SECTION_START));
	NAND_LOG((_T("[INFO] DATA_SECTION_SIZE = %d\n"), DATA_SECTION_SIZE));
	NAND_LOG((_T("[INFO] FTL_AREA_START = %d\n"), FTL_AREA_START));
	NAND_LOG((_T("[INFO] FTL_AREA_SIZE = %d\n"), FTL_AREA_SIZE));

	NAND_MSG((_T("[FIL]--NAND_Init()\r\n")));

	return FIL_SUCCESS;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_GetPlatformInfo                                                 */
/* DESCRIPTION                                                               */
/*      This function returns platform information                           */
/* PARAMETERS                                                                */
/*      pstFILPlantformInfo                                                  */
/*          Platform information                                             */
/* RETURN VALUES                                                             */
/*****************************************************************************/
VOID
NAND_GetPlatformInfo(FILPlatformInfo* pstFILPlatformInfo)
{
	NAND_LOG((_T("[INFO] ++NAND_GetPlatformInfo\n")));
    /* Warning : Do not change !!!! */
    pstFILPlatformInfo->nType = 1;
    /* Address of command register in MAP11 */
    pstFILPlatformInfo->nAddrOfCmdReg = 0xB0200008;
    /* Address of address register */
    pstFILPlatformInfo->nAddrOfAdrReg = 0xB020000C;
    /* Address of register for reading ID*/
    pstFILPlatformInfo->nAddrOfReadIDReg = 0xB0200010;
    /* Address of status register */
    pstFILPlatformInfo->nAddrOfStatusReg = 0xB0200028;
    /* Command of reading Device ID  */
    pstFILPlatformInfo->nCmdOfReadID = CMD_READ_ID;
    /* Command of read page */
    pstFILPlatformInfo->nCmdOfReadPage = CMD_READ;
    /* Command of read status */
    pstFILPlatformInfo->nCmdOfReadStatus = CMD_READ_STATUS;
    /* Mask value for Ready or Busy status */
    pstFILPlatformInfo->nMaskOfRnB = NF_RNB_READY;

	// Must be set CE to low
	NF_CE_L(0);
	NF_WAIT_RnB(0);

	NAND_LOG((_T("[INFO] --NAND_GetPlatformInfo\n")));
    return;
}

INT32
NAND_Read_Retry(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap, UINT32 nPlaneBitmap,
				UINT8* pDBuf, UINT8* pSBuf, BOOL32 bECCIn, BOOL32 bCleanCheck)
{
    INT32 dwRet;
    INT32 dwCnt;

    dwRet = NAND_Read( nBank, nPpn, nSctBitmap, nPlaneBitmap, pDBuf, pSBuf, bECCIn, bCleanCheck);

    if (bECCEngineError == TRUE32)
    {
        goto ECCPROBLEM;
    }
    // Try read again.
    for ( dwCnt = 0; dwCnt < 3; dwCnt++ )
    {
        if ( dwRet == FIL_U_ECC_ERROR )
        {
            NAND_LOG((_T("[FIL] NAND_Read Try to read again(%d)\n"), dwCnt));
            bReadSafeMode = TRUE32;
            dwRet = NAND_Read( nBank, nPpn, nSctBitmap, nPlaneBitmap, pDBuf, pSBuf, bECCIn, bCleanCheck);
            bReadSafeMode = FALSE32;
            if (bECCEngineError == TRUE32)
            {
                goto ECCPROBLEM;
            }
        }
        else
        {
            break;
        }
    }
    return dwRet;

ECCPROBLEM:
    {
       	nSctBitmap &= FULL_SECTOR_BITMAP_PAGE;

        if ( pDBuf != NULL )
        {
			if ((nSctBitmap & LEFT_SECTOR_BITMAP_PAGE) > 0 && (nSctBitmap & RIGHT_SECTOR_BITMAP_PAGE) > 0)
			{
                WMR_MEMSET(pDBuf, 0x0, BYTES_PER_MAIN_PAGE);
                WMR_MEMSET(pDBuf + BYTES_PER_MAIN_PAGE, 0x0, BYTES_PER_MAIN_PAGE);
			}
			else if ((nSctBitmap & LEFT_SECTOR_BITMAP_PAGE) > 0 && (nSctBitmap & RIGHT_SECTOR_BITMAP_PAGE) == 0)
			{
                WMR_MEMSET(pDBuf, 0x0, BYTES_PER_MAIN_PAGE);
			}
			else if ((nSctBitmap & LEFT_SECTOR_BITMAP_PAGE) == 0 && (nSctBitmap & RIGHT_SECTOR_BITMAP_PAGE) > 0)
			{
                WMR_MEMSET(pDBuf + BYTES_PER_MAIN_PAGE, 0x0, BYTES_PER_MAIN_PAGE);
			}
        }
        NAND_ERR((_T("[FIL] NAND_Write(0x%x)\n"), nPpn));
        NAND_Write( nBank, nPpn, nSctBitmap, nPlaneBitmap, pDBuf, pSBuf );

		// TODO:
        // Reset Device.
		// HCLK_IROM, HCLK_MEM1, HCLK_MEM0, HCLK_MFC Should be Always On for power Mode (Something coupled with BUS operation)
		pSYSConReg->HCLK_GATE |= ((1<<25)|(1<<22)|(1<<21)|(1<<0));

		pWTDogReg->WTCON = WTCON_PRESCALE(WD_PRESCALER) | WTCON_CLK_DIV128 | WTCON_RESET;
		pWTDogReg->WTDAT = WTDAT_CNT(0x1);
		pWTDogReg->WTCNT = WTCNT_CNT(0x1);
		pWTDogReg->WTCON |= WTCON_ENABLE;

        while(1);
    }
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_Read		                                                     */
/* DESCRIPTION                                                               */
/*      This function reads NAND page area							 		 */
/* PARAMETERS                                                                */
/*      nBank    	[IN] 	Physical device number			               	 */
/*      nPpn     	[IN] 	Physical page number				         	 */
/*      nSctBitmap  [IN] 	Physical sector bitmap in a page area        	 */
/*      nPlaneBitmap[IN]    The indicator of the plane  					 */
/*      pDBuf		[OUT]	Buffer pointer of main area to read          	 */
/*      pSBuf		[OUT]	Buffer pointer of spare area to read         	 */
/*      bECCIn		[IN] 	Whether read page with ECC value or not      	 */
/*      bCleanCheck [IN] 	When it's TRUE, checks the clean status        	 */
/*							of the page if the data of spare area is all	 */
/*							0xFF, returns PAGE_CLEAN						 */
/* RETURN VALUES                                                             */
/*		FIL_SUCCESS															 */
/*					NAND_Read is success.									 */
/*		FIL_SUCCESS_CLEAN													 */
/*					NAND_Read is success and all data is 0xFF.				 */
/*		FIL_U_ECC_ERROR														 */
/*					ECC value is not correct.								 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
NAND_Read(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap, UINT32 nPlaneBitmap,
				UINT8* pDBuf, UINT8* pSBuf, BOOL32 bECCIn, BOOL32 bCleanCheck)
{
	UINT32 nPbn;
	UINT32 nPOffset;

	UINT32 nPageReadStatus = 0;
	UINT32 nPageReadStatus1st = 0;
	UINT32 nPageReadStatus2nd = 0;
	UINT32 nCnt;
	UINT32 nRet = 0;

	BOOL32 bECCErr = FALSE32;
	BOOL32 bPageClean = TRUE32;		// When the data is all 0xFF, regard the page as clean
	BOOL32 bIsSBufNull = FALSE32;	// When the pSBuf is NULL, set to check whether the page is clean or not

	BOOL32 bSecondRead = FALSE32;	// In case of twice read
	BOOL32 bLoopNeed = FALSE32;		// Only for nSctOffset == 8
	BOOL32 bRetry = TRUE32;

	UINT32  nLoopCount;
	UINT32  nVBank;
	UINT32  nPairBank;

	UINT32  nSyncRet;

#ifdef	USE_SETKMODE
	BOOL bLastMode;
#endif

	NAND_MSG((_T("[FIL]++NAND_Read(%d, %d, 0x%02x, 0x%02x, %d)\r\n"), nBank, nPpn, nSctBitmap, nPlaneBitmap, bCleanCheck));

	// pDBuf & pSBuf can't be the NULL at the same time
    if ((nBank >= BANKS_TOTAL) || (nPpn >= PAGES_PER_BANK) || ((pDBuf == NULL) && (pSBuf == NULL)))
	{
		NAND_ERR((_T("[FIL:ERR]--NAND_Read() : Parameter overflow\r\n")));
		WMR_ASSERT(FALSE32);
		return FIL_CRITICAL_ERROR;
	}

	WMR_ASSERT((nPlaneBitmap == enuBOTH_PLANE_BITMAP) || (nPlaneBitmap == enuLEFT_PLANE_BITMAP)
	          || (nPlaneBitmap == enuRIGHT_PLANE_BITMAP) || (nPlaneBitmap == enuNONE_PLANE_BITMAP));

	nVBank = nBank;	// Do not change nBank before copy to nVBank
	// avoid r/b check error with internal interleaving
	if (bInternalInterleaving == TRUE32)
	{
		nPairBank = ((nBank & 0x1) == 1) ? (nBank - 1) : (nBank + 1);
	}

	/*
	In case of Internal Interleaving, the first address of the second bank should be
	the half of toal block number of NAND.
	For example, In 4Gbit DDP NAND, its total block number is 4096.
	So, the bank 0 has 2048 blocks (Physical number : 0 ~ 2047),
	the bank 1 has another 2048 blocks (Physical number : 2048 ~ 4095).
	therefore, first block address of Bank 1 could be the physically 2048th block.
	*/
	if (bInternalInterleaving == TRUE32)
	{
		if ((nBank & 0x1) == 1)
		{
			nPpn += PAGES_PER_BANK;
		}
		nBank /= 2;
	}

	nSctBitmap &= FULL_SECTOR_BITMAP_PAGE;

#if (WMR_STDLIB_SUPPORT)
	nPbn = nPpn / PAGES_PER_BLOCK;
	nPOffset = nPpn % PAGES_PER_BLOCK;
#else
	nPbn = DIV(nPpn, PAGES_PER_BLOCK_SHIFT);
	nPOffset = REM(nPpn, PAGES_PER_BLOCK_SHIFT);
#endif

	// In case of 2 Plane Program, re-calculate the page address
	if (TWO_PLANE_PROGRAM == TRUE32)
	{
		nPpn = nPbn * 2 * PAGES_PER_BLOCK + nPOffset;

		if (nPlaneBitmap == enuBOTH_PLANE_BITMAP)
		{
			if(pDBuf != NULL)
			{
				if ((nSctBitmap & LEFT_SECTOR_BITMAP_PAGE) > 0 && (nSctBitmap & RIGHT_SECTOR_BITMAP_PAGE) > 0)
				{
					// read from both plane
					bLoopNeed = TRUE32;
				}
				else if ((nSctBitmap & LEFT_SECTOR_BITMAP_PAGE) > 0 && (nSctBitmap & RIGHT_SECTOR_BITMAP_PAGE) == 0)
				{
					// read from left plane
					bLoopNeed = FALSE32;
				}
				else if ((nSctBitmap & LEFT_SECTOR_BITMAP_PAGE) == 0 && (nSctBitmap & RIGHT_SECTOR_BITMAP_PAGE) > 0)
				{
					// read from right plane
					bLoopNeed = FALSE32;
					bSecondRead = TRUE32;
				}
			}
			else
			{
				// When read only the spare area, must read twice
				bLoopNeed = TRUE32;
			}
		}
		else if (nPlaneBitmap == enuRIGHT_PLANE_BITMAP)
		{
			nPpn += PAGES_PER_BLOCK;
		}
	}

	nLoopCount = SECTORS_PER_PAGE;

	NAND_Sync(nVBank, &nSyncRet);

	if (bInternalInterleaving == TRUE32)
	{
		// avoid r/b check error with internal interleaving
		aNeedSync[nVBank] = FALSE32;
	}

_B_SecondRead:	// back here again for read right plane

	if(bSecondRead)
	{
		nPpn += PAGES_PER_BLOCK;
		nSctBitmap = (nSctBitmap >> SECTORS_PER_PAGE);

		if(pDBuf != NULL)
		{
			pDBuf += BYTES_PER_MAIN_PAGE;
		}
		else
		{
			// When read only the spare, read 64 + 64 bytes to check the Bad Block
			pSBuf += BYTES_PER_SPARE_PAGE;
		}
	}

#if (WMR_READ_RECLAIM)
	READ_ERR_FLAG = FALSE32;
#endif

	if(pSBuf == NULL)
	{
		pSBuf = aTempSBuf;
		bIsSBufNull = TRUE32;
	}

	//NAND_Sync(nVBank, &nSyncRet);

#ifdef	USE_SETKMODE
	bLastMode = SetKMode(TRUE);
#endif

	// Chip Select
	NF_CE_L(nBank);

	// Read Command (Always read spare area before main area in a page
	NF_CMD(CMD_READ);
    if ((ECC_8BIT_SUPPORT == FALSE32) && (SECTORS_PER_PAGE == 8))
    {
        NF_SET_ADDR(nPpn, (BYTES_PER_MAIN_PAGE+NAND_SECC_OFFSET_4K));
	}
	else
	{
    	NF_SET_ADDR(nPpn, BYTES_PER_MAIN_PAGE);
	}
	NF_CMD(CMD_READ_CONFIRM);

	if (bInternalInterleaving == TRUE32)
	{
#if	1
		_TRDelay(nPpn);
#else
		if (nVBank%2)
		{
			NF_CMD(CMD_READ_STATUS_CHIP1);
		}
		else
		{
			NF_CMD(CMD_READ_STATUS_CHIP0);
		}
		while(!(NF_DATA_R()&0x40));

		// Dummy Command to Set Proper Pointer to Read Position after NF_WAIT_RnB()
		NF_CMD(CMD_READ);
#endif
	}
	else
	{
		NF_WAIT_RnB(nBank);

		// Dummy Command to Set Proper Pointer to Read Position after NF_WAIT_RnB()
		NF_CMD(CMD_READ);
	}

	// Read Spare Area
	if (ECC_8BIT_SUPPORT == TRUE32)
	{
        NF_SETREG_8BITECC();
		nRet = Read_Spare_8BitECC(nBank, nPpn, (UINT32*)pSBuf);
	}
	else if (SECTORS_PER_PAGE == 8)
	{
        NF_SETREG_8BITECC();
		nRet = Read_Spare_Separate(nBank, nPpn, (UINT32*)pSBuf);
        NF_SETREG_4BITECC();
	}
	else
	{
		nRet = Read_Spare(nBank, nPpn, (UINT32*)pSBuf);
	}

	if ((nRet & ECC_UNCORRECTABLE_ERROR) == 0)
	{
		nPageReadStatus = (nRet<<16);

		if (WMR_GetChkSum(&pSBuf[1], 1) >= 4)
		{
			// Not Clean Page
			bPageClean = FALSE32;
		}

		// Read Main Area
		if(pDBuf != NULL)
		{
			for (nCnt=0; nCnt<nLoopCount; nCnt++)
			{
				if (nSctBitmap&(0x1<<nCnt))
				{
 					if (ECC_8BIT_SUPPORT == TRUE32)
					{
                        NF_SETREG_8BITECC();
						nRet = Read_Sector_8BitECC(nBank, nPpn, nCnt, pDBuf, (UINT32*)pSBuf, bPageClean);
					}
					else
					{
						nRet = Read_Sector(nBank, nPpn, nCnt, pDBuf, (UINT32*)pSBuf, bPageClean);
					}
					//nPageReadStatus |= (nRet<<(nCnt*4));  // if 2KByte/Page
					nPageReadStatus |= (nRet<<(nCnt*2));  // if both non-case for all-ff and 4KByte/Page
				}
			}
		}
	}
	else
	{
    	nPageReadStatus |= (nRet<<(8*2));  // The spare area position.
	}

	// Chip Unselect
	NF_CE_H(nBank);

#ifdef	USE_SETKMODE
	SetKMode(bLastMode);
#endif

	if(bSecondRead)
	{
		nPageReadStatus2nd = nPageReadStatus;
	}
	else
	{
		nPageReadStatus1st = nPageReadStatus;
	}

	if(bLoopNeed && !bSecondRead)
	{
		bSecondRead = TRUE32;
		goto _B_SecondRead;
	}

	if(nPageReadStatus1st&PAGE_UNCORRECTABLE_ERROR_MASK || nPageReadStatus2nd&PAGE_UNCORRECTABLE_ERROR_MASK)
	{
		// Uncorrectable ECC Error
		NAND_ERR((_T("[FIL:ERR]--NAND_Read() : Uncorrectable Error in Bank %d, Page %d [0x%08x] [0x%08x]\r\n"), nBank, nPpn, nPageReadStatus1st, nPageReadStatus));
		return FIL_U_ECC_ERROR;
	}
	else
	{

#if (WMR_READ_RECLAIM)
		if (nPageReadStatus1st&PAGE_CORRECTABLE_ERROR_MASK || nPageReadStatus2nd&PAGE_CORRECTABLE_ERROR_MASK)
		{
			READ_ERR_FLAG = TRUE32;
			NAND_MSG((_T("[FIL:INF] NAND_Read() : Correctable Error in Bank %d, Page %d [0x%08x] [0x%08x]\r\n"), nBank, nPpn, nPageReadStatus1st, nPageReadStatus2nd));
		}
#endif
		if (bCleanCheck&&bPageClean)
		{
			if (bIsSBufNull == FALSE32)
			{
				BOOL32 bClean;

				// Check 32 bytes is all 0xFF & don't care about ECC Value
				if ((pDBuf == NULL) && (bECCIn))
				{
					// When the pMBuf is NULL, read 128 bytes(twice read) in the spare area
					if (bSecondRead)
					{
						pSBuf -= BYTES_PER_SPARE_PAGE;
					}

					bClean = _IsAllFF(pSBuf, BYTES_PER_SPARE_SUPAGE);
				}
				else
				{
					// TODO: to be changed all FF check Size
					bClean = _IsAllFF(pSBuf, ((SECTORS_PER_PAGE == 8) ? NAND_MECC_OFFSET_4K : NAND_MECC_OFFSET));
				}

				if (bClean)
				{
					NAND_MSG((_T("[FIL]--NAND_Read() : FIL_SUCCESS_CLEAN\r\n")));
					return FIL_SUCCESS_CLEAN;
				}
				else
				{
					NAND_MSG((_T("[FIL]--NAND_Read()[bClean==FASLE32]\r\n")));
					return FIL_SUCCESS;
				}
			}
			else
			{
				NAND_MSG((_T("[FIL]--NAND_Read()[bIsSBufNull != FALSE32] : FIL_SUCCESS_CLEAN\r\n")));
				return FIL_SUCCESS_CLEAN;
			}
		}
		else
		{
			NAND_MSG((_T("[FIL]--NAND_Read()[bCleanCheck&&bPageClean == FASLE32]\r\n")));
			return FIL_SUCCESS;
		}
	}
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_Write		                                                     */
/* DESCRIPTION                                                               */
/*      This function writes NAND page area							 		 */
/* PARAMETERS                                                                */
/*      nBank    	[IN] 	Physical device number			               	 */
/*      nPpn     	[IN] 	Physical page number				         	 */
/*      nSctBitmap 	[IN] 	The indicator for the sector to write         	 */
/*      nPlaneBitmap[IN]    The indicator of the plane  					 */
/*      pDBuf		[IN]	Buffer pointer of main area to write          	 */
/*      pSBuf		[IN]	Buffer pointer of spare area to write         	 */
/*																			 */
/* RETURN VALUES                                                             */
/*		None																 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
NAND_Write(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap,
			UINT32  nPlaneBitmap, UINT8* pDBuf, UINT8* pSBuf)
{
	UINT32 nCnt;
	UINT32 nPbn;
	UINT32 nPOffset;

	//BOOL32	bFirstWrite = TRUE32;
	BOOL32	bSecondWrite = FALSE32;
	BOOL32  bLoopNeed = FALSE32;

	pSECCCxt pSpareCxt = NULL;

#ifdef	USE_SETKMODE
	BOOL bLastMode;
#endif

	NAND_MSG((_T("[FIL] ++NAND_Write(%d, %d, 0x%02x, 0x%02x)\r\n"), nBank, nPpn, nSctBitmap, nPlaneBitmap));

	if (nBank >= BANKS_TOTAL || nPpn >= PAGES_PER_BANK || (pDBuf == NULL && pSBuf == NULL))
	{
		NAND_ERR((_T("[FIL:ERR]--NAND_Write() : Parameter Overflow\r\n")));
		WMR_ASSERT(FALSE32);
		return;
	}

	// avoid r/b check error with internal interleaving
	if (bInternalInterleaving == TRUE32)
	{
		aNeedSync[nBank] = TRUE32;
	}

	/*
	In case of Internal Interleaving, the first address of the second bank should be
	the half of toal block number of NAND.
	For example, In 4Gbit DDP NAND, its total block number is 4096.
	So, the bank 0 has 2048 blocks (Physical number : 0 ~ 2047),
	the bank 1 has another 2048 blocks (Physical number : 2048 ~ 4095).
	therefore, first block address of Bank 1 could be the physically 2048th block.
	*/
	if (bInternalInterleaving == TRUE32)
	{
		if ((nBank & 0x1) == 1)
		{
			nPpn += PAGES_PER_BANK;
		}
		nBank /= 2;
	}

#if (WMR_STDLIB_SUPPORT)
	nPbn = nPpn / PAGES_PER_BLOCK;
	nPOffset = nPpn % PAGES_PER_BLOCK;
#else
	nPbn = DIV(nPpn, PAGES_PER_BLOCK_SHIFT);
	nPOffset = REM(nPpn, PAGES_PER_BLOCK_SHIFT);
#endif

	// In case of 2-Plane Program, re-calculate the page address
	if(TWO_PLANE_PROGRAM == TRUE32)
	{
		nPpn = nPbn * 2 * PAGES_PER_BLOCK + nPOffset;

		if (nPlaneBitmap == enuBOTH_PLANE_BITMAP)
		{
			bLoopNeed = TRUE32;
		}
		else if (nPlaneBitmap == enuRIGHT_PLANE_BITMAP)
		{
			nPpn += PAGES_PER_BLOCK;
			//nSctBitmap = nSctBitmap>>4;
		}
	}

	nSctBitmap &= FULL_SECTOR_BITMAP_PAGE;

#ifdef	USE_SETKMODE
	bLastMode = SetKMode(TRUE);
#endif

	// CE Select
	NF_CE_L(nBank);

_B_SecondWrite:

	// 2-Plane Program, page address is changed
	if (bSecondWrite)
	{
		nPpn += PAGES_PER_BLOCK;
		nSctBitmap = nSctBitmap >> SECTORS_PER_PAGE;
	}

	if(pSBuf == NULL)
	{
		pSBuf = aTempSBuf;
		WMR_MEMSET(pSBuf, 0xFF, BYTES_PER_SPARE_PAGE);		// Initialize the spare buffer
	}
	else
	{
		// Set 0xFF to ECC Area
		if (IS_CHECK_SPARE_ECC == TRUE32)
		{
			pSBuf[2] = 0xff;	// Reserved
			pSBuf[3] = 0xff;
			//WMR_MEMSET(pSBuf+2, 0xFF, 2);						// Clear Reserved area in Spare Buffer
			WMR_MEMSET(pSBuf+((SECTORS_PER_PAGE == 8) ? NAND_MECC_OFFSET_4K : NAND_MECC_OFFSET),
						0xFF,
						BYTES_PER_SPARE_PAGE-((SECTORS_PER_PAGE == 8) ? NAND_MECC_OFFSET_4K : NAND_MECC_OFFSET));		// Clear ECC area in Spare Buffer
		}
	}

	pSpareCxt = (pSECCCxt)pSBuf;
	pSpareCxt->cCleanMark = 0x0;	// Clean mark to 0. It means that page is written

	if (bSecondWrite)
	{
		NF_CMD(CMD_2PLANE_PROGRAM);
		NF_SET_ADDR(nPpn, 0);
	}
	else
	{
		NF_CMD(CMD_PROGRAM);
		NF_SET_ADDR(nPpn, 0);
	}

	// Write Main Sector
	if (pDBuf != NULL)
	{
		// In case of the second write, the position of buffer pointer is moved backward as much as 1 page size
		if (bSecondWrite)
		{
			pDBuf += BYTES_PER_MAIN_PAGE;
		}

		for (nCnt=0; nCnt<SECTORS_PER_PAGE; nCnt++)
		{
			if (nSctBitmap&(0x1<<nCnt))
			{
				if (ECC_8BIT_SUPPORT == TRUE32)
				{
                    NF_SETREG_8BITECC();
					Write_Sector_8BitECC(nPpn, nCnt, pDBuf);
				}
				else
				{
					Write_Sector(nPpn, nCnt, pDBuf);
				}

				if (IS_CHECK_SPARE_ECC == TRUE32)
				{
				    if ( ECC_8BIT_SUPPORT != TRUE32 )
				    {
    					pSpareCxt->aMECC[nCnt*2] = NF_MECC0();
    					pSpareCxt->aMECC[nCnt*2+1] = NF_MECC1() | (0xff<<24);
    				}
    				else
    				{
    					pSpareCxt->aMECC[nCnt*4] = NF_8MECC0();
    					pSpareCxt->aMECC[nCnt*4+1] = NF_8MECC1();
    					pSpareCxt->aMECC[nCnt*4+2] = NF_8MECC2();
    					pSpareCxt->aMECC[nCnt*4+3] = (NF_8MECC3() & 0xff) | (0xffffff<<8);
    				}
				}
			}
			else
			{
				if (ECC_8BIT_SUPPORT == TRUE32)
				{
					pSpareCxt->aMECC[nCnt*4] = ECCVAL_ALLFF0;
					pSpareCxt->aMECC[nCnt*4+1] = ECCVAL_ALLFF1;
					pSpareCxt->aMECC[nCnt*4+2] = ECCVAL_ALLFF2;
					pSpareCxt->aMECC[nCnt*4+3] = (ECCVAL_ALLFF3 & 0xff) | (0xffffff<<8);
    			}
    			else
    			{
    				pSpareCxt->aMECC[nCnt*2] = ECCVAL_ALLFF0;	// All 0xFF ECC
    				pSpareCxt->aMECC[nCnt*2+1] = ECCVAL_ALLFF1 | (0xff<<24);
    			}
    		}
		}
	}
	else if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		if (ECC_8BIT_SUPPORT == TRUE32)
		{
			for (nCnt=0; nCnt<SECTORS_PER_PAGE; nCnt++)
			{
				pSpareCxt->aMECC[nCnt*4] = ECCVAL_ALLFF0;
				pSpareCxt->aMECC[nCnt*4+1] = ECCVAL_ALLFF1;
				pSpareCxt->aMECC[nCnt*4+2] = ECCVAL_ALLFF2;
				pSpareCxt->aMECC[nCnt*4+3] = (ECCVAL_ALLFF3 & 0xff) | (0xffffff<<8);
			}
		}
		else
		{
			for (nCnt=0; nCnt<SECTORS_PER_PAGE; nCnt++)
			{
				pSpareCxt->aMECC[nCnt*2] = ECCVAL_ALLFF0;	// All 0xFF ECC
				pSpareCxt->aMECC[nCnt*2+1] = ECCVAL_ALLFF1 | (0xff<<24);
			}
		}
	}

	// Write Spare
	if (ECC_8BIT_SUPPORT == TRUE32)
	{
        NF_SETREG_8BITECC();
		Write_Spare_8BitECC(nBank, nPpn, pSpareCxt);
	}
	else if (SECTORS_PER_PAGE == 8)
	{
        NF_SETREG_8BITECC();
		Write_Spare_Separate(nBank, nPpn, pSpareCxt);
        NF_SETREG_4BITECC();
	}
	else
	{
		Write_Spare(nBank, nPpn, pSpareCxt);
	}
	// Write Confirm
	if(TWO_PLANE_PROGRAM == TRUE32 && !bSecondWrite && bLoopNeed)
	{
		bSecondWrite = TRUE32;
		NF_CMD(CMD_2PLANE_PROGRAM_DUMMY);

		_TRDelay2(0);

		goto _B_SecondWrite;
	}

	NF_CMD(CMD_PROGRAM_CONFIRM);

	// Chip Unselect
	NF_CE_H(nBank);

#ifdef	USE_SETKMODE
	SetKMode(bLastMode);
#endif

	NAND_MSG((_T("[FIL]--NAND_Write()\r\n")));

	return;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_Write_Steploader	                                             */
/* DESCRIPTION                                                               */
/*      This function writes NAND page area							 		 */
/* PARAMETERS                                                                */
/*      nBank    	[IN] 	Physical device number			               	 */
/*      nPpn     	[IN] 	Physical page number				         	 */
/*      nSctBitmap 	[IN] 	The indicator for the sector to write         	 */
/*      nPlaneBitmap[IN]    The indicator of the plane  					 */
/*      pDBuf		[IN]	Buffer pointer of main area to write          	 */
/*      pSBuf		[IN]	Buffer pointer of spare area to write         	 */
/*																			 */
/* RETURN VALUES                                                             */
/*		None																 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/

VOID
NAND_Steploader_Write(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap,
			UINT32  nPlaneBitmap, UINT8* pDBuf, UINT8* pSBuf)
{
	UINT32 nCnt;
	UINT32 nPbn;
	UINT32 nPOffset;
	UINT32 i;

	//BOOL32	bFirstWrite = TRUE32;
	BOOL32	bSecondWrite = FALSE32;
	BOOL32  bLoopNeed = FALSE32;

	SECCInfo tSECCInfo;


#ifdef	USE_SETKMODE
	BOOL bLastMode;
#endif

	NAND_MSG((_T("[FIL] ++NAND_Write(%d, %d, 0x%02x, 0x%02x)\r\n"), nBank, nPpn, nSctBitmap, nPlaneBitmap));
	if (nBank >= BANKS_TOTAL || nPpn >= PAGES_PER_BANK || (pDBuf == NULL && pSBuf == NULL))
	{
		NAND_ERR((_T("[FIL:ERR]--NAND_Write() : Parameter Overflow\r\n")));
		WMR_ASSERT(FALSE32);
		return;
	}


#if (WMR_STDLIB_SUPPORT)
	nPbn = nPpn / PAGES_PER_BLOCK;
	nPOffset = nPpn % PAGES_PER_BLOCK;
#else
	nPbn = DIV(nPpn, PAGES_PER_BLOCK_SHIFT);
	nPOffset = REM(nPpn, PAGES_PER_BLOCK_SHIFT);
#endif

	// In case of 2-Plane Program, re-calculate the page address
	if(TWO_PLANE_PROGRAM == TRUE32)
	{
		nPpn = nPbn * 2 * PAGES_PER_BLOCK + nPOffset;

		if (nPlaneBitmap == enuBOTH_PLANE_BITMAP)
		{
			bLoopNeed = TRUE32;
		}
		else if (nPlaneBitmap == enuRIGHT_PLANE_BITMAP)
		{
			nPpn += PAGES_PER_BLOCK;
			//nSctBitmap = nSctBitmap>>4;
		}
	}

	nSctBitmap &= FULL_SECTOR_BITMAP_PAGE;

#ifdef	USE_SETKMODE
	bLastMode = SetKMode(TRUE);
#endif

	pNANDFConReg->NFCONF = (pNANDFConReg->NFCONF & ~((1<<30)|(3<<23))) | (1<<23) | NF_TACLS(DEFAULT_TACLS) | NF_TWRPH0(DEFAULT_TWRPH0) | NF_TWRPH1(DEFAULT_TWRPH1); // System Clock is more than 66Mhz, ECC type is MLC.
	pNANDFConReg->NFCONT |= (1<<18)|(1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9); //ECC for programming.// Enable RnB Interrupt
	pNANDFConReg->NFSTAT |= ((1<<6)|(1<<5)|(1<<4));

	// CE Select
	NF_CE_L(nBank);

	NF_CMD(CMD_PROGRAM);
	NF_SET_ADDR(nPpn, 0);

	WMR_MEMSET(&tSECCInfo, 0xFF, 53);		// Initialize the spare buffer
	// Write Main Sector
	if (pDBuf != NULL)
	{
		for (nCnt=0; nCnt<SECTORS_PER_PAGE; nCnt++)
		{
			NF_MECC_UnLock();
			NF_MECC_Reset();

			if ((UINT32)pDBuf&0x3)
			{
				_Write_512Byte_Unaligned(pDBuf+NAND_SECTOR_SIZE*nCnt);
			}
			else
			{
				_Write_512Byte(pDBuf+NAND_SECTOR_SIZE*nCnt);
			}

			NF_MECC_Lock();

			while(!(pNANDFConReg->NFSTAT&(1<<7))) ;
			pNANDFConReg->NFSTAT|=(1<<7);

			tSECCInfo.t8MECC[nCnt].n8MECC0 = NF_8MECC0();
			tSECCInfo.t8MECC[nCnt].n8MECC1 = NF_8MECC1();
			tSECCInfo.t8MECC[nCnt].n8MECC2 = NF_8MECC2();
			tSECCInfo.t8MECC[nCnt].n8MECC3 = (NF_8MECC3() & 0xff);
		}


	}

//	NF_DATA_W(tSECCInfo.nBadBlock); // 1 byte n8MECC3 ==> bad block marker

	for(i = 0; i < SECTORS_PER_PAGE; i++) {
		NF_DATA_W4(tSECCInfo.t8MECC[i].n8MECC0); // 4 byte n8MECC0
		NF_DATA_W4(tSECCInfo.t8MECC[i].n8MECC1); // 4 byte n8MECC1
		NF_DATA_W4(tSECCInfo.t8MECC[i].n8MECC2); // 4 byte n8MECC2
		NF_DATA_W((tSECCInfo.t8MECC[i].n8MECC3) & 0xff); // 1 byte n8MECC3
	}

	pNANDFConReg->NFSTAT |=  (1<<4); //NF_CLEAR_RB
	NF_CMD(CMD_PROGRAM_CONFIRM);

	NF_WAIT_RnB(nBank);

	NF_CMD(CMD_READ_STATUS);   // Read status command

	for(i=0;i<3;i++);  //twhr=60ns

   	 if (NF_DATA_R()&NAND_STATUS_ERROR)
    	{// Page write error
		NF_CE_H(nBank);
		RETAILMSG(1,(TEXT("##### Error write operation #####\r\n")));
		pNANDFConReg->NFCONF = NF_4BIT_ECC | NF_TACLS(DEFAULT_TACLS) | NF_TWRPH0(DEFAULT_TWRPH0) | NF_TWRPH1(DEFAULT_TWRPH1);
		return;
	}
	else
	{
		NF_CE_H(nBank);
		pNANDFConReg->NFCONF = NF_4BIT_ECC | NF_TACLS(DEFAULT_TACLS) | NF_TWRPH0(DEFAULT_TWRPH0) | NF_TWRPH1(DEFAULT_TWRPH1);
		return;
	}
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_Erase		                                                     */
/* DESCRIPTION                                                               */
/*      This function erases NAND block area						 		 */
/* PARAMETERS                                                                */
/*      nBank    	[IN] 	Physical device number			               	 */
/*      nPpn     	[IN] 	Physical page number				         	 */
/*      nPlaneBitmap[IN]    The indicator of the plane  					 */
/*																			 */
/* RETURN VALUES                                                             */
/*		None																 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
NAND_Erase (UINT32 nBank, UINT32 nPbn, UINT32 nPlaneBitmap)
{
	UINT32	nVBank;
	UINT32	nPageAddr;
	UINT32 	nPairBank;
	UINT32  nSyncRet;

	BOOL32	bSecondErase = FALSE32;
	BOOL32	bLoopNeed = FALSE32;


#ifdef	USE_SETKMODE
	BOOL bLastMode;
#endif

	NAND_MSG((_T("[FIL]++NAND_Erase(%d, %d, 0x%02x)\r\n"), nBank, nPbn, nPlaneBitmap));
//	NAND_ERR((_T("[FIL]++NAND_Erase(%d, %d, 0x%02x)\r\n"), nBank, nPbn, nPlaneBitmap));

	if (nBank >= BANKS_TOTAL || nPbn >= BLOCKS_PER_BANK)
	{
    	NAND_ERR((_T("[FIL]++NAND_Erase(%d, %d, 0x%02x)\r\n"), nBank, nPbn, nPlaneBitmap));
		NAND_ERR((_T("[FIL:ERR]--NAND_Erase() : Parameter overflow\r\n")));
		WMR_ASSERT(FALSE32);
		return;
	}

	nVBank = nBank;

	// avoid r/b check error with internal interleaving
	if (bInternalInterleaving == TRUE32)
	{
		nPairBank = ((nBank & 0x1) == 1) ? (nBank - 1) : (nBank + 1);
	}

	/*
	In case of Internal Interleaving, the first address of the second bank should be
	the half of toal block number of NAND.
	For example, In 4Gbit DDP NAND, its total block number is 4096.
	So, the bank 0 has 2048 blocks (Physical number : 0 ~ 2047),
	the bank 1 has another 2048 blocks (Physical number : 2048 ~ 4095).
	therefore, first block address of Bank 1 could be the physically 2048th block.
	*/
	if (bInternalInterleaving == TRUE32)
	{
		if ((nBank & 0x1) == 1)
		{
			nPbn += BLOCKS_PER_BANK;
		}
		nBank /= 2;
	}

	if (TWO_PLANE_PROGRAM == TRUE32)
	{
		if (nPlaneBitmap == enuBOTH_PLANE_BITMAP)
		{
			bLoopNeed = TRUE32;
		}
		else if (nPlaneBitmap == enuRIGHT_PLANE_BITMAP)
		{
			bSecondErase = TRUE32;
		}
	}

	nPbn = nPbn*(1+(TWO_PLANE_PROGRAM == TRUE32));

	/*
	   In the Internal Interleaving, it's forbidden NAND to do Write operation & the other operation
	   at the same time. When Write is going on in Bank 1, Bank 0 has to wait to finish
	   the operation of Bank 1 if the next operation is not Write.

	   While Bank 1 is erased, Bank 0 do not start Write operation. (But, Erase or Read is allowed)
	   Internal Interleaving concept is only existed between Write and Write.
	*/
	if (bInternalInterleaving == TRUE32)
	{
		NAND_Sync(nVBank, &nSyncRet);

		// TODO: what does this means???
#if	1
		NF_CE_L(nBank);
		NF_CMD(CMD_READ);
		NF_CE_H(nBank);
#endif

		NAND_Sync(nPairBank, &nSyncRet);

		aNeedSync[nVBank] = TRUE32;
	}

#ifdef	USE_SETKMODE
	bLastMode = SetKMode(TRUE);
#endif

	// Chip Select
	NF_CE_L(nBank);

_B_SecondErase:

	if(bSecondErase)
	{
		nPbn++;
	}

	// Calculate Row Address of the Block (128 page/block)
	nPageAddr = (nPbn << (6 + IS_MLC));

	// Erase Command
	NF_CMD(CMD_ERASE);

	// Write Row Address
	NF_ADDR(nPageAddr&0xff);
	NF_ADDR((nPageAddr>>8)&0xff);
	NF_ADDR((nPageAddr>>16)&0xff);

	if (TWO_PLANE_PROGRAM == TRUE32 && !bSecondErase && bLoopNeed)
	{
		bSecondErase = TRUE32;
		goto _B_SecondErase;
	}

	// Erase confirm command
	NF_CMD( CMD_ERASE_CONFIRM);

	// Chip Unselect
	NF_CE_H(nBank);

#ifdef	USE_SETKMODE
	SetKMode(bLastMode);
#endif

	/* In the Internal Interleaving, it's forbidden NAND to do Write operation & the other operation
	   at the same time. When Write is going on in Bank 1, Bank 0 has to wait to finish
	   the operation of Bank 1 if the next operation is not Write.

	   While Bank 1 is erased, Bank 0 do not start Write operation. (But, Erase or Read is allowed)
	   Internal Interleaving concept is only existed between Write and Write.
	*/
	if (bInternalInterleaving == TRUE32)
	{
		NAND_Sync(nVBank, &nSyncRet);
	}

	NAND_MSG((_T("[FIL]--NAND_Erase()\r\n")));

	return;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      NAND_Sync		                                                     */
/* DESCRIPTION                                                               */
/*      This function checks the R/B signal of NAND					 		 */
/*		When it's busy, it means NAND is under operation.					 */
/*		When it's ready, it means NAND is ready for the next operation.		 */
/* PARAMETERS                                                                */
/*      nBank    	[IN] 	Physical device number			               	 */
/*																			 */
/* RETURN VALUES                                                             */
/*		FIL_CRITICAL_ERROR													 */
/*				When the input value is more than its range					 */
/*				or it has erase fail										 */
/*		FIL_SUCCESS															 */
/*				When the erase operation has done clearly					 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
NAND_Sync(UINT32 nBank, UINT32 *nPlaneBitmap)
{
	UINT32	nData;
	UINT32	nPairBank;
	UINT32	nVBank;

	INT32	nRet = FIL_SUCCESS;

	BOOL32	bInternalBank = FALSE32;

#ifdef	USE_SETKMODE
	BOOL bLastMode;
#endif

	NAND_MSG((_T("[FIL]++NAND_Sync(%d, %d)\r\n"), nBank, nPlaneBitmap));

	if (nBank >= BANKS_TOTAL)
	{
		NAND_ERR((_T("[FIL:ERR]--NAND_Sync() : Parameter overflow\r\n")));
		WMR_ASSERT(FALSE32);
		return FIL_CRITICAL_ERROR;
	}

	WMR_ASSERT(nPlaneBitmap != NULL);

	nVBank = nBank;

	// avoid r/b check error with internal interleaving
	if (bInternalInterleaving == TRUE32)
	{
		nPairBank = ((nBank & 0x1) == 1) ? (nBank - 1) : (nBank + 1);
	}

	if (bInternalInterleaving == TRUE32)
	{
		if ((nBank & 0x1) == 1)
		{
			bInternalBank = TRUE32;
		}
		nBank = (nBank >> 1);
	}

	if ((bInternalInterleaving == TRUE32) && (aNeedSync[nPairBank] == FALSE32))
	{
		// TODO: what does this means???
#if	1
		NF_CE_L(nBank);
		NF_ADDR(0x0);
		NF_CE_H(nBank);
#else
		BLUES_NF_ADDR0(0x0);
		rFMCTRL1 = (1 << BLUES_FM_ADDR_SET);

		BLUES_NF_CPU_CTRL0(nBank);
#endif
	}

#ifdef	USE_SETKMODE
	bLastMode = SetKMode(TRUE);
#endif

	// Chip Select
	NF_CE_L(nBank);

	if (bInternalInterleaving || TWO_PLANE_READ_STATUS)
	{
		if (bInternalBank)
		{
			NF_CMD(CMD_READ_STATUS_CHIP1);
		}
		else
		{
			NF_CMD(CMD_READ_STATUS_CHIP0);
		}
	}
	else
	{
		// Read Status Command is acceptable during Busy
		NF_CMD(CMD_READ_STATUS);
	}

	do
	{
		nData = NF_DATA_R();
	}
	while(!(nData&NAND_STATUS_READY));

	*nPlaneBitmap = enuNONE_PLANE_BITMAP;

	// Read Status
	if (nData&NAND_STATUS_ERROR)
	{
		if (TWO_PLANE_READ_STATUS == TRUE32)
		{
			if (nData & NAND_STATUS_PLANE0_ERROR)
			{
				NAND_ERR((_T("[FIL:ERR] NAND_Sync() : Left-plane Sync Error\r\n")));
				*nPlaneBitmap = enuLEFT_PLANE_BITMAP;
			}
			if (nData & NAND_STATUS_PLANE1_ERROR)
			{
				NAND_ERR((_T("[FIL:ERR] NAND_Sync() : Right-plane Sync Error\r\n")));
				*nPlaneBitmap = enuRIGHT_PLANE_BITMAP;
			}
		}
		else
		{
			NAND_ERR((_T("[FIL:ERR] NAND_Sync() : Status Error\r\n")));
			*nPlaneBitmap = enuLEFT_PLANE_BITMAP;
		}

		nRet = FIL_CRITICAL_ERROR;
	}

	// Chip Unselect
	NF_CE_H(nBank);

#ifdef	USE_SETKMODE
	SetKMode(bLastMode);
#endif

	NAND_MSG((_T("[FIL]--NAND_Sync()\r\n")));

	return nRet;
}

VOID
NAND_Reset(UINT32 nBank)
{
#ifdef	USE_SETKMODE
	BOOL bLastMode;
#endif

	NAND_MSG((_T("[FIL]++NAND_Reset(%d)\r\n"), nBank));

#ifdef	USE_SETKMODE
	bLastMode = SetKMode(TRUE);
#endif

	// Chip Select
	NF_CE_L(nBank);

	// Reset Command is accepted during Busy
	NF_CMD(CMD_RESET);

	// Chip Unselect
	NF_CE_H(nBank);

#ifdef	USE_SETKMODE
	SetKMode(bLastMode);
#endif

	NAND_MSG((_T("[FIL]--NAND_Reset()\r\n")));

	return;
}


VOID
NAND_PowerUp(VOID)
{
    pSYSConReg->MEM_SYS_CFG = (pSYSConReg->MEM_SYS_CFG & ~(0x1000)); // 8-bit data width
    pSYSConReg->MEM_SYS_CFG = (pSYSConReg->MEM_SYS_CFG & ~(0x3F)) | (0x00); // Xm0CSn[2] = NFCON CS0

	// Initialize NAND Flash Controller for MLC NAND Flash
	pNANDFConReg->NFCONF = NF_4BIT_ECC | NF_TACLS(DEFAULT_TACLS) | NF_TWRPH0(DEFAULT_TWRPH0) | NF_TWRPH1(DEFAULT_TWRPH1);
	pNANDFConReg->NFCONT = NF_MAIN_ECC_LOCK | NF_SPARE_ECC_LOCK | NF_INIT_MECC | NF_INIT_SECC | NF_NFCE1 | NF_NFCE0 | NF_NFCON_EN;
	pNANDFConReg->NFSTAT = NF_RNB_READY;	// Clear RnB Transition Detect Bit
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      Read_DeviceID		                                                 */
/* DESCRIPTION                                                               */
/*      This function reads manufacturer id, device id and hidden id. 		 */
/* PARAMETERS                                                                */
/*      nBank    [IN] 		Physical device number				             */
/*      pDID     [OUT] 		NAND flash density id							 */
/*		pHID	 [OUT]		NAND flash hidden id							 */
/* RETURN VALUES                                                             */
/*		nScanIdx			Device's stDEVInfo[nScanIdx]					 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE INT32
Read_DeviceID(UINT32 nBank, UINT8 *pDID, UINT8 *pHID)
{
	UINT8 nMID, nDID, nHID[3];
	UINT32 nScanIdx;
	UINT32 i;
#ifdef	USE_SETKMODE
	BOOL bLastMode;
#endif

	NAND_MSG((_T("[FIL]++Read_DeviceID(%d)\r\n"), nBank));

#ifdef	USE_SETKMODE
	bLastMode = SetKMode(TRUE);
#endif

	// Chip Select
	NF_CE_L(nBank);
	NF_WAIT_RnB(nBank);

	// Read ID Command
	NF_CMD(CMD_READ_ID);
	NF_ADDR(0x00);

	// Find Maker Code
	for (i=0; i<5; i++)
	{
		nMID = NF_DATA_R();		// Maker Code
		if (nMID == 0xEC) break;
	}

	// Read Device Code
	nDID = NF_DATA_R();		// Device Code
	nHID[0] = NF_DATA_R();	// Internal Chip Number
	nHID[1] = NF_DATA_R();	// Page, Block, Redundant Area Size
	nHID[2] = NF_DATA_R();	// Plane Number, Size

	// Chip Unselect
	NF_CE_H(nBank);

#ifdef	USE_SETKMODE
	SetKMode(bLastMode);
#endif

	for (nScanIdx = 0; nScanIdx < sizeof(stDEVInfo)/sizeof(DEVInfo); nScanIdx++)
	{
		if ((nMID == (UINT8)0xEC) && (nDID == stDEVInfo[nScanIdx].nDevID) && (nHID[0] == stDEVInfo[nScanIdx].nHidID))
		{
			*pDID = nDID;
			*pHID = nHID[0];

			NAND_LOG((_T("[FIL] ################\r\n")));
			NAND_LOG((_T("[FIL]  MID    = 0x%02x\r\n"), nMID));
			NAND_LOG((_T("[FIL]  DID    = 0x%02x\r\n"), nDID));
			NAND_LOG((_T("[FIL]  HID[0] = 0x%02x\r\n"), nHID[0]));
			NAND_LOG((_T("[FIL]  HID[1] = 0x%02x\r\n"), nHID[1]));
			NAND_LOG((_T("[FIL]  HID[2] = 0x%02x\r\n"), nHID[2]));
			NAND_LOG((_T("[FIL] ################\r\n")));

			NAND_MSG((_T("[FIL]  Bank %d Detect\r\n"), nBank));
			return nScanIdx;
		}
	}

	*pDID = 0x00;
	*pHID = 0x00;

	NAND_MSG((_T("[FIL]--Read_DeviceID()\r\n")));

	return FIL_CRITICAL_ERROR;

}


PRIVATE UINT32
Read_Sector(UINT32 nBank, UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf, UINT32* pSpareCxt, BOOL32 bCheckAllFF)
{
	UINT32 nOffSet;
	UINT32 nRet = 0;
	UINT32 nMECC[2];
	BOOL32 bECCDecDone = TRUE;

	NAND_MSG((_T("[FIL]++Read_Sector(%d, %d)\r\n"), nPpn, nSctOffset));

	gnPpn = nPpn;

	// Move pointer to Sector Offset
	nOffSet = nSctOffset * NAND_SECTOR_SIZE;

	// Random data output command
	NF_CMD(CMD_RANDOM_DATA_OUTPUT);
	NF_ADDR(nOffSet&0xFF);
	NF_ADDR((nOffSet>>8)&0xFF);
	NF_CMD(CMD_RANDOM_DATA_OUTPUT_CONFIRM);

	// Initialize 4-bit ECC Decoding
	NF_SET_ECC_DEC();
	NF_MECC_Reset();
	NF_MECC_UnLock();

	// Read 512 bytes Sector data
	if ((UINT32)pBuf&0x3)
	{
		_Read_512Byte_Unaligned(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}
	else
	{
		_Read_512Byte(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}

	NF_CLEAR_ECC_DEC_DONE();

	NF_CE_H(nBank);
	NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

	// Instead of Read Main ECC from NAND, Write Main ECC with CE don't care
	if (bCheckAllFF)
	{
		NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
		NF_DATA_W4(ECCVAL_ALLFF1);
	}
	else
	{
		nMECC[0] = pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_MECC_OFFSET:NAND_MECC_OFFSET_4K)/4)+nSctOffset*2];
		nMECC[1] = pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_MECC_OFFSET:NAND_MECC_OFFSET_4K)/4)+nSctOffset*2+1];
		NF_DATA_W4(nMECC[0]);  // pSpareCxt->aMECC[nSctOffset*2]
		NF_DATA_W4(nMECC[1]);  // pSpareCxt->aMECC[nSctOffset*2+1]
	}

	if ( bReadSafeMode == TRUE32 )
	{
    	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
	}
	else
	{
    	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
    }
	NF_CE_L(nBank);

	// Waiting for Main ECC compare
	bECCDecDone = NF_WAIT_ECC_DEC_DONE(pNANDFConReg);

    if ( bECCDecDone == TRUE32 )
    {
	    nRet = Decoding_MainECC(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}
	else
	{
	    nRet = ECC_UNCORRECTABLE_ERROR;
    	NAND_ERR((_T("Read_Sector: ECC Decoding Timeout Error\r\n")));
	}
#if (PROVE_ECC_ALGORITHM)
	if ( nRet&ECC_NEED_DECODING_AGAIN )
	{
    	// Initialize 4-bit ECC Decoding
    	NF_SET_ECC_DEC();
    	NF_MECC_Reset();
    	NF_MECC_UnLock();
    	NF_CE_H(nBank);

    	NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

    	if ((UINT32)pBuf&0x3)
    	{
    		_Write_512Byte_Unaligned(pBuf+NAND_SECTOR_SIZE*nSctOffset);
    	}
    	else
    	{
       		_Write_512Byte(pBuf+NAND_SECTOR_SIZE*nSctOffset);
    	}

    	NF_CLEAR_ECC_DEC_DONE();

    	if (bCheckAllFF)
    	{
    		NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
    		NF_DATA_W4(ECCVAL_ALLFF1);
    	}
    	else
    	{
    		NF_DATA_W4(nMECC[0]);
    		NF_DATA_W4(nMECC[1]);
    	}

    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
        	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }

    	NF_CE_L(nBank);
//    	NAND_ERR((_T("+NF_WAIT_ECC_DEC_DONE\r\n")));
    	bECCDecDone = NF_WAIT_ECC_DEC_DONE(pNANDFConReg);
//    	NAND_ERR((_T("-NF_WAIT_ECC_DEC_DONE\r\n")));

    	nRet &= ~ECC_NEED_DECODING_AGAIN;

        if ( bECCDecDone == TRUE32 )
        {
        	nRet |= Decoding_MainECC(pBuf+NAND_SECTOR_SIZE*nSctOffset);
        	if ( nRet&ECC_NEED_DECODING_AGAIN )
        	{
            	nRet &= ~ECC_NEED_DECODING_AGAIN;
        	    nRet |= ECC_UNCORRECTABLE_ERROR;
            	NAND_ERR((_T("[FIL:ERR] Read_Sector() : Multiple Decoding Error. Over 4bit Error!!\r\n")));
        	}
    	}
    	else
    	{
    	    NAND_ERR((_T("Read_Sector: ECC Decoding Timeout Error\r\n")));
    	    nRet |= ECC_UNCORRECTABLE_ERROR;
    	}
	}
#endif

	if (nRet&ECC_UNCORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL:ERR] Read_Sector() : ECC Uncorrectable Error in Page %d Sector %d\r\n"), nPpn, nSctOffset));

        // Print the current(latest) L,V,P number of block, page, sector.
        NAND_ERR((_T("[FIL:ERR] Lsn(0x%x) -> Lbn(0x%x) -> Vbn(0x%x) used in FTL.\r\n"), WMR_LLSN, WMR_LLBN, WMR_LVBN));
        NAND_ERR((_T("[FIL:ERR] Vpn(0x%x) -> Pbn(0x%x) -> Ppn(0x%x) used in VFL.\r\n"), WMR_LVPN, WMR_LPBN, WMR_LPPN));

        // Print the failed main data.
        MLC_Print_Page_Data(pBuf+(NAND_SECTOR_SIZE*nSctOffset), (UINT8 *)pSpareCxt);

        // Print the NFCON SFR.
        MLC_Print_SFR();
	}
	else if (nRet&ECC_CORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL] Read_Sector() : ECC Correctable Error in Page %d Sector %d\r\n"), nPpn, nSctOffset));
	}

	NAND_MSG((_T("[FIL]--Read_Sector()\r\n")));

	return nRet;
}


PRIVATE UINT32
Read_Sector_8BitECC(UINT32 nBank, UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf, UINT32* pSpareCxt, BOOL32 bCheckAllFF)
{
	UINT32 nOffSet;
	UINT32 nRet = 0;
	UINT32 nMECC[4];

	//_STOP_FOR_BREAK();

	NAND_MSG((_T("[FIL]++Read_Sector_8BitECC(%d, %d)\r\n"), nPpn, nSctOffset));
 	// Move pointer to Sector Offset
	nOffSet = nSctOffset * NAND_SECTOR_SIZE;

	NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
	pNANDFConReg->NFCONT |= NF_RNB_TRANS;


	// Random data output command
	NF_CMD(CMD_RANDOM_DATA_OUTPUT);
	NF_ADDR(nOffSet&0xFF);
	NF_ADDR((nOffSet>>8)&0xFF);
	NF_CMD(CMD_RANDOM_DATA_OUTPUT_CONFIRM);

	// Initialize 8-bit ECC Decoding
	NF_8BIT_ECC_STOP();
	NF_SET_ECC_DEC();
	NF_MECC_UnLock();
	NF_MECC_Reset();


	// Read 512 bytes Sector data
	if ((UINT32)pBuf&0x3)
	{
		_Read_512Byte_Unaligned(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}
	else
	{
		_Read_512Byte(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}

	NF_CLEAR_ECC_DEC_DONE();
	NF_CE_H(nBank);
   	NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

	// Instead of Read Main ECC from NAND, Write Main ECC with CE don't care
	if (bCheckAllFF)
	{
 		NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
		NF_DATA_W4(ECCVAL_ALLFF1);
		NF_DATA_W4(ECCVAL_ALLFF2);	// All 0xFF ECC
		NF_DATA_W(ECCVAL_ALLFF3&0xff);
	}
	else
	{
		nMECC[0] = pSpareCxt[(NAND_MECC_OFFSET_4K/4)+nSctOffset*4];
		nMECC[1] = pSpareCxt[(NAND_MECC_OFFSET_4K/4)+nSctOffset*4+1];
		nMECC[2] = pSpareCxt[(NAND_MECC_OFFSET_4K/4)+nSctOffset*4+2];
		nMECC[3] = pSpareCxt[(NAND_MECC_OFFSET_4K/4)+nSctOffset*4+3]&0xff;

		NF_DATA_W4(nMECC[0]);  // pSpareCxt->aMECC[nSctOffset*2]
		NF_DATA_W4(nMECC[1]);  // pSpareCxt->aMECC[nSctOffset*2+1]
		NF_DATA_W4(nMECC[2]);  // pSpareCxt->aMECC[nSctOffset*2]
		NF_DATA_W(nMECC[3]&0xff);  // pSpareCxt->aMECC[nSctOffset*2]
	}
	if ( bReadSafeMode == TRUE32 )
	{
    	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
	}
	else
	{
    	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
    }
	NF_CE_L(nBank);

	NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
	// Waiting for Main ECC compare
//	NF_WAIT_ECC_DEC_DONE();
	NF_CLEAR_ECC_DEC_DONE();
	NF_WAIT_8BITECC_DEC_DONE();

   	nRet = Decoding_Main8BitECC(pBuf+NAND_SECTOR_SIZE*nSctOffset);

#if (PROVE_ECC_ALGORITHM)
	if ( nRet&ECC_NEED_DECODING_AGAIN )
	{
    	// Initialize 8-bit ECC Decoding
    	NF_8BIT_ECC_STOP();
    	NF_SET_ECC_DEC();
    	NF_MECC_UnLock();
    	NF_MECC_Reset();

    	NF_CE_H(nBank);
    	NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

    	// Read 512 bytes Sector data
    	if ((UINT32)pBuf&0x3)
    	{
    		_Write_512Byte_Unaligned(pBuf+NAND_SECTOR_SIZE*nSctOffset);
    	}
    	else
    	{
    		_Write_512Byte(pBuf+NAND_SECTOR_SIZE*nSctOffset);
    	}
    	NF_CLEAR_ECC_DEC_DONE();

    	// Instead of Read Main ECC from NAND, Write Main ECC with CE don't care
    	if (bCheckAllFF)
    	{
     		NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
    		NF_DATA_W4(ECCVAL_ALLFF1);
    		NF_DATA_W4(ECCVAL_ALLFF2);	// All 0xFF ECC
    		NF_DATA_W(ECCVAL_ALLFF3&0xff);
    	}
    	else
    	{
    		NF_DATA_W4(nMECC[0]);  // pSpareCxt->aMECC[nSctOffset*2]
    		NF_DATA_W4(nMECC[1]);  // pSpareCxt->aMECC[nSctOffset*2+1]
    		NF_DATA_W4(nMECC[2]);  // pSpareCxt->aMECC[nSctOffset*2]
    		NF_DATA_W(nMECC[3]&0xff);  // pSpareCxt->aMECC[nSctOffset*2]
    	}
    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
        	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
    	NF_CE_L(nBank);

    	NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
    	// Waiting for Main ECC compare
//    	NF_WAIT_ECC_DEC_DONE();
    	NF_CLEAR_ECC_DEC_DONE();
    	NF_WAIT_8BITECC_DEC_DONE();

        nRet &= ~ECC_NEED_DECODING_AGAIN;

    	nRet |= Decoding_Main8BitECC(pBuf+NAND_SECTOR_SIZE*nSctOffset);
    	if ( nRet&ECC_NEED_DECODING_AGAIN )
    	{
        	nRet &= ~ECC_NEED_DECODING_AGAIN;
    	    nRet |= ECC_UNCORRECTABLE_ERROR;
        	NAND_ERR((_T("[FIL:ERR] Read_Sector_8BitECC() : Multiple Decoding Error. Over 8bit Error!!\r\n")));
    	}
	}
#endif

	if (nRet&ECC_UNCORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL:ERR] Read_Sector_8BitECC() : ECC Uncorrectable Error in Page %d Sector %d\r\n"), nPpn, nSctOffset));

        // Print the current(latest) L,V,P number of block, page, sector.
        NAND_ERR((_T("[FIL:ERR] Lsn(0x%x) -> Lbn(0x%x) -> Vbn(0x%x) used in FTL.\r\n"), WMR_LLSN, WMR_LLBN, WMR_LVBN));
        NAND_ERR((_T("[FIL:ERR] Vpn(0x%x) -> Pbn(0x%x) -> Ppn(0x%x) used in VFL.\r\n"), WMR_LVPN, WMR_LPBN, WMR_LPPN));

        // Print the failed main data.
        MLC_Print_Page_Data(pBuf+(NAND_SECTOR_SIZE*nSctOffset), (UINT8 *)pSpareCxt);

        // Print the NFCON SFR.
        MLC_Print_SFR();
 	}
	else if (nRet&ECC_CORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL] Read_Sector_8BitECC() : ECC Correctable Error in Page %d Sector %d\r\n"), nPpn, nSctOffset));
	}

	NAND_MSG((_T("[FIL]--Read_Sector_8BitECC()\r\n")));

	return nRet;
}


PRIVATE UINT32
Read_Spare(UINT32 nBank, UINT32 nPpn, UINT32* pSpareCxt)
{
	UINT32 nOffset;
	UINT32 nRet = 0;
	BOOL32 bCheckMore = FALSE32;
	BOOL32 bCheckAllFF = FALSE32;
	UINT32 nCnt;
	UINT32 nPosPtr;
	UINT8  cCleanMark;
	UINT8* pSBuf;
	BOOL32 bECCDecDone = TRUE;

	NAND_MSG((_T("[FIL]++Read_Spare(%d)\r\n"), nPpn));
	//NAND_ERR((_T("[FIL]++Read_Spare(%d)\r\n"), nPpn));  // ksk dbg

	// Read Spare Area
	pSpareCxt[0] = NF_DATA_R4();		// 1 byte Bad Mark(->cBadMark) + 1 byte Clean Mark(->cCleanMark) + 2 byte Reserved(->cReserved)

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 4-bit ECC Decoding
		NF_SET_ECC_DEC();
		NF_MECC_Reset();
		NF_MECC_UnLock();
		cCleanMark = (UINT8)((pSpareCxt[0]&0xff00)>>8);		// ->cCleanMark
		if (WMR_GetChkSum(&cCleanMark, 1) < 4)
		{
			bCheckAllFF = TRUE32;
		}
	}

	nPosPtr = NAND_SCXT_OFFSET/4;

	if (SECTORS_PER_PAGE == 4)
	{
        for (nCnt = 0; nCnt < ((NAND_SECC_OFFSET-NAND_SCXT_OFFSET)/4); nCnt++)
			pSpareCxt[nPosPtr++] = NF_DATA_R4();		// 12 byte Spare Context(->aSpareData) + 32 byte Sector ECC data(->aMECC)
	}
	else if (SECTORS_PER_PAGE == 8)
	{
		for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_4K-NAND_SCXT_OFFSET)/4; nCnt++)
			pSpareCxt[nPosPtr++] = NF_DATA_R4();		// 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)
	}
	else
	{
		WMR_ASSERT(FALSE32);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		NF_CE_H(nBank);
   		NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

		if (SECTORS_PER_PAGE == 4)
		{
			_Write_Dummy_468Byte_AllFF();
		}
		else if (SECTORS_PER_PAGE == 8)
		{
			_Write_Dummy_428Byte_AllFF();
		}

    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
    		NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
		NF_CE_L(nBank);

		//NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding

		// Read Spare ECC
		NF_CLEAR_ECC_DEC_DONE();

		if (bCheckAllFF)
		{
			NF_CE_H(nBank);
   			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
			NF_DATA_W4(ECCVAL_ALLFF1);

        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
    			NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }
			NF_CE_L(nBank);
		}
		else
		{
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Read 8 byte Spare ECC data,  ->ASECC[0]
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[1]
		}

		// Wait Spare ECC Compare Done
		bECCDecDone = NF_WAIT_ECC_DEC_DONE(pNANDFConReg);

		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);
        if ( bECCDecDone == TRUE32 )
        {
    		nRet = Decoding_SpareECC(pSBuf);
    	}
    	else
    	{
    	    NAND_ERR((_T("Read_Spare1: ECC Decoding Timeout Error\r\n")));
    	    nRet = ECC_UNCORRECTABLE_ERROR;
    	}
#if (PROVE_ECC_ALGORITHM)
    	if ( nRet&ECC_NEED_DECODING_AGAIN )
    	{
    		// Initialize 4-bit ECC Decoding
    		NF_SET_ECC_DEC();
    		NF_MECC_Reset();
    		NF_MECC_UnLock();

    		NF_CE_H(nBank);

       		NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

        	nPosPtr = NAND_SCXT_OFFSET/4;

        	if (SECTORS_PER_PAGE == 4)
        	{
        		for (nCnt = 0; nCnt < (NAND_SECC_OFFSET-NAND_SCXT_OFFSET)/4; nCnt++)
        		    NF_DATA_W4(pSpareCxt[nPosPtr++]); // 12 byte Spare Context(->aSpareData) + 32 byte Sector ECC data(->aMECC)
        	}
        	else if (SECTORS_PER_PAGE == 8)
        	{
        		for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_4K-NAND_SCXT_OFFSET)/4; nCnt++)
        		    NF_DATA_W4(pSpareCxt[nPosPtr++]); // 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)
        	}
        	else
        	{
        		WMR_ASSERT(FALSE32);
        	}

    		if (SECTORS_PER_PAGE == 4)
    		{
    			_Write_Dummy_468Byte_AllFF();
    		}
    		else if (SECTORS_PER_PAGE == 8)
    		{
    			_Write_Dummy_428Byte_AllFF();
    		}

    		// Read Spare ECC
    		NF_CLEAR_ECC_DEC_DONE();

    		if (bCheckAllFF)
    		{
    			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
    			NF_DATA_W4(ECCVAL_ALLFF1);
    		}
    		else
    		{
    			NF_DATA_W4(pSpareCxt[nPosPtr++]);	// Read 8 byte Spare ECC data,  ->ASECC[0]
    			NF_DATA_W4(pSpareCxt[nPosPtr++]);	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[1]
    		}

        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
            	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }

   			NF_CE_L(nBank);

    		// Wait Spare ECC Compare Done
    		bECCDecDone = NF_WAIT_ECC_DEC_DONE(pNANDFConReg);

    		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);

        	nRet &= ~ECC_NEED_DECODING_AGAIN;

            if ( bECCDecDone == TRUE32 )
            {
            	nRet |= Decoding_SpareECC(pSBuf);
            	if ( nRet&ECC_NEED_DECODING_AGAIN )
            	{
                   	nRet &= ~ECC_NEED_DECODING_AGAIN;
            	    nRet |= ECC_UNCORRECTABLE_ERROR;
                	NAND_ERR((_T("[FIL:ERR] Read_Spare() : Multiple Decoding Error. Over 4bit Error!!\r\n")));
        	    }
        	}
        	else
        	{
    	        NAND_ERR((_T("Read_Spare2: ECC Decoding Timeout Error\r\n")));
            	nRet |= ECC_UNCORRECTABLE_ERROR;
    	    }
    	}
#endif

		if (bCheckAllFF)
		{
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Read 8 byte Spare ECC data  ->ASECC[0]
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[1]
		}

		pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Read 2nd Spare ECC  ->ASECC[2]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[3]

_B_CheckMore:
		// 2nd Try
		if (nRet == ECC_UNCORRECTABLE_ERROR)
		{

			NAND_ERR((_T("[FIL:ERR] Read_Spare() : ECC Uncorrectable Error in Spare of Page %d 1st Time : 0x%x\r\n"), nPpn, nRet));
			if (bCheckMore == TRUE32) NAND_ERR((_T("[FIL:ERR] Read_Spare() : Try ECC Decoding again with 2nd SECC copy\r\n")));
			else                      NAND_ERR((_T("[FIL:ERR] Read_Spare() : Try ECC Decoding again with SECC bit change\r\n")));

			nOffset = BYTES_PER_MAIN_PAGE+NAND_SCXT_OFFSET;		// Position to SpareData

			NF_CMD(CMD_RANDOM_DATA_OUTPUT);
			NF_ADDR(nOffset&0xFF);
			NF_ADDR((nOffset>>8)&0xFF);
			NF_CMD(CMD_RANDOM_DATA_OUTPUT_CONFIRM);

			// Initialize 4-bit ECC Decoding
			NF_SET_ECC_DEC();
			NF_MECC_Reset();
			NF_MECC_UnLock();

			nPosPtr = NAND_SCXT_OFFSET/4;

			if (SECTORS_PER_PAGE == 4)
			{
				for (nCnt = 0; nCnt < (NAND_SECC_OFFSET-NAND_SCXT_OFFSET)/4; nCnt++)
					pSpareCxt[nPosPtr++] = NF_DATA_R4();		// 12 byte Spare Context(->aSpareData) + 32 byte Sector ECC data(->aMECC)
			}
			if (SECTORS_PER_PAGE == 8)
			{
				for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_4K-NAND_SCXT_OFFSET)/4; nCnt++)
					pSpareCxt[nPosPtr++] = NF_DATA_R4();		// 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)
			}

			NF_CE_H(nBank);
   			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

			if (SECTORS_PER_PAGE == 4)
			{
				_Write_Dummy_468Byte_AllFF();
			}
			else if (SECTORS_PER_PAGE == 8)
			{
				_Write_Dummy_428Byte_AllFF();
			}

			//NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding

			NF_CLEAR_ECC_DEC_DONE();

			if (bCheckMore == TRUE32)
			{
				NF_DATA_W4(pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC2_OFFSET:NAND_SECC2_OFFSET_4K)/4)]);		// Write 2nd Spare ECC,  ->aSECC[2]
				NF_DATA_W4(pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC2_OFFSET:NAND_SECC2_OFFSET_4K)/4)+1]);		// Actually after read 7th byte, ECC decoding starts!!  ->aSECC[3]
				bCheckMore = FALSE32;
			}
			else
			{
				NF_DATA_W4(pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC_OFFSET:NAND_SECC_OFFSET_4K)/4)]);		// Write modified ECC for the 53th bit on SECC data  ->aSECC[0]
				NF_DATA_W4((pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC_OFFSET:NAND_SECC_OFFSET_4K)/4)+1])^(1<<19));		// position to be modified on SECC[1]  ->aSECC[1]
				bCheckMore = TRUE32;
			}

        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
    			NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }
			NF_CE_L(nBank);

			// Wait Spare ECC Compare Done
			bECCDecDone = NF_WAIT_ECC_DEC_DONE(pNANDFConReg);

    		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);
            if ( bECCDecDone == TRUE32 )
            {
            	nRet = Decoding_SpareECC(pSBuf);
            }
            else
            {
        	    NAND_ERR((_T("Read_Spare3: ECC Decoding Timeout Error\r\n")));
    	        nRet = ECC_UNCORRECTABLE_ERROR;
            }
#if (PROVE_ECC_ALGORITHM)
        	if ( nRet&ECC_NEED_DECODING_AGAIN )
        	{
    			// Initialize 4-bit ECC Decoding
    			NF_SET_ECC_DEC();
    			NF_MECC_Reset();
    			NF_MECC_UnLock();

    			NF_CE_H(nBank);

       			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

    			nPosPtr = NAND_SCXT_OFFSET/4;

    			if (SECTORS_PER_PAGE == 4)
    			{
    				for (nCnt = 0; nCnt < (NAND_SECC_OFFSET-NAND_SCXT_OFFSET)/4; nCnt++)
    					NF_DATA_W4(pSpareCxt[nPosPtr++]);		// 12 byte Spare Context(->aSpareData) + 32 byte Sector ECC data(->aMECC)
    			}
    			if (SECTORS_PER_PAGE == 8)
    			{
    				for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_4K-NAND_SCXT_OFFSET)/4; nCnt++)
    					NF_DATA_W4(pSpareCxt[nPosPtr++]);		// 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)
    			}

    			if (SECTORS_PER_PAGE == 4)
    			{
    				_Write_Dummy_468Byte_AllFF();
    			}
    			else if (SECTORS_PER_PAGE == 8)
    			{
    				_Write_Dummy_428Byte_AllFF();
    			}

    			NF_CLEAR_ECC_DEC_DONE();

    			if (bCheckMore == FALSE32)  // Change Check More... in PROVE_ECC_ALGORITHM code.
    			{
    				NF_DATA_W4(pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC2_OFFSET:NAND_SECC2_OFFSET_4K)/4)]);		// Write 2nd Spare ECC,  ->aSECC[2]
    				NF_DATA_W4(pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC2_OFFSET:NAND_SECC2_OFFSET_4K)/4)+1]);		// Actually after read 7th byte, ECC decoding starts!!  ->aSECC[3]
    			}
    			else
    			{
    				NF_DATA_W4(pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC_OFFSET:NAND_SECC_OFFSET_4K)/4)]);		// Write modified ECC for the 53th bit on SECC data  ->aSECC[0]
    				NF_DATA_W4((pSpareCxt[(((SECTORS_PER_PAGE==4)?NAND_SECC_OFFSET:NAND_SECC_OFFSET_4K)/4)+1])^(1<<19));		// position to be modified on SECC[1]  ->aSECC[1]
    			}

            	if ( bReadSafeMode == TRUE32 )
            	{
                	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
            	}
            	else
            	{
                	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
                }
    			NF_CE_L(nBank);

    			// Wait Spare ECC Compare Done
    			bECCDecDone = NF_WAIT_ECC_DEC_DONE(pNANDFConReg);

        		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);

     	        nRet &= ~ECC_NEED_DECODING_AGAIN;

                if ( bECCDecDone == TRUE32 )
                {
               	    nRet |= Decoding_SpareECC(pSBuf);
                	if ( nRet&ECC_NEED_DECODING_AGAIN )
                	{
                       	nRet &= ~ECC_NEED_DECODING_AGAIN;
                	    nRet |= ECC_UNCORRECTABLE_ERROR;
                    	NAND_ERR((_T("[FIL:ERR] Read_Spare() : Multiple Decoding Error. Over 4bit Error!!\r\n")));
                	}
                }
                else
                {
    	            NAND_ERR((_T("Read_Spare4: ECC Decoding Timeout Error\r\n")));
    	            nRet |= ECC_UNCORRECTABLE_ERROR;
            	}
        	}
#endif

			if (bCheckMore == TRUE32)
				goto _B_CheckMore;
		}
	}
	else
	{

		// just read Spare ECC from NAND for read pointer, NOT decoding ECC
		pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 8 byte Spare ECC data,  ->aSECC[0]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 8 byte Spare ECC data 2nd copy  ->aSECC[2]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
	}

	if (nRet&ECC_UNCORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL:ERR] Read_Spare() : ECC Uncorrectable Error in Spare of Page %d\r\n"), nPpn));

        // Print the current(latest) L,V,P number of block, page, sector.
        NAND_ERR((_T("[FIL:ERR] Lsn(0x%x) -> Lbn(0x%x) -> Vbn(0x%x) used in FTL.\r\n"), WMR_LLSN, WMR_LLBN, WMR_LVBN));
        NAND_ERR((_T("[FIL:ERR] Vpn(0x%x) -> Pbn(0x%x) -> Ppn(0x%x) used in VFL.\r\n"), WMR_LVPN, WMR_LPBN, WMR_LPPN));

        // Print the failed spare data.
        MLC_Print_Page_Data(NULL, (UINT8 *)pSpareCxt);

        // Print the NFCON SFR.
        MLC_Print_SFR();
	}
	else if (nRet&ECC_CORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL] Read_Spare() : ECC Correctable Error in Spare of Page %d\r\n"), nPpn));
	}

	NAND_MSG((_T("[FIL]--Read_Spare()\r\n")));

	return nRet;
}

PRIVATE UINT32
Read_Spare_Separate(UINT32 nBank, UINT32 nPpn, UINT32* pSpareCxt)
{
	UINT32 nOffset;
	UINT32 nRet = 0;
	BOOL32 bCheckAllFF = FALSE32;
	UINT32 nCnt;
	UINT32 nPosPtr;
	UINT8  cCleanMark;
	UINT8* pSBuf;
	UINT32 nEffectiveByte;
	BOOL32 bECCDecDone = TRUE;

	NAND_MSG((_T("[FIL]++Read_Spare_Separate(%d)\r\n"), nPpn));

    // Read ECC parity Code for spare area
    nPosPtr = NAND_SECC_OFFSET_4K/4;
	pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 16 byte Spare Context ECC data
	pSpareCxt[nPosPtr++] = NF_DATA_R4();
	pSpareCxt[nPosPtr++] = NF_DATA_R4();
	pSpareCxt[nPosPtr++] = NF_DATA_R4();
	pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 16 byte ECC data of MECC
	pSpareCxt[nPosPtr++] = NF_DATA_R4();
	pSpareCxt[nPosPtr++] = NF_DATA_R4();
	pSpareCxt[nPosPtr++] = NF_DATA_R4();

	nOffset = BYTES_PER_MAIN_PAGE;		            // Position to SpareData first byte.

	NF_CMD(CMD_RANDOM_DATA_OUTPUT);
	NF_ADDR(nOffset&0xFF);
	NF_ADDR((nOffset>>8)&0xFF);
	NF_CMD(CMD_RANDOM_DATA_OUTPUT_CONFIRM);

	// Read Spare Area
	pSpareCxt[0] = NF_DATA_R4();		// 1 byte Bad Mark(->cBadMark) + 1 byte Clean Mark(->cCleanMark) + 2 byte Reserved(->cReserved)

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 8-bit ECC Decoding
		NF_8BIT_ECC_STOP();
		NF_SET_ECC_DEC();
		NF_MECC_UnLock();
		NF_MECC_Reset();

		cCleanMark = (UINT8)((pSpareCxt[0]&0xff00)>>8);		// ->cCleanMark
		if (WMR_GetChkSum(&cCleanMark, 1) < 4)
		{
			bCheckAllFF = TRUE32;
		}
	}

	nPosPtr = NAND_SCXT_OFFSET/4;       // position to spare cxt.

    if (SECTORS_PER_PAGE == 8)
	{
		for (nCnt = 0; nCnt < (NAND_MECC_OFFSET_4K-NAND_SCXT_OFFSET)/4; nCnt++)
			pSpareCxt[nPosPtr++] = NF_DATA_R4();		// 20 byte Spare Context(->aSpareData)
	}
	else
	{
		WMR_ASSERT(FALSE32);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		NF_CLEAR_ECC_DEC_DONE();

		NF_CE_H(nBank);
   		NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

        if (SECTORS_PER_PAGE == 8)
		{
			_Write_Dummy_492Byte_AllFF();
		}

		if (bCheckAllFF)
		{
			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
			NF_DATA_W4(ECCVAL_ALLFF1);
			NF_DATA_W4(ECCVAL_ALLFF2);
			NF_DATA_W4(ECCVAL_ALLFF3);
		}
		else
		{
            nPosPtr = NAND_SECC_OFFSET_4K/4;
            NF_DATA_W4(pSpareCxt[nPosPtr++]);
            NF_DATA_W4(pSpareCxt[nPosPtr++]);
            NF_DATA_W4(pSpareCxt[nPosPtr++]);
            NF_DATA_W(pSpareCxt[nPosPtr++]&0xff);
		}

    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
			NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
		NF_CE_L(nBank);

		NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
		// Wait Spare ECC Compare Done
//		NF_WAIT_ECC_DEC_DONE();
		NF_CLEAR_ECC_DEC_DONE();
		NF_WAIT_8BITECC_DEC_DONE();

		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);
		nEffectiveByte = 20;
		nRet = Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
#if (PROVE_ECC_ALGORITHM)
    	if ( nRet&ECC_NEED_DECODING_AGAIN )
    	{
    		// Initialize 8-bit ECC Decoding
    		NF_8BIT_ECC_STOP();
    		NF_SET_ECC_DEC();
    		NF_MECC_UnLock();
    		NF_MECC_Reset();

    		NF_CE_H(nBank);
       		NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

        	nPosPtr = NAND_SCXT_OFFSET/4;       // position to spare cxt.

    		for (nCnt = 0; nCnt < (NAND_MECC_OFFSET_4K-NAND_SCXT_OFFSET)/4; nCnt++)
    			NF_DATA_W4(pSpareCxt[nPosPtr++]);		// 20 byte Spare Context(->aSpareData)

    		NF_CLEAR_ECC_DEC_DONE();

            if (SECTORS_PER_PAGE == 8)
    		{
    			_Write_Dummy_492Byte_AllFF();
    		}

    		if (bCheckAllFF)
    		{
    			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
    			NF_DATA_W4(ECCVAL_ALLFF1);
    			NF_DATA_W4(ECCVAL_ALLFF2);
    			NF_DATA_W4(ECCVAL_ALLFF3);
    		}
    		else
    		{
                nPosPtr = NAND_SECC_OFFSET_4K/4;
                NF_DATA_W4(pSpareCxt[nPosPtr++]);
                NF_DATA_W4(pSpareCxt[nPosPtr++]);
                NF_DATA_W4(pSpareCxt[nPosPtr++]);
                NF_DATA_W(pSpareCxt[nPosPtr++]&0xff);
    		}

        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
            	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }
    		NF_CE_L(nBank);

    		NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
    		// Wait Spare ECC Compare Done
//    		NF_WAIT_ECC_DEC_DONE();
    		NF_CLEAR_ECC_DEC_DONE();
    		NF_WAIT_8BITECC_DEC_DONE();

    		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);

    	    nRet &= ~ECC_NEED_DECODING_AGAIN;
        	nRet |= Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
        	if ( nRet&ECC_NEED_DECODING_AGAIN )
        	{
               	nRet &= ~ECC_NEED_DECODING_AGAIN;
        	    nRet |= ECC_UNCORRECTABLE_ERROR;
               	NAND_ERR((_T("[FIL:ERR] Read_Spare_Separate() : Multiple Decoding Error. Over 8bit Error in Meta area!!\r\n")));
        	}
    	}
#endif

    	NAND_MSG((_T("[FIL]Read_Spare_Separate(nRet:%d)\r\n"), nRet));

		if (nRet != ECC_UNCORRECTABLE_ERROR)
		{
    		// Initialize 8-bit ECC Decoding
    		NF_8BIT_ECC_STOP();
    		NF_SET_ECC_DEC();
    		NF_MECC_UnLock();
    		NF_MECC_Reset();

			nPosPtr = NAND_MECC_OFFSET_4K/4;

			if (SECTORS_PER_PAGE == 8)
			{
        		// read Main ECC parity code
        		for (nCnt = 0; nCnt < SECTORS_PER_PAGE; nCnt++) // 8 MECC Parity Code read
        		{
            		pSpareCxt[nPosPtr++] = NF_DATA_R4();
            		pSpareCxt[nPosPtr++] = NF_DATA_R4();
            	}
			}

			NF_CE_H(nBank);
   			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

			if (SECTORS_PER_PAGE == 8)
			{
				_Write_Dummy_448Byte_AllFF();
			}

			NF_CLEAR_ECC_DEC_DONE();

    		if (bCheckAllFF)
    		{
    			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
    			NF_DATA_W4(ECCVAL_ALLFF1);
    			NF_DATA_W4(ECCVAL_ALLFF2);
    			NF_DATA_W4(ECCVAL_ALLFF3);
    		}
    		else
    		{
                nPosPtr = (NAND_SECC_OFFSET_4K+16)/4;
                NF_DATA_W4(pSpareCxt[nPosPtr++]);
                NF_DATA_W4(pSpareCxt[nPosPtr++]);
                NF_DATA_W4(pSpareCxt[nPosPtr++]);
                NF_DATA_W(pSpareCxt[nPosPtr++]&0xff);
    		}

        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
    			NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }
			NF_CE_L(nBank);

    		NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
    		// Wait Spare ECC Compare Done
//    		NF_WAIT_ECC_DEC_DONE();
    		NF_CLEAR_ECC_DEC_DONE();
    		NF_WAIT_8BITECC_DEC_DONE();

    		pSBuf =(UINT8*) &(pSpareCxt[NAND_MECC_OFFSET_4K/4]);

            nEffectiveByte = 64;
    		nRet = Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
#if (PROVE_ECC_ALGORITHM)
        	if ( nRet&ECC_NEED_DECODING_AGAIN )
        	{
        		// Initialize 8-bit ECC Decoding
        		NF_8BIT_ECC_STOP();
        		NF_SET_ECC_DEC();
        		NF_MECC_UnLock();
        		NF_MECC_Reset();

    			NF_CE_H(nBank);

       			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

    			nPosPtr = NAND_MECC_OFFSET_4K/4;

        		// read Main ECC parity code
        		for (nCnt = 0; nCnt < SECTORS_PER_PAGE; nCnt++) // 8 MECC Parity Code read
        		{
            		NF_DATA_W4(pSpareCxt[nPosPtr++]);
            		NF_DATA_W4(pSpareCxt[nPosPtr++]);
            	}

    			if (SECTORS_PER_PAGE == 8)
    			{
    				_Write_Dummy_448Byte_AllFF();
    			}

    			NF_CLEAR_ECC_DEC_DONE();

        		if (bCheckAllFF)
        		{
        			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
        			NF_DATA_W4(ECCVAL_ALLFF1);
        			NF_DATA_W4(ECCVAL_ALLFF2);
        			NF_DATA_W4(ECCVAL_ALLFF3);
        		}
        		else
        		{
                    nPosPtr = (NAND_SECC_OFFSET_4K+16)/4;
                    NF_DATA_W4(pSpareCxt[nPosPtr++]);
                    NF_DATA_W4(pSpareCxt[nPosPtr++]);
                    NF_DATA_W4(pSpareCxt[nPosPtr++]);
                    NF_DATA_W(pSpareCxt[nPosPtr++]&0xff);
        		}

            	if ( bReadSafeMode == TRUE32 )
            	{
                	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
            	}
            	else
            	{
                	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
                }
    			NF_CE_L(nBank);

        		NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
        		// Wait Spare ECC Compare Done
//        		NF_WAIT_ECC_DEC_DONE();
        		NF_CLEAR_ECC_DEC_DONE();
        		NF_WAIT_8BITECC_DEC_DONE();

        		pSBuf =(UINT8*) &(pSpareCxt[NAND_MECC_OFFSET_4K/4]);

    	        nRet &= ~ECC_NEED_DECODING_AGAIN;
            	nRet |= Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
            	if ( nRet&ECC_NEED_DECODING_AGAIN )
            	{
                   	nRet &= ~ECC_NEED_DECODING_AGAIN;
            	    nRet |= ECC_UNCORRECTABLE_ERROR;
                	NAND_ERR((_T("[FIL:ERR] Read_Spare_Separate() : Multiple Decoding Error. Over 8bit Error in MECC area!!\r\n")));
            	}
        	}
#endif
        	NAND_MSG((_T("[FIL]Read_Spare_Separate2(nRet:%d)\r\n"), nRet));
		}
		else
		{
			if (SECTORS_PER_PAGE == 8)
			{
        		// read Main ECC parity code
        		for (nCnt = 0; nCnt < SECTORS_PER_PAGE; nCnt++) // 8 MECC Parity Code read
        		{
            		pSpareCxt[nPosPtr++] = NF_DATA_R4();
            		pSpareCxt[nPosPtr++] = NF_DATA_R4();
            	}
			}
		}
	}
	else
	{
		// just read Main ECC from NAND for read pointer, NOT decoding ECC
		for (nCnt = 0; nCnt < SECTORS_PER_PAGE; nCnt++) // 8 MECC Parity Code read
		{
    		pSpareCxt[nPosPtr++] = NF_DATA_R4();
    		pSpareCxt[nPosPtr++] = NF_DATA_R4();
    	}

		// just read Spare ECC from NAND for read pointer, NOT decoding ECC
		pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 16 byte Spare ECC data,  ->aSECC[0]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 16 byte Spare ECC data,  ->aSECC[1]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
	}

	if (nRet&ECC_UNCORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL:ERR] Read_Spare_Separate() : ECC Uncorrectable Error in Spare of Page %d\r\n"), nPpn));

        // Print the current(latest) L,V,P number of block, page, sector.
        NAND_ERR((_T("[FIL:ERR] Lsn(0x%x) -> Lbn(0x%x) -> Vbn(0x%x) used in FTL.\r\n"), WMR_LLSN, WMR_LLBN, WMR_LVBN));
        NAND_ERR((_T("[FIL:ERR] Vpn(0x%x) -> Pbn(0x%x) -> Ppn(0x%x) used in VFL.\r\n"), WMR_LVPN, WMR_LPBN, WMR_LPPN));

        // Print the failed spare data.
        MLC_Print_Page_Data(NULL, (UINT8 *)pSpareCxt);

        // Print the NFCON SFR.
        MLC_Print_SFR();
	}
	else if (nRet&ECC_CORRECTABLE_ERROR)
	{
		NAND_MSG((_T("[FIL] Read_Spare_Separate() : ECC Correctable Error in Spare of Page %d\r\n"), nPpn));
	}

	NAND_MSG((_T("[FIL]--Read_Spare_Separate()\r\n")));

	return nRet;
}

PRIVATE void
MLC_Print_Page_Data(unsigned char *pMBuf, unsigned char *pSBuf)
{
    unsigned int i = 0, j = 0;

    if (pMBuf != NULL)
    {
        RETAILMSG(TRUE, (TEXT("Main Data\r\n")));
        RETAILMSG(TRUE, (TEXT("=================================================\r\n")));
        for (i = 0; i < BYTES_PER_SECTOR; i++)
        {
            RETAILMSG(TRUE, (TEXT("%02x "), pMBuf[i]));
            if (i%BYTES_PER_SPARE == 15) RETAILMSG(TRUE, (TEXT("\r\n")));
        }
        RETAILMSG(TRUE, (TEXT("=================================================\r\n")));
    }

    if (pSBuf != NULL)
    {
        RETAILMSG(TRUE, (TEXT("Spare Data\r\n")));
        RETAILMSG(TRUE, (TEXT("=================================================\r\n")));
        for (i = 0; i < BYTES_PER_SPARE_PAGE; i++)
        {
            RETAILMSG(TRUE, (TEXT("%02x "), pSBuf[i]));
            if ((i%BYTES_PER_SPARE == 15) || (i == (BYTES_PER_SPARE_PAGE-1))) RETAILMSG(TRUE, (TEXT("\r\n")));
        }
        RETAILMSG(TRUE, (TEXT("\r\n=================================================\r\n")));
    }
}

PRIVATE UINT32
Read_Spare_8BitECC(UINT32 nBank, UINT32 nPpn, UINT32* pSpareCxt)
{
	UINT32 nOffset;
	UINT32 nRet = 0;
	BOOL32 bCheckMore = FALSE32;
	BOOL32 bCheckAllFF = FALSE32;
	UINT32 nCnt;
	UINT32 nPosPtr;
	UINT8  cCleanMark;
	UINT8* pSBuf;
    UINT32 nEffectiveByte;
    UINT8  cReadAndTrash;

	NAND_MSG((_T("[FIL]++Read_Spare_8BitECC(%d)\r\n"), nPpn));

	// Read Spare Area
	pSpareCxt[0] = NF_DATA_R4();		// 1 byte Bad Mark(->cBadMark) + 1 byte Clean Mark(->cCleanMark) + 2 byte Reserved(->cReserved)

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 8-bit ECC Decoding
		//NF_8BIT_ECC_STOP_CLEAR();
		NF_8BIT_ECC_STOP();
		NF_SET_ECC_DEC();
		NF_MECC_UnLock();
		NF_MECC_Reset();
		cCleanMark = (UINT8)((pSpareCxt[0]&0xff00)>>8);		// ->cCleanMark
		if (WMR_GetChkSum(&cCleanMark, 1) < 4)
		{
 			bCheckAllFF = TRUE32;
		}
 	}

	nPosPtr = NAND_SCXT_OFFSET/4;

	if (SECTORS_PER_PAGE == 8)
	{
		for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_8BIT_ECC_4K-NAND_SCXT_OFFSET)/4; nCnt++)
			pSpareCxt[nPosPtr++] = NF_DATA_R4();		// 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)
	}
	else
	{
		WMR_ASSERT(FALSE32);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		NF_CLEAR_ECC_DEC_DONE();

		NF_CE_H(nBank);
		NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

		if (SECTORS_PER_PAGE == 8)
		{
			_Write_Dummy_364Byte_AllFF();
		}

    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
			NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
		NF_CE_L(nBank);

		// Read Spare ECC
		if (bCheckAllFF)
		{
			NF_CE_H(nBank);
   			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
			NF_DATA_W4(ECCVAL_ALLFF1);
			NF_DATA_W4(ECCVAL_ALLFF2);	// All 0xFF ECC
			NF_DATA_W(ECCVAL_ALLFF3&0xff);


        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
    			NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }
			NF_CE_L(nBank);
		}
		else
		{
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Read 8 byte Spare ECC data,  ->ASECC[0]
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[1]
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[2]
			pSpareCxt[nPosPtr++] = (UINT32)(NF_DATA_R()&0xff);	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[3]
		}

		NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
		// Wait Spare ECC Compare Done
//		NF_WAIT_ECC_DEC_DONE();
		NF_CLEAR_ECC_DEC_DONE();
		NF_WAIT_8BITECC_DEC_DONE();

		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);
		nEffectiveByte = 20 + 16*8;
		nRet = Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
#if (PROVE_ECC_ALGORITHM)
    	if ( nRet&ECC_NEED_DECODING_AGAIN )
    	{
    		// Initialize 8-bit ECC Decoding
    		//NF_8BIT_ECC_STOP_CLEAR();
    		NF_8BIT_ECC_STOP();
    		NF_SET_ECC_DEC();
    		NF_MECC_UnLock();
    		NF_MECC_Reset();

    		NF_CE_H(nBank);
    		NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

        	nPosPtr = NAND_SCXT_OFFSET/4;

    		for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_8BIT_ECC_4K-NAND_SCXT_OFFSET)/4; nCnt++)
    			NF_DATA_W4(pSpareCxt[nPosPtr++]);		// 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)

    		NF_CLEAR_ECC_DEC_DONE();

    		if (SECTORS_PER_PAGE == 8)
    		{
    			_Write_Dummy_364Byte_AllFF();
    		}

    		// Read Spare ECC
    		if (bCheckAllFF)
    		{
    			NF_DATA_W4(ECCVAL_ALLFF0);	// All 0xFF ECC
    			NF_DATA_W4(ECCVAL_ALLFF1);
    			NF_DATA_W4(ECCVAL_ALLFF2);	// All 0xFF ECC
    			NF_DATA_W(ECCVAL_ALLFF3&0xff);
    		}
    		else
    		{
    			NF_DATA_W4(pSpareCxt[nPosPtr++]);	// Read 8 byte Spare ECC data,  ->ASECC[0]
    			NF_DATA_W4(pSpareCxt[nPosPtr++]);	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[1]
    			NF_DATA_W4(pSpareCxt[nPosPtr++]);	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[2]
    			NF_DATA_W(pSpareCxt[nPosPtr++]&0xff);	// Actually after read 7th byte, ECC decoding starts!!,  ->ASECC[3]
    		}

        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
            	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }
    		NF_CE_L(nBank);

    		NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
    		// Wait Spare ECC Compare Done
//    		NF_WAIT_ECC_DEC_DONE();
    		NF_CLEAR_ECC_DEC_DONE();
    		NF_WAIT_8BITECC_DEC_DONE();

    		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);

        	nRet &= ~ECC_NEED_DECODING_AGAIN;
        	nRet |= Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
        	if ( nRet&ECC_NEED_DECODING_AGAIN )
        	{
            	nRet &= ~ECC_NEED_DECODING_AGAIN;
        	    nRet |= ECC_UNCORRECTABLE_ERROR;
               	NAND_ERR((_T("[FIL:ERR] Read_Spare_8BitECC() : Multiple Decoding Error. Over 8bit Error!!\r\n")));
        	}
    	}
#endif

		if (bCheckAllFF)
		{
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Read 8 byte Spare ECC data  ->ASECC[0]
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[1]
			pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[2]
			pSpareCxt[nPosPtr++] = (UINT32)(NF_DATA_R()&0xff);	// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[3]
		}

        // Read and Trash 3 bytes, because there are 3 remaind data
		cReadAndTrash = NF_DATA_R();
		cReadAndTrash = NF_DATA_R();
		cReadAndTrash = NF_DATA_R();

		pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Read 8 byte Spare ECC data  ->ASECC[0]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[1]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();	// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[2]
		pSpareCxt[nPosPtr++] = (UINT32)(NF_DATA_R()&0xff);		// Actually after read 7th byte, ECC decoding starts!!  ->ASECC[3]

_B_CheckMore:
		// 2nd Try
		if (nRet == ECC_UNCORRECTABLE_ERROR)
		{

			NAND_ERR((_T("[FIL:ERR] Read_Spare_8BitECC() : ECC Uncorrectable Error in Spare of Page %d 1st Time : 0x%x\r\n"), nPpn, nRet));
			if (bCheckMore == TRUE32) NAND_ERR((_T("[FIL:ERR] Read_Spare_8BitECC() : Try ECC Decoding again with 2nd SECC copy\r\n")));
			else                      NAND_ERR((_T("[FIL:ERR] Read_Spare_8BitECC() : Try ECC Decoding again with SECC bit change\r\n")));

			nOffset = BYTES_PER_MAIN_PAGE+NAND_SCXT_OFFSET;		// Position to SpareData

			NF_CMD(CMD_RANDOM_DATA_OUTPUT);
			NF_ADDR(nOffset&0xFF);
			NF_ADDR((nOffset>>8)&0xFF);
			NF_CMD(CMD_RANDOM_DATA_OUTPUT_CONFIRM);

			// Initialize 4-bit ECC Decoding
			NF_SET_ECC_DEC();
			NF_MECC_Reset();
			NF_MECC_UnLock();

			nPosPtr = NAND_SCXT_OFFSET/4;

			if (SECTORS_PER_PAGE == 8)
			{
				for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_8BIT_ECC_4K - NAND_SCXT_OFFSET)/4; nCnt++)
					pSpareCxt[nPosPtr++] = NF_DATA_R4();		// 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)
			}
			NF_CLEAR_ECC_DEC_DONE();
			NF_CE_H(nBank);
   			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

			if (SECTORS_PER_PAGE == 8)
			{
				_Write_Dummy_364Byte_AllFF();
			}

			//NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding

			if (bCheckMore == TRUE32)
			{
				NF_DATA_W4(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)]);		// Write 2nd Spare ECC,  ->aSECC[2]
				NF_DATA_W4(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)+1]);		// Write 2nd Spare ECC,  ->aSECC[2]
				NF_DATA_W4(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)+2]);		// Write 2nd Spare ECC,  ->aSECC[2]
				NF_DATA_W(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)+3]&0xff);		// Write 2nd Spare ECC,  ->aSECC[2]
				bCheckMore = FALSE32;
			}
			else
			{
				NF_DATA_W4(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)]);		// Write 2nd Spare ECC,  ->aSECC[2]
				NF_DATA_W4(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)+1]);		// Write 2nd Spare ECC,  ->aSECC[2]
				NF_DATA_W4(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)+2]);		// Write 2nd Spare ECC,  ->aSECC[2]
				NF_DATA_W(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)+3]&0xff);		// Write 2nd Spare ECC,  ->aSECC[2]
				bCheckMore = TRUE32;
			}

        	if ( bReadSafeMode == TRUE32 )
        	{
            	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        	}
        	else
        	{
    			NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
            }
			NF_CE_L(nBank);
			NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
			// Wait Spare ECC Compare Done
//			NF_WAIT_ECC_DEC_DONE();

    		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);
    		nRet = Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
#if (PROVE_ECC_ALGORITHM)
        	if ( nRet&ECC_NEED_DECODING_AGAIN )
        	{
    			// Initialize 4-bit ECC Decoding
    			NF_SET_ECC_DEC();
    			NF_MECC_Reset();
    			NF_MECC_UnLock();

    			NF_CE_H(nBank);
       			NF_SET_CLK(DUMMY_R_TACLS, DUMMY_R_TWRPH0, DUMMY_R_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs

    			nPosPtr = NAND_SCXT_OFFSET/4;

    			for (nCnt = 0; nCnt < (NAND_SECC_OFFSET_8BIT_ECC_4K - NAND_SCXT_OFFSET)/4; nCnt++)
    				NF_DATA_W4(pSpareCxt[nPosPtr++]);		// 20 byte Spare Context(->aSpareData) + 64 byte Sector ECC data(->aMECC)

    			NF_CLEAR_ECC_DEC_DONE();
    			_Write_Dummy_364Byte_AllFF();

    			if (bCheckMore == FALSE32)      // Change from TRUE32 to FALSE32.. in PROVE_ECC_ALGORITHM
    			{
    				NF_DATA_W4(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)]);		// Write 2nd Spare ECC,  ->aSECC[2]
    				NF_DATA_W4(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)+1]);		// Write 2nd Spare ECC,  ->aSECC[2]
    				NF_DATA_W4(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)+2]);		// Write 2nd Spare ECC,  ->aSECC[2]
    				NF_DATA_W(pSpareCxt[(NAND_SECC2_OFFSET_8BIT_ECC_4K/4)+3]&0xff);		// Write 2nd Spare ECC,  ->aSECC[2]
    			}
    			else
    			{
    				NF_DATA_W4(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)]);		// Write 2nd Spare ECC,  ->aSECC[2]
    				NF_DATA_W4(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)+1]);		// Write 2nd Spare ECC,  ->aSECC[2]
    				NF_DATA_W4(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)+2]);		// Write 2nd Spare ECC,  ->aSECC[2]
    				NF_DATA_W(pSpareCxt[(NAND_SECC_OFFSET_8BIT_ECC_4K/4)+3]&0xff);		// Write 2nd Spare ECC,  ->aSECC[2]
    			}

            	if ( bReadSafeMode == TRUE32 )
            	{
                	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
            	}
            	else
            	{
                	NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
                }
    			NF_CE_L(nBank);
    			NF_MECC_Lock();	// Do NOT Lock MECC when using 4-bit ECC Decoding
    			// Wait Spare ECC Compare Done
//    			NF_WAIT_ECC_DEC_DONE();
        		pSBuf =(UINT8*) &(pSpareCxt[NAND_SCXT_OFFSET/4]);

               	nRet &= ~ECC_NEED_DECODING_AGAIN;
            	nRet |= Decoding_Spare8BitECC(pSBuf, nEffectiveByte);
            	if ( nRet&ECC_NEED_DECODING_AGAIN )
            	{
                	nRet &= ~ECC_NEED_DECODING_AGAIN;
            	    nRet |= ECC_UNCORRECTABLE_ERROR;
                	NAND_ERR((_T("[FIL:ERR] Read_Spare_8BitECC() :Multiple Decoding Error. Over 8bit Error!!\r\n")));
            	}
        	}
#endif

			if (bCheckMore == TRUE32)
				goto _B_CheckMore;
		}
	}
	else
	{
		pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 8 byte Spare ECC data,  ->aSECC[0]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();			// 8 byte Spare ECC data 2nd copy  ->aSECC[4]
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
		pSpareCxt[nPosPtr++] = NF_DATA_R4();
	}

	if (nRet&ECC_UNCORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL:ERR] Read_Spare_8BitECC() : ECC Uncorrectable Error in Spare of Page %d\r\n"), nPpn));

        // Print the current(latest) L,V,P number of block, page, sector.
        NAND_ERR((_T("[FIL:ERR] Lsn(0x%x) -> Lbn(0x%x) -> Vbn(0x%x) used in FTL.\r\n"), WMR_LLSN, WMR_LLBN, WMR_LVBN));
        NAND_ERR((_T("[FIL:ERR] Vpn(0x%x) -> Pbn(0x%x) -> Ppn(0x%x) used in VFL.\r\n"), WMR_LVPN, WMR_LPBN, WMR_LPPN));

        // Print the failed spare data.
        MLC_Print_Page_Data(NULL, (UINT8 *)pSpareCxt);

        // Print the NFCON SFR.
        MLC_Print_SFR();
	}
	else if (nRet&ECC_CORRECTABLE_ERROR)
	{
		NAND_ERR((_T("[FIL] Read_Spare_8BitECC() : ECC Correctable Error in Spare of Page %d\r\n"), nPpn));
	}

	NAND_MSG((_T("[FIL]--Read_Spare_8BitECC()\r\n")));

	return nRet;
}

PRIVATE VOID
Write_Sector(UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf)
{
	UINT32 nOffset;

	NAND_MSG((_T("[FIL]++Write_Sector(%d, %d)\r\n"), nPpn, nSctOffset));
	//NAND_ERR((_T("[FIL]++Write_Sector(%d, %d)\r\n"), nPpn, nSctOffset));  // ksk dbg

	nOffset = NAND_SECTOR_SIZE*nSctOffset;

	NF_CMD(CMD_RANDOM_DATA_INPUT);
	NF_ADDR(nOffset&0xFF);
	NF_ADDR((nOffset>>8)&0xFF);

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 4-bit ECC Encoding
		NF_SET_ECC_ENC();
		NF_MECC_Reset();
		NF_CLEAR_ECC_ENC_DONE();
		NF_MECC_UnLock();
	}

	if ((UINT32)pBuf&0x3)
	{
		_Write_512Byte_Unaligned(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}
	else
	{
		_Write_512Byte(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}
	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		NF_MECC_Lock();

		// Waiting for Main ECC Encoding
		NF_WAIT_ECC_ENC_DONE();
	}

	NAND_MSG((_T("[FIL]--Write_Sector()\r\n")));

	return;
}


PRIVATE VOID
Write_Sector_8BitECC(UINT32 nPpn, UINT32 nSctOffset, UINT8* pBuf)
{
	UINT32 nOffset;
#if ECC_MODULE_TEST_SIMULATION
	BOOL32 bChangeData = FALSE;
#endif

	NAND_MSG((_T("[FIL]++Write_Sector_8BitECC(%d, %d)\r\n"), nPpn, nSctOffset));
#if ECC_MODULE_TEST_SIMULATION
Loop1:
#endif

	nOffset = NAND_SECTOR_SIZE*nSctOffset;

	NF_CMD(CMD_RANDOM_DATA_INPUT);
	NF_ADDR(nOffset&0xFF);
	NF_ADDR((nOffset>>8)&0xFF);
#if ECC_MODULE_TEST_SIMULATION
	if (IS_CHECK_SPARE_ECC == TRUE32 && bChangeData == FALSE)
#else
	if (IS_CHECK_SPARE_ECC == TRUE32)
#endif
	{
		// Initialize 8-bit ECC Encoding
		NF_8BIT_ECC_STOP();
		NF_SET_ECC_ENC();
		NF_MECC_UnLock();
		NF_MECC_Reset();
		NF_CLEAR_ECC_ENC_DONE();
	}
#if ECC_MODULE_TEST_SIMULATION
	else if (bChangeData == TRUE)
	{
		if (nSctOffset == 0)  // forcefully happen to error on 4-bit in case of ECC mobule error
		{
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+243) ^= (1<<4);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+419) ^= (1<<3);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+333) ^= (1<<2);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+308) ^= (1<<4);
		}
		else if (nSctOffset == 1)  // forcefully happen to error on 4-bit in case of ECC mobule error
		{
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+228) ^= (1<<0);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+400) ^= (1<<3);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+486) ^= (1<<4);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+165) ^= (1<<2);
		}
		else if (nSctOffset == 2)  // forcefully happen to error on 4-bit except ECC mobule error
		{
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+228) ^= (1<<0);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+400) ^= (1<<3);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+486) ^= (1<<4);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+165) ^= (1<<1);
		}
		else if (nSctOffset == 3)  // forcefully happen to error on 3-bit in case of ECC mobule error
		{
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+148) ^= (1<<3);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+435) ^= (1<<2);
			*(pBuf+NAND_SECTOR_SIZE*nSctOffset+501) ^= (1<<1);
		}
	}
#endif

	if ((UINT32)pBuf&0x3)
	{
		_Write_512Byte_Unaligned(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}
	else
	{
		_Write_512Byte(pBuf+NAND_SECTOR_SIZE*nSctOffset);
	}

#if ECC_MODULE_TEST_SIMULATION
	if (IS_CHECK_SPARE_ECC == TRUE32 && bChangeData == FALSE)
#else
	if (IS_CHECK_SPARE_ECC == TRUE32)
#endif
	{
		NF_MECC_Lock();

		// Waiting for Main ECC Encoding
		NF_WAIT_ECC_ENC_DONE();
	}
#if ECC_MODULE_TEST_SIMULATION
	if (bChangeData == FALSE)
	{
		bChangeData = TRUE;
		goto Loop1;
	}
#endif

	NAND_MSG((_T("[FIL]--Write_Sector_8BitECC()\r\n")));

	return;
}


PRIVATE VOID
Write_Spare(UINT32 nBank, UINT32 nPpn, pSECCCxt pSpareCxt)
{
	UINT32 nOffset;

	NAND_MSG((_T("[FIL]++Write_Spare(%d, %d)\r\n"), nBank, nPpn));
	//NAND_ERR((_T("[FIL]++Write_Spare(%d, %d)\r\n"), nBank, nPpn));  // ksk dbg

	nOffset = BYTES_PER_MAIN_PAGE;

	NF_CMD(CMD_RANDOM_DATA_INPUT);
	NF_ADDR(nOffset&0xFF);
	NF_ADDR((nOffset>>8)&0xFF);

	NF_DATA_W(pSpareCxt->cBadMark);			// 1 byte Bad Mark
	NF_DATA_W(pSpareCxt->cCleanMark);			// 1 byte Clean Mark

#if	1
	NF_DATA_W(0xff);			// 2 byte Reserved
	NF_DATA_W(0xff);
#else
	NF_DATA_W(pSpareCxt->cReserved[0]); 	// 2 byte Reserved
	NF_DATA_W(pSpareCxt->cReserved[1]);
#endif

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 4-bit ECC Encoding
		NF_SET_ECC_ENC();
		NF_MECC_Reset();
		NF_CLEAR_ECC_ENC_DONE();
		NF_MECC_UnLock();
	}

	if (SECTORS_PER_PAGE == 4)
	{
		NF_DATA_W4(pSpareCxt->aSpareData[0]);		// 12 byte Spare Context
		NF_DATA_W4(pSpareCxt->aSpareData[1]);
		NF_DATA_W4(pSpareCxt->aSpareData[2]);

		NF_DATA_W4(pSpareCxt->aMECC[0]);		// 8 byte Sector0 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[1]);
		NF_DATA_W4(pSpareCxt->aMECC[2]);		// 8 byte Sector1 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[3]);
		NF_DATA_W4(pSpareCxt->aMECC[4]);		// 8 byte Sector2 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[5]);
		NF_DATA_W4(pSpareCxt->aMECC[6]);		// 8 byte Sector3 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[7]);
	}
	else if (SECTORS_PER_PAGE == 8)
	{
		NF_DATA_W4(pSpareCxt->aSpareData[0]);		// 20 byte Spare Context for 4KByte/Page
		NF_DATA_W4(pSpareCxt->aSpareData[1]);
		NF_DATA_W4(pSpareCxt->aSpareData[2]);
		NF_DATA_W4(pSpareCxt->aSpareData[3]);
		NF_DATA_W4(pSpareCxt->aSpareData[4]);

		NF_DATA_W4(pSpareCxt->aMECC[0]);		// 8 byte Sector0 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[1]);
		NF_DATA_W4(pSpareCxt->aMECC[2]);		// 8 byte Sector1 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[3]);
		NF_DATA_W4(pSpareCxt->aMECC[4]);		// 8 byte Sector2 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[5]);
		NF_DATA_W4(pSpareCxt->aMECC[6]);		// 8 byte Sector3 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[7]);
		NF_DATA_W4(pSpareCxt->aMECC[8]);		// 8 byte Sector4 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[9]);
		NF_DATA_W4(pSpareCxt->aMECC[10]);		// 8 byte Sector5 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[11]);
		NF_DATA_W4(pSpareCxt->aMECC[12]);		// 8 byte Sector6 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[13]);
		NF_DATA_W4(pSpareCxt->aMECC[14]);		// 8 byte Sector7 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[15]);
	}
	else
	{
		WMR_ASSERT(FALSE32);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Write Dummy 500 byte for ECC Encoding using CE Don't care
		NF_CE_H(nBank);
    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
        	NF_SET_CLK(DUMMY_W_TACLS, DUMMY_W_TWRPH0, DUMMY_W_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        }

		if (SECTORS_PER_PAGE == 4)
		{
			_Write_Dummy_468Byte_AllFF();
		}
		else if (SECTORS_PER_PAGE == 8)
		{
			_Write_Dummy_428Byte_AllFF();
		}

    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
    		NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
		NF_CE_L(nBank);

		NF_MECC_Lock();

		// Waiting for Main ECC Encoding
		NF_WAIT_ECC_ENC_DONE();

		pSpareCxt->aSECC[0] = NF_MECC0();	// Spare ECC x 2 copies
		pSpareCxt->aSECC[1] = NF_MECC1() | (0xff<<24);
		pSpareCxt->aSECC[2] = NF_MECC0();
		pSpareCxt->aSECC[3] = NF_MECC1() | (0xff<<24);
	}
	NF_DATA_W4(pSpareCxt->aSECC[0]);		// Spare ECC 8 bytes
	NF_DATA_W4(pSpareCxt->aSECC[1]);
	NF_DATA_W4(pSpareCxt->aSECC[2]);		// Spare ECC 8 bytes 2nd copy
	NF_DATA_W4(pSpareCxt->aSECC[3]);

	NAND_MSG((_T("[FIL]--Write_Spare()\r\n")));

	return;
}

PRIVATE VOID
Write_Spare_Separate(UINT32 nBank, UINT32 nPpn, pSECCCxt pSpareCxt)
{
	UINT32 nOffset;

	NAND_MSG((_T("[FIL]++Write_Spare_Separate(%d, %d)\r\n"), nBank, nPpn));

	nOffset = BYTES_PER_MAIN_PAGE;

	NF_CMD(CMD_RANDOM_DATA_INPUT);
	NF_ADDR(nOffset&0xFF);
	NF_ADDR((nOffset>>8)&0xFF);

	NF_DATA_W(pSpareCxt->cBadMark);			// 1 byte Bad Mark
	NF_DATA_W(pSpareCxt->cCleanMark);			// 1 byte Clean Mark

#if	1
	NF_DATA_W(0xff);			// 2 byte Reserved
	NF_DATA_W(0xff);
#else
	NF_DATA_W(pSpareCxt->cReserved[0]); 	// 2 byte Reserved
	NF_DATA_W(pSpareCxt->cReserved[1]);
#endif

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 8-bit ECC Encoding
		NF_8BIT_ECC_STOP();
		NF_SET_ECC_ENC();
		NF_MECC_UnLock();
		NF_MECC_Reset();
		NF_CLEAR_ECC_ENC_DONE();
	}

    if (SECTORS_PER_PAGE == 8)
	{
		NF_DATA_W4(pSpareCxt->aSpareData[0]);		// 20 byte Spare Context for 4KByte/Page
		NF_DATA_W4(pSpareCxt->aSpareData[1]);
		NF_DATA_W4(pSpareCxt->aSpareData[2]);
		NF_DATA_W4(pSpareCxt->aSpareData[3]);
		NF_DATA_W4(pSpareCxt->aSpareData[4]);
	}
	else
	{
		WMR_ASSERT(FALSE32);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Write Dummy 500 byte for ECC Encoding using CE Don't care
		NF_CE_H(nBank);
    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
        	NF_SET_CLK(DUMMY_W_TACLS, DUMMY_W_TWRPH0, DUMMY_W_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        }

        if (SECTORS_PER_PAGE == 8)
		{
			_Write_Dummy_492Byte_AllFF();
		}

    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
    		NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
		NF_CE_L(nBank);

		NF_MECC_Lock();

		// Waiting for Main ECC Encoding
		NF_WAIT_ECC_ENC_DONE();

		pSpareCxt->aSECC[0] = NF_8MECC0();	// Spare ECC x 2 copies
		pSpareCxt->aSECC[1] = NF_8MECC1();
		pSpareCxt->aSECC[2] = NF_8MECC2();
		pSpareCxt->aSECC[3] = (NF_8MECC3() & 0xff) | (0xffffff<<8);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 8-bit ECC Encoding
		NF_8BIT_ECC_STOP();
		NF_SET_ECC_ENC();
		NF_MECC_UnLock();
		NF_MECC_Reset();
		NF_CLEAR_ECC_ENC_DONE();
	}

    if (SECTORS_PER_PAGE == 8)
	{
		NF_DATA_W4(pSpareCxt->aMECC[0]);		// 8 byte Sector0 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[1]);
		NF_DATA_W4(pSpareCxt->aMECC[2]);		// 8 byte Sector1 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[3]);
		NF_DATA_W4(pSpareCxt->aMECC[4]);		// 8 byte Sector2 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[5]);
		NF_DATA_W4(pSpareCxt->aMECC[6]);		// 8 byte Sector3 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[7]);
		NF_DATA_W4(pSpareCxt->aMECC[8]);		// 8 byte Sector4 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[9]);
		NF_DATA_W4(pSpareCxt->aMECC[10]);		// 8 byte Sector5 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[11]);
		NF_DATA_W4(pSpareCxt->aMECC[12]);		// 8 byte Sector6 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[13]);
		NF_DATA_W4(pSpareCxt->aMECC[14]);		// 8 byte Sector7 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[15]);
	}
	else
	{
		WMR_ASSERT(FALSE32);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Write Dummy 500 byte for ECC Encoding using CE Don't care
		NF_CE_H(nBank);
    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
        	NF_SET_CLK(DUMMY_W_TACLS, DUMMY_W_TWRPH0, DUMMY_W_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        }

        if (SECTORS_PER_PAGE == 8)
		{
			_Write_Dummy_448Byte_AllFF();
		}

    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
    		NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
		NF_CE_L(nBank);

		NF_MECC_Lock();

		// Waiting for Main ECC Encoding
		NF_WAIT_ECC_ENC_DONE();

		pSpareCxt->aSECC[4] = NF_8MECC0();	// Spare ECC x 2 copies
		pSpareCxt->aSECC[5] = NF_8MECC1();
		pSpareCxt->aSECC[6] = NF_8MECC2();
		pSpareCxt->aSECC[7] = (NF_8MECC3() & 0xff) | (0xffffff<<8);
	}

	NF_DATA_W4(pSpareCxt->aSECC[0]);		// Spare ECC 16 bytes for SCXT
	NF_DATA_W4(pSpareCxt->aSECC[1]);
	NF_DATA_W4(pSpareCxt->aSECC[2]);
	NF_DATA_W4(pSpareCxt->aSECC[3]);
	NF_DATA_W4(pSpareCxt->aSECC[4]);		// Spare ECC 16 bytes for MECC Parity Code
	NF_DATA_W4(pSpareCxt->aSECC[5]);
	NF_DATA_W4(pSpareCxt->aSECC[6]);
	NF_DATA_W4(pSpareCxt->aSECC[7]);

	NAND_MSG((_T("[FIL]--Write_Spare_Separate()\r\n")));

	return;
}


PRIVATE VOID
Write_Spare_8BitECC(UINT32 nBank, UINT32 nPpn, pSECCCxt pSpareCxt)
{
	UINT32 nOffset;

	NAND_MSG((_T("[FIL]++Write_Spare(%d, %d)\r\n"), nBank, nPpn));

	nOffset = BYTES_PER_MAIN_PAGE;

	NF_CMD(CMD_RANDOM_DATA_INPUT);
	NF_ADDR(nOffset&0xFF);
	NF_ADDR((nOffset>>8)&0xFF);

	NF_DATA_W(pSpareCxt->cBadMark);			// 1 byte Bad Mark
	NF_DATA_W(pSpareCxt->cCleanMark);		// 1 byte Clean Mark

#if	1
	NF_DATA_W(0xff);			            // 2 byte Reserved
	NF_DATA_W(0xff);
#else
	NF_DATA_W(pSpareCxt->cReserved[0]); 	// 2 byte Reserved
	NF_DATA_W(pSpareCxt->cReserved[1]);
#endif

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Initialize 8-bit ECC Encoding
		NF_8BIT_ECC_STOP();
		NF_SET_ECC_ENC();
		NF_MECC_UnLock();
		NF_MECC_Reset();
		NF_CLEAR_ECC_ENC_DONE();
	}

	if (SECTORS_PER_PAGE == 8)
	{
		NF_DATA_W4(pSpareCxt->aSpareData[0]);		// 20 byte Spare Context for 4KByte/Page
		NF_DATA_W4(pSpareCxt->aSpareData[1]);
		NF_DATA_W4(pSpareCxt->aSpareData[2]);
		NF_DATA_W4(pSpareCxt->aSpareData[3]);
		NF_DATA_W4(pSpareCxt->aSpareData[4]);

		NF_DATA_W4(pSpareCxt->aMECC[0]);		// 16 byte Sector0 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[1]);
		NF_DATA_W4(pSpareCxt->aMECC[2]);
		NF_DATA_W4(pSpareCxt->aMECC[3]);
		NF_DATA_W4(pSpareCxt->aMECC[4]);		// 16 byte Sector1 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[5]);
		NF_DATA_W4(pSpareCxt->aMECC[6]);
		NF_DATA_W4(pSpareCxt->aMECC[7]);
		NF_DATA_W4(pSpareCxt->aMECC[8]);		// 16 byte Sector2 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[9]);
		NF_DATA_W4(pSpareCxt->aMECC[10]);
		NF_DATA_W4(pSpareCxt->aMECC[11]);
		NF_DATA_W4(pSpareCxt->aMECC[12]);		// 16 byte Sector3 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[13]);
		NF_DATA_W4(pSpareCxt->aMECC[14]);
		NF_DATA_W4(pSpareCxt->aMECC[15]);
		NF_DATA_W4(pSpareCxt->aMECC[16]);		// 16 byte Sector4 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[17]);
		NF_DATA_W4(pSpareCxt->aMECC[18]);
		NF_DATA_W4(pSpareCxt->aMECC[19]);
		NF_DATA_W4(pSpareCxt->aMECC[20]);		// 16 byte Sector5 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[21]);
		NF_DATA_W4(pSpareCxt->aMECC[22]);
		NF_DATA_W4(pSpareCxt->aMECC[23]);
		NF_DATA_W4(pSpareCxt->aMECC[24]);		// 16 byte Sector6 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[25]);
		NF_DATA_W4(pSpareCxt->aMECC[26]);
		NF_DATA_W4(pSpareCxt->aMECC[27]);
		NF_DATA_W4(pSpareCxt->aMECC[28]);		// 16 byte Sector7 ECC data
		NF_DATA_W4(pSpareCxt->aMECC[29]);
		NF_DATA_W4(pSpareCxt->aMECC[30]);
		NF_DATA_W4(pSpareCxt->aMECC[31]);
	}
	else
	{
		WMR_ASSERT(FALSE32);
	}

	if (IS_CHECK_SPARE_ECC == TRUE32)
	{
		// Write Dummy 500 byte for ECC Encoding using CE Don't care
		NF_CE_H(nBank);
    	if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
        	NF_SET_CLK(DUMMY_W_TACLS, DUMMY_W_TWRPH0, DUMMY_W_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
        }

		if (SECTORS_PER_PAGE == 8)
		{
			_Write_Dummy_364Byte_AllFF();
		}

		if ( bReadSafeMode == TRUE32 )
    	{
        	NF_SET_CLK(MAX_RW_TACLS, MAX_RW_TWRPH0, MAX_RW_TWRPH1);		// Don't set clk to (0, 0, 0) !!! Decoding error occurs
    	}
    	else
    	{
    		NF_SET_CLK(DEFAULT_TACLS, DEFAULT_TWRPH0, DEFAULT_TWRPH1);
        }
		NF_CE_L(nBank);

		NF_MECC_Lock();

		// Waiting for Main ECC Encoding
		NF_WAIT_ECC_ENC_DONE();

		pSpareCxt->aSECC[0] = NF_8MECC0();	// Spare ECC x 2 copies
		pSpareCxt->aSECC[1] = NF_8MECC1();
		pSpareCxt->aSECC[2] = NF_8MECC2();
		pSpareCxt->aSECC[3] = (NF_8MECC3() & 0xff) | (0xffffff<<8);
		pSpareCxt->aSECC[4] = NF_8MECC0();	// Spare ECC x 2 copies
		pSpareCxt->aSECC[5] = NF_8MECC1();
		pSpareCxt->aSECC[6] = NF_8MECC2();
		pSpareCxt->aSECC[7] = (NF_8MECC3() & 0xff) | (0xffffff<<8);
	}

	NF_DATA_W4(pSpareCxt->aSECC[0]);		// Spare ECC 8 bytes
	NF_DATA_W4(pSpareCxt->aSECC[1]);
	NF_DATA_W4(pSpareCxt->aSECC[2]);		// Spare ECC 8 bytes 2nd copy
	NF_DATA_W4(pSpareCxt->aSECC[3]);
	NF_DATA_W4(pSpareCxt->aSECC[4]);		// Spare ECC 8 bytes
	NF_DATA_W4(pSpareCxt->aSECC[5]);
	NF_DATA_W4(pSpareCxt->aSECC[6]);		// Spare ECC 8 bytes 2nd copy
	NF_DATA_W4(pSpareCxt->aSECC[7]);

	NAND_MSG((_T("[FIL]--Write_Spare()\r\n")));

	return;
}

#define NUMBER_OF_ECC_ERROR 3

PRIVATE UINT32
Decoding_MainECC(UINT8* pBuf)
{
	UINT32 nError0, nError1;
	UINT32 nErrorCnt, nErrorByte, nErrorPattern;
	UINT32 nRet = 0;
	UINT8  nErrorBitPat;

	NAND_MSG((_T("[FIL]++Decoding_MainECC()\r\n")));

	nError0 = NF_ECC_ERR0();
	nError1 = NF_ECC_ERR1();

	//NAND_MSG((_T("[FIL] NF_ECC_ERR0 = 0x%08x()\r\n"), nError0));
	//NAND_MSG((_T("[FIL] NF_ECC_ERR1 = 0x%08x()\r\n"), nError1));

	nErrorCnt = (nError0>>26)&0x7;

	if (nErrorCnt == 0)			// No Error
	{
		NAND_MSG((_T("[FIL] Decoding_MainECC() : No ECC Error\r\n")));
	}
	else	 if (nErrorCnt > 4)			// Uncorrectable Error
	{
		NAND_ERR((_T("[FIL:ERR] Decoding_MainECC() : Uncorrectable Error\r\n")));
		nRet = ECC_UNCORRECTABLE_ERROR;
	}
	else							// Correctable Error
	{
		NAND_MSG((_T("[FIL] Decoding_MainECC() : Correctable Error %d bit\r\n"), nErrorCnt));

		nErrorPattern = NF_ECC_ERR_PATTERN();

		// 1st Bit Error Correction
		nErrorByte = nError0&0x3ff;
		nErrorBitPat = (UINT8)(nErrorPattern&0xff);
		if (nErrorByte < 512)
		{
			NAND_MSG((_T("[FIL] Decoding_MainECC() : 1st Error Buf[%d] [%02x]->"), nErrorByte, pBuf[nErrorByte]));
			pBuf[nErrorByte] = pBuf[nErrorByte]^nErrorBitPat;
			NAND_MSG((_T("[%02x]\r\n"), pBuf[nErrorByte]));
		}

		if (nErrorCnt > 1)
		{
			// 2nd Bit Error Correction
			nErrorByte = (nError0>>16)&0x3ff;
			nErrorBitPat = (UINT8)((nErrorPattern>>8)&0xff);
			if (nErrorByte < 512)
			{
				NAND_MSG((_T("[FIL] Decoding_MainECC() : 2nd Error Buf[%d] [%02x]->"), nErrorByte, pBuf[nErrorByte]));
				pBuf[nErrorByte] = pBuf[nErrorByte]^nErrorBitPat;
				NAND_MSG((_T("[%02x]\r\n"), pBuf[nErrorByte]));
			}

			if (nErrorCnt > 2)
			{
				// 3rd Bit Error Correction
				nErrorByte = nError1&0x3ff;
				nErrorBitPat = (UINT8)((nErrorPattern>>16)&0xff);
				if (nErrorByte < 512)
				{
					NAND_MSG((_T("[FIL] Decoding_MainECC() : 3rd Error Buf[%d] [%02x]->"), nErrorByte, pBuf[nErrorByte]));
					pBuf[nErrorByte] = pBuf[nErrorByte]^nErrorBitPat;
					NAND_MSG((_T("[%02x]\r\n"), pBuf[nErrorByte]));
				}

				if (nErrorCnt > 3)
				{
					// 4 th Bit Error Correction
					nErrorByte = (nError1>>16)&0x3ff;
					nErrorBitPat = (UINT8)((nErrorPattern>>24)&0xff);
					if (nErrorByte < 512)
					{
						NAND_MSG((_T("[FIL] Decoding_MainECC() : 4th Error Buf[%d] [%02x]->"), nErrorByte, pBuf[nErrorByte]));
						pBuf[nErrorByte] = pBuf[nErrorByte]^nErrorBitPat;
						NAND_MSG((_T("[%02x]\r\n"), pBuf[nErrorByte]));
					}
				}
			}
		}

#if (PROVE_ECC_ALGORITHM)
		nRet |= ECC_NEED_DECODING_AGAIN;
#endif
		if (nErrorCnt >= NUMBER_OF_ECC_ERROR) nRet |= ECC_CORRECTABLE_ERROR;
	}

	NAND_MSG((_T("[FIL]--Decoding_MainECC()\r\n")));

	return nRet;
}


#define NUMBER_OF_8BIT_ECC_ERROR (5)

PRIVATE UINT32
Decoding_Main8BitECC(UINT8* pBuf)
{
	UINT32 nErrorByte[8], nTempError0, nTempError1, nTempError2;
	UINT32 nTempErrorPattern0,nTempErrorPattern1;
	UINT32 nErrorCnt,nLoop;
	UINT32 nRet = 0;
	UINT32 nEffectiveByte;
	BOOL32 bDummyError = FALSE32;
	UINT8 nErrorPattern[8];

	NAND_MSG((_T("[FIL]++Decoding_Main8BitECC()\r\n")));

	nEffectiveByte = NAND_SECC_OFFSET_8BIT_ECC_4K - NAND_SCXT_OFFSET;  // 20B + 16*8B = 148 [152(1B+1B+2B+20B+128B)-4]

	nTempError0 = NF_8ECC_ERR0();
	nTempError1 = NF_8ECC_ERR1();
	nTempError2 = NF_8ECC_ERR2();

	nErrorCnt = (nTempError0>>25)&0xF;

	if (nErrorCnt == 0)			// No Error
	{
		NAND_MSG((_T("[FIL] Decoding_Main8BitECC() : No ECC Error\r\n")));
	}
	else if (nErrorCnt > 8)
	{
		NAND_ERR((_T("[FIL:ERR] Decoding_Main8BitECC() : Uncorrectable Error (8BIT ECC) \r\n")));
		nRet = ECC_UNCORRECTABLE_ERROR;
	}
	else
	{
		// Init Error information 080923 hsjang

		nTempErrorPattern0 = NF_8ECC_ERR_PATTERN0();
		nTempErrorPattern1 = NF_8ECC_ERR_PATTERN1();

		nErrorByte[0] = ((nTempError0>>0)&0x3ff);
		nErrorByte[1] = ((nTempError0>>15)&0x3ff);
		nErrorByte[2] = ((nTempError1>>0)&0x3ff);
		nErrorByte[3] = ((nTempError1>>11)&0x3ff);
		nErrorByte[4] = ((nTempError1>>22)&0x3ff);
		nErrorByte[5] = ((nTempError2>>0)&0x3ff);
		nErrorByte[6] = ((nTempError2>>11)&0x3ff);
		nErrorByte[7] = ((nTempError2>>22)&0x3ff);

		nErrorPattern[0] = ((nTempErrorPattern0>>0)&0xff);
		nErrorPattern[1] = ((nTempErrorPattern0>>8)&0xff);
		nErrorPattern[2] = ((nTempErrorPattern0>>16)&0xff);
		nErrorPattern[3] = ((nTempErrorPattern0>>24)&0xff);
		nErrorPattern[4] = ((nTempErrorPattern1>>0)&0xff);
		nErrorPattern[5] = ((nTempErrorPattern1>>8)&0xff);
		nErrorPattern[6] = ((nTempErrorPattern1>>16)&0xff);
		nErrorPattern[7] = ((nTempErrorPattern1>>24)&0xff);

		for ( nLoop = 0 ; nLoop < nErrorCnt && nLoop < 8 ; nLoop++ )
		{
			if (nErrorByte[nLoop] < 512)
			{
				NAND_MSG((_T("[FIL] Decoding_Main8BitECC() : 1st Error Buf[%d] [%02x]->"), nErrorByte[nLoop], pBuf[nErrorByte[nLoop]]));
				pBuf[nErrorByte[nLoop]] = pBuf[nErrorByte[nLoop]]^nErrorPattern[nLoop];
				NAND_MSG((_T("[%02x]\r\n"), pBuf[nErrorByte[nLoop]]));
			}
		}

#if (PROVE_ECC_ALGORITHM)
		nRet |= ECC_NEED_DECODING_AGAIN;
#endif
		if (nErrorCnt >= NUMBER_OF_8BIT_ECC_ERROR) nRet |= ECC_CORRECTABLE_ERROR;
	}
	NAND_MSG((_T("[FIL]--Decoding_Main8BitECC()\r\n")));

	return nRet;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Meaningful ECC error is first 24 bytes of 512 byte
//
//	SpareData + MECC_Data     + DummyData
//	12 byte   + MECC 8x4 byte + 468 byte Dummy = 512 bytes : 2KByte/Page
//	20 byte   + MECC 8x8 byte + 428 byte Dummy = 512 bytes : 4KByte/Page
//
//////////////////////////////////////////////////////////////////////////////
PRIVATE UINT32
Decoding_SpareECC(UINT8* pBuf)
{
	UINT32 nError0, nError1;
	UINT32 nErrorCnt;
	UINT32 nRet = 0;
	UINT32 nEffectiveByte;
	BOOL32 bDummyError = FALSE32;
	UINT8  nErrorBitPat;
	UINT32 nEffectiveAreaErrorCnt = 0;

	NAND_MSG((_T("[FIL]++Decoding_SpareECC()\r\n")));

	if (SECTORS_PER_PAGE == 8)
	{
		nEffectiveByte = NAND_SECC_OFFSET_4K - NAND_SCXT_OFFSET;  // 20B + 8*8B
	}
	else
	{
		nEffectiveByte = NAND_SECC_OFFSET - NAND_SCXT_OFFSET;  // 12B + 8*4B
	}

	nError0 = NF_ECC_ERR0();
	nError1 = NF_ECC_ERR1();

	nErrorCnt = (nError0>>26)&0x7;

	if (nErrorCnt == 0)			// No ECC Error
	{
		NAND_MSG((_T("[FIL] Decoding_SpareECC() : No ECC Error\r\n")));
	}
	else if (nErrorCnt > 4)			// Uncorrectable Error
	{
		NAND_ERR((_T("[FIL:ERR] Decoding_SpareECC() : Uncorrectable Error\r\n")));
		nRet = ECC_UNCORRECTABLE_ERROR;
	}
	else		// Check ECC error occurs in first 44 (12+32) bytes (468 byte is Dummy 0xFF) for 2KByte/Page
	{
		UINT32 nErrorByte, nErrorPattern;
		UINT8 cTempBuf;

		nErrorPattern = NF_ECC_ERR_PATTERN();

		// 1st Bit Error Correction
		nErrorByte = nError0&0x3ff;
		nErrorBitPat = (UINT8)(nErrorPattern&0xff);
		if (nErrorByte < nEffectiveByte)
		{
			cTempBuf = pBuf[nErrorByte];
			pBuf[nErrorByte] = cTempBuf^nErrorBitPat;
			NAND_MSG((_T("[FIL] Decoding_SpareECC() : 1st Error Buf[%d] [%02x]->[%02x]\r\n"), nErrorByte, cTempBuf, pBuf[nErrorByte]));
			nEffectiveAreaErrorCnt ++;
		}
		else if (nErrorByte < 512)
		{
			NAND_MSG((_T("[FIL] Decoding_SpareECC() : 1st Error in Dummy Data Buf[%d]\r\n"), nErrorByte));
			bDummyError = TRUE32;
		}

		if (nErrorCnt > 1)
		{
			// 2nd Bit Error Correction
			nErrorByte = (nError0>>16)&0x3ff;
			nErrorBitPat = (UINT8)((nErrorPattern>>8)&0xff);
			if (nErrorByte < nEffectiveByte)
			{
				cTempBuf = pBuf[nErrorByte];
				pBuf[nErrorByte] = cTempBuf^nErrorBitPat;
				NAND_MSG((_T("[FIL] Decoding_SpareECC() : 2nd Error Buf[%d] [%02x]->[%02x]\r\n"), nErrorByte, cTempBuf, pBuf[nErrorByte]));
				nEffectiveAreaErrorCnt ++;
			}
			else if (nErrorByte < 512)
			{
				NAND_MSG((_T("[FIL] Decoding_SpareECC() : 2nd Error in Dummy Data Buf[%d]\r\n"), nErrorByte));
				bDummyError = TRUE32;
			}

			if (nErrorCnt > 2)
			{
				// 3rd Bit Error Correction
				nErrorByte = nError1&0x3ff;
				nErrorBitPat = (UINT8)((nErrorPattern>>16)&0xff);
				if (nErrorByte < nEffectiveByte)
				{
					cTempBuf = pBuf[nErrorByte];
					pBuf[nErrorByte] = cTempBuf^nErrorBitPat;
					NAND_MSG((_T("[FIL] Decoding_SpareECC() : 3rd Error Buf[%d] [%02x]->[%02x]\r\n"), nErrorByte, cTempBuf, pBuf[nErrorByte]));
    				nEffectiveAreaErrorCnt ++;
				}
				else if (nErrorByte < 512)
				{
					NAND_MSG((_T("[FIL] Decoding_SpareECC() : 3rd Error in Dummy Data Buf[%d]\r\n"), nErrorByte));
					bDummyError = TRUE32;
				}

				if (nErrorCnt > 3)
				{
					// 4 th Bit Error Correction
					nErrorByte = (nError1>>16)&0x3ff;
					nErrorBitPat = (UINT8)((nErrorPattern>>24)&0xff);
					if (nErrorByte < nEffectiveByte)
					{
						cTempBuf = pBuf[nErrorByte];
						pBuf[nErrorByte] = cTempBuf^nErrorBitPat;
						NAND_MSG((_T("[FIL] Decoding_SpareECC() : 4th Error Buf[%d] [%02x]->[%02x]\r\n"), nErrorByte, cTempBuf, pBuf[nErrorByte]));
        				nEffectiveAreaErrorCnt ++;
					}
					else if (nErrorByte < 512)
					{
						NAND_MSG((_T("[FIL] Decoding_SpareECC() : 4th Error in Dummy Data Buf[%d]\r\n"), nErrorByte));
						bDummyError = TRUE32;
					}
				}
			}
		}

		if (bDummyError)			// ECC Error in Dummy Data
		{
			NAND_ERR((_T("[FIL] Decoding_SpareECC() : ECC Error in Dummy Data\r\n")));
			nRet = ECC_UNCORRECTABLE_ERROR;
		}
		else						// Correctable Error
		{
			NAND_MSG((_T("[FIL] Decoding_SpareECC() : Correctable Error %d bits\r\n"), nErrorCnt));
#if (PROVE_ECC_ALGORITHM)
            if ( nEffectiveAreaErrorCnt > 0 )
            {
        		nRet |= ECC_NEED_DECODING_AGAIN;
        	}
#endif
			if (nErrorCnt >= NUMBER_OF_ECC_ERROR) nRet = ECC_CORRECTABLE_ERROR;
		}
	}

	NAND_MSG((_T("[FIL]--Decoding_SpareECC()\r\n")));

	return nRet;
}

//////////////////////////////////////////////////////////////////////////////
//
//	Meaningful ECC error is first 24 bytes of 512 byte
//
//	SpareData + MECC_Data     + DummyData
//	12 byte   + MECC 8x4 byte + 468 byte Dummy = 512 bytes : 2KByte/Page
//	20 byte   + MECC 8x8 byte + 428 byte Dummy = 512 bytes : 4KByte/Page
//  20 byte   + MECC 16x8 byte + 364 byte Dummy = 512 bytes : 4KByte/Page
//
//////////////////////////////////////////////////////////////////////////////
PRIVATE UINT32
Decoding_Spare8BitECC(UINT8* pBuf, UINT32 nEffectiveByte)
{
	UINT32 nErrorByte[8], nTempError0, nTempError1, nTempError2;
	UINT32 nTempErrorPattern0,nTempErrorPattern1;
	UINT32 nErrorCnt,nLoop;
	UINT32 nRet = 0;
	BOOL32 bDummyError = FALSE32;
	UINT8 nErrorPattern[8];
	UINT32 nEffectiveAreaErrorCnt = 0;

	NAND_MSG((_T("[FIL]++Decoding_Spare8BitECC()\r\n")));

	nTempError0 = NF_8ECC_ERR0();
	nTempError1 = NF_8ECC_ERR1();
	nTempError2 = NF_8ECC_ERR2();

	nErrorCnt = (nTempError0>>25)&0xF;
	NAND_MSG((_T("[FIL]++Decoding_Spare8BitECC(nErrorCnt:%d)\r\n"), nErrorCnt));

	if (nErrorCnt == 0)			// No Error
	{
		NAND_MSG((_T("[FIL] Decoding_Spare8BitECC() : No ECC Error\r\n")));
	}
	else if (nErrorCnt > 8)
	{
		NAND_ERR((_T("[FIL:ERR] Decoding_Spare8BitECC() : Uncorrectable Error (8BIT ECC) \r\n")));
		nRet = ECC_UNCORRECTABLE_ERROR;
	}
	else
	{
		UINT8 cTempBuf;
		// Init Error information 080923 hsjang

		nTempErrorPattern0 = NF_8ECC_ERR_PATTERN0();
		nTempErrorPattern1 = NF_8ECC_ERR_PATTERN1();

		nErrorByte[0] = ((nTempError0>>0)&0x3ff);
		nErrorByte[1] = ((nTempError0>>15)&0x3ff);
		nErrorByte[2] = ((nTempError1>>0)&0x3ff);
		nErrorByte[3] = ((nTempError1>>11)&0x3ff);
		nErrorByte[4] = ((nTempError1>>22)&0x3ff);
		nErrorByte[5] = ((nTempError2>>0)&0x3ff);
		nErrorByte[6] = ((nTempError2>>11)&0x3ff);
		nErrorByte[7] = ((nTempError2>>22)&0x3ff);

		nErrorPattern[0] = ((nTempErrorPattern0>>0)&0xff);
		nErrorPattern[1] = ((nTempErrorPattern0>>8)&0xff);
		nErrorPattern[2] = ((nTempErrorPattern0>>16)&0xff);
		nErrorPattern[3] = ((nTempErrorPattern0>>24)&0xff);
		nErrorPattern[4] = ((nTempErrorPattern1>>0)&0xff);
		nErrorPattern[5] = ((nTempErrorPattern1>>8)&0xff);
		nErrorPattern[6] = ((nTempErrorPattern1>>16)&0xff);
		nErrorPattern[7] = ((nTempErrorPattern1>>24)&0xff);

		for ( nLoop = 0 ; nLoop < nErrorCnt && nLoop < 8 ; nLoop++ )
		{
			if (nErrorByte[nLoop] < nEffectiveByte)
			{
			    cTempBuf = pBuf[nErrorByte[nLoop]];
				pBuf[nErrorByte[nLoop]] = pBuf[nErrorByte[nLoop]]^nErrorPattern[nLoop];
				NAND_MSG((_T("[FIL] Decoding_Spare8BitECC() : %dth Error Buf[%d] [%02x]->[%02x]\r\n"), nLoop, nErrorByte[nLoop], cTempBuf, pBuf[nErrorByte[nLoop]]));
				nEffectiveAreaErrorCnt ++;
			}
			else if (nErrorByte[nLoop]< 512)
			{
				NAND_MSG((_T("[FIL] Decoding_Spare8BitECC() : %dth Error in Dummy Data Buf[%d]\r\n"), nLoop, nErrorByte[nLoop]));
				bDummyError = TRUE32;
			}
			else
			{
				NAND_MSG((_T("[FIL] Decoding_Spare8BitECC() : %dth Error in Spare Are[%d]\r\n"), nLoop, nErrorByte[nLoop]));
			}
		}

		if (bDummyError)			// ECC Error in Dummy Data
		{
			NAND_ERR((_T("[FIL] Decoding_Spare8BitECC() : ECC Error in Dummy Data\r\n")));
			nRet = ECC_UNCORRECTABLE_ERROR;
		}
		else						// Correctable Error
		{
			NAND_MSG((_T("[FIL] Decoding_Spare8BitECC() : Correctable Error %d bits\r\n"), nErrorCnt));
#if (PROVE_ECC_ALGORITHM)
            if ( nEffectiveAreaErrorCnt > 0 )
            {
    		    nRet |= ECC_NEED_DECODING_AGAIN;
    		}
#endif
			if (nErrorCnt >= NUMBER_OF_ECC_ERROR) nRet = ECC_CORRECTABLE_ERROR;
		}
	}

	NAND_MSG((_T("[FIL]--Decoding_Spare8BitECC()\r\n")));

	return nRet;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _IsAllFF		                                                     */
/* DESCRIPTION                                                               */
/*      This function inspects the specific area whether its data is 		 */
/*		all 0xFF or not													     */
/* PARAMETERS                                                                */
/*      pBuf     [IN] 		Data buffer to inspect				             */
/*      pSize    [IN] 		Amount of data to inspect						 */
/* RETURN VALUES                                                             */
/*		FALSE	There is a data that is not 0xFF							 */
/*		TURE	All data is 0xFF											 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE UINT32
_IsAllFF(UINT8* pBuf, UINT32 nSize)
{
	register UINT32 nIdx;
	register UINT32 nLoop;
	UINT32 *pBuf32;

	pBuf32 = (UINT32 *)pBuf;
	nLoop = nSize / sizeof(UINT32);

	for (nIdx = nLoop; nIdx > 0; nIdx--)
	{
		if(*pBuf32++ != 0xFFFFFFFF)
		{
			return FALSE32;
		}
	}

	return TRUE32;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      _TRDelay		                                                     */
/* DESCRIPTION                                                               */
/*      This function wait TR.										 		 */
/* PARAMETERS                                                                */
/*		None																 */
/* RETURN VALUES                                                             */
/*		None																 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
PRIVATE UINT32
_TRDelay(UINT32 nNum)
{
	volatile int count;

//	count = 2500;		// 62.5us
	count = 4500;
//	count = 250000;		// chandolp

	while(count--)
	{
		nNum++;
	}

	return nNum;
}

PRIVATE UINT32
_TRDelay2(UINT32 nNum)
{
	volatile int count;

//	count = 20000;		// chandolp
	count = 200;		// 1.25us
	while(count--)
	{
		nNum++;
	}

	return nNum;
}


#if	1	// for test
void MLC_Set_SpareECC(void)
{
	IS_CHECK_SPARE_ECC = TRUE32;
}

void MLC_Clear_SpareECC(void)
{
	IS_CHECK_SPARE_ECC = FALSE32;
}

UINT32 MLC_Read_RAW(UINT32 nBank, UINT32 nPpn, UINT8 *pMBuf, UINT8 *pSBuf)
{
	UINT32 nCnt;
	UINT32 nRet = 0;

	NAND_MSG((_T("[TST]++MLC_Read_RAW(%d)\r\n"), nPpn));

	NF_CE_L(nBank);

	NF_CMD(CMD_READ);
	NF_SET_ADDR(nPpn, 0x00);
	NF_CMD(CMD_READ_CONFIRM);
	NF_WAIT_RnB(nBank);

	// Dummy Command to Set Proper Pointer to Read Position after NF_WAIT_RnB()
	NF_CMD(CMD_READ);

	// Read Main
	for (nCnt = 0; nCnt < SECTORS_PER_PAGE; nCnt++)
	{
		_Read_512Byte(pMBuf+NAND_SECTOR_SIZE*nCnt);
	}

	// Read Spare
	if (BYTES_PER_SPARE_PAGE == 256)
	{
		for (nCnt=0; nCnt<218; nCnt++)
			*pSBuf++ = NF_DATA_R();
	}
	else
	{
		for (nCnt=0; nCnt<BYTES_PER_SPARE_PAGE; nCnt++)
			*pSBuf++ = NF_DATA_R();
	}

	NF_CE_H(nBank);

	NAND_MSG((_T("[TST]--MLC_Read_RAW()\r\n")));

	return 0;
}

UINT32 MLC_Write_RAW(UINT32 nBank, UINT32 nPpn, UINT8 *pMBuf, UINT8 *pSBuf)
{
	UINT32 nCnt;
	UINT32 nRet = 0;

	NAND_MSG((_T("[TST]++MLC_Write_RAW(%d)\r\n"), nPpn));

	NF_CE_L(nBank);

	NF_CMD(CMD_PROGRAM);
	NF_SET_ADDR(nPpn, 0x00);

	// Write Main
	for (nCnt = 0; nCnt < SECTORS_PER_PAGE; nCnt++)
	{
		_Write_512Byte(pMBuf+NAND_SECTOR_SIZE*nCnt);
	}

	// Write Spare
	if ( BYTES_PER_SPARE_PAGE == 256 )
	{
		for (nCnt=0; nCnt<(218); nCnt++)
			NF_DATA_W(*pSBuf++);
	}
	else
	{
		for (nCnt=0; nCnt<(BYTES_PER_SPARE_PAGE); nCnt++)
			NF_DATA_W(*pSBuf++);
	}

	NF_CMD(CMD_PROGRAM_CONFIRM);

	NF_WAIT_RnB(nBank);

	// Write Status Check
	NF_CMD(CMD_READ_STATUS);

	if (NF_DATA_R()&NAND_STATUS_ERROR)
	{
		NAND_ERR((_T("[TST] MLC_Write_RAW() : Status Error\r\n")));
		nRet = 1;
	}

	NF_CE_H(nBank);

	NAND_MSG((_T("[TST]--MLC_Write_RAW()\r\n")));

	return nRet;
}

UINT32 MLC_Erase_RAW(UINT32 nBank, UINT32 nBlock)
{
	UINT32 RowAddr;
	UINT32 nRet = 0;

	NAND_MSG((_T("[TST]++MLC_Erase_RAW(%d)\r\n"), nBlock));

	// Chip Select
	NF_CE_L(nBank);

	// Calculate Row Address of the Block (128 page/block)
	RowAddr = (nBlock * PAGES_PER_BLOCK);//<< (6 + IS_MLC));

	// Erase Command
	NF_CMD( CMD_ERASE);

	// Write Row Address
	NF_ADDR(RowAddr&0xff);
	NF_ADDR((RowAddr>>8)&0xff);
	NF_ADDR((RowAddr>>16)&0xff);

	// Erase confirm command
	NF_CMD( CMD_ERASE_CONFIRM);

	// Wait for Ready
	NF_WAIT_RnB(nBank);

	// Write Status Check
	NF_CMD(CMD_READ_STATUS);

	if (NF_DATA_R()&NAND_STATUS_ERROR)
	{
		NAND_ERR((_T("MLC_Erase_RAW() : Status Error\r\n")));
		nRet = 1;
	}

	NF_CE_H(nBank);

	NAND_ERR((_T("[TST]--MLC_Erase_RAW()\r\n")));

	return nRet;
}

PRIVATE VOID MLC_Print_SFR(VOID)
{
	NAND_ERR((_T("\n=================================================\r\n")));
	NAND_ERR((_T("NFCONF     = 0x%08x\r\n"), pNANDFConReg->NFCONF));
	NAND_ERR((_T("NFCONT     = 0x%08x\r\n"), pNANDFConReg->NFCONT));
	NAND_ERR((_T("NFMECCD0   = 0x%08x\r\n"), pNANDFConReg->NFMECCD0));
	NAND_ERR((_T("NFMECCD1   = 0x%08x\r\n"), pNANDFConReg->NFMECCD1));
	NAND_ERR((_T("NFSECCD    = 0x%08x\r\n"), pNANDFConReg->NFSECCD));
	NAND_ERR((_T("NFSBLK     = 0x%08x\r\n"), pNANDFConReg->NFSBLK));

	NAND_ERR((_T("NFEBLK     = 0x%08x\r\n"), pNANDFConReg->NFEBLK));
	NAND_ERR((_T("NFSTAT     = 0x%08x\r\n"), pNANDFConReg->NFSTAT));
	NAND_ERR((_T("NFECCERR0  = 0x%08x\r\n"), pNANDFConReg->NFECCERR0));
	NAND_ERR((_T("NFECCERR1  = 0x%08x\r\n"), pNANDFConReg->NFECCERR1));
	NAND_ERR((_T("NFMECC0    = 0x%08x\r\n"), pNANDFConReg->NFMECC0));
	NAND_ERR((_T("NFMECC1    = 0x%08x\r\n"), pNANDFConReg->NFMECC1));
	NAND_ERR((_T("NFSECC     = 0x%08x\r\n"), pNANDFConReg->NFSECC));
	NAND_ERR((_T("NFMLCBITPT = 0x%08x\r\n"), pNANDFConReg->NFMLCBITPT));
	NAND_ERR((_T("=================================================\r\n\n")));
}
#endif
