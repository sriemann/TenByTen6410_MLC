/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2001  Microsoft Corporation

Module Name:    FMD_LB.H

Abstract:        FLASH Media Driver Interface Samsung S3C6410 CPU with NAND Flash
                controller.

Environment:    As noted, this media driver works on behalf of the FAL to directly
                access the underlying FLASH hardware.  Consquently, this module
                needs to be linked with FLASHFAL.LIB to produce the device driver
                named FLASHDRV.DLL.

-----------------------------------------------------------------------------*/
#ifndef _S3C6410_FMD_LB_
#define _S3C6410_FMD_LB_

#define LB_NAND_LOG_2_PAGES_PER_BLOCK    (6)        // Used to avoid multiplications
#define LB_NEED_EXT_ADDR                    (TRUE)    // 5th Address Cycle
#define LB_POS_BADBLOCK                    (0x00)
#define LB_POS_OEMRESERVED                (0x05)

///////////////////////////////////////////////
// NAND configuration definitions
///////////////////////////////////////////////
#define MAX_NAND_SPAREPAGE_SIZE             (128)   // 4Kpagesize -> 128byte, 2Kpagesize -> 64Byte

///////////////////////////////////////////////
// Main Area Layout (512 bytes x 4) -> 2Kpagesize
///////////////////////////////////////////////
// +----------+----------+----------+----------+
// | 512B     | 512B     | 512B     | 512B     |
// | Sector 0 | Sector 1 | Sector 2 | Sector 3 |
// +----------+----------+----------+----------+

///////////////////////////////////////////////
// Spare Area Layout (64 bytes) for 2KByte/Page
///////////////////////////////////////////////
// +-----------+-------------+--------------+------------+----------+----------+----------+----------+----------------------+--------------------+
// | 1Byte     | 4Bytes      | 1Byte        | 2Bytes     | 4Bytes   | 4Bytes   | 4Bytes   | 4Bytes   | 2Bytes               | 2Bytes             |
// | bBadBlock | dwReserved1 | bOEMReserved | wReserved2 | Sec0 ECC | Sec1 ECC | Sec2 ECC | Sec3 ECC | SectorInfo Spare ECC | Main ECC Spare ECC |
// +-----------+-------------+--------------+------------+----------+----------+----------+----------+----------------------+--------------------+

///////////////////////////////////////////////
// ECC Spare context definitions
///////////////////////////////////////////////
#define NAND_MECC_OFFSET    (8)     // 1B+4B+1B+2B
#define NAND_SECC1_OFFSET   (24)    // 1B+4B+1B+2B+4B+4B+4B+4B
#define NAND_SECC2_OFFSET   (26)    // 1B+4B+1B+2B+4B+4B+4B+4B+2B

#ifdef __cplusplus
extern "C" {
#endif
BOOL NAND_LB_ReadSectorInfo(SECTOR_ADDR sectorAddr, PSectorInfo pInfo);
BOOL NAND_LB_WriteSectorInfo(SECTOR_ADDR sectorAddr, PSectorInfo pInfo);
BOOL FMD_LB_ReadSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors);
BOOL RAW_LB_ReadSector(UINT32 startSectorAddr, LPBYTE pSectorBuff, LPBYTE pSectorInfoBuff);
BOOL NAND_LB_IsBlockBad(BLOCK_ID blockID);
BOOL NAND_LB_IsSectorBad(SECTOR_ADDR sectorID);
BOOL NAND_LB_MarkBlockBad(BLOCK_ID blockID);
BOOL NAND_LB_MarkSectorBad(SECTOR_ADDR sectorID);
DWORD FMD_LB_GetBlockStatus(BLOCK_ID blockID);
DWORD FMD_LB_GetSectorStatus(SECTOR_ADDR sectorAddr);
BOOL FMD_LB_EraseBlock(BLOCK_ID blockID);
BOOL FMD_LB_EraseSector(DWORD dwPageID);
BOOL FMD_LB_WriteSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors);
BOOL FMD_LB_SetBlockStatus(BLOCK_ID blockID, DWORD dwStatus);
BOOL FMD_LB_SetSectorStatus(SECTOR_ADDR sectorID, DWORD dwStatus);
BOOL FMD_LB_GetOEMReservedByte(SECTOR_ADDR physicalSectorAddr, PBYTE pOEMReserved);
BOOL FMD_LB_SetOEMReservedByte(SECTOR_ADDR physicalSectorAddr, BYTE bOEMReserved);

#ifdef _IROMBOOT_
BOOL FMD_LB_WriteSector_Steploader(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors);
#endif
#ifdef __cplusplus
}
#endif

#endif _S3C6410_FMD_LB_
