/******************* (c) Marvell Semiconductor, Inc., 2006 ********************
 * File: wlan_roam.c
 * Purpose:
 *      This file contains implementation for roaming
 *
 *
 *****************************************************************************/
#include "precomp.h"
#include "wlan_roam.h"
#include "wlan_thread.h"

#define WROAM_INITID_ON_TABLE				(-1)
#define WROAM_INIT_RSSI_THRESHOLD		(10)			///default threshold: 10dbm

#define WROAM_MIN_TESTED_RSSI			(-88	)		///Minimun RSSI (about to be disconnected, from test)
#define WROAM_MAX_TESTED_RSSI			(-21)		///Maximum RSSI (from test)

///==============================================
///Parameters for Scan
///
/// - Parameters for BG-Scan
typedef VOID (*UPDATESCANINTERVALFUNC)	(VOID*, ULONG);
typedef VOID (*SETSCANDONE)					(VOID*);
typedef VOID (*UPDATESCANTABLEFUNC)		(VOID*, BOOLEAN enable);

typedef struct _bgscan_param
{
	HANDLE		hEventScanComplete;		///event of scan_complete
	WORD		ChannelList;
	PMRVDRV_ADAPTER	pAdapter;
	ULONG		ScanIntervalPerChannel;		///Scan interval per channel (ms)

} BGSCANPARAM, *PBGSCANPARAM;

/// - Parameters for Normal-Scan
typedef struct _normal_scan_param
{
	HANDLE		hEventScanComplete;		///event of scan_complete
	WORD		ChannelList;
	USHORT		Channel;
	PMRVDRV_ADAPTER	pAdapter;
	ULONG		ScanIntervalPerChannel;

} NSCANPARAM, *PNSCANPARAM;


/// Background Scan
static VOID wlan_roam_BGScan_Init(PBGSCANPARAM pScanParam, PWROAMPARAM pparam);
static VOID wlan_roam_BGScan_deInit(PBGSCANPARAM pScanParam);
static VOID wlan_roam_BGScan_SetInterval(PBGSCANPARAM pScanParam, ULONG intervalms);
static VOID wlan_roam_BGScan_SetScanDone(PBGSCANPARAM pScanParam);
static VOID wlan_roam_UseBGScan(PBGSCANPARAM pScanParam, BOOLEAN enable);

/// Normal Scan
static VOID wlan_roam_NScan_Init(PNSCANPARAM pScanParam, PWROAMPARAM pparam);
static VOID wlan_roam_NScan_deInit(PNSCANPARAM pScanParam);
static VOID wlan_roam_NScan_SetInterval(PNSCANPARAM pScanParam, ULONG intervalms);
static VOID wlan_roam_NScan_SetScanDone(PNSCANPARAM pScanParam);
static VOID wlan_roam_UseNormalScan(PNSCANPARAM pScanParam, BOOLEAN enable);
////Others
static VOID mrv_roam_FindNextAP(PWROAMPARAM pparam, PMRVDRV_ADAPTER pAdapter, PAPINFO pAPInfo);
///==============================================

typedef struct _wroam_param
{
	BOOLEAN				initialized;				///Is the parameter initialized
	WROAM_STATE		roam_state;				///Roaming State

	PMRVDRV_ADAPTER	pAdapter;
	HANDLE				hEventThreadSleep;		///event of the roaming thread (to wakeup the thread)
	PWROAMEXTPARAM	pExtParam;				///External parameters to initial the roaming parameters

	///int					idOnAPTable;			///Which AP on the table is the connected one
	NDIS_802_11_RSSI	rssi_threshold;			///RSSI Threshold. Roam to the AP if it's more than this

	WROAM_SCAN_ALGORITHM	ScanAlgorithm;		///Which scan algorithm we will use
	///BOOLEAN				isRoaming;				///We are in the middle of roaming process

	UPDATESCANINTERVALFUNC UpdateScanInterval;	///Function pointer to update the scan interval
	SETSCANDONE			SetScanDone;		///Function pointer, called to be informed "Scan-Completely"
	UPDATESCANTABLEFUNC	UpdateScanTable;	///Funtion pointer to update the scan table
	VOID*					pScanParam;
        NDIS_802_11_SSID                    CurrentSSID;    ///Current Associated SSID

        RECONNECTARG                ReconnectArg;           ///Parameters to reconnect to a new AP
} WROAMPARAM, *PWROAMPARAM;

static const TCHAR	rmstate_string[WRS_MAX_STATE][30] = 
							{L"WRS_NOT_CONNECT",
							L"WRS_STABLE",
							L"WRS_ROAM"
};

static WROAMPARAM	wlan_roam[1];
static NSCANPARAM	nscan_param[1];
static BGSCANPARAM	bgscan_param[1];

///========================================================
static VOID mrv_roam_Thread(IN PVOID pContext);
///Inform the module to enter the stable state
static VOID wlan_roam_set_stableState(PWROAMPARAM);
///Inform the module to enter the roaming state
static VOID wlan_roam_set_roamState(PWROAMPARAM);
///Inform the module to enter the disconnected state
static VOID wlan_roam_set_disconnectState(PWROAMPARAM);

///Update the RSSI of the current associated AP
static VOID wlan_roam_update_rssi(PWROAMPARAM pparam);

///----------------------------------------------------------------
///Update the AP table in the environment
static VOID wlan_roam_UpdateScanTable(PWROAMPARAM pparam, BOOLEAN enable);

///Connect to another AP
///static VOID wlan_roam_connect(PWROAMPARAM pparam, int nextAPId);

///Interval between each roaming behaviors (ms)
static ULONG	wlan_roam_GetInterval(PWROAMPARAM pparam);

///========================================================
///Initialize the roaming module
PWROAMPARAM wlan_roam_init(PMRVDRV_ADAPTER pAdapter, PWROAMEXTPARAM pExtParam)
{
	PWROAMPARAM		pparam = &wlan_roam[0];

	if (pparam->initialized == TRUE) {
		///This module has been initialized. 
		DBGPRINT(DBG_ROAM|DBG_ERROR,(L"->Roam: Roaming module has been initialized\n"));
		pparam = NULL;
		goto funcFinal;
	}
	if (pExtParam == NULL) {
		///This module has been initialized. 
		DBGPRINT(DBG_ROAM|DBG_ERROR,(L"->Roam: Roaming can't be initalized (parameter error)\n"));
		pparam = NULL;
		goto funcFinal;
	}
	///wlan_roam_reset_param(pparam, pAdapter, pExtParam);
	NdisZeroMemory(pparam, sizeof(WROAMPARAM));
	pparam->pAdapter = pAdapter;
	pparam->pExtParam = pExtParam;
	///pparam->idOnAPTable = WROAM_INITID_ON_TABLE;
	pparam->rssi_threshold = pExtParam->diffRSSI;
	wlan_roam_SetScanAlgorithm(pparam, pExtParam->ScanAlgorithm);
	
	///----------------------------------------------------
	///Create the thread to handle the roaming
	///pparam->hEventThreadSleep = wlan_create_thread(mrv_roam_Thread, pparam, DEFAULT_THREAD_PRIORITY);
	pparam->hEventThreadSleep = wlan_create_thread(mrv_roam_Thread, pparam, 90);
	if (pparam->hEventThreadSleep == NULL) {
		DBGPRINT(DBG_ROAM|DBG_ERROR,(L"->Roam: Roaming thread can't be created\n"));
		pparam = NULL;
		goto funcFinal;
	}

	///Initial state of the roaming is "Not_Connected"
	wlan_roam_set_state(pparam, WRS_NOT_CONNECT);

        ///Initialize the reconnect method
        wlan_reconnect_init(&pparam->ReconnectArg);
	///Disable the variables which may enable the BG-Scan => testing~
	pAdapter->bBgScanEnabled = FALSE;
	//pAdapter->bActiveRoamingwithBGSCAN = FALSE;
	//pAdapter->bPreActiveRoamingwithBGSCAN = FALSE;
    
	pparam->initialized = TRUE;
funcFinal:
	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: EndOf wlan_roam_init, (%x)\n", (int)pparam));
	return pparam;
}

///Deinitialize the roaming module
void wlan_roam_deinit(PWROAMPARAM pparam)
{
	if (pparam->initialized == FALSE) {
		DBGPRINT(DBG_ROAM|DBG_WARNING,(L"->Roam: Roaming module has been deinitialized\n"));
		goto funcFinal;
	}
	///Set the state back to Disconnectd
	wlan_roam_set_state(pparam, WRS_NOT_CONNECT);

	if (pparam->pScanParam != NULL) {
		switch (pparam->ScanAlgorithm) {
			case WRSCAN_BGSCAN:
				wlan_roam_BGScan_deInit(pparam->pScanParam);
				break;
			case WRSCAN_NORMALSCAN:
				wlan_roam_NScan_deInit(pparam->pScanParam);
				break;
			default:
				wlan_roam_BGScan_deInit(pparam->pScanParam);
				break;
		}
		pparam->pScanParam = NULL;
	}
    wlan_reconnect_deinit(&pparam->ReconnectArg);

	///Close the thread we created for roaming
	wlan_destroy_thread(pparam->hEventThreadSleep);

	pparam->initialized = FALSE;
	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: Roaming module is deinitialized successfully\n"));
funcFinal:
	return;		
}


///Transite to the different state
VOID wlan_roam_set_state(PWROAMPARAM pparam, WROAM_STATE rmstate)
{
	if (rmstate == pparam->roam_state) {
		DBGPRINT(DBG_ROAM|DBG_WARNING,(L"->Roam: wlan_roam_set_state: Transite to the same state(%s)\n", rmstate_string[rmstate]));
		goto funcFinal;
	}
	switch (rmstate) 
	{
		case WRS_NOT_CONNECT:
			wlan_roam_set_disconnectState(pparam);
			break;
		case WRS_STABLE:
			wlan_roam_set_stableState(pparam);
			break;
		case WRS_ROAM:
			wlan_roam_set_roamState(pparam);
			break;
		default:
			DBGPRINT(DBG_ROAM|DBG_WARNING,(L"->Roam: wlan_roam_set_state: Undefined state(%d)\n", rmstate));
			break;
	}
funcFinal:
	return;
}

///Query if we are in connecting status
BOOLEAN wlan_roam_query_isConnecting(PWROAMPARAM pparam)
{
	///return pparam->isRoaming;
	return pparam->ReconnectArg.FastRoamProgressing;
}

VOID	wlan_roam_ScanTableReady(PWROAMPARAM pparam, WROAM_SCAN_ALGORITHM ScanType)
{
	DBGPRINT(DBG_ROAM | DBG_HELP, (L"->Roam: set Scan done!\n"));
	if (ScanType == pparam->ScanAlgorithm) {
		pparam->SetScanDone(pparam->pScanParam);
	} else {
		DBGPRINT(DBG_ROAM | DBG_ERROR, (L"->Roam: Incorrect Scan Type, (WeGet,Exp)=(%d, %d)!\n", ScanType, pparam->ScanAlgorithm));
	}
	
	return;
}

VOID	wlan_roam_SetScanAlgorithm(PWROAMPARAM pparam, WROAM_SCAN_ALGORITHM	ScanAlgorithm)
{
	if ((pparam->pScanParam != NULL) && (ScanAlgorithm == pparam->ScanAlgorithm)) {
		///We have already been in status we expected
		///Nothing to do. Leave now.
		goto funcFinal;
	}

	///Clear the old parameter, if it exists	
	if (pparam->pScanParam != NULL) {
		///We are changing the scan algorithm, destroy and clear the old one
		switch (pparam->ScanAlgorithm) {
			case WRSCAN_BGSCAN:
				wlan_roam_BGScan_deInit(pparam->pScanParam);
				break;
			case WRSCAN_NORMALSCAN:
				wlan_roam_NScan_deInit(pparam->pScanParam);
				break;
			default:
				wlan_roam_BGScan_deInit(pparam->pScanParam);
				break;
		}
		pparam->pScanParam = NULL;
	}

	///Create the new parameters we expected	
	switch (ScanAlgorithm) {
		case WRSCAN_BGSCAN:
			DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: Using background-scan\n"));
			pparam->pScanParam = (VOID*)(&bgscan_param[0]);
			NdisZeroMemory(pparam->pScanParam, sizeof(BGSCANPARAM));
			wlan_roam_BGScan_Init(pparam->pScanParam, pparam);
			break;
		case WRSCAN_NORMALSCAN:
			DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: Using normal-scan\n"));
			pparam->pScanParam = (VOID*)(&nscan_param[0]);
			NdisZeroMemory(pparam->pScanParam, sizeof(NSCANPARAM));
			wlan_roam_NScan_Init(pparam->pScanParam, pparam);
			break;
		default:
			///Default: Using BG_Scan
			DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: (default) Using background-scan\n"));
			pparam->pScanParam = (VOID*)(&bgscan_param[0]);
			NdisZeroMemory(pparam->pScanParam, sizeof(BGSCANPARAM));
			wlan_roam_BGScan_Init(pparam->pScanParam, pparam);
	}
	pparam->ScanAlgorithm = ScanAlgorithm;
	
funcFinal:
	return;
}
///========================================================
///
///Inform the module to enter the stable state
///
///Tasks in the Stable state:
///		- Register the RSSI_LOW
///		
static VOID wlan_roam_set_stableState(PWROAMPARAM pparam)
{
	PMRVDRV_ADAPTER     pAdapter = pparam->pAdapter;
	///					i;
	
	pparam->roam_state = WRS_STABLE;	
	wlan_roam_UpdateScanTable(pparam, FALSE);
	///Register the RSSI_LOW
	pparam->pAdapter->EventRecord.RSSILowValue  = (UCHAR)pparam->pExtParam->RoamSST;
	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: wlan_roam_set_stable: register RSSI_LOW(%d)\n", pparam->pAdapter->EventRecord.RSSILowValue));	
	SetUpLowRssiValue(pparam->pAdapter);

	///for (i=0 ; i<(int)pAdapter->ulPSNumOfBSSIDs ; i++) {
	///	if (NdisEqualMemory(pAdapter->PSBSSIDList[i].MacAddress, pAdapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN)) {
	///		break;
	///	}
	///}
	///if (i ==  pAdapter->ulPSNumOfBSSIDs) {
	///	DBGPRINT(DBG_ROAM|DBG_ERROR,(L"->Roam: Can't find the associated AP on the list\n"));
	///} else {
	///	DBGPRINT(DBG_ROAM|DBG_ERROR,(L"->Roam: The associated AP is at item (%d) on the list\n", i));
	///	pparam->idOnAPTable = i;
	///}
	if (pAdapter->WmmDesc.WmmUapsdEnabled == TRUE) {
		///If the connected AP is UAPSD enabled, always use the active-scan
		wlan_roam_SetScanAlgorithm(pparam, WRSCAN_NORMALSCAN);
	}

	return;
}

///
///Inform the module to enter the roaming state
///
///Tasks in the Roaming state:
///		- Register the RSSI_HIGH
///		- Wake up the thread to process the roaming tasks
///		
static VOID wlan_roam_set_roamState(PWROAMPARAM pparam)
{
	pparam->roam_state = WRS_ROAM;
	///Register the RSSI_HIGH
	pparam->pAdapter->EventRecord.RSSIHighValue  = (UCHAR)pparam->pExtParam->RoamSST;
	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: wlan_roam_set_roam: register RSSI_HIGH(%d)\n", pparam->pAdapter->EventRecord.RSSIHighValue));

	SetUpHighRssiValue(pparam->pAdapter);
	///Wake up the thread to process the roaming tasks
	SetEvent(pparam->hEventThreadSleep);

	return;
}

///
///Inform the module to enter the disconnected state
///
static VOID wlan_roam_set_disconnectState(PWROAMPARAM pparam)
{
	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: wlan_roam_set_disconnectState: %x\n", pparam));
	
	pparam->roam_state = WRS_NOT_CONNECT;
	///pparam->idOnAPTable = WROAM_INITID_ON_TABLE;
	pparam->rssi_threshold = WROAM_INIT_RSSI_THRESHOLD;

	///SetEvent(pparam->hEventScanComplete);
	wlan_roam_UpdateScanTable(pparam, FALSE);
	return;
}

///------------------------------------------------------------------------

///========================================================
static VOID wlan_roam_BGScan_Init(PBGSCANPARAM pScanParam, PWROAMPARAM pparam)
{
	NdisZeroMemory(pScanParam, sizeof(BGSCANPARAM));

	if (pparam->pExtParam == NULL) {
		DBGPRINT(DBG_ROAM | DBG_ERROR, (L"->Roam: (bgscan)ExtParam should not be NULL\n"));
		goto funcFinal;
	}
	pScanParam->ChannelList = pparam->pExtParam->ChannelList & 0x3fff;
	pScanParam->pAdapter = pparam->pAdapter;
	pScanParam->hEventScanComplete = CreateEvent(NULL, FALSE, FALSE,NULL);
	if (pScanParam->hEventScanComplete == NULL) {
		DBGPRINT(DBG_ROAM|DBG_ERROR,(L"->Roam: BG-Scan create Event failed\n"));
		pScanParam = NULL;
		goto funcFinal;
	}
	pparam->UpdateScanInterval = wlan_roam_BGScan_SetInterval;
	pparam->SetScanDone = wlan_roam_BGScan_SetScanDone;
	pparam->UpdateScanTable = wlan_roam_UseBGScan;

funcFinal:
	return;
}

static VOID wlan_roam_BGScan_deInit(PBGSCANPARAM pScanParam)
{
	if (pScanParam->hEventScanComplete != NULL) {
		SetEvent(pScanParam->hEventScanComplete);
		///To yield the resource
		NdisMSleep(1);
		CloseHandle(pScanParam->hEventScanComplete);
		pScanParam->hEventScanComplete = NULL;
	}

	return;
}
static VOID wlan_roam_BGScan_CurrentSSID(	PBGSCANPARAM pScanParam)
{
	PMRVDRV_ADAPTER pAdapter = pScanParam->pAdapter;
	ULONG				ScanInterval = pScanParam->ScanIntervalPerChannel;
	POID_MRVL_DS_BG_SCAN_CONFIG	pNewCfg;
	
	MrvlIEtypes_SsIdParamSet_t			*pSsid;
	MrvlIEtypes_ChanListParamSet_t		*pChan;
	ChanScanParamSet_t				*pChaninfo; 
	MrvlIEtypes_NumProbes_t			*pNumProbes;
	MrvlIEtypes_BcastProbe_t			*pBcProb;
	MrvlIEtypes_SnrThreshold_t			*pSnrThreshold;   
	MrvlIEtypes_NumSSIDProbe_t		*pNumSSIDProbes;
	PREGION_CHANNEL				region_channel;

	DBGPRINT(DBG_ROAM|DBG_HELP, (L"BG-SCAN interval: %d ms\n", ScanInterval));
       pNewCfg =(POID_MRVL_DS_BG_SCAN_CONFIG)(pAdapter->BgScanCfg);
	pNewCfg->CmdLen 			= 0;

	*(USHORT *)(&pNewCfg->Action)				= 1;

	pNewCfg->Enable                       = 1;
	pNewCfg->BssType			= 3;        		/// 1:Infrastructure, 2: IBSS, 3:Any
	pNewCfg->ChannelsPerScan		= 14;			/// NumOfChannel per scan (14=max)
	pNewCfg->DiscardWhenFull         = 0;				/// 0:Discard old result, 1:discard new result
	pNewCfg->ScanInterval       	= ScanInterval;	/// Scan interval (ms)
	pNewCfg->StoreCondition		= 1;				/// bit0:SSID match (), bit1:SSID match & SNR above threshold
	pNewCfg->ReportConditions		= 1;				/// bit0:SSID match (), bit1:SSID match & SNR above threshold 
	pNewCfg->MaxScanResults            = 14;			/// MAX scan results that will trigger a scan_complete event
	pNewCfg->CmdLen = sizeof(OID_MRVL_DS_BG_SCAN_CONFIG) -sizeof(ULONG) - sizeof(USHORT) -1;

	//ssid 
	pSsid = (MrvlIEtypes_SsIdParamSet_t    *)(&(pNewCfg->TlvData[0]));
	pSsid->Header.Type = TLV_TYPE_SSID;
	pSsid->Header.Len  = 0;
	NdisMoveMemory ( pSsid->SsId,
                     pAdapter->CurrentSSID.Ssid,
                     pAdapter->CurrentSSID.SsidLength );
	
	pSsid->Header.Len = (USHORT)pAdapter->CurrentSSID.SsidLength;  
	pNewCfg->CmdLen += (sizeof(MrvlIEtypesHeader_t) + pSsid->Header.Len);

	//channel
	pChan = (MrvlIEtypes_ChanListParamSet_t *)
                    ((UCHAR *)pSsid + sizeof(MrvlIEtypesHeader_t) + pSsid->Header.Len);
       pChan->Header.Type = TLV_TYPE_CHANLIST;
       pChan->Header.Len = 0;

	pChaninfo =  (ChanScanParamSet_t *)((UCHAR *)pChan + sizeof(MrvlIEtypesHeader_t));
	region_channel = &(pAdapter->region_channel[pAdapter->connected_band]); 

       // fill channel info
       {
	       int			i;
		UCHAR		channelNum;
		WORD		ChannelList = pAdapter->wlanRoamExtParam.ChannelList;
		
		DBGPRINT(DBG_ROAM|DBG_HELP, (L"ChannelList: %xh\n", ChannelList));
		for (i=0, channelNum=1 ; i<14 ; i++, channelNum++) {
			if ((ChannelList& 0x01) == 0) {
				ChannelList >>= 1;
				continue;
			}
			DBGPRINT(DBG_ROAM|DBG_HELP, (L"Will Scan Channel(%d)\n", (i+1)));
			if ((region_channel->Band == MRVDRV_802_11_BAND_B) ||
				(region_channel->Band == MRVDRV_802_11_BAND_BG))
			{
				pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_BG;
			}
			else if (region_channel->Band == MRVDRV_802_11_BAND_A)
			{
				pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_A;
			}
			pChaninfo->ChanNumber  = (i+1); 
			pChaninfo->ScanType    = HostCmd_SCAN_TYPE_ACTIVE;
			pChaninfo->MinScanTime = 6;
			pChaninfo->ScanTime    = HostCmd_SCAN_MIN_CH_TIME;
			pChan->Header.Len += sizeof(ChanScanParamSet_t);

			///------------------------------------------------------
			pChaninfo = pChaninfo + 1;
			ChannelList >>= 1;
			
		}
       }
/*	   
	if ((region_channel->Band == MRVDRV_802_11_BAND_B) ||
	(region_channel->Band == MRVDRV_802_11_BAND_BG))
	{
		pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_BG;
	}
	else if (region_channel->Band == MRVDRV_802_11_BAND_A)
	{
		pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_A;
	}
RETAILMSG(1, (L"connected_channel: %d\n", pAdapter->connected_channel));
	pChaninfo->ChanNumber  = pAdapter->connected_channel; 
	pChaninfo->ScanType    = HostCmd_SCAN_TYPE_ACTIVE;
	pChaninfo->MinScanTime = 6;
	pChaninfo->ScanTime    = HostCmd_SCAN_MIN_CH_TIME;
	pChan->Header.Len += sizeof(ChanScanParamSet_t);
*/
	///===============
	/*
	pChaninfo = pChaninfo + 1;
	region_channel = &(pAdapter->region_channel[pAdapter->connected_band]); 

       // fill channel info
	if ((region_channel->Band == MRVDRV_802_11_BAND_B) ||
	(region_channel->Band == MRVDRV_802_11_BAND_BG))
	{
		pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_BG;
	}
	else if (region_channel->Band == MRVDRV_802_11_BAND_A)
	{
		pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_A;
	}
RETAILMSG(1, (L"Connected Channel: %d\n", pAdapter->connected_channel));
///	pChaninfo->ChanNumber  = pAdapter->connected_channel; 
	pChaninfo->ChanNumber  = 4; 
	pChaninfo->ScanType    = 3;
	pChaninfo->MinScanTime = 6;
	pChaninfo->ScanTime    = HostCmd_SCAN_MIN_CH_TIME;
	pChan->Header.Len += sizeof(ChanScanParamSet_t);
*/
	///===============

	pNewCfg->CmdLen += (sizeof(MrvlIEtypesHeader_t) + pChan->Header.Len);



    
	//numprobes             
	pNumProbes = (MrvlIEtypes_NumProbes_t *)
                           ((UCHAR *)pChan + sizeof(MrvlIEtypesHeader_t) + pChan->Header.Len);
	pNumProbes->Header.Type = TLV_TYPE_NUMPROBES;
	pNumProbes->Header.Len   = sizeof(USHORT);
	pNumProbes->NumProbes   = 1;

	pNewCfg->CmdLen += (sizeof(MrvlIEtypesHeader_t) + pNumProbes->Header.Len);

	//snr
	pSnrThreshold=(MrvlIEtypes_SnrThreshold_t *)
	                   ((UCHAR *)pNumProbes + sizeof(MrvlIEtypesHeader_t) + pNumProbes->Header.Len);

	pSnrThreshold->Header.Type = TLV_TYPE_SNR;
	pSnrThreshold->Header.Len = sizeof(USHORT);
	pSnrThreshold->SNRValue=40;
	///pSnrThreshold->SNRFreq = 1; 
	pSnrThreshold->SNRFreq = 0; 

	pNewCfg->CmdLen += (sizeof(MrvlIEtypesHeader_t) + pSnrThreshold->Header.Len);

	//BroadcastProbe
	pBcProb=(MrvlIEtypes_BcastProbe_t *)
	                   ((UCHAR *)pSnrThreshold + sizeof(MrvlIEtypesHeader_t) + pSnrThreshold->Header.Len);
	pBcProb->Header.Type = TLV_TYPE_BCASTPROBE;
	pBcProb->Header.Len = sizeof(USHORT);
	pBcProb->BcastProbe = 0; 

	pNewCfg->CmdLen += (sizeof(MrvlIEtypesHeader_t) + pBcProb->Header.Len);     

	//NumSSIDProbe      
	pNumSSIDProbes = (MrvlIEtypes_NumSSIDProbe_t *)
	              ((UCHAR *)pBcProb + sizeof(MrvlIEtypesHeader_t) + pBcProb->Header.Len);
	pNumSSIDProbes->Header.Type = TLV_TYPE_NUMSSID_PROBE;
	pNumSSIDProbes->Header.Len  =  sizeof(USHORT);
	pNumSSIDProbes->NumSSIDProbe   = 1;

	pNewCfg->CmdLen += (sizeof(MrvlIEtypesHeader_t) + pNumSSIDProbes->Header.Len);    
	return;
}

static VOID wlan_roam_BGScan_SetInterval(PBGSCANPARAM pScanParam, ULONG intervalms)
{
	pScanParam->ScanIntervalPerChannel = intervalms;
	return;
}

static VOID wlan_roam_BGScan_SetScanDone(PBGSCANPARAM pScanParam)
{
	SetEvent(pScanParam->hEventScanComplete);
	return;
}

static VOID wlan_roam_UseBGScan(PBGSCANPARAM pparam, BOOLEAN enable)
{
	PMRVDRV_ADAPTER     pAdapter = pparam->pAdapter;

	if (enable == TRUE) {
		///Enable background scan
		ResetEvent(pparam->hEventScanComplete);
		EnableBgScan(pAdapter,FALSE);
		wlan_roam_BGScan_CurrentSSID(pparam);
		EnableBgScan(pAdapter,TRUE);
		DBGPRINT(DBG_ROAM | DBG_HELP, (L"->Roam: bgscan-enter waiting\n"));
		WaitForSingleObject(pparam->hEventScanComplete, INFINITE);
		DBGPRINT(DBG_ROAM | DBG_HELP, (L"->Roam: bgscan-waken\n"));
	} else {
		///Disable background scan
		EnableBgScan(pAdapter,FALSE);
		SetEvent(pparam->hEventScanComplete);
		///To yield the resource
		NdisMSleep(1);
	}
	return;
}

///--------------------------------------------------------
static VOID wlan_roam_NScan_Init(PNSCANPARAM pScanParam, PWROAMPARAM pparam)
{
	NdisZeroMemory(pScanParam, sizeof(NSCANPARAM));

	if (pparam->pExtParam == NULL) {
		DBGPRINT(DBG_ROAM | DBG_ERROR, (L"->Roam: ExtParam should not be NULL\n"));
		goto funcFinal;
	}
	pScanParam->ChannelList = pparam->pExtParam->ChannelList & 0x3fff;
	pScanParam->Channel = 0;
	pScanParam->pAdapter = pparam->pAdapter;
	pScanParam->hEventScanComplete = CreateEvent(NULL, FALSE, FALSE,NULL);
	if (pScanParam->hEventScanComplete == NULL) {
		DBGPRINT(DBG_ROAM|DBG_ERROR,(L"->Roam: Normal-Scan create Event failed\n"));
		pScanParam = NULL;
		goto funcFinal;
	}

	pparam->UpdateScanInterval = wlan_roam_NScan_SetInterval;
	pparam->SetScanDone = wlan_roam_NScan_SetScanDone;
	pparam->UpdateScanTable = wlan_roam_UseNormalScan;

funcFinal:
	return;
}

static VOID wlan_roam_NScan_deInit(PNSCANPARAM pScanParam)
{
	if (pScanParam->hEventScanComplete != NULL) {
		SetEvent(pScanParam->hEventScanComplete);
		///To yield the resource
		NdisMSleep(1);
		CloseHandle(pScanParam->hEventScanComplete);
		pScanParam->hEventScanComplete = NULL;
	}

	return;
}

static VOID wlan_roam_NScan_SetInterval(PNSCANPARAM pScanParam, ULONG intervalms)
{
	pScanParam->ScanIntervalPerChannel = intervalms;
	return;
}

///
///	To get the next channel we would like to scan
///		0: There is no channel to be scaned
///
static  USHORT wlan_roam_NScan_GetNextChannel(PNSCANPARAM	pScanParam)
{
	USHORT	i;
	WORD	ChannelList = (pScanParam->ChannelList >> pScanParam->Channel);

	if (ChannelList == 0) {
		///Reset the the initial state
		ChannelList = pScanParam->ChannelList;
		pScanParam->Channel = 0;
	}
	for (i = pScanParam->Channel ;i < 14; i++) {
		if (ChannelList & 0x01) {
			pScanParam->Channel = i+1;
			break;
		}
            ChannelList >>= 1;
	}

	if (i == 14) {
		DBGPRINT(DBG_ROAM | DBG_ERROR, (L"->Roam: Can't get the valid channel(%xh, %d)\n",
                    pScanParam->ChannelList,
                    pScanParam->Channel));
		pScanParam->Channel = 0;
	} else {
	        DBGPRINT(DBG_ROAM | DBG_HELP, (L"->Roam: Scanning channel(%d)\n", pScanParam->Channel));
	}
	return pScanParam->Channel;
}

static VOID wlan_roam_NScan_SetScanDone(PNSCANPARAM pScanParam)
{
	SetEvent(pScanParam->hEventScanComplete);
	return;
}

static VOID wlan_roam_UseNormalScan(PNSCANPARAM pparam, BOOLEAN enable)
{
	PMRVDRV_ADAPTER     pAdapter = pparam->pAdapter;
	
	if (enable == TRUE) {
		///Enable the scan function
		ResetEvent(pparam->hEventScanComplete);
		pAdapter->SpecifyChannel = wlan_roam_NScan_GetNextChannel(pparam);
		pAdapter->ChannelMaxDuration = HostCmd_SCAN_MAX_CH_TIME;
		pAdapter->ChannelMinDuration = HostCmd_SCAN_MIN_CH_TIME;
		pAdapter->bIsScanWhileConnect = TRUE;
		pAdapter->SetActiveScanSSID = TRUE;
		pAdapter->bIsAssociationBlockedByScan = TRUE;
		NdisMoveMemory(&pAdapter->ActiveScanSSID, &pAdapter->CurrentSSID, sizeof(pAdapter->CurrentSSID));

		DBGPRINT(DBG_ROAM | DBG_HELP, (L"->Roam: Normal-scan Channel(%d)\n", pAdapter->SpecifyChannel));
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
		DBGPRINT(DBG_ROAM | DBG_HELP, (L"->Roam: Enter wait for HostCmd_CMD_802_11_SCAN CMD\n"));
		WaitForSingleObject(pparam->hEventScanComplete, INFINITE);
		DBGPRINT(DBG_ROAM | DBG_HELP, (L"->Roam: Exit-Wait\n"));
	} else {
		///Disable the scan function
		SetEvent(pparam->hEventScanComplete);
		///To yield the resource
		NdisMSleep(1);
	}
	return;
}

static VOID wlan_roam_UpdateScanTable(PWROAMPARAM pparam, BOOLEAN enable)
{
	PMRVDRV_ADAPTER     pAdapter = pparam->pAdapter;
	
	///DBGPRINT(DBG_ROAM|DBG_HELP, (L"->Roam: CurrSSID(%d):%s\n", pAdapter->CurrentSSID.SsidLength, pAdapter->CurrentSSID.Ssid));
	if (pparam->UpdateScanTable == NULL) {
		DBGPRINT(DBG_ROAM|DBG_ERROR, (L"->Roam: Scan Function is missing\n"));
		goto funcFinal;
	}

	pparam->UpdateScanInterval(pparam->pScanParam, wlan_roam_GetInterval(pparam));
	pparam->UpdateScanTable(pparam->pScanParam, enable);

funcFinal:
	return;
}

///
///Interval between each roaming behaviors
///According to our test by (sdio-8686, 9.70.3.p0-37.p4), 
///	RSSI range is (good, almost_disconnected) = (-21, -88)
///	Set to mapping this range to (WROAM_MAX_UPDATE_INTERVAL ~ WROAM_MIN_UPDATE_INTERVAL)
static ULONG	wlan_roam_GetInterval(PWROAMPARAM pparam)
{
	///ULONG	updateinterval = ((WROAM_MAX_UPDATE_INTERVAL - WROAM_MIN_UPDATE_INTERVAL));
	PWROAMEXTPARAM	pExtParam = pparam->pExtParam;
	ULONG	updateinterval = (pExtParam->MaxScanInterval- pExtParam->MinScanInterval);
	///Note: pparam->pExtParam->RoamSST is positive
	ULONG	RssiInterval = -((LONG)pparam->pExtParam->RoamSST + WROAM_MIN_TESTED_RSSI);

	PMRVDRV_ADAPTER     pAdapter = pparam->pAdapter;
	ULONG		delayinterval;

	///DBGPRINT(1, (L"(RoamSST, RssiInterval)=(%d, %d)\n", pparam->pExtParam->RoamSST, RssiInterval));
	///DBGPRINT(1, (L"(RSSI)=(%d)\n", pAdapter->RSSI[TYPE_BEACON][TYPE_NOAVG]));
	///DBGPRINT(1, (L"(updateinterval)=(%d)\n", updateinterval));

	if (pAdapter->RSSI[TYPE_BEACON][TYPE_NOAVG] < WROAM_MIN_TESTED_RSSI) {
            ///RSSI is lower than the boundary. Set the delayinterval as the minimum value
		delayinterval = pExtParam->MinScanInterval;
	} else {
		delayinterval = pExtParam->MinScanInterval + 
			(pAdapter->RSSI[TYPE_BEACON][TYPE_NOAVG] - WROAM_MIN_TESTED_RSSI) * updateinterval / RssiInterval;
	}

	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: update interval: %d ms\n", delayinterval));
	return delayinterval;
}

static VOID mrv_roam_FindNextAP(PWROAMPARAM pparam, PMRVDRV_ADAPTER pAdapter, PAPINFO pAPInfo)
{
	int					i;
	NDIS_802_11_RSSI	diffRSSI;
	int					idOnAPTable = -1;			///Which AP on the table is the connected one

	pAPInfo->id = -1;
	NdisZeroMemory(pAPInfo->MacAddr, sizeof(NDIS_802_11_MAC_ADDRESS));
	///If the RSSI of the candidate is higher than the threshold, connect to that one
	DBGPRINT(DBG_ROAM|DBG_HELP, (L"->Roam: Searching (%02x:%2x:%2x:%2x:%2x:%2x) in AP_table(%d)\n", 
										pAdapter->CurrentBSSID[0], pAdapter->CurrentBSSID[1],
										pAdapter->CurrentBSSID[2], pAdapter->CurrentBSSID[3],
										pAdapter->CurrentBSSID[4], pAdapter->CurrentBSSID[5],
										pAdapter->ulPSNumOfBSSIDs
										));
	///Find "idOnAPTable" first
	for (i=0 ; i<(int)pAdapter->ulPSNumOfBSSIDs ; i++) {
		if (NdisEqualMemory(pAdapter->PSBSSIDList[i].MacAddress, pAdapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN)) {
			///this is the AP we are connected.
			idOnAPTable = i;
			break;
		}
	}
	if (idOnAPTable == -1) {
		DBGPRINT(DBG_ROAM|DBG_ERROR, (L"->Roam: Fatal Error. The associated AP is no on the AP table.....\n"));
		///Continue to update the table again
		pAPInfo->id = -1;
		goto FuncFinal;
	}

	for (i=0 ; i<(int)pAdapter->ulPSNumOfBSSIDs ; i++) {
		///if (NdisEqualMemory(pAdapter->PSBSSIDList[i].MacAddress, pAdapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN)) {
		if (idOnAPTable == i) {
			///this is the AP we are connected.
			DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: This AP(%d) is the one we are connecting\n", i));
			continue;
		} else {
			DBGPRINT(DBG_ROAM|DBG_HELP, (L"->Roam: Checking (%d) (%02x:%2x:%2x:%2x:%2x:%2x) in table\n", 
										i,
										pAdapter->PSBSSIDList[i].MacAddress[0], 
										pAdapter->PSBSSIDList[i].MacAddress[1],
										pAdapter->PSBSSIDList[i].MacAddress[2], 
										pAdapter->PSBSSIDList[i].MacAddress[3],
										pAdapter->PSBSSIDList[i].MacAddress[4], 
										pAdapter->PSBSSIDList[i].MacAddress[5]));
		}

		if (!NdisEqualMemory(&pAdapter->PSBSSIDList[i].Ssid, &pAdapter->CurrentSSID, sizeof(NDIS_802_11_SSID))) {
			///Not our candidate, skip it now
			///DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: Diff SSID, skip it now\n"));
			continue;
		}

		///if (idOnAPTable == -1) {
		///	DBGPRINT(DBG_ROAM|DBG_ERROR, (L"->Roam: Fatal Error.....\n"));
		///	break;
		///}
		///Check the RSSI of the AP we are associating with and the other APs whose SSID is identical with ours
		diffRSSI = pAdapter->PSBSSIDList[i].Rssi - pAdapter->PSBSSIDList[idOnAPTable].Rssi;
		if (diffRSSI < pparam->rssi_threshold) {
			///The RSSI does not exceed the threshld. Still skip it
			DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: (%d, %d) (%d, %d) diffRSSI(%d) is not above the threshold (%ddbm), skip it now\n", 
					i,  idOnAPTable,
					pAdapter->PSBSSIDList[i].Rssi,
					pAdapter->PSBSSIDList[idOnAPTable].Rssi,
					diffRSSI, 
					pparam->rssi_threshold));
			continue;
		}
		///This AP's RSSI is higher than the threshold, 
		///	=> Compare the one we saved. If it's even higher, replace the item
		if (pAPInfo->id == -1) {
			///This is the 1st one. Record it now
			pAPInfo->id = i;
			NdisMoveMemory(pAPInfo->MacAddr, pAdapter->PSBSSIDList[i].MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS));
			DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: 1st Possible Candidate: %d, (%02xh-%02xh-%02xh-%02xh-%02xh-%02xh)\n", 
											i, 
											pAdapter->PSBSSIDList[i].MacAddress[0],
											pAdapter->PSBSSIDList[i].MacAddress[1],
											pAdapter->PSBSSIDList[i].MacAddress[2],
											pAdapter->PSBSSIDList[i].MacAddress[3],
											pAdapter->PSBSSIDList[i].MacAddress[4],
											pAdapter->PSBSSIDList[i].MacAddress[5]));
			continue;
		}
		if (pAdapter->PSBSSIDList[i].Rssi > pAdapter->PSBSSIDList[pAPInfo->id].Rssi) {
			DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: Next Possible Candidate: %d, (%02xh-%02xh-%02xh-%02xh-%02xh-%02xh)\n", 
											i, 
											pAdapter->PSBSSIDList[i].MacAddress[0],
											pAdapter->PSBSSIDList[i].MacAddress[1],
											pAdapter->PSBSSIDList[i].MacAddress[2],
											pAdapter->PSBSSIDList[i].MacAddress[3],
											pAdapter->PSBSSIDList[i].MacAddress[4],
											pAdapter->PSBSSIDList[i].MacAddress[5]));

			pAPInfo->id = i;
			NdisMoveMemory(pAPInfo->MacAddr, pAdapter->PSBSSIDList[i].MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS));
		}
	}
FuncFinal:
	if (pAPInfo->id != -1) {
		DBGPRINT(DBG_ROAM|DBG_HELP, (L"->Roam: mrv_roam_FindNextAP(%d) (%02x:%2x:%2x:%2x:%2x:%2x) in table\n", 
										pAPInfo->id,
										pAdapter->PSBSSIDList[pAPInfo->id].MacAddress[0], 
										pAdapter->PSBSSIDList[pAPInfo->id].MacAddress[1],
										pAdapter->PSBSSIDList[pAPInfo->id].MacAddress[2], 
										pAdapter->PSBSSIDList[pAPInfo->id].MacAddress[3],
										pAdapter->PSBSSIDList[pAPInfo->id].MacAddress[4], 
										pAdapter->PSBSSIDList[pAPInfo->id].MacAddress[5]));
	}
	return;
}
///========================================================
/// A stand alone thread to handle the roaming
///
VOID mrv_roam_Thread(IN PVOID pContext)
{
	PWROAMPARAM 		pparam = (PWROAMPARAM)pContext;
	PMRVDRV_ADAPTER     pAdapter = pparam->pAdapter;
	APINFO				apinfo;

	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: 1,Roaming handler is triggered\n"));
	ResetEvent(pAdapter->hPendOnCmdEvent);
	while (pparam->roam_state == WRS_ROAM) {
		///Adaptive Scan to collect the information of the AP in the environment
		wlan_roam_UpdateScanTable(pparam, TRUE);
		if (pparam->roam_state != WRS_ROAM) {
			///Updating the AP table may take a longer time.
			///If we have already left the roaming state, leave the loop as well now
			break;
		}
		mrv_roam_FindNextAP(pparam, pAdapter, &apinfo);
		if (apinfo.id != -1) {
			///Get the candidate... Roam to the other one
			///wlan_roam_connect(pparam, nextAPId);
			NdisMSleep(100000);		///Delay a while before reconnecting to the new AP
			wlan_reconnect(&pparam->ReconnectArg, pAdapter, &apinfo);
		}

		if (pparam->roam_state != WRS_ROAM) {
			///If we have left the roaming state, leave the loop now
			break;
		}
		///Update the current RSSI Value
		wlan_roam_update_rssi(pparam);
		///Delay a period before the next run
		NdisMSleep(wlan_roam_GetInterval(pparam)*1000);
	}

	///wlan_roam_UpdateScanTable(pparam, FALSE);

	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: Exit Roaming handler, state is set to \"%s\"\n", rmstate_string[pparam->roam_state]));
	return;
}

PRECONNECTARG   wlan_roam_GetReconnectArg(PWROAMPARAM pparam)
{
    return &pparam->ReconnectArg;
}

static VOID wlan_roam_update_rssi(PWROAMPARAM		pparam)
{
	PMRVDRV_ADAPTER     pAdapter = pparam->pAdapter;
	NDIS_STATUS			retstatus;
	int i;
	retstatus = PrepareAndSendCommand(
                  pAdapter,
                  HostCmd_CMD_802_11_RSSI,
                  0,
                  HostCmd_OPTION_USE_INT,
                  (NDIS_OID)0,
                  HostCmd_PENDING_ON_CMD,
                  0, 
                  FALSE, 
                  NULL, 
                  NULL, 
                  NULL, 
                  NULL);
	if (retstatus == NDIS_STATUS_SUCCESS) {
		DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: 	Enter Wait\n"));
		WaitForSingleObject(pAdapter->hPendOnCmdEvent, INFINITE);
		DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: 1, Update RSSI: (%d, %d, %d, %d)\n",
					pAdapter->RSSI[0][0], 
					pAdapter->RSSI[0][1], 
					pAdapter->RSSI[1][0], 
					pAdapter->RSSI[1][1]));
	} else {
		DBGPRINT(DBG_ROAM|DBG_WARNING,(L"->Roam: wlan_roam_update_rssi: Download HostCmd_CMD_802_11_RSSI failed\n"));
	}
	///Update the RSSI back to the AP list
	for (i=0 ; i<(int)pAdapter->ulPSNumOfBSSIDs ; i++) {
		if (NdisEqualMemory(pAdapter->PSBSSIDList[i].MacAddress, pAdapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN)) {
			///this is the AP we are connected.
			pAdapter->PSBSSIDList[i].Rssi = pAdapter->RSSI[TYPE_BEACON][TYPE_NOAVG];
			break;
		}
	}


        return;
}


///========================================================

VOID wlan_Recalculate_APID(PAPINFO	pApInfo, PMRVDRV_ADAPTER pAdapter)
{
	int i;

	for (i=0 ; i<(int)pAdapter->ulPSNumOfBSSIDs ; i++) {
		DBGPRINT(DBG_ROAM|DBG_CCX, (L"==> Recomparing AP(%d)-%02x:%02x:%02x:%02x:%02x:%02x\r\n", 
                                        i, 
                                        pAdapter->PSBssDescList[i].BSSID[0],
                                        pAdapter->PSBssDescList[i].BSSID[1],
                                        pAdapter->PSBssDescList[i].BSSID[2],
                                        pAdapter->PSBssDescList[i].BSSID[3],
                                        pAdapter->PSBssDescList[i].BSSID[4],
                                        pAdapter->PSBssDescList[i].BSSID[5]));
		if (NdisEqualMemory(pAdapter->PSBssDescList[i].BSSID, pApInfo->MacAddr, sizeof(NDIS_802_11_MAC_ADDRESS))) {
			DBGPRINT(DBG_ROAM, (L"==> Reassign APID to :%d\r\n", i));
			pApInfo->id = i;
			break;
		} else {
			DBGPRINT(DBG_ROAM, (L"==> Not match: %d\r\n", i));
		}
	}
	if (i == pAdapter->ulPSNumOfBSSIDs) {
		pApInfo->id = -1;
		DBGPRINT(DBG_ROAM, (L"==> Not matched AP in the list now %d\r\n", pAdapter->ulPSNumOfBSSIDs));
	}
	return;
}

VOID wlan_reconnect_init(PRECONNECTARG  pReconArg)
{
    pReconArg->hroamEvent = CreateEvent(NULL, FALSE, FALSE,NULL);
    pReconArg->FastRoamProgressing = FALSE;
    pReconArg->privacyEnabled = FALSE;
    pReconArg->cckmEnabled = FALSE;
    return;
}

static VOID GetFirmwarTSF(PMRVDRV_ADAPTER pAdapter, PAPINFO pApInfo)
{
	ULONG   byteNeeded = sizeof(HostCmd_DS_802_11_GET_TSF);
	UCHAR   *tsfpt = (UCHAR*)&(pAdapter->PSBssDescList[pApInfo->id].ccx_bss_info.CurrentTSF);

	PrepareAndSendCommand(pAdapter,
                                            HostCmd_CMD_802_11_GET_TSF,
                                            HostCmd_ACT_GET,
                                            HostCmd_OPTION_USE_INT,
                                            0,
                                            HostCmd_PENDING_ON_GET_OID,
                                            0,
                                            FALSE,
                                            &byteNeeded,
                                            0,
                                            &byteNeeded,
                                            &pAdapter->PSBssDescList[pApInfo->id].ccx_bss_info.CurrentTSF);
	WaitForSingleObject( pAdapter->hOidQueryEvent, INFINITE );
	NdisMoveMemory(
				&pAdapter->PSBssDescList[pApInfo->id].ccx_bss_info.CurrentTSF,
				(PUCHAR) (pAdapter->OidCmdRespBuf),
				pAdapter->nSizeOfOidCmdResp );
	DBGPRINT(DBG_CCX, (L"Current TSF: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                                                        tsfpt[0], tsfpt[1], tsfpt[2], tsfpt[3],
                                                        tsfpt[4], tsfpt[5], tsfpt[6], tsfpt[7]));

	return;
}

static VOID SetLastWepKey(PMRVDRV_ADAPTER pAdapter)
{
	NDIS_STATUS     nStatus;
	MRVL_WEP_KEY    *WepKeyPt = &pAdapter->LastAddedWEPKey;

	DBGPRINT(DBG_ROAM|DBG_CCX|DBG_HELP,(L"==> Add the saved WEP key to the roamed AP\r\n"));
	nStatus = PrepareAndSendCommand(
                          pAdapter,
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
                          WepKeyPt);
	DBGPRINT(DBG_CCX|DBG_ROAM|DBG_HELP,(L"HostCmd_CMD_802_11_SET_WEP(nStatus: %xh)\r\n", nStatus));
		
	return;
}

VOID wlan_reconnect( PRECONNECTARG  pReconArg, PMRVDRV_ADAPTER pAdapter, PAPINFO pApInfo)
{
	NDIS_STATUS     nStatus;

	DBGPRINT(DBG_ROAM|DBG_HELP,(L"->Roam: Roaming......(to %dth) (%02xh-%02xh-%02xh-%02xh-%02xh-%02xh)\n", 
											pApInfo->id, 
											pAdapter->PSBSSIDList[pApInfo->id].MacAddress[0],
											pAdapter->PSBSSIDList[pApInfo->id].MacAddress[1],
											pAdapter->PSBSSIDList[pApInfo->id].MacAddress[2],
											pAdapter->PSBSSIDList[pApInfo->id].MacAddress[3],
											pAdapter->PSBSSIDList[pApInfo->id].MacAddress[4],
											pAdapter->PSBSSIDList[pApInfo->id].MacAddress[5]));

	DBGPRINT(DBG_V9|DBG_CCX,(L"==>[HandleFastRoam]Adapter->SentPacket=%xh\r\n", pAdapter->SentPacket));
	if ((pAdapter->InfrastructureMode != Ndis802_11Infrastructure) ||
		(pAdapter->MediaConnectStatus == NdisMediaStateDisconnected)) {
		goto funcFinal;
	}

	NdisGetSystemUpTime(&pAdapter->RoamStartTime); 

	///Start roaming...
	DBGPRINT(DBG_ROAM|DBG_CCX|DBG_HELP,(L"==>CCX-FastRoaming with %s\r\n", pAdapter->CurrentSSID.Ssid));
	DBGPRINT(DBG_ROAM|DBG_CCX, (L"==> New AP(%d)-%02x:%02x:%02x:%02x:%02x:%02x\r\n", 
                                        pApInfo->id, 
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[0],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[1],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[2],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[3],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[4],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[5]));
	///Get the firmware TSF 
	GetFirmwarTSF(pAdapter, pApInfo);
	if (!NdisEqualMemory(pAdapter->PSBssDescList[pApInfo->id].BSSID, pApInfo->MacAddr, sizeof(NDIS_802_11_MAC_ADDRESS))) {
		wlan_Recalculate_APID(pApInfo, pAdapter);
	}
	pReconArg->FastRoamProgressing = TRUE;
	while (pAdapter->SentPacket != NULL) {
		///If we are in a middle of sending a packet, wait until it's done
		NdisMSleep(1);
	}
	if (pApInfo->id == -1) {
		DBGPRINT(DBG_ROAM|DBG_CCX, (L"==> The AP we are going to connect is removed. Give up roaming\r\n"));
		goto funcFinal;
	}

	ResetEvent(pReconArg->hroamEvent);
	DBGPRINT(DBG_ROAM|DBG_CCX, (L"==> Connecting to(%d)-%02x:%02x:%02x:%02x:%02x:%02x\r\n", 
                                        pApInfo->id, 
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[0],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[1],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[2],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[3],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[4],
                                        pAdapter->PSBssDescList[pApInfo->id].BSSID[5]));
	nStatus = PrepareAndSendCommand(
                                    pAdapter,
                                    HostCmd_CMD_802_11_ASSOCIATE_EXT,
                                    0,
                                    HostCmd_OPTION_USE_INT,
                                    (NDIS_OID)0,
                                    HostCmd_PENDING_ON_NONE,
                                    0,
                                    FALSE,
                                    NULL,
                                    NULL,
                                    NULL,
                                    (PVOID)&pApInfo->id);
	if (nStatus == NDIS_STATUS_SUCCESS) {
		DBGPRINT(DBG_CCX, (L"==>[FastRoam] Enter waiting for Assoc_Resp\n"));
		WaitForSingleObject( pReconArg->hroamEvent, INFINITE);   ///Wait for 5 seconds for association response             
	} else {
		DBGPRINT(DBG_CCX, (L"==>[FastRoam] Failed to send ReAssociation\n"));
		goto funcFinal;
	}
	if (pAdapter->MediaConnectStatus == NdisMediaStateDisconnected) {
		DBGPRINT(DBG_CCX, (L"==>[FastRoam] Roaming failed in ReAssociation\n"));
		goto funcFinal;
	}
	else {
		DBGPRINT(DBG_CCX, (L"==>[FastRoam] ReAssociation success, Set wlan_ccx_authSuccess\n"));
	        ///Roaming successfully
		wlan_ccx_authSuccess(pAdapter);
		///Associated success. Cache the AP information
		NdisMoveMemory(&pAdapter->frParam.ccxLastAP, &pAdapter->frParam.ccxCurrentAP, sizeof(CCX_AP_INFO));
	}

	///Add the WEP key, if the AP is wep-enabled
	if ((pReconArg->privacyEnabled == TRUE) && 
		(pAdapter->WPAEnabled == FALSE)) {
		SetLastWepKey(pAdapter);
	}

funcFinal:
	pReconArg->FastRoamProgressing = FALSE;
	DBGPRINT(DBG_ROAM|DBG_HELP, (TEXT("Leave roaming....\r\n")));
	return;

}

VOID wlan_reconnect_deinit(PRECONNECTARG  pReconArg)
{
	if (pReconArg->hroamEvent != NULL) {
		CloseHandle(pReconArg->hroamEvent);
		pReconArg->hroamEvent = NULL;
	}
	return;
}

VOID wlan_recnnect_set_ap_param(PRECONNECTARG  pReconArg, BOOLEAN isPrivacy, BOOLEAN isCCKM)
{
	if (pReconArg == NULL) {
		return;
	}
	pReconArg->privacyEnabled = isPrivacy;
	pReconArg->cckmEnabled = isCCKM;
	return;
}



