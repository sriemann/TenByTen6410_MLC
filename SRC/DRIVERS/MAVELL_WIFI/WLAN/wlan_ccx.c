/** @file wlan_ccx.c
 *  @brief This file contains data structures and functions
 *  for CCX standard
 *
 *  Copyright (c) Marvell Semiconductor, Inc., 2003-2005 
 */
/*********************************************************
Change log:
	09/27/05: add comments for Doxygen
*********************************************************/
#include "precomp.h"
#include "wlan_ccx.h"


// For WinCE +++
#define u8 UCHAR
#define s8 char
#define u16 USHORT
#define WLAN_STATUS_SUCCESS			(0)
#define cpu_to_le16(x) x
// For WinCE ---

#pragma pack(1)

// wlan_types.h +++

/**  Local Power Capability */
typedef struct _MrvlIEtypes_PowerCapability_t {
	MrvlIEtypesHeader_t	Header;
	s8 	MinPower;
	s8	MaxPower;
} MrvlIEtypes_PowerCapability_t;
// wlan_types.h ---

/**  OUI Types encoded in the CCX Vendor specific IE. 
 */
typedef enum
{
    CCX_OUI_TYPE_VERSION   = 0x03,
    CCX_OUI_TYPE_QOS_PARAM = 0x04,

} CCX_OUI_Type_t;


/**  CCX IE types that are checked for in BSS descriptions returned in scans.
 */
typedef enum
{
    CCX_IE_VERSION = VENDOR_SPECIFIC_221,
    CCX_IE_CELL_POWER_LIMIT = 0x96,
    CCX_IE_AIRONET = 133,
} CCX_IE_Id_t;



/**  Generic IE Structure containing an OUI and oui type used to encapsulate 
 *    CCX Proprietary elements. 
 */
typedef struct _CCX_IE_OUT_Hdr
{
    u8 elemId;
    u8 len;
    u8 oui[3];
    u8 ouiType;
} CCX_IE_OUI_Hdr_t;
    

/**  CCX Version IE
 *    Received in beacons/probe responses and sent out in (Re)Assocation
 *    requests as a vendor specific 221 element with the OUI set to 0x004096 
 *    and the ouiType set to 0x03.
 *    One octet specifying the major CCX Version supported.
 */    
typedef struct
{
    CCX_IE_OUI_Hdr_t ouiHdr;
    u8 version;

} CCX_IE_Version_t;

typedef struct
{
    CCX_IE_OUI_Hdr_t ouiHdr;
    u8 cwMin[2];
    u8 cwMax[2];
    u8 flags[2];
    u8 apName[16];
    u8 clients;
    u8 value[3];
} CCX_IE_AIRONET_t;

/**  CCX Cell Power Limit IE
 *    Received in beacon and probe responses (scans) with a Element ID of 0x96
 *    with the OUI set to 0x004096 and the ouiType set to 0x00.
 *    One octet containing the maximum transmit power for the STA
 */
typedef struct
{
    CCX_IE_OUI_Hdr_t ouiHdr;
    u8 maxPower;
    u8 reserved;

} CCX_IE_Cell_Power_Limit_t;
///	
/* almost a macro, inline function instead */
/* convert big endian bytes pointed to by pSrc to native UINT16 */
_inline static OS_UINT16 GetBigEndian(OS_UINT8 *pSrc)               
{                               
    OS_UINT16 dest;
    dest = pSrc[0];             
    dest = (dest << 8) + pSrc[1]; 
    return dest;
}

/* 
** The following struct is used by STA to send previous AP info to new AP with
** a single member in the imList field as well as messages from the AP with up 
** to 30 AP in teh imList field.
*/
typedef struct _CCX_IAPP_HEADER_           	/// header of IAPP message
{
	OS_UINT16	Length;						/// Big endian length, including this field
	OS_UINT8	Type;
	OS_UINT8 	Func_SubType;

	NDIS_802_11_MAC_ADDRESS     imDest;     /// unicast destination
	NDIS_802_11_MAC_ADDRESS     imSrc;      /// source address
    /// [list follows here]
} CCX_IAPP_HEADER,*PCCX_IAPP_HEADER;


typedef struct _CCX_DDPINFO_CONTENT_
{
    OS_UINT16     	imTag;   				/// 0x009b
    OS_UINT16     	imIELength;        		/// Big endian length, including this field
    OS_UINT8        imIEOUI[4];				/// 00 40 96 00
    NDIS_802_11_MAC_ADDRESS imPreAPMac;		/// mac Addr of the previous AP 
    OS_UINT16      	imChannel;				/// Channel of the previous AP 
    OS_UINT16     	imSSIDLength;
	BYTE			ssid[32];
	OS_UINT16		disassocTime;			///Disconnected period
} CCX_DDPINFO_CONTENT,*PCCX_DDPINF_CONTENT;


typedef struct _CCX_IAPP_FRAME_
{
	CCX_IAPP_HEADER		iappHdr;				/// Type defined as 0x30
												/// SubType defined as 0x00
	CCX_DDPINFO_CONTENT	Content;
} CCX_DDPINFO, *PCCX_DDPINFO;

///
/// [S36] Radio Measurement
///
/// Data structure of Beacon measurement report
///
typedef struct _CCX_BEACON_MEASURE_REPORT_    
{
    OS_UINT8        ChannelNumber; 	
    OS_UINT8        Spare;            /* unused */        
    OS_UINT16       MeasureDuration; 
	OS_UINT8		PhyType;
	OS_UINT8		RcvSignalPower;
	OS_UINT8  		BSSID[6];
	OS_UINT32		ParentTSF;
	OS_UINT8		TargetTSF[8];
	OS_UINT16		BeaconInterval;
	OS_UINT16		CapabilityInfo;
///	OS_UINT8		RcvElements[1];   /* variable elements */
} CCX_BEACON_MEASURE_REPORT,*PCCX_BEACON_MEASURE_REPORT;

/// Header for radio measurement report elements
typedef struct _CCX_MEASURE_REPORT_ELEMENT_    
{
    OS_UINT16       ElementID; 				/// Element ID 39
    OS_UINT16       Length;					/// Big endian length, including this field
    OS_UINT16       MeasureToken; 
    OS_UINT8        MeasureMode;         
	OS_UINT8        MeasureType;         
	// Measure Reports follow (variable length) 
} CCX_MEASURE_REPORT_ELEMENT,*PCCX_MEASURE_REPORT_ELEMENT;




/// Header fo radio measurement request element fields
typedef struct _CCX_MEASURE_REQ_ELEMENT_    
{
    OS_UINT16       ElementID; 					/// Element ID 38
    OS_UINT16       Length;						/// Big endian length, including this field
    OS_UINT16       MeasureToken; 
    OS_UINT8        MeasureMode;         
	OS_UINT8        MeasureType;         
	CCX_MEASURE_REQUEST MeasureReq[1];			/// Measure Requests follow (variable length)
} CCX_MEASURE_REQ_ELEMENT,*PCCX_MEASURE_REQ_ELEMENT;

///
/// Header for radio measurement report message
///
typedef struct _CCX_MEASURE_REPORT_MSG_    
{
	CCX_IAPP_HEADER 	iappHdr;				/// Type defined as 0x32
												/// SubType defined as 0x81
	OS_UINT16			dialogToken;			/// equal to value in corresponding request frame
	///CCX_MEASURE_REPORT_ELEMENT 	repElement[1];	/// Measure report elements follow (variable)

} CCX_MEASURE_REPORT_MSG,*PCCX_MEASURE_REPORT_MSG;

///
/// Header for radio measurement request message
///
typedef struct _CCX_MEASURE_REQ_MSG_    
{
	CCX_IAPP_HEADER 	iappHdr;				/// Type defined as 0x32
												/// SubType defined as 0x01
	OS_UINT16			dialogToken;			/// nonzero value chosen by STA
	OS_UINT8			activateDelay;			/// # of TBTTs until Measurement Offset field starts
	OS_UINT8			measureOffset;			/// time after activation delay
	CCX_MEASURE_REQ_ELEMENT		reqElement[1];	/// Measure request elements follow (variable)
} CCX_MEASURE_REQ_MSG,*PCCX_MEASURE_REQ_MSG;

typedef enum
{
	CCX_CCKM_START = 0,
	CCX_CCKM_REQUEST,
	CCX_CCKM_RESULT,
	CCX_CCKM_COMPLETE
} CCX_CCKM_STATES;
/// Element ID, 
/// ref: Table S36-4 of CCX specific
#define ELEM_MEASURE_REQUEST		38
#define ELEM_MEASUR_REPORT			39
#define	ELEM_AP_TXPOWER				150
#define	ELEM_RM_CAPABILITY			221

/// Radio measurement request element mode
/// ref: Figure S36-5
#define MEASURE_MODE_REQ_PARALLEL	1			/// 0 - req starts immediately after prev req completed
												/// 1 = request start as same time as prev req 
#define MEASURE_MODE_REQ_ENABLE		2			/// 0 - req and report bits are invalid
												/// 1 - req and report bits are valid
#define MEASURE_MODE_REQ_REPORT		4			/// 0 - disable autonomous report or report bit is invalid 
												///	1 - enable autonomous report


/// Measurement types for Measurement request element
/// ref: Table S36-6
#define MEASURE_REQ_UNUSED				0
#define MEASURE_REQ_CHANNEL_LOAD		1
#define MEASURE_REQ_NOISE_HIST			2
#define MEASURE_REQ_BEACON				3
#define MEASURE_REQ_FRAME				4
#define MEASURE_REQ_HIDDENNODE			5


/// Measurement types for Measurement report element
/// ref: Table S36-6
#define MEASURE_REP_UNUSED				0
#define MEASURE_REP_CHANNEL_LOAD		1
#define MEASURE_REP_NOISE_HIST			2
#define MEASURE_REP_BEACON				3
#define MEASURE_REP_FRAME				4
#define MEASURE_REP_HIDDENNODE			5

/// Different PHY types for mrpPhyType ( All others are undefined )
/// ref: Table 36-9
#define PHY_TYPE_FH 				1
#define PHY_TYPE_DSS 				2				///b, b/g
#define PHY_TYPE_UNUSED 			3
#define PHY_TYPE_OFDM 				4				///a
#define PHY_TYPE_HIGH_RATE_DSS 		5
#define PHY_TYPE_ERP 				6				///g-only

///Measure Report Mode
/// ref: Table 36-12
#define MEASURE_REPORT_MODE_PARALLEL	0x01		// 0 - req starts immediately after prev req completed
													// 1 = request start as same time as prev req
#define MEASURE_REPORT_MODE_INCAPABLE	0x02		// 0 - STA is capable or report is autonomous
													// 1 - STA is incapable
#define MEASURE_REPORT_MODE_REFUSED		0x04		// 0 - STA is not refusing or report is autonomous
													// 1 - STA is refusing

#define CCX_IAPP_TYPE_REPORT_INFORM   	0x30        ///Type for information reporting
#define CCX_IAPP_TYPE_RADIO_MEASURE		0x32		///Type for Radio Measurement

#define CCX_IAPP_SUBTYPE_REQUEST		0x01
#define	CCX_IAPP_SUBTYPE_REPORT			0x81
///#define CCX_IAPP_FUNCTION 	0x00        /* function for above */
///#define CCX_TAG_ADJACENT_AP 0x009b      /* 16 bites */

///crlo:fastroam --

typedef struct _ccx_msg_t {
	UCHAR dstAddr[6];
	UCHAR srcAddr[6];
	UCHAR protocolLen[2];			// protocol Length set to 0 can set SNAP
	UCHAR snapHdr[8];
	UCHAR length[2];			// 
	UCHAR msgType;			// 0x40
	UCHAR function;			// 0x8e
	UCHAR apAddr[6];		// MAC address of destination AP
	UCHAR staAddr[6];		// station MAC addr
	UCHAR failcode[2];		// failure code
	UCHAR rogueAddr[6];		// MAC address of Rogue AP
	UCHAR rogueName[16];	// name of Rogue AP
} CCX_MSG_T, *PCCX_MSG_T;
#pragma pack()

/********************************************************
		Local Variables
********************************************************/

/**
 *  Supported version included in CCX Version IE for (Re)Association requests
 */
static u8 ccx_supported_version = 4U;

/**
 *  Cisco OUI parsed in Vendor Specific IEs 
 */
static u8 cisco_oui[] = { 0x00, 0x40, 0x96 };
static u8 aironet_oui[] = { 0x00, 0x01, 0x6d };


static ULONG ccx_flags = 0U;
static ULONG ccx_EAPstate = 0U;

// BSSID
static NDIS_802_11_SSID  ccxCurSsid;
static NDIS_802_11_MAC_ADDRESS ccxCurBSSID;
static NDIS_802_11_MAC_ADDRESS ccxStaMac;

// Rogue AP
#define CCX_ROGUE_AP_COUNT	16
static FSW_CCX_ROGUE_AP_DETECTED ccxRogueAP[CCX_ROGUE_AP_COUNT];

// packet
static NDIS_HANDLE	ccxPacketPoolHandle;
static NDIS_HANDLE      ccxBufferPoolHandle;
static PNDIS_PACKET    pCCXPacket[CCX_ROGUE_AP_COUNT];
static PNDIS_BUFFER     pCCXBuffer[CCX_ROGUE_AP_COUNT];
static PVOID                 pCCXBufVM[CCX_ROGUE_AP_COUNT];
static PNDIS_PACKET	pCCXCurPacket = NULL;

static NDIS_HANDLE      *pMrvDrvAdapterHdl = NULL;
static PMRVDRV_ADAPTER pCcxAdapter = NULL;
static UCHAR ccxAssociationRequestIE[128];

static int ifNeedTPC=0;
static int ccxTPCMinPower;
static int ccxTPCMaxPower;
static int ccxTPCUserPower = CCX_TPC_MaxPowerValue;

static    CCX_CCKM_STATES  ccxCCKMState = CCX_CCKM_START;
static ULONG    ccxCCKMulAttemptSSIDIdx = -1;
static ULONG ccxAssociationRequestIELength;
static UCHAR ccxAssociationRequestIE[128];


/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief Append a ccx version ie to the buffer and advance
 *  the buffer pointer by the size of the IE appended.
 *  Return the size appended.
 *
 *  @param ppBuffer	pointer to the buffer pointer
 *  @return		number of bytes appended to **ppBuffer	
 */  
static int append_ccx_version_ie( u8** ppBuffer )
{
    CCX_IE_Version_t versionIe;

    /* Null checks */
    if (ppBuffer == 0) return 0;
    if (*ppBuffer == 0) return 0;

    /* Setup OUI Header information */
    versionIe.ouiHdr.elemId = CCX_IE_VERSION;
    versionIe.ouiHdr.len = sizeof(CCX_IE_Version_t) - 2; /* -2 for len + id */
    memcpy(versionIe.ouiHdr.oui, cisco_oui, sizeof(cisco_oui));
	///CCX support version
    ///crlo: CCX-VERSION ++
    versionIe.ouiHdr.ouiType = CCX_OUI_TYPE_VERSION;
	///versionIe.ouiHdr.ouiType = CCX_OUI_TYPE_QOS_PARAM;
    ///crlo: CCX-VERSION --

    /* Set version to be internally stored supported version */
    versionIe.version = ccx_supported_version;

    /* Copy the stack versionIe to the returned buffer parameter */
    memcpy(*ppBuffer, (u8*)&versionIe, sizeof(versionIe));
    
    /* Advance the buffer pointer by the size we appended */
    *ppBuffer += sizeof(versionIe);

//    DBGPRINT(DBG_CCX, ("[append_ccx_version_ie]  return size=%d\n", sizeof(versionIe)));
	
    /* Return appended size */
    return (sizeof(versionIe));
}

static int append_ccx_aironet_ie( u8** ppBuffer )
{
    CCX_IE_AIRONET_t aironetIe;

    /* Null checks */
    if (ppBuffer == 0) return 0;
    if (*ppBuffer == 0) return 0;

    NdisZeroMemory(&aironetIe, sizeof(aironetIe));
	
    /* Setup OUI Header information */
    aironetIe.ouiHdr.elemId = CCX_IE_AIRONET;
    aironetIe.ouiHdr.len = sizeof(CCX_IE_AIRONET_t) - 2; /* -2 for len + id */
    memcpy(aironetIe.ouiHdr.oui, aironet_oui, sizeof(aironet_oui));
    aironetIe.ouiHdr.ouiType = 0x00;

    aironetIe.flags[0] = 0x18;
    memcpy(aironetIe.apName, "MRVL-CW", 7);

    /* Copy the stack versionIe to the returned buffer parameter */
    memcpy(*ppBuffer, (u8*)&aironetIe, sizeof(aironetIe));
    
    /* Advance the buffer pointer by the size we appended */
    *ppBuffer += sizeof(aironetIe);

//    DBGPRINT(DBG_CCX, ("[append_ccx_aironet_ie]  return size=%d\n", sizeof(aironetIe)));
	
    /* Return appended size */
    return (sizeof(aironetIe));
}

/** 
 *  @brief Append a marvell power capability TLV to the buffer.
 *  The minimum power should be set to 0.  The maximum power is
 *  taken from the CCX Cell Power IE received in the scan results
 *  before association.
 *
 *  @param ppBuffer	pointer to the buffer pointer
 *  @param minPower	min power in power capability TLV
 *  @param maxPower	max power in power capability TLV 
 *  @return 	   	number of bytes appended to **ppBuffer 	
 */  
static int append_mrvl_power_capability(u8** ppBuffer,
                                        u8 minPower, u8 maxPower)
{
    MrvlIEtypes_PowerCapability_t powerCapIe;

    /* Null checks */
    if (ppBuffer == 0) return 0;
    if (*ppBuffer == 0) return 0;

    /* Set up marvell Power Capability TLV with passed parameters */
	powerCapIe.Header.Type = cpu_to_le16(TLV_TYPE_POWER_CAPABILITY);
	powerCapIe.Header.Len = cpu_to_le16(sizeof(powerCapIe) 
                                        - sizeof(powerCapIe.Header));
    powerCapIe.MinPower = minPower;
    powerCapIe.MaxPower = maxPower;

    DBGPRINT(DBG_CCX, (L"CCX: Power Cap Added: min(%d), max(%d)\n", minPower, maxPower));

    /* Copy the stack powerCapIe to the returned buffer parameter */
    memcpy(*ppBuffer, (u8*)&powerCapIe, sizeof(powerCapIe));

//    HEXDUMP("CCX: PwrCap IE", *ppBuffer, sizeof(powerCapIe));

    /* Advance the buffer pointer by the size we appended */
    *ppBuffer += sizeof(powerCapIe);
    
    /* Return appended size */
    return (sizeof(powerCapIe));
}

/** 
 *  @brief Append a cckm request ie to the buffer and advance
 *  the buffer pointer by the size of the IE appended.
 *  Return the size appended.
 *
 *  @param		
 *		- ppBuffer			pointer to the buffer pointer
 *		- timestamp			Timestamp of the AP to roam
 *		- ulAttemptSSIDIdx	The id in the AP list that we will associate with
 *  @return		number of bytes appended to **ppBuffer	
 */  
static int append_cckm_request_ie( u8** ppBuffer, UCHAR *timestamp, ULONG ulAttemptSSIDIdx )
{
	DWORD		dwWaitStatus;
	const int	CCKMTimeout = 1200;            ///Maximum 1.2 seconds to wait (tried value)

	DBGPRINT(DBG_CCX,  (L"[wlan_ccx_process_assoc_req] cckmEnabled\r\n"));
	if ( ccxCCKMState == CCX_CCKM_START )
	{
		ccxAssociationRequestIELength = 0;
		ccxCCKMulAttemptSSIDIdx = ulAttemptSSIDIdx;
		DBGPRINT(DBG_CCX,  (L"[wlan_ccx_process_assoc_req] ccxCCKMulAttemptSSIDIdx: %d\r\n", ccxCCKMulAttemptSSIDIdx));
		ResetEvent(pCcxAdapter->WaitCCKMIEEvent);
		wlan_ccx_cckmStart(timestamp);
	}

	DBGPRINT(DBG_CCX,  (L"[%s] Enter wait for CCKMIE\n", TEXT(__FUNCTION__)));

	dwWaitStatus = WaitForSingleObject(pCcxAdapter->WaitCCKMIEEvent, CCKMTimeout);
	if (dwWaitStatus != WAIT_OBJECT_0)
	{
		DBGPRINT(DBG_CCX,  (L"[%s] timeout, (%d ms)....\n", TEXT(__FUNCTION__), CCKMTimeout));
	}

	DBGPRINT(DBG_CCX,  (L"[%s] Resumed for CCKMIE\n", TEXT(__FUNCTION__)));

	DBGPRINT(DBG_CCX,  (L"[wlan_ccx_process_assoc_req] Appending CCX_CCKM_REQUEST: IE length=%d\n", ccxAssociationRequestIELength));
	if ( ccxAssociationRequestIELength > 0 ) {
		DBGPRINT(DBG_CCX,  (L"[wlan_ccx_process_assoc_req] copy ccxAssociationRequestIE, len=%d\n", ccxAssociationRequestIELength));
		NdisMoveMemory(*ppBuffer, ccxAssociationRequestIE, ccxAssociationRequestIELength);
		*ppBuffer += ccxAssociationRequestIELength;
		///passThroughLen +=(USHORT)ccxAssociationRequestIELength;   // neo cckm 
	}
	return ccxAssociationRequestIELength;
}

static void ccx_freePacketBuffer()
{
	int i;

	for ( i=0; i < CCX_ROGUE_AP_COUNT; i++ )
	{
		if ( pCCXBufVM[i] != NULL )
		{
			NdisFreeMemory(pCCXBufVM[i],
			 		      MRVDRV_ETH_TX_PACKET_BUFFER_SIZE,
			       			0);            
			pCCXBufVM[i] = NULL;
		}

		if ( pCCXBuffer[i] != NULL )
		{
			NdisFreeBuffer(pCCXBuffer[i]);
			pCCXBuffer[i] = NULL;
		}

		if ( pCCXPacket[i] != NULL )
		{
			NdisFreePacket(pCCXPacket[i]);
			pCCXPacket[i] = NULL;
		}
	} 

	if ( ccxBufferPoolHandle != NULL )
	{
		NdisFreeBufferPool(ccxBufferPoolHandle);
		ccxBufferPoolHandle = NULL;
	}

	if ( ccxPacketPoolHandle != NULL )
	{
		NdisFreePacketPool(ccxPacketPoolHandle);
		ccxPacketPoolHandle = NULL;
	}


}



static int ccx_allocatePacketBuffer()
{
    int i;
    NDIS_STATUS	tStatus;
    	PNDIS_PACKET_OOB_DATA  pOOB;
	
    // null init
    ccxPacketPoolHandle = NULL;
    ccxBufferPoolHandle = NULL;
    	for ( i=0; i < CCX_ROGUE_AP_COUNT; i++ )
    	{
    		pCCXPacket[i] = NULL;
		pCCXBuffer[i] = NULL;
		pCCXBufVM[i] = NULL;
    	}
		
       // allocate packet pool
	// packet pool 
	NdisAllocatePacketPoolEx(
		&tStatus, 
		&ccxPacketPoolHandle,
	       CCX_ROGUE_AP_COUNT,
	   	CCX_ROGUE_AP_COUNT, 
	       PROTOCOL_RESERVED_SIZE_IN_PACKET);

	if ( tStatus != NDIS_STATUS_SUCCESS )
	{
		DBGPRINT( DBG_ERROR, (L"[CCX] Unable to allocate packet pool!\n"));
		goto error;
	}

	// buffer pool 
	NdisAllocateBufferPool(
		&tStatus,
	       &ccxBufferPoolHandle,
	       CCX_ROGUE_AP_COUNT);

	if ( tStatus != NDIS_STATUS_SUCCESS )
	{
		DBGPRINT( DBG_ERROR,
	        			(L"Unable to allocate buffer pool!\n"));
		goto error;	
	}


	// assign space to used three array 
	for ( i=0; i < CCX_ROGUE_AP_COUNT; i++ )
	{
		// data payload space array 
		tStatus = 
		NdisAllocateMemoryWithTag(
				&pCCXBufVM[i], 
		              MRVDRV_ETH_TX_PACKET_BUFFER_SIZE,
		              MRVDRV_MEMORY_TAG);
		if ( tStatus != NDIS_STATUS_SUCCESS ) {
			DBGPRINT( DBG_ERROR,
	        			(L"Unable to do NdisAllocateMemoryWithTag [%d]\n", i));
			goto error;
		}

		// buffer array 
		NdisAllocateBuffer(
					&tStatus,
	                		&pCCXBuffer[i],
	                		ccxBufferPoolHandle,
	                		pCCXBufVM[i],
	                		MRVDRV_ETH_TX_PACKET_BUFFER_SIZE);
		if ( tStatus != NDIS_STATUS_SUCCESS ) {
			DBGPRINT( DBG_ERROR,
	        			(L"Unable to do NdisAllocateBuffer [%d]\n", i));
			goto error;
		}
		
		// packet array 	
		NdisAllocatePacket(
				&tStatus, 
	               	&pCCXPacket[i],
	               	ccxPacketPoolHandle);
		if ( tStatus != NDIS_STATUS_SUCCESS ) {
			DBGPRINT( DBG_ERROR,
	        			(L"Unable to do NdisAllocatePacket [%d]\n", i));
			goto error;
		}

		// init OBB space
		pOOB = NDIS_OOB_DATA_FROM_PACKET(pCCXPacket[i]);
		NdisZeroMemory(pOOB, sizeof(NDIS_PACKET_OOB_DATA));
		NDIS_SET_PACKET_HEADER_SIZE(pCCXPacket[i], MRVDRV_ETH_HEADER_SIZE);

    		// chain the packet and buffer 
    		NdisChainBufferAtFront(pCCXPacket[i], pCCXBuffer[i]);

	}

	tStatus = NDIS_STATUS_SUCCESS;
	
error:
	if (tStatus != NDIS_STATUS_SUCCESS) ccx_freePacketBuffer();
	return tStatus;
}


///static UCHAR ccxSnapHdr[8] = {0xaa, 0xaa, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};
static UCHAR ccxSnapHdr[8] = {0xaa, 0xaa, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};

NDIS_STATUS
ccx_send_packet(
	IN PMRVDRV_ADAPTER Adapter,
	IN PNDIS_PACKET pPacket,
	FSW_CCX_ROGUE_AP_DETECTED *pRougeAPMsg	
	)
{
	 PNDIS_BUFFER	pBuffer;
	 PUCHAR		    	pBufVM;
	 UINT			bufLen;
	 PCCX_MSG_T	pCCXMsg;

	NdisQueryPacket(pPacket, NULL, NULL, &pBuffer, NULL);
	NdisQueryBufferSafe(pBuffer, (void *)&pBufVM, &bufLen, NormalPagePriority);
	NdisAdjustBufferLength(pBuffer, sizeof(CCX_MSG_T));

	pCCXMsg = (PCCX_MSG_T)pBufVM;
	NdisMoveMemory(pCCXMsg->dstAddr, ccxCurBSSID, sizeof(pCCXMsg->dstAddr));	 
	NdisMoveMemory(pCCXMsg->srcAddr, ccxStaMac, sizeof(pCCXMsg->srcAddr));
	pCCXMsg->protocolLen[0] = 0;
	pCCXMsg->protocolLen[1] = 0;
	NdisMoveMemory(pCCXMsg->snapHdr, ccxSnapHdr, sizeof(pCCXMsg->snapHdr));
	pCCXMsg->length[0] = 0;
	pCCXMsg->length[1] = 40;
	pCCXMsg->msgType = 0x40;
	pCCXMsg->function = 0x8e;
	NdisMoveMemory(pCCXMsg->apAddr, ccxCurBSSID, sizeof(pCCXMsg->apAddr));	 
	NdisMoveMemory(pCCXMsg->staAddr, ccxStaMac, sizeof(pCCXMsg->staAddr));

	//
	pCCXMsg->failcode[0] = (UCHAR)(pRougeAPMsg->FailureReason >> 8);
	pCCXMsg->failcode[1] = (UCHAR)(pRougeAPMsg->FailureReason & 0xFF);
	NdisMoveMemory(pCCXMsg->rogueAddr, pRougeAPMsg->RogueAPMacAddress, sizeof(pCCXMsg->rogueAddr));	 
	NdisMoveMemory(pCCXMsg->rogueName, pRougeAPMsg->RogueAPName, sizeof(pCCXMsg->rogueName));

	pCCXCurPacket = pPacket;
	DBGPRINT(DBG_CCX, (L"[ccx_send_packet] cur packet=%x\n", pCCXCurPacket));
	///return SendSinglePacket(Adapter, pPacket);
	return CCX_SendSinglePacket(Adapter, pPacket);
}

NDIS_STATUS ccx_set_txpower(ULONG minPower, ULONG maxPower)
{
	NDIS_STATUS 	status = NDIS_STATUS_SUCCESS;
	UCHAR			power=0;

	DBGPRINT(DBG_CCX,  (L"[ccx_set_power] maxPower=%x, ccxTPCUserPower=%x\n", maxPower, ccxTPCUserPower));
	if ( ((int)maxPower) > ccxTPCUserPower ) {
		maxPower = ccxTPCUserPower;
	}

        ///Using HostCmd_CMD_802_11_TPC_CFG/HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT instead
        ProcessOIDSetTxPowerLevel(pCcxAdapter, maxPower);
	DBGPRINT(DBG_CCX, (L"[ccx_set_power] SET powerLevel=%x, status=%d\n", maxPower, status));
	return status;
}

/********************************************************
		Global Functions
********************************************************/

void wlan_ccx_init(	IN PMRVDRV_ADAPTER pAdapter)
{
	int i;

	// Set adapter
	pCcxAdapter = pAdapter;

	// clear ccxRogueAP
	for (i=0; i<CCX_ROGUE_AP_COUNT; i++)
		NdisZeroMemory(&(ccxRogueAP[i]), sizeof(FSW_CCX_ROGUE_AP_DETECTED));

	ccx_allocatePacketBuffer();

        pAdapter->bStartCcxS56Test = FALSE;
        pAdapter->bEnableS56DriverIAPP = 0;
        NdisMInitializeTimer(
                        &pAdapter->hCcxV4S56TestTimer,
                        pAdapter->MrvDrvAdapterHdl,
                        (PNDIS_TIMER_FUNCTION)FunCcxV4S56TestTimer,
                        (PVOID)pAdapter
                        );
        pAdapter->bCcxV4S56TestTimerIsSet = FALSE;

}

void wlan_ccx_free(IN PMRVDRV_ADAPTER pAdapter)
{

        pAdapter->bStartCcxS56Test = FALSE;

        if (pAdapter->bCcxV4S56TestTimerIsSet == TRUE)
        {
                BOOLEAN  timerStatus;
                NdisMCancelTimer(&pAdapter->hCcxV4S56TestTimer, &timerStatus);
                pAdapter->bCcxV4S56TestTimerIsSet=FALSE;		 
        }		

	ccx_freePacketBuffer();
}

void wlan_ccx_setCurMAC(
	IN PNDIS_802_11_SSID curSsid,
	IN NDIS_802_11_MAC_ADDRESS curBSSID, 
	IN NDIS_802_11_MAC_ADDRESS curStaMac 
	)
{
	NdisMoveMemory(&ccxCurSsid, curSsid, sizeof(NDIS_802_11_SSID));
	NdisMoveMemory(&ccxCurBSSID, curBSSID, sizeof(NDIS_802_11_MAC_ADDRESS));	 
	NdisMoveMemory(&ccxStaMac, curStaMac, sizeof(NDIS_802_11_MAC_ADDRESS));	 
}

static NDIS_802_11_MAC_ADDRESS tmpMacAddr = {0x01, 0x02, 0x03, 0x04, 0x05,0x06};
static UCHAR tmpName[] = "HelloAP";
void wlan_ccx_send_packet(
	IN PMRVDRV_ADAPTER Adapter
	)
{
	FSW_CCX_ROGUE_AP_DETECTED rogueAPMsg;

	rogueAPMsg.FailureReason = 0x04;
	NdisMoveMemory(rogueAPMsg.RogueAPMacAddress, tmpMacAddr, sizeof(NDIS_802_11_MAC_ADDRESS));
	NdisMoveMemory(rogueAPMsg.RogueAPName, tmpName, sizeof(tmpName));
	ccx_send_packet(Adapter, pCCXPacket[0], &rogueAPMsg); 
	
}

int wlan_ccx_isCurPacket(PNDIS_PACKET pPacket)
{
    if (pCCXCurPacket == NULL) {
        return 0;
    }
    if ( pCCXCurPacket == pPacket ) {
        return 1;
    } 	else {
        return 0;
    }
}

///crlo:slowping ++
void wlan_ccx_clrCurPacket()
{
	//PrintMacro("CurrPacket: %xh\n", pCCXCurPacket);
	pCCXCurPacket = NULL;
	return;
}
///crlo:slowping --

/** 
 *  @brief Call back from the main command module to allow CCX proprietary additions
 *  to the association request sent to the firmware.  
 *
 *  @param ppAssocBuf		pointer to the buffer pointer for the assoc req.
 *  @param pCcxBssInfo		CCX information of the specific AP from the scan
 *  @param enableCcxPwrLimit 	boolean CCX transmit power control
 *  @return			number of bytes added to the assoc. request 
 */
				     
int wlan_ccx_process_association_req(u8** ppAssocBuf,
					  CCX_BSS_Info_t* pCcxBssInfo,
					  WPA_SUPPLICANT* pWpaInfo,
					  UCHAR *timestamp,
                                     int enableCcxPwrLimit,
                                            ULONG ulAttemptSSIDIdx)
{
	u8* pBufSav;
	u16 passThroughLen = 0;
	MrvlIEtypesHeader_t ieHeader;
	UCHAR minPower;
	UCHAR maxPower;

	DBGPRINT(DBG_CCX, (L"CCX: process assoc req: ccxEnabled: %d, enableCcxPwrLimit: %d\n", 
            pCcxBssInfo->ccxEnabled, enableCcxPwrLimit));

    
    ///pCcxAdapter->isCiscoAP = FALSE;
    ///DBGPRINT(DBG_TMP|DBG_HELP,(L"==>wlan_ccx_process_association_req(1): %d\n", pCcxAdapter->isCiscoAP)); 
    /* Null checks */
    if (ppAssocBuf == 0) return 0;
    if (*ppAssocBuf == 0) return 0;
    if (pCcxBssInfo == 0) return 0;

    /*
    **  Return immediately if CCX support isn't required by this specific AP
    **   we are attempting to associate with.  Flag set during scan result 
    **   processing.
    */
    if (pCcxBssInfo->ccxEnabled == FALSE)
    {
        return 0;
    }
    ///pCcxAdapter->isCiscoAP = TRUE;
    ///DBGPRINT(DBG_TMP|DBG_HELP,(L"==>wlan_ccx_process_association_req: %d\n", pCcxAdapter->isCiscoAP)); 

    /* Save off the original buffer pointer */
    pBufSav = *ppAssocBuf;

    /* Increment the buffer pointer by the size of the Marvell TLV header.
    **   Skip past the buffer space for the header to append the CCX version
    **   IE.  When we get the size of the version IE, then append the 
    **   marvell TLV header with the appropriate size
    */
    *ppAssocBuf += sizeof(MrvlIEtypesHeader_t);

    // Implement for CCKM
    if (pCcxBssInfo->cckmEnabled == TRUE)
    {
		if (pCcxBssInfo->isMediaConnected == FALSE) {
			/// We will try to request the CCKM-Request_IE from supplicant. The information will be passed down by an OID
			/// Becuase this is in the middle of OID_802_11_SSID
			///	=> If using Device_Scape supplicant, it will be blocked in here
			/// Because if it's disconnected, we don't need this IE, but to send the WPA_IE. It's not needed to talk with supplicant
			DBGPRINT(DBG_CCX,  (L"Not Connected yet, not roaming~\r\n"));
		} else {
			passThroughLen += append_cckm_request_ie(ppAssocBuf, timestamp, ulAttemptSSIDIdx );
		}
    }

    /*
    ** Append the version IE to the association buffer.  passThroughLen holds
    **   the amount of data inserted (used for the passthrough TLV header len
    */
    passThroughLen += append_ccx_version_ie(ppAssocBuf);
    /* 
    **  Append the TLV Header to the saved buffer pointer (insert it before
    **    the version IE that was just appended).  Set the TLV ID to the 
    **    PASSTHROUGH type and encode the length as the size of the version IE
    **    previously added.
    */

    passThroughLen += append_ccx_aironet_ie(ppAssocBuf);
   

// neo cckm    ieHeader.Type = TLV_TYPE_PASSTHROUGH;
// neo cckm    ieHeader.Len  = passThroughLen;
// neo cckm    memcpy(pBufSav, &ieHeader, sizeof(ieHeader));

	/*
	**  If a CCX power IE was sensed in the scan response, and CCX Power
	**    limiting is enabled (i.e. no AP 11h support), add a power capability
	**    TLV to the association request setting the max power to the AP
	**    specified maximum
	*/
	DBGPRINT(DBG_CCX,  (L"CCX: maxPowerValid=%x, enableCcxPwrLimit=%x\n", pCcxBssInfo->maxPowerValid, enableCcxPwrLimit));
	if (pCcxBssInfo->maxPowerValid && enableCcxPwrLimit) {
		minPower = CCX_TPC_MinPowerValue;
		maxPower = pCcxBssInfo->maxPower;
		if ( maxPower < minPower ) {
			maxPower = minPower;
		}
		if ( maxPower > CCX_TPC_MaxPowerValue ) {
			maxPower = CCX_TPC_MaxPowerValue;
		}
		ifNeedTPC = 1;
		ccxTPCMaxPower = maxPower;
		ccxTPCMinPower = minPower;
	} else {
		ifNeedTPC = 0;
	}

// neo cckm 
    ieHeader.Type = TLV_TYPE_PASSTHROUGH;
    ieHeader.Len  = passThroughLen;
    memcpy(pBufSav, &ieHeader, sizeof(ieHeader));
// neo cckm

    /* Return the length we've appended to the buffer */
//    DBGPRINT(DBG_CCX, ("[wlan_ccx_proc_asoc_req] return size=%d\n", (*ppAssocBuf-pBufSav))); 
    return (*ppAssocBuf - pBufSav);
}

/** 
 *  @brief This function processes an element in a description
 *
 *  @param pCcxBssInfo	CCX information
 *  @param pBssElem	BSS IE for CCX relevance evaluation
 *  @return 		WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int wlan_ccx_process_bss_elem(CCX_BSS_Info_t* pCcxBssInfo, u8* pBssElem)
{

    CCX_IE_Version_t*           pVersionIe;
    CCX_IE_Cell_Power_Limit_t*  pCellPowerIe;
    CCX_IE_OUI_Hdr_t*           pOuiHdr = (CCX_IE_OUI_Hdr_t*)pBssElem;

    int enableBssCcxExt = FALSE;
    
//    DBGPRINT(DBG_CCX,  ("CCX: process bss element: %d (0x%x)\n", 
//            pOuiHdr->elemId, pOuiHdr->elemId));

    /*
    ** Switch statement based on the element ID encoded in the Header
    */
    switch (pOuiHdr->elemId)
    {
    case VENDOR_SPECIFIC_221:
        /*
        ** Vendor Specific Element ID 221 passes information for a variety 
        **   of protocols.  The CCX elements are identified by the Cisco
        **   OUI  (0x004096), check for it and then the OUI Type to correctly
        **   parse a CCX Element
        */
        if (NdisEqualMemory(pOuiHdr->oui, cisco_oui, sizeof(cisco_oui)) == 1)
        {
            /* Flag the presence of CCX elements */
            enableBssCcxExt = TRUE;

            switch (pOuiHdr->ouiType)
            {
            case CCX_OUI_TYPE_VERSION:
                /*
                ** CCX Version IE, just identify it for now.  May need
                **   to customize the CCX behavior in the driver and 
                **   supplicant based on the version of CCX the AP supports
                */
                pVersionIe = (CCX_IE_Version_t*)pOuiHdr;
                //DBGPRINT(DBG_CCX,  ("CCX: Version IE found: CCX Version %d\n",
                //        pVersionIe->version));
                break;
            case CCX_OUI_TYPE_QOS_PARAM:
                /*
                ** No longer used in CCX V2 specification v2.13 
                */
                //DBGPRINT(DBG_CCX,  ("CCX: QOS Param Set IE found: Deprecated\n"));
                break;
            default:
                /*
                **  Aironet/Cisco OUI specified, but unidentified subtype
                */
                //DBGPRINT(DBG_CCX,  ("CCX: unhandled oui type: %x (%d)\n",
                //        pOuiHdr->ouiType, pOuiHdr->ouiType));
                break;
            }
        }
        break;
    case CCX_IE_CELL_POWER_LIMIT:
        if (NdisEqualMemory(pOuiHdr->oui, cisco_oui, sizeof(cisco_oui)) == 1)
        {
            /* Flag the presence of CCX elements */
            enableBssCcxExt = TRUE;
            
            /*
            ** Set the max power indicated by the Cell Power Limit IE
            **   in the CCX BSS information structure for this AP.
            **   Mark it as valid. Used in association processing.
            */
            pCellPowerIe = (CCX_IE_Cell_Power_Limit_t*)pOuiHdr;
            pCcxBssInfo->maxPowerValid = TRUE;
            pCcxBssInfo->maxPower = pCellPowerIe->maxPower;

            DBGPRINT(DBG_CCX,  (L"CCX: Cell Power Limit IE found: Max Power = %d\n",
                    pCellPowerIe->maxPower));
        }
        break;
    default:
        /*
        **  Not a CCX element
        */
        DBGPRINT(DBG_CCX,  (L"CCX: Unhandled IE: %d\n", pOuiHdr->elemId));
        break;
    }

    /*
    ** If the parsing routines above find CCX elements, set the enable 
    **   flag in the BSS info structure for this AP.  Triggers CCX support
    **   in association processing.
    */
    pCcxBssInfo->ccxEnabled = enableBssCcxExt;

    /* Succssful return */
    return WLAN_STATUS_SUCCESS;
}


ULONG wlan_ccx_getFlags(void)
{
	return ccx_flags;
}

void wlan_ccx_setFlags(ULONG flags)
{
	ccx_flags = flags;
}

ULONG wlan_ccx_getEAPState(void)
{
	return ccx_EAPstate;
}

void wlan_ccx_setEAPState(ULONG state)
{
	ccx_EAPstate = state;
}

USHORT wlan_ccx_MapAuthMode(ULONG state)
{
    USHORT      AuthMode;
    switch (state) {
        case FswCcx_NetworkEapOff:
        case FswCcx_NetworkEapAllowed:
            AuthMode = 0x00;
            break;
        case FswCcx_NetworkEapOn:
        case FswCcx_NetworkEapPreferred:
            AuthMode = 0x80;
            break;
    }
    return AuthMode;
}

void wlan_ccx_setRogueAP(FSW_CCX_ROGUE_AP_DETECTED *pRogueAP)
{
        int ix;
        
	for (ix=0;ix < CCX_ROGUE_AP_COUNT;ix++)
	{
		if (NdisEqualMemory(pRogueAP, &(ccxRogueAP[ix]), sizeof(FSW_CCX_ROGUE_AP_DETECTED)) )
		{
		//DBGPRINT(DBG_CCX,  ("[CCX:wlan_ccx_setRogueAP] Rogue AP already exists in the list\n"));
		    break;
		}

		if ( ccxRogueAP[ix].FailureReason != 0 )
		{                
		    continue; /* not empty */
		}
		//DBGPRINT(DBG_CCX, ("[CCX] Rogue AP [%d] : ", ix));
		//HexDump(DBG_CCX,"Data: ", (PCHAR) pRogueAP, sizeof(FSW_CCX_ROGUE_AP_DETECTED));
		NdisMoveMemory(&ccxRogueAP[ix], pRogueAP, sizeof(FSW_CCX_ROGUE_AP_DETECTED));	 
		break;                                 
        }
        if ( ix == CCX_ROGUE_AP_COUNT )
        {
		DBGPRINT(DBG_ERROR,  (L"[CCX:wlan_ccx_setRogueAP] No empty in Rogue AP list\n"));
        }
                
}


void wlan_ccx_sendRogueAPList(
	IN PMRVDRV_ADAPTER Adapter
	)
{
    int i;

    for (i=0; i<CCX_ROGUE_AP_COUNT; i++) {
		if ( ccxRogueAP[i].FailureReason == 0 ) continue; // No failure

		ccx_send_packet(Adapter, pCCXPacket[i], &(ccxRogueAP[i]));
		ccxRogueAP[i].FailureReason = 0;	// clear after sending
    }
	
}

//042507++, compile option CCX 
//042507--, compile option CCX 
void wlan_ccx_fillFrameHeader(PMRVDRV_ADAPTER pAdapter, u8* pktBufPt)
{
	PMRVPKTDESC	pPktMsg = (PMRVPKTDESC)pktBufPt;
	u8*				bufPt = pktBufPt;

	NdisMoveMemory(pPktMsg->dstAddr, pAdapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
	NdisMoveMemory(pPktMsg->srcAddr, pAdapter->PermanentAddr, MRVDRV_ETH_ADDR_LEN);
	pPktMsg->protocolLen[0] = 0;
	pPktMsg->protocolLen[1] = 0;

	///pCCXMsg->tmp[0] = pCCXMsg->tmp[1] = 0;
	bufPt += sizeof(MRVPKTDESC);
	///NdisMoveMemory(bufPt, ccxSnapHdr, sizeof(ccxSnapHdr));

	return ;
}
//042507++, compile option CCX 
//042507--, compile option CCX 

NDIS_STATUS
ccx_send(
	IN PMRVDRV_ADAPTER Adapter,
	IN PNDIS_PACKET pPacket,
	IN CHAR*	pktbuf,
	IN UINT	pktlen
	)
{
	PNDIS_BUFFER	pBuffer;
	PUCHAR		    	pBufVM;
	UINT			bufLen;
	NDIS_STATUS		status;
///	 PCCX_MSG_T	pCCXMsg;

	//PrintMacro("ccx_send(%u)\n", pktlen);
	DBGPRINT(DBG_CCX|DBG_HELP,(L"ccx_send(%u)\n", pktlen));

	NdisQueryPacket(pPacket, NULL, NULL, &pBuffer, NULL);
	NdisQueryBufferSafe(pBuffer, (void *)&pBufVM, &bufLen, NormalPagePriority);
	NdisAdjustBufferLength(pBuffer, pktlen);

	NdisMoveMemory(pBufVM, pktbuf, pktlen);

//	pCCXMsg = (PCCX_MSG_T)pBufVM;
//	NdisMoveMemory(pCCXMsg->dstAddr, ccxCurBSSID, sizeof(pCCXMsg->dstAddr));	 
//	NdisMoveMemory(pCCXMsg->srcAddr, ccxStaMac, sizeof(pCCXMsg->srcAddr));
//	pCCXMsg->protocolLen[0] = 0;
//	pCCXMsg->protocolLen[1] = 0;
//	NdisMoveMemory(pCCXMsg->snapHdr, ccxSnapHdr, sizeof(pCCXMsg->snapHdr));
//	pCCXMsg->length[0] = 0;
//	pCCXMsg->length[1] = 40;
//	pCCXMsg->msgType = 0x40;
//	pCCXMsg->function = 0x8e;
//	NdisMoveMemory(pCCXMsg->apAddr, ccxCurBSSID, sizeof(pCCXMsg->apAddr));	 
//	NdisMoveMemory(pCCXMsg->staAddr, ccxStaMac, sizeof(pCCXMsg->staAddr));

	//
//	pCCXMsg->failcode[0] = (UCHAR)(pRougeAPMsg->FailureReason >> 8);
//	pCCXMsg->failcode[1] = (UCHAR)(pRougeAPMsg->FailureReason & 0xFF);
//	NdisMoveMemory(pCCXMsg->rogueAddr, pRougeAPMsg->RogueAPMacAddress, sizeof(pCCXMsg->rogueAddr));	 
//	NdisMoveMemory(pCCXMsg->rogueName, pRougeAPMsg->RogueAPName, sizeof(pCCXMsg->rogueName));

//	pCCXCurPacket = pPacket;
//	DBGPRINT(DBG_CCX, ("[ccx_send_packet] cur packet=%x\n", pCCXCurPacket));
	///return SendSinglePacket(Adapter, pPacket);
	EnterCriticalSection(&Adapter->TxCriticalSection);
	status = CCX_SendSinglePacket(Adapter, pPacket);
	LeaveCriticalSection(&Adapter->TxCriticalSection);
	return status;
}

/** 
 *  @brief This function processes an element in a description
 *
 *  @param pCcxBssInfo	CCX information
 *  @param pBssElem	BSS IE for CCX relevance evaluation
 *  @return 		WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
void wlan_ccx_fillDDPINFOPkt(PMRVDRV_ADAPTER pAdapter, u8* pktBufPt)
{
	PCCX_DDPINFO			pDDPINFOPkt = (CCX_DDPINFO*)pktBufPt;
	PCCX_IAPP_HEADER		pIAPPFrameHdr = &pDDPINFOPkt->iappHdr;
	PCCX_DDPINF_CONTENT		pFrameContent = &pDDPINFOPkt->Content;
	PFASTROAMPARAM			pfrParam = &pAdapter->frParam;

	OS_UINT16			tmp;
	OS_UINT16			currTm;
	UINT				fillPktLen = 0;
	int					i;

	///
	///Fill the IAPP header
	///
	tmp = (WORD)sizeof(CCX_DDPINFO);
	pIAPPFrameHdr->Length = GetBigEndian((OS_UINT8 *) &tmp);
	pIAPPFrameHdr->Type = CCX_IAPP_TYPE_REPORT_INFORM;
	pIAPPFrameHdr->Func_SubType = 0x00;
	NdisMoveMemory((PVOID)pIAPPFrameHdr->imDest, pfrParam->ccxCurrentAP.alMACAddr, MRVDRV_ETH_ADDR_LEN);
	NdisMoveMemory((PVOID)pIAPPFrameHdr->imSrc, pAdapter->PermanentAddr, MRVDRV_ETH_ADDR_LEN);

	///Fill the IAPP IE
	tmp = 0x009b;
	pFrameContent->imTag = GetBigEndian((OS_UINT8 *) &tmp);
	pFrameContent->imIEOUI[0] = 0x00;
	pFrameContent->imIEOUI[1] = 0x40;
	pFrameContent->imIEOUI[2] = 0x96;
	pFrameContent->imIEOUI[3] = 0x00;

	pFrameContent->imSSIDLength = 
		GetBigEndian((OS_UINT8 *) &(pfrParam->ccxLastAP.ssid.SsidLength));
	pFrameContent->imChannel =  
		GetBigEndian((OS_UINT8 *) &(pfrParam->ccxLastAP.alChannel));
	tmp = sizeof(CCX_DDPINFO_CONTENT) - 4;		
	pFrameContent->imIELength = GetBigEndian((OS_UINT8 *) &tmp);


	NdisMoveMemory((PVOID)pFrameContent->imPreAPMac, pfrParam->ccxLastAP.alMACAddr, MRVDRV_ETH_ADDR_LEN);
	NdisMoveMemory((PVOID)pFrameContent->ssid, (PVOID)pfrParam->ccxLastAP.ssid.Ssid, 
		(pfrParam->ccxLastAP.ssid.SsidLength<32)?pfrParam->ccxLastAP.ssid.SsidLength:32);

	DBGPRINT(DBG_CCX|DBG_HELP,(TEXT("SsidLength: %d\n"), pfrParam->ccxLastAP.ssid.SsidLength));
	for (i=pfrParam->ccxLastAP.ssid.SsidLength ; i<32 ; i++) {
		pFrameContent->ssid[i] = '\0';
	}

	currTm = (OS_UINT16)((GetTickCount() - pfrParam->ccxLastAP.alDisassocTime)/1000);

	DBGPRINT(DBG_CCX|DBG_HELP,(L"DissAssocTime: %d (%d, %d)\n", currTm, GetTickCount(), pfrParam->ccxLastAP.alDisassocTime));
	pFrameContent->disassocTime = GetBigEndian((OS_INT8*)&currTm);

	return;
}

void wlan_ccx_send_ddp(IN PMRVDRV_ADAPTER Adapter)
{
	UCHAR		iapp[256];
	UCHAR		*bufpt = iapp;

	///Fill the "header" of the packet we need to generate
	wlan_ccx_fillFrameHeader(Adapter, bufpt);
	bufpt += sizeof(MRVPKTDESC);
	///Fill the "SNAP" header
	NdisMoveMemory(bufpt, ccxSnapHdr, sizeof(ccxSnapHdr));
	bufpt += sizeof(ccxSnapHdr);
	///Generate the iapp frame
	wlan_ccx_fillDDPINFOPkt(Adapter, bufpt);
	///Send the iapp frame
	{
		int		i, j, k;
		UCHAR	*datpt=iapp;
		int		pktlen = sizeof(MRVPKTDESC)+sizeof(ccxSnapHdr)+sizeof(CCX_DDPINFO);

		DBGPRINT(DBG_CCX|DBG_HELP,(L"PktLen(%d): ", pktlen));
		for (i=0; i<pktlen/16 ; i++) {
			k = i*16;
			DBGPRINT(DBG_CCX|DBG_HELP,(L"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				datpt[k+0], datpt[k+1], datpt[k+2], datpt[k+3], datpt[k+4], datpt[k+5], datpt[k+6], datpt[k+7],
				datpt[k+8], datpt[k+9], datpt[k+10], datpt[k+11], datpt[k+12], datpt[k+13], datpt[k+14], datpt[k+15]));
		}
		if ((pktlen%16) != 0) {
			UCHAR	tmpbuf[16];
			for (j=0 ; j<(pktlen%16) ; j++) {
				tmpbuf[j] = iapp[i*16+j];
			}
			for (;j<16 ; j++) {
				tmpbuf[j] = 0xff;
			}
			DBGPRINT(DBG_CCX|DBG_HELP,(L"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				tmpbuf[0], tmpbuf[1], tmpbuf[2], tmpbuf[3], tmpbuf[4], tmpbuf[5], tmpbuf[6], tmpbuf[7],
				tmpbuf[8], tmpbuf[9], tmpbuf[10], tmpbuf[11], tmpbuf[12], tmpbuf[13], tmpbuf[14], tmpbuf[15]));
		}

	}
	ccx_send(Adapter, pCCXPacket[0], iapp, sizeof(MRVPKTDESC)+sizeof(ccxSnapHdr)+sizeof(CCX_DDPINFO));
	return;
}
void wlan_ccx_recTxPower(int userPower)
{
	ccxTPCUserPower = userPower;
	DBGPRINT(DBG_CCX|DBG_HELP,  (L"[wlan_ccx_recTxPower] userPower=%x\n", userPower));
}

void wlan_ccx_setTxPower(void)
{
	DBGPRINT(DBG_HELP,  (L"Enter wlan_ccx_setTxPower(%d, %d, %d)\n", ifNeedTPC, ccxTPCMinPower, ccxTPCMaxPower));
	if (ifNeedTPC) {
		ccx_set_txpower(ccxTPCMinPower, ccxTPCMaxPower);
	}
	DBGPRINT(DBG_HELP,  (L"Exit wlan_ccx_setTxPower\n"));
}

void wlan_ccx_assocSuccess(void)
{
	wlan_ccx_setTxPower();
}

void wlan_ccx_authSuccess(IN PMRVDRV_ADAPTER pAdapter)
{
	ccxCCKMState = CCX_CCKM_START;
	///Associated success. Cache the AP information
	NdisMoveMemory(&pAdapter->frParam.ccxLastAP, &pAdapter->frParam.ccxCurrentAP, sizeof(CCX_AP_INFO));

	return;
}
int wlan_ccx_fillBeaconTable(IN PMRVDRV_ADAPTER pAdapter, u8* pktBufPt)
{
	PCCX_MEASURE_REPORT_MSG	pMeasureRep = (PCCX_MEASURE_REPORT_MSG)pktBufPt;
	PCCX_IAPP_HEADER		pIAPPFrameHdr = &pMeasureRep->iappHdr;
	///PCCX_MEASURE_REPORT_ELEMENT	pMRElem = pMeasureRep->repElement;
	PCCX_MEASURE_REPORT_ELEMENT	pMRElem;
	PCCX_BEACON_MEASURE_REPORT	pBeaconReport;
	OS_UINT16	pktlen;
	UCHAR		*repBufPt;
	OS_UINT16	i;

	///
	///Fill the IAPP header
	///
	///tmp = (WORD)sizeof(CCX_DDPINFO);
	///pIAPPFrameHdr->Length = GetBigEndian((OS_UINT8 *) &tmp);
	pIAPPFrameHdr->Type = CCX_IAPP_TYPE_RADIO_MEASURE;
	pIAPPFrameHdr->Func_SubType = CCX_IAPP_SUBTYPE_REPORT;

	///Destination MAC (All 0s, ref S36.5 of CCX specification v2.8
	///NdisMoveMemory((PVOID)pIAPPFrameHdr->imDest, pAdapter->ccxCurrentAP.alMACAddr, MRVDRV_ETH_ADDR_LEN);
	NdisZeroMemory((PVOID)pIAPPFrameHdr->imDest, MRVDRV_ETH_ADDR_LEN);
	///Source MAC
	NdisMoveMemory((PVOID)pIAPPFrameHdr->imSrc, pAdapter->PermanentAddr, MRVDRV_ETH_ADDR_LEN);
	pktlen = sizeof(CCX_MEASURE_REPORT_MSG);
    DBGPRINT(DBG_CCX|DBG_HELP,(L"CCX_MEASURE_REPORT_MSG len: %d\n", sizeof(CCX_MEASURE_REPORT_MSG)));
	///Fill the Dialog-token
	pMeasureRep->dialogToken = pAdapter->dialogToken;
///	pktlen += sizeof(pMeasureRep->dialogToken);
    DBGPRINT(DBG_CCX|DBG_HELP,(L"dialogToken len: %d\n", sizeof(pMeasureRep->dialogToken)));

	///
	///Fill in the Measurment Report Element
	///
    DBGPRINT(DBG_CCX|DBG_HELP,(L"(NumOfBSSIDs, channel): (%d,%d) \n", pAdapter->ulPSNumOfBSSIDs, pAdapter->ChannelNum));

	pMRElem = (PCCX_MEASURE_REPORT_ELEMENT)(pMeasureRep+1);
	for (i=0 ; i< pAdapter->ulPSNumOfBSSIDs; i++) {
		NDIS_WLAN_BSSID_EX*	pBSSIDList = &pAdapter->PSBSSIDList[i];
		BSS_DESCRIPTION_SET_ALL_FIELDS*	pBssDescList = &pAdapter->PSBssDescList[i];
		PMRV_BSSID_IE_LIST  pBSSIDIEListSrc = &pAdapter->PSIEBuffer[i];
		UCHAR       *pCurPtr;
        UCHAR       ElemID, IELen;
		OS_UINT16	MRELength;
		///PUCHAR		pIEBuf = pBSSIDIEListSrc->VariableIE;
		ULONG		ulCurOffset = 0;

		///report-channel-only ++
		{
                OS_UINT8 currchannel = (OS_UINT8)((pBSSIDList->Configuration.DSConfig - 2407) / 5);
                if (currchannel != pAdapter->ChannelNum) {
                    DBGPRINT(DBG_CCX|DBG_HELP,(L"This AP is in channel (%d) skip now\n", currchannel));
                    continue;
                }
		}
		///report-channel-only --

		pMRElem->ElementID = ELEM_MEASUR_REPORT;
		pMRElem->MeasureToken = pAdapter->MeasureToken;
		pMRElem->MeasureMode = pAdapter->MeasureMode;
		pMRElem->MeasureType = MEASURE_REP_BEACON;
///		pktlen += sizeof(CCX_MEASURE_REPORT_ELEMENT);
		MRELength = sizeof(CCX_MEASURE_REPORT_ELEMENT);

		///Fill in the Measure Report (Beacon Report)
		///repBufPt is one byte ahead of pBeaconReport
		///repBufPt = ((UCHAR*)pMRElem)+sizeof(CCX_MEASURE_REPORT_MSG);
		pBeaconReport = (PCCX_BEACON_MEASURE_REPORT)(pMRElem+1);
		///Channel Number
		pBeaconReport->ChannelNumber = (OS_UINT8)((pBSSIDList->Configuration.DSConfig - 2407) / 5);
		DBGPRINT(DBG_CCX|DBG_HELP,(L"CCX=>(%d) Channel: %xh\n", i, pBeaconReport->ChannelNumber));

		///Spare
		pBeaconReport->Spare = 0;
		///MeasureDuration
		pBeaconReport->MeasureDuration = pAdapter->MeasureDuration;
		///PHY Type
		///PrintMacro("PhyType: %d\n", pBSSIDList->NetworkTypeInUse);
		pBeaconReport->PhyType = pBSSIDList->NetworkTypeInUse;

		switch (pBSSIDList->NetworkTypeInUse) 
		{
		case Ndis802_11DS:
			pBeaconReport->PhyType = PHY_TYPE_DSS;
			///RETAILMSG(1,(TEXT("PhyType: PHY_TYPE_DSS\n")));
			break;
		case Ndis802_11OFDM24:
			pBeaconReport->PhyType = PHY_TYPE_OFDM;
			///RETAILMSG(1,(TEXT("PhyType: PHY_TYPE_OFDM\n")));
			break;
		case Ndis802_11OFDM5:
			pBeaconReport->PhyType = PHY_TYPE_ERP;
			///RETAILMSG(1,(TEXT("PhyType: PHY_TYPE_ERP\n")));
			break;
		}
		
		///Received Signal Power
		pBeaconReport->RcvSignalPower = (OS_UINT8)pBSSIDList->Rssi;
		///BSSID
		///NdisMoveMemory((PVOID)pBeaconReport->BSSID, pAdapter->PermanentAddr, MRVDRV_ETH_ADDR_LEN);
		NdisMoveMemory((PVOID)pBeaconReport->BSSID, pBSSIDList->MacAddress, MRVDRV_ETH_ADDR_LEN);
		///Parent TSF
		pBeaconReport->ParentTSF = pAdapter->parentTSF;
		///Target TSF
		NdisMoveMemory((PVOID)pBeaconReport->TargetTSF, pBssDescList->TimeStamp, 8);
		///Beacon Interval
		pBeaconReport->BeaconInterval = (OS_UINT16)(pBSSIDList->Configuration.BeaconPeriod);
		///Capability Information
		NdisMoveMemory((PVOID)&pBeaconReport->CapabilityInfo, (PVOID)&pBssDescList->Cap, sizeof(pBeaconReport->CapabilityInfo));
		///RETAILMSG(1,(TEXT("CapabilityInfo: %xh\n"), pBeaconReport->CapabilityInfo));
		MRELength += sizeof(CCX_BEACON_MEASURE_REPORT);

		repBufPt = ((UCHAR*)pBeaconReport) + sizeof(CCX_BEACON_MEASURE_REPORT);
		///Received-Element
		pCurPtr = pBSSIDIEListSrc->VariableIE;
///PrintMacro("VariableIE_Len: %d\n", (pBSSIDList->IELength - MRVL_FIXED_IE_SIZE));
///RETAILMSG(1,(TEXT("VariableIE_Len: %d\n"), (pBSSIDList->IELength - MRVL_FIXED_IE_SIZE)));
		while ( ulCurOffset < 
                   (pBSSIDList->IELength - MRVL_FIXED_IE_SIZE) )
		{
			ElemID = *pCurPtr;
            IELen = *(pCurPtr+1);
			if (IELen == 0) {
				//PrintMacro("Abnormal: IELen is 0\n");
				DBGPRINT(DBG_CCX|DBG_HELP,(L"Abnormal: IELen is 0\n"));
				break;
			}
			if ((ElemID != SSID) &&
				(ElemID != SUPPORTED_RATES) && 
				(ElemID != FH_PARAM_SET) && 
				(ElemID != DS_PARAM_SET) && 
				(ElemID != CF_PARAM_SET) && 
				(ElemID != IBSS_PARAM_SET) && 
				(ElemID != TIM)  
				///(ElemID != RM_CAPABILITY)
				) {

				///RETAILMSG(1,(TEXT("Unknown IE(%d): %xh\n"), IELen, ElemID));
				pCurPtr = pCurPtr + 2 + IELen;
				continue;
			}

			NdisMoveMemory((PVOID)repBufPt, (PVOID)pCurPtr, (IELen+2));
			repBufPt = repBufPt + 2 + IELen;
			pCurPtr = pCurPtr + 2 + IELen;
			MRELength += (2 + IELen);
			ulCurOffset = ulCurOffset + 2 + IELen;
		}
		///Record the length of the Measure_Report_Element (= total_length - [ElementID] - [Length])
		pMRElem->Length = MRELength - 4;
		///RETAILMSG(1,(TEXT("CCX_MEASURE_REPORT_ELEMENT len: %d\n"), pMRElem->Length));
		pktlen += MRELength;
		///Moving the pointer for the next Measure_Report_Element
		pMRElem = (PCCX_MEASURE_REPORT_ELEMENT)repBufPt;
	}

	pIAPPFrameHdr->Length = GetBigEndian((OS_UINT8 *)&pktlen);

	return pktlen;
}

void wlan_ccx_send_BeaconTable(IN PMRVDRV_ADAPTER Adapter)
{
	UCHAR		iapp[SDIO_MAX_PKT_LEN];
	UCHAR		*bufpt = iapp;
	int			pktlen;

	///Fill the "header" of the packet we need to generate
	wlan_ccx_fillFrameHeader(Adapter, bufpt);
	bufpt += sizeof(MRVPKTDESC);
	///Fill the "SNAP" header
	NdisMoveMemory(bufpt, ccxSnapHdr, sizeof(ccxSnapHdr));
	bufpt += sizeof(ccxSnapHdr);

	///Generate the iapp frame
	pktlen = wlan_ccx_fillBeaconTable(Adapter, bufpt);
	///PrintMacro("Iapp(%d) \n", pktlen);
	DBGPRINT(DBG_CCX|DBG_HELP,(L"Iapp(%d) \n", pktlen));
	///Send the iapp frame
	ccx_send(Adapter, pCCXPacket[0], iapp, sizeof(MRVPKTDESC)+sizeof(ccxSnapHdr)+pktlen);
	DBGPRINT(DBG_CCX|DBG_HELP,(L"End of wlan_ccx_send_BeaconTable \n"));
	return;
}


void wlan_ccx_parse_iapp(IN PMRVDRV_ADAPTER Adapter, u8* pktBufPt, PPKTPARAM pPktParam)
{
	PCCX_MEASURE_REQ_MSG	pPktMsg;

	NdisZeroMemory(pPktParam, sizeof(PKTPARAM));
	pPktParam->pkttype = CCXPKT_INVALID;
	if (!NdisEqualMemory(pktBufPt, ccxSnapHdr, sizeof(ccxSnapHdr))) {
		///RETAILMSG(1,(TEXT("ccxSnapHdr(%d)\n"), sizeof(ccxSnapHdr)));
		///RETAILMSG(1,(TEXT("pktBufPt(%02xh, %02xh, %02xh, %02xh, %02xh, %02xh, %02xh, %02xh)\n"), 
		///	pktBufPt[0], pktBufPt[1], pktBufPt[2], pktBufPt[3],
		///	pktBufPt[4], pktBufPt[5], pktBufPt[6], pktBufPt[7]));
		///RETAILMSG(1,(TEXT("ccxSnapHdr(%02xh, %02xh, %02xh, %02xh, %02xh, %02xh, %02xh, %02xh)\n"), 
		///	ccxSnapHdr[0], ccxSnapHdr[1], ccxSnapHdr[2], ccxSnapHdr[3],
		///	ccxSnapHdr[4], ccxSnapHdr[5], ccxSnapHdr[6], ccxSnapHdr[7]));
		goto funcFinal;
	}

	pPktMsg = (PCCX_MEASURE_REQ_MSG)(&pktBufPt[sizeof(ccxSnapHdr)]);
	if ((pPktMsg->iappHdr.Type == CCX_IAPP_TYPE_RADIO_MEASURE) &&
		(pPktMsg->iappHdr.Func_SubType == CCX_IAPP_SUBTYPE_REQUEST) &&
		(pPktMsg->reqElement[0].ElementID == ELEM_MEASURE_REQUEST)) {
		///This is Radio Measurement, request Pkt
		PCCX_MEASURE_REQ_ELEMENT	pMRmsg = &pPktMsg->reqElement[0];
		Adapter->dialogToken = pPktMsg->dialogToken;
		Adapter->MeasureToken = pMRmsg->MeasureToken;
		DBGPRINT(DBG_CCX|DBG_HELP,(L"MeasureToken: %xh\n", Adapter->MeasureToken));
		switch (pMRmsg->MeasureType) {
		case MEASURE_REP_BEACON: {		///ASD Only supports Beacon_Request			
			PCCX_MEASURE_REQUEST	pParam = &pPktParam->beaconReq;
			pPktParam->pkttype = CCXPKT_BEACON_REQUEST;
			NdisMoveMemory(&pPktParam->beaconReq, pMRmsg->MeasureReq, sizeof(pMRmsg->MeasureReq[0]));
			if (pPktParam->beaconReq.MeasureDuration > 200) {
				DBGPRINT(DBG_CCX|DBG_HELP,(L"MeasureDuration: %d is too big, Igonore this request\n", pPktParam->beaconReq.MeasureDuration));
				pPktParam->pkttype = CCXPKT_INVALID;
				break;
			}
			}
			break;
		default:
			break;;
		}
		DBGPRINT(DBG_CCX|DBG_HELP,(L"pMRmsg->MeasureType: %xh\n", pMRmsg->MeasureType));
	}

funcFinal:
	return ;
}
int wlan_ccx_cckmStart(PUCHAR pTimestamp)
{
	static FSW_CCX_CCKM_START cckmStart;

	DBGPRINT(DBG_CCX,  (L"[wlan_ccx_cckmStart] start"));

	NdisMoveMemory(cckmStart.BSSID, ccxCurBSSID, sizeof(cckmStart.BSSID));	 
	NdisMoveMemory(cckmStart.Timestamp, pTimestamp, sizeof(cckmStart.Timestamp));	 
	///crlo:cckm-fastroaming ++
	ccxCCKMState = CCX_CCKM_REQUEST;
	///crlo:cckm-fastroaming ++
       NdisMIndicateStatus(
                              pCcxAdapter->MrvDrvAdapterHdl,
                              OID_FSW_CCX_CCKM_START,
                              (PVOID)&cckmStart,
                              sizeof(cckmStart));
       NdisMIndicateStatusComplete(pCcxAdapter->MrvDrvAdapterHdl);

//	Our UI doesn't support CCKM yet
//    MrvlNotifyApplication(COMMAND_EVENT_CCX_CCKM_START, (PUCHAR)&CckmStart,  sizeof(CckmStart));

	// Start Timer
	///ccxCCKMState = CCX_CCKM_REQUEST;
	DBGPRINT(DBG_CCX,  (L"Exit %s", TEXT(__FUNCTION__)));
	return 0;
}

void	wlan_ccx_CCKMRequest(PFSW_CCX_CCKM_REQUEST pRequest)
{
        DBGPRINT(DBG_CCX,  (L"[wlan_ccx_CCKMRequest] start (CCKMMode: %x)\n", ccxCCKMState));

	// Timer Handler
	if ( ccxCCKMState == CCX_CCKM_REQUEST ) {
	    	DBGPRINT(DBG_CCX,  (L"[wlan_ccx_CCKMRequest] RequestCode=%d\n", pRequest->RequestCode));
	        /* if this is a good hand-off, save parameters */
			DBGPRINT(DBG_CCX,  (L"[wlan_ccx_CCKMRequest] start (reqcode:%d)", pRequest->RequestCode));
	        if ( pRequest->RequestCode == FswCcx_CckmFastHandoff )
	        {
	            /* make sure IE length will fit our buffer */
	            if ( pRequest->AssociationRequestIELength > 128 )
	            {
	                DBGPRINT(DBG_CCX, (L"ccxCCKMReqeust: IE too long\n"));
	                return;
	            }
		     ccxAssociationRequestIELength = pRequest->AssociationRequestIELength;
		     NdisMoveMemory(ccxAssociationRequestIE, pRequest->AssociationRequestIE, ccxAssociationRequestIELength);
		      ccxCCKMState = CCX_CCKM_RESULT;  // do re-association and wait result
			  DBGPRINT(DBG_CCX, (L"FswCcx_CckmFastHandoff --StaReAssociateRequest\n"));
			//StaReAssociateRequest(p802_11,&p802_11->ccxTargetBSSID);	
			
	        } else
	        {   /* otherwise, make sure that we don't send out CCKM IE */
			ccxAssociationRequestIELength = 0;
				DBGPRINT(DBG_CCX, (L"FswCcx_CckmFastFirstTime-StaAssociateRequest \n"));
		      //StaAssociateRequest(p802_11,&p802_11->ccxTargetBSSID);
			///crlo:cckm-fastroaming ++		      
		      ///1st time to cckm. Reset the id
                    ccxCCKMulAttemptSSIDIdx = -1;
                    DBGPRINT(DBG_CCX,  (L"[wlan_ccx_CCKMRequest] Reset ccxCCKMulAttemptSSIDIdx: %d\n", ccxCCKMulAttemptSSIDIdx));
			///crlo:cckm-fastroaming --
	        }
		//ccx_associate();
		///crlo:cckm-fastroaming ++
            DBGPRINT(DBG_CCX,  (L"[wlan_ccx_CCKMRequest] Resuming assoc_pkt\n"));
            SetEvent(pCcxAdapter->WaitCCKMIEEvent);
		///crlo:cckm-fastroaming --
	}
    DBGPRINT(DBG_CCX,  (L"==>Exit %s\n", TEXT(__FUNCTION__)));
}

VOID
Send_CCX4_S56_Notification(
    PMRVDRV_ADAPTER Adapter,
    UCHAR *ie, 
    USHORT ie_len, 
    UCHAR *dest_addr, 
    UCHAR *src_addr
) 
{     
    static OID_MRVL_DS_CCX_NOTIFY_UI      ccx4_notify;
    ccx4_notify.NotifyType = NOTIFY_CCX4_S56_TSM;
    ccx4_notify.ParamLen = sizeof(MRVPKTDESC)+ sizeof(IE_TRAFFIC_STREAM_METRIC);

    DBGPRINT(DBG_CCX_V4, (L"Send_CCX4_S56_Notification() \n"));

    NdisMoveMemory((PUCHAR)(ccx4_notify.MrvPktDesc.dstAddr), (PUCHAR)dest_addr, 6);
    NdisMoveMemory((PUCHAR)(ccx4_notify.MrvPktDesc.srcAddr), (PUCHAR)src_addr, 6);
    ccx4_notify.MrvPktDesc.protocolLen[0] = 0;
    ccx4_notify.MrvPktDesc.protocolLen[1] = 0;

    NdisMoveMemory((PUCHAR)(&(ccx4_notify.ccx4_s56_tsm)), ie, ie_len+2);
    {
        int i =0; PUCHAR pos;

        DBGPRINT(DBG_CCX_V4,(L"ccx4_notify.NotifyType : %d\n",ccx4_notify.NotifyType));
        DBGPRINT(DBG_CCX_V4,(L"ccx4_notify.ParamLen : %d\n",ccx4_notify.ParamLen));
        DBGPRINT(DBG_CCX_V4,(L"total len : %d\n",sizeof(MRVPKTDESC)+ sizeof(IE_TRAFFIC_STREAM_METRIC)+8));
        DBGPRINT(DBG_CCX_V4,(L"ccx4_notify.NotifyType : %d\n",ccx4_notify.NotifyType));
        pos = (PUCHAR)(&(ccx4_notify.NotifyType));

        for (i=0; i< sizeof(MRVPKTDESC)+ sizeof(IE_TRAFFIC_STREAM_METRIC)+8;i++)
        {
            // DBGPRINT(DBG_CCX_V4,(L"%02x ",*(ie+i)));
            DBGPRINT(DBG_CCX_V4,(L"%02x ",*(pos+i)));  
            if (i%12 ==11 )
            {
                DBGPRINT(DBG_CCX_V4,(L"\n"));
            }
        }
        DBGPRINT(DBG_CCX_V4,(L"\n"));
    }	
    NdisMIndicateStatus(Adapter->MrvDrvAdapterHdl,
                        NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                        (PUCHAR)(&ccx4_notify),
                        sizeof(MRVPKTDESC)+ sizeof(IE_TRAFFIC_STREAM_METRIC)+8);
    NdisMIndicateStatusComplete(Adapter->MrvDrvAdapterHdl);  
}


BOOLEAN
Find_Ie_Traffic_Stream_Metric(
    UCHAR *buf, 
    USHORT buf_len, 
    USHORT *offset
)
{
    PIE_TRAFFIC_STREAM_METRIC pTarget; 
    UCHAR TSM_OUI[] = { 0x00, 0x40, 0x96, 0x07 };
	
    SHORT remain_len =(SHORT)buf_len;
    PUCHAR  CurPos =buf;
    *offset =0;
	
    while (remain_len > 2)
    {
        pTarget=(PIE_TRAFFIC_STREAM_METRIC)CurPos;

        if (pTarget->length == 0)
            break;	  	
		
        if ((pTarget->element_id== 0xdd) && (pTarget->length == 8))
        {
            if (NdisEqualMemory(pTarget->oui, TSM_OUI, 4) == 1)
            {	
                DBGPRINT(DBG_CCX_V4, (L"find the Ie_traffic_stream_metric \n"));
                return TRUE;
            }
        }
        remain_len -= (pTarget->length +2);
        *offset += (pTarget->length +2);
        CurPos +=(pTarget->length +2);
     }
     DBGPRINT(DBG_CCX_V4, (L"don't find the Ie_traffic_stream_metric \n"));
     
     return FALSE;
}

VOID
HandleCcxv4S56Event(
    PMRVDRV_ADAPTER Adapter,
    UCHAR *ie, 
    USHORT ie_len
) 
{
        NDIS_STATUS     Status; 
	 BOOLEAN  timerStatus;
        PIE_TRAFFIC_STREAM_METRIC pTSM;
        ULONG interval;
        UCHAR Cmdbuf[sizeof(HostCmd_DS_802_11_WMM_QUEUE_STATS) + 8];
        PHostCmd_DS_802_11_WMM_QUEUE_STATS pDsWQS;
        ULONG   bytewriten;
		
	 pDsWQS = (PHostCmd_DS_802_11_WMM_QUEUE_STATS)Cmdbuf;


	 
        DBGPRINT(DBG_CCX_V4, (L"HandleCcxv4S56Event \r\n"));
	
        pTSM = (PIE_TRAFFIC_STREAM_METRIC)ie;
	
        if (pTSM->state ==TRUE)
        {
		    interval = ((pTSM->measurement_interval * 1024) / 1000);
			  
                  if (Adapter->bCcxV4S56TestTimerIsSet == FALSE) 
                  {
                            DBGPRINT(DBG_CCX_V4, (L"Start Timer , interval = %d \r\n", interval)); 

				NdisMSetPeriodicTimer(&Adapter->hCcxV4S56TestTimer, interval);
                            Adapter->bCcxV4S56TestTimerIsSet = TRUE;  
							
                            pDsWQS->Action = 0;
	                     pDsWQS->AC = 3;
	
                            Status = PrepareAndSendCommand(
                                                       Adapter,
                                                       HostCmd_CMD_802_11_WMM_QUEUE_STATS,
                                                       0,
                                                       HostCmd_OPTION_USE_INT,
                                                       0,
                                                       HostCmd_PENDING_ON_NONE,
                                                       0,
                                                       FALSE,
                                                       &bytewriten,
                                                       0,
                                                       0,
                                                       &(pDsWQS->Action));
							
                            if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
                            {     
                                 DBGPRINT(DBG_CCX_V4, (L"Clean WMM_QUEUE_STATS fail \r\n")); 
                            }    
							
                  } 		  
                  else
                  { 
                           if (interval != Adapter->TSMRinterval)
			      {		
                                   DBGPRINT(DBG_CCX_V4, (L"Change interval = %d \r\n", interval)); 

                                   NdisMCancelTimer(&Adapter->hCcxV4S56TestTimer, &timerStatus);
			              NdisMSetPeriodicTimer(&Adapter->hCcxV4S56TestTimer, interval);		   
                                   Adapter->bCcxV4S56TestTimerIsSet = TRUE;

				       pDsWQS->Action = 0;
	                            pDsWQS->AC = 3;
	
                                   Status = PrepareAndSendCommand(
                                                               Adapter,
                                                               HostCmd_CMD_802_11_WMM_QUEUE_STATS,
                                                               0,
                                                               HostCmd_OPTION_USE_INT,
                                                               0,
                                                               HostCmd_PENDING_ON_NONE,
                                                               0,
                                                               FALSE,
                                                               &bytewriten,
                                                               0,
                                                               0,
                                                               &(pDsWQS->Action));	   
								   
                                   if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
                                   {     
                                        DBGPRINT(DBG_CCX_V4, (L"Clean WMM_QUEUE_STATS fail \r\n")); 
                                   } 		   
                           }			   
                  } 
		    Adapter->TSMRinterval = interval;	  
        }
        else
        {
                  if (Adapter->bCcxV4S56TestTimerIsSet == TRUE)
                  {
                          DBGPRINT(DBG_CCX_V4, (L"Stop timer\r\n")); 

                          NdisMCancelTimer(&Adapter->hCcxV4S56TestTimer, &timerStatus);
                          Adapter->bCcxV4S56TestTimerIsSet=FALSE;		 
                  }		

        }

}

VOID
FunCcxV4S56TestTimer( IN PVOID SystemSpecific1,
                                                IN NDIS_HANDLE MiniportAdapterContext,
                                                IN PVOID SystemSpecific2,
                                                IN PVOID SystemSpecific3
                                                 )
{
       NDIS_STATUS     Status;  
	ULONG	    dwWaitStatus;  
       UCHAR Cmdbuf[sizeof(HostCmd_DS_802_11_WMM_QUEUE_STATS) + 8];
	PHostCmd_DS_802_11_WMM_QUEUE_STATS pDsWQS;
	ULONG   bytewriten;
	
	UCHAR iapp[SDIO_MAX_PKT_LEN];
       PIE_TRAFFIC_STREAM_METRIC_REPORT_IAPP Iappout;
       PMEASURE_REPORT_ELEMENT pMRE;
	PMEASURE_REPORT pMR;

	PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER) MiniportAdapterContext;
       Iappout = (PIE_TRAFFIC_STREAM_METRIC_REPORT_IAPP)iapp;

       DBGPRINT(DBG_CCX_V4, (L"FunCcxV4S56TestTimer \r\n")); 

       pDsWQS = (PHostCmd_DS_802_11_WMM_QUEUE_STATS)Cmdbuf;
       pDsWQS->Action = 2;
	pDsWQS->AC = 3;
	
       Status = PrepareAndSendCommand(
                                   Adapter,
                                   HostCmd_CMD_802_11_WMM_QUEUE_STATS,
                                   0,
                                   HostCmd_OPTION_USE_INT,
                                   0,
                                   HostCmd_PENDING_ON_GET_OID,
                                   0,
                                   FALSE,
                                   &bytewriten,
                                   0,
                                   0,
                                   &(pDsWQS->Action));


       if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
       {     
              return;
       }    
	   
       dwWaitStatus = WaitForSingleObjectWithCancel( Adapter, Adapter->hOidQueryEvent, ASYNC_OID_QUERY_TIMEOUT );
       DBGPRINT( DBG_CUSTOM, (L"FunCcxV4S56TestTimer: WaitForSingleObjectWithCancel returns %d\r\n", dwWaitStatus) );
       if (dwWaitStatus == WAIT_OBJECT_0)
       {
              if ( Adapter->nOidCmdResult != HostCmd_RESULT_OK )
              {
                     return ;
              }

              NdisMoveMemory(
 	                         (PUCHAR) (&(pDsWQS->Action)),
                                (PUCHAR) (Adapter->OidCmdRespBuf),
                                Adapter->nSizeOfOidCmdResp );

              HexDump(DBG_CCX_V4, "WMM_QUEUE_STATS:", (PUCHAR) (&(pDsWQS->Action)), Adapter->nSizeOfOidCmdResp);                    

              NdisZeroMemory(Iappout, sizeof(IE_TRAFFIC_STREAM_METRIC_REPORT_IAPP));

              NdisMoveMemory(Iappout->DA, Adapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
	       NdisMoveMemory(Iappout->SA, Adapter->PermanentAddr, MRVDRV_ETH_ADDR_LEN);
              Iappout->length = 0x0800;
              Iappout->cisco_aironet_snap[0] = 0xaa;
              Iappout->cisco_aironet_snap[1] = 0xaa;
              Iappout->cisco_aironet_snap[2] = 0x03;
              Iappout->cisco_aironet_snap[3] = 0x00;
              Iappout->cisco_aironet_snap[4] = 0x40;
              Iappout->cisco_aironet_snap[5] = 0x96;
	       Iappout->cisco_aironet_snap[6] = 0x00;
	       Iappout->cisco_aironet_snap[7] = 0x00;
              Iappout->iapp_id_length = 0x2f00;
	       Iappout->iapp_type = 0x32;
	       Iappout->iapp_subtype = 0x81;      
              NdisZeroMemory(Iappout->destination_mac_addr,MRVDRV_ETH_ADDR_LEN);
              NdisMoveMemory(Iappout->source_mac_addr, Adapter->PermanentAddr, MRVDRV_ETH_ADDR_LEN);
	       Iappout->dialog_token = 0x00;

              pMRE = &(Iappout->measurement_report_element[0]);
              pMRE->element_id = 0x27;
              pMRE->length = 0x19;
              pMRE->measurement_token = 0;
              pMRE->measure_mode = 0;
              pMRE->measure_type = 6;
			  
              pMR = &(pMRE->measure_report[0]);
              pMR->packet_q_delay_avg = (USHORT)(pDsWQS->AvgQueueDelay /1000);
              pMR->packet_q_delay_histogram[0] = pDsWQS->DelayHistogram[0] + pDsWQS->DelayHistogram[1];
              pMR->packet_q_delay_histogram[1] = pDsWQS->DelayHistogram[2];
              pMR->packet_q_delay_histogram[2] = pDsWQS->DelayHistogram[3] + pDsWQS->DelayHistogram[4];
              pMR->packet_q_delay_histogram[3] = pDsWQS->DelayHistogram[5] + pDsWQS->DelayHistogram[6];
              pMR->packet_transmit_media_delay = pDsWQS->AvgTxDelay;
              pMR->packet_loss = pDsWQS->PktLoss;
              pMR->packet_count = pDsWQS->PktCount;

              DBGPRINT(DBG_CCX_V4, (L"Adapter->RoamAccount  = %d \r\n", Adapter->RoamAccount)); 
 
		if (Adapter->RoamAccount > 1)
		{
                  pMR->roaming_count = Adapter->RoamAccount - 1;
                  pMR->roaming_delay = Adapter->RoamDelay;
		}
		else
		{
		    pMR->roaming_count = 0;
                  pMR->roaming_delay = 0;
		}
        }
        else
        {
                return;
        }

        DBGPRINT(DBG_CCX_V4, (L"sizeof(IE_TRAFFIC_STREAM_METRIC_REPORT_IAPP) = x%02x \r\n", sizeof(IE_TRAFFIC_STREAM_METRIC_REPORT_IAPP))); 
	 HexDump(DBG_CCX_V4, "Iappout:", iapp, sizeof(IE_TRAFFIC_STREAM_METRIC_REPORT_IAPP));                    
	
	 ccx_send(Adapter, pCCXPacket[1], iapp, sizeof(IE_TRAFFIC_STREAM_METRIC_REPORT_IAPP));

        pDsWQS->Action = 0;
	 pDsWQS->AC = 3;
	
        Status = PrepareAndSendCommand(
                                   Adapter,
                                   HostCmd_CMD_802_11_WMM_QUEUE_STATS,
                                   0,
                                   HostCmd_OPTION_USE_INT,
                                   0,
                                   HostCmd_PENDING_ON_NONE,
                                   0,
                                   FALSE,
                                   &bytewriten,
                                   0,
                                   0,
                                   &(pDsWQS->Action));
		
        if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
        {     
                DBGPRINT(DBG_CCX_V4, (L"Clean WMM_QUEUE_STATS fail \r\n")); 
        } 
		
        return;
}


///CCX_RADIOMEASURE
VOID wlan_ccx_RxPktFilter(PMRVDRV_ADAPTER pAdapter, UCHAR* bufpt)
{
	PKTPARAM        pktParam;
                    
	wlan_ccx_parse_iapp(pAdapter, bufpt, &pktParam);
	if (pktParam.pkttype != CCXPKT_BEACON_REQUEST) {
		goto FuncFinal;
	}
	DBGPRINT(DBG_ROAM|DBG_CCX|DBG_HELP,(L"CCXPKT: BEACON_REQUEST\n"));
	if (pktParam.beaconReq.Operation == BEACON_TABLE) {
		DBGPRINT(DBG_ROAM|DBG_CCX|DBG_HELP,(L"Reporting Beacon-Table\n"));
		wlan_ccx_send_BeaconTable(pAdapter);
		goto FuncFinal;
	} else {
		DBGPRINT(DBG_ROAM|DBG_CCX|DBG_HELP,(L"Operation: \n", pktParam.beaconReq.Operation));
	}
		
	switch (pktParam.beaconReq.Operation) {
	case PASSIVE_SCAN:
		DBGPRINT(DBG_CUSTOM,(L"CCX set passive scan: \r\n"));
		pAdapter->isPassiveScan = TRUE;
		break;
	case ACTIVE_SCAN:
		pAdapter->isPassiveScan = FALSE;
		break;
	default:
		break;
	}
	pAdapter->MeasureDurtion = pktParam.beaconReq.MeasureDuration;
	pAdapter->ChannelNum = pktParam.beaconReq.ChannelNumber;
	PrepareAndSendCommand(
                                                    pAdapter,
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

FuncFinal:
	return;
}
///CCX_RADIOMEASURE


///#endif ///CCX

