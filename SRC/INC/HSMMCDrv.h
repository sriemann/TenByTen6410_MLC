//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#ifndef __HS_MMC_H__
#define __HS_MMC_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _MMC_PARSED_REGISTER_EXTCSD
{    // structure of Extended CSD register
    UCHAR Reserved180[181];

    UCHAR ErasedMemCont;    // initial value after erase operation
    UCHAR Reserved182;
    UCHAR BusWidth;    // bus width
    UCHAR Reserved184;

    UCHAR HSTiming;    // high speed timing

    UCHAR Reserved186;
    UCHAR PowClass;    // power class
    UCHAR Reserved188;
    UCHAR CmdSetRev;    // command set revision

    UCHAR Reserved190;

    UCHAR CmdSet;    // contains a binary code of the command set
    UCHAR EXTCSDRev;    // Extended CSD revision
    UCHAR Reserved193;
    UCHAR CSDStruct;    // CSD structure field in CSD register

    UCHAR Reserved195;

    UCHAR CardType;    // MMC card type
    UCHAR Reserved197[3];
    UCHAR PwrCl52195;
    UCHAR PwrCl26195;

    UCHAR PwrCl52360;

    UCHAR PwrCl26360;    // supported power class by the card
    UCHAR Reserved204;
    UCHAR MinPerfR0426;    // min. read performance with 4 bit bus width & 26MHz
    UCHAR MinPerfW0426;    // min. write performance with 4 bit bus width & 26MHz

    UCHAR MinPerfR08260452;  // min. read performance with 8 bit bus width & 26MHz

          // min. read performance with 4 bit bus width & 52MHz
    UCHAR MinPerfW08260452;  // min. write performance with 8 bit bus width & 26MHz
            // min. write performance with 4 bit bus width & 26MHz
    UCHAR MinPerfR0852;    // min. read performance with 8 bit bus width & 52MHz

    UCHAR MinPerfW0852;    // min. write performance with 8 bit bus width & 52MHz

    UCHAR Reserved211;
    ULONG Sec_Count;    // sector count
    UCHAR Reserved216[288];
    UCHAR sCmdSet;    // command sets are supported by the card

    UCHAR Reserved505[7];

} MMC_PARSED_REGISTER_EXTCSD, *PMMC_PARSED_REGISTER_EXTCSD;

//////////////////////////////////////////////////////////////////////////////////////////////////

UINT32 SDHC_READ(UINT32 dwStartSector, UINT32 dwSector, UINT32 dwAddr);
UINT32 SDHC_READ_DMA(UINT32 dwStartSector, UINT32 dwSector, UINT32 dwAddr);
UINT32 SDHC_WRITE(UINT32 dwStartSector, UINT32 dwSector, UINT32 dwAddr);
UINT32 SDHC_WRITE_DMA(UINT32 dwStartSector, UINT32 dwSector, UINT32 dwAddr);

BOOL SDHC_INIT(void);
UINT32 SDHC_GET_EXTCSD(void);
UINT32 SDHC_SET_PREDEFINE(UINT32 dwSector);

UINT32 SDHC_CLK_ON_OFF(UINT32 OnOff);
UINT32 SDHC_CONFIG_CLK(UINT32 Divisior);

UINT32 SDHC_WAIT_CMD_COMPLETE(void);
UINT32 SDHC_WAIT_TRANS_COMPLETE(void);
UINT32 SDHC_WAIT_WRITE_READY(void);

UINT32 SDHC_CLEAR_WRITE_READY(void);
UINT32 SDHC_CLEAR_READ_READY(void);
UINT32 SDHC_CLEAR_CMD_COMPLETE(void);
UINT32 SDHC_CLEAR_TRANS_COMPLETE(void);
UINT32 SDHC_CLEAR_INTR_STS(void);
UINT32 SDHC_WAIT_READ_READY(void);
void SDHC_SET_TRANS_MODE(UINT32 MultiBlk,UINT32 DataDirection, UINT32 AutoCmd12En,UINT32 BlockCntEn,UINT32 DmaEn);
void SDHC_SET_ARG(UINT32 uArg);
void SDHC_SET_BLOCK_COUNT(UINT16 uBlkCnt);
void SDHC_SET_SYSTEM_ADDR(UINT32 SysAddr);
void SDHC_SET_BLOCK_SIZE(UINT16 uDmaBufBoundary, UINT16 uBlkSize);
void SDHC_SET_SW_RESET(UINT8 bReset);
UINT32 SDHC_SET_MMC_SPEED(UINT32 eSDSpeedMode);

void SDHC_SET_CMD(UINT16 uCmd,UINT32 uIsAcmd);
UINT32 SDHC_SET_CLK(UINT16 Divisor);
UINT32 SDHC_SET_BUS_WIDTH(UINT32 dwBusWidth);

UINT32 SDHC_CHECK_TRASN_STATE(void);
UINT32 SDHC_ISSUE_CMD( UINT16 uCmd, UINT32 uArg, UINT32 uIsAcmd);
UINT32 SDHC_ENABLE_INTR_STS(UINT16 NormalIntEn, UINT16 ErrorIntEn);
void SDHC_SET_HOST_SPEED(UINT8 SpeedMode);

void SDHC_GET_CARD_SIZE(void);

void SDHC_GPIO_INIT(void);
void SDHC_SDMMC_CONTROLLER_INIT(UINT32 dwClksrc);
void SDHC_CLK_INIT();
UINT32 SDHC_MMC_INIT();
UINT32 SDHC_SD_INIT();
UINT32 SDHC_OCR_CHECK(void);
UINT32 SDHC_SD_SPEED_CHECK(void);
UINT32 SDHC_GET_SCR(void);

extern UINT32 g_dwSectorCount; // it stores a whole sector of the SD/MMC card.

#define    SD_HCLK    1
#define    SD_EPLL        2
#define    SD_EXTCLK    3

#define    NORMAL    0
#define    HIGH    1

//Normal Interrupt Signal Enable
#define    READWAIT_SIG_INT_EN                (1<<10)
#define    CARD_SIG_INT_EN                    (1<<8)
#define    CARD_REMOVAL_SIG_INT_EN            (1<<7)
#define    CARD_INSERT_SIG_INT_EN            (1<<6)
#define    BUFFER_READREADY_SIG_INT_EN        (1<<5)
#define    BUFFER_WRITEREADY_SIG_INT_EN    (1<<4)
#define    DMA_SIG_INT_EN                    (1<<3)
#define    BLOCKGAP_EVENT_SIG_INT_EN        (1<<2)
#define    TRANSFERCOMPLETE_SIG_INT_EN        (1<<1)
#define    COMMANDCOMPLETE_SIG_INT_EN        (1<<0)

//Normal Interrupt Status Enable
#define    READWAIT_STS_INT_EN                (1<<10)
#define    CARD_STS_INT_EN                    (1<<8)
#define    CARD_REMOVAL_STS_INT_EN            (1<<7)
#define    CARD_INSERT_STS_INT_EN            (1<<6)
#define    BUFFER_READREADY_STS_INT_EN        (1<<5)
#define    BUFFER_WRITEREADY_STS_INT_EN    (1<<4)
#define    DMA_STS_INT_EN                    (1<<3)
#define    BLOCKGAP_EVENT_STS_INT_EN        (1<<2)
#define    TRANSFERCOMPLETE_STS_INT_EN        (1<<1)
#define    COMMANDCOMPLETE_STS_INT_EN        (1<<0)



#ifdef __cplusplus
}
#endif
#endif /*__HS_MMC_H__*/

