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

// Header file for the SD Memory Card driver  

#ifndef _SD_MEMORY_H_
#define _SD_MEMORY_H_

#include "../HSMMCCh1/s3c6410_hsmmc_lib/SDCardDDK.h"
#include <diskio.h>

    // need the following for POST_INIT definition
#include <cardserv.h>
#include <cardapi.h>
#include <tuple.h>
#include <devload.h>
    //////////////////////////////////////////////

#include <pm.h>
#include <storemgr.h>
#include <winnt.h>

    // some debug zones
#define SDMEM_ZONE_DISK_IO               SDCARD_ZONE_0
#define ENABLE_SDBUS_ZONE_DISK_IO        ZONE_ENABLE_0
#define SDMEM_ZONE_CARD_IO               SDCARD_ZONE_1
#define ENABLE_SDBUS_ZONE_CARD_IO        ZONE_ENABLE_1
#define SDMEM_ZONE_BUS_REQS              SDCARD_ZONE_2
#define ENABLE_SDBUS_ZONE_BUS_REQS       ZONE_ENABLE_2
#define SDMEM_ZONE_POWER                 SDCARD_ZONE_3
#define ENABLE_ZONE_POWER                ZONE_ENABLE_3

#define SD_MEMORY_TAG 'SDMC'

#define SD_BLOCK_SIZE  512
#define DEFAULT_BLOCK_TRANSFER_SIZE 8

//
// Registry configuration values
//

#define BLOCK_TRANSFER_SIZE_KEY TEXT("BlockTransferSize")
#define SINGLE_BLOCK_WRITES_KEY TEXT("SingleBlockWrites")
#define DISABLE_POWER_MANAGEMENT TEXT("DisablePowerManagement")
#define IDLE_TIMEOUT TEXT("IdleTimeout")
#define IDLE_POWER_STATE TEXT("IdlePowerState")

#define DEFAULT_IDLE_TIMEOUT 2000           // 2 seconds and we suspend the card
#define DEFAULT_DESELECT_RETRY_COUNT 3

    // size of manufacturer ID and serial number as ASCII strings
    // 2 chars for manufacturer ID, 8 for serial number + a nul char for each string
#define SD_SIZEOF_STORAGE_ID    12

typedef struct {
  SD_DEVICE_HANDLE       hDevice;
  SD_MEMORY_LIST_HANDLE  hBufferList;
  PWSTR                  pRegPath;
  DWORD                  BlockTransferSize;   // Maximum block transfer size, set by registry key
  DISK_INFO              DiskInfo;            // for DISK_IOCTL_GET/SETINFO
  SD_PARSED_REGISTER_CID CIDRegister;
  SD_PARSED_REGISTER_CSD CSDRegister;
  SD_CARD_RCA            RCA;                 // relative card address
  BOOL                   SingleBlockWrites;
  BOOL                   WriteProtected;
  BOOL                   HighCapacity;
  CRITICAL_SECTION       RemovalLock;         // removal lock critical section
  CRITICAL_SECTION       CriticalSection;
  BOOL                   CardEjected;         // card has been ejected
  BOOL                   EnablePowerManagement;   // power management on
  BOOL                   EnableLowPower;          // enable low power operation
  CEDEVICE_POWER_STATE   CurrentPowerState;       // current power state
  CEDEVICE_POWER_STATE   PowerStateForIdle;       // power state for idle 
  BOOL                   ShutDownIdleThread;      // flag to kill idle thread
  BOOL                   CancelIdleTimeout;       // cancel the idle timeout
  DWORD                  IdleTimeout;             // idle timeout value
  BOOL                   CardDeSelected;          // card has been deselected
  HANDLE                 hWakeUpIdleThread;       // wake up the idle thread to start the timer
  HANDLE                 hIdleThread;             // idle thread
  PSG_REQ                pSterileIoRequest;
  BOOL                   fPreDeinitCalled;
#ifdef _FOR_MOVI_NAND_
  /**
   * Description : There is no way to distinguish between HSMMC and moviNAND.
   *               So, We assume A HSMMC card is  moviNAND
   */
  BOOL                    IsHSMMC;
#endif
} SD_MEMCARD_INFO, *PSD_MEMCARD_INFO;

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define AcquireLock(pDevice) EnterCriticalSection(&(pDevice)->CriticalSection)
#define ReleaseLock(pDevice) LeaveCriticalSection(&(pDevice)->CriticalSection)

#define AcquireRemovalLock(pDevice) EnterCriticalSection(&(pDevice)->RemovalLock)
#define ReleaseRemovalLock(pDevice) LeaveCriticalSection(&(pDevice)->RemovalLock)

//
// SDDiskIO
//

    //  SDMemCardConfig    - Initialise the memcard structure and card itself
DWORD SDMemCardConfig( PSD_MEMCARD_INFO pMemCard );

    //  SDMemRead          - Read data from card into pSG scatter gather buffers
DWORD SDMemRead( PSD_MEMCARD_INFO pMemCard, PSG_REQ pSG );

    //  SDMemWrite         - Write data to card from pSG scatter gather buffers
DWORD SDMemWrite( PSD_MEMCARD_INFO pMemCard, PSG_REQ pSG );

    //  SDMemErase         - Erase a contiguous set of blocks
DWORD SDMemErase( PSD_MEMCARD_INFO pMemCard, PDELETE_SECTOR_INFO pDSI );

//
// SDCardIO
//

    //  SDMemDoBusRequest  - Perform a bus request, returns Windows Status
DWORD SDMemDoBusRequest( PSD_MEMCARD_INFO  pMemcard,
                         UCHAR             Command,
                         DWORD             Argument,
                         SD_TRANSFER_CLASS TransferClass,
                         SD_RESPONSE_TYPE  ResponseType,
                         ULONG             NumBlocks,
                         ULONG             BlockSize,
                         PUCHAR            pBuffer,
                         DWORD             Flags);

    //  SDMemSetBlockLen   - Sets read/write block length for SD memory card
DWORD SDMemSetBlockLen( PSD_MEMCARD_INFO pMemcard, 
                        DWORD            BlockLen );

    //  SDMemReadMultiple  - Read multiple 512 byte blocks of data from card
DWORD SDMemReadMultiple( PSD_MEMCARD_INFO pHandle,
                         ULONG            StartBlock,
                         ULONG            NumBlocks,
                         PUCHAR           pBuffer );

    //  SDMemWriteMultiple - Write multiple 512 byte blocks of data to card
DWORD SDMemWriteMultiple( PSD_MEMCARD_INFO pHandle,
                          LONG             StartBlock,
                          LONG             NumBlocks,
                          PUCHAR           pBuffer );

    //  SDMemWriteUsingSingleBlocks - Write using single block writes
DWORD SDMemWriteUsingSingleBlocks( PSD_MEMCARD_INFO pHandle,
                                   LONG             StartBlock,
                                   LONG             NumBlocks,
                                   PUCHAR           pBuffer );

DWORD SDAPIStatusToErrorCode( SD_API_STATUS Status );

DWORD SDGetCardStatus(PSD_MEMCARD_INFO pMemCard , SD_CARD_STATUS *pCardStatus);

VOID HandleIoctlPowerSet(PSD_MEMCARD_INFO       pMemCard, 
                         PCEDEVICE_POWER_STATE  pDevicePowerState);
VOID InitializePowerManagement(PSD_MEMCARD_INFO pMemCard);
VOID DeinitializePowerManagement(PSD_MEMCARD_INFO pMemCard);
SD_API_STATUS IssueCardSelectDeSelect(PSD_MEMCARD_INFO pMemCard, BOOL Select);
VOID RequestEnd(PSD_MEMCARD_INFO pMemCard);

SD_API_STATUS RequestPrologue(PSD_MEMCARD_INFO pMemCard, DWORD DeviceIoControl);


#endif // _SD_MEMORY_H_

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

