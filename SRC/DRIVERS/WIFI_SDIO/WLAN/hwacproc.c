/******************* ?Marvell Semiconductor, Inc., ***************************
 *
 *  Purpose:    This module has implmentation of station command 
 *              processing functions
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
#include "wlan_roam.h"

/*
===============================================================================
                                 CLOBAL CONSTANT
===============================================================================
*/

extern ULONG DSFeqList[];
extern USHORT RegionCodeToIndex[MRVDRV_MAX_REGION_CODE];
extern UCHAR IEEERegionChannel[MRVDRV_MAX_REGION_CODE][MRVDRV_MAX_CHANNEL_NUMBER];


/*
===============================================================================
                           CODED PUBLIC PROCEDURES
===============================================================================
*/

UCHAR EAPOLStartPacket[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // dest
                             0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // src
                             0x88, 0x8e, 0x01, 0x01, 0x0, 0x0, 0x0 }; 

static UCHAR S56AddTspec[] = {
                0x00,                               //TspecResult
                0xd0, 0x07, 0x00, 0x00,    //TimeOut
                0x01,                               //DialogToken
                0x00,                               //IEEEStatus
                0xdd,                               //Element_ID
                0x3d,                               //Length
                0x00, 0x50, 0xf2,             //OUI_1 
                0x02,                               //OUI_Type
                0x02,                               //OUI_SubType
                0x01,                               //Version 
                0xe0, 0x34, 0x00,             //TSInfo
                0xd0, 0x80,                      //Nominal_MSDU_Size  
                0xd0, 0x00,                      //Maximum_MSDU_Size 
                0x20, 0x4e, 0x00, 0x00,    //Min_Service_Interval
                0x20, 0x4e, 0x00, 0x00,    //Max_Service_Interval
                0x80, 0x96, 0x98, 0x00,    //Inactivity_Interval 
                0xff, 0xff, 0xff, 0xff,    //Suspension_Interval
                0x00, 0x00, 0x00, 0x00,    //Service_Start_Time
                0x00, 0x45, 0x01, 0x00,    //Minimum_Data_Rate
                0x00, 0x45, 0x01, 0x00,    //Mean_Data_Rate
                0x00, 0x45, 0x01, 0x00,    //Peak_Data_Rate
                0x00, 0x00, 0x00, 0x00,    //Max_Burst_Size
                0x00, 0x00, 0x00, 0x00,    //Delay_Bound
                0x00, 0x1b, 0xb7, 0x00,    //Min_PHY_Rate
                0x04, 0x20,                      //Surplus_Bandwidth_Allowance
                0x00, 0x00                       //Medium_Time
};

static VOID
HandleADDTSResponse(
        PHostCmd_DS_802_11_WMM_ADDTS pAddts, PMRVDRV_ADAPTER Adapter);

static VOID
HandleRSSI(
            PHostCmd_DS_802_11_RSSI_RSP pRSSIRSP,
            PMRVDRV_ADAPTER Adapter );

VOID
HandleAdhocAES( IN PMRVDRV_ADAPTER Adapter,
          IN PHostCmd_DS_802_11_KEY_MATERIAL pKeyReturn,
          IN PVOID info,
          IN PULONG BytesWritten
          )

{   
  PMRVL_ADHOC_AES_KEY pKey;
  UCHAR key_ascii[32];
  UCHAR key_hex[16];
  UCHAR *tmp;
  int i;
  
  pKey = (PMRVL_ADHOC_AES_KEY)info;
 
  NdisZeroMemory(key_hex, sizeof(key_hex));
  NdisMoveMemory(key_hex, Adapter->aeskey.KeyParamSet.Key, sizeof(key_hex));
  pKey->Length = sizeof(key_ascii) + 1;  
  NdisZeroMemory(key_ascii, sizeof(key_ascii)); 
  tmp = key_ascii;
  for (i = 0; i < sizeof(key_hex); i++) 
        tmp += sprintf(tmp, "%02x", key_hex[i]);

  //Transfer Ascii[ex: char 0(Char)=0x31(hex)=48(dec)] to hex: 0
  for (i = 0; i < sizeof(key_ascii); i++)
  {
     
        if ((key_ascii[i] >= 48) && (key_ascii[i] <= 57))
            key_ascii[i]-=48;
        else if ((key_ascii[i] >= 65) && (key_ascii[i] <= 70))
            key_ascii[i]-=55;
        else if ((key_ascii[i] >= 97) && (key_ascii[i] <= 102))
            key_ascii[i]-=87;
        else
            break;
  }  
  
  NdisMoveMemory(pKey->KeyBody, key_ascii, sizeof(key_ascii));
  *BytesWritten = 32 + 4;
  return ;
}

/******************************************************************************
 *
 *  Name: HandleBgScanResponse()
 *
 *  Description: Handle background scan results
 *
 *  Notes:
 *              The background scan results are added to PSBssidList.
 *
 *****************************************************************************/
VOID
HandleBgScanResponse(
        PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP pBgScanResponse,
        PMRVDRV_ADAPTER Adapter
            )
{
    ULONG i,j;
    USHORT Flag = 0;
    BOOLEAN bFound;
    ULONG TempV = 0,k;
    ULONG SsidEntries = 0;
    NDIS_STATUS ndStat;
    POID_MRVL_DS_BG_SCAN_CONFIG pOidBG;
    NDIS_802_11_SSID     ScannedSSID;   //050307


                                
    DBGPRINT(DBG_SCAN|DBG_HOSTSLEEP|DBG_HELP,(L"-------   Got background scan results -------\r\n" ));
    DBGPRINT(DBG_SCAN|DBG_HOSTSLEEP|DBG_HELP,(L"Command %x\r\n",(ULONG)pBgScanResponse -> Command));
    DBGPRINT(DBG_SCAN|DBG_HOSTSLEEP|DBG_HELP,(L"Size %d  SeqNum %d  Result %d \r\n",
                                (ULONG)pBgScanResponse -> Size, 
                                (ULONG)pBgScanResponse -> SeqNum,
                                (ULONG)pBgScanResponse -> Result
                                ));
  
    DBGPRINT(DBG_SCAN|DBG_HOSTSLEEP|DBG_HELP,(L"BSSDescriptSize %d\r\n",(ULONG)pBgScanResponse ->BSSDescriptSize));
    DBGPRINT(DBG_SCAN|DBG_HOSTSLEEP|DBG_HELP,(L"NumberOfSets    %d\r\n",(ULONG)pBgScanResponse ->NumberOfSets));
    DBGPRINT(DBG_CMD|DBG_HOSTSLEEP|DBG_HELP,(L"*** Scan returned %d AP before parsing ***\r\n", pBgScanResponse->NumberOfSets));
      
    // Get number of BSS Descriptors
    Adapter->ulNumOfBSSIDs = pBgScanResponse->NumberOfSets;
        // Parse the return SCAN result
    ndStat = InterpretBSSDescription(
                        Adapter, 
                            (PVOID)(Adapter->CurCmd->BufVirtualAddr),
                            pBgScanResponse->BSSDescriptSize,
                            HostCmd_RET_802_11_BG_SCAN_QUERY );
    if ( ndStat != NDIS_STATUS_SUCCESS )
    {
            DBGPRINT(DBG_SCAN | DBG_ERROR,(L"ERROR: InterpretBSSDescription returned ERROR\r\n"));
    }

    DBGPRINT(DBG_SCAN|DBG_WARNING,(L"*** Background scan Results: number of BSSID: %d\r\n", Adapter->ulNumOfBSSIDs));
    for ( i=0; i < Adapter->ulNumOfBSSIDs; i++ )
    {
            ///DBGPRINT(DBG_SCAN,(L"\t\t%2d:\t%32s - %2x-%2x-%2x-%2x-%2x-%2x RSSI=%d\n", 
            DBGPRINT(DBG_SCAN|DBG_WARNING,(L"\t\t%2d:\t%32s - %2x-%2x-%2x-%2x-%2x-%2x RSSI=%d\r\n", 
                                    i, Adapter->BSSIDList[i].Ssid.Ssid,
            Adapter->BSSIDList[i].MacAddress[0],
            Adapter->BSSIDList[i].MacAddress[1],
            Adapter->BSSIDList[i].MacAddress[2],
            Adapter->BSSIDList[i].MacAddress[3],
            Adapter->BSSIDList[i].MacAddress[4],
            Adapter->BSSIDList[i].MacAddress[5],
            Adapter->BSSIDList[i].Rssi));
    }

    //Check the whole BSSID list string if there's invalid character.
    for ( Flag = 0, i=0; (i < Adapter->ulNumOfBSSIDs)||(Flag!=0); i++ )
    {
            // We'll re-check the first one when the previous first one has been discard.
            if (Flag == 1)
            {
                Flag = 0;
                i--;
            }

            do
            {   //Check if there's invalid data rate.
                for ( k=0,TempV=0; k < NDIS_SUPPORTED_RATES; k++)
                    TempV |= Adapter->BSSIDList[i].SupportedRates[k];

                    if (( TempV == 0 ) || (Adapter->BSSIDList[i].SupportedRates[0] == 0))
                    {
                        Flag = 1;
                        DBGPRINT(DBG_SCAN, (L"Invalid support rate\r\n")); 
                        break;
                    }

                    // marked for support associate to Hide SSID
            if ( Adapter->BSSIDList[i].Ssid.SsidLength == 0 )
                    {   
                           Flag = 1;
                break;
                            DBGPRINT(DBG_SCAN, (L"0 length SSID, discard (%d)\r\n", i));
                    }

                    // Check the invalid SSID.
                for (j=0; j < Adapter->BSSIDList[i].Ssid.SsidLength; j++)
                {
                    if ( Adapter->BSSIDList[i].Ssid.Ssid[j] < 0x20 )
                    {
                            DBGPRINT(DBG_SCAN,(L"INVALID BSSID Setting flag to discard (%d) %s\r\n",i, Adapter->BSSIDList[i].Ssid.Ssid));
                            DBGPRINT(DBG_SCAN,(L"i = %d, j = %d, TempV = %x, Ssid[j] = %x, dsconfig = %d\r\n", i,j, TempV, Adapter->BSSIDList[i].Ssid.Ssid[j],Adapter->BSSIDList[i].Configuration.DSConfig));
                            // Replace the current BSSID struct with last one
                            Flag = 1;
                            break;
                    }
                }
            if  ( Adapter->BSSIDList[i].Ssid.Ssid[0] == 0x20 )
            {
                Flag = 1;
                break;      
            }       
            } while (0);

            if (Flag == 1)
            {
                if ((i+1) == Adapter->ulNumOfBSSIDs)
                {
                    DBGPRINT(DBG_SCAN,(L"INVALID BSSID DISCARDING LAST(%d) %s\r\n",i, Adapter->BSSIDList[i].Ssid.Ssid));
                    NdisZeroMemory( &(Adapter->BSSIDList[i]), 
                                    sizeof(NDIS_WLAN_BSSID_EX));
                        NdisZeroMemory( &(Adapter->IEBuffer[i]),
                                        sizeof(MRV_BSSID_IE_LIST));
                    Adapter->ulNumOfBSSIDs -= 1;
                    Flag = 0;
                    break;
                }
                else
                    {
                    //We start replace the current invalid SSID with last SSID in array.
                    DBGPRINT(DBG_SCAN,(L"INVALID BSSID DISCARDING CURRENT(%d) %s\r\n",i, Adapter->BSSIDList[i].Ssid.Ssid));
                    NdisMoveMemory( &(Adapter->BSSIDList[i]), 
                                    &(Adapter->BSSIDList[Adapter->ulNumOfBSSIDs-1]),      
                                    sizeof(NDIS_WLAN_BSSID_EX));
                    NdisZeroMemory( &(Adapter->BSSIDList[Adapter->ulNumOfBSSIDs-1]), 
                                sizeof(NDIS_WLAN_BSSID_EX));
                        NdisMoveMemory( &(Adapter->IEBuffer[i]),
                                        &(Adapter->IEBuffer[Adapter->ulNumOfBSSIDs-1]),
                                        sizeof(MRV_BSSID_IE_LIST));
                        NdisZeroMemory( &(Adapter->IEBuffer[Adapter->ulNumOfBSSIDs-1]),
                                        sizeof(MRV_BSSID_IE_LIST));
                    Adapter->ulNumOfBSSIDs -= 1;
                    }
            } // if (Flag == 1)
        }
  
        DBGPRINT(DBG_CMD|DBG_HOSTSLEEP|DBG_WARNING,(L"HWAC - Background scanned %2d APs\r\n", Adapter->ulNumOfBSSIDs));  

    if ((Adapter->RoamingMode == SMLS_ROAMING_MODE) && (Adapter->pwlanRoamParam!=NULL)) {
        wlan_roam_ScanTableReady(Adapter->pwlanRoamParam, WRSCAN_BGSCAN);
    }

    //091407                  
    SelectBgScanConfig(Adapter);

    pOidBG = (POID_MRVL_DS_BG_SCAN_CONFIG)Adapter->BgScanCfg;
     HexDump(DBG_ERROR, "HandleBgScanResponse A=>dapter->BgScanCfg", (UCHAR *)pOidBG, 
                                 sizeof(OID_MRVL_DS_BG_SCAN_CONFIG));

//++ remove_bg_ssid
  
  DBGPRINT(DBG_SCAN|DBG_HELP ,(L"HandleBgScanResponse=>Adapter->nBgScanCfg=%d,sizeof(OID_MRVL_DS_BG_SCAN_CONFIG)=%d,Total=%d\r\n", Adapter->nBgScanCfg, sizeof(OID_MRVL_DS_BG_SCAN_CONFIG),(ULONG)(Adapter->nBgScanCfg-(USHORT)sizeof(OID_MRVL_DS_BG_SCAN_CONFIG)+1)));
                                       
   SsidEntries = FindSsidInBgCfg(&pOidBG->TlvData[0], 
                                     (ULONG)(Adapter->nBgScanCfg-(USHORT)sizeof(OID_MRVL_DS_BG_SCAN_CONFIG)+1),
                                      0,
                                  &ScannedSSID); //050307
     

    
    DBGPRINT(DBG_ERROR|DBG_SCAN|DBG_HELP,(L"HandleBgScanResponse=>Find %x SSIDs in BG SCBG_SCAN CFG\r\n", SsidEntries)); 

    if( SsidEntries == 0 )
    {
       Adapter->ulPSNumOfBSSIDs = 0;
       DBGPRINT(DBG_ERROR|DBG_SCAN|DBG_HELP,(L"***HandleBgScanResponse=>BG_SCAN:Remove all BSSID list  ===> Set Adapter->ulPSNumOfBSSIDs = 0\r\n"));
    }
    else
    {
         DBGPRINT(DBG_SCAN|DBG_HELP, (L" Adapter->ActiveScanSSID=" ));  
        //050307
        //for(j=0;j<Adapter->ActiveScanSSID.SsidLength;j++)
		//			DBGPRINT(DBG_ERROR, (L"%c",Adapter->ActiveScanSSID.Ssid[j]));  
        //       DBGPRINT(DBG_ERROR, (L"\n" ));  
    
        while(SsidEntries)
        {
           //find a specific order of SSID TLV
           FindSsidInBgCfg(&pOidBG->TlvData[0], 
                           (Adapter->nBgScanCfg-sizeof(OID_MRVL_DS_BG_SCAN_CONFIG)+1),
                            SsidEntries,
                            (PVOID)&ScannedSSID);  //050307
                           //(PVOID)&Adapter->ActiveScanSSID); 
           SsidEntries--;                             
           RemoveTheActiveSsidEntry(Adapter, &ScannedSSID); //050307
 
           //050307
           DBGPRINT(DBG_ERROR|DBG_SCAN|DBG_HELP,(L"***BG_SCAN:Remove BSSID list:%x,%x..\r\n",
                                                   ScannedSSID.Ssid[0],ScannedSSID.Ssid[1]));
       }
    } 
//-- remove_bg_ssid

        // active scan, append AP only if it does not already exist

        for ( i=0; i < Adapter->ulNumOfBSSIDs; i++ )
            {
                    bFound = FALSE;
                    for ( j=0; j < Adapter->ulPSNumOfBSSIDs; j++ )
                    {
                        // compare SSID, BSSID, and Mode
                        if ( Adapter->BSSIDList[i].Ssid.SsidLength != 
                            Adapter->PSBSSIDList[j].Ssid.SsidLength )
                        {
                                continue;
                        }
                        if ( NdisEqualMemory(Adapter->BSSIDList[i].Ssid.Ssid,
                                     Adapter->PSBSSIDList[j].Ssid.Ssid,
                                     Adapter->PSBSSIDList[j].Ssid.SsidLength) == 0 )
                        {
                                continue;
                        }
                        if ( NdisEqualMemory(Adapter->BSSIDList[i].MacAddress,
                                     Adapter->PSBSSIDList[j].MacAddress,
                                     ETH_ADDR_LENGTH) == 0 )
                        {
                                continue;
                        }
                        if ( Adapter->BSSIDList[i].InfrastructureMode != 
                            Adapter->PSBSSIDList[j].InfrastructureMode )
                        {
                                continue;
                        }
                        bFound = TRUE;

                        // replace the current entry 
                        NdisMoveMemory(&Adapter->PSBSSIDList[j],
                                    &Adapter->BSSIDList[i],
                                    sizeof(NDIS_WLAN_BSSID_EX));
                        NdisMoveMemory(&Adapter->PSIEBuffer[j],
                                    &Adapter->IEBuffer[i],
                                    sizeof(MRV_BSSID_IE_LIST));  
                    NdisMoveMemory(&Adapter->PSBssDescList[j], 
                                &Adapter->BssDescList[i],
                                    sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));

                        DBGPRINT(DBG_SCAN, (L"PScan: Replace Specific Scan Entry %d To Scan List\r\n", j));
                    }
                    
                    if (! bFound)
                    {
                           if (Adapter->ulPSNumOfBSSIDs  < MRVDRV_MAX_BSSID_LIST)
                    {
                            // append at the end
                            NdisMoveMemory( &Adapter->PSBSSIDList[Adapter->ulPSNumOfBSSIDs],
                                                  &Adapter->BSSIDList[i],
                                                  sizeof(NDIS_WLAN_BSSID_EX));
                            NdisMoveMemory( &Adapter->PSIEBuffer[Adapter->ulPSNumOfBSSIDs],
                                                  &Adapter->IEBuffer[i],
                                                  sizeof(MRV_BSSID_IE_LIST)); 
                            NdisMoveMemory( &Adapter->PSBssDescList[Adapter->ulPSNumOfBSSIDs], 
                                                  &Adapter->BssDescList[i],
                                                  sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS)); 

                            DBGPRINT(DBG_SCAN|DBG_HELP, (L"PScan: Append Specific Scan Entry %d To Scan List\r\n", i)); 
                            DBGPRINT(DBG_SCAN|DBG_HELP, (L"Appended Entry: BSSID:%2x-%2x-%2x-%2x-%2x-%2x, RSSI:%d \r\n",
                                                  Adapter->PSBssDescList[Adapter->ulPSNumOfBSSIDs].BSSID[0],
                                                  Adapter->PSBssDescList[Adapter->ulPSNumOfBSSIDs].BSSID[1],
                                                  Adapter->PSBssDescList[Adapter->ulPSNumOfBSSIDs].BSSID[2],
                                                  Adapter->PSBssDescList[Adapter->ulPSNumOfBSSIDs].BSSID[3],
                                                  Adapter->PSBssDescList[Adapter->ulPSNumOfBSSIDs].BSSID[4],
                                                  Adapter->PSBssDescList[Adapter->ulPSNumOfBSSIDs].BSSID[5],
                                                  Adapter->PSBSSIDList[Adapter->ulPSNumOfBSSIDs].Rssi ));

                            Adapter->ulPSNumOfBSSIDs++;

                }
        }
        }

}


NDIS_STATUS 
HandleBgScanResultEvent ( 
        PMRVDRV_ADAPTER pAdapter 
        )
{
    NDIS_STATUS Status;
        OID_MRVL_DS_BG_SCAN_QUERY       BgQuery;

    DBGPRINT(DBG_SCAN|DBG_HOSTSLEEP|DBG_HELP,(L"HandleBgScanResultEvent() \r\n"));
    BgQuery.Oid = OID_MRVL_BG_SCAN_QUERY;
    BgQuery.Flush = 1; // flush current results in firmware.

        Status = PrepareAndSendCommand(
                          pAdapter,
                            HostCmd_CMD_802_11_BG_SCAN_QUERY,
                                HostCmd_ACT_GET,
                                HostCmd_OPTION_USE_INT,
                                (NDIS_OID)0,
                                HostCmd_PENDING_ON_NONE,
                                0,
                                FALSE,
                                NULL,
                                NULL,
                                NULL,
                                &BgQuery);   

        DBGPRINT(DBG_SCAN|DBG_HOSTSLEEP|DBG_HELP, (L"Send out BG_SCAN_QUERY for getting background scan results [return: 0x%x]\r\n", Status) );

        pAdapter->bBgScanEnabled=FALSE;
                         

        return Status;
        
}

/******************************************************************************
 *
 *  Name: HandleMicError()
 *
 *  Description: handle MIC error
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *        UINT ErrorType
 *    
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/

VOID
HandleMicError(
               IN PMRVDRV_ADAPTER Adapter, 
             IN UINT ErrorType)
{
    WPA_NOTIFY_OS   message;
    ULONG           ulCurrTime;
    ULONG           ulTimeDiff;

    NdisZeroMemory(&message, sizeof(WPA_NOTIFY_OS));
    NdisMoveMemory(message.request.Bssid, Adapter->CurrentBSSID, 
                sizeof(Adapter->CurrentBSSID));
    message.request.Length = sizeof(NDIS_802_11_AUTHENTICATION_REQUEST);
    message.status = Ndis802_11StatusType_Authentication;
    
    switch (ErrorType)
    {

    case MACREG_INT_CODE_WPA_MIC_ERR_UNICAST:
        message.request.Flags = NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR;
        DBGPRINT(DBG_WARNING, (L"Pairwise MIC Error!\r\n"));
        break;

    case MACREG_INT_CODE_WPA_MIC_ERR_MULTICAST:
        message.request.Flags = NDIS_802_11_AUTH_REQUEST_GROUP_ERROR;
        DBGPRINT(DBG_WARNING, (L"Groupwise MIC Error!\r\n"));
        break;

    default:
        DBGPRINT(DBG_ERROR, (L"HandleMicError(): do not know how to handle error code 0x%x!\r\n",
                            ErrorType));
        // skip the indication
        return;
    }

    NdisMIndicateStatus(Adapter->MrvDrvAdapterHdl,
                        NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                        &message,
                        sizeof(WPA_NOTIFY_OS));
    NdisMIndicateStatusComplete(Adapter->MrvDrvAdapterHdl);
    DBGPRINT(DBG_MACEVT, (L"*** Indicated MIC error!\r\n"));

    NdisGetSystemUpTime(&ulCurrTime);
    
    if ( ulCurrTime > Adapter->ulLastMICErrorTime )
    {
        ulTimeDiff = ulCurrTime - Adapter->ulLastMICErrorTime;
    }
    else
    {
        ulTimeDiff = 0xFFFFFFFF - Adapter->ulLastMICErrorTime + ulCurrTime;
    }

    NdisMSleep(20000);
    
    if ( (Adapter->ulLastMICErrorTime != 0) && (ulTimeDiff < MRVDRV_MIC_ERROR_PERIOD) )
    { 
// tt mic error ++: driver will send out de-auth after de-auth mac event is rising.
// tt ++ mic 2         USHORT          usCommand;
       
// tt ++ mic error 2
        Adapter->ulMicErrorStartTime = ulCurrTime;
// tt --
        Adapter->bMicErrorIsHappened = TRUE;
        DBGPRINT( DBG_MACEVT, (L"*** MIC error is happended!\r\n") );

        NdisMSetTimer( &Adapter->MrvMicDisconnectTimer, 200 ); // tt ++ mic 2
        Adapter->DisconnectTimerIsSet = TRUE;

    }

    Adapter->ulLastMICErrorTime = ulCurrTime;
  
}

// tt mic error ++
VOID DisconnectDueToMicError( PMRVDRV_ADAPTER pAdapter )
{
    USHORT          usCommand;

    if ( pAdapter->MediaConnectStatus != NdisMediaStateConnected )
    {
        DBGPRINT( DBG_MACEVT, (L"*** Connection had already been disconnected.\r\n") );
        return;
    }
    
    DBGPRINT( DBG_MACEVT, (L"*** Send out de-authentication due to MIC error\r\n") );

    if ( pAdapter->InfrastructureMode == Ndis802_11Infrastructure )
        usCommand = HostCmd_CMD_802_11_DEAUTHENTICATE;
    else
        usCommand = HostCmd_CMD_802_11_AD_HOC_STOP;

    DBGPRINT( DBG_WARNING, (L"[Mrvl] Send out de-auth due to MIC error\r\n") );
    
    PrepareAndSendCommand(
                  pAdapter,
                  usCommand,
                  0,
                  HostCmd_OPTION_USE_INT,
                  0,
                  HostCmd_PENDING_ON_NONE,
                  0,
                  FALSE,
                  NULL,
                  NULL,
                  NULL,
                  NULL);

// tt ++ mic 2    NdisMSleep(60000);
    
    DBGPRINT( DBG_WARNING, (L"[Mrvl] Reset and disconnect due to MIC error\r\n") );

    ResetDisconnectStatus( pAdapter );
}
// tt mic error --

/******************************************************************************
 *
 *  Name: HandleAdHocModeLink()
 *
 *  Description: Handle link sensed and bcn lost event in adhoc mode
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *        UINT INTCode
 *    
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleAdHocModeLink(
    IN PMRVDRV_ADAPTER Adapter, 
    IN UINT INTCode)
{
    if ( Adapter->InfrastructureMode != Ndis802_11IBSS )
    {
        DBGPRINT(DBG_ERROR, (L"HandleAdHocModeLink(): Currently NOT in adhoc mode, event IGNORED!\r\n"));
        return;
    } 
     
    //032007 fix adhoc connection indication
    switch( INTCode )
    {
       case MACREG_INT_CODE_LINK_SENSED: 
            if( Adapter->bIsMoreThanOneStaInAdHocBSS == FALSE )
            {                       
                //032007 first link joined. indicate connected status
                Adapter->bIsMoreThanOneStaInAdHocBSS = TRUE; 
                
                Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_CONNECT); 
                
                DBGPRINT(DBG_ADHOC|DBG_HELP,(L"first STA joins, indicate connected\r\n"));
            }
            else
               DBGPRINT(DBG_ADHOC|DBG_HELP,(L"STAs joining, no connected indication\r\n"));   
       break;
       case MACREG_INT_CODE_ADHOC_BCN_LOST:
            if( Adapter->bIsMoreThanOneStaInAdHocBSS == TRUE )
            {
                Adapter->bIsMoreThanOneStaInAdHocBSS = FALSE; 
                 
                Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_DISCONNECT); 

                DBGPRINT(DBG_ADHOC|DBG_HELP,(L"Joiner Missing, indicate disconnected\r\n"));
            }
            else
                DBGPRINT(DBG_ADHOC|DBG_HELP,(L"Joiner Missing, no disconnected indication\r\n"));
       break;
       default: 
            DBGPRINT(DBG_ADHOC|DBG_HELP,(L"Wrong Mac Event Type\r\n"));
       break;
    }
    
    return;
} // HandleAdHocModeLink

/*
VOID
HandleFastRoam(IN PMRVDRV_ADAPTER Adapter)
{
    return;
}
*/

/******************************************************************************
 *
 *  Name: HandleMACEvent()
 *
 *  Description: MAC event handler
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *        UINT INTCode
 *    
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleMACEvent(
  IN PMRVDRV_ADAPTER Adapter, 
  IN UINT INTCode)
{
    PNDIS_PACKET pPacket;
    NDIS_STATUS Status;
   
    switch( Adapter->LogEventCfg )
    {   
       case 0:
       break;
       case 1: //only log subscribe events
          if(! (INTCode == MACREG_INT_CODE_RSSI_LOW ||
                INTCode == MACREG_INT_CODE_SNR_LOW ||
                INTCode == MACREG_INT_CODE_MAX_FAIL ||
                INTCode == MACREG_INT_CODE_RSSI_HIGH ||
                INTCode == MACREG_INT_CODE_SNR_HIGH ||
                INTCode == MACREG_INT_CODE_HOST_SLEEP_AWAKE ||
                INTCode == MACREG_INT_CODE_LINK_LOSE_NO_SCAN) )
       break;
       case 2: //log all events    
      {                           
         DWORD       tick;

         tick = GetTickCount();  
         MrvPrintEventLogToFile(L"Event: %x at Time Tick: %x\r\n", INTCode, tick);
      }
       break;
    }

    switch( INTCode )
    {  
       case MACREG_INT_CODE_WMM_STATUS_CHANGE:
          PrepareAndSendCommand(
             Adapter,
             HostCmd_CMD_802_11_WMM_GET_STATUS,
             HostCmd_ACT_GET,
             HostCmd_OPTION_USE_INT,
             (NDIS_OID)0,
             HostCmd_PENDING_ON_NONE,
             0,
             FALSE,
             NULL,
             NULL,
             NULL,
             NULL );               
       break;

       case MACREG_INT_CODE_BG_SCAN_REPORT:
            DBGPRINT(DBG_HOSTSLEEP|DBG_WARNING,(L"BG_SCAN_REPORT: \r\n") );
            if( IsThisDsState(Adapter, DS_STATE_SLEEP) )
                SetDsState( Adapter, DS_STATE_NONE ); 
            if ( Adapter->PSMode == Ndis802_11PowerModeMAX_PSP )
            {
              Adapter->PSMode = Ndis802_11PowerModeCAM;
              Adapter->PSMode_B = Ndis802_11PowerModeCAM;
              PSWakeup(Adapter);
              Adapter->bPSEnableBGScan=TRUE;
                    
              SelectBgScanConfig(Adapter);                               
            }
          HandleBgScanResultEvent( Adapter );
       break;
  

       case MACREG_INT_CODE_LINK_SENSED: //Link found
          DBGPRINT(DBG_MACEVT,(L"MACREG_INT_CODE_LINK_SENSED\r\n"));
          HandleAdHocModeLink(Adapter, MACREG_INT_CODE_LINK_SENSED);

          if( Adapter->PSMode_B == Ndis802_11PowerModeMAX_PSP && 
              Adapter->PSMode == Ndis802_11PowerModeCAM )
          {   
              Adapter->PSMode = Ndis802_11PowerModeMAX_PSP;
              PSSleep(Adapter);
          } 
       break;

       case MACREG_INT_CODE_ADHOC_BCN_LOST:    // last partner in adhoc mode left
          DBGPRINT(DBG_MACEVT,(L"MACREG_INT_CODE_ADHOC_BCN_LOST\r\n"));
                                
            if( Adapter->PSMode ) 
            {
                Adapter->PSMode_B = Adapter->PSMode;
                Adapter->PSMode = Ndis802_11PowerModeCAM;                 
                PSWakeup(Adapter);
            }

          HandleAdHocModeLink(Adapter, MACREG_INT_CODE_ADHOC_BCN_LOST);
       break;

       case MACREG_INT_CODE_DEAUTHENTICATED:
          DBGPRINT(DBG_MACEVT|DBG_CCX,(L"HWAC - Deauthenticated\r\n"));
        if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
            wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_NOT_CONNECT);
        }

          if ( IsInMicErrorPeriod( Adapter ) )
         {
            DisconnectDueToMicError( Adapter );
         }
          else
         {
             if (Adapter->bIsDeauthenticationEvent == FALSE)
            {
                Adapter->bIsDeauthenticationEvent = TRUE;
                MacEventDisconnected(Adapter, 5000, FALSE);
                InfraBssReconnectStart( Adapter, RECONNECT_DEAUTHENTICATE );
            }
         }
        break;
    
       case MACREG_INT_CODE_DISASSOCIATED:
          DBGPRINT(DBG_MACEVT|DBG_HELP,(L"HWAC - Disassociated\r\n"));
        if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
            wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_NOT_CONNECT);
        }
    
          if ( IsInMicErrorPeriod( Adapter ) )
         {
             DisconnectDueToMicError( Adapter );
         }
          else
         {
             MacEventDisconnected(Adapter,(UINT) MRVDRV_TEN_SECS_TIMER_INTERVAL, TRUE);
         }
       break;



       case MACREG_INT_CODE_LINK_LOSE_NO_SCAN:
          DBGPRINT(DBG_MACEVT,(L"HWAC - Link lost\r\n"));
        if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
            wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_NOT_CONNECT);
        }

          Adapter->bIsBeaconLoseEvent = TRUE;
          if( Adapter->bHostWakeCfgSet == SPCFG_ENABLE )
         {
             Deactivate_Host_Sleep_Cfg(Adapter, 0);
             Adapter->bHostWakeCfgSet = SPCFG_MASKED;  
         }
          MacEventDisconnected(Adapter, 5000,FALSE);

          InfraBssReconnectStart( Adapter, RECONNECT_LINK_LOST );        
       break;

       case MACREG_INT_CODE_PS_SLEEP:
          DBGPRINT(DBG_MACEVT|DBG_HELP,(L"Evt - PS_SLEEP\r\n"));
          Adapter->bNullFrameSent = 0; //clear flag to let ps_confirm can be sent down.
 
          UpdatePowerSaveState( Adapter, NULL, MACREG_INT_CODE_PS_SLEEP);

       break;

       case MACREG_INT_CODE_PS_AWAKE:


          DBGPRINT(DBG_MACEVT|DBG_WARNING,(L"Evt - PS_AWAKE\r\n"));
          Adapter->bNullFrameSent = 0;
          UpdatePowerSaveState( Adapter, NULL, MACREG_INT_CODE_PS_AWAKE);

          GetCmdFromQueueToExecute( Adapter );

#if 0        
          if( Adapter->bHostWakeCfgSet == SPCFG_MASKED )
          {    
             ULONG       tick;
             
            if(Adapter->bIsReconnectEnable == FALSE && 
               Adapter->MediaConnectStatus == NdisMediaStateConnected )
            {   
                if( Adapter->SleepCfgRestoreTick == 0 )
                {              
                    Adapter->SleepCfgRestoreTick = GetTickCount();
                    Adapter->SleepCfgRestoreTick += TIME_TO_RESTORE_HOST_SLEEP_CFG; 
                }
                else if( (tick = GetTickCount()) >= Adapter->SleepCfgRestoreTick )
                {
                   Adapter->SleepCfgRestoreTick = 0;
                   Status =Reactivate_Host_Sleep_Cfg(Adapter); 
                   if( Status == NDIS_STATUS_SUCCESS )
                      Adapter->bHostWakeCfgSet = SPCFG_ENABLE;
                }
            }
          }  

#endif

          EnterCriticalSection(&Adapter->TxCriticalSection);

          Adapter->TxLock = 0;  

          if(Adapter->TxPacketCount &&
             Adapter->SentPacket == NULL)
          {    
             PTXQ_KEEPER  pTxQKeeper;
             PTXQ_NODE    pTQNode;
 
             TxPacketDeQueue(Adapter, &pTxQKeeper, &pTQNode);
             Adapter->TxPacketCount--;

             pPacket = pTQNode -> pPacket; 

             Status = SendSinglePacket(Adapter,pPacket,&(pTQNode->DnldPacket));
        
             if(Status == NDIS_STATUS_SUCCESS)
            {                         
                PushFreeTxQNode(Adapter->TxFreeQ,pTQNode);  
            }
             else
            {    
                DBGPRINT(DBG_ERROR,(L"[PS_AWAKE]Send packet fail\r\n"));;
                //Error handling , push back this node 
                InsertTxQNodeFromHead(pTxQKeeper,pTQNode);
                Adapter->TxPacketCount++;
             }
          }
// we need the following WMM_UAPSD for a compiler issue
          else if( (Adapter->PPS_Enabled == 0) && Adapter->WmmDesc.WmmUapsdEnabled && CheckLastPacketIndication(Adapter) )
         {    
             Adapter->bNullFrameSent = 1;
             SendNullPacket(Adapter, MRVDRV_WCB_POWER_MGMT_NULL_PACKET|MRVDRV_WCB_POWER_MGMT_LAST_PACKET);
         } 
          else if( (Adapter->PPS_Enabled == 1) && CheckLastPacketIndication(Adapter)  ) 
         {               
             SendNullPacket(Adapter, (MRVDRV_WCB_POWER_MGMT_NULL_PACKET|MRVDRV_WCB_POWER_MGMT_LAST_PACKET));
             Adapter->TxLock = 1;  
         } 

          LeaveCriticalSection(&Adapter->TxCriticalSection);
      
       break;
       case MACREG_INT_CODE_DS_AWAKE:
            //071107
            If_ClearPowerUpBit(Adapter);

         if( IsThisDsState(Adapter, DS_STATE_SLEEP) )
            SetDsState( Adapter, DS_STATE_NONE );

         DBGPRINT(DBG_DEEPSLEEP|DBG_HELP|DBG_CUSTOM,(L"DS_AWAKED State %x \r\n", Adapter->DSState));
         //SetEvent( Adapter->hOidQueryEvent );

            
       break;

     case MACREG_INT_CODE_HOST_SLEEP_AWAKE:
     {                       
          DBGPRINT(DBG_HOSTSLEEP|DBG_MACEVT|DBG_HELP,(L"Host Sleep Awake!!\r\n"));
          //071107
          If_ClearPowerUpBit(Adapter);

          DBGPRINT( DBG_CUSTOM, (L"goto sleep for 60 ms\r\n") );
          NdisMSleep(60 *1000 );

          SetHostPowerState( Adapter, HTWK_STATE_FULL_POWER );
          UpdatePowerSaveState( Adapter, NULL, MACREG_INT_CODE_PS_AWAKE);

          PrepareAndSendCommand(
                             Adapter,
                             HostCmd_CMD_802_11_HOST_SLEEP_AWAKE_CONFIRM,
                             0,          
                             HostCmd_OPTION_USE_INT,
                             0,
                             HostCmd_PENDING_ON_NONE,
                             1,  //062707 set priority pass. ePS
                             FALSE,
                             NULL,
                             NULL,
                             NULL,
                             NULL); 

       //Host Sleep without IEEE PS mode
        if(Adapter->PSMode  == Ndis802_11PowerModeCAM &&
           Adapter->psState == PS_STATE_FULL_POWER ) 
         {                                    
             Adapter->bHostWakeCfgSet = SPCFG_DISABLE;
             Adapter->SleepCfg.ulCriteria = -1; //disable previous HostSleep CFG first
             Deactivate_Host_Sleep_Cfg(Adapter,0);
         }
         //060407 reenable this code for handling gap != 0xff case
        else if( Adapter->PSMode  != Ndis802_11PowerModeCAM && 
                 Adapter->psState != PS_STATE_FULL_POWER && 
                  Adapter->bHostWakeCfgSet == SPCFG_ENABLE &&
                  Adapter->SleepCfg.ucGap != 0xff )
        { 
#if 0        
              Adapter->bHostWakeCfgSet = SPCFG_MASKED; 
#else
              DBGPRINT(DBG_HOSTSLEEP|DBG_MACEVT|DBG_HELP,(L"Adapter->bHostWakeCfgSet = SPCFG_DISABLE!!\r\n"));
		Adapter->bHostWakeCfgSet = SPCFG_DISABLE;
              Adapter->SleepCfg.ulCriteria = -1; //disable previous HostSleep CFG first
#endif             
              Deactivate_Host_Sleep_Cfg(Adapter, 0);
        }
           
    }
       break;
       case MACREG_INT_CODE_IBSS_COALESCED:
          Status = PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_IBSS_COALESING_STATUS,
                HostCmd_ACT_GEN_GET,
                HostCmd_OPTION_USE_INT,
                (NDIS_OID)0,
                HostCmd_PENDING_ON_NONE,
                0,
                FALSE,
                NULL,
                NULL,
                NULL,
                NULL);

          DBGPRINT(DBG_ADHOC|DBG_HELP,(L"IBSSCOALSE Event:%x\r\n",Adapter->IbssCoalsecingEnable));         
       break; 

       case MACREG_INT_CODE_CMD_FINISHED: // Command finished
          DBGPRINT(DBG_MACEVT,(L"MACREG_INT_CODE_CMD_FINISHED\r\n"));
          HandleCommandFinishedEvent(Adapter);
       break;

       case MACREG_INT_CODE_WPA_MIC_ERR_UNICAST:
          DBGPRINT(DBG_MACEVT, (L"*** Received UNICAST MIC error ***\r\n"));
          HandleMicError(Adapter, MACREG_INT_CODE_WPA_MIC_ERR_UNICAST); 
       break;

       case MACREG_INT_CODE_WPA_MIC_ERR_MULTICAST:
          DBGPRINT(DBG_MACEVT, (L"*** Received MULTICAST MIC error ***\r\n"));
          HandleMicError(Adapter, MACREG_INT_CODE_WPA_MIC_ERR_MULTICAST);
       break;

    case MACREG_INT_CODE_RSSI_LOW:
        DBGPRINT(DBG_MACEVT|DBG_HELP, (L"MACREG_INT_CODE_RSSI_LOW\r\n"));
        
        
        if (Adapter->MediaConnectStatus == NdisMediaStateConnected)
        {
            switch (Adapter->RoamingMode) {
                case FAST_ROAMING_MODE:
                    break;
                case ACTIVE_ROAMING_MODE:
                    break;
                case SMLS_ROAMING_MODE:
                    DBGPRINT(DBG_ROAM|DBG_HELP,(L"Get RSSI_Low\r\n"));
                    wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_ROAM);
                    break;
                default:
                    ///Not supported roaming => doing nothing
                    break;
            }
        }    
        else 
        {
            DBGPRINT(DBG_CCX|DBG_HELP,(L"Media is disconnected. Ignore it now\r\n"));
        }

       break;
       case MACREG_INT_CODE_SNR_LOW:
          DBGPRINT(DBG_MACEVT|DBG_HELP, (L"MACREG_INT_CODE_SNR_LOW\r\n"));
       break;
       case MACREG_INT_CODE_MAX_FAIL:
          DBGPRINT(DBG_MACEVT|DBG_HELP, (L"MACREG_INT_CODE_MAX_FAIL\r\n"));
       break;
               
       case MACREG_INT_CODE_RSSI_HIGH: 
          DBGPRINT(DBG_MACEVT|DBG_HELP, (L"MACREG_INT_CODE_RSSI_HIGH\r\n"));
        if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
            DBGPRINT(DBG_ROAM|DBG_HELP,(L"Get RSSI_HIGH\r\n"));
            wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_STABLE);
        }
       break;
       case MACREG_INT_CODE_SNR_HIGH: 
          DBGPRINT(DBG_MACEVT|DBG_HELP, (L"MACREG_INT_CODE_SNR_HIGH\r\n"));
       break;      
          
       default:
       break;
   }
    return;
}


/******************************************************************************
 *
 *  Name: MacEventDisconnected()
 *
 *  Description: MAC event handler for link lost condition, disassociation,
 *          deauthentication
 *
 *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
MacEventDisconnected(
  PMRVDRV_ADAPTER Adapter,
  UINT TimerValue,
  BOOLEAN PowerFlag
  )
{
	BOOLEAN                            timerStatus;
	PFASTROAMPARAM      pfrParam = &Adapter->frParam;
	DBGPRINT(DBG_PS | DBG_NEWCMD, (L"MacEventDisconnected() \r\n"));

	if ((Adapter->MediaConnectStatus != NdisMediaStateDisconnected) && 
		(Adapter->PSMode == Ndis802_11PowerModeMAX_PSP)) {
		PSWakeup(Adapter);
	}
	if((Adapter->MediaConnectStatus == NdisMediaStateConnected)&&(Adapter->bAvoidScanAfterConnectedforMSecs == TRUE)) {
		NdisMCancelTimer(&Adapter->MrvDrvAvoidScanAfterConnectedTimer, &timerStatus);
		Adapter->bAvoidScanAfterConnectedforMSecs=FALSE;         
	}

	if (NdisEqualMemory(pfrParam->ccxLastAP.alMACAddr, pfrParam->ccxCurrentAP.alMACAddr, sizeof(NDIS_802_11_MAC_ADDRESS))) {
		///The Adapter->ccxCurrentAP is an valid data, complete the data by recording 
		///     the dis-association time
		pfrParam->ccxLastAP.alDisassocTime = GetTickCount();
		DBGPRINT(DBG_ROAM|DBG_ASSO|DBG_HELP,(L"Disconnected, Ticks: %d \r\n", pfrParam->ccxLastAP.alDisassocTime));
	}
	NdisZeroMemory(&pfrParam->ccxCurrentAP, sizeof(CCX_AP_INFO));
	DBGPRINT(DBG_ROAM|DBG_ASSO|DBG_HELP, (L"Get Disconnected Event\r\n"));
	ResetDisconnectStatus(Adapter);
    
	return; 
}

/******************************************************************************
 *
 *  Name: HandleCommandFinishedEvent()
 *
 *  Description: Command finished event handler
 * *  Arguments:  PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_FAILURE
 *      NDIS_STATUS_NOT_ACCEPTED
 *  Notes:               
 *
 *****************************************************************************/
NDIS_STATUS
HandleCommandFinishedEvent(
  IN PMRVDRV_ADAPTER Adapter)
{
    USHORT Ret;
    BOOLEAN  timerStatus;
    PHostCmd_DS_GEN            pRetPtr;
    PHostCmd_DS_GET_HW_SPEC        pHWSpec;
    PHostCmd_DS_802_11_SCAN_RSP      pScanResponse;
    PHostCmd_DS_802_11_ASSOCIATE_RESULT  pAssoResult;
    PHostCmd_DS_802_11_AD_HOC_RESULT   pAdHocResult;
    USHORT                 Channel=0;
    LONG                 lCurRSSI=0; 
     
    
    //Coverity Error id:4 (REVERSE_INULL)
    //Code Move here to fix Coverity code analysis
    if ( !Adapter->CurCmd ) 
    {
       // False alarm
       DBGPRINT(DBG_CMD | DBG_ERROR,
                (L"**** hwacproc:  False trigger for command ****\r\n"));
       NdisAcquireSpinLock(&Adapter->FreeQSpinLock);
       ResetCmdBuffer(Adapter);  
       NdisReleaseSpinLock(&Adapter->FreeQSpinLock);
       return NDIS_STATUS_NOT_ACCEPTED;
    } 

    // Get the command buffer
    pRetPtr = (PHostCmd_DS_GEN)Adapter->CurCmd->BufVirtualAddr;   

    Ret = pRetPtr->Command;    

    DBGPRINT(DBG_V9|DBG_CUSTOM, (L"HWAC: Received cmd resp for 0x%x\r\n",Ret));

    if (Adapter->CurCmd->CmdFlags & CMD_F_HOSTCMD)
    {
        Adapter->CurCmd->CmdFlags &= ~CMD_F_HOSTCMD;

        if (Adapter->CommandTimerSet)
        {
  
          NdisMCancelTimer(
              &Adapter->MrvDrvCommandTimer, 
              &timerStatus);

          Adapter->CommandTimerSet = FALSE;
        }

        ReturnCmdNode(Adapter, Adapter->CurCmd);
        Adapter->CurCmd = NULL;
        GetCmdFromQueueToExecute (Adapter);

        return NDIS_STATUS_SUCCESS;
       }
    

    if ( Adapter->CurCmd->ExpectedRetCode != pRetPtr->Command )
    {
        DBGPRINT(DBG_ERROR, (L"ERROR: Received different cmd response from expected!! Expected: 0x%x, Got: 0x%x\r\n",
                             Adapter->CurCmd->ExpectedRetCode, pRetPtr->Command));
        ReturnCmdNode(Adapter, Adapter->CurCmd);
        Adapter->CurCmd = NULL;
        GetCmdFromQueueToExecute (Adapter);
  
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    // Now we got response from FW, cancel the command timer
    if (Adapter->CommandTimerSet)
    {
  
      NdisMCancelTimer(
          &Adapter->MrvDrvCommandTimer, 
          &timerStatus);

          Adapter->CommandTimerSet = FALSE;
    }


    // if the current return code is not 80xx, it's a time-out command
    if ( (Ret & HostCmd_RET_NONE) == 0 ) 
    {
      DBGPRINT(DBG_CMD|DBG_ERROR,(L"*** hwacproc: Command timeout from FW!!\r\n"));

      ReturnCmdNode(Adapter, Adapter->CurCmd);
      Adapter->CurCmd = NULL;
      GetCmdFromQueueToExecute (Adapter);

      return NDIS_STATUS_FAILURE;
    }
 
                                 
    if ( pRetPtr->Result != HostCmd_RESULT_OK ) 
         DBGPRINT(DBG_ERROR,(L"COMMAND %x FAIL %x \r\n",Ret, pRetPtr->Result));

  //dralee_20051119. Just force MrvIstThread get CPU time
  //if MrvIstThread occupy CmdQueueExeSection critical section.
  EnterCriticalSection(&Adapter->CmdQueueExeSection);
  LeaveCriticalSection(&Adapter->CmdQueueExeSection); 
 
  switch( Ret ) {

    case HostCmd_RET_802_11_WMM_GET_STATUS:
        wmm_update_status_from_cmd_resp( Adapter );
        break;
//tt ++ v5 firmware
        case HostCmd_RET_802_11_BG_SCAN_CONFIG:
        {
           PHostCmd_DS_802_11_BG_SCAN_CONFIG   pCfg = (PHostCmd_DS_802_11_BG_SCAN_CONFIG) pRetPtr;
           DBGPRINT(DBG_ROAM|DBG_CMD|DBG_HELP, (L"Get HostCmd_RET_802_11_BG_SCAN_CONFIG\r\n"));
           if ( pRetPtr->Result == HostCmd_RESULT_OK )
           {
              Adapter->bBgScanEnabled = pCfg->Enable;             
           }
           else
           {
              Adapter->bBgScanEnabled = FALSE;
           }        
        }            
        break;
            
        case HostCmd_RET_802_11_BG_SCAN_QUERY: 
        {
        DBGPRINT(DBG_ROAM|DBG_CMD|DBG_HELP, (L"Get HostCmd_RET_802_11_BG_SCAN_QUERY\r\n"));
    if ( pRetPtr->Result == HostCmd_RESULT_OK )
            {
               PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP    pBgScanResponse; 
               pBgScanResponse = (PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP) pRetPtr;
               {
                   PBSS_DESCRIPTION_SET_ALL_FIELDS     pDesc;
                   pDesc = (PBSS_DESCRIPTION_SET_ALL_FIELDS) (pBgScanResponse+1);
               }
               HandleBgScanResponse( pBgScanResponse, Adapter );

               //011506
               Application_Event_Notify(Adapter,NOTIFY_SCAN_RDY);  

               if(Adapter->bPSEnableBGScan==TRUE)
               {
                  Adapter->PSMode = Ndis802_11PowerModeMAX_PSP;
                  Adapter->PSMode_B = Ndis802_11PowerModeMAX_PSP;     
                  Adapter->bPSEnableBGScan=FALSE;
                  PSSleep(Adapter);  
               }
            }
            else
               DBGPRINT( DBG_SCAN, (L"[Mrvl] Failed to get background scan results!\r\n") );  
        }   
        break;


    case HostCmd_RET_802_11_KEY_ENCRYPT:
    {
         PHostCmd_DS_802_11_KEY_ENCRYPT pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_KEY_ENCRYPT) pRetPtr;

    }
        break;
    case HostCmd_RET_802_11_CRYPTO:
         {        
             PHostCmd_DS_802_11_CRYPTO cmdcry;
             PHostCmd_DS_802_11_CRYPTO pCmdResult;
             USHORT   *a;
 
             cmdcry =  (PHostCmd_DS_802_11_CRYPTO)pRetPtr;
                
             DBGPRINT(DBG_CMD|DBG_HELP,(L"CMD RESPONSE--------------------\r\n"));
             DBGPRINT(DBG_CMD|DBG_HELP,(L"Enc:%x,Agm:%x,KL:%x\r\n",
                          cmdcry->EncDec,cmdcry->Algorithm,cmdcry->KeyLength) );
              
             a = (USHORT *)(&cmdcry->Key[0]); 
             DBGPRINT(DBG_CMD|DBG_HELP,(L"%4x,%4x,%4x,%4x,%4x,%4x,%4x,%4x\r\n",a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7] ));
             //RETAILMSG(1,(L"--%4x,%4x,%4x,%4x,%4x,%4x,%4x,%4x\r\n",a[8],a[9],a[10],a[11],a[12],a[13],a[14],a[15]));

             DBGPRINT(DBG_CMD|DBG_HELP,(L"T:%x,L:%x\r\n",
                          cmdcry->TLVBuf.Header.Type,cmdcry->TLVBuf.Header.Len ) );

             a = (USHORT *)(&cmdcry->TLVBuf.payload[0]); 
             DBGPRINT(DBG_CMD|DBG_HELP,(L"%4x,%4x,%4x,%4x,%4x,%4x,%4x,%4x\r\n",a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7] ));

             pCmdResult = (PHostCmd_DS_802_11_CRYPTO) pRetPtr;

         }
        break;

//ahan [2005-12-13] ++dralee_20060217
    case HostCmd_RET_802_11_SUBSCRIBE_EVENT:
        {
            PHostCmd_DS_802_11_SUBSCRIBE_EVENT  pEventCmd;
            pEventCmd = (PHostCmd_DS_802_11_SUBSCRIBE_EVENT) pRetPtr;

            Adapter->SubscribeEvents = pEventCmd->Events;
            if (pEventCmd->Action == HostCmd_ACT_GET)
            {
                DBGPRINT(DBG_CMD|DBG_HELP, (L"Adapter->SubscribeEvents = [%x]\r\n", 
                                              Adapter->SubscribeEvents));
            }
        }
        break;
//ahan [2005-12-13] ++dralee_20060217

    case HostCmd_RET_802_11_LED_CONTROL:
    {
         PHostCmd_DS_802_11_LED_CONTROL pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_LED_CONTROL) pRetPtr;

    }
        break;
        
    case HostCmd_RET_802_11_CAL_DATA_EXT:
    {
         PHostCmd_DS_802_11_CAL_DATA_EXT pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_CAL_DATA_EXT) pRetPtr;

    }
        break;

    case HostCmd_RET_802_11_PWR_CFG:
    {
         PHostCmd_DS_802_11_PWR_CFG pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_PWR_CFG) pRetPtr;

    }

        break;

    case HostCmd_RET_802_11_TPC_CFG:
    {
            PHostCmd_DS_802_11_TPC_CFG      pCmdResult;
            
         pCmdResult = (PHostCmd_DS_802_11_TPC_CFG) pRetPtr;

        }
    break;
        
    case HostCmd_RET_802_11_RATE_ADAPT_RATESET:
    {
            PHostCmd_RATE_ADAPT_RATESET pCmdResult;

            pCmdResult = (PHostCmd_RATE_ADAPT_RATESET) pRetPtr;

        }
    break;

//tt --

    case HostCmd_RET_HW_SPEC_INFO:
      pHWSpec = (PHostCmd_DS_GET_HW_SPEC)pRetPtr;
      HandleHardwareSpec(pHWSpec, Adapter);
    break;

    case HostCmd_RET_802_11_RESET:

      DBGPRINT(DBG_CMD,(L"HWAC - Reset command successful\r\n"));  
  
      SetMacControl(Adapter);
    break;


    case HostCmd_RET_802_11_PS_MODE:    
        DBGPRINT(DBG_PS,(L"PS command response \r\n"));
        UpdatePowerSaveState( Adapter, Adapter->CurCmd, 0);
      
      break;
    case HostCmd_RET_802_11_SCAN:
            pScanResponse = (PHostCmd_DS_802_11_SCAN_RSP) pRetPtr;
              
            HandleScanResponse(pScanResponse, Adapter);
		if (Adapter->ChannelNum > 0) {
	            Adapter->MeasureDurtion = 0;
	            Adapter->CurCmd->Pad[2] |= MRVDRV_SCAN_CMD_END;
	            Adapter->bIsScanInProgress = FALSE;

	            DBGPRINT(DBG_CMD|DBG_CCX|DBG_HELP,(L"Reporting Beacon-Table (PassiveScan: %d)\r\n", Adapter->isPassiveScan));
	            wlan_ccx_send_BeaconTable(Adapter);
	            Adapter->isPassiveScan = FALSE;
	            Adapter->ChannelNum = 0;
		}

    break;

    case HostCmd_RET_802_11_ASSOCIATE:
    case HostCmd_RET_802_11_REASSOCIATE:
        pAssoResult = (PHostCmd_DS_802_11_ASSOCIATE_RESULT) 
              (Adapter->CurCmd->BufVirtualAddr);

        HandleAssocReassoc(pAssoResult, Adapter); 
        ///pmkcache: bug#16956 ++
        Adapter->bIsReconnectAssociation = FALSE;  
        ///pmkcache: bug#16956 --

        if (Adapter->pReconectArg != NULL) {
            SetEvent(Adapter->pReconectArg->hroamEvent);
        }
    break;

    case HostCmd_RET_802_11_AD_HOC_STOP:
         Adapter->bIsMoreThanOneStaInAdHocBSS = FALSE;
    break;
    
    case HostCmd_RET_802_11_AD_HOC_JOIN:
    case HostCmd_RET_802_11_AD_HOC_START:
      pAdHocResult = (PHostCmd_DS_802_11_AD_HOC_RESULT) 
              (Adapter->CurCmd->BufVirtualAddr);
      HandleAdHocJoinStart(pAdHocResult, Ret, Adapter);
    break;
  
    case HostCmd_RET_802_11_DEAUTHENTICATE:
    if ( Adapter->CurCmd->PendingInfo != HostCmd_PENDING_ON_SET_OID )
    {
        //#ifdef MRVL_ROAMING
        //if (Adapter->pReconectArg->skipDeauth == FALSE) {
        //#endif ///MRVL_ROAMING
            ///Skip indicating the disconnected status to host
            ///Otherwise, the WZC will issuing CMDs to scan->assoc...

        //#ifdef MRVL_ROAMING
        //if ((Adapter->RoamingMode == SMLS_ROAMING_MODE)?(wlan_roam_query_isConnecting(Adapter->pwlanRoamParam) == FALSE):TRUE) {
        //#endif ///MRVL_ROAMING

        if ( Adapter->bIsReConnectNow != 1 ) {
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"HostCmd_RET_802_11_DEAUTHENTICATE, reset connect status\r\n"));
            ResetDisconnectStatus(Adapter); 
        }
        //#ifdef MRVL_ROAMING
        //} 
        //else 
        //{
        //    DBGPRINT(DBG_CMD|DBG_CCX|DBG_HELP,(L"Not calling ResetDisconnectStatus in HandleCommandFinishedEvent() if fastroaming\r\n"));
        //    Adapter->pReconectArg->skipDeauth = FALSE;
        //}
        //#endif ///MRVL_ROAMING

    } 
    else 
    {
        DBGPRINT(DBG_CMD|DBG_CCX|DBG_HELP,(L"Not calling ResetDisconnectStatus in HandleCommandFinishedEvent() if fastroaming\r\n"));
    }
    break;
            

        case HostCmd_RET_802_11_GRP_KEY:

            break;

        case HostCmd_RET_MAC_REG_ACCESS:
    case HostCmd_RET_BBP_REG_ACCESS:
    case HostCmd_RET_RF_REG_ACCESS:
    break;


    case HostCmd_RET_802_11_HOST_SLEEP_CFG:
    {  
       if( IsThisHostPowerState(Adapter, HTWK_STATE_PRESLEEP) )
           SetHostPowerState( Adapter, HTWK_STATE_SLEEP);
       else
       {   
           SetHostPowerState( Adapter, HTWK_STATE_FULL_POWER); 
            
           //011506
           if( Adapter->SleepCfg.ulCriteria == -1 )
               Application_Event_Notify(Adapter,NOTIFY_HS_DEACTIVATED);
       }
    } 
    break;
    case HostCmd_RET_802_11_HOST_SLEEP_AWAKE_CONFIRM:
         DBGPRINT(DBG_HOSTSLEEP|DBG_PS|DBG_HELP, (L"[Marvell]HostCmd_RET_802_11_HOST_SLEEP_AWAKE_CONFIRM:Host Awake Confirm cmd resp 0x8044=0x%04X received. \r\n",pRetPtr->Command));  
         //011506 
         Application_Event_Notify(Adapter,NOTIFY_HS_DEACTIVATED);

      break;
      case HostCmd_RET_CMD_802_11_HOST_SLEEP_ACTIVATE:
           Adapter->bPSConfirm=TRUE; 
           //011506
           Application_Event_Notify(Adapter,NOTIFY_HS_ACTIVATED); 
      break;
      
      case HostCmd_RET_802_11_IBSS_COALESING_STATUS: 
      {  
          PHostCmd_802_11_IBSS_Coalesing_Status  Ico; 
          
          Ico = (PHostCmd_802_11_IBSS_Coalesing_Status)pRetPtr;
          if(Ico->Action == HostCmd_ACT_GEN_GET)
          { 
             NdisMoveMemory( &(Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex].MacAddress),
                             Ico->bssid, 
                             MRVDRV_ETH_ADDR_LEN);
            
             Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex].Configuration.ATIMWindow = Ico->AtimWindow;
             Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex].Configuration.BeaconPeriod = Ico->BeanconInterval; 
                              
        //ERP Information
        Adapter->PSBssDescList[Adapter->ulCurrentBSSIDIndex].ERPFlags = (UCHAR) Ico->UseGRateProtection;
             DBGPRINT(DBG_CMD|DBG_ADHOC|DBG_HELP,(L"CURRENT BSSID:%x,%x,%x,%x,%x,%x\r\n",
                         Ico->bssid[0],
                         Ico->bssid[1],
                         Ico->bssid[2],
                         Ico->bssid[3],
                         Ico->bssid[4],
                         Ico->bssid[5]));

             DBGPRINT(DBG_CMD|DBG_ADHOC|DBG_HELP,(L"IBSS COALESCING:ATIM:%x,beacon Ivl:%x\r\n",
                                                  Ico->AtimWindow, Ico->BeanconInterval));
          } 
      }
      break;

    case HostCmd_RET_802_11_QUERY_TRAFFIC:
    case HostCmd_RET_MAC_CONTROL:
    if ( Ret == HostCmd_RET_MAC_CONTROL )
    {
        PHostCmd_DS_MAC_CONTROL  pMacCtrl = (PHostCmd_DS_MAC_CONTROL) pRetPtr;
        DbgWmmMsg( (L"+wmm+ mac control resp: action=0x%x\r\n", pMacCtrl->Action) );
    }
    if ( Ret == HostCmd_RET_MAC_CONTROL )
    {
        PHostCmd_DS_MAC_CONTROL  pMacCtrl = (PHostCmd_DS_MAC_CONTROL) pRetPtr;
        
        if ( pMacCtrl->Action & HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON )
        {
            DBGPRINT( DBG_ADHOC, (L"[MRVL] response for enabling g protection (result=0x%x)\r\n", pMacCtrl->Result ) );
        }
    }
    case HostCmd_RET_MAC_MULTICAST_ADR:
    case HostCmd_RET_802_11_SET_WEP:
    case HostCmd_RET_802_11_DEEP_SLEEP: 
    case HostCmd_RET_802_11_FW_WAKE_METHOD:
    case HostCmd_RET_802_11_INACTIVITY_TIMEOUT: 
    case HostCmd_RET_802_11_SLEEP_PARAMS: 
    case HostCmd_RET_802_11_SLEEP_PERIOD: 
        

        break;  
    case HostCmd_RET_802_11_SNMP_MIB:
         break;
    case HostCmd_RET_802_11D_DOMAIN_INFO:
    break;  

    case HostCmd_RET_802_11_WMM_ADDTS_REQ:
    {
        HandleADDTSResponse((PHostCmd_DS_802_11_WMM_ADDTS)pRetPtr, Adapter);
    }
    break;
    case HostCmd_RET_802_11_RSSI:
        HandleRSSI((PHostCmd_DS_802_11_RSSI_RSP)pRetPtr, Adapter);
        break;
    default:
      break;  
  } 
   
  //
  // Check pending OID or command
  //
  if ( Adapter->CurCmd->PendingInfo == HostCmd_PENDING_ON_CMD )
    HandleHostPendCommand(Ret,Adapter,pRetPtr); 
  else if ( Adapter->CurCmd->PendingInfo == HostCmd_PENDING_ON_GET_OID )
    HandleHostPendGetOid(Ret,Adapter,pRetPtr);
  else if ( Adapter->CurCmd->PendingInfo == HostCmd_PENDING_ON_SET_OID )
    HandleHostPendSetOid(Ret,Adapter);
  
  ReturnCmdNode(Adapter, Adapter->CurCmd);
  Adapter->CurCmd = NULL;
  GetCmdFromQueueToExecute (Adapter);

  //RETAILMSG(1,(TEXT("[Marvell]-HandleCommandFinishedEvent"))); 
  return NDIS_STATUS_SUCCESS;
}

static VOID
HandleADDTSResponse(
        PHostCmd_DS_802_11_WMM_ADDTS pAddts, PMRVDRV_ADAPTER pAdapter)
{
    PWMM_ADDTS_RESP     pAddTSResp = &pAdapter->AddTSResp;
    USHORT offset;
            
    DBGPRINT(DBG_CCX_V4|DBG_WMM, (L"Enter HandleADDTSResponse \r\n"));
    {
              int i;
              UCHAR   *datpt = (UCHAR*)pAddts->ExtraIeData;
              DBGPRINT(DBG_TMP|DBG_HELP, (L"ExtraIeData: \r\n"));
              for (i=0 ; i<(256+15)/16 ; i++, datpt += 16) {
                  DBGPRINT(DBG_TMP|DBG_HELP, (L"%02x %02x %02x %02x %02x %02x %02x %02x  - %02x %02x %02x %02x %02x %02x %02x %02x \r\n", 
                      datpt[0], datpt[1], datpt[2], datpt[3], datpt[4], datpt[5], datpt[6], datpt[7],
                      datpt[8], datpt[9], datpt[10], datpt[11], datpt[12], datpt[13], datpt[14], datpt[15]) );
              }
    }
      

    pAddTSResp->TspecResult = pAddts->TspecResult;
    pAddTSResp->TimeOut = pAddts->TimeOut;
    pAddTSResp->DialogToken = pAddts->DialogToken;
    pAddTSResp->IEEEStatus = pAddts->IEEEStatus;
    NdisMoveMemory(pAddTSResp->TspecData, pAddts->TspecData, sizeof(pAddTSResp->TspecData));
    NdisMoveMemory(pAddTSResp->ExtraIeData, pAddts->ExtraIeData, sizeof(pAddts->ExtraIeData));
        if (Find_Ie_Traffic_Stream_Metric(&(pAddts->ExtraIeData[0]), 256, &offset))
        {
            PUCHAR ie = (PUCHAR)(&pAddts->ExtraIeData[0]) + offset;
            UCHAR   ie_len = *(ie +1);   
			

            DBGPRINT(DBG_CCX_V4, (L"HostCmd_RET_802_11_WMM_ADDTS_REQ \r\n"));
            if (pAdapter->bEnableS56DriverIAPP)   
            HandleCcxv4S56Event(
                               pAdapter,
                               ie, 
                               ie_len); 
        }
    return;
}

/******************************************************************************
 *
 *  Name: HandleHardwareSpec()
 *
 *  Description: Handle hardware spec command
 *
 *  Arguments:  PHostCmd_DS_802_11_SCAN_RSP pRetPtr
 *        PMRVDRV_ADAPTER Adapter
 *          
 *
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleHardwareSpec(
  PHostCmd_DS_GET_HW_SPEC pHWSpec,
  PMRVDRV_ADAPTER Adapter
  )
{
  ULONG i;
  UCHAR TempBuf[4];
    UCHAR ucA, ucB, ucC, ucP;


    // check if the interface number is correct

    if ( pHWSpec->HWIfVersion != 2 )
    {
        DBGPRINT(DBG_ERROR, (L"Fatal error: HW interface version mismatch! \r\n"));
        DBGPRINT(DBG_ERROR, (L"Expected version 2, got %d\r\n", pHWSpec->HWIfVersion));
        ASSERT(FALSE);
    }
    
  Adapter->HardwareStatus = NdisHardwareStatusReady;

  // permanent address should only be set once at start up
  //dralee 081005, V4
  if ( Adapter->PermanentAddr[0] == 0xff )
  {
    // permanent address has not been set yet, set it

    NdisMoveMemory(
      (PVOID)Adapter->PermanentAddr, 
      (PVOID)pHWSpec->PermanentAddr,
      MRVDRV_ETH_ADDR_LEN);

     DBGPRINT(DBG_CMD | DBG_LOAD|DBG_ALLEN,(L"*** PermanentAddr %02x.%02x.%02x.%02x.%02x.%02x ***\r\n", 
                                Adapter->PermanentAddr[0],
                          Adapter->PermanentAddr[1],
                          Adapter->PermanentAddr[2],
                          Adapter->PermanentAddr[3],
                          Adapter->PermanentAddr[4],
                          Adapter->PermanentAddr[5]
                             ));
  }
 
  NdisMoveMemory(
    &(Adapter->FWReleaseNumber), 
    &(pHWSpec->FWReleaseNumber),
    4);

    ucA = (UCHAR)((Adapter->FWReleaseNumber >> 16) & 0xff);
    ucB = (UCHAR)((Adapter->FWReleaseNumber >> 8 ) & 0xff);
    ucC = (UCHAR)((Adapter->FWReleaseNumber      ) & 0xff);
  ucP = (UCHAR)((Adapter->FWReleaseNumber >> 24) & 0xff);
    DBGPRINT(DBG_ALL,(L"*** FW Release number (0x%x) %d.%d.%d.%d ***\r\n", 
             Adapter->FWReleaseNumber, ucA, ucB, ucC, ucP));

    {   // Add FW version to registry
    REG_VALUE_DESCR Vals[2];
    wchar_t buf[50];
    
    swprintf(buf, TEXT("%d.%d.%d.%d"), ucA, ucB, ucC, ucP);
    Vals[0].val_data = (PBYTE)buf;
    Vals[0].val_name = TEXT("FWVersion");
    Vals[0].val_type = REG_SZ;
    Vals[1].val_name = NULL;
	///Display the firmware version to debug message
	NKDbgPrintfW(TEXT("[WiFi]: Firmware Version: %s\n"), buf);

    ///assoc-patch ++
    Adapter->fmver[0] = ucA;
    Adapter->fmver[1] = ucB;
    Adapter->fmver[2] = ucC;
    Adapter->fmver[3] = ucP;
    ///assoc-patch --
    AddKeyValues ((TEXT("Comm\\") TEXT(IFSTRN) TEXT(CHIPSTRN)), Vals);
    }

  DBGPRINT(DBG_CMD,(L"*** Got permanent addr: %2x %2x %2x %2x %2x %2x ***\r\n", 
    pHWSpec->PermanentAddr[0],
    pHWSpec->PermanentAddr[1],
    pHWSpec->PermanentAddr[2],
    pHWSpec->PermanentAddr[3],
    pHWSpec->PermanentAddr[4],
    pHWSpec->PermanentAddr[5]));

    SetEvent( Adapter->hHwSpecEvent );

  //lykao, 060205, for WinCE50 retail mode, Data Abort
// region-code  #ifndef MRVL_WINCE50  
  // Get the region code 
    // if it's unidentified region code, use the default (USA)
  Adapter->RegionTableIndex = 0;
    // the upper byte is the primary region code
    Adapter->RegionCode = pHWSpec->RegionCode; //junius
// region-code  #endif  

  for (i=0; i<MRVDRV_MAX_REGION_CODE; i++)
  {

    // use the region code to search for the index
    if ( Adapter->RegionCode == RegionCodeToIndex[i] )
    {
    
      Adapter->RegionTableIndex = (USHORT) i;
      //RETAILMSG(1,(L"HW Spec. regioncode:%x, index:%x\r\n",Adapter->RegionCode,i)); //dralee_20060502 

      break;
    }
  }

    // if it's unidentified region code, use the default (USA)
  if (i == MRVDRV_MAX_REGION_CODE)
    {
        //dralee_20060502 for test
        //RETAILMSG(1,(L"errous regioncode in HW spec. used default region code:0x10\r\n"));
  
        DBGPRINT(DBG_ERROR, (L"Region code not found, value returned by FW = 0x%x.  Default to 0x10!\r\n", Adapter->RegionCode));
        Adapter->RegionCode = 0x10;
        Adapter->RegionTableIndex = 0;
    }

// region-code
    DBGPRINT(DBG_LOAD, (L"Region code = 0x%x \r\n", Adapter->RegionCode));
    SetRegionCode(Adapter);   //set region band
// region-code

   //dralee 081005, V4  
  if( Adapter->CurrentAddr[0] == 0xff )
  { 
    NdisMoveMemory(
        (PVOID)Adapter->CurrentAddr, 
        (PVOID)pHWSpec->PermanentAddr,
        MRVDRV_ETH_ADDR_LEN);
  }
  
  NdisMoveMemory(
    (PVOID)TempBuf,
    (PVOID)pHWSpec->PermanentAddr,
    4);
    


        TempBuf[3] = MRVL_8100_SDIO_VER_ID;   
  
  NdisMoveMemory(
    (PVOID)&Adapter->VendorID,
    (PVOID)TempBuf,
    4);

    Adapter->FWCapInfo = pHWSpec->fwCapInfo;

    DBGPRINT(DBG_CMD, (L"FW Cap Info = 0x%x\r\n", Adapter->FWCapInfo));

} 

/******************************************************************************
 *
 *  Name: CopyBSSIDInfo()
 *
 *  Description: Copy BSSID Info
 *
 *  Arguments:  PCONDOR_ADAPTER Adapter
 *          
 *
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
CopyBSSIDInfo(
  PMRVDRV_ADAPTER Adapter
  )
{
    PNDIS_WLAN_BSSID_EX pBSSIDListSrc;
    PMRV_BSSID_IE_LIST   pIESrc;

    pBSSIDListSrc = Adapter->PSBSSIDList;
    pIESrc = Adapter->PSIEBuffer;
    
    NdisMoveMemory( &Adapter->CurrentBSSIDDesciptor,
                    &pBSSIDListSrc[Adapter->ulCurrentBSSIDIndex],
                    sizeof(NDIS_WLAN_BSSID_EX));

    NdisMoveMemory( &Adapter->CurrentBSSIDIEBuffer,
                    &pIESrc[Adapter->ulCurrentBSSIDIndex],
                    sizeof(MRV_BSSID_IE_LIST));

    NdisMoveMemory( &Adapter->CurrentSSID,
                &(pBSSIDListSrc[Adapter->ulCurrentBSSIDIndex].Ssid),
                sizeof(NDIS_802_11_SSID));
    
  // Set the new BSSID (AP's MAC address) to current BSSID
  NdisMoveMemory( Adapter->CurrentBSSID,
            &(pBSSIDListSrc[Adapter->ulCurrentBSSIDIndex].MacAddress), 
            MRVDRV_ETH_ADDR_LEN);
    
  // Make a copy of current BSSID descriptor
  NdisMoveMemory( &(Adapter->CurrentBssDesciptor), 
          &(Adapter->PSBssDescList[Adapter->ulCurrentBSSIDIndex]), 
          sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));

  // Set the new configuration to the current config
  NdisMoveMemory( &Adapter->CurrentConfiguration, 
          &(pBSSIDListSrc[Adapter->ulCurrentBSSIDIndex].Configuration),
          sizeof(NDIS_802_11_CONFIGURATION));

    ///ACTIVE_ROAMING ++
      if (Adapter->CurrentConfiguration .DSConfig > 5000)
     {
         Adapter->connected_channel= (UCHAR)((Adapter->CurrentConfiguration.DSConfig - 5000)/5);
     Adapter->connected_band  = MRVDRV_802_11_BAND_A;    
     }
     else if (Adapter->CurrentConfiguration .DSConfig > 2407)
    {
        Adapter->connected_channel  = (UCHAR)((Adapter->CurrentConfiguration.DSConfig  - 2407)/5);
        Adapter->connected_band  = MRVDRV_802_11_BAND_BG; 
     }
    ///ACTIVE_ROAMING --
  
}

/******************************************************************************
 *
 *  Name: MrvDrvAddCurrentSSID()
 *
 *  Description: Add current SSID to the scan list if it is not on the list
 *
 *  Arguments:  Adapter             Adapter structure
 *              bSetActiveScanSSID  specific scan or not
 *          
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
MrvDrvAddCurrentSSID(PMRVDRV_ADAPTER Adapter,
                     BOOLEAN         bSetActiveScanSSID)
{
    ULONG i;

    // try to find the current SSID in the new scan list
    for (i=0; i<Adapter->ulPSNumOfBSSIDs; i++)
    {
        if ((NdisEqualMemory(Adapter->PSBSSIDList[i].Ssid.Ssid, 
                            Adapter->CurrentSSID.Ssid, 
                            Adapter->CurrentSSID.SsidLength)) &&
      (NdisEqualMemory(Adapter->CurrentBSSID, 
                            &(Adapter->PSBSSIDList[i].MacAddress), 
                            MRVDRV_ETH_ADDR_LEN)))
        { 
                break;
        }
    }

    // if we found matching SSID, update the index
    if (i < Adapter->ulPSNumOfBSSIDs)
    {
    // Set the attempted BSSID Index to current
    Adapter->ulCurrentBSSIDIndex = i;

    // Set the new BSSID (AP's MAC address) to current BSSID
    NdisMoveMemory( Adapter->CurrentBSSID,
                &(Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex].MacAddress), 
                MRVDRV_ETH_ADDR_LEN);
  
        // Make a copy of current BSSID descriptor
    NdisMoveMemory( &(Adapter->CurrentBSSIDDesciptor), 
                &(Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex]),
                        sizeof(NDIS_WLAN_BSSID_EX));

        NdisMoveMemory( &(Adapter->CurrentBSSIDIEBuffer),
                        &(Adapter->PSIEBuffer[Adapter->ulCurrentBSSIDIndex]),
                        sizeof(MRV_BSSID_IE_LIST));
    
    //36.p7++
       //NdisMoveMemory( &(Adapter->CurrentBSSIDDesciptor), 
       NdisMoveMemory( &(Adapter->CurrentBssDesciptor),
                &(Adapter->PSBssDescList[Adapter->ulCurrentBSSIDIndex]),
                        sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS));
       
            
    // Set the new configuration to the current config
    NdisMoveMemory(&Adapter->CurrentConfiguration, 
      &(Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex].Configuration),
      sizeof(NDIS_802_11_CONFIGURATION));
  
    }
    // if the current associated SSID is not contained in the list, append it
    else 
    {
        ///if(bSetActiveScanSSID != TRUE)
        {
            DBGPRINT(DBG_CMD,(L"HWAC - Append current SSID to SCAN list\r\n"));
            
            if ( i < MRVDRV_MAX_BSSID_LIST )
            {
                Adapter->ulCurrentBSSIDIndex = i;
                Adapter->ulPSNumOfBSSIDs++;
                
                NdisMoveMemory( &(Adapter->PSBSSIDList[i]), 
                          &(Adapter->CurrentBSSIDDesciptor),      
                          sizeof(NDIS_WLAN_BSSID_EX));
                
                NdisMoveMemory( &(Adapter->PSIEBuffer[i]),
                                &(Adapter->CurrentBSSIDIEBuffer),
                                sizeof(MRV_BSSID_IE_LIST));
        
        NdisMoveMemory(&Adapter->PSBssDescList[i], 
                     &(Adapter->CurrentBssDesciptor), 
                               sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS)); 
            }
        }
    }
}

///Reconnect_DiffSetting ++
#define     SECURITY_PRIV		0x0001		///PrivacyEnable bit of Capability
#define     SECURITY_WPA        0x0002
#define     SECURITY_WPA2      0x0004

static BYTE    ExtractSecuritymode(MRV_BSSID_IE_LIST    *pIEBuffer)
{
	BYTE	SecurityMode = 0;
	NDIS_802_11_VARIABLE_IEs	*pVarIE;
	UCHAR		AccumulateDatLen = 0;
	UCHAR		*ouipt;
	
	if (pIEBuffer->FixedIE.Capabilities & 0x0010) {
		SecurityMode |= SECURITY_PRIV;
		DBGPRINT(1, (L"==> Privacy Enabled \r\n"));
	} else {
		DBGPRINT(1, (L"==> Privacy Disabled \r\n"));
	}

	pVarIE = (NDIS_802_11_VARIABLE_IEs *)(&pIEBuffer->VariableIE);
	AccumulateDatLen += (pVarIE->Length + 2);
	while ((AccumulateDatLen < MRVDRV_SCAN_LIST_VAR_IE_SPACE) &&(pVarIE->Length != 0)){
		switch(pVarIE->ElementID) {
			case WPA_IE:
				ouipt = pVarIE->data;
				if (NdisEqualMemory(ouipt, "\x00\x50\xf2\01", 4)) {
					SecurityMode |= SECURITY_WPA;
					DBGPRINT(1, (L"==> WPA IE is detected\r\n "));
				}
				break;
			case WPA2_IE:
				DBGPRINT(1, (L"==> WPA2 IE is detected\r\n "));
				SecurityMode |= SECURITY_WPA2;
				break;
		}
		DBGPRINT(1, (L"==> (IE, LEN): (%xh, %d)\r\n", pVarIE->ElementID, pVarIE->Length));
		pVarIE = (NDIS_802_11_VARIABLE_IEs*)(((UCHAR*)pVarIE) + (pVarIE->Length + 2));
		AccumulateDatLen += (pVarIE->Length + 2);
	}

	return SecurityMode;
}

///Reconnect_DiffSetting --
/******************************************************************************
 *
 *  Name: HandleScanResponse()
 *
 *  Description: Handle scan results
 *
 *  Arguments:  PHostCmd_DS_802_11_SCAN_RSP pRetPtr
 *        PMRVDRV_ADAPTER Adapter
 *          
 *
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleScanResponse(
  PHostCmd_DS_802_11_SCAN_RSP pScanResponse,
  PMRVDRV_ADAPTER Adapter
    )
{
    ULONG i, j, k;
    ULONG TempV = 0;
    USHORT Flag = 0;
    BOOLEAN bFound;
    BOOLEAN bISAssociationBlockedByScan;
    BOOLEAN bSetActiveScanSSID = Adapter->SetActiveScanSSID;
    NDIS_STATUS Status, ndStat;
    MrvlIEtypes_Data_t*         pTlv;
    MrvlIEtypes_TsfTimestamp_t* pTsfTlv;
    INT                         tlvBufSize;
    INT                         bytesLeft;
    PUCHAR                      ptr;
    

    if (Adapter->CurCmd->Pad[1] & MRVDRV_ES_ASSOCIATIONBLOCKED)
        bISAssociationBlockedByScan = TRUE;
    else
        bISAssociationBlockedByScan = FALSE;

    if (Adapter->CurCmd->Pad[0] & MRVDRV_ES_NOTSPECIFICSCAN)
        bSetActiveScanSSID = FALSE;
    else
        bSetActiveScanSSID = TRUE;    

    DBGPRINT(DBG_SCAN, (L"Command %x\r\n",
                (ULONG)pScanResponse->Command));
    DBGPRINT(DBG_SCAN, (L"Size %d  SeqNum %d  Result %d \r\n",
                (ULONG)pScanResponse -> Size, 
                (ULONG)pScanResponse -> SeqNum,
                (ULONG)pScanResponse -> Result));
    
    DBGPRINT(DBG_SCAN, (L"BSSDescriptSize %d\r\n",
                (ULONG)pScanResponse->BSSDescriptSize));
    DBGPRINT(DBG_CCX|DBG_SCAN, (L"NumberOfSets    %d\r\n",
                (ULONG)pScanResponse->NumberOfSets));
      
    // Get number of BSS Descriptors
    Adapter->ulNumOfBSSIDs = pScanResponse->NumberOfSets;

    bytesLeft = pScanResponse->BSSDescriptSize;
    tlvBufSize = pScanResponse->Size - pScanResponse->BSSDescriptSize 
                 - sizeof(pScanResponse->BSSDescriptSize) 
                 - sizeof(pScanResponse->NumberOfSets) 
                 - sizeof(HostCmd_DS_GEN);

    ptr = (PUCHAR)pScanResponse;
    ptr += (sizeof(HostCmd_DS_802_11_SCAN_RSP) + bytesLeft);
    pTlv = (MrvlIEtypes_Data_t *)ptr;
    GetTsfTlvFromScanResult(pTlv, tlvBufSize, &pTsfTlv);

    // Parse the return SCAN result
    ndStat = InterpretBSSDescription(
            Adapter, 
            (PVOID)(Adapter->CurCmd->BufVirtualAddr),
            pScanResponse->BSSDescriptSize
            , HostCmd_RET_802_11_SCAN
           );

    if ( ndStat != NDIS_STATUS_SUCCESS )
    {
        DBGPRINT(DBG_SCAN | DBG_ERROR,(L"ERROR: InterpretBSSDescription returned ERROR\r\n"));
    }

    // Update networkTSF value.
    // The networkTSF is the firmware TSF value at the time 
    // the beacon or probe response was received
    if (pTsfTlv)
    {
        for(i = 0; i < Adapter->ulNumOfBSSIDs; i++)
        {
            NdisMoveMemory(&Adapter->BssDescList[i].networkTSF,
                           &pTsfTlv->tsfTable[i],
                           sizeof(ULONGLONG));
        }
    }

    //Check the whole BSSID list string if there's invalid character.
    for (Flag = 0, i = 0; (i < Adapter->ulNumOfBSSIDs) || (Flag != 0); i++)
    {
        // We'll re-check the first one when the previous first one has been discard.
        if (Flag == 1)
        {
            Flag = 0;
            i--;
        }

        do
        {
            //Check if there's invalid data rate.
            for (k = 0, TempV = 0; k < NDIS_SUPPORTED_RATES; k++)
                TempV |= Adapter->BSSIDList[i].SupportedRates[k];

            if (TempV == 0)
            {
                Flag = 1;
                DBGPRINT(DBG_SCAN|DBG_ERROR, (L"Invalid support rate\r\n")); 
                break;
            }

            // WinCE5.0 on Bulverde has problems that the system will hang up
           // if we returned a ESSID which length is 32 bytes.
           // So we discard the AP with ESSID length of 32
           //dralee_20060929, support 32bytes SSID depending on registry.
           if ((Adapter->BSSIDList[i].Ssid.SsidLength > 32) ||
               (Adapter->BSSIDList[i].Ssid.SsidLength == 32 && Adapter->ESSID_32 == 1)) 
           {   
                Flag = 1;
                break;
           }
            
           // Check the invalid SSID.
           for (j = 0; j < Adapter->BSSIDList[i].Ssid.SsidLength; j++)
           {
                if (Adapter->BSSIDList[i].Ssid.Ssid[j] < 0x20)
                {
                        DBGPRINT(DBG_SCAN, (L"INVALID BSSID Setting flag to discard (%d) %s\r\n",
                                 i, Adapter->BSSIDList[i].Ssid.Ssid));
                        DBGPRINT(DBG_SCAN, (L"i = %d, j = %d, TempV = %x, Ssid[j] = %x, dsconfig = %d\r\n",
                                 i, j, TempV, Adapter->BSSIDList[i].Ssid.Ssid[j], Adapter->BSSIDList[i].Configuration.DSConfig));
                        // Replace the current BSSID struct with last one
                        Flag = 1;
                        break;
                }
            }

            if (Adapter->BSSIDList[i].Ssid.Ssid[0] == 0x20)
            {
                Flag = 1;
                break;      
            }       
        } while (0);

        if (Flag == 1)
        {
            if ((i+1) == Adapter->ulNumOfBSSIDs)
            {
                DBGPRINT(DBG_SCAN,(L"INVALID BSSID DISCARDING LAST(%d) %s\r\n",
                         i, Adapter->BSSIDList[i].Ssid.Ssid));
                NdisZeroMemory(&(Adapter->BSSIDList[i]), 
                               sizeof(NDIS_WLAN_BSSID_EX));
                NdisZeroMemory(&(Adapter->IEBuffer[i]),
                               sizeof(MRV_BSSID_IE_LIST));
                Adapter->ulNumOfBSSIDs -= 1;
                Flag = 0;
                break;
            }
            else
            {
                //Start to replace the current invalid SSID with the last SSID in array.
                DBGPRINT(DBG_SCAN, (L"INVALID BSSID DISCARDING CURRENT(%d) %s\r\n",
                         i, Adapter->BSSIDList[i].Ssid.Ssid));
                NdisMoveMemory(&(Adapter->BSSIDList[i]), 
                               &(Adapter->BSSIDList[Adapter->ulNumOfBSSIDs-1]),      
                               sizeof(NDIS_WLAN_BSSID_EX));
                NdisZeroMemory(&(Adapter->BSSIDList[Adapter->ulNumOfBSSIDs-1]), 
                               sizeof(NDIS_WLAN_BSSID_EX));
                NdisMoveMemory(&(Adapter->IEBuffer[i]),
                               &(Adapter->IEBuffer[Adapter->ulNumOfBSSIDs-1]),
                               sizeof(MRV_BSSID_IE_LIST));
                NdisZeroMemory(&(Adapter->IEBuffer[Adapter->ulNumOfBSSIDs-1]),
                               sizeof(MRV_BSSID_IE_LIST));
                Adapter->ulNumOfBSSIDs -= 1;
            }
        } // if (Flag == 1)
    } // end of (i < Adapter->ulNumOfBSSIDs) loop

    DBGPRINT(DBG_SCAN|DBG_CUSTOM, (L"HWAC - Scanned %2d APs\r\n", Adapter->ulNumOfBSSIDs));
    
    // Active scan, append AP only if it does not already exist

        // active scan, append AP only if it does not already exist
    //051707 improve table update to match real condition 060407
    for( i=0; i < Adapter->ulNumOfBSSIDs; i++ )
    {
       ULONG index;

       index =(ULONG)FindBSSIDInPSList(Adapter, Adapter->BSSIDList[i].MacAddress); 
                      
       if(index == 0xff)
          index = (Adapter->ulPSNumOfBSSIDs % MRVDRV_MAX_BSSID_LIST);

       ReplacePSListEntryByBSSListEntry(Adapter, index, i );
       
       Adapter->PSBssEntryAge[index] = 0;
    }



/*
    ///crlo:Move this fragment after RemoveAgeOutEntryFromPSList
#ifdef MRVL_ROAMING
    if ((Adapter->RoamingMode == SMLS_ROAMING_MODE) && (Adapter->pwlanRoamParam!=NULL))
    {
        wlan_roam_ScanTableReady(Adapter->pwlanRoamParam, WRSCAN_NORMALSCAN);
    }
#endif //MRVL_ROAMING
*/

    // the progressive scan is not finished 
    if (!(Adapter->CurCmd->Pad[2] & MRVDRV_SCAN_CMD_END))
        return;

    //051707
    RemoveAgeOutEntryFromPSList(Adapter);    



    Adapter->bIsScanInProgress = FALSE;
    Adapter->bIsScanWhileConnect = FALSE;
    //041007 
    //Adapter->SetActiveScanSSID = FALSE;

    // Check if current SSID is in the list, if not, 
    // add the SSID to the list
    if ((Adapter->MediaConnectStatus == NdisMediaStateConnected) &&
        (Adapter->CurrentSSID.SsidLength !=0) &&
        (Adapter->CurrentSSID.Ssid[0] > 0x20))
    {
        MrvDrvAddCurrentSSID(Adapter, bSetActiveScanSSID);
    }

    if ((Adapter->RoamingMode == SMLS_ROAMING_MODE) && (Adapter->pwlanRoamParam!=NULL))
    {
        wlan_roam_ScanTableReady(Adapter->pwlanRoamParam, WRSCAN_NORMALSCAN);
    }

    if (bISAssociationBlockedByScan == TRUE)
    {
        // If connect to ANY, default is the first one 
        if (Adapter->bIsConnectToAny)
        {
            NdisMoveMemory(&(Adapter->AttemptedSSIDBeforeScan), 
                           &(Adapter->BSSIDList[0].Ssid), 
                           sizeof(NDIS_802_11_SSID));
      
            Adapter->ulAttemptedBSSIDIndex = 0;
        } 

        if (Adapter->InfrastructureMode == Ndis802_11IBSS)
        {
            bFound = FALSE;

            // Find out the BSSID that matches the SSID given in InformationBuffer
            for (i = 0; i< Adapter->ulPSNumOfBSSIDs; i++)
            {
                if (Adapter->PSBSSIDList[i].InfrastructureMode == Ndis802_11Infrastructure)
                {
                    continue;
                }
        
                if (Adapter->SetActiveScanSSID == TRUE)
                {
                    // If found matched on SSID, get the MAC address(BSSID)
                    if (Adapter->AttemptedSSIDBeforeScan.SsidLength != 
                            Adapter->PSBSSIDList[i].Ssid.SsidLength)
                    {
                        continue;
                    }
        
                    if (NdisEqualMemory(Adapter->PSBSSIDList[i].Ssid.Ssid, 
                                        Adapter->AttemptedSSIDBeforeScan.Ssid, 
                                        Adapter->AttemptedSSIDBeforeScan.SsidLength) == 0)
                    {
                        continue;
                    }
                }
                else if (Adapter->SetActiveScanBSSID == TRUE)
                { 
                    if (NdisEqualMemory(Adapter->PSBSSIDList[i].MacAddress, 
                                        Adapter->ActiveScanBSSID, 
                                        MRVDRV_ETH_ADDR_LEN) == 0)
                    {
                        continue;
                    }
                }   

                for (j = 1; j <= 14 ; j++)
                { 
                    //Search the channel number.
                    if (DSFeqList[j] == Adapter->PSBSSIDList[i].Configuration.DSConfig)
                        break;
                } 

                if (j < 15) 
                    Adapter->Channel = j;

                bFound = TRUE;
                break;                
            }
      
            Status = PrepareAndSendCommand(
                             Adapter,
                             HostCmd_CMD_802_11_IBSS_COALESING_STATUS,
                             HostCmd_ACT_GEN_SET,
                             HostCmd_OPTION_USE_INT,
                             (NDIS_OID)0,
                             HostCmd_PENDING_ON_NONE,
                             0,
                             FALSE,
                             NULL,
                             NULL,
                             NULL,
                             &(Adapter->IbssCoalsecingEnable));

            DBGPRINT(DBG_ADHOC|DBG_SCAN|DBG_HELP, (L"setup IBSSCOALSEcing:%x\r\n",
                     Adapter->IbssCoalsecingEnable));         
            // Check if the matching SSID is found, send JOIN command
            if (bFound)
            {
                DBGPRINT(DBG_V9, (L" >>> Find Adhoc BSS to connect \r\n"));
                      
                if (Adapter->Enable80211D)  
                {                          
                    PBSS_DESCRIPTION_SET_ALL_FIELDS pBssDescListSrc;
                    ULONG ulAttemptSSIDIdx;
             
                    ulAttemptSSIDIdx = i;

                    pBssDescListSrc = Adapter->PSBssDescList;
                    if (pBssDescListSrc[ulAttemptSSIDIdx].bHaveCountryIE)
                        UpdateScanTypeFromCountryIE(Adapter, (UCHAR)ulAttemptSSIDIdx);
                }
                else
                    ResetAllScanTypeAndPower(Adapter);   
                   
                //032007 fix adhoc indication
                Adapter->bIsMoreThanOneStaInAdHocBSS = FALSE;
                           
                Adapter->ulAttemptedBSSIDIndex = i;

                Status = PrepareAndSendCommand(
                                       Adapter,
                                       HostCmd_CMD_802_11_AD_HOC_JOIN,
                                       0,
                                       HostCmd_OPTION_USE_INT,
                                       (NDIS_OID)0,
                                       HostCmd_PENDING_ON_NONE,
                                       0,
                                       FALSE,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &(Adapter->PSBSSIDList[i]));

                Adapter->bIsAssociationBlockedByScan = FALSE;
                return;
            }
            else
            {
                DBGPRINT(DBG_CMD|DBG_ADHOC|DBG_HELP, 
                         (L" >>> Not Find Adhoc BSS to connect \r\n"));

                if (Adapter->Enable80211D)
                {             
                    UCHAR countryNum;    
       
                    countryNum = (UCHAR)Adapter->RegionCode;
                    DBGPRINT(DBG_CMD|DBG_ADHOC|DBG_HELP,
                            (L"enable 11d before adhoc associate:region:%x\r\n", countryNum));
                    PrepareAndSendCommand(
                                   Adapter, 
                                   HostCmd_CMD_802_11D_DOMAIN_INFO,
                                   HostCmd_ACT_SET,
                                   HostCmd_OPTION_USE_INT,
                                   0, 
                                   HostCmd_PENDING_ON_NONE, 
                                   0,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   NULL,
                                   (PVOID)&countryNum
                                   );   
           
                    DBGPRINT(DBG_CMD|DBG_ADHOC|DBG_HELP,
                             (L"set domain info before ahhoc start:%x\r\n", countryNum)); 
         
                    UpdateScanTypeByCountryCode(Adapter, (UCHAR)Adapter->RegionCode);  
                }
                else
                    ResetAllScanTypeAndPower(Adapter);  

                //032007 fix adhoc indication
                Adapter->bIsMoreThanOneStaInAdHocBSS = FALSE;
        
                // start ad_hoc mode
                Status = PrepareAndSendCommand(
                                 Adapter,
                                 HostCmd_CMD_802_11_AD_HOC_START,
                                 0,
                                 HostCmd_OPTION_USE_INT,
                                 (NDIS_OID)0,
                                 HostCmd_PENDING_ON_NONE,
                                 0,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &(Adapter->AttemptedSSIDBeforeScan));
            } 

            Adapter->bIsAssociationBlockedByScan = FALSE;
            return;
        }  //if (Adapter->InfrastructureMode == Ndis802_11IBSS)
       
        //042407
        Adapter->SetActiveScanSSID = FALSE;
        Adapter->SetActiveScanBSSID = FALSE;
       
        // In Infrastructure mode
        Adapter->bRetryAssociate = FALSE;

        if (Adapter->bIsReConnectNow)
        {
            INT apid = FindAPBySSID(Adapter, &(Adapter->ReSSID));
            ///Reconnect_DiffSetting ++
		{
			BYTE        PrevSecurityMode, SecurityMode;
			PrevSecurityMode = ExtractSecuritymode(&Adapter->ReIEBuffer);
			SecurityMode = ExtractSecuritymode(&Adapter->PSIEBuffer[apid]);
			///if (!NdisEqualMemory(&Adapter->ReIEBuffer.FixedIE.BeaconInterval, 
			///                                &Adapter->PSIEBuffer[apid].FixedIE.BeaconInterval, 
			///                                sizeof(MRV_BSSID_IE_LIST)-8))  {
			if (PrevSecurityMode != SecurityMode) {
				apid = -1;
			}
		}
		///Reconnect_DiffSetting --
            if (apid != -1) 
            {
                Status = PrepareAndSendCommand(
                                 Adapter,
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
                                 (PVOID)&apid);
            }
        else
        {
          Adapter->NeedSetWepKey = FALSE;
        }
          
        }
        else
        {
            INT apid = FindAPBySSID(Adapter, &(Adapter->AttemptedSSIDBeforeScan));
            if (apid != -1)
            {
                Status = PrepareAndSendCommand(
                                 Adapter,
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
                                 (PVOID)&apid);
            }
        else
        {
          Adapter->NeedSetWepKey = FALSE;
        }

        }
    
        if (Status != NDIS_STATUS_SUCCESS)
        {     
            Adapter->bIsReConnectNow = FALSE;
        }

        Adapter->bIsAssociationBlockedByScan = FALSE;
        return;
    } // end if(bISAssociationBlockedByScan == TRUE)
    else
    {
        Adapter->SetActiveScanSSID = FALSE;
        Adapter->SetActiveScanBSSID = FALSE;
    }    

}


/******************************************************************************
 *
 *  Name: HandleAdHocJoinStart()
 *
 *  Description: Handle ad hoc/join start
 *
 *  Arguments:  PHostCmd_DS_802_11_AD_HOC_RESULT pAdHocResult,
 *              USHORT Ret
 *              PMRVDRV_ADAPTER Adapter
 *          
 *
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleAdHocJoinStart(
    PHostCmd_DS_802_11_AD_HOC_RESULT pAdHocResult,
    USHORT Ret,
    PMRVDRV_ADAPTER Adapter
)
{

    Adapter->bIsAssociateInProgress = FALSE;
    Adapter->bIsAssociationBlockedByScan = FALSE;
    Adapter->SetActiveScanSSID = FALSE;

    // Join result code 0 --> SUCCESS
  
    // if join result code != 0, will return Failure later
    if ( pAdHocResult->ResultCode != 0x0 ) 
    {
        DBGPRINT(DBG_CMD|DBG_CUSTOM, (L"HWAC - Join or Start Command Failed %x \n",
                 pAdHocResult->ResultCode));
  
        if( Adapter->MediaConnectStatus ==  NdisMediaStateConnected )
        {
            ResetDisconnectStatus(Adapter);                            
        }
          
        // Clean up Adapter->LastAddedWEPKey
        NdisZeroMemory(&(Adapter->LastAddedWEPKey), 
                       sizeof(MRVL_WEP_KEY));

        // If it's a START command and it fails, 
        // remove the entry on BSSIDList nad BssDescList
        if ( Ret == HostCmd_RET_802_11_AD_HOC_START )
        {        
            NdisZeroMemory(&(Adapter->PSBSSIDList[Adapter->ulAttemptedBSSIDIndex]), 
                           sizeof(NDIS_WLAN_BSSID_EX));
            Adapter->ulNumOfBSSIDs--;
        }
    
        Adapter->bIsReConnectNow = FALSE;
        Adapter->bIsSystemConnectNow = FALSE;
    


        return;
    } 

    Adapter->MediaConnectStatus = NdisMediaStateConnected;
    Adapter->LinkSpeed = MRVDRV_LINK_SPEED_11mbps;
  
    // Set the attempted BSSID Index to current
    Adapter->ulCurrentBSSIDIndex = Adapter->ulAttemptedBSSIDIndex;
    
    // if it's a Start command, set the BSSID to the returned value
    if ( Ret == HostCmd_RET_802_11_AD_HOC_START )
    { 
        NdisMoveMemory(&(Adapter->PSBSSIDList[Adapter->ulCurrentBSSIDIndex].MacAddress),
                       pAdHocResult->BSSID, 
                       MRVDRV_ETH_ADDR_LEN);
        
        DBGPRINT(DBG_CMD|DBG_ADHOC, (L"pAdHocResult->BSSID = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n",
                    pAdHocResult->BSSID[0],
                    pAdHocResult->BSSID[1],
                    pAdHocResult->BSSID[2],
                    pAdHocResult->BSSID[3],
                    pAdHocResult->BSSID[4],
                    pAdHocResult->BSSID[5]));
    
        Adapter->AdHocCreated = TRUE;     
        //032007 fix wrong adhoc indication
        Adapter->bIsMoreThanOneStaInAdHocBSS = FALSE;    
    }
    else //032007 fix wrong adhoc indication
    {  
        //032007 fix wrong adhoc indication
        Adapter->bIsMoreThanOneStaInAdHocBSS = TRUE;   

        Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_CONNECT);   
    }

 
    CopyBSSIDInfo(Adapter);
  
    // ++ adhoc rejoin
    Adapter->AdhocDefaultChannel = Adapter->connected_channel;
    DBGPRINT(DBG_ADHOC, (L"Current AdHoc channel is %d\n", Adapter->AdhocDefaultChannel) );
    // --
  
    DBGPRINT(DBG_V9|DBG_ADHOC,(L"HWAC - Joined/Started Ad Hoc\r\n"));
  
  
  
    if (Adapter->PSMode == Ndis802_11PowerModeMAX_PSP)
        PSSleep(Adapter);


      Adapter->bIsReConnectNow = FALSE;
      Adapter->bIsSystemConnectNow = FALSE;
      InfraBssReconnectStop(Adapter);

}

//tt ++ ra fail
USHORT HowManyPacketInQueue( PMRVDRV_ADAPTER Adapter)
{
    PTXQ_KEEPER  pTxQKeeper;
    PTXQ_NODE    pTQNode;
    USHORT  nCount = 0;

EnterCriticalSection(&Adapter->TxCriticalSection);
    pTxQKeeper = (PTXQ_KEEPER)&Adapter->TXRETQ;
    pTQNode = pTxQKeeper->head;
    while( pTQNode && nCount < MAX_TX_PACKETS )
        {
//          if ( pTQNode->pPacket == NULL )
//              V5DbgMsg( (L"    !!! null pkt (%d)\n", nCount) );
            nCount ++;
            pTQNode = pTQNode->next;
        }
LeaveCriticalSection(&Adapter->TxCriticalSection); 
    return nCount;
}

USHORT HowManyFreeNodes( PMRVDRV_ADAPTER Adapter )
{
    PTXQ_NODE    pFreeNode;
    USHORT      nCount=0;

EnterCriticalSection(&Adapter->TxCriticalSection);
    pFreeNode = Adapter->TxFreeQ;
    while( pFreeNode && nCount < MAX_TX_PACKETS )
    {
        nCount ++;
        pFreeNode = pFreeNode->next;
    }
LeaveCriticalSection(&Adapter->TxCriticalSection); 
    return nCount;
}

/******************************************************************************
 *
 *  Name: HandleAssocReassoc()
 *
 *  Description: Handle associate/reassociate command
 *
 *  Arguments:  PCONDOR_ADAPTER Adapter
 *          
 *
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleAssocReassoc(
  PHostCmd_DS_802_11_ASSOCIATE_RESULT pAssoResult,
  PMRVDRV_ADAPTER Adapter
  )
{
    WPA_SUPPLICANT  *pwpa_supplicant;
    PNDIS_802_11_ASSOCIATION_INFORMATION pInfo;
    USHORT           IEOffset, IELen;
    PUCHAR           IEBuf;
    BOOLEAN          IndicateConnect;
    PFASTROAMPARAM      pfrParam = &Adapter->frParam;

    Adapter->bIsAssociateInProgress = FALSE;

    //050207 Clear ReqBSSID
    NdisZeroMemory(&(Adapter->ReqBSSID[0]), MRVDRV_ETH_ADDR_LEN);
 
  //  association result code 0 --> SUCCESS
  //  if association result code != 0, will return Failure later
    Adapter->frParam.AssocStatusCode = pAssoResult->StatusCode;
    if (pAssoResult->StatusCode != 0x0)
   {
       DBGPRINT(DBG_V9,(L"HWAC - Association Failed, code = %d\r\n", 
       pAssoResult->StatusCode));

        // clear out current association
       if( (Adapter->MediaConnectStatus ==  NdisMediaStateConnected) 
          && (Adapter->NeedDisconnectNotification == TRUE) )
      {
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"AssocReassoc, Association Failed\r\n"));
             NdisZeroMemory(&(Adapter->CurrentSSID), sizeof(NDIS_802_11_SSID));
             ResetDisconnectStatus(Adapter);                 
      } 
          // add scan to clean up the old BSSID list in firmware/driver   
       if(Adapter->bRetryAssociate == TRUE)
      {
          Adapter->bRetryAssociate = FALSE;

          PrepareAndSendCommand(
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
                                NULL);
      }
    
       Adapter->bIsReConnectNow = FALSE;
       Adapter->bIsSystemConnectNow = FALSE;

       Adapter->NeedSetWepKey = FALSE;
       return;
   }
    
    
    IEOffset = (USHORT)(ULONG)&(((PHostCmd_DS_802_11_ASSOCIATE_RESULT)0)->IELength);
    IELen = pAssoResult->Size - IEOffset;

    if(IELen == (pAssoResult->IELength + 2)) // compatible for FW before 5.0.14.0
    {
        IELen -= 2;
        IEBuf = &pAssoResult->IE[0];
    }
    else
        IEBuf = (PUCHAR)&pAssoResult->IELength;

    Adapter->nAssocRspCount += 1;

    // copy the association result info to the ASSOCIATION_INFO buffer

    pInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)Adapter->AssocInfoBuffer;

    // only currently copy the fixed IE
    // TODO copy the entire IE once it is available!
 
    pInfo->ResponseFixedIEs.Capabilities = pAssoResult->CapInfo;
    pInfo->ResponseFixedIEs.StatusCode = pAssoResult->StatusCode;
    pInfo->ResponseFixedIEs.AssociationId = pAssoResult->AssociationID;

    pInfo->AvailableResponseFixedIEs |= NDIS_802_11_AI_RESFI_CAPABILITIES;
    pInfo->AvailableResponseFixedIEs |= NDIS_802_11_AI_RESFI_STATUSCODE;
    pInfo->AvailableResponseFixedIEs |= NDIS_802_11_AI_RESFI_ASSOCIATIONID;

    pInfo->ResponseIELength = IELen;
    NdisMoveMemory( Adapter->AssocInfoBuffer + pInfo->OffsetResponseIEs,
                    IEBuf,
                    pInfo->ResponseIELength);
    if ((Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2)   ||  
            (Adapter->AuthenticationMode == Ndis802_11AuthModeWPA2PSK) ) 
   {
            pwpa_supplicant = &(Adapter->PSBssDescList[Adapter->ulAttemptedBSSIDIndex].wpa2_supplicant);    
   }       
    else
   {
            pwpa_supplicant = &(Adapter->PSBssDescList[Adapter->ulAttemptedBSSIDIndex].wpa_supplicant);     
   }       
    pInfo->ResponseIELength += pwpa_supplicant->Wpa_ie_len; 
    NdisMoveMemory( Adapter->AssocInfoBuffer + pInfo->OffsetResponseIEs + pAssoResult->IELength,
                        pwpa_supplicant->Wpa_ie,
                        pwpa_supplicant->Wpa_ie_len);


         
    DBGPRINT(DBG_V9, (L"*** Association Result IE: CapInfo = 0x%x, StatusCode = 0x%x, AssociationID = 0x%x\r\n",
                               pInfo->ResponseFixedIEs.Capabilities,
                               pInfo->ResponseFixedIEs.StatusCode,
                               pInfo->ResponseFixedIEs.AssociationId));
        

    // Set the attempted BSSID Index to current
    Adapter->ulCurrentBSSIDIndex = Adapter->ulAttemptedBSSIDIndex;  
    CopyBSSIDInfo(Adapter);

    Adapter->WmmDesc.enabled = FALSE;
    if (Adapter->CurrentBssDesciptor.Wmm_IE[0] == WPA_IE)
    {
        if ( Adapter->WmmDesc.required )
            Adapter->WmmDesc.enabled = TRUE;
    }

    Adapter->WmmDesc.WmmUapsdEnabled = FALSE;
    if ( Adapter->WmmDesc.enabled )
    {
        ///bug#16791 ++
    ///Add one more checking, "if (Adapter->sleepperiod != 0)"
        if ((Adapter->CurrentBssDesciptor.Wmm_IE[8] & 0x80) && (Adapter->sleepperiod != 0)) {
            Adapter->WmmDesc.WmmUapsdEnabled = TRUE;
    }
    ///bug#16791 --
    }




    
    //  Initialize RSSI value to specific number so the first
    //  RSSI value will be recorded as so without the average
    Adapter->LastRSSI = MRVL_DEFAULT_INITIAL_RSSI;   // default value

    Adapter->usTimeElapsedSinceLastScan = 0;
    Adapter->ulTxByteInLastPeriod = 
    Adapter->ulRxByteInLastPeriod = 0;

    // reset awake time stamp
    Adapter->ulAwakeTimeStamp = 0;

    DBGPRINT(DBG_CMD|DBG_V9|DBG_RECONNECT|DBG_WARNING,(L"HWAC - Associated\r\n"));



    if( Adapter->PSMode_B == Ndis802_11PowerModeMAX_PSP)
   {   
      Adapter->PSMode = Ndis802_11PowerModeMAX_PSP;
   } 
  

            
    if(!(Adapter->EncryptionStatus == Ndis802_11Encryption2KeyAbsent))
   {
       if (Adapter->PSMode == Ndis802_11PowerModeMAX_PSP)
      {
          NdisMSleep(300);
      }  
   }

    UpdateTsfTimestamps(Adapter, &Adapter->CurrentBssDesciptor);
                     
    Adapter->MediaConnectStatus = NdisMediaStateConnected;
    Adapter->LinkSpeed = MRVDRV_LINK_SPEED_11mbps;
    DBGPRINT(DBG_ROAM|DBG_HELP,(TEXT(__FUNCTION__)TEXT(": RoamingMode: %d \r\n"), Adapter->RoamingMode));
    if (Adapter->RoamingMode == SMLS_ROAMING_MODE) {
		///Special case for DeviceScape supplicant => They won't set OID_FSW_CCX_AUTH_SUCCESS.
		///	=> We can't postpone to set the state until the authentication is successful.
		{
                UCHAR   *ssidpt = (UCHAR*)Adapter->CurrentSSID.Ssid;
                TCHAR   apmsg[66];
                unsigned int i;
                NdisZeroMemory(apmsg, sizeof(apmsg));
                for (i=0 ; i<Adapter->CurrentSSID.SsidLength ; i++) {
                    apmsg[i] = (TCHAR) ssidpt[i];
                }
			DBGPRINT(DBG_ROAM|DBG_HELP,(TEXT(__FUNCTION__)TEXT(": Connected - \"%s\"\n\r"), Adapter->CurrentSSID.Ssid));
			DBGPRINT(DBG_ROAM|DBG_HELP,(TEXT(__FUNCTION__)TEXT(": Connected - \"%s\"\n\r"), apmsg));
			wlan_roam_set_state(Adapter->pwlanRoamParam , WRS_STABLE);

	    }
    }
            ///Record the current AP information
            NdisMoveMemory((PVOID)pfrParam->ccxCurrentAP.alMACAddr, Adapter->CurrentBSSID, MRVDRV_ETH_ADDR_LEN);
            pfrParam->ccxCurrentAP.alChannel = pfrParam->assocChannel;
            NdisZeroMemory((PVOID)&pfrParam->ccxCurrentAP.ssid, sizeof(NDIS_802_11_SSID));
            NdisMoveMemory((PVOID)&pfrParam->ccxCurrentAP.ssid, (PVOID)&Adapter->CurrentSSID, sizeof(NDIS_802_11_SSID));
            pfrParam->ccxCurrentAP.alDisassocTime = 0;
            ///Associated successful. 
            if (NdisEqualMemory(pfrParam->ccxLastAP.ssid.Ssid, pfrParam->ccxCurrentAP.ssid.Ssid, sizeof(pfrParam->ccxCurrentAP.ssid.SsidLength))) 
           {
           } 
        
            ///if (pfrParam->privacyEnabled == FALSE)
            if (pfrParam->is80211x == FALSE)
           {
               ///The associated AP is in normal mode. 
               /// => Update the cached AP information now.
               NdisMoveMemory(&pfrParam->ccxLastAP, &pfrParam->ccxCurrentAP, sizeof(CCX_AP_INFO));
           }
        
    {
           ULONG currenttime;
           
        NdisGetSystemUpTime(&currenttime); 

        DBGPRINT(DBG_CCX_V4,(L"Adapter->RoamAccount %d \r\n", Adapter->RoamAccount));

        Adapter->RoamAccount ++;
        Adapter->RoamDelay = (USHORT)(currenttime - Adapter->RoamStartTime);
    }

    switch(Adapter->RoamingMode)
    {
        case FAST_ROAMING_MODE:  
            IndicateConnect = TRUE;
            ///Subscribe an RSSI_Low event so that we can roam to a new AP if the signal is poor
            ///
            SetUpLowRssiValue(Adapter);     
            break;
       case ACTIVE_ROAMING_MODE:
        case SMLS_ROAMING_MODE:
            IndicateConnect = TRUE;
       default:
            IndicateConnect = TRUE;
            break;
    } // end of switch (Adapter->RoamingMode)


    if(IndicateConnect == TRUE)
    {
    // indicate the NDIS the media is didconnected
         
       Ndis_MediaStatus_Notify(Adapter,NDIS_STATUS_MEDIA_CONNECT); 

       DBGPRINT(DBG_ASSO|DBG_WARNING,(L"Indicate connected\r\n"));
    }
    ///pmkcache: bug#16956 ++
    if (Adapter->isPktPending == TRUE) {
        PNDIS_PACKET      pPacket;
        NDIS_STATUS        pStatus;
        ///RETAILMSG(1, (L"==>Handle Previous RxPKT now\n"));
        Adapter->isPktPending = FALSE;
        if ( Adapter->pPendedRxPkt == NULL )
        {
            DBGPRINT(DBG_ERROR, ( L"ERROR, there is no pended RX packet!!\r\n" ) );
        }
        else
        {
            pPacket = Adapter->pPendedRxPkt;
            Adapter->pPendedRxPkt = NULL;
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
    }
    ///pmkcache: bug#16956 --
    
    if (Adapter->EncryptionStatus == Ndis802_11Encryption2KeyAbsent||
      Adapter->EncryptionStatus ==Ndis802_11Encryption3KeyAbsent)
    {  
         NdisMSetTimer(&Adapter->MrvDrvAvoidScanAfterConnectedTimer, Adapter->AvoidScanTime);
         Adapter->bAvoidScanAfterConnectedforMSecs = TRUE;  
    }


      Adapter->bIsReConnectNow = FALSE;
      Adapter->bIsSystemConnectNow = FALSE;
      InfraBssReconnectStop(Adapter);

    wlan_ccx_assocSuccess();

    DBGPRINT(DBG_V9,(L" Authentication Mode %x  Encryption Status %x \r\n", 
             Adapter->AuthenticationMode, Adapter->EncryptionStatus));

    if(Adapter->NeedSetWepKey == TRUE)
    {
                    PrepareAndSendCommand(
                          Adapter,
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
                          &Adapter->LastAddedWEPKey);


        Adapter->NeedSetWepKey = FALSE;
    }
        
    Adapter->ConnectedAPAuthMode = Adapter->AuthenticationMode;

    {
        USHORT offset;
		
        if (Adapter->bEnableS56DriverIAPP)
        {
                NDIS_STATUS     Status;
				
                if (Adapter->bStartCcxS56Test == FALSE)
                {
                        DBGPRINT(DBG_CCX_V4, (L"HostCmd_CMD_802_11_WMM_ADDTS_REQ \r\n"));
				
                        Status = PrepareAndSendCommand(
                                                    Adapter,
                                                    HostCmd_CMD_802_11_WMM_ADDTS_REQ,
                                                    HostCmd_ACT_GEN_SET,
                                                    HostCmd_OPTION_USE_INT,
                                                    0,
                                                    HostCmd_PENDING_ON_NONE,
                                                    0,
                                                    FALSE,
                                                    0,
                                                    0,
                                                    0,
                                                    S56AddTspec
                                                    );
                        if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
                        {     
                               DBGPRINT(DBG_CCX_V4, (L"Add WMM_ADDTS_REQ fail \r\n")); 		   
                        } 

                        Adapter->bStartCcxS56Test = TRUE;
                }
        }			

        if (Find_Ie_Traffic_Stream_Metric(&pAssoResult->IE[0], (USHORT)pAssoResult->IELength, &offset))
        {
            PUCHAR ie = (PUCHAR)(&pAssoResult->IE[0]) + offset;
            UCHAR   ie_len = *(ie +1);   
            if (Adapter->bEnableS56DriverIAPP)
            HandleCcxv4S56Event(
                               Adapter,
                               ie, 
                               ie_len); 
        }
    }
    return;
}

static VOID
HandleRSSI(
            PHostCmd_DS_802_11_RSSI_RSP pRSSIRSP,
            PMRVDRV_ADAPTER Adapter )
{
    /* store the non average value */
    Adapter->SNR[TYPE_BEACON][TYPE_NOAVG] = pRSSIRSP->SNR ;
    Adapter->NF[TYPE_BEACON][TYPE_NOAVG]  = pRSSIRSP->NoiseFloor ;
        
    Adapter->SNR[TYPE_BEACON][TYPE_AVG] = pRSSIRSP->AvgSNR;
    Adapter->NF[TYPE_BEACON][TYPE_AVG] = pRSSIRSP->AvgNoiseFloor ;
        
    Adapter->RSSI[TYPE_BEACON][TYPE_NOAVG] = 
        (SHORT)CAL_RSSI(Adapter->SNR[TYPE_BEACON][TYPE_NOAVG],
                    Adapter->NF[TYPE_BEACON][TYPE_NOAVG]);

    Adapter->RSSI[TYPE_BEACON][TYPE_AVG] = 
        (SHORT)CAL_RSSI(Adapter->SNR[TYPE_BEACON][TYPE_AVG],
                    Adapter->NF[TYPE_BEACON][TYPE_AVG] );

    Adapter->ulRSSITickCount=GetTickCount();
    if ((Adapter->RSSI[TYPE_BEACON][TYPE_AVG] > -10) || (Adapter->RSSI[TYPE_BEACON][TYPE_AVG] < -200)) {
        //Ling++, 011206
        if(Adapter->MediaConnectStatus == NdisMediaStateConnected)
        {
            DBGPRINT(DBG_ERROR, (L"ERROR: Incorrect RSSI Value1 - SNR = %d, NF= %d, Adapter->RSSI[TYPE_BEACON][TYPE_AVG] = %d, Adapter->LastRSSI = %d\r\n", 
                       pRSSIRSP->SNR, 
                       pRSSIRSP->NoiseFloor,
                       Adapter->RSSI[TYPE_BEACON][TYPE_AVG],
                       Adapter->LastRSSI));
        }
        //Ling--, 011206       
    }
    else
    {    
        Adapter->LastRSSI = (LONG)Adapter->RSSI[TYPE_BEACON][TYPE_AVG];     
    }

    DBGPRINT(DBG_RSSI|DBG_HELP, (L"HostCmd_RET_802_11_RSSI: (%d, %d, %d, %d)\r\n",
                    Adapter->RSSI[0][0], 
                    Adapter->RSSI[0][1], 
                    Adapter->RSSI[1][0], 
                    Adapter->RSSI[1][1]));
    return;
}
    
/******************************************************************************
 *
 *  Name: HandleHostPendCommand()
 *
 *  Description: Handle any pending commands
 *
 *  Arguments:  USHORT Ret
 *        PMRVDRV_ADAPTER Adapter
 *        PHostCmd_DS_GEN pRetPtr
 *  
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID 
HandleHostPendCommand(
  USHORT Ret, 
  PMRVDRV_ADAPTER Adapter,
  PHostCmd_DS_GEN pRetPtr
  )
{
  PHostCmd_DS_GET_HW_SPEC pHwSpec;
  // LONG lCurRSSI; 
  // PHostCmd_DS_802_11_RSSI pCmdPtrRSSIandSQ;
  
  switch(Ret){

    case HostCmd_RET_HW_SPEC_INFO:
    
      pHwSpec = (PHostCmd_DS_GET_HW_SPEC)Adapter->CurCmd->InformationBuffer;
      
      NdisMoveMemory(
        (PVOID)pHwSpec, 
        (PHostCmd_DS_GET_HW_SPEC)pRetPtr, 
        sizeof(HostCmd_DS_GET_HW_SPEC));

/* tt v5 firmware
      NdisMQueryInformationComplete(
        Adapter->MrvDrvAdapterHdl,
        NDIS_STATUS_SUCCESS);
*/
      break;  
                        
      case HostCmd_RET_802_11_RF_CHANNEL:           
      case HostCmd_RET_MAC_REG_ACCESS:
      case HostCmd_RET_BBP_REG_ACCESS:
      case HostCmd_RET_RF_REG_ACCESS: 
      case HostCmd_RET_802_11_RADIO_CONTROL:
        
        if( *((USHORT *)pRetPtr + 4)==HostCmd_ACT_GEN_READ){          
      
          ULONG SizeOfBytesWritten;
        
          if(Ret==HostCmd_RET_802_11_RF_CHANNEL){
            SizeOfBytesWritten=sizeof(HostCmd_DS_802_11_RF_CHANNEL);
          }
          else if (Ret==HostCmd_RET_MAC_REG_ACCESS){
            SizeOfBytesWritten=sizeof(HostCmd_DS_MAC_REG_ACCESS);
          }   
          else if (Ret==HostCmd_RET_BBP_REG_ACCESS){
            SizeOfBytesWritten=sizeof(HostCmd_DS_BBP_REG_ACCESS); 
          }                   
          else if (Ret==HostCmd_RET_RF_REG_ACCESS){
            SizeOfBytesWritten=sizeof(HostCmd_DS_RF_REG_ACCESS);
          }
          else if (Ret==HostCmd_RET_802_11_RADIO_CONTROL){
            SizeOfBytesWritten=sizeof(HostCmd_DS_802_11_RADIO_CONTROL);
          }           
      
          if( Adapter->CurCmd->InformationBuffer ){
          
            NdisMoveMemory(
              (PVOID)Adapter->CurCmd->InformationBuffer,
              (PVOID)Adapter->CurCmd->BufVirtualAddr,
              SizeOfBytesWritten);
          }

          if( Adapter->CurCmd->BytesWritten )
            *(Adapter->CurCmd->BytesWritten) = SizeOfBytesWritten; 
        }

/* tt v5 firmware
        NdisMQueryInformationComplete(
          Adapter->MrvDrvAdapterHdl,
          NDIS_STATUS_SUCCESS);       
*/
      break;                    

        ///Move it to HandleCommandFinishedEvent
//    case HostCmd_RET_802_11_RSSI:
//        HandleRSSI((PHostCmd_DS_802_11_RSSI_RSP)pRetPtr, Adapter);
//        break;
      default:
        break;
  }   
    SetEvent(Adapter->hPendOnCmdEvent);
  
}   


/******************************************************************************
 *
 *  Name: HandleHostPendGetOid()
 *
 *  Description: Handle any Get OIDs
 *
 *  Arguments:  USHORT Ret
 *        PCONDOR_ADAPTER Adapter
 *        PHostCmd_DS_GEN pRetPtr
 *  
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/

//unsigned char aaa[32];


VOID 
HandleHostPendGetOid(
  USHORT Ret, 
  PMRVDRV_ADAPTER Adapter,
  PHostCmd_DS_GEN pRetPtr
  )
{
  NDIS_802_11_ANTENNA *p802dot11Antenna;
  // NDIS_802_11_RSSI *p802dot11RSSI; 
  PHostCmd_DS_802_11_SNMP_MIB pSNMPMIB;
  PHostCmd_DS_802_11_RF_ANTENNA pAntenna;
  PHostCmd_DS_802_11_DATA_RATE pDataRate;
   
  //lykao, 020105
  NDIS_802_11_TX_POWER_LEVEL *p802dot11TxPowerLevel;
  PHostCmd_DS_802_11_RF_TX_POWER pRFTxPower;

  USHORT usTemp;
  UCHAR *pDot11DataRate;

  NDIS_802_11_RSSI *p802dot11RSSI;
  PHostCmd_DS_802_11_RSSI_RSP pRSSIRSP;


  PHostCmd_TX_RATE_QUERY  pTXRATE;

  // LONG lCurRSSI; 

    if ( (! Adapter->CurCmd->InformationBuffer) || 
         (! Adapter->CurCmd->BytesWritten) )
    {
        DBGPRINT(DBG_ERROR, (L"ERROR: No information buffer or bytes written in OID_QUERY reply!\r\n"));
        NdisMQueryInformationComplete(
        Adapter->MrvDrvAdapterHdl,
        NDIS_STATUS_FAILURE);
    }

    DBGPRINT(DBG_OID, (L"HandleHostPendGetOid: Ret=0x%x\r\n", Ret));    
  switch(Ret) {

//tt ++ v5 firmware
            case HostCmd_RET_802_11_BG_SCAN_QUERY:
            {
         PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP) Adapter->CurCmd->BufVirtualAddr;

         if ( (sizeof(NDIS_OID)) > 0 )
                 NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                 (PUCHAR) &(pCmdResult->ReportCondition),
                 pCmdResult->Size-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_BG_SCAN_QUERY_RSP *)0)->ReportCondition)) );

         (Adapter)->nSizeOfOidCmdResp = pCmdResult->Size-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_BG_SCAN_QUERY_RSP *)0)->ReportCondition));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
            }           
            return;

    case HostCmd_RET_802_11_KEY_ENCRYPT:
    {
         PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP) Adapter->CurCmd->BufVirtualAddr;

         if ( (sizeof(NDIS_OID)) > 0 )
                 NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                 (PUCHAR) &(pCmdResult->ReportCondition),
                 pCmdResult->Size-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_BG_SCAN_QUERY_RSP *)0)->ReportCondition)) );

         (Adapter)->nSizeOfOidCmdResp = pCmdResult->Size-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_BG_SCAN_QUERY_RSP *)0)->ReportCondition));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 051207
         Signal_Pending_OID(Adapter);
    }
        return;

    case HostCmd_RET_802_11_CRYPTO:  
    {
         PHostCmd_DS_802_11_CRYPTO pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_CRYPTO) Adapter->CurCmd->BufVirtualAddr;

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (0),
                 (PUCHAR) &(pCmdResult->EncDec),
                 sizeof(HostCmd_DS_802_11_CRYPTO)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_CRYPTO *)0)->EncDec)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_CRYPTO)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_CRYPTO *)0)->EncDec));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 051207
         Signal_Pending_OID(Adapter);
    }
        return;
    case HostCmd_RET_802_11_POWER_ADAPT_CFG_EXT:
        {
     PHostCmd_DS_802_11_POWER_ADAPT_CFG_EXT pCmdResult;

     pCmdResult = (PHostCmd_DS_802_11_POWER_ADAPT_CFG_EXT) Adapter->CurCmd->BufVirtualAddr;

     if ( ((sizeof(NDIS_OID)+2)) > 0 )
         NdisZeroMemory( (Adapter)->OidCmdRespBuf, ((sizeof(NDIS_OID)+2)) );

     NdisMoveMemory(
         (Adapter)->OidCmdRespBuf + ((sizeof(NDIS_OID)+2)),
         (PUCHAR) &(pCmdResult->Action),
         sizeof(HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT *)0)->Action)) );

     (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT *)0)->Action));
     (Adapter)->nOidCmdResult = pCmdResult->Result;

     DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
     //SetEvent( (Adapter)->hOidQueryEvent ); 060407
     Signal_Pending_OID(Adapter);
    };
        return;
    case HostCmd_RET_802_11_LED_CONTROL:
    {
         PHostCmd_DS_802_11_LED_CONTROL pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_LED_CONTROL) Adapter->CurCmd->BufVirtualAddr;

         if ( (sizeof(NDIS_OID)) > 0 )
                 NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                 (PUCHAR) &(pCmdResult->NumLed),
                 sizeof(HostCmd_DS_802_11_LED_CONTROL)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_LED_CONTROL *)0)->NumLed)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_LED_CONTROL)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_LED_CONTROL *)0)->NumLed));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }
        return;

    case HostCmd_RET_802_11_CAL_DATA_EXT:
    {
         PHostCmd_DS_802_11_CAL_DATA_EXT pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_CAL_DATA_EXT) Adapter->CurCmd->BufVirtualAddr;

         if ( (sizeof(NDIS_OID)) > 0 )
                 NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                 (PUCHAR) &(pCmdResult->Revision),
                 sizeof(HostCmd_DS_802_11_CAL_DATA_EXT)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_CAL_DATA_EXT *)0)->Revision)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_CAL_DATA_EXT)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_CAL_DATA_EXT *)0)->Revision));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }

        return;

    case HostCmd_RET_802_11_PWR_CFG:
    {
         PHostCmd_DS_802_11_PWR_CFG pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_PWR_CFG) Adapter->CurCmd->BufVirtualAddr;

         if ( (sizeof(NDIS_OID)) > 0 )
                 NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                 (PUCHAR) &(pCmdResult->Enable),
                 sizeof(HostCmd_DS_802_11_PWR_CFG)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_PWR_CFG *)0)->Enable)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_PWR_CFG)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_PWR_CFG *)0)->Enable));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
        }

        return;
    case HostCmd_RET_802_11_TPC_CFG:
        {
         PHostCmd_DS_802_11_TPC_CFG pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_TPC_CFG) Adapter->CurCmd->BufVirtualAddr;

         if ( (sizeof(NDIS_OID)) > 0 )
                 NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                 (PUCHAR) &(pCmdResult->Enable),
                 sizeof(HostCmd_DS_802_11_TPC_CFG)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_TPC_CFG *)0)->Enable)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_TPC_CFG)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_TPC_CFG *)0)->Enable));
         (Adapter)->nOidCmdResult = pCmdResult->Result;
            
         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
        }
        return;
    case HostCmd_RET_802_11_RATE_ADAPT_RATESET:
    {
         PHostCmd_RATE_ADAPT_RATESET pCmdResult;

         pCmdResult = (PHostCmd_RATE_ADAPT_RATESET) Adapter->CurCmd->BufVirtualAddr;
         //0327
         //if ( (sizeof(NDIS_OID)) > 0 )
         //        NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf,
                 (PUCHAR) &(pCmdResult->EnableHwAuto),
                 sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET)); 
                 //sizeof(HostCmd_RATE_ADAPT_RATESET)-((LONG)(LONG_PTR)&(((HostCmd_RATE_ADAPT_RATESET *)0)->EnableHwAuto)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(OID_MRVL_DS_RATE_ADAPT_RATESET);//sizeof(HostCmd_RATE_ADAPT_RATESET)-((LONG)(LONG_PTR)&(((HostCmd_RATE_ADAPT_RATESET *)0)->EnableHwAuto));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"Cmd Ret Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
        }
    return;
    case HostCmd_RET_BBP_REG_ACCESS:
        {
            PHostCmd_DS_CMD_BBP_REG_ACCESS pCmd;
            POID_MRVL_DS_BBP_REGISTER_ACCESS pUserBuffer;

        pUserBuffer = (POID_MRVL_DS_BBP_REGISTER_ACCESS) Adapter->OidCmdRespBuf; 
            pCmd = (PHostCmd_DS_CMD_BBP_REG_ACCESS)pRetPtr;
        pUserBuffer->ulValue = (ULONG) pCmd->Value; 
        Adapter->nOidCmdResult = pCmd->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(OID_MRVL_DS_BBP_REGISTER_ACCESS); 
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", pUserBuffer->ulValue));
            
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
        //SetEvent( (Adapter)->hOidQueryEvent ); 060407
        Signal_Pending_OID(Adapter);
        }

    return;

    case HostCmd_RET_802_11_GET_STAT:
        {
            PHostCmd_DS_802_11_GET_STAT pCmd;
            PNDIS_802_11_STATISTICS pUserBuffer;

        pUserBuffer = (PNDIS_802_11_STATISTICS) Adapter->OidCmdRespBuf; 
        pCmd = (PHostCmd_DS_802_11_GET_STAT)pRetPtr;
        pUserBuffer->Length =pCmd->Length; 

        DBGPRINT(DBG_CUSTOM,(L"HostCmd_RET_802_11_GET_STAT:Length=%d\r\n",pUserBuffer->Length ));
	 pUserBuffer->TransmittedFragmentCount.LowPart= (DWORD) pCmd->TransmittedFragmentCount; 
	 pUserBuffer->TransmittedFragmentCount.HighPart= 0; 
         pUserBuffer->MulticastTransmittedFrameCount.LowPart= (DWORD) pCmd->MulticastTransmittedFrameCount; 
	 pUserBuffer->MulticastTransmittedFrameCount.HighPart= 0; 
	 pUserBuffer->FailedCount.LowPart= (DWORD) pCmd->FailedCount; 
	 pUserBuffer->FailedCount.HighPart= 0; 
	 pUserBuffer->RetryCount.LowPart= (DWORD) pCmd->RetryCount; 
	 pUserBuffer->RetryCount.HighPart= 0; 
	 pUserBuffer->MultipleRetryCount.LowPart= (DWORD) pCmd->MultipleRetryCount; 
	 pUserBuffer->MultipleRetryCount.HighPart= 0; 
	 pUserBuffer->RTSSuccessCount.LowPart= (DWORD) pCmd->RTSSuccessCount; 
	 pUserBuffer->RTSSuccessCount.HighPart= 0; 
	 pUserBuffer->RTSFailureCount.LowPart= (DWORD) pCmd->RTSFailureCount; 
	 pUserBuffer->RTSFailureCount.HighPart= 0; 
	 pUserBuffer->ACKFailureCount.LowPart= (DWORD) pCmd->ACKFailureCount; 
	 pUserBuffer->ACKFailureCount.HighPart= 0; 
	 pUserBuffer->FrameDuplicateCount.LowPart= (DWORD) pCmd->FrameDuplicateCount; 
	 pUserBuffer->FrameDuplicateCount.HighPart= 0; 
	 pUserBuffer->ReceivedFragmentCount.LowPart= (DWORD) pCmd->ReceivedFragmentCount; 
	 pUserBuffer->ReceivedFragmentCount.HighPart= 0; 
	 pUserBuffer->MulticastReceivedFrameCount.LowPart= (DWORD) pCmd->MulticastReceivedFrameCount; 
	 pUserBuffer->MulticastReceivedFrameCount.HighPart= 0; 
	 pUserBuffer->FCSErrorCount.LowPart= (DWORD) pCmd->FCSErrorCount; 
	 pUserBuffer->FCSErrorCount.HighPart= 0; 

	//below item was under #ifndef UNDER_CE
#ifndef UNDER_CE
	 pUserBuffer->TKIPLocalMICFailures.LowPart= (DWORD) pCmd->TKIPLocalMICFailures; 
	 pUserBuffer->TKIPLocalMICFailures.HighPart= 0; 
         pUserBuffer->TKIPICVErrorCount.LowPart= (DWORD) pCmd->TKIPICVErrorCount; 
	 pUserBuffer->TKIPICVErrorCount.HighPart= 0; 
	 pUserBuffer->TKIPCounterMeasuresInvoked.LowPart= (DWORD) pCmd->TKIPCounterMeasuresInvoked; 
	 pUserBuffer->TKIPCounterMeasuresInvoked.HighPart= 0; 
	 pUserBuffer->TKIPReplays.LowPart= (DWORD) pCmd->TKIPReplays; 
	 pUserBuffer->TKIPReplays.HighPart= 0; 
	 pUserBuffer->CCMPFormatErrors.LowPart= (DWORD) pCmd->CCMPFormatErrors; 
	 pUserBuffer->CCMPFormatErrors.HighPart= 0; 
	 pUserBuffer->CCMPReplays.LowPart= (DWORD) pCmd->CCMPReplays; 
	 pUserBuffer->CCMPReplays.HighPart= 0; 
	 pUserBuffer->CCMPDecryptErrors.LowPart= (DWORD) pCmd->CCMPDecryptErrors; 
	 pUserBuffer->CCMPDecryptErrors.HighPart= 0; 
	 pUserBuffer->FourWayHandshakeFailures.LowPart= (DWORD) pCmd->FourWayHandshakeFailures; 
	 pUserBuffer->FourWayHandshakeFailures.HighPart= 0; 
	 pUserBuffer->WEPUndecryptableCount.LowPart= (DWORD) pCmd->WEPUndecryptableCount; 
	 pUserBuffer->WEPUndecryptableCount.HighPart= 0; 
	 pUserBuffer->WEPICVErrorCount.LowPart= (DWORD) pCmd->WEPICVErrorCount; 
	 pUserBuffer->WEPICVErrorCount.HighPart= 0; 
	 pUserBuffer->DecryptSuccessCount.LowPart= (DWORD) pCmd->DecryptSuccessCount; 
	 pUserBuffer->DecryptSuccessCount.HighPart= 0; 
	 pUserBuffer->DecryptFailureCount.LowPart= (DWORD) pCmd->DecryptFailureCount; 
	 pUserBuffer->DecryptFailureCount.HighPart= 0; 
#endif
	
        Adapter->nOidCmdResult = pCmd->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(NDIS_802_11_STATISTICS); 
       
        Signal_Pending_OID(Adapter);
        }

    return;

	
    case HostCmd_RET_MAC_REG_ACCESS:
        {
            PHostCmd_DS_CMD_MAC_REG_ACCESS pCmd;
            POID_MRVL_DS_MAC_REGISTER_ACCESS pUserBuffer;

        pUserBuffer = (POID_MRVL_DS_MAC_REGISTER_ACCESS) Adapter->OidCmdRespBuf; 
            pCmd = (PHostCmd_DS_CMD_MAC_REG_ACCESS)pRetPtr;
        pUserBuffer->ulValue = (ULONG) pCmd->Value; 
        Adapter->nOidCmdResult = pCmd->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(OID_MRVL_DS_MAC_REGISTER_ACCESS); 
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", pUserBuffer->ulValue));

        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
        //SetEvent( (Adapter)->hOidQueryEvent ); 060407
        Signal_Pending_OID(Adapter);
        }
    return;

    case HostCmd_RET_RF_REG_ACCESS:
        {
            PHostCmd_DS_CMD_RF_REG_ACCESS pCmd;
            POID_MRVL_DS_RF_REGISTER_ACCESS pUserBuffer;

        pUserBuffer = (POID_MRVL_DS_RF_REGISTER_ACCESS) Adapter->OidCmdRespBuf; 
            pCmd = (PHostCmd_DS_CMD_RF_REG_ACCESS)pRetPtr;
        pUserBuffer->ulValue = (ULONG) pCmd->Value; 
        Adapter->nOidCmdResult = pCmd->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(OID_MRVL_DS_RF_REGISTER_ACCESS); 
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", pUserBuffer->ulValue));
            
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
        //SetEvent( (Adapter)->hOidQueryEvent ); 060407
        Signal_Pending_OID(Adapter);
        }
    return;

    case HostCmd_RET_MAC_ADDRESS:
    {
         PHostCmd_DS_CMD_MAC_ADDRESS pCmdResult;

         pCmdResult = (PHostCmd_DS_CMD_MAC_ADDRESS) Adapter->CurCmd->BufVirtualAddr;

         if ( (sizeof(NDIS_OID)) > 0 )
                 NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                 (PUCHAR) &(pCmdResult->MacAddress[0]),
                 sizeof(HostCmd_DS_CMD_MAC_ADDRESS)-((LONG)(LONG_PTR)&(((HostCmd_DS_CMD_MAC_ADDRESS *)0)->MacAddress[0])) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_CMD_MAC_ADDRESS)-((LONG)(LONG_PTR)&(((HostCmd_DS_CMD_MAC_ADDRESS *)0)->MacAddress[0]));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }
        return;

    case HostCmd_RET_REGION_CODE:
    {
        POID_MRVL_DS_REGION_CODE    pUserBuffer2;
        PHostCmd_DS_CMD_REGION_CODE pCmd; 
        POID_MRVL_DS_REGION_CODE pUserBuffer; 
        

        pUserBuffer = (POID_MRVL_DS_REGION_CODE) Adapter->OidCmdRespBuf; 
        pCmd = (PHostCmd_DS_CMD_REGION_CODE) pRetPtr; 
        pUserBuffer->usRegionCode = (USHORT) pCmd->RegionCode; 
        Adapter->nOidCmdResult = pCmd->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(OID_MRVL_DS_REGION_CODE); 
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", pUserBuffer->usRegionCode));

        pUserBuffer2 = (POID_MRVL_DS_REGION_CODE) Adapter->OidCmdRespBuf;
        pUserBuffer2->usBand = 0xffff; //tt v5 doesn't use it.

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
        }
    return;



    case HostCmd_RET_802_11_RF_ANTENNA: 
    {
        p802dot11Antenna=(NDIS_802_11_ANTENNA *)Adapter->OidCmdRespBuf;
        pAntenna = (PHostCmd_DS_802_11_RF_ANTENNA)pRetPtr;
        *p802dot11Antenna=(NDIS_802_11_ANTENNA)(pAntenna->AntennaMode);

        Adapter->nOidCmdResult = pAntenna->Result;
        Adapter->nSizeOfOidCmdResp = sizeof(NDIS_802_11_ANTENNA);
        //SetEvent( (Adapter)->hOidQueryEvent ); 060407
        Signal_Pending_OID(Adapter);
        }
    return;
    case HostCmd_RET_802_11_SNMP_MIB:

        
    if (Adapter->CurCmd->PendingOID == OID_802_11_FRAGMENTATION_THRESHOLD)
    {
        pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pRetPtr;
        usTemp = *((PUSHORT)(pSNMPMIB->Value)); 
        *((NDIS_802_11_FRAGMENTATION_THRESHOLD *)Adapter->OidCmdRespBuf)
            =(NDIS_802_11_FRAGMENTATION_THRESHOLD)usTemp;
        Adapter->nSizeOfOidCmdResp = sizeof(NDIS_802_11_FRAGMENTATION_THRESHOLD);
    }     
    else if (Adapter->CurCmd->PendingOID == OID_802_11_RTS_THRESHOLD)
    {
        pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pRetPtr;
        usTemp = *((PUSHORT)(pSNMPMIB->Value)); 
        *((NDIS_802_11_RTS_THRESHOLD *)Adapter->OidCmdRespBuf)
            =(NDIS_802_11_RTS_THRESHOLD)usTemp;     
        Adapter->nSizeOfOidCmdResp = sizeof(NDIS_802_11_RTS_THRESHOLD);
    }  //dralee_20060502
    else if( Adapter->CurCmd->PendingOID == OID_802_11D_ENABLE )
    { 
        pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pRetPtr;
        usTemp = *((PUSHORT)(pSNMPMIB->Value)); 
        *((USHORT *)Adapter->OidCmdRespBuf)
            =(USHORT)usTemp;     
        Adapter->nSizeOfOidCmdResp = 2;    
    }
    else //Coverity Error id:20 (UNINIT)
    {
        DBGPRINT(DBG_ERROR, (L"Unmatched pending OID:%x\r\n",Adapter->CurCmd->PendingOID));
        pSNMPMIB = (PHostCmd_DS_802_11_SNMP_MIB)pRetPtr;  
    }

    Adapter->nOidCmdResult = pSNMPMIB->Result;

    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
    //SetEvent( (Adapter)->hOidQueryEvent ); 060407
    Signal_Pending_OID(Adapter);
    return;
    
    case HostCmd_RET_802_11_RF_TX_POWER:
        {
        (p802dot11TxPowerLevel) = (NDIS_802_11_TX_POWER_LEVEL *) Adapter->OidCmdRespBuf; 
        (pRFTxPower) = (PHostCmd_DS_802_11_RF_TX_POWER) pRetPtr; 
        *(p802dot11TxPowerLevel) = (NDIS_802_11_TX_POWER_LEVEL)((pRFTxPower)->CurrentLevel); 
        Adapter->nOidCmdResult = (pRFTxPower)->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(NDIS_802_11_TX_POWER_LEVEL); 
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", *(p802dot11TxPowerLevel)));

        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
        //SetEvent( (Adapter)->hOidQueryEvent ); 060407
        Signal_Pending_OID(Adapter);
        }

        return;
    
    case HostCmd_RET_802_11_DATA_RATE:
    {
        ULONG   idx;

        pDot11DataRate=(UCHAR *)(Adapter->OidCmdRespBuf);
        pDataRate=(PHostCmd_DS_802_11_DATA_RATE)pRetPtr;

        Adapter->nOidCmdResult = pDataRate->Result;
        
        for ( idx = 0; idx < NDIS_SUPPORTED_RATES; idx++ )
        {
            if ( (pDataRate->DataRate[idx] == 0) && (idx != 0) )
            {
                // no more valid value according to 
                // FW's format
                break;
            }
            pDot11DataRate[idx] = ConvertFWIndexToNDISRate(pDataRate->DataRate[idx]);
            DBGPRINT(DBG_CMD, (L" DATARATE x2: %d\r\n", pDot11DataRate[idx]));
        }

        if( idx > 1) // return auto rate to UI
        { 
            pDot11DataRate[0]=0;
            pDot11DataRate[1]=1;
            Adapter->nSizeOfOidCmdResp = 2;
        }
        else
            Adapter->nSizeOfOidCmdResp = 1;

         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);

        }
    return;
        

    case HostCmd_RET_802_11_RF_CHANNEL: 
        {
        PHostCmd_DS_802_11_RADIO_CONTROL pCmd; 
        POID_DS_MRVL_RF_RADIO pUserBuffer; 
               
        pUserBuffer = (POID_DS_MRVL_RF_RADIO) Adapter->OidCmdRespBuf; 
        pCmd = (PHostCmd_DS_802_11_RADIO_CONTROL) pRetPtr; 
        pUserBuffer->RADIO = (UCHAR) pCmd->Control; 
        Adapter->nOidCmdResult = pCmd->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(OID_DS_MRVL_RF_RADIO); 
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", pUserBuffer->RADIO));

     DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
     //SetEvent( (Adapter)->hOidQueryEvent ); 060407
     Signal_Pending_OID(Adapter);
    }
    return;


        case HostCmd_RET_802_11_RADIO_CONTROL:
    {
                              PHostCmd_DS_802_11_RADIO_CONTROL pCmd;
                  POID_DS_MRVL_RF_RADIO    pUserBuffer;

        pUserBuffer = (POID_DS_MRVL_RF_RADIO) Adapter->OidCmdRespBuf; 
                  pCmd = (PHostCmd_DS_802_11_RADIO_CONTROL)pRetPtr;
        pUserBuffer->RADIO = (UCHAR) pCmd->Control; 
        Adapter->nOidCmdResult = pCmd->Result; 
        Adapter->nSizeOfOidCmdResp = sizeof(OID_DS_MRVL_RF_RADIO); 
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", pUserBuffer->RADIO));

     DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
     //SetEvent( (Adapter)->hOidQueryEvent ); 060407
     Signal_Pending_OID(Adapter);
    }
    return;
    
     case HostCmd_RET_802_11_KEY_MATERIAL:
        {
            PHostCmd_DS_802_11_KEY_MATERIAL pCmd = ( PHostCmd_DS_802_11_KEY_MATERIAL)pRetPtr;

            //V5DbgMsg( (L"[Marvell]HostCmd_RET_802_11_KEY_MATERIAL: Key[0]=%d,Key[1]=%d,Key[2]=%d,Key[29]=%d,Key[30]=%d,Key[31]=%d,\n", pCmd->KeyParamSet.Key[0],pCmd->KeyParamSet.Key[1],pCmd->KeyParamSet.Key[2],pCmd->KeyParamSet.Key[29],pCmd->KeyParamSet.Key[30],pCmd->KeyParamSet.Key[31],) );

            Adapter->nOidCmdResult = pCmd->Result;
            NdisMoveMemory(Adapter->aeskey.KeyParamSet.Key, pCmd ->KeyParamSet.Key, sizeof( pCmd->KeyParamSet.Key));
                HandleAdhocAES( Adapter,
                    pCmd,
                    Adapter->OidCmdRespBuf,
                    &(Adapter->nSizeOfOidCmdResp) );
            
            //SetEvent( (Adapter)->hOidQueryEvent ); 060407
            Signal_Pending_OID(Adapter);
            }
        return;


       case HostCmd_RET_802_11_WMM_ACK_POLICY:
       {
     PHostCmd_DS_802_11_WMM_ACK_POLICY pCmdResult;

     pCmdResult = (PHostCmd_DS_802_11_WMM_ACK_POLICY) Adapter->CurCmd->BufVirtualAddr;

     NdisMoveMemory(
         (Adapter)->OidCmdRespBuf,
         (PUCHAR) &(pCmdResult->AC),
         sizeof(HostCmd_DS_802_11_WMM_ACK_POLICY)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_WMM_ACK_POLICY *)0)->AC)) );

     (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_WMM_ACK_POLICY)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_WMM_ACK_POLICY *)0)->AC));
     (Adapter)->nOidCmdResult = pCmdResult->Result;

     DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
     //SetEvent( (Adapter)->hOidQueryEvent ); 060407
     Signal_Pending_OID(Adapter);
       }

       return;      
       case HostCmd_RET_802_11_WMM_GET_STATUS:
       {
     PHostCmd_DS_802_11_WMM_GET_STATUS pCmdResult;

     pCmdResult = (PHostCmd_DS_802_11_WMM_GET_STATUS) Adapter->CurCmd->BufVirtualAddr;

     NdisMoveMemory(
         (Adapter)->OidCmdRespBuf,
         (PUCHAR) &(pCmdResult->Status[0]),
         sizeof(HostCmd_DS_802_11_WMM_GET_STATUS)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_WMM_GET_STATUS *)0)->Status[0])) );

     (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_WMM_GET_STATUS)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_WMM_GET_STATUS *)0)->Status[0]));
     (Adapter)->nOidCmdResult = pCmdResult->Result;

     DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
     //SetEvent( (Adapter)->hOidQueryEvent ); 060407
     Signal_Pending_OID(Adapter);
       }

       return;
       

    case HostCmd_RET_802_11_INACTIVITY_TIMEOUT:
    {
         PHostCmd_DS_802_11_INACTIVITY_TIMEOUT pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_INACTIVITY_TIMEOUT) Adapter->CurCmd->BufVirtualAddr;

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf,
                 (PUCHAR) &(pCmdResult->timeout),
                 sizeof(HostCmd_DS_802_11_INACTIVITY_TIMEOUT)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_INACTIVITY_TIMEOUT *)0)->timeout)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_INACTIVITY_TIMEOUT)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_INACTIVITY_TIMEOUT *)0)->timeout));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }
        return;     
    case HostCmd_RET_802_11_FW_WAKE_METHOD:
    {
         PHostCmd_DS_802_11_FW_WAKE_METHOD pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_FW_WAKE_METHOD) Adapter->CurCmd->BufVirtualAddr;

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf,
                 (PUCHAR) &(pCmdResult->Method),
                 sizeof(HostCmd_DS_802_11_FW_WAKE_METHOD)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_FW_WAKE_METHOD *)0)->Method)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_FW_WAKE_METHOD)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_FW_WAKE_METHOD *)0)->Method));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }

        return;

    case HostCmd_RET_802_11_SLEEP_PARAMS:
    {
         PHostCmd_DS_802_11_SLEEP_PARAMS pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_SLEEP_PARAMS) Adapter->CurCmd->BufVirtualAddr;

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf,
                 (PUCHAR) &(pCmdResult->Error),
                 sizeof(HostCmd_DS_802_11_SLEEP_PARAMS)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_SLEEP_PARAMS *)0)->Error)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_SLEEP_PARAMS)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_SLEEP_PARAMS *)0)->Error));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }

        return; 

    //++dralee_20060327
    case HostCmd_RET_802_11_SLEEP_PERIOD:
    {
         PHostCmd_DS_802_11_SLEEP_PERIOD pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_SLEEP_PERIOD) Adapter->CurCmd->BufVirtualAddr;

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf,
                 (PUCHAR) &(pCmdResult->period),
                 sizeof(HostCmd_DS_802_11_SLEEP_PERIOD)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_SLEEP_PERIOD *)0)->period)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_SLEEP_PERIOD)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_SLEEP_PERIOD *)0)->period));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }

        return;     

    //++dralee_20060509
    case HostCmd_RET_802_11_GET_LOG:    
    {
         PHostCmd_DS_802_11_GET_LOG pCmdResult;

         pCmdResult = (PHostCmd_DS_802_11_GET_LOG) Adapter->CurCmd->BufVirtualAddr;

         NdisMoveMemory(
                 (Adapter)->OidCmdRespBuf,
                 (PUCHAR) &(pCmdResult->Log),
                 sizeof(HostCmd_DS_802_11_GET_LOG)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_GET_LOG *)0)->Log)) );

         (Adapter)->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_GET_LOG)-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_GET_LOG *)0)->Log));
         (Adapter)->nOidCmdResult = pCmdResult->Result;

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
    }

        return; 
  case HostCmd_RET_802_11_RSSI:
        {
         p802dot11RSSI=(NDIS_802_11_RSSI *)Adapter->OidCmdRespBuf;      
            pRSSIRSP = (PHostCmd_DS_802_11_RSSI_RSP)pRetPtr;
            ///It has been handled in HandleRSSI()
#if 0
          /* store the non average value */
            Adapter->SNR[TYPE_BEACON][TYPE_NOAVG] = pRSSIRSP->SNR ;
            Adapter->NF[TYPE_BEACON][TYPE_NOAVG]  = pRSSIRSP->NoiseFloor ;
                
            Adapter->SNR[TYPE_BEACON][TYPE_AVG] = pRSSIRSP->AvgSNR;
            Adapter->NF[TYPE_BEACON][TYPE_AVG] = pRSSIRSP->AvgNoiseFloor ;
                
            Adapter->RSSI[TYPE_BEACON][TYPE_NOAVG] = 
              (SHORT)CAL_RSSI(Adapter->SNR[TYPE_BEACON][TYPE_NOAVG],
                  Adapter->NF[TYPE_BEACON][TYPE_NOAVG]);
          
            Adapter->RSSI[TYPE_BEACON][TYPE_AVG] = 
              (SHORT)CAL_RSSI(Adapter->SNR[TYPE_BEACON][TYPE_AVG],
                Adapter->NF[TYPE_BEACON][TYPE_AVG] );


        Adapter->ulRSSITickCount=GetTickCount();
        
          
              if ((Adapter->RSSI[TYPE_BEACON][TYPE_AVG] > -10) || (Adapter->RSSI[TYPE_BEACON][TYPE_AVG] < -200))
                  {
                      //Ling++, 011206
                      if(Adapter->MediaConnectStatus == NdisMediaStateConnected)
                        {
                                DBGPRINT(DBG_ERROR, (L"ERROR: Incorrect RSSI Value1 - SNR = %d, NF= %d, Adapter->RSSI[TYPE_BEACON][TYPE_AVG] = %d, Adapter->LastRSSI = %d\r\n", 
                                           pRSSIRSP->SNR, 
                                           pRSSIRSP->NoiseFloor,
                                           Adapter->RSSI[TYPE_BEACON][TYPE_AVG],
                                           Adapter->LastRSSI));
          
          
                        }
                 //Ling--, 011206       
                  }
                  else
                  {    
                       Adapter->LastRSSI = (LONG)Adapter->RSSI[TYPE_BEACON][TYPE_AVG];     
                  }
#endif ///
                 *p802dot11RSSI=(NDIS_802_11_RSSI)(Adapter->LastRSSI);

                 Adapter->nOidCmdResult = pRSSIRSP->Result;
                 Adapter->nSizeOfOidCmdResp = sizeof(NDIS_802_11_RSSI);
                  
                 DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
                 //SetEvent( (Adapter)->hOidQueryEvent ); 060407
                 Signal_Pending_OID(Adapter);
        DBGPRINT(DBG_HELP, (L"HostCmd_RET_802_11_RSSI: (%d, %d, %d, %d)\r\n",
                    Adapter->RSSI[0][0], 
                    Adapter->RSSI[0][1], 
                    Adapter->RSSI[1][0], 
                    Adapter->RSSI[1][1]));
         }
    return;
   

    case HostCmd_RET_802_11_TX_RATE_QUERY: 
        {
         PHostCmd_TX_RATE_QUERY pCmd; 
         POID_MRVL_DS_GET_TX_RATE pUserBuffer; 

         pTXRATE = (PHostCmd_TX_RATE_QUERY)pRetPtr;  

         if (pTXRATE) 
         {
         
           Adapter->TxRate = pTXRATE->TxRate;
         }

         pUserBuffer = (POID_MRVL_DS_GET_TX_RATE) Adapter->OidCmdRespBuf; 
         pCmd = (PHostCmd_TX_RATE_QUERY) pRetPtr; 
         pUserBuffer->usTxRate = (USHORT) pCmd->TxRate; 
         Adapter->nOidCmdResult = pCmd->Result; 
         Adapter->nSizeOfOidCmdResp = sizeof(OID_MRVL_DS_GET_TX_RATE); 
         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\r\n", pUserBuffer->usTxRate));

         DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
         //SetEvent( (Adapter)->hOidQueryEvent ); 060407
         Signal_Pending_OID(Adapter);
        }
        return;

        case HostCmd_RET_802_11_GET_TSF:
        {
            ///TT_CMDPARSE_STD_QUERY_BACKUP_SINGLE_MEMBER(
            PHostCmd_DS_802_11_GET_TSF  pCmd = (PHostCmd_DS_802_11_GET_TSF)pRetPtr;
            
            Adapter->nOidCmdResult = pCmd->Result;
            Adapter->nSizeOfOidCmdResp = sizeof(pCmd->TSF);
            NdisMoveMemory(Adapter->OidCmdRespBuf, &pCmd->TSF, sizeof(pCmd->TSF));
            //SetEvent( (Adapter)->hOidQueryEvent ); 060407
            Signal_Pending_OID(Adapter);
            //break; 051207 10:22
            return;    
        }
        case HostCmd_RET_802_11_WMM_ADDTS_REQ:
            //SetEvent( (Adapter)->hOidQueryEvent ); 060407
            Signal_Pending_OID(Adapter);
            return;
        case HostCmd_RET_802_11_WMM_DELTS_REQ:
            //SetEvent( (Adapter)->hOidQueryEvent ); 060407
            Signal_Pending_OID(Adapter);
            Adapter->bHasTspec = FALSE;
            return;
        case HostCmd_RET_802_11_WMM_QUEUE_CONFIG:
        {
            PHostCmd_DS_802_11_WMM_QUEUE_CONFIG pCmd = (PHostCmd_DS_802_11_WMM_QUEUE_CONFIG)pRetPtr;
            Adapter->nOidCmdResult = pCmd->Result;
            Adapter->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_WMM_QUEUE_CONFIG) - sizeof(HostCmd_DS_GEN);
            NdisMoveMemory(Adapter->OidCmdRespBuf, &pCmd->Action, Adapter->nSizeOfOidCmdResp);
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
            //SetEvent( (Adapter)->hOidQueryEvent ); 060407
            Signal_Pending_OID(Adapter);
        }   
            return;
        case HostCmd_RET_802_11_WMM_QUEUE_STATS:
        {
            PHostCmd_DS_802_11_WMM_QUEUE_STATS pCmd = (PHostCmd_DS_802_11_WMM_QUEUE_STATS)pRetPtr;

            Adapter->nOidCmdResult = pCmd->Result;
            Adapter->nSizeOfOidCmdResp = sizeof(HostCmd_DS_802_11_WMM_QUEUE_STATS) - sizeof(HostCmd_DS_GEN);
            NdisMoveMemory(Adapter->OidCmdRespBuf, &pCmd->Action, Adapter->nSizeOfOidCmdResp);
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\n", Ret));
            {
                int i =0; PUCHAR pos; int len;
                
                *((UCHAR *)(Adapter->OidCmdRespBuf +  Adapter->nSizeOfOidCmdResp)) = Adapter->RoamAccount;
                *((USHORT *)(Adapter->OidCmdRespBuf +  Adapter->nSizeOfOidCmdResp + sizeof(UCHAR))) = Adapter->RoamDelay;
                Adapter->nSizeOfOidCmdResp += sizeof(UCHAR)+sizeof(USHORT);
            
                pos = (PUCHAR)pRetPtr;
                len = sizeof(HostCmd_DS_802_11_WMM_QUEUE_STATS) + 8;
            
                DBGPRINT(DBG_CCX_V4,(L" HostCmd_RET_802_11_WMM_QUEUE_STATS \n"));
                DBGPRINT(DBG_CCX_V4,(L"return buffer length = %d \n",len));  
            
                for (i=0; i< len ;i++)
                {
                    DBGPRINT(DBG_CCX_V4,(L"%02x ",*(pos+i)));  
                    if (i%8 ==7 )
                    {
                        DBGPRINT(DBG_CCX_V4,(L"\n"));
                    }
                }
                DBGPRINT(DBG_CCX_V4,(L"\n"));
            }   
            Signal_Pending_OID(Adapter);
        }   
            return;

    case HostCmd_RET_MFG_COMMAND:
    {
        if (Adapter->UseMfgFw == 1)
        {                           
            NdisMoveMemory(Adapter->OidCmdRespBuf, 
                           Adapter->CurCmd->BufVirtualAddr, 
                           pRetPtr->Size);              
            Adapter->nSizeOfOidCmdResp = pRetPtr->Size;

            if(WaitForSingleObject( (Adapter)->hOidQueryEvent ,0 ) == WAIT_OBJECT_0)
            {
                DBGPRINT(DBG_ERROR, (L"ERROR: hOidQueryEvent already set, possible missed event\r\n"));
            }
                
            SetEvent(Adapter->hOidQueryEvent);
            return ;   
        }
    }
    default:
        return;
  }
 
  NdisMQueryInformationComplete(
    Adapter->MrvDrvAdapterHdl,
    NDIS_STATUS_SUCCESS);
  
}


/******************************************************************************
 *
 *  Name: HandleHostPendSetOid()
 *
 *  Description: Handle any Set OIDs
 *
 *  Arguments:  USHORT Ret
 *        PMRVDRV_ADAPTER Adapter
 *  
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID 
HandleHostPendSetOid(
  USHORT Ret, 
  PMRVDRV_ADAPTER Adapter
  )
{
    switch (Ret){

        case HostCmd_RET_802_11_BG_SCAN_CONFIG:
            {
        UINT    nSize;
        PHostCmd_DS_802_11_BG_SCAN_CONFIG   pCmdResult;
        pCmdResult = (PHostCmd_DS_802_11_BG_SCAN_CONFIG) Adapter->CurCmd->BufVirtualAddr;
        DBGPRINT(DBG_ROAM|DBG_HELP, (L"HandleHostPendSetOid: Getting HostCmd_RET_802_11_BG_SCAN_CONFIG\r\n"));
        nSize = pCmdResult->Size - FIELD_OFFSET( HostCmd_DS_802_11_BG_SCAN_CONFIG, Enable );
        if ( (sizeof(NDIS_OID)) > 0 )
            NdisZeroMemory( (Adapter)->OidCmdRespBuf, (sizeof(NDIS_OID)) );

        NdisMoveMemory(
                    (Adapter)->OidCmdRespBuf + (sizeof(NDIS_OID)),
                    (PUCHAR) &(pCmdResult->Enable),
                    pCmdResult->Size-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_BG_SCAN_CONFIG *)0)->Enable)) );

        (Adapter)->nSizeOfOidCmdResp = pCmdResult->Size-((LONG)(LONG_PTR)&(((HostCmd_DS_802_11_BG_SCAN_CONFIG *)0)->Enable));
        (Adapter)->nOidCmdResult = pCmdResult->Result;

        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
        //SetEvent( (Adapter)->hOidQueryEvent ); 060407
        Signal_Pending_OID(Adapter);

            }
        return;
    case HostCmd_RET_802_11_ASSOCIATE:
    case HostCmd_RET_802_11_REASSOCIATE:
         { 
             PHostCmd_DS_802_11_ASSOCIATE_RESULT pCmdResult; 

             pCmdResult = (PHostCmd_DS_802_11_ASSOCIATE_RESULT) Adapter->CurCmd->BufVirtualAddr; 

             //040307 We only extract status code. 
             *((PUSHORT)Adapter->OidCmdRespBuf)= pCmdResult->StatusCode;
             (Adapter)->nSizeOfOidCmdResp = sizeof(USHORT);
             (Adapter)->nOidCmdResult = pCmdResult->Result;

             DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
             //SetEvent( (Adapter)->hOidQueryEvent ); 060407

             Signal_Pending_OID(Adapter);
         }
         return;    
    case HostCmd_RET_802_11_PWK_KEY:
    case HostCmd_RET_TEST_RX_MODE:
    case HostCmd_RET_TEST_TX_MODE:
    case HostCmd_RET_802_11_KEY_MATERIAL:
    case HostCmd_RET_802_11_AD_HOC_STOP:
    case HostCmd_RET_802_11_DEAUTHENTICATE:  
    case HostCmd_RET_802_11_AUTHENTICATE:
    case HostCmd_RET_802_11_SET_WEP:
    case HostCmd_RET_802_11_SNMP_MIB:
    //case HostCmd_RET_802_11_ASSOCIATE:
    //case HostCmd_RET_802_11_REASSOCIATE:
    case HostCmd_RET_MAC_CONTROL:
    case HostCmd_RET_MAC_ADDRESS:
    case HostCmd_RET_802_11_RF_CHANNEL:
    case HostCmd_RET_EEPROM_ACCESS:
    case HostCmd_RET_RF_REG_ACCESS:
    case HostCmd_RET_MAC_REG_ACCESS:
    case HostCmd_RET_BBP_REG_ACCESS:
    case HostCmd_RET_802_11_RF_TX_POWER:
    case HostCmd_RET_802_11_RF_ANTENNA:
    case HostCmd_RET_802_11_DATA_RATE:
    case HostCmd_RET_MAC_MULTICAST_ADR:
    case HostCmd_RET_802_11_LED_CONTROL:
    case HostCmd_RET_802_11_CAL_DATA_EXT:
    case HostCmd_RET_802_11_PWR_CFG:
    case HostCmd_RET_802_11_TPC_CFG:
    case HostCmd_RET_802_11_RATE_ADAPT_RATESET:   
    case HostCmd_RET_802_11_POWER_ADAPT_CFG_EXT:
    //022607
      case HostCmd_RET_802_11_AD_HOC_START:
        case HostCmd_RET_802_11_AD_HOC_JOIN:
    case HostCmd_RET_802_11_BCA_CONFIG_TIMESHARE:
    { 
        PHostCmd_DS_GEN pCmdResult; 
         
        pCmdResult = (PHostCmd_DS_GEN) Adapter->CurCmd->BufVirtualAddr; 
        
        (Adapter)->nSizeOfOidCmdResp = 0; 
        (Adapter)->nOidCmdResult = pCmdResult->Result;

        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Set event (0x%x)...\r\n", Ret));
        //SetEvent( (Adapter)->hOidQueryEvent ); 060407
        Signal_Pending_OID(Adapter); 
    }

        return;

    default:
          
          
      NdisMSetInformationComplete(
        Adapter->MrvDrvAdapterHdl,
        NDIS_STATUS_SUCCESS);

      break;
  }// end of switch
}


/******************************************************************************
 *
 *  Name: HandleEnableQosWmeCommand
 *
 *  Description: Handle enable QOS WME command
 *
 *  Arguments:
 *          
 *
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID 
HandleEnableQosWmeCommand(
  PHostCmd_CMD_QOS_WME_ENABLE_STATE pCmd,
  PMRVDRV_ADAPTER Adapter
)
{

  if ( pCmd->Result == HostCmd_RESULT_NOT_SUPPORT )
  {
    // QOS is not supported in the FW
    DBGPRINT(DBG_QOS, (L"QOS is not supported in the FW!"));

    if ( Adapter->EnableQOS )
    {
      DBGPRINT(DBG_QOS, (L"DISABLE QOS support in the driver!\r\n"));
      Adapter->EnableQOS = 0;
    }
  }

}

/******************************************************************************
 *
 *  Name: HandleWmeACParamsCommand
 *
 *  Description: Handle WME AC params command
 *
 *  Arguments:
 *          
 *
 *  Return Value:        
 * 
 *  Notes:               
 *
 *****************************************************************************/
VOID
HandleWmeACParamsCommand(
  PHostCmd_CMD_QOS_WME_ACCESS_CATEGORY_PARAMETERS pCmd,
  PMRVDRV_ADAPTER Adapter
)
{
  if ( pCmd->Result != HostCmd_RESULT_OK )
  {
    return;
  }

  if ( pCmd->Action == HostCmd_ACT_GET )
  {
    // update the parameter in Adapter structure
    Adapter->QOSParams.AC_BE = pCmd->AC_BE;
    Adapter->QOSParams.AC_BE_XTRA = pCmd->AC_BE_XTRA;
    Adapter->QOSParams.AC_BK = pCmd->AC_BK;
    Adapter->QOSParams.AC_BK_XTRA = pCmd->AC_BK_XTRA;
    Adapter->QOSParams.AC_VI = pCmd->AC_VI;
    Adapter->QOSParams.AC_VI_XTRA = pCmd->AC_VI_XTRA;
    Adapter->QOSParams.AC_VO = pCmd->AC_VO;
    Adapter->QOSParams.AC_VO_XTRA = pCmd->AC_VO_XTRA;

    DBGPRINT(DBG_QOS, (L"QOS Params:\r\n"));
    DBGPRINT(DBG_QOS, 
      (L"AC_BE = 0x%x, XTRA = 0x%x, AC_BK = 0x%x, XTRA = 0x%x\r\n",
      Adapter->QOSParams.AC_BE, Adapter->QOSParams.AC_BE_XTRA,
      Adapter->QOSParams.AC_BK, Adapter->QOSParams.AC_BK_XTRA));
    DBGPRINT(DBG_QOS, 
      (L"AC_VI = 0x%x, XTRA = 0x%x, AC_VO = 0x%x, XTRA = 0x%x\r\n",
      Adapter->QOSParams.AC_VI, Adapter->QOSParams.AC_VI_XTRA,
      Adapter->QOSParams.AC_VO, Adapter->QOSParams.AC_VO_XTRA));
  }
}


/******************************************************************************
 *
 *  Name: InterpretBSSDescription()
 *
 *  Description: To parse BSSDescription in the MLME-SCAN.confirm(10.3.2.2 of 
 *               802.11 Spec and translate to NDIS_802_11_BSSID_LIST
 *
 *               The result buffer is using the new format with FW
 *               passing up the IE
 *
 *  Requirement: Adapter->ulNumOfBSSIDs stores the number of sets expected
 *  
 *  Arguments:  
 *      IN PMRVDRV_ADAPTER Adapter
 *      OUT PVOID pRetCommandBuffer
 *      IN ULONG ulBSSDescriptionListSize
 *    
 *  Return Value: 
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_FAILURE
 * 
 *  Notes: 
 *
 *****************************************************************************/
NDIS_STATUS
InterpretBSSDescription(
    IN PMRVDRV_ADAPTER Adapter,
    PVOID pRetCommandBuffer,
    ULONG ulBSSDescriptionListSize
    ,USHORT  nHostCmdRet
)
{
    PUCHAR pCurrentPtr, pNextBeacon;
    ULONG nBytesLeft;
    ULONG ulBSSIDIndex = 0;
    BOOLEAN bParsingError = FALSE, bIsParsingParamDone = FALSE;

    PNDIS_WLAN_BSSID_EX             pBSSIDList;
    IEEEtypes_ElementId_e           nElemID;
    IEEEtypes_FhParamSet_t          *pFH;
    IEEEtypes_DsParamSet_t          *pDS;
    IEEEtypes_CfParamSet_t          *pCF;
    IEEEtypes_IbssParamSet_t        *pIbss;
    IEEEtypes_CapInfo_t             *pCap;
    IEEEtypes_CountryInfoSet_t      *pcountryinfo;
    PNDIS_802_11_FIXED_IEs          pFixedIE;
    PNDIS_802_11_VARIABLE_IEs       pVariableIE;
    PBSS_DESCRIPTION_SET_ALL_FIELDS pBssDescList;

    USHORT                          usBeaconSize;
    USHORT                          usBytesLeftForCurrentBeacon;
    UCHAR                           ucElemLen, ucRateSize;
    BOOLEAN                         bFoundDataRateIE;

    PHostCmd_DS_802_11_SCAN_RSP     pScanResponse;
    UCHAR                           ucCurIDXOnOriginalScanResponse = 0;
    PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP pBgScanResponse;


    DBGPRINT(DBG_SCAN,(L"+InterpretBSSDescriptionWithIE\r\n"));

    if ( nHostCmdRet == HostCmd_RET_802_11_SCAN )
    {
        pScanResponse = (PHostCmd_DS_802_11_SCAN_RSP) pRetCommandBuffer;
        pNextBeacon = pCurrentPtr = (PUCHAR)(pScanResponse+1);
    }
    else if ( nHostCmdRet == HostCmd_RET_802_11_BG_SCAN_QUERY )
    {
        pBgScanResponse = (PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP) pRetCommandBuffer;
        pNextBeacon = pCurrentPtr = (PUCHAR)(pBgScanResponse+1);   
    }
    else
    {
        DBGPRINT( DBG_SCAN|DBG_ERROR, (L"[Mrvl] ERROR: Invalid scan response code [code: 0x%x]\r\n", nHostCmdRet) );
        return NDIS_STATUS_FAILURE;
    }

   // HexDump(DBG_CUSTOM, "SCAN RESPONSE", (PUCHAR)pScanResponse, ulBSSDescriptionListSize);
    
    // Clean up the current BSSID List
    NdisZeroMemory( Adapter->BSSIDList, 
                sizeof(NDIS_WLAN_BSSID_EX) * MRVDRV_MAX_BSSID_LIST);

    // Clean up the current BSS Descriptor List
    NdisZeroMemory( Adapter->BssDescList, 
                sizeof(BSS_DESCRIPTION_SET_ALL_FIELDS)*MRVDRV_MAX_BSSID_LIST);

    // At the beginning, nBytesLeft is the total BSSDescription List
    nBytesLeft = (LONG) ulBSSDescriptionListSize;

    // expected format:
    //
    // Length - 2 bytes
    // BSSID  - 6 bytes
    // IE     - variable length

    // Adapter->ulNumOfBSSIDs already store the number of BSSID
    ulBSSIDIndex=0;
    while ( nBytesLeft > 0 && ulBSSIDIndex < Adapter->ulNumOfBSSIDs )
    {
        BOOLEAN bParseOkay;

        pCurrentPtr = pNextBeacon;
        DBGPRINT(DBG_SCAN, (L"pCurrentPtr = 0x%x\r\n", pCurrentPtr));

        NdisMoveMemory(&usBeaconSize, pCurrentPtr, sizeof(USHORT));
        usBytesLeftForCurrentBeacon = usBeaconSize;


        if ( usBeaconSize > nBytesLeft )
        {
            // not enough to cover fixed field, beacon is broken
            DBGPRINT( DBG_SCAN | DBG_WARNING, 
                     (L"InterpretBSSDescription(): beacon size > bytes left\r\n"));

            // reset the number of valid BSSID
            Adapter->ulNumOfBSSIDs = ulBSSIDIndex;

            return NDIS_STATUS_FAILURE;
        }


        pCurrentPtr += sizeof(usBeaconSize);

        pNextBeacon = pCurrentPtr + usBeaconSize;

        pBSSIDList = &(Adapter->BSSIDList[ulBSSIDIndex]);
        pFixedIE = &((Adapter->IEBuffer[ulBSSIDIndex]).FixedIE);
        pVariableIE = (PNDIS_802_11_VARIABLE_IEs)
                      (Adapter->IEBuffer[ulBSSIDIndex]).VariableIE;
        NdisZeroMemory(pVariableIE, MRVDRV_SCAN_LIST_VAR_IE_SPACE);
        pBssDescList = &(Adapter->BssDescList[ulBSSIDIndex]);

        // initial the default value
        pBssDescList->ccx_bss_info.ccxEnabled = 0;
	///CCX_CCKM
        pBssDescList->ccx_bss_info.cckmEnabled = 0;
	//CCX_CCKM

        // copy BSSID
        NdisMoveMemory( pBSSIDList->MacAddress,
                        pCurrentPtr,
                        MRVDRV_ETH_ADDR_LEN);

        NdisMoveMemory( pBssDescList->BSSID,
                        pCurrentPtr,
                        MRVDRV_ETH_ADDR_LEN);

        pCurrentPtr += MRVDRV_ETH_ADDR_LEN;
        usBytesLeftForCurrentBeacon -= MRVDRV_ETH_ADDR_LEN;

        ucCurIDXOnOriginalScanResponse++;

        pBSSIDList->Configuration.Length = sizeof(NDIS_802_11_CONFIGURATION);

        if ( usBytesLeftForCurrentBeacon < 12 )
        {
            // not enough to cover fixed field, beacon is broken
            DBGPRINT( DBG_SCAN | DBG_WARNING, 
                     (L"InterpretBSSDescription(): not enough bytes left\r\n"));

            // reset the number of valid BSSID
            Adapter->ulNumOfBSSIDs = ulBSSIDIndex;

            return NDIS_STATUS_FAILURE;
        }

        // reset of the current buffer are IE's
        pBSSIDList->IELength = usBytesLeftForCurrentBeacon;
        DBGPRINT(DBG_SCAN | DBG_WPA,
                 (L"IELength including fixed and variable IE's for this AP = %d\r\n",
                  pBSSIDList->IELength));

        // RSSI of beacon 
        pBSSIDList->Rssi = -(LONG)((*pCurrentPtr) & 0x7f);
        pCurrentPtr +=1;
        usBytesLeftForCurrentBeacon -= 1;

        DBGPRINT(DBG_SCAN, (L"RSSI value = %d\r\n", pBSSIDList->Rssi));

        // next 3 fields are time stamp, beacon interval, and capability information
        // time stamp is 8 byte long
        NdisMoveMemory(pFixedIE->Timestamp, pCurrentPtr, 8); 
        NdisMoveMemory(pBssDescList->TimeStamp, pCurrentPtr, 8);
        pCurrentPtr += 8;
        usBytesLeftForCurrentBeacon -= 8;

        // beacon interval is 2 byte long
        NdisMoveMemory(&(pFixedIE->BeaconInterval), pCurrentPtr, 2);
        pBSSIDList->Configuration.Length = sizeof(NDIS_802_11_CONFIGURATION);
        pBSSIDList->Configuration.BeaconPeriod = 
            pBssDescList->BeaconPeriod = pFixedIE->BeaconInterval;
        pCurrentPtr += 2;
        usBytesLeftForCurrentBeacon -= 2;

        DBGPRINT(DBG_SCAN, (L"Beacon Interval = %d\r\n", pFixedIE->BeaconInterval));

        // capability information is 2 byte long
        NdisMoveMemory(&(pFixedIE->Capabilities), pCurrentPtr, 2);
        pCap = (IEEEtypes_CapInfo_t *)&pFixedIE->Capabilities;

        if ( pCap->Privacy )
        {   
            DBGPRINT(DBG_WEP,(L"AP Capabilties have WEP SET. Setting the bit\r\n"));
            pBSSIDList->Privacy = Ndis802_11PrivFilter8021xWEP;
        }
        else
        {
            pBSSIDList->Privacy = Ndis802_11PrivFilterAcceptAll;
        }

        if ( pCap->Ibss == 1 )
        {
            // IBSS network
            pBSSIDList->InfrastructureMode = Ndis802_11IBSS;
            pBssDescList->BSSType = HostCmd_BSS_TYPE_IBSS;   
        }
        else
        {
            // Infrastructure network
            pBSSIDList->InfrastructureMode = Ndis802_11Infrastructure;
            pBssDescList->BSSType = HostCmd_BSS_TYPE_BSS;
        }

        // Copy IEEEtypes_CapInfo_t info to pBssDescList
        NdisMoveMemory(&pBssDescList->Cap,
                       pCap, 
                       sizeof(IEEEtypes_CapInfo_t));

        pCurrentPtr += 2;
        usBytesLeftForCurrentBeacon -= 2;

        if ( usBytesLeftForCurrentBeacon > MRVDRV_SCAN_LIST_VAR_IE_SPACE )
        {
            // not enough space to copy the IE for the current AP
            // just skip it for now
            pCurrentPtr += usBytesLeftForCurrentBeacon;
            nBytesLeft -= usBeaconSize;
            Adapter->ulNumOfBSSIDs--;

            DBGPRINT(DBG_SCAN | DBG_WARNING, 
                     (L"Variable IE size too big to fit into buffer, skip!\r\n"));
            continue;
        }

        // Filter unregular IE
        RegularIeFilter(pCurrentPtr, &usBytesLeftForCurrentBeacon);

        // Rest of buffer is variable IE
        // Copy the entire variable IE to IE buffer;
        NdisMoveMemory(pVariableIE,
                       pCurrentPtr,
                       usBytesLeftForCurrentBeacon);

        //HexDump(DBG_SCAN, "Variable IE Buffer: ", pCurrentPtr, usBytesLeftForCurrentBeacon);  
        bFoundDataRateIE = FALSE;
        bParseOkay = TRUE;
        
        pBSSIDList->Reserved[1]= Adapter->CurCmd->Pad[2] & 0x03;  // band
        DBGPRINT(DBG_CMD,(L"Set Band to %d\r\n", (Adapter->CurCmd->Pad[2] & 0x03))); 
    
        pBSSIDList->Reserved[0] = Adapter->CurCmd->PadExt[0];     // channel
        DBGPRINT(DBG_CMD,(L"Set Channel to %d\r\n", pBSSIDList->Reserved[0])); 

        // Process variable IE
        while (usBytesLeftForCurrentBeacon > 2)
        {
            nElemID = (IEEEtypes_ElementId_e) (*((PUCHAR)pCurrentPtr));
            ucElemLen = *((PUCHAR)pCurrentPtr+1);


            if (usBytesLeftForCurrentBeacon < (ucElemLen+2))
            {
                nBytesLeft -= usBeaconSize;
                Adapter->ulNumOfBSSIDs--;

                DBGPRINT(DBG_SCAN | DBG_WARNING, 
                        (L"Error in processing IE, bytes left(%d) < IE length + 2(%d)!\r\n",
                        usBytesLeftForCurrentBeacon, ucElemLen + 2));

                bParseOkay = FALSE;

                /*HexDump(DBG_WARNING | DBG_SCAN, "Error IE Dump: ", 
                        (PUCHAR)pVariableIE, pBSSIDList->IELength - MRVL_FIXED_IE_SIZE);*/

                // Quit the variable IE process loop
                usBytesLeftForCurrentBeacon = 0;
                continue;
            }

            switch (nElemID)
            {
                case SSID:
                {
                    // 36.p1 +++
                    if ( ucElemLen > 0 )
                    {  
                        NdisZeroMemory(pBssDescList->SSID, 
                                       MRVDRV_MAX_SSID_LENGTH);

                        // copy into fixed description set
                        NdisMoveMemory(pBssDescList->SSID,
                                       (pCurrentPtr+2),
                                       ucElemLen);

                        DBGPRINT(DBG_SCAN|DBG_WPA|DBG_HELP, (L"BSSID = %02x-%02x-%02x-%02x-%02x-%02x\r\n",
                                    pBssDescList->BSSID[0],
                                    pBssDescList->BSSID[1],
                                    pBssDescList->BSSID[2],
                                    pBssDescList->BSSID[3],
                                    pBssDescList->BSSID[4],
                                    pBssDescList->BSSID[5]));

                        pBSSIDList->Ssid.SsidLength = ucElemLen;
                        NdisMoveMemory(pBSSIDList->Ssid.Ssid,
                                       (pCurrentPtr+2),
                                       ucElemLen);                
                    }
                    else
                    {
                        // This may be a failure
                        nBytesLeft -= usBeaconSize;
                        Adapter->ulNumOfBSSIDs--;

                        bParseOkay = FALSE;

                        // Quit the variable IE process loop
                        usBytesLeftForCurrentBeacon = 0;
                        DBGPRINT(DBG_SCAN|DBG_ERROR, 
                                 (L"Strange IE, ssid length = 0\r\n"));
                        continue;
                    }
                    break;
                }

                case SUPPORTED_RATES:
                {
                    NdisMoveMemory(pBssDescList->DataRates,
                                   (pCurrentPtr+2),
                                   ucElemLen);
                    NdisMoveMemory(pBSSIDList->SupportedRates,
                                   (pCurrentPtr+2),
                                   ucElemLen);

                    ucRateSize = ucElemLen;
                    bFoundDataRateIE = TRUE;

                    break;
                }

                case FH_PARAM_SET:
                {
                    DBGPRINT(DBG_SCAN, (L"FH_PARAM_SET\r\n"));
                             pFH = (IEEEtypes_FhParamSet_t *)pCurrentPtr;

                    pBSSIDList->NetworkTypeInUse = Ndis802_11FH;
                    pBSSIDList->Configuration.DSConfig = 0;
                    pBSSIDList->Configuration.FHConfig.Length = sizeof(NDIS_802_11_CONFIGURATION_FH);
                    pBSSIDList->Configuration.FHConfig.HopPattern = (ULONG)pFH->HopPattern;
                    pBSSIDList->Configuration.FHConfig.HopSet = (ULONG)pFH->HopSet;
                    pBSSIDList->Configuration.FHConfig.DwellTime = pFH->DwellTime;
          
                    // Copy IEEEtypes_FhParamSet_t info to pBssDescList
                    NdisMoveMemory(&(pBssDescList->PhyParamSet.FhParamSet),
                                   pFH, 
                                   sizeof(IEEEtypes_FhParamSet_t));

                    break;
                }
 
                case DS_PARAM_SET:
                {
                    DBGPRINT(DBG_SCAN, (L"DS_PARAM_SET\r\n"));
                    pDS = (IEEEtypes_DsParamSet_t *) pCurrentPtr;
                    DBGPRINT(DBG_SCAN, (L"pDS->CurrentChan = %d\r\n", pDS->CurrentChan));
        
                    switch (pBSSIDList->Reserved[1])
                    {
                        case MRVDRV_802_11_BAND_B:
                            pBSSIDList->NetworkTypeInUse = Ndis802_11DS;
                            pBSSIDList->Configuration.DSConfig =
                                    2407 + (pDS->CurrentChan * 5);
                            break;
          
                        case MRVDRV_802_11_BAND_BG:
                            pBSSIDList->NetworkTypeInUse = Ndis802_11OFDM24;
                            pBSSIDList->Configuration.DSConfig =
                                    2407 + (pDS->CurrentChan * 5);  
                            break;
            
                        case MRVDRV_802_11_BAND_A:
                            pBSSIDList->NetworkTypeInUse = Ndis802_11OFDM5;
                            pBSSIDList->Configuration.DSConfig =
                                    5000 + (pDS->CurrentChan * 5);  
                            break;
            
                        default:
                            pBSSIDList->NetworkTypeInUse = Ndis802_11OFDM24;
                            pBSSIDList->Configuration.DSConfig =
                                    2407 + (pDS->CurrentChan * 5);  
                            break;
                    }
        
                    // Copy IEEEtypes_DsParamSet_t info to pBssDescList
                    NdisMoveMemory(&(pBssDescList->PhyParamSet.DsParamSet),
                                   pDS, 
                                   sizeof(IEEEtypes_DsParamSet_t));
                    break;
                }

                case CF_PARAM_SET:
                {
                    pCF = (IEEEtypes_CfParamSet_t *)pCurrentPtr;

                    // Copy IEEEtypes_CfParamSet_t info to pBssDescList
                    NdisMoveMemory(&(pBssDescList->SsParamSet.CfParamSet),
                                   pCF, 
                                   sizeof(IEEEtypes_CfParamSet_t));

                    break;
                }

                case IBSS_PARAM_SET:
                {
                    pIbss = (IEEEtypes_IbssParamSet_t *)pCurrentPtr;

                    pBSSIDList->Configuration.ATIMWindow = pIbss->AtimWindow;

                    // Copy IEEEtypes_IbssParamSet_t info to pBssDescList
                    NdisMoveMemory(&(pBssDescList->SsParamSet.IbssParamSet),
                                   pIbss, 
                                   sizeof(IEEEtypes_IbssParamSet_t));

                    break;
                }

                case COUNTRY_INFO: /* Handle Country Info IE */
                {
                    pcountryinfo = (IEEEtypes_CountryInfoSet_t *)pCurrentPtr;
                 
                    NdisMoveMemory(&(pBssDescList->CountryInfo),  
                                   pcountryinfo, 
                                   pcountryinfo->Len + 2);

                    pBssDescList->bHaveCountryIE = 1;
                    DBGPRINT(DBG_SCAN, (L"COUNTRY_INFO\r\n"));
                    break;
                }

                case ERP_INFO:
                {
                    IEEEtypes_ERPInfo_t* pERPInfo;

                    pERPInfo = (IEEEtypes_ERPInfo_t *)pCurrentPtr; 
                    pBssDescList->ERPFlags = pERPInfo->ERPFlags;

                    break;
                }

                case EXTENDED_SUPPORTED_RATES:
                {
                    // only process extended supported rate
                    // if data rate is already found.
                    // data rate IE should come before
                    // extended supported rate IE
                    if ( bFoundDataRateIE )
                    {
                        PUCHAR  pRate;
                        UCHAR   ucBytesToCopy;
                    
                        if ( (ucElemLen + ucRateSize) > NDIS_SUPPORTED_RATES )
                            ucBytesToCopy = ( NDIS_SUPPORTED_RATES - ucRateSize);
                        else
                            ucBytesToCopy = ucElemLen;

                        pRate = (PUCHAR)pBssDescList->DataRates;
                        pRate += ucRateSize; 
                        NdisMoveMemory(pRate, (pCurrentPtr+2), ucBytesToCopy);

                        pRate = (PUCHAR)pBSSIDList->SupportedRates;
                        pRate += ucRateSize; 
                        NdisMoveMemory(pRate, (pCurrentPtr+2), ucBytesToCopy);
                    }
                    break;
                }

                case ELE_CISCO_CLIENT_TX_PWR:
                {
                    wlan_ccx_process_bss_elem(&(pBssDescList->ccx_bss_info),
                                              pCurrentPtr);
                    break;
                } // case ELE_CISCO_CLIENT_TX_PWR


                case WPA_IE: // WMM_IE is also 221
                {
                    UCHAR   len;
                    UCHAR   WPA_OUI[] = { 0x00, 0x50, 0xF2, 0x01};
                    typedef struct tagWPA_RSN_IE
                    {
                        UCHAR  wriIE;                  /* info element ID, 0xdd(221) */
                        UCHAR  wriLen;                 /* numer of bytes following */
                        UCHAR  wriOUI[4];              /* always 00 50 f2 01 */
                        UCHAR  wriVersion[2];          /* 01 00*/
                        UCHAR  wriGrpKeyCipherOUI[3];  /* 00 50 f2 */
                        UCHAR  wriGrpKeyCipher;        /* see CipherSuiteSelectors */
                        UCHAR  wriPwKeyCipherCnt[2];   /* 00 01 - count always 1 */
                        UCHAR  wriPwKeyCipherOUI[3];   /* 00 50 F2 */
                        UCHAR  wriPwKeyCipher;         /* see CipherSuiteSelectors */
                        UCHAR  wriAuthKeySuiteCnt[2];  /* 00 01 - count of authentications keys */
                        UCHAR  wriAuthKeysuiteOUI[3];  /* 00 50 F2 for WPA */
                                                       /* 00 40 96 for CCKM */
                        UCHAR  wriAuthKeysuite;        /* 00 50 F2 for WPA */
                        UCHAR  wriRSNCaps[2];          /* optional capabilities field */                                            
                    } WPA_RSN_IE,*PWPA_RSN_IE;

                    // CCX : 0x03, CCX_QOS : 0x04
                    UCHAR   CCX_OUI[] = { 0x00, 0x40, 0x96};
                    UCHAR   CCX_CCKM_OUI[] = { 0x00, 0x50, 0xF2, 0x01, 0x01, 0x00};
                    UCHAR   CCX_CCKM_1[] = { 0x01,0x00, 0x00, 0x40, 0x96, 0x00 };

                    WPA_RSN_IE *pRSN = (WPA_RSN_IE*)pCurrentPtr;
                    UCHAR   WMM_OUI[4] = { 0x00, 0x50, 0xf2, 0x02 };
                    UCHAR   WPS_OUI[4] = { 0x00, 0x50, 0xf2, 0x04 };
                    if (NdisEqualMemory((pCurrentPtr+2), WPA_OUI, 4) == 1)
                    {
                        len = *(pCurrentPtr+1) + 2;
                        NdisMoveMemory(pBssDescList->wpa_supplicant.Wpa_ie,
                                       pCurrentPtr,
                                       len);
                        pBssDescList->wpa_supplicant.Wpa_ie_len = len;
                        if ( pRSN->wriLen>=0x16 ) 
                        {
                            if (NdisEqualMemory((pCurrentPtr+2),
                                                CCX_CCKM_OUI,
                                                sizeof(CCX_CCKM_OUI)) == 1)
                            {
                                if (NdisEqualMemory(pRSN->wriAuthKeySuiteCnt,
                                                    CCX_CCKM_1,
                                                    sizeof(CCX_CCKM_1)) == 1)
                                {
                                    pBssDescList->ccx_bss_info.ccxEnabled = 1;
                                    pBssDescList->ccx_bss_info.cckmEnabled = 1;
                                }
                            }
                        }   
                    }
                    else if(NdisEqualMemory((pCurrentPtr+2), WMM_OUI, 4) == 1)
                    {
                        if (pCurrentPtr[6] == 1)
                        {
                            len = *(pCurrentPtr+1) + 2;

                            if (len > sizeof(pBssDescList->Wmm_IE))
                            {
                                len = sizeof(pBssDescList->Wmm_IE);
                            }

                            NdisMoveMemory(pBssDescList->Wmm_IE, pCurrentPtr, len);
                            pBssDescList->Wmm_ie_len = len;
                        }
                        else
                        {
                            //Dont' need to save WMM IE. Driver uses a fixed one with a user specified QosInfo when setup association request.
                           DbgWmmMsg((L"+wmm+ WMM IE (OUI subtype=0x%x): len=%d (%x %x %x...)\r\n", pCurrentPtr[6],
                                      len, pCurrentPtr[6], pCurrentPtr[7], pCurrentPtr[8] ) );
                        }
                    }
                    else if (NdisEqualMemory((pCurrentPtr+2), CCX_OUI, 3) == 1)
                    {
                        wlan_ccx_process_bss_elem(&(pBssDescList->ccx_bss_info),
                                                  pCurrentPtr);
                    }
                    else if (NdisEqualMemory((pCurrentPtr+2), WPS_OUI, 4) == 1)
                    {
                        // WSC IE_ID is also 221
                        len = *(pCurrentPtr+1) + 2;
                        NdisMoveMemory (pBssDescList->wps.Wps_ie, pCurrentPtr, len);
                        pBssDescList->wps.Wps_ie_len = len;
                    }
                    break;
                }

                case WPA2_IE:
                {
                    UCHAR   len;
                    len = *(pCurrentPtr+1) + 2;
                    NdisMoveMemory(pBssDescList->wpa2_supplicant.Wpa_ie, pCurrentPtr, len);
                    pBssDescList->wpa2_supplicant.Wpa_ie_len = len; 
                    break;
                }
            }

            pCurrentPtr += ucElemLen + 2;
            // need to account for IE ID and IE Len
            usBytesLeftForCurrentBeacon = usBytesLeftForCurrentBeacon - ucElemLen - 2;
        } // while ( usBytesLeftForCurrrentBeacon > 2 ) - process variable IE

        if ( !bParseOkay )
        {
            continue;
        }

        // Update the length field
        pBSSIDList->Length = sizeof(NDIS_WLAN_BSSID_EX) + pBSSIDList->IELength;

        // Need to be 4 byte aligned
        pBSSIDList->Length = ((pBSSIDList->Length + 3) >> 2) << 2;
        
        nBytesLeft -= usBeaconSize;
        ulBSSIDIndex++;

    } // while ( nBytesLeft > 0 && ulBSSIDIndex < MRVDRV_MAX_BSSID_LIST )

    DBGPRINT(DBG_SCAN ,(L"-InterpretBSSDescription: normal exit\r\n"));

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS Deactivate_Host_Sleep_Cfg(PMRVDRV_ADAPTER Adapter, USHORT prioritypass)
{
   OID_MRVL_DS_HOST_SLEEP_CFG HW;
   NDIS_STATUS Status;

  
   HW.ulCriteria = -1;
   HW.ucGPIO=0xff;
   HW.ucGap=0x10; 
   //dralee-20060606
   HW.Filter.Header.Type = 0;
   HW.Filter.Header.Len = 0;

   Status = PrepareAndSendCommand(
              Adapter,        
              HostCmd_CMD_802_11_HOST_SLEEP_CFG,
              HostCmd_ACT_GEN_SET,
              HostCmd_OPTION_USE_INT,
              (NDIS_OID)0,        
              HostCmd_PENDING_ON_NONE,
              prioritypass,
              FALSE,
              NULL,
              NULL,
              0,
              (PVOID)&HW); 

     if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
     {
         //RETAILMSG(1, (TEXT("[Marvell]Set OID_MRVL_HOST_SLEEP_CFG Fail\n")));
         return Status;
     }

     return NDIS_STATUS_SUCCESS;

}

//dralee_20060515
NDIS_STATUS Reactivate_Host_Sleep_Cfg(PMRVDRV_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    Status = PrepareAndSendCommand(
              Adapter,        
              HostCmd_CMD_802_11_HOST_SLEEP_CFG,
              HostCmd_ACT_GEN_SET,
              HostCmd_OPTION_USE_INT,
              (NDIS_OID)0,        
              HostCmd_PENDING_ON_NONE,
              0,
              FALSE,
              NULL,
              NULL,
              0,
              //(PVOID)&HW);  //dralee_20060606
              (PVOID)&Adapter->SleepCfg); 

     if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
     {
         //RETAILMSG(1, (TEXT("[Marvell]Set OID_MRVL_HOST_SLEEP_CFG Fail\n")));
         return Status;
     }
      
     //RETAILMSG(1,(L"REACTIVE SLP CFG:%x,%x,%x\r\n",HW.ulCriteria,HW.ucGPIO,HW.ucGap)); 

     return NDIS_STATUS_SUCCESS;

}


//060407
// the function remove the entry(s) which have the same ssid of active SSID scan 
VOID 
RemoveTheActiveSsidEntry( IN PMRVDRV_ADAPTER Adapter,
                          PNDIS_802_11_SSID ScannedSSID )
{
    ULONG PSnum;
    int idx; 

     
    if( !(ScannedSSID->SsidLength <=32 && ScannedSSID->SsidLength > 0) )
    {   
        DBGPRINT(DBG_SCAN|DBG_HELP,(L"Remove 0 APs because of Null Adapter->ActiveScanSSID\r\n")); 
        return;
    } 

    DBGPRINT(DBG_SCAN|DBG_HELP,(L"Adapter->ActiveScanSSID:%x-%x-%x...Len:%d\r\n",
                                                            ScannedSSID->Ssid[0],
                                                            ScannedSSID->Ssid[1],
                                                            ScannedSSID->Ssid[2],
                                                            ScannedSSID->SsidLength));

    PSnum = Adapter->ulPSNumOfBSSIDs;

    for(idx=PSnum-1; idx>=0; idx--)
    { 

        if ( NdisEqualMemory(Adapter->PSBSSIDList[idx].Ssid.Ssid,
                             ScannedSSID->Ssid,
                             ScannedSSID->SsidLength)  )
    {       
           if( idx == (PSnum-1) )//the last one 
              {
               DBGPRINT(DBG_SCAN|DBG_HELP,(L"Remove the last entry from PS table\r\n")); 
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
        }
    }

    DBGPRINT(DBG_SCAN|DBG_HELP,(L"Totaly %d entries are removed, %d are left\r\n",(PSnum-Adapter->ulPSNumOfBSSIDs), PSnum)); 

    Adapter->ulPSNumOfBSSIDs = PSnum;

    return;
}

//051707 : 060407
// the function add a tag to the entry that will be age out
///type:
///     0: Clean out all APs in the table, whose SSID is identical with the SSID we are going to scan
///     1: Clean out all APs
///     2: Clean out all APs in the table, whose channel is identical with the channel we are 
///
static void AgeOutSsidEntry(IN PMRVDRV_ADAPTER Adapter,
                          PNDIS_802_11_SSID ScannedSSID)
{
    ULONG PSnum = Adapter->ulPSNumOfBSSIDs;
    int idx;
    TCHAR   wssid[66];

    if( !(ScannedSSID->SsidLength <=32 && ScannedSSID->SsidLength > 0 )) {   
        DBGPRINT(DBG_SCAN|DBG_HELP,(L"Remove 0 APs because of Null Adapter->ActiveScanSSID\n\r")); 
        goto FuncFinal;
    } else {
        NdisZeroMemory(wssid, sizeof(wssid));
        for (idx=0 ; idx<(int)ScannedSSID->SsidLength ; idx++) {
            wssid[idx] = (TCHAR)ScannedSSID->Ssid[idx];
        }
        DBGPRINT(DBG_SCAN|DBG_HELP,(L"Age out entries whose SSID: %s\n\r", wssid)); 
    }
    for(idx=PSnum-1; idx>=0; idx--) { 
        if ( NdisEqualMemory(Adapter->PSBSSIDList[idx].Ssid.Ssid,
                             ScannedSSID->Ssid,
                             ScannedSSID->SsidLength)  )   
        {
             Adapter->PSBssEntryAge[idx] = 1;
             DBGPRINT(DBG_SCAN|DBG_HELP,(L"One entry[%d] is tag to age out\n\r", idx)); 
        } 
    }
FuncFinal:
    return;
}

static void AgeOutChannelEntry(IN PMRVDRV_ADAPTER Adapter)
{
    ULONG PSnum = Adapter->ulPSNumOfBSSIDs;
    int idx;
    USHORT SpecifyChannelFrequency;

    DBGPRINT(DBG_SCAN|DBG_HELP,(L"Age out entries whose channel: %d\n\r", Adapter->SpecifyChannel)); 

    if(Adapter->SpecifyChannel>11) {
        SpecifyChannelFrequency=5000+(Adapter->SpecifyChannel*5);
    } else {
        SpecifyChannelFrequency=2407+(Adapter->SpecifyChannel*5);	
    }	

    for(idx=PSnum-1; idx>=0; idx--) { 
        DBGPRINT(DBG_OID_OPT,(L"*****AgeOutChannelEntry Adapter->SpecifyChannel=0x%x,SpecifyChannelFrequency=%d,Adapter->PSBSSIDList[idx].Configuration.DSConfig=%d\n\r",Adapter->SpecifyChannel,SpecifyChannelFrequency,Adapter->PSBSSIDList[idx].Configuration.DSConfig));   
        if(Adapter->PSBSSIDList[idx].Configuration.DSConfig==SpecifyChannelFrequency) {
            DBGPRINT(DBG_ERROR,(L"*****AgeOutChannelEntry One entry is tag to age out: idx=%d,Adapter->PSBSSIDList[%d].MacAddress[0]=0x%x,Adapter->PSBSSIDList[%d].MacAddress[5]=0x%x\n\r",idx,idx,Adapter->PSBSSIDList[idx].MacAddress[0],idx,Adapter->PSBSSIDList[idx].MacAddress[5])); 
            Adapter->PSBssEntryAge[idx] = 1;
        }
    }

    return;
}
VOID 
AgeOutTheActiveSsidEntry( IN PMRVDRV_ADAPTER Adapter,
                          PNDIS_802_11_SSID ScannedSSID, AGEOUTTYPE type)
{
    ULONG PSnum = Adapter->ulPSNumOfBSSIDs;
    int idx;

    switch (type) {
        case AGEOUT_SSID:{
            AgeOutSsidEntry(Adapter, ScannedSSID);
        }
        break;
        case AGEOUT_ALL: {
            DBGPRINT(DBG_SCAN|DBG_HELP,(L"Age out all entries\n\r")); 
            for(idx=PSnum-1; idx>=0; idx--) {
                Adapter->PSBssEntryAge[idx] = 1;
                DBGPRINT(DBG_SCAN|DBG_HELP,(L"One entry[%d] is tag to age out\n\r", idx));
            }
        }
        break;
        case AGEOUT_CHNL:
            AgeOutChannelEntry(Adapter);
            break;
        default:
            break;
    }
/*
    if( type == 1 )
    {             
        DBGPRINT(DBG_SCAN|DBG_HELP,(L"Age out all entries\n\r")); 
    }
    else if( !(ScannedSSID->SsidLength <=32 && ScannedSSID->SsidLength > 0 ))
    {   
        DBGPRINT(DBG_SCAN|DBG_HELP,(L"Remove 0 APs because of Null Adapter->ActiveScanSSID\n\r")); 
        return;
    } 

    DBGPRINT(DBG_SCAN|DBG_HELP,(L"Adapter->ActiveScanSSID:%x-%x-%x...Len:%d\n\r",
                                                            ScannedSSID->Ssid[0],
                                                            ScannedSSID->Ssid[1],
                                                            ScannedSSID->Ssid[2],
                                                            ScannedSSID->SsidLength));

    PSnum = Adapter->ulPSNumOfBSSIDs;
 
    for(idx=PSnum-1; idx>=0; idx--)
    { 
        if( type == 1 ) 
        {
           Adapter->PSBssEntryAge[idx] = 1;
           DBGPRINT(DBG_SCAN|DBG_HELP,(L"One entry is tag to age out\n\r")); 
        }
        else if ( NdisEqualMemory(Adapter->PSBSSIDList[idx].Ssid.Ssid,
                             ScannedSSID->Ssid,
                             ScannedSSID->SsidLength)  )   
        {
#ifdef OPTIMIZER_OID
              if(type==2)
           	{
           	      USHORT SpecifyChannelFrequency;
		     if(Adapter->SpecifyChannel>11)
		   	{
                         SpecifyChannelFrequency=5000+(Adapter->SpecifyChannel*5);
		   	}else
                         	{
                                 SpecifyChannelFrequency=2407+(Adapter->SpecifyChannel*5);	
                         	}	

                  DBGPRINT(DBG_OID_OPT,(L"*****AgeOutTheActiveSsidEntry  type==2 Adapter->SpecifyChannel=0x%x,SpecifyChannelFrequency=%d,Adapter->PSBSSIDList[idx].Configuration.DSConfig=%d\n\r",Adapter->SpecifyChannel,SpecifyChannelFrequency,Adapter->PSBSSIDList[idx].Configuration.DSConfig));   
			
		    if(Adapter->PSBSSIDList[idx].Configuration.DSConfig==SpecifyChannelFrequency)
		   	{
		   	         DBGPRINT(DBG_ERROR,(L"*****AgeOutTheActiveSsidEntry type==2 One entry is tag to age out: idx=%d,Adapter->PSBSSIDList[%d].MacAddress[0]=0x%x,Adapter->PSBSSIDList[%d].MacAddress[5]=0x%x\n\r",idx,idx,Adapter->PSBSSIDList[idx].MacAddress[0],idx,Adapter->PSBSSIDList[idx].MacAddress[5])); 
		   	        Adapter->PSBssEntryAge[idx] = 1;
		   	}
           	   }
		   else{  
#endif	   	
             Adapter->PSBssEntryAge[idx] = 1;
             DBGPRINT(DBG_SCAN|DBG_HELP,(L"One entry is tag to age out\n\r")); 
#ifdef OPTIMIZER_OID
		 }
#endif	   
        } 
    }
*/
    return;
}

// tt ++ mic 2
VOID MrvMicDisconnectTimerFunction(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PMRVDRV_ADAPTER Adapter = (PMRVDRV_ADAPTER) FunctionContext;

    DBGPRINT(DBG_ASSO|DBG_TMR|DBG_HELP,(L"[Mrvl] Enter disconnect timer due to MIC error (%d)!\r\n", GetTickCount() ));

    DisconnectDueToMicError( Adapter );
}
// tt -- mic 2

// tt ++ mic error 2
BOOL IsInMicErrorPeriod( PMRVDRV_ADAPTER pAdapter )
{
    ULONG       ulCurrTime, ulTimeDiff;
    
    if ( pAdapter->ulMicErrorStartTime == 0 || pAdapter->bMicErrorIsHappened != TRUE )
        return FALSE;

    DBGPRINT( DBG_MACEVT, (L"[Mrvl] Check MIC error period\r\n") );

    NdisGetSystemUpTime(&ulCurrTime);
    
    if ( ulCurrTime > pAdapter->ulMicErrorStartTime )
    {
        ulTimeDiff = ulCurrTime - pAdapter->ulMicErrorStartTime;
    }
    else
    {
        ulTimeDiff = 0xFFFFFFFF - pAdapter->ulMicErrorStartTime + ulCurrTime;
    }

    if ( ulTimeDiff < 62000 )
    {
        DBGPRINT( DBG_MACEVT, (L"[Mrvl] Driver is still in MIC error period. Remaining %d ms.\r\n", 62000-ulTimeDiff ) );
        return TRUE;
    }

    DBGPRINT( DBG_MACEVT, (L"[Mrvl] MIC error period is finished\r\n") );

    pAdapter->ulMicErrorStartTime = 0;
    pAdapter->bMicErrorIsHappened = FALSE;
    
    return FALSE;
}
// tt --
                                                              
VOID
Application_Event_Notify(PMRVDRV_ADAPTER Adapter, ULONG type) 
{     
   OID_MRVL_DS__NOTIFY_UI      notifyui;
   notifyui.NotifyType = type;
   notifyui.ParamLen = 0;
   NdisMIndicateStatus(Adapter->MrvDrvAdapterHdl,
                       NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                       &notifyui,
                       sizeof(notifyui));
   NdisMIndicateStatusComplete(Adapter->MrvDrvAdapterHdl); 
}

VOID
Ndis_MediaStatus_Notify(PMRVDRV_ADAPTER Adapter, ULONG type)
{                                        
   //DWORD tick1,tick2;
              
   //tick1 = GetTickCount();

   NdisMIndicateStatus(Adapter->MrvDrvAdapterHdl,
                       type,
                       (PVOID)NULL, 
                       0);
   NdisMIndicateStatusComplete(Adapter->MrvDrvAdapterHdl); 

   //tick2 = GetTickCount(); 

   DBGPRINT(DBG_STHRD|DBG_ASSO|DBG_WARNING,(L"Indicate Media Status:%x \r\n",type ));
}


VOID
Signal_Pending_OID(PMRVDRV_ADAPTER Adapter)
{
   ULONG i=0;

    if(WaitForSingleObject( (Adapter)->hOidQueryEvent ,0 ) == WAIT_OBJECT_0)
    {
        DBGPRINT(DBG_ERROR, (L"ERROR: hOidQueryEvent already set, possible missed event\r\n"));
    }

   SetEvent( Adapter->hOidQueryEvent );

   Adapter->OIDAccessBlocked = FALSE;   
     
}







