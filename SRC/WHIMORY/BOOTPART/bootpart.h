//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
#ifndef _BOOTPART_H_
#define _BOOTPART_H_

#include <windows.h>
#include <pcireg.h>
#include <WMRTypes.h>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// end of master boot record contains 4 partition entries
typedef struct _PARTENTRY {
        BYTE            Part_BootInd;           // If 80h means this is boot partition
        BYTE            Part_FirstHead;         // Partition starting head based 0
        BYTE            Part_FirstSector;       // Partition starting sector based 1
        BYTE            Part_FirstTrack;        // Partition starting track based 0
        BYTE            Part_FileSystem;        // Partition type signature field
        BYTE            Part_LastHead;          // Partition ending head based 0
        BYTE            Part_LastSector;        // Partition ending sector based 1
        BYTE            Part_LastTrack;         // Partition ending track based 0
        DWORD           Part_StartSector;       // Logical starting sector based 0
        DWORD           Part_TotalSectors;      // Total logical sectors in partition
} PARTENTRY;
typedef PARTENTRY UNALIGNED *PPARTENTRY;

#define SECTOR_SIZE             512

// Flags for Part_BootInd
#define PART_IND_ACTIVE         0x1
#define PART_IND_READ_ONLY      0x2
#define PART_IND_HIDDEN         0x4

// Flags for Part_FileSystem

#define PART_UNKNOWN            0
#define PART_DOS2_FAT           0x01    // legit DOS partition
#define PART_DOS3_FAT           0x04    // legit DOS partition
#define PART_EXTENDED           0x05    // legit DOS partition
#define PART_DOS4_FAT           0x06    // legit DOS partition
#define PART_DOS32              0x0B    // legit DOS partition (FAT32)
#define PART_DOS32X13           0x0C    // Same as 0x0B only "use LBA"
#define PART_DOSX13             0x0E    // Same as 0x06 only "use LBA"
#define PART_DOSX13X            0x0F    // Same as 0x05 only "use LBA"

// CE only partition types for Part_FileSystem

#define PART_CE_HIDDEN          0x18
#define PART_BOOTSECTION        0x20
#define PART_BINFS              0x21    // BINFS file system
#define PART_XIP                0x22    // XIP ROM Image
#define PART_ROMIMAGE           0x22    // XIP ROM Image (same as PART_XIP)
#define PART_RAMIMAGE           0x23    // XIP RAM Image
#define PART_IMGFS              0x25    // IMGFS file system
#define PART_BINARY             0x26    // Raw Binary Data


// Flags for dwCreationFlags
#define     PART_CREATE_NEW        0
#define     PART_OPEN_EXISTING     1
#define     PART_OPEN_ALWAYS       2

// Flags for BP_LowLevelFormat
#define     FORMAT_SKIP_BLOCK_CHECK      0x1
#define     FORMAT_SKIP_RESERVED         0x2

#define NEXT_FREE_LOC -1
#define USE_REMAINING_SPACE -1

#define IS_BLOCK_BAD(blockID) ((FMD_GetBlockStatus (blockID) & BLOCK_STATUS_BAD) > 0)
#define IS_BLOCK_READONLY(blockID) ((FMD_GetBlockStatus (blockID) & BLOCK_STATUS_READONLY) > 0)
#define IS_BLOCK_RESERVED(blockID) ((FMD_GetBlockStatus (blockID) & BLOCK_STATUS_RESERVED) > 0)
#define IS_BLOCK_UNUSABLE(blockID) ((FMD_GetBlockStatus (blockID) & (BLOCK_STATUS_BAD|BLOCK_STATUS_RESERVED)) > 0)

#define IS_PART_READONLY(bootind) ((bootind & PART_IND_READ_ONLY) != 0)


#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus
//
// Bootpart public functions
//
BOOL BP_Init(LPBYTE pMemory, DWORD dwSize, LPCTSTR lpActiveReg, PPCI_REG_INFO pRegIn, PPCI_REG_INFO pRegOut);
HANDLE BP_OpenPartition(DWORD dwStartSector, DWORD dwNumSectors, DWORD dwPartType, BOOL fActive, DWORD dwCreationFlags);
BOOL   BP_ReadData(HANDLE hPartition, LPBYTE pbBuffer, DWORD dwLength);
BOOL   BP_WriteData(HANDLE hPartition, LPBYTE pbBuffer, DWORD dwLength);
BOOL   BP_SetDataPointer (HANDLE hPartition, DWORD dwAddress);
BOOL   BP_SetPhysDataPointer (HANDLE hPartition, DWORD dwPhysAddress);
PPARTENTRY BP_GetPartitionInfo (HANDLE hPartition);
BOOL BP_LowLevelFormat(DWORD dwStartBlock, DWORD dwNumBlocks, DWORD dwFlags);
// these functions are from fallite.lib
BOOL BP_ReadLogicalSector(HANDLE hPartition, DWORD dwSector, LPBYTE lpBuffer);
BOOL BP_GetUpdateModeFlag(BOOL *pfUpdateMode);

BOOL IsValidMBR(VOID);
BOOL IsValidMBRSector(DWORD dwBlock);
BOOL CreateMBR(VOID);
static BOOL WriteMBR(VOID);
#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _BOOTPART_H_
