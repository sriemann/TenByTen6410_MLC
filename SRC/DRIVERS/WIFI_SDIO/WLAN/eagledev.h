/******************* (c) Marvell Semiconductor, Inc., *************************
 *
 *
 *  Purpose:
 *
 *      This file contains definitions and data structures that are specific
 *      to Marvell MrvDrv 802.11 NIC. It contains the Device Information
 *      structure MRVDRV_ADAPTER.
 *
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/12/15 $
 *
 *  $Revision: #10 $
 *
 *****************************************************************************/

#ifndef _MRVDRVDEVICE_H_
#define _MRVDRVDEVICE_H_

#include <ntddndis.h>



#include "wlan_roam.h"

#include "SDCardDDK.h" 
#define LENGTH_BGSCAN_CFG      512
#define LENGTH_PA_CFG      64
#define MAX_TX_PACKETS      64  
#define MAX_ACTIVE_REG_PATH 256
#define NDIS_SUCCESS(Status) ((NDIS_STATUS)(Status) == NDIS_STATUS_SUCCESS)

typedef struct _TX_QUEUE_NODE {
    PNDIS_PACKET        pNextPacket;
}TX_QUEUE_NODE, *PTX_QUEUE_NODE;

typedef struct _TX_QUEUE_HEADER{
    PNDIS_PACKET        Head, Tail;
}TX_QUEUE_HEADER, *PTX_QUEUE_HEADER;

typedef struct _MRV_QOS_PARAMS
{
    ULONG  AC_BE;       // Best Effort AC parameters
    ULONG  AC_BE_XTRA;  // Extra AC_BE parameters
    ULONG  AC_BK;       // Background AC parameters
    ULONG  AC_BK_XTRA;  // Extra AC_BK parameters
    ULONG  AC_VI;       // Video AC parameters
    ULONG  AC_VI_XTRA;  // Extra Video AC parameters
    ULONG  AC_VO;       // Voice AC parameters
    ULONG  AC_VO_XTRA;  // Extra Voice AC parameters
} MRV_QOS_PARAMS, *PMRV_QOS_PARAMS;

typedef struct _MRV_BSSID_IE_LIST
{
    NDIS_802_11_FIXED_IEs       FixedIE;
    UCHAR                       VariableIE[MRVDRV_SCAN_LIST_VAR_IE_SPACE];
} MRV_BSSID_IE_LIST, *PMRV_BSSID_IE_LIST;

typedef struct _PER_CHANNEL_BSSID_LIST_DATA 
{
    UCHAR               ucNumEntry;
    MRV_BSSID_IE_LIST   IEBuffer[MRVDRV_MAX_BSSID_LIST];
    NDIS_WLAN_BSSID_EX  BSSIDList[MRVDRV_MAX_BSSID_LIST];
} PER_CHANNEL_BSSID_LIST_DATA, *PPER_CHANNEL_BSSID_LIST_DATA; 

#define MAX_PMKID_CACHE 8
#define LEN_OF_PMKID        16
#define MAX_PMKID_CANDIDATE 5

typedef struct _PMKID_CACHE
{
    UCHAR       bssid[MRVDRV_ETH_ADDR_LEN];
    UCHAR       pad[2]; // for 4 bytes boundary. ?do we need it?
    UCHAR       pmkid[LEN_OF_PMKID];
} PMKID_CACHE, *PPMKID_CACHE;


/// Fast Roaming parameter
typedef struct _FASTROAM_PARAM 
{
	///Parameters to record the AP information for Fast-Roaming test items.
	CCX_AP_INFO			ccxCurrentAP;
	CCX_AP_INFO			ccxLastAP;
	int					assocChannel;
	USHORT				AssocStatusCode;
	BOOLEAN				is80211x;
} FASTROAMPARAM, *PFASTROAMPARAM;

typedef struct _AutoDSRecord
{
   USHORT   LastSeqNum;
   USHORT   timer;
}  AUTO_DS_RECORD, *PAUTO_DS_RECORD;        

//071207
//power management for WM60



/////////////////////////////////////////////////////////////////
//
//          Adapter information
//
/////////////////////////////////////////////////////////////////

#pragma pack(4)

typedef struct _MRVDRV_ADAPTER
{
	///pmkcache: bug#16956 ++
	BOOLEAN					bIsReconnectAssociation;  
	BOOLEAN					isPktPending;
	///pmkcache: bug#16956 --
    // Handle given by NDIS when the NIC registered itself
    NDIS_HANDLE             MrvDrvAdapterHdl;
    // Device information
    ULONG                   VendorID;
    UINT                    BusNumber;
    USHORT              SlotNumber;
    USHORT              DeviceNumber;
      UCHAR         ChipVersion;
    USHORT              DriverVersion;
    UCHAR                   MachineName[MRVDRV_DEFAULT_MACHINE_NAME_SIZE];

    // Interrupt object for this adapter
    NDIS_MINIPORT_INTERRUPT MrvDrvINT;
    NDIS_INTERRUPT_MODE     MrvDrvINTMode;
    UINT                    MrvDrvINTVector;
    UINT                    MrvDrvINTLevel;
    ULONG                   INTCause;
    NDIS_HANDLE             MrvDrvWrapperConfigurationContext;

    ULONG                   InitializationStatus;
    UINT                    TimerInterval;


    // NDIS Timer
  NDIS_MINIPORT_TIMER       MrvDrvCommandTimer;   
    NDIS_MINIPORT_TIMER     MrvDrvTxPktTimer;
  //dralee, unused 
    //NDIS_MINIPORT_TIMER       MrvDrvCheckTxQueueTimer;
    NDIS_MINIPORT_TIMER     MrvDrvIndicateConnectStatusTimer;
    NDIS_MINIPORT_TIMER        MrvReConnectTimer;     
    BOOLEAN                             ReConnectTimerIsSet;  
       // reconnect
       BOOLEAN                 bIsReconnectEnable;
       USHORT                   ReconnectType;
       USHORT                   usReconnectCounter;
       ULONG                     ulReconnectPeriod;
       ULONG                     ulStartTimeStamp;
       BOOLEAN                 bIsReConnectNow;
       BOOLEAN                 bIsSystemConnectNow;
       BOOLEAN                 bIsAcceptSystemConnect;
       NDIS_802_11_NETWORK_INFRASTRUCTURE  ReInfrastructureMode; 
       NDIS_802_11_AUTHENTICATION_MODE     ReAuthenticationMode;
       NDIS_802_11_ENCRYPTION_STATUS       ReEncryptionStatus;
       MRVL_WEP_KEY            ReWEPKey;
       NDIS_802_11_SSID        ReSSID;  
       
       ///Reconnect_DiffSetting ++
       UCHAR                               ReBSSID[MRVDRV_ETH_ADDR_LEN];
       MRV_BSSID_IE_LIST           ReIEBuffer;
       ///Reconnect_DiffSetting --
    //35.p6++
    BOOLEAN                   bIsBeaconLoseEvent;
    BOOLEAN              bIsDeauthenticationEvent;
       //35.p6--     


    BOOLEAN                 CommandTimerSet;
    BOOLEAN                 TxPktTimerIsSet;
    //BOOLEAN                   CheckTxQueueTimerIsSet;
    BOOLEAN                 isCommandTimerExpired;
    BOOLEAN                 DisconnectTimerSet;
    
    NDIS_WORK_ITEM          MrvDrvPMWorkItem;
    

    // Command-related variables
    USHORT                  SeqNum;
    CmdCtrlNode             *CmdArray;
    CmdCtrlNode             *CurCmd;       // Current Command
    Q_KEEPER                CmdFreeQ;      // Queue to keep free command buffers
    Q_KEEPER                            CmdPendQ;          // plus 

    
    TXQ_NODE          FreeTxQNode[MAX_TX_PACKETS];
    PTXQ_NODE         TxFreeQ;

    TXQ_KEEPER              TXPRYQ[MAX_AC_QUEUES];     // Tx priority Queue 0,1,2,3
    TXQ_KEEPER              TXRETQ;     // Tx priority Queue 3  
  

    // Tx-related variables 
  //ULONG PacketSentState;

    WCB                     Wcb;
    TxCtrlNode              TxNode;
    PNDIS_PACKET            SentPacket;            
  ULONG                 bSentPacketReturned;  //dralee++ 09172005
  ULONG                 bNullFrameSent;
  //  PNDIS_PACKET            QueuedPacket;   // packet queued while in sleep , dralee

    /// Rx-related variables 
    //dralee, 072705
    //RxPD                                RxPD1;
    UCHAR                               pRxBuf[(MRVDRV_ETH_RX_PACKET_BUFFER_SIZE)*2];
    ULONG                               ulRxSize;

    PNDIS_PACKET                    pPendedRxPkt;
    PNDIS_PACKET                    pRxCurPkt;
    PRxPD                           pRxPD1;

    NDIS_HANDLE                         RxPacketPoolHandle;
    NDIS_HANDLE                         RxBufferPoolHandle;
    PNDIS_PACKET                        pRxPacket[MRVDRV_NUM_RX_PKT_IN_QUEUE];
    PNDIS_BUFFER                        pRxBuffer[MRVDRV_NUM_RX_PKT_IN_QUEUE];
    PVOID                               pRxBufVM[MRVDRV_NUM_RX_PKT_IN_QUEUE];
    PACKET_QUEUE_NODE                   RxPacketQueueNode[MRVDRV_NUM_RX_PKT_IN_QUEUE];
    NDIS_SPIN_LOCK                      RxQueueSpinLock;
    Q_KEEPER                            RxPacketFreeQueue;
    SHORT                             sNumOutstandingRxPacket;            
        
    /// NDIS spin lock
      NDIS_SPIN_LOCK                        FreeQSpinLock;      // Command : free queue 
      NDIS_SPIN_LOCK            PendQSpinLock;      

    SubscibeEvent_t         EventRecord;         
    USHORT                  SubscribeEvents; 
           
    USHORT                  ScanProbes;          
    ULONG                   LocalListenInterval;  

    // MAC address information
    UCHAR                   PermanentAddr[MRVDRV_ETH_ADDR_LEN];
    UCHAR                   CurrentAddr[MRVDRV_ETH_ADDR_LEN];
    UCHAR                   MulticastList[MRVDRV_MAX_MULTICAST_LIST_SIZE][MRVDRV_ETH_ADDR_LEN];
    ULONG                   NumOfMulticastMACAddr;
    USHORT                  RegionCode;
    USHORT                  RegionTableIndex;
    ULONG                   FWReleaseNumber;

    // Operation characteristics
    ULONG                   LinkSpeed;
    ULONG                   CurrentPacketFilter;
    USHORT        CurrentMacControl;
    ULONG                   CurrentLookAhead;
    ULONG                   ProtocolOptions;
    ULONG                   MACOptions;
    ULONG                   MediaConnectStatus;
    NDIS_MEDIUM             MediaInUse;

    // Status variables
    NDIS_HARDWARE_STATUS    HardwareStatus;
    USHORT                  FWStatus;
    USHORT                  MACStatus;
    USHORT                  RFStatus;
    USHORT                  CurentChannel; // 1..99

    /// WPA-related variables
    NDIS_802_11_ENCRYPTION_STATUS       EncryptionStatus;
    BOOLEAN                             WPAEnabled;

    // Alignment on PDA
    UCHAR                               RESERVED[3];

    // Buffer to store data from OID_802_11_ASSOCIATION_INFORMATION
    UCHAR                               AssocInfoBuffer[MRVDRV_ASSOCIATE_INFO_BUFFER_SIZE];

    // Key management
    MRVL_WPA_KEY_SET                    KeyMap[MRVL_NUM_WPA_KEYSET_SUPPORTED];
    UCHAR                               ucCurrentUsedKeyIdx;

    // Last added pair wise key
    PMRVL_WPA_KEY                       pLastAddedPWK;

    // Last matching group wise key
    PMRVL_WPA_KEY                       pLastAddedGWK;

    ///Misc variables
    ULONG                               FWCapInfo;

    NDIS_802_11_WEP_STATUS              WEPStatus;

    NDIS_802_11_AUTHENTICATION_MODE     AuthenticationMode;
    NDIS_802_11_NETWORK_INFRASTRUCTURE  InfrastructureMode;
    NDIS_802_11_PRIVACY_FILTER          PrivacyFilter;
    ULONG                               ulAttemptedBSSIDIndex;
    ULONG                               ulCurrentBSSIDIndex;
    NDIS_802_11_SSID                    CurrentSSID;
    UCHAR                               CurrentBSSID[MRVDRV_ETH_ADDR_LEN];
    NDIS_802_11_CONFIGURATION           CurrentConfiguration;

    NDIS_WLAN_BSSID_EX                  CurrentBSSIDDesciptor;
    MRV_BSSID_IE_LIST                   CurrentBSSIDIEBuffer;
    BSS_DESCRIPTION_SET_ALL_FIELDS      CurrentBssDesciptor;

    NDIS_802_11_SSID                    AttemptedSSIDBeforeScan;
    NDIS_802_11_SSID                    PreviousSSID;
    UCHAR                               PreviousBSSID[MRVDRV_ETH_ADDR_LEN];

    // BSSID LIST for ESS and IBSS 
    NDIS_WLAN_BSSID_EX                  BSSIDList[MRVDRV_MAX_BSSID_LIST];
    MRV_BSSID_IE_LIST                   IEBuffer[MRVDRV_MAX_BSSID_LIST];
    BSS_DESCRIPTION_SET_ALL_FIELDS      BssDescList[MRVDRV_MAX_BSSID_LIST];
    ULONG                               ulNumOfBSSIDs;
    // progressive scan data
    NDIS_WLAN_BSSID_EX                  PSBSSIDList[MRVDRV_MAX_BSSID_LIST];    
    MRV_BSSID_IE_LIST                   PSIEBuffer[MRVDRV_MAX_BSSID_LIST];
    BSS_DESCRIPTION_SET_ALL_FIELDS      PSBssDescList[MRVDRV_MAX_BSSID_LIST];
    ULONG                               ulPSNumOfBSSIDs;    
    UCHAR                               PSBssEntryAge[MRVDRV_MAX_BSSID_LIST];     //051707 tag to age out entry 060407

    BOOLEAN                             bIsPendingReset;
    BOOLEAN                             bIsAssociationBlockedByScan;
    BOOLEAN                             bIsScanWhileConnect;
    BOOLEAN                             bIsConnectToAny;
  
    BOOLEAN                             bIsScanInProgress;
    BOOLEAN                             bIsAssociateInProgress;
    
    BOOLEAN                             bRetryAssociate;


    LONG                                LastRSSI;
    LONG                                RSSITriggerValue;
    USHORT                              RSSITriggerCounter;
 
    USHORT                             RxPDRate;

    /** Requested Signal Strength*/
    USHORT                            bcn_avg_factor;
    USHORT                               data_avg_factor;
    SHORT                             SNR[MAX_TYPE_B][MAX_TYPE_AVG];
    SHORT                                NF[MAX_TYPE_B][MAX_TYPE_AVG];
    SHORT                                RSSI[MAX_TYPE_B][MAX_TYPE_AVG];
    ULONG                                    ulRSSITickCount; 
    ULONG                                    ulRSSIThresholdTimer;
    SHORT                                    RSSI_Range;
    BOOLEAN                             bIsLastOIDSetEncryptionStatus;
    BOOLEAN                             TxPowerLevelIsSetByOS; // is the power level set by the OS or API?
    NDIS_802_11_TX_POWER_LEVEL          TxPowerLevel;

    USHORT                              DTIMNum;
    BOOLEAN                             DTIMFlag;

    UCHAR                               ucNumCmdTimeOut;

    ULONG                               DefaultPerChannelScanTime;
    ULONG                               ulLastMICErrorTime;

   
   /// Used to deal with 0 length SET SSID (Need to auto associate)
    NDIS_802_11_SSID                    DesiredSSID;
    NDIS_802_11_SSID                    NullSSID;
    NDIS_802_11_SSID                    ActiveScanSSID;
    BOOLEAN                             SetActiveScanSSID;
    UCHAR                               ActiveScanBSSID[MRVDRV_ETH_ADDR_LEN];
    BOOLEAN                             SetActiveScanBSSID;
    UCHAR                               NullBSSID[MRVDRV_ETH_ADDR_LEN];
    ULONG                               ulLastScanRequestTime;
// MSFT
//	UINT 								m_NumScanInAssociationBlocked;	

    /// Power management and pNp support
    NDIS_DEVICE_POWER_STATE CurPowerState;
    ULONG                   PowerLevelList[MRVDRV_TX_POWER_LEVEL_TOTAL];
    BOOLEAN                 SurpriseRemoved;
    UCHAR                   Pad3;
    USHORT                  SupportTxPowerLevel;
    USHORT                  CurrentTxPowerLevel;

    /// 802.3 Statistics
    ULONGLONG               XmitOK;
    ULONGLONG               RcvOK;
    ULONGLONG               XmitError;
    ULONGLONG               RcvError;
    ULONGLONG               RcvNoBuffer;
    ULONGLONG               RcvCRCError;
    ULONGLONG               DirectedFramesRcvOK;

    /// 802.11 statistics
    LARGE_INTEGER           TransmittedFragmentCount;
    LARGE_INTEGER           MulticastTransmittedFrameCount;
    LARGE_INTEGER           FailedCount;
    LARGE_INTEGER           RetryCount;
    LARGE_INTEGER           MultipleRetryCount;
    LARGE_INTEGER           RTSSuccessCount;
    LARGE_INTEGER           RTSFailureCount;
    LARGE_INTEGER           ACKFailureCount;
    LARGE_INTEGER           FrameDuplicateCount;
    LARGE_INTEGER           ReceivedFragmentCount;
    LARGE_INTEGER           MulticastReceivedFrameCount;
    LARGE_INTEGER           FCSErrorCount;

    // Software filter support
    BOOLEAN                 SoftwareFilterOn;

    ULONG                   NoOfWEPEntry;
    MRVL_WEP_KEY            LastAddedWEPKey;

    ULONG                   IV;
    MRVL_WEP_KEY            CurrentWEPKey;

    BOOLEAN AdHocCreated;
    ULONG   AcceptAnySSID;
    ULONG   AutoConfig;
    ULONG   TxWepKey;
    ULONG   Preamble;
    NDIS_802_11_ANTENNA TxAntenna;
    NDIS_802_11_ANTENNA RxAntenna;
    ULONG   Channel;
    ULONG   TxPower;
    NDIS_802_11_FRAGMENTATION_THRESHOLD FragThsd;
    NDIS_802_11_RTS_THRESHOLD   RTSThsd;
    ULONG   RTSRetryLimit;
    ULONG   NetworkMode;    
    // 0:auto, 1:1 Mbps, 2:2 Mbps, 3:5.5 Mbps,4:11 Mbps, 5:22 Mbps
    ULONG   DataRate;
    
    MRVL_WEP_KEY WepKey1;
    MRVL_WEP_KEY WepKey2;
    MRVL_WEP_KEY WepKey3;
    MRVL_WEP_KEY WepKey4; 
    USHORT  ChannelSelect; 

  /// Power save 
    NDIS_802_11_POWER_MODE              PSMode; // Ndis802_11PowerModeCAM=disable,Ndis802_11PowerModeMAX_PSP=enable
    NDIS_802_11_POWER_MODE              PSMode_B;   // Ndis802_11PowerModeCAM=disable,Ndis802_11PowerModeMAX_PSP=enable
    ULONG                               psState;
    ULONG                               ulDTIM;     
    ULONG                               ulAwakeTimeStamp;   
    BOOLEAN                             RadioOn;
    
    NDIS_HANDLE ConfigurationHandle;    // NDIS configuration handle
    BOOL             ShutDown;              // driver shutdown 
    HANDLE           hEjectEvent;
    SD_DEVICE_HANDLE hDevice;               // SD device handle
    
    WCHAR            ActivePath[MAX_ACTIVE_REG_PATH];   // adapter regpath
                                               
    BOOLEAN             MrvDrvSdioCheckFWReadyTimerIsSet;
    NDIS_MINIPORT_TIMER MrvDrvSdioCheckFWReadyTimer;
    
    // buffer to remember the first 32 bytes of the last pkt sent down to FW
    // in case the pkt needs to be resent
    UCHAR               LastFWBuffer[32];
    UCHAR               RadioControl;

    ULONG                       IntWARTimeoutCount;

    DWORD                       dwPacketSendTick;
    DWORD                       dwEndPktSendTick;
    DWORD                       dwInterruptTick;

    UCHAR                      ucGHostIntStatus;

    ULONG                      HostPowerState;
    ULONG                      SleepCfgRestoreTick;
    ULONG                      bHostWakeCfgSet; 
    CRITICAL_SECTION   CsForHostPowerState;

    BOOLEAN                  bPSConfirm;

    // QOS enabled, 0 = disabled, 1 = enabled
    ULONG                       EnableQOS;
    MRV_QOS_PARAMS              QOSParams;
    
    ULONG                       ulTxByteInLastPeriod;
    ULONG                       ulRxByteInLastPeriod;
    USHORT                      usTimeElapsedSinceLastScan;
  
    // BT co-existence
    ULONG                       BTMode;

    ULONG           BTTime;
    ULONG           BTTrafficType;
    ULONG           BTTimeShareInterval;
//tt -- 

    /// Deep Sleep related variables 
    //012207 
    //BOOLEAN                       IsDeepSleep;
    //BOOLEAN                       IsDeepSleepRequired;
    ULONG                       DSState;  
    CRITICAL_SECTION    CsForDSState;

    /// Adhoc AES related variables 
    BOOLEAN                     AdhocAESEnabled;
    MRVL_ADHOC_AES_KEY          AdhocAesKey;
    HostCmd_DS_802_11_KEY_MATERIAL      aeskey;

     /// Multi-Bands related variable
    REGION_CHANNEL              region_channel[MAX_REGION_BAND_NUM];
    REGION_CHANNEL              *cur_region_channel;

    UCHAR                       connected_channel;
    UCHAR                       connected_band;

    /// default values of Adhoc starter 
    ULONG                       AdhocDefaultBand;
    ULONG                       AdhocDefaultChannel;
    ULONG                       AdhocWiFiDataRate;
    ULONG                       SetSD4BIT; 
    BOOLEAN                     SendPSFlag; 

    ULONG                SdioTick; 


    HANDLE TxResourceControlEvent;

    HANDLE hThreadReadyEvent;
    HANDLE hControllerInterruptEvent;
    HANDLE hControllerInterruptThread;
    //HANDLE hTxEvent;
    //HANDLE hTxThread;
    CRITICAL_SECTION     TxCriticalSection;
    CRITICAL_SECTION     CmdQueueExeSection;
    //CRITICAL_SECTION     SDIOCriticalSection;
    CRITICAL_SECTION     IntCriticalSection;     
    PNDIS_PACKET         ReturnPacket;
    ULONG                TxPacketCount;                  // number of tx pending
    ULONG                TxPacketPut;                    // number of tx pending
    ULONG                TxPacketGet;                    // number of tx pending
    ULONG                TxPacketComplete;
    ULONG                TxPacketSend;
    ULONG                TxPacketSendComplete;
    //ULONG                NeedIndicatePacket;
    //ULONG                DisplayTxMsg;

    //dralee port
    //ULONG                 ResourcesEvts;    
    //ULONG                     PendingTxCnt; 

    ULONG                 ulTXByteInLastPeriod; 
//dralee -- 09162005, It seems unused.  
    BOOLEAN               plusDebug;  
    //BOOLEAN               bCmdDownloadLock;
    //011506 unused
    //ULONG                 bPowerUpDevice;  
    
    //dralee 20051031 for test
    //ULONG                 txqnodeused;
    ULONG                   sp_evt; 
    // int                    deauthcnt; //012207
    //BSS_DESCRIPTION_WPA_IE      CurrentWPA_IE;

    // there was a TX packet request rejected
    //BOOLEAN bTxPacketWaiting;
    CmdCtrlNode             *PSCurCmd;     // Current Command


    ULONG                               Enable80211D;
    ULONG                               DefaultRegion;
    ULONG                               DefaultBand;


    HostCmd_DS_802_11_SLEEP_PARAMS      SleepParams;
    UCHAR                               SleepParamFlag;

    WMM_DESC            WmmDesc;
    UCHAR                   AckPolicy[4];
    UCHAR                           AckPolicyState;
    WMM_AC_STATUS                   WmmAcStatus[4];
    UCHAR                           WmmStatusState;
        PNDIS_PACKET                    CurrentTxPkt; 


//tt ++ v5 firmware
    HANDLE      hOidQueryEvent;
    UCHAR       OidCmdRespBuf[MRVDRV_SIZE_OF_CMD_BUFFER];
    ULONG       nSizeOfOidCmdResp;
    USHORT      nOidCmdResult;
//tt --

    UINT            NumOfPmkid;
    PMKID_CACHE PmkidCache[MAX_PMKID_CACHE];
    NDIS_802_11_STATUS_INDICATION       RSNStatusIndicated;
    NDIS_802_11_PMKID_CANDIDATE_LIST    RSNPmkidCandidateList;
    PMKID_CANDIDATE                 PmkidCandidateArray[MAX_PMKID_CANDIDATE];

    DWORD   nAssocRspCount; //tt ra fail
    //NDIS_MINIPORT_TIMER       MrvDrvSendCompleteTimer; //tt ra fail   

     ULONG            SdioIstThread;

    //Plus++, 012706,  802.1x wep, for 802.1x eap-xxx wep issue      
     USHORT  txKeyIndex; 
     USHORT  txKeyLength;
     UCHAR    txKeyMaterial[16];   
    //Plus--     

    //Plus++, 012606(for Broadcom mix mode fix)
    WPA_SUPPLICANT      wpa2_supplicant;
	//Plus--, 012606  
	//Junius Added 20071017
	WPA_SUPPLICANT      wpa_supplicant;
	//end added
    //++dralee_20060327
    ULONG TxLock;
    USHORT sleepperiod;  

    NDIS_MINIPORT_TIMER     MrvDrvAvoidScanAfterConnectedTimer; 
    BOOLEAN                            bAvoidScanAfterConnectedforMSecs;
    ULONG                               AvoidScanTime;  
    ///assoc-patch ++
    UCHAR       fmver[4];
    ///assoc-patch --
    ///crlo:filter_32_ESSID ++
    DWORD       ESSID_32;
    ///crlo:filter_32_ESSID --
    DWORD       BusPowerInD3;           ///Bus power is on/off in D3 or not
    // tt mic error ++
    BOOLEAN                             bMicErrorIsHappened;
    // tt mic error --
    // tt ++ mic error 2
    ULONG                                   ulMicErrorStartTime;
    // tt --
    NDIS_MINIPORT_TIMER           MrvMicDisconnectTimer; // tt ++ mic 2
    BOOLEAN                     DisconnectTimerIsSet;

	///================================
	///Parameters for CCX
	///
	///CCX_RADIOMEASURE
	WORD				dialogToken;
	WORD				MeasureToken;
	BYTE				MeasureMode;
	WORD				MeasureDuration;
	DWORD				parentTSF;
	BOOLEAN				isPassiveScan;      /// Passive/Active scan
	BYTE				ChannelNum;     ///The channel we need to scan
	WORD				MeasureDurtion;     ///Scan duration
	///CCX_RADIOMEASURE
	///CCX_FASTROAM
	FASTROAMPARAM         frParam;
	///CCX_FASTROAM
	///CCX_V4_S56
	UCHAR				CurrentTspec[64];
	BOOLEAN				bHasTspec;
	UCHAR				RoamAccount;
	USHORT				RoamDelay;
	ULONG				RoamStartTime;	
	///CCX_V4_S56
	///CCX_CCKM
	HANDLE				WaitCCKMIEEvent;
	///CCX_CCKM
	ULONG				bEnableS56DriverIAPP;
	BOOLEAN				bStartCcxS56Test;
	NDIS_MINIPORT_TIMER	hCcxV4S56TestTimer;   
	BOOLEAN				bCcxV4S56TestTimerIsSet;  
	ULONG				TSMRinterval;

    ///Others...
    WROAMEXTPARAM   wlanRoamExtParam;       ///External parameters for roaming
    PWROAMPARAM pwlanRoamParam;
    PRECONNECTARG   pReconectArg;               ///Reconnection argument
    
    //UCHAR          BgScanCfg[LENGTH_BGSCAN_CFG];
    UCHAR          *BgScanCfg;
    ULONG           nBgScanCfg;       
    BOOLEAN         bBgScanEnabled;
    ULONG           bActiveRoamingwithBGSCAN;
    BOOLEAN         bPSEnableBGScan;
    ULONG           ulScanInterval;
    BOOLEAN         bBgDeepSleep;
    UCHAR  BgScanCfgInfo[2][LENGTH_BGSCAN_CFG];
    ULONG  BgScanCfgInfoLen[2];
    
     
    USHORT TxRate;

    UCHAR              PACfg[LENGTH_PA_CFG];   

    OID_MRVL_DS_HOST_SLEEP_CFG       SleepCfg;

    USHORT           IbssCoalsecingEnable;
     
    USHORT           PPS_Enabled;  
    USHORT           LogEventCfg;
    USHORT           ChipResetting;
    ULONG            SoftIntStatus;

    USHORT            SpecifyChannel;
    USHORT            ChannelMaxDuration;
    USHORT            ChannelMinDuration;
    
    BOOLEAN            NeedDisconnectNotification;
    ULONG              RoamingMode;
    UCHAR              ConnectedAPAuthMode;
    UCHAR              NeedSetWepKey;

      
    ULONG             NullPktInterval;
    ULONG             MultipleDTim;
    ULONG             AdhocAwakePeriod; 

    USHORT           TCloseWZCFlag;
    ULONG           TX_Control_Value;
    ULONG           MacFrameType;  // 0: use firmware default, 802.3+LLC (default registry vaule), 1: Ehternet II
    ULONG           WaitEventType;
    ULONG           SdioFastPath; // 0: disable (default value), 1: enable.
    //define GPIO pin for INT use
    ULONG           GPIOIntPinNumber;
    ULONG           GPIOIntTriggerEdge;
    ULONG           GPIOIntPulsewidth;                           
    UCHAR           ReqBSSID[MRVDRV_ETH_ADDR_LEN]; 

    HANDLE      hPendOnCmdEvent;
    DWORD           RoamSignalStrengthThreshold;                ///Roaming Signal Strength Threshold
    ///Parameters to be set from the registry file    
    DWORD           RoamChannelScanList;                        ///Channel list
    DWORD           RoamMaxScanInterval, RoamMinScanInterval;   ///Scan interval per channel (ms)
    DWORD           RoamDiffRSSIThreshold;                      ///RSSI threshold. We will roam to the other AP if it's RSSI is higher more than this value.
    DWORD           RoamScanAlgorithm;                          ///Scan algorithm. 0: BG-Scan, 1: Active-Scan
    //032007 fix adhoc indication
    ULONG           bIsMoreThanOneStaInAdHocBSS; 
    ULONG           AutoDeepSleepTime;
    DWORD           IsAutoDeepSleep;
    AUTO_DS_RECORD  AutoDsRec;
    ULONG           AssoRetryTimes;     


    DWORD           OIDAccessBlocked;   //060407 

    DWORD           dwTxTimeoutCount;

    ULONG           UseMfgFw;     // Set from the registry file

    ULONG PowerBitFlag;   //071107
    HANDLE hHwSpecEvent;

    //071207



    WMM_ADDTS_RESP      AddTSResp;
    
}MRVDRV_ADAPTER, *PMRVDRV_ADAPTER;  

                    
//dralee, 
//define cause of return resources
#define RSC_PS         (1<<0)
#define RSC_SCANNING   (1<<1)
#define RSC_BUF_FULL   (1<<2)


#pragma pack()
             
               
//dralee
typedef enum
{
    // card is in full power
    PS_STATE_FULL_POWER,
    // PS mode, but card is ready for TX
    PS_STATE_WAKEUP,
    // PS Mode, card is not ready for TX
    PS_STATE_SLEEP, 
    // SLEEP event is already received, but have not sent confirm yet
  //PS_STATE_SLEEP_PENDING, dralee, 072705    
} PS_STATE;  


typedef enum
{
  HTWK_STATE_FULL_POWER, 
  HTWK_STATE_PRESLEEP, 
  HTWK_STATE_PREAWAKE, 
  HTWK_STATE_SLEEP, 
} HTWK_STATE;

//012207
typedef enum 
{
  DS_STATE_NONE,
  DS_STATE_PRESLEEP,
  DS_STATE_SLEEP
} DS_STATE;


// Reconnect Reason 
typedef enum
{
    RECONNECT_ASSOCIATE_FAIL =1,
    RECONNECT_LINK_LOST,
    RECONNECT_DEAUTHENTICATE,
    ///RECONNECT_ROAMING,               ///=> Handled by roaming
    RECONNECT_D3_TO_D0,
    RECONNECT_DEEP_SLEEP_AWAKE,
    RECONNECT_HIDE_SSID,
    ///RECONNECT_RSSI_LOW               ///=> Handled by roaming
} RECONNECT_REASON;

// Reconnect Type
typedef enum
{     //
    RECONNECT_COUNTER_TYPE = 1,
    //
    RECONNECT_PERIOD_TYPE,
    //  
    RECONNECT_RSSI_TYPE
} RECONNECT_TYPE;

#define RE_CONNECT_PERIOD_TIME    8000    // 8 sec


#define ROAMING_MAX_RETRY      3
typedef enum {   
    NOT_ROAMING_MODE = 1, 
    ACTIVE_ROAMING_MODE,            ///active roaming
    FAST_ROAMING_MODE,              ///fast roaming (for CCX), expired~
    SMLS_ROAMING_MODE              ///seamless roaming (new implemented. ref: wlan_roam.c)
} ROAMING_MODE;                      

#endif

             






