///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Marvell Corporation.  All rights reserved.
//
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT 
// WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
//
// Module Name:
//
//
//  SDNdis.h
//
// Abstract:
//
//  NDIS Adapater Header file
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////
#define  MINIPORT_SDNDIS_MAJOR_VERSION                0x04
#define  MINIPORT_SDNDIS_MINOR_VERSION                0x00

#define SDNDIS_REG_PATH TEXT("\\Comm\\SDNDis")
#include "bldver.h"

extern "C" {    
        // NDIS ddk header
#include <ndis.h>
}

#if (CE_MAJOR_VER >= 4)
#define NDIS_SUCCESS(Status) ((NDIS_STATUS)(Status) == NDIS_STATUS_SUCCESS)
#endif 


    // define the maximum size of frame size minus ethernet header
#define  SDNDIS_MAXIMUM_FRAME_SIZE         1500
#define  SDNDIS_ETHERNET_HEADER_SIZE       14
 
    // maximum total size, include the ethernet header
#define  SDNDIS_MAXIMUM_TOTAL_SIZE         SDNDIS_MAXIMUM_FRAME_SIZE +    \
                                            SDNDIS_ETHERNET_HEADER_SIZE

    // TODO change these values for the appropriate hardware configuration
    // for now we use some dummy values
#define  SDNDIS_LINK_SPEED                 100000
#define  SDNDIS_TRANSMIT_BUFFER_SPACE      SDNDIS_MAXIMUM_TOTAL_SIZE
#define  SDNDIS_TRANSMIT_BLOCK_SIZE        256
#define  SDNDIS_RECEIVE_BUFFER_SPACE       SDNDIS_MAXIMUM_TOTAL_SIZE
#define  SDNDIS_RECEIVE_BLOCK_SIZE         256
#define  SDNDIS_ETHERNET_ADDRESS_LENGTH    6
#define  SDNDIS_MAXIMUM_ETHERNET_LIST_SIZE 8

#define  SDNDIS_ZONE_SEND   SDCARD_ZONE_0
#define  ENABLE_ZONE_SEND   ZONE_ENABLE_0
#define  SDNDIS_ZONE_RCV    SDCARD_ZONE_1
#define  ENABLE_ZONE_RCV    ZONE_ENABLE_1

    // define the maximum size of look ahead buffer
    // Note: this definition is copied from the source code of
    // damini.h copied from Ne2000 for consistency purposes
#define  SDNDIS_MAXIMUM_LOOKAHEAD          252 - SDNDIS_ETHERNET_HEADER_SIZE

    // ethernet header structure
typedef struct _SDNDIS_ETHERNET_HEADER
{
    UCHAR   DestinationMACAddress[6];
    UCHAR   SourceMACAddress[6];
    USHORT  FrameType;
} SDNDIS_ETHERNET_HEADER, *PSDNDIS_ETHERNET_HEADER;

typedef struct _SDNDIS_ADAPTER* PSDNDIS_ADAPTER;

#define MAX_ACTIVE_REG_PATH 256

    // NDIS adapter context
typedef struct _SDNDIS_ADAPTER {
    NDIS_HANDLE MiniportAdapterHandle;  // handle passed to NDIS functions
    NDIS_HANDLE ConfigurationHandle;    // NDIS configuration handle
    ULONG MaxLookAhead;                 // max lookahead buffer size
    ULONG TransmitBufferSpace;          // transmit buffer space
    ULONG TransmitBlockSize;            // transmit block size
    ULONG ReceiveBufferSpace;           // receive buffer space
    ULONG ReceiveBlockSize;             // receive block size
    ULONG LinkSpeed;                    // hardware link speed
    ULONG FramesRcvGood;                // good receive frame count
    ULONG FramesXmitGood;               // good transmit frame count
    ULONG FramesXmitBad;                // bad transmit frame count
    ULONG FramesRcvBad;                 // bad receive frame count
    ULONG MissedPackets;                // missed packet count
    NDIS_SPIN_LOCK AdapterSpinLock;     // spinlock for protecting members of this adapter
    UCHAR EthernetPermanentAddress[SDNDIS_ETHERNET_ADDRESS_LENGTH];
    UCHAR EthernetCurrentAddress[SDNDIS_ETHERNET_ADDRESS_LENGTH];
    ULONG CurrentPacketFilter;          // current packet filter
    UCHAR MulticastList[SDNDIS_MAXIMUM_ETHERNET_LIST_SIZE]
                       [SDNDIS_ETHERNET_ADDRESS_LENGTH];
    ULONG PacketsCompleted;
    ULONG MulticastNumber;              // number of elements of size 
                                        // SDNDIS_ETHERNET_ADDRESS_LENGTH in 
                                        // multicast list
    ULONG          LinkStatus;          // link status
    SD_MEMORY_LIST_HANDLE hWriteLookasideList;
    BOOL             ShutDown;              // driver shutdown 
    SD_DEVICE_HANDLE hDevice;               // SD device handle
    PNDIS_PACKET     pCurrentXmitPacket;    // current packet
    WCHAR            ActivePath[MAX_ACTIVE_REG_PATH];   // adapter regpath
} SDNDIS_ADAPTER, *PSDNDIS_ADAPTER;


extern "C" {

    NDIS_STATUS SDNdisInitialize(
            OUT PNDIS_STATUS pOpenErrorStatus,
            OUT PUINT pSelectedMediumIndex,
            IN PNDIS_MEDIUM MediumArray,
            IN UINT MediumArraySize,
            IN NDIS_HANDLE MiniportAdapterHandle,
            IN NDIS_HANDLE ConfigurationHandle);

        // calls IOCTL_DGANAT_HALT
    VOID SDNdisHalt(
            IN NDIS_HANDLE MiniportAdapterContext);
        
        // calls IOCTL_DGANAT_QUERY_INFORMATION
    NDIS_STATUS SDNdisQueryInformation(
            IN NDIS_HANDLE MiniportAdapterContext,
            IN NDIS_OID Oid,
            IN PVOID pInfoBuffer,
            IN ULONG BytesLeft,
            OUT PULONG pBytesWritten,
            OUT PULONG pBytesNeeded);
        
        // calls IOCTL_DGANAT_RESET
    NDIS_STATUS SDNdisReset(
            OUT PBOOLEAN pAddressingReset,
            IN NDIS_HANDLE MiniportAdapterContext);

        // called to determine if the underlying drivers are responding
    BOOLEAN SDNdisCheckForHang(IN NDIS_HANDLE MiniportAdapterContext);

        // calls IOCTL_DGANAT_SET_INFORMATION
    NDIS_STATUS SDNdisSetInformation(
            IN NDIS_HANDLE MiniportAdapterContext,
            IN NDIS_OID Oid,
            IN PVOID pInformationBuffer,
            IN ULONG InformationBufferLength,
            OUT PULONG pBytesRead,
            OUT PULONG pBytesNeeded);

        // gets a name for an OID
    PCHAR GetOidName(IN NDIS_OID Oid);
        // gets pointer to DgaMiniportSupportedOids
    PVOID GetMiniportSupportedOids(PULONG pSize);
        // initialize adapter members
    NDIS_STATUS SDNdisInitializeAdapter(PSDNDIS_ADAPTER pAdapter);

        // generates IRP_MJ_WRITE request to NAT
    NDIS_STATUS SDNdisSend(
            IN NDIS_HANDLE MiniportAdapterContext,
            IN PNDIS_PACKET pPacket,
            IN UINT Flags);
        
    /*VOID SDNdisSlotEventCallBack(SD_DEVICE_HANDLE   hDevice,
                                 PVOID              pContext, 
                                 SD_SLOT_EVENT_TYPE Option,
                                 PVOID              pOptionData,
                                 DWORD              OptionSize);    */

    SD_API_STATUS SDNdisInterruptCallback(SD_DEVICE_HANDLE hDevice, 
                                          PSDNDIS_ADAPTER  pAdapter);

    NDIS_STATUS SDNdisGetSDDeviceHandle(PSDNDIS_ADAPTER pAdapter);

    BOOL LoaderEntry(HINSTANCE  hInstance,
                     ULONG      Reason,
                     LPVOID     pReserved);

} // extern "C"

