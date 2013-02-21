#ifndef __NAND_H__
#define __NAND_H__

////////////////////////////////////////////////
// NAND Flash Controller SFR Bit Field Macro
////////////////////////////////////////////////

// NFCONF
#define NF_NANDBOOT	(0x1<<31)
#define NF_1BIT_ECC		(0x0<<23)
#define NF_4BIT_ECC		(0x2<<23)
#define NF_8BIT_ECC		(0x1<<23)
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
#define CMD_READ_STATUS_CE0					(0xF1)
#define CMD_READ_STATUS_CE1					(0xF2)

// 2 Plane Program Sequence : 80h - 11h - 81h - 10h (Write CMD - 2Plane Dummy - 2 Plane Prog - Write Confirm)
#define CMD_2PLANE_PROGRAM					(0x81)
#define CMD_2PLANE_PROGRAM_DUMMY			(0x11)

// What is this CMD???
#define CMD_READ_FOR_COPY_BACK				(0x35)

#define NAND_STATUS_READY						(0x40)
#define NAND_STATUS_ERROR						(0x01)

#define NAND_MAINPAGE_SIZE			(2048)
#define NAND_SPAREPAGE_SIZE			(64)
#define NAND_SECTOR_SIZE				(512)

///////////////////////////////////////////////
// Spare Area Layout (64 bytes)
///////////////////////////////////////////////
// +-----+-------+----------+--------------+----------+----------+----------+----------+-----------+-----------+
// | 1B  | 1B    | 2B       | 12B          | 8B       | 8B       | 8B       | 8B       | 8B        | 8B (copy) |
// | Bad | Clean | Reserved | SpareContext | Sec0 ECC | Sec1 ECC | Sec2 ECC | Sec3 ECC | Spare ECC | Spare ECC |
// +-----+-------+----------+--------------+----------+----------+----------+----------+-----------+-----------+

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
#define NAND_SECC2_OFFSET_4K	(96)		// 1B+1B+2B+20B+64B+8B

// Default NAND Flash timing @HCLK 133MHz (tHCLK = 7.5ns)
#define	 DEFAULT_TACLS		(1)	// 1 HCLK (7.5ns)
//#define	 DEFAULT_TWRPH0	(3)	// 4 HCLK (30ns)
#define	 DEFAULT_TWRPH0	(6)	// 5 HCLK (37.5ns)
#define	 DEFAULT_TWRPH1	(1)	// 2 HCLK (15ns)

// for ECC decoding with Dummy Data
#define	 DUMMY_R_TACLS	(0)
#define	 DUMMY_R_TWRPH0	(1)
#define	 DUMMY_R_TWRPH1	(1)

// for ECC encoding with Dummy Data
#define	 DUMMY_W_TACLS	(0)
#define	 DUMMY_W_TWRPH0	(0)
#define	 DUMMY_W_TWRPH1	(0)


#ifdef __cplusplus
	extern "C" {
#endif

void NAND_Init(void);
void NAND_Reset(DWORD dwBank);
BOOL NAND_Read(DWORD dwBank, DWORD dwPage, unsigned char *pBuf, BOOL b4KPage);
BOOL Read_Spare(DWORD dwBank, DWORD dwPage, DWORD *pSpareCxt, UINT8 bSecondTry);
BOOL Decoding_ECC(unsigned char* pBuf, DWORD nValidBufLength);
void Read_DeviceID(DWORD dwBank, unsigned char *pDID, unsigned char *pHID);

#ifdef __cplusplus
}
#endif

#endif // __NAND_H__
