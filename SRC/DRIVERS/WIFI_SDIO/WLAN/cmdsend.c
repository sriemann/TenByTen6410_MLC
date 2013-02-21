 /*******************  Marvell Semiconductor, Inc., ***************************
 *
 *  Purpose:    This module has implmentation of station command sending 
 *              function
 *
 *  $Author: schiu $
 *
 *  $Date: 2005/01/19 $
 *
 *  $Revision: #17 $
 *
 *****************************************************************************/

#include "precomp.h"

static UCHAR WMM_IE[WMM_IE_LENGTH] = {0xDD,0x07,0x00,0x50,0xf2,0x02,0x00,0x01,0x00};
int SetupKeyMaterial(
    PMRVDRV_ADAPTER Adapter, 
    PHostCmd_DS_802_11_KEY_MATERIAL  pKeyMaterial, 
    USHORT option, 
    USHORT enable, 
    void *InformationBuffer);


NDIS_STATUS PrepareScanCommand(PMRVDRV_ADAPTER Adapter, 
                               USHORT NumberOfChannelPerScan,
                               USHORT NumberOfScanIteration,
                               PUSHORT pCmdIndex,
                               PUSHORT pChannelIndex,
                               PREGION_CHANNEL pRegionChannel);


/******************************************************************************
 *
 *  Name: PrepareAndSendCommand()
 *
 *  Description: Prepare and send host command to station
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:
 *      NDIS_STATUS_RESOURCES
 *      NDIS_STATUS_SUCCESS
 * 
 *  Notes: Driver will prepare and send the command to station
 *
 *****************************************************************************/
NDIS_STATUS
PrepareAndSendCommand(
  IN PMRVDRV_ADAPTER Adapter,
  IN USHORT Cmd,
  IN USHORT CmdOption,
  IN USHORT INTOption,
  IN  NDIS_OID PendingOID,
  IN  USHORT PendingInfo,
  IN  USHORT PriorityPass,
  IN  BOOLEAN IsLastBatchCmd,
  IN  PULONG BytesWritten,
  IN  PULONG BytesRead,
  IN  PULONG BytesNeeded,
  IN  PVOID InformationBuffer
)
{
	NDIS_STATUS		retval;
  USHORT Size = 0;
  PHostCmd_DS_GEN pCmdPtr;
  UCHAR ucTemp;
  CmdCtrlNode *pTempCmd;
  NDIS_STATUS Status;
  PNDIS_802_11_WEP pNewWEP;

  PHostCmd_DS_GET_HW_SPEC              pHWSpec;
  PHostCmd_DS_802_11_RESET             pReset;
  PHostCmd_DS_MAC_CONTROL              pControl;
  PHostCmd_DS_MAC_MULTICAST_ADR        pMCastAdr;
  PHostCmd_DS_802_11_DATA_RATE         pDataRate;
  PHostCmd_DS_802_11_PS_MODE           pPSMode;
  PHostCmd_DS_802_11_SET_WEP           pSetWEP;
  PHostCmd_DS_802_11_SNMP_MIB          pSNMPMIB;
  PHostCmd_DS_802_11_AUTHENTICATE    pAuthenticate;
  PHostCmd_DS_802_11_ASSOCIATE_EXT     pAsso;
  PHostCmd_DS_802_11_RF_ANTENNA    pAntenna;
  PHostCmd_DS_802_11_RF_TX_POWER       pRTP;
  PHostCmd_DS_802_11_DEAUTHENTICATE  pDeAuthenticate;
  PHostCmd_DS_802_11_AD_HOC_JOIN     pAdHocJoin;
  PHostCmd_DS_802_11_AD_HOC_START    pAdHocStart;
  PHostCmd_DS_802_11_RADIO_CONTROL   pRadioControl;
  PHostCmd_DS_802_11_RF_CHANNEL      pRFChannel;
  PHostCmd_DS_802_11D_DOMAIN_INFO      pDomainInfo;
  PHostCmd_DS_802_11_KEY_MATERIAL      pKeyMaterial;
        PHostCmd_DS_802_11_WMM_ACK_POLICY    pWmmAckPolicy;
        PHostCmd_DS_802_11_WMM_GET_STATUS    pWmmGetStatus;
        POID_MRVL_DS_WMM_ACK_POLICY          pAckPolicy;

       PHostCmd_DS_802_11_RSSI          prssi;
    BOOLEAN                            timerStatus;

	if( Adapter->SurpriseRemoved == TRUE ) {
		retval = NDIS_STATUS_ADAPTER_REMOVED;
		goto funcFinal;
	}
  // Get next free command control node
  
    //051407
    if( IsThisDsState(Adapter, DS_STATE_SLEEP) )
    {                                        
       DBGPRINT(DBG_OID|DBG_PS|DBG_HELP,(L"Exit DeepSleep in prepareandsendcommand()\r\n"));
       If_PowerUpDevice(Adapter);  
    }
    pTempCmd = GetFreeCmdCtrlNode(Adapter);
  
	if( !pTempCmd) 
	{
		ResetCmdBuffer(Adapter);
		retval = NDIS_STATUS_RESOURCES;
		goto funcFinal;
	}

  pTempCmd->ExpectedRetCode = GetExpectedRetCode(Cmd);
  pTempCmd->Next = NULL;

  // Set other command information
  SetCmdCtrlNode( 
      Adapter,
      pTempCmd, 
      PendingOID, 
      PendingInfo, 
      INTOption, 
      PriorityPass, 
      IsLastBatchCmd, 
      BytesWritten,
      BytesRead,
      BytesNeeded, 
      InformationBuffer
      );

  pCmdPtr = (PHostCmd_DS_GEN)pTempCmd->BufVirtualAddr;

  // Set sequnece number, command and INT option
  Adapter->SeqNum++;
  pCmdPtr->SeqNum = Adapter->SeqNum;
  pCmdPtr->Command = Cmd;
  pCmdPtr->Result = INTOption;


  // Check and prepare command
  switch( Cmd ) {







    case HostCmd_CMD_802_11_BG_SCAN_CONFIG:
        {
            POID_MRVL_DS_BG_SCAN_CONFIG   pBgCfg;
            PHostCmd_DS_802_11_BG_SCAN_CONFIG   pHostCmdBgScanCfg;
    
                    pBgCfg = (POID_MRVL_DS_BG_SCAN_CONFIG) Adapter->BgScanCfg;
            pHostCmdBgScanCfg =     (PHostCmd_DS_802_11_BG_SCAN_CONFIG)(pTempCmd->BufVirtualAddr);

            NdisMoveMemory( (PUCHAR)(&(pHostCmdBgScanCfg->Action)), 
                               (PUCHAR)(&(pBgCfg->Action)), 
                               pBgCfg->CmdLen); 

            Size = pBgCfg->CmdLen + sizeof(USHORT)*4;       

            // Set command size
                     pCmdPtr->Size = Size;
        }

        break;
        
        case HostCmd_CMD_802_11_BG_SCAN_QUERY:
        { 
            PHostCmd_DS_802_11_BG_SCAN_QUERY pThisCmd; 
            POID_MRVL_DS_BG_SCAN_QUERY pOid; 

            pThisCmd = (PHostCmd_DS_802_11_BG_SCAN_QUERY) pCmdPtr; 
            Size = sizeof(HostCmd_DS_802_11_BG_SCAN_QUERY); 
            if ( CmdOption == HostCmd_ACT_GEN_SET ) 
            { 
                pOid = (POID_MRVL_DS_BG_SCAN_QUERY) InformationBuffer; 
                memcpy(((PUCHAR)&(pThisCmd->Flush)),((PUCHAR)&(pOid->Flush)),(sizeof(OID_MRVL_DS_BG_SCAN_QUERY) - sizeof(NDIS_OID))); 
                DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- GET_OID " L"HostCmd_DS_802_11_BG_SCAN_QUERY" L"." L"Flush" L"=0x%x\n", pThisCmd->Flush)); 
                DBGPRINT(DBG_OID|DBG_HELP, (L"     [0x%x] [0x%x] [%d] [TotalLen=%d]\n", pThisCmd, &(pThisCmd->Flush), ((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_BG_SCAN_QUERY *)0)->Flush)), sizeof(OID_MRVL_DS_BG_SCAN_QUERY)-sizeof(NDIS_OID))); 
            } 
            else 
                DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- GET_OID " L"HostCmd_DS_802_11_BG_SCAN_QUERY" L"...\n")); 
            }

        break;

    case HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT:
        {
            POID_MRVL_DS_POWER_ADAPT_CFG_EXT   pPACfg;
            PHostCmd_DS_802_11_POWER_ADAPT_CFG_EXT   pHostCmdPACfg;
    
            pPACfg = (POID_MRVL_DS_POWER_ADAPT_CFG_EXT) Adapter->PACfg;
            pHostCmdPACfg =     (PHostCmd_DS_802_11_POWER_ADAPT_CFG_EXT)(pTempCmd->BufVirtualAddr);

            NdisMoveMemory( (PUCHAR)(&(pHostCmdPACfg->Action)), 
                               (PUCHAR)(&(pPACfg->Action)), 
                               pPACfg->CmdLen); 

            Size = pPACfg->CmdLen + sizeof(USHORT)*4;       

            // Set command size
            pCmdPtr->Size = Size;
                    
            DBGPRINT(DBG_CMD|DBG_OID|DBG_HELP,(L"[Marvell]HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT:pHostCmdPACfg->Action= %d,pHostCmdPACfg->EnablePA= %d,pCmdPtr->Size=0x%x\r\n",pHostCmdPACfg->Action,pHostCmdPACfg->EnablePA,pCmdPtr->Size));
        }
        break;

    case HostCmd_CMD_802_11_SUBSCRIBE_EVENT:
        {
            PHostCmd_DS_802_11_SUBSCRIBE_EVENT    pCmdSubscribeEvent;

            pCmdSubscribeEvent = (PHostCmd_DS_802_11_SUBSCRIBE_EVENT)pCmdPtr;

            Size = sizeof(HostCmd_DS_802_11_SUBSCRIBE_EVENT);
            pCmdSubscribeEvent->Size = sizeof(HostCmd_DS_802_11_SUBSCRIBE_EVENT);

            /* Fill the command node with data in private structure */
            pCmdSubscribeEvent->Action = CmdOption;
            pCmdSubscribeEvent->Events = Adapter->EventRecord.EventMap;

            pCmdSubscribeEvent->RssiLow.Header.Type = TLV_TYPE_RSSI;
            pCmdSubscribeEvent->RssiLow.Header.Len  = TLV_PAYLOAD_SIZE;
            pCmdSubscribeEvent->RssiLow.RSSIValue   = Adapter->EventRecord.RSSILowValue;
            pCmdSubscribeEvent->RssiLow.RSSIFreq    = Adapter->EventRecord.RSSILowFreq;

            pCmdSubscribeEvent->SnrLow.Header.Type = TLV_TYPE_SNR;
            pCmdSubscribeEvent->SnrLow.Header.Len  = TLV_PAYLOAD_SIZE;
            pCmdSubscribeEvent->SnrLow.SNRValue    = Adapter->EventRecord.SNRLowValue;
            pCmdSubscribeEvent->SnrLow.SNRFreq     = Adapter->EventRecord.SNRLowFreq;

            pCmdSubscribeEvent->FailCnt.Header.Type = TLV_TYPE_FAILCOUNT;
            pCmdSubscribeEvent->FailCnt.Header.Len  = TLV_PAYLOAD_SIZE;
            pCmdSubscribeEvent->FailCnt.FailValue   = Adapter->EventRecord.FailValue;
            pCmdSubscribeEvent->FailCnt.FailFreq    = Adapter->EventRecord.FailFreq;

            pCmdSubscribeEvent->BcnMiss.Header.Type  = TLV_TYPE_BCNMISS;
            pCmdSubscribeEvent->BcnMiss.Header.Len   = TLV_PAYLOAD_SIZE;
            pCmdSubscribeEvent->BcnMiss.BeaconMissed = Adapter->EventRecord.BeaconMissed;
            pCmdSubscribeEvent->BcnMiss.Reserved     = Adapter->EventRecord.Reserved;

         
            pCmdSubscribeEvent->RssiHigh.Header.Type = TLV_TYPE_RSSI_HIGH;
            pCmdSubscribeEvent->RssiHigh.Header.Len  = TLV_PAYLOAD_SIZE;
            pCmdSubscribeEvent->RssiHigh.RSSIValue   = Adapter->EventRecord.RSSIHighValue;
            pCmdSubscribeEvent->RssiHigh.RSSIFreq    = Adapter->EventRecord.RSSIHighFreq;

            pCmdSubscribeEvent->SnrHigh.Header.Type = TLV_TYPE_SNR_HIGH;
            pCmdSubscribeEvent->SnrHigh.Header.Len  = TLV_PAYLOAD_SIZE;
            pCmdSubscribeEvent->SnrHigh.SNRValue    = Adapter->EventRecord.SNRHighValue;
            pCmdSubscribeEvent->SnrHigh.SNRFreq     = Adapter->EventRecord.SNRHighFreq;
            
        }
        break;


    case HostCmd_CMD_802_11_LED_CONTROL:
        TT_CMDPARSE_SEND_CMD( HostCmd_DS_802_11_LED_CONTROL, OID_MRVL_DS_LED_CONTROL, NumLed );
        break;
        
    case HostCmd_CMD_802_11_CAL_DATA_EXT:
       { 
            PHostCmd_DS_802_11_CAL_DATA_EXT pThisCmd; 
            POID_MRVL_DS_CAL_DATA_EXT pOid; 

            pThisCmd = (PHostCmd_DS_802_11_CAL_DATA_EXT) pCmdPtr; 
            Size = sizeof(HostCmd_DS_802_11_CAL_DATA_EXT); 
            pThisCmd->Action = CmdOption; 
            if ( CmdOption == HostCmd_ACT_GEN_SET ) 
            { 
               pOid = (POID_MRVL_DS_CAL_DATA_EXT) InformationBuffer; 
               memcpy(((PUCHAR)&(pThisCmd->Revision)),((PUCHAR)&(pOid->Revision)),(sizeof(OID_MRVL_DS_CAL_DATA_EXT) - sizeof(NDIS_OID))); 
               DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- SET_OID " L"HostCmd_DS_802_11_CAL_DATA_EXT" L"." L"Revision" L"=0x%x\n", pThisCmd->Revision)); 
               DBGPRINT(DBG_OID|DBG_HELP, (L"     [0x%x] [0x%x] [%d] [TotalLen=%d]\n", pThisCmd, &(pThisCmd->Revision), ((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_CAL_DATA_EXT *)0)->Revision)), sizeof(OID_MRVL_DS_CAL_DATA_EXT)-sizeof(NDIS_OID))); 
            } 
             else 
                 DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- GET_OID " L"HostCmd_DS_802_11_CAL_DATA_EXT" L"...\n")); 
        }
        break;

    case HostCmd_CMD_802_11_PWR_CFG:
        TT_CMDPARSE_SEND_CMD( HostCmd_DS_802_11_PWR_CFG, OID_MRVL_DS_PWR_CFG, Enable );
        break;

    case HostCmd_CMD_802_11_TPC_CFG:
       { 
          PHostCmd_DS_802_11_TPC_CFG pThisCmd; 
          POID_MRVL_DS_TPC_CFG pOid; 
        
          pThisCmd = (PHostCmd_DS_802_11_TPC_CFG) pCmdPtr; 
          Size = sizeof(HostCmd_DS_802_11_TPC_CFG); 
        pThisCmd->Action = CmdOption; 
        if ( CmdOption == HostCmd_ACT_GEN_SET ) 
        { 
            pOid = (POID_MRVL_DS_TPC_CFG) InformationBuffer; 
            memcpy(((PUCHAR)&(pThisCmd->Enable)),((PUCHAR)&(pOid->Enable)),(sizeof(OID_MRVL_DS_TPC_CFG) - sizeof(NDIS_OID))); 
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- SET_OID " L"HostCmd_DS_802_11_TPC_CFG" L"." L"Enable" L"=0x%x\n", pThisCmd->Enable)); 
            DBGPRINT(DBG_OID|DBG_HELP, (L"     [0x%x] [0x%x] [%d] [TotalLen=%d]\n", pThisCmd, &(pThisCmd->Enable), ((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_TPC_CFG *)0)->Enable)), sizeof(OID_MRVL_DS_TPC_CFG)-sizeof(NDIS_OID))); 
        } 
        else 
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- GET_OID " L"HostCmd_DS_802_11_TPC_CFG" L"...\n"));
        }
        break;

       case HostCmd_CMD_802_11_RATE_ADAPT_RATESET:
        {
           PHostCmd_RATE_ADAPT_RATESET pThisCmd; 
           POID_MRVL_DS_RATE_ADAPT_RATESET pOid; 

           pThisCmd = (PHostCmd_RATE_ADAPT_RATESET) pCmdPtr; 
           Size = sizeof(HostCmd_RATE_ADAPT_RATESET); 
           pThisCmd->Action = CmdOption; 
            if ( CmdOption == HostCmd_ACT_GEN_SET )
            {
              pOid = (POID_MRVL_DS_RATE_ADAPT_RATESET) InformationBuffer; 
              memcpy(((PUCHAR)&(pThisCmd->EnableHwAuto)),((PUCHAR)&(pOid->EnableHwAuto)),sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET)); 
              DBGPRINT(DBG_OID|DBG_HELP, (L"DropMode:%x, BitMap:%x, Threshold:%x,FinalRate:%x\r\n",
                       pThisCmd->EnableHwAuto, pThisCmd->Bitmap,pThisCmd->Threshold,pThisCmd->FinalRate));
            }
        }
        break;
       
        case HostCmd_CMD_802_11_GET_STAT:
        {
           Size = sizeof(HostCmd_DS_802_11_GET_STAT); 	 	   
        }
        break;
 

  case HostCmd_CMD_802_11_DEEP_SLEEP:
       //012207
       SetDsState( Adapter, DS_STATE_PRESLEEP );
       Size = sizeof(HostCmd_DS_802_11_DEEP_SLEEP);
       break;

  case HostCmd_CMD_802_11_HOST_SLEEP_CFG:
    { 
      PHostCmd_DS_HOST_SLEEP pCmdHostSleep;
      POID_MRVL_DS_HOST_SLEEP_CFG   pOIDHostSleep; 
      int len;

      pOIDHostSleep = (POID_MRVL_DS_HOST_SLEEP_CFG)InformationBuffer;
      pCmdHostSleep= (PHostCmd_DS_HOST_SLEEP)pCmdPtr;
      pCmdHostSleep->Criteria=pOIDHostSleep->ulCriteria;
      pCmdHostSleep->GPIO=pOIDHostSleep->ucGPIO;
      pCmdHostSleep->Gap=pOIDHostSleep->ucGap;
                       
      Size = sizeof(HostCmd_DS_HOST_SLEEP)-sizeof(HostCmd_DS_HOST_WAKEUP_FILTER);      

      //dralee_20060606
      if( pCmdHostSleep->Criteria != -1 && pOIDHostSleep->Filter.Header.Type == Host_Sleep_Filter_Type)
      {
            //dralee_20060606
            len = (pOIDHostSleep->Filter.Header.Len + sizeof(MrvlIEtypesHeader_t));
            NdisMoveMemory( (UCHAR *)&pCmdHostSleep->Filter.Header.Type,
                            (UCHAR *)&pOIDHostSleep->Filter.Header.Type, 
                            len ); 
            Size = sizeof(HostCmd_DS_HOST_SLEEP)-sizeof(HostCmd_DS_HOST_WAKEUP_FILTER)+ len;
            DBGPRINT(DBG_CMD|DBG_HOSTSLEEP|DBG_HELP,(L"v8 hostsleepcfg cmd:%x,%x,%x,%x,%x,%x\r\n",
                           pCmdHostSleep->Criteria,
                           pCmdHostSleep->Filter.Header.Type,
                           pOIDHostSleep->Filter.Header.Len,
                           pOIDHostSleep->Filter.entry[0].AddressType,
                           pOIDHostSleep->Filter.entry[0].EthType,
                           pOIDHostSleep->Filter.entry[0].Ipv4Addr
                          ));
      }
     
      DBGPRINT(DBG_HOSTSLEEP|DBG_CMD|DBG_HELP, (L"HostCmd_CMD_802_11_HOST_SLEEP_CFG:pCmdHostSleep->Criteria=0x%x\n,pCmdHostSleep->GPIO=0x%x\n,pCmdHostSleep->Gap =0x%x,pCmdHostSleep->Size =0x%x\n", pCmdHostSleep->Criteria ,pCmdHostSleep->GPIO,pCmdHostSleep->Gap,pCmdHostSleep->Size));                  
    }
    break;
    
  case HostCmd_CMD_802_11_HOST_SLEEP_AWAKE_CONFIRM:
    Size = sizeof( HostCmd_DS_WAKEUP_CONFIRM);
    break; 

  case HostCmd_CMD_802_11_SLEEP_PARAMS:
        {
            PHostCmd_DS_802_11_SLEEP_PARAMS    pCmdSleepParams;
            POID_MRVL_DS_SLEEP_PARAMS          pOIDSleep_Params; 

            pCmdSleepParams= (PHostCmd_DS_802_11_SLEEP_PARAMS)pCmdPtr; 
            Size = sizeof(HostCmd_DS_802_11_SLEEP_PARAMS);
            if(CmdOption ==HostCmd_ACT_SET)
            {    
              pOIDSleep_Params = (POID_MRVL_DS_SLEEP_PARAMS)InformationBuffer;
              pCmdSleepParams->Action=HostCmd_ACT_SET;
              pCmdSleepParams->Error=pOIDSleep_Params->Error;
              pCmdSleepParams->Offset=pOIDSleep_Params->Offset;
              pCmdSleepParams->StableTime=pOIDSleep_Params->StableTime;
              pCmdSleepParams->CalControl=pOIDSleep_Params->CalControl;
              pCmdSleepParams->ExternalSleepClk=pOIDSleep_Params->ExternalSleepClk;
              pCmdSleepParams->Reserved=pOIDSleep_Params->Reserved;
               
              DBGPRINT(DBG_OID|DBG_PS|DBG_HOSTSLEEP|DBG_HELP, (L"Set HostCmd_CMD_802_11_SLEEP_PARAMS:Size=%d\n, pCmdSleepParams->Error=%d\n,pCmdSleepParams->Offset=%d\n,pCmdHostWakeup->Gap =%d\n,pCmdSleepParams->CalControl=%d\n,pCmdSleepParams->ExternalSleepClk=%d\n,pCmdSleepParams->Reserved=%d\n",Size, pCmdSleepParams->Error ,pCmdSleepParams->Offset,pCmdSleepParams->StableTime,pCmdSleepParams->CalControl,pCmdSleepParams->ExternalSleepClk,pCmdSleepParams->Reserved));                         
            }else if(CmdOption ==HostCmd_ACT_GET)
            {
                  pCmdSleepParams->Action=HostCmd_ACT_GET;
                
            }
        }
        break;


        // TX and RX commands are the same
        case HostCmd_CMD_TEST_RX_MODE:
        case HostCmd_CMD_TEST_TX_MODE:
            {
                POID_MRVL_DS_TX_MODE        pUserBuffer;
                PHostCmd_DS_CMD_TXRX_MODE   pCmd;

                // set only
                if ( CmdOption != HostCmd_ACT_SET ) {
				retval = NDIS_STATUS_NOT_ACCEPTED;
				goto funcFinal;
                	}

                // TX and RX mode command does not check for valid mode value
                // Check should be done before calling this function
                pCmd = (PHostCmd_DS_CMD_TXRX_MODE)pCmdPtr;
                pUserBuffer = (POID_MRVL_DS_TX_MODE) InformationBuffer;
                pCmd->Mode = (UCHAR)pUserBuffer->ulMode;
                Size = sizeof(HostCmd_DS_CMD_TXRX_MODE);
            }
            break;

        case HostCmd_CMD_REGION_CODE:
            {
                POID_MRVL_DS_REGION_CODE    pUserBuffer;
                PHostCmd_DS_CMD_REGION_CODE pCmd;

                pCmd = (PHostCmd_DS_CMD_REGION_CODE)pCmdPtr;
                pCmd->Action = CmdOption;

        if(pCmd->Action==HostCmd_ACT_SET)
        {
                 pUserBuffer = (POID_MRVL_DS_REGION_CODE)InformationBuffer;
                 pCmd->RegionCode = (USHORT)(pUserBuffer->usRegionCode);
              }
               
                Size = sizeof(HostCmd_DS_CMD_REGION_CODE);

            }
            break;

        case HostCmd_CMD_MAC_ADDRESS:
            {
                PHostCmd_DS_CMD_MAC_ADDRESS     pCmd;
                POID_MRVL_DS_MAC_ADDRESS    pUserBuffer;

        DBGPRINT(DBG_ALLEN,(L"***+ HostCmd_CMD_MAC_ADDRES\n"));  
                pCmd = (PHostCmd_DS_CMD_MAC_ADDRESS)pCmdPtr;
                pCmd->Action = CmdOption;
                pUserBuffer = (POID_MRVL_DS_MAC_ADDRESS)InformationBuffer;
                NdisMoveMemory( pCmd->MacAddress,
                                pUserBuffer->EthAddr,
                                MRVDRV_ETH_ADDR_LEN);
                Size = sizeof(HostCmd_DS_CMD_MAC_ADDRESS);       
            }
            break;

        case HostCmd_CMD_BBP_REG_ACCESS:
            {
                PHostCmd_DS_CMD_BBP_REG_ACCESS pCmd;
                POID_MRVL_DS_BBP_REGISTER_ACCESS pUserBuffer;

                pCmd = (PHostCmd_DS_CMD_BBP_REG_ACCESS)pCmdPtr;
                pCmd->Action = CmdOption;

                pUserBuffer = (POID_MRVL_DS_BBP_REGISTER_ACCESS)InformationBuffer;
                pCmd->Offset = pUserBuffer->usOffset;

                // no harm in setting the value if it is a read
                pCmd->Value = (UCHAR)pUserBuffer->ulValue; 

                Size = sizeof(HostCmd_DS_CMD_BBP_REG_ACCESS);
            }
            break;

        case HostCmd_CMD_MAC_REG_ACCESS:
            {
                PHostCmd_DS_CMD_MAC_REG_ACCESS pCmd;
                POID_MRVL_DS_MAC_REGISTER_ACCESS pUserBuffer;

                pCmd = (PHostCmd_DS_CMD_MAC_REG_ACCESS)pCmdPtr;
                pCmd->Action = CmdOption;

                pUserBuffer = (POID_MRVL_DS_MAC_REGISTER_ACCESS)InformationBuffer;
                pCmd->Offset = pUserBuffer->usOffset;

                // no harm in setting the value if it is a read
                pCmd->Value = pUserBuffer->ulValue; 

                Size = sizeof(HostCmd_DS_CMD_MAC_REG_ACCESS);  

                DBGPRINT(DBG_CMD|DBG_HELP,(L"MAC ACCESS:%x,%x,%x\r\n",pCmd->Offset,pCmd->Value,CmdOption));   

            }

            break;

        case HostCmd_CMD_RF_REG_ACCESS:
            {
                PHostCmd_DS_CMD_RF_REG_ACCESS pCmd;
                POID_MRVL_DS_RF_REGISTER_ACCESS pUserBuffer;

                pCmd = (PHostCmd_DS_CMD_RF_REG_ACCESS)pCmdPtr;
                pCmd->Action = CmdOption;

                pUserBuffer = (POID_MRVL_DS_RF_REGISTER_ACCESS)InformationBuffer;
                pCmd->Offset = pUserBuffer->usOffset;

                // no harm in setting the value if it is a read
                pCmd->Value = (UCHAR)pUserBuffer->ulValue; 
                Size = sizeof(HostCmd_DS_CMD_RF_REG_ACCESS);
            }

            break;
            
    case HostCmd_CMD_802_11_RSSI:
     {
    
         Size = (USHORT)(sizeof(HostCmd_DS_802_11_RSSI));
      prssi = (PHostCmd_DS_802_11_RSSI)pCmdPtr;
      prssi->N=(SHORT)Adapter->bcn_avg_factor;

      /* reset Beacon SNR/NF/RSSI values */
     Adapter->SNR[TYPE_BEACON][TYPE_NOAVG] = 0;
     Adapter->SNR[TYPE_BEACON][TYPE_AVG] = 0;
     Adapter->NF[TYPE_BEACON][TYPE_NOAVG] = 0;
     Adapter->NF[TYPE_BEACON][TYPE_AVG] = 0;
     Adapter->RSSI[TYPE_BEACON][TYPE_NOAVG] = 0;
     Adapter->RSSI[TYPE_BEACON][TYPE_AVG] = 0;
    } 
      break;

    case HostCmd_CMD_802_11_GET_LOG:
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_GET_LOG));
      break;

    case HostCmd_CMD_GET_HW_SPEC:
      Size = (USHORT)(sizeof(HostCmd_DS_GET_HW_SPEC));
      pHWSpec = (PHostCmd_DS_GET_HW_SPEC)pCmdPtr;
      
      // Move Adapter->CurrentAddr to address field
      NdisMoveMemory(pHWSpec->PermanentAddr, Adapter->CurrentAddr, MRVDRV_ETH_ADDR_LEN);
    
      DBGPRINT(DBG_CMD,(L"*** set f/w permanent addr : %2x %2x %2x %2x %2x %2x ***\n", 
        pHWSpec->PermanentAddr[0],
        pHWSpec->PermanentAddr[1],
        pHWSpec->PermanentAddr[2],
        pHWSpec->PermanentAddr[3],
        pHWSpec->PermanentAddr[4],
        pHWSpec->PermanentAddr[5]));        
    break;
  
    case HostCmd_CMD_802_11_RESET:
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_RESET));
      pReset = (PHostCmd_DS_802_11_RESET)pCmdPtr;
    break;
  
    case HostCmd_CMD_MAC_CONTROL:
      Size = (USHORT)(sizeof(HostCmd_DS_MAC_CONTROL));
      pControl = (PHostCmd_DS_MAC_CONTROL)pCmdPtr;
      pControl->Action = CmdOption;
    break;
  
    case HostCmd_CMD_MAC_MULTICAST_ADR:
      Size = (USHORT)(sizeof(HostCmd_DS_MAC_MULTICAST_ADR));
      pMCastAdr = (PHostCmd_DS_MAC_MULTICAST_ADR)pCmdPtr;
      pMCastAdr->Action = CmdOption;
      pMCastAdr->NumOfAdrs = (USHORT)Adapter->NumOfMulticastMACAddr;
      
      NdisMoveMemory(
        (PVOID)pMCastAdr->MACList, 
        (PVOID)Adapter->MulticastList,
        Adapter->NumOfMulticastMACAddr*ETH_LENGTH_OF_ADDRESS);
    break;
  
    case HostCmd_CMD_802_11_PS_MODE:
         {
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_PS_MODE));
      pPSMode = (PHostCmd_DS_802_11_PS_MODE)pCmdPtr;

      //pPSMode->Action = 0;
      pPSMode->SubCommand = (USHORT)InformationBuffer;
      //pPSMode->PowerMode = 0;

      pPSMode->PSNumDtims = 0x1;
    }
    break;
  
    case HostCmd_CMD_802_11_DATA_RATE:
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_DATA_RATE));
      pDataRate = (PHostCmd_DS_802_11_DATA_RATE)pCmdPtr;
      
      SetupDataRate(CmdOption, pDataRate, Adapter,InformationBuffer);
    break;

  
    case HostCmd_CMD_802_11_SCAN:
      DBGPRINT(DBG_CMD|DBG_SCAN|DBG_HELP,(L"Enter HostCmd_CMD_802_11_SCAN : 1 : \r\n")); 

      Adapter->SeqNum--; 
          NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
          CleanUpCmdCtrlNode(pTempCmd);
          InsertQNodeAtTail(&Adapter->CmdFreeQ, (PQ_NODE)pTempCmd);
          NdisReleaseSpinLock(&Adapter->FreeQSpinLock);   

      pTempCmd = NULL;



         SetupScanCommand (Adapter );
    
    break;

    case HostCmd_CMD_802_11_SET_WEP:
        pSetWEP = (PHostCmd_DS_802_11_SET_WEP)pCmdPtr;
      pNewWEP = (PNDIS_802_11_WEP)InformationBuffer;

      DBGPRINT(DBG_CMD|DBG_HELP,(L"Enter HostCmd_DS_802_11_SET_WEP : \n"));
      
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_SET_WEP));

      SetupWepKeys(
        PendingOID,
        pSetWEP, 
        pNewWEP,
        InformationBuffer,
        Adapter,
        CmdOption);
  
      break;

    case HostCmd_CMD_802_11_SNMP_MIB:

      switch (PendingOID)
      {
        case OID_802_11_INFRASTRUCTURE_MODE:
        {
          Size = (USHORT)(sizeof(HostCmd_DS_802_11_SNMP_MIB));
          pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pCmdPtr;
          pSNMPMIB->QueryType = HostCmd_ACT_GEN_SET;
          pSNMPMIB->OID = (USHORT)DesiredBssType_i; // InfrastructureMode Index = 0
          pSNMPMIB->BufSize = sizeof(UCHAR);
        
          if( Adapter->InfrastructureMode == Ndis802_11Infrastructure )
            ucTemp = 1; // Infrastructure mode
          else
            ucTemp = 2; // Ad hoc mode
        
          NdisMoveMemory(pSNMPMIB->Value, &ucTemp, sizeof(UCHAR));        
          break;
        }   
      
        case OID_802_11_FRAGMENTATION_THRESHOLD:
        {
          NDIS_802_11_FRAGMENTATION_THRESHOLD ulTemp;
          USHORT usTemp;
        
          Size = (USHORT)(sizeof(HostCmd_DS_802_11_SNMP_MIB));
          pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pCmdPtr;
          pSNMPMIB->OID = (USHORT)FragThresh_i; 
          
          if(PendingInfo==HostCmd_PENDING_ON_GET_OID){
            pSNMPMIB->QueryType = HostCmd_ACT_GEN_GET;
          }
          else if(PendingInfo==HostCmd_PENDING_ON_SET_OID){
            pSNMPMIB->QueryType = HostCmd_ACT_GEN_SET;
            pSNMPMIB->BufSize = sizeof(USHORT);
            ulTemp=*((NDIS_802_11_FRAGMENTATION_THRESHOLD *)InformationBuffer);
            *((PUSHORT)(pSNMPMIB->Value))=(USHORT)ulTemp;
            DBGPRINT(DBG_OID,(L"OID: Setting Fragmentation threshold to %d\n", ulTemp ));
          }
          else if(PendingInfo==HostCmd_PENDING_ON_NONE){
            pSNMPMIB->QueryType = HostCmd_ACT_GEN_SET;
            pSNMPMIB->BufSize = sizeof(USHORT);
            usTemp=(USHORT)Adapter->FragThsd;
            NdisMoveMemory(pSNMPMIB->Value, &usTemp, sizeof(USHORT));
          }                       
          break;          
        } 
      
        case OID_802_11_RTS_THRESHOLD:
        {
          USHORT usTemp;
          NDIS_802_11_RTS_THRESHOLD ulTemp;
        
          Size = (USHORT)(sizeof(HostCmd_DS_802_11_SNMP_MIB));
          pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pCmdPtr;
          pSNMPMIB->OID = (USHORT)RtsThresh_i; 
      
          if(PendingInfo==HostCmd_PENDING_ON_GET_OID){
            pSNMPMIB->QueryType = HostCmd_ACT_GEN_GET;
          }
          else if(PendingInfo==HostCmd_PENDING_ON_SET_OID){
            pSNMPMIB->QueryType = HostCmd_ACT_GEN_SET;
            pSNMPMIB->BufSize = sizeof(USHORT);
            ulTemp=*((NDIS_802_11_RTS_THRESHOLD *)InformationBuffer);
            *((PUSHORT)(pSNMPMIB->Value))=(USHORT)ulTemp;
            DBGPRINT(DBG_OID,(L"OID: Setting RTS threshold to %d\n", ulTemp ));
          }
          else if(PendingInfo==HostCmd_PENDING_ON_NONE){
            pSNMPMIB->QueryType = HostCmd_ACT_GEN_SET;
            pSNMPMIB->BufSize = sizeof(USHORT);
            usTemp=(USHORT)Adapter->RTSThsd;
            NdisMoveMemory(pSNMPMIB->Value, &usTemp, sizeof(USHORT));
          }       
          break;    
        }         
        case OID_802_11D_ENABLE:
        {
          USHORT  usTemp;

          Size = (USHORT)(sizeof(HostCmd_DS_802_11_SNMP_MIB));
          pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pCmdPtr;
          pSNMPMIB->OID = Dot11D_i;

          if( CmdOption == HostCmd_ACT_GEN_SET )
          {
              pSNMPMIB->QueryType = HostCmd_ACT_GEN_SET;
              pSNMPMIB->BufSize = sizeof(USHORT);
              usTemp = *(USHORT*) InformationBuffer;
              *((USHORT *) (pSNMPMIB->Value)) = usTemp;
                  
              DBGPRINT(DBG_11D|DBG_CMD|DBG_HELP,(L"CMD SET enable11d:%x\r\n",usTemp));
 
              if( Adapter->MediaConnectStatus == NdisMediaStateDisconnected )          
                  ResetAllScanTypeAndPower(Adapter);
          }
          else 
          {   
              pSNMPMIB->QueryType = HostCmd_ACT_GEN_GET;
              pSNMPMIB->BufSize = sizeof(USHORT);
          } 
        }
        break;
        default:
          break;
            }     
    break;    

    case HostCmd_CMD_802_11_AUTHENTICATE:
        {
            UCHAR ucAuth;

      Size = (USHORT)(sizeof(HostCmd_DS_802_11_AUTHENTICATE));
      pAuthenticate = (PHostCmd_DS_802_11_AUTHENTICATE) pCmdPtr;

      // if the authentication type need to be determined at initialization time
      if(PendingInfo == HostCmd_PENDING_ON_NONE)
            {
        ucAuth = (UCHAR)Adapter->AuthenticationMode;          
      } 
      else
            {
                ucAuth = (UCHAR) (*((PNDIS_802_11_AUTHENTICATION_MODE) InformationBuffer));
            }

            switch (ucAuth)
            {
            case Ndis802_11AuthModeWPAPSK:
            case Ndis802_11AuthModeWPA:
            case Ndis802_11AuthModeOpen:
      case Ndis802_11AuthModeWPA2:       
      case Ndis802_11AuthModeWPA2PSK: 
                pAuthenticate->AuthType = 0;  // open authentication
                break;

            case Ndis802_11AuthModeShared:
                pAuthenticate->AuthType = 1;  // shared authentication
                break;

            default:
                DBGPRINT(DBG_WARNING, (L"(sendcmd)Received unsupported authentication mode: %d\n",
                    ucAuth));
			retval = NDIS_STATUS_FAILURE;
			goto funcFinal;
            }

      // set AP MAC address
      NdisMoveMemory(pAuthenticate->MacAddr, Adapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
        }
    break;
   
    case HostCmd_CMD_802_11_ASSOCIATE_EXT:
    case HostCmd_CMD_802_11_ASSOCIATE:
      pAsso = (PHostCmd_DS_802_11_ASSOCIATE_EXT)pCmdPtr;

           if ( Adapter->bIsAssociateInProgress == TRUE )
            {
                DBGPRINT(DBG_CMD | DBG_WARNING, 
                    (L"There is already an association command pending,fail the current request!\n"));
                       

                NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
                CleanUpCmdCtrlNode(pTempCmd);
                InsertQNodeAtTail(&Adapter->CmdFreeQ, (PQ_NODE)pTempCmd);
                NdisReleaseSpinLock(&Adapter->FreeQSpinLock);

                    if (Adapter->bIsReconnectEnable == TRUE)    
                        Adapter->bIsReConnectNow = FALSE;

                    Adapter->bIsSystemConnectNow = FALSE;   
			retval = NDIS_STATUS_NOT_ACCEPTED;
          


			goto funcFinal;

            }
      Status = SetupAssociationExt(
              (PHostCmd_DS_802_11_ASSOCIATE_EXT)pCmdPtr, 
              PendingInfo, 
              Adapter, 
              pTempCmd, 
              InformationBuffer
              );
      Size = pAsso->Size;
      DBGPRINT(DBG_CMD|DBG_CCX, (L"HostCmd_CMD_802_11_ASSOCIATE_EXT size = %d \n",Size));
	if (Status == NDIS_STATUS_PENDING) {
		DBGPRINT(DBG_CCX|DBG_WARNING, (L"[PrepareAndSendCommand] return NDIS_STATUS_PENDING\n"));
		retval = Status;
		goto funcFinal;
	}


      if (Status != NDIS_STATUS_SUCCESS)
      {
               if (Adapter->bIsReconnectEnable == TRUE)    
                        Adapter->bIsReConnectNow = FALSE;

                    Adapter->bIsSystemConnectNow = FALSE;
		retval = Status;


		goto funcFinal;
      }

      break;

    case HostCmd_CMD_802_11_AD_HOC_START:
    {      
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_AD_HOC_START));
        pAdHocStart = (PHostCmd_DS_802_11_AD_HOC_START)pCmdPtr;

        if (Adapter->ulPSNumOfBSSIDs >= MRVDRV_MAX_BSSID_LIST)
        {
            ReturnCmdNode(Adapter, pTempCmd); 
            retval = NDIS_STATUS_FAILURE;
            goto funcFinal;
        }

        SetupAdHocStart(pAdHocStart,
                        PendingInfo,
                        Adapter, 
                        pTempCmd,
                        InformationBuffer);


        DBGPRINT(DBG_ADHOC|DBG_HELP,(L"HWAC - ADHOC Start command is ready\r\n"));
        break;
    }

    case HostCmd_CMD_802_11_AD_HOC_JOIN:
    {
        // The command structure is HostCmd_CMD_802_11_AD_HOC_JOIN
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_AD_HOC_JOIN));
        pAdHocJoin = (PHostCmd_DS_802_11_AD_HOC_JOIN)pCmdPtr;

        SetupAdHocJoin(pAdHocJoin,
                       PendingInfo,
                       Adapter, 
                       pTempCmd,
                       InformationBuffer);


        DBGPRINT(DBG_V9|DBG_ADHOC, (L"HWAC - ADHOC Join command is ready\n"));
        break;
    }

      case HostCmd_CMD_802_11_DEAUTHENTICATE:

          Size = (USHORT)(sizeof(HostCmd_DS_802_11_DEAUTHENTICATE));
          pDeAuthenticate = (PHostCmd_DS_802_11_DEAUTHENTICATE) pCmdPtr;

          // set AP MAC address
          NdisMoveMemory(pDeAuthenticate->MacAddr,Adapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
        
          // Reason code 3 = Station is leaving
          pDeAuthenticate->ReasonCode = 3;

          if ( Adapter->bMicErrorIsHappened )
          {
             pDeAuthenticate->ReasonCode = 14;
          }

          if(Adapter->bAvoidScanAfterConnectedforMSecs == TRUE)  
         {
             NdisMCancelTimer(&Adapter->MrvDrvAvoidScanAfterConnectedTimer, &timerStatus);
             Adapter->bAvoidScanAfterConnectedforMSecs=FALSE;       
         }

          if ( Adapter->bBgScanEnabled==TRUE && Adapter->bBgDeepSleep==FALSE)
         {
             EnableBgScan( Adapter, FALSE); 
                 
         }
        //  need to clean up Rx and Tx first
        CleanUpSingleTxBuffer(Adapter);
        ResetRxPDQ(Adapter);
        
        {
            PFASTROAMPARAM      pfrParam = &Adapter->frParam;
            if (NdisEqualMemory(pfrParam->ccxLastAP.alMACAddr, pfrParam->ccxCurrentAP.alMACAddr, sizeof(NDIS_802_11_MAC_ADDRESS))) 
           {
                ///The Adapter->ccxCurrentAP is an valid data, complete the data by recording 
                ///     the dis-association time
                pfrParam->ccxLastAP.alDisassocTime = GetTickCount();
                DBGPRINT(DBG_CCX|DBG_HELP,(L"Disconnected, Ticks: %d \r\n",pfrParam->ccxLastAP.alDisassocTime));
           } 
            NdisZeroMemory(&pfrParam->ccxCurrentAP, sizeof(CCX_AP_INFO));
        }
        
        //  need to report disconnect event if currently associated
        
      if ( Adapter->CurrentSSID.SsidLength !=0 )
      { 
        //memorize the previous SSID and BSSID
        NdisMoveMemory( &(Adapter->PreviousSSID), 
          &(Adapter->CurrentSSID), 
          sizeof(NDIS_802_11_SSID));
            
          NdisMoveMemory( Adapter->PreviousBSSID,
          Adapter->CurrentBSSID, 
          MRVDRV_ETH_ADDR_LEN);
            
        //  need to erase the current SSID and BSSID info
        Adapter->ulCurrentBSSIDIndex =0;
        NdisZeroMemory(&(Adapter->CurrentSSID), sizeof(NDIS_802_11_SSID));
                NdisZeroMemory(Adapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
                NdisZeroMemory(&(Adapter->CurrentBSSIDDesciptor), sizeof(NDIS_WLAN_BSSID_EX));
        NdisZeroMemory(&(Adapter->CurrentBssDesciptor), sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));
      }
    break;


    case HostCmd_CMD_802_11_RF_ANTENNA:
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_RF_ANTENNA));
      pAntenna = (PHostCmd_DS_802_11_RF_ANTENNA)pCmdPtr;    
      pAntenna->Action = CmdOption;   
      
      if( (CmdOption ==HostCmd_ACT_SET_RX) || (CmdOption==HostCmd_ACT_SET_TX))
        pAntenna->AntennaMode=(USHORT)(*(NDIS_802_11_ANTENNA *)InformationBuffer);
    break;

    case HostCmd_CMD_802_11_RF_TX_POWER:
      
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_RF_TX_POWER));
      pRTP = (PHostCmd_DS_802_11_RF_TX_POWER)pCmdPtr;
    
        if ( InformationBuffer != NULL )
            {
                // OID command
                switch (CmdOption)
                {
          case HostCmd_ACT_TX_POWER_OPT_SET :
          {
            ULONG  usDesiredPowerInDbm = *((ULONG *)InformationBuffer);

            if ( (usDesiredPowerInDbm < 5) || (usDesiredPowerInDbm > 17) )
            {
			retval = NDIS_STATUS_NOT_SUPPORTED;
			goto funcFinal;
            }

// 35.p6 : new format for RF_TX_POWER            
              pRTP->CurrentLevel=(USHORT)usDesiredPowerInDbm;
                  pRTP->Action=(USHORT)HostCmd_ACT_TX_POWER_OPT_SET;
            DBGPRINT(DBG_OID,(L"****SendCommand -HostCmd_CMD_802_11_RF_TX_POWER- pRTP->CurrentLevel=%d\n",pRTP->CurrentLevel)); 

          }
          break;

          case HostCmd_ACT_TX_POWER_OPT_GET:
                  {   
                    DBGPRINT(DBG_OID,(L"****SendCommand -HostCmd_CMD_802_11_RF_TX_POWER- HostCmd_ACT_TX_POWER_OPT_GET\n")); 
            pRTP->Action = HostCmd_ACT_GEN_GET;
// 35.p6 : new format for RF_TX_POWER            
            pRTP->CurrentLevel = 0;
                  }
                  break;
      
          case HostCmd_ACT_TX_POWER_OPT_SET_HIGH:
            pRTP->Action = HostCmd_ACT_GEN_SET;
// 35.p6 : new format for RF_TX_POWER            
            pRTP->CurrentLevel = MRVDRV_TX_POWER_LEVEL_HIGH;
              break;
      
          case HostCmd_ACT_TX_POWER_OPT_SET_MID:
            pRTP->Action = HostCmd_ACT_GEN_SET;
// 35.p6 : new format for RF_TX_POWER            
            pRTP->CurrentLevel = MRVDRV_TX_POWER_LEVEL_MID;
              break;
  
          case HostCmd_ACT_TX_POWER_OPT_SET_LOW:
            pRTP->Action = HostCmd_ACT_GEN_SET;
// 35.p6 : new format for RF_TX_POWER            
            pRTP->CurrentLevel = MRVDRV_TX_POWER_LEVEL_LOW;
            break;

          case HostCmd_ACT_TX_POWER_OPT_SET_TPC:
            pRTP->Action = HostCmd_ACT_GEN_SET;
// 35.p6 : new format for RF_TX_POWER            
            pRTP->CurrentLevel = HostCmd_ACT_TX_POWER_INDEX_TPC;
           
                    DBGPRINT(DBG_OID,(L"\n**** TPC %d****\n\n", *((ULONG *)InformationBuffer))); 
            
            break;
          
          default:
			retval = NDIS_STATUS_NOT_SUPPORTED;
			goto funcFinal;
              }
            }
      break;
  
    case HostCmd_CMD_802_11_RADIO_CONTROL:
      Size = (USHORT)(sizeof(HostCmd_DS_802_11_RADIO_CONTROL));
      pRadioControl = (PHostCmd_DS_802_11_RADIO_CONTROL)pCmdPtr;    
      pRadioControl->Action = CmdOption;    
              
      DBGPRINT(DBG_CMD|DBG_OID|DBG_HELP,(L"RADIO CONTROL:%x,%x\r\n",CmdOption,Adapter->Preamble)); 
      switch((UCHAR)(Adapter->Preamble)) 
      { 
        case HostCmd_TYPE_SHORT_PREAMBLE:
          pRadioControl->Control=SET_SHORT_PREAMBLE;
        break;
    
        case HostCmd_TYPE_LONG_PREAMBLE:
          pRadioControl->Control=SET_LONG_PREAMBLE;
        break;          
        
        default:
        //case HostCmd_TYPE_AUTO_PREAMBLE: //dralee_20060613
             pRadioControl->Control=SET_AUTO_PREAMBLE;
        break;
      }
      if ( CmdOption == HostCmd_ACT_SET )
      {
          if (Adapter->RadioOn) 
              pRadioControl->Control |= TURN_ON_RF;
          else 
              pRadioControl->Control &= ~TURN_ON_RF;
        
           DBGPRINT(DBG_OID, (L"HostCmd_CMD_802_11_RADIO_CONTROL pRadioControl->Control=0x%x,Adapter->RadioOn=%d\n",pRadioControl->Control,Adapter->RadioOn));
      }
      break;
    case HostCmd_CMD_802_11_RF_CHANNEL:
    {
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_RF_CHANNEL));
        pRFChannel = (PHostCmd_DS_802_11_RF_CHANNEL)pCmdPtr;    
        pRFChannel->Action = CmdOption;

        if (Adapter->Channel == 0) {
            Adapter->Channel = DEFAULT_CHANNEL;
        }
        if(CmdOption ==HostCmd_ACT_GEN_SET)
        {
            pRFChannel->CurentChannel = (UCHAR)Adapter->Channel;
            DBGPRINT(DBG_OID,(L"SendCommand - HostCmd_CMD_802_11_RF_CHANNEL,  Adapter->Channel=0x%x ****\n",Adapter->Channel)); 
        }
          }
            break;

    // Command Code 0x0040 is used as CMD_802_11_AD_HOC_STOP in normal firmware
    // but as CMD_MFG_COMMAND in manufacture firmware 
    //case HostCmd_CMD_MFG_COMMAND:
    case HostCmd_CMD_802_11_AD_HOC_STOP:
    {
        if (Adapter->UseMfgFw == 1)    // Mfg fw, 0x0040 treat as Manufature Command
        {
            PHostCmd_DS_Mfg_Cmd   pCmd;
            POID_MRVL_DS_MFG_CMD  pUserBuffer;

            pCmd = (PHostCmd_DS_Mfg_Cmd)pCmdPtr;
            pUserBuffer = (POID_MRVL_DS_MFG_CMD)InformationBuffer;
            
            NdisMoveMemory(pCmd->MfgCmd,
                           pUserBuffer->MfgCmd,
                           MRVDRV_MFG_CMD_LEN);

            HexDump(DBG_CMD|DBG_HELP, "MfgCmd : ",
                   (UCHAR *)&pCmd->MfgCmd, MRVDRV_MFG_CMD_LEN);    
            Size = sizeof(HostCmd_DS_Mfg_Cmd);       
        }
        else    // Normal fw, 0x0040 treat as Adhoc_Stop Command
        {
            // nothing to set for adhoc stop cmd
            Size = (USHORT)(sizeof(HostCmd_DS_GEN));
        }
        break;
    }
      
        case HostCmd_CMD_802_11D_DOMAIN_INFO:
        {    
             UCHAR countryNum;
      
             pDomainInfo = (PHostCmd_DS_802_11D_DOMAIN_INFO)pCmdPtr;
             countryNum = *((UCHAR *)InformationBuffer); 
             pDomainInfo-> Action = CmdOption;   

             if (CmdOption == HostCmd_ACT_SET)
             {                   
               
                 Generate_domain_info_11d( countryNum, 
                                           Adapter->connected_band, 
                                           &pDomainInfo->Domain);
             } 
             
             Size = pDomainInfo->Domain.Header.Len + sizeof(MrvlIEtypesHeader_t) + sizeof(HostCmd_DS_GEN) + 2;
      
             HexDump(DBG_ALLEN, "HostCmd_CMD_802_11D_DOMAIN_INFO dump", (UCHAR *)pCmdPtr, 
                                 pDomainInfo->Size);
        }
            break;
    case HostCmd_CMD_802_11_KEY_MATERIAL:
      
      pKeyMaterial = (PHostCmd_DS_802_11_KEY_MATERIAL)pCmdPtr; 
      
            SetupKeyMaterial(
              Adapter, 
            pKeyMaterial, 
            CmdOption, 
            INTOption, 
            InformationBuffer);

      Size =  pKeyMaterial->Size;

      break;
            case HostCmd_CMD_802_11_WMM_ACK_POLICY:
                 pWmmAckPolicy = (PHostCmd_DS_802_11_WMM_ACK_POLICY)pCmdPtr;
                 pAckPolicy = (POID_MRVL_DS_WMM_ACK_POLICY)InformationBuffer;       
                 pWmmAckPolicy->Command = HostCmd_CMD_802_11_WMM_ACK_POLICY;
                 pWmmAckPolicy->Size = sizeof(HostCmd_DS_802_11_WMM_ACK_POLICY);
             pWmmAckPolicy->Action = CmdOption;
             pWmmAckPolicy->AC = pAckPolicy->AC;
             pWmmAckPolicy->AckPolicy = pAckPolicy->AckPolicy;
                 Size = pWmmAckPolicy->Size;
            break;
            case HostCmd_CMD_802_11_WMM_GET_STATUS:
                 pWmmGetStatus = (PHostCmd_DS_802_11_WMM_GET_STATUS)pCmdPtr;
                 pWmmGetStatus->Command = HostCmd_CMD_802_11_WMM_GET_STATUS;
                 pWmmGetStatus->Size = sizeof(HostCmd_DS_802_11_WMM_GET_STATUS);
                 Size = pWmmGetStatus->Size;
      break;
    case HostCmd_CMD_802_11_INACTIVITY_TIMEOUT:
     {
         PHostCmd_DS_802_11_INACTIVITY_TIMEOUT  pcmdTO;
         POID_MRVL_DS_INACTIVITY_TIMEOUT        poidTO;

         pcmdTO = (PHostCmd_DS_802_11_INACTIVITY_TIMEOUT)pCmdPtr; 
         poidTO = (POID_MRVL_DS_INACTIVITY_TIMEOUT)InformationBuffer; 
         
         Size = sizeof(HostCmd_DS_802_11_INACTIVITY_TIMEOUT);  
         pcmdTO->Action = (USHORT)CmdOption;    
         pcmdTO->timeout = poidTO->time; 
 
         DBGPRINT(DBG_PS|DBG_CMD|DBG_HELP,(L"Inactivity Timeout:%d,%d\r\n",pcmdTO->Action, pcmdTO->timeout));
 
     }
      break;  
    //++dralee_20060307
    case HostCmd_CMD_802_11_SLEEP_PERIOD:
     {
         PHostCmd_DS_802_11_SLEEP_PERIOD  pcmdSP;
         POID_MRVL_DS_WMM_SLEEP_PERIOD    poidSP;

         pcmdSP = (PHostCmd_DS_802_11_SLEEP_PERIOD)pCmdPtr; 
         poidSP = (POID_MRVL_DS_WMM_SLEEP_PERIOD)InformationBuffer; 
         
         Size = sizeof(HostCmd_DS_802_11_SLEEP_PERIOD);  
         pcmdSP->Action = (USHORT)CmdOption;    
         pcmdSP->period = poidSP->period; 
 
         DBGPRINT(DBG_OID|DBG_CMD|DBG_HELP,(L"sleep period:%d\r\n",pcmdSP->period));
 
     }
     break;

    case HostCmd_CMD_802_11_KEY_ENCRYPT:
         {
            POID_MRVL_DS_KEY_ENCRYPT        oidencry;
            PHostCmd_DS_802_11_KEY_ENCRYPT  cmdencry;
            
            oidencry = (POID_MRVL_DS_KEY_ENCRYPT)InformationBuffer;
            cmdencry = (PHostCmd_DS_802_11_KEY_ENCRYPT)pCmdPtr;
            Size = sizeof(HostCmd_DS_802_11_KEY_ENCRYPT);

            if( CmdOption == HostCmd_ACT_SET )
            {   
                NdisMoveMemory((PUCHAR)(&cmdencry->EncType),(PUCHAR)(&oidencry->EncType), sizeof(OID_MRVL_DS_KEY_ENCRYPT)); 
                DBGPRINT(DBG_CMD|DBG_HELP,(L"T:%x,L:%x\r\n",cmdencry->EncType,cmdencry->KeyDataLen));
            }
         }
        break;

    case HostCmd_CMD_802_11_CRYPTO:
        {   
            POID_MRVL_DS_CRYPTO           oidcry;  
            PHostCmd_DS_802_11_CRYPTO     cmdcry; 

            oidcry = (POID_MRVL_DS_CRYPTO)InformationBuffer;
            cmdcry = (PHostCmd_DS_802_11_CRYPTO)pCmdPtr;
            Size = sizeof(HostCmd_DS_802_11_CRYPTO);   
               
            if( CmdOption == HostCmd_ACT_SET )
            {     
                NdisMoveMemory((PUCHAR)(&cmdcry->EncDec),(PUCHAR)(&oidcry->EncDec), 80);  

                DBGPRINT(DBG_CMD|DBG_HELP,(L"T:%x,L:%x\r\n",
                         cmdcry->TLVBuf.Header.Type,cmdcry->TLVBuf.Header.Len ) ); 

                cmdcry->TLVBuf.Header.Type = TLV_TYPE_CRYPTO_DATA;
                cmdcry->TLVBuf.Header.Len = oidcry->TLVBuf.Header.Len; 

                NdisMoveMemory((PUCHAR)(&cmdcry->TLVBuf.payload[0]),
                               (PUCHAR)(&oidcry->TLVBuf.payload[0]),
                               cmdcry->TLVBuf.Header.Len );  
                
            }
        } 
        break;

    case HostCmd_CMD_802_11_TX_RATE_QUERY:              
        Size = (USHORT)(sizeof(HostCmd_TX_RATE_QUERY));
        Adapter->TxRate = 0;
        break;

    case HostCmd_CMD_802_11_GET_TSF: {
        PHostCmd_DS_802_11_GET_TSF pGetTSF = (PHostCmd_DS_802_11_GET_TSF)pCmdPtr;
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_GET_TSF));
        NdisMoveMemory(&pGetTSF->TSF, InformationBuffer, sizeof(HostCmd_DS_802_11_GET_TSF)-sizeof(HostCmd_DS_GEN));
        }
        break;
    case HostCmd_CMD_802_11_WMM_ADDTS_REQ: {
        PHostCmd_DS_802_11_WMM_ADDTS pAddts = (PHostCmd_DS_802_11_WMM_ADDTS) pCmdPtr;
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_WMM_ADDTS) - sizeof(pAddts->ExtraIeData));
        NdisMoveMemory(&pAddts->TspecResult, InformationBuffer, Size - sizeof(HostCmd_DS_GEN));
	{
            Adapter->bHasTspec = TRUE;
            NdisMoveMemory(Adapter->CurrentTspec, pAddts->TspecData, 63);
	}
        }
        break;
    case HostCmd_CMD_802_11_WMM_DELTS_REQ: {
        PHostCmd_DS_802_11_WMM_DELTS pDelts = (PHostCmd_DS_802_11_WMM_DELTS) pCmdPtr;
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_WMM_DELTS));
        NdisMoveMemory(&pDelts->TspecResult, InformationBuffer, sizeof(HostCmd_DS_802_11_WMM_DELTS)-sizeof(HostCmd_DS_GEN));
        ///We are going to send DELTS => Clear the previous ADDTS_Response
        NdisZeroMemory(&Adapter->AddTSResp, sizeof(&Adapter->AddTSResp));
        }
        break;
    case HostCmd_CMD_802_11_WMM_QUEUE_CONFIG: {
        PHostCmd_DS_802_11_WMM_QUEUE_CONFIG pQueCfg = (PHostCmd_DS_802_11_WMM_QUEUE_CONFIG) pCmdPtr;
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_WMM_QUEUE_CONFIG));
        NdisMoveMemory(&pQueCfg->Action, InformationBuffer, sizeof(HostCmd_DS_802_11_WMM_QUEUE_CONFIG)-sizeof(HostCmd_DS_GEN));
        }
        break;
    case HostCmd_CMD_802_11_WMM_QUEUE_STATS: {
        PHostCmd_DS_802_11_WMM_QUEUE_STATS pQueStats = (PHostCmd_DS_802_11_WMM_QUEUE_STATS) pCmdPtr;
        Size = (USHORT)(sizeof(HostCmd_DS_802_11_WMM_QUEUE_STATS));
        NdisMoveMemory(&pQueStats->Action, InformationBuffer, sizeof(HostCmd_DS_802_11_WMM_QUEUE_STATS)-sizeof(HostCmd_DS_GEN));
        }
        break;
    case HostCmd_CMD_802_11_IBSS_COALESING_STATUS:
         { 
            PHostCmd_802_11_IBSS_Coalesing_Status  IbssCo;

            IbssCo = (PHostCmd_802_11_IBSS_Coalesing_Status)pCmdPtr;
            Size = sizeof(HostCmd_802_11_IBSS_Coalesing_Status);   
            IbssCo->Action = (USHORT)CmdOption;

            if( CmdOption == HostCmd_ACT_SET ) 
            {
              IbssCo->Enable = *((USHORT *)InformationBuffer);
              DBGPRINT(DBG_ADHOC|DBG_CMD|DBG_HELP,(L"SET CMD IBSS_COALESCE...\r\n"));
            } 
         }
         break;
    case HostCmd_CMD_802_11_HOST_SLEEP_ACTIVATE:
         Size = sizeof( HostCmd_DS_WAKEUP_CONFIRM);
         break;
    case HostCmd_CMD_802_11_FW_WAKE_METHOD:
         {  
            PHostCmd_DS_802_11_FW_WAKE_METHOD fwd;

            fwd = (PHostCmd_DS_802_11_FW_WAKE_METHOD)pCmdPtr;
            fwd->Action = (USHORT)CmdOption;
            fwd->Method =  ((POID_MRVL_DS_FW_WAKE_METHOD)InformationBuffer)->method;
            Size = sizeof(HostCmd_DS_802_11_FW_WAKE_METHOD);   
            DBGPRINT(DBG_CMD|DBG_HELP, (L"Fw Wake Method:Action:%x,Method:%x\r\n",fwd->Action,fwd->Method));
         }
         break;                       
    //022607
    case HostCmd_CMD_802_11_BCA_CONFIG_TIMESHARE:
         { 
           PHostCmd_DS_802_11_BCA_TIMESHARE cmdbca;
           POID_MRVL_DS_BCA_TIMESHARE oidbca;
           
           cmdbca = (PHostCmd_DS_802_11_BCA_TIMESHARE)pCmdPtr;
           oidbca = (POID_MRVL_DS_BCA_TIMESHARE)InformationBuffer; 
           
           cmdbca->Action = (USHORT)CmdOption;
           cmdbca->TrafficType = oidbca->TrafficType;
           cmdbca->TimeShareInterval = oidbca->TimeShareInterval;
           cmdbca->BTTime = oidbca->BTTime;
           Size = sizeof(HostCmd_DS_802_11_BCA_TIMESHARE); 
           DBGPRINT(DBG_CMD|DBG_HELP,(L"BCA CONFIG TIMESHARE:%x,%x,%x\r\n",
                                      cmdbca->TrafficType,cmdbca->TimeShareInterval,cmdbca->BTTime)); 
         } 
         break;
    default:
            DBGPRINT(DBG_ERROR, (L"ERROR: Unrecognized command 0x%x!!\n", Cmd));
    break;
  }

    // Set command size
    pCmdPtr->Size = Size;

    // If there is no pending command, download command, else, append command to Q
    if (Size == 0 || pTempCmd == NULL)
    { 
        if (Size == 0 )
            DBGPRINT(DBG_CMD,(L"HWAC - Detected Null command or size zero, cmd = 0x%x\n", Cmd));
    }
    else
    {              
        //dralee_1108++
        EnterCriticalSection(&Adapter->CmdQueueExeSection);
        InsertCmdToQueue (Adapter, pTempCmd); 
        //dralee_1108++ 
        LeaveCriticalSection(&Adapter->CmdQueueExeSection); 

        GetCmdFromQueueToExecute(Adapter);
    }
	retval = NDIS_STATUS_SUCCESS;
funcFinal:
  return retval;
}




/******************************************************************************
 *
 *  Name: SetupDataRate()
 *
 *  Description: Prepare data rate command
 *
 *  Arguments:  PHostCmd_DS_802_11_DATA_RATE pDataRate
 *        PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:
 * 
 *  Notes:
 *
 *****************************************************************************/
VOID 
SetupDataRate(
  USHORT CmdOption,
  PHostCmd_DS_802_11_DATA_RATE pDataRate,
  PMRVDRV_ADAPTER Adapter,
  PVOID InformationBuffer
  )
{ 
    UCHAR *pDot11DataRate;
    UCHAR   FWIndex;

    DBGPRINT(DBG_CMD,(L"HostCmd_CMD_802_11_DATA_RATE\n")); 
    // HostCmd_ACT_SET    
    if(CmdOption == HostCmd_ACT_GET) 
    {   
        DBGPRINT(DBG_CMD,(L"SetupDataRate(): GET Data Rate\n"));
    pDataRate->Action = HostCmd_ACT_GET_TX_RATE;
    }
    else if(CmdOption == HostCmd_ACT_SET) 
    {
        DBGPRINT(DBG_DATARATE,(L"SetupDataRate(): Setting FW for fixed rate\n"));

    pDot11DataRate = (UCHAR *)InformationBuffer;
          
        FWIndex = ConvertNDISRateToFWIndex(*(pDot11DataRate));

        if (*pDot11DataRate==0)
        {
            DBGPRINT(DBG_CMD | DBG_ERROR, 
            (L"SetupDataRate(): Error looking up FW Index, use auto rate!\n"));
            pDataRate->Action = HostCmd_ACT_SET_TX_AUTO;
        }
        else
        {
          // Fixed rate setting
        pDataRate->Action = HostCmd_ACT_SET_TX_FIX_RATE;
            pDataRate->DataRate[0] = FWIndex;
            DBGPRINT(DBG_CMD, (L"Data rate index set to %d\n", FWIndex));
        }
    }
}

/******************************************************************************
 *
 *  Name: CleanupWepKeys()
 *
 *  Description: Cleanup WEP keys
 *
 *  Arguments:  PHostCmd_DS_802_11_SET_WEP pSetWEP
 *        
 *    
 *  Return Value:
 * 
 *  Notes:
 *
 *****************************************************************************/
VOID 
CleanupWepKeys(
  PHostCmd_DS_802_11_SET_WEP pSetWEP
  )
{
  pSetWEP->WEPTypeForKey1 = 0;
  pSetWEP->WEPTypeForKey2 = 0;
  pSetWEP->WEPTypeForKey3 = 0;
  pSetWEP->WEPTypeForKey4 = 0;
  NdisZeroMemory(pSetWEP->WEP1, MRVL_KEY_BUFFER_SIZE_IN_BYTE);
  NdisZeroMemory(pSetWEP->WEP2, MRVL_KEY_BUFFER_SIZE_IN_BYTE);
  NdisZeroMemory(pSetWEP->WEP3, MRVL_KEY_BUFFER_SIZE_IN_BYTE);
  NdisZeroMemory(pSetWEP->WEP4, MRVL_KEY_BUFFER_SIZE_IN_BYTE);
}

/******************************************************************************
 *
 *  Name: SetupWepKeys()
 *
 *  Description: Setup WEP keys
 *
 *  Arguments:  NDIS_OID PendingOID
 *        PHostCmd_DS_802_11_SET_WEP pSetWEP
 *        PNDIS_802_11_WEP pNewWEP
 *    
 *  Return Value:
 * 
 *  Notes:
 *
 *****************************************************************************/
VOID
SetupWepKeys(
  NDIS_OID PendingOID,
  PHostCmd_DS_802_11_SET_WEP pSetWEP,
  PNDIS_802_11_WEP pNewWEP,
  PVOID InformationBuffer,
  PMRVDRV_ADAPTER Adapter,
  USHORT CmdOption
  )
{
  NDIS_802_11_KEY_INDEX KeyIndex;
  UCHAR ucWepType;
  ULONG KeyLength;
   USHORT wepKeyIndex;

  // May need to check pending OID and OID information buffer to get 
  // more information

  if(PendingOID == OID_802_11_ADD_WEP ){
    
    pSetWEP->Action = HostCmd_ACT_ADD;
    pSetWEP->KeyIndex = (USHORT) (pNewWEP->KeyIndex & (ULONG)HostCmd_CCKM_KEY_INDEX_MASK); // 2k60815
    KeyLength = pNewWEP->KeyLength;
     
    //Coverity Error id:18 (UNINIT)
    if( KeyLength == 5 )
        ucWepType= HostCmd_TYPE_WEP_40_BIT;
    else if( KeyLength == 13 )
        ucWepType= HostCmd_TYPE_WEP_104_BIT;
    else if( KeyLength == 16 )
        ucWepType= HostCmd_TYPE_WEP_128_BIT;
    else 
    {
        ucWepType= HostCmd_TYPE_WEP_40_BIT;
        DBGPRINT(DBG_ERROR,(L"Wrong Wep Key Length:%x\r\n",KeyLength));
        return;
    }   

    // clean-up all the WEP key types and WEP keys
    CleanupWepKeys(pSetWEP);  
  wepKeyIndex = pSetWEP->KeyIndex;
    *((UCHAR *)(&pSetWEP->WEPTypeForKey1 + wepKeyIndex)) = ucWepType;
    NdisMoveMemory((void *)(pSetWEP->WEP1 + wepKeyIndex * 16), pNewWEP->KeyMaterial, KeyLength);

    if(pNewWEP->KeyIndex & 0x80000000)
    {
       Adapter->txKeyIndex = pSetWEP->KeyIndex;
       Adapter->txKeyLength = (USHORT)pNewWEP->KeyLength;
       NdisMoveMemory(Adapter->txKeyMaterial, pNewWEP->KeyMaterial, pNewWEP->KeyLength);
    }
    else
    {
         if(Adapter->txKeyLength)
         {
          switch(Adapter->txKeyLength )
          {
            case 5:
                 ucWepType= HostCmd_TYPE_WEP_40_BIT;
            break;  
            case 13:
                 ucWepType= HostCmd_TYPE_WEP_104_BIT;
            break;  
            case 16:
                 ucWepType= HostCmd_TYPE_WEP_128_BIT;
            break;
            default:
                 ucWepType= HostCmd_TYPE_WEP_40_BIT;
            break;  
          }
           pSetWEP->KeyIndex = Adapter->txKeyIndex;
          wepKeyIndex = pSetWEP->KeyIndex;
          *((UCHAR *)(&pSetWEP->WEPTypeForKey1 + wepKeyIndex)) = ucWepType;
          NdisMoveMemory((void *)(pSetWEP->WEP1 + wepKeyIndex * 16), Adapter->txKeyMaterial, Adapter->txKeyLength);
         } 
    }   

  }
  
  if(PendingOID == OID_802_11_REMOVE_WEP ){
    pSetWEP->Action = HostCmd_ACT_REMOVE;
    KeyIndex = *((NDIS_802_11_KEY_INDEX *)(InformationBuffer));
    KeyIndex = KeyIndex & HostCmd_WEP_KEY_INDEX_MASK;
    Adapter->txKeyIndex = 0xFFFF;
    Adapter->txKeyLength = 0;
    NdisZeroMemory(Adapter->txKeyMaterial,MRVL_KEY_BUFFER_SIZE_IN_BYTE);
    
    CleanupWepKeys(pSetWEP);
       *((UCHAR *)(&pSetWEP->WEP1 + KeyIndex * 16)) = 0xFF;

     NdisZeroMemory(&(Adapter->CurrentWEPKey), sizeof(MRVL_WEP_KEY)); 
  }
    
  if(PendingOID == OID_802_11_RELOAD_DEFAULTS )
  {   
    pSetWEP->Action = CmdOption;
  }
}


/******************************************************************************
 *
 *  Name: SetupAssociationExt()
 *
 *  Description: Setup association extended
 *
 *  Arguments:  NDIS_OID PendingOID
 *        PMRVDRV_ADAPTER Adapter
 *        CmdCtrlNode *pTempCmd
 *    
 *  Return Value:
 * 
 *  Notes:
 *
 *****************************************************************************/
int SetupKeyMaterial(
    PMRVDRV_ADAPTER Adapter, 
    PHostCmd_DS_802_11_KEY_MATERIAL  pKeyMaterial, 
    USHORT option, 
    USHORT enable, 
    void *InformationBuffer)
{
  
  PNDIS_802_11_KEY    pKey = (PNDIS_802_11_KEY)InformationBuffer;
  USHORT        KeyParamSet_len;
    PMRVL_NDIS_WPA_KEY  pKeyIn;
  KeyMaterial_TKIP_t  *pKeyTkipOut;
  KeyMaterial_AES_t   *pKeyAesOut;

  pKeyMaterial->Action = option;

  if( option == HostCmd_ACT_GET )
  {
    pKeyMaterial->Size = 2 + 8;
    DBGPRINT(DBG_CMD, (L"SetupKeyMaterial Get\n"));
    return 0;
  }
    DBGPRINT(DBG_CMD, (L"SetupKeyMaterial Set\n"));
  
  NdisZeroMemory(&(pKeyMaterial->KeyParamSet), sizeof(MrvlIEtype_KeyParamSet_t));

  pKeyIn = (PMRVL_NDIS_WPA_KEY)pKey->KeyMaterial;
  
  if (pKey->KeyLength == WPA_AES_KEY_LEN)
  {   
      DBGPRINT(DBG_CMD, (L"Set AES key\n"));
    
    pKeyMaterial->KeyParamSet.KeyTypeId = KEY_TYPE_ID_AES;
    if ( enable == KEY_INFO_ENABLED )
      pKeyMaterial->KeyParamSet.KeyInfo = KEY_INFO_AES_ENABLED;
    else
      pKeyMaterial->KeyParamSet.KeyInfo = !(KEY_INFO_AES_ENABLED);

    if (pKey->KeyIndex & 0x40000000)  // AES pairwise key: unicast
      pKeyMaterial->KeyParamSet.KeyInfo |= KEY_INFO_AES_UNICAST;
    else                // AES group key: multicast
      pKeyMaterial->KeyParamSet.KeyInfo |= KEY_INFO_AES_MCAST;

    pKeyAesOut = (KeyMaterial_AES_t *)pKeyMaterial->KeyParamSet.Key;
    NdisMoveMemory(pKeyAesOut->AesKey, pKeyIn->EncryptionKey, 16);

        DBGPRINT(DBG_ADHOC, (L"++SetupKeyMaterial\n"));
        DBGPRINT(DBG_ADHOC, (L"++Set AES key\n"));
        DBGPRINT(DBG_ADHOC, (L"KeyParamSet.KeyTypeId: %d\n", pKeyMaterial->KeyParamSet.KeyTypeId));
        DBGPRINT(DBG_ADHOC, (L"KeyParamSet.KeyInfo: %d\n", pKeyMaterial->KeyParamSet.KeyInfo));
        HexDump(DBG_ADHOC, "AESKey: ", (PUCHAR)pKeyAesOut->AesKey, 16);
  }
  else if (pKey->KeyLength == WPA_TKIP_KEY_LEN) 
  {
      DBGPRINT(DBG_CMD, (L"Set TKIP key\n"));
    
    pKeyMaterial->KeyParamSet.KeyTypeId = KEY_TYPE_ID_TKIP;
    pKeyMaterial->KeyParamSet.KeyInfo = KEY_INFO_TKIP_ENABLED;

      if (pKey->KeyIndex & 0x40000000)  // TKIP pairwise key: unicast
      pKeyMaterial->KeyParamSet.KeyInfo |= KEY_INFO_TKIP_UNICAST;
    else                // TKIP group key: multicast
      pKeyMaterial->KeyParamSet.KeyInfo |= KEY_INFO_TKIP_MCAST;

    pKeyTkipOut = (KeyMaterial_TKIP_t *)pKeyMaterial->KeyParamSet.Key;
    
    NdisMoveMemory(pKeyTkipOut->TkipKey, pKeyIn->EncryptionKey, 16);
    NdisMoveMemory(pKeyTkipOut->TkipRxMicKey, pKeyIn->MICKey1, 8);
    NdisMoveMemory(pKeyTkipOut->TkipTxMicKey, pKeyIn->MICKey2, 8);

        DBGPRINT(DBG_ADHOC, (L"++SetupKeyMaterial\n"));
        DBGPRINT(DBG_ADHOC, (L"++Set TKIP key\n"));
        DBGPRINT(DBG_ADHOC, (L"KeyParamSet.KeyTypeId: %d\n", pKeyMaterial->KeyParamSet.KeyTypeId));
        DBGPRINT(DBG_ADHOC, (L"KeyParamSet.KeyInfo: %d\n", pKeyMaterial->KeyParamSet.KeyInfo));
        HexDump(DBG_ADHOC, "TkipKey: ", (PUCHAR)pKeyTkipOut->TkipKey, 16);
        HexDump(DBG_ADHOC, "TkipRxMicKey: ", (PUCHAR)pKeyTkipOut->TkipRxMicKey, 8);
        HexDump(DBG_ADHOC, "TkipTxMicKey: ", (PUCHAR)pKeyTkipOut->TkipTxMicKey, 8);
  }

  if (pKeyMaterial->KeyParamSet.KeyTypeId) 
  {
    pKeyMaterial->KeyParamSet.Type = TLV_TYPE_KEY_MATERIAL;
    pKeyMaterial->KeyParamSet.KeyLen = (USHORT) pKey->KeyLength;

    pKeyMaterial->KeyParamSet.Length = (USHORT) (pKey->KeyLength + 6);

    // 2 bytes for Type, 2 bytes for Length
    KeyParamSet_len = pKeyMaterial->KeyParamSet.Length + 4;
    // 2 bytes for Action
    pKeyMaterial->Size = KeyParamSet_len + 2 + 8;
  }
  
    HexDump(DBG_CMD, "NdisKey context", (UCHAR *)pKey, sizeof(NDIS_802_11_KEY)+ pKey->KeyLength);
    HexDump(DBG_CMD, "pKeyMaterial context", (UCHAR *)pKeyMaterial, pKeyMaterial->Size);
  
  return 0;
}

INT FindAPBySSID(PMRVDRV_ADAPTER Adapter, PVOID InformationBuffer)
{
	INT		ulAttemptSSIDIdx = -1;
	ULONG	i;
	BOOLEAN bSsidFound = FALSE;
	long    lRSSI;
	ULONG   ulNumBSSID;
	PNDIS_WLAN_BSSID_EX pBSSIDListSrc;
	PMRV_BSSID_IE_LIST  pBSSIDIEListSrc;
	PBSS_DESCRIPTION_SET_ALL_FIELDS   pBssDescListSrc;
	BOOLEAN bHasHideSSID = FALSE;
	ULONG   iHideSSID; 
	int                     choosedBSSIDIdxList[MRVDRV_MAX_BSSID_LIST];
	UINT                    choosedBSSIDIdxNum = 0;
	static NDIS_802_11_SSID preSSID;
	static UINT                 preIndex=0;
///	int                     size;
    //022607
    ULONG                   AssoByBSSIDindex=0xff;


	///Adapter->bIsAssociateInProgress = TRUE;

	pBSSIDListSrc = Adapter->PSBSSIDList;
	pBSSIDIEListSrc = Adapter->PSIEBuffer;
	pBssDescListSrc = Adapter->PSBssDescList;
	ulNumBSSID = Adapter->ulPSNumOfBSSIDs;

	DBGPRINT(DBG_CMD|DBG_ASSO|DBG_HELP, (L"Associate_EXT: Number of BSSID %x \n", ulNumBSSID));

	// find out the BSSID that matches the SSID given in InformationBuffer
	for (i=0; i<ulNumBSSID; i++)
	{
		if ((NdisEqualMemory( pBSSIDListSrc[i].Ssid.Ssid, 
		   ((PNDIS_802_11_SSID)InformationBuffer)->Ssid, 
		   ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength)) &&
		   (pBSSIDListSrc[i].Ssid.SsidLength == 
		   ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength))
		{
        
			if ( pBSSIDListSrc[i].InfrastructureMode == Ndis802_11Infrastructure )
			{
				choosedBSSIDIdxList[choosedBSSIDIdxNum++]=i;
				if ( bSsidFound )
				{
					if ( (LONG)(pBSSIDListSrc[i].Rssi) > lRSSI )                
					{
						DBGPRINT(DBG_CMD|DBG_ASSO|DBG_HELP,(L"LWAC - RSSI(%d) of this AP > RSSI(%d) of current MAX RSSI AP\n",
                                                            Adapter->BSSIDList[i].Rssi,
                                                            lRSSI));

						lRSSI =Adapter->BSSIDList[i].Rssi= (LONG)(pBSSIDListSrc[i].Rssi);   //++dralee_20060417 comment from alanchin
						ulAttemptSSIDIdx = i;
					}  
				}
				else
				{    
					bSsidFound = TRUE;
					lRSSI = (LONG)(pBSSIDListSrc[i].Rssi);
					ulAttemptSSIDIdx = i;
					DBGPRINT(DBG_CMD|DBG_ASSO|DBG_HELP,(L"HWAC - First matching SSID with RSSI %d\n",
								lRSSI));
				}

                //022607
                if( AssoByBSSIDindex == 0xff &&
                    (NdisEqualMemory(Adapter->PSBssDescList[i].BSSID, Adapter->ReqBSSID, MRVDRV_ETH_ADDR_LEN)))
                {
                   AssoByBSSIDindex= i;
                   DBGPRINT(DBG_CMD|DBG_OID|DBG_ASSO|DBG_HELP,(L"Found SSID/BSSID pair:%x\r\n",AssoByBSSIDindex)); 
                }
 
			}
		} // end of SSID compare block

		DBGPRINT(DBG_CMD|DBG_ASSO|DBG_HELP,(L"SSID %s %d\n",
                                          pBSSIDListSrc[i].Ssid.Ssid, 
                                          pBSSIDListSrc[i].Ssid.SsidLength));

		// the part is for associate to hide ssid bss
		if ( pBSSIDListSrc[i].Ssid.Ssid[0] == '\0')
		{
			DBGPRINT(DBG_CMD|DBG_HELP,(L"HWAC - Find A Hide SSID in the list\n"));
			bHasHideSSID = TRUE;
			iHideSSID = i;  
		}
    
	} // for (i=0; i<Adapter->ulNumOfBSSIDs; i++)

	if ( choosedBSSIDIdxNum > 0 ) { // 35.p6: Added for check CCX
		// If multiple same SSID
		if ((NdisEqualMemory( preSSID.Ssid, 
		   ((PNDIS_802_11_SSID)InformationBuffer)->Ssid, 
		   ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength)) &&
		   ( preSSID.SsidLength == 
		   ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength))
		{
			preIndex = (preIndex +1)%choosedBSSIDIdxNum;
			ulAttemptSSIDIdx = choosedBSSIDIdxList[preIndex];
			///CCX_FASTROAM
			///Remember the current AP information since V9 because we will not send the deauth CMD
			if (Adapter->MediaConnectStatus == NdisMediaStateConnected)
			{
				PFASTROAMPARAM      pfrParam = &Adapter->frParam;
				if (NdisEqualMemory(pfrParam->ccxLastAP.alMACAddr, pfrParam->ccxCurrentAP.alMACAddr, sizeof(NDIS_802_11_MAC_ADDRESS))) {
					///The Adapter->ccxCurrentAP is an valid data, complete the data by recording 
					///     the dis-association time
					pfrParam->ccxLastAP.alDisassocTime = GetTickCount();
					DBGPRINT(DBG_ROAM|DBG_ASSO|DBG_HELP,(L"Disconnected, Ticks: %d \n", pfrParam->ccxLastAP.alDisassocTime));
				}
				NdisZeroMemory(&pfrParam->ccxCurrentAP, sizeof(CCX_AP_INFO));
			}
			///CCX_FASTROAM
		}
		else { // different then the previous one
			if ( choosedBSSIDIdxNum > 1 ) { 
				NdisMoveMemory(preSSID.Ssid, ((PNDIS_802_11_SSID)InformationBuffer)->Ssid,
					((PNDIS_802_11_SSID)InformationBuffer)->SsidLength);        
				preSSID.SsidLength = ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength;
				preIndex = 0;
			 }
		}
	}   // 35.p6: Added for check CCX

	// check if the matching SSID is found, if not, return
	if (! bSsidFound)
	{
		if (bHasHideSSID)
		{
			ulAttemptSSIDIdx = iHideSSID;
		}
		else
		{
			DBGPRINT(DBG_CMD,(L"HWAC - SSID is not in the list\n"));
			Adapter->bIsAssociateInProgress = FALSE;
///			ReturnCmdNode(Adapter, pTempCmd);
			///return NDIS_STATUS_FAILURE;
			return ulAttemptSSIDIdx;
		} 
	}
    
    //022607
    if( AssoByBSSIDindex != 0xff )
    { 
        ulAttemptSSIDIdx = AssoByBSSIDindex;
        ///RETAILMSG(1,(L"Found a matched SSID+BSSID pair:%x\r\n",AssoByBSSIDindex));
    }

	return ulAttemptSSIDIdx;
}

NDIS_STATUS
SetupAssociationExt (
	PHostCmd_DS_802_11_ASSOCIATE_EXT pAsso,
	USHORT PendingInfo,
	PMRVDRV_ADAPTER Adapter, 
	CmdCtrlNode *pTempCmd,
	PVOID InformationBuffer
  )
{
	ULONG ulAttemptSSIDIdx = (*(ULONG*)InformationBuffer);
	PNDIS_WLAN_BSSID_EX pBSSIDListSrc;
	PNDIS_802_11_FIXED_IEs  pFixedIE;
	PMRV_BSSID_IE_LIST  pBSSIDIEListSrc;
	PBSS_DESCRIPTION_SET_ALL_FIELDS   pBssDescListSrc;
	UCHAR               *curpos;
	MrvlIEtypes_SsIdParamSet_t    *ssid;
	MrvlIEtypes_PhyParamSet_t   *phy;
	MrvlIEtypes_SsParamSet_t    *ss;
	MrvlIEtypes_RatesParamSet_t   *rates;
	MrvlIEtypes_AuthType_t      *authtype;
	MrvlIEtypes_RsnParamSet_t   *rsn=NULL;  //Coverity Error id:19 (UNINIT)
	int                     size;
	///CCX_V4_S56
	MrvlIEtypes_Passthrough_t          *pass;
	///CCX_V4_S56
	MrvlIEtypes_WmmParamSet_t   *wmm;
	WPA_SUPPLICANT  *pwpa2_supplicant;
	UCHAR Mode;
	int     nPmkidIdx;
	PUCHAR      pOriWpa2Ie;


	Adapter->bIsAssociateInProgress = TRUE;
	///==========================================
	pBSSIDListSrc = Adapter->PSBSSIDList;
	pBSSIDIEListSrc = Adapter->PSIEBuffer;
	pBssDescListSrc = Adapter->PSBssDescList;

	wlan_ccx_setCurMAC( (&pBSSIDListSrc[ulAttemptSSIDIdx].Ssid), pBSSIDListSrc[ulAttemptSSIDIdx].MacAddress, Adapter->CurrentAddr);
	if (Adapter->pReconectArg != NULL) {
		BOOLEAN     isPrivay, isCCKM;

		if (pBSSIDListSrc[ulAttemptSSIDIdx].Privacy == Ndis802_11PrivFilter8021xWEP) {
			isPrivay = TRUE;
			DBGPRINT(DBG_CCX|DBG_ROAM|DBG_HELP,(L"This AP is WEP-Enabled\n"));
		} else {
			isPrivay = FALSE;
			DBGPRINT(DBG_CCX|DBG_ROAM|DBG_HELP,(L"This AP is WEP-Disabled\n"));
		}
		if (pBssDescListSrc[ulAttemptSSIDIdx].ccx_bss_info.cckmEnabled == TRUE) {
			isCCKM = TRUE;
			DBGPRINT(DBG_CCX|DBG_ROAM|DBG_HELP,(L"This AP is CCKM-Enabled\n"));
		} else {
			isCCKM = FALSE;
			DBGPRINT(DBG_CCX|DBG_ROAM|DBG_HELP,(L"This AP is CCKM-Disabled\n"));
		}
		wlan_recnnect_set_ap_param(Adapter->pReconectArg, isPrivay, isCCKM);
        }
        ///CCX_RADIOMEASURE
        Adapter->parentTSF = pBssDescListSrc[ulAttemptSSIDIdx].TimeStamp[0] |
                            pBssDescListSrc[ulAttemptSSIDIdx].TimeStamp[1] << 8 |
                            pBssDescListSrc[ulAttemptSSIDIdx].TimeStamp[2] << 16 |
                            pBssDescListSrc[ulAttemptSSIDIdx].TimeStamp[3] << 24;
        ///CCX_RADIOMEASURE
    if( Adapter->Enable80211D )   //++dralee_20060712
    {
      if (pBssDescListSrc[ulAttemptSSIDIdx].bHaveCountryIE)
      {
        UpdateScanTypeFromCountryIE( Adapter, (UCHAR) ulAttemptSSIDIdx);
        SetDomainInfo( Adapter, &(pBssDescListSrc[ulAttemptSSIDIdx].CountryInfo));
      } 
   }
   else
      UpdateScanTypeByCountryCode(Adapter, (UCHAR)Adapter->RegionCode ); 
   

    // size of general header (command size include the header 8 bytes)
   pAsso->Size = 8;  
   curpos = (PUCHAR)pAsso;

   // set the temporary BSSID Index
   Adapter->ulAttemptedBSSIDIndex = ulAttemptSSIDIdx;
   NdisMoveMemory(pAsso->PeerStaAddr, pBSSIDListSrc[ulAttemptSSIDIdx].MacAddress, 
                 MRVDRV_ETH_ADDR_LEN); 

   DBGPRINT(DBG_ASSO|DBG_HELP,(L"peer mac:%x,%x\r\n",pAsso->PeerStaAddr[4],pAsso->PeerStaAddr[5]));


  // set the Capability info
  pFixedIE = &(pBSSIDIEListSrc[ulAttemptSSIDIdx].FixedIE);
  NdisMoveMemory(&pAsso->CapInfo, &(pFixedIE->Capabilities), sizeof(IEEEtypes_CapInfo_t));  
    if ( Adapter->WmmDesc.required )
        ((IEEEtypes_CapInfo_t *)&pFixedIE->Capabilities)->QoS = 1;
    else
        ((IEEEtypes_CapInfo_t *)&pFixedIE->Capabilities)->QoS = 0;

  //dralee_20060613
  if( ((IEEEtypes_CapInfo_t *)&pFixedIE->Capabilities)->ShortPreamble )
     Adapter->Preamble = HostCmd_TYPE_SHORT_PREAMBLE;
  else
     Adapter->Preamble = HostCmd_TYPE_LONG_PREAMBLE;  
   
	// set the listen interval and Beacon period
	pAsso->ListenInterval = (USHORT)Adapter->LocalListenInterval;//MRVDRV_DEFAULT_LISTEN_INTERVAL; dralee_20060728
	pAsso->BcnPeriod = pBSSIDIEListSrc[ulAttemptSSIDIdx].FixedIE.BeaconInterval;
	pAsso->DtimPeriod = 0;  

	pAsso->Size += 13;
	curpos += pAsso->Size;  

	// ssid 
	ssid = (MrvlIEtypes_SsIdParamSet_t *)curpos;
	ssid->Header.Type = TLV_TYPE_SSID;
	ssid->Header.Len = (USHORT) (pBSSIDListSrc[ulAttemptSSIDIdx].Ssid.SsidLength);
	NdisMoveMemory(ssid->SsId, pBSSIDListSrc[ulAttemptSSIDIdx].Ssid.Ssid,
	                 pBSSIDListSrc[ulAttemptSSIDIdx].Ssid.SsidLength);
	pAsso->Size += (sizeof(MrvlIEtypesHeader_t) + ssid->Header.Len);
	curpos += (sizeof(MrvlIEtypesHeader_t) + ssid->Header.Len);

	// ds 
	phy = (MrvlIEtypes_PhyParamSet_t *)curpos;
	phy->Header.Type = TLV_TYPE_PHY_DS;
	phy->Header.Len = sizeof(phy->fh_ds.DsParamSet);
	NdisMoveMemory(&(phy->fh_ds.DsParamSet), &(pBssDescListSrc[ulAttemptSSIDIdx].PhyParamSet.DsParamSet.CurrentChan),
		sizeof(phy->fh_ds.DsParamSet));
	Adapter->frParam.assocChannel = pBssDescListSrc[ulAttemptSSIDIdx].PhyParamSet.DsParamSet.CurrentChan;
    
	curpos += (sizeof(phy->Header) + phy->Header.Len);
	pAsso->Size += (sizeof(phy->Header) + phy->Header.Len); 

	// cf : now we don't need to fill the field
	ss = (MrvlIEtypes_SsParamSet_t *)curpos;
	ss->Header.Type = TLV_TYPE_CF;
	ss->Header.Len = sizeof(ss->cf_ibss.CfParamSet);

	curpos += sizeof(ss->Header) + ss->Header.Len;
	pAsso->Size += (sizeof(ss->Header) + ss->Header.Len); 

	// rate
	rates = (MrvlIEtypes_RatesParamSet_t *)curpos;
	rates->Header.Type = TLV_TYPE_RATES;

    NdisMoveMemory (rates->Rates, &(pBSSIDListSrc[ulAttemptSSIDIdx].SupportedRates), G_SUPPORTED_RATES);
   {
        UCHAR  *card_rates;
        int    card_rates_size;
        int slen; 
        
        if (pBSSIDListSrc[ulAttemptSSIDIdx].Configuration.DSConfig > 5000)
        {
            card_rates = SupportedRates_A;
            card_rates_size = sizeof(SupportedRates_A);
        } 
        else //if (pBSSIDListSrc[ulAttemptSSIDIdx].Configuration.DSConfig > 2407)
        {
            card_rates = SupportedRates_G;
            card_rates_size = sizeof(SupportedRates_G);
        }

        get_common_rates(rates->Rates, G_SUPPORTED_RATES,
                           card_rates, card_rates_size);

        if( (slen = strlen(rates->Rates) ) > card_rates_size ) 
            rates->Header.Len = card_rates_size;
        else
            rates->Header.Len = (USHORT)slen;
    }

  curpos += (sizeof(rates->Header) + rates->Header.Len);
  pAsso->Size += (sizeof(rates->Header) + rates->Header.Len); 

  
  pAsso->Size += AppendTsfTlv(&curpos, &pBssDescListSrc[ulAttemptSSIDIdx]);   // <== Junius Modified

	// authType 
	authtype = (MrvlIEtypes_AuthType_t *)curpos;
	authtype->Header.Type = TLV_TYPE_AUTH_TYPE;
	authtype->Header.Len = (USHORT) sizeof(authtype->AuthType);
	authtype->AuthType = AuthModeMapping(Adapter->AuthenticationMode);
	///For CCX, If the CCX type has been set, use the type instead
	if (wlan_ccx_getEAPState() != 0) {
		authtype->AuthType = wlan_ccx_MapAuthMode(wlan_ccx_getEAPState());
	}
	pAsso->Size += (sizeof(MrvlIEtypesHeader_t) + authtype->Header.Len);
	curpos += (sizeof(MrvlIEtypesHeader_t) + authtype->Header.Len);

	{
	ULONGLONG       TmStamp_little;
	UCHAR               *datpt;

	NdisMoveMemory(&TmStamp_little, pBssDescListSrc[ulAttemptSSIDIdx].TimeStamp, sizeof(ULONGLONG));
	TmStamp_little += (pBssDescListSrc[ulAttemptSSIDIdx].ccx_bss_info.CurrentTSF - pBssDescListSrc[ulAttemptSSIDIdx].networkTSF);
	datpt = (UCHAR*)&TmStamp_little;
	DBGPRINT(DBG_CCX, (L"TmStamp_little: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n", 
                                            datpt[0], datpt[1], datpt[2], datpt[3], 
                                            datpt[4], datpt[5], datpt[6], datpt[7]));
	datpt = (UCHAR*)&pBssDescListSrc[ulAttemptSSIDIdx].ccx_bss_info.CurrentTSF;
	DBGPRINT(DBG_CCX, (L"CurrentTSF: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n", 
                                            datpt[0], datpt[1], datpt[2], datpt[3], 
                                            datpt[4], datpt[5], datpt[6], datpt[7]));
	datpt = (UCHAR*)&pBssDescListSrc[ulAttemptSSIDIdx].networkTSF;
	DBGPRINT(DBG_CCX, (L"networkTSF: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n", 
                                            datpt[0], datpt[1], datpt[2], datpt[3], 
                                            datpt[4], datpt[5], datpt[6], datpt[7]));
	DBGPRINT(DBG_CCX, (L"MAC: %02x-%02x-%02x-%02x-%02x-%02x\n", 
                                            pBssDescListSrc[ulAttemptSSIDIdx].BSSID[0],
                                            pBssDescListSrc[ulAttemptSSIDIdx].BSSID[1],
                                            pBssDescListSrc[ulAttemptSSIDIdx].BSSID[2],
                                            pBssDescListSrc[ulAttemptSSIDIdx].BSSID[3],
                                            pBssDescListSrc[ulAttemptSSIDIdx].BSSID[4],
                                            pBssDescListSrc[ulAttemptSSIDIdx].BSSID[5]));
                                            
	if (Adapter->MediaConnectStatus == NdisMediaStateDisconnected ) {
		pBssDescListSrc[ulAttemptSSIDIdx].ccx_bss_info.isMediaConnected = FALSE;
	} else {
		pBssDescListSrc[ulAttemptSSIDIdx].ccx_bss_info.isMediaConnected = TRUE;
	}
	size = wlan_ccx_process_association_req(&curpos, &(pBssDescListSrc[ulAttemptSSIDIdx].ccx_bss_info), 
                    &(pBssDescListSrc[ulAttemptSSIDIdx].wpa_supplicant), (UCHAR*)&TmStamp_little, TRUE, ulAttemptSSIDIdx);
	DBGPRINT(DBG_CCX, (L"returned from wlan_ccx_process_association_req(%d)\r\n", size));
	///CCX_CCKM
	if ( size < 0 ) { // This is CCKM
		Adapter->bIsAssociateInProgress = FALSE;
		DBGPRINT(DBG_CCX, (L"[SetupAssociationExt] return NDIS_STATUS_SUCCESS for CCKM\n"));
		return NDIS_STATUS_SUCCESS;
	}
	///CCX_CCKM
	pAsso->Size += size;
	}


    // if using one of the WPA authentication mode
    if ( (FW_IS_WPA_ENABLED(Adapter)) && 
         ((Adapter->AuthenticationMode == Ndis802_11AuthModeWPAPSK) ||
          (Adapter->AuthenticationMode == Ndis802_11AuthModeWPA)    ||
        (Adapter->AuthenticationMode == Ndis802_11AuthModeWPANone)||
          (Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2)   ||  
          (Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2PSK)) )  
	{
		PNDIS_802_11_ASSOCIATION_INFORMATION pAdapterAssoInfo;
		WPA_SUPPLICANT  *pwpa_supplicant;
      
		//DBGPRINT(DBG_CMD, ("ASSO_EXT: wpa_supplicant.Wpa_ie_len =%d \n",pBssDescListSrc[ulAttemptSSIDIdx].wpa_supplicant.Wpa_ie_len));
		if ((Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2)   ||  
            (Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2PSK) ) 
        {
			//Plus++, 012606(for Broadcom mix mode fix)
			//pwpa_supplicant = &(pBssDescListSrc[ulAttemptSSIDIdx].wpa2_supplicant); 
			pwpa2_supplicant = &(pBssDescListSrc[ulAttemptSSIDIdx].wpa2_supplicant); 
			pwpa_supplicant = &Adapter->wpa2_supplicant;

			if(Adapter->EncryptionStatus == Ndis802_11Encryption3Enabled ||Adapter->EncryptionStatus == Ndis802_11Encryption3KeyAbsent )
              Mode = RSN_IE_AES;
			else
              Mode = RSN_IE_TKIP;  
			pwpa_supplicant->Wpa_ie_len = Wpa2RsnIeAdjust(&pwpa_supplicant->Wpa_ie[0],
                                        &pwpa2_supplicant->Wpa_ie[0], pwpa2_supplicant->Wpa_ie_len, Mode);
			//Plus--, 012606            
		}   
        else
        {
         //     pwpa_supplicant = &(pBssDescListSrc[ulAttemptSSIDIdx].wpa_supplicant);   //Junius remove 20071017
         //Junius Added 20071017
              pwpa2_supplicant = &(pBssDescListSrc[ulAttemptSSIDIdx].wpa_supplicant); 
              pwpa_supplicant = &Adapter->wpa_supplicant;
		if(Adapter->EncryptionStatus == Ndis802_11Encryption3Enabled ||Adapter->EncryptionStatus == Ndis802_11Encryption3KeyAbsent )
              Mode = RSN_IE_AES;
            else
              Mode = RSN_IE_TKIP;  
                pwpa_supplicant->Wpa_ie_len = WpaIeAdjust(&pwpa_supplicant->Wpa_ie[0],
                                        &pwpa2_supplicant->Wpa_ie[0], pwpa2_supplicant->Wpa_ie_len, Mode);
              Adapter->wpa2_supplicant.Wpa_ie_len = 0; //Junius Added 20071017    
         //end added	
        }   

		//Change for WPS
		// In WPS spec v1.0h page 51,
		// if client does not include WPS IE in association request,
		// it must send Authentication frame to open and
		// Association Request frame "without" an RSN IE or SSN IE
		if (pwpa_supplicant->Wpa_ie_len
			&& (Adapter->TCloseWZCFlag == WZC_Default || Adapter->TCloseWZCFlag == WZC_Ignore_Send_EAPOL_START)
        )
        { 
          DBGPRINT(DBG_CMD, (L"ASSO_EXT: fill the WPA IE \n"));
          rsn = (MrvlIEtypes_RsnParamSet_t *)curpos;
          rsn->Header.Type = (USHORT)(pwpa_supplicant->Wpa_ie[0]);  
          rsn->Header.Len = (USHORT)(pwpa_supplicant->Wpa_ie[1]);
          NdisMoveMemory(rsn->RsnIE, &(pwpa_supplicant->Wpa_ie[2]), rsn->Header.Len);
        
          curpos += (sizeof(rsn->Header) + rsn->Header.Len);
          pAsso->Size += (sizeof(rsn->Header) + rsn->Header.Len); 
        }
        /*
            Driver appends pmkid only if it is in WPA2 authentication mode.
        */
        if ( Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2 )
        {
//          int     nPmkidIdx;

            DBGPRINT( DBG_MACEVT, (L"    Check PMKID cache\n") );
            HexDump( DBG_MACEVT, "    BSSID: ", pBSSIDListSrc[Adapter->ulAttemptedBSSIDIndex].MacAddress, 6 );
            
            nPmkidIdx = FindPmkidInCache( Adapter, pBSSIDListSrc[Adapter->ulAttemptedBSSIDIndex].MacAddress );
            if ( nPmkidIdx >= 0 )
            {
                DBGPRINT( DBG_MACEVT, (L"    ASSOC_EXT: Append PMKID\n") );
                DBGPRINT( DBG_MACEVT, (L"    Original EID=%d, IELen=%d\n", rsn->Header.Type, rsn->Header.Len) );
                
                curpos[0] = 1;
                curpos[1] = 0;
                NdisMoveMemory( &curpos[2], Adapter->PmkidCache[nPmkidIdx].pmkid, LEN_OF_PMKID );

                rsn->Header.Len += ( 2 + LEN_OF_PMKID );

                curpos += ( 2 + LEN_OF_PMKID );
                pAsso->Size += ( 2 + LEN_OF_PMKID );
                
                DBGPRINT( DBG_MACEVT, (L"    Modified EID=%d, IELen=%d\n", rsn->Header.Type, rsn->Header.Len) );

                HexDump(DBG_MACEVT, "\n    >>>>> SetupAssocExt dump -- PMKID had been added", (UCHAR *)rsn,     
                            (sizeof(rsn->Header) + rsn->Header.Len));

            }
        }
        else
        {
            nPmkidIdx = -1;
        }

    // reset the cap and interval info according to OID_802_11_ASSOCIATION_INFO
        pAdapterAssoInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)
                            Adapter->AssocInfoBuffer;
        NdisZeroMemory(pAdapterAssoInfo, MRVDRV_ASSOCIATE_INFO_BUFFER_SIZE);

    pAdapterAssoInfo->Length = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);
    pAdapterAssoInfo->OffsetRequestIEs = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);
      
    // add the default value
        pAdapterAssoInfo->AvailableRequestFixedIEs |= NDIS_802_11_AI_REQFI_CAPABILITIES;
    NdisMoveMemory (&pAdapterAssoInfo->RequestFixedIEs.Capabilities, 
            &(pAsso->CapInfo), 
            sizeof(IEEEtypes_CapInfo_t));
        
        // add the default value
        pAdapterAssoInfo->AvailableRequestFixedIEs |= NDIS_802_11_AI_REQFI_LISTENINTERVAL; 
        pAdapterAssoInfo->RequestFixedIEs.ListenInterval = pAsso->ListenInterval;
        
        // TODO: not sure what to do with CurrentAPAddress Field!
        pAdapterAssoInfo->AvailableRequestFixedIEs |= NDIS_802_11_AI_REQFI_CURRENTAPADDRESS;
        NdisMoveMemory(pAdapterAssoInfo->RequestFixedIEs.CurrentAPAddress,
                       pAsso->PeerStaAddr,
                       sizeof(NDIS_802_11_MAC_ADDRESS));

    // add SSID, supported rates, and wpa IE info associate request
        {
            //UCHAR tmpBuf[500];
            ULONG ulReqIELen = 0;
            ULONG ulCurOffset = 0;
            UCHAR ucElemID;
            UCHAR ucElemLen;
            PUCHAR pIEBuf, pRequestIEBuf;


            pIEBuf = pBSSIDIEListSrc[Adapter->ulAttemptedBSSIDIndex].VariableIE;
            pRequestIEBuf = Adapter->AssocInfoBuffer + pAdapterAssoInfo->OffsetRequestIEs;

            DBGPRINT( DBG_WPA, (L"Variable IE Length is %d\n", 
                    pBSSIDListSrc[Adapter->ulAttemptedBSSIDIndex].IELength - 
                    MRVL_FIXED_IE_SIZE));

			Adapter->WPAEnabled = FALSE;
            while ( ulCurOffset < 
                   (pBSSIDListSrc[Adapter->ulAttemptedBSSIDIndex].IELength -
                   MRVL_FIXED_IE_SIZE) )
            {
                ucElemID = *(pIEBuf + ulCurOffset);
                ucElemLen = *(pIEBuf + ulCurOffset + 1);
                

                // wpa, supported rate, or wpa
                 //Change for WPS 
                 // In WPS spec v1.0h page 51,
                 // if client does not include WPS IE in association request,
                 // it must send Authentication frame to open and
                 // Association Request frame "without" an RSN IE or SSN IE
                 
                // ssid, supported rate IE
                if ((ucElemID == SSID) || (ucElemID == SUPPORTED_RATES))
                {
                    NdisMoveMemory(pRequestIEBuf + ulReqIELen,
                                   pIEBuf + ulCurOffset,
                                   ucElemLen + 2 );
                    ulReqIELen += ( ucElemLen + 2); 
                }
                // wpa, rsn IE
                else if ((ucElemID == WPA_IE || ucElemID == WPA2_IE)
                         && (Adapter->TCloseWZCFlag == WZC_Default || Adapter->TCloseWZCFlag == WZC_Ignore_Send_EAPOL_START)
                )
				{
					// copy the IE to association information buffer
					if((ucElemID == WPA2_IE) &&  
						(Adapter->wpa2_supplicant.Wpa_ie_len > 0 ) &&
						((Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2) ||
						(Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2PSK )))
					{
						NdisMoveMemory(pRequestIEBuf + ulReqIELen,
                                    &Adapter->wpa2_supplicant.Wpa_ie[0],
                                    Adapter->wpa2_supplicant.Wpa_ie_len);
						ulReqIELen += Adapter->wpa2_supplicant.Wpa_ie_len; 
					}                               
					else
					{
						//Junius Added 20071017
						if ((ucElemID == WPA_IE) && 
							(Adapter->wpa_supplicant.Wpa_ie_len > 0 ))
                      	{
							DBGPRINT(DBG_ERROR,(L"Test 2\n"));
							NdisMoveMemory(pRequestIEBuf + ulReqIELen,
									&Adapter->wpa_supplicant.Wpa_ie[0],
									Adapter->wpa_supplicant.Wpa_ie_len);
							ulReqIELen += Adapter->wpa_supplicant.Wpa_ie_len;			  
						}
						else
						{
						//end added
						//original           
							NdisMoveMemory( pRequestIEBuf + ulReqIELen,
                                         pIEBuf + ulCurOffset,
                                         ucElemLen + 2 );
							ulReqIELen += ( ucElemLen + 2); 
						} //junius Added 20071017	
					}

					DBGPRINT(DBG_WPA, (L"Copied elemID = 0x%x, %d bytes into request IE\n",
                        ucElemID, ulReqIELen));

					{
					PFASTROAMPARAM      pfrParam = &Adapter->frParam;
					pfrParam->is80211x = TRUE;
					}
					
					if ( ucElemID == WPA2_IE )
					{
						if ( nPmkidIdx >= 0 )
						{
							pOriWpa2Ie = pRequestIEBuf + ulReqIELen - ( ucElemLen + 2);
							DBGPRINT( DBG_MACEVT, (L"    Append PMKID to association buffer\n") );
							DBGPRINT( DBG_MACEVT, (L"    . Original Len=%d, Current IE=%d, ucElemLen=%d\n", pOriWpa2Ie[1], pOriWpa2Ie[0], ucElemLen ) );

							pRequestIEBuf[ulReqIELen] = 1;
							pRequestIEBuf[ulReqIELen+1] = 0;
							NdisMoveMemory( pRequestIEBuf+ulReqIELen+2,
							Adapter->PmkidCache[nPmkidIdx].pmkid, LEN_OF_PMKID);
							ulReqIELen += (LEN_OF_PMKID+2);
							pOriWpa2Ie[1] += (LEN_OF_PMKID+2);
							DBGPRINT( DBG_MACEVT, (L"    . New Len=%d, New ReqIeLen=%d\n", pOriWpa2Ie[1], ulReqIELen ) );
							HexDump( DBG_MACEVT, "    === Assoc Buffer Dump ===", pRequestIEBuf, ulReqIELen ); //tt
						}
					}

                    HexDump(DBG_WPA, "RequestIE:", pRequestIEBuf, 32);
					Adapter->WPAEnabled = TRUE;
				}
				ulCurOffset = ulCurOffset + 2 + ucElemLen;
            }
			pAdapterAssoInfo->RequestIELength = ulReqIELen;
			pAdapterAssoInfo->OffsetResponseIEs = pAdapterAssoInfo->OffsetRequestIEs + 
                                                  pAdapterAssoInfo->RequestIELength;
        }
		
	}
    DBGPRINT(DBG_ASSO|DBG_CMD|DBG_HELP,(L"+wmm+ wmm required=%d, wmm_ie=%d\n", Adapter->WmmDesc.required, pBssDescListSrc[ulAttemptSSIDIdx].Wmm_IE[0]) );
    DBGPRINT(DBG_ASSO|DBG_CMD|DBG_HELP,(L"[Mrvl] wep=%d, encry=%d\n", Adapter->WEPStatus, Adapter->EncryptionStatus) );
    if ( Adapter->WEPStatus == Ndis802_11WEPEnabled && Adapter->EncryptionStatus != Ndis802_11WEPEnabled ) // tt 8021x+wep
    {
        Adapter->WEPStatus = Ndis802_11WEPKeyAbsent;
    }
    
    if(Adapter->WmmDesc.required && (pBssDescListSrc[ulAttemptSSIDIdx].Wmm_IE[0]==WPA_IE))
    {


    wmm = (MrvlIEtypes_WmmParamSet_t *)curpos;
    wmm->Header.Type = (USHORT)(WMM_IE[0]); 
    wmm->Header.Len = (USHORT)(WMM_IE[1]);
    NdisMoveMemory(wmm->WmmIE, &(WMM_IE[2]), wmm->Header.Len);
    curpos += sizeof(wmm->Header) + wmm->Header.Len;
    pAsso->Size += (sizeof(wmm->Header) + wmm->Header.Len); 
  
        if((pBssDescListSrc[ulAttemptSSIDIdx].Wmm_IE[WMM_QOS_INFO_OFFSET] & WMM_QOS_INFO_UAPSD_BIT) && ((Adapter->WmmDesc.qosinfo & 0x0f) != 0))
     {
            DbgWmmMsg( (L"+wmm+ Append UAPSD QosInfo = 0x%x\n", Adapter->WmmDesc.qosinfo ));
            NdisMoveMemory((UCHAR*)wmm->WmmIE + wmm->Header.Len - 1,(UCHAR*)&Adapter->WmmDesc.qosinfo, 1);
     }

    }

                        
    if (Adapter->bHasTspec == TRUE)
    {
        DBGPRINT( DBG_CCX_V4, (L" CCX_V4 Append TSPEC IE after associate command\n") );
			
        pass = (MrvlIEtypes_Passthrough_t *)curpos;
        pass->Header.Type = 0x10a; 
        pass->Header.Len = Adapter->CurrentTspec[1]+2;
        NdisMoveMemory(pass->Data, &Adapter->CurrentTspec[0], pass->Header.Len);

        curpos += sizeof(pass->Header) + pass->Header.Len;
        pAsso->Size += (sizeof(pass->Header) + pass->Header.Len); 
    }
                        

                        
  return NDIS_STATUS_SUCCESS;
}


/******************************************************************************
 *
 *  Name: SetupAdHocStart()
 *
 *  Description: Setup adHoc Start
 *
 *  Arguments:  PHostCmd_DS_802_11_AD_HOC_START pAdHocStart
 *              NDIS_OID PendingInfo 
 *              PMRVDRV_ADAPTER Adapter
 *              CmdCtrlNode *pTempCmd
 *              PVOID InformationBuffer
 *    
 *  Return Value:
 * 
 *  Notes:
 *
 *****************************************************************************/
VOID SetupAdHocStart(
    PHostCmd_DS_802_11_AD_HOC_START pAdHocStart,
    USHORT PendingInfo,
    PMRVDRV_ADAPTER Adapter, 
    CmdCtrlNode *pTempCmd,
    PVOID InformationBuffer
)
{
    ULONG i;
    PNDIS_WLAN_BSSID_EX pBSSIDListSrc;


    pBSSIDListSrc = Adapter->PSBSSIDList;
    i = Adapter->ulPSNumOfBSSIDs;
    Adapter->ulPSNumOfBSSIDs++;

    DBGPRINT(DBG_CMD|DBG_ADHOC|DBG_WARNING,(L">>> SetupAdHocStart()\n"));
   
    // Add a new entry in the BSSID list
    Adapter->ulAttemptedBSSIDIndex = i;
   
    NdisZeroMemory(pAdHocStart->SSID, MRVDRV_MAX_SSID_LENGTH);
    NdisMoveMemory(pAdHocStart->SSID,
                   ((PNDIS_802_11_SSID)InformationBuffer)->Ssid, 
                   ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength);
  
    DBGPRINT(DBG_ADHOC, (L"*** Adhoc Start - SSID = %S ***\n", 
             pAdHocStart->SSID)); 
  
    NdisMoveMemory(Adapter->PSBssDescList[i].SSID,
                   ((PNDIS_802_11_SSID)InformationBuffer)->Ssid, 
                   ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength);
    Adapter->PSBssDescList[i].BSSType = HostCmd_BSS_TYPE_IBSS;
   
    NdisMoveMemory(pBSSIDListSrc[i].Ssid.Ssid,
                   ((PNDIS_802_11_SSID)InformationBuffer)->Ssid, 
                   ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength);

    pBSSIDListSrc[i].Ssid.SsidLength = 
        ((PNDIS_802_11_SSID)InformationBuffer)->SsidLength;
    
    // Set the length of Adapter->BSSIDList[i]
    pBSSIDListSrc[i].Length = sizeof(NDIS_WLAN_BSSID_EX);
  
    // Set the length of configuration in Adapter->BSSIDList[i]
    pBSSIDListSrc[i].Configuration.Length  = sizeof(NDIS_802_11_CONFIGURATION);

    // Set the BSS type
    pAdHocStart->BSSType = HostCmd_BSS_TYPE_IBSS;
  
    //030507
    pBSSIDListSrc[i].IELength = 0;

    pBSSIDListSrc[i].InfrastructureMode = Ndis802_11IBSS;

    // set Physical param set
    pAdHocStart->PhyParamSet.DsParamSet.ElementId = 3;
    pAdHocStart->PhyParamSet.DsParamSet.Len       = 1;

    //dralee_20060526
    NdisZeroMemory( pAdHocStart->BasicDataRates, sizeof(pAdHocStart->BasicDataRates));

    switch(Adapter->AdhocDefaultBand)
    {
        case MRVDRV_802_11_BAND_A:
            NdisMoveMemory(pAdHocStart->BasicDataRates, 
                           SupportedRates_A, 
                           sizeof(SupportedRates_A));
            break;           

        case MRVDRV_802_11_BAND_B:
            NdisMoveMemory(pAdHocStart->BasicDataRates, 
                           SupportedRates_B, 
                           sizeof(SupportedRates_B));
            break;           

       case MRVDRV_802_11_BAND_BG:
       default:
            NdisMoveMemory(pAdHocStart->BasicDataRates, 
                           SupportedRates_G, 
                           sizeof(SupportedRates_G));  
        {
            DBGPRINT(DBG_ADHOC,(L"[MRVL] Enabled G rate for Adhoc mode\n"));
            DownloadGProtectSetting( Adapter );
        }
        break;            
    }

    // for Wifi test : the starter must only support B band daterate 
    if (Adapter->AdhocWiFiDataRate)
    {  
        NdisZeroMemory(pAdHocStart->BasicDataRates, sizeof(pAdHocStart->BasicDataRates));
  
        NdisMoveMemory(pAdHocStart->BasicDataRates, 
                       AdhocRates_B, 
                       sizeof(AdhocRates_B));
    }

    Adapter->Channel = (USHORT)Adapter->AdhocDefaultChannel;
  
    pAdHocStart->PhyParamSet.DsParamSet.CurrentChan = (UCHAR)Adapter->Channel;

    // Set IBSS param set 
    pAdHocStart->SsParamSet.IbssParamSet.ElementId = 6;
    pAdHocStart->SsParamSet.IbssParamSet.Len       = 2;
    pAdHocStart->SsParamSet.IbssParamSet.AtimWindow = (USHORT)Adapter->CurrentConfiguration.ATIMWindow; //0; ++dralee_20060412
  
    DBGPRINT(DBG_CMD|DBG_ADHOC|DBG_HELP, (L"Atim setting:%x\r\n",
             Adapter->CurrentConfiguration.ATIMWindow));
                           
    // Set Capability info
    pAdHocStart->Cap.Ess= 0;  //Ling++, 012706
    pAdHocStart->Cap.Ibss = 1;
    Adapter->PSBssDescList[i].Cap.Ibss =1;
     
    //dralee_20060613
    pAdHocStart->Cap.ShortPreamble = 1;
    Adapter->Preamble = HostCmd_TYPE_AUTO_PREAMBLE;

    // ProbeDelay
    pAdHocStart->ProbeDelay = HostCmd_SCAN_PROBE_DELAY_TIME; 

    // Set up privacy in Adapter->BSSIDList[i]
    if( Adapter->WEPStatus == Ndis802_11WEPEnabled 
     || Adapter->AdhocAESEnabled
    )
    {
        DBGPRINT(DBG_ADHOC, (L"*** Set Privacy bit ***\r\n"));
        pBSSIDListSrc[i].Privacy = Ndis802_11PrivFilter8021xWEP;
        pAdHocStart->Cap.Privacy = 1;
    }
    else
    {
        DBGPRINT(DBG_ADHOC, (L"*** Clear Privacy bit ***\r\n"));
        pBSSIDListSrc[i].Privacy = Ndis802_11PrivFilterAcceptAll;   
    }


    // Get ready to start an ad hoc network
    Adapter->bIsAssociateInProgress = TRUE;

    // Need to clean up Rx and Tx first
    CleanUpSingleTxBuffer(Adapter);
    ResetRxPDQ(Adapter);
    ResetSingleTxDoneAck(Adapter);
    
    NdisMoveMemory(&(Adapter->PSBssDescList[i].PhyParamSet),
                   &(pAdHocStart->PhyParamSet), 
                   sizeof(IEEEtypes_PhyParamSet_t));

    NdisMoveMemory(&(Adapter->PSBssDescList[i].SsParamSet),
                   &(pAdHocStart->SsParamSet), 
                   sizeof(IEEEtypes_SsParamSet_t));
 
    //030507
    switch(Adapter->AdhocDefaultBand)
    {
        case MRVDRV_802_11_BAND_A:
            pBSSIDListSrc[i].NetworkTypeInUse = Ndis802_11OFDM5;
            pBSSIDListSrc[i].Configuration.DSConfig = 5000 + (Adapter->Channel * 5);
            break;           
        case MRVDRV_802_11_BAND_B:
            pBSSIDListSrc[i].NetworkTypeInUse = Ndis802_11DS;
            pBSSIDListSrc[i].Configuration.DSConfig = 2407 + (Adapter->Channel * 5);
            break;           
        case MRVDRV_802_11_BAND_BG:
        default:
            //030507 
            pBSSIDListSrc[i].NetworkTypeInUse =Ndis802_11OFDM24;
            pBSSIDListSrc[i].Configuration.DSConfig = 2407 + (Adapter->Channel * 5);
            break;            
    }

    //pBSSIDListSrc[i].NetworkTypeInUse = Ndis802_11DS;

    //030507
    //pBSSIDListSrc[i].Configuration.DSConfig = 2407 + (Adapter->Channel * 5); 
    //DSFeqList[pAdHocStart->PhyParamSet.DsParamSet.CurrentChan];

    pBSSIDListSrc[i].Rssi = -10;    //set default.
    pBSSIDListSrc[i].Configuration.ATIMWindow = 
        pAdHocStart->SsParamSet.IbssParamSet.AtimWindow;



    return;
}

/******************************************************************************
 *
 *  Name: SetupAdHocJoin()
 *
 *  Description: Setup adHoc Join
 *
 *  Arguments:  PHostCmd_DS_802_11_AD_HOC_JOIN pAdHocJoin
 *              NDIS_OID PendingInfo 
 *              PMRVDRV_ADAPTER Adapter
 *              CmdCtrlNode *pTempCmd
 *              PVOID InformationBuffer
 *    
 *  Return Value:
 * 
 *  Notes:
 *
 *****************************************************************************/
NDIS_STATUS
SetupAdHocJoin(
    PHostCmd_DS_802_11_AD_HOC_JOIN pAdHocJoin,
    USHORT PendingInfo,
    PMRVDRV_ADAPTER Adapter, 
    CmdCtrlNode *pTempCmd,
    PVOID InformationBuffer
)
{
    PNDIS_WLAN_BSSID_EX pBSSIDDesc = (PNDIS_WLAN_BSSID_EX)InformationBuffer;
    BSS_DESCRIPTION_SET_ALL_FIELDS *PSBssDesc = &Adapter->PSBssDescList[Adapter->ulAttemptedBSSIDIndex];


#define USE_G_PROTECTION    0x02    
    if ( PSBssDesc->ERPFlags & USE_G_PROTECTION  && Adapter->AdhocWiFiDataRate == 0 )
        Adapter->CurrentMacControl |= HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON;
    else
        Adapter->CurrentMacControl &= ~HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON;

    DownloadGProtectSetting(Adapter);
    
    // v9 adhoc join
    // reset Join cmd data structure
    NdisZeroMemory(&(pAdHocJoin->BssDescriptor), sizeof(pAdHocJoin->BssDescriptor));

    // copy memory (from BSSID to CapInfo) to Adhoc Join cmd data structure
    NdisMoveMemory(&(pAdHocJoin->BssDescriptor),
                   &(Adapter->PSBssDescList[Adapter->ulAttemptedBSSIDIndex]),
                   FIELD_OFFSET(BSS_DESCRIPTION_SET_ALL_FIELDS, DataRates));

    // copy data rate
    if ( Adapter->AdhocWiFiDataRate )
    {
        NdisMoveMemory(pAdHocJoin->BssDescriptor.DataRates, 
                       AdhocRates_B, 
                       sizeof(AdhocRates_B));
    }
    else
    {
        NdisMoveMemory(pAdHocJoin->BssDescriptor.DataRates, 
                       AdhocRates_G, 
                       sizeof(AdhocRates_G));
    }

    // MSFT
    // Set the temporary BSSID Index
    // Adapter->ulAttemptedBSSIDIndex = i;

    // information on BSSID descriptor passed to FW
    DBGPRINT(DBG_V9|DBG_CUSTOM|DBG_ADHOC, (L"*** Adhoc Join - BSSID = %2x-%2x-%2x-%2x-%2x-%2x , SSID = %s ***\n", 
                pAdHocJoin->BssDescriptor.BSSID[0],
                pAdHocJoin->BssDescriptor.BSSID[1],
                pAdHocJoin->BssDescriptor.BSSID[2],
                pAdHocJoin->BssDescriptor.BSSID[3],
                pAdHocJoin->BssDescriptor.BSSID[4],
                pAdHocJoin->BssDescriptor.BSSID[5],
                pAdHocJoin->BssDescriptor.SSID)); 

    // FailTimeOut
    pAdHocJoin->FailTimeOut = MRVDRV_ASSOCIATION_TIME_OUT;  //2 sec in TU
    pAdHocJoin->ProbeDelay = HostCmd_SCAN_PROBE_DELAY_TIME; 

    pAdHocJoin->BssDescriptor.BSSType = HostCmd_BSS_TYPE_IBSS;
    pAdHocJoin->BssDescriptor.BeaconPeriod = (USHORT)pBSSIDDesc->Configuration.BeaconPeriod;

    if (Adapter->WEPStatus == Ndis802_11WEPEnabled
        || Adapter->AdhocAESEnabled
    )
    {
        DBGPRINT(DBG_ADHOC ,(L"*** Set Privacy bit ***\r\n"));
        pAdHocJoin->BssDescriptor.Cap.Privacy = 1;   
    }


    // Get ready to join
    Adapter->bIsAssociateInProgress = TRUE;

    // Need to clean up Rx and Tx first
    CleanUpSingleTxBuffer(Adapter);
    ResetRxPDQ(Adapter);
  
    // Need to report disconnect event if currently associated
    if ( Adapter->MediaConnectStatus == NdisMediaStateConnected )
        ResetDisconnectStatus(Adapter);                              

    //dralee_20060613         
    if( pAdHocJoin->BssDescriptor.Cap.ShortPreamble == 1 )
        Adapter->Preamble = HostCmd_TYPE_SHORT_PREAMBLE;
    else
        Adapter->Preamble = HostCmd_TYPE_LONG_PREAMBLE; 

    return NDIS_STATUS_SUCCESS;
}

///Select the type to update the AP table
static void UpdateAPTableType(PMRVDRV_ADAPTER Adapter)
{
    //051707 don't clean up table, just add a tag for age out
    // clean up the list

    if ((Adapter->SetActiveScanSSID == FALSE) || 
         (Adapter->bIsAssociationBlockedByScan == FALSE))
    {
        //051707 age out all entries 060407
        //Adapter->ulPSNumOfBSSIDs = 0;
        AgeOutTheActiveSsidEntry(Adapter, &Adapter->ActiveScanSSID, AGEOUT_ALL);
        goto FuncFinal;
    }
    if ((Adapter->SpecifyChannel > 0) && (Adapter->SetActiveScanSSID == TRUE)) {
        AgeOutTheActiveSsidEntry(Adapter, &Adapter->ActiveScanSSID, AGEOUT_CHNL);
        goto FuncFinal;
    }
    
    //050307 age out active scan entries 060407
    //RemoveTheActiveSsidEntry(Adapter, &Adapter->ActiveScanSSID);  
    AgeOutTheActiveSsidEntry(Adapter, &Adapter->ActiveScanSSID, AGEOUT_SSID);

 FuncFinal:
    return;
}



NDIS_STATUS
SetupScanCommand (
    PMRVDRV_ADAPTER Adapter
)
{
  HostCmd_DS_802_11_SCAN      *pScanCMD;  
  MrvlIEtypes_SsIdParamSet_t    *ssid;
  MrvlIEtypes_ChanListParamSet_t  *chan;
  ChanScanParamSet_t              *chaninfo; 
  MrvlIEtypes_RatesParamSet_t   *rates;
  MrvlIEtypes_NumProbes_t         *scanprobes;   

  PREGION_CHANNEL                 region_channel;
  USHORT              i,j,k,cmd_index;
  USHORT              TotalChan;
    USHORT              ScanChannel=1;  //051107 Coverity
        CmdCtrlNode           *pTempScanCmd[40];
  
  
  
  BOOLEAN      bBgScan;

        bBgScan= Adapter->bBgScanEnabled;
        if (bBgScan==TRUE)
        {
              EnableBgScan( Adapter, FALSE);
        }
  
  // scan in progress --> set the flag 
  Adapter->bIsScanInProgress = TRUE;
  //060407

    //Select the type to update the AP table
    UpdateAPTableType(Adapter);

  // count the channels need to scan 
  TotalChan = 0;
  for ( i=0; i < MAX_REGION_BAND_NUM; i++ ) 
  { 
      region_channel = &(Adapter->region_channel[i]);
      if (region_channel->Valid == TRUE)
      TotalChan += region_channel->NrCFP;
  } 
  DBGPRINT(DBG_V9,(L">>> SCAN command TotalChan = %d\n", TotalChan));

  // no channel need to be scaned 
  if (TotalChan == 0)
    { 
        Adapter->bIsScanInProgress = FALSE;  
      return NDIS_STATUS_SUCCESS;
    }
  
  cmd_index=0;  
  
  for ( i=0; i < MAX_REGION_BAND_NUM; i++ )
  {
    region_channel = &(Adapter->region_channel[i]);

    if (region_channel->Valid == FALSE)
      continue;

        DBGPRINT(DBG_V9,(L"Scan Band %d \n", region_channel->Band));          
    j=0;
    
      while ( j< region_channel->NrCFP)
      { 
      pTempScanCmd[cmd_index] = GetFreeCmdCtrlNode(Adapter);

      if ( pTempScanCmd[cmd_index] == NULL )
      {
              DBGPRINT(DBG_ERROR,(L">>> Get Free CmdNode Fail \n"));
		Adapter->bIsScanInProgress = FALSE;  
              return NDIS_STATUS_FAILURE;
            }

            // setup the command
            pTempScanCmd[cmd_index]->ExpectedRetCode = GetExpectedRetCode(HostCmd_CMD_802_11_SCAN);
            pScanCMD = (PHostCmd_DS_802_11_SCAN)pTempScanCmd[cmd_index]->BufVirtualAddr;
            
            // setup scan command and parameter
            pScanCMD->SeqNum  = (Adapter->SeqNum++);
            pScanCMD->Command = HostCmd_CMD_802_11_SCAN;
            pScanCMD->BSSType = HostCmd_BSS_TYPE_ANY;
            pScanCMD->Size    = 15;
        
            if (Adapter->bIsAssociationBlockedByScan)
              pTempScanCmd[cmd_index]->Pad[1] |= MRVDRV_ES_ASSOCIATIONBLOCKED;
            
          if (Adapter->SetActiveScanBSSID) 
            {
        NdisMoveMemory (pScanCMD->BSSID,
                Adapter->ActiveScanBSSID,
                MRVDRV_ETH_ADDR_LEN);
              pTempScanCmd[cmd_index]->Pad[0] |= MRVDRV_ES_SPECIFICSCAN;
      }
      else 
            {   
              pTempScanCmd[cmd_index]->Pad[0] |= MRVDRV_ES_NOTSPECIFICSCAN;
                NdisFillMemory(pScanCMD->BSSID, MRVDRV_ETH_ADDR_LEN, 0xff);
            } 

      // TLV_SSID_IE
            ssid = (MrvlIEtypes_SsIdParamSet_t *)
           ((UCHAR *)pScanCMD + sizeof(HostCmd_DS_GEN) + MRVDRV_ETH_ADDR_LEN +1);
                                  
            ssid->Header.Type = TLV_TYPE_SSID;
      ssid->Header.Len  = 0;
      if (Adapter->SetActiveScanSSID)
            {
              NdisMoveMemory (ssid->SsId,
                Adapter->ActiveScanSSID.Ssid,
                Adapter->ActiveScanSSID.SsidLength);

        ssid->Header.Len = (USHORT) (Adapter->ActiveScanSSID.SsidLength);
              pTempScanCmd[cmd_index]->Pad[0] |= MRVDRV_ES_SPECIFICSCAN;
      }
            pScanCMD->Size += (USHORT)(sizeof(MrvlIEtypesHeader_t) + ssid->Header.Len);

      // TLV_CHAN_IE
      chan = (MrvlIEtypes_ChanListParamSet_t *)
                   ((UCHAR *)ssid + sizeof(MrvlIEtypesHeader_t) + ssid->Header.Len);

      chan->Header.Type = TLV_TYPE_CHANLIST;
      chan->Header.Len = 0;
      
      chaninfo =  (ChanScanParamSet_t *)((UCHAR *)chan + sizeof(MrvlIEtypesHeader_t));

      
      for( k=0; k<PSCAN_NUM_CH_PER_SCAN; k++)
      { 

        // fill channel info
        if ((region_channel->Band == MRVDRV_802_11_BAND_B) ||
            (region_channel->Band == MRVDRV_802_11_BAND_BG))
        {
          chaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_BG;
        }
        else if (region_channel->Band == MRVDRV_802_11_BAND_A)
        {
          chaninfo->RadioType = HostCmd_SCAN_RADIO_TYPE_A;
        } 
        chaninfo->ChanNumber  = (UCHAR)(region_channel->CFP[j].Channel); 

        chaninfo->ScanType    = region_channel->ScanType[j];
        chaninfo->ScanTime    = HostCmd_SCAN_MIN_CH_TIME;
        chaninfo->MinScanTime = HostCmd_SCAN_MIN_CH_TIME;


        chan->Header.Len += sizeof(ChanScanParamSet_t);

        if(Adapter->SpecifyChannel)
        {
        
                      chaninfo->ScanTime = Adapter->ChannelMaxDuration;
                      chaninfo->MinScanTime = Adapter->ChannelMinDuration;
                      chaninfo->ChanNumber  = (UCHAR)(Adapter->SpecifyChannel);
            
                      j = region_channel->NrCFP;
                      TotalChan = 0;
                ScanChannel = Adapter->SpecifyChannel;
                      DBGPRINT(DBG_CUSTOM, (L" **** Set specified channel %d %d \r\n", ScanChannel, j));
                      Adapter->SpecifyChannel = 0;
                      break;
        }
        else
        {
        j++;  
        TotalChan--; 
        }
              if (j >= region_channel->NrCFP)
                  break;
                ///Force to use the PASSIVE_SCAN if we need it by overwriting the flag
                if (Adapter->isPassiveScan == TRUE) 
                {
                    DBGPRINT(DBG_SCAN|DBG_HELP,(L"Set PASSIVE_SCAN to channel\n"));
                    chaninfo->ScanType = HostCmd_SCAN_TYPE_PASSIVE;
                }
                if (Adapter->MeasureDurtion > 0) {
                    chaninfo->ScanTime = chaninfo->MinScanTime = Adapter->MeasureDurtion;
                }
                if (Adapter->ChannelNum > 0) {
                    chaninfo->ChanNumber  = (UCHAR)(Adapter->ChannelNum);
                    j = region_channel->NrCFP;
                ScanChannel = Adapter->ChannelNum;
                    TotalChan = 0;
                    break;
                }

        chaninfo = (ChanScanParamSet_t *)
                 ((UCHAR *)chan + sizeof(MrvlIEtypesHeader_t) + chan->Header.Len);
           } // end of for(k<PSCAN_NUM_CH_PER_SCAN) loop          
      
            pScanCMD->Size += (sizeof(MrvlIEtypesHeader_t) + chan->Header.Len); 

      
        // TLV_RATES_IE
        rates = (MrvlIEtypes_RatesParamSet_t *)
                ((UCHAR *)chan + sizeof(MrvlIEtypesHeader_t) + chan->Header.Len); 

      rates->Header.Type = TLV_TYPE_RATES;
            
      switch(region_channel->Band)
      { 
        case MRVDRV_802_11_BAND_B:
      
          NdisMoveMemory (rates->Rates, SupportedRates_B, B_SUPPORTED_RATES);
          rates->Header.Len = B_SUPPORTED_RATES;
              break;
          
        case MRVDRV_802_11_BAND_BG:
           
          NdisMoveMemory (rates->Rates, SupportedRates_G, G_SUPPORTED_RATES);
            rates->Header.Len = G_SUPPORTED_RATES;
            break;
          
        case MRVDRV_802_11_BAND_A:
      
            NdisMoveMemory (rates->Rates, SupportedRates_A, G_SUPPORTED_RATES);
            rates->Header.Len = G_SUPPORTED_RATES;
          break;

        default:
          NdisMoveMemory (rates->Rates, SupportedRates_G, G_SUPPORTED_RATES);
            rates->Header.Len = G_SUPPORTED_RATES;
          break;
      }   

      pScanCMD->Size += (sizeof(MrvlIEtypesHeader_t) + rates->Header.Len); 

      if (Adapter->ScanProbes == 1 || Adapter->ScanProbes == 2)
      {
#define PROBES_PAYLOAD_SIZE 2
        scanprobes = (MrvlIEtypes_NumProbes_t *)
                      ((UCHAR *)rates + sizeof(MrvlIEtypesHeader_t) + rates->Header.Len);
                                         
        scanprobes->Header.Type = TLV_TYPE_NUMPROBES;
        scanprobes->Header.Len  = PROBES_PAYLOAD_SIZE;
        scanprobes->NumProbes   = Adapter->ScanProbes;
        pScanCMD->Size += (sizeof(MrvlIEtypesHeader_t) + scanprobes->Header.Len);
      }
      // fill the command node
      SetCmdCtrlNode (Adapter,
                        pTempScanCmd[cmd_index], 
                      0, 
                      HostCmd_PENDING_ON_NONE, 
                      HostCmd_OPTION_USE_INT, 
                      0, 
                      FALSE, 
                      NULL,
                      NULL,
                      NULL, 
                      NULL);
      
      pTempScanCmd[cmd_index]->Pad[2] |= (0x03 & region_channel->Band);

      if (cmd_index == 0)
      {  
        pTempScanCmd[cmd_index]->Pad[2] |= MRVDRV_SCAN_CMD_START;  // scan start
      }
      if (TotalChan == 0)
        pTempScanCmd[cmd_index]->Pad[2] |= MRVDRV_SCAN_CMD_END;  // scan end 

      switch (region_channel->Band)
      {
        case MRVDRV_802_11_BAND_B:
          pTempScanCmd[cmd_index]->Pad[2] |= MRVDRV_802_11_BAND_B;              
            break;

        case MRVDRV_802_11_BAND_BG:
          pTempScanCmd[cmd_index]->Pad[2] |= MRVDRV_802_11_BAND_BG;
          break;
          
        case MRVDRV_802_11_BAND_A:
          pTempScanCmd[cmd_index]->Pad[2] |= MRVDRV_802_11_BAND_A;      
          break;

        default:
          pTempScanCmd[cmd_index]->Pad[2] |= MRVDRV_802_11_BAND_BG;
          break;
      }
      
          pTempScanCmd[cmd_index]->PadExt[0] = (UCHAR)(ScanChannel);

      // send command    
            DBGPRINT(DBG_CMD,(L">>> send scan command %d\n", cmd_index));

      EnterCriticalSection(&Adapter->CmdQueueExeSection);
            
      InsertCmdToQueue (Adapter, pTempScanCmd[cmd_index]);
      LeaveCriticalSection(&Adapter->CmdQueueExeSection); 

 
      GetCmdFromQueueToExecute(Adapter);
          
            cmd_index ++;
      
      }  // while

  }  // for 

        if (bBgScan==TRUE)
        {
             EnableBgScan( Adapter, TRUE);
        }

 
  return NDIS_STATUS_SUCCESS;
}




//dralee_20060529
/**
 *  @brief This function finds out the common rates between rate1 and rate2.
 *
 * It will fill common rates in rate1 as output if found.
 *
 * NOTE: Setting the MSB of the basic rates need to be taken
 *   care, either before or after calling this function
 *
 *  @param Adapter     A pointer to wlan_adapter structure
 *  @param rate1       the buffer which keeps input and output
 *  @param rate1_size  the size of rate1 buffer
 *  @param rate2       the buffer which keeps rate2
 *  @param rate2_size  the size of rate2 buffer.
 *
 *  @return            WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int get_common_rates(UCHAR *rate1, int rate1_size, UCHAR *rate2, int rate2_size)
                            
{
    UCHAR *ptr = rate1;
    UCHAR  tmp[30];
    int i;
    //070907
    UCHAR *pdest;
    UCHAR  orgtmp[30];
       
    NdisZeroMemory(tmp, sizeof(tmp)); 
       
    //070907
    NdisZeroMemory(orgtmp, sizeof(orgtmp));
    NdisMoveMemory (orgtmp, rate1, rate1_size );   
               
    if( rate1_size < sizeof(tmp) )
        NdisMoveMemory (tmp, rate1, rate1_size );
    else
        NdisMoveMemory (tmp, rate1, sizeof(tmp));
      

    NdisZeroMemory(rate1, rate1_size);   


    /* Mask the top bit of the original values */
    for (i = 0; tmp[i] && i < sizeof(tmp); i++)
        tmp[i] &= 0x7F;

    for (i = 0; rate2[i] && i < rate2_size; i++) 
    {
        /* Check for Card Rate in tmp, excluding the top bit */
        if ((pdest = strchr(tmp, rate2[i] & 0x7F)) )
        {    
            ULONG idx;

            /* Values match, so copy the Card Rate to rate1 */
            //070907 
            //*rate1++ = rate2[i];
            idx = pdest-tmp;
            *rate1++ = orgtmp[idx];
        }
    }
     

    return 0;
}

        
//dralee_20060613
VOID SetRadioControl(PMRVDRV_ADAPTER Adapter)
{      
    ///v5MrvPrintFileW(L"=><RC)SetRadioControl\n");
    PrepareAndSendCommand(
                        Adapter,                
                        HostCmd_CMD_802_11_RADIO_CONTROL,
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
    
}


#if 0
VOID MrvfrThread(IN PVOID pContext)
{
	PMRVDRV_ADAPTER     pAdapter = (PMRVDRV_ADAPTER) pContext;
	unsigned int                    i, AvailableAPCnt = 0;
	PNDIS_WLAN_BSSID_EX pBSSIDListSrc = pAdapter->PSBSSIDList;
	PFASTROAMPARAM      pfrParam = &pAdapter->frParam;

	for (i=0 ; i<pAdapter->ulPSNumOfBSSIDs; i++) {
		if (NdisEqualMemory( pBSSIDListSrc[i].Ssid.Ssid, pAdapter->CurrentSSID.Ssid, pAdapter->CurrentSSID.SsidLength) &&
			(pBSSIDListSrc[i].Ssid.SsidLength == pAdapter->CurrentSSID.SsidLength)) {
			AvailableAPCnt++;
		}
	}
	DBGPRINT(DBG_CCX, (L"MrvfrThread: Available AP: %d\r\n", AvailableAPCnt));
	if (AvailableAPCnt == 1) {
		DBGPRINT(DBG_CCX, (L"Only one available AP in the environment. Leave now\n"));
		NdisMSleep(500000);
		///Resubscribe the RSSI_LOW & Leave now...
		SetUpLowRssiValue(pAdapter);
		goto FuncFinal;
	}
	while (1) {
		pfrParam->AssocStatusCode = 0;
		HandleFastRoam(pAdapter);
		if (pfrParam->AssocStatusCode != 0) {
			DBGPRINT(DBG_CCX, (L"==>Warning Roaming failed. Do it again...\r\n"));
			NdisMSleep(500000);
		} else {
			DBGPRINT(DBG_CCX, (L"==>Roaming successfully!\r\n"));
			break;
		}
	}
FuncFinal:
    return;
}
#endif ///0

VOID GetTsfTlvFromScanResult(MrvlIEtypes_Data_t *pTlv, INT tlvBufSize, MrvlIEtypes_TsfTimestamp_t **pTsfTlv)
{
    MrvlIEtypes_Data_t *pCurrentTlv;    
    UINT               tlvBufLeft;
    
    pCurrentTlv = pTlv;
    tlvBufLeft = tlvBufSize;
    *pTsfTlv = NULL;
    
    while (tlvBufLeft >= sizeof(MrvlIEtypesHeader_t)) 
    {
        switch(pCurrentTlv->Header.Type)
       {
            case TLV_TYPE_TSFTIMESTAMP:
            *pTsfTlv = (MrvlIEtypes_TsfTimestamp_t*)pCurrentTlv;
            break;

            default:
            /* Give up, this seems corrupted */
            return;
       } /* switch */

        tlvBufLeft -= (sizeof(pTlv->Header) + pCurrentTlv->Header.Len);
        pCurrentTlv = (MrvlIEtypes_Data_t*)(pCurrentTlv->Data + pCurrentTlv->Header.Len);
    } /* while */
        
}   

UINT AppendTsfTlv(PUCHAR* pBuffer, PBSS_DESCRIPTION_SET_ALL_FIELDS pBSSDesc)
{
    MrvlIEtypes_TsfTimestamp_t tsfTlv;
    ULONGLONG tsfVal;

    /* Null Checks */
    if (pBuffer == 0 || *pBuffer == 0)
        return 0;

    tsfTlv.Header.Type = TLV_TYPE_TSFTIMESTAMP;
    tsfTlv.Header.Len  = (2 * sizeof(tsfVal));

    NdisMoveMemory((PVOID)*pBuffer, (PVOID)&tsfTlv, sizeof(tsfTlv.Header));
    *pBuffer += sizeof(tsfTlv.Header);

    /* TSF timestamp from the firmware TSF when the bcn/prb rsp was received */
    NdisMoveMemory((PVOID)*pBuffer, (PVOID)&pBSSDesc->networkTSF, sizeof(pBSSDesc->networkTSF));
    *pBuffer += sizeof(pBSSDesc->networkTSF);

    NdisMoveMemory((PVOID)*pBuffer, (PVOID)pBSSDesc->TimeStamp, sizeof(pBSSDesc->TimeStamp));
    *pBuffer += sizeof(pBSSDesc->TimeStamp);

    return (sizeof(tsfTlv.Header) + (2 * sizeof(tsfVal)));
}


VOID UpdateTsfTimestamps(PMRVDRV_ADAPTER Adapter, PBSS_DESCRIPTION_SET_ALL_FIELDS pBSSDesc)
{
    UINT tableIdx;
    ULONGLONG newTsfBase;
    LONGLONG tsfDelta;
 
    NdisMoveMemory((PVOID)&newTsfBase, (PVOID)pBSSDesc->TimeStamp, sizeof(newTsfBase));
    
    tsfDelta = newTsfBase - pBSSDesc->networkTSF;

    for (tableIdx = 0; tableIdx < Adapter->ulPSNumOfBSSIDs; tableIdx++)
        Adapter->PSBssDescList[tableIdx].networkTSF += tsfDelta;
}

USHORT AuthModeMapping(NDIS_802_11_AUTHENTICATION_MODE mode)
{
      USHORT type=-1; //060407 Coverity
      
    switch (mode)
   {
       case Ndis802_11AuthModeWPAPSK:
       case Ndis802_11AuthModeWPA:
       case Ndis802_11AuthModeOpen:
       case Ndis802_11AuthModeWPA2:       
       case Ndis802_11AuthModeWPA2PSK: 
		type = Wlan802_11AuthModeOpen;  // open authentication
		break;
       case Ndis802_11AuthModeShared:
		type = Wlan802_11AuthModeShared;  // shared authentication
		break;
       default:
		DBGPRINT(DBG_ERROR, (L"Received unsupported authentication mode: %d\n",
		mode));
   }
      return type;
}   

VOID SendDeauthCommand(PMRVDRV_ADAPTER Adapter)
{
    USHORT usCommand;

    // indicate to the OS that we are disconnected
            
    //memorize the previous SSID and BSSID
    NdisMoveMemory( &(Adapter->PreviousSSID), &(Adapter->CurrentSSID), 
                  sizeof(NDIS_802_11_SSID));
            
    NdisMoveMemory( Adapter->PreviousBSSID, Adapter->CurrentBSSID, 
                  MRVDRV_ETH_ADDR_LEN);

    if( Adapter->InfrastructureMode == Ndis802_11Infrastructure )
       usCommand = HostCmd_CMD_802_11_DEAUTHENTICATE;
    else
       usCommand = HostCmd_CMD_802_11_AD_HOC_STOP;
        // completely clean up
    PrepareAndSendCommand(
       Adapter,
       usCommand,
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
            
    // need to erase the current SSID and BSSID info
    Adapter->ulCurrentBSSIDIndex =0;

    if (IsIndicateDisconnect(Adapter) == TRUE) {
        ResetDisconnectStatus(Adapter);  
    } else {
        ///Can not indicate disconnect to upper layer because of some conditions, eg: roaming
        /// => Just to reset the stateus variable
        Adapter->MediaConnectStatus = NdisMediaStateDisconnected;
    }
}

