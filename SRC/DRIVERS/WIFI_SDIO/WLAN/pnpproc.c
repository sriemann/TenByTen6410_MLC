/******************* ?Marvell Semiconductor, Inc., *******************************
 *
 *  Purpose:    This module provides PnP and Power management routine
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


/*
===============================================================================
                           CODED PUBLIC PROCEDURES
===============================================================================
*/

/******************************************************************************
 *
 *  Name: MrvDrvPnPEventNotify()
 *
 *  Description: 
 *
 *  Conditions for Use: NDIS wrapper will call MrvDrvPnPEventNotify() to notify PnP event
 *						WINXP specific function
 *  Arguments:
 *
 *    IN NDIS_HANDLE  MiniportAdapterContext,
 *    IN NDIS_DEVICE_PNP_EVENT  PnPEvent,
 *    IN PVOID  InformationBuffer,
 *    IN ULONG  InformationBufferLength
 *    
 *  Return Value: none
 * 
 *  Notes:   
 *
 *****************************************************************************/
VOID
MrvDrvPnPEventNotify(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN NDIS_DEVICE_PNP_EVENT  PnPEvent,
    IN PVOID  InformationBuffer,
    IN ULONG  InformationBufferLength
)
{
    PMRVDRV_ADAPTER Adapter;
    NDIS_POWER_PROFILE PWProfile;
    BOOLEAN timerStatus;

     /// add for test 
    return;

    Adapter = (PMRVDRV_ADAPTER)(MiniportAdapterContext);

    switch( PnPEvent )
    {
        case NdisDevicePnPEventSurpriseRemoved:
            DBGPRINT(DBG_PNP,(L"PNP - Got NdisDevicePnPEventSurpriseRemoved event !!! \n"));
            Adapter->SurpriseRemoved = TRUE;

            // We should disable bus interrupt
            If_DisableInterrupt(Adapter);

            // Reset Tx and command queue (Will be called again in CondorHalt())
            CleanUpSingleTxBuffer(Adapter);
            ResetRxPDQ(Adapter);
            ResetCmdBuffer(Adapter);
        
			
            if(Adapter->TxPktTimerIsSet==TRUE){		
                NdisMCancelTimer(&Adapter->MrvDrvTxPktTimer, &timerStatus);
                Adapter->TxPktTimerIsSet=FALSE;
            }
            Adapter->TxPktTimerIsSet=FALSE;
            NdisMCancelTimer(&Adapter->MrvDrvCommandTimer, &timerStatus);
            DBGPRINT(DBG_PNP,(L"**** PNP - Finished NdisDevicePnPEventSurpriseRemoved ****\n"));
            break;

        case NdisDevicePnPEventPowerProfileChanged:
            PWProfile = *((PNDIS_POWER_PROFILE)InformationBuffer);
            if ( PWProfile == NdisPowerProfileBattery )
            {
                DBGPRINT(DBG_PNP,(L"PNP - Power profile has been changed to NdisPowerProfileBattery !!! \n"));
                // only set TX power if OS/App did not set the desired power
                if ( Adapter->TxPowerLevelIsSetByOS == FALSE )
                {
                    Adapter->CurrentTxPowerLevel = HostCmd_ACT_TX_POWER_INDEX_MID;
                    // Set Tx power to battery profile
                    PrepareAndSendCommand(
                                                            Adapter,
                                                            HostCmd_CMD_802_11_RF_TX_POWER,
                                                            HostCmd_ACT_TX_POWER_OPT_SET_MID,
                                                            HostCmd_OPTION_USE_INT,
                                                            0,
                                                            HostCmd_PENDING_ON_NONE,
                                                            0,
                                                            FALSE,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL);
                }
            }
            else
            { 
                DBGPRINT(DBG_PNP,(L"PNP - Power profile has been changed to NdisPowerProfileAcOnLine !!! \n"));
                // only set TX power if OS/App did not set the desired power
                if ( Adapter->TxPowerLevelIsSetByOS == FALSE )
                {
                    USHORT usValue;
                    usValue = 17;
                    Adapter->CurrentTxPowerLevel = HostCmd_ACT_TX_POWER_INDEX_HIGH;
                }
            }
            break;
    }
    return;
}

