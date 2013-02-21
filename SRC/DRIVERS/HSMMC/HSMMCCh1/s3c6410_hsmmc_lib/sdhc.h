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

#ifndef _SDHC_H_
#define _SDHC_H_

#include <windows.h>
#include <ceddk.h>
#include <devload.h>
#include <pm.h>
#include <ddkreg.h>
#include <creg.hxx>
#include <nkintr.h>
#include "SDCardDDK.h"
#include "SDHCD.h"
#include "SDHCRegs.h"
#include "SDHCSlot.h"

// Configuration information for a slot.
typedef struct SDHC_SLOT_INFO {
    volatile UCHAR *pucRegisters;
    DWORD           dwExtraInfo;
} *PSDHC_SLOT_INFO;


typedef class CSDHCBase *PCSDHCBase;

typedef PCSDHCBase (WINAPI *LPSDHC_CREATION_PROC) ();
typedef VOID (WINAPI *LPSDHC_DESTRUCTION_PROC) (PCSDHCBase);


// Base standard host controller class.
typedef class CSDHCBase {
public:
    // Constructor - only initializes the member data. True initialization
    // occurs in Init().
    CSDHCBase();
    virtual ~CSDHCBase();

    // Perform basic initialization including initializing the hardware
    // so that the capabilities register can be read.
    virtual BOOL Init(LPCTSTR pszActiveKey);

    // Second stage of hardware initialization. Start the IST and turn on
    // interrupts. Should be called from SDHCInitialize.
    virtual SD_API_STATUS Start();

    // Closes the IST and disables interrupts. Should be called from  
    // SDHCDeinitialize.
    virtual SD_API_STATUS Stop();

    // Called by SDHCCancelIoHandler.
    virtual BOOLEAN CancelIoHandler(DWORD dwSlot, PSD_BUS_REQUEST pRequest) {
        // We should never get here because all requests are non-cancelable.
        // The hardware supports timeouts so it is impossible for the 
        // controller to get stuck.
        DEBUGCHK(FALSE);

        return TRUE;
    }

    // Called by SDHCBusRequestHandler.
    virtual SD_API_STATUS BusRequestHandler(DWORD dwSlot, PSD_BUS_REQUEST pRequest) {
        Validate();

        // Acquire the device lock to protect from device removal
        Lock();
        SD_API_STATUS status = GetSlot(dwSlot)->BusRequestHandler(pRequest);
        Unlock();
        return status;
    }

    // Called by SDHCSlotOptionHandler.
    virtual SD_API_STATUS SlotOptionHandler(DWORD dwSlot, 
        SD_SLOT_OPTION_CODE sdOption, PVOID pData, DWORD cbData);

    // Called by SHC_IOControl. Does nothing by default.
    virtual DWORD IOControl(DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, 
        PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut) 
    { 
        return ERROR_INVALID_PARAMETER;
    }

    // Called by SHC_PowerDown.
    virtual VOID PowerDown();

    // Called by SHC_PowerUp.
    virtual VOID PowerUp();

    // Called by SHC_PreDeinit in order to release threads before destroying
    // our objects. Calls SDHCDDeregisterHostController if necessary.
    virtual VOID PreDeinit(); 

    // Called by SHC_Deinit to get the destructor function.
    virtual LPSDHC_DESTRUCTION_PROC GetDestructionProc() = 0;

    // Prototypes for bus callbacks
    static BOOLEAN SDHCCancelIoHandler(PSDCARD_HC_CONTEXT pHCContext,
        DWORD dwSlot, PSD_BUS_REQUEST pRequest);
    static SD_API_STATUS SDHCBusRequestHandler(PSDCARD_HC_CONTEXT pHCContext,
        DWORD dwSlot, PSD_BUS_REQUEST pRequest);
    static SD_API_STATUS SDHCSlotOptionHandler(PSDCARD_HC_CONTEXT pHCContext, 
        DWORD dwSlot, SD_SLOT_OPTION_CODE Option, PVOID pData, 
        ULONG OptionSize);
    static SD_API_STATUS SDHCDeinitialize(PSDCARD_HC_CONTEXT pHCContext);
    static SD_API_STATUS SDHCInitialize(PSDCARD_HC_CONTEXT pHCContext);

    // Called by SHC_Init to create the proper subclass object.f
    static PCSDHCBase CreateSDHCControllerObject(LPCTSTR pszActiveKey);

protected:
    // Allocate the slot objects. Override this method to use a slot
    // subclass.
    virtual PCSDHCSlotBase AllocateSlotObjects(DWORD cSlots) {
        PREFAST_ASSERT(CheckSlotCount(cSlots));
        return new CSDHCSlotBase[cSlots];
    }

    // Get the physical device information from the registry and
    // initialize the host controller hardware. Called by Init().
    virtual BOOL InitializeHardware();

    // Deinitialize all resources from InitializeHardware(). Called
    // by ~CSDHCBase().
    virtual BOOL DeinitializeHardware();


    // Static stub that calls IST().
    static DWORD WINAPI ISTStub(LPVOID lpParameter) {
        PCSDHCBase pController = (PCSDHCBase) lpParameter;
        return pController->IST();
    }


    // The controller's interrupt routine. Terminates when 
    // m_fDriverShutdown becomes TRUE.
    virtual DWORD IST(); 

    // Interrupt handler for every slot on the controller.
    // Calls HandleInterrupt() for each slot
    // that needs servicing.
    virtual VOID HandleInterrupt();

    // Determine the number of slots on this controller.
    // Returns 0 if there was a problem.
    virtual DWORD DetermineSlotCount();

    // Return the index of the slot zero's window.
    virtual DWORD DetermineFirstSlotWindow(PDDKWINDOWINFO pwini);

    // Look at all the slots to see which power state is needed
    // at the controller level.
    virtual CEDEVICE_POWER_STATE DetermineRequiredControllerPowerState();

    // Set the power state for the entire controller.
    virtual SD_API_STATUS SetControllerPowerState(CEDEVICE_POWER_STATE cpsNew);

    // Get the pointer to the slot.
    virtual PCSDHCSlotBase GetSlot(DWORD dwSlot) { 
        PREFAST_DEBUGCHK(m_pSlots);
        DEBUGCHK(dwSlot < m_cSlots);
        return &m_pSlots[dwSlot];
    }

    // Take and release the host controller critical section.
    virtual VOID Lock()   { SDHCDAcquireHCLock(m_pHCDContext); }
    virtual VOID Unlock() { SDHCDReleaseHCLock(m_pHCDContext); }

    // Return TRUE if the give slot count is acceptable.
    static inline BOOL CheckSlotCount(DWORD dwSlots) {
        return (dwSlots != 0) && (dwSlots <= SDHC_MAX_SLOTS);
    }


#ifdef DEBUG
    static const LPCTSTR sc_rgpszOptions[SDHCDSlotOptionCount];

    // Validate the member data.
    virtual VOID Validate();

    // Validate the slot count.
    virtual VOID ValidateSlotCount() {
        DEBUGCHK(CheckSlotCount(m_cSlots));
    }
#else
    // These routines do nothing in non-debug builds.
    inline VOID Validate() {}
    inline VOID ValidateSlotCount() {}
#endif


    CReg                    m_regDevice;                    // device key
    HANDLE                  m_hBusAccess;                   // bus parent
    DWORD                   m_cSlots;                       // number of slots on controller
    PCSDHCSlotBase          m_pSlots;                       // dynamic array of slot objects    
    PSDHC_SLOT_INFO         m_pSlotInfos;                   // dynamic arry of info on each slot

    PSDCARD_HC_CONTEXT      m_pHCDContext;                  // the host controller context
    INTERFACE_TYPE          m_interfaceType;                // interface of the controller
    DWORD                   m_dwBusNumber;                  // bus number of the controller

    HANDLE                  m_hISRHandler;                  // handle to the ISR
    DWORD                   m_dwSysIntr;                    // system interrupt

    DWORD                   m_dwPriority;                   // IST priority
    HANDLE                  m_hevInterrupt;                 // controller interrupt event
    HANDLE                  m_htIST;                        // controller interrupt thread

    CEDEVICE_POWER_STATE    m_cpsCurrent;                   // current power state
    BOOL                    m_fDriverShutdown;              // driver is terminating

    BOOL                    m_fHardwareInitialized : 1;     // InitializeHardware() succeeded 
    BOOL                    m_fRegisteredWithBusDriver : 1; // SDHCDRegisterHostController() succeeded
    BOOL                    m_fInterruptInitialized : 1;    // InterruptInitialize() succeeded
}*PCSDHCBase;


// Called by SHC_Init to create the instance of CSDHCBase. Define to
// create your subclass.
extern "C" PCSDHCBase CreateSDHCControllerObject();

// Called by SHC_Deinit to free the instance of CSDHCBase. Define to
// free your subclass.
extern "C" VOID DestroySDHCControllerObject(PCSDHCBase pSDHC);



#define SDHC_INTERRUPT_ZONE    SDCARD_ZONE_0
#define SDHC_SEND_ZONE         SDCARD_ZONE_1
#define SDHC_RESPONSE_ZONE     SDCARD_ZONE_2
#define SDHC_RECEIVE_ZONE      SDCARD_ZONE_3
#define SDHC_CLOCK_ZONE        SDCARD_ZONE_4
#define SDHC_TRANSMIT_ZONE     SDCARD_ZONE_5

#define SDHC_INTERRUPT_ZONE_ON ZONE_ENABLE_0
#define SDHC_SEND_ZONE_ON      ZONE_ENABLE_1
#define SDHC_RESPONSE_ZONE_ON  ZONE_ENABLE_2
#define SDHC_RECEIVE_ZONE_ON   ZONE_ENABLE_3
#define SDHC_CLOCK_ZONE_ON     ZONE_ENABLE_4
#define SDHC_TRANSMIT_ZONE_ON  ZONE_ENABLE_5

#define SDHC_CARD_CONTROLLER_PRIORITY   100
#define SDHC_PRIORITY_KEY               _T("Priority256")
#define SDHC_CREATION_PROC_KEY          _T("ObjectCreationProc")

#define INVALID_BUS_NUMBER              0xFFFFFFFF


#define GET_PCONTROLLER_FROM_HCD(pHCDContext) \
    GetExtensionFromHCDContext(PCSDHCBase, pHCDContext)


#endif // _SDHC_H_

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

