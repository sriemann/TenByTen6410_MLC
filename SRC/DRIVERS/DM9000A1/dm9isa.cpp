/********************************************************************************
 * 
 * $Id: dm9isa.cpp,v 1.2 2007/07/10 04:16:22 bill Exp $
 *
 * Copyright (c) 2000-2005 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/

#include	"dm9isa.h"


/*******************************************************************************
 *
 *
 *
 ********************************************************************************/
CONFIG_PARAMETER	g_szDm9ConfigParams[] =
{
	{ CID_CONNECTION_TYPE, -1, NDIS_STRING_CONST("ConnectionType") },
	{ CID_SLOT_NUMBER, -1, NDIS_STRING_CONST("SlotNumber")},
	{ CID_BUFFER_PHYSICAL_ADDRESS, 0, NDIS_STRING_CONST("BufferPhysicalAddress")},
	{ CID_TXBUFFER_NUMBER, 0x20, NDIS_STRING_CONST("XmitBuffer")},
	{ CID_RXBUFFER_NUMBER, 0x10, NDIS_STRING_CONST("RecvBuffer")},
	{ CID_ADAPTER_NUMBER, 0, NDIS_STRING_CONST("AdapterNumber")},
	{ CID_IO_BASE_ADDRESS, 0x18000300, NDIS_STRING_CONST("IoAddress")},
	{ CID_IO_RANGE, 0x10, NDIS_STRING_CONST("IoRange")},
	{ CID_IRQ_NUMBER, 12, NDIS_STRING_CONST("IrqNumber")},
	{ -1,-1,NULL}
};

/*******************************************************************************
 *
 * Device attributes and characteristics
 *
 ********************************************************************************/
PCONFIG_PARAMETER	C_DM9000::DeviceConfigureParameters(void)
{
	return (PCONFIG_PARAMETER)&g_szDm9ConfigParams[0];
}

void	C_DM9000::DeviceSetEepromFormat(void)
{
	m_szEepromFormat[EID_MAC_ADDRESS] = 0;
	m_szEepromFormat[EID_VENDOR_ID] = 8;
	m_szEepromFormat[EID_PRODUCT_ID] = 10;
}

void	C_DM9000::EDeviceRegisterIoSpace(void)
{
	NIC_DEVICE_OBJECT::EDeviceRegisterIoSpace();
	
	U32	val;
	
	val  = DeviceReadPort(0x2a);
	val |= DeviceReadPort(0x2b)<<8;
	val |= DeviceReadPort(0x28)<<16;
	val |= DeviceReadPort(0x29)<<24;

	DEBUG_PRINT((
		TEXT("[dm9: Chip signature is %08X\r\n"), val
		));

	RETAILMSG(1, (TEXT("[dm9: Chip signature is %08X\r\n"), val));
	
	if( (DeviceReadPort(DM9_VIDL) != LOW_BYTE(DM9000_VID)) ||
		(DeviceReadPort(DM9_VIDH) != HIGH_BYTE(DM9000_VID)) ||
		(DeviceReadPort(DM9_PIDL) != LOW_BYTE(DM9000_PID)) ||
		(DeviceReadPort(DM9_PIDH) != HIGH_BYTE(DM9000_PID)) )
		THROW((ERR_STRING("Unknown device")));

}

void	C_DM9000::EDeviceValidateConfigurations(void)
{
	NDIS_HANDLE		hndis = m_pUpper->GetNdisHandle();

	// validate slot number
	if( 
		(m_szConfigures[CID_IO_BASE_ADDRESS] == -1) ||
		(m_szConfigures[CID_IRQ_NUMBER] == -1) ) 
		THROW(());

	m_szCurrentSettings[SID_GEN_TRANSMIT_BUFFER_SPACE] = 
		m_szConfigures[CID_TXBUFFER_NUMBER]
		* ETH_MAX_FRAME_SIZE;
	m_szCurrentSettings[SID_GEN_RECEIVE_BUFFER_SPACE] = 
		m_szConfigures[CID_RXBUFFER_NUMBER]
		* ETH_MAX_FRAME_SIZE;

	m_szConfigures[CID_CHECK_FOR_HANG_PERIOD] = 3;
	m_szConfigures[CID_IRQ_GEN_TYPE] = NdisInterruptLatched;
	m_szConfigures[CID_IRQ_SHARED] = TRUE;
	m_szConfigures[CID_IRQ_LEVEL] = 0x0F;
	m_szConfigures[CID_INTERFACE_TYPE] = NdisInterfaceIsa;
	m_szConfigures[CID_BUS_MASTER] = FALSE;

	// set receive mode
	// <5> discard long packet
	// <4> discard CRC error packet
	// <0> rx enable
	m_szCurrentSettings[SID_OP_MODE] = MAKE_MASK3(5,4,0);

	m_szCurrentSettings[SID_802_3_MAXIMUM_LIST_SIZE] = DM9_MULTICAST_LIST;
}


/*******************************************************************************
 *
 * Device access routines
 *
 ********************************************************************************/
#define	ENTER_CRITICAL_SECTION	m_spinAccessToken.Lock();
#define	LEAVE_CRITICAL_SECTION	m_spinAccessToken.Release();
#define	VALIDATE_ADDR_PORT(p) \
	if(m_uLastAddressPort != (p)) \
		NdisRawWritePortUchar( \
		m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_ADDR_OFFSET,  \
		(U8) (m_uLastAddressPort=(U32(p))) )


U32	C_DM9000::DeviceWritePort(
	U32		uPort,
	U32		uValue)
{

	ENTER_CRITICAL_SECTION

	VALIDATE_ADDR_PORT(uPort);

	NdisRawWritePortUchar(
		m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, 
		(U8)uValue);

	LEAVE_CRITICAL_SECTION
	
	return uValue;
}


U32	C_DM9000::DeviceReadPort(
	U32		uPort)
{
	U16		val;

	ENTER_CRITICAL_SECTION

	VALIDATE_ADDR_PORT(uPort);

	NdisRawReadPortUchar(
		m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, &val);
	
	LEAVE_CRITICAL_SECTION
	
	return (U32)val;
}

U16	C_DM9000::DeviceReadEeprom(
	U32		uWordAddress)
{
	U16		highbyte,lowbyte;
	
	// assign the register offset
	DeviceWritePort(DM9_EPADDR,uWordAddress);
	
	// issue EEPROM read command<2>
	DeviceWritePort(DM9_EPCNTL,(1<<2));
	
	// wait until status bit<0> cleared
	// 80 uS, 5 times
	if(!DevicePolling(DM9_EPCNTL,(1<<0),0x00,80,5))
		return (U16)-1;
	
	// stop command
	DeviceWritePort(DM9_EPCNTL,0);

	// retrive data
	lowbyte  = (U16)DeviceReadPort(DM9_EPLOW);
	highbyte = (U16)DeviceReadPort(DM9_EPHIGH);
	
	return ((highbyte<<8) | lowbyte);
}

U16	C_DM9000::DeviceWriteEeprom(
	U32		uWordAddress,
	U16		uValue)
{
	// assign the register offset
	DeviceWritePort(DM9_EPADDR,uWordAddress);

	// put data
	DeviceWritePort(DM9_EPLOW, LOW_BYTE(uValue));
	DeviceWritePort(DM9_EPHIGH,HIGH_BYTE(uValue));
		
	// issue EEPROM write enable<4> and write command<1>
	DeviceWritePort(DM9_EPCNTL,MAKE_MASK2(4,1));
	
	// wait until status bit<0> cleared
	DevicePolling(DM9_EPCNTL,MAKE_MASK(0),0x00);
	
	// stop command
	DeviceWritePort(DM9_EPCNTL,0);

	// extra delay
	NdisStallExecution(1000);
	
	return uValue;
}

U16	C_DM9000::DeviceReadPhy(
	U32		uRegister,
	U32		uOffset)
{
	U16		highbyte,lowbyte;
	
	// assign the phy register offset, internal phy(0x40) plus phy offset
	DeviceWritePort(DM9_EPADDR,(0x40|(uOffset&0x3F)));
	
	// issue PHY select<3> and PHY read command<2>
	DeviceWritePort(DM9_EPCNTL,((1<<3)|(1<<2)) );
	
	// wait until status bit<0> cleared
	DevicePolling(DM9_EPCNTL,(1<<0),0x00);
	
	// stop command
	DeviceWritePort(DM9_EPCNTL,0);

	// retrive data
	lowbyte  = (U16)DeviceReadPort(DM9_EPLOW);
	highbyte = (U16)DeviceReadPort(DM9_EPHIGH);
	
	return ((highbyte<<8) | lowbyte);
}

U16	C_DM9000::DeviceWritePhy(
	U32		uRegister,
	U32		uOffset,
	U16		uValue)
{
	// assign the phy register offset, internal phy(0x40) plus phy offset
	DeviceWritePort(DM9_EPADDR,(0x40|(uOffset&0x3F)));

	// put data
	DeviceWritePort(DM9_EPLOW, LOW_BYTE(uValue));
	DeviceWritePort(DM9_EPHIGH,HIGH_BYTE(uValue));

	// issue PHY select<3> and write command<1>		
	DeviceWritePort(DM9_EPCNTL,((1<<3)|(1<<1)) );
	
	// wait until status bit<0> cleared
	DevicePolling(DM9_EPCNTL,(1<<0),0x00);
	
	// stop command
	DeviceWritePort(DM9_EPCNTL,0);

	return uValue;
}

	
U32		C_DM9000::DeviceReadData(void)
{
	U32		value;

	return	*(PU32)DeviceReadString((PU8)&value,sizeof(value));
}
	
U32		C_DM9000::DeviceReadDataWithoutIncrement(void)
{
	U32		value,tmp;

	ENTER_CRITICAL_SECTION

	VALIDATE_ADDR_PORT(DM9_MRCMDX);

	switch (m_nIoMode)
	{
		case BYTE_MODE:
			NdisRawReadPortUchar(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
				+ DM9000_DATA_OFFSET, (PU8)&tmp);
			NdisRawReadPortUchar(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
				+ DM9000_DATA_OFFSET, (PU8)&value);
			value = (value&0x000000FF);
			break;

		case WORD_MODE:
			NdisRawReadPortUshort(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
				+ DM9000_DATA_OFFSET, (PU16)&tmp);
			NdisRawReadPortUshort(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
				+ DM9000_DATA_OFFSET, (PU16)&value);
			value = (value&0x0000FFFF);
			break;
				
		case DWORD_MODE:
			NdisRawReadPortUlong(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
				+ DM9000_DATA_OFFSET, (PU32)&tmp);
			NdisRawReadPortUlong(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
				+ DM9000_DATA_OFFSET, (PU32)&value);
			break;
				
		default:
			break;
	} // of switch

	LEAVE_CRITICAL_SECTION
	
	return value;
}


PU8		C_DM9000::DeviceReadString(
	PU8		ptrBuffer,
	int		nLength)
{
	int		count;
	
	count = (nLength + m_nIoMaxPad) / m_nIoMode;

	// select port to be read from
	ENTER_CRITICAL_SECTION

	VALIDATE_ADDR_PORT(DM9_MRCMD);

	switch (m_nIoMode)
	{
		case BYTE_MODE:
			NdisRawReadPortBufferUchar(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, 
				ptrBuffer,count);
			break;
			
		case WORD_MODE:
			NdisRawReadPortBufferUshort(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, 
				(PU16)ptrBuffer,count);
			break;

		case DWORD_MODE:
			NdisRawReadPortBufferUlong(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, 
				(PU32)ptrBuffer,count);
			break;
		
		default:
			break;
	} // of switch

	LEAVE_CRITICAL_SECTION
	
	return ptrBuffer;
}


PU8		C_DM9000::DeviceWriteString(
	PU8		ptrBuffer,
	int		nLength)
{
	int		count;
	
	count = (nLength + m_nIoMaxPad) / m_nIoMode;

#if defined(PREEMPTIVE_TX_WRITE) 

	switch (m_nIoMode)
	{
		case BYTE_MODE:
		{
			PU8	pcurr=(PU8)ptrBuffer;
			for(;count--;pcurr++)
			{
				ENTER_CRITICAL_SECTION
				VALIDATE_ADDR_PORT(DM9_MWCMD);

				NdisRawWritePortUchar(
					m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
					+ DM9000_DATA_OFFSET, *pcurr);
				
				LEAVE_CRITICAL_SECTION
			}
		}
			break;
			
		case WORD_MODE:
		{
			PU16	pcurr=(PU16)ptrBuffer;
			
			for(;count--;pcurr++)
			{
				ENTER_CRITICAL_SECTION
				VALIDATE_ADDR_PORT(DM9_MWCMD);

				NdisRawWritePortUshort(
					m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
					+ DM9000_DATA_OFFSET, *pcurr);

				LEAVE_CRITICAL_SECTION
			}
		}
			break;

		case DWORD_MODE:
		{
			PU32	pcurr=(PU32)ptrBuffer;
			for(;count--;pcurr++)
			{
				ENTER_CRITICAL_SECTION
				VALIDATE_ADDR_PORT(DM9_MWCMD);

				NdisRawWritePortUlong(
					m_szCurrentSettings[SID_PORT_BASE_ADDRESS] 
					+ DM9000_DATA_OFFSET, *pcurr);
				
				LEAVE_CRITICAL_SECTION
			}
		}
			break;
		
		default:
			break;
	} // of switch
	
#else // !PREEMPTIVE_TX_WRITE
	// select port to be read from
	ENTER_CRITICAL_SECTION

	VALIDATE_ADDR_PORT(DM9_MWCMD);

	switch (m_nIoMode)
	{
		case BYTE_MODE:
			NdisRawWritePortBufferUchar(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, 
				ptrBuffer,count);
			break;
			
		case WORD_MODE:
			NdisRawWritePortBufferUshort(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, 
				(PU16)ptrBuffer,count);
			break;

		case DWORD_MODE:
			NdisRawWritePortBufferUlong(
				m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET, 
				(PU32)ptrBuffer,count);
			break;
		
		default:
			break;
	} // of switch
	
	LEAVE_CRITICAL_SECTION

#endif

	return ptrBuffer;
}


/********************************************************************************
 *
 * Devcie control routines
 *
 ********************************************************************************/

void	C_DM9000::DeviceEnableInterrupt(void)
{
	// bits to turn on interrupt latch
	// <7> buffer chain enable
	// <3> rx overflow count overflow
	// <2> rx overflow
	// <1> tx completed indication
	// <0> rx completed indication
	DeviceWritePort(DM9_IMR,((1<<7)|(1<<3)|(1<<2)|(1<<1)|(1<<0)));
}

void	C_DM9000::DeviceDisableInterrupt(void)
{
	// <7> buffer chain enable
	DeviceWritePort(DM9_IMR,(1<<7));
}

void 	C_DM9000::DeviceEnableReceive(void)
{
	// RX enable RXCR<0>
	if(m_szCurrentSettings[SID_OP_MODE] & MAKE_MASK(0)) return;

	m_szCurrentSettings[SID_OP_MODE] |= MAKE_MASK(0);
		
	DeviceWritePort(DM9_RXCR,m_szCurrentSettings[SID_OP_MODE]);
}

void 	C_DM9000::DeviceDisableReceive(void)
{
	// RX enable RXCR<0>
	if(!(m_szCurrentSettings[SID_OP_MODE] & MAKE_MASK(0))) return;
		
	m_szCurrentSettings[SID_OP_MODE] &= ~MAKE_MASK(0);
		
	DeviceWritePort(DM9_RXCR,m_szCurrentSettings[SID_OP_MODE]);
}

U32	C_DM9000::DeviceGetInterruptStatus(void)
{
	// mask for bits
	// <3> rx overflow count overflow
	// <2> rx overflowf
	// <1> tx completed indication
	// <0> rx completed indication
	return DeviceReadPort(DM9_ISR) & MAKE_MASK4(3,2,1,0);

}

U32	C_DM9000::DeviceSetInterruptStatus(
	U32	uValue)
{
	return DeviceWritePort(DM9_ISR,uValue);
}

U32	C_DM9000::DeviceGetReceiveStatus(void)
{
	DEBUG_PRINT((
		TEXT("nTxPendings=%d, %d"),
			m_nTxPendings, m_pUpper->m_TQueue.Size()
		));
	
	U32	cr;

	cr = DeviceReadPort(DM9_ROCR);
	
	return ((cr>>7)&1)<<31 | (cr&0x7F);
}

void	C_DM9000::DeviceStart(void)
{
#ifdef	IMPL_FLOW_CONTROL	

	U32		val;
	
	// set PHY supports flow control
	DeviceWritePhy(0, 4, (DeviceReadPhy(0,4)|(1<<10)));
	
	// check full-duplex mode or not<3>
	val = DeviceReadPort(DM9_NCR);
	if( val & MAKE_MASK(3))
	{
		/* full duplex mode */
		val = DeviceReadPort(DM9_PAUSETH);
		DeviceWritePort(DM9_PAUSETH,(U8)val);
		
		// enable flow control<0>
		// enable pause packet<5>
		DeviceWritePort(DM9_FLOW,MAKE_MASK2(5,0));
	}
	else
	{
		/* non full duplex mode */
		val = DeviceReadPort(DM9_BACKTH);
		DeviceWritePort(DM9_BACKTH,(U8)val);

		// enable back pressure<half dumplex mode)<4,3>
		DeviceWritePort(DM9_FLOW,MAKE_MASK2(4,3));
	}
#endif

	// enable interrupt
	DeviceEnableInterrupt();

	DeviceWritePort(DM9_RXCR,m_szCurrentSettings[SID_OP_MODE]);
}

void	C_DM9000::DeviceReset(void)
{

	// announce shutdown
	m_bShutdown = 1;
	
	// wait until all activities are fi.
	m_mutexRxValidate.Lock();
	m_mutexTxValidate.Lock();
		
	
	C_Exception	*pexp;
	
	TRY
	{
	
		EDeviceInitialize(++m_nResetCounts);
	
		DeviceOnSetupFilter(0);
		
		FI;
	}
	CATCH(pexp){
		// nothing to do
		CLEAN(pexp);
	} // of catch

	// dequeue for all objects in waiting and standby queue
	PCQUEUE_GEN_HEADER	pcurr;
	for(;(pcurr=m_TQWaiting.Dequeue());)
		DeviceSendCompleted(pcurr);
	
	for(;(pcurr=m_TQStandby.Dequeue());)
		DeviceSendCompleted(pcurr);
	
	m_mutexRxValidate.Release();
	m_mutexTxValidate.Release();

	m_bShutdown = 0;
}

void	C_DM9000::EDeviceInitialize(
	int		nResetCounts)
{
	U32		val;
	
	// reset member varialbes
	m_uLastAddressPort = (U32)-1;
	
	DeviceWritePort(0x1f, 0x00);
	NdisStallExecution(20);

	// software reset the device
	DeviceWritePort(DM9_NCR, 0x03);
	NdisStallExecution(20);

	DeviceWritePort(DM9_NCR, 0x03);
	NdisStallExecution(20);

	//DeviceWritePort(DM9_NCR, 0x00);
	
	// read the io orgnization
	// ISR<7:6> == x1, dword
	// ISR<7:6> == 0x, word
	// ISR<7:6> == 10, byte mode
	val = DeviceReadPort(DM9_ISR);
	if(val & MAKE_MASK(6))
	{
		m_nIoMode = DWORD_MODE;
		m_nIoMaxPad = 3;
	}
	else if(!(val & MAKE_MASK(7)))
	{
		m_nIoMode = WORD_MODE;
		m_nIoMaxPad = 1;
	}
	else
	{
		m_nIoMode = BYTE_MODE;
		m_nIoMaxPad = 0;
	}
	
	// activate internal phy
	// select GPIO 0<0>, set it to output
	DeviceWritePort(DM9_GPCR, (1<<0));
	// output zero to activate internal phy
	DeviceWritePort(DM9_GPR,  0x00);
	
	// clear TX status
	DeviceWritePort(DM9_NSR, 0x00);
	
	
	// Enable memory chain
	DeviceWritePort(DM9_IMR, (1<<7));
	
#ifdef	IMPL_STORE_AND_INDICATION
	if(nResetCounts) return;
	
	/* init rx buffers */
	U32		m,uaddr;
	if(!(uaddr = (U32)malloc(sizeof(DATA_BLOCK)*
		(m=m_szConfigures[CID_RXBUFFER_NUMBER]*2)))) 
		THROW((ERR_STRING("Insufficient memory")));

	for(;m--;uaddr+=sizeof(DATA_BLOCK))
		m_RQueue.Enqueue((PCQUEUE_GEN_HEADER)uaddr);

	/* set indication timer */
	DeviceInitializeTimer();
	
#endif

// v3.2.9
	m_nMaxTxPending = (DeviceReadPort(DM9_CHIPREV) >= 0x10)?2:1;
	m_nTxPendings = 0;
}

void	C_DM9000::DeviceResetPHYceiver(void)
{
	return;
}

/********************************************************************************
 *
 * Devcie handler routines
 *
 ********************************************************************************/
#ifdef	IMPL_DEVICE_ISR
void	C_DM9000::DeviceIsr(
	U32		uState)
{
}
#endif

#ifdef	IMPL_STORE_AND_INDICATION
void	C_DM9000::DeviceOnTimer(void)
{
	int	val = m_RQStandby.Size();
	
	PCQUEUE_GEN_HEADER	pcurr;
	
	for(;(pcurr=m_RQStandby.Dequeue());m_RQueue.Enqueue(pcurr))
	{
		DeviceReceiveIndication(
			0,CQueueGetUserPointer(pcurr),pcurr->nLength);

	} // of for RQStandby loop
	
}
#endif

int	C_DM9000::DeviceOnSetupFilter(
	U32		uFilter)
{
	int		n;
	U8		sz[8];
	U32		hashval;
	U32		newmode;
	
	// save old op mode
	newmode = m_szCurrentSettings[SID_OP_MODE];
	// clear filter related bits,
	// pass all multicast<3> and promiscuous<1>
	newmode	&= ~MAKE_MASK2(3,1);

	// zero input means one reset request
	if(!(m_szCurrentSettings[SID_GEN_CURRENT_PACKET_FILTER]=uFilter)) 
	{
		
		/* 1. set unicast */
		// retrive node address
		DeviceMacAddress(&sz[0]);
		// set node address
		for(n=0;n<ETH_ADDRESS_LENGTH;n++)
			DeviceWritePort(DM9_PAR0+n,(U32)sz[n]);
			
		/* 2. clear multicast list and count */
		m_nMulticasts = 0;
		memset((void*)&m_szMulticastList,0,sizeof(m_szMulticastList));
		
		/* 3. clear hash table */
		// clear hash table
		memset((void*)(&sz[0]),0,sizeof(sz));
		for(n=0;n<sizeof(sz);n++)
			DeviceWritePort(DM9_MAR0+n,(U32)sz[n]);

		return uFilter;		
	}
	

	// if promiscuous mode<1> is requested,
	// just set this bit and return
	if( (uFilter & NDIS_PACKET_TYPE_PROMISCUOUS) )
	{
		// add promiscuous<1>
		newmode |= MAKE_MASK(1);
		DeviceWritePort(DM9_RXCR,m_szCurrentSettings[SID_OP_MODE]=newmode);
		return uFilter;
	}

	// if pass all multicast<3> is requested,
	if(uFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) newmode |= MAKE_MASK(3);

	// prepare new hash table
	memset((void*)(&sz[0]),0,sizeof(sz));
	
	// if broadcast, its hash value is known as 63.
	if(uFilter & NDIS_PACKET_TYPE_BROADCAST) sz[7] |= 0x80;

	if(uFilter & NDIS_PACKET_TYPE_MULTICAST)
		for(n=0;n<m_nMulticasts;n++)
		{
			hashval = DeviceCalculateCRC32(
				&m_szMulticastList[n][0],ETH_ADDRESS_LENGTH,FALSE) & 0x3f;
			sz[hashval/8] |= 1 << (hashval%8);
		} // of calculate hash table

	// submit the new hash table
	for(n=0;n<sizeof(sz);n++)
		DeviceWritePort(DM9_MAR0+n,(U32)sz[n]);
	
	DeviceWritePort(DM9_RXCR,m_szCurrentSettings[SID_OP_MODE]=newmode);

	return uFilter;
}


void C_DM9000::DeviceInterruptEventHandler(
	U32	uValue)
{
	// check RX activities 
	if(uValue & 0x01) Dm9LookupRxBuffers();

	// return if not TX latch
	if(!(uValue & 0x02)) return;

	U32	nsr;

	nsr = DeviceReadPort(DM9_NSR);
	
	// check TX-END2
	if(nsr & 0x08)
	{
		m_nTxPendings--;
		DeviceSendCompleted(m_TQWaiting.Dequeue());
	}

	// check TX-END1
	if(nsr & 0x04)
	{
		m_nTxPendings--;
		DeviceSendCompleted(m_TQWaiting.Dequeue());
	}
	
	// report tx available now
	if( nsr & 0x0C )
		NdisMSendResourcesAvailable(m_pUpper->GetNdisHandle());

}

U32	C_DM9000::DeviceHardwareStatus(void)
{
	return m_nTxPendings?0:NIC_HW_TX_IDLE;
}

				
int	C_DM9000::DeviceSend(
	PCQUEUE_GEN_HEADER	pObject)
{
	PCQUEUE_GEN_HEADER pcurr;

	if(pObject) m_TQStandby.Enqueue(pObject);
	
	/* increment counter */
	m_nTxPendings++;
	
	/* get first pkt in queue */
	m_TQWaiting.Enqueue(pcurr=m_TQStandby.Dequeue());
	
	/* fill data */
	DeviceWriteString((PU8)CQueueGetUserPointer(pcurr),pcurr->nLength);
	
	DeviceWritePort(DM9_TXLENH,HIGH_BYTE(pcurr->nLength));
	DeviceWritePort(DM9_TXLENL,LOW_BYTE(pcurr->nLength));
		
	// TXCR<0>, issue TX request
	DeviceWritePort(DM9_TXCR, MAKE_MASK(0));

	return 0;
}

int	C_DM9000::Dm9LookupRxBuffers(void)
{

	if(!m_mutexRxValidate.TryLock()) return 0;

	int		counts=0;
	int		errors=0;
	
	U32		desc;
	PDM9_RX_DESCRIPTOR	pdesc;

#ifdef	IMPL_STORE_AND_INDICATION
	PCQUEUE_GEN_HEADER	pcurr;
#else
	U8		szbuffer[DRIVER_BUFFER_SIZE];
#endif
		
	for(pdesc=(PDM9_RX_DESCRIPTOR)&desc;;)
	{
		CHECK_SHUTDOWN();

		// probe first byte
		desc = DeviceReadDataWithoutIncrement();
	
		// check if packet available, 01h means available, 00h means no data
		if(pdesc->bState != 0x01) break;

		// get the data descriptor again
		desc = DeviceReadData();

		// read out the data to buffer
		// Performance issue: maybe we may discard the data
		// just add the rx address.
		// if the length is greater than buffer size, ...
		if((pdesc->nLength > DRIVER_BUFFER_SIZE))
		{
			DeviceIndication(AID_LARGE_INCOME_PACKET);
			break;
		}

#ifdef	IMPL_STORE_AND_INDICATION
		if(!(pcurr=m_RQueue.Dequeue()))
		{
			RETAILMSG(TRUE,(TEXT("Queue overflow")));
			BREAK;
			// packet will lost!!
			break;
		}		
		DeviceReadString((PU8)CQueueGetUserPointer(pcurr),pcurr->nLength=pdesc->nLength);
#else
		DeviceReadString((PU8)&szbuffer,pdesc->nLength);
#endif
		
		// check status, as specified in DM9_RXSR,
		// the following bits are error
		// <3> PLE
		// <2> AE
		// <1> CE
		// <0> FOE
		if(pdesc->bStatus & MAKE_MASK4(3,2,1,0))
		{
			errors++;
#ifdef	IMPL_STORE_AND_INDICATION
			m_RQueue.Enqueue(pcurr);
#endif
			continue;
		} // of error happens

		counts++;

#ifdef	IMPL_STORE_AND_INDICATION
		m_RQStandby.Enqueue(pcurr);
#else
		DeviceReceiveIndication(
			0,
			(PVOID)&szbuffer,
			pdesc->nLength);
#endif
	} // of forever read loop

	REPORT(TID_GEN_RCV_OK, counts);
	REPORT(TID_GEN_RCV_ERROR, errors);

	m_mutexRxValidate.Release();
#ifdef	IMPL_STORE_AND_INDICATION
	if (!m_RQStandby.IsQueueEmpty()) DeviceSetTimer(5);
#endif	
	return counts;

}


/*******************************************************************
 *
 * Hooked function
 *
 *******************************************************************/

/*******************************************************************
 *
 * Device Entry
 *
 *******************************************************************/

extern "C" NIC_DEVICE_OBJECT	*DeviceEntry(
	NIC_DRIVER_OBJECT	*pDriverObject,
	PVOID				pVoid)
{
	
	return new C_DM9000(pDriverObject,pVoid);
}
