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
    atamain.h

Abstract:
    ATA/ATAPI device driver definitions.

Revision History:

--*/

#ifndef _ATAMAIN_H_
#define _ATAMAIN_H_

#include <windows.h>
#include <ceddk.h>
#include <devload.h>
#include <ddkreg.h>
#include <nkintr.h>
#include <celog.h>
#include <diskio.h>
#include <atapi2.h>
#include <cdioctl.h>
#include <dvdioctl.h>
#include <atapiio.h>
#include <helper.h>
#include <debug.h>
#include <storemgr.h>
#include <winnt.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntddmmc.h>

#define MIN(a,b)                          ((a)<(b)?(a):(b))
#define SECTOR_SIZE                       512

#define MAX_RESET_ATTEMPTS                256
#define MAX_SECT_PER_COMMAND              256
#define MAX_CD_SECT_PER_COMMAND           32

#define MAX_SECT_PER_EXT_COMMAND          65536   // As per the 48 bit LBA ATA spec

// IDE/ATA controller subkey names for device enumeration
#define REG_KEY_PRIMARY_MASTER            (_T("Device0"))
#define REG_KEY_PRIMARY_SLAVE             (_T("Device1"))
#define REG_KEY_SECONDARY_MASTER          (_T("Device2"))
#define REG_KEY_SECONDARY_SLAVE           (_T("Device3"))

// IDE/ATA controller registry value definitions
#define REG_VAL_IDE_LEGACY                (_T("Legacy"))                // {0, 1}; 1 => use legacy IRQ settings, i.e., primary 14, secondary 15
#define REG_VAL_IDE_IRQ                   (_T("Irq"))                   // IDE/ATA channel's IRQ
#define REG_VAL_IDE_SYSINTR               (_T("SysIntr"))               // IDE/ATA channel's SysIntr
#define REG_VAL_IDE_VENDORID              (_T("VendorID"))              // vendor id; identify controller; is this needed?
#define REG_VAL_IDE_DMAALIGNMENT          (_T("DMAAlignment"))          // DMA alignment
#define REG_VAL_IDE_SOFTRESETTIMEOUT      (_T("SoftResetTimeout"))      // seconds; ATA/ATAPI specification default is 31 seconds (too long)
#define REG_VAL_IDE_STATUSPOLLCYCLES      (_T("StatusPollCycles"))      // number of Status register poll cycles
#define REG_VAL_IDE_STATUSPOLLSPERCYCLE   (_T("StatusPollsPerCycle"))   // number of Status registers polls per cycle
#define REG_VAL_IDE_STATUSPOLLCYCLEPAUSE  (_T("StatusPollCyclePause"))  // number of milliseconds to wait between between poll cycles
#define REG_VAL_IDE_SPAWNFUNCTION         (_T("SpawnFunction"))         // name of function to create controller-specific CDisk instance
#define REG_VAL_IDE_ISRDLL                (_T("IsrDll"))                // name of installable ISR DLL
#define REG_VAL_IDE_ISRHANDLER            (_T("IsrHandler"))            // name of ISR handler in ISR DLL
#define REG_VAL_IDE_DEVICECONTROLOFFSET   (_T("DeviceControlOffset"))   // base ATA register offset of device control I/O port
#define REG_VAL_IDE_ALTERNATESTATUSOFFSET (_T("AlternateStatusOffset")) // base ATA register offset of alternate status I/O port
#define REG_VAL_IDE_REGISTERSTRIDE        (_T("RegisterStride"))        // number of bytes between ATA registers
#define REG_VAL_IDE_DISABLE48BITLBA       (_T("Disable48BitLBA"))       // {0, 1}; 0 => enable 48-bit LBA; 1 => disable

// IDE_ registry value set
typedef struct _IDEREG {
    DWORD dwLegacy;
    DWORD dwIrq;
    DWORD dwSysIntr;
    DWORD dwVendorId;
    DWORD dwDMAAlignment;
    DWORD dwSoftResetTimeout;
    DWORD dwStatusPollCycles;
    DWORD dwStatusPollsPerCycle;
    DWORD dwStatusPollCyclePause;
    PTSTR pszSpawnFunction;
    PTSTR pszIsrDll;
    PTSTR pszIsrHandler;
    DWORD dwDeviceControlOffset;
    DWORD dwAlternateStatusOffset;
    DWORD dwRegisterStride;
    DWORD dwDisable48BitLBA;
} IDEREG, *PIDEREG;

// Populate IDE_ registry value set from registry
BOOL
GetIDERegistryValueSet(
    HKEY hIDEInstanceKey,
    PIDEREG pIdeReg
    );

// ATA/ATAPI device registry value definitions
#define REG_VAL_DSK_PORT             (_T("Port"))             // heap address of associated CPort instance
#define REG_VAL_DSK_INTERRUPTDRIVEN  (_T("InterruptDriven"))  // {0, 1}; 0 => polled I/O, 1 => interrupt I/O
#define REG_VAL_DSK_DMA              (_T("DMA"))              // {0, 1}; 0 => PIO, 1 => DMA, 2 => ATA DMA only
#define REG_VAL_DSK_DOUBLEBUFFERSIZE (_T("DoubleBufferSize")) // {512, ..., 131072}; scatter/gather processing
#define REG_VAL_DSK_DRQDATABLOCKSIZE (_T("DrqDataBlockSize")) // {512}; when we support R/W multiple, increase
#define REG_VAL_DSK_WRITECACHE       (_T("WriteCache"))       // {0, 1}; 0 => disable write cache; 1 => enable write cache
#define REG_VAL_DSK_LOOKAHEAD        (_T("LookAhead"))        // {0, 1}; 0 => disable look-ahead; 1 => enable look-ahead
#define REG_VAL_DSK_DEVICEID         (_T("DeviceId"))         // initially (0, 1, 2, 3), then resolve re-written as (0, 1); 0 => master, 1 => slave
#define REG_VAL_DSK_TRANSFERMODE     (_T("TransferMode"))     // 1 byte transfer mode encoding; see ATA/ATAPI 8.46.11; 0xFF is default mode

#define REG_VAL_DSK_ENABLE_PDMA       (_T("EnablePDMA"))      // Enable PDMA.
#define REG_VAL_DSK_ENABLE_UDMA       (_T("EnableUDMA"))      // Enable UDMA. It's for 6410
#define REG_VAL_DSK_INDIRECT_MODE       (_T("IndirectMode"))      // Enable UDMA. It's for 6410
#define REG_VAL_DSK_DOUBLEBUFFERSIZE_MAX 131072
#define REG_VAL_DSK_DOUBLEBUFFERSIZE_MIN 512
#define REG_VAL_DSK_DRQDATABLOCKSIZE_MAX 130560
#define REG_VAL_DSK_DRQDATABLOCKSIZE_MIN 512

// DSK_ registry value set
typedef struct _DSKREG {
    DWORD dwInterruptDriven;
    DWORD dwDMA;
    DWORD dwDoubleBufferSize;
    DWORD dwDrqDataBlockSize;
    DWORD dwWriteCache;
    DWORD dwLookAhead;
    DWORD dwDeviceId;
    DWORD dwTransferMode;
    DWORD dwEnablePDMA;
    DWORD dwEnableUDMA;
    DWORD dwIndirectMode;
} DSKREG, *PDSKREG;

// Populate DSK_ registry value set from registry
BOOL
GetDSKRegistryValueSet(
    HKEY hDSKInstanceKey,
    PDSKREG pDskReg
    );

// Registry configuration value names
#define REG_VALUE_IOBASEADDRESS     TEXT("IOBaseAddress")    // not used
#define REG_VALUE_BMR               TEXT("BMR")              // not used
#define REG_VALUE_INTERRUPT         TEXT("Interrupt")        // not used
#define REG_VALUE_IRQ               TEXT("IRQ")              // not used
#define REG_VALUE_DVD               TEXT("DVD")              // read in diskmain.cpp!ReadSettings; not used
#define REG_VALUE_CHS               TEXT("CHSMode")          // read in diskmain.cpp!Identify
#define REG_VALUE_SYSINTR           TEXT("SysIntr")          // not used, but should be used in atamain.cpp
#define REG_VALUE_INTENABLE         TEXT("IntEnable")        // read in diskmain.cpp!ReadSettings; should be read in atamain.cpp
#define REG_VALUE_HDPROFILE         TEXT("HDProfile")        // read in diskmain.cpp!GetDeviceInfo; why can't this just be "StorageManagerProfile"?
#define REG_VALUE_6410_CFPROFILE         TEXT("6410_CF") 
#define REG_VALUE_CDPROFILE         TEXT("CDProfile")        // read in diskmain.cpp!GetDeviceInfo; why can't this just be "StorageManagerProfile"?
#define REG_VALUE_PCMCIAPROFILE     TEXT("PCMCIA")           // not used
#define REG_VALUE_ENABLE_WRITECACHE TEXT("EnableWriteCache") // read in diskmain.cpp!ReadSettings

// Registry configurable DMA alignment
#define REG_VALUE_DMA_ALIGN         TEXT("DMAAlignment")     // read in diskmain.cpp!ReadSettings; should be read in atamain.cpp
#define DEFAULT_DMA_ALIGN_VALUE     4

// Registry configurable timeout values
#define REG_VALUE_MEDIACHECKTIME    TEXT("MediaCheckTime")      // read in diskmain.cpp!ReadSettings
#define REG_VALUE_WAIT_CHECK_ITER   TEXT("WaitCheckIterations") // read in diskmain.cpp!ReadRegistry; why?
#define REG_VALUE_WAIT_SAMPLE_TIMES TEXT("WaitSampleTimes")     // read in diskmain.cpp!ReadRegistry; why?
#define REG_VALUE_WAIT_STALL_TIME   TEXT("WaitStallTime")       // read in diskmain.cpp!ReadRegistry; why?
#define REG_VALUE_DISK_IO_TIME_OUT  TEXT("DiskIOTimeOut")       // read in diskmain.cpp!ReadRegistry; why?

#define DEFAULT_MEDIA_CHECK_TIME    5000
#define DEFAULT_WAIT_CHECK_ITER     2000
#define DEFAULT_WAIT_SAMPLE_TIMES   100
#define DEFAULT_WAIT_STALL_TIME     400
#define DEFAULT_DISK_IO_TIME_OUT    20000

// "Settings" registry value
#define REG_VALUE_SETTINGS          TEXT("Settings")
#define REG_VALUE_DMA               TEXT("DMA")
#define ATA_SETTINGS_HDDMA          0x1 // Hard disk DMA enabled
#define ATA_SETTINGS_CDDMA          0x4 // CD-ROM/DVD DMA enabled
#define ATA_SETTINGS_HDINT          0x2 // Hard disk interrupt enabled
#define ATA_SETTINGS_CDINT          0x8 // CD-ROM/DVD interrupt enabled

#define REG_VALUE_PORT              (_T("Port"))

class CDisk;
class CPort;

// Helper function prototype
BOOL
AtaIsValidDisk(
    CDisk *pDisk
    );

// IDE/ATA bus abstraction; Not needed

#define MAX_DEVICES_PER_CONTROLLER 4

#define ATA_PRIMARY 0
#define ATA_SECONDARY 1

class CIDEBUS {
  public:
    // member variables
    HANDLE         m_hDevice[MAX_DEVICES_PER_CONTROLLER];  // device activation handles
    LPWSTR         m_szDevice[MAX_DEVICES_PER_CONTROLLER]; // device key paths
    CPort         *m_pPort[MAX_DEVICES_PER_CONTROLLER];    // I/O port of channels
    DDKWINDOWINFO  m_dwi;                                  // resource information for devices attached to the controller
    PIDEREG        m_pIdeReg;                              // IDE_ registry value set
//    CPort         *m_pPrimaryPort;                         // I/O port of primary channel
//    CPort         *m_pSecondaryPort;                       // I/O port of secondary channel
    BOOL              m_bisIOMapped;                       // Are the registers IOmapped
    // constructors/destructors
    CIDEBUS();
    ~CIDEBUS();
};

// Bus master definitions
#define BM_STATUS_SIMPLEX 0x80
#define BM_STATUS_D1_DMA  0x40
#define BM_STATUS_D0_DMA  0x20
#define BM_STATUS_INTR    0x04
#define BM_STATUS_ERROR   0x02
#define BM_STATUS_ACTIVE  0x01

// DMA support structures

typedef struct {
    LPBYTE pDstAddress;
    LPBYTE pSrcAddress;
    DWORD dwSize;
} SGCopyTable, *PSGCopyTable;

typedef struct {
    DWORD dwVirtualAddress;
    DWORD dwPhysicalAddress;
    DWORD dwFlags;
    DWORD dwSize;
} MEMTable, *PMEMTable;

typedef struct {
    DWORD physAddr;
    USHORT size;
    USHORT EOTpad;
} DMATable, *PDMATable;

#define MIN_PHYS_PAGES 4

typedef struct _PhysTable {
    LPBYTE pVirtualAddress;
    LPBYTE pPhysicalAddress;
} PhysTable, *PPhysTable;

// IDE/ATA channel abstraction
class CPort {
  public:
    // member variables
    CIDEBUS          *m_pController;   // parent
    CRITICAL_SECTION  m_csPort;        // protect access to I/O ports
    DWORD             m_fInitialized;  // whether port has been initialized by IDE driver
    DWORD             m_dwFlag;        // m_dwFlag
    DWORD             m_dwRegBase;     // base virtual address of command I/O port
    DWORD             m_dwRegAlt;      // base virtual address of status I/O port
    DWORD             m_dwBMR;         // base virtual address of bus master I/O port
    DWORD             m_dwBMRStatic;   // base physical address of bus master I/O port
    HANDLE            m_hIRQEvent;     // IRQ event handle
    PDSKREG           m_pDskReg[2];    // DSK_ registry value set for master, slave
    DWORD             m_dwSysIntr;     // SysIntr associated with ATA channel
    DWORD             m_dwIrq;         // IRQ associated with ATA channel
    // not used
    HANDLE            m_hThread;       // not used; IST handle
    HANDLE            m_pDisk[2];      // only used by Promise; store handle?
    // constructors/destructors
    CPort(CIDEBUS *pParent);
    ~CPort();
    // member functions
    void TakeCS();
    void ReleaseCS();
    void PrintInfo();
};

#endif _ATAMAIN_H_
