/********************************************************************************
 * 
 * $Id: driver.cpp,v 1.1.1.1 2007/04/16 03:45:52 bill Exp $
 *
 * File: Main.cpp
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/


#include	"common.h"
#include	"driver.h"
#include	"device.h"

/****************************************************************************************
 *
 * Driver-wide global data declaration
 *
 ****************************************************************************************/

NDIS_OID gszNICSupportedOid[] = {
		OID_GEN_SUPPORTED_LIST,
        OID_GEN_HARDWARE_STATUS,
        OID_GEN_MEDIA_SUPPORTED,
        OID_GEN_MEDIA_IN_USE,
        OID_GEN_MEDIA_CONNECT_STATUS,
        OID_GEN_MAXIMUM_SEND_PACKETS,
        OID_GEN_VENDOR_DRIVER_VERSION,
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
        OID_GEN_VENDOR_ID,
        OID_GEN_VENDOR_DESCRIPTION,
        OID_GEN_CURRENT_PACKET_FILTER,
        OID_GEN_CURRENT_LOOKAHEAD,
        OID_GEN_DRIVER_VERSION,
        OID_GEN_XMIT_OK,
        OID_GEN_RCV_OK,
        OID_GEN_XMIT_ERROR,
        OID_GEN_RCV_ERROR,
        OID_GEN_RCV_NO_BUFFER,
        OID_GEN_RCV_CRC_ERROR,
        OID_802_3_PERMANENT_ADDRESS,
        OID_802_3_CURRENT_ADDRESS,
        OID_802_3_MULTICAST_LIST,
        OID_802_3_MAXIMUM_LIST_SIZE,
        OID_802_3_RCV_ERROR_ALIGNMENT,
        OID_802_3_XMIT_ONE_COLLISION,
        OID_802_3_XMIT_MORE_COLLISIONS,
        OID_802_3_XMIT_DEFERRED,
        OID_802_3_XMIT_MAX_COLLISIONS,
        OID_802_3_XMIT_UNDERRUN,
        OID_802_3_XMIT_HEARTBEAT_FAILURE,
        OID_802_3_XMIT_TIMES_CRS_LOST,
        OID_802_3_XMIT_LATE_COLLISIONS,
    };

/********************************************************************************************
 *
 * NIC_DRIVER_OBJECT Implementation
 *
 ********************************************************************************************/
void NIC_DRIVER_OBJECT::DriverStart(void)
{
	m_pLower->DeviceStart();
}

void NIC_DRIVER_OBJECT::EDriverInitialize(
		OUT PNDIS_STATUS OpenErrorStatus,
		OUT PUINT SelectedMediaIndex, 
		IN PNDIS_MEDIUM MediaArray, 
		IN UINT MediaArraySize)
{
	
	m_uRecentInterruptStatus = 0;
	
	if(!m_pLower)
		m_pLower = DeviceEntry(this,NULL);
		
	if(!m_pLower)
			THROW((ERR_STRING("Error in creating device")));
	
	UINT		i;
	NDIS_STATUS		status;
	NDIS_HANDLE		hconfig;

	// Determinate media type
	for(i=0; i<MediaArraySize; i++) 
		if(MediaArray[i] == NdisMedium802_3)	break;

    if (i == MediaArraySize) 
    	THROW((ERR_STRING("Unsupported media"),NDIS_STATUS_UNSUPPORTED_MEDIA));
		

	*SelectedMediaIndex = i;

	// Read registry configurations
	NdisOpenConfiguration(
		&status,
		&hconfig,
		m_NdisWrapper);

	if(status != NDIS_STATUS_SUCCESS) 
		THROW((ERR_STRING("Error in opening configuration"),status));

	C_Exception	*pexp;	
	TRY
	{
		m_pLower->DeviceSetDefaultSettings();
		m_pLower->DeviceSetEepromFormat();	
		m_pLower->DeviceRetriveConfigurations(hconfig);
		m_pLower->EDeviceValidateConfigurations();

		FI;
	}
	CATCH(pexp)
	{
		pexp->PrintErrorMessage();
		CLEAN(pexp);
		NdisCloseConfiguration(hconfig);
		THROW((ERR_STRING("*** Error in retriving configurations.\n")));
	}

	NdisCloseConfiguration(hconfig);

	m_pLower->DeviceRegisterAdapter();
	
	/* init tx buffers */
	U32		m,uaddr;
	if(!(uaddr = (U32)malloc(sizeof(DATA_BLOCK)*
		(m=m_pLower->m_szConfigures[CID_TXBUFFER_NUMBER]*2)))) 
		THROW((ERR_STRING("Insufficient memory")));

	for(;m--;uaddr+=sizeof(DATA_BLOCK))
		m_TQueue.Enqueue((PCQUEUE_GEN_HEADER)uaddr);

	TRY
	{
		m_pLower->EDeviceRegisterIoSpace();

//		m_pLower->EDeviceLoadEeprom();

		m_pLower->EDeviceInitialize(m_pLower->m_nResetCounts=0);

		m_pLower->EDeviceRegisterInterrupt();
		
		FI;
	}
	CATCH(pexp)
	{
		pexp->PrintErrorMessage();
		CLEAN(pexp);
		THROW((ERR_STRING("Device error")));
	}

	
	m_pLower->DeviceOnSetupFilter(0);

}

PVOID NIC_DRIVER_OBJECT::DriverBindAddress(
	U32		uPhysicalAddress,
	U32		uLength)
{
	void	*pvoid;
	// allocate memory for the work space
	
    if(!(pvoid = VirtualAlloc(
		NULL, 
		uLength,
		MEM_RESERVE, 
		PAGE_NOACCESS))) return NULL;

	/* binding this virtual addr to physical location */
	VirtualCopy(
		pvoid,
		(PVOID)uPhysicalAddress,
		uLength,
		PAGE_READWRITE | PAGE_NOCACHE);

	return pvoid;
}


void	NIC_DRIVER_OBJECT::DriverIndication(
	U32		uIndication)
{
	switch (uIndication)
	{
		case NIC_IND_TX_IDLE:
			if(m_bOutofResources)
			{
				m_bOutofResources = 0;
				NdisMSendResourcesAvailable(m_NdisHandle);
			}
			break;
			
		case AID_ERROR:
		case AID_LARGE_INCOME_PACKET:
			m_bSystemHang = 1;
		default:
			break;
	} // of switch
	
}

void NIC_DRIVER_OBJECT::DriverIsr(
		OUT PBOOLEAN InterruptRecognized, 
		OUT PBOOLEAN QueueInterrupt)		
{
	m_pLower->DeviceDisableInterrupt();

	U32	intstat;

	if(!(intstat = m_pLower->DeviceGetInterruptStatus()))
	{
		*InterruptRecognized =
		*QueueInterrupt = FALSE;
		m_pLower->DeviceEnableInterrupt();
		return;
	}

	/* clear it immediately */
	m_pLower->DeviceSetInterruptStatus(
		m_uRecentInterruptStatus = intstat);
		
	*InterruptRecognized = TRUE;
	*QueueInterrupt = TRUE;

#ifdef	IMPL_DEVICE_ISR
	m_pLower->DeviceIsr(intstat);
#endif

}

void	NIC_DRIVER_OBJECT::DriverInterruptHandler(void)
{
	
	
	m_pLower->DeviceInterruptEventHandler(m_uRecentInterruptStatus);

	m_pLower->DeviceEnableInterrupt();
	
}

#define	HANDLE_QUERY(event,ptr,len)	\
	case event: if(InfoBufferLength < (*BytesNeeded=len)) \
		{ status = NDIS_STATUS_INVALID_LENGTH; break; }	\
	panswer = (void*)(ptr); *BytesWritten = len; break;


NDIS_STATUS NIC_DRIVER_OBJECT::DriverQueryInformation(
		IN NDIS_OID		Oid,
		IN PVOID		InfoBuffer, 
		IN ULONG		InfoBufferLength, 
		OUT PULONG		BytesWritten,
		OUT PULONG		BytesNeeded)
{
	NDIS_STATUS	status = NDIS_STATUS_SUCCESS;
	
	PVOID	panswer;
	U8		szbuffer[32];
	U32		tmp32;
	
	// pass to lower object, to see if it can handle this query,
	// if it can, return TRUE and set status.
	if(m_pLower->DeviceQueryInformation(
		&status,
		Oid,
		InfoBuffer,
		InfoBufferLength,
		BytesWritten,
		BytesNeeded)) return status;
		
	switch (Oid) {
		HANDLE_QUERY( OID_GEN_SUPPORTED_LIST,
			&gszNICSupportedOid,sizeof(gszNICSupportedOid));
			
		HANDLE_QUERY( OID_GEN_HARDWARE_STATUS,
			&m_pLower->m_szCurrentSettings[SID_HW_STATUS],sizeof(U32));
			
		HANDLE_QUERY( OID_GEN_MEDIA_IN_USE,
			&m_pLower->m_szCurrentSettings[SID_MEDIA_IN_USE],sizeof(U32));

		HANDLE_QUERY( OID_GEN_MEDIA_SUPPORTED,
			&m_pLower->m_szCurrentSettings[SID_MEDIA_SUPPORTED],sizeof(U32));
			
		HANDLE_QUERY( OID_GEN_MEDIA_CONNECT_STATUS,
			&m_pLower->m_szCurrentSettings[SID_MEDIA_CONNECTION_STATUS],sizeof(U32));

		HANDLE_QUERY( OID_GEN_MAXIMUM_LOOKAHEAD,
			&m_pLower->m_szCurrentSettings[SID_MAXIMUM_LOOKAHEAD],sizeof(U32));

		HANDLE_QUERY( OID_GEN_MAXIMUM_FRAME_SIZE,
 			&m_pLower->m_szCurrentSettings[SID_MAXIMUM_FRAME_SIZE],sizeof(U32));
       
		HANDLE_QUERY( OID_GEN_MAXIMUM_TOTAL_SIZE,
 			&m_pLower->m_szCurrentSettings[SID_MAXIMUM_TOTAL_SIZE],sizeof(U32));
        
		HANDLE_QUERY( OID_GEN_MAXIMUM_SEND_PACKETS,
 			&m_pLower->m_szCurrentSettings[SID_MAXIMUM_SEND_PACKETS],sizeof(U32));
        
		HANDLE_QUERY( OID_GEN_LINK_SPEED,
 			&m_pLower->m_szCurrentSettings[SID_LINK_SPEED],sizeof(U32));

		HANDLE_QUERY( OID_GEN_XMIT_OK,
			&m_pLower->m_szStatistics[TID_GEN_XMIT_OK],sizeof(U32));
		HANDLE_QUERY( OID_GEN_RCV_OK,
			&m_pLower->m_szStatistics[TID_GEN_RCV_OK],sizeof(U32));
		HANDLE_QUERY( OID_GEN_XMIT_ERROR,
			&m_pLower->m_szStatistics[TID_GEN_XMIT_ERROR],sizeof(U32));
		HANDLE_QUERY( OID_GEN_RCV_ERROR,
			&m_pLower->m_szStatistics[TID_GEN_RCV_ERROR],sizeof(U32));
		HANDLE_QUERY( OID_GEN_RCV_NO_BUFFER,
			&m_pLower->m_szStatistics[TID_GEN_RCV_NO_BUFFER],sizeof(U32));
		HANDLE_QUERY( OID_GEN_RCV_CRC_ERROR,
			&m_pLower->m_szStatistics[TID_GEN_RCV_CRC_ERROR],sizeof(U32));

		HANDLE_QUERY( OID_802_3_RCV_ERROR_ALIGNMENT,
			&m_pLower->m_szStatistics[TID_802_3_RCV_ERROR_ALIGNMENT],sizeof(U32));			
        HANDLE_QUERY( OID_802_3_RCV_OVERRUN,
			&m_pLower->m_szStatistics[TID_802_3_RCV_OVERRUN],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_ONE_COLLISION,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_ONE_COLLISION],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_MORE_COLLISIONS,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_MORE_COLLISIONS],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_DEFERRED,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_DEFERRED],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_MAX_COLLISIONS,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_MAX_COLLISIONS],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_UNDERRUN,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_UNDERRUN],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_HEARTBEAT_FAILURE,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_HEARTBEAT_FAILURE],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_TIMES_CRS_LOST,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_TIMES_CRS_LOST],sizeof(U32));
		HANDLE_QUERY( OID_802_3_XMIT_LATE_COLLISIONS,
			&m_pLower->m_szStatistics[TID_802_3_XMIT_LATE_COLLISIONS],sizeof(U32));


		HANDLE_QUERY( OID_GEN_MAC_OPTIONS,
 			&m_pLower->m_szCurrentSettings[SID_GEN_MAC_OPTIONS],sizeof(U32));

		HANDLE_QUERY( OID_802_3_PERMANENT_ADDRESS,
			m_pLower->DeviceMacAddress(&szbuffer[0]),ETH_ADDRESS_LENGTH);
		HANDLE_QUERY( OID_802_3_CURRENT_ADDRESS,
			m_pLower->DeviceMacAddress(&szbuffer[0]),ETH_ADDRESS_LENGTH);

		HANDLE_QUERY( OID_802_3_MAXIMUM_LIST_SIZE,
 			&m_pLower->m_szCurrentSettings[SID_802_3_MAXIMUM_LIST_SIZE],sizeof(U32));
    
		HANDLE_QUERY( OID_802_3_MULTICAST_LIST,
 			&m_pLower->m_szMulticastList[0][0],
 			m_pLower->m_nMulticasts*ETH_ADDRESS_LENGTH);

		HANDLE_QUERY( OID_GEN_CURRENT_PACKET_FILTER,
 			&m_pLower->m_szCurrentSettings[SID_GEN_CURRENT_PACKET_FILTER],sizeof(U32));

		HANDLE_QUERY( OID_GEN_TRANSMIT_BUFFER_SPACE,
 			&m_pLower->m_szCurrentSettings[SID_GEN_TRANSMIT_BUFFER_SPACE],sizeof(U32));
		HANDLE_QUERY( OID_GEN_RECEIVE_BUFFER_SPACE,
 			&m_pLower->m_szCurrentSettings[SID_GEN_RECEIVE_BUFFER_SPACE],sizeof(U32));

		HANDLE_QUERY( OID_GEN_TRANSMIT_BLOCK_SIZE,
 			&m_pLower->m_szCurrentSettings[SID_GEN_TRANSMIT_BLOCK_SIZE],sizeof(U32));
		HANDLE_QUERY( OID_GEN_RECEIVE_BLOCK_SIZE,
 			&m_pLower->m_szCurrentSettings[SID_GEN_RECEIVE_BLOCK_SIZE],sizeof(U32));
		
		HANDLE_QUERY( OID_GEN_VENDOR_ID,
 			(tmp32=(U32)m_pLower->DeviceVendorID(),&tmp32),sizeof(U32));
		
		HANDLE_QUERY( OID_GEN_VENDOR_DESCRIPTION,
 			VENDOR_DESC,strlen(VENDOR_DESC));
		
		HANDLE_QUERY( OID_GEN_CURRENT_LOOKAHEAD,
 			&m_pLower->m_szCurrentSettings[SID_GEN_CURRENT_LOOKAHEAD],sizeof(U32));
		HANDLE_QUERY( OID_GEN_DRIVER_VERSION,
 			&m_pLower->m_szCurrentSettings[SID_GEN_DRIVER_VERSION],sizeof(U32));
		HANDLE_QUERY( OID_GEN_VENDOR_DRIVER_VERSION,
 			&m_pLower->m_szCurrentSettings[SID_GEN_VENDOR_DRIVER_VERSION],sizeof(U32));
		HANDLE_QUERY( OID_GEN_PROTOCOL_OPTIONS,
 			&m_pLower->m_szCurrentSettings[SID_GEN_PROTOCOL_OPTIONS],sizeof(U32));
			
		default:
			status = NDIS_STATUS_INVALID_OID;
			break;
	} // of switch

	if(status == NDIS_STATUS_SUCCESS)
	{
		NdisMoveMemory(InfoBuffer,panswer,*BytesWritten);
	}

	return status;
}

#define	HANDLE_SET(event,len)	\
	case event: if(InfoBufferLength < (*BytesNeeded=len)) \
		{ status = NDIS_STATUS_INVALID_LENGTH; break; }	\
		*BytesRead = len; HANDLE_SET_##event(); break;

#define	HANDLE_SET_OID_GEN_CURRENT_PACKET_FILTER()	\
	m_pLower->DeviceOnSetupFilter(*(U32*)InfoBuffer)

	
NDIS_STATUS NIC_DRIVER_OBJECT::DriverSetInformation(
	IN NDIS_OID		Oid,
	IN PVOID		InfoBuffer, 
	IN ULONG		InfoBufferLength, 
	OUT PULONG		BytesRead,
	OUT PULONG		BytesNeeded)
{
	NDIS_STATUS	status = NDIS_STATUS_SUCCESS;

	// pass to lower object, to see if it can handle this request,
	// if it can, return TRUE and set status.
	if(m_pLower->DeviceSetInformation(
		&status,
		Oid,
		InfoBuffer,
		InfoBufferLength,
		BytesRead,
		BytesNeeded)) return status;

	switch (Oid)
	{
		HANDLE_SET( OID_GEN_CURRENT_PACKET_FILTER,sizeof(U32));
		
		case OID_802_3_MULTICAST_LIST:
			NdisMoveMemory(
				&m_pLower->m_szMulticastList[0][0],
				InfoBuffer,
				InfoBufferLength);
			m_pLower->m_nMulticasts = 
				InfoBufferLength / ETH_ADDRESS_LENGTH;
			m_pLower->DeviceOnSetupFilter(
				m_pLower->m_szCurrentSettings[SID_GEN_CURRENT_PACKET_FILTER]
				| NDIS_PACKET_TYPE_MULTICAST);
			break;

		// don't care oids
		case OID_GEN_CURRENT_LOOKAHEAD:
			break;

		case OID_GEN_NETWORK_LAYER_ADDRESSES:
		default:
			status = NDIS_STATUS_INVALID_OID;
		
	} // of switch Oid
	
	return status;
}

void	NIC_DRIVER_OBJECT::DriverEnableInterrupt(void)
{
	m_pLower->DeviceEnableInterrupt();
}

void	NIC_DRIVER_OBJECT::DriverDisableInterrupt(void)
{
	m_pLower->DeviceDisableInterrupt();
}

BOOL	NIC_DRIVER_OBJECT::DriverCheckForHang(void)
{
	if(m_bSystemHang) return TRUE;
	return m_pLower->DeviceCheckForHang();
}

VOID	NIC_DRIVER_OBJECT::DriverHalt(void)
{
	m_pLower->DeviceHalt();
}

NDIS_STATUS NIC_DRIVER_OBJECT::DriverReset(
	OUT PBOOLEAN	pbAddressingReset)
{
	// Reset activities
	// 1. Abort all current tx and rx.
	// 2. Cleanup waiting and standby queues.
	// 3. Re-init tx and rx descriptors.
	// 4. Softreset MAC, PHY and set registers angain.

	*pbAddressingReset = TRUE;
#ifndef	IMPL_RESET
	return	NDIS_STATUS_SUCCESS;
#endif

	m_pLower->DeviceReset();
	DriverStart();
	m_bSystemHang = 0;
	m_bOutofResources = 0;
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS	NIC_DRIVER_OBJECT::DriverSend(
	IN PNDIS_PACKET	pPacket,
	IN UINT			uFlags)
{
#if !defined(IMPL_TX_QUEUE)
	
	if(!m_pLower->DeviceQueryTxResources())
		return NDIS_STATUS_RESOURCES;
	
#endif

	PCQUEUE_GEN_HEADER	pobj;
	
	if(!(pobj = m_TQueue.Dequeue())) 
	{
		m_bOutofResources = 1;
		DEBUG_PRINT((TEXT("<DM9:m_bOutofResources\n")));
		return NDIS_STATUS_RESOURCES;
	}
	
	PNDIS_BUFFER	pndisFirstBuffer;
	UINT			uPhysicalBufferCount;
	UINT			uBufferCount;
	UINT			uTotalPacketLength;

	PNDIS_BUFFER	pndisCurrBuffer;
	PU8		pcurr = (PU8)CQueueGetUserPointer(pobj);

	PVOID	ptrBuffer;
	UINT	nBuffer;
	U32		idx,check;

	NdisQueryPacket(
		pPacket, 
		&uPhysicalBufferCount, 
		&uBufferCount,
		&pndisFirstBuffer,
        &uTotalPacketLength);

    if (uTotalPacketLength > ETH_MAX_FRAME_SIZE) {
        return NDIS_STATUS_FAILURE;
    }

	uPhysicalBufferCount &= 0xFFFF;

	for(idx=0,check=0,pndisCurrBuffer=pndisFirstBuffer;
		idx < uBufferCount;
		idx++, pndisCurrBuffer = pndisCurrBuffer->Next)
	{
		NdisQueryBuffer(
			pndisCurrBuffer,
			&ptrBuffer,
			&nBuffer);

		if(!nBuffer) continue;
		
		NdisMoveMemory(pcurr, ptrBuffer, nBuffer);
		pcurr += nBuffer;
		check += nBuffer;
        
	} // of for gathering buffer

	if(uTotalPacketLength != check) return NDIS_STATUS_FAILURE;


	pobj->pPacket = (PVOID)pPacket;
	pobj->uFlags = uFlags;
	pobj->nLength = uTotalPacketLength;	
	m_pLower->DeviceSend(pobj);
	
#ifdef	IMPL_SEND_INDICATION
	return NDIS_STATUS_PENDING;
#else
	return NDIS_STATUS_SUCCESS;
#endif


}

void	NIC_DRIVER_OBJECT::DriverReceiveIndication(
	int		nCurr,
	PVOID	pVoid,
	int		nLength)
{
	NdisMEthIndicateReceive(
		m_NdisHandle,
		(PNDIS_HANDLE)nCurr,
		(char*)pVoid,
		ETH_HEADER_SIZE,
		((char*)pVoid + ETH_HEADER_SIZE),
		nLength - ETH_HEADER_SIZE,
		nLength - ETH_HEADER_SIZE);
	
	NdisMEthIndicateReceiveComplete(m_NdisHandle);
	return;
}
	
void	NIC_DRIVER_OBJECT::DriverSendCompleted(
	PCQUEUE_GEN_HEADER	pObject)
{

	m_TQueue.Enqueue(pObject);
	
#ifdef	IMPL_SEND_INDICATION

	NdisMSendResourcesAvailable(m_NdisHandle);
	NdisMSendComplete(
				m_NdisHandle,
				(PNDIS_PACKET)(pObject->pPacket),
				NDIS_STATUS_SUCCESS);
#endif
}
	

/********************************************************************************************
 *
 * Trunk Functions
 *
 ********************************************************************************************/

#ifdef	__cplusplus
extern "C" {	// miniport driver trunk functions
#endif

NDIS_STATUS MiniportInitialize(
	OUT PNDIS_STATUS OpenErrorStatus,
	OUT PUINT SelectedMediaIndex, 
	IN PNDIS_MEDIUM MediaArray, 
	IN UINT MediaArraySize,
    IN NDIS_HANDLE MiniportHandle, 
	IN NDIS_HANDLE WrapperConfigHandle)
{
	
	PUTS(("<DM9:++MiniportIntialize>\n"));


	NIC_DRIVER_OBJECT	*pnic;

	if(!(pnic = new NIC_DRIVER_OBJECT(
		MiniportHandle,WrapperConfigHandle)))	
		return	NDIS_STATUS_FAILURE;

	C_Exception	*pexp;
	TRY
	{
		pnic->EDriverInitialize(
			OpenErrorStatus,
			SelectedMediaIndex, 
			MediaArray, 
			MediaArraySize);

		pnic->DriverStart();	

		FI;
	}
	CATCH(pexp)
	{
		pexp->PrintErrorMessage();
		CLEAN(pexp);
		delete pnic;
		return NDIS_STATUS_FAILURE;
	}
	
	PUTS(("<DM9:--MiniportInitialize>\n"));
	return NDIS_STATUS_SUCCESS;
}

void MiniportISRHandler(
		OUT PBOOLEAN InterruptRecognized, 
		OUT PBOOLEAN QueueInterrupt,
		IN  NDIS_HANDLE MiniportContext)
{
	((NIC_DRIVER_OBJECT*)MiniportContext)->DriverIsr(
		InterruptRecognized,
		QueueInterrupt);
}

VOID	MiniportInterruptHandler(
	IN NDIS_HANDLE  MiniportContext)
{
	((NIC_DRIVER_OBJECT*)MiniportContext)->DriverInterruptHandler();
}

NDIS_STATUS MiniportQueryInformation(
	IN NDIS_HANDLE	MiniportContext,
	IN NDIS_OID		Oid,
	IN PVOID		InfoBuffer, 
	IN ULONG		InfoBufferLength, 
	OUT PULONG		BytesWritten,
	OUT PULONG		BytesNeeded)
{
	return ((NIC_DRIVER_OBJECT*)MiniportContext)->DriverQueryInformation(
			Oid,
			InfoBuffer, 
			InfoBufferLength, 
			BytesWritten,
			BytesNeeded);
}

NDIS_STATUS MiniportSetInformation(
	IN NDIS_HANDLE	MiniportContext, 
	IN NDIS_OID		Oid,
	IN PVOID		InfoBuffer, 
	IN ULONG		InfoBufferLength, 
	OUT PULONG		BytesRead,
	OUT PULONG		BytesNeeded)
{
	return ((NIC_DRIVER_OBJECT*)MiniportContext)->DriverSetInformation(
			Oid,
			InfoBuffer, 
			InfoBufferLength, 
			BytesRead,
			BytesNeeded);
}

VOID	MiniportEnableInterrupt(
	IN NDIS_HANDLE  MiniportContext)
{
	((NIC_DRIVER_OBJECT*)MiniportContext)->DriverEnableInterrupt();
}

VOID	MiniportDisableInterrupt(
	IN NDIS_HANDLE  MiniportContext)
{
	((NIC_DRIVER_OBJECT*)MiniportContext)->DriverDisableInterrupt();
}

BOOLEAN	MiniportCheckForHang(
	IN NDIS_HANDLE  MiniportContext)
{
	return ((NIC_DRIVER_OBJECT*)MiniportContext)->DriverCheckForHang();
}
 
VOID	MiniportHalt(
	IN NDIS_HANDLE  MiniportContext)
{
	((NIC_DRIVER_OBJECT*)MiniportContext)->DriverHalt();
}
 

NDIS_STATUS MiniportReset(
    OUT PBOOLEAN  AddressingReset,
    IN NDIS_HANDLE  MiniportContext)
{
	return ((NIC_DRIVER_OBJECT*)MiniportContext)->DriverReset(AddressingReset);
}

NDIS_STATUS	MiniportSend(
	IN NDIS_HANDLE	MiniportContext,
	IN PNDIS_PACKET	Packet,
	IN UINT			Flags)
{
	return ((NIC_DRIVER_OBJECT*)MiniportContext)->DriverSend(Packet,Flags);
}



#ifdef	__cplusplus   
}	// of miniport trunk functions
#endif
/********************************************************************************************
 *
 * DriverEntry
 *
 ********************************************************************************************/

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT	pDriverObject, 
	IN PUNICODE_STRING	pRegistryPath)
{
	NDIS_STATUS		status;
	NDIS_HANDLE		hwrapper;
	NDIS40_MINIPORT_CHARACTERISTICS	ndischar;


	PUTS(("<Davicom DM9000/9000A/9010 driver v3.04 for WinCE 4.2/5.0/6.0>\r\n"));

	NdisMInitializeWrapper(
		&hwrapper, 
		pDriverObject,
		pRegistryPath,
		NULL);

	memset((void*)&ndischar,0,sizeof(ndischar));
    
    ndischar.Ndis30Chars.MajorNdisVersion = PRJ_NDIS_MAJOR_VERSION;
	ndischar.Ndis30Chars.MinorNdisVersion = PRJ_NDIS_MINOR_VERSION;
    
	ndischar.Ndis30Chars.InitializeHandler = MiniportInitialize;
    ndischar.Ndis30Chars.ResetHandler      = MiniportReset;
    ndischar.Ndis30Chars.CheckForHangHandler = MiniportCheckForHang;
    ndischar.Ndis30Chars.HaltHandler         = MiniportHalt;
    ndischar.Ndis30Chars.HandleInterruptHandler   = MiniportInterruptHandler;
    ndischar.Ndis30Chars.ISRHandler               = MiniportISRHandler;
    ndischar.Ndis30Chars.QueryInformationHandler  = MiniportQueryInformation;
    ndischar.Ndis30Chars.SetInformationHandler	  = MiniportSetInformation;
    ndischar.Ndis30Chars.SendHandler              = MiniportSend;


	if((status = NdisMRegisterMiniport(
		hwrapper,
		(PNDIS_MINIPORT_CHARACTERISTICS)&ndischar,
		sizeof(ndischar)) != NDIS_STATUS_SUCCESS))
	{
		NdisTerminateWrapper(hwrapper,NULL);
		return status;
	}


#ifndef	IMPL_DLL_ENTRY	
	INIT_EXCEPTION();
#endif

    return NDIS_STATUS_SUCCESS;

}

