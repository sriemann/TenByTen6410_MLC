/******************* ?Marvell Semiconductor, Inc., *************************
 *
 *  Purpose:
 *
 *      This module provides the implementation of power management routines
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/08/23 $
 *
 *	$Revision: #2 $
 *
 *****************************************************************************/

#include "precomp.h"
#include "If.h"
#include "power.h"


VOID
PSWakeup(
	IN PMRVDRV_ADAPTER Adapter
	)
{
	CmdCtrlNode *NewCmd  = NULL;
	
	DBGPRINT(DBG_PS,(L"PSWakeup: exit powersave mode!\n"));

    // Add a PS Exit command in Q1
	NewCmd = GetFreeCmdCtrlNode(Adapter);
	if (NewCmd)
		MakePsCmd (Adapter, NewCmd, HostCmd_SubCmd_Exit_PS);
              
    //dralee_1108++
    EnterCriticalSection(&Adapter->CmdQueueExeSection);

	InsertCmdToQueue(Adapter, NewCmd);

    //dralee_1108++ 
    LeaveCriticalSection(&Adapter->CmdQueueExeSection); 


	GetCmdFromQueueToExecute(Adapter);

	return;
}


VOID
PSSleep(
	IN PMRVDRV_ADAPTER Adapter
	)
{
	CmdCtrlNode *NewCmd  = NULL;
	
	DBGPRINT(DBG_CMD | DBG_PS | DBG_CRLF ,(L"PSSleep: enter powersave mode!\n"));

    // Add a PS Exit command in Q1
	NewCmd = GetFreeCmdCtrlNode(Adapter);
	if (NewCmd)
		MakePsCmd (Adapter, NewCmd, HostCmd_SubCmd_Enter_PS);
                                       

    //dralee_1108++
    EnterCriticalSection(&Adapter->CmdQueueExeSection);

	InsertCmdToQueue(Adapter, NewCmd);    

    //dralee_1108++ 
    LeaveCriticalSection(&Adapter->CmdQueueExeSection); 


	GetCmdFromQueueToExecute (Adapter);	

	return;
}


// should change this to inline later
// acknowledge the sleep from the FW
VOID
PSConfirmSleep(
	IN PMRVDRV_ADAPTER Adapter
	)
{
	CmdCtrlNode *NewCmd  = NULL;  

                              
	
	DBGPRINT(DBG_CMD | DBG_PS | DBG_CRLF ,(L"PSConfirmSleep: confirm sleep\n"));
    
    // Add a PS Exit command in Q1
	NewCmd = GetFreeCmdCtrlNode(Adapter);
	if (NewCmd)
		MakePsCmd (Adapter, NewCmd, HostCmd_SubCmd_Sleep_Confirmed);
                               
    //dralee_1108++
    EnterCriticalSection(&Adapter->CmdQueueExeSection);

	InsertCmdToQueue(Adapter, NewCmd);

    //dralee_1108++ 
    LeaveCriticalSection(&Adapter->CmdQueueExeSection); 

	GetCmdFromQueueToExecute (Adapter);	
	return;
}

VOID
MakePsCmd(IN PMRVDRV_ADAPTER Adapter,
	      IN CmdCtrlNode *pTempCmd,
	      IN USHORT SubCmd)
{			   
	PHostCmd_DS_802_11_PS_MODE pCmdPtr;

	if (!pTempCmd)
		return;
	
	pCmdPtr = (PHostCmd_DS_802_11_PS_MODE)(pTempCmd->BufVirtualAddr);
	
	pCmdPtr->Command = HostCmd_CMD_802_11_PS_MODE;
	pCmdPtr->Size    = (USHORT)(sizeof(HostCmd_DS_802_11_PS_MODE));
	Adapter->SeqNum += 1;
	pCmdPtr->SeqNum  = Adapter->SeqNum;
	pCmdPtr->Result  = HostCmd_OPTION_USE_INT;
	pCmdPtr->SubCommand = SubCmd;
	              
    //dralee_20061115
	pCmdPtr->NullPktInterval  = (USHORT)Adapter->NullPktInterval;
	if (SubCmd == HostCmd_SubCmd_Enter_PS)
    {
		pCmdPtr->PSNumDtims=(USHORT)Adapter->MultipleDTim;
	}
    else
    {
		pCmdPtr->PSNumDtims=0;
	}
    pCmdPtr->reserved2 = 0;  

    pCmdPtr->LocalListenInterval = (USHORT)Adapter->LocalListenInterval;  

    pCmdPtr->AdhocAwakePeriod = (USHORT)Adapter->AdhocAwakePeriod;

	pTempCmd->ExpectedRetCode	= GetExpectedRetCode(HostCmd_CMD_802_11_PS_MODE);
	pTempCmd->Next				= NULL;
	pTempCmd->PendingOID		= 0;
	pTempCmd->PendingInfo		= HostCmd_PENDING_ON_NONE;
	pTempCmd->INTOption			= HostCmd_OPTION_USE_INT;
	pTempCmd->PriorityPass			= 0;
	pTempCmd->IsLastBatchCmd	= FALSE;
	pTempCmd->BytesWritten		= NULL;
	pTempCmd->BytesRead			= NULL;
	pTempCmd->BytesNeeded		= NULL;
	pTempCmd->InformationBuffer = NULL;

	return;
}

IF_API_STATUS If_PowerUpDevice(PMRVDRV_ADAPTER pAdapter)
{
    IF_API_STATUS       result = IF_SUCCESS;

    if( pAdapter->PowerBitFlag == 1 )
        goto FuncEnd;
 
    pAdapter->PowerBitFlag = 1;
    result = sdio_PowerUpDevice(pAdapter);

FuncEnd:
    return result;
}

IF_API_STATUS If_ClearPowerUpBit(PMRVDRV_ADAPTER pAdapter)
{
    IF_API_STATUS       result = IF_SUCCESS;
    if( pAdapter->PowerBitFlag == 0) {
        goto FuncFinal;
    }
    pAdapter->PowerBitFlag =0;
    result = sdio_ClearPowerUpBit(pAdapter);

FuncFinal:
    return result;
}

IF_API_STATUS If_WaitFWPowerUp(PMRVDRV_ADAPTER Adapter)
{
    ULONG   cnt = 0; 

    while(Adapter->PowerBitFlag && ++cnt < 10)
    {
        NdisMSleep(50000);
        DBGPRINT(DBG_ERROR, (L"Waiting FW awaked..\r\n"));
    }  
    
    if(Adapter->PowerBitFlag)
    {
        DBGPRINT(DBG_ERROR,(L"Power up FW may fail...\r\n"));
        return IF_FAIL;
    }

    return  IF_SUCCESS;
}


