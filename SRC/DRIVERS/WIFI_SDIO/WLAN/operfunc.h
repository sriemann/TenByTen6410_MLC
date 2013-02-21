/******************* (c) Marvell Semiconductor, Inc., *************************
 *
 *  Purpose:
 *
 *      This file contains the function prototypes for MRVDRV operation 
 *      functions
 *  
 *  Notes:
 *
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/11/08 $
 *
 *	$Revision: #5 $
 *
 *****************************************************************************/

#ifndef _OPERFUNC_H_    /* filename in CAPS */
#define _OPERFUNC_H_


/*
===============================================================================
            PRIVATE PROCEDURES
===============================================================================
*/
NDIS_STATUS PostFwDownload ( IN PMRVDRV_ADAPTER Adapter );

VOID 
InitAdapterObject(
    IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS 
AllocateAdapterBuffer(
    IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS
InitSyncObjects(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS 
SetUpStationHW(
	IN PMRVDRV_ADAPTER Adapter,
	IN  NDIS_HANDLE  WrapperConfigurationContext
	);


BOOL
AddKeyValues (
       IN LPWSTR KeyName, 
       IN PREG_VALUE_DESCR Vals
       );

VOID 
FreeAdapterObject(
	IN PMRVDRV_ADAPTER Adapter
	);

VOID
CleanUpStationHW(
	IN PMRVDRV_ADAPTER Adapter
	);

ULONG
GetConnectionStatus(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS
SetMacControl(
	IN PMRVDRV_ADAPTER Adapter 
	);


VOID
ReadStatsCounters(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS 
ReadRegistryInfo(
    IN PMRVDRV_ADAPTER Adapter,
    IN NDIS_HANDLE RegHdl
	);

VOID
HandleRxReadyEvent(
	IN PMRVDRV_ADAPTER Adapter
	);

VOID
HandleMACEvent(
	IN PMRVDRV_ADAPTER Adapter, 
	IN UINT INTCode
	);

wchar_t*	OIDMSG(NDIS_OID Oid, NDIS_STATUS MsgStatus);


///VOID
///HandleFastRoam(IN PMRVDRV_ADAPTER Adapter);

NDIS_STATUS
HandleTxSingleDoneEvent(
	IN PMRVDRV_ADAPTER Adapter
	); 

VOID
HandleTxDMADoneEvent(
	IN PMRVDRV_ADAPTER Adapter
	);

VOID
HandleTxPPADoneEvent(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS
HandleCommandFinishedEvent(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS
HandleNDISSendPacket(
	IN PMRVDRV_ADAPTER Adapter,
	IN PNDIS_PACKET Packet,
	IN UINT Index,
	IN BOOLEAN SpinLockAcquired
	);

NDIS_STATUS AllocateCmdBuffer(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS FreeCmdBuffer(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS ResetCmdBuffer(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS
ProcessHostCommand(
	IN PMRVDRV_ADAPTER Adapter,
	IN PUCHAR pCmdBuf
	);


NDIS_STATUS
PrepareAndSendCommand(
	IN PMRVDRV_ADAPTER Adapter,
	IN USHORT Cmd,
	IN USHORT CmdOPtion,
	IN USHORT INTOPtion, 
	IN NDIS_OID PendingOID,
	IN USHORT PendingInfo,
	IN USHORT BatchQNum,
	IN BOOLEAN IsLastBatchCmd,
	IN PULONG BytesWritten,
	IN PULONG BytesRead,
	IN PULONG BytesNeeded,
	IN PVOID InformationBuffer
	);

USHORT 
GetExpectedRetCode(
	IN USHORT Cmd
	);

NDIS_STATUS AllocateRxQ(
	IN PMRVDRV_ADAPTER Adapter
	);

 


NDIS_STATUS
CleanUpSingleTxBuffer(
IN PMRVDRV_ADAPTER Adapter
);

NDIS_STATUS 
FreeRxQ(
	IN PMRVDRV_ADAPTER Adapter
	);

VOID
ResetRxPDQ(
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS
SetStationPowerState(
    PMRVDRV_ADAPTER Adapter,
	NDIS_DEVICE_POWER_STATE NewPowerState,
	IN NDIS_OID Oid,
	IN PULONG BytesRead,
	IN PULONG BytesNeeded,
	IN PVOID InformationBuffer
);

NDIS_STATUS
CheckCurrentSationStatus(
	IN PMRVDRV_ADAPTER Adapter
);
NDIS_STATUS
SendSinglePacket(
	IN PMRVDRV_ADAPTER Adapter,
	IN PNDIS_PACKET Packet,
	IN PSDIO_TX_PKT pDnldPacket	
);    

NDIS_STATUS CCX_SendSinglePacket(
				IN PMRVDRV_ADAPTER Adapter,
				IN PNDIS_PACKET Packet ); 

NDIS_STATUS 
AllocateSingleTx(
	IN PMRVDRV_ADAPTER Adapter
	); 

CmdCtrlNode *
GetFreeCmdCtrlNode(
	IN PMRVDRV_ADAPTER Adapter
);

VOID
ReturnCmdNode(
	IN PMRVDRV_ADAPTER Adapter,
	CmdCtrlNode *TempNode
);

VOID
SetCmdCtrlNode(
	IN PMRVDRV_ADAPTER Adapter,
	IN  CmdCtrlNode *pTempNode,
	IN  NDIS_OID PendingOID,
	IN  USHORT PendingInfo,
	IN  USHORT INTOption,
	IN  USHORT BatchQNum,
	IN  BOOLEAN IsLastBatchCmd,
	IN  PULONG BytesWritten,
	IN  PULONG BytesRead,
	IN  PULONG BytesNeeded,
	IN  PVOID InformationBuffer
);

VOID 
CleanUpCmdCtrlNode(
	IN  CmdCtrlNode *pTempNode
);

VOID
InsertCmdToQueue(
	IN PMRVDRV_ADAPTER Adapter,
	IN CmdCtrlNode *pTempCmd
	);

VOID
GetCmdFromQueueToExecute(	
	IN PMRVDRV_ADAPTER Adapter
	);

NDIS_STATUS
InterpretBSSDescription(
	IN PMRVDRV_ADAPTER Adapter,
	PVOID pRetCommandBuffer,
	ULONG ulBSSDescriptionListSize
  	,USHORT  nHostCmdRet
);  

//060407
VOID 
RemoveTheActiveSsidEntry(
	IN PMRVDRV_ADAPTER Adapter, PNDIS_802_11_SSID ScannedSSID
);	


void MrvIstThread(
        PVOID pContext
);

VOID
MrvTransmitThread(
	IN PVOID pContext
	); 




VOID
ResetAdapterObject(
	PMRVDRV_ADAPTER Adapter
);

void
SendAutoAssociationCmds(
	IN PMRVDRV_ADAPTER Adapter
);

void
SendDesiredSSIDCmds(
	IN PMRVDRV_ADAPTER Adapter
);

NDIS_STATUS 
FirmwareDownload(
    IN PMRVDRV_ADAPTER Adapter
);

//lykao, 050505, begin
//IX_STATUS
//cf_HostBootFirmwareDownload(
//	PCF_OBJECT pCf,
//    IN PMRVDRV_ADAPTER Adapter
//);

NDIS_STATUS
AddWEPKey(
	IN PMRVDRV_ADAPTER Adapter,
    IN PNDIS_802_11_SSID pInputSSID,
    IN PNDIS_802_11_WEP pWEP
);

NDIS_STATUS
RemoveWEPKey(
	IN PMRVDRV_ADAPTER Adapter,
    IN PNDIS_802_11_SSID pInputSSID,
    IN PNDIS_802_11_WEP pWEP
);

NDIS_STATUS
SearchWEPKey(
	IN PMRVDRV_ADAPTER Adapter,
    IN PNDIS_802_11_SSID pInputSSID,
    OUT PNDIS_802_11_WEP pWEP
);

VOID
WEP_Encrypt(
	IN PMRVDRV_ADAPTER Adapter,
    IN PUCHAR Buf,
    IN ULONG Len
);


VOID
ResetDisconnectStatus(
	IN PMRVDRV_ADAPTER Adapter
);

VOID
ResetSingleTxDoneAck(
	IN PMRVDRV_ADAPTER Adapter
);

VOID 
HandleHostPendGetOid(
	USHORT Ret, 
	PMRVDRV_ADAPTER Adapter,
	PHostCmd_DS_GEN pRetPtr
);

VOID 
HandleHostPendSetOid(
	USHORT Ret, 
	PMRVDRV_ADAPTER Adapter
);

VOID 
HandleHostPendCommand(
	USHORT Ret, 
	PMRVDRV_ADAPTER Adapter,
	PHostCmd_DS_GEN pRetPtr
);

/*
NDIS_STATUS
HandleHostCmdFailure(
	USHORT Ret, 
	PMRVDRV_ADAPTER Adapter,
	PHostCmd_DS_GEN pRetPtr
	);
*/
VOID
MacEventDisconnected(
	PMRVDRV_ADAPTER Adapter,
	UINT TimerValue,
	BOOLEAN PowerFlag
);

VOID
HandleHardwareSpec(
	PHostCmd_DS_GET_HW_SPEC pHWSpec,
	PMRVDRV_ADAPTER Adapter
);

VOID
HandleScanResponse(
	PHostCmd_DS_802_11_SCAN_RSP pScanResponse,
	PMRVDRV_ADAPTER Adapter
);

VOID
HandleAssocReassoc(
	PHostCmd_DS_802_11_ASSOCIATE_RESULT pAssoResult,
	PMRVDRV_ADAPTER Adapter
);

VOID
HandleAdHocJoinStart(
	PHostCmd_DS_802_11_AD_HOC_RESULT pAdHocResult,
	USHORT Ret,
	PMRVDRV_ADAPTER Adapter
);

VOID 
HandleEnableQosWmeCommand(
	PHostCmd_CMD_QOS_WME_ENABLE_STATE pCmd,
	PMRVDRV_ADAPTER Adapter
);

VOID
HandleWmeACParamsCommand(
	PHostCmd_CMD_QOS_WME_ACCESS_CATEGORY_PARAMETERS pCmd,
	PMRVDRV_ADAPTER Adapter
);

VOID 
SetupDataRate(
	USHORT CMDOption,
	PHostCmd_DS_802_11_DATA_RATE pDataRate,
	PMRVDRV_ADAPTER Adapter,
	PVOID InformationBuffer
	);

VOID
SetupWepKeys(
	NDIS_OID PendingOID,
	PHostCmd_DS_802_11_SET_WEP pSetWEP,
	PNDIS_802_11_WEP pNewWEP,
	PVOID InformationBuffer,
	PMRVDRV_ADAPTER Adapter,
	USHORT CmdOption
	);


INT 
FindAPBySSID(
	PMRVDRV_ADAPTER pAdapter, 
	PVOID InformationBuffer);

NDIS_STATUS
SetupAssociationExt (
	PHostCmd_DS_802_11_ASSOCIATE_EXT pAsso,
	USHORT PendingInfo,
	PMRVDRV_ADAPTER Adapter, 
	CmdCtrlNode *pTempCmd,
	PVOID InformationBuffer
	);

VOID 
SetupAdHocStart(
	PHostCmd_DS_802_11_AD_HOC_START pAdHocStart,
	USHORT PendingInfo,
    PMRVDRV_ADAPTER Adapter, 
   	CmdCtrlNode *pTempCmd,
	PVOID InformationBuffer
	);

NDIS_STATUS
SetupAdHocJoin(
	PHostCmd_DS_802_11_AD_HOC_JOIN pAdHocJoin,
	USHORT PendingInfo,
    PMRVDRV_ADAPTER Adapter, 
   	CmdCtrlNode *pTempCmd,
	PVOID InformationBuffer
    );

NDIS_STATUS
SetupScanCommand (
	PMRVDRV_ADAPTER Adapter
	);


BOOL
InitializeWirelessConfig(
	PMRVDRV_ADAPTER Adapter
	);


NDIS_STATUS
SendPowerModeCAM(
	IN PMRVDRV_ADAPTER Adapter,
	IN NDIS_OID Oid, 
	OUT PULONG BytesRead,
	OUT PULONG BytesNeeded,	 
	PVOID InformationBuffer 
);

VOID
CopyBSSIDInfo(
	PMRVDRV_ADAPTER Adapter
);

VOID
UpdatePowerSaveState(
	IN PMRVDRV_ADAPTER Adapter,
	IN CmdCtrlNode *pTempNode,
	IN ULONG  PsEvent
);

UCHAR ConvertNDISRateToFWIndex(UCHAR  NDISRate);

UCHAR 
ConvertFWIndexToNDISRate(
	IN UCHAR  FWIndex
);

VOID 
CleanUpTimers(
	IN PMRVDRV_ADAPTER Adapter
);

UCHAR ConvertFWIndexToNDISRate(UCHAR  FWIndex);
UCHAR 
region_2_code(
	IN CHAR *region
);

VOID CleanUpTimers(PMRVDRV_ADAPTER Adapter);
VOID
ResetAllScanTypeAndPower(
	IN PMRVDRV_ADAPTER Adapter
);

UCHAR FindSSIDInList(
   IN PMRVDRV_ADAPTER Adapter,
   IN PNDIS_802_11_SSID pSSID
   ); 

UCHAR FindSSIDInPSList(
      IN PMRVDRV_ADAPTER Adapter,
      IN PNDIS_802_11_SSID pSSID);

VOID
UpdateScanTypeFromCountryIE(
   IN PMRVDRV_ADAPTER Adapter,
   //IN PNDIS_802_11_SSID pSSID  // dralee 081005 V4
	IN UCHAR BssidListIndex   
   ); 

VOID 
GenerateDomainInfoFromCountryIE( 
	IN MrvlIEtypes_DomainParamSet_t  *pDomainInfo,
    IN IEEEtypes_CountryInfoFullSet_t  *pCountryIE
);

__inline
extern MRVDRV_GET_PACKET_STATUS
GetRxPacketDesc(PMRVDRV_ADAPTER Adapter,
                PNDIS_PACKET    *p_PPacket);
VOID
Enable_11d( 
	IN PMRVDRV_ADAPTER Adapter, 
	IN BOOLEAN enable 
);

VOID 
SetDomainInfo( 
	IN PMRVDRV_ADAPTER Adapter, 
	IEEEtypes_CountryInfoFullSet_t  *pCountryIE 
);

VOID RegularIeFilter(UCHAR *IeBuf, USHORT *pBufLen);

VOID
Generate_domain_info_11d(
                          UCHAR CountryNum,
                          UCHAR  band, 
                          MrvlIEtypes_DomainParamSet_t *domaininfo);


//Plus++, 012606(for Broadcom mix mode fix)
UCHAR Wpa2RsnIeAdjust(UCHAR *Dest, UCHAR *Src, UCHAR RsnIeLen, UCHAR Mode);
//Plus--, 012606

//Junius Added 20071017
UCHAR WpaIeAdjust(UCHAR *Dest, UCHAR *Src, UCHAR RsnIeLen, UCHAR Mode);
//end added

int ascii2hex(UCHAR *d, char *s, int dlen);


BOOLEAN  TxPacketEnQueue(PMRVDRV_ADAPTER Adapter,PNDIS_PACKET Packet);
VOID TxPacketDeQueue(PMRVDRV_ADAPTER Adapter,PPTXQ_KEEPER ppTxQKeeper,PPTXQ_NODE ppTQNode);
VOID InitializeTxNodeQ(IN PMRVDRV_ADAPTER Adapter );


NDIS_STATUS PrepareDownloadPacket(PMRVDRV_ADAPTER Adapter,PNDIS_PACKET Packet, PSDIO_TX_PKT pDnldPacket);


void ResetPmkidCache( IN PMRVDRV_ADAPTER Adapter );
int SavePmkidToCache( IN PMRVDRV_ADAPTER Adapter, IN PNDIS_802_11_PMKID pNewPmkid );
int FindPmkidInCache( IN PMRVDRV_ADAPTER Adapter, IN UCHAR *pBssid );
void DbgDumpCurrentPmkidCache( IN PMRVDRV_ADAPTER Adapter );
void OsNotify_RSN( IN PMRVDRV_ADAPTER Adapter );

// plus
UINT8 EncryptionStateCheck(IN PMRVDRV_ADAPTER Adapter);  

//++dralee_20060327
NDIS_STATUS
SendNullPacket(IN PMRVDRV_ADAPTER Adapter,UCHAR pwmgr );
UCHAR CheckLastPacketIndication(IN PMRVDRV_ADAPTER Adapter);


VOID
InfraBssReconnectStart(
		IN PMRVDRV_ADAPTER Adapter,
		IN USHORT reason );

VOID
InfraBssReconnectStop(
		PMRVDRV_ADAPTER Adapter);

VOID
ReConnectHandler(
		PMRVDRV_ADAPTER Adapter);

NDIS_STATUS
SetupScanCommand (
	PMRVDRV_ADAPTER Adapter);


#ifdef MRVL_PRINT_DBG_MSG
       void MrvPrintFile(const unsigned short *fmt, ...);
#endif

#ifdef DBG_MSG_TO_RETAILMSG 
void MrvRETAILMSG(const wchar_t *fmt, ...);
#endif

VOID
SetupBgScanCurrentSSID(
		IN PMRVDRV_ADAPTER Adapter,
		IN ULONG ScanInterval
		);

NDIS_STATUS 
EnableBgScan( 
		IN	PMRVDRV_ADAPTER Adapter,
		IN	BOOLEAN bEnable
		);

VOID 
DumpBgScanConfig ( 
		IN POID_MRVL_DS_BG_SCAN_CONFIG  pBgScanCfg 
		);



BOOLEAN 
CalcAndCheckTlvData ( 
		IN PMRVDRV_ADAPTER pAdapter, 
		IN PUCHAR pSrcTlvBuf, 
		IN USHORT nMaxSizeOfSrcTlv, 
		IN USHORT nCheckTlvTypeId, 
		IN PUSHORT pnSizeOfTlv 
		);

BOOLEAN 
AppendDefaultChannelListToCmd ( 
		IN PMRVDRV_ADAPTER pAdapter, 
		IN CmdCtrlNode *pCmd 
		);

USHORT
AddDefaultChannelList ( 
		IN PMRVDRV_ADAPTER pAdapter, 
		IN UCHAR *pDestBuf, 
		IN UCHAR *ChanList, 
		IN UCHAR nNumOfChan );


VOID 
SaveCurrentConfig ( 
		IN PMRVDRV_ADAPTER pAdapter, 
		IN PVOID InformationBuffer, 
		IN ULONG InformationBufferLength,
		USHORT Cmd
		);

VOID
SetUpLowRssiValue(
		IN PMRVDRV_ADAPTER Adapter
		);
VOID
SetUpHighRssiValue(
		IN PMRVDRV_ADAPTER Adapter
		);



//dralee_20060509
NDIS_STATUS Reactivate_Host_Sleep_Cfg(PMRVDRV_ADAPTER Adapter);



// tt ++ mic error 2
BOOL IsInMicErrorPeriod( PMRVDRV_ADAPTER pAdapter );
// tt --

static int get_common_rates(UCHAR *rate1, int rate1_size, UCHAR *rate2, int rate2_size);

VOID
GetPACFGValue(
		IN PMRVDRV_ADAPTER Adapter
		);
                                              
//dralee_20060613
VOID SetRadioControl(PMRVDRV_ADAPTER Adapter);
VOID UpdateScanTypeByCountryCode( PMRVDRV_ADAPTER Adapter, UCHAR CountryNum );   

//dralee_20060706
void MrvPrintEventLogToFile(const unsigned short *fmt, ...);

// region-code
VOID 
SetRegionCode( IN PMRVDRV_ADAPTER Adapter );

void DownloadGProtectSetting( PMRVDRV_ADAPTER pAdapter );

NDIS_STATUS
ProcessOIDResetChip(PMRVDRV_ADAPTER Adapter); 

NDIS_STATUS
WaitOIDAccessRespond(PMRVDRV_ADAPTER Adapter);


NDIS_STATUS
ProcessOIDHostSleepCfg( PMRVDRV_ADAPTER Adapter,
                        IN PVOID InformationBuffer,
                        IN ULONG InformationBufferLength,
                        OUT PULONG BytesNeeded);



VOID GetTsfTlvFromScanResult(MrvlIEtypes_Data_t* pTlv,
                             INT tlvBufSize,
                             MrvlIEtypes_TsfTimestamp_t** pTsfTlv);

UINT AppendTsfTlv(PUCHAR* pBuffer, 
                  PBSS_DESCRIPTION_SET_ALL_FIELDS pBSSDesc);
                  
VOID UpdateTsfTimestamps(PMRVDRV_ADAPTER Adapter, 
                         PBSS_DESCRIPTION_SET_ALL_FIELDS pBSSDesc);


USHORT AuthModeMapping(NDIS_802_11_AUTHENTICATION_MODE mode);

VOID SendDeauthCommand(PMRVDRV_ADAPTER Adapter);

VOID
ProcessOIDWakeUpDevice(PMRVDRV_ADAPTER Adapter);

NDIS_STATUS Deactivate_Host_Sleep_Cfg(PMRVDRV_ADAPTER Adapter,USHORT prioritypass);

VOID
ProcessOIDHSCancel(PMRVDRV_ADAPTER Adapter);

VOID
Application_Event_Notify(PMRVDRV_ADAPTER Adapter, ULONG type);

VOID      
HandleD0toD3(PMRVDRV_ADAPTER Adapter);

NDIS_STATUS
HandleD3toD0(PMRVDRV_ADAPTER Adapter);

//022607  
NDIS_STATUS  
ProcessOIDBCATimeshare( PMRVDRV_ADAPTER Adapter,
                        ULONG action,
                        IN PVOID InformationBuffer);

//041307
NDIS_STATUS
ProcessOIDDeepSleep(PMRVDRV_ADAPTER Adapter,
                    ULONG action,
                    ULONG InformationBufferLength,
                    PVOID InformationBuffer, 
                    USHORT pendingInfo);
  
//++ remove_bg_ssid
ULONG
FindSsidInBgCfg(PVOID ptlv, ULONG nsize, ULONG seq, PNDIS_802_11_SSID pNdisSsid);
//-- remove_bg_ssid

NDIS_STATUS
ProcessOIDRateAdaptRateSet(PMRVDRV_ADAPTER Adapter,
                           ULONG action,
                           ULONG InformationBufferLength,
                           PULONG BytesWritten,
                           PVOID InformationBuffer); 

NDIS_STATUS
ProcessOIDDesiredRates(PMRVDRV_ADAPTER Adapter,
                       ULONG action,
                       ULONG InformationBufferLength,
                       PULONG BytesWritten,
                       PVOID InformationBuffer);


       
VOID
Ndis_MediaStatus_Notify(PMRVDRV_ADAPTER Adapter, ULONG type);

//051207 060407
VOID
Signal_Pending_OID(PMRVDRV_ADAPTER Adapter);



void SendOneQueuedPacket( PMRVDRV_ADAPTER Adapter );

BOOLEAN IsValidChannel( PMRVDRV_ADAPTER Adapter, ULONG ulChannelNumber );


//051407
NDIS_STATUS
AutoDeepSleepPending(PMRVDRV_ADAPTER Adapter);


VOID
AutoDeepSleepCounter(IN PMRVDRV_ADAPTER Adapter); 
//051707
VOID
RemoveAgeOutEntryFromPSList( PMRVDRV_ADAPTER Adapter);

VOID
ReplacePSListEntryByBSSListEntry( PMRVDRV_ADAPTER Adapter, ULONG PSidx, ULONG TSidx);

//051707 
UCHAR FindBSSIDInPSList(
      IN PMRVDRV_ADAPTER Adapter,
      IN PUCHAR BSSID);

//051707
typedef enum {
    AGEOUT_SSID,        ///Ageout all APs whose SSID is identical with the probe-request
    AGEOUT_ALL,         ///Ageout all APs in the APTable
    AGEOUT_CHNL,        ///Ageout all APs whose channel is identical with the probe-request
    AGEOUT_MAX
} AGEOUTTYPE;

void 
AgeOutTheActiveSsidEntry( IN PMRVDRV_ADAPTER Adapter,
                          PNDIS_802_11_SSID ScannedSSID, AGEOUTTYPE type);








NDIS_STATUS
ProcessOIDStatistics(PMRVDRV_ADAPTER Adapter,
                            ULONG InformationBufferLength,
                            PVOID InformationBuffer,
                            OUT PULONG BytesNeeded,
                            OUT PULONG BytesWritten);

NDIS_STATUS
ProcessOIDGetTxPowerLevel(PMRVDRV_ADAPTER Adapter, 
                                                            ULONG InformationBufferLength,
                                                            PVOID InformationBuffer,
                                                            OUT PULONG BytesNeeded,
                                                            OUT PULONG BytesWritten);

NDIS_STATUS
ProcessOIDSetTxPowerLevel(PMRVDRV_ADAPTER Adapter, ULONG PowerLevel);

DWORD WaitForSingleObjectWithCancel(PMRVDRV_ADAPTER pAdapter, HANDLE hEvent, DWORD dwTimeout);

void SetDsState( PMRVDRV_ADAPTER Adapter, ULONG ulNewState );
BOOLEAN IsThisDsState( PMRVDRV_ADAPTER Adapter, ULONG ulState );
void SetHostPowerState( PMRVDRV_ADAPTER Adapter, ULONG ulNewState );
BOOLEAN IsThisHostPowerState( PMRVDRV_ADAPTER Adapter, ULONG ulState );

BOOLEAN     IsIndicateDisconnect(PMRVDRV_ADAPTER pAdapter);

#endif /* _OPERFUNC_H_ */


NDIS_STATUS
ProcessOIDBgScanCfg(PMRVDRV_ADAPTER Adapter,
                    IN NDIS_OID Oid, 
                    IN PVOID InformationBuffer,
                    IN ULONG InformationBufferLength,
                    OUT PULONG BytesNeeded );



VOID
SelectBgScanConfig( PMRVDRV_ADAPTER Adapter );

ULONG TxPacketDeQueueByLowPriority(PMRVDRV_ADAPTER Adapter,PPTXQ_KEEPER ppTxQKeeper,PPTXQ_NODE ppTQNode, ULONG ac);




