/******************* (c) Marvell Semiconductor, Inc., *************************
 *
 *          Purpose:
 *
 *          This file includes all the necessary header files for the MrvDrv NIDS driver
 *
 *          Notes:
 *
 *			$Author: yyi $
 *
 *			$Date: 2004/12/09 18:35:17 $
 *
 *			$Revision: 1.6 $
 *
 *****************************************************************************/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

/*
===============================================================================
            INCLUDE FILES
===============================================================================
*/
#include <ndis.h>

//
//Soujanya 1/14/2003
//
//To print traces in Debugger enable the define MRVL_USE_DBG. This will print the 
// Traces both in release as well as checked builds.
//
//

#ifdef DBG
#define MRVL_USE_DBG        // Turn on debug info
#endif // #ifdef DBG

#define fNDIS_GUID_ALLOW_READ       0x00000020
#define fNDIS_GUID_ALLOW_WRITE      0x00000040


#include "ndisdef.h"      // 802.11-related NDIS definitions needed in NDIS 5.0
#include "dsdef.h"        // Data structure definition
#include "macrodef.h"     // Macro definition
#include "wmm.h"
#include "ixstatus.h"	  // IGX status return codes.
#include "macreg.h"       // SDIO & CF MAC registers
#include "procdef.h"      // NDIS Miniport procedure definition
#include "hostcmd.h"      // Host command
#include "oid.h"          // common OID header between driver and application
#include "eagledev.h"     // Device Object definition
#include "IgxBug.h"        // debug macros
#include "operfunc.h"     // MrvDrv operation functions  definition
#include "If.h"
#include <cardserv.h>

#include "power.h"


#include "wlan_ccx.h"

#endif
