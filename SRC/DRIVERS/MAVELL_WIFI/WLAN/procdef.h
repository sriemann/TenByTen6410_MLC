/******************* (c) Marvell Semiconductor, Inc., *************************
 *
 *          Purpose:
 *
 *          This file contains the function prototypes for NDIS miniport event handlers
 *
 *  Notes:
 *		    (1) The following NDIS_MINIPORT_CHARACTERISTICS event handlers are 
 *			    declared as inline functions.
 *				1. MrvDrvDisableInterrupt() is declared for DisableInterruptHandler
 *				2. MrvDrvEnableInterrupt() is decalred for EnableInterruptHandler
 *
 *		    (2) The following NDIS_MINIPORT_CHARACTERISTICS event handlers are not
 *			    implemented.
 *				1. ReconfigureHandler
 *				2. SendHandler
 *				3. TransferDataHandler
 *
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/08/23 $
 *
 *	$Revision: #2 $
 *  
 *****************************************************************************/

#ifndef _PROCDEF_H_
#define _PROCDEF_H_

#include "If.h"

//===============================================================================
//          PUBLIC PROCEDURES
//===============================================================================

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	);

BOOLEAN
MrvDrvCheckForHang(
	IN NDIS_HANDLE MiniportAdapterContext
	);

VOID
MrvDrvHalt(
	IN  NDIS_HANDLE MiniportAdapterContext
	);

NDIS_STATUS
MrvDrvInitialize(
	OUT PNDIS_STATUS OpenErrorStatus,
	OUT PUINT SelectedMediumIndex,
	IN PNDIS_MEDIUM MediumArray,
	IN UINT MediumArraySize,
	IN NDIS_HANDLE MiniportAdapterHandle,
	IN NDIS_HANDLE WrapperConfigurationContext
	);

VOID
MrvDrvReturnPacket(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN PNDIS_PACKET Packet);

NDIS_STATUS
MrvDrvReset(
	OUT PBOOLEAN AddressingReset,
	IN  NDIS_HANDLE MiniportAdapterContext
	);

VOID
MrvDrvAllocateComplete(NDIS_HANDLE MiniportAdapterContext,
	IN PVOID VirtualAddress,
	IN PNDIS_PHYSICAL_ADDRESS PhysicalAddress,
	IN ULONG Length,
	IN PVOID Context
	);

//VOID  dralee 20050802
//MrvDrvIsr(
//	OUT PBOOLEAN InterruptRecognized,
//	OUT PBOOLEAN QueueMiniportHandleInterrupt,
//	IN NDIS_HANDLE MiniportAdapterContext
//	);

VOID
MrvDrvHandleInterrupt(
	IN NDIS_HANDLE MiniportAdapterContext
	);

VOID
MrvDrvDisableInterrupt(
	IN NDIS_HANDLE MiniportAdapterContext
	);

VOID
MrvDrvEnableInterrupt(
	IN NDIS_HANDLE MiniportAdapterContext
	);

NDIS_STATUS
MrvDrvSetInformation(
	IN NDIS_HANDLE MiniportAdapterContext,
	IN NDIS_OID Oid,
	IN PVOID InformationBuffer,
	IN ULONG InformationBufferLength,
	OUT PULONG BytesRead,
	OUT PULONG BytesNeeded
	);

NDIS_STATUS
MrvDrvQueryInformation(
	IN NDIS_HANDLE MiniportAdapterContext,
	IN NDIS_OID Oid,
	IN PVOID InformationBuffer,
	IN ULONG InformationBufferLength,
	OUT PULONG BytesWritten,
	OUT PULONG BytesNeeded
	);

 

NDIS_STATUS
MrvDrvSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags); 
    
VOID    
MrvDrvTxPktTimerFunction(
	IN PVOID SystemSpecific1,
	IN NDIS_HANDLE MiniportAdapterContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	); 


VOID MrvDrvCheckTxQueueTimerFunction(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);

VOID MrvDrvPSCheckTxReadyTimerFunction(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);

VOID MrvDrvReConnectTimerFunction(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);


// tt ++ mic 2
VOID MrvMicDisconnectTimerFunction(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);
// tt -- mic 2

VOID MrvfrThread(IN PVOID pContext);

VOID
MrvDrvSendPackets(
	IN  NDIS_HANDLE             MiniportAdapterContext,
	IN  PPNDIS_PACKET           PacketArray,
	IN  UINT                    NumberOfPackets
	);

VOID
MrvDrvShutdownHandler(
	IN  NDIS_HANDLE MiniportAdapterContext
	);

VOID	
MrvDrvPMCallback(
    IN struct _NDIS_WORK_ITEM *pWorkItem,
    IN PVOID  Context
	);
	
VOID
MrvDrvIndicateConnectStatusTimer(
	IN PVOID SystemSpecific1,
	IN NDIS_HANDLE MiniportAdapterContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);

VOID
MrvDrvAvoidScanAfterConnectedTimer(
	IN PVOID SystemSpecific1,
	IN NDIS_HANDLE MiniportAdapterContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);

VOID
MrvDrvCommandTimerFunction(
	IN PVOID SystemSpecific1,
	IN NDIS_HANDLE MiniportAdapterContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);

VOID
MrvDrvPnPEventNotify(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_DEVICE_PNP_EVENT  PnPEvent,
    IN PVOID  InformationBuffer,
    IN ULONG  InformationBufferLength
    );


VOID
MrvDrvCancelSendPackets(
	IN  NDIS_HANDLE MiniportAdapterContext,
    IN  PVOID       GroupCancelId
	);


void MrvIstThread(IN PVOID pContext);


// interrupt timer
VOID MrvDrvSdioIntTimerHandler(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);

VOID MrvDrvSdioCheckFWReadyTimerFunction(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3
	);


VOID SDNdisSlotEventCallBack(SD_DEVICE_HANDLE    hDevice,
                             PVOID               pContext,                      
                             SD_SLOT_EVENT_TYPE  SlotEventType,         
                             PVOID               pData,                      
                             DWORD               DataLength);


///////////////////////////////////////////////////////////////////
//  SDNdisInterruptCallback - interrupt handler
//  Input:  hDevice - handle to the SDIO device
//          pAdapter - adapter context
//  Output: 
//  Returns:  SD_API_STATUS code
//  Notes:  
///////////////////////////////////////////////////////////////////
SD_API_STATUS SDNdisInterruptCallback(SD_DEVICE_HANDLE hDevice, 
                                      PVOID   pAdapter);

///////////////////////////////////////////////////////////////////
//  SDIODownloadPkt - download a SDIO paket to FW
//  Input:  Adapter - Adapter context
//          DownloadPkt - Pkt to be downloaded
//  Output: 
//  Returns:  SD_API_STATUS code
//  Notes:  
///////////////////////////////////////////////////////////////////
SD_API_STATUS SDIODownloadPkt(
    IN PVOID            pAdapter,
    IN PSDIO_TX_PKT      DownloadPkt
);

///////////////////////////////////////////////////////////////////
//  sdio_IsFirmwareLoaded - check if FW has been loaded/initialized
//  Input:  Adapter - Adapter context
//       
//  Output: 
//  Returns:  TRUE if the FW is loaded, FALSE otherwise
//  Notes:  
///////////////////////////////////////////////////////////////////
IF_FW_STATUS sdio_IsFirmwareLoaded( 
    IN PVOID Adapter
);

///////////////////////////////////////////////////////////////////
//  sdio_FirmwareDownload - download firmware
//  Input:  Adapter - Adapter context
//       
//  Output: 
//  Returns:  return SDIO_FW_STATUS
//  Notes:  
///////////////////////////////////////////////////////////////////

IF_FW_STATUS sdio_FirmwareDownload(
    IN PVOID Adapter
);


IF_API_STATUS SDIOGetPktTypeAndLength(PMRVDRV_ADAPTER pAdapter, 
                                      PUCHAR type, 
                                      PUCHAR mEvent, 
                                      PUSHORT usLength,
                                      PPVOID pRxBufVM);





// tt ++ re-assoc issue 060123
VOID DeleteBssidInPSList( PMRVDRV_ADAPTER pAdapter, UCHAR pBssid[] );
// tt -- re-assoc issue 060123

///pmkcache: bug#16956 ++
VOID
ReturnRxPacketDesc(PMRVDRV_ADAPTER Adapter,
                   PNDIS_PACKET    pPacket);
///pmkcache: bug#16956 --



#endif


