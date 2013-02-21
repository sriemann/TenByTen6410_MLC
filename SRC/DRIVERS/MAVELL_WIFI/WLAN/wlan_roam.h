/******************* (c) Marvell Semiconductor, Inc., 2006 ********************
 * File: wlan_roam.h
 * Purpose:
 *      This file contains definitions and data structures for roaming
 *
 *
 *****************************************************************************/
#ifndef _WLAN_ROAM_H
#define _WLAN_ROAM_H
#ifdef __cplusplus
extern "C" {
#endif ///__cplusplus
#include "precomp.h"

///==============================================
/// Reconnection 
///
typedef struct _reconnect_arg
{
    HANDLE                      hroamEvent;
    ///HANDLE                      PendForRoamEvent;
    BOOLEAN                     FastRoamProgressing;
    //BOOLEAN                     skipDeauth;
    BOOLEAN                     privacyEnabled;
    BOOLEAN                     cckmEnabled;
} RECONNECTARG, *PRECONNECTARG;

typedef struct _ap_info
{
	int			id;							///This AP is at the ith of the table
	NDIS_802_11_MAC_ADDRESS	MacAddr;		///MAC address of this AP
} APINFO, *PAPINFO;


VOID wlan_reconnect_init(PRECONNECTARG  pReconArg);
VOID wlan_reconnect(PRECONNECTARG  pReconArg, PMRVDRV_ADAPTER pAdapter, PAPINFO pApInfo);
VOID wlan_reconnect_deinit(PRECONNECTARG  pReconArg);
VOID wlan_recnnect_set_ap_param(PRECONNECTARG  pReconArg, BOOLEAN isPrivacy, BOOLEAN isCCKM);

typedef struct _wroam_param* PWROAMPARAM;

///State of the roaming module
typedef enum
{
	WRS_NOT_CONNECT=0,	///=> If UUT is Not connected.
	WRS_STABLE,			///=> If UUT has connected to an AP and signal strength is good
	WRS_ROAM,				///=> If UUT is connected, but the AP signal is not so good
	WRS_MAX_STATE		/// => Indicate this is the maximum value
} WROAM_STATE;

typedef enum
{
	WRSCAN_BGSCAN,		/// Use background scan 
	WRSCAN_NORMALSCAN,	/// Use normal scan
	WRSCAN_MAX
} WROAM_SCAN_ALGORITHM;

typedef struct _wroam_extparam
{
	DWORD				RoamSST;		///Roaming Signal Strength Threshold
	NDIS_802_11_SSID      ScanSSID;		///The SSID we only care about

	WORD				ChannelList;		///Scanned channel list. 
										///bit[i] = 0, channel[i] is cleared
										///bit[i] = 1, channel[i] is selected

	DWORD				MaxScanInterval, MinScanInterval;	///Scan interval per channel

	DWORD				diffRSSI;		///If the new AP's RSSI is higher than this value, roam to that
	DWORD				ScanAlgorithm;	///Default scan algorithm
	
} WROAMEXTPARAM, *PWROAMEXTPARAM;

///Initialize the roaming module
PWROAMPARAM wlan_roam_init(PMRVDRV_ADAPTER , PWROAMEXTPARAM);
///Deinitialize the roaming module
void wlan_roam_deinit(PWROAMPARAM);

///Transite to the different state
void wlan_roam_set_state(PWROAMPARAM, WROAM_STATE);

///Query if we are in connecting status
BOOLEAN wlan_roam_query_isConnecting(PWROAMPARAM);

//To inform the roaming module that the scanning is completed
VOID	wlan_roam_ScanTableReady(PWROAMPARAM, WROAM_SCAN_ALGORITHM);

VOID	wlan_roam_SetScanAlgorithm(PWROAMPARAM, WROAM_SCAN_ALGORITHM);

PRECONNECTARG   wlan_roam_GetReconnectArg(PWROAMPARAM);

#ifdef __cplusplus
}
#endif ///__cplusplus
#endif ///_WLAN_ROAM_H

