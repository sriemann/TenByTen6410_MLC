/******************* (c) Marvell Semiconductor, Inc., *************************
 *
 *     
 *  This file contains definitions and data structures that are specific
 *  to Marvell 802.11 NIC. 
 *
 *  $Author: schiu $
 *
 *	$Date: 2004/12/15 $
 *
 *	$Revision: #18 $   	     
 *
 *****************************************************************************/

#ifndef _DSDEF_H_
#define _DSDEF_H_

//===============================================================================
//          PUBLIC TYPE DEFINITIONS
//===============================================================================

// ***************** DO NOT MODIFY *****************
// #define MRVL_AUTOMATED_BUILD_VERSION
// ***************** END DO NOT MODIFY *************

/* ==============================================
	<< Version nameing rule >>
   ==============================================
* Main stream: Use the old way as usual
* Branchs: digits:  [aa].[bb].[cc].p[dd]
-          aa: The id for customer project. reference: CustomID_Parameters.xls
-          bb.cc: The version number of the main stream, which this project is branched from.
-          dd: The patchs

Example, The zune branch id = 1. The branch is from v38.p25. The number will be: 
	1.38.25.p1 for the 1st patch. 
	1.38.25.p2 for the 2nd patch.
	... etc
*/

//36.p0 is sourced from 36.p25
#define MRVDRV_DRIVER_BUILD_VERSION                 "38.p35"

#define MRVDRV_NUM_OF_WEP_INFO                      16
// Number of RSSI values to keep for roaming
#define MRVDRV_NUM_RSSI_VALUE                       10

#define MRVDRV_MFG_CMD_LEN                          512

// number of packet in RX queue
#define MRVDRV_NUM_RX_PKT_IN_QUEUE                  20

#define MRVDRV_MEMORY_TAG                           'lvrm'   

#define NUM_OF_WMM_PRIORITY_LEVEL                   4

//
//          Ethernet Frame Sizes
//
//          Currently, Microsoft passes 802.3/Ethernet packet to the NDIS miniport driver,
//          Driver allocates 1536 bytes of host memory to keep Tx/Rx packets.
//
#define MRVDRV_ETH_ADDR_LEN							6
#define MRVDRV_ETH_HEADER_SIZE						14
#define MRVL_DEFAULT_INITIAL_RSSI					-200

// Threshold is 500kbps
#define MRVL_SCAN_ALLOW_THRESHOLD					500000/8    

// when connected, the driver will try to scan every 60s if there is no traffic
#define MRVL_BACKGOUND_SCAN_TIME					30

//
//          Buffer Constants
//
//          The size of SQ memory PPA, DPA are 8 DWORDs, that keep the physical
//          addresses of WCB buffers. Station has only 8 WCB available, Whereas
//          NDIS miniport driver has more local WCBs(32). Each WAB on the host memory
//          is associated with a Tx control node. NDIS miniport driver maintains 8 
//          RxPD descriptors for station firmware to store Rx packet information.
//
//          Current version of MAC has a 32x6 multicast address buffer.
//
//          802.11b can have up to  14 channels, NDIS miniport driver keeps the
//          BSSID(MAC address) of each APs or Ad hoc stations it has sensed.
//
#define MRVDRV_MAX_MULTICAST_LIST_SIZE   0x00000020
#define MRVDRV_SIZE_OF_CMD_BUFFER        0x00000c00 // 3K
#define MRVDRV_NUM_OF_CMD_BUFFER         0x00000040
//
//          Timer constants
//
//          This section defines time-out intervals for different tasks
//
#define MRVDRV_TEN_SECS_TIMER_INTERVAL     	10000     // 10/14/02 - tune to 10 seconds

#define MRVDRV_DEFAULT_TIMER_INTERVAL       10000     // 10/14/02 - tune to 10 seconds

// most command should finish within 0.5 second
#define MRVDRV_DEFAULT_COMMAND_TIME_OUT	            500      
#define MRVDRV_DEFAULT_LONG_COMMAND_TIME_OUT	    10000      // 11/04/02 - tune to 10 seconds

#define MRVDRV_ASSOCIATION_TIME_OUT      195 

// disconnect if more than one MIC error occurred within the following period (60s)
#define MRVDRV_MIC_ERROR_PERIOD          60000   

// plus_clean_up
#if DBG
#define MRVDRV_DEFAULT_TX_PKT_TIME_OUT   1000         // lot's of time for debug
#else                                        
#define MRVDRV_DEFAULT_TX_PKT_TIME_OUT   1000 //300  , dralee 072705     
#endif // end of Debug

    /*
        The new tx timeout mechanism. The new timeout value will be around
        MRVL_CHECK_FOR_HANG_TIME (second) * MRVDRV_TX_TIMEOUT_COUNT_THRESHOLD
    */
#define MRVDRV_TX_TIMEOUT_COUNT_THRESHOLD   3

#define MRVDRV_DEFAULT_INT_WORKAROUND_TIMEOUT  2000


// for Adapter->Initialization Status flags
#define MRVDRV_INIT_STATUS_CMDTIMER_INITALIZED    0x1
#define MRVDRV_INIT_STATUS_MAP_REGISTER_ALLOCATED 0x2

//
//          Misc constants
//
//          This section defines 802.11 specific contants
//
#define MRVDRV_DEFAULT_MACHINE_NAME_SIZE    32
#define MRVDRV_MAX_SSID_LENGTH			    32
#define MRVDRV_MAX_BSS_DESCRIPTS			16
#define MRVDRV_MAX_REGION_CODE              6
#define MRVDRV_MAX_CHANNEL_NUMBER		    14
#define MRVDRV_DEFAULT_LISTEN_INTERVAL	    0

// We define 8 levels in TX power command
#define MRVDRV_TX_POWER_LEVEL_TOTAL         0x0008 

// definition for power level high, mid, and low
#define MRVDRV_TX_POWER_LEVEL_HIGH          19
#define MRVDRV_TX_POWER_LEVEL_MID           10
#define MRVDRV_TX_POWER_LEVEL_LOW           5

#define MRVDRV_MAX_BSSID_LIST			      0x40  //34.p1  //0x20   

// reserve 400 bytes for each AP's variable IE in the scan list
#define MRVDRV_SCAN_LIST_VAR_IE_SPACE       400

// set aside 500 bytes for the association information buffer
#define MRVDRV_ASSOCIATE_INFO_BUFFER_SIZE   500

// max time between awake in ms, used by check for hang
#define MRVL_MAX_TIME_BETWEEN_AWAKE         5000

#define BGACTIVE_ROAMING_PERIOD_TIME   120000   // 120 sec

	/*
		The original value is 1. But it takes a longer time and affects wpa(2) handshakes.
	*/
#define PSCAN_NUM_CH_PER_SCAN           3 // tt wled

#define MAX_LIST_BUFFER_SIZE                  8192
//
//          Resource size
//
//          NDIS miniport driver (driver) has 2 ways to get BAR from the bus. The 
//          driver can either look at the PCI configuration BAR directly or query 
//          CM_PARTIAL_RESOURCE_DESCRIPTOR to get BAR information, which requires 
//          a buffer to keep resource list.
//
#define MRVDRV_RESOURCE_BUF_SIZE         (sizeof(NDIS_RESOURCE_LIST) + \
                                            (10*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)))

//
//          Define WCB, PPA, DPA page mask
//
//          SQ memory on MrvDrv chip starts at address 0xC00000000
//

//
// WCB Status
//
//           Station firmware use WCB status field to report final Tx transmit result,
//           Bit masks are used to present combined situations.
//
#define MRVDRV_WCB_STATUS_IDLE                   0x00000000
#define MRVDRV_WCB_STATUS_USED                   0x00000001 // for WCBRefNode to use
#define MRVDRV_WCB_STATUS_OK                     0x00000001
#define MRVDRV_WCB_STATUS_OK_RETRY               0x00000002
#define MRVDRV_WCB_STATUS_OK_MORE_RETRY          0x00000004
#define MRVDRV_WCB_STATUS_MULTICAST_TX           0x00000008
#define MRVDRV_WCB_STATUS_BROADCAST_TX           0x00000010
#define MRVDRV_WCB_STATUS_FAILED_LINK_ERROR      0x00000020
#define MRVDRV_WCB_STATUS_FAILED_EXCEED_LIMIT    0x00000040
#define MRVDRV_WCB_STATUS_FAILED_AGING           0x00000080

// in extended scan mode, because there will be 
// multiple outstanding scan, this is the way to tell
// whether a command was specific scan or not
// in pad[0]
#define MRVDRV_ES_SPECIFICSCAN                  0x01
#define MRVDRV_ES_NOTSPECIFICSCAN               0x00

// in pad[1]
#define MRVDRV_ES_ASSOCIATIONBLOCKED            0x01
#define MRVDRV_ES_ASSOCIATIONNOTBLOCKED         0x00


//
// Tx control node status
//
//          For miniport driver internal use
//
#define MRVDRV_TX_CTRL_NODE_STATUS_IDLE      0x0000
#define MRVDRV_TX_CTRL_NODE_STATUS_USED      0x0001
#define MRVDRV_TX_CTRL_NODE_STATUS_PENDING   0x0002

//
// RxPD Control
//
//          RxPD control field is used to keep ownership information
//
#define MRVDRV_RXPD_CONTROL_DRIVER_OWNED     0x00
#define MRVDRV_RXPD_CONTROL_OS_OWNED         0x04
#define MRVDRV_RXPD_CONTROL_DMA_OWNED        0x80

//
// RxPD Status
//
//          Station firmware (FW) use RxPD Status field to report Rx packet type
//
#define MRVDRV_RXPD_STATUS_IDLE              0x0000
#define MRVDRV_RXPD_STATUS_OK                0x0001
#define MRVDRV_RXPD_STATUS_MULTICAST_RX      0x0002
#define MRVDRV_RXPD_STATUS_BROADCAST_RX      0x0004
#define MRVDRV_RXPD_STATUS_FRAGMENT_RX       0x0008

//
// Command control node status
//
//          Miniport driver use status field to maintian command control nodes
//
#define MRVDRV_CMD_CTRL_NODE_STATUS_IDLE        0x0000
#define MRVDRV_CMD_CTRL_NODE_STATUS_PENDING     0x0001
#define MRVDRV_CMD_CTRL_NODE_STATUS_PROCESSING  0x0002

#define MRVDRV_FW_STATUS_READY                  0x5A
#define MRVDRV_FW_DL_RDY_FOR_NEXT_BLOCK         0xAA
//
// Link spped
//
//          Microsoft defines the unit of link speed measurement to be 100 bps
//
#define MRVDRV_LINK_SPEED_10mbps         100000    


#define MRVDRV_LINK_SPEED_B_RATES		1

#define MRVDRV_LINK_SPEED_DEFAULT        MRVDRV_LINK_SPEED_11mbps

#define MRVDRV_LINK_SPEED_0mbps          0
#define MRVDRV_LINK_SPEED_1mbps          10000 // in unit of 100bps
#define MRVDRV_LINK_SPEED_2mbps          20000
#define MRVDRV_LINK_SPEED_5dot5mbps      55000
#define MRVDRV_LINK_SPEED_6mbps          60000   
#define MRVDRV_LINK_SPEED_9mbps          90000
#define MRVDRV_LINK_SPEED_10mbps         100000 
#define MRVDRV_LINK_SPEED_11mbps         110000
#define MRVDRV_LINK_SPEED_12mbps         120000
#define MRVDRV_LINK_SPEED_18mbps         180000
#define MRVDRV_LINK_SPEED_22mbps         220000
#define MRVDRV_LINK_SPEED_24mbps         240000
#define MRVDRV_LINK_SPEED_33mbps         330000
#define MRVDRV_LINK_SPEED_36mbps         360000
#define MRVDRV_LINK_SPEED_48mbps         480000
#define MRVDRV_LINK_SPEED_54mbps         540000
#define MRVDRV_LINK_SPEED_72mbps         720000

#define MRVDRV_LINK_SPEED_100mbps        1000000

//
// RSSI-related defines
//
//          RSSI constants are used to implement NDIS 5.1 802.11 RSSI threshold 
//          indication. if the Rx packet signal got too weak for 5 consecutive times,
//          miniport driver (driver) will report this event to wrapper
//

#define SNR_BEACON		0
#define SNR_RXPD		    1
#define NF_BEACON		2
#define NF_RXPD			3

/* MACRO DEFINITIONS */
#define CAL_NF(NF)			((LONG)(-(LONG)(NF)))
#define CAL_RSSI(SNR, NF) 	((LONG)((UCHAR)(SNR) + CAL_NF(NF)))
#define SCAN_RSSI(RSSI)		(0x100 - ((UCHAR)(RSSI)))

#define DEFAULT_BCN_AVG_FACTOR       8
#define DEFAULT_DATA_AVG_FACTOR     8
#define AVG_SCALE			100
#define QUERY_RSSI_TIMER           3000
#define CAL_AVG_SNR_NF(AVG, SNRNF, N)	(((AVG) == 0) ? ((USHORT)(SNRNF) * AVG_SCALE) : \
					                    ((((int)(AVG) * (N-1)) + ((USHORT)(SNRNF) * \
						                 AVG_SCALE)) /N))

  
/* Definitions for SNR and NF */
typedef enum _SNRNF_TYPE {
    TYPE_BEACON = 0,
    TYPE_RXPD,
    MAX_TYPE_B
} SNRNF_TYPE;

typedef enum _SNRNF_DATA {
    TYPE_NOAVG = 0,
    TYPE_AVG,
    MAX_TYPE_AVG
} SNRNF_DATA;


#define MRVDRV_RSSI_TRIGGER_DEFAULT      (-200)
#define MRVDRV_RSSI_INDICATION_THRESHOLD 5
#define MRVDRV_RSSI_DEFAULT_NOISE_VALUE	0 

#define MRVL_CHECK_FOR_HANG_TIME        2

//Power Saving Mode related
//
 
// Send packet status
#define READY_TO_SEND	0
#define STOP_SENDING	1

#define PS_CAM_STATE	1
#define PS_HOST_CMD		2  

//dralee_20060307, Add for pripority PS 
#define MRVDRV_WCB_POWER_MGMT_NULL_PACKET 0x01
#define MRVDRV_WCB_POWER_MGMT_LAST_PACKET 0x08

 
  
//
// Define WCB, PPA, DPA and RxPD
//

#pragma pack(1)
//          WCB descriptor
//
//          Status                : Current Tx packet transmit status
//          PktPtr                : Physical address of the Tx Packet on host PC memory
//          PktLen                : Tx packet length
//          UCHAR DestMACAdrHi[2] : First 2 byte of destination MAC address
//          DestMACAdrLow[4]      : Last 4 byte of destination MAC address
//          DataRate              : Driver uses this field to specify data rate for the Tx packet
//          NextWCBPtr            : Address to next WCB (Used by the driver)
//
typedef struct _WCB 
{
    ULONG Status;
    ULONG TxControl;
    // Packet offset in CF
    ULONG PktPtr;
    USHORT PktLen;
    UCHAR DestMACAdrHi[2];
    UCHAR DestMACAdrLow[4];
    UCHAR Priority;
    UCHAR PowerMgmt;
    UCHAR Reserved[2];

} WCB, *PWCB;

//
// RxPD descirptor
//			
// Status      : Rx packet reception status 
// RSSI        : Receive RF signal strength for this packet (dbm)
// Control     : Not used in CF and SDIO 
// PktLen      : Number of bytes in the payload  
// NF   	   : Noise floor for this packet
// Rate        : Rate at which this packet is received 
// PktPtr      : Offset from the start of the packet to the begining of the payload data
// NextRxPDPtr : Not used in CF and SDIO 
//
typedef struct _RxPD
{
    USHORT   Status;
    UCHAR    SNR;     
    UCHAR    Control;    
    USHORT   PktLen; 
    UCHAR    NF;       
    UCHAR    RxRate;
    ULONG    PktPtr;
    // Based On TX/RX PD Document Ver1.1
    UCHAR    RxPacketType;
    UCHAR    Reserved_1[3];
    UCHAR    Priority;
    UCHAR    Reserved[3];
} RxPD, *PRxPD;

#pragma pack()

// code_clean_up
// PJG: get these to match FW and then fix all usage.
#define MRVDRV_MAXIMUM_ETH_PACKET_SIZE             1514  // PJG: 
#define CF_MAX_PACKET_SIZE (MRVDRV_MAXIMUM_ETH_PACKET_SIZE - MRVDRV_ETH_HEADER_SIZE)
///crlo: defined MTU to report to the TCP layer
#define MRVDRV_MTU								1500
#define MRVDRV_MTU_TOTAL						(MRVDRV_MTU+MRVDRV_ETH_HEADER_SIZE)

// TODO: Check why it is necessary to add extra 38 bytes

#define WZC_Default                 0
#define WZC_Infrastructure_Mode     1
#define WZC_Adhoc_Mode              2
#define WZC_Ignore_Send_EAPOL_START 3

#define MRVDRV_ETH_TX_PACKET_BUFFER_SIZE (MRVDRV_MAXIMUM_ETH_PACKET_SIZE + sizeof(WCB)+SDIO_EXTENDED_IO_BLOCK_SIZE*2)
#define MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE (sizeof(RxPD)+4)
#define MRVDRV_ETH_RX_PACKET_BUFFER_SIZE (MRVDRV_MAXIMUM_ETH_PACKET_SIZE + MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE + SDIO_EXTENDED_IO_BLOCK_SIZE*2)
//
//          Tx control node data structure 
//
//          Tx control node is used by the driver to keep ready to send Tx packet information.
//
//          Status         :
//          NPTxPacket     : NDIS_PACKET reference
//          LocalWCB       : Local WCB reference
//          BufVirtualAddr : Tx staging buffer logical address
//          BufPhyAddr     : Tx staging buffer physical address
//          NextNode       : link to next Tx control node
//
#pragma pack(1)
typedef struct _TxCtrlNode
{
    ULONG                 Status;
    PNDIS_PACKET          NPTxPacket;
    PWCB                  LocalWCB;
    PUCHAR                BufVirtualAddr;
    NDIS_PHYSICAL_ADDRESS BufPhyAddr;
    struct _TxCtrlNode    *NextNode;
} TxCtrlNode, *PTxCtrlNode;

//
//          Command control nore data structure, command data structures are defined in
//          hostcmd.h
//
//          Next              : Link to nect command control node
//          Status            : Current command execution status
//          PendingOID        : Set or query OID passed from NdisRequest()
//          ExpectedRetCode   : Result code for the current command
//          PendingInfo       : Init, Reset, Set OID, Get OID, Host command, etc.
//          INTOption         : USE_INT or NO_INT
//          BatchQNum         : Reserved for batch command processing
//          IsLastBatchCmd    : Reserved for batch command processing
//          BytesWritten      : For async OID processing
//          BytesRead         : For async OID processing
//          BytesNeeded       : For async OID processing
//          InformationBuffer : For async OID processing
//          BufVirtualAddr    : Command buffer logical address
//          BufPhyAddr        : Command buffer physical address
//
typedef struct _CmdCtrlNode
{
    struct _CmdCtrlNode  *Next;
    ULONG                 Status;
    NDIS_OID              PendingOID;
    USHORT                ExpectedRetCode;
    USHORT                PendingInfo;       // Init, Reset, Set OID, Get OID, Host command, etc.
    USHORT                INTOption;
    USHORT                PriorityPass;      // Is First Priority Command
    BOOLEAN               IsLastBatchCmd;    // BOOLEAN is defined as unsigned char
    UCHAR                 Pad[3];            // To make it on 4 byte boundary
    PULONG                BytesWritten;      // For async OID processing
    PULONG                BytesRead;         // For async OID processing
    PULONG                BytesNeeded;       // For async OID processing
    PVOID                 InformationBuffer; // For async OID processing
    PUCHAR                 BufVirtualAddr;
    NDIS_PHYSICAL_ADDRESS BufPhyAddr;
    UCHAR                 PadExt[4];
    USHORT		   CmdFlags;          // Flags for driver Host Command
    UCHAR                 Pad2[2];           // To make it on 4 byte boundary
} CmdCtrlNode, *PCmdCtrlNode;

#define	CMD_F_HOSTCMD		(1 << 0)

// for packet Q
typedef struct _PacketQueueNode
{
    struct _PacketQueueNode     *Next;
    PNDIS_PACKET                pPacket;
} PACKET_QUEUE_NODE, *PPACKET_QUEUE_NODE;



//
//          Ethernet Frame Structure
//

//          Ethernet MAC address
typedef struct _ETH_ADDRESS_STRUC
{
     UCHAR EthNodeAddr[MRVDRV_ETH_ADDR_LEN];
} ETH_ADDR_STRUC, *PETH_ADDR_STRUC;


//          Ethernet frame header
typedef struct _ETH_HEADER_STRUC 
{
     UCHAR DestAddr[MRVDRV_ETH_ADDR_LEN];
     UCHAR SrcAddr[MRVDRV_ETH_ADDR_LEN];
     USHORT TypeLength;
} ETH_HEADER_STRUC, *PETH_HEADER_STRUC;

typedef struct _REGINFOTAB 
{
    NDIS_STRING ObjNSName;  // Object name (UNICOODE)
    char *ObjName;          // Object name (ASCII)
    UINT Type;              // StringData (1), IntegerData (0)
    UINT Offset;            // Offset to adapter object field
    UINT MaxSize;           // Maximum Size (in bytes) 
} REGINFOTAB;

#pragma pack()

//      11/08/02 - Add WEP list data structures
#define MRVL_KEY_BUFFER_SIZE_IN_BYTE  16 

typedef enum _GET_PACKET_STATUS
{
    GPS_SUCCESS = 0,    // get packet was successful
    GPS_RESOURCE,       // get packet was successful, but number of free packet is low
    GPS_FAILED          // unable to get packet
} MRVDRV_GET_PACKET_STATUS;

// Based on NDIS_802_11_WEP, we extend the WEP buffer length to 128 bits
#pragma pack(1)

typedef struct _MRVL_WEP_KEY
{ 
    ULONG Length;
    ULONG KeyIndex;
    ULONG KeyLength;
    UCHAR KeyMaterial[MRVL_KEY_BUFFER_SIZE_IN_BYTE];
} MRVL_WEP_KEY, *PMRVL_WEP_KEY;

#pragma pack()


// support 5 key sets
#define MRVL_NUM_WPA_KEYSET_SUPPORTED   5

// support 4 keys per key set
#define MRVL_NUM_WPA_KEY_PER_SET        4

// max key length is 32 bytes acording to current WPA document
#define MRVL_MAX_WPA_KEY_LENGTH         32

#pragma pack(1)

typedef struct _MRVL_WPA_KEY
{
    ULONG           KeyIndex;           
    ULONG           KeyLength;          // length of key in bytes
    NDIS_802_11_KEY_RSC KeyRSC;
    // variable length depending on above field
    UCHAR           KeyMaterial[MRVL_MAX_WPA_KEY_LENGTH];     
} MRVL_WPA_KEY, *PMRVL_WPA_KEY;

typedef struct _MRVL_WPA_KEY_SET
{
    // BSSID is common within the set
    NDIS_802_11_MAC_ADDRESS     BSSID;
    MRVL_WPA_KEY                Key[MRVL_NUM_WPA_KEY_PER_SET];
} MRVL_WPA_KEY_SET, *PMRVL_WPA_KEY_SET;

typedef struct _MRVL_NDIS_WPA_KEY 
{
    UCHAR   EncryptionKey[16];
    UCHAR   MICKey1[8];
    UCHAR   MICKey2[8];
} MRVL_NDIS_WPA_KEY, *PMRVL_NDIS_WPA_KEY;

#pragma pack()

// Fixed IE size is 8 bytes time stamp + 2 bytes beacon interval + 2 bytes cap 
#define     MRVL_FIXED_IE_SIZE      12


#define KEY_INFO_ENABLED	0x01

typedef enum {
	KEY_TYPE_ID_WEP = 0,
	KEY_TYPE_ID_TKIP,
	KEY_TYPE_ID_AES
} KEY_TYPE_ID;

typedef enum {
	KEY_INFO_WEP_DEFAULT_KEY = 0x01
} KEY_INFO_WEP;

typedef enum {
	KEY_INFO_TKIP_MCAST = 0x01,
	KEY_INFO_TKIP_UNICAST = 0x02,
	KEY_INFO_TKIP_ENABLED = 0x04
} KEY_INFO_TKIP;

typedef enum {
	KEY_INFO_AES_MCAST = 0x01,
	KEY_INFO_AES_UNICAST = 0x02,
	KEY_INFO_AES_ENABLED = 0x04
} KEY_INFO_AES;



#define WPA_AES_KEY_LEN 		16
#define WPA_TKIP_KEY_LEN 		32

#define WPA_CCKM_KEY_LEN 		13
#pragma pack(1)
typedef struct _MRVL_ADHOC_AES_KEY 
{
    ULONG   Length;
    UCHAR   KeyBody[32];  //assic form
} MRVL_ADHOC_AES_KEY, *PMRVL_ADHOC_AES_KEY;
#pragma pack()

/*
===============================================================================
                                 GLOBAL CONSTANT
===============================================================================
*/
static ULONG DSFeqList[15] = {
    0, 2412000, 2417000, 2422000, 2427000, 2432000, 2437000, 2442000, 
       2447000, 2452000, 2457000, 2462000, 2467000, 2472000, 2484000
};

// region code table
static USHORT RegionCodeToIndex[MRVDRV_MAX_REGION_CODE] = {0x10, 0x20, 0x30, 0x31, 0x32, 0x40};

// for Extended Channel List to work, all INVALID (0) channels must be grouped together
// and store at the end of the list
static UCHAR IEEERegionChannel[MRVDRV_MAX_REGION_CODE][MRVDRV_MAX_CHANNEL_NUMBER] = {
	{ 1, 2, 3, 4, 5, 6, 7,8, 9, 10, 11, 0, 0, 0},	//USA FCC
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 0, 0},	//Canada IC
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13, 0},	//Europe ETSI
	{10,11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	//Spain
	{10,11,12,13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	//France
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13, 14} };	//Japan MKK
//BugFix
//Ling++, 012706, Tx DataRate, modify MrvDrvSupportedRates: Add item 4 : 0x00,// reserved
	///BG-rate
	static const UCHAR MrvDrvSupportedRates[] = { 
       0x02,   // 1Mbps
    	0x04,   // 2Mbps
    	0x0b,   // 5.5Mbps
    	0x16,   // 11Mbps
    	0x00,   // reserced
    	0x0c,   // 6Mbps
    	0x12,   // 9Mbps
    	0x18,   // 12Mbps
    	0x24,   // 18Mbps
    	0x30,   // 24Mbps
    	0x48,   // 36Mbps
    	0x60,   // 48Mbps
    	0x6c,   // 54Mbps
    	0x00,   //reserved
	};
#define MRVDRV_NUM_SUPPORTED_RATES   sizeof(MrvDrvSupportedRates)

#ifndef ETH_ADDR_LENGTH
#define ETH_ADDR_LENGTH 6
#endif

// Structure describing Registry values
#pragma pack(1)
typedef struct _REG_VALUE_DESCR 
{
    LPWSTR val_name;
    DWORD  val_type;
    PBYTE  val_data;
	} REG_VALUE_DESCR, *PREG_VALUE_DESCR;
#pragma pack()

/// It's possible that the size of the beacon-table is big (more than 1550)
/// related information: There is an limitation to the AP items on the list
///		ref: CCX specification 2.8, S32, the beacon-table's AP item is limited 
///		to 30. But there is no limitation to the table.
#define SDIO_MAX_PKT_LEN            4096

#define SDIO_MAX_CMD_LEN            1024
#pragma pack(1)

typedef struct _SDIO_TX_PKT_T
{
    WCB         Wcb;
    UCHAR       Pkt[SDIO_MAX_PKT_LEN];
} SDIO_TX_PKT_T, *PSDIO_TX_PKT_T;

typedef struct _SDIO_TX_PKT
{
    // lenth of the sdio communication packet
    USHORT  Len;

    // type of the sdio communication packet
    USHORT  Type;

    union 
    {
        SDIO_TX_PKT_T  TxDataBuf;
        UCHAR          CmdBuf[SDIO_MAX_CMD_LEN];
    } Buf;
} SDIO_TX_PKT, *PSDIO_TX_PKT;

#pragma pack()

// to calculate the SDIO packet length, len is the original length
#define ADD_SDIO_PKT_HDR_LENGTH(len) len + sizeof(USHORT) + sizeof(USHORT)

#pragma pack(1)

typedef struct _PENDING_OID {
    BOOLEAN     bIsPendingOID;
    NDIS_OID    PendingOID;
    PVOID 		InformationBuffer;
	ULONG 		InformationBufferLength;
	PULONG 		BytesReadWrite;
	PULONG 		BytesNeeded;
} PENDING_OID, *PPENDING_OID;

#pragma pack()


#define MRVDRV_802_11_BAND_B 		0x00  
#define MRVDRV_802_11_BAND_BG		0x01
#define MRVDRV_802_11_BAND_A		0x02

#define MRVDRV_SCAN_CMD_START       0x20
#define MRVDRV_SCAN_CMD_END         0x10

#define MAX_CHAN_NUM			255

#define UNIVERSAL_REGION_CODE	0xff

#define MRVDRV_MAX_SUBBAND_802_11D		83 

#define COUNTRY_CODE_LEN	3   //3 
#define MAX_NO_OF_CHAN 		40

#define OID_802_11D_ENABLE      0x00008020
#pragma pack(1)

typedef struct _CHANNEL_FREQ_POWER 
{
	USHORT	Channel;	  // Channel Number		
	ULONG	Freq;		  // Frequency of this Channel	
	USHORT	MaxTxPower;	  // Max allowed Tx power level	

} CHANNEL_FREQ_POWER, *PCHANNEL_FREQ_POWER;
#pragma pack()

#define	MAX_REGION_BAND_NUM		2	
#define MAX_PSCAN_CH_NUM        34  

#pragma pack(1)
typedef	struct _REGION_CHANNEL 
{
	BOOLEAN	Valid;	  	  // TRUE if this entry is valid	
	UCHAR	Region;		  // Region code for US, Japan ...	     
	UCHAR	Band;		  // B, G, or A, used for BAND_CONFIG cmd	
	UCHAR	NrCFP;		  // Actual No. of elements in the array below 
	CHANNEL_FREQ_POWER	*CFP;            // Pointer to CFP table
	UCHAR   ScanType[MAX_PSCAN_CH_NUM];  // ACTIVE=0, PASSIVE=1 
	UCHAR   TxPower[MAX_PSCAN_CH_NUM];   // Tx Power Level for channel
	
} REGION_CHANNEL, *PREGION_CHANNEL;

//
//  Format { Channel, Frequency (MHz), MaxTxPower } 
// 
#define TX_PWR_DEFAULT	10

typedef struct _region_code_mapping 
{
	CHAR	region[COUNTRY_CODE_LEN];
	UCHAR	code;
} region_code_mapping_t;

#pragma pack()

static region_code_mapping_t region_code_mapping[] =
{
	    { "US", 0x10 }, /* US FCC	*/
    	{ "CA", 0x10 }, /* IC Canada	*/ 
    	{ "SG", 0x10 }, /* Singapore	*/
    	{ "EU", 0x30 }, /* ETSI	*/
    	{ "AU", 0x30 }, /* Australia	*/
    	{ "KR", 0x30 }, /* Republic Of Korea */
    	{ "ES", 0x31 }, /* Spain	*/
    	{ "FR", 0x32 }, /* France	*/
    	{ "JP", 0x40 } /* Japan	*/
};

/* Following 2 structure defines the supported channels */
static CHANNEL_FREQ_POWER	channel_freq_power_UN_BG[] = {
			{1,  2412, TX_PWR_DEFAULT}, 
			{2,  2417, TX_PWR_DEFAULT}, 
			{3,  2422, TX_PWR_DEFAULT}, 
			{4,  2427, TX_PWR_DEFAULT}, 
			{5,  2432, TX_PWR_DEFAULT}, 
			{6,  2437, TX_PWR_DEFAULT}, 
			{7,  2442, TX_PWR_DEFAULT}, 
			{8,  2447, TX_PWR_DEFAULT},
			{9,  2452, TX_PWR_DEFAULT}, 
			{10, 2457, TX_PWR_DEFAULT},
			{11, 2462, TX_PWR_DEFAULT}, 
			{12, 2467, TX_PWR_DEFAULT},
			{13, 2472, TX_PWR_DEFAULT}, 
			{14, 2484, TX_PWR_DEFAULT} 
};

static CHANNEL_FREQ_POWER	channel_freq_power_UN_AJ[] = {
//			{8,   5040, TX_PWR_DEFAULT}, 
//			{12,  5060, TX_PWR_DEFAULT}, 
//			{16,  5080, TX_PWR_DEFAULT},
//			{34,  5170, TX_PWR_DEFAULT}, 
			{36,  5180, TX_PWR_DEFAULT}, 
//			{38,  5190, TX_PWR_DEFAULT}, 
			{40,  5200, TX_PWR_DEFAULT}, 
//			{42,  5210, TX_PWR_DEFAULT}, 
			{44,  5220, TX_PWR_DEFAULT}, 
//			{46,  5230, TX_PWR_DEFAULT}, 
			{48,  5240, TX_PWR_DEFAULT}, 
			{52,  5260, TX_PWR_DEFAULT}, 
			{56,  5280, TX_PWR_DEFAULT}, 
			{60,  5300, TX_PWR_DEFAULT}, 
			{64,  5320, TX_PWR_DEFAULT}, 
			{100, 5500, TX_PWR_DEFAULT}, 
			{104, 5520, TX_PWR_DEFAULT}, 
			{108, 5540, TX_PWR_DEFAULT}, 
			{112, 5560, TX_PWR_DEFAULT}, 
			{116, 5580, TX_PWR_DEFAULT}, 
			{120, 5600, TX_PWR_DEFAULT}, 
			{124, 5620, TX_PWR_DEFAULT}, 
			{128, 5640, TX_PWR_DEFAULT}, 
			{132, 5660, TX_PWR_DEFAULT}, 
			{136, 5680, TX_PWR_DEFAULT}, 
			{140, 5700, TX_PWR_DEFAULT},
			{149, 5745, TX_PWR_DEFAULT}, 
			{153, 5765, TX_PWR_DEFAULT}, 
			{157, 5785, TX_PWR_DEFAULT},
			{161, 5805, TX_PWR_DEFAULT}, 
			{165, 5825, TX_PWR_DEFAULT}
//			{240, 4920, TX_PWR_DEFAULT}, 
//			{244, 4940, TX_PWR_DEFAULT}, 
//			{248, 4960, TX_PWR_DEFAULT}, 
//			{252, 4980, TX_PWR_DEFAULT}, 
};

// Band: 'B/G', Region: USA FCC/Canada IC
static CHANNEL_FREQ_POWER	channel_freq_power_US_BG[] = {
			{1,  2412, TX_PWR_DEFAULT}, 
			{2,  2417, TX_PWR_DEFAULT}, 
			{3,  2422, TX_PWR_DEFAULT}, 
			{4,  2427, TX_PWR_DEFAULT}, 
			{5,  2432, TX_PWR_DEFAULT}, 
			{6,  2437, TX_PWR_DEFAULT}, 
			{7,  2442, TX_PWR_DEFAULT}, 
			{8,  2447, TX_PWR_DEFAULT}, 
			{9,  2452, TX_PWR_DEFAULT}, 
			{10, 2457, TX_PWR_DEFAULT}, 
			{11, 2462, TX_PWR_DEFAULT}
};

// Band: 'B/G', Region: Europe ETSI
static CHANNEL_FREQ_POWER	channel_freq_power_EU_BG[] = {
			{1,  2412, TX_PWR_DEFAULT}, 
			{2,  2417, TX_PWR_DEFAULT}, 
			{3,  2422, TX_PWR_DEFAULT}, 
			{4,  2427, TX_PWR_DEFAULT}, 
			{5,  2432, TX_PWR_DEFAULT}, 
			{6,  2437, TX_PWR_DEFAULT}, 
			{7,  2442, TX_PWR_DEFAULT}, 
			{8,  2447, TX_PWR_DEFAULT}, 
			{9,  2452, TX_PWR_DEFAULT}, 
			{10, 2457, TX_PWR_DEFAULT}, 
			{11, 2462, TX_PWR_DEFAULT}, 
			{12, 2467, TX_PWR_DEFAULT}, 
			{13, 2472, TX_PWR_DEFAULT}
};

// Band: 'B/G', Region: Spain 
static CHANNEL_FREQ_POWER	channel_freq_power_SPN_BG[] = {
			{10, 2457, TX_PWR_DEFAULT},  
			{11, 2462, TX_PWR_DEFAULT} 
};

// Band: 'B/G', Region: France 
static CHANNEL_FREQ_POWER	channel_freq_power_FR_BG[] = {
			{10, 2457, TX_PWR_DEFAULT},  
			{11, 2462, TX_PWR_DEFAULT}, 
			{12, 2467, TX_PWR_DEFAULT}, 
			{13, 2472, TX_PWR_DEFAULT}
};

// Band: 'B/G', Region: Japan 
static CHANNEL_FREQ_POWER	channel_freq_power_JPN_BG[] = {
			{1,  2412, TX_PWR_DEFAULT}, 
			{2,  2417, TX_PWR_DEFAULT}, 
			{3,  2422, TX_PWR_DEFAULT}, 
			{4,  2427, TX_PWR_DEFAULT},  
			{5,  2432, TX_PWR_DEFAULT},  
			{6,  2437, TX_PWR_DEFAULT},  
			{7,  2442, TX_PWR_DEFAULT},  
			{8,  2447, TX_PWR_DEFAULT}, 
			{9,  2452, TX_PWR_DEFAULT}, 
			{10, 2457, TX_PWR_DEFAULT}, 
			{11, 2462, TX_PWR_DEFAULT}, 
			{12, 2467, TX_PWR_DEFAULT}, 
			{13, 2472, TX_PWR_DEFAULT},  
			{14, 2484, TX_PWR_DEFAULT}
};

// Band: 'A', Region: USA FCC, Canada IC, Spain, France 
static CHANNEL_FREQ_POWER	channel_freq_power_A[] = {
			{36,  5180, TX_PWR_DEFAULT},  
			{40,  5200, TX_PWR_DEFAULT}, 
			{44,  5220, TX_PWR_DEFAULT}, 
			{48,  5240, TX_PWR_DEFAULT},  
			{52,  5260, TX_PWR_DEFAULT}, 
			{56,  5280, TX_PWR_DEFAULT},  
			{60,  5300, TX_PWR_DEFAULT},  
			{64,  5320, TX_PWR_DEFAULT},  
			{149, 5745, TX_PWR_DEFAULT}, 
			{153, 5765, TX_PWR_DEFAULT},  
			{157, 5785, TX_PWR_DEFAULT}, 
			{161, 5805, TX_PWR_DEFAULT},  
			{165, 5825, TX_PWR_DEFAULT}
};
	
// Band: 'A', Region: Europe ETSI 
static CHANNEL_FREQ_POWER	channel_freq_power_EU_A[] = {
			{36,  5180, TX_PWR_DEFAULT}, 
			{40,  5200, TX_PWR_DEFAULT}, 
			{44,  5220, TX_PWR_DEFAULT}, 
			{48,  5240, TX_PWR_DEFAULT}, 
			{52,  5260, TX_PWR_DEFAULT}, 
			{56,  5280, TX_PWR_DEFAULT}, 
			{60,  5300, TX_PWR_DEFAULT}, 
			{64,  5320, TX_PWR_DEFAULT}, 
			{100, 5500, TX_PWR_DEFAULT}, 
			{104, 5520, TX_PWR_DEFAULT}, 
			{108, 5540, TX_PWR_DEFAULT}, 
			{112, 5560, TX_PWR_DEFAULT}, 
			{116, 5580, TX_PWR_DEFAULT}, 
			{120, 5600, TX_PWR_DEFAULT}, 
			{124, 5620, TX_PWR_DEFAULT}, 
			{128, 5640, TX_PWR_DEFAULT}, 
			{132, 5660, TX_PWR_DEFAULT}, 
			{136, 5680, TX_PWR_DEFAULT}, 
			{140, 5700, TX_PWR_DEFAULT}
};
	
// Band: 'A', Region: Japan 
static CHANNEL_FREQ_POWER	channel_freq_power_JPN_A[] = {
			{8,   5040, TX_PWR_DEFAULT}, 
			{12,  5060, TX_PWR_DEFAULT}, 
			{16,  5080, TX_PWR_DEFAULT},
			{34,  5170, TX_PWR_DEFAULT}, 
			{38,  5190, TX_PWR_DEFAULT}, 
			{42,  5210, TX_PWR_DEFAULT}, 
			{46,  5230, TX_PWR_DEFAULT}, 
			{240, 4920, TX_PWR_DEFAULT}, 
			{244, 4940, TX_PWR_DEFAULT}, 
			{248, 4960, TX_PWR_DEFAULT}, 
			{252, 4980, TX_PWR_DEFAULT}
};

static UCHAR	SupportedRates[G_SUPPORTED_RATES];
// First two rates are basic rates 
static UCHAR	SupportedRates_B[B_SUPPORTED_RATES] =
{ 0x82, 0x84, 0x8b, 0x96, 0, 0, 0, 0 };

// First four rates are basic rates 
static UCHAR	SupportedRates_G[G_SUPPORTED_RATES] =
{ 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c, 0, 0}; 

static UCHAR	SupportedRates_Basic[BASIC_SUPPORTED_RATES] =
{ 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c}; 

// First two rates are basic rates 
static UCHAR	SupportedRates_A[G_SUPPORTED_RATES] =
{ 0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, 0, 0, 0, 0, 0, 0 };


//dralee_20060529, copy from Linux  
//the rates supported in A mode for ad-hoc
static UCHAR	AdhocRates_A[G_SUPPORTED_RATES] =
		{ 0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, 0};
//the rates supported for ad-hoc G mode
static UCHAR	AdhocRates_G[G_SUPPORTED_RATES] =
		{ 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c, 0};
//the rates supported for ad-hoc B mode
static UCHAR	AdhocRates_B[4] =
		{ 0x82, 0x84, 0x8b, 0x96};


typedef struct _WPA_NOTIFY_OS
{
    NDIS_802_11_STATUS_TYPE             status;
    NDIS_802_11_AUTHENTICATION_REQUEST  request;
} WPA_NOTIFY_OS, *PWPA_NOTIFY_OS;


///String of the interface
#define IFSTRN		"SDIO"

///String of the baseband chip
#define CHIPSTRN	"8686"

///String of the RF chip
#define RFSTRN	"8015"

#endif _DSDEF_H_


