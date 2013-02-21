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

#include <bsp.h>

#include "SDHC.h"
#include "SDHCSlot.h"

static volatile BSP_ARGS *v_gBspArgs;
#define CARD_INSERTED 1
#define CARD_REMOVED 2
#define _SRCCLK_48MHZ_  // USB_PHY 48MHZ Clock. Keep sync with "hsmmc_s3c6410.cpp"

// Macros
#define DX_D1_OR_D2(cps)            ((cps) == D1 || (cps) == D2)
#define SETFNAME()  LPCTSTR pszFname = _T(__FUNCTION__) _T(":")

#ifdef DEBUG
// dump the current request info to the debugger
static 
VOID 
DumpRequest(
            PSD_BUS_REQUEST pRequest,
            DWORD dwZone
            )
{   
    PREFAST_DEBUGCHK(pRequest);

    if (dwZone) {
        DEBUGMSG(1, (TEXT("DumpCurrentRequest: 0x%08X\n"), pRequest)); 
        DEBUGMSG(1, (TEXT("\t Command: %d\n"),  pRequest->CommandCode));
        DEBUGMSG(1, (TEXT("\t Argument: 0x%08x\n"),  pRequest->CommandArgument));
        DEBUGMSG(1, (TEXT("\t ResponseType: %d\n"),  pRequest->CommandResponse.ResponseType)); 
        DEBUGMSG(1, (TEXT("\t NumBlocks: %d\n"),  pRequest->NumBlocks)); 
        DEBUGMSG(1, (TEXT("\t BlockSize: %d\n"),  pRequest->BlockSize)); 
        DEBUGMSG(1, (TEXT("\t HCParam: %d\n"),    pRequest->HCParam)); 
    }
}
#else
#define DumpRequest(ptr, dw)
#endif


CSDHCSlotBase::CSDHCSlotBase(
        )
{
    m_pregDevice = NULL;
    m_SlotDma = NULL;
    m_dwSlot = 0;
    m_pbRegisters = NULL;
    m_pHCDContext = NULL;
    m_dwSysIntr = 0;
    m_hBusAccess = NULL;
    m_interfaceType = InterfaceTypeUndefined;
    m_dwBusNumber = 0;  

    m_dwVddWindows = 0;
    m_dwMaxClockRate = 0;
    m_dwTimeoutControl = 0;
    m_dwMaxBlockLen = 0;

    m_wRegClockControl = 0; 
    m_wIntSignals = 0;
    m_cpsCurrent = D0;
    m_cpsAtPowerDown = D0;

    m_dwDefaultWakeupControl = 0;
    m_bWakeupControl = 0;

#ifdef DEBUG
    m_dwReadyInts = 0;
#endif
    m_fCommandCompleteOccurred = FALSE;

    m_fSleepsWithPower = FALSE;
    m_fPowerUpDisabledInts = FALSE;
    m_fIsPowerManaged = FALSE;
    m_fSDIOInterruptsEnabled = FALSE;
    m_fCardPresent = FALSE;
    m_fAutoCMD12Success = FALSE;
    m_fCheckSlot = TRUE;
    m_fCanWakeOnSDIOInterrupts = FALSE;
    m_f4BitMode = FALSE;
    m_fFakeCardRemoval = FALSE;

    m_pCurrentRequest = NULL;
    m_fCurrentRequestFastPath = FALSE;
    m_FastPathStatus = SD_API_STATUS_SUCCESS;
    
    m_fCommandPolling = TRUE;
    m_fDisableDMA = FALSE;

    m_dwPollingModeSize = NUM_BYTE_FOR_POLLING_MODE ;
}


CSDHCSlotBase::~CSDHCSlotBase(
        )
{
    if (m_SlotDma)
        delete m_SlotDma;
}


BOOL
CSDHCSlotBase::Init(
        DWORD               dwSlot,
        volatile BYTE      *pbRegisters,
        PSDCARD_HC_CONTEXT  pHCDContext,
        DWORD               dwSysIntr,
        HANDLE              hBusAccess,
        INTERFACE_TYPE      interfaceType, 
        DWORD               dwBusNumber,
        CReg               *pregDevice
        )
{
    BOOL fRet = TRUE;

    DEBUGCHK(dwSlot < SDHC_MAX_SLOTS);
    DEBUGCHK(pbRegisters);
    DEBUGCHK(pHCDContext);
    DEBUGCHK(hBusAccess);
    PREFAST_DEBUGCHK(pregDevice && pregDevice->IsOK());
    PREFAST_DEBUGCHK(m_SlotDma==NULL);

    m_dwSlot = dwSlot;
    m_pbRegisters = pbRegisters;
    m_pHCDContext = pHCDContext;
    m_dwSysIntr = dwSysIntr;
    m_hBusAccess = hBusAccess;
    m_interfaceType = interfaceType;
    m_dwBusNumber = dwBusNumber;
    m_pregDevice = pregDevice;

    fRet = SoftwareReset(SOFT_RESET_ALL);
    if (fRet) { 
        Sleep(10); // Allow time for card to power down after a device reset

        SSDHC_CAPABILITIES caps = GetCapabilities();

        DEBUGMSG(SDCARD_ZONE_INIT && caps.bits.SDMA,
                (_T("SDHC Will use DMA for slot %u\n"), m_dwSlot));

        m_dwVddWindows = DetermineVddWindows();
        m_dwMaxClockRate = DetermineMaxClockRate();
        m_dwMaxBlockLen = DetermineMaxBlockLen();
        m_dwTimeoutControl = DetermineTimeoutControl();
        m_dwDefaultWakeupControl = DetermineWakeupSources();
        m_fCanWakeOnSDIOInterrupts = m_pregDevice->ValueDW(SDHC_CAN_WAKE_ON_INT_KEY);
        m_dwPollingModeSize = m_pregDevice->ValueDW(SDHC_POLLINGMODE_SIZE, NUM_BYTE_FOR_POLLING_MODE);
        m_dwFastPathTimeoutTicks = m_pregDevice->ValueDW(SDHC_POLLINGMODE_TIMEOUT,POLLING_MODE_TIMEOUT_DEFAULT);
        m_fDisableDMA = (m_pregDevice->ValueDW(SDHC_DISABLE_DMA_KEY,0)!=0);
        Validate();

        DumpRegisters();
    }

        PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
        // Below code lines are needed for working as a mass storage device
        if (v_gBspArgs == NULL)
        {
            ioPhysicalBase.LowPart = IMAGE_SHARE_ARGS_PA_START;
            v_gBspArgs = (volatile BSP_ARGS *)MmMapIoSpace(ioPhysicalBase, sizeof(BSP_ARGS), FALSE);
            if (v_gBspArgs == NULL)
            {
                RETAILMSG(TRUE, (TEXT("[HSMMC1] HSMMC MmMapIoSpace: FAILED\r\n")));
                if (v_gBspArgs)
                {
                    MmUnmapIoSpace((PVOID) v_gBspArgs, sizeof(BSP_ARGS));    
                    v_gBspArgs = NULL;
                }
                return FALSE;
            }

            v_gBspArgs->g_SDCardState = CARD_REMOVED;    //initialize
            v_gBspArgs->g_SDCardDetectEvent = CreateEvent(NULL, FALSE, FALSE,NULL);

    }
    return fRet;
}


SD_API_STATUS
CSDHCSlotBase::Start(
        )
{
    Validate();

    SD_API_STATUS status;

    if (SoftwareReset(SOFT_RESET_ALL)) {
        // timeout control
        WriteByte(SDHC_TIMEOUT_CONTROL, (BYTE) m_dwTimeoutControl);

        // Enable error interrupt status and signals for all but the vendor
        // errors. This allows any normal error to generate an interrupt.
        WriteWord(SDHC_ERROR_INT_STATUS_ENABLE, ~0 & ~ERR_INT_STATUS_VENDOR);
        WriteWord(SDHC_ERROR_INT_SIGNAL_ENABLE, ~0 & ~ERR_INT_STATUS_VENDOR);

        // disable the interrupt signals on the FIFO.
        WriteWord(SDHC_NORMAL_INT_SIGNAL_ENABLE, 0x1FF);
        WriteWord(SDHC_NORMAL_INT_STATUS_ENABLE,
                NORMAL_INT_ENABLE_CARD_INSERTION | NORMAL_INT_ENABLE_CARD_REMOVAL);

        status = SD_API_STATUS_SUCCESS;
    }
    else {
        status = SD_API_STATUS_DEVICE_NOT_RESPONDING;
    }

    return status;
}


SD_API_STATUS
CSDHCSlotBase::Stop(
        )
{    
    if (m_fCardPresent) {
        // Remove device
        HandleRemoval(FALSE);
    }

    SoftwareReset(SOFT_RESET_ALL);

    // We could notify the USBFn driver here to detach from the USB bus if 
    // Mass Storage UsbFn is selected with SD card as the store
    if(NULL != v_gBspArgs->g_SDCardDetectEvent) 
    {
        CloseHandle(v_gBspArgs->g_SDCardDetectEvent);
        v_gBspArgs->g_SDCardDetectEvent = NULL;
    }
    if (v_gBspArgs)
    {
        MmUnmapIoSpace((PVOID) v_gBspArgs, sizeof(BSP_ARGS));    
        v_gBspArgs = NULL;
    }
    
    return SD_API_STATUS_SUCCESS;
}


SD_API_STATUS 
CSDHCSlotBase::GetSlotInfo(
        PSDCARD_HC_SLOT_INFO pSlotInfo
        )
{
    PREFAST_DEBUGCHK(pSlotInfo);
    DEBUGCHK(m_pregDevice->IsOK());

    // set the slot capabilities
    DWORD dwCap = SD_SLOT_SD_4BIT_CAPABLE | 
        SD_SLOT_SD_1BIT_CAPABLE | 
#ifdef _SMDK6410_CH1_8BIT_
        // In order to distinguish Ch1 from Ch0 and 2, a capability for DAT 8-Bit on MMCplus is added.
        SD_SLOT_SD_8BIT_CAPABLE | 
#endif
        SD_SLOT_SDIO_CAPABLE    |
        SD_SLOT_SDIO_INT_DETECT_4BIT_MULTI_BLOCK;

    if (GetCapabilities().bits.HighSpeed){
        dwCap |= SD_SLOT_HIGH_SPEED_CAPABLE;
    }

    SDHCDSetSlotCapabilities(pSlotInfo,dwCap );

    SDHCDSetVoltageWindowMask(pSlotInfo, m_dwVddWindows); 

    // Set optimal voltage
    SDHCDSetDesiredSlotVoltage(pSlotInfo, GetDesiredVddWindow());     

    // Controller may be able to clock at higher than the max SD rate,
    // but we should only report the highest rate in the range.
    DWORD dwMaxClockRateInSDRange = SD_FULL_SPEED_RATE;
    SetClockRate(&dwMaxClockRateInSDRange);
    SDHCDSetMaxClockRate(pSlotInfo, dwMaxClockRateInSDRange);

    // Set power up delay. We handle this in SetVoltage().
    SDHCDSetPowerUpDelay(pSlotInfo, 1);

    return SD_API_STATUS_SUCCESS;
}


SD_API_STATUS
CSDHCSlotBase::SlotOptionHandler(
        SD_SLOT_OPTION_CODE   sdOption, 
        PVOID                 pData,
        DWORD                 cbData
        )
{
    SD_API_STATUS status = SD_API_STATUS_SUCCESS;

    switch (sdOption) {
        case SDHCDSetSlotPower: {
                                    DEBUGCHK(cbData == sizeof(DWORD));
                                    PDWORD pdwVddSetting = (PDWORD) pData;
                                    SetVoltage(*pdwVddSetting);
                                    break;
                                }

        case SDHCDSetSlotInterface: {
                                        DEBUGCHK(cbData == sizeof(SD_CARD_INTERFACE));
                                        PSD_CARD_INTERFACE pInterface = (PSD_CARD_INTERFACE) pData;

                                        DEBUGMSG(SDCARD_ZONE_INFO, 
                                                (TEXT("CSDHCSlotBase::SlotOptionHandler: Clock Setting: %d\n"), 
                                                 pInterface->ClockRate));
                                        SD_CARD_INTERFACE_EX sdCardInterfaceEx;
                                        memset(&sdCardInterfaceEx,0, sizeof(sdCardInterfaceEx));

                                        sdCardInterfaceEx.InterfaceModeEx.bit.sd4Bit = (((pInterface->InterfaceMode) == SD_INTERFACE_SD_4BIT)? 1:0);
                                        // An interface for DAT 8-Bit on MMCplus is added.
                                        sdCardInterfaceEx.InterfaceModeEx.bit.hsmmc8Bit = (((pInterface->InterfaceMode) == SD_INTERFACE_MMC_8BIT)? 1:0);

                                        sdCardInterfaceEx.ClockRate = pInterface->ClockRate;
                                        sdCardInterfaceEx.InterfaceModeEx.bit.sdWriteProtected = (pInterface->WriteProtected?1:0);
                                        SetInterface(&sdCardInterfaceEx);
                                        // Update the argument back.
                                        // The interface for DAT 8-Bit on MMCplus must be checked prior to orther interfaces.
                                        if (sdCardInterfaceEx.InterfaceModeEx.bit.hsmmc8Bit != 0) {
                                            pInterface->InterfaceMode = SD_INTERFACE_MMC_8BIT;
                                        } else {
                                            pInterface->InterfaceMode = (sdCardInterfaceEx.InterfaceModeEx.bit.sd4Bit!=0?SD_INTERFACE_SD_4BIT:SD_INTERFACE_SD_MMC_1BIT);
                                        }
                                        pInterface->ClockRate =  sdCardInterfaceEx.ClockRate;
                                        pInterface->WriteProtected = (sdCardInterfaceEx.InterfaceModeEx.bit.sdWriteProtected!=0?TRUE:FALSE);
                                        break;
                                    }
        case SDHCDSetSlotInterfaceEx: {
                                          DEBUGCHK(cbData == sizeof(SD_CARD_INTERFACE_EX));
                                          PSD_CARD_INTERFACE_EX pInterface = (PSD_CARD_INTERFACE_EX) pData;

                                          DEBUGMSG(SDCARD_ZONE_INFO, 
                                                  (TEXT("CSDHCSlotBase::SlotOptionHandler: Clock Setting: %d\n"), 
                                                   pInterface->ClockRate));

                                          SetInterface((PSD_CARD_INTERFACE_EX)pInterface);
                                      }

        case SDHCDEnableSDIOInterrupts:
        case SDHCDAckSDIOInterrupt:
                                      EnableSDIOInterrupts(TRUE);
                                      break;

        case SDHCDDisableSDIOInterrupts:
                                      EnableSDIOInterrupts(FALSE);
                                      break;

        case SDHCDGetWriteProtectStatus: {
                                             DEBUGCHK(cbData == sizeof(SD_CARD_INTERFACE));
                                             PSD_CARD_INTERFACE pInterface = (PSD_CARD_INTERFACE) pData;
                                             pInterface->WriteProtected = IsWriteProtected();
                                             break;
                                         }

        case SDHCDQueryBlockCapability: {
                                            DEBUGCHK(cbData == sizeof(SD_HOST_BLOCK_CAPABILITY));
                                            PSD_HOST_BLOCK_CAPABILITY pBlockCaps = 
                                                (PSD_HOST_BLOCK_CAPABILITY)pData;

                                            DEBUGMSG(SDCARD_ZONE_INFO, 
                                                    (TEXT("CSDHCSlotBase::SlotOptionHandler: Read Block Length: %d , Read Blocks: %d\n"), 
                                                     pBlockCaps->ReadBlockSize, pBlockCaps->ReadBlocks));
                                            DEBUGMSG(SDCARD_ZONE_INFO, 
                                                    (TEXT("CSDHCSlotBase::SlotOptionHandler: Write Block Length: %d , Write Blocks: %d\n"), 
                                                     pBlockCaps->WriteBlockSize, pBlockCaps->WriteBlocks));

                                            pBlockCaps->ReadBlockSize = max(pBlockCaps->ReadBlockSize, SDHC_MIN_BLOCK_LENGTH);
                                            pBlockCaps->ReadBlockSize = min(pBlockCaps->ReadBlockSize, (USHORT) m_dwMaxBlockLen);

                                            pBlockCaps->WriteBlockSize = max(pBlockCaps->WriteBlockSize, SDHC_MIN_BLOCK_LENGTH);
                                            pBlockCaps->WriteBlockSize = min(pBlockCaps->WriteBlockSize, (USHORT) m_dwMaxBlockLen);
                                            break;
                                        }

        case SDHCDGetSlotPowerState: {
                                         DEBUGCHK(cbData == sizeof(CEDEVICE_POWER_STATE));
                                         PCEDEVICE_POWER_STATE pcps = (PCEDEVICE_POWER_STATE) pData;
                                         *pcps = GetPowerState();
                                         break;
                                     }

        case SDHCDSetSlotPowerState: {
                                         DEBUGCHK(cbData == sizeof(CEDEVICE_POWER_STATE));
                                         PCEDEVICE_POWER_STATE pcps = (PCEDEVICE_POWER_STATE) pData;
                                         SetPowerState(*pcps);
                                         break;
                                     }

        case SDHCDWakeOnSDIOInterrupts: {
                                            DEBUGCHK(cbData == sizeof(BOOL));
                                            PBOOL pfWake = (PBOOL) pData;

                                            if (m_fCanWakeOnSDIOInterrupts) {
                                                DWORD dwWakeupControl = m_dwDefaultWakeupControl;

                                                if (*pfWake) {
                                                    dwWakeupControl |= WAKEUP_INTERRUPT;
                                                }

                                                m_bWakeupControl = (BYTE) dwWakeupControl;
                                            }
                                            else {
                                                status = SD_API_STATUS_UNSUCCESSFUL;
                                            }
                                            break;
                                        }

        case SDHCDGetSlotInfo: {
                                   DEBUGCHK(cbData == sizeof(SDCARD_HC_SLOT_INFO));
                                   PSDCARD_HC_SLOT_INFO pSlotInfo = (PSDCARD_HC_SLOT_INFO) pData;
                                   status = GetSlotInfo(pSlotInfo);
                                   break;
                               }

        case SDHCAllocateDMABuffer: {
                                        DEBUGCHK (cbData == sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER));
                                        PREFAST_ASSERT(pData!=NULL);
                                        PSD_HOST_ALLOC_FREE_DMA_BUFFER pSdDmaBuffer = (PSD_HOST_ALLOC_FREE_DMA_BUFFER)pData;
                                        pSdDmaBuffer->VirtualAddress = 
                                            SlotAllocDMABuffer(pSdDmaBuffer->Length, &pSdDmaBuffer->LogicalAddress,pSdDmaBuffer->CacheEnabled);
                                        status = (pSdDmaBuffer->VirtualAddress!=NULL? SD_API_STATUS_SUCCESS: SD_API_STATUS_BUFFER_OVERFLOW);
                                        break;
                                    }
        case SDHCFreeDMABuffer:{
                                   DEBUGCHK (cbData == sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER));
                                   PSD_HOST_ALLOC_FREE_DMA_BUFFER pSdDmaBuffer = (PSD_HOST_ALLOC_FREE_DMA_BUFFER)pData;
                                   BOOL fResult =
                                       SlotFreeDMABuffer(pSdDmaBuffer->Length, pSdDmaBuffer->LogicalAddress,pSdDmaBuffer->VirtualAddress,pSdDmaBuffer->CacheEnabled);
                                   status = (fResult? SD_API_STATUS_SUCCESS: SD_API_STATUS_INVALID_PARAMETER);
                                   break;
                               }
        default:
                               status = SD_API_STATUS_INVALID_PARAMETER;
    }
    return status;
}


DWORD
CSDHCSlotBase::DetermineMaxClockRate(
        )
{
    DEBUGCHK(m_pregDevice->IsOK());

    // We allow the registry to override what is in the capabilities register.
    DWORD dwMaxClockRate = m_pregDevice->ValueDW(SDHC_FREQUENCY_KEY);
    if (dwMaxClockRate == 0) {
        SSDHC_CAPABILITIES caps = GetCapabilities();

        dwMaxClockRate = caps.bits.ClkFreq * 1000000;
        if (dwMaxClockRate == 0) {
            // No clock frequency specified. Use the highest possible that
            // could have been specified so that a working clock divisor 
            // will be chosen.
            dwMaxClockRate = SDHC_MAX_CLOCK_FREQUENCY;
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC: No base clock frequency specified\n")));
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC: Using default frequency of %u\n"), 
                        dwMaxClockRate));
        }
    }

    return dwMaxClockRate;
}


DWORD
CSDHCSlotBase::DetermineMaxBlockLen(
        )
{
    static const USHORT sc_rgusBlockLen[] = { 
        SDHC_CAPABILITIES_MAX_BLOCK_LENGTH_0,
        SDHC_CAPABILITIES_MAX_BLOCK_LENGTH_1,
        SDHC_CAPABILITIES_MAX_BLOCK_LENGTH_2
    };

    SSDHC_CAPABILITIES caps = GetCapabilities();
    // Determine the maximum block length
    DWORD dwMaxBlockLen;
    if (caps.bits.MaxBlockLen < _countof(sc_rgusBlockLen)) {    
        dwMaxBlockLen = sc_rgusBlockLen[caps.bits.MaxBlockLen];
    }
    else { // We hit reserved bit by Standard SDHC specification. So, it is better to returns smallest one.
        ASSERT(FALSE);
        dwMaxBlockLen = SDHC_CAPABILITIES_MAX_BLOCK_LENGTH_0 ;
    }
    return dwMaxBlockLen;
}


DWORD
CSDHCSlotBase::DetermineTimeoutControl(
        )
{
    DEBUGCHK(m_pregDevice->IsOK());

    return SDHC_TIMEOUT_CONTROL_MAX;
}


DWORD 
CSDHCSlotBase::DetermineWakeupSources(
        )
{
    DEBUGCHK(m_pregDevice->IsOK());
    DWORD dwWakeupSources = m_pregDevice->ValueDW(SDHC_WAKEUP_SOURCES_KEY);
    dwWakeupSources &= WAKEUP_ALL_SOURCES;

    // Waking on SDIO interrupts must be enabled by the bus driver.
    dwWakeupSources &= ~WAKEUP_INTERRUPT; 

    return dwWakeupSources;
}


VOID
CSDHCSlotBase::SetVoltage(
        DWORD dwVddSetting
        )
{
    Validate();

    UCHAR ucVoltageSelection = SDBUS_POWER_ON;
    UCHAR ucOldVoltage;

    DEBUGCHK(dwVddSetting & m_dwVddWindows);

    if ( dwVddSetting & 
            (SD_VDD_WINDOW_3_2_TO_3_3 | SD_VDD_WINDOW_3_3_TO_3_4) ) {
        ucVoltageSelection |= SDBUS_VOLTAGE_SELECT_3_3V;
    }
    else if ( dwVddSetting & 
            (SD_VDD_WINDOW_2_9_TO_3_0 | SD_VDD_WINDOW_3_0_TO_3_1) ) {
        ucVoltageSelection |= SDBUS_VOLTAGE_SELECT_3_0V;
    }
    else if ( dwVddSetting & 
            (SD_VDD_WINDOW_1_7_TO_1_8 | SD_VDD_WINDOW_1_8_TO_1_9) ) {
        ucVoltageSelection |= SDBUS_VOLTAGE_SELECT_1_8V;
    }

    ucOldVoltage = ReadByte(SDHC_POWER_CONTROL);
    if (ucOldVoltage != ucVoltageSelection) {
        // SD Bus Power must be initially set to 0 when changing voltages
        WriteByte(SDHC_POWER_CONTROL, 0);
        WriteByte(SDHC_POWER_CONTROL, ucVoltageSelection);

        DEBUGMSG(SDCARD_ZONE_INFO,( 
                    TEXT("CSDHCSlotBase::SetVoltage: Set SDHC_POWER_CONTROL reg = 0x%02x\n"),
                    ucVoltageSelection));

        Sleep(GetPowerSupplyRampUpMs());
    }
}


// Set up the controller according to the interface parameters.
VOID 
CSDHCSlotBase::SetInterface(
        PSD_CARD_INTERFACE_EX pInterface
        )
{
    PREFAST_DEBUGCHK(pInterface);
    Validate();

    BYTE bHostCtr = 0;
#ifdef _MMC_SPEC_42_
    if (0 != pInterface->InterfaceModeEx.bit.hsmmc8Bit) {
        DEBUGMSG(SDCARD_ZONE_INIT, 
                (TEXT("SHCSDSlotOptionHandler - Setting for 8 bit mode\n")));
        RETAILMSG(TRUE, 
                (TEXT("[HSMMC1] Setting for 8 bit mode , Clock Rate = %d Hz\n"),pInterface->ClockRate));
        bHostCtr |= HOSTCTL_DAT_WIDTH_8BIT;
        m_f4BitMode = TRUE;
    } else if (SD_INTERFACE_SD_MMC_1BIT == pInterface->InterfaceModeEx.bit.sd4Bit) {
        DEBUGMSG(SDCARD_ZONE_INIT, 
                (TEXT("SHCSDSlotOptionHandler - Setting for 1 bit mode\n")));
        RETAILMSG(TRUE, 
                (TEXT("[HSMMC1] Setting for 1 bit mode , Clock Rate = %d Hz\n"),pInterface->ClockRate));        
        bHostCtr = 0;
        m_f4BitMode = FALSE;
    } else if (SD_INTERFACE_SD_4BIT == pInterface->InterfaceModeEx.bit.sd4Bit) {
        DEBUGMSG(SDCARD_ZONE_INIT, 
                (TEXT("SHCSDSlotOptionHandler - Setting for 4 bit mode\n")));
        RETAILMSG(TRUE, 
                (TEXT("[HSMMC1] Setting for 4 bit mode , Clock Rate = %d Hz\n"),pInterface->ClockRate));
        bHostCtr |= HOSTCTL_DAT_WIDTH;
        m_f4BitMode = TRUE;
    }

#else
    m_f4BitMode = (pInterface->InterfaceModeEx.bit.sd4Bit!=0);
    bHostCtr |= (m_f4BitMode?HOSTCTL_DAT_WIDTH:0);
    bHostCtr |= (pInterface->InterfaceModeEx.bit.sdHighSpeed!=0?HOSTCTL_HIGH_SPEED:0);
#endif

    if (m_SlotDma) {
        bHostCtr |= (m_SlotDma->DmaSelectBit()<<3);
    }
    DEBUGMSG(SDCARD_ZONE_INIT, 
            (TEXT("SHCSDSlotOptionHandler - Setting Host Control Register %x \n"),bHostCtr));
    WriteByte(SDHC_HOST_CONTROL, bHostCtr);

    SetClockRate(&pInterface->ClockRate);
}


VOID
CSDHCSlotBase::SetPowerState(
        CEDEVICE_POWER_STATE cpsNew
        )
{
    DEBUGCHK(VALID_DX(cpsNew));

    m_fIsPowerManaged = TRUE;

    if (DX_D1_OR_D2(cpsNew)) {
        cpsNew = D0;
    }

    if (m_cpsCurrent != cpsNew) {
        SetHardwarePowerState(cpsNew);
    }
}


VOID
CSDHCSlotBase::PowerDown(
        )
{
    Validate();

    m_cpsAtPowerDown = m_cpsCurrent;

    if (!m_fIsPowerManaged) {
        CEDEVICE_POWER_STATE cps;

        if (m_bWakeupControl) {
            cps = D3;
        }
        else {
            cps = D4;
        }

        SetHardwarePowerState(cps);
    }

    BOOL fKeepPower = FALSE;
    if (m_fSleepsWithPower || m_cpsCurrent == D0) {
        DEBUGCHK(!m_fSleepsWithPower || m_cpsCurrent == D3);
        fKeepPower = TRUE;
    }
    else
        m_fFakeCardRemoval = TRUE;

    PowerUpDown(FALSE, fKeepPower);
}


VOID 
CSDHCSlotBase::PowerUp(
        )
{
    Validate();

    if (!m_fIsPowerManaged) {
        SetHardwarePowerState(m_cpsAtPowerDown);
    }
    else if (m_fSleepsWithPower) {
        WORD wIntStatus = ReadWord(SDHC_NORMAL_INT_STATUS);
        if (wIntStatus == NORMAL_INT_STATUS_CARD_INT) {
            // We woke system through a card interrupt. We need to clear
            // this so that the IST will not be signalled.
            EnableSDIOInterrupts(FALSE);
            m_fPowerUpDisabledInts = TRUE;
        }
    }

    PowerUpDown(TRUE, TRUE);
    if (m_fFakeCardRemoval){
        Start();
        SetInterruptEvent();
    }
}
//
// For BC, 
// 1. Returns SD_API_STATUS_FAST_PATH_SUCCESS, callback is NOT called. Fastpass only.
// 2. Return !SD_API_SUCCESS(status). callback is NOT called.
// 3. Return SD_API_STATUS_SUCCESS, callback is called
// 4. Return SD_API_STATUS_PENDING, callback is NOT call Yet.
//
    SD_API_STATUS
CSDHCSlotBase::BusRequestHandler( PSD_BUS_REQUEST pRequest)
{
    SETFNAME();
    SD_API_STATUS status;
    PREFAST_DEBUGCHK(pRequest);
    Validate();
    DEBUGMSG(SDHC_SEND_ZONE, (TEXT("%s CMD:%d\n"), pszFname, pRequest->CommandCode));

    if (m_pCurrentRequest) { // We have outstand request.
        ASSERT(FALSE);
        IndicateBusRequestComplete(pRequest, SD_API_STATUS_CANCELED);
        m_pCurrentRequest = NULL;
    }
    if (!m_fCardPresent) {
        status= SD_API_STATUS_DEVICE_REMOVED;
    }
    else {
        WORD wIntSignals = ReadWord(SDHC_NORMAL_INT_SIGNAL_ENABLE);
        WriteWord(SDHC_NORMAL_INT_SIGNAL_ENABLE,0);
        m_fCurrentRequestFastPath = FALSE ;
        m_pCurrentRequest = pRequest ;
        // if no data transfer involved, use FAST PATH
        if ((pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)!=0) { // Fastpath
            m_fCurrentRequestFastPath = TRUE;
            status = SubmitBusRequestHandler( pRequest );
            if( status == SD_API_STATUS_PENDING ) { // Polling for completion.
                BOOL fCardInserted = TRUE;
                DWORD dwStartTicks = GetTickCount();
#ifdef _SMDK6410_CH1_EXTCD_
                if (m_pCurrentRequest && 
                        (fCardInserted = (IsCardPresent() & TRUE)!=0) &&
                        GetTickCount() - dwStartTicks <= m_dwFastPathTimeoutTicks) {
#else
                while (m_pCurrentRequest &&
                        (fCardInserted = (ReadDword(SDHC_PRESENT_STATE) & STATE_CARD_INSERTED)!=0 ) &&
                        ((GetTickCount() - dwStartTicks) <= m_dwFastPathTimeoutTicks)) {
#endif
                    HandleInterrupt();
                }

                if (m_pCurrentRequest && fCardInserted ) {
                    // Time out , need to switch to asyn.it will call callback after this
                    pRequest->SystemFlags &= ~SD_FAST_PATH_AVAILABLE;
                    m_fCurrentRequestFastPath = FALSE ;
                }
                else { // Fastpass completed.
                    status = m_FastPathStatus;
                    // Clear before status of fastpath.
                    m_FastPathStatus = 0;
                    if (m_pCurrentRequest) {
                        ASSERT(FALSE);
                        status = SD_API_STATUS_DEVICE_REMOVED;
                    }
                }
            }
            if (status == SD_API_STATUS_SUCCESS) {
                status = SD_API_STATUS_FAST_PATH_SUCCESS;
            }
        }
        else
            status = SubmitBusRequestHandler( pRequest );

        if (status!=SD_API_STATUS_PENDING && m_pCurrentRequest) { 
            // if there is error case. We don't notify the callback function either So.
            m_fCurrentRequestFastPath = TRUE;
            IndicateBusRequestComplete(pRequest,status);
        }
        WriteWord(SDHC_NORMAL_INT_SIGNAL_ENABLE,wIntSignals);
    }

    return status;
}


    SD_API_STATUS
CSDHCSlotBase::SubmitBusRequestHandler(PSD_BUS_REQUEST pRequest)
{
    SETFNAME();

    PREFAST_DEBUGCHK(pRequest);
    Validate();

    WORD            wRegCommand;
    SD_API_STATUS   status;
    WORD            wIntStatusEn;
    BOOL            fSuccess;

    DEBUGCHK(m_dwReadyInts == 0);
    DEBUGCHK(!m_fCommandCompleteOccurred);

    DEBUGMSG(SDHC_SEND_ZONE, (TEXT("%s CMD:%d\n"), pszFname, pRequest->CommandCode));

    // bypass CMD12 if AutoCMD12 was done by hardware
    if (pRequest->CommandCode == 12) {
        if (m_fAutoCMD12Success) {
            DEBUGMSG(SDHC_SEND_ZONE, 
                    (TEXT("%s AutoCMD12 Succeeded, bypass CMD12.\n"), pszFname));
            // The response for Auto CMD12 is in a special area
            UNALIGNED DWORD *pdwResponseBuffer = 
                (PDWORD) (pRequest->CommandResponse.ResponseBuffer + 1); // Skip CRC
            *pdwResponseBuffer = ReadDword(SDHC_R6);
            IndicateBusRequestComplete(pRequest, SD_API_STATUS_SUCCESS);
            status = SD_API_STATUS_SUCCESS;
            goto EXIT;
        }
    }

    m_fAutoCMD12Success = FALSE;

    // initialize command register with command code
    wRegCommand = (pRequest->CommandCode << CMD_INDEX_SHIFT) & CMD_INDEX_MASK;

    // check for a response
    switch (pRequest->CommandResponse.ResponseType) {
        case NoResponse:
            break;

        case ResponseR2:
            wRegCommand |= CMD_RESPONSE_R2;
            break;

        case ResponseR3:
        case ResponseR4:
            wRegCommand |= CMD_RESPONSE_R3_R4;
            break;

        case ResponseR1:
        case ResponseR5:
        case ResponseR6:
        case ResponseR7:
            wRegCommand |= CMD_RESPONSE_R1_R5_R6_R7;
            break;

        case ResponseR1b:
            wRegCommand |= CMD_RESPONSE_R1B_R5B;   
            break;

        default:
            status = SD_API_STATUS_INVALID_PARAMETER;
            goto EXIT;
    }

    // Set up variable for the new interrupt sources. Note that we must
    // enable DMA and read/write interrupts in this routine (not in
    // HandleCommandComplete) or they will be missed.
    wIntStatusEn = ReadWord(SDHC_NORMAL_INT_STATUS_ENABLE);
    wIntStatusEn |= NORMAL_INT_ENABLE_CMD_COMPLETE | NORMAL_INT_ENABLE_TRX_COMPLETE;

    // check command inhibit, wait until OK
    fSuccess = WaitForReg<DWORD>(&CSDHCSlotBase::ReadDword, SDHC_PRESENT_STATE, STATE_CMD_INHIBIT, 0);
    if (!fSuccess) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Timeout waiting for CMD Inhibit\r\n"),
                    pszFname));
        status = SD_API_STATUS_DEVICE_NOT_RESPONDING;
        goto EXIT;
    }

    // programming registers
    if (!TRANSFER_IS_COMMAND_ONLY(pRequest)) {
        WORD wRegTxnMode = 0;        
#ifdef _MMC_SPEC_42_
        // To distinguish btween MMCmicro and MMCplus, we will issue MMC_CMD_SEND_EXT_CSD.
        // At that time, Dat line is 8bit. If the inserted card is MMCmicro, "Data timeout" error will be occurred.
        // Because MMCmicro does not supports 8bit DAT line but 4bit. To reduce the delay time on Data timeout error occurring,
        // we modify the timeout value.
        if ( (ReadByte(SDHC_TIMEOUT_CONTROL) != m_dwTimeoutControl) && 
                (pRequest->CommandCode != MMC_CMD_SEND_EXT_CSD) ) {
            WriteByte(SDHC_TIMEOUT_CONTROL, (BYTE) m_dwTimeoutControl);
        } else if ( (ReadByte(SDHC_TIMEOUT_CONTROL) == m_dwTimeoutControl) &&
                (pRequest->CommandCode == MMC_CMD_SEND_EXT_CSD) ) {
            WriteByte(SDHC_TIMEOUT_CONTROL, (BYTE)0x3);
        }
#endif
        wRegCommand |= CMD_DATA_PRESENT;

        if (m_SlotDma &&  m_SlotDma->ArmDMA(*pRequest,TRANSFER_IS_WRITE(pRequest))) {
            wIntStatusEn |= NORMAL_INT_ENABLE_DMA;
            wRegTxnMode |= TXN_MODE_DMA;
        }
        else {
            if (TRANSFER_IS_WRITE(pRequest)) {
                wIntStatusEn |= NORMAL_INT_ENABLE_BUF_WRITE_RDY;
            }
            else {
                wIntStatusEn |= NORMAL_INT_ENABLE_BUF_READ_RDY;
            }
        }

        // BlockSize
        // Note that for DMA we are programming the buffer boundary for 4k
        DEBUGMSG(SDHC_SEND_ZONE,(TEXT("Sending command block size 0x%04X\r\n"), (WORD) pRequest->BlockSize));
        ASSERT(PAGE_SIZE == 0x1000);
        WriteWord(SDHC_BLOCKSIZE, (WORD)(pRequest->BlockSize & 0xfff) | (0<<12)); // SDHC 2.2.2, CE is 4k-aligned page.

        // We always go into block mode even if there is only 1 block. 
        // Otherwise the Pegasus will occaissionally hang when
        // writing a single block with DMA.
        wRegTxnMode |= (TXN_MODE_MULTI_BLOCK | TXN_MODE_BLOCK_COUNT_ENABLE);

        // BlockCount
        DEBUGMSG(SDHC_SEND_ZONE,(TEXT("Sending command block count 0x%04X\r\n"), 
                    (WORD) pRequest->NumBlocks));            
        WriteWord(SDHC_BLOCKCOUNT, (WORD) pRequest->NumBlocks);

        if (pRequest->Flags & SD_AUTO_ISSUE_CMD12) {
            wRegTxnMode |= TXN_MODE_AUTO_CMD12;
        }

        if (TRANSFER_IS_READ(pRequest)) {
            wRegTxnMode |= TXN_MODE_DATA_DIRECTION_READ;     
        }

        // check dat inhibit, wait until okay
        fSuccess = WaitForReg<DWORD>(&CSDHCSlotBase::ReadDword, SDHC_PRESENT_STATE, STATE_DAT_INHIBIT, 0); 
        if (!fSuccess) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Timeout waiting for DAT Inhibit\r\n"),
                        pszFname));
            status = SD_API_STATUS_DEVICE_NOT_RESPONDING;
            goto EXIT;
        }

        DEBUGMSG(SDHC_SEND_ZONE,(TEXT("Sending Transfer Mode 0x%04X\r\n"),wRegTxnMode));
        WriteWord(SDHC_TRANSFERMODE, wRegTxnMode);
    }
    else {
        // Command-only
        if (pRequest->CommandCode == SD_CMD_STOP_TRANSMISSION) {
            wRegCommand |= CMD_TYPE_ABORT;
        }
        else if (TransferIsSDIOAbort(pRequest)) {
            // Use R5b For CMD52, Function 0, I/O Abort
            DEBUGMSG(SDHC_SEND_ZONE, (TEXT("Sending Abort command \r\n")));
            wRegCommand |= CMD_TYPE_ABORT | CMD_RESPONSE_R1B_R5B;
        }
    }

    DEBUGMSG(SDHC_SEND_ZONE,(TEXT("Sending command register 0x%04X\r\n"),wRegCommand));
    DEBUGMSG(SDHC_SEND_ZONE,(TEXT("Sending command Argument 0x%08X\r\n"),pRequest->CommandArgument));

    WriteDword(SDHC_ARGUMENT_0, pRequest->CommandArgument);

    // Enable transfer interrupt sources.
    WriteWord(SDHC_NORMAL_INT_STATUS_ENABLE, wIntStatusEn);

    // Status Busy bit checking for clearing the interrupt status register before "CMD ISSUE".
    fSuccess = WaitForReg<DWORD>(&CSDHCSlotBase::ReadDword, SDHC_CONTROL4, SDHC_CONTROL4_STABUSY, 0); 
    if (!fSuccess) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Timeout waiting for CMD operation finish\r\n"),
                    pszFname));
        status = SD_API_STATUS_DEVICE_NOT_RESPONDING;
        goto EXIT;
    }

    // Turn the clock on. It is turned off in IndicateBusRequestComplete().
    SDClockOn();

    // Turn the LED on.
    EnableLED(TRUE);

    // Writing the upper byte of the command register starts the command.
    // All register initialization must already be complete by this point.
    WriteWord(SDHC_COMMAND, wRegCommand);
    if (m_fCommandPolling  ) {
        PollingForCommandComplete();
    }
    status = SD_API_STATUS_PENDING;

EXIT:
    return status;
}

BOOL CSDHCSlotBase::PollingForCommandComplete()
{
    BOOL            fContinue = TRUE;
    if (m_fFakeCardRemoval && m_fCardPresent) {
        m_fFakeCardRemoval = FALSE;
        HandleRemoval(TRUE);
    }
    else {
        // Assume we reading PCI register at 66 Mhz. for times of 100 us. it should be 10*1000 time
        for (DWORD dwIndex=0; fContinue  && dwIndex<10*1000; dwIndex ++ ) {
            WORD wIntStatus = ReadWord(SDHC_NORMAL_INT_STATUS);
            if (wIntStatus != 0) {
                DEBUGMSG(SDHC_INTERRUPT_ZONE,
                        (TEXT("PollingForCommandComplete (%u) - Normal Interrupt_Status=0x%02x\n"),
                         m_dwSlot, wIntStatus));

                // Error handling. Make sure to handle errors first. 
                if ( wIntStatus & NORMAL_INT_STATUS_ERROR_INT ) {
                    HandleErrors();
                    fContinue = FALSE;
                }

                // Command Complete handling.
                if ( wIntStatus & NORMAL_INT_STATUS_CMD_COMPLETE ) {
                    // Clear status
                    m_fCommandCompleteOccurred = TRUE;
                    fContinue = FALSE;
                    WriteWord(SDHC_NORMAL_INT_STATUS, NORMAL_INT_STATUS_CMD_COMPLETE);
                    if (HandleCommandComplete()) { // If completed. 
                        WriteWord(SDHC_NORMAL_INT_STATUS, (wIntStatus & NORMAL_INT_STATUS_TRX_COMPLETE));
                    }
                }
            }
        }
    }
    ASSERT(!fContinue);
    return (!fContinue);
}


VOID 
CSDHCSlotBase::EnableSDIOInterrupts(
        BOOL fEnable
        )
{
    Validate();

    if (fEnable) {
        m_fSDIOInterruptsEnabled = TRUE;
        DoEnableSDIOInterrupts(fEnable);
    }
    else {
        DoEnableSDIOInterrupts(fEnable);
        m_fSDIOInterruptsEnabled = FALSE;
    }
}

#ifndef _SMDK6410_CH1_EXTCD_

    VOID CSDHCSlotBase::HandleInterrupt()
#else
        // New Interrupt handler function can process factors on new card detect interrupt of HSMMC ch1 on SMDK6410.
        VOID CSDHCSlotBase::HandleInterrupt(SDSLOT_INT_TYPE intType)
#endif
{
    Validate();
#ifdef _SMDK6410_CH1_EXTCD_
            // in case of it is occurred a card detect interrupt of HSMMC ch1 on SMDK6410
            if (intType == SDSLOT_INT_CARD_DETECTED) {
                RETAILMSG(1,(TEXT("[HSMMC1] Card Insertion or Removal Detect!!\n")));    
                m_fCheckSlot = TRUE;
            } else {  
#endif
    WORD wIntStatus = 0;
    wIntStatus = ReadWord(SDHC_NORMAL_INT_STATUS);

    if (m_fFakeCardRemoval ) {
        m_fFakeCardRemoval = FALSE;
        if (m_fCardPresent)
            HandleRemoval(TRUE);
        m_fCheckSlot = TRUE;
    }
    else if (wIntStatus != 0) {
        DEBUGMSG(SDHC_INTERRUPT_ZONE, 
                (TEXT("HandleInterrupt (%u) - Normal Interrupt_Status=0x%02x\n"),
                 m_dwSlot, wIntStatus));

        // Error handling. Make sure to handle errors first. 
        if ( wIntStatus & NORMAL_INT_STATUS_ERROR_INT ) {
            HandleErrors();
        }

        // Command Complete handling.
        if ( wIntStatus & NORMAL_INT_STATUS_CMD_COMPLETE ) {
            // Clear status
            m_fCommandCompleteOccurred = TRUE;
            WriteWord(SDHC_NORMAL_INT_STATUS, NORMAL_INT_STATUS_CMD_COMPLETE);
            if ( HandleCommandComplete() ) {
                wIntStatus &= ~NORMAL_INT_STATUS_TRX_COMPLETE; // this is command-only request. 
            }
        }

        // Sometimes at the lowest clock rate, the Read/WriteBufferReady
        // interrupt actually occurs before the CommandComplete interrupt.
        // This confuses our debug validation code and could potentially
        // cause problems. This is why we will verify that the CommandComplete
        // occurred before processing any data transfer interrupts.
        if (m_fCommandCompleteOccurred) {
            if (wIntStatus & NORMAL_INT_STATUS_DMA) {
                WriteWord(SDHC_NORMAL_INT_STATUS, NORMAL_INT_STATUS_DMA);
                // get the current request  
                PSD_BUS_REQUEST pRequest = GetAndLockCurrentRequest();
                if (m_SlotDma && pRequest) 
                    m_SlotDma->DMANotifyEvent(*pRequest, DMA_COMPLETE);
                else {
                    ASSERT(FALSE);
                }
                // do not break here. Continue to check TransferComplete. 
            }

            // Buffer Read Ready handling
            if (wIntStatus & NORMAL_INT_STATUS_BUF_READ_RDY ) {
                // Clear status
                WriteWord(SDHC_NORMAL_INT_STATUS, NORMAL_INT_STATUS_BUF_READ_RDY);
                HandleReadReady();
                // do not break here. Continue to check TransferComplete. 
            }

            // Buffer Write Ready handling
            if (wIntStatus & NORMAL_INT_STATUS_BUF_WRITE_RDY ) {
                // Clear status
                WriteWord(SDHC_NORMAL_INT_STATUS, NORMAL_INT_STATUS_BUF_WRITE_RDY);
                HandleWriteReady();
                // do not break here. Continue to check TransferComplete. 
            }
        }
        else {
            // We received data transfer interrupt before command 
            // complete interrupt. Wait for the command complete before
            // processing the data interrupt.
        }

        // Transfer Complete handling
        if ( wIntStatus & NORMAL_INT_STATUS_TRX_COMPLETE ) {
            // Clear status
            WriteWord(SDHC_NORMAL_INT_STATUS,
                    NORMAL_INT_STATUS_TRX_COMPLETE | NORMAL_INT_STATUS_DMA);
            HandleTransferDone();
        }

        // SDIO Interrupt Handling
        if ( wIntStatus & NORMAL_INT_STATUS_CARD_INT ) {
            if (m_fSDIOInterruptsEnabled)
            {
                DEBUGMSG(SDHC_INTERRUPT_ZONE, (_T("SDHCControllerIst: Card interrupt (SDIO)!\n")));

                EnableSDIOInterrupts(FALSE);
                IndicateSlotStateChange(DeviceInterrupting);
            }
        }

        // Card Detect Interrupt Handling
        if (wIntStatus & (NORMAL_INT_STATUS_CARD_INSERTION | NORMAL_INT_STATUS_CARD_REMOVAL)) {
            WriteWord(SDHC_NORMAL_INT_STATUS,
                    NORMAL_INT_STATUS_CARD_INSERTION | NORMAL_INT_STATUS_CARD_REMOVAL);
            m_fCheckSlot = TRUE;
        }
    }
#ifdef _SMDK6410_CH1_EXTCD_
            } // The end of "if(intType == SDSLOT_INT_CARD_DETECTED)"
#endif
    if (m_fCheckSlot) {
        m_fCheckSlot = FALSE;
#ifdef _SMDK6410_CH1_EXTCD_
        // At this time, we have to validate the card present status.
        if((IsCardPresent() == TRUE) && (m_fCardPresent != TRUE))
#else    
        // check card inserted or removed
        DWORD dwPresentState = ReadDword(SDHC_PRESENT_STATE);
        if (dwPresentState & STATE_CARD_INSERTED) {
#endif
        {
            DEBUGMSG(SDHC_INTERRUPT_ZONE, (TEXT("SDHCControllerIst - Card is Inserted! \n")));
            RETAILMSG(TRUE, (TEXT("[HSMMC1] SDHCControllerIst - Card is Inserted! \n")));
            m_fFakeCardRemoval = FALSE;
            if (m_fCardPresent == FALSE ) {
                Start();
                HandleInsertion();
            }
        }
#ifdef _SMDK6410_CH1_EXTCD_
        else if((IsCardPresent() == FALSE))
#else    
        else
#endif
        {
            DEBUGMSG(SDHC_INTERRUPT_ZONE, (TEXT("SDHCControllerIst - Card is Removed! \n")));
            RETAILMSG(TRUE, (TEXT("[HSMMC1] SDHCControllerIst - Card is Removed! \n")));
            m_fFakeCardRemoval = FALSE;
            if (m_fCardPresent) {
                HandleRemoval(TRUE);
            }
        }
    }
}
#ifdef _SMDK6410_CH1_EXTCD_
    // New function can detect whether card is presented of HSMMC ch1 on SMDK6410.
    BOOL CSDHCSlotBase::IsCardPresent()
    {
        BOOL fRetVal;
        volatile S3C6410_GPIO_REG *pIOPreg = NULL;

        PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
        
        pIOPreg = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
        if (pIOPreg == NULL) {
            RETAILMSG (1,(TEXT("GPIO registers not mapped\r\n")));
            return FALSE;
        }

		#if 0
        if ( (pIOPreg->GPNDAT & (0x1<<13)) == 0 ) {
		#else
		if ( (pIOPreg->GPNDAT & (0x1<<10)) == 0 ) {
		#endif
            fRetVal = TRUE;
        }    else {
            fRetVal = FALSE;
        }

        RETAILMSG(1,(TEXT("IsCardPresent() : fRetVal = %d  GPNDAT = 0x%x\r\n"), fRetVal, pIOPreg->GPNDAT));

        MmUnmapIoSpace((PVOID)pIOPreg, sizeof(S3C6410_GPIO_REG));

        return fRetVal;
    }
#endif
VOID 
CSDHCSlotBase::HandleRemoval(
        BOOL fCancelRequest
        )
{    
    m_fCardPresent = FALSE;
    m_fIsPowerManaged = FALSE;
    m_fSleepsWithPower = FALSE;
    m_fPowerUpDisabledInts = FALSE;
    m_f4BitMode = FALSE;
    m_cpsCurrent = D0;

    // Wake on SDIO interrupt must be set by the client
    m_bWakeupControl &= ~WAKEUP_INTERRUPT;

    // To control the Data CRC error
    WORD wErrIntSignalEn = ReadWord(SDHC_ERROR_INT_SIGNAL_ENABLE);
    WORD wErrIntStatusEn = ReadWord(SDHC_ERROR_INT_STATUS_ENABLE);
    WORD wErrIntStatus = ReadWord(SDHC_ERROR_INT_STATUS);
    WriteWord(SDHC_ERROR_INT_SIGNAL_ENABLE, (wErrIntSignalEn & ~(0x20))); //Command and Data CRC error disable
    WriteWord(SDHC_ERROR_INT_STATUS_ENABLE, (wErrIntStatusEn & ~(0x20))); //Command and Data CRC error disable

    if (m_fSDIOInterruptsEnabled) {
        EnableSDIOInterrupts(FALSE);
    }

    IndicateSlotStateChange(DeviceEjected);

    // turn off clock and remove power from the slot
    SDClockOff();
    WriteByte(SDHC_POWER_CONTROL, 0);

    if (fCancelRequest) {
        // get the current request  
        PSD_BUS_REQUEST pRequest = GetAndLockCurrentRequest();

        if (pRequest != NULL) {
            DEBUGMSG(SDCARD_ZONE_WARN, 
                    (TEXT("Card Removal Detected - Canceling current request: 0x%08X, command: %d\n"), 
                     pRequest, pRequest->CommandCode));
            DumpRequest(pRequest, SDHC_SEND_ZONE || SDHC_RECEIVE_ZONE);
            DumpRequest(pRequest, 0);
            IndicateBusRequestComplete(pRequest, SD_API_STATUS_DEVICE_REMOVED);
        }
    }

    if (m_SlotDma) {
        delete m_SlotDma;
        m_SlotDma = NULL;

        // The Pegasus requires the following so that the next
        // insertion will work correctly.
        SoftwareReset(SOFT_RESET_CMD | SOFT_RESET_DAT);
        WriteDword(SDHC_SYSTEMADDRESS_LO, 0);
        WriteWord(SDHC_BLOCKSIZE, 0);
        WriteWord(SDHC_BLOCKCOUNT, 0);
        WriteWord(SDHC_NORMAL_INT_STATUS, NORMAL_INT_STATUS_DMA);

        // To diable the Data CRC error interrupt
        WriteWord(SDHC_ERROR_INT_SIGNAL_ENABLE,0);
        WriteWord(SDHC_ERROR_INT_STATUS,wErrIntStatus);
        WriteWord(SDHC_ERROR_INT_SIGNAL_ENABLE,wErrIntSignalEn);    
    }

    // If needed, we can set an event here to notify the USBFn driver that SD card is removed
    // so that it can detach itself from the bus
    v_gBspArgs->g_SDCardState = CARD_REMOVED;
    SetEvent( v_gBspArgs->g_SDCardDetectEvent );

}


VOID 
CSDHCSlotBase::HandleInsertion(
        )
{
    DWORD dwClockRate = SD_DEFAULT_CARD_ID_CLOCK_RATE;

    // To control the Data CRC error
    WORD wErrIntSignalEn = ReadWord(SDHC_ERROR_INT_SIGNAL_ENABLE);
    WORD wErrIntStatusEn = ReadWord(SDHC_ERROR_INT_STATUS_ENABLE);

    m_fCardPresent = TRUE;

    // Apply the initial voltage to the card.
    SetVoltage(GetMaxVddWindow());

    // Send at least 74 clocks to the card over the course of at least 1 ms
    // with allowance for power supply ramp-up time. (SD Phys Layer 6.4)
    // Note that power supply ramp-up time occurs in SetVoltage().
    SetClockRate(&dwClockRate);
    SDClockOn();
    DWORD dwSleepMs = (74 / (dwClockRate / 1000)) + 1;
    Sleep(dwSleepMs);
    SDClockOff();
    if (m_SlotDma) {
        ASSERT(FALSE);
        delete m_SlotDma;
        m_SlotDma = NULL;
    }

    if (!m_fDisableDMA) {
        SSDHC_CAPABILITIES caps = GetCapabilities();
        if (FALSE) { // Disable ADM2 support for now    // caps.bits.ADMA2
            m_SlotDma = new CSDHCSlotBase32BitADMA2(*this);
        }
        else if (caps.bits.SDMA) {
            m_SlotDma = new CSDHCSlotBaseSDMA(*this);
        }

        if (m_SlotDma && !m_SlotDma->Init()) { // failed.
            delete m_SlotDma;
            m_SlotDma = NULL;
        }
        ASSERT(!(caps.bits.ADMA2 || caps.bits.SDMA) || m_SlotDma!=NULL);
    }

    // Interrupts are not enabled on a newly inserted card.
    EnableSDIOInterrupts(FALSE);

    // Indicate device arrival
    IndicateSlotStateChange(DeviceInserted);

    // Disable the command and data CRC error
    WriteWord(SDHC_ERROR_INT_SIGNAL_ENABLE, (wErrIntSignalEn | (0x20)));
    WriteWord(SDHC_ERROR_INT_STATUS_ENABLE, (wErrIntStatusEn | (0x20)));

    // If needed we can set an event here to notify the USBFn driver that SD card is inserted
    // so that it can attach itself to the bus
    v_gBspArgs->g_SDCardState = CARD_INSERTED;
    SetEvent( v_gBspArgs->g_SDCardDetectEvent );

}

BOOL 
CSDHCSlotBase::HandleCommandComplete(
        )
{
    SETFNAME();

    PSD_BUS_REQUEST pRequest;
    BOOL fRet = FALSE;

    // get the current request
    pRequest = GetAndLockCurrentRequest();
    DEBUGCHK(m_dwReadyInts == 0);

    if (pRequest) {
        DEBUGCHK(pRequest->HCParam == 0);
        // INT_TRX_COMPLETE is not suitable for s3c6410, so I comment out below code.
        // therefore, in order to keep fastpath status, those code like just below is needed.
        SD_API_STATUS transferStatus = (m_FastPathStatus >= SD_API_STATUS_SUCCESS) ? SD_API_STATUS_SUCCESS : m_FastPathStatus;

        if (NoResponse != pRequest->CommandResponse.ResponseType) {
            // Copy response over to request structure. Note that this is a 
            // bus driver buffer, so we do not need to SetProcPermissions 
            // or __try/__except.
            UNALIGNED DWORD *pdwResponseBuffer = 
                (PDWORD) (pRequest->CommandResponse.ResponseBuffer + 1); // Skip CRC
            WORD wCommand = ReadWord(SDHC_COMMAND);

            if ((wCommand & CMD_RESPONSE_R1B_R5B) == CMD_RESPONSE_R1B_R5B) {
                // Reset cmd and dat circuits
                SoftwareReset(SOFT_RESET_CMD | SOFT_RESET_DAT);
            }

            pdwResponseBuffer[0] = ReadDword(SDHC_R0);
            if (pRequest->CommandResponse.ResponseType == ResponseR2) {
                pdwResponseBuffer[1] = ReadDword(SDHC_R2);
                pdwResponseBuffer[2] = ReadDword(SDHC_R4);
                pdwResponseBuffer[3] = ReadDword(SDHC_R6);
            }
        }

        // check for command/response only
        if (TRANSFER_IS_COMMAND_ONLY(pRequest)) {
            IndicateBusRequestComplete(pRequest, transferStatus);
            fRet = TRUE;
        } else {
            // handle data phase transfer
            pRequest->HCParam = 0;
            fRet = FALSE;
        }
    }
    // else request must have been canceled due to an error

    return fRet;
}

VOID 
CSDHCSlotBase::HandleErrors(
        )
{
    SD_API_STATUS status = SD_API_STATUS_SUCCESS;

    // get the current request
    PSD_BUS_REQUEST pRequest = NULL;
    pRequest = GetAndLockCurrentRequest();

    WORD wErrorStatus = 0;
    WORD RetryCount = 100;

    do {
        wErrorStatus = ReadWord(SDHC_ERROR_INT_STATUS);
        RetryCount--;
    } while ((wErrorStatus == 0) || (RetryCount != 0));

    if (wErrorStatus == 0)
    {
        DEBUGMSG(SDCARD_ZONE_ERROR, 
                (TEXT("HandleErrors - ERROR INT STATUS=0x%02X\n"), wErrorStatus));
        return;
    }

    DEBUGMSG(SDCARD_ZONE_ERROR, 
            (TEXT("HandleErrors - ERROR INT STATUS=0x%02X\n"), wErrorStatus));
    if (pRequest) {
        DumpRequest(pRequest, SDCARD_ZONE_ERROR);
          // For simple debugging, we have to confirm the command number!
          RETAILMSG(TRUE, (TEXT("[HSMMC1] HandleErrors - ERR CMD:%d : "), pRequest->CommandCode));
    }

    DEBUGCHK( (wErrorStatus & ERR_INT_STATUS_VENDOR) == 0 );

    if (wErrorStatus) {
        if ( wErrorStatus & ERR_INT_STATUS_CMD_TIMEOUT ) {
            status = SD_API_STATUS_RESPONSE_TIMEOUT;
            switch(pRequest->CommandCode) {
                case 1 : RETAILMSG(TRUE, (TEXT("If the card is not a MMC, CMD 1 does not work in reason.\n")));
                    break;
                case 5 : RETAILMSG(TRUE, (TEXT("If the card is not a SDIO, CMD 5 does not work in reason.\n")));
                    break;
                case 8 : RETAILMSG(TRUE, (TEXT("If the card is not SD SPEC 2.0, CMD 8 does not work in reason.\n")));
                    break;
                default :
                    RETAILMSG(TRUE, (TEXT("[HSMMC1] HandleErrors - CMD Timeout Error...\n")));
                    break;
            }
        }

        if ( wErrorStatus & ERR_INT_STATUS_CMD_CRC ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - CMD CRC Error...\r\n")));            
            status = SD_API_STATUS_CRC_ERROR;
            if ( wErrorStatus & ERR_INT_STATUS_CMD_TIMEOUT )
                status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
        }

        if ( wErrorStatus & ERR_INT_STATUS_CMD_ENDBIT ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - CMD ENDBIT Error...\r\n")));                        
            status = SD_API_STATUS_RESPONSE_TIMEOUT;
        }

        if ( wErrorStatus & ERR_INT_STATUS_CMD_INDEX ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - CMD INDEX Error...\r\n")));            
            status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
        }

        if ( wErrorStatus & ERR_INT_STATUS_DAT_TIMEOUT ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - DAT Timeout Error...\r\n")));            
            status = SD_API_STATUS_DATA_TIMEOUT;
        }

        if ( wErrorStatus & ERR_INT_STATUS_DAT_CRC ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - Data CRC Error...\r\n")));              
            status = SD_API_STATUS_CRC_ERROR;
        }

        if ( wErrorStatus & ERR_INT_STATUS_DAT_ENDBIT ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - DAT END BIT Error...\r\n")));            
            status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
        }

        if ( wErrorStatus & ERR_INT_STATUS_BUS_POWER ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - Bus Power Error...\r\n")));
            status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
        }

        if ( wErrorStatus & ERR_INT_STATUS_AUTOCMD12 ) {
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - AUTOCMD12 Error...\r\n")));            
            status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
        }

        if (wErrorStatus & ERR_INT_STATUS_ADMA) { // ADMA Error
            RETAILMSG(TRUE,(TEXT("[HSMMC1] HandleErrors - ADMA Error...\r\n")));
            if (m_SlotDma && pRequest ) {
                m_SlotDma->DMANotifyEvent(*pRequest, DMA_ERROR_OCCOR );
            }
            else {
                DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("[HSMMC1] HandleErrors - ADMA Error without DMA Enabled (0x%x). Resetting CMD line.\r\n"),
                            wErrorStatus));
            }
        }
        // Perform basic error recovery
        WORD wErrIntSignal = ReadWord(SDHC_ERROR_INT_SIGNAL_ENABLE);
        WriteWord(SDHC_ERROR_INT_SIGNAL_ENABLE, 0);
        // Make sure that the value of "Error Interrupt Status Enable" is zero
        WORD wErrIntStatusEn = ReadWord(SDHC_ERROR_INT_STATUS_ENABLE);
        WriteWord(SDHC_ERROR_INT_STATUS_ENABLE,0);

        if (IS_CMD_LINE_ERROR(wErrorStatus)) {
            // Reset CMD line
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("[HSMMC1] HandleErrors - Command line error (0x%x). Resetting CMD line.\r\n"),
                        wErrorStatus));
            SoftwareReset(SOFT_RESET_CMD);
        }

        if (IS_DAT_LINE_ERROR(wErrorStatus)) {
            // Reset DAT line
            DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("[HSMMC1] HandleErrors - Data line error (0x%x). Resetting DAT line.\r\n"),
                        wErrorStatus));

            DWORD RetryResetCount = 5000;

            SoftwareReset(SOFT_RESET_DAT);
            do {
                WriteWord(SDHC_ERROR_INT_STATUS,wErrorStatus);

                if ((ReadWord(SDHC_NORMAL_INT_STATUS) & NORMAL_INT_STATUS_ERROR_INT)) {
                    RetryResetCount--;
                } else {
                    break;
                }

                if(RetryResetCount == 0) {
                    DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("[HSMMC1] HandleErrors - DAT line error Recovery Timeout...\r\n")));
                }
            } while(RetryResetCount != 0);
        }

        // clear all error status
        WriteWord(SDHC_ERROR_INT_STATUS, wErrorStatus);

        // re-enable error interrupt signals
        WriteWord(SDHC_ERROR_INT_STATUS_ENABLE, wErrIntStatusEn);
        WriteWord(SDHC_ERROR_INT_SIGNAL_ENABLE, wErrIntSignal);

        // complete the request
        if (pRequest) {
            IndicateBusRequestComplete(pRequest, status);
        } else {
            // If there is not a current request, the initialize of normal interrupt status enable is needed.
            WriteWord(SDHC_NORMAL_INT_STATUS_ENABLE,
                    (ReadWord(SDHC_NORMAL_INT_STATUS_ENABLE)) | NORMAL_INT_ENABLE_CMD_COMPLETE | NORMAL_INT_ENABLE_TRX_COMPLETE);
        }
    }
}


VOID 
CSDHCSlotBase::HandleTransferDone(
        )
{
    PSD_BUS_REQUEST pRequest;
    SD_API_STATUS   status = SD_API_STATUS_SUCCESS;

    // get the current request  
    pRequest = GetAndLockCurrentRequest();

    if (pRequest) {
        if (!TRANSFER_IS_COMMAND_ONLY(pRequest)) {
            if (m_SlotDma && !m_SlotDma->IsDMACompleted()) {
                m_SlotDma->DMANotifyEvent(*pRequest, TRANSFER_COMPLETED);
            }
        }
        if (pRequest->HCParam != TRANSFER_SIZE(pRequest)) {
            // This means that a Command Complete interrupt occurred before
            // a Buffer Ready interrupt. Hardware should not allow this. 
            DEBUGCHK(FALSE);
            status = SD_API_STATUS_DEVICE_RESPONSE_ERROR;
        }

        // complete the request
        if (pRequest->Flags & SD_AUTO_ISSUE_CMD12) {
            m_fAutoCMD12Success = TRUE;
        }
        IndicateBusRequestComplete(pRequest, status);
    }
    // else request must have been canceled due to an error
}


VOID 
CSDHCSlotBase::HandleReadReady(
        )
{
    DEBUGMSG(SDHC_RECEIVE_ZONE, (TEXT("HandleReadReady - HandleReadReady!\n"))); 

    // get the current request  
    PSD_BUS_REQUEST pRequest = GetAndLockCurrentRequest();

    if (pRequest) {
        DEBUGCHK(pRequest->NumBlocks > 0);
        DEBUGCHK(pRequest->HCParam < TRANSFER_SIZE(pRequest));
        DEBUGCHK(TRANSFER_IS_READ(pRequest));

#ifdef DEBUG
        ++m_dwReadyInts;
#endif

        __try {
            PDWORD pdwUserBuffer = (PDWORD) &pRequest->pBlockBuffer[pRequest->HCParam];
            PDWORD pdwBuffer = pdwUserBuffer;
            DWORD  rgdwIntermediateBuffer[SDHC_MAX_BLOCK_LENGTH / sizeof(DWORD)];
            BOOL   fUsingIntermediateBuffer = FALSE;
            DWORD  cDwords = pRequest->BlockSize / 4;
            DWORD  dwRemainder = pRequest->BlockSize % 4;

            PREFAST_DEBUGCHK(sizeof(rgdwIntermediateBuffer) >= pRequest->BlockSize);

            if (((DWORD) pdwUserBuffer) % 4 != 0) {
                // Buffer is not DWORD aligned so we must use an
                // intermediate buffer.
                pdwBuffer = rgdwIntermediateBuffer;
                fUsingIntermediateBuffer = TRUE;
            }

            DWORD dwDwordsRemaining = cDwords;
            pRequest->HCParam += dwDwordsRemaining * 4;

            // Read the data from the device
            while ( dwDwordsRemaining-- ) {
                *(pdwBuffer++) = ReadDword(SDHC_BUFFER_DATA_PORT_0);
            }

            if ( dwRemainder != 0 ) {
                DWORD dwLastWord = ReadDword(SDHC_BUFFER_DATA_PORT_0);
                memcpy(pdwBuffer, &dwLastWord, dwRemainder);
                pRequest->HCParam += dwRemainder;
            }

            if (fUsingIntermediateBuffer) {
                memcpy(pdwUserBuffer, rgdwIntermediateBuffer, pRequest->BlockSize);
            }
        }
        __except(SDProcessException(GetExceptionInformation())) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (_T("Exception reading from client buffer!\r\n")));
            IndicateBusRequestComplete(pRequest, SD_API_STATUS_ACCESS_VIOLATION);
        }

        DEBUGCHK(pRequest->HCParam == (m_dwReadyInts * pRequest->BlockSize));
    }
    // else request must have been canceled due to an error
}


VOID 
CSDHCSlotBase::HandleWriteReady(
        )
{    
    DEBUGMSG(SDHC_TRANSMIT_ZONE, (TEXT("HandleWriteReady - HandleWriteReady! \n"))); 

    // get the current request  
    PSD_BUS_REQUEST pRequest = GetAndLockCurrentRequest();

    if (pRequest) {
        DEBUGCHK(TRANSFER_IS_WRITE(pRequest));
        DEBUGCHK(pRequest->NumBlocks > 0);
        DEBUGCHK(pRequest->HCParam < TRANSFER_SIZE(pRequest));

#ifdef DEBUG
        ++m_dwReadyInts;
#endif

        __try {
            PDWORD pdwUserBuffer = (PDWORD) &pRequest->pBlockBuffer[pRequest->HCParam];
            PDWORD pdwBuffer = pdwUserBuffer;
            DWORD  rgdwIntermediateBuffer[SDHC_MAX_BLOCK_LENGTH / sizeof(DWORD)];
            BOOL   fUsingIntermediateBuffer = FALSE;
            DWORD  cDwords = pRequest->BlockSize / 4;
            DWORD  dwRemainder = pRequest->BlockSize % 4;

            PREFAST_DEBUGCHK(sizeof(rgdwIntermediateBuffer) >= pRequest->BlockSize);

            if (((DWORD) pdwUserBuffer) % 4 != 0) {
                // Buffer is not DWORD aligned so we must use an
                // intermediate buffer.
                pdwBuffer = rgdwIntermediateBuffer;
                memcpy(rgdwIntermediateBuffer, pdwUserBuffer, pRequest->BlockSize);
            }

            DWORD dwDwordsRemaining = cDwords;
            pRequest->HCParam += dwDwordsRemaining * 4;

            // Write data to buffer data port
            while ( dwDwordsRemaining-- ) {
                WriteDword(SDHC_BUFFER_DATA_PORT_0, *(pdwBuffer++));
            }

            if ( dwRemainder != 0 ) {
                DWORD dwLastWord = 0;
                memcpy(&dwLastWord, pdwBuffer, dwRemainder);
                WriteDword(SDHC_BUFFER_DATA_PORT_0, dwLastWord);
                pRequest->HCParam += dwRemainder;
            }

        }
        __except(SDProcessException(GetExceptionInformation())) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (_T("Exception reading from client buffer!\r\n")));
            IndicateBusRequestComplete(pRequest, SD_API_STATUS_ACCESS_VIOLATION);
        }

        DEBUGCHK(pRequest->HCParam == (m_dwReadyInts * pRequest->BlockSize));
    }
    // else request must have been canceled due to an error
}

PVOID
CSDHCSlotBase::AllocPhysBuffer(
        size_t cb,
        PDWORD pdwPhysAddr
        )
{
    PVOID pvUncached;
    PVOID pvRet = NULL;
    DWORD dwPhysAddr;

    pvUncached = AllocPhysMem(cb, PAGE_READWRITE, 0, 0, &dwPhysAddr);

    if (pvUncached) {
        *pdwPhysAddr = dwPhysAddr;
        pvRet = pvUncached;
    }

    return pvRet;
}


VOID
CSDHCSlotBase::FreePhysBuffer(
        PVOID pv
        )
{
    BOOL fSuccess;

    DEBUGCHK(pv);

    fSuccess = FreePhysMem(pv);
    DEBUGCHK(fSuccess);
}


VOID
CSDHCSlotBase::SetHardwarePowerState(
        CEDEVICE_POWER_STATE cpsNew
        )
{
    DEBUGCHK(VALID_DX(cpsNew));
    DEBUGCHK(!DX_D1_OR_D2(cpsNew));

    DEBUGCHK(m_cpsCurrent != cpsNew);
    CEDEVICE_POWER_STATE cpsCurrent = m_cpsCurrent;
    m_cpsCurrent = cpsNew;
    BYTE bWakeupControl = m_bWakeupControl;

    if (cpsCurrent == D0) {
        SDClockOff();

        if (cpsNew == D3) {
            if ( m_fSDIOInterruptsEnabled &&  
                    (bWakeupControl & WAKEUP_INTERRUPT) ) {
                DEBUGCHK(m_fCardPresent);
                m_fSleepsWithPower = TRUE;
                m_fPowerUpDisabledInts = FALSE;
            }
            else {
                // Wake on status changes only
                WriteByte(SDHC_POWER_CONTROL, 0);
                bWakeupControl &= ~WAKEUP_INTERRUPT;
            }

            // enable wakeup sources
            m_wIntSignals = ReadWord(SDHC_NORMAL_INT_SIGNAL_ENABLE);
            WriteWord(SDHC_NORMAL_INT_SIGNAL_ENABLE, 0);
            WriteWord(SDHC_NORMAL_INT_STATUS, ReadWord(SDHC_NORMAL_INT_STATUS));
            WriteByte(SDHC_WAKEUP_CONTROL, bWakeupControl);
        }
        else {
            DEBUGCHK(cpsNew == D4);
            WriteByte(SDHC_CLOCK_CONTROL, 0);
            WriteByte(SDHC_POWER_CONTROL, 0);
        }
    }
    else if (cpsCurrent == D3) {
        // Coming out of wakeup state
        if (cpsNew == D0) {            
            WriteByte(SDHC_WAKEUP_CONTROL, 0);

            if (!m_fSleepsWithPower) {
                // Power was turned off to the socket. Re-enumerate card.
                if (m_fCardPresent) {
                    HandleRemoval(TRUE);
                }

                m_fCheckSlot = TRUE;
                SetInterruptEvent();
            }
            else {
                if (m_fCardPresent) {
                    // Do not do this if the card was removed or 
                    // if power was not kept.
                    if (m_fPowerUpDisabledInts) {
                        EnableSDIOInterrupts(TRUE);
                    }
                }
            }

            WriteWord(SDHC_NORMAL_INT_SIGNAL_ENABLE, m_wIntSignals);
        }
        else {
            DEBUGCHK(cpsNew == D4);
            WriteByte(SDHC_CLOCK_CONTROL, 0);
            WriteByte(SDHC_WAKEUP_CONTROL, 0);
            WriteByte(SDHC_POWER_CONTROL, 0);
            WriteWord(SDHC_NORMAL_INT_SIGNAL_ENABLE, m_wIntSignals);
        }

        m_fSleepsWithPower = FALSE;
    }
    else {
        DEBUGCHK(cpsCurrent == D4);

        // Coming out of unpowered state - signal card removal
        // so any card present will be re-enumerated.
        //
        // We do the same thing when we go to D3 as D0 because
        // the slot has lost power so it could have been removed
        // or changed. In other words, D3 is a meaningless state
        // after D4.
        m_cpsCurrent = D0; // Force to D0

        // Do not call HandleRemoval here because it could cause
        // a context switch in a PowerUp callback.
        m_fFakeCardRemoval = TRUE;

        m_fCheckSlot = TRUE;
        SetInterruptEvent();
    }
}

VOID
CSDHCSlotBase::DoEnableSDIOInterrupts(
        BOOL fEnable
        )
{
    // To control the SDIO card interrupt on S3C6410, control the normal interrupt signal, too.
    WORD wIntSignalEn = ReadWord(SDHC_NORMAL_INT_SIGNAL_ENABLE);
    if (fEnable) {
        m_isSDIOInterrupt = FALSE;    // SDIO card interrupt Enable.
        wIntSignalEn |= NORMAL_INT_SIGNAL_CARD_INT;
    } else {
        m_isSDIOInterrupt = TRUE;   // SDIO card interrupt was occured.
        wIntSignalEn &= (~NORMAL_INT_SIGNAL_CARD_INT);      
    }
    WriteWord(SDHC_NORMAL_INT_SIGNAL_ENABLE, wIntSignalEn);

    WORD wIntStatusEn = ReadWord(SDHC_NORMAL_INT_STATUS_ENABLE);
    if (fEnable) {
        wIntStatusEn |= NORMAL_INT_ENABLE_CARD_INT;
    } else {
        wIntStatusEn &= (~NORMAL_INT_ENABLE_CARD_INT);      
    }
    WriteWord(SDHC_NORMAL_INT_STATUS_ENABLE, wIntStatusEn);
}


template<class T>
BOOL
CSDHCSlotBase::WaitForReg(
        T (CSDHCSlotBase::*pfnReadReg)(DWORD),
        DWORD dwRegOffset,
        T tMask,
        T tWaitForEqual,
        DWORD dwTimeout
        )
{
    SETFNAME();

    const DWORD dwStart = GetTickCount();

    T tValue;

    BOOL fRet = TRUE;
    DWORD dwIteration = 1;

    // Verify that reset has completed.
    do {
        tValue = (this->*pfnReadReg)(dwRegOffset);

        if ( (dwIteration % 16) == 0 ) {
            // Check time
            DWORD dwCurr = GetTickCount();

            // Unsigned arithmetic handles rollover.
            DWORD dwTotal = dwCurr - dwStart;

            if (dwTotal > dwTimeout) {
                // Timeout
                fRet = FALSE;
              DEBUGMSG(SDCARD_ZONE_WARN, (_T("%s Timeout (%u ms) waiting for (ReadReg<%u>(0x%02x) & 0x%08x) == 0x%08x\r\n"),
                            pszFname, dwTimeout, sizeof(T), dwRegOffset, tMask, tWaitForEqual));
                break;
            }
        }

        ++dwIteration;
    } while ((tValue & tMask) != tWaitForEqual);

    return fRet;
}


BOOL 
CSDHCSlotBase::SoftwareReset(
        BYTE bResetBits
        )
{
    SETFNAME();

    // Reset the controller
    WriteByte(SDHC_SOFT_RESET, bResetBits);
    BOOL fSuccess = WaitForReg<BYTE>(&CSDHCSlotBase::ReadByte, SDHC_SOFT_RESET, bResetBits, 0);
    if (!fSuccess) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Timeout waiting for controller reset - 0x%02x\r\n"),
                    pszFname, bResetBits));
    }

    // Command Conflict Mask Enable
    WriteDword(SDHC_CONTROL2, (ReadDword(SDHC_CONTROL2) | SDHC_CONTROL2_ENCMDCNFMSK));

    return fSuccess;
}


VOID
CSDHCSlotBase::EnableLED(
        BOOL fEnable
        )
{
    BYTE bHostControl = ReadByte(SDHC_HOST_CONTROL);

    if (fEnable) {
        bHostControl |= HOSTCTL_LED_CONTROL;
    }
    else {
        bHostControl &= ~HOSTCTL_LED_CONTROL;
    }

    WriteByte(SDHC_HOST_CONTROL, bHostControl);
}


VOID 
CSDHCSlotBase::IndicateSlotStateChange(SD_SLOT_EVENT sdEvent) {
    SDHCDIndicateSlotStateChange(m_pHCDContext,
            (UCHAR) m_dwSlot, sdEvent);
}


PSD_BUS_REQUEST
CSDHCSlotBase::GetAndLockCurrentRequest() {
    return SDHCDGetAndLockCurrentRequest(m_pHCDContext,
            (UCHAR) m_dwSlot);
}


VOID CSDHCSlotBase::PowerUpDown(BOOL fPowerUp, BOOL fKeepPower) {
    SDHCDPowerUpDown(m_pHCDContext, fPowerUp, fKeepPower,
            (UCHAR) m_dwSlot);
}

VOID 
CSDHCSlotBase::IndicateBusRequestComplete(
        PSD_BUS_REQUEST pRequest,
        SD_API_STATUS status
        )
{
    const WORD c_wTransferIntSources = 
        NORMAL_INT_STATUS_CMD_COMPLETE | 
        NORMAL_INT_STATUS_TRX_COMPLETE |
        NORMAL_INT_STATUS_DMA |
        NORMAL_INT_STATUS_BUF_WRITE_RDY | 
        NORMAL_INT_STATUS_BUF_READ_RDY;

    DEBUGCHK(pRequest);

    if ( (m_fSDIOInterruptsEnabled && m_f4BitMode) == FALSE ) {
        SDClockOff();
    }
    // else need to leave clock on in order to receive interrupts in 4 bit mode

    // Turn off LED.
    EnableLED(FALSE);

    // Turn off interrupt sources
    WORD wIntStatusEn = ReadWord(SDHC_NORMAL_INT_STATUS_ENABLE);
    wIntStatusEn &= ~c_wTransferIntSources;
    WriteWord(SDHC_NORMAL_INT_STATUS_ENABLE, wIntStatusEn);

    // Clear any remaining spurious interrupts.
    WriteWord(SDHC_NORMAL_INT_STATUS, c_wTransferIntSources);

#ifdef DEBUG
    m_dwReadyInts = 0;
#endif

    m_fCommandCompleteOccurred = FALSE;
    //ASSERT(m_pCurrentRequest!=NULL); // m_pCurrentRequest can be NULL in case of CMD errors
    m_pCurrentRequest = NULL;
    if (m_fCurrentRequestFastPath) {
        if (status == SD_API_STATUS_SUCCESS) {
            status = SD_API_STATUS_FAST_PATH_SUCCESS;
        }
        m_FastPathStatus = status;
    }
    else
        SDHCDIndicateBusRequestComplete(m_pHCDContext, pRequest, status);
}

BOOL CSDHCSlotBase::DetermineCommandPolling()
{
    DWORD   dwDivisor =  (m_wRegClockControl>>8);
    DWORD   dwMaxClockRate = (dwDivisor!=0? (m_dwMaxClockRate/2/dwDivisor): m_dwMaxClockRate);
    return (dwMaxClockRate >= (10^6)); // If bigger than 1 Mhz, we do the polling.
}

VOID 
CSDHCSlotBase::SetClockRate(
        PDWORD pdwRate
        )
{
    DEBUGCHK(m_dwMaxClockRate);

    const DWORD dwClockRate = *pdwRate;
    DWORD       dwMaxClockRate = m_dwMaxClockRate;
    int         i = 0; // 2^i is the divisor value
    DWORD dwControlValue = 0;

    // Command Conflict Mask Enable
    dwControlValue = SDHC_CONTROL2_ENCMDCNFMSK;

    // Write Status Clear Async Mode Enable
    // and, if this bit is enabled, it must be needed that Status Busy bit checking for clearing the interrupt status register before "CMD ISSUE"!!
    dwControlValue |= SDHC_CONTROL2_ENSTAASYNCCLR;

    if (dwClockRate < HSMMC_FULL_SPEED_RATE)
    {
        DEBUGMSG(SDHC_CLOCK_ZONE,(TEXT("[HSMMC1] Turn OFF the F/B delay control.\r\n")));
        dwControlValue &= ~(0x3<<14); // Turn off the feedback clock delay
    }
    else
    {
        DEBUGMSG(SDHC_CLOCK_ZONE,(TEXT("[HSMMC1] Turn ON the F/B delay control.\r\n")));
        dwControlValue = (dwControlValue & (~(0x3<<14))) | (0x1<<14); // Turn on the RX feedback clock delay
        WriteDword(SDHC_CONTROL3, (ReadDword(SDHC_CONTROL3) | (0x1<<15) | (0x1<<7)));
    }

#ifdef _SRCCLK_48MHZ_
    dwControlValue |= ((0x3<<9) |  // Debounce Filter Count 0x3=64 iSDCLK
            (0x1<<8) |  // SDCLK Hold Enable
            (0x3<<4));  // Base Clock Source = External Clock(USB_PHY)
#else
    dwControlValue |= ((0x3<<9) |
            (0x1<<8) |
            (0x2<<4));  // Base Clock Source = EPLL out
#endif    // !_SRCCLK_48MHZ_
    WriteDword(SDHC_CONTROL2,  dwControlValue);

    // shift MaxClockRate until we find the closest frequency <= target
    while ( (dwClockRate < dwMaxClockRate) && ( i < 8 ) ) {
        dwMaxClockRate = dwMaxClockRate >> 1;
        i++;
    }

    // set the actual clock rate 
    *pdwRate = dwMaxClockRate;
    m_wRegClockControl = CLOCK_INTERNAL_ENABLE;

    if (i != 0) {
        DWORD dwDivisor = 1 << (i - 1);
        m_wRegClockControl |= (dwDivisor << 8);
    }
    m_fCommandPolling = DetermineCommandPolling();

    DEBUGMSG(SDHC_CLOCK_ZONE,(TEXT("SDHCSetRate - Clock Control Reg = %X\n"), 
                m_wRegClockControl));
    DEBUGMSG(SDHC_CLOCK_ZONE,(TEXT("SDHCSetRate - Actual clock rate = %d\n"), *pdwRate));
}


VOID 
CSDHCSlotBase::SDClockOn(
        )
{
    SETFNAME();

    // Must call SetClockRate() first to init m_wRegClockControl
    // We separate clock divisor calculation with ClockOn, so we can call 
    // ClockOn without recalcuating the divisor.
    WriteWord(SDHC_CLOCK_CONTROL, m_wRegClockControl);

    // wait until clock stable
    BOOL fSuccess = WaitForReg<WORD>(&CSDHCSlotBase::ReadWord, SDHC_CLOCK_CONTROL, 
            CLOCK_STABLE, CLOCK_STABLE); 
    if (fSuccess) {
        // SD Clock Output PAD Drive Strength is needed as 9mA.
        WriteDword(SDHC_CONTROL4, ((ReadDword(SDHC_CONTROL4) | (0x3<<16))));
        // enable it
        WriteWord(SDHC_CLOCK_CONTROL, m_wRegClockControl | CLOCK_ENABLE);
    }
    else {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Timeout waiting for CLOCK_STABLE\r\n"), pszFname));
        RETAILMSG(TRUE, (_T("[HSMMC1] %s Timeout waiting for CLOCK_STABLE\r\n"), pszFname));
    }
}


VOID 
CSDHCSlotBase::SDClockOff(
        )
{
    WriteWord(SDHC_CLOCK_CONTROL, m_wRegClockControl);
}


DWORD
CSDHCSlotBase::DetermineVddWindows(
        )
{
    DWORD dwVddWindows = 0;
    // This is needed for supporting both SD Memory and MMC.
    dwVddWindows = SD_VDD_WINDOW_3_2_TO_3_3 | SD_VDD_WINDOW_3_3_TO_3_4;

    return dwVddWindows;
}


DWORD
CSDHCSlotBase::GetDesiredVddWindow(
        )
{
    DEBUGCHK(m_dwVddWindows);

    DWORD dwDesiredVddWindow = 0;

    // Return lowest supportable voltage.
    if (m_dwVddWindows & SD_VDD_WINDOW_1_7_TO_1_8) {
        dwDesiredVddWindow = SD_VDD_WINDOW_1_7_TO_1_8;
    }
    else if (m_dwVddWindows & SD_VDD_WINDOW_2_9_TO_3_0) {
        dwDesiredVddWindow = SD_VDD_WINDOW_2_9_TO_3_0;
    }
    else if (m_dwVddWindows & SD_VDD_WINDOW_3_2_TO_3_3) {
        dwDesiredVddWindow = SD_VDD_WINDOW_3_2_TO_3_3;
    }

    return dwDesiredVddWindow;
}


DWORD
CSDHCSlotBase::GetMaxVddWindow(
        )
{
    DEBUGCHK(m_dwVddWindows);

    DWORD dwMaxVddWindow = 0;

    if (m_dwVddWindows & SD_VDD_WINDOW_3_2_TO_3_3) {
        dwMaxVddWindow = SD_VDD_WINDOW_3_2_TO_3_3;
    }
    else if (m_dwVddWindows & SD_VDD_WINDOW_2_9_TO_3_0) {
        dwMaxVddWindow = SD_VDD_WINDOW_2_9_TO_3_0;
    }
    else if (m_dwVddWindows & SD_VDD_WINDOW_1_7_TO_1_8) {
        dwMaxVddWindow = SD_VDD_WINDOW_1_7_TO_1_8;
    }

    return dwMaxVddWindow;
}

BOOL
CSDHCSlotBase::IsWriteProtected() {
#ifdef _SMDK6410_CH1_WP_
    // On the CPU B'd Rev 0.0 of SMDK6410 , Write protect is detected with GPIO_F[13]
    BOOL fRet = FALSE;
    volatile S3C6410_GPIO_REG *pIOPreg = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;                
    pIOPreg = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (pIOPreg == NULL) {
        RETAILMSG (1,(TEXT("[HSMMC1] GPIO registers is *NOT* mapped.\n")));
        return FALSE;
    }
    //fRet = (pIOPreg->GPFDAT & (0x1<<13)) ? 0 : 1; // It is always "0" because WP pin is not connected for now and the status of line is pulled-up.
                fRet = (pIOPreg->GPLDAT & (0x1<<14)) ? 1 : 0; // It is always "0" because WP pin is not connected for now and the status of line is floating.
    MmUnmapIoSpace((PVOID)pIOPreg, sizeof(S3C6410_GPIO_REG));
    return fRet;
#else
    return ((ReadDword(SDHC_PRESENT_STATE) & STATE_WRITE_PROTECT) == 0);
#endif
}

PVOID CSDHCSlotBase::SlotAllocDMABuffer(ULONG Length,PPHYSICAL_ADDRESS  LogicalAddress,BOOLEAN CacheEnabled )
{
    CE_DMA_ADAPTER  dmaAdapter = {
        sizeof(CE_DMA_ADAPTER),
        m_interfaceType,
        m_dwBusNumber,
        0,0
    };
    dmaAdapter.BusMaster = TRUE;
    return OALDMAAllocBuffer(&dmaAdapter, Length, LogicalAddress,CacheEnabled);
}
BOOL CSDHCSlotBase::SlotFreeDMABuffer(ULONG Length,PHYSICAL_ADDRESS  LogicalAddress,PVOID VirtualAddress,BOOLEAN CacheEnabled )
{
    CE_DMA_ADAPTER  dmaAdapter = {
        sizeof(CE_DMA_ADAPTER),
        m_interfaceType,
        m_dwBusNumber,
        0,0
    };
    dmaAdapter.BusMaster = TRUE;
    OALDMAFreeBuffer(&dmaAdapter,Length, LogicalAddress, VirtualAddress, CacheEnabled);
    return TRUE;
}


#ifdef DEBUG

VOID 
CSDHCSlotBase::Validate(
        )
{
    DEBUGCHK(m_pregDevice && m_pregDevice->IsOK());
    DEBUGCHK(m_dwSlot < SDHC_MAX_SLOTS);
    DEBUGCHK(m_pbRegisters);
    DEBUGCHK(m_pHCDContext);
    DEBUGCHK(VALID_DX(m_cpsCurrent));
    DEBUGCHK(VALID_DX(m_cpsAtPowerDown));    
}


VOID 
CSDHCSlotBase::DumpRegisters(
        )
{
    BYTE                rgbSdRegs[sizeof(SSDHC_REGISTERS)];
    SSDHC_REGISTERS    *pStdHCRegs = (SSDHC_REGISTERS *) rgbSdRegs;

    DEBUGCHK(sizeof(SSDHC_REGISTERS) % sizeof(DWORD) == 0);

    for (DWORD dwRegIndex = 0; dwRegIndex < dim(rgbSdRegs); ++dwRegIndex) {
        rgbSdRegs[dwRegIndex] = ReadByte(dwRegIndex);
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("+DumpStdHCRegs - Slot %d -------------------------\n"),
                m_dwSlot));

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SystemAddressLo:    0x%04X \n"), pStdHCRegs->SystemAddressLo)); 
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SystemAddressHi:    0x%04X \n"), pStdHCRegs->SystemAddressHi));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("BlockSize:  0x%04X \n"), pStdHCRegs->BlockSize));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("BlockCount: 0x%04X \n"), pStdHCRegs->BlockCount));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("Argument0:  0x%04X \n"), pStdHCRegs->Argument0));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("Argument1:  0x%04X \n"), pStdHCRegs->Argument1));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("TransferMode:   0x%04X \n"), pStdHCRegs->TransferMode));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("Command:    0x%04X \n"), pStdHCRegs->Command));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R0: 0x%04X \n"), pStdHCRegs->R0));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R1: 0x%04X \n"), pStdHCRegs->R1));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R2: 0x%04X \n"), pStdHCRegs->R2));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R3: 0x%04X \n"), pStdHCRegs->R3));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R4: 0x%04X \n"), pStdHCRegs->R4));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R5: 0x%04X \n"), pStdHCRegs->R5));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R6: 0x%04X \n"), pStdHCRegs->R6));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("R7: 0x%04X \n"), pStdHCRegs->R7));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("BufferDataPort0:    0x%04X \n"), pStdHCRegs->BufferDataPort0));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("BufferDataPort1:    0x%04X \n"), pStdHCRegs->BufferDataPort1));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("PresentState:   0x%04X \n"), pStdHCRegs->PresentState));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("HostControl:    0x%04X \n"), pStdHCRegs->HostControl));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("PowerControl:   0x%04X \n"), pStdHCRegs->PowerControl));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("BlockGapControl:    0x%04X \n"), pStdHCRegs->BlockGapControl));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("WakeUpControl:  0x%04X \n"), pStdHCRegs->WakeUpControl));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("ClockControl:   0x%04X \n"), pStdHCRegs->ClockControl));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("TimeOutControl: 0x%04X \n"), pStdHCRegs->TimeOutControl));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SoftReset:  0x%04X \n"), pStdHCRegs->SoftReset));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("NormalIntStatus:    0x%04X \n"), pStdHCRegs->NormalIntStatus));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("ErrorIntStatus: 0x%04X \n"), pStdHCRegs->ErrorIntStatus));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("NormalIntStatusEnable:  0x%04X \n"), pStdHCRegs->NormalIntStatusEnable));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("ErrorIntStatusEnable:   0x%04X \n"), pStdHCRegs->ErrorIntStatusEnable));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("NormalIntSignalEnable:  0x%04X \n"), pStdHCRegs->NormalIntSignalEnable));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("ErrorIntSignalEnable:   0x%04X \n"), pStdHCRegs->ErrorIntSignalEnable));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("AutoCMD12ErrorStatus:   0x%04X \n"), pStdHCRegs->AutoCMD12ErrorStatus));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("Capabilities:   0x%04X \n"), pStdHCRegs->Capabilities));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("MaxCurrentCapabilites:  0x%04X \n"), pStdHCRegs->MaxCurrentCapabilites));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SlotInterruptStatus:    0x%04X \n"), pStdHCRegs->SlotInterruptStatus));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("HostControllerVersion:  0x%04X \n"), pStdHCRegs->HostControllerVersion));
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("-DumpStdHCRegs-------------------------\n")));
}

#endif

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

