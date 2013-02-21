//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft
// premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license
// agreement, you are not authorized to use this source code.
// For the terms of the license, please see the license agreement
// signed by you and Microsoft.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#include <windows.h>
#include <oal.h>
#include <usb100.h>
#include <usb200.h>
#include <usbdbgddsi.h>
#include <usbfntypes.h>
#include <usbdbgutils.h>
#include "usbrndis.h"

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

// Class Interface
#define CS_INTERFACE    0x24

//USB Communication Class specific request code used by RNDIS
#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01

#define EP0_MAX_MSG_RECV_BUFFER  1024
///=============================================================================
/// Local function prototypes
///=============================================================================


///=============================================================================
/// Static variables
///=============================================================================

// usb rndis descriptor
static UCHAR m_pucUSBDescriptors[] =
{
    ////////////////////////////////////////////////////////////////////////////
    // Standard Device Descriptor
    //

/* 0  */    18,                         //  bLength = 18 bytes.
/* 1  */    USB_DEVICE_DESCRIPTOR_TYPE, //  bDescriptorType = DEVICE
/* 2  */    0x10, 0x01,                 //  bcdUSB          = 1.1
/* 4  */    0x02,                       //  bDeviceClass    = CDC
/* 5  */    0x00,                       //  bDeviceSubClass = 0
/* 6  */    0x00,                       //  bDeviceProtocol = 0
/* 7  */    0,                          //  bMaxPacketSize0 = EP0 buffer size. retrieved from PDD. 0 by default.
/* 8  */    0x5E, 0x04,                 //  idVendor        = Microsoft Vendor ID.
/* 10 */    0x01, 0x03,                 //  idProduct       = Microsoft generic RNDISMINI Product ID.
/* 12 */    0x01, 0x00,                 //  bcdDevice       = 0.1
/* 14 */    0x01,                       //  iManufacturer   = OEM should fill this..
/* 15 */    0x02,                       //  iProduct        = OEM should fill this..
/* 16 */    0x03,                       //  iSerialNumber   = OEM should fill this..
/* 17 */    0x01,                       //  bNumConfigs     = 1


    ////////////////////////////////////////////////////////////////////////////
    //  RNDIS requires only one configuration as follows..
    //  And we have 2 interfaces (Communication Class if & Dataclass if).
    //

/* 18 */    9,                                  //  bLength         = 9 bytes.
/* 19 */    USB_CONFIGURATION_DESCRIPTOR_TYPE,  //  bDescriptorType = CONFIGURATION
/* 20 */    0x3e, 0x00,                 //  wTotalLength    = From offset 18 to end <---
/* 22 */    0x02,                       //  bNumInterfaces  = 2 (RNDIS spec).
/* 23 */    0x01,                       //  bConfValue      = 1
/* 24 */    0x00,                       //  iConfiguration  = unused.
/* 25 */    0xC0,                       //  bmAttributes    = Self-Powered & Bus-Powered
/* 26 */    0x32,                       //  MaxPower        = 100mA



    ////////////////////////////////////////////////////////////////////////////
    //  Communication Class INTERFACE descriptor.
    //  RNDIS specifies 2 endpoints, EP0 & notification element (interrupt)
    //

/* 27 */    9,                          //  bLength         = 9 bytes.
/* 28 */    USB_INTERFACE_DESCRIPTOR_TYPE, //  bDescriptorType = INTERFACE
/* 29 */    0x00,                       //  bInterfaceNo    = 0
/* 30 */    0x00,                       //  bAlternateSet   = 0
/* 31 */    0x01,                       //  bNumEndPoints   = 1 (RNDIS spec)
/* 32 */    0x02,                       //  bInterfaceClass = CDC
/* 33 */    0x00,                       //  bIfSubClass     = 0
/* 34 */    0x00,                       //  bIfProtocol     = 0
/* 35 */    0x00,                       //  iInterface      = unused.

    ////////////////////////////////////////////////////////////////////////////
    //  Functional Descriptors for Communication Class Interface
    //  per RNDIS spec.
    //

/* 36 */    5,                  //  bFunctionLength = 5 bytes.
/* 37 */    CS_INTERFACE,       //  bDescriptorType = Class Interface
/* 38 */    0x01,               //  bDescSubType    = CALL MGMT func descriptor.
/* 39 */    0x00,               //  bmCapabilities  = See sect 5.2.3.2 USB CDC
/* 40 */    0x01,               //  bDataInterface  = 1 data class i/f.


/* 41 */    4,                  //  bFunctionLength = 4 bytes.
/* 42 */    CS_INTERFACE,       //  bDescriptorType = Class Interface
/* 43 */    0x02,               //  bDescSubType    = ABSTRACT ctrl mgmt desc.
/* 44 */    0x00,               //  bmCapabilities  = See sect 5.2.3.3 USB CDC

/* 45 */    5,                  //  bFunctionLength = 5 bytes.
/* 46 */    CS_INTERFACE,       //  bDescriptorType = Class Interface.
/* 47 */    0x02,               //  bDescSubType    = UNION func descriptor.
/* 48 */    0x00,               //  bMasterIf       = i/f 0 is the master if.
/* 49 */    0x01,               //  bSlaveIf        = i/f 1 is the slave if.

    ////////////////////////////////////////////////////////////////////////////
    //  Endpoint descriptors for Communication Class Interface
    //

/* 50 */    7,                  //  bLength         = 7 bytes.
/* 51 */    USB_ENDPOINT_DESCRIPTOR_TYPE, //  bDescriptorType = ENDPOINT
/* 52 */    0x83,               //  bEndpointAddr   = IN - EP3
/* 53 */    0x03,               //  bmAttributes    = Interrupt endpoint.
/* 54 */    0, 0x00,            //  wMaxPacketSize. retrieved from PDD. 0 by default.
/* 56 */    80,                 //  bInterval       = 80 ms polling from host.



    ////////////////////////////////////////////////////////////////////////////
    //  Data Class INTERFACE descriptor.
    //

/* 57 */    9,                  //  bLength         = 9 bytes.
/* 58 */    USB_INTERFACE_DESCRIPTOR_TYPE, //  bDescriptorType = INTERFACE
/* 59 */    0x01,               //  bInterfaceNo    = 1
/* 60 */    0x00,               //  bAlternateSet   = 0
/* 61 */    0x02,               //  bNumEndPoints   = 2 (RNDIS spec)
/* 62 */    0x0A,               //  bInterfaceClass = Data if class (RNDIS spec)
/* 63 */    0x00,               //  bIfSubClass     = unused.
/* 64 */    0x00,               //  bIfProtocol     = unused.
/* 65 */    0x00,               //  iInterface      = unused.


    ////////////////////////////////////////////////////////////////////////////
    //  Endpoint descriptors for Data Class Interface
    //

/* 66 */    7,                  //  bLength         = 7 bytes.
/* 67 */    USB_ENDPOINT_DESCRIPTOR_TYPE, //  bDescriptorType = ENDPOINT [IN]
/* 68 */    0x82,               //  bEndpointAddr   = IN -- EP2
/* 69 */    0x02,               //  bmAttributes    = BULK
/* 70 */    0, 0x00,            //  wMaxPacketSize. retrieved from PDD. 0 by default.
/* 72 */    0,                  //  bInterval       = ignored for BULK.

/* 73 */    7,                  //  bLength         = 7 bytes.
/* 74 */    USB_ENDPOINT_DESCRIPTOR_TYPE, //  bDescriptorType = ENDPOINT [OUT]
/* 75 */    0x01,               //  bEndpointAddr   = OUT -- EP1
/* 76 */    0x02,               //  bmAttributes    = BULK
/* 77 */    0, 0x00,            //  wMaxPacketSize. retrieved from PDD. 0 by default.
/* 79 */    0                   //  bInterval       = ignored for BULK.

};  //  m_pucUSBDescriptors[]

// bookkeeping pointers and defines to m_pucUSBDescriptors
// change the offsets if you change m_pucUSBDescriptors
//

static USB_ENDPOINT_DESCRIPTOR* const m_pIntInEndPtDesc =
    (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[50];
static USB_ENDPOINT_DESCRIPTOR* const m_pBulkInEndPtDesc =
    (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[66];
static USB_ENDPOINT_DESCRIPTOR* const m_pBulkOutEndPtDesc =
    (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[73];

static USB_ENDPOINT_DESCRIPTOR* const m_pUsbEndPointDescriptor[] =
    {   (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[50],
        (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[66],
        (USB_ENDPOINT_DESCRIPTOR*) &m_pucUSBDescriptors[73]
    };
static const USBDBG_INTERFACE_DESCRIPTOR m_intrfcDesc1 =
    {
        (USB_INTERFACE_DESCRIPTOR*) &m_pucUSBDescriptors[27],
        (USB_ENDPOINT_DESCRIPTOR**) &m_pUsbEndPointDescriptor[0]
    };
static const USBDBG_INTERFACE_DESCRIPTOR m_intrfcDesc2 =
    {
        (USB_INTERFACE_DESCRIPTOR*) &m_pucUSBDescriptors[57],
        (USB_ENDPOINT_DESCRIPTOR**) &m_pUsbEndPointDescriptor[1]
    };
static const USBDBG_INTERFACE_DESCRIPTOR* const m_pIntrfcDesc[] =
    {
        &m_intrfcDesc1,
        &m_intrfcDesc2
    };
static const USBDBG_CONFIG_DESCRIPTOR m_configDesc1 =
    {
        (USB_CONFIGURATION_DESCRIPTOR *) &m_pucUSBDescriptors[18],
        (USBDBG_INTERFACE_DESCRIPTOR**) &m_pIntrfcDesc[0]
    };
static const USBDBG_CONFIG_DESCRIPTOR* const m_configDesc[] =
    {
        &m_configDesc1
    };
static const USBDBG_DEVICE_DESCRIPTOR m_deviceDesc =
    {
        (USB_DEVICE_DESCRIPTOR* ) &m_pucUSBDescriptors,
        (USBDBG_CONFIG_DESCRIPTOR**) &m_configDesc[0]
    };

#define CONFIGDESC_LEN  62  // len from starting of
                            // USB_CONFIGURATION_DESCRIPTOR_TYPE to end (80)

#define CONTROL_ENDPT   0   // control endpt
#define BULKOUT_ENDPT   1   // logical bulkout endpoint
#define BULKIN_ENDPT    2   // logical bulkin endpoint
#define INTIN_ENDPT     3   // logical intin endpoint
#define MAX_ENDPT       INTIN_ENDPT

typedef struct {
    DWORD Notification;
    DWORD dwReserved;
} INTERRUPT_DATA, *PINTERRUPT_DATA;

static const INTERRUPT_DATA m_interruptData = {
    0x01,               // notification
    0x00                // reserved
};

#define USBSERIALNUM_DEFAULT L"FFFFFFFFFFFF\0"
static USB_STRING m_UsbSerialNum =
{
    sizeof(USBSERIALNUM_DEFAULT) + 2,
    USB_STRING_DESCRIPTOR_TYPE,
    USBSERIALNUM_DEFAULT
};

static const USB_STRING_DESCRIPTOR m_SupportedLang =
{
    0x04,
    USB_STRING_DESCRIPTOR_TYPE,
    0x0409          //  US English by default
};

static const USB_STRING m_Manufacturer =
{
    sizeof(MANUFACTURER) + 2,
    USB_STRING_DESCRIPTOR_TYPE,
    MANUFACTURER
};

static const USB_STRING m_Product =
{
    sizeof(PRODUCT) + 2,
    USB_STRING_DESCRIPTOR_TYPE,
    PRODUCT
};

static struct
{
    PBYTE pbData;
    UINT32 cbData;
    BOOL fMsgSent;
} m_rndisMsgToSend;

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

#define MAX_BULKRX_PKT_SIZE         1600
static BYTE m_BulkRxPktBuf[MAX_BULKRX_PKT_SIZE];

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


// compute a unique device serial number using device MAC
static
BOOL
ComputeUSBSerialNumber(
    UINT16 mac[3]
    )
{
const TCHAR DigitTable[] = TEXT("0123456789ABCDEF");

    BYTE b;
    int i;
    BYTE *pIdBytes;

    pIdBytes = (BYTE*)&mac[0];

    for (i=0; i< 6; i++) {
        b = pIdBytes[i];
        m_UsbSerialNum.pwcbString[i * 2] = DigitTable[b % 16];
        m_UsbSerialNum.pwcbString[(i * 2) + 1] = DigitTable[b / 16];
    }

    m_UsbSerialNum.pwcbString[sizeof(mac) * 2] = '\0';

    return TRUE;
}

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


// continue a receive transfer already started on EP0 or EP2
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
        AbortTransfer(INTIN_ENDPT, ENDPT_DIR_IN);
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
        UsbRndis_EventHandler();

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

        case USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE:
            USBDBGMSG(USBDBG_ZONE_INFO, (
                L"UsbDbg:GET_DESCRIPTOR request:0x%x for "
                L"Device Qualifier descriptor is not supported\r\n",
                HIBYTE(wType)
            ));
            fRet = FALSE;
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

static
BOOL
SendPendingRndisMsg(
    )
{
    DWORD dwRet;

    // send the pendig RNDIS message on EP0
    dwRet = SendRecvData(CONTROL_ENDPT,
              ENDPT_DIR_TX,
              m_rndisMsgToSend.pbData,
              m_rndisMsgToSend.cbData,
              0);
    m_rndisMsgToSend.fMsgSent = TRUE;

    if (dwRet == ERROR_SUCCESS)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (
            L"usbdbg:Sent RNDIS Message on EP0 IN %d bytes\r\n",
            m_rndisMsgToSend.cbData));

        return TRUE;
    }
    else
        return FALSE;

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
ProcessRndisMsg(
    PBYTE pbData,
    UINT32 cbData
    )
{
    //wait for the last processing to complete before processing the new pkt
    //

    static BOOL s_fProcessingRndisMsg = FALSE;
    static PBYTE s_pbData = NULL;
    static UINT32 s_cbData = 0;
    static BOOL s_fPendingRndisMsg = FALSE;

    if (s_fProcessingRndisMsg)
    {
        // remember the data for processing once last call to processrndismsg
        // returns
        s_pbData = pbData;
        s_cbData = cbData;
        s_fPendingRndisMsg = TRUE;
        return;
    }

    s_fPendingRndisMsg = FALSE;
    s_fProcessingRndisMsg = TRUE;
    RndisPdd_RecvdMsg(pbData,cbData);
    s_fProcessingRndisMsg = FALSE;

    while (s_fPendingRndisMsg)
    {
        s_fPendingRndisMsg = FALSE;
        s_fProcessingRndisMsg = TRUE;
        RndisPdd_RecvdMsg(s_pbData, s_cbData);
        s_fProcessingRndisMsg = FALSE;
    }

}

static
void
ProcessSetupPacket(
    USB_DEVICE_REQUEST* pUdr,
    BOOL* pfProcessRndisMsg
    )
{
    BOOL fSendRecvStatusACK = TRUE;
    DWORD dwSuccess;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg: +ProcessSetupPacket\r\n"));

    *pfProcessRndisMsg = FALSE;

    // class/vendor request
    //
    if (pUdr->bmRequestType & (USB_REQUEST_CLASS | USB_REQUEST_VENDOR))
    {
        switch (pUdr->bRequest) {

            case SEND_ENCAPSULATED_COMMAND:
                USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                    L"UsbDbg:Setup request [SEND_ENCAPSULATED_COMMAND]\r\n"
                    ));

                // read the rndis msg from PDD
                dwSuccess = SendRecvData(CONTROL_ENDPT,
                                          ENDPT_DIR_RX,
                                          m_ep0MsgRxBuffer,
                                          pUdr->wLength,
                                          0);

                fSendRecvStatusACK = (dwSuccess == ERROR_SUCCESS);
                if (dwSuccess == ERROR_SUCCESS)
                {
                    // This is a RNDIS message. Set flag to process it
                    *pfProcessRndisMsg = TRUE;
                }
                break;

            case GET_ENCAPSULATED_RESPONSE:
                USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                    L"UsbDbg:Setup request [GET_ENCAPSULATED_RESPONSE]\r\n"
                ));
                fSendRecvStatusACK = SendPendingRndisMsg();
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
                if((pUdr->bmRequestType ==  (USB_REQUEST_DEVICE_TO_HOST |USB_REQUEST_FOR_DEVICE))
                    ||(pUdr->bmRequestType ==  (USB_REQUEST_DEVICE_TO_HOST |USB_REQUEST_FOR_INTERFACE))  )
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
                m_pddIfc.pfnIoctl(   // Call the PDD so it can handle the request
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
                m_pddIfc.pfnIoctl( // Call the PDD so it can handle the request
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
    m_pIntInEndPtDesc->wMaxPacketSize = (WORD) m_epMaxPktSize[INTIN_ENDPT];

    return TRUE;
}

///=============================================================================
/// Public Functions (RndisMin -> UsbRndis)
///=============================================================================



// Called by RndisMin to deinitialize
//
void
UsbRndis_Deinit(
    )
{
    // abort all transfers
    AbortTransfers(TRUE, TRUE);

    // disconnect
    m_pddIfc.pfnDisconnect();

    // deinit
    m_pddIfc.pfnDeinit();
}

// Called by RndisMin & UsbRndis to get "interesting" events from PDD
//
UINT32
UsbRndis_EventHandler(
    )
{
    USBDBG_MSG msg = USBDBG_MSG_NOMSG;
    DWORD retVal;
    DWORD epNum;
    static USB_DEVICE_REQUEST pendingUdr;
    static BOOL fProcessingSetupRequest = FALSE;


    USBDBGMSG(USBDBG_ZONE_EVENT, (L"usbdbg:+UsbRndis_EventHandler\r\n"));

    // call PDD to get any interesting events
    retVal = m_pddIfc.pfnEventHandler(&msg, &m_MsgParamBuf[0]);

    //quit if error
    if (retVal != ERROR_SUCCESS)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"ERROR!usbdbg:UsbRndis_EventHandler\r\n"));
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
            BOOL fProcessRndisMsg = FALSE;

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

            ProcessSetupPacket(&udr, &fProcessRndisMsg);

            fProcessingSetupRequest = FALSE;
            //rndis msg received?
            if (fProcessRndisMsg)
                ProcessRndisMsg(m_ep0MsgRxBuffer, udr.wLength);

            // while a setup packet was being processed another might have come
            // in. udr's processing was cancelled and the pending udr was stored
            // in m_pendingUdr.
            while (m_fSetupCancelled)
            {
                m_fSetupCancelled = FALSE;
                fProcessingSetupRequest = TRUE;
                ProcessSetupPacket(&pendingUdr, &fProcessRndisMsg);
                fProcessingSetupRequest = FALSE;
                if (fProcessRndisMsg)
                    ProcessRndisMsg(m_ep0MsgRxBuffer, udr.wLength);
            }

            break;
        }

        case USBDBG_MSG_BUS_EVENT_DETACH:
        case USBDBG_MSG_BUS_EVENT_SUSPEND:
        case USBDBG_MSG_BUS_EVENT_RESET:
        {
            // tell rndismin layer
            RndisPdd_SetRndisState(TRUE);

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
    USBDBGMSG(USBDBG_ZONE_EVENT, (L"usbdbg:-UsbRndis_EventHandler.msg=%d\r\n",
                msg));
    return msg;
}

// Called by RndisMin to send an RNDIS Message.
// We don't do the actual sending here but save it. We then trigger Interrupt
// endpoint to send interrupt to host which in turn uses EP0 to get data by
// sending GET_ENCAPSULATED_RESPONSE
//
void
UsbRndis_SendMessage(
    PBYTE rndisMsg,         /* (IN) rndis message to send */
    UINT32 rndisMsgLen      /* (IN) cb of rndisMsg */
    )
{
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:+UsbRndis_SendMessage.\r\n"));

    m_rndisMsgToSend.pbData = rndisMsg;
    m_rndisMsgToSend.cbData = rndisMsgLen;
    m_rndisMsgToSend.fMsgSent = FALSE;

    USBDBGMSG(USBDBG_ZONE_VERBOSE, (
        L"UsbDbg:Queueing RNDIS Msg [%d bytes]. Issuing EP1 INT IN "
        L"interrupt...\r\n",
        rndisMsgLen
        ));

    // send INT IN msg
    SendRecvData(INTIN_ENDPT,
              ENDPT_DIR_TX,
              (PBYTE) &m_interruptData,
              sizeof(m_interruptData),
              0);

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-UsbRndis_SendMessage.\r\n"));

}

// Called by RndisMin to receive data packets (over EP3 BULK OUT)
//
DWORD
UsbRndis_RecvData(
    PBYTE pbBuffer,
    DWORD cbBufSize
    )
{
    DWORD cbRecvdData = 0;
    DWORD epNum = BULKOUT_ENDPT;    // data comes in on BULK OUT EP3
    DWORD retryCnt = 0;
    EPTransfer* pTransfer = &m_epTransfers[epNum];

#define RECV_RETRY_COUNT 20

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:+UsbRndis_RecvData.\r\n"));


    // call common events handler if rx in progress
    while ((pTransfer->status & USBDBG_TRANSFER_STATUS_INPROGRESS) &&
           (retryCnt++ < RECV_RETRY_COUNT))
    {
        UsbRndis_EventHandler();
    }

    if (pTransfer->status & USBDBG_TRANSFER_STATUS_COMPLETE)
    {
        // copy to return buffer
        cbRecvdData = RndisPdd_ProcessRecvdData(pTransfer->pbBuffer,
                                             pTransfer->cbTransferred,
                                             pbBuffer,
                                             cbBufSize);
    }

//clean:
    // if a transfer is not in progress start next transfer
    if (!(pTransfer->status & USBDBG_TRANSFER_STATUS_INPROGRESS))
    {
        StartTransfer(epNum,
                       ENDPT_DIR_RX,
                       &m_BulkRxPktBuf[0],
                       sizeof(m_BulkRxPktBuf),
                       0);
    }

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbdbg:-UsbRndis_RecvData.\r\n"));
    return cbRecvdData;
}

// Called by RndisMin to send data packets (over EP2 BULK IN)
//
DWORD
UsbRndis_SendData(
    UINT8 *pData,           /* (IN) data to send */
    UINT32 cbDataLen        /* (IN) cb of data to send */
    )
{
    return SendRecvData(BULKIN_ENDPT, ENDPT_DIR_TX, pData, cbDataLen, 0);
}

//
// Called by RndisMin to check if the last rndis message has been sent
//
BOOL
UsbRndis_RndisMsgSent(
    )
{
    return m_rndisMsgToSend.fMsgSent;
}

void
UsbRndis_SetPower(
    BOOL fPowerOff
    )
{
    m_pddIfc.pfnSetPower(fPowerOff);
    if (fPowerOff)
    {
        // Set RNDIS to uninitialized state
        RndisPdd_SetRndisState(TRUE);
    }
}

// Called by RndisMin to initialize UsbDbgRndis stack
//
BOOL                        /* TRUE if success, FALSE if error */
UsbRndis_Init(
    UINT16 mac[3]           /* (IN) device mac address */
    )
{

    m_rndisMsgToSend.fMsgSent = TRUE;

    m_mddInterface.version = 2;

    memset(&m_pddIfc, 0, sizeof(m_pddIfc));

    // Compute USB serial number using mac address.
    // To be used later.
    //
    if (!ComputeUSBSerialNumber(mac))
        return FALSE;

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

