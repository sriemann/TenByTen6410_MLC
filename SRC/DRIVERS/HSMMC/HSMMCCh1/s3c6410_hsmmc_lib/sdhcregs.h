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

#ifndef _SDHCREGS_DEFINED
#define _SDHCREGS_DEFINED

#include <pshpack1.H>
typedef struct SSDHC_REGISTERS {
    USHORT SystemAddressLo;     // 00H
    USHORT SystemAddressHi;     // 02H
    USHORT BlockSize;           // 04H
    USHORT BlockCount;          // 06H
    USHORT Argument0;           // 08H
    USHORT Argument1;           // 0AH
    USHORT TransferMode;        // 0CH
    USHORT Command;             // 0EH
    USHORT R0;                  // 10H
    USHORT R1;                  // 12H
    USHORT R2;                  // 14H
    USHORT R3;                  // 16H
    USHORT R4;                  // 18H
    USHORT R5;                  // 1AH
    USHORT R6;                  // 1CH
    USHORT R7;                  // 1EH
    USHORT BufferDataPort0;     // 20H
    USHORT BufferDataPort1;     // 22H
    ULONG  PresentState;        // 24H
    UCHAR  HostControl;         // 28H
    UCHAR  PowerControl;        // 29H
    UCHAR  BlockGapControl;     // 2AH
    UCHAR  WakeUpControl;       // 2BH
    USHORT ClockControl;        // 2CH
    UCHAR  TimeOutControl;      // 2EH
    UCHAR  SoftReset;           // 2FH
    USHORT NormalIntStatus;     // 30H
    USHORT ErrorIntStatus;      // 32H
    USHORT NormalIntStatusEnable;       // 34H
    USHORT ErrorIntStatusEnable;        // 36H
    USHORT NormalIntSignalEnable;       // 38H
    USHORT ErrorIntSignalEnable;        // 3AH
    USHORT AutoCMD12ErrorStatus;        // 3CH
    USHORT Reserved1;                   // 3EH
    ULONG  Capabilities;                // 40H
    ULONG  Reserved2;                   // 44H
    ULONG  MaxCurrentCapabilites;       // 48H
    ULONG  Reserved3;                   // 4CH
    USHORT FEAER;                       // 50H
    USHORT FEERR;                       // 52H
    ULONG  ADMAErr;                     // 54H
    ULONG  ADMASysAddr;                 // 58H
    ULONG  Reserved4[9];                // 5CH
    ULONG  Control2;                    // 80H
    ULONG  Control3;                    // 84H
    ULONG  Reserved5;                   // 88H
    ULONG  Control4;                    // 8CH
    ULONG  Reserved6[27];               // 90H
    USHORT SlotInterruptStatus;         // FCH
    USHORT HostControllerVersion;       // FEH
} *PSDHC_REGISTERS;
#include <poppack.h>


// Standard host controller registers
#define SDHC_SYSTEMADDRESS_LO         0x00
#define SDHC_SYSTEMADDRESS_HI         0x02
#define SDHC_BLOCKSIZE                0x04
#define SDHC_BLOCKCOUNT               0x06
#define SDHC_ARGUMENT_0               0x08
#define SDHC_ARGUMENT_1               0x0A
#define SDHC_TRANSFERMODE             0x0C
#define SDHC_COMMAND                  0x0E
#define SDHC_R0                       0x10
#define SDHC_R1                       0x12
#define SDHC_R2                       0x14
#define SDHC_R3                       0x16
#define SDHC_R4                       0x18
#define SDHC_R5                       0x1A
#define SDHC_R6                       0x1C
#define SDHC_R7                       0x1E
#define SDHC_BUFFER_DATA_PORT_0       0x20
#define SDHC_BUFFER_DATA_PORT_1       0x22
#define SDHC_PRESENT_STATE            0x24
#define SDHC_HOST_CONTROL             0x28
#define SDHC_POWER_CONTROL            0x29
#define SDHC_BLOCK_GAP_CONTROL        0x2A
#define SDHC_WAKEUP_CONTROL           0x2B
#define SDHC_CLOCK_CONTROL            0x2C
#define SDHC_TIMEOUT_CONTROL          0x2E
#define SDHC_SOFT_RESET               0x2F
#define SDHC_NORMAL_INT_STATUS        0x30
#define SDHC_ERROR_INT_STATUS         0x32
#define SDHC_NORMAL_INT_STATUS_ENABLE 0x34
#define SDHC_ERROR_INT_STATUS_ENABLE  0x36
#define SDHC_NORMAL_INT_SIGNAL_ENABLE 0x38
#define SDHC_ERROR_INT_SIGNAL_ENABLE  0x3A
#define SDHC_AUTOCMD12_ERROR_STATUS   0x3C
#define SDHC_Reserved1                0x3E
#define SDHC_CAPABILITIES             0x40
#define SDHC_Reserved2                0x44
#define SDHC_MAX_CURRENT_CAPABILITIES 0x48
#define SDHC_Reserved3                0x4C
//#define   SDHC_Reserved4[43]        // 50H
#define SDHC_ADMA_ERROR_STATUS          0x54
#define SDHC_ADMA_SYSTEMADDRESS_LO      0x58
#define SDHC_ADMA_SYSTEMADDRESS_HI      0x5C
#define SDHC_CONTROL2                   0x80
#define SDHC_CONTROL3                   0x84
#define SDHC_CONTROL4                    0x8C

#define SDHC_SLOT_INT_STATUS            0xFC
#define SDHC_HOST_CONTROLLER_VER        0xFE

// Transfer Mode Register
#define TXN_MODE_DMA                    0x0001
#define TXN_MODE_BLOCK_COUNT_ENABLE     0x0002
#define TXN_MODE_AUTO_CMD12             0x0004
#define TXN_MODE_DATA_DIRECTION_READ    0x0010
#define TXN_MODE_MULTI_BLOCK            0x0020

// Command Register (3.2.6)
#define CMD_NO_RESPONSE                 0x0000
#define CMD_RESPONSE_LENGTH_136         0x0001
#define CMD_RESPONSE_LENGTH_48          0x0002
#define CMD_RESPONSE_LENGTH_48_BUSY     0x0003
#define CMD_CRC_CHECK                   0x0008
#define CMD_INDEX_CHECK                 0x0010
#define CMD_DATA_PRESENT                0x0020
#define CMD_TYPE_NORMAL                 0x0000
#define CMD_TYPE_RESUME                 0x0040
#define CMD_TYPE_SUSPEND                0x0080
#define CMD_TYPE_ABORT                  0x00c0

#define CMD_INDEX_MASK                  0x3F00
#define CMD_INDEX_SHIFT                 8
#define CMD_RESPONSE_R2                 (CMD_RESPONSE_LENGTH_136 | CMD_CRC_CHECK)
#define CMD_RESPONSE_R3_R4              CMD_RESPONSE_LENGTH_48
#define CMD_RESPONSE_R1_R5_R6_R7           (CMD_RESPONSE_LENGTH_48 | CMD_CRC_CHECK | CMD_INDEX_CHECK)
#define CMD_RESPONSE_R1B_R5B            (CMD_RESPONSE_LENGTH_48_BUSY | CMD_CRC_CHECK | CMD_INDEX_CHECK)

// Present State Register (3.2.9)
#define STATE_CMD_INHIBIT               0x00000001
#define STATE_DAT_INHIBIT               0x00000002
#define STATE_DAT_ACTIVE                0x00000004
#define STATE_WRITE_ACTIVE              0x00000100
#define STATE_READ_ACTIVE               0x00000200
#define STATE_BUF_WRITE_ENABLE          0x00000400
#define STATE_BUF_READ_ENABLE           0x00000800
#define STATE_CARD_INSERTED             0x00010000
#define STATE_CARD_STATE_STABLE         0x00020000
#define STATE_CARD_DETECT_SIGNAL        0x00040000
#define STATE_WRITE_PROTECT             0x00080000
#define STATE_DAT_LINE_SIGNAL           0x00F00000
#define STATE_CMD_LINE_SIGNAL           0x01000000

// Host Control Register (3.2.10)
#define HOSTCTL_LED_CONTROL             0x01
#define HOSTCTL_DAT_WIDTH               0x02
#define HOSTCTL_DAT_WIDTH_8BIT          0x20
#define HOSTCTL_HIGH_SPEED              0x04

// Power Control Register (3.2.11)
#define SDBUS_POWER_ON                  0x01
#define SDBUS_VOLTAGE_SELECT_3_3V       0x0E
#define SDBUS_VOLTAGE_SELECT_3_0V       0x0C
#define SDBUS_VOLTAGE_SELECT_1_8V       0x0A

// Wakeup Control Register (3.2.13)
#define WAKEUP_INTERRUPT                0x01
#define WAKEUP_INSERTION                0x02
#define WAKEUP_REMOVAL                  0x04

#define WAKEUP_ALL_SOURCES              (WAKEUP_INTERRUPT | WAKEUP_INSERTION | WAKEUP_REMOVAL)

// Clock Control Register (3.2.14)
#define CLOCK_INTERNAL_ENABLE           0x0001
#define CLOCK_STABLE                    0x0002
#define CLOCK_ENABLE                    0x0004
#define CLOCK_EXTERNAL_STABLE                  0x0008

// Timeout Control Register (3.2.15)
#define TIME_DATA_TIMEOUT_MASK          0x000F

// Soft Reset Register  (3.2.16)
#define SOFT_RESET_ALL                  0x0001
#define SOFT_RESET_CMD                  0x0002
#define SOFT_RESET_DAT                  0x0004

// Normal Interrupt status register (3.2.17)
#define NORMAL_INT_STATUS_CMD_COMPLETE   0x0001
#define NORMAL_INT_STATUS_TRX_COMPLETE   0x0002
#define NORMAL_INT_STATUS_BLOCK_GAP      0x0004
#define NORMAL_INT_STATUS_DMA            0x0008
#define NORMAL_INT_STATUS_BUF_WRITE_RDY  0x0010
#define NORMAL_INT_STATUS_BUF_READ_RDY   0x0020
#define NORMAL_INT_STATUS_CARD_INSERTION 0x0040
#define NORMAL_INT_STATUS_CARD_REMOVAL   0x0080
#define NORMAL_INT_STATUS_CARD_INT       0x0100
#define NORMAL_INT_STATUS_ERROR_INT      0x8000

// Error Interrupt Status Register (3.2.18)
#define ERR_INT_STATUS_CMD_TIMEOUT       0x0001
#define ERR_INT_STATUS_CMD_CRC           0x0002
#define ERR_INT_STATUS_CMD_CONFLICT      (ERR_INT_STATUS_CMD_TIMEOUT | ERR_INT_STATUS_CMD_CRC_ERR)
#define ERR_INT_STATUS_CMD_ENDBIT        0x0004
#define ERR_INT_STATUS_CMD_INDEX         0x0008
#define ERR_INT_STATUS_DAT_TIMEOUT       0x0010
#define ERR_INT_STATUS_DAT_CRC           0x0020
#define ERR_INT_STATUS_DAT_ENDBIT        0x0040
#define ERR_INT_STATUS_BUS_POWER         0x0080
#define ERR_INT_STATUS_AUTOCMD12         0x0100
#define ERR_INT_STATUS_ADMA              0x0200
#define ERR_INT_STATUS_VENDOR            0xF000

#define IS_CMD_LINE_ERROR(x)    (((x) & 0x0F) != 0)
#define IS_DAT_LINE_ERROR(x)    (((x) & 0x70) != 0)

// Normal interrupt status enable register (3.2.19)
#define NORMAL_INT_ENABLE_CMD_COMPLETE   0x0001
#define NORMAL_INT_ENABLE_TRX_COMPLETE   0x0002
#define NORMAL_INT_ENABLE_BLOCK_GAP      0x0004
#define NORMAL_INT_ENABLE_DMA            0x0008
#define NORMAL_INT_ENABLE_BUF_WRITE_RDY  0x0010
#define NORMAL_INT_ENABLE_BUF_READ_RDY   0x0020
#define NORMAL_INT_ENABLE_CARD_INSERTION 0x0040
#define NORMAL_INT_ENABLE_CARD_REMOVAL   0x0080
#define NORMAL_INT_ENABLE_CARD_INT       0x0100

// Error Interrupt status enable register (3.2.20)
#define ERR_INT_ENABLE_CMD_TIMEOUT       0x0001
#define ERR_INT_ENABLE_CMD_CRC           0x0002
#define ERR_INT_ENABLE_CMD_ENDBIT        0x0004
#define ERR_INT_ENABLE_CMD_INDEX         0x0008
#define ERR_INT_ENABLE_DAT_TIMEOUT       0x0010
#define ERR_INT_ENABLE_DAT_CRC           0x0020
#define ERR_INT_ENABLE_DAT_ENDBIT        0x0040
#define ERR_INT_ENABLE_BUS_POWER         0x0080
#define ERR_INT_ENABLE_AUTOCMD12         0x0100

// Normal interrupt signal enable register (3.2.21)
#define NORMAL_INT_SIGNAL_CMD_COMPLETE   0x0001
#define NORMAL_INT_SIGNAL_TRX_COMPLETE   0x0002
#define NORMAL_INT_SIGNAL_BLOCK_GAP      0x0004
#define NORMAL_INT_SIGNAL_DMA            0x0008
#define NORMAL_INT_SIGNAL_BUF_WRITE_RDY  0x0010
#define NORMAL_INT_SIGNAL_BUF_READ_RDY   0x0020
#define NORMAL_INT_SIGNAL_CARD_INSERTION 0x0040
#define NORMAL_INT_SIGNAL_CARD_REMOVAL   0x0080
#define NORMAL_INT_SIGNAL_CARD_INT       0x0100
#define NORMAL_INT_SIGNAL_FIFO_SD_INT    0x7800

// Error Interrupt signal enable register (3.2.22)
#define ERR_INT_SIGNAL_CMD_TIMEOUT       0x0001
#define ERR_INT_SIGNAL_CMD_CRC           0x0002
#define ERR_INT_SIGNAL_CMD_ENDBIT        0x0004
#define ERR_INT_SIGNAL_CMD_INDEX         0x0008
#define ERR_INT_SIGNAL_DAT_TIMEOUT       0x0010
#define ERR_INT_SIGNAL_DAT_CRC           0x0020
#define ERR_INT_SIGNAL_DAT_ENDBIT        0x0040
#define ERR_INT_SIGNAL_BUS_POWER         0x0080
#define ERR_INT_SIGNAL_AUTOCMD12         0x0100

#include <pshpack1.h>
typedef union __SSDHC_CAPABILITIES {
    struct {
        ULONG TOFreq:6;
        ULONG Rsvd1:1;
        ULONG TimeoutUnit:1;
        
        ULONG ClkFreq:6;
        ULONG Rsvd2:2;
        
        ULONG MaxBlockLen:2;
        ULONG :1;
        ULONG ADMA2:1;
        ULONG ADMA1:1;
        ULONG HighSpeed:1;
        ULONG SDMA:1;
        ULONG SuspendResume:1;
        
        ULONG Voltage33:1;
        ULONG Voltage30:1;
        ULONG Voltage18:1;
        ULONG :1;
        ULONG Bus64BitSystem:1;
        ULONG :3;
    } bits;

    DWORD dw;
}SSDHC_CAPABILITIES,*PSSDHC_CAPABILITIES;
typedef union __SSDHC_VERSION {
    struct {
        USHORT SpecVersionNumber:8;
        USHORT VendorVersionNumber:8;
    } bits;
    USHORT uw;
} SSDHC_VERSION,*PSSDHC_VERSION;
#include <poppack.h>

// Values that MaxBlockLen maps to
#define SDHC_CAPABILITIES_MAX_BLOCK_LENGTH_0    512
#define SDHC_CAPABILITIES_MAX_BLOCK_LENGTH_1    1024
#define SDHC_CAPABILITIES_MAX_BLOCK_LENGTH_2    2048

#define SDHC_MAX_CLOCK_FREQUENCY      63000000    // 63 MHz

#define SDHC_TIMEOUT_CONTROL_MAX      0xE //14

#define SDHC_CONTROL2_ENSTAASYNCCLR   0x80000000    // Write Status Clear Async Mode Enable
#define SDHC_CONTROL2_ENCMDCNFMSK     0x40000000    // Command Conflict Mask Enable
#define SDHC_CONTROL4_STABUSY         0x1

// Returns the timeout control value given a timeout clock rate and 
// timeout in seconds.
inline
DOUBLE
SdhcTimeoutSecondsToControl(
    DOUBLE dTimeoutClock,   // In Hz
    DOUBLE dTimeout         // In seconds
    )
{
    DEBUGCHK(dTimeoutClock > 0.0);
    DEBUGCHK(dTimeout > 0.0);

    return ( log(dTimeoutClock * dTimeout) / log(2.0) ) - 13.0;
}


// Returns the timeout in seconds given a timeout clock rate and timeout
// control value.
inline
DOUBLE
SdhcTimeoutControlToSeconds(
    DOUBLE dTimeoutClock,   // In Hz
    DOUBLE dTimeoutControl
    )
{
    DEBUGCHK(dTimeoutClock > 0.0);
    DEBUGCHK(dTimeoutControl >= 0.0);
    DEBUGCHK(dTimeoutControl <= SDHC_TIMEOUT_CONTROL_MAX);

    return pow(2.0, dTimeoutControl + 13.0) / dTimeoutClock;
}


#define SDHC_RESPONSE_BYTES   (SDCARD_RESPONSE_BUFFER_BYTES - 1) // -1 for CRC
#define SDHC_MIN_BLOCK_LENGTH 1
#define SDHC_MAX_BLOCK_LENGTH 2048

#define SDHC_MAX_SLOTS                8

#endif // _SDHCREGS_DEFINED

// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

