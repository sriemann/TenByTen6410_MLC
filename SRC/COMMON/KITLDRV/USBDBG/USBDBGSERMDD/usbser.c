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

#include <windows.h>
#include <oal.h>
#include <usb100.h>
#include <usbdbgddsi.h>
#include <usbfntypes.h>
#include <usbdbgutils.h>
#include "usbser.h"

///=============================================================================
/// extern variables
///=============================================================================

///=============================================================================
/// Extern functions
///=============================================================================
// implemented by PDD
extern BOOL UsbDbgPdd_Init (USBDBG_MDD_INTERFACE_INFO* pMddIfc,
                                   USBDBG_PDD_INTERFACE_INFO* pPddIfc,
                                   USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc);

///=============================================================================
/// Local defines
///=============================================================================
#define SEND_RECV_TIME_OUT   20      // 20 secs

// Device specific request
#define SET_CONTROL_LINE_STATE  0x22

#define EP0_MAX_MSG_RECV_BUFFER  4  // no message ever recvd on EP0
///=============================================================================
/// Local function prototypes
///=============================================================================


///=============================================================================
/// Static variables
///=============================================================================



///////////////////////////////
// Serial USB Descriptor Set //
///////////////////////////////


static UCHAR m_pucUSBDescriptors[] =
{

/////////////////////////////////////////////
// Standard Device Descriptor (One Device) //
/////////////////////////////////////////////

/* 0  */    18,                         //  bLength = 18 bytes.
/* 1  */    USB_DEVICE_DESCRIPTOR_TYPE, //  bDescriptorType = DEVICE
/* 2  */    0x10, 0x01,                 //  bcdUSB          = 1.1
/* 4  */    0xff,                       //  bDeviceClass    = Communication Device Class
/* 5  */    0xff,                       //  bDeviceSubClass = Unused at this time.
/* 6  */    0xff,                       //  bDeviceProtocol = Unused at this time.
/* 7  */    0,                          //  bMaxPacketSize0 = EP0 buffer size. retrieved from PDD. 0 by default.
/* 8  */    0x5E, 0x04,                 //  idVendor        = Microsoft Vendor ID.
/* 10 */    0xCE, 0x00,                 // idProduct
/* 12 */    0, 0,                       // bcdDevice
/* 14 */    0x01,                       //  iManufacturer   = OEM should fill this..
/* 15 */    0x02,                       //  iProduct        = OEM should fill this..
/* 16 */    0x03,                       //  iSerialNumber   = OEM should fill this..
/* 17 */    0x01,                       //  bNumConfigs     = 1

///////////////////////////////////////////////////////////
// Standard Configuration Descriptor (One Configuration) //
///////////////////////////////////////////////////////////

/* 18 */    9,                  // bLength
/* 19 */    USB_CONFIGURATION_DESCRIPTOR_TYPE,
/* 20 */    32, 0,              // wTotalLength
/* 22 */    1,                  // bNumInterfaces
/* 23 */    1,                  // bConfigurationValue
/* 24 */    0,                  // iConfiguration
/* 25 */    0xC0,               // bmAttributes    = Self-Powered & Bus-Powered
/* 26 */    0x32,               // MaxPower        = 100mA

///////////////////////////////////////////////////
// Standard Interface Descriptor (One Interface) //
///////////////////////////////////////////////////

/* 27 */    9,                  // bLength
/* 28 */    USB_INTERFACE_DESCRIPTOR_TYPE,
/* 29 */    0,                  // bInterfaceNumber
/* 30 */    0,                  // bAlternateSetting
/* 31 */    2,                  // bNumEndpoints (number endpoints used, excluding EP0)
/* 32 */    0xff,               // bInterfaceClass
/* 33 */    0xff,               // bInterfaceSubClass
/* 34 */    0xff,               // bInterfaceProtocol
/* 35 */    0,                  // ilInterface  (Index of this interface string desc.)

///////////////////////////////////////////////////
// Standard Endpoint Descriptor (EP1 - BULK OUT) //
///////////////////////////////////////////////////

/* 36 */    7,                  // bLength
/* 37 */    USB_ENDPOINT_DESCRIPTOR_TYPE,
/* 38 */    0x01,               // bEndpointAddress (EP 1, OUT)
/* 39 */    2,                  // bmAttributes  (0010 = Bulk)
/* 40 */    0, 0,               // wMaxPacketSize. retrieved from PDD. 0 by default.
/* 42 */    0,                  // bInterval (ignored for Bulk)

//////////////////////////////////////////////////
// Standard Endpoint Descriptor (EP2 - BULK IN) //
//////////////////////////////////////////////////

/* 43 */    7,                  // bLength
/* 44 */    USB_ENDPOINT_DESCRIPTOR_TYPE,
/* 45 */    0x82,               // bEndpointAddress (EP 2, IN)
/* 46 */    2,                  // bmAttributes  (0010 = Bulk)
/* 47 */    0, 0,               // wMaxPacketSize. retrieved from PDD. 0 by default.
/* 49 */    0                   // bInterval (ignored for Bulk)
};

// bookkeeping pointers and defines to m_pucUSBDescriptors
// change the offsets if you change m_pucUSBDescriptors
//

static USB_ENDPOINT_DESCRIPTOR* m_pBulkInEndPtDesc =
    (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[43];
static USB_ENDPOINT_DESCRIPTOR* m_pBulkOutEndPtDesc =
    (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[36];

static USB_ENDPOINT_DESCRIPTOR* m_pUsbEndPointDescriptor[] =
    {   (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[36],
        (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[43]
    };
static USBDBG_INTERFACE_DESCRIPTOR m_intrfcDesc1 =
    {
        (USB_INTERFACE_DESCRIPTOR*) &m_pucUSBDescriptors[27],
        (USB_ENDPOINT_DESCRIPTOR**) &m_pUsbEndPointDescriptor[0]
    };
static USBDBG_INTERFACE_DESCRIPTOR* m_pIntrfcDesc[] =
    {
        &m_intrfcDesc1
    };
static USBDBG_CONFIG_DESCRIPTOR m_configDesc1 =
    {
        (USB_CONFIGURATION_DESCRIPTOR *) &m_pucUSBDescriptors[18],
        (USBDBG_INTERFACE_DESCRIPTOR**) &m_pIntrfcDesc[0]
    };
static USBDBG_CONFIG_DESCRIPTOR* m_configDesc[] =
    {
        &m_configDesc1
    };
static USBDBG_DEVICE_DESCRIPTOR m_deviceDesc =
    {
        (USB_DEVICE_DESCRIPTOR* ) &m_pucUSBDescriptors,
        (USBDBG_CONFIG_DESCRIPTOR**) &m_configDesc[0]
    };

#define CONFIGDESC_LEN        32    //len from starting of USB_CONFIGURATION_DESCRIPTOR_TYPE
                                    //to end (50)

#define CONTROL_ENDPT   0
#define BULKOUT_ENDPT   1   //logical bulkout endpoint num
#define BULKIN_ENDPT    2   //logical bulkin endpoint num
#define MAX_ENDPT       BULKIN_ENDPT

#define USBSERIALNUM_DEFAULT L"FFFFFFFFFFFF"
static USB_STRING m_UsbSerialNum =
{
    sizeof(USBSERIALNUM_DEFAULT),
    USB_STRING_DESCRIPTOR_TYPE,
    USBSERIALNUM_DEFAULT
};

static const USB_STRING_DESCRIPTOR m_SupportedLang =
{
    0x04,
    USB_STRING_DESCRIPTOR_TYPE,
    0x0409          //  US English by default
};

// default manufacturer
static const USB_STRING m_Manufacturer =
{
    sizeof(MANUFACTURER) + 2,
    USB_STRING_DESCRIPTOR_TYPE,
    MANUFACTURER
};

// default product
static const USB_STRING m_Product =
{
    sizeof(PRODUCT) + 2,
    USB_STRING_DESCRIPTOR_TYPE,
    PRODUCT
};

static BOOL m_fConnected = FALSE;

static BYTE m_ep0MsgRxBuffer[EP0_MAX_MSG_RECV_BUFFER];
static BYTE m_MsgParamBuf[64];     // 64 bytes message buffer

// when registering with the USBFN PDD layer, the PDD will fill in this
// structure with appropriate data and function pointers
static USBDBG_PDD_INTERFACE_INFO m_pddIfc;

// contains the MDD version information
static USBDBG_MDD_INTERFACE_INFO m_mddInterface;


typedef volatile struct {
    PBYTE pbBuffer;
    DWORD cbTransferSize;
    DWORD transferFlags;
    DWORD status;
    DWORD cbTransferred;
    DWORD SendRecvNULL;
    DWORD cbToTransfer;
} EPTransfer;

static EPTransfer m_epTransfers[MAX_ENDPT+1];
static DWORD m_epMaxPktSize[MAX_ENDPT+1];

static BYTE m_BulkRxPktBuf[RECVBUF_MAXSIZE];

static BOOL m_fSetupCancelled = FALSE;

// transfer status'
#define USBDBG_TRANSFER_STATUS_INPROGRESS   0x00000001
#define USBDBG_TRANSFER_STATUS_COMPLETE     0x00000002
#define USBDBG_TRANSFER_STATUS_ABORTED      0x00000004
#define USBDBG_TRANSFER_STATUS_STARTED      0x00000008

///=============================================================================
/// Private Function Prototypes
///=============================================================================

///=============================================================================
/// Private Functions
///=============================================================================


// start a transmit or receive transfer
static
void
StartTransfer(
    DWORD epNum,
    DWORD dir,
    UINT8 *pData,
    UINT32 cbDataLen,
    DWORD transferflags
    )
{
    DWORD transferStatus = 0;
    // get transfer struct on epNum
    EPTransfer* pTransfer = &m_epTransfers[epNum];

    // if ep0 and transfer is cancelled return
    if ((epNum == 0) && m_fSetupCancelled)
        return;

    // reset transfer struct
    pTransfer->pbBuffer = pData;
    pTransfer->cbTransferSize = cbDataLen;
    pTransfer->transferFlags = transferflags;
    pTransfer->status = USBDBG_TRANSFER_STATUS_INPROGRESS;
    pTransfer->cbTransferred = 0;

    // multiple of max pkt size?
    if ((pTransfer->cbTransferSize & (m_epMaxPktSize[epNum]-1)) == 0)
        pTransfer->SendRecvNULL = TRUE;     // yes, send a NULL packet
    else
        pTransfer->SendRecvNULL = FALSE;    // no

    if (dir == ENDPT_DIR_RX)
    {
        // call PDD to enable RX FIFO
        // PDD is expected to only start RX transfer and not return any
        // data when USBDBG_MDD_TRANSFER_START flag is passed

        pTransfer->cbToTransfer = m_pddIfc.pfnRecvData(
                        epNum,
                        pTransfer->pbBuffer,
                        pTransfer->cbTransferSize,
                        pTransfer->transferFlags | USBDBG_MDD_TRANSFER_START,
                        &transferStatus);

        // if transfer is aborted, mark it
        if (pTransfer->cbToTransfer == -1)
        {
            pTransfer->status = USBDBG_TRANSFER_STATUS_ABORTED;
            goto clean;
        }

        // mark xfer complete if PDD signalled transfer complete
        if (transferStatus & USBDBG_PDD_TRANSFER_COMPLETE)
        {
            pTransfer->status = USBDBG_TRANSFER_STATUS_COMPLETE;
        }
        // else NOP (transfer continues)

    }
    else // dir == ENDPT_DIR_TX
    {
        // send first xfer
        pTransfer->cbToTransfer = m_pddIfc.pfnSendData(
                         epNum,
                         pTransfer->pbBuffer,
                         pTransfer->cbTransferSize,
                         pTransfer->transferFlags | USBDBG_MDD_TRANSFER_START,
                         &transferStatus);

        if (pTransfer->cbToTransfer == -1)
        {
            pTransfer->status = USBDBG_TRANSFER_STATUS_ABORTED;
            goto clean;
        }

        // mark xfer complete if PDD signalled transfer complete
        if (transferStatus & USBDBG_PDD_TRANSFER_COMPLETE)
        {
            pTransfer->status = USBDBG_TRANSFER_STATUS_COMPLETE;
        }
    }

clean: ;
}


// continue a receive transfer already started on EP0 or IN endpt
static
void
ContinueRxTransfer(
    DWORD epNum
    )
{
    DWORD transferStatus = 0;
    EPTransfer* pTransfer = &m_epTransfers[epNum];

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:+ContinueRxTransfer.\r\n"));

    // quit if transfer has already been aborted
    if (pTransfer->status & USBDBG_TRANSFER_STATUS_ABORTED)
        goto clean;

    // quit with an error message if transfer was never started
    if (!(pTransfer->status & USBDBG_TRANSFER_STATUS_INPROGRESS))
    {
        USBDBGMSG(USBDBG_ZONE_WARN, (
            L"WARN!:usbdbg:Rx xfer not in progress on ep=%d\r\n",
            epNum));
        goto clean;
    }

    // PDD's responsibility when pfnRecvData is called.
    // 1. BULK ENDPT OUT - if buffer is filled up or short packet received,
    //    disable RX FIFO and set USBDBG_PDD_TRANSFER_COMPLETE
    // 2. CONTROL DATA STAGE OUT - if buffer is filled up disable RX FIFO and
    //    set USBDBG_PDD_TRANSFER_COMPLETE
    // 3. CONTROL STATUS STAGE - if buffer (size passed is 0) is filled up
    //    disable RX FIFO and set USBDBG_PDD_TRANSFER_COMPLETE.

    // get data from PDD
    pTransfer->cbToTransfer = m_pddIfc.pfnRecvData(
                        epNum,
                        pTransfer->pbBuffer +  pTransfer->cbTransferred,
                        pTransfer->cbTransferSize -  pTransfer->cbTransferred,
                        pTransfer->transferFlags,
                        &transferStatus);

    // if transfer is aborted, mark it
    if (pTransfer->cbToTransfer == -1)
    {
        pTransfer->status = USBDBG_TRANSFER_STATUS_ABORTED;
        goto clean;
    }

    // a packet received on epNum
    pTransfer->cbTransferred += pTransfer->cbToTransfer;

    // mark xfer complete if RX buffer is full or PDD signalled transfer complet
    if ((pTransfer->cbTransferred >= pTransfer->cbTransferSize) ||
        (transferStatus & USBDBG_PDD_TRANSFER_COMPLETE))
    {
        pTransfer->status = USBDBG_TRANSFER_STATUS_COMPLETE;
    }
    // else NOP (transfer continues)

clean:
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-ContinueRxTransfer.\r\n"));
}



static
void
ContinueTxTransfer(
    DWORD epNum
    )
{
    BOOL fTransferComplete = FALSE;
    DWORD cbLeft;
    DWORD transferStatus = 0;
    EPTransfer* pTransfer = &m_epTransfers[epNum];

    USBDBGMSG(USBDBG_ZONE_FUNC, (
        L"usbdbg:+ContinueTxTransfer. ep%d\r\n", epNum));

    // quit if transfer has already been aborted
    if (pTransfer->status & USBDBG_TRANSFER_STATUS_ABORTED)
        goto clean;

    // quit with an error message if transfer was never started
    if (!(pTransfer->status & USBDBG_TRANSFER_STATUS_INPROGRESS))
    {
        USBDBGMSG(USBDBG_ZONE_WARN, (
            L"WARN!:usbdbg:Tx xfer not in progress on ep=%d\r\n",
            epNum));
        goto clean;
    }

    // a packet transferred on epNum
    pTransfer->cbTransferred += pTransfer->cbToTransfer;
    cbLeft = pTransfer->cbTransferSize - pTransfer->cbTransferred;

    // if data left to send transfer not finished yet
    if (cbLeft > 0)
        fTransferComplete = FALSE;
    // if null packet not sent yet transfer not finished
    else if (pTransfer->SendRecvNULL && (pTransfer->cbToTransfer != 0))
        fTransferComplete = FALSE;
    else
        fTransferComplete = TRUE;


    // mark xfer complete
    if (fTransferComplete)
    {
        pTransfer->status = USBDBG_TRANSFER_STATUS_COMPLETE;
    }
    // xfer not finished. send more data
    else
    {
        pTransfer->cbToTransfer = m_pddIfc.pfnSendData(
                                epNum,
                                pTransfer->pbBuffer + pTransfer->cbTransferred,
                                cbLeft,
                                pTransfer->transferFlags,
                                &transferStatus);

        // if transfer is aborted, mark it
        if (pTransfer->cbToTransfer == -1)
            pTransfer->status = USBDBG_TRANSFER_STATUS_ABORTED;
        // mark xfer complete if PDD signalled transfer complete
        else if (transferStatus & USBDBG_PDD_TRANSFER_COMPLETE)
            pTransfer->status = USBDBG_TRANSFER_STATUS_COMPLETE;
    }

clean:
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-ContinueTxTransfer.\r\n"));
}

// abort transfer
static
void
AbortTransfer(
    DWORD epNum,
    DWORD dir
    )
{
    EPTransfer* pTransfer = &m_epTransfers[epNum];

    //disable transfer
    m_pddIfc.pfnSendCmd(USBDBG_CMD_TRANSFER_ABORT, epNum, dir);

    pTransfer->status = USBDBG_TRANSFER_STATUS_ABORTED;
}

static
void
AbortTransfers(
    BOOL abortEp0Transfers,
    BOOL abortEpxTransfers
)
{
    if (abortEp0Transfers)
    {
        AbortTransfer(CONTROL_ENDPT, ENDPT_DIR_IN);
        AbortTransfer(CONTROL_ENDPT, ENDPT_DIR_OUT);
    }

    if (abortEpxTransfers)
    {
//        AbortTransfer(INTIN_ENDPT, ENDPT_DIR_IN);
        AbortTransfer(BULKIN_ENDPT, ENDPT_DIR_IN);
        AbortTransfer(BULKOUT_ENDPT, ENDPT_DIR_OUT);
    }
}

// blocking call starts a rx or tx transaction
static
DWORD
SendRecvData(
    UINT32 epNum,
    DWORD dir,
    UINT8 *pData,
    UINT32 cbDataLen,
    DWORD flags
    )
{
    DWORD dwStartSec, dwSec;
    DWORD dwRet;
    EPTransfer* pTransfer = &m_epTransfers[epNum];

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:+SendRecvData. ep%d\r\n", epNum));

    dwStartSec = OEMKitlGetSecs();

    // start rx or tx transfer
    StartTransfer(epNum, dir, pData, cbDataLen, flags);

    // loop until all data sent
    while(pTransfer->status & USBDBG_TRANSFER_STATUS_INPROGRESS)
    {
        UsbSerial_EventHandler();

        dwSec = OEMKitlGetSecs();
        // timed out sending data?
        if ((INT32)(dwSec - dwStartSec) > SEND_RECV_TIME_OUT)
        {
            USBDBGMSG(USBDBG_ZONE_ERROR, (
                L"ERROR!UsbDbg: ep%d dir=%d timed out tx/rx data."
                L"dwSec=%d, dwStartSec=%d\r\n",
                epNum, dir, dwSec, dwStartSec));
            dwRet = ERROR_TIMEOUT;
            AbortTransfer(epNum, dir);
            goto clean;
        }
    }

    if (pTransfer->status & USBDBG_TRANSFER_STATUS_COMPLETE)
    {
        dwRet = ERROR_SUCCESS;
    }
    else
    {
        dwRet = ERROR_IO_INCOMPLETE;
    }

clean:
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-SendRecvData. ep%d\r\n", epNum));

    return dwRet;
}

static
DWORD
SendControlStatusHandshake(
    )
{
    DWORD dwRet;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:+SendControlStatusHandshake\r\n"));

    // Send an empty packet to ACK the transfer
    dwRet = SendRecvData(CONTROL_ENDPT,
                          ENDPT_DIR_TX,
                          m_ep0MsgRxBuffer,
                          0,
                          USBDBG_MDD_EP0_STATUS_STAGE);

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-SendControlStatusHandshake\r\n"));

    return dwRet;

}

static
DWORD
RecvControlStatusHandshake(
    )
{
    DWORD dwRet;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:+RecvControlStatusHandshake\r\n"));

    // receive an empty packet
    dwRet = SendRecvData(CONTROL_ENDPT,
                          ENDPT_DIR_RX,
                          m_ep0MsgRxBuffer,
                          0,
                          USBDBG_MDD_EP0_STATUS_STAGE);

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-RecvControlStatusHandshake\r\n"));

    return dwRet;
}

// for a GET_DESCRIPTOR setup request sends the descriptor
static
BOOL
SendDescriptor(
    USB_DEVICE_REQUEST* pUdr
    )
{
    UCHAR *pucData = NULL;
    WORD wLength = 0;
    WORD wType = pUdr->wValue;
    BOOL fRet = TRUE;

    switch (HIBYTE(wType)) {
        case USB_DEVICE_DESCRIPTOR_TYPE:
            USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                L"UsbDbg:Setup request [USB_REQUEST_GET_DESCRIPTOR: "
                L"USB_DEVICE_DESCRIPTOR_TYPE] Len:%d\r\n",
                pUdr->wLength));
            pucData = m_pucUSBDescriptors;
            wLength = m_pucUSBDescriptors[0];
            break;

        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                L"UsbDbg:Setup request [USB_REQUEST_GET_DESCRIPTOR: "
                L"USB_CONFIGURATION_DESCRIPTOR_TYPE] Len:%d\r\n",
                pUdr->wLength
            ));
            pucData = (UCHAR *)m_configDesc1.pUsbConfigDescriptor;
            // return the full descriptor if host asked for it
            if (pUdr->wLength > m_configDesc1.pUsbConfigDescriptor->bLength)
                wLength = CONFIGDESC_LEN;
            else
                wLength = m_configDesc1.pUsbConfigDescriptor->bLength;
            break;

        case USB_STRING_DESCRIPTOR_TYPE:
            USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                L"UsbDbg:Setup request [USB_REQUEST_GET_DESCRIPTOR: "
                L"USB_STRING_DESCRIPTOR_TYPE] Len:%d\r\n",
                pUdr->wLength));
            switch (LOBYTE(wType))
            {
                case 0x00:
                    pucData = (UCHAR *) &m_SupportedLang;
                    wLength = m_SupportedLang.bLength;
                    break;

                case 0x01:
                    {
                        USB_STRING* pManufacturer = NULL;
                        if (m_pddIfc.pfnIoctl(
                                USBDBG_PDD_IOCTL_MANUFACTURER_STRING,
                                NULL,
                                0,
                                (LPVOID) &pManufacturer,
                                sizeof(pManufacturer),
                                0))
                        {
                            pucData = (UCHAR *)pManufacturer;
                            wLength = pManufacturer->ucbLength;
                        }
                        else
                        {
                            USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                                L"UsbDbg: Using default manufacturer"
                                L" string.\r\n"));
                            pucData = (UCHAR *) &m_Manufacturer;
                            wLength = m_Manufacturer.ucbLength;
                        }
                    }
                    break;

                case 0x02:
                    {
                        USB_STRING* pProductString = NULL;
                        if (m_pddIfc.pfnIoctl(
                                USBDBG_PDD_IOCTL_PRODUCT_STRING,
                                NULL,
                                0,
                                (LPVOID) &pProductString,
                                sizeof(pProductString),
                                0))
                        {
                            pucData = (UCHAR *)pProductString;
                            wLength = pProductString->ucbLength;
                        }
                        else
                        {
                            USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                                L"UsbDbg:Using default product string.\r\n"
                            ));
                            pucData = (UCHAR *) &m_Product;
                            wLength = m_Product.ucbLength;
                        }
                    }
                    break;

                case 0x03:
                    {
                        USB_STRING* pucSerialNumString = NULL;
                        if (m_pddIfc.pfnIoctl(
                            USBDBG_PDD_IOCTL_SERIALNUM_STRING,
                            NULL,
                            0,
                            (LPVOID) &pucSerialNumString,
                            sizeof(pucSerialNumString),
                            0))
                        {
                            pucData = (UCHAR *)pucSerialNumString;
                            wLength = pucSerialNumString->ucbLength;
                        }
                        else
                        {
                            USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                                L"UsbDbg:Using default implementation of "
                                L"UsbSerialNumber generation.\r\n"
                            ));
                            pucData = (UCHAR *) &m_UsbSerialNum;
                            wLength = m_UsbSerialNum.ucbLength;
                        }
                    }
                    break;

                default:
                    USBDBGMSG(USBDBG_ZONE_ERROR, (
                        L"ERROR!UsbDbg:*** Unknown STRING index %d\r\n",
                        LOBYTE(wType)
                    ));
                    fRet = FALSE;
                    break;
            }
            break;

        default:
            USBDBGMSG(USBDBG_ZONE_ERROR, (
                L"ERROR!UsbDbg:*** Unknown GET_DESCRIPTOR request:0x%x\r\n",
                HIBYTE(wType)
            ));
            fRet = FALSE;
            break;
    }

    if (pucData == NULL)
    {
        fRet = FALSE;
    }

    if (fRet)
    {
        fRet = (SendRecvData(CONTROL_ENDPT, ENDPT_DIR_TX, pucData, wLength, 0)
                == ERROR_SUCCESS);
        return fRet;
    }
    else
        return TRUE;    //status stage should still happen

}

//Process GetStatus request from host side
static
BOOL
ProcessGetStatus(
    USB_DEVICE_REQUEST* pUdr
    )
{
    UCHAR pucData[2] = {0};

    if(pUdr->bmRequestType == (USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_FOR_DEVICE))
    {
        //is this device selfpowered?
        if(((m_configDesc1.pUsbConfigDescriptor)->bmAttributes & USB_CONFIG_SELF_POWERED) != 0)
            pucData[0] |= USB_GETSTATUS_SELF_POWERED;
        //is this device configured as remote wakeup resource?
        if(((m_configDesc1.pUsbConfigDescriptor)->bmAttributes & USB_CONFIG_REMOTE_WAKEUP) != 0)
            pucData[0] |= USB_GETSTATUS_REMOTE_WAKEUP_ENABLED;
    }

    return (SendRecvData(CONTROL_ENDPT, ENDPT_DIR_TX, pucData, sizeof(pucData), 0) == ERROR_SUCCESS);

}


static
void
ProcessSetupPacket(
    USB_DEVICE_REQUEST* pUdr
    )
{
    BOOL fSendRecvStatusACK = TRUE;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg: +ProcessSetupPacket\r\n"));

    // class/vendor request
    //
    if (pUdr->bmRequestType & (USB_REQUEST_CLASS | USB_REQUEST_VENDOR))
    {
        switch (pUdr->bRequest) {

            case SET_CONTROL_LINE_STATE:
                USBDBGMSG(USBDBG_ZONE_INFO, (
                    L"UsbDbg:Setup request [SET_CONTROL_LINE_STATE] %d\r\n",
                    pUdr->wValue));

                if (pUdr->wValue)
                    m_fConnected = TRUE;
                else
                    m_fConnected = FALSE;

                break;

            default:
                USBDBGMSG(USBDBG_ZONE_ERROR, (
                    L"ERROR!UsbDbg:Unrecognized Setup request. "
                    L"bmRequestType=0x%x. bRequest=0x%x\r\n",
                    pUdr->bmRequestType,
                    pUdr->bRequest
                ));
                break;
        }
    }

    //a standard request
    //
    else
    {
        switch(pUdr->bRequest) {
            case USB_REQUEST_GET_STATUS:
                if( (pUdr->bmRequestType == (USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_FOR_DEVICE))
                    || (pUdr->bmRequestType == (USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_FOR_INTERFACE)))
                {
                    fSendRecvStatusACK = ProcessGetStatus(pUdr);
                }
                else
                {
                    USBDBGMSG(USBDBG_ZONE_WARN, (
                        L"WARN!UsbDbg:RequestType==0x%d, Unrecognzied"
                        L"or unsupported request\r\n", pUdr->bmRequestType
                        ));
                }
                break;

            case USB_REQUEST_CLEAR_FEATURE:
                if (pUdr->bmRequestType == 0x02)
                    USBDBGMSG(USBDBG_ZONE_WARN, (
                        L"WARN!UsbDbg:***RequestType==0x02\r\n"
                        ));
                break;

            case USB_REQUEST_SET_FEATURE:
                if (pUdr->bmRequestType == 0x02)
                    USBDBGMSG(USBDBG_ZONE_WARN, (
                        L"WARN!usbdbg:***RequestType==0x02\r\n"));
                break;

            case USB_REQUEST_SET_ADDRESS:
                USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                    L"usbdbg:Recvd. USB_REQUEST_SET_ADDRESS.\r\n"));

                m_pddIfc.pfnIoctl( // Call the PDD so it can handle the request
                        USBDBG_PDD_IOCTL_SET_ADDRESS,
                        &pUdr->wValue,
                        sizeof(pUdr->wValue),
                        NULL,
                        0,
                        NULL);
                break;

            case USB_REQUEST_GET_DESCRIPTOR:
                fSendRecvStatusACK = SendDescriptor(pUdr);
                break;

            case USB_REQUEST_SET_DESCRIPTOR:
                break;

            case USB_REQUEST_GET_CONFIGURATION:
                break;

            case USB_REQUEST_SET_CONFIGURATION:
                USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                     L"usbdbg:Recvd. USB_REQUEST_SET_CONFIGURATION.\r\n"));
                m_pddIfc.pfnIoctl(   // Call the PDD so it can handle the request
                     USBDBG_PDD_IOCTL_SET_CONFIGURATION,
                     &(pUdr->wValue),
                     sizeof(pUdr->wValue),
                     NULL,
                     0,
                     NULL);

                break;

            case USB_REQUEST_GET_INTERFACE:
                break;

            case USB_REQUEST_SET_INTERFACE:
                break;

            case USB_REQUEST_SYNC_FRAME:
                break;

            default:
                USBDBGMSG(USBDBG_ZONE_WARN, (
                    L"WARN!usbdbg:***Unknown request 0x%x\r\n", pUdr->bRequest
                ));
        }
    }   // end of else for standard request

    if (fSendRecvStatusACK)
    {
        if (USBFN_IS_OUT_REQUESTTYPE(pUdr->bmRequestType))//Host->Device request
            SendControlStatusHandshake();
        else    // Device->Host request
            RecvControlStatusHandshake();
    }
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg: -ProcessSetupPacket\r\n"));

}


// get end pt max pkt sizes from PDD
//
static
BOOL
UpdateUSBDesc(
    )
{
    UINT32 epNum;

    // get end pt max size from PDD
    for (epNum=0; epNum <= MAX_ENDPT; epNum++)
    {
        if (!m_pddIfc.pfnIoctl(
                USBDBG_PDD_IOCTL_ENDPT_MAXPACKETSIZE,
                (LPVOID) &epNum,
                sizeof(epNum),
                (LPVOID) &m_epMaxPktSize[epNum],
                sizeof(m_epMaxPktSize[epNum]),
                NULL))
        {
            USBDBGMSG(USBDBG_ZONE_ERROR, (
                L"ERROR!usbdbg: Failed to retrieve maxpacketsize from PDD "
                L"for ep=%d\r\n", epNum));
            return FALSE;
        }
    }

    m_deviceDesc.pUsbDeviceDescriptor->bMaxPacketSize0 =
            (UCHAR) m_epMaxPktSize[CONTROL_ENDPT];
    m_pBulkOutEndPtDesc->wMaxPacketSize = (WORD) m_epMaxPktSize[BULKOUT_ENDPT];
    m_pBulkInEndPtDesc->wMaxPacketSize = (WORD) m_epMaxPktSize[BULKIN_ENDPT];
//    m_pIntInEndPtDesc->wMaxPacketSize = (WORD) m_epMaxPktSize[INTIN_ENDPT];

    return TRUE;
}

///=============================================================================
/// Public Functions (serifc -> UsbSerial)
///=============================================================================



// Called by serifc to deinitialize
//
void
UsbSerial_Deinit(
    )
{
    // abort all transfers
    AbortTransfers(TRUE, TRUE);

    // disconnect
    m_pddIfc.pfnDisconnect();

    // deinit
    m_pddIfc.pfnDeinit();
}

// Called by serifc & UsbSerial to get "interesting" events from PDD
//
UINT32
UsbSerial_EventHandler(
    )
{
    USBDBG_MSG msg = USBDBG_MSG_NOMSG;
    DWORD retVal;
    DWORD epNum;
    static USB_DEVICE_REQUEST pendingUdr;
    static BOOL fProcessingSetupRequest = FALSE;


    USBDBGMSG(USBDBG_ZONE_EVENT, (L"usbdbg:+UsbSerial_EventHandler\r\n"));

    // call PDD to get any interesting events
    retVal = m_pddIfc.pfnEventHandler(&msg, &m_MsgParamBuf[0]);

    //quit if error
    if (retVal != ERROR_SUCCESS)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"ERROR!UsbDbg: UsbSerial_EventHandler\r\n"));
        goto clean;
    }

    switch (msg)
    {
        // a packet received on an EP
        case USBDBG_MSG_EP_RX_PKT:
        {
            epNum = *((DWORD*)m_MsgParamBuf);
            ContinueRxTransfer(epNum);
            break;
        }
        // a packet sent on an EP
        case USBDBG_MSG_EP_TX_PKT:
        {
            epNum = *((DWORD*)m_MsgParamBuf);
            ContinueTxTransfer(epNum);
            break;
        }
        // set up packet recvd
        case USBDBG_MSG_SETUP_PACKET:
        {
            USB_DEVICE_REQUEST udr = *((USB_DEVICE_REQUEST*) m_MsgParamBuf);

            // if a transfer is already in progress on EP0, mark this
            // setup packet pending and return to unwind stack.
            if (fProcessingSetupRequest)
            {
                m_fSetupCancelled = TRUE;
                pendingUdr = udr;

                // stop any transfers on EP0
                if (m_epTransfers[0].status & USBDBG_TRANSFER_STATUS_INPROGRESS)
                {
                    AbortTransfer(CONTROL_ENDPT, ENDPT_DIR_RX);
                    AbortTransfer(CONTROL_ENDPT, ENDPT_DIR_TX);
                }
                goto clean;
            }
            else
            {
                m_fSetupCancelled = FALSE;
            }


            fProcessingSetupRequest = TRUE;

            ProcessSetupPacket(&udr);

            fProcessingSetupRequest = FALSE;

            // while a setup packet was being processed another might have come
            // in. udr's processing was cancelled and the pending udr was stored
            // in m_pendingUdr.
            while (m_fSetupCancelled)
            {
                m_fSetupCancelled = FALSE;
                fProcessingSetupRequest = TRUE;
                 ProcessSetupPacket(&pendingUdr);
                fProcessingSetupRequest = FALSE;
            }
            break;
        }

        case USBDBG_MSG_BUS_EVENT_DETACH:
        case USBDBG_MSG_BUS_EVENT_SUSPEND:
        case USBDBG_MSG_BUS_EVENT_RESET:
        {
            // set disconnected
            m_fConnected = FALSE;

            // abort all transfers
            AbortTransfers(TRUE, TRUE);

            // bus speed is determined, update ep's maxpktsize
            UpdateUSBDesc();

            break;
        }

        default:
            break;
    }

clean:
    USBDBGMSG(USBDBG_ZONE_EVENT, (L"usbdbg:-UsbSerial_EventHandler.msg=%d\r\n",
                msg));
    return msg;
}

// Called by serifc to receive data packets (over BULKOUT_ENDPT)
//
UINT32
UsbSerial_RecvData(
    PBYTE pbBuffer,       /* (IN) pointer to received data buffer */
    DWORD cbBufSize       /* (IN) received data buffer length */
    )
{
    DWORD cbRecvdData = 0;
    DWORD epNum = BULKOUT_ENDPT;    // data comes in on BULK OUT EP1
    DWORD retryCnt = 0;
    EPTransfer* pTransfer = &m_epTransfers[epNum];

#define RECV_RETRY_COUNT 20

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:+UsbSerial_RecvData.\r\n"));


    // call common events handler if rx in progress
    while ((pTransfer->status & USBDBG_TRANSFER_STATUS_INPROGRESS) &&
           (retryCnt++ < RECV_RETRY_COUNT))
    {
        UsbSerial_EventHandler();
    }

    if (pTransfer->status & USBDBG_TRANSFER_STATUS_COMPLETE)
    {
        if (cbBufSize < pTransfer->cbTransferred)
        {
            USBDBGMSG(USBDBG_ZONE_ERROR, (L"ERROR!usbdbg:UsbSerial_RecvData:"
                L"Recvd %d bytes > buffer size %d\r\n",
                pTransfer->cbTransferred, cbBufSize));
        }
        else
        {
            cbRecvdData = pTransfer->cbTransferred;
            memcpy(pbBuffer, pTransfer->pbBuffer, cbRecvdData);
        }
    }

    // if a transfer is not in progress start next transfer
    if (!(pTransfer->status & USBDBG_TRANSFER_STATUS_INPROGRESS))
    {
        StartTransfer(epNum,
                       ENDPT_DIR_RX,
                       &m_BulkRxPktBuf[0],
                       sizeof(m_BulkRxPktBuf),
                       0);
    }

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-UsbSerial_RecvData.\r\n"));
    return cbRecvdData;
}

// Called by SerIfc to send data packets (over EP2 BULK IN)
//
DWORD
UsbSerial_SendData(
    UINT8 *pData,           /* (IN) data to send */
    UINT32 cbDataLen        /* (IN) cb of data to send */
    )
{
    return SendRecvData(BULKIN_ENDPT, ENDPT_DIR_TX, pData, cbDataLen, 0);
}

void
UsbSerial_SetPower(
    BOOL fPowerOff
    )
{
    m_pddIfc.pfnSetPower(fPowerOff);
    if (fPowerOff)
    {
        m_fConnected = FALSE;
    }
}

// Called by SerIfc to initialize UsbDbgSer stack
//
BOOL                        /* TRUE if success, FALSE if error */
UsbSerial_Init(
    )
{

    m_mddInterface.version = 2;

    memset(&m_pddIfc, 0, sizeof(m_pddIfc));

    // get usbdbgpdd function table
    //
    if( UsbDbgPdd_Init(&m_mddInterface,
                       &m_pddIfc,
                       (USBDBG_DEVICE_DESCRIPTOR*) &m_deviceDesc
                       ) != ERROR_SUCCESS)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"ERROR!usbdbg:UsbDbgPdd_Init failed!\r\n"));
        return FALSE;
    }

    // verify m_pddIfc
    //
    if ((m_pddIfc.version != 2) ||
        (!m_pddIfc.pfnDeinit) ||
        (!m_pddIfc.pfnConnect) ||
        (!m_pddIfc.pfnDisconnect) ||
        (!m_pddIfc.pfnIoctl) ||
        (!m_pddIfc.pfnEventHandler) ||
        (!m_pddIfc.pfnRecvData) ||
        (!m_pddIfc.pfnSendData) ||
        (!m_pddIfc.pfnSendCmd) ||
        (!m_pddIfc.pfnSetPower))
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"ERROR!usbdbg:Incorrect USBDBGPDD interface. UsbDbgPdd_Init "
            L"failed!\r\n"));
        return FALSE;
    }

    // get max sizes of Endpoints from PDD and store in m_pucUSBDescriptors
    //
    if (!UpdateUSBDesc())
        return FALSE;

    // kick off the attach/reset process
    //
    if (!m_pddIfc.pfnConnect((USBDBG_DEVICE_DESCRIPTOR*) &m_deviceDesc))
        return FALSE;

    return TRUE;
}

BOOL UsbSerial_IsConnected(
    )
{
    return m_fConnected;
}

// This function allows clients to change the Vid/Pid.
void UsbSerial_SetProductId(DWORD dwProductId)
{
    m_pucUSBDescriptors[10] = (UCHAR) ((dwProductId & 0xff));
    m_pucUSBDescriptors[11] = (UCHAR) ((dwProductId & 0xff00) >> 8);
    return;
}

// This function allows clients to change the serial number for the device
void UsbSerial_SetSerialNumberString(__in_ecount(ucNumWideChars) WCHAR* pwszString, UCHAR ucNumWideChars)
{
    DWORD i;
    for (i = 0; i < ucNumWideChars; i++)
    {
        m_UsbSerialNum.pwcbString[i] =  pwszString[i];
    }
        
    m_UsbSerialNum.ucbLength = ucNumWideChars * sizeof(WCHAR);
  
}
