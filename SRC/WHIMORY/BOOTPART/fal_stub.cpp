//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#include "fallite.h"

BOOL bExec = FALSE;

// --------------------------------------------------------------------
// --------------------------------------------------------------------
BOOL FAL_LockFlashRegion (REGION_TYPE regionType)	// normal booting
{
	return TRUE;	
}
// --------------------------------------------------------------------
// --------------------------------------------------------------------

BOOL FAL_UnlockFlashRegion (REGION_TYPE regionType)	//updating
{
    UINT32  nVol = 0;

    RETAILMSG (1, (L"FAL_UnlockFlashRegion++\r\n"));
#if 0
	if(!bExec) {
	    INT32   nErr;
	    UINT32  nBytesReturned;

		nErr = BML_IOCtl(nVol,
	                     BML_IOCTL_UNLOCK_WHOLEAREA,
	                     NULL, 0,
	                     NULL, 0,
	                     &nBytesReturned);
		if (nErr != BML_SUCCESS)
		{
	        RETAILMSG (1, (L"BML_IOCtl is failed\r\n"));
	        return FALSE;
		}	                     
		bExec = TRUE;
	    return TRUE;
	}
	return FALSE;	    
#else
    // do not anything... hmseo-061027
    return TRUE;
#endif
}

