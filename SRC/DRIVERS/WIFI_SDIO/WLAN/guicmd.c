/******************* © Marvell Semiconductor, Inc., 2001-2004 *****************
 *
 *  Purpose:    This module has implmentation of station command 
 *              processing functions
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/08/20 $
 *
 *	$Revision: #1 $
 *
 *****************************************************************************/

/*
===============================================================================
                                 INCLUDE FILES
===============================================================================
*/

#include "precomp.h"

ULONG beaconFrame[26] =   {
       0x00000a00, 0xc000075C, 0x00010002, 0x00403c34, 0x00000000,
       0x00000000, 0x00050005, 0x0080002c, 0xffff0014, 0xffffffff,
       0xaa55aa54, 0x1122aa55, 0x11221122, 0x00000000, 0xd8142ffd,
       0x000003f6, 0x00000000, 0x00040001, 0x616d0700, 0x6c657672,
       0x3802016c, 0x01010332, 0x02000604, 0x00010001, 0x01000605,
       0x33323100      };
       


/******************************************************************************
 *
 *  Name: ProcessHostCommand()
 *
 *  Description: Pass GUI host command to station
 *
 *  Arguments:	PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:        
 * 
 *  Notes: Driver will assign a sequence number for GUI command
 *
 *****************************************************************************/
NDIS_STATUS
ProcessHostCommand(
	IN PMRVDRV_ADAPTER Adapter,
	IN PUCHAR pCmdBuf
)
{
	//ULONG OPCode;
	USHORT Cmd, Size;
	PHostCmd_DS_GEN pCmdPtr;
	NDIS_STATUS Status;
	PHostCmd_DS_802_11_GET_STAT p11Stat;
	PHostCmd_DS_802_3_GET_STAT  p3Stat;
	//PHostCmd_DS_802_11_RSSI     pRSSI;

	pCmdPtr = (PHostCmd_DS_GEN)pCmdBuf;
	//      Get cmd and size info
	Cmd = pCmdPtr->Command;
	Size = pCmdPtr->Size;

	Status = NDIS_STATUS_SUCCESS;
	//      Check if we can process this command
	switch( Cmd ){
	case HostCmd_CMD_802_11_GET_STAT:
		pCmdPtr->Command = GetExpectedRetCode(Cmd);
		p11Stat = (PHostCmd_DS_802_11_GET_STAT)pCmdBuf;
		//      ToDo: Update 11 stat
		return Status;
		break;
	case HostCmd_CMD_802_3_GET_STAT:
		pCmdPtr->Command = GetExpectedRetCode(Cmd);
		p3Stat = (PHostCmd_DS_802_3_GET_STAT)pCmdBuf;
		p3Stat->XmitOK		= (ULONG)Adapter->XmitOK;
		p3Stat->RcvOK		= (ULONG)Adapter->RcvOK;
		p3Stat->XmitError	= (ULONG)Adapter->XmitError;
		p3Stat->RcvError	= (ULONG)Adapter->RcvError;
		p3Stat->RcvNoBuffer = (ULONG)Adapter->RcvNoBuffer;
		p3Stat->RcvCRCError = (ULONG)Adapter->RcvCRCError;
		return Status;
		break;
//	case HostCmd_CMD_802_11_RSSI:
//		pCmdPtr->Command = GetExpectedRetCode(Cmd);
//		pRSSI = (PHostCmd_DS_802_11_RSSI)pCmdBuf;
//		pRSSI->RSSI = Adapter->LastRSSI;
//		return Status;
//		break;
	default:
		//      First to check if the FW is busy processing any command
		//if( Adapter->CmdState != HostCmd_STATE_IDLE ){
			//pCmdPtr->Result = HostCmd_RESULT_BUSY;
			//Status = NDIS_STATUS_SUCCESS;
		//}
		//else{
		//	Status = PassHostCommand(Adapter, pCmdBuf);
		//}
		Status = NDIS_STATUS_PENDING;
	}
	return Status;
}

/******************************************************************************
 *
 *  Name: PassHostCommand()
 *
 *  Description: Pass GUI host command to station
 *
 *  Arguments:	PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value:
 *      NDIS_STATUS_RESOURCES
 *      NDIS_STATUS_PENDING
 * 
 *  Notes: Driver will assign a sequence number for GUI command
 *
 *****************************************************************************/
NDIS_STATUS
PassHostCommand(
	IN PMRVDRV_ADAPTER Adapter,
	IN USHORT PendingInfo,
	IN PVOID InformationBuffer,
	IN ULONG InformationBufferLength,
	IN PULONG BytesWritten,
	IN PULONG BytesNeeded
)
{
	USHORT Cmd, Size;
	PHostCmd_DS_GEN pCmdPtr;
	CmdCtrlNode *pTempCmd;

	pCmdPtr = (PHostCmd_DS_GEN)InformationBuffer;
	//      Get cmd and size info
	Cmd = pCmdPtr->Command;
	Size = pCmdPtr->Size;
	//      Get next free commnad control node
	pTempCmd = GetFreeCmdCtrlNode(Adapter);
	if( pTempCmd == NULL )
		return NDIS_STATUS_RESOURCES;
	pTempCmd->ExpectedRetCode = GetExpectedRetCode(Cmd);
	pTempCmd->Next = NULL;

	//      Assign new sequence number
	Adapter->SeqNum++;
	pCmdPtr->SeqNum = Adapter->SeqNum;


	switch(Cmd)
	{
/*		case HostCmd_CMD_SET_ACTIVE_SCAN_SSID:
		{
			PHostCmd_DS_SET_ACTIVE_SCAN_SSID pActiveScanSSID;
			
			Adapter->SetActiveScanSSID = TRUE;
			pActiveScanSSID = (PHostCmd_DS_SET_ACTIVE_SCAN_SSID)InformationBuffer;
			NdisMoveMemory(&(Adapter->ActiveScanSSID), 
							&(pActiveScanSSID->ActiveScanSSID), 
							sizeof(NDIS_802_11_SSID)); 
			return NDIS_STATUS_SUCCESS;
			
		}
*/		
/*
		case HostCmd_CMD_DUTY_CYCLE_TEST:
		{
			ULONG i;
			PHostCmd_DS_DUTY_CYCLE_TEST pCmdPtrDutyTest;	
			
			pCmdPtrDutyTest=(PHostCmd_DS_DUTY_CYCLE_TEST)InformationBuffer;
			beaconFrame[2]=pCmdPtrDutyTest->RFParam;
			for(i=0;i<26;i++)
			{
				NdisMoveToMappedMemory((PVOID)((ULONG)Adapter->MrvDrvVirtualAddrBAR2+pCmdPtrDutyTest->BeaconOffsetInSQ +i*4),
						&beaconFrame[i], 
						sizeof(ULONG));
			}	
		}				 						
			return NDIS_STATUS_SUCCESS;	
*/
/*		
		case HostCmd_CMD_802_11_RF_CHANNEL:
		{			
			PHostCmd_DS_802_11_RF_CHANNEL pRF;
			
			pRF = (PHostCmd_DS_802_11_RF_CHANNEL)InformationBuffer;
			if ( pRF->Action == HostCmd_ACT_GEN_SET ) {
				Adapter->Channel = pRF->CurentChannel;
				return NDIS_STATUS_SUCCESS;
			}
			break;
		}
*/
		default:
			break;
	}
	
	SetCmdCtrlNode(
		Adapter,
		pTempCmd,
		OID_MRVL_OEM_COMMAND,
		PendingInfo,
		HostCmd_OPTION_USE_INT,
		0,
		FALSE,
		BytesWritten,
		NULL,
		BytesNeeded,
		InformationBuffer);

    // Copy command to command buffer
    NdisMoveMemory((PVOID)pTempCmd->BufVirtualAddr, (PVOID)pCmdPtr, (ULONG)Size);

    // If there is no pending commnad, else, append command to Q
    //DownloadCommandToStation(Adapter, pTempCmd); 

    //dralee_1108++ 
    LeaveCriticalSection(&Adapter->CmdQueueExeSection); 
    InsertCmdToQueue (Adapter, pTempCmd);    
    //dralee_1108++ 
    LeaveCriticalSection(&Adapter->CmdQueueExeSection); 
    GetCmdFromQueueToExecute(Adapter);

    return NDIS_STATUS_PENDING;
}

