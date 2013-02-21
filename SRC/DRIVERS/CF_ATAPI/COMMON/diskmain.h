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

/*++

Module Name:
    diskmain.h

Abstract:
    Base ATA/ATAPI device abstraction.

Revision History:

--*/

#ifndef _DISKMAIN_H_
#define _DISKMAIN_H_

#include <atapi2.h>
#include <atamain.h>
#include <atapiio.h>
#include <atapipm.h>
#include <csgreq.h>

#include <storemgr.h>

#define MAXIMUM_DMA_TRANSFER_SIZE               (512 * 256)

#define MUX_REG             0x1800

#define ATA_CONTROL         0x1900
#define ATA_STATUS          0x1904
#define ATA_COMMAND         0x1908
#define ATA_SWRST           0x190c
#define ATA_IRQ             0x1910
#define ATA_IRQ_MASK        0x1914
#define ATA_CFG             0x1918
#define ATA_PIO_TIME        0x192c
#define ATA_UDMA_TIME       0x1930
#define ATA_XFR_NUM         0x1934
#define ATA_XFR_CNT         0x1938
#define ATA_TBUF_START      0x193c
#define ATA_TBUF_SIZE       0x1940
#define ATA_SBUF_START      0x1944
#define ATA_SBUF_SIZE       0x1948
#define ATA_CADDR_TBUR      0x194c
#define ATA_CADDR_SBUF      0x1950

#define ATA_PIO_DTR         0x1954
#define ATA_PIO_FED         0x1958
#define ATA_PIO_SCR         0x195c
#define ATA_PIO_LLR         0x1960
#define ATA_PIO_LMR         0x1964
#define ATA_PIO_LHR         0x1968    
#define ATA_PIO_DVR         0x196c
#define ATA_PIO_CSD         0x1970
#define ATA_PIO_DAD         0x1974

#define ATA_PIO_READY       0x1978
#define ATA_PIO_RDATA       0x197c
#define BUS_FIFO_STATUS     0x1990
#define ATA_FIFO_STATUS     0x1994
// These flags are used by CDisk m_dwDeviceFlags member

#define DFLAGS_DEVICE_PRESENT       (1 << 0)  // Device is present (Set in Identify; Tested in ADC)
#define DFLAGS_ATAPI_DEVICE         (1 << 1)  // Device is ATAPI (Set in Identify; Not tested--redundant, same as m_fAtapiDevice)
#define DFLAGS_INT_DRQ              (1 << 3)  // Device interrupts as DRQ is set after receiving ATAPI packet command
#define DFLAGS_REMOVABLE_DRIVE      (1 << 4)  // Removable media device bit set in Identify Data (General Configuration) (Set in Identify; Not tested)
#define DFLAGS_DEVICE_ISDVD         (1 << 6)  // Device is DVD (use different read command) (Set in ReadSettings, based on registry; Tested in SetupCdRomRead)
#define DFLAGS_MEDIA_ISDVD          (1 << 7)  // Media is DVD (Not set; Unset in DVDGetCopySystem; Tested in DVDGetRegion)
#define DFLAGS_USE_WRITE_CACHE      (1 << 8)  // Use write cache (Set in ReadSettings based on Identify Data and in ReadSettings based on registry; Unset in ReadSettings based on registry; Tested in FlushCache)
#define DFLAGS_DEVICE_PWRDN         (1 << 24) // Device has been powered down (Set in PowerDown; Unset in PowerUp; Tested in MainIoctl)
#define DFLAGS_DEVICE_CDROM         (1 << 26) // Device is CD-ROM (Set in Identify; Tested in StallExecution)
#define DFLAGS_DEVICE_INITIALIZED   (1 << 30) // File system is loaded and Storage Manager has requested device information (Set in CPCIDisk::ConfigPort and CPCIDisk::InitController; Tested in CPCIDisk::InitController)

// These flags are not set and not tested

#define DFLAGS_TAPE_DEVICE          (1 << 2)  // Device is tape (Not set; Not tested)
#define DFLAGS_MEDIA_STATUS_ENABLED (1 << 5)  // Device supports removable media status notification feature set? (Not set; Not tested)
#define DFLAGS_USE_DMA              (1 << 9)  // Device supports DMA (Not set; Not tested)
#define DFLAGS_LBA                  (1 << 10) // Device supports LBA (Not set; Not tested)
#define DFLAGS_MULTI_LUN_INITED     (1 << 11) // Multi-LUN support intialized (Not set; Not tested)
#define DFLAGS_MSN_SUPPORT          (1 << 12) // Device supports media status notifications (Not set; Not tested)
#define DFLAGS_AUTO_EJECT_ZIP       (1 << 13) // "Boot-up default enables auto eject" (Not set; Not tested)
#define DFLAGS_WD_MODE              (1 << 14) // Device is WD, not SFF (Not set; Not tested)
#define DFLAGS_USE_UDMA             (1 << 16) // Device supports UDMA (Not set; Not tested)
#define DFLAGS_IDENTIFY_VALID       (1 << 17) // Identify Data is valid (Not set; Not tested)
#define DFLAGS_DEVICE_BUSY          (1 << 25) // Device is busy (Not set; Not tested)
#define DFLAGS_MEDIA_CHANGED        (1 << 27) // Media has changed (Not set; Not tested)
#define DFLAGS_DEVICE_READY         (1 << 28) // Device supports removable media and is ready (Not set; Not tested)
#define DFLAGS_DEVICE_ACTIVATED     (1 << 29) // Device supports removable media and has volume(s) mounted (Not set; Not tested)
#ifdef FAKE_CRC_ERROR
#define DFLAGS_FAKING_CRC_ERROR     (1 << 31) // Fake CRC error (Not set; Not tested)
#endif // FAKE_CRC_ERROR

// These flags are not set and not tested

#define ATA_VERSION_MASK (0xfffe)
#define ATA1_COMPLIANCE  (1 << 1)
#define ATA2_COMPLIANCE  (1 << 2)
#define ATA3_COMPLIANCE  (1 << 3)
#define ATA4_COMPLIANCE  (1 << 4)

// WaitForDisc wait condition codes; for more information on WaitForDisc
// (bStatusType), see atapiio.cpp.
// For more information on ATA/ATAPI device conditions, see ATA/ATAPI-6 R3B
// 7.14 (Status register).

#define WAIT_TYPE_BUSY      1
#define WAIT_TYPE_NOT_BUSY  2
#define WAIT_TYPE_READY     3
#define WAIT_TYPE_DRQ       4
#define WAIT_TYPE_NOT_DRQ   5
#define WAIT_TYPE_ERROR     6

// By default, this will use the CEDDK functions.  To use macros instead,
// turn on CEDDK_USEDDKMACRO flag in the sources file.

// Base ATA/ATAPI device abstraction

// forward declaration
class CPort;

class CDisk {

  public:

    // Number of bytes between each ATA register; this is to facilitate PMC
    // platforms (i.e., Intel PXA)

    DWORD m_dwStride;

    // ATA register offsets from start of device control I/O port

                                     // Name             | Ref  | Notes
                                     // -----------------|------|------
    DWORD m_dwDataDrvCtrlOffset;     // Data             | 7.8  | 8 or 16 bits; PIO out <=> read, PIO in <=> write
    DWORD m_dwFeatureErrorOffset;    // Error            | 7.11 | RO; ABRT bit, i.e., Command code/parameter is invalid
                                     // Features         | 7.12 | WO; Command parameter
    DWORD m_dwSectCntReasonOffset;   // Sector Count     | 7.13 | Command parameter
    DWORD m_dwSectNumOffset;         // Sector Number    | 7.14 | Command parameter
    DWORD m_dwByteCountLowOffset;    // Cylinder Low     | 7.6  | Command parameter
    DWORD m_dwByteCountHighOffset;   // Cylinder High    | 7.5  | Command parameter
    DWORD m_dwDrvHeadOffset;         // Device/Head      | 7.10 | DEV bit selects device; 0 <=> master/device 0, 1 <=> slave/device 1
    DWORD m_dwCommandStatusOffset;   // Status           | 7.15 | RO; Read clears pending interrupt; contains BSY, DRDY, and ERR bits
                                     // Command          | 7.4  | WO; Command code; execution begins immediately after write

    // ATA register offsets from start of alternate status I/O port

    DWORD m_dwAltStatusOffset;       // Alternate Status | 7.3  | RO; Same as Status; Read does not clear pending interrupt
                                     // Device Control   | 7.9  | WO; SRST and nIEN bits; Both devices respond to write
    DWORD m_dwAltDrvCtrl;            // Same as m_dwAltStatusOffset

    volatile PBYTE m_pATAReg;        // Base address of device control register set (MmMapped)
    volatile PBYTE m_pATARegAlt;     // Base address of alternate status register set (MmMapped)
    BOOL           m_f16Bit;         // Data register width; set to TRUE in CPCIDisk::Init, tested in Identify and ReadWritedisk

    CPort *m_pPort;                  // I/O resources; including registry settings
    BOOL   m_fInterruptSupported;    // Set based on registry
    BOOL   m_fDMAActive;             // Set based on registry
    DWORD  m_dwDMAAlign;             // Set based on registry
    PBYTE  m_rgbDoubleBuffer;        // PIO mode double buffer; set based on registry
    WORD   m_wNextByte;              // Store extra byte when reading/writing unaligned buffer

    PSG_REQ m_pSterileIoRequest;     // Sterile I/O request for safe I/O

    CRITICAL_SECTION m_csDisk;       // CDisk critical section

    DWORD  m_dwOpenCount;            // DSK_Open count; number of system-wide references to this device
    CDisk *m_pNextDisk;              // Next CDisk in global (ATAPI.DLL) disk list
    TCHAR  m_szActiveKey[MAX_PATH];  // Name of DeviceX's active key
    TCHAR  m_szDeviceKey[MAX_PATH];  // Name of DeviceX's instance key
    HKEY   m_hDevKey;                // Handle to DeviceX subkey
    DWORD  m_dwDevice;               // Master <=> 0; Slave <=> 1
    DWORD  m_dwDeviceId;             // Master <=> 0; Slave <=> 1
    DWORD  m_dwPort;                 // Not used; if device is on primary channel, 0; If device is on secondary channel, 1

    IDENTIFY_DATA m_Id;              // IDENTIFY DEVICE data
    DWORD         m_dwDeviceFlags;   // Device attributes, see above
    BOOL          m_fLBAMode;        // Device supports LBA or C/H/S
    BOOL          m_fUseLBA48;       // Device supports extended 48 bit LBA from the revised ATA6 spec (disks > 128 GB)
    BOOL          m_fAtapiDevice;    // Device is ATA or ATAPI
    BYTE          m_bReadCommand;    // Read command to use (single-sector or multi-sector)
    BYTE          m_bWriteCommand;   // Write command to use (single-sector or multi-sector)
    BYTE          m_bDMAReadCommand; // DMA Read command to use
    BYTE          m_bDMAWriteCommand;// DMA Write command to use

    DISK_INFO m_DiskInfo;            // DISK_INFO (disk geometry) Storage Manager structure
    PTSTR     m_szDiskName;

    DWORD m_dwWaitCheckIter;         // Timeout
    DWORD m_dwWaitSampleTimes;       // Timeout
    DWORD m_dwWaitStallTime;         // Timeout
    DWORD m_dwDiskIoTimeOut;         // Timeout
    DWORD m_dwUnitReadyTime;         // Time between TEST_UNIT_READY commands
    DWORD m_dwLastCheckTime;         // Not used; time of last TEST_UNIT_READY command

    CDiskPower *m_pDiskPower;

    HANDLE m_hDevice;                // Handle to this store

    DWORD dwPktSize;                 // Not used; command packet size (12 or 16 bytes)
    DWORD m_dwStateFlag;             // Not used
    INQUIRY_DATA m_InqData;          // Not used
    BYTE m_bSectorsPerBlock;         // This was used at some point in time, but is no longer used (Set in Identify)

    STORAGEDEVICEINFO m_storagedeviceinfo; // This structure holds information to be returned for IOCTL_DISK_DEVICE_INFO

    PHYSICAL_ADDRESS m_DMAPhyaddress;
    PBYTE   m_pDMAVirtualAddress;
    DWORD m_dwCurrentUDMAMode;

public:

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    CDisk();          // Not used
    CDisk(HKEY hKey); // Handle to device's DeviceX instance key
    virtual ~CDisk();

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // Read/write Data register
    void ReadBuffer(PBYTE pBuffer, DWORD dwCount);
    void WriteBuffer(PBYTE pBuffer, DWORD dwCount);
    BOOL WaitForDeviceAccessReady(void);

    inline void WriteDriveController(BYTE bData) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwDataDrvCtrlOffset, bData);
    }
    inline WORD ReadByte() {
        return ATA_READ_BYTE(m_pATAReg+m_dwDataDrvCtrlOffset);
    }
    inline WORD ReadWord() {
        return ATA_READ_WORD((PWORD)(m_pATAReg+m_dwDataDrvCtrlOffset));
    }

/* // These macro has to change to our CF controller style.
    inline void ReadByteBuffer(PBYTE pBuffer, DWORD dwCount) {
        volatile BYTE * const Port = (volatile BYTE * const)(m_pATAReg+m_dwDataDrvCtrlOffset);
        if (m_pPort->m_pController->m_bisIOMapped)
            READ_PORT_BUFFER_UCHAR((PBYTE)Port, pBuffer, dwCount);
        else {
            while( dwCount-- )
                *pBuffer++ = *Port;
        }
    }
    inline void ReadWordBuffer(PWORD pBuffer, DWORD dwCount) {
        volatile USHORT * const Port = (volatile USHORT * const)(m_pATAReg+m_dwDataDrvCtrlOffset);
        if (m_pPort->m_pController->m_bisIOMapped)
            READ_PORT_BUFFER_USHORT((PUSHORT)Port, pBuffer, dwCount);
        else {
            while( dwCount-- )
                *pBuffer++ = *Port;
        }
    }
*/    
    inline void WriteByte(BYTE bData) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwDataDrvCtrlOffset, bData);
    }
    inline void WriteWord(WORD wData) {
        ATA_WRITE_WORD((PWORD)(m_pATAReg + m_dwDataDrvCtrlOffset), wData);
    }

    inline UINT32 ReadReg(UINT32 dwOffset)
    {
        volatile UINT32 uRegister = *(volatile UINT32 *)(m_pATAReg + dwOffset);
        return uRegister;
    }

    inline VOID WriteReg(UINT32 dwOffset, UINT32 uValue)
    {
        volatile UINT32 *puRegister = (volatile UINT32 *)(m_pATAReg + dwOffset);
        *puRegister = uValue;
    }

/* // These macro has to change to our CF controller style.    
    inline void WriteByteBuffer(PBYTE pBuffer, DWORD dwCount) {
        volatile BYTE * const Port = (volatile BYTE * const)(m_pATAReg+m_dwDataDrvCtrlOffset);
        if (m_pPort->m_pController->m_bisIOMapped)
            WRITE_PORT_BUFFER_UCHAR((PBYTE)Port, pBuffer, dwCount);
        else {
            while( dwCount-- )
                *Port = *pBuffer++;
        }
    }
    inline void WriteWordBuffer(PWORD pBuffer, DWORD dwCount) {
        volatile USHORT * const Port = (volatile USHORT * const)(m_pATAReg+m_dwDataDrvCtrlOffset);
        if (m_pPort->m_pController->m_bisIOMapped)
            WRITE_PORT_BUFFER_USHORT((PUSHORT)Port, pBuffer, dwCount);
        else {
            while( dwCount-- )
                *Port = *pBuffer++;
        }
    }
*/

    // Read/write Error/Feature register
    inline BYTE GetError() {
        return (BYTE) ATA_READ_BYTE(m_pATAReg + m_dwFeatureErrorOffset);
    }
    inline void WriteFeature(BYTE bFeature) {
        ATA_WRITE_BYTE( m_pATAReg + m_dwFeatureErrorOffset, bFeature);
    }
    // Read/write Sector Count register
    inline BYTE GetReason() {
        return (BYTE) ATA_READ_BYTE(m_pATAReg + m_dwSectCntReasonOffset);
    }
    inline void WriteSectorCount(BYTE bValue) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwSectCntReasonOffset, bValue);
    }
    // Write Sector Number register
    inline void WriteSectorNumber(BYTE bValue) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwSectNumOffset, bValue);
    }
    // Read/write Cylinder Low, Cylinder High register
    inline BYTE GetLowCount() {
        return ATA_READ_BYTE(m_pATAReg+ m_dwByteCountLowOffset);
    }
    inline void WriteLowCount(BYTE bValue) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwByteCountLowOffset, bValue);
    }
    inline BYTE GetHighCount() {
        return ATA_READ_BYTE(m_pATAReg+ m_dwByteCountHighOffset);
    }
    inline void WriteHighCount(BYTE bValue) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwByteCountHighOffset, bValue);
    }
    inline WORD GetCount() {
        return GetLowCount() + ((WORD)GetHighCount() << 8);
    }
    // Set DEV bit in Device/Head register
    inline void SelectDevice() {
        ATA_WRITE_BYTE(m_pATAReg + m_dwDrvHeadOffset, (m_dwDevice == 0 ) ? ATA_HEAD_DRIVE_1 : ATA_HEAD_DRIVE_2);
    }
    // Write Device/Head register
    inline void SetDriveHead(BYTE bDriveHead) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwDrvHeadOffset, bDriveHead);
    }
    inline void WriteDriveHeadReg(BYTE bValue) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwDrvHeadOffset, bValue);
    }
    // Read Status register (does not clear pending interrupt)
    inline BYTE GetBaseStatus() {
        BYTE bStatus = ATA_READ_BYTE(m_pATAReg + m_dwCommandStatusOffset);
        if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_BASESTATUS, &bStatus, sizeof(bStatus), 0, CELZONE_ALWAYSON, 0, FALSE);
        return bStatus;
    }
    // Write Command register
    inline void WriteCommand(BYTE bCommand) {
        ATA_WRITE_BYTE(m_pATAReg + m_dwCommandStatusOffset, bCommand);
    }
    // Read/write Alternate Status register (read clears pending interrupt)
    inline BYTE GetAltStatus() {
        return (BYTE) ATA_READ_BYTE(m_pATARegAlt + m_dwAltStatusOffset);
    }
    inline void WriteAltDriveController(BYTE bData) {
        ATA_WRITE_BYTE(m_pATARegAlt + m_dwAltDrvCtrl, bData);
    }
 
    inline BYTE ReadByteFromRDATA() {
        return (BYTE) (*((PBYTE)(m_pATAReg + ATA_PIO_RDATA)));
    }

    inline WORD ReadWordFromRDATA() {
        return (WORD) (*((PUSHORT)(m_pATAReg + ATA_PIO_RDATA)));
    }

    inline DWORD ReadDWordFromRDATA() {
        return (DWORD) (*((PULONG)(m_pATAReg + ATA_PIO_RDATA)));
    }    
    

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // DSK_Init calls this before Init; set up register blocks
    virtual VOID ConfigureRegisterBlock(DWORD dwStride);
    // Called by DSK_Init
    virtual BOOL Init(HKEY hActiveKey);
    // Reset controller, enable interrupts, call Identify
    virtual BOOL InitController(BOOL fForce = FALSE);
    // Set m_fUseLBA48 if appropriate
    void ConfigLBA48(void);
    // Return STORAGEDEVICEINFO structure
    virtual DWORD GetDeviceInfo(PIOREQ pIOReq);
    // Return STORAGEDEVICEINFO structure by reading the registry.
    virtual DWORD GetDeviceInfo(PSTORAGEDEVICEINFO psdi);
    // Handle standard (diskio.h), non-CD, non-DVD IOCTL-s
    virtual DWORD MainIoctl(PIOREQ pIOReq);
    // Wait for Status register DRQ bit to assert
    virtual BOOL WaitForDRQ();
    // Wait for a specified Status register bit to assert/negate
    virtual BOOL WaitForDisc(BYTE bStatusType, DWORD dwTimeOut, DWORD dwPeriod = 0);
    // DMA I/O
    virtual DWORD ReadWriteDiskDMA(PIOREQ pIOReq, BOOL fRead = TRUE);
    // Instantiate a CDiskPower object
    virtual CDiskPower *GetDiskPowerInterface(void);
    // Eventually called in response to an IOCTL_POWER_SET
    virtual BOOL SetDiskPowerState(CEDEVICE_POWER_STATE newDx);
    // Base wake up function
    virtual BOOL WakeUp();
    // Acquire this instance's critical section
    virtual void TakeCS();
    // Release this instance's critical section
    virtual void ReleaseCS();
    // Called by DSK_Init; this information is used by CDiskPower
    void SetActiveKey(TCHAR *szActiveKey);
    // Called by DSK_Init; this information is used by CDiskPower
    void SetDeviceKey(TCHAR *szDeviceKey);

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // Wait for interrupt
    virtual BOOL WaitForInterrupt(DWORD dwTimeOut) = 0;
    // Enable device interrupt
    virtual void EnableInterrupt() = 0;
 
    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // Core device/CDisk initialize function
    BOOL Identify();
    // Determine whether this device is present on the channel
    BOOL IsDevicePresent();

    // Wait for Status register BSY bit to negate
    BYTE WaitOnBusy(BOOL fBase);
    // Sleep for @dwTime milliseconds
    void StallExecution(DWORD dwTime);
    // Return interrupt state?
    WORD CheckIntrState();

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // Perform ATA/ATAPI soft-reset
    BOOL ResetController(BOOL bSoftReset = FALSE);
    // This function sets the transfer mode to @dwMode; For more information
    // on transfer mode encodings, see ATA/ATAPI-6 R3B 8.46.11 (Set transfer mode)
    BOOL SetTransferMode(BYTE bMode);
    // Send EXECUTE DEVICE DIAGNOSTIC command; Determine whether this device is
    // an ATA device or an ATAPI device
    BOOL SendExecuteDeviceDiagnostic(PBYTE pbDiagnosticCode, PBOOL pfIsAtapi);
    // Send IDENTIFY DEVICE/IDENTIFY PACKET DEVICE command
    BOOL SendIdentifyDevice(BOOL fIsAtapi);
    // Enable write cache through SET FEATURES
    BOOL SetWriteCacheMode(BOOL fEnable);
    // Enable read look-ahead through SET FEATURES
    BOOL SetLookAhead();
    // Non-DMA I/O
    BOOL DoRead(CSgReq* pSgReqWrapper, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesRead);
    BOOL DoRead(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesRead);
    BOOL DoReadDMA(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesRead);
    DWORD ReadDisk(PIOREQ pIoReq);
    BOOL DoWrite(CSgReq* pSgReqWrapper, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesWritten);
    BOOL DoWrite(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesWritten);
    BOOL DoWriteDMA(PBYTE pbBuf, DWORD dwStartingSector, DWORD dwNumberOfSectors, PDWORD pdwBytesWritten);
    DWORD WriteDisk(PIOREQ pIoReq);
    // Issue FLUSH CACHE
    DWORD FlushCache();
    // Send read/write command
    BOOL SendIOCommand(DWORD dwStartSector, DWORD dwNumberOfSectors, BYTE bCmd);
    // Issue Power Management command and implements the non-data command protocol
    BOOL SendDiskPowerCommand(BYTE bCmd, BYTE bParam = 0);
    // Issue ATAPI soft-reset
    void AtapiSoftReset();

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // Increment m_dwOpenCount; references to device
    void Open();
    // Decrement m_dwOpenCount; references to device
    void Close();
    // Return DISK_INFO structure
    DWORD GetDiskInfo(PIOREQ pIOReq);
    // Set ISK_INFO structure
    DWORD SetDiskInfo(PIOREQ pIOReq);
    // Return mount name, e.g., "Hard Disk"
    DWORD GetDiskName(PIOREQ pIOReq);
    // Return STORAGE_IDENTIFICATION
    DWORD GetStorageId(PIOREQ pIOReq);
    // Handle DISK_IOCTL_INITIALIZED
    BOOL PostInit(PPOST_INIT_BUF pPostInitBuf);
    // Intercept IOCTL_POWER_Xxx-s (and instantiates a CDiskPower object during
    // IOCTL_POWER_CAPABILITIES) and forwards all other IOCTL-s to MainIoctl
    BOOL PerformIoctl(PIOREQ pIOReq);
    // Validate pointers embedded in scatter/gather requests
    BOOL static ValidateSg(PSG_REQ pSgReq, DWORD InBufLen, DWORD dwArgType, OUT PUCHAR * saveoldptrs);
    // Unmap pointers mapped by ValidateSg, Essentially an array version of CeCloseCallerBuffer
    HRESULT static UnmapSg(
        IN const SG_BUF * sr_sglist,
        IN const PUCHAR * saveoldptrs,
        IN DWORD sr_sglistlen,
        IN DWORD dwArgType) ;

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // Determine if Power Management is supported by examining the IDENTIFY
    // DEVICE information
    BOOL IsPMSupported(void);
    // Determines if Power Management is enabled by examining the IDENTIFY
    // DEVICE information
    BOOL IsPMEnabled(void);

    void PowerUp() { m_dwDeviceFlags &= ~DFLAGS_DEVICE_PWRDN; }
    void PowerDown() { m_dwDeviceFlags |= DFLAGS_DEVICE_PWRDN; }

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------

    // Return m_fAtapiDevice
    BOOL IsAtapiDevice();
    // Check the IDENTIFY DEVICE data to see if the device is a CD-ROM device
    BOOL IsCDRomDevice();
    // Check the IDENTIDY DEVICE data to see if the device is a disk device
    BOOL IsDiskDevice();
    // Check the IDENTIDY DEVICE data to see if the device is a removable media device
    BOOL IsRemoveableDevice();
    // Check the IDENTIFY DEVICE data to see if the device supports DMA and
    // whether m_fDMAActive is TRUE
    BOOL IsDMASupported();
    // Check the IDENTIFY DEVICE data to see the packet size supported by the device
    WORD GetPacketSize();
    // Check the IDENTIFY DEVICE data to see if a portion of it is valid
    BOOL IsValidCommandSupportInfo();
    // Check the IDENTIFY DEVICE data to see if the device supports the write cache feature
    BOOL IsWriteCacheSupported();
    // Not used
    BOOL IsDRQTypeIRQ();
    // Not used
    BOOL IsDVDROMDevice();

    BYTE ATA_READ_BYTE(PBYTE p);
    WORD ATA_READ_WORD(PUSHORT p);
    ULONG ATA_READ_DWORD(PULONG p);   
    void ATA_WRITE_BYTE(PBYTE p,BYTE v);
    void ATA_WRITE_WORD(PUSHORT p,USHORT v);
    void ATA_WRITE_DWORD(PULONG p,ULONG v);

    void ReadByteBuffer(PBYTE pBuffer, DWORD dwCount);
    void ReadWordBuffer(PWORD pBuffer, DWORD dwCount);
    void WriteByteBuffer(PBYTE pBuffer, DWORD dwCount);
    void WriteWordBuffer(PWORD pBuffer, DWORD dwCount);
  
    

    void WaitForNoBusyStatus(void);

    void SetConfigMode(int mode, int isWriteMode);

    int SetTransferCommand(UINT32 command);

    virtual void SetPioMode(UCHAR pmode) = 0;
    virtual void SetUdmaMode(void) = 0;
 };

#define ATA_CMD_STOP 0
#define ATA_CMD_START 1
#define ATA_CMD_ABORT 2

#define PIO_CPU 0
#define PIO_DMA 1
#define UDMA 2


#define PIO0 0
#define PIO1 1
#define PIO2 2
#define PIO3 3
#define PIO4 4


#define UDMA0 0
#define UDMA1 1
#define UDMA2 2
#define UDMA3 3
#define UDMA4 4

#endif // _DISKMAIN_H_
