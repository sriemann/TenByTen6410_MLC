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

#ifndef _KITL_USB_SERIAL_H_
#define _KITL_USB_SERIAL_H_

#if 0
#define DBGUSB(x) EdbgOutputDebugString(x)
#else
#define DBGUSB(x) 
#endif

#define DMABUFFER            (EBOOT_USB_BUFFER_CA_START)            // Actually this buffer is not for DMA, Just read/write with ARM
#define USBOTG_LINK_BASE    (0xB1800000) //(0x7C000000) -> 0x91800000
#define USBOTG_PHY_BASE        (0xB1900000) //(0x7C100000) -> 0x91900000

#define pISR    (*(volatile unsigned *)(DRAM_BASE_PA_START+0x18))        // Virtual Address 0x0 is mapped to 0x50000000, ISR Address is VA 0x18

static void delayLoop(int count);

#define Outp32(addr, data) (*(volatile UINT32 *)(addr) = (data))
#define Outp16(addr, data) (*(volatile UINT16 *)(addr) = (data))
#define Outp8(addr, data)  (*(volatile UINT8 *)(addr) = (data))
#define Inp32(addr, data) (data = (*(volatile UINT32 *)(addr)))
#define Inp16(addr, data) (data = (*(volatile UINT16 *)(addr)))
#define Inp8(addr, data)  (data = (*(volatile UINT16 *)(addr)))
#define Input32(addr) (*(volatile UINT32 *)(addr))

// start // from OTG_MON/def.h


typedef BOOL        bool;
#define false     FALSE
#define true    TRUE
// end // from OTG_MON/def.h


// start // from OTG_MON/otg_dev.h

//=====================================================================================
//
#define CONTROL_EP                        0
#define BULK_IN_EP                        1
#define BULK_OUT_EP                        2

#define FULL_SPEED_CONTROL_PKT_SIZE    8
#define FULL_SPEED_BULK_PKT_SIZE        64

#define HIGH_SPEED_CONTROL_PKT_SIZE    64
#define HIGH_SPEED_BULK_PKT_SIZE        512

#define RX_FIFO_SIZE                    0x800
#define NPTX_FIFO_START_ADDR            RX_FIFO_SIZE
#define NPTX_FIFO_SIZE                    0x800
#define PTX_FIFO_SIZE                    0x800

// string descriptor
#define LANGID_US_L                     (0x09)
#define LANGID_US_H                     (0x04)



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
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bcdUSBL;
    UINT8 bcdUSBH;
    UINT8 bDeviceClass;
    UINT8 bDeviceSubClass;
    UINT8 bDeviceProtocol;
    UINT8 bMaxPacketSize0;
    UINT8 idVendorL;
    UINT8 idVendorH;
    UINT8 idProductL;
    UINT8 idProductH;
    UINT8 bcdDeviceL;
    UINT8 bcdDeviceH;
    UINT8 iManufacturer;
    UINT8 iProduct;
    UINT8 iSerialNumber;
    UINT8 bNumConfigurations;
} USB_DEVICE_DESCRIPTOR;

typedef struct
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 wTotalLengthL;
    UINT8 wTotalLengthH;
    UINT8 bNumInterfaces;
    UINT8 bConfigurationValue;
    UINT8 iConfiguration;
    UINT8 bmAttributes;
    UINT8 maxPower;
} USB_CONFIGURATION_DESCRIPTOR;

typedef struct
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bInterfaceNumber;
    UINT8 bAlternateSetting;
    UINT8 bNumEndpoints;
    UINT8 bInterfaceClass;
    UINT8 bInterfaceSubClass;
    UINT8 bInterfaceProtocol;
    UINT8 iInterface;
} USB_INTERFACE_DESCRIPTOR;

typedef struct
{
    UINT8 bLength;
    UINT8 bDescriptorType;
    UINT8 bEndpointAddress;
    UINT8 bmAttributes;
    UINT8 wMaxPacketSizeL;
    UINT8 wMaxPacketSizeH;
    UINT8 bInterval;
} USB_ENDPOINT_DESCRIPTOR;

typedef struct
{
    USB_DEVICE_DESCRIPTOR oDescDevice;
    USB_CONFIGURATION_DESCRIPTOR oDescConfig;
    USB_INTERFACE_DESCRIPTOR oDescInterface;
    USB_ENDPOINT_DESCRIPTOR oDescEndpt1;
    USB_ENDPOINT_DESCRIPTOR oDescEndpt2;
} USB_DESCRIPTORS;

typedef struct
{
    UINT8 bmRequestType;  
    UINT8 bRequest;       
    UINT8 wValue_L;       
    UINT8 wValue_H;       
    UINT8 wIndex_L;       
    UINT8 wIndex_H;       
    UINT8 wLength_L;      
    UINT8 wLength_H;      
} DEVICE_REQUEST;

typedef struct
{
    USB_DESCRIPTORS m_oDesc;
    DEVICE_REQUEST m_oDeviceRequest;

    UINT32  m_uEp0State;
    UINT32  m_uEp0SubState;
    USB_OPMODE m_eOpMode;
    USB_SPEED m_eSpeed;
    UINT32  m_uControlEPMaxPktSize;
    UINT32  m_uBulkInEPMaxPktSize;
    UINT32  m_uBulkOutEPMaxPktSize;
    UINT32  m_uDownloadAddress;
    UINT32  m_uDownloadFileSize;
    UINT32  m_uUploadAddr;
    UINT32  m_uUploadSize;
    UINT8*  m_pDownPt;
    UINT8*  m_pUpPt;
    UINT32  m_uIsUsbOtgSetConfiguration;
} OTGDEV;


typedef struct
{
    UINT8 ConfigurationValue;
} USB_CONFIGURATION_SET;

typedef struct
{
    UINT8 Device;
    UINT8 Interface;
    UINT8 Endpoint0;
    UINT8 Endpoint1;
    UINT8 Endpoint2;
} USB_GET_STATUS;

typedef struct
{
    UINT8 AlternateSetting;
} USB_INTERFACE_GET;



//=====================================================================================
// prototypes of API functions
void OTGDEV_InitOtg(USB_SPEED eSpeed);

//void OTGDEV_HandleEvent(void);
UINT16 OTGDEV_HandleEvent(UINT8 *pch, UINT16 length);
void OTGDEV_HandleEvent_EP0(void);
void OTGDEV_TransferEp0(void);
UINT16 OTGDEV_HandleEvent_BulkIn(UINT8 *pch, UINT32 fifoCntByte);
void OTGDEV_HandleEvent_BulkOut(UINT8 *pch, UINT32 fifoCntByte);


//-------------------------------------------------------------------------------------
// prototypes of sub functions
void OTGDEV_InitPhyCon(void);
void OTGDEV_SoftResetCore(void);
void OTGDEV_InitCore(void);
void OTGDEV_CheckCurrentMode(UINT8 *pucMode);
void OTGDEV_SetSoftDisconnect(void);
void OTGDEV_ClearSoftDisconnect(void);
void OTGDEV_InitDevice(void);


void OTGDEV_SetAllOutEpNak(void);
void OTGDEV_ClearAllOutEpNak(void);
void OTGDEV_SetMaxPktSizes(USB_SPEED eSpeed);
void OTGDEV_SetEndpoint(void);
void OTGDEV_SetDescriptorTable(void);

void OTGDEV_CheckEnumeratedSpeed(USB_SPEED *eSpeed);

void OTGDEV_SetInEpXferSize(EP_TYPE eType, UINT32 uPktCnt, UINT32 uXferSize);
void OTGDEV_SetOutEpXferSize(EP_TYPE eType, UINT32 uPktCnt, UINT32 uXferSize);

void OTGDEV_WrPktEp0(UINT8 *buf, int num);
void OTGDEV_PrintEp0Pkt(UINT8 *pt, UINT8 count);
void OTGDEV_WrPktBulkInEp(UINT8 *buf, int num);
void OTGDEV_RdPktBulkOutEp(UINT8 *buf, int num);

//=====================================================================================
// prototypes of API functions
BOOL OTGDEV_IsUsbOtgSetConfiguration(void);
void OTGDEV_SetOpMode(USB_OPMODE eMode);
void OTGDEV_VerifyChecksum(void);

// end // from OTG_MON/otg_dev.h

// start // from otg_dev.c

//==========================
// OTG LINK CORE REGISTERS
//==========================
enum USBOTG_REGS
{
    //==============================================================================================
    // Core Global Registers
    GOTGCTL        = (USBOTG_LINK_BASE + 0x000),        // OTG Control & Status
    GOTGINT        = (USBOTG_LINK_BASE + 0x004),        // OTG Interrupt
    GAHBCFG        = (USBOTG_LINK_BASE + 0x008),        // Core AHB Configuration
    GUSBCFG        = (USBOTG_LINK_BASE + 0x00C),        // Core USB Configuration
    GRSTCTL        = (USBOTG_LINK_BASE + 0x010),        // Core Reset
    GINTSTS        = (USBOTG_LINK_BASE + 0x014),        // Core Interrupt
    GINTMSK        = (USBOTG_LINK_BASE + 0x018),        // Core Interrupt Mask
    GRXSTSR        = (USBOTG_LINK_BASE + 0x01C),        // Receive Status Debug Read/Status Read
    GRXSTSP        = (USBOTG_LINK_BASE + 0x020),        // Receive Status Debug Pop/Status Pop
    GRXFSIZ        = (USBOTG_LINK_BASE + 0x024),        // Receive FIFO Size
    GNPTXFSIZ    = (USBOTG_LINK_BASE + 0x028),        // Non-Periodic Transmit FIFO Size
    GNPTXSTS    = (USBOTG_LINK_BASE + 0x02C),        // Non-Periodic Transmit FIFO/Queue Status

    HPTXFSIZ    = (USBOTG_LINK_BASE + 0x100),        // Host Periodic Transmit FIFO Size
    DPTXFSIZ1    = (USBOTG_LINK_BASE + 0x104),        // Device Periodic Transmit FIFO-1 Size
    DPTXFSIZ2    = (USBOTG_LINK_BASE + 0x108),        // Device Periodic Transmit FIFO-2 Size
    DPTXFSIZ3    = (USBOTG_LINK_BASE + 0x10C),        // Device Periodic Transmit FIFO-3 Size
    DPTXFSIZ4    = (USBOTG_LINK_BASE + 0x110),        // Device Periodic Transmit FIFO-4 Size
    DPTXFSIZ5    = (USBOTG_LINK_BASE + 0x114),        // Device Periodic Transmit FIFO-5 Size
    DPTXFSIZ6    = (USBOTG_LINK_BASE + 0x118),        // Device Periodic Transmit FIFO-6 Size
    DPTXFSIZ7    = (USBOTG_LINK_BASE + 0x11C),        // Device Periodic Transmit FIFO-7 Size
    DPTXFSIZ8    = (USBOTG_LINK_BASE + 0x120),        // Device Periodic Transmit FIFO-8 Size
    DPTXFSIZ9    = (USBOTG_LINK_BASE + 0x124),        // Device Periodic Transmit FIFO-9 Size
    DPTXFSIZ10    = (USBOTG_LINK_BASE + 0x128),        // Device Periodic Transmit FIFO-10 Size
    DPTXFSIZ11    = (USBOTG_LINK_BASE + 0x12C),        // Device Periodic Transmit FIFO-11 Size
    DPTXFSIZ12    = (USBOTG_LINK_BASE + 0x130),        // Device Periodic Transmit FIFO-12 Size
    DPTXFSIZ13    = (USBOTG_LINK_BASE + 0x134),        // Device Periodic Transmit FIFO-13 Size
    DPTXFSIZ14    = (USBOTG_LINK_BASE + 0x138),        // Device Periodic Transmit FIFO-14 Size
    DPTXFSIZ15    = (USBOTG_LINK_BASE + 0x13C),        // Device Periodic Transmit FIFO-15 Size
    
    //==============================================================================================
    // Host Mode Registers
    //------------------------------------------------
    // Host Global Registers
    HCFG        = (USBOTG_LINK_BASE + 0x400),        // Host Configuration
    HFIR        = (USBOTG_LINK_BASE + 0x404),        // Host Frame Interval
    HFNUM        = (USBOTG_LINK_BASE + 0x408),        // Host Frame Number/Frame Time Remaining
    HPTXSTS        = (USBOTG_LINK_BASE + 0x410),        // Host Periodic Transmit FIFO/Queue Status
    HAINT        = (USBOTG_LINK_BASE + 0x414),        // Host All Channels Interrupt
    HAINTMSK    = (USBOTG_LINK_BASE + 0x418),        // Host All Channels Interrupt Mask

    //------------------------------------------------
    // Host Port Control & Status Registers
    HPRT        = (USBOTG_LINK_BASE + 0x440),        // Host Port Control & Status

    //------------------------------------------------
    // Host Channel-Specific Registers
    HCCHAR0        = (USBOTG_LINK_BASE + 0x500),        // Host Channel-0 Characteristics
    HCSPLT0        = (USBOTG_LINK_BASE + 0x504),        // Host Channel-0 Split Control
    HCINT0        = (USBOTG_LINK_BASE + 0x508),        // Host Channel-0 Interrupt
    HCINTMSK0    = (USBOTG_LINK_BASE + 0x50C),        // Host Channel-0 Interrupt Mask
    HCTSIZ0        = (USBOTG_LINK_BASE + 0x510),        // Host Channel-0 Transfer Size
    HCDMA0        = (USBOTG_LINK_BASE + 0x514),        // Host Channel-0 DMA Address

    
    //==============================================================================================
    // Device Mode Registers
    //------------------------------------------------
    // Device Global Registers
    DCFG         = (USBOTG_LINK_BASE + 0x800),        // Device Configuration
    DCTL         = (USBOTG_LINK_BASE + 0x804),        // Device Control
    DSTS         = (USBOTG_LINK_BASE + 0x808),        // Device Status
    DIEPMSK     = (USBOTG_LINK_BASE + 0x810),        // Device IN Endpoint Common Interrupt Mask
    DOEPMSK     = (USBOTG_LINK_BASE + 0x814),        // Device OUT Endpoint Common Interrupt Mask
    DAINT         = (USBOTG_LINK_BASE + 0x818),        // Device All Endpoints Interrupt
    DAINTMSK     = (USBOTG_LINK_BASE + 0x81C),        // Device All Endpoints Interrupt Mask
    DTKNQR1     = (USBOTG_LINK_BASE + 0x820),        // Device IN Token Sequence Learning Queue Read 1
    DTKNQR2     = (USBOTG_LINK_BASE + 0x824),        // Device IN Token Sequence Learning Queue Read 2
    DVBUSDIS     = (USBOTG_LINK_BASE + 0x828),        // Device VBUS Discharge Time
    DVBUSPULSE     = (USBOTG_LINK_BASE + 0x82C),        // Device VBUS Pulsing Time
    DTKNQR3     = (USBOTG_LINK_BASE + 0x830),        // Device IN Token Sequence Learning Queue Read 3
    DTKNQR4     = (USBOTG_LINK_BASE + 0x834),        // Device IN Token Sequence Learning Queue Read 4
    
    //------------------------------------------------
    // Device Logical IN Endpoint-Specific Registers
    DIEPCTL0     = (USBOTG_LINK_BASE + 0x900),        // Device IN Endpoint 0 Control
    DIEPINT0     = (USBOTG_LINK_BASE + 0x908),        // Device IN Endpoint 0 Interrupt
    DIEPTSIZ0     = (USBOTG_LINK_BASE + 0x910),        // Device IN Endpoint 0 Transfer Size
    DIEPDMA0     = (USBOTG_LINK_BASE + 0x914),        // Device IN Endpoint 0 DMA Address
    //------------------------------------------------
    // Device Logical OUT Endpoint-Specific Registers
    DOEPCTL0     = (USBOTG_LINK_BASE + 0xB00),        // Device OUT Endpoint 0 Control
    DOEPINT0     = (USBOTG_LINK_BASE + 0xB08),        // Device OUT Endpoint 0 Interrupt
    DOEPTSIZ0     = (USBOTG_LINK_BASE + 0xB10),        // Device OUT Endpoint 0 Transfer Size
    DOEPDMA0     = (USBOTG_LINK_BASE + 0xB14),        // Device OUT Endpoint 0 DMA Address

    //------------------------------------------------
    PCGCCTL     = (USBOTG_LINK_BASE + 0x0E00),

    // Endpoint FIFO address
    EP0_FIFO    = (USBOTG_LINK_BASE + 0x1000)
    
 };

 
enum BULK_IN_EP_CSR
{
    bulkIn_DIEPCTL        = (DIEPCTL0 + 0x20*BULK_IN_EP),
    bulkIn_DIEPINT        = (DIEPINT0 + 0x20*BULK_IN_EP),
    bulkIn_DIEPTSIZ        = (DIEPTSIZ0 + 0x20*BULK_IN_EP),
    bulkIn_DIEPDMA        = (DIEPDMA0 + 0x20*BULK_IN_EP)
};

enum BULK_OUT_EP_CSR
{
    bulkOut_DOEPCTL        = (DOEPCTL0 + 0x20*BULK_OUT_EP),
    bulkOut_DOEPINT        = (DOEPINT0 + 0x20*BULK_OUT_EP),
    bulkOut_DOEPTSIZ    = (DOEPTSIZ0 + 0x20*BULK_OUT_EP),
    bulkOut_DOEPDMA        = (DOEPDMA0 + 0x20*BULK_OUT_EP)
};

enum EP_FIFO_ADDR
{
    control_EP_FIFO        = (EP0_FIFO + 0x1000*CONTROL_EP),
    bulkIn_EP_FIFO        = (EP0_FIFO + 0x1000*BULK_IN_EP),
    bulkOut_EP_FIFO    = (EP0_FIFO + 0x1000*BULK_OUT_EP)
};


//==========================
// OTG PHY CORE REGISTERS
//==========================
enum OTGPHYC_REG
{
    PHYPWR        = (USBOTG_PHY_BASE+0x00),
    PHYCTRL        = (USBOTG_PHY_BASE+0x04),
    RSTCON        = (USBOTG_PHY_BASE+0x08)
};


//==============================================================================================
// definitions related to Standard Device Requests
enum EP_INDEX
{
    EP0, EP1, EP2, EP3, EP4
};

//------------------------------------------------
// EP0 state
enum EP0_STATE
{
    EP0_STATE_INIT              = 0,
    EP0_STATE_GD_DEV_0            = 11,
    EP0_STATE_GD_DEV_1            = 12,
    EP0_STATE_GD_DEV_2            = 13,
    EP0_STATE_GD_CFG_0          = 21,
    EP0_STATE_GD_CFG_1          = 22,
    EP0_STATE_GD_CFG_2          = 23,
    EP0_STATE_GD_CFG_3          = 24,
    EP0_STATE_GD_CFG_4          = 25,
    EP0_STATE_GD_STR_I0         = 30,
    EP0_STATE_GD_STR_I1         = 31,
    EP0_STATE_GD_STR_I2         = 32,
    EP0_STATE_GD_DEV_QUALIFIER  = 33,
    EP0_STATE_INTERFACE_GET        = 34,    
    EP0_STATE_GET_STATUS0        = 35,
    EP0_STATE_GET_STATUS1        = 36,
    EP0_STATE_GET_STATUS2        = 37,
    EP0_STATE_GET_STATUS3        = 38,
    EP0_STATE_GET_STATUS4        = 39,
    EP0_STATE_GD_CFG_ONLY_0     = 41,
    EP0_STATE_GD_CFG_ONLY_1     = 42,
    EP0_STATE_GD_IF_ONLY_0      = 44,
    EP0_STATE_GD_IF_ONLY_1      = 45,
    EP0_STATE_GD_EP0_ONLY_0     = 46,
    EP0_STATE_GD_EP1_ONLY_0     = 47,
    EP0_STATE_GD_EP2_ONLY_0     = 48,
    EP0_STATE_GD_EP3_ONLY_0     = 49
};

// SPEC1.1

// Standard bmRequestType (direction)
// #define DEVICE_bmREQUEST_TYPE(oDeviceRequest)  ((m_poDeviceRequest->bmRequestType) & 0x80)
enum DEV_REQUEST_DIRECTION
{
    HOST_TO_DEVICE                = 0x00,
    DEVICE_TO_HOST                = 0x80
};

// Standard bmRequestType (Type)
// #define DEVICE_bmREQUEST_TYPE(oDeviceRequest)  ((m_poDeviceRequest->bmRequestType) & 0x60)
enum DEV_REQUEST_TYPE
{
    STANDARD_TYPE               = 0x00,
    CLASS_TYPE                  = 0x20,
    VENDOR_TYPE                 = 0x40,
    RESERVED_TYPE               = 0x60
};

// Standard bmRequestType (Recipient)
// #define DEVICE_bmREQUEST_RECIPIENT(oDeviceRequest)  ((m_poDeviceRequest->bmRequestType) & 0x07)
enum DEV_REQUEST_RECIPIENT
{
    DEVICE_RECIPIENT            = 0,
    INTERFACE_RECIPIENT            = 1,
    ENDPOINT_RECIPIENT            = 2,
    OTHER_RECIPIENT                = 3
};

// Standard bRequest codes
enum STANDARD_REQUEST_CODE
{
    STANDARD_GET_STATUS         = 0,
    STANDARD_CLEAR_FEATURE      = 1,
    STANDARD_RESERVED_1         = 2,
    STANDARD_SET_FEATURE        = 3,
    STANDARD_RESERVED_2         = 4,
    STANDARD_SET_ADDRESS        = 5,
    STANDARD_GET_DESCRIPTOR     = 6,
    STANDARD_SET_DESCRIPTOR     = 7,
    STANDARD_GET_CONFIGURATION  = 8,
    STANDARD_SET_CONFIGURATION  = 9,
    STANDARD_GET_INTERFACE      = 10,
    STANDARD_SET_INTERFACE      = 11,
    STANDARD_SYNCH_FRAME        = 12
};


// Descriptor types
enum DESCRIPTOR_TYPE
{
    DEVICE_DESCRIPTOR           = 1,
    CONFIGURATION_DESCRIPTOR    = 2,
    STRING_DESCRIPTOR           = 3,
    INTERFACE_DESCRIPTOR        = 4,
    ENDPOINT_DESCRIPTOR         = 5,
    DEVICE_QUALIFIER            = 6,
    OTHER_SPEED_CONFIGURATION   = 7,
    INTERFACE_POWER                = 8
};

// configuration descriptor: bmAttributes
enum CONFIG_ATTRIBUTES
{
    CONF_ATTR_DEFAULT           = 0x80, // in Spec 1.0, it was BUSPOWERED bit.
    CONF_ATTR_REMOTE_WAKEUP     = 0x20,
    CONF_ATTR_SELFPOWERED       = 0x40
};

// endpoint descriptor
enum ENDPOINT_ATTRIBUTES
{
    EP_ADDR_IN              = 0x80,
    EP_ADDR_OUT             = 0x00,

    EP_ATTR_CONTROL         = 0x0,
    EP_ATTR_ISOCHRONOUS     = 0x1,
    EP_ATTR_BULK            = 0x2,
    EP_ATTR_INTERRUPT       = 0x3
};

// Descriptor size
enum DESCRIPTOR_SIZE
{
    DEVICE_DESC_SIZE            = 18,
    STRING_DESC0_SIZE           = 4,
    STRING_DESC1_SIZE           = 22,
    STRING_DESC2_SIZE           = 44,
    CONFIG_DESC_SIZE            = 9,
    INTERFACE_DESC_SIZE         = 9,
    ENDPOINT_DESC_SIZE          = 7,
    DEVICE_QUALIFIER_SIZE       = 10,
    OTHER_SPEED_CFG_SIZE         = 9
};
#define CONFIG_DESC_TOTAL_SIZE       (CONFIG_DESC_SIZE+INTERFACE_DESC_SIZE+ENDPOINT_DESC_SIZE*2)
//32 <cfg desc>+<if desc>+<endp0 desc>+<endp1 desc>


//=====================================================================
//definitions related to CSR setting

// GOTGCTL
#define B_SESSION_VALID                (0x1<<19)
#define A_SESSION_VALID            (0x1<<18)

// GAHBCFG
#define PTXFE_HALF                    (0<<8)
#define PTXFE_ZERO                    (1<<8)
#define NPTXFE_HALF                    (0<<7)
#define NPTXFE_ZERO                    (1<<7)
#define MODE_SLAVE                    (0<<5)
#define MODE_DMA                    (1<<5)
#define BURST_SINGLE                (0<<1)
#define BURST_INCR                    (1<<1)
#define BURST_INCR4                    (3<<1)
#define BURST_INCR8                    (5<<1)
#define BURST_INCR16                (7<<1)
#define GBL_INT_UNMASK                (1<<0)
#define GBL_INT_MASK                (0<<0)

// GRSTCTL
#define AHB_MASTER_IDLE            (0x1<<31)
#define CORE_SOFT_RESET            (0x1<<0)

// GINTSTS/GINTMSK core interrupt register
#define INT_RESUME                          (0x1<<31)
#define INT_DISCONN                          (0x1<<29)
#define INT_CONN_ID_STS_CNG        (0x1<<28)
#define INT_OUT_EP                    (0x1<<19)
#define INT_IN_EP                    (0x1<<18)
#define INT_ENUMDONE                (0x1<<13)
#define INT_RESET                           (0x1<<12)
#define INT_SUSPEND                         (0x1<<11)
#define INT_TX_FIFO_EMPTY            (0x1<<5)
#define INT_RX_FIFO_NOT_EMPTY        (0x1<<4)
#define INT_SOF                        (0x1<<3)
#define INT_DEV_MODE                (0x0<<0)
#define INT_HOST_MODE                (0x1<<1)

// GRXSTSP STATUS
#define OUT_PKT_RECEIVED            (0x2<<17)
#define SETUP_PKT_RECEIVED            (0x6<<17)

// DCTL device control register
#define NORMAL_OPERATION            (0x1<<0)
#define SOFT_DISCONNECT            (0x1<<1)

// DAINT device all endpoint interrupt register
#define INT_IN_EP0                    (0x1<<0)
#define INT_IN_EP1                    (0x1<<1)
#define INT_IN_EP3                    (0x1<<3)
#define INT_OUT_EP0                    (0x1<<16)
#define INT_OUT_EP2                    (0x1<<18)
#define INT_OUT_EP4                    (0x1<<20)

// DIEPCTL0/DOEPCTL0 device control IN/OUT endpoint 0 control register
#define DEPCTL_EPENA                (0x1<<31)
#define DEPCTL_EPDIS                (0x1<<30)
#define DEPCTL_SNAK                    (0x1<<27)
#define DEPCTL_CNAK                    (0x1<<26)
#define DEPCTL_CTRL_TYPE            (EP_TYPE_CONTROL<<18)
#define DEPCTL_ISO_TYPE                (EP_TYPE_ISOCHRONOUS<<18)
#define DEPCTL_BULK_TYPE            (EP_TYPE_BULK<<18)
#define DEPCTL_INTR_TYPE            (EP_TYPE_INTERRUPT<<18)
#define DEPCTL_USBACTEP            (0x1<<15)
#define DEPCTL0_MPS_64                (0x0<<0)
#define DEPCTL0_MPS_32                (0x1<<0)
#define DEPCTL0_MPS_16                (0x2<<0)
#define DEPCTL0_MPS_8                (0x3<<0)

// DIEPCTLn/DOEPCTLn device control IN/OUT endpoint n control register

// DIEPMSK/DOEPMSK device IN/OUT endpoint common interrupt mask register
// DIEPINTn/DOEPINTn device IN/OUT endpoint interrupt register
#define BACK2BACK_SETUP_RECEIVED          (0x1<<6)
#define INTKN_TXFEMP                    (0x1<<4)
#define NON_ISO_IN_EP_TIMEOUT            (0x1<<3)
#define CTRL_OUT_EP_SETUP_PHASE_DONE    (0x1<<3)
#define AHB_ERROR                        (0x1<<2)
#define TRANSFER_DONE                    (0x1<<0)


// end // from otg_dev.c


// start // from sync.h
#if 1
#define Outp32Inform(No_Reg, data)    (*(volatile UINT32 *)(0xB2A0FA00 + 4*No_Reg) = (data))
#define Inp32Inform(No_Reg)            (*(volatile UINT32 *)(0xB2A0FA00 + 4*No_Reg))
#define Outp32SYSC(Offset, data)    (*(volatile UINT32 *)(0xB2A0F000 + Offset) = (data))
#define Inp32SYSC(Offset)            (*(volatile UINT32 *)(0xB2A0F000 + Offset))
#else
#define Outp32Inform(No_Reg, data)    (*(volatile UINT32 *)(OALPAtoVA(0x7E00FA00, FALSE) + 4*No_Reg) = (data))
#define Inp32Inform(No_Reg)            (*(volatile UINT32 *)(OALPAtoVA(0x7E00FA00, FALSE) + 4*No_Reg))
#define Outp32SYSC(Offset, data)    (*(volatile UINT32 *)(OALPAtoVA(0x7E00F000, FALSE) + Offset) = (data))
#define Inp32SYSC(Offset)            (*(volatile UINT32 *)(OALPAtoVA(0x7E00F000, FALSE) + Offset))
#endif
// end // from sync.h


#endif  // _KITL_USB_SERIAL_H_.
