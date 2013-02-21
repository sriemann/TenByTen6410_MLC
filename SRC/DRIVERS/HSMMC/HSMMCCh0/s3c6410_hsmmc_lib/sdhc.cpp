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

// SDHC controller driver implementation

#include "SDCardDDK.h"
#include "SDHC.h"


#ifndef SHIP_BUILD
#define STR_MODULE _T("CSDHCBase::")
#define SETFNAME(name) LPCTSTR pszFname = STR_MODULE name _T(":")
#else
#define SETFNAME(name)
#endif

#ifdef DEBUG
#define MAKE_OPTION_STRING(x) _T(#x)

const LPCTSTR CSDHCBase::sc_rgpszOptions[SDHCDSlotOptionCount] = {
    MAKE_OPTION_STRING(SDHCDSetSlotPower),
    MAKE_OPTION_STRING(SDHCDSetSlotInterface),
    MAKE_OPTION_STRING(SDHCDEnableSDIOInterrupts),
    MAKE_OPTION_STRING(SDHCDDisableSDIOInterrupts),
    MAKE_OPTION_STRING(SDHCDAckSDIOInterrupt),
    MAKE_OPTION_STRING(SDHCDGetWriteProtectStatus),
    MAKE_OPTION_STRING(SDHCDQueryBlockCapability),
    MAKE_OPTION_STRING(SDHCDSetClockStateDuringIdle),
    MAKE_OPTION_STRING(SDHCDSetSlotPowerState),
    MAKE_OPTION_STRING(SDHCDGetSlotPowerState),
    MAKE_OPTION_STRING(SDHCDWakeOnSDIOInterrupts),
    MAKE_OPTION_STRING(SDHCDGetSlotInfo),
    MAKE_OPTION_STRING(SDHCDSetSlotInterfaceEx),
    MAKE_OPTION_STRING(SDHCAllocateDMABuffer),
    MAKE_OPTION_STRING(SDHCFreeDMABuffer),
};
#endif

extern LPCTSTR HostControllerName;

CSDHCBase::CSDHCBase(
                     ) 
                     : m_regDevice()
{    
    m_hBusAccess = NULL;
    m_cSlots = 0;
    m_pSlots = NULL;
    m_pSlotInfos = NULL;
    m_pHCDContext = NULL;
    m_interfaceType = InterfaceTypeUndefined;
    m_dwBusNumber = INVALID_BUS_NUMBER;
    m_hISRHandler = NULL;
    m_dwSysIntr = SYSINTR_UNDEFINED;
    m_dwPriority = 0;
    m_hevInterrupt = NULL;
    m_htIST = NULL;
    m_cpsCurrent = D0;

    m_fHardwareInitialized = FALSE;
    m_fRegisteredWithBusDriver = FALSE;
    m_fDriverShutdown = FALSE;
    m_fInterruptInitialized = FALSE;
}


CSDHCBase::~CSDHCBase(
                      )
{
    // We call PreDeinit just in case we are not being destroyed by
    // a call to SHC_PreDeinit.
    PreDeinit();

    if (m_fHardwareInitialized) {
        DeinitializeHardware();
    }

    if (m_pHCDContext) {
        // Cleanup the host context
        SDHCDDeleteContext(m_pHCDContext);
    }

    if (m_pSlots) delete [] m_pSlots;
    if (m_pSlotInfos) LocalFree(m_pSlotInfos);
    if (m_hBusAccess) CloseBusAccessHandle(m_hBusAccess);
}


BOOL
CSDHCBase::Init(
                LPCTSTR pszActiveKey
                )
{
    BOOL fRet = FALSE;
    SD_API_STATUS status;
    HKEY    hkDevice = NULL;

    hkDevice = OpenDeviceKey(pszActiveKey);
    if (!hkDevice || !m_regDevice.Open(hkDevice, _T(""))) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC: Failed to open device key\n")));
        goto EXIT;
    }

    // Get a handle to our parent bus.
    m_hBusAccess = CreateBusAccessHandle(pszActiveKey);
    if (m_hBusAccess == NULL) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC: Could not get handle to parent\n")));
        goto EXIT;
    }

    m_cSlots = DetermineSlotCount();
    if (!CheckSlotCount(m_cSlots)) {
        goto EXIT;
    }
    PREFAST_ASSERT(m_cSlots <= SDHC_MAX_SLOTS);

    m_pSlotInfos = (PSDHC_SLOT_INFO) LocalAlloc(LPTR, 
        sizeof(SDHC_SLOT_INFO) * m_cSlots);
    if (m_pSlotInfos == NULL) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC Failed to allocate slot info objects\n")));
        goto EXIT;
    }

    status = SDHCDAllocateContext(m_cSlots, &m_pHCDContext);
    if (!SD_API_SUCCESS(status)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC Failed to allocate context : 0x%08X \n"),
            status));
        goto EXIT;
    }
    
    // Set our extension 
    m_pHCDContext->pHCSpecificContext = this;

    if (!InitializeHardware()) {
        goto EXIT;
    }

    // Allocate slot objects
    m_pSlots = AllocateSlotObjects(m_cSlots);
    if (m_pSlots == NULL) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC Failed to allocate slot objects\n")));
        goto EXIT;
    }

    // Initialize the slots
    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PSDHC_SLOT_INFO pSlotInfo = &m_pSlotInfos[dwSlot];
        PCSDHCSlotBase pSlot = GetSlot(dwSlot);

        if (!pSlot->Init(dwSlot, pSlotInfo->pucRegisters, m_pHCDContext, 
            m_dwSysIntr, m_hBusAccess, m_interfaceType, m_dwBusNumber, &m_regDevice)) {
                goto EXIT;
            }
    }

    // set the host controller name
    //SDHCDSetHCName(m_pHCDContext, TEXT("HostControllerName"));
    SDHCDSetHCName(m_pHCDContext, HostControllerName);

    // set init handler
    SDHCDSetControllerInitHandler(m_pHCDContext, CSDHCBase::SDHCInitialize);
    // set deinit handler    
    SDHCDSetControllerDeinitHandler(m_pHCDContext, CSDHCBase::SDHCDeinitialize);
    // set the Send packet handler
    SDHCDSetBusRequestHandler(m_pHCDContext, CSDHCBase::SDHCBusRequestHandler);   
    // set the cancel I/O handler
    SDHCDSetCancelIOHandler(m_pHCDContext, CSDHCBase::SDHCCancelIoHandler);   
    // set the slot option handler
    SDHCDSetSlotOptionHandler(m_pHCDContext, CSDHCBase::SDHCSlotOptionHandler);

    // These values must be set before calling SDHCDRegisterHostController()
    // because they are used during that call.
    m_dwPriority = m_regDevice.ValueDW(SDHC_PRIORITY_KEY, 
        SDHC_CARD_CONTROLLER_PRIORITY);

    // now register the host controller 
    status = SDHCDRegisterHostController(m_pHCDContext);

    if (!SD_API_SUCCESS(status)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC Failed to register host controller: %0x08X \n"), 
            status));
        goto EXIT;
    }

    m_fRegisteredWithBusDriver = TRUE;
    fRet = TRUE;

EXIT:
    if (hkDevice) RegCloseKey(hkDevice);

    return fRet;
}


SD_API_STATUS
CSDHCBase::Start()
{
    SD_API_STATUS status = SD_API_STATUS_INSUFFICIENT_RESOURCES;

    m_fDriverShutdown = FALSE;

    // allocate the interrupt event
    m_hevInterrupt = CreateEvent(NULL, FALSE, FALSE,NULL);

    if (NULL == m_hevInterrupt) {
        goto EXIT;
    }

    // initialize the interrupt event
    if (!InterruptInitialize (m_dwSysIntr, m_hevInterrupt, NULL, 0)) {
        goto EXIT;
    }

    m_fInterruptInitialized = TRUE;

    // create the interrupt thread for controller interrupts
    m_htIST = CreateThread(NULL, 0, ISTStub, this, 0, NULL);
    if (NULL == m_htIST) {
        goto EXIT;
    }

    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PCSDHCSlotBase pSlot = GetSlot(dwSlot);
        status = pSlot->Start();

        if (!SD_API_SUCCESS(status)) {
            goto EXIT;
        }
    }

    // wake up the interrupt thread to check the slot
    ::SetInterruptEvent(m_dwSysIntr);

    status = SD_API_STATUS_SUCCESS;

EXIT:
    if (!SD_API_SUCCESS(status)) {
        // Clean up
        Stop();
    }

    return status;
}


SD_API_STATUS
CSDHCBase::Stop()
{
    // Mark for shutdown
    m_fDriverShutdown = TRUE;

    if (m_fInterruptInitialized) {
        KernelIoControl(IOCTL_HAL_DISABLE_WAKE, &m_dwSysIntr, sizeof(m_dwSysIntr),
            NULL, 0, NULL);

        InterruptDisable(m_dwSysIntr);
    }

    // Clean up controller IST
    if (m_htIST) {
        // Wake up the IST
        SetEvent(m_hevInterrupt);
        WaitForSingleObject(m_htIST, INFINITE); 
        CloseHandle(m_htIST);
        m_htIST = NULL;
    }

    // free controller interrupt event
    if (m_hevInterrupt) {
        CloseHandle(m_hevInterrupt);
        m_hevInterrupt = NULL;
    }

    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PCSDHCSlotBase pSlot = GetSlot(dwSlot);
        pSlot->Stop();
    }

    return SD_API_STATUS_SUCCESS;
}


SD_API_STATUS 
CSDHCBase::SlotOptionHandler(
                             DWORD                 dwSlot, 
                             SD_SLOT_OPTION_CODE   sdOption, 
                             PVOID                 pData,
                             DWORD                 cbData
                             )
{
    SD_API_STATUS   status = SD_API_STATUS_SUCCESS;
    BOOL            fCallSlotsHandler = TRUE;

    Lock();
    Validate();
    PCSDHCSlotBase pSlot = GetSlot(dwSlot);

    DEBUGCHK(sdOption < dim(sc_rgpszOptions));
    DEBUGCHK(sc_rgpszOptions[sdOption] != NULL);
    DEBUGMSG(SDCARD_ZONE_INFO, (_T("CSDHCBase::SlotOptionHandler(%u, %s)\n"),
        dwSlot, sc_rgpszOptions[sdOption]));

    switch (sdOption) {
    case SDHCDSetSlotPower: {
        if (cbData != sizeof(DWORD)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }

    case SDHCDSetSlotInterface: {
        if (cbData != sizeof(SD_CARD_INTERFACE)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }

    case SDHCDEnableSDIOInterrupts:
    case SDHCDDisableSDIOInterrupts:
    case SDHCDAckSDIOInterrupt:
        if (pData || cbData != 0) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;

    case SDHCDGetWriteProtectStatus: {
        if (cbData != sizeof(SD_CARD_INTERFACE)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }

    case SDHCDQueryBlockCapability: {
        if (cbData != sizeof(SD_HOST_BLOCK_CAPABILITY)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }

    case SDHCDSetSlotPowerState: {
        if (cbData != sizeof(CEDEVICE_POWER_STATE)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }

        PCEDEVICE_POWER_STATE pcps = (PCEDEVICE_POWER_STATE) pData;

        if (*pcps < m_cpsCurrent) {
            // Move controller to higher power state initially since
            // it will need to be powered for the slot to access
            // registers.
            SetControllerPowerState(*pcps);
        }

        status = pSlot->SlotOptionHandler(sdOption, pData, cbData);

        // Set the power state based on current conditions. Note that 
        // the slot may have gone to a state different from what was 
        // requested.
        CEDEVICE_POWER_STATE cps = DetermineRequiredControllerPowerState();
        SetControllerPowerState(cps);
        fCallSlotsHandler = FALSE;
        break;
    }

    case SDHCDGetSlotPowerState: {
        if (cbData != sizeof(CEDEVICE_POWER_STATE)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }

    case SDHCDWakeOnSDIOInterrupts: {
        if (cbData != sizeof(BOOL)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }

    case SDHCDGetSlotInfo: {
        if (cbData != sizeof(SDCARD_HC_SLOT_INFO)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }
    case SDHCDSetSlotInterfaceEx: {
        if (cbData != sizeof(SD_CARD_INTERFACE_EX)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }
    case SDHCAllocateDMABuffer: {
        if (pData==NULL || cbData != sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
        break;
    }
    case SDHCFreeDMABuffer:{
        if (pData==NULL || cbData != sizeof(SD_HOST_ALLOC_FREE_DMA_BUFFER)) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
      break;
    }
    
    default:
        break;
    }

    if (SD_API_SUCCESS(status) && fCallSlotsHandler) {
        // Call the slots handler to do the real work.
        __try {
            status = pSlot->SlotOptionHandler(sdOption, pData, cbData);
        }__except (SDProcessException(GetExceptionInformation())) {
            status = SD_API_STATUS_INVALID_PARAMETER;
        }
    }

    Unlock();

    return status;
}


VOID
CSDHCBase::PowerDown(
                     )
{
    Validate();

    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PCSDHCSlotBase pSlot = GetSlot(dwSlot);
        pSlot->PowerDown();
    }

    CEDEVICE_POWER_STATE cps = DetermineRequiredControllerPowerState();
    SetControllerPowerState(cps);
}


VOID
CSDHCBase::PowerUp(
                   )
{
    Validate();

    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PCSDHCSlotBase pSlot = GetSlot(dwSlot);

        CEDEVICE_POWER_STATE cpsRequired = pSlot->GetPowerUpRequirement();
        if (cpsRequired < m_cpsCurrent) {
            // Move controller to higher power state initially since
            // it will need to be powered for the slot to access
            // registers.
            SetControllerPowerState(cpsRequired);
        }

        pSlot->PowerUp();
    }
}


VOID
CSDHCBase::PreDeinit(
                     )
{
    if (m_fRegisteredWithBusDriver) {
        if (m_fDriverShutdown == FALSE) {
            // Deregister the host controller
            SDHCDDeregisterHostController(m_pHCDContext);
        }
        // else the bus driver itself already deregistered us.

        m_fRegisteredWithBusDriver = FALSE;
    }
}


BOOL 
CSDHCBase::InitializeHardware(
                              )
{
    SETFNAME(_T("InitializeHardware"));

    DEBUGCHK(m_hBusAccess);
    DEBUGCHK(m_regDevice.IsOK());
    PREFAST_DEBUGCHK(m_pSlotInfos);
    ValidateSlotCount();

    BOOL fRet = FALSE;
    PHYSICAL_ADDRESS PortAddress;
    DWORD inIoSpace = 0;

    // Read window information
    DDKWINDOWINFO wini;
    wini.cbSize = sizeof(wini);
    DWORD dwStatus = DDKReg_GetWindowInfo(m_regDevice, &wini);
    if (dwStatus != ERROR_SUCCESS) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Error getting window information\r\n"),
            pszFname));
        goto EXIT;
    }

    // Read ISR information
    DDKISRINFO isri;
    isri.cbSize = sizeof(isri);
    dwStatus = DDKReg_GetIsrInfo(m_regDevice, &isri);
    if (dwStatus != ERROR_SUCCESS) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Error getting ISR information\r\n"),
            pszFname));
        goto EXIT;
    }

#ifdef SET_TI_BOARD_PCI_REG
    {
        DDKPCIINFO dpi;
        dpi.cbSize = sizeof(dpi);
        DDKReg_GetPciInfo(m_regDevice, &dpi);

        DWORD RetVal;
        PCI_SLOT_NUMBER SlotNumber;
        SlotNumber.u.AsULONG = 0;
        SlotNumber.u.bits.DeviceNumber = dpi.dwDeviceNumber;
        SlotNumber.u.bits.FunctionNumber = 1;

        HalGetBusDataByOffset( PCIConfiguration,
                               wini.dwBusNumber,
                               SlotNumber.u.AsULONG,
                               &RetVal,
                               0x84,
                               sizeof( RetVal ) );

        if (!(RetVal & 0x00200000)) {
            RetVal |= 0x00200000;
            HalSetBusDataByOffset( PCIConfiguration,
                                   wini.dwBusNumber,
                                   SlotNumber.u.AsULONG,
                                   &RetVal,
                                   0x84,
                                   sizeof( RetVal ) );
        }
    }
#endif

    // Sanity check ISR
    if (isri.dwSysintr == SYSINTR_NOP) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s No sysintr specified in registry\r\n"),
            pszFname));
                // For HSMMC driver request OAL a SYSINTR instead of ststic SYSINTR mapping, Below KernelIoControl function is called.
                if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &(isri.dwIrq), sizeof(DWORD), &(isri.dwSysintr), sizeof(DWORD), NULL))
                {
                    RETAILMSG(TRUE, (TEXT("%s IOCTL_HAL_REQUEST_SYSINTR HSMMC1 Failed \n\r"), pszFname));
                    goto EXIT;
                }
    }

    if (isri.szIsrDll[0] != 0) {
        if ( (isri.szIsrHandler[0] == 0) || (isri.dwIrq == IRQ_UNSPECIFIED) ) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Invalid installable ISR information in registry\r\n"),
                pszFname));
            goto EXIT;
        }
    }

    m_interfaceType = (INTERFACE_TYPE) wini.dwInterfaceType;
    m_dwBusNumber = wini.dwBusNumber;

    DWORD dwSlotZeroWindow;
    dwSlotZeroWindow = DetermineFirstSlotWindow(&wini);

    DEBUGCHK(dwSlotZeroWindow < wini.dwNumMemWindows);
    DEBUGCHK( (dwSlotZeroWindow + m_cSlots) <= wini.dwNumMemWindows );

    // Use the slot zero window for the ISR
    PDEVICEWINDOW pWindowSlotZero = &wini.memWindows[dwSlotZeroWindow];
    PortAddress.LowPart = pWindowSlotZero->dwBase;
    PortAddress.HighPart = 0;

    // Install an ISR, if present
    if (isri.szIsrDll[0] != 0) {
        m_hISRHandler = LoadIntChainHandler(isri.szIsrDll, isri.szIsrHandler, 
            (BYTE) isri.dwIrq);

        if (m_hISRHandler == NULL) {
            DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Error installing ISR\r\n"), pszFname));
            goto EXIT;
        } 
        else {
            GIISR_INFO  Info;
            DWORD       dwPhysAddr;

            fRet = BusTransBusAddrToStatic(m_hBusAccess, 
                (INTERFACE_TYPE) wini.dwInterfaceType,
                wini.dwBusNumber, PortAddress, pWindowSlotZero->dwLen,
                &inIoSpace, (PVOID *) &dwPhysAddr);
            if (fRet == FALSE) {
                DEBUGMSG(SDCARD_ZONE_ERROR,
                    (_T("%s Error translating bus address to static address\r\n"), 
                    pszFname));
                goto EXIT;
            }

            // Initialize ISR
            Info.SysIntr = isri.dwSysintr;
            Info.CheckPort = TRUE;
            Info.PortIsIO = (inIoSpace != 0);
            Info.UseMaskReg = FALSE;
            Info.PortAddr = dwPhysAddr + SDHC_SLOT_INT_STATUS;
            Info.PortSize = sizeof(USHORT);
            Info.Mask = 0xFF;

            fRet = KernelLibIoControl(m_hISRHandler, IOCTL_GIISR_INFO, &Info, 
                sizeof(Info), NULL, 0, NULL);
            if (fRet == FALSE) {
                DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Error setting up ISR\r\n"), pszFname));
                goto EXIT;
            }
        }
    }

    m_dwSysIntr = isri.dwSysintr;
    DEBUGMSG(SDCARD_ZONE_INIT, (_T("%s IRQ 0x%X mapped to SYS_INTR 0x%X\r\n"), 
        pszFname, isri.dwIrq, m_dwSysIntr));

    const DWORD dwEndWindow = dwSlotZeroWindow + m_cSlots;

    for (DWORD dwWindow = dwSlotZeroWindow; dwWindow < dwEndWindow; ++dwWindow) {
        DEBUGCHK(dwWindow < wini.dwNumMemWindows);
        PDEVICEWINDOW pWindowSD = &wini.memWindows[dwWindow];

        DEBUGMSG(SDCARD_ZONE_INIT,
            (_T("%s Base address -> 0x%x; length -> 0x%x \r\n"),
            pszFname, pWindowSD->dwBase, pWindowSD->dwLen));

        PortAddress.LowPart = pWindowSD->dwBase;
        PortAddress.HighPart = 0;

        inIoSpace = 0;
        PVOID pvRegisters;
        DEBUGCHK(pWindowSlotZero->dwLen >= sizeof(SSDHC_REGISTERS));

        fRet = BusTransBusAddrToVirtual(m_hBusAccess, 
            (INTERFACE_TYPE) wini.dwInterfaceType,
            wini.dwBusNumber, PortAddress, pWindowSD->dwLen, &inIoSpace, 
            &pvRegisters);
        if (fRet == FALSE) {
            DEBUGMSG(SDCARD_ZONE_ERROR,
                (_T("%s error translating SD address \r\n"),
                pszFname));
            goto EXIT;
        }

        DEBUGCHK(inIoSpace == 0); // Will not work for I/O mappings.

        DWORD dwSlot = dwWindow - dwSlotZeroWindow;
        DEBUGCHK(dwSlot < m_cSlots);
        m_pSlotInfos[dwSlot].pucRegisters = (volatile UCHAR*) pvRegisters;
        m_pSlotInfos[dwSlot].dwExtraInfo = pWindowSD->dwLen;
    }

    m_fHardwareInitialized = TRUE;
    fRet = TRUE;

EXIT:
    return fRet;
}


BOOL 
CSDHCBase::DeinitializeHardware(
                                )
{
    DEBUGCHK(m_hBusAccess);
    PREFAST_DEBUGCHK(m_pSlotInfos);
    ValidateSlotCount();

    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PVOID pvRegisters = (PVOID) m_pSlotInfos[dwSlot].pucRegisters;
        DWORD dwLen = m_pSlotInfos[dwSlot].dwExtraInfo;
        if (pvRegisters) MmUnmapIoSpace(pvRegisters, dwLen);
    }

    if (m_hISRHandler) FreeIntChainHandler(m_hISRHandler);

    return TRUE;
}

DWORD
CSDHCBase::IST()
{
#ifdef DEBUG
    const DWORD dwTimeout = 2000;
#else
    const DWORD dwTimeout = INFINITE;
#endif
    SETFNAME(_T("IST"));

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("%s Thread Starting\n"), pszFname));

    if (!CeSetThreadPriority(GetCurrentThread(), m_dwPriority)) {
        DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("%s Failed to set CEThreadPriority\n"),
            pszFname));
    }

    while (TRUE) {
        DEBUGCHK(m_hevInterrupt);
        DWORD dwWaitStatus = WaitForSingleObject(m_hevInterrupt, dwTimeout);
        Validate();
        if (m_fDriverShutdown) {
            break;
        }
        if (WAIT_OBJECT_0 != dwWaitStatus) {
            PCSDHCSlotBase pSlotZero = GetSlot(0);
            DEBUGMSG(SDCARD_ZONE_INFO, (TEXT("%s Wait timeout Slot Interrupt Reg x%08X ! Wait status 0x%08X\n"),pszFname,pSlotZero->ReadControllerInterrupts(), dwWaitStatus));
        } else {
            HandleInterrupt();
            InterruptDone(m_dwSysIntr);
        }
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("%s Thread Exiting\n"), pszFname));
    return 0;
}


VOID 
CSDHCBase::HandleInterrupt(
                           ) 
{
    Lock();

    // Use slot zero to get the shared global interrupt register
    PCSDHCSlotBase pSlotZero = GetSlot(0);
    DWORD dwIntStatus = 0;
    dwIntStatus = pSlotZero->ReadControllerInterrupts();
    //DWORD dwIntStatus = pSlotZero->ReadControllerInterrupts();

    do {
        if ( (dwIntStatus) || pSlotZero->NeedsServicing() ) {
           pSlotZero->HandleInterrupt();
        }

        dwIntStatus = pSlotZero->ReadControllerInterrupts();
        // In order to prevent infinite CARD INT occuring, below code is needed because of the architecture of HSMMC on s3c6410.
         if(dwIntStatus && pSlotZero->IsOnlySDIOInterrupt()) break;
    } while (dwIntStatus);
    Unlock();
}


DWORD
CSDHCBase::DetermineSlotCount(
                              )
{
    SETFNAME(_T("DetermineSlotCount"));

    DWORD cSlots = 0;

    // Read window information
    DDKWINDOWINFO wini = { sizeof(wini) };
    DWORD dwStatus = DDKReg_GetWindowInfo(m_regDevice, &wini);
    if (dwStatus != ERROR_SUCCESS) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s Error getting window information\r\n"),
            pszFname));
        goto EXIT;
    }

    cSlots = wini.dwNumMemWindows;

    if (cSlots == 0) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (_T("%s There were not any reported slots.\r\n"),
            pszFname));
        goto EXIT;
    }

EXIT:
    return cSlots;
}




DWORD 
CSDHCBase::DetermineFirstSlotWindow(
                                    PDDKWINDOWINFO pwini
                                    )
{
    PREFAST_DEBUGCHK(pwini);
    DEBUGCHK(pwini->dwNumMemWindows >= m_cSlots);
    DEBUGCHK(pwini->dwNumMemWindows > 0);

    DWORD dwSlotZeroWindow = 0;

    return dwSlotZeroWindow;
}


CEDEVICE_POWER_STATE
CSDHCBase::DetermineRequiredControllerPowerState(
    )
{
    CEDEVICE_POWER_STATE cps = D4;

    for (DWORD dwSlot = 0; dwSlot < m_cSlots; ++dwSlot) {
        PCSDHCSlotBase pSlot = GetSlot(dwSlot);
        cps = min(cps, pSlot->GetPowerState());
    }

    return cps;
}


SD_API_STATUS
CSDHCBase::SetControllerPowerState(
                                   CEDEVICE_POWER_STATE cpsNew
                                   )
{
    if (cpsNew != m_cpsCurrent) {
        switch (cpsNew) {
        case D0:
        case D4:
            KernelIoControl(IOCTL_HAL_DISABLE_WAKE, &m_dwSysIntr, 
                sizeof(m_dwSysIntr), NULL, 0, NULL);
            break;

        case D3:
            KernelIoControl(IOCTL_HAL_ENABLE_WAKE, &m_dwSysIntr, 
                sizeof(m_dwSysIntr), NULL, 0, NULL);
            break;
        }

        SetDevicePowerState(m_hBusAccess, cpsNew, NULL);
        m_cpsCurrent = cpsNew;
    }

    return SD_API_STATUS_SUCCESS;
}


#ifdef DEBUG
VOID 
CSDHCBase::Validate(
                    )
{
    DEBUGCHK(m_regDevice.IsOK());
    DEBUGCHK(m_hBusAccess);
    ValidateSlotCount();
    DEBUGCHK(m_pSlots);
    DEBUGCHK(m_pSlotInfos);
    DEBUGCHK(m_pHCDContext);
    DEBUGCHK(m_dwBusNumber != INVALID_BUS_NUMBER);
    DEBUGCHK(m_interfaceType != InterfaceTypeUndefined);
    DEBUGCHK(m_dwSysIntr != SYSINTR_UNDEFINED);
    DEBUGCHK(VALID_DX(m_cpsCurrent));

    if (m_fRegisteredWithBusDriver && !m_fDriverShutdown) {
        DEBUGCHK(m_htIST);
        DEBUGCHK(m_fHardwareInitialized);
        DEBUGCHK(m_fInterruptInitialized);
    }
}
#endif


// Get the creation proc address and call it
PCSDHCBase
CSDHCBase::CreateSDHCControllerObject(
    LPCTSTR pszActiveKey
    )
{
    PCSDHCBase pSDHC = NULL;
    HKEY hkDevice = OpenDeviceKey(pszActiveKey);

    if (hkDevice) {
        CReg regDevice(hkDevice, _T(""));
        DEBUGCHK(regDevice.IsOK());
        TCHAR szDll[MAX_PATH];

        if (regDevice.ValueSZ(DEVLOAD_DLLNAME_VALNAME, szDll, dim(szDll))) {
            szDll[dim(szDll) - 1] = 0; // Null-terminate
            
            TCHAR szProc[MAX_PATH];

            if (regDevice.ValueSZ(SDHC_CREATION_PROC_KEY, szProc, dim(szProc))) {
                szProc[dim(szProc) - 1] = 0; // Null-terminate
                
                HMODULE hMod = LoadLibrary(szDll);
                if (hMod) {
                    LPSDHC_CREATION_PROC pfnCreate = (LPSDHC_CREATION_PROC)
                        GetProcAddress(hMod, szProc);
                    if (pfnCreate) {
                        pSDHC = (*pfnCreate)();
                    }
                    
                    FreeLibrary(hMod);
                }
            }
        }

        RegCloseKey(hkDevice);
    }

    return pSDHC;
}


///////////////////////////////////////////////////////////////////////////////
//  SDHCInitialize - Initialize the the controller
//  Input:  pHCContext -  host controller context
//          
//  Output: 
//  Return: SD_API_STATUS
//  Notes:  
//          
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHCBase::SDHCInitialize(PSDCARD_HC_CONTEXT pHCContext)
{
    DEBUGMSG(SDCARD_ZONE_INIT,(TEXT("SDHCInitialize++\n")));

    PREFAST_DEBUGCHK(pHCContext);
    PCSDHCBase pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    PREFAST_DEBUGCHK(pController);
    SD_API_STATUS status = pController->Start();

    DEBUGMSG(SDCARD_ZONE_INIT,(TEXT("SDHCInitialize--\n")));

    return status;
}


///////////////////////////////////////////////////////////////////////////////
//  SDHCDeinitialize - Deinitialize the SDHC Controller
//  Input:  pHCContext - HC context
//          
//  Output: 
//  Return: SD_API_STATUS
//  Notes:  
//          
//
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDHCBase::SDHCDeinitialize(PSDCARD_HC_CONTEXT pHCContext)
{
    DEBUGMSG(SDCARD_ZONE_INIT,(TEXT("SDHCDeinitialize++\n")));

    PREFAST_DEBUGCHK(pHCContext);
    PCSDHCBase pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    PREFAST_DEBUGCHK(pController);
    SD_API_STATUS status = pController->Stop();

    DEBUGMSG(SDCARD_ZONE_INIT,(TEXT("SDHCDeinitialize--\n")));

    return status;
}


///////////////////////////////////////////////////////////////////////////////
//  SDHCSDCancelIoHandler - io cancel handler 
//  Input:  pHostContext - host controller context
//          dwSlot - slot the request is going on
//          pRequest - the request to be cancelled
//          
//  Output: 
//  Return: TRUE if I/O was cancelled
//  Notes:  
//          the HC lock is taken before entering this cancel handler
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN 
CSDHCBase::SDHCCancelIoHandler(
                    PSDCARD_HC_CONTEXT  pHCContext,
                    DWORD               dwSlot,
                    PSD_BUS_REQUEST     pRequest
                    )
{
    PREFAST_DEBUGCHK(pHCContext);
    PCSDHCBase pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    PREFAST_DEBUGCHK(pController);
    return pController->CancelIoHandler(dwSlot, pRequest);
}


///////////////////////////////////////////////////////////////////////////////
//  SDHCBusRequestHandler - bus request handler 
//  Input:  pHostContext - host controller context
//          dwSlot - slot the request is going to
//          pRequest - the request
//          
//  Output: 
//  Return: SD_API_STATUS
//  Notes:  The request passed in is marked as uncancelable, this function
//          has the option of making the outstanding request cancelable    
//          returns status pending
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS 
CSDHCBase::SDHCBusRequestHandler(
                      PSDCARD_HC_CONTEXT pHCContext, 
                      DWORD              dwSlot, 
                      PSD_BUS_REQUEST    pRequest
                      ) 
{
    PREFAST_DEBUGCHK(pHCContext);
    PCSDHCBase pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    PREFAST_DEBUGCHK(pController);
    return pController->BusRequestHandler(dwSlot, pRequest);
}

///////////////////////////////////////////////////////////////////////////////
//  SDHCSlotOptionHandler - handler for slot option changes
//  Input:  pHostContext - host controller context
//          dwSlot       - the slot the change is being applied to
//          Option       - the option code
//          pData        - data associaSHC with the option
//  Output: 
//  Return: SD_API_STATUS
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS 
CSDHCBase::SDHCSlotOptionHandler(
                      PSDCARD_HC_CONTEXT    pHCContext,
                      DWORD                 dwSlot, 
                      SD_SLOT_OPTION_CODE   sdOption, 
                      PVOID                 pData,
                      ULONG                 ulOptionSize
                      )
{
    PCSDHCBase pController = GET_PCONTROLLER_FROM_HCD(pHCContext);
    PREFAST_DEBUGCHK(pController);
    return pController->SlotOptionHandler(dwSlot, sdOption, pData, ulOptionSize);
}


// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

