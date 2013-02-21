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
#include "csgreq.h"

/*++

Module Name:
    diskmain.cpp

Abstract:
    Base ATA/ATAPI device abstraction.

Revision History:

--*/

static HANDLE g_hTestUnitReadyThread = NULL;

// ----------------------------------------------------------------------------
// Function: CDisk
//     Constructor
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

CDisk::CDisk(
    )
{
    // empty
}

// ----------------------------------------------------------------------------
// Function: CDisk
//     Constructor
//
// Parameters:
//     hKey -
// ----------------------------------------------------------------------------

CDisk::CDisk(
    HKEY hKey
    )
{
    m_dwDeviceFlags = 0;
    m_pNextDisk = NULL;
    m_pATAReg = NULL;
    m_pATARegAlt = NULL;
    m_dwDevice = 0;
    m_hDevKey = hKey;
    m_dwDeviceId = 0;
    m_dwPort = 0;
    m_f16Bit = FALSE;
    m_fUseLBA48 = FALSE;
    m_fAtapiDevice = FALSE;
    m_fInterruptSupported = FALSE;
    m_szDiskName = NULL;
    m_fDMAActive = FALSE;
    m_dwOpenCount = 0;
    m_dwUnitReadyTime = 0;
    m_dwStateFlag = 0;
    m_dwLastCheckTime = 0;
    m_dwStride = 1;
    m_pDiskPower = NULL;
    m_rgbDoubleBuffer = NULL;
    m_wNextByte = 0xFFFF;

    m_pPort = NULL;

    // init generic structures
    InitializeCriticalSection(&m_csDisk);
    memset(&m_Id, 0, sizeof(IDENTIFY_DATA));
    memset(&m_DiskInfo, 0, sizeof(DISK_INFO));
    memset(&m_InqData, 0, sizeof(INQUIRY_DATA));
    m_dwCurrentUDMAMode = 0;
    m_pDMAVirtualAddress = NULL;
    
}

// ----------------------------------------------------------------------------
// Function: ~CDisk
//     Destructor
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

CDisk::~CDisk(
    )
{
    if (m_hDevKey) {
        RegCloseKey(m_hDevKey);
    }

    if(m_pDiskPower != NULL) {
        delete m_pDiskPower;
    }

    DeleteCriticalSection(&m_csDisk);

    // deallocate double buffer, if present
    if (NULL != m_rgbDoubleBuffer) {
        LocalFree((HLOCAL)m_rgbDoubleBuffer);
    }

    // deallocate sterile I/O request, if present
    if (NULL != m_pSterileIoRequest) {
        LocalFree((HLOCAL)m_pSterileIoRequest);
    }
    if (m_pPort->m_pDskReg[m_dwDeviceId]->dwEnablePDMA)
    {
        HalFreeCommonBuffer( NULL, 0, m_DMAPhyaddress, m_pDMAVirtualAddress, FALSE );    
    }

}

// ----------------------------------------------------------------------------
// Function: StallExecution
//     Stall execution for the specified period of time
//
// Parameters:
//     dwTime -
// ----------------------------------------------------------------------------

void
CDisk::StallExecution(
    DWORD dwTime
    )
{
    if ((dwTime >= 100) && (m_dwDeviceFlags & DFLAGS_DEVICE_CDROM)) {
        Sleep (dwTime / 100);
    }
    else {
        ::StallExecution(dwTime * 10);
    }
}

#define HELPER_

// These functions should be inlined or converted to macros
void CDisk::TakeCS()                    { EnterCriticalSection(&m_csDisk); }
void CDisk::ReleaseCS()                 { LeaveCriticalSection(&m_csDisk); }
void CDisk::Open()                      { InterlockedIncrement((LONG *)&m_dwOpenCount); }
void CDisk::Close()                     { InterlockedDecrement((LONG *)&m_dwOpenCount); }
BOOL CDisk::IsAtapiDevice()             { return m_fAtapiDevice; }
BOOL CDisk::IsCDRomDevice()             { return (((m_Id.GeneralConfiguration >> 8) & 0x1f) == ATA_IDDEVICE_CDROM); }
BOOL CDisk::IsDVDROMDevice()            { return TRUE; }
BOOL CDisk::IsDiskDevice()              { return (((m_Id.GeneralConfiguration >> 8) & 0x1f) == ATA_IDDEVICE_DISK); }
BOOL CDisk::IsRemoveableDevice()        { return (m_Id.GeneralConfiguration & IDE_IDDATA_REMOVABLE); }
BOOL CDisk::IsDMASupported()            { return ((m_Id.Capabilities & IDENTIFY_CAPABILITIES_DMA_SUPPORTED) && m_fDMAActive); }
BOOL CDisk::IsDRQTypeIRQ()              { return ((m_Id.GeneralConfiguration >> 5) & 0x0003) == ATA_DRQTYPE_INTRQ; }
WORD CDisk::GetPacketSize()             { return m_Id.GeneralConfiguration & 0x0003 ? 16 : 12; }
BOOL CDisk::IsValidCommandSupportInfo() { return ((m_Id.CommandSetSupported2 & (1 << 14)) && !(m_Id.CommandSetSupported2 & (1 << 15))); }
BOOL CDisk::IsWriteCacheSupported()     { return ((m_Id.CommandSetSupported1 & COMMAND_SET_WRITE_CACHE_SUPPORTED) && IsValidCommandSupportInfo()); }
BOOL CDisk::IsPMSupported()             { return (m_Id.CommandSetSupported1 & COMMAND_SET_POWER_MANAGEMENT_SUPPORTED && IsValidCommandSupportInfo()); }
BOOL CDisk::IsPMEnabled()               { return (IsPMSupported() && (m_Id.CommandSetFeatureEnabled1 & COMMAND_SET_POWER_MANAGEMENT_ENABLED)); }

// These functions are called (1x) in atamain and should be inlined
void CDisk::SetActiveKey(TCHAR *szActiveKey)
{
    wcsncpy(m_szActiveKey, szActiveKey, MAX_PATH - 1);
    m_szActiveKey[MAX_PATH - 1] = 0;
}

void CDisk::SetDeviceKey(TCHAR *szDeviceKey)
{
    wcsncpy(m_szDeviceKey, szDeviceKey, MAX_PATH - 1);
    m_szDeviceKey[MAX_PATH - 1] = 0;
}

#define _HELPER

// ----------------------------------------------------------------------------
// Function: InitController
//     Reset the controller and determine whether a device is present on the
//     channel; if a device is present, then query and store its capabilities
//
// Parameters:
//     fForce -
// ----------------------------------------------------------------------------

BOOL
CDisk::InitController(
    BOOL fForce
    )
{
    BOOL bRet = TRUE;

    // if the controller has not already been reset, then perform a soft-reset
    // to enable the channel

    if (!(m_dwDeviceFlags & DFLAGS_DEVICE_INITIALIZED)) {

        // perform a soft-reset on the controller; if we don't do this, then
        // we won't be able to detect whether or not devices are present on the
        // channel

        bRet = ResetController(FALSE);
        if (!bRet) {
            goto exit;
        }

        // if interrupt is supported, enable interrupt

        if (m_fInterruptSupported) {
            SelectDevice();
            WriteAltDriveController(ATA_CTRL_ENABLE_INTR);
            EnableInterrupt();
        }
    }

    // issue IDENTIFY DEVICE and/or IDENTIFY PACKET DEVICE
    bRet = Identify();
    if (!bRet) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!CDisk::InitController> Device did not respond to identify\r\n"
            )));
        goto exit;
    }
    else {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!CDisk::InitController> Device responded to identify\r\n"
            )));
        m_dwDeviceFlags |= DFLAGS_DEVICE_INITIALIZED;
    }

exit:;
    return bRet;
}

// ----------------------------------------------------------------------------
// Function: ConfigureRegisterBlock
//     This function is called by DSK_Init before any other CDisk function to
//     set up the register block.
//
// Parameters:
//     dwStride -
// ----------------------------------------------------------------------------

VOID
CDisk::ConfigureRegisterBlock(
    DWORD dwStride
    )
{
    m_dwStride = dwStride;
    m_dwDataDrvCtrlOffset = ATA_REG_DATA * dwStride;
    m_dwFeatureErrorOffset = ATA_REG_FEATURE * dwStride;
    m_dwSectCntReasonOffset = ATA_REG_SECT_CNT * dwStride;
    m_dwSectNumOffset = ATA_REG_SECT_NUM * dwStride;
    m_dwDrvHeadOffset = ATA_REG_DRV_HEAD * dwStride;
    m_dwCommandStatusOffset = ATA_REG_COMMAND * dwStride;
    m_dwByteCountLowOffset = ATA_REG_BYTECOUNTLOW * dwStride;
    m_dwByteCountHighOffset = ATA_REG_BYTECOUNTHIGH * dwStride;
    m_dwAltStatusOffset = ATA_REG_ALT_STATUS_CS1 * dwStride;
    m_dwAltDrvCtrl = ATA_REG_DRV_CTRL_CS1 * dwStride;
}

// ----------------------------------------------------------------------------
// Function: Init
//     This function is called by the IDE/ATA controller enumerator to trigger
//     the initialization of a device
//
// Parameters:
//     hActiveKey -
// ----------------------------------------------------------------------------

BOOL
CDisk::Init(
    HKEY hActiveKey
    )
{
    BOOL fRet = FALSE;
    // replicate CDisk::ReadRegistry

    m_dwWaitCheckIter = m_pPort->m_pController->m_pIdeReg->dwStatusPollCycles;
    m_dwWaitSampleTimes = m_pPort->m_pController->m_pIdeReg->dwStatusPollsPerCycle;
    m_dwWaitStallTime = m_pPort->m_pController->m_pIdeReg->dwStatusPollCyclePause;

    m_dwDiskIoTimeOut = DEFAULT_DISK_IO_TIME_OUT;

    // replicate CDisk::ReadSettings

    m_dwUnitReadyTime = DEFAULT_MEDIA_CHECK_TIME;

    // if DMA=2 and this is not an ATAPI device, then we'll set m_fDMAActive in Identify
    if (1 == m_pPort->m_pDskReg[m_dwDeviceId]->dwDMA) { // 0=PIO, 1=DMA, 2=ATA DMA only
        m_fDMAActive = TRUE;
    }
    m_dwDMAAlign = m_pPort->m_pController->m_pIdeReg->dwDMAAlignment;

    // m_dwDeviceFlags |= DFLAGS_DEVICE_ISDVD; this is ignored

    if (m_pPort->m_pDskReg[m_dwDeviceId]->dwInterruptDriven) {
        m_fInterruptSupported = TRUE;
    }

    // initialize controller

    if (!InitController(TRUE)) {
        goto exit;
    }

    if (m_Id.CommandSetSupported1 & 0x20) {
        if (SetWriteCacheMode(m_pPort->m_pDskReg[m_dwDeviceId]->dwWriteCache)) {
            if (m_pPort->m_pDskReg[m_dwDeviceId]->dwWriteCache) {
                m_dwDeviceFlags |= DFLAGS_USE_WRITE_CACHE;
                DEBUGMSG(ZONE_INIT, (_T(
                    "Atapi!CDisk::Init> Enabled write cache\r\n"
                    )));
            }
            else {
                m_dwDeviceFlags &= ~DFLAGS_USE_WRITE_CACHE;
                DEBUGMSG(ZONE_INIT, (_T(
                    "Atapi!CDisk::Init> Disabled on device write cache\r\n"
                    )));
            }
        }
        else {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::Init> Failed to set write cache mode\r\n"
                )));
        }
    }

    // set read look-ahead, if read look-ahead supported

    if ((m_Id.CommandSetSupported1 & 0x40) && m_pPort->m_pDskReg[m_dwDeviceId]->dwLookAhead) {
        if (SetLookAhead()) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Enabled look-ahead\r\n")));
        }
        else {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Failed to enable look-ahead\r\n")));
        }
    }

    // set transfer mode, if a specific transfer mode was specified in the
    // device's instance key

    BYTE bTransferMode = (BYTE)m_pPort->m_pDskReg[m_dwDeviceId]->dwTransferMode;
    if (0xFF != bTransferMode) {
        if (0x00 == bTransferMode) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Activating PIO default mode (0x%x)\r\n"), bTransferMode));
        }
        else if (0x01 == bTransferMode) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Activating PIO default mode (0x%x) with IORDY disabled\r\n"), bTransferMode));
        }
        else if ((bTransferMode & 0xF8) == 0x08) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Activating PIO flow control mode %d (0x%x)\r\n"), (bTransferMode & 0x07), bTransferMode));
        }
        else if ((bTransferMode & 0xF0) == 0x20) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Activating multiword DMA mode %d (0x%x)\r\n"), (bTransferMode & 0x07), bTransferMode));
        }
        else if ((bTransferMode & 0xF0) == 0x40) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Activating ultra DMA mode %d (0x%x)\r\n"), (bTransferMode & 0x07), bTransferMode));
        }
        else {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Activating unknown transfer mode (0x%x)\r\n"), bTransferMode));
        }
        // bTransferMode is a valid transfer mode
        if (!SetTransferMode(bTransferMode)) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Failed to set transfer mode\r\n")));
        }
    }

    if (IsDMASupported()) {
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Enabled DMA\r\n")));
    }

    fRet = TRUE;

exit:;
    return fRet;
}

// ----------------------------------------------------------------------------
// Function: ResetController
//     Implement ATA/ATAPI-6 R3B 9.2 (Software reset protocol)
//
// Parameters:
//     bSoftReset -
// ----------------------------------------------------------------------------

BOOL
CDisk::ResetController(
    BOOL bSoftReset // ignore
    )
{
    DWORD dwAttempts = 0;
    BYTE bStatus = 0;
    BOOL fRet = FALSE;

    // we have to negate the RESET signal for 5 microseconds before we assert it

    WriteAltDriveController(0x00);
    ::StallExecution(25);

    // Set_SRST
    // --------
    // to enter Set_SRST state, set SRST in the Device Control register to one;
    // this will assert the RESET signal and reset both devices on the current
    // channel

    WriteAltDriveController(0x04); // 0x04 == SRST
    // remain in this state for at least 5 microseconds; i.e., assert RESET signal
    // for at least 5 microseconds
    // if this is a hardware reset, then assert RESET signal for at least 25
    // microseconds
    ::StallExecution(25); // this should be CEDDK implementation

    // Clear_wait
    // ----------
    // clear SRST in the Device Control register, i.e., negate RESET signal

    WriteAltDriveController(0x00);
    // remain in this state for at least 2 milliseconds
    Sleep(5);

HSR2_Check_status:;

    // Check_status
    // ------------
    // read the Status or Alternate Status register
    // if BSY is set to one, then re-enter this state
    // if BSY is cleared to zero, check the ending status in the Error register
    // and the signature (9.12) and transition to Host_Idle

    bStatus = GetAltStatus(); // read Status register
    if (bStatus & 0x80) {
        // BSY is set to one, re-enter this state
        DEBUGMSG(ZONE_INIT, (TEXT(
            "Atapi!CDisk::ResetController> Device is busy; %u seconds remaining\r\n"
            ), (m_pPort->m_pController->m_pIdeReg->dwSoftResetTimeout - dwAttempts)));
        Sleep(1000);
        dwAttempts += 1;
        // a device has at most 31 seconds to complete a software reset; we'll use 3 seconds
        if (dwAttempts == m_pPort->m_pController->m_pIdeReg->dwSoftResetTimeout) {
            DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::ResetController> Timeout\r\n")));
            goto exit;
        }
        goto HSR2_Check_status;
    }
    DEBUGMSG(ZONE_INIT, (TEXT(
        "Atapi!CDisk::ResetController> Device is ready\r\n"
        )));

    // BSY is cleared to zero, check the ending status in the Error register
    // and the signature
    // TODO: Check the signature (9.12)

    // if ERR bit set to one, then the reset failed
    bStatus = GetAltStatus(); // read Status register
    if (bStatus & 0x01) {
        // ERR is set to one
        // the bits in the Error register are valid, but the Error register
        // doesn't provide any useful information in the case of SRST failing
        DEBUGMSG(ZONE_INIT, (TEXT(
            "Atapi!CDisk::ResetController> SRST failed\r\n"
            )));
        // TODO: Recover from error
        goto exit;
    }

    fRet = TRUE;

exit:;
    return fRet;
}

// ----------------------------------------------------------------------------
// Function: AtapiSoftReset
//     Issue ATAPI SOFT RESET command
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

void
CDisk::AtapiSoftReset(
    )
{
    WriteCommand(ATAPI_CMD_SOFT_RESET);
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 400);
    WaitForDisc(WAIT_TYPE_READY, 500);
}

// ----------------------------------------------------------------------------
// Function: IsDevicePresent
//     Determine whether a device is present on the channel
//
// Parameters:
//     None
//
// Notes:
//     If a device is present on a channel, then the device's associated
//     Error register is populated with 0x1.  If a device is not present on
//     a channel, then the device's associated Error register is populated
//     with 0xa or 0xb, for master or slave, respectively.
// ----------------------------------------------------------------------------

BOOL
CDisk::IsDevicePresent(
    )
{
    BYTE bError;
    BYTE bStatus;
    DWORD dwRetryCount = 200;

    // determine which device to select (i.e., which device this device is)
    if ((m_dwDevice == 0) || (m_dwDevice == 2)) {
        // select device 0
        ATA_WRITE_BYTE(m_pATAReg + m_dwDrvHeadOffset, ATA_HEAD_DRIVE_1);
    }
    else {
        // select device 1
        ATA_WRITE_BYTE(m_pATAReg + m_dwDrvHeadOffset, ATA_HEAD_DRIVE_2);
    }

Retry:;
    // read Status register
    bStatus = GetAltStatus();

    // read Error register
    bError = GetError();
    // test Error register
    if (bError == 0x1) {
        DEBUGMSG(ZONE_INIT, (_T(
            "Atapi!CDisk::IsDevicePresent> Device %d is present. Device is attached at %d times\r\n"
            ), m_dwDevice, 200 - dwRetryCount));
        return TRUE;
    }
    Sleep(10);
    if ( dwRetryCount-- )
        goto Retry;
    DEBUGMSG(ZONE_INIT, (_T(
        "Atapi!CDisk::IsDevicePresent> Device %d is not present; Error register(0x%x)\r\n"
        ), m_dwDevice, bError));
    
    return FALSE;
}

// ----------------------------------------------------------------------------
// Function: SendExecuteDeviceDiagnostic
//     Implement ATA/ATAPI-6 R3B 9.10 (Device diagnostic protocol)
//
// Parameters:
//     pbDiagnosticCode - diagnostic code returned by controller in Error
//                        register as a result of issuing EXECUTE DEVICE
//                        DIAGNOSTIC (8.11)
//
//     pfIsAtapi - whether device is an ATAPI device
// ----------------------------------------------------------------------------

BOOL
CDisk::SendExecuteDeviceDiagnostic(
    PBYTE pbDiagnosticCode,
    PBOOL pfIsAtapi
    )
{
    BYTE bStatus = 0;
    DWORD dwWaitAttempts = 1200;
    BOOL fReadSignature = FALSE;

    PREFAST_DEBUGCHK(NULL != pbDiagnosticCode);
    PREFAST_DEBUGCHK(NULL != pfIsAtapi);

    // HI4:HED0, write command

    WaitOnBusy(FALSE);
    WriteCommand(0x90); // EXECUTE DEVICE DIAGNOSTIC command code

    // HED0:Wait, wait for at least 2 milliseconds; see following Sleep(5)
    // HED2:Check_Status, wait on BSY=0

    while (1) {
        Sleep(5);                 // wait 5 milliseconds
        bStatus = GetAltStatus(); // get status
        // test error
        if (bStatus & ATA_STATUS_ERROR) {
            // error
            DEBUGMSG(ZONE_ERROR, (TEXT(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device failed to process command\r\n"
                ), m_dwDeviceId));
            break;
        }
        // test BSY=0
        if (!(bStatus & ATA_STATUS_BUSY)) break;
        // retry
        if (dwWaitAttempts-- == 0) {
            DEBUGMSG(ZONE_ERROR, (TEXT(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> No response; assuming channel is empty\r\n"
                ), m_dwDeviceId));
            break;
        }
    }

    // inspect result of diagnosis (table 26, 8.11); select self

    SelectDevice();
    *pbDiagnosticCode = GetError();
    if ((m_dwDevice == 0) || (m_dwDevice == 0)) {
        // device 0 (master)
        if (*pbDiagnosticCode == 0x01) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device 0 passed, Device 1 passed or not present\r\n"
                )));
            fReadSignature = TRUE; // read signature to determine if we're ATA or ATAPI
        }
        else if (*pbDiagnosticCode == 0x00 || (0x02 <= *pbDiagnosticCode && *pbDiagnosticCode <= 0x7F)) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device 0 failed, Device 1 passed or not present\r\n"
                )));
        }
        else if (*pbDiagnosticCode == 0x81) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device 0 passed, Device 1 failed\r\n"
                )));
            fReadSignature = TRUE; // read signature to determine if we're ATA or ATAPI
        }
        else if (*pbDiagnosticCode == 0x80 || (0x82 <= *pbDiagnosticCode && *pbDiagnosticCode <= 0xFF)) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device 0 failed, Device 1 failed\r\n"
                )));
        }
        else {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Unknown diagnostic code(0x%x)\r\n"
                ), *pbDiagnosticCode));
        }
    }
    else {
        // device 1 (slave)
        if (*pbDiagnosticCode == 0x01) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device 1 passed\r\n"
                )));
            fReadSignature = TRUE; // read signature to determine if we're ATA or ATAPI
        }
        else if (*pbDiagnosticCode == 0x00 || (0x02 <= *pbDiagnosticCode && *pbDiagnosticCode <= 0x7F)) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device 1 failed\r\n"
                )));
        }
        else {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Unknown diagnostic code(0x%x)\r\n"
                ), *pbDiagnosticCode));
        }
    }

    if (fReadSignature) {

        // we passed; read signature to determine if it's ATA or ATAPI

        // test for ATA
        if (
            ATA_READ_BYTE(m_pATAReg + m_dwSectNumOffset) == 0x01 &&
            ATA_READ_BYTE(m_pATAReg + m_dwByteCountLowOffset) == 0x00 &&
            ATA_READ_BYTE(m_pATAReg + m_dwByteCountHighOffset) == 0x00 // &&
            // ATA_READ_BYTE(m_pATAReg + m_dwDrvHeadOffset) == 0x00 &&
            // ATA_READ_BYTE(m_pATAReg + m_dwSectCntReasonOffset) == 0x01 &&
        ) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> ATA device\r\n"
                )));
            *pfIsAtapi = FALSE;
        }
        // test for ATAPI
        else if (
            ATA_READ_BYTE(m_pATAReg + m_dwSectNumOffset) == 0x01 &&
            ATA_READ_BYTE(m_pATAReg + m_dwByteCountLowOffset) == 0x14 &&
            ATA_READ_BYTE(m_pATAReg + m_dwByteCountHighOffset) == 0xEB // &&
            // ATA_READ_BYTE(m_pATAReg + m_dwDrvHeadOffset) == 0x00 &&
            // ATA_READ_BYTE(m_pATAReg + m_dwSectCntReasonOffset) == 0x01 &&
        ) {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> ATAPI device\r\n"
                )));
            *pfIsAtapi = TRUE;
        }
        // unknown
        else {
            DEBUGMSG(ZONE_INIT, (_T(
                "Atapi!CDisk::SendExecuteDeviceDiagnostic> Device 0 = Unknown device type (i.e., not ATA, not ATAPI)\r\n"
                )));
        }
    }

    // clear pending interrupt, if applicable
    GetBaseStatus();

    return TRUE;
}

// ----------------------------------------------------------------------------
// Function: SendIdentifyDevice
//     Issue IDENTIFY_DEVICE or IDENTIFY_PACKET_DEVICE depending on whether
//     fIsAtapi is TRUE.  Implement PIO data-in command protocol as per
//     ATA/ATAPI-6 R3B 9.2.
//     Implement ATA/ATAPI-6 R3B 9.10 (Device diagnostic protocol)
//
// Parameters:
//     fIsAtapi - if device is ATAPI, send IDENTIFY PACKET DEVICE
//
// Notes:
//     After issuing a PIO data-in command, if BSY=0 and DRQ=0, then the device
//     failed to process the command.  However, there exist devices that
//     require additional time to return status via the Status register.  As
//     such, a delayed retry has been introduced to faciliate such devices,
//     even though their actions do not comply with the specification.
// ----------------------------------------------------------------------------

#define HPIOI1_CHECK_STATUS_RETRIES 10
BOOL
CDisk::SendIdentifyDevice(
    BOOL fIsAtapi
    )
{
    BOOL fResult = TRUE;
    DWORD dwRetries = 0;
    BYTE bStatus;               // Status register
    DWORD cbIdentifyDeviceData; // IDENTIFY DEVICE data size

    // Host Idle protocol

    // select correct device
    SelectDevice();

    // HI1:Check_Status
    // ----------------
HI1_Check_Status:;
    bStatus = GetAltStatus();
    if ((bStatus & ATA_STATUS_BUSY) || (bStatus & ATA_STATUS_DATA_REQ)) { // BSY=1 or DRQ=1
        Sleep(5);
        goto HI1_Check_Status;
    }

    // HI3:Write_Parameters
    // --------------------
    // no paramters

    // HI4:Write_Command
    *((UINT32 *)(m_pATAReg + ATA_CFG)) |= 0x40;
    __try {
        WriteCommand(fIsAtapi ? ATAPI_CMD_IDENTIFY : ATA_CMD_IDENTIFY);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::SendIdentifyDevice> Exception writing to Command register\r\n"
            )));
        fResult = FALSE;
        goto exit;
    }

    // PIO data-in command protocol

    // HPIOI1:Check_Status
    // -------------------
HPIOI1_Check_Status:;
    __try {
        bStatus = GetAltStatus();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::SendIdentifyDevice> Exception\r\n"
            )));
        fResult = FALSE;
        goto exit;
    }
    if (!(bStatus & (ATA_STATUS_BUSY|ATA_STATUS_DATA_REQ))) { // BSY=0 and DRQ=0
        // an error occurred
        if (dwRetries < HPIOI1_CHECK_STATUS_RETRIES) {
            dwRetries++;
            Sleep(5);
            goto HPIOI1_Check_Status;
         }
         fResult = FALSE;
         goto exit;
    }
    if (bStatus & ATA_STATUS_BUSY) { // BSY=1
        goto HPIOI1_Check_Status;
    }
    if (!(bStatus & ATA_STATUS_BUSY) && (bStatus & ATA_STATUS_DATA_REQ)) { // BSY=0 and DRQ=1
        goto HPIOI2_Transfer_Data;
    }

    // HPIOI2:Transfer_Data
    // --------------------
    // (IDENTIFY [ATAPI] DEVICE only returns a single DRQ data block)
HPIOI2_Transfer_Data:;
    cbIdentifyDeviceData = sizeof(IDENTIFY_DATA);
    DEBUGCHK(cbIdentifyDeviceData <= BYTES_PER_SECTOR);
    // read result of IDENTIFY DEVICE/IDENTIFY PACKET DEVICE
    if (m_f16Bit) {
    
        USHORT temp[sizeof(IDENTIFY_DATA)/2];    
        USHORT value;
        
        cbIdentifyDeviceData /= 2;
        ReadWordBuffer((PWORD)temp, cbIdentifyDeviceData);

        for (DWORD i=0; i<cbIdentifyDeviceData ; i++)
        {
            value = ((*(temp + i)& 0xff00) >> 8) & 0xff;
            value |= ((*(temp + i)& 0xff ) << 8) & 0xff00;
            
            *(((USHORT *)(&m_Id)) + i) = value;
        }
    }
    else {
        ReadByteBuffer((PBYTE)&m_Id, cbIdentifyDeviceData);
    }
    // ignore extraneous data
    while (GetAltStatus() & ATA_STATUS_DATA_REQ ) {
        if (m_f16Bit) {
            ReadWord();
        }
        else {
            ReadByte();
        }
    }

    *((UINT32 *)(m_pATAReg + ATA_CFG)) &= ~(0x40);
    // Return to Host Idle protocol

exit:;

    // clear pending interrupt, if applicable
    GetBaseStatus();

    return fResult;
}

// ----------------------------------------------------------------------------
// Function: Identify
//     This function initiates communication with a device.  If the
//     appropriate device is detected on the current channel, then issue an
//     EXECUTE DEVICE DIAGNOSTIC to determine whether the device is a master
//     or slave and ATA or ATAPI device.  Then, issue an IDENTIFY DEVICE/
//     IDENTIFY PACKET DEVICE and select the appropriate READ/WRITE command.
//     Finally, assemble the device's associated DISK_INFO structure.
//
// Parameters:
//     None
// ----------------------------------------------------------------------------
BOOL CDisk::Identify()
{
    DWORD dwBlockSize = 0; // Size of IDENTIFY DEVICE/IDENTIFY PACKET DEVICE information.
    WORD  wDevType = 0;    // Supported command packet set (e.g., direct-access, CD-ROM, etc.).

    TakeCS();

    // If device isn't present, exit.
    if (FALSE == IsDevicePresent()) {
        ReleaseCS();
        return FALSE;
    }

    // Issue EXECUTE DEVICE DIAGNOSTIC.  Determine whether the device is ATA or
    // ATAPI (ignore the result of this call, as old devices fail to respond
    // correctly).
    BYTE bDiagnosticCode;
    BOOL fIsAtapi;
    SendExecuteDeviceDiagnostic(&bDiagnosticCode, &fIsAtapi);

    // Is this an ATA device?
    if (TRUE == SendIdentifyDevice(FALSE)) { // fIsAtapi=FALSE
        m_fAtapiDevice = FALSE;
        // Some IDE controllers have problems with ATAPI DMA.
        if (2 == m_pPort->m_pDskReg[m_dwDeviceId]->dwDMA) { // 0=PIO, 1=DMA, 2=ATA DMA only
            m_fDMAActive = TRUE;
        }
    }
    // Is this an ATAPI device?
    else if (TRUE == SendIdentifyDevice(TRUE)) { // fIsAtapi=TRUE
        m_fAtapiDevice = TRUE;
    }
    else {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T("Atapi!CDisk::Identify> Device failed to respond to IDENTIFY DEVICE and IDENTIFY PACKET DEVICE\r\n")));
        ReleaseCS();
        return FALSE;
    }

    ReleaseCS();

    // Validate IDENTIFY DEVICE/IDENTIFY PACKET DEVICE signature; an empty
    // channel may return invalid data.

    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> ATA-Disk Total Sector Size 0x%x\r\n"), m_Id.TotalUserAddressableSectors));            

    if ((m_Id.GeneralConfiguration == 0) || (m_Id.GeneralConfiguration == 0xffff) ||
        (m_Id.GeneralConfiguration == 0xff7f) ||
        (m_Id.GeneralConfiguration == 0x7fff) ||
       ((m_Id.GeneralConfiguration == m_Id.IntegrityWord) && (m_Id.NumberOfCurrentCylinders == m_Id.IntegrityWord))
    ) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!CDisk::Identify> General configuration(%04X) not valid; device not present\r\n"
            ), m_Id.GeneralConfiguration));
        return FALSE;
    }
    // Dump IDENTFY DEVICE/IDENTIFY PACKET DEVICE data.
    PIDENTIFY_DATA pId = &m_Id;
    DUMPIDENTIFY(pId);
    DUMPSUPPORTEDTRANSFERMODES(pId);

    // ATA/ATAPI-3 compatible devices store the supported command packet set in
    // bits 12-8 of word 0 of IDENTIFY DEVICE/IDENTIFY PACKET DEVICE data (this
    // information is retired in ATA/ATAPI-6).  Determine which command packet
    // set is implemented by the device.
    wDevType = (m_Id.GeneralConfiguration >> 8) & 0x1F;
    switch (wDevType) {
        case ATA_IDDEVICE_UNKNOWN:
            return FALSE;
        case ATA_IDDEVICE_CDROM:
            m_dwDeviceFlags |= DFLAGS_DEVICE_CDROM;
            break;
        case ATA_IDDEVICE_DISK:
            break;
        case ATA_IDDEVICE_OPTICAL_MEM:
            break;
        default:
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Assuming direct-access device (hard disk drive)\r\n")));
            break;
    }

    // This is redundant, but various routines use this information.
    m_dwDeviceFlags |= DFLAGS_DEVICE_PRESENT;
    m_dwDeviceFlags |= (IsAtapiDevice()) ? DFLAGS_ATAPI_DEVICE : 0;
    m_dwDeviceFlags |= (IsRemoveableDevice()) ? DFLAGS_REMOVABLE_DRIVE : 0;

    // ATA devices support two flavors of read/write commands: READ/WRITE
    // SECTOR(S) and READ/WRITE MULTIPLE.  READ/WRITE SECTOR(S) use a fixed DRQ
    // data block size of 1 sector and READ/WRITE MULTIPLE use a driver-
    // specified (variable) DRQ data block size of up to 255 sectors.
    if (FALSE == IsAtapiDevice()) {
        // Default to READ/WRITE SECTOR(S).
        m_bReadCommand = ATA_CMD_READ;
        m_bWriteCommand = ATA_CMD_WRITE;
        m_bSectorsPerBlock = 1;
        // Is READ/WRITE MULTIPLE supported?
        if (0 != m_Id.MaximumBlockTransfer) {
            // The device supports a variable DRQ data block size.  Set the DRQ
            // data block size to the minimum of the registry-specified DRQ
            // data block size and the maximum DRQ data block size supported by
            // the device.
            if (0 == m_pPort->m_pDskReg[m_dwDeviceId]->dwDrqDataBlockSize) {
                // Use device's maximum DRQ data block size.
                m_bSectorsPerBlock = m_Id.MaximumBlockTransfer;
            }
            else {
                m_bSectorsPerBlock = MIN((BYTE)(m_pPort->m_pDskReg[m_dwDeviceId]->dwDrqDataBlockSize / SECTOR_SIZE), m_Id.MaximumBlockTransfer);
            }
            SelectDevice();
            WriteSectorCount(m_bSectorsPerBlock);
            WriteCommand(ATA_CMD_SET_MULTIPLE);
            if ((FALSE == WaitOnBusy(FALSE)) && (ATA_STATUS_READY & GetAltStatus())) {
                m_bReadCommand = ATA_CMD_MULTIPLE_READ;
                m_bWriteCommand = ATA_CMD_MULTIPLE_WRITE;
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Set DRQ data block size to %d sectors (READ/WRITE MULTIPLE sectors per interrupt)\r\n"), m_bSectorsPerBlock));
            }
            else {
                // Revert DRQ data block size.
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Failed to set DRQ data block size to %d sectors; using READ/WRITE SECTOR(S)\r\n"), m_bSectorsPerBlock));
                m_bSectorsPerBlock = 1;
            }
        }
    }

    m_bDMAReadCommand = ATA_CMD_READ_DMA;
    m_bDMAWriteCommand = ATA_CMD_WRITE_DMA;

    // Assemble DISK_INFO structure.
    m_fLBAMode = (m_Id.Capabilities & 0x0200) ? TRUE : FALSE;
    m_DiskInfo.di_flags = DISK_INFO_FLAG_MBR;
    m_DiskInfo.di_bytes_per_sect = BYTES_PER_SECTOR;
    m_DiskInfo.di_cylinders = m_Id.NumberOfCylinders;
    m_DiskInfo.di_heads = m_Id.NumberOfHeads;
    m_DiskInfo.di_sectors = m_Id.SectorsPerTrack;
    if (m_fLBAMode) {
        m_DiskInfo.di_total_sectors = m_Id.TotalUserAddressableSectors;
        ConfigLBA48(); // set m_fUseLBA48 if applicable
    }
    else {
        m_DiskInfo.di_total_sectors = m_DiskInfo.di_cylinders*m_DiskInfo.di_heads * m_DiskInfo.di_sectors;
    }

    return TRUE;
}


// ----------------------------------------------------------------------------
// Function: ConfigLBA48
//     This is a helper function which is called after the IDENTIFY_DEVICE
//     command has been successfully executed.  It parses the results
//     of the IDENTIFY_DEVICE command to determine if 48-bit LBA is supported
//     by the device.
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

void CDisk::ConfigLBA48(void)
{
    PIDENTIFY_DATA pId = (PIDENTIFY_DATA)&m_Id;

    if (m_pPort->m_pController->m_pIdeReg->dwDisable48BitLBA) 
    {
        m_fUseLBA48 = FALSE;
        return;
    }

    // Word 87 (CommandSetFeatureDefault): 
    //         bit 14 is set and bit 15 is cleared if config data
    //         in word 86 (CommandSetFeatureEnabled2) is valid.
    // Note that this is only valid for non-ATAPI devices
    if ( !IsAtapiDevice() &&
         (pId->CommandSetFeatureDefault & (1 << 14)) &&
         !(pId->CommandSetFeatureDefault & (1 << 15)) &&
         (pId->CommandSetFeatureEnabled2 & (1 << 10)) )
    {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::ConfigLBA48> Device supports 48-bit LBA\r\n")));
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::ConfigLBA48> Max LBA Address = 0x%08x%08x"),
                             pId->lMaxLBAAddress[1],
                             pId->lMaxLBAAddress[0]));


        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::ConfigLBA48> Device supports 48-bit LBA\r\n")));
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::ConfigLBA48> Max LBA Address = 0x%08x, 0x%08x"),
                             pId->lMaxLBAAddress[1],
                             pId->lMaxLBAAddress[0]));

        m_fUseLBA48 = TRUE;

        // The CE file system currently supports a maximum of 32-bit sector addresses,
        if (pId->lMaxLBAAddress[1] == 0)
        {
            m_DiskInfo.di_total_sectors = pId->lMaxLBAAddress[0];
            DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::ConfigLBA48> This disk supports LBA48 mode and total sector 0x%x\r\n"), m_DiskInfo.di_total_sectors));            
        }
            
        else
            m_DiskInfo.di_total_sectors = 0xFFFFFFFF; // Couldn't find MAX_DWORD

        ASSERT(pId->lMaxLBAAddress[1] <= 0x10000);

        // CDisk::Identify has determined whether or not the device supports multi-sector transfers
        // Update read/write command to use [READ|WRITE] [SECTORS|MULTIPLE] EXT
        if (m_bReadCommand == ATA_CMD_READ) 
        {
            m_bReadCommand = ATA_CMD_READ_SECTOR_EXT;
            m_bWriteCommand = ATA_CMD_WRITE_SECTOR_EXT;
        }
        else // CDisk::Identify has determined that the devce supports multi-sector transfers
        {
            m_bReadCommand = ATA_CMD_READ_MULTIPLE_EXT;
            m_bWriteCommand = ATA_CMD_WRITE_MULTIPLE_EXT;
        }

        m_bDMAReadCommand = ATA_CMD_READ_DMA_EXT;
        m_bDMAWriteCommand = ATA_CMD_WRITE_DMA_EXT;

    }
    else
    {
        m_fUseLBA48 = FALSE;
    }
}

// ----------------------------------------------------------------------------
// Function: ValidateSg
//     Map embedded pointers
//
// Parameters:
//     pSgReq -
//     InBufLen - The size of the actual buffer as specified by the user.
//                This needs to be cross checked with pSgReq->sr_num_sg to
//                prevent buffer overflows
//     saveoldptrs - Your old pointers are saved here. Must be at least
//                   pSgReq->sr_num_sg in size
// ----------------------------------------------------------------------------

BOOL
CDisk::ValidateSg(
    PSG_REQ pSgReq,
    DWORD InBufLen,
    DWORD dwArgType,
    OUT PUCHAR * saveoldptrs
    )
{
    DWORD dwIndex ;
    PUCHAR ptemp;

    if (NULL == pSgReq) {
        ASSERT(pSgReq);
        return FALSE;
    }

    if (InBufLen < sizeof(SG_REQ)) {
        ASSERT(InBufLen >= sizeof(SG_REQ));
        return FALSE;
    }

    // pSgReq is a sterile copy of the caller's SG_REQ; we're supposed to map
    // the embedded sb_bufs back into the SG_REQ

    if(
        !(pSgReq->sr_num_sg >= 1) ||
        !(pSgReq->sr_num_sg <= MAX_SG_BUF) ||
        !((sizeof(SG_REQ) + sizeof(SG_BUF)*(pSgReq->sr_num_sg-1)) == InBufLen) ||
        !(pSgReq->sr_num_sec > 0))
    {
        ASSERT(pSgReq->sr_num_sg >= 1);
        ASSERT(pSgReq->sr_num_sg <= MAX_SG_BUF);
        ASSERT((sizeof(SG_REQ) + sizeof(SG_BUF)*(pSgReq->sr_num_sg-1)) == InBufLen);
        ASSERT(pSgReq->sr_num_sec > 0);
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < pSgReq->sr_num_sg; dwIndex++) {
        if (
            (NULL == pSgReq->sr_sglist[dwIndex].sb_buf) ||
            (0 == pSgReq->sr_sglist[dwIndex].sb_len)
        ) {
            goto CleanUpLeak;
        }

        // Verify embedded pointer access and map user mode pointers

        if (FAILED(CeOpenCallerBuffer(
                    (PVOID *)&ptemp,
                    (LPVOID)pSgReq->sr_sglist[dwIndex].sb_buf,
                    pSgReq->sr_sglist[dwIndex].sb_len,
                    dwArgType,
                    FALSE)))
        {
            goto CleanUpLeak;
        }

        saveoldptrs[dwIndex] = pSgReq->sr_sglist[dwIndex].sb_buf;
        pSgReq->sr_sglist[dwIndex].sb_buf = ptemp;

        if (NULL == pSgReq->sr_sglist[dwIndex].sb_buf) {
            goto CleanUpLeak;
        }
    }

    return TRUE;

CleanUpLeak:

    if (FAILED(UnmapSg(
                pSgReq->sr_sglist,
                saveoldptrs,
                dwIndex,
                dwArgType)))
    {
        ASSERT(!"Cleanup call to CeCloseCallerBuffer failed unexpectedly");
        return FALSE;
    }

    return FALSE;

}


// ----------------------------------------------------------------------------
// Function: UnmapSg
//     UnMap embedded pointers, previously mapped by ValidateSg.
//     Basically, an SG Array version of CeCloseCallerBuffer
//
// Parameters:
//     sr_sglist - List of mapped SG buffers to unmap
//     saveoldptrs - List of old unmapped pointers
//     sr_sglistlen - The size of sr_sglist
//     dwArgType - ARG_O_PTR/ ARG_I_PTR
//     Return value -  HRESULT from a failed call to CeCloseCallerBuffer
//                     otherwise ERROR_SUCCESS
// ----------------------------------------------------------------------------

HRESULT
CDisk::UnmapSg(
    IN const SG_BUF * sr_sglist,
    IN const PUCHAR * saveoldptrs,
    IN DWORD sr_sglistlen,
    IN DWORD dwArgType
    )
{
    HRESULT dwError = ERROR_SUCCESS;    
    ASSERT(sr_sglistlen <= MAX_SG_BUF);
    
    for (DWORD dwIndex = 0; dwIndex < sr_sglistlen; dwIndex++) {

        HRESULT dwtemp;
        ASSERT(NULL != sr_sglist[dwIndex].sb_buf);
        ASSERT(0 != sr_sglist[dwIndex].sb_len);

        // Close pointers previously mapped in ValidateSg

        dwtemp = CeCloseCallerBuffer(
                    (LPVOID)sr_sglist[dwIndex].sb_buf,
                    (LPVOID)saveoldptrs[dwIndex],
                    sr_sglist[dwIndex].sb_len,
                    dwArgType);
        
        if (FAILED(dwtemp)) {
            ASSERT(!"Cleanup call to CeCloseCallerBuffer failed unexpectedly");
            dwError = dwtemp;
        }
    }

    return dwError;
    
}

// ----------------------------------------------------------------------------
// Function: SendDiskPowerCommand
//     Put the device into a specified power state.  The optional parameter is
//     programmed into the Sector Count register, which is used for the
//     ATA NEW CMD IDLE and ATA CMD STANDBY commands.
//
// Parameters:
//     bCmd -
//     bParam -
// ----------------------------------------------------------------------------

BOOL
CDisk::SendDiskPowerCommand(
    BYTE bCmd,
    BYTE bParam
    )
{
    BYTE bError, bStatus;
    BOOL fOk = TRUE;

    if(ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_POWERCOMMAND, &bCmd, sizeof(bCmd), 0, CELZONE_ALWAYSON, 0, FALSE);

    // HI:Check_Status (Host Idle); wait until BSY=0 and DRQ=0
    // read Status register
    while (1) {
        bStatus = GetAltStatus();
        if (!(bStatus & (0x80|0x08))) break; // BSY := Bit 7, DRQ := Bit 3
        Sleep(5);
    }

    // HI:Device_Select; select device
    SelectDevice();

    // HI:Check_Status (Host Idle); wait until BSY=0 and DRQ=0
    // read Status register
    while (1) {
        bStatus = GetAltStatus();
        if (!(bStatus & (0x80|0x08))) break; // BSY := Bit 7, DRQ := Bit 3
        Sleep(5);
    }

    // HI:Write_Parameters
    WriteSectorCount(bParam);
    // WriteAltDriveController(0x00); // disable interrupt (nIEN := Bit 1 of Device Control register)

    // HI:Write_Command
    WriteCommand(bCmd);

    // transition to non-data command protocol

    // HND:INTRQ_Wait
    // transition to HND:Check_Status
    // read Status register
    while (1) { // BSY := Bit 7
        bStatus = GetAltStatus();
        bError = GetError();
        if (bError & 0x04) { // ABRT := Bit 2
            // command was aborted
            DEBUGMSG(ZONE_ERROR, (_T(
                "Atapi!CDisk::SendDiskPowerCommand> Failed to send command 0x%x, parameter 0x%x\r\n"
                ), bCmd, bParam));
            fOk = FALSE;
            break;
        }
        if (!(bStatus & 0x80)) break; // BSY := Bit 7
        Sleep(5);
    }

    // transition to host idle protocol

    return fOk;
}

// ----------------------------------------------------------------------------
// Function: GetDiskPowerInterface
//     Return the power management object associated with this device
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

CDiskPower *
CDisk::GetDiskPowerInterface(
    void
    )
{
    CDiskPower *pDiskPower = new CDiskPower;
    return pDiskPower;
}

// ----------------------------------------------------------------------------
// Function: SetDiskPowerState
//     Map a power state to an ATA power management command and issue the
//     command
//
// Parameters:
//     newDx -
// ----------------------------------------------------------------------------

BOOL
CDisk::SetDiskPowerState(
    CEDEVICE_POWER_STATE newDx
    )
{
    BYTE bCmd;

    if (ZONE_CELOG) {
        DWORD dwDx = (DWORD) newDx;
        CeLogData(TRUE, CELID_ATAPI_SETDEVICEPOWER, &dwDx, sizeof(dwDx), 0, CELZONE_ALWAYSON, 0, FALSE);
    }

    // on D0 go to IDLE to minimize latency during disk accesses
    if(newDx == D0 || newDx == D1) {
        bCmd = ATA_CMD_IDLE_IMMEDIATE;
    }
    else if(newDx == D2) {
        bCmd = ATA_CMD_STANDBY_IMMEDIATE;
    }
    else if(newDx == D3 || newDx == D4) {
        bCmd = ATA_CMD_SLEEP;
    }
    else {
        DEBUGMSG(ZONE_WARNING, (_T(
            "CDisk::SetDiskPowerState> Invalid power state value(%u)\r\n"
            ), newDx));
        return FALSE;
    }

    // update the disk power state
    return SendDiskPowerCommand(bCmd);
}

// ----------------------------------------------------------------------------
// Function: WakeUp
//     Wake the device up from sleep
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL
CDisk::WakeUp(
    )
{
    if (!ResetController(FALSE)) {
        return FALSE;
    }
    return SendIdentifyDevice(IsAtapiDevice());
}

// ----------------------------------------------------------------------------
// Function: MainIoctl
//     Process IOCTL_DISK_ and DISK_IOCTL_ I/O controls
//
// Parameters:
//     pIOReq -
// ----------------------------------------------------------------------------
DWORD CDisk::MainIoctl(PIOREQ pIOReq)
{
    DWORD dwError = ERROR_SUCCESS;

    DEBUGMSG(ZONE_IOCTL, (TEXT("Atapi!CDisk::MainIoctl> IOCTL(%x), device(%x)\r\n"), pIOReq->dwCode, m_dwDeviceId));

    // If device is powering down, then fail.
    if (m_dwDeviceFlags & DFLAGS_DEVICE_PWRDN) {
        SetLastError(ERROR_DEVICE_NOT_AVAILABLE);
        return FALSE;
    }
    switch(pIOReq->dwCode) {
        case IOCTL_DISK_GETINFO:
        case DISK_IOCTL_GETINFO:
            if (IsCDRomDevice()) {
                dwError = ERROR_BAD_COMMAND;
            }
            else {
                dwError = GetDiskInfo(pIOReq);
            }
            break;
        case IOCTL_DISK_DEVICE_INFO:
            dwError = GetDeviceInfo(pIOReq);
            break;
        case DISK_IOCTL_GETNAME:
        case IOCTL_DISK_GETNAME:
            dwError = GetDiskName(pIOReq);
            break;
        case DISK_IOCTL_SETINFO:
        case IOCTL_DISK_SETINFO:
            dwError = SetDiskInfo(pIOReq);
            break;
        case DISK_IOCTL_READ:
        case IOCTL_DISK_READ:
        case DISK_IOCTL_WRITE:
        case IOCTL_DISK_WRITE:
        {
            PSG_REQ pUnsafeInBuf = (PSG_REQ)pIOReq->pInBuf;
            BOOL    fRead = (pIOReq->dwCode == DISK_IOCTL_READ || pIOReq->dwCode == IOCTL_DISK_READ);

            if (!pIOReq->dwInBufSize || !pIOReq->pInBuf) {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }
            
            if (pIOReq->dwInBufSize >(sizeof(SG_REQ) + ((MAX_SG_BUF) - 1) * sizeof(SG_BUF))) {
                dwError = ERROR_INVALID_PARAMETER;
                // size of m_pSterileIoRequest is sizeof(SG_REQ) + ((MAX_SG_BUF) - 1) * sizeof(SG_BUF))
                break;
            }

            // Copy the caller's SG_REQ.
            if (0 == CeSafeCopyMemory((LPVOID)m_pSterileIoRequest, (LPVOID)pUnsafeInBuf, pIOReq->dwInBufSize)) {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            if (NULL == m_pSterileIoRequest) {
                ASSERT(m_pSterileIoRequest);
                dwError = ERROR_GEN_FAILURE;
                break;
            }

            DWORD dwIndex = 0, mappedbuffers = 0;
            PUCHAR  savedoldptrs[MAX_SG_BUF] ;  // This will hold a copy of the user mode pointers that get overwritten
                                                // ValidateSg


            // Validate the SG_REQ request and map the caller's sb_bufs into
            // the sterile copy,
            if (FALSE == ValidateSg(m_pSterileIoRequest, pIOReq->dwInBufSize, fRead ? ARG_O_PTR : ARG_I_PTR,savedoldptrs)) {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            // Replace the caller's SG_REQ in the I/O request with the sterile
            // copy.
            pIOReq->pInBuf = (PBYTE)m_pSterileIoRequest;
            // Execute the I/O request.
            if (IsDMASupported()) {
                 __try {
                    dwError = ReadWriteDiskDMA(pIOReq, fRead);
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
//                    AbortDMA();
                    dwError = ERROR_INVALID_PARAMETER;
                    m_pSterileIoRequest->sr_status = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
            else {
                 // Allocate the double buffer, as required.
                if ((NULL == m_rgbDoubleBuffer) && (0 < m_pPort->m_pDskReg[m_dwDeviceId]->dwDoubleBufferSize)) {
                    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::MainIoctl> Allocating double buffer (first use)\r\n")));
                    m_rgbDoubleBuffer = (PBYTE)LocalAlloc(LPTR, m_pPort->m_pDskReg[m_dwDeviceId]->dwDoubleBufferSize);
                    if (NULL == m_rgbDoubleBuffer) {
                        DEBUGMSG(ZONE_ERROR, (TEXT("Atapi!CDisk::MainIoctl> Failed to allocate double buffer\r\n")));
                        dwError = ERROR_OUTOFMEMORY;
                        m_pSterileIoRequest->sr_status = ERROR_OUTOFMEMORY;
                        goto Cleanup;
                    }
                }
                // Perform the I/O.
                if (fRead) {
                    dwError = this->ReadDisk(pIOReq);
                }
                else {
                    dwError = this->WriteDisk(pIOReq);
                }
            }

Cleanup:
            if (FAILED(UnmapSg(
                        m_pSterileIoRequest->sr_sglist,
                        savedoldptrs,
                        m_pSterileIoRequest->sr_num_sg,
                        fRead ? ARG_O_PTR : ARG_I_PTR)))
            {
                ASSERT(!"Cleanup call to CeCloseCallerBuffer failed unexpectedly");
                dwError = ERROR_GEN_FAILURE;
            }
                    
            // Copy sr_status from the sterile SG_REQ to the caller's SG_REQ.
            __try {
                pUnsafeInBuf->sr_status = m_pSterileIoRequest->sr_status;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                dwError = ERROR_INVALID_PARAMETER;
            }
            break;
        }
        case IOCTL_DISK_GET_STORAGEID:
            dwError = GetStorageId(pIOReq);
            break;
        case DISK_IOCTL_FORMAT_MEDIA:
        case IOCTL_DISK_FORMAT_MEDIA:
            dwError = ERROR_SUCCESS;
            break;
        case IOCTL_DISK_FLUSH_CACHE:
            dwError = FlushCache();
            break;
        default:
            dwError = ERROR_NOT_SUPPORTED;
            break;
    }
    return dwError;
}

// ----------------------------------------------------------------------------
// Function: PerformIoctl
//     This is the top-most IOCTL processor and is used to trap IOCTL_POWER_
//     I/O controls to pass to the associated power management object
//
// Parameters:
//     pIOReq -
// ----------------------------------------------------------------------------

BOOL
CDisk::PerformIoctl(
    PIOREQ pIOReq
    )
{
    DWORD dwError = ERROR_SUCCESS;

    DEBUGMSG(ZONE_IOCTL, (TEXT(
        "Atapi!CDisk::PerformIoctl> IOCTL(%x), device(%x)\r\n"
        ), pIOReq->dwCode, m_dwDeviceId));

    if (pIOReq->pBytesReturned) {
        *(pIOReq->pBytesReturned) = 0;
    }

    TakeCS();
    m_pPort->TakeCS();

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_STARTIOCTL, pIOReq, sizeof(*pIOReq), 0, CELZONE_ALWAYSON, 0, FALSE);

    __try {

        if (pIOReq->dwCode == IOCTL_POWER_CAPABILITIES) {

            // instantiate DiskPower object on first use, if necessary

            if (m_pDiskPower == NULL) {
                CDiskPower *pDiskPower = GetDiskPowerInterface();
                if (pDiskPower == NULL) {
                    DEBUGMSG(ZONE_WARNING, (_T(
                        "Atapi!CDisk::PerformIoctl> Failed to create power management object\r\n"
                        )));
                }
                else if (!pDiskPower->Init(this)) {
                    DEBUGMSG(ZONE_WARNING, (_T(
                        "Atapi!CDisk::PerformIoctl> Failed to initialize power management\r\n"
                        )));
                    delete pDiskPower;
                }
                else {
                    m_pDiskPower = pDiskPower;
                }
            }
        }

        if (m_pDiskPower != NULL) {

            // is this a power IOCTL? if an exception occurs, then we'll catch
            // it below
            dwError = m_pDiskPower->DiskPowerIoctl(pIOReq);
            if (dwError != ERROR_NOT_SUPPORTED) {
                goto done;
            }

            // request that the disk spin up (if it's not up already)
            if (!m_pDiskPower->RequestDevice()) {
                // the disk is powered down
                dwError = ERROR_RESOURCE_DISABLED;
                goto done;
            }
        }

        // call the driver
        dwError = MainIoctl(pIOReq);

        // indicate we're done with the disk
        if (m_pDiskPower != NULL) {
            m_pDiskPower->ReleaseDevice();
        }

done:;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = ERROR_GEN_FAILURE;
    }

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_COMPLETEIOCTL, &dwError, sizeof(dwError), 0, CELZONE_ALWAYSON, 0, FALSE);

    m_pPort->ReleaseCS();
    ReleaseCS();

    if (dwError != ERROR_SUCCESS) {
        SetLastError(dwError);
    }

    return (ERROR_SUCCESS == dwError);
}

// ----------------------------------------------------------------------------
// Function: PostInit
//     This function facilitates backward compatibility
//
// Parameters:
//     pPostInitBuf -
// ----------------------------------------------------------------------------

BOOL
CDisk::PostInit(
    PPOST_INIT_BUF pPostInitBuf
    )
{
    DWORD dwError = ERROR_SUCCESS;

    DEBUGMSG(ZONEID_INIT, (TEXT("Atapi!CDisk::PostInit> device(%d)\r\n"), m_dwDeviceId));

    m_hDevice = pPostInitBuf->p_hDevice;

    return (dwError == ERROR_SUCCESS);
}

// ----------------------------------------------------------------------------
// Function: GetDiskInfo
//     Implement IOCTL_DISK_GETINFO
//
// Parameters:
//     pIOReq -
// ----------------------------------------------------------------------------

DWORD
CDisk::GetDiskInfo(
    PIOREQ pIOReq
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DISK_INFO *pInfo = NULL;

    // for B/C, this call has three forms; only pInBuf, only pOutBuf, or both
    // if both, then use pOutBuf

    if (pIOReq->pInBuf) {
        if (pIOReq->dwInBufSize != sizeof(DISK_INFO)) {
            return ERROR_INVALID_PARAMETER;
        }
        pInfo = (DISK_INFO *)pIOReq->pInBuf;
    }

    if (pIOReq->pOutBuf) {
        if (pIOReq->dwOutBufSize!= sizeof(DISK_INFO)) {
            return ERROR_INVALID_PARAMETER;
        }
        pInfo = (DISK_INFO *)pIOReq->pOutBuf;
    }

    if (!pInfo) {
        DEBUGMSG(ZONE_ERROR|ZONE_IOCTL, (_T(
            "Atapi!CDisk::GetDiskInfo> bad argument; pInBuf/pOutBuf null\r\n")));
        return ERROR_INVALID_PARAMETER;
    }

    // TODO: if device is ATAPI, call AtapiGetDiskInfo

    if (ERROR_SUCCESS == dwError) {
        __try {
            memcpy(pInfo, &m_DiskInfo, sizeof(DISK_INFO));
            pInfo->di_flags |= DISK_INFO_FLAG_PAGEABLE;
            pInfo->di_flags &= ~DISK_INFO_FLAG_UNFORMATTED;
            if (pIOReq->pBytesReturned){
                *(pIOReq->pBytesReturned) = sizeof(DISK_INFO);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = ERROR_INVALID_PARAMETER;
        }
    }

    return dwError;
}

// ----------------------------------------------------------------------------
// Function: SetDiskInfo
//     Implement IOCTL_DISK_SETINFO
//
// Parameters:
//     pSgReq -
//     InBufLen -
// ----------------------------------------------------------------------------

DWORD
CDisk::SetDiskInfo(
    PIOREQ pIOReq
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DISK_INFO *pInfo = (DISK_INFO *)pIOReq->pInBuf;

    if ((pIOReq->pInBuf == NULL) || (pIOReq->dwInBufSize != sizeof(DISK_INFO))) {
        return ERROR_INVALID_PARAMETER;
    }

    memcpy(&m_DiskInfo, pInfo, sizeof(DISK_INFO));

    return dwError;
}

// ----------------------------------------------------------------------------
// Function: GetDeviceInfo
//     IOCTL_DISK_DEVICE_INFO
//
// Parameters:
//     pIOReq -
// ----------------------------------------------------------------------------

DWORD
CDisk::GetDeviceInfo(
    PIOREQ pIOReq
    )
{

    if ((pIOReq->dwInBufSize == 0) || (pIOReq->pInBuf == NULL)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (pIOReq->dwInBufSize < sizeof(STORAGEDEVICEINFO)) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
        
    ASSERT(m_storagedeviceinfo.cbSize == sizeof(STORAGEDEVICEINFO));

    PSTORAGEDEVICEINFO psdi = (PSTORAGEDEVICEINFO)pIOReq->pInBuf;

    *psdi = m_storagedeviceinfo;
        
    if (pIOReq->pBytesReturned) {
        *(pIOReq->pBytesReturned) = sizeof(STORAGEDEVICEINFO);
    }

    return ERROR_SUCCESS;

}

// ----------------------------------------------------------------------------
// Function: GetDeviceInfo
//     IOCTL_DISK_DEVICE_INFO
//     This function queries the registry and must be called only in the INIT
//     routines to prevent deadlocks, as this method will end up acquiring
//     the registry lock. Otherwise the registry lock, mapped file sections
//     global lock and block driver lock can possibly deadlock. Ideally mapped file
//     sections shouldn't have a global lock. m_szDeviceKey must be set before this
//     method is called
//
// Parameters:
//     PSTORAGEDEVICEINFO -
// ----------------------------------------------------------------------------

DWORD
CDisk::GetDeviceInfo(
    PSTORAGEDEVICEINFO psdi
    )
{
    HKEY hKey;

    psdi->dwDeviceClass = 0;
    psdi->dwDeviceType = 0;
    psdi->dwDeviceFlags = 0;

    PTSTR szProfile = psdi->szProfile;

    wcscpy(szProfile, L"Default");

    if (ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, m_szDeviceKey, 0, 0, &hKey)) {
        hKey = NULL;
    }

    if (IsAtapiDevice() && IsCDRomDevice()) {

        psdi->dwDeviceClass = STORAGE_DEVICE_CLASS_MULTIMEDIA;
        psdi->dwDeviceType |= STORAGE_DEVICE_TYPE_REMOVABLE_MEDIA;
        psdi->dwDeviceType |= STORAGE_DEVICE_TYPE_ATAPI;
        psdi->dwDeviceType |= STORAGE_DEVICE_TYPE_PCIIDE;
        psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_MEDIASENSE;
        psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_READONLY;

        if (!hKey || !AtaGetRegistryString(hKey, REG_VALUE_CDPROFILE, &szProfile, sizeof(psdi->szProfile))) {
            wcscpy(psdi->szProfile, REG_VALUE_CDPROFILE);
        }

    }
    else {

        psdi->dwDeviceClass = STORAGE_DEVICE_CLASS_BLOCK;
        psdi->dwDeviceType |= STORAGE_DEVICE_TYPE_PCIIDE;
        psdi->dwDeviceType |= STORAGE_DEVICE_TYPE_ATA;
        psdi->dwDeviceFlags |= STORAGE_DEVICE_FLAG_READWRITE;

        if (!hKey || !AtaGetRegistryString(hKey, REG_VALUE_6410_CFPROFILE, &szProfile, sizeof(psdi->szProfile))) {
            wcscpy(psdi->szProfile, REG_VALUE_6410_CFPROFILE);
        }

    }

    psdi->cbSize = sizeof(STORAGEDEVICEINFO);
    return ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Function: GetDiskName
//     Implement IOCTL_DISK_GETNAME
//
// Parameters:
//     pIOReq -
// ----------------------------------------------------------------------------

DWORD
CDisk::GetDiskName(
    PIOREQ pIOReq
    )
{
    static PTCHAR szDefaultDiscDrive = (_T("External Volume"));
    PTCHAR szDiskName = NULL;
    DWORD dwSize;

    DEBUGMSG(ZONE_IOCTL, (_T("Atapi!GeDisktName\r\n")));

    if ((pIOReq->pBytesReturned == NULL) || (pIOReq->dwOutBufSize == 0) || (pIOReq->pOutBuf == NULL)) {
        return ERROR_INVALID_PARAMETER;
    }

    *(pIOReq->pBytesReturned) = 0;

    if (m_szDiskName) {
        if (wcslen(m_szDiskName)) {
            szDiskName = m_szDiskName;
        }
        else {
            return ERROR_NOT_SUPPORTED;
        }
    }
    else {
        szDiskName = szDefaultDiscDrive;
    }

    dwSize = (wcslen(szDiskName) + 1) * sizeof(TCHAR);

    if (pIOReq->dwOutBufSize < dwSize) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    wcscpy((PTCHAR) pIOReq->pOutBuf, szDiskName);

    *(pIOReq->pBytesReturned) = dwSize;

    return ERROR_SUCCESS;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
BOOL CDisk::DoRead(CSgReq* pSgReqWrapper, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesRead)
{
    DWORD dwBytesRead = 0;
    BYTE  bStatus;
    DWORD dwByteInDrqDataBlock = 0;
    BYTE  bData = 0;
    WORD  wData = 0;

    if (NULL == pSgReqWrapper) {
        ASSERT(NULL != pSgReqWrapper);
        return FALSE;
    }

    // Host_Idle
    // ---------
    // Issue the appropriate command.
    if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, m_bReadCommand)) {
        DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoRead> Failed to issue read/write command\r\n")));
        return FALSE;
    }

    // INTRQ_Wait
    // ----------
    // Wait for interrupt if nIEN=0 (i.e., if interrupt enabled).
HPIOI_INTRQ_Wait:;
    if (m_fInterruptSupported) {
        if (FALSE == WaitForInterrupt(m_dwDiskIoTimeOut) || (ATA_INTR_ERROR == CheckIntrState())) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoRead> Failed to wait for interrupt (m_dwDeviceId=%d)\r\n"), m_dwDeviceId));
            return FALSE;
        }
    }

    // Check_Status
    // ------------
    // If BSY=0 and DRQ=0, transition to Host_Idle.
    // If BSY=1, re-enter this state.
    // If BSY=0 and DRQ=1, transition to Transfer_Data.
HPIOI_Check_Status:;
    // Read the Status register.
    bStatus = GetAltStatus();
    // Test for BSY=0 and DRQ=0.
    if ((!(bStatus & 0x80)) && (!(bStatus & 0x08))) {
        ASSERT(0 == dwNumberOfSectors);
        *pdwBytesRead = dwBytesRead;
        return TRUE;
    }
    // Test for BSY=1.
    if (bStatus & 0x80) {
        goto HPIOI_Check_Status;
    }
    // Test for BSY=0, DRQ=1.
    if ((!(bStatus & 0x80)) && (bStatus & 0x08)) {
        goto HPIOI_Transfer_Data_Setup;
    }
    // Illegal status.
    ASSERT(FALSE);
    return FALSE;

    // Transfer_Data
    // -------------
    // If read Data register, DRQ data block transfer not complete, re-enter
    // this state.
    // If read Data register, all data for command transferred, transition to
    // Host_Idle.
    // If read Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=1, transition to Check_Status.
    // If read Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=0, transition to INTRQ_Wait.
HPIOI_Transfer_Data_Setup:;
    dwByteInDrqDataBlock = 0;
HPIOI_Transfer_Data:;
    // Read Data register.
    if (m_f16Bit) {
        wData = ReadWord();
        VERIFY(TRUE == pSgReqWrapper->DoWriteWord(&wData));
        dwByteInDrqDataBlock += 2;
        dwBytesRead += 2;
    }
    else {
        bData = (BYTE)ReadByte();
        VERIFY(TRUE == pSgReqWrapper->DoWriteByte(&bData));
        dwByteInDrqDataBlock += 1;
        dwBytesRead += 1;
    }
    if (0 == (dwByteInDrqDataBlock % SECTOR_SIZE)) {
        dwNumberOfSectors -= 1;
    }
    // Is the transfer complete?
    if (0 == dwNumberOfSectors) {
        *pdwBytesRead = dwBytesRead;
        return TRUE;
    }
    // The transfer is not complete.  Has a DRQ data block been transferred?
    if (dwByteInDrqDataBlock == (SECTOR_SIZE * m_bSectorsPerBlock)) {
        goto HPIOI_INTRQ_Wait;
    }
    // A DRQ data block has not been transferred.  Continue transferring.
    goto HPIOI_Transfer_Data;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
BOOL CDisk::DoRead(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesRead)
{
    DWORD dwBytesRead = 0;
    BYTE  bStatus;
    DWORD dwByteInDrqDataBlock = 0;

    if (NULL == pbBuf) {
        ASSERT(NULL != pbBuf);
        return FALSE;
    }

    // Host_Idle
    // ---------
    // Issue the appropriate command.
    if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, m_bReadCommand)) {
        DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoRead> Failed to issue read/write command\r\n")));
        return FALSE;
    }

    // INTRQ_Wait
    // ----------
    // Wait for interrupt if nIEN=0 (i.e., if interrupt enabled).
HPIOI_INTRQ_Wait:;
    if (m_fInterruptSupported) {
        if (FALSE == WaitForInterrupt(m_dwDiskIoTimeOut) || (ATA_INTR_ERROR == CheckIntrState())) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoRead> Failed to wait for interrupt (m_dwDeviceId=%d)\r\n"), m_dwDeviceId));
            return FALSE;
        }
    }

    // Check_Status
    // ------------
    // If BSY=0 and DRQ=0, transition to Host_Idle.
    // If BSY=1, re-enter this state.
    // If BSY=0 and DRQ=1, transition to Transfer_Data.
HPIOI_Check_Status:;
    // Read the Status register.
    bStatus = GetAltStatus();
    // Test for BSY=0 and DRQ=0.
    if ((!(bStatus & 0x80)) && (!(bStatus & 0x08))) {
        *pdwBytesRead = dwBytesRead;
        return TRUE;
    }
    // Test for BSY=1.
    if (bStatus & 0x80) {
        goto HPIOI_Check_Status;
    }
    // Test for BSY=0, DRQ=1.
    if ((!(bStatus & 0x80)) && (bStatus & 0x08)) {
        goto HPIOI_Transfer_Data_Setup;
    }
    // Illegal status.
    ASSERT(FALSE);
    return FALSE;

    // Transfer_Data
    // -------------
    // If read Data register, DRQ data block transfer not complete, re-enter
    // this state.
    // If read Data register, all data for command transferred, transition to
    // Host_Idle.
    // If read Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=1, transition to Check_Status.
    // If read Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=0, transition to INTRQ_Wait.
HPIOI_Transfer_Data_Setup:;
    dwByteInDrqDataBlock = 0;
HPIOI_Transfer_Data:;
    // Read Data register.
    if (m_f16Bit) {
        *((PWORD)pbBuf) = ReadWord();
        pbBuf += 2;
        dwByteInDrqDataBlock += 2;
        dwBytesRead += 2;
    }
    else {
        *((PBYTE)pbBuf) = (BYTE)ReadByte();
        pbBuf += 1;
        dwByteInDrqDataBlock += 1;
        dwBytesRead += 1;
    }
    if (0 == (dwByteInDrqDataBlock % SECTOR_SIZE))
    {
        dwNumberOfSectors -= 1;
    }
    // Is the transfer complete?
    if (0 == dwNumberOfSectors) {
        *pdwBytesRead = dwBytesRead;
        return TRUE;
    }
    // The transfer is not complete.  Has a DRQ data block been transferred?
    if (dwByteInDrqDataBlock == (SECTOR_SIZE * m_bSectorsPerBlock))
    {
        goto HPIOI_INTRQ_Wait;
    }
    // A DRQ data block has not been transferred.  Continue transferring.
    goto HPIOI_Transfer_Data;
}

BOOL CDisk::DoReadDMA(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesRead)
{
    DWORD dwBytesRead = 0;
    BYTE  bStatus;
    DWORD dwByteInDrqDataBlock = 0;
    DWORD dwEnablePDMA=FALSE;
    DWORD dwEnableUDMA=FALSE;
    DWORD dwCount;        

    if (NULL == pbBuf) {
        ASSERT(NULL != pbBuf);
        return FALSE;
    }

    if ( pbBuf == m_pDMAVirtualAddress && m_pPort->m_pDskReg[m_dwDeviceId]->dwEnableUDMA )
        dwEnableUDMA = TRUE;
    else if ( pbBuf == m_pDMAVirtualAddress && m_pPort->m_pDskReg[m_dwDeviceId]->dwEnablePDMA )
        dwEnablePDMA = TRUE;

    
    // Host_Idle
    // ---------
    // Issue the appropriate command.
    if (dwEnableUDMA )
    {
        if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, 0xc8)) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoRead> Failed to issue read/write command\r\n")));
            return FALSE;
        }
    }
    else
    {    
        if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, m_bReadCommand)) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoRead> Failed to issue read/write command\r\n")));
            return FALSE;
        }
    }

    // INTRQ_Wait
    // ----------
    // Wait for interrupt if nIEN=0 (i.e., if interrupt enabled).
HPIOI_INTRQ_Wait:;
    if (m_fInterruptSupported) {
        if (FALSE == WaitForInterrupt(m_dwDiskIoTimeOut) || (ATA_INTR_ERROR == CheckIntrState())) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoRead> Failed to wait for interrupt (m_dwDeviceId=%d)\r\n"), m_dwDeviceId));
            return FALSE;
        }
    }

    // Check_Status
    // ------------
    // If BSY=0 and DRQ=0, transition to Host_Idle.
    // If BSY=1, re-enter this state.
    // If BSY=0 and DRQ=1, transition to Transfer_Data.
HPIOI_Check_Status:;
    // Read the Status register.
    bStatus = GetAltStatus();
    // Test for BSY=0 and DRQ=0.
    if ((!(bStatus & 0x80)) && (!(bStatus & 0x08))) {
        *pdwBytesRead = dwBytesRead;
        return TRUE;
    }
    // Test for BSY=1.
    if (bStatus & 0x80) {
        goto HPIOI_Check_Status;
    }
    // Test for BSY=0, DRQ=1.
    if ((!(bStatus & 0x80)) && (bStatus & 0x08)) {
        goto HPIOI_Transfer_Data_Setup;
    }
    // Illegal status.
    ASSERT(FALSE);
    return FALSE;

    // Transfer_Data
    // -------------
    // If read Data register, DRQ data block transfer not complete, re-enter
    // this state.
    // If read Data register, all data for command transferred, transition to
    // Host_Idle.
    // If read Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=1, transition to Check_Status.
    // If read Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=0, transition to INTRQ_Wait.
HPIOI_Transfer_Data_Setup:;
    dwByteInDrqDataBlock = 0;

    // Read Data register.
    if ( dwEnableUDMA )    
    {
        DWORD dwStatus;

        /*Track Buffer 1 Setting*/
        WriteReg(ATA_TBUF_START, (UINT32)m_DMAPhyaddress.LowPart);
        WriteReg(ATA_TBUF_SIZE, dwNumberOfSectors * m_DiskInfo.di_bytes_per_sect);
        WriteReg(ATA_XFR_NUM, dwNumberOfSectors * m_DiskInfo.di_bytes_per_sect);
        
        dwCount = 0x1000000;            
        SetConfigMode(UDMA, FALSE);

        WriteReg(ATA_IRQ, 0xff);
        InterruptDone(m_pPort->m_dwSysIntr);
        WriteReg(ATA_IRQ_MASK, 0x1e);

        WaitForDeviceAccessReady();
        SetTransferCommand(ATA_CMD_START); // trigger PDMA

        dwStatus = WaitForSingleObject(m_pPort->m_hIRQEvent, dwNumberOfSectors * 100);
        if (dwStatus == WAIT_TIMEOUT) 
        {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("Atapi!CDisk::DoReadDMA> Wait Time out at Interrupt\n")));
            return FALSE;
        }
        
        SetConfigMode(PIO_CPU, FALSE);

        WriteReg(ATA_IRQ, ReadReg(ATA_IRQ) | 0x1);

        WriteReg(ATA_CFG, ReadReg(ATA_CFG) & ~(0x200));

        *pdwBytesRead = dwNumberOfSectors * m_DiskInfo.di_bytes_per_sect;
        return TRUE;
    }
    
    else if ( dwEnablePDMA )
    {
        WriteReg(ATA_IRQ, 0xff);
        /*Track Buffer 1 Setting*/
        WriteReg(ATA_TBUF_START, (UINT32)m_DMAPhyaddress.LowPart + dwBytesRead);
        WriteReg(ATA_TBUF_SIZE, m_DiskInfo.di_bytes_per_sect);
        WriteReg(ATA_XFR_NUM, m_DiskInfo.di_bytes_per_sect);
        
        dwCount = 0x10000;            
        SetConfigMode(PIO_DMA, FALSE);

        WaitForDeviceAccessReady();
        SetTransferCommand(ATA_CMD_START); // trigger PMDA
        
        while ( dwCount-- )
        {
            if (ReadReg(ATA_IRQ) & 0x1)
                break;
        }

        if (dwCount <= 0)
        {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("Atapi!CDisk::DoReadDMA> Wait Time out\n")));
            return FALSE;
        }

        SetConfigMode(PIO_CPU, FALSE);

        WriteReg(ATA_IRQ, ReadReg(ATA_IRQ) | 0x1);

        dwNumberOfSectors--;
        dwBytesRead += SECTOR_SIZE;
    
        if ( dwNumberOfSectors == 0 )
        {
            *pdwBytesRead = dwBytesRead;
            return TRUE;
        }

        goto HPIOI_INTRQ_Wait;    
    }
    goto HPIOI_Check_Status;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
DWORD CDisk::ReadDisk(PIOREQ pIoReq)
{
    DWORD   m_dwDblBufSectorCount = 0;
    PSG_REQ pSgReq;
    CSgReq  SgReqWrapper;
    DWORD   dwStartingSector;
    DWORD   dwSectorsRemaining;
    DWORD   dwNumberOfSectors;
    DWORD   dwAbsoluteBufferLength;
    DWORD   dwBytesRead = 0;
    DWORD   dwBytesWritten = 0;

    PSG_BUF pSgBuf; 

    DEBUGCHK(NULL != pIoReq);

    // Unpack the Scatter/Gather request.
    pSgReq = (PSG_REQ)pIoReq->pInBuf;

    // Ensure the Scatter/Gather request does not extend past the end of the
    // disk.
    if ((pSgReq->sr_start + pSgReq->sr_num_sec) > m_DiskInfo.di_total_sectors) {
        return ERROR_SECTOR_NOT_FOUND;
    }

    // Attach the Scatter/Gather request to the wrapper.
    if (FALSE == SgReqWrapper.DoAttach(pSgReq, SECTOR_SIZE)) {
        return ERROR_GEN_FAILURE;
    }

    // Prepare the transfer.
    dwStartingSector = SgReqWrapper.GetStartingSector();
    dwSectorsRemaining = SgReqWrapper.GetNumberOfSectors();
    dwNumberOfSectors = 0;
    dwAbsoluteBufferLength = SgReqWrapper.GetAbsoluteBufferLength();

    pSgBuf = &(pSgReq->sr_sglist[0]);

    if ( NULL != m_rgbDoubleBuffer) {
        m_dwDblBufSectorCount = (m_pPort->m_pDskReg[m_dwDeviceId]->dwDoubleBufferSize / SECTOR_SIZE); // dwDrqDataBlockSize
        // Fill the double buffer from the disk and copy to the Scatter/Gather
        // buffer list.
        while (SgReqWrapper.GetAbsoluteBufferPosition() < dwAbsoluteBufferLength) {
            // Fill the double buffer from the disk.
            dwNumberOfSectors = MIN(m_dwDblBufSectorCount, dwSectorsRemaining);

            if (FALSE == this->DoRead(m_rgbDoubleBuffer, dwStartingSector, dwNumberOfSectors, &dwBytesRead)) 
            {
                return ERROR_GEN_FAILURE;
            }

            ASSERT(dwBytesRead == (SECTOR_SIZE * dwNumberOfSectors));
            // Write to the Scatter/Gather buffer list.

            if (FALSE == SgReqWrapper.DoWriteMultiple(m_rgbDoubleBuffer, dwBytesRead, &dwBytesWritten)) {
                return ERROR_GEN_FAILURE;
            }
            ASSERT(dwBytesWritten == dwBytesRead);
            dwStartingSector += dwNumberOfSectors;
            dwSectorsRemaining -= dwNumberOfSectors;
        }
    }
    else {
        // Fill the Scatter/Gather buffer list from the disk.
        while (SgReqWrapper.GetAbsoluteBufferPosition() < dwAbsoluteBufferLength) {
            dwNumberOfSectors = MIN(256, dwSectorsRemaining);
            if (FALSE == this->DoRead(&SgReqWrapper, dwStartingSector, dwNumberOfSectors, &dwBytesRead)) {
                return ERROR_GEN_FAILURE;
            }
            ASSERT(dwBytesRead == (SECTOR_SIZE * dwNumberOfSectors));
            dwStartingSector += dwNumberOfSectors;
            dwSectorsRemaining -= dwNumberOfSectors;
        }
    }


    *pIoReq->pBytesReturned = dwBytesRead;
    return ERROR_SUCCESS;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
BOOL CDisk::DoWrite(CSgReq* pSgReqWrapper, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesWritten)
{
    DWORD dwBytesWritten = 0;
    BYTE  bStatus;
    DWORD dwByteInDrqDataBlock = 0;
    BOOL  fComplete = FALSE;
    BYTE  bData = 0;
    WORD  wData = 0;

    if (NULL == pSgReqWrapper) {
        ASSERT(NULL != pSgReqWrapper);
        return FALSE;
    }

    // Host_Idle
    // ---------
    // Issue the appropriate command.
    if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, m_bWriteCommand)) {
        DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoWrite> Failed to issue read/write command\r\n")));
        return FALSE;
    }

    // Check_Status
    // ------------
    // If BSY=0 and DRQ=0, transition to Host_Idle.
    // If BSY=1, re-enter this state.
    // If BSY=0 and DRQ=1, transition to Transfer_Data.
HPIOO_Check_Status:;
    // Read the Status register.
    bStatus = GetAltStatus();
    // Test for BSY=0 and DRQ=0.
    if ((!(bStatus & 0x80)) && (!(bStatus & 0x08))) {
        if (fComplete) {
            return TRUE;
        }
    }
    // Test for BSY=1.
    if (bStatus & 0x80) {
        goto HPIOO_Check_Status;
    }
    // Test for BSY=0 and DRQ=1.
    if ((!(bStatus & 0x80)) && (bStatus & 0x08)) {
        goto HPIOO_Transfer_Data_Reset_DRQ_Data_Block;
    }
    // Illegal status.
    ASSERT(FALSE);
    return FALSE;

    // Transfer_Data
    // -------------
    // If write Data register, DRQ data block transfer not complete, re-enter
    // this state.
    // If write Data register, all data for command transferred, transition to
    // Host_Idle.
    // If write Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=1, transition to Check_Status.
    // If write Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=0, transition to INTRQ_Wait.
HPIOO_Transfer_Data_Reset_DRQ_Data_Block:;
    dwByteInDrqDataBlock = 0;
HPIOO_Transfer_Data:;
    // Write to the Data register.
    if (m_f16Bit) {
        VERIFY(TRUE == pSgReqWrapper->DoReadWord(&wData));
        WriteWord(wData);
        dwByteInDrqDataBlock += 2;
        dwBytesWritten += 2;
    }
    else {
        VERIFY(TRUE == pSgReqWrapper->DoReadByte(&bData));
        WriteByte(bData);
        dwByteInDrqDataBlock += 1;
        dwBytesWritten += 1;
    }
    if (0 == (dwByteInDrqDataBlock % SECTOR_SIZE)) {
        dwNumberOfSectors -= 1;
    }
    // Is the transfer complete?
    if (0 == dwNumberOfSectors) {
        *pdwBytesWritten = dwBytesWritten;
        fComplete = TRUE;
        goto HPIOO_INTRQ_Wait;
    }
    // The transfer is not complete.  Has a DRQ data block been transferred?
    if (dwByteInDrqDataBlock == (SECTOR_SIZE * m_bSectorsPerBlock)) {
        goto HPIOO_INTRQ_Wait;
    }
    // A DRQ data block has not been transferred.  Continue transferring.
    goto HPIOO_Transfer_Data;

    // INTRQ_Wait
    // ----------
    // Wait for interrupt if nIEN=0 (i.e., if interrupt enabled).
HPIOO_INTRQ_Wait:;
    if (m_fInterruptSupported) {
        if (FALSE == WaitForInterrupt(m_dwDiskIoTimeOut) || (CheckIntrState() == ATA_INTR_ERROR)) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoWrite> Failed to wait for interrupt (m_dwDeviceId=%d)\r\n"), m_dwDeviceId));
            return FALSE;
        }
    }
    goto HPIOO_Check_Status;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
BOOL CDisk::DoWrite(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesWritten)
{
    DWORD dwBytesWritten = 0;
    BYTE  bStatus;
    DWORD dwByteInDrqDataBlock = 0;
    BOOL  fComplete = FALSE;

    // Host_Idle
    // ---------
    // Issue the appropriate command.
    if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, m_bWriteCommand)) {
        DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoWrite> Failed to issue read/write command\r\n")));
        return FALSE;
    }
    // Check_Status
    // ------------
    // If BSY=0 and DRQ=0, transition to Host_Idle.
    // If BSY=1, re-enter this state.
    // If BSY=0 and DRQ=1, transition to Transfer_Data.
HPIOO_Check_Status:;
    // Read the Status register.
    bStatus = GetAltStatus();
    // Test for BSY=0 and DRQ=0.
    if ((!(bStatus & 0x80)) && (!(bStatus & 0x08))) {
        if (fComplete) {
            return TRUE;
        }
    }
    // Test for BSY=1.
    if (bStatus & 0x80) {
        goto HPIOO_Check_Status;
    }
    // Test for BSY=0 and DRQ=1.
    if ((!(bStatus & 0x80)) && (bStatus & 0x08)) {
        goto HPIOO_Transfer_Data_Reset_DRQ_Data_Block;
    }
    DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("Atapi!CDisk::DoWrite> Illegal Status\r\n")));    
    // Illegal status.
    ASSERT(FALSE);
    return FALSE;

    // Transfer_Data
    // -------------
    // If write Data register, DRQ data block transfer not complete, re-enter
    // this state.
    // If write Data register, all data for command transferred, transition to
    // Host_Idle.
    // If write Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=1, transition to Check_Status.
    // If write Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=0, transition to INTRQ_Wait.
HPIOO_Transfer_Data_Reset_DRQ_Data_Block:;
    dwByteInDrqDataBlock = 0;
HPIOO_Transfer_Data:;
    // Write to the Data register.
    if (m_f16Bit) {
        WriteWord(*((PWORD)pbBuf));
        pbBuf += 2;
        dwByteInDrqDataBlock += 2;
        dwBytesWritten += 2;
    }
    else {
        WriteByte(*((PBYTE)pbBuf));
        pbBuf += 1;
        dwByteInDrqDataBlock += 1;
        dwBytesWritten += 1;
    }
    if (0 == (dwByteInDrqDataBlock % SECTOR_SIZE)) {
        dwNumberOfSectors -= 1;
    }
    // Is the transfer complete?
    if (0 == dwNumberOfSectors) {
        *pdwBytesWritten = dwBytesWritten;
        fComplete = TRUE;
        goto HPIOO_INTRQ_Wait;
    }
    // The transfer is not complete.  Has a DRQ data block been transferred?
    if (dwByteInDrqDataBlock == (SECTOR_SIZE * m_bSectorsPerBlock)) {
        goto HPIOO_INTRQ_Wait;
    }
    // A DRQ data block has not been transferred.  Continue transferring.
    goto HPIOO_Transfer_Data;

    // INTRQ_Wait
    // ----------
    // Wait for interrupt if nIEN=0 (i.e., if interrupt enabled).
HPIOO_INTRQ_Wait:;
    if (m_fInterruptSupported) {
        if (FALSE == WaitForInterrupt(m_dwDiskIoTimeOut) || (CheckIntrState() == ATA_INTR_ERROR)) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoWrite> Failed to wait for interrupt (m_dwDeviceId=%d)\r\n"), m_dwDeviceId));
            return FALSE;
        }
    }
    goto HPIOO_Check_Status;
}

BOOL CDisk::DoWriteDMA(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesWritten)
{
    DWORD dwBytesWritten = 0;
    BYTE  bStatus;
    DWORD dwByteInDrqDataBlock = 0;
    BOOL  fComplete = FALSE;

    DWORD dwEnableUDMA=FALSE;
    DWORD dwEnablePDMA=FALSE;
    DWORD dwCount;    

    // Host_Idle
    // ---------
    // Issue the appropriate command.
    if ( (pbBuf == m_pDMAVirtualAddress) && m_pPort->m_pDskReg[m_dwDeviceId]->dwEnableUDMA )
        dwEnableUDMA = TRUE;
    else if ( (pbBuf == m_pDMAVirtualAddress) && m_pPort->m_pDskReg[m_dwDeviceId]->dwEnablePDMA )
        dwEnablePDMA = TRUE;

    if ( dwEnableUDMA == TRUE )
    {
        if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, 0xca)) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoWrite> Failed to issue read/write command\r\n")));
            return FALSE;
        }
    }
    else
    {
        if (FALSE == SendIOCommand(dwStartingSector, dwNumberOfSectors, m_bWriteCommand)) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoWrite> Failed to issue read/write command\r\n")));
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("Atapi!CDisk::DoWriteDMA> Failed to issue read/write command\r\n")));
            return FALSE;
        }
    }
    // Check_Status
    // ------------
    // If BSY=0 and DRQ=0, transition to Host_Idle.
    // If BSY=1, re-enter this state.
    // If BSY=0 and DRQ=1, transition to Transfer_Data.
HPIOO_Check_Status:;
    // Read the Status register.
    bStatus = GetAltStatus();
    // Test for BSY=0 and DRQ=0.
    if ((!(bStatus & 0x80)) && (!(bStatus & 0x08))) {
        if (fComplete) {
            return TRUE;
        }
    }
    // Test for BSY=1.
    if (bStatus & 0x80) {
        goto HPIOO_Check_Status;
    }
    // Test for BSY=0 and DRQ=1.
    if ((!(bStatus & 0x80)) && (bStatus & 0x08)) {
        goto HPIOO_Transfer_Data_Reset_DRQ_Data_Block;
    }
    DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("Atapi!CDisk::DoWriteDMA> Illegal Status\r\n")));    
    // Illegal status.
    ASSERT(FALSE);
    return FALSE;

    // Transfer_Data
    // -------------
    // If write Data register, DRQ data block transfer not complete, re-enter
    // this state.
    // If write Data register, all data for command transferred, transition to
    // Host_Idle.
    // If write Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=1, transition to Check_Status.
    // If write Data register, DRQ data block transferred, all data for command
    // not transferred, and nIEN=0, transition to INTRQ_Wait.
HPIOO_Transfer_Data_Reset_DRQ_Data_Block:;
    dwByteInDrqDataBlock = 0;

    // Write to the Data register.
    if ( dwEnableUDMA == TRUE )
    {
        DWORD dwStatus;
        WriteReg(ATA_IRQ, 0xff);
        /*Track Buffer 1 Setting*/
        WriteReg(ATA_SBUF_START, (UINT32)m_DMAPhyaddress.LowPart);
        WriteReg(ATA_SBUF_SIZE, dwNumberOfSectors * m_DiskInfo.di_bytes_per_sect);
        WriteReg(ATA_XFR_NUM, dwNumberOfSectors * m_DiskInfo.di_bytes_per_sect);
        
        dwCount = 0x1000000;            
        SetConfigMode(UDMA, TRUE);

        WriteReg(ATA_IRQ, 0xff);
        InterruptDone(m_pPort->m_dwSysIntr);
        WriteReg(ATA_IRQ_MASK, 0x1e);
        
        WaitForDeviceAccessReady();
        SetTransferCommand(ATA_CMD_START); // trigger PMDA

        dwStatus = WaitForSingleObject(m_pPort->m_hIRQEvent, dwNumberOfSectors * 10);
        if (dwStatus == WAIT_TIMEOUT) 
        {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("Atapi!CDisk::DoWriteDMA> Wait Time out at Interrupt########\n")));
            return FALSE;
        }

        SetConfigMode(PIO_CPU, FALSE);

        WriteReg(ATA_IRQ, ReadReg(ATA_IRQ) | 0x1);

        *pdwBytesWritten = dwNumberOfSectors * m_DiskInfo.di_bytes_per_sect;
        fComplete = TRUE;
        
        goto HPIOO_INTRQ_Wait;        
    }
    else if ( dwEnablePDMA == TRUE )
    {

        WriteReg(ATA_IRQ, 0xff);
        /*Track Buffer 1 Setting*/
        WriteReg(ATA_SBUF_START, (UINT32)m_DMAPhyaddress.LowPart + (UINT32)dwBytesWritten);
        WriteReg(ATA_SBUF_SIZE, m_DiskInfo.di_bytes_per_sect);
        WriteReg(ATA_XFR_NUM, m_DiskInfo.di_bytes_per_sect);
        
        dwCount = 0x10000;            
        SetConfigMode(PIO_DMA, TRUE);

        WaitForDeviceAccessReady();
        SetTransferCommand(ATA_CMD_START); // trigger PMDA
        
        while ( dwCount-- )
        {
            if (ReadReg(ATA_IRQ) & 0x1)
                break;
        }

        if (dwCount <= 0)
        {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("Atapi!CDisk::DoWriteDMA> Wait Time out\n")));
            return FALSE;
        }

        SetConfigMode(PIO_CPU, FALSE);


        WriteReg(ATA_IRQ, ReadReg(ATA_IRQ) | 0x1);  // clear trans_done status bit.

        dwNumberOfSectors -= 1;
        dwBytesWritten += m_DiskInfo.di_bytes_per_sect;

        if (0 == dwNumberOfSectors) {
            *pdwBytesWritten = dwBytesWritten;
            fComplete = TRUE;
        }
        
        goto HPIOO_INTRQ_Wait;
        
    }

    // INTRQ_Wait
    // ----------
    // Wait for interrupt if nIEN=0 (i.e., if interrupt enabled).
HPIOO_INTRQ_Wait:;
    if (m_fInterruptSupported) {
        if (FALSE == WaitForInterrupt(m_dwDiskIoTimeOut) || (CheckIntrState() == ATA_INTR_ERROR)) {
            DEBUGMSG(ZONE_ERROR, (_T("Atapi!CDisk::DoWrite> Failed to wait for interrupt (m_dwDeviceId=%d)\r\n"), m_dwDeviceId));
            return FALSE;
        }
    }
    goto HPIOO_Check_Status;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
DWORD CDisk::WriteDisk(PIOREQ pIoReq)
{
    DWORD   m_dwDblBufSectorCount = 0;
    PSG_REQ pSgReq;
    CSgReq  SgReqWrapper;
    DWORD   dwStartingSector;
    DWORD   dwSectorsRemaining;
    DWORD   dwNumberOfSectors;
    DWORD   dwAbsoluteBufferLength;
    DWORD   dwBytesRead = 0;
    DWORD   dwBytesWritten = 0;

    PSG_BUF pSgBuf; 

    DEBUGCHK(NULL != pIoReq);

    // Unpack the Scatter/Gather request.
    pSgReq = (PSG_REQ)pIoReq->pInBuf;

    // Ensure the Scatter/Gather request does not extend past the end of the
    // disk.
    if ((pSgReq->sr_start + pSgReq->sr_num_sec) > m_DiskInfo.di_total_sectors) {
        return  ERROR_SECTOR_NOT_FOUND;
    }

    // Attach the Scatter/Gather request to the wrapper.
    if (FALSE == SgReqWrapper.DoAttach(pSgReq, SECTOR_SIZE)) {
        return ERROR_GEN_FAILURE;
    }

    // Prepare the transfer.
    dwStartingSector = SgReqWrapper.GetStartingSector();
    dwSectorsRemaining = SgReqWrapper.GetNumberOfSectors();
    dwNumberOfSectors = 0;
    dwAbsoluteBufferLength = SgReqWrapper.GetAbsoluteBufferLength();

    pSgBuf = &(pSgReq->sr_sglist[0]);

    // Is double buffering required?
    if ( NULL != m_rgbDoubleBuffer ) {
        m_dwDblBufSectorCount = (m_pPort->m_pDskReg[m_dwDeviceId]->dwDoubleBufferSize / SECTOR_SIZE); // dwDrqDataBlockSize
        // Fill the double buffer from the Scatter/Gather buffer list and write
        // to disk.
        while (SgReqWrapper.GetAbsoluteBufferPosition() < dwAbsoluteBufferLength) {
            // Fill the double buffer from the Scatter/Gather buffer list.
            dwNumberOfSectors = MIN(m_dwDblBufSectorCount, dwSectorsRemaining);

            if (FALSE == SgReqWrapper.DoReadMultiple(m_rgbDoubleBuffer, SECTOR_SIZE * dwNumberOfSectors, &dwBytesRead)) {
                return ERROR_GEN_FAILURE;
            }
            
            ASSERT(dwBytesRead == (SECTOR_SIZE * dwNumberOfSectors));
            // Write to disk.
            if (FALSE == this->DoWrite(m_rgbDoubleBuffer, dwStartingSector, dwNumberOfSectors, &dwBytesWritten)) {
                return ERROR_GEN_FAILURE;
            }
            ASSERT(dwBytesRead == dwBytesWritten);
            dwStartingSector += dwNumberOfSectors;
            dwSectorsRemaining -= dwNumberOfSectors;
        }
    }
    else {
        // Empty the Scatter/Gather buffer list to the disk.
        while (SgReqWrapper.GetAbsoluteBufferPosition() < dwAbsoluteBufferLength) {
            dwNumberOfSectors = MIN(256, dwSectorsRemaining);
            if (FALSE == this->DoWrite(&SgReqWrapper, dwStartingSector, dwNumberOfSectors, &dwBytesWritten)) {
                return ERROR_GEN_FAILURE;
            }
            ASSERT(dwBytesWritten == (SECTOR_SIZE * dwNumberOfSectors));
            dwStartingSector += dwNumberOfSectors;
            dwSectorsRemaining -= dwNumberOfSectors;
        }
    }

    
    *pIoReq->pBytesReturned = dwBytesWritten;
    return ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Function: ReadWriteDiskDMA
//     This function reads from/writes to an ATA device
//
// Parameters:
//     pIOReq -
//     fRead -
// ----------------------------------------------------------------------------

DWORD
CDisk::ReadWriteDiskDMA(
    PIOREQ pIOReq,
    BOOL fRead
    )
{
    DWORD   m_dwDblBufSectorCount = 0;
    PSG_REQ pSgReq;
    CSgReq  SgReqWrapper;
    DWORD   dwStartingSector;
    DWORD   dwSectorsRemaining;
    DWORD   dwNumberOfSectors;
    DWORD   dwAbsoluteBufferLength;
    DWORD   dwBytesRead = 0;
    DWORD   dwBytesWritten = 0;

    PSG_BUF pSgBuf; 

    DEBUGCHK(NULL != pIOReq);

    // Unpack the Scatter/Gather request.
    pSgReq = (PSG_REQ)pIOReq->pInBuf;

    // Ensure the Scatter/Gather request does not extend past the end of the
    // disk.
    if ((pSgReq->sr_start + pSgReq->sr_num_sec) > m_DiskInfo.di_total_sectors) {
        return ERROR_SECTOR_NOT_FOUND;
    }

    // Attach the Scatter/Gather request to the wrapper.
    if (FALSE == SgReqWrapper.DoAttach(pSgReq, SECTOR_SIZE)) {
        return ERROR_GEN_FAILURE;
    }

    // Prepare the transfer.
    dwStartingSector = SgReqWrapper.GetStartingSector();
    dwSectorsRemaining = SgReqWrapper.GetNumberOfSectors();
    dwNumberOfSectors = 0;
    dwAbsoluteBufferLength = SgReqWrapper.GetAbsoluteBufferLength();

    pSgBuf = &(pSgReq->sr_sglist[0]);

    // Is double buffering required?
    m_dwDblBufSectorCount = (m_pPort->m_pDskReg[m_dwDeviceId]->dwDoubleBufferSize / SECTOR_SIZE); // dwDrqDataBlockSize

    while (SgReqWrapper.GetAbsoluteBufferPosition() < dwAbsoluteBufferLength)
    {
        dwNumberOfSectors = MIN(m_dwDblBufSectorCount, dwSectorsRemaining);
        if (fRead)
        {
            if ( (m_pDMAVirtualAddress!=NULL))
            {
                if (FALSE == this->DoReadDMA(m_pDMAVirtualAddress, dwStartingSector, dwNumberOfSectors, &dwBytesRead))
                {
                    return ERROR_GEN_FAILURE;
                }
            }
            else
            {
                return ERROR_GEN_FAILURE;
            }

            ASSERT(dwBytesRead == (SECTOR_SIZE * dwNumberOfSectors));
            // Write to the Scatter/Gather buffer list.

            if ( (m_pDMAVirtualAddress!=NULL))
            {
                if (FALSE == SgReqWrapper.DoWriteMultiple(m_pDMAVirtualAddress, dwBytesRead, &dwBytesWritten))
                {
                    return ERROR_GEN_FAILURE;
                }
            }
            else
            {
                return ERROR_GEN_FAILURE;
            }
        }
        else
        {
            if ( (m_pDMAVirtualAddress != NULL) )
            {
                if (FALSE == SgReqWrapper.DoReadMultiple(m_pDMAVirtualAddress, SECTOR_SIZE * dwNumberOfSectors, &dwBytesRead))
                {
                    return ERROR_GEN_FAILURE;
                }
            }
            else
            {
                return ERROR_GEN_FAILURE;
            }
            
            ASSERT(dwBytesRead == (SECTOR_SIZE * dwNumberOfSectors));
            // Write to disk.
            if ( (m_pDMAVirtualAddress!=NULL) )
            {
                if (FALSE == this->DoWriteDMA(m_pDMAVirtualAddress, dwStartingSector, dwNumberOfSectors, &dwBytesWritten))
                {
                    return ERROR_GEN_FAILURE;
                }            
            }
            else
            {
                return ERROR_GEN_FAILURE;
            }
        }
        ASSERT(dwBytesWritten == dwBytesRead);
        dwStartingSector += dwNumberOfSectors;
        dwSectorsRemaining -= dwNumberOfSectors;
    }
    *pIOReq->pBytesReturned = dwBytesRead;
    return ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Function: GetStorageId
//     Implement IOCTL_DISK_GET_STORAGEID
//
// Parameters:
//     pIOReq -
// ----------------------------------------------------------------------------

DWORD
CDisk::GetStorageId(
    PIOREQ pIOReq
    )
{
    DWORD dwBytesLeft;
    PBYTE pDstOffset;
    PSTORAGE_IDENTIFICATION pStorageId = (PSTORAGE_IDENTIFICATION)pIOReq->pOutBuf;

    // validate arguments
    if (!pStorageId || (pIOReq->dwOutBufSize < sizeof(STORAGE_IDENTIFICATION)) || !pIOReq->pBytesReturned) {
        return ERROR_INVALID_PARAMETER;
    }

    // prepare return structure
    pStorageId->dwSize = sizeof(STORAGE_IDENTIFICATION);
    pStorageId->dwFlags = 0; // {MANUFACTUREID,SERIALNUM}_INVALID

    // prepare return structure indicies, for write
    dwBytesLeft = pIOReq->dwOutBufSize - sizeof(STORAGE_IDENTIFICATION);
    pDstOffset = (PBYTE)(pStorageId + 1);
    pStorageId->dwManufactureIDOffset = pDstOffset - (PBYTE)pStorageId;

    SetLastError(ERROR_SUCCESS);

    // fetch manufacturer ID
    if (!ATAParseIdString((PBYTE)m_Id.ModelNumber, sizeof(m_Id.ModelNumber), &(pStorageId->dwManufactureIDOffset), &pDstOffset, &dwBytesLeft)) {
        pStorageId->dwFlags |= MANUFACTUREID_INVALID;
    }
    pStorageId->dwSerialNumOffset = pDstOffset - (PBYTE)pStorageId;
    // fetch serial number
    if (!ATAParseIdString((PBYTE)m_Id.SerialNumber, sizeof(m_Id.SerialNumber), &(pStorageId->dwSerialNumOffset), &pDstOffset, &dwBytesLeft)) {
        pStorageId->dwFlags |= SERIALNUM_INVALID;
    }
    pStorageId->dwSize = pDstOffset - (PBYTE)pStorageId;

    // store bytes written
    *(pIOReq->pBytesReturned)= min(pStorageId->dwSize, pIOReq->dwOutBufSize);

    return GetLastError();
}

// ----------------------------------------------------------------------------
// Function: SetWriteCacheMode
//     Issue SET FEATURES enable write cache command
//
// Parameters:
//     fEnable -
// ----------------------------------------------------------------------------

BOOL
CDisk::SetWriteCacheMode(
    BOOL fEnable
    )
{
    BYTE bError, bStatus;

    // select device
    SelectDevice();

    // wait for device to acknowledge selection
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 100);
    WaitForDisc(WAIT_TYPE_READY, 1000);
    WaitOnBusy(TRUE);

    // write command
    SelectDevice();
    WriteFeature(fEnable ? ATA_ENABLE_WRITECACHE : ATA_DISABLE_WRITECACHE);
    WriteCommand(ATAPI_CMD_SET_FEATURES);

    // wait for device to respond to command
    SelectDevice();
    WaitOnBusy(TRUE);
    SelectDevice();
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 200);

    // check response
    bStatus = GetBaseStatus();
    bError = GetError();
    if ((bStatus & ATA_STATUS_ERROR) && (bError & ATA_ERROR_ABORTED)) {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::SetWriteCacheMode> Failed to enable write cache; status(%02X), error(%02X)\r\n"
            ), bStatus, bError));
        ResetController(FALSE);
        return FALSE;
    }

    return TRUE;
}

// ----------------------------------------------------------------------------
// Function: SetLookAhead
//     Issue SET FEATURES enable read look-ahead command
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL
CDisk::SetLookAhead(
    )
{
    BYTE bError = 0;
    BYTE bStatus = 0;

    // select device
    SelectDevice();

    // wait for device to acknowledge selection
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 100);
    WaitForDisc(WAIT_TYPE_READY, 1000);
    WaitOnBusy(TRUE);

    // write command
    SelectDevice();
    WriteFeature(ATA_ENABLE_LOOKAHEAD);
    WriteCommand(ATAPI_CMD_SET_FEATURES);

    // wait for device to respond to command
    SelectDevice();
    WaitOnBusy(TRUE);
    SelectDevice();
    WaitForDisc(WAIT_TYPE_NOT_BUSY, 200);

    // check response
    bStatus = GetBaseStatus();
    bError = GetError();
    if ((bStatus & ATA_STATUS_ERROR) && (bError & ATA_ERROR_ABORTED)) {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::SetLookAhead> Failed to enable read look-ahead; status(%02X), error(%02X)\r\n"
            ), bStatus, bError));
        ResetController(FALSE);
        return FALSE;
    }

    return TRUE;
}

// ----------------------------------------------------------------------------
// Function: FlushCache
//     Issue FLUSH CACHE command; this command may take up to 30s to complete
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

DWORD
CDisk::FlushCache(
    )
{
    BYTE bError = 0;
    BYTE bStatus = 0;
    BOOL fOk = TRUE;

    // if write cache is not enabled, then fail
    if (!(m_dwDeviceFlags & DFLAGS_USE_WRITE_CACHE)) {
        return ERROR_NOT_SUPPORTED;
    }

    // HI:Check_Status (Host Idle); wait until BSY=0 and DRQ=0
    // read Status register
    while (1) {
        bStatus = GetAltStatus();
        if (!(bStatus & (0x80|0x08))) break; // BSY := Bit 7, DRQ := Bit 3
        Sleep(5);
    }

    // HI:Device_Select; select device
    SelectDevice();

    // HI:Check_Status (Host Idle); wait until BSY=0 and DRQ=0
    // read Status register
    while (1) {
        bStatus = GetAltStatus();
        if (!(bStatus & (0x80|0x08))) break; // BSY := Bit 7, DRQ := Bit 3
        Sleep(5);
    }

    // HI:Write_Command
    WriteCommand(ATA_CMD_FLUSH_CACHE);

    // transition to non-data command protocol

    // HND:INTRQ_Wait
    // transition to HND:Check_Status
    // read Status register
    while (1) { // BSY := Bit 7
        bStatus = GetAltStatus();
        bError = GetError();
        if (bError & 0x04) { // ABRT := Bit 2
            // command was aborted
            DEBUGMSG(ZONE_ERROR, (_T(
                "Atapi!CDisk::FlushCache> Failed to send FLUSH CACHE\r\n"
                )));
            fOk = FALSE;
            break;
        }
        if (!(bStatus & 0x80)) break; // BSY := Bit 7
        Sleep(5);
    }

    // transition to host idle protocol

    return (fOk ? ERROR_SUCCESS : ERROR_GEN_FAILURE);
}

BOOL
CDisk::WaitForDeviceAccessReady(
    )
{
    UINT32 tempRead;
    UINT8  retVal=TRUE;
    UINT32 count=1;

    do {
        //Inp32(ATA_FIFO_STATUS, tempRead); // modified by Bryan W. Lee (Oct.19th, 2005)
        if ( count == 10000000)
        {
            retVal=FALSE;
            DEBUGMSG(ZONE_ERROR, (_T(
                "Atapi!CDisk::FlushCache> Failed to Wait For Device Access Ready\r\n"
                )));
            break;
        }
        count++;
        tempRead = *((UINT32 *)(m_pATAReg + ATA_FIFO_STATUS));
    } while((tempRead>>28)!=0);
    return retVal;
}

BYTE 
CDisk::ATA_READ_BYTE(PBYTE p)
{
    BYTE ret;

    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to read byte\r\n"
        )));
    }
    ret = READ_PORT_UCHAR(p);
    
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to read byte\r\n"
        )));
    }
    ret = ReadByteFromRDATA();
    
    return ret;
}


USHORT 
CDisk::ATA_READ_WORD(PUSHORT p)
{
    WORD ret;
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to read word\r\n"
        )));
    }    
    ret = READ_PORT_USHORT(p);
    
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to read word\r\n"
        )));
    }
    ret = ReadWordFromRDATA();
    
    return ret;
}

ULONG 
CDisk::ATA_READ_DWORD(PULONG p)
{
    ULONG ret;
    
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to read dword\r\n"
        )));
    }    
    ret = READ_PORT_ULONG(p);
    
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to read dword\r\n"
        )));
    }
    ret = ReadDWordFromRDATA();
    
    return ret;
}

void
CDisk::ATA_WRITE_BYTE(PBYTE p,BYTE v)
{
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to write byte\r\n"
        )));
    }
    
    WRITE_PORT_UCHAR(p,v);
    
}

void
CDisk::ATA_WRITE_WORD(PUSHORT p,USHORT v)
{
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to write word\r\n"
        )));
    }

    WRITE_PORT_USHORT(p,v);
}

void
CDisk::ATA_WRITE_DWORD(PULONG p,ULONG v)
{
    if (!WaitForDeviceAccessReady())
    {
        DEBUGMSG(ZONE_ERROR, (_T(
            "Atapi!CDisk::FlushCache> Failed to write dword\r\n"
        )));
    }

    WRITE_PORT_ULONG(p,v);
}

void 
CDisk::ReadByteBuffer(PBYTE pBuffer, DWORD dwCount)
{
    for (DWORD i = 0 ; i < dwCount ; i++ )
    {    
        *(pBuffer+i) = ATA_READ_BYTE(m_pATAReg + ATA_PIO_DTR);
    }
}

void 
CDisk::ReadWordBuffer(PWORD pBuffer, DWORD dwCount)
{
    for (DWORD i = 0 ; i < dwCount ; i++ )
    {    
        *(pBuffer+i) = ATA_READ_WORD((PWORD)(m_pATAReg + ATA_PIO_DTR));
    }
}

void 
CDisk::WriteByteBuffer(PBYTE pBuffer, DWORD dwCount)
{
    for (DWORD i = 0 ; i < dwCount ; i++ )
    {    
        ATA_WRITE_BYTE((m_pATAReg + ATA_PIO_DTR),*(pBuffer+i));
    }
}

void 
CDisk::WriteWordBuffer(PWORD pBuffer, DWORD dwCount)
{
    for (DWORD i = 0 ; i < dwCount ; i++ )
    {    
        ATA_WRITE_WORD((PWORD)(m_pATAReg + ATA_PIO_DTR),*(pBuffer+i));
    }
}


void 
CDisk::WaitForNoBusyStatus(void)
{
    UINT8 tempRead;
    UINT32 count=1;

    while(1) 
    {
        count++;
        tempRead = ATA_READ_BYTE(m_pATAReg + ATA_PIO_DAD);
        tempRead = ATA_READ_BYTE(m_pATAReg + ATA_PIO_CSD);

        if((tempRead & ATA_STATUS_BUSY) == 0) 
            break;
        if(count == 1000000 )
        {
            break;
        }
    }
}



void 
CDisk::SetConfigMode(int mode, int isWriteMode)
{
    UINT32 tempCfg;

    tempCfg = ReadReg(ATA_CFG);
    switch(mode)
    {
        case PIO_CPU:
            tempCfg = ((tempCfg&0x1F3) | (0<<2)); // set PIO_CPU class
            break;
        case PIO_DMA:
            tempCfg = ((tempCfg&0x1e3) | (1<<2)); // set PDMA class
            if (isWriteMode == TRUE)
                tempCfg |= 0x10; // DMA write mode
            break;
        case UDMA:
            tempCfg = ((tempCfg&0x1F3) | (2<<2)); // set UDMA class
            tempCfg |= 0x200; // set ATA DMA auto mode (enable multi block transfer)
            if (isWriteMode == TRUE)
                tempCfg |= 0x10; // DMA write mode
            else
                tempCfg &= (~0x10); // DMA read mode
            break;            
        default:
            break;
    }
    WriteReg(ATA_CFG, tempCfg);
}


int 
CDisk::SetTransferCommand(UINT32 command)
{
    if (!(WaitForDeviceAccessReady()))
        return FALSE;
    WriteReg(ATA_COMMAND, command);
    return TRUE;
}
