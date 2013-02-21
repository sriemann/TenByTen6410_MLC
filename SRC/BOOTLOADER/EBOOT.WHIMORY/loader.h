//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef _LOADER_H_
#define _LOADER_H_

#include <blcommon.h>
#include <bootpart.h>

#include <WMRConfig.h>
#include <WMRTypes.h>

// Bootloader version.
//
#define EBOOT_VERSION_MAJOR     3
#define EBOOT_VERSION_MINOR     6


//------------------------------------------------------------------------------
//
//  Section Name:   Memory Configuration
//  Description.:   The constants defining memory layout below must match
//                  those defined in the .bib files.
//
//------------------------------------------------------------------------------

#define CACHED_TO_UNCACHED_OFFSET   	0x20000000

#define STEPLDR_RAM_IMAGE_BASE      (IMAGE_STEPLOADER_PA_START)
#define STEPLDR_RAM_IMAGE_SIZE      (0x00040000)// Stepping Stone size is just 8Kbytes
                                                // But Stepldr is fused in Block 0 that can hold 32KBytes in Small Block NAND
                                                // and 128KBytes in Large Block NAND. So, we can define this value over original
                                                // Stepping stone size.

#define STEPLDR_BIN_HEAD_CUT_OFFSET (0x00001000) // Cut 4Kbyte that has some jump code and ZI data, So 

#define EBOOT_RAM_IMAGE_BASE        (IMAGE_EBOOT_CA_START)
#define EBOOT_RAM_IMAGE_SIZE        (IMAGE_EBOOT_SIZE)

#define EBOOT_STORE_OFFSET          (0)
#define EBOOT_STORE_ADDRESS         (EBOOT_RAM_IMAGE_BASE + EBOOT_STORE_OFFSET)
#define EBOOT_STORE_MAX_LENGTH      (EBOOT_RAM_IMAGE_SIZE)
#define LOGO_STORE_ADDRESS            (EBOOT_FRAMEBUFFER_UA_START)            //logo.bin 
#define LOGO_STORE_MAX_LENGTH         (EBOOT_FRAMEBUFFER_SIZE) 

// BinFS work area defined in boot.bib
#define BINFS_RAM_START             (EBOOT_BINFS_BUFFER_UA_START)    // Uncached
#define BINFS_RAM_LENGTH            (EBOOT_BINFS_BUFFER_SIZE)

// Nk Memory reigions defined in config.bib...
#define ROM_RAMIMAGE_START          (IMAGE_NK_CA_START)
#define ROM_RAMIMAGE_SIZE           (IMAGE_NK_SIZE)

#define FILE_CACHE_START            (IMAGE_NK_CA_START)    // Uncached

// Driver globals pointer (parameter sharing memory used by bootloader and OS).
#define pBSPArgs                    ((BSP_ARGS *)IMAGE_SHARE_ARGS_UA_START)

// Platform "BOOTME" root name.
#define PLATFORM_STRING             "SMDK6410"

#define SECTOR_TO_BLOCK(sector)		(sector/(SECTORS_PER_SUPAGE*PAGES_PER_SUBLK))
#define BLOCK_TO_SECTOR(block)		(block*(SECTORS_PER_SUPAGE*PAGES_PER_SUBLK))
#define SECTORS_PER_BLOCK			(SECTORS_PER_SUPAGE*PAGES_PER_SUBLK)

//#define SB_PAGE_SIZE					(512)
//#define LB_PAGE_SIZE					(2048)

//#define SECTOR_TO_BLOCK(sector)			((sector) >> ((IS_LB)?6:5) )
//#define BLOCK_TO_SECTOR(block)			((block)  << ((IS_LB)?6:5) )

//#ifndef BADBLOCKMARK
//#define BADBLOCKMARK					(0x00)
//#endif

// fs: sector number
// returns block aligned value
__inline DWORD SECTOR_TO_BLOCK_SIZE(DWORD sn) {
    return ( (sn / SECTORS_PER_SUBLK) + ( (sn % SECTORS_PER_SUBLK) ? 1 : 0) );
}

// fs: file size in bytes
// returns sector aligned value
__inline DWORD FILE_TO_SECTOR_SIZE(DWORD fs) {
    return ( (fs / SECTOR_SIZE) + ( (fs % SECTOR_SIZE) ? 1 : 0) );
}

// ns: number of sectors
// N.B: returns sector aligned value since we can't tell
__inline DWORD SECTOR_TO_FILE_SIZE(DWORD ns) {
    return ( ns * SECTOR_SIZE );
}

//
// Boot Config Flags...
//

// BOOT_MODE_ flags indicate where we are booting from
#define BOOT_MODE_NAND          (0x00000001)
#define BOOT_MODE_NOR_AMD       (0x00000002)
#define BOOT_MODE_NOR_STRATA    (0x00000004)
#define BOOT_MODE_TEST          (0x00000008)
#define BOOT_MODE_MASK(dw)      (0x0000000F & (dw))

// BOOT_TYPE_ flags indicate whether we are booting directly off media, or downloading an image
#define BOOT_TYPE_DIRECT        (0x00000010)    // boot off media?
#define BOOT_TYPE_MULTISTAGE    (0x00000020)    // multi-stage bootloaders?
#define BOOT_TYPE_MASK(dw)      (0x000000F0 & (dw))

// TARGET_TYPE_ flags indicate where the image is to be written to.
#define TARGET_TYPE_RAMIMAGE    (0x00000100)    // Directly to SDRAM
#define TARGET_TYPE_CACHE       (0x00000200)    // FLASH_CACHE
#define TARGET_TYPE_NOR         (0x00000400)    // NOR Flash
#define TARGET_TYPE_NAND        (0x00000800)    // NAND Flash
//...
#define TARGET_TYPE_MASK(dw)    (0x00000F00 & (dw))

#define CONFIG_FLAGS_DHCP       (0x00001000)
#define CONFIG_FLAGS_KITL       (0x00002000)
#define CONFIG_FLAGS_KITLPOLL   (0x00004000)
#define CONFIG_FLAGS_MASK(dw)   (0x0000F000 & (dw))

#define BOOT_OPTION_CLEAN       (0x00010000)
#define BOOT_OPTION_HIVECLEAN   (0x00020000)
#define BOOT_OPTION_FORMATPARTITION (0x00040000)

#define CONFIG_BOOTDELAY_DEFAULT    (5)

//
// Boot device options
//
typedef enum
{
    BOOT_DEVICE_ETHERNET = 0,
    BOOT_DEVICE_USB_SERIAL,
    BOOT_DEVICE_USB_RNDIS,
    BOOT_DEVICE_USB_DNW
} BOOT_DEVICE_TYPE;

#define NUM_BOOT_DEVICES  4


//-------------------------------------
//
// Bootloader configuration parameters.
//
typedef struct _BOOTCFG {

    ULONG       ImageIndex;

    ULONG       ConfigFlags;
    ULONG       BootDelay;

    EDBG_ADDR   EdbgAddr;
    ULONG       SubnetMask;
    ULONG       BootDevice;
	DWORD		powerCTL;
	DWORD		displayType;
	DWORD		framebufferDepth;
	DWORD		backgroundColor;
	DWORD		logoHW;
	DWORD		debugUART;
} BOOT_CFG, *PBOOT_CFG;


//
// On disk structures for NAND bootloaders...
//

//
// OEM Reserved (Nand) Blocks for TOC and various bootloaders
//
#define STEPPINGSTONE_SIZE          (0x2000)    //< S3C6410 support 8kbytes Steppingstone

// NAND Boot (loads into SteppingStone) @ Block 0
#define NBOOT_BLOCK				(0)
#define NBOOT_BLOCK_SIZE		(1)
#define NBOOT_SECTOR			BLOCK_TO_SECTOR(NBOOT_BLOCK)

// TOC @ Block 1
#define TOC_BLOCK				(1)
#define TOC_BLOCK_SIZE			(1)
#define TOC_BLOCK_RESERVED		(1)
#define TOC_SECTOR                  BLOCK_TO_SECTOR(TOC_BLOCK)

// Eboot @ Block 2
#define EBOOT_BLOCK				(3)
#define EBOOT_BLOCK_SIZE		(5)
#define EBOOT_BLOCK_RESERVED	(2)
#define EBOOT_SECTOR                BLOCK_TO_SECTOR(EBOOT_BLOCK)

#define RESERVED_BOOT_BLOCKS	(NBOOT_BLOCK_SIZE + TOC_BLOCK_SIZE +TOC_BLOCK_RESERVED + EBOOT_BLOCK_SIZE + EBOOT_BLOCK_RESERVED)

// Images start after OEM Reserved Blocks
#define IMAGE_START_BLOCK           RESERVED_BOOT_BLOCKS
#define IMAGE_START_SECTOR          BLOCK_TO_SECTOR(IMAGE_START_BLOCK)

//
// OEM Defined TOC within Eboot NAND BLOCK
//
#define MAX_SG_SECTORS              14
#define IMAGE_STRING_LEN            16  // chars
#define MAX_TOC_DESCRIPTORS         4   // per sector
#define TOC_SIGNATURE               0x434F544E  // (NAND TOC)

// Default image descriptor to load.
// We store Eboot as image 0, nk.nb0 as image 1
#define DEFAULT_IMAGE_DESCRIPTOR    1

// IMAGE_TYPE_ flags indicate whether the image is a Bootloader,
// RAM image, supports BinFS, Multiple XIP, ...
//
#define IMAGE_TYPE_LOADER           0x00000001  // eboot.bin
#define IMAGE_TYPE_RAMIMAGE         0x00000002  // nk.bin
#define IMAGE_TYPE_BINFS            0x00000004
#define IMAGE_TYPE_MXIP             0x00000008
#define IMAGE_TYPE_RAWBIN            0x00000040    // raw bin .nb0
#define IMAGE_TYPE_STEPLDR            0x00000080    // stepldr.bin
#define IMAGE_TYPE_MASK(dw)        (0x0000000F & (dw))



// SG_SECTOR: supports chained (scatter gather) sectors.
// This structure equates to a sector-based MXIP RegionInfo in blcommon.
//
typedef struct _SG_SECTOR {

    DWORD dwSector;     // Starting sector of the image segment
    DWORD dwLength;     // Image length of this segment, in contigious sectors.

} SG_SECTOR, *PSG_SECTOR;


// IMAGE_DESCRIPTOR: describes the image to load.
// The image can be anything: bootloaders, RAMIMAGE, MXIP, ...
// Note: Our NAND uses H/W ECC, so no checksum needed.
//
typedef struct _IMAGE_DESCRIPTOR {

    // File version info
    DWORD dwVersion;                    // e.g: build number
    DWORD dwSignature;                  // e.g: "EBOT", "CFSH", etc
    UCHAR ucString[IMAGE_STRING_LEN];   // e.g: "PocketPC_2002"

    DWORD dwImageType;      // IMAGE_TYPE_ flags
    DWORD dwTtlSectors;     // TTL image size in sectors.
                            // We store size in sectors instead of bytes
                            // to simplify sector reads in Nboot.

    DWORD dwLoadAddress;    // Virtual address to load image (ImageStart)
    DWORD dwJumpAddress;    // Virtual address to jump (StartAddress/LaunchAddr)

    // This array equates to a sector-based MXIP MultiBINInfo in blcommon.
    // Unused entries are zeroed.
    // You could chain image descriptors if needed.
    SG_SECTOR sgList[MAX_SG_SECTORS];

    // BinFS support to load nk region only
    //struct
    //{
        ULONG dwStoreOffset;    // byte offset - not needed - remove!
        //ULONG RunAddress;     // nk dwRegionStart address
        //ULONG Length;         // nk dwRegionLength in bytes
        //ULONG LaunchAddress;  // nk dwLaunchAddr
    //} NKRegion;

} IMAGE_DESCRIPTOR, *PIMAGE_DESCRIPTOR;

//
// IMAGE_DESCRIPTOR signatures we know about
//
#define IMAGE_EBOOT_SIG             0x45424F54          // "EBOT", eboot.nb0
#define IMAGE_RAM_SIG               0x43465348          // "CFSH", nk.nb0
#define IMAGE_BINFS_SIG             (IMAGE_RAM_SIG + 1)

__inline BOOL VALID_IMAGE_DESCRIPTOR(PIMAGE_DESCRIPTOR pid) {
    return ( (pid) &&
      ((IMAGE_EBOOT_SIG == (pid)->dwSignature) ||
       (IMAGE_RAM_SIG == (pid)->dwSignature) ||
       (IMAGE_BINFS_SIG == (pid)->dwSignature)));
}

//  This is for MXIP chain.bin, which needs to be loaded into the SDRAM
//  during the start up.
typedef struct _CHAININFO {

    DWORD   dwLoadAddress;          // Load address in SDRAM
    DWORD   dwFlashAddress;         // Start location on the NAND
    DWORD   dwLength;               // The length of the image
} CHAININFO, *PCHAININFO;

//
// TOC: Table Of Contents, OEM on disk structure.
// sizeof(TOC) = SECTOR_SIZE.
// Consider the TOC_BLOCK to contain an array of PAGES_PER_BLOCK
// TOC entries, since the entire block is reserved.
//
typedef struct _TOC {
    DWORD               dwSignature;
    // How to boot the images in this TOC.
    // This could be moved into the image descriptor if desired,
    // but I prefer to conserve space.
    BOOT_CFG            BootCfg;

    // Array of Image Descriptors.
    IMAGE_DESCRIPTOR    id[MAX_TOC_DESCRIPTORS];

    CHAININFO           chainInfo;
} TOC, *PTOC;           // 512 bytes


__inline BOOL VALID_TOC(PTOC ptoc) {
    return ((ptoc) &&  (TOC_SIGNATURE == (ptoc)->dwSignature));
}


// Loader function prototypes.
//
BOOL InitEthDevice(PBOOT_CFG pBootCfg);
int OEMReadDebugByte(void);
BOOL OEMVerifyMemory(DWORD dwStartAddr, DWORD dwLength);
void Launch(DWORD dwLaunchAddr);
PVOID GetKernelExtPointer(DWORD dwRegionStart, DWORD dwRegionLength);
void OEMWriteDebugLED(WORD wIndex, DWORD dwPattern);

void InitializeDisplay(void);

BOOL WriteOSImageToBootMedia(DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr);
BOOL ReadOSImageFromBootMedia(void);
BOOL WriteRawImageToBootMedia(DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr);


BOOL    TOC_Init(DWORD dwEntry, DWORD dwImageType, DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr);
BOOL    TOC_Read(void);
BOOL    TOC_Write(void);
void    TOC_Print(void);
void Delay(UINT32 usec);
void MLC_LowLevelTest(void);
unsigned OEMDisplayBytes();


#endif  // _LOADER_H_.
