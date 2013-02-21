
#include "precomp.h"

// Convert Region string to code integer
UCHAR region_2_code(CHAR *region)
{
    int i, size;
    CHAR country1[COUNTRY_CODE_LEN + 1];

    DBGPRINT(DBG_ALLEN, (L"region_2_code() \n"));
    NdisMoveMemory (country1, region, COUNTRY_CODE_LEN);
    country1[COUNTRY_CODE_LEN]='\0';
	
    size = sizeof(region_code_mapping)/sizeof(region_code_mapping_t);  

    for (i=0; i<size; i++)
    {  
        if((strncmp(region_code_mapping[i].region,country1,COUNTRY_CODE_LEN-1) ) )  
              continue;         
        return (region_code_mapping[i].code);
    }
                                              
    DBGPRINT(DBG_11D|DBG_HELP,(L"Use default:US region code:%x",region_code_mapping[0].code));
    return region_code_mapping[0].code;
}


// Convert interger code to region string
CHAR *code_2_region( UCHAR code )
{
	int i;
	int size = sizeof(region_code_mapping)/ sizeof(region_code_mapping_t); 
	for ( i=0; i< size; i++ ) 
	{
		if ( region_code_mapping[i].code == code )
			return (region_code_mapping[i].region);
	}
	return NULL;
}

/*
// Convert chan to frequency
ULONG chan_2_freq ( USHORT chan, UCHAR band)
{
	CHANNEL_FREQ_POWER 	*cf;
	UCHAR			    cnt;
	int 			    i;

	if ( band == MRVDRV_802_11_BAND_A ) 
	{
		cf =channel_freq_power_UN_AJ; 
		cnt = sizeof(channel_freq_power_UN_AJ)/sizeof(CHANNEL_FREQ_POWER);
	}
	else
	{
		cf = channel_freq_power_UN_BG;
		cnt = sizeof(channel_freq_power_UN_BG)/sizeof(CHANNEL_FREQ_POWER);
	}

	for ( i=0; i< cnt; i++ ) 
	{
		if ( chan == cf[i].Channel ) 
			return cf[i].Freq;
	}
	return 0;
}
*/

VOID
ResetAllScanTypeAndPower(
		PMRVDRV_ADAPTER Adapter
)
{
    UCHAR mode;

	DBGPRINT(DBG_ALLEN, (L"ResetAllScanTypeAndPower() \n"));
                                                       
    //++dralee_20060427
	//RETAILMSG(1, (L"0ResetAllScanTypeAndPower() \n")); 
    if( Adapter->Enable80211D )  
        mode = HostCmd_SCAN_TYPE_PASSIVE;
    else
        mode = HostCmd_SCAN_TYPE_ACTIVE; 

	if (Adapter->region_channel[0].Valid)
	{
		Adapter->region_channel[0].Region = 0;
		NdisFillMemory(Adapter->region_channel[0].ScanType, MAX_PSCAN_CH_NUM, mode);
		NdisFillMemory(Adapter->region_channel[0].TxPower, MAX_PSCAN_CH_NUM, TX_PWR_DEFAULT);
	}
	if (Adapter->region_channel[1].Valid)
	{
		Adapter->region_channel[1].Region = 0;
		NdisFillMemory(Adapter->region_channel[1].ScanType, MAX_PSCAN_CH_NUM, mode);
		NdisFillMemory(Adapter->region_channel[1].TxPower, MAX_PSCAN_CH_NUM, TX_PWR_DEFAULT);
	}
}

VOID
UpdateScanTypeFromCountryIE(
		PMRVDRV_ADAPTER Adapter, 
	    UCHAR           BssidListIndex   // 0xff for all list entry	
)
{
    PBSS_DESCRIPTION_SET_ALL_FIELDS pBissidListDesc;
	IEEEtypes_CountryInfoFullSet_t  *pCountryIE;
	CHANNEL_FREQ_POWER              *pNewCFP_BG;
    CHANNEL_FREQ_POWER              *pNewCFP_A;

	IEEEtypes_SubbandSet_t          *pSubband;
	int                             RemainLen;
	int 							i, j, k, k1;
    UCHAR 							CountryNum;

	//RETAILMSG(1, (L"UpdateScanTypeFromCountryIE() \n"));	
	
	pBissidListDesc = Adapter->PSBssDescList;
	
	if (BssidListIndex == 0xff)
	{
    	for (i=0; Adapter->ulPSNumOfBSSIDs ; i++)
    	{
            pCountryIE = &(pBissidListDesc[i].CountryInfo);	
			CountryNum = region_2_code( pCountryIE->CountryCode);
			if (CountryNum != 0)  // first conntry info 
				break;			
    	}
	}
	else
	{
	    pCountryIE = &(pBissidListDesc[BssidListIndex].CountryInfo);
        CountryNum = region_2_code( pCountryIE->CountryCode);
	}
    
    DBGPRINT(DBG_11D|DBG_HELP,(L"REGION CODE INDEX:%x\r\n",CountryNum)); 

	switch (CountryNum)
	{
    	case 0x10:    // "US ", "CA "
        	pNewCFP_BG = channel_freq_power_US_BG;
			pNewCFP_A  = channel_freq_power_A;

			k = sizeof(channel_freq_power_US_BG)/sizeof(CHANNEL_FREQ_POWER); 
	        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
        	break;
		case 0x30:    // "EU "
			pNewCFP_BG = channel_freq_power_EU_BG;
			pNewCFP_A  = channel_freq_power_EU_A;

			k = sizeof(channel_freq_power_EU_BG)/sizeof(CHANNEL_FREQ_POWER); 
	        k1 = sizeof(channel_freq_power_EU_A)/sizeof(CHANNEL_FREQ_POWER);
			break;
		case 0x31:    // "ES "	
			pNewCFP_BG = channel_freq_power_SPN_BG;
			pNewCFP_A  = channel_freq_power_A;

			k = sizeof(channel_freq_power_SPN_BG)/sizeof(CHANNEL_FREQ_POWER); 
	        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
			break;
		case 0x32:    // "FR "
			pNewCFP_BG = channel_freq_power_FR_BG;
			pNewCFP_A  = channel_freq_power_A;

			k = sizeof(channel_freq_power_FR_BG)/sizeof(CHANNEL_FREQ_POWER); 
	        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
			break;	
		case 0x40:    // "JP "
			pNewCFP_BG = channel_freq_power_JPN_BG;
			pNewCFP_A  = channel_freq_power_JPN_A;

			k = sizeof(channel_freq_power_JPN_BG)/sizeof(CHANNEL_FREQ_POWER); 
	        k1 = sizeof(channel_freq_power_JPN_A)/sizeof(CHANNEL_FREQ_POWER);
			break;
        default:  //Coverity Error id:5,6,7 (UNINIT)
        	pNewCFP_BG = channel_freq_power_US_BG;
			pNewCFP_A  = channel_freq_power_A;

			k = sizeof(channel_freq_power_US_BG)/sizeof(CHANNEL_FREQ_POWER); 
	        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
        	break;
    }

	DBGPRINT(DBG_ALLEN, (L"CountryNum = %d, k = %d, k1 = %d \n", CountryNum, k, k1));
		
    if (Adapter->region_channel[0].Valid)
	{   
        //RETAILMSG(1,(L"region channel 0 valid\r\n"));

		Adapter->region_channel[0].Region  = CountryNum;
		// set passive scan type to all channel 
		NdisFillMemory(Adapter->region_channel[0].ScanType, MAX_PSCAN_CH_NUM, 0x01);

		for (i=0; i< Adapter->region_channel[0].NrCFP; i++)
		{
			for (j=0; j< k; j++)
			{
				if (Adapter->region_channel[0].CFP[i].Channel == pNewCFP_BG[j].Channel)
				{
					Adapter->region_channel[0].ScanType[i] = 0;	
				    break;
				}	
			}
        }

        RemainLen = pCountryIE->Len - COUNTRY_CODE_LEN;
		pSubband = pCountryIE->Subband;
		while (RemainLen)
		{
			for(j=0; j<pSubband->NoOfChan; j++)
			{
				for (i=0; i< Adapter->region_channel[0].NrCFP; i++)
				{
					if (Adapter->region_channel[0].CFP[i].Channel == pSubband->FirstChan + j)
					{
						Adapter->region_channel[0].TxPower[i] = pSubband->MaxTxPwr;
						break;
					
					}
				}	
			}
			RemainLen -= sizeof(IEEEtypes_SubbandSet_t);
			if (RemainLen < sizeof(IEEEtypes_SubbandSet_t))
				break;
			pSubband += 1;
		}

	}
	if (Adapter->region_channel[1].Valid)
	{                  
        //RETAILMSG(1,(L"region channel 1 valid\r\n"));

		Adapter->region_channel[1].Region  = CountryNum;
		// set passive scan type to all channel 
        NdisFillMemory(Adapter->region_channel[1].ScanType, MAX_PSCAN_CH_NUM, 0x01);

		
		for (i=0; i< Adapter->region_channel[1].NrCFP; i++)
		{
			for (j=0; j< k1; j++)
			{
				if (Adapter->region_channel[1].CFP[i].Channel == pNewCFP_BG[j].Channel)
				{   // set active sacn type to the channel  
					Adapter->region_channel[1].ScanType[i] = 0;	
				    break;
				}	
			}
        }

		RemainLen = pCountryIE->Len - COUNTRY_CODE_LEN;
		pSubband = pCountryIE->Subband;
		while (RemainLen)
		{
			for(j=0; j<pSubband->NoOfChan; j++)
			{
				for (i=0; i< Adapter->region_channel[1].NrCFP; i++)
				{
					if (Adapter->region_channel[1].CFP[i].Channel == pSubband->FirstChan + j)
					{
						Adapter->region_channel[1].TxPower[i] = pSubband->MaxTxPwr;
						break;
					
					}
				}	
			}
			RemainLen -= sizeof(IEEEtypes_SubbandSet_t);
			if (RemainLen < sizeof(IEEEtypes_SubbandSet_t))
				break;
			pSubband += 1;
		}
		HexDump(DBG_ALLEN, "region_channel[1]", (UCHAR *)&(Adapter->region_channel[1]), 
			               sizeof(REGION_CHANNEL));
		HexDump(DBG_ALLEN, "Adapter->region_channel.CFP", (UCHAR *)(Adapter->region_channel[1].CFP), 
			               sizeof(CHANNEL_FREQ_POWER)*k1);
	} 
    return;
}  
                                                     
//dralee_20060613
VOID UpdateScanTypeByCountryCode( PMRVDRV_ADAPTER Adapter, UCHAR CountryNum ) 
{                                               
	CHANNEL_FREQ_POWER              *pNewCFP_BG;
    CHANNEL_FREQ_POWER              *pNewCFP_A;
	int 							i, j, k, k1; 
  
		switch (CountryNum)
		{
        	case 0x10:    // "US ", "CA "
            	pNewCFP_BG = channel_freq_power_US_BG;
				pNewCFP_A  = channel_freq_power_A;

				k = sizeof(channel_freq_power_US_BG)/sizeof(CHANNEL_FREQ_POWER); 
		        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
            	break;
				
			case 0x30:    // "EU "
				pNewCFP_BG = channel_freq_power_EU_BG;
				pNewCFP_A  = channel_freq_power_EU_A;

				k = sizeof(channel_freq_power_EU_BG)/sizeof(CHANNEL_FREQ_POWER); 
		        k1 = sizeof(channel_freq_power_EU_A)/sizeof(CHANNEL_FREQ_POWER);
				break;

			case 0x31:    // "ES "	
				pNewCFP_BG = channel_freq_power_SPN_BG;
				pNewCFP_A  = channel_freq_power_A;

				k = sizeof(channel_freq_power_SPN_BG)/sizeof(CHANNEL_FREQ_POWER); 
		        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
				break;

			case 0x32:    // "FR "
				pNewCFP_BG = channel_freq_power_FR_BG;
				pNewCFP_A  = channel_freq_power_A;

				k = sizeof(channel_freq_power_FR_BG)/sizeof(CHANNEL_FREQ_POWER); 
		        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
				break;	

			case 0x40:    // "JP "
				pNewCFP_BG = channel_freq_power_JPN_BG;
				pNewCFP_A  = channel_freq_power_JPN_A;

				k = sizeof(channel_freq_power_JPN_BG)/sizeof(CHANNEL_FREQ_POWER); 
		        k1 = sizeof(channel_freq_power_JPN_A)/sizeof(CHANNEL_FREQ_POWER);
				break;
            default: //Coverity Error id:10,11,12 (UNINIT)
            	pNewCFP_BG = channel_freq_power_US_BG;
				pNewCFP_A  = channel_freq_power_A;

				k = sizeof(channel_freq_power_US_BG)/sizeof(CHANNEL_FREQ_POWER); 
		        k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
            	break;
        }

		DBGPRINT(DBG_ALLEN, (L"CountryNum = %d, k = %d, k1 = %d \n", CountryNum, k, k1));
			
        if (Adapter->region_channel[0].Valid)
		{   
            //RETAILMSG(1,(L"region channel 0 valid\r\n"));

			Adapter->region_channel[0].Region  = CountryNum;
			// set passive scan type to all channel 
			NdisFillMemory(Adapter->region_channel[0].ScanType, MAX_PSCAN_CH_NUM, 0x01);

			for (i=0; i< Adapter->region_channel[0].NrCFP; i++)
			{
				for (j=0; j< k; j++)
				{
					if (Adapter->region_channel[0].CFP[i].Channel == pNewCFP_BG[j].Channel)
					{
						Adapter->region_channel[0].ScanType[i] = 0;	
					    break;
					}	
				}
            }
		}    

		if (Adapter->region_channel[1].Valid)
		{                  
            //RETAILMSG(1,(L"region channel 1 valid\r\n"));

			Adapter->region_channel[1].Region  = CountryNum;
			// set passive scan type to all channel 
            NdisFillMemory(Adapter->region_channel[1].ScanType, MAX_PSCAN_CH_NUM, 0x01);

			
			for (i=0; i< Adapter->region_channel[1].NrCFP; i++)
			{
				for (j=0; j< k1; j++)
				{
					if (Adapter->region_channel[1].CFP[i].Channel == pNewCFP_BG[j].Channel)
					{   // set active sacn type to the channel  
						Adapter->region_channel[1].ScanType[i] = 0;	
					    break;
					}	
				}
            }

		} 
}

// Generate domaininfo CountryIE
VOID 
GenerateDomainInfoFromCountryIE( 
		MrvlIEtypes_DomainParamSet_t  *pDomainInfo,
		IEEEtypes_CountryInfoFullSet_t  *pCountryIE
)
{
	DBGPRINT(DBG_ALLEN, (L"GenerateDomainInfoFromCountryIE() \n"));	

	NdisMoveMemory (pDomainInfo->CountryCode, pCountryIE->CountryCode, COUNTRY_CODE_LEN);
	pDomainInfo->Header.Len= (SHORT)pCountryIE->Len;
	pDomainInfo->Header.Type = TLV_TYPE_DOMAIN;	

   	NdisMoveMemory(pDomainInfo->Subband, pCountryIE->Subband, pCountryIE->Len);
	
	return;
}

// Enable/Disable 11D via set SNMP OID to FW
VOID
Enable_11d( 
		PMRVDRV_ADAPTER Adapter, 
		BOOLEAN enable )
{
	USHORT Cmdopt = 0;

	if (enable)
    {
		Cmdopt = 1;
        DBGPRINT(DBG_11D|DBG_HELP,(L"enable 11d:%x\r\n",Cmdopt ));
    } 
		
    PrepareAndSendCommand(
			Adapter,
			HostCmd_CMD_802_11_SNMP_MIB,
                        HostCmd_ACT_GEN_SET,
			HostCmd_OPTION_USE_INT,
			OID_802_11D_ENABLE,
			HostCmd_PENDING_ON_NONE,
			0,
		    FALSE,
			NULL,
			NULL,
			NULL,
			&Cmdopt);
	
	return;
}

// Set DOMAIN INFO to FW
VOID 
SetDomainInfo( 
		PMRVDRV_ADAPTER Adapter, 
		IEEEtypes_CountryInfoFullSet_t  *pCountryIE)
{

    UCHAR  CountryNum;
            
    CountryNum = region_2_code( pCountryIE->CountryCode);
 
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
            (PVOID)&CountryNum
			);
}  
	
//dralee_20060502   
VOID
Generate_domain_info_11d(
                          UCHAR CountryNum,
                          UCHAR  band, 
                          MrvlIEtypes_DomainParamSet_t *domaininfo)
{
    UCHAR  NoOfSubband =0;
    UCHAR  NoOfChan = 0;
    UCHAR  NoOfParsedChan =0;
    UCHAR  firstChan=0, nextChan=0, maxPwr=0;
    UCHAR  i, flag =0;
    CHANNEL_FREQ_POWER              *pNewCFP_BG;
    CHANNEL_FREQ_POWER              *pNewCFP_A;
    CHANNEL_FREQ_POWER              *pNewCFP;
    int k, k1;   
    //USHORT *dbg;
                               
    
    //copy region code string 
    NdisMoveMemory( domaininfo->CountryCode, code_2_region((UCHAR)CountryNum), 
                COUNTRY_CODE_LEN);
    
    domaininfo->CountryCode[2] = 0x20;
    //RETAILMSG(1,(L"REGION:%c%c%c\r\n",domaininfo->CountryCode[0],domaininfo->CountryCode[1],domaininfo->CountryCode[2]));

    if ((CountryNum != 0) && 
        ((CountryNum == 0x10) || (CountryNum == 0x30) || (CountryNum == 0x31) ||
         (CountryNum == 0x32) || (CountryNum == 0x40)))
    {  
        switch (CountryNum)
        {
            case 0x10:    // "US ", "CA "
                pNewCFP_BG = channel_freq_power_US_BG;
                pNewCFP_A  = channel_freq_power_A;

                k = sizeof(channel_freq_power_US_BG)/sizeof(CHANNEL_FREQ_POWER); 
                k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
                break;
                
            case 0x30:    // "EU "
                pNewCFP_BG = channel_freq_power_EU_BG;
                pNewCFP_A  = channel_freq_power_EU_A;

                k = sizeof(channel_freq_power_EU_BG)/sizeof(CHANNEL_FREQ_POWER); 
                k1 = sizeof(channel_freq_power_EU_A)/sizeof(CHANNEL_FREQ_POWER);
                break;

            case 0x31:    // "ES "  
                pNewCFP_BG = channel_freq_power_SPN_BG;
                pNewCFP_A  = channel_freq_power_A;

                k = sizeof(channel_freq_power_SPN_BG)/sizeof(CHANNEL_FREQ_POWER); 
                k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
                break;

            case 0x32:    // "FR "
                pNewCFP_BG = channel_freq_power_FR_BG;
                pNewCFP_A  = channel_freq_power_A;

                k = sizeof(channel_freq_power_FR_BG)/sizeof(CHANNEL_FREQ_POWER); 
                k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
                break;  

            case 0x40:    // "JP "
                pNewCFP_BG = channel_freq_power_JPN_BG;
                pNewCFP_A  = channel_freq_power_JPN_A;

                k = sizeof(channel_freq_power_JPN_BG)/sizeof(CHANNEL_FREQ_POWER); 
                k1 = sizeof(channel_freq_power_JPN_A)/sizeof(CHANNEL_FREQ_POWER);
                break;
            default:  //Coverity Error id:13,14,15,16 (UNINIT)
                pNewCFP_BG = channel_freq_power_US_BG;
                pNewCFP_A  = channel_freq_power_A;

                k = sizeof(channel_freq_power_US_BG)/sizeof(CHANNEL_FREQ_POWER); 
                k1 = sizeof(channel_freq_power_A)/sizeof(CHANNEL_FREQ_POWER);
                break;
        }
    }
    else   //dralee, add for coverity code analysis
    {
        DBGPRINT(DBG_11D|DBG_ERROR,(L"Unknowed Country code\r\n")); 
        return;
    }

   
    if( band == MRVDRV_802_11_BAND_A )  
    {
        pNewCFP = pNewCFP_A;
        NoOfChan = k1;
    }
    else 
    {
        pNewCFP = pNewCFP_BG;
        NoOfChan = k; 
}

    for ( i=0; i<NoOfChan; i++ )
    {
        if ( !flag )
        {
            flag = 1;
            nextChan = firstChan =(UCHAR) pNewCFP[i].Channel;
            maxPwr =  (UCHAR)pNewCFP[i].MaxTxPower;
            NoOfParsedChan = 1;
            continue;
        }

        if ( pNewCFP[i].Channel == nextChan+1 && 
            pNewCFP[i].MaxTxPower == maxPwr )
        {
            nextChan++;
            NoOfParsedChan++;
        }
        else
        {
            domaininfo->Subband[NoOfSubband].FirstChan = firstChan;
            domaininfo->Subband[NoOfSubband].NoOfChan = NoOfParsedChan;
            domaininfo->Subband[NoOfSubband].MaxTxPwr = maxPwr;
            NoOfSubband++;
            nextChan = firstChan = (UCHAR)pNewCFP[i].Channel;
            maxPwr = (UCHAR) pNewCFP[i].MaxTxPower;
        }
    }

    if( flag ) 
    {
        domaininfo->Subband[NoOfSubband].FirstChan = firstChan;
        domaininfo->Subband[NoOfSubband].NoOfChan = NoOfParsedChan;
        domaininfo->Subband[NoOfSubband].MaxTxPwr = maxPwr;
        NoOfSubband++;
    }
    //domaininfo->NoOfSubband = NoOfSubband;   
    domaininfo->Header.Len = NoOfSubband * sizeof(IEEEtypes_SubbandSet_t) + COUNTRY_CODE_LEN; 
    domaininfo->Header.Type = TLV_TYPE_DOMAIN;

    //dbg = (USHORT *)domaininfo;
    //RETAILMSG(1,(L"domain:%x,%x,%x,%x,%x,%x\r\n",dbg[0],dbg[1],dbg[2],dbg[3],dbg[4],dbg[5]));
}
