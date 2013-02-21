///////////////////////////////////////////////////////////////
//
//	MODULE		: FIL
//	NAME		: S3C6410X Flash Interface Layer
//	FILE			: S3C6410X_NAND.h
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
//
///////////////////////////////////////////////////////////////

#ifndef __S3C6410_FIL_H__
#define __S3C6410_FIL_H__

#ifndef _FIL_H_
#include <FIL.H>
#endif

////////////////////////////////////////////////
// SFR Bit Field Macro
////////////////////////////////////////////////

// NFCONF
#define NF_NANDBOOT	(0x1<<31)
#define NF_1BIT_ECC		(0x0<<24)
#define NF_4BIT_ECC		(0x1<<24)
#define NF_8BIT_ECC		(0x1<<23)
#define NF_MASK_ECC_BIT_INDICATES   (0x3<<23)
#define NF_8BIT_ECC_STOP_BIT	    (0x1<11)
#define NF_TACLS(n)		(((n)&0x7)<<12)
#define NF_TWRPH0(n)	(((n)&0x7)<<8)
#define NF_TWRPH1(n)	(((n)&0x7)<<4)

// NFCONT
#define NF_4BIT_ECC_DEC		(0x0<<18)
#define NF_4BIT_ECC_ENC		(0x1<<18)
#define NF_LOCK_TIGHT_EN		(0x1<<17)
#define NF_SOFT_LOCK_EN		(0x1<<16)
#define NF_ECC_ENC_INT_EN		(0x1<<13)
#define NF_ECC_DEC_INT_EN		(0x1<<12)
#define NF_ILLACC_INT_EN		(0x1<<10)
#define NF_RNB_INT_EN			(0x1<<9)
#define NF_RNB_DETECT_RISE		(0x0<<8)
#define NF_RNB_DETECT_FALL		(0x1<<8)
#define NF_MAIN_ECC_UNLOCK	(0x0<<7)
#define NF_MAIN_ECC_LOCK		(0x1<<7)
#define NF_SPARE_ECC_UNLOCK	(0x0<<6)
#define NF_SPARE_ECC_LOCK		(0x1<<6)
#define NF_INIT_MECC			(0x1<<5)
#define NF_INIT_SECC			(0x1<<4)
#define NF_NFCE1				(0x1<<2)
#define NF_NFCE0				(0x1<<1)
#define NF_NFCON_DIS			(0x0)
#define NF_NFCON_EN			(0x1)

// NFCCMD
// NFADDR
// NFDATA
// NFMECCD0
// NFMECCD1
// NFSECCD
// NFSBLK
// NFEBLK

// NFSTAT
#define NF_ECC_ENC_DONE	(0x1<<7)
#define NF_ECC_DEC_DONE	(0x1<<6)
#define NF_ILLEGAL_ACCESS	(0x1<<5)
#define NF_RNB_TRANS            (0x1<<4)
#define NF_NFCE1_HI			(0x1<<3)
#define NF_NFCE0_HI			(0x1<<2)
#define NF_RNB_BUSY			(0x0)
#define NF_RNB_READY		(0x1)

// NFECCERR0
// NFECCERR1
// NFMECC0
// NFMECC1
// NFSECC
// NFMLCBITPT

#define NF_8IBTECC_DEC_DONE (0x1<<31)
///////////////////////////////////////////////
// NAND Controller Macro
///////////////////////////////////////////////
#define NF_CE_L(bank)		{	\
								if (bank == 0) pNANDFConReg->NFCONT &= ~NF_NFCE0;	\
								else if (bank == 1) pNANDFConReg->NFCONT &= ~NF_NFCE1;	\
							}
#define NF_CE_H(bank)		{	\
								if (bank == 0) pNANDFConReg->NFCONT |= NF_NFCE0;	\
								else if (bank == 1) pNANDFConReg->NFCONT |= NF_NFCE1;		\
							}
#define NF_CMD(cmd)			(pNANDFConReg->NFCMD = (UINT8)cmd)
#define NF_ADDR(addr)		(pNANDFConReg->NFADDR = (UINT8)addr)
#define NF_DATA_R()			(pNANDFConReg->NFDATA)
#define NF_DATA_R4()		(*(volatile UINT32 *)((UINT32)pNANDFConReg+0x10))
#define NF_DATA_W(data)		(pNANDFConReg->NFDATA = (UINT8)data)
#define NF_DATA_W4(data)	(*(volatile UINT32 *)((UINT32)pNANDFConReg+0x10) = (UINT32)data)
#define NF_SET_ECC_DEC()	(pNANDFConReg->NFCONT &= ~NF_4BIT_ECC_ENC)
#define NF_SET_ECC_ENC()	(pNANDFConReg->NFCONT |= NF_4BIT_ECC_ENC)
#define NF_MECC_Reset()		(pNANDFConReg->NFCONT |= NF_INIT_MECC)
#define NF_MECC_Lock()		(pNANDFConReg->NFCONT |= NF_MAIN_ECC_LOCK)
#define NF_MECC_UnLock()	(pNANDFConReg->NFCONT &= ~NF_MAIN_ECC_LOCK)
#define NF_SECC_Reset()		(pNANDFConReg->NFCONT |= NF_INIT_SECC)
#define NF_SECC_Lock()		(pNANDFConReg->NFCONT |= NF_SPARE_ECC_LOCK)
#define NF_SECC_UnLock()	(pNANDFConReg->NFCONT &= ~NF_SPARE_ECC_LOCK)

#define NF_8BIT_ECC_STOP()	(pNANDFConReg->NFCONT |= NF_8BIT_ECC_STOP_BIT)
#define NF_8BIT_ECC_STOP_CLEAR() (pNANDFConReg->NFCONT &= ~NF_8BIT_ECC_STOP_BIT)
#define NF_MECC0()			(pNANDFConReg->NFMECC0)
#define NF_MECC1()			(pNANDFConReg->NFMECC1)
#define NF_CLEAR_ECC_ENC_DONE()	(pNANDFConReg->NFSTAT |= NF_ECC_ENC_DONE)
#define NF_CLEAR_ECC_DEC_DONE()	(pNANDFConReg->NFSTAT |= NF_ECC_DEC_DONE)
#define NF_WAIT_ECC_ENC_DONE()	{	\
										while(!(pNANDFConReg->NFSTAT&NF_ECC_ENC_DONE));	\
									}

#if 0
#define NF_WAIT_ECC_DEC_DONE()	{	\
										while(!(pNANDFConReg->NFSTAT&NF_ECC_DEC_DONE));	\
									}
#endif
#define NF_WAIT_8BITECC_DEC_DONE()	{	\
										while((pNANDFConReg->NF8ECCERR0&NF_8IBTECC_DEC_DONE));	\
									}

#define NF_ECC_DEC_ERROR()			((pNANDFConReg->NFECCERR0>>26)&0x7)
#define NF_ECC_ERR0()				(pNANDFConReg->NFECCERR0)
#define NF_ECC_ERR1()				(pNANDFConReg->NFECCERR1)
#define NF_ECC_ERR_PATTERN()		(pNANDFConReg->NFMLCBITPT)

#define NF_8MECC0()			(pNANDFConReg->NFM8ECC0)
#define NF_8MECC1()			(pNANDFConReg->NFM8ECC1)
#define NF_8MECC2()			(pNANDFConReg->NFM8ECC2)
#define NF_8MECC3()			(pNANDFConReg->NFM8ECC3)

#define NF_8ECC_ERR0()				(pNANDFConReg->NF8ECCERR0)
#define NF_8ECC_ERR1()				(pNANDFConReg->NF8ECCERR1)
#define NF_8ECC_ERR2()				(pNANDFConReg->NF8ECCERR2)

#define NF_8ECC_ERR_PATTERN0()		(pNANDFConReg->NFMLC8BITPT0)
#define NF_8ECC_ERR_PATTERN1()		(pNANDFConReg->NFMLC8BITPT1)
#define NF_DETECT_RB()			{ while((pNANDFConReg->NFSTAT&0x11)!=0x11);} // RnB_Transdetect & RnB

#if	0	// Busy Check using RnB Pin
#define NF_WAIT_RnB(bank)	{	\
								while(!(pNANDFConReg->NFSTAT&NF_RNB_READY));	\
							}
#else	// Busy Check using I/O[6] Pin (Need Dummy READ Command)
#define NF_WAIT_RnB(bank)	{	\
								NF_CMD(CMD_READ_STATUS);		\
								while(!(NF_DATA_R()&0x40));		\
							}
#endif
#define NF_SET_ADDR(nPpn, nOffset)	{	\
										NF_ADDR(nOffset&0xFF);			\
										NF_ADDR((nOffset>>8)&0xFF);		\
										NF_ADDR(nPpn&0xFF);			\
										NF_ADDR((nPpn>>8)&0xFF);		\
										if (DEV_ADDR_CYCLE > 4)			\
											NF_ADDR((nPpn>>16)&0xFF);	\
									}
#define NF_SET_CLK(tacls, twrph0, twrph1)		(pNANDFConReg->NFCONF = (pNANDFConReg->NFCONF&~0x7770)		\
											|NF_TACLS(tacls) | NF_TWRPH0(twrph0) | NF_TWRPH1(twrph1))

#define NF_WAIT_ECC_READY()	{	\
									while(!(pNANDFConReg->NF8ECCERR0&(1<<30)));	\
								}

#define NF_SETREG_8BITECC() {   \
                        		pNANDFConReg->NFCONF &= ~NF_MASK_ECC_BIT_INDICATES; \
                        		pNANDFConReg->NFCONF |= NF_8BIT_ECC;                \
                        		pNANDFConReg->NFCONT |= NF_8BIT_ECC_STOP_BIT;       \
                            }

#define NF_SETREG_4BITECC() {   \
                        		pNANDFConReg->NFCONF &= ~NF_MASK_ECC_BIT_INDICATES; \
                        		pNANDFConReg->NFCONF |= NF_4BIT_ECC;                \
                            }

///////////////////////////////////////////////
// NAND Flash Command Set
///////////////////////////////////////////////
#define CMD_READ_ID							(0x90)
#define CMD_READ								(0x00)
#define CMD_READ_CONFIRM						(0x30)
#define CMD_RESET								(0xFF)
#define CMD_PROGRAM							(0x80)
#define CMD_PROGRAM_CONFIRM					(0x10)
#define CMD_ERASE								(0x60)
#define CMD_ERASE_CONFIRM						(0xD0)
#define CMD_RANDOM_DATA_INPUT				(0x85)
#define CMD_RANDOM_DATA_OUTPUT				(0x05)
#define CMD_RANDOM_DATA_OUTPUT_CONFIRM		(0xE0)
#define CMD_READ_STATUS						(0x70)
#define CMD_READ_STATUS_CHIP0					(0xF1)
#define CMD_READ_STATUS_CHIP1					(0xF2)

// 2 Plane Program Sequence : 80h - 11h - 81h - 10h (Write CMD - 2Plane Dummy - 2 Plane Prog - Write Confirm)
#define CMD_2PLANE_PROGRAM					(0x81)
#define CMD_2PLANE_PROGRAM_DUMMY			(0x11)

// What is this CMD???
#define CMD_READ_FOR_COPY_BACK				(0x35)

#define NAND_STATUS_READY						(0x40)
#define NAND_STATUS_ERROR						(0x01)
#define NAND_STATUS_PLANE0_ERROR			(0x02)
#define NAND_STATUS_PLANE1_ERROR			(0x04)

///////////////////////////////////////////////
// NAND configuration definitions
///////////////////////////////////////////////
#define NAND_SPAREPAGE_SIZE			(256) // for supporting 8Bit-ECC 080926 hsjang

///////////////////////////////////////////////
// Main Area Layout (512 bytes x 4)
///////////////////////////////////////////////
// +----------+----------+----------+----------+
// | 512B     | 512B     | 512B     | 512B     |
// | Sector 0 | Sector 1 | Sector 2 | Sector 3 |
// +----------+----------+----------+----------+

///////////////////////////////////////////////
// Main Area Layout (512 bytes x 8) : 4KBytes/Page
///////////////////////////////////////////////
// +----------+----------+----------+----------+----------+----------+----------+----------+
// | 512B     | 512B     | 512B     | 512B     | 512B     | 512B     | 512B     | 512B     |
// | Sector 0 | Sector 1 | Sector 2 | Sector 3 | Sector 4 | Sector 5 | Sector 6 | Sector 7 |
// +----------+----------+----------+----------+----------+----------+----------+----------+
#define NAND_SECTOR_SIZE				(512)

///////////////////////////////////////////////
// Spare Area Layout (64 bytes) for 2KByte/Page
///////////////////////////////////////////////
// +-----+-------+----------+--------------+----------+----------+----------+----------+-----------+-----------+
// | 1B  | 1B    | 2B       | 12B          | 8B       | 8B       | 8B       | 8B       | 8B        | 8B (copy) |
// | Bad | Clean | Reserved | SpareContext | Sec0 ECC | Sec1 ECC | Sec2 ECC | Sec3 ECC | Spare ECC | Spare ECC |
// +-----+-------+----------+--------------+----------+----------+----------+----------+-----------+-----------+

///////////////////////////////////////////////
// Spare Area Layout (64 bytes) for 4KByte/Page
///////////////////////////////////////////////
// +-----+-------+----------+--------------+----------+----------+----------+----------+----------+----------+----------+----------+-----------+-----------+
// | 1B  | 1B    | 2B       | 20B          | 8B       | 8B       | 8B       | 8B       | 8B       | 8B       | 8B       | 8B       | 8B        | 8B (copy) |
// | Bad | Clean | Reserved | SpareContext | Sec0 ECC | Sec1 ECC | Sec2 ECC | Sec3 ECC | Sec4 ECC | Sec5 ECC | Sec6 ECC | Sec7 ECC | Spare ECC | Spare ECC |
// +-----+-------+----------+--------------+----------+----------+----------+----------+----------+----------+----------+----------+-----------+-----------+

///////////////////////////////////////////////
// Spare Area Layout (128 bytes) for 4KByte/Page : Read_Spare_Separate ( Total 120 Bytes are used )
///////////////////////////////////////////////
// +-----+-------+----------+--------------+----------+----------+----------+----------+----------+----------+----------+----------+------------------+--------------------+
// | 1B  | 1B    | 2B       | 20B          | 8B       | 8B       | 8B       | 8B       | 8B       | 8B       | 8B       | 8B       | 16B (8bit ECC)   | 16B (8bit ECC)     |
// | Bad | Clean | Reserved | SpareContext | Sec0 ECC | Sec1 ECC | Sec2 ECC | Sec3 ECC | Sec4 ECC | Sec5 ECC | Sec6 ECC | Sec7 ECC | SpareContext ECC | ECC of Sec0 ~ Sec7 |
// +-----+-------+----------+--------------+----------+----------+----------+----------+----------+----------+----------+----------+------------------+--------------------+

///////////////////////////////////////////////
// Spare Area Layout (218 bytes) for 4KByte/Page : Read_Spare_8BitECC ( Total 184 Bytes are used )
///////////////////////////////////////////////
// +-----+-------+----------+--------------+----------+----------+----------+----------+----------+----------+----------+----------+-----------+-----------+
// | 1B  | 1B    | 2B       | 20B          | 16B      | 16B      | 16B      | 16B      | 16B      | 16B      | 16B      | 16B      | 16B       | 16B(copy) |
// | Bad | Clean | Reserved | SpareContext | Sec0 ECC | Sec1 ECC | Sec2 ECC | Sec3 ECC | Sec4 ECC | Sec5 ECC | Sec6 ECC | Sec7 ECC | Spare ECC | Spare ECC |
// +-----+-------+----------+--------------+----------+----------+----------+----------+----------+----------+----------+----------+-----------+-----------+

///////////////////////////////////////////////
// ECC Spare context definitions
///////////////////////////////////////////////
#define NAND_SCXT_OFFSET	(4)		// 1B+1B+2B
#define NAND_MECC_OFFSET	(16)		// 1B+1B+2B+12B
#define NAND_SECC_OFFSET	(48)		// 1B+1B+2B+12B+32B
#define NAND_SECC2_OFFSET	(56)		// 1B+1B+2B+12B+32B+8B
// for 4KByte/Page
#define NAND_MECC_OFFSET_4K	(24)		// 1B+1B+2B+20B
#define NAND_SECC_OFFSET_4K	(88)		// 1B+1B+2B+20B+64B
#define NAND_SECC_OFFSET_8BIT_ECC_4K	(152)		// 1B+1B+2B+20B+128B
#define NAND_SECC2_OFFSET_4K	(96)		// 1B+1B+2B+20B+64B+8B
#define NAND_SECC2_OFFSET_8BIT_ECC_4K	(168)		// 1B+1B+2B+20B+128B+16B

typedef struct {
	UINT8	cBadMark;			// 1 bytes bad mark
	UINT8	cCleanMark;			// 1 Byte clean mark
	UINT8	cReserved[2];		// 2 byte Reserved
	INT32	aSpareData[5];		// 20 bytes spare data, use only 12 byte for 2KByte/Page
	UINT32	aMECC[8*4];			// 32 bytes ECC for Sec0~Sec3 in Main Area, 
	                            // 64 bytes ECC for Sec4~Sec7 in Main Area for 4K page, 
	                            // 128 byte for sec0~7 8Bit ECC data
	UINT32	aSECC[8];			// 8 bytes ECC x 2 for Spare Area
} SECCCxt, *pSECCCxt;

// Default NAND Flash timing @HCLK 133MHz (tHCLK = 7.5ns)
//#define	 DEFAULT_TACLS		(1)	// 1 HCLK (7.5ns)
//#define	 DEFAULT_TWRPH0	(4)	// 5 HCLK (37.5ns)
//#define	 DEFAULT_TWRPH1	(1)	// 2 HCLK (15ns)

#define	 DEFAULT_TACLS	(1)//(1)	// 1 HCLK (7.5ns) 777
#define	 DEFAULT_TWRPH0	(6)//(4)	// 5 HCLK (37.5ns)
#define	 DEFAULT_TWRPH1	(1)//(1)	// 2 HCLK (15ns)
#define	 MAX_RW_TACLS	(7)
#define	 MAX_RW_TWRPH0	(7)
#define	 MAX_RW_TWRPH1	(7)

// for ECC decoding with Dummy Data
#define	 DUMMY_R_TACLS	(0)
#define	 DUMMY_R_TWRPH0	(1)
#define	 DUMMY_R_TWRPH1	(1)

// for ECC encoding with Dummy Data
#define	 DUMMY_W_TACLS	(0)
#define	 DUMMY_W_TWRPH0	(0)
#define	 DUMMY_W_TWRPH1	(0)

///////////////////////////////////////////////
// Exported Function Prototype of FIL
///////////////////////////////////////////////

#ifdef __cplusplus
	extern "C" {
#endif

INT32 NAND_Init(VOID);
VOID  NAND_Reset (UINT32 nBank);
INT32 NAND_Read(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap, UINT32 nPlaneBitmap,
				UINT8* pDBuf, UINT8* pSBuf, BOOL32 bECCIn, BOOL32 bCleanCheck);
INT32 NAND_Read_Retry(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap, UINT32 nPlaneBitmap,
				UINT8* pDBuf, UINT8* pSBuf, BOOL32 bECCIn, BOOL32 bCleanCheck);
VOID NAND_Write(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap,
				UINT32 nPlaneBitmap, UINT8* pDBuf, UINT8* pSBuf);
VOID NAND_Write_AfterCheck(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap,
				UINT32 nPlaneBitmap, UINT8* pDBuf, UINT8* pSBuf);
VOID  NAND_Erase (UINT32 nBank, UINT32 nPbn, UINT32 nPlaneBitmap);
INT32 NAND_Sync (UINT32 nBank, UINT32 *nPlaneBitmap);
VOID NAND_Steploader_Write(UINT32 nBank, UINT32 nPpn, UINT32 nSctBitmap,
				UINT32 nPlaneBitmap, UINT8* pDBuf, UINT8* pSBuf);
VOID NAND_GetPlatformInfo(FILPlatformInfo* pstFILPlatformInfo);
#ifdef __cplusplus
}
#endif

#endif	// __S3C6410_FIL_H__
