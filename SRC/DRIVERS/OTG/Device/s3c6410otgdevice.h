//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name: 

        S3C6410OTGD.H

Abstract:

       Samsung S3C6410OTG USB Function Platform-Dependent Driver header.

--*/

#ifndef _S3C6410OTGD_H_
#define _S3C6410OTGD_H_

#include <bsp.h>
#include <usbfntypes.h>
#include <usbfn.h>
#include <devload.h>


#ifndef SHIP_BUILD
#define STR_MODULE _T("S3C6410UsbFn!")
#define SETFNAME() LPCTSTR pszFname = STR_MODULE _T(__FUNCTION__) _T(":")
#else
#define SETFNAME()
#endif

#define __MODULE__  _T("S3C6410USBFN")

#ifndef DEBUG
// The PDD should use this macro to set up the debug zones.
#define UFN_GENERATE_DPCURSETTINGS(szName, szZone8, szZone9, szZone10, szZone11, ulMask) \
    extern "C" DBGPARAM dpCurSettings = { \
        szName, \
        { \
            _T("Error"), _T("Warning"), _T("Init"), _T("Transfer"), \
            _T("Pipe"), _T("Send"), _T("Receive"), _T("USB Events"), \
            szZone8, szZone9, szZone10, szZone11, \
            _T("Function"), _T("Comments"), _T(""), _T("") \
        }, \
        ulMask \
    };
#endif


// Debug zone defs
#define UFN_ZONE_ERROR          DEBUGZONE(0)
#define UFN_ZONE_WARNING        DEBUGZONE(1)
#define UFN_ZONE_INIT           DEBUGZONE(2)
#define UFN_ZONE_TRANSFER       DEBUGZONE(3)
#define UFN_ZONE_PIPE           DEBUGZONE(4)
#define UFN_ZONE_SEND           DEBUGZONE(5)
#define UFN_ZONE_RECEIVE        DEBUGZONE(6)
#define UFN_ZONE_USB_EVENTS     DEBUGZONE(7)
#define UFN_ZONE_POWER          DEBUGZONE(8)
#define UFN_ZONE_TRACE          DEBUGZONE(9)

#define DBG_ERROR               (1 << 0)
#define DBG_WARNING             (1 << 1)
#define DBG_INIT                (1 << 2)
#define DBG_TRANSFER            (1 << 3)
#define DBG_PIPE                (1 << 4)
#define DBG_SEND                (1 << 5)
#define DBG_RECEIVE             (1 << 6)
#define DBG_USB_EVENTS          (1 << 7)
#define DBG_POWER               (1 << 8)
#define DBG_TRACE               (1 << 9)

#define USBFNCTL_RETAILZONES        (DBG_ERROR)
#define USBFNCTL_DEBUGZONES         (DBG_ERROR | DBG_INIT)
#ifdef DEBUG
#define USBFNCTL_ZONES               (USBFNCTL_DEBUGZONES)
#else
#define USBFNCTL_ZONES              (USBFNCTL_RETAILZONES)
#endif

/////// Test Mode For USB HS Electrical Test ///////
// To use Test mode below "TEST_MODE_SUPPORT" shoude be defined TRUE.

#define TEST_MODE_SUPPORT        TRUE    // To use Test Mode for HS Electrical Compliance Test, Should be TRUE.

#if TEST_MODE_SUPPORT
#define USB_TEST_J                 0x01
#define USB_TEST_K                 0x02
#define USB_TEST_SE0_NAK         0x03
#define USB_TEST_PACKET         0x04
#define USB_TEST_FORCE_ENABLE     0x05

#define USB_FEATURE_TEST_MODE     2


#define TEST_PKT_SIZE 53
#define TEST_ARR_SIZE 27

WORD ahwTestPkt [TEST_ARR_SIZE] = {

    0x0000, 0x0000, 0x0000, 
    0xAA00, 0xAAAA, 0xAAAA, 0xAAAA,
    0xEEAA, 0xEEEE, 0xEEEE, 0xEEEE,
    0xFEEE,    0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x7FFF, 0xDFBF,
    0xF7EF, 0xFDFB, 0x7EFC, 0xDFBF,
    0xF7EF, 0xFDFB, 0x007E, 0x0000 

};
#endif

// Test Mode Bits in DCTL
#define TEST_MODE_MASK                (0x7 << 4)
#define TEST_MODE_DISABLED            (0x0 << 4)
#define TEST_J_MODE                    (0x1 << 4)
#define TEST_K_MODE                    (0x2 << 4)
#define TEST_SE0_NAK_MODE            (0x3 << 4)
#define TEST_PACKET_MODE            (0x4 << 4)
#define TEST_FORCE_ENABLED            (0x5 << 4)

#define TM_Disabled(parm)            do {    \
                    parm = ((parm) & ~(TEST_MODE_MASK) | (TEST_MODE_DISABLED));    \
                } while(0)
#define TM_J_Selected(parm)            do {    \
                    parm = ((parm) & ~(TEST_MODE_MASK) | (TEST_J_MODE));    \
                } while(0)
#define TM_K_Selected(parm)            do {    \
                    parm = ((parm) & ~(TEST_MODE_MASK) | (TEST_K_MODE));    \
                } while(0)
#define TM_SN_Selected(parm)        do {    \
                    parm = ((parm) & ~(TEST_MODE_MASK) | (TEST_SE0_NAK_MODE));    \
                } while(0)
#define TM_PKT_Selected(parm)        do {    \
                    parm = ((parm) & ~(TEST_MODE_MASK) | (TEST_PACKET_MODE));    \
                } while(0)
#define TM_Enabled(parm)            do {    \
                    parm = ((parm) & ~(TEST_MODE_MASK) | (TEST_FORCE_ENABLED));    \
                } while(0)
                

/////////////////////////////////////////////////////////////////////////

#define OTG_LINK_REG_SIZE            0x11000

#define GOTGCTL                        0x000        // OTG Control & Status
#define GOTGINT                        0x004        // OTG Interrupt
#define GAHBCFG                        0x008        // Core AHB Configuration
#define GUSBCFG                        0x00C        // Core USB Configuration
#define GRSTCTL                        0x010        // Core Reset
#define GINTSTS                        0x014        // Core Interrupt
#define GINTMSK                        0x018        // Core Interrupt Mask
#define GRXSTSR                        0x01C        // Receive Status Debug Read/Status Read
#define GRXSTSP                        0x020        // Receive Status Debug Pop/Status Pop
#define GRXFSIZ                        0x024        // Receive FIFO Size
#define GNPTXFSIZ                    0x028        // Non-Periodic Transmit FIFO Size
#define GNPTXSTS                    0x02C        // Non-Periodic Transmit FIFO/Queue Status
#define GPVNDCTL                    0x034        // PHY Vendor Control
#define GGPIO                        0x038        // General Purpose I/O
#define GUID                        0x03C        // User ID
#define GSNPSID                        0x040        // Synopsys ID
#define GHWCFG1                        0x044        // User HW Config1
#define GHWCFG2                        0x048        // User HW Config2
#define GHWCFG3                        0x04C        // User HW Config3
#define GHWCFG4                        0x050        // User HW Config4
                                           
#define HPTXFSIZ                    0x100        // Host Periodic Transmit FIFO Size
#define DPTXFSIZ1                    0x104        // Device Periodic Transmit FIFO-1 Size
#define DPTXFSIZ2                    0x108        // Device Periodic Transmit FIFO-2 Size
#define DPTXFSIZ3                    0x10C        // Device Periodic Transmit FIFO-3 Size
#define DPTXFSIZ4                    0x110        // Device Periodic Transmit FIFO-4 Size
#define DPTXFSIZ5                    0x114        // Device Periodic Transmit FIFO-5 Size
#define DPTXFSIZ6                    0x118        // Device Periodic Transmit FIFO-6 Size
#define DPTXFSIZ7                    0x11C        // Device Periodic Transmit FIFO-7 Size
#define DPTXFSIZ8                    0x120        // Device Periodic Transmit FIFO-8 Size
#define DPTXFSIZ9                    0x124        // Device Periodic Transmit FIFO-9 Size
#define DPTXFSIZ10                    0x128        // Device Periodic Transmit FIFO-10 Size
#define DPTXFSIZ11                    0x12C        // Device Periodic Transmit FIFO-11 Size
#define DPTXFSIZ12                    0x130        // Device Periodic Transmit FIFO-12 Size
#define DPTXFSIZ13                    0x134        // Device Periodic Transmit FIFO-13 Size
#define DPTXFSIZ14                    0x138        // Device Periodic Transmit FIFO-14 Size
#define DPTXFSIZ15                    0x13C        // Device Periodic Transmit FIFO-15 Size

//*********************************************************************
// Host Mode Registers
//*********************************************************************
// Host Global Registers

#define HCFG                        0x400        // Host Configuration
#define HFIR                        0x404        // Host Frame Interval
#define HFNUM                        0x408        // Host Frame Number/Frame Time Remaining
#define HPTXSTS                        0x410        // Host Periodic Transmit FIFO/Queue Status
#define HAINT                        0x414        // Host All Channels Interrupt
#define HAINTMSK                     0x418        // Host All Channels Interrupt Mask

// Host Port Control & Status Registers

#define HPRT                        0x440        // Host Port Control & Status

// Host Channel-Specific Registers #0

#define HCCHAR0                        0x500        // Host Channel-0 Characteristics
#define HCSPLT0                        0x504        // Host Channel-0 Split Control
#define HCINT0                        0x508        // Host Channel-0 Interrupt
#define HCINTMSK0                    0x50C        // Host Channel-0 Interrupt Mask
#define HCTSIZ0                        0x510        // Host Channel-0 Transfer Size
#define HCDMA0                        0x514        // Host Channel-0 DMA Address

// Host Channel-Specific Registers #1

#define HCCHAR1                        0x520       // Host Channel-1 Characteristics
#define HCSPLT1                        0x524       // Host Channel-1 Split Control
#define HCINT1                         0x528       // Host Channel-1 Interrupt
#define HCINTMSK1                     0x52C       // Host Channel-1 Interrupt Mask
#define HCTSIZ1                        0x530       // Host Channel-1 Transfer Size
#define HCDMA1                         0x534       // Host Channel-1 DMA Address

// Host Channel-Specific Registers #2

#define HCCHAR2                        0x540       // Host Channel-2 Characteristics
#define HCSPLT2                        0x544       // Host Channel-2 Split Control
#define HCINT2                         0x548       // Host Channel-2 Interrupt
#define HCINTMSK2                     0x54C       // Host Channel-2 Interrupt Mask
#define HCTSIZ2                        0x550       // Host Channel-2 Transfer Size
#define HCDMA2                         0x554       // Host Channel-2 DMA Address

// Host Channel-Specific Registers #3

#define HCCHAR3                        0x560       // Host Channel-3 Characteristics
#define HCSPLT3                        0x564       // Host Channel-3 Split Control
#define HCINT3                         0x568       // Host Channel-3 Interrupt
#define HCINTMSK3                     0x56C       // Host Channel-3 Interrupt Mask
#define HCTSIZ3                        0x570       // Host Channel-3 Transfer Size
#define HCDMA3                         0x574       // Host Channel-3 DMA Address

// Host Channel-Specific Registers #4

#define HCCHAR4                        0x580       // Host Channel-4 Characteristics
#define HCSPLT4                        0x584       // Host Channel-4 Split Control
#define HCINT4                         0x588       // Host Channel-4 Interrupt
#define HCINTMSK4                     0x58C       // Host Channel-4 Interrupt Mask
#define HCTSIZ4                        0x590       // Host Channel-4 Transfer Size
#define HCDMA4                         0x594       // Host Channel-4 DMA Address

// Host Channel-Specific Registers #5

#define HCCHAR5                        0x5A0       // Host Channel-5 Characteristics
#define HCSPLT5                        0x5A4       // Host Channel-5 Split Control
#define HCINT5                         0x5A8       // Host Channel-5 Interrupt
#define HCINTMSK5                     0x5AC       // Host Channel-5 Interrupt Mask
#define HCTSIZ5                        0x5B0       // Host Channel-5 Transfer Size
#define HCDMA5                         0x5B4       // Host Channel-5 DMA Address

// Host Channel-Specific Registers #6

#define HCCHAR6                        0x5C0       // Host Channel-6 Characteristics
#define HCSPLT6                        0x5C4       // Host Channel-6 Split Control
#define HCINT6                         0x5C8       // Host Channel-6 Interrupt
#define HCINTMSK6                     0x5CC       // Host Channel-6 Interrupt Mask
#define HCTSIZ6                        0x5D0       // Host Channel-6 Transfer Size
#define HCDMA6                         0x5D4       // Host Channel-6 DMA Address

// Host Channel-Specific Registers #7

#define HCCHAR7                        0x5E0       // Host Channel-7 Characteristics
#define HCSPLT7                        0x5E4       // Host Channel-7 Split Control
#define HCINT7                         0x5E8       // Host Channel-7 Interrupt
#define HCINTMSK7                     0x5EC       // Host Channel-7 Interrupt Mask
#define HCTSIZ7                        0x5F0       // Host Channel-7 Transfer Size
#define HCDMA7                         0x5F4       // Host Channel-7 DMA Address

// Host Channel-Specific Registers #8

#define HCCHAR8                        0x600       // Host Channel-8 Characteristics
#define HCSPLT8                        0x604       // Host Channel-8 Split Control
#define HCINT8                         0x608       // Host Channel-8 Interrupt
#define HCINTMSK8                     0x60C       // Host Channel-8 Interrupt Mask
#define HCTSIZ8                        0x610       // Host Channel-8 Transfer Size
#define HCDMA8                         0x614       // Host Channel-8 DMA Address

// Host Channel-Specific Registers #9

#define HCCHAR9                        0x620       // Host Channel-9 Characteristics
#define HCSPLT9                        0x624       // Host Channel-9 Split Control
#define HCINT9                         0x628       // Host Channel-9 Interrupt
#define HCINTMSK9                     0x62C       // Host Channel-9 Interrupt Mask
#define HCTSIZ9                        0x630       // Host Channel-9 Transfer Size
#define HCDMA9                         0x634       // Host Channel-9 DMA Address

// Host Channel-Specific Registers #10

#define HCCHAR10                    0x640       // Host Channel-10 Characteristics
#define HCSPLT10                    0x644       // Host Channel-10 Split Control
#define HCINT10                     0x648       // Host Channel-10 Interrupt
#define HCINTMSK10                     0x64C       // Host Channel-10 Interrupt Mask
#define HCTSIZ10                    0x650       // Host Channel-10 Transfer Size
#define HCDMA10                     0x654       // Host Channel-10 DMA Address

// Host Channel-Specific Registers #11

#define HCCHAR11                    0x660       // Host Channel-11 Characteristics
#define HCSPLT11                    0x664       // Host Channel-11 Split Control
#define HCINT11                     0x668       // Host Channel-11 Interrupt
#define HCINTMSK11                     0x66C       // Host Channel-11 Interrupt Mask
#define HCTSIZ11                    0x670       // Host Channel-11 Transfer Size
#define HCDMA11                     0x674       // Host Channel-11 DMA Address

// Host Channel-Specific Registers #12

#define HCCHAR12                    0x680       // Host Channel-12 Characteristics
#define HCSPLT12                    0x684       // Host Channel-12 Split Control
#define HCINT12                     0x688       // Host Channel-12 Interrupt
#define HCINTMSK12                     0x68C       // Host Channel-12 Interrupt Mask
#define HCTSIZ12                    0x690       // Host Channel-12 Transfer Size
#define HCDMA12                     0x694       // Host Channel-12 DMA Address

// Host Channel-Specific Registers #13

#define HCCHAR13                    0x6A0       // Host Channel-13 Characteristics
#define HCSPLT13                    0x6A4       // Host Channel-13 Split Control
#define HCINT13                     0x6A8       // Host Channel-13 Interrupt
#define HCINTMSK13                     0x6AC       // Host Channel-13 Interrupt Mask
#define HCTSIZ13                       0x6B0       // Host Channel-13 Transfer Size
#define HCDMA13                     0x6B4       // Host Channel-13 DMA Address

// Host Channel-Specific Registers #14

#define HCCHAR14                    0x6C0       // Host Channel-14 Characteristics
#define HCSPLT14                    0x6C4       // Host Channel-14 Split Control
#define HCINT14                     0x6C8       // Host Channel-14 Interrupt
#define HCINTMSK14                     0x6CC       // Host Channel-14 Interrupt Mask
#define HCTSIZ14                    0x6D0       // Host Channel-14 Transfer Size
#define HCDMA14                      0x6D4       // Host Channel-14 DMA Address

// Host Channel-Specific Registers #15

#define HCCHAR15                    0x680       // Host Channel-15 Characteristics
#define HCSPLT15                    0x684       // Host Channel-15 Split Control
#define HCINT15                     0x688       // Host Channel-15 Interrupt
#define HCINTMSK15                     0x68C       // Host Channel-15 Interrupt Mask
#define HCTSIZ15                    0x690       // Host Channel-15 Transfer Size
#define HCDMA15                     0x694       // Host Channel-15 DMA Address

//*********************************************************************
// Device Mode Registers
//*********************************************************************
// Device Global Registers

#define DCFG                         0x800        // Device Configuration
#define DCTL                         0x804        // Device Control
#define DSTS                         0x808        // Device Status
#define DIEPMSK                     0x810        // Device IN Endpoint Common Interrupt Mask
#define DOEPMSK                     0x814        // Device OUT Endpoint Common Interrupt Mask
#define DAINT                         0x818        // Device All Endpoints Interrupt
#define DAINTMSK                     0x81C        // Device All Endpoints Interrupt Mask
#define DTKNQR1                     0x820        // Device IN Token Sequence Learning Queue Read 1
#define DTKNQR2                     0x824        // Device IN Token Sequence Learning Queue Read 2
#define DVBUSDIS                     0x828        // Device VBUS Discharge Time
#define DVBUSPULSE                     0x82C        // Device VBUS Pulsing Time
#define DTKNQR3                     0x830        // Device IN Token Sequence Learning Queue Read 3
#define DTKNQR4                     0x834        // Device IN Token Sequence Learning Queue Read 4


#define DIEPCTL                     0x900        // Device IN Endpoint 0 Control
#define DOEPCTL                     0xB00        // Device OUT Endpoint 0 Control
#define DIEPINT                     0x908        // Device IN Endpoint 0 Interrupt
#define DOEPINT                     0xB08        // Device OUT Endpoint 0 Interrupt
#define DIEPTSIZ                     0x910        // Device IN Endpoint 0 Transfer Size
#define DOEPTSIZ                     0xB10        // Device OUT Endpoint 0 Transfer Size
#define DIEPDMA                     0x914        // Device IN Endpoint 0 DMA Address
#define DOEPDMA                     0xB14        // Device OUT Endpoint 0 DMA Address

// Device Logical Endpoints-Specific Registers

#define DIEPCTL0                     0x900        // Device IN Endpoint 0 Control
#define DOEPCTL0                     0xB00        // Device OUT Endpoint 0 Control
#define DIEPINT0                     0x908        // Device IN Endpoint 0 Interrupt
#define DOEPINT0                     0xB08        // Device OUT Endpoint 0 Interrupt
#define DIEPTSIZ0                     0x910        // Device IN Endpoint 0 Transfer Size
#define DOEPTSIZ0                     0xB10        // Device OUT Endpoint 0 Transfer Size
#define DIEPDMA0                     0x914        // Device IN Endpoint 0 DMA Address
#define DOEPDMA0                     0xB14        // Device OUT Endpoint 0 DMA Address

#define DIEPCTL1                     0x920        // Device IN Endpoint 1 Control
#define DOEPCTL1                     0xB20        // Device OUT Endpoint 1 Control
#define DIEPINT1                     0x928        // Device IN Endpoint 1 Interrupt
#define DOEPINT1                     0xB28        // Device OUT Endpoint 1 Interrupt
#define DIEPTSIZ1                     0x930        // Device IN Endpoint 1 Transfer Size
#define DOEPTSIZ1                     0xB30        // Device OUT Endpoint 1 Transfer Size
#define DIEPDMA1                     0x934        // Device IN Endpoint 1 DMA Address
#define DOEPDMA1                     0xB34        // Device OUT Endpoint 1 DMA Address

#define DIEPCTL2                     0x940        // Device IN Endpoint 2 Control
#define DOEPCTL2                     0xB40        // Device OUT Endpoint 2 Control
#define DIEPINT2                     0x948        // Device IN Endpoint 2 Interrupt
#define DOEPINT2                     0xB48        // Device OUT Endpoint 2 Interrupt
#define DIEPTSIZ2                     0x950        // Device IN Endpoint 2 Transfer Size
#define DOEPTSIZ2                     0xB50        // Device OUT Endpoint 2 Transfer Size
#define DIEPDMA2                     0x954        // Device IN Endpoint 2 DMA Address
#define DOEPDMA2                     0xB54        // Device OUT Endpoint 2 DMA Address
                               
#define DIEPCTL3                     0x960        // Device IN Endpoint 3 Control
#define DOEPCTL3                     0xB60        // Device OUT Endpoint 3 Control
#define DIEPINT3                     0x968        // Device IN Endpoint 3 Interrupt
#define DOEPINT3                     0xB68        // Device OUT Endpoint 3 Interrupt
#define DIEPTSIZ3                     0x970        // Device IN Endpoint 3 Transfer Size
#define DOEPTSIZ3                     0xB70        // Device OUT Endpoint 3 Transfer Size
#define DIEPDMA3                     0x974        // Device IN Endpoint 3 DMA Address
#define DOEPDMA3                     0xB74        // Device OUT Endpoint 3 DMA Address
                                
#define DIEPCTL4                     0x980        // Device IN Endpoint 4 Control
#define DOEPCTL4                     0xB80        // Device OUT Endpoint 4 Control
#define DIEPINT4                     0x988        // Device IN Endpoint 4 Interrupt
#define DOEPINT4                     0xB88        // Device OUT Endpoint 4 Interrupt
#define DIEPTSIZ4                     0x990        // Device IN Endpoint 4 Transfer Size
#define DOEPTSIZ4                     0xB90        // Device OUT Endpoint 4 Transfer Size
#define DIEPDMA4                     0x994        // Device IN Endpoint 4 DMA Address
#define DOEPDMA4                     0xB94        // Device OUT Endpoint 4 DMA Address
                                
#define DIEPCTL5                     0x9A0        // Device IN Endpoint 5 Control
#define DOEPCTL5                     0xBA0        // Device OUT Endpoint 5 Control
#define DIEPINT5                     0x9A8        // Device IN Endpoint 5 Interrupt
#define DOEPINT5                     0xBA8        // Device OUT Endpoint 5 Interrupt
#define DIEPTSIZ5                     0x9B0        // Device IN Endpoint 5 Transfer Size
#define DOEPTSIZ5                     0xBB0        // Device OUT Endpoint 5 Transfer Size
#define DIEPDMA5                     0x9B4        // Device IN Endpoint 5 DMA Address
#define DOEPDMA5                     0xBB4        // Device OUT Endpoint 5 DMA Address
                                
#define DIEPCTL6                     0x9C0        // Device IN Endpoint 6 Control
#define DOEPCTL6                     0xBC0        // Device OUT Endpoint 6 Control
#define DIEPINT6                     0x9C8        // Device IN Endpoint 6 Interrupt
#define DOEPINT6                     0xBC8        // Device OUT Endpoint 6 Interrupt
#define DIEPTSIZ6                     0x9D0        // Device IN Endpoint 6 Transfer Size
#define DOEPTSIZ6                     0xBD0        // Device OUT Endpoint 6 Transfer Size
#define DIEPDMA6                     0x9D4        // Device IN Endpoint 6 DMA Address
#define DOEPDMA6                     0xBD4        // Device OUT Endpoint 6 DMA Address
                                    
#define DIEPCTL7                     0x9E0        // Device IN Endpoint 7 Control
#define DOEPCTL7                     0xBE0        // Device OUT Endpoint 7 Control
#define DIEPINT7                     0x9E8        // Device IN Endpoint 7 Interrupt
#define DOEPINT7                     0xBE8        // Device OUT Endpoint 7 Interrupt
#define DIEPTSIZ7                     0x9F0        // Device IN Endpoint 7 Transfer Size
#define DOEPTSIZ7                     0xBF0        // Device OUT Endpoint 7 Transfer Size
#define DIEPDMA7                     0x9F4        // Device IN Endpoint 7 DMA Address
#define DOEPDMA7                     0xBF4        // Device OUT Endpoint 7 DMA Address
                                 
#define DIEPCTL8                     0xA00        // Device IN Endpoint 8 Control
#define DOEPCTL8                     0xC00        // Device OUT Endpoint 8 Control
#define DIEPINT8                     0xA08        // Device IN Endpoint 8 Interrupt
#define DOEPINT8                     0xC08        // Device OUT Endpoint 8 Interrupt
#define DIEPTSIZ8                     0xA10        // Device IN Endpoint 8 Transfer Size
#define DOEPTSIZ8                     0xC10        // Device OUT Endpoint 8 Transfer Size
#define DIEPDMA8                     0xA14        // Device IN Endpoint 8 DMA Address
#define DOEPDMA8                     0xC14        // Device OUT Endpoint 8 DMA Address
                                        
#define DIEPCTL9                     0xA20        // Device IN Endpoint 9 Control
#define DOEPCTL9                     0xC20        // Device OUT Endpoint 9 Control
#define DIEPINT9                     0xA28        // Device IN Endpoint 9 Interrupt
#define DOEPINT9                     0xC28        // Device OUT Endpoint 9 Interrupt
#define DIEPTSIZ9                     0xA30        // Device IN Endpoint 9 Transfer Size
#define DOEPTSIZ9                     0xC30        // Device OUT Endpoint 9 Transfer Size
#define DIEPDMA9                     0xA34        // Device IN Endpoint 9 DMA Address
#define DOEPDMA9                     0xC34        // Device OUT Endpoint 9 DMA Address
                                
#define DIEPCTL10                     0xA40        // Device IN Endpoint 10 Control
#define DOEPCTL10                     0xC40        // Device OUT Endpoint 10 Control
#define DIEPINT10                     0xA48        // Device IN Endpoint 10 Interrupt
#define DOEPINT10                     0xC48        // Device OUT Endpoint 10 Interrupt
#define DIEPTSIZ10                     0xA50        // Device IN Endpoint 10 Transfer Size
#define DOEPTSIZ10                     0xC50        // Device OUT Endpoint 10 Transfer Size
#define DIEPDMA10                     0xA54        // Device IN Endpoint 10 DMA Address
#define DOEPDMA10                     0xC54        // Device OUT Endpoint 10 DMA Address
                                 
#define DIEPCTL11                     0xA60        // Device IN Endpoint 11 Control
#define DOEPCTL11                     0xC60        // Device OUT Endpoint 11 Control
#define DIEPINT11                     0xA68        // Device IN Endpoint 11 Interrupt
#define DOEPINT11                     0xC68        // Device OUT Endpoint 11 Interrupt
#define DIEPTSIZ11                     0xA70        // Device IN Endpoint 11 Transfer Size
#define DOEPTSIZ11                     0xC70        // Device OUT Endpoint 11 Transfer Size
#define DIEPDMA11                     0xA74        // Device IN Endpoint 11 DMA Address
#define DOEPDMA11                     0xC74        // Device OUT Endpoint 11 DMA Address
                                        
#define DIEPCTL12                     0xA80        // Device IN Endpoint 12 Control
#define DOEPCTL12                     0xC80        // Device OUT Endpoint 12 Control
#define DIEPINT12                     0xA88        // Device IN Endpoint 12 Interrupt
#define DOEPINT12                     0xC88        // Device OUT Endpoint 12 Interrupt
#define DIEPTSIZ12                     0xA90        // Device IN Endpoint 12 Transfer Size
#define DOEPTSIZ12                     0xC90        // Device OUT Endpoint 12 Transfer Size
#define DIEPDMA12                     0xA94        // Device IN Endpoint 12 DMA Address
#define DOEPDMA12                     0xC94        // Device OUT Endpoint 12 DMA Address
                                 
#define DIEPCTL13                     0xAA0        // Device IN Endpoint 13 Control
#define DOEPCTL13                     0xCA0        // Device OUT Endpoint 13 Control
#define DIEPINT13                     0xAA8        // Device IN Endpoint 13 Interrupt
#define DOEPINT13                     0xCA8        // Device OUT Endpoint 13 Interrupt
#define DIEPTSIZ13                     0xAB0        // Device IN Endpoint 13 Transfer Size
#define DOEPTSIZ13                     0xCB0        // Device OUT Endpoint 13 Transfer Size
#define DIEPDMA13                     0xAB4        // Device IN Endpoint 13 DMA Address
#define DOEPDMA13                     0xCB4        // Device OUT Endpoint 13 DMA Address
                               
#define DIEPCTL14                     0xAC0        // Device IN Endpoint 14 Control
#define DOEPCTL14                     0xCC0        // Device OUT Endpoint 14 Control
#define DIEPINT14                     0xAC8        // Device IN Endpoint 14 Interrupt
#define DOEPINT14                     0xCC8        // Device OUT Endpoint 14 Interrupt
#define DIEPTSIZ14                     0xAD0        // Device IN Endpoint 14 Transfer Size
#define DOEPTSIZ14                     0xCD0        // Device OUT Endpoint 14 Transfer Size
#define DIEPDMA14                     0xAD4        // Device IN Endpoint 14 DMA Address
#define DOEPDMA14                     0xCD4        // Device OUT Endpoint 14 DMA Address
                                
#define DIEPCTL15                     0xAE0        // Device IN Endpoint 15 Control
#define DOEPCTL15                     0xCE0        // Device OUT Endpoint 15 Control
#define DIEPINT15                     0x9E8        // Device IN Endpoint 15 Interrupt
#define DOEPINT15                     0xCE8        // Device OUT Endpoint 15 Interrupt
#define DIEPTSIZ15                     0x9F0        // Device IN Endpoint 15 Transfer Size
#define DOEPTSIZ15                     0xCF0        // Device OUT Endpoint 15 Transfer Size
#define DIEPDMA15                     0x9F4        // Device IN Endpoint 15 DMA Address
#define DOEPDMA15                     0xCF4        // Device OUT Endpoint 15 DMA Address

//////////////////////////////////////////////////////////////////////////////////////////

#define EP0_FIFO                    0x1000
#define EP1_FIFO                    0x2000
#define EP2_FIFO                    0x3000
#define EP3_FIFO                    0x4000
#define EP4_FIFO                    0x5000
#define EP5_FIFO                    0x6000
#define EP6_FIFO                    0x7000
#define EP7_FIFO                    0x8000
#define EP8_FIFO                    0x9000
#define EP9_FIFO                    0xa000
#define EP10_FIFO                    0xb000
#define EP11_FIFO                    0xc000
#define EP12_FIFO                    0xd000
#define EP13_FIFO                    0xe000
#define EP14_FIFO                    0xf000
#define EP15_FIFO                    0x10000

#define BASE_REGISTER_OFFSET        0x0

// Can be used for Interrupt and Interrupt Enable Reg - common bit def
#define EP0_IN_INT                  (0x1<<0)
#define EP1_IN_INT                  (0x1<<1)
#define EP2_IN_INT                  (0x1<<2)
#define EP3_IN_INT                  (0x1<<3)
#define EP4_IN_INT                  (0x1<<4)
#define EP5_IN_INT                  (0x1<<5)
#define EP6_IN_INT                  (0x1<<6)
#define EP7_IN_INT                  (0x1<<7)
#define EP8_IN_INT                  (0x1<<8)
#define EP9_IN_INT                  (0x1<<9)
#define EP10_IN_INT                 (0x1<<10)
#define EP11_IN_INT                 (0x1<<11)
#define EP12_IN_INT                 (0x1<<12)
#define EP13_IN_INT                 (0x1<<13)
#define EP14_IN_INT                 (0x1<<14)
#define EP15_IN_INT                    (0x1<<15)
#define EP0_OUT_INT                   (0x1<<16)
#define EP1_OUT_INT                   (0x1<<17)
#define EP2_OUT_INT                   (0x1<<18)
#define EP3_OUT_INT                   (0x1<<19)
#define EP4_OUT_INT                   (0x1<<20)
#define EP5_OUT_INT                   (0x1<<21)
#define EP6_OUT_INT                   (0x1<<22)
#define EP7_OUT_INT                   (0x1<<23)
#define EP8_OUT_INT                   (0x1<<24)
#define EP9_OUT_INT                   (0x1<<25)
#define EP10_OUT_INT                 (0x1<<26)
#define EP11_OUT_INT                 (0x1<<27)
#define EP12_OUT_INT                 (0x1<<28)
#define EP13_OUT_INT                 (0x1<<29)
#define EP14_OUT_INT                 (0x1<<30)
#define EP15_OUT_INT                (0x1<<31)

// GOTGINT 
#define SesEndDet                   (0x1<<2)

// GUSBCFG
#define EXTERNAL_48MCLK                (0x1<<15)    // PHY Low-Power Clock Select 0:480Mhz Internall PLL clock, 1:48Mhz External clock
#define NP_TXFIFO_REWIND_EN         (0x1<<14)
#define TURNAROUND_TIME                (0x3<<10)
#define HNP_EN                        (0x1<<9)
#define SRP_EN                        (0x1<<8)
#define PHYIF_16BIT                    (0x1<<3)    // PHY Interface 0:8bit, 1:16bit
#define HS_FS_TIMEOUT                (0)         // [2:0] HS/FS Timeout Calibration

// GRSTCTL
#define AHBIDLE                        (0x1<<31)    // AHB Master IDLE
#define TXFFLSH                        (0x1<<5)    // TxFIFO Flush
#define RXFFLSH                        (0x1<<4)    // RxFIFO Flush
#define INTKNQFLSH                    (0x1<<3)    // IN Token Sequence Learning Queue Flush
#define FRMCNTRRST                    (0x1<<2)    // Host Frame Counter Reset
#define HSFTRST                        (0x1<<1)    // Hclk Soft Reset
#define CSFTRST                        (0x1<<0)    // Core Soft Reset

// GINTSTS core interrupt register
#define INT_RESUME                  (0x1<<31)
#define INT_SSREQ                   (0x1<<30)
#define INT_DISCONN                 (0x1<<29)
#define INT_OUT_EP                  (0x1<<19)
#define INT_IN_EP                   (0x1<<18)
#define INT_EPMIS                    (0x1<<17)
#define INT_SDE                     (0x1<<13)
#define INT_RESET                   (0x1<<12)
#define INT_SUSPEND                 (0x1<<11)
#define INT_TX_FIFO_EMPTY           (0x1<<5)
#define INT_RX_FIFO_NOT_EMPTY       (0x1<<4)
#define INT_SOF                     (0x1<<3)
#define INT_OTG                        (0x1<<2)

// GRXSTSP & GRXSTSR
#define PACKET_STATUS_MSK            (0xf<<17)
#define PACKET_STATUS_IDX            (17)
#define PKTSTS_GOUTNAK                (0x1)
#define BYTE_COUNT_MSK                (0x7ff0)
#define BYTE_COUNT_IDX                (4)
#define EPNUM_MSK                    (0xf)

// GAHBCFG
#define NPTXFEMPLVL_COMPLETE_EMPTY    (1<<7)
#define MODE_DMA                    (1<<5)
#define MODE_SLAVE                  (0<<5)
#define BURST_SINGLE                (0<<1)
#define BURST_INCR                  (1<<1)
#define BURST_INCR4                 (3<<1)
#define BURST_INCR8                 (5<<1)
#define BURST_INCR16                (7<<1)
#define GBL_INT_MASK                (0<<0)
#define GBL_INT_UNMASK              (1<<0)

// GOTGCTL
#define B_SESSION_VALID                (0x1<<19)
#define A_SESSION_VALID                (0x1<<18)
#define SESSION_REQUEST                (0x1<<1)    // 0:No session request, 1:Session request

// GRX STATUS
#define PKTSTS                        (0xF<<17)
#define GLOBAL_OUT_NAK                (0x1<<17)
#define OUT_PKT_RECEIVED            (0x2<<17)
#define OUT_TRF_COMPLETED            (0x3<<17)
#define SETUP_TRANS_COMPLETED        (0x4<<17)
#define SETUP_PKT_RECEIVED            (0x6<<17)

#define SETUPPHASEDONE                (0x1<<3)    //Setup Phase Done Interrupt
#define XFERCOPMPL                    (0x1<<0)     //Transfer Complete Interrupt

// GRXFSIZ
#define RXFIFO_DEPTH                (0x800)

// GNPTXFSIZ
#define NPTXFIFO_DEPTH                (0x800)
#define NPTXFIFO_DEPTH_IDX            (16)        // [31:16] Non-Periodic TxFIFO Depth

// DCFG
#define IN_EP_MISS_CNT_IDX            (18)        // [22:18] IN Endpoint Mismatch Count
#define DEVICE_ADDRESS_MSK            (0x7F<<4)    // [10:4] Device Address
#define DEVICE_SPEED_HIGH            (0x0<<0)    // [1:0] High speed (USB 2.0 PHY clock is 30 MHz or 60 MHz)
#define DEVICE_SPEED_FULL            (0x1<<0)    // [1:0] Full speed (USB 2.0 PHY clock is 30 MHz or 60 MHz) 

// DIEPCTL/DOEPCTL device control IN/OUT endpoint control register
#define EP_ENABLE                    (0x1<<31)
#define EP_DISABLE                    (0x1<<30)
#define SET_D0_PID                    (0x1<<28)
#define SET_NAK                        (0x1<<27)
#define CLEAR_NAK                    (0x1<<26)
#define STALL                        (0x1<<21)
#define EPTYPE                        (0x3<<18)

#define SET_TYPE_CONTROL            (0x0<<18)
#define SET_TYPE_ISO                (0x1<<18)
#define SET_TYPE_BULK                (0x2<<18)
#define SET_TYPE_INTERRUPT             (0x3<<18)

#define USB_ACT_EP                    (0x1<<15)
#define NEXT_EP_IDX                    (11)
#define EP0_MAX_PK_SIZ                (0x0<<0)    // [1:0] 0:64byte, 1:32byte, 2:16byte, 3:8byte

// DIEPINT
#define IN_TKN_RECEIVED                (0x1<<4)
#define TIMEOUT_CONDITION            (0x1<<3)
#define XFER_COMPLETE                (0x1<<0)

// DOEPINT
#define SETUP_PHASE_DONE            (0x1<<3)


// DOEPTSIZ , DIEPTSIZ
#define SETUP_PKT_CNT_IDX            (29)
#define MULTI_CNT_IDX                (29)
#define PACKET_COUTNT_IDX            (19)        

// DCTL
#define CLEAR_GOUTNAK                (0x1<<10)
#define SET_GOUTNAK                    (0x1<<9)
#define CLEAR_GNPINNAK                (0x1<<8)
#define SET_GNINNAK                    (0x1<<7)
#define SOFT_DISCONNECT                (0x1<<1)
#define RMTWKUPSIG                    (0x1<<0)

// DSTS
#define ENUM_SPEED_MSK                (0x6)

// SYSCON - HCLK GATE
#define OTG_HCLK_EN                    (1<<20)

// SYSCON - OTHERS
#define USB_SIG_MASK                (1<<16)

#define IN_EP                        0
#define OUT_EP                        1

/////////////////////////////////////////////
#define IN_TRANSFER  1
#define OUT_TRANSFER 2

enum EP0_STATE {
    EP0_STATE_IDLE = 0,
    EP0_STATE_IN_DATA_PHASE,
    EP0_STATE_OUT_DATA_PHASE
};

typedef enum
{
    USB_HIGH, USB_FULL, USB_LOW
} USB_SPEED;

typedef struct EP_STATUS {
    DWORD                       dwEndpointNumber;
    DWORD                       dwDirectionAssigned;
    DWORD                       dwPacketSizeAssigned;
    BOOL                        bInitialized;
    DWORD                       dwEndpointType;
    PSTransfer                  pTransfer;
    CRITICAL_SECTION      cs;
} *PEP_STATUS;

typedef enum
{
    USB_RNDIS = 0, 
    USB_SERIAL, 
    USB_MSF
};

#endif //_S3C6410OTGD_H_


