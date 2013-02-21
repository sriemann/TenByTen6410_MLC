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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
#include <diskmain.h>

#define dim(x)      (sizeof(x) / sizeof(x[0]))
 
static BOOL 
HexStringToDword(LPCWSTR FAR& lpsz, DWORD FAR& Value,
                 int cDigits, WCHAR chDelim)
{
    int Count;
    
    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }
    
    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}


static BOOL 
wUUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;
    
    if (!HexStringToDword(lpsz, pguid->Data1, sizeof(DWORD)*2, '-'))
        return FALSE;
    
    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;
    
    pguid->Data2 = (WORD)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;
    
    pguid->Data3 = (WORD)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;
    
    pguid->Data4[0] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
        return FALSE;
    
    pguid->Data4[1] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;
    
    pguid->Data4[2] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;
    
    pguid->Data4[3] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;
    
    pguid->Data4[4] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;
    
    pguid->Data4[5] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;
    
    pguid->Data4[6] = (BYTE)dw;
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;
    
    pguid->Data4[7] = (BYTE)dw;
    
    return TRUE;
}

static BOOL 
GUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    if (*lpsz++ != '{' )
        return FALSE;
    
    if(wUUIDFromString(lpsz, pguid) != TRUE)
        return FALSE;
    
    lpsz +=36;
    
    if (*lpsz++ != '}' )
        return FALSE;
    
    return TRUE;
}


CDiskPower::CDiskPower(void) :
        m_curDx(D0),
        m_timeoutDx(D2),
        m_fBoostRequested(FALSE),
        m_fReductionRequested(FALSE),
        m_dwPowerTimeout(1000),
        m_htPower(NULL),
        m_hevPowerSignal(NULL),
        m_fShutdownPowerThread(FALSE),
        m_PowerThreadPriority(250),     // THREAD_PRIORITY_ABOVE_NORMAL
        m_pDisk(NULL),
        m_pszPMName(NULL),
        m_UseCount(0),
        m_pfnDevicePowerNotify(NULL)
{
    memset(&m_dxInfo, 0, sizeof(m_dxInfo));
    m_dwStartTickCount = GetTickCount();
    QueryPerformanceCounter(&m_startQPC);
    m_dxInfo[m_curDx].dwCount++;
    InitializeCriticalSection(&m_csPower);
}

CDiskPower::~CDiskPower(void)
{
    if(m_htPower) {
        DEBUGCHK(m_hevPowerSignal);
        m_fShutdownPowerThread = TRUE;
        SignalActivity();
        WaitForSingleObject(m_htPower, INFINITE);
        CloseHandle(m_htPower);
    }
    if(m_hevPowerSignal) {
        CloseHandle(m_hevPowerSignal);
    }
    if(m_hevDummy) {
        CloseHandle(m_hevDummy);
    }
    if(m_pszPMName) {
        LocalFree((LPWSTR) m_pszPMName);    // cast to remove const
    }
        
    DeleteCriticalSection(&m_csPower);
}

// Note: the caller should own the parent disk device's critical section
void CDiskPower::TakeCS(void) 
{ 
    EnterCriticalSection(&m_csPower); 
}

void CDiskPower::ReleaseCS(void) 
{ 
    LeaveCriticalSection(&m_csPower); 
}

// this routine wakes the power timeout thread
void CDiskPower::SignalActivity(void) 
{
    DEBUGCHK(m_hevPowerSignal);
    DEBUGCHK(m_htPower);
    SetEvent(m_hevPowerSignal);
}

// This routine marks the disk as in use.  If it is powered down,
// it spins it up and waits for it to become ready.  The caller
// must hold the disk critical section.
BOOL CDiskPower::RequestDevice(void)
{
    BOOL fOk = TRUE;
    PREFAST_DEBUGCHK(m_pfnDevicePowerNotify != NULL);

    TakeCS();

    // is the disk powered up?
    if(m_curDx != D0) {
        // don't bother requesting from the PM if we've already asked in a previous request
        DEBUGMSG(ZONE_POWER, (_T("CDiskPower::RequestDevice: device at D%d, m_fBoostRequested is %d\r\n"), m_curDx, m_fBoostRequested));
        if(!m_fBoostRequested) {
            // request that the PM make us available
            m_fBoostRequested = TRUE;
            DWORD dwStatus = m_pfnDevicePowerNotify((PVOID) m_pszPMName, D0, POWER_NAME);
            if(dwStatus != ERROR_SUCCESS) {
                DEBUGMSG(ZONE_WARNING, (_T("CDiskPower::RequestDevice: DevicePowerNotify('%s') failed %d\r\n"), m_pszPMName, dwStatus));
                m_fBoostRequested = FALSE;
                fOk = FALSE;
            }
        }
    }

    if(m_curDx == D0) {
        // wait for the disk to spin up so that we can do I/O
        DEBUGCHK(m_UseCount == 0);
        m_UseCount++;
    } else {
        fOk = FALSE;
    }

    ReleaseCS();
    
    return fOk;
}

// This API signals that the caller is done doing I/O.  The caller must hold the 
// parent disk's critical section.
void CDiskPower::ReleaseDevice(void)
{
    PREFAST_DEBUGCHK(m_pfnDevicePowerNotify);
    
    TakeCS();

    // update the usage counter
    DEBUGCHK(m_UseCount != 0);
    m_UseCount--;
    DEBUGCHK(m_UseCount == 0);

    // wake the timeout thread to restart its countdown
    SignalActivity();
    if(m_fReductionRequested) {
        // cancel outstanding requests to spin down the disk
        DWORD dwStatus = m_pfnDevicePowerNotify((PVOID) m_pszPMName, D0, POWER_NAME);
        if(dwStatus != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_WARNING, (_T("CDiskPower::DiskPowerThread: DevicePowerNotify('%s', D%d) failed %d\r\n"), m_pszPMName, D0, dwStatus));
        } else {
            m_fReductionRequested = FALSE;
        }
    }    
    ReleaseCS();
}

DWORD CDiskPower::DiskPowerThread(void)
{
    BOOL fDone = FALSE;
    DWORD dwTimeout = m_dwPowerTimeout;
    HANDLE hev = m_hevPowerSignal;

    PREFAST_DEBUGCHK(m_pfnDevicePowerNotify != NULL);
    DEBUGCHK(m_hevPowerSignal);
    DEBUGCHK(m_hevDummy);

    DEBUGMSG(ZONE_INIT, (_T("CDiskPower::DiskPowerThread: starting up for '%s', timeout is %d ms\r\n"), m_pszPMName, m_dwPowerTimeout));

    while(!fDone) {
        //DEBUGMSG(ZONE_POWER, (_T("CDiskPower::DiskPowerThread: waiting on '%s', timeout is %u\r\n"), m_pszPMName, dwTimeout));
        DWORD dwStatus = WaitForSingleObject(hev, dwTimeout);
        //DEBUGMSG(ZONE_POWER, (_T("CDiskPower::DiskPowerThread: WaitForSingleObject() returned %u\r\n"), dwStatus));
        switch(dwStatus) {
        case WAIT_OBJECT_0:
            // are we supposed to exit?
            if(m_fShutdownPowerThread) {
                DEBUGMSG(ZONE_INIT, (_T("CDiskPower::DiskPowerThread: shutdown event signaled\r\n")));
                fDone = TRUE;
            } else {
                // ignore further activity until the timeout expires
                TakeCS();               // Note: if you take the disk cs here, take it first
                DEBUGMSG(ZONE_POWER, (_T("CDiskPower::DiskPowerThread: disk activity detected on '%s', use count is %d\r\n"), m_pszPMName, m_UseCount));
                DEBUGCHK(hev != m_hevDummy);
                hev = m_hevDummy;
                dwTimeout = m_dwPowerTimeout;
                ReleaseCS();
            }
            break;
        case WAIT_TIMEOUT:
            // inactivity timeout -- see if we should spin down the disk
            m_pDisk->TakeCS();
            TakeCS();

            // we should be the only thread in the driver at this point
            DEBUGCHK(m_UseCount == 0);

            // By the time we have acquired these critical sections, we may have seen
            // some disk activity from an I/O thread that held them previously.  Check
            // for this by polling our timeout event.
            if(WaitForSingleObject(m_hevPowerSignal, 0) == WAIT_TIMEOUT) {
                // don't bother asking the PM if we've already requested to spin down
                DEBUGMSG(ZONE_POWER, (_T("CDiskPower::DiskPowerThread: no disk activity on '%s', m_fReductionRequested is %d\r\n"), m_pszPMName, m_fReductionRequested));
                if(!m_fReductionRequested) {
                    // spin down the disk to m_timeoutDx
                    m_fReductionRequested = TRUE;
                    dwStatus = m_pfnDevicePowerNotify((PVOID) m_pszPMName, m_timeoutDx, POWER_NAME);
                    if(dwStatus != ERROR_SUCCESS) {
                        DEBUGMSG(ZONE_WARNING, (_T("CDiskPower::DiskPowerThread: DevicePowerNotify('%s', D%d) failed %d\r\n"), m_pszPMName, m_timeoutDx, dwStatus));
                        m_fReductionRequested = FALSE;
                    }
                }

                // no need for more timeouts until the disk spins up again
                hev = m_hevPowerSignal;
                dwTimeout = INFINITE;
            } else {
                DEBUGMSG(ZONE_POWER, (_T("CDiskPower::DiskPowerThread: activity on '%s' after timeout, device at D%d\r\n"), m_pszPMName, m_curDx));
                DEBUGCHK(hev == m_hevDummy);

                // if we are already at or below the spin-down disk power state we don't need
                // to have a timeout.  The comparison relies on the fact that D0 >= Dx >= D4.
                if(m_curDx < m_timeoutDx) {
                    dwTimeout = m_dwPowerTimeout;
                } else {
                    dwTimeout = INFINITE;
                }

                // if we are not spun up, allow disk activity to wake us up
                if(m_curDx != D0) {
                    hev = m_hevPowerSignal;
                }
            }

            // release resources
            ReleaseCS();
            m_pDisk->ReleaseCS();
            break;
        default:
            DEBUGMSG(ZONE_WARNING, (_T("CDiskPower::DiskPowerThread: WaitForSingleObject() returned %d, error %d\r\n"), dwStatus, GetLastError()));
            break;
        }
    }

    DEBUGMSG(ZONE_INIT, (_T("CDiskPower::DiskPowerThread: all done\r\n")));
    return 0;
}

DWORD CDiskPower::DiskPowerThreadStub(LPVOID lpvParam)
{
    PREFAST_DEBUGCHK(lpvParam != NULL);
    CDiskPower *pDiskPower = (CDiskPower *) lpvParam;
    DWORD dwStatus = pDiskPower->DiskPowerThread();
    return dwStatus;
}

// This routine is called during IOCTL_POWER_CAPABILITIES processing.  It 
// returns TRUE if successful and FALSE if not.  The caller is expected to
// destroy the CDiskPower object if this routine fails.
BOOL CDiskPower::Init(CDisk *pDiskParent)
{
    DWORD dwStatus;
    GUID gPMClass;
    int nPriority = 250;    // THREAD_PRIORITY_ABOVE_NORMAL
    HANDLE hActive = NULL;
    BOOL fOk = TRUE;

    PREFAST_DEBUGCHK(pDiskParent != NULL);
    DEBUGMSG(ZONE_INIT, (_T("+CDiskPower::Init(): parent is 0x%08x\r\n"), pDiskParent));

    // record the parent device
    m_pDisk = pDiskParent;

    // get a pointer to the PM APIs we need
    if(fOk) {
        HMODULE hmCoreDll = LoadLibrary(L"coredll.dll");
        if(hmCoreDll == NULL) {
            DEBUGMSG(ZONE_INIT || ZONE_ERROR, (_T("CDevicePower::Init: LoadLibrary('coredll.dll') failed %d\r\n"), GetLastError()));
            fOk = FALSE;
        } else {
            m_pfnDevicePowerNotify = (DWORD ((*)(PVOID, CEDEVICE_POWER_STATE, DWORD))) GetProcAddress(hmCoreDll, L"DevicePowerNotify");
            if(m_pfnDevicePowerNotify == NULL) {
                DEBUGMSG(ZONE_INIT || ZONE_ERROR, (_T("CDevicePower::Init: GetProcAddress('DevicePowerNotify') failed %d\r\n"), GetLastError()));
                fOk = FALSE;
            }
            // we're explicitly linked with coredll so we don't need the handle
            FreeLibrary(hmCoreDll);
        }
    }

    // read registry configuration
    if(fOk) {
        HKEY hk;
        BOOL fGotClass = FALSE;
        WCHAR szClass[64] = {0}; // big enough for a GUID

        // determine the power class we are advertising
        dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_pDisk->m_szDeviceKey, 0, 0, &hk);
        if(dwStatus == ERROR_SUCCESS) {
            // read the PM class
            DWORD dwSize = sizeof(szClass);
            dwStatus = RegQueryValueEx(hk, L"PowerClass", NULL, NULL, (LPBYTE) szClass, &dwSize);
            if(dwStatus == ERROR_SUCCESS) {
                fGotClass = TRUE;
            }

            // get the inactivity timeout
            DWORD dwValue;
            dwSize = sizeof(dwValue);
            dwStatus = RegQueryValueEx(hk, L"InactivityTimeout", NULL, NULL, (LPBYTE) &dwValue, &dwSize);
            if(dwStatus == ERROR_SUCCESS) {
                m_dwPowerTimeout = dwValue;
            }
            DEBUGMSG(ZONE_INIT, (_T("CDiskPower::Init: inactivity timeout is %u ms\r\n"), m_dwPowerTimeout));
            
            // get the inactivity timeout
            dwSize = sizeof(dwValue);
            dwStatus = RegQueryValueEx(hk, L"TimeoutDx", NULL, NULL, (LPBYTE) &dwValue, &dwSize);
            if(dwStatus == ERROR_SUCCESS) {
                if(VALID_DX((CEDEVICE_POWER_STATE)dwValue) && dwValue != D3) {
                    m_timeoutDx = (CEDEVICE_POWER_STATE) dwValue;
                } else {
                    DEBUGMSG(ZONE_WARNING, (_T("CDiskPower::Init: invalid or unsupported timeout device power state %d (0x%x)\r\n"), dwValue, dwValue));
                }
            }
            DEBUGMSG(ZONE_INIT, (_T("CDiskPower::Init: timeout state is D%d\r\n"), m_timeoutDx));
            
            // get the inactivity timeout
            dwSize = sizeof(dwValue);
            dwStatus = RegQueryValueEx(hk, L"InactivityPriority256", NULL, NULL, (LPBYTE) &dwValue, &dwSize);
            if(dwStatus == ERROR_SUCCESS) {
                nPriority = (int) dwValue;
            }
            DEBUGMSG(ZONE_INIT, (_T("CDiskPower::Init: inactivity timeout thread priority is %d\r\n"), nPriority));
            
            RegCloseKey(hk);
        }   

        // did we get a class string?
        if(!fGotClass) {
            // no, use the default disk class
            wcsncpy(szClass, PMCLASS_BLOCK_DEVICE, dim(szClass));
            szClass[dim(szClass) - 1] = 0;
        }

        // convert to a GUID
        fOk = GUIDFromString(szClass, &gPMClass);
        if(!fOk) {
            DEBUGMSG(ZONE_WARNING || ZONE_INIT, (_T("CDiskPower::Init: invalid power management class '%s'\r\n"),
                szClass));
        }
    }

    // get our active key from the registry
    if(fOk) {
        HKEY hk;
        dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_pDisk->m_szActiveKey, 0, 0, &hk);
        if(dwStatus == ERROR_SUCCESS) {
            DWORD dwValue;
            DWORD dwSize = sizeof(dwValue);
            dwStatus = RegQueryValueEx(hk, DEVLOAD_HANDLE_VALNAME, NULL, NULL, (LPBYTE) &dwValue, &dwSize);
            if(dwStatus != ERROR_SUCCESS) {
                DEBUGMSG(ZONE_WARNING || ZONE_INIT, (_T("CDiskPower::Init: can't read '%s' from '%s'\r\n"),
                    DEVLOAD_HANDLE_VALNAME, m_pDisk->m_szActiveKey));
                fOk = FALSE;
            } else {
                DEBUGCHK(dwValue != 0);
                hActive = (HANDLE) dwValue;
            }
        }
    }

    // figure out the name we are using
    if(fOk) {
        WCHAR szName[MAX_PATH];
        DWORD dwIndex = 0;
        do {
            DWORD dwSize = sizeof(szName);
            GUID gClass;
            fOk = EnumDeviceInterfaces(hActive, dwIndex, &gClass, szName, &dwSize);
            if(fOk && gPMClass == gClass) {
                // we found the interface
                break;
            }
            dwIndex++;
        } while(fOk);
        DEBUGMSG(!fOk && (ZONE_WARNING || ZONE_INIT), (_T("CDiskPower::Init: can't find PM interface\r\n")));

        // did we find the name?
        if(fOk) {
            // yes, allocate a name buffer to use to talk to the power manager
            DWORD dwChars = wcslen(PMCLASS_BLOCK_DEVICE) + wcslen(szName) + 2;  // class + separator + name + null
            LPWSTR pszPMName = (LPWSTR) LocalAlloc(LPTR, dwChars * sizeof(WCHAR));
            fOk = FALSE;        // assume failure
            if(pszPMName) {
                HRESULT hr = StringCchPrintfW(pszPMName, dwChars, L"{%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x}\\%s",
                    gPMClass.Data1, gPMClass.Data2, gPMClass.Data3,
                    (gPMClass.Data4[0] << 8) + gPMClass.Data4[1], gPMClass.Data4[2], gPMClass.Data4[3], 
                    gPMClass.Data4[4], gPMClass.Data4[5], gPMClass.Data4[6], gPMClass.Data4[7],
                    szName);
                if(SUCCEEDED(hr)) {
                    m_pszPMName = (LPCWSTR) pszPMName;
                    fOk = TRUE;
                }
            }
            DEBUGMSG(!fOk && (ZONE_WARNING || ZONE_INIT), (_T("CDiskPower::Init: can't find PM interface\r\n")));
        }
    }

    // create an event to tell the timeout thread about activity
    if(fOk) {
        m_hevPowerSignal = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hevDummy = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(!m_hevPowerSignal || !m_hevDummy) {
            DEBUGMSG(ZONE_WARNING || ZONE_INIT, (_T("CDiskPower::Init: couldn't create status events\r\n")));
            fOk = FALSE;
        }
    }

    // create the timeout thread
    if(fOk) {
        m_htPower = CreateThread(NULL, 0, DiskPowerThreadStub, this, 0, NULL);
        if(!m_htPower) {
            DEBUGMSG(ZONE_WARNING || ZONE_INIT, (_T("CDiskPower::Init: CreateThread() failed %d\r\n"), GetLastError()));
            fOk = FALSE;
        } else {
            BOOL fSuccess = CeSetThreadPriority(m_htPower, nPriority);
            DEBUGMSG(!fSuccess && (ZONE_WARNING || ZONE_INIT), (_T("CDiskPower::Init: CeSetThreadPriority(%d) failed %d\r\n"), nPriority, GetLastError()));
        }
    }

    // disable the standby timer, since the PM is going to be controlling
    // the disk power state
    if(fOk) {
        if(!m_pDisk->SendDiskPowerCommand(ATA_NEW_CMD_IDLE, 0)) {
            DEBUGMSG(ZONE_WARNING || ZONE_INIT, (_T("CDiskPower::Init: disable standby timer failed\r\n")));
        }
    }

    // on error, cleanup will happen in the destructor
    DEBUGMSG(ZONE_INIT, (_T("-CDiskPower::Init(): returning %d\r\n"), fOk));
    return fOk;
}

// This routine issues the ATAPI commands necessary to put the disk into a new power state.
// The caller must hold the disk critical section.
DWORD CDiskPower::SetDiskPower(CEDEVICE_POWER_STATE newDx)
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    DEBUGCHK(VALID_DX(newDx));
    PREFAST_DEBUGCHK(m_pDisk != NULL);
    
    TakeCS();
    DEBUGMSG(ZONE_POWER, (_T("CDiskPower::SetDiskPower: updating from D%d to D%d\r\n"), m_curDx, newDx));
    if(newDx != m_curDx) {
        switch(newDx) {
        case D0:
        case D1:
        case D2:
            if(m_curDx == D4) {
                // have to reset and reinitialize to come out of SLEEP mode
                if(!m_pDisk->WakeUp()) {
                    DEBUGMSG(ZONE_ERROR, (_T("CDiskPower::SetDiskPower: couldn't re-initialize hard drive\r\n")));
                    dwStatus = ERROR_GEN_FAILURE;
                }
            }
            break;
        case D3:
        case D4:
            newDx = D4;         // no D3 support
            break;
        }

        // enter the new device state
        if(dwStatus == ERROR_SUCCESS && !m_pDisk->SetDiskPowerState(newDx)) {
            DEBUGMSG(ZONE_WARNING, (_T("CDiskPower::SetDiskPower: SetDiskPowerState(D%d) failed\r\n"), newDx));
            dwStatus = ERROR_GEN_FAILURE;
        }
        
        // update the device power status
        if(dwStatus == ERROR_SUCCESS) {
            LARGE_INTEGER li;
            BOOL fGotQPC = QueryPerformanceCounter(&li);
            if(!fGotQPC) {
                DEBUGMSG(ZONE_WARNING, (_T("CDiskPower::SetDiskPower: QueryPerformanceCounter() failed, can't update statistics\r\n")));
            } else {
                m_dxInfo[m_curDx].totalQPC.QuadPart += li.QuadPart - m_startQPC.QuadPart;
            }
            m_curDx = newDx;
            m_dxInfo[m_curDx].dwCount++;
            if(fGotQPC) {
                m_startQPC = li;
            }
        }
    }

    ReleaseCS();
    return dwStatus;
}

// This routine returns the disk's current device power state.  The caller must hold the disk
// critical section.
CEDEVICE_POWER_STATE CDiskPower::GetDiskPower(void)
{
    CEDEVICE_POWER_STATE curDx;
    TakeCS();
    curDx = m_curDx;
    DEBUGMSG(ZONE_POWER, (_T("CDiskPower::GetDiskPower: returning D%d\r\n"), curDx));
    ReleaseCS();
    return curDx;
}

// This routine fills in the POWER_CAPABILITIES structure with information for this driver
DWORD CDiskPower::GetDiskCapabilities(PPOWER_CAPABILITIES pCap)
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    DEBUGCHK(pCap != NULL);
    PREFAST_DEBUGCHK(m_pDisk != NULL);

    // clear the capabilities structure
    memset(pCap, 0, sizeof(*pCap));

    // has power management been enabled for this drive?
    if(!m_pDisk->IsPMEnabled()) {
        // no, just report D0 support
        pCap->DeviceDx = 0x11;       // support D4, D2, D1, D0        
    } else {
        pCap->DeviceDx = 0x11;       // support D4, D2, D1, D0
    }

    return dwStatus;
}

// This routine handles Power Manager IOCTLs for the disk.  It returns a Win32 error
// code if there's a problem, ERROR_SUCCESS if the IOCTL was handled successfully, or 
// ERROR_NOT_SUPPORTED if the IOCTL was not from the PM.  The caller must hold the
// disk critical section.
DWORD CDiskPower::DiskPowerIoctl(PIOREQ pIOReq)
{
    DWORD dwStatus = ERROR_INVALID_PARAMETER;

    PREFAST_DEBUGCHK(pIOReq != NULL);

    if (GetCurrentProcessId() != (DWORD)GetDirectCallerProcessId())
    {
        RETAILMSG(1, (L"ERROR: ATAPI: "
            L"Power IOCTLs can be called only from device process (caller process id 0x%08x)\n", GetDirectCallerProcessId()));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    switch(pIOReq->dwCode) {
    case IOCTL_POWER_CAPABILITIES:
        if(pIOReq->pOutBuf != NULL && pIOReq->dwOutBufSize >= sizeof(POWER_CAPABILITIES) && pIOReq->pBytesReturned != NULL) {
            POWER_CAPABILITIES pc;
            dwStatus = GetDiskCapabilities(&pc);
            if(dwStatus == ERROR_SUCCESS) {
                PPOWER_CAPABILITIES ppc = (PPOWER_CAPABILITIES) pIOReq->pOutBuf;
                *ppc = pc;
                *pIOReq->pBytesReturned = sizeof(*ppc);
            }
        }
        break;
    case IOCTL_POWER_SET:
        if(pIOReq->pOutBuf != NULL && pIOReq->dwOutBufSize == sizeof(CEDEVICE_POWER_STATE) && pIOReq->pBytesReturned != NULL) {
            CEDEVICE_POWER_STATE newDx = *(PCEDEVICE_POWER_STATE) pIOReq->pOutBuf;
            m_fReductionRequested = FALSE;
            m_fBoostRequested = FALSE;
            dwStatus = SetDiskPower(newDx);
            *pIOReq->pBytesReturned = sizeof(newDx);
            dwStatus = ERROR_SUCCESS;
        }
        break;
    case IOCTL_POWER_GET:
        if(pIOReq->pOutBuf != NULL && pIOReq->dwOutBufSize == sizeof(CEDEVICE_POWER_STATE) && pIOReq->pBytesReturned != NULL) {
            CEDEVICE_POWER_STATE curDx = GetDiskPower();
            *(PCEDEVICE_POWER_STATE) pIOReq->pOutBuf = curDx;
            *pIOReq->pBytesReturned = sizeof(curDx);
            dwStatus = ERROR_SUCCESS;
        }
        break;
    case IOCTL_DISK_GETPMTIMINGS:
        if(pIOReq->pInBuf != NULL && pIOReq->dwInBufSize >= sizeof(PowerTimings)) {
            PowerTimings pt;
            memset(&pt, 0, sizeof(pt));
            pt.dwSize = sizeof(pt);
            TakeCS();
            pt.dwLoadedTicks = GetTickCount() - m_dwStartTickCount;
            for(int i = 0; i < PwrDeviceMaximum; i++) {
                pt.DxTiming[i].dwCount = m_dxInfo[i].dwCount;
                pt.DxTiming[i].liElapsed = m_dxInfo[i].totalQPC;
            }
            LARGE_INTEGER li;
            if(QueryPerformanceCounter(&li)) {
                pt.DxTiming[m_curDx].liElapsed.QuadPart += li.QuadPart - m_startQPC.QuadPart;
            }
            ReleaseCS();

            // copy the data to the user buffer
            pPowerTimings ppt = (pPowerTimings) pIOReq->pInBuf;
            if(ppt->dwSize >= sizeof(PowerTimings) && ppt->dwSize <= pIOReq->dwInBufSize) {
                *ppt = pt;
                dwStatus = ERROR_SUCCESS;
            } else {
                dwStatus = ERROR_INVALID_PARAMETER;
            }
        }
        break;
    default:    // not a PM ioctl
        dwStatus = ERROR_NOT_SUPPORTED;
        break;
    }

    DEBUGMSG(dwStatus != ERROR_NOT_SUPPORTED && dwStatus != ERROR_SUCCESS && ZONE_WARNING, 
        (_T("CDiskPower::DiskPowerIoctl: ioctl 0x%x failed %u\r\n"), pIOReq->dwCode, dwStatus));
    return dwStatus;
}

