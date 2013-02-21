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

// Copyright (c) 2002-2004 BSQUARE Corporation.  All rights reserved.
// DO NOT REMOVE --- BEGIN EXTERNALLY DEVELOPED SOURCE CODE ID 40973--- DO NOT REMOVE

// Core header file for SD Card DDK

#ifndef _SDCARD_DDK_DEFINED
#define _SDCARD_DDK_DEFINED


#include <windows.h>
#include <types.h>
#include <linklist.h>
#include <ceddk.h>

#include "SDMem.h"
#include "SDCard.h"

#define SD_CSD_VERSION_CODE_1_0         0
#define SD_CSD_VERSION_CODE_2_0         1

#define SD_CSD_VERSION_CODE_SUPPORTED   SD_CSD_VERSION_CODE_2_0 
#define MMC_CSD_VERSION_CODE_SUPPORTED  2 // version 3.1
#define HSMMC_CSD_SPEC_VERSION_CODE_SUPPORTED 4 // version 4.x

#ifdef _MMC_SPEC_42_ 
// an argument value of CMD1 to support sector address mode, dual voltage 
#define OCR_SECTOR_SUPPORT    0x40FF8080 
#endif   


typedef PVOID SD_DEVICE_HANDLE;

// typedef for API Status codes
typedef LONG SD_API_STATUS;

// transfer class enum, indicates whether a bus request is read,write or a command
typedef enum { 
    SD_READ = 0,         // block read
    SD_WRITE = 1,        // block write
    SD_COMMAND = 2,      // command only
} SD_TRANSFER_CLASS;

// SDIO Read/Write Enum
typedef enum { 
    SD_IO_READ = 0, 
    SD_IO_WRITE = 1,  
} SD_IO_TRANSFER_TYPE;

// indicates Interface being used for a card
typedef enum {  
    SD_INTERFACE_SD_MMC_1BIT = 0, 
    SD_INTERFACE_SD_4BIT = 1, 
    SD_INTERFACE_MMC_8BIT = 2
} SD_INTERFACE_MODE;

#ifdef _MMC_SPEC_42_ 
typedef union {
  struct {
    ULONG sd4Bit:1;
    ULONG sdHighSpeed:1;
    ULONG hsmmc8Bit:1;
    ULONG :27;
    ULONG sdWriteProtected:1;
    ULONG sdHighCapacity:1;
  } bit;
  ULONG uInterfaceMode;
} SD_INTERFACE_MODE_EX,*PSD_INTERFACE_MODE_EX;
#else
typedef union {
    struct {
        ULONG sd4Bit:1;
        ULONG sdHighSpeed:1;
        ULONG :28;
        ULONG sdWriteProtected:1;
        ULONG sdHighCapacity:1;
    } bit;
    ULONG uInterfaceMode;
} SD_INTERFACE_MODE_EX,*PSD_INTERFACE_MODE_EX;
#endif

// enum for querying card information, some of this information is cached
typedef enum { 
    SD_INFO_REGISTER_OCR = 0,       // 32-bit OCR cached value for MMC/SDMemory Cards
    SD_INFO_REGISTER_CID,           // CID for MMC/SDMemory Cards
    SD_INFO_REGISTER_CSD,           // CSD for MMC/SDMemory Cards
    SD_INFO_REGISTER_RCA,           // RCA data type for MMC/SDMemory/SD IO Cards
    SD_INFO_REGISTER_IO_OCR,        // 24 bit IO-OCR register for SDIO Cards
    SD_INFO_CARD_INTERFACE,         // card interface for all card types
    SD_INFO_CARD_STATUS,            // card status for MMC/SD Memory cards
    SD_INFO_SDIO,                   // SDIO information (SD IO only)
    SD_INFO_HOST_IF_CAPABILITIES,   // SD Host Interface Capabilities
    SD_INFO_HOST_BLOCK_CAPABILITY,  // SD Host Block Length Capabilities
    SD_INFO_HIGH_CAPACITY_SUPPORT,  // SD Memory High Capacity
    SD_INFO_TYPE_COUNT,             // number of SD_INFO_TYPEs
    SD_INFO_CARD_INTERFACE_EX,      // CARD extension bit after 1.10
    SD_INFO_SWITCH_FUNCTION,        // SWITCH Function, Read Current Supported Function.    
#ifdef _MMC_SPEC_42_
    MMC_INFO_REGISTER_EXTCSD,       // Extended CSD register 
#endif 
} SD_INFO_TYPE;

// enum for device type
typedef enum _SDCARD_DEVICE_TYPE {
    Device_Unknown = 0,
    Device_MMC = 1,
    Device_SD_Memory = 2,
    Device_SD_IO = 3,
    Device_SD_Combo = 4  // combo is only an intermediate device type reserved only for the bus driver
#ifdef _MMC_SPEC_42_
    ,
    Device_HSMMC40 = 5,
    Device_HSMMC42 = 6
#endif
}SDCARD_DEVICE_TYPE, *PSDCARD_DEVICE_TYPE;

// enum for setting SD Card Features
typedef enum { 
    SD_IO_FUNCTION_ENABLE = 0,          // enable I/O Function, uses SD_IO_FUNCTION_ENABLE_INFO
    SD_IO_FUNCTION_DISABLE,             // disable I/O Function - no structure required
    SD_IO_FUNCTION_SET_BLOCK_SIZE,      // set the SDIO Function block size,takes a DWORD parameter
    SD_SET_DATA_TRANSFER_CLOCKS,        // set read and write data transfer clocks
    SD_SET_CARD_INTERFACE,              // set the physical card interface
    SD_SET_CLOCK_STATE_DURING_IDLE,     // set the clock state during the idle state
    SD_IS_SOFT_BLOCK_AVAILABLE,         // Determine if Soft-Block is available in the SD Bus driver.
    SD_SOFT_BLOCK_FORCE_UTILIZATION,    // Always use Soft-Block (testing use only).
    SD_SOFT_BLOCK_DEFAULT_UTILIZATON,   // Enable Soft-Block use.
    SD_CARD_SELECT_REQUEST,             // request the card to be selected
    SD_CARD_DESELECT_REQUEST,           // request the card to be deselected
    SD_CARD_FORCE_RESET,                // force the card to be reset
    SD_IS_FAST_PATH_AVAILABLE,          // Determine if Fast-Path is available in the SD Bus driver.
    SD_FAST_PATH_DISABLE,               // Disable Fast-Path use (testing use only).
    SD_FAST_PATH_ENABLE,                // Enable Fast-Path use.
    SD_IO_FUNCTION_HIGH_POWER,          // Select High Power mode for Function
    SD_IO_FUNCTION_LOW_POWER,           // Select Low Power mode for Function
    SD_INFO_POWER_CONTROL_STATE,         // Get functions power state
    SD_SET_CARD_INTERFACE_EX,           // set the physical card interface with extended structure SD_CARD_INTERFACE_EX.
    SD_SET_SWITCH_FUNCTION,             // For 1.1 Memory card to switch Function.
    SD_DMA_ALLOC_PHYS_MEM,              // Optional, Allocate DMAable Physical Memory,
    SD_DMA_FREE_PHYS_MEM                // Optional, Free DMAable Pysical Memor located by SD_DMA_ALLOC_PHYS_MEM    
} SD_SET_FEATURE_TYPE, *PSD_SET_FEATURE_TYPE;

// structure for SD_IO_FUNCTION_ENABLE card feature
typedef struct _SD_IO_FUNCTION_ENABLE_INFO{
    ULONG   ReadyRetryCount;    // number of retries to check the ready bit in the function
    ULONG   Interval;           // interval in milliseconds to check
}SD_IO_FUNCTION_ENABLE_INFO, *PSD_IO_FUNCTION_ENABLE_INFO;

// info for SD_SET_DATA_TRANSFER_CLOCKS card feature
typedef struct _SD_DATA_TRANSFER_CLOCKS {
    ULONG   ReadClocks;         // the worst case number of read clocks before read data is sent 
    ULONG   WriteClocks;        // number of clocks that busy signal can remain assrted
} SD_DATA_TRANSFER_CLOCKS, *PSD_DATA_TRANSFER_CLOCKS;

// structure for information about a card's interface
typedef struct _SD_CARD_INTERFACE {
    SD_INTERFACE_MODE   InterfaceMode;  // interface mode
    ULONG               ClockRate;      // clock rate
    BOOL                WriteProtected; // write protect flag (SD Memory cards)
} SD_CARD_INTERFACE, *PSD_CARD_INTERFACE;

// structure for information about a card's interface
typedef struct _SD_CARD_INTERFACE_EX {
    SD_INTERFACE_MODE_EX InterfaceModeEx;  // Extended interface mode
    ULONG               ClockRate;      // clock rate
    ULONG               Reserved[6];    // Reserved should be zero.
} SD_CARD_INTERFACE_EX, *PSD_CARD_INTERFACE_EX;

// structure for SDIO device information
typedef struct _SDIO_CARD_INFO{
    UCHAR   FunctionNumber;   // SDIO function number
    UCHAR   DeviceCode;       // device interface code for this function
    DWORD   CISPointer;       // CIS pointer for this function
    DWORD   CSAPointer;       // CSA pointer for this function
    UCHAR   CardCapability;   // common card capabilities
} SDIO_CARD_INFO, *PSDIO_CARD_INFO;

// structure for SD_INFO_HOST_BLOCK_CAPABILITY card information
typedef struct _SD_HOST_BLOCK_CAPABILITY{
    USHORT  ReadBlockSize;   // block size for read requests
    USHORT  WriteBlockSize;  // block size for write requests
    USHORT  ReadBlocks;      // number of blocks for each read request
    USHORT  WriteBlocks;     // number of blocks for each write request
} SD_HOST_BLOCK_CAPABILITY, *PSD_HOST_BLOCK_CAPABILITY;

// structure for SDHCAllocateDMABuffer and SDHCFreeDMABuffer
typedef struct {
    ULONG               Length;
    BOOLEAN             CacheEnabled;
    PVOID               VirtualAddress;
    PHYSICAL_ADDRESS    LogicalAddress;
} SD_HOST_ALLOC_FREE_DMA_BUFFER,*PSD_HOST_ALLOC_FREE_DMA_BUFFER;

typedef struct _SD_CARD_EXTENSION {
    union {
        struct {
            ULONG   HighCapacity:1;  // High Capacity card for 2.0.
            ULONG   Reserved:31;
        } bit;
        ULONG   ulHighCapacityValue;
    };
    ULONGLONG   ullDeviceSize;
    ULONG   Reserved[5];        // Reserved for future.
} SD_CARD_EXTENSION, *PSD_CARD_EXTENSION;
typedef struct _SD_CARD_SWITCH_FUNCTION {
    DWORD   dwSelectedFunction; // 6 function group. each one has 4 bits.
    DWORD   dwMaxCurrentAllowed;// Only allow dwMaxCurrentAllowed mA for switch to new Function
    DWORD   dwTimeOut;          // Ticks for time out the select.
    BYTE    clientData[512/8];    // Client Data which is provided by Switch function.
} SD_CARD_SWITCH_FUNCTION, *PSD_CARD_SWITCH_FUNCTION;

// enum for response types
typedef enum  _SD_RESPONSE_TYPE {
    NoResponse  = 0,
    ResponseR1  = 1,
    ResponseR1b = 2,
    ResponseR2  = 3,
    ResponseR3  = 4, 
    ResponseR4  = 5,
    ResponseR5  = 6,
    ResponseR6  = 7,
    ResponseR7  = 8
} SD_RESPONSE_TYPE, *PSD_RESPONSE_TYPE;

// fast path event callback event types
typedef enum _SD_SLOT_EVENT_TYPE {
    SDCardEjected = 1,              // card ejected
    SDCardSelected = 2,             // card selected
    SDCardDeselected = 3,           // card deselected
    SDCardBeginSelectDeselect = 4,  // Start to do Select_Deselect 
    SDCardDeselectRequest = 5,      // bus driver received card deselect request
    SDCardSelectRequest = 6         // bus driver received card select request
}SD_SLOT_EVENT_TYPE, *PSD_SLOT_EVENT_TYPE;


// maximum length of a debug string
#define MAXIMUM_DEBUG_STRING_LENGTH 384
#define MAX_KEY_NAME_LENGTH    32
#define MAX_KEY_PATH_LENGTH    256
#define MAX_SDCARD_CLIENT_NAME 32
#define MAX_FILE_NAME_LENGTH   256

// 64K debug file
#define DEFAULT_MAX_DEBUG_FILE_LENGTH 0x10000

// forward declaration
typedef struct _SD_BUS_REQUEST *PSD_BUS_REQUEST;

// typedef for bus request completion callback
typedef VOID (*PSD_BUS_REQUEST_CALLBACK) (SD_DEVICE_HANDLE,
                                          PSD_BUS_REQUEST,
                                          PVOID,
                                          DWORD);

typedef struct _SD_COMMAND_RESPONSE {
    SD_RESPONSE_TYPE ResponseType;                                  // response type
    UCHAR            ResponseBuffer[SDCARD_RESPONSE_BUFFER_BYTES];  // largest possible reponse buffer
}SD_COMMAND_RESPONSE, *PSD_COMMAND_RESPONSE;

typedef struct __PHYS_BUFF_LIST {
    PHYSICAL_ADDRESS    PhysAddr;
    UINT32              PhysLen;
} PHYS_BUFF_LIST , *PPHYS_BUFF_LIST;

// structure to identify a bus request, used to cancel/track request
typedef struct _SD_BUS_REQUEST {
    LIST_ENTRY  ListEntry;          // list entry
    SD_DEVICE_HANDLE hDevice;       // the device this request belongs to
    ULONG       SystemFlags;        // system flags
    SD_TRANSFER_CLASS TransferClass; // transfer class
    UCHAR       CommandCode;        // command code
    DWORD       CommandArgument;    // command argument 
    SD_COMMAND_RESPONSE CommandResponse;  // command response
    DWORD       RequestParam;       // optional request parameter
    SD_API_STATUS Status;           // completion status
    ULONG       NumBlocks;          // number of blocks
    ULONG       BlockSize;          // size of each block   
    DWORD       HCParam;            // host controller parameter, reserved for HC drivers
    PUCHAR      pBlockBuffer;       // buffer holding block data
    PSD_BUS_REQUEST_CALLBACK pCallback;  // callback when the request completes
    DWORD       DataAccessClocks;   // data access clocks for data transfers (READ or WRITE), reserved for HC driver
    // for reads, this is the typical clock delay for the first read data byte
    // for writes, this is the typical clock delay for the write operation to finish
    DWORD       Flags;              // request flags
    DWORD       cbSizeOfPhysList;
    PPHYS_BUFF_LIST pPhysBuffList;    
} SD_BUS_REQUEST, *PSD_BUS_REQUEST;

// request flags
#define SD_AUTO_ISSUE_CMD12     0x00000001  // automatically issue CMD12 after the command
#define SD_SDIO_AUTO_IO_ABORT   0x00000002  // (SDIO Only) automatically abort I/O using CMD52 

#ifdef _FOR_MOVI_NAND_
/**
 * Description : It means that make option request to send Cmd 23 ( SET_BLOCK_LENGTH )
 */
#define SD_MOVINAND_PRE_DEFINE  0x00000010
#endif

// unspecified access clocks 
#define SD_UNSPECIFIED_ACCESS_CLOCKS 0xFFFFFFFF

// some macros to get fields from the bus request structure
#define SD_BUS_REQUEST_RESPONSE(request) ((request)->CommandResponse)
#define SD_BUS_REQUEST_STATUS(request)   ((request)->Status)
#define SD_SYNCHRONOUS_REQUEST  0x00000004  // This is a synchronous request from the bus driver.
#define SD_SLOTRESET_REQUEST    0x00000008  // This is a request for resetting the slot

    // bus request System Flags
    //     Bits 27-0 are defined in SDBusDriver.h
#define SD_FAST_PATH_AVAILABLE  0x80000000  // Fast-Path is available for this request.
#define SD_SET_EVENT_FLAG       0x40000000  // Set the event flag upon request completion.
#define SD_BUS_REQUEST_PHYS_BUFFER 0x00008000

// request queue
typedef struct _SD_REQUEST_QUEUE {
    LIST_ENTRY      ListHead;           // list head for all bus requests
    ULONG           QueueCount;         // number of entries
}SD_REQUEST_QUEUE, *PSD_REQUEST_QUEUE;

// definition of interrupt callback for SDIO cards
typedef SD_API_STATUS (*PSD_INTERRUPT_CALLBACK) (SD_DEVICE_HANDLE,   // device handle
                                                 PVOID);             // device context

// typedef for fast-path slot event callback
typedef VOID (*PSD_SLOT_EVENT_CALLBACK) (SD_DEVICE_HANDLE,           // handle
                                         PVOID,                      // device context
                                         SD_SLOT_EVENT_TYPE,         // slot event type
                                         PVOID,                      // slot event data (optional)
                                         DWORD);                     // slot event data size (optional

// bit fields for client registration flags    
#define SD_CLIENT_HANDLES_RETRY 0x00000001         // client driver handles retries

// client registration structure
typedef struct _SDCARD_CLIENT_REGISTRATION_INFO {
    TCHAR                       ClientName[MAX_SDCARD_CLIENT_NAME]; // client name
    PSD_SLOT_EVENT_CALLBACK     pSlotEventCallBack;                 // slot event callback
    DWORD                       ClientFlags;                        // flags
    DWORD                       Reserved1;                          // future use
    DWORD                       Reserved2;                          // future use        
}SDCARD_CLIENT_REGISTRATION_INFO, *PSDCARD_CLIENT_REGISTRATION_INFO;

// parsed CID structure
typedef struct _SD_PARSED_REGISTER_CID{
    UCHAR  ManufacturerID;              // 8 bit manufacturer ID
    CHAR   OEMApplicationID[3];         // 2 character ASCII string NULL terminated
    CHAR   ProductName[7];              // product name string 6 (to accomodate MMC cards) ASCII chars + 1 nul char
    UCHAR  MajorProductRevision;        // 4 bit major
    UCHAR  MinorProductRevision;        // 4 bit minor rev
    DWORD  ProductSerialNumber;         // 32 bit product serial number
    UCHAR  ManufacturingMonth;          // Month, 1=Jan, 12=Dec
    USHORT ManufacturingYear;           // Decoded year, range 2000 upwards
    UCHAR  RawCIDRegister[16];          // raw data
} SD_PARSED_REGISTER_CID, *PSD_PARSED_REGISTER_CID;

// file System type present on card
typedef enum {
    SD_FS_FAT_PARTITION_TABLE,
    SD_FS_FAT_NO_PARTITION_TABLE,
    SD_FS_UNIVERSAL,
    SD_FS_OTHER
} SD_FS_TYPE, *PSD_FS_TYPE;

// parsed CSD structure
typedef struct _SD_PARSED_REGISTER_CSD {
    UCHAR CSDVersion;               // 4 bit CSD version
#ifdef _MMC_SPEC_42_ 
    UCHAR SpecVersion;      // spec. version information 
    ULONG SectorCount;
#endif       // 4 bit CSD version
    struct {
        DOUBLE TAAC;                // The TAAC field in units of nano-seconds
        USHORT NSAC;                // The NSAC field in units of clock-cycle 
    } DataAccessTime;
    ULONG MaxDataTransferRate;      // In units of kbit/s
    USHORT CardCommandClasses;      // 12 bit command class support indicators
    USHORT MaxReadBlockLength;      // In units of bytes
    BOOLEAN ReadBlockPartial;       // Partial Block Read allowed ?
    BOOLEAN WriteBlockMisalign;     // Misaligned Block Writes allowed ?
    BOOLEAN ReadBlockMisalign;      // Misaligned Block Reads allowed ?
    BOOLEAN DSRImplemented;         // DSR Register implemented ?
    ULONG DeviceSize;               // Calculated device memory size in bytes
    USHORT VDDReadCurrentMin;       // in units of milliamps
    USHORT VDDReadCurrentMax;       // in units of milliamps
    USHORT VDDWriteCurrentMin;      // in units of milliamps
    USHORT VDDWriteCurrentMax;      // in units of milliamps
    BOOLEAN EraseBlockEnable;       // Erase in multiples of MaxWriteBlockLength ?
    UCHAR EraseSectorSize;          // Size of an erase sector in blocks (SECTOR_SIZE+1)
    UCHAR WPGroupSize;              // Size of a WP group in erase sectors (WP_GRP_SIZE+1)
    UCHAR WPGroupEnable;            // Group Write Protection possible ?
    UCHAR WriteSpeedFactor;         // Block Write time as multiple of Read access time
    USHORT MaxWriteBlockLength;     // In units of bytes
    BOOLEAN WriteBlockPartial;      // Partial block writes allowed ?
    BOOLEAN CopyFlag;               // Copy control flag
    BOOLEAN PermanentWP;            // Permanent Write Protect flag
    BOOLEAN TemporaryWP;            // Temporary Write Protect flag
    SD_FS_TYPE FileSystem;          // File System type
    UCHAR RawCSDRegister[16];       // raw data
} SD_PARSED_REGISTER_CSD, *PSD_PARSED_REGISTER_CSD;

#ifdef _MMC_SPEC_42_  
typedef struct _MMC_PARSED_REGISTER_EXTCSD {    // structure of Extended CSD register 
  UCHAR Reserved180[181]; 
  UCHAR ErasedMemCont;    // initial value after erase operation 
  UCHAR Reserved182; 
  UCHAR BusWidth;    // bus width 
  UCHAR Reserved184; 
  UCHAR HSTiming;    // high speed timing 
  UCHAR Reserved186; 
  UCHAR PowClass;    // power class  
  UCHAR Reserved188; 
  UCHAR CmdSetRev;    // command set revision 
  UCHAR Reserved190; 
  UCHAR CmdSet;    // contains a binary code of the command set 
  UCHAR EXTCSDRev;    // Extended CSD revision 
  UCHAR Reserved193; 
  UCHAR CSDStruct;    // CSD structure field in CSD register 
  UCHAR Reserved195; 
  UCHAR CardType;    // MMC card type 
  UCHAR Reserved197[3]; 
  UCHAR PwrCl52195; 
  UCHAR PwrCl26195; 
  UCHAR PwrCl52360; 
  UCHAR PwrCl26360;    // supported power class by the card 
  UCHAR Reserved204; 
  UCHAR MinPerfR0426;    // min. read performance with 4 bit bus width & 26MHz 
  UCHAR MinPerfW0426;    // min. write performance with 4 bit bus width & 26MHz 
  UCHAR MinPerfR08260452;  // min. read performance with 8 bit bus width & 26MHz 
  // min. read performance with 4 bit bus width & 52MHz 
  UCHAR MinPerfW08260452;  // min. write performance with 8 bit bus width & 26MHz 
  // min. write performance with 4 bit bus width & 26MHz 
  UCHAR MinPerfR0852;    // min. read performance with 8 bit bus width & 52MHz 
  UCHAR MinPerfW0852;    // min. write performance with 8 bit bus width & 52MHz 
  UCHAR Reserved211; 
  ULONG Sec_Count;    // sector count 
  UCHAR Reserved216[288]; 
  UCHAR sCmdSet;    // command sets are supported by the card 
  UCHAR Reserved505[7]; 
} MMC_PARSED_REGISTER_EXTCSD, *PMMC_PARSED_REGISTER_EXTCSD; 
#endif   

#include <pshpack1.h>
typedef struct _SD_REGISTER_SCR {
    DWORD ReservedForManufacturer:32;
    WORD  Reserverd:16;
    BYTE  SD_BUS_WIDTHS:4;
    BYTE  SD_SECURITY:3;
    BYTE  DATA_STAT_AFTER_ERASE:1;
    BYTE  SD_SPEC:4;
    BYTE  SCR_STRUCTURE:4;
} SD_REGISTER_SCR, *PSD_REGISTER_SCR;
#include <poppack.h>


typedef USHORT SD_CARD_RCA;         // card RCA
typedef DWORD  SD_CARD_STATUS;      // card status

typedef struct _SLOT_POWER_DATA {
    UCHAR       Functions;                  // Number of functions for the card
    BOOL        fCardSupportsPowerControl;  // True if card supports power control
    BOOL        fPowerControlEnabled;       // True if power control is enabled
    DWORD       CurrentDrawOfSlot;          // Maximum possible current draw in mA 
                                            // allocated at the slot

    DWORD       OperatingVoltage;           // bit mask similar to OCR register
    DWORD         CurrentTupleVoltageRange;  // bit mask similar to OCR register

    UCHAR         NumberOfHighPowerTuples;    // For SDIO 1.1 this is 1
    DWORD        SizeOfFunctionRecord;       // Size of data per function
} SLOT_POWER_DATA, *PSLOT_POWER_DATA;


typedef struct _SD_FUNCTION_POWER_DRAW {
        //for SDIO 1.0 cards
    USHORT        OpMinPower;     // Minimum current when this function is operating
    USHORT        OpAvePower;        // Average current when this function is operating
    USHORT        OpMaxPower;        // Maximum current when this function is operating
                                                  
        //for SDIO 1.1 cards    Note: EMPC reffers to the EMPC bit of the CCCR register "Power Control"   
        //                            EPS  reffers to the EPS  bit of the FBR  register for Power Selection 
    USHORT        SpAvePower33;    //Ave current if EMPC is not set. (Same as OpAvePower)
    USHORT        SpMaxPower33;   //Max current if EMPC is not set. (Same as OpAvePower)

    USHORT        HpAvePower33;   //Ave current if EMPC is set, and ESP is clear. (High Power)
    USHORT        HpMaxPower33;   //Max current if EMPC is set, and ESP is clear. (High Power)

    USHORT        LpAvePower33;   //Ave current if EMPC is set, and ESP is set. (Low Power)
    USHORT        LpMaxPower33;   //Max current if EMPC is set, and ESP is set. (Low Power)

        //future tuples will require six USHORTs each.
} SD_FUNCTION_POWER_DRAW, *PSD_FUNCTION_POWER_DRAW;

typedef struct _FUNCTION_POWER_STATE {
    BOOL    fPowerControlSupport;   // Does card support Power Control
    BOOL    fPowerControlEnabled;   // Is cards power control enabled
    BOOL    fFunctionEnabled;       // Enable state of the Function
    BOOL    fSupportsPowerSelect;   // Is power selection is supported for this function
    BOOL    fLowPower;              // Function is in the Low or High power state

    DWORD   OperatingVoltage;       // Operating voltage given as an OCR register value
    USHORT  CurrentDrawNow;         // current draw of the function now
    INT     EnableDelta;            // change in current if enabled state changed
    INT     SelectDelta;            // change in current if High/Low power select state changed
} FUNCTION_POWER_STATE, *PFUNCTION_POWER_STATE;


typedef struct _FUNCTION_POWER_DATA {
    TCHAR                   ClientName[MAX_SDCARD_CLIENT_NAME]; // client name, if the functions registered witha client
    FUNCTION_POWER_STATE    PowerState;
    SD_FUNCTION_POWER_DRAW  PowerTuples;
} FUNCTION_POWER_DATA, *PFUNCTION_POWER_DATA;


typedef struct _BUS_DRIVER_IN_SLOT_FUNCTION_POWER {
    DWORD        SlotIndex;      // Slots start at 0
    DWORD        FunctionNumber; // Functions start at 1
} BUS_DRIVER_IN_SLOT_FUNCTION_POWER, *PBUS_DRIVER_IN_SLOT_FUNCTION_POWER;

// DDK api status codes
#define SD_API_STATUS_SUCCESS                          ((SD_API_STATUS)0x00000000L)
#define SD_API_STATUS_PENDING                          ((SD_API_STATUS)0x00000001L)
#define SD_API_STATUS_FAST_PATH_SUCCESS                ((SD_API_STATUS)0x00000002L)
#define SD_API_STATUS_FAST_PATH_OPT_SUCCESS            ((SD_API_STATUS)0x00000003L)
#define SD_API_STATUS_BUFFER_OVERFLOW                  ((SD_API_STATUS)0xC0000001L)
#define SD_API_STATUS_DEVICE_BUSY                      ((SD_API_STATUS)0xC0000002L)
#define SD_API_STATUS_UNSUCCESSFUL                     ((SD_API_STATUS)0xC0000003L)
#define SD_API_STATUS_NOT_IMPLEMENTED                  ((SD_API_STATUS)0xC0000004L)
#define SD_API_STATUS_ACCESS_VIOLATION                 ((SD_API_STATUS)0xC0000005L)
#define SD_API_STATUS_INVALID_HANDLE                   ((SD_API_STATUS)0xC0000006L)
#define SD_API_STATUS_INVALID_PARAMETER                ((SD_API_STATUS)0xC0000007L)
#define SD_API_STATUS_NO_SUCH_DEVICE                   ((SD_API_STATUS)0xC0000008L)
#define SD_API_STATUS_INVALID_DEVICE_REQUEST           ((SD_API_STATUS)0xC0000009L)
#define SD_API_STATUS_NO_MEMORY                        ((SD_API_STATUS)0xC000000AL)
#define SD_API_STATUS_BUS_DRIVER_NOT_READY             ((SD_API_STATUS)0xC000000BL)
#define SD_API_STATUS_DATA_ERROR                       ((SD_API_STATUS)0xC000000CL)
#define SD_API_STATUS_CRC_ERROR                        ((SD_API_STATUS)0xC000000DL)
#define SD_API_STATUS_INSUFFICIENT_RESOURCES           ((SD_API_STATUS)0xC000000EL)
#define SD_API_STATUS_DEVICE_NOT_CONNECTED             ((SD_API_STATUS)0xC0000010L)
#define SD_API_STATUS_DEVICE_REMOVED                   ((SD_API_STATUS)0xC0000011L)
#define SD_API_STATUS_DEVICE_NOT_RESPONDING            ((SD_API_STATUS)0xC0000012L)
#define SD_API_STATUS_CANCELED                         ((SD_API_STATUS)0xC0000013L)
#define SD_API_STATUS_RESPONSE_TIMEOUT                 ((SD_API_STATUS)0xC0000014L)
#define SD_API_STATUS_DATA_TIMEOUT                     ((SD_API_STATUS)0xC0000015L)
#define SD_API_STATUS_DEVICE_RESPONSE_ERROR            ((SD_API_STATUS)0xC0000016L)
#define SD_API_STATUS_DEVICE_UNSUPPORTED               ((SD_API_STATUS)0xC0000017L)
#define SD_API_STATUS_SHUT_DOWN                        ((SD_API_STATUS)0xC0000018L)

#ifdef _MMC_SPEC_42_    // error status of bus width change 
#define MMC_API_STATUS_NOT_CHANGE_BUSWIDTH             ((SD_API_STATUS)0xC0000100L) 
#endif
#define SD_API_STATUS_INSUFFICIENT_HOST_POWER          ((SD_API_STATUS)0xC0000019L)


// macro to test for success
#define SD_API_SUCCESS(Status) ((SD_API_STATUS)(Status) >= 0)

// macro to test for success where a SD_API_STATUS_TIMEOUT is okay
#define SD_API_SUCCESS_RESPONSE_TIMEOUT_OK(Status) (((SD_API_STATUS)(Status) >= 0) || (SD_API_STATUS_RESPONSE_TIMEOUT == (SD_API_STATUS)(Status)))

// macro to test explicitly for time out 
#define SD_API_RESPONSE_TIMEOUT(Status) (SD_API_STATUS_RESPONSE_TIMEOUT == (SD_API_STATUS)(Status))

// debug zone slot IDs - user assignable slots
#define SDCARD_ZONESLOT_0             0
#define SDCARD_ZONESLOT_1             1
#define SDCARD_ZONESLOT_2             2
#define SDCARD_ZONESLOT_3             3
#define SDCARD_ZONESLOT_4             4
#define SDCARD_ZONESLOT_5             5
#define SDCARD_ZONESLOT_6             6
#define SDCARD_ZONESLOT_7             7
#define SDCARD_ZONESLOT_8             8
#define SDCARD_ZONESLOT_9             9
#define SDCARD_ZONESLOT_10            10
// reserved slots
#define SDCARD_ZONESLOT_FUNC          11
#define SDCARD_ZONESLOT_INFO          12
#define SDCARD_ZONESLOT_INIT          13
#define SDCARD_ZONESLOT_WARN          14
#define SDCARD_ZONESLOT_ERROR         15

// zone enable bit definitions
#define ZONE_ENABLE_0               (1 << SDCARD_ZONESLOT_0)
#define ZONE_ENABLE_1               (1 << SDCARD_ZONESLOT_1)
#define ZONE_ENABLE_2               (1 << SDCARD_ZONESLOT_2)
#define ZONE_ENABLE_3               (1 << SDCARD_ZONESLOT_3)
#define ZONE_ENABLE_4               (1 << SDCARD_ZONESLOT_4)
#define ZONE_ENABLE_5               (1 << SDCARD_ZONESLOT_5)
#define ZONE_ENABLE_6               (1 << SDCARD_ZONESLOT_6)
#define ZONE_ENABLE_7               (1 << SDCARD_ZONESLOT_7)
#define ZONE_ENABLE_8               (1 << SDCARD_ZONESLOT_8)
#define ZONE_ENABLE_9               (1 << SDCARD_ZONESLOT_9)
#define ZONE_ENABLE_10              (1 << SDCARD_ZONESLOT_10)
#define ZONE_ENABLE_FUNC            (1 << SDCARD_ZONESLOT_FUNC)
#define ZONE_ENABLE_INFO            (1 << SDCARD_ZONESLOT_INFO)
#define ZONE_ENABLE_INIT            (1 << SDCARD_ZONESLOT_INIT)
#define ZONE_ENABLE_WARN            (1 << SDCARD_ZONESLOT_WARN)
#define ZONE_ENABLE_ERROR           (1 << SDCARD_ZONESLOT_ERROR)

// debug zones masks macros
#define SDCARD_ZONE_0          DEBUGZONE(SDCARD_ZONESLOT_0)
#define SDCARD_ZONE_1          DEBUGZONE(SDCARD_ZONESLOT_1)
#define SDCARD_ZONE_2          DEBUGZONE(SDCARD_ZONESLOT_2)
#define SDCARD_ZONE_3          DEBUGZONE(SDCARD_ZONESLOT_3)
#define SDCARD_ZONE_4          DEBUGZONE(SDCARD_ZONESLOT_4)
#define SDCARD_ZONE_5          DEBUGZONE(SDCARD_ZONESLOT_5)
#define SDCARD_ZONE_6          DEBUGZONE(SDCARD_ZONESLOT_6)
#define SDCARD_ZONE_7          DEBUGZONE(SDCARD_ZONESLOT_7)
#define SDCARD_ZONE_8          DEBUGZONE(SDCARD_ZONESLOT_8)
#define SDCARD_ZONE_9          DEBUGZONE(SDCARD_ZONESLOT_9)
#define SDCARD_ZONE_10         DEBUGZONE(SDCARD_ZONESLOT_10)
#define SDCARD_ZONE_FUNC       DEBUGZONE(SDCARD_ZONESLOT_FUNC)
#define SDCARD_ZONE_INFO       DEBUGZONE(SDCARD_ZONESLOT_INFO)
#define SDCARD_ZONE_INIT       DEBUGZONE(SDCARD_ZONESLOT_INIT)
#define SDCARD_ZONE_WARN       DEBUGZONE(SDCARD_ZONESLOT_WARN)
#define SDCARD_ZONE_ERROR      DEBUGZONE(SDCARD_ZONESLOT_ERROR)


#define SDCARD_WAKE_ON_SDIO_INTERRUPTS_VALNAME  _T("WakeOnSDIOInterrupts")
#define SDCARD_WAKE_ON_SDIO_INTERRUPTS_VALTYPE  REG_DWORD


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

    //  SDRegisterClient()
    typedef SD_API_STATUS (*PSD_REGISTER_CLIENT) (SD_DEVICE_HANDLE, 
        PVOID, 
        PSDCARD_CLIENT_REGISTRATION_INFO);

    //  SDSynchronousBusRequest 
    typedef SD_API_STATUS (*PSD_SYNCHRONOUS_BUS_REQUEST) (SD_DEVICE_HANDLE, 
        UCHAR,
        DWORD,
        SD_TRANSFER_CLASS,
        SD_RESPONSE_TYPE,
        PSD_COMMAND_RESPONSE ,
        ULONG,
        ULONG,
        PUCHAR,
        DWORD,
        DWORD,
        PPHYS_BUFF_LIST);        

    //  SDBusRequest 
    typedef SD_API_STATUS (*PSDBUS_REQUEST) (SD_DEVICE_HANDLE ,
        UCHAR,
        DWORD,
        SD_TRANSFER_CLASS ,
        SD_RESPONSE_TYPE ,
        ULONG ,
        ULONG ,
        PUCHAR ,
        PSD_BUS_REQUEST_CALLBACK,
        DWORD,
        PSD_BUS_REQUEST*,
        DWORD,
        DWORD,
        PPHYS_BUFF_LIST);      

    //  SDCancelBusRequest 
    typedef BOOLEAN (*PSD_CANCEL_BUS_REQUEST) (PSD_BUS_REQUEST);


    //  SDFreeBusRequest
    typedef VOID (*PSD_FREE_BUS_REQUEST) (PSD_BUS_REQUEST);


    //  SDCardRegistersQuery 
    typedef SD_API_STATUS (*PSD_CARD_INFO_QUERY) (SD_DEVICE_HANDLE,
        SD_INFO_TYPE,
        PVOID,
        ULONG);


    //  SDReadWriteRegisterDirect
    typedef SD_API_STATUS (*PSD_READ_WRITE_REGISTER_DIRECT)(SD_DEVICE_HANDLE,
        SD_IO_TRANSFER_TYPE,
        UCHAR,
        DWORD,
        BOOLEAN,
        PUCHAR,
        ULONG);
    // SDGetTuple
    typedef SD_API_STATUS (*PSD_GET_TUPLE)(SD_DEVICE_HANDLE,
        UCHAR,
        PUCHAR,
        PULONG,
        BOOL);

    // SDIOConnectInterrupt
    typedef SD_API_STATUS (*PSD_IO_CONNECT_INTERRUPT) (SD_DEVICE_HANDLE, 
        PSD_INTERRUPT_CALLBACK);

    // SDIODisconnectInterrupt
    typedef VOID (*PSD_IO_DISCONNECT_INTERRUPT) (SD_DEVICE_HANDLE);

    //  SDSetCardFeature
    typedef SD_API_STATUS (*PSD_SET_CARD_FEATURE) (SD_DEVICE_HANDLE,
        SD_SET_FEATURE_TYPE,
        PVOID,
        ULONG);

    typedef SD_API_STATUS (*PSD_BUS_REQUEST_RESPONSE)(HANDLE, PSD_COMMAND_RESPONSE );

    // SD Card Api function structure definition
    typedef struct _SDCARD_API_FUNCTIONS {
        DWORD                           dwSize;
        PSD_REGISTER_CLIENT             pSDRegisterClient;
        PSD_SYNCHRONOUS_BUS_REQUEST     pSDSynchronousBusRequest;
        PSDBUS_REQUEST                  pSDBusRequest;
        PSD_FREE_BUS_REQUEST            pSDFreeBusRequest;
        PSD_CARD_INFO_QUERY             pSDCardInfoQuery;
        PSD_READ_WRITE_REGISTER_DIRECT  pSDReadWriteRegistersDirect;
        PSD_CANCEL_BUS_REQUEST          pSDCancelBusRequest;
        PSD_GET_TUPLE                   pSDGetTuple;
        PSD_IO_CONNECT_INTERRUPT        pSDIOConnectInterrupt;
        PSD_IO_DISCONNECT_INTERRUPT     pSDIODisconnectInterrupt;
        PSD_SET_CARD_FEATURE            pSDSetCardFeature;
        PSD_BUS_REQUEST_RESPONSE        pSdBusRequestResponse;        
    }SDCARD_API_FUNCTIONS, *PSDCARD_API_FUNCTIONS;

    // global variable containing function pointers to SD Card api
    extern SDCARD_API_FUNCTIONS g_SDClientApiFunctions;

    // debug output function
    VOID SDCardDebugOutput(TCHAR *pDebugText, ...);

#ifdef DEBUG

    //  SDRegisterDebugZones - register a client device
    //  Input:  hDevice - device handle
    //          pRegPath - pointer to registry path
    //          
    //  Output:
    //  Return: 
    //  Notes:  
    //
    VOID SDRegisterDebugZones(SD_DEVICE_HANDLE hDevice, PTSTR);

#else

#define SDRegisterDebugZones(hDevice, reserved)

#endif // DEBUG


    // Sending debug output to a file is no longer supported.
#define SDInitializeOutputToFile(pFileName, MaxFileLength)
#define SDDeinitializeOutputToFile()
#define SDSwitchDebugOutput(OutputToFile)
#define SD_CHECK_OUTPUT_TO_FILE(pPath)


    //  SDInitializeQueue - initialize a driver allocated request queue
    //  Input:  pQueue - queue to initialize        
    //  Output: 
    //  Return: 
    //  Notes:
    //
    VOID SDInitializeQueue(PSD_REQUEST_QUEUE pQueue);

    //  SDQueueBusRequest - queue a request (FIFO order)
    //  Input:  pQueue - queue to use
    //          pRequest - request to add to queue        
    //  Output: 
    //  Return: 
    //  Notes:
    //
    VOID SDQueueBusRequest(PSD_REQUEST_QUEUE pQueue, PSD_BUS_REQUEST pRequest); 

    //  SDDequeueBusRequest - de-queue a request (FIFO order)
    //  Input:  pQueue - the queue        
    //  Output: 
    //  Return: the first request queued , otherwise NULL if request queue is empty 
    //  Notes:
    //
    PSD_BUS_REQUEST SDDequeueBusRequest(PSD_REQUEST_QUEUE pQueue);


    //  SDGetCurrentRequest - Get the current request at the front of the queue
    //                        but do not remove it from the queue
    //  Input:  pQueue - the queue        
    //  Output: 
    //  Return: the current request that is at the front of the queue (FIFO orer)
    //          NULL if the queue is empty
    //  Notes:
    //
    PSD_BUS_REQUEST SDGetCurrentRequest(PSD_REQUEST_QUEUE pQueue);


    //  SDRemoveEntryFromQueue - remove an entry from the queue
    //  Input:  pQueue - queue to use
    //          pRequest - request to be removed       
    //  Output: 
    //  Return: 
    //  Notes:
    //
    VOID SDRemoveEntryFromQueue(PSD_REQUEST_QUEUE pQueue, PSD_BUS_REQUEST pRequest); 

    //  GetBitSlice - Get a bit slice from a byte buffer
    //  Input:  pBuffer - buffer containing data 
    //          BufferSize - size of buffer in bytes
    //          BitOffset - bit offset from start of buffer
    //          NumberOfBits - number of bits (less than or equal to 32)
    //  Output: 
    //
    //  Return: returns a DWORD contain the bit slice shifted to fill the least significant bits
    //  Notes:  
    //
    DWORD GetBitSlice(PUCHAR pBuffer, ULONG BufferSize, DWORD BitOffset, UCHAR NumberOfBits);

    //  SDGetDeviceHandle - Get the device handle from the context passed in
    //                      XXX_Init
    //  Input:  InitContext - Context passed in XXX_Init
    //          
    //  Output: ppRegPath - registry path of device (optional)
    //  Return: SD_DEVICE_HANDLE
    //  Notes:  
    //      returns SD Client Device Handle or NULL on failure
    //      Caller must free ppRegPath using SDFreeMemory()
    //
    SD_DEVICE_HANDLE SDGetDeviceHandle(DWORD InitContext, PWCHAR *ppRegPath);

    //  SDGetCardStatusFromResponse - get the card status from a response
    //  Input:  pCardResponse - the card response
    //          
    //  Output:
    //  Return: 
    //  Notes:  
    //     the response must be of type R1 or R1b
    //

#define SDGetCardStatusFromResponse(pCardResponse, pCardStatus) \
    memcpy((pCardStatus), &(pCardResponse)->ResponseBuffer[1], sizeof(SD_CARD_STATUS))

    //  SDRegisterClient - register a client device
    //  Input:  hDevice - device handle
    //          pDeviceContext  - device specific context allocated by driver
    //          pInfo  - registration information
    //          
    //  Output: 
    //  Return: SD API Status
    //  Notes:
    //     
    //      
    SD_API_STATUS SDRegisterClient(SD_DEVICE_HANDLE                 hDevice, 
        PVOID                            pDeviceContext, 
        PSDCARD_CLIENT_REGISTRATION_INFO pInfo);


    //  SDGetRegPathFromInitContext - Get the real device registry path from the context 
    //                                passed in XXX_Init
    //  Input:  pActivePath - the active path
    //          Length      - length in bytes of pRegPath
    //  Output: pRegPath - caller supplied storage for the registry path
    //
    //  Return: SD API Status
    //  Notes:  
    //      For streams drivers the InitContext passed in the XXX_Init entry point
    //      is the active registry path and not the real path of the device.
    //      This function can be used to retrieve the real path.
    //
    //      returns WIN32 Error
    //
    DWORD SDGetRegPathFromInitContext(PWCHAR pActivePath, PWCHAR pRegPath, ULONG Length);


    //  SDGetClientFunctions - get the client function table
    //  Input:  pClientFunctions - function table
    //  Return: SD API Status
    //
    SD_API_STATUS SDGetClientFunctions(PSDCARD_API_FUNCTIONS pClientFunctions);


    //  SDSynchronousBusRequest - send an SD Bus request synchronously
    //  Input:  hDevice -  device handle
    //          Command - command to send
    //          Argument - argument for command
    //          TransferClass - Command only, or associated with read/write data
    //          ResponseType - expected response
    //          NumBlocks   - number of blocks
    //          BlockSize   - block size
    //          pBuffer     - block buffer
    //          Flags       - request flags
    //          
    //  Output: pResponse - caller allocated storage for the return response 
    //  Return: SD API Status    
    //  Notes:  
    //        This function provides a synchronous (blocking) call to perform a 
    //        bus request.  This function should not be called in a bus request callback
    //        which can be called in a dispatch context.  Calling this function
    //        within a Bus Request callback will result in a deadlock.
    //
#define SDSynchronousBusRequest(hDevice, \
    Command, \
    Argument, \
    TransferClass, \
    ResponseType, \
    pResponse, \
    NumBlocks, \
    BlockSize, \
    pBuffer,   \
    Flags)     \
    g_SDClientApiFunctions.pSDSynchronousBusRequest((hDevice),(Command),(Argument),(TransferClass), \
    (ResponseType),(pResponse), \
    /*(NumBlocks), (BlockSize), (pBuffer), (Flags))*/ \
    (NumBlocks), (BlockSize), (pBuffer), ((Flags)&~SD_BUS_REQUEST_PHYS_BUFFER),0,NULL)

    //  SDBusRequest - send command over SD bus
    //  Input:  hDevice       - SD device handle
    //          Command       - SD command to send over bus
    //          Argument      - 32 bit argument specific to the command
    //          TransferClass - Command only, or associated with read/write data
    //          ResponseType  - the response type for the command
    //          NumBlocks     - Number of data blocks in pBlockArray, can be zero
    //                          if transfer class is not read or write
    //          BlockSize     - Size of data blocks in pBlockArray. All blocks
    //                          must be same size.
    //          pBuffer       - Pointer to buffer containing BlockSize*NumBlocks bytes
    //          pCallback     - completion callback
    //          RequestParam  - optional driver specific parameter for this request
    //          Flags         - bus request flags
    //  Output: pRequest      - newly allocated request
    //  Return: SD API Status
    //  Notes:  
    //          Must call free request in the completion callback
    //
#define  SDBusRequest(hDevice, \
    Command, \
    Argument, \
    TransferClass, \
    ResponseType, \
    NumBlocks, \
    BlockSize, \
    pBuffer,   \
    pCallback, \
    RequestParam,\
    pRequest,    \
    Flags)       \
    g_SDClientApiFunctions.pSDBusRequest((hDevice),(Command),(Argument),(TransferClass), \
    (ResponseType),(NumBlocks),(BlockSize), \
    (pBuffer), (pCallback), (RequestParam), (pRequest), (Flags))


    //  SDSynchronousBusRequest - send an SD Bus request synchronously
    //  Input:  hDevice -  device handle
    //          Command - command to send
    //          Argument - argument for command
    //          TransferClass - Command only, or associated with read/write data
    //          ResponseType - expected response
    //          NumBlocks   - number of blocks
    //          BlockSize   - block size
    //          pBuffer     - block buffer
    //          Flags       - request flags
    //          cbSize        - pPyysBuffList structure size in byte. it only valid when SD_BUS_REQUEST_PHYS_BUFFER is set in Flags
    //          pPhysBuffList - physical buffer list.
    //          
    //  Output: pResponse - caller allocated storage for the return response 
    //  Return: SD API Status    
    //  Notes:  
    //        This function provides a synchronous (blocking) call to perform a 
    //        bus request.  This function should not be called in a bus request callback
    //        which can be called in a dispatch context.  Calling this function
    //        within a Bus Request callback will result in a deadlock.
    //
#define SDSynchronousBusRequestEx(hDevice, \
    Command, \
    Argument, \
    TransferClass, \
    ResponseType, \
    pResponse, \
    NumBlocks, \
    BlockSize, \
    pBuffer,   \
    Flags, \
    cbSize, \
    pPhysBuffList)     \
    g_SDClientApiFunctions.pSDSynchronousBusRequest((hDevice),(Command),(Argument),(TransferClass), \
    (ResponseType),(pResponse), \
    (NumBlocks), (BlockSize), (pBuffer), (Flags),(cbSize),(pPhysBuffList))

    //  SDBusRequestEx - send command over SD bus with physical buffer
    //  Input:  hDevice       - SD device handle
    //          Command       - SD command to send over bus
    //          Argument      - 32 bit argument specific to the command
    //          TransferClass - Command only, or associated with read/write data
    //          ResponseType  - the response type for the command
    //          NumBlocks     - Number of data blocks in pBlockArray, can be zero
    //                          if transfer class is not read or write
    //          BlockSize     - Size of data blocks in pBlockArray. All blocks
    //                          must be same size.
    //          pBuffer       - Pointer to buffer containing BlockSize*NumBlocks bytes
    //          pCallback     - completion callback
    //          RequestParam  - optional driver specific parameter for this request
    //          Flags         - bus request flags
    //          cbSize        - pPyysBuffList structure size in byte. it only valid when SD_BUS_REQUEST_PHYS_BUFFER is set in Flags
    //          pPhysBuffList - physical buffer list.
    //  Output: pRequest      - newly allocated request
    //  Return: SD API Status
    //  Notes:  
    //          Must call free request in the completion callback
    //
#define  SDBusRequestEx(hDevice, \
    Command, \
    Argument, \
    TransferClass, \
    ResponseType, \
    NumBlocks, \
    BlockSize, \
    pBuffer,   \
    pCallback, \
    RequestParam,\
    pRequest,    \
    Flags, \
    cbSize, \
    pPhysBuffList)     \
    g_SDClientApiFunctions.pSDBusRequest((hDevice),(Command),(Argument),(TransferClass), \
    (ResponseType),(NumBlocks),(BlockSize), \
    (pBuffer), (pCallback), (RequestParam), (pRequest), (Flags),(cbSize),(pPhysBuffList))

    //  SDCancelBusRequest - Cancel an outstanding bus request
    //  Input:  
    //          pRequest - request to cancel,(returned from SDBusRequest)
    //  Output: 
    //  Return: TRUE if request was cancelled , FALSE if the request is still pending
    //  Notes:
    //
#define SDCancelBusRequest(pRequest) g_SDClientApiFunctions.pSDCancelBusRequest((pRequest))


    //  SDFreeBusRequest - free a bus request
    //  Input:  pRequest        
    //  Output: 
    //  Return: 
    //  Notes:
    //          this function should be called when the request has completed, 
    //          typically in the completion callback
    //
#define SDFreeBusRequest(pRequest) g_SDClientApiFunctions.pSDFreeBusRequest((pRequest))

    //  SDCardInfoQuery        - Obtain Card information
    //  Input:  hDevice        - SD Device Handle
    //          InfoType       - information to get
    //          StructureSize  - size of info structure
    //  Output: pCardInfo      - Information specific structure 
    //  Return: SD_API_STATUS code
    //  Notes:  pCardInfo must point to sufficient memory for the informtaion type
    //    
#define SDCardInfoQuery(hDevice,         \
    InfoType,        \
    pCardInfo,       \
    StructureSize)   \
    g_SDClientApiFunctions.pSDCardInfoQuery((hDevice), (InfoType), (pCardInfo), \
    (StructureSize))


    //  SDReadWriteRegistersDirect - Read/Write I/O register(s) direct
    //  Input:  hDevice - the device handle
    //          ReadWrite - read write flag 
    //          Function - Function number
    //          Address -  starting address
    //          ReadAfterWrite - flag to instruct card to read after write
    //          pBuffer -   buffer to hold value of registers
    //          Length - number of bytes to read/write
    //      
    //  Output: 
    //  Return: SD_API_STATUS code
    //  Notes:  
    //          This function can be called to read or write multiple contiguous registers synchronously
    //          using the SDIO RW_DIRECT command. This function issues multiple commands to transfer
    //          to or from the user's buffer
    //          If ReadAfterWrite is set to 1, the operation will instruct the card
    //          to return the new value of the register in the response.  
    //
#define SDReadWriteRegistersDirect(hDevice,          \
    ReadWrite,        \
    Function,         \
    Address,          \
    ReadAfterWrite,   \
    pBuffer,          \
    Length)           \
    g_SDClientApiFunctions.pSDReadWriteRegistersDirect((hDevice), (ReadWrite), (Function),     \
    (Address), (ReadAfterWrite), (pBuffer), \
    (Length))


    //  SDGetTuple - Get tuple data from CIS
    //  Input:  hDevice   - SD device handle
    //          TupleCode - Tuple code
    //          pBufferSize - size of buffer to store Tuple Data
    //  Output: pBuffer   - Tuple data is copied here (optional)
    //          pBufferSize - if pBuffer is NULL, this will store the size of the
    //          CommonCIS - flag indicating common or function CIS
    //  Return: SD_API_STATUS code
    //          
    //  Notes: The caller should initially call this function with a NULL buffer
    //         to determine the size of the tuple.  The variable pBufferSize points
    //         to the caller supplied storage for this result.   If no bus errors occurs
    //         the function returns SD_API_STATUS_SUCCESS.   The caller must check 
    //         the value of the buffer size returned.  If the value is non-zero, the
    //         tuple exists and can be fetched by calling this function again with a
    //         non-zero buffer.
    //
#define  SDGetTuple(hDevice,    \
    TupleCode,  \
    pBuffer,    \
    pBufferSize,\
    CommonCIS)  \
    g_SDClientApiFunctions.pSDGetTuple((hDevice), (TupleCode), (pBuffer), (pBufferSize),(CommonCIS))

    //  SDIOConnectInterrupt  - Associate an interrupt service routine for an SDIO
    //                          peripheral interrupt
    //  Input:  hDevice   - SD device handle
    //          pIsrFunction - the Interrupt service routine
    //  Output: 
    //  Return: SD_API_STATUS code
    //          
    //  Notes: This function is provided for an SDIO peripheral driver to 
    //         register an interrupt routine for the device. 
    //         The interrupt function has the form of PSD_INTERRUPT_CALLBACK.
    //         The caller should call SDIODisconnectInterrupt when cleaning up
    //         the device. The bus driver will enable the interrupt for the function in the
    //         card's CCCR area prior to returning from this function. 
    //         The interrupt callback is called whenever the device function is 
    //         interrupting. The bus driver will determine the interrupting function,
    //         disable the interrupt on the card and call the callback.  Upon return 
    //         from the callback the bus driver will reenable the interrupt in the
    //         card's CCCR.
    //
#define SDIOConnectInterrupt(hDevice,      \
    pIsrFunction) \
    g_SDClientApiFunctions.pSDIOConnectInterrupt((hDevice), (pIsrFunction))

    //  SDIODisconnectInterrupt  - disconnect the interrupt 
    //  Input:  hDevice   - SD device handle
    //  Output: 
    //  Return: 
    //          
    //  Notes: This function should be called to disconnect the interrupt
    //         from the device. The bus driver will disable the interrupt in the
    //         card's CCCR area
    //
#define SDIODisconnectInterrupt(hDevice) \
    g_SDClientApiFunctions.pSDIODisconnectInterrupt((hDevice))

    //  SDSetCardFeature       - Set card feature
    //  Input:  hDevice        - SD Device Handle
    //          CardFeature    - Card Feature to set
    //          StructureSize  - size of card feature structure
    //  Output: pCardInfo      - Information for the feature
    //  Return: SD_API_STATUS code
    //  Notes:  This function is provided to set various card features
    //          in a thread safe manner.  SDIO cards utilize shared register sets
    //          between functions. This requires that the 
    //          register state be preserved between functions that can be 
    //          controlled in separate thread contexts.
    //          This function can potentially block by issuing synchronous bus 
    //          request.  This function must not be called from a bus request callback
    //
#define SDSetCardFeature(hDevice,         \
    CardFeature,     \
    pCardInfo,       \
    StructureSize)   \
    g_SDClientApiFunctions.pSDSetCardFeature((hDevice), (CardFeature), (pCardInfo), \
    (StructureSize))

    //  SDFreeBusRequest - free a bus request
    //  Input:  pRequest        
    //  Output: 
    //  Return: 
    //  Notes:
    //          this function should be called when the request has completed, 
    //          typically in the completion callback
    //
#define SdBusRequestResponse(pRequest,pSdCmdResp) g_SDClientApiFunctions.pSdBusRequestResponse(pRequest,pSdCmdResp)

    //  SDPerformSafeCopy - perform a safe memory copy
    //  Input:  pSource   - source data
    //          Length    - number of bytes to copy
    //  Output: pDestination - Destination of the copy
    //  Return:  returns TRUE if the copy succeeded, FALSE if an exception  occured
    //  Notes:  
    BOOLEAN SDPerformSafeCopy(
        __out_bcount(Length)    PVOID pDestination, 
        __in_bcount(Length)     const VOID *pSource,
                                ULONG Length);

    //  SDProcessException - main SD exception handler
    //  Input:  
    //          pException - exception record passed in from the kernel
    //  Output: 
    //  Return:
    //  Notes:  
    DWORD SDProcessException(LPEXCEPTION_POINTERS pException);

    // macro to set the process permissions to the specified access key
    // caller must call RESTORE_PROC_PERMISSIONS in braces

//  SDOutputBuffer - dump buffer to debugger
//  Input:  
//          pBuffer - the buffer to dump
//          BufferSize - size of buffer in bytes
//  Output: 
//  Notes:  
//          This function prints the buffer to the debugger using
//          16 bytes per line and displays the ASCII character respresentation 
//          per line, if the character is not displayable, a '.' is used
//
VOID SDOutputBuffer(PVOID pBuffer, ULONG BufferSize);


#ifdef __cplusplus
}
#endif //__cplusplus


#ifdef DEBUG

// register zones macro
// Input: hInstance - Dll hInstance 
//        pPath   - registry path for ZoneMask override value
// get initial zone override value from register if one exists
// use this value to overide the initial zone value
#define SD_DEBUG_ZONE_REGISTER(hInstance, reserved)    DEBUGREGISTER(hInstance)

// this macro instantiates the global debug variable
#define SD_DEBUG_INSTANTIATE_ZONES(ModuleName, InitialValue, Slot0, Slot1, Slot2, Slot3, Slot4, Slot5, Slot6, Slot7, Slot8, Slot9, Slot10) \
    DBGPARAM dpCurSettings =        \
{                               \
    ModuleName,                 \
{                           \
    Slot0,                  \
    Slot1,                  \
    Slot2,                  \
    Slot3,                  \
    Slot4,                  \
    Slot5,                  \
    Slot6,                  \
    Slot7,                  \
    Slot8,                  \
    Slot9,                  \
    Slot10,                 \
    TEXT("Functions"),      \
    TEXT("Information"),    \
    TEXT("Init"),           \
    TEXT("Warnings"),       \
    TEXT("Errors"),         \
},                          \
    (InitialValue)              \
};


#define DbgPrintZo(m, s) \
    ((void)((m)?(SDCardDebugOutput s),1:0))

#else   // no DEBUG
#define SD_DEBUG_ZONE_REGISTER(hInstance, reserved) 
#define SD_DEBUG_INSTANTIATE_ZONES(ModuleName, InitialValue, Slot0, Slot1, Slot2, Slot3, Slot4, Slot5, Slot6, Slot7, Slot8, Slot9, Slot10)
#define DbgPrintZo(m, s)
#endif  // DEBUG

#ifndef SHIP_BUILD
#define DbgPrintRetail(s)   SDCardDebugOutput s
#else
#define DbgPrintRetail(s)
#endif


#if DEBUG

// will break into the debugger if the expression is false
// Input:  an expression to test
// Output: 
#define DEBUG_ASSERT(t) DEBUGCHK(t)

#define DEBUG_CHECK(t,s)\
    if (!(t)) {\
    DbgPrintZo(-1,s); \
    DebugBreak();\
    }

#else   // DEBUG

#define DEBUG_ASSERT(t)
#define DEBUG_CHECK(t,s)

#endif  // DEBUG


#define MAX_SD_DESCRIPTION_STRING (MAX_SDCARD_CLIENT_NAME*2)

// bus driver name
#define SDCARD_BUS_DRIVER_DEVICE_NAME TEXT("SDC1:")

// bus driver ioctls
// Note: 0 and 1 are reserved.

// Output: DWORD - number of slots in the system
//
#define IOCTL_SD_BUS_DRIVER_GET_SLOT_COUNT\
    CTL_CODE(0, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Output: BUS_DRIVER_SLOT_INFO array
#define IOCTL_SD_BUS_DRIVER_GET_SLOT_INFO\
    CTL_CODE(0, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Output: DWORD, Upper 16 bits = Version Major, Lower 16 bits = Version Minor
#define IOCTL_SD_BUS_DRIVER_GET_VERSION\
    CTL_CODE(0, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

    // Reset SD/MMC/SDIO/Combo/Multifunction card
#define IOCTL_SD_BUS_DRIVER_SLOT_CARD_RESET\
            CTL_CODE(0, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

    // Select SD/MMC/SDIO/Combo/Multifunction card 
#define IOCTL_SD_BUS_DRIVER_SLOT_CARD_SELECT\
            CTL_CODE(0, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

    // Deselect SD/MMC/SDIO/Combo/Multifunction card
#define IOCTL_SD_BUS_DRIVER_SLOT_CARD_DESELECT\
            CTL_CODE(0, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

    // Output: Power Control Slot or Function Data  
#define IOCTL_SD_BUS_DRIVER_GET_SLOT_POWER_CONTROL\
            CTL_CODE(0, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

   // Disable power control for slot on next insertion (if supported)  
#define IOCTL_SD_BUS_DRIVER_DISABLE_SLOT_POWER_CONTROL\
            CTL_CODE(0, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

   // Enable power control for slot on next insertion (if supported)  
#define IOCTL_SD_BUS_DRIVER_ENABLE_SLOT_POWER_CONTROL\
            CTL_CODE(0, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)

// slot info structure (one per slot)
typedef struct _BUS_DRIVER_SLOT_INFO {
    DWORD              HostIndex;       // host controller index
    DWORD              SlotIndex;       // slot index in host controller
    BOOL               CardPresent;     // card is present
    SDCARD_DEVICE_TYPE DeviceType;
    WCHAR              Description[MAX_SD_DESCRIPTION_STRING]; // device description string
    SD_CARD_INTERFACE  CardInterface;   // card interface
}BUS_DRIVER_SLOT_INFO, *PBUS_DRIVER_SLOT_INFO;


#ifndef dim
#define dim(x) (sizeof(x)/sizeof(x[0]))
#endif


#endif

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

