//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#include <atamain.h>
#include <diskmain.h>
//#include "../atapiromi/atapiromi.h"

// DLL instance handle; differentiate this driver from other ATAPI instances
// (what other buses?)
HINSTANCE g_hInstance;

// List of active devices/disks
CDisk *g_pDiskRoot = NULL;

// Protect global variables
CRITICAL_SECTION g_csMain;

// Debug

extern "C" BOOL RegisterDbgZones(HMODULE hMod, LPDBGPARAM lpdbgparam);

// Definitions

typedef CDisk *(* POBJECTFUNCTION)(HKEY hKey);

// IDE/ATA bus implementation

// Constructor
CIDEBUS::CIDEBUS(
    )
{
    // initialize device handle table and device registry key name table
    for (int i = 0; i < MAX_DEVICES_PER_CONTROLLER; i++) {
        m_hDevice[i] = NULL;
        m_szDevice[i] = NULL;
        m_pPort[i] = NULL;
    }
    // initialize DDKREGWINDOW structure
    memset(&m_dwi, 0, sizeof(m_dwi));
    m_pIdeReg = NULL;
    m_bisIOMapped = TRUE;
}

// Destructor
CIDEBUS::~CIDEBUS(
    )
{
    // deinitialize device handle table and device registry key name table
    for (int i = 0; i < MAX_DEVICES_PER_CONTROLLER; i++) {
        if (m_hDevice[i]) {
            BOOL fOk = DeactivateDevice(m_hDevice[i]);
            DEBUGCHK(fOk);
            DEBUGCHK(m_szDevice[i] != NULL);
        }
        if (m_szDevice[i] != NULL) {
            delete m_szDevice[i];
        }
    }
    // delete IDE_ registry value set
    if (m_pIdeReg) {
        if (m_pIdeReg->pszSpawnFunction) {
            LocalFree(m_pIdeReg->pszSpawnFunction);
        }
        if (m_pIdeReg->pszIsrDll) {
            LocalFree(m_pIdeReg->pszIsrDll);
        }
        if (m_pIdeReg->pszIsrHandler) {
            LocalFree(m_pIdeReg->pszIsrHandler);
        }
        LocalFree(m_pIdeReg);
    }
    // deinitialize port structures
    for (int i = 0; i < MAX_DEVICES_PER_CONTROLLER; i++) {
        if (m_pPort[i]) delete m_pPort[i];
    }
}

// IDE/ATA channel/port implementation

// Constructor
CPort::CPort(
    CIDEBUS *pParent
    )
{
    DEBUGCHK(pParent);
    InitializeCriticalSection(&m_csPort);
    // hook up bus
    m_pController = pParent;
    // initialize flags
    m_fInitialized = 0;
    m_dwFlag = 0;
    // initialize I/O ports
    m_dwRegBase = 0;
    m_dwRegAlt = 0;
    m_dwBMR = 0;
    m_dwBMRStatic = 0;
    // initialize interrupt data
    m_hIRQEvent = NULL;
    m_hThread = NULL;
    m_dwSysIntr = SYSINTR_NOP;
    m_dwIrq = IRQ_UNSPECIFIED;
    // initialize master/slave registry value set
    m_pDskReg[0] = NULL;
    m_pDskReg[1] = NULL;
    // initialize master/slave stream interface handles
    m_pDisk[0] = NULL;
    m_pDisk[1] = NULL;
}

// Destructor
CPort::~CPort(
    )
{
    DeleteCriticalSection(&m_csPort);

    // unmap ATA channel's I/O windows if they had been memory mapped
    if (!m_pController->m_bisIOMapped) {
        if (m_dwRegBase) {
            MmUnmapIoSpace((LPVOID)m_dwRegBase, ATA_REG_LENGTH);
        }
        if (m_dwRegAlt) {
            MmUnmapIoSpace((LPVOID)m_dwRegAlt, ATA_REG_LENGTH);
        }
        if (m_dwBMR) {
            MmUnmapIoSpace((LPVOID)m_dwBMR, 16);
        }
    }
    
    // close interrupt event handle
    if (m_hIRQEvent) {
        CloseHandle(m_hIRQEvent);
    }
    // close interrupt thread
    if (m_hThread) {
        CloseHandle(m_hThread);
    }
    // disable interrupt
    if (m_dwSysIntr != SYSINTR_NOP) {
        InterruptDisable(m_dwSysIntr);
    }
    // free DSK_ registry value set
    if (m_pDskReg[0]) {
        LocalFree(m_pDskReg[0]);
    }
    if (m_pDskReg[1]) {
        LocalFree(m_pDskReg[1]);
    }
}

// Acquire exclusive access to IDE/ATA channel's I/O window
VOID
CPort::TakeCS(
    )
{
    EnterCriticalSection(&m_csPort);
}

// Release exclusive access to IDE/ATA channel's I/O window
VOID
CPort::ReleaseCS(
    )
{
    LeaveCriticalSection(&m_csPort);
}

// Write I/O window and interrupt data to debug output
VOID
CPort::PrintInfo(
    )
{
    DEBUGMSG(ZONE_INIT, (TEXT("dwRegBase   = %08X\r\n"), m_dwRegBase));
    DEBUGMSG(ZONE_INIT, (TEXT("dwRegAlt    = %08X\r\n"), m_dwRegAlt));
    DEBUGMSG(ZONE_INIT, (TEXT("dwBMR       = %08X\r\n"), m_dwBMR));
    DEBUGMSG(ZONE_INIT, (TEXT("dwSysIntr   = %08X\r\n"), m_dwSysIntr));
    DEBUGMSG(ZONE_INIT, (TEXT("dwIrq       = %08X\r\n"), m_dwIrq));
    DEBUGMSG(ZONE_INIT, (TEXT("dwBMRStatic = %08X\r\n"), m_dwBMRStatic));
}

// Helper functions

// This function is used by an Xxx_Init function to fetch the name of and return
// a handle to the instance/device ("Key") key from an Active key
HKEY
AtaLoadRegKey(
    HKEY hActiveKey,
    TCHAR **pszDevKey
    )
{
    DWORD dwValueType;        // registry value type
    DWORD dwValueLength = 0;  // registry value length
    PTSTR szDeviceKey = NULL; // name of device key; value associated with "Key"
    HKEY hDeviceKey = NULL;   // handle to device key; handle to "Key"

    // query the value of "Key" with @dwValueLength=0, to determine the actual
    // length of the value (so as to allocate the exact amount of memory)

    if (ERROR_SUCCESS == RegQueryValueEx(hActiveKey, DEVLOAD_DEVKEY_VALNAME, NULL, &dwValueType, NULL, &dwValueLength)) {

        // allocate just enough memory to store the value of "Key"
        szDeviceKey = (PTSTR)LocalAlloc(LPTR, dwValueLength);
        if (szDeviceKey) {

            // read the actual value of "Key" and null terminate the target buffer
            if ( ERROR_SUCCESS == RegQueryValueEx(hActiveKey, DEVLOAD_DEVKEY_VALNAME, NULL, &dwValueType, (PBYTE)szDeviceKey, &dwValueLength)) {
                
                DEBUGCHK(dwValueLength != 0);
                szDeviceKey[(dwValueLength / sizeof(TCHAR)) - 1] = 0;
    
                // open the device key
                if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDeviceKey, 0, 0, &hDeviceKey)) {
                    DEBUGMSG(ZONE_INIT, (_T(
                        "AtaLoadRegyKey> Failed to open %s\r\n"
                        ), szDeviceKey));
                    hDeviceKey = NULL;
                }
            }
        }
    }
    if (!hDeviceKey) {
        if (szDeviceKey) {
            LocalFree(szDeviceKey);
        }
        *pszDevKey = NULL;
    }
    else {
        *pszDevKey = szDeviceKey;
    }
    return hDeviceKey;
}

// This function is used to determine whether a target disk instance is valid
BOOL
AtaIsValidDisk(
    CDisk *pDisk
    )
{
    CDisk *pTemp = g_pDiskRoot;
    while (pTemp) {
        if (pTemp == pDisk) {
            return TRUE;
        }
        pTemp = pTemp->m_pNextDisk;
    }
    return FALSE;
}

// This function is used to fetch an IDE/ATA channel's I/O window from its instance
// key; this function recovers gracefully if an OEM has a proprietary registry
// configuration that doesn't specify bus type or bus number
BOOL
AtaGetRegistryResources(
    HKEY hDevKey,
    PDDKWINDOWINFO pdwi
    )
{
    DEBUGCHK(pdwi != NULL);

    if (!pdwi) {
        return FALSE;
    }

    // fetch I/O window information
    pdwi->cbSize = sizeof(*pdwi);
    if (ERROR_SUCCESS != ::DDKReg_GetWindowInfo(hDevKey, pdwi)) {
        return FALSE;
    }

    // if interface not specified, then assume PCI
    if (pdwi->dwInterfaceType == InterfaceTypeUndefined) {
        DEBUGMSG(ZONE_WARNING, (_T(
            "Atapi!AtaGetRegistryResources> bus type not specified, using PCI as default\r\n"
            )));
        pdwi->dwInterfaceType = PCIBus;
    }

    return TRUE;
}

// This function translates a bus address in an I/O window to a virtual address
DWORD
DoIoTranslation(
    PDDKWINDOWINFO pdwi,
    DWORD dwIoWindowIndex,
    PBOOL pbisIOmapped
    )
{
    PHYSICAL_ADDRESS PhysicalAddress; // bus address
    DWORD AddressSpace = 1;           // mark bus address as being in an I/O window
    LPVOID pAddress;                  // return

    DEBUGCHK(pbisIOmapped != NULL);
    *pbisIOmapped = TRUE;
    DEBUGCHK(pdwi != NULL);
    DEBUGCHK(dwIoWindowIndex < MAX_DEVICE_WINDOWS);
    if (!pdwi) {
        return NULL;
    }

    // extract the target bus address
    PhysicalAddress.HighPart = 0;
    PhysicalAddress.LowPart = pdwi->ioWindows[dwIoWindowIndex].dwBase;

    // translate the target bus address to a virtual address
    if (!TransBusAddrToVirtual(
        (INTERFACE_TYPE)pdwi->dwInterfaceType,
        pdwi->dwBusNumber,
        PhysicalAddress,
        pdwi->ioWindows[dwIoWindowIndex].dwLen,
        &AddressSpace,
        &pAddress
     )) {
        return NULL;
    }
    
    *pbisIOmapped = BOOL(AddressSpace);
    return (DWORD)pAddress;
}

// This function translates a bus address to a statically physical address
DWORD
DoStaticTranslation(
    PDDKWINDOWINFO pdwi,
    DWORD dwIoWindowIndex
    )
{
    PHYSICAL_ADDRESS PhysicalAddress; // bus address
    DWORD AddressSpace = 1;           // mark bus address as being in an I/O window
    LPVOID pAddress;                  // return

    DEBUGCHK(pdwi != NULL);
    DEBUGCHK(dwIoWindowIndex < MAX_DEVICE_WINDOWS);

    if (!pdwi) {
        return NULL;
    }

    // extract bus address
    PhysicalAddress.HighPart = 0;
    PhysicalAddress.LowPart = pdwi->ioWindows[dwIoWindowIndex].dwBase;

    // translate the target bus address to a statically mapped physical address
    if (!TransBusAddrToStatic(
        (INTERFACE_TYPE)pdwi->dwInterfaceType,
        pdwi->dwBusNumber,
        PhysicalAddress,
        pdwi->ioWindows[dwIoWindowIndex].dwLen,
        &AddressSpace,
        &pAddress
    )) {
        return NULL;
    }

    return (DWORD)pAddress;
}

// This function reads the IDE registry value set from the IDE/ATA controller's
// registry key
BOOL
GetIDERegistryValueSet(
    HKEY hIDEInstanceKey,
    PIDEREG pIdeReg
    )
{
    BOOL fRet;

    DEBUGCHK(NULL != pIdeReg);

    // fetch legacy boolean
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_LEGACY, &pIdeReg->dwLegacy);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_LEGACY));
        return FALSE;
    }
    if (pIdeReg->dwLegacy >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Bad value(%d) for %s in IDE instance key; valid: {0, 1}\r\n"
            ), pIdeReg->dwLegacy, REG_VAL_IDE_LEGACY));
        return FALSE;
    }

    // fetch IRQ; this value is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_IRQ, &pIdeReg->dwIrq);
    if (!fRet) {
        pIdeReg->dwIrq = IRQ_UNSPECIFIED;
    }

    // fetch SysIntr; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_SYSINTR, &pIdeReg->dwSysIntr);
    if (!fRet) {
        pIdeReg->dwSysIntr = SYSINTR_NOP;
    }

    // fetch vendor id; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_VENDORID, &pIdeReg->dwVendorId);
    if (!fRet) {
        pIdeReg->dwVendorId = 0;
    }

    // fetch DMA alignment; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_DMAALIGNMENT, &pIdeReg->dwDMAAlignment);
    if (!fRet) {
        pIdeReg->dwDMAAlignment = 0;
    }

    // fetch soft reset timeout
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_SOFTRESETTIMEOUT, &pIdeReg->dwSoftResetTimeout);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_SOFTRESETTIMEOUT));
        return FALSE;
    }

    // fetch Status register poll cycles
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_STATUSPOLLCYCLES, &pIdeReg->dwStatusPollCycles);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_STATUSPOLLCYCLES));
        return FALSE;
    }

    // fetch Status register polls per cycle
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_STATUSPOLLSPERCYCLE, &pIdeReg->dwStatusPollsPerCycle);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_STATUSPOLLSPERCYCLE));
        return FALSE;
    }

    // fetch Status register poll cycle pause
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_STATUSPOLLCYCLEPAUSE, &pIdeReg->dwStatusPollCyclePause);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_STATUSPOLLCYCLEPAUSE));
        return FALSE;
    }

    // fetch spawn function
    fRet = AtaGetRegistryString(hIDEInstanceKey, REG_VAL_IDE_SPAWNFUNCTION, &pIdeReg->pszSpawnFunction);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_SPAWNFUNCTION));
        return FALSE;
    }

    // fetch ISR dll; this is not mandatory; allocate pszIsrDll
    fRet = AtaGetRegistryString(hIDEInstanceKey, REG_VAL_IDE_ISRDLL, &pIdeReg->pszIsrDll, 0);
    if (!fRet) {
        pIdeReg->pszIsrDll = NULL;
    }

    // fetch ISR handler; this is not mandatory; allocate pszIsrHandler
    fRet = AtaGetRegistryString(hIDEInstanceKey, REG_VAL_IDE_ISRHANDLER, &pIdeReg->pszIsrHandler, 0);
    if (!fRet) {
        pIdeReg->pszIsrHandler = NULL;
    }

    // fetch device control offset; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_DEVICECONTROLOFFSET, &pIdeReg->dwDeviceControlOffset);
    if (!fRet) {
        // this value is only used by atapipcmcia
        pIdeReg->dwDeviceControlOffset = ATA_REG_ALT_STATUS;
    }

    // fetch alternate status offset; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_ALTERNATESTATUSOFFSET, &pIdeReg->dwAlternateStatusOffset);
    if (!fRet) {
        // this value is only used by atapipcmcia
        pIdeReg->dwAlternateStatusOffset = ATA_REG_DRV_CTRL;
    }

    // fetch register stride
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_REGISTERSTRIDE, &pIdeReg->dwRegisterStride);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Failed to read %s from IDE instance key\r\n"
            ), REG_VAL_IDE_REGISTERSTRIDE));
        return FALSE;
    }
    if (0 == pIdeReg->dwRegisterStride) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetIDERegistryValueSet> Bad value(%d) for %s in IDE instance key; valid: > 0\r\n"
            ), pIdeReg->dwRegisterStride, REG_VAL_IDE_REGISTERSTRIDE));
        return FALSE;
    }

    // fetch disable 48-bit LBA boolean; this is not mandatory
    fRet = AtaGetRegistryValue(hIDEInstanceKey, REG_VAL_IDE_DISABLE48BITLBA, &pIdeReg->dwDisable48BitLBA);
    if (!fRet) {
        pIdeReg->dwDisable48BitLBA = 0;
    }

    return TRUE;
}

// This function reads the DSK registry value set from the IDE/ATA controller's
// registry key
BOOL
GetDSKRegistryValueSet(
    HKEY hDSKInstanceKey,
    PDSKREG pDskReg
    )
{
    BOOL fRet;

    // fetch device ID
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DEVICEID, &pDskReg->dwDeviceId);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DEVICEID));
        return FALSE;
    }
    if (!((0 <= pDskReg->dwDeviceId) && (pDskReg->dwDeviceId <= 3))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0, 1, 2, 3}\r\n"
            ), pDskReg->dwDeviceId, REG_VAL_DSK_DEVICEID));
        return FALSE;
    }

    // fetch interrupt driven I/O boolean
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_INTERRUPTDRIVEN, &pDskReg->dwInterruptDriven);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_INTERRUPTDRIVEN));
        return FALSE;
    }
    if (pDskReg->dwInterruptDriven >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0, 1}\r\n"
            ), pDskReg->dwInterruptDriven, REG_VAL_DSK_INTERRUPTDRIVEN));
        return FALSE;
    }

    // fetch DMA triple
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DMA, &pDskReg->dwDMA);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DMA));
        pDskReg->dwDMA = 0;
    }
    if (pDskReg->dwDMA >= 3) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0=no DMA, 1=DMA, 2=ATA DMA only}\r\n"
            ), pDskReg->dwDMA, REG_VAL_DSK_DMA));
        return FALSE;
    }

    // fetch double buffer size
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DOUBLEBUFFERSIZE, &pDskReg->dwDoubleBufferSize);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DOUBLEBUFFERSIZE));
        return FALSE;
    }
    if ((0 != pDskReg->dwDoubleBufferSize) && ((pDskReg->dwDoubleBufferSize < REG_VAL_DSK_DOUBLEBUFFERSIZE_MIN) || (pDskReg->dwDoubleBufferSize > REG_VAL_DSK_DOUBLEBUFFERSIZE_MAX))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {%d, ..., %d}\r\n"
            ), pDskReg->dwDoubleBufferSize, REG_VAL_DSK_DOUBLEBUFFERSIZE, REG_VAL_DSK_DOUBLEBUFFERSIZE_MIN, REG_VAL_DSK_DOUBLEBUFFERSIZE_MAX));
        return FALSE;
    }
    if (0 != (pDskReg->dwDoubleBufferSize % SECTOR_SIZE)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; must be multiple of %d\r\n"
            ), pDskReg->dwDoubleBufferSize, REG_VAL_DSK_DOUBLEBUFFERSIZE, SECTOR_SIZE));
        return FALSE;
    }

    // fetch DRQ data block size
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_DRQDATABLOCKSIZE, &pDskReg->dwDrqDataBlockSize);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_DRQDATABLOCKSIZE));
        return FALSE;
    }
    if (pDskReg->dwDrqDataBlockSize > REG_VAL_DSK_DRQDATABLOCKSIZE_MAX) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {%d, ..., %d}\r\n"
            ), pDskReg->dwDrqDataBlockSize, REG_VAL_DSK_DRQDATABLOCKSIZE, REG_VAL_DSK_DRQDATABLOCKSIZE_MIN, REG_VAL_DSK_DRQDATABLOCKSIZE_MAX));
        return FALSE;
    }
    if (0 != (pDskReg->dwDrqDataBlockSize % SECTOR_SIZE)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; must be multiple of %d\r\n"
            ), pDskReg->dwDrqDataBlockSize, REG_VAL_DSK_DRQDATABLOCKSIZE, SECTOR_SIZE));
        return FALSE;
    }

    // fetch write cache boolean
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_WRITECACHE, &pDskReg->dwWriteCache);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_WRITECACHE));
        return FALSE;
    }
    if (pDskReg->dwWriteCache >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0, 1}\r\n"
            ), pDskReg->dwWriteCache, REG_VAL_DSK_WRITECACHE));
        return FALSE;
    }

    // fetch look-ahead boolean
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_LOOKAHEAD, &pDskReg->dwLookAhead);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_LOOKAHEAD));
        return FALSE;
    }
    if (pDskReg->dwLookAhead >= 2) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Bad value(%d) for %s in DSK instance key; valid: {0, 1}\r\n"
            ), pDskReg->dwLookAhead, REG_VAL_DSK_LOOKAHEAD));
        return FALSE;
    }

    // fetch transfer mode
    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_TRANSFERMODE, &pDskReg->dwTransferMode);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_TRANSFERMODE));
        return FALSE;
    }

    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_ENABLE_PDMA, &pDskReg->dwEnablePDMA);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_ENABLE_PDMA));
        pDskReg->dwEnablePDMA= 0;
    }

    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_ENABLE_UDMA, &pDskReg->dwEnableUDMA);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_ENABLE_UDMA));
        pDskReg->dwEnableUDMA= 0;
    } 

    fRet = AtaGetRegistryValue(hDSKInstanceKey, REG_VAL_DSK_INDIRECT_MODE, &pDskReg->dwIndirectMode);
    if (!fRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!GetDSKRegistryValueSet> Failed to read %s from DSK instance key\r\n"
            ), REG_VAL_DSK_ENABLE_UDMA));
        pDskReg->dwIndirectMode= 0;
    }     
    return TRUE;
}

// This function reads the I/O window data from the IDE instance key and builds
// the I/O ports for the primary and secondary IDE controller channels
BOOL
GetIoPort(
    HKEY hDevKey,
    PTSTR szDevKey,
    CIDEBUS *pBus
    )
{
    BOOL fRet = FALSE;
    BOOL isIOMapped = FALSE;

    DEBUGCHK(pBus);
    DEBUGCHK(pBus->m_pPort[ATA_PRIMARY]);
    DEBUGCHK(pBus->m_pPort[ATA_SECONDARY]);

    // TODO: What if the IDE/ATA controller only supports a single device on
    // TODO: (con't) a single channel and bus mastering?

    // fetch the IDE/ATA channel's I/O window; a channel can contain a single device
    // or a master and a slave device; each device requires a device control I/O
    // window and an alternate status I/O window; if the IDE/ATA controller supports
    // bus mastering, then a bus master I/O window will be present

    if (
        (!AtaGetRegistryResources(hDevKey, &pBus->m_dwi)) ||
        (pBus->m_dwi.dwNumIoWindows < 2)
    ) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!BuildPortSet> Resource configuration missing or invalid in device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // save the base virtual addresss of the device control (RegBase)
    // and alternate status (RegAlt) I/O windows

    pBus->m_pPort[ATA_PRIMARY]->m_dwRegBase   = DoIoTranslation(&pBus->m_dwi, 0,&isIOMapped);
    pBus->m_bisIOMapped                       = isIOMapped;
    pBus->m_pPort[ATA_PRIMARY]->m_dwRegAlt    = DoIoTranslation(&pBus->m_dwi, 1,&isIOMapped);
    DEBUGCHK(pBus->m_bisIOMapped == isIOMapped); // If one register is IO/memory mapped so are all others
    pBus->m_pPort[ATA_PRIMARY]->m_fInitialized = TRUE;

    if (pBus->m_dwi.dwNumIoWindows >= 4) {

        // this channel supports a primary and secondary IDE/ATA channels; save
        // the base virtual addresses of the secondary channel's devoce control
        // and alternate status I/O windows

        pBus->m_pPort[ATA_SECONDARY]->m_dwRegBase   = DoIoTranslation(&pBus->m_dwi, 2,&isIOMapped);
        DEBUGCHK(pBus->m_bisIOMapped == isIOMapped); // If one register is IO/memory mapped so are all others
        pBus->m_pPort[ATA_SECONDARY]->m_dwRegAlt    = DoIoTranslation(&pBus->m_dwi, 3,&isIOMapped);
        DEBUGCHK(pBus->m_bisIOMapped == isIOMapped); // If one register is IO/memory mapped so are all others
        pBus->m_pPort[ATA_SECONDARY]->m_fInitialized = TRUE;

    }
    if (pBus->m_dwi.dwNumIoWindows >= 5) {

        // the IDE/ATA controller supports bus mastering; save the base virtual
        // address of each channel's bus master I/O window

        DEBUGCHK(pBus->m_dwi.ioWindows[4].dwLen >= 16);

        pBus->m_pPort[ATA_PRIMARY]->m_dwBMR = DoIoTranslation(&pBus->m_dwi, 4,&isIOMapped);
        DEBUGCHK(pBus->m_bisIOMapped == isIOMapped); // If one register is IO/memory mapped so are all others
        pBus->m_pPort[ATA_PRIMARY]->m_dwBMRStatic= DoStaticTranslation(&pBus->m_dwi, 4);
        pBus->m_pPort[ATA_SECONDARY]->m_dwBMR = pBus->m_pPort[ATA_PRIMARY]->m_dwBMR + 8;
        pBus->m_pPort[ATA_SECONDARY]->m_dwBMRStatic = pBus->m_pPort[ATA_PRIMARY]->m_dwBMRStatic + 8;
    }

    fRet = TRUE;

exit:
    return fRet;
}

// IDE/ATA and ATA/ATAPI device stream interface

/*++

DSK_Init
    This function is called as a result of IDE_Init calling ActivateDevice on
    HKLM\Drivers\@BUS\@IDEAdapter\DeviceX, to initialize a master or slave
    device on a particular IDE/ATA channel of a particular IDE/ATA controller.
    That is, an "IDE" driver is a bus driver for devices to one its IDE/ATA
    controller's channels.

    This function is responsible for creating a CDisk instance to associate
    with a device.  This function reads the "Object" value from its instance
    key to determine which CDisk (sub)type to instantiate and calls Init on the
    CDisk instance to initialize the device.  If the device is not present, then
    Init will fail.  The "Object" value maps to a function that creates an
    instance of the target CDisk (sub)type.

    Note that this driver model is convoluted.  A CDisk (sub)type instance
    corresponds to both an IDE/ATA controller and an ATA/ATAPI device.

Parameters:
    dwContext - pointer to string containing the registry path to the active key
    of the associated device; the active key contains a key to the device's instance
    key, which stores all of the device's configuration information

Return:
    On success, return handle to device (to identify device); this handle is
    passed to all subsequent DSK_Xxx calls.  Otherwise, return null.

--*/

#define DSKINIT_UNDO_CLS_KEY_ACTIVE 0x1
#define DSKINIT_UNDO_CLS_KEY_DEVICE 0x2

EXTERN_C
DWORD
DSK_Init(
    DWORD dwContext
    )
{
    DWORD dwUndo = 0;                     // undo bitset

    PTSTR szActiveKey = (PTSTR)dwContext; // name of device's active key
    HKEY hActiveKey;                      // handle to device's active key
    PTSTR szDevKey = NULL;                // name of device's instance key
    HKEY hDevKey;                         // handle to device's instance key

    POBJECTFUNCTION pObject = NULL;       // pointer to spawn function

    CPort *pPort = NULL;                  // port
    DWORD dwDeviceId = 0;                 // device ID; 0 => master, 1 => slave

    CDisk *pDisk = NULL;                  // return

    // guard global data; i.e., g_pDiskRoot

    EnterCriticalSection(&g_csMain);

    // open device's active key

    if ((ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szActiveKey, 0, 0, &hActiveKey))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to open device's active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= DSKINIT_UNDO_CLS_KEY_ACTIVE;
    DUMPREGKEY(ZONE_PCI, szActiveKey, hActiveKey);

    // read name of and open device's instance key from device's active key

    if (!(hDevKey = AtaLoadRegKey(hActiveKey, &szDevKey))) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to fetch/open device's instance key from device's active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= DSKINIT_UNDO_CLS_KEY_DEVICE;
    DUMPREGKEY(ZONE_PCI, szDevKey, hDevKey);

    // fetch heap address of port instance from device's instance key

    if (!AtaGetRegistryValue(hDevKey, REG_VALUE_PORT, (PDWORD)&pPort)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to read address of port instance from device's instance key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // Validate pPort value read from the registry
    __try
    {
        // The only check we can really do is a cyclic check as below.
        if (pPort == NULL ||
            (pPort != pPort->m_pController->m_pPort[ATA_PRIMARY] && pPort != pPort->m_pController->m_pPort[ATA_SECONDARY]))
        {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!DSK_Init> Invalid address of port (0x%x) instance read from device's instance key(%s)\r\n"
                ), pPort, szDevKey));
            goto exit;            
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Exception while trying to access the port instance address (0x%x) read from device's instance key(%s)\r\n"
            ), pPort, szDevKey));
        goto exit;
    }

    // fetch device ID from device's instance key; this informs the CDisk
    // instance as to which device (i.e., master/slave) it is

    if (!AtaGetRegistryValue(hDevKey, REG_VAL_DSK_DEVICEID, &dwDeviceId) ||
        dwDeviceId > 1) // DeviceID should 0 or 1
    {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to read device ID device's instance key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // resolve address of spawn function

    pObject = (POBJECTFUNCTION)GetProcAddress(g_hInstance, pPort->m_pController->m_pIdeReg->pszSpawnFunction);
    if (!pObject) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!DSK_Init> Failed to resolve address of device's spawn function(%s)\r\n"
            ), pPort->m_pController->m_pIdeReg->pszSpawnFunction));
        goto exit;
    }

    // instantiate CDisk object

    pDisk = pObject(hDevKey);

    // if successful, write the name of the device's active and instance keys to
    // its CDisk instance, and add the CDisk instance to the IDE/ATA bus driver's
    // list of active disk devices

    if (pDisk) {

        // allocate the sterile, maximal SG_REQ for safe I/O
        pDisk->m_pSterileIoRequest = (PSG_REQ)LocalAlloc(
            LPTR,
            (sizeof(SG_REQ) + ((MAX_SG_BUF) - 1) * sizeof(SG_BUF))
            );

        if (NULL == pDisk->m_pSterileIoRequest) {
            delete pDisk;
            pDisk = NULL;
            goto exit;
        }

        // this information is used for ATA/ATAPI power management

        pDisk->SetActiveKey(szActiveKey);
        pDisk->SetDeviceKey(szDevKey);

        // inform the CDisk instance as to which device it is

        pDisk->m_pPort = pPort;
        pDisk->m_dwDeviceId = dwDeviceId;
        pDisk->m_dwDevice = dwDeviceId;

        // configure register block

        pDisk->ConfigureRegisterBlock(pPort->m_pController->m_pIdeReg->dwRegisterStride);

        // initialize device

        if (!pDisk->Init(hActiveKey)) {
            delete pDisk;
            pDisk = NULL;
            goto exit;
        }

        pDisk->GetDeviceInfo(&pDisk->m_storagedeviceinfo);
        
        // add CDisk instance to IDE/ATA controller's list of active devices

        pDisk->m_pNextDisk = g_pDiskRoot;
        g_pDiskRoot = pDisk;

        DEBUGMSG(ZONE_INIT, (_T(
            "Atapi!DSK_Init> Initialized %s %s on %s\r\n"
            ), ((pPort == pPort->m_pController->m_pPort[ATA_PRIMARY]) ? (_T("PRIMARY")) : (_T("SECONDARY"))),
               ((dwDeviceId == 0) ? (_T("MASTER")) : (_T("SLAVE"))),
               szDevKey
            ));
    }

exit:;

    // clean up
    if (dwUndo & DSKINIT_UNDO_CLS_KEY_ACTIVE) {
        RegCloseKey(hActiveKey);
    }
    if (NULL == pDisk) {
        if (dwUndo & DSKINIT_UNDO_CLS_KEY_DEVICE) {
            RegCloseKey(hDevKey);
        }
        // pPort is deleted in IDE_Deinit
    }
    if (szDevKey) {
        LocalFree(szDevKey);
    }

    LeaveCriticalSection(&g_csMain);

    return (DWORD)pDisk;
}

/*++

DSK_Deinit
    This function deallocates the associated CDisk instance.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)

Return:
    This function always succeeds.

--*/
EXTERN_C
BOOL
DSK_Deinit(
    DWORD dwHandle
    )
{
    CDisk *pDiskPrev = NULL;
    CDisk *pDiskCur = g_pDiskRoot;

    EnterCriticalSection(&g_csMain);

    // find the CDisk instance in global CDisk list

    while (pDiskCur) {
        if (pDiskCur == (CDisk *)dwHandle) {
            break;
        }
        pDiskPrev = pDiskCur;
        pDiskCur = pDiskCur->m_pNextDisk;
    }

    // remove CDisk instance from global CDisk list

    if (pDiskCur) {
        if (pDiskPrev) {
            pDiskPrev = pDiskCur->m_pNextDisk;
        }
        else {
            g_pDiskRoot = pDiskCur->m_pNextDisk;
        }
        delete pDiskCur;
    }

    LeaveCriticalSection(&g_csMain);

    return TRUE;
}

/*++

DSK_Open
    This function opens a CDisk instance for use.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)
    dwAccess - specifes how the caller would like too use the device (read
    and/or write) [this argument is ignored]
    dwShareMode - specifies how the caller would like this device to be shared
    [this argument is ignored]

Return:
    On success, return handle to "open" CDisk instance; this handle is the
    same as dwHandle.  Otherwise, return null.

--*/
EXTERN_C
DWORD
DSK_Open(
    HANDLE dwHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    CDisk *pDisk = (CDisk *)dwHandle;

    EnterCriticalSection(&g_csMain);

    // validate the CDisk instance

    if (!AtaIsValidDisk(pDisk)) {
        pDisk = NULL;
    }

    LeaveCriticalSection(&g_csMain);

    // if the CDisk instance is valid, then open; open just increments the
    // instance's open count

    if (pDisk) {
        pDisk->Open();
    }

    return (DWORD)pDisk;
}

/*++

DSK_Close
    This function closes a CDisk instance.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
BOOL
DSK_Close(
    DWORD dwHandle
    )
{
    CDisk *pDisk = (CDisk *)dwHandle;

    EnterCriticalSection(&g_csMain);

    // validate the CDisk instance

    if (!AtaIsValidDisk(pDisk)) {
        pDisk = NULL;
    }

    LeaveCriticalSection(&g_csMain);

    // if CDisk instance is valid, then close; close just decrements the
    // instance's open count

    if (pDisk) {
        pDisk->Close();
    }

    return (pDisk != NULL);
}

/*++

DSK_IOControl
    This function processes an IOCTL_DISK_Xxx/DISK_IOCTL_Xxx I/O control.

Parameters:
    dwHandle - pointer to associated CDisk instance (initially returned by
    DSK_Init)
    dwIOControlCode - I/O control to perform
    pInBuf - pointer to buffer containing the input data of the I/O control
    nInBufSize - size of pInBuf (bytes)
    pOutBuf - pointer to buffer that is to receive the output data of the
    I/O control
    nOutBufSize - size of pOutBuf (bytes)
    pBytesReturned - pointer to DWORD that is to receive the size (bytes) of the
    output data of the I/O control
    pOverlapped - ignored

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
BOOL
DSK_IOControl(
    DWORD dwHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned,
    PDWORD pOverlapped)
{
    CDisk *pDisk = (CDisk *)dwHandle;
    DWORD SafeBytesReturned = 0;
    BOOL fRet = FALSE;

    // Allow only kernel mode callers
    if (GetDirectCallerProcessId() != GetCurrentProcessId()){
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection(&g_csMain);

    // validate CDisk instance

    if (!AtaIsValidDisk(pDisk)) {
        pDisk = NULL;
    }

    LeaveCriticalSection(&g_csMain);

    if (!pDisk) {
        return FALSE;
    }

    // DISK_IOCTL_INITIALIZED is a deprecated IOCTL; what does PostInit do?

    if (dwIoControlCode == DISK_IOCTL_INITIALIZED) {
        fRet = pDisk->PostInit((PPOST_INIT_BUF)pInBuf);
    }
    else {

        IOREQ IOReq;

        // build I/O request structure

        memset(&IOReq, 0, sizeof(IOReq));
        IOReq.dwCode = dwIoControlCode;
        IOReq.pInBuf = pInBuf;
        IOReq.dwInBufSize = nInBufSize;
        IOReq.pOutBuf = pOutBuf;
        IOReq.dwOutBufSize = nOutBufSize;
        IOReq.pBytesReturned = &SafeBytesReturned;
        IOReq.hProcess = GetCallerProcess();

        // perform I/O control

        __try {

            fRet = pDisk->PerformIoctl(&IOReq);

            // if the caller supplied pBytesReturned, then write the value
            // from our safe copy

            if (pBytesReturned) {
                *pBytesReturned = SafeBytesReturned;
            }

        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fRet = FALSE;
            SetLastError(ERROR_GEN_FAILURE);
        }
    }

    return fRet;
}

/*++

DSK_PowerUp
    This function resumes the device.

Parameters:
    None

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
VOID
DSK_PowerUp(
    VOID
    )
{
    EnterCriticalSection(&g_csMain);

    CDisk *pDisk = g_pDiskRoot;

    // iterate through the global CDisk list and direct each CDisk instance to
    // power up its associated device

    while (pDisk) {
        pDisk->PowerUp();
        pDisk = pDisk->m_pNextDisk;
    }

    LeaveCriticalSection(&g_csMain);
}

/*++

DSK_PowerDown
    This function suspends a device.

Parameters:
    None

Return:
    On success, return true.  Otherwise, return false.

--*/
EXTERN_C
VOID
DSK_PowerDown(
    VOID
    )
{
    EnterCriticalSection(&g_csMain);

    CDisk *pDisk = g_pDiskRoot;

    // iterate through the global CDisk list and direct each CDist instance to
    // power down its associated device

    while (pDisk) {
        pDisk->PowerDown();
        pDisk = pDisk->m_pNextDisk;
    }

    LeaveCriticalSection(&g_csMain);
}

/*++

IDE_Init
    This function is called as a result of a bus driver enumerating an IDE/ATA
    controller.

    Each IDE/ATA controller is a "bus".  An IDE/ATA controller contains at most
    two channels, and each channel can contain a master and a slave device.
    An IDE/ATA controller's instance key will typically contain the following
    subkeys: Device0, Device1, Device2, and Device3.  Device0 is the master
    device on the primary channel.  Device1 is the slave device on the primary
    channel.  Device2 is the master device on the secondary channel.  Device3
    is the slave device on the secondary.

    This function is responsible for searching the driver's instance key for
    DeviceX subkeys and calling ActivateDevice on each DeviceX subkey found.
    The call to ActivateDevice will eventually enter DSK_Init.  DSK_Init is
    responsible for creating a CDisk instance to associate with a device.  If a
    device is present and intialization succeeds, then DSK_Init will succeed.

Parameters:
    dwContext - pointer to string containing the registry path to the active key
    of the IDE/ATA controller; the active key contains a key to the IDE/ATA
    controller's instance key, which stores all of the IDE/ATA controller's
    configuration information

Return:
    On success, return handle to IDE/ATA controller (for identification); this
    handle is passed to all subsequent IDE_Xxx calls.  Otherwise, return null.

--*/

#define IDEINIT_UNDO_CLS_KEY_ACTIVE 0x01
#define IDEINIT_UNDO_CLS_KEY_DEVICE 0x02
#define IDEINIT_UNDO_DEL_BUS        0x04
#define IDEINIT_UNDO_DEL_PORT_PRI   0x08
#define IDEINIT_UNDO_DEL_PORT_SEC   0x10
#define IDEINIT_UNDO_DEL_REG_IDE    0x20
#define IDEINIT_UNDO_DEL_REG_DSK    0x40

EXTERN_C
DWORD
IDE_Init(
    DWORD dwContext
    )
{
    DWORD    dwUndo = 0;                     // undo bitset
    PTSTR    szActiveKey = (PTSTR)dwContext; // name of IDE/ATA controller's active key
    HKEY     hActiveKey;                     // handle to IDE/ATA controller's active key
    PTSTR    szDevKey = NULL;                // name of IDE/ATA controller's instance key
    HKEY     hDevKey;                        // handle to IDE/ATA controller's instance key
    PDSKREG  pDskReg = NULL;                // ATA/ATAPI device's registry value set
    CIDEBUS *pBus = NULL;                    // return

    // open the IDE/ATA controllers's active key

    if ((ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szActiveKey, 0, 0, &hActiveKey)) || (!hActiveKey)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to open IDE/ATA controller's active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_CLS_KEY_ACTIVE;
    DUMPREGKEY(ZONE_PCI, szActiveKey, hActiveKey);

    // fetch the name of the IDE/ATA controller's instance key and open it

    if (!(hDevKey = AtaLoadRegKey(hActiveKey, &szDevKey)) || !szDevKey) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to fetch/open IDE/ATA controller's instance key from active key(%s)\r\n"
            ), szActiveKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_CLS_KEY_DEVICE;
    DUMPREGKEY(ZONE_PCI, szDevKey, hDevKey);

    // instantiate an IDE/ATA controller ("bus") object

    if (!(pBus = new CIDEBUS)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to instantiate IDE/ATA controller bus object; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_DEL_BUS;

    // instantiate primary channel port object

    pBus->m_pPort[ATA_PRIMARY] = new CPort(pBus);
    if (!pBus->m_pPort[ATA_PRIMARY]) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to allocate port for primary channel; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_DEL_PORT_PRI;

    // instantiate secondary channel port object

    pBus->m_pPort[ATA_SECONDARY] = new CPort(pBus);
    if (!pBus->m_pPort[ATA_SECONDARY]) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to allocate port for secondary channel; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_DEL_PORT_SEC;

    // configure port instances based on I/O window information in registry

    if (!GetIoPort(hDevKey, szDevKey, pBus)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Bad I/O window information; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // fetch IDE/ATA controller registry value set (i.e., registry configuration)

    pBus->m_pIdeReg = (PIDEREG)LocalAlloc(LPTR, sizeof(IDEREG));
    if (!pBus->m_pIdeReg) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to allocate IDE_ registry value set; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }
    dwUndo |= IDEINIT_UNDO_DEL_REG_IDE;
    if (!GetIDERegistryValueSet(hDevKey, pBus->m_pIdeReg)) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!IDE_Init> Failed to read IDE_ registry value set from registry; device key(%s)\r\n"
            ), szDevKey));
        goto exit;
    }

    // assign IRQs

    pBus->m_pPort[ATA_PRIMARY]->m_dwIrq = pBus->m_pIdeReg->dwIrq;
    pBus->m_pPort[ATA_SECONDARY]->m_dwIrq = pBus->m_pIdeReg->dwIrq;

    // if IDE/ATA controller is legacy ISA/PCI, then IRQ of secondary channel is
    // (IRQ of primary channel + 1); otherwise, the primary and secondary
    // channels must share an interrupt and employ an ISR

    if (pBus->m_pIdeReg->dwLegacy) {
        pBus->m_pPort[ATA_SECONDARY]->m_dwIrq =pBus->m_pPort[ATA_PRIMARY]->m_dwIrq + 1;
    }

    // no SysIntr provided; we have to map IRQ to SysIntr ourselves; note that,
    // even if the primary and secondary channels share an IRQ, each channel is
    // required to have its own IRQ-SysIntr mapping

    if (!pBus->m_pIdeReg->dwSysIntr) {

        DWORD dwReturned = 0;
        UINT32 primaryIrqAndFlags[3];

        primaryIrqAndFlags[0] = -1;
        primaryIrqAndFlags[1] = 0x00000008; // translate statically mapped sysintr instead of requesting a new one
        primaryIrqAndFlags[2] = pBus->m_pPort[ATA_PRIMARY]->m_dwIrq;
        
        if (!KernelIoControl(
            IOCTL_HAL_REQUEST_SYSINTR,
            (LPVOID)primaryIrqAndFlags, sizeof(primaryIrqAndFlags),
            (LPVOID)&pBus->m_pPort[ATA_PRIMARY]->m_dwSysIntr, sizeof(DWORD),
            &dwReturned
        )) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to map IRQ(%d) to SysIntr for primary channel of device(%s)\r\n"
                ), pBus->m_pPort[ATA_PRIMARY]->m_dwIrq, szDevKey));
            goto exit;
        }

        // even if primary and secondary channels use the same IRQ, we need two
        // separate SysIntr mappings

        if (pBus->m_pPort[ATA_SECONDARY]->m_fInitialized) {

            UINT32 secondaryIrqAndFlags[3];
            secondaryIrqAndFlags[0] = -1;
            secondaryIrqAndFlags[1] = 0x00000008; // translate statically mapped sysintr instead of requesting a new one
            secondaryIrqAndFlags[2] = pBus->m_pPort[ATA_SECONDARY]->m_dwIrq;

            if (!KernelIoControl(
                IOCTL_HAL_REQUEST_SYSINTR,
                (LPVOID)secondaryIrqAndFlags, sizeof(secondaryIrqAndFlags),
                (LPVOID)&pBus->m_pPort[ATA_SECONDARY]->m_dwSysIntr, sizeof(DWORD),
                &dwReturned
            )) {               
                DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                    "Atapi!IDE_Init> Failed to map IRQ(%d) to SysIntr for secondary channel on device(%s)\r\n"
                    ), pBus->m_pPort[ATA_SECONDARY]->m_dwIrq, szDevKey));
                goto exit;
            }
        }
    }
    else {

        DWORD dwReturned = 0;

        // the SysIntr corresponds to the primary channel; we need to request
        // a separate SysIntr for the secondary channel

        pBus->m_pPort[ATA_PRIMARY]->m_dwSysIntr = pBus->m_pIdeReg->dwSysIntr;

        if (pBus->m_pPort[ATA_SECONDARY]->m_fInitialized) {

            UINT32 secondaryIrqAndFlags[3];
            secondaryIrqAndFlags[0] = -1;
            secondaryIrqAndFlags[1] = 0x00000008; // translate statically mapped sysintr instead of requesting a new one
            secondaryIrqAndFlags[2] = pBus->m_pPort[ATA_SECONDARY]->m_dwIrq;

            if (!KernelIoControl(
                IOCTL_HAL_REQUEST_SYSINTR,
                (LPVOID)secondaryIrqAndFlags, sizeof(secondaryIrqAndFlags),
                (LPVOID)&pBus->m_pPort[ATA_SECONDARY]->m_dwSysIntr, sizeof(DWORD),
                &dwReturned
            )) {
                DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                    "Atapi!IDE_Init> Failed to map IRQ(%d) to SysIntr for secondary channel on device(%s)\r\n"
                    ), pBus->m_pPort[ATA_SECONDARY]->m_dwIrq, szDevKey));
                goto exit;
            }
        }
    }

    // if this IDE/ATA controller only has a single channel, e.g., PC Card,
    // then destroy the secondary channel's port

    DEBUGCHK(pBus->m_pPort[ATA_PRIMARY]->m_fInitialized);
    if (!pBus->m_pPort[ATA_SECONDARY]->m_fInitialized) {
        delete pBus->m_pPort[ATA_SECONDARY];
        pBus->m_pPort[ATA_SECONDARY] = NULL;
        dwUndo &= ~IDEINIT_UNDO_DEL_PORT_SEC;
    }

    // IDE/ATA "bus" enumeration; scan the current IDE/ATA controller's instance
    // key for DeviceX subkeys

    DEBUGMSG(ZONE_INIT, (_T(
        "Atapi!IDE_Init> Start of IDE/ATA device enumeration\r\n"
        )));

    DWORD dwIndex = 0;        // index of next DeviceX subkey to fetch/enumerate
    HKEY hKey;                // handle to DeviceX subkey
    TCHAR szNewKey[MAX_PATH]; // name of DeviceX subkey
    DWORD dwNewKeySize;       // size of name of DeviceX subkey
    DWORD dwDeviceId;         // "DeviceId" read from DeviceX subkey and resolved to 0, 1

    dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));

    while (
        ERROR_SUCCESS == RegEnumKeyEx(
            hDevKey,       // IDE/ATA controller's instance key
            dwIndex,       // index of the subkey to fetch
            szNewKey,      // name of subkey (e.g., "Device0")
            &dwNewKeySize, // size of name of subkey
            NULL,          // lpReserved; set to NULL
            NULL,          // lpClass; not required
            NULL,          // lpcbClass; lpClass is NULL; hence, NULL
            NULL           // lpftLastWriteTime; set to NULL
    )) {

        dwIndex += 1;
        dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));
        pDskReg = NULL;

        // open the DeviceX subkey; copy configuration information from the
        // IDE/ATA controller's instance key to the device's DeviceX key

        if (ERROR_SUCCESS != RegOpenKeyEx(hDevKey, szNewKey, 0, 0, &hKey)) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to open DeviceX subkey; device key(%s)\r\n"
                ), szDevKey));
            goto exit;
        }
        if (
            (NULL == pBus->m_pPort[ATA_SECONDARY]) &&
            ((0 == wcscmp(szNewKey, REG_KEY_SECONDARY_MASTER)) || (0 == wcscmp(szNewKey, REG_KEY_SECONDARY_SLAVE)))
        ) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Secondary channel does not exist, but Device2 and Device3 subkeys exist in %s; ignoring\r\n"
                ), szDevKey));
            dwIndex -= 1;
            continue;
        }
        if (
            (0 != wcscmp(szNewKey, REG_KEY_PRIMARY_MASTER)) && (0 != wcscmp(szNewKey, REG_KEY_PRIMARY_SLAVE)) &&
            (0 != wcscmp(szNewKey, REG_KEY_SECONDARY_MASTER)) && (0 != wcscmp(szNewKey, REG_KEY_SECONDARY_SLAVE))
        ) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Found bad DeviceX subkey(%s) in device's(%s) key; ignoring\r\n"
                ), szNewKey, szDevKey));
            dwIndex -= 1;
            continue;
        }

        // fetch the device's registry value set

        pDskReg = (PDSKREG)LocalAlloc(LPTR, sizeof(DSKREG));
        if (!pDskReg) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to allocate DSK_ registry value set; device key(%s)\r\n"
                ), szNewKey));
            goto exit;
        }
        dwUndo |= IDEINIT_UNDO_DEL_REG_DSK;
        if (!GetDSKRegistryValueSet(hKey, pDskReg)) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to read DSK_ registry value set from registry; device key(%s)\r\n"
                ), szNewKey));
            goto exit;
        }

        // resolve DeviceX subkey's "DeviceId" to (0, 1), so a CDisk instance can
        // reference the correct m_pBus->m_p(Primary, Secondary)Port->(m_pDisk, m_pDskReg)
        // array element

        dwDeviceId = pDskReg->dwDeviceId; // store the original value
        pDskReg->dwDeviceId &= 0x01;

        // write the new device ID value back to the device's instance key

        if (!AtaSetRegistryValue(hKey, REG_VAL_DSK_DEVICEID, pDskReg->dwDeviceId)) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!IDE_Init> Failed to write %s(%d) DSK_ registry value to device's instance key(%s)\r\n"
                ), REG_VAL_DSK_DEVICEID, dwDeviceId, szNewKey));
            goto exit;
        }

        // the master and slave CDisk instances of a particular channel have to
        // share the port instance associated with the channel; write the heap
        // address of the port instance to the device's instance key

        if ((0 == wcscmp(szNewKey, REG_KEY_PRIMARY_MASTER)) || (0 == wcscmp(szNewKey, REG_KEY_PRIMARY_SLAVE))) {

            // store the DSK_ register value set of the master/slave device in
            // the appropriate slot of the port instance

            pBus->m_pPort[ATA_PRIMARY]->m_pDskReg[pDskReg->dwDeviceId] = pDskReg;

            if (!AtaSetRegistryValue(hKey, REG_VALUE_PORT, (DWORD)pBus->m_pPort[ATA_PRIMARY])) {
                DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                    "Atapi!IDE_Init> Failed to write address of primary port instance to device's(%s) DeviceX subkey(%s)\r\n"
                    ), szDevKey, szNewKey));
                goto exit;
            }

        }
        else if ((0 == wcscmp(szNewKey, REG_KEY_SECONDARY_MASTER)) || (0 == wcscmp(szNewKey, REG_KEY_SECONDARY_SLAVE))) {

            // store the DSK_ register value set of the master/slave device in
            // the appropriate slot of the port instance

            pBus->m_pPort[ATA_SECONDARY]->m_pDskReg[pDskReg->dwDeviceId] = pDskReg;

            if (!AtaSetRegistryValue(hKey, REG_VALUE_PORT, (DWORD)pBus->m_pPort[ATA_SECONDARY])) {
                DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                    "Atapi!IDE_Init> Failed to write address of secondary port instance to device's(%s) DeviceX subkey(%s)\r\n"
                    ), szDevKey, szNewKey));
                goto exit;
            }

        }

        if (!pBus->m_szDevice[dwDeviceId]) {

            // save name of device's full registry key path; when we've finished
            // enumerating the "bus", we'll call ActivateDevice against all of
            // these paths

            pBus->m_szDevice[dwDeviceId] = new TCHAR[wcslen(szDevKey) + wcslen(szNewKey) + 10];
            wcscpy(pBus->m_szDevice[dwDeviceId], szDevKey);
            wcscat(pBus->m_szDevice[dwDeviceId], L"\\");
            wcscat(pBus->m_szDevice[dwDeviceId], szNewKey);

            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!IDE_Init> Enumerated IDE/ATA device %s\r\n"
                ), pBus->m_szDevice[dwDeviceId]));
        }

    } // while

    DEBUGMSG(ZONE_INIT, (_T(
        "Atapi!IDE_Init> End of IDE/ATA device enumeration\r\n"
        )));

    // initialize enumerated devices; it's imperative that we activate the
    // channel master before the channel slave

    for (dwDeviceId = 0; dwDeviceId < 4; dwDeviceId += 1) {
        if (pBus->m_szDevice[dwDeviceId]) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!IDE_Init> Activating IDE/ATA device %s\r\n"
                ), pBus->m_szDevice[dwDeviceId]));
            pBus->m_hDevice[dwDeviceId] = ActivateDeviceEx(pBus->m_szDevice[dwDeviceId], NULL, 0, NULL);
        }
    }

    dwUndo &= ~IDEINIT_UNDO_DEL_BUS;
    dwUndo &= ~IDEINIT_UNDO_DEL_PORT_PRI;
    dwUndo &= ~IDEINIT_UNDO_DEL_PORT_SEC;

exit:

    if (dwUndo & IDEINIT_UNDO_CLS_KEY_ACTIVE) {
        RegCloseKey(hActiveKey);
    }
    if (dwUndo & IDEINIT_UNDO_CLS_KEY_DEVICE) {
        RegCloseKey(hDevKey);
    }
    if (szDevKey) {
        LocalFree(szDevKey);
    }
    if ((NULL != pBus) && (dwUndo & IDEINIT_UNDO_DEL_BUS)) {
        delete pBus;
        pBus = NULL;
    }



    return (DWORD)pBus;
}

/*++

IDE_Deinit
    This function deallocates the associated IDE/ATA controller ("bus") instance.

Parameters:
    dwHandle - pointer to associated bus instance (initially returned by
    IDE_Init)

Return:
    This function always succeeds.

--*/
EXTERN_C
BOOL
IDE_Deinit(
    DWORD dwHandle
    )
{   
    CIDEBUS *pBus = (CIDEBUS *)dwHandle;

    DEBUGCHK(pBus != NULL);
    delete pBus;

    return TRUE;
}


/*++

IDE_Open
    This function is not supported.

Parameters:
    N/A

Return:
    This function always fails.

--*/
EXTERN_C
DWORD
IDE_Open(
    HANDLE dwHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}


/*++

IDE_Close
    This function is not supported.

Parameters:
    N/A

Return:
    This function always fails.

--*/
EXTERN_C
BOOL
IDE_Close(
    DWORD dwHandle
    )
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

/*++

IDE_IOControl
    This function is not supported.

Parameters:
    N/A

Return:
    This function always fails.

--*/
EXTERN_C
BOOL
IDE_IOControl(
    DWORD dwHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned,
    PDWORD pOverlapped
    )
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}



/*++

DllMain
    This function is the main ATAPI.DLL entry point.

Parameters:
    hInstance - a handle to the dll; this value is the base address of the DLL
    dwReason - the reason for the DLL is being entered
    lpReserved - not used

Return:
    On success, return true.  Otherwise, return false.

--*/
BOOL
WINAPI
DllMain(
    HANDLE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:

        // initialize global data
        g_hInstance = (HINSTANCE)hInstance;
        InitializeCriticalSection(&g_csMain);
        // register debug zones
        RegisterDbgZones((HMODULE)hInstance, &dpCurSettings);
        DisableThreadLibraryCalls((HMODULE)hInstance);
        DEBUGMSG(ZONE_INIT, (_T("ATAPI DLL_PROCESS_ATTACH\r\n")));

        break;

    case DLL_PROCESS_DETACH:

        // deinitialize global data
        DeleteCriticalSection(&g_csMain);
        DEBUGMSG(ZONE_INIT, (TEXT("ATAPI DLL_PROCESS_DETACH\r\n")));

        break;
    }

    return TRUE;
}

