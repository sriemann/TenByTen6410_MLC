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

// Copyright (c) 2002-2004 BSQUARE Corporation.  All rights reserved.
// DO NOT REMOVE --- BEGIN EXTERNALLY DEVELOPED SOURCE CODE ID 40973--- DO NOT REMOVE

// Header file defining the host controller interface

#ifndef _SDCARD_HCD_DEFINED
#define _SDCARD_HCD_DEFINED

#include "SDCardDDK.h"

#define SD_HC_MAX_NAME_LENGTH 16

#define BUS_VER_STR_2_0     (TEXT("VER 2.0"))
#define BUS_VER_FOR_HOST    BUS_VER_STR_2_0

#define DEFAULT_MAX_SLOT_CURRENT   200


typedef struct _SDCARD_HC_CONTEXT *PSDCARD_HC_CONTEXT;

// typedef for slot option handler codes
typedef enum _SD_SLOT_OPTION_CODE {
    SDHCDNop = -1,
    SDHCDSetSlotPower,              // set slot power, takes a DWORD for the power bit mask
    SDHCDSetSlotInterface,          // set slot interface, takes a SD_CARD_INTERFACE structure
    SDHCDEnableSDIOInterrupts,      // enable SDIO interrupts on the slot, no parameters
    SDHCDDisableSDIOInterrupts,     // disable SDIO interrupts on the slot, no parameters
    SDHCDAckSDIOInterrupt,          // acknowledge that the SDIO interrupt was handled, no parameters
    SDHCDGetWriteProtectStatus,     // get Write protect status. Updates SD_CARD_INTERFACE structure
    SDHCDQueryBlockCapability,      // query whether HC supports requested block length, 
                                    //   takes SD_HOST_BLOCK_CAPABILITY structure
    SDHCDSetClockStateDuringIdle,   // set the clock state(on or off) during the idle state
    SDHCDSetSlotPowerState,         // set the slot power state, takes a CEDEVICE_POWER_STATE
    SDHCDGetSlotPowerState,         // get the slot power state, takes a CEDEVICE_POWER_STATE
    SDHCDWakeOnSDIOInterrupts,      // wake on SDIO interrupts in D3, takes a BOOL
    SDHCDGetSlotInfo,               // get info on a specific slot, takes a PSDCARD_HC_SLOT_INFO
    SDHCDSetSlotInterfaceEx,        // set slot interface, takes a SD_CARD_INTERFACE_EX structure
    SDHCAllocateDMABuffer,          // Allocate Physical Memory thnat can be used by SDHC.
    SDHCFreeDMABuffer,              // Free physicla memory that allocated by SDHCAllocateDMABuffer
    SDHCDSlotOptionCount            // count of valid slot option codes
} SD_SLOT_OPTION_CODE, *PSD_SLOT_OPTION_CODE;

// typedef for asynchronous slot event codes
typedef enum _SD_SLOT_EVENT {
    NOP,                // no operation
    DeviceEjected,      // device was ejected
    DeviceInserted,     // device was inserted 
    DeviceInterrupting, // device is interrupting
    BusRequestComplete, // a bus request was completed
    SlotDeselectRequest,// Deslect the card in the slot
    SlotSelectRequest,  // Select the card in the slot
    SlotResetRequest    // Reset the card in the slot
} SD_SLOT_EVENT, *PSD_SLOT_EVENT;

// typedef for the Send command Handler
typedef SD_API_STATUS (*PSD_BUS_REQUEST_HANDLER) (PSDCARD_HC_CONTEXT, DWORD, PSD_BUS_REQUEST);

// typedef for the I/O cancel handler
typedef BOOLEAN (*PSD_CANCEL_REQUEST_HANDLER) (PSDCARD_HC_CONTEXT, DWORD, PSD_BUS_REQUEST);

// typedef for set slot options
typedef SD_API_STATUS (*PSD_GET_SET_SLOT_OPTION) (PSDCARD_HC_CONTEXT, DWORD, SD_SLOT_OPTION_CODE, PVOID, ULONG);

// typedef for initialize controller
typedef SD_API_STATUS (*PSD_INITIALIZE_CONTROLLER) (PSDCARD_HC_CONTEXT);

// typedef for initialize controller
typedef SD_API_STATUS (*PSD_DEINITIALIZE_CONTROLLER) (PSDCARD_HC_CONTEXT);

// typedef for power control handler
typedef SD_API_STATUS (*PSD_CHANGE_CARD_POWER) (PSDCARD_HC_CONTEXT, DWORD, INT);


// the following bit masks define the slot capabilities of a host controller

#ifdef _MMC_SPEC_42_
// slot supports 8 bit data transfers
#define SD_SLOT_SD_8BIT_CAPABLE                  0x00000002
#endif

// slot supports 1 bit data transfers
#define SD_SLOT_SD_1BIT_CAPABLE                  0x00000004
// the slot supports 4 bit data transfers
#define SD_SLOT_SD_4BIT_CAPABLE                  0x00000008
// the slot supports High Speed Transfer.
#define SD_SLOT_HIGH_SPEED_CAPABLE               0x00000010
// the slot is SD I/O capable, minimally supporting interrupt signalling. 
// driver can OR-in SD_SLOT_SDIO_INT_DETECT_4BIT_MULTI_BLOCK if the slot is capable
// of handling interrupt signalling during 4 bit Data transfers
#define SD_SLOT_SDIO_CAPABLE                     0x00000020
// the slot supports 4 bit SDMemory data transfer.
// Combine this flag with SD_SLOT_SDIO_CAPABLE and SD_SLOT_SD_1BIT_CAPABLE if
// the slot supports 4 bit SDMemory and 1 bit SDIO
#define SD_SLOT_SDMEM_4BIT_CAPABLE               0x00000040
// the slot supports 4 bit SDIO data transfer
// Combine this flag with SD_SLOT_SDIO_CAPABLE and SD_SLOT_SD_1BIT_CAPABLE if
// the slot supports 4 bit SDIO and 1 bit SD/MMC
#define SD_SLOT_SDIO_4BIT_CAPABLE                0x00000080

// slot supports interrupt detection during 4-bit multi-block transfers
#define SD_SLOT_SDIO_INT_DETECT_4BIT_MULTI_BLOCK 0x00000100

    // Host needs Soft-Block support for CMD18 read operations.
#define SD_SLOT_USE_SOFT_BLOCK_CMD18             0x00000200
    // Host needs Soft-Block support for CMD25 write operations.
#define SD_SLOT_USE_SOFT_BLOCK_CMD25             0x00000400
    // Host needs Soft-Block support for CMD53 multi-block read operations.
#define SD_SLOT_USE_SOFT_BLOCK_CMD53_READ        0x00000800
    // Host needs Soft-Block support for CMD53 multi-block read operations.
#define SD_SLOT_USE_SOFT_BLOCK_CMD53_WRITE       0x00001000


// host controller context
typedef struct _SDCARD_HC_CONTEXT {
    DWORD                       dwVersion;          // version of context structure

    WCHAR                       HostControllerName[SD_HC_MAX_NAME_LENGTH];  // friendly name
    CRITICAL_SECTION            HCCritSection;      // host controller critical section
    PSD_BUS_REQUEST_HANDLER     pBusRequestHandler; // bus request handler
    PSD_GET_SET_SLOT_OPTION     pSlotOptionHandler; // slot option handler
    PSD_CANCEL_REQUEST_HANDLER  pCancelIOHandler;   // cancel request handler
    PSD_INITIALIZE_CONTROLLER   pInitHandler;       // init handler       
    PSD_DEINITIALIZE_CONTROLLER pDeinitHandler;     // deinit handler
    PVOID                       pHCSpecificContext; // host controller specific context
    PSD_CHANGE_CARD_POWER       pChangeCardPowerHandler; // Pointer to power control handler
} SDCARD_HC_CONTEXT, *PSDCARD_HC_CONTEXT;

// host controller slot info structure
typedef struct _SDCARD_HC_SLOT_INFO {
    DWORD       Capabilities;           // bit mask defining capabilities
    DWORD       VoltageWindowMask;      // bit mask for slot's voltage window capability, same format at OCR register
    DWORD       DesiredVoltageMask;     // desired voltage bit mask
    DWORD       MaxClockRate;           // maximum clock rate in HZ, Max 4.29 GHz
    DWORD       PowerUpDelay;           // power up delay in MS
} SDCARD_HC_SLOT_INFO, *PSDCARD_HC_SLOT_INFO;

#define SDCARD_HC_BUS_INTERFACE_VERSION_MAJOR   1
#define SDCARD_HC_BUS_INTERFACE_VERSION_MINOR   0
#define SDCARD_HC_BUS_INTERFACE_VERSION \
    MAKELONG(SDCARD_HC_BUS_INTERFACE_VERSION_MINOR, \
        SDCARD_HC_BUS_INTERFACE_VERSION_MAJOR)

// macro to extract the extension specific context
#define GetExtensionFromHCDContext(d, pContext) ((d)((pContext)->pHCSpecificContext))

// macros to set host controller context structure entries
// the following structures entries must be filled out by all host controller drivers
#define SDHCDSetBusRequestHandler(pHc, d)       ((pHc)->pBusRequestHandler = d)
#define SDHCDSetCancelIOHandler(pHc, d)         ((pHc)->pCancelIOHandler = d)
#define SDHCDSetSlotOptionHandler(pHc, d)       ((pHc)->pSlotOptionHandler = d)
#define SDHCDSetControllerInitHandler(pHc, d)   ((pHc)->pInitHandler = d)
#define SDHCDSetControllerDeinitHandler(pHc, d) ((pHc)->pDeinitHandler = d)
#define SDHCDSetHCName(pHc, d)                  \
    (wcsncpy((pHc)->HostControllerName, d, dim((pHc)->HostControllerName) - 1))
#define SDHCDSetVersion(pHc)                    \
    ((pHc)->dwVersion = SDCARD_HC_BUS_INTERFACE_VERSION)
#define SDHCDSetChangePowerHandler(pHc,d)       ((pHc)->pChangeCardPowerHandler = d)
#define SDHCDGetChangePowerHandler(pHc)         ((pHc)->pChangeCardPowerHandler)

// the following macros provide HC synchronization and information
#define SDHCDAcquireHCLock(pHc)             EnterCriticalSection(&((pHc)->HCCritSection))
#define SDHCDReleaseHCLock(pHc)             LeaveCriticalSection(&((pHc)->HCCritSection))
#define SDHCDGetHCName(pHC)                 (pHC)->HostControllerName

// macros to set slot info structure entries
#define SDHCDSetSlotCapabilities(pSlot, d)    ((pSlot)->Capabilities = d)
#define SDHCDSetVoltageWindowMask(pSlot, d)   ((pSlot)->VoltageWindowMask = d) 
#define SDHCDSetDesiredSlotVoltage(pSlot,d)   ((pSlot)->DesiredVoltageMask = d)      
#define SDHCDSetMaxClockRate(pSlot, d)        ((pSlot)->MaxClockRate = d)
#define SDHCDSetPowerUpDelay(pSlot, d)        ((pSlot)->PowerUpDelay = d)


// macros used by the bus driver to invoke slot option handlers

#define SDEnableSDIOInterrupts(pSlot) \
    (pSlot)->pHostController->pSlotOptionHandler((pSlot)->pHostController,  \
    (pSlot)->SlotIndex,        \
    SDHCDEnableSDIOInterrupts, \
    NULL,                      \
    0)
#define SDDisableSDIOInterrupts(pSlot) \
    (pSlot)->pHostController->pSlotOptionHandler((pSlot)->pHostController,   \
    (pSlot)->SlotIndex,         \
    SDHCDDisableSDIOInterrupts, \
    NULL,                       \
    0)    
#define SDAckSDIOInterrupts(pSlot) \
    (pSlot)->pHostController->pSlotOptionHandler((pSlot)->pHostController,   \
    (pSlot)->SlotIndex,         \
    SDHCDAckSDIOInterrupt,      \
    NULL,                       \
    0)

typedef struct _SDHOST_API_FUNCTIONS {
    DWORD dwSize;

    SD_API_STATUS (*pAllocateContext)(DWORD cSlots,
        PSDCARD_HC_CONTEXT *ppHostContext);

    VOID (*pDeleteContext)(PSDCARD_HC_CONTEXT pHostContext);

    SD_API_STATUS (*pRegisterHostController)(PSDCARD_HC_CONTEXT pHCContext);

    SD_API_STATUS (*pDeregisterHostController)(PSDCARD_HC_CONTEXT pHCContext);

    VOID (*pIndicateSlotStateChange)(PSDCARD_HC_CONTEXT pHCContext, 
        DWORD              SlotNumber,
        SD_SLOT_EVENT      Event);

    VOID (*pIndicateBusRequestComplete)(PSDCARD_HC_CONTEXT pHCContext,
        PSD_BUS_REQUEST    pRequest,
        SD_API_STATUS      Status);

    VOID (*pUnlockRequest)(PSDCARD_HC_CONTEXT  pHCContext,
        PSD_BUS_REQUEST     pRequest);

    PSD_BUS_REQUEST (*pGetAndLockCurrentRequest)(PSDCARD_HC_CONTEXT pHCContext, 
        DWORD              SlotIndex);

    VOID (*pPowerUpDown)(PSDCARD_HC_CONTEXT  pHCContext, 
        BOOL                PowerUp, 
        BOOL                SlotKeepPower,
        DWORD               SlotIndex);
} SDHOST_API_FUNCTIONS, *PSDHOST_API_FUNCTIONS;
typedef SDHOST_API_FUNCTIONS const * PCSDHOST_API_FUNCTIONS;



#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// SDHCDInitializeHCLib - Initialize the host controller library
//
// Return: SD_API_STATUS 
// Notes: Call from DLL entry
//
SD_API_STATUS SDHCDInitializeHCLib();

// SDHCDDeinitializeHCLib - Deinitialize the host controller library
//
// Return: SD_API_STATUS 
// Notes: Call from DLL entry
//
SD_API_STATUS SDHCDDeinitializeHCLib();

// SDHCDAllocateContext - Allocate an HCD Context
//
// Input: NumberOfSlots - Number of slots
// Output:
//        ppHostContext - caller supplied storage for the host context
// Return: SD_API_STATUS 
// Notes:
//
SD_API_STATUS SDHCDAllocateContext(
    DWORD               cSlots, 
    PSDCARD_HC_CONTEXT *ppHostContext);

// SDHCDDeleteContext - Delete an HCD context
//
// Input: pHostContext - Host Context to delete
// Output:
// Return:
//
VOID SDHCDDeleteContext(
    PSDCARD_HC_CONTEXT pHostContext);


// SDHCDRegisterHostController - Register a host controller with the bus driver
//
// Input: pHCContext - Allocated Host controller context
//
// Output:
// Return: SD_API_STATUS 
// Notes:      
//      the caller must allocate a host controller context and 
//      initialize the various parameters
SD_API_STATUS SDHCDRegisterHostController(
    PSDCARD_HC_CONTEXT pHCContext);

// SDHCDDeregisterHostController - Deregister a host controller 
//
// Input: pHCContext - Host controller context that was previously registered
//        
// Output:
// Return: SD_API_STATUS 
// Notes:       
//      A host controller must call this api before deleting the HC context
//      
// returns SD_API_STATUS
SD_API_STATUS SDHCDDeregisterHostController(
    PSDCARD_HC_CONTEXT pHCContext);


// SDHCDIndicateSlotStateChange - indicate a change in the SD Slot 
//
// Input: pHCContext - Host controller context that was previously registered
//        SlotNumber - Slot Number
//        Event     - new event code
// Output:
// Return:
// Notes:        
//      A host controller driver calls this api when the slot changes state (i.e.
//      device insertion/deletion).
//      
// 
VOID SDHCDIndicateSlotStateChange(
    PSDCARD_HC_CONTEXT pHCContext, 
    DWORD              SlotNumber,
    SD_SLOT_EVENT      Event);

// SDHCDIndicateBusRequestComplete - indicate to the bus driver that
//                                   the request is complete
//
// Input: pHCContext - host controller context
//        pRequest   - the request to indicate
//        Status     - the ending status of the request
// Output:
// Return: 
// Notes:       
//     
VOID SDHCDIndicateBusRequestComplete(
    PSDCARD_HC_CONTEXT pHCContext,
    PSD_BUS_REQUEST    pRequest,
    SD_API_STATUS      Status);


// SDHCDUnlockRequest - Unlock a request that was previous locked
//                             
// Input:   pHCContext - host controller context   
//          pRequest  - the request to lock
// Output:
// Return:    
// Notes:   This function unlocks the request that was returned from the
//          function SDHCDGetAndLockCurrentRequest()
//          
//          This request can now be cancelled from any thread context
VOID SDHCDUnlockRequest(
    PSDCARD_HC_CONTEXT  pHCContext,
    PSD_BUS_REQUEST     pRequest);

// SDHCDGetAndLockCurrentRequest - get the current request in the host controller
//                                 slot and lock it to keep it from being cancelable
// Input:   pHCContext - host controller context   
//          SlotIndex  - the slot number 
// Output:
// Return: current bus request  
// Notes:
//          This function retrieves the current request and marks the
//          request as NON-cancelable.  To return the request back to the
//          cancelable state the caller must call SDHCDUnlockRequest()     
//          This function returns the current request which can be NULL if 
//          the request was previously marked cancelable and the host controller's
//          cancelIo Handler completed the request 
PSD_BUS_REQUEST SDHCDGetAndLockCurrentRequest(
    PSDCARD_HC_CONTEXT pHCContext, 
    DWORD              SlotIndex);


// SDHCDPowerUpDown - Indicate a power up/down event
//                             
// Input:   pHCContext - host controller context   
//          PowerUp    - set to TRUE if powering up
//          SlotKeepPower - set to TRUE if the slot maintains power to the
//                          slot during power down
// Output:
// Return:        
// Notes:   This function notifies the bus driver of a power up/down event.
//          The host controller driver can indicate to the bus driver that power
//          can be maintained for the slot.  If power is removed, the bus driver
//          will unload the device driver on the next power up event.
//          This function can only be called from the host controller's XXX_PowerOn
//          and XXX_PowerOff function.
VOID SDHCDPowerUpDown(
    PSDCARD_HC_CONTEXT  pHCContext, 
    BOOL                PowerUp, 
    BOOL                SlotKeepPower,
    DWORD               SlotIndex);

#ifdef __cplusplus
}
#endif //__cplusplus


#endif // _SDCARD_HCD_DEFINED

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

