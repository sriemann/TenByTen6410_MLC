/******************* ?Marvell Semiconductor, Inc., ***************************
 *
 *  Purpose:    This module provides implementaion of ISR, DPC and other 
 *              interrupt related routines
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/10/27 $
 *
 *  $Revision: #3 $
 *
 *****************************************************************************/

/*
===============================================================================
INCLUDE FILES
===============================================================================
*/
#include "precomp.h"
#include "If.h"
#include "pkfuncs.h"

extern HANDLE g_hLoaderThread;
extern BOOL g_fMrvDrvHaltCalled;
extern CRITICAL_SECTION    LoaderCriticalSection;

/******************************************************************************
*
*  Name: MrvDrvDisableInterrupt()
*
*  Description: Miniport HW interrupt disable routine
*
*  Conditions for Use: Will be called to disable HW PCI/CardBus interrupt
*
*  Arguments:
*      IN NDIS_HANDLE MiniportAdapterContext
*    
*  Return Value: None
* 
*  Notes:               
*
*****************************************************************************/
VOID
MrvDrvDisableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    )
{
    PMRVDRV_ADAPTER Adapter;

    DBGPRINT(DBG_ISR,(L"INT - Enter MrvDrvDisableInterrupt \n"));
    Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;
    {
        SD_API_STATUS   status;          // intermediate status

    // Though macro names are different in below lines,
    // the values defined for them are same. To avoid
    // confusion, we have redefined with new names.
    // See "sdio.h" for their defenitions.
    //
        UCHAR           ucMask = SDIO_ENABLE_INTERRUPT;;       
        status = If_WriteRegister(Adapter,
                                                //SD_IO_WRITE ,
                                                1, // function 1
                                                HCR_HOST_INT_MASK_REGISTER, // reg 4
                                                FALSE,
                                                &ucMask,
                                                sizeof(ucMask));
    }
}

/******************************************************************************
*
*  Name: MrvDrvEnableInterrupt()
*
*  Description: Miniport HW interrupt enable routine
*
*  Conditions for Use: Will be called to enable HW PCI/CardBus interrupt
*
*  Arguments:
*      IN NDIS_HANDLE MiniportAdapterContext
*    
*  Return Value: None
* 
*  Notes:               
*
*****************************************************************************/
VOID
MrvDrvEnableInterrupt(
  IN NDIS_HANDLE MiniportAdapterContext
    )
{
    PMRVDRV_ADAPTER Adapter;

    DBGPRINT(DBG_ISR,(L"INT - Enter MrvDrvEnbleInterrupt \n"));
    Adapter = (PMRVDRV_ADAPTER)MiniportAdapterContext;
    {
        SD_API_STATUS   status;          // intermediate status

        // Though macro names are different in below lines,
        // the values defined for them are same. To avoid
        // confusion, we have redefined with new names.
        // See "sdio.h" for their defenitions.
        //
        UCHAR           ucMask = SDIO_DISABLE_INTERRUPT;;       
        status = If_WriteRegister(Adapter,
                                                //SD_IO_WRITE ,
                                                1, // function 1
                                                HCR_HOST_INT_MASK_REGISTER, // reg 4
                                                FALSE,
                                                &ucMask,
                                                sizeof(ucMask));
    }
}

///////////////////////////////////////////////////////////////////////////////
//  SDNdisSlotEventCallBack - slot event callback for fast-path events
//  Input:  hDevice - device handle
//          pContext - device specific context that was registered
//          SlotEventType - slot event type
//          pData - Slot event data (can be NULL)
//          DataLength - length of slot event data (can be 0)
//  Output:
//  Returns: 
//  Notes:  
//
//      If this callback is registered the client driver can be notified of
//      slot events (such as device removal) using a fast path mechanism.  This
//      is useful if a driver must be notified of device removal 
//      before its XXX_Deinit is called.  
//
//      This callback can be called at a high thread priority and should only
//      set flags or set events.  This callback must not perform any
//      bus requests or call any apis that can perform bus requests.
///////////////////////////////////////////////////////////////////////////////
VOID SDNdisSlotEventCallBack(SD_DEVICE_HANDLE    hDevice,
                             PVOID               pContext,                      
                             SD_SLOT_EVENT_TYPE  SlotEventType,         
                             PVOID               pData,                      
                             DWORD               DataLength)
{
    PMRVDRV_ADAPTER pAdapter;
    //BOOLEAN timerStatus;

    DBGPRINT(DBG_OID,(L"Entered +SDNdisSlotEventCallBack: event 0x%x \n", 
                        SlotEventType));

    switch (SlotEventType)
    {
        case SDCardEjected :
                 
            DBGPRINT(DBG_ERROR, (L"SDCardEjected Event!\n"));

            EnterCriticalSection(&LoaderCriticalSection);

            // there is a race where we can get this call without having a valid instance of 
            if(!g_fMrvDrvHaltCalled)
            {               
                pAdapter = (PMRVDRV_ADAPTER)pContext;

                   pAdapter->SurpriseRemoved = TRUE; 
                   pAdapter->ShutDown = TRUE;

                   // needed to cancel pending waits
                   SetEvent(pAdapter->hEjectEvent);
            }

            LeaveCriticalSection(&LoaderCriticalSection);

          break;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  SDNdisInterruptCallback - interrupt handler
//  Input:  hDevice - handle to the SDIO device
//          pAdapter - adapter context
//  Output: 
//  Returns:  SD_API_STATUS code
//  Notes:  
//
//      SD_API_STATUS_SUCCESS  is returned for HW to acknowledge the interrupt
///////////////////////////////////////////////////////////////////////////////

SD_API_STATUS SDNdisInterruptCallback(SD_DEVICE_HANDLE hDevice, 
                                      PVOID pContext)
{                         
    
    PMRVDRV_ADAPTER  pAdapter;
    UCHAR            ucHostIntStatus;
    UCHAR            ucCardStatus;
    SD_API_STATUS    status;

    pAdapter = (PMRVDRV_ADAPTER)pContext;

    //clear Int work around timeout count
    pAdapter->IntWARTimeoutCount = 0;

        
    // read Host Int Status register (function 1,addr 6)
    status = If_ReadRegister(pAdapter,
                                        //SD_IO_READ ,
                                        1, // function 1
                                        HCR_HOST_INT_STATUS_REGISTER, 
                                        FALSE,
                                        &ucHostIntStatus,
                                        sizeof(ucHostIntStatus));
    
    if (!SD_API_SUCCESS(status))
    {
        DBGPRINT(DBG_ISR,(L"Unable to read Host interrupt status register\n"));
        DBGPRINT(DBG_ISR, (L"ISR read error 1\n") );
        return status;
    } 
     
    EnterCriticalSection(&pAdapter->IntCriticalSection);   
    pAdapter->ucGHostIntStatus |= ucHostIntStatus; 
    LeaveCriticalSection(&pAdapter->IntCriticalSection);   
    ucCardStatus = ~ucHostIntStatus;    
    //dralee_1226
    ucCardStatus &= 0x1f;
    //EnterCriticalSection(&pAdapter->SDIOCriticalSection);
    status = If_WriteRegister(pAdapter,
                                        //SD_IO_WRITE,          
                                        1,     
                                        HCR_HOST_INT_STATUS_REGISTER,
                                        FALSE,
                                        &ucCardStatus,   // reg
                                        sizeof(ucCardStatus));   

    if (!SD_API_SUCCESS(status))
    {
        DBGPRINT(DBG_ISR,(L"Unable to clear Host interrupt status register\n"));
        DBGPRINT(DBG_ISR, (L"ISR read error 2\n") );
        return status;
    } 

    if ( ucHostIntStatus == 0xff )
    {
        // card removed?
        DBGPRINT(DBG_ISR,(L"read 0xff\n"));
        return SD_API_STATUS_SUCCESS;
    }  

    SetEvent(pAdapter->hControllerInterruptEvent);
  
    return SD_API_STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//  MrvDrvSdioAsyncBusRequestCB -  handles asynchronous bus call back for
//                                 reading CMD53 from card.
//                                 It is used when the response is discarded
//                                 so it just frees up the buffer
//    
///////////////////////////////////////////////////////////////////////////////
VOID MrvDrvSdioAsyncBusRequestCB(SD_DEVICE_HANDLE hDevice,
                                 PSD_BUS_REQUEST  pRequest,
                                 PVOID            pDeviceContext,
                                 DWORD            DeviceParam)
{
    SDFreeBusRequest(pRequest);
}





///////////////////////////////////////////////////////////////////////////////
//  MrvIstThread -  IST to get back to miniport context during
//                      interrupt
//  Input:  
//  Output: 
//  Returns:  
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
void MrvIstThread(PVOID pContext)
{
    UCHAR                          ucCardStatus;
    USHORT                         usLength;
    DWORD waitStatus;    // wait status

    PMRVDRV_ADAPTER                pAdapter = (PMRVDRV_ADAPTER) pContext;
    static HostCmd_DS_CODE_DNLD    *p_pkt=NULL;
    static HostCmd_DS_CODE_DNLD      pkt;
  
    UCHAR                    macEvent;

    BOOLEAN                     ErrorFlag=FALSE;
    PVOID                       pRxBufVM=NULL; //Coverity Error id:17 (UNINIT)
    

      UCHAR usWrongPktLen=0;    
#define TT_WRONG_PKT_LEN_TOO_BIG    1
#define TT_WRONG_PKT_LEN_TOO_SMALL  2

  //--------------
    UCHAR   ucLHostIntStatus=0; 

    UCHAR   ucLCardStatus=0;


    DBGPRINT(DBG_TX|DBG_HELP,(L"[Marvell:MrvIstThread] pAdapter->SdioIstThread=%d\n", pAdapter->SdioIstThread));     
    //CeSetThreadPriority(GetCurrentThread(), 101);      // don't set priority higher than 101
    
    CeSetThreadPriority(GetCurrentThread(), pAdapter->SdioIstThread); 
    {
        DBGPRINT(DBG_RX|DBG_TX|DBG_WARNING, (L"IstThread pri: %d\n", CeGetThreadPriority(GetCurrentThread())) );
    }
    SetEvent(pAdapter->hThreadReadyEvent);  
   while(1) 
   {
        waitStatus = WaitForSingleObject(pAdapter->hControllerInterruptEvent, INFINITE);

        if(WaitForSingleObject(pAdapter->hEjectEvent,0) == WAIT_OBJECT_0)
        {
            // cancelled
            
			DBGPRINT(DBG_RX|DBG_TX|DBG_WARNING, (L"My god hEjectEvent event \r\n"));
            return;
        }
        
     while( pAdapter->SoftIntStatus )
     {
        //add internal software interrupt event 
        if(pAdapter->SoftIntStatus & MRVL_SOFT_INT_ChipReset)
        {    
               
            DBGPRINT(DBG_OID|DBG_PS|DBG_HELP,(L"Post handle Chip Reset\r\n"));
            pAdapter->SoftIntStatus &= ~MRVL_SOFT_INT_ChipReset; 
                                                   
            ProcessOIDResetChip(pAdapter);
                                               
            //Application may reset chip in sleep state. so,
            //we need to turn on all flags to force driver leave ps state. 
            pAdapter->PSMode = Ndis802_11PowerModeCAM;  
            pAdapter->psState = PS_STATE_FULL_POWER;
            pAdapter->bPSConfirm=FALSE; 

            pAdapter->bNullFrameSent = 0;
            pAdapter->TxLock = 0;

            pAdapter->WaitEventType |= MRVL_SOFT_INT_ChipReset;
            Signal_Pending_OID(pAdapter);
        
            if(pAdapter->MediaConnectStatus == NdisMediaStateConnected)
                HandleMACEvent(pAdapter,MACREG_INT_CODE_LINK_LOSE_NO_SCAN);  
        }
         if(pAdapter->SoftIntStatus & MRVL_SOFT_INT_AutoDeepSleep)
         {                                        
            OID_MRVL_DS_DEEP_SLEEP ds;
 
            DBGPRINT(DBG_OID|DBG_PS|DBG_HELP,(L"Post handle Chip Reset\r\n"));
            pAdapter->SoftIntStatus &= ~MRVL_SOFT_INT_AutoDeepSleep; 
                                                             
            ds.ulEnterDeepSleep = 1;  //enter deep sleep
            ProcessOIDDeepSleep(pAdapter,0xff,sizeof(OID_MRVL_DS_DEEP_SLEEP),(PVOID)&ds,HostCmd_PENDING_ON_NONE);  //051407
            pAdapter->IsAutoDeepSleep = 0; //051407
         }

 
        if ( pAdapter->SoftIntStatus & MRVL_SOFT_INT_TxRequest )
        {
            if ( pAdapter->SentPacket == NULL )
            {
                SendOneQueuedPacket( pAdapter );
            }
            else
            {
                DBGPRINT( DBG_WARNING, (L"Packet is sending... skip this TX request\n" ));
            }

            pAdapter->SoftIntStatus &= ~((ULONG)MRVL_SOFT_INT_TxRequest);
        }

    }
                                            
    EnterCriticalSection(&pAdapter->IntCriticalSection);   
    ucLHostIntStatus = pAdapter->ucGHostIntStatus;
    pAdapter->ucGHostIntStatus &= ~ucLHostIntStatus;
    LeaveCriticalSection(&pAdapter->IntCriticalSection);   


     if ( pAdapter->SurpriseRemoved == TRUE )
     {
         DBGPRINT(DBG_ISR,(L"MrvDrvSdioIntTimer: Adapter already removed!\n"));
         return;
     }
      

     //////////////////////////////////////////////////////
     // HostIntStatus &0x1
     if( ucLHostIntStatus & UPLOAD_HOST_INT_STATUS_RDY )
     {
          IF_API_STATUS   ifStatus;

          ifStatus = SDIOGetPktTypeAndLength(pAdapter, &ucCardStatus, &macEvent, &usLength, &p_pkt);

          if ( !IF_IS_SUCCESS(ifStatus) )
          {  
             DBGPRINT(DBG_ERROR,(L"Read Data lenth/type/blcok fail....\r\n")); 
             goto UldErrHdl; 
          }

          pRxBufVM = (PVOID)((ULONG)p_pkt + MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE);

          DBGPRINT(DBG_ERROR,(L"Read Data lenth/type/blcok:%x,%x,%x\r\n",ucCardStatus,macEvent,usLength)); 

          // process the interrupt type
          switch ( ucCardStatus ) 
          {
            case IF_DATA_PKT:
               //051107 Coverity chagne to mainstream //060407
               if( pRxBufVM != NULL)
               {  
                 if((*((UCHAR *)((ULONG)pRxBufVM + 14)) == 0xaa ) &&  
                              (*((UCHAR *)((ULONG)pRxBufVM + 15)) == 0xaa ) &&
                              (*((UCHAR *)((ULONG)pRxBufVM + 16)) == 0x03 ) &&
                              (*((UCHAR *)((ULONG)pRxBufVM + 20)) == 0x88 ) &&
                              (*((UCHAR *)((ULONG)pRxBufVM + 21)) == 0x8e ) )
                           {        
                    //RETAILMSG(1,(TEXT("EAPOL Key: Packet is a Rx DATA! \r\n")));
                    usLength -= 8;
                    //NdisMoveMemory( ttBuf, pAdapter->pRxBuf+20, pAdapter->ulRxSize-20);    
                    NdisMoveMemory((PUCHAR)((ULONG)pRxBufVM + 12),(PUCHAR)((ULONG)pRxBufVM + 20), (usLength-20));   
                               }
               }
                 //082707                 
                 pAdapter->ulRxSize = usLength-(MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE-4);

                 DBGPRINT(DBG_RX|DBG_HELP,(L"Offset:%x\r\n",pAdapter->pRxPD1->PktPtr));
                 HexDump(DBG_RX|DBG_HELP,"Rx Packet Content:",(PUCHAR)pRxBufVM,pAdapter->ulRxSize);
 
      
                 if(pAdapter->bPSConfirm==FALSE)
                     HandleRxReadyEvent(pAdapter);
                 else if(pAdapter->bPSConfirm==TRUE)
                     DBGPRINT(DBG_ERROR,(L"[Marvell]MrvDrvSdioIntTimerHandler: Error => Receive RxReadyEvent()"));

                {
                    UCHAR   *bufpt = ((UCHAR*)pRxBufVM)+14;
                    wlan_ccx_RxPktFilter(pAdapter, bufpt);
                }
                 break;
                                       
            case IF_CMD_PKT:
                 if (pAdapter->CurCmd == NULL)
                 {
                    p_pkt = NULL;
                    return;
                 }
                 if(pAdapter->bPSConfirm==FALSE)
                 {
                     if(pAdapter->CurCmd != NULL)
                     {
                       // still making 1 buffer copy, need to improve on this
                       NdisMoveMemory(pAdapter->CurCmd->BufVirtualAddr, p_pkt->Code, usLength);
                     }  
                     HandleCommandFinishedEvent(pAdapter); 

                 }
                 else if(pAdapter->bPSConfirm==TRUE)
                 {
                    DBGPRINT(DBG_ERROR,(L"[Marvell]MrvDrvSdioIntTimerHandler: Error => Receive CommandFinishedEvent()"));
                 }       
                  break;
               case IF_TX_DONE_EVENT:
                    RETAILMSG(1,(TEXT("MrvDrvSdioIntTimer: Packet is a Tx done Pkt! \r\n")));
                    //dralee, 072705, for integrity, only for Hw versin early than B0 
                    if(pAdapter->SentPacket)
                    {          
                      if(pAdapter->bPSConfirm==FALSE)
                      HandleTxSingleDoneEvent(pAdapter);
                    }
                    //HandleTxSingleDoneEvent(pAdapter);
                  break;
               case IF_MAC_EVENT:
                    //dralee 072705
                    //if((pAdapter->bPSConfirm==TRUE)&&(macEvent==MACREG_INT_CODE_PS_AWAKE))
                    if (pAdapter->bPSConfirm == FALSE)
                            HandleMACEvent(pAdapter,macEvent);
                    else if(macEvent==MACREG_INT_CODE_PS_AWAKE || macEvent==MACREG_INT_CODE_HOST_SLEEP_AWAKE)
                    {
                            //RETAILMSG(1,(L"Rx Evts in Ps state:%x\r\n",macEvent));
                            pAdapter->bPSConfirm=FALSE; 
                            HandleMACEvent(pAdapter,macEvent);
                    }
                    else if (macEvent==MACREG_INT_CODE_BG_SCAN_REPORT)  //For under PS
                    {
                            HandleMACEvent(pAdapter,macEvent);
                    }
                  if(pAdapter->plusDebug)
                     DBGPRINT(DBG_V9, (L"IF_MAC_DONE \n"));
                  
                  break;
               default:
                  // SHOULD not be here
                  DBGPRINT(DBG_ERROR, (L"MrvDrvSdioIntTimer: ERROR, received unexpected type from FW: %d\n", pkt.Type));
                  ASSERT(FALSE);
                 break;
          }// switch ( ucCardStatus )

		UldErrHdl:;
		//041807
		if(pAdapter->pRxCurPkt && pAdapter->isPktPending == FALSE)
		{ 
             ReturnRxPacketDesc(pAdapter,pAdapter->pRxCurPkt);
             pAdapter->pRxCurPkt = NULL;
		}
	} //HostIntStatus &0x1 
 

     /////////////////////////////////////////////////////
     //HostIntStatus & 0x2 
     if( ucLHostIntStatus & DOWNLOAD_HOST_INT_STATUS_RDY)
     {     
         if(pAdapter->SentPacket)
         {          
            //if(pAdapter->bPSConfirm==FALSE)  //++dralee_20060327
               HandleTxSingleDoneEvent(pAdapter);
         }
         //else
            //RETAILMSG(1,(TEXT("MrvDrvSdioIstThread: Packet is NULL \r\n")));
     }

   } // end of while loop  
}

