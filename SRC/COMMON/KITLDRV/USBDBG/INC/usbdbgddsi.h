//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement,
// you are not authorized to use this sample source code.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

//=============================================================================
// File Description:
//
// Defines the interface between UsbDbgRndisMdd/UsbDbgSerMdd and UsbDbgPdd.
//
//
//=============================================================================

#ifndef __USBDBGDDSI_H_
#define __USBDBGDDSI_H_

#include <usb100.h>

#ifdef __cplusplus
extern "C" {
#endif

// endpoint direction
typedef enum USBDBG_ENDPTDIR
{
    ENDPT_DIR_RX = 0,
    ENDPT_DIR_OUT = 0,
    ENDPT_DIR_TX = 1,
    ENDPT_DIR_IN = 1
} USBDBG_ENDPTDIR;

// Messages passed to MDD by PDD
//
typedef enum USBDBG_MSG
{
    // no message
    USBDBG_MSG_NOMSG = 0,
    // PDD sends this msg to signal a SETUP packet received
    // A USBDBG_SETUP_DATA* should be placed in dwParam.
    USBDBG_MSG_SETUP_PACKET,
    // PDD sends this msg to signal an OUT packet received on an endpt
    USBDBG_MSG_EP_RX_PKT,
    // PDD sends this msg to signal an IN packet sent on an endpt
    USBDBG_MSG_EP_TX_PKT,
    // PDD sends this msg to signal BUS event detach
    USBDBG_MSG_BUS_EVENT_DETACH, 
    // PDD sends this msg to signal BUS events attach
    USBDBG_MSG_BUS_EVENT_ATTACH, 
    // PDD sends this msg to signal BUS events reset
    USBDBG_MSG_BUS_EVENT_RESET,  
    // PDD sends this msg to signal BUS events suspend
    USBDBG_MSG_BUS_EVENT_SUSPEND,
    // PDD sends this msg to signal BUS events resume
    USBDBG_MSG_BUS_EVENT_RESUME
} USBDBG_MSG;

// Commands passed by MDD to PDD
//
typedef enum USBDBG_CMD
{
    // command to abort a transfer on an endpt
    USBDBG_CMD_TRANSFER_ABORT,
    // command to stall an endpt
    USBDBG_CMD_STALL
} USBDBG_CMD;

// Flags passed by MDD to PDD
//
// indicates a send or receive data for EP0 status stage (ACK stage)
#define USBDBG_MDD_EP0_STATUS_STAGE     0x00000001
// indicates PDD to start a RX/TX transfer
#define USBDBG_MDD_TRANSFER_START       0x00000002

// Flags PDD can pass back for transfer status    
//
#define USBDBG_PDD_TRANSFER_COMPLETE    0x00000001


// properties retrieved by UsbDbgRndisMdd from UsbDbgPdd
//
typedef enum USBDBG_PDD_IOCTL
{
    USBDBG_PDD_IOCTL_ENDPT_MAXPACKETSIZE,      //endpt maximum packet size
    USBDBG_PDD_IOCTL_SERIALNUM_STRING,         //usb serial number
    USBDBG_PDD_IOCTL_MANUFACTURER_STRING,      //usb manufacturer string
    USBDBG_PDD_IOCTL_PRODUCT_STRING,           //usb product string
    USBDBG_PDD_IOCTL_SET_ADDRESS,              //usb device address
    USBDBG_PDD_IOCTL_SET_CONFIGURATION         //usb configuration value
} USBDBG_PDD_IOCTL;

#define MAX_USB_SERIAL_STRING_LEN   (254 / sizeof(WCHAR))   // See USB Spec 2.0 Section 9.6.7
typedef struct 
{
    UCHAR   ucbLength;
    UCHAR   ucbDescriptorType;
    WCHAR   pwcbString[MAX_USB_SERIAL_STRING_LEN];
} USB_STRING;


// structs for organizing a device descriptor
//
typedef struct USBDBG_INTERFACE_DESCRIPTOR
{
    USB_INTERFACE_DESCRIPTOR* pUsbInterfaceDescriptor;
    USB_ENDPOINT_DESCRIPTOR** ppUsbEndPtDescriptors;   //array of usb endpts
                                                       //descriptors

} USBDBG_INTERFACE_DESCRIPTOR;

typedef struct USBDBG_CONFIG_DESCRIPTOR
{
    USB_CONFIGURATION_DESCRIPTOR* pUsbConfigDescriptor;
    USBDBG_INTERFACE_DESCRIPTOR** ppUsbInterfaceDescriptors;//array of usb
                                                            //intrfc descriptors
    
} USBDBG_CONFIG_DESCRIPTOR;


typedef struct USBDBG_DEVICE_DESCRIPTOR
{
    USB_DEVICE_DESCRIPTOR* pUsbDeviceDescriptor;
    USBDBG_CONFIG_DESCRIPTOR** ppUsbConfigDescriptors;   //array of usb config
                                                         //descriptors

} USBDBG_DEVICE_DESCRIPTOR;


// Structure UsbDbgRndisMdd passes to UsbDbgPdd_Init
typedef struct USBDBG_MDD_INTERFACE_INFO
{
    DWORD               version;
} USBDBG_MDD_INTERFACE_INFO;


// Called to DeInit USBDBG_PDD
typedef void (*PFN_USBDBG_PDD_DEINIT) (
    );
    
// Called to attach to USB bus
typedef BOOL (*PFN_USBDBG_PDD_CONNECT) (
    USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc
    );

// Called to detach from USB bus
typedef BOOL (*PFN_USBDBG_PDD_DISCONNECT) (
    );
    

// Called to get various properties from USBDBG_PDD
typedef BOOL (*PFN_USBDBG_PDD_IOCTL) (
    DWORD           ioControlCode,
    LPVOID          lpInBuffer,
    DWORD           cbInBufferSize,
    LPVOID          lpOutBuffer,
    DWORD           cbOutBufferSize,
    LPDWORD         lpBytesReturned
    );

// Called often to check if any interesting events occured
typedef DWORD (*PFN_USBDBG_PDD_EVENTHANDLER) (
    USBDBG_MSG* msg,
    BYTE* msgBuf
    );

// Called by UsbDbgRndisMdd to receive data
typedef DWORD (*PFN_USBDBG_PDD_RECVDATA) (
    DWORD epNum,
    PBYTE pBuffer,
    DWORD pcbBufLen,
    DWORD transferflags,
    DWORD* transferStatus
    );
    
// Called by UsbDbgRndisMdd to send data
typedef DWORD (*PFN_USBDBG_PDD_SENDDATA) (
    DWORD epNum,
    PBYTE pBuffer,
    DWORD cbBufLen,
    DWORD transferFlags,
    DWORD* transferStatus
    );

// Commands USBDBG_CMD_* sent by MDD
typedef DWORD (*PFN_USBDBG_PDD_SENDCMD) (
    USBDBG_CMD cmd,
    DWORD epNum,
    USBDBG_ENDPTDIR epDir
    );

// Called by UsbDbgRndisMdd when device suspends/resumes
typedef void (*PFN_USBDBG_PDD_SETPOWER) (
    BOOL fPowerOff
    );
    
// UsbDbgPdd must fill out this structure
typedef struct USBDBG_PDD_INTERFACE_INFO
{
    DWORD                version;

    PFN_USBDBG_PDD_DEINIT pfnDeinit;
    PFN_USBDBG_PDD_CONNECT pfnConnect;
    PFN_USBDBG_PDD_DISCONNECT pfnDisconnect;
    PFN_USBDBG_PDD_IOCTL pfnIoctl;
    PFN_USBDBG_PDD_EVENTHANDLER pfnEventHandler;
    PFN_USBDBG_PDD_RECVDATA pfnRecvData;
    PFN_USBDBG_PDD_SENDDATA pfnSendData;
    PFN_USBDBG_PDD_SENDCMD pfnSendCmd;
    PFN_USBDBG_PDD_SETPOWER pfnSetPower;
    
} USBDBG_PDD_INTERFACE_INFO;  


// defines for debug zones
#define USBDBG_ZONE_ERROR               0x00010000
#define USBDBG_ZONE_WARN                0x00020000
#define USBDBG_ZONE_INFO                0x00040000
#define USBDBG_ZONE_INIT                0x00080000
#define USBDBG_ZONE_FUNC                0x00100000
#define USBDBG_ZONE_VERBOSE             0x00200000
#define USBDBG_ZONE_SEND                0x00400000
#define USBDBG_ZONE_RECV                0x00800000
#define USBDBG_ZONE_DUMP_PKT            0x01000000
#define USBDBG_ZONE_SETUP_REQUEST       0x02000000
#define USBDBG_ZONE_EVENT               0x04000000

// defined by PDD
extern UINT32 g_UsbDbgZones;

// USBDBGMSG prints a messages if
// 1) the appropriate USBDBG_ZONE_* is turned ON
#define USBDBGMSG(zone, str) \
    ((g_UsbDbgZones & (zone)) ? (OALMSGS(1, str)), 1 : 0)
//    ((g_UsbDbgZones & (zone)) ? (OALMSGS(OAL_ETHER, str)): 0)

#ifdef __cplusplus
}
#endif

#endif
