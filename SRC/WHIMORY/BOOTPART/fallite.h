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

#ifndef __FALLITE_H__
#define __FALLITE_H__

#include <fmd.h>
#include <fls.h>

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

//PVOID FAL_Init (PVOID hFMD);
//VOID FAL_Deinit(PVOID hFAL);
//BOOL FAL_ReadSectors(PVOID hFAL, SECTOR_ADDR sectorLogical, LPBYTE pSectorBuff, DWORD dwSectorCount);
//BOOL FAL_WriteSectors(PVOID hFAL, SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, DWORD dwSectorCount);
//BOOL FAL_GetInfo (PVOID hFAL, DISK_INFO *pdi);
BOOL FAL_LockFlashRegion (REGION_TYPE regionType);
BOOL FAL_UnlockFlashRegion (REGION_TYPE regionType);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif __FALLITE_H__

