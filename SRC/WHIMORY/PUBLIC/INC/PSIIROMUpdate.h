/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : PSIIROMUpdate.h                                                 */
/* PURPOSE : This file contains Flash interface that the OAL must expose     */
/*           for Microsoft Mobile Devices                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*               COPYRIGHT 2003 SAMSUNG ELECTRONICS CO., LTD.                */
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
/* - 04/04/2003 [SongHo Yoon]: first writing                                 */
/*                                                                           */
/*****************************************************************************/

/*++

Copyright (c) Microsoft Corporation

Module Name:

    PSIIROMUpdate.h

Abstract:

    Flash interface that the OAL must expose
    for Microsoft Mobile Devices

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// **************************************************************************
//
// Function Name: FlashInit
// 
// Purpose: initializes or deinitializes resources for flash routines
//
// Arguments:
//    IN    bInit   TRUE- initialize, FALSE- deinitialize
//
// Return Values:
//
//      ERROR_SUCCESS
//      ERROR_OUTOFMEMORY   can't allocate memory for flash functions
//      
// Description:  
//    
//  Initialization required to write to flash.
//
//  For example, you may have routines that need to be relocated to RAM
//  in order to write the flash device.
//
//  init is platform dependent.
//

DWORD
WINAPI
FlashInit(
            BOOL    bInit
         );


// **************************************************************************
//
// Function Name: FlashWrite
// 
// Purpose: writes data to flash memory
//
// Arguments:
//    IN    pvDestAddr      dest address in flash
//    IN    dwLength        amount of data to write
//    IN    pvSrcAddr       src address of data
//
// Return Values:
//    
//    ERROR_SUCCESS
//    ERROR_NOT_READY   FlashInit(TRUE) wasn't called
//    ERROR_INVALID_PARAMETER   data not aligned or bad pointer
//    ERROR_INVALID_DATA   data verification check failed
//    non-zero error from flash device, indicates failure XX
//
//

DWORD
WINAPI
FlashWrite(
            LPVOID  pvDestAddr,
            LPVOID  pvSrcAddr,
            DWORD   dwLength
          );
          
// **************************************************************************
//
// Function Name: FlashRead
// 
// Purpose: reads data from flash memory
//
// Arguments:
//    IN    pvDestAddr      dest address in flash
//    IN    dwLength        amount of data to write
//    IN    pvSrcAddr       src address of data
//
// Return Values:
//    
//    ERROR_SUCCESS
//    ERROR_NOT_READY   FlashInit(TRUE) wasn't called
//    ERROR_INVALID_PARAMETER   data not aligned or bad pointer
//    ERROR_INVALID_DATA   data verification check failed
//    non-zero error from flash device, indicates failure XX
//
// added by SHYoon. 10-MAR-2003

DWORD
WINAPI
FlashRead(
            LPVOID  pvDestAddr,
            LPVOID  pvSrcAddr,
            DWORD   dwLength
          );

// **************************************************************************
//
// Function Name: FlashErase
// 
// Purpose: erases blocks of flash memory
//
// Arguments:
//    IN    pvStartAddr     start address to erase
//    IN    dwLength        length of region to erase
//
// Return Values:
//    
//    ERROR_SUCCESS
//    ERROR_NOT_READY   FlashInit(TRUE) was not called
//
// Description:
//
//  Erases flash blocks containing the specified region
//
//

DWORD
WINAPI
FlashErase(
            LPVOID  pvStartAddr,
            DWORD   dwLength
          );



// **************************************************************************
//
// Function Name: FlashGetBlockInfo
// 
// Purpose: returns block address and length for a specified address
//
// Arguments:
//    IN    pvStartAddr     address to get block info for
//    OUT   dwBlockStart    start address of block the address is in
//    OUT   dwBlockLen      length of block the address is in
//
// Return Values:
//    
//    ERROR_SUCCESS
//    ERROR_NOT_READY   can't get flash params
//
//

DWORD
WINAPI
FlashGetBlockInfo(
                    LPVOID  pvStartAddr, 
                    DWORD * pdwBlockStart,
                    DWORD * pdwBlockLen
                 ) ;

#ifdef __cplusplus
}
#endif
