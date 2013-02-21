/******************* ?Marvell Semiconductor, Inc., 2001-2004 *****************
 *
 *  Purpose:    This module has the implementaion of RX routines
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/10/29 $
 *
 *  $Revision: #6 $
 *
 *****************************************************************************/

/*
===============================================================================
            INCLUDE FILES
===============================================================================
*/
#include "precomp.h"
#include "pkfuncs.h"

VOID
ReturnRxPacketDesc(PMRVDRV_ADAPTER Adapter,
                   PNDIS_PACKET    pPacket);


VOID
wlan_compute_rssi(PMRVDRV_ADAPTER Adapter, PRxPD   pRxPDCurrent);
/******************************************************************************
 *
 *  Name: MrvDrvReturnPacket()
 *
 *  Description: the rx ndis return packet callback function (handler) 
 *
 *  Arguments:  Packet : the packet Ndis return
 *    
 *  Notes:        
 *
 *****************************************************************************/

VOID
MrvDrvReturnPacket(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN PNDIS_PACKET Packet)
{
  PMRVDRV_ADAPTER Adapter;
       //RETAILMSG(1,(TEXT("[Marvell]+MrvDrvReturnPacket")));
  Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;

  if (Adapter->sNumOutstandingRxPacket)
      ReturnRxPacketDesc( MiniportAdapterContext, Packet);

  //RETAILMSG(1,(TEXT("[Marvell]-MrvDrvReturnPacket")));
  return;
}
 
/******************************************************************************
 *
 *  Name: AllocateRxQ()
 *
 *  Description: Allocate Rx Buffer
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
 * 
 *  Notes:        
 *
 *****************************************************************************/
NDIS_STATUS AllocateRxQ(
  IN PMRVDRV_ADAPTER Adapter
  )
{

  ULONG i;
    NDIS_STATUS tStatus;
    BOOLEAN                 bSuccess = TRUE;
    PACKET_QUEUE_NODE       **pNode;
    PNDIS_PACKET_OOB_DATA   pOOB; 

    /// Initialize rx-related variables 
    Adapter->RxBufferPoolHandle = 
    Adapter->RxPacketPoolHandle = NULL;
    //
    InitializeQKeeper(&Adapter->RxPacketFreeQueue);
    // 
    for ( i=0; i < MRVDRV_NUM_RX_PKT_IN_QUEUE; i++ )
    {
        Adapter->pRxBufVM[i] = NULL;
        Adapter->pRxBuffer[i] = NULL;
        Adapter->pRxPacket[i] = NULL;
    }

    /// Allocate all needed memory space 
    do
    {
     // packet pool 
     NdisAllocatePacketPoolEx(
                                &tStatus, 
                                &Adapter->RxPacketPoolHandle,
                                MRVDRV_NUM_RX_PKT_IN_QUEUE,
                                MRVDRV_NUM_RX_PKT_IN_QUEUE, 
                                PROTOCOL_RESERVED_SIZE_IN_PACKET);

        if ( tStatus != NDIS_STATUS_SUCCESS )
        {
            DBGPRINT(DBG_LOAD | DBG_ERROR,
                    (L"Unable to allocate packet pool!\n"));
            return tStatus;
        }

        // buffer pool 
        NdisAllocateBufferPool(
                               &tStatus,
                               &Adapter->RxBufferPoolHandle,
                               MRVDRV_NUM_RX_PKT_IN_QUEUE);
        
        if ( tStatus != NDIS_STATUS_SUCCESS )
        {
            DBGPRINT(DBG_LOAD | DBG_ERROR,
                    (L"Unable to allocate buffer pool!\n"));
            bSuccess = FALSE;
            break;
        }

    // assign space to used three array 
        for ( i=0; i < MRVDRV_NUM_RX_PKT_IN_QUEUE; i++ )
        {                          
          // data payload space array 
           tStatus = NdisAllocateMemoryWithTag(
                         &Adapter->pRxBufVM[i], 
                         MRVDRV_ETH_RX_PACKET_BUFFER_SIZE,
                         MRVDRV_MEMORY_TAG);
           //to hide unused packet header ahead of pointer.   
           //(ULONG)Adapter->pRxBufVM[i] += (sizeof(RxPD)+sizeof(pkt.Len)+sizeof(pkt.Type));
           (ULONG)Adapter->pRxBufVM[i] += MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE;

            if ( tStatus != NDIS_STATUS_SUCCESS )
            {
                bSuccess = FALSE;
                break;
            }
                      
            // buffer array 
            NdisAllocateBuffer(
                                &tStatus,
                                &Adapter->pRxBuffer[i],
                                Adapter->RxBufferPoolHandle,
                                Adapter->pRxBufVM[i],
                                (MRVDRV_ETH_RX_PACKET_BUFFER_SIZE-MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE));

            if ( tStatus != NDIS_STATUS_SUCCESS )
            {
                bSuccess = FALSE;
                break;
            }

      // packet array   
            NdisAllocatePacket(
                               &tStatus, 
                               &Adapter->pRxPacket[i],
                               Adapter->RxPacketPoolHandle);

            if ( tStatus != NDIS_STATUS_SUCCESS )
            {
                bSuccess = FALSE;
                break;
            }

            // init OBB space
            pOOB = NDIS_OOB_DATA_FROM_PACKET(Adapter->pRxPacket[i]);
            NdisZeroMemory(pOOB, sizeof(NDIS_PACKET_OOB_DATA));
            NDIS_SET_PACKET_HEADER_SIZE(Adapter->pRxPacket[i], MRVDRV_ETH_HEADER_SIZE);

            // chain the packet and buffer 
            NdisChainBufferAtFront(Adapter->pRxPacket[i],
                                   Adapter->pRxBuffer[i]);

            // fill packet node 
            Adapter->RxPacketQueueNode[i].pPacket = Adapter->pRxPacket[i];
    
            pNode = (PACKET_QUEUE_NODE **)Adapter->pRxPacket[i]->MiniportReserved;
            *pNode = &Adapter->RxPacketQueueNode[i];
            
            // insert to free queue
            InsertQNodeAtTail(&Adapter->RxPacketFreeQueue, &Adapter->RxPacketQueueNode[i]);

        }   // end of for(;;)

    } while (0);

    if ( ! bSuccess )
    {
        // clean up all
        FreeRxQ(Adapter);
        return NDIS_STATUS_FAILURE;
    }

    Adapter->sNumOutstandingRxPacket = 0;

    return NDIS_STATUS_SUCCESS;
}

/******************************************************************************
 *
 *  Name: FreeRxQ()
 *
 *  Description: Free Rx buffer
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
 * 
 *  Notes:        
 *
 *****************************************************************************/
NDIS_STATUS FreeRxQ(
  IN PMRVDRV_ADAPTER Adapter
  )
{
    ULONG i;    

    DBGPRINT(DBG_BUF|DBG_RX|DBG_HELP,(L"Free Rx Q\r\n"));
   
    for ( i=0; i < MRVDRV_NUM_RX_PKT_IN_QUEUE; i++ )
    {
        if ( Adapter->pRxBufVM[i] != NULL )
        {                       
            (ULONG)Adapter->pRxBufVM[i] -= MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE; 

            NdisFreeMemory(Adapter->pRxBufVM[i],
                           MRVDRV_ETH_RX_PACKET_BUFFER_SIZE,
                           0);            
            Adapter->pRxBufVM[i] = NULL;
        }

        if ( Adapter->pRxBuffer[i] != NULL )
        {
            NdisFreeBuffer(Adapter->pRxBuffer[i]);
            Adapter->pRxBuffer[i] = NULL;
        }

        if ( Adapter->pRxPacket[i] != NULL )
        {
            NdisFreePacket(Adapter->pRxPacket[i]);
            Adapter->pRxPacket[i] = NULL;
        }
    } 
    
    if ( Adapter->RxBufferPoolHandle != NULL )
    {
        NdisFreeBufferPool(Adapter->RxBufferPoolHandle);
        Adapter->RxBufferPoolHandle = NULL;
    }

    if ( Adapter->RxPacketPoolHandle != NULL )
    {
        NdisFreePacketPool(Adapter->RxPacketPoolHandle);
        Adapter->RxPacketPoolHandle = NULL;
    }
    return NDIS_STATUS_SUCCESS;
}

/******************************************************************************
 *
 *  Name: ResetRxPDQ()
 *
 *  Description: RxPDQ reset
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:        
 * 
 *
 *****************************************************************************/
VOID
ResetRxPDQ(
  IN PMRVDRV_ADAPTER Adapter)
{


  ULONG i;
      
  NdisAcquireSpinLock(&Adapter->RxQueueSpinLock);

  Adapter->sNumOutstandingRxPacket = 0;
  InitializeQKeeper(&Adapter->RxPacketFreeQueue);
  
    for ( i=0; i < MRVDRV_NUM_RX_PKT_IN_QUEUE; i++ )
    InsertQNodeAtTail(&Adapter->RxPacketFreeQueue, &Adapter->RxPacketQueueNode[i]);

    NdisReleaseSpinLock(&Adapter->RxQueueSpinLock);
  return;
}


/******************************************************************************
 *
 *  Name: GetRxPacketDesc()
 *
 *  Description: Get a free RX Packet descriptor
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: MRVDRV_GET_PACKET_STATUS
 * 
 *  Notes:        
 *
 *****************************************************************************/
__inline
extern MRVDRV_GET_PACKET_STATUS
GetRxPacketDesc(PMRVDRV_ADAPTER Adapter,
                PNDIS_PACKET    *p_PPacket)
{
    PPACKET_QUEUE_NODE  pPacketNode;

    NdisAcquireSpinLock(&Adapter->RxQueueSpinLock);

    if ( Adapter->sNumOutstandingRxPacket == MRVDRV_NUM_RX_PKT_IN_QUEUE )
    {
        NdisReleaseSpinLock(&Adapter->RxQueueSpinLock);
        return GPS_FAILED;
    }

    // get a free packet 
    pPacketNode = (PPACKET_QUEUE_NODE)PopFirstQNode(&Adapter->RxPacketFreeQueue);

    // return the pointer of the packet 
    *p_PPacket = pPacketNode->pPacket;
  
    Adapter->sNumOutstandingRxPacket++;

    NdisReleaseSpinLock(&Adapter->RxQueueSpinLock);

    DBGPRINT(DBG_RX|DBG_HELP, (L"Packet 0x%x allocated, NumOutstandingRxPacket = %d\n", 
                               pPacketNode->pPacket,
                               Adapter->sNumOutstandingRxPacket));

    return GPS_SUCCESS;
}

/******************************************************************************
 *
 *  Name: ReturnRxPacketDesc()
 *
 *  Description: Return a RX Packet descriptor
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NONE
 * 
 *  Notes:        
 *
 *****************************************************************************/
__inline 
VOID
ReturnRxPacketDesc(PMRVDRV_ADAPTER Adapter,
                   PNDIS_PACKET    pPacket)
{
    PPACKET_QUEUE_NODE  pPacketNode = *((PACKET_QUEUE_NODE **)pPacket->MiniportReserved);

    NdisAcquireSpinLock(&Adapter->RxQueueSpinLock);

    InsertQNodeFromHead(&Adapter->RxPacketFreeQueue, pPacketNode);

    if ( Adapter->sNumOutstandingRxPacket < 0 )
    {
        DBGPRINT(DBG_ERROR, (L"ERROR: numOutstandingPacket already 0 before subtraction!!\n"));
        NdisReleaseSpinLock(&Adapter->RxQueueSpinLock);
        return;
    }

    Adapter->sNumOutstandingRxPacket--;
    DBGPRINT(DBG_RX|DBG_HELP, (L"PacketNode(0x%x) with Packet(0x%x) returned, NumOutstandingRxPacket = %d\n", 
                            pPacketNode,
                            pPacketNode->pPacket,
                            Adapter->sNumOutstandingRxPacket));
    
    NdisReleaseSpinLock(&Adapter->RxQueueSpinLock);
}

/******************************************************************************
 *
 *  Name: wlan_compute_rssi()
 *
 *  Description: This function computes the RSSI in received packet
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter:  A pointer to wlan adapter structure
                        RxPD   pRxPDCurrent: A pointer to RxPD structure of received packet
 *    
 *  Return Value: NONE
 * 
 *  Notes:        
 *
 *****************************************************************************/
__inline 
VOID
wlan_compute_rssi(PMRVDRV_ADAPTER Adapter, PRxPD   pRxPDCurrent)
{

    DBGPRINT(DBG_RSSI|DBG_HELP ,(L"RxPD: SNR = %d, NF = %d\n", pRxPDCurrent->SNR, pRxPDCurrent->NF));
    DBGPRINT(DBG_RSSI|DBG_HELP ,(L"Before computing SNR and NF\n"));
    DBGPRINT(DBG_RSSI|DBG_HELP,(L"Adapter: SNR- avg = %d, NF-avg = %d\n", Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE, Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE));
  
     Adapter->SNR[TYPE_RXPD][TYPE_NOAVG] = pRxPDCurrent->SNR;
     Adapter->NF[TYPE_RXPD][TYPE_NOAVG] = pRxPDCurrent->NF;
     Adapter->RxPDRate = pRxPDCurrent->RxRate;
     //Adapter->RxPDSNRAge = os_time_get();

      /* Average out the SNR from the received packet */
      Adapter->SNR[TYPE_RXPD][TYPE_AVG] =
        CAL_AVG_SNR_NF(Adapter->SNR[TYPE_RXPD][TYPE_AVG], pRxPDCurrent->SNR,Adapter->data_avg_factor);

      /* Average out the NF value */
      Adapter->NF[TYPE_RXPD][TYPE_AVG] = 
        CAL_AVG_SNR_NF(Adapter->NF[TYPE_RXPD][TYPE_AVG], pRxPDCurrent->NF, Adapter->data_avg_factor);
    
      DBGPRINT(DBG_RSSI|DBG_HELP ,(L"After computing SNR and NF\n"));
      DBGPRINT(DBG_RSSI|DBG_HELP ,(L"Adapter: SNR- avg = %d, NF-avg = %d\n", (Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE), (Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE)));

      Adapter->RSSI[TYPE_RXPD][TYPE_NOAVG] = 
            (SHORT)CAL_RSSI(Adapter->SNR[TYPE_RXPD][TYPE_NOAVG],
                Adapter->NF[TYPE_RXPD][TYPE_NOAVG]);

      Adapter->RSSI[TYPE_RXPD][TYPE_AVG] = 
            (SHORT)CAL_RSSI(Adapter->SNR[TYPE_RXPD][TYPE_AVG] / AVG_SCALE,
                Adapter->NF[TYPE_RXPD][TYPE_AVG] / AVG_SCALE);

    
      Adapter->ulRSSITickCount=GetTickCount();

      if ((Adapter->RSSI[TYPE_RXPD][TYPE_AVG] > -10) || (Adapter->RSSI[TYPE_RXPD][TYPE_AVG] < -200))
      {
    
         if(Adapter->MediaConnectStatus == NdisMediaStateConnected)
         {
          DBGPRINT(DBG_ERROR, (L"ERROR: Incorrect RSSI Value2 - SNR = %d, NF= %d, Adapter->RSSI[TYPE_RXPD][TYPE_AVG] = %d, Adapter->LastRSSI = %d\n", 
                               pRxPDCurrent->SNR, 
                               pRxPDCurrent->NF,
                               Adapter->RSSI[TYPE_RXPD][TYPE_AVG],
                               Adapter->LastRSSI));
         }
      
      }
      else
      {           
              Adapter->LastRSSI = (LONG)Adapter->RSSI[TYPE_RXPD][TYPE_AVG];                      
      }
        
       if (Adapter->LastRSSI <= Adapter->RSSITriggerValue)
        {
          // Increment RSSITriggerCounter
          Adapter->RSSITriggerCounter++;
        }
        else
        {
          // Reset the counter if RSSI goes above the trigger level
          if (Adapter->RSSITriggerCounter !=0)
            Adapter->RSSITriggerCounter=0;
        }
      
        // If the trigger occurs many times, send indication above
      if (Adapter->RSSITriggerCounter >= MRVDRV_RSSI_INDICATION_THRESHOLD)
        {     
          // Indicate to protocol driver about RSSI status
          NdisMIndicateStatus(Adapter->MrvDrvAdapterHdl,
                NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                &(Adapter->LastRSSI),
                sizeof(LONG));
          NdisMIndicateStatusComplete(Adapter->MrvDrvAdapterHdl);
          // Reset the counter
          Adapter->RSSITriggerCounter=0;
    
        }

}



/******************************************************************************
 *
 *  Name: HandleRxReadyEvent()
 *
 *  Description: Rx ready event handler
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleRxReadyEvent(
    IN PMRVDRV_ADAPTER Adapter
)
{
    int           IsRxOK = 0;
    PRxPD         pRxPDCurrent;
    PNDIS_PACKET  pPacket;
    NDIS_STATUS   pStatus;


    DBGPRINT(DBG_RX | DBG_HELP,(L"+HandleRxReadyEvent()\n"));
    pRxPDCurrent = (PRxPD)(Adapter->pRxPD1);


//lykao, 060905, begin
    if (pRxPDCurrent->Status & MRVDRV_RXPD_STATUS_OK)
    {
        Adapter->RcvOK++;
        Adapter->DirectedFramesRcvOK++;
        wlan_compute_rssi(Adapter,pRxPDCurrent);
    }
    else
    {
        DBGPRINT(DBG_RX | DBG_WARNING,(L"WARNING: frame received with bad status\n"));

        //dralee++ 09212005 for error handling 
        pPacket = Adapter->pRxCurPkt;
        Adapter->pRxCurPkt = NULL; 
        if ( pPacket )
            ReturnRxPacketDesc(Adapter,pPacket);
        return;
    }
        
    pPacket = Adapter->pRxCurPkt;
    Adapter->pRxCurPkt = NULL;  

    if (Adapter->MediaConnectStatus == NdisMediaStateConnected)
    {              
        Adapter->ulRxByteInLastPeriod += Adapter->ulRxSize;  

        NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_SUCCESS);

        NdisMIndicateReceivePacket(Adapter->MrvDrvAdapterHdl, &pPacket, 1);
      
        pStatus = NDIS_GET_PACKET_STATUS(pPacket);
        
        if ((pStatus == NDIS_STATUS_RESOURCES) || (pStatus == NDIS_STATUS_SUCCESS))
        {
            // return packet
            DBGPRINT(DBG_RX|DBG_HELP, (L"Packet returned success or resources...\n"));
            ReturnRxPacketDesc(Adapter,pPacket);            
        }
        else
        {
            DBGPRINT(DBG_RX|DBG_ERROR, (L"Packet returned pending...\n"));
        }
    }
    else
    {
        ///pmkcache: bug#16956 ++		
        if (Adapter->bIsReconnectAssociation == TRUE)
        {
            Adapter->isPktPending = TRUE;
            if ( Adapter->pPendedRxPkt )
            {
                NKDbgPrintfW( L"ERROR, a pended RX packet has not been process!!\r\n" );
            }
            Adapter->pPendedRxPkt = pPacket;
            return;
        }
        ///pmkcache: bug#16956 --

        DBGPRINT(DBG_RX|DBG_ERROR, (L"Not connected, packet was dropped...\n"));
        ReturnRxPacketDesc(Adapter,pPacket);
    } 

    return;
}
