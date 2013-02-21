/******************* ?Marvell Semiconductor, Inc., ***************************
 *
 *  Purpose:    This module has the implementation of TX functions
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/11/03 $
 *
 *  $Revision: #7 $
 *
 *****************************************************************************/

#include "precomp.h"
#include "pkfuncs.h"

#include "SDCardDDK.h"

#define EQ_REPLACE_QUEUE 2   


extern UCHAR  wmm_tos2ac[16][8];

///Indicate "Disconnect" to upper layer or not
///Conditions:
///     - If roaming mode is enabled & we are in the middle of roaming => Skip to indicate to the upper layer
BOOLEAN     IsIndicateDisconnect(PMRVDRV_ADAPTER pAdapter)
{
    BOOLEAN     result;

    switch (pAdapter->RoamingMode) {
        case SMLS_ROAMING_MODE:
            if (wlan_roam_query_isConnecting(pAdapter->pwlanRoamParam) == TRUE) {
                result = FALSE;
                break;
            }
            result = TRUE;
            break;
        default:
            result = TRUE;
            break;
    }

    return result;
}


/******************************************************************************
 *
 *  Name: MrvDrvSend()
 *
 *  Description: NDIS miniport serialized send packet routine
 *
 *  Conditions for Use: Protocol driver will call this routine to pass Tx NDIS_PACKET
 *
 *  Arguments:           
 *      IN  NDIS_HANDLE   MiniportAdapterContext
 *      IN  PPNDIS_PACKET Packet
 *      IN  UINT          Flags
 *    
 *  Return Value: NDIS_STATUS_RESOURCES or NDIS_STATUS_PENDING
 * 
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS
MrvDrvSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags)
{
    PMRVDRV_ADAPTER Adapter;
    NDIS_STATUS Status;



       PNDIS_BUFFER  pBuffer;
       UINT            BufferCount;
    UINT           Length;   
    UINT           TotalPacketLength = 0;   
       PVOID           pVirtualAddr;
    PTxCtrlNode      pTxNode;
    PUCHAR            pHeader = NULL;
  

    Status = NDIS_STATUS_SUCCESS;  

    Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;
    DBGPRINT(DBG_TX, (L"+MrvDrvSend()\n")); 
  //  printf ("+MrvDrvSend()\n");
    
    // In Deep Sleep Mode no packet can be sent out 
    //012207
    //if (Adapter->IsDeepSleep)
    if( !IsThisDsState(Adapter, DS_STATE_NONE) )
    {
      //Status = NDIS_STATUS_NO_CABLE;
      return NDIS_STATUS_FAILURE;
    }
  
    //      Check device removal status
    if( Adapter->SurpriseRemoved == TRUE )    
    {   
        DBGPRINT(DBG_TX|DBG_WARNING,(TEXT("[MrvSend]: NDIS_STATUS_FAILURE by supriseRemoved\r\n")));
        return NDIS_STATUS_FAILURE;
    }

    if( Adapter->bIsPendingReset == TRUE || Adapter->ChipResetting == 1)
    {                         
        DBGPRINT(DBG_TX|DBG_CMD|DBG_WARNING,(L"[MrvSend]: NDIS RETURN FAILURE by bIsPendingReset or ChipReset\r\n"));
        return NDIS_STATUS_FAILURE;
    }


    if ( Adapter->MediaConnectStatus == NdisMediaStateDisconnected )
    {
        DBGPRINT(DBG_TX|DBG_WARNING, (L"***WARNING: OS attempted to send packet while disconnected!\r\n"));
        
        if (IsIndicateDisconnect(Adapter) == TRUE) {
            Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_DISCONNECT); 
        } else {
            NdisMSleep(100000);     ///Sleep 100ms temporally
        }
        //dralee_20060712
        ResetAllScanTypeAndPower(Adapter);
        CleanUpSingleTxBuffer(Adapter);
        ResetRxPDQ(Adapter); 
        return NDIS_STATUS_FAILURE;
    }                       

    if( Adapter->bIsScanInProgress == TRUE ) {
        return NDIS_STATUS_SUCCESS;
    }

	*((ULONG *)(&Packet->MacReserved[0])) = GetTickCount();


    // check if in key absent state, if so, block all packet other than
    // 802.1x
    if ( (Adapter->EncryptionStatus == Ndis802_11Encryption2KeyAbsent )|| (Adapter->EncryptionStatus == Ndis802_11Encryption3KeyAbsent ) )  
    {
    pTxNode = &Adapter->TxNode;
        
    NdisQueryPacket( 
            Packet,
            NULL,
            &BufferCount,
            &pBuffer,
            &TotalPacketLength );

      if (!pBuffer || !BufferCount || !TotalPacketLength)
      {
        return NDIS_STATUS_FAILURE;
        
      }

       NdisQueryBuffer(pBuffer, &pVirtualAddr, &Length);
       pHeader = (PUCHAR)pVirtualAddr;

         if ( TotalPacketLength < 14 ) 
        {
            // malformed packet, blocked!
            DBGPRINT(DBG_TX|DBG_WARNING,(L"Got packet with size less than 14 bytes, reject!\n"));
            return NDIS_STATUS_FAILURE;
        }

        if ( (pHeader[12] != 0x88) || (pHeader[13] != 0x8E) )
        {
            DBGPRINT(DBG_TX|DBG_WARNING,(L"Still no key and packet type(0x%x 0x%x)is not 802.1x , drop!\n",
                                  pHeader[12],
                                  pHeader[13]));
            return NDIS_STATUS_FAILURE;
        }
     }// if ( (Adapter->EncryptionStatus == Ndis802_11Encryption2KeyAbsent )|| (Adapter->EncryptionStatus == Ndis802_11Encryption3KeyAbsent ) )  
                       
if(Adapter->TCloseWZCFlag==WZC_Ignore_Send_EAPOL_START)
{
 if ( (Adapter->EncryptionStatus == Ndis802_11Encryption2Enabled )||(Adapter->EncryptionStatus == Ndis802_11Encryption2KeyAbsent )||(Adapter->EncryptionStatus == Ndis802_11Encryption3Enabled )||(Adapter->EncryptionStatus == Ndis802_11Encryption3KeyAbsent )  )  
    {
        pTxNode = &Adapter->TxNode;
        
        NdisQueryPacket( 
            Packet,
            NULL,
            &BufferCount,
            &pBuffer,
            &TotalPacketLength );

      if (!pBuffer || !BufferCount || !TotalPacketLength)
      {
        return NDIS_STATUS_FAILURE;
        
      }

       NdisQueryBuffer(pBuffer, &pVirtualAddr, &Length);
       pHeader = (PUCHAR)pVirtualAddr;


        if ( (pHeader[12] == 0x88) && (pHeader[13] == 0x8E)&& (pHeader[14] == 0x01) &&(pHeader[15] == 0x01) )
        {
            DBGPRINT(DBG_TX|DBG_HELP,(L"Temporary don't send EAPOL-start!!EncryptionStatus=0x%x, (0x%x, 0x%x, 0x%x, 0x%x)\n",
                                  Adapter->EncryptionStatus, 
			             pHeader[12],
                                  pHeader[13],
                                  pHeader[14],
                                  pHeader[15]));
            return NDIS_STATUS_SUCCESS;
        }
 	}
}
        
        EnterCriticalSection(&Adapter->TxCriticalSection);

        if(Adapter->TxPacketCount >= (MAX_TX_PACKETS-1) )
        {   
             UCHAR sts;    
             //DBGPRINT(DBG_ERROR,(L"Tx queue is still full (count=%d), return FAILURE for this packet\r\n",Adapter->TxPacketCount));

             sts = TxPacketEnQueue(Adapter, Packet);

             if( sts == TRUE ) //101607 
             {
                 Adapter->TxPacketCount++;  
                 LeaveCriticalSection(&Adapter->TxCriticalSection);   
                 return NDIS_STATUS_SUCCESS;    
             }
             else if ( sts == EQ_REPLACE_QUEUE )
             { 
                 LeaveCriticalSection(&Adapter->TxCriticalSection); 
                 DBGPRINT(DBG_ERROR,(L"Replace a queued packet:%d\n\r",Adapter->TxPacketCount));
                 return NDIS_STATUS_SUCCESS;    
             }  
             else
             {  
                 LeaveCriticalSection(&Adapter->TxCriticalSection); 
                 //NdisMSleep(2000);   
                 DBGPRINT(DBG_ERROR,(L"Throw out current packet:%d\n\r",Adapter->TxPacketCount));  
                 //return success otherwise the endpoint may retransmit this packet that low down the throughput.
                 return NDIS_STATUS_SUCCESS;    
             }
        } 
                                   
        //record how many tx pkt is sent.   
        Adapter->TxPacketSend++;  
    
        DBGPRINT(DBG_TX|DBG_HELP,(L"[Marvell:MrvDrvSend] Adapter->TxPacketSend=%d\n", Adapter->TxPacketSend));    


        if ( TxPacketEnQueue(Adapter, Packet) != TRUE)
        {
            LeaveCriticalSection(&Adapter->TxCriticalSection);
            return NDIS_STATUS_SUCCESS;
        }
        
        Adapter->TxPacketCount++; 

        LeaveCriticalSection(&Adapter->TxCriticalSection);

        if ( Adapter->SentPacket == NULL && Adapter->TxLock == 0 )
        {
            // Fire TxThread!
            Adapter->SoftIntStatus |= MRVL_SOFT_INT_TxRequest;
            SetEvent(Adapter->hControllerInterruptEvent);
        }

        return NDIS_STATUS_SUCCESS;
        

    return Status;
} 

//++dralee_20060327
NDIS_STATUS
SendNullPacket(IN PMRVDRV_ADAPTER Adapter,UCHAR pwmgr )
{
    NDIS_STATUS        sdstatus;            
    SDIO_TX_PKT         downloadPkt;

    downloadPkt.Len = ADD_SDIO_PKT_HDR_LENGTH(sizeof(WCB));
    downloadPkt.Type = IF_DATA_PKT;                 

    NdisZeroMemory(&downloadPkt.Buf.TxDataBuf.Wcb, sizeof(WCB));
    downloadPkt.Buf.TxDataBuf.Wcb.PktPtr = sizeof(WCB); 
    downloadPkt.Buf.TxDataBuf.Wcb.PowerMgmt = pwmgr;  
    sdstatus = If_DownloadPkt(Adapter, &downloadPkt);
   
    if (!SD_API_SUCCESS(sdstatus))
    {                
        DBGPRINT(DBG_ERROR,(L"Send Null Pkt Fail\r\n"));  
        return NDIS_STATUS_FAILURE; 
    }
    return sdstatus;
}   

//++dralee_20060327
//only for PPS enabled. when sleepperiod command is set and no queued packets & pending Q
UCHAR CheckLastPacketIndication(IN PMRVDRV_ADAPTER Adapter)
{   
  
   if( Adapter->sleepperiod != 0 &&
       Adapter->TxPacketCount == 0 &&
       Adapter->CurCmd == 0 &&        //dralee_20060706
       IsQEmpty(&Adapter->CmdPendQ) )
       return 1;
   else 
       return 0;

}
//////////////////////////////////////////////////////////////
/******************************************************************************
 *
 *  Name: SendSinglePacket()
 *
 *  Description: TX packet handler
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:
 * 
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS
SendSinglePacket(
                  IN PMRVDRV_ADAPTER Adapter,
                  IN PNDIS_PACKET Packet,
                  IN PSDIO_TX_PKT pDnldPacket
)
{
    NDIS_STATUS  Status = NDIS_STATUS_SUCCESS;
    SD_API_STATUS           sdstatus;          // intermediate status

	// tt 060831: we need the following WMM_UAPSD for a compiler issue
//dralee_20060629
    if (Adapter->PPS_Enabled == 0) //WMM_UAPSD
    {
        if ( Adapter->bNullFrameSent )
        {
            return NDIS_STATUS_RESOURCES;
        }
    }
    else if (Adapter->PPS_Enabled == 1)//#else
    {
        if(Adapter->TxLock == 1)
        {  
            return NDIS_STATUS_RESOURCES;
        }
    }

    DBGPRINT(DBG_ALLEN|DBG_CCX,(L"[Marvell]SendSinglePacket Adapter->psState=%d\n",Adapter->psState));



    if(Adapter->bPSConfirm == TRUE )
    {
        DBGPRINT(DBG_TX|DBG_ERROR ,(L"Tx can not send with PS confirmed\r\n"));
        return NDIS_STATUS_RESOURCES;
    }


    if((Adapter->psState != PS_STATE_FULL_POWER) &&
        (Adapter->psState != PS_STATE_WAKEUP))
    {
        DBGPRINT(DBG_TX|DBG_ERROR ,(L"Tx can not send in the power state \n"));
        return NDIS_STATUS_RESOURCES;
    }


    if ( Adapter->WmmDesc.enabled )
    {
        pDnldPacket->Buf.TxDataBuf.Wcb.Priority = wmm_get_pkt_priority( Packet );

        pDnldPacket->Buf.TxDataBuf.Wcb.PowerMgmt = 0;
    
        if( Adapter->PPS_Enabled == 0 ) //WMM_UAPSD   dralee_20060629
        {
            if( Adapter->WmmDesc.WmmUapsdEnabled && CheckLastPacketIndication(Adapter) )
            {
                pDnldPacket->Buf.TxDataBuf.Wcb.PowerMgmt |= MRVDRV_WCB_POWER_MGMT_LAST_PACKET;
            }
        }
    }
     
//dralee_20060629
    if( Adapter->PPS_Enabled == 1 )
    {    
        //set last packet indication when PPS is enabled and system in PS mode.
        if(Adapter->psState != PS_STATE_FULL_POWER)
        {                                                                    
            if( CheckLastPacketIndication(Adapter) )
            {
                pDnldPacket->Buf.TxDataBuf.Wcb.PowerMgmt |= MRVDRV_WCB_POWER_MGMT_LAST_PACKET;
                Adapter->TxLock = 1;
            } 
            else   
                pDnldPacket->Buf.TxDataBuf.Wcb.PowerMgmt &= ~MRVDRV_WCB_POWER_MGMT_LAST_PACKET;
        }
    }

    //remember the current packet being sent
    Adapter->SentPacket = Packet; 

    Adapter->dwTxTimeoutCount = 0;

    if ((Adapter->RoamingMode == SMLS_ROAMING_MODE) ? (wlan_roam_query_isConnecting(Adapter->pwlanRoamParam) == FALSE):TRUE) {
        ///DBGPRINT(DBG_CCX,(L"Dn Pkt ....\r\n"));
        sdstatus = If_DownloadPkt(Adapter, pDnldPacket);
    } else {
        sdstatus = SD_API_STATUS_UNSUCCESSFUL;
    }
        
    Adapter->ulTxByteInLastPeriod += pDnldPacket->Len;

    if (!SD_API_SUCCESS(sdstatus))
    {                
        DBGPRINT(DBG_TX|DBG_HELP,(L"[Marvell]TX download failed!\n"));
        //dralee 09202005
        Adapter->SentPacket = NULL;    

        return NDIS_STATUS_FAILURE; 
    }
    else
    {
        DBGPRINT(DBG_LOAD,(L"TX download success!\n"));
    }
    Adapter->SeqNum++;

    return Status;
} 


NDIS_STATUS
HandleTxSingleDoneEvent(
  PMRVDRV_ADAPTER Adapter
)
{
  PNDIS_PACKET pPacket;
  NDIS_STATUS Status;
  USHORT usTxFrmSeqNum = 0, usTxFrmStatus = 0, usTCNSeqNum = 0;
  USHORT usTxCurrentRate = 0;
  PTXQ_KEEPER  pTxQKeeper;
  PTXQ_NODE    pTQNode;


  //DBGSTROBE_LINE_OFF(DBLINE2);
  DBGPRINT(DBG_TX | DBG_CRLF ,(L"+HandleTxSingleDoneEvent()\n"));

    Adapter->dwTxTimeoutCount = 0;
  
  /* Handle Txsingle done may be caused by txed data packet and command packet. 
     If caused by command packet, Adapter->SentPacket == null. 
     If caused by data packet, Adapter->SentPacket != null except the txed packet is null packet. 
     When PPS is actived, null packet with last packet indication set may be sent to FW 
  */
    Status = NDIS_STATUS_SUCCESS;

    EnterCriticalSection(&Adapter->TxCriticalSection);

    pPacket = Adapter->SentPacket;

    Adapter->SentPacket = NULL;  
    if ( ! wlan_ccx_isCurPacket(pPacket) )
    {
       
    Adapter->TxPacketSendComplete++;    

    //dralee_20051128
    if ( Adapter->MediaConnectStatus == NdisMediaStateDisconnected )
    { 
      LeaveCriticalSection(&Adapter->TxCriticalSection);
      CleanUpSingleTxBuffer(Adapter);
      ResetRxPDQ(Adapter); 
      return NDIS_STATUS_FAILURE;
    }


    DBGPRINT(DBG_TX|DBG_HELP,(L"[Marvell:HandleTxSingleDoneEvent] Adapter->TxPacketSendComplete=%d\n",  Adapter->TxPacketSendComplete));  
 
//dralee_20060629     
if( Adapter->PPS_Enabled == 0 ) //WMM_UAPSD
{
   if( Adapter->psState != PS_STATE_SLEEP )
   {     
       if( Adapter->TxPacketCount )
       { 
            TxPacketDeQueue(Adapter, &pTxQKeeper, &pTQNode);

            //++dralee_20060327
            Adapter->TxPacketCount--;

            pPacket = pTQNode -> pPacket; 

            LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx

            if(pPacket)
            {
                Status = SendSinglePacket(Adapter,pPacket,&(pTQNode->DnldPacket));

                EnterCriticalSection(&Adapter->TxCriticalSection);   // tt tx
                if(Status == NDIS_STATUS_SUCCESS)
                {
                    PushFreeTxQNode(Adapter->TxFreeQ,pTQNode);  
                    //Adapter->TxPacketCount--;  //++dralee_20060327
                }
                else
                {  
                    DBGPRINT(DBG_ERROR,(L"[TxDone] Send packet fail\r\n"));

                    //Error handling , push back this node 
                    InsertTxQNodeFromHead(pTxQKeeper,pTQNode);

                    //++dralee_20060327
                    Adapter->TxPacketCount++;  
                 } 
                LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
            }
            else
            {
                DBGPRINT(DBG_ERROR,(L"severe error, Get NULL packet...\r\n"));
// tt ++ wmm
                EnterCriticalSection(&Adapter->TxCriticalSection);   // tt tx

                PushFreeTxQNode(Adapter->TxFreeQ,pTQNode);  

                LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
// tt --
            }
       }
       else if(Adapter->WmmDesc.WmmUapsdEnabled && Adapter->psState == PS_STATE_WAKEUP ) //PS mode enabled
       {    
          LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
          Adapter->bNullFrameSent = 1;
          SendNullPacket(Adapter, MRVDRV_WCB_POWER_MGMT_NULL_PACKET|MRVDRV_WCB_POWER_MGMT_LAST_PACKET);
       }
// tt tx ++
        else
            LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
// tt tx --
   }
   else //080807 dralee
     LeaveCriticalSection(&Adapter->TxCriticalSection);   
}
else //#else      
{          //++dralee_20060327
          if( Adapter->TxPacketCount && Adapter->psState != PS_STATE_SLEEP && Adapter->TxLock == 0 )
          {       
            
            TxPacketDeQueue(Adapter, &pTxQKeeper, &pTQNode);

            //++dralee_20060327
            Adapter->TxPacketCount--;
 
            // tt ++ ps check
            //       if ( pTQNode == NULL ) {
                       ///crlo: queue is full. Leave now, instead of sending the packet
            //           DBGPRINT(DBG_TX, ("pq: 3\n") );
            //           RETAILMSG(1,(L"Exit: %s, queue is full, LeaveCriticalSection \n", TEXT(__FUNCTION__))); 
            //           LeaveCriticalSection(&Adapter->TxCriticalSection);   
            //           return NDIS_STATUS_FAILURE;
            //       }
            // tt --

              pPacket = pTQNode -> pPacket; 
                LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
              if(pPacket)
              {
                  Status = SendSinglePacket(Adapter,pPacket,&(pTQNode->DnldPacket));

                  EnterCriticalSection(&Adapter->TxCriticalSection);   // tt tx

                  if(Status == NDIS_STATUS_SUCCESS)
                  {       
                      PushFreeTxQNode(Adapter->TxFreeQ,pTQNode);  
                      //Adapter->TxPacketCount--;  //++dralee_20060327
                  }
                  else
                  {  
                      DBGPRINT(DBG_ERROR,(L"[TxDone] Send packet fail\r\n"));
              
                      //Error handling , push back this node 
                      InsertTxQNodeFromHead(pTxQKeeper,pTQNode);

                      //++dralee_20060327
                      Adapter->TxPacketCount++;  
                  } 
                  LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
              }
              else
              {
                    DBGPRINT(DBG_ERROR,(L"severe error, Get NULL packet...\r\n"));
                    /*
                        The original 37.p13 doesn't push the node back to the Tx queue.
                    */
                    EnterCriticalSection(&Adapter->TxCriticalSection);   // tt tx
                    PushFreeTxQNode(Adapter->TxFreeQ,pTQNode);  
                    LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
              }
          }
          else if( Adapter->psState == PS_STATE_WAKEUP && (CheckLastPacketIndication(Adapter)) ) 
          {                     
              if(Adapter->TxLock == 0)
              {   
                SendNullPacket(Adapter, (MRVDRV_WCB_POWER_MGMT_NULL_PACKET|MRVDRV_WCB_POWER_MGMT_LAST_PACKET));
                Adapter->TxLock = 1;    
              } 
              LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
          }
// tt tx ++
        else
            LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
// tt tx --

} //#endif // WMM_UAPSD

    }
    else
    {   
        LeaveCriticalSection(&Adapter->TxCriticalSection); //080807 +dralee

        DBGPRINT(DBG_CCX, (L"[HandleTxSingleDone] It is CCX packet\n"));
        ///crlo:slowping ++
        wlan_ccx_clrCurPacket();
        ///crlo:slowping --
    }
 

        //this condiction only true when ps_confirm is enqueue, but not txed. 
        //It's possible when doing GetCmdFromQueueToExecute(), Adapter->SentPacket != 0
        //that will stop downloading this command   
        if ( Adapter->psState == PS_STATE_SLEEP && Adapter->bPSConfirm==FALSE )
        {                     
          if( IsThisHostPowerState(Adapter, HTWK_STATE_FULL_POWER) || 
              IsThisHostPowerState(Adapter, HTWK_STATE_SLEEP) )
          {       
              if (Adapter->CurCmd == NULL)
              {
                  GetCmdFromQueueToExecute (Adapter);  
              } 
          }
        }
        //dralee_20060607, As command sending flow may be disrupted due to Adapter->SentPacket != null
        //we add this condition to let flow started again.
        else if ( Adapter->CurCmd == NULL &&
                 Adapter->SentPacket == NULL &&
                 Adapter->psState != PS_STATE_SLEEP )
        {
              GetCmdFromQueueToExecute (Adapter);  
        }

        return NDIS_STATUS_SUCCESS;
} 


/******************************************************************************
 *
 *  Name: AllocateSingleTx()
 *
 *  Description: Allocate single Tx 
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
 * 
 *  Notes:        
 *
 *****************************************************************************/
 
NDIS_STATUS AllocateSingleTx(
  IN PMRVDRV_ADAPTER Adapter
  )
{
  Adapter->TxNode.BufVirtualAddr = NULL;
  Adapter->TxNode.BufPhyAddr.LowPart = 0xffffffff;
  Adapter->TxNode.BufPhyAddr.HighPart = 0xffffffff;
  Adapter->TxNode.LocalWCB = &Adapter->Wcb;
  Adapter->TxNode.Status = MRVDRV_TX_CTRL_NODE_STATUS_IDLE;
  Adapter->Wcb.PktPtr = 0xffffffff;


    return NDIS_STATUS_SUCCESS;
}

VOID FreeSingleTx(
  IN PMRVDRV_ADAPTER Adapter
  )
{
    return;
}
                    
void InitializeTxNodeQ(IN PMRVDRV_ADAPTER Adapter )
{   
   int i; 
                             
   Adapter->TxFreeQ = (PTXQ_NODE)&Adapter->FreeTxQNode[0];
   for(i=0; i<MAX_TX_PACKETS-1; i++)
      Adapter->FreeTxQNode[i].next = (PTXQ_NODE)&Adapter->FreeTxQNode[i+1];

   Adapter->FreeTxQNode[MAX_TX_PACKETS-1].next = NULL; 
}



/******************************************************************************
 *
 *  Name: CleanUpSingleTxBuffer()
 *
 *  Description: Clean up single Tx Bbuffer
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
 * 
 *  Notes:        
 *
 *****************************************************************************/
NDIS_STATUS
CleanUpSingleTxBuffer(
IN PMRVDRV_ADAPTER Adapter
)
{      
    ULONG i;
    BOOLEAN timerStatus;
    PNDIS_PACKET pPacket;

             
    DBGPRINT(DBG_BUF|DBG_ERROR,(L"[Marvell]+CleanUpSingleTxBuffer()")); 

    if(Adapter->TxPktTimerIsSet==TRUE)
    {   
       NdisMCancelTimer(&Adapter->MrvDrvTxPktTimer, &timerStatus); 
       Adapter->TxPktTimerIsSet=FALSE;
    }
    //dralee
    //tt ra fail: Dragon suggested 2005/10/17
    EnterCriticalSection(&Adapter->TxCriticalSection);
             
    if( Adapter->SentPacket )
    {           
        pPacket = Adapter->SentPacket;
        Adapter->SentPacket = NULL;
    }
    for(i=0; i< MAX_TX_PACKETS; i++)
    {
        PTXQ_KEEPER  pTxQKeeper;
        PTXQ_NODE    pTQNode;

        TxPacketDeQueue(Adapter, &pTxQKeeper, &pTQNode);
        if(pTQNode == NULL)
           break;
               

        PushFreeTxQNode( Adapter->TxFreeQ, pTQNode );
    } 



    InitializeTxNodeQ(Adapter); 

    //set Tx queue parameter to default 
    Adapter->TxPacketCount=0;
    Adapter->TxPacketComplete = 0;
    Adapter->TxPacketPut = 0;
    Adapter->TxPacketGet = 0; 
    Adapter->SentPacket = NULL;
    
    LeaveCriticalSection(&Adapter->TxCriticalSection); 

    return NDIS_STATUS_SUCCESS;
}




BOOLEAN TxPacketEnQueue(PMRVDRV_ADAPTER Adapter,PNDIS_PACKET Packet)
{                       
  PTXQ_KEEPER  pTxQKeeper;
  PTXQ_NODE    pTQNode;
  //int          priority;
  UCHAR ac;
     UCHAR tos;

  NDIS_STATUS Status, ret=TRUE;
  
  if( Adapter->WmmDesc.enabled )
  {
    tos = wmm_get_pkt_priority( Packet );
  
     ac = wmm_tos2ac[Adapter->WmmDesc.acstatus][tos];

    //DbgWmmMsg( (L"Pkt pri: tos=%d, ac=%d\n", tos, ac) );

     /* Access control of the current packet not the Lowest */
     if(ac > AC_PRIO_BE)
     Adapter->WmmDesc.fw_notify = 1; 
  } 
  else
    ac = 3;


  if(!Adapter->TxFreeQ)
  {
     pTQNode = NULL;
     
     TxPacketDeQueueByLowPriority(Adapter,&pTxQKeeper,&pTQNode,(ULONG)ac);

     if( !pTQNode )
     {     
         ret = FALSE; 
         goto EqFinal;
     }
     else
         ret = EQ_REPLACE_QUEUE; 

  }
  else
  {
     pTQNode = PopFreeTxQNode(Adapter->TxFreeQ);
  }

  pTQNode->pPacket = Packet;    
 
  Status = PrepareDownloadPacket(Adapter, Packet, &(pTQNode->DnldPacket));
       
  if( Status == NDIS_STATUS_SUCCESS )
  {
    pTxQKeeper = (PTXQ_KEEPER)&Adapter->TXPRYQ[ac];
    InsertTxQNodeAtTail(pTxQKeeper,pTQNode); 
  }
  else
    ret = FALSE;
     
  EqFinal:

  return ret;
 
}


VOID TxPacketDeQueue(PMRVDRV_ADAPTER Adapter,PPTXQ_KEEPER ppTxQKeeper,PPTXQ_NODE ppTQNode)
{ 
    int pri;

// tt wled    for(pri=0; pri<4; pri++)
    /*
        TXPRYQ[MAX_AC_QUEUES-1] is the priority.
    */
    for(pri=MAX_AC_QUEUES-1; pri>=0; pri--) // tt wled
    {
       *ppTxQKeeper = (PTXQ_KEEPER)&Adapter->TXPRYQ[pri];
       *ppTQNode = PopFirstTxQNode(*ppTxQKeeper);
       if(*ppTQNode)
          break;
    }   

} 


//101607
ULONG TxPacketDeQueueByLowPriority(PMRVDRV_ADAPTER Adapter,PPTXQ_KEEPER ppTxQKeeper,PPTXQ_NODE ppTQNode, ULONG ac)
{           
    ULONG pri;

    for(pri=0; pri<ac; pri++) // tt wled
    {
       *ppTxQKeeper = (PTXQ_KEEPER)&Adapter->TXPRYQ[pri];
       *ppTQNode = PopFirstTxQNode(*ppTxQKeeper);
       if(*ppTQNode)
       {          
          //Adapter->AC[pri]--;
          //RETAILMSG(1,(L"****:%d:%d,%d,%d,%d\n\r",pri,Adapter->AC[0],Adapter->AC[1],Adapter->AC[2],Adapter->AC[3]));
          break; 

       }
    }
 
    return pri;   
} 






/******************************************************************************
 *
 *  Name: PrepareDownloadPacket()
 *
 *  Description: Prepare the DownloadPacket from packet send by upper layer
 *
 *  Conditions for Use:
 *
 *  Arguments:
 *      IN PMRVDRV_ADAPTER Adapter,
 *      IN PNDIS_PACKET Packet,
 *      IN PSDIO_TX_PKT pDnldPacket
 *    
 *  Return Value: None
 * 
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS PrepareDownloadPacket(PMRVDRV_ADAPTER Adapter,PNDIS_PACKET Packet, PSDIO_TX_PKT pDnldPacket)
{

  NDIS_STATUS  Status = NDIS_STATUS_SUCCESS;
  UINT BufferCount;
  UINT TotalPacketLength=0;
  UINT TotalBufferLength=0;
  UINT BytesCopied=0;
  UINT Length,AccumLength;
  PNDIS_BUFFER pBuffer, pNextBuffer;
  PVOID pVirtualAddr;
  PUCHAR pHeader = NULL;
  ULONG        coBufferLength = 0;
  ULONG i;
  PWCB    pWcb;
  UCHAR                   *pCurPtr = pDnldPacket->Buf.CmdBuf;

  Status = NDIS_STATUS_SUCCESS; 

  NdisQueryPacket(
    Packet,
    NULL,
    &BufferCount,
    &pBuffer,
    &TotalPacketLength);

  if(!pBuffer || !BufferCount || !TotalPacketLength)
  {
    Status=NDIS_STATUS_FAILURE; 

    DBGPRINT(DBG_TX|DBG_WARNING,(L"TX - NDIS buffer is not valid, return FAILURE \n"));  
    return Status;
  }
  //RETAILMSG(1,(TEXT("[Marvell]SendSinglePacket:1 NdisQueryBuffer")));
  NdisQueryBuffer(pBuffer, &pVirtualAddr, &Length);
  //RETAILMSG(1,(TEXT("[Marvell]SendSinglePacket:2 NdisQueryBuffer")));
  pHeader = (PUCHAR)pVirtualAddr;



  DBGPRINT(DBG_TX|DBG_HELP,(L"SendSinglePacket: buffers %d, packet len %d\n",BufferCount, TotalPacketLength));
  pWcb = (PWCB)&(pDnldPacket->Buf.TxDataBuf.Wcb);
  NdisZeroMemory(pWcb,sizeof(WCB));
  pWcb->Status =    MRVDRV_WCB_STATUS_USED;
  pWcb->PktLen = (USHORT)TotalPacketLength;
 
  DBGPRINT(DBG_TX|DBG_HELP,(L"DataRate = %x\n", (ULONG)Adapter -> DataRate));

    // number of retry is 3
//    pWcb->TxControl = ( (3 << 12 ) | Adapter->DataRate);
//    pWcb->TxControl =0;
    pWcb->TxControl = Adapter->TX_Control_Value; 
    pWcb->PktPtr = sizeof(WCB);
 
  
    //      First buffer contains the MAC header
    //      Call NdisMoveMemory() to copy DEST MAC adress to WCB
    //RETAILMSG(1,(TEXT("[Marvell]SendSinglePacket:1 NdisMoveMemory")));
    NdisMoveMemory(
                    (PVOID)&(pWcb->DestMACAdrHi),
                    pVirtualAddr, 
                    MRVDRV_ETH_ADDR_LEN);
   //RETAILMSG(1,(TEXT("[Marvell]SendSinglePacket:2 NdisMoveMemory")));

    {
        ULONG currenttime, packettime;
	
        currenttime = GetTickCount(); 
        packettime = *((ULONG *)(&Packet->MacReserved[0]));
        // PKtdelay in driver layer , unit 2ms
        pWcb->Reserved[0]=(UCHAR)((currenttime - packettime) / 2);
        DBGPRINT(DBG_CCX_V4,(L"PktDelay%02x ",pWcb->Reserved[0]));    
    }

  // Start the packet.
    TotalBufferLength = TotalPacketLength;
    TotalBufferLength += sizeof(WCB);


      
    // TotalBufferLength contains the size of the packet
    pDnldPacket->Len = ADD_SDIO_PKT_HDR_LENGTH(TotalBufferLength); 

    DBGPRINT(DBG_TX,(L"TX %d bytes: packet size %d\n",(ULONG)TotalBufferLength, (ULONG)TotalPacketLength));
    
    pCurPtr += sizeof(WCB);
   
  
    AccumLength = sizeof(WCB);
    //      Query each buffer for the packet and move data to SQ
    for(i=0; i<BufferCount; i++)
    {
     NdisQueryBuffer(pBuffer, &pVirtualAddr, &Length);
     AccumLength += Length;
        if( AccumLength > MRVDRV_ETH_TX_PACKET_BUFFER_SIZE )        // PJG: accum length already counts the header... need to compare to entire buffer size
            break;
       
          if( pVirtualAddr )
        {
       NdisMoveMemory(pCurPtr, (PUCHAR)pVirtualAddr, (USHORT)Length);
           pCurPtr += Length;
         }
     
      NdisGetNextBuffer(pBuffer, &pNextBuffer);
    if( !pNextBuffer )
      break;
    pBuffer = pNextBuffer;
  }//for(i=0; i<BufferCount; i++)

  DBGPRINT(DBG_TX|DBG_HELP,(L"\n"));

  DBGPRINT(DBG_TX|DBG_HELP,(L"[Marvell]TX SendSinglePacket!\n"));  

                            
  pDnldPacket->Type = IF_DATA_PKT;

return NDIS_STATUS_SUCCESS;
}




NDIS_STATUS
CCX_SendSinglePacket(
  IN PMRVDRV_ADAPTER Adapter,
  IN PNDIS_PACKET Packet
)
{
    NDIS_STATUS     status;            
    SDIO_TX_PKT     downloadPkt;
    UCHAR           *pCurPtr = (UCHAR *)downloadPkt.Buf.CmdBuf;
    UINT            TotalPacketLength=0;
    UINT            TotalBufferLength=0;
    UINT            Length;
    UINT            BufferCount;
    PNDIS_BUFFER    pBuffer;
    PVOID           pVirtualAddr;
    WCB             LocalWCB;


    NdisQueryPacket(Packet, NULL, &BufferCount, &pBuffer, &TotalPacketLength);
    if(!pBuffer || !BufferCount || !TotalPacketLength) {
        DBGPRINT(DBG_CCX|DBG_HELP,(L"TX - NDIS buffer is not valid, return FAILURE \n"));
        status=NDIS_STATUS_FAILURE; 
        goto end;
    } else {
        DBGPRINT(DBG_CCX|DBG_HELP,(L"BufferCount: %d\n", BufferCount));
    }
    NdisQueryBuffer(pBuffer, &pVirtualAddr, &Length);

    ///
    ///Fill the WCB
    ///
    NdisZeroMemory(&LocalWCB, sizeof(WCB));
    LocalWCB.Status = MRVDRV_WCB_STATUS_USED;
    ///crlo:s36 ++
    ///LocalWCB.PktLen = (USHORT)TotalPacketLength;
    LocalWCB.PktLen = (USHORT)Length;
    ///crlo:s36 --
    LocalWCB.TxControl  = 0;
    LocalWCB.PktPtr = sizeof(WCB);
    ///LocalWCB.PowerMgmt |= MRVDRV_WCB_POWER_MGMT_LAST_PACKET;
    downloadPkt.Buf.TxDataBuf.Wcb.PowerMgmt = MRVDRV_WCB_POWER_MGMT_LAST_PACKET;
    NdisMoveMemory((PVOID)LocalWCB.DestMACAdrHi,
                        pVirtualAddr, 
                        MRVDRV_ETH_ADDR_LEN);
    NdisMoveMemory(pCurPtr, (PUCHAR)&LocalWCB, sizeof(WCB));
    pCurPtr += sizeof(WCB);
    
    ///
    /// Fill in the packet data content
    ///
    ///NdisQueryBuffer(pBuffer, &pVirtualAddr, &Length);
    ///pHeader = (PUCHAR)pVirtualAddr;
    NdisMoveMemory(pCurPtr, (PUCHAR)pVirtualAddr, (USHORT)Length);
    pCurPtr += Length;


    ///downloadPkt.Len = ADD_SDIO_PKT_HDR_LENGTH(sizeof(WCB));
    ///crlo:s36 ++
    ///TotalBufferLength = TotalPacketLength;
    TotalBufferLength = Length;
    ///crlo:s36 --
    TotalBufferLength += sizeof(WCB);
    downloadPkt.Len = ADD_SDIO_PKT_HDR_LENGTH(TotalBufferLength);
    downloadPkt.Type = IF_DATA_PKT;                 

    ///NdisZeroMemory(&downloadPkt.Buf.TxDataBuf.Wcb, sizeof(WCB));
    ///downloadPkt.Buf.TxDataBuf.Wcb.PktPtr = sizeof(WCB); 
    ///downloadPkt.Buf.TxDataBuf.Wcb.PowerMgmt = pwmgr;  
    //RETAILMSG(1,(L"NULL pkt:%x\r\n",pwmgr));

    DBGPRINT(DBG_CCX|DBG_HELP,(L"CCX_SendSinglePacket, calling If_DownloadPkt\n"));
    status = If_DownloadPkt(Adapter, &downloadPkt);

    if (!SD_API_SUCCESS(status))
    {                
        DBGPRINT(DBG_CCX|DBG_HELP,(L"[Marvell]SendSinglePacket:TX download failed! "));  
        status = NDIS_STATUS_FAILURE;
        goto end;
    } else {
        DBGPRINT(DBG_CCX|DBG_HELP,(L"CCX_SendSinglePacket, If_DownloadPkt success\n"));
    }
end:
    return status;
}


void SendOneQueuedPacket( PMRVDRV_ADAPTER Adapter )
{
    PNDIS_PACKET pPacket;
    NDIS_STATUS Status;
    USHORT usTxFrmSeqNum = 0, usTxFrmStatus = 0, usTCNSeqNum = 0;
    USHORT usTxCurrentRate = 0;
    PTXQ_KEEPER  pTxQKeeper;
    PTXQ_NODE    pTQNode;

    EnterCriticalSection(&Adapter->TxCriticalSection);

    if( Adapter->TxPacketCount )
    { 
        TxPacketDeQueue(Adapter, &pTxQKeeper, &pTQNode);

        Adapter->TxPacketCount--;

        pPacket = pTQNode -> pPacket; 

        LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx

        if(pPacket)
        {
            Status = SendSinglePacket(Adapter,pPacket,&(pTQNode->DnldPacket));

            EnterCriticalSection(&Adapter->TxCriticalSection);   // tt tx

            if(Status == NDIS_STATUS_SUCCESS)
            {
                PushFreeTxQNode(Adapter->TxFreeQ,pTQNode);  
                //Adapter->TxPacketCount--;  //++dralee_20060327
            }
            else
            {  
                DBGPRINT(DBG_ERROR,(L"[TxDone] Send packet fail\r\n"));

                //Error handling , push back this node 
                InsertTxQNodeFromHead(pTxQKeeper,pTQNode);

                //++dralee_20060327
                Adapter->TxPacketCount++;
            } 

            LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
        }
    }
    else
    {
        LeaveCriticalSection(&Adapter->TxCriticalSection);   // tt tx
    }
}






