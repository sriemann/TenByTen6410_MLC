/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2001  Microsoft Corporation

Module Name:    S3C6410.H

Abstract:        FLASH Media Driver Interface Samsung S3C6410 CPU with NAND Flash
                controller.

Environment:    As noted, this media driver works on behalf of the FAL to directly
                access the underlying FLASH hardware.  Consquently, this module
                needs to be linked with FLASHFAL.LIB to produce the device driver
                named FLASHDRV.DLL.

-----------------------------------------------------------------------------*/
#ifndef _S3C6410_FMD_SB_
#define _S3C6410_FMD_SB_

#define SB_NAND_LOG_2_PAGES_PER_BLOCK    (5)        // Used to avoid multiplications
#define SB_NEED_EXT_ADDR                    (TRUE)    // 5th Address Cycle
#define SB_POS_BADBLOCK                    (0x05)
#define SB_POS_OEMRESERVED                (0x04)

#ifdef __cplusplus
extern "C" {
#endif
BOOL NAND_SB_ReadSectorInfo(SECTOR_ADDR sectorAddr, PSectorInfo pInfo);
BOOL NAND_SB_WriteSectorInfo(SECTOR_ADDR sectorAddr, PSectorInfo pInfo);
BOOL NAND_SB_IsBlockBad(BLOCK_ID blockID);
BOOL NAND_SB_MarkBlockBad(BLOCK_ID blockID);
BOOL FMD_SB_ReadSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors);
BOOL RAW_SB_ReadSector(UINT32 startSectorAddr, LPBYTE pSectorBuff, LPBYTE pSectorInfoBuff);
DWORD FMD_SB_GetBlockStatus(BLOCK_ID blockID);
BOOL FMD_SB_EraseBlock(BLOCK_ID blockID);
BOOL FMD_SB_WriteSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors);
BOOL FMD_SB_SetBlockStatus(BLOCK_ID blockID, DWORD dwStatus);
BOOL FMD_SB_GetOEMReservedByte(SECTOR_ADDR physicalSectorAddr, PBYTE pOEMReserved);
BOOL FMD_SB_SetOEMReservedByte(SECTOR_ADDR physicalSectorAddr, BYTE bOEMReserved);
#ifdef _IROMBOOT_
BOOL FMD_SB_WriteSector_Steploader(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors);
#endif
#ifdef __cplusplus
}
#endif

#endif _S3C6410_FMD_SB_

