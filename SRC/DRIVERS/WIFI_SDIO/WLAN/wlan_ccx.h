/** File: wlan_ccx.h
 *  Description: prototype of CCX function / Data structure
 *  Copyright (c) Marvell Semiconductor, Inc., 
 */
#ifndef _WLAN_CCX_H
#define _WLAN_CCX_H

/// =======================================================
///		Macro Definition
///

#define u8 UCHAR
#define OS_INT8		char
#define OS_UINT8	BYTE
#define OS_UINT16	WORD
#define OS_UINT32	DWORD

#define CCX_TPC_MinPowerValue	5
#define CCX_TPC_MaxPowerValue	17
///CCX_RADIOMEASURE
/// Scan types for Operation ( all others are reserved )
/// ref: Table 36-7
#define PASSIVE_SCAN				0
#define ACTIVE_SCAN					1
#define BEACON_TABLE				2

typedef enum {
	CCXPKT_INVALID,
	CCXPKT_BEACON_REQUEST
} CCXPKT;
///CCX_RADIOMEASURE


/// =======================================================
///		Data Structure Definition
///
#pragma pack(1)

///CCX_FASTROAM
typedef struct _CCX_AP_INFO_            /* list of current APs. */
{
	NDIS_802_11_MAC_ADDRESS alMACAddr;  /* MAC address */
    OS_UINT16			alChannel;          /* channel of AP */
	NDIS_802_11_SSID	ssid;
    DWORD				alDisassocTime;     /* seconds since disassociated */
} CCX_AP_INFO,*PCCX_AP_INFO;
//////CCX_FASTROAM
///CCX_RADIOMEASURE
/// Radio measurement request fields
typedef struct _CCX_MEASURE_REQUEST_    
{
	OS_UINT8		ChannelNumber; 	
	OS_UINT8		Operation;					///Defined as scan Mode for beacon req,
												///spare for all others
	OS_UINT16		MeasureDuration; 
} CCX_MEASURE_REQUEST,*PCCX_MEASURE_REQUEST;


typedef struct _pkt_param
{
	CCXPKT		pkttype;
	union {
	CCX_MEASURE_REQUEST		beaconReq;
	};
} PKTPARAM, *PPKTPARAM;


///This is the header our firmware is using to generate our own packet which is not from TCP/IP stack
typedef struct _MRVPKT_DESC 
{
    UCHAR dstAddr[MRVDRV_ETH_ADDR_LEN];
    UCHAR srcAddr[MRVDRV_ETH_ADDR_LEN];
    UCHAR protocolLen[2];			// protocol Length set to 0 can set SNAP
} MRVPKTDESC, *PMRVPKTDESC;
///CCX_RADIOMEASURE

/* Information Element IDs */
typedef enum tagINFO_ELEMENT_CCX_IDS 
{
    ELE_CISCO_AIRONET                   =   133, /* 0x85 */
    ELE_CISCO_ENCAPTRANS                =   136, /* 0x88 (S20 in spec */
    ELE_CISCO_APIP_ADDR                 =   149, /* 0x95 (S21 in spec */
    ELE_CISCO_CLIENT_TX_PWR             =   150, /* 0x96 */
    ELE_CISCO_FAST_HANDOFF              =   156, /* 0x9c */
    ELE_CISCO_SYMBOL                    =   173, /* 0xAD (S22 in spec)*/
    ELE_CISCO_MEASUREMENT_REQUEST       =   38,  /* 0x26 */
    ELE_CISCO_MEASUREMENT_REPORT        =   39,  /* 0x27 */
}  INFO_ELEMENT_CCX_IDS;

typedef struct _IE_TRAFFIC_STREAM_METRIC
{
    UCHAR  element_id;   // 0xdd (221)
    UCHAR  length;       // 8 
    UCHAR  oui[3];       // 00:40:96 (Cisco)
    UCHAR  oui_type;     // 0x07 
    UCHAR  tsid;         // 
    UCHAR  state;        // enable=1 ; disable=0
    USHORT measurement_interval;   	// 1ut=1024 microsec ; 4883=5sec 
} IE_TRAFFIC_STREAM_METRIC, *PIE_TRAFFIC_STREAM_METRIC;

typedef struct _OID_MRVL_DS_CCX_NOTIFY_UI 
{
    ULONG  NotifyType;		///Notification Type
    DWORD  ParamLen;		///Length of the parameters, not including type & Length
    MRVPKTDESC  MrvPktDesc;
    IE_TRAFFIC_STREAM_METRIC  ccx4_s56_tsm;
} OID_MRVL_DS_CCX_NOTIFY_UI, *POID_MRVL_DS_CCX_NOTIFY_UI;


typedef struct _MEASURE_REPORT
{
	USHORT packet_q_delay_avg;  		   //
	USHORT packet_q_delay_histogram[4];   //
	ULONG packet_transmit_media_delay;    //
	USHORT packet_loss; 				          //
	USHORT packet_count;				   //
	UCHAR roaming_count;				   //
	USHORT roaming_delay;   			          //
} MEASURE_REPORT, * PMEASURE_REPORT;

typedef struct _PMEASURE_REPORT_ELEMENT
{
	USHORT element_id;    				   // 0x27
	USHORT length;  	  				          //  
	USHORT measurement_token;   		   // 0
	UCHAR measure_mode; 	 			   // 0
	UCHAR measure_type; 		 		   // 6
	MEASURE_REPORT measure_report[1];	   // 
} MEASURE_REPORT_ELEMENT, * PMEASURE_REPORT_ELEMENT;

typedef struct _IE_TRAFFIC_STREAM_METRIC_REPORT_IAPP
{
	BYTE DA[6];
	BYTE SA[6];
	USHORT length;
	UCHAR cisco_aironet_snap[8];	                 // aa:aa:03:00:40:96:00:00
	USHORT iapp_id_length;  	  	
	UCHAR iapp_type;				          // 0x32
	UCHAR iapp_subtype; 			          // 0x81
	UCHAR destination_mac_addr[6];             // 00:00:00:00:00:00 
	UCHAR source_mac_addr[6];   	          // STA's MAC addr
	USHORT dialog_token;			          // 0
	MEASURE_REPORT_ELEMENT measurement_report_element[1];
} IE_TRAFFIC_STREAM_METRIC_REPORT_IAPP,
	* PIE_TRAFFIC_STREAM_METRIC_REPORT_IAPP;


typedef ULONG FSW_CCX_CONFIGURATION;
#define FSW_CCX_CONFIGURATION_ENABLE_CKIP		0x00000001
#define FSW_CCX_CONFIGURATION_ENABLE_ROGUE_AP	0x00000002
#define FSW_CCX_CONFIGURATION_ENABLE_CCKM		0x00000004

typedef enum _FSW_CCX_NETWORK_EAP
{
	FswCcx_NetworkEapOff = 0,
	FswCcx_NetworkEapOn,
	FswCcx_NetworkEapAllowed,
	FswCcx_NetworkEapPreferred
} FSW_CCX_NETWORK_EAP;

typedef struct _FSW_CCX_ROGUE_AP_DETECTED
{
	OS_UINT16 FailureReason;
	NDIS_802_11_MAC_ADDRESS RogueAPMacAddress;
	OS_INT8 RogueAPName[16];
} FSW_CCX_ROGUE_AP_DETECTED,*PFSW_CCX_ROGUE_AP_DETECTED;

typedef struct _FSW_CCX_AUTH_SUCCESS
{
	NDIS_802_11_SSID Ssid;
	NDIS_802_11_MAC_ADDRESS MacAddress;
} FSW_CCX_AUTH_SUCCESS;

typedef struct _FSW_CCX_CCKM_START
{
	OS_UINT8 Timestamp[8];
	NDIS_802_11_MAC_ADDRESS BSSID;
} FSW_CCX_CCKM_START,*PFSW_CCX_CCKM_START;

typedef enum _FSW_CCX_CCKM_REQUEST_CODE
{
	FswCcx_CckmFirstTime = 0,
	FswCcx_CckmFastHandoff,
} FSW_CCX_CCKM_REQUEST_CODE;

typedef struct _FSW_CCX_CCKM_REQUEST
{
	FSW_CCX_CCKM_REQUEST_CODE RequestCode;
	OS_UINT32 AssociationRequestIELength;
	OS_UINT8 AssociationRequestIE[1];
} FSW_CCX_CCKM_REQUEST,*PFSW_CCX_CCKM_REQUEST;

typedef enum _FSW_CCX_CCKM_RESULT_CODE
{
	FswCcx_CckmSuccess = 0,
	FswCcx_CckmFailure,
	FswCcx_CckmNotInUse
} FSW_CCX_CCKM_RESULT_CODE;

typedef struct _FSW_CCX_CCKM_RESULT
{
	FSW_CCX_CCKM_RESULT_CODE ResultCode;
} FSW_CCX_CCKM_RESULT,*PFSW_CCX_CCKM_RESULT;

/**
 * CCX information for a specific BSS.  Part of the BSS descriptor, setup
 * during scan interpretation and used if needed in association requests
 */

typedef struct
{
	UCHAR ccxEnabled;
	UCHAR cckmEnabled;
	UCHAR maxPowerValid;
	UCHAR maxPower;
	ULONGLONG   CurrentTSF;
	BOOLEAN     isMediaConnected;
} CCX_BSS_Info_t;

typedef struct _WPA_SUPPLICANT
{
	UCHAR   Wpa_ie[256];
	UCHAR	Wpa_ie_len;
} WPA_SUPPLICANT, *PWPA_SUPPLICANT;
#pragma pack()

/// =======================================================
///		Function Prototype
///
extern void wlan_ccx_init(IN PMRVDRV_ADAPTER pAdapter);
extern void wlan_ccx_free(IN PMRVDRV_ADAPTER pAdapter);

/* Add any additional CCX elments to an association request to the firmware */
extern int wlan_ccx_process_association_req(u8** pAssocBuf, 
					  CCX_BSS_Info_t* pCcxBssInfo,	
					  WPA_SUPPLICANT * pWpaInfo,
					  UCHAR *timestamp,
                                            int enableCcxPwrLimit,
                                            ULONG ulAttemptSSIDIdx);

/* Process any incoming bss elements in a scan response that are CCX related */
extern int wlan_ccx_process_bss_elem(CCX_BSS_Info_t* pBSSID, u8* pBssElem);
extern ULONG wlan_ccx_getFlags(void);
extern void wlan_ccx_setFlags(ULONG flags);
extern ULONG wlan_ccx_getEAPState(void);
extern void wlan_ccx_setEAPState(ULONG state);
extern USHORT wlan_ccx_MapAuthMode(ULONG state);
extern void wlan_ccx_setRogueAP(FSW_CCX_ROGUE_AP_DETECTED *pRogueAP);
extern void wlan_ccx_sendRogueAPList(IN PMRVDRV_ADAPTER Adapter);

extern void wlan_ccx_setCurMAC(
	IN PNDIS_802_11_SSID curSsid,
	IN NDIS_802_11_MAC_ADDRESS curBSSID, 
	IN NDIS_802_11_MAC_ADDRESS curStaMac 
	);

extern void wlan_ccx_send_packet(IN PMRVDRV_ADAPTER Adapter);
extern int wlan_ccx_isCurPacket(PNDIS_PACKET pPacket);
extern void wlan_ccx_clrCurPacket(void);
extern void wlan_ccx_authSuccess(IN PMRVDRV_ADAPTER pAdapter);
extern void wlan_ccx_assocSuccess(void);
extern void wlan_ccx_disconnect(void);

extern int 	wlan_ccx_cckmStart(PUCHAR pTimestamp);
extern void	wlan_ccx_CCKMRequest(PFSW_CCX_CCKM_REQUEST pdata);
extern void	wlan_ccx_CCKMResult(PFSW_CCX_CCKM_RESULT pdata);
//////CCX_FASTROAM
void wlan_ccx_send_ddp(IN PMRVDRV_ADAPTER Adapter);
///CCX_FASTROAM
///CCX_RADIOMEASURE
void wlan_ccx_send_BeaconTable(IN PMRVDRV_ADAPTER Adapter);
void wlan_ccx_parse_iapp(IN PMRVDRV_ADAPTER Adapter, u8* pktBufPt, PPKTPARAM pPktParam);
VOID wlan_ccx_RxPktFilter(PMRVDRV_ADAPTER pAdapter, UCHAR* bufpt);
///CCX_RADIOMEASURE
///CCX_TPC
extern void wlan_ccx_setTxPower(void);
extern void wlan_ccx_recTxPower(int userPower);
///CCX_TPC
VOID 		Send_CCX4_S56_Notification( PMRVDRV_ADAPTER Adapter, UCHAR *ie, 
                            USHORT ie_len, UCHAR *dest_addr, UCHAR *src_addr );
BOOLEAN		Find_Ie_Traffic_Stream_Metric(UCHAR *buf, USHORT buf_len, USHORT *offset);
VOID HandleCcxv4S56Event (PMRVDRV_ADAPTER Adapter, UCHAR *ie, USHORT ie_len);
VOID FunCcxV4S56TestTimer( IN PVOID SystemSpecific1,
                                                IN NDIS_HANDLE MiniportAdapterContext,
                                                IN PVOID SystemSpecific2,
                                                IN PVOID SystemSpecific3
                                                 );


#endif

