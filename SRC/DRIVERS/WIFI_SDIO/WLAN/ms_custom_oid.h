#ifndef _MS_CUSTOM_OID_H_
#define _MS_CUSTOM_OID_H_


#include <ntddndis.h>

#ifdef __cplusplus
extern "C" {	
#endif

#define	OID_MS_802_11_RADIO_STATUS		0xFF000001
#define	OID_MS_802_11_SINGLE_LIST_SCAN	0xFF000002
#define	OID_MS_802_11_SET_CUSTOM_IE		0xFF000003
#define	OID_MS_802_11_BSSID_LIST		       0xFF000004
#define	OID_MS_802_11_BSSID				0xFF000005
#define	OID_MS_802_11_DISCONNECT		0xFF000006
#define	OID_MS_802_11_SET_PS_PARAMS_DISCOVERY	0xFF000007
#define	OID_MS_802_11_SINGLE_LIST_SCAN_BURST	0xFF000008
#define	OID_MS_802_11_BSSID_BURST		0xFF000009
#define	OID_MS_802_11_ALLOW_SLEEP_MODE	0xFF000010	// PS_DJ	2006/08/04
#define	OID_MS_802_11_SLEEP_BROADCAST	0xFF000011	// PS_DJ	2006/08/04
#define	OID_MS_802_11_SLEEP				0xFF000012
#define	OID_MS_802_11_GET_STATS			0xFF000013
#define	OID_MS_802_11_DISCOVERY_MODE      0xFF000014
#define OID_MS_802_11_BSSID_BURST_EX      0xFF000015

#define OID_MS_802_11_DUTY_CYCLE_TABLE          0xFF000024
#define OID_MS_802_11_DUTY_CYCLE_STATS          0xFF000025

#define OID_MS_802_11_PS_STATE                   0xFF000020
// Last OID I know of was 0xFF000014, so I skipped ahead to ...20
//This OID is defined for both SET and GET.

#define   MS_802_11_PS_STATE_D0  0x0
#define   MS_802_11_PS_STATE_D1  0x1
#define   MS_802_11_PS_STATE_D2  0x2
#define   MS_802_11_PS_STATE_D3  0x3
#define   MS_802_11_PS_STATE_D4  0x4

#define   MS_RADIO_OFF           0x00
#define   MS_RADIO_ON            0x01


//InformationBuffer
typedef struct _NDIS_MS_802_11_PS_STATE
{
UCHAR 	Length;  //in bytes of this InfoBuf
UCHAR	  State;  // contains desired state on SET, current state on GET.
}  NDIS_MS_802_11_PS_STATE, *PNDIS_MS_802_11_PS_STATE;




#define	OID_MS_802_11_SET_PASSIVE_DISCOVERY		0xFF000021
// This OID has no attached InformationBuffer

#define	OID_MS_802_11_WIRELESS_MODE		        0xFF000022
//This OID is defined for both SET and GET/Query.

#define OID_MS_802_11_ACKNOWLEDGE_DEVICE_WAKE   0xFF000023

#define MS_802_11_MODE_IEEE  0x0
#define MS_802_11_MODE_MSFT_ZUNE  0x1

//InformationBuffer
typedef struct _NDIS_MS_802_11_WIRELESS_MODE
{
UCHAR 	length;  //in bytes of this InfoBuf
UCHAR	mode;  // contains desired mode on SET, current mode on GET.
}  NDIS_MS_802_11_WIRELESS_MODE, *PNDIS_MS_802_11_WIRELESS_MODE;



typedef ULONG	NDIS_MS_802_11_RADIO_STATUS;

typedef ULONG	NDIS_MS_802_11_SINGLE_LIST_SCAN;

typedef struct _NDIS_MS_802_11_CUSTOM_IE
{
	UCHAR						elementID;	// Vendor specific information element(221)
	UCHAR						length;
	UCHAR						oui[3];		// Microsoft OUI(00-50-f2)
	UCHAR						type;		// OUI Type(6)
	UCHAR						format[4];	// Hash(Pyxis URL)
	UCHAR						data[241];  // Pyxis specific data
} NDIS_MS_802_11_CUSTOM_IE, *PNDIS_MS_802_11_CUSTOM_IE;

typedef struct _NDIS_MS_802_11_BSSID_LIST
{
	NDIS_WLAN_BSSID_EX			bssid_info;
	NDIS_MS_802_11_CUSTOM_IE	custom_ie;
} NDIS_MS_802_11_BSSID_LIST, *PNDIS_MS_802_11_BSSID_LIST;

typedef NDIS_802_11_MAC_ADDRESS	NDIS_MS_802_11_BSSID;

typedef NDIS_802_11_MAC_ADDRESS	NDIS_MS_802_11_DISCONNECT;

typedef struct _NDIS_MS_802_11_SET_PS_PARAMS_DISCOVERY
{
	ULONG			PyxisDiscoveryInterval;
	ULONG			PyxisDiscoveryWindow;
	ULONG			ProbeInterval;
	ULONG			PyxisProbePeriod;
} NDIS_MS_802_11_SET_PS_PARAMS_DISCOVERY, *PNDIS_MS_802_11_SET_PS_PARAMS_DISCOVERY;

typedef ULONG	NDIS_MS_802_11_SINGLE_LIST_SCAN_BURST;

typedef NDIS_802_11_MAC_ADDRESS	NDIS_MS_802_11_BSSID_BURST;

typedef struct _NDIS_MS_802_11_BSSID_BURST_EX
{
    NDIS_MS_802_11_BSSID      BSSID;	
	  NDIS_802_11_MAC_ADDRESS   MacAddress;
} NDIS_802_11_BSSID_BURST_EX, *PNDIS_802_11_BSSID_BURST_EX;	

typedef BOOL	NDIS_MS_802_11_ALLOW_SLEEP_MODE;	

typedef ULONG	NDIS_MS_802_11_SLEEP_BROADCAST;		

typedef ULONG	NDIS_MS_802_11_SLEEP;

typedef struct _NDIS_MS_802_11_GET_STATS
{
	ULONG			PowerSaveTime;
	ULONG			SendTime;
	ULONG			ListenTime;
} NDIS_MS_802_11_GET_STATS, *PNDIS_MS_802_11_GET_STATS;

typedef enum _NDIS_MS_802_11_STATUS_SLEEP_INFO
{
	Ndis802_11_StatusType_Authentication,
	NDIS_STATUS_MS_JOIN_FALURE = 0x101,
	NDIS_STATUS_MS_JOIN_MEMBER,
	NDIS_STATUS_MS_SLEEP,
	NDIS_STATUS_MS_AWAKE,
	Ndis802_11_StatusTypeMax
} NDIS_MS_802_11_STATUS_TYPE, *PNDIS_MS_802_11_STATUS_TYPE;

#define	ASSOCIATION_TIMEOUT		3
#define	TOO_MANY_ASSOCIATION	5
typedef struct _NDIS_MS_802_11_JOIN_FAILURE_INFO
{
	NDIS_802_11_MAC_ADDRESS	MacAddr;		// BSSID(参加に失敗したBSSID)
	USHORT	ReasonCode;
} NDIS_MS_802_11_JOIN_FAILURE_INFO, *PNDIS_MS_802_11_JOIN_FAILURE_INFO;

typedef struct _MS_JOIN_FAILURE_INDICATION
{
	NDIS_802_11_STATUS_INDICATION		ind;	// NDIS_STATUS_MS_JOIN_FALURE
	NDIS_MS_802_11_JOIN_FAILURE_INFO	info;
} MS_MS_JOIN_INDICATION, *PMS_MS_JOIN_INDICATION;

typedef struct _NDIS_MS_802_11_JOIN_INFO
{
     NDIS_802_11_MAC_ADDRESS MacAddr;		//MAC Address(新たに参加したPyxisのMACアド?ス)
     USHORT	Length;							//Set IE Length
     UCHAR	CustomIE[1];					//Custom IE(新たに参加したPyxisのCustomIE)
} NDIS_MS_802_11_JOIN_INFO, *PNDIS_MS_802_11_JOIN_INFO;

typedef struct _MS_JOIN_INDICATION
{
	NDIS_802_11_STATUS_INDICATION	ind;	//NDIS_STATUS_MS_JOIN_MEMBER
	NDIS_MS_802_11_JOIN_INFO		info;
} MS_JOIN_INDICATION, *PMS_JOIN_INDICATION;

typedef struct _NDIS_MS_802_11_SLEEP_INFO
{
	ULONG SleepPeriod;						//Sleep Period(ad-hoc formerから指定されたス?ープ期間)
} NDIS_MS_802_11_SLEEP_INFO, *PNDIS_MS_802_11_SLEEP_INFO;

typedef struct _MS_SLEEP_INDICATION
{
	NDIS_802_11_STATUS_INDICATION	ind;	//NDIS_STATUS_MS_SLEEP
	NDIS_MS_802_11_SLEEP_INFO		info;
} MS_SLEEP_INDICATION, *PMS_SLEEP_INDICATION;

typedef enum _NDIS_MS_PYXIS_IE_OPERATION_TYPE
{
	PYXIE_DISCOVERY_REQUEST = 1,
	PYXIE_DISCOVERY_RESPONSE,
	PYXIE_VIRTUAL_ASSOCIATION_REQUEST,
	PYXIE_VIRTUAL_ASSOCIATION_RESPONSE
}NDIS_MS_PYXIS_IE_OPERATION_TYPE, *PNDIS_MS_PYXIS_IE_OPERATION_TYPE;

#define MAX_DUTY_CYCLE_TABLE          12+2 //there are 2 additional reserved fields
#define MAX_DUTY_CYCLE_STATS          128

#pragma pack(1)
typedef struct _NDIS_MS_802_11_DUTY_CYCLE_TABLE
{
    UCHAR 	 length;
    USHORT      usDutyCycleFeatureEnable;

    /*
        The order is...
        1, 2, 5.5, 11, Reserved, 6, 9, 12, 18, 24, 36, 48, 54, Reserved
    */
    UCHAR        Table[MAX_DUTY_CYCLE_TABLE];
} NDIS_MS_802_11_DUTY_CYCLE_TABLE, *PNDIS_MS_802_11_DUTY_CYCLE_TABLE;

typedef struct _NDIS_MS_802_11_DUTY_CYCLE_STATS
{
#if 1
    UCHAR      length;  //in bytes of this InfoBuf
    BOOL        ClearStats;  // If True on calling this OID, all stats will be cleared.  Value on return indicates if stats were actually cleared.
						// Contents of remaining fields are INVALID if ClearStats==True.  Use only to clear the stats.
    DWORD   totalTxTime;  //Total accumulated airtime of all frames transmissions (Includes Frame/PLCP header/Preamble and frame retries) in microseconds.
    DWORD   totalCCATime; //Total accumulated time for which FW forced CCA to prevent packet transmission in microseconds.
    DWORD   lastPktCCATime; //Total time FW forced CCA due to most recent transmission (Cost associated with last Tx) in microseconds.
    DWORD   lastPktInitRate; //Initial Tx Rate of most recently transmitted packet.
    DWORD   lastPktRetries; // Retries of most recently transmitted packet.
#else
    USHORT      usLenOfData;
    UCHAR        Stats[MAX_DUTY_CYCLE_STATS];
#endif
} NDIS_MS_802_11_DUTY_CYCLE_STATS, *PNDIS_MS_802_11_DUTY_CYCLE_STATS;
#pragma pack()


typedef ULONG 	RADIOSTATUS, *PRADIOSTATUS;

#define CUSTOM_CMD_SET       1
#define CUSTOM_CMD_GET       0
#define OP_NORMAL_MODE       0x0001
#define OP_ZUNE_FORMER_MODE  0x0002
#define OP_ZUNE_JOINER_MODE  0x0004
#define OP_ACTIVE_DISCOVERY_MODE      0x0010
#define OP_PASSIVE_DISCOVERY_MODE     0x0020
#define VIRTUAL_ASSOCIATION_REQUEST                   1
#define DISCOVERY_REQUEST                             0
#define ZUNE_DEFAULT_CHANNEL                          11
#define HostCmd_CMD_ADHOC_IE_CONFIG                   0x009f
#define HostCmd_CMD_ADHOC_DISCOVERY_CONFIG            0x00a0
#define HostCmd_CMD_ADHOC_DATA_WIN_CONFIG             0x00a1
#define HostCmd_CMD_ADHOC_NEIGHBOR_QUERY              0x00a2

#define HostCmd_CMD_ADHOC_DUTY_CYCLE_TABLE          0x00a6
#define HostCmd_CMD_ADHOC_DUTY_CYCLE_STATS          0x00a7
#define HostCmd_RET_ADHOC_DUTY_CYCLE_TABLE           0x80a6
#define HostCmd_RET_ADHOC_DUTY_CYCLE_STATS           0x80a7

#define HostCmd_RET_ADHOC_IE_CONFIG                   0x809f
#define HostCmd_RET_ADHOC_DISCOVERY_CONFIG            0x80a0
#define HostCmd_RET_ADHOC_DATA_WIN_CONFIG             0x80a1
#define HostCmd_RET_ADHOC_NEIGHBOR_QUERY              0x80a2
#define ADHOC_PROBE_TLV_PAYLOAD                       8
#define CUSTOM_IE_HEADER_SIZE                         10 
#define CUSTOM_IE_MAC_OFFSET                          48
#define CUSTOM_IE_OPERATION_TYPE_OFFSET               8 
#define MACREG_INT_CODE_ADHOC_STATION_CONNECTED       32
#define MAX_JOIN_RETRY_COUNT                          14

#define DISCOVERY_MODE_SWITCH_TIMEOUT   12000


#define WIAT_EVENT_MODE_SWITCH_NONE                    0
#define WIAT_EVENT_MODE_SWITCH_ACTIVE                  1
#define WIAT_EVENT_MODE_SWITCH_PASSIVE                 2 

#define SCAN_USE_FW_ITERATION                          0
#define SCAN_USE_SW_ITERATION                          1

#define PMIC_IOCTL_WIFI_GPIO_ENABLE            CTL_CODE(                      \
                                                        FILE_DEVICE_UNKNOWN,  \
                                                        0xA00,                \
                                                        METHOD_BUFFERED,      \
                                                        FILE_ANY_ACCESS       \
                                                        )
                                                        
#define CUSTOM_VA_TIMEOUT       2000

#pragma pack(4)

typedef struct _CUSTOM_KEY
{
    NDIS_802_11_KEY  Key;
    UCHAR            KeyData[32];    // KeyMaterial is only 1 byte in NDIS_802_11_KEY
} CUSTOM_KEY, *PCUSTOM_KEY;

typedef struct _MS_ZUNE_INFO
{
	  NDIS_MS_802_11_CUSTOM_IE                 CustomIeList[MRVDRV_MAX_BSSID_LIST];
	  NDIS_MS_802_11_SET_PS_PARAMS_DISCOVERY   PsDiscovery;
	  NDIS_MS_802_11_SINGLE_LIST_SCAN_BURST    BurstScanChannel;
	  NDIS_MS_802_11_BSSID_BURST               AdhocBssidBurst;
 	  NDIS_MS_802_11_GET_STATS			           MSPSStats;
	  NDIS_802_11_MAC_ADDRESS                  ProbeDestinationAddress;
    NDIS_802_11_SSID                         RandomSSID;
        NDIS_DEVICE_POWER_STATE         PnpPowerState;
	  ULONG  WifiModeAdhocChannel;
    ULONG  ScanIterations;
    ULONG  PyxisProbePeriod;
    ULONG  RadioStatus; 
	ULONG  ScanPending;
	ULONG  GetPeerVirtAssocResp;	   
	INT    JoinRetryCount;
	USHORT IsVirtualAssociation;
	USHORT OpMode;
	UCHAR  ZuneMac[6];
	NDIS_MS_802_11_CUSTOM_IE CurrentCustomIe;
	UCHAR  PowerState;
	ULONG  Criteria; 
	UCHAR  GPIO; 
	UCHAR	 Gap;
//Add Wait_Event for the MS_OID_Command    
    HANDLE      OidCompleteEvent;
  ULONG  WaitModeSwitchEventType;  
// This declaration should be last one field in the structure   
    CUSTOM_KEY  CustomAesKey;
    CUSTOM_KEY  CustomEncrAesKey;
    NDIS_MS_802_11_DUTY_CYCLE_TABLE            DutyCycleTable;
    NDIS_MINIPORT_TIMER     hVATimerHandler;
    BOOLEAN                          bVATimerSet;

} MS_ZUNE_INFO, *PMS_ZUNE_INFO;
#pragma pack()


#pragma pack(1)
typedef struct _ADHOC_PROBE_TLV 
{
	  USHORT  Type;
	  USHORT  Length;
	  USHORT  IsVirtualAssociation;
	  NDIS_802_11_MAC_ADDRESS ProbeDestinationAddress;
}ADHOC_PROBE_TLV, *PADHOC_PROBE_TLV;
 	
typedef struct _ZUNE_PYXIS_TLV 
{
	  USHORT  Type;
	  USHORT  Length;
	  UCHAR   PyxisIE[255];
}ZUNE_PYXIS_TLV, *PZUNE_PYXIS_TLV;
typedef struct _HostCmd_ADHOC_IE_CONFIG
{
	USHORT  Command;
	USHORT  Size;
	USHORT  SeqNum;
	USHORT  Result;
	USHORT	Action;

} HostCmd_ADHOC_IE_CONFIG, *PHostCmd_ADHOC_IE_CONFIG;

typedef struct _HostCmd_ADHOC_DISCOVERY_CONFIG 
{
	  USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
	  USHORT	Action;
	  USHORT        Reserved;
	  ULONG   ProbeInterval;
	  ULONG   ProbePeriod;
	  ULONG   DiscoveryWindow;
	  ULONG   DiscoveryInterval;
}HostCmd_ADHOC_DISCOVERY_CONFIG, *PHostCmd_ADHOC_DISCOVERY_CONFIG;

typedef struct _HostCmd_ADHOC_DATA_WIN_CONFIG 
{
	  USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
	  USHORT	Action;
	  ULONG   DataWindowSize;
}HostCmd_ADHOC_DATA_WIN_CONFIG, *PHostCmd_ADHOC_DATA_WIN_CONFIG;

typedef struct _HostCmd_ADHOC_NEIGHBOR_QUERY 
{
	  USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
}HostCmd_ADHOC_NEIGHBOR_QUERY, *PHostCmd_ADHOC_NEIGHBOR_QUERY;

typedef struct _HostCmd_ADHOC_NEIGHBOR_QUERY_RESPONSE 
{
	  USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    NDIS_802_11_MAC_ADDRESS JoinerMac;
    ZUNE_PYXIS_TLV PyxisTlv;
}HostCmd_ADHOC_NEIGHBOR_QUERY_RESPONSE, *PHostCmd_ADHOC_NEIGHBOR_QUERY_RESPONSE;

typedef struct _HostCmd_ADHOC_DUTY_CYCLE_TABLE
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;

    USHORT      Action;
    USHORT      usDutyCycleFeatureEnable;
    UCHAR        Table[MAX_DUTY_CYCLE_TABLE];
} HostCmd_ADHOC_DUTY_CYCLE_TABLE , *PHostCmd_ADHOC_DUTY_CYCLE_TABLE;

typedef struct _HostCmd_ADHOC_DUTY_CYCLE_STATS
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    USHORT  Resverd;
} HostCmd_ADHOC_DUTY_CYCLE_STATS, *PHostCmd_ADHOC_DUTY_CYCLE_STATS;

typedef struct _HostCmd_ADHOC_DUTY_CYCLE_STATS_RESPONSE
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    USHORT  Resverd;
    DWORD   totalTxTime;
    DWORD   totalCCATime;
    DWORD   lastPktCCATime;
    DWORD   lastPktInitRate;
    DWORD   lastPktRetries;
} HostCmd_ADHOC_DUTY_CYCLE_STATS_RESPONSE, *PHostCmd_ADHOC_DUTY_CYCLE_STATS_RESPONSE;

typedef struct _CUSTOM_OID_ATTRIBUTE
{
    NDIS_OID    oid;
    UCHAR       Attribute[5]; // 1: available, 0: unavailable, [PowerState]:bit1-get, bit0-set
} CUSTOM_OID_ATTRIBUTE, *PCUSTOM_OID_ATTRIBUTE;

//#define CUS_OID_ATTR(bGet, bSet)  ( ((UCHAR)bGet<<1) | ((UCHAR)bSet) )

#pragma pack()


NDIS_STATUS HandleSetSingleScan(PMRVDRV_ADAPTER Adapter, 
					                             PVOID InformationBuffer,
					                             ULONG InformationBufferLength, 
					                             OUT PULONG BytesRead,
					                             OUT PULONG BytesNeeded);

NDIS_STATUS HandleSetCustomIE(PMRVDRV_ADAPTER Adapter, 
					                           PVOID InformationBuffer,
					                           ULONG InformationBufferLength, 
					                           OUT PULONG BytesRead,
					                           OUT PULONG BytesNeeded);

static NDIS_STATUS HandleGetRadioStatus(PMRVDRV_ADAPTER Adapter, 
					PVOID InformationBuffer, ULONG InformationBufferLength, 
					OUT PULONG BytesWritten,  OUT PULONG BytesNeeded);

static NDIS_STATUS HandleGetBSSIDList(PMRVDRV_ADAPTER Adapter, 
					PVOID InformationBuffer, ULONG InformationBufferLength, 
					OUT PULONG BytesWritten,  OUT PULONG BytesNeeded);

static NDIS_STATUS HandleGetBSSID(PMRVDRV_ADAPTER Adapter, 
					PVOID InformationBuffer, ULONG InformationBufferLength, 
					OUT PULONG BytesWritten,  OUT PULONG BytesNeeded);


static NDIS_STATUS ProcessOIDPsParamsDiscovery(PMRVDRV_ADAPTER Adapter,
                                           ULONG Action,                                          
                                           PVOID InformationBuffer,
                                           ULONG InformationBufferLength,
                                           PULONG BytesWirte,
                                           PULONG BytesNeeded);

static NDIS_STATUS ProcessOIDSingleListScanBurst(PMRVDRV_ADAPTER Adapter,
                                            PVOID InformationBuffer,
                                            ULONG InformationBufferLength,
                                            PULONG BytesRead,
                                            PULONG BytesNeeded); 

NDIS_STATUS ProcessOIDMSDisconnect( PMRVDRV_ADAPTER Adapter,
                                    PVOID InformationBuffer,
				    ULONG InformationBufferLength,
                                    PULONG BytesRead,
                                    PULONG BytesNeeded); 

static NDIS_STATUS ProcessOIDMSDutyCycleTable(PMRVDRV_ADAPTER Adapter,
                                          ULONG Action,
                                          PVOID InformationBuffer, 
                                          ULONG InformationBufferLength,
                                          PULONG BytesWritten,
                                          PULONG BytesNeeded);

static NDIS_STATUS ProcessOIDMSDutyCycleStat(PMRVDRV_ADAPTER Adapter,
					   PVOID InformationBuffer, 
					   ULONG InformationBufferLength, 
					   OUT PULONG BytesWritten,  
					   OUT PULONG BytesNeeded);

static NDIS_STATUS ProcessOIDMSAckDeviceWake(PMRVDRV_ADAPTER Adapter,
                                          PVOID InformationBuffer, 
                        ULONG InformationBufferLength,
                                          PULONG BytesRead,
                                          PULONG BytesNeeded);

NDIS_STATUS ProcessOIDMSAllowSleep( PMRVDRV_ADAPTER Adapter,
					ULONG action,  
					PULONG BytesWritten,
					ULONG InformationBufferLength,
					IN PVOID InformationBuffer);

NDIS_STATUS ProcessOIDBssidBurst(PMRVDRV_ADAPTER Adapter,
                                 PVOID InformationBuffer,
                                 ULONG InformationBufferLength,
                                 PULONG BytesWritten,
                                 PULONG BytesNeeded);

NDIS_STATUS ProcessOIDBssidBurstEx( PMRVDRV_ADAPTER Adapter,
                                    PVOID InformationBuffer,
                                    ULONG InformationBufferLength,
                                    PULONG BytesRead,
                                    PULONG BytesNeeded); 

NDIS_STATUS ProcessOIDDiscoveryMode (PMRVDRV_ADAPTER Adapter,
					    ULONG Action,                                          
                                            PVOID InformationBuffer,
                                            ULONG InformationBufferLength,
                                            PULONG BytesWritten,
                                            PULONG BytesNeeded);

NDIS_STATUS ProcessOIDMSPowerState(PMRVDRV_ADAPTER Adapter,
 										   ULONG Action,                                          
                       PVOID InformationBuffer,
                       ULONG InformationBufferLength,
                       PULONG BytesWritten,
                       PULONG BytesNeeded);

VOID PrepareAdhocDiscoveryConfigCmd(PHostCmd_DS_GEN pCmdPtr,
                                    USHORT CmdOption,
                                    PUSHORT Size,
                                    PVOID InformationBuffer);

VOID PrepareAdhocDataWinConfigCmd(PHostCmd_DS_GEN pCmdPtr,
                                  USHORT CmdOption,
                                  PUSHORT Size,
                                  PVOID InformationBuffer);

VOID PrepareAdhocNeighborQueryCmd(PHostCmd_DS_GEN pCmdPtr,
                                  USHORT CmdOption,
                                  PUSHORT Size,
                                  PVOID InformationBuffer);


VOID GetRandomSSID(PNDIS_802_11_SSID pSsid);

NDIS_STATUS DiscoveryModeSwitch(PMRVDRV_ADAPTER Adapter,
                                USHORT SwitchMode);

VOID HandleCustomPendOid(USHORT Ret,
                         PMRVDRV_ADAPTER Adapter,
                         PHostCmd_DS_GEN pRetPtr,
                         USHORT PendingInfo );

VOID PrepareAdhocIEConfigCmd(PHostCmd_DS_GEN pCmdPtr,
                                    USHORT CmdOption,
                                    PUSHORT Size,
                                    PVOID InformationBuffer);

BOOL CustomSetPMU(BOOL GpioState);

NDIS_STATUS ZuneHandleD0toDx(PMRVDRV_ADAPTER Adapter, 
                             UCHAR PowerState);
                             
NDIS_STATUS ZuneHandleDxtoD0(PMRVDRV_ADAPTER Adapter, 
                             UCHAR PowerState);
                             
NDIS_STATUS ZuneHandleD2toD4(PMRVDRV_ADAPTER Adapter);


#ifdef __cplusplus
}
#endif

#endif	// _MS_CUSTOM_OID_H_
