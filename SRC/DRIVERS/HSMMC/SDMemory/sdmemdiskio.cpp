//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

// Copyright (c) 2002 BSQUARE Corporation.  All rights reserved.
// DO NOT REMOVE --- BEGIN EXTERNALLY DEVELOPED SOURCE CODE ID 40973--- DO NOT REMOVE

// SD Memory Card driver disk IO implementation

#include "SDMemory.h"

///////////////////////////////////////////////////////////////////////////////
//  SDMemCalcDataAccessClocks   - Calculate the data access clocks
//  Input:  pMemCard            - the memcard
//  Output: pReadAccessClocks   - Pointer to ULONG for read access clocks
//          pWriteAccessClocks  - Pointer to ULONG for write access clocks
//  Return: TRUE or FALSE to indicate function success/failure
//  Notes: Calculate data access times for memory devices. This calculation
//         is to fine tune the data delay time
///////////////////////////////////////////////////////////////////////////////
BOOL SDMemCalcDataAccessClocks(PSD_MEMCARD_INFO pMemCard,
                               PULONG           pReadAccessClocks,
                               PULONG           pWriteAccessClocks)
{
    SD_CARD_INTERFACE cardInterface;    // current card interface
    SD_API_STATUS     status;           // intermediate status
    DOUBLE            clockPeriodNs;    // clock period in nano seconds
    ULONG             asyncClocks;      // clocks required for the async portion

        // fetch the card clock rate
    status = SDCardInfoQuery(pMemCard->hDevice,
                             SD_INFO_CARD_INTERFACE,
                             &cardInterface,
                             sizeof(cardInterface));

    if(!SD_API_SUCCESS(status)) {
        DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("SDMemCalcDataAccessClocks: Can't get card interface info\r\n")));
        return FALSE;
    }

    if(0 == cardInterface.ClockRate) {
        DEBUGCHK(FALSE);
        return FALSE;
    }

        // if the clock rate is greater than 1 Ghz, this won't work
    if(cardInterface.ClockRate > 1000000000) {
        DEBUGCHK(FALSE);
        return FALSE;
    }
        // calculate the clock period in nano seconds, clock rate is in Hz
    clockPeriodNs = 1000000000 / cardInterface.ClockRate;

        // calculate the async portion now that we know the clock rate
        // make asyncClock an integer
    asyncClocks = (ULONG)(pMemCard->CSDRegister.DataAccessTime.TAAC / clockPeriodNs);

        // add the async and synchronous portions together for the read access
    *pReadAccessClocks = asyncClocks + pMemCard->CSDRegister.DataAccessTime.NSAC;

        // for the write access the clocks area multiple of the read clocks
    *pWriteAccessClocks = (*pReadAccessClocks) * pMemCard->CSDRegister.WriteSpeedFactor;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemCalcDataAccessClocks: Tpd:%f ns, Asynch: %f ns, AsyncClocks:%d , SyncClocks: %d, ReadTotal: %d, Write Factor: %d WriteTotal: %d \n"),
        clockPeriodNs,
        pMemCard->CSDRegister.DataAccessTime.TAAC,
        asyncClocks,
        pMemCard->CSDRegister.DataAccessTime.NSAC,
        *pReadAccessClocks,
        pMemCard->CSDRegister.WriteSpeedFactor,
        *pWriteAccessClocks));

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemCardConfig  - Initialise the memcard structure and card itself
//  Input:  pMemCard - SD memory card structure
//  Output:
//  Return: win32 status code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemCardConfig( PSD_MEMCARD_INFO pMemCard )
{
    DWORD                   status = ERROR_SUCCESS; // intermediate win32 status
    DWORD                   dwSDHC;                 // high capacity value
    SD_API_STATUS           apiStatus;              // intermediate SD API status
    SD_CARD_INTERFACE       cardInterface;          // card interface
    SD_DATA_TRANSFER_CLOCKS dataTransferClocks;     // data transfer clocks

        // retrieve CID Register contents
    apiStatus = SDCardInfoQuery( pMemCard->hDevice,
                                 SD_INFO_REGISTER_CID,
                                 &(pMemCard->CIDRegister),
                                 sizeof(SD_PARSED_REGISTER_CID) );

    if(!SD_API_SUCCESS(apiStatus)) {
        DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("SDMemCardConfig: Can't read CID Register\r\n")));
        return ERROR_GEN_FAILURE;
    }

        // Retrieve CSD Register contents
    apiStatus = SDCardInfoQuery( pMemCard->hDevice,
                                 SD_INFO_REGISTER_CSD,
                                 &(pMemCard->CSDRegister),
                                 sizeof(SD_PARSED_REGISTER_CSD) );

    if(!SD_API_SUCCESS(apiStatus)) {
        DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("SDMemCardConfig: Can't read CSD Register\r\n")));
        return ERROR_GEN_FAILURE;
    }

        // retreive the card's RCA
    apiStatus = SDCardInfoQuery(pMemCard->hDevice,
                                SD_INFO_REGISTER_RCA,
                                &(pMemCard->RCA),
                                sizeof(pMemCard->RCA));

    if(!SD_API_SUCCESS(apiStatus)) {
        DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("SDMemCardConfig: Can't read RCA \r\n")));
        return ERROR_GEN_FAILURE;
    }

        // get write protect state
    apiStatus = SDCardInfoQuery( pMemCard->hDevice,
                                 SD_INFO_CARD_INTERFACE,
                                 &cardInterface,
                                 sizeof(cardInterface));

    if(!SD_API_SUCCESS(apiStatus)) {
        DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("SDMemCardConfig: Can't read Card Interface\r\n")));
        return ERROR_GEN_FAILURE;
    }

        // Get write protect state from Card Interface structure
    pMemCard->WriteProtected = cardInterface.WriteProtected;

    if( pMemCard->WriteProtected ) {
        DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemCardConfig: Card is write protected\r\n")));
    }

        // get capacity information
    apiStatus = SDCardInfoQuery( pMemCard->hDevice,
                                 SD_INFO_HIGH_CAPACITY_SUPPORT,
                                 &dwSDHC,
                                 sizeof(dwSDHC));

    if(!SD_API_SUCCESS(apiStatus)) {
        pMemCard->HighCapacity = FALSE;
    }
    else {
        pMemCard->HighCapacity = dwSDHC != 0;
    }

    if( pMemCard->HighCapacity ) {
        DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemCardConfig: Card is high capacity (2.0+)\r\n")));
    }

        // If the card doesn't support block reads, then fail
    if (!(pMemCard->CSDRegister.CardCommandClasses & SD_CSD_CCC_BLOCK_READ)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemCardConfig: Card does not support block read\r\n")));
        return ERROR_BAD_DEVICE;
    }
        // If the card doesn't support block writes, then mark the card as
        // write protected
    if (!(pMemCard->CSDRegister.CardCommandClasses & SD_CSD_CCC_BLOCK_WRITE)) {
        DEBUGMSG(SDCARD_ZONE_INIT || SDCARD_ZONE_WARN, (TEXT("SDMemCardConfig: Card does not support block write; mark as WP\r\n")));
        pMemCard->WriteProtected = TRUE;
    }

        // Calculate read and write data access clocks to fine tune access times
    if( !SDMemCalcDataAccessClocks( pMemCard, &dataTransferClocks.ReadClocks, &dataTransferClocks.WriteClocks) ) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemCardConfig: Unable to calculate data access clocks\r\n")));
        return ERROR_GEN_FAILURE;
    }
        // Call API to set the read and write data access clocks
    apiStatus = SDSetCardFeature( pMemCard->hDevice,
                                  SD_SET_DATA_TRANSFER_CLOCKS,
                                  &dataTransferClocks,
                                  sizeof(dataTransferClocks));

    if(!SD_API_SUCCESS(apiStatus)) {
        DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("SDMemCardConfig: Can't set data access clocks\r\n")));
        return ERROR_GEN_FAILURE;
    }

        // FATFS only supports 512 bytes per sector so set that
    pMemCard->DiskInfo.di_bytes_per_sect = SD_BLOCK_SIZE;

        // indicate that we aren't using Cylinder/Head/Sector addressing,
        // and that reads and writes are synchronous.
    pMemCard->DiskInfo.di_flags = DISK_INFO_FLAG_CHS_UNCERTAIN |
                                  DISK_INFO_FLAG_PAGEABLE;

        // since we aren't using C/H/S addressing we can set the counts of these
        // items to zero
    pMemCard->DiskInfo.di_cylinders = 0;
    pMemCard->DiskInfo.di_heads = 0;
    pMemCard->DiskInfo.di_sectors = 0;

        // Work out whether we have a Master Boot Record
    switch( pMemCard->CSDRegister.FileSystem ) {
        case SD_FS_FAT_PARTITION_TABLE:
                // yes, we have a MBR
            pMemCard->DiskInfo.di_flags |= DISK_INFO_FLAG_MBR;
            break;

        case SD_FS_FAT_NO_PARTITION_TABLE:
                // no, we don't have a MBR
            break;

        default:
                // ee don't do "Other" file systems
            DEBUGMSG( SDCARD_ZONE_ERROR, (TEXT("SDMemCardConfig: Card indicates unsupported file system (non FAT)\r\n")));
            return ERROR_GEN_FAILURE;
    }

        // calculate total number of sectors on the card
        //
        // NOTE:The bus is only using BLOCK units instead of BYTE units if
        // the device type is SD (not MMC) and if the CSDVersion is SD_CSD_VERSION_CODE_2_0.
        // Since we don't have access to the device type, we are checking to see if it is
        // a high definition card.  This should work for most cases.
        //
    if( pMemCard->CSDRegister.CSDVersion == SD_CSD_VERSION_CODE_2_0 &&
        pMemCard->HighCapacity ) {
        pMemCard->DiskInfo.di_total_sectors = pMemCard->CSDRegister.DeviceSize;
#ifdef _MMC_SPEC_42_
/**
 * Description : If MMC card is on SPEC42
 */
    } else if (pMemCard->CSDRegister.SpecVersion >= HSMMC_CSD_SPEC_VERSION_CODE_SUPPORTED )
    {
#ifdef _FOR_MOVI_NAND_
/**
 * Description : There is no way to distinguish between HSMMC and moviNAND.
 *               So, We assume that All HSMMC card is the moviNAND
 */
        pMemCard->IsHSMMC = TRUE;
#endif
        if( pMemCard->CSDRegister.SectorCount > 0)
        {
            DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("[SDMEM] This MMC card is on SPEC42.\n")));
            pMemCard->DiskInfo.di_total_sectors = pMemCard->CSDRegister.SectorCount;
            pMemCard->HighCapacity = TRUE;
        }
        else
        {
            pMemCard->DiskInfo.di_total_sectors = pMemCard->CSDRegister.DeviceSize/SD_BLOCK_SIZE;        
        }
#endif        
    } else {
        pMemCard->DiskInfo.di_total_sectors = pMemCard->CSDRegister.DeviceSize/SD_BLOCK_SIZE;
    }

        // FATFS and the SD Memory file spec only supports 512 byte sectors. An SD Memory
        // card will ALWAYS allow us to read and write in 512 byte blocks - but it might
        // be configured for a larger size initially. We set the size to 512 bytes here if needed.
    if( pMemCard->CSDRegister.MaxReadBlockLength != SD_BLOCK_SIZE ) {
            // Set block length to 512 bytes
        status = SDMemSetBlockLen( pMemCard, SD_BLOCK_SIZE );
    }

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemRead        - Read data from card into pSG scatter gather buffers
//  Input:  pMemCard - SD memory card structure
//          pSG      - Scatter Gather buffer structure from FATFS
//  Output:
//  Return: Status   - windows status code
//  Notes:  Reads from the card are split into groups of size TransferBlockSize
//          This is controlled by a registry entry for the driver.
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemRead(PSD_MEMCARD_INFO pMemCard, PSG_REQ pSG)
{
    DWORD  NumBlocks;
    DWORD  StartBlock;
    PUCHAR pBlockBuffer = NULL, pCardDataPtr = NULL;
    PUCHAR pSGBuffer = NULL;
    PUCHAR pSGBufferCursor = NULL;
    DWORD  status = ERROR_SUCCESS;
    DWORD  SGBufNum, SGBufLen, SGBufRemaining;
    DWORD  PartialStartBlock;
    DWORD  CardDataRemaining;

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +SDMemRead\r\n")));

    PREFAST_DEBUGCHK(pSG);

    // pSG is a sterile SG_REQ copy of the callers's SG_REQ; we can map the
    // embedded pointers back into it

    // validate the embedded sb_bufs
    for (ULONG ul = 0; ul < pSG->sr_num_sg; ul += 1) {
        if (
            (NULL == pSG->sr_sglist[ul].sb_buf) ||
            (0 == pSG->sr_sglist[ul].sb_len)
        ) {
            status = ERROR_INVALID_PARAMETER;
            goto statusReturn;
        }
    }

    // validate the I/O request
    if ((pSG->sr_start > pSG->sr_start + pSG->sr_num_sec)
        ||(pSG->sr_start + pSG->sr_num_sec) > pMemCard->DiskInfo.di_total_sectors) {
        status = ERROR_INVALID_PARAMETER;
        goto statusReturn;
    }

    // get number of sectors
    StartBlock = pSG->sr_start;
    NumBlocks = pSG->sr_num_sec;

    // cannot read more than 4GB at a time or SGBufLen will overflow
    if (ULONG_MAX / SD_BLOCK_SIZE < NumBlocks) {
        status = ERROR_INVALID_PARAMETER;
        goto statusReturn;
    }

    DEBUGMSG(SDMEM_ZONE_DISK_IO, (TEXT("SDMemRead: Reading blocks %d-%d\r\n"),
        StartBlock,
        StartBlock+NumBlocks-1));

    SGBufLen = 0;

    // calculate total buffer space of scatter gather buffers
    for (SGBufNum = 0; SGBufNum < pSG->sr_num_sg; SGBufNum++) {
        SGBufLen += pSG->sr_sglist[SGBufNum].sb_len;
    }

    // check total SG buffer space is enough for reqeusted transfer size
    if (SGBufLen < (NumBlocks * SD_BLOCK_SIZE)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemRead: SG Buffer space %d bytes less than block read size %d bytes\r\n"),
            SGBufLen,
            NumBlocks*SD_BLOCK_SIZE));
        status = ERROR_GEN_FAILURE;
        goto statusReturn;
    }

    // get block transfer buffer
    pBlockBuffer = (PUCHAR)SDAllocateFromMemList(pMemCard->hBufferList);

    // initialize some variables used in data copy
    SGBufNum = 0;
    SGBufRemaining = pSG->sr_sglist[SGBufNum].sb_len;

    // open the first sg buffer
    if (FAILED (CeOpenCallerBuffer (
                    (void**)&pSGBuffer,
                    pSG->sr_sglist[SGBufNum].sb_buf,
                    pSG->sr_sglist[SGBufNum].sb_len,
                    ARG_O_PTR,
                    FALSE
                    ))) {

        status = ERROR_INVALID_PARAMETER;
        goto statusReturn;
    }
    pSGBufferCursor = pSGBuffer;


    // split the reads into groups of TransferBlockSize in size to avoid
    // hogging the SD Bus with large reads
    for (PartialStartBlock = StartBlock; PartialStartBlock < StartBlock+NumBlocks; PartialStartBlock += pMemCard->BlockTransferSize) {

        // some variables just used for copying
        DWORD PartialTransferSize;
        DWORD CopySize;

        pCardDataPtr = pBlockBuffer;

        PartialTransferSize = MIN(
            pMemCard->BlockTransferSize,
            StartBlock+NumBlocks-PartialStartBlock);

        // read the data from SD Card
        status = SDMemReadMultiple(
            pMemCard,
            PartialStartBlock,
            PartialTransferSize,
            pBlockBuffer);

        if (status != ERROR_SUCCESS) {
            break;
        }

        // copy from pBlockArray to pSG buffers
        CardDataRemaining = PartialTransferSize*SD_BLOCK_SIZE;

        while (CardDataRemaining) {

            // get minimum of remaining size in SG buf and data left in pBlockBuffer
            CopySize = MIN(SGBufRemaining, CardDataRemaining);

            // copy that much data to SG buffer
            if (0 == CeSafeCopyMemory(pSGBufferCursor, pCardDataPtr, CopySize)) {
                status = ERROR_INVALID_PARAMETER;
                goto statusReturn;
            }

            // update pointers and counts
            pSGBufferCursor += CopySize;
            pCardDataPtr += CopySize;
            CardDataRemaining -= CopySize;
            SGBufRemaining -= CopySize;

            // fet the next SG Buffer if needed
            if (!SGBufRemaining && CardDataRemaining) {

                // close the current sg buffer
                VERIFY (SUCCEEDED (CeCloseCallerBuffer (
                                pSGBuffer,
                                pSG->sr_sglist[SGBufNum].sb_buf,
                                pSG->sr_sglist[SGBufNum].sb_len,
                                ARG_O_PTR
                                )));

                // open the next sg buffer
                SGBufNum++;
                SGBufRemaining = pSG->sr_sglist[SGBufNum].sb_len;
                if (FAILED (CeOpenCallerBuffer (
                                (void**)&pSGBuffer,
                                pSG->sr_sglist[SGBufNum].sb_buf,
                                pSG->sr_sglist[SGBufNum].sb_len,
                                ARG_O_PTR,
                                FALSE
                                ))) {
                    status = ERROR_INVALID_PARAMETER;
                    goto statusReturn;
                }
                pSGBufferCursor = pSGBuffer;
            }
        }
    }

    // close the final sg buffer
    VERIFY (SUCCEEDED (CeCloseCallerBuffer (
                    pSGBuffer,
                    pSG->sr_sglist[SGBufNum].sb_buf,
                    pSG->sr_sglist[SGBufNum].sb_len,
                    ARG_O_PTR
                    )));

statusReturn:

    // free the allocated block buffer
    if(pBlockBuffer) {
        SDFreeToMemList(pMemCard->hBufferList, pBlockBuffer);
    }

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: -SDMemRead\r\n")));

    // FATFS wants the status returned in the SG buffers also
    pSG->sr_status = status;

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemWrite       - Write data to card from pSG scatter gather buffers
//  Input:  pMemCard - SD memory card structure
//          pSG      - Scatter Gather buffer structure from FATFS
//  Output:
//  Return: Status   - windows status code
//  Notes:  Writes to the card are split into groups of size TransferBlockSize
//          This is controlled by a registry entry for the driver.
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemWrite( PSD_MEMCARD_INFO pMemCard, PSG_REQ pSG )
{
    DWORD  NumBlocks;
    DWORD  StartBlock;
    PUCHAR pBlockBuffer = NULL, pCardDataPtr = NULL;
    PUCHAR pSGBuffer = NULL;
    PUCHAR pSGBufferCursor = NULL;
    DWORD  status = ERROR_SUCCESS;
    DWORD  SGBufNum, SGBufLen, SGBufRemaining;
    DWORD  PartialStartBlock;
    DWORD  CardDataRemaining;

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +SDMemWrite\r\n")));

    PREFAST_DEBUGCHK(pSG);

    // pSG is a sterile SG_REQ copy of the callers's SG_REQ; we can map the
    // embedded pointers back into it

    // validate the embedded sb_bufs
    for (ULONG ul = 0; ul < pSG->sr_num_sg; ul += 1) {
        if (
            (NULL == pSG->sr_sglist[ul].sb_buf) ||
            (0 == pSG->sr_sglist[ul].sb_len)
        ) {
            status = ERROR_INVALID_PARAMETER;
            goto statusReturn;
        }
    }

    // validate the I/O request
    if ((pSG->sr_start > pSG->sr_start + pSG->sr_num_sec)
        ||(pSG->sr_start + pSG->sr_num_sec) > pMemCard->DiskInfo.di_total_sectors) {
        status = ERROR_INVALID_PARAMETER;
        goto statusReturn;
    }

    // check card write protect status
    if (pMemCard->WriteProtected) {
        DEBUGMSG(SDMEM_ZONE_DISK_IO, (TEXT("SDMemWrite: Card is write protected\r\n")));
        status = ERROR_WRITE_PROTECT;
        goto statusReturn;
    }

    // get number of sectors
    StartBlock = pSG->sr_start;
    NumBlocks = pSG->sr_num_sec;

    // cannot read more than 4GB at a time or SGBufLen will overflow
    if (ULONG_MAX / SD_BLOCK_SIZE < NumBlocks) {
        status = ERROR_INVALID_PARAMETER;
        goto statusReturn;
    }

    DEBUGMSG(SDMEM_ZONE_DISK_IO, (TEXT("SDMemWrite: Writing blocks %d-%d\r\n"),
        StartBlock,
        StartBlock+NumBlocks-1));

    // calculate total buffer space of scatter gather buffers
    SGBufLen = 0;
    for (SGBufNum = 0; SGBufNum < pSG->sr_num_sg; SGBufNum++) {
        SGBufLen += pSG->sr_sglist[SGBufNum].sb_len;
    }

    // check total SG buffer space is enough for reqeusted transfer size
    if(SGBufLen < NumBlocks * SD_BLOCK_SIZE) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemWrite: SG Buffer space %d bytes less than block write size %d bytes\r\n"),
            SGBufLen,
            NumBlocks * SD_BLOCK_SIZE));
        status = ERROR_GEN_FAILURE;
        goto statusReturn;
    }

    // get block transfer buffer
    pBlockBuffer = (PUCHAR)SDAllocateFromMemList(pMemCard->hBufferList);

    // initialize some variables used in data copy
    SGBufNum = 0;
    SGBufRemaining = pSG->sr_sglist[SGBufNum].sb_len;

    // open the first sg buffer
    if (FAILED (CeOpenCallerBuffer (
                    (void**)&pSGBuffer,
                    pSG->sr_sglist[SGBufNum].sb_buf,
                    pSG->sr_sglist[SGBufNum].sb_len,
                    ARG_I_PTR,
                    FALSE
                    ))) {

        status = ERROR_INVALID_PARAMETER;
        goto statusReturn;
    }
    pSGBufferCursor = pSGBuffer;

    // split the writes into groups of TransferBlockSize in size to avoid
    // hogging the SD Bus with large writes
    for(PartialStartBlock = StartBlock; PartialStartBlock < StartBlock+NumBlocks; PartialStartBlock += pMemCard->BlockTransferSize) {

        // some variables just used for copying
        DWORD PartialTransferSize;
        DWORD CopySize;

        pCardDataPtr = pBlockBuffer;

        PartialTransferSize = MIN(
            pMemCard->BlockTransferSize,
            StartBlock+NumBlocks-PartialStartBlock);

        // copy from pSG buffers to pBlockArray
        CardDataRemaining = PartialTransferSize*SD_BLOCK_SIZE;

        while (CardDataRemaining) {

            // get minimum of remaining size in SG buf and data left in pBlockBuffer
            CopySize = MIN(SGBufRemaining, CardDataRemaining);

            // copy that much data to block buffer
            if (0 == CeSafeCopyMemory(pCardDataPtr, pSGBufferCursor, CopySize)) {
                status = ERROR_INVALID_PARAMETER;
                goto statusReturn;
            }

            // update pointers and counts
            pSGBufferCursor += CopySize;
            pCardDataPtr += CopySize;
            CardDataRemaining -= CopySize;
            SGBufRemaining -= CopySize;

            // get the next SG Buffer if needed
            if (!SGBufRemaining && CardDataRemaining) {

                // close the current sg buffer
                VERIFY (SUCCEEDED (CeCloseCallerBuffer (
                                pSGBuffer,
                                pSG->sr_sglist[SGBufNum].sb_buf,
                                pSG->sr_sglist[SGBufNum].sb_len,
                                ARG_I_PTR
                                )));

                // open the next sg buffer
                SGBufNum++;
                SGBufRemaining = pSG->sr_sglist[SGBufNum].sb_len;
                if (FAILED (CeOpenCallerBuffer (
                                (void**)&pSGBuffer,
                                pSG->sr_sglist[SGBufNum].sb_buf,
                                pSG->sr_sglist[SGBufNum].sb_len,
                                ARG_I_PTR,
                                FALSE
                                ))) {
                    status = ERROR_INVALID_PARAMETER;
                    goto statusReturn;
                }
                pSGBufferCursor = pSGBuffer;
            }
        }

        // write the data to the SD Card
        status = SDMemWriteMultiple(
            pMemCard,
            PartialStartBlock,
            PartialTransferSize,
            pBlockBuffer);

        if (status != ERROR_SUCCESS) {
            break;
        }
    }

    // close the final sg buffer
    VERIFY (SUCCEEDED (CeCloseCallerBuffer (
                    pSGBuffer,
                    pSG->sr_sglist[SGBufNum].sb_buf,
                    pSG->sr_sglist[SGBufNum].sb_len,
                    ARG_I_PTR
                    )));

statusReturn:

    // free the allocated block buffer
    if (pBlockBuffer) {
        SDFreeToMemList(pMemCard->hBufferList, pBlockBuffer);
    }

    // FATFS wants the status returned in the SG buffers also
    pSG->sr_status = status;

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: -SDMemWrite\r\n")));

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemErase       - Erase a contiguous set of blocks
//  Input:  pMemCard - SD memory card structure
//          pDSI     - structure describing the range of sectors to erase
//  Output:
//  Return: Status   - windows status code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemErase( PSD_MEMCARD_INFO pMemCard, PDELETE_SECTOR_INFO pDSI )
{
    DWORD dwStatus = ERROR_NOT_SUPPORTED;

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +SDMemErase\n")));

    PREFAST_DEBUGCHK(pMemCard);
    PREFAST_DEBUGCHK(pDSI);

    // Validate Gather request
    if ((pDSI->startsector + pDSI->numsectors) > pMemCard->DiskInfo.di_total_sectors) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto statusReturn;
    }

    /*
    dwStatus = SDMemDoErase(
        pMemCard,
        (LONG) pDSI->startsector,
        (LONG) pDSI->numsectors
        );
    */

statusReturn:

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: -SDMemErase\n")));

    return dwStatus;
}

///////////////////////////////////////////////////////////////////////////////
//  RequestPrologue  - pre-tasks before handling the ioctl
//  Input:  pMemCard - memory card instance
//          DeviceIoControl - device iocontrol to filter
//  Output:
//  Return: SD_API_STATUS code
//  Notes:
//          This function should be called from the Ioctl dispatch function
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS RequestPrologue(PSD_MEMCARD_INFO pMemCard, DWORD DeviceIoControl)
{
    SD_API_STATUS status = SD_API_STATUS_SUCCESS; // intermediate status

    if (pMemCard->CardEjected) {
        return SD_API_STATUS_DEVICE_REMOVED;
    }

        // check and see if we need to do power management tasks
    if (!pMemCard->EnablePowerManagement) {
        return SD_API_STATUS_SUCCESS;
    }

        // pass power Ioctls through without issuing card re-select
    if ((IOCTL_POWER_CAPABILITIES == DeviceIoControl) ||
        (IOCTL_POWER_QUERY == DeviceIoControl) ||
        (IOCTL_POWER_SET == DeviceIoControl)) {
        return SD_API_STATUS_SUCCESS;
    }

        // for all other ioctls, re-select the card
    AcquireLock(pMemCard);

        // cancel the idle timer
    pMemCard->CancelIdleTimeout= TRUE;

        // check to see if the card was deselected due to power
        // management
    if (pMemCard->CardDeSelected) {
        DEBUGMSG(SDMEM_ZONE_POWER, (TEXT("SDMemory: Re-Selecting Card \n")));
        status = IssueCardSelectDeSelect(pMemCard, TRUE);
        if (SD_API_SUCCESS(status)) {
            pMemCard->CardDeSelected = FALSE;
        }
    }

    ReleaseLock(pMemCard);

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  RequestEnd  - post-tasks before handling the ioctl
//  Input:  pMemCard - memory card instance
//  Output:
//  Return:
//  Notes:
//          this function should be called from the Ioctl dispatch function
///////////////////////////////////////////////////////////////////////////////
void RequestEnd(PSD_MEMCARD_INFO pMemCard)
{
    AcquireLock(pMemCard);
        // clear the idle timer cancel flag after ioctl request is end
    pMemCard->CancelIdleTimeout= FALSE;

        ReleaseLock(pMemCard);
}

///////////////////////////////////////////////////////////////////////////////
//  IssueCardSelectDeSelect - issue card select
//  Input:  pMemCard - memory card instance
//          Select - select the card
//  Output:
//  Return: SD_API_STATUS code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS IssueCardSelectDeSelect(PSD_MEMCARD_INFO pMemCard, BOOL Select)
{
    USHORT              relativeAddress;    // relative address
    SD_RESPONSE_TYPE    responseType;       // expected response
    SD_API_STATUS       status;             // intermediate status
    int                 retryCount;         // retryCount;
    SD_CARD_STATUS      cardStatus;         // card status

    if (Select) {
            // using the cards original address selects the card again
        relativeAddress = pMemCard->RCA;
        DEBUG_CHECK((relativeAddress != 0), (TEXT("IssueCardSelectDeSelect - Relative address is zero! \n")));
            // the selected card should return a response
        responseType = ResponseR1b;
    } else {
            // address of zero deselects the card
        relativeAddress = 0;
            // according to the spec no response will be returned
        responseType = NoResponse;
    }

    retryCount = DEFAULT_DESELECT_RETRY_COUNT;

    while (retryCount) {

        status = SDSynchronousBusRequest(pMemCard->hDevice,
                                         SD_CMD_SELECT_DESELECT_CARD,
                                         ((DWORD)relativeAddress) << 16,
                                         SD_COMMAND,
                                         responseType,
                                         NULL,
                                         0,
                                         0,
                                         NULL,
                                         0);

        if (!SD_API_SUCCESS(status)) {
            break;
        }

        if (!Select) {
                // if we deselected, get the card status and check for
                // standby state
            status = SDCardInfoQuery(pMemCard->hDevice,
                                     SD_INFO_CARD_STATUS,
                                     &cardStatus,
                                     sizeof(SD_CARD_STATUS));

            if (!SD_API_SUCCESS(status)){
                break;
            }

            if (SD_STATUS_CURRENT_STATE(cardStatus) ==
                     SD_STATUS_CURRENT_STATE_STDBY) {
                DEBUGMSG(SDMEM_ZONE_POWER, (TEXT("SDMemory: Card now in Standby \n")));
                break;
            } else {

                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemory: Card not in standby! Card Status: 0x%08X \n")
                          , cardStatus));
                    // set unusuccessful for retry
                status = SD_API_STATUS_UNSUCCESSFUL;
            }


            retryCount--;

        } else {
            DEBUGMSG(SDMEM_ZONE_POWER, (TEXT("SDMemory: Card now in Transfer state \n")));
            break;
        }

    }

    return status;
}

///////////////////////////////////////////////////////////////////////////////
//  IdleThread - idle thread
//  Input:  pMemCard - memory card instance
//  Output:
//  Return: thread exit code
//  Notes:
//      This thread is created if power management is enabled.
//      Normally the thread lays dormant until signalled to perform
//      the idle timeout.  When an idle timeout occurs, if the
//      timer was not cancelled, the thread deselects the memory card
//      to reduce power consumption.
///////////////////////////////////////////////////////////////////////////////
DWORD IdleThread(PSD_MEMCARD_INFO pMemCard)
{
    while(1) {

            // wait for event
        WaitForSingleObject(pMemCard->hWakeUpIdleThread, INFINITE);

        if (pMemCard->ShutDownIdleThread) {
            return 0;
        }

            // while low power polling is enabled
        while (pMemCard->EnableLowPower) {

                // wait with a timeout
            WaitForSingleObject(pMemCard->hWakeUpIdleThread, pMemCard->IdleTimeout);

            if (pMemCard->EnableLowPower) {

                DEBUGMSG(SDMEM_ZONE_POWER, (TEXT("SDMemory: Idle Timeout Wakeup after %d MS \n"),pMemCard->IdleTimeout));

                AcquireLock(pMemCard);
                    // check for cancel
                if (!pMemCard->CancelIdleTimeout){
                        // make sure we haven't already deselected the card
                    if (!pMemCard->CardDeSelected) {
                        DEBUGMSG(SDMEM_ZONE_POWER, (TEXT("SDMemory: Idle Timout, De-Selecting Card \n")));
                            // we haven't been canceled, so issue the card de-select
                        if (SD_API_SUCCESS(IssueCardSelectDeSelect(pMemCard, FALSE))) {
                            pMemCard->CardDeSelected = TRUE;
                        }
                    }
                } else {
                    DEBUGMSG(SDMEM_ZONE_POWER, (TEXT("SDMemory: Idle Timout, Cancelled! \n")));
                }

                    // clear the cancel flag
                                    // SMC_IOControl() will clear pMemCard->CancelIdleTimeout once ioctl request is end

                ReleaseLock(pMemCard);
            }
        }

        if (pMemCard->ShutDownIdleThread) {
            return 0;
        }
    }

}

///////////////////////////////////////////////////////////////////////////////
//  HandleIoctlPowerSet
//  Input:  pMemCard - SD memory card structure
//          pDevicePowerState - device power state
//  Output:
//  Return:
//  Notes:
///////////////////////////////////////////////////////////////////////////////
VOID HandleIoctlPowerSet(PSD_MEMCARD_INFO       pMemCard,
                         PCEDEVICE_POWER_STATE  pDevicePowerState)
{
    AcquireLock(pMemCard);

    DEBUGMSG(SDMEM_ZONE_POWER, (TEXT("SDMemory: IOCTL_POWER_SET %d \n"),*pDevicePowerState));

    if (*pDevicePowerState < pMemCard->PowerStateForIdle)  {
            // everything above the power state for idle is treated as D0
        *pDevicePowerState = D0;
        pMemCard->CurrentPowerState = D0;
            // disable low power operation
        pMemCard->EnableLowPower = FALSE;

    } else {
            // everything above the IDLE power state is set to IDLE
        *pDevicePowerState = pMemCard->PowerStateForIdle;
        pMemCard->CurrentPowerState = pMemCard->PowerStateForIdle;
            // enable low power operation
        pMemCard->EnableLowPower = TRUE;
            // wake up the idle thread to go into power idle polling
        SetEvent(pMemCard->hWakeUpIdleThread);
    }

    ReleaseLock(pMemCard);

}

///////////////////////////////////////////////////////////////////////////////
//  InitializePowerManagement - initialize power management feature
//  Input:  pMemCard - SD memory card structure
//  Output:
//  Return:
//  Notes:
///////////////////////////////////////////////////////////////////////////////
VOID InitializePowerManagement(PSD_MEMCARD_INFO pMemCard)
{
    DWORD threadID;     // idle thread ID

    if (!pMemCard->EnablePowerManagement) {
        DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: Power Management Disabled\n")));
        return;
    }

    pMemCard->EnableLowPower  = FALSE;
    pMemCard->CurrentPowerState = D0;
    pMemCard->ShutDownIdleThread = FALSE;
    pMemCard->CancelIdleTimeout = FALSE;
    pMemCard->CardDeSelected = FALSE;

        // create the wake up event
    pMemCard->hWakeUpIdleThread = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (NULL == pMemCard->hWakeUpIdleThread) {
        pMemCard->EnablePowerManagement = FALSE;
        return;
    }

        // create the thread
    pMemCard->hIdleThread = CreateThread(NULL,
                                         0,
                                         (LPTHREAD_START_ROUTINE)IdleThread,
                                         pMemCard,
                                         0,
                                         &threadID);

    if (NULL == pMemCard->hIdleThread) {
        pMemCard->EnablePowerManagement = FALSE;
        return;
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: Power Management Setup complete \n")));

}


///////////////////////////////////////////////////////////////////////////////
//  DeinitializePowerManagent - clean up power management feature
//  Input:  pMemCard - SD memory card structure
//  Output:
//  Return:
//  Notes:
///////////////////////////////////////////////////////////////////////////////
VOID DeinitializePowerManagement(PSD_MEMCARD_INFO pMemCard)
{
    pMemCard->ShutDownIdleThread = TRUE;
    pMemCard->EnableLowPower = FALSE;
    pMemCard->CancelIdleTimeout = TRUE;

    if (NULL != pMemCard->hIdleThread) {
        SetEvent(pMemCard->hWakeUpIdleThread);
        WaitForSingleObject(pMemCard->hIdleThread, INFINITE);
        CloseHandle(pMemCard->hIdleThread);
        pMemCard->hIdleThread = NULL;
    }

    if (NULL != pMemCard->hWakeUpIdleThread) {
        CloseHandle(pMemCard->hWakeUpIdleThread);
        pMemCard->hWakeUpIdleThread = NULL;
    }

}

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

