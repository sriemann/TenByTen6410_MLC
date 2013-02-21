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

// Header file for data types and definitions from the SD Card specification
#ifndef _SD_CARD_H_
#define _SD_CARD_H_

// SD card bus commands
#define SD_CMD_GO_IDLE_STATE        0       // CMD0
#define SD_CMD_MMC_SEND_OPCOND      1       // CMD1
#define SD_CMD_ALL_SEND_CID         2       // CMD2
#define SD_CMD_MMC_SET_RCA          3       // CMD3
#define SD_CMD_SEND_RELATIVE_ADDR   3       // CMD3
#define SD_CMD_SET_DSR              4       // CMD4
#define SD_CMD_IO_OP_COND           5       // CMD5
#define SD_CMD_SWITCH_FUNCTION      6       // CMD6
#define SD_CMD_SELECT_DESELECT_CARD 7       // CMD7
#define SD_CMD_SEND_IF_COND         8       // CMD8  it is for SPEC 2.0

#ifdef _MMC_SPEC_42_
#define MMC_CMD_SEND_EXT_CSD        8       // CMD8
#endif

#define SD_CMD_SEND_CSD             9       // CMD9
#define SD_CMD_SEND_CID             10      // CMD10
#define SD_CMD_STOP_TRANSMISSION    12      // CMD12
#define SD_CMD_SEND_STATUS          13      // CMD13

#ifdef _MMC_SPEC_42_ 
#define MMC_CMD_READ_BUSTEST        14      // CMD14 
#endif

#define SD_CMD_GO_INACTIVE_STATE    15       // CMD15
#define SD_CMD_SET_BLOCKLEN         16       // CMD16
#define SD_CMD_READ_SINGLE_BLOCK    17       // CMD17
#define SD_CMD_READ_MULTIPLE_BLOCK  18       // CMD18

#ifdef _MMC_SPEC_42_  
#define MMC_CMD_WRITE_BUSTEST       19       // CMD19
#define MMC_CMD_SET_BLOCK_LENGTH    23       // CMD23
#endif

#define SD_CMD_SET_BLOCK_COUNT      23       // CMD23 
#define SD_CMD_WRITE_BLOCK          24       // CMD24
#define SD_CMD_WRITE_MULTIPLE_BLOCK 25       // CMD25
#define SD_CMD_PROGRAM_CSD          27       // CMD27
#define SD_CMD_SET_WRITE_PROT       28       // CMD28
#define SD_CMD_CLR_WRITE_PROT       29       // CMD29
#define SD_CMD_SEND_WRITE_PROT      30       // CMD30
#define SD_CMD_ERASE_WR_BLK_START   32       // CMD32
#define SD_CMD_ERASE_WR_BLK_END     33       // CMD33
#define SD_CMD_ERASE                38       // CMD38
#define SD_CMD_LOCK_UNLOCK          42       // CMD42
#define SD_CMD_IO_RW_DIRECT         52       // CMD52
#define SD_CMD_IO_RW_EXTENDED       53       // CMD53
#define SD_CMD_APP_CMD              55       // CMD55
#define SD_CMD_GEN_CMD              56       // CMD56

// command packet byte indexes
#define SD_COMMAND_BYTE_INDEX           5   // command is byte 5
#define SD_COMMAND_ARG_BYTE0_INDEX      1   // argument byte 0 is in byte 1
#define SD_COMMAND_ARG_BYTE1_INDEX      2   // argument byte 1 is in byte 2
#define SD_COMMAND_ARG_BYTE2_INDEX      3   // argument byte 2 is in byte 3
#define SD_COMMAND_ARG_BYTE3_INDEX      4   // argument byte 3 is in byte 4

// application Specific commands
#define SD_ACMD_SET_BUS_WIDTH               6
#define SD_ACMD_SD_STATUS                   13
#define SD_ACMD_SEND_NUM_WR_BLOCKS          22
#define SD_ACMD_SET_WR_BLOCK_ERASE_COUNT    23
#define SD_ACMD_SD_SEND_OP_COND             41
#define SD_ACMD_SET_CLR_CARD_DETECT         42
#define SD_ACMD_SEND_SCR                    51

// arg definition for ACMD SET_BUS_WIDTH
#define SD_ACMD_ARG_SET_BUS_4BIT            0x00000002

#ifdef _MMC_SPEC_42_ 
#define MMC_ACMD_ARG_SET_HIGHSPEED      0x03b90100  // (3<<24)|(185<<16)|(1<<8)
#define MMC_ACMD_ARG_SET_BUS_8BIT       0x03b70200  // (3<<24)|(183<<16)|(2<<8)
#define MMC_ACMD_ARG_SET_BUS_4BIT       0x03b70100  // (3<<24)|(183<<16)|(1<<8)
#define MMC_ACMD_ARG_SET_BUS_1BIT       0x03b70000  // (3<<24)|(183<<16)|(0<<8)
#define SD_ACMD_ARG_GET_TRANS_SPEED     ((0x1<<31)|(0xFFFF<<8)|(1<<0))
#endif

// SDIO commands
#define SD_IO_RW_DIRECT                 SD_CMD_IO_RW_DIRECT
#define SD_IO_RW_EXTENDED               SD_CMD_IO_RW_EXTENDED

// CMD53 Bits
#define SD_CMD53_RW_MASK                0x80000000
#define SD_CMD53_READ_OP                0
#define SD_CMD53_WRITE_OP               SD_CMD53_RW_MASK
#define SD_CMD53_FUNCTION_NUMBER        0x70000000
#define SD_CMD53_BLOCK_MODE             0x08000000
#define SD_CMD53_OPCODE                 0x04000000
#define SD_CMD53_REGISTER_ADDRESS       0x03fffe00
#define SD_CMD53_REGISTER_ADDRESS_POS   9
#define SD_CMD53_BLOCK_COUNT            (( 1 << SD_CMD53_REGISTER_ADDRESS_POS ) - 1 )

// card status bits
#define SD_STATUS_OUT_OF_RANGE          0x80000000
#define SD_STATUS_ADDRESS_ERROR         0x40000000
#define SD_STATUS_BLOCK_LEN_ERROR       0x20000000
#define SD_STATUS_ERASE_SEQ_ERROR       0x10000000
#define SD_STATUS_ERASE_PARAM           0x08000000
#define SD_STATUS_WP_VIOLATION          0x04000000
#define SD_STATUS_CARD_IS_LOCKED        0x02000000
#define SD_STATUS_LOCK_UNLOCK_FAILED    0x01000000
#define SD_STATUS_COM_CRC_ERROR         0x00800000
#define SD_STATUS_ILLEGAL_COMMAND       0x00400000
#define SD_STATUS_CARD_ECC_FAILED       0x00200000
#define SD_STATUS_CC_ERROR              0x00100000
#define SD_STATUS_ERROR                 0x00080000
#define SD_STATUS_CID_CSD_OVERWRITE     0x00010000
#define SD_STATUS_WP_ERASE_SKIP         0x00008000
#define SD_STATUS_CARD_ECC_DISABLED     0x00004000
#define SD_STATUS_ERASE_RESET           0x00002000

#define SD_STATUS_CURRENT_STATE_MASK    0x00001E00
#define SD_STATUS_CURRENT_STATE_SHIFT   9
#define SD_STATUS_CURRENT_STATE(sd_status) \
    (((sd_status)&SD_STATUS_CURRENT_STATE_MASK)>>SD_STATUS_CURRENT_STATE_SHIFT)

// states pulled out from the status word
#define SD_STATUS_CURRENT_STATE_IDLE    0
#define SD_STATUS_CURRENT_STATE_READY   1
#define SD_STATUS_CURRENT_STATE_IDENT   2
#define SD_STATUS_CURRENT_STATE_STDBY   3
#define SD_STATUS_CURRENT_STATE_TRAN    4
#define SD_STATUS_CURRENT_STATE_DATA    5
#define SD_STATUS_CURRENT_STATE_RCV     6
#define SD_STATUS_CURRENT_STATE_PRG     7
#define SD_STATUS_CURRENT_STATE_DIS     8

#define SD_STATUS_READY_FOR_DATA    0x00000100
#define SD_STATUS_APP_CMD           0x00000020
#define SD_STATUS_AKE_SEQ_ERROR     0x00000008

#define SD_STATUS_ERROR_MASK        0xFFF90008
#define SD_CARD_STATUS_SUCCESS(s)   (0 == ((s) & SD_STATUS_ERROR_MASK))



// defines for the SCR register
#define SCR_VERSION_1_0             0
#define SD_SCR_REGISTER_SIZE        8  // 64 bits
#define SD_SPEC_VERSION_1_0         0
#define SD_SECURITY_NONE            0
#define SD_SECURITY_PROTOCOL_1      1
#define SD_SECURITY_PROTOCOL_2      2
#define SD_BUS_WIDTH_1_BIT          0x1
#define SD_BUS_WIDTH_4_BIT          0x4

// defines for CSD register
#define SD_CSD_VERSION_1_0              0
#define SD_FILE_FORMAT_HARD_DISK_LIKE   0
#define SD_FILE_FORMAT_FLOPPY_LIKE      1
#define SD_FILE_FORMAT_UNIVERSAL        2
#define SD_FILE_FORMAT_OTHER            3

// define for the OCR register
#define SD_OCR_REGISTER_SIZE            4 // 32 bits
#define SD_IO_OCR_REGISTER_SIZE         3 // 24 bits
#define SD_VDD_WINDOW_1_6_TO_1_7        0x00000010      // 1.6 V to 1.7 Volts
#define SD_VDD_WINDOW_1_7_TO_1_8        0x00000020      // 1.7 V to 1.8 Volts
#define SD_VDD_WINDOW_1_8_TO_1_9        0x00000040      // 1.8 V to 1.9 Volts
#define SD_VDD_WINDOW_1_9_TO_2_0        0x00000080      // 1.9 V to 2.0 Volts
#define SD_VDD_WINDOW_2_0_TO_2_1        0x00000100      // 2.0 V to 2.1 Volts
#define SD_VDD_WINDOW_2_1_TO_2_2        0x00000200      // 2.1 V to 2.2 Volts
#define SD_VDD_WINDOW_2_2_TO_2_3        0x00000400      // 2.2 V to 2.3 Volts
#define SD_VDD_WINDOW_2_3_TO_2_4        0x00000800      // 2.3 V to 2.4 Volts
#define SD_VDD_WINDOW_2_4_TO_2_5        0x00001000      // 2.4 V to 2.5 Volts
#define SD_VDD_WINDOW_2_5_TO_2_6        0x00002000      // 2.5 V to 2.6 Volts
#define SD_VDD_WINDOW_2_6_TO_2_7        0x00004000      // 2.6 V to 2.7 Volts
#define SD_VDD_WINDOW_2_7_TO_2_8        0x00008000      // 2.7 V to 2.8 Volts
#define SD_VDD_WINDOW_2_8_TO_2_9        0x00010000      // 2.8 V to 2.9 Volts
#define SD_VDD_WINDOW_2_9_TO_3_0        0x00020000      // 2.9 V to 3.0 Volts
#define SD_VDD_WINDOW_3_0_TO_3_1        0x00040000      // 3.0 V to 3.1 Volts
#define SD_VDD_WINDOW_3_1_TO_3_2        0x00080000      // 3.1 V to 3.2 Volts
#define SD_VDD_WINDOW_3_2_TO_3_3        0x00100000      // 3.2 V to 3.3 Volts
#define SD_VDD_WINDOW_3_3_TO_3_4        0x00200000      // 3.3 V to 3.4 Volts
#define SD_VDD_WINDOW_3_4_TO_3_5        0x00400000      // 3.4 V to 3.5 Volts
#define SD_VDD_WINDOW_3_5_TO_3_6        0x00800000      // 3.5 V to 3.6 Volts
#define SD_CARD_POWER_UP_STATUS         0x80000000      // powerup finished indicator

// define for SWITCH_FUNCTION CMD6.
#define SD_SWITCH_FUNCTION_DATA_SIZE    (512/8)

// defines for SD IO Function Basic Information registers
#define SD_IO_NON_STANDARD_DEVICE_CODE  0

// defines for CIS tuples       
#define SD_CISTPL_NULL                  0x00
#define SD_CISTPL_CHECKSUM              0x10
#define SD_CISTPL_VERS_1                0x15
#define SD_CISTPL_ALTSTR                0x16
#define SD_CISTPL_MANFID                0x20
#define SD_CISTPL_FUNCID                0x21
#define SD_CISTPL_FUNCE                 0x22
#define SD_CISTPL_SDIO_STD              0x91
#define SD_CISTPL_SDIO_EXT              0x92
#define SD_CISTPL_END                   0xFF
#define SD_TUPLE_LINK_END               0xFF

#define SD_CISTPLE_MAX_BODY_SIZE        256
#define SD_CISTPL_FUNCID_BODY_SIZE      2
#define SD_CISTPL_MANFID_BODY_SIZE      4


#define SD_CISTPL_FUNCE_COMMON_TYPE     0x00
#define SD_CISTPL_FUNCE_FUNCTION_TYPE   0x01

#include <pshpack1.h>
typedef struct _SD_CISTPL_FUNCE_FUNCTION {
    BYTE  bType;
    union {
        struct {
            BYTE FN_WUS : 1;
        };
        BYTE  bFunctionInfo;
    };
    BYTE  bStdIORev;
    DWORD dwCardPSN;
    DWORD dwCSASize;
    BYTE  bCSAProperty;
    WORD  wMaxBlkSize;
    DWORD dwOCR;
    BYTE  bOpMinPwr;
    BYTE  bOpAvgPwr;
    BYTE  bOpMaxPwr;
    BYTE  bSbMinPwr;
    BYTE  bSbAvgPwr;
    BYTE  bSbMaxPwr;
    WORD  wMinBw;
    WORD  wOptBw;
}SD_CISTPL_FUNCE_FUNCTION, *PSD_CISTPL_FUNCE_FUNCTION;
#include <poppack.h>


// SD card status
typedef DWORD SD_CARD_STATUS;

// defines for Card IDentification (CID) register.
#define SD_CID_REGISTER_SIZE    16  // 128 bits
#define SD_CID_PSN_OFFSET       3
#define SD_CID_PRV_OFFSET       7
#define SD_CID_PNM_OFFSET       8
#define SD_CID_OID_OFFSET       13
#define SD_CID_MID_OFFSET       15
#define SD_CID_MDT_OFFSET       1
#define SD_CID_MONTH_MASK       0x0F
#define SD_CID_YEAR0_MASK       0xF0
#define SD_CID_YEAR_SHIFT       4
// some of the CID definitions are different for MMC cards...
#define MMC_CID_MDT_OFFSET      1
#define MMC_CID_PSN_OFFSET      2
#define MMC_CID_PRV_OFFSET      6
#define MMC_CID_PNM_OFFSET      7
#define MMC_CID_MONTH_MASK      0xF0
#define MMC_CID_YEAR_MASK       0x0F
#define MMC_CID_MONTH_SHIFT     4

// macros for Card Specific Data (CSD) register.
#define SD_CSD_REGISTER_SIZE                16      // 128 bits

// bit slice definitions
#define SD_CSD_VERSION_BIT_SLICE            126
#define SD_CSD_VERSION_SLICE_SIZE           2

#ifdef _MMC_SPEC_42_ 
#define SD_CSD_SPEC_VERSION_BIT_SLICE       122   // 122 ~ 125 bits of CSD register 
#define SD_CSD_SPEC_VERSION_SLICE_SIZE      4     // contain spec. version information 
#endif   

// 120-125, 6 reserved bits
#define SD_CSD_TAAC_BIT_SLICE               112
#define SD_CSD_TAAC_SLICE_SIZE              8
#define SD_CSD_NSAC_BIT_SLICE               104
#define SD_CSD_NSAC_SLICE_SIZE              8
#define SD_CSD_TRANS_SPEED_BIT_SLICE        96
#define SD_CSD_TRANS_SPEED_SLICE_SIZE       8
#define SD_CSD_CCC_BIT_SLICE                84
#define SD_CSD_CCC_SLICE_SIZE               12
#define SD_CSD_READ_BL_LEN_BIT_SLICE        80
#define SD_CSD_READ_BL_LEN_SLICE_SIZE       4
#define SD_CSD_READ_BL_PARTIAL_BIT_SLICE    79
#define SD_CSD_READ_BL_PARTIAL_SLICE_SIZE   1
#define SD_CSD_WRITE_BL_MISALIGN_BIT_SLICE  78
#define SD_CSD_WRITE_BL_MISALIGN_SLICE_SIZE 1
#define SD_CSD_READ_BL_MISALIGN_BIT_SLICE   77
#define SD_CSD_READ_BL_MISALIGN_SLICE_SIZE  1
#define SD_CSD_DSR_IMP_BIT_SLICE            76
#define SD_CSD_DSR_IMP_SLICE_SIZE           1
// 74-75, 2 reserved bits
#define SD_CSD_CSIZE_BIT_SLICE              62
#define SD_CSD_CSIZE_SLICE_SIZE             12
#define SD_CSD20_CSIZE_BIT_SLICE            48
#define SD_CSD20_CSIZE_SLICE_SIZE           22
#define SD_CSD_R_CURR_MIN_BIT_SLICE         59
#define SD_CSD_R_CURR_MIN_SLICE_SIZE        3
#define SD_CSD_R_CURR_MAX_BIT_SLICE         56
#define SD_CSD_R_CURR_MAX_SLICE_SIZE        3      
#define SD_CSD_W_CURR_MIN_BIT_SLICE         53
#define SD_CSD_W_CURR_MIN_SLICE_SIZE        3
#define SD_CSD_W_CURR_MAX_BIT_SLICE         50
#define SD_CSD_W_CURR_MAX_SLICE_SIZE        3
#define SD_CSD_CSIZE_MULT_BIT_SLICE         47
#define SD_CSD_CSIZE_MULT_SLICE_SIZE        3
#define SD_CSD_ERASE_BL_ENABLE_BIT_SLICE    46
#define SD_CSD_ERASE_BL_ENABLE_SLICE_SIZE   1
#define SD_CSD_ERASE_SECT_SIZE_BIT_SLICE    39
#define SD_CSD_ERASE_SECT_SIZE_SLICE_SIZE   7
#define SD_CSD_WP_GROUP_SIZE_BIT_SLICE      32
#define SD_CSD_WP_GROUP_SIZE_SLICE_SIZE     7
#define SD_CSD_WP_GRP_ENABLE_BIT_SLICE      31
#define SD_CSD_WP_GRP_ENABLE_SLICE_SIZE     1 
// 29-30, 3 reserved bits
#define SD_CSD_R2W_FACTOR_BIT_SLICE         26
#define SD_CSD_R2W_FACTOR_SLICE_SIZE        3
#define SD_CSD_WRITE_BL_LEN_BIT_SLICE       22
#define SD_CSD_WRITE_BL_LEN_SLICE_SIZE      4
#define SD_CSD_WRITE_BL_PARTIAL_BIT_SLICE   21
#define SD_CSD_WRITE_BL_PARTIAL_SLICE_SIZE  1
// 16-20, 5 reserved bits
#define SD_CSD_FILE_GROUP_BIT_SLICE         15
#define SD_CSD_FILE_GROUP_SLICE_SIZE        1
#define SD_CSD_COPY_FLAG_BIT_SLICE          14
#define SD_CSD_COPY_FLAG_SLICE_SIZE         1 
#define SD_CSD_PERM_WR_PROT_BIT_SLICE       13
#define SD_CSD_PERM_WR_PROT_SLICE_SIZE      1
#define SD_CSD_TEMP_WR_PROT_BIT_SLICE       12
#define SD_CSD_TEMP_WR_PROT_SLICE_SIZE      1
#define SD_CSD_FILE_FORMAT_BIT_SLICE        10 
#define SD_CSD_FILE_FORMAT_SLICE_SIZE       2
// 8-9, 2 reserved bits
#define SD_CSD_CRC_BIT_SLICE                1
#define SD_CSD_CRC_SLICE_SIZE               7

// CSD Card Command Classes (CCC)
#define SD_CSD_CCC_BASIC                    (0 << 0)
#define SD_CSD_CCC_RESERVED1                (1 << 1)
#define SD_CSD_CCC_BLOCK_READ               (1 << 2)
#define SD_CSD_CCC_RESERVED3                (1 << 3)
#define SD_CSD_CCC_BLOCK_WRITE              (1 << 4)
#define SD_CSD_CCC_ERASE                    (1 << 5)
#define SD_CSD_CCC_WRITE_PROTECTION         (1 << 6)
#define SD_CSD_CCC_LOCK_CARD                (1 << 7)
#define SD_CSD_CCC_APPLICATION_SPECIFIC     (1 << 8)
#define SD_CSD_CCC_RESERVED9                (1 << 9)
#define SD_CSD_CCC_RESERVED10               (1 << 10)
#define SD_CSD_CCC_RESERVED11               (1 << 11)

// CSD definitions for MMC cards
#define MMC_CSD_ER_GRP_SIZE_BIT_SLICE       42
#define MMC_CSD_ER_GRP_SIZE_SLICE_SIZE      5
#define MMC_CSD_ER_GRP_MULT_BIT_SLICE       37
#define MMC_CSD_ER_GRP_MULT_SLICE_SIZE      5
#define MMC_CSD_WP_GROUP_SIZE_BIT_SLICE     32
#define MMC_CSD_WP_GROUP_SIZE_SLICE_SIZE    5

#ifdef _MMC_SPEC_42_ 
#define MMC_EXTCSD_REGISTER_SIZE            512  // total byte length of Extended CSD register 
#endif 


// macros for bit shifting and masking in the CSD register
#define SD_CSD_TRANS_SPEED_VALUE_SHIFT      3
#define SD_CSD_TRANS_SPEED_VALUE_MASK       0x78
#define SD_CSD_TRANS_SPEED_UNIT_MASK        0x07
#define SD_CSD_TAAC_VALUE_MASK              0x78
#define SD_CSD_TAAC_VALUE_SHIFT             3
#define SD_CSD_TAAC_UNIT_MASK               0x07

// SCR register
#define SD_SCR_VERSION_BIT_SLICE            60
#define SD_SCR_VERSION_SLICE_SIZE           4
#define SD_SCR_SD_SPEC_BIT_SLICE            56
#define SD_SCR_SD_SPEC_SLICE_SIZE           4
#define SD_SCR_DATA_STAT_ERASE_BIT_SLICE    55
#define SD_SCR_DATA_STAT_ERASE_SLICE_SIZE   1
#define SD_SCR_SECURITY_SUPPORT_BIT_SLICE   52
#define SD_SCR_SECURITY_SUPPORT_SLICE_SIZE  3
#define SD_SCR_BUS_WIDTH_BIT_SLICE          48
#define SD_SCR_BUS_WIDTH_SLICE_SIZE         4
// bits 0 - 47 reserved   

#define SD_SCR_BUS_WIDTH_1_BIT  0x01    // bit 0
#define SD_SCR_BUS_WIDTH_4_BIT  0x04    // bit 4

#define SD_DSR_REGISTER_SIZE 2

// definitions for IO RW Register and Extended Arguments
#define SD_IO_OP_READ           0   // Read_Write
#define SD_IO_OP_WRITE          1   // Read_Write
#define SD_IO_RW_NORMAL         0   // RAW
#define SD_IO_RW_RAW            1   // RAW
#define SD_IO_BYTE_MODE         0   // BlockMode
#define SD_IO_BLOCK_MODE        1   // BlockMode
#define SD_IO_FIXED_ADDRESS     0   // IncrementAddress
#define SD_IO_INCREMENT_ADDRESS 1   // IncrementAddress

// macro to build RW_Direct Argument
//    Read_Write = 1 (write) or 0 (read)
//    RAW        = 1 (read after write) or 0 (echo write data)
//    Function   = function number 0-7
//    Address    = register address 0-1FFFF
//    Data       = data to write (set to 0 if read)
#define BUILD_IO_RW_DIRECT_ARG(Read_Write, RAW, Function, Address, Data) \
    (((Read_Write & 1) << 31) | ((Function & 0x7) << 28) | \
     ((RAW & 1) << 27) | ((Address & 0x1FFFF) << 9) | \
     (Data & 0xFF))

// macros to split the RW_Direct argument up again
#define IO_RW_DIRECT_ARG_RW(Arg)    (((Arg)>>31)&1)
#define IO_RW_DIRECT_ARG_RAW(Arg)   (((Arg)>>27)&1)
#define IO_RW_DIRECT_ARG_FUNC(Arg)  ((UCHAR)(((Arg)>>28)&0x7))
#define IO_RW_DIRECT_ARG_ADDR(Arg)  (((Arg)>>9)&0x1FFFF)
#define IO_RW_DIRECT_ARG_DATA(Arg)  ((UCHAR)((Arg)&0xFF))


// macro to build RW Extended
//    Read_Write = 1 (write) or 0 (read)
//    BlockMode  = 1 = Block Mode or 0 = byte mode
//    Function   = function number 0-7
//    Address    = register address 0-1FFFF
//    IncrementAddress = 1 = Card should increment the register address
//                       0 = Card should keep the address register fixed
//    count       = 9 bit Block or Byte Count
#define BUILD_IO_RW_EXTENDED_ARG(Read_Write, BlockMode, Function, Address, IncrementAddress, Count) \
    (((Read_Write & 1) << 31) | ((Function & 0x7) << 28) | \
     ((BlockMode & 1) << 27) | ((IncrementAddress & 1) << 26) | ((Address & 0x1FFFF) << 9) | \
     (Count & 0x1FF))

// macros to split the RW_Extended argument up again
#define IO_RW_EXTENDED_ARG_RW(Arg)      IO_RW_DIRECT_ARG_RW(Arg)
#define IO_RW_EXTENDED_ARG_BLK(Arg)     (((Arg)>>27)&1)
#define IO_RW_EXTENDED_ARG_OP(Arg)      (((Arg)>>26)&1)
#define IO_RW_EXTENDED_ARG_FUNC(Arg)    IO_RW_DIRECT_ARG_FUNC(Arg)
#define IO_RW_EXTENDED_ARG_ADDR(Arg)    IO_RW_DIRECT_ARG_ADDR(Arg) 
#define IO_RW_EXTENDED_ARG_CNT(Arg)     ((Arg)&0x1FF)


#define SDIO_CCCR_SPEC_REV_MASK     0x0F
#define SDIO_CCCR_SPEC_REV_1_0      0x00
#define SDIO_CCCR_SPEC_REV_1_1      0x01
// SDIO register offsets in the Common Register Area
#define SD_IO_REG_CCCR              0x00
#define SD_IO_REG_SPEC_REV          0x01
#define SD_IO_REG_ENABLE            0x02
#define SD_IO_REG_IO_READY          0x03
#define SD_IO_REG_INT_ENABLE        0x04
#define SD_IO_REG_INT_PENDING       0x05
#define SD_IO_REG_IO_ABORT          0x06
#define SD_IO_REG_BUS_CONTROL       0x07
#define SD_IO_REG_CARD_CAPABILITY   0x08
#define SD_IO_REG_COMMON_CIS_POINTER 0x09   // extends to 0xA, 0xB for 24 bits total 
#define SD_IO_REG_BUS_SUSPEND        0x0C
#define SD_IO_REG_FUNCTION_SELECT    0x0D
#define SD_IO_REG_EXEC_FLAGS         0x0E
#define SD_IO_REG_READY_FLAGS        0x0F
#define SD_IO_REG_FB0_BLOCK_SIZE     0x10   // extends to 0x11, for 16 bits total
#define SD_IO_REG_POWER_CONTROL      0x12   // Power Control


#define SD_IO_CIS_PTR_BYTES          3
#define SD_IO_CSA_PTR_BYTES          3

// FBR Definitions
#define SD_IO_FBR_1_OFFSET           0x100  // Function Basic Information Register offset
#define SD_IO_FBR_DEVICE_CODE        0x0    // Device code offset from FBR base
#define SD_IO_FBR_DEVICE_CODE_EXT    0x1    // Device code extention offset (1.1 only devices)
#define SD_IO_FBR_POWER_SELECT       0x2    // Power Control offset (1.1 only devices)
#define SD_IO_FBR_CISP_BYTE_0        0x9    // CIS Pointer byte 0 offset from FBR base
#define SD_IO_FBR_CISP_BYTE_1        0xA    // CIS Pointer byte 1 offset from FBR base
#define SD_IO_FBR_CISP_BYTE_2        0xB    // CIS Pointer byte 2 offset from FBR base
#define SD_IO_FBR_CSAP_BYTE_0        0xC    // CSA pointer byte 0 offerst from FBR
#define SD_IO_FBR_CSAP_BYTE_1        0xD    // CSA pointer byte 1 offerst from FBR
#define SD_IO_FBR_CSAP_BYTE_2        0xE    // CSA pointer byte 2 offerst from FBR
#define SD_IO_FBR_DATA_ACCESS        0xF    // CSA data access offset
#define SD_IO_FBR_IO_BLOCK_SIZE      0x10   // I/O block size, spans 2 bytes
#define SD_IO_FBR_LENGTH             0x100  // FBR register length

// I/O Abort register Bit definitions
#define SD_IO_REG_IO_ABORT_RES       (1 << 3)
#define SD_IO_REG_IO_ABORT_AS2       (1 << 2)
#define SD_IO_REG_IO_ABORT_AS1       (1 << 1)
#define SD_IO_REG_IO_ABORT_AS0       (1 << 0)

// Card Capability Bit definitions
#define SD_IO_CARD_CAP_4_BIT_LOW_SPEED             (1 << 7)
#define SD_IO_CARD_CAP_LOW_SPEED                   (1 << 6)
#define SD_IO_CARD_CAP_ENABLE_INTS_4_BIT_MB_MODE   (1 << 5)
#define SD_IO_CARD_CAP_SUPPORTS_INTS_4_BIT_MB_MODE (1 << 4)
#define SD_IO_CARD_CAP_SUPPORTS_SUSPEND_RESUME     (1 << 3)
#define SD_IO_CARD_CAP_SUPPORTS_READ_WAIT          (1 << 2)
#define SD_IO_CARD_CAP_SUPPORTS_MULTI_BLOCK_TRANS  (1 << 1)
#define SD_IO_CARD_CAP_SUPPORTS_DIRECT_COMMAND     (1 << 0)

#define SD_IO_CARD_POWER_CONTROL_SUPPORT        (1 << 0)
#define SD_IO_CARD_POWER_CONTROL_ENABLE         (1 << 1)

#define SD_IO_FUNCTION_POWER_SELECT_SUPPORT    (1 << 0)
#define SD_IO_FUNCTION_POWER_SELECT_STATE      (1 << 1)

#define SD_MASK_FOR_33V_POWER_CONTROL_TUPLE     0x00FF8000      // 3.6 V to 2.7 Volts


// device code mask 
#define SDIO_DEV_CODE_MASK           0x0F
// device I/O extension token
#define SDIO_DEV_CODE_USES_EXTENSION 0x0F

// bit definitions for BUS CONTROL registers
#define SD_IO_BUS_CONTROL_CD_DETECT_DISABLE        (1 << 7)
#define SD_IO_BUS_CONTROL_BUS_WIDTH_1BIT           0x00
#define SD_IO_BUS_CONTROL_BUS_WIDTH_4BIT           0x02

// bit definition for Interrupt enable register
#define SD_IO_INT_ENABLE_MASTER_ENABLE       (1 << 0)
#define SD_IO_INT_ENABLE_ALL_FUNCTIONS       0xFE

// response flag bits
#define SD_IO_R5_RESPONSE_FLAGS_BYTE_OFFSET  0x02
#define SD_IO_R5_RESPONSE_DATA_BYTE_OFFSET   0x01

#define SD_IO_R5_RESPONSE_ERROR_MASK         0xCB  // bits 7,6,3,1,0
#define SD_IO_COM_CRC_ERROR                  0x80
#define SD_IO_ILLEGAL_COMMAND                0x40
#define SD_IO_GENERAL_ERROR                  0x08
#define SD_IO_INVALID_FUNCTION               0x02
#define SD_IO_ARG_OUT_OF_RANGE               0x01

#define SD_IO_R5_RESPONSE_ERROR(r)  ((r) & SD_IO_R5_RESPONSE_ERROR_MASK)

#define SDCARD_COMMAND_BUFFER_BYTES     6           // 48 bit commands
#define SDCARD_RESPONSE_BUFFER_BYTES    17          // max 136 bits
#define SD_DEFAULT_CARD_ID_CLOCK_RATE   100000      // 100 khz 
#define SD_LOW_SPEED_RATE               400000      // 400 khz
#define SD_FULL_SPEED_RATE              25000000    // 25 Mhz
#define SDHC_FULL_SPEED_RATE            50000000    // 50 Mhz
#define MMC_FULL_SPEED_RATE             20000000    // 20 Mhz
#define MMCPLUS_SPEED_RATE                26000000    // 26 Mhz
#define HSMMC_FULL_SPEED_RATE            52000000    // 52 Mhz

#endif _SDCARD_H_

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

