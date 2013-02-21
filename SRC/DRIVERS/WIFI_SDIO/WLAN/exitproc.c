/*******************  Marvell Semiconductor, Inc., ***************************
 *
 *  Purpose:    This module provides NDIS shutdown routines
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/12/15 $
 *
 *  $Revision: #16 $
 *
 *****************************************************************************/

/*
===============================================================================
            INCLUDE FILES
===============================================================================
*/
#include "precomp.h"
#include "If.h"
///crlo:cckm-fastroaming ++
#include "wlan_thread.h"

extern CRITICAL_SECTION    LoaderCriticalSection;
extern HANDLE g_hLoaderThread;
extern BOOL g_fMrvDrvHaltCalled;

///crlo:cckm-fastroaming --
#include "wlan_roam.h"
/*
===============================================================================
            CODED PUBLIC PROCEDURES
===============================================================================

/******************************************************************************
 *
 *  Name: MrvDrvReset()
 *
 *  Description: 
 *      NDIS miniport reset routine.
 *
 *  Conditions for Use:   
 *      Will be called by NDIS wrapper to reset the device
 *
 *  Arguments:            
 *      OUT PBOOLEAN AddressingReset
 *      IN  NDIS_HANDLE MiniportAdapterContext
 *    
 *  Return Value:         
 * 
 *  Notes:                
 *
 *****************************************************************************/
NDIS_STATUS
MrvDrvReset(
  OUT PBOOLEAN AddressingReset,
  IN  NDIS_HANDLE MiniportAdapterContext
  )
{
  
  PMRVDRV_ADAPTER Adapter;
  BOOLEAN  bDoNotify = FALSE;
    BOOLEAN                            timerStatus;
  DBGPRINT(DBG_ERROR,(L"WARNING: +MrvDrvReset() \r\n"));

   
    //MrvPrintFile("\nMrvDrvReset");
 

  Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;

Adapter->bIsPendingReset = TRUE; // tt cetk_1
/* tt ra fail
  Adapter->bIsPendingReset = TRUE;
*/

  // Set AddressingReset indication
  *AddressingReset = FALSE;
    // report link status change
  //ResetDisconnectStatus(Adapter);
  
  if ( Adapter->MediaConnectStatus == NdisMediaStateConnected)
  {
    bDoNotify = TRUE;
	DBGPRINT(DBG_LOAD, (L"set Disconnected(2)!\n"));
    Adapter->MediaConnectStatus = NdisMediaStateDisconnected;
	      if(Adapter->bAvoidScanAfterConnectedforMSecs == TRUE)  
	     {
                NdisMCancelTimer(&Adapter->MrvDrvAvoidScanAfterConnectedTimer, &timerStatus);
		  Adapter->bAvoidScanAfterConnectedforMSecs=FALSE;		 
	      	}
  }

    //tx
    CleanUpSingleTxBuffer(Adapter);

    // rx
    ResetRxPDQ(Adapter);
  
  // Enable interrupt
  If_EnableInterrupt(Adapter);

  if (bDoNotify == TRUE)
  {
    NdisMSetTimer(&Adapter->MrvDrvIndicateConnectStatusTimer, 10);
    Adapter->DisconnectTimerSet = TRUE;
  }
  else
  {
        Adapter->bIsPendingReset = FALSE;
  }

  return NDIS_STATUS_SUCCESS;
  
}


/******************************************************************************
 *
 *  Name: MrvDrvCheckForHang()
 *
 *  Description:
 *      NDIS miniport check for hang routine. 
 *
 *  Conditions for Use:
 *      Will be called by NDIS wrapper to determine current station operation status.
 *      If the station is not responding, NDIS wrapper will call MrvDrvReset() to reset 
 *      the station. Driver first check and use current HW status stored in device 
 *      object to report system then update the status and send command to query HW status.
 *
 *  Arguments:
 *      IN NDIS_HANDLE MiniportAdapterContext
 *    
 *  Return Value:
 *    TRUE if the NIC HW is not operating properly
 *    FALSE if the NIC HW is OK
 * 
 *  Notes: According to Microsoft NDIS document, this function is normally called 
 *         by NDIS wrapper approximately every 2 seconds.
 *
 *****************************************************************************/
BOOLEAN
MrvDrvCheckForHang(
  IN NDIS_HANDLE MiniportAdapterContext
  )
{
    PMRVDRV_ADAPTER  Adapter;

    UCHAR            ucHostIntStatus;
    UCHAR            ucCardStatus;
    SD_API_STATUS    status;

    DBGPRINT(DBG_LOAD|DBG_WARNING,(L"INIT - Enter MrvDrvCheckForHang\n"));
	//return FALSE; //tt ra fail    
  
    Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;   

    if ( Adapter->SurpriseRemoved )
    {
        DBGPRINT(DBG_WARNING, (L"[MRVL] in CheckForHang, card is removed, return FALSE directly\n") );
        return FALSE;
    }


    if ( Adapter->SentPacket )
    {
        Adapter->dwTxTimeoutCount ++;
        if ( Adapter->dwTxTimeoutCount > MRVDRV_TX_TIMEOUT_COUNT_THRESHOLD )
        {
            DBGPRINT( DBG_ERROR, (L"Tx timeout!\n") );
            Adapter->TxPktTimerIsSet=TRUE;
            NdisMSetTimer(&Adapter->MrvDrvTxPktTimer, 0);
            Adapter->dwTxTimeoutCount = 0;
        }
    }

                     
    //012207
    // We won't check in deepsleep, ps mode, HostSleep and D3 state.  
    // In Deep Sleep Mode no packet can be sent out 
    if( !IsThisDsState(Adapter, DS_STATE_NONE) )
       return FALSE;

    if( (!IsThisHostPowerState(Adapter, HTWK_STATE_FULL_POWER)) ||
        (Adapter->psState == PS_STATE_SLEEP) ||
        (Adapter->CurPowerState != NdisDeviceStateD0)
		)
       return FALSE;
    if ( Adapter->IntWARTimeoutCount )
         AutoDeepSleepCounter(Adapter);
    else
         Adapter->AutoDsRec.timer = 0;
    
    //060407 flag to start auto deep sleep counter 
    //This flag also used to interrupt missing. 
    if(((++Adapter->IntWARTimeoutCount) & 0x1) == 0 )
       return FALSE;    

    //Adapter->IntWARTimeoutCount = 0; 

    if( IsThisDsState(Adapter, DS_STATE_SLEEP) )
        return FALSE;

    status = If_ReadRegister(Adapter,
                             //SD_IO_READ ,
                             1, // function 1
                             HCR_HOST_INT_STATUS_REGISTER, 
                             FALSE,
                             &ucHostIntStatus,
                             sizeof(ucHostIntStatus));
    
    if (!SD_API_SUCCESS(status))
    {
	    DBGPRINT(DBG_ERROR, (L"Read error in CheckForHang()\r\n") );
        return FALSE;  
    }

    if( ucHostIntStatus & (UPLOAD_HOST_INT_STATUS_RDY | DOWNLOAD_HOST_INT_STATUS_RDY) ) 
    {
     
        EnterCriticalSection(&Adapter->IntCriticalSection);   
        Adapter->ucGHostIntStatus |= ucHostIntStatus; 
        LeaveCriticalSection(&Adapter->IntCriticalSection);   
        ucCardStatus = ~ucHostIntStatus;    
        ucCardStatus &= 0x1f;
        status = If_WriteRegister(Adapter,
                                  //SD_IO_WRITE,          
                                  1,     
                                  HCR_HOST_INT_STATUS_REGISTER,
                                  FALSE,
                                  &ucCardStatus,   // reg
                                  sizeof(ucCardStatus));   
      
        if (!SD_API_SUCCESS(status))
        {
            DBGPRINT(DBG_ISR,(L"Unable to clear Host interrupt status register\r\n"));
            return FALSE;
        } 
        
        SetEvent(Adapter->hControllerInterruptEvent);
    }

        

    return FALSE;
    
}

/******************************************************************************
 *
 *  Name: MrvDrvHalt()
 *
 *  Description:  NDIS miniport halt event handler
 *
 *
 *  Arguments:    IN NDIS_HANDLE MiniportAdapterContext
 *          Miniport context
 *    
 *  Return Value:   None
 *
 *  Notes: 
 *
 *****************************************************************************/

VOID
MrvDrvHalt(
  IN NDIS_HANDLE MiniportAdapterContext
  )
{
  PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;
  
  DBGPRINT(DBG_UNLOAD|DBG_ERROR,(L"EXIT - Enter MrvDrvHalt\n"));

  EnterCriticalSection(&LoaderCriticalSection);  

  
  Adapter->SurpriseRemoved = TRUE;
  SetEvent(Adapter->hEjectEvent);

  // Disable interrupt
  If_DisableInterrupt(Adapter);
  Adapter->INTCause =0;

  if(Adapter->hControllerInterruptEvent)
    SetEvent(Adapter->hControllerInterruptEvent);

  if(Adapter->hControllerInterruptThread)
  {
    DBGPRINT( DBG_UNLOAD, (L"Wait for the end of IstThread...\r\n") );
    WaitForSingleObject(Adapter->hControllerInterruptThread,INFINITE);
    DBGPRINT( DBG_UNLOAD, (L"... done\r\n") );
  }

  FreeAdapterObject(Adapter);
  g_fMrvDrvHaltCalled = TRUE;

  LeaveCriticalSection(&LoaderCriticalSection);

  DBGPRINT(DBG_UNLOAD,(L"EXIT - Leave MrvDrvHalt\n"));
  return;
}


/******************************************************************************
 *
 *  Name: MrvDrvShutdownHandler()
 *
 *  Description:  NDIS miniport shutdown event handler
 *
 *
 *  Arguments:    IN NDIS_HANDLE MiniportAdapterContext
 *          Miniport context
 *    
 *  Return Value:   None
 *
 * 
 *  Notes: 
 *
 *****************************************************************************/
VOID
MrvDrvShutdownHandler(
  IN NDIS_HANDLE MiniportAdapterContext
  )
{
  PMRVDRV_ADAPTER Adapter;

  DBGPRINT(DBG_UNLOAD|DBG_ERROR,(L"+MrvDrvShutdownHandler()\n"));
  Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;

  //      Shutdown the NIC
  //
  PrepareAndSendCommand(
    Adapter, 
    HostCmd_CMD_802_11_RESET, 
    HostCmd_ACT_HALT, 
    HostCmd_OPTION_NO_INT,
    (NDIS_OID)0,
    HostCmd_PENDING_ON_NONE,
    0,
    FALSE,
    NULL,
    NULL,
    NULL,
    NULL);
  
  DBGPRINT(DBG_LOAD,(L"-MrvDrvShutdownHandler()\n"));
  return;
}

/*
===============================================================================
            CODED LOCAL PROCEDURES
===============================================================================
*/

VOID CleanUpTimers(PMRVDRV_ADAPTER Adapter)
{

    BOOLEAN  timerStatus;

  // Cancel and reset NDIS Tiemr variables
        
    //RETAILMSG(1,(TEXT("Exit: CleanUPTimers \r\n")));

    if(Adapter->DisconnectTimerIsSet == TRUE)
    {
        NdisMCancelTimer(&Adapter->MrvMicDisconnectTimer, &timerStatus);
        Adapter->DisconnectTimerIsSet = FALSE;
    }

    if ( Adapter->CommandTimerSet == TRUE )
    {
        NdisMCancelTimer(&Adapter->MrvDrvCommandTimer, &timerStatus);
        Adapter->CommandTimerSet = FALSE;
    }

    //dralee
    if(Adapter->TxPktTimerIsSet == TRUE)
    {
      NdisMCancelTimer(&Adapter->MrvDrvTxPktTimer, &timerStatus); 
      Adapter->TxPktTimerIsSet = FALSE;   
    } 

    if ( Adapter->DisconnectTimerSet ) 
    {
        NdisMCancelTimer(&Adapter->MrvDrvIndicateConnectStatusTimer, &timerStatus);
        Adapter->DisconnectTimerSet = FALSE;
    } 
     
    //dralee
    if( Adapter->MrvDrvSdioCheckFWReadyTimerIsSet == TRUE )
    {
        NdisMCancelTimer(&Adapter->MrvDrvSdioCheckFWReadyTimer, &timerStatus);
        Adapter->MrvDrvSdioCheckFWReadyTimerIsSet = FALSE;
    }   

      if ( Adapter->ReConnectTimerIsSet == TRUE )
    	{
       	NdisMCancelTimer(&Adapter->MrvReConnectTimer, &timerStatus);
	    	Adapter->ReConnectTimerIsSet = FALSE;
    	}

	      if(Adapter->bAvoidScanAfterConnectedforMSecs == TRUE)  
	     {
                NdisMCancelTimer(&Adapter->MrvDrvAvoidScanAfterConnectedTimer, &timerStatus);
		  Adapter->bAvoidScanAfterConnectedforMSecs=FALSE;		 
	      	}
     


}
/******************************************************************************
 *
 *  Name: ResetAdapterObject()
 *
 *  Description:  Reset all the variables in the Adapter object
 *
 *  Arguments:  
 *    
 *  Return Value:   None
 *
 *  Notes: 
 *
 *****************************************************************************/
VOID
ResetAdapterObject(PMRVDRV_ADAPTER Adapter)
{
  //      Disable interrupt, reset INTCause
  If_DisableInterrupt(Adapter);
  Adapter->INTCause =0;

  CleanUpTimers(Adapter);
  Adapter->TimerInterval = MRVDRV_DEFAULT_TIMER_INTERVAL;

  // Tx-related variables
  CleanUpSingleTxBuffer(Adapter);

  // Rx-related variables
  ResetRxPDQ(Adapter);
  
  // Command-related variables
  ResetCmdBuffer(Adapter);  

  // Operation characteristics
  Adapter->CurrentLookAhead = (ULONG)MRVDRV_MAXIMUM_ETH_PACKET_SIZE - MRVDRV_ETH_HEADER_SIZE;
  // 9/19/02: cannot set ProtocolOption to 0
  // Adapter->ProtocolOptions = 0;
  Adapter->MACOptions  = (ULONG)NDIS_MAC_OPTION_NO_LOOPBACK;
  DBGPRINT(DBG_LOAD, (L"set Disconnected!(3)\n"));
  Adapter->MediaConnectStatus = NdisMediaStateDisconnected;
  Adapter->MediaInUse = NdisMedium802_3;
  Adapter->LinkSpeed = MRVDRV_LINK_SPEED_0mbps;

  // Status variables
  Adapter->HardwareStatus = NdisHardwareStatusReady;

  // 802.11 specific
  // need to keep WEPStatus, AuthenticationMode, InfrastructureMode
  //Adapter->WEPStatus = Ndis802_11WEPDisabled;
  //Adapter->AuthenticationMode = Ndis802_11AuthModeOpen;
  //Adapter->InfrastructureMode = Ndis802_11Infrastructure;
  Adapter->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
  Adapter->ulNumOfBSSIDs =0;
  Adapter->ulCurrentBSSIDIndex =0;
  Adapter->ulAttemptedBSSIDIndex =0;
  ///Adapter->bAutoAssociation = FALSE;
  Adapter->bIsAssociateInProgress = FALSE;

  Adapter->bIsScanInProgress = FALSE;
  Adapter->ulLastScanRequestTime =0;

  // Memorize the previous SSID and BSSID
  if ( Adapter->CurrentSSID.SsidLength != 0 ) 
  {
    NdisMoveMemory( &(Adapter->PreviousSSID), &(Adapter->CurrentSSID), sizeof(NDIS_802_11_SSID));
    NdisMoveMemory( Adapter->PreviousBSSID, Adapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
  }

  NdisZeroMemory(&(Adapter->CurrentSSID), sizeof(NDIS_802_11_SSID));
  NdisZeroMemory(Adapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
  
  NdisZeroMemory( &(Adapter->CurrentBSSIDDesciptor), 
                    sizeof(NDIS_WLAN_BSSID_EX));
  
  NdisZeroMemory( &(Adapter->CurrentBssDesciptor), 
                    sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));
    
  NdisZeroMemory( Adapter->BSSIDList, 
                  sizeof(NDIS_WLAN_BSSID_EX) * MRVDRV_MAX_BSSID_LIST);
/*  
  NdisZeroMemory( Adapter->PSBSSIDList, 
        sizeof(NDIS_WLAN_BSSID_EX) * MRVDRV_MAX_BSSID_LIST);
*/
  NdisZeroMemory(Adapter->BssDescList, 
          sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS) * MRVDRV_MAX_BSSID_LIST);
/*
  NdisZeroMemory(Adapter->PSBssDescList, 
        sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS) * MRVDRV_MAX_BSSID_LIST);
*/
  NdisZeroMemory(&Adapter->CurrentConfiguration,sizeof(NDIS_802_11_CONFIGURATION));

  // Initialize RSSI-related variables
  Adapter->LastRSSI = MRVDRV_RSSI_DEFAULT_NOISE_VALUE;
  Adapter->RSSITriggerValue = MRVDRV_RSSI_TRIGGER_DEFAULT;
  Adapter->RSSITriggerCounter = 0;

  Adapter->CurPowerState = NdisDeviceStateD0;
  Adapter->SupportTxPowerLevel = MRVDRV_TX_POWER_LEVEL_TOTAL;
  Adapter->CurrentTxPowerLevel = HostCmd_ACT_TX_POWER_INDEX_MID;

  // Clean up current WEP key
  NdisZeroMemory(&(Adapter->CurrentWEPKey), sizeof(MRVL_WEP_KEY));

  Adapter->SoftwareFilterOn = FALSE;

  Adapter->bIsReconnectEnable = FALSE;

  //012207
  //Adapter->IsDeepSleep = FALSE;
  //Adapter->IsDeepSleepRequired = FALSE;
  SetDsState( Adapter, DS_STATE_NONE );

   Adapter->bIsSystemConnectNow = FALSE;
   Adapter->bIsReconnectEnable = FALSE; 
   Adapter->bIsBeaconLoseEvent = FALSE;
   Adapter->bIsDeauthenticationEvent = FALSE;

  Adapter->WEPStatus = Ndis802_11WEPDisabled;
  Adapter->AuthenticationMode = Ndis802_11AuthModeOpen;
  Adapter->InfrastructureMode = Ndis802_11Infrastructure;
  Adapter->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
  Adapter->ulNumOfBSSIDs =0;
  Adapter->ulCurrentBSSIDIndex =0;
  Adapter->ulAttemptedBSSIDIndex =0;
  ///Adapter->bAutoAssociation = FALSE;
  Adapter->LastRSSI =  MRVDRV_RSSI_DEFAULT_NOISE_VALUE;
  // Adapter->RxPDIndex = 0;
  Adapter->ulLastScanRequestTime =0;

  //Adapter->EncryptionStatus = Ndis802_11EncryptionDisabled;
    // setup association information buffer
  {
        PNDIS_802_11_ASSOCIATION_INFORMATION pAssoInfo;
        //ULONG   ulLength;
        
        pAssoInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION) 
                        Adapter->AssocInfoBuffer;

       // ulLength = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);

        //ulLength = (ulLength) + 3

        DBGPRINT(DBG_LOAD, (L"pAssoInfo = 0x%x, pAssoInfo->Length = 0x%x\n", pAssoInfo, 
                            &(pAssoInfo->Length)));
        // assume the buffer has already been zero-ed
        // no variable IE, so both request and response IE are
        // pointed to the end of the buffer, 4 byte aligned
        pAssoInfo->OffsetRequestIEs = 
        pAssoInfo->OffsetResponseIEs = 
        pAssoInfo->Length = ((sizeof(NDIS_802_11_ASSOCIATION_INFORMATION) + 3) 
                                /4) * 4 ;
  }

  return;
}


/******************************************************************************
 *
 *  Name: FreeAdapterObject()
 *
 *  Description:  Device object clean-up routine
 *
 *
 *  Arguments:    IN NDIS_HANDLE MiniportAdapterContext
 *          Miniport context
 *    
 *  Return Value:   None
 *
 *  Notes: 
 *
 *****************************************************************************/
VOID
FreeAdapterObject(PMRVDRV_ADAPTER Adapter)
{
	//free TX Q
	CleanUpSingleTxBuffer(Adapter); 
	// Free Rx Q
	FreeRxQ(Adapter);

	wlan_ccx_free(Adapter);
	///CCX_CCKM
	if (Adapter->WaitCCKMIEEvent != NULL) {
		CloseHandle(Adapter->WaitCCKMIEEvent);
	}
	///CCX_CCKM

	// Relase command buffer
	FreeCmdBuffer(Adapter);
	CleanUpStationHW(Adapter);

	if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
		wlan_roam_deinit(Adapter->pwlanRoamParam);
	}

	if ( Adapter->InitializationStatus & MRVDRV_INIT_STATUS_MAP_REGISTER_ALLOCATED )
	{
		// Release map registers
		NdisMFreeMapRegisters(Adapter->MrvDrvAdapterHdl);
	}

	CleanUpTimers(Adapter);

	// Free the adapter object itself
	MRVDRV_FREE_MEM((PVOID)Adapter, sizeof(MRVDRV_ADAPTER));

	return;
}

/******************************************************************************
 *
 *  Name: CleanUpStationHW()
 *
 *  Description:  Clean up HW memory mapping and other system resource
 *
 *  Arguments:    Miniport context
 *    
 *  Return Value:   None
 *
 * 
 *  Notes: 
 *
 *****************************************************************************/
VOID
CleanUpStationHW(IN PMRVDRV_ADAPTER Adapter)
{
     
    DeleteCriticalSection(&Adapter->TxCriticalSection);
    DeleteCriticalSection(&Adapter->IntCriticalSection);
    DeleteCriticalSection(&Adapter->CmdQueueExeSection);
    DeleteCriticalSection( &Adapter->CsForDSState );
    DeleteCriticalSection( &Adapter->CsForHostPowerState );
  
    NdisFreeSpinLock(&Adapter->RxQueueSpinLock);
    NdisFreeSpinLock(&Adapter->FreeQSpinLock);
    NdisFreeSpinLock(&Adapter->PendQSpinLock);   
           
    
    CloseHandle(Adapter->hControllerInterruptEvent); 
    CloseHandle(Adapter->TxResourceControlEvent);
    CloseHandle(Adapter->hOidQueryEvent);
    CloseHandle(Adapter->hThreadReadyEvent);
    CloseHandle( Adapter->hHwSpecEvent );
    CloseHandle(Adapter->hControllerInterruptThread);
    CloseHandle( Adapter->hEjectEvent );

	CloseHandle(Adapter->hPendOnCmdEvent);

}

VOID
AutoDeepSleepCounter(IN PMRVDRV_ADAPTER Adapter)
{
    //032107
    //Handle Auto Deep Sleep checking 
    if( Adapter->AutoDeepSleepTime != 0 )
    { 
        if(IsThisDsState(Adapter, DS_STATE_NONE) &&
           Adapter->MediaConnectStatus == NdisMediaStateDisconnected )
        {
           if( Adapter->AutoDsRec.LastSeqNum != Adapter->SeqNum )
           {   //starting new check point                             
               Adapter->AutoDsRec.LastSeqNum = Adapter->SeqNum;
               Adapter->AutoDsRec.timer = 0;
           }
           else
           {    
               //device idle over the autodeepsleep time in disconnected state
               //enter deep sleep
               if( ++Adapter->AutoDsRec.timer >= Adapter->AutoDeepSleepTime )
               {      
                  DBGPRINT(DBG_DEEPSLEEP|DBG_OID|DBG_HELP,(L"signal Auto Deep Sleep\r\n"));
                  Adapter->SoftIntStatus |= MRVL_SOFT_INT_AutoDeepSleep;
                  Adapter->IsAutoDeepSleep = 1;
                  SetEvent(Adapter->hControllerInterruptEvent);
                  Adapter->AutoDsRec.timer = 0;
               }   
           }
        }
        else
           Adapter->AutoDsRec.timer = 0;    
    }    
}



