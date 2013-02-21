/*******************  Marvell Semiconductor, Inc.*****************************
 *
 *
 *  Purpose:    This module provides implementaion of host command 
 *              processing routines
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/11/11 $
 *
 *  $Revision: #8 $
 *
 *****************************************************************************/

/*
===============================================================================
            INCLUDE FILES
===============================================================================
*/
#include "precomp.h"
#include "If.h"

static BOOLEAN NeedToClearMaskCmd(PHostCmd_DS_GEN	pCmdData);

NDIS_STATUS
DownloadCommand(
  IN PMRVDRV_ADAPTER Adapter,
  IN CmdCtrlNode *pTempNode
  );
/******************************************************************************
 *
 *  Name: AllocateCmdBuffer()
 *
 *  Description: Allocate command buffer
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS or NDIS_STATUS_RESOURCES
 * 
 *  Notes:        
 *
 *****************************************************************************/
NDIS_STATUS AllocateCmdBuffer(
  IN PMRVDRV_ADAPTER Adapter
  )
{
  NDIS_STATUS Status;
  ULONG ulBufSize;
  CmdCtrlNode *TempCmdArray;
  PUCHAR pTempVirtualAddr;
  UINT i;

  Status = NDIS_STATUS_SUCCESS;

  // initialize free queue
  NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
  InitializeQKeeper(&Adapter->CmdFreeQ);
  NdisReleaseSpinLock(&Adapter->FreeQSpinLock);
    // initialize pending queue
    NdisAcquireSpinLock(&Adapter->PendQSpinLock);
    InitializeQKeeper(&Adapter->CmdPendQ);   
    NdisReleaseSpinLock(&Adapter->PendQSpinLock);

  // allocate the command array space  
  ulBufSize = sizeof(CmdCtrlNode) * MRVDRV_NUM_OF_CMD_BUFFER;
  Status = MRVDRV_ALLOC_MEM((PVOID *)(&(Adapter->CmdArray)), ulBufSize);
  if( Status != NDIS_STATUS_SUCCESS )
      return Status;
  NdisZeroMemory(Adapter->CmdArray, ulBufSize);

  // Allocate shared memory buffers 
  for ( i=0; i<MRVDRV_NUM_OF_CMD_BUFFER; i++ )
  {
    TempCmdArray = ((CmdCtrlNode *)(Adapter->CmdArray) + i);

    DBGPRINT(DBG_NEWCMD,(L"TempCmdArray %d 0x%x ****\n", 
                i, TempCmdArray));
 
    NdisAllocateMemoryWithTag(&pTempVirtualAddr, MRVDRV_SIZE_OF_CMD_BUFFER, 'LVRM');
    if ( !pTempVirtualAddr )
      return NDIS_STATUS_RESOURCES;
    
      NdisZeroMemory(pTempVirtualAddr, MRVDRV_SIZE_OF_CMD_BUFFER);

    TempCmdArray->BufVirtualAddr    = pTempVirtualAddr;
    TempCmdArray->BufPhyAddr.LowPart  = 0xffffffff;
    TempCmdArray->BufPhyAddr.HighPart = 0xffffffff;

    // insert command to free queue
    NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
    InsertQNodeAtTail(&Adapter->CmdFreeQ,(PQ_NODE) TempCmdArray);
    NdisReleaseSpinLock(&Adapter->FreeQSpinLock);
      CleanUpCmdCtrlNode(TempCmdArray);
  }

  return Status;
}

/******************************************************************************
 *
 *  Name: FreeCmdBuffer()
 *
 *  Description: Free command buffer
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS
 * 
 *  Notes:        
 *
 *****************************************************************************/
NDIS_STATUS FreeCmdBuffer(
  IN PMRVDRV_ADAPTER Adapter
  )
{
  CmdCtrlNode *TempCmdArray;
  ULONG ulBufSize;
  UINT i;

  // Need to check if cmd array is allocated or not
  if ( Adapter->CmdArray == NULL )
    return NDIS_STATUS_SUCCESS;
                                     
  DBGPRINT(DBG_BUF|DBG_HELP,(L"Free Cmd Buf \r\n"));

  // Release shared memory buffers
  for(i=0; i<MRVDRV_NUM_OF_CMD_BUFFER; i++)
  {
    TempCmdArray = ((CmdCtrlNode*)(Adapter->CmdArray) + i);
        
    if ( TempCmdArray->BufVirtualAddr ) 
      NdisFreeMemory ((PVOID)TempCmdArray->BufVirtualAddr, 
        MRVDRV_SIZE_OF_CMD_BUFFER, 
        0);
  }

  //  Release CmdCtrlNode
  if ( Adapter->CmdArray ) 
  {
    ulBufSize = sizeof(CmdCtrlNode) * MRVDRV_NUM_OF_CMD_BUFFER;
        NdisFreeMemory((PVOID *)Adapter->CmdArray, ulBufSize, 0);
  }

	///Set the event so that no one is pending for CMDs
	SetEvent(Adapter->hPendOnCmdEvent);	///Set this event before close it, in case somebody is pending for this event
	NdisMSleep(10);

	

    return NDIS_STATUS_SUCCESS;
}

/******************************************************************************
 *
 *  Name: ResetCmdBuffer()
 *
 *  Description: Reset command buffers
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS
 * 
 *  Notes:        
 *
 *****************************************************************************/
NDIS_STATUS ResetCmdBuffer(
  IN PMRVDRV_ADAPTER Adapter
  )
{
  CmdCtrlNode *TempCmdArray;
  UINT i;
    BOOLEAN  timerStatus;

    DBGPRINT(DBG_CMD, (L"ResetCmdBuffer\n"));

  // Cancel command timer
  if (Adapter->CommandTimerSet)
  {   
    NdisMCancelTimer(&Adapter->MrvDrvCommandTimer, &timerStatus);
    Adapter->CommandTimerSet = FALSE;
    Adapter->isCommandTimerExpired = FALSE; 
  }

  //051407:060407
  if( Adapter->CurCmd && 
      Adapter->OIDAccessBlocked == TRUE &&
      (Adapter->CurCmd->PendingInfo == HostCmd_PENDING_ON_SET_OID  || 
      Adapter->CurCmd->PendingInfo == HostCmd_PENDING_ON_SET_OID ) )
      Signal_Pending_OID(Adapter);

  // Reset CmdFreeQ and CmdPendingQ
    NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
  InitializeQKeeper(&Adapter->CmdFreeQ);
    NdisReleaseSpinLock(&Adapter->FreeQSpinLock);

    NdisAcquireSpinLock(&Adapter->PendQSpinLock);
    InitializeQKeeper(&Adapter->CmdPendQ); 
    NdisReleaseSpinLock(&Adapter->PendQSpinLock);

  for(i=0; i<MRVDRV_NUM_OF_CMD_BUFFER; i++)
  {
    TempCmdArray = ((CmdCtrlNode*)(Adapter->CmdArray) + i);

    // Clean up node content
    TempCmdArray->Next = NULL;

    CleanUpCmdCtrlNode(TempCmdArray);
    
    // Append node to the end of CmdFreeQ
      NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
    InsertQNodeAtTail(&Adapter->CmdFreeQ, (PQ_NODE)(TempCmdArray));
      NdisReleaseSpinLock(&Adapter->FreeQSpinLock);
  }

  Adapter->CurCmd = NULL;
  Adapter->SeqNum = 0;

    return NDIS_STATUS_SUCCESS;
}

/******************************************************************************
 *
 *  Name: GetFreeCmdCtrlNode()
 *
 *  Description: to get next available CmdCtrlNode
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter, 
 *    
 *  Return Value:        
 * 
 *  Notes: 
 *
 *****************************************************************************/
CmdCtrlNode *
GetFreeCmdCtrlNode(
  IN PMRVDRV_ADAPTER Adapter
)
{
  CmdCtrlNode *TempNode;

  if ( !Adapter )
    return NULL;

  NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
  TempNode = (CmdCtrlNode *)PopFirstQNode(&Adapter->CmdFreeQ);    
  NdisReleaseSpinLock(&Adapter->FreeQSpinLock);
  // plus_clean_up
  CleanUpCmdCtrlNode(TempNode);

  return TempNode;
}

VOID
ReturnCmdNode(
  IN PMRVDRV_ADAPTER Adapter,
  CmdCtrlNode *TempNode
  )
{
    if ( !TempNode )
    return;
  
  NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
  InsertQNodeAtTail(&Adapter->CmdFreeQ, (PQ_NODE)TempNode);
  NdisReleaseSpinLock(&Adapter->FreeQSpinLock);

  return;
}

/******************************************************************************
 *
 *  Name: SetCmdCtrlNode()
 *
 *  Description: to set up CmdCtrlNode
 *
 *  Arguments:  CmdCtrlNode *pTempNode
 *        NDIS_OID PendingOID
 *        USHORT PendingInfo
 *        USHORT INTOption
 *        USHORT PriorityPass
 *        BOOLEAN IsLastBatchCmd
 *        PULONG BytesWritten
 *        PULONG BytesRead
 *        PULONG BytesNeeded
 *        PVOID InformationBuffer
 *    
 *  Return Value:        
 * 
 *  Notes: 
 *
 *****************************************************************************/
VOID SetCmdCtrlNode(
  IN  PMRVDRV_ADAPTER Adapter,
  IN  CmdCtrlNode *pTempNode,
  IN  NDIS_OID PendingOID,
  IN  USHORT PendingInfo,
  IN  USHORT INTOption,
  IN  USHORT PriorityPass,
  IN  BOOLEAN IsLastBatchCmd,
  IN  PULONG BytesWritten,
  IN  PULONG BytesRead,
  IN  PULONG BytesNeeded,
  IN  PVOID InformationBuffer
)
{
  if( !pTempNode )
    return;
  
  pTempNode->PendingOID    = PendingOID;
  pTempNode->PendingInfo     = PendingInfo;
  pTempNode->INTOption     = INTOption;
  pTempNode->PriorityPass     = PriorityPass;
  pTempNode->IsLastBatchCmd  = IsLastBatchCmd;
  pTempNode->BytesWritten    = BytesWritten;
  pTempNode->BytesRead     = BytesRead;
  pTempNode->BytesNeeded     = BytesNeeded;
  pTempNode->InformationBuffer = InformationBuffer;
  
  return;
}
/******************************************************************************
 *
 *  Name: GetExpectedRetCode()
 *
 *  Description: 
 *
 *  Arguments:  
 *    
 *  Return Value:        
 * 
 *  Notes: 
 *
 *****************************************************************************/
USHORT 
GetExpectedRetCode(USHORT Cmd)
{
  USHORT RetCode;


  if (Cmd==HostCmd_CMD_802_11_ASSOCIATE_EXT)
    RetCode = HostCmd_RET_802_11_ASSOCIATE;
  else
    RetCode = (Cmd | 0x8000); 
   
  return RetCode;
}

/******************************************************************************
 *
 *  Name: CleanUpCmdCtrlNode()
 *
 *  Description: to clean up CmdCtrlNode
 *
 *  Arguments:  CmdCtrlNode *pTempNode
 *    
 *  Return Value:        
 * 
 *  Notes: 
 *
 *****************************************************************************/
VOID CleanUpCmdCtrlNode(
  IN  CmdCtrlNode *pTempNode
)
{
  if( !pTempNode )
    return;

  pTempNode->Status      = HostCmd_STATE_IDLE;
  pTempNode->PendingOID    = (NDIS_OID)0;
  pTempNode->ExpectedRetCode   = HostCmd_CMD_NONE;
  pTempNode->PendingInfo     = HostCmd_PENDING_ON_NONE;
  pTempNode->INTOption     = HostCmd_OPTION_USE_INT; // Default
  pTempNode->PriorityPass     = 0;
  pTempNode->IsLastBatchCmd  = FALSE;
  pTempNode->BytesWritten    = NULL;
  pTempNode->BytesRead     = NULL;
  pTempNode->BytesNeeded     = NULL;
  pTempNode->InformationBuffer = NULL;
  pTempNode->Pad[0]            = 0;
  pTempNode->Pad[1]            = 0;
  pTempNode->Pad[2]            = 0;
  pTempNode->PadExt[0]     = 0;
  pTempNode->PadExt[0]     = 0;
  pTempNode->PadExt[0]     = 0;
  pTempNode->PadExt[0]     = 0;
    pTempNode->CmdFlags      = 0;
  if( pTempNode->BufVirtualAddr != NULL )
      NdisZeroMemory(pTempNode->BufVirtualAddr, MRVDRV_SIZE_OF_CMD_BUFFER);
  
  return;
}


VOID
InsertCmdToQueue(
  IN PMRVDRV_ADAPTER Adapter,
  IN CmdCtrlNode *pTempCmd
  )
{
    USHORT Command;
    USHORT PsSubCommand;
    USHORT AddToQueue = 0xFFFF;
  
    if (!pTempCmd)
        return;

    Command = ((PHostCmd_DS_GEN)(pTempCmd->BufVirtualAddr))->Command;
    if ((Command == HostCmd_CMD_802_11_PS_MODE))
        PsSubCommand = ((PHostCmd_DS_802_11_PS_MODE)pTempCmd->BufVirtualAddr)->SubCommand;
    else
        PsSubCommand = 0;

    switch(Adapter->psState)
    {
        case PS_STATE_FULL_POWER:
            if((PsSubCommand == HostCmd_SubCmd_Enter_PS) ||
                (PsSubCommand == 0))
                AddToQueue = 0;           
        break;
        case PS_STATE_WAKEUP:
              if(PsSubCommand == HostCmd_SubCmd_Exit_PS)
                  AddToQueue = 1;            
              else if (PsSubCommand == 0)
                  AddToQueue = 0;

        break;
        case PS_STATE_SLEEP:   //dralee_20051128
             if (PsSubCommand == HostCmd_SubCmd_Sleep_Confirmed|| PsSubCommand == HostCmd_SubCmd_Exit_PS)
                 AddToQueue = 1;
             else if (PsSubCommand == 0)
                  AddToQueue = 0;
        break;
    }   
    
    if( AddToQueue == 0 )
    {  
        if( pTempCmd->PriorityPass == 1) 
            AddToQueue = 1;
    }
    if ((Command == HostCmd_CMD_802_11_ASSOCIATE_EXT) || 
        (Command == HostCmd_CMD_802_11_ASSOCIATE)){
        DBGPRINT(DBG_CCX, (L"Inserting Assoc-CMD to queue\r\n"));
    }
    switch(AddToQueue)
    {
        case 0: 
          NdisAcquireSpinLock(&Adapter->PendQSpinLock);
          InsertQNodeAtTail(&(Adapter->CmdPendQ),(PQ_NODE)pTempCmd);
          NdisReleaseSpinLock(&Adapter->PendQSpinLock);
        break;
        case 1:
          NdisAcquireSpinLock(&Adapter->PendQSpinLock);
          InsertQNodeFromHead(&(Adapter->CmdPendQ),(PQ_NODE)pTempCmd);
          NdisReleaseSpinLock(&Adapter->PendQSpinLock);
        break;
        default:  
          ReturnCmdNode(Adapter, pTempCmd);
        break;
    }     

}


VOID
GetCmdFromQueueToExecute( 
  IN PMRVDRV_ADAPTER Adapter
  )
{
    CmdCtrlNode *pTempNode=NULL;
    CmdCtrlNode *pPsEnterCmd;
    CmdCtrlNode *pPsExitCmd;
    USHORT    PsSubCommand;
    USHORT    Command;
    USHORT    IsDownloadCmdToFW;
    NDIS_STATUS     status;


    DBGPRINT(DBG_CMD|DBG_WARNING, (L"GetCmdFromQueueToExecute() \r\n"));
  
    //dralee_20060515. In case Adapter-> SentPacket is one of cases to return, the driver will lost 
    //a added on command to let FW enter PS again when Queue is empty. (see below)   
    if(Adapter->CurCmd || Adapter-> SentPacket) 
    {
        DBGPRINT(DBG_CMD|DBG_TX|DBG_WARNING|DBG_CCX, (L"GetCmdFromQueueToExecute: return 1 =>Adapter->CurCmd (%d, %d)\n", (int)Adapter->CurCmd, Adapter->SentPacket)); 
        return;
    }  

    if ( Adapter->psState == PS_STATE_SLEEP && Adapter->bPSConfirm==TRUE )
    {
        DBGPRINT(DBG_CMD|DBG_HOSTSLEEP|DBG_TX|DBG_WARNING|DBG_CCX, (L"GetCmdFromQueueToExecute: return =>Adapter->psState == PS_STATE_SLEEP  PS_STATE_SLEEP_PENDING \n")); 
        return;
    } 
  
    //dralee 072705
    if( TryEnterCriticalSection(&Adapter->CmdQueueExeSection) == 0 ) {
        DBGPRINT(DBG_CMD|DBG_TX|DBG_WARNING|DBG_CCX, (L"GetCmdFromQueueToExecute: try to get CmdQueueExeSection Fail, leaveNow\n")); 
        return; 
    }

    //dralee 08092005
    EnterCriticalSection(&Adapter->TxCriticalSection);
    //////////////////////
    IsDownloadCmdToFW = 0;
    if (!IsQEmpty(&Adapter->CmdPendQ))
    { 
        NdisAcquireSpinLock(&Adapter->PendQSpinLock);
        pTempNode = (CmdCtrlNode *)PopFirstQNode(&Adapter->CmdPendQ);
        NdisReleaseSpinLock(&Adapter->PendQSpinLock);
        
        //HostSleep with active mode FW
        if( Adapter->bPSConfirm==TRUE && IsThisHostPowerState(Adapter, HTWK_STATE_SLEEP) )
        {    
           //only pass prioritypass commands (HS cancel & Enter Deep Sleep) to FW
           if( pTempNode->PriorityPass == 0 && (((PHostCmd_DS_GEN)(pTempNode->BufVirtualAddr))->Command != HostCmd_CMD_802_11_DEEP_SLEEP))
           {  
               NdisAcquireSpinLock(&Adapter->PendQSpinLock);
               InsertQNodeFromHead(&(Adapter->CmdPendQ),(PQ_NODE)pTempNode);
               NdisReleaseSpinLock(&Adapter->PendQSpinLock);
               pTempNode=NULL; 
           }  
        }
        
        if(pTempNode != NULL)
        {
            Command = ((PHostCmd_DS_GEN)(pTempNode->BufVirtualAddr))->Command;
            if ((Command == HostCmd_CMD_802_11_ASSOCIATE_EXT) || 
                (Command == HostCmd_CMD_802_11_ASSOCIATE)){
                DBGPRINT(DBG_CCX, (L"Trying to send Assoc-CMD to queue\r\n"));
            }
            
            if ((Command ==  HostCmd_CMD_802_11_PS_MODE))
                PsSubCommand = ((PHostCmd_DS_802_11_PS_MODE)pTempNode->BufVirtualAddr)->SubCommand;
            else
                PsSubCommand = 0;

            switch(Adapter->psState)
            {
                case PS_STATE_FULL_POWER:
                    IsDownloadCmdToFW = 1;
                break;    
                case PS_STATE_WAKEUP:
                    IsDownloadCmdToFW = 1;
                    if(PsSubCommand != HostCmd_SubCmd_Exit_PS &&
                        Command != HostCmd_CMD_802_11_HOST_SLEEP_AWAKE_CONFIRM &&
                        Command != HostCmd_CMD_802_11_RSSI )
                    {
                        pPsExitCmd = GetFreeCmdCtrlNode(Adapter);
                        MakePsCmd (Adapter, pPsExitCmd, HostCmd_SubCmd_Exit_PS);
                        NdisAcquireSpinLock(&Adapter->PendQSpinLock);
                        InsertQNodeFromHead(&(Adapter->CmdPendQ),(PQ_NODE)pTempNode);
                        NdisReleaseSpinLock(&Adapter->PendQSpinLock);
                        pTempNode = pPsExitCmd;
                    }   
                    break;
                case PS_STATE_SLEEP:
                    if(PsSubCommand == HostCmd_SubCmd_Sleep_Confirmed)
                        IsDownloadCmdToFW = 1;
                    else
                    { 
                        NdisAcquireSpinLock(&Adapter->PendQSpinLock);
                        InsertQNodeFromHead(&(Adapter->CmdPendQ),(PQ_NODE)pTempNode);
                        NdisReleaseSpinLock(&Adapter->PendQSpinLock);
                    }   
                    break;
            }    
        } // (pTempNode != NULL) end    
    }
    else // Queue is empty
    {
        DBGPRINT(DBG_CMD|DBG_TX|DBG_WARNING, (L"GetCmdFromQueueToExecute: Warning, CMD_Queue is empty\n")); 
        if((Adapter->MediaConnectStatus == NdisMediaStateConnected)&&
            (Adapter->PSMode != Ndis802_11PowerModeCAM) &&
            (Adapter->psState == PS_STATE_FULL_POWER) &&
            (EncryptionStateCheck(Adapter))   // pluschen
            )
        {
            pPsEnterCmd = GetFreeCmdCtrlNode(Adapter);
            MakePsCmd (Adapter, pPsEnterCmd, HostCmd_SubCmd_Enter_PS);
            pTempNode = pPsEnterCmd;
            IsDownloadCmdToFW = 1;
        }     
    }

    if(IsDownloadCmdToFW)
    {
      status = DownloadCommand(Adapter, pTempNode);
      if ( status != NDIS_STATUS_SUCCESS)  
      {
          DBGPRINT(DBG_ERROR,(L"Download Command Fail, Command dropped\r\n"));
      }
  }
  LeaveCriticalSection(&Adapter->TxCriticalSection);   
 
  LeaveCriticalSection(&Adapter->CmdQueueExeSection); 
} 

static BOOLEAN NeedToClearMaskCmd(PHostCmd_DS_GEN	pCmdData)
{
	static BOOLEAN	result = FALSE;
	if (pCmdData->Command == HostCmd_CMD_802_11_DEEP_SLEEP) {
		result = TRUE;
	} else if (pCmdData->Command == HostCmd_CMD_802_11_PS_MODE) {
		PHostCmd_DS_802_11_PS_MODE pPsCmd = (PHostCmd_DS_802_11_PS_MODE)pCmdData;
		if (pPsCmd->SubCommand == HostCmd_SubCmd_Enter_PS) {
			result = TRUE;
		}
	}
	return result;
}


NDIS_STATUS
DownloadCommand(
  IN PMRVDRV_ADAPTER Adapter,
  IN CmdCtrlNode *pTempNode
  )
{
    BOOLEAN  timerStatus;  
    PHostCmd_DS_GEN pCmdPtr;
    ULONG           ulTemp =0;
    BOOLEAN         bQueueCMD = FALSE;
    USHORT      Command = 0;
    USHORT      PsSubCommand = 0; 
    SD_API_STATUS           status;          // intermediate status
    SDIO_TX_PKT             cmdDownload;

    DBGPRINT(DBG_CMD|DBG_HELP, (L"DownloadCommand() \n"));
  
    // Check device removal status
    if( Adapter->SurpriseRemoved == TRUE )
        return NDIS_STATUS_ADAPTER_REMOVED;
     
    //Coverity Error id:3 (REVERSE_INULL)
    if( pTempNode == NULL )
    {   
        DBGPRINT(DBG_ERROR, (L"NULL node in downloadcommand\r\n"));
        return NDIS_STATUS_FAILURE;
    }
   
    Adapter->CurCmd = pTempNode;
  
    pCmdPtr = (PHostCmd_DS_GEN)pTempNode->BufVirtualAddr;
  
    /// set command timer 
    Command = ((PHostCmd_DS_GEN)Adapter->CurCmd->BufVirtualAddr)->Command; 

    //DBGPRINT(DBG_V9,(L"Download command  0x%x ****\n", 
    //        ((PHostCmd_DS_GEN)Adapter->CurCmd->BufVirtualAddr)->Command));

	///pmkcache: bug#16956 ++
	if (((PHostCmd_DS_GEN)Adapter->CurCmd->BufVirtualAddr)->Command == HostCmd_CMD_802_11_ASSOCIATE_EXT) {
		Adapter->bIsReconnectAssociation = TRUE;
	}
	///pmkcache: bug#16956 --
    // prepare command download packet
    cmdDownload.Len = ADD_SDIO_PKT_HDR_LENGTH(pCmdPtr->Size);    
    cmdDownload.Type = IF_CMD_PKT;

    NdisMoveMemory(cmdDownload.Buf.CmdBuf, pCmdPtr, pCmdPtr->Size);


    DBGPRINT(DBG_CMD|DBG_OID|DBG_HELP|DBG_CCX,(L"Download command:%x\r\n",Command));
     
    Adapter->CommandTimerSet = TRUE;
    Adapter->isCommandTimerExpired = FALSE; 
    NdisMSetTimer(&Adapter->MrvDrvCommandTimer, MRVDRV_DEFAULT_LONG_COMMAND_TIME_OUT);

    status = If_DownloadPkt(Adapter, &cmdDownload);

    if (!SD_API_SUCCESS(status))
    {
        if ( Adapter->CurCmd != NULL )
        {
            DBGPRINT(DBG_CMD | DBG_ERROR, 
                (L"CMD 0x%x download failed!\n", 
                ((PHostCmd_DS_GEN)Adapter->CurCmd->BufVirtualAddr)->Command));
			
			if ( Adapter->CurCmd->PendingInfo == HostCmd_PENDING_ON_CMD )
            {			
				SetEvent(Adapter->hPendOnCmdEvent);
			}

            //060407 cancel command timer
            if (Adapter->CommandTimerSet == TRUE)
            {      
                BOOLEAN  timerStatus;

                NdisMCancelTimer( &Adapter->MrvDrvCommandTimer,&timerStatus); 
                Adapter->CommandTimerSet = FALSE;
            }

            // there is a race condition for suspend/resume, so need
            // to check if suspend occured
            if ( Adapter->ShutDown == TRUE ) //Adapter->ShutDown = TRUE  when: SDCardEjected
            {
                // card eject event occured, the commands are cleaned up!
                DBGPRINT(DBG_UNLOAD|DBG_ERROR, 
                    (L"Detected shutdown in DownloadCommand procedure, exit!\n"));
                return NDIS_STATUS_FAILURE;
            }

            //Clean up the command control node
            CleanUpCmdCtrlNode(Adapter->CurCmd);

            //Put current command back to CmdFreeQ
            
            NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
            InsertQNodeAtTail(&Adapter->CmdFreeQ, (PQ_NODE)Adapter->CurCmd);
            NdisReleaseSpinLock(&Adapter->FreeQSpinLock);

            Adapter->CurCmd = NULL;
        }
        else
        {
            DBGPRINT(DBG_ERROR, (L"Current command already removed, possibly due to card removal?\n"));
        }

        //090507
        NdisMCancelTimer(
                         &Adapter->MrvDrvCommandTimer, 
                         &timerStatus);
        Adapter->CommandTimerSet = FALSE;

        return NDIS_STATUS_FAILURE; 
    } 

  switch(Command)
  {       
     case HostCmd_CMD_802_11_DEEP_SLEEP:
          DBGPRINT(DBG_DEEPSLEEP|DBG_HELP,(L"Deep Sleep : DEEP_SLEEP_CMD Sent Down\n"));
          //012207
          //Adapter->IsDeepSleep = TRUE;
          SetDsState( Adapter, DS_STATE_SLEEP );
          //060407
		  //041307 No Command return, so we need to handle pending issue here.
          if(Adapter->CurCmd->PendingInfo == HostCmd_PENDING_ON_SET_OID)
          {
            //SetEvent( Adapter->hOidQueryEvent ); 051207
            Signal_Pending_OID(Adapter);
          }
          ResetCmdBuffer(Adapter);
          Application_Event_Notify(Adapter,NOTIFY_HS_ACTIVATED);  

          //dralee_20060712
          ResetAllScanTypeAndPower(Adapter);
          break;
     case HostCmd_CMD_802_11_PS_MODE:
          PsSubCommand = ((PHostCmd_DS_802_11_PS_MODE)pTempNode->BufVirtualAddr)->SubCommand;

          if(PsSubCommand == HostCmd_SubCmd_Sleep_Confirmed) 
          {    
            Adapter->bPSConfirm=TRUE; 
            UpdatePowerSaveState( Adapter, Adapter->CurCmd, 0);

            Adapter->CurCmd = NULL;
            ReturnCmdNode(Adapter, pTempNode);
            DBGPRINT(DBG_CMD|DBG_PS|DBG_HELP,(L"[Maevell]DownloadCommand :HostCmd_SubCmd_Sleep_Confirmed 0x34\n"));
            
            //--021207: 060407
            if( Adapter->bHostWakeCfgSet == SPCFG_ENABLE && 
                Adapter->SleepCfg.ulCriteria != -1 && 
                Adapter->SleepCfg.ucGap != 0xff)
            {
                DBGPRINT(DBG_ERROR,(L"Marvell: HTWK_STATE_SLEEP entered (2)\r\n"));
                SetHostPowerState( Adapter, HTWK_STATE_SLEEP );
            }

            
            if( IsThisHostPowerState(Adapter, HTWK_STATE_SLEEP) ) 
            {   
                //011506
                Application_Event_Notify(Adapter,NOTIFY_HS_ACTIVATED); 
            }
            
            //090507 
            //No command respond, so, clear command timer here
            NdisMCancelTimer(&Adapter->MrvDrvCommandTimer, 
                             &timerStatus);
            Adapter->CommandTimerSet = FALSE;

          } 
          else
            NdisMSleep(100);
          
          break; 
     case HostCmd_CMD_802_11_HOST_SLEEP_CFG: 
          if( ((PHostCmd_DS_HOST_SLEEP)pTempNode->BufVirtualAddr)->Criteria == -1 )
            {
                DBGPRINT(DBG_ERROR,(L"Marvell: HTWK_STATE_PREAWAKE entered \r\n"));
               SetHostPowerState( Adapter,  HTWK_STATE_PREAWAKE );
            }
          else
            {
                DBGPRINT(DBG_ERROR,(L"Marvell: HTWK_STATE_PRESLEEP entered \r\n"));
               SetHostPowerState( Adapter, HTWK_STATE_PRESLEEP );
            }
          break;                              
     case HostCmd_CMD_802_11_HOST_SLEEP_ACTIVATE:
          //Adapter->bPSConfirm=TRUE;
          break;
     default: 
          break;
  }

    
  return NDIS_STATUS_SUCCESS;
} 


VOID
UpdatePowerSaveState(
  IN PMRVDRV_ADAPTER Adapter,
  IN CmdCtrlNode *pTempNode,
  IN ULONG  PsEvent
  )
{
  USHORT Command;
  USHORT PsSubCommand;
  //CmdCtrlNode *pPsComfirm;

  DBGPRINT(DBG_PS|DBG_HELP,(L"++ UpdatePowerSaveState\n"));
  DBGPRINT(DBG_PS|DBG_WARNING,(L"Current ps State : %d\n",Adapter->psState));

  if (PsEvent)  // the input is a event
  {
    DBGPRINT(DBG_PS|DBG_WARNING,(L"Event Enter 0x%08x\n", PsEvent));
    
    switch(Adapter->psState)
    {                       
        case PS_STATE_WAKEUP: 
             DBGPRINT(DBG_PS,(L"Current State : PS_STATE_WAKEUP\n"));
             if (PsEvent == MACREG_INT_CODE_PS_SLEEP)
             { 
               DBGPRINT(DBG_PS|DBG_WARNING,(L"Receive Event : MACREG_INT_CODE_PS_SLEEP\n")); 

               // insert a PS_COMFIRM command to pQ  
               if( IsThisHostPowerState(Adapter, HTWK_STATE_FULL_POWER) || 
                   IsThisHostPowerState(Adapter, HTWK_STATE_SLEEP) )
               {
                   //dralee_0126, to prevent MrvDrvSend() sendout a packet at this time
                   EnterCriticalSection(&Adapter->TxCriticalSection);
                                 
                   //++dralee_20060327 
                   //if PPS is active, we only check CurCmd == NULL, otherwise, in IEEE PS, 4 conditions below must match
                   if( Adapter->CurCmd == NULL && 
                       (Adapter->sleepperiod != 0 ||
                       (Adapter->SentPacket == NULL && Adapter->TxPacketCount == 0 && IsQEmpty(&Adapter->CmdPendQ) ) ) )
                   {  
                       Adapter->psState = PS_STATE_SLEEP; 
                       LeaveCriticalSection(&Adapter->TxCriticalSection);    
                       PSConfirmSleep(Adapter);
                   }
                   else 

                   {   
                       LeaveCriticalSection(&Adapter->TxCriticalSection);   
                   }
               }
             }
             break;
        case PS_STATE_SLEEP:
             DBGPRINT(DBG_PS|DBG_HELP,(L"Current State : PS_STATE_SLEEP\r\n"));
             if (PsEvent == MACREG_INT_CODE_PS_AWAKE)
             {
               DBGPRINT(DBG_PS|DBG_HELP,(L"Receive Event : MACREG_INT_CODE_PS_AWAKE\r\n")); 
               Adapter->psState = PS_STATE_WAKEUP;  
             }
             break;
        }
  }
  else  // the input is a command
  {
    if (pTempNode==NULL)
      return;
    
    Command = ((PHostCmd_DS_GEN)(pTempNode->BufVirtualAddr))->Command;

    DBGPRINT(DBG_PS|DBG_HELP,(L"Command 0x%04x Enter\n", Command));
    
    if ((Command == HostCmd_CMD_802_11_PS_MODE) ||
       (Command == HostCmd_RET_802_11_PS_MODE))
    {
      PsSubCommand = ((PHostCmd_DS_802_11_PS_MODE)pTempNode->BufVirtualAddr)->SubCommand;

      DBGPRINT(DBG_PS,(L"PS Command : subcommand = %d\n",PsSubCommand));

      switch(Adapter->psState)
      {
        case PS_STATE_FULL_POWER:
             DBGPRINT(DBG_PS,(L"Current State : PS_STATE_FULL_POWER\n"));
             
             if (PsSubCommand == HostCmd_SubCmd_Enter_PS)
             {
               DBGPRINT(DBG_PS,(L"Receive PS SubCommand : HostCmd_SubCmd_Enter_PS\n"));
               Adapter->psState = PS_STATE_WAKEUP;
             }
             break;
        case PS_STATE_WAKEUP: 
             DBGPRINT(DBG_PS,(L"Current State : PS_STATE_WAKEUP\n"));
             if (PsSubCommand == HostCmd_SubCmd_Exit_PS)
             { 
               DBGPRINT(DBG_PS,(L"Receive PS SubCommand : HostCmd_SubCmd_Exit_PS\n"));
               Adapter->psState = PS_STATE_FULL_POWER; 
             }
             if (PsSubCommand == HostCmd_SubCmd_Sleep_Confirmed)
             {
               DBGPRINT(DBG_PS,(L"Receive PS SubCommand : HostCmd_SubCmd_Sleep_Confirmed\n"));
               Adapter->psState = PS_STATE_SLEEP;
             }
             break;
        case PS_STATE_SLEEP:  
             DBGPRINT(DBG_PS,(L"Current State : PS_STATE_SLEEP_PENDING\n"));
             if (PsSubCommand == HostCmd_SubCmd_Exit_PS)
             { 
               DBGPRINT(DBG_PS,(L"Receive PS SubCommand : HostCmd_SubCmd_Exit_PS\n"));
               Adapter->psState = PS_STATE_FULL_POWER; 
             } 
             break;   
      }
    }
  } 

  DBGPRINT(DBG_ALLEN,(L"[Marvell]UpdatePowerSaveState Adapter->psState=%d\n",Adapter->psState));
  DBGPRINT(DBG_PS,(L"-- UpdatePowerSaveState\n"));
  
  return;
}

// pluschen

UINT8 EncryptionStateCheck(IN PMRVDRV_ADAPTER Adapter)
{
     UINT8 IsAllowPS;
     
     DBGPRINT(DBG_ALL,(L"ESCheck %x \n", Adapter->EncryptionStatus));
        
     switch(Adapter->EncryptionStatus)
     {  
        case Ndis802_11Encryption1Enabled:    
        case Ndis802_11EncryptionDisabled:    
        case Ndis802_11Encryption2Enabled:
        case Ndis802_11Encryption3Enabled:
             IsAllowPS = 1;
        break;
        default:
             IsAllowPS = 0;
        break;
     }  
    
     return IsAllowPS;  
}   

