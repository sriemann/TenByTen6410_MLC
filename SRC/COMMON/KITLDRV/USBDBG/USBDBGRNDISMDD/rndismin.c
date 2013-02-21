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
#include <ndis.h>
#include <usbdbgrndis.h>
#include <usbdbgddsi.h>
#include "rndismin.h"
#include "usbrndis.h"


///=============================================================================
/// Local defines
///=============================================================================

#define USBDBG_KITL_MTU             1520        // same as KITL_MTU
#define MAX_OUTGOING_PKT_SIZE       USBDBG_KITL_MTU
#define MAX_OUTGOING_MSG_SIZE       1024

#define RNDIS_MAX_TRANSFER_SIZE     8192        

#define MDD_DRIVER_MAJOR_VERSION    1
#define MDD_DRIVER_MINOR_VERSION    0

#define DEFAULT_MULTICASTLIST_MAX   8   //  Most adapters should support
                                        //      this minimum no..

// vendorid sent to host in response to OID_GEN_VENDOR_ID
// 0xFFFFFF = IEEE-registered code not available
#define RNDIS_VENDORID  0xFFFFFF;
// NIC description sent to host in response to OID_GEN_VENDOR_DESCRIPTION
#define RNDIS_VENDORDESC "Microsoft RNDIS virtual adapter miniport.\0"


///=============================================================================
/// Static variables
///=============================================================================

static struct
{
    RNDIS_MESSAGE rndisMsg;
    BYTE          outgoingBuf[MAX_OUTGOING_MSG_SIZE];
} m_outgoingMsg;

static struct
{
    RNDIS_MESSAGE rndisMsg;
    BYTE          outgoingPktBuf[MAX_OUTGOING_PKT_SIZE];
} m_outgoingPkt;

static RNDIS_MESSAGE m_rndisMsg;

// vendorid sent to host in response to OID_GEN_VENDOR_ID
static const UINT32 m_rndisVendorId = RNDIS_VENDORID;
// NIC description sent to host in response to OID_GEN_VENDOR_DESCRIPTION
static const UCHAR m_pucRndisVendorDesc[] = RNDIS_VENDORDESC;

static UINT32 RNdisMddSupportedOids[] = 
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_MEDIA_CONNECT_STATUS,

/*
//not supported
#ifdef NDIS50_MINIPORT

    //
    //  Power Management..
    //
    
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
    OID_PNP_ENABLE_WAKE_UP    
#endif
*/
};      

static struct 
{
    UINT32              state;                  //  From host point of view.

    //  Rndis miniport operation.
    DWORD               dwCurrentPacketFilter;  //  Current Filter.     
    UCHAR               MacAddr[6];
    UCHAR               MulticastAddresses[DEFAULT_MULTICASTLIST_MAX][6];
    DWORD               dwMulticastListInUse;
}   m_rndisDev;


///=============================================================================
/// Local functions
///=============================================================================

#if 0
extern ULONG SC_GetThreadCallStack (HANDLE hThrd, ULONG dwMaxFrames,
                                    LPVOID lpFrames, DWORD dwFlags,
                                    DWORD dwSkip);
//------------------------------------------------------------------------------
// Description: (utility function) Logs call stack to serial port
//
// Note: Not available in bootloader 
static
void
LogCallStack()
{
    HANDLE hThread = GetCurrentThread();
    #define MAX_FRAMES 10
    CallSnapshotEx lpFrames[MAX_FRAMES];
    DWORD dwCnt, i = 0;

    USBDBGMSG(USBDBG_ZONE_INFO, (L"Current Stack: \r\n"));

    dwCnt = SC_GetThreadCallStack   (hThread,
                                    MAX_FRAMES,
                                    lpFrames,
                                    STACKSNAP_EXTENDED_INFO,
                                    0);
    for (i=0; i < dwCnt; i++)
        USBDBGMSG(USBDBG_ZONE_INFO, (L"RA=0x%x FP=0x%x CP=0x%x PR=0x%x\r\n",
                    lpFrames[i]));

}
#endif

#ifdef DEBUG
static
void
DumpPacket(
    DWORD zone,
    BYTE* pData,
    UINT32 cbDataSize
    )
{
    UINT32 i;

    if (g_UsbDbgZones & (zone))
    {
        for (i=0; i < cbDataSize; i++)
        {
            USBDBGMSG(zone, (L"%02X ", *((PUCHAR)(pData+i))));
            if ( (i+1) % 25 == 0)
                USBDBGMSG(zone, (L"\r\n"));
        }
        if ( i % 25 != 0)
            USBDBGMSG(zone, (L"\r\n"));
    }
}
#endif


//------------------------------------------------------------------------------
// Description: Process NDIS query request
//
// Arguments:
//      NDIS_OID    NDIS OID
//      PVOID       pvInformationBuffer
//      ULONG       ulInformationBufferLength
//      PULONG      pulBytesWritten
//      PULONG      pulBytesNeeded
//
//  Return Value:
//      NDIS_STATUS
//
static 
NDIS_STATUS
HostMiniQueryInformation(    
    IN NDIS_OID Oid,
    IN PVOID    pvInformationBuffer,
    IN ULONG    ulInformationBufferLength,
    OUT PULONG  pulBytesWritten,
    OUT PULONG  pulBytesNeeded
    )
{       
    ULONG       GenericUlong;
    USHORT      GenericUshort;
    PUCHAR      pucBuffer    = (PUCHAR)&GenericUlong;       
    ULONG       ulTotalBytes = sizeof(ULONG);       
    NDIS_STATUS NdisStatus   = NDIS_STATUS_SUCCESS;

    *pulBytesNeeded = *pulBytesWritten = 0;

    switch (Oid) 
    {   
        case OID_GEN_MAC_OPTIONS:
            GenericUlong = (ULONG)(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND   |
                                NDIS_MAC_OPTION_RECEIVE_SERIALIZED      |
                                NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA     |
                                NDIS_MAC_OPTION_NO_LOOPBACK);
            break;

        case OID_GEN_SUPPORTED_LIST:
            pucBuffer = (PUCHAR)RNdisMddSupportedOids;
            ulTotalBytes   = sizeof(RNdisMddSupportedOids);         
            break;

        case OID_GEN_HARDWARE_STATUS:           
            GenericUlong = NdisHardwareStatusReady;         
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            GenericUlong = NdisMedium802_3;         
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            GenericUlong = 1500;
            break;

        case OID_GEN_LINK_SPEED:
            GenericUlong = (ULONG)(10000000);
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            GenericUlong = 1518;
            break;      

        case OID_GEN_RECEIVE_BLOCK_SIZE:
            GenericUlong = 1518;
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
            GenericUlong = 1500;
            break;

        case OID_GEN_VENDOR_ID:
            GenericUlong = m_rndisVendorId;
            ulTotalBytes = 4;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:        
            pucBuffer = (PUCHAR) &m_pucRndisVendorDesc[0];
            ulTotalBytes = strlen((const char *)m_pucRndisVendorDesc);
            break;

        case OID_GEN_VENDOR_DRIVER_VERSION:
            GenericUshort = (MDD_DRIVER_MAJOR_VERSION << 8) + 
                            (MDD_DRIVER_MINOR_VERSION);
            pucBuffer    = (PUCHAR)&GenericUshort;
            ulTotalBytes = sizeof(USHORT);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:         
            GenericUlong = m_rndisDev.dwCurrentPacketFilter;
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            GenericUlong = (ULONG)(1514);
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            GenericUlong = NdisMediaStateConnected;
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            pucBuffer    = m_rndisDev.MacAddr;
            ulTotalBytes = 6;
            break;

        case OID_802_3_CURRENT_ADDRESS:            
            pucBuffer    = m_rndisDev.MacAddr;
            ulTotalBytes = 6;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            //  Arbitrarily chosen number..
            GenericUlong = 16;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            GenericUlong = RNDIS_MAX_PACKETS_PER_MESSAGE;             
            break;
        
        case OID_GEN_XMIT_OK:
            GenericUlong = 0;   
            break;

        case OID_GEN_RCV_OK:
            GenericUlong = 0;   
            break;

        case OID_GEN_XMIT_ERROR:                    
            GenericUlong = 0;
            break;

        case OID_GEN_RCV_ERROR:                 
            GenericUlong = 0;
            break;

        case OID_GEN_RCV_NO_BUFFER:                 
            GenericUlong = 0;
            break;

        case OID_GEN_RCV_CRC_ERROR:                 
            GenericUlong = 0x00;
            break;
        
        default:
            USBDBGMSG(USBDBG_ZONE_WARN, (
                L"RNdis:: OID[%x] not yet implemented!\r\n", Oid
                ));         
            NdisStatus = NDIS_STATUS_INVALID_OID;
            break;
    
    }   //  switch()


    //  Everyone gets here...

    if (NdisStatus == NDIS_STATUS_SUCCESS) 
    {
        if (ulTotalBytes > ulInformationBufferLength) 
        {
            //  Not enough room in pvInformationBuffer. 
            *pulBytesNeeded = ulTotalBytes;
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
        } 
        else 
        {
            //  Store result.
            memcpy(pvInformationBuffer, pucBuffer, ulTotalBytes);
            *pulBytesWritten = ulTotalBytes;         
        }   
    }
    
    return NdisStatus;  

}

//------------------------------------------------------------------------------
//
//  Description: Handles a set operation for a single Oid
//
//  Arguments:
//   Oid                         - The OID of the set.
//   pvInformationBuffer         - Holds the data to be set.
//   ulInformationBufferLength   - The length of pvInformationBuffer.
//   pulBytesRead                - If the call is successful, returns the
//                                 number of bytes read from pvInformationBuffer
//   pulBytesNeeded              - If there is not enough data in 
//                                 pvInformationBuffer to satisfy the OID, 
//                                 returns the amount of storage needed.
//
//  Return Value:
//   NDIS_STATUS_SUCCESS
//   NDIS_STATUS_PENDING
//   NDIS_STATUS_INVALID_LENGTH
//   NDIS_STATUS_INVALID_OID
//
static 
NDIS_STATUS
HostMiniSetInformation(
   IN   NDIS_OID    Oid,
   IN   PVOID       pvInformationBuffer,
   IN   ULONG       ulInformationBufferLength,
   OUT  PULONG      pulBytesRead,
   OUT  PULONG      pulBytesNeeded
   )
{    
    DWORD           dwFilter;

    switch (Oid) 
    { 

        case OID_802_3_MULTICAST_LIST:                                      
            *pulBytesRead = ulInformationBufferLength;
            *pulBytesNeeded = 0;
            
            if ((ulInformationBufferLength % 6) != 0)
                return NDIS_STATUS_INVALID_LENGTH;          

            if ((ulInformationBufferLength / 6) > DEFAULT_MULTICASTLIST_MAX)
                return NDIS_STATUS_MULTICAST_FULL;
            
            memcpy(&(m_rndisDev.MulticastAddresses),
                    pvInformationBuffer, 
                    ulInformationBufferLength);

            m_rndisDev.dwMulticastListInUse = ulInformationBufferLength / 6;

            //  todo VEHub interface
            //  Do something here to inform VEHub that we are listening to the
            //  new set of multicast addresses..

            return NDIS_STATUS_SUCCESS;
            break;
        
        case OID_GEN_CURRENT_PACKET_FILTER:
            *pulBytesRead   = ulInformationBufferLength;
            *pulBytesNeeded = 0;

            if (ulInformationBufferLength != 4 ) 
                return NDIS_STATUS_INVALID_LENGTH;                        
        
            memcpy (&dwFilter, pvInformationBuffer, 4);
        
            //  Reject types we don't support..
            if (dwFilter & 
                 (NDIS_PACKET_TYPE_SOURCE_ROUTING   |
                  NDIS_PACKET_TYPE_SMT              |
                  NDIS_PACKET_TYPE_MAC_FRAME        |
                  NDIS_PACKET_TYPE_FUNCTIONAL       |
                  NDIS_PACKET_TYPE_ALL_FUNCTIONAL   |
                  NDIS_PACKET_TYPE_GROUP)) 
            {
                return NDIS_STATUS_NOT_SUPPORTED;               
            }           

            USBDBGMSG(USBDBG_ZONE_INFO, (
                L"RNdis:: New filter set: [0x%x] --> [0x%x]\r\n",
                 m_rndisDev.dwCurrentPacketFilter, dwFilter
                 ));

            //  Hence we support:
            //  DIRECTED, MULTICAST, ALL_MULTICAST, BROADCAST, 
            //  Set the new value on the adapter..
            
            m_rndisDev.dwCurrentPacketFilter = dwFilter;

            if (dwFilter)                           
                m_rndisDev.state = RNDIS_INITIALIZED;
            else            
                m_rndisDev.state = RNDIS_DATA_INITIALIZED;

            return NDIS_STATUS_SUCCESS;
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            //  No need to record requested lookahead length since we
            //  always indicate the whole packet.
            *pulBytesRead = 4;
            return NDIS_STATUS_SUCCESS;
            break;

        default:
            *pulBytesRead = 0;
            *pulBytesNeeded = 0;
            return NDIS_STATUS_INVALID_OID;
    }
    
}

//------------------------------------------------------------------------------
//  Prepare a rndis packet for transmission
//
static
BOOL
PrepareRndisPacket(
    PBYTE pbData,
    UINT32 cbData
    )
{
    UINT32 outPktLen;
    UINT32 tmp;
    PRNDIS_PACKET   pRndisPacket;

    outPktLen = RNDIS_MESSAGE_SIZE (RNDIS_PACKET) + cbData;
    
    if (outPktLen < cbData)     //overflow
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"RNdis:: ERROR! Outgoing pkt is too big, bailing out!\r\n"
            ));
        return FALSE;
    }        

    tmp = outPktLen;
    
    //  Pad it to multiple of 8 bytes (required by RNDIS host).
    outPktLen += (8 - (outPktLen % 8));

    
    if ((outPktLen > sizeof(m_outgoingPkt)) || (outPktLen < tmp))
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"RNdis:: Outgoing pkt is bigger than buffer size, bailing out!\r\n"
            ));
        return FALSE;
    }   

    //  Fill up the RNDIS_PACKET header
    //
    m_outgoingPkt.rndisMsg.NdisMessageType = REMOTE_NDIS_PACKET_MSG;
    m_outgoingPkt.rndisMsg.MessageLength   = outPktLen; 

    pRndisPacket = &(m_outgoingPkt.rndisMsg.Message.Packet);

    pRndisPacket->DataOffset            = sizeof(RNDIS_PACKET);
    pRndisPacket->DataLength            = cbData;
    pRndisPacket->OOBDataOffset         = 0x00;
    pRndisPacket->OOBDataLength         = 0x00;
    pRndisPacket->NumOOBDataElements    = 0x00;
    pRndisPacket->PerPacketInfoOffset   = 0x00;
    pRndisPacket->PerPacketInfoLength   = 0x00;
    pRndisPacket->VcHandle              = 0x00;
    pRndisPacket->Reserved              = 0x00;
    

    //copy the data to outgoing packet buffer
    memcpy(pRndisPacket+1,pbData,cbData);

    return TRUE;
}

//------------------------------------------------------------------------------
//  Send the Rndis message to host (as opposed to a data packet)
//
static
void
SendRndisMessage(
    DWORD       dwRndisMessageType,
    DWORD       dwRequestId,
    NDIS_STATUS NdisStatus,
    PUCHAR      pucInBuffer,
    DWORD       dwBufferLength,
    DWORD       dwUnspecified1
    )
{
    UINT32      uiMessageSize;

    switch (dwRndisMessageType)
    {
        case REMOTE_NDIS_INITIALIZE_CMPLT:
        {
            PRNDIS_INITIALIZE_COMPLETE  pInitComplete;

            uiMessageSize = RNDIS_MESSAGE_SIZE(RNDIS_INITIALIZE_COMPLETE);  

            m_outgoingMsg.rndisMsg.NdisMessageType = REMOTE_NDIS_INITIALIZE_CMPLT;
            m_outgoingMsg.rndisMsg.MessageLength   = uiMessageSize;
            pInitComplete = &(m_outgoingMsg.rndisMsg.Message.InitializeComplete);
            pInitComplete->RequestId             = dwRequestId;
            pInitComplete->Status                = NdisStatus;
            pInitComplete->MajorVersion          = SUPPORTED_RNDIS_MAJOR_VER;
            pInitComplete->MinorVersion          = SUPPORTED_RNDIS_MINOR_VER;
            pInitComplete->DeviceFlags           = RNDIS_DF_CONNECTIONLESS;
            pInitComplete->Medium                = 0x00;    
            pInitComplete->MaxPacketsPerMessage  = RNDIS_MAX_PACKETS_PER_MESSAGE;           
            pInitComplete->AFListOffset          = 0x00;    
            pInitComplete->AFListSize            = 0x00;            
            pInitComplete->MaxTransferSize       = RNDIS_MAX_TRANSFER_SIZE;
            //   8 byte alignment, to cater for non x86 devices..
            pInitComplete->PacketAlignmentFactor = 0x03;    

            break;
        }

        case REMOTE_NDIS_QUERY_CMPLT:
        {
            PRNDIS_QUERY_COMPLETE   pQueryComplete;
            PUCHAR                  pucBuffer;          
            UINT                    uiRndisQuerySize;

            uiRndisQuerySize = RNDIS_MESSAGE_SIZE(RNDIS_QUERY_COMPLETE);            
            uiMessageSize    = uiRndisQuerySize + dwBufferLength;

            if (uiMessageSize > sizeof(m_outgoingMsg))
            {
                USBDBGMSG(USBDBG_ZONE_ERROR, (
                    L"RNdis:: SendRndisMessage: REMOTE_NDIS_QUERY_CMPLT: "
                    L"Message too big for statically allocated space.\r\n"
                    ));
                break;
            }
            
            pucBuffer = (PUCHAR)(&m_outgoingMsg.rndisMsg) + uiRndisQuerySize;

            //  Have the buffer will fill now..
            //

            m_outgoingMsg.rndisMsg.NdisMessageType = dwRndisMessageType;
            m_outgoingMsg.rndisMsg.MessageLength   = uiMessageSize;     
            pQueryComplete = &(m_outgoingMsg.rndisMsg.Message.QueryComplete);
            pQueryComplete->RequestId               = dwRequestId;          
            pQueryComplete->Status                  = NdisStatus;
            pQueryComplete->InformationBufferLength = dwUnspecified1;           

            pQueryComplete->InformationBufferOffset = (dwBufferLength == 0) ? 
                    (0) : 
                    (pucBuffer - (PUCHAR)pQueryComplete);           

            if (dwBufferLength)             
                memcpy(pucBuffer, pucInBuffer, dwUnspecified1);

            break;
        }

        //  Both messages have same entries..
        //  They are only interested in NdisStatus..
        //

        case REMOTE_NDIS_KEEPALIVE_CMPLT:
        case REMOTE_NDIS_SET_CMPLT:
        {
            PRNDIS_KEEPALIVE_COMPLETE   pKeepAliveOrSetComplete;

            uiMessageSize = RNDIS_MESSAGE_SIZE(RNDIS_KEEPALIVE_COMPLETE); 
            
            if (uiMessageSize > sizeof(m_outgoingMsg))
            {
                USBDBGMSG(USBDBG_ZONE_ERROR, (
                    L"RNdis:: SendRndisMessage: REMOTE_NDIS_SET_CMPLT: Message "
                    L"too big for statically allocated space.\r\n"
                    ));
                break;
            }

            m_outgoingMsg.rndisMsg.NdisMessageType = dwRndisMessageType;
            m_outgoingMsg.rndisMsg.MessageLength   = uiMessageSize;
            pKeepAliveOrSetComplete = &(m_outgoingMsg.rndisMsg.Message.KeepaliveComplete);
            pKeepAliveOrSetComplete->RequestId = dwRequestId;
            pKeepAliveOrSetComplete->Status    = NdisStatus;
            break;
        }

        case REMOTE_NDIS_RESET_CMPLT:
        {
            PRNDIS_RESET_COMPLETE   pResetComplete;

            uiMessageSize = RNDIS_MESSAGE_SIZE(RNDIS_RESET_COMPLETE); 
            
            if (uiMessageSize > sizeof(m_outgoingMsg))
            {
                USBDBGMSG(USBDBG_ZONE_ERROR, (
                    L"RNdis:: SendRndisMessage: REMOTE_NDIS_RESET_CMPLT: "
                    L"Message too big for statically allocated space.\r\n"
                    ));
                break;
            }
            
            m_outgoingMsg.rndisMsg.NdisMessageType = dwRndisMessageType;
            m_outgoingMsg.rndisMsg.MessageLength   = uiMessageSize;
            pResetComplete = &(m_outgoingMsg.rndisMsg.Message.ResetComplete);

            //  Reset is always successful..
            //  We always require host to return the multicast list etc..
            pResetComplete->Status          = NDIS_STATUS_SUCCESS;          
            pResetComplete->AddressingReset = 0x00;                     
            break;
        }
        
        default:
            USBDBGMSG(USBDBG_ZONE_ERROR, (
                L"ERROR: SendRndisMessage:: Incorrect message send "
                L"request\r\n"));
            return;
    }


    //  Send it out..
    //  Buffer will return back to us and freed in 
    //  RndisPdd_SendMessageComplete()
    USBDBGMSG (USBDBG_ZONE_VERBOSE,
        (L"RNdis:: Sending Msg [%s]\r\n",
        dwRndisMessageType == REMOTE_NDIS_INITIALIZE_CMPLT  ? 
            L"INIT-CMPLT" :
        dwRndisMessageType == REMOTE_NDIS_QUERY_CMPLT       ? 
            L"QUERY-CMPLT":
        dwRndisMessageType == REMOTE_NDIS_SET_CMPLT         ? 
            L"SET-CMPLT"  :
        dwRndisMessageType == REMOTE_NDIS_RESET_CMPLT       ? 
            L"RESET-CMPLT":
        dwRndisMessageType == REMOTE_NDIS_KEEPALIVE_CMPLT   ? 
            L"KEEPALIVE-CMPLT" :
            L"UNKNOWN!"
        ));

    UsbRndis_SendMessage((PBYTE)&m_outgoingMsg,
                         m_outgoingMsg.rndisMsg.MessageLength);

}

//------------------------------------------------------------------------------
//  Wait for Rndis to Init.
//
static 
BOOL
IsRndisInit(
    )
{
    DWORD retryCnt = 0;
    
    if (m_rndisDev.dwCurrentPacketFilter != 0)
        return TRUE;

#define INIT_RETRY_COUNT 5

    // call event handler to give an opportunity to initialize
    do
    {
        UsbRndis_EventHandler();
    }
    while ((m_rndisDev.dwCurrentPacketFilter == 0) && 
           (retryCnt++ < INIT_RETRY_COUNT));
           
    // if not initialized return FALSE
    if (m_rndisDev.dwCurrentPacketFilter == 0)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (
            L"INFO: RNdis:: Not initialized. Aborting send/recv frame\r\n"
            ));
        return FALSE;
    }

    return TRUE;
}

/// Public Functions (interface implementation)
/// 
///

//---------------------------------------------------------------------------//
/// incoming OAL_KITL_ETH_DRIVER interface

//------------------------------------------------------------------------------
// Description: Signature matches OAL_KITLETH_INIT defined in oal_kitl.h
//              Called by KITL or DLE to initialize USB DBG RNDIS.
//
// Returns: TRUE if initialized
//          else returns FALSE
// 
BOOL 
Rndis_Init(
    UINT8 *pAddress,
    UINT32 logicalLocation,
    UINT16 mac[3]
    )
{
#define INITIALTIME_SEC 300
    DWORD dwStartSec;

    UNREFERENCED_PARAMETER(pAddress);
    UNREFERENCED_PARAMETER(logicalLocation);

    m_rndisDev.state = RNDIS_UNINITIALIZED;

    // copy mac address
    //
    if ((mac[0] == 0) && (mac[1] == 0) && (mac[2] == 0))
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"ERROR: RNdis:: Empty MAC address passed to Rndis_Init\r\n"
            ));
        return FALSE;
    }
    
    memcpy(&m_rndisDev.MacAddr, &mac[0], 6);
    USBDBGMSG(USBDBG_ZONE_INFO, (
        L"RNdis:: Init: MAC address:%x-%x-%x-%x-%x-%x\r\n",
        m_rndisDev.MacAddr[0], m_rndisDev.MacAddr[1],
        m_rndisDev.MacAddr[2], m_rndisDev.MacAddr[3],
        m_rndisDev.MacAddr[4], m_rndisDev.MacAddr[5]
        )); 

    // init lower layer - usbrndis
    //
    if( !UsbRndis_Init(mac) ){
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"ERROR: RNdis:: UsbRndis_Init failed to initialize"
            ));
        return FALSE;
    }        

    
    // For RNDIS packets to flow from host to device, MAC address of the
    // the virtual ethernet adapter and device MAC address should be different.
    // Change OUI to locally administered address.
    m_rndisDev.MacAddr[0] |= 0x02;

    USBDBGMSG(USBDBG_ZONE_INFO, (L"UsbDbgRndis:: Waiting to connect ...\r\n"));

    // wait for RNDIS init msg from host and filters for INITIALTIME_SEC
    //
    dwStartSec=OEMKitlGetSecs();
    while (OEMKitlGetSecs()-dwStartSec<=INITIALTIME_SEC) {
        UsbRndis_EventHandler();             // call message handler
        if (m_rndisDev.dwCurrentPacketFilter != 0)
            break;
    }

    // print success/failure message
    //
    if (m_rndisDev.dwCurrentPacketFilter != 0) {
        USBDBGMSG(USBDBG_ZONE_INFO, (L"RNdis:: initialization: Success\r\n"));
        return TRUE;
    }
    else {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"RNdis:: initialization: Fail!\r\n"));
        return FALSE;
    }    
}

void 
Rndis_EnableInts(
    )
{
    //empty
}

void
Rndis_DisableInts(
    )
{
    //empty
}   

void
Rndis_CurrentPktFilter(
    UINT32 filter
    )
{
    UNREFERENCED_PARAMETER(filter);
    //empty
}

//------------------------------------------------------------------------------
// Description: Signature matches OAL_KITLETH_SEND_FRAME defined in oal_kitl.h
//              Called by KITL or DLE to send a packet over USB DBG RNDIS.
//
// Returns: ERROR_SUCCESS if no error
//          else return error code
// 
UINT16
Rndis_SendFrame(
    IN  UINT8 *pData,   /* data to send */
    IN  UINT32 size     /* number of bytes to send from pData */
    )
{
    UINT32 cbLeftToSend;
    UINT32 retVal;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"+Rndis_SendFrame: size=%d\r\n", size));
    
    if (!IsRndisInit())
    {
        retVal = 0;
        goto cleanUp;
    }
    
    if (!PrepareRndisPacket(pData, size))
    {
        retVal = ERROR_BUFFER_OVERFLOW;
        goto cleanUp;
    }
    
    cbLeftToSend = m_outgoingPkt.rndisMsg.MessageLength +
                    ((UINT32)(&m_outgoingPkt.rndisMsg.Message) -
                     (UINT32)(&m_outgoingPkt.rndisMsg));
    pData = (UINT8*) &m_outgoingPkt;

    UsbRndis_SendData(pData, cbLeftToSend);
    retVal = ERROR_SUCCESS;

cleanUp:
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"-Rndis_SendFrame: retVal=%d\r\n", retVal));
    return (UINT16)retVal;

}

//------------------------------------------------------------------------------
// Description: Signature matches OAL_KITLETH_GET_FRAME defined in oal_kitl.h
//              Called by KITL or DLE to receive a packet over USB DBG RNDIS.
//
// Returns: Size of data received.
// 
UINT16
Rndis_RecvFrame(
    LPBYTE pbOutBuffer,
    PUSHORT pcbOutBufSize
    )
{
    DWORD cbRecvdData = 0;
    
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"+Rndis_RecvFrame: bufSize=%d\r\n",*pcbOutBufSize));
    
    if (!IsRndisInit())
        goto clean;

    // get frame from PDD
    cbRecvdData = UsbRndis_RecvData(pbOutBuffer, *pcbOutBufSize);

clean:
    //sanity check retVal
    if (cbRecvdData > USBDBG_KITL_MTU)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"ERROR: RNdis:: RecvFrame: cbRecvdData=%d > KITL_MTU.\r\n", cbRecvdData));
    }
    *pcbOutBufSize = (USHORT)cbRecvdData;
    
#ifdef DEBUG
    if (cbRecvdData > 0)
    {
        USBDBGMSG(USBDBG_ZONE_RECV, (L"<<RECV_UsbDbg: %d bytes\r\n", cbRecvdData));
        DumpPacket(USBDBG_ZONE_RECV, pbOutBuffer, cbRecvdData);
    }
#endif

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"-Rndis_RecvFrame: cbRecvdData=%d\r\n",cbRecvdData));

    return (UINT16)cbRecvdData;
}

void
Rndis_PowerOff(
    )
{
    UsbRndis_SetPower(TRUE);
}

void
Rndis_PowerOn(
    )
{
    UsbRndis_SetPower(FALSE);
}

void
Rndis_DeInit(
    )
{
    UsbRndis_Deinit();
}

//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
/// callback interface (from USBRNDIS layer)


//------------------------------------------------------------------------------
// Description: Received a RNDIS_ message. Process the message and send the
//              reply.
//
// Returns: Size of data received.
// 
void
RndisPdd_RecvdMsg(
    PBYTE pbData,
    UINT32 cbData
    )
{
    PRNDIS_MESSAGE  pRndisMessage;

#ifdef SHIP_BUILD
    UNREFERENCED_PARAMETER(cbData);
#endif

    pRndisMessage = (PRNDIS_MESSAGE) pbData;

    USBDBGMSG(USBDBG_ZONE_VERBOSE,  
        (L"RNdis:: Processing RndisMessage [%s] - Length [%d]\r\n",
        pRndisMessage->NdisMessageType == REMOTE_NDIS_INITIALIZE_MSG      ? 
                L"RNDIS_INITIALIZE" : 
        pRndisMessage->NdisMessageType == REMOTE_NDIS_HALT_MSG            ? 
                L"RNDIS_HALT"       :
        pRndisMessage->NdisMessageType == REMOTE_NDIS_QUERY_MSG           ? 
                L"RNDIS_QUERY"      :
        pRndisMessage->NdisMessageType == REMOTE_NDIS_SET_MSG             ?
                L"RNDIS_SET"        :
        pRndisMessage->NdisMessageType == REMOTE_NDIS_RESET_MSG           ? 
                L"RNDIS_RESET"      :
        pRndisMessage->NdisMessageType == REMOTE_NDIS_PACKET_MSG          ? 
                L"RNDIS_PACKET"   :
        pRndisMessage->NdisMessageType == REMOTE_NDIS_INDICATE_STATUS_MSG ? 
                L"RNDIS_INDICATE"   :
        pRndisMessage->NdisMessageType == REMOTE_NDIS_KEEPALIVE_MSG       ? 
                L"RNDIS_KEEPALIVE"  :      
        pRndisMessage->NdisMessageType == REMOTE_NDIS_PACKET_MSG          ? 
                L"RNDIS_DATA_PACKET"   :
                L"UNKNOWN!",
        pRndisMessage->MessageLength));

    // we only queue one rndis message and response
    // Hence drop this message if last response has not been sent yet
    // do not drop REMOTE_NDIS_INITIALIZE_MSG & REMOTE_NDIS_RESET_MSG
    //
    if (pRndisMessage->NdisMessageType!= REMOTE_NDIS_INITIALIZE_MSG &&
        pRndisMessage->NdisMessageType!= REMOTE_NDIS_RESET_MSG &&
        !UsbRndis_RndisMsgSent())
    {
        USBDBGMSG(USBDBG_ZONE_WARN, (L"RNdis:: Dropping RndisMessageType=%d\r\n", pRndisMessage->NdisMessageType));
        return;
    }
    
    switch (pRndisMessage->NdisMessageType)
    {
        case REMOTE_NDIS_PACKET_MSG:
            USBDBGMSG(USBDBG_ZONE_ERROR, (L"ERROR: RNdis:: Current codepath not expecting REMOTE_NDIS_PACKET_MSG\r\n"));
            break;
            
        case REMOTE_NDIS_INITIALIZE_MSG:
        {
            PRNDIS_INITIALIZE_REQUEST   pInitializeRequest;
            NDIS_STATUS                 NdisStatus;

            pInitializeRequest = &pRndisMessage->Message.InitializeRequest;

            USBDBGMSG (USBDBG_ZONE_VERBOSE,
                (L"RNdis:: ReqID[0x%x] - Ver[%d-%d] - MaxXfer[%d]\r\n",
                pInitializeRequest->RequestId,
                pInitializeRequest->MajorVersion,
                pInitializeRequest->MinorVersion,
                pInitializeRequest->MaxTransferSize));

            //  We support SUPPORTED_RNDIS_MAJOR_VERSION, so bail out if 
            //  it's not.           
            if (pInitializeRequest->MajorVersion > SUPPORTED_RNDIS_MAJOR_VER  ||
                (pInitializeRequest->MajorVersion == SUPPORTED_RNDIS_MAJOR_VER &&
                 pInitializeRequest->MinorVersion > SUPPORTED_RNDIS_MINOR_VER))
            {
                USBDBGMSG (USBDBG_ZONE_ERROR,
                    (L"RNdisMini Err!! unsupported RNDIS host ver.\r\n"));
                NdisStatus = NDIS_STATUS_FAILURE;
            }
            else
            {
                NdisStatus = NDIS_STATUS_SUCCESS;               
            }

            //  Send the reply out..
            //
            USBDBGMSG (USBDBG_ZONE_VERBOSE, 
                (TEXT("RNdis:: Send InitializeComplete Status[0x%x] ID[0x%x]\r\n"),
                NdisStatus,
                pInitializeRequest->RequestId));
            
            SendRndisMessage(
                REMOTE_NDIS_INITIALIZE_CMPLT,
                pInitializeRequest->RequestId,                  
                NdisStatus,
                NULL,
                0x00,
                0x00);
            
            m_rndisDev.state = RNDIS_INITIALIZED;

            break;
        }

        case REMOTE_NDIS_QUERY_MSG:
        {
            PRNDIS_QUERY_REQUEST    pRndisRequest;
            NDIS_STATUS             NdisStatus;         
            ULONG                   ulBytesWritten;
            ULONG                   ulBytesNeeded;
            UCHAR                   *pucBuffer;


            if(m_rndisDev.state == RNDIS_UNINITIALIZED)
                break;

            pRndisRequest = &pRndisMessage->Message.QueryRequest;

            USBDBGMSG(USBDBG_ZONE_VERBOSE,
                (TEXT("RNdis:: Query: ID[0x%x]:OID[0x%x]:Buff[%d-%d]\r\n"),
                pRndisRequest->RequestId,
                pRndisRequest->Oid,
                pRndisRequest->InformationBufferLength,
                pRndisRequest->InformationBufferOffset));           

            //  Special treatment for this OID.
            //
            if (pRndisRequest->Oid == OID_GEN_SUPPORTED_LIST)
            {
                pucBuffer = (PUCHAR)RNdisMddSupportedOids;
                
                HostMiniQueryInformation(pRndisRequest->Oid, NULL, 0x00,
                        &ulBytesWritten, &ulBytesNeeded);

                ulBytesWritten  = ulBytesNeeded;
                ulBytesNeeded   = 0x00;
                NdisStatus      = NDIS_STATUS_SUCCESS;
            }
            else
            {
                //  Pass this to our miniport handler..
                //
                pucBuffer  = (PUCHAR)((PUCHAR)pRndisRequest + 
                            pRndisRequest->InformationBufferOffset);

                NdisStatus = HostMiniQueryInformation(pRndisRequest->Oid,
                        pucBuffer,  pRndisRequest->InformationBufferLength,
                        &ulBytesWritten, &ulBytesNeeded);
                
            }


            //  Reply back to host..
            SendRndisMessage(REMOTE_NDIS_QUERY_CMPLT, pRndisRequest->RequestId,
                NdisStatus, pucBuffer, ulBytesWritten,
                ulBytesWritten ? ulBytesWritten : ulBytesNeeded);


            break;
        }   

        case REMOTE_NDIS_SET_MSG:
        {
            PRNDIS_SET_REQUEST      pRndisSet;
            NDIS_STATUS             NdisStatus;         
            ULONG                   ulBytesRead;
            ULONG                   ulBytesNeeded;
            UCHAR                   *pucBuffer;

            if (m_rndisDev.state == RNDIS_UNINITIALIZED)
                break;

            pRndisSet = &pRndisMessage->Message.SetRequest;
            pucBuffer  = (PUCHAR)((PUCHAR)pRndisSet + pRndisSet->InformationBufferOffset);
            NdisStatus = HostMiniSetInformation(
                            pRndisSet->Oid,
                            pucBuffer,
                            pRndisSet->InformationBufferLength,
                            &ulBytesRead,
                            &ulBytesNeeded);

            //  Reply back to host on the status..
            SendRndisMessage(
                REMOTE_NDIS_SET_CMPLT,
                pRndisSet->RequestId,
                NdisStatus,
                NULL,
                0,
                0);

            break;
        }       

        case REMOTE_NDIS_KEEPALIVE_MSG:
        {
            PRNDIS_KEEPALIVE_REQUEST    pKeepAliveRequest;

            if(m_rndisDev.state == RNDIS_UNINITIALIZED)
                break;

            USBDBGMSG (USBDBG_ZONE_VERBOSE, (TEXT("RNdis:: REMOTE_NDIS_KEEPALIVE_MSG.\r\n")));

            pKeepAliveRequest = &pRndisMessage->Message.KeepaliveRequest;

            //  [todo] DISCONNECTION HANDLING
            //  We probably need to keep track of this to detect 
            //  device being disconnected..

            //  We are here host!!!
            SendRndisMessage(
                REMOTE_NDIS_KEEPALIVE_CMPLT,
                pKeepAliveRequest->RequestId,
                RNDIS_STATUS_SUCCESS,
                NULL,
                0,
                0);
        
            break;
        }       

        case REMOTE_NDIS_HALT_MSG:
        {
            if(m_rndisDev.state == RNDIS_UNINITIALIZED)
                break;

            USBDBGMSG(USBDBG_ZONE_WARN, (L"RNdis:: recv REMOTE_NDIS_HALT_MSG from host!\r\n"));

            break;
        }
        
        case REMOTE_NDIS_RESET_MSG:
        {
            if(m_rndisDev.state == RNDIS_UNINITIALIZED)
                break;          

            USBDBGMSG(USBDBG_ZONE_WARN, (L"RNdis:: rcv REMOTE_NDIS_RESET_MSG from host!\r\n"));

#if 0
            LogCallStack();
#endif
            //  Per RNDIS spec discard all outstanding messages.
            //  Since we don't queue RNDIS messages just reply with reset complete.

            //  Reply this message.
            SendRndisMessage(
                REMOTE_NDIS_RESET_CMPLT,
                0x00,
                0x00,
                NULL,
                0,
                0);

            break;
        }
        
        case REMOTE_NDIS_INDICATE_STATUS_MSG:
        {
            if(m_rndisDev.state == RNDIS_UNINITIALIZED)
                break;      
            break;      
        }
        
        default:
            USBDBGMSG(USBDBG_ZONE_ERROR, (
                L"USBDBGRNDIS:: Unknown RNDIS msg, buff[0x%x] l=[%d]!\r\n",
                pbData, cbData
                ));
            break;
    }   

}

DWORD
RndisPdd_ProcessRecvdData(
    PBYTE pbSourceBuf,
    DWORD cbSourceBufLen,
    PBYTE pbTargetBuf,
    DWORD cbTargetBufLen
    )
{
    DWORD cbRecvdData = 0;
    PRNDIS_MESSAGE pRndisMessage = (PRNDIS_MESSAGE) pbSourceBuf;
    PRNDIS_PACKET pRndisPacket =RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pRndisMessage);
    UINT32 rndisDataOffset = 
        (UINT32)GET_PTR_TO_RNDIS_DATA_BUFF(pRndisPacket) -
        (UINT32)pRndisMessage;

#ifdef SHIP_BUILD
    UNREFERENCED_PARAMETER(cbSourceBufLen);
#endif
    UNREFERENCED_PARAMETER(cbTargetBufLen);
    if (pRndisMessage->NdisMessageType != REMOTE_NDIS_PACKET_MSG)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (
            L"ERROR:usbdbg:REMOTE_NDIS_PACKET_MSG not recvd t=%d, len=%d",
            pRndisMessage->NdisMessageType, cbSourceBufLen));
        goto clean;
    }

    cbRecvdData = pRndisPacket->DataLength;
    
    // copy data payload to target buffer
    memcpy(pbTargetBuf,
           pbSourceBuf + rndisDataOffset,
           pRndisPacket->DataLength);


    
clean:           
    return cbRecvdData;                
}

void
RndisPdd_SetRndisState(
    BOOL notInit
    )
{
    if (notInit)
    {
        // set current packet filter to 0.
        m_rndisDev.dwCurrentPacketFilter = 0;
        // state to uninit
        m_rndisDev.state = RNDIS_UNINITIALIZED;
    }
}
