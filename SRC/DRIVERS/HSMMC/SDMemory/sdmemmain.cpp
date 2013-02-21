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

// Driver entry points for SD Memory Card client driver

#include <windows.h>
#include "SDMemory.h"
#include <ceddk.h>

    // initialize debug zones
SD_DEBUG_INSTANTIATE_ZONES(
     TEXT("SDMemory"), // module name
     ZONE_ENABLE_INIT | ZONE_ENABLE_ERROR | ZONE_ENABLE_WARN,   // initial settings
     TEXT("Disk I/O"),
     TEXT("Card I/O"),
     TEXT("Bus Requests"),
     TEXT("Power"),
     TEXT(""),
     TEXT(""),
     TEXT(""),
     TEXT(""),
     TEXT(""),
     TEXT(""),
     TEXT(""));

DWORD SetDiskInfo( PSD_MEMCARD_INFO, PDISK_INFO );
DWORD GetDiskInfo( PSD_MEMCARD_INFO, PDISK_INFO );
DWORD GetStorageID( PSD_MEMCARD_INFO, PSTORAGE_IDENTIFICATION, DWORD, DWORD* );
BOOL GetDeviceInfo(PSD_MEMCARD_INFO pMemCard, PSTORAGEDEVICEINFO pStorageInfo);

#define DEFAULT_MEMORY_TAGS 4

///////////////////////////////////////////////////////////////////////////////
//  DllEntry - the main dll entry point
//  Input:  hInstance - the instance that is attaching
//          Reason - the reason for attaching
//          pReserved - not much
//  Output:
//  Return: always returns TRUE
//  Notes:  this is only used to initialize the zones
///////////////////////////////////////////////////////////////////////////////
extern "C" BOOL WINAPI DllEntry(HINSTANCE hInstance, ULONG Reason, LPVOID pReserved)
{
    BOOL fRet = TRUE;

    if ( Reason == DLL_PROCESS_ATTACH ) {
        DisableThreadLibraryCalls((HMODULE) hInstance);
        if (!SDInitializeCardLib()) {
            fRet = FALSE;
        }
    }
    else if ( Reason == DLL_PROCESS_DETACH ) {
        SDDeinitializeCardLib();
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_Close - the close entry point for the memory driver
//  Input:  hOpenContext - the context returned from SMC_Open
//  Output:
//  Return: always returns TRUE
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C" BOOL WINAPI SMC_Close(DWORD hOpenContext)
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +-SMC_Close\n")));
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//  CleanUpDevice - cleanup the device instance
//  Input:  pDevice - device instance
//  Output:
//  Return:
//  Notes:
///////////////////////////////////////////////////////////////////////////////
VOID CleanUpDevice(PSD_MEMCARD_INFO pDevice)
{
    DeinitializePowerManagement(pDevice);

    // acquire removal lock
    AcquireRemovalLock(pDevice);

    if (NULL != pDevice->pRegPath) {
        // free the reg path
        SDFreeMemory(pDevice->pRegPath);
    }

    if (NULL != pDevice->hBufferList) {
        // delete the buffer memory list
        SDDeleteMemList(pDevice->hBufferList);
    }

    ReleaseRemovalLock(pDevice);

    DeleteCriticalSection(&pDevice->RemovalLock);
    DeleteCriticalSection(&pDevice->CriticalSection);

    // free the sterile I/O request
    if (NULL != pDevice->pSterileIoRequest) {
        LocalFree(pDevice->pSterileIoRequest);
        pDevice->pSterileIoRequest = NULL;
    }

    // free the device memory
    SDFreeMemory(pDevice);
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_Deinit - the deinit entry point for the memory driver
//  Input:  hDeviceContext - the context returned from SMC_Init
//  Output:
//  Return: always returns TRUE
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C" BOOL WINAPI SMC_Deinit(DWORD hDeviceContext)
{
    PSD_MEMCARD_INFO pDevice;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: +SMC_Deinit\n")));

    pDevice = (PSD_MEMCARD_INFO)hDeviceContext;

        // now it is safe to clean up
    CleanUpDevice(pDevice);

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SMC_Deinit\n")));

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//  SlotEventCallBack - slot event callback for fast-path events
//  Input:  hDevice - device handle
//          pContext - device specific context that was registered
//          SlotEventType - slot event type
//          pData - Slot event data (can be NULL)
//          DataLength - length of slot event data (can be 0)
//  Output:
//  Return:
//  Notes:
//
//      If this callback is registered the client driver can be notified of
//      slot events (such as device removal) using a fast path mechanism.  This
//      is useful if a driver must be notified of device removal
//      before its XXX_Deinit is called.
//
//      This callback can be called at a high thread priority and should only
//      set flags or set events.  This callback must not perform any
//      bus requests or call any apis that can perform bus requests.
///////////////////////////////////////////////////////////////////////////////
VOID SlotEventCallBack(SD_DEVICE_HANDLE    hDevice,
                       PVOID               pContext,
                       SD_SLOT_EVENT_TYPE  SlotEventType,
                       PVOID               pData,
                       DWORD               DataLength)
{
    PSD_MEMCARD_INFO pDevice;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: +SlotEventCallBack - %d \n"),SlotEventType));

    switch (SlotEventType) {
        case SDCardEjected :
            pDevice = (PSD_MEMCARD_INFO)pContext;
                // mark that the card is being ejected
            pDevice->CardEjected = TRUE;
                // acquire the removal lock to block this callback
                // in case an ioctl is in progress
            AcquireRemovalLock(pDevice);
            ReleaseRemovalLock(pDevice);

            break;

    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SlotEventCallBack \n")));
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_Init - the init entry point for the memory driver
//  Input:  dwContext - the context for this init
//  Output:
//  Return: non-zero context
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C" DWORD WINAPI SMC_Init(DWORD dwContext)
{
  SD_DEVICE_HANDLE                hClientHandle;  // client handle
  PSD_MEMCARD_INFO                pDevice;        // this instance of the device
  SDCARD_CLIENT_REGISTRATION_INFO ClientInfo;     // client into
  ULONG                           BufferSize;     // size of buffer
  HKEY                            hSubKey;        // registry key
  SD_API_STATUS                   Status;         // intermediate status
  DWORD                           data;           // registry data
  DWORD                           dataLength;     // registry data length

  DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: +SMC_Init\r\n")));

  pDevice = (PSD_MEMCARD_INFO)SDAllocateMemoryWithTag(
      sizeof(SD_MEMCARD_INFO),
      SD_MEMORY_TAG);
  if (pDevice == NULL) {
    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemory: Failed to allocate device info\r\n")));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SMC_Init\r\n")));
    return 0;
  }

  // initialize sterile I/O request to NULL
  pDevice->pSterileIoRequest = NULL;

  InitializeCriticalSection(&pDevice->CriticalSection);
  InitializeCriticalSection(&pDevice->RemovalLock);

  // get the device handle from the bus driver
  hClientHandle = SDGetDeviceHandle(dwContext, &pDevice->pRegPath);
  // store device handle in local context
  pDevice->hDevice = hClientHandle;
  if (NULL == hClientHandle) {
    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemory: Failed to get client handle\r\n")));
    CleanUpDevice(pDevice);
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SMC_Init\r\n")));
    return 0;
  }

  // allocate sterile I/O request
  pDevice->pSterileIoRequest = (PSG_REQ)LocalAlloc(
      LPTR,
      (sizeof(SG_REQ) + ((MAX_SG_BUF - 1) * sizeof(SG_BUF)))
      );
  if (NULL == pDevice->pSterileIoRequest) {
    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemory: Failed to allocate sterile I/O request\r\n")));
    CleanUpDevice(pDevice);
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SMC_Init\r\n")));
    return 0;
  }

  // register our debug zones
  SDRegisterDebugZones(hClientHandle, pDevice->pRegPath);

  memset(&ClientInfo, 0, sizeof(ClientInfo));

  // set client options and register as a client device
  _tcscpy(ClientInfo.ClientName, TEXT("Memory Card"));

  // set the callback
  ClientInfo.pSlotEventCallBack = SlotEventCallBack;

  Status = SDRegisterClient(hClientHandle, pDevice, &ClientInfo);
  if (!SD_API_SUCCESS(Status)) {
    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemory: Failed to register client : 0x%08X\r\n"),
          Status));
    CleanUpDevice(pDevice);
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SMC_Init\r\n")));
    return 0;
  }
#ifdef _FOR_MOVI_NAND_
  /**
   * Description : There is no way to distinguish between HSMMC and moviNAND.
   *               So, We assume A HSMMC card is a moviNAND. Default value is false;
   */
  pDevice->IsHSMMC = FALSE;
#endif
  // configure card and retrieve disk size/format information
  if( SDMemCardConfig( pDevice ) != ERROR_SUCCESS ) {
    CleanUpDevice(pDevice);
    DEBUGMSG( SDCARD_ZONE_ERROR, (TEXT("SDMemory: Error initializing MemCard structure and card\r\n")));
    return 0;
  }

  // aet a default block transfer size
  pDevice->BlockTransferSize = DEFAULT_BLOCK_TRANSFER_SIZE;

  // read configuration from registry

  // open the reg path
  if (RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        pDevice->pRegPath,
        0,
        KEY_ALL_ACCESS,
        &hSubKey) == ERROR_SUCCESS
     ) {
    // read "BlockTransferSize"
    dataLength = sizeof(pDevice->BlockTransferSize);
    RegQueryValueEx(
        hSubKey,
        BLOCK_TRANSFER_SIZE_KEY,
        NULL,
        NULL,
        (PUCHAR)&(pDevice->BlockTransferSize),
        &dataLength);
    if (pDevice->BlockTransferSize != DEFAULT_BLOCK_TRANSFER_SIZE) {
      DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: Initialize: Using block transfer size of %d blocks\r\n"),
            pDevice->BlockTransferSize));
    }

    // read "SingleBlockWrites"
    // default to using mulitple block writes
    pDevice->SingleBlockWrites = FALSE;
    dataLength = sizeof(DWORD);
    data = 0;
    if (RegQueryValueEx(
          hSubKey,
          SINGLE_BLOCK_WRITES_KEY,
          NULL,
          NULL,
          (PUCHAR)&data,
          &dataLength) == ERROR_SUCCESS
       ) {
      // key is present
      pDevice->SingleBlockWrites = TRUE;
      DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: Initialize: Using single block write commands only\r\n")));
    }

    // read "DisablePowerManagement"
    // on by default unless key is present
    pDevice->EnablePowerManagement = TRUE;
    dataLength = sizeof(DWORD);
    data = 0;
    if (RegQueryValueEx(
          hSubKey,
          DISABLE_POWER_MANAGEMENT,
          NULL,
          NULL,
          (PUCHAR)&data,
          &dataLength) == ERROR_SUCCESS
       ) {
      // key is present
      DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: Initialize: Disabled power management\r\n")));
      pDevice->EnablePowerManagement = FALSE;
    }

    // read "IdleTimeout"
    pDevice->IdleTimeout = DEFAULT_IDLE_TIMEOUT;
    dataLength = sizeof(pDevice->IdleTimeout);
    if (RegQueryValueEx(
          hSubKey,
          IDLE_TIMEOUT,
          NULL,
          NULL,
          (PUCHAR)&pDevice->IdleTimeout,
          &dataLength) == ERROR_SUCCESS
       ) {
      // key is present
      DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: Initialize: Using idle timeout of %u milliseconds\r\n"),
            pDevice->IdleTimeout));
    }

    // read "IdlePowerState"
    // default power state for idle
    // if the power state is greater than D2, we do idle checking
    pDevice->PowerStateForIdle = D2;
    dataLength = sizeof(DWORD);
    data = 0;
    if (RegQueryValueEx(
          hSubKey,
          IDLE_POWER_STATE,
          NULL,
          NULL,
          (PUCHAR)&data,
          &dataLength) == ERROR_SUCCESS
       ) {
      if (data <= (ULONG)D4) {
        pDevice->PowerStateForIdle = (CEDEVICE_POWER_STATE)data;
      }
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: Idle Timeout: %d Idle Power State: %d\r\n"),
          pDevice->IdleTimeout,
          pDevice->PowerStateForIdle));

    RegCloseKey(hSubKey);
  }

  // allocate our buffer list; we need 2 buffers: 1 for read and 1 for write data
  BufferSize = (ULONG)(pDevice->BlockTransferSize * SD_BLOCK_SIZE);

  // create the data buffer memory list
  pDevice->hBufferList = SDCreateMemoryList(SD_MEMORY_TAG, 2, BufferSize);
  if (pDevice->hBufferList == NULL) {
    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDMemory: Initialize: Failed to allocate buffer list\r\n")));
    CleanUpDevice(pDevice);
    return 0;
  }

  pDevice->fPreDeinitCalled = FALSE;

  InitializePowerManagement(pDevice);

  DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SMC_Init\r\n")));

  return (DWORD)pDevice;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_IOControl - the I/O control entry point for the memory driver
//  Input:  Handle - the context returned from SMC_Open
//          IoctlCode - the ioctl code
//          pInBuf - the input buffer from the user
//          InBufSize - the length of the input buffer
//          pOutBuf - the output buffer from the user
//          InBufSize - the length of the output buffer
//          pBytesReturned - the size of the transfer
//  Output:
//  Return:  TRUE if ioctl was handled
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C" BOOL WINAPI SMC_IOControl(
    DWORD   Handle,
    DWORD   IoctlCode,
    PBYTE   pInBuf,
    DWORD   InBufSize,
    PBYTE   pOutBuf,
    DWORD   OutBufSize,
    PDWORD  pBytesReturned
    )
{
    DWORD            Status = ERROR_SUCCESS;             // win32 status
    PSD_MEMCARD_INFO pHandle = (PSD_MEMCARD_INFO)Handle; // memcard info
    PSG_REQ          pSG;                                // scatter gather buffer
    SD_API_STATUS    sdStatus;                           // SD API status
    DWORD            SafeBytesReturned = 0;              // safe copy of pBytesReturned
    DWORD             dwStartTicks = 0;

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +SMC_IOControl\r\n")));

    // any of these IOCTLs can access the device instance or card handle so we
    // must protect it from being freed from XXX_Deinit; Windows CE does not
    // synchronize the  callback from Deinit
    AcquireRemovalLock(pHandle);

    if (pHandle->fPreDeinitCalled) {
        Status = ERROR_INVALID_HANDLE;
        goto ErrorStatusReturn;
    }

    sdStatus = RequestPrologue(pHandle, IoctlCode);

    if (!SD_API_SUCCESS(sdStatus)) {
        ReleaseRemovalLock(pHandle);
        SetLastError(SDAPIStatusToErrorCode(sdStatus));
        return FALSE;
    }

    DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("SMC_IOControl: Recevied IOCTL %d ="), IoctlCode));
    switch(IoctlCode) {
        case IOCTL_DISK_READ:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_READ\r\n")));
            break;
        case DISK_IOCTL_READ:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("DISK_IOCTL_READ\r\n")));
            break;
        case IOCTL_DISK_WRITE:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_WRITE\r\n")));
            break;
        case DISK_IOCTL_WRITE:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("DISK_IOCTL_WRITE\r\n")));
            break;
        case IOCTL_DISK_GETINFO:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_GETINFO\r\n")));
            break;
        case DISK_IOCTL_GETINFO:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("DISK_IOCTL_GETINFO\r\n")));
            break;
        case IOCTL_DISK_SETINFO:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_SETINFO\r\n")));
            break;
        case DISK_IOCTL_INITIALIZED:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("DISK_IOCTL_INITIALIZED\r\n")));
            break;
        case IOCTL_DISK_INITIALIZED:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_INITIALIZED\r\n")));
            break;
        case IOCTL_DISK_GETNAME:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_GETNAME\r\n")));
            break;
        case DISK_IOCTL_GETNAME:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("DISK_IOCTL_GETNAME\r\n")));
            break;
        case IOCTL_DISK_GET_STORAGEID:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_GET_STORAGEID\r\n")));
            break;
        case IOCTL_DISK_FORMAT_MEDIA:
        case DISK_IOCTL_FORMAT_MEDIA:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_FORMAT_MEDIA\r\n")));
            break;
        case IOCTL_DISK_DEVICE_INFO:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_DEVICE_INFO\r\n")));
            break;
        case IOCTL_DISK_DELETE_SECTORS:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("IOCTL_DISK_DELETE_SECTORS\r\n")));
            break;
        default:
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("**UNKNOWN**\r\n")));
            break;
    }

    // validate parameters
    switch(IoctlCode) {

        case IOCTL_DISK_READ:
        case DISK_IOCTL_READ:
        case IOCTL_DISK_WRITE:
        case DISK_IOCTL_WRITE:
            if (pInBuf == NULL || InBufSize < sizeof(SG_REQ) || InBufSize > (sizeof(SG_REQ) + ((MAX_SG_BUF - 1) * sizeof(SG_BUF)))) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case DISK_IOCTL_GETINFO:
        case IOCTL_DISK_SETINFO:
            if (NULL == pInBuf || InBufSize != sizeof(DISK_INFO)) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_DISK_DELETE_SECTORS:
            if (pInBuf == NULL || InBufSize != sizeof(DELETE_SECTOR_INFO)) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_DISK_GETINFO:
            if (pOutBuf == NULL || OutBufSize != sizeof(DISK_INFO)) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_DISK_GET_STORAGEID:
            // the identification data is stored after the struct, so the out
            // buffer must be at least the size of the struct.
            if (pOutBuf == NULL || OutBufSize < sizeof(STORAGE_IDENTIFICATION)) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_DISK_FORMAT_MEDIA:
        case DISK_IOCTL_FORMAT_MEDIA:
            break;

        case IOCTL_DISK_DEVICE_INFO:
            if (NULL == pInBuf || (InBufSize != sizeof(STORAGEDEVICEINFO))) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_POWER_CAPABILITIES:
            if (!pOutBuf || OutBufSize < sizeof(POWER_CAPABILITIES) || !pBytesReturned) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_POWER_SET:
            if (!pOutBuf || OutBufSize < sizeof(CEDEVICE_POWER_STATE) || !pBytesReturned) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        default:
            Status = ERROR_INVALID_PARAMETER;
    }

    if (Status != ERROR_SUCCESS) {
        goto ErrorStatusReturn;
    }

    // execute the IOCTL
    switch(IoctlCode) {

        case IOCTL_DISK_READ:
        case DISK_IOCTL_READ:
            pSG = (PSG_REQ)pInBuf;
            if (0 == CeSafeCopyMemory((LPVOID)pHandle->pSterileIoRequest, (LPVOID)pSG, InBufSize)) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            Status = SDMemRead(pHandle, pHandle->pSterileIoRequest);
            __try {
                pSG->sr_status = Status;
                if (pBytesReturned && (ERROR_SUCCESS == Status)) {
                    *pBytesReturned = (pHandle->pSterileIoRequest->sr_num_sec * SD_BLOCK_SIZE);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_DISK_WRITE:
        case DISK_IOCTL_WRITE:
            pSG = (PSG_REQ)pInBuf;
            if (0 == CeSafeCopyMemory((LPVOID)pHandle->pSterileIoRequest, (LPVOID)pSG, InBufSize)) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            Status = SDMemWrite(pHandle, pHandle->pSterileIoRequest);
            __try {
                pSG->sr_status = Status;
                if (pBytesReturned && (ERROR_SUCCESS == Status)) {
                    *pBytesReturned = (pHandle->pSterileIoRequest->sr_num_sec * SD_BLOCK_SIZE);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                Status = ERROR_INVALID_PARAMETER;
            }
            break;

        case IOCTL_DISK_GETINFO:
        {
            DISK_INFO SafeDiskInfo = {0};
            SafeBytesReturned = sizeof(DISK_INFO);
            Status = GetDiskInfo(pHandle, &SafeDiskInfo);
            if (0 == CeSafeCopyMemory((LPVOID)pOutBuf, (LPVOID)&SafeDiskInfo, sizeof(DISK_INFO))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            if (pBytesReturned) {
                if (0 == CeSafeCopyMemory((LPVOID)pBytesReturned, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    Status = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
        }
            break;

        case DISK_IOCTL_GETINFO:
        {
            DISK_INFO SafeDiskInfo = {0};
            SafeBytesReturned = sizeof(DISK_INFO);
            Status = GetDiskInfo(pHandle, &SafeDiskInfo);
            if (0 == CeSafeCopyMemory((LPVOID)pInBuf, (LPVOID)&SafeDiskInfo, sizeof(DISK_INFO))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            if (pBytesReturned) {
                if (0 == CeSafeCopyMemory((LPVOID)pBytesReturned, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    Status = ERROR_INVALID_PARAMETER;
                    break;
                }
            }
        }
            break;

        case IOCTL_DISK_SETINFO:
        {
            DISK_INFO SafeDiskInfo = {0};
            if (0 == CeSafeCopyMemory((LPVOID)&SafeDiskInfo, (LPVOID)pInBuf, sizeof(DISK_INFO))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            Status = SetDiskInfo(pHandle, &SafeDiskInfo);
        }
            break;

        case IOCTL_DISK_FORMAT_MEDIA:
        case DISK_IOCTL_FORMAT_MEDIA:
            Status = ERROR_SUCCESS;
            break;

        case IOCTL_DISK_GET_STORAGEID:
        {
            __try {
                Status = GetStorageID(
                    pHandle,
                    (PSTORAGE_IDENTIFICATION)pOutBuf,
                    OutBufSize,
                    pBytesReturned);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                Status = ERROR_INVALID_PARAMETER;
            }
        }
            break;

        case IOCTL_DISK_DEVICE_INFO:
        {
            STORAGEDEVICEINFO SafeStorageDeviceInfo = {0};
            SafeBytesReturned = sizeof(STORAGEDEVICEINFO);
            if (!GetDeviceInfo(pHandle, &SafeStorageDeviceInfo)) {
                Status = ERROR_GEN_FAILURE;
                break;
            }
            if (0 == CeSafeCopyMemory((LPVOID)pInBuf, (LPVOID)&SafeStorageDeviceInfo, sizeof(STORAGEDEVICEINFO))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            Status = ERROR_SUCCESS;
            if (pBytesReturned) {
                if (0 == CeSafeCopyMemory((LPVOID)pBytesReturned, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
        }
            break;

        case IOCTL_DISK_DELETE_SECTORS:
        {
            DELETE_SECTOR_INFO SafeDeleteSectorInfo = {0};
            if (0 == CeSafeCopyMemory((LPVOID)&SafeDeleteSectorInfo, (LPVOID)pInBuf, sizeof(DELETE_SECTOR_INFO))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            Status = SDMemErase(pHandle, &SafeDeleteSectorInfo);
        }
            break;

        case IOCTL_POWER_CAPABILITIES:
        {
            POWER_CAPABILITIES SafePowerCapabilities = {0};
            SafeBytesReturned = sizeof(POWER_CAPABILITIES);

            // support D0 + PowerStateForIdle (D2, by default)
            SafePowerCapabilities.DeviceDx = DX_MASK(D0) | DX_MASK(pHandle->PowerStateForIdle);

            SafePowerCapabilities.Power[D0] = PwrDeviceUnspecified;
            SafePowerCapabilities.Power[D1] = PwrDeviceUnspecified;
            SafePowerCapabilities.Power[D2] = PwrDeviceUnspecified;
            SafePowerCapabilities.Power[D3] = PwrDeviceUnspecified;
            SafePowerCapabilities.Power[D4] = PwrDeviceUnspecified;

            SafePowerCapabilities.Latency[D0] = 0;
            SafePowerCapabilities.Latency[D1] = 0;
            SafePowerCapabilities.Latency[D2] = 0;
            SafePowerCapabilities.Latency[D3] = 0;
            SafePowerCapabilities.Latency[D4] = 1000;

            // no device wake
            SafePowerCapabilities.WakeFromDx = 0;
            // no inrush
            SafePowerCapabilities.InrushDx = 0;

            if (0 == CeSafeCopyMemory((LPVOID)pOutBuf, (LPVOID)&SafePowerCapabilities, sizeof(POWER_CAPABILITIES))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            Status = ERROR_SUCCESS;
            if (pBytesReturned) {
                if (0 == CeSafeCopyMemory((LPVOID)pBytesReturned, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
        }
            break;

        case IOCTL_POWER_SET:
        {
            // pOutBuf is a pointer to CEDEVICE_POWER_STATE; this is the device
            // state incd .. which to put the device; if the driver does not support
            // the requested power state, then we return the adjusted power
            // state
            CEDEVICE_POWER_STATE SafeCeDevicePowerState;
            SafeBytesReturned = sizeof(CEDEVICE_POWER_STATE);
            if (0 == CeSafeCopyMemory((LPVOID)&SafeCeDevicePowerState, (LPVOID)pOutBuf, sizeof(CEDEVICE_POWER_STATE))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            Status = ERROR_SUCCESS;
            HandleIoctlPowerSet(pHandle, &SafeCeDevicePowerState);
            // return the adjusted power state
            if (0 == CeSafeCopyMemory((LPVOID)pOutBuf, (LPVOID)&SafeCeDevicePowerState, sizeof(CEDEVICE_POWER_STATE))) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
            if (pBytesReturned) {
                if (0 == CeSafeCopyMemory((LPVOID)pBytesReturned, (LPVOID)&SafeBytesReturned, sizeof(DWORD))) {
                    Status = ERROR_INVALID_PARAMETER;
                }
            }
        }
            break;

        default:
            Status = ERROR_INVALID_PARAMETER;
            break;
    }

    RequestEnd(pHandle);

ErrorStatusReturn:

    ReleaseRemovalLock(pHandle);

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: -SMC_IOControl returning %d\n"),Status == ERROR_SUCCESS));

    if (Status != ERROR_SUCCESS) {
        SetLastError(Status);
    }

    return (ERROR_SUCCESS == Status);
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_Open - the open entry point for the memory driver
//  Input:  hDeviceContext - the device context from SMC_Init
//          AccessCode - the desired access
//          ShareMode - the desired share mode
//  Output:
//  Return: open context to device instance
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C" DWORD WINAPI SMC_Open(
    DWORD hDeviceContext,
    DWORD AccessCode,
    DWORD ShareMode
)
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +-SMC_Open\n")));
    return hDeviceContext;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_PowerDown - the power down entry point for the bus driver
//  Input:  hDeviceContext - the device context from SMC_Init
//  Output:
//  Return:
//  Notes:  preforms no actions
///////////////////////////////////////////////////////////////////////////////
extern "C" VOID WINAPI SMC_PowerDown(DWORD hDeviceContext)
{
        // no prints allowed
    return;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_PowerUp - the power up entry point for the CE file system wrapper
//  Input:  hDeviceContext - the device context from SMC_Init
//  Output:
//  Return:
//  Notes:  preforms no actions
///////////////////////////////////////////////////////////////////////////////
extern "C" VOID WINAPI SMC_PowerUp(DWORD hDeviceContext)
{
        // no prints allowed
    return;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_Read - the read entry point for the memory driver
//  Input:  hOpenContext - the context from SMC_Open
//          pBuffer - the user's buffer
//          Count - the size of the transfer
//  Output:
//  Return: zero
//  Notes:  always returns zero (failure)
///////////////////////////////////////////////////////////////////////////////
extern "C" DWORD WINAPI SMC_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +-SMC_Read\n")));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_Seek - the seek entry point for the memory driver
//  Input:  hOpenContext - the context from SMC_Open
//          Amount - the amount to seek
//          Type - the type of seek
//  Output:
//  Return: zero
//  Notes:  always returns zero (failure)
///////////////////////////////////////////////////////////////////////////////
extern "C" DWORD WINAPI SMC_Seek(DWORD hOpenContext, long Amount, DWORD Type)
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +-SMC_Seek\n")));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_Write - the write entry point for the memory driver
//  Input:  hOpenContext - the context from SMC_Open
//          pBuffer - the user's buffer
//          Count - the size of the transfer
//  Output:
//  Return: zero
//  Notes:  always returns zero (failure)
///////////////////////////////////////////////////////////////////////////////
extern "C" DWORD WINAPI SMC_Write(DWORD hOpenContext, LPCVOID pBuffer, DWORD Count)
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDMemory: +-SMC_Write\n")));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  GetDiskInfo      - return disk info in response to DISK_IOCTL_GETINFO
//  Input:  pMemCard - SD memory card structure
//  Output: pInfo    - PDISK_INFO structure containing disk parameters
//  Return: win32 status
//  Notes:
///////////////////////////////////////////////////////////////////////////////
DWORD GetDiskInfo( PSD_MEMCARD_INFO pMemCard, PDISK_INFO pInfo )
{
    *pInfo = pMemCard->DiskInfo;
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//  SetDiskInfo      - store disk info in response to DISK_IOCTL_SETINFO
//  Input:  pMemCard - SD memory card structure
//          pInfo    - PDISK_INFO structure containing disk parameters
//  Output:
//  Return: win32 status
//  Notes
///////////////////////////////////////////////////////////////////////////////
DWORD SetDiskInfo( PSD_MEMCARD_INFO pMemCard, PDISK_INFO pInfo )
{
    pMemCard->DiskInfo = *pInfo;
    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//  GetStorageID      - Returns storage ID based on manufactured ID + serial #
//  Input:  pMemCard  - SD memory card structure
//          cBytes    - Size of psid buffer
//  Output: psid      - Storage ID structure
//          pcBytes   - Size of data written to psid
//  Return: win32 status
//  Notes:  The Storage ID gets to written to space allocated after the actual
//          PSTORAGE_IDENTIFICATION structure.
///////////////////////////////////////////////////////////////////////////////
DWORD GetStorageID( PSD_MEMCARD_INFO pMemCard,
                    PSTORAGE_IDENTIFICATION psid,
                    DWORD cBytes,
                    DWORD *pcBytes )
{
    PCHAR pDstOffset;   // destination offset for ID

    DEBUGMSG( SDCARD_ZONE_FUNC, (TEXT("SDMemory: +GetStorageID\r\n")));

        // check enough space exists in buffer
    if( cBytes < (sizeof(*psid)+SD_SIZEOF_STORAGE_ID) ) {
        DEBUGMSG( SDCARD_ZONE_ERROR, (TEXT("SDMemory: GetStorageID Insufficient buffer space\r\nSDMemory: -GetStorageID\r\n")));
        psid->dwSize = (sizeof(*psid)+SD_SIZEOF_STORAGE_ID);
        return ERROR_INSUFFICIENT_BUFFER;
    }

        // point to location after end of PSTORAGE_IDENTIFICATION
    pDstOffset = (PCHAR)(psid+1);

        // form manufacturer ID as string in the structure
    psid->dwManufactureIDOffset = pDstOffset - (PCHAR)psid;
    pDstOffset += sprintf( pDstOffset, "%02X\0", pMemCard->CIDRegister.ManufacturerID );

        // form serial number as string in the structure
    psid->dwSerialNumOffset = pDstOffset - (PCHAR)psid;
    sprintf( pDstOffset, "%08X\0", pMemCard->CIDRegister.ProductSerialNumber );

        // set structure fields
    psid->dwSize = sizeof(*psid) + SD_SIZEOF_STORAGE_ID;
    psid->dwFlags = 0;

    *pcBytes = psid->dwSize;

    DEBUGMSG( SDCARD_ZONE_FUNC, (TEXT("SDMemory: -GetStorageID\r\n")));

    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//  GetDeviceInfo  - get the device profile and information
//  Input:  pMemCard - the memory card instance
//          pStorageInfo - storage info structure to fill in
//  Output:
//  Return: returns TRUE if device information was retreived
//  Notes
///////////////////////////////////////////////////////////////////////////////
BOOL GetDeviceInfo(PSD_MEMCARD_INFO pMemCard, PSTORAGEDEVICEINFO pStorageInfo)
{
    HKEY  hDriverKey;   // driver key
    DWORD ValType;      // registry key value type
    DWORD status;       // win32 status
    DWORD dwSize;       // size of key

      // get the FolderName key if it exists
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      pMemCard->pRegPath,
                      0,
                      KEY_ALL_ACCESS,
                      &hDriverKey) != ERROR_SUCCESS) {
        DEBUGMSG(SDCARD_ZONE_ERROR,
                (TEXT("SDemory: GetDeviceInfo - Failed to open reg path %s \r\n"),
                      pMemCard->pRegPath));

        return FALSE;
    }

    if (hDriverKey) {
        dwSize = sizeof(pStorageInfo->szProfile);
        status = RegQueryValueEx(
                    hDriverKey,
                    TEXT("Profile"),
                    NULL,
                    &ValType,
                    (LPBYTE)pStorageInfo->szProfile,
                    &dwSize);
        if ((status != ERROR_SUCCESS) || (dwSize > sizeof(pStorageInfo->szProfile))){
            DEBUGMSG(SDCARD_ZONE_ERROR | SDCARD_ZONE_INIT,
                (TEXT("SDemory: GetDeviceInfo - RegQueryValueEx(Profile) returned %d\r\n"),
                      status));
            wcscpy( pStorageInfo->szProfile, L"Default");
        } else {
            DEBUGMSG(SDCARD_ZONE_INIT,
                (TEXT("SDMemory: GetDeviceInfo - Profile = %s, length = %d\r\n"),
                 pStorageInfo->szProfile, dwSize));
        }
        RegCloseKey(hDriverKey);
    }

    pStorageInfo->dwDeviceClass = STORAGE_DEVICE_CLASS_BLOCK;
    pStorageInfo->dwDeviceType = STORAGE_DEVICE_TYPE_UNKNOWN;
    pStorageInfo->dwDeviceType |= STORAGE_DEVICE_TYPE_REMOVABLE_MEDIA;

    if (pMemCard->WriteProtected) {
        pStorageInfo->dwDeviceFlags = STORAGE_DEVICE_FLAG_READONLY;
    }
    else {
        pStorageInfo->dwDeviceFlags = STORAGE_DEVICE_FLAG_READWRITE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//  SMC_PreDeinit - the deinit entry point for the memory driver
//  Input:  hDeviceContext - the context returned from SMC_Init
//  Output:
//  Return: always returns TRUE
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern "C" BOOL WINAPI SMC_PreDeinit(DWORD hDeviceContext)
{
    PSD_MEMCARD_INFO pDevice;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: +SMC_PreDeinit\n")));

    pDevice = (PSD_MEMCARD_INFO)hDeviceContext;

    AcquireRemovalLock(pDevice);
    pDevice->fPreDeinitCalled = TRUE;
    ReleaseRemovalLock(pDevice);

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDMemory: -SMC_PreDeinit\n")));

    return TRUE;
}


// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

