/******************* (c) Marvell Semiconductor, Inc.,  ************************
 * File: power.h
 * Purpose:
 *      The protocol type of power.c
 *          - IEEE PS mode
 *          - System Power Handle
 *
 *
 *****************************************************************************/
#ifndef POWER_H
#define POWER_H
#ifdef __cplusplus
extern "C" {
#endif ///__cplusplus

#include "precomp.h"

///
///Functions for IEEE Power Save 
///
VOID PSWakeup(IN PMRVDRV_ADAPTER Adapter);
VOID PSSleep(IN PMRVDRV_ADAPTER Adapter);
VOID PSConfirmSleep(	IN PMRVDRV_ADAPTER Adapter);
VOID MakePsCmd(IN PMRVDRV_ADAPTER Adapter,
	IN CmdCtrlNode *pTempCmd,
	IN USHORT SubCmd
	);

///==========================================================
///
///Functions for system power status
///
IF_API_STATUS If_PowerUpDevice(PMRVDRV_ADAPTER pAdapter);
IF_API_STATUS If_ClearPowerUpBit(PMRVDRV_ADAPTER pAdapter);
IF_API_STATUS If_WaitFWPowerUp(PMRVDRV_ADAPTER Adapter);


#ifdef __cplusplus
}
#endif ///MRVL_ROAMING
#endif ///POWER_H

