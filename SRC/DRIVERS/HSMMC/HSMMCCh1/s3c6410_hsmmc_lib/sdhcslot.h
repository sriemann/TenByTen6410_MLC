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

#ifndef _SDHCSLOT_DEFINED
#define _SDHCSLOT_DEFINED

#include "SDCardDDK.h"
#include "SDHCD.h"
#include <ceddk.h>
#include "SDHCRegs.h"
#include "Sdhcdma.hpp"
#include <s3c6410.h>

#define SDHC_MAX_POWER_SUPPLY_RAMP_UP   250     // SD Phys Layer 6.6
#define NUM_BYTE_FOR_POLLING_MODE 0x800
#define POLLING_MODE_TIMEOUT_DEFAULT    2

#define SDHC_POWER_UP_DELAY_KEY         _T("PowerUpDelay")
#define SDHC_WAKEUP_SOURCES_KEY         _T("DefaultWakeupSources")
#define SDHC_CAN_WAKE_ON_INT_KEY        _T("AllowWakeOnSDIOInterrupts")
#define SDHC_FREQUENCY_KEY              _T("BaseClockFrequency")
#define SDHC_TIMEOUT_FREQUENCY_KEY      _T("TimeoutClockFrequency")
#define SDHC_TIMEOUT_KEY                _T("TimeoutInMS")
#define SDHC_POLLINGMODE_SIZE           _T("PollingModeSize")
#define SDHC_POLLINGMODE_TIMEOUT        _T("PollingModeTiemout")
#define SDHC_DISABLE_DMA_KEY            _T("DisableDMA")
#ifdef _SMDK6410_CH1_EXTCD_
// The enumeration for factors is used in card detect interrupt handler of HSMMC ch1 on SMRP6400.
typedef enum  _SDSLOT_INT_TYPE {
    SDSLOT_INT_NULL = 0,
    SDSLOT_INT_CARD_DETECTED  = 1,
} SDSLOT_INT_TYPE, *PSDSLOT_INT_TYPE;
#endif

typedef class CSDHCSlotBase {
    friend class CSDHCSlotBaseDMA;
    friend class CSDHCSlotBaseSDMA;
    friend class CSDHCSlotBase32BitADMA2;
public:
    // Constructor - only initializes the member data. True initialization
    // occurs in Init().
    CSDHCSlotBase();
    virtual ~CSDHCSlotBase();

    // Perform basic initialization including initializing the hardware
    // so that the capabilities register can be read.
    virtual BOOL Init(DWORD dwSlot, volatile BYTE *pbRegisters,
        PSDCARD_HC_CONTEXT pHCDContext, DWORD dwSysIntr, HANDLE hBusAccess,
        INTERFACE_TYPE interfaceType, DWORD dwBusNumber, CReg *pregDevice);

    // Second stage of hardware initialization. Complete slot configuration
    // and enable interrupts.
    virtual SD_API_STATUS Start();
    // Signals card removal disables the slot.
    virtual SD_API_STATUS Stop();

    // Process a slot option call.
    virtual SD_API_STATUS SlotOptionHandler(SD_SLOT_OPTION_CODE sdOption, 
        PVOID pData, DWORD cbData);

    // Get this slot's power state.
    virtual CEDEVICE_POWER_STATE GetPowerState() { return m_cpsCurrent; }

    // What power state is required upon power up?
    virtual CEDEVICE_POWER_STATE GetPowerUpRequirement() { return m_cpsAtPowerDown; }

    // Called when the device is suspending.
    virtual VOID PowerDown();
    // Called when the device is resuming.
    virtual VOID PowerUp();

    // Start this bus request.
    virtual SD_API_STATUS BusRequestHandler(PSD_BUS_REQUEST pRequest);

    // Returns TRUE if the interrupt routine needs servicing, say at
    // initialization to see if a card is present.
    virtual BOOL NeedsServicing() { return m_fCheckSlot; }

#ifndef _SMDK6410_CH1_EXTCD_
    // Handle a slot interrupt. Also called when NeedsServicing() returns TRUE.
    virtual VOID HandleInterrupt();
#else
    virtual VOID HandleInterrupt(SDSLOT_INT_TYPE intType = SDSLOT_INT_NULL);
#endif
    // In order to prevent infinite CARD INT occuring, below code is needed because of the architecture of HSMMC on s3c6410.
    inline BOOL IsOnlySDIOInterrupt() {
        WORD wIntStatus = ReadWord(SDHC_NORMAL_INT_STATUS);
        if (wIntStatus == NORMAL_INT_STATUS_CARD_INT && m_isSDIOInterrupt == TRUE ) {
            return TRUE;
        }
        return FALSE;
    }

    // Called by the controller to get the controller interrupt register.
    inline WORD ReadControllerInterrupts() {
        // We must use NORMAL_INT_STATUS instead of SLOT_INT_STATUS because SLOT_INT_STATUS is not apply to our controller.
        return ReadWord(SDHC_NORMAL_INT_STATUS);
    }

protected:
    virtual SD_API_STATUS SubmitBusRequestHandler(PSD_BUS_REQUEST pRequest );

    // What is this slot's maximum clock rate?
    virtual DWORD DetermineMaxClockRate();

    // What is this slot's maximum block length?
    virtual DWORD DetermineMaxBlockLen();

    // What should this slot use for timeout control?
    virtual DWORD DetermineTimeoutControl();

    // What are the default wakeup sources?
    virtual DWORD DetermineWakeupSources();
    
    // Set the slot voltage.
    virtual VOID SetVoltage(DWORD dwVddSetting);
    // Set the bus width and clock rate.
    virtual VOID SetInterface(PSD_CARD_INTERFACE_EX pInterface);

    // Set this slot's power state.
    virtual VOID SetPowerState(CEDEVICE_POWER_STATE cpsNew);

    // Get the capabilities register.
    virtual SSDHC_CAPABILITIES GetCapabilities() {
        SSDHC_CAPABILITIES caps;
        caps.dw = ReadDword(SDHC_CAPABILITIES);
        return caps;
    }

    virtual SSDHC_VERSION GetSDHCVersion() {
        SSDHC_VERSION version;
        version.uw = ReadWord(SDHC_HOST_CONTROLLER_VER);
        return version;
    }
    // Fill in the slot info structure.
    virtual SD_API_STATUS GetSlotInfo(PSDCARD_HC_SLOT_INFO pSlotInfo);

    // Get the desired Vdd window.
    virtual DWORD GetDesiredVddWindow();

    // Get the max Vdd window.
    virtual DWORD GetMaxVddWindow();

    // Is the card write-protected?
    virtual BOOL IsWriteProtected();

    // Enable/disable SDIO card interrupts.
    virtual VOID EnableSDIOInterrupts(BOOL fEnable);

    // How much extra time in ms for initial clocks is needed upon
    // insertion of a card for the power supply ramp up?
    virtual DWORD GetPowerSupplyRampUpMs() {
        return m_pregDevice->ValueDW(SDHC_POWER_UP_DELAY_KEY,
            SDHC_MAX_POWER_SUPPLY_RAMP_UP);
    }
    
    // Register access routines. These are not virtual so that we get
    // good inline read/write perf.
    template <class T>
    inline VOID WriteReg   (DWORD dwOffset, T tValue) {
        CheckRegOffset(dwOffset, sizeof(T));
        volatile T *ptRegister = (volatile T *) (m_pbRegisters + dwOffset);
        *ptRegister = tValue;
    }
    template <class T>
    inline T ReadReg       (DWORD dwOffset) {
        CheckRegOffset(dwOffset, sizeof(T));
        volatile T *ptRegister = (volatile T *) (m_pbRegisters + dwOffset);
        return *ptRegister;
    }

    inline BYTE  ReadByte  (DWORD dwOffset) {
        return ReadReg<BYTE>(dwOffset);
    }
    inline VOID  WriteByte (DWORD dwOffset, BYTE bValue) {
        WriteReg(dwOffset, bValue);
    }

    inline WORD  ReadWord  (DWORD dwOffset) {
        return ReadReg<WORD>(dwOffset);
    }
    inline VOID  WriteWord (DWORD dwOffset, WORD wValue) {
        WriteReg(dwOffset, wValue);
    }

    inline DWORD ReadDword (DWORD dwOffset) {
        return ReadReg<DWORD>(dwOffset);
    }
    inline VOID  WriteDword(DWORD dwOffset, DWORD dwValue) {
        WriteReg(dwOffset, dwValue);
    }


    // Interrupt handling methods
    virtual VOID HandleRemoval(BOOL fCancelRequest);
    virtual VOID HandleInsertion();
    virtual BOOL HandleCommandComplete();
    virtual VOID HandleErrors();
    virtual VOID HandleTransferDone();
    virtual VOID HandleReadReady();
    virtual VOID HandleWriteReady();
    virtual PVOID SlotAllocDMABuffer(ULONG Length,PPHYSICAL_ADDRESS  LogicalAddress,BOOLEAN CacheEnabled );
    virtual BOOL SlotFreeDMABuffer( ULONG Length,PHYSICAL_ADDRESS  LogicalAddress,PVOID VirtualAddr,BOOLEAN CacheEnabled );

    // Allocates a physical buffer for DMA.
    virtual PVOID AllocPhysBuffer(size_t cb, PDWORD pdwPhysAddr);

    // Frees the physical buffer.
    virtual VOID FreePhysBuffer(PVOID pv);

    // Place the slot into the desired power state.
    virtual VOID SetHardwarePowerState(CEDEVICE_POWER_STATE cpsNew);

    // Performs the actual enabling/disabling of SDIO card interrupts.
    virtual VOID DoEnableSDIOInterrupts(BOOL fEnable);

    // Perform the desired reset and wait for completion. Returns FALSE
    // if there is a timeout.
    virtual BOOL SoftwareReset(BYTE bResetBits);

    // Keep reading the register using (*pfnReadReg)(dwRegOffset) until
    // value & tMask == tWaitForEqual.
    template<class T>
    BOOL WaitForReg(
        T (CSDHCSlotBase::*pfnReadReg)(DWORD),
        DWORD dwRegOffset,
        T tMask,
        T tWaitForEqual,
        DWORD dwTimeout = 1000        
        );

    // Turn the LED on or off.
    virtual VOID EnableLED(BOOL fEnable);

    // Calls SDHCDIndicateSlotStateChange.
    virtual VOID IndicateSlotStateChange(SD_SLOT_EVENT sdEvent);

    // Calls SDHCDGetAndLockCurrentRequest.
    virtual PSD_BUS_REQUEST GetAndLockCurrentRequest();

    // Calls SDHCDPowerUpDown.
    virtual VOID PowerUpDown(BOOL fPowerUp, BOOL fKeepPower);

    // Calls SDHCDIndicateBusRequestComplete.
    virtual VOID IndicateBusRequestComplete(PSD_BUS_REQUEST pRequest, SD_API_STATUS status);

    // Finds the closest rate that is *pdwRate or lower. Stores the
    // actual rate in *pdwRate.
    virtual VOID SetClockRate(PDWORD pdwRate);

    // Turn on the SD clock according to the clock divisor found 
    // in SetClockRate().
    virtual VOID SDClockOn();

    // Turn off the SD clock.
    virtual VOID SDClockOff();

    // Determine the Vdd windows from the capabilities register.
    virtual DWORD DetermineVddWindows();

    // Set an interrupt event.
    virtual VOID SetInterruptEvent() { ::SetInterruptEvent(m_dwSysIntr); }

    virtual BOOL DetermineCommandPolling();
    virtual BOOL PollingForCommandComplete();
#ifdef DEBUG
    // Print out the standard host register set.
    virtual VOID DumpRegisters();

    // Validate the member data.
    virtual VOID Validate();

    // Verify that the desired register accesses are properly aligned.
    VOID CheckRegOffset(DWORD dwOffset, DWORD dwRegSize) {
        DEBUGCHK( (dwOffset % dwRegSize) == 0);
        DEBUGCHK(dwOffset < sizeof(SSDHC_REGISTERS));
        DEBUGCHK( (dwOffset + dwRegSize) <= sizeof(SSDHC_REGISTERS));
    }
#else
    // These routines do nothing in non-debug builds.
    inline VOID DumpRegisters() {}
    inline VOID Validate() {}
    inline VOID CheckRegOffset(DWORD dwOffset, DWORD dwRegSize) {}
#endif

#ifdef _SMDK6410_CH1_EXTCD_
        // Prototype for a new function can detect whether card is presented of HSMMC ch1 on SMRP6400.
        virtual BOOL IsCardPresent();
#endif
    CReg                   *m_pregDevice;           // pointer to device registry key
    CSDHCSlotBaseDMA        *m_SlotDma;             // DMA object
    DWORD                   m_dwSlot;               // physical slot number
    volatile BYTE          *m_pbRegisters;          // memory-mapped registers

    PSDCARD_HC_CONTEXT      m_pHCDContext;          // host context
    DWORD                   m_dwSysIntr;            // system interrupt
    HANDLE                  m_hBusAccess;           // bus parent
    INTERFACE_TYPE          m_interfaceType;        // interface of the controller
    DWORD                   m_dwBusNumber;          // bus number of the controller

    DWORD                   m_dwVddWindows;         // supported VDD windows
    DWORD                   m_dwMaxClockRate;       // maximum clock rate
    DWORD                   m_dwTimeoutControl;     // timeout control value
    DWORD                   m_dwMaxBlockLen;        // maximum block length

    WORD                    m_wRegClockControl;     // register value of Clock Control
    WORD                    m_wIntSignals;          // saved int signals for powerup
    CEDEVICE_POWER_STATE    m_cpsCurrent;           // current power state
    CEDEVICE_POWER_STATE    m_cpsAtPowerDown;       // power state at PowerDown()

    BOOL                    m_isSDIOInterrupt;

    DWORD                   m_dwDefaultWakeupControl;   // wakeup source list 
    BYTE                    m_bWakeupControl;           // current wakeup interrupts

#ifdef DEBUG
    DWORD                   m_dwReadyInts;          // number of Read/WriteReady interrupts that have occurred
#endif DEBUG

    BOOL                    m_fCommandCompleteOccurred;     // has the Command Complete occurred for the current transfer?

    PSD_BUS_REQUEST         m_pCurrentRequest; // Current Processing Request.
    BOOL                    m_fCurrentRequestFastPath;
    SD_API_STATUS           m_FastPathStatus;
    DWORD                   m_dwFastPathTimeoutTicks;

    DWORD                   m_dwPollingModeSize;

    BOOL                    m_fSleepsWithPower : 1;         // keep power in PowerDown()?
    BOOL                    m_fPowerUpDisabledInts : 1;     // did PowerUp disable SDIO card interrupts?
    BOOL                    m_fIsPowerManaged : 1;          // is the power manager handling us?
    BOOL                    m_fSDIOInterruptsEnabled : 1;   // are SDIO card interrupts enabled?
    BOOL                    m_fCardPresent : 1;             // is a card present
    BOOL                    m_fAutoCMD12Success : 1;        // AutoCMD12 success
    BOOL                    m_fCheckSlot : 1;               // does HandleInterrupt() need to be run?
    BOOL                    m_fCanWakeOnSDIOInterrupts : 1; // can wake on SDIO interrupts
    BOOL                    m_f4BitMode : 1;                // 4 bit bus mode?
    BOOL                    m_fFakeCardRemoval : 1;         // should we simulate card removal?    
    BOOL                    m_fCommandPolling: 1; 
    BOOL                    m_fDisableDMA:1;                // Disable The DMA
} *PCSDHCSlotBase;

#define CB_DMA_BUFFER 0x20000 // 128KB buffer
#define CB_DMA_PAGE   0x1000  // we program DMA for 4KB pages

#define TRANSFER_IS_WRITE(pRequest)        (SD_WRITE == (pRequest)->TransferClass)
#define TRANSFER_IS_READ(pRequest)         (SD_READ == (pRequest)->TransferClass)
#define TRANSFER_IS_COMMAND_ONLY(pRequest) (SD_COMMAND == (pRequest)->TransferClass)      

#define TRANSFER_SIZE(pRequest)            ((pRequest)->BlockSize * (pRequest)->NumBlocks)


#define SDHC_DEFAULT_TIMEOUT                2000 // 2 seconds


// Is this request an SDIO abort (CMD52, Function 0, I/O Abort Reg)?
inline
BOOL
TransferIsSDIOAbort(
                    PSD_BUS_REQUEST pRequest
                    )
{
    PREFAST_DEBUGCHK(pRequest);

    BOOL fRet = FALSE;
    
    if (pRequest->CommandCode == SD_CMD_IO_RW_DIRECT) {
        if (IO_RW_DIRECT_ARG_FUNC(pRequest->CommandArgument) == 0) {
            if (IO_RW_DIRECT_ARG_ADDR(pRequest->CommandArgument) == SD_IO_REG_IO_ABORT) {
                fRet = TRUE;
            }
        }
    }

    return fRet;
}

#endif // _SDHCSLOT_DEFINED

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

