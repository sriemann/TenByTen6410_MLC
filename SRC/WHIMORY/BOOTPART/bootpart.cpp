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
extern "C"	// hmseo-061028
{
#include <WMRConfig.h>
#include <WMRTypes.h>
}

#include <bootpart.h>
#include "bppriv.h"

LPBYTE g_pbMBRSector = NULL;
LPBYTE g_pbBlock = NULL;
DWORD g_dwMBRSectorNum = INVALID_ADDR;
FlashInfo g_FlashInfo;
PARTSTATE g_partStateTable[NUM_PARTS];
PSectorInfo g_pSectorInfoBuf;
DWORD g_dwLastLogSector;          // Stores the last valid logical sector
DWORD g_dwDataBytesPerBlock;
DWORD g_dwLastWrittenLoc;  // Stores the byte address of the last physical flash address written to

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static Addr LBAtoCHS(FlashInfo *pFlashInfo, Addr lba)
{
    if(lba.type == CHS)
        return lba;

    Addr chs;
    DWORD tmp = pFlashInfo->dwNumBlocks * pFlashInfo->wSectorsPerBlock;

    chs.type = CHS;
    chs.chs.cylinder = (WORD)(lba.lba / tmp);
    tmp = lba.lba % tmp;
    chs.chs.head = (WORD)(tmp / pFlashInfo->wSectorsPerBlock);
    chs.chs.sector = (WORD)((tmp % pFlashInfo->wSectorsPerBlock) + 1);

    return chs;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static Addr CHStoLBA(FlashInfo *pFlashInfo, Addr chs)
{
    Addr lba;

    if(chs.type == LBA)
        return chs;

    lba.type = LBA;
    lba.lba = ((chs.chs.cylinder * pFlashInfo->dwNumBlocks + chs.chs.head)
        * pFlashInfo->wSectorsPerBlock)+ chs.chs.sector - 1;

    return lba;
}

#define Log2Phys(sector)	(sector)

#define SECTOR_TO_BLOCK(sector)     ((sector) / (SECTORS_PER_SUPAGE*PAGES_PER_SUBLK) )
#define BLOCK_TO_SECTOR(block)      ((block)  * (SECTORS_PER_SUPAGE*PAGES_PER_SUBLK) )

extern BOOL WriteBlock(DWORD dwBlock, LPBYTE pbBlock, PSectorInfo pSectorInfoTable);
extern BOOL ReadBlock(DWORD dwBlock, LPBYTE pbBlock, PSectorInfo pSectorInfoTable);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static BOOL WriteMBR()
{
	DWORD dwMBRBlockNum = g_dwMBRSectorNum / g_FlashInfo.wSectorsPerBlock;

	RETAILMSG(1, (TEXT("WriteMBR: MBR block = 0x%x.\r\n"), dwMBRBlockNum));

	memset (g_pbBlock, 0xff, g_dwDataBytesPerBlock);
	memset (g_pSectorInfoBuf, 0xff, sizeof(SectorInfo) * g_FlashInfo.wSectorsPerBlock);

	// No need to check return, since a failed read means data hasn't been written yet.
	ReadBlock (dwMBRBlockNum, g_pbBlock, g_pSectorInfoBuf);

	if (!FMD_EraseBlock (dwMBRBlockNum)) {
		RETAILMSG (1, (TEXT("CreatePartition: error erasing block 0x%x\r\n"), dwMBRBlockNum));
		return FALSE;
	}

	memcpy (g_pbBlock + (g_dwMBRSectorNum % g_FlashInfo.wSectorsPerBlock) * g_FlashInfo.wDataBytesPerSector, g_pbMBRSector, g_FlashInfo.wDataBytesPerSector);
	g_pSectorInfoBuf->bOEMReserved &= ~OEM_BLOCK_READONLY;
	g_pSectorInfoBuf->wReserved2 &= ~SECTOR_WRITE_COMPLETED;
	g_pSectorInfoBuf->dwReserved1 = 0;

	RETAILMSG(1, (TEXT("WriteBlock: dwMBRBlockNum = 0x%x.\r\n"), dwMBRBlockNum));
#if 0
	for (DWORD i = 0; i < 512; i++) {
		if (i % 16 == 0)
			RETAILMSG(1, (L"0x%x: ", g_pbBlock+i));
		RETAILMSG(1, (L"%x%x ", (g_pbBlock[i] >> 4) & 0x0f, g_pbBlock[i] & 0x0f));
		if ((i + 1) % 16 == 0)
			RETAILMSG(1, (L"\r\n"));
	}
#endif
	if (!WriteBlock (dwMBRBlockNum, g_pbBlock, g_pSectorInfoBuf)) {
		RETAILMSG (1, (TEXT("CreatePartition: could not write to block 0x%x\r\n"), dwMBRBlockNum));
		return FALSE;
	}

	return TRUE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL CreateMBR()
{
    // This, plus a valid partition table, is all the CE partition manager needs to recognize
    // the MBR as valid. It does not contain boot code.

    memset (g_pbMBRSector, 0xff, g_FlashInfo.wDataBytesPerSector);
    g_pbMBRSector[0] = 0xE9;
    g_pbMBRSector[1] = 0xfd;
    g_pbMBRSector[2] = 0xff;
    g_pbMBRSector[SECTOR_SIZE-2] = 0x55;
    g_pbMBRSector[SECTOR_SIZE-1] = 0xAA;

    // Zero out partition table so that mspart treats entries as empty.
    memset (g_pbMBRSector+PARTTABLE_OFFSET, 0, sizeof(PARTENTRY) * NUM_PARTS);

    return WriteMBR();

}

BOOL IsValidMBR()
{
    // Check to see if the MBR is valid

    // MBR block is always located at logical sector 0
#if 0	// hmseo-061029
    g_dwMBRSectorNum = 0;	//GetMBRSectorNum();
#else	// hmseo-061029
//    g_dwMBRSectorNum = BLOCK_TO_SECTOR(FTL_AREA_START);	// hmseo-061029 set the MBR sector to FTL area start sector.
#endif	// hmseo-061029

    RETAILMSG (1, (TEXT("IsValidMBR: MBR sector = 0x%x\r\n"), g_dwMBRSectorNum));

    if ((g_dwMBRSectorNum == INVALID_ADDR) || !FMD_ReadSector (g_dwMBRSectorNum, g_pbMBRSector, NULL, 1)) {
        return FALSE;
    }

#if 0
    LPBYTE pbBuf = g_pbMBRSector + 512 - 0x42;
    RETAILMSG(1, (L"g_pbMBRSector = 0x%x\r\n", g_pbMBRSector));
    for (DWORD i = 0; i < 0x40; i++) {
    	if (i % 16 == 0)
    		RETAILMSG(1, (L"0x%x: ", pbBuf+i));
    	RETAILMSG(1, (L"%x%x ", (pbBuf[i] >> 4) & 0x0f, pbBuf[i] & 0x0f));
    	if ((i + 1) % 16 == 0)
    		RETAILMSG(1, (L"\r\n"));
    }
#endif
    return ((g_pbMBRSector[0] == 0xE9) &&
         (g_pbMBRSector[1] == 0xfd) &&
         (g_pbMBRSector[2] == 0xff) &&
         (g_pbMBRSector[SECTOR_SIZE-2] == 0x55) &&
         (g_pbMBRSector[SECTOR_SIZE-1] == 0xAA));
}

BOOL IsValidMBRSector(DWORD dwBlock)
{
	g_dwMBRSectorNum = BLOCK_TO_SECTOR(dwBlock);

	return IsValidMBR();
}

static BOOL IsValidPart (PPARTENTRY pPartEntry)
{
    return (pPartEntry->Part_FileSystem != 0xff) && (pPartEntry->Part_FileSystem != 0);
}

/*  AddPartitionTableEntry
 *
 *  Generates the partition entry for the partition table and copies the entry
 *  into the MBR that is stored in memory.
 *
 *
 *  ENTRY
 *      entry - index into partition table
 *      startSector - starting logical sector
 *      totalSectors - total logical sectors
 *      fileSystem - type of partition
 *      bootInd - byte in partition entry that stores various flags such as
 *          active and read-only status.
 *
 *  EXIT
 */

static void AddPartitionTableEntry(DWORD entry, DWORD startSector, DWORD totalSectors, BYTE fileSystem, BYTE bootInd)
{
    PARTENTRY partentry = {0};
    Addr startAddr;
    Addr endAddr;

    ASSERT(entry < 4);

    // no checking with disk info and start/total sectors because we allow
    // bogus partitions for testing purposes

    // initially known partition table entry
    partentry.Part_BootInd = bootInd;
    partentry.Part_FileSystem = fileSystem;
    partentry.Part_StartSector = startSector;
    partentry.Part_TotalSectors = totalSectors;

    // logical block addresses for the first and final sector (start on the second head)
    startAddr.type = LBA;
    startAddr.lba = partentry.Part_StartSector;
    endAddr.type = LBA;
    endAddr.lba = partentry.Part_StartSector + partentry.Part_TotalSectors-1;

    // translate the LBA addresses to CHS addresses
    startAddr = LBAtoCHS(&g_FlashInfo, startAddr);
    endAddr = LBAtoCHS(&g_FlashInfo, endAddr);

    // starting address
    partentry.Part_FirstTrack = (BYTE)(startAddr.chs.cylinder & 0xFF);
    partentry.Part_FirstHead = (BYTE)(startAddr.chs.head & 0xFF);
    // lower 6-bits == sector, upper 2-bits = cylinder upper 2-bits of 10-bit cylinder #
    partentry.Part_FirstSector = (BYTE)((startAddr.chs.sector & 0x3F) | ((startAddr.chs.cylinder & 0x0300) >> 2));

    // ending address:
    partentry.Part_LastTrack = (BYTE)(endAddr.chs.cylinder & 0xFF);
    partentry.Part_LastHead = (BYTE)(endAddr.chs.head & 0xFF);
    // lower 6-bits == sector, upper 2-bits = cylinder upper 2-bits of 10-bit cylinder #
    partentry.Part_LastSector = (BYTE)((endAddr.chs.sector & 0x3F) | ((endAddr.chs.cylinder & 0x0300) >> 2));

    memcpy(g_pbMBRSector+PARTTABLE_OFFSET+(sizeof(PARTENTRY)*entry), &partentry, sizeof(PARTENTRY));
}



/*  GetPartitionTableIndex
 *
 *  Get the partition index for a particular partition type and active status.
 *  If partition is not found, then the index of the next free partition in the
 *  partition table of the MBR is returned.
 *
 *
 *  ENTRY
 *      dwPartType - type of partition
 *      fActive - TRUE indicates the active partition.  FALSE indicates inactive.
 *
 *  EXIT
 *      pdwIndex - Contains the index of the partition if found.  If not found,
 *          contains the index of the next free partition
 *      returns TRUE if partition found. FALSE if not found.
 */

static BOOL GetPartitionTableIndex (DWORD dwPartType, BOOL fActive, PDWORD pdwIndex)
{
    PPARTENTRY pPartEntry = (PPARTENTRY)(g_pbMBRSector + PARTTABLE_OFFSET);
    DWORD iEntry = 0;

    for (iEntry = 0; iEntry < NUM_PARTS; iEntry++, pPartEntry++) {
        if ((pPartEntry->Part_FileSystem == dwPartType) && (((pPartEntry->Part_BootInd & PART_IND_ACTIVE) != 0) == fActive)) {
            *pdwIndex = iEntry;
            return TRUE;
        }
        if (!IsValidPart (pPartEntry)) {
            *pdwIndex = iEntry;
            return FALSE;
        }
    }

    return FALSE;
}

/* WriteLogicalNumbers
 *
 *  Writes a range of logical sector numbers
 *
 *  ENTRY
 *      dwStartSector - starting logical sector
 *      dwNumSectors - number of logical sectors to mark
 *      fReadOnly - TRUE indicates to mark read-only.  FALSE to mark not read-only
 *
 *  EXIT
 *      TRUE on success
 */


static BOOL WriteLogicalNumbers (DWORD dwStartSector, DWORD dwNumSectors, BOOL fReadOnly)
{
    DWORD dwNumSectorsWritten = 0;

    DWORD dwPhysSector = Log2Phys (dwStartSector);
    DWORD dwBlockNum = dwPhysSector / g_FlashInfo.wSectorsPerBlock;
    DWORD dwOffset = dwPhysSector % g_FlashInfo.wSectorsPerBlock;

    while (dwNumSectorsWritten < dwNumSectors) {

        // If bad block, move to the next block
#if 0
        if (IS_BLOCK_UNUSABLE (dwBlockNum)) {
            dwBlockNum++;
            continue;
        }
#endif

        memset (g_pbBlock, 0xff, g_dwDataBytesPerBlock);
        memset (g_pSectorInfoBuf, 0xff, sizeof(SectorInfo) * g_FlashInfo.wSectorsPerBlock);
        // No need to check return, since a failed read means data hasn't been written yet.
        ReadBlock (dwBlockNum, g_pbBlock, g_pSectorInfoBuf);
        if (!FMD_EraseBlock (dwBlockNum)) {
            return FALSE;
        }

        DWORD dwSectorsToWrite = g_FlashInfo.wSectorsPerBlock - dwOffset;
        PSectorInfo pSectorInfo = g_pSectorInfoBuf + dwOffset;

        // If this is the last block, then calculate sectors to write if there isn't a full block to update
        if ((dwSectorsToWrite + dwNumSectorsWritten) > dwNumSectors)
            dwSectorsToWrite = dwNumSectors - dwNumSectorsWritten;

        for (DWORD iSector = 0; iSector < dwSectorsToWrite; iSector++, pSectorInfo++, dwNumSectorsWritten++) {
            // Assert read only by setting bit to 0 to prevent wear-leveling by FAL
            if (fReadOnly)
                pSectorInfo->bOEMReserved &= ~OEM_BLOCK_READONLY;
            // Set to write completed so FAL can map the sector
            pSectorInfo->wReserved2 &= ~SECTOR_WRITE_COMPLETED;
            // Write the logical sector number
            pSectorInfo->dwReserved1 = dwStartSector + dwNumSectorsWritten;
        }

        if (!WriteBlock (dwBlockNum, g_pbBlock, g_pSectorInfoBuf))
            return FALSE;

        dwOffset = 0;
        dwBlockNum++;
    }
    return TRUE;
}



/*  CreatePartition
 *
 *  Creates a new partition.  If it is a boot section partition, then it formats
 *  flash.
 *
 *  ENTRY
 *      dwStartSector - Logical sector to start the partition.  NEXT_FREE_LOC if
 *          none specified.
 *      dwNumSectors - Number of logical sectors of the partition.  USE_REMAINING_SPACE
 *          to indicate to take up the rest of the space on the flash for that partition.
 *      dwPartType - Type of partition to create.
 *      fActive - TRUE indicates to create the active partition.  FALSE for
 *          inactive.
 *      dwPartIndex - Index of the partition entry on the MBR
 *
 *  EXIT
 *      Handle to the partition on success.  INVALID_HANDLE_VALUE on error.
 */

static HANDLE CreatePartition (DWORD dwStartSector, DWORD dwNumSectors, DWORD dwPartType, BOOL fActive, DWORD dwPartIndex)
{
    DWORD dwBootInd = 0;

    RETAILMSG(1, (TEXT("CreatePartition: Enter CreatePartition for 0x%x.\r\n"), dwPartType));
#if 0

    if (fActive)
        dwBootInd |= PART_IND_ACTIVE;
    if (dwPartType == PART_BOOTSECTION || dwPartType == PART_BINFS || dwPartType == PART_XIP)
        dwBootInd |= PART_IND_READ_ONLY;

     // If start sector is invalid, it means find next free sector
    if (dwStartSector == NEXT_FREE_LOC) {

        dwStartSector = FindFreeSector();
        if (dwStartSector == INVALID_ADDR) {
            RETAILMSG(1, (TEXT("CreatePartition: can't find free sector.\r\n")));
            return INVALID_HANDLE_VALUE;
        }

        // Start partitions on the next block if they are currently on the wrong block type.
        if (dwStartSector % g_FlashInfo.wSectorsPerBlock) {
            DWORD dwBlock = dwStartSector / g_FlashInfo.wSectorsPerBlock;
            //if (IS_PART_READONLY(dwBootInd) != IS_BLOCK_READONLY(dwBlock)) {
                dwStartSector = (dwBlock+1) * g_FlashInfo.wSectorsPerBlock;
            //}
        }
    }
    if (IS_PART_READONLY(dwBootInd)) {

        // Allow read-only partitions to go to the end of disk, if requested.
        if (dwNumSectors == USE_REMAINING_SPACE) {

            DWORD dwLastLogSector = LastLogSector();
            if (dwLastLogSector == INVALID_ADDR)
                return INVALID_HANDLE_VALUE;

            dwNumSectors = dwLastLogSector - dwStartSector + 1;
        }
    }
    else {
        DWORD dwLastLogSector = LastLogSector();
        if (dwLastLogSector == INVALID_ADDR)
            return INVALID_HANDLE_VALUE;

        // Determine the number of blocks to reserve for the FAL compaction when creating an extended partition.
        DWORD dwReservedBlocks = g_FlashInfo.dwNumBlocks / PERCENTAGE_OF_MEDIA_TO_RESERVE;
        if((dwReservedBlocks = g_FlashInfo.dwNumBlocks / PERCENTAGE_OF_MEDIA_TO_RESERVE) < MINIMUM_FLASH_BLOCKS_TO_RESERVE) {
            dwReservedBlocks = MINIMUM_FLASH_BLOCKS_TO_RESERVE;
        }

        DWORD dwNumMaxSectors = dwLastLogSector - dwStartSector + 1 - dwReservedBlocks * g_FlashInfo.wSectorsPerBlock;

        // If dwNumSectors was provided, validate it isn't past the max.
        // If dwNumSectors is USE_REMAINING_SPACE, fill disk with max sectors.
        if ((dwNumSectors == USE_REMAINING_SPACE)  || (dwNumMaxSectors <  dwNumSectors)) {
            RETAILMSG(1, (TEXT("CreatePartition: Num sectors set to 0x%x to allow for compaction blocks.\r\n"), dwNumMaxSectors));
            dwNumSectors = dwNumMaxSectors ;
        }
    //}


    if (!AreSectorsFree (dwStartSector, dwNumSectors)){
        RETAILMSG (1, (TEXT("CreatePartition: sectors [0x%x, 0x%x] requested are out of range or taken by another partition\r\n"), dwStartSector, dwNumSectors));
        return INVALID_HANDLE_VALUE;
    }
#endif

//	dwStartSector = (SPECIAL_AREA_START + 1)*SECTORS_PER_SUBLK;
	dwStartSector = (1)*SECTORS_PER_SUBLK;	// by hmseo-061209.. because bibdrv calculate the sector address by add with SPECIAL_AREA_START.

	RETAILMSG(1, (TEXT("CreatePartition: Start = 0x%x, Num = 0x%x.\r\n"), dwStartSector, dwNumSectors));

	AddPartitionTableEntry (dwPartIndex, dwStartSector, dwNumSectors, (BYTE)dwPartType, (BYTE)dwBootInd);

#if 0	// by hmseo - it is not fmd driver, therefore this WriteLogicalNumbers routine is not needed. 061124
    //if (IS_PART_READONLY(dwBootInd)) {
        if (!WriteLogicalNumbers (dwStartSector, dwNumSectors, TRUE)) {
            RETAILMSG(1, (TEXT("CreatePartition: can't mark sector info.\r\n")));
            return INVALID_HANDLE_VALUE;
        }
    //}
#endif

    if (!WriteMBR())
        return INVALID_HANDLE_VALUE;

    g_partStateTable[dwPartIndex].pPartEntry = (PPARTENTRY)(g_pbMBRSector + PARTTABLE_OFFSET + sizeof(PARTENTRY)*dwPartIndex);
    g_partStateTable[dwPartIndex].dwDataPointer = 0;

    return (HANDLE)&g_partStateTable[dwPartIndex];
}


/*  BP_ReadData
 *
 *  Reads data from the partition starting at the data pointer.  Call fails
 *  if length of the buffer is too long (i.e. trying to read past the end
 *  of the partition)
 *
 *  ENTRY
 *      hPartition - handle to the partition
 *      pbBuffer - pointer to buffer of data to read
 *      dwLength - number of bytes to read
 *
 *  EXIT
 *      TRUE on success
 */
BOOL BP_ReadData(HANDLE hPartition, LPBYTE pbBuffer, DWORD dwLength)
{
    if (hPartition == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD dwNumSects;
    static LPBYTE pbSector = g_pbBlock;
    PPARTSTATE pPartState = (PPARTSTATE) hPartition;
    DWORD dwNextPtrValue = pPartState->dwDataPointer + dwLength;

    if (!pbBuffer || !pbSector || dwLength == 0)
        return(FALSE);

    // RETAILMSG(1,(TEXT("ReadData: Start = 0x%x, Length = 0x%x.\r\n"), pPartState->dwDataPointer, dwLength));

    // Check to make sure buffer size is within limits of partition
    if (((dwNextPtrValue - 1) / g_FlashInfo.wDataBytesPerSector) >= pPartState->pPartEntry->Part_TotalSectors) {
        RETAILMSG (1, (TEXT("ReadData: trying to read past end of partition.\r\n")));
        return FALSE;
    }

    // Get the starting physical sector
    DWORD dwSectorAddr = Log2Phys (pPartState->dwDataPointer / g_FlashInfo.wDataBytesPerSector + pPartState->pPartEntry->Part_StartSector);
    DWORD dwSector0 = dwSectorAddr;
    DWORD dwLen0 = dwLength;
    LPBYTE pbBuf0 = pbBuffer;

    // If current pointer is not on a sector boundary, copy bytes up to the first sector boundary
    DWORD dwOffsetSector = pPartState->dwDataPointer % g_FlashInfo.wDataBytesPerSector;
    if (dwOffsetSector)
    {
        if (!FMD_ReadSector(dwSectorAddr, pbSector, NULL, 1))
        {
            RETAILMSG (1, (TEXT("ReadData: failed to read sector (0x%x).\r\n"), dwSectorAddr));
            return(FALSE);
        }

        DWORD dwNumBytesRead = g_FlashInfo.wDataBytesPerSector - dwOffsetSector;
        if (dwNumBytesRead > dwLength)
            dwNumBytesRead = dwLength;

        memcpy(pbBuffer, pbSector + dwOffsetSector, dwNumBytesRead);
        dwLength -= dwNumBytesRead;
        pbBuffer += dwNumBytesRead;
        dwSectorAddr++;
    }

    // Compute sector length.
    dwNumSects = (dwLength / g_FlashInfo.wDataBytesPerSector);

	if (dwNumSects)
		FMD_ReadSector(dwSectorAddr, pbBuffer, NULL, dwNumSects);

    DWORD dwNumExtraBytes = (dwLength % g_FlashInfo.wDataBytesPerSector);
    if (dwNumExtraBytes)
    {
    	dwSectorAddr += dwNumSects;
    	pbBuffer += dwNumSects*g_FlashInfo.wDataBytesPerSector;
        if (!FMD_ReadSector(dwSectorAddr, pbSector, NULL, 1))
        {
            RETAILMSG (1, (TEXT("ReadData: failed to read sector (0x%x).\r\n"), dwSectorAddr));
            return(FALSE);
        }
        memcpy(pbBuffer, pbSector, dwNumExtraBytes);
    }

#if 0
    //dump if inside 10 sectors
    if (pPartState->dwDataPointer < 10*512) {
    	RETAILMSG(1, (L"BP_ReadData: StartSector=0x%x, SectorOffset=0x%x, Leghth=0x%x",
    		dwSector0, dwOffsetSector, dwLen0));

    	DWORD dwAddr = pPartState->dwDataPointer;
    	DWORD dwOffset16 = dwOffsetSector % 16;
    	DWORD i;

    	if (dwOffset16 != 0) {
    		dwAddr -= dwOffset16;
    		RETAILMSG(1, (L"\r\nSector=0x%x(0x%x)",
    			dwSector0 + (dwAddr / 512),
    			(dwSector0 + (dwAddr / 512))*512));
    		RETAILMSG(1, (L"\r\n%x%x%x%x: ", (dwAddr >> 12) & 0x0f, (dwAddr >> 8) & 0x0f,
    				(dwAddr >> 4) & 0x0f, dwAddr & 0x0f));
    		for (i = 0; i < dwOffset16; i++, dwAddr++)
    			RETAILMSG(1, (L"__ "));
    	}

    	for (i = 0; i < dwLen0; i++, dwAddr++) {
    		if ((dwAddr % 16) == 0) {
    			if ((dwAddr % 512) == 0)
    				RETAILMSG(1, (L"\r\nSector=0x%x(0x%x)",
    					dwSector0 + (dwAddr / 512),
    					(dwSector0 + (dwAddr / 512))*512));
    			RETAILMSG(1, (L"\r\n%x%x%x%x: ", (dwAddr >> 12) & 0x0f, (dwAddr >> 8) & 0x0f,
    				(dwAddr >> 4) & 0x0f, dwAddr & 0x0f));
    		}
    		RETAILMSG(1, (L"%x%x ", (pbBuf0[i] >> 4) & 0x0f, pbBuf0[i] & 0x0f));
    	}

    	RETAILMSG(1, (L"\r\n"));
    }
#endif

    pPartState->dwDataPointer = dwNextPtrValue;
    return(TRUE);
}

/*  BP_WriteData
 *
 *  Writes data to the partition starting at the data pointer.  Call fails
 *  if length of the buffer is too long (i.e. trying to write past the end
 *  of the partition)
 *
 *  ENTRY
 *      hPartition - handle to the partition
 *      pbBuffer - pointer to buffer of data to write
 *      dwLength - length in bytes of the buffer
 *
 *  EXIT
 *      TRUE on success
 */

BOOL BP_WriteData(HANDLE hPartition, LPBYTE pbBuffer, DWORD dwLength)
{
	if (hPartition == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD dwNumSects;
	static LPBYTE pbSector = g_pbBlock;
	PPARTSTATE pPartState = (PPARTSTATE) hPartition;
	DWORD dwNextPtrValue = pPartState->dwDataPointer + dwLength;

	if (!pbBuffer || !pbSector || dwLength == 0)
		return(FALSE);

	RETAILMSG (1, (TEXT("BP_WriteData: Start = 0x%x, Length = 0x%x.\r\n"), pPartState->dwDataPointer, dwLength));

	// Check to make sure buffer size is within limits of partition
	if (((dwNextPtrValue - 1) / g_FlashInfo.wDataBytesPerSector) >= pPartState->pPartEntry->Part_TotalSectors) {
		RETAILMSG (1, (TEXT("ReadData: trying to read past end of partition.(Part_TotalSectors = 0x%x)\r\n"), pPartState->pPartEntry->Part_TotalSectors));
		return FALSE;
	}

	// Get the starting physical sector
//	DWORD dwSectorAddr = Log2Phys (pPartState->dwDataPointer / g_FlashInfo.wDataBytesPerSector + pPartState->pPartEntry->Part_StartSector);
//    DWORD dwSectorAddr = 0;	// hmseo-061124
//	RETAILMSG (1, (TEXT("BP_WriteData: pPartState->pPartEntry->Part_StartSector = 0x%x.\r\n"), pPartState->pPartEntry->Part_StartSector));
//	DWORD dwSectorAddr = pPartState->dwDataPointer / BYTES_PER_SECTOR + pPartState->pPartEntry->Part_StartSector;	// hmseo-061124
	DWORD dwSectorAddr = (pPartState->dwDataPointer / BYTES_PER_SECTOR);	// hmseo-061124

	// If current pointer is not on a sector boundary, copy bytes up to the first sector boundary
	DWORD dwOffsetSector = pPartState->dwDataPointer % g_FlashInfo.wDataBytesPerSector;

	RETAILMSG (1, (TEXT("BP_WriteData: dwSectorAddr = 0x%x, dwOffsetSector = 0x%x.\r\n"), dwSectorAddr, dwOffsetSector));

	if (dwOffsetSector)
	{
		if (!FMD_ReadSector(dwSectorAddr, pbSector, NULL, SECTORS_PER_SUPAGE))
		{
			RETAILMSG (1, (TEXT("WriteData: failed to read sector (0x%x).\r\n"), dwSectorAddr));
			return(FALSE);
		}

		DWORD dwNumBytesWrite = g_FlashInfo.wDataBytesPerSector - dwOffsetSector;
		if (dwNumBytesWrite > dwLength)
			dwNumBytesWrite = dwLength;

		memcpy(pbSector + dwOffsetSector, pbBuffer, dwNumBytesWrite);
		dwLength -= dwNumBytesWrite;
		pbBuffer += dwNumBytesWrite;

		if (!FMD_WriteSector(dwSectorAddr, pbSector, NULL, SECTORS_PER_SUPAGE))
		{
			RETAILMSG(1, (TEXT("WriteData: faild to write sector (0x%x).\r\n"), dwSectorAddr));
			return(FALSE);
		}

		dwSectorAddr+=SECTORS_PER_SUPAGE;
	}

	// Compute sector length.
//	dwNumSects = (dwLength / g_FlashInfo.wDataBytesPerSector);	// by hmseo
	dwNumSects = (dwLength / (BYTES_PER_SECTOR*SECTORS_PER_SUPAGE) * SECTORS_PER_SUPAGE);

	RETAILMSG (1, (TEXT("BP_WriteData: dwNumSects = 0x%x\r\n"), dwNumSects));

	if (dwNumSects)
	{
		INT32   nErr;
		UINT32  nVol = 0;
		UINT32	nPercent = 0;
		UINT32	nCnt;

//		for(dwSectorAddr = 0; dwSectorAddr < dwNumSects; dwSectorAddr+=4)
		for(nCnt = 0; nCnt < dwNumSects; nCnt+=SECTORS_PER_SUPAGE)
		{
			nErr = FMD_WriteSector(dwSectorAddr+nCnt, pbBuffer, NULL, SECTORS_PER_SUPAGE);
			if(nErr != TRUE)
			{
				RETAILMSG(1, (L"[ONW: ERR]  FMD_WriteSector Error at %d sector\r\n", dwSectorAddr));
				while(1);
			}
			pbBuffer += (BYTES_PER_SECTOR*SECTORS_PER_SUPAGE);
		}
	}

//	DWORD dwNumExtraBytes = (dwLength % g_FlashInfo.wDataBytesPerSector);
	DWORD dwNumExtraBytes = (dwLength % (BYTES_PER_SECTOR*SECTORS_PER_SUPAGE));

	RETAILMSG (1, (TEXT("BP_WriteData: dwNumExtraBytes = 0x%x\r\n"), dwNumExtraBytes));

	if (dwNumExtraBytes)
	{
		RETAILMSG (1, (TEXT("BP_WriteData: dwSectorAddr = 0x%x\r\n"), dwSectorAddr));
		RETAILMSG (1, (TEXT("BP_WriteData: dwNumSects = 0x%x\r\n"), dwNumSects));

		dwSectorAddr += dwNumSects;
//		pbBuffer += dwNumSects*g_FlashInfo.wDataBytesPerSector;

		RETAILMSG (1, (TEXT("BP_WriteData: dwSectorAddr = 0x%x\r\n"), dwSectorAddr));
		RETAILMSG (1, (TEXT("BP_WriteData: pbBuffer = 0x%x\r\n"), pbBuffer));

		if (!FMD_ReadSector(dwSectorAddr, pbSector, NULL, SECTORS_PER_SUPAGE))
		{
			RETAILMSG (1, (TEXT("WriteData: failed to read sector (0x%x).\r\n"), dwSectorAddr));
			return(FALSE);
		}
		memcpy(pbSector, pbBuffer, dwNumExtraBytes);

		if (!FMD_WriteSector(dwSectorAddr, pbSector, NULL, SECTORS_PER_SUPAGE))
		{
			RETAILMSG(1, (TEXT("WriteData: failed to read sector (0x%x).\r\n"), dwSectorAddr));
			return(FALSE);
		}
	}

	pPartState->dwDataPointer = dwNextPtrValue;
	return(TRUE);
}

/*  BP_OpenPartition
 *
 *  Opens/creates a partition depending on the creation flags.  If it is opening
 *  and the partition has already been opened, then it returns a handle to the
 *  opened partition.  Otherwise, it loads the state information of that partition
 *  into memory and returns a handle.
 *
 *  ENTRY
 *      dwStartSector - Logical sector to start the partition.  NEXT_FREE_LOC if none
 *          specified.  Ignored if opening existing partition.
 *      dwNumSectors - Number of logical sectors of the partition.  USE_REMAINING_SPACE
 *          to indicate to take up the rest of the space on the flash for that partition (should
 *          only be used when creating extended partitions).  This parameter is ignored
 *          if opening existing partition.
 *      dwPartType - Type of partition to create/open.
 *      fActive - TRUE indicates to create/open the active partition.  FALSE for
 *          inactive.
 *      dwCreationFlags - PART_CREATE_NEW to create only.  Fail if it already
 *          exists.  PART_OPEN_EXISTING to open only.  Fail if it doesn't exist.
 *          PART_OPEN_ALWAYS creates if it does not exist and opens if it
 *          does exist.
 *
 *  EXIT
 *      Handle to the partition on success.  INVALID_HANDLE_VALUE on error.
 */

HANDLE BP_OpenPartition(DWORD dwStartSector, DWORD dwNumSectors, DWORD dwPartType, BOOL fActive, DWORD dwCreationFlags)
{
	DWORD dwPartIndex;
	BOOL fExists;

	RETAILMSG(1, (TEXT("OpenPartition: dwStartSector = 0x%x.\r\n"), dwStartSector));
	RETAILMSG(1, (TEXT("OpenPartition: dwNumSectors = 0x%x.\r\n"), dwNumSectors));
	RETAILMSG(1, (TEXT("OpenPartition: dwPartType = 0x%x.\r\n"), dwPartType));
	RETAILMSG(1, (TEXT("OpenPartition: fActive = 0x%x.\r\n"), fActive));
	RETAILMSG(1, (TEXT("OpenPartition: dwCreationFlags = 0x%x.\r\n"), dwCreationFlags));

	ASSERT (g_pbMBRSector);

	if (!IsValidMBR()) {
		DWORD dwFlags = 0;

		if (dwCreationFlags == PART_OPEN_EXISTING) {
			RETAILMSG(1, (TEXT("OpenPartition: Invalid MBR.  Cannot open existing partition 0x%x.\r\n"), dwPartType));
			return INVALID_HANDLE_VALUE;
		}

		RETAILMSG(1, (TEXT("OpenPartition: Invalid MBR.  Formatting flash.\r\n")));
		if (g_FlashInfo.flashType == NOR) {
			dwFlags |= FORMAT_SKIP_BLOCK_CHECK;
		}

		BP_LowLevelFormat (SECTOR_TO_BLOCK(dwStartSector), dwNumSectors/(SECTORS_PER_SUPAGE*PAGES_PER_SUBLK), dwFlags);
		dwPartIndex = 0;
		fExists = FALSE;
	}
	else {
		fExists = GetPartitionTableIndex(dwPartType, fActive, &dwPartIndex);
	}

	if (fExists) {
		// Partition was found.
		if (dwCreationFlags == PART_CREATE_NEW)
			return INVALID_HANDLE_VALUE;

		if (g_partStateTable[dwPartIndex].pPartEntry == NULL) {
			// Open partition.  If this is the boot section partition, then file pointer starts after MBR
			g_partStateTable[dwPartIndex].pPartEntry = (PPARTENTRY)(g_pbMBRSector + PARTTABLE_OFFSET + sizeof(PARTENTRY)*dwPartIndex);
			g_partStateTable[dwPartIndex].dwDataPointer = 0;
		}
		return (HANDLE)&g_partStateTable[dwPartIndex];
	}
	else {
		// If there are already 4 partitions, or creation flag specified OPEN_EXISTING, fail.
		if ((dwPartIndex == NUM_PARTS) || (dwCreationFlags == PART_OPEN_EXISTING))
			return INVALID_HANDLE_VALUE;

		// Create new partition
		return CreatePartition (dwStartSector, dwNumSectors, dwPartType, fActive, dwPartIndex);
	}

	return INVALID_HANDLE_VALUE;
}


/*  BP_SetDataPointer
 *
 *  Sets the data pointer of a particular partition.  Data pointer stores the logical
 *  byte address where the next read or write will occur.
 *
 *  ENTRY
 *      hPartition - handle to the partition
 *      dwAddress - Address to set data pointer to
 *
 *  EXIT
 *      TRUE on success
 */

BOOL BP_SetDataPointer (HANDLE hPartition, DWORD dwAddress)
{
	if (hPartition == INVALID_HANDLE_VALUE)
		return FALSE;

	RETAILMSG(1,(TEXT("BP_SetDataPointer at 0x%x\r\n"), dwAddress));

	PPARTSTATE pPartState = (PPARTSTATE) hPartition;

	if (dwAddress >= pPartState->pPartEntry->Part_TotalSectors * g_FlashInfo.wDataBytesPerSector)
	{
		RETAILMSG(1,(TEXT("pPartState->pPartEntry->Part_TotalSectors = 0x%x\r\n"), pPartState->pPartEntry->Part_TotalSectors));
		RETAILMSG(1,(TEXT("g_FlashInfo.wDataBytesPerSector = 0x%x\r\n"), g_FlashInfo.wDataBytesPerSector));
		RETAILMSG(1,(TEXT("pPartState->pPartEntry->Part_TotalSectors * g_FlashInfo.wDataBytesPerSector = 0x%x\r\n"), pPartState->pPartEntry->Part_TotalSectors * g_FlashInfo.wDataBytesPerSector));
		return FALSE;
	}

	pPartState->dwDataPointer = dwAddress;
	return TRUE;
}

/*  BP_SetPhysDataPointer
 *
 *  Sets the data pointer of a particular partition.  This function compensates
 *  for bad and reserved blocks, and accounts for them in the supplied address.
 *  This behavior is useful on NOR flash when locating data based on statically
 *  mapped virtual addresses that do not account for reserved ranges.
 *
 *  ENTRY
 *      hPartition - handle to the partition
 *      dwAddress - Physical address to set data pointer to, zero-based from the
 *          partition start.
 *
 *  EXIT
 *      TRUE on success
 */

BOOL BP_SetPhysDataPointer (HANDLE hPartition, DWORD dwPhysAddress)
{
    if (hPartition == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // RETAILMSG(1,(TEXT("BP_SetPhysDataPointer at 0x%x\r\n"), dwPhysAddress));

    return BP_SetDataPointer(hPartition, dwPhysAddress);
}


/*  BP_GetPartitionInfo
 *
 *  Get the partition entry for an open partition.
 *
 *  ENTRY
 *      hPartition - handle to the partition
 *
 *  EXIT
 *      The partition entry
 */

PPARTENTRY BP_GetPartitionInfo (HANDLE hPartition)
{
    if (!hPartition)
        return NULL;

    return ((PPARTSTATE)hPartition)->pPartEntry;
}


/*  BP_Init
 *
 *  Sets up locations for various objects in memory provided by caller
 *
 *  ENTRY
 *      pMemory - pointer to memory for storing objects
 *      dwSize - size of the memory
 *      lpActiveReg - used by FMD_Init. NULL if not needed.
 *      pRegIn - used by FMD_Init. NULL if not needed.
 *      pRegOut - used by FMD_Init. NULL if not needed.
 *
 *  EXIT
 *      TRUE on success
 */
BOOL BP_Init (LPBYTE pMemory, DWORD dwSize, LPCTSTR lpActiveReg, PPCI_REG_INFO pRegIn, PPCI_REG_INFO pRegOut)
{
    if (!pMemory) {
        RETAILMSG(1,(TEXT("BP_Init Fails No memory fails!!!\r\n")));
        return FALSE;
    }

    if (!FMD_Init (lpActiveReg, pRegIn, pRegOut))
        return FALSE;

    if (!FMD_GetInfo (&g_FlashInfo)) {
        RETAILMSG(1,(TEXT("BP_Init Fails FMD_GetInfo fails!!!\r\n")));
        return FALSE;
    }

    // Check to make sure size is enough for one sector, one block, and sectorinfo buffer for one block
    g_dwDataBytesPerBlock = g_FlashInfo.wDataBytesPerSector * g_FlashInfo.wSectorsPerBlock;

    for (int i = 0; i < NUM_PARTS; i++) {
        g_partStateTable[i].pPartEntry= NULL;
        g_partStateTable[i].dwDataPointer = 0;
    }

    g_pbMBRSector = pMemory;
    g_pbBlock = pMemory + g_FlashInfo.wDataBytesPerSector;
    g_pSectorInfoBuf = (PSectorInfo)(g_pbBlock + g_dwDataBytesPerBlock);
    g_dwLastLogSector = 0;

    return TRUE;
}

/*  BP_LowLevelFormat
 *
 *  Called when preparing flash for a multiple-BIN download.
 *  Erases, verifies, and writes logical sector numbers in the range to be written.
 *
 *  ENTRY
 *      dwStartBlock - starting physical block for format
 *      dwNumBlocks - number of physical blocks to format
 *      dwFlags - Flags used in formatting.
 *
 *  EXIT
 *      TRUE returned on success and FALSE on failure.
 */
BOOL BP_LowLevelFormat(DWORD dwStartBlock, DWORD dwNumBlocks, DWORD dwFlags)
{
	DWORD nBlockNum;
	//dwNumBlocks = min (dwNumBlocks, g_FlashInfo.dwNumBlocks);

	RETAILMSG(1,(TEXT("Enter LowLevelFormat [0x%x, 0x%x].\r\n"), dwStartBlock, dwStartBlock + dwNumBlocks - 1));

	// Erase all the flash blocks.
#if 0
	if (!EraseBlocks(dwStartBlock, dwNumBlocks, dwFlags))
		return(FALSE);
#endif

	RETAILMSG(1,(TEXT("BP_LowLevelFormat: // Erase all the flash blocks.\r\n")));
	for (nBlockNum = dwStartBlock ; nBlockNum < (dwStartBlock + dwNumBlocks); nBlockNum++)
	{
		//RETAILMSG(1,(TEXT("BP_LowLevelFormat: // Erase all the flash blocks.-#\r\n")));
		if (!FMD_EraseBlock(nBlockNum))
		{
			return(FALSE);
		}
	}
	RETAILMSG(1,(TEXT("BP_LowLevelFormat: // Erase all the flash blocks.-End\r\n")));

	// Determine first good starting block
#if 0
	while (IS_BLOCK_UNUSABLE (dwStartBlock) && dwStartBlock < g_FlashInfo.dwNumBlocks) {
	dwStartBlock++;
	}

	if (dwStartBlock >= g_FlashInfo.dwNumBlocks) {
	RETAILMSG(1,(TEXT("BP_LowLevelFormat: no good blocks\r\n")));
	return FALSE;
	}
#endif

	// MBR goes in the first sector of the starting block.  This will be logical sector 0.
	g_dwMBRSectorNum = dwStartBlock * g_FlashInfo.wSectorsPerBlock;

	// Create an MBR.
	CreateMBR();

	RETAILMSG (1, (TEXT("Done.\r\n\r\n")));
	return(TRUE);
}

typedef struct
{
    GUID guidReserved;
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
    DWORD dwReserved4;
    DWORD dwReserved5;
    DWORD dwReserved6;
    DWORD dwReserved7;
    DWORD dwReserved8;
    DWORD dwReserved9;
    DWORD dwReserved10;
    DWORD dwUpdateModeFlag;

} IMGFS_BOOT_SECTOR, *PIMGFS_BOOT_SECTOR;


// --------------------------------------------------------------------
// --------------------------------------------------------------------
BOOL BP_ReadLogicalSector(HANDLE hPartition, DWORD dwSector, LPBYTE lpBuffer)
{
    static PVOID hFAL = NULL;
    PPARTSTATE pPartState = (PPARTSTATE) hPartition;
    if (INVALID_HANDLE_VALUE == hPartition) {
        return FALSE;
    }
    return FMD_ReadSector(dwSector + pPartState->pPartEntry->Part_StartSector, lpBuffer, NULL, 1);
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
BOOL BP_GetUpdateModeFlag(BOOL *pfUpdateMode)
{
    // sectors will be 4KB at the most
    static BYTE sector[0x1000];

    if (NULL == pfUpdateMode) {
        return FALSE;
    }

    // open the imgfs partition
    HANDLE hPartition = BP_OpenPartition(0, 0, PART_IMGFS, FALSE, PART_OPEN_EXISTING);
    if (INVALID_HANDLE_VALUE == hPartition) {
        // try again with fActive = TRUE
        hPartition = BP_OpenPartition(0, 0, PART_IMGFS, TRUE, PART_OPEN_EXISTING);
    }

    if (INVALID_HANDLE_VALUE == hPartition) {
        // there is probably no IMGFS partition
        RETAILMSG(1, (TEXT("BP_GetUpdateModeFlag: failed to open IMGFS partition\r\n")));
        return FALSE;
    }

    // read logical sector zero of the imgfs partition
    if (!BP_ReadLogicalSector(hPartition, 0, sector)) {
        RETAILMSG(1, (TEXT("BP_GetUpdateModeFlag: failed to read bootsector of IMGFS partition\r\n")));
        return FALSE;
    }

    *pfUpdateMode = ((PIMGFS_BOOT_SECTOR)sector)->dwUpdateModeFlag;
    RETAILMSG(1, (L"BP_GetUpdateModeFlag: fUpdateMode=%d\r\n", *pfUpdateMode));
    //if(*pfUpdateMode!=1) *pfUpdateMode=0;

    return TRUE;
}
