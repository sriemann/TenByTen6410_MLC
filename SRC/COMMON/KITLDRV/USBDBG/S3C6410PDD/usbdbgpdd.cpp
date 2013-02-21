//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#include <windows.h>
#include <usb200.h>
#include <usbtypes.h>
#include <oal.h>
#include <bsp.h>
#include <usbdbgddsi.h>
#include <usbdbgutils.h>
#include "Otgdev.h"


///=============================================================================
/// Discussions
///=============================================================================
/* Interrupt mode - USBDBG mode always runs in polling mode. Interrupt mode
// means interrupts will inform kernel kitl code that a packet has been received
// on a usb endpoint. Implementing interrupt mode improves kitl driver
// performance.
//
// To enable interrupt mode:
// 1. Enable interrupts for
//      a. changes to bus state (reset, suspend, resume, attach, detach)
//      b. setup packet on control endpoint, EP0
//      c. OUT endpoints, EP3 (receiving endpoint)
// 2. Tie the USB function controller interrupt to KITL sysintr (kernel KITL
//    code will then call Rndis_RecvFrame in usbdbgrndismdd to receive the
//    packet).
*/
///

///=============================================================================
/// Global defines
///=============================================================================
// mandatory (refer usbdbgddsi.h for available zones)
UINT32 g_UsbDbgZones =  USBDBG_ZONE_ERROR | USBDBG_ZONE_WARN |
                        USBDBG_ZONE_INFO;// | USBDBG_ZONE_RECV | USBDBG_ZONE_SEND | USBDBG_ZONE_VERBOSE |
                        //USBDBG_ZONE_FUNC;

DWORD g_LastRxPktStatus;


///=============================================================================
/// Macros
///=============================================================================
                      


///=============================================================================
/// Local defines
///=============================================================================

// define product specific strings
#define MANUFACTURER    L"Samsung"
#define PRODUCT         L"USBDBG KITL for SMDK6410"


///=============================================================================
/// External functions
///=============================================================================



///=============================================================================
/// Static variables
///=============================================================================
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


///=============================================================================
/// Private Functions
///=============================================================================

///=============================================================================
/// Private Functions
///=============================================================================

#ifdef DEBUG
// print a setup packet request
static
void
PrintUDR(
    USB_DEVICE_REQUEST* udr
    )
{
    USBDBGMSG(USBDBG_ZONE_VERBOSE, (
        L"usbpdd: Endpoint Zero Request bmRequestType = 0x%x, bRequest=0x%x,"
        L"wValue=0x%x,wIndex=0x%x,wLength=0x%x\r\n",
        udr->bmRequestType,
        udr->bRequest,
        udr->wValue,
        udr->wIndex,
        udr->wLength
    ));

}
#endif

static
void
DevStatChangeEvent(
    DWORD source,
    USBDBG_MSG* msg,
    BYTE* msgBuf
    )
{
    // Attach
    if ((source & INT_CONN_ID_STS_CNG) != 0)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Device attach detected.\r\n"));

        // Clear source bit
        OUTREG32(GINTSTS, INT_CONN_ID_STS_CNG);
    }

    // Detach
    if ((source & INT_DISCONN) != 0)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd:Device detach detected.\r\n"));

        // Clear source bit
        OUTREG32(GINTSTS, INT_DISCONN);
        
        *msg = USBDBG_MSG_BUS_EVENT_DETACH;
        goto clean;
    }

    // Reset
    if ((source & INT_RESET) != 0)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Reset detected.\r\n"));

        // Clear source bit
        OUTREG32(GINTSTS, INT_RESET);

        // Send the reset event only after the enumeration is done
        // This way the MDD can query the current packet sizes selected
        // based on the enumerated speed.
        
        OTGDevice_HandleReset();
        
        goto clean;
    }

    // Suspend
    if ((source & INT_SUSPEND) != 0)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Suspend detected.\r\n"));

        // Clear source bit
        OUTREG32(GINTSTS, INT_SUSPEND);

        *msg = USBDBG_MSG_BUS_EVENT_SUSPEND;
        goto clean;
    }

    // Resume
    if ((source & INT_RESUME) != 0)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Resume detected.\r\n")); 

        // Clear source bit
        OUTREG32(GINTSTS, INT_RESUME);

        // In the middle of a reset don't process other changes
        *msg = USBDBG_MSG_BUS_EVENT_RESUME;
        goto clean;
    }
    
    // Enum Done
    if ((source & INT_ENUMDONE) != 0)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd:Device enumeration completed.\r\n"));

        OTGDevice_HandleEnumDone();

        // Clear source bit
        OUTREG32(GINTSTS, INT_ENUMDONE);

        // Signal the reset (and enum done) event to MDD
        *msg = USBDBG_MSG_BUS_EVENT_RESET;
        
        goto clean;
    }

clean:
    return;
}


static
void
ProcessSetupEvent(
    USBDBG_MSG* msg,
    BYTE* msgBuf
    )
{
    DWORD data[2];
    USB_DEVICE_REQUEST *pSetup = (USB_DEVICE_REQUEST*)data;
    
    data[0] = INREG32(EP_FIFO[0]);
    data[1] = INREG32(EP_FIFO[0]);

#ifdef DEBUG
    PrintUDR(pSetup);
#endif

    // MDD doesn't call PDD back on these messages
    if (pSetup->bmRequestType == 0)
    {
        switch(pSetup->bRequest)
        {
            case USB_REQUEST_SET_CONFIGURATION:
                if (pSetup->wValue != 0)
                {
                    // device is now in configured state
                    oOtgDev.m_uIsUsbOtgSetConfiguration = 1;
                }
                else
                {
                    oOtgDev.m_uIsUsbOtgSetConfiguration = 0;
                }
                break;

            case USB_REQUEST_SET_ADDRESS:
                OTGDevice_SetAddress(pSetup->wValue);
                break;
        }
    }

    *msg = USBDBG_MSG_SETUP_PACKET;
    memcpy(msgBuf, data, sizeof(data));
}



static
void
WaitForSetupXferDone()
{
    DWORD iStatusRetry = 0;
    DWORD LastCTLRegVal=0;
    DWORD LastINTRegVal=0;
    DWORD LastStatRegVal=0;
    DWORD CTLRegVal=0;
    DWORD INTRegVal=0;
    DWORD StatRegVal=0;
    DWORD pktsize;
    DWORD i=0;
    
    LastCTLRegVal = INREG32(DOEPCTL0);
    LastINTRegVal = INREG32(DOEPINT0);
    
    do
    {
        INTRegVal = INREG32(GINTSTS);
        if (INTRegVal & INT_RX_FIFO_NOT_EMPTY)
        {
            StatRegVal = INREG32(GRXSTSR);
            if ((StatRegVal & PKT_STATUS_MASK) == OUT_XFR_COMPLETED)
            {
                // status entry notifying completion of an earlier OUT transfer
                // just pop it and drop, otherwise we will never get to the Setup Xfer completed status 
                StatRegVal = INREG32(GRXSTSP);
                g_LastRxPktStatus = StatRegVal;

                pktsize = (g_LastRxPktStatus & 0x7ff0)>>4;
                if (pktsize > 0)
                {
                    USBDBGMSG(USBDBG_ZONE_WARN, (L"UsbDbgPdd_RecvData: Waitloop: dwGrxStatus=0x%x (OUT_XFR_COMPLETED) Non zero packet size\r\n", g_LastRxPktStatus));
                }
            } 

            if ((StatRegVal & PKT_STATUS_MASK) == OUT_PKT_RECEIVED)
            {
                break;
            }

            if ((StatRegVal & PKT_STATUS_MASK) == SETUP_XFR_COMPLETED)
            {
                StatRegVal = INREG32(GRXSTSP);
                g_LastRxPktStatus = StatRegVal;

                pktsize = (g_LastRxPktStatus & 0x7ff0)>>4;
                if (pktsize > 0)
                {
                    USBDBGMSG(USBDBG_ZONE_WARN, (L"UsbDbgPdd_RecvData: Waitloop: dwGrxStatus=0x%x (SETUP_XFR_COMPLETED) Non zero packet size\r\n", g_LastRxPktStatus));
                }
                
                break;
            }
        }
        iStatusRetry++;
    } while (iStatusRetry < 10000);
    
    
    CTLRegVal = INREG32(DOEPCTL0);
    INTRegVal = INREG32(DOEPINT0);

    if (!(INTRegVal & 0x8) && iStatusRetry < 10000)
    {
        do
        {
            INTRegVal = INREG32(DOEPINT0);
            if(INTRegVal & 0x8)
            {
                break;
            }
            iStatusRetry++;
        } while (iStatusRetry < 10000);
    }

}



static
void
PowerOff(
    )
{
    USBDBGMSG(USBDBG_ZONE_INFO, (L"\r\nusbpdd: Powering off USB.\r\n"));
    
    // Disconnect hardware
    OTGDevice_SetSoftDisconnect();

    OTGDevice_DeInit();
}

static
void
PowerOn(
    )
{
    OTGDevice_Init();

    if (!OTGDevice_InitEndPts(g_pusbDeviceDescriptor))
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"usbpdd: PowerOn - failed to re-initialize endpoints.\r\n"));
    }

    OTGDevice_ClearSoftDisconnect();
    USBDBGMSG(USBDBG_ZONE_INFO, (L"usbpdd: PowerOn success\r\n"));
}



///=============================================================================
/// USBDBG_PDD DDSI (mdd/pdd) interface
///=============================================================================

//==============================================================================
// Description: Called by MDD when KITL or DLE call its pfnDeInit
//
// Arguments:
//
// Return Value:
//
extern "C"
void
UsbDbgPdd_DeInit(
    )
{
    // Disconnect hardware
    OTGDevice_SetSoftDisconnect();

    OTGDevice_DeInit();
}


//==============================================================================
// Description: Called by MDD during initialization to attach to USB bus.
//
// Arguments:   Device descriptor returned by MDD to Host
//
// Return Value: TRUE if success else FALSE 
// 
extern "C"
BOOL
UsbDbgPdd_Connect(
    USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc
    )
{
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbpdd: +UsbDbgPdd_Connect\r\n"));

    // Connect the line
    OTGDevice_ClearSoftDisconnect();

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbpdd: -UsbDbgPdd_Connect\r\n"));
    return TRUE;
}

//==============================================================================
// Description: Called by MDD to disconnect from USB bus
//
// Arguments:  None
//
// Return Value: TRUE if success else FALSE 
// 
extern "C"
BOOL
UsbDbgPdd_Disconnect(
    )
{
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbpdd: +UsbDbgPdd_Disconnect\r\n"));
    
    // Disconnect hardware
    OTGDevice_SetSoftDisconnect();

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbpdd: -UsbDbgPdd_Disconnect\r\n"));

    return TRUE;

}


//==============================================================================
// Description: Called by MDD to get some PDD specific properties. Look at
//              USBDBG_PDD_IOCTL in file usbdbgifc.h
//
// Arguments:   see below
//
// Ret Value:   TRUE if IOCTL is supported else return FALSE. MDD will use
//              the default implementation of an IOCTL if PDD returns FALSE.
// 
extern "C"
BOOL
UsbDbgPdd_Ioctl(
    DWORD ioControlCode,        /* USBDBG_PDD_IOCTL */
    LPVOID lpInBuffer,          /* incoming buffer */
    DWORD  cbInBufferSize,      /* incoming buffer size */
    LPVOID lpOutBuffer,         /* outgoing buffer */
    DWORD  cbOutBufferSize,     /* outgoing buffer size */
    LPDWORD lpBytesReturned     /* outgoing buffer filled */
    )
{
    switch (ioControlCode)
    {
        // return maxpacket size for an endpoint
        case USBDBG_PDD_IOCTL_ENDPT_MAXPACKETSIZE:
            if (lpInBuffer && lpOutBuffer)
            {
                DWORD endPtNum = *((DWORD*)lpInBuffer);
                if (endPtNum == 0)
                {
                    *((UINT32*)lpOutBuffer) = oOtgDev.m_uControlEPMaxPktSize;
                }
                else
                {
                    *((UINT32*)lpOutBuffer) = oOtgDev.m_uBulkInEPMaxPktSize;
                }
            }            
            return TRUE;
            break;

        case USBDBG_PDD_IOCTL_MANUFACTURER_STRING:
        {
            // lpOutBuffer = USB_STRING** ppManufacturer
            if (lpOutBuffer)
                *((USB_STRING**)(lpOutBuffer)) = (USB_STRING*) &m_Manufacturer;
            return TRUE;
            break;
        }
        case USBDBG_PDD_IOCTL_PRODUCT_STRING:
        {
            // lpOutBuffer = USB_STRING** ppProduct
            if (lpOutBuffer)
                *((USB_STRING**)(lpOutBuffer)) = (USB_STRING*) &m_Product;

            return TRUE;
            break;
        }
    }

    // for rest of the properties use MDD's default    
    return FALSE;
}


//==============================================================================
// Description: MDD calls this API repeatedly to receive updates on USB bus
//              states, receive, transmit complete events on endpoints and setup
//              packet on Endpoint 0.
//
// Arguments:   msg. (OUT) set to the appropriate USBDBG_MSG* defined in
//                   usbdbgddsi.h
//              msgBuf. (OUT) set to setup packet or the endpoint number
//                  [64 bytes in length]
//
// Ret Value:   ERROR_SUCCESS if no error else return error code.
// 
extern "C"
DWORD
UsbDbgPdd_EventHandler(
    USBDBG_MSG* msg,
    BYTE* msgBuf
    )
{
    DWORD source;
    DWORD epNum;
    DWORD epIntNum;
    DWORD pktsize;
    DWORD epIntStatus;
    BOOL fProcessed = FALSE;
    
    *msg = USBDBG_MSG_NOMSG;           //no msg by default

    // Get interrupt source and clear it
    source = INREG32(GINTSTS);

    if (source == 0)
        goto clean;

    // Device state change?
    if ((source & (INT_RESET | INT_RESUME | INT_SUSPEND | INT_DISCONN | INT_CONN_ID_STS_CNG | INT_ENUMDONE)) != 0)
    {
        // Handle device state change
        DevStatChangeEvent(source, msg, msgBuf);
        
        // don't process other interrupts
        goto clean;
    }
    
    
    // RX interrupt
    if (source & INT_RX_FIFO_NOT_EMPTY)
    {
        // Pop packet status
        g_LastRxPktStatus = INREG32(GRXSTSP);

        if (((g_LastRxPktStatus & PKT_STATUS_MASK) == SETUP_XFR_COMPLETED))
        {
            pktsize = (g_LastRxPktStatus & 0x7ff0)>>4;
            if (pktsize > 0)
            {
                USBDBGMSG(USBDBG_ZONE_WARN, (L"UsbDbgPdd_EventHandler: RX_FIFO_NOT_EMPTY dwGrxStatus=0x%x (SETUP Completed) Non zero packet size\r\n", g_LastRxPktStatus));
            }
        }
        else if ((g_LastRxPktStatus & PKT_STATUS_MASK) == OUT_PKT_RECEIVED)
        {
            // get endpoint
            epNum = g_LastRxPktStatus & 0xF;

            // tell MDD pkt received on EPn
            *msg = USBDBG_MSG_EP_RX_PKT;
            *((DWORD*)(msgBuf)) = epNum;

            fProcessed = TRUE;
        }
        else if ((g_LastRxPktStatus & PKT_STATUS_MASK) == SETUP_PKT_RECEIVED)
        {
            // Setup Packet
            ProcessSetupEvent(msg, msgBuf);
            fProcessed = TRUE;
        }

        if (fProcessed)
        {
            goto clean; 
        }
    }

    // IN Endpoint interrupt
    if (source & INT_IN_EP)
    {
        // Find the endpoint number
        epIntNum = INREG32(DAINT);
        epNum = 0;
        while(!(epIntNum & 1))
        {
            epIntNum = epIntNum >> 1;
            epNum++;
        }
        epIntStatus = INREG32(DIEPINT[epNum]);

        if ((epIntStatus & TRANSFER_DONE) &&
            (EndPointsInfo[epNum].EndPtAttributes != 3)) // Need not indicate TRANSFER_DONE for INT IN ep
        {
            // tell MDD pkt transmitted on EPn
            *msg = USBDBG_MSG_EP_TX_PKT;
            *((DWORD*)(msgBuf)) = epNum;
        }

        // Clear source bit
        OUTREG32(DIEPINT[epNum], epIntStatus);
        goto clean;
    }

    // OUT Endpoint interrupt
    if (source & INT_OUT_EP)
    {
        // Find the endpoint number
        epIntNum = INREG32(DAINT);
        epIntNum = epIntNum >> 16; 
        epNum = 0;
        while(!(epIntNum & 1))
        {
            epIntNum = epIntNum >> 1;
            epNum++;
        }
        epIntStatus = INREG32(DOEPINT[epNum]);

        // Clear source bit
        OUTREG32(DOEPINT[epNum], epIntStatus);
        goto clean;
    }


clean:           

    return ERROR_SUCCESS;  

}



//==============================================================================
// Description: MDD calls this function to 
//              1) start a receive transfer on an endpoint
//              2) receive data from an endpoint after getting endpoint rx 
//                 interrupt via UsbDbgPdd_EventsHandler
//
//              Start a receive transfer: when transferFlags = 
//              USBDBG_MDD_TRANSFER_START, start a receive transfer on the 
//              endpt. Do not read value from the receive FIFO yet.
//
//              Receive data: when transferFlags != USBDBG_MDD_TRANSFER_START,
//              read the data from FIFO and write to pBuffer.
//              If buffer is filled up completely or a short packet (packet size
//              smaller than maxpktsize) is received, disable the receive FIFO
//              else reenable the FIFO.
//
// Arguments:   epNum. endpt to receive data on.
//                  USBDBGRNDISMDD endpts: 0, 1: INT IN, 2: BULK IN, 3: BULK OUT
//                  USBDBGSERMDD endpts:   0, 1: BULK OUT, 2: BULK IN
//              pBuffer. buffer data is received in. 
//              cbBufLen. size of pBuffer. If buffer is completely filled 
//                  disable the RX FIFO and set pTransferStatus = 
//                  USBDBG_PDD_TRANSFER_COMPLETE. If a short packet is received,
//                  disable RX FIFO and set pTransferStatus = 
//                  USBDBG_PDD_TRANSFER_COMPLETE. Else reenable the FIFO for
//                  next packet.
//              transferFlags. Bitwise OR of flags.
//                  if transferFlags == USBDBG_MDD_TRANSFER_START start the
//                  transfer. do not return data.
//                  USBDBG_MDD_EP0_STATUS_STAGE marks a receive for EP0 status
//                  stage.
//              pTransferStatus. OUT. set pTransferStatus = 
//                  USBDBG_PDD_TRANSFER_COMPLETE when pBuffer is filled up or
//                  a short packet is received.
//
// Ret Value:   count in bytes of data received. return -1 if error (MDD cancels
//              the transfer if -1 is returned)
// 
extern "C"
DWORD
UsbDbgPdd_RecvData(
    DWORD epNum,
    PBYTE pBuffer,
    DWORD cbBufLen,
    DWORD transferFlags,
    DWORD* pTransferStatus
    )
{
    DWORD count, remain;
    DWORD dwData;
    DWORD index;
    DWORD cbRecvd = 0;
    DWORD cbFullPkt = 0;
    DWORD GrxStatus;

    // just enable RX FIFO and return if transfer is starting
    // MDD is not expecting data at this time
    if (transferFlags & USBDBG_MDD_TRANSFER_START)
    {
        if (((g_LastRxPktStatus & PKT_STATUS_MASK) == SETUP_PKT_RECEIVED) &&
            (epNum == 0))
        {
            // MDD is expecting an OUT packet on EP0 right after receiving a setup packet.
            // This happens for USB RNDIS because the SEND_ENCAPSULATED_COMMAND message is
            // sent as a SETUP packet with the command details coming in as OUT packets on EP0
            // Let's wait here to ensure that the setup transaction is completed and/or OUT EP is ready
            // Otherwise, some data munching is observed on Rx FIFO

            WaitForSetupXferDone();
        }
        
        // set transfer sizes
        OTGDevice_SetOutEpXferSize(epNum, 1); // 1 packet
        
        // Enable endpoint, clear NAK bit
        SETREG32(DOEPCTL[epNum], (DEPCTL_EPENA |DEPCTL_CNAK)); 
        
        goto clean;
    }

    // We get to this point because a packet was RX on epNum
    // (HW automatically disables RX FIFO after receiving packet)
    
    GrxStatus = g_LastRxPktStatus;
    
    ASSERT((GrxStatus & OUT_PKT_RECEIVED) == OUT_PKT_RECEIVED);
    if((GrxStatus & OUT_PKT_RECEIVED) != OUT_PKT_RECEIVED)
        USBDBGMSG(USBDBG_ZONE_WARN, (L"usbpdd:UsbDbgPdd_RecvData: !OUT_PKT_RECEIVED\r\n"));
    
    count = (GrxStatus & 0x7ff0)>>4;

    // if expected data = 0 bytes, we just received it.
    // do not read FIFO. return. ?? since MDD buffer is full disable RX FIFO
    if (cbBufLen == 0) 
    {
        if(count != 0)
            USBDBGMSG(USBDBG_ZONE_WARN, (L"usbpdd:UsbDbgPdd_RecvData: count != 0 for a zero byte packet\r\n"));
        goto clean;
    }

    // Read data
    remain = count;
    while (remain > 4)
    {
        dwData = INREG32(EP_FIFO[epNum]);
        pBuffer[0] = (UCHAR)dwData;
        pBuffer[1] = (UCHAR)(dwData >> 8);
        pBuffer[2] = (UCHAR)(dwData >> 16);
        pBuffer[3] = (UCHAR)(dwData >> 24);
        pBuffer += 4;
        remain -= 4;
    }
    
    if (remain > 0)
    {
        dwData = INREG32(EP_FIFO[epNum]);
        for (index=0; index<remain; index++)
        {
            pBuffer[index] = (UCHAR)(dwData >> (index*8));
        }
    }

    // set number of bytes received
    cbRecvd = count;

    // check to make sure data recvd < buffer length (should not happen)
    if (cbRecvd > cbBufLen)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"usbpdd:ERROR: rx %d bytes > buflen=%d bytes\r\n",
            cbRecvd, cbBufLen));
        cbRecvd = 0;
        goto clean;
    }

    // enable RX FIFO/endpoint if MDD receive buffer is not full
    // and short packet not received
    cbFullPkt = (epNum == 0) ? oOtgDev.m_uControlEPMaxPktSize : oOtgDev.m_uBulkOutEPMaxPktSize;
    if ((cbBufLen > cbRecvd) && (cbRecvd == cbFullPkt))
    {
        // set transfer sizes
        OTGDevice_SetOutEpXferSize(epNum, 1); // 1 packet
        
        // Enable endpoint
        SETREG32(DOEPCTL[epNum], (DEPCTL_EPENA |DEPCTL_CNAK));
    }
    else
    {
        *pTransferStatus |= USBDBG_PDD_TRANSFER_COMPLETE;
    }

clean:

    USBDBGMSG(USBDBG_ZONE_FUNC, (
            L"usbpdd:ep%d recv %d bytes flags=0x%x\r\n",
            epNum, cbRecvd, transferFlags
        ));

    return cbRecvd;
}



//==============================================================================
// Description: MDD calls this function to send data over an endpoint. Send 
//              max data over the endpoint and return cb of data sent.
//
// Arguments:   epNum. endpt to send data on.
//                  USBDBGRNDISMDD endpts: 0, 1: INT IN, 2: BULK IN, 3: BULK OUT 
//                  USBDBGSERMDD endpts:   0, 1: BULK OUT, 2: BULK IN
//              pBuffer. buffered data to send. Note: **this is not aligned.**
//              cbBufLen. size of pBuffer.
//              transferFlags. Bitwise OR. 
//                  USBDBG_EP0_STATUS_STAGE marks a send for EP0 status
//                  stage.
//              pTransferStatus. OUT. set pTransferStatus = 
//                  USBDBG_PDD_TRANSFER_COMPLETE when MDD should not wait for a
//                  TX interrupt which signals that this packet has been
//                  transferred
//
// Ret Value:   count in bytes of data sent. if error returns -1 (current
//              transfer will be cancelled).
//
extern "C"
DWORD
UsbDbgPdd_SendData(
    DWORD epNum,
    PBYTE pBuffer,
    DWORD cbLength,
    DWORD transferFlags,
    DWORD* pTransferStatus
    )
{
    DWORD data;
    DWORD index;
    volatile DWORD dwRegVal;
    DWORD maxpkt = (epNum == 0) ? oOtgDev.m_uControlEPMaxPktSize : oOtgDev.m_uBulkInEPMaxPktSize;
    DWORD pktCount = 0;
    DWORD cbDataToSend = cbLength > maxpkt ? maxpkt : cbLength;
    DWORD cbDataSent = cbDataToSend;

    USBDBGMSG(USBDBG_ZONE_FUNC, (
            L"usbpdd (++):ep%d To send %d bytes flags=0x%x maxpkt=%d\r\n",
            epNum, cbLength, transferFlags, maxpkt
        ));
   
    // clear all interrupts for this endpoint
    dwRegVal = INREG32(DIEPINT[epNum]);
    OUTREG32(DIEPINT[epNum], dwRegVal);

    // Set the transfer sizes
    OUTREG32(DIEPTSIZ[epNum], (1<<19)|(cbDataToSend<<0)); // packet count = 1, xfr size

    // Enable endpoint, clear NAK bit
    SETREG32(DIEPCTL[epNum], (DEPCTL_EPENA |DEPCTL_CNAK)); 

    
    // Write data to FIFO
    while (cbDataSent >= 4) 
    {
        data = (pBuffer[3] << 24) | (pBuffer[2] << 16) | (pBuffer[1] << 8) | pBuffer[0];
        OUTREG32(EP_FIFO[epNum], data);
        pBuffer += 4;
        cbDataSent -= 4;
    }

    if (cbDataSent > 0)
    {
        data = 0;
        for (index=0; index<cbDataSent; index++)
        {
            data |= (pBuffer[0] << (index*8));
            pBuffer++;
        }
        OUTREG32(EP_FIFO[epNum], data);
    }

    // for interrupt IN EP, indicate completion now.
    if (EndPointsInfo[epNum].EndPtAttributes == 3)
    {
        *pTransferStatus |= USBDBG_PDD_TRANSFER_COMPLETE;
    }

    // issued a transfer on ENDPTN
    USBDBGMSG(USBDBG_ZONE_FUNC, (
            L"usbpdd (--):ep%d send %d bytes flags=0x%x\r\n",
            epNum, cbDataToSend, transferFlags
        ));

    return cbDataToSend;
}


//==============================================================================
// Description: MDD calls this function to send miscellaneous commands to pdd.
//              Look at usbdbgddsi.h for USBDBG_CMD_* commands
//
// Arguments:   cmd: an USBDBG_CMD_* command
//              epNum: logical endpoint number
//              epDir: endpoint direction (ENDPT_DIR_IN or ENDPT_DIR_OUT)
//
// Ret Value:   Ignored.
//
extern "C"
DWORD
UsbDbgPdd_SendCmd(
    USBDBG_CMD cmd,
    DWORD epNum,
    USBDBG_ENDPTDIR epDir
    )
{
    DWORD dwRegVal;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"+UsbDbgPdd_SendCmd\r\n"));
    
    switch (cmd)
    {
        // abort transfer on an endpt
        case USBDBG_CMD_TRANSFER_ABORT:
        {
            if (epNum != 0)
            {
                if (epDir == ENDPT_DIR_IN)
                {
                     if (INREG32(DIEPCTL[epNum]) & DEPCTL_EPENA)
                     {
                        OUTREG32(DIEPCTL[epNum], DEPCTL_EPDIS);
                        do
                        {
                            dwRegVal = INREG32(DIEPINT[epNum]);
                        }while( (dwRegVal & 0x2) == 0);
                     }
                }
                else
                {
                    if (INREG32(DOEPCTL[epNum]) & DEPCTL_EPENA)
                    {
                        OUTREG32(DOEPCTL[epNum], DEPCTL_EPDIS);
                        do
                        {
                            dwRegVal = INREG32(DIEPINT[epNum]);
                        }while( (dwRegVal & 0x2) == 0);
                     }
                }
            }
            break;
        }

        default:
            break;
    }

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"-UsbDbgPdd_SendCmd\r\n"));
    return ERROR_SUCCESS;
}

//==============================================================================
// Description: MDD calls this function when OAL calls IOCTL_KITL_POWER_CALL to
//              suspend or resume the device. When OAL calls suspend, 
//              fPowerOff=TRUE. When OAL calls resume, fPowerOff=FALSE. 
//              PDD should take appropriate action here to tear down the USB
//              debug connection (can be as simple as logically detaching USB
//              cable) or reenable the USB Debug connection (can be as simple
//              as logically attaching USB cable).
//
// Arguments:   When OAL calls suspend, fPowerOff=TRUE.
//              When OAL calls resume, fPowerOff=FALSE.
//
// Ret Value:  
//
extern "C"
void
UsbDbgPdd_SetPower(
    BOOL fPowerOff
    )
{
    if (fPowerOff)
        PowerOff();
    else
        PowerOn();
}

//==============================================================================
// Description: PDD should always implement this function. MDD calls it during
//              initialization to fill up the function table with rest of the
//              DDSI functions.
//
// Arguments:   pMddIfc (with MDD's version number)
//              pPddIfc (the function table to be filled)
//              pDeviceDesc (usb device descriptor)
//
// Ret Value:   ERROR_SUCCESS.
//
extern "C"
DWORD
UsbDbgPdd_Init(
    USBDBG_MDD_INTERFACE_INFO* pMddIfc,
    USBDBG_PDD_INTERFACE_INFO* pPddIfc,
    USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc
    )
{
    DWORD rc = E_FAIL;
    
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbpdd: +UsbDbgPdd_Init\r\n"));

    //1. fill up pddifc table
    //
    pPddIfc->version = 2;
    pPddIfc->pfnDeinit = UsbDbgPdd_DeInit;
    pPddIfc->pfnConnect = UsbDbgPdd_Connect;
    pPddIfc->pfnDisconnect = UsbDbgPdd_Disconnect;
    pPddIfc->pfnIoctl = UsbDbgPdd_Ioctl;
    pPddIfc->pfnEventHandler = UsbDbgPdd_EventHandler;
    pPddIfc->pfnRecvData = UsbDbgPdd_RecvData;
    pPddIfc->pfnSendData = UsbDbgPdd_SendData;
    pPddIfc->pfnSendCmd = UsbDbgPdd_SendCmd;
    pPddIfc->pfnSetPower = UsbDbgPdd_SetPower;

    //2. init local variables
    //
    g_LastRxPktStatus = 0;
    
    //3. initialize hardware
    //
    OTGDevice_Init();

    //4. init end points
    //
    if (!OTGDevice_InitEndPts(pDeviceDesc))
        goto clean;

    rc = ERROR_SUCCESS;

clean:
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbpdd: -UsbDbgPdd_Init\r\n"));

    return rc;
}

