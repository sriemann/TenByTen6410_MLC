#include "precomp.h"
#include "pkfuncs.h"

UCHAR  wmm_tos2ac[16][8] = {
	{	// 0 0 0 0	all enabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 0 0 0 1	AC_PRIO_BE disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 0 0 1 0	AC_PRIO_BK should NOT be disabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 0 0 1 1	AC_PRIO_BE disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 0 1 0 0	AC_PRIO_VI disabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 0 1 0 1	AC_PRIO_VI & BE	disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 0 1 1 0	AC_PRIO_VI disabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 0 1 1 1	AC_PRIO_VI & BE	disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_VO,
		AC_PRIO_VO
	},
	{	// 1 0 0 0	AC_PRIO_VO disabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI
	},
	{	// 1 0 0 1	AC_PRIO_VO & BE disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI
	},
	{	// 1 0 1 0	AC_PRIO_VO disabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI
	},
	{	// 1 0 1 1	AC_PRIO_VO & BE disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI,
		AC_PRIO_VI
	},
	{	// 1 1 0 0	AC_PRIO_VO & VI disabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE
	},
	{	// 1 1 0 1	AC_PRIO_VO & VI & BE disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK
	},
	{	// 1 1 1 0	AC_PRIO_VO & VI disabled
		AC_PRIO_BE,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE,
		AC_PRIO_BE
	},
	{	// 1 1 1 1	AC_PRIO_VO & VI & BE disabled
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK,
		AC_PRIO_BK
	}
};

static UCHAR wmm_tos2priority[8] = {
/*	Priority   DSCP   DSCP   DSCP	WMM
		    P2     P1     P0 	AC    */
	0x00,	/*  0	   0	  0	AC_BE */
	0x04,	/*  0	   0	  1	AC_BK */
	0x02,	/*  0	   1	  0	AC_BK */
	0x06,	/*  0	   1	  1	AC_BE */
	0x01,	/*  1	   0	  0	AC_VI */
	0x05,	/*  1	   0	  1	AC_VI */
	0x03,	/*  1	   1	  0	AC_VO */
	0x07	/*  1	   1	  1	AC_VO */
};


int wlan_wmm_enable_ioctl(PMRVDRV_ADAPTER Adapter, PVOID InformationBuffer)
{
    //NDIS_STATUS     Status;
    //PNDIS_PACKET    pPacket;
    POID_MRVL_DS_WMM_STATE pWmmState;

    pWmmState = (POID_MRVL_DS_WMM_STATE)InformationBuffer;

//    DBGPRINT(DBG_WMM, ("wlan_wmm_enable_ioctl state %x \n", pWmmState->State));
//    DBGPRINT(DBG_WMM, ("wmm state size %d \n", sizeof(POID_MRVL_DS_WMM_STATE)));

    switch (pWmmState->State)
    {
	case CMD_DISABLED: /* disable */
	     if (Adapter->MediaConnectStatus == NdisMediaStateConnected)
		return FALSE; 

	     Adapter->WmmDesc.required = 0;

	     if (!Adapter->WmmDesc.enabled)
 		  return FALSE; 
	     else
		  Adapter->WmmDesc.enabled = 0;

	     if (Adapter->SentPacket) 
	     { 
		 Adapter->SentPacket = NULL;
	     }

		/* Release all skb's in all the queues */
	     wmm_cleanup_queues(Adapter);
		
	break;

	case CMD_ENABLED: /* enable */
	     if (Adapter->MediaConnectStatus == NdisMediaStateConnected)
	     {
//                DBGPRINT(DBG_WMM, ("The media is already connected \n"));
		return FALSE; 
             } 
	     Adapter->WmmDesc.required = 1;
//             DBGPRINT(DBG_WMM, ("Enable Firmware WMM \n"));

		break;
	}

	return TRUE;
}


int wmm_lists_empty(PMRVDRV_ADAPTER Adapter)
{
	int i;

  for (i = AC_PRIO_BE; i < MAX_AC_QUEUES; i++)
    if (!IsTxQEmpty(&Adapter->TXPRYQ[i]))
      return 0; 
    return 1;
}

void wmm_cleanup_queues(PMRVDRV_ADAPTER Adapter)
{
    PTXQ_KEEPER  pTxQKeeper;
    PTXQ_NODE    pTQNode;

    PNDIS_PACKET	pPacket;
    NDIS_STATUS     Status;

    Status = NDIS_STATUS_FAILURE;
    while(1)
    { 
        TxPacketDeQueue(Adapter, &pTxQKeeper, &pTQNode); 
        if( pTQNode == NULL )
            return;

        pPacket = pTQNode -> pPacket;   
        NDIS_SET_PACKET_STATUS(pPacket, Status);
        NdisMSendComplete(
                                Adapter->MrvDrvAdapterHdl,
                                pPacket,
                                Status);    

        PushFreeTxQNode(Adapter->TxFreeQ,pTQNode);  
        Adapter->TxPacketCount--;
    } // end of for loop
    return;
}

UCHAR wmm_get_tos(PNDIS_PACKET pPacket)
{
     UCHAR tos = 0, temp;
     PNDIS_BUFFER pBuffer, pNextBuffer;
     UINT BufferCount, TotalPktLen;
     UINT Length1, Length2;
     PVOID pVirtualAddr1=NULL, pVirtualAddr2=NULL;
     USHORT type;
     PUCHAR ptr;
     
     NdisQueryPacket(pPacket, NULL, &BufferCount, &pBuffer, &TotalPktLen);    
     
     
     if(TotalPktLen >= MIN_IP_PKT_LEN)
     {
     	  NdisQueryBuffer(pBuffer, pVirtualAddr1, &Length1);
          ptr = (PUCHAR)pVirtualAddr1;
     	  if(Length1 < 16)
     	  { 
     	      NdisGetNextBuffer(pBuffer, &pNextBuffer);
     	      NdisQueryBuffer(pNextBuffer, pVirtualAddr2, &Length2);
              type = ptr[12] << 8;
              switch(Length1)
              {
              	  case 13:
              	       ptr = (PUCHAR)pVirtualAddr2;
              	       type |= ptr[0];
              	       temp = ptr[2];
              	  break;
              	  case 14:
              	       type |= ptr[13];
              	       ptr = (PUCHAR)pVirtualAddr2;
                       temp = ptr[1];              	       
              	  break;
              	  case 15:
              	       type |= ptr[13];
              	       ptr = (PUCHAR)pVirtualAddr2;
                       temp = ptr[0];              	       
              	  break;
              	
              }	
     	  }
     	  else
     	  {
               type |= ptr[13];
               temp = ptr[15];     	  	
     	  }	    
          
         
         if(type = 0x0800)  // IP packet
            tos = temp;
     }

     return tos;	
	
}	


/*
typedef struct _NDIS_PACKET_8021Q_INFO {
    union {
        struct {
            UINT32  UserPriority:3;
            UINT32  CanonicalFormatId:1;
            UINT32  VlanId:12;
            UINT32  Reserved:16;
        } TagHeader;
        PVOID  Value;
    };
} NDIS_PACKET_8021Q_INFO, *PNDIS_PACKET_8021Q_INFO;

// the value is deinfed in XP DDK ndis.h. MSFT will provide a formal header file later.
#define Ieee8021QInfo       6
*/
UCHAR wmm_get_pkt_priority( PNDIS_PACKET Packet )
{
    PNDIS_PACKET_EXTENSION pNdisExt;
    UINT        nPriority;

    pNdisExt = NDIS_PACKET_EXTENSION_FROM_PACKET( Packet );
    if ( pNdisExt )
    {
        nPriority = (UINT) pNdisExt->NdisPacketInfo[Ieee8021pPriority];
        //DbgWmmMsg( (L"WMM- pkt priority=%d\n", nPriority) );
        return nPriority;
    }
    else
    {
        return 0;
    }
}

UCHAR wmm_get_pkt_priority_2( PNDIS_PACKET Packet )
{
	return 0;

}

// tt ++ wmm
VOID wmm_update_status_from_cmd_resp( PMRVDRV_ADAPTER Adapter )
{
    int     i;
    PHostCmd_DS_802_11_WMM_GET_STATUS   pCmdResp;

    pCmdResp = (PHostCmd_DS_802_11_WMM_GET_STATUS) Adapter->CurCmd->BufVirtualAddr;

    for (Adapter->WmmDesc.acstatus=0, i=0; i<MAX_AC_QUEUES; i++)
    {
        Adapter->WmmDesc.acstatus |= (pCmdResp->Status[i].Disabled ? 1 : 0) << (MAX_AC_QUEUES - 1 - i);
    }
    
    DbgWmmMsg( (L"+wmm+ WMM status change: %d %d %d %d [acstatus=0x%x]\n", 
        pCmdResp->Status[0].Disabled, pCmdResp->Status[1].Disabled, 
        pCmdResp->Status[2].Disabled, pCmdResp->Status[3].Disabled,
        Adapter->WmmDesc.acstatus ) );

}

/*
    nAC = AC_PRIO_BE to AC_PRIO_VO, if = 0xff, means set policy for all AC
*/
VOID wmm_set_ack_policy( PMRVDRV_ADAPTER Adapter, UCHAR nAC, UCHAR nPolicy )
{
    OID_MRVL_DS_WMM_ACK_POLICY AckPolicy;

    if ( nAC >= AC_PRIO_BE && nAC <= AC_PRIO_VO )
    {
        AckPolicy.AC = nAC;
        AckPolicy.AckPolicy = nPolicy;
        
        DbgWmmMsg( (L"+wmm+ Set wmm Ack policy for AC=%d, policy=%d\n", nAC, nPolicy) );
        PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_WMM_ACK_POLICY,
                HostCmd_ACT_SET,
                HostCmd_OPTION_USE_INT,
                (NDIS_OID)0,
                HostCmd_PENDING_ON_NONE,
                0,
                FALSE,
                NULL,
                NULL,
                NULL,
                &AckPolicy);
    }
    else if ( nAC == 0xff )
    {
        DbgWmmMsg( (L"+wmm+ Set wmm Ack policy for all AC, !!NOT implement yet!!\n") );
    }
    else
    {
        DbgWmmMsg( (L"+wmm+ Wrong AC (%d) for setting Ack policy\n", nAC) );
    }
    
}

NDIS_STATUS wmm_set_sleep_period( PMRVDRV_ADAPTER Adapter, USHORT nPeriod )
{
    OID_MRVL_DS_WMM_SLEEP_PERIOD    OidSleepPeriod;
    NDIS_STATUS         nStatus;
    
    if ( nPeriod < 10 || nPeriod > 60 )
        return NDIS_STATUS_FAILURE;

    OidSleepPeriod.period = nPeriod;

    DbgWmmMsg( (L"+wmm+ Set SLEEP PERIOD = %d ms\n", OidSleepPeriod.period ) );

    nStatus = PrepareAndSendCommand(
                Adapter,
                HostCmd_CMD_802_11_SLEEP_PERIOD,
                HostCmd_ACT_SET,
                HostCmd_OPTION_USE_INT,
                (NDIS_OID)0,
                HostCmd_PENDING_ON_NONE,
                0,
                FALSE,
                NULL,
                NULL,
                NULL,
                &OidSleepPeriod);

    if ( nStatus == NDIS_STATUS_SUCCESS )
        Adapter->sleepperiod = nPeriod;

    return nStatus;
    
}
// tt --
