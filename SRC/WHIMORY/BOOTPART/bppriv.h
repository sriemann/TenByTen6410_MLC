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
#ifndef _BPPRIV_H_
#define _BPPRIV_H_

#include <fmd.h>

#define NUM_PARTS                   4
#define SIZE_END_SIG              2
#define PART_ENTRY_SIG          0xabcdabcd
#define INVALID_ADDR            0xffffffff
#define INVALID_PART             0xffffffff
#define INVALID_HANDLE         (HANDLE)-1
// end of sector - 2 bytes for signature - maximum of 4 16-byte partition records
#define PARTTABLE_OFFSET        (SECTOR_SIZE - SIZE_END_SIG - (sizeof(PARTENTRY) * NUM_PARTS))

#define SECTOR_WRITE_COMPLETED 0x0004  // Indicates data is valid for the FAL
#define MINIMUM_FLASH_BLOCKS_TO_RESERVE				2
#define PERCENTAGE_OF_MEDIA_TO_RESERVE				400		// 0.25% of the media {NOTE: 100% / 0.25% == 400}

typedef struct _PARTSTATE {
        PPARTENTRY  pPartEntry;
        DWORD         dwDataPointer;        // Pointer to where next read and write will occur
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


extern "C"
{
//
// Bootpart.cpp private helper functions
//
static BOOL IsValidPart (PPARTENTRY pPartEntry);
static BOOL GetPartitionTableIndex (DWORD dwPartType, BOOL fActive, PDWORD pdwIndex);
}

extern FlashInfo g_FlashInfo;
extern LPBYTE g_pbBlock;


    
#endif  // _BPPRIV_H_

