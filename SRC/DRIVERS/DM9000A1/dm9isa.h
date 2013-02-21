/********************************************************************************
 * 
 * $Id: dm9isa.h,v 1.2 2007/07/10 04:14:37 bill Exp $
 *
 * Copyright (c) 2000-2005 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/

#ifndef	__DM9000_H__
#define	__DM9000_H__


#include	"common.h"
#include	"driver.h"
#include	"device.h"

#define DM9000_VID		0x0A46
#define	DM9000_PID		0x9000

#define	DM9000_ADDR_OFFSET		0x00
#define	DM9000_DATA_OFFSET		0x04

#define IMPL_FLOW_CONTROL

typedef	enum {
	DM9_NCR = 0,
	DM9_NSR,
	DM9_TXCR,
	DM9_TXSR1,
	DM9_TXSR2,
	DM9_RXCR,
	DM9_RXSR,
	DM9_ROCR,
	DM9_BACKTH,
	DM9_PAUSETH,
	DM9_FLOW,
	DM9_EPCNTL,	/* 0x0B */
	DM9_EPADDR,
	DM9_EPLOW,
	DM9_EPHIGH,
	DM9_WCR,
	
	DM9_PAR0 = 0x10,
	DM9_PAR1,
	DM9_PAR2,
	DM9_PAR3,
	DM9_PAR4,
	DM9_PAR5,

	DM9_MAR0 = 0x16,
	DM9_MAR1,
	DM9_MAR2,
	DM9_MAR3,
	DM9_MAR4,
	DM9_MAR5,
	DM9_MAR6,
	DM9_MAR7,
		
	DM9_GPCR = 0x1E,
	DM9_GPR,

	DM9_TRAL = 0x22,
	DM9_TRAH = 0x23,
	
	DM9_VIDL = 0x28,
	DM9_VIDH,
	DM9_PIDL,
	DM9_PIDH,
	
	DM9_CHIPREV = 0x2C,
	
	DM9_MRCMDX = 0xF0,
	DM9_MRCMD = 0xF2,
	DM9_MDRAH = 0xF4,
	DM9_MDRAL,
	

	DM9_MWCMDX = 0xF6,
	DM9_MWCMD = 0xF8,
	DM9_MDWAL = 0xFA,
	DM9_MDWAH = 0xFB,
	
	DM9_TXLENL = 0xFC,
	DM9_TXLENH,

	DM9_ISR = 0xFE,
	DM9_IMR
		
} DM9000_REGISTER_TYPE;

/******************************************************************************************
 *
 * Definition of C_DM9000
 *
 *******************************************************************************************/
#define	DM9_MULTICAST_LIST	64

typedef	struct	{
	U8		bState;
	U8		bStatus;
	U16		nLength;
} DM9_RX_DESCRIPTOR, *PDM9_RX_DESCRIPTOR;

/*******************************************************************************
 *
 * Device DM9000 characteristics
 *
 * - ISA device, program IO.
 *   Note: The theoretical maximum transfer rate of one ISA is only 8MBps.
 * - On chip 16K byte memory, 3K for TX and 13K for RX.
 * - MAC is responsible to maintain the buffer chain.
 * - Provide word-level interface for EEPROM and PHY access.
 *
 *******************************************************************************/

class	C_DM9000 : public NIC_DEVICE_OBJECT
{
public:
	C_DM9000::C_DM9000(NIC_DRIVER_OBJECT	*pUpper,PVOID pVoid) 
		: NIC_DEVICE_OBJECT(pUpper,pVoid)
	{
		m_uLastAddressPort = (U32)-1;
	};

	// overwrite routines with exception
	virtual void	EDeviceInitialize(int);
	
	// overwrite this routine for identification purpose
	virtual	void	EDeviceRegisterIoSpace(void);

	// Device attributes and characteristics
	virtual U32		DevicePCIID( void) { return 0L; };
	virtual	PCONFIG_PARAMETER	DeviceConfigureParameters(void);
	virtual void	EDeviceValidateConfigurations(void);
	virtual	void	DeviceSetEepromFormat(void);

	
	// Device access routines
	virtual U32		DeviceReadPort( U32 uPort);
	virtual U32		DeviceWritePort( U32 uPort, U32 uValue);
	virtual	U16		DeviceReadEeprom(U32 uWordAddr);
	virtual	U16		DeviceWriteEeprom(U32 uWordAddr,U16 uValue);
	virtual U16		DeviceReadPhy(U32 nPhy,U32 nOff);
	virtual U16		DeviceWritePhy(U32 nPhy,U32 nOff,U16);
	
	// Device access routines, new 
	virtual	U32		DeviceReadData(void);
	virtual	U32		DeviceReadDataWithoutIncrement(void);
	virtual	PU8		DeviceReadString(PU8,int);
	virtual	PU8		DeviceWriteString(PU8,int);

	// Device control routines
	virtual void	DeviceStart( void);
	virtual void	DeviceReset( void);
	virtual void 	DeviceEnableInterrupt( void);
	virtual void 	DeviceDisableInterrupt( void);
	virtual U32 	DeviceGetInterruptStatus( void);
	virtual U32 	DeviceSetInterruptStatus( U32);
	virtual U32		DeviceGetReceiveStatus( void);
	virtual	U32		DeviceHardwareStatus(void);
	virtual	void 	DeviceEnableReceive(void);
	virtual	void 	DeviceDisableReceive(void);

	// There is no hardware control for transmission,
	// so, need to set one flag to control the tx.
	virtual	void 	DeviceEnableTransmit(void){ m_fTxEnabled=1; };
	virtual	void 	DeviceDisableTransmit(void){ m_fTxEnabled=0; };

	// Device hanlder routines
	virtual int		DeviceOnSetupFilter( U32);
	virtual void 	DeviceInterruptEventHandler( U32);
	virtual void 	DeviceResetPHYceiver(void);
	virtual int 	DeviceSend(PCQUEUE_GEN_HEADER);

#ifdef	IMPL_HOOK_INDICATION
	// hooked function
	virtual void	DeviceIndication(U32)=0;
	virtual	void	DeviceSendCompleted(PCQUEUE_GEN_HEADER)=0;
	virtual	void	DeviceReceiveIndication(int,PVOID,int)=0;
#endif

	// class specific routines
	virtual	int		Dm9LookupRxBuffers(void);
	
#ifdef	IMPL_DEVICE_ISR
	virtual	void	DeviceIsr(U32);
#endif
		
	virtual	void	DeviceHalt(void){/* nothing to do */;};
public:

	CSpinlock	m_spinAccessToken;
	U32			m_uLastAddressPort;

	int		m_nIoMode;
	int		m_nIoMaxPad;

#ifdef	IMPL_STORE_AND_INDICATION

	virtual	void	DeviceOnTimer(void);

	CQueue	m_RQueue;
	CQueue	m_RQStandby;

#endif

	//
	// 3.2.9 revised
	virtual	int	DeviceQueryTxResources(void)
	{
		return (m_nTxPendings < m_nMaxTxPending);
	}
	
	int	m_nMaxTxPending;	
	int	m_nTxPendings;
	//int	m_nTx;
};

#endif	//	__DM9000_H__
