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

// SD Memory driver SD card I/O routines

#include "SDMemory.h"


///////////////////////////////////////////////////////////////////////////////
//  SDAPIStatusToErrorCode - Convert SD API status code to windows status code
//  Input:  Status         - SD API Status code
//  Output:
//  Return:                - Windows status/error code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDAPIStatusToErrorCode( SD_API_STATUS Status )
{
    switch( Status ) {
        case SD_API_STATUS_SUCCESS:                 return ERROR_SUCCESS;
        case SD_API_STATUS_BUFFER_OVERFLOW:         return ERROR_GEN_FAILURE;       
        case SD_API_STATUS_DEVICE_BUSY:             return ERROR_BUSY;
        case SD_API_STATUS_UNSUCCESSFUL:            return ERROR_GEN_FAILURE;
        case SD_API_STATUS_NOT_IMPLEMENTED:         return ERROR_GEN_FAILURE; 
        case SD_API_STATUS_ACCESS_VIOLATION:        return ERROR_GEN_FAILURE; 
        case SD_API_STATUS_INVALID_HANDLE:          return ERROR_GEN_FAILURE; 
        case SD_API_STATUS_INVALID_PARAMETER:       return ERROR_GEN_FAILURE; 
        case SD_API_STATUS_NO_SUCH_DEVICE:          return ERROR_DEV_NOT_EXIST;
        case SD_API_STATUS_INVALID_DEVICE_REQUEST:  return ERROR_GEN_FAILURE;
        case SD_API_STATUS_NO_MEMORY:               return ERROR_NOT_ENOUGH_MEMORY;
        case SD_API_STATUS_BUS_DRIVER_NOT_READY:    return ERROR_GEN_FAILURE;
        case SD_API_STATUS_DATA_ERROR:              return ERROR_GEN_FAILURE;
        case SD_API_STATUS_CRC_ERROR:               return ERROR_CRC;
        case SD_API_STATUS_INSUFFICIENT_RESOURCES:  return ERROR_GEN_FAILURE;
        case SD_API_STATUS_DEVICE_NOT_CONNECTED:    return ERROR_DEVICE_NOT_CONNECTED;
        case SD_API_STATUS_DEVICE_REMOVED:          return ERROR_DEVICE_REMOVED;
        case SD_API_STATUS_DEVICE_NOT_RESPONDING:   return ERROR_GEN_FAILURE;
        case SD_API_STATUS_CANCELED:                return ERROR_GEN_FAILURE;
        default:                                    return ERROR_GEN_FAILURE;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  SDStatusToErrorCode - Convert SD card status code to windows status code
//  Input:  Status      - SD card Status code
//  Output:
//  Return:             - Windows status/error code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
static DWORD SDStatusToErrorCode( DWORD Status )
{
        // mask off the state bits to leave just errors
    Status &= SD_STATUS_ERROR_MASK;

    if (!Status) return ERROR_SUCCESS;
     
    DEBUGMSG( SDCARD_ZONE_ERROR, (TEXT("SDStatusToErrorCode: Card Status Error 0x%08X \r\n"),Status));
        
    if (Status & SD_STATUS_COM_CRC_ERROR) return ERROR_CRC;
    if (Status & SD_STATUS_WP_VIOLATION)  return ERROR_WRITE_PROTECT;
    
        // the rest just get reported as a general failure
    return ERROR_GEN_FAILURE;
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemDoBusRequest     - Perform a bus request, returns Windows Status
//  Input:  pMemCard      - SD memory card structure
//          Command       - SD command to send over bus
//          Argument      - 32 bit argument specific to the command
//          TransferClass - Command only, or associated with read/write data
//          ResponseType  - Response Type for the command
//          NumBlocks     - Number of data blocks in pBlockArray, can be zero
//                          if transfer class is not read or write
//          BlockSize     - Size of data blocks in pBlockArray. All blocks
//                          must be same size.
//          pBuffer       - Pointer to buffer containing BlockSize*NumBlocks bytes
//          Flags
//  Output:
//  Return: standard win32 status code
//  Notes:  This function performs an SD Bus Request and converts the SD status
//          of that transaction into a standard windows status code.
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemDoBusRequest( PSD_MEMCARD_INFO  pMemcard,
                         UCHAR             Command,
                         DWORD             Argument,
                         SD_TRANSFER_CLASS TransferClass,
                         SD_RESPONSE_TYPE  ResponseType,
                         ULONG             NumBlocks,
                         ULONG             BlockSize,
                         PUCHAR            pBuffer,
                         DWORD             Flags)
{
    SD_API_STATUS RequestStatus;    // intermediate status

    DEBUGMSG( SDMEM_ZONE_BUS_REQS, 
        (TEXT("SDMemDoBusRequest: CMD%d Arg 0x%08X TransferClass %d NumBlocks %d BlockSize %d\r\n"),
        Command,Argument,TransferClass,NumBlocks,BlockSize));

        // initiate the bus transaction
    RequestStatus = SDSynchronousBusRequest( pMemcard->hDevice,
                                             Command,
                                             Argument,
                                             TransferClass,
                                             ResponseType,
                                             NULL,
                                             NumBlocks,
                                             BlockSize,
                                             pBuffer,
                                             Flags);

        // get the status and convert if necessary
    if (!SD_API_SUCCESS(RequestStatus)) {
        DEBUGMSG( SDCARD_ZONE_ERROR, (TEXT("SDMemDoBusRequest Failed: CMD%d returned API status 0x%X\r\n"),
                   Command,RequestStatus));
        return SDAPIStatusToErrorCode(RequestStatus);
    }

        // everything was OK, return success
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemSetBlockLen - Sets read/write block length for SD memory card
//  Input:  pMemCard - SD memory card structure
//          BlockLen - New read/write block length
//  Output:
//  Return: win32 status code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemSetBlockLen(PSD_MEMCARD_INFO pMemcard, 
                       DWORD            BlockLen)
{
    DEBUGMSG(SDMEM_ZONE_CARD_IO, (TEXT("SDMemSetBlockLen: Setting block length to %d bytes\r\n"),BlockLen));
    
        // issue a Set Block Length command
    return SDMemDoBusRequest( pMemcard,
                              SD_CMD_SET_BLOCKLEN,
                              BlockLen,
                              SD_COMMAND,
                              ResponseR1,
                              0,
                              0,
                              NULL,
                              0);
}

///////////////////////////////////////////////////////////////////////////////
//  SDGetCardStatus  - Get the current card status
//  Input:  pMemCard   - SD memory card structure
//          
//  Output: pCardStatus - card status
//  Return: win32 status code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDGetCardStatus(PSD_MEMCARD_INFO pMemCard , SD_CARD_STATUS *pCardStatus)
{
    SD_API_STATUS   status;         // api status

    status = SDCardInfoQuery(pMemCard->hDevice,
                             SD_INFO_CARD_STATUS,
                             pCardStatus,
                             sizeof(SD_CARD_STATUS));

    if (!SD_API_SUCCESS(status)){
        return SDAPIStatusToErrorCode(status);
    }

    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemReadMultiple  - Read multiple 512 byte blocks of data from card
//  Input:  pMemCard    - SD memory card info
//          StartBlock - Starting 512 byte block for read
//          NumBlocks  - Number of blocks to read
//          pBuffer    - Pointer to buffer for read data
//  Output:
//  Return: Win32 status code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemReadMultiple( PSD_MEMCARD_INFO pMemCard,
    ULONG            StartBlock,
    ULONG            NumBlocks,
    PUCHAR           pBuffer )
{
  DEBUGMSG( SDMEM_ZONE_CARD_IO, (TEXT("SDMemReadMultiple: Reading blocks %d-%d\r\n"), 
        StartBlock, StartBlock+NumBlocks-1));
  // high capacity cards just take the block offset.  standard cards
  // take a byte offset.
  if (!pMemCard->HighCapacity) {
    if (ULONG_MAX / SD_BLOCK_SIZE < StartBlock) {
      ASSERT(FALSE);
      return ERROR_INVALID_PARAMETER;
    }

    StartBlock *= SD_BLOCK_SIZE;
  }

  // perform block read request
  if (NumBlocks == 1) {
    // for single blocks 
    return SDMemDoBusRequest( pMemCard,
        SD_CMD_READ_SINGLE_BLOCK,
        StartBlock,
        SD_READ,
        ResponseR1,
        NumBlocks,
        SD_BLOCK_SIZE,
        pBuffer,
        0);  

  }
  // multi-block writes use the optimized command
  // we have to auto issue a stop command though
#ifdef _FOR_MOVI_NAND_
  /**
   * Description : To use Pre-Define. Set the flag as
   *               for moviANND. If you do not use moviNAND
   *               Do NOT define _FOR_MOVI_NAND_
   */
  if ( pMemCard->IsHSMMC == TRUE )
  {
    return SDMemDoBusRequest( pMemCard,
        SD_CMD_READ_MULTIPLE_BLOCK,
        StartBlock,
        SD_READ,
        ResponseR1,
        NumBlocks,
        SD_BLOCK_SIZE,
        pBuffer,
        SD_MOVINAND_PRE_DEFINE);     
  }
#endif
  return SDMemDoBusRequest( pMemCard,
      SD_CMD_READ_MULTIPLE_BLOCK,
      StartBlock,
      SD_READ,
      ResponseR1,
      NumBlocks,
      SD_BLOCK_SIZE,
      pBuffer,
      SD_AUTO_ISSUE_CMD12);  // auto issue CMD12
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemWriteMultiple  - Write multiple 512 byte blocks of data to card
//  Input:  pMemCard    - SD memory card structure
//          StartBlock  - Starting 512 byte block for write
//          NumBlocks   - Number of blocks to write
//          pBuffer     - Pointer to buffer containing write data
//  Output:
//  Return:  win32 status code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemWriteMultiple( PSD_MEMCARD_INFO pMemCard,
    LONG             StartBlock,
    LONG             NumBlocks,
    PUCHAR           pBuffer )
{

  DEBUGMSG(SDMEM_ZONE_CARD_IO, (TEXT("SDMemWriteMultiple: Writing blocks %d-%d\r\n"), StartBlock, StartBlock+NumBlocks-1));
  
#ifdef _FOR_MOVI_NAND_
  /**
   * Description : moviNAND dose "not" support Single-Block write
   */
  if ( pMemCard->IsHSMMC == TRUE )
  {    
  }
  else
#endif 
  {
    if( pMemCard->SingleBlockWrites || (NumBlocks == 1)) {
      DEBUGMSG(SDMEM_ZONE_CARD_IO, (TEXT("SDMemWriteMultiple: Sending request to SDMemWriteUsingSingleBlocks\r\n")));

      return SDMemWriteUsingSingleBlocks( pMemCard,
          StartBlock,
          NumBlocks,
          pBuffer );
    }
  }

  // high capacity cards just take the block offset.  standard cards
  // take a byte offset.
  if (!pMemCard->HighCapacity) {
    if (ULONG_MAX / SD_BLOCK_SIZE < StartBlock) {
      ASSERT(FALSE);
      return ERROR_INVALID_PARAMETER;
    }

    StartBlock *= SD_BLOCK_SIZE;
  }

  // issue multi-block write request
#ifdef _FOR_MOVI_NAND_
  /**
   * Description : To use Pre-Define. Set the flag as
   *               for moviANND. If you do not use moviNAND Do NOT define _FOR_MOVI_NAND_
   */
  if ( pMemCard->IsHSMMC == TRUE )
  {    
    return SDMemDoBusRequest( pMemCard,
        SD_CMD_WRITE_MULTIPLE_BLOCK,
        StartBlock,
        SD_WRITE,
        ResponseR1,
        NumBlocks,
        SD_BLOCK_SIZE,
        pBuffer,
        SD_MOVINAND_PRE_DEFINE);
  }
#endif 

  return SDMemDoBusRequest( pMemCard,
      SD_CMD_WRITE_MULTIPLE_BLOCK,
      StartBlock,
      SD_WRITE,
      ResponseR1,
      NumBlocks,
      SD_BLOCK_SIZE,
      pBuffer,
      SD_AUTO_ISSUE_CMD12);
}

///////////////////////////////////////////////////////////////////////////////
//  SDMemWriteUsingSingleBlocks  - Write using only single block commands
//  Input:  pMemCard    - SD memory card structure
//          StartBlock  - Starting 512 byte block for write
//          NumBlocks   - Number of blocks to write
//          pBuffer     - Pointer to buffer containing write data
//  Output:
//  Return: win32 status code
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD SDMemWriteUsingSingleBlocks( PSD_MEMCARD_INFO pMemCard,
                                   LONG             StartBlock,
                                   LONG             NumBlocks,
                                   PUCHAR           pBuffer )
{
    DWORD status = SD_API_STATUS_SUCCESS;   // intermediate win32 status
    LONG  block;    // block count

    if (StartBlock + NumBlocks < StartBlock) {
        // Check for arithmetic overflow
        return ERROR_INVALID_PARAMETER;
    }

    if (!pMemCard->HighCapacity) {
        if (ULONG_MAX / SD_BLOCK_SIZE < StartBlock + NumBlocks) {
            return ERROR_INVALID_PARAMETER;
        }
    }

        // Split write operation into single blocks
    for( block=0; block<NumBlocks; block++ ) {
        DEBUGMSG(SDMEM_ZONE_CARD_IO, (TEXT("SDMemWriteUsingSingleBlocks: Writing block %d\r\n"), StartBlock+block));

        DWORD dwOffset = StartBlock + block;
        if (!pMemCard->HighCapacity) {
            dwOffset *= SD_BLOCK_SIZE;
        }
        
            // issue single block write request
        status = SDMemDoBusRequest( pMemCard,
                                    SD_CMD_WRITE_BLOCK,
                                    dwOffset,
                                    SD_WRITE,
                                    ResponseR1,
                                    1,
                                    SD_BLOCK_SIZE,
                                    pBuffer + (block*SD_BLOCK_SIZE),
                                    0);
        
        if( ERROR_SUCCESS != status )
            break;
    }

    return status;
}

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE
