/*********************************************************************************
 * 
 * $Id: device.h,v 1.1.1.1 2007/04/16 03:45:52 bill Exp $
 *
 * File: Device.h
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 *********************************************************************************/

#ifndef	__WINCE_DEVICE_H__
#define	__WINCE_DEVICE_H__

#include	"common.h"
#include	"driver.h"

typedef	enum {
	BYTE_MODE=1,
	WORD_MODE=2,
	DWORD_MODE=4,
} DEVICE_IO_MODE;

typedef	enum {
	NIC_HW_OK,
	NIC_HW_TX_IDLE,
	NIC_HW_TX_BUSY,
} NIC_HW_STATUS_TYPE;

#ifdef	IMPL_STATISTICS
#define	REPORT(evt,val)	if((val)) DeviceReportStatistics(evt,val)
#else
#define	REPORT(evt,val)
#endif

#ifdef	IMPL_RESET
#define	CHECK_SHUTDOWN()	if(m_bShutdown) break
#else
#define	CHECK_SHUTDOWN()
#endif

/********************************************************************************
 *
 * EEPROM 93C46 Parameters
 *
 ********************************************************************************/
#define	BITS_PER_BYTE	8

#define EEPROM_93C46_BITS				1024
#define EEPROM_93C46_SIZE				(EEPROM_93C46_BITS/BITS_PER_BYTE)
#define EEPROM_93C46_MAX_CYCLES			25
#define	EEPROM_93C46_ADDRESS_BITS		6
#define	EEPROM_93C46_DATA_BITS			16
#define EEPROM_93C46_DELAY				20
typedef	unsigned short EEPROM_93C46_DATA_TYPE;

/********************************************************************************
 *
 * EEPROM Parameters
 *
 ********************************************************************************/

#define	EEPROM_SIZE			EEPROM_93C46_SIZE
#define	EEPROM_MAX_CYCLES	EEPROM_93C46_MAX_CYCLES
#define	EEPROM_DELAY		EEPROM_93C46_DELAY
#define	EEPROM_ADDRESS_BITS EEPROM_93C46_ADDRESS_BITS
#define	EEPROM_DATA_BITS 	EEPROM_93C46_DATA_BITS
#define	EEPROM_DATA_TYPE	EEPROM_93C46_DATA_TYPE

/********************************************************************************
 *
 * PHY Parameters
 *
 ********************************************************************************/

#define MII_INTERNAL_PHY_ADDR		1
#define	MII_PHY_ADDR_LEN	5
#define	MII_REG_ADDR_LEN	5
#define	MII_DELAY			20
#define	MII_PREAMBLES		32

#define MII_OFFSET_BMCR		0x00
#define MII_OFFSET_BMSR		0x01
#define MII_OFFSET_OUI_M	0x02
#define MII_OFFSET_OUI_L	0x03
#define MII_OFFSET_ANAR		0x04
#define MII_OFFSET_ALPAR	0x05

/********************************************************************************
 *
 * Configuration definitions
 *
 ********************************************************************************/

#define CONNECTION_NONE             0x00000000
#define CONNECTION_AUTO             0x00000001
#define CONNECTION_10_HALF          0x00000002
#define CONNECTION_10_FULL          0x00000003
#define CONNECTION_100_HALF         0x00000004
#define CONNECTION_100_FULL         0x00000005
#define CONNECTION_1M8_HPNA         0x00000100
#define CONNECTION_10M8_HPNA        0x00000200

extern "C" void DeviceTimerTrunkRoutine(LPVOID,LPVOID,LPVOID,LPVOID);

/******************************************************************************
 *
 * . about the DeviceOnSetupFilter
 *
 *	Basically, filter modification is one memoryless task. The driver needs not
 *	to remember the last state of filter. It simply sets the filter to meet
 *	the request.
 *	The driver object will call DeviceOnSetupFilter at the late stage of
 *	EDeviceInitialize for filter reset. The device object need call this
 *	function at DeviceReset routine if the reset action will impact the filter.
 *	The driver will later call this routine when NDIS submits set request.
 *
 *	Argument zero means reset the filter, the routine should set the NIC in
 *	unicast mode only and clear the multicast list and counts.
 *
 *****************************************************************************/
class	NIC_DEVICE_OBJECT
{
public:
	NIC_DEVICE_OBJECT(NIC_DRIVER_OBJECT*	pUpper,PVOID)
	{
		m_pUpper = pUpper;
		m_nResetCounts = 0;
		m_nMulticasts = 0;
	  m_nOldLinkState = 0;
	
		memset((void*)&m_szEepromFormat,0,sizeof(m_szEepromFormat));
		memset((void*)&m_szStatistics,0,sizeof(m_szStatistics));
		memset((void*)&m_szLastStatistics,0,sizeof(m_szLastStatistics));
		memset((void*)&m_szConfigures,0xFF,sizeof(m_szConfigures));
		memset((void*)&m_szCurrentSettings,0,sizeof(m_szCurrentSettings));

#ifdef	IMPL_RESET
		m_bShutdown=0;
#endif	
	};

	~NIC_DEVICE_OBJECT()
	{
	};

public:

	void	DeviceReportStatistics(U32,U32);
	U32		DeviceCalculateCRC32(PU8,int,BOOL bReservse=TRUE);

	// device attributes or characteristics
	virtual	U32		DevicePCIID(void)=0;
	virtual	PU8		DeviceMacAddress(PU8);
	virtual	U16		DeviceVendorID(void);
	virtual	U16		DeviceProductID(void);
	virtual	PCONFIG_PARAMETER	DeviceConfigureParameters(void)=0;
	virtual	void	DeviceSetDefaultSettings(void);
	virtual	void	DeviceSetEepromFormat(void)=0;
	virtual	void	DeviceRegisterAdapter(void);
	virtual	BOOL	DeviceQueryInformation(
		OUT NDIS_STATUS		*Status,
		IN NDIS_OID		Oid,
		IN PVOID		InfoBuffer, 
		IN ULONG		InfoBufferLength, 
		OUT PULONG		BytesWritten,
		OUT PULONG		BytesNeeded){ return FALSE; }
	virtual	BOOL	DeviceSetInformation(
		OUT NDIS_STATUS		*Status,
		IN NDIS_OID		Oid,
		IN PVOID		InfoBuffer, 
		IN ULONG		InfoBufferLength, 
		OUT PULONG		BytesRead,
		OUT PULONG		BytesNeeded){ return FALSE; }
	
	// device control routines
	virtual	void	EDeviceInitialize(int)=0;
	virtual	void	DeviceRetriveConfigurations(NDIS_HANDLE);
	virtual void	EDeviceValidateConfigurations(void)=0;
	virtual	void	EDeviceRegisterIoSpace(void);
	virtual	void	EDeviceRegisterInterrupt(void);
	virtual void	DeviceStart(void)=0;
	virtual int		DeviceOnSetupFilter(U32)=0;
	virtual	void	DeviceHalt(void)=0;

	// device access routines
	virtual U32		DeviceReadPort(U32 Port)=0;
	virtual U32		DeviceWritePort(U32 Port,U32 Value)=0;
	virtual	U16		DeviceReadEeprom(U32 uWordAddr)=0;
	virtual	U16		DeviceWriteEeprom(U32 uWordAddr,U16 uValue)=0;
	virtual U16		DeviceReadPhy(U32 nPhy,U32 nOff)=0;
	virtual U16		DeviceWritePhy(U32 nPhy,U32 nOff,U16)=0;
	virtual	void	EDeviceLoadEeprom(void);


	virtual int	DeviceSend(PCQUEUE_GEN_HEADER) = 0;
	virtual BOOL	DeviceCheckForHang(void);
	virtual	U32		DeviceHardwareStatus(void) { return NIC_HW_OK; };
	virtual	void	DeviceReset(void)=0;
	virtual	void	DeviceResetPHYceiver(void)=0;

	virtual	void DeviceEnableInterrupt(void)=0;
	virtual	void DeviceDisableInterrupt(void)=0;
	virtual	void DeviceEnableReceive(void)=0;
	virtual	void DeviceDisableReceive(void)=0;
	virtual	void DeviceEnableTransmit(void){};
	virtual	void DeviceDisableTransmit(void){};
	virtual	U32 DeviceGetInterruptStatus(void)=0;
	virtual	U32 DeviceSetInterruptStatus(U32)=0;
	virtual	U32	DeviceGetReceiveStatus(void)=0;
	virtual	void DeviceInterruptEventHandler(U32)=0;

#ifdef	IMPL_HOOK_INDICATION
	virtual void	DeviceIndication(U32)=0;
	virtual	void	DeviceSendCompleted(PCQUEUE_GEN_HEADER)=0;
	virtual	void	DeviceReceiveIndication(int,PVOID,int)=0;
#else
#define	DeviceIndication(u) m_pUpper->DriverIndication(u)
#define DeviceSendCompleted(p) m_pUpper->DriverSendCompleted(p)
#define DeviceReceiveIndication(a,b,c) \
		 m_pUpper->DriverReceiveIndication(a,b,c)
#endif

	BOOL	DevicePolling(
		U32		uPort,
		U32		uMask,
		U32		uExpected,
		U32		uInterval=20,	/* in micro-second */
		U32		uRetries=WAIT_FOREVER);

	// Device timer routine
	virtual	void	DeviceOnTimer(void);
	virtual void	DeviceInitializeTimer(void);
	virtual void	DeviceCancelTimer(void);
	virtual void	DeviceSetTimer(U32 milliseconds);

#ifdef	IMPL_DEVICE_ISR
	virtual	void	DeviceIsr(U32)=0;
#endif

	virtual	int	DeviceQueryTxResources(void)=0;
	
public:

	class NIC_DRIVER_OBJECT*	m_pUpper;
	
	BOOL		m_fTxEnabled;

	CQueue	m_TQStandby;
	CQueue	m_TQWaiting;
	
	CMutex	m_mutexRxValidate;
	CMutex	m_mutexTxValidate;
	
	U32		m_szConfigures[CID_SIZE];
	U32		m_szEepromFormat[EID_SIZE];
	U32		m_szCurrentSettings[SID_SIZE];
	U32		m_szStatistics[TID_SIZE];
	U32		m_szLastStatistics[TID_SIZE];
	U8		m_szMulticastList[MAX_MULTICAST_LIST][ETH_ADDRESS_LENGTH];
	int		m_nMulticasts;
	int		m_nResetCounts;
	int 		m_nOldLinkState;

#ifdef	IMPL_RESET
	int		m_bShutdown;
#endif
		
	NDIS_MINIPORT_INTERRUPT	m_InterruptHandle;
	

	CMutex	m_mutexTimer;
	NDIS_MINIPORT_TIMER	m_timerObject;

protected:
	void SetConnectionStatus(bool bConnected);

private:
	
	CMutex	m_mutexEEPROM;
	U8		m_szEeprom[EEPROM_SIZE];
};

#endif