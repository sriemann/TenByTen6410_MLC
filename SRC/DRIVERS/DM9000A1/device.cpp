/********************************************************************************
 * 
 * $Id: device.cpp,v 1.2 2007/07/10 04:17:51 bill Exp $
 *
 * File: Device.cpp
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/

#include	"device.h"

/********************************************************************************
 *
 * CRC32 Routine
 *
 * This routine calculates the CRC based on the following polynominal
 *   x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1
 *
 * About the CRC value of one EEPROM
 *
 * eeprom<126-127> = CRC value = (U16)crc32(eeprom<0..125>);
 *
 *
 *
 *******************************************************************************/
#if LEGAL_PATENT
U32 g_szCRCTable[256] = {
   0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
   0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
   0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
   0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
   0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
   0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
   0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
   0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
   0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
   0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
   0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
   0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
   0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
   0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
   0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
   0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
   0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
   0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
   0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
   0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
   0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
   0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
   0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
   0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
   0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
   0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
   0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
   0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
   0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
   0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
   0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
   0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
   0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
   0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
   0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
   0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
   0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
   0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
   0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
   0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
   0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
   0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
   0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
   0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
   0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
   0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
   0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
   0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
   0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
   0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
   0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
   0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
   0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
   0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
   0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
   0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
   0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
   0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
   0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
   0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
   0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
   0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
   0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
   0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

U32	NIC_DEVICE_OBJECT::DeviceCalculateCRC32(
	PU8		ptrBuffer,
	int		nLength)
{
   U32 Crc = 0xffffffff;

   while (nLength--) {
      Crc = g_szCRCTable[(Crc ^ *ptrBuffer++) & 0xFF] ^ (Crc >> 8);
   }

   return ~Crc;

}
#endif


U32	NIC_DEVICE_OBJECT::DeviceCalculateCRC32(
	PU8		ptrBuffer,
	int		nLength,
	BOOL	bReverse)
{
    U32 	Crc, Carry;
    int		i, j;
    U8		CurByte;

    Crc = 0xffffffff;

    for (i = 0; i < nLength; i++) {

        CurByte = ptrBuffer[i];

        for (j = 0; j < 8; j++) {

            Carry = ((Crc & 0x80000000) ? 1 : 0) ^ (CurByte & 0x01);

            Crc <<= 1;

            CurByte >>= 1;

            if (Carry) {

                Crc = (Crc ^ 0x04c11db6) | Carry;

            }

        }

    }

    for (i=0, Carry=0x0L; i < 32 ; i++) {
       Carry <<= 1;
       if (Crc & 0x1) {
          Carry |= 0x1L;
       }
       Crc >>= 1;
    }

    return bReverse?~Carry:Carry;

}

/*******************************************************************************
 *
 * Device Attributes and Characteristics
 *
 ********************************************************************************/
PU8		NIC_DEVICE_OBJECT::DeviceMacAddress(
	PU8		ptrBuffer)
{
	if(!ptrBuffer) return ptrBuffer;

	PU16	pcurr=(PU16)&m_szEeprom[
		m_szEepromFormat[EID_MAC_ADDRESS]];
	*(PU16)ptrBuffer = *pcurr++;
	*(PU16)(ptrBuffer+2) = *pcurr++;
	*(PU16)(ptrBuffer+4) = *pcurr++;
	return ptrBuffer;
}

U16		NIC_DEVICE_OBJECT::DeviceVendorID(void)
{
	return *(PU16)(&m_szEeprom[m_szEepromFormat[EID_VENDOR_ID]]);
}

U16		NIC_DEVICE_OBJECT::DeviceProductID(void)
{
	return *(PU16)(&m_szEeprom[m_szEepromFormat[EID_PRODUCT_ID]]);
}

void	NIC_DEVICE_OBJECT::DeviceRegisterAdapter(void)
{
	int		attr=0;
	
	if(m_szConfigures[CID_BUS_MASTER])
		attr |= NDIS_ATTRIBUTE_BUS_MASTER;
		
	if(m_szConfigures[CID_INTERMEDIATE])
		attr = attr
			| NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER
			//| NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT
			//| NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT
			;
			
    NdisMSetAttributesEx(
		m_pUpper->GetNdisHandle(),	// miniport handle 
		(NDIS_HANDLE)m_pUpper,		// miniport context
		m_szConfigures[CID_CHECK_FOR_HANG_PERIOD],
		attr,
		(NDIS_INTERFACE_TYPE)m_szConfigures[CID_INTERFACE_TYPE]);
}

/*******************************************************************************
 *
 *
 ********************************************************************************/

void	NIC_DEVICE_OBJECT::DeviceReportStatistics(
	U32		uEvent,
	U32		uValue)
{

	if(uEvent >= TID_SIZE) return;
	m_szStatistics[uEvent] += uValue;
}


void	NIC_DEVICE_OBJECT::DeviceRetriveConfigurations(
	NDIS_HANDLE		hConfig)
{
	NDIS_STATUS	status;

	PCONFIG_PARAMETER	pconfig;
	
	PNDIS_CONFIGURATION_PARAMETER	param;

	for(pconfig=DeviceConfigureParameters();
		(pconfig->uId != (U32)-1);
		pconfig++)
	{
		NdisReadConfiguration(
			&status,
			&param,
			hConfig,
			&(pconfig->szName),
			NdisParameterHexInteger);
		if(status == NDIS_STATUS_SUCCESS)
			m_szConfigures[pconfig->uId] = 
				param->ParameterData.IntegerData;
		else
			m_szConfigures[pconfig->uId] = pconfig->uDefValue;
	}

//--------------------------------------------------------  
	// read mac addr
	{
	    NDIS_STATUS	 Status;  // Status of Ndis calls.
	    PVOID    NetAddress;
	    UINT     Length;

	    NdisReadNetworkAddress(&Status,&NetAddress,&Length,hConfig);
	    if ((Length	== ETH_ADDRESS_LENGTH) &&	(Status	== NDIS_STATUS_SUCCESS))
	    {
			// Save	the address that should	be used.
			NdisMoveMemory(	&m_szEeprom[	m_szEepromFormat[EID_MAC_ADDRESS]],NetAddress,ETH_ADDRESS_LENGTH);
	   	}
	}
//--------------------------------------------------------

}

void	NIC_DEVICE_OBJECT::DeviceSetDefaultSettings(void)
{
	m_szConfigures[CID_CHIP_STEPPING] = 0;

	m_szConfigures[CID_INTERMEDIATE] = 0;
	m_szConfigures[CID_NEED_IO_SPACE] = 1;
	m_szConfigures[CID_NEED_INTERRUPT] = 1;
	
	m_szCurrentSettings[SID_PHY_NUMBER] = MII_INTERNAL_PHY_ADDR;
	m_szCurrentSettings[SID_HW_STATUS] = NdisHardwareStatusReady;
	m_szCurrentSettings[SID_MEDIA_SUPPORTED] = NdisMedium802_3;
	m_szCurrentSettings[SID_MEDIA_IN_USE] = NdisMedium802_3;
	m_szCurrentSettings[SID_MEDIA_CONNECTION_STATUS] = NdisMediaStateConnected;
	m_szCurrentSettings[SID_OP_MODE] = 0;
	
	m_szCurrentSettings[SID_MAXIMUM_LOOKAHEAD] = ETH_MAX_FRAME_SIZE;
	m_szCurrentSettings[SID_MAXIMUM_FRAME_SIZE] = ETH_MAX_FRAME_SIZE - ETH_HEADER_SIZE;
    m_szCurrentSettings[SID_MAXIMUM_TOTAL_SIZE] = ETH_MAX_FRAME_SIZE;
    m_szCurrentSettings[SID_BUFFER_SIZE] = DRIVER_BUFFER_SIZE;
    m_szCurrentSettings[SID_MAXIMUM_SEND_PACKETS] = 1;
	m_szCurrentSettings[SID_LINK_SPEED] = 100000;

	m_szCurrentSettings[SID_GEN_MAC_OPTIONS] =
		NDIS_MAC_OPTION_TRANSFERS_NOT_PEND
		| NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA
		| NDIS_MAC_OPTION_RECEIVE_SERIALIZED
		| NDIS_MAC_OPTION_NO_LOOPBACK;
	
	m_szCurrentSettings[SID_GEN_CURRENT_PACKET_FILTER] = 0; 
	m_szCurrentSettings[SID_GEN_TRANSMIT_BUFFER_SPACE] = 
		m_szConfigures[CID_TXBUFFER_NUMBER]
		* ETH_MAX_FRAME_SIZE;
	m_szCurrentSettings[SID_GEN_RECEIVE_BUFFER_SPACE] = 
		m_szConfigures[CID_RXBUFFER_NUMBER]
		* ETH_MAX_FRAME_SIZE;
	m_szCurrentSettings[SID_GEN_TRANSMIT_BLOCK_SIZE] = ETH_MAX_FRAME_SIZE;
	m_szCurrentSettings[SID_GEN_RECEIVE_BLOCK_SIZE] = ETH_MAX_FRAME_SIZE;
	m_szCurrentSettings[SID_GEN_CURRENT_LOOKAHEAD] = ETH_MAX_FRAME_SIZE;
	m_szCurrentSettings[SID_GEN_DRIVER_VERSION] = 
		(PRJ_NDIS_MAJOR_VERSION << 8) | PRJ_NDIS_MINOR_VERSION;
	m_szCurrentSettings[SID_GEN_VENDOR_DRIVER_VERSION] = 0x01010000; 
	m_szCurrentSettings[SID_GEN_PROTOCOL_OPTIONS] = 0;

}

void	NIC_DEVICE_OBJECT::EDeviceRegisterIoSpace(void)
{
	if(!m_szConfigures[CID_NEED_IO_SPACE]) return;

#if defined(DM9102)

	NDIS_STATUS	status;

	if((status = NdisMRegisterIoPortRange(
		(PVOID*)&m_szCurrentSettings[SID_PORT_BASE_ADDRESS],
		m_pUpper->GetNdisHandle(),
		m_szConfigures[CID_IO_BASE_ADDRESS],
		m_szConfigures[CID_IO_RANGE])) != NDIS_STATUS_SUCCESS)	
		THROW((status));

	DEBUG_PRINT((
		TEXT("[dm9: NDIS returns io port at %X\r\n"),
		m_szCurrentSettings[SID_PORT_BASE_ADDRESS]
		));

#elif defined(DM9000)

	U32		uBase;
	PHYSICAL_ADDRESS	phyAddr;
	
	phyAddr.HighPart = 0;
	phyAddr.LowPart = m_szConfigures[CID_IO_BASE_ADDRESS];
	
	DEBUG_PRINT((
		TEXT("[dm9: Tries to map io space with %X\r\n"), phyAddr.LowPart
		));
		
	if(!(uBase = (U32)MmMapIoSpace(phyAddr, 16, FALSE)))
		THROW((ERR_STRING("Fails to map io space")));
		
	DEBUG_PRINT((
		TEXT("[dm9: The mapped address is %X\r\n"), uBase
		));	
		
	m_szCurrentSettings[SID_PORT_BASE_ADDRESS] = uBase;	

#endif
}		

void	NIC_DEVICE_OBJECT::EDeviceRegisterInterrupt(void)
{
	if(!m_szConfigures[CID_NEED_INTERRUPT]) return;

	NDIS_STATUS	status;
	if((status=NdisMRegisterInterrupt(
		&m_InterruptHandle,
		m_pUpper->GetNdisHandle(),
		m_szConfigures[CID_IRQ_NUMBER],	// or say, irq vector
		m_szConfigures[CID_IRQ_LEVEL],	// irql level
		TRUE,		// request ISR
		(BOOLEAN)m_szConfigures[CID_IRQ_SHARED],		// shared interrupt
		(KINTERRUPT_MODE)m_szConfigures[CID_IRQ_GEN_TYPE])) != NDIS_STATUS_SUCCESS)	
		THROW((ERR_STRING("Error in registering interrupt"),status));
}


void	NIC_DEVICE_OBJECT::EDeviceLoadEeprom(void)
{
	int		n;
	EEPROM_DATA_TYPE	*pcurr=(EEPROM_DATA_TYPE*)&m_szEeprom[0];
	
#if 0
	for(n=0;n<(DIM(m_szEeprom)/sizeof(EEPROM_DATA_TYPE));n++,pcurr++)
	{
		*pcurr = DeviceReadEeprom(n);

	} // of for offset n
	
	DeviceCalculateCRC32(&m_szEeprom[0],DIM(m_szEeprom)-2);
#else//DLP
	*(pcurr + 0) = 0xc000;
	*(pcurr + 1) = 0x2826;
	*(pcurr + 2) = 0x3096;
	*(pcurr + 3) = 0x28;
	*(pcurr + 4) = 0x96;
	*(pcurr + 5) = 0x30;
	*(pcurr + 6) = 0x00;
#endif
}

BOOL	NIC_DEVICE_OBJECT::DevicePolling(
	U32		uPort,
	U32		uMask,
	U32		uExpected,
	U32		uInterval,	/* in millisecond */
	U32		uRetries)
{
	for(;uRetries;uRetries--)
	{
		if((DeviceReadPort(uPort) & uMask) == uExpected) break;
		NdisStallExecution(uInterval);
	} // of retry loop
	
	return (BOOL)uRetries;
}

void NIC_DEVICE_OBJECT::SetConnectionStatus(bool bConnected)
{
	if (bConnected)
	{
		DEBUG_PRINT((TEXT("Send connection!\r\n")));
		m_szCurrentSettings[SID_MEDIA_CONNECTION_STATUS] = NdisMediaStateConnected;
		NdisMIndicateStatus(m_pUpper->GetNdisHandle(), NDIS_STATUS_MEDIA_CONNECT,
                 			(PVOID) 0, 0);

	}
   	else
	{
		DEBUG_PRINT((TEXT("Send disconnection!\r\n")));
		m_szCurrentSettings[SID_MEDIA_CONNECTION_STATUS] = NdisMediaStateDisconnected;
		NdisMIndicateStatus(m_pUpper->GetNdisHandle(), NDIS_STATUS_MEDIA_DISCONNECT,
			                (PVOID) 0, 0);
	}

	NdisMIndicateStatusComplete(m_pUpper->GetNdisHandle());
}

BOOL	NIC_DEVICE_OBJECT::DeviceCheckForHang(void)
{
	U16 link_state;

	// check link status
	link_state = DeviceReadPhy(0, 0x1);
	link_state = DeviceReadPhy(0, 0x1);
	DEBUG_PRINT((TEXT("[DM9isa]Link=%d\r\n"), (link_state&0x4)));

	if (m_nOldLinkState != link_state)
	{	
		if (link_state & 0x4)
			SetConnectionStatus(true);
		else
    	SetConnectionStatus(false);
	}

	m_nOldLinkState = link_state;


	U32	cr= DeviceGetReceiveStatus();
	U32	rxps,rxci;
	
	rxps = cr >> 31;
	rxci = cr & 0x7FFFFFFF;

	REPORT(TID_NIC_RXPS,rxps);	
	REPORT(TID_NIC_RXCI,rxci);	

#ifndef	IMPL_RESET
	return FALSE;
#endif

	U32	lastread = m_szLastStatistics[TID_GEN_RCV_OK];
	U32	lastsent = m_szLastStatistics[TID_GEN_XMIT_OK];

	memcpy(
		(void*)&m_szLastStatistics,
		(void*)&m_szStatistics,
		sizeof(m_szStatistics));

	// report hang if 
	// 1. receive count stalled but overflow, or
	if((m_szStatistics[TID_GEN_RCV_OK] == lastread) && cr) 
		return TRUE;
	
	// 2. tx idle while tqueue out of resource
	if(m_pUpper->DriverIsOutOfResource() &&
		(DeviceHardwareStatus() & NIC_HW_TX_IDLE) ) 
	{
		return TRUE;
	}
	return FALSE;
}

/*******************************************************************************
 *
 * Device timer routine
 *
 ********************************************************************************/

extern "C" void	DeviceTimerTrunkRoutine(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3)
{
	NIC_DEVICE_OBJECT	*dev = (NIC_DEVICE_OBJECT*)FunctionContext;

	if(!dev->m_mutexTimer.TryLock()) return;
	dev->DeviceOnTimer();
	dev->m_mutexTimer.Release();
}


void	NIC_DEVICE_OBJECT::DeviceInitializeTimer(void)
{
	NdisMInitializeTimer(
		&m_timerObject,
		m_pUpper->GetNdisHandle(),
		(PNDIS_TIMER_FUNCTION)DeviceTimerTrunkRoutine,
		(PVOID)this);
}

void	NIC_DEVICE_OBJECT::DeviceCancelTimer(void)
{
	BOOLEAN	result;
	NdisMCancelTimer(
		&m_timerObject,
		&result);
}

void	NIC_DEVICE_OBJECT::DeviceSetTimer(
	U32 milliseconds)
{
	NdisMSetTimer(&m_timerObject,milliseconds);
}

void	NIC_DEVICE_OBJECT::DeviceOnTimer(void)
{
}
