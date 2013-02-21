/******************* ?Marvell Semiconductor, Inc., *************************
 *
 *  Purpose:
 *
 *	    Structure and constant definition for custome OID supported by 
 *      the driver
 *
 *  Notes:
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/10/21 $
 *
 *	$Revision: #3 $
 *
 *****************************************************************************/

#ifndef _MRVL_OID_H_
#define _MRVL_OID_H_

#ifndef NDIS_OID
typedef ULONG NDIS_OID, *PNDIS_OID;
#endif


//
//
// IEEE 802.11 OIDs
//
//Marked by ice.zhang 20080828
#if  0
#define OID_802_11_BSSID                        0x0D010101
#define OID_802_11_SSID                         0x0D010102
#define OID_802_11_NETWORK_TYPES_SUPPORTED      0x0D010203
#define OID_802_11_NETWORK_TYPE_IN_USE          0x0D010204
#define OID_802_11_TX_POWER_LEVEL               0x0D010205
#define OID_802_11_RSSI                         0x0D010206
#define OID_802_11_RSSI_TRIGGER                 0x0D010207
#define OID_802_11_INFRASTRUCTURE_MODE          0x0D010108
#define OID_802_11_FRAGMENTATION_THRESHOLD      0x0D010209
#define OID_802_11_RTS_THRESHOLD                0x0D01020A
#define OID_802_11_NUMBER_OF_ANTENNAS           0x0D01020B
#define OID_802_11_RX_ANTENNA_SELECTED          0x0D01020C
#define OID_802_11_TX_ANTENNA_SELECTED          0x0D01020D
#define OID_802_11_SUPPORTED_RATES              0x0D01020E
#define OID_802_11_DESIRED_RATES                0x0D010210
#define OID_802_11_CONFIGURATION                0x0D010211
#define OID_802_11_STATISTICS                   0x0D020212
#define OID_802_11_ADD_WEP                      0x0D010113
#define OID_802_11_REMOVE_WEP                   0x0D010114
#define OID_802_11_DISASSOCIATE                 0x0D010115
#define OID_802_11_POWER_MODE                   0x0D010216
#define OID_802_11_BSSID_LIST                   0x0D010217
#define OID_802_11_AUTHENTICATION_MODE          0x0D010118
#define OID_802_11_PRIVACY_FILTER               0x0D010119
#define OID_802_11_BSSID_LIST_SCAN              0x0D01011A
#define OID_802_11_WEP_STATUS                   0x0D01011B
#define OID_802_11_RELOAD_DEFAULTS              0x0D01011C
#define OID_802_11_ENCRYPTION_STATUS OID_802_11_WEP_STATUS
#define OID_802_11_ADD_KEY                      0x0d01011D
#define OID_802_11_REMOVE_KEY                   0x0d01011E
#define OID_802_11_ASSOCIATION_INFORMATION      0x0d01011F
#define OID_802_11_TEST                         0x0d010120
#define OID_802_11_CAPABILITY                   0x0d010122
#define OID_802_11_PMKID                        0x0d010123



#define NDIS_802_11_LENGTH_SSID  32
#define Ndis802_11AuthModeWPA2  6
#define Ndis802_11AuthModeWPA2PSK  7
#define Ndis802_11StatusType_PMKID_CandidateList 2


typedef struct _NDIS_PACKET_8021Q_INFO {
	 union {
		struct {
			UINT32  UserPriority : 3;
			UINT32  CanonicalFormatId : 1;
			UINT32  VlanId : 12;
			UINT32  Reserved : 16;
		} TagHeader;
		PVOID  Value;
	} DUMMYUNIONNAME;
} NDIS_PACKET_8021Q_INFO, *PNDIS_PACKET_8021Q_INFO;

typedef struct _PMKID_CANDIDATE
{
  NDIS_802_11_MAC_ADDRESS  BSSID;
  DWORD  Flags;
} *PPMKID_CANDIDATE;

typedef struct NDIS_802_11_AUTHENTICATION_ENCRYPTION { 
         NDIS_802_11_AUTHENTICATION_MODE AuthModeSupported; 
         NDIS_802_11_ENCRYPTION_STATUS EncryptStatusSupported; 
} NDIS_802_11_AUTHENTICATION_ENCRYPTION; 
  
typedef struct _NDIS_802_11_CAPABILITY{  
	ULONG  Length;  ULONG  Version;  
	ULONG  NoOfPMKIDs;  
	ULONG  NoOfAuthEncryptPairsSupported;  
	NDIS_802_11_AUTHENTICATION_ENCRYPTION AuthenticationEncryptionSupported[1];
} NDIS_802_11_CAPABILITY, *PNDIS_802_11_CAPABILITY;

// PMKID Structures
typedef UCHAR   NDIS_802_11_PMKID_VALUE[16];

typedef struct _BSSID_INFO
{
        NDIS_802_11_MAC_ADDRESS BSSID;
        NDIS_802_11_PMKID_VALUE PMKID;
} BSSID_INFO, *PBSSID_INFO;


typedef struct _NDIS_802_11_PMKID
{
        ULONG Length;
        ULONG BSSIDInfoCount;
        BSSID_INFO BSSIDInfo[1];
} NDIS_802_11_PMKID, *PNDIS_802_11_PMKID;

typedef struct PMKID_CANDIDATE {
	NDIS_802_11_MAC_ADDRESS BSSID;
	ULONG Flags;
} PMKID_CANDIDATE;

#define NDIS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLED 0x01


typedef struct NDIS_802_11_PMKID_CANDIDATE_LIST {
	ULONG Version;
	ULONG NumCandidates;
	PMKID_CANDIDATE CandidateList[1];
} NDIS_802_11_PMKID_CANDIDATE_LIST;

#endif  /*0*/

typedef enum 
{
    OID_FSW_CCX_CONFIGURATION = 0xFFCCA000,
    OID_FSW_CCX_NETWORK_EAP,
    OID_FSW_CCX_ROGUE_AP_DETECTED,
    OID_FSW_CCX_REPORT_ROGUE_APS,
    OID_FSW_CCX_AUTH_SUCCESS,
    OID_FSW_CCX_CCKM_START,     /* not an OID - Status message */
    OID_FSW_CCX_CCKM_REQUEST,   /* new one */
    OID_FSW_CCX_CCKM_RESULT
} FUNK_CCX_OID;




// ****************************************************************************
//          Marvel specific OIDs
//
#define OID_MRVL_OEM_GET_ULONG                  0xff010201
#define OID_MRVL_OEM_SET_ULONG                  0xff010202
#define OID_MRVL_OEM_GET_STRING                 0xff010203
#define OID_MRVL_OEM_COMMAND                    0xff010204
// set the region code
// the setting is permanently written into card's EEPROM
#define OID_MRVL_REGION_CODE					0xff010205
#define OID_MRVL_RADIO_PREAMBLE					0xff010206
#define OID_MRVL_BSSID_SCAN						0xff010207
#define OID_MRVL_SSID_SCAN						0xff010208
// set the RF channel
// can only be accessed when not associated, SET only
// Information buffer should contain the channel number
#define OID_MRVL_RF_CHANNEL						0xff010209
#define OID_MRVL_WIRELESS_MODE					0xff01020a
#define OID_MRVL_CHIP_VERSION					0xff01020b
// access Baseband register
#define OID_MRVL_BBP_REG                        0xff01020c
// access MAC register
#define OID_MRVL_MAC_REG                        0xff01020d
// access RF register
#define OID_MRVL_RF_REG                         0xff01020e
// access flash memory

#define OID_MRVL_EEPROM_ACCESS                  0xff01020f
// send manufacture command
//#define OID_MRVL_MFG_COMMAND                  0xff010210

// program the MAC address in the EEPROM
// driver and firmware will also update the MAC address
#define OID_MRVL_MAC_ADDRESS                    0xFF010210
// put the card into TX mode
// can only be accessed when not associated, SET Only
#define OID_MRVL_TX_MODE                        0xFF010211
// put the card into RX mode
// can only be accessed when not associated, SET only
#define OID_MRVL_RX_MODE						0xFF010212
// L2 romaing OID
#define OID_MRVL_L2ROAMING						0XFF010213
// LED control, set only
// Controls the behavior of the LED
#define OID_MRVL_LED_CONTROL					0xFF010214
// Multiple dtim value, set only
// Number of DTIM to sleep
//#define OID_MRVL_MULTIPLE_DTIM					0xFF010215
#define OID_MRVL_ENABLE_RADIO					0xFF010217
#define OID_MRVL_DISABLE_RADIO					0xFF010218
#define OID_MRVL_RADIO_CONTROL					0xFF010219

#define OID_MRVL_DEEP_SLEEP		            	0xff010220

#define OID_MRVL_REMOVE_ADHOCAES				0xff010221
#define OID_MRVL_SET_ADHOCAES					0xff010222
#define OID_MRVL_GET_ADHOCAES					0xff010223


#define	OID_MRVL_HOST_SLEEP_CFG					0xff010224
#define OID_MRVL_WAKEUP_DEVICE					0xff01023A 


#define OID_MRVL_SLEEP_PARAMS					0xff010225
#define OID_MRVL_SCAN_EXT             0xff01022f
#define OID_MRVL_WMM_STATE						0xff010234
#define OID_MRVL_WMM_ACK_POLICY					0xff010235
#define OID_MRVL_WMM_STATUS						0xff010236

#define OID_MRVL_SLEEP_PERIOD					0xff010237 

//022607
#define OID_MRVL_BCA_TIMESHARE                  0xff010238


#define	OID_MRVL_HOST_WAKE_UP_SYNC_PS			0xff010230


#define	OID_MRVL_FW_WAKE_METHOD					0xff010231 
#define OID_MRVL_INACTIVITY_TIMEOUT				0xff010232 

#define OID_MRVL_RSSI_BCNAVG					0xff01023C 
#define OID_MRVL_RSSI_DATAAVG					0xff01023D
#define OID_MRVL_RSSI_THRESHOLDTIMER			0xff01023E
        
//ahan, [2005-12-13]   ++dralee_20060217
#define OID_MRVL_SUBSCRIBE_EVENT				0xff01023F

#define OID_MRVL_EMI_CHANNEL					0xff01023B

#define OID_MRVL_ATIMWINDOW						0xff010241
        
#define OID_MRVL_BG_SCAN_CONFIG         		0xff010239
#define OID_MRVL_BG_SCAN_QUERY           		0xff010240
#define OID_MRVL_BG_SCAN_CONFIG_UAPSD           0xff01027f


#define OID_MRVL_KEY_ENCRYPT					0xff010244
#define OID_MRVL_CRYPTO							0xff010245

#define OID_MRVL_802_11D_ENABLE					0xff010246
//dralee_20060509
#define OID_MRVL_802_11_GET_LOG                0xff010247
#define OID_MRVL_802_11_BAND_CFG               0xff01024B


#define OID_MRVL_GET_TX_RATE					0xff010248

#define OID_MRVL_GET_RX_INFO					0xff010249

#define OID_MRVL_POWER_ADAPT_CFG_EXT			0xff01024A

//dralee_20060515
#define OID_MRVL_LOCAL_LISTEN_INTERVAL			0xff010260

//dralee_20060601
#define OID_MRVL_IBSS_COALESCING_CFG         0xff010261

#define OID_MRVL_PROPRIETARY_PERIODIC_PS     0xff010262
          
//dralee_20060706
#define OID_MRVL_DBG_LOG_EVENT               0xff010263

#define OID_MRVL_RESET_CHIP                  0xff010264

#define OID_MRVL_MFG_COMMAND	             0xFF020000

typedef struct _OID_MRVL_DS_MFG_CMD
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    UCHAR   MfgCmd[MRVDRV_MFG_CMD_LEN];
} OID_MRVL_DS_MFG_CMD, *POID_MRVL_DS_MFG_CMD;        

#define OID_MRVL_ADHOC_AWAKED_PERIOD         0xff010265 

#define OID_MRVL_TEMP_FIX_CLOSEWZC	          0xff010266

#define OID_MRVL_HOSTCMD	                 0xff010267

#define OID_MRVL_TX_CONTROL	                 0xff010268

#define OID_MRVL_ADHOC_G_PROTECTION          0xff010269

#define OID_MRVL_HS_CANCEL                 0xff01027a

#define OID_MRVL_ADHOC_DEFAULT_CHANNEL          0xff01026d

#define OID_MRVL_S56   0xff01026e



//GUI_NOTIFY
///This is the header our firmware is using to generate our own packet which is not from TCP/IP stack
/*typedef struct _MRVPKT_DESC {
	UCHAR dstAddr[MRVDRV_ETH_ADDR_LEN];
	UCHAR srcAddr[MRVDRV_ETH_ADDR_LEN];
	UCHAR protocolLen[2];			// protocol Length set to 0 can set SNAP
} MRVPKTDESC, *PMRVPKTDESC;*/

typedef enum {
	NOTIFY_IAPP,			//Passing the IAPP  packet
	NOTIFY_ROAMING,			//Roaming criteria matched
	NOTIFY_ASSOCIATED,		//Pass the association paramters
	NOTIFY_DISCONNECT,		//Pass the disconnected parameter
	NOTIFY_HS_ACTIVATED,	//Notify Application the host sleep mode is activated in both driver & FW
	NOTIFY_HS_DEACTIVATED,	//Notify Application the host sleep mode is deactivated in both driver & FW
	NOTIFY_SCAN_RDY,         //Notify Application the BG SCAN completed.
	NOTIFY_FW_RELOADED,    //Notify Fw re-reload completed    012207
	NOTIFY_CCX4_S56_TSM  	 //Notify the ccx4 s56 traffic stream metric IE
} NOTIFY_UI_TYPE;

typedef struct _OID_MRVL_DS_NOTIFY_UI 
{
	NOTIFY_UI_TYPE		NotifyType;				///Notification Type
	DWORD				ParamLen;				///Length of the parameters, not including type & Length
} OID_MRVL_DS__NOTIFY_UI, *POID_MRVL_DS__NOTIFY_UI;

//#endif ///GUI_NOTIFY

////////////////////////////////////////////////////////////////
// Information buffer should point to one of the following
// data structure
////////////////////////////////////////////////////////////////
typedef struct _OID_MRVL_DS_BBP_REGISTER_ACCESS 
{
    NDIS_OID 	Oid;
    USHORT      usOffset;
    ULONG       ulValue;
} OID_MRVL_DS_BBP_REGISTER_ACCESS, *POID_MRVL_DS_BBP_REGISTER_ACCESS;


typedef struct _OID_MRVL_DS_RF_REGISTER_ACCESS 
{
    NDIS_OID 	Oid;
    USHORT      usOffset;
    ULONG       ulValue;
} OID_MRVL_DS_RF_REGISTER_ACCESS, *POID_MRVL_DS_RF_REGISTER_ACCESS;


typedef struct _OID_MRVL_DS_MAC_REGISTER_ACCESS 
{
    NDIS_OID 	Oid;
    USHORT      usOffset;
    ULONG       ulValue;
} OID_MRVL_DS_MAC_REGISTER_ACCESS, *POID_MRVL_DS_MAC_REGISTER_ACCESS;


typedef struct _OID_MRVL_DS_EEPROM_ACCESS 
{
    NDIS_OID 	Oid;
    USHORT      usDataLength;
    ULONG       ulAddress;
    UCHAR       pData[1];
} OID_MRVL_DS_EEPROM_ACCESS, *POID_MRVL_DS_EEPROM_ACCESS;


typedef struct _OID_MRVL_DS_REGION_CODE
{
     //dralee 081005, V4    
     //NDIS_OID 	Oid;	
    //ULONG 	    ulRegionCode;	
    ULONG       bIsEnable;
	USHORT 	    usRegionCode;	// country code 
    USHORT      usBand;         //  b:
	
} OID_MRVL_DS_REGION_CODE, *POID_MRVL_DS_REGION_CODE;
 

typedef struct _OID_MRVL_DS_MAC_ADDRESS
{
    NDIS_OID 	Oid;	
    UCHAR	 	EthAddr[6];	
} OID_MRVL_DS_MAC_ADDRESS, *POID_MRVL_DS_MAC_ADDRESS;

typedef struct _OID_DS_MRVL_RF_CHANNEL
{
    NDIS_OID 	Oid;	
    ULONG		ulChannelNumber;	
} OID_MRVL_DS_RF_CHANNEL, *POID_MRVL_DS_RF_CHANNEL;

typedef struct _OID_DS_MRVL_RF_RADIO
{
	NDIS_OID Oid;	
	UCHAR	 RADIO;	
} OID_DS_MRVL_RF_RADIO, * POID_DS_MRVL_RF_RADIO;


// OID_MRVL_TX_Mode Mode values
#define TX_MODE_NORMAL      0
#define TX_MODE_CONT        1
#define TX_MODE_CW          2
#define TX_MODE_MAX         3   // not a valid value, used as a upper limit

typedef struct _OID_MRVL_DS_TX_MODE
{
    NDIS_OID 	Oid;	
    ULONG 	    ulMode;	
} OID_MRVL_DS_TX_MODE, *POID_MRVL_DS_TX_MODE;


// OID_MRVL_RX_Mode Mode values
#define RX_MODE_NORMAL      0
#define RX_MODE_RDONLY      1
#define RX_MODE_MAX         2   // not a valid value, used as a upper limit

typedef struct _OID_DS_MRVL_RX_MODE
{
    NDIS_OID 	Oid;	
    ULONG 	    ulMode;	
} OID_MRVL_DS_RX_MODE, *POID_MRVL_DS_RX_MODE;

// L2 Roaming data structure
typedef struct _OID_MRVL_DS_L2ROAMING
{
    NDIS_OID    Oid;
    UCHAR       Mode;   // Off, Fixed level roaming, or auto level roaming
    UCHAR       Delta;  // Delta from the original AP RSSI used for switching
                        // between 0-100.  For example if 80% is desired, use 80
    ULONG       Period; // number of seconds between each period
} OID_MRVL_DS_L2ROAMING, *POID_MRVL_DS_L2ROAMING;

// Mode definition for OID_MRVL_DS_L2ROAMING
#define MRVL_L2ROAMING_MODE_OFF     0
#define MRVL_L2ROAMING_MODE_FIXED   1   // user picks the delta
#define MRVL_L2ROAMING_MODE_AUTO    2   // driver picks the delta

// minmum roaming period is 30 seconds
#define MRVL_L2ROAMING_MIN_PERIOD   30

// Define LED cycles
#define MRVL_LED_CONTROL_LED_CYCLE_37MS     0x0
#define MRVL_LED_CONTROL_LED_CYCLE_74MS     0x1
#define MRVL_LED_CONTROL_LED_CYCLE_149MS    0x2
#define MRVL_LED_CONTROL_LED_CYCLE_298MS    0x3
#define MRVL_LED_CONTROL_LED_CYCLE_596MS    0x4
#define MRVL_LED_CONTROL_LED_CYCLE_1192MS   0x5
#define MRVL_LED_CONTROL_LED_ON             0xfffd /* LED ON  */
#define MRVL_LED_CONTROL_LED_OFF            0xffff /* LED OFF */

/* 
Define LED duty cycles
*/
#define MRVL_LED_CONTROL_LED_DUTY_CYCLE_2   0x0  /* 1/2  */
#define MRVL_LED_CONTROL_LED_DUTY_CYCLE_4   0x1  /* 1/4  */
#define MRVL_LED_CONTROL_LED_DUTY_CYCLE_8   0x2  /* 1/8  */
#define MRVL_LED_CONTROL_LED_DUTY_CYCLE_16  0x3  /* 1/16 */
#define MRVL_LED_CONTROL_LED_DUTY_CYCLE_32  0x4  /* 1/32 */


typedef struct _OID_MRVL_DS_DEEP_SLEEP
{
	ULONG      	ulEnterDeepSleep; 
} OID_MRVL_DS_DEEP_SLEEP, *POID_MRVL_DS_DEEP_SLEEP;


#pragma pack(1)              
typedef struct _OID_DS_WakeUpFilterEntry 
{
    USHORT      AddressType; 
    USHORT      EthType; 
    ULONG      	Ipv4Addr;
} OID_DS_WakeUpFilterEntry, *POID_DS_WakeUpFilterEntry;

typedef struct _OID_MRVL_DS_HOST_WAKEUP_FILTER
{
    MrvlIEtypesHeader_t	     Header;
    OID_DS_WakeUpFilterEntry entry[MAX_HOST_SLEEP_WAKEUP_FILETER_ENTRY];
} OID_MRVL_DS_HOST_WAKEUP_FILTER, *POID_MRVL_DS_HOST_WAKEUP_FILTER;

typedef struct _OID_MRVL_DS_HOST_SLEEP_CFG
{      
    //NDIS_OID    OId;
	//USHORT      CmdLen;   

	ULONG      	ulCriteria; 
	UCHAR       ucGPIO; 
	UCHAR	    ucGap;
    OID_MRVL_DS_HOST_WAKEUP_FILTER Filter;
} OID_MRVL_DS_HOST_SLEEP_CFG, *POID_MRVL_DS_HOST_SLEEP_CFG;


//dralee_20060509
#define SPCFG_DISABLE  0
#define SPCFG_ENABLE   1
#define SPCFG_MASKED   2 
#define TIME_TO_RESTORE_HOST_SLEEP_CFG  15000   //in unit of ms


#pragma pack(1) 
typedef struct _OID_MRVL_DS_SLEEP_PARAMS
{
	USHORT      Error; 
	USHORT      Offset; 
	USHORT	    StableTime;
	UCHAR       CalControl;
	UCHAR       ExternalSleepClk;
	USHORT      Reserved;
} OID_MRVL_DS_SLEEP_PARAMS, *POID_MRVL_DS_SLEEP_PARAMS;


typedef struct _OID_MRVL_DS_FW_WAKE_METHOD
{
  USHORT method;
} OID_MRVL_DS_FW_WAKE_METHOD, *POID_MRVL_DS_FW_WAKE_METHOD;

typedef struct _OID_MRVL_INACTIVITY_TIMEOUT
{
  USHORT time;
} OID_MRVL_DS_INACTIVITY_TIMEOUT, *POID_MRVL_DS_INACTIVITY_TIMEOUT;  

typedef struct _OID_MRVL_DS_WMM_STATE
{
     UCHAR       State;	
}OID_MRVL_DS_WMM_STATE, *POID_MRVL_DS_WMM_STATE;	

typedef struct _OID_MRVL_DS_WMM_ACK_POLICY
{
     UCHAR       AC;
     UCHAR       AckPolicy;	
}OID_MRVL_DS_WMM_ACK_POLICY, *POID_MRVL_DS_WMM_ACK_POLICY;	

typedef struct _OID_MRVL_DS_WMM_AC_STATUS
{
     WMM_AC_STATUS  Status[4];
     MrvlIEtypesHeader_t Header;
     WMM_PARAMETER_IE    WmmParamIe;
     
}OID_MRVL_DS_WMM_AC_STATUS, *POID_MRVL_DS_WMM_AC_STATUS;	


typedef struct _OID_MRVL_DS_WMM_SLEEP_PERIOD
{
     USHORT  period;
}OID_MRVL_DS_WMM_SLEEP_PERIOD, *POID_MRVL_DS_WMM_SLEEP_PERIOD;	

typedef struct _OID_MRVL_DS_WAKEUP_DEVICE
{
   UCHAR    wake;
}OID_MRVL_DS_WAKEUP_DEVICE, *POID_MRVL_DS_WAKEUP_DEVICE;

          
//++dralee_20060412
typedef struct _OID_MRVL_DS_ATIM_WINDOW
{
     USHORT  atimwindow;
}OID_MRVL_DS_ATIM_WINDOW, *POID_MRVL_DS_ATIM_WINDOW;	



#pragma pack()

//tt ++ v5 firmware

#define OID_MRVL_TPC_CFG						0xff010226
#define OID_MRVL_PWR_CFG						0xff010227
#define OID_MRVL_RATE_ADAPT_RATESET			0xff010233
 
typedef struct _OID_MRVL_DS_LED_CONTROL
{
	NDIS_OID	Oid;
	USHORT	NumLed;
	UCHAR	data[256]; //TLV format
} OID_MRVL_DS_LED_CONTROL, *POID_MRVL_DS_LED_CONTROL;

#define OID_MRVL_CAL_DATA						0xff010228
#define OID_MRVL_CAL_DATA_EXT					0xff010229
typedef struct _OID_MRVL_DS_CAL_DATA
{
	NDIS_OID	Oid;
	UCHAR	Reserved1[9];
	UCHAR	PAOption;		/* PA calibration options */
	UCHAR	ExtPA;			/* type of external PA */
	UCHAR	Ant;			/* Antenna selection */
	USHORT	IntPA[14];		/* channel calibration */
	UCHAR	PAConfig[4];		/* RF register calibration */
	UCHAR	Reserved2[4];
	USHORT	Domain;			/* Regulatory Domain */
	UCHAR	ECO;			/* ECO present or not */
	UCHAR	LCT_cal;		/* VGA capacitor calibration */
	UCHAR	Reserved3[12];		
} OID_MRVL_DS_CAL_DATA, *POID_MRVL_DS_CAL_DATA;

typedef struct _OID_MRVL_DS_CAL_DATA_EXT
{
	NDIS_OID	Oid;
	USHORT	Revision;
	USHORT	CalDataLen;
	UCHAR	CalData[1024]; 
} OID_MRVL_DS_CAL_DATA_EXT, *POID_MRVL_DS_CAL_DATA_EXT;

typedef struct _OID_MRVL_DS_PWR_CFG
{
	NDIS_OID	Oid;
	UCHAR		Enable;
	UCHAR		P0;
	UCHAR		P1;
	UCHAR		P2;
} OID_MRVL_DS_PWR_CFG, *POID_MRVL_DS_PWR_CFG;

typedef struct _OID_MRVL_DS_TPC_CFG
{
	NDIS_OID	Oid;
	UCHAR		Enable;
	UCHAR		P0;
	UCHAR		P1;
	UCHAR		P2;
	UCHAR		UseSNR;
} OID_MRVL_DS_TPC_CFG,  *POID_MRVL_DS_TPC_CFG;



//ahan, [2005-12-13], for subscibe event TLV format ++dralee_20060217
typedef struct _OID_MRVL_DS_SUBSCRIBE_EVENT
{
	USHORT		Events;
	union{
	    MrvlIEtypes_EventTLV_t          TLVData;
		MrvlIEtypes_RssiParamSet_t		LowRssi;  
		MrvlIEtypes_SnrThreshold_t		LowSnr; 
		MrvlIEtypes_FailureCount_t		FailCnt;
		MrvlIEtypes_BeaconsMissed_t		BcnMiss;
		MrvlIEtypes_RssiParamSet_t		HighRssi;
		MrvlIEtypes_SnrThreshold_t		HighSnr;
	} Param;
} OID_MRVL_DS_SUBSCRIBE_EVENT, *POID_MRVL_DS_SUBSCRIBE_EVENT;
//ahan, [2005-12-13]    ++dralee_20060217

//tt --
#define OID_MRVL_GET_TSF						0xff01022a
#define OID_MRVL_WMM_ADDTS						0xff01022b
#define OID_MRVL_WMM_DELTS						0xff01022c
#define OID_MRVL_WMM_QUEUE_CONFIG				0xff01022d
#define OID_MRVL_WMM_QUEUE_STATS				0xff01022e
///new-assoc ++
/// This OID is identical with OID_802_11_SSID. However, we will not wait until getting the response
/// => Workaround for the system which may be block if not return immediately
#define OID_MRVL_ASSOC								0xff010280
///new-assoc --


#define MAX_LEN_OF_CHANNEL_LIST_STR         100
#define MAX_BG_SCAN_SSID                               6

#pragma pack(1) 

typedef struct _OID_MRVL_DS_RATE_ADAPT_RATESET
{
	USHORT		EnableHwAuto;
	USHORT		Bitmap;
    USHORT      Threshold;
    USHORT      FinalRate;
} OID_MRVL_DS_RATE_ADAPT_RATESET, *POID_MRVL_DS_RATE_ADAPT_RATESET;

typedef struct _REG_BG_SCAN_CONFIG
{
	ULONG       Enable; 
    	ULONG       ScanInterval;
    	ULONG       StoreCondition;	
    	ULONG       ReportCondition;	

    	ULONG       ScanPriority;

    	ULONG       NumProbes;
    	ULONG       BcastProbe;
    	ULONG       NumSSIDProbe;
    	ULONG       SnrValue;
    	ULONG       SnrFreq;
   	UCHAR       ChannelList[MAX_LEN_OF_CHANNEL_LIST_STR];
    	ULONG       ScanTime;
    	UCHAR       Ssids[MAX_BG_SCAN_SSID][NDIS_802_11_LENGTH_SSID]; // SSID string will be terminated by a 0 or it's len is equale to 32.
    
} REG_BG_SCAN_CONFIG, *PREG_BG_SCAN_CONFIG;

#define BG_SCAN_NORMAL 0
#define BG_SCAN_UAPSD  1


typedef struct _OID_MRVL_DS_BG_SCAN_CONFIG
{
	NDIS_OID	Oid;
	USHORT         CmdLen;

  USHORT         Action;

	UCHAR		Enable;	
	
	// bssType 
	// 1 - Infrastructure
	// 2 - IBSS
	// 3 - any 
	UCHAR		BssType;
	
	// ChannelsPerScan 
	// No of channels to scan at one scan 
	UCHAR		ChannelsPerScan;
	
	// 0 - Discard old scan results
	// 1 - Discard new scan results 
	UCHAR		DiscardWhenFull;
	
	USHORT  	Reserved;
	
	// ScanInterval 
	ULONG		ScanInterval;
	
	// StoreCondition 
	// - SSID Match
	// - Exceed RSSI threshold
	// - SSID Match & Exceed RSSI Threshold 
	// - Always 
	ULONG		StoreCondition;	
	
	// ReportConditions 
	// - SSID Match
	// - Exceed RSSI threshold
	// - SSID Match & Exceed RSSIThreshold
	// - Exceed MaxScanResults
	// - Entire channel list scanned once 
	// - Domain Mismatch in country IE 
	ULONG		ReportConditions;	

	// MaxScanResults 
	// Max scan results that will trigger 
	// a scn completion event 
	USHORT		MaxScanResults;
	
	// attach TLV based parameters as needed, e.g.
	// MrvlIEtypes_SsIdParamSet_t 		SsIdParamSet;
	// MrvlIEtypes_ChanListParamSet_t	ChanListParamSet;
	// MrvlIEtypes_NumProbes_t		NumProbes;
	UCHAR 		TlvData[1];
} OID_MRVL_DS_BG_SCAN_CONFIG, *POID_MRVL_DS_BG_SCAN_CONFIG;

#pragma pack() 

typedef struct _OID_MRVL_DS_BG_SCAN_QUERY
{
    NDIS_OID    	Oid;
   	UCHAR        	Flush;
} OID_MRVL_DS_BG_SCAN_QUERY, *POID_MRVL_DS_BG_SCAN_QUERY;   

typedef struct _OID_MRVL_DS_POWER_ADAPT_CFG_EXT
{
	NDIS_OID	Oid;
	USHORT         CmdLen;
	USHORT         Action;
	USHORT		EnablePA;	
	
	MrvlIEtypes_PowerAdapt_Group_t  PowerAdaptGroup;
} OID_MRVL_DS_POWER_ADAPT_CFG_EXT, *POID_MRVL_DS_POWER_ADAPT_CFG_EXT;


typedef struct _OID_MRVL_DS_KEY_ENCRYPT
{
	//NDIS_OID	Oid;
    USHORT      EncType;
    UCHAR       KeyIV[16];
    UCHAR       KeyEncKey[16];
    USHORT      KeyDataLen;
    UCHAR       KeyData[512];
} OID_MRVL_DS_KEY_ENCRYPT, *POID_MRVL_DS_KEY_ENCRYPT;


typedef struct _OID_MRVL_DS_CRYPTO
{
	//NDIS_OID	Oid;
	USHORT      EncDec;         //Decrypt=0, Encrypt=1
    USHORT      Algorithm;	    //RC4=1, AES=2, AES_KEY_WRAP=3
	USHORT      KeyIVLength;    //Length of Key IV (bytes)		
    UCHAR       KeyIV[32];		//Key IV
	USHORT      KeyLength;		//Length of Key (bytes)
	UCHAR       Key[32];		//Key 
    MrvlIEAesCrypt_t TLVBuf;    //++dralee_20060421
	//UCHAR       DataBuf[1024];
} OID_MRVL_DS_CRYPTO, *POID_MRVL_DS_CRYPTO;


typedef struct _OID_MRVL_802_11D_CFG
{
  USHORT enable;
} OID_MRVL_DS_802_11D_CFG, *POID_MRVL_DS_802_11D_CFG;  

//dralee_20060509
typedef struct _OID_MRVL_802_11_GET_LOG_t
{
    ULONG   mcasttxframe;
    ULONG   failed;
    ULONG   retry;
    ULONG   multiretry;
    ULONG   framedup;
    ULONG   rtssuccess;
    ULONG   rtsfailure;
    ULONG   ackfailure;
    ULONG   rxfrag;
    ULONG   mcastrxframe;
    ULONG   fcserror;
    ULONG   txframe;
    ULONG   wepundecryptable;
} OID_MRVL_DS_802_11_GET_LOG, *POID_MRVL_DS_802_11_GET_LOG; 
  
//dralee_20060509
typedef struct _OID_MRVL_DS802_11_BAND_CFG_t
{
   	USHORT		BandSelection; 	
	USHORT		Channel;  
} OID_MRVL_DS_802_11_BAND_CFG, *POID_MRVL_DS_802_11_BAND_CFG;


typedef struct _OID_MRVL_DS_GET_TX_RATE
{
	USHORT      usTxRate;       
} OID_MRVL_DS_GET_TX_RATE, *POID_MRVL_DS_GET_TX_RATE;

typedef struct _OID_MRVL_DS_GET_RX_INFO
{
       SHORT       RXPDSNR;  
       USHORT      RXPDRate;  
} OID_MRVL_DS_GET_RX_INFO, *POID_MRVL_DS_GET_RX_INFO;
//tt --
//dralee_20060601 
typedef struct _OID_MRVL_DS_IBSS_COALESCING_CFG
{
   	USHORT		Enable;
} OID_MRVL_DS_IBSS_COALESCING_CFG, *POID_MRVL_DS_IBSS_COALESCING_CFG;
    
//dralee_20060629 enable/disable PPS
typedef struct _OID_MRVL_PROPRIETARY_PERIODIC_PS
{
  USHORT enable;
} OID_MRVL_DS_PROPRIETARY_PERIODIC_PS, *POID_MRVL_DS_PROPRIETARY_PERIODIC_PS;  

typedef struct _OID_DS_MRVL_ADHOC_CHANNEL
{
    ULONG		ulChannelNumber;	
} OID_DS_MRVL_ADHOC_CHANNEL, *POID_DS_MRVL_ADHOC_CHANNEL;


#pragma pack(1)

typedef struct _OID_MRVL_DS_SCAN_EXT {
	USHORT  Channel;
	USHORT  MaxDuration;
	USHORT  MinDuration;
} OID_MRVL_DS_SCAN_EXT, *POID_MRVL_DS_SCAN_EXT;

typedef struct _OID_DS_MRVL_ADHOC_AWAKED_PERIOD
{
  USHORT AdhocAwakePeriod;
} OID_MRVL_DS_ADHOC_AWAKED_PERIOD, *POID_MRVL_DS_ADHOC_AWAKED_PERIOD;  

typedef struct _OID_MRVL_DS_HOSTSLEEP_CANCEL
{ 
   USHORT cancel;
} OID_MRVL_DS_HOSTSLEEP_CANCEL;

typedef struct _OID_MRVL_DS_SDIO_INT_CFG
{
   USHORT pin;
   USHORT edge;
   USHORT width;
} OID_MRVL_DS_GPIO_SDIO_INT_CFG, *POID_MRVL_DS_GPIO_SDIO_INT_CFG;

//022607
typedef struct _OID_MRVL_DS_BCA_TIMESHARE
{
   USHORT TrafficType;
   ULONG TimeShareInterval;
   ULONG BTTime;
} OID_MRVL_DS_BCA_TIMESHARE, *POID_MRVL_DS_BCA_TIMESHARE;





#pragma pack()


#endif // _MRVL_OID_H_
