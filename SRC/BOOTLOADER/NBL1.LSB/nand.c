#include <windows.h>
#include "s3c6410_addr.h"
#include "utils.h"
#include "nand.h"

#define DEV_ADDR_CYCLE (5)

///////////////////////////////////////////////
// NAND Controller Macro
///////////////////////////////////////////////
#define NF_CE_L(bank)		{	\
								if (bank == 0) rNFCONT &= ~NF_NFCE0;	\
								else if (bank == 1) rNFCONT &= ~NF_NFCE1;	\
							}
#define NF_CE_H(bank)		{	\
								if (bank == 0) rNFCONT |= NF_NFCE0;	\
								else if (bank == 1) rNFCONT |= NF_NFCE1;		\
							}
#define NF_CMD(cmd)			(rNFCMD = (unsigned char)cmd)
#define NF_ADDR(addr)		(rNFADDR = (unsigned char)addr)
#define NF_DATA_R()			(rNFDATA8)
#define NF_DATA_R4()		(rNFDATA32)
#define NF_DATA_W4(n)		(rNFDATA32 = (DWORD)n)
#define NF_SET_ECC_DEC()	(rNFCONT &= ~NF_4BIT_ECC_ENC)
#define NF_MECC_Reset()		(rNFCONT |= NF_INIT_MECC)
#define NF_MECC_Lock()		(rNFCONT |= NF_MAIN_ECC_LOCK)
#define NF_MECC_UnLock()	(rNFCONT &= ~NF_MAIN_ECC_LOCK)
#define NF_CLEAR_ECC_DEC_DONE()	(rNFSTAT |= NF_ECC_DEC_DONE)
#define NF_WAIT_ECC_DEC_DONE()	{	\
										while(!(rNFSTAT&NF_ECC_DEC_DONE));	\
									}

#define NF_WRMECCD0(data)			{rNFMECCD0 = (data);}
#define NF_WRMECCD1(data)			{rNFMECCD1 = (data);}
#define NF_ECC_DEC_ERROR()			((rNFECCERR0>>26)&0x7)
#define NF_ECC_ERR0()				(rNFECCERR0)
#define NF_ECC_ERR1()				(rNFECCERR1)
#define NF_ECC_ERR_PATTERN()		(rNFMLCBITPT)

#define NF_DETECT_RB()	{	\
								while(!(rNFSTAT&NF_RNB_READY));	\
							}

#define NF_WAIT_RnB(bank)	{	\
								NF_CMD(CMD_READ_STATUS);		\
								while(!(NF_DATA_R()&0x40));			\
							}

#define NF_SET_ADDR(nPpn, nOffset)	{	\
										NF_ADDR(nOffset&0xFF);			\
										NF_ADDR((nOffset>>8)&0xFF);		\
										NF_ADDR(nPpn&0xFF);			\
										NF_ADDR((nPpn>>8)&0xFF);		\
										if (DEV_ADDR_CYCLE > 4)			\
											NF_ADDR((nPpn>>16)&0xFF);	\
									}

#define NF_SET_CLK(tacls, twrph0, twrph1)		(rNFCONF = (rNFCONF&~0x7770)		\
											|NF_TACLS(tacls) | NF_TWRPH0(twrph0) | NF_TWRPH1(twrph1))


///////////////////////////////////////////////
// Assembly Read Function (in nand.s)
///////////////////////////////////////////////
void _Read_512Byte(unsigned char* pBuf);
void _Write_Dummy_Byte_AllFF(int nByteSize);  // nByteSize is the multifilication of 4
int CorrectECC8Data(unsigned char *pEncodingBaseAddr);


void NAND_Init(void)
{

	// Initialize NAND Flash Controller for MLC NAND Flash
	rNFCONF = NF_8BIT_ECC | NF_TACLS(DEFAULT_TACLS) | NF_TWRPH0(DEFAULT_TWRPH0) | NF_TWRPH1(DEFAULT_TWRPH1);
	rNFCONT = NF_MAIN_ECC_LOCK | NF_SPARE_ECC_LOCK | NF_INIT_MECC | NF_INIT_SECC | NF_NFCE1 | NF_NFCE0 | NF_NFCON_EN;
	rNFSTAT = NF_RNB_READY;	// Clear RnB Transition Detect Bit

	rNFCONF = rNFCONF & ~(1<<30);
	rNFCONT |= (1<<18)|(1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9); //ECC for programming.// Enable RnB Interrupt 
	rNFSTAT |= ((1<<6)|(1<<5)|(1<<4));

	NAND_Reset(0);
}

void NAND_Reset(DWORD dwBank)
{
	// Chip Select
	NF_CE_L(dwBank);

	// Reset Command is accepted during Busy
	NF_CMD(CMD_RESET);

	// Chip Unselect
	NF_CE_H(dwBank);
}

BOOL NAND_Read(DWORD dwBank, DWORD dwPage, unsigned char *pBuf, BOOL b4KPage)
{
	DWORD SpareDataDummy;
	DWORD dwOffset;
	DWORD dwCnt;
	DWORD dwSpareAddress = 2048 + 1;	//include Bad block

	unsigned char uSctCnt;
	BOOL bRet = TRUE;

	if (b4KPage == TRUE) uSctCnt = 8;
	else uSctCnt = 4;

	rNFCONF = ( rNFCONF&~(0x3<<23) ) |(2<<30)|(1<<23)|(0x7<<12)|(0x7<<8)|(0x7<<4);
	rNFCONT = ( rNFCONT&~(0x1<<18) ) |(0<<18)|(0<<11)|(0<<10)|(0<<9)|(1<<6)|(1<<0); // Init NFCONT	
	rNFSTAT |= ((1<<6)|(1<<5)|(1<<4));

	NAND_Reset(dwBank);
	
	NF_MECC_Lock(); // Main ECC Lock
	NF_CE_L(dwBank);
	rNFSTAT |= (1<<4); // RnB Clear


	NF_CMD(CMD_READ);
	NF_SET_ADDR(dwPage, 0x00);
	NF_CMD(CMD_READ_CONFIRM);
	NF_DETECT_RB();
	rNFSTAT |= (1<<4); // RnB Clear

	//READ Spare ECC Data
	for(dwCnt = 0; dwCnt < uSctCnt; dwCnt++) {
		NF_MECC_Lock(); // Main ECC Lock
		rNFSTAT |= (1<<4); // RnB Clear

		if(!dwCnt) {
			NF_CMD(CMD_READ);
			NF_SET_ADDR(dwPage, 0);
			NF_CMD(CMD_READ_CONFIRM);

			NF_DETECT_RB();
			rNFSTAT |= (1<<4); // RnB Clear			
		}
		else
		{
			dwOffset = dwCnt * 512;
			NF_CMD(CMD_RANDOM_DATA_OUTPUT);
			NF_ADDR(dwOffset&0xFF);
			NF_ADDR((dwOffset>>8)&0xFF);
			NF_CMD(CMD_RANDOM_DATA_OUTPUT_CONFIRM);
		}

		NF_MECC_UnLock(); 	// Main ECC Unlock
		NF_MECC_Reset();	 // Initialize ECC
 
		//read 512byte
		_Read_512Byte(pBuf + 512*dwCnt);

                if (b4KPage == TRUE)
                        dwOffset = 4096 + (dwCnt * 13);
                else
                        dwOffset = 2048 + (dwCnt * 13);

		NF_CMD(CMD_RANDOM_DATA_OUTPUT);
		NF_ADDR(dwOffset&0xFF);
		NF_ADDR((dwOffset>>8)&0xFF);
		NF_CMD(CMD_RANDOM_DATA_OUTPUT_CONFIRM);

		//read Spare ECC Data		
		SpareDataDummy = NF_DATA_R4();
		SpareDataDummy = NF_DATA_R4();
		SpareDataDummy = NF_DATA_R4();
		SpareDataDummy = NF_DATA_R();

		NF_MECC_Lock(); // Main ECC Lock

		while(!(rNFSTAT&(1<<6))); 	// Check decoding done 
		rNFSTAT = rNFSTAT | (1<<6);   // Decoding done Clear

		while( rNF8ECCERR0 & (unsigned int)(1<<31) ) ; // 8bit ECC Decoding Busy Check.

		if(CorrectECC8Data((pBuf + 512*dwCnt))) {
			return FALSE;

		}		
	}

	NF_CE_H(dwBank);
	
	return TRUE;
}




int CorrectECC8Data(unsigned char *pEncodingBaseAddr)
{	
	unsigned int i,uErrorByte[9];
	unsigned char uErrorBit[9];
	unsigned int uErrorType;
	
	
	uErrorType = (rNF8ECCERR0>>25)&0xf;// Searching Error Type //How many Error bits does exist?	
	uErrorByte[1] = rNF8ECCERR0&0x3ff;// Searching Error Byte //Where is the error byte?
	uErrorByte[2] = (rNF8ECCERR0>>15)&0x3ff;	
	uErrorByte[3] = (rNF8ECCERR1)&0x3ff;
	uErrorByte[4] = (rNF8ECCERR1>>11)&0x3ff;	
	uErrorByte[5] = (rNF8ECCERR1>>22)&0x3ff;	
	uErrorByte[6] = (rNF8ECCERR2)&0x3ff;
	uErrorByte[7] = (rNF8ECCERR2>>11)&0x3ff;
	uErrorByte[8] = (rNF8ECCERR2>>22)&0x3ff;
	
	uErrorBit[1] = rNFMLC8BITPT0&0xff;// Searching Error Bit //Where is the error bit?
	uErrorBit[2] = (rNFMLC8BITPT0>>8)&0xff;
	uErrorBit[3] = (rNFMLC8BITPT0>>16)&0xff;
	uErrorBit[4] = (rNFMLC8BITPT0>>24)&0xff;	
	uErrorBit[5] = rNFMLC8BITPT1&0xff;
	uErrorBit[6] = (rNFMLC8BITPT1>>8)&0xff;
	uErrorBit[7] = (rNFMLC8BITPT1>>16)&0xff;
	uErrorBit[8] = (rNFMLC8BITPT1>>24)&0xff;
	
	if(uErrorType == 0x0) {
		return 0;
	}

	if(uErrorType == 0x9) {
		return 1;
	}
	for(i=1;i<=uErrorType ;i++)	
	{
		if(uErrorByte[i] < 512)	
			pEncodingBaseAddr[uErrorByte[i]]^=uErrorBit[i];
		else
		{;	}			
	}
	return 0;
	
}



void Read_DeviceID(DWORD dwBank, unsigned char *pDID, unsigned char *pHID)
{
	unsigned char ucMID, ucDID, ucHID[3];
	int i;

	// Chip Select
	NF_CE_L(dwBank);
	NF_WAIT_RnB(dwBank);

	// Read ID Command
	NF_CMD(CMD_READ_ID);
	NF_ADDR(0x00);

	// Find Maker Code
	for (i=0; i<10; i++)
	{
		ucMID = NF_DATA_R();		// Maker Code
		if (ucMID == 0xEC) break;
	}

	// Read Device Code
	ucDID = NF_DATA_R();		// Device Code
	ucHID[0] = NF_DATA_R();		// Internal Chip Number
	ucHID[1] = NF_DATA_R();		// Page, Block, Redundant Area Size
	ucHID[2] = NF_DATA_R();		// Plane Number, Size

	// Chip Unselect
	NF_CE_H(dwBank);

	if (ucMID == 0xEC)
	{
		*pDID = ucDID;
		*pHID = ucHID[0];
	}
	else
	{
		*pDID = 0x00;
		*pHID = 0x00;
	}
}

