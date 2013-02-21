/*******************  Marvell Semiconductor, Inc., ***************************
 *
 *  Purpose:    This module provides the implementation of NDIS set and get 
 *              information handler
 *
 *  $Author: achin $
 *
 *  $Date: 2004/12/29 $
 *
 *  $Revision: #12 $
 *
 *****************************************************************************/

#include "precomp.h"

//  Currently, Microsoft still use 802.3 type of frame for 802.11 netowrking
//
static const NDIS_MEDIUM MediaSupported[2] = {NdisMedium802_3, (NDIS_MEDIUM)NULL};
///new-assoc ++
static NDIS_STATUS ProcessOIDMrvlAssoc(PMRVDRV_ADAPTER Adapter, 
                                        IN PVOID InformationBuffer, IN ULONG InformationBufferLength,
                                        OUT PULONG BytesRead, OUT PULONG BytesNeeded,
                                        BOOLEAN needPending);

///new-assoc --

static NDIS_STATUS HandleSSIDScan(PMRVDRV_ADAPTER Adapter, 
                                        IN PVOID InformationBuffer, IN ULONG InformationBufferLength,
                                        OUT PULONG BytesRead, OUT PULONG BytesNeeded);

//  WMI support, please check MOF file for GUID definition
//
static const NDIS_GUID GUIDList[] = {
    { 
        {0x21190696,0x118D,0x4654,0x9E,0x9A,0xC6,0x9C,0xA7,0xC7,0x95,0xB8}, // GUID
        OID_FSW_CCX_CONFIGURATION,                  // custom OID, see enum above
        sizeof(OS_UINT32),                          // size of data 
        fNDIS_GUID_TO_OID|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE  // flags go here
    },                        
    {
        {0x0725E492,0x3025,0x477C,0x91,0xDC,0xD5,0xC1,0x2A,0x4E,0xEC,0x1F},
        OID_FSW_CCX_NETWORK_EAP,
        sizeof(OS_UINT32),
        fNDIS_GUID_TO_OID|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE 
        },
    {
        {0x5858FA82,0x0DFD,0x4A4A,0xBB,0xC9,0xDC,0xC7,0x8F,0x63,0x01,0x70},
        OID_FSW_CCX_ROGUE_AP_DETECTED,
        sizeof(FSW_CCX_ROGUE_AP_DETECTED),
        fNDIS_GUID_TO_OID|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE 
    },
    {
        {0x6E72993A,0x59A7,0x4A3E,0xB1,0x65,0x0C,0xEC,0xB3,0xC5,0x0C,0xDC},
        OID_FSW_CCX_REPORT_ROGUE_APS,
        0,
        fNDIS_GUID_TO_OID|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE 
    },
    {
        {0x55019653,0x0454,0x4309,0xB8,0xCA,0xD2,0xE9,0xF4,0xD0,0xAF,0x83},
//        {0x871DBA61,0xF66A,0x426d,0xA0,0x7D,0xAE,0xF9,0x3F,0x36,0x80,0x27},
        OID_FSW_CCX_AUTH_SUCCESS,
        sizeof(FSW_CCX_AUTH_SUCCESS),
        fNDIS_GUID_TO_OID|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE 
    },
    {
        {0x8C389E47,0xE511,0x4D96,0xAE,0xFE,0x2F,0xB7,0x31,0xD8,0x0C,0x05},
//        {0xA42D234C,0xABEF,0x4f3c,0x8A,0x34,0x52,0xC4,0x9C,0x46,0x62,0x27},
        OID_FSW_CCX_CCKM_START,             
        -1,
        fNDIS_GUID_TO_STATUS|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE // this one sends status only
    },
    {
        {0x1163FCA7,0x9C1A,0x4E39,0xA8,0x79,0x9F,0x93,0xAD,0x1B,0x84,0x07},
//        {0xCD1D414E,0x9CFD,0x4d46,0x8A,0x78,0x88,0x60,0xB3,0x15,0xF0,0x80},
        OID_FSW_CCX_CCKM_RESULT,
        sizeof(FSW_CCX_CCKM_RESULT),
        fNDIS_GUID_TO_OID|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE 
    },
    {
        {0xF5190942,0x6D90,0x4858,0x8A,0xDF,0x08,0x6A,0x2F,0xA5,0xB7,0xEB},
        OID_FSW_CCX_CCKM_REQUEST,
        //sizeof(FSW_CCX_CCKM_REQUEST),
      32,
        
        fNDIS_GUID_TO_OID|fNDIS_GUID_ALLOW_READ|fNDIS_GUID_ALLOW_WRITE 
    },

    { // {3d8f1f43-7c2a-4393-bcf0-ee4e07dc4068} // Set Query
              {0x3d8f1f43, 0x7c2a, 0x4393,  0xbc, 0xf0, 0xee, 0x4e, 0x07, 0xdc, 0x40, 0x68},
              OID_MRVL_OEM_SET_ULONG,
              sizeof(ULONG),
              (fNDIS_GUID_TO_OID)},
    { // {3d8f1f43-7c2a-4393-bcf0-ee4e07dc4068} // Get Query
              {0x3d8f1f43, 0x7c2a, 0x4393,  0xbc, 0xf0, 0xee, 0x4e, 0x07, 0xdc, 0x40, 0x68},
              OID_MRVL_OEM_GET_ULONG,
              sizeof(ULONG),
              (fNDIS_GUID_TO_OID)},
    { // {3d8f1f43-7c2a-4393-bcf0-ee4e07dc4068} // Get array query
            {0x3d8f1f43, 0x7c2a, 0x4393,  0xbc, 0xf0, 0xee, 0x4e, 0x07, 0xdc, 0x40, 0x68},
            OID_MRVL_OEM_GET_STRING,
            (ULONG) -1, // size is size of each element in the string
            (fNDIS_GUID_TO_OID|fNDIS_GUID_ANSI_STRING|fNDIS_GUID_ALLOW_READ)}
#ifdef NDIS50_MINIPORT
    { // {3d8f1f43-7c2a-4393-bcf0-ee4e07dc4068} // Get array query
            {0x3d8f1f43, 0x7c2a, 0x4393,  0xbc, 0xf0, 0xee, 0x4e, 0x07, 0xdc, 0x40, 0x68},
            OID_MRVL_OEM_GET_STRING,
            (ULONG) -1, // size is size of each element in the string
            (fNDIS_GUID_TO_OID|fNDIS_GUID_ANSI_STRING)}
#endif            
};

// MrvDrv supported ODI list
//
static const UINT MrvDrvGlobalSupportedOids[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_CURRENT_PACKET_FILTER,  // ToDo: Function to set filter
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,       // ToDo: Function to set protocol option
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_SUPPORTED_GUIDS,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,
    OID_GEN_DIRECTED_FRAMES_RCV,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    //For ndis 4 packet priority
    //OID_802_3_MAC_OPTIONS,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    //OID_802_3_XMIT_DEFERRED,
    //OID_802_3_XMIT_MAX_COLLISIONS,
    //OID_802_3_RCV_OVERRUN,
    //OID_802_3_XMIT_UNDERRUN,
    //OID_802_3_XMIT_HEARTBEAT_FAILURE,
    //OID_802_3_XMIT_TIMES_CRS_LOST,
    //OID_802_3_XMIT_LATE_COLLISIONS,
    //TCP/IP checksum offload 
    //OID_TCP_TASK_OFFLOAD,
    //Power management 
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
    OID_PNP_ENABLE_WAKE_UP,
    //OID_PNP_WAKE_UP_PATTERN_LIST,
    OID_GEN_MACHINE_NAME,
    OID_GEN_PHYSICAL_MEDIUM,
    OID_802_11_BSSID,
    OID_802_11_SSID,
    OID_802_11_NETWORK_TYPES_SUPPORTED,  //(Optional)
    OID_802_11_NETWORK_TYPE_IN_USE,
    OID_802_11_TX_POWER_LEVEL,           //(Optional)
    OID_802_11_RSSI,
    OID_802_11_RSSI_TRIGGER,             //(Optional)
    OID_802_11_INFRASTRUCTURE_MODE, 
    OID_802_11_FRAGMENTATION_THRESHOLD,  //(Optional)
    OID_802_11_RTS_THRESHOLD,            //(Optional)
    //OID_802_11_NUMBER_OF_ANTENNAS,     //(Optional)
    OID_802_11_RX_ANTENNA_SELECTED,      //(Optional)
    OID_802_11_TX_ANTENNA_SELECTED,      //(Optional)
    OID_802_11_SUPPORTED_RATES,
    OID_802_11_DESIRED_RATES,            //(Optional) 
    OID_802_11_CONFIGURATION,
    OID_802_11_STATISTICS,               //(Optional)
    OID_802_11_ADD_WEP,
    OID_802_11_REMOVE_WEP,
    OID_802_11_DISASSOCIATE,
    OID_802_11_POWER_MODE,               
    OID_802_11_BSSID_LIST,
    OID_802_11_AUTHENTICATION_MODE,
    OID_802_11_PRIVACY_FILTER,           //(optional)
    OID_802_11_BSSID_LIST_SCAN,
    OID_802_11_RELOAD_DEFAULTS,
    OID_802_11_ADD_KEY,
    OID_802_11_REMOVE_KEY,
    OID_802_11_ENCRYPTION_STATUS,
    OID_802_11_ASSOCIATION_INFORMATION,
    OID_802_11_TEST,                     // not supported

    /* custom oid WMI support */
    OID_MRVL_OEM_GET_ULONG,
    OID_MRVL_OEM_SET_ULONG,
    OID_MRVL_OEM_GET_STRING,
    /* OEM command */
    OID_MRVL_OEM_COMMAND,
    OID_MRVL_BBP_REG,                    // access Baseband register
    OID_MRVL_MAC_REG,                    // access MAC register
    OID_MRVL_RF_REG,                     // access RF register
    OID_MRVL_EEPROM_ACCESS,              // access flash memory
    OID_MRVL_TX_MODE,                    // set card to special TX mode
    OID_MRVL_RX_MODE,                    // set card to special RX mode
    OID_MRVL_MAC_ADDRESS,
    OID_MRVL_REGION_CODE,
    OID_MRVL_L2ROAMING,                  // set L2 roaming parameters
    OID_MRVL_LED_CONTROL,                // set LED behavior
    //OID_MRVL_MULTIPLE_DTIM,              // set multiple of DTIM period
    OID_MRVL_WMM_STATE,
    OID_MRVL_WMM_ACK_POLICY,
    OID_MRVL_WMM_STATUS,
    OID_MRVL_SLEEP_PERIOD,
    OID_MRVL_LED_CONTROL,
    OID_MRVL_CAL_DATA,
    OID_MRVL_CAL_DATA_EXT,
    OID_MRVL_BG_SCAN_QUERY,
    OID_MRVL_BG_SCAN_CONFIG,
    OID_MRVL_POWER_ADAPT_CFG_EXT,

    OID_MRVL_GET_TX_RATE,

    OID_MRVL_GET_RX_INFO,
    OID_MRVL_PWR_CFG,
    OID_MRVL_TPC_CFG,
    OID_MRVL_RATE_ADAPT_RATESET,
   OID_MRVL_HOSTCMD,
   OID_MRVL_TEMP_FIX_CLOSEWZC,
    OID_MRVL_GET_TSF,
    OID_MRVL_WMM_ADDTS,
    OID_MRVL_WMM_DELTS,
    OID_MRVL_WMM_QUEUE_CONFIG,
    OID_MRVL_WMM_QUEUE_STATS,
///new-assoc ++
    OID_MRVL_ASSOC  
///new-assoc --
};


wchar_t*	OIDMSG(NDIS_OID Oid, NDIS_STATUS MsgStatus)
{
	static wchar_t	oidmsg[100];
	switch (Oid) {
		case OID_GEN_NETWORK_LAYER_ADDRESSES: //0x00010118
		wcscpy(oidmsg, L"OID_GEN_NETWORK_LAYER_ADDRESSES");
		break;
		case OID_GEN_TRANSPORT_HEADER_OFFSET: //0x00010119
		wcscpy(oidmsg, L"OID_GEN_TRANSPORT_HEADER_OFFSET");
		break;
		case OID_GEN_INIT_TIME_MS:    //0x00020213
		wcscpy(oidmsg, L"OID_GEN_INIT_TIME_MS");
		break;
		case OID_GEN_DIRECTED_BYTES_XMIT:  //0x00020201
		wcscpy(oidmsg, L"OID_GEN_DIRECTED_BYTES_XMIT");
		break;
		case OID_GEN_MULTICAST_BYTES_XMIT: //0x00020203
		wcscpy(oidmsg, L"OID_GEN_MULTICAST_BYTES_XMIT");
		break;
		case OID_GEN_BROADCAST_BYTES_XMIT: //0x00020205
		wcscpy(oidmsg, L"OID_GEN_BROADCAST_BYTES_XMIT");
		break;
		case OID_GEN_DIRECTED_BYTES_RCV: //0x00020207
		wcscpy(oidmsg, L"OID_GEN_DIRECTED_BYTES_RCV");
		break;
		case OID_GEN_MULTICAST_BYTES_RCV: //0x00020209
		wcscpy(oidmsg, L"OID_GEN_MULTICAST_BYTES_RCV");
		break;
		case OID_GEN_BROADCAST_BYTES_RCV: //0x0002020b
		wcscpy(oidmsg, L"OID_GEN_BROADCAST_BYTES_RCV");
		break;
		case OID_GEN_MEDIA_SENSE_COUNTS: //0x00020215
		wcscpy(oidmsg, L"OID_GEN_MEDIA_SENSE_COUNTS");
		break;
		case OID_802_11_SSID:
		wcscpy(oidmsg, L"OID_802_11_SSID");
		break;
		case OID_802_11_INFRASTRUCTURE_MODE:
		wcscpy(oidmsg, L"OID_802_11_INFRASTRUCTURE_MODE");
		break;
		case OID_802_11_AUTHENTICATION_MODE:
		wcscpy(oidmsg, L"OID_802_11_AUTHENTICATION_MODE");
		break;
		case OID_802_11_ENCRYPTION_STATUS:
		wcscpy(oidmsg, L"OID_802_11_ENCRYPTION_STATUS");
		break;
		case OID_802_11_BSSID_LIST_SCAN:
		wcscpy(oidmsg, L"OID_802_11_BSSID_LIST_SCAN");
		break;
		case OID_802_11_ADD_KEY:
		wcscpy(oidmsg, L"OID_802_11_ADD_KEY");
		break;
		case OID_802_11_PRIVACY_FILTER:
		wcscpy(oidmsg, L"OID_802_11_PRIVACY_FILTER");
		break;
		case OID_802_11_DISASSOCIATE:
		wcscpy(oidmsg, L"OID_802_11_DISASSOCIATE");
		break;
		case OID_802_11_REMOVE_WEP:
		wcscpy(oidmsg, L"OID_802_11_REMOVE_WEP");
		break;
		case OID_802_11_ADD_WEP:
		wcscpy(oidmsg, L"OID_802_11_ADD_WEP");
		break;
		case OID_802_11_RTS_THRESHOLD:
		wcscpy(oidmsg, L"OID_802_11_RTS_THRESHOLD");
		break;
		case OID_MRVL_802_11D_ENABLE:
		wcscpy(oidmsg, L"OID_MRVL_802_11D_ENABLE");
		break;
		case OID_MRVL_SLEEP_PERIOD:
		wcscpy(oidmsg, L"OID_MRVL_SLEEP_PERIOD");
		break;
		case OID_MRVL_SUBSCRIBE_EVENT:
		wcscpy(oidmsg, L"OID_MRVL_SUBSCRIBE_EVENT");
		break;
		case OID_MRVL_INACTIVITY_TIMEOUT:
		wcscpy(oidmsg, L"OID_MRVL_INACTIVITY_TIMEOUT");
		break;
		case OID_MRVL_FW_WAKE_METHOD:
		wcscpy(oidmsg, L"OID_MRVL_FW_WAKE_METHOD");
		break;
		case OID_MRVL_SLEEP_PARAMS:
		wcscpy(oidmsg, L"OID_MRVL_SLEEP_PARAMS");
		break;
		case OID_MRVL_ENABLE_RADIO:
		wcscpy(oidmsg, L"OID_MRVL_ENABLE_RADIO");
		break;case OID_MRVL_DISABLE_RADIO:
		wcscpy(oidmsg, L"OID_MRVL_DISABLE_RADIO");
		break;
		case OID_MRVL_RADIO_CONTROL:
		wcscpy(oidmsg, L"OID_MRVL_RADIO_CONTROL");
		break;
		case OID_MRVL_LED_CONTROL:
		wcscpy(oidmsg, L"OID_MRVL_LED_CONTROL");
		break;
		case OID_MRVL_RX_MODE:
		wcscpy(oidmsg, L"OID_MRVL_RX_MODE");
		break;
		case OID_MRVL_TX_MODE:
		wcscpy(oidmsg, L"OID_MRVL_TX_MODE");
		break;
		case OID_MRVL_EEPROM_ACCESS:
		wcscpy(oidmsg, L"OID_MRVL_EEPROM_ACCESS");
		break;
		case OID_MRVL_ASSOC:
		wcscpy(oidmsg, L"OID_MRVL_ASSOC");
		break;	 
		case OID_802_11_FRAGMENTATION_THRESHOLD:
		wcscpy(oidmsg, L"OID_MRVL_POWER_ADAPT_CFG_EXT");
		break;
		case OID_MRVL_POWER_ADAPT_CFG_EXT:
		wcscpy(oidmsg, L"OID_MRVL_POWER_ADAPT_CFG_EXT");
		break;
		case OID_MRVL_BG_SCAN_CONFIG:
		wcscpy(oidmsg, L"OID_MRVL_BG_SCAN_CONFIG");
		break;
		case OID_MRVL_MFG_COMMAND:
		wcscpy(oidmsg, L"OID_MRVL_MFG_COMMAND");
		break;
		case OID_MRVL_SET_ADHOCAES:
		wcscpy(oidmsg, L"OID_MRVL_SET_ADHOCAES");
		break;
		case OID_MRVL_REMOVE_ADHOCAES:
		wcscpy(oidmsg, L"OID_MRVL_REMOVE_ADHOCAES");
		break;
		default:
		swprintf(oidmsg, L"0x%x", Oid);
	}

	 if(MsgStatus==NDIS_STATUS_NOT_SUPPORTED)
	 {
	       wcscat(oidmsg, L", Return NDIS_STATUS_NOT_SUPPORTED!!");
	  }
	  else if(MsgStatus==NDIS_STATUS_FAILURE)
	  {
		wcscat(oidmsg, L" FAIL, Return NDIS_STATUS_FAILURE!!");
	  }  
	  else if(MsgStatus==NDIS_STATUS_RESOURCES)
	  {
	       wcscat(oidmsg, L", Return NDIS_STATUS_RESOURCES!!");
	  }

	return oidmsg;
}





//060407 This Table fill the OIDs that allow access in
//DeepSleep or PS mode.
NDIS_OID PsQueryableOID[] =
{   
 OID_MRVL_ADHOC_G_PROTECTION,
 OID_802_11_PMKID,
 OID_802_11_CAPABILITY,
 OID_MRVL_BG_SCAN_QUERY,
 OID_MRVL_GET_RX_INFO,               
 OID_GEN_MAC_OPTIONS,
 OID_GEN_CURRENT_PACKET_FILTER,
 OID_GEN_SUPPORTED_LIST,            
 OID_GEN_HARDWARE_STATUS,           
 OID_GEN_MEDIA_SUPPORTED,           
 OID_GEN_MEDIA_IN_USE,              
 OID_GEN_MAXIMUM_FRAME_SIZE,        
 OID_GEN_MAXIMUM_LOOKAHEAD,         
 OID_GEN_CURRENT_LOOKAHEAD,         
 OID_GEN_MAXIMUM_TOTAL_SIZE,        
 OID_GEN_RECEIVE_BLOCK_SIZE,        
 OID_GEN_TRANSMIT_BLOCK_SIZE,       
 OID_GEN_LINK_SPEED,                
 OID_GEN_TRANSMIT_BUFFER_SPACE,     
 OID_GEN_RECEIVE_BUFFER_SPACE,      
 OID_GEN_VENDOR_ID,                 
 OID_GEN_VENDOR_DESCRIPTION,        
 OID_GEN_DRIVER_VERSION,            
 OID_GEN_VENDOR_DRIVER_VERSION,     
 OID_GEN_SUPPORTED_GUIDS,           
 OID_802_3_MULTICAST_LIST,          
 OID_802_3_PERMANENT_ADDRESS,       
 OID_802_3_CURRENT_ADDRESS,         
 OID_802_3_MAXIMUM_LIST_SIZE,       
 OID_GEN_MAXIMUM_SEND_PACKETS,      
 OID_GEN_MEDIA_CONNECT_STATUS,      
 OID_PNP_CAPABILITIES,              
 OID_PNP_QUERY_POWER,               
 OID_PNP_WAKE_UP_PATTERN_LIST,      
 OID_GEN_PHYSICAL_MEDIUM,           
 OID_802_11_NETWORK_TYPES_SUPPORTED,
 OID_802_11_NETWORK_TYPE_IN_USE,    
 OID_802_11_BSSID,                  
 OID_802_11_SSID,                   
 OID_802_11_POWER_MODE,             
 OID_802_11_RSSI_TRIGGER,           
 OID_MRVL_RSSI_BCNAVG,              
 OID_MRVL_RSSI_THRESHOLDTIMER,      
 OID_802_11_INFRASTRUCTURE_MODE,    
 OID_802_11_CONFIGURATION,          
 OID_802_11_SUPPORTED_RATES,        
 OID_802_11_STATISTICS,             
 OID_802_11_BSSID_LIST,             
 OID_802_11_AUTHENTICATION_MODE,    
 OID_802_11_PRIVACY_FILTER,         
 OID_802_11_WEP_STATUS,             
 OID_802_11_ENCRYPTION_STATUS,      
 OID_802_11_ASSOCIATION_INFORMATION,
 OID_FSW_CCX_CONFIGURATION,         
 OID_FSW_CCX_NETWORK_EAP,           
 OID_FSW_CCX_ROGUE_AP_DETECTED,     
 OID_FSW_CCX_REPORT_ROGUE_APS,      
 OID_FSW_CCX_AUTH_SUCCESS,          
 OID_FSW_CCX_CCKM_REQUEST,          
 OID_FSW_CCX_CCKM_RESULT,           
 OID_MRVL_OEM_GET_ULONG,            
 OID_MRVL_OEM_SET_ULONG,            
 OID_MRVL_OEM_GET_STRING,           
 OID_MRVL_EEPROM_ACCESS,            
 OID_MRVL_DEEP_SLEEP,               
 OID_MRVL_WMM_STATE,                
 OID_MRVL_SUBSCRIBE_EVENT,          
 OID_GEN_XMIT_OK,                   
 OID_GEN_RCV_OK,                    
 OID_GEN_DIRECTED_FRAMES_RCV,       
 OID_GEN_XMIT_ERROR,                
 OID_GEN_RCV_ERROR,                 
 OID_GEN_RCV_NO_BUFFER,             
 OID_GEN_RCV_CRC_ERROR,             
 OID_GEN_TRANSMIT_QUEUE_LENGTH,     
 OID_802_3_RCV_ERROR_ALIGNMENT,     
 OID_802_3_XMIT_ONE_COLLISION,      
 OID_802_3_XMIT_MORE_COLLISIONS,    
 OID_MRVL_ATIMWINDOW,               
 OID_MRVL_802_11D_ENABLE,           
 OID_MRVL_LOCAL_LISTEN_INTERVAL,    
 OID_MRVL_IBSS_COALESCING_CFG,      
 OID_MRVL_PROPRIETARY_PERIODIC_PS,  
 OID_MRVL_WMM_ADDTS,                
 OID_MRVL_WMM_DELTS,                
 OID_MRVL_TEMP_FIX_CLOSEWZC,        
 OID_MRVL_TX_CONTROL,               
 OID_MRVL_DBG_LOG_EVENT,            
 OID_MRVL_ADHOC_AWAKED_PERIOD,      
 OID_MRVL_BCA_TIMESHARE
};


BOOLEAN MrvlDrvQueryCheckPsQueryableOID(NDIS_OID Oid)
{
	USHORT i;
	
	for( i = 0; i < sizeof(PsQueryableOID)/sizeof(NDIS_OID); i++)
	{
		if(Oid == PsQueryableOID[i])
		  return TRUE;
	}

	
    return FALSE;
}						



/******************************************************************************
 *
 *  Name: MrvDrvCopyBSSIDList()
 *
 *  Description: Copy BSSID List
 *
 *  Arguments:
 *    IN      PMRVDRV_ADAPTER         Adapter
 *      IN OUT  PNDIS_802_11_BSSID_LIST         pScanList
 *      IN      NDIS_WLAN_BSSID_EX              DriverScanList
 *      IN      MRV_BSSID_IE_LIST               DriverIEList
 *    
 *  Return Value:
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_NOT_SUPPORTED
 * 
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS MrvDrvCopyBSSIDList(
    IN      PMRVDRV_ADAPTER         Adapter,
  IN OUT  NDIS_WLAN_BSSID_EX        *pScanList,
    IN      ULONG                   BufferLength,
    OUT     ULONG                   *BytesNeeded,
    OUT     ULONG                   *BytesWritten,
    IN      ULONG                   ulNumBSSIDs,
    IN      NDIS_WLAN_BSSID_EX      *pBSSIDList,
    IN      MRV_BSSID_IE_LIST       *pDriverIEList)      
{

    // check if using the old structure
    UINT    idx, size;
    PUCHAR  pCurBuf, pPrevBuf;
    ULONG   ulRequiredSize;
    ULONG   ulActualWriteSize = 0;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    // Find out the total size required
    ulRequiredSize = sizeof(NDIS_802_11_BSSID_LIST_EX);

    for ( idx = 0; idx < ulNumBSSIDs; idx++ )
    {
        // 4 byte align
        size = (((sizeof(NDIS_WLAN_BSSID_EX) + 
                pBSSIDList[idx].IELength) + 3) << 2) >> 2;
        
        ulRequiredSize += size;
    }                
    
    // Check the buffer size
    if (BufferLength < ulRequiredSize)
    {         
       *BytesWritten = 0;
       *BytesNeeded = ulRequiredSize;
    
       return NDIS_STATUS_INVALID_LENGTH;
    }

    pPrevBuf = pCurBuf = (PUCHAR)pScanList;

    // DBGPRINT(DBG_OID, ("Number of AP: %d\n", pNdisBssidList->NumberOfItems));

    for ( idx =0; idx < ulNumBSSIDs; idx++ )
    {
        // copy NDIS_WLAN_BSSID_EX structure
        NdisMoveMemory( pCurBuf, 
               &(pBSSIDList[idx]),
               sizeof(NDIS_WLAN_BSSID_EX));
        
        if ( pBSSIDList[idx].IELength > 0 )
        {
            pCurBuf = ((PNDIS_WLAN_BSSID_EX)pCurBuf)->IEs;
            
            // copy IE
            NdisMoveMemory( pCurBuf, 
                            &(pDriverIEList[idx]),
                            pBSSIDList[idx].IELength);
        }

        size = (((sizeof(NDIS_WLAN_BSSID_EX) + 
                pBSSIDList[idx].IELength) + 3) >> 2) << 2;
        

        DBGPRINT(DBG_SCAN|DBG_HELP, (L"Copied SSID %s, %2x-%2x-%2x-%2x-%2x-%2x\n", 
                                       pBSSIDList[idx].Ssid.Ssid,
                                       pBSSIDList[idx].MacAddress[0],
                                       pBSSIDList[idx].MacAddress[1],
                                       pBSSIDList[idx].MacAddress[2],
                                       pBSSIDList[idx].MacAddress[3],
                                       pBSSIDList[idx].MacAddress[4],
                                       pBSSIDList[idx].MacAddress[5]));

        // move the pointers
        pCurBuf = pPrevBuf + size;
        ulActualWriteSize += size;
        pPrevBuf = pCurBuf;
    }

    *BytesWritten = ulActualWriteSize;
    *BytesNeeded = 0;
    return (Status);
}

/******************************************************************************
 *
 *  Name: MrvDrvSetEncryptionStatus()
 *
 *  Description: process set encryption status OID
 *
 *  Arguments:
 *    IN PMRVDRV_ADAPTER          Adapter
 *      IN NDIS_802_11_ENCRYPTION_STATUS  EncryptionStatus
 *    
 *  Return Value:
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_NOT_SUPPORTED
 * 
 *  Notes: 
 *    Due to the new OID operation mechanism, this function can only be called from
 *    MrvDrvSetInformation(). TT noted.
 *
 *****************************************************************************/
NDIS_STATUS 
MrvDrvSetEncryptionStatus(
  IN PMRVDRV_ADAPTER          Adapter,
  IN NDIS_802_11_ENCRYPTION_STATUS  EncryptionStatus,
    IN NDIS_OID Oid,
  IN PVOID InformationBuffer,
  OUT PULONG BytesRead,
  OUT PULONG BytesNeeded
)
{
    NDIS_STATUS     Status;
    NDIS_802_11_ENCRYPTION_STATUS   PrevEncryptionStatus;

    PrevEncryptionStatus = Adapter->EncryptionStatus;
    DBGPRINT(DBG_V9,(L"EncryptionStatus %x %x \n", Adapter->EncryptionStatus, EncryptionStatus));
    
    
    switch (EncryptionStatus) 
    {
       case Ndis802_11Encryption3Enabled:
       case Ndis802_11Encryption3KeyAbsent:  
       case Ndis802_11Encryption2Enabled:
       case Ndis802_11Encryption2KeyAbsent:

           if ( (PrevEncryptionStatus == Ndis802_11Encryption1Enabled) || 
                (PrevEncryptionStatus == Ndis802_11Encryption1KeyAbsent) )
            {
                  Adapter->CurrentMacControl &= (~HostCmd_ACT_MAC_WEP_ENABLE);
                  Status = SetMacControl(Adapter);           
            }
             else
            {
                 Status = NDIS_STATUS_SUCCESS;
            } 
            if ((Adapter->bIsReconnectEnable == TRUE) || 
                (Adapter->bIsBeaconLoseEvent == TRUE) || 
                (Adapter->bIsDeauthenticationEvent == TRUE)
               )
            {
                Adapter->EncryptionStatus = EncryptionStatus;
            }
            else
            {
                if (EncryptionStatus == Ndis802_11Encryption3Enabled ||
                    EncryptionStatus == Ndis802_11Encryption3KeyAbsent)
                    Adapter->EncryptionStatus = Ndis802_11Encryption3KeyAbsent;
                else       
                    Adapter->EncryptionStatus = Ndis802_11Encryption2KeyAbsent;
                            
                if (Adapter->AdhocAESEnabled)
                    Adapter->EncryptionStatus = Ndis802_11Encryption3Enabled;

            }

            Adapter->WEPStatus = Ndis802_11WEPDisabled;

            break;

       case Ndis802_11Encryption1Enabled:
       case Ndis802_11Encryption1KeyAbsent:


            // disable RSN if previous status is 2 or 3

            Adapter->EncryptionStatus = Ndis802_11Encryption1Enabled;
            Adapter->WEPStatus = Ndis802_11WEPEnabled;
        
            DBGPRINT(DBG_V9,(L"KeyLength is %d \n", Adapter->LastAddedWEPKey.KeyLength));
            // Enable WEP
            Adapter->CurrentMacControl |= (HostCmd_ACT_MAC_WEP_ENABLE);
           if(Adapter->LastAddedWEPKey.KeyLength==13)
           {
             //WEP 128bit
             Adapter->CurrentMacControl |= HostCmd_ACT_MAC_WEP_TYPE;
             DBGPRINT(DBG_V9,(L"MrvDrvSetEncryptionStatus: SetCMD 0x28 WEP 128bit\n"));
           }
           else
           {
            
             //WEP 64bit
              Adapter->CurrentMacControl &= (~HostCmd_ACT_MAC_WEP_TYPE);
              DBGPRINT(DBG_V9,(L"MrvDrvSetEncryptionStatus: SetCMD 0x28 WEP 64bit\n"));
           }

             Status = SetMacControl(Adapter);           

           break;

       case Ndis802_11EncryptionDisabled:

           // tt ++ 060907: When using Funk and no security setting, driver needs this

//tt funk            if ( (PrevEncryptionStatus == Ndis802_11Encryption1Enabled) || 
//tt funk                (PrevEncryptionStatus == Ndis802_11Encryption1KeyAbsent) )

            {
                  Adapter->CurrentMacControl &= (~HostCmd_ACT_MAC_WEP_ENABLE);
                  Status = SetMacControl(Adapter);           
            }
//tt funk             else
//tt funk                  Status = NDIS_STATUS_SUCCESS;


           Adapter->EncryptionStatus = Ndis802_11EncryptionDisabled;
           Adapter->WEPStatus = Ndis802_11WEPDisabled;

           Adapter->NeedSetWepKey = FALSE;

           break; 

       default:
           DBGPRINT(DBG_OID | DBG_WPA|DBG_ERROR, (L"Not supported EncryptionStatus: %d\r\n", 
                                                  EncryptionStatus));
           Status = NDIS_STATUS_INVALID_DATA;
    }

    return Status;
}



/******************************************************************************
 *
 *  Name: MrvDrvQueryInformation()
 *
 *  Description: NDIS miniport get information handler
 *
 *  Conditions for Use: NDIS wrapper will call this handler to get device information
 *
 *  Arguments:
 *      IN NDIS_HANDLE MiniportAdapterContext
 *      IN NDIS_OID Oid
 *      IN PVOID InformationBuffer
 *      IN ULONG InformationBufferLength
 *      OUT PULONG BytesWritten
 *      OUT PULONG BytesNeeded
 *    
 *  Return Value:
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_PENDING
 *      NDIS_STATUS_BUFFER_TOO_SHORT
 *      NDIS_STATUS_NOT_SUPPORTED
 * 
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS
MrvDrvQueryInformation(
  IN NDIS_HANDLE MiniportAdapterContext,
  IN NDIS_OID Oid,
  IN PVOID InformationBuffer,
  IN ULONG InformationBufferLength,
  OUT PULONG BytesWritten,
  OUT PULONG BytesNeeded
  )
{
    //      String describing our adapter
    char VendorDescriptor[] = VENDORDESCRIPTOR;
    PMRVDRV_ADAPTER Adapter;

    //UCHAR VendorId[4];
    ULONGLONG GenericULONGLONG;
    ULONG GenericULONG;
    LONG GenericLONG;
    USHORT GenericUSHORT;
    UCHAR GenericArray[20];
    NDIS_STATUS Status;

    NDIS_PNP_CAPABILITIES PMCap;
    NDIS_DEVICE_POWER_STATE NewPowerState;
    // NDIS_PM_PACKET_PATTERN PMPattern;
    // PUCHAR PUchar;

    PVOID MoveSource;
    ULONG MoveBytes;
    ULONG ulRequiredSize;
    POID_MRVL_DS_WMM_STATE pWmmState;
    POID_MRVL_DS_GET_RX_INFO pGetRxInfo;
    NDIS_802_11_CONFIGURATION     NdisCFG;  //dralee 10/13/2006
    DWORD dwWaitStatus;


    //      Common variables for pointing to result of query
    MoveSource = (PVOID)&GenericULONG;
    MoveBytes = sizeof(GenericULONG);
    Adapter = (PMRVDRV_ADAPTER)(MiniportAdapterContext);
    DBGPRINT(DBG_OID, (L"REQUEST - Enter MrvDrvQueryInformation - 0x%x \n", Oid));

        if( Adapter->ChipResetting == 1 || Adapter->CurPowerState == NdisDeviceStateD3)
    	{
    	       DBGPRINT(DBG_OID|DBG_ERROR, (L"Query: OID= 0x%x ,because Adapter->ChipResetting=0x%x  Adapter->CurPowerState =0x%x , retrun NDIS_STATUS_FAILURE!!  \n", Oid,Adapter->ChipResetting,Adapter->CurPowerState));
               return NDIS_STATUS_FAILURE;                            
    	}
        //051407 : 060407
        Status = AutoDeepSleepPending(Adapter);
        if( Status != NDIS_STATUS_SUCCESS)
            return Status;

        if ( !IsThisDsState(Adapter, DS_STATE_NONE) )
        {
            if( Adapter->AutoDeepSleepTime == 0 )
            {
                if (Oid != OID_MRVL_DEEP_SLEEP)
                {     
                //051207 palm patch   
                    if(MrvlDrvQueryCheckPsQueryableOID(Oid) == FALSE)
                    {
                        DBGPRINT(DBG_OID|DBG_CUSTOM, (L"Query: OID= 0x%x ,because this oid is not allow access in DeepSleep or PS mode ,  retrun NDIS_STATUS_FAILURE!!  \n", Oid));
                        return NDIS_STATUS_FAILURE;
                    }
                }
            }   
            else //power up device in DeepSleep mode
            {   
                OID_MRVL_DS_DEEP_SLEEP ds;
                ULONG                  cnt=0;

				DBGPRINT(DBG_OID|DBG_DEEPSLEEP| DBG_WARNING,(L"OID access, PWR up device in deepsleep\r\n"));
                ds.ulEnterDeepSleep = 0;  //exit deep sleep

                Status = ProcessOIDDeepSleep(Adapter,
                                      HostCmd_ACT_GEN_SET, 
                                      sizeof(OID_MRVL_DS_DEEP_SLEEP), 
                                      (PVOID)&ds,
                                       HostCmd_PENDING_ON_NONE); //041307 
                if( Status != NDIS_STATUS_SUCCESS )
                {
         	        DBGPRINT(DBG_OID|DBG_ERROR, (L"Query: OID= 0x%x ,ProcessOIDDeepSleep:PWR up device in deepsleep FAIL ,  retrun NDIS_STATUS_FAILURE!!  \r\n", Oid));
                    return NDIS_STATUS_FAILURE; 
                }
                //waiting for DS state change
                Status = If_WaitFWPowerUp(Adapter);
                if( Status != IF_SUCCESS )
                {          
                    DBGPRINT( DBG_ERROR,(L"Set: OID=0%x ,Fail to wait DS_AWAKE event, return NDIS_STATUS_FAILURE\r\n",Oid));
                    return NDIS_STATUS_FAILURE;  
                }
            }
        }   

#ifndef NDIS50_MINIPORT
        //      Check device removal status
        if( Adapter->SurpriseRemoved == TRUE )
        {
            return NDIS_STATUS_NOT_ACCEPTED;
        }
#else
        {
            //012207
            if ( IsThisDsState(Adapter, DS_STATE_NONE) )  
            {
                if ( sdio_IsFirmwareLoaded(Adapter) == FW_STATUS_READ_FAILED)
                {
                    DBGPRINT(DBG_OID|DBG_ERROR,(L"card removed !  ****\n"));
                    return NDIS_STATUS_NOT_ACCEPTED;       
                }    
            }      
        }
#endif
    //Initialize the result variables
    *BytesWritten = 0;
    *BytesNeeded = 0;
    Status = NDIS_STATUS_SUCCESS;

    if (Adapter->UseMfgFw == 1)
    {
        if (Oid != OID_MRVL_MFG_COMMAND)
        {
            DBGPRINT(DBG_OID ,(L"Query OID received is not OID_MRVL_MFG_COMMAND in MFG driver\r\n"));
            return NDIS_STATUS_SUCCESS;
        }
    }

    if ( Adapter->CurPowerState == NdisDeviceStateD3 )
    {
        // card powered off
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    // Switch on request type
    switch (Oid)
    {
    	
        case OID_MRVL_S56:
        {
            *((ULONG *)InformationBuffer) = Adapter->bEnableS56DriverIAPP; 	
            *BytesWritten = sizeof(ULONG);
            return Status;
        }
    	
    	
        case OID_MRVL_MFG_COMMAND:
        { 
            if (Adapter->UseMfgFw != 1)
            {
                DBGPRINT(DBG_OID|DBG_HELP,
                         (L"[Marvell]Query OID_MRVL_MFG_COMMAND for Normal Fw\r\n"));
                return NDIS_STATUS_NOT_SUPPORTED;
            }

            DBGPRINT(DBG_OID|DBG_HELP, (L"[Marvell]Begin: Query OID_MRVL_MFG_COMMAND\r\n"));

            Status=PrepareAndSendCommand(
                            Adapter,
                            HostCmd_CMD_MFG_COMMAND,
                            HostCmd_ACT_GET,
                            HostCmd_OPTION_USE_INT,
                            Oid,
                            HostCmd_PENDING_ON_GET_OID,
                            0,
                            FALSE,
                            BytesWritten,
                            0,
                            BytesNeeded,
                            InformationBuffer);
            
            dwWaitStatus = WaitForSingleObjectWithCancel(Adapter, Adapter->hOidQueryEvent, ASYNC_OID_QUERY_TIMEOUT);
          
            if (dwWaitStatus != WAIT_OBJECT_0 || Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
            {
                DBGPRINT(DBG_OID|DBG_ERROR, (L"Failed: Query OID=%s \r\n", OIDMSG(Oid,Status)));  
            }
            else
            {
                DBGPRINT(DBG_OID|DBG_HELP, (L"Query OID_MRVL_MFG_COMMAND Success\r\n"));
                NdisMoveMemory(InformationBuffer,
                           Adapter->OidCmdRespBuf,
                           Adapter->nSizeOfOidCmdResp);
                *(BytesWritten) = Adapter->nSizeOfOidCmdResp;
            }

            return Status;
        }  
        case OID_MRVL_ADHOC_DEFAULT_CHANNEL:
        {
            POID_DS_MRVL_ADHOC_CHANNEL      pAdhocChan = (POID_DS_MRVL_ADHOC_CHANNEL) InformationBuffer;

            if ( InformationBufferLength < sizeof(OID_DS_MRVL_ADHOC_CHANNEL) )
                return NDIS_STATUS_INVALID_LENGTH;
            
            pAdhocChan->ulChannelNumber = Adapter->AdhocDefaultChannel;

            return NDIS_STATUS_SUCCESS;
        }
        break;
            
        case OID_MRVL_ADHOC_G_PROTECTION:
        {
            ULONG ulGProtect = *((PULONG)InformationBuffer);

            if ( InformationBufferLength < sizeof(ULONG) )
                return NDIS_STATUS_INVALID_LENGTH;

            if ( Adapter->CurrentMacControl & HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON )
                GenericULONG = (ULONG) 1;
            else
                GenericULONG = (ULONG) 0;                
        }
        break;

    case OID_802_11_PMKID:
        {
            PNDIS_802_11_PMKID  pPmkid = (PNDIS_802_11_PMKID) InformationBuffer;

            DBGPRINT( DBG_OID|DBG_MACEVT, (L"QUERY - OID_802_11_PMKID\n") );

            if ( InformationBufferLength < sizeof(NDIS_802_11_PMKID) )
            {
                DBGPRINT(DBG_OID|DBG_MACEVT, (L"    Buffer too small, return.\n") );
                *BytesNeeded = sizeof(NDIS_802_11_PMKID);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pPmkid->Length = sizeof(NDIS_802_11_PMKID);
            pPmkid->BSSIDInfoCount = 0;

            return NDIS_STATUS_SUCCESS;
        }
        break;
        
    case OID_802_11_CAPABILITY:
        {
            PNDIS_802_11_CAPABILITY     pCap = (PNDIS_802_11_CAPABILITY) InformationBuffer;
            ULONG   nNeedLen;
            const ULONG nNumOfModes = 12;

            DBGPRINT( DBG_OID|DBG_MACEVT, (L"QUERY - OID_802_11_CAPABILITY\n") );

            nNeedLen = sizeof(NDIS_802_11_CAPABILITY) + 
                        ((nNumOfModes-1) * sizeof(NDIS_802_11_AUTHENTICATION_ENCRYPTION) );

            if (InformationBufferLength < nNeedLen)
            {
                DBGPRINT(DBG_OID|DBG_MACEVT, (L"    Buffer too small, return.\n") );
                *BytesNeeded = nNeedLen;
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pCap->Length = nNeedLen;
            pCap->Version = 2;
            pCap->NoOfPMKIDs = MAX_PMKID_CACHE;
            pCap->NoOfAuthEncryptPairsSupported = nNumOfModes;

            DBGPRINT(DBG_OID|DBG_MACEVT, (L"    Copy data... [Len=%d, Pmkid=%d, Modes=%d]\n",
                        pCap->Length, pCap->NoOfPMKIDs, pCap->NoOfAuthEncryptPairsSupported ) );

            pCap->AuthenticationEncryptionSupported[0].AuthModeSupported = Ndis802_11AuthModeOpen;
            pCap->AuthenticationEncryptionSupported[0].EncryptStatusSupported = Ndis802_11EncryptionDisabled;

            pCap->AuthenticationEncryptionSupported[1].AuthModeSupported = Ndis802_11AuthModeOpen;
            pCap->AuthenticationEncryptionSupported[1].EncryptStatusSupported = Ndis802_11Encryption1Enabled;
            
            pCap->AuthenticationEncryptionSupported[2].AuthModeSupported = Ndis802_11AuthModeShared;
            pCap->AuthenticationEncryptionSupported[2].EncryptStatusSupported = Ndis802_11EncryptionDisabled;
            
            pCap->AuthenticationEncryptionSupported[3].AuthModeSupported = Ndis802_11AuthModeShared;
            pCap->AuthenticationEncryptionSupported[3].EncryptStatusSupported = Ndis802_11Encryption1Enabled;
            
            pCap->AuthenticationEncryptionSupported[4].AuthModeSupported = Ndis802_11AuthModeWPA;
            pCap->AuthenticationEncryptionSupported[4].EncryptStatusSupported = Ndis802_11Encryption2Enabled;

            pCap->AuthenticationEncryptionSupported[5].AuthModeSupported = Ndis802_11AuthModeWPA;
            pCap->AuthenticationEncryptionSupported[5].EncryptStatusSupported = Ndis802_11Encryption3Enabled;
            
            pCap->AuthenticationEncryptionSupported[6].AuthModeSupported = Ndis802_11AuthModeWPAPSK;
            pCap->AuthenticationEncryptionSupported[6].EncryptStatusSupported = Ndis802_11Encryption2Enabled;
            
            pCap->AuthenticationEncryptionSupported[7].AuthModeSupported = Ndis802_11AuthModeWPAPSK;
            pCap->AuthenticationEncryptionSupported[7].EncryptStatusSupported = Ndis802_11Encryption3Enabled;
            
            pCap->AuthenticationEncryptionSupported[8].AuthModeSupported = Ndis802_11AuthModeWPA2;
            pCap->AuthenticationEncryptionSupported[8].EncryptStatusSupported = Ndis802_11Encryption2Enabled;

            pCap->AuthenticationEncryptionSupported[9].AuthModeSupported = Ndis802_11AuthModeWPA2;
            pCap->AuthenticationEncryptionSupported[9].EncryptStatusSupported = Ndis802_11Encryption3Enabled;
            
            pCap->AuthenticationEncryptionSupported[10].AuthModeSupported = Ndis802_11AuthModeWPA2PSK;
            pCap->AuthenticationEncryptionSupported[10].EncryptStatusSupported = Ndis802_11Encryption2Enabled;

            pCap->AuthenticationEncryptionSupported[11].AuthModeSupported = Ndis802_11AuthModeWPA2PSK;
            pCap->AuthenticationEncryptionSupported[11].EncryptStatusSupported = Ndis802_11Encryption3Enabled;
            *BytesWritten = nNeedLen;

            return NDIS_STATUS_SUCCESS;
        }
        break;
        case OID_MRVL_BG_SCAN_QUERY:
        {
            POID_MRVL_DS_BG_SCAN_CONFIG pOidBG;
            pOidBG = (POID_MRVL_DS_BG_SCAN_CONFIG)Adapter->BgScanCfg;

            if (Adapter->bBgScanEnabled)
                pOidBG->Enable = 1;
            else
                pOidBG->Enable = 0;

            MoveSource = (PVOID) (Adapter->BgScanCfg);
                MoveBytes = Adapter->nBgScanCfg;
            }       
            break;
   
     case OID_MRVL_GET_RX_INFO:
     {
        
       if( InformationBufferLength < sizeof(OID_MRVL_DS_GET_RX_INFO) )
            {
                   *BytesNeeded = sizeof(OID_MRVL_DS_GET_RX_INFO);         
                   return NDIS_STATUS_INVALID_LENGTH;
             }
              pGetRxInfo = (POID_MRVL_DS_GET_RX_INFO)InformationBuffer;
              pGetRxInfo->RXPDSNR = Adapter->SNR[TYPE_RXPD][TYPE_NOAVG] ;
          pGetRxInfo->RXPDRate = Adapter->RxPDRate ;    

          MoveBytes=sizeof(OID_MRVL_DS_GET_RX_INFO);      
              Status = NDIS_STATUS_SUCCESS; 
              *BytesWritten=sizeof(OID_MRVL_DS_GET_RX_INFO);  
          return NDIS_STATUS_SUCCESS;       
        }       
       break;

      case OID_MRVL_POWER_ADAPT_CFG_EXT:
      {
         DBGPRINT(DBG_OID|DBG_HELP,(L"sizeof OID_MRVL_DS_POWER_ADAPT_CFG_EXT=%x, InformationBufferLength=%x\r\n",sizeof(OID_MRVL_DS_POWER_ADAPT_CFG_EXT),InformationBufferLength));

         GetPACFGValue(Adapter);   
         TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT, OID_MRVL_POWER_ADAPT_CFG_EXT, 
         sizeof(OID_MRVL_DS_POWER_ADAPT_CFG_EXT), HostCmd_ACT_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
         TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_POWER_ADAPT_CFG_EXT, Action, SIZEOF_OID_DS_LEADING );
      }    
      break;
      case OID_MRVL_LED_CONTROL:
           TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_LED_CONTROL, OID_MRVL_LED_CONTROL, 
           sizeof(OID_MRVL_DS_LED_CONTROL), HostCmd_ACT_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
           TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_LED_CONTROL, NumLed, SIZEOF_OID_DS_LEADING );
      break;
    
      case OID_MRVL_CAL_DATA:
           TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_CAL_DATA, OID_MRVL_CAL_DATA, 
           sizeof(OID_MRVL_DS_CAL_DATA), HostCmd_ACT_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
           TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_CAL_DATA, Reserved1[0], SIZEOF_OID_DS_LEADING );
           break;
      case OID_MRVL_CAL_DATA_EXT:
           TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_CAL_DATA_EXT, OID_MRVL_CAL_DATA_EXT, 
           sizeof(OID_MRVL_DS_CAL_DATA_EXT), HostCmd_ACT_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
           TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_CAL_DATA_EXT, Revision, SIZEOF_OID_DS_LEADING );
           break;

      case OID_MRVL_PWR_CFG:
           TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_PWR_CFG, OID_MRVL_PWR_CFG, 
           sizeof(OID_MRVL_DS_PWR_CFG), HostCmd_ACT_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
           TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_PWR_CFG, Enable, SIZEOF_OID_DS_LEADING );
           break;
      case OID_MRVL_TPC_CFG:
           TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_TPC_CFG, OID_MRVL_TPC_CFG, 
           sizeof(OID_MRVL_DS_TPC_CFG), HostCmd_ACT_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
           TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_TPC_CFG, Enable, SIZEOF_OID_DS_LEADING );
           break;
      case OID_MRVL_RATE_ADAPT_RATESET:
           //0327
           Status = ProcessOIDRateAdaptRateSet(Adapter,
                                               HostCmd_ACT_GET,
                                               InformationBufferLength,
                                               BytesWritten,
                                               InformationBuffer);
           return Status;

           break;
      case OID_GEN_MAC_OPTIONS:
           //      NOTE: Don't set NDIS_MAC_OPTION_RECEIVE_SERIALIZED if we
           //      are doing multipacket (ndis4) style receives.
        
           GenericULONG = (ULONG)NDIS_MAC_OPTION_NO_LOOPBACK |
           NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;

          if ( Adapter->EnableQOS )
          {
             GenericULONG = GenericULONG | NDIS_MAC_OPTION_8021P_PRIORITY;
          }

          break;

      case OID_GEN_CURRENT_PACKET_FILTER:
           GenericULONG = Adapter->CurrentPacketFilter;
           break;

      case OID_GEN_SUPPORTED_LIST:

           MoveSource = (PVOID) (MrvDrvGlobalSupportedOids);
           MoveBytes = sizeof(MrvDrvGlobalSupportedOids);
           break;
      case OID_GEN_HARDWARE_STATUS:
           GenericULONG = (ULONG)Adapter->HardwareStatus;
           break;
      case OID_GEN_MEDIA_SUPPORTED:

           MoveSource = (PVOID)MediaSupported;
           MoveBytes = sizeof(NDIS_MEDIUM); // need to change for 802.11
           break;
      case OID_GEN_MEDIA_IN_USE:
           GenericULONG = Adapter->MediaInUse;
           break;

      case OID_GEN_MAXIMUM_FRAME_SIZE:
          // this is a work around for the ping problem.
          GenericULONG = (ULONG)CF_MAX_PACKET_SIZE;
          break;
    case OID_GEN_MAXIMUM_LOOKAHEAD:
         GenericULONG = (ULONG)MRVDRV_MAXIMUM_ETH_PACKET_SIZE - MRVDRV_ETH_HEADER_SIZE;
         DBGPRINT(DBG_RX|DBG_OID|DBG_HELP, (L"OID: Return OID_GEN_MAXIMUM_LOOKAHEAD to %d\n", 
         MRVDRV_MAXIMUM_ETH_PACKET_SIZE- MRVDRV_ETH_HEADER_SIZE));
         break;

    case OID_GEN_CURRENT_LOOKAHEAD:
         GenericULONG = (ULONG)MRVDRV_MAXIMUM_ETH_PACKET_SIZE - MRVDRV_ETH_HEADER_SIZE;
         DBGPRINT(DBG_RX|DBG_OID|DBG_HELP, (L"OID: Return OID_GEN_CURRENT_LOOKAHEAD to %d\n", 
         MRVDRV_MAXIMUM_ETH_PACKET_SIZE- MRVDRV_ETH_HEADER_SIZE));
         break;

    
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
        ///crlo:MTU ++
        GenericULONG = (ULONG)MRVDRV_MAXIMUM_ETH_PACKET_SIZE;
        ///crlo:MTU --
        break;

    case OID_GEN_LINK_SPEED:
    // should never return 0mbps according to spec
    if ( Adapter->LinkSpeed == MRVDRV_LINK_SPEED_0mbps )
    {
      GenericULONG = (ULONG) MRVDRV_LINK_SPEED_1mbps;
    }
    else
    {
      GenericULONG = (ULONG) Adapter->LinkSpeed;
    }
    break;


    case OID_GEN_TRANSMIT_BUFFER_SPACE:

        GenericULONG = (ULONG)0;
        break;


    case OID_GEN_RECEIVE_BUFFER_SPACE:
        GenericULONG = (ULONG) MRVDRV_MAXIMUM_ETH_PACKET_SIZE; // 1514 * 8
        break;


    case OID_GEN_VENDOR_ID:

        MoveSource = (PVOID) (&Adapter->VendorID);
        MoveBytes = 4;
        break;


    case OID_GEN_VENDOR_DESCRIPTION:

        MoveSource = (PVOID) VendorDescriptor;
        MoveBytes = sizeof(VendorDescriptor);
        break;


    case OID_GEN_DRIVER_VERSION:

        GenericUSHORT = (USHORT) MRVDRV_DRIVER_VERSION;
        MoveSource = (PVOID)(&GenericUSHORT);
        MoveBytes = sizeof(GenericUSHORT);
        break;

    case OID_GEN_VENDOR_DRIVER_VERSION:
        // CF8381/8385 only uses the last 3 bytes
        GenericULONG = Adapter->FWReleaseNumber << 8;  
        break;

    //      WMI support
    case OID_GEN_SUPPORTED_GUIDS:
        MoveSource = (PUCHAR)&GUIDList;
        MoveBytes =  sizeof(GUIDList);
        break; 

    case OID_802_3_MULTICAST_LIST:
        DBGPRINT(DBG_OID,(L"REQUEST - OID_802_3_MULTICAST_LIST \n"));
        if (InformationBufferLength < (Adapter->NumOfMulticastMACAddr*ETH_LENGTH_OF_ADDRESS))
        {
            *BytesNeeded = Adapter->NumOfMulticastMACAddr*ETH_LENGTH_OF_ADDRESS;
            return NDIS_STATUS_INVALID_LENGTH;
        }

        MoveSource = (PVOID) Adapter->MulticastList;
        MoveBytes = Adapter->NumOfMulticastMACAddr*ETH_LENGTH_OF_ADDRESS;
        break;
    case OID_802_3_PERMANENT_ADDRESS:

        ETH_COPY_NETWORK_ADDRESS((PCHAR) GenericArray, Adapter->PermanentAddr);

        MoveSource = (PVOID) (GenericArray);
        MoveBytes = MRVDRV_ETH_ADDR_LEN;

        DBGPRINT(DBG_OID|DBG_HELP,(L"*** OID return permanent addr: %2x %2x %2x %2x %2x %2x ***\r\n", 
                 Adapter->PermanentAddr[0],
                 Adapter->PermanentAddr[1],
                 Adapter->PermanentAddr[2],
                 Adapter->PermanentAddr[3],
                 Adapter->PermanentAddr[4],
                 Adapter->PermanentAddr[5]));
        break;

    case OID_802_3_CURRENT_ADDRESS:
        DBGPRINT(DBG_OID|DBG_HELP,(L"OID_802_3_CURRENT_ADDRESS return permanent addr: %2x %2x %2x %2x %2x %2x ***\n\r", 
                 Adapter->CurrentAddr[0],
                 Adapter->CurrentAddr[1],
                 Adapter->CurrentAddr[2],
                 Adapter->CurrentAddr[3],
                 Adapter->CurrentAddr[4],
                 Adapter->CurrentAddr[5]));
        ETH_COPY_NETWORK_ADDRESS(GenericArray, Adapter->CurrentAddr);

        MoveSource = (PVOID) (GenericArray);
        MoveBytes = MRVDRV_ETH_ADDR_LEN;
        break;

    case OID_802_3_MAXIMUM_LIST_SIZE:

        GenericULONG = (ULONG)MRVDRV_MAX_MULTICAST_LIST_SIZE;
        break;

    case OID_GEN_MAXIMUM_SEND_PACKETS:
         GenericULONG = 1;//MiniportSend only 
         break;
  case OID_GEN_MEDIA_CONNECT_STATUS:
        if ((Adapter->bIsReconnectEnable == TRUE) || 
            (Adapter->bIsBeaconLoseEvent == TRUE) || 
            (Adapter->bIsDeauthenticationEvent == TRUE)
           )
           {
             GenericLONG = (ULONG) NdisMediaStateConnected;
           }
           else
           {
             GenericLONG = (ULONG) Adapter->MediaConnectStatus;
           }

           DBGPRINT(DBG_OID|DBG_HELP, (L"Request - OID_GEN_MEDIA_CONNECT_STATUS : %d\r\n",
                                         GenericLONG));
  
           MoveSource = (PVOID) (&GenericLONG);
           MoveBytes = sizeof(GenericLONG);
           break;

    case OID_PNP_CAPABILITIES:
         DBGPRINT(DBG_OID|DBG_POWER|DBG_HELP,(L"OID_PNP_CAPABILITIES\n"));


         PMCap.WakeUpCapabilities.MinMagicPacketWakeUp   = NdisDeviceStateUnspecified;
         PMCap.WakeUpCapabilities.MinPatternWakeUp       = NdisDeviceStateUnspecified;
         PMCap.WakeUpCapabilities.MinLinkChangeWakeUp    = NdisDeviceStateUnspecified;

         MoveSource = (PVOID) &PMCap;
         MoveBytes = sizeof(NDIS_PNP_CAPABILITIES);

         break;

    case OID_PNP_QUERY_POWER:
        DBGPRINT(DBG_POWER|DBG_OID|DBG_HELP,(L"OID_PNP_QUERY_POWER\n"));
        //051207 : 060407
        //NewPowerState = (NDIS_DEVICE_POWER_STATE)*(PNDIS_DEVICE_POWER_STATE)InformationBuffer;
        NewPowerState = Adapter->CurPowerState;
        //      Determine if Station can suuport this state
        if( ( NewPowerState == NdisDeviceStateD0 ) || ( NewPowerState == NdisDeviceStateD3 ) )
        {
            DBGPRINT(DBG_POWER,(L"REQUESTED POWER STATE SUPPORTED!\n"));
            return NDIS_STATUS_SUCCESS;
        }
        else //         We don't know hot to handle NdisDeviceStateD2 yet
        {
            DBGPRINT(DBG_POWER,(L"REQUESTED POWER STATE NOT SUPPORTED %x\n",NewPowerState ));
            return NDIS_STATUS_NOT_SUPPORTED;
        }
        break;


    case OID_GEN_PHYSICAL_MEDIUM:

        GenericULONG = (ULONG)NdisPhysicalMediumWirelessLan;
        break;

    case OID_802_11_NETWORK_TYPES_SUPPORTED:
        {
            NDIS_802_11_NETWORK_TYPE_LIST *pNetworkTypeList;

            DBGPRINT(DBG_OID, (L"Requst - OID_802_11_NETWORK_TYPES_SUPPORTED ****\n"));
            
            if( InformationBufferLength < sizeof(NDIS_802_11_NETWORK_TYPE_LIST) )
            {
                *BytesNeeded = sizeof(NDIS_802_11_NETWORK_TYPE_LIST);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            // only 1 item
            pNetworkTypeList = (NDIS_802_11_NETWORK_TYPE_LIST *)InformationBuffer;
            pNetworkTypeList->NumberOfItems = 1;
            pNetworkTypeList->NetworkType[0] = Ndis802_11DS;

            *BytesWritten = sizeof(NDIS_802_11_NETWORK_TYPE_LIST);
            return NDIS_STATUS_SUCCESS;
        }
        break;

  case OID_802_11_NETWORK_TYPE_IN_USE:

       DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_NETWORK_TYPE_IN_USE ****\n"));
       if( InformationBufferLength < sizeof(NDIS_802_11_NETWORK_TYPE) )
       {
           *BytesNeeded = sizeof(NDIS_802_11_NETWORK_TYPE);
           return NDIS_STATUS_INVALID_LENGTH;
       }     
       GenericULONG = (ULONG)Ndis802_11DS;
       break;

  case OID_802_11_BSSID:
    
       DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_BSSID ****\n"));
       
       // if the CurrentBSSID is a NULL string
       if ( NdisEqualMemory(Adapter->CurrentBSSID, 
                  Adapter->NullBSSID, 
                  MRVDRV_ETH_ADDR_LEN) )
       {
         Status = NDIS_STATUS_ADAPTER_NOT_READY;
       }
       else if( InformationBufferLength < MRVDRV_ETH_ADDR_LEN )
       {
         
         *BytesNeeded = MRVDRV_ETH_ADDR_LEN;
         return NDIS_STATUS_INVALID_LENGTH;
       }
       else{
         MoveSource = (PVOID)Adapter->CurrentBSSID;
         MoveBytes = MRVDRV_ETH_ADDR_LEN;
       }
       break;

  case OID_802_11_SSID:
    
       DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_SSID ****\n"));
           
       if( InformationBufferLength < sizeof(NDIS_802_11_SSID))
       {
         *BytesNeeded = sizeof(NDIS_802_11_SSID);
         return NDIS_STATUS_INVALID_LENGTH;
       }   
     
       if ( NdisEqualMemory(&(Adapter->CurrentSSID), 
                  &(Adapter->NullSSID), 
                  sizeof(NDIS_802_11_SSID)))
       {
         MoveBytes = 0;
               DBGPRINT(DBG_OID,(L"Current SSID is null!\n"));
       }
       else
       {   
         DBGPRINT(DBG_OID,(L"Current SSID is %S\n", Adapter->CurrentSSID.Ssid));
         MoveSource = (PVOID)&Adapter->CurrentSSID;
         MoveBytes = sizeof(NDIS_802_11_SSID);
         HexDump(DBG_OID|DBG_HELP, "CurrentSSID: ", (PUCHAR)&Adapter->CurrentSSID,
                          sizeof(NDIS_802_11_SSID));
       }
       break;

  case OID_802_11_POWER_MODE:
    
       DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_POWER_MODE ****\n\\r"));
       if( InformationBufferLength < sizeof(NDIS_802_11_POWER_MODE) )
       {
         *BytesNeeded = sizeof(NDIS_802_11_POWER_MODE);
         return NDIS_STATUS_INVALID_LENGTH;
       }  

       if((Adapter->PSMode==Ndis802_11PowerModeCAM)&&(Adapter->PSMode_B== Ndis802_11PowerModeMAX_PSP))
       {
               MoveSource = (PVOID)&Adapter->PSMode_B;
       }else
       {
               MoveSource = (PVOID)&Adapter->PSMode;
       }   

       MoveBytes = sizeof(NDIS_802_11_POWER_MODE);  
       break;
   
 case OID_802_11_RSSI:
      {

          ULONG ulTimeDiff;
          if( (ulTimeDiff=GetTickCount())>Adapter->ulRSSITickCount)
          {
                     ulTimeDiff=ulTimeDiff-Adapter->ulRSSITickCount;
          }
          else
          {
                     ulTimeDiff=0xFFFFFFFF-ulTimeDiff+Adapter->ulRSSITickCount; 
          }
     
      
          if((Adapter->MediaConnectStatus == NdisMediaStateConnected) &&( ulTimeDiff > Adapter->ulRSSIThresholdTimer) )
          {
                     TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(NDIS_802_11_RSSI), HostCmd_CMD_802_11_RSSI, HostCmd_ACT_GET );
                     return NDIS_STATUS_SUCCESS;  
          }
          else
          {
        
                    if( InformationBufferLength < sizeof(NDIS_802_11_RSSI) )
                    {
                       *BytesNeeded = sizeof(NDIS_802_11_RSSI);
                       return NDIS_STATUS_INVALID_LENGTH;
                    }
                
                    GenericLONG = Adapter->LastRSSI;
                    MoveSource = (PVOID) (&GenericLONG);
                    MoveBytes = sizeof(GenericLONG);
          }   

    
    }
    break;

  case OID_802_11_RSSI_TRIGGER:

    DBGPRINT(DBG_OID|DBG_ERROR,(L"REQUEST - OID_802_11_RSSI_TRIGGER ****\r\n"));
    if( InformationBufferLength < sizeof(NDIS_802_11_RSSI) )
    {
      *BytesNeeded = sizeof(NDIS_802_11_RSSI);
      return NDIS_STATUS_INVALID_LENGTH;
    }   
    GenericLONG = Adapter->RSSITriggerValue;
    MoveSource = (PVOID) (&GenericLONG);
    MoveBytes = sizeof(GenericLONG);
    break; 

  case OID_MRVL_RSSI_BCNAVG  :
   {
       GenericUSHORT =  Adapter->bcn_avg_factor;
       MoveSource = (PVOID)(&GenericUSHORT);
       MoveBytes = sizeof(GenericUSHORT);
   }
   break;

 case OID_MRVL_RSSI_THRESHOLDTIMER:
   {
         GenericULONG = Adapter->ulRSSIThresholdTimer;
         MoveSource = (PVOID) (&GenericULONG);
         MoveBytes = sizeof(GenericULONG);
   }
   break;  

  case OID_802_11_INFRASTRUCTURE_MODE:
    DBGPRINT(DBG_V9,(L"REQUEST - OID_802_11_INFRASTRUCTURE_MODE ****\n"));

    if( InformationBufferLength < sizeof(NDIS_802_11_NETWORK_INFRASTRUCTURE) )
    {
      *BytesNeeded = sizeof(NDIS_802_11_NETWORK_INFRASTRUCTURE);
      return NDIS_STATUS_INVALID_LENGTH;
    }

    if (Adapter->ulCurrentBSSIDIndex < MRVDRV_MAX_BSSID_LIST)
    {
      GenericULONG = (Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex].InfrastructureMode);
    }
    break;

  case OID_802_11_FRAGMENTATION_THRESHOLD:
   {
    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_FRAGMENTATION_THRESHOLD ****\n"));

    TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( 
      sizeof(NDIS_802_11_FRAGMENTATION_THRESHOLD), 
      HostCmd_CMD_802_11_SNMP_MIB, 
      HostCmd_ACT_GET );
   }
  case OID_802_11_RTS_THRESHOLD:
  {
    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_RTS_THRESHOLD ****\n"));

    TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( 
      sizeof(NDIS_802_11_RTS_THRESHOLD), 
      HostCmd_CMD_802_11_SNMP_MIB, 
      HostCmd_ACT_GET );
  } 

  case OID_802_11_CONFIGURATION:

    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_CONFIGURATION ****\n"));
  
    if( InformationBufferLength < sizeof(NDIS_802_11_CONFIGURATION) )
    {
      
      *BytesNeeded = sizeof(NDIS_802_11_CONFIGURATION);
      return NDIS_STATUS_INVALID_LENGTH;
    }   
     
    MoveSource = (PVOID)&(Adapter->CurrentConfiguration);
    MoveBytes = sizeof(NDIS_802_11_CONFIGURATION); 
    
    //dralee 10/13/2006
    NdisMoveMemory((PVOID)&NdisCFG, MoveSource, MoveBytes);
    NdisCFG.DSConfig *=1000;
    MoveSource = (PVOID)&NdisCFG;
    break;
  case OID_802_11_SUPPORTED_RATES:

    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_SUPPORTED_RATES ****\n"));
    
    if( InformationBufferLength < BASIC_SUPPORTED_RATES )
    {
      *BytesNeeded = BASIC_SUPPORTED_RATES;
      return NDIS_STATUS_INVALID_LENGTH;
    }   
   
    MoveSource = (PVOID)SupportedRates_Basic;
    MoveBytes = BASIC_SUPPORTED_RATES;
    break;

  case OID_802_11_DESIRED_RATES:
    Status = ProcessOIDDesiredRates(Adapter,
                                    HostCmd_ACT_GET,  
                                    InformationBufferLength, 
                                    BytesWritten,
                                    InformationBuffer);

    return Status;
  case OID_802_11_STATISTICS:
     Status = ProcessOIDStatistics(
                                        Adapter,
                                        InformationBufferLength,
                                        InformationBuffer,
                                        BytesNeeded,
                                        BytesWritten);

   return Status;

  case OID_802_11_BSSID_LIST:
    {
        ULONG                       ulNumBSSID;
        PNDIS_WLAN_BSSID_EX         pBSSIDListSrc;
        PMRV_BSSID_IE_LIST          pBSSIDIEListSrc;
        UINT                        idx, size;
        PUCHAR                      pCurBuf, pPrevBuf;
        PNDIS_802_11_BSSID_LIST_EX  pNdisBssidList;
        // BOOLEAN                     bFoundCurrentSSID;

    DBGPRINT(DBG_OID, (L"OID_802_11_BSSID_LIST\n"));

	///Remove this checking for bug#21089 ++
/*    if ( Adapter->bIsScanInProgress )
    {
      *BytesWritten = 0;
      return NDIS_STATUS_SUCCESS; 
    }*/
	///Remove this checking for bug#21089 --

      pBSSIDListSrc = Adapter->PSBSSIDList;
        pBSSIDIEListSrc = Adapter->PSIEBuffer;
        ulNumBSSID = Adapter->ulPSNumOfBSSIDs;

    if (ulNumBSSID == 0)
    {
      *BytesWritten = 0;
      return NDIS_STATUS_SUCCESS; 
    }
    
    // Find out the total size required
        ulRequiredSize = sizeof(NDIS_802_11_BSSID_LIST_EX);

        for ( idx = 0; idx < ulNumBSSID; idx++ )
        {
            // 4 byte align
            size = (((sizeof(NDIS_WLAN_BSSID_EX) + 
                    pBSSIDListSrc[idx].IELength) + 3) >> 2) << 2;
            
            ulRequiredSize += size;
        }                
        
        // Check the buffer size
    if (InformationBufferLength < ulRequiredSize)
        {         
      *BytesWritten = 0;
      *BytesNeeded = ulRequiredSize;
            DBGPRINT(DBG_OID, (L"OID_802_11_BSSID_LIST: Need %d bytes, (provided: %d)\n", ulRequiredSize, InformationBufferLength));
            return NDIS_STATUS_INVALID_LENGTH;
    }

    // Copy the cached BSSID list         
    pNdisBssidList = (PNDIS_802_11_BSSID_LIST_EX)InformationBuffer;
    pNdisBssidList->NumberOfItems = ulNumBSSID;
        pPrevBuf = pCurBuf = (PUCHAR)pNdisBssidList->Bssid;

        for ( idx =0; idx < ulNumBSSID; idx++ )
        {
            // copy NDIS_WLAN_BSSID_EX structure
      NdisMoveMemory( pCurBuf, 
              &(pBSSIDListSrc[idx]),
              sizeof(NDIS_WLAN_BSSID_EX));

            if ( pBSSIDListSrc[idx].IELength > 0 )
            {
                pCurBuf = ((PNDIS_WLAN_BSSID_EX)pCurBuf)->IEs;
                
                // copy IE
                NdisMoveMemory( pCurBuf, 
                                &(pBSSIDIEListSrc[idx]),
                                pBSSIDListSrc[idx].IELength);
            }

            size = (((sizeof(NDIS_WLAN_BSSID_EX) + 
                    pBSSIDListSrc[idx].IELength) + 3) >> 2) << 2;


            // move the pointers
            pCurBuf = pPrevBuf + size;
            pPrevBuf = pCurBuf;
        }

    *BytesWritten = ulRequiredSize;
    *BytesNeeded = 0;

    return NDIS_STATUS_SUCCESS;        
    }
    
  case OID_802_11_AUTHENTICATION_MODE:

    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_AUTHENTICATION_MODE - %d ****\n",
                            Adapter->AuthenticationMode));
    if( InformationBufferLength < sizeof(NDIS_802_11_AUTHENTICATION_MODE) )
    {
      
      *BytesNeeded = sizeof(NDIS_802_11_AUTHENTICATION_MODE);
      return NDIS_STATUS_INVALID_LENGTH;
    }   
    
    GenericULONG = (ULONG)Adapter->AuthenticationMode;
    *BytesWritten = sizeof(NDIS_802_11_AUTHENTICATION_MODE);

    break;

  case OID_802_11_PRIVACY_FILTER:

    //DBGPRINT(DBG_OID,("REQUEST - OID_802_11_PRIVACY_FILTER ****\n"));
    if( InformationBufferLength < sizeof(NDIS_802_11_PRIVACY_FILTER) )
    {
      
      *BytesNeeded = sizeof(NDIS_802_11_PRIVACY_FILTER);
      return NDIS_STATUS_INVALID_LENGTH;
    }   
    GenericULONG = (ULONG)Adapter->PrivacyFilter;
    *BytesWritten = sizeof(NDIS_802_11_PRIVACY_FILTER);
    break;


  case OID_802_11_RX_ANTENNA_SELECTED: 
    {
    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_RX_ANTENNA_SELECTED ****\n"));

    TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(NDIS_802_11_ANTENNA), HostCmd_CMD_802_11_RF_ANTENNA, HostCmd_ACT_GET_RX );
    }
    return NDIS_STATUS_SUCCESS;
    
  case OID_802_11_TX_ANTENNA_SELECTED: 
    {
    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_TX_ANTENNA_SELECTED ****\n"));
    
    TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(NDIS_802_11_ANTENNA), HostCmd_CMD_802_11_RF_ANTENNA, HostCmd_ACT_GET_TX );
    }
    return NDIS_STATUS_SUCCESS;
    case OID_802_11_TX_POWER_LEVEL:
  {
    DBGPRINT(DBG_OID|DBG_HELP,(L"REQUEST - OID_802_11_TX_POWER_LEVEL ****\n"));

    ProcessOIDGetTxPowerLevel(Adapter, InformationBufferLength, InformationBuffer, BytesNeeded, BytesWritten);
    ///TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(NDIS_802_11_TX_POWER_LEVEL), HostCmd_CMD_802_11_RF_TX_POWER, HostCmd_ACT_TX_POWER_OPT_GET );
      }
  return NDIS_STATUS_SUCCESS;


  case OID_802_11_ENCRYPTION_STATUS:
    
    DBGPRINT(DBG_OID | DBG_WPA|DBG_HELP,
                (L"REQUEST - OID_802_11_ENCRYPTION_STATUS - %d ****\n",
                Adapter->EncryptionStatus));

        if( InformationBufferLength < sizeof(NDIS_802_11_ENCRYPTION_STATUS) ){
      
      *BytesNeeded = sizeof(NDIS_802_11_ENCRYPTION_STATUS);
      return NDIS_STATUS_INVALID_LENGTH;
    }   
        GenericULONG = (ULONG)Adapter->EncryptionStatus;
        
                //GenericULONG = 4;
    *BytesWritten = sizeof(NDIS_802_11_ENCRYPTION_STATUS);
    break;

  case OID_802_11_ASSOCIATION_INFORMATION:
        {
            PNDIS_802_11_ASSOCIATION_INFORMATION pAssoInfo;

            DBGPRINT(DBG_OID | DBG_WPA|DBG_HELP ,
                    (L"REQUEST - OID_802_11_ASSOCIATION_INFORMATION ****\n"));

            pAssoInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)
                            Adapter->AssocInfoBuffer;

            // reponse IE is the the last part of the buffer
            if ( InformationBufferLength < (pAssoInfo->OffsetResponseIEs + 
                                            pAssoInfo->ResponseIELength) )
            {
                *BytesNeeded = pAssoInfo->OffsetResponseIEs + 
                                            pAssoInfo->ResponseIELength;

          return NDIS_STATUS_INVALID_LENGTH;
            }

            MoveSource = Adapter->AssocInfoBuffer;
            MoveBytes =  pAssoInfo->OffsetResponseIEs + 
                         pAssoInfo->ResponseIELength;
            
            DBGPRINT(DBG_WPA, (L"OID_802_11_ASSOCIATiON_INFO move bytes = %d\n", MoveBytes));
           
        }
    break;



    case OID_FSW_CCX_CONFIGURATION:
         GenericULONG = wlan_ccx_getFlags();
         MoveBytes = sizeof(ULONG);
         DBGPRINT(DBG_CCX|DBG_HELP, (L"query OID_FSW_CCX_CONFIGURATION return 0x%x\n", GenericULONG));
         break;
    case OID_FSW_CCX_NETWORK_EAP:
         GenericULONG = wlan_ccx_getEAPState();
         MoveBytes = sizeof(ULONG);
         DBGPRINT(DBG_CCX|DBG_HELP, (L"query OID_FSW_CCX_NETWORK_EAP return 0x%x\n", GenericULONG));
         break;
    case OID_FSW_CCX_ROGUE_AP_DETECTED:
         DBGPRINT(DBG_CCX|DBG_HELP, (L"query OID_FSW_CCX_ROGUE_AP_DETECTED should be not supported\n"));
         break;
    case OID_FSW_CCX_REPORT_ROGUE_APS:
         DBGPRINT(DBG_CCX|DBG_HELP, (L"query OID_FSW_CCX_REPORT_ROGUE_APS should be not supported\n"));
         break;
    case OID_FSW_CCX_AUTH_SUCCESS:
         DBGPRINT(DBG_CCX|DBG_HELP, (L"query OID_FSW_CCX_AUTH_SUCCESS should be not supported\n"));
         break;
    case OID_FSW_CCX_CCKM_REQUEST:
         DBGPRINT(DBG_CCX|DBG_HELP, (L"query OID_FSW_CCX_CCKM_REQUEST should be not supported\n"));
         break;
    case OID_FSW_CCX_CCKM_RESULT:
         DBGPRINT(DBG_CCX|DBG_HELP, (L"query OID_FSW_CCX_CCKM_RESULT should be not supported\n"));
         break;


  //      WMI support
    case OID_MRVL_OEM_GET_ULONG:
        GenericULONG = 0;
        break;

    case OID_MRVL_OEM_SET_ULONG:
        break;

    case OID_MRVL_OEM_GET_STRING:
        MoveSource = (PVOID)VendorDescriptor;
        MoveBytes = sizeof(VendorDescriptor);
        break;

    
    case OID_MRVL_BBP_REG:
       {
         TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(OID_MRVL_DS_BBP_REGISTER_ACCESS), HostCmd_CMD_BBP_REG_ACCESS, HostCmd_ACT_GET );
       }
       return NDIS_STATUS_SUCCESS;
    case OID_MRVL_MAC_REG:
      {
         TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(OID_MRVL_DS_MAC_REGISTER_ACCESS), HostCmd_CMD_MAC_REG_ACCESS, HostCmd_ACT_GET );
      }
      return NDIS_STATUS_SUCCESS;
    case OID_MRVL_RF_REG:
      {
        DBGPRINT(DBG_OID,(L"REQUEST - OID_MRVL_RF_REG ****\n"));
        TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(OID_MRVL_DS_RF_REGISTER_ACCESS), HostCmd_CMD_RF_REG_ACCESS, HostCmd_ACT_GET );
      }
      return NDIS_STATUS_SUCCESS;
    case OID_MRVL_EEPROM_ACCESS:
      {
        POID_MRVL_DS_EEPROM_ACCESS pData;

        DBGPRINT(DBG_OID,(L"REQUEST - OID_MRVL_EEPROM_ACCESS ****\n"));
    
        pData = (POID_MRVL_DS_EEPROM_ACCESS)InformationBuffer;

        if ( (pData->usDataLength + sizeof(OID_MRVL_DS_EEPROM_ACCESS)) > InformationBufferLength )
        {
             *BytesNeeded = sizeof(OID_MRVL_DS_EEPROM_ACCESS) + pData->usDataLength;
             return NDIS_STATUS_INVALID_LENGTH;
        }

      }
      return NDIS_STATUS_NOT_SUPPORTED;

    case OID_MRVL_RF_CHANNEL:
  {
    DBGPRINT(DBG_OID,(L"REQUEST - OID_MRVL_RF_REG ****\n"));

      TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(OID_MRVL_DS_RF_CHANNEL), HostCmd_CMD_802_11_RF_CHANNEL, HostCmd_ACT_GEN_GET );
  }
  return NDIS_STATUS_SUCCESS;
  
  case OID_MRVL_REGION_CODE:
   {
    DBGPRINT(DBG_OID,(L"REQUEST - OID_MRVL_REGION_CODE ****\n"));
    
    TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(OID_MRVL_DS_REGION_CODE), HostCmd_CMD_REGION_CODE, HostCmd_ACT_GET );
   }
  return NDIS_STATUS_SUCCESS;

  case OID_MRVL_MAC_ADDRESS:
  {
    DBGPRINT(DBG_OID,(L"REQUEST - OID_MRVL_MAC_ADDRESS ****\n"));
    
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_MAC_ADDRESS, OID_MRVL_MAC_ADDRESS, 
      sizeof(OID_MRVL_DS_MAC_ADDRESS), HostCmd_ACT_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
    TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_MAC_ADDRESS, EthAddr, SIZEOF_OID_DS_LEADING );
  }
  return NDIS_STATUS_SUCCESS;

  case OID_MRVL_RADIO_CONTROL:
    {
      DBGPRINT(DBG_OID,(L"REQUEST - OID_MRVL_RADIO_CONTROL ****\n"));
    
      TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_RADIO_CONTROL, OID_MRVL_RADIO_CONTROL, 
        sizeof(OID_DS_MRVL_RF_RADIO), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
      TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_DS_MRVL_RF_RADIO, RADIO, SIZEOF_OID_DS_LEADING );
        }
    return NDIS_STATUS_SUCCESS;


  case OID_MRVL_GET_ADHOCAES:
    {
      DBGPRINT(DBG_OID,(L"REQUEST - OID_MRVL_GET_ADHOCAES ****\n"));
    
     TT_CMDPARSE_SYNC_STD_OID_QUERY_ADHOCAES_RETURN( sizeof(MRVL_ADHOC_AES_KEY), HostCmd_CMD_802_11_KEY_MATERIAL, HostCmd_ACT_GET );
        }
    return NDIS_STATUS_SUCCESS;
    

  case OID_MRVL_DEEP_SLEEP:
       ProcessOIDDeepSleep(Adapter, HostCmd_ACT_GEN_GET,sizeof(GenericULONG),(PVOID)&GenericULONG, HostCmd_PENDING_ON_NONE); //041307
       MoveSource = (PVOID)(&GenericULONG);
       MoveBytes = sizeof(GenericULONG);
    break;            

       case OID_MRVL_WMM_STATE:

            if( InformationBufferLength < sizeof(OID_MRVL_DS_WMM_STATE) )
            {
    *BytesNeeded = sizeof(OID_MRVL_DS_WMM_STATE);
    return NDIS_STATUS_INVALID_LENGTH;
      }
      pWmmState = (POID_MRVL_DS_WMM_STATE)InformationBuffer;
            pWmmState->State = Adapter->WmmDesc.required;
            Status = NDIS_STATUS_SUCCESS; 
            *BytesWritten=sizeof(OID_MRVL_DS_WMM_STATE);
       break;
       case OID_MRVL_WMM_ACK_POLICY:
      TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_WMM_ACK_POLICY, OID_MRVL_WMM_ACK_POLICY, 
      sizeof(OID_MRVL_DS_WMM_ACK_POLICY), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
      TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_WMM_ACK_POLICY, AC, 0 );
       break;
       case OID_MRVL_WMM_STATUS:
      TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_WMM_GET_STATUS, OID_MRVL_WMM_STATUS, 
      sizeof(OID_MRVL_DS_WMM_AC_STATUS), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
      TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_WMM_AC_STATUS, Status, 0 );
       break;

  case OID_MRVL_SLEEP_PARAMS:
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_SLEEP_PARAMS, OID_MRVL_SLEEP_PARAMS, 
      sizeof(OID_MRVL_DS_SLEEP_PARAMS), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
    TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_SLEEP_PARAMS, Error, 0 );
    break;


  case OID_MRVL_FW_WAKE_METHOD:
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_FW_WAKE_METHOD, OID_MRVL_FW_WAKE_METHOD, 
      sizeof(OID_MRVL_DS_FW_WAKE_METHOD), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
    TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_FW_WAKE_METHOD, method, 0 );
    break;


     case OID_MRVL_INACTIVITY_TIMEOUT:
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_INACTIVITY_TIMEOUT, OID_MRVL_INACTIVITY_TIMEOUT, 
      sizeof(OID_MRVL_DS_INACTIVITY_TIMEOUT), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
    TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_INACTIVITY_TIMEOUT, time, 0 );
    break;

    case OID_MRVL_SUBSCRIBE_EVENT:

        DBGPRINT(DBG_OID|DBG_HELP,(L"\nQUERY - OID_MRVL_SUBSCRIBE_EVENT - %x", Adapter->SubscribeEvents));
                                     
        GenericUSHORT = (USHORT)Adapter->SubscribeEvents;

        MoveSource = (PVOID)(&GenericUSHORT);
        MoveBytes = sizeof(GenericUSHORT);
        break;

    default:
    {
        switch (Oid)
        {
          case OID_GEN_XMIT_OK:

      // bytes needed should always be set regardless whether
      // the request is successful or not
      *BytesNeeded = sizeof(ULONGLONG);

      // reply with ULONG size stats 
      // only when the buffer is exactly the size of ULONG
      if( InformationBufferLength == sizeof(ULONG) )
      {
        GenericULONG = (ULONG)Adapter->XmitOK;        
      }
      else
      {
        GenericULONGLONG = Adapter->XmitOK;
            MoveSource = (PVOID)&GenericULONGLONG;
        MoveBytes = sizeof(ULONGLONG);
        *BytesWritten = sizeof(ULONGLONG);                
      }
    

            break;

        case OID_GEN_RCV_OK:
      // bytes needed should always be set regardless whether
      // the request is successful or not
      *BytesNeeded = sizeof(ULONGLONG);

      // reply with ULONG size stats 
      // only when the buffer is exactly the size of ULONG
      if( InformationBufferLength == sizeof(ULONG) )
      {
        GenericULONG = (ULONG)Adapter->RcvOK;       
      }
      else
      {
        GenericULONGLONG = Adapter->RcvOK;
            MoveSource = (PVOID)&GenericULONGLONG;
        MoveBytes = sizeof(ULONGLONG);
        *BytesWritten = sizeof(ULONGLONG);                
      }

            break;

    case OID_GEN_DIRECTED_FRAMES_RCV:

      *BytesNeeded = sizeof(ULONGLONG);
      // reply with ULONG size stats 
      // only when the buffer is exactly the size of ULONG
      if( InformationBufferLength == sizeof(ULONG) )
      {
        GenericULONG = (ULONG)Adapter->DirectedFramesRcvOK;       
      }
      else
      {
        GenericULONGLONG = Adapter->DirectedFramesRcvOK;
            MoveSource = (PVOID)&GenericULONGLONG;
        MoveBytes = sizeof(ULONGLONG);
        *BytesWritten = sizeof(ULONGLONG);                
      }
      break;

        case OID_GEN_XMIT_ERROR:
      // bytes needed should always be set regardless whether
      // the request is successful or not
      *BytesNeeded = sizeof(ULONGLONG);

      // reply with ULONG size stats 
      // only when the buffer is exactly the size of ULONG
      if( InformationBufferLength == sizeof(ULONG) )
      {
        GenericULONG = (ULONG)Adapter->XmitError;       
      }
      else
      {
        GenericULONGLONG = Adapter->XmitError;
            MoveSource = (PVOID)&GenericULONGLONG;
        MoveBytes = sizeof(ULONGLONG);
        *BytesWritten = sizeof(ULONGLONG);                
      }
            break;

        case OID_GEN_RCV_ERROR:
      // bytes needed should always be set regardless whether
      // the request is successful or not
      *BytesNeeded = sizeof(ULONGLONG);

      // reply with ULONG size stats 
      // only when the buffer is exactly the size of ULONG
      if( InformationBufferLength == sizeof(ULONG) )
      {
        GenericULONG = (ULONG)Adapter->RcvError;        
      }
      else
      {
        GenericULONGLONG = Adapter->RcvError;
            MoveSource = (PVOID)&GenericULONGLONG;
        MoveBytes = sizeof(ULONGLONG);
        *BytesWritten = sizeof(ULONGLONG);                
      }
            break;

        case OID_GEN_RCV_NO_BUFFER:

      // bytes needed should always be set regardless whether
      // the request is successful or not
      *BytesNeeded = sizeof(ULONGLONG);

      // reply with ULONG size stats 
      // only when the buffer is exactly the size of ULONG
      if( InformationBufferLength == sizeof(ULONG) )
      {
        GenericULONG = (ULONG)Adapter->RcvNoBuffer;       
      }
      else
      {
        GenericULONGLONG = Adapter->RcvNoBuffer;
            MoveSource = (PVOID)&GenericULONGLONG;
        MoveBytes = sizeof(ULONGLONG);
        *BytesWritten = sizeof(ULONGLONG);                
      }
            break;

        case OID_GEN_RCV_CRC_ERROR:

      // bytes needed should always be set regardless whether
      // the request is successful or not
      *BytesNeeded = sizeof(ULONGLONG);

      // reply with ULONG size stats 
      // only when the buffer is exactly the size of ULONG
      if( InformationBufferLength == sizeof(ULONG) )
      {
        GenericULONG = (ULONG)Adapter->RcvCRCError;       
      }
      else
      {
        GenericULONGLONG = Adapter->RcvCRCError;
            MoveSource = (PVOID)&GenericULONGLONG;
        MoveBytes = sizeof(ULONGLONG);
        *BytesWritten = sizeof(ULONGLONG);                
      }
      break;
      
        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
//            GenericULONG = (ULONG)MRVDRV_NUM_OF_WCB;
            GenericULONG = (ULONG)1;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:

            GenericULONG = 0;
            break;

        case OID_802_3_XMIT_ONE_COLLISION:

            GenericULONG = 0;
            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:

            GenericULONG = 0;
            break;  

/*
   tt: WMM and PPS needs SLEEP_PERIOD
*/
        case OID_MRVL_SLEEP_PERIOD:     
             TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_SLEEP_PERIOD, OID_MRVL_SLEEP_PERIOD, 
                                   sizeof(OID_MRVL_DS_WMM_SLEEP_PERIOD), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
             TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_WMM_SLEEP_PERIOD, period, 0 );
        break;

        case OID_MRVL_ATIMWINDOW:     
        {
             GenericUSHORT = (USHORT)Adapter->CurrentConfiguration.ATIMWindow;
             *BytesWritten=sizeof(USHORT);
             DBGPRINT(DBG_OID|DBG_HELP, (L"atim:%x\r\n",(USHORT)Adapter->CurrentConfiguration.ATIMWindow));   
        }
        break; 
                      
        case OID_MRVL_802_11D_ENABLE:    
        { 
            GenericUSHORT = (USHORT)Adapter->Enable80211D;
            *BytesWritten=sizeof(USHORT);
            DBGPRINT(DBG_OID|DBG_HELP, (L"11D enable:%x\r\n",(USHORT)GenericUSHORT));   
        } 
        break;

        case OID_MRVL_KEY_ENCRYPT:
             TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_KEY_ENCRYPT, OID_MRVL_KEY_ENCRYPT, 
             sizeof(OID_MRVL_DS_KEY_ENCRYPT), HostCmd_ACT_SET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
             TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_KEY_ENCRYPT, EncType, 0 );
             break;

        case OID_MRVL_CRYPTO:
             TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_CRYPTO, OID_MRVL_CRYPTO, 
             sizeof(OID_MRVL_DS_CRYPTO), HostCmd_ACT_SET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
             TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_CRYPTO, EncDec, 0 );
             break;

        
        case OID_MRVL_GET_TX_RATE: 
             TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( sizeof(OID_MRVL_DS_GET_TX_RATE), HostCmd_CMD_802_11_TX_RATE_QUERY, HostCmd_ACT_GEN_GET); 
             return NDIS_STATUS_SUCCESS;
        
        case OID_MRVL_802_11_GET_LOG:
             TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_GET_LOG, OID_MRVL_802_11_GET_LOG, 
                                   sizeof(OID_MRVL_DS_802_11_GET_LOG), HostCmd_ACT_GEN_GET, HostCmd_PENDING_ON_GET_OID, BytesWritten );
             TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_DS_802_11_GET_LOG, mcasttxframe, 0 );
             break;  
            
        case OID_MRVL_LOCAL_LISTEN_INTERVAL:
        {     
             GenericUSHORT = (USHORT)Adapter->LocalListenInterval;
             *BytesWritten=sizeof(USHORT);
             //RETAILMSG(1, (L"LocalListenInterval:%x\r\n",(USHORT)Adapter->LocalListenInterval));   
        }
        break;

        case OID_MRVL_IBSS_COALESCING_CFG:
         {    
             GenericUSHORT = Adapter->IbssCoalsecingEnable; 
             *BytesWritten=sizeof(USHORT);
             DBGPRINT(DBG_OID|DBG_HELP, (L"IbssCoalsecingEnable:%x\r\n",(USHORT)Adapter->IbssCoalsecingEnable));   
         }
         break;
        //dralee_20060629
        case OID_MRVL_PROPRIETARY_PERIODIC_PS:
             { 
                GenericUSHORT = Adapter->PPS_Enabled; 
                *BytesWritten=sizeof(USHORT);
                DBGPRINT(DBG_OID|DBG_HELP, (L"PPS enabled:%x\r\n",(USHORT)Adapter->PPS_Enabled));   
             }
            break;

    case OID_MRVL_GET_TSF:
        DBGPRINT(DBG_OID|DBG_CCX|DBG_HELP,(L"GET - OID_MRVL_GET_TSF\n")); 
        TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( 
              sizeof(HostCmd_DS_802_11_GET_TSF), 
                HostCmd_CMD_802_11_GET_TSF, 
                HostCmd_ACT_GET );
        break;
    case OID_MRVL_WMM_ADDTS:
        {
            PWMM_ADDTS_RESP     pAddTSResp = &Adapter->AddTSResp;
            DBGPRINT(DBG_CCX|DBG_HELP,(L"GET - OID_MRVL_WMM_ADDTS(%d, %d)\n",
                                InformationBufferLength, sizeof(WMM_ADDTS_RESP)));

            if( InformationBufferLength < sizeof(WMM_ADDTS_RESP) )  {
                *BytesNeeded = sizeof(WMM_ADDTS_RESP);
                DBGPRINT(DBG_OID|DBG_HELP, (L"     Invalid length [In:%d, Needed:%d]\n", InformationBufferLength, sizeof(WMM_ADDTS_RESP)) );
                RETAILMSG(1, (L"     Invalid length [In:%d, Needed:%d]\n", InformationBufferLength, sizeof(WMM_ADDTS_RESP)) );
                return NDIS_STATUS_INVALID_LENGTH;
            }

        {
            int i;
            UCHAR   *datpt = (UCHAR*)pAddTSResp;
            for (i=0 ; i<(sizeof(WMM_ADDTS_RESP)+15)/16 ; i++, datpt += 16) {
                DBGPRINT(DBG_HELP, (L"%02x %02x %02x %02x %02x %02x %02x %02x  - %02x %02x %02x %02x %02x %02x %02x %02x \r\n", 
                    datpt[0], datpt[1], datpt[2], datpt[3], datpt[4], datpt[5], datpt[6], datpt[7],
                    datpt[8], datpt[9], datpt[10], datpt[11], datpt[12], datpt[13], datpt[14], datpt[15]) );
            }
        }
            
            MoveBytes = sizeof(WMM_ADDTS_RESP);
            MoveSource = pAddTSResp;
            //NdisMoveMemory((PUCHAR) InformationBuffer,
            //                            pAddTSResp, sizeof(WMM_ADDTS_RESP));
            //*BytesWritten = sizeof(WMM_ADDTS_RESP);
            DBGPRINT(0,(L"GET - OID_MRVL_WMM_ADDTS success\n"));
            Status = NDIS_STATUS_SUCCESS;
        }
        break;
    case OID_MRVL_WMM_DELTS:
        DBGPRINT(DBG_OID|DBG_CCX|DBG_HELP,(L"GET - OID_MRVL_WMM_DELTS\n")); 
        Status = NDIS_STATUS_SUCCESS;
        break;
    case OID_MRVL_WMM_QUEUE_CONFIG:
    {
      PUCHAR ptr;
        DBGPRINT(DBG_OID|DBG_CCX|DBG_HELP,(L"GET - OID_MRVL_WMM_QUEUE_CONFIG\n")); 
        ptr = (PUCHAR)InformationBuffer;
        TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( 
              sizeof(HostCmd_DS_802_11_WMM_QUEUE_CONFIG), 
                HostCmd_CMD_802_11_WMM_QUEUE_CONFIG, 
                HostCmd_ACT_GET );
    }             
        break;
    case OID_MRVL_WMM_QUEUE_STATS:
        DBGPRINT(DBG_OID|DBG_CCX|DBG_HELP,(L"GET - OID_MRVL_WMM_QUEUE_STATS\n")); 
        DBGPRINT(DBG_TMP|DBG_CCX|DBG_HELP,(L"GET - OID_MRVL_WMM_QUEUE_STATS(%d, %d)\n",InformationBufferLength, sizeof(DS_802_11_WMM_QUEUE_STATS))); 
        TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( 
              sizeof(DS_802_11_WMM_QUEUE_STATS), 
                HostCmd_CMD_802_11_WMM_QUEUE_STATS, 
                HostCmd_ACT_GET );

//        TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( 
//              sizeof(HostCmd_DS_802_11_WMM_QUEUE_STATS), 
//                HostCmd_CMD_802_11_WMM_QUEUE_STATS, 
//                HostCmd_ACT_GET );
        break;
     case OID_MRVL_TEMP_FIX_CLOSEWZC: 
        { 
            GenericUSHORT = Adapter->TCloseWZCFlag; 
            *BytesWritten=sizeof(USHORT);
            DBGPRINT(DBG_OID|DBG_HELP, (L"TCloseWZCFlag:%x\r\n",(USHORT)Adapter->TCloseWZCFlag));   
        }
        break;
    case OID_MRVL_TX_CONTROL: 
        { 
            GenericULONG = Adapter->TX_Control_Value; 
            *BytesWritten=sizeof(ULONG);
            DBGPRINT(DBG_OID|DBG_HELP, (L"TX_Control_Value:%x\r\n",(ULONG)Adapter->TX_Control_Value));   
        }
        break;  
    case OID_MRVL_DBG_LOG_EVENT: 
        { 
            GenericUSHORT = Adapter->LogEventCfg; 
            *BytesWritten=sizeof(USHORT);
            DBGPRINT(DBG_OID|DBG_HELP, (L"DBG Log Evts:%x\r\n",(USHORT)Adapter->LogEventCfg));   
        }
        break; 
    case OID_MRVL_ADHOC_AWAKED_PERIOD:
        {       
            GenericUSHORT = (USHORT)Adapter->AdhocAwakePeriod;
            *BytesWritten=sizeof(USHORT);
            DBGPRINT(DBG_ADHOC|DBG_OID|DBG_HELP, (L"ADHOC AWAKE PERIOD:%x\r\n",Adapter->AdhocAwakePeriod));   
        }
    break;
    //022607
    case OID_MRVL_BCA_TIMESHARE:
        {           
           POID_MRVL_DS_BCA_TIMESHARE  bca;
 
           bca = (POID_MRVL_DS_BCA_TIMESHARE)InformationBuffer;
           
           if( InformationBufferLength < sizeof(OID_MRVL_DS_BCA_TIMESHARE) )
               return NDIS_STATUS_BUFFER_TOO_SHORT; 

           Status = ProcessOIDBCATimeshare(Adapter,HostCmd_ACT_GET,InformationBuffer);
           *BytesWritten = sizeof(OID_MRVL_DS_BCA_TIMESHARE);

           return Status;
        }
     break;
     default:
        Status = NDIS_STATUS_NOT_SUPPORTED; 
        DBGPRINT(DBG_WARNING|DBG_CUSTOM, (L"Query: Got unknown OID=%s\r\n", OIDMSG(Oid,Status)));  	 
            break;
        }  //end of switch within default
    }  //end of default
    break;
    }  //end of switch

    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (MoveBytes > InformationBufferLength)
        {
            // Not enough room in InformationBuffer
            *BytesNeeded = MoveBytes;

            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
        else
        {
            // Copy result into InformationBuffer
            *BytesWritten = MoveBytes;
            if (MoveBytes > 0)
                NdisMoveMemory(InformationBuffer, MoveSource, MoveBytes);
        }
    }
  
    return (Status);
}

/******************************************************************************
 *
 *  Name: MrvDrvSetInformation()
 *
 *  Description: NDIS miniport set information handler
 *
 *  Conditions for Use: NDIS wrapper will call this handler to set device information
 *
 *  Arguments:           
 *      IN NDIS_HANDLE MiniportAdapterContext
 *      IN NDIS_OID Oid
 *      IN PVOID InformationBuffer
 *      IN ULONG InformationBufferLength
 *      OUT PULONG BytesRead
 *      OUT PULONG BytesNeeded
 *    
 *  Return Value:
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_PENDING
 *      NDIS_STATUS_NOT_SUPPORTED
 *      NDIS_STATUS_INVALID_DATA
 *      NDIS_STATUS_INVALID_OID
 * 
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS
MrvDrvSetInformation(
  IN NDIS_HANDLE MiniportAdapterContext,
  IN NDIS_OID Oid,
  IN PVOID InformationBuffer,
  IN ULONG InformationBufferLength,
  OUT PULONG BytesRead,
  OUT PULONG BytesNeeded
  )
{

    NDIS_STATUS Status;
    PMRVDRV_ADAPTER Adapter;
    NDIS_DEVICE_POWER_STATE NewPowerState;
    UINT WakeUpFlags;
    ULONG OptFlags, MoveSize, ulSize;
    NDIS_802_11_RELOAD_DEFAULTS N11rd;
    NDIS_802_11_NETWORK_TYPE N11nt;
    NDIS_802_11_KEY_INDEX KeyIndex;
    PNDIS_802_11_WEP pNewWEP;
    ULONG tmpPktFilter;
    USHORT Action;
    USHORT Cmd = 0;
  

    Adapter = (PMRVDRV_ADAPTER)(MiniportAdapterContext); 

    DBGPRINT(DBG_OID,(L"SET - Enter MrvDrvSetInformation - 0x%x\r\n", Oid));

    //051407
    Status = AutoDeepSleepPending(Adapter);
    if( Status != NDIS_STATUS_SUCCESS)
        return Status;
        if( !IsThisDsState(Adapter, DS_STATE_NONE) )
        {
            DBGPRINT(DBG_OID|DBG_DEEPSLEEP| DBG_HELP,(L"Set OID in DSState = %x\r\n", Adapter->DSState));
            //032107 add auto deep sleep
            if (!((Oid == OID_MRVL_DEEP_SLEEP) || (Oid == OID_MRVL_HS_CANCEL) || (Oid == OID_PNP_SET_POWER)))
            {
                if( Adapter->AutoDeepSleepTime == 0 )
                {
                    DBGPRINT(DBG_OID|DBG_DEEPSLEEP| DBG_WARNING|DBG_ERROR,(L"Set: OID=0x%x, Not OID_MRVL_DEEP_SLEEP OID:return NDIS_STATUS_FAILURE\r\n",Oid));
                    return NDIS_STATUS_FAILURE;
                }
                else //power up device in DeepSleep mode
                {
                    OID_MRVL_DS_DEEP_SLEEP ds;
                    ULONG                  cnt=0;
     
                    DBGPRINT(DBG_OID|DBG_DEEPSLEEP| DBG_WARNING,(L"OID access, PWR up device in deepsleep\r\n"));
                    ds.ulEnterDeepSleep = 0;  //exit deep sleep
                    Status = ProcessOIDDeepSleep(Adapter,
                                           HostCmd_ACT_GEN_SET, 
                                           sizeof(OID_MRVL_DS_DEEP_SLEEP), 
                                           (PVOID)&ds,
                                           HostCmd_PENDING_ON_NONE); //041307
                    if( Status != NDIS_STATUS_SUCCESS )
                    {
                        DBGPRINT( DBG_ERROR,(L"Set: OID=0%x ,ProcessOIDDeepSleep Fail, return NDIS_STATUS_FAILURE\r\n",Oid));
                        return NDIS_STATUS_FAILURE; 
                    }
                    //waiting for DS state change
                    Status = If_WaitFWPowerUp(Adapter);
                    if( Status != IF_SUCCESS )
                    {   
                        DBGPRINT( DBG_ERROR,(L"Set: OID=0%x ,Fail to wait DS_AWAKE event, return NDIS_STATUS_FAILURE\r\n",Oid));
                        return NDIS_STATUS_FAILURE;  
                    }
                }
            }
        }

    if(Adapter->CurPowerState == NdisDeviceStateD3 && Oid != OID_PNP_SET_POWER)
    {
        DBGPRINT( DBG_ERROR,(L"Set: OID=0%x ,Fail to set this OID under NdisDeviceStateD3 , return NDIS_STATUS_FAILURE\r\n",Oid)); 
        return NDIS_STATUS_FAILURE; 
    }

#ifndef NDIS50_MINIPORT
    //      Check device removal status
    if( Adapter->SurpriseRemoved == TRUE )
    {
        DBGPRINT(DBG_ERROR,(L"***** Card Removed in MrvDrvSetInformation! *****\r\n"));
        return NDIS_STATUS_NOT_ACCEPTED;
    }
#else
    if( IsThisDsState(Adapter, DS_STATE_NONE) )
    {   
        if ( sdio_IsFirmwareLoaded(Adapter) == FW_STATUS_READ_FAILED)  
        {
            DBGPRINT(DBG_OID,(L"***** Card Removed ! *****\r\n"));
            return NDIS_STATUS_NOT_ACCEPTED;       
        }
    }//Ling++, 012706
#endif
    *BytesRead = 0;
    *BytesNeeded = 0;
    Status = NDIS_STATUS_SUCCESS;

    if (Adapter->UseMfgFw == 1)
    {
        if (Oid != OID_MRVL_MFG_COMMAND)
        {
            DBGPRINT(DBG_OID ,(L"Set OID received is not OID_MRVL_MFG_COMMAND in MFG driver\r\n"));
            return NDIS_STATUS_SUCCESS;
        }
    }

    if ( Oid == OID_802_11_ENCRYPTION_STATUS )        
    {
        // set for OID_802_11_SSID to figure out whether
        // it's a removal from preferred list or
        // just a clean up
        Adapter->bIsLastOIDSetEncryptionStatus = TRUE;
    }
    ///new-assoc ++
    else if (( Oid != OID_802_11_SSID ) && (Oid != OID_MRVL_ASSOC))
    ///new-assoc --
    {
        Adapter->bIsLastOIDSetEncryptionStatus = FALSE;
    }

        if (Adapter->bIsReconnectEnable)    
       {
        if (((Adapter->bIsAcceptSystemConnect == TRUE) &&  (Adapter->bIsReConnectNow == TRUE)) ||
            (Adapter->bIsAcceptSystemConnect == FALSE))
           {
            if ((Oid == OID_802_11_INFRASTRUCTURE_MODE) ||
                    (Oid == OID_802_11_AUTHENTICATION_MODE) ||
                    (Oid == OID_802_11_ENCRYPTION_STATUS) ||
                    (Oid == OID_802_11_ADD_KEY))

                return NDIS_STATUS_SUCCESS;

            else if (Oid == OID_802_11_BSSID_LIST_SCAN) 
                return NDIS_STATUS_SUCCESS; 
            ///new-assoc ++
            else if ((Oid == OID_802_11_SSID) || (Oid == OID_MRVL_ASSOC))
            ///new-assoc -- 
                return NDIS_STATUS_SUCCESS;
            }
        }

        if ( Adapter->bIsAssociateInProgress)
       {
          if ((Oid == OID_802_11_SSID) ||
            ///new-assoc ++
               (Oid == OID_MRVL_ASSOC) ||
            ///new-assoc --
               (Oid == OID_802_11_INFRASTRUCTURE_MODE) ||
               (Oid == OID_802_11_AUTHENTICATION_MODE) ||
               (Oid == OID_802_11_ENCRYPTION_STATUS) ||
               (Oid == OID_802_11_BSSID_LIST_SCAN) ||
               (Oid == OID_802_11_ADD_KEY))
               {  
                  DBGPRINT( DBG_ERROR,(L"Set: OID=%s   => Fail to set this OID whenr bIsAssociateInProgress \r\n",OIDMSG(Oid,NDIS_STATUS_FAILURE))); 
                  return NDIS_STATUS_FAILURE;
                }
          }

    switch (Oid)
    {
    	
        case OID_MRVL_S56:
        {
            Adapter->bEnableS56DriverIAPP = *((ULONG *)InformationBuffer); 	
            return Status;
        }
    	

        case OID_MRVL_ADHOC_DEFAULT_CHANNEL:
        {
            POID_DS_MRVL_ADHOC_CHANNEL      pAdhocChan = (POID_DS_MRVL_ADHOC_CHANNEL) InformationBuffer;

            if ( InformationBufferLength < sizeof(OID_DS_MRVL_ADHOC_CHANNEL) )
                return NDIS_STATUS_INVALID_LENGTH;

            // We need to check if the channel is valid.
            if ( !IsValidChannel( Adapter, pAdhocChan->ulChannelNumber ) )
            {
                DBGPRINT( DBG_OID|DBG_ADHOC, (L"The channel (%d) is invalid", pAdhocChan->ulChannelNumber) );
                return NDIS_STATUS_INVALID_DATA;
            }

            DBGPRINT( DBG_OID|DBG_ADHOC, (L"SET OID_MRVL_ADHOC_DEFAULT_CHANNEL - new channel is %d\n", pAdhocChan->ulChannelNumber ) );

            Adapter->AdhocDefaultChannel = pAdhocChan->ulChannelNumber;
        }
        break;
            
        case OID_MRVL_ADHOC_G_PROTECTION:
        {
            ULONG ulGProtect = *((PULONG)InformationBuffer);

            if ( ulGProtect )
                Adapter->CurrentMacControl |= HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON;
            else
                Adapter->CurrentMacControl &= ~HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON;

            SetMacControl( Adapter );
        }
        break;

        case OID_MRVL_BG_SCAN_CONFIG_UAPSD:
        case OID_MRVL_BG_SCAN_CONFIG:
             Status = ProcessOIDBgScanCfg(Adapter, 
                                          Oid,
                                          InformationBuffer,
                                          InformationBufferLength,
                                          BytesNeeded); 
             return Status;
            break;
    
      case OID_MRVL_POWER_ADAPT_CFG_EXT:
            {
                POID_MRVL_DS_POWER_ADAPT_CFG_EXT pOidPA;
         
                if (InformationBufferLength >= sizeof(ULONG) + 2*sizeof(USHORT) ) 
                {
                    pOidPA = (POID_MRVL_DS_POWER_ADAPT_CFG_EXT) InformationBuffer;

                }
                else
                {
                    return NDIS_STATUS_INVALID_LENGTH;
                }

                
                if ( pOidPA->EnablePA==1) 
                    NdisZeroMemory(Adapter->PACfg, LENGTH_PA_CFG);

                SaveCurrentConfig( Adapter, InformationBuffer, InformationBufferLength,HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT );

                Status = PrepareAndSendCommand( 
                                     Adapter, 
                                     HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT,
                                     HostCmd_ACT_SET, 
                                     HostCmd_OPTION_USE_INT,
                                     0, 
                                     HostCmd_PENDING_ON_SET_OID,
                                     0, 
                                     FALSE,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL );
    
                if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
                 {
                	   DBGPRINT(DBG_ERROR, (L"Set:OID=%s\r\n", OIDMSG(Oid,NDIS_STATUS_FAILURE)));  
                    return NDIS_STATUS_FAILURE; 
                 }

                TT_CMDPARSE_SET_WAIT_COMPLETE_THEN_GO_OR_RETURN( Adapter, HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT );
        
            }
            break;
    case OID_802_11_PMKID:
    {
        PNDIS_802_11_PMKID      pPmkid = (PNDIS_802_11_PMKID) InformationBuffer;
        PPMKID_CACHE            pPmkidCache = Adapter->PmkidCache;

        DBGPRINT(DBG_OID|DBG_MACEVT, (L"SET - OID_802_11_PMKID\r\n") );
        DBGPRINT(DBG_OID|DBG_MACEVT, (L"    %d PMKIDs [len=%d]\r\n", pPmkid->BSSIDInfoCount, pPmkid->Length) );

        DBGPRINT(DBG_OID|DBG_MACEVT, (L"    AuthMode=%d\r\n", Adapter->AuthenticationMode) );

        // TODO: if Adapter->AuthenticationMode != Ndis802_11AuthModeWPA2 then return NDIS_STATUS_INVALID_DATA

        if ( pPmkid->BSSIDInfoCount > MAX_PMKID_CACHE )
            return NDIS_STATUS_NOT_ACCEPTED;

        // clear all cache
        ResetPmkidCache( Adapter );

        // copy to our cache
        if ( SavePmkidToCache( Adapter, pPmkid ) == 0 )
            return NDIS_STATUS_NOT_ACCEPTED;

        //tt just for debugging
        DbgDumpCurrentPmkidCache( Adapter );
        //tt
    }
    break;

//tt ++ v5 firmware
  case OID_MRVL_CAL_DATA:
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_CAL_DATA, OID_MRVL_CAL_DATA, 
      sizeof(OID_MRVL_DS_CAL_DATA), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_CAL_DATA, sizeof(OID_MRVL_DS_CAL_DATA) );
    break;
  case OID_MRVL_CAL_DATA_EXT:
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_CAL_DATA_EXT, OID_MRVL_CAL_DATA_EXT, 
      sizeof(OID_MRVL_DS_CAL_DATA_EXT), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_CAL_DATA_EXT, sizeof(OID_MRVL_DS_CAL_DATA_EXT) );
    break;

  case OID_MRVL_PWR_CFG:
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_PWR_CFG, OID_MRVL_PWR_CFG, 
      sizeof(OID_MRVL_DS_PWR_CFG), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_PWR_CFG, sizeof(OID_MRVL_DS_PWR_CFG) );
    break;
  case OID_MRVL_TPC_CFG:
    TT_CMDPARSE_DUMP_DATA3( InformationBuffer, OID_MRVL_DS_TPC_CFG, P0, P1, P2 );
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_TPC_CFG, OID_MRVL_TPC_CFG, 
      sizeof(OID_MRVL_DS_TPC_CFG), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_TPC_CFG, sizeof(OID_MRVL_DS_TPC_CFG) );
    break;
  case OID_MRVL_RATE_ADAPT_RATESET:
    
       Status = ProcessOIDRateAdaptRateSet(Adapter,
                                          HostCmd_ACT_SET,
                                          InformationBufferLength,
                                          BytesNeeded,
                                          InformationBuffer);
       break;

  case OID_802_3_MULTICAST_LIST:
  {
    //      The data must be a multiple of the Ethernet address size.
    if (InformationBufferLength % MRVDRV_ETH_ADDR_LEN != 0)
      return (NDIS_STATUS_INVALID_LENGTH);  

    //      Save these new MC addresses to our adapter object
    NdisMoveMemory(Adapter->MulticastList, InformationBuffer, InformationBufferLength);  

    //      Save the number of MC address in our adapter object
    Adapter->NumOfMulticastMACAddr = (InformationBufferLength / ETH_LENGTH_OF_ADDRESS);
    
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_MAC_MULTICAST_ADR, OID_802_3_MULTICAST_LIST, 
      0, HostCmd_ACT_GEN_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_3_MULTICAST_LIST, InformationBufferLength );
      }
    break;

  case OID_GEN_PROTOCOL_OPTIONS:
    OptFlags = *((PULONG)InformationBuffer);

    if( OptFlags & NDIS_PROT_OPTION_ESTIMATED_LENGTH ){
      Status = NDIS_STATUS_NOT_SUPPORTED;
      break;
    }
    
    NdisMoveMemory((PVOID)&(Adapter->ProtocolOptions), InformationBuffer, sizeof(ULONG));
    //      Currently, we only suport NDIS_PROT_OPTION_NO_LOOPBACK in the MAC
        break;


    case OID_GEN_CURRENT_PACKET_FILTER:
      {
    DBGPRINT(DBG_OID,(L"*** ENTER OID_GEN_CURRENT_PACKET_FILTER\r\n"));

        //      Verify the Length
        if (InformationBufferLength != 4)
            return (NDIS_STATUS_INVALID_LENGTH);

        //      Now call the filter function to set the packet filter.
        NdisMoveMemory((PVOID)&tmpPktFilter, InformationBuffer, sizeof(ULONG));


    // If any bits are set that we don't support, return NDIS_STATUS_NOT_SUPPORTED
        if ( tmpPktFilter &
            ~(NDIS_PACKET_TYPE_DIRECTED |
             NDIS_PACKET_TYPE_MULTICAST |
             NDIS_PACKET_TYPE_BROADCAST |
             NDIS_PACKET_TYPE_PROMISCUOUS |
             NDIS_PACKET_TYPE_ALL_MULTICAST))
    {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            *BytesRead = 4;
            break;
        }

        //0330 not support promiscuous mode
        if ( tmpPktFilter & NDIS_PACKET_TYPE_PROMISCUOUS )
        {
           DBGPRINT(DBG_OID|DBG_ERROR,(L"*** Reject NDIS_PACKET_TYPE_PROMISCUOUS in pkt filter ***\r\n"));
           *BytesRead = 4;
           return NDIS_STATUS_NOT_SUPPORTED;
        }

    Adapter->CurrentPacketFilter = tmpPktFilter;

        if( tmpPktFilter & NDIS_PACKET_TYPE_ALL_MULTICAST )
            Adapter->CurrentMacControl |= HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;
        else
            Adapter->CurrentMacControl &= ~HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;

        if ( tmpPktFilter & NDIS_PACKET_TYPE_MULTICAST )
            Adapter->CurrentMacControl |= HostCmd_ACT_MAC_MULTICAST_ENABLE;
        else
            Adapter->CurrentMacControl &= ~HostCmd_ACT_MAC_MULTICAST_ENABLE;

        if ( tmpPktFilter & NDIS_PACKET_TYPE_BROADCAST )
            Adapter->CurrentMacControl |=  HostCmd_ACT_MAC_BROADCAST_ENABLE;
        else
            Adapter->CurrentMacControl &=  ~HostCmd_ACT_MAC_BROADCAST_ENABLE;

        if ( tmpPktFilter & NDIS_PACKET_TYPE_PROMISCUOUS )
            Adapter->CurrentMacControl |= HostCmd_ACT_MAC_PROMISCUOUS_ENABLE;
        else
            Adapter->CurrentMacControl &= ~HostCmd_ACT_MAC_PROMISCUOUS_ENABLE; 

    Status = SetMacControl(Adapter);
   }
        break;


    case OID_GEN_CURRENT_LOOKAHEAD:

        //      Verify the Length
        if (InformationBufferLength != 4)
            return (NDIS_STATUS_INVALID_LENGTH);
    NdisMoveMemory((PVOID)&(Adapter->CurrentLookAhead), InformationBuffer, sizeof(ULONG));
        *BytesRead = 4;
        DBGPRINT(DBG_RX, (L"OID: Set OID_GEN_CURRENT_LOOKAHEAD to %d\r\n", Adapter->CurrentLookAhead));
        Status = NDIS_STATUS_SUCCESS;
        break;

    case OID_PNP_SET_POWER:

         DBGPRINT(DBG_OID,(L"SET - OID_PNP_SET_POWER \r\n"));


         NewPowerState = (NDIS_DEVICE_POWER_STATE)*(PNDIS_DEVICE_POWER_STATE)InformationBuffer;
         DBGPRINT(DBG_OID | DBG_POWER | DBG_ALLEN, 
                 (L"SET - OID_PNP_SET_POWER, new power state = %d\r\n", NewPowerState));

        //return NDIS_STATUS_NOT_SUPPORTED;

    Status = SetStationPowerState(
          Adapter, 
          NewPowerState, 
          Oid, 
          BytesRead, 
          BytesNeeded, 
          InformationBuffer);
        
    break;

    case OID_PNP_ADD_WAKE_UP_PATTERN:
        DBGPRINT(DBG_POWER | DBG_ALLEN,(L"OID_PNP_ADD_WAKE_UP_PATTERN\r\n"));
      Status = NDIS_STATUS_NOT_SUPPORTED;
    //      ToDo : Status = SetStationWakeUpPattern(Adapter, NewPattern);
        break;

    case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        DBGPRINT(DBG_POWER | DBG_ALLEN,(L"OID_PNP_REMOVE_WAKE_UP_PATTERN\r\n"));
        Status = NDIS_STATUS_NOT_SUPPORTED;
    //      ToDo : Status = ReMoveStationWakeUpPattern(Adapter, NewPattern);
        break;

    case OID_PNP_ENABLE_WAKE_UP:
         DBGPRINT(DBG_POWER | DBG_ALLEN,(L"OID_PNP_ENABLE_WAKE_UP\r\n"));
         WakeUpFlags = (UINT)*(PUINT)InformationBuffer;
         if( WakeUpFlags & NDIS_PNP_WAKE_UP_LINK_CHANGE )
             Status = NDIS_STATUS_SUCCESS;
         else
             Status = NDIS_STATUS_NOT_SUPPORTED;
         break;

  case OID_802_11_BSSID:
  	{
            //Set expected BSSID 
            NdisMoveMemory(Adapter->ReqBSSID, InformationBuffer, MRVDRV_ETH_ADDR_LEN);
            DBGPRINT(DBG_OID|DBG_ASSO|DBG_HELP,(L"SET - OID_802_11_BSSID:xxx%x%x\r\n", Adapter->ReqBSSID[4],Adapter->ReqBSSID[5])); 

            Status = NDIS_STATUS_SUCCESS;
  	}	
    break;

    case OID_802_11_SSID:
    ///new-assoc ++
    case OID_MRVL_ASSOC:
        DBGPRINT(DBG_V9|DBG_OID|DBG_RECONNECT|DBG_WARNING,
            (L"SET - %s ****\r\n", ((Oid == OID_802_11_SSID)? TEXT("OID_802_11_SSID"): TEXT("OID_MRVL_ASSOC")))); 

        Status = ProcessOIDMrvlAssoc(Adapter, InformationBuffer, InformationBufferLength,
                                    BytesRead, BytesNeeded, 
                                    ((Oid == OID_802_11_SSID)? TRUE: FALSE));
    break;
    ///new-assoc --
    case OID_GEN_MACHINE_NAME:
         DBGPRINT(DBG_OID|DBG_WARNING,(L"SET - OID_GEN_MACHINE_NAME ****\r\n"));

         if( InformationBufferLength < MRVDRV_DEFAULT_MACHINE_NAME_SIZE )
         MoveSize = InformationBufferLength;
         else
         MoveSize = MRVDRV_DEFAULT_MACHINE_NAME_SIZE-1;
         NdisMoveMemory(Adapter->MachineName, InformationBuffer, MoveSize);
         break;
    case OID_802_11_NETWORK_TYPE_IN_USE:
        DBGPRINT(DBG_OID,(L"SET - OID_802_11_NETWORK_TYPE_IN_USE ****\r\n"));

        MoveSize = InformationBufferLength;
        N11nt = *((PNDIS_802_11_NETWORK_TYPE)InformationBuffer);
        if( N11nt != Ndis802_11DS )
           Status = NDIS_STATUS_INVALID_DATA;
        break;
  case OID_802_11_RSSI_TRIGGER:
    DBGPRINT(DBG_OID,(L"SET - OID_802_11_RSSI_TRIGGER ****\r\n"));
    Adapter->RSSITriggerValue = (NDIS_802_11_RSSI) InformationBuffer; 
    break;


  case OID_MRVL_RSSI_BCNAVG:
  {            
         Adapter->bcn_avg_factor = (USHORT)*((ULONG *)InformationBuffer);
            DBGPRINT(DBG_OID|DBG_HELP, (L"Set: OID_MRVL_RSSI_BCNAVG: Adapter->bcn_avg_factor=%d\r\n",Adapter->bcn_avg_factor) );           
    }
    break;
    
  case OID_MRVL_RSSI_DATAAVG :
  {            
         Adapter->data_avg_factor = (USHORT)*((ULONG *)InformationBuffer);
            DBGPRINT(DBG_OID|DBG_HELP, (L"Set: OID_MRVL_RSSI_DATAAVG: Adapter->data_avg_factor=%d\r\n",Adapter->data_avg_factor) );            
   }
   break;   

   case OID_MRVL_RSSI_THRESHOLDTIMER :
  {            
         Adapter->ulRSSIThresholdTimer= (ULONG)*((ULONG *)InformationBuffer);
            DBGPRINT(DBG_OID|DBG_HELP, (L"Set: OID_MRVL_RSSI_THRESHOLDTIMER: Adapter->ulRSSIThresholdTimer=%d\r\n",Adapter->ulRSSIThresholdTimer) );           
   }
   break;   

  case OID_802_11_INFRASTRUCTURE_MODE: 
    {   //051107 Coverity 060407
        NDIS_802_11_NETWORK_INFRASTRUCTURE PreviousMode=Adapter->InfrastructureMode;
        
      if(Adapter->TCloseWZCFlag==WZC_Infrastructure_Mode)
      {
              Adapter->InfrastructureMode = Ndis802_11Infrastructure;
      }else if(Adapter->TCloseWZCFlag==WZC_Adhoc_Mode)
      {
             Adapter->InfrastructureMode = Ndis802_11IBSS;
          }else{ 
        PreviousMode = Adapter->InfrastructureMode; 
    
    //Set infrastructure mode
    Adapter->InfrastructureMode = *((PNDIS_802_11_NETWORK_INFRASTRUCTURE)
                                        InformationBuffer);
            }
    DBGPRINT(DBG_V9|DBG_OID, (L"SET - OID_802_11_INFRASTRUCTURE_MODE, Mode = 0x%x \r\n", 
                           Adapter->InfrastructureMode));  


       if (Adapter->MediaConnectStatus == NdisMediaStateConnected)
      {
          if( Adapter->PSMode == Ndis802_11PowerModeMAX_PSP)
         {   
             Adapter->PSMode_B = Ndis802_11PowerModeMAX_PSP;
             Adapter->PSMode = Ndis802_11PowerModeCAM;   
             Adapter->sp_evt = 1;
             PSWakeup(Adapter);  
         }
        
        Cmd = 0;
          if(Adapter->TCloseWZCFlag==WZC_Default ||Adapter->TCloseWZCFlag==WZC_Ignore_Send_EAPOL_START)
         {
            if(Adapter->InfrastructureMode == Ndis802_11IBSS)
            {
                  if(PreviousMode == Ndis802_11Infrastructure)
                    Cmd = HostCmd_CMD_802_11_DEAUTHENTICATE;
                  else   
                    Cmd = HostCmd_CMD_802_11_AD_HOC_STOP;
            }
            else
            { 
                if(PreviousMode == Ndis802_11IBSS)
                   Cmd = HostCmd_CMD_802_11_AD_HOC_STOP;
            }    

            if(Cmd)
            {   
             
       Status = PrepareAndSendCommand(
                               Adapter,
                               Cmd,
                               0,
                               HostCmd_OPTION_USE_INT,
                               (NDIS_OID)0,
                               HostCmd_PENDING_ON_SET_OID,
                               0,
                               FALSE,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
                               
       if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
       {
            DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));  
              return Status;
       }		  
       else
      {   
      //RETAILMSG(1, (L"infrat-struct mode\n"));
          ResetDisconnectStatus(Adapter); 
          TT_CMDPARSE_SET_WAIT_COMPLETE_THEN_GO_OR_RETURN( Adapter, Cmd );
      }
      }
   }    
      } //if (Adapter->MediaConnectStatus == NdisMediaStateConnected)                      
    
    
       
       Status = PrepareAndSendCommand(
                       Adapter,
                       HostCmd_CMD_802_11_SNMP_MIB,
                       (USHORT)Adapter->InfrastructureMode,
                       HostCmd_OPTION_USE_INT,
                       Oid,
                       HostCmd_PENDING_ON_SET_OID,   // tt v5 firmware
                       0,
                       FALSE,
                       NULL,
                       BytesRead,
                       BytesNeeded,
                       InformationBuffer);
                       
             
       if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
       {
             DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));    
              return Status;
       }		  
       else
       {
             TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_INFRASTRUCTURE_MODE, sizeof(NDIS_802_11_NETWORK_INFRASTRUCTURE) );
       }
     }      
           break;

  case OID_802_11_FRAGMENTATION_THRESHOLD:

    Adapter->FragThsd = *((NDIS_802_11_FRAGMENTATION_THRESHOLD *)InformationBuffer);

    Status = PrepareAndSendCommand(
            Adapter,
            HostCmd_CMD_802_11_SNMP_MIB,
            0,
            HostCmd_OPTION_USE_INT,
            Oid,
            HostCmd_PENDING_ON_SET_OID,
            0,
            FALSE,
            NULL,
            BytesRead,
            BytesNeeded,
            InformationBuffer);

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
    {
         DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
      return Status;
    }
  else
  {
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_FRAGMENTATION_THRESHOLD, sizeof(NDIS_802_11_FRAGMENTATION_THRESHOLD) );
  }

    break;        
      
  case OID_802_11_RTS_THRESHOLD:
  
    Adapter->RTSThsd = *((NDIS_802_11_RTS_THRESHOLD *)InformationBuffer);

    Status = PrepareAndSendCommand(
            Adapter,
            HostCmd_CMD_802_11_SNMP_MIB,
            0,
            HostCmd_OPTION_USE_INT,
            Oid,
            HostCmd_PENDING_ON_SET_OID,
            0,
            FALSE,
            NULL,
            BytesRead,
            BytesNeeded,
            InformationBuffer); 

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
   {
        DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status))); 
      return Status;
    }
  else
  {
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_RTS_THRESHOLD, sizeof(NDIS_802_11_RTS_THRESHOLD) );
  }
    break;              
    
  case OID_802_11_CONFIGURATION:
       DBGPRINT(DBG_OID,(L"SET - OID_802_11_CONFIGURATION ****\r\n"));
       //      Can only be set when NIC is not associated with any AP
       if( Adapter->CurrentConfiguration.Length != 0 )
       {
           Status = NDIS_STATUS_FAILURE;
           break;
       }

       if ( InformationBufferLength < sizeof(NDIS_802_11_CONFIGURATION) )
       {
          Status = NDIS_STATUS_INVALID_LENGTH;
          *BytesNeeded = sizeof(NDIS_802_11_CONFIGURATION);
          break;
       }

       NdisMoveMemory(
                      &Adapter->CurrentConfiguration, 
                      InformationBuffer,
                      sizeof(NDIS_802_11_CONFIGURATION));
    
       {
            int ucChannel;

            // setup the channel
            for ( ucChannel =0; ucChannel < 15; ucChannel++ )
            {
                if ( Adapter->CurrentConfiguration.DSConfig == DSFeqList[ucChannel] )
                {
                    DBGPRINT(DBG_OID, (L"CONFIGURATION: Set Channel to %d\r\n",ucChannel));

                    Adapter->Channel = ucChannel;
                    break;
                }
            } // for ( ucChannel =0; ucChannel < 15; ucChannel++ )

            if ( ucChannel == 15 )
            {
                DBGPRINT(DBG_OID|DBG_HELP, (L"CONFIGURATION: Set Channel to defult channel(%d)\r\n",DEFAULT_CHANNEL));
                // did not find the channel
                Adapter->Channel = DEFAULT_CHANNEL;
            }
        }
    break;

    case OID_802_11_ADD_WEP:
        DBGPRINT(DBG_V9|DBG_CCX,(L"SET - OID_802_11_ADD_WEP ****\r\n"));
        pNewWEP = (PNDIS_802_11_WEP)InformationBuffer;
        KeyIndex = pNewWEP->KeyIndex; 
        KeyIndex = KeyIndex & (ULONG)HostCmd_WEP_KEY_INDEX_MASK;
        DBGPRINT(DBG_OID|DBG_HELP,(L"SET - In ADD_WEP, KeyIndex:0x%X \r\n",KeyIndex));    
        if( KeyIndex >= (ULONG)HostCmd_NUM_OF_WEP_KEYS )
        { // We only support 4 WPE keys now
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        //      Check current SSID
        DBGPRINT(DBG_OID|DBG_HELP,(L"SET - In ADD_WEP, SSID = %s \r\n", Adapter->CurrentSSID.Ssid));

#ifdef NDIS50_MINIPORT
        if(pNewWEP->KeyIndex & 0x80000000)
            NdisMoveMemory(&(Adapter->CurrentWEPKey),InformationBuffer , 12 + pNewWEP->KeyLength);
#endif

    
        Status=PrepareAndSendCommand(
              Adapter,
              HostCmd_CMD_802_11_SET_WEP,
              0,
              HostCmd_OPTION_USE_INT,
              Oid,
              HostCmd_PENDING_ON_SET_OID,
              0,
              FALSE,
              NULL,
              BytesRead,
              BytesNeeded,
              InformationBuffer);

        if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
        {
              DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
            return Status;
        }		
        else
        {
            //  11/08/02 -  Move key to Adapter->LastAddedWEPKey
            NdisZeroMemory(&(Adapter->LastAddedWEPKey), sizeof(MRVL_WEP_KEY));
            if(pNewWEP->KeyLength == 5) // Assume we support 40 ans 104 bit WEP
                ulSize = sizeof(MRVL_WEP_KEY) - 11;
            else
                ulSize = sizeof(MRVL_WEP_KEY) - 3;
            NdisMoveMemory(&(Adapter->LastAddedWEPKey), pNewWEP, ulSize);
            TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_ADD_WEP, InformationBufferLength );
        }
        break;

    case OID_802_11_REMOVE_WEP:
    DBGPRINT(DBG_OID|DBG_CCX,(L"SET - OID_802_11_REMOVE_WEP ****\r\n"));
    KeyIndex = *((NDIS_802_11_KEY_INDEX *)InformationBuffer);
    KeyIndex = KeyIndex & HostCmd_WEP_KEY_INDEX_MASK;
    if( KeyIndex >= HostCmd_NUM_OF_WEP_KEYS ){ // We only support 4 WPE keys now
      Status = NDIS_STATUS_INVALID_DATA;
      break;
    }


    Status=PrepareAndSendCommand(
          Adapter,
          HostCmd_CMD_802_11_SET_WEP,
          0,
          HostCmd_OPTION_USE_INT,
          Oid,
          HostCmd_PENDING_ON_SET_OID,
          0,
          FALSE,
          NULL,
          BytesRead,
          BytesNeeded,
          InformationBuffer);

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
    {
        DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));  
      return Status;
    }
  else
  {
    //  1/23/02 -  set Adapter->LastAddedWEPKey.Length to 0xffff to indicate 
    //             removing keys
    KeyIndex = *((NDIS_802_11_KEY_INDEX *)InformationBuffer);
    NdisZeroMemory(&(Adapter->LastAddedWEPKey), sizeof(MRVL_WEP_KEY));
    Adapter->LastAddedWEPKey.Length = 0xffff;
    Adapter->LastAddedWEPKey.KeyIndex = KeyIndex;
    DBGPRINT(DBG_OID,(L"SET - Ready to removing key index = %d ****\r\n", KeyIndex));
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_REMOVE_WEP, sizeof(NDIS_802_11_KEY_INDEX) );
  }

    break;

  case OID_802_11_DISASSOCIATE:
 {
    DBGPRINT(DBG_OID,(L"SET - OID_802_11_DISASSOCIATE ****\r\n"));

    // if the card is disassociated, then just return
    if ( Adapter->MediaConnectStatus == NdisMediaStateDisconnected)
    {
      return NDIS_STATUS_SUCCESS;
    }

            if ( Adapter->InfrastructureMode == Ndis802_11Infrastructure )
            {
                Cmd= HostCmd_CMD_802_11_DEAUTHENTICATE;
            }
            else
            {
                Cmd = HostCmd_CMD_802_11_AD_HOC_STOP;
            }

        // Call Deauthenticate cmd instead of Disassociate cmd
        Status=PrepareAndSendCommand(
              Adapter,
              Cmd,
              0,
              HostCmd_OPTION_USE_INT,
              Oid,
              HostCmd_PENDING_ON_SET_OID,
              0,
              FALSE,
              NULL,
              BytesRead,
              BytesNeeded,
              InformationBuffer);

     if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
     {
          DBGPRINT(DBG_ERROR, (L" Set:OID=%s \r\n", OIDMSG(Oid,Status)));
      return Status;
     }		  
  else
  {
    //RETAILMSG(1, (L"oid: dis-assoc\n"));
    ResetDisconnectStatus(Adapter); 
          TT_CMDPARSE_SET_WAIT_COMPLETE_THEN_GO_OR_RETURN( Adapter, Cmd );
      }
  }
    break;

    case OID_802_11_AUTHENTICATION_MODE:
    {
        if ((*((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer) != Ndis802_11AuthModeOpen) &&
            (*((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer) != Ndis802_11AuthModeWPAPSK) &&
            (*((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer) != Ndis802_11AuthModeWPA) &&
            (*((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer) != Ndis802_11AuthModeWPANone) &&
            (*((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer) != Ndis802_11AuthModeShared ) &&
            (*((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer) != Ndis802_11AuthModeWPA2 )   &&
            (*((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer) != Ndis802_11AuthModeWPA2PSK ))
        {
            DBGPRINT(DBG_OID | DBG_WARNING | DBG_WPA,
                     (L"*** SET - OID_802_11_AUTHENTICATION_MODE - unsupported mode (%d) \r\n",
                     *((PNDIS_802_11_AUTHENTICATION_MODE)InformationBuffer)));
          
            return NDIS_STATUS_NOT_SUPPORTED;
        }
   
        Adapter->AuthenticationMode = *((PNDIS_802_11_AUTHENTICATION_MODE)
                                            InformationBuffer);

        DBGPRINT(DBG_V9|DBG_OID,
                 (L"SET - OID_802_11_AUTHENTICATION_MODE, Mode = 0x%x \r\n", 
                 Adapter->AuthenticationMode));
        break;
    }

  case OID_802_11_PRIVACY_FILTER:
    
    DBGPRINT(DBG_OID,(L"SET - OID_802_11_PRIVACY_FILTER ****\r\n"));
    Adapter->PrivacyFilter = *((PNDIS_802_11_PRIVACY_FILTER)InformationBuffer);
    //      Generate MAC control command
    if( Adapter->PrivacyFilter != Ndis802_11PrivFilterAcceptAll ){
        //??????? need check it later 081005     
        Action = HostCmd_ACT_MAC_RX_ON | 
           HostCmd_ACT_MAC_TX_ON |
           HostCmd_ACT_MAC_WEP_ENABLE;
// v8,v9 doesn't need this           HostCmd_ACT_MAC_INT_ENABLE;
      }
    else{
         //???????? need check it later 081005
         Action = HostCmd_ACT_MAC_RX_ON | 
           HostCmd_ACT_MAC_TX_ON;
// v8,v9 doesn't need this           HostCmd_ACT_MAC_INT_ENABLE;
    }

    Status = PrepareAndSendCommand(
            Adapter,
            HostCmd_CMD_802_11_SNMP_MIB,
            Action,
            HostCmd_OPTION_USE_INT,
            Oid,
            HostCmd_PENDING_ON_SET_OID,
            0,
            FALSE,
            NULL,
            BytesRead,
            BytesNeeded,
            InformationBuffer);

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
    {  
        DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
      return Status;
    }	  
  else
  {
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_PRIVACY_FILTER, sizeof(NDIS_802_11_PRIVACY_FILTER) );
  }
    break;

    case OID_802_11_BSSID_LIST_SCAN:
  
  if(Adapter->plusDebug)  
  DBGPRINT(DBG_V9,(L"SET - OID_802_11_BSSID_LIST_SCAN ****\r\n"));
                  
       if ( Adapter->bIsScanInProgress || Adapter->bIsAssociateInProgress)
        {
            DBGPRINT(DBG_OID, (L"In Progress Scan ,reject the OID\r\n"));
            return NDIS_STATUS_SUCCESS;
        }

     if(Adapter->bAvoidScanAfterConnectedforMSecs==TRUE)
        {
            return NDIS_STATUS_SUCCESS;
        }
  
           
  if ( Adapter->MediaConnectStatus == NdisMediaStateConnected )
        {           

       Adapter->bIsScanWhileConnect = TRUE;
        }
  
    Status = PrepareAndSendCommand(
            Adapter,
            HostCmd_CMD_802_11_SCAN,
            0,
            HostCmd_OPTION_USE_INT,
            (NDIS_OID)0,
                  HostCmd_PENDING_ON_NONE,
            0,
            FALSE,
            NULL,
            NULL,
            NULL,
            NULL);
        
    return NDIS_STATUS_SUCCESS;

        break;


    case OID_802_11_RELOAD_DEFAULTS:
    
        DBGPRINT(DBG_OID,(L"SET - In RELOAD_DEFAULTS, SSID = %s \r\n", Adapter->CurrentSSID.Ssid));
    N11rd = *((PNDIS_802_11_RELOAD_DEFAULTS)InformationBuffer);
    if( N11rd ==  Ndis802_11ReloadWEPKeys )
      { 
            NdisZeroMemory(&(Adapter->LastAddedWEPKey), sizeof(MRVL_WEP_KEY));
    }
    else
    {
      Status = NDIS_STATUS_INVALID_DATA;
    }
    break;

  case OID_802_11_RX_ANTENNA_SELECTED: 
    
    DBGPRINT(DBG_OID,(L"SET - OID_802_11_RX_ANTENNA_SELECTED ****\r\n"));

    Adapter->RxAntenna = *((NDIS_802_11_ANTENNA *)InformationBuffer);

  TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_RF_ANTENNA, OID_802_11_RX_ANTENNA_SELECTED, 
    0, HostCmd_ACT_SET_RX, HostCmd_PENDING_ON_SET_OID, BytesRead );
  TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_RX_ANTENNA_SELECTED, sizeof(NDIS_802_11_ANTENNA) );


    break;
  
  case OID_802_11_TX_ANTENNA_SELECTED: 
    DBGPRINT(DBG_OID,(L"SET - OID_802_11_TX_ANTENNA_SELECTED ****\r\n"));
 
      Adapter->TxAntenna = *((NDIS_802_11_ANTENNA *)InformationBuffer);

  TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_RF_ANTENNA, OID_802_11_TX_ANTENNA_SELECTED, 
    0, HostCmd_ACT_SET_TX, HostCmd_PENDING_ON_SET_OID, BytesRead );
  TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_TX_ANTENNA_SELECTED, sizeof(NDIS_802_11_ANTENNA) );


    break;
 
 
  case OID_802_11_DESIRED_RATES:
         Status = ProcessOIDDesiredRates(Adapter,
                                    HostCmd_ACT_SET,  
                                    InformationBufferLength, 
                                    BytesNeeded,
                                    InformationBuffer);
       return Status;
	case OID_802_11_TX_POWER_LEVEL:
	{
		ULONG PowerLevel ;

		PowerLevel = *((ULONG *)InformationBuffer);
		DBGPRINT(DBG_OID,(L"SET - OID_802_11_TX_POWER_LEVEL ****\r\n"));
        
		if ( InformationBufferLength < sizeof(NDIS_802_11_TX_POWER_LEVEL) ) {
			*BytesNeeded = sizeof(NDIS_802_11_TX_POWER_LEVEL);
			return NDIS_STATUS_INVALID_LENGTH;
		} 

		DBGPRINT(DBG_OID,(L"\r\n TX_POWER_LEVEL>>>> %d\r\n",PowerLevel));
		wlan_ccx_recTxPower(PowerLevel);
		ProcessOIDSetTxPowerLevel(Adapter, PowerLevel);
		break;
	}

    case OID_802_11_POWER_MODE:
        {
            USHORT NewPSMode;

            // the driver currently only support Ndis802_11PowerModeMAX_PSP
            // and Ndis802_11PowerModeCAM
            NewPSMode = (USHORT)*((ULONG *)InformationBuffer);
            DBGPRINT(DBG_PS | DBG_OID, (L"SET - OID_802_11_POWER_MODE: %d\r\n", NewPSMode));


            if ((NewPSMode != Ndis802_11PowerModeMAX_PSP) &&
                (NewPSMode != Ndis802_11PowerModeCAM) )
            {
                DBGPRINT(DBG_PS | DBG_WARNING | DBG_OID,(L"New power mode not supported!\r\n"));
                return NDIS_STATUS_NOT_SUPPORTED;
            }

            if ( Adapter->PSMode == NewPSMode )
            {
                if(Adapter->PSMode_B!=Adapter->PSMode)
                    Adapter->PSMode_B=Adapter->PSMode;      
                return NDIS_STATUS_SUCCESS;
            }

            if ( Adapter->PSMode == Ndis802_11PowerModeCAM )
            {
                DBGPRINT(DBG_PS,(L"PowerMode: Changed from CAM to MAX_PSP\r\n"));

                Adapter->PSMode = NewPSMode;
                Adapter->PSMode_B = NewPSMode;      
                if ( Adapter->MediaConnectStatus == NdisMediaStateDisconnected )
                {
                    return NDIS_STATUS_SUCCESS;
                }
                DBGPRINT(DBG_PS|DBG_OID|DBG_HELP, (L"Enter Power Save\r\n"));
                PSSleep(Adapter);                
            }
            else
            {
                DBGPRINT(DBG_PS,(L"PowerMode: Changed from MAX_PSP to CAM\r\n"));
                
                Adapter->PSMode = NewPSMode;
                Adapter->PSMode_B = NewPSMode;      
                if ( Adapter->MediaConnectStatus == NdisMediaStateDisconnected )
                {
                    return NDIS_STATUS_SUCCESS;
                }
                DBGPRINT(DBG_PS|DBG_OID|DBG_HELP, (L"Exit Power Save\r\n"));

                // IEEE PS is CAM and also MAX_PSP in new PS schema
                PSWakeup(Adapter);
            }     
        }
    break;


    case OID_802_11_TEST:
        {
            PNDIS_802_11_TEST   pTest;

            DBGPRINT(DBG_OID, (L"SET - OID_802_11_TEST ****\r\n"));
            if ( InformationBufferLength < sizeof(NDIS_802_11_TEST) )
            {
                *BytesNeeded = sizeof(NDIS_802_11_TEST);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pTest = (PNDIS_802_11_TEST)InformationBuffer;

            if ( pTest->Type == 1 )
            {
                NdisMIndicateStatus(Adapter->MrvDrvAdapterHdl,
          NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                    &pTest->AuthenticationEvent,
                    pTest->Length);
        NdisMIndicateStatusComplete(Adapter->MrvDrvAdapterHdl);
            }
            else if ( pTest->Type == 2 )
            {
                NdisMIndicateStatus(Adapter->MrvDrvAdapterHdl,
          NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                    &pTest->RssiTrigger,
                    sizeof(pTest->RssiTrigger));
        NdisMIndicateStatusComplete(Adapter->MrvDrvAdapterHdl);
            }
            else
            {
                return NDIS_STATUS_INVALID_DATA;
            }
        }
        break;

    case OID_802_11_ADD_KEY:
        {
            PNDIS_802_11_KEY    pKey;

            DBGPRINT(DBG_V9|DBG_OID|DBG_CCX, (L"SET - OID_802_11_ADD_KEY, Auth Mode %x \r\n",
                     Adapter->AuthenticationMode));

            // The alignment and storage requirement for KeyMaterial field is
            // creating for sizeof(NDIs_802_11_KEY).  8 bytes is added
            // automatically because of KeyMaterial even if the key
            // is less than 8 bytes
            if ( InformationBufferLength < (sizeof(NDIS_802_11_KEY)-8) )
            {
                *BytesNeeded = sizeof(NDIS_802_11_KEY);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pKey = (PNDIS_802_11_KEY)InformationBuffer;

            DBGPRINT(DBG_OID|DBG_HELP, (L"Length: %d\r\n",pKey->Length));
            DBGPRINT(DBG_OID|DBG_HELP, (L"KeyIndex: 0x%x\r\n",pKey->KeyIndex));
            DBGPRINT(DBG_OID|DBG_HELP, (L"KeyLength: %d\r\n",pKey->KeyLength));
            DBGPRINT(DBG_OID|DBG_HELP, (L"BSSID: 0x%2x-0x%2x-0x%2x-0x%2x-0x%2x-0x%2x\r\n",
                                pKey->BSSID[0],
                                pKey->BSSID[1],
                                pKey->BSSID[2],
                                pKey->BSSID[3],
                                pKey->BSSID[4],
                                pKey->BSSID[5]));

            DBGPRINT(DBG_OID|DBG_HELP, (L"KeyRSC: 0x%I64x\r\n",pKey->KeyRSC));

            if ( InformationBufferLength < pKey->Length )
            {
                DBGPRINT( DBG_OID | DBG_WPA | DBG_CCX, 
                          (L"Invalid buffer size:NDIS_802_11_KEY size = %d, pKey->KeyLength = %d InfoBufLength = %d\r\n",
                                              sizeof(NDIS_802_11_KEY),
                                              pKey->KeyLength,
                                              InformationBufferLength));

                *BytesNeeded = sizeof(NDIS_802_11_KEY) + pKey->KeyLength;         
                return NDIS_STATUS_INVALID_DATA;
            }

            // current driver only supports key length of up to 32 bytes
            if ( pKey->KeyLength > MRVL_MAX_WPA_KEY_LENGTH )
            {
                return NDIS_STATUS_INVALID_DATA;
            }

            // check bit 30 for pairwise key, if pairwise key, store in 
            // location 0 of the key map
            if ( pKey->KeyIndex & 0x40000000 )
            {
                UCHAR ucBroadcastMAC[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
                UCHAR ucIdx;

                // check for illegal combination
                if ( ! (pKey->KeyIndex & 0x80000000) )
                {
                    DBGPRINT(DBG_OID | DBG_WPA | DBG_CCX|DBG_HELP, (L"Illegal KeyIndex combination: 0x%x\r\n",
                                                 pKey->KeyIndex));
                    return NDIS_STATUS_INVALID_DATA;
                }

                if ( NdisEqualMemory(ucBroadcastMAC, pKey->BSSID,
                                     sizeof(NDIS_802_11_MAC_ADDRESS)) )
                {
                    DBGPRINT(DBG_OID | DBG_WPA | DBG_CCX|DBG_HELP, (L"Pairwise key rejected, has boradcast BSSID\r\n"));
                                            
                    return NDIS_STATUS_INVALID_DATA;
                }

                if ( ! NdisEqualMemory(pKey->BSSID,
                                       Adapter->KeyMap[0].BSSID,
                                       sizeof(NDIS_802_11_MAC_ADDRESS)) )
                {
                    // current pairwise key is different from the new key
                    // delete the current key
                    DBGPRINT(DBG_OID | DBG_WPA | DBG_CCX|DBG_HELP, (L"Current BSSID is different from BSSID of new key!\r\n"));
                    NdisZeroMemory(&Adapter->KeyMap[0],
                                   sizeof(MRVL_WPA_KEY_SET));
                }

                // search for idential KeyIndex;
                for ( ucIdx = 0; ucIdx < MRVL_NUM_WPA_KEY_PER_SET; ucIdx++ )
                {
                    if ((Adapter->KeyMap[0].Key[ucIdx].KeyLength == 0) || 
                        (Adapter->KeyMap[0].Key[ucIdx].KeyIndex == pKey->KeyIndex) )
                    {
                        // replace this entry
                        break;
                    }
                }

                if ( ucIdx == MRVL_NUM_WPA_KEY_PER_SET )
                {
                    // did not find any matching key
                    // or empty slot

                    // just place the key at the end
                    // TODO: check if this is correct

                    ucIdx--;
                }

                // copy key into the structure
                NdisMoveMemory(Adapter->KeyMap[0].BSSID,
                               pKey->BSSID,
                               sizeof(NDIS_802_11_MAC_ADDRESS));

                Adapter->KeyMap[0].Key[ucIdx].KeyIndex = pKey->KeyIndex;
                Adapter->KeyMap[0].Key[ucIdx].KeyLength = pKey->KeyLength;
                NdisMoveMemory(&Adapter->KeyMap[0].Key[ucIdx].KeyRSC,
                               &pKey->KeyRSC,
                               sizeof(NDIS_802_11_KEY_RSC));
                NdisMoveMemory(Adapter->KeyMap[0].Key[ucIdx].KeyMaterial,
                               pKey->KeyMaterial,
                               pKey->KeyLength);

                Adapter->pLastAddedPWK = &Adapter->KeyMap[0].Key[ucIdx];

                // check if need to immediately send the key down
                if ( Adapter->MediaConnectStatus == NdisMediaStateConnected )
                {
                    if (Adapter->EncryptionStatus == Ndis802_11Encryption2KeyAbsent)
                        Adapter->EncryptionStatus = Ndis802_11Encryption2Enabled;
                    else if (Adapter->EncryptionStatus == Ndis802_11Encryption3KeyAbsent)
                        Adapter->EncryptionStatus = Ndis802_11Encryption3Enabled;

                    // send the pairwise key down
      if ( pKey->KeyLength == WPA_CCKM_KEY_LEN )
      {
            MRVL_WEP_KEY WepKeyFormat;

//       v5MrvPrintFileW( L" +cckm\n" );
//            v5MrvPrintFileW( L"[Mrvl] set wep, KeyLen=%d, KeyIdx=0x%x\n", pKey->KeyLength, pKey->KeyIndex );
            DBGPRINT( DBG_WEP|DBG_CCX, ( L"WEP+TKIP encryption!" ) );
            DBGPRINT( DBG_WEP, ( L"set wep, KeyLen=%d, KeyIdx=0x%x\n", pKey->KeyLength, pKey->KeyIndex ) );


			//Adapter->WPAEnabled = FALSE;
            
            WepKeyFormat.KeyIndex = pKey->KeyIndex;
            WepKeyFormat.KeyLength = pKey->KeyLength;
            WepKeyFormat.Length = pKey->Length;

            NdisMoveMemory(WepKeyFormat.KeyMaterial,
                                   pKey->KeyMaterial,
                                   pKey->KeyLength);

            Status=PrepareAndSendCommand(
                          Adapter,
                          HostCmd_CMD_802_11_SET_WEP,
                          0,
                          HostCmd_OPTION_USE_INT,
                          OID_802_11_ADD_WEP,
                          HostCmd_PENDING_ON_NONE,
                          0,
                          FALSE,
                          NULL,
                          NULL,
                          NULL,
                          &WepKeyFormat);

                    //  copied from add_wep
                    NdisZeroMemory(&(Adapter->LastAddedWEPKey), sizeof(MRVL_WEP_KEY));
                    
                    if(pKey->KeyLength == 5) // Assume we support 40 ans 104 bit WEP
                        ulSize = sizeof(MRVL_WEP_KEY) - 11;
                    else
                        ulSize = sizeof(MRVL_WEP_KEY) - 3;    

                    NdisMoveMemory(&(Adapter->LastAddedWEPKey), &WepKeyFormat, ulSize);

                Adapter->CurrentMacControl |= (HostCmd_STATUS_MAC_WEP_ENABLE | HostCmd_ACT_MAC_WEP_TYPE);                    

                SetMacControl(Adapter);


    return NDIS_STATUS_SUCCESS; // tt v5 firmware

      }


                    Status = PrepareAndSendCommand(
                                      Adapter,
                                    HostCmd_CMD_802_11_KEY_MATERIAL,
                                    HostCmd_ACT_SET,
                                    KEY_INFO_ENABLED,
                                    Oid,
                                    HostCmd_PENDING_ON_SET_OID,
                                    /// HostCmd_PENDING_ON_NONE,
                                    0,
                                    FALSE,
                                    NULL,
                                    NULL,
                                    NULL,
                                    InformationBuffer);
                    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
                    {
                        DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
                        return Status;
                    	}			
                    else
                    {
                        TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_ADD_KEY, (sizeof(NDIS_802_11_KEY)+pKey->KeyLength-1) );
                    }
                }

                return NDIS_STATUS_SUCCESS; // tt v5 firmware
            }

// tt ++ wep+tkip

            DBGPRINT(DBG_V9, (L"ADD_KEY , Encryption %x, Authentication %x\r\n", 
                     Adapter->EncryptionStatus, Adapter->AuthenticationMode ) );

            if ( (pKey->KeyLength == WEP_KEY_40_BIT_LEN || pKey->KeyLength == WEP_KEY_104_BIT_LEN) && 
                 ((Adapter->EncryptionStatus == Ndis802_11Encryption2Enabled ) ||
                  (Adapter->AuthenticationMode < Ndis802_11AuthModeWPA))
               )  
            {
            /*
                This block is for Cisco AP (with WEP+TKIP configuration). Currently, driver only supports
                40 bits and 104 bits of WEP.
            */
            
            MRVL_WEP_KEY WepKeyFormat;

            DBGPRINT(DBG_V9|DBG_CCX, (L"set wep, KeyLen=%d, KeyIdx=0x%x\r\n", pKey->KeyLength, pKey->KeyIndex ) );
            
            WepKeyFormat.KeyIndex = pKey->KeyIndex;
            WepKeyFormat.KeyLength = pKey->KeyLength;
            WepKeyFormat.Length = pKey->Length;

            NdisMoveMemory(WepKeyFormat.KeyMaterial,
                                   pKey->KeyMaterial,
                                   pKey->KeyLength);

            Adapter->NeedSetWepKey = TRUE;
            Status=PrepareAndSendCommand(
                          Adapter,
                          HostCmd_CMD_802_11_SET_WEP,
                          0,
                          HostCmd_OPTION_USE_INT,
                          OID_802_11_ADD_WEP,
                          HostCmd_PENDING_ON_NONE,
                          0,
                          FALSE,
                          NULL,
                          NULL,
                          NULL,
                          &WepKeyFormat);

                    //  copied from add_wep
                    NdisZeroMemory(&(Adapter->LastAddedWEPKey), sizeof(MRVL_WEP_KEY));
                    
                    if(pKey->KeyLength == 5) // Assume we support 40 ans 104 bit WEP
                        ulSize = sizeof(MRVL_WEP_KEY) - 11;
                    else
                        ulSize = sizeof(MRVL_WEP_KEY) - 3;    

                    NdisMoveMemory(&(Adapter->LastAddedWEPKey), &WepKeyFormat, ulSize);

            }
            else if ((Adapter->MediaConnectStatus == NdisMediaStateConnected) &&
                ( NdisEqualMemory(pKey->BSSID, Adapter->CurrentBSSID,
                                  sizeof(NDIS_802_11_MAC_ADDRESS))))
            {
                    if (Adapter->EncryptionStatus == Ndis802_11Encryption2KeyAbsent)
                        Adapter->EncryptionStatus = Ndis802_11Encryption2Enabled;
                    else if (Adapter->EncryptionStatus == Ndis802_11Encryption3KeyAbsent)
                        Adapter->EncryptionStatus = Ndis802_11Encryption3Enabled;




                    Status = PrepareAndSendCommand(
                                      Adapter,
                                    HostCmd_CMD_802_11_KEY_MATERIAL,
                                    HostCmd_ACT_SET,
                                    KEY_INFO_ENABLED,
                                    Oid,
                                    HostCmd_PENDING_ON_SET_OID,
                                    0,
                                    FALSE,
                                    NULL,
                                    NULL,
                                    NULL,
                                    InformationBuffer);
                    if (Status != NDIS_STATUS_FAILURE && Status != NDIS_STATUS_RESOURCES)
                    {
                        TT_CMDPARSE_SET_WAIT_COMPLETE_THEN_GO_OR_RETURN( Adapter, HostCmd_CMD_802_11_KEY_MATERIAL );
                        Status = NDIS_STATUS_SUCCESS;
                    }

                    OsNotify_RSN( Adapter );
            }
            else
            {
                // save the key for now
                Status = NDIS_STATUS_SUCCESS;
            }

      
            if ((Adapter->MediaConnectStatus == NdisMediaStateConnected) &&
            ((Adapter->EncryptionStatus == Ndis802_11Encryption2Enabled) ||
             (Adapter->EncryptionStatus == Ndis802_11Encryption3Enabled))
            )
            {
                if (Adapter->PSMode == Ndis802_11PowerModeMAX_PSP)
                    PSSleep(Adapter); 
            }
        }
        break;

		
  case OID_802_11_REMOVE_KEY:
        {
            PNDIS_802_11_REMOVE_KEY    pKey;
        DBGPRINT(DBG_OID | DBG_WPA,(L"SET - OID_802_11_REMOVE_KEY ****\r\n"));

            if ( InformationBufferLength < sizeof(NDIS_802_11_REMOVE_KEY))
            {
                *BytesNeeded = sizeof(NDIS_802_11_REMOVE_KEY);
                return NDIS_STATUS_INVALID_LENGTH;
            }
            pKey = (PNDIS_802_11_REMOVE_KEY)InformationBuffer;
            DBGPRINT(DBG_OID | DBG_WPA, (L"Key Index = %x, BSSID = 0x2x-0x2x-0x2x-0x2x-0x2x-0x2x",
                                         pKey->KeyIndex,
                                         pKey->BSSID[0],
                                         pKey->BSSID[1],
                                         pKey->BSSID[2],
                                         pKey->BSSID[3],
                                         pKey->BSSID[4],
                                         pKey->BSSID[5]));


            if ( pKey->KeyIndex & 0x80000000 )
            {
                // delete pairwise key
                // the driver does not support extra PWK, so just
                // delete group key 0
                NdisZeroMemory(&Adapter->KeyMap[0], sizeof(MRVL_WPA_KEY_SET));

                return NDIS_STATUS_SUCCESS;
            }

            {
                UCHAR   ucIdx, ucKeyIdx;
                UCHAR   ucBroadcastMAC[] = { 0xff, 0xff, 0xff, 0xff, 
                                             0xff, 0xff };

                if ( NdisEqualMemory(pKey->BSSID,
                                     ucBroadcastMAC,
                                     sizeof(NDIS_802_11_MAC_ADDRESS)))
                {
                    for ( ucIdx = 1; ucIdx < MRVL_NUM_WPA_KEYSET_SUPPORTED; ucIdx++ )
                    {
                        for (ucKeyIdx = 0; ucKeyIdx < MRVL_NUM_WPA_KEY_PER_SET; 
                             ucKeyIdx++ )
                        {
                            if ( Adapter->KeyMap[ucIdx].Key[ucKeyIdx].KeyIndex 
                                  == pKey->KeyIndex ) 
                            {
                                // remove this entry
                                NdisZeroMemory(&(Adapter->KeyMap[ucIdx].Key[ucKeyIdx]),
                                               sizeof(MRVL_WPA_KEY));


                                return NDIS_STATUS_SUCCESS;

                            }                            
                        }
                    }
                    // did not find the key
                    return NDIS_STATUS_FAILURE;
                }

                // not broadcast mac address, removing the matching entry
               for ( ucIdx = 1; ucIdx < MRVL_NUM_WPA_KEYSET_SUPPORTED; ucIdx++ )
                {
                    if ( NdisEqualMemory(pKey->BSSID,
                                         Adapter->KeyMap[ucIdx].BSSID,
                                         sizeof(NDIS_802_11_MAC_ADDRESS)))
                    {
                        
                        for (ucKeyIdx = 0; ucKeyIdx < MRVL_NUM_WPA_KEY_PER_SET; 
                             ucKeyIdx++ )
                        {
                            if ( Adapter->KeyMap[ucIdx].Key[ucKeyIdx].KeyIndex 
                                  == pKey->KeyIndex ) 
                            {
                                // remove this entry
                                NdisZeroMemory(&(Adapter->KeyMap[ucIdx].Key[ucKeyIdx]),
                                               sizeof(MRVL_WPA_KEY));

                                if ( Adapter->KeyMap[ucIdx].Key[ucKeyIdx].KeyLength <= 16 ) 
                                {
                                    // WEP key is equal to or less than 16 bytes long
                                    // TKIP key is always 32 bytes long
                                    KeyIndex = pKey->KeyIndex;

                                    Status=PrepareAndSendCommand(
                                          Adapter,
                                          HostCmd_CMD_802_11_SET_WEP,
                                          0,
                                          HostCmd_OPTION_USE_INT,
                                          OID_802_11_REMOVE_WEP,
                                          HostCmd_PENDING_ON_NONE,
                                          0,
                                          FALSE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &KeyIndex);

                                    NdisZeroMemory(&(Adapter->LastAddedWEPKey), sizeof(MRVL_WEP_KEY));
                                Adapter->LastAddedWEPKey.Length = 0xffff;
                                    Adapter->LastAddedWEPKey.KeyIndex = KeyIndex;
                                DBGPRINT(DBG_OID,(L"SET - Ready to removing key index = %d ****\r\n", KeyIndex));
                                }


                                return NDIS_STATUS_SUCCESS;
                            }                            
                        }
                    }
                }

                return NDIS_STATUS_FAILURE;
            }
        }
    break;

  case OID_802_11_ENCRYPTION_STATUS:

    DBGPRINT(DBG_V9|DBG_OID,
                (L"SET - OID_802_11_ENCRYPTION_STATUS - %d ****\r\n",
                *((PNDIS_802_11_ENCRYPTION_STATUS)InformationBuffer)));

    if ( InformationBufferLength < sizeof(NDIS_802_11_ENCRYPTION_STATUS) )
        {
            *BytesNeeded = sizeof(NDIS_802_11_ENCRYPTION_STATUS);
      return NDIS_STATUS_INVALID_LENGTH;
    }  

    Status = MrvDrvSetEncryptionStatus(
      Adapter,
      *((PNDIS_802_11_ENCRYPTION_STATUS)InformationBuffer),
            Oid,
          InformationBuffer,
          BytesRead,
          BytesNeeded);


  if ( Status == NDIS_STATUS_PENDING )
  {
    DBGPRINT(DBG_V9, (L"Status Pending wait for complete \r\n", Status) );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_ENCRYPTION_STATUS, InformationBufferLength );
  }
  else if ( Status != NDIS_STATUS_SUCCESS )
  {
    DBGPRINT(DBG_V9, (L"-v5- Fail to SET encryption status [Status=0x%x]\r\n", Status) );
  }

    break;

  case OID_802_11_ASSOCIATION_INFORMATION:
        {

            PNDIS_802_11_ASSOCIATION_INFORMATION pInfo,pAdapInfo;
            
            DBGPRINT(DBG_V9,(L"SET - OID_802_11_ASSOCIATION_INFORMATION ****\r\n"));
        
            if ( InformationBufferLength < 
                    sizeof(NDIS_802_11_ASSOCIATION_INFORMATION) )
            {
                *BytesNeeded = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);
          return NDIS_STATUS_INVALID_LENGTH;
            }

            pInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)InformationBuffer;

            pAdapInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)Adapter->AssocInfoBuffer;

            // request buffer is ahead of response buffer
            if ( pInfo->RequestIELength > pAdapInfo->RequestIELength )
            {
                //need to make a copy of the response buffer first
                UCHAR pTempBuffer[MRVDRV_ASSOCIATE_INFO_BUFFER_SIZE];

                NdisMoveMemory(pTempBuffer, 
                               ((PUCHAR)pAdapInfo) + pAdapInfo->OffsetResponseIEs,
                               pAdapInfo->ResponseIELength);

                NdisMoveMemory(((PUCHAR)pAdapInfo) + pAdapInfo->OffsetRequestIEs,
                               ((PUCHAR)pInfo) + pInfo->OffsetRequestIEs,
                               pInfo->RequestIELength);

                pAdapInfo->RequestIELength = pInfo->RequestIELength;

                // four byte align the start of the response buffer
                pAdapInfo->OffsetResponseIEs = ((pAdapInfo->RequestIELength + 3) >> 2) << 2;
                NdisMoveMemory(((PUCHAR)pAdapInfo) + pAdapInfo->OffsetResponseIEs,
                                pTempBuffer,
                                pAdapInfo->ResponseIELength);
            }
            else
            {
                ULONG   ulOriginalOffset;

                // copy the request buffer and move the response buffer
                NdisMoveMemory(((PUCHAR)pAdapInfo) + pAdapInfo->OffsetRequestIEs,
                               ((PUCHAR)pInfo) + pInfo->OffsetRequestIEs,
                               pInfo->RequestIELength);
                pAdapInfo->RequestIELength = pInfo->RequestIELength;

                ulOriginalOffset = pAdapInfo->OffsetResponseIEs;

                // four byte align the start of the response buffer
                pAdapInfo->OffsetResponseIEs = ((pAdapInfo->RequestIELength + 3) >> 2) << 2;
                NdisMoveMemory(((PUCHAR)pAdapInfo) + pAdapInfo->OffsetResponseIEs,
                                ((PUCHAR)pAdapInfo) + ulOriginalOffset,
                                pAdapInfo->ResponseIELength);
            }

            Status = NDIS_STATUS_SUCCESS;        
        }
    break;


    case OID_FSW_CCX_CONFIGURATION:
    {
        ULONG flag;
        flag = *((ULONG *)InformationBuffer);
        wlan_ccx_setFlags(flag);
        DBGPRINT(DBG_CCX ,(L"SET - OID_FSW_CCX_CONFIGURATION flag=0x%x\r\n", flag));
        break;
    }

    case OID_FSW_CCX_NETWORK_EAP:
    {
        ULONG state;
        state = *((ULONG *)InformationBuffer);
        wlan_ccx_setEAPState(state);
        DBGPRINT(DBG_CCX ,(L"SET - OID_FSW_CCX_NETWORK_EAP state=0x%x\r\n", state));
        break;
    }


    case OID_FSW_CCX_ROGUE_AP_DETECTED:
      {
        DBGPRINT(DBG_CCX ,(L"SET - OID_FSW_CCX_ROGUE_AP_DETECTED :"));

        wlan_ccx_setRogueAP(InformationBuffer);
        break;
      }
    case OID_FSW_CCX_REPORT_ROGUE_APS:
        DBGPRINT(DBG_CCX|DBG_HELP ,(L"SET - OID_FSW_CCX_REPORT_ROGUE_APS :\r\n"));
        wlan_ccx_sendRogueAPList(Adapter);
        break;
        
    case OID_FSW_CCX_AUTH_SUCCESS:
        DBGPRINT(DBG_CCX|DBG_HELP|DBG_ROAM ,(L"SET - OID_FSW_CCX_AUTH_SUCCESS : for fast roaming\r\n"));
        wlan_ccx_authSuccess(Adapter);

		if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
			DBGPRINT(DBG_ROAM|DBG_HELP,(TEXT(__FUNCTION__)TEXT(": Connected - \"%s\"\r\n"), Adapter->CurrentSSID.Ssid));
			wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_STABLE);
		}

        break;

    case OID_FSW_CCX_CCKM_REQUEST:
        DBGPRINT(DBG_CCX ,(L"SET - OID_FSW_CCX_CCKM_REQUEST :\r\n"));
        wlan_ccx_CCKMRequest((PFSW_CCX_CCKM_REQUEST) InformationBuffer);

        break;        

    case OID_FSW_CCX_CCKM_RESULT:
        DBGPRINT(DBG_CCX ,(L"SET - OID_FSW_CCX_CCKM_RESULT :"));

        break;



    case OID_MRVL_REGION_CODE:
    return NDIS_STATUS_NOT_SUPPORTED;

        break;

    case OID_MRVL_MAC_ADDRESS:
  {
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_MAC_ADDRESS, OID_MRVL_MAC_ADDRESS, 
      sizeof(OID_MRVL_DS_MAC_ADDRESS), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_MAC_ADDRESS, sizeof(OID_MRVL_DS_MAC_ADDRESS) );
      }
  

    case OID_MRVL_BBP_REG:
        {
            // set BBP reg access
            HexDump(DBG_OID,"OID_MRVL_BBP_REG(SET): ", (PUCHAR)InformationBuffer, InformationBufferLength);

    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_BBP_REG_ACCESS, OID_MRVL_BBP_REG, 
      sizeof(OID_MRVL_DS_BBP_REGISTER_ACCESS), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_BBP_REG, sizeof(OID_MRVL_DS_BBP_REGISTER_ACCESS) );


        }
        break;

    case OID_MRVL_MAC_REG:
        {
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_MAC_REG_ACCESS, OID_MRVL_MAC_REG, 
      sizeof(OID_MRVL_DS_MAC_REGISTER_ACCESS), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_MAC_REG, sizeof(OID_MRVL_DS_MAC_REGISTER_ACCESS) );


        }
        break;

    case OID_MRVL_RF_REG:
        {
    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_RF_REG_ACCESS, OID_MRVL_RF_REG, 
      sizeof(OID_MRVL_DS_RF_REGISTER_ACCESS), HostCmd_ACT_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_RF_REG, sizeof(OID_MRVL_DS_RF_REGISTER_ACCESS) );


        }
        break;

    case OID_MRVL_EEPROM_ACCESS:
        {
            POID_MRVL_DS_EEPROM_ACCESS pData;

            // request flash access
            if ( InformationBufferLength < sizeof(OID_MRVL_DS_EEPROM_ACCESS) )
            {
                *BytesNeeded = sizeof(OID_MRVL_DS_EEPROM_ACCESS);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pData = (POID_MRVL_DS_EEPROM_ACCESS)InformationBuffer;

            if ( (pData->usDataLength + sizeof(OID_MRVL_DS_EEPROM_ACCESS)) >
                 InformationBufferLength )
            {
                *BytesNeeded = sizeof(OID_MRVL_DS_EEPROM_ACCESS) + 
                                pData->usDataLength;

                return NDIS_STATUS_INVALID_LENGTH;
            }

            Status=PrepareAndSendCommand(
        Adapter,
        HostCmd_CMD_EEPROM_ACCESS,
        HostCmd_ACT_SET,
        HostCmd_OPTION_USE_INT,
        Oid,
        HostCmd_PENDING_ON_SET_OID,
        0,
        FALSE,
        0,
        0,
        BytesNeeded,
        InformationBuffer);

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
    {
         DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
      return Status;
    }	  
    else
    {
      TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_EEPROM_ACCESS, InformationBufferLength );
    }

        }
        break;

    case OID_MRVL_RF_CHANNEL:
        {
            ULONG                       ulNewChannel,i;
            BOOLEAN                     bValidChannel;
            POID_MRVL_DS_RF_CHANNEL     pChannelInfo;

            if ( InformationBufferLength < sizeof(OID_MRVL_DS_RF_CHANNEL) )
            {
                *BytesNeeded = sizeof(OID_MRVL_DS_RF_CHANNEL);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pChannelInfo  = (POID_MRVL_DS_RF_CHANNEL)InformationBuffer;
            ulNewChannel = pChannelInfo->ulChannelNumber;
            DBGPRINT(DBG_OID, (L"SET - OID_MRVL_RF_CHANNEL - %d ****\r\n",
                               ulNewChannel));

            bValidChannel = FALSE;
            // check for valid channel
            for ( i = 0; i < MRVDRV_MAX_CHANNEL_NUMBER; i++ )
            {
                if ( IEEERegionChannel[Adapter->RegionTableIndex][i] ==
                     ulNewChannel ) 
                {
                    bValidChannel = TRUE;
                    break;
                }
            }

            if ( ! bValidChannel )
            {
                DBGPRINT(DBG_OID | DBG_WARNING, 
                    (L"Invalid Channel parameter: %d\r\n", ulNewChannel));
                return NDIS_STATUS_FAILURE;
            }

            Adapter->Channel = ulNewChannel;    

    TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_RF_CHANNEL, OID_MRVL_RF_CHANNEL, 
      0, HostCmd_ACT_GEN_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
    TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_RF_CHANNEL, sizeof(OID_MRVL_DS_RF_CHANNEL) );


        }
        break;
     case OID_MRVL_EMI_CHANNEL:
        {
            ULONG                       ulNewChannel;
            BOOLEAN                     bValidChannel;
            POID_MRVL_DS_RF_CHANNEL     pChannelInfo;

            if ( InformationBufferLength < sizeof(OID_MRVL_DS_RF_CHANNEL) )
            {
                *BytesNeeded = sizeof(OID_MRVL_DS_RF_CHANNEL);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pChannelInfo  = (POID_MRVL_DS_RF_CHANNEL)InformationBuffer;
            ulNewChannel = pChannelInfo->ulChannelNumber;
            DBGPRINT(DBG_OID, (L"SET - OID_MRVL_EMI_CHANNEL - %d ****\r\n",
                               ulNewChannel));

            bValidChannel = FALSE;

         if((ulNewChannel>=1 )&&( ulNewChannel<= MRVDRV_MAX_CHANNEL_NUMBER))
            bValidChannel = TRUE;
         
            if ( ! bValidChannel )
            {
                DBGPRINT(DBG_OID | DBG_WARNING, 
                    (L"Invalid Channel parameter: %d\r\n", ulNewChannel));
                return NDIS_STATUS_FAILURE;
            }

            Adapter->Channel = ulNewChannel;    

            TT_CMDPARSE_SYNC_OID( HostCmd_CMD_802_11_RF_CHANNEL, OID_MRVL_EMI_CHANNEL, 
              0, HostCmd_ACT_GEN_SET, HostCmd_PENDING_ON_SET_OID, BytesRead );
            TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_EMI_CHANNEL, sizeof(OID_MRVL_DS_RF_CHANNEL) );

        }
        break;
    case OID_MRVL_TX_MODE:
        {
            POID_MRVL_DS_TX_MODE pMode;

            if ( InformationBufferLength < sizeof(OID_MRVL_DS_TX_MODE) )
            {
                *BytesNeeded = sizeof(OID_MRVL_DS_TX_MODE);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pMode = (POID_MRVL_DS_TX_MODE)InformationBuffer;

            DBGPRINT(DBG_OID, (L"SET - OID_MRVL_TX_MODE, Set to %d\r\n", 
                                pMode->ulMode));

            if ( pMode->ulMode >= TX_MODE_MAX )
            {
                DBGPRINT(DBG_OID | DBG_WARNING, 
                        (L"Attempt to set TX_MODE to invalid value!\r\n"));
                return NDIS_STATUS_INVALID_DATA;
            }

            Status=PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_TEST_TX_MODE,
                HostCmd_ACT_SET,
                HostCmd_OPTION_USE_INT,
                0,
                HostCmd_PENDING_ON_SET_OID,
                0,
                FALSE,
                NULL,
                0,
                0,
                        InformationBuffer
                );

      if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
      {
           DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
           return Status;
      	}	
      else
      {
        TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_TX_MODE, sizeof(OID_MRVL_DS_TX_MODE) );
      }
        }
        break;

    case OID_MRVL_RX_MODE :
        {
            POID_MRVL_DS_RX_MODE pMode;

            if ( InformationBufferLength < sizeof(OID_MRVL_DS_RX_MODE) )
            {
                *BytesNeeded = sizeof(OID_MRVL_DS_RX_MODE);
                return NDIS_STATUS_INVALID_LENGTH;
            }

            pMode = (POID_MRVL_DS_RX_MODE)InformationBuffer;

            DBGPRINT(DBG_OID, (L"SET - OID_MRVL_RX_MODE, Set to %d\r\n", 
                                pMode->ulMode));

            if ( pMode->ulMode >= RX_MODE_MAX )
            {
                DBGPRINT(DBG_OID | DBG_WARNING, 
                        (L"Attempt to set RX_MODE to invalid value!\r\n"));
                return NDIS_STATUS_INVALID_DATA;
            }

            Status=PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_TEST_RX_MODE,
                HostCmd_ACT_SET,
                HostCmd_OPTION_USE_INT,
                0,
                HostCmd_PENDING_ON_SET_OID,
                0,
                FALSE,
                NULL,
                0,
                0,
                        InformationBuffer
                );

      if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
      {
           DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
           return Status;
      	}	
      else
      {
        TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_RX_MODE, sizeof(OID_MRVL_DS_RX_MODE) );
      }

        }
        break;

    case OID_MRVL_LED_CONTROL:
        {
            if ( InformationBufferLength < sizeof(OID_MRVL_DS_LED_CONTROL) )
            {
                *BytesNeeded = sizeof(OID_MRVL_DS_LED_CONTROL);
                return NDIS_STATUS_INVALID_LENGTH;
            }
            
            DBGPRINT(DBG_OID, (L"SET - OID_MRVL_LED_CONTROL\r\n"));

            Status=PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_LED_CONTROL,
                HostCmd_ACT_SET,
                HostCmd_OPTION_USE_INT,
                0,
                HostCmd_PENDING_ON_SET_OID,
                0,
                FALSE,
                NULL,
                0,
                0,
                        InformationBuffer
                );

// tt ++ v5 firmware
      if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
      {
           DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
           return Status;
      	}		
      else
      {
        TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_LED_CONTROL, sizeof(OID_MRVL_DS_LED_CONTROL) );
      }
// tt --

        }
        break;

  case OID_MRVL_DEEP_SLEEP:
       //041307
       Status = ProcessOIDDeepSleep(Adapter,
                                    HostCmd_ACT_GEN_SET,
                                    InformationBufferLength,
                                    InformationBuffer,
                                    HostCmd_PENDING_ON_SET_OID);
       break; 

   case OID_MRVL_REMOVE_ADHOCAES:
  {
      NDIS_802_11_KEY key;
       DBGPRINT(DBG_ADHOC|DBG_HELP , (L"OID_MRVL_REMOVE_ADHOCAES\r\n"));   
       if (Adapter->MediaConnectStatus == NdisMediaStateConnected)
         return NDIS_STATUS_FAILURE;

      Adapter->AdhocAESEnabled = FALSE;

      NdisZeroMemory(&key,sizeof(NDIS_802_11_KEY)); 
      key.KeyLength = WPA_AES_KEY_LEN;
      key.KeyIndex = 0x40000000;
          
      Status = PrepareAndSendCommand(
                        Adapter,
                      HostCmd_CMD_802_11_KEY_MATERIAL,
                      HostCmd_ACT_SET,
                      !(KEY_INFO_ENABLED),
                      Oid,
                      HostCmd_PENDING_ON_SET_OID,
                      0,
                      FALSE,
                      NULL,
                      NULL,
                      NULL,
                      &key);

      if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
      {
              DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
              return Status;
      	}			
      else
      {

        TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_REMOVE_ADHOCAES, 0 );
      }
    } 
 
    break;
  
case OID_MRVL_SET_ADHOCAES: //stophere
    {
      PNDIS_802_11_KEY pkey;
      UCHAR buffer[sizeof(NDIS_802_11_KEY)+16];   
      PMRVL_ADHOC_AES_KEY pAdhocKey;      
      UCHAR Key_hex[16];
      
      
     pkey=(PNDIS_802_11_KEY)buffer;   
  
     Adapter->AdhocAESEnabled = TRUE;  
      
     DBGPRINT(DBG_ADHOC|DBG_HELP , (L"OID_MRVL_SET_ADHOCAES\r\n"));
     pAdhocKey = (PMRVL_ADHOC_AES_KEY) InformationBuffer;
    
     pkey->KeyLength= WPA_AES_KEY_LEN;
     pkey->KeyIndex= 0x40000000; 
    
     ascii2hex(Key_hex, pAdhocKey->KeyBody, sizeof(Key_hex));   
     NdisMoveMemory (pkey->KeyMaterial,Key_hex, pkey->KeyLength);   
     HexDump(DBG_ERROR, "SetAESKey: ", (PUCHAR)pkey->KeyMaterial, pkey->KeyLength);
     //051107 coverity change to mainstream 060407
     Status =  PrepareAndSendCommand(
                        Adapter,
                      HostCmd_CMD_802_11_KEY_MATERIAL,
                      HostCmd_ACT_SET,
                      KEY_INFO_ENABLED,
                      Oid,
                      HostCmd_PENDING_ON_SET_OID,
                      0,
                      FALSE,
                      NULL,
                      NULL,
                      NULL,
                      pkey);    

      if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
      {
            DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
            return Status;
      	} 		
      else
      {
        TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_MRVL_SET_ADHOCAES, 0);
      }
    }                    
    break;   

                    
  case OID_MRVL_HOST_SLEEP_CFG:
    {   
       Status = ProcessOIDHostSleepCfg(Adapter,
                                       InformationBuffer,
                                       InformationBufferLength,
                                       BytesNeeded);
       return Status;
    }
    break;



 case OID_MRVL_RADIO_CONTROL:
    {
            POID_DS_MRVL_RF_RADIO pRF_RADIO; 
            
            pRF_RADIO = (POID_DS_MRVL_RF_RADIO) InformationBuffer;

            if ( InformationBufferLength < sizeof(OID_DS_MRVL_RF_RADIO) )
            {
                *BytesNeeded = sizeof(OID_DS_MRVL_RF_RADIO);
                return NDIS_STATUS_INVALID_LENGTH;
            }


       
      Adapter->RadioOn=(BOOLEAN)(pRF_RADIO->RADIO&0x1);
      Adapter->Preamble=(pRF_RADIO->RADIO&0x6);
      Adapter->Preamble|=0x1;
            DBGPRINT(DBG_OID|DBG_HELP, (L"SET - OID_MRVL_RADIO_CONTROL pRF_RADIO->RADIO=0x%x\r\n,RadioOn=0x%x\r\n,Preamble=0x%x\r\n",pRF_RADIO->RADIO,Adapter->RadioOn,Adapter->Preamble));
       
        Status = PrepareAndSendCommand(
          Adapter,        
          HostCmd_CMD_802_11_RADIO_CONTROL,
          HostCmd_ACT_GEN_SET,
          HostCmd_OPTION_USE_INT,
          (NDIS_OID)0,        
          HostCmd_PENDING_ON_NONE,
          0,
          FALSE,
          NULL,
          NULL,
          NULL,
          NULL);    
         
         NdisMSleep(2000);

         if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
         {    
		 DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
         }
         else
         {
               DBGPRINT(DBG_OID, (L"Set OID_MRVL_RADIO_CONTROL Success\r\n"));
         }   
   }  
  break;


 case OID_MRVL_DISABLE_RADIO:
        {
          DBGPRINT(DBG_OID, (L"SET - OID_MRVL_DISABLE_RADIO\r\n"));
            
      Adapter->RadioOn = FALSE;
        Status = PrepareAndSendCommand(
          Adapter,        
          HostCmd_CMD_802_11_RADIO_CONTROL,
          HostCmd_ACT_GEN_SET,
          HostCmd_OPTION_USE_INT,
          (NDIS_OID)0,        
          HostCmd_PENDING_ON_NONE,
          0,
          FALSE,
          NULL,
          NULL,
          NULL,
          NULL);    
    
          NdisMSleep(2000);

           if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
           {    
               DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
            }
            else
              {
               DBGPRINT(DBG_OID, (L"Set OID_MRVL_DISABLE_RADIO Success\r\n"));
              }   
     
  break;
    }

  case OID_MRVL_ENABLE_RADIO:
          {
      DBGPRINT(DBG_OID, (L"SET - OID_MRVL_ENABLE_RADIO"));
       
         

    Adapter->RadioOn = TRUE;
    Status = PrepareAndSendCommand(
          Adapter,        
          HostCmd_CMD_802_11_RADIO_CONTROL,
          HostCmd_ACT_GEN_SET,
          HostCmd_OPTION_USE_INT,
          (NDIS_OID)0,     //OID_MRVL_ENABLE_RADIO,
          HostCmd_PENDING_ON_NONE,
          0,
          FALSE,
          NULL,
          NULL,
          NULL,
          NULL);    
    

    

         if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
         {     
              DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
            }
            else
              {
               DBGPRINT(DBG_OID, (L"Set OID_MRVL_ENABLE_RADIO Success\r\n"));
              }   
      }   
  break;
  
     case OID_MRVL_SLEEP_PARAMS:  
     {
       POID_MRVL_DS_SLEEP_PARAMS   Sp;
       Sp = (POID_MRVL_DS_SLEEP_PARAMS)InformationBuffer; 

       if( InformationBufferLength < sizeof(OID_MRVL_DS_SLEEP_PARAMS) )
       {
          *BytesNeeded = sizeof(OID_MRVL_DS_SLEEP_PARAMS);
          return NDIS_STATUS_INVALID_LENGTH;
       } 

       if( Sp->CalControl > 2 || Sp->ExternalSleepClk > 2 )
           return NDIS_STATUS_FAILURE;


       Status=PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_SLEEP_PARAMS,
                HostCmd_ACT_GEN_SET,
                HostCmd_OPTION_USE_INT,
                0,
                HostCmd_PENDING_ON_NONE,
                0,
                FALSE,
                0,
                0,
                BytesNeeded,
                InformationBuffer
                );

        
        if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
        {
             DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
           return Status;
        }	   
        else
           Status = NDIS_STATUS_SUCCESS;   
     }
     break;  
     case OID_MRVL_FW_WAKE_METHOD: 
     {   
          POID_MRVL_DS_FW_WAKE_METHOD  Wm;

          Wm = (POID_MRVL_DS_FW_WAKE_METHOD)InformationBuffer; 

          if( Wm->method > 2 ) 
              return NDIS_STATUS_FAILURE;    

          Status=PrepareAndSendCommand(
                                       Adapter,
                                       HostCmd_CMD_802_11_FW_WAKE_METHOD,
                                       HostCmd_ACT_GEN_SET,
                                       HostCmd_OPTION_USE_INT,
                                       0,
                                       HostCmd_PENDING_ON_NONE,
                                       0,
                                       FALSE,
                                       0,
                                       0,
                                       0,
                                       InformationBuffer
                                       );

          if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
          {
               DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
              return Status;  
          }
          DBGPRINT(DBG_OID|DBG_HELP,(L"Fw Wake Method : %d\r\n",Wm->method ));
          break;   
     }                                
     case OID_MRVL_INACTIVITY_TIMEOUT:
     {
        POID_MRVL_DS_INACTIVITY_TIMEOUT  psto;
        
        psto = (POID_MRVL_DS_INACTIVITY_TIMEOUT)InformationBuffer;
        if ( psto->time > 1000 ) 
             return NDIS_STATUS_FAILURE; 

        //RETAILMSG(1, (L"psto->time:%x\r\n",psto->time));   

        Status=PrepareAndSendCommand(
                                    Adapter,
                                    HostCmd_CMD_802_11_INACTIVITY_TIMEOUT,
                                    HostCmd_ACT_GEN_SET,
                                    HostCmd_OPTION_USE_INT,
                                    0,
                                    HostCmd_PENDING_ON_NONE,
                                    0,
                                    FALSE,
                                    0,
                                    0,
                                    0,
                                    InformationBuffer
                                    );

          if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
          {
               DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status))); 
               return Status;
          } 		  
          break;   
     }
     
       case OID_MRVL_WMM_STATE:
            if ( InformationBufferLength < sizeof(OID_MRVL_DS_WMM_STATE) )
           {
              *BytesNeeded = sizeof(OID_MRVL_DS_WMM_STATE);
        return NDIS_STATUS_INVALID_LENGTH;
           } 
           wlan_wmm_enable_ioctl(Adapter, InformationBuffer);
       
       break;
       case OID_MRVL_WMM_ACK_POLICY:
       
            if ( InformationBufferLength < sizeof(OID_MRVL_DS_WMM_ACK_POLICY) )
           {
              *BytesNeeded = sizeof(OID_MRVL_DS_WMM_ACK_POLICY);
        return NDIS_STATUS_INVALID_LENGTH;
           } 
     
           Status=PrepareAndSendCommand(
      Adapter,
      HostCmd_CMD_802_11_WMM_ACK_POLICY,
      HostCmd_ACT_SET,
      HostCmd_OPTION_USE_INT,
      OID_MRVL_WMM_ACK_POLICY,
      HostCmd_PENDING_ON_NONE,
      0,
      FALSE,
      NULL,
      NULL,
      NULL,
      InformationBuffer); 

       break;
      case OID_MRVL_WAKEUP_DEVICE:  
      { 

           if( ((POID_MRVL_DS_WAKEUP_DEVICE)InformationBuffer)->wake == 0 ) 
               break; 

           ProcessOIDWakeUpDevice(Adapter);
      }            
      break;
     
//-----------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------    
/* Extended Switch case for compiler limitation */  //dralee_20060214
    default:
    {
        switch( Oid )
        {
        //ahan, [2005-12-13] ++dralee_20060217
           case OID_MRVL_SUBSCRIBE_EVENT:
               {
                   POID_MRVL_DS_SUBSCRIBE_EVENT pOID_Events;
            
                   pOID_Events = (POID_MRVL_DS_SUBSCRIBE_EVENT)InformationBuffer;
         
                   DBGPRINT(DBG_OID|DBG_HELP,
                            (L"Set OID_MRVL_SUBSCRIBE_EVENT : Event %x : Type %x : Len %x :param1 %x : param2 %x \r\n",
                             pOID_Events->Events,
                             pOID_Events->Param.TLVData.Header.Type,
                             pOID_Events->Param.TLVData.Header.Len,
                             pOID_Events->Param.TLVData.data1,
                             pOID_Events->Param.TLVData.data2));
         
                   Adapter->EventRecord.EventMap |= (USHORT)pOID_Events->Events;
                   
                   if (pOID_Events->Events & LINK_LOSS)
                   {
                       Adapter->EventRecord.BeaconMissed = pOID_Events->Param.TLVData.data1;
                       Adapter->EventRecord.Reserved     = 0x0;
                   }
                   else if (pOID_Events->Events & MAX_FAIL)
                   {
                    Adapter->EventRecord.FailValue = pOID_Events->Param.TLVData.data1;
                    Adapter->EventRecord.FailFreq  = pOID_Events->Param.TLVData.data2;
                   }
                   else if (pOID_Events->Events & SNR_LOW)
                   {
                    Adapter->EventRecord.SNRLowValue = pOID_Events->Param.TLVData.data1;
                    Adapter->EventRecord.SNRLowFreq  = pOID_Events->Param.TLVData.data2;
                   }
                   else if (pOID_Events->Events & RSSI_LOW)
                   {
                    Adapter->EventRecord.RSSILowValue = pOID_Events->Param.TLVData.data1;
                    Adapter->EventRecord.RSSILowFreq  = pOID_Events->Param.TLVData.data2;
                   }
                   else if (pOID_Events->Events & SNR_HIGH)
                   {
                    Adapter->EventRecord.SNRHighValue = pOID_Events->Param.TLVData.data1;
                    Adapter->EventRecord.SNRHighFreq  = pOID_Events->Param.TLVData.data2;
                   }
                   else if (pOID_Events->Events & RSSI_HIGH)
                   {
                    Adapter->EventRecord.RSSIHighValue = pOID_Events->Param.TLVData.data1;
                    Adapter->EventRecord.RSSIHighFreq  = pOID_Events->Param.TLVData.data2;
                   }
                   else
                   {
                    return NDIS_STATUS_INVALID_DATA;
                   }
                   
                   Adapter->EventRecord.EventMap |= (USHORT)pOID_Events->Events;
         
                   Status = PrepareAndSendCommand(
                                   Adapter,        
                                   HostCmd_CMD_802_11_SUBSCRIBE_EVENT,
                                   HostCmd_ACT_GEN_SET,
                                   HostCmd_OPTION_USE_INT,
                                   (NDIS_OID)0,        
                                   HostCmd_PENDING_ON_NONE,
                                   0,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   0,
                                   NULL); 
         
                   if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
                   {
                       DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
                       return Status;
                   } 
         
         
               }
               break;

/*
   tt: WMM and PPS needs SLEEP_PERIOD
*/
               case OID_MRVL_SLEEP_PERIOD:     
               {
                    POID_MRVL_DS_WMM_SLEEP_PERIOD  sp;
                    
                    sp = (POID_MRVL_DS_WMM_SLEEP_PERIOD)InformationBuffer;
                    if ( sp->period > 60 || (sp->period < 10 && sp->period !=0 )  ) 
                         return NDIS_STATUS_FAILURE; 
            
                    DBGPRINT(DBG_OID|DBG_HELP, (L"sp->time:%x\r\n",sp->period));   
            
                    Status=PrepareAndSendCommand(
                                                Adapter,
                                                HostCmd_CMD_802_11_SLEEP_PERIOD,
                                                HostCmd_ACT_GEN_SET,
                                                HostCmd_OPTION_USE_INT,
                                                0,
                                                HostCmd_PENDING_ON_NONE,
                                                0,
                                                FALSE,
                                                0,
                                                0,
                                                0,
                                                InformationBuffer
                                                );
            
                      if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
                      {
                            DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));  
                            return Status;
                      }			  
                      else 
                      { 
                          //RETAILMSG(1,(L"SLEEP PERIOD SET STATUS:%x\r\n",Status));
                          Adapter->sleepperiod = sp->period; 
                      }
               }
               break;
                
               case OID_MRVL_ATIMWINDOW:     
               {
                    POID_MRVL_DS_ATIM_WINDOW  atim;
                    
                    atim = (POID_MRVL_DS_ATIM_WINDOW)InformationBuffer;
            
                    DBGPRINT(DBG_OID|DBG_HELP, (L"atim:%x\r\n",atim->atimwindow));   
            
                    Adapter->CurrentConfiguration.ATIMWindow =  (ULONG)atim->atimwindow;  
                    Status = NDIS_STATUS_SUCCESS;
               }
               break;

                                    
               case OID_MRVL_802_11D_ENABLE:
               {        
                 USHORT  enable;
                  
                 if( ((POID_MRVL_DS_802_11D_CFG)InformationBuffer)->enable ) 
                    enable = 1;
                 else      
                    enable = 0;
                 
                 Adapter->Enable80211D = ((POID_MRVL_DS_802_11D_CFG)InformationBuffer)->enable;

                 Status = PrepareAndSendCommand(
                          Adapter,
                          HostCmd_CMD_802_11_SNMP_MIB,
                          HostCmd_ACT_GEN_SET,
                          HostCmd_OPTION_USE_INT,
                          OID_802_11D_ENABLE,
                          HostCmd_PENDING_ON_NONE,
                          0,
                          FALSE,
                          NULL,
                          0,
                          0,
                          (PVOID)&enable);

                 if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
                 {
                        DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,Status)));
                        return Status;
                 } 				
                 else
                 {
                     DBGPRINT(DBG_OID|DBG_HELP,(L"802.11d set status:%x\r\n",Adapter->Enable80211D));
                 }

               }
               break;
                         
              case OID_MRVL_LOCAL_LISTEN_INTERVAL:
               {      
                   USHORT ivl; 
                       
                   ivl = *((USHORT *)InformationBuffer); 
                   if( ivl > 20 ) 
                     return NDIS_STATUS_FAILURE; 

                   Adapter->LocalListenInterval = ivl; 
                   Status = NDIS_STATUS_SUCCESS;
               }
               break;
                
              //dralee_20060601
              case OID_MRVL_IBSS_COALESCING_CFG:
               {    
                   USHORT ivl; 
                    
                   ivl = ((POID_MRVL_DS_IBSS_COALESCING_CFG)InformationBuffer)->Enable;
                   if( ivl & 0xfffe ) 
                      return NDIS_STATUS_FAILURE;
                    
                   Adapter->IbssCoalsecingEnable = ivl;
                   Status = NDIS_STATUS_SUCCESS;
               }
               break;
            case OID_MRVL_GET_TSF: {
                DBGPRINT(DBG_OID|DBG_CCX,(L"SET - OID_MRVL_GET_TSF\r\n")); 
                Status=PrepareAndSendCommand( 
                                            Adapter,
                                            HostCmd_CMD_802_11_GET_TSF,
                                            HostCmd_ACT_GEN_SET,
                                            HostCmd_OPTION_USE_INT,
                                            0,
                                            HostCmd_PENDING_ON_NONE,
                                            0,
                                            FALSE,
                                            0,
                                            0,
                                            0,
                                            InformationBuffer
                                            );
                }
                break;
            case OID_MRVL_WMM_ADDTS: {
                PUCHAR ptr;
                DBGPRINT(DBG_OID|DBG_CCX,(L"SET - OID_MRVL_WMM_ADDTS\r\n")); 
                ptr = (PUCHAR)InformationBuffer;
                
                Status=PrepareAndSendCommand(
                                            Adapter,
                                            HostCmd_CMD_802_11_WMM_ADDTS_REQ,
                                            HostCmd_ACT_GEN_SET,
                                            HostCmd_OPTION_USE_INT,
                                            0,
                                            HostCmd_PENDING_ON_NONE,
                                            0,
                                            FALSE,
                                            0,
                                            0,
                                            0,
                                            InformationBuffer
                                            );
                }
                break;
            case OID_MRVL_WMM_DELTS: {
                DBGPRINT(DBG_OID|DBG_CCX,(L"SET - OID_MRVL_WMM_DELTS\r\n")); 
                Status=PrepareAndSendCommand(
                                            Adapter,
                                            HostCmd_CMD_802_11_WMM_DELTS_REQ,
                                            HostCmd_ACT_GEN_SET,
                                            HostCmd_OPTION_USE_INT,
                                            0,
                                            HostCmd_PENDING_ON_NONE,
                                            0,
                                            FALSE,
                                            0,
                                            0,
                                            0,
                                            InformationBuffer
                                            );
                }
                break;
            case OID_MRVL_WMM_QUEUE_CONFIG: {
                PUCHAR ptr;
                DBGPRINT(DBG_OID|DBG_CCX,(L"SET - OID_MRVL_WMM_QUEUE_CONFIG\r\n")); 
                ptr = (PUCHAR)InformationBuffer;
                Status=PrepareAndSendCommand(
                                            Adapter,
                                            HostCmd_CMD_802_11_WMM_QUEUE_CONFIG,
                                            HostCmd_ACT_GEN_SET,
                                            HostCmd_OPTION_USE_INT,
                                            0,
                                            HostCmd_PENDING_ON_NONE,
                                            0,
                                            FALSE,
                                            0,
                                            0,
                                            0,
                                            InformationBuffer
                                            );
                }
                break;
                
            case OID_MRVL_WMM_QUEUE_STATS: {
                DBGPRINT(DBG_OID|DBG_CCX,(L"SET - OID_MRVL_WMM_QUEUE_STATS\r\n")); 
                Status=PrepareAndSendCommand(
                                            Adapter,
                                            HostCmd_CMD_802_11_WMM_QUEUE_STATS,
                                            HostCmd_ACT_GEN_SET,
                                            HostCmd_OPTION_USE_INT,
                                            0,
                                            HostCmd_PENDING_ON_NONE,
                                            0,
                                            FALSE,
                                            0,
                                            0,
                                            0,
                                            InformationBuffer
                                            );
                }
                break;
            //dralee_20060629
            case OID_MRVL_PROPRIETARY_PERIODIC_PS:
               {        
                 if( ((POID_MRVL_DS_PROPRIETARY_PERIODIC_PS)InformationBuffer)->enable ) 
                    Adapter->PPS_Enabled = 1;
                 else      
                    Adapter->PPS_Enabled = 0;

                 Status = NDIS_STATUS_SUCCESS;
               }
               break;
            case OID_MRVL_SCAN_EXT:
               {  
                 POID_MRVL_DS_SCAN_EXT scan_ext = (POID_MRVL_DS_SCAN_EXT)InformationBuffer; 
                 Adapter->SpecifyChannel = scan_ext->Channel;
                 Adapter->ChannelMaxDuration = scan_ext->MaxDuration;
                 Adapter->ChannelMinDuration = scan_ext->MinDuration;
           
                 if ( Adapter->MediaConnectStatus == NdisMediaStateConnected )
                 {           
                      // scan while mediaState connected
                      Adapter->bIsScanWhileConnect = TRUE;
                 }
  
                 Status = PrepareAndSendCommand(
                          Adapter,
                          HostCmd_CMD_802_11_SCAN,
                          0,
                          HostCmd_OPTION_USE_INT,
                          (NDIS_OID)0,
                          HostCmd_PENDING_ON_NONE,
                          0,
                          FALSE,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
               }  
               break;

               case OID_MRVL_DBG_LOG_EVENT: 
               { 
                   Adapter->LogEventCfg = *((USHORT *)InformationBuffer); 
                   DBGPRINT(DBG_OID|DBG_HELP, (L"DBG Log Evts Set:%x\r\n",(USHORT)Adapter->LogEventCfg));     
                   Status = NDIS_STATUS_SUCCESS;
               }  
               break;
                case OID_MRVL_TEMP_FIX_CLOSEWZC: 
               { 
                   Adapter->TCloseWZCFlag = *((USHORT *)InformationBuffer); 
                   DBGPRINT(DBG_OID|DBG_HELP, (L"TCloseWZCFlag:%x\r\n",(USHORT)Adapter->TCloseWZCFlag));     
                   Status = NDIS_STATUS_SUCCESS;
               }  
               break;
         case OID_MRVL_TX_CONTROL: 
               { 
                   Adapter->TX_Control_Value = *((ULONG *)InformationBuffer); 
                   DBGPRINT(DBG_OID|DBG_HELP, (L"TX_Control_Value:%x\r\n",(ULONG)Adapter->TX_Control_Value));     
                   Status = NDIS_STATUS_SUCCESS;
               }       
               break;       
               case OID_MRVL_HOSTCMD:
               {
                   PHostCmd_DS_GEN pCmdPtr;
                   CmdCtrlNode *pCmdNode;

                   DBGPRINT(DBG_OID|DBG_HELP, (L"OID_MRVL_HOSTCMD Set\r\n"));
                   HexDump(DBG_OID|DBG_HELP, "HOSTCMD : ", (UCHAR *)InformationBuffer, InformationBufferLength);

                   // Get next free command control node
                   pCmdNode = GetFreeCmdCtrlNode(Adapter);
                   if (!pCmdNode) {
                       return NDIS_STATUS_FAILURE;
                   } 

                   pCmdPtr = (PHostCmd_DS_GEN)pCmdNode->BufVirtualAddr;

                   /*
                    * Copy the whole command into the command buffer 
                    */
                   NdisMoveMemory(pCmdPtr,
                                  InformationBuffer,
                                  InformationBufferLength);

                      pCmdNode->ExpectedRetCode = pCmdPtr->Command | 0x8000;
                      pCmdNode->Next = NULL;
                   pCmdNode->PendingOID  = 0;
                   pCmdNode->PendingInfo = HostCmd_PENDING_ON_NONE;
                   pCmdNode->INTOption   = HostCmd_OPTION_USE_INT;
                   pCmdNode->InformationBuffer = InformationBuffer;
                   pCmdNode->CmdFlags |= CMD_F_HOSTCMD;

                   // Set sequnece number, command and INT option
                      Adapter->SeqNum++;
                   pCmdPtr->SeqNum = Adapter->SeqNum;
                   pCmdPtr->Result = 0;

                   /* Queue this host command */
                   EnterCriticalSection(&Adapter->CmdQueueExeSection);
                   InsertCmdToQueue (Adapter, pCmdNode); 
                   LeaveCriticalSection(&Adapter->CmdQueueExeSection);
                   GetCmdFromQueueToExecute(Adapter);
               }
               break;
               case OID_MRVL_RESET_CHIP: 
               {        
                   USHORT chipreset;
                   int cnt = 0;
                                 
                   chipreset = *((USHORT *)InformationBuffer); 
                   DBGPRINT(DBG_OID|DBG_HELP, (L"Reset Chip Set:%x\r\n",(USHORT)Adapter->ChipResetting));
                   
                   if( chipreset == 1 )
                   {   
                       Adapter->SoftIntStatus |= MRVL_SOFT_INT_ChipReset;
                       SetEvent(Adapter->hControllerInterruptEvent);
                   }
                   //022607
                   while(++cnt < 3)  
                   {         

                     Status = WaitOIDAccessRespond(Adapter); 
                         
                     if( Status != NDIS_STATUS_SUCCESS )
                         continue; 

                     if(Adapter->WaitEventType & MRVL_SOFT_INT_ChipReset)
                     {                               
                         DBGPRINT(DBG_OID|DBG_PS|DBG_WARNING,(L"Chip Reset FW Reloaded\r\n"));
                         Adapter->WaitEventType &= ~MRVL_SOFT_INT_ChipReset; 
                           
                         InitializeWirelessConfig(Adapter);
                   
                         Application_Event_Notify(Adapter,NOTIFY_FW_RELOADED);
                         //
                   Status = NDIS_STATUS_SUCCESS;
                         break;  
                     }                  
                     else 
                     {   
                         Status = NDIS_STATUS_FAILURE;
                         continue;
                     }
                   }
                   //Status = NDIS_STATUS_SUCCESS;
               }
               break;
                 
               case OID_MRVL_ADHOC_AWAKED_PERIOD:
               {
                   USHORT awakeperiod;
                                 
                   awakeperiod = *((USHORT *)InformationBuffer); 
                   DBGPRINT(DBG_OID|DBG_HELP, (L"Adhoc Awake Period Set:%x\r\n",awakeperiod));
                   
                   //if( ((awakeperiod & (~0x100)) < 32) || ((awakeperiod & (~0x100)) == 0xff) )
                   if( awakeperiod < 32 && awakeperiod >0 )
                   {        
                       Adapter->AdhocAwakePeriod = awakeperiod;
                       Status = NDIS_STATUS_SUCCESS;
                   }
                   else
                       Status = NDIS_STATUS_FAILURE;   
               }
               break;
               case OID_MRVL_HS_CANCEL:
               {
                   ULONG cancel;
                   
                   cancel = *((ULONG *)InformationBuffer); 
                   Status = NDIS_STATUS_SUCCESS;
                   if( cancel == 1 ) 
                   {                              
                      //011506
                      ProcessOIDHSCancel(Adapter);
                      /* 011506
                      if( Adapter->psState == PS_STATE_FULL_POWER &&
                          Adapter->PSMode  == Ndis802_11PowerModeCAM && 
                          Adapter->HostPowerState == HTWK_STATE_SLEEP )
                      {
                          DBGPRINT(DBG_OID|DBG_HOSTSLEEP|DBG_WARNING,(L"Wake up Device in FULL power mode\r\n"));
                          Adapter->HostPowerState = HTWK_STATE_FULL_POWER;
                          Adapter->bPSConfirm=FALSE;

                          ProcessOIDWakeUpDevice(Adapter); 
                          //set as first priority to cancel hostsleecfg
                          Deactivate_Host_Sleep_Cfg(Adapter, 1);
                      }
                      else if( Adapter->PSMode != Ndis802_11PowerModeCAM &&
                               Adapter->psState != PS_STATE_FULL_POWER &&
                               Adapter->HostPowerState == HTWK_STATE_SLEEP )
                      { 
                         //set as first priority to cancel hostsleecfg
                         Deactivate_Host_Sleep_Cfg(Adapter, 1);
                         ProcessOIDWakeUpDevice(Adapter);
                      }
                      else
                         DBGPRINT(DBG_OID|DBG_HOSTSLEEP|DBG_ERROR,(L"HS cancel fail\r\n")); 
                      */
                   }     
               }
               break;
               //022607
               case OID_MRVL_BCA_TIMESHARE:
                    Status = ProcessOIDBCATimeshare(Adapter,HostCmd_ACT_SET,InformationBuffer);
               break;
        case OID_MRVL_SSID_SCAN:
            Status = HandleSSIDScan(Adapter, InformationBuffer, InformationBufferLength,
                                    BytesRead, BytesNeeded);
            break;
            default:
                Status = NDIS_STATUS_NOT_SUPPORTED; 
                DBGPRINT(DBG_WARNING|DBG_CUSTOM, (L"Set: Got unknown OID=%s!!\r\n", OIDMSG(Oid,Status))); 
                break;
        } //end of extended switch 
    }  //end of default
    break;

    } // End of switch construct

    return Status;
}


NDIS_STATUS
ProcessOIDResetChip(PMRVDRV_ADAPTER Adapter)
{ 
   //NDIS_STATUS sdStatus;
      
   //Exit Sleep mode first. 
   if ( Adapter->PSMode != Ndis802_11PowerModeCAM )
   {     
      Adapter->PSMode_B = Adapter->PSMode;    
      Adapter->PSMode = Ndis802_11PowerModeCAM;        
      //PSWakeup(Adapter);   //031407 don't care this case with HW reset mothod
      //NdisMSleep(300000); 
   }
   
   Adapter->ChipResetting = 1;
   SDIODisconnectInterrupt(Adapter->hDevice);
   /*------------------------------------------*
    *     Add reset API here..                 *
    *------------------------------------------*/
   ResetCmdBuffer(Adapter);  

   {    
      SD_API_STATUS ifstatus;
      IF_FW_STATUS  fwStatus;
             
      DBGPRINT(DBG_OID|DBG_HELP,(L"OID handle ChipReset\r\n"));

      ifstatus = If_ReInitCard(Adapter);
      if( ifstatus != IF_SUCCESS ) 
      {
         DBGPRINT(DBG_ERROR,(L"SDBUS reinit card fail\r\n"));
         //SetEvent( Adapter->hOidQueryEvent ); 051207 060407
         Signal_Pending_OID(Adapter);
         return FALSE;
      }
      DBGPRINT(DBG_LOAD|DBG_OID|DBG_HELP, (L"SDBUS reinit card Success!\r\n"));

      if(If_ReInitialize(Adapter) != NDIS_STATUS_SUCCESS )
      {
           // Only adapter object itself has been created up to this point
            DBGPRINT(DBG_CMD| DBG_ERROR, (L"SDIO reset reinitialization fail\r\n"));
            //SetEvent( Adapter->hOidQueryEvent ); 051207 060407
            Signal_Pending_OID(Adapter);
            return FALSE;     
      }
      
      //031407
      //NdisMSleep(100000);
      //NdisMSleep(10000);

      fwStatus = If_FirmwareDownload(Adapter);
      if ( fwStatus != FW_STATUS_INITIALIZED  )
      {
           DBGPRINT(DBG_LOAD|DBG_OID|DBG_ERROR, (L"Reset:Failed to download the firmware\n"));
           return FALSE;
      }
  
      DBGPRINT(DBG_LOAD|DBG_OID|DBG_HELP, (L"Reset:Success to download the firmware\r\n")); 

      //NdisMSleep(IF_WAITING_FW_BOOTUP);
      //NdisMSleep(20000);
          
      Adapter->ChipResetting = 0; 
   }

   DBGPRINT(DBG_OID|DBG_HELP,(L"ProcessOIDResetChip Completed\r\n"));
   
   return NDIS_STATUS_SUCCESS;
}

 
NDIS_STATUS
ProcessOIDHostSleepCfg( PMRVDRV_ADAPTER Adapter,
                        IN PVOID InformationBuffer,
                        IN ULONG InformationBufferLength,
                        OUT PULONG BytesNeeded)
{       
     NDIS_STATUS Status;
     POID_MRVL_DS_HOST_SLEEP_CFG pOIDHost_Sleep;  
     USHORT prioritypass=0;

     DBGPRINT(DBG_HOSTSLEEP|DBG_HELP,(L"[Marvell]Begin: Set OID_MRVL_HOST_SLEEP_CFG\r\n"));   
     if ( InformationBufferLength < (sizeof(OID_MRVL_DS_HOST_SLEEP_CFG)- sizeof(OID_MRVL_DS_HOST_WAKEUP_FILTER)) )
     {
        *BytesNeeded = sizeof(OID_MRVL_DS_HOST_SLEEP_CFG); 
        return NDIS_STATUS_INVALID_LENGTH;
     }
    
     pOIDHost_Sleep = (POID_MRVL_DS_HOST_SLEEP_CFG) InformationBuffer;  
        

     if( pOIDHost_Sleep->ulCriteria == -1 )
     { 
         Adapter->bHostWakeCfgSet = SPCFG_DISABLE;
         DBGPRINT(DBG_HOSTSLEEP|DBG_HELP,(L"HSCFG cancel\r\n")); 
         ProcessOIDHSCancel(Adapter);
         return NDIS_STATUS_SUCCESS;
         //prioritypass = 1;
     } 
     else
     {       
         if( pOIDHost_Sleep->Filter.Header.Type == Host_Sleep_Filter_Type )
         {    
             if( pOIDHost_Sleep->Filter.Header.Len > (MAX_HOST_SLEEP_WAKEUP_FILETER_ENTRY * sizeof(WakeUpFilterEntry)) )
                 return NDIS_STATUS_INVALID_LENGTH;

             NdisMoveMemory( (UCHAR *)&Adapter->SleepCfg.Filter.Header.Type,
                             (UCHAR *)&pOIDHost_Sleep->Filter.Header.Type, 
                             (pOIDHost_Sleep->Filter.Header.Len + sizeof(MrvlIEtypesHeader_t)) );
         } 
         else
         {   
               Adapter->SleepCfg.Filter.Header.Type =0;
               Adapter->SleepCfg.Filter.Header.Len =0;
         }

         Adapter->bHostWakeCfgSet = SPCFG_ENABLE;
     } 

     Adapter->SleepCfg.ulCriteria = pOIDHost_Sleep->ulCriteria;
     Adapter->SleepCfg.ucGPIO     = pOIDHost_Sleep->ucGPIO;
     Adapter->SleepCfg.ucGap      = pOIDHost_Sleep->ucGap;

     DBGPRINT(DBG_OID|DBG_HOSTSLEEP|DBG_PS|DBG_HELP,(L"V8 OID set sleep cfg:%x,%x,%x\r\n",
                  Adapter->SleepCfg.ulCriteria,
                  Adapter->SleepCfg.Filter.Header.Type,
                  Adapter->SleepCfg.Filter.Header.Len)); 

                         
     Adapter->SleepCfgRestoreTick = 0;
         
     Status = PrepareAndSendCommand(
              Adapter,        
              HostCmd_CMD_802_11_HOST_SLEEP_CFG,
              HostCmd_ACT_GEN_SET,
              HostCmd_OPTION_USE_INT,
              (NDIS_OID)0,        
              HostCmd_PENDING_ON_NONE,
              prioritypass,     //HS cancel is priority pass command
              FALSE,
              NULL,
              NULL,
              BytesNeeded,
              (PVOID)&Adapter->SleepCfg);

     if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
     {
         DBGPRINT(DBG_HOSTSLEEP|DBG_ERROR, (L"[Marvell]Set OID_MRVL_HOST_SLEEP_CFG Fail\r\n"));
     }
       
     if( Adapter->psState == PS_STATE_FULL_POWER &&
         Adapter->PSMode  == Ndis802_11PowerModeCAM && 
         pOIDHost_Sleep->ulCriteria != -1)
     { 
         Status = PrepareAndSendCommand(
                  Adapter,        
                  HostCmd_CMD_802_11_HOST_SLEEP_ACTIVATE,
                  HostCmd_ACT_GEN_SET,
                  HostCmd_OPTION_USE_INT,
                  (NDIS_OID)0,        
                  HostCmd_PENDING_ON_NONE,
                  0,
                  FALSE,
                  NULL,
                  NULL,
                  0,
                  NULL);

         if( Status != NDIS_STATUS_SUCCESS )
         {
            DBGPRINT(DBG_HOSTSLEEP|DBG_HELP,(L"Activate Hostsleep fail...\r\n"));
            return Status;                                                             
         } 
        
         DBGPRINT(DBG_HOSTSLEEP|DBG_HELP,(L"Activate HostSleep Success\r\n"));
     }

     DBGPRINT(DBG_HOSTSLEEP|DBG_HELP,(L"[Marvell]Set OID_MRVL_HOST_SLEEP_CFG Success\r\n"));
     return Status;

}


VOID
ProcessOIDWakeUpDevice(PMRVDRV_ADAPTER Adapter)
{   
    DBGPRINT(DBG_OID|DBG_HOSTSLEEP|DBG_WARNING,(L"Wake up Device\r\n"));
    If_PowerUpDevice(Adapter);

    DBGPRINT( DBG_CUSTOM, (L"going to wait for PoweBit clear!\n" ) );

    If_WaitFWPowerUp(Adapter);
} 

//011506
VOID
ProcessOIDHSCancel(PMRVDRV_ADAPTER Adapter)
{  

   DBGPRINT(DBG_HOSTSLEEP|DBG_WARNING,(L"HS_Cancel...psstate,psmode,hostpwmode,%x,%x,%x \r\n",
            Adapter->psState,Adapter->PSMode,Adapter->HostPowerState)); 
   
   //011506
   Adapter->SleepCfg.ulCriteria = -1;
   Adapter->bHostWakeCfgSet = SPCFG_DISABLE;

   if( Adapter->psState == PS_STATE_FULL_POWER &&
       Adapter->PSMode  == Ndis802_11PowerModeCAM && 
       IsThisHostPowerState(Adapter, HTWK_STATE_SLEEP) )
   {
       DBGPRINT(DBG_OID|DBG_HOSTSLEEP|DBG_WARNING,(L"Wake up Device in FULL power mode\r\n"));
       SetHostPowerState( Adapter, HTWK_STATE_FULL_POWER );
       Adapter->bPSConfirm=FALSE;
                    
       //012207
       //Adapter->IsDeepSleep = FALSE;
       //Adapter->IsDeepSleepRequired= FALSE; 
//       Adapter->DSState = DS_STATE_NONE;

       ProcessOIDWakeUpDevice(Adapter); 
       //set as first priority to cancel hostsleecfg
       //Deactivate_Host_Sleep_Cfg(Adapter, 1);

   }
   else if( Adapter->PSMode != Ndis802_11PowerModeCAM &&
            Adapter->psState != PS_STATE_FULL_POWER )
   {  
      //set as first priority to cancel hostsleecfg
      Deactivate_Host_Sleep_Cfg(Adapter, 0);  //011506 
       
      if( IsThisHostPowerState(Adapter, HTWK_STATE_SLEEP) ) 
          ProcessOIDWakeUpDevice(Adapter);
   }
   else 
   {
      Deactivate_Host_Sleep_Cfg(Adapter, 0);
    }

}

//022607  
NDIS_STATUS
ProcessOIDBCATimeshare( PMRVDRV_ADAPTER Adapter,
                        ULONG action,
                        IN PVOID InformationBuffer)
{  
   NDIS_STATUS Status;

   POID_MRVL_DS_BCA_TIMESHARE  bca;
 
   bca = (POID_MRVL_DS_BCA_TIMESHARE)InformationBuffer;
   
   if(action == HostCmd_ACT_GET)
   { 
      bca->TrafficType = (USHORT)Adapter->BTTrafficType;
      bca->TimeShareInterval = Adapter->BTTimeShareInterval; 
      bca->BTTime = Adapter->BTTime; 
      return NDIS_STATUS_SUCCESS;
   }

   if( (bca->TrafficType !=0 && bca->TrafficType != 1) ||
       !(bca->TimeShareInterval >= 20 && bca->TimeShareInterval <= 60000) || 
       bca->BTTime > bca->TimeShareInterval )
   {
      DBGPRINT(DBG_OID|DBG_HELP,(L"Wrong BCA TimeShare configuration\r\n"));
      return NDIS_STATUS_FAILURE;
   } 

   Adapter->BTTrafficType = (ULONG)bca->TrafficType;
   Adapter->BTTimeShareInterval = bca->TimeShareInterval; 
   Adapter->BTTime = bca->BTTime;
   
   Status = PrepareAndSendCommand(
         Adapter,
         HostCmd_CMD_802_11_BCA_CONFIG_TIMESHARE,
         HostCmd_ACT_SET,
         HostCmd_OPTION_USE_INT,
         (NDIS_OID)0,
         HostCmd_PENDING_ON_SET_OID,
         0,
         FALSE,
         NULL,
         NULL,
         NULL,
         InformationBuffer);

   if( Status != NDIS_STATUS_SUCCESS )
   {
      DBGPRINT(DBG_OID|DBG_ERROR,(L"BCA Timeshare config. Fail...\r\n"));
      return Status;                                                             
   }  
     
   Status = WaitOIDAccessRespond(Adapter);
   if( Status != NDIS_STATUS_SUCCESS || Adapter->nOidCmdResult != HostCmd_RESULT_OK )
   {   
       DBGPRINT(DBG_OID|DBG_ERROR,(L"BCA Timeshare command Fail...\r\n"));
       Status = NDIS_STATUS_NOT_ACCEPTED; 
       return Status;
   }

   Adapter->BTTrafficType = (ULONG)bca->TrafficType;
   Adapter->BTTimeShareInterval = bca->TimeShareInterval; 
   Adapter->BTTime = bca->BTTime;

   DBGPRINT(DBG_OID|DBG_ERROR,(L"BCA Timeshare command success...\r\n"));    
   return Status;
}


        

NDIS_STATUS
WaitOIDAccessRespond(PMRVDRV_ADAPTER Adapter)
{             
    DWORD   dwWaitStatus;

    Adapter->OIDAccessBlocked = TRUE; //051207 060407
    
    dwWaitStatus = WaitForSingleObjectWithCancel( Adapter, Adapter->hOidQueryEvent, ASYNC_OID_QUERY_TIMEOUT );
    Adapter->OIDAccessBlocked = FALSE; //051207 060407

    if ( dwWaitStatus != WAIT_OBJECT_0 )
        return NDIS_STATUS_NOT_ACCEPTED;  

    return NDIS_STATUS_SUCCESS;
}

static NDIS_STATUS HandleSSIDScan(PMRVDRV_ADAPTER Adapter, 
                                        IN PVOID InformationBuffer, IN ULONG InformationBufferLength,
                                        OUT PULONG BytesRead, OUT PULONG BytesNeeded)
{
    NDIS_STATUS         Status;
    NDIS_802_11_SSID        *pActiveScanSSID;

    DBGPRINT(DBG_OID|DBG_HELP, (L"Enter HandleSSIDScan\r\n"));
    Adapter->bIsAssociationBlockedByScan = TRUE;
    Adapter->SetActiveScanSSID = TRUE;
    if ( Adapter->bIsScanInProgress ) {
        DBGPRINT(DBG_OID|DBG_ERROR, (L"HandleSSIDScan - In Progress Scan ,reject the OID\r\n"));
        Status = NDIS_STATUS_FAILURE;
        goto FuncFinal;
    }
    if( Adapter->bIsPendingReset == TRUE ) {
        Status = NDIS_STATUS_RESET_IN_PROGRESS;
        goto FuncFinal;
    }

    pActiveScanSSID = (PNDIS_802_11_SSID)InformationBuffer;
    if ((InformationBufferLength != sizeof(NDIS_802_11_SSID))||
        (pActiveScanSSID->SsidLength <= 0) ||
        ((pActiveScanSSID->SsidLength > 0) && (pActiveScanSSID->Ssid[0] < 0x20) )){
        DBGPRINT(DBG_OID|DBG_ERROR,(L"HandleSSIDScan - Input parameter invalid\r\n"));
        Status = NDIS_STATUS_INVALID_LENGTH;
        goto FuncFinal;
    }
    
    {
        UCHAR       msg[64];
        unsigned int i;
        NdisZeroMemory(msg, sizeof(msg));
        for (i=0 ; i<pActiveScanSSID->SsidLength ; i++) {
            msg[i*2] = pActiveScanSSID->Ssid[i];
        }
        DBGPRINT(DBG_OID|DBG_HELP, (L"Sending Scan with SSID(%d)=%s\n", pActiveScanSSID->SsidLength, msg));
    }

    NdisMoveMemory(&Adapter->ActiveScanSSID, pActiveScanSSID, sizeof(NDIS_802_11_SSID));
    Status = PrepareAndSendCommand(Adapter,             
                                    HostCmd_CMD_802_11_SCAN,
                                    0,
                                    HostCmd_OPTION_USE_INT,
                                    (NDIS_OID)0,
                                    HostCmd_PENDING_ON_NONE,
                                    0, 
                                    FALSE, 
                                    NULL, 
                                    NULL, 
                                    NULL, 
                                    pActiveScanSSID);
FuncFinal:
    return Status;
}

//032107
NDIS_STATUS
ProcessOIDDeepSleep(PMRVDRV_ADAPTER Adapter,
                    ULONG action,
                    ULONG InformationBufferLength,
                    PVOID InformationBuffer, 
                    USHORT pendingInfo) //041307
{ 

   POID_MRVL_DS_DEEP_SLEEP pDeep_Sleep; 
   USHORT prioritypass;
   NDIS_STATUS Status; 
   
   if(action == HostCmd_ACT_GEN_GET)
   { 
      if( IsThisDsState(Adapter, DS_STATE_SLEEP) )  
         *((PULONG)InformationBuffer) = (ULONG) 1;  
      else
         *((PULONG)InformationBuffer) = (ULONG) 0;

      return NDIS_STATUS_SUCCESS;
   }  
   else if ( action == 0xff )     //051407
   {  //return fail to autodeep sleep 
      if( !(Adapter->CurCmd == NULL && IsQEmpty(&Adapter->CmdPendQ)) )
          return NDIS_STATUS_FAILURE; 
          
      //don't do autodeepsleep in such state
      if( !IsThisHostPowerState(Adapter, HTWK_STATE_FULL_POWER) ||
          Adapter->psState == PS_STATE_SLEEP ||
          Adapter->CurPowerState != NdisDeviceStateD0 )
          return NDIS_STATUS_FAILURE;
   }

   pDeep_Sleep = (POID_MRVL_DS_DEEP_SLEEP) InformationBuffer;

   if ( InformationBufferLength < sizeof(OID_MRVL_DS_DEEP_SLEEP) )
         return NDIS_STATUS_INVALID_LENGTH;
     
    if (pDeep_Sleep->ulEnterDeepSleep) //enter Deep Sleep 
    {  //012207 : 060407
        if( !IsThisDsState(Adapter, DS_STATE_NONE) )
        {
           DBGPRINT(DBG_DEEPSLEEP | DBG_WARNING , (L"Already in deep sleep mode\r\n"));
           return NDIS_STATUS_SUCCESS;
        }
      
         if(((Adapter->PSMode != Ndis802_11PowerModeCAM) &&
            (Adapter->psState  != PS_STATE_FULL_POWER) ))
         {
            DBGPRINT(DBG_DEEPSLEEP|DBG_WARNING, (L"[Marvell]Error  OID_MRVL_DEEP_SLEEP: Cannot process Deep Sleep command in IEEE PS mode\r\n",
                                                  Adapter->bIsScanInProgress,Adapter->PSMode,Adapter->psState ));  
            return  NDIS_STATUS_FAILURE;    
         } 
         //031607
         if( Adapter->bIsScanInProgress )
         {            
            ULONG cnt = 0;
            //051407 don't do autoDS
            if ( action == 0xff )
               return NDIS_STATUS_FAILURE;
         }  


        Adapter->bBgDeepSleep=1;
        if ( Adapter->MediaConnectStatus == NdisMediaStateConnected )
        {
          USHORT usCommand;

          //memorize the previous SSID and BSSID
          NdisMoveMemory( &(Adapter->PreviousSSID), 
                         &(Adapter->CurrentSSID), 
                         sizeof(NDIS_802_11_SSID));
            
          NdisMoveMemory( Adapter->PreviousBSSID,
                         Adapter->CurrentBSSID, 
                         MRVDRV_ETH_ADDR_LEN);

          if ( Adapter->InfrastructureMode == Ndis802_11Infrastructure )
          {
              usCommand = HostCmd_CMD_802_11_DEAUTHENTICATE;
          }
          else
          {
              usCommand = HostCmd_CMD_802_11_AD_HOC_STOP;
          }

          // completely clean up
          Status=PrepareAndSendCommand(
                Adapter,
                usCommand,
                0,
                HostCmd_OPTION_USE_INT,
                (NDIS_OID)0,
                HostCmd_PENDING_ON_NONE,
                0,
                FALSE,
                NULL,
                NULL,
                NULL,
                NULL); 

          if( Status != NDIS_STATUS_SUCCESS )
          { 
               DBGPRINT(DBG_ERROR,(L"Fail to stop connection before DeepSleep\r\n"));
               return NDIS_STATUS_FAILURE;  
          }
        }
     
        if( IsThisHostPowerState(Adapter, HTWK_STATE_SLEEP) &&
            Adapter->psState == PS_STATE_FULL_POWER ) 
            prioritypass = 1;
        else
            prioritypass = 0;

        Status = PrepareAndSendCommand(
                                     Adapter,        
                                     HostCmd_CMD_802_11_DEEP_SLEEP,
                                     HostCmd_ACT_GEN_SET,
                                     HostCmd_OPTION_USE_INT,
                                     (NDIS_OID)0,        
                                     //HostCmd_PENDING_ON_NONE, //041307
                                     pendingInfo,
                                     prioritypass,
                                     FALSE,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);    

         Adapter->bBgDeepSleep=0;
         if (Status != NDIS_STATUS_SUCCESS )
         { 
             DBGPRINT(DBG_ERROR , (L"Set OID_MRVL_DEEP_SLEEP Fail\r\n"));
             return Status;
         }
          

         //041307
         if( pendingInfo == HostCmd_PENDING_ON_SET_OID )
         { 
             Status = WaitOIDAccessRespond(Adapter);
             DBGPRINT(DBG_DEEPSLEEP|DBG_HELP , (L"Set OID_MRVL_DEEP_SLEEP Success\r\n")); 
             NdisMSleep(20000);
             return Status;
         } 
    }
    else  //exit Deep Sleep
    {                     
                   
      if( IsThisDsState(Adapter, DS_STATE_NONE) ) 
      {
           DBGPRINT(DBG_DEEPSLEEP | DBG_ALLEN , (L"Already out of deep sleep mode\r\n"));
           return NDIS_STATUS_SUCCESS;
      }
      
      Status = If_PowerUpDevice(Adapter);

      if( Status != NDIS_STATUS_SUCCESS )
          return Status;                             
      //081007
      If_WaitFWPowerUp(Adapter);
    } 

    return NDIS_STATUS_SUCCESS; 

} 



///new-assoc ++
static NDIS_STATUS ProcessOIDMrvlAssoc(PMRVDRV_ADAPTER Adapter, 
                                        IN PVOID InformationBuffer, IN ULONG InformationBufferLength,
                                        OUT PULONG BytesRead, OUT PULONG BytesNeeded,
                                        BOOLEAN needPending)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNDIS_802_11_SSID pRequestSSID;

    DBGPRINT(DBG_ASSO|DBG_HELP, (L"Enter ProcessOIDMrvlAssoc\n"));
    
    if ( IsInMicErrorPeriod( Adapter ) )
    {
        DBGPRINT(DBG_OID|DBG_CCX, (L"In MIC error period, reject the OID\r\n"));
        Status = NDIS_STATUS_FAILURE;
        goto FuncFinal;
    }
    if ( Adapter->bIsScanInProgress )
    {
            DBGPRINT(DBG_OID|DBG_CCX, (L"In Progress Scan ,reject the OID\r\n"));
            Status = NDIS_STATUS_FAILURE;
        goto FuncFinal;
    }

    if ( Adapter->bIsAssociateInProgress )
    {
        DBGPRINT(DBG_OID|DBG_CCX, (L"Already have association pending, reject OID\r\n"));
        Status = NDIS_STATUS_FAILURE;
        goto FuncFinal;
    }

    pRequestSSID = (PNDIS_802_11_SSID) InformationBuffer;

    if( pRequestSSID->SsidLength > 0 ) 
    {
        UINT i;
        DBGPRINT(DBG_V9,(L"ProcessOIDMrvlAssoc: len %d new SSID = ", pRequestSSID->SsidLength));
        if(pRequestSSID->Ssid[0] >= 0x20)
        {
            for(i=0; i<pRequestSSID->SsidLength; i++)
                DBGPRINT(DBG_V9,(L"%c", pRequestSSID->Ssid[i]))
            DBGPRINT(DBG_V9,(L"\n"));
        }   
    }
    else 
    {
            DBGPRINT(DBG_OID,(L"ProcessOIDMrvlAssoc, new SSID = NULL \r\n"));
    }

    if( Adapter->bIsPendingReset == TRUE ) {
        Status = NDIS_STATUS_RESET_IN_PROGRESS;
        goto FuncFinal;
    }

    // Code to weed out invalid SSID from the OS
    if ( (pRequestSSID->SsidLength > 0) && (pRequestSSID->Ssid[0] < 0x20)  )
    {
        DBGPRINT(DBG_OID|DBG_HELP|DBG_CUSTOM,(L"Detected invalid SSID, clean up all association \r\n"));  

        
        if(Adapter->TCloseWZCFlag==WZC_Default ||Adapter->TCloseWZCFlag==WZC_Ignore_Send_EAPOL_START )
        {
            if ( Adapter->MediaConnectStatus == NdisMediaStateConnected )
            {
                    DBGPRINT(DBG_OID|DBG_WARNING,(L"ProcessOIDMrvlAssoc:deauth before connection\r\n"));
                   Status = PrepareAndSendCommand(
                                  Adapter,
                                  ((Adapter->InfrastructureMode == Ndis802_11IBSS)?HostCmd_CMD_802_11_AD_HOC_STOP:HostCmd_CMD_802_11_DEAUTHENTICATE),
                                  0,
                                  HostCmd_OPTION_USE_INT,
                                  (NDIS_OID)0,
                                  //HostCmd_PENDING_ON_SET_OID, //051207:060407
                                  HostCmd_PENDING_ON_NONE,
                                  0,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);
              //RETAILMSG(1, (L"Disconnected from OID: OID_802_11_SSID\n"));
                  ResetDisconnectStatus(Adapter);
            }
            NdisZeroMemory(&Adapter->CurrentSSID, sizeof(NDIS_802_11_SSID));

            if (Adapter->bIsLastOIDSetEncryptionStatus)
            {
                // This was a removal from GUI's preferred list
                Adapter->bIsLastOIDSetEncryptionStatus = FALSE;
                NdisZeroMemory( Adapter->PreviousBSSID, 
                             MRVDRV_ETH_ADDR_LEN);
                             NdisMoveMemory( &Adapter->PreviousSSID,
                             &Adapter->NullSSID,
                             sizeof(NDIS_802_11_SSID));
            }
        }

        DBGPRINT(DBG_CUSTOM, (L"[MRVL] >>> return status_success for an invalid SSID\r\n") );
        Status = NDIS_STATUS_SUCCESS;
        goto FuncFinal;
    }

    if ( Adapter->MediaConnectStatus == NdisMediaStateConnected )
    {   
        if ( !NdisEqualMemory(&Adapter->CurrentSSID, pRequestSSID, sizeof(NDIS_802_11_SSID)) )
        {  
            //Associate to another SSID
            DBGPRINT(DBG_OID|DBG_WARNING,(L"ProcessOIDMrvlAssoc:Try another SSID\r\n"));
            ///RETAILMSG(1, (L"Set SSID:Try another SSID\n"));
            ResetDisconnectStatus(Adapter); 
        }
        else
        { 
            //022607
            //Associate to the same SSID with different BSSID
            DBGPRINT(DBG_OID|DBG_WARNING,(L"ProcessOIDMrvlAssoc:Try the same SSID\r\n"));
            if( ( (*((USHORT *)&Adapter->ReqBSSID[0])) | (*((USHORT *)&Adapter->ReqBSSID[2])) |(*((USHORT *)&Adapter->ReqBSSID[4])) ) != 0  && 
              (!NdisEqualMemory(Adapter->CurrentBSSID, Adapter->ReqBSSID, MRVDRV_ETH_ADDR_LEN)) )
            {  
                ResetDisconnectStatus(Adapter);
                DBGPRINT(DBG_OID|DBG_ASSO|DBG_WARNING,(L"ProcessOIDMrvlAssocD: with different BSSID\r\n"));          
            }
            else {
                Status = NDIS_STATUS_SUCCESS;
                goto FuncFinal;
            }
        }
    }

    DBGPRINT(DBG_CCX_V4,(L"Adapter->RoamAccount =0 \r\n"));

    Adapter->bHasTspec = FALSE;
    Adapter->RoamAccount = 0;
    Adapter->RoamDelay = 0;

    {//internal block
    BOOLEAN bScanThenAssociate = FALSE;
    UCHAR index;
      
    // clear out previous SSID so check for hang does
    // not attempt to reassociate
    NdisMoveMemory( &Adapter->PreviousSSID,
                  &Adapter->NullSSID,
                  sizeof(NDIS_802_11_SSID));
          
    if ((Adapter->InfrastructureMode == Ndis802_11Infrastructure) &&
      (pRequestSSID->SsidLength != 0))
    {
        //040307 Don't enable it. it will cause problem in handling pending on set OID    
        //Adapter->bRetryAssociate = TRUE;
        Adapter->bRetryAssociate = FALSE;

        index = FindSSIDInPSList(Adapter, pRequestSSID);
        if(index != 0xFF) 
        {         
            ULONG AssoRetry=0;

            INT     apid = FindAPBySSID(Adapter, InformationBuffer);
             
            //040207 add for Retry asso. on fail
            while (AssoRetry <= Adapter->AssoRetryTimes )
            {
              if (apid != -1)
              {
                  Status = PrepareAndSendCommand(Adapter,
                                                HostCmd_CMD_802_11_ASSOCIATE_EXT,
                                                0,
                                                HostCmd_OPTION_USE_INT,
                                                OID_802_11_SSID, 
                                                ((needPending == TRUE)?HostCmd_PENDING_ON_SET_OID:HostCmd_PENDING_ON_NONE),
                                                0, 
                                                FALSE,
                                                NULL, 
                                                BytesRead, 
                                                BytesNeeded, 
                                                (PVOID)&apid);
              }
         
              DBGPRINT(DBG_CCX|DBG_OID|DBG_HELP, (L"OID_802_11_SSID assoc : %d\r\n", Status));
         
              if (Status != NDIS_STATUS_SUCCESS)
              { 
                  //bScanThenAssociate = TRUE;
                  goto FuncFinal;
              }
              else
              {     
                  if (needPending == TRUE)
                  {   
                      DBGPRINT(DBG_HELP|DBG_OID, (L"Wiat OID_802_11_SSID complete  \r\n"));
                      //040207
                      //TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( Adapter, OID_802_11_SSID, InformationBufferLength );
                      Status = WaitOIDAccessRespond(Adapter); 
                      if( Status == NDIS_STATUS_SUCCESS )
                      {
                         if( Adapter->nOidCmdResult == HostCmd_RESULT_OK && 
                             *((PUSHORT)Adapter->OidCmdRespBuf) == 0 ) //asso. status == 0
                         { 
                             *BytesRead = InformationBufferLength;   
                             DBGPRINT(DBG_ERROR,(L"Association success....\r\n")); 
                             return NDIS_STATUS_SUCCESS;
                         }
                         else
                         {    
                            DBGPRINT(DBG_ERROR,(L"Association fail, ReTry it....\r\n"));
                            ++AssoRetry;
                            NdisMSleep(50000);
                            continue;            
                         }
                      } 
                      else 
                         break; 
                  } 
                  else
                  {
                      DBGPRINT(DBG_HELP, (L"NOT Wiat OID_802_11_SSID complete  \r\n"));
                      break;
                  }
                  Adapter->bIsSystemConnectNow = TRUE;
              }
           } //while (AssoRetry <= Adapter->AssoRetryTimes )  
            ///bug#18332 ++
            if (AssoRetry > Adapter->AssoRetryTimes) {
                DBGPRINT(DBG_CCX, (L"OID_802_11_SSID: Too many retry. Scan AP now\r\n"));
                bScanThenAssociate = TRUE;
            }
            ///bug#18332 ++
        }
        else
        {
            DBGPRINT(DBG_CCX|DBG_HELP, (L"OID_802_11_SSID: can not find in list\r\n"));
            bScanThenAssociate = TRUE;
        }
    }
    else if (Adapter->InfrastructureMode == Ndis802_11IBSS) //adhoc mode
    {
        // always scan for adhoc mode
        bScanThenAssociate = TRUE;
    }

    if ( bScanThenAssociate )
    {
        // If the SSID is an empty string and the NIC is not associated
        // with any AP, just associate with any SSID
        if (pRequestSSID->SsidLength == 0)
            Adapter->bIsConnectToAny = TRUE;
        
        // AP is not found, try to perform active scan
        Adapter->bIsAssociationBlockedByScan = TRUE;
// MSFT         
//            Adapter->m_NumScanInAssociationBlocked = 0;

        // store the SSID info temporarily
        NdisMoveMemory( &(Adapter->AttemptedSSIDBeforeScan), 
                            InformationBuffer, 
                            sizeof(NDIS_802_11_SSID));

        Adapter->SetActiveScanSSID = TRUE;

        DBGPRINT(DBG_V9|DBG_OID|DBG_HELP,(L"Adhoc mode set SSID \n"));

        NdisMoveMemory(  &(Adapter->ActiveScanSSID), 
                              (InformationBuffer), 
                              sizeof(NDIS_802_11_SSID)); 
        
        Adapter->bIsSystemConnectNow = TRUE;


            Status = PrepareAndSendCommand(
                                    Adapter,             
                                    HostCmd_CMD_802_11_SCAN,
                                    0,
                                    HostCmd_OPTION_USE_INT,
                                    (NDIS_OID)0,
                                    HostCmd_PENDING_ON_NONE,
                                    0, 
                                    FALSE, 
                                    NULL, 
                                    NULL, 
                                    NULL, 
                                    &(Adapter->ActiveScanSSID));

       
            *BytesRead = sizeof(NDIS_802_11_SSID);

        Status = NDIS_STATUS_SUCCESS;

        goto FuncFinal;
    } // end if bScanThenAssociate 
    } //internal block
FuncFinal:
    DBGPRINT(DBG_ASSO|DBG_HELP, (L"Exit ProcessOIDMrvlAssoc\n"));
    return Status;
}

///new-assoc --



NDIS_STATUS
ProcessOIDRateAdaptRateSet(PMRVDRV_ADAPTER Adapter,
                           ULONG action,
                           ULONG InformationBufferLength,
                           PULONG BytesWritten,
                           PVOID InformationBuffer)
{             
   NDIS_STATUS Status;

   if( InformationBufferLength < sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET) )
       return NDIS_STATUS_FAILURE; 

   if(action == HostCmd_ACT_GET)
   {       
       // completely clean up
       Status=PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                HostCmd_ACT_GEN_GET,
                HostCmd_OPTION_USE_INT,
                (NDIS_OID)0,
                HostCmd_PENDING_ON_GET_OID,
                0,
                FALSE,
                BytesWritten,
                NULL,
                NULL,
                InformationBuffer); 

       if( Status != NDIS_STATUS_SUCCESS )
       { 
            DBGPRINT(DBG_ERROR,(L"Fail get Rate Adaption RateSet\r\n"));
            return NDIS_STATUS_FAILURE;  
       } 

       Status = WaitOIDAccessRespond(Adapter);
       if( Status != NDIS_STATUS_SUCCESS || Adapter->nOidCmdResult != HostCmd_RESULT_OK )
       {   
          DBGPRINT(DBG_OID|DBG_ERROR,(L"Get RateAdapt RateSet command Fail...\r\n"));
          Status = NDIS_STATUS_NOT_ACCEPTED; 
          return Status;
       }

       NdisMoveMemory((PUCHAR)InformationBuffer,
                      (PUCHAR)Adapter->OidCmdRespBuf,
                      sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET)); 

       *BytesWritten = sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET);

       return NDIS_STATUS_SUCCESS;
   } 

   
   Status=PrepareAndSendCommand(
            Adapter,
            HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
            HostCmd_ACT_GEN_SET,
            HostCmd_OPTION_USE_INT,
            (NDIS_OID)0,
            HostCmd_PENDING_ON_SET_OID,
            0,
            FALSE,
            NULL,
            NULL,
            NULL,
            InformationBuffer);

    

   if( Status != NDIS_STATUS_SUCCESS )
   { 
       DBGPRINT(DBG_ERROR,(L"Fail Set Rate Adaption RateSet\r\n"));
       return NDIS_STATUS_FAILURE;  
   } 

   Status = WaitOIDAccessRespond(Adapter);
   if( Status != NDIS_STATUS_SUCCESS || Adapter->nOidCmdResult != HostCmd_RESULT_OK )
   {   
       DBGPRINT(DBG_OID|DBG_ERROR,(L"Set RateAdapt RateSet command Fail...\r\n"));
       Status = NDIS_STATUS_NOT_ACCEPTED; 
       return Status;
   }

   return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
ProcessOIDDesiredRates(PMRVDRV_ADAPTER Adapter,
                       ULONG action,
                       ULONG InformationBufferLength,
                       PULONG BytesWritten,
                       PVOID InformationBuffer)
{       
   OID_MRVL_DS_RATE_ADAPT_RATESET RA;
   ULONG len;
   USHORT BitMap;
   UCHAR *rate, didx, value; 
   char   idx;
   NDIS_STATUS  Status;

   if( InformationBufferLength < sizeof( NDIS_802_11_RATES) )
   {
      DBGPRINT( DBG_OID |DBG_WARNING, (L"Return NDIS_STATUS_INVALID_LENGTH\r\n"));
      return NDIS_STATUS_INVALID_LENGTH;
   }

   if( action == HostCmd_ACT_GET )
   { 
      Status= ProcessOIDRateAdaptRateSet(Adapter,
                                       HostCmd_ACT_GET,
                                       sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET),
                                       &len,
                                       (PVOID)&RA);
      if( Status != NDIS_STATUS_SUCCESS )
          return Status; 
                           
      NdisZeroMemory((PVOID)InformationBuffer, sizeof(NDIS_802_11_RATES));

      BitMap = RA.Bitmap;
      rate = (PUCHAR)InformationBuffer;

      didx =0;
      for(idx=12; idx>=0; idx--)
      {
         if( BitMap & (1L<<idx) )
         {   
            if( (value = ConvertFWIndexToNDISRate(idx)) != 0 )
                rate[didx++] = value; 
         }
         if( didx >= sizeof( NDIS_802_11_RATES) )
             break;
      }

      DBGPRINT(DBG_OID|DBG_HELP,(L"Get Desired rate:%x, %x, %x\r\n",
               BitMap, *((PULONG)&rate[0]),*((PULONG)&rate[4]))); 

      *BytesWritten = sizeof(NDIS_802_11_RATES);
    
      return NDIS_STATUS_SUCCESS;
   }  

   RETAILMSG(1,(L"sizeof(NDIS_802_11_RATES)=%x\r\n",sizeof(NDIS_802_11_RATES))); 
   rate = (PUCHAR)InformationBuffer;

   DBGPRINT(DBG_OID|DBG_HELP,(L"%x,%x,%x,%x,%x,%x,%x,%x\r\n",
                rate[0],rate[1],rate[2],rate[3],rate[4],rate[5],rate[6],rate[7]));
       
   BitMap = 0;
   didx = 0; 
   for(idx=0; idx<sizeof(NDIS_802_11_RATES); idx++)
   {
      if (((value = ConvertNDISRateToFWIndex(rate[idx])) != MRVDRV_NUM_SUPPORTED_RATES) &&
          (rate[idx] != 0)) 
      {
          BitMap |= (1L<<value); 
          didx++;
      }
   }  
   
   if( didx > 1 )
      RA.EnableHwAuto = 1;
   else
      RA.EnableHwAuto = 0;

   RA.Bitmap = BitMap;
   RA.Threshold = 3;
   RA.FinalRate = 1;

   Status =  ProcessOIDRateAdaptRateSet(Adapter,
                                        HostCmd_ACT_SET,
                                        sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET),
                                        &len,
                                        (PVOID)&RA);
   if( Status != NDIS_STATUS_SUCCESS )
       return Status; 

   return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
AutoDeepSleepPending(PMRVDRV_ADAPTER Adapter)
{
      int cnt = 0;

      while( Adapter->IsAutoDeepSleep == 1 && ++cnt <10 )
      {
        DBGPRINT(DBG_ERROR,(L"is AutoDeepsleeping, pending OID access\r\n")); 
        NdisMSleep(100000);
      } 

      if( Adapter->IsAutoDeepSleep == 1 ) 
          return NDIS_STATUS_NOT_ACCEPTED;
      else
          return NDIS_STATUS_SUCCESS;
}







 
VOID DumpStatisticCounter(PNDIS_802_11_STATISTICS pStatistics)
{

    DBGPRINT(DBG_CUSTOM, (L"[DumpStatisticCounter]:Length=0x%x \r\n", pStatistics->Length ));	
    DBGPRINT(DBG_CUSTOM, (L"TransmittedFragmentCount: Low %x High %x \r\n", 
             pStatistics->TransmittedFragmentCount.LowPart, pStatistics->TransmittedFragmentCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"MulticastTransmittedFrameCount: Low %x High %x \r\n", 
             pStatistics->MulticastTransmittedFrameCount.LowPart, pStatistics->MulticastTransmittedFrameCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"FailedCount: Low %x High %x \r\n", 
             pStatistics->FailedCount.LowPart, pStatistics->FailedCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"RetryCount: Low %x High %x \r\n", 
             pStatistics->RetryCount.LowPart, pStatistics->RetryCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"MultipleRetryCount: Low %x High %x \r\n", 
             pStatistics->MultipleRetryCount.LowPart, pStatistics->MultipleRetryCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"RTSSuccessCount: Low %x High %x \r\n", 
             pStatistics->RTSSuccessCount.LowPart, pStatistics->RTSSuccessCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"RTSFailureCount: Low %x High %x \r\n", 
             pStatistics->RTSFailureCount.LowPart, pStatistics->RTSFailureCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"ACKFailureCount: Low %x High %x \r\n", 
             pStatistics->ACKFailureCount.LowPart, pStatistics->ACKFailureCount.HighPart));		
    DBGPRINT(DBG_CUSTOM, (L"FrameDuplicateCount: Low %x High %x \r\n", 
             pStatistics->FrameDuplicateCount.LowPart, pStatistics->FrameDuplicateCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"ReceivedFragmentCount: Low %x High %x \r\n", 
             pStatistics->ReceivedFragmentCount.LowPart, pStatistics->ReceivedFragmentCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"MulticastReceivedFrameCount: Low %x High %x \r\n", 
             pStatistics->MulticastReceivedFrameCount.LowPart, pStatistics->MulticastReceivedFrameCount.HighPart));	
    DBGPRINT(DBG_CUSTOM, (L"FCSErrorCount: Low %x High %x \r\n", 
             pStatistics->FCSErrorCount.LowPart, pStatistics->FCSErrorCount.HighPart));	
   //below item was under #ifndef UNDER_CE
#ifndef UNDER_CE
   DBGPRINT(DBG_CUSTOM, (L"TKIPLocalMICFailures: Low %x High %x \r\n", 
             pStatistics->TKIPLocalMICFailures.LowPart, pStatistics->TKIPLocalMICFailures.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"TKIPICVErrorCount: Low %x High %x \r\n", 
             pStatistics->TKIPICVErrorCount.LowPart, pStatistics->TKIPICVErrorCount.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"TKIPCounterMeasuresInvoked: Low %x High %x \r\n", 
             pStatistics->TKIPCounterMeasuresInvoked.LowPart, pStatistics->TKIPCounterMeasuresInvoked.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"TKIPReplays: Low %x High %x \r\n", 
             pStatistics->TKIPReplays.LowPart, pStatistics->TKIPReplays.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"CCMPFormatErrors: Low %x High %x \r\n", 
             pStatistics->CCMPFormatErrors.LowPart, pStatistics->CCMPFormatErrors.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"CCMPReplays: Low %x High %x \r\n", 
             pStatistics->CCMPReplays.LowPart, pStatistics->CCMPReplays.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"CCMPDecryptErrors: Low %x High %x \r\n", 
             pStatistics->CCMPDecryptErrors.LowPart, pStatistics->CCMPDecryptErrors.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"FourWayHandshakeFailures: Low %x High %x \r\n", 
             pStatistics->FourWayHandshakeFailures.LowPart, pStatistics->FourWayHandshakeFailures.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"WEPUndecryptableCount: Low %x High %x \r\n", 
             pStatistics->WEPUndecryptableCount.LowPart, pStatistics->WEPUndecryptableCount.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"WEPICVErrorCount: Low %x High %x \r\n", 
             pStatistics->WEPICVErrorCount.LowPart, pStatistics->WEPICVErrorCount.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"DecryptSuccessCount: Low %x High %x \r\n", 
             pStatistics->DecryptSuccessCount.LowPart, pStatistics->DecryptSuccessCount.HighPart));	
   DBGPRINT(DBG_CUSTOM, (L"DecryptFailureCount: Low %x High %x \r\n", 
             pStatistics->DecryptFailureCount.LowPart, pStatistics->DecryptFailureCount.HighPart));	
#endif           
	
}	  



NDIS_STATUS
ProcessOIDStatistics(PMRVDRV_ADAPTER Adapter,
                            ULONG InformationBufferLength,
                            PVOID InformationBuffer,
                            OUT PULONG BytesNeeded,
                            OUT PULONG BytesWritten)
{
    
    NDIS_STATUS     Status;
    PNDIS_802_11_STATISTICS pStatistics;

    if (InformationBufferLength < sizeof(NDIS_802_11_STATISTICS))
    {
        *BytesNeeded = sizeof(NDIS_802_11_STATISTICS);
	 DBGPRINT(DBG_OID|DBG_ERROR,(L"ProcessOIDStatistics: InformationBufferLength=0x%x, sizeof(NDIS_802_11_STATISTICS)=0x%x  \r\n", InformationBufferLength,sizeof(NDIS_802_11_STATISTICS)));	
        return NDIS_STATUS_INVALID_LENGTH;
    }


   Status=PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_GET_STAT,
                HostCmd_ACT_GEN_GET,
                HostCmd_OPTION_USE_INT,
                (NDIS_OID)0,
                HostCmd_PENDING_ON_GET_OID,
                0,
                FALSE,
                BytesWritten,
                NULL,
                NULL,
                InformationBuffer); 

       if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
            return Status;

       Status = WaitOIDAccessRespond(Adapter);
	   
       if( Status != NDIS_STATUS_SUCCESS || Adapter->nOidCmdResult != HostCmd_RESULT_OK )
       {   
          DBGPRINT(DBG_OID|DBG_ERROR,(L"Fail to issue statistic command ...%x \r\n", Adapter->nOidCmdResult));
          Status = NDIS_STATUS_NOT_ACCEPTED; 
          return Status;
       }

       NdisMoveMemory((PUCHAR)InformationBuffer,
                      (PUCHAR)Adapter->OidCmdRespBuf,
                      sizeof(NDIS_802_11_STATISTICS)); 

    *BytesWritten = sizeof(NDIS_802_11_STATISTICS);

     pStatistics = (PNDIS_802_11_STATISTICS)InformationBuffer;
     DumpStatisticCounter(pStatistics);
    
     return NDIS_STATUS_SUCCESS; 
}

/*
Function: ProcessOIDSetTxPowerLevel
Description: Set the tx-power level.
    - We make it by disable TPC & Set PA_P0, PA_1, PA_2 to the power level we expected
*/
NDIS_STATUS
ProcessOIDGetTxPowerLevel(PMRVDRV_ADAPTER Adapter, 
                                                            ULONG InformationBufferLength,
                                                            PVOID InformationBuffer,
                                                            OUT PULONG pBytesNeeded,
                                                            OUT PULONG pBytesWritten)
{
    NDIS_STATUS     Status;
    DWORD       dwWaitStatus;
    OID_MRVL_DS_POWER_ADAPT_CFG_EXT pacfg;
    ULONG BytesWritten, BytesNeeded;
    ULONG           TxPowerLevel;
    OID_MRVL_DS_GET_TX_RATE       txrate;
    int i, PAGroupsCnt;
    USHORT          ratemap;

    DBGPRINT(DBG_TMP, (L"Enter %s\r\n", TEXT(__FUNCTION__)));
    if( InformationBufferLength < sizeof(NDIS_802_11_TX_POWER_LEVEL)) {
        *pBytesNeeded = sizeof(NDIS_802_11_TX_POWER_LEVEL);
        DBGPRINT(DBG_TMP|DBG_OID|DBG_HELP, (L"     Invalid length [In:%d, Needed:%d]\n", InformationBufferLength, *pBytesNeeded) );
        Status = NDIS_STATUS_INVALID_LENGTH;
        goto FuncFinal;
    }

    GetPACFGValue(Adapter);
    Adapter->nOidCmdResult = HostCmd_RESULT_ERROR;
    BytesWritten = sizeof(pacfg);
    DBGPRINT(DBG_TMP, (L"%s: Sending HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT\r\n", TEXT(__FUNCTION__)));
    Status=PrepareAndSendCommand( 
                                                            Adapter, 
                                                            (HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT), 
                                                            (HostCmd_ACT_GET), 
                                                            HostCmd_OPTION_USE_INT, 
                                                            0, 
                                                            HostCmd_PENDING_ON_GET_OID, 
                                                            0, 
                                                            FALSE, 
                                                            &BytesWritten, 
                                                            0, 
                                                            &BytesNeeded, 
                                                            &pacfg); 

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) {
        DBGPRINT(DBG_TMP|DBG_OID|DBG_ERROR, (L"==> %s: Send PowerAdapt failed\n", TEXT(__FUNCTION__)));
        goto FuncFinal; 
    }

    dwWaitStatus = WaitOIDAccessRespond( Adapter );
    if ((dwWaitStatus != NDIS_STATUS_SUCCESS) ||
        (Adapter->nOidCmdResult != HostCmd_RESULT_OK ))  {
        *pBytesWritten = 0;
        Status = NDIS_STATUS_NOT_ACCEPTED;
        DBGPRINT(DBG_TMP|DBG_OID|DBG_ERROR, (L"==> %s: set PowerAdapt failed\n", TEXT(__FUNCTION__)));
        goto FuncFinal;
    }
    DBGPRINT(DBG_TMP, (L"%s: HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT competed\r\n", TEXT(__FUNCTION__)));
    NdisMoveMemory((PUCHAR) &pacfg.Action, 
         Adapter->OidCmdRespBuf + ((sizeof(NDIS_OID)+2)),
         sizeof(HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT *)0)->Action)) );
    
    if (pacfg.EnablePA == FALSE) {
        TxPowerLevel = pacfg.PowerAdaptGroup.PA_Group[0].PowerAdaptLevel;
        Status = NDIS_STATUS_SUCCESS;
        DBGPRINT(DBG_TMP|DBG_OID|DBG_HELP, (L"==> %s: Power Groups is off\n", TEXT(__FUNCTION__)));
        goto FuncFinal;
    }
    PAGroupsCnt = pacfg.PowerAdaptGroup.Header.Len / sizeof(PA_Group_t);
    DBGPRINT(DBG_TMP, (L" (Len, Group)=(%d, %d), %d\r\n", pacfg.PowerAdaptGroup.Header.Len, PAGroupsCnt, sizeof(PA_Group_t)));
    for (i=0 ; i<PAGroupsCnt; i++) {
        DBGPRINT(DBG_TMP, (L"%s: PAGroup(%d):%d\r\n", TEXT(__FUNCTION__), i, pacfg.PowerAdaptGroup.PA_Group[i].PowerAdaptLevel));
    }
    
    for (i=0 ; i<PAGroupsCnt; i++) {
        if (pacfg.PowerAdaptGroup.PA_Group[0].PowerAdaptLevel != pacfg.PowerAdaptGroup.PA_Group[i].PowerAdaptLevel) {
            break;
        }
    }
/*    if (i == PAGroupsCnt) {
        /// Power Level of all groups are identical. Report this value
        TxPowerLevel = pacfg.PowerAdaptGroup.PA_Group[0].PowerAdaptLevel;
        Status = NDIS_STATUS_SUCCESS;
        DBGPRINT(DBG_TMP|DBG_OID|DBG_HELP, (L"==> %s: Power Groups are identical\n", TEXT(__FUNCTION__)));
        goto FuncFinal;
    }
*/

    DBGPRINT(DBG_TMP, (L"%s: Querying TX_Rate\r\n", TEXT(__FUNCTION__)));
    ///The Power Levels are different. Query the Tx_Rate to see which group is being used.
    Adapter->nOidCmdResult = HostCmd_RESULT_ERROR;
    BytesWritten = sizeof(txrate);
    Status=PrepareAndSendCommand(
                    Adapter,
                    (HostCmd_CMD_802_11_TX_RATE_QUERY),
                    (HostCmd_ACT_GEN_GET),
                    HostCmd_OPTION_USE_INT,
                    0,
                    HostCmd_PENDING_ON_GET_OID,
                    0,
                    FALSE,
                    &BytesWritten,
                    NULL,
                    &BytesNeeded,
                    &txrate);

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) {
        DBGPRINT(DBG_TMP|DBG_OID|DBG_ERROR, (L"==> %s: Sending TxRateQuery Failed\n", TEXT(__FUNCTION__)));
        goto FuncFinal; 
    }
    dwWaitStatus = WaitOIDAccessRespond( Adapter );
    if ((dwWaitStatus != NDIS_STATUS_SUCCESS) ||
         (Adapter->nOidCmdResult != HostCmd_RESULT_OK))  {
         Status = NDIS_STATUS_NOT_ACCEPTED;
         DBGPRINT(DBG_TMP|DBG_OID|DBG_ERROR, (L"==> %s: seting TxRateQuery Failed\n", TEXT(__FUNCTION__)));
         goto FuncFinal;
    }
    DBGPRINT(DBG_TMP, (L"%s: Querying TX_Rate Completed\r\n", TEXT(__FUNCTION__)));
    NdisMoveMemory(
                (PUCHAR) &txrate,
                (PUCHAR) (Adapter->OidCmdRespBuf),
                Adapter->nSizeOfOidCmdResp );
    DBGPRINT(DBG_TMP|DBG_CCX|DBG_OID|DBG_HELP, (L"%s: Current_TxRate: %d\r\n", TEXT(__FUNCTION__), txrate));
    ratemap = 1 << txrate.usTxRate;
    for (i=0 ; i<MAX_POWER_ADAPT_GROUP ; i++) {
        if (pacfg.PowerAdaptGroup.PA_Group[i].RateBitmap & ratemap) {
            break;
        }
    }
    if (i == MAX_POWER_ADAPT_GROUP) {
        DBGPRINT(DBG_TMP|DBG_OID|DBG_ERROR, (L"tx_rate-Power_level mapping failed\n"));
        Status = NDIS_STATUS_FAILURE;
         goto FuncFinal;
    }
    TxPowerLevel = pacfg.PowerAdaptGroup.PA_Group[i].PowerAdaptLevel;
    Status = NDIS_STATUS_SUCCESS;

FuncFinal:
    if (Status == NDIS_STATUS_SUCCESS) {
        ///Generate the TX_POWER_LEVEL data structure to report to system
        DBGPRINT(DBG_TMP|DBG_OID|DBG_HELP, (L"Reporting tx_power_level: %d\n", TxPowerLevel));
        NdisMoveMemory(
                (PUCHAR) InformationBuffer,
                (PUCHAR) (&TxPowerLevel),
                sizeof(NDIS_802_11_TX_POWER_LEVEL) );
        *pBytesWritten = sizeof(NDIS_802_11_TX_POWER_LEVEL);
        DBGPRINT(DBG_TMP|DBG_OID|DBG_HELP, (L"Reported tx_power_level: %d\n", TxPowerLevel));
    }
    return Status;
}
/*
Function: ProcessOIDSetTxPowerLevel
Description: Set the tx-power level.
    - We make it by disable TPC & Set PA_P0, PA_1, PA_2 to the power level we expected
*/
NDIS_STATUS
ProcessOIDSetTxPowerLevel(PMRVDRV_ADAPTER Adapter, ULONG PowerLevel)
{
    NDIS_STATUS     Status;
///    DWORD       dwWaitStatus;
    OID_MRVL_DS_TPC_CFG     tpccfg;
    OID_MRVL_DS_POWER_ADAPT_CFG_EXT pacfg;
    ULONG BytesWritten;          

    DBGPRINT(DBG_CCX|DBG_OID|DBG_HELP, (L"==>Disabling TPC\r\n"));
    ///Disable TPC 
    tpccfg.Oid = OID_802_11_TX_POWER_LEVEL;
    tpccfg.Enable = 0;
    tpccfg.P0 = tpccfg.P1 = tpccfg.P2 = 0;
    tpccfg.UseSNR = 0;
    BytesWritten = sizeof(OID_MRVL_DS_TPC_CFG);
    Status=PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_TPC_CFG,
                HostCmd_ACT_GEN_SET,
                HostCmd_OPTION_USE_INT,
                (NDIS_OID)0,
                HostCmd_PENDING_ON_NONE,
                0,
                FALSE,
                &BytesWritten,
                NULL,
                NULL,
                &tpccfg); 

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) {
            DBGPRINT(DBG_CCX|DBG_OID|DBG_ERROR, (L"==>Disabling TPC, sending CMD failed (%xh)\r\n", Status));
            goto FuncFinal;
    }
/*    dwWaitStatus = WaitOIDAccessRespond(Adapter);
    if ((dwWaitStatus  != NDIS_STATUS_SUCCESS) ||
        (Adapter->nOidCmdResult != HostCmd_RESULT_OK )) {
        DBGPRINT(DBG_CCX|DBG_OID|DBG_ERROR, (L"==>Disabling TPC, set CMD failed (%xh)\r\n", dwWaitStatus));
        Status = NDIS_STATUS_NOT_ACCEPTED;
        goto FuncFinal;
    }
    */
    ///Enable PA and set PA_P0, PA_P1, PA_P2 to the level we expected
    DBGPRINT(DBG_CCX|DBG_OID|DBG_HELP, (L"==>Setting PA (%d)\r\n", PowerLevel));
    ///=======================
    pacfg.Oid = OID_802_11_TX_POWER_LEVEL;
    pacfg.Action = HostCmd_ACT_GEN_SET;
    pacfg.EnablePA = (USHORT)1;
    pacfg.PowerAdaptGroup.Header.Type = 0x114;               ///MrvlIETypes_PowerAdapt_Group_t
    //pacfg.PowerAdaptGroup.PA_Group[0].PowerAdaptLevel = (USHORT)PowerLevel;
    //pacfg.PowerAdaptGroup.PA_Group[0].RateBitmap = 0x1fef;          ///Set all rates to this group;
    pacfg.PowerAdaptGroup.PA_Group[0].PowerAdaptLevel = (USHORT)PowerLevel;
    pacfg.PowerAdaptGroup.PA_Group[0].RateBitmap = 0x1800;          ///Set all rates to this group;
    pacfg.PowerAdaptGroup.PA_Group[1].PowerAdaptLevel = (USHORT)PowerLevel;
    pacfg.PowerAdaptGroup.PA_Group[1].RateBitmap = 0x07e0;          ///Set all rates to this group;
    pacfg.PowerAdaptGroup.PA_Group[2].PowerAdaptLevel = (USHORT)PowerLevel;
    pacfg.PowerAdaptGroup.PA_Group[2].RateBitmap = 0x000f;          ///Set all rates to this group;

    pacfg.PowerAdaptGroup.Header.Len = sizeof(PA_Group_t)*3;
    pacfg.CmdLen = sizeof(USHORT)*4+pacfg.PowerAdaptGroup.Header.Len ;

    NdisZeroMemory(Adapter->PACfg, LENGTH_PA_CFG);
    SaveCurrentConfig( Adapter, &pacfg, pacfg.CmdLen,HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT );

    Status = PrepareAndSendCommand( 
                                     Adapter, 
                                     HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT,
                                     HostCmd_ACT_SET, 
                                     HostCmd_OPTION_USE_INT,
                                     0, 
                                     HostCmd_PENDING_ON_NONE,
                                     0, 
                                     FALSE,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL );
     ///=======================

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) {
        DBGPRINT(DBG_CCX|DBG_OID|DBG_ERROR, (L"==>Setting PA, sending CMD failed (%xh)\r\n", Status));
        goto FuncFinal;
    }
    /*
    dwWaitStatus = WaitOIDAccessRespond(Adapter);
    if ((dwWaitStatus  != NDIS_STATUS_SUCCESS) ||
        (Adapter->nOidCmdResult != HostCmd_RESULT_OK )) {
        DBGPRINT(DBG_CCX|DBG_OID|DBG_ERROR, (L"==>Setting PA, set CMD failed (%xh)\r\n", Status));
        Status = NDIS_STATUS_NOT_ACCEPTED;
        goto FuncFinal;
    };
    */
    DBGPRINT(DBG_CCX|DBG_OID|DBG_HELP, (L"==>Setting Power Level (%d) Successfully\r\n", PowerLevel));
FuncFinal:
    return Status;
}

NDIS_STATUS
ProcessOIDBgScanCfg(PMRVDRV_ADAPTER Adapter,
                    IN NDIS_OID Oid, 
                    IN PVOID InformationBuffer,
                    IN ULONG InformationBufferLength,
                    OUT PULONG BytesNeeded )
{
   NDIS_STATUS     Status;
   POID_MRVL_DS_BG_SCAN_CONFIG pOidBG;
                    

   if (InformationBufferLength >= sizeof(ULONG) + 2*sizeof(USHORT) + 1) 
   {
       pOidBG = (POID_MRVL_DS_BG_SCAN_CONFIG) InformationBuffer;
     
       RETAILMSG(1,(L"BG_SCAN ACTION(OID:%x):%x\r\n",Oid,*(USHORT*)&pOidBG->Action));

       if (pOidBG->Enable == 1)
       {   
           if (InformationBufferLength < sizeof(OID_MRVL_DS_BG_SCAN_CONFIG)+6-1)
           { 
               *BytesNeeded = sizeof(OID_MRVL_DS_BG_SCAN_CONFIG)+6-1;
               return NDIS_STATUS_INVALID_LENGTH;
           }
       }

       if( InformationBufferLength > LENGTH_BGSCAN_CFG )
       {    
           *BytesNeeded = LENGTH_BGSCAN_CFG;
           return NDIS_STATUS_INVALID_LENGTH;    
       }
   }
   else
   {    
       *BytesNeeded = sizeof(ULONG) + 2*sizeof(USHORT) + 1;
       return NDIS_STATUS_INVALID_LENGTH;
   }

      
   if( Oid == OID_MRVL_BG_SCAN_CONFIG )
   {                  
      Adapter->BgScanCfg = &Adapter->BgScanCfgInfo[BG_SCAN_NORMAL][0];
      Adapter->BgScanCfgInfoLen[BG_SCAN_NORMAL] = InformationBufferLength;
      Adapter->nBgScanCfg = Adapter->BgScanCfgInfoLen[BG_SCAN_NORMAL];
   } 
                
   if ( pOidBG->Enable==1) 
       NdisZeroMemory(Adapter->BgScanCfg, LENGTH_BGSCAN_CFG);
    
   //length has assigned earlier 
   NdisMoveMemory( Adapter->BgScanCfg,  InformationBuffer ,InformationBufferLength );
     
   //SaveCurrentConfig( Adapter, InformationBuffer, InformationBufferLength,HostCmd_CMD_802_11_BG_SCAN_CONFIG );

   Status = PrepareAndSendCommand( 
                        Adapter, 
                        HostCmd_CMD_802_11_BG_SCAN_CONFIG,
                        HostCmd_ACT_SET, 
                        HostCmd_OPTION_USE_INT,
                        0, 
                        HostCmd_PENDING_ON_SET_OID,
                        0, 
                        FALSE,
                        NULL,
                        NULL,
                        NULL,
                        NULL );
   
   if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) 
   {
   	   DBGPRINT(DBG_ERROR, (L"Set:OID=%s \r\n", OIDMSG(Oid,NDIS_STATUS_FAILURE)));  
       return NDIS_STATUS_FAILURE; 
   }
    

   Status = WaitOIDAccessRespond(Adapter);
   if( Status != NDIS_STATUS_SUCCESS || Adapter->nOidCmdResult != HostCmd_RESULT_OK )
   {   
       DBGPRINT(DBG_OID|DBG_ERROR,(L"BG_SCAN_CONFIG command Fail...\r\n"));
       Status = NDIS_STATUS_FAILURE; 
       return Status;
   } 

   return NDIS_STATUS_SUCCESS;   
}



#if 0
NDIS_STATUS
ProcessOIDSetTxPowerLevel(PMRVDRV_ADAPTER Adapter, ULONG PowerLevel)
{
    NDIS_STATUS     Status;
    ULONG BytesNeeded;
    ULONG BytesWritten=sizeof(PowerLevel);
    DWORD       dwWaitStatus;

    ///Adapter->nOidCmdResult = HostCmd_RESULT_ERROR;
    Status=PrepareAndSendCommand(
                    Adapter,
                    HostCmd_CMD_802_11_RF_TX_POWER,
                    HostCmd_ACT_TX_POWER_OPT_SET,
                    HostCmd_OPTION_USE_INT,
                    OID_802_11_TX_POWER_LEVEL,
                    HostCmd_PENDING_ON_SET_OID,
                    0,
                    FALSE,
                    &BytesWritten,
                    NULL,
                    &BytesNeeded,
                    &PowerLevel);

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) {
            DBGPRINT(DBG_CCX|DBG_OID|DBG_ERROR, (L"==>Setting TX_Power_Level Failed, sending CMD failed (%xh)\r\n", Status));
            goto FuncFinal;
    }
    dwWaitStatus = WaitOIDAccessRespond(Adapter);
    if ((dwWaitStatus  != NDIS_STATUS_SUCCESS) ||
        (Adapter->nOidCmdResult != HostCmd_RESULT_OK )) {
        DBGPRINT(DBG_CCX|DBG_OID|DBG_ERROR, (L"==>Setting TX_Power_Level Failed, set CMD failed (%xh, %xh)\r\n", dwWaitStatus, Adapter->nOidCmdResult));
        Status = NDIS_STATUS_NOT_ACCEPTED;
        goto FuncFinal;
    }

    Adapter->TxPowerLevelIsSetByOS = TRUE;

FuncFinal:
    return Status;
}
#endif ///0
