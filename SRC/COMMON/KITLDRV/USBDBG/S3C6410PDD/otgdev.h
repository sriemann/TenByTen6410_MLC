//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef _OTGDEV_H_
#define _OTGDEV_H_

#ifdef __cplusplus
extern "C" {
#endif 


#define USBOTG_LINK_BASE    (0xB1800000) //(0x7C000000) -> 0x91800000
#define USBOTG_PHY_BASE     (0xB1900000) //(0x7C100000) -> 0x91900000

#define SYSCON_BASE (0xB2A0F000)
#define SYSCON_OTHERS_REG   (SYSCON_BASE + 0x900)


//==========================
// OTG LINK CORE REGISTERS
//==========================
enum USBOTG_REGS
{
    //==============================================================================================
    // Core Global Registers
    GOTGCTL     = (USBOTG_LINK_BASE + 0x000),       // OTG Control & Status
    GOTGINT     = (USBOTG_LINK_BASE + 0x004),       // OTG Interrupt
    GAHBCFG     = (USBOTG_LINK_BASE + 0x008),       // Core AHB Configuration
    GUSBCFG     = (USBOTG_LINK_BASE + 0x00C),       // Core USB Configuration
    GRSTCTL     = (USBOTG_LINK_BASE + 0x010),       // Core Reset
    GINTSTS     = (USBOTG_LINK_BASE + 0x014),       // Core Interrupt
    GINTMSK     = (USBOTG_LINK_BASE + 0x018),       // Core Interrupt Mask
    GRXSTSR     = (USBOTG_LINK_BASE + 0x01C),       // Receive Status Debug Read/Status Read
    GRXSTSP     = (USBOTG_LINK_BASE + 0x020),       // Receive Status Debug Pop/Status Pop
    GRXFSIZ     = (USBOTG_LINK_BASE + 0x024),       // Receive FIFO Size
    GNPTXFSIZ   = (USBOTG_LINK_BASE + 0x028),       // Non-Periodic Transmit FIFO Size
    GNPTXSTS    = (USBOTG_LINK_BASE + 0x02C),       // Non-Periodic Transmit FIFO/Queue Status

    HPTXFSIZ    = (USBOTG_LINK_BASE + 0x100),       // Host Periodic Transmit FIFO Size
    DPTXFSIZ1   = (USBOTG_LINK_BASE + 0x104),       // Device Periodic Transmit FIFO-1 Size
    DPTXFSIZ2   = (USBOTG_LINK_BASE + 0x108),       // Device Periodic Transmit FIFO-2 Size
    DPTXFSIZ3   = (USBOTG_LINK_BASE + 0x10C),       // Device Periodic Transmit FIFO-3 Size
    DPTXFSIZ4   = (USBOTG_LINK_BASE + 0x110),       // Device Periodic Transmit FIFO-4 Size
    DPTXFSIZ5   = (USBOTG_LINK_BASE + 0x114),       // Device Periodic Transmit FIFO-5 Size
    DPTXFSIZ6   = (USBOTG_LINK_BASE + 0x118),       // Device Periodic Transmit FIFO-6 Size
    DPTXFSIZ7   = (USBOTG_LINK_BASE + 0x11C),       // Device Periodic Transmit FIFO-7 Size
    DPTXFSIZ8   = (USBOTG_LINK_BASE + 0x120),       // Device Periodic Transmit FIFO-8 Size
    DPTXFSIZ9   = (USBOTG_LINK_BASE + 0x124),       // Device Periodic Transmit FIFO-9 Size
    DPTXFSIZ10  = (USBOTG_LINK_BASE + 0x128),       // Device Periodic Transmit FIFO-10 Size
    DPTXFSIZ11  = (USBOTG_LINK_BASE + 0x12C),       // Device Periodic Transmit FIFO-11 Size
    DPTXFSIZ12  = (USBOTG_LINK_BASE + 0x130),       // Device Periodic Transmit FIFO-12 Size
    DPTXFSIZ13  = (USBOTG_LINK_BASE + 0x134),       // Device Periodic Transmit FIFO-13 Size
    DPTXFSIZ14  = (USBOTG_LINK_BASE + 0x138),       // Device Periodic Transmit FIFO-14 Size
    DPTXFSIZ15  = (USBOTG_LINK_BASE + 0x13C),       // Device Periodic Transmit FIFO-15 Size

    //==============================================================================================
    // Host Mode Registers
    //------------------------------------------------
    // Host Global Registers
    HCFG        = (USBOTG_LINK_BASE + 0x400),       // Host Configuration
    HFIR        = (USBOTG_LINK_BASE + 0x404),       // Host Frame Interval
    HFNUM       = (USBOTG_LINK_BASE + 0x408),       // Host Frame Number/Frame Time Remaining
    HPTXSTS     = (USBOTG_LINK_BASE + 0x410),       // Host Periodic Transmit FIFO/Queue Status
    HAINT       = (USBOTG_LINK_BASE + 0x414),       // Host All Channels Interrupt
    HAINTMSK    = (USBOTG_LINK_BASE + 0x418),       // Host All Channels Interrupt Mask

    //------------------------------------------------
    // Host Port Control & Status Registers
    HPRT        = (USBOTG_LINK_BASE + 0x440),       // Host Port Control & Status

    //------------------------------------------------
    // Host Channel-Specific Registers
    HCCHAR0     = (USBOTG_LINK_BASE + 0x500),       // Host Channel-0 Characteristics
    HCSPLT0     = (USBOTG_LINK_BASE + 0x504),       // Host Channel-0 Split Control
    HCINT0      = (USBOTG_LINK_BASE + 0x508),       // Host Channel-0 Interrupt
    HCINTMSK0   = (USBOTG_LINK_BASE + 0x50C),       // Host Channel-0 Interrupt Mask
    HCTSIZ0     = (USBOTG_LINK_BASE + 0x510),       // Host Channel-0 Transfer Size
    HCDMA0      = (USBOTG_LINK_BASE + 0x514),       // Host Channel-0 DMA Address


    //==============================================================================================
    // Device Mode Registers
    //------------------------------------------------
    // Device Global Registers
    DCFG        = (USBOTG_LINK_BASE + 0x800),       // Device Configuration
    DCTL        = (USBOTG_LINK_BASE + 0x804),       // Device Control
    DSTS        = (USBOTG_LINK_BASE + 0x808),       // Device Status
    DIEPMSK     = (USBOTG_LINK_BASE + 0x810),       // Device IN Endpoint Common Interrupt Mask
    DOEPMSK     = (USBOTG_LINK_BASE + 0x814),       // Device OUT Endpoint Common Interrupt Mask
    DAINT       = (USBOTG_LINK_BASE + 0x818),       // Device All Endpoints Interrupt
    DAINTMSK    = (USBOTG_LINK_BASE + 0x81C),       // Device All Endpoints Interrupt Mask
    DTKNQR1     = (USBOTG_LINK_BASE + 0x820),       // Device IN Token Sequence Learning Queue Read 1
    DTKNQR2     = (USBOTG_LINK_BASE + 0x824),       // Device IN Token Sequence Learning Queue Read 2
    DVBUSDIS    = (USBOTG_LINK_BASE + 0x828),       // Device VBUS Discharge Time
    DVBUSPULSE  = (USBOTG_LINK_BASE + 0x82C),       // Device VBUS Pulsing Time
    DTKNQR3     = (USBOTG_LINK_BASE + 0x830),       // Device IN Token Sequence Learning Queue Read 3
    DTKNQR4     = (USBOTG_LINK_BASE + 0x834),       // Device IN Token Sequence Learning Queue Read 4

    //------------------------------------------------
    // Device Logical IN Endpoint-Specific Registers
    DIEPCTL0    = (USBOTG_LINK_BASE + 0x900),       // Device IN Endpoint 0 Control
    DIEPINT0    = (USBOTG_LINK_BASE + 0x908),       // Device IN Endpoint 0 Interrupt
    DIEPTSIZ0   = (USBOTG_LINK_BASE + 0x910),       // Device IN Endpoint 0 Transfer Size
    DIEPDMA0    = (USBOTG_LINK_BASE + 0x914),       // Device IN Endpoint 0 DMA Address
    //------------------------------------------------
    // Device Logical OUT Endpoint-Specific Registers
    DOEPCTL0    = (USBOTG_LINK_BASE + 0xB00),       // Device OUT Endpoint 0 Control
    DOEPINT0    = (USBOTG_LINK_BASE + 0xB08),       // Device OUT Endpoint 0 Interrupt
    DOEPTSIZ0   = (USBOTG_LINK_BASE + 0xB10),       // Device OUT Endpoint 0 Transfer Size
    DOEPDMA0    = (USBOTG_LINK_BASE + 0xB14),       // Device OUT Endpoint 0 DMA Address

    //------------------------------------------------
    PCGCCTL     = (USBOTG_LINK_BASE + 0x0E00),
    
    // Endpoint FIFO address
    EP0_FIFO    = (USBOTG_LINK_BASE + 0x1000)
 };



//==========================
// OTG PHY CORE REGISTERS
//==========================
enum OTGPHYC_REG
{
    PHYPWR      = (USBOTG_PHY_BASE+0x00),
    PHYCTRL     = (USBOTG_PHY_BASE+0x04),
    RSTCON      = (USBOTG_PHY_BASE+0x08)
};


//=====================================================================
//definitions related to CSR setting

// GOTGCTL
#define B_SESSION_VALID             (0x1<<19)
#define A_SESSION_VALID             (0x1<<18)

// GAHBCFG
#define PTXFE_HALF                  (0<<8)
#define PTXFE_ZERO                  (1<<8)
#define NPTXFE_HALF                 (0<<7)
#define NPTXFE_ZERO                 (1<<7)
#define MODE_SLAVE                  (0<<5)
#define MODE_DMA                    (1<<5)
#define BURST_SINGLE                (0<<1)
#define BURST_INCR                  (1<<1)
#define BURST_INCR4                 (3<<1)
#define BURST_INCR8                 (5<<1)
#define BURST_INCR16                (7<<1)
#define GBL_INT_UNMASK              (1<<0)
#define GBL_INT_MASK                (0<<0)

// GRSTCTL
#define AHB_MASTER_IDLE         (0x1<<31)
#define CORE_SOFT_RESET             (0x1<<0)

// GINTSTS/GINTMSK core interrupt register
#define INT_RESUME                          (0x1<<31)
#define INT_DISCONN                 (0x1<<29)
#define INT_CONN_ID_STS_CNG         (0x1<<28)
#define INT_OUT_EP                  (0x1<<19)
#define INT_IN_EP                   (0x1<<18)
#define INT_ENUMDONE                (0x1<<13)
#define INT_RESET                   (0x1<<12)
#define INT_SUSPEND                 (0x1<<11)
#define INT_TX_FIFO_EMPTY           (0x1<<5)
#define INT_RX_FIFO_NOT_EMPTY       (0x1<<4)
#define INT_SOF                     (0x1<<3)
#define INT_DEV_MODE                (0x0<<0)
#define INT_HOST_MODE               (0x1<<1)

// GRXSTSP STATUS
#define PKT_STATUS_MASK             (0xF<<17)
#define OUT_PKT_RECEIVED            (0x2<<17)
#define OUT_XFR_COMPLETED          (0x3<<17)
#define SETUP_PKT_RECEIVED          (0x6<<17)
#define SETUP_XFR_COMPLETED        (0x4<<17)

// DCTL device control register
#define NORMAL_OPERATION            (0x1<<0)
#define SOFT_DISCONNECT             (0x1<<1)

// DAINT device all endpoint interrupt register
#define INT_IN_EP0                  (0x1<<0)
#define INT_IN_EP1                  (0x1<<1)
#define INT_IN_EP3                  (0x1<<3)
#define INT_OUT_EP0                 (0x1<<16)
#define INT_OUT_EP2                 (0x1<<18)
#define INT_OUT_EP4                 (0x1<<20)

// DIEPCTL0/DOEPCTL0 device control IN/OUT endpoint 0 control register
#define DEPCTL_EPENA                (0x1<<31)
#define DEPCTL_EPDIS                (0x1<<30)
#define DEPCTL_SNAK                 (0x1<<27)
#define DEPCTL_CNAK                 (0x1<<26)
#define DEPCTL_CTRL_TYPE            (EP_TYPE_CONTROL<<18)
#define DEPCTL_ISO_TYPE             (EP_TYPE_ISOCHRONOUS<<18)
#define DEPCTL_BULK_TYPE            (EP_TYPE_BULK<<18)
#define DEPCTL_INTR_TYPE            (EP_TYPE_INTERRUPT<<18)
#define DEPCTL_USBACTEP             (0x1<<15)
#define DEPCTL0_MPS_64              (0x0<<0)
#define DEPCTL0_MPS_32              (0x1<<0)
#define DEPCTL0_MPS_16              (0x2<<0)
#define DEPCTL0_MPS_8               (0x3<<0)

// DIEPCTLn/DOEPCTLn device control IN/OUT endpoint n control register

// DIEPMSK/DOEPMSK device IN/OUT endpoint common interrupt mask register
// DIEPINTn/DOEPINTn device IN/OUT endpoint interrupt register
#define BACK2BACK_SETUP_RECEIVED        (0x1<<6)
#define INTKN_TXFEMP                    (0x1<<4)
#define NON_ISO_IN_EP_TIMEOUT           (0x1<<3)
#define CTRL_OUT_EP_SETUP_PHASE_DONE    (0x1<<3)
#define AHB_ERROR                       (0x1<<2)
#define TRANSFER_DONE                   (0x1<<0)


#define OTGINTMASK (    INT_RESUME |\
                        INT_ENUMDONE |\
                        INT_RESET |\
                        INT_SUSPEND |\
                        INT_RX_FIFO_NOT_EMPTY |\
                        INT_DISCONN |\
                        INT_CONN_ID_STS_CNG) 



//=====================================================================================
//
#define FULL_SPEED_CONTROL_PKT_SIZE     64
#define FULL_SPEED_BULK_PKT_SIZE        64

#define HIGH_SPEED_CONTROL_PKT_SIZE     64
#define HIGH_SPEED_BULK_PKT_SIZE        512

#define RX_FIFO_SIZE                    512
#define NPTX_FIFO_START_ADDR            RX_FIFO_SIZE
#define NPTX_FIFO_SIZE                  512
#define PTX_FIFO_SIZE                   512


typedef enum
{
    USB_CPU, USB_DMA
} USB_OPMODE;

typedef enum
{
    USB_HIGH, USB_FULL, USB_LOW
} USB_SPEED;

typedef enum
{
    EP_TYPE_CONTROL, EP_TYPE_ISOCHRONOUS, EP_TYPE_BULK, EP_TYPE_INTERRUPT
}EP_TYPE;


typedef struct
{
    USB_SPEED m_eSpeed;
    UINT32  m_uControlEPMaxPktSize;
    UINT32  m_uBulkInEPMaxPktSize;
    UINT32  m_uBulkOutEPMaxPktSize;
    UINT32  m_uIsUsbOtgSetConfiguration;
} OTGDEV;

typedef struct
{
    WORD EndPtNum;
    BYTE EndPtAddress;
    BYTE EndPtAttributes;
} ENDPT_INFO;

#define MAX_ENDPTS  15

extern OTGDEV oOtgDev;
extern DWORD g_NumEPsUsed;
extern ENDPT_INFO EndPointsInfo[MAX_ENDPTS];
extern USBDBG_DEVICE_DESCRIPTOR *g_pusbDeviceDescriptor;

extern DWORD EP_FIFO[MAX_ENDPTS];

extern DWORD DIEPCTL[MAX_ENDPTS];
extern DWORD DIEPINT[MAX_ENDPTS];
extern DWORD DIEPTSIZ[MAX_ENDPTS];
extern DWORD DIEPDMA[MAX_ENDPTS];

extern DWORD DOEPCTL[MAX_ENDPTS];
extern DWORD DOEPINT[MAX_ENDPTS];
extern DWORD DOEPTSIZ[MAX_ENDPTS];
extern DWORD DOEPDMA[MAX_ENDPTS];


//=====================================================================================
// prototypes of functions


void OTGDevice_Init();
void OTGDevice_InitPhyCon(void);
void OTGDevice_SoftResetCore(void);
void OTGDevice_InitCore(void);
void OTGDevice_CheckCurrentMode(UINT8 *pucMode);
void OTGDevice_SetSoftDisconnect(void);
void OTGDevice_ClearSoftDisconnect(void);
void OTGDevice_InitDevice(void);
void OTGDevice_SetAddress(USHORT address);
BOOL OTGDevice_InitEndPts(USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc);
BOOL OTGDevice_InitNonControlEndPts();
void OTGDevice_SetOutEpXferSize(UINT32 epNum, UINT32 uPktCnt);
void OTGDevice_DeInit();
void OTGDevice_HandleReset();
void OTGDevice_HandleEnumDone();
void OTGDevice_StallHelper(int count);


#ifdef __cplusplus
}
#endif


#endif  // _OTGDEV_H_

