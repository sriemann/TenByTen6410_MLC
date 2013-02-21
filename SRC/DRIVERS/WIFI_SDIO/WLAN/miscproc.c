/******************* ?Marvell Semiconductor, Inc., ***************************
 *
 *  Purpose:    This module provides the implementation of power managment 
 *              and timer routines
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/12/15 $
 *
 *  $Revision: #12 $
 *
 *****************************************************************************/
 

/*
===============================================================================
                                 INCLUDE FILES
===============================================================================
*/
#include "precomp.h"
NDIS_STATUS 
MrvDrvSetEncryptionStatus(
  IN PMRVDRV_ADAPTER          Adapter,
  IN NDIS_802_11_ENCRYPTION_STATUS  EncryptionStatus,
    IN NDIS_OID Oid,
  IN PVOID InformationBuffer,
  OUT PULONG BytesRead,
  OUT PULONG BytesNeeded
);
/*
===============================================================================
                                  DEFINITIONS
===============================================================================
*/

/*
===============================================================================
                           CODED PUBLIC PROCEDURES
===============================================================================
*/

/******************************************************************************
 *
 *  Name: MrvDrvIndicateConnectStatusTimer()
 *
 *  Description: Timer that will be fired after initialize routine is done
 *               to indicate disconnect
 *
 *
 *  Arguments:           
 *      IN PVOID SystemSpecific1
 *      IN PVOID FunctionContext
 *      IN PVOID SystemSpecific2
 *      IN PVOID SystemSpecific3
 *    
 *  Return Value: None
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
MrvDrvIndicateConnectStatusTimer(
    IN PVOID SystemSpecific1,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{

    PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER) MiniportAdapterContext;

    if  (Adapter->bIsPendingReset == TRUE)  //  used for rest 
    {
        Adapter->MediaConnectStatus = NdisMediaStateConnected ;
        Adapter->bIsPendingReset = FALSE;
    }
    Adapter->DisconnectTimerSet = FALSE;
}


/******************************************************************************
 *
 *  Name: MrvDrvAvoidScanAfterConnectedTimer()
 *
 *  Description: Timer that will be fired after initialize routine is done
 *               to indicate connected
 *
 *
 *  Arguments:           
 *      IN PVOID SystemSpecific1
 *      IN PVOID FunctionContext
 *      IN PVOID SystemSpecific2
 *      IN PVOID SystemSpecific3
 *    
 *  Return Value: None
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
MrvDrvAvoidScanAfterConnectedTimer(
    IN PVOID SystemSpecific1,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{

    PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER) MiniportAdapterContext;

       Adapter->bAvoidScanAfterConnectedforMSecs = FALSE;

}

/******************************************************************************
 *
 *  Name: MrvDrvTxPktTimerFunction()
 *
 *  Description: Tx Timer routine
 *
 *  Conditions for Use: (Not used in the current verison of driver)
 *
 *  Arguments:           
 *      IN PVOID SystemSpecific1
 *      IN PVOID FunctionContext
 *      IN PVOID SystemSpecific2
 *      IN PVOID SystemSpecific3
 *    
 *  Return Value: None
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID MrvDrvTxPktTimerFunction(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{     
    UCHAR ucHostIntStatus=0;
    SD_API_STATUS    status;
    UCHAR           ucCardStatus;
    PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER) FunctionContext;

    DBGPRINT(DBG_TMR| DBG_WARNING,(L"- TX Pkt timeout, Tx Done does not response \r\n"));
    Adapter->TxPktTimerIsSet=FALSE;
    DBGPRINT(DBG_ERROR,(L"--SDIO Tx Timeout: #%d pkts in Queue\r\n",Adapter->TxPacketCount));
    status = If_ReadRegister(Adapter,
                               //SD_IO_READ ,
                               1, // function 1
                               HCR_HOST_INT_STATUS_REGISTER, 
                               FALSE,
                               &ucHostIntStatus,
                               sizeof(ucHostIntStatus)); 
    if(ucHostIntStatus)
    {
        EnterCriticalSection(&Adapter->IntCriticalSection);   
        Adapter->ucGHostIntStatus |= ucHostIntStatus; 
        LeaveCriticalSection(&Adapter->IntCriticalSection);  
        ucCardStatus = ~ucHostIntStatus;    
        status = If_WriteRegister(Adapter,
                              //SD_IO_WRITE,          
                              1,     
                              HCR_HOST_INT_STATUS_REGISTER,
                              FALSE,
                              &ucCardStatus,   // reg
                              sizeof(ucCardStatus));    

        DBGPRINT(DBG_ERROR,(L"HOST INT status:%x:%x\n",ucHostIntStatus, status)); 
        SetEvent(Adapter->hControllerInterruptEvent);
    } 
    else 
    {   
        // clean up the timed out pkt
        HandleTxSingleDoneEvent(Adapter);                   
    } 
    return;
} 

VOID MrvDrvReConnectTimerFunction(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{

    
    PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER) FunctionContext;

    DBGPRINT(DBG_TMR|DBG_ASSO|DBG_HELP|DBG_RECONNECT|DBG_WARNING,(L"++ MrvDrvReConnectTimerFunction\n"));

    ReConnectHandler(Adapter);
    
    return;
}

/******************************************************************************
 *
 *  Name: MrvDrvCommandTimerFunction()
 *
 *  Description: Command time-out routine
 *
 *  Conditions for Use: When current command exceeds expected execution time limit,
 *                      commad will expire and call the command time-out ruotine
 *
 *  Arguments:           
 *      IN PVOID SystemSpecific1
 *      IN PVOID FunctionContext
 *      IN PVOID SystemSpecific2
 *      IN PVOID SystemSpecific3
 *    
 *  Return Value: None
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID MrvDrvCommandTimerFunction(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    CmdCtrlNode *pTempNode;
    PHostCmd_DS_GEN pRetPtr;

    // get the adpater context
    PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER) FunctionContext;

    if ( Adapter->CurCmd == NULL )
    {
        DBGPRINT(DBG_CMD|DBG_ERROR, (L"[MRVL] Command timeout handler, no current command, just return\n") );
        return;
    }

    do 
    {
        IF_API_STATUS   status;
        IF_API_STATUS     ifStatus;
        USHORT      usLength;
        UCHAR        ucHostIntStatus, ucHostIntMask, ucCardStatus, macEvent;

        pRetPtr = (PHostCmd_DS_GEN)(Adapter->CurCmd->BufVirtualAddr);

        if ( pRetPtr == NULL )
        {
            DBGPRINT(DBG_CMD|DBG_ERROR, (L"[MRVL] Command timeout handler, current command contains a NULL buffer pointer, just return\n") );
            return;
        }
        
        DBGPRINT(DBG_CMD|DBG_ERROR,(L"[MRVL] command timeout (0x%x) ********\r\n", pRetPtr->Command ));

        status = If_ReadRegister(Adapter,
                                        //SD_IO_READ ,
                                        1, // function 1
                                        HCR_HOST_INT_STATUS_REGISTER, 
                                        FALSE,
                                        &ucHostIntStatus,
                                        sizeof(ucHostIntStatus));

        status = If_ReadRegister(Adapter,
                                        //SD_IO_READ ,
                                        1, // function 1
                                        HCR_HOST_INT_MASK_REGISTER, 
                                        FALSE,
                                        &ucHostIntMask,
                                        sizeof(ucHostIntMask));

        
        if ( ucHostIntStatus & UPLOAD_HOST_INT_STATUS_RDY )
        {
            DBGPRINT(DBG_CMD|DBG_ERROR,( L"[TT] INT upload is ready! (int:0x%x, mask:0x%x)\n", ucHostIntStatus, ucHostIntMask ));

            ifStatus = If_GetCardStatusAndMacEvent( Adapter, &ucCardStatus, &macEvent );
            if ( !IF_IS_SUCCESS(ifStatus) )
            {
                DBGPRINT(DBG_CMD|DBG_ERROR,(L"[TT] cannot read card status and mac event (0x%x)\n", ifStatus ));
                break;
            }

            ifStatus = If_GetLengthOfDataBlock( Adapter, &usLength );
            if ( !IF_IS_SUCCESS(ifStatus) )
            {
                DBGPRINT(DBG_CMD|DBG_ERROR,( L"[TT] cannot read length of data block (0x%x)\n", ifStatus ));
                break;
            }

            DBGPRINT(DBG_CMD|DBG_ERROR,( L"[TT] length=%d, event=%d\n", usLength, macEvent ));

            if ( macEvent == IF_CMD_PKT )
            {
                ucHostIntStatus = 0xfe; // only clear bit 0 for upload ready status
                ucHostIntStatus &= 0x1f;
                DBGPRINT(DBG_CMD|DBG_ERROR,( L"[TT] command resp is ready, clear upload ready status (0x%x)\n", ucHostIntStatus ));
                status = If_WriteRegister(Adapter,
                                        //SD_IO_WRITE,          
                                        1,     
                                        HCR_HOST_INT_STATUS_REGISTER,
                                        FALSE,
                                        &ucHostIntStatus,   // reg
                                        sizeof(ucHostIntStatus));
                                           
                if( status != IF_SUCCESS )
                    DBGPRINT(DBG_CMD|DBG_ERROR,(L"[TT] clear is failed\r\n"));
            }
        }
        else
        {
            DBGPRINT(DBG_CMD|DBG_ERROR,( L"[TT] INT NO upload status (int:0x%x, mask:0x%x)\n", ucHostIntStatus, ucHostIntMask ));
        }
    } while( 0 );
    if( !Adapter->CurCmd ) // False alarm
    {
        DBGPRINT(DBG_TMR,(L"MrvDrvCommandTimerFunction() Timer fired, but no current command.\n"));

//lykao, 051305, begin      
        // Check CmdWaitQ and execute the next command
        GetCmdFromQueueToExecute(Adapter);
//lykao, 051305, end
        return;
    }   
 

    Adapter->ucNumCmdTimeOut++;
    DBGPRINT(DBG_TMR | DBG_CMD | DBG_ERROR,(L"MISC - Command TIME OUT !! \n"));
    pTempNode = Adapter->CurCmd;
    pRetPtr = (PHostCmd_DS_GEN)(pTempNode->BufVirtualAddr);

//lykao, 051305, begin
	///NoisyEnv-NoRemove ++
	/*
 if (Adapter->ucNumCmdTimeOut > 2)
   {
        DBGPRINT(DBG_CMD, (L"Cmd Timer: More than 2 time out! set AdapterRemoved To TRUE!\n"));
        Adapter->SurpriseRemoved = TRUE;
    }
*/
 	///NoisyEnv-NoRemove --

    
    // if the current command uses NO_INT option and is already finished
    if ((pRetPtr->Command & HostCmd_RET_NONE) != 0 &&
           pTempNode->INTOption == HostCmd_OPTION_NO_INT )
    {
        DBGPRINT(DBG_CMD,(L"NO INT COMMAND: calling HandleCommand FinishedEvent from timeout\n"));
          HandleCommandFinishedEvent(Adapter);

        return;
    }

///NoisyEnv ++
  if (((pRetPtr->Command == HostCmd_RET_802_11_SCAN) || (pRetPtr->Command == HostCmd_CMD_802_11_SCAN) ) &&
///NoisyEnv --  	
         (Adapter->CurCmd->Pad[2] |= MRVDRV_SCAN_CMD_END))
    {
                                     

    Adapter->bIsScanInProgress = FALSE;
    Adapter->bIsAssociationBlockedByScan = FALSE;
         
    }   

     if ((pRetPtr->Command == HostCmd_RET_802_11_ASSOCIATE)   &&
        (pRetPtr->Command == HostCmd_RET_802_11_REASSOCIATE) &&
        (pRetPtr->Command == HostCmd_RET_802_11_REASSOCIATE) &&
        (pRetPtr->Command == HostCmd_RET_802_11_AD_HOC_START))
    {
        Adapter->bIsAssociateInProgress = FALSE;
        Adapter->bIsAssociationBlockedByScan = FALSE;
        
    }

    /// old code : verify later
    if ((pRetPtr->Command == HostCmd_RET_802_11_SCAN ) &&
        (pRetPtr->Command == HostCmd_RET_802_11_ASSOCIATE)   &&
        (pRetPtr->Command == HostCmd_RET_802_11_REASSOCIATE) &&
        (pRetPtr->Command == HostCmd_RET_802_11_REASSOCIATE) &&
        (pRetPtr->Command == HostCmd_RET_802_11_AD_HOC_START))
    {
        // if there is a reset pending
        if (Adapter->bIsPendingReset == TRUE)
        {
            Adapter->bIsPendingReset = FALSE;
            DBGPRINT(DBG_TMR,(L"HWAC - Sending ResetComplete \n"));
            Adapter->HardwareStatus = NdisHardwareStatusReady;

            SetMacControl(Adapter);
            NdisMResetComplete(
                    Adapter->MrvDrvAdapterHdl,
                    NDIS_STATUS_SUCCESS,
                    FALSE);     
        }
    }

    // set the timer state variable to TRUE
    Adapter->isCommandTimerExpired = TRUE;

    if( pTempNode->PendingInfo == HostCmd_PENDING_ON_GET_OID )
    {
        NdisMQueryInformationComplete(
                Adapter->MrvDrvAdapterHdl,
                NDIS_STATUS_NOT_ACCEPTED
                );
        DBGPRINT(DBG_CMD, (L"Call NdisMQueryInformationComplete With FAILURE\n"));
        
    }
    else if( pTempNode->PendingInfo == HostCmd_PENDING_ON_SET_OID )
    {
        NdisMSetInformationComplete(
                Adapter->MrvDrvAdapterHdl,
                NDIS_STATUS_RESOURCES
                );
        DBGPRINT(DBG_CMD, (L"Call NdisMSetInformationComplete With FAILURE\n"));
    }

    // Clean up 
    ReturnCmdNode (Adapter,Adapter->CurCmd);
    Adapter->CurCmd = NULL;

    //// ExecuteNextCommand(Adapter);
    GetCmdFromQueueToExecute (Adapter);
    return;
}
/******************************************************************************
 *
 *  Name: MrvDrvSdioCheckFWReadyTimer()
 *
 *  Description: Check if the fw is ready
 *
 *
 *  Arguments:           
 *      IN PVOID SystemSpecific1
 *      IN PVOID FunctionContext
 *      IN PVOID SystemSpecific2
 *      IN PVOID SystemSpecific3
 *    
 *  Return Value: None
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID MrvDrvSdioCheckFWReadyTimerFunction(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    // This timer is used only in SDIO mode. When NDIS calls us
    // to transmit a packet and at point of time, if the FW is not ready,
    // we return insufficient resource error to NIDS and start this timer.
    // When this timer expires (this fn called), it checks whether the
    // FW is ready. If yes, it indicates the NDIS to resend the packet.
    // If FW is not yet ready, it restarts the timer.
    //
    // In case of CF, the equivalent timer fn is MrvDrvCheckTxReadyTimerFunction()

    // get the adpater context
    PMRVDRV_ADAPTER         Adapter = (PMRVDRV_ADAPTER) FunctionContext;
    UCHAR                   ucCardStatus;
    SD_API_STATUS           status;          // intermediate status

    Adapter->MrvDrvSdioCheckFWReadyTimerIsSet = FALSE;

    // check if FW is ready for download
    status = If_ReadRegister(Adapter,
                                            //SD_IO_READ ,
                                            1, // function 1
                                            HCR_HOST_CARD_STATUS_REGISTER, // reg 0x20
                                            FALSE,
                                            &ucCardStatus,
                                            sizeof(ucCardStatus));

    if (!SD_API_SUCCESS(status))
    {
        DBGPRINT(DBG_TMR,
            (L"SDIO: reading CCR_CARD_STATUS_REGISTER failed, reschedule timer\n"));
        // set up the timer again
        //Adapter->EagleSdioCheckFWReadyTimerIsSet = TRUE;
        //NdisMSetTimer(&Adapter->EagleSdioCheckFWReadyTimer,
        //    SDIO_FW_NOT_READY_WAIT_TIME);

        return;
    }

    if ( ucCardStatus & SDIO_IO_READY )
    {

        if ( ucCardStatus & SDIO_DOWNLOAD_CARD_READY )
        {
            // the firmware is ready for new download
            DBGPRINT(DBG_TMR,
                (L"SDIO: device is ready for new packet, indicate to NDIS\n"));
            NdisMSendResourcesAvailable(Adapter->MrvDrvAdapterHdl);
            return;
        }

        // HW is ready but FW is not, perhaps missed an interrupt
        DBGPRINT(DBG_TMR,
            (L"SDIO: SDIO_DOWNLOAD_CARD_READY not set, re-schedule timer.\n"));
        
        // send down last 32 bytes, then start timer
        {
            SD_API_STATUS           status;          // intermediate status
            SD_TRANSFER_CLASS       transferClass;   // general transfer class
            DWORD                   argument;        // argument
            ULONG                   numBlocks;       // number of blocks
            SD_COMMAND_RESPONSE     response;        // IO response status

            numBlocks = 1;

            // write, block mode, address starts at 0, fixed address
            argument =  BUILD_IO_RW_EXTENDED_ARG(SD_IO_OP_WRITE, 
                                                 SD_IO_BLOCK_MODE, 
                                                 1, // function number is 1 
                                                 SDIO_IO_PORT , 
                                                 SD_IO_FIXED_ADDRESS, 
                                                 numBlocks);
            transferClass = SD_WRITE;

            status = SDSynchronousBusRequest(Adapter->hDevice, 
                                             SD_CMD_IO_RW_EXTENDED,
                                             argument,
                                             transferClass, 
                                             ResponseR5,
                                             &response, 
                                             numBlocks,
                                             SDIO_EXTENDED_IO_BLOCK_SIZE,
                                             // send down the last packet, but only the first 32 bytes
                                             (PUCHAR)Adapter->LastFWBuffer, 
                                             0); 
        }

        // set up the timer again
        Adapter->MrvDrvSdioCheckFWReadyTimerIsSet = TRUE;
        NdisMSetTimer(&Adapter->MrvDrvSdioCheckFWReadyTimer,
            SDIO_FW_NOT_READY_WAIT_TIME);

        return;

    }

    DBGPRINT(DBG_TMR,(L"SDIO: SDIO_IO_READY not set, re-schedule timer.\n"));
    // set up the timer again
    Adapter->MrvDrvSdioCheckFWReadyTimerIsSet = TRUE;
    NdisMSetTimer(&Adapter->MrvDrvSdioCheckFWReadyTimer,
        SDIO_FW_NOT_READY_WAIT_TIME);

    return;
}



/******************************************************************************
 *
 *  Name: SetStationPowerState()
 *
 *  Description: Set MrvDrv power state
 *
 *  Conditions for Use: Called by power state set OID handler
 *
 *  Arguments:           
 *      PMRVDRV_ADAPTER Adapter
 *      NDIS_DEVICE_POWER_STATE NewPowerState
 *      IN NDIS_OID Oid
 *      IN PULONG BytesRead
 *      IN PULONG BytesNeeded
 *      IN PVOID InformationBuffer
 *    
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS
SetStationPowerState(
    PMRVDRV_ADAPTER Adapter,
    NDIS_DEVICE_POWER_STATE NewPowerState,
    IN NDIS_OID Oid,
    IN PULONG BytesRead,
    IN PULONG BytesNeeded,
    IN PVOID InformationBuffer
)
{
    
    BOOLEAN     bFwReloaded = FALSE;
    
    //IF_FW_STATUS  SdioFWStatus;
    
    if( Adapter->CurPowerState == NewPowerState )
        return NDIS_STATUS_SUCCESS;

    
    switch( Adapter->CurPowerState )
    {
      case NdisDeviceStateD0:  // D0->D3
           DBGPRINT(DBG_OID|DBG_PS|DBG_WARNING,(L"D0->D3\r\n"));  
           HandleD0toD3(Adapter);
           break;
      case NdisDeviceStateD1:
      case NdisDeviceStateD2:
           break;
        
      case NdisDeviceStateD3: // D3->D0
           if(NewPowerState != NdisDeviceStateD0)
           {
              DBGPRINT(DBG_OID|DBG_PS|DBG_ERROR,(L"Un-Supported Power State\r\n"));
              break; 
           }
           HandleD3toD0(Adapter);
           break;
        default :
            DBGPRINT(DBG_ERROR,(L"ERROR: Unknown power state\r\n"));
            break;
    }
    
    Adapter->CurPowerState = NewPowerState;

    return NDIS_STATUS_SUCCESS;
}



/******************************************************************************
 *
 *  Name: CheckCurrentSationStatus()
 *
 *  Description: Check the current station operation status
 *
 *  Conditions for Use: called by the OID handler to get current station status
 *
 *  Arguments:           
 *      PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_ADAPTER_NOT_READY
 *      NDIS_STATUS_RESET_IN_PROGRESS
 *      NDIS_STATUS_MEDIA_DISCONNECT
 *
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS
CheckCurrentSationStatus(
    IN PMRVDRV_ADAPTER Adapter
)
{
    //
    //      If the station is not available, return all NDIS packet with NDIS_STATUS_####,
    //      depends on the following situation:
    //      (1) If the hardware status is ready
    //      (2) If the media is connected
    //      (3) If the link is established
    //

    //      First check HW status
    switch( Adapter->HardwareStatus ){
    case NdisHardwareStatusReset:
        return NDIS_STATUS_RESET_IN_PROGRESS;
    case NdisHardwareStatusInitializing:
    case NdisHardwareStatusNotReady:
        return NDIS_STATUS_ADAPTER_NOT_READY;
    default:
        break;
    }

    //      Check Power management state
    if( Adapter->CurPowerState != NdisDeviceStateD0 )
        return NDIS_STATUS_ADAPTER_NOT_READY;

    //      Check media connected status
    if( Adapter->MediaConnectStatus == NdisMediaStateDisconnected ){
        return NDIS_STATUS_MEDIA_DISCONNECT;
    }

    //      Check link
    if( Adapter->LinkSpeed == MRVDRV_LINK_SPEED_0mbps ){
        return NDIS_STATUS_MEDIA_DISCONNECT;
    }

    //      Check device removal status
    if( Adapter->SurpriseRemoved == TRUE ){
        return NDIS_STATUS_ADAPTER_REMOVED;
    }

    //return Status;
    return NDIS_STATUS_SUCCESS;
}

/*
===============================================================================
                           CODED PRIVATE PROCEDURES
===============================================================================
*/

VOID
ResetDisconnectStatus(
    IN PMRVDRV_ADAPTER Adapter
)
{
    DBGPRINT(DBG_CMD, (L"Connection reset!\n"));
    // TODO: remove this if FW supports PS when not associated!!

    if ( Adapter->bBgScanEnabled==TRUE)
    {    
        EnableBgScan( Adapter, FALSE);
    }

   if (Adapter->CurrentSSID.SsidLength != 0)
   {
     NdisMoveMemory(&Adapter->PreviousSSID, &Adapter->CurrentSSID,
                    sizeof(NDIS_802_11_SSID));
   }
   DBGPRINT(DBG_LOAD, (L"set Disconnected!(5)\n"));
    Adapter->MediaConnectStatus = NdisMediaStateDisconnected;
    if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
        wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_NOT_CONNECT);
    }


    Adapter->ulCurrentBSSIDIndex =0;
    Adapter->LinkSpeed = MRVDRV_LINK_SPEED_1mbps; //do not set to 0mbps
    //032007 FIX adhoc indication
    Adapter->bIsMoreThanOneStaInAdHocBSS = FALSE;


    

       Adapter->ulLastMICErrorTime = 0;
//35.p6++      
    if ((Adapter->bIsReconnectEnable == FALSE) && 
        (Adapter->bIsBeaconLoseEvent == FALSE) &&
        (Adapter->bIsDeauthenticationEvent == FALSE)
        )
    {

        DBGPRINT(DBG_V9|DBG_HELP, (L"** IndicateStatus disconnect \n"));

          
        Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_DISCONNECT); 

        //dralee_20060712
        ResetAllScanTypeAndPower(Adapter);

                
        if ( Adapter->EncryptionStatus == Ndis802_11Encryption2Enabled )    // wpa
        {
            // set to key absent
            Adapter->EncryptionStatus = Ndis802_11Encryption2KeyAbsent;
        }
        else if ( Adapter->EncryptionStatus == Ndis802_11Encryption3Enabled ) // wpa2
        {
            // set to key absent
            Adapter->EncryptionStatus = Ndis802_11Encryption3KeyAbsent;
        }
    
    }
    return;
}   


VOID
ResetSingleTxDoneAck(
    IN PMRVDRV_ADAPTER Adapter
)
{
                
}   
NDIS_STATUS
SetMacControl(
    IN PMRVDRV_ADAPTER Adapter 
)
{
    NDIS_STATUS Status;

       
    DBGPRINT(DBG_V9 ,(L"+Set HostCmd_CMD_MAC_CONTROL 0x%x \n", Adapter->CurrentMacControl));
    //      Send MAc control command ro station
    Status=PrepareAndSendCommand(
        Adapter, 
        HostCmd_CMD_MAC_CONTROL, 
        Adapter->CurrentMacControl, 
        HostCmd_OPTION_USE_INT,
        (NDIS_OID)0,
        HostCmd_PENDING_ON_NONE, 
        0,
        FALSE,
        NULL,
        NULL,
        NULL,
        NULL);

    return Status;
}
#ifdef MRVL_PRINT_DBG_MSG  
void MrvPrintFile(const unsigned short *fmt, ...)
{
    va_list     argP; 
    FILE        *MrvDbgfp;
    //DWORD       dwThreadID;
    //DWORD       dwTick; 

    if((MrvDbgfp = fopen("\\My Documents\\DbgMsg.txt","a+")) == NULL){
		
		ERRORMSG(1,(TEXT("Failed to open \\My Documents\\DbgMsg.txt\r\n")));
        return;
    	}
    //dwThreadID = GetCurrentThreadId();
    //dwTick = GetTickCount();

    //fprintf(MrvDbgfp, "%8x:%d:", dwThreadID, dwTick);

    va_start(argP, fmt);
    vfwprintf(MrvDbgfp, fmt, argP);
    fflush(MrvDbgfp);
    va_end(argP);
    fclose(MrvDbgfp);  
}
#endif // #ifdef MRVL_PRINT_DBG_MSG

#ifdef DBG_MSG_TO_RETAILMSG 

#define MSGHEADER	L"[MARVELL-WIFI]:"

void MrvRETAILMSG(const wchar_t *fmt, ...)
{
  	
    wchar_t dbgbuf[1024] = {MSGHEADER};
    wchar_t  *msgpt = &dbgbuf[wcslen(MSGHEADER)];
    wchar_t* buffer;
	
    va_list ap;
    va_start(ap, fmt);
    buffer = msgpt;
    _vsnwprintf(buffer, sizeof(dbgbuf), fmt, ap);
    va_end(ap);
	
    NKDbgPrintfW (dbgbuf);
	
}
#endif

/******************************************************************************
 *
 *  Name: ConvertNDISRateToFWIndex()
 *
 *  Description: Look up the FW index for the rate
 *
 *  Arguments:  NDISRate    Rate in NDIS format
 *    
 *  Return Value: Equivalent index in FW, return MRVDRV_NUM_SUPPORTED_RATES 
 *                if no matching rate is found
 * 
 *  Notes:
 *
 *****************************************************************************/
UCHAR   ConvertNDISRateToFWIndex(UCHAR  NDISRate)
{
    UCHAR i;

    for ( i=0; i < MRVDRV_NUM_SUPPORTED_RATES; i++ )
    {
        if ( NDISRate == MrvDrvSupportedRates[i] )
        {
            return i;
        }
    }

    return MRVDRV_NUM_SUPPORTED_RATES;
}

/******************************************************************************
 *
 *  Name: ConvertFWIndexToNDISRate()
 *
 *  Description: Look up the rate for FW index
 *
 *  Arguments:  FWIndex
 *    
 *  Return Value: Equivalent NDIS Rate, return 0 
 *                if no matching rate is found
 * 
 *  Notes:
 *
 *****************************************************************************/
UCHAR   ConvertFWIndexToNDISRate(UCHAR  FWIndex)
{
    if ( FWIndex >= MRVDRV_NUM_SUPPORTED_RATES )
    {
        return 0;
    }

    return MrvDrvSupportedRates[FWIndex];
}

//lykao, 053005, for Hidden SSID, from Plus            
UCHAR FindSSIDInList(
      IN PMRVDRV_ADAPTER Adapter,
      IN PNDIS_802_11_SSID pSSID)
{
    UCHAR i, index = 0xFF;                   

    for (i=0; i<Adapter->ulNumOfBSSIDs; i++)
    {
        if(Adapter->BSSIDList[i].Ssid.SsidLength == pSSID->SsidLength)
        {

            if(NdisEqualMemory(Adapter->BSSIDList[i].Ssid.Ssid, 
                       pSSID->Ssid, pSSID->SsidLength))
                             index = i;
                }
         }
         
         return index;
                                                    
}   

// tt wled
UCHAR FindSSIDInPSList(
      IN PMRVDRV_ADAPTER Adapter,
      IN PNDIS_802_11_SSID pSSID)
{
    UCHAR i, index = 0xFF;                   

    for (i=0; i<Adapter->ulPSNumOfBSSIDs; i++)
    {
        if(Adapter->PSBSSIDList[i].Ssid.SsidLength == pSSID->SsidLength)
        {

            if(NdisEqualMemory(Adapter->PSBSSIDList[i].Ssid.Ssid, 
                       pSSID->Ssid, pSSID->SsidLength))
                             index = i;
                }
         }
         
         return index;
                                                    
}   

/******************************************************************************
 *
 *  Name: ascii2hex()
 *
 *  Description: convert acssic hex string to hex value
 *
 *  Arguments:  s-->input string  d-->output value array  dlen-->input length
 *    
 *  Return Value: 
 * 
 *  Notes: for ADHOCAES key convert to sync. with linux driver
 *
 *****************************************************************************/
int ascii2hex(UCHAR *d, char *s, int dlen)
{
    int i;
    UCHAR n;

    NdisZeroMemory(d, dlen);
    for (i = 0; i < dlen * 2; i++) 
    {
        /*
        if ((s[i] >= 48) && (s[i] <= 57))
            n = s[i] - 48;
        else if ((s[i] >= 65) && (s[i] <= 70))
            n = s[i] - 55;
        else if ((s[i] >= 97) && (s[i] <= 102))
            n = s[i] - 87;
        else
            break;
            */   
        n=s[i];
        if ((i % 2) == 0)
            n = n * 16;
        d[i / 2] += n;
    }

    return i;
}

// The function just filter and reorganize WPA IE now and will skip WMM IE
VOID
RegularIeFilter(UCHAR *IeBuf, USHORT *pBufLen)
{
    UCHAR   IeId;
    UCHAR   IeLen,TmpIeLen;
    USHORT  CurPos =0;
    UCHAR   TmpBuf[500];
    USHORT  TmpPos = 0, BytesLeft; // plusIEfix
    BOOLEAN IsWpaFound = FALSE;
    UCHAR   WPA_OUI[] = { 0x00, 0x50, 0xF2, 0x01};
    UCHAR   WMM_OUI[] = { 0x00, 0x50, 0xF2, 0x02};
    UCHAR   WPS_OUI[] = { 0x00, 0x50, 0xF2, 0x04 };
    UCHAR   CCX_OUI[] = { 0x00, 0x40, 0x96};

    NdisMoveMemory( TmpBuf, IeBuf ,*pBufLen);
    BytesLeft = *pBufLen;  // plusIEfix

    //DBGPRINT(DBG_ALLEN, ("RegularIeFilter++  *pBufLen = %d\n", *pBufLen));

    //HexDump(DBG_ALLEN, "IeBuf ", IeBuf, *pBufLen);
    
    while (TmpPos < *pBufLen)
    {
        IeId  = *(TmpBuf + TmpPos);
        TmpIeLen = *(TmpBuf + TmpPos + 1);
        IeLen = TmpIeLen;
        
//        DBGPRINT(DBG_ERROR, ("IE length = %d ID is %d  ByteLeft %d \n\r", IeLen, IeId, BytesLeft));
        
// plusIEfix ++        
        if(BytesLeft < (IeLen + 2))
        {
            DBGPRINT(DBG_ERROR, (L"Abnormal IE length = %d ID is %d  ByteLeft %d \n\r", IeLen, IeId, BytesLeft));
        	  break;
        }	
        BytesLeft -= (IeLen + 2);
// plusIEfix --
            
        switch (IeId)
        {
        case 0xdd:  // WPA IE or WMM IE
       {
        if (NdisEqualMemory((TmpBuf + TmpPos + 2), WPA_OUI, 4) == 1) //WPA IE
        {
                     if (!IsWpaFound)
                     {
                  NdisMoveMemory((IeBuf + CurPos), (TmpBuf + TmpPos), TmpIeLen + 2);
                  //HexDump(DBG_WPA, "WPA IE ", (IeBuf + CurPos), TmpIeLen + 2);    
              CurPos += (TmpIeLen + 2);
                  IsWpaFound = TRUE;
             }    
                }
                    else if ( NdisEqualMemory( (TmpBuf + TmpPos + 2), CCX_OUI, 3 ) == 1 )
                    {
                            NdisMoveMemory((IeBuf + CurPos), (TmpBuf + TmpPos), TmpIeLen + 2);
                                CurPos += (TmpIeLen + 2);
                    }

                 else if(NdisEqualMemory((TmpBuf + TmpPos + 2), WMM_OUI, 4) == 1) // WMM IE 
                {
             NdisMoveMemory((IeBuf + CurPos), (TmpBuf + TmpPos), TmpIeLen + 2);
//               HexDump(DBG_WMM, "WMM IE ", (IeBuf + CurPos), TmpIeLen + 2);    
                 CurPos += (TmpIeLen + 2);
                }
                 else if(NdisEqualMemory((TmpBuf + TmpPos + 2), WPS_OUI, 4) == 1) // WPS(WSC) IE 
                {
                     NdisMoveMemory((IeBuf + CurPos), (TmpBuf + TmpPos), TmpIeLen + 2);
                     CurPos += (TmpIeLen + 2);
                }
           }
           break;
    
       default:

        NdisMoveMemory((IeBuf + CurPos), (TmpBuf + TmpPos), TmpIeLen + 2);
        //HexDump(DBG_ALLEN, "ELSE IE ", (IeBuf + CurPos), TmpIeLen + 2);
        CurPos += (TmpIeLen + 2);
        
       break;
    } // end of switch
        
        TmpPos += (TmpIeLen + 2);
    }//while    

    *pBufLen = CurPos;

    //HexDump(DBG_ALLEN, "IeBuf ", IeBuf, *pBufLen);
    
    //DBGPRINT(DBG_ALLEN, ("RegularIeFilter-- *pBufLen = %d\n", *pBufLen));
    return; 
}

UCHAR Wpa2RsnIeAdjust(UCHAR *Dest, UCHAR *Src, UCHAR RsnIeLen, UCHAR Mode)
{
     UCHAR length, DestOff, SrcOff;  
     UCHAR Tkip_Oui[] = {0x00, 0xF, 0xAC, 0x2}; 
     UCHAR Aes_Oui[] =  {0x00, 0xF, 0xAC, 0x4};

     if ( RsnIeLen == 0 )
     {
        DBGPRINT(DBG_ERROR,(L"Warning: RSN IE length=%d, skip IE adjustment!\r\n", RsnIeLen));
        return 0;
     }
     
     length = RsnIeLen;
     DestOff = SrcOff = 0;
     NdisMoveMemory(Dest, Src, 8);    //copy IE header and group key suite
     DestOff += 8;
     SrcOff += 8;
     if(Src[SrcOff] == 2)  // two pairwise key AES+TKIP
     {
        Dest[DestOff++] = 1;
        Dest[DestOff++] = 0;
       
        if(Mode == RSN_IE_AES)
           NdisMoveMemory(&Dest[DestOff], &Aes_Oui[0], 4);  //copy AES pairwise key suite
        else
            NdisMoveMemory(&Dest[DestOff], &Tkip_Oui[0], 4); //copy TKIP pairwise keu suite
        
        DestOff += 4;
        SrcOff += 10; 
        length -= 4;
     Dest[1] -= 4;
     }
     else
     { 
        NdisMoveMemory(&Dest[DestOff], &Src[SrcOff], 6);    //copy pairwise key suite
        DestOff += 6;
        SrcOff += 6;
     }       
      
     if (Src[SrcOff] == 2)   //two mgmt suite
     {
        Dest[DestOff++] = 1;
        Dest[DestOff++] = 0;
     NdisMoveMemory(&Dest[DestOff], Tkip_Oui, 4); //always copy AKMP suite OUI as None
        DestOff += 4;
        SrcOff += 10; 
        length -= 4;    
    Dest[1] -= 4;   
     }
      else
     {
        NdisMoveMemory(&Dest[DestOff], &Src[SrcOff], 6); //copy mgmt suite
        DestOff += 6;
        SrcOff += 6;
}   
     NdisMoveMemory(&Dest[DestOff], &Src[SrcOff], (RsnIeLen - SrcOff));    //copy pairwise key suite

     return length;     
}
//Junius Added 20071017
UCHAR WpaIeAdjust(UCHAR *Dest, UCHAR *Src, UCHAR RsnIeLen, UCHAR Mode)
{
     UCHAR length, DestOff, SrcOff;  
     UCHAR Tkip_Oui[] = {0x00, 0x50, 0xF2, 0x2}; 
     UCHAR Aes_Oui[] =  {0x00, 0x50, 0xF2, 0x4};
     if ( RsnIeLen == 0 )
     {
        DBGPRINT(DBG_ERROR,(L"Warning: RSN IE length=%d, skip IE adjustment!\n\r", RsnIeLen));
        return 0;
     }
     
     length = RsnIeLen;
     DestOff = SrcOff = 0;
     NdisMoveMemory(Dest, Src, 12);    //copy IE header and group key suite
     DestOff += 12;
     SrcOff += 12;
     if(Src[SrcOff] == 2)  // two pairwise key AES+TKIP
     {
        DBGPRINT(DBG_ERROR,(L"test 1 \n"));
        Dest[DestOff++] = 1;
        Dest[DestOff++] = 0;
       
        if(Mode == RSN_IE_AES)
           NdisMoveMemory(&Dest[DestOff], &Aes_Oui[0], 4);  //copy AES pairwise key suite
        else
            NdisMoveMemory(&Dest[DestOff], &Tkip_Oui[0], 4); //copy TKIP pairwise keu suite
        
        DestOff += 4;
        SrcOff += 10; 
        length -= 4;
     Dest[1] -= 4;
     }
     else
     { 
        NdisMoveMemory(&Dest[DestOff], &Src[SrcOff], 6);    //copy pairwise key suite
        DestOff += 6;
        SrcOff += 6;
     }       
      
     if (Src[SrcOff] == 2)   //two mgmt suite
     {
        Dest[DestOff++] = 1;
        Dest[DestOff++] = 0;
     NdisMoveMemory(&Dest[DestOff], Tkip_Oui, 4); //always copy AKMP suite OUI as None
        DestOff += 4;
        SrcOff += 10; 
        length -= 4;    
        Dest[1] -= 4;   
     }
      else
     {
        NdisMoveMemory(&Dest[DestOff], &Src[SrcOff], 6); //copy mgmt suite
        DestOff += 6;
        SrcOff += 6;
	}   
    	
     NdisMoveMemory(&Dest[DestOff], &Src[SrcOff], (RsnIeLen - SrcOff));    //copy pairwise key suite
    
     return length;     
}
//end added
///Reconnect_DiffSetting ++
int FindAPByBssid(PMRVDRV_ADAPTER pAdapter, UCHAR*  Bssid)
{
	int i;

	for (i=0 ; i<(int)pAdapter->ulPSNumOfBSSIDs ; i++) {
		if (NdisEqualMemory(pAdapter->PSBSSIDList[i].MacAddress, Bssid, MRVDRV_ETH_ADDR_LEN)) {
			///this is the AP we are connected.
			break;
		}
	}
    if (i == pAdapter->ulPSNumOfBSSIDs) {
        return -1;
    } else {
        return i;
    }
}
///Reconnect_DiffSetting --


VOID
InfraBssReconnectStart(
        IN PMRVDRV_ADAPTER Adapter,
        IN USHORT reason )
{
    ///Reconnect_DiffSetting ++
    int     AssocAPId = FindAPByBssid(Adapter, Adapter->CurrentBSSID);
    ///Reconnect_DiffSetting --
    DBGPRINT(DBG_ASSO|DBG_HELP, (L"[Marvell]InfraBssReconnectStart() , reason = 0x%x, Adapter->EncryptionStatus =%d \n", reason,Adapter->EncryptionStatus));

    Adapter->ReInfrastructureMode = Adapter->InfrastructureMode; 
    Adapter->ReAuthenticationMode = Adapter->AuthenticationMode;
    Adapter->ReEncryptionStatus = Adapter->EncryptionStatus;
    ///Reconnect_DiffSetting ++
    if (AssocAPId == -1) {
        NdisZeroMemory(&Adapter->ReIEBuffer, sizeof(MRV_BSSID_IE_LIST));
    } else {
        NdisMoveMemory(&Adapter->ReIEBuffer, &Adapter->PSIEBuffer[AssocAPId], sizeof(MRV_BSSID_IE_LIST));
    }
    ///Reconnect_DiffSetting --
    NdisMoveMemory(  &(Adapter->ReWEPKey), 
                    &(Adapter->LastAddedWEPKey),
                    sizeof(MRVL_WEP_KEY));
    NdisMoveMemory(  &(Adapter->ReSSID),
                                &(Adapter->PreviousSSID), 
                    sizeof(NDIS_802_11_SSID));
    
       
        switch (reason)
    {
        case RECONNECT_ASSOCIATE_FAIL:
            
            Adapter->bIsReconnectEnable = TRUE;
            Adapter->usReconnectCounter = 1;
            Adapter->ulReconnectPeriod = 0;
            Adapter->ulStartTimeStamp = 0;
            Adapter->ReconnectType = RECONNECT_COUNTER_TYPE;
            Adapter->bIsAcceptSystemConnect = FALSE;
            break;
            
        case RECONNECT_LINK_LOST:

            Adapter->bIsReconnectEnable = TRUE;
            Adapter->usReconnectCounter = 1;
            Adapter->ulReconnectPeriod = 0; 
            Adapter->ulStartTimeStamp = 0;      
            Adapter->ReconnectType =RECONNECT_COUNTER_TYPE;         
            Adapter->bIsAcceptSystemConnect = TRUE;
            
            break;
            
        case RECONNECT_DEAUTHENTICATE:

             Adapter->bIsReconnectEnable = TRUE;
            Adapter->usReconnectCounter = 1;  //default=1, //Ling++, test
            Adapter->ulReconnectPeriod = 0;  
            Adapter->ulStartTimeStamp = 0;
            Adapter->ReconnectType = RECONNECT_COUNTER_TYPE;
            Adapter->bIsAcceptSystemConnect = FALSE;
            break;
            
/*
    ///Move to roaming implementation
    case RECONNECT_ROAMING:

            Adapter->bIsReconnectEnable = TRUE;
            Adapter->usReconnectCounter = 0;
            Adapter->ulReconnectPeriod = 0;
            Adapter->ulStartTimeStamp = 0;
            Adapter->ReconnectType = RECONNECT_RSSI_TYPE;
            Adapter->bIsAcceptSystemConnect = FALSE;
            break;
*/            
        case RECONNECT_D3_TO_D0:
            
            Adapter->bIsReconnectEnable = TRUE;
            Adapter->usReconnectCounter = 1;
            Adapter->ulReconnectPeriod = 0;
            Adapter->ulStartTimeStamp = 0;
            Adapter->ReconnectType = RECONNECT_COUNTER_TYPE;
            break;
            
        case RECONNECT_DEEP_SLEEP_AWAKE:
            
            Adapter->bIsReconnectEnable = TRUE;
            Adapter->usReconnectCounter = 1;
            Adapter->ulReconnectPeriod = 0;
            Adapter->ulStartTimeStamp = 0;
            Adapter->ReconnectType = RECONNECT_COUNTER_TYPE;
            Adapter->bIsAcceptSystemConnect = FALSE;
            break;

        case RECONNECT_HIDE_SSID:
            
            Adapter->bIsReconnectEnable = TRUE;
            Adapter->usReconnectCounter = 10;
            Adapter->ulReconnectPeriod = 0;
            Adapter->ulStartTimeStamp = 0;
            Adapter->ReconnectType = RECONNECT_COUNTER_TYPE;
            Adapter->bIsAcceptSystemConnect = FALSE;
            break;
            
        }

       switch (Adapter->ReconnectType)
       {
        case RECONNECT_COUNTER_TYPE:

            ReConnectHandler( Adapter);
            // Start Reconnect timer
                    NdisMSetPeriodicTimer( &Adapter->MrvReConnectTimer,  RE_CONNECT_PERIOD_TIME);
                    Adapter->ReConnectTimerIsSet = TRUE;
            break;

        case RECONNECT_PERIOD_TYPE:
            
            DBGPRINT(DBG_ALLEN, (L"RECONNECT_PERIOD_TYPE Set timer \n"));

            // Start Reconnect timer
            NdisMSetPeriodicTimer( &Adapter->MrvReConnectTimer,  RE_CONNECT_PERIOD_TIME);
            Adapter->ReConnectTimerIsSet = TRUE;
            break;

        default :
            break;
       }    
    return; 
}

VOID
InfraBssReconnectStop(
        PMRVDRV_ADAPTER Adapter)
{
    BOOLEAN  timerStatus;
    
    DBGPRINT(DBG_RECONNECT|DBG_HELP, (L"InfraBssReconnectStop()  \n"));
    
    Adapter->bIsReconnectEnable = FALSE;
       Adapter->bIsReConnectNow = FALSE;
    
    if (Adapter->ReConnectTimerIsSet == TRUE)
    {
        NdisMCancelTimer(&Adapter->MrvReConnectTimer, &timerStatus);
        Adapter->ReConnectTimerIsSet = FALSE;
    }
    
  
    if (Adapter->MediaConnectStatus == NdisMediaStateDisconnected)
    {
        Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_DISCONNECT); 
  
          //dralee_20060712
          ResetAllScanTypeAndPower(Adapter);
         
    }
     Adapter->bIsBeaconLoseEvent = FALSE;
        Adapter->bIsDeauthenticationEvent = FALSE;

    return;
}   

VOID
DoReConnect(
        PMRVDRV_ADAPTER Adapter)
{
    NDIS_STATUS Status;

    DBGPRINT(DBG_RECONNECT, (L"DoReConnect()  \n"));

       if ( Adapter->bIsReConnectNow == TRUE)
       {
       DBGPRINT(DBG_RECONNECT, (L"Adapter->bIsReConnectNow = TRUE  \n"));   
        return;
       }
        Adapter->bIsReConnectNow = TRUE;

    // set infrastructure mode
    DBGPRINT(DBG_RECONNECT, (L"InfrastructureMode = 0x%x  \n", Adapter->ReInfrastructureMode));
    PrepareAndSendCommand(
                 Adapter,
                 HostCmd_CMD_802_11_SNMP_MIB,
                 (USHORT)Adapter->ReInfrastructureMode,
                 HostCmd_OPTION_USE_INT,
                 (NDIS_OID)OID_802_11_INFRASTRUCTURE_MODE,
                         HostCmd_PENDING_ON_NONE,
                 0,
                 FALSE,
                 NULL,
                 NULL,
                 NULL,
                 NULL);
       
    // set authentication mode 
    DBGPRINT(DBG_RECONNECT, (L"AuthenticationMode = 0x%x  \n", Adapter->ReAuthenticationMode));
    Adapter->AuthenticationMode = Adapter->ReAuthenticationMode;

    // set encryption mode 
    DBGPRINT(DBG_RECONNECT, (L"EncryptionStatus = 0x%x  \n", Adapter->ReEncryptionStatus));
    Status = MrvDrvSetEncryptionStatus( 
                 Adapter, 
                 Adapter->ReEncryptionStatus,
                        (NDIS_OID) 0,                     
                 &(Adapter->ReEncryptionStatus),   
                 NULL,      
                 NULL);         
                
       // Add Key : now just support wep , no WPA
    if ((Adapter->ReEncryptionStatus == Ndis802_11Encryption1Enabled) ||
        (Adapter->ReEncryptionStatus == Ndis802_11Encryption1KeyAbsent))
    {
        Adapter->NeedSetWepKey = TRUE;        
        Adapter->EncryptionStatus = Ndis802_11Encryption1Enabled;
    }
    
    // set ssid
    NdisMoveMemory(  &(Adapter->ActiveScanSSID), 
                        &(Adapter->ReSSID), 
                    sizeof(NDIS_802_11_SSID)); 
    

    if (( ! Adapter->bIsScanInProgress ) && ( ! Adapter->bIsAssociateInProgress ))
    {  
        DBGPRINT(DBG_RECONNECT, (L"Send HostCmd_CMD_802_11_SCAN  then Association\n"));  
        
        Adapter->bIsAssociationBlockedByScan = TRUE;
        Adapter->SetActiveScanSSID = TRUE;
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

    }
       else
    {
        Adapter->bIsReConnectNow = FALSE;
       }   
}

/// called by 
VOID
ReConnectHandler(
        PMRVDRV_ADAPTER Adapter)
{

    if (Adapter->bIsReconnectEnable == FALSE )
    {
        return;
    }

    if (Adapter->bIsAssociateInProgress ||Adapter->bIsScanInProgress ||Adapter->bIsSystemConnectNow)
    {     
        DBGPRINT(DBG_RECONNECT, (L"Adapter->bIsSystemConnectNow = TRUE\n"));
        return;   
    }

    DBGPRINT(DBG_RECONNECT, (L"ReConnectHandler()  ,ReconnectType = 0x%x\n", Adapter->ReconnectType));
    
    switch (Adapter->ReconnectType)
    {
        case RECONNECT_COUNTER_TYPE:

          DBGPRINT(DBG_RECONNECT, (L"usReconnectCounter = 0x%x  \n", Adapter->usReconnectCounter));

            if ((Adapter->usReconnectCounter >= 1) && 
                 (Adapter->MediaConnectStatus == NdisMediaStateDisconnected))   
            {
                Adapter->usReconnectCounter --;

                DoReConnect (Adapter);
            }
            else
            {
                InfraBssReconnectStop(Adapter); 
            }
            break;
            
        case RECONNECT_PERIOD_TYPE:

            {   
                ULONG   ulCurrentSystemUpTime, ulDiff;

                    NdisGetSystemUpTime(&ulCurrentSystemUpTime);
                ulDiff = ulCurrentSystemUpTime - Adapter->ulStartTimeStamp ;

          DBGPRINT(DBG_RECONNECT, (L"ulStartTimeStamp = 0x%x  \n", Adapter->ulStartTimeStamp));
          DBGPRINT(DBG_RECONNECT, (L"ulCurrentSystemUpTime = 0x%x  \n", ulCurrentSystemUpTime));
          DBGPRINT(DBG_RECONNECT, (L"ulDiff = 0x%x  \n", ulDiff));

                if (ulDiff  < Adapter->ulReconnectPeriod) 
                {
                    DoReConnect (Adapter);
                }
                else
                {   
                    InfraBssReconnectStop(Adapter); 
                }
            }
            break;
        
        case RECONNECT_RSSI_TYPE:
            break;
    }
}





void ResetPmkidCache( IN PMRVDRV_ADAPTER Adapter )
{
    Adapter->NumOfPmkid = 0;
    NdisZeroMemory( Adapter->PmkidCache, sizeof(Adapter->PmkidCache) );
}

/*
    RETURN:
        0, failed
        1, successful
*/
int SavePmkidToCache( IN PMRVDRV_ADAPTER Adapter, IN PNDIS_802_11_PMKID pNewPmkid )
{
    ULONG               idx;
    PPMKID_CACHE    pPmkidCache;
    PBSSID_INFO     pIdInfo;

    if ( pNewPmkid->BSSIDInfoCount > MAX_PMKID_CACHE )
        return 0;

    pPmkidCache = Adapter->PmkidCache;
    pIdInfo = pNewPmkid->BSSIDInfo;
    
    for( idx=0; idx<pNewPmkid->BSSIDInfoCount; idx++ )
    {
        NdisMoveMemory( pPmkidCache[idx].bssid, pIdInfo[idx].BSSID, sizeof(pIdInfo[idx].BSSID) );
        NdisMoveMemory( pPmkidCache[idx].pmkid, pIdInfo[idx].PMKID, sizeof(pIdInfo[idx].PMKID) );
    }

    Adapter->NumOfPmkid = pNewPmkid->BSSIDInfoCount;

    return 1;
}

/*
    RETURN: 
        <0, could not find Pmkid in cache
        >=0, found one. the return value is the index of the pmkid.
*/
int FindPmkidInCache( IN PMRVDRV_ADAPTER Adapter, IN UCHAR *pBssid )
{
    ULONG               idx;
    PPMKID_CACHE    pPmkidCache;

    DBGPRINT( DBG_MACEVT, (L"    Try to find PMKID in cache ...\n") );
    DBGPRINT( DBG_MACEVT, (L"        Total number of PMKID in cache: %d\n", Adapter->NumOfPmkid) );
    DBGPRINT( DBG_MACEVT, (L"        Desired BSSID=...%x:%x:%x\n", pBssid[3], pBssid[4], pBssid[5]) );
    
    for( idx=0; idx<Adapter->NumOfPmkid; idx++ )
    {
        pPmkidCache = &(Adapter->PmkidCache[idx]);
        
        if ( NdisEqualMemory( pPmkidCache->bssid, pBssid, MRVDRV_ETH_ADDR_LEN ) )
            break;
    }

    if ( idx < Adapter->NumOfPmkid )
        return idx;
    else
        return -1;
}

void DbgDumpCurrentPmkidCache( IN PMRVDRV_ADAPTER Adapter )
{
    ULONG               idx, i;
    PPMKID_CACHE    pPmkidCache;

    pPmkidCache = Adapter->PmkidCache;
    
    DBGPRINT( DBG_MACEVT, (L"*** PMKID cache dump ***\n") );
    DBGPRINT( DBG_MACEVT, (L"   Total number of PMKIDs is %d\n", Adapter->NumOfPmkid) );

    for( idx=0; idx<Adapter->NumOfPmkid; idx++ )
    {
        DBGPRINT( DBG_MACEVT, (L"   %2x:%2x:%2x:%2x:%2x",
                pPmkidCache[idx].bssid[0], 
                pPmkidCache[idx].bssid[1], 
                pPmkidCache[idx].bssid[2], 
                pPmkidCache[idx].bssid[3],
                pPmkidCache[idx].bssid[4],
                pPmkidCache[idx].bssid[5] ) );

        DBGPRINT( DBG_MACEVT, (L"  [ ") );

        for ( i=0; i<LEN_OF_PMKID; i++ )
            DBGPRINT( DBG_MACEVT, (L"%2x ", pPmkidCache[idx].pmkid[i]) );

        DBGPRINT( DBG_MACEVT, (L"]\n") );
    }
}

void DbgDumpRSN( IN PMRVDRV_ADAPTER Adapter )
{
}

UINT PrepareRSNForOsNotify( IN PMRVDRV_ADAPTER Adapter, IN UCHAR *pDesiredSsid, IN UINT nSsidLen )
{
    UINT    nIdxBss, nIdxCand;
    PPMKID_CANDIDATE        pPmkidCand;
    PNDIS_WLAN_BSSID_EX pBssidEx;

// tt ++ patch pmk
    static NDIS_802_11_MAC_ADDRESS  s_PrevAP[3];
    static DWORD                        s_IdxAP=0, s_NumAP=0;
    DWORD               i;
// tt --

    DBGPRINT( DBG_MACEVT, (L"+ PrepareRSNForOsNotify\n") );

    DBGPRINT( DBG_MACEVT, (L"   TotalNumOfBssid=%d, DesiredSsid=%c%c%c%c...\n",
        Adapter->ulPSNumOfBSSIDs,
        pDesiredSsid[0],
        pDesiredSsid[1], 
        pDesiredSsid[2], 
        pDesiredSsid[3] ) );

// tt ++ patch pmk
    DBGPRINT( DBG_MACEVT, (L"   Going to append previous %d APs\n", s_NumAP ) );
    nIdxCand = 0;
    for ( i=0; i<s_NumAP; i++ )
    {
        pPmkidCand = &(Adapter->RSNPmkidCandidateList.CandidateList[nIdxCand]);
        pPmkidCand->Flags = 0; // means PreAuth disabled.
        NdisMoveMemory( pPmkidCand->BSSID, s_PrevAP[i], sizeof(pPmkidCand->BSSID) );
        nIdxCand ++;
    }
    DBGPRINT( DBG_MACEVT, (L"   Added %d APs\n", nIdxCand ) );
// tt --

#if 0 // tt ++ patch pmk
    for( nIdxBss=0, nIdxCand=0; nIdxBss<Adapter->ulPSNumOfBSSIDs && nIdxCand<MAX_PMKID_CANDIDATE; nIdxBss++ )
#else
       for( nIdxBss=0; nIdxBss<Adapter->ulPSNumOfBSSIDs && nIdxCand<MAX_PMKID_CANDIDATE; nIdxBss++ )
#endif
    {
        pBssidEx = &(Adapter->PSBSSIDList[nIdxBss]);
/*
        DBGPRINT( DBG_MACEVT, ("   matching %c%c%c...\n", 
            pBssidEx->Ssid.Ssid[0],
            pBssidEx->Ssid.Ssid[1], 
            pBssidEx->Ssid.Ssid[2] ) );
*/  
        if ( nSsidLen == 0 || 
            (nSsidLen==pBssidEx->Ssid.SsidLength && NdisEqualMemory(pDesiredSsid, pBssidEx->Ssid.Ssid, nSsidLen)) )
        {
            pPmkidCand = &(Adapter->RSNPmkidCandidateList.CandidateList[nIdxCand]);
// tt ++ patch pmk
                     for ( i=0; i<s_NumAP; i++ )
                     {
                        if ( NdisEqualMemory( s_PrevAP[i], pBssidEx->MacAddress, 6 ) )
                            break;
                     }
                     if ( i < s_NumAP ) continue;
// tt --
            // TODO: value of the Flags should depend on PreAuth field of RSNCapability that comes from an AP.
            pPmkidCand->Flags = 0; // means PreAuth disabled.
            NdisMoveMemory( pPmkidCand->BSSID, pBssidEx->MacAddress, sizeof(pPmkidCand->BSSID) );

            nIdxCand ++;
// tt ++ patch pmk
                     NdisMoveMemory( s_PrevAP[s_IdxAP], pBssidEx->MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS) );
                     s_IdxAP ++;
                     if ( s_NumAP < 3 )    s_NumAP ++;
                     if ( s_IdxAP >= 3 )    s_IdxAP = 0;
// tt --
        }
    }

    DBGPRINT( DBG_MACEVT, (L"- PrepareRSNForOsNotify [NumOfCand=%d]\n", nIdxCand) );

    return nIdxCand;
}

/*
    This function can be called after we get pairwise and group key.
*/
void OsNotify_RSN( IN PMRVDRV_ADAPTER Adapter )
{
    UINT    nNumOfRSNItems;
    UINT    ulSize;
    
    DBGPRINT( DBG_MACEVT, (L"+ OsNotify_RSN\n") );

    DBGPRINT( DBG_MACEVT, (L"    NumOfBssid: normal=%d, progressive=%d\n", Adapter->ulNumOfBSSIDs, Adapter->ulPSNumOfBSSIDs) );

    // TODO: Need to check association state and whether we got group key or not.

    if ( Adapter->ulPSNumOfBSSIDs == 0 )
    {
        DBGPRINT( DBG_MACEVT, (L"- OsNotify_RSN [There is no BSSID record]\n") );
        return;
    }

    Adapter->RSNStatusIndicated.StatusType = Ndis802_11StatusType_PMKID_CandidateList;
    Adapter->RSNPmkidCandidateList.Version = 1;

    nNumOfRSNItems = PrepareRSNForOsNotify( Adapter, Adapter->CurrentSSID.Ssid, Adapter->CurrentSSID.SsidLength );

    if ( nNumOfRSNItems == 0 )
    {
        DBGPRINT( DBG_MACEVT, (L"- OsNotify_RSN [There is no RSN item to be set]\n") );
        return;
    }

    Adapter->RSNPmkidCandidateList.NumCandidates = nNumOfRSNItems;

    ulSize = sizeof(NDIS_802_11_STATUS_INDICATION) +
            sizeof(NDIS_802_11_PMKID_CANDIDATE_LIST) +
            (sizeof(PMKID_CANDIDATE)*(Adapter->RSNPmkidCandidateList.NumCandidates-1));

    DBGPRINT(DBG_MACEVT, (L"   NumCandidate=%d, TotalSize=%d\n", Adapter->RSNPmkidCandidateList.NumCandidates, ulSize) );
    DbgDumpRSN( Adapter );

    NdisMIndicateStatus( Adapter->MrvDrvAdapterHdl, 
                        NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                        &Adapter->RSNStatusIndicated,
                        ulSize );

    NdisMIndicateStatusComplete( Adapter->MrvDrvAdapterHdl );       

    DBGPRINT( DBG_MACEVT, (L"- OsNotify_RSN [Indicate completely]\n") );
}

   
//dralee_20060706
VOID MrvPrintEventLogToFile(const unsigned short *fmt, ...)
{
    va_list     argP;
    FILE        *MrvDbgfp;

   if( (MrvDbgfp = fopen("\\My Documents\\EvtLog.txt","a+")) == NULL )
        return;  

   va_start(argP, fmt);
   vfwprintf(MrvDbgfp, fmt, argP);
   fflush(MrvDbgfp);
   va_end(argP);
   fclose(MrvDbgfp);
}


// tt ++ re-assoc issue 060123
VOID DeleteBssidInPSList( PMRVDRV_ADAPTER pAdapter, UCHAR pBssid[] )
{
    UINT    nDelIdx, nLastIdx;
    PNDIS_WLAN_BSSID_EX pPsList;
    
    if ( !pBssid )
        return;

    DBGPRINT( DBG_MACEVT, (L"[Mrvl] Try to delete BSSID: %2x %2x %2x %2x %2x %2x\n", pBssid[0], pBssid[1], pBssid[2], pBssid[3], pBssid[4], pBssid[5] ) );

    pPsList = pAdapter->PSBSSIDList;

    // find the record
    for ( nDelIdx=0; nDelIdx<pAdapter->ulPSNumOfBSSIDs; nDelIdx++ )
    {
        if ( NdisEqualMemory( pBssid, pPsList[nDelIdx].MacAddress, ETH_ADDR_LENGTH ) )
            break;
    }

    if ( nDelIdx >= pAdapter->ulPSNumOfBSSIDs )
        return;

    DBGPRINT( DBG_MACEVT, (L"[Mrvl] Found the BSSID, Idx=%d, SSID=%c%c%c..., NumOfBssid=%d\n", 
        nDelIdx, 
        pAdapter->PSBSSIDList[nDelIdx].Ssid.Ssid[0], pAdapter->PSBSSIDList[nDelIdx].Ssid.Ssid[1], pAdapter->PSBSSIDList[nDelIdx].Ssid.Ssid[2], 
        pAdapter->ulPSNumOfBSSIDs ) );

    nLastIdx = pAdapter->ulPSNumOfBSSIDs - 1;

    if ( nLastIdx != nDelIdx )
    {
        // replace the record that you want to delete with the last record
        NdisMoveMemory(&pAdapter->PSBSSIDList[nDelIdx],
            &pAdapter->PSBSSIDList[nLastIdx],
            sizeof(NDIS_WLAN_BSSID_EX));
        NdisMoveMemory(&pAdapter->PSIEBuffer[nDelIdx],
            &pAdapter->PSIEBuffer[nLastIdx],
            sizeof(MRV_BSSID_IE_LIST));  
        NdisMoveMemory(&pAdapter->PSBssDescList[nDelIdx], 
            &pAdapter->PSBssDescList[nLastIdx],
            sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));
    }

    // clear the last record
    NdisZeroMemory(&pAdapter->PSBSSIDList[nLastIdx],
        sizeof(NDIS_WLAN_BSSID_EX));
    NdisZeroMemory(&pAdapter->PSIEBuffer[nLastIdx],
        sizeof(MRV_BSSID_IE_LIST));  
    NdisZeroMemory(&pAdapter->PSBssDescList[nLastIdx], 
        sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));

    pAdapter->ulPSNumOfBSSIDs -= 1;
    DBGPRINT( DBG_MACEVT, (L"[Mrvl] Delete the BSSID, NumOfBssid=%d\n", pAdapter->ulPSNumOfBSSIDs ) );

}

VOID
SetUpLowRssiValue(
    PMRVDRV_ADAPTER Adapter  
    )
{
    Adapter->EventRecord.EventMap   |= RSSI_LOW;
    Adapter->EventRecord.RSSILowFreq   = 0;

    PrepareAndSendCommand(
                    Adapter,
                    HostCmd_CMD_802_11_SUBSCRIBE_EVENT,
                    HostCmd_ACT_SET,
                    HostCmd_OPTION_USE_INT,
                    (NDIS_OID)0,
                    HostCmd_PENDING_ON_NONE,
                    0,
                    FALSE,
                    NULL,
                    0,
                    0,
                    NULL);
        
    return;
}

VOID
SetUpHighRssiValue(
    PMRVDRV_ADAPTER Adapter  
    )
{
    Adapter->EventRecord.EventMap   |= RSSI_HIGH;
    Adapter->EventRecord.RSSIHighFreq   = 0;
    PrepareAndSendCommand(
                    Adapter,
                    HostCmd_CMD_802_11_SUBSCRIBE_EVENT,
                    HostCmd_ACT_SET,
                    HostCmd_OPTION_USE_INT,
                    (NDIS_OID)0,
                    HostCmd_PENDING_ON_NONE,
                    0,
                    FALSE,
                    NULL,
                    0,
                    0,
                    NULL);
        
    return;

}
///#endif



VOID 
DumpBgScanConfig ( 
        POID_MRVL_DS_BG_SCAN_CONFIG  pBgScanCfg 
        )
{
    UCHAR   *pCurPos;
    UCHAR    SsidBuf[NDIS_802_11_LENGTH_SSID+1];
    USHORT  nLenOfSsid, nMaxSizeOfAllTlvs;
    MrvlIEtypesHeader_t     *pTlv;
    MrvlIEtypes_SnrThreshold_t  *pSnr;
    MrvlIEtypes_SsIdParamSet_t  *pSsid;
    ChanScanParamSet_t  *pChanParam;
    
    DBGPRINT( DBG_SCAN, (L"=== Background scan configuration dump ===\n") );

    DBGPRINT( DBG_SCAN, (L"    Enable: %d\n", pBgScanCfg->Enable) );
    // Dump TLV data
    nMaxSizeOfAllTlvs = sizeof(pBgScanCfg->TlvData);
    pCurPos = pBgScanCfg->TlvData;
    pTlv = (MrvlIEtypesHeader_t*) pCurPos;
    while( pTlv->Len && (pCurPos-pBgScanCfg->TlvData) < nMaxSizeOfAllTlvs)
    {
        switch( pTlv->Type )
        {
            case TLV_TYPE_BCASTPROBE:
                DBGPRINT( DBG_SCAN, (L"    Broadcast probe: %d\n", ((MrvlIEtypes_BcastProbe_t*)pTlv)->BcastProbe ) );
                    break;
            case TLV_TYPE_NUMPROBES:
                    DBGPRINT( DBG_SCAN, (L"    Number of probes: %d\n", ((MrvlIEtypes_NumProbes_t*)pTlv)->NumProbes ) );
                    break;
            case TLV_TYPE_NUMSSID_PROBE:
                DBGPRINT( DBG_SCAN, (L"    SSID probes: %d\n", ((MrvlIEtypes_NumSSIDProbe_t*)pTlv)->NumSSIDProbe ) );
                    break;
            case TLV_TYPE_SNR:
                    pSnr = (MrvlIEtypes_SnrThreshold_t *) pTlv;
                    DBGPRINT( DBG_SCAN, (L"    SNR: Threshold=%d, Freq=%d\n", pSnr->SNRValue, pSnr->SNRFreq ) );
                    break;
            case TLV_TYPE_SSID:
                    pSsid = (MrvlIEtypes_SsIdParamSet_t*) pTlv;
                    nLenOfSsid = (pSsid->Header.Len > NDIS_802_11_LENGTH_SSID ? NDIS_802_11_LENGTH_SSID : pSsid->Header.Len);
                    NdisMoveMemory( SsidBuf, pSsid->SsId, nLenOfSsid );
                    SsidBuf[nLenOfSsid] = 0;
                    DBGPRINT( DBG_SCAN, (L"    SSID: %s\n", SsidBuf ) );
                break;
            case TLV_TYPE_CHANLIST:
                    {
                            USHORT  nNumOfChans, nSizeOfChanTlv, nChanIdx;

                            nSizeOfChanTlv = pTlv->Len;
                            nNumOfChans = nSizeOfChanTlv / sizeof(ChanScanParamSet_t);
                            pChanParam = (ChanScanParamSet_t*) (pCurPos + sizeof(MrvlIEtypesHeader_t) );
                            DBGPRINT( DBG_SCAN, (L"    Channel: %d channels ...\n", nNumOfChans) );
                            for ( nChanIdx=0; nChanIdx<nNumOfChans; nChanIdx++ )
                            {
                                DBGPRINT( DBG_SCAN|DBG_HELP, (L"        ChanNum=%d, ScanType=%d, MinScanTime=%d, ScanTime=%d\n",
                                pChanParam->ChanNumber, pChanParam->ScanType, pChanParam->MinScanTime, pChanParam->ScanTime) );
                                pChanParam += 1;
                            }
                    }
                    break;
                    
            default:
                    break;
        }

        pCurPos += pTlv->Len + sizeof(MrvlIEtypesHeader_t);
        pTlv = (MrvlIEtypesHeader_t*) pCurPos;
    } 
}




//Enable, Disable BgScan
NDIS_STATUS 
EnableBgScan( 
        PMRVDRV_ADAPTER pAdapter, 
        BOOLEAN bEnable
        )
{
        POID_MRVL_DS_BG_SCAN_CONFIG      pNewCfg;
        
        NDIS_STATUS     Status;

       //091407
       SelectBgScanConfig( pAdapter );

       pNewCfg=(POID_MRVL_DS_BG_SCAN_CONFIG)pAdapter->BgScanCfg;

      
       if (bEnable)
        pNewCfg->Enable = 1;    
    else   
        pNewCfg->Enable = 0;    
       

    Status = PrepareAndSendCommand( 
                                pAdapter, 
                                HostCmd_CMD_802_11_BG_SCAN_CONFIG,
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

        return Status;

}

VOID
SetupBgScanCurrentSSID( 
        PMRVDRV_ADAPTER pAdapter ,
        ULONG ScanInterval
        )
{    
    POID_MRVL_DS_BG_SCAN_CONFIG pNewCfg;
    MrvlIEtypes_SsIdParamSet_t          *pSsid;
    MrvlIEtypes_ChanListParamSet_t      *pChan;
    ChanScanParamSet_t              *pChaninfo; 
    MrvlIEtypes_NumProbes_t         *pNumProbes;
    MrvlIEtypes_BcastProbe_t            *pBcProb;
    MrvlIEtypes_SnrThreshold_t          *pSnrThreshold;   
    MrvlIEtypes_NumSSIDProbe_t      *pNumSSIDProbes;
    PREGION_CHANNEL             region_channel;

    //091407
    SelectBgScanConfig(pAdapter);

       pNewCfg =(POID_MRVL_DS_BG_SCAN_CONFIG)(pAdapter->BgScanCfg);
    pNewCfg->CmdLen = 0;

    *(USHORT *)(&pNewCfg->Action)                       = 1;
    pNewCfg->Enable                       = 1;
    pNewCfg->BssType            = 1;        // Infrastructure
    pNewCfg->ChannelsPerScan    = 14;       
    pNewCfg->DiscardWhenFull           = 0;      
    pNewCfg->ScanInterval       = ScanInterval;
    pNewCfg->StoreCondition         = 1;      
    pNewCfg->ReportConditions   = 1;    
    pNewCfg->MaxScanResults            = 14;
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
    if ((region_channel->Band == MRVDRV_802_11_BAND_B) ||
    (region_channel->Band == MRVDRV_802_11_BAND_BG))
    {
        pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_BG;
    }
    else if (region_channel->Band == MRVDRV_802_11_BAND_A)
    {
        pChaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_A;
    }

    pChaninfo->ChanNumber  = pAdapter->connected_channel; 
    pChaninfo->ScanType    = HostCmd_SCAN_TYPE_ACTIVE;
    pChaninfo->ScanTime    = HostCmd_SCAN_MIN_CH_TIME;
    pChaninfo->MinScanTime = 6;
    pChan->Header.Len += sizeof(ChanScanParamSet_t);

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
    pSnrThreshold->SNRFreq = 1; 

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



void 
SaveCurrentConfig ( 
        PMRVDRV_ADAPTER pAdapter, 
        PVOID InformationBuffer, 
        ULONG InformationBufferLength,
        USHORT Cmd
        )
{
    switch( Cmd ) {
/*
#ifdef BG_SCAN
    case HostCmd_CMD_802_11_BG_SCAN_CONFIG:
           NdisMoveMemory( pAdapter->BgScanCfg,  InformationBuffer ,InformationBufferLength );
        pAdapter->nBgScanCfg = InformationBufferLength;
    return;
#endif
*/

          case HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT:
           NdisMoveMemory( pAdapter->PACfg,  InformationBuffer ,InformationBufferLength );      
           return;
     default:
        return;
        }   
    
}


VOID
GetPACFGValue( 
        PMRVDRV_ADAPTER pAdapter
        )
{
    POID_MRVL_DS_POWER_ADAPT_CFG_EXT  pNewCfg;
         
    pNewCfg =(POID_MRVL_DS_POWER_ADAPT_CFG_EXT)(pAdapter->PACfg);
    pNewCfg->CmdLen = 0;
    pNewCfg->Action                       = 0; //Get
    pNewCfg->EnablePA                       = 0; 
    pNewCfg->PowerAdaptGroup.Header.Type            = 0x114;        // Infrastructure
    pNewCfg->CmdLen = sizeof(OID_MRVL_DS_POWER_ADAPT_CFG_EXT) -sizeof(ULONG) - sizeof(USHORT);

    return;
}
 
// region-code
VOID 
SetRegionCode(
        IN PMRVDRV_ADAPTER Adapter
)
{
    DBGPRINT(DBG_LOAD, (L"Region code = 0x%x \n", Adapter->RegionCode));
    switch (Adapter->RegionCode)
    {
        case  0x20:
            Adapter->region_channel[0].Valid    = TRUE;
            Adapter->region_channel[0].Band     = MRVDRV_802_11_BAND_BG;        
            Adapter->region_channel[0].Region    = 0x20; 
            Adapter->region_channel[0].CFP        = channel_freq_power_US_BG;
            Adapter->region_channel[0].NrCFP     = 11;
            break;
            
        case 0x30:
            Adapter->region_channel[0].Valid    = TRUE;
            Adapter->region_channel[0].Band     = MRVDRV_802_11_BAND_BG;        
            Adapter->region_channel[0].Region    = 0x30; 
            Adapter->region_channel[0].CFP        = channel_freq_power_EU_BG;
            Adapter->region_channel[0].NrCFP     = 13;
            break;
            
        case 0x31:
            Adapter->region_channel[0].Valid    = TRUE;
            Adapter->region_channel[0].Band     = MRVDRV_802_11_BAND_BG;    
            Adapter->region_channel[0].Region    = 0x31; 
            Adapter->region_channel[0].CFP        = channel_freq_power_SPN_BG;
            Adapter->region_channel[0].NrCFP     = 2;
            break;
            
        case 0x32:
            Adapter->region_channel[0].Valid    = TRUE;
            Adapter->region_channel[0].Band     = MRVDRV_802_11_BAND_BG;    
            Adapter->region_channel[0].Region    = 0x32; 
            Adapter->region_channel[0].CFP        = channel_freq_power_FR_BG;
            Adapter->region_channel[0].NrCFP     = 4;
            break;
            
        case 0x40:
            Adapter->region_channel[0].Valid    = TRUE;
            Adapter->region_channel[0].Band     = MRVDRV_802_11_BAND_BG;        
            Adapter->region_channel[0].Region   = 0x40; 
            Adapter->region_channel[0].CFP       = channel_freq_power_JPN_BG;
            Adapter->region_channel[0].NrCFP    = 14;
            break;

        case 0x10:
        default:
            
            Adapter->region_channel[0].Valid    = TRUE;
            Adapter->region_channel[0].Band     = MRVDRV_802_11_BAND_BG;    
            Adapter->region_channel[0].Region    = 0x10; 
            Adapter->region_channel[0].CFP        = channel_freq_power_US_BG;
            Adapter->region_channel[0].NrCFP     = 11;
            break;
            
    }

    Adapter->cur_region_channel = &(Adapter->region_channel[0]);
    Adapter->connected_band  = MRVDRV_802_11_BAND_BG;
    Adapter->connected_channel = 1;
    
}


void DownloadGProtectSetting( PMRVDRV_ADAPTER pAdapter )
{
    NDIS_STATUS     Status;

    /*
        The usMacControlRecord is set with "g protection enabled" when initializing driver.

        When driver is a adhoc joiner, driver check the ERP flag from adhoc creater and change
        g protection setting.

        When driver is a adhoc creater, user can change the setting via an OID. When the OID 
        is called, driver records the g protection setting in usMacControlRecord.

    */

    if ( pAdapter->CurrentMacControl & HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON )
    {
        DBGPRINT( DBG_ADHOC, (L"[MRVL] going to enable G protection [0x%x]\n", pAdapter->CurrentMacControl) );
    }
    else
    {
        DBGPRINT( DBG_ADHOC, (L"[MRVL] going to disable G protection [0x%x]\n", pAdapter->CurrentMacControl) );
    }

    Status = SetMacControl( pAdapter );
}


NDIS_STATUS
HandleD3toD0(PMRVDRV_ADAPTER Adapter)
{  
    NDIS_STATUS status;  
    ULONG       cnt=0;

    DBGPRINT(DBG_OID|DBG_PS|DBG_WARNING,(L"D3->D0\r\n"));
    if (Adapter->BusPowerInD3 == 0) {
        ///No bus power in D3
        Adapter->ChipResetting = 1;
        Adapter->SoftIntStatus |= MRVL_SOFT_INT_ChipReset;
        SetEvent(Adapter->hControllerInterruptEvent);
           
        while(++cnt < 4)  
        {         
            WaitOIDAccessRespond(Adapter);
            if(Adapter->WaitEventType & MRVL_SOFT_INT_ChipReset)
            {                               
                DBGPRINT(DBG_OID|DBG_PS|DBG_WARNING,(L"D3-D0 FW Reloaded\r\n"));
                Adapter->WaitEventType &= ~MRVL_SOFT_INT_ChipReset; 
                  
                InitializeWirelessConfig(Adapter);

                Application_Event_Notify(Adapter,NOTIFY_FW_RELOADED);
                //
                status = NDIS_STATUS_SUCCESS;  
                break;  
            }                  
            else 
            {   
                status = NDIS_STATUS_FAILURE;
                continue;
            }
        }
        return status;
    }else {
        ///Bus power is still on in D3
        status = NDIS_STATUS_SUCCESS;
        if( IsThisDsState(Adapter, DS_STATE_SLEEP) )
        {                             
            SetDsState( Adapter, DS_STATE_NONE );
            If_PowerUpDevice(Adapter);
        } 
    }
  return status;

}

VOID      
HandleD0toD3(PMRVDRV_ADAPTER Adapter)
{  
   int cnt=0;
    
   DBGPRINT(DBG_OID|DBG_PS|DBG_WARNING,(L"D0->D3\r\n"));
   
   if ( Adapter->MediaConnectStatus != NdisMediaStateDisconnected )
   {
        USHORT  usCommand;
     
       if (Adapter->psState != PS_STATE_FULL_POWER)
           PSWakeup(Adapter);
                              
       // disconnect the link 
       if ( Adapter->InfrastructureMode == Ndis802_11Infrastructure )
            usCommand = HostCmd_CMD_802_11_DEAUTHENTICATE;
       else
            usCommand = HostCmd_CMD_802_11_AD_HOC_STOP;
 
       // send down deauthenticate
       PrepareAndSendCommand(
                             Adapter,
                             usCommand,
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
       
       //wait command is done
       WaitOIDAccessRespond(Adapter);
 
       Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_DISCONNECT); 
   } 

    if (Adapter->BusPowerInD3 == 0) {
        ///No Bus power in D3
        //012207
        //let IST thread return the command node.
        NdisMSleep(3000);
        //reset the Adapter object
        ResetAdapterObject(Adapter);

        Adapter->bIsScanInProgress              = FALSE;
        Adapter->bIsAssociateInProgress         = FALSE;

        Adapter->bIsAssociationBlockedByScan    = FALSE;
        Adapter->bIsScanWhileConnect            = FALSE;
        Adapter->bIsConnectToAny                = FALSE;

        //Adapter->EncryptionStatus = Ndis802_11Encryption2KeyAbsent; 
    } else {
        if( IsThisDsState(Adapter, DS_STATE_NONE) )
        {
            //put device to Deep sleep state
            PrepareAndSendCommand(
                     Adapter,        
                     HostCmd_CMD_802_11_DEEP_SLEEP,
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
   
            NdisMSleep(3000);    
            while(++cnt < 3)
            {
                if(!IsThisDsState(Adapter, DS_STATE_SLEEP))
                    NdisMSleep(10000);
                else
                    break;
            }
   
            // Tx-related variables
            CleanUpSingleTxBuffer(Adapter);
            // Rx-related variables
            ResetRxPDQ(Adapter);
            // Command-related variables
            ResetCmdBuffer(Adapter);  
        }
    }

    return;
}

//++ remove_bg_ssid
ULONG
FindSsidInBgCfg(PVOID ptlv, ULONG nsize, ULONG seq, PNDIS_802_11_SSID pNdisSsid )
{                                                       
   
   ULONG End, CurPos,Num, cnt;                        
   MrvlIEtypesHeader_t  *pCur;
   MrvlIEtypes_SsIdParamSet_t  *pSsid;

   CurPos = (ULONG)ptlv;
   End = CurPos + nsize;
                       
   Num = 0;
   cnt = 0;
   while(CurPos <  End)
   {               
      
      pCur = (MrvlIEtypesHeader_t *)CurPos; 

      if( pCur->Type == TLV_TYPE_SSID )
      {       
          cnt++; 
          pSsid = (MrvlIEtypes_SsIdParamSet_t*)pCur;
 
          DBGPRINT(DBG_SCAN|DBG_HELP,(L"Find a SSID Tlv:%x\r\n",cnt));

          if( cnt == seq )
          {
            NdisMoveMemory( (PUCHAR)&pNdisSsid->Ssid[0], (PUCHAR)&pSsid->SsId[0], pSsid->Header.Len );
            break;;
          }
      }
      else
          DBGPRINT(DBG_SCAN|DBG_HELP,(L"No SSID Tlv:%x\r\n",pCur->Type));
          

      CurPos += pCur->Len + sizeof(MrvlIEtypesHeader_t);
   }

   return cnt;
} 
//-- remove_bg_ssid

//051707 
UCHAR FindBSSIDInPSList(
      IN PMRVDRV_ADAPTER Adapter,
      IN PUCHAR BSSID)
{
    UCHAR i, index = 0xFF;                   

    for (i=0; i<Adapter->ulPSNumOfBSSIDs; i++)
    {
       if( NdisEqualMemory(Adapter->PSBSSIDList[i].MacAddress, BSSID, 6) )
       { 
           index = i;
           break;
       }
    } 
    return index;
}
  
//051707
VOID
ReplacePSListEntryByBSSListEntry( PMRVDRV_ADAPTER Adapter, ULONG PSidx, ULONG TSidx)
{

   NdisMoveMemory(&Adapter->PSBSSIDList[PSidx],
                  &Adapter->BSSIDList[TSidx],
                  sizeof(NDIS_WLAN_BSSID_EX));
   NdisMoveMemory(&Adapter->PSIEBuffer[PSidx],
                  &Adapter->IEBuffer[TSidx],
                  sizeof(MRV_BSSID_IE_LIST)); 
   NdisMoveMemory(&Adapter->PSBssDescList[PSidx], 
                  &Adapter->BssDescList[TSidx],
                  sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS)); 

   
   if( (PSidx == Adapter->ulPSNumOfBSSIDs) &&  
	   (Adapter->ulPSNumOfBSSIDs < (MRVDRV_MAX_BSSID_LIST-2)) ) ///Reserve 2 items for Adhoc creater 
   {                          
      Adapter->ulPSNumOfBSSIDs++;
      DBGPRINT(DBG_SCAN|DBG_HELP,(L"Append a etnry to PSList\r\n"));
   }
   else
   {
      DBGPRINT(DBG_SCAN|DBG_HELP,(L"Replace a etnry to PSList\r\n"));
   }  
}

//051707
VOID
RemoveAgeOutEntryFromPSList( PMRVDRV_ADAPTER Adapter)
{  
          
   int idx;
   ULONG PSnum;

   PSnum = Adapter->ulPSNumOfBSSIDs;

   for(idx=PSnum-1; idx>=0; idx--)
   {
      if( Adapter->PSBssEntryAge[idx] == 1 )
      {        
         if( idx == (PSnum-1) )//the last one 
         {    
            DBGPRINT(DBG_SCAN|DBG_HELP,(L"Remove & AgeOut last entry from PS table\r\n")); 
            PSnum--; 
         }
         else
         {  //copy current last one to the hole.  
             NdisMoveMemory( &(Adapter->PSBSSIDList[idx]), 
                             &(Adapter->PSBSSIDList[PSnum-1]), 
                             sizeof(NDIS_WLAN_BSSID_EX));

             NdisMoveMemory( &(Adapter->PSIEBuffer[idx]), 
                             &(Adapter->PSIEBuffer[PSnum-1]), 
                             sizeof(MRV_BSSID_IE_LIST));

             NdisMoveMemory( &(Adapter->PSBssDescList[idx]), 
                             &(Adapter->PSBssDescList[PSnum-1]), 
                             sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));
             PSnum--;   
             DBGPRINT(DBG_SCAN|DBG_HELP,(L"Remove one entry from PS table\r\n")); 
         }

         Adapter->PSBssEntryAge[idx] =0;
      }
   }

   DBGPRINT(DBG_SCAN|DBG_HELP|DBG_CUSTOM,(L"Totaly %d entries are removed, %d are left\r\n",(Adapter->ulPSNumOfBSSIDs-PSnum), PSnum)); 

   Adapter->ulPSNumOfBSSIDs = PSnum;

}





BOOLEAN IsValidChannel( PMRVDRV_ADAPTER Adapter, ULONG ulChannelNumber )
{
    UINT    nIdxChan;
    PREGION_CHANNEL     pCurReg;

    /*
        When driver initializes, driver fills region_channel table, region_channel[0] is for B or BG,
        region_channel[1] is for A. Currently, we only support B or G for AdHoc, so we just need to search 
        region_channel[0].
    */

    // Search for Band B or BG.
    
    pCurReg = &(Adapter->region_channel[0]);

    for ( nIdxChan=0; nIdxChan<pCurReg->NrCFP; nIdxChan ++ )
    {
        if ( pCurReg->CFP[nIdxChan].Channel == ulChannelNumber )
            return TRUE;
    }

    return FALSE;

}




//
// helper function to help us bail out early on eject events - code tests for 'WAIT_OBJECT_0' most of the time, so a eject event will look like a failure
//
DWORD WaitForSingleObjectWithCancel(PMRVDRV_ADAPTER pAdapter, HANDLE hEvent, DWORD dwTimeout)
{
    HANDLE rgHandles[] = {hEvent, pAdapter->hEjectEvent};
    DEBUGCHK(pAdapter);

    return WaitForMultipleObjects(2,rgHandles,FALSE,dwTimeout);
}

void SetDsState( PMRVDRV_ADAPTER Adapter, ULONG ulNewState )
{

    EnterCriticalSection( &Adapter->CsForDSState );
    Adapter->DSState = ulNewState;
    LeaveCriticalSection( &Adapter->CsForDSState );
}

BOOLEAN IsThisDsState( PMRVDRV_ADAPTER Adapter, ULONG ulState )
{
    BOOLEAN bResult;

    EnterCriticalSection( &Adapter->CsForDSState );
    bResult = (Adapter->DSState == ulState ? TRUE : FALSE);
    LeaveCriticalSection( &Adapter->CsForDSState );
    return bResult;
}

void SetHostPowerState( PMRVDRV_ADAPTER Adapter, ULONG ulNewState )
{
    EnterCriticalSection( &Adapter->CsForHostPowerState);
    Adapter->HostPowerState = ulNewState;
    LeaveCriticalSection( &Adapter->CsForHostPowerState );
}

BOOLEAN IsThisHostPowerState( PMRVDRV_ADAPTER Adapter, ULONG ulState )
{
    BOOLEAN bResult;

    EnterCriticalSection( &Adapter->CsForHostPowerState);
    bResult = (Adapter->HostPowerState == ulState ? TRUE : FALSE);
    LeaveCriticalSection( &Adapter->CsForHostPowerState );
    return bResult;
}

//091407
VOID
SelectBgScanConfig( PMRVDRV_ADAPTER Adapter )
{ 

    {  
       Adapter->nBgScanCfg = Adapter->BgScanCfgInfoLen[BG_SCAN_NORMAL];
       Adapter->BgScanCfg = &Adapter->BgScanCfgInfo[BG_SCAN_NORMAL][0];  
    }
}   




