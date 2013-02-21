/********************************************************************************
 * 
 * $Id: driver.h,v 1.1.1.1 2007/04/16 03:45:52 bill Exp $
 *
 * File: Driver.h
 *
 * Optimizations:
 *	When build in RETAIL mode, the compiler options is set to /Oxs,
 *  which means maximum opts.(x) and favor for code space(s).
 *  Option Ox stands for Ogityb1 and Gs, for more detail information,
 *  type CL/? at command your DOS command prompt.
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/
 
#ifndef	__WINCE_DRIVER_H__
#define	__WINCE_DRIVER_H__

#include	"types.h"
#include	"common.h"

#define PRJ_NDIS_MAJOR_VERSION   4
#define PRJ_NDIS_MINOR_VERSION   0

#ifndef	VENDOR_DESC
#define	VENDOR_DESC	"Davicom Semiconductor, Inc"
#endif


/********************************************************************************
 *
 * Driver behaviors
 *
 ********************************************************************************/
//#define	IMPL_SEND_INDICATION
#define	IMPL_STATISTICS
#define	IMPL_RESET
#define	IMPL_SHARED_TX
#define	SHARED_TX_LEVEL	2
//#define	IMPL_DEVICE_ISR

/********************************************************************************
 *
 * PCI MAP
 *
 ********************************************************************************/
#define PCI_OFFSET_ID			0x00
#define PCI_OFFSET_CMD			0x04
#define PCI_OFFSET_STATUS		0x04
#define PCI_OFFSET_REV			0x08
#define PCI_OFFSET_MISC			0x0C
#define PCI_OFFSET_IO_BASE		0x10
#define PCI_OFFSET_MEM_BASE		0x14
#define PCI_OFFSET_SUB_ID		0x2C
#define PCI_OFFSET_INT			0x3C
#define PCI_OFFSET_USR			0x40
#define PCI_OFFSET_PMR			0x50


/******************************************************************************************
 *
 * Ethernet definitions
 *
 *******************************************************************************************/
#define ETH_MAX_FRAME_SIZE  1514
#define ETH_HEADER_SIZE     14
#define ETH_ADDRESS_LENGTH  6
#define ETH_CRC_SIZE     	4
#define	MAX_MULTICAST_LIST	64
#define	DRIVER_BUFFER_SIZE	0x5F0


/******************************************************************************************
 *
 * NIC_DRIVER_OBJECT definition
 *
 *******************************************************************************************/
typedef	struct	_CONFIG_PARAMETER
{
	U32				uId;
	U32				uDefValue;
	NDIS_STRING		szName;
}	CONFIG_PARAMETER, *PCONFIG_PARAMETER;

typedef	enum {
	CID_CONNECTION_TYPE=0,
	CID_SLOT_NUMBER,
	CID_BUFFER_PHYSICAL_ADDRESS,
	CID_TXBUFFER_NUMBER,
	CID_RXBUFFER_NUMBER,
	CID_ADAPTER_NUMBER,
	CID_IO_BASE_ADDRESS,
	CID_IO_RANGE,
	CID_IRQ_NUMBER,		// irq pin or line number
	CID_IRQ_LEVEL,		// to raise irql level
	CID_IRQ_GEN_TYPE,	// level sensitive(pci) or latched(isa)
	CID_IRQ_SHARED,		// shared or not
	CID_INTERFACE_TYPE,	// isa or pci device
	CID_BUS_MASTER,		// is a bus master or not
	CID_INTERMEDIATE,	// is a intermediate miniport
	CID_CHECK_FOR_HANG_PERIOD,	// in seconds
	CID_CHIP_STEPPING,
	CID_NEED_IO_SPACE,
	CID_NEED_INTERRUPT,
	
	/* wireless settings */
	CID_WLAN_NETWORK_TYPE,
	
	CID_SIZE
} DEVICE_CID_TYPE;

#if 0
typedef struct _SYSTEM_PARAM
{
	U8		szClientName[MAX_ID_LEN];
	U32		uIpAddress;
	U8		szBSSID[IEEE_ADDR_LEN];
	U8		szSSID1[IEEE_ADDR_LEN];
	U8		szSSID2[IEEE_ADDR_LEN];
	int		nNetworkType;		/* 0: ad hoc, 1: infras */
	int		nPowerMode;			/* D0..D2 */
	int		nWakeupDuration;	/* in KuS */
	int		nBeaconInterval;	/* in KuS */
	int		nPrivacy;			/* 0: disable, 1: 48bit, 2: 128 bit */
	int		nAuthMethod;		/* 0: Open, 1: Shared key */
}	SYSTEM_PARAM, *PSYSTEM_PARAM;
typedef	struct	_TRANS_PARAM
{
	int		nShortPreemble;		/* 0: false, 1: true */
	int		nDataRate;			/* 0: for auto, others: Mbps times 10 */
	int		nChannel;			/* 0: for auto, 1..13 */
	int		nTransmitPower;		/* in mW */
	int		nTxAntenna;			/* 0: for both, 1: left, 2: right */
	int		nRxAntenna;			/* 0: for both, 1: left, 2: right */
	int		nRTSThreshold;		/* 0..2312 */
	int		nRTSRetries;		/* 0..? */
	int		nFragmentThreshold;	/* 256..2312 */
	int		nFragmentRetries;	/* 0..? */

	int		nDateLength;		/* test purpose, 1..2312 */
	int		nDebugCode;			/* debug purpose only */
	int		nRSSICode;			/* debug RSSI value */
	int		nTransmitCode;		/* debug TX command */
	int		nReceiveCode;		/* debug RX command */
	U8		szTargetAddress[IEEE_ADDR_LEN];

	int		nTxTimeout;
	int		nRxTimeout;	
}	TRANS_PARAM, *PTRANS_PARAM;
#endif

typedef	enum {
	SID_HW_STATUS,
	SID_OP_MODE,
	SID_INT_MASK,
	SID_INT_GEN_MASK,
	SID_PORT_BASE_ADDRESS,
	SID_PHY_NUMBER,
	SID_MEDIA_SUPPORTED,
    SID_MEDIA_IN_USE,
	SID_MEDIA_CONNECTION_STATUS,

	SID_MAXIMUM_LOOKAHEAD,
	SID_MAXIMUM_FRAME_SIZE,
    SID_MAXIMUM_TOTAL_SIZE,
    SID_BUFFER_SIZE,
    SID_MAXIMUM_SEND_PACKETS,
    SID_LINK_SPEED,


	SID_GEN_MAC_OPTIONS,
	SID_802_3_PERMANENT_ADDRESS,
	SID_802_3_CURRENT_ADDRESS,
	SID_802_3_MAXIMUM_LIST_SIZE,
	SID_802_3_MULTICAST_LIST,
	SID_GEN_CURRENT_PACKET_FILTER,
	SID_GEN_TRANSMIT_BUFFER_SPACE,
	SID_GEN_RECEIVE_BUFFER_SPACE,
	SID_GEN_TRANSMIT_BLOCK_SIZE,
	SID_GEN_RECEIVE_BLOCK_SIZE,
	SID_GEN_VENDOR_ID,
	SID_GEN_VENDOR_DESCRIPTION,
	SID_GEN_CURRENT_LOOKAHEAD,
	SID_GEN_DRIVER_VERSION,
	SID_GEN_VENDOR_DRIVER_VERSION,
	SID_GEN_PROTOCOL_OPTIONS,

	SID_SIZE
} DEVICE_SID_TYPE;

typedef	enum {
	TID_GEN_XMIT_OK,
	TID_GEN_RCV_OK,
	TID_GEN_XMIT_ERROR,
	TID_GEN_RCV_ERROR,
	TID_GEN_RCV_NO_BUFFER,
	TID_GEN_RCV_CRC_ERROR,

    TID_802_3_RCV_ERROR_ALIGNMENT,
	TID_802_3_RCV_OVERRUN,
    TID_802_3_XMIT_ONE_COLLISION,
    TID_802_3_XMIT_MORE_COLLISIONS,
    TID_802_3_XMIT_DEFERRED,
    TID_802_3_XMIT_MAX_COLLISIONS,
    TID_802_3_XMIT_UNDERRUN,
    TID_802_3_XMIT_HEARTBEAT_FAILURE,
    TID_802_3_XMIT_TIMES_CRS_LOST,
    TID_802_3_XMIT_LATE_COLLISIONS,

	TID_NIC_RXPS,
	TID_NIC_RXCI,
	
	TID_RX_FIFO_OVERFLOW,
	TID_RX_FIFO_OVERFLOW_OVERFLOW,
	TID_SIZE
}	DEVICE_TID_TYPE;

typedef	enum
{
	EID_MAC_ADDRESS,
	EID_VENDOR_ID,
	EID_PRODUCT_ID,
	EID_SIZE
}	DEVICE_EID_TYPE;

typedef	enum
{
	NIC_IND_TX_IDLE=0,
	AID_ERROR=0x8000,
	AID_LARGE_INCOME_PACKET,
} NIC_IND_TYPE;

typedef	struct	_DATA_BLOCK
{
	CQUEUE_GEN_HEADER	Header;
	unsigned char		Buffer[DRIVER_BUFFER_SIZE];
} DATA_BLOCK, *PDATA_BLOCK;

/******************************************************************************************
 *
 * NIC_DRIVER_OBJECT definition
 *
 *******************************************************************************************/
class	NIC_DEVICE_OBJECT;
class	NIC_DRIVER_OBJECT
{
public:
	NIC_DRIVER_OBJECT(NDIS_HANDLE	pHandle,NDIS_HANDLE	pWrapper)
	{
		m_NdisHandle = pHandle;
		m_NdisWrapper = pWrapper;
		m_bSystemHang = 0;
		m_bOutofResources = 0;
		m_pLower = NULL;
	};

	~NIC_DRIVER_OBJECT()
	{
	};

public:

	// routines that may throw exceptions
	virtual void EDriverInitialize(
		OUT PNDIS_STATUS OpenErrorStatus,
		OUT PUINT SelectedMediaIndex, 
		IN PNDIS_MEDIUM MediaArray, 
		IN UINT MediaArraySize);

	// routines that return void or error code
	virtual void	DriverStart(void);
	virtual void	DriverIndication(U32);
	virtual	void	DriverSendCompleted(PCQUEUE_GEN_HEADER);
	
	virtual	void	DriverReceiveIndication(int,PVOID,int);
	
	NDIS_HANDLE	GetNdisHandle() { return m_NdisHandle; };
	NDIS_HANDLE	GetNdisWrapper() { return m_NdisWrapper; };
	U32	GetRecentInterruptStatus() { return m_uRecentInterruptStatus; };
	
	PVOID	DriverBindAddress(U32,U32);

	// miniport driver callback
	virtual	void DriverIsr(
		OUT PBOOLEAN InterruptRecognized, 
		OUT PBOOLEAN QueueInterrupt);

	virtual	VOID	DriverInterruptHandler(void);

	virtual NDIS_STATUS DriverQueryInformation(
		IN NDIS_OID		Oid,
		IN PVOID		InfoBuffer, 
		IN ULONG		InfoBufferLength, 
		OUT PULONG		BytesWritten,
		OUT PULONG		BytesNeeded);

	virtual NDIS_STATUS DriverSetInformation(
		IN NDIS_OID		Oid,
		IN PVOID		InfoBuffer, 
		IN ULONG		InfoBufferLength, 
		OUT PULONG		BytesRead,
		OUT PULONG		BytesNeeded);

	virtual	VOID	DriverEnableInterrupt(void);

	virtual	VOID	DriverDisableInterrupt(void);

	virtual	BOOL	DriverCheckForHang(void);

	virtual	VOID	DriverHalt(void);

	virtual	NDIS_STATUS DriverReset(OUT PBOOLEAN);

	virtual	NDIS_STATUS	DriverSend(IN PNDIS_PACKET,IN UINT);
	
	virtual int DriverIsOutOfResource(void)
	{
		return m_bOutofResources;
	};
	
public:

	int		m_bSystemHang;
	int		m_bOutofResources;
	
	NIC_DEVICE_OBJECT	*m_pLower;

	U32	m_uRecentInterruptStatus;
	NDIS_HANDLE	m_NdisHandle;
	NDIS_HANDLE	m_NdisWrapper;

	CQueue	m_TQueue;
	
	
};

/*********************************************************************************
 *
 * External functions
 *
 *********************************************************************************/
extern "C"
BOOL VirtualCopy ( 
LPVOID lpvDest, 
LPVOID lpvSrc, 
DWORD cbSize, 
DWORD fdwProtect );


extern "C" NIC_DEVICE_OBJECT	*DeviceEntry(
	NIC_DRIVER_OBJECT	*pDriverObject,
	PVOID				pVoid);

#endif	// of __PROJECT_H__
