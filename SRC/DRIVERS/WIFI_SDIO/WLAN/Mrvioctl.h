/******************* (c) Marvell Semiconductor, Inc., 2004 ********************
 *
 *  Purpose:
 *
 *      This file contains definitions and data structures that are specific
 *      to conduct IOCTL calls from user application.
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/08/23 $
 *
 *	$Revision: #2 $
 *     	     
 *
 *****************************************************************************/

#ifndef _MRVIOCTL_H_
#define _MRVIOCTL_H_

//
//	802.11-related NDIS definitions needed
//
//#define MODE_802_11B		1
//#define MODE_802_11G		0
#define MAX_NUM_DATA_RATES	14

/*
#define ETH_ADDR_LENGTH 	6
typedef char ETH_ADDR[ETH_ADDR_LENGTH];

typedef struct _ETH_HEADER {

	ETH_ADDR DestAddr;
	ETH_ADDR SrcAddr;
	USHORT Type;	// type or length
} ETH_HEADER, *PETH_HEADER;
*/

//
// Simple Mutual Exclusion constructs used in preference to
// using KeXXX calls since we don't have Mutex calls in NDIS.
// These can only be called at passive IRQL.
//

#define MODULE_NUMBER 'M'
typedef struct _IOCTL_MUTEX
{
    ULONG                   Counter;
    ULONG                   ModuleAndLine;  // useful for debugging

} IOCTL_MUTEX, *PIOCTL_MUTEX;

#define IOCTL_INIT_MUTEX(_pMutex)                                 \
{                                                               \
    (_pMutex)->Counter = 0;                                     \
    (_pMutex)->ModuleAndLine = 0;                               \
}

#define IOCTL_ACQUIRE_MUTEX(_pMutex)                              \
{                                                               \
    while (NdisInterlockedIncrement(&((_pMutex)->Counter)) != 1)\
    {                                                           \
        NdisInterlockedDecrement(&((_pMutex)->Counter));        \
		NdisMSleep(10000);                                      \
    }                                                           \
    (_pMutex)->ModuleAndLine = (MODULE_NUMBER << 16) | __LINE__;\
}

#define IOCTL_RELEASE_MUTEX(_pMutex)                              \
{                                                               \
    (_pMutex)->ModuleAndLine = 0;                               \
    NdisInterlockedDecrement(&(_pMutex)->Counter);              \
}

#define IOCTL_MRVL_QUERY_OID_VALUE   \
CTL_CODE(FILE_DEVICE_NETWORK, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MRVL_SET_OID_VALUE   \
CTL_CODE(FILE_DEVICE_NETWORK, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MRVL_PROVIDE_EVENT_HANDLE   \
CTL_CODE(FILE_DEVICE_NETWORK, 0x920, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MRVL_UNLOCK_PAGES   \
CTL_CODE(FILE_DEVICE_NETWORK, 0x930, METHOD_BUFFERED, FILE_ANY_ACCESS)
	
#define MAX_BUFFER_SIZE	4096 

// Right now design only supports 4 devices
#define MAX_NUM_OF_DEVICES 4

#define DRVR_DEVICE_NAME0 L"\\Device\\Mrvl8xxx0"
#define DRVR_LINK_NAME0 L"\\DosDevices\\MrvlDev0"

#define DRVR_DEVICE_NAME1 L"\\Device\\Mrvl8xxx1"
#define DRVR_LINK_NAME1 L"\\DosDevices\\MrvlDev1"

#define DRVR_DEVICE_NAME2 L"\\Device\\Mrvl8xxx2"
#define DRVR_LINK_NAME2 L"\\DosDevices\\MrvlDev2"

#define DRVR_DEVICE_NAME3 L"\\Device\\Mrvl8xxx3"
#define DRVR_LINK_NAME3 L"\\DosDevices\\MrvlDev3"


#define COMMAND_EVENT_ASSOC_FAIL			0
#define COMMAND_EVENT_ASSOC_SUCCESS			1
#define COMMAND_EVENT_EXIT					2
#define	COMMAND_EVENT_ADHOC_SUCCESS			3
#define	COMMAND_EVENT_ADHOC_FAIL			4
#define	COMMAND_EVENT_DISCONNECT			5
#define COMMAND_EVENT_BSSID_LIST			6
#define COMMAND_EVENT_READ_REG_DONE			7

#pragma pack(1)
typedef struct _MRVL_QUERY_OID
{
    NDIS_OID        Oid;
	UCHAR           Data[2*sizeof(ULONG)];

} MRVL_QUERY_OID, *PMRVL_QUERY_OID;
	
typedef struct _MRVL_SET_OID
{
    NDIS_OID        Oid;
    UCHAR           Data[2*sizeof(ULONG)];

} MRVL_SET_OID, *PMRVL_SET_OID;

typedef struct _MRVL_DATA_BUFFER
{
	HANDLE			Event;
    ULONG		  	BufferSize;
	PUCHAR			Buffer;

} MRVL_DATA_BUFFER, *PMRVL_DATA_BUFFER;
#pragma pack()

BOOLEAN SurprisedRemoved;
VOID
MrvlNotifyApplication(
	IN ULONG	Command,
	IN PUCHAR	DataSource,
	IN ULONG	DataSize
	);

#if DBG 
extern char *oidToString( ULONG oid );
#define OID_TO_STRING(oid) oidToString(oid)
#else
#define OID_TO_STRING(oid)
#endif
#endif
