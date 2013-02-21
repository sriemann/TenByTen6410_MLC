/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : MkMBR.h                                                         */
/* PURPOSE : This file contains making MBR(Master Boot Recorder)             */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2002,2003 SAMSUNG ELECTRONICS CO., LTD.                */
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
/*   14-FEB-2003 [SongHo Yoon]: first writing                                */
/*   28-APR-2004 [JangHwan Kim]: PocketStoreII migration                     */
/*                                                                           */
/*****************************************************************************/

#ifndef _POCKETSTORE_II_MKMBR_H_
#define _POCKETSTORE_II_MKMBR_H_

#include <windows.h>

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
#define MBR_SIZE                SECTOR_SIZE

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

#define PART_CE_HIDDEN          0x18

#define PART_BOOTSECTION        0x20
#define PART_BINFS              0x21
#define PART_XIP                0x22
#define PART_FAT                0x24

#define NUM_PARTS               4
#define SIZE_END_SIG            2
#define PART_ENTRY_SIG          0xabcdabcd
#define INVALID_ADDR            0xffffffff
#define INVALID_PART            0xffffffff
#define INVALID_HANDLE          (HANDLE)-1

// end of sector - 2 bytes for signature - maximum of 4 16-byte partition records
#define PARTTABLE_OFFSET        (SECTOR_SIZE - SIZE_END_SIG - (sizeof(PARTENTRY) * NUM_PARTS))

#define SECTOR_WRITE_COMPLETED                      0x0004  // Indicates data is valid for the FAL
#define MINIMUM_FLASH_BLOCKS_TO_RESERVE				2
#define PERCENTAGE_OF_MEDIA_TO_RESERVE				400		// 0.25% of the media {NOTE: 100% / 0.25% == 400}

typedef struct _PARTSTATE {
    PPARTENTRY  pPartEntry;
    DWORD       dwDataPointer;        // Pointer to where next read and write will occur
} PARTSTATE, *PPARTSTATE;


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// stores a cylinder/head/sector based ATA address
typedef struct _CHSAddr {
    WORD cylinder;
    WORD head;
    WORD sector;
} CHSAddr, *PCHSAddr;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// stores a Logical Block Address
typedef DWORD LBAAddr, *PLBAAddr;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef enum { CHS, LBA } CHSLBA ;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// stores a union of LBA and CHS address
typedef struct _Addr {
    CHSLBA type;
    union {
        LBAAddr lba;
        CHSAddr chs;
    };
} Addr, *PAddr;

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

BOOL32 MakeMBRnPartition(UINT32 nVol, UINT32 nXIPKernBAddr);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif	// _POCKETSTORE_II_MKMBR_H_
