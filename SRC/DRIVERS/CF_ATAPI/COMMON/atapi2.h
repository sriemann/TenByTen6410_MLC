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
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

    atapi2.h

Abstract:

    This module contains the function prototypes and constant, type and
    structure definitions for the WINCE Implementation of the AT
    Attachment API for disk devices.

Notes: 


--*/

#ifndef _ATAPI_H_
#define _ATAPI_H_

#include <windef.h> 

#define BYTES_PER_SECTOR        512
#define CDROM_RAW_SECTOR_SIZE   2352
#define CDROM_SECTOR_SIZE       2048

#define ATA_MEM_REG_BASE    0    
#define ATA_REG_DATA        0
#define ATA_REG_FEATURE     1   // write
#define ATA_REG_ERROR       1   // read
#define ATA_REG_SECT_CNT    2
#define ATA_REG_SECT_NUM    3
#define ATA_REG_CYL_LOW     4
#define ATA_REG_CYL_HIGH    5
#define ATA_REG_DRV_HEAD    6
#define ATA_REG_COMMAND     7   // write
#define ATA_REG_STATUS      7   // read     (reading this acknowledges the interrupt)
#define ATA_REG_LENGTH      ATA_REG_STATUS+1

#define ATA_REG_BYTECOUNTLOW    ATA_REG_CYL_LOW    
#define ATA_REG_BYTECOUNTHIGH   ATA_REG_CYL_HIGH
#define ATA_REG_REASON          2

#define ATA_REG_ALT_STATUS_CS1  6   // this is at address 6 in the ATA CS1 space
#define ATA_REG_DRV_CTRL_CS1    6   // this is at address 6 in the ATA CS1 space

// CMD PCI chip maps four bytes starting at 3f4 (same registers as above, but PCI
// skips 4 bytes of unused address space)
#define ATA_REG_ALT_STATUS  2   // read     (reading this does not ack the interrupt)
#define ATA_REG_DRV_CTRL    2   // write

#define ATA_ALT_MEM_REG_BASE               (ATA_MEM_REG_BASE + 0xE)
#define ATA_ALT_REG_LENGTH                  4

//
// ATA drive select/head register bit masks.
//
#define ATA_HEAD_MUST_BE_ON         0xA0 // These 2 bits are always on
#define ATA_HEAD_DRIVE_1            (0x00 | ATA_HEAD_MUST_BE_ON)
#define ATA_HEAD_DRIVE_2            (0x10 | ATA_HEAD_MUST_BE_ON)
#define ATA_HEAD_LBA_MODE           (0x40 | ATA_HEAD_MUST_BE_ON)

//
// ATAPI Interrupt Reason (ATA Sector Count) Definition
//
#define ATA_IR_CoD          0x01    // Command OR Data
#define ATA_IR_IO           0x02    // Input OR Output (Host & Device)
#define ATA_RELEASED        0x04    // ATA Bus Released


//
//  ATAPI Interrupt Reason & Status Reg. Results
//
//                                 DRQ-IO-CoD
#define ATA_INTR_CMD    5       //   1- 0 - 1 (5) ATA Device acknowledge command
#define ATA_INTR_MSG    7       //   1- 1 - 1 (7) ATA Device ready to send MSG (Future) to host
#define ATA_INTR_READ   6       //   1- 1 - 0 (6) ATA Device ready to transfer data to host
#define ATA_INTR_WRITE  4       //   1- 0 - 0 (4) ATA Device ready to receive data from host
#define ATA_INTR_READY  3       //   0- 1 - 1 (3) ATA Device Processing Completed
#define ATA_INTR_ERROR  0       //   Return value in case of Device error
#define ATA_INTR_DMA    8       //   DMA Interrupt

#define ATA_IO_ERROR    9   
#define ATA_IO_SUCCESS  0


//
//  WaitForSingleObject Results
//
#define ATA_WFSO_OK         1
#define ATA_WFSO_TIMEOUT    2
#define ATA_WFSO_ERROR      4


//
// ATA error register bit masks.
//
#define ATA_ERROR_GENERAL          0x01
#define ATA_ERROR_ABORTED          0x04
#define ATA_ERROR_BAD_SECT_NUM     0x10
#define ATA_ERROR_UNCORRECTABLE    0x40
#define ATA_ERROR_BAD_BLOCK        0x80

//
// ATAPI error register bit masks.
//
#define ATAPI_ERR_ILLEGAL_LENGTH            0x01
#define ATAPI_ERR_END_OF_MEDIA              0x02
#define ATAPI_ERR_ABORTED_COMMAND           0x04
#define ATAPI_ERR_MEDIA_CHANGE_REQ          0x08
#define ATAPI_ERR_MEDIA_CHANGED             0x20
#define SENSE_KEYS                          0xF0    // Sense Key Descriptions page 183

// ATAPI Sense Key and Sense code definitions continued from above

#define ATAPI_SENSE_NOSENSE                 0x00
#define ATAPI_SENSE_RECOVERED_ERROR         0x10    
#define ATAPI_SENSE_NOT_READY               0x20    //
#define ATAPI_SENSE_MEDIUM_ERROR            0x30    //
#define ATAPI_SENSE_HARDWARE_ERROR          0x40    //
#define ATAPI_SENSE_ILLEGAL_REQUEST         0x50    //
#define ATAPI_SENSE_UNIT_ATTENTION          0x60    //
#define ATAPI_SENSE_DATA_PROTECT            0x70    //
#define ATAPI_SENSE_ABORTED_COMMAND         0xB0    //
#define ATAPI_SENSE_MISCOMPARE              0xE0    //

//
// ATA commands for the command register.
//
#define ATA_CMD_RECALIBRATE         0x10 // move drive heads to track 0
#define ATA_CMD_READ                0x20 // NO retries enabled
#define ATA_CMD_WRITE               0x30 // No retries enabled
#define ATA_CMD_MULTIPLE_READ       0xC4 // No retries enabled
#define ATA_CMD_MULTIPLE_WRITE      0xC5 // No retries enabled
#define ATA_CMD_SET_MULTIPLE        0xC6 // Set Multiple Mode 
#define ATA_CMD_SEEK                0x70
#define ATA_CMD_SET_DRIVE_PARMS     0x91 // set drive parameters
#define ATA_CMD_IDLE                0x97
#define ATA_CMD_FLUSH_CACHE         0xE7
#define ATA_CMD_IDENTIFY            0xEC // ATA identify drive parameters
#define ATA_CMD_ACKMEDIACHANGE      0xDB // Acknowledge media change
#define ATA_CMD_READ_DMA            0xC8 // DMA Read with retries
#define ATA_CMD_WRITE_DMA           0xCA // DMA Write with retries
#define ATA_CMD_STANDBY_IMMEDIATE   0xE0
#define ATA_CMD_IDLE_IMMEDIATE      0xE1
#define ATA_CMD_STANDBY             0xE2
#define ATA_NEW_CMD_IDLE            0xE3        // replaces the legacy 0x97 value
#define ATA_CMD_CHECK_POWER_MODE    0xE5
#define ATA_CMD_SLEEP               0xE6

//
// ATA commands (extended) for 48 bit LBA mode
//
#define  ATA_CMD_FLUSH_CACHE_EXT                0xEA
#define  ATA_CMD_READ_DMA_EXT                   0x25
#define  ATA_CMD_READ_DMA_QUEUED_EXT            0x26
#define  ATA_CMD_READ_LOG_EXT                   0x2F
#define  ATA_CMD_READ_MULTIPLE_EXT              0x29
#define  ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT    0x27
#define  ATA_CMD_READ_SECTOR_EXT                0x24
#define  ATA_CMD_READ_VERIFY_SECTOR             0x42
#define  ATA_CMD_SET_MAX_ADDRESS_EXT            0x37
#define  ATA_CMD_WRITE_DMA_EXT                  0x35
#define  ATA_CMD_DMA_QUEUED_EXT                 0x36
#define  ATA_CMD_WRITE_LOG_EXT                  0x3F
#define  ATA_CMD_WRITE_MULTIPLE_EXT             0x39
#define  ATA_CMD_WRITE_SECTOR_EXT               0x34

//
// ATA commands for the features register.
//
#define ATA_ENABLE_WRITECACHE   0x02
#define ATA_SET_TRANSFER_MODE   0x03        // 
#define ATA_DISABLE_WRITECACHE  0x82
#define ATA_ENABLE_LOOKAHEAD    0xAA

//
// The following values must be written into sector count register
// before setting transfer mode command into the features register.

#define ATA_PIO_DEFAULT_MODE    0       // PIO Default Mode
#define ATA_PIO_FCT_MODE        0x08    // PIO Flow Control Transfer Mode
#define ATA_DMA_ONE_WORD_MODE   0x10    // DMA Single Word Transfer Mode
#define ATA_DMA_MULTI_WORD_MODE 0x20    // DMA Multi-word Transfer Mode
#define ATA_DMA_PSEUDO_WORD_MOD 0x18    // Reserved  Mode



//
// ATAPI commands for the TASK command register.
//
#define ATAPI_CMD_COMMAND             0xA0 // move drive heads to track 0
#define ATAPI_CMD_IDENTIFY            0xA1 // identify drive parameters
#define ATAPI_CMD_SERVICE             0xA2 // Service
#define ATAPI_CMD_SOFT_RESET          0x08 // Soft Reset
#define ATAPI_CMD_SET_FEATURES        0xEF // Set Features

//
// ATAPI PACKET commands used by the TASK command 0xA0.
//

#define ATAPI_PACKET_CMD_TEST_READY     0x00 // Test unit ready
#define ATAPI_PACKET_CMD_REQUEST_SENSE  0x03 // Request additional status info
#define ATAPI_PACKET_CMD_INQUIRY        0x12 // Inquiry
#define ATAPI_PACKET_CMD_READ           0x28 // Read CD-DA format
#define ATAPI_PACKET_CMD_READ_12        0xA8 // 12 byte read command
#define ATAPI_PACKET_CMD_WRITE          0x2A // Read CD-DA format
#define ATAPI_PACKET_CMD_PAUSE_RESUME   0x4B // Pause/Resume
#define ATAPI_PACKET_CMD_PLAY           0x45 // Play
#define ATAPI_PACKET_CMD_PLAY_MSF       0x47 // Play MSF
#define ATAPI_PACKET_CMD_PLAY_CD        0xBC // Play CD
#define ATAPI_PACKET_CMD_SCAN_AUDIO     0xBA // Scan Audio
#define ATAPI_PACKET_CMD_READ_CAPACITY  0xBB // Read CD-ROM Capacity
#define ATAPI_PACKET_CMD_READ_CD        0xBE // Read CD-DA format
#define ATAPI_PACKET_CMD_READ_CD_MSF    0xB9 // Read CD-DA MSF format
#define ATAPI_PACKET_CMD_READ_HEADER    0x44 // Read Header
#define ATAPI_PACKET_CMD_READ_SUB_CHAN  0x42 // Read Sub Channel info
#define ATAPI_PACKET_CMD_READ_TOC       0x43 // Read Table of Contents
#define ATAPI_PACKET_CMD_SEEK           0x2B // Seek
#define ATAPI_PACKET_CMD_STOP_PLAY_SCAN 0x4E // Stop Play/Scan
#define ATAPI_PACKET_CMD_START_STOP     0x1B // Start/Stop Unit
#define ATAPI_PACKET_CMD_MODE_SENSE     0x5a // Mode Sense
#define ATAPI_PACKET_CMD_MODE_SELECT    0x55 // Mode Select
#define ATAPI_PACKET_CMD_SET_SPEED      0xBB // Set Speed
#define ATAPI_PACKET_CMD_READ_DISC_INFO 0x51 // Read Disc Info

// DVD PACKET commands

    
//
// ATA status register bit masks.
//
#define ATA_STATUS_ERROR            0x01 // error bit in status register
#define ATA_STATUS_CORRECTED_ERROR  0x04 // corrected error in status register
#define ATA_STATUS_DATA_REQ         0x08 // data request bit in status register
#define ATA_STATUS_SEEK_DONE        0x10 // DSC - Drive Seek Complete
#define ATA_STATUS_WRITE_FAULT      0x20 // DWF - Drive Write Fault
#define ATA_STATUS_READY            0x40
#define ATA_STATUS_IDLE             0x50
#define ATA_STATUS_BUSY             0x80

//
// ATA drive select/head register bit masks.
//
#define ATA_HEAD_MUST_BE_ON         0xA0 // These 2 bits are always on
#define ATA_HEAD_DRIVE_1            (0x00 | ATA_HEAD_MUST_BE_ON)
#define ATA_HEAD_DRIVE_2            (0x10 | ATA_HEAD_MUST_BE_ON)
#define ATA_HEAD_LBA_MODE           (0x40 | ATA_HEAD_MUST_BE_ON)

//
// ATA device control register bit masks.
//
#define ATA_CTRL_ENABLE_INTR        0x00
#define ATA_CTRL_DISABLE_INTR       0x02
#define ATA_CTRL_RESET              0x04


//
// IDENTIFY capability bit definitions.
//

//#define IDENTIFY_CAPABILITIES_DMA_SUPPORTED 0x0100
//#define IDENTIFY_CAPABILITIES_LBA_SUPPORTED 0x0200

//
// IDENTIFY DMA timing cycle modes.
//

//
// IDE Cycle Timing
//
#define PIO_MODE0_CYCLE_TIME        600
#define PIO_MODE1_CYCLE_TIME        383
#define PIO_MODE2_CYCLE_TIME        240
#define PIO_MODE3_CYCLE_TIME        180
#define PIO_MODE4_CYCLE_TIME        120

#define SWDMA_MODE0_CYCLE_TIME      960
#define SWDMA_MODE1_CYCLE_TIME      480
#define SWDMA_MODE2_CYCLE_TIME      240

#define MWDMA_MODE0_CYCLE_TIME      480
#define MWDMA_MODE1_CYCLE_TIME      150
#define MWDMA_MODE2_CYCLE_TIME      120

#define UDMA_MODE0_CYCLE_TIME       120
#define UDMA_MODE1_CYCLE_TIME       80
#define UDMA_MODE2_CYCLE_TIME       60
#define UDMA_MODE3_CYCLE_TIME       45
#define UDMA_MODE4_CYCLE_TIME       30

//
// IDENTIFY DMA timing cycle modes.
//

#define UNINITIALIZED_CYCLE_TIME    0xffffffff
#define UNINITIALIZED_TRANSFER_MODE 0xffffffff
#define IDENTIFY_DMA_CYCLES_MODE_0 0x00
#define IDENTIFY_DMA_CYCLES_MODE_1 0x01
#define IDENTIFY_DMA_CYCLES_MODE_2 0x02


//
// Identify Data General Configuration Bit Definition
//
#define IDE_IDDATA_DEVICE_TYPE_MASK          ((1 << 15) | (1 << 14))
#define IDE_IDDATA_ATAPI_DEVICE              (1 << 15)

#define IDE_IDDATA_ATAPI_DEVICE_MASK         ((1 << 12) | (1 << 11) | (1 << 10) | (1 << 9) | (1 << 8))

#define IDE_IDDATA_REMOVABLE                 (1 << 7)

#define IDE_IDDATA_DRQ_TYPE_MASK             ((1 << 6) | (1 << 5))
#define IDE_IDDATA_INTERRUPT_DRQ             ((1 << 6) | (0 << 5))


//
//  Peripheral Device Types 
//
#define ATA_IDDEVICE_DISK           0  // DIRECT ACCESS DEVICE
#define ATA_IDDEVICE_CDROM          5
#define ATA_IDDEVICE_OPTICAL_MEM    7
#define ATA_IDDEVICE_UNKNOWN        0x1F

#define PIO_MODE2           (1 << 2)
#define PIO_MODE3           (1 << 3)
#define PIO_MODE4           (1 << 4)

#define SWDMA_MODE0         (1 << 5)
#define SWDMA_MODE1         (1 << 6)
#define SWDMA_MODE2         (1 << 7)

#define MWDMA_MODE0         (1 << 8)
  
#define PIO_MODE0           (1 << 0)
#define PIO_MODE1           (1 << 1)
#define MWDMA_MODE1         (1 << 9)
#define MWDMA_MODE2         (1 << 10)

#define UDMA_MODE0          (1 << 11)
#define UDMA_MODE1          (1 << 12)
#define UDMA_MODE2          (1 << 13)
#define UDMA_MODE3          (1 << 14)
#define UDMA_MODE4          (1 << 15)

#define PIO_SUPPORT         (PIO_MODE0      | PIO_MODE1     | PIO_MODE2    | PIO_MODE3     | PIO_MODE4)
#define SWDMA_SUPPORT       (SWDMA_MODE0    | SWDMA_MODE1   | SWDMA_MODE2)
#define MWDMA_SUPPORT       (MWDMA_MODE0    | MWDMA_MODE1   | MWDMA_MODE2)
#define UDMA33_SUPPORT      (UDMA_MODE0     | UDMA_MODE1    | UDMA_MODE2)
#define UDMA66_SUPPORT      (UDMA_MODE3     | UDMA_MODE4)
#define UDMA_SUPPORT        (UDMA33_SUPPORT | UDMA66_SUPPORT    )

#define DMA_SUPPORT         (SWDMA_SUPPORT  | MWDMA_SUPPORT | UDMA_SUPPORT)
#define ALL_MODE_SUPPORT    (PIO_SUPPORT | DMA_SUPPORT)

//
// ID DATA
//
#define IDD_UDMA_MODE0_ACTIVE           (1 << 0)
#define IDD_UDMA_MODE1_ACTIVE           (1 << 1)
#define IDD_UDMA_MODE2_ACTIVE           (1 << 2)
#define IDD_UDMA_MODE3_ACTIVE           (1 << 3)
#define IDD_UDMA_MODE4_ACTIVE           (1 << 4)

#define IDD_MWDMA_MODE0_ACTIVE          (1 << 0)
#define IDD_MWDMA_MODE1_ACTIVE          (1 << 1)
#define IDD_MWDMA_MODE2_ACTIVE          (1 << 2)

#define IDD_SWDMA_MODE0_ACTIVE          (1 << 0)
#define IDD_SWDMA_MODE1_ACTIVE          (1 << 1)
#define IDD_SWDMA_MODE2_ACTIVE          (1 << 2)

#define IDD_UDMA_MODE0_SUPPORTED        (1 << 0)
#define IDD_UDMA_MODE1_SUPPORTED        (1 << 1)
#define IDD_UDMA_MODE2_SUPPORTED        (1 << 2)

#define IDD_MWDMA_MODE0_SUPPORTED       (1 << 0)
#define IDD_MWDMA_MODE1_SUPPORTED       (1 << 1)
#define IDD_MWDMA_MODE2_SUPPORTED       (1 << 2)

#define IDD_SWDMA_MODE0_SUPPORTED       (1 << 0)
#define IDD_SWDMA_MODE1_SUPPORTED       (1 << 1)
#define IDD_SWDMA_MODE2_SUPPORTED       (1 << 2)

//
// Beautification macros
//
// IDENTIFY capability bit definitions.
//

#define IDENTIFY_CAPABILITIES_DMA_SUPPORTED             (1 << 8)
#define IDENTIFY_CAPABILITIES_LBA_SUPPORTED             (1 << 9)
#define IDENTIFY_CAPABILITIES_IOREADY_CAN_BE_DISABLED   (1 << 10)
#define IDENTIFY_CAPABILITIES_IOREADY_SUPPORTED         (1 << 11)

//
// Command set bit definitions
//
#define IDENTIFY_COMMAND_SET_POWER_MANAGEMENT           (1 << 3)
#define IDENTIFY_COMMAND_SET_WRITE_CACHE                (1 << 5)

//
// Command set supported bit definitions
//
#define COMMAND_SET_WRITE_CACHE_SUPPORTED               IDENTIFY_COMMAND_SET_WRITE_CACHE
#define COMMAND_SET_POWER_MANAGEMENT_SUPPORTED          IDENTIFY_COMMAND_SET_POWER_MANAGEMENT

//
// Command set enabled bit definitions
//
#define COMMAND_SET_WRITE_CACHE_ENABLED                 IDENTIFY_COMMAND_SET_WRITE_CACHE
#define COMMAND_SET_POWER_MANAGEMENT_ENABLED            IDENTIFY_COMMAND_SET_POWER_MANAGEMENT

//
//  This command causes all members of the following structure to be stored in packed 
//  stored in packedform e.i  without extra space for alignment.
//

 

//
// ATA/ATAPI-6 definition of IDENTIFY_DEVICE results with 48-bit LBA support.
//
#pragma pack(1)
typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00   Mandatory for ATAPI
    USHORT NumberOfCylinders;               // 01   Not used for ATAPI
    USHORT Reserved1;                       // 02   Not used for ATAPI
    USHORT NumberOfHeads;                   // 03   Not used for ATAPI
    USHORT UnformattedBytesPerTrack;        // 04   Not used for ATAPI
    USHORT UnformattedBytesPerSector;       // 05   Not used for ATAPI
    USHORT SectorsPerTrack;                 // 06   Not used for ATAPI
    USHORT VendorUnique1[3];                // 07-09    Not used for ATAPI
    USHORT SerialNumber[10];                // 10   Optional for ATAPI
    USHORT BufferType;                      // 20   Not used for ATAPI
    USHORT BufferSectorSize;                // 21   Not used for ATAPI
    USHORT NumberOfEccBytes;                // 22   Not used for ATAPI
    USHORT FirmwareRevision[4];             // 23   Mandatory for ATAPI
    USHORT ModelNumber[20];                 // 27   Mandatory for ATAPI
    UCHAR  MaximumBlockTransfer;            // 47 low byte     Not used for ATAPI
    UCHAR  VendorUnique2;                   // 47 high byte    Not used for ATAPI
    USHORT DoubleWordIo;                    // 48   Not used for ATAPI
    USHORT Capabilities;                    // 49   Mandatory for ATAPI
    USHORT Capabilities2;                   // 50 bit 0 = 1 to indicate a device specific Standby timer value minimum
    UCHAR  VendorUnique3;                   // 51 low byte      Mandatory for ATAPI
    UCHAR  PioCycleTimingMode;              // 51 high byte     Mandatory for ATAPI
    UCHAR  VendorUnique4;                   // 52 low byte      Mandatory for ATAPI
    UCHAR  DmaCycleTimingMode;              // 52 high byte     Mandatory for ATAPI
    USHORT TranslationFieldsValid;          // 53 (low bit)     Mandatory for ATAPI
    USHORT NumberOfCurrentCylinders;        // 54   Not used for ATAPI
    USHORT NumberOfCurrentHeads;            // 55   Not used for ATAPI
    USHORT CurrentSectorsPerTrack;          // 56   Not used for ATAPI
    ULONG  CurrentSectorCapacity;           // 57 & 58          Not used for ATAPI
    UCHAR  MultiSectorCount;                // 59 low           Not used for ATAPI
    UCHAR  MultiSectorSettingValid;         // 59 high (low bit)Not used for ATAPI
    ULONG  TotalUserAddressableSectors;     // 60 & 61          Not used for ATAPI
    UCHAR  SingleDmaModesSupported;         // 62 low byte      Mandatory for ATAPI
    UCHAR  SingleDmaTransferActive;         // 62 high byte     Mandatory for ATAPI
    UCHAR  MultiDmaModesSupported;          // 63 low byte      Mandatory for ATAPI
    UCHAR  MultiDmaTransferActive;          // 63 high byte     Mandatory for ATAPI
    UCHAR  AdvancedPIOxferreserved;         // 64 low byte      Mandatory for ATAPI
    UCHAR  AdvancedPIOxfer;                 // 64 high byte     Mandatory for ATAPI
    USHORT MinimumMultiwordDMATime;         // 65 Mandatory for ATAPI
    USHORT ManuRecomendedDMATime;           // 66 Mandatory for ATAPI
    USHORT MinimumPIOxferTimeWOFlow;        // 67 Mandatory for ATAPI
    USHORT MinimumPIOxferTimeIORDYFlow;     // 68 Mandatory for ATAPI
    USHORT ReservedADVPIOSupport[2];        // 69 Not used for ATAPI
    USHORT TypicalProcTimeForOverlay;       // 71 Optional for ATAPI
    USHORT TypicalRelTimeForOverlay;        // 72 Optional for ATAPI
    USHORT MajorRevisionNumber;             // 73 Optional for ATAPI
    USHORT MinorRevisionNumber;             // 74 Optional for ATAP  
    USHORT QueueDepth;                      // 75
    USHORT Reserved6[4];                    // 76-79
    USHORT MajorVersionNumber;              // 80
    USHORT MinorVersionNumber;              // 81
    USHORT CommandSetSupported1;            // 82
    USHORT CommandSetSupported2;            // 83
    USHORT CommandSetFeaturesSupported;     // 84
    USHORT CommandSetFeatureEnabled1;       // 85
    USHORT CommandSetFeatureEnabled2;       // 86
    USHORT CommandSetFeatureDefault ;       // 87
    UCHAR  UltraDMASupport;                 // 88 Low
    UCHAR  UltraDMAActive;                  // 88 High
    USHORT TimeRequiredForSecurityErase;    // 89 Time Required For Security Erase Unit Completion
    USHORT TimeReuiregForEnhancedSecurtity; // 90 Time Required For Enhanced Security Erase Unit Completion
    USHORT CurrentAdvancePowerMng;          // 91 CurrentAdvanced Power Managemnt Value
    USHORT MasterPasswordRevisionCode;      // 92 Master Password Revision Code
    USHORT HardwareResetResult;             // 93 Hardware Reset Result
    UCHAR  CurrentAcousticManagement;       // 94 Acoustic Management (low byte = current; high byte = vendor recommended)
    UCHAR  VendorAcousticManagement;       
    USHORT Reserved7a[99-95+1];             // 95-99
    ULONG  lMaxLBAAddress[2];               // 100-103 Maximum User LBA for 48-bit Address feature set
    USHORT Reserved7b[126-104+1];           // 104-126
    USHORT MediaStatusNotification:2;       // 127 Removable Media Status Notification Support
    USHORT SecurityStatus;                  // 128 Security Status
    USHORT Reserved8[31];                   // 129-159 Vendor Specific
    USHORT CFAPowerMode1;                   // 160
    USHORT Reserved9[94];                   // 161-254
    USHORT IntegrityWord;                   // 255 Checksum & Signature    
} IDENTIFY_DATA, *PIDENTIFY_DATA;
#pragma pack()

//
// ATAPI typical command packet setup
//
typedef struct _ATAPI_COMMAND_PACKET {
    UCHAR Opcode;               // 00   ATAPI opcode
    UCHAR Byte_1;              // 01    reserved
    UCHAR Byte_2;              // 02    Starting LBA MSB
    UCHAR Byte_3;              // 03    LBA
    UCHAR Byte_4;              // 04    LBA
    UCHAR Byte_5;              // 05    LBA MSB
    UCHAR Byte_6;              // 06    X-FER length MSB
    UCHAR Byte_7;              // 07    X-FER length
    UCHAR Byte_8;              // 08    X-FER length or LSB if opcode is BEh
    UCHAR Byte_9;              // 09    X-FER length LSB / or FLAG bits
    UCHAR Byte_10;             // 10    MISC.
    UCHAR Byte_11;             // 11    reserved
    UCHAR Byte_12;             // 12    reserved
    UCHAR Byte_13;             // 13    reserved
    UCHAR Byte_14;             // 14    reserved
    UCHAR Byte_15;             // 15    reserved
    HANDLE pIoReq;             // Pointer to the corresponding Ioctl Request
} ATAPI_COMMAND_PACKET, *PATAPI_COMMAND_PACKET;

#define BEW_TO_W(x) ((x << 8) + (x >> 8))
#define W_TO_BEW(x) ((x << 8) + (x >> 8))

#define BEDW_TO_DW(x) ((x >> 24) + ((x & 0x00ff0000) >> 8) + ((x & 0x0000ff00) << 8) + ( x << 24))
#define DW_TO_BEDW(x) ((x >> 24) + ((x & 0x00ff0000) >> 8) + ((x & 0x0000ff00) << 8) + ( x << 24))

#define LBA_LSB(plba)       (*((BYTE *)(((BYTE *)plba)) + 0))
#define LBA_2ndLSB(plba)    (*((BYTE *)(((BYTE *)plba)) + 1))
#define LBA_3rdLSB(plba)    (*((BYTE *)(((BYTE *)plba)) + 2))
#define LBA_4thLSB(plba)    (*((BYTE *)(((BYTE *)plba)) + 3))
#define LBA_MSB(plba)       LBA_4thLSB(plba)


//
// NOTE: I'm not sure where the following information was pulled from - perhaps 
// an early ATA/PI document.  According to revision 6:
// 00 = Device shall set DRQ to one within 3 ms of receiving PACKET command
// 01 = Obsolete
// 10 = Device shall set DRQ to one within 50 us of receiving PACKET command
// 11 = Reserved
//
#define ATA_DRQTYPE_INTRQ         1   // INTRQ will be asserted with DRQ within 10ms
#define ATA_DRQTYPE_ACCDRQ        2   // no INTRQ but DRQ will be asserted within 50 us of
                                        // issuing ATAPI packet command (0xA0)


//
// ATAPI TOC data struct 
// Data returned by the ATAPI_PACKET_CMD_READ_TOC command
//
typedef struct _ATAPI_TOC_DATA {
    USHORT TOC_Data[0xFF];              
} ATAPI_TOC_DATA, *PATAPI_TOC_DATA;

typedef struct _INQUIRY_DATA {
    UCHAR   inqDevType;
    UCHAR   inqRMB;
    UCHAR   inqVersion;
    UCHAR   inqAtapiVersion;
    UCHAR   inqLength;
    UCHAR   inqReserved[3];
    UCHAR   inqVendor[8];
    UCHAR   inqProdID[16];
    UCHAR   inqRev[4];
    UCHAR   inqReserved2[60];
} INQUIRY_DATA, *PINQUIRY_DATA;

#endif //_ATAPI_H_

