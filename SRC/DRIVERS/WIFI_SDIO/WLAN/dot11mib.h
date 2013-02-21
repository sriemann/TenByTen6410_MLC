/******************* (c) Marvell Semiconductor, Inc., 2004 ********************
 *
 *
 *  Purpose:    802.11 data structure
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/12/15 $
 *
 *	$Revision: #2 $
 *
 *****************************************************************************/

#ifndef _DOT11MIB_H
#define _DOT11MIB_H

typedef struct _WPA_NOTIFY_OS
{
    //NDIS_802_11_STATUS_INDICATION   status;
    NDIS_802_11_STATUS_TYPE             status;
    NDIS_802_11_AUTHENTICATION_REQUEST  request;
} WPA_NOTIFY_OS, *PWPA_NOTIFY_OS;

#endif // #ifndef _DOT11MIB_H

