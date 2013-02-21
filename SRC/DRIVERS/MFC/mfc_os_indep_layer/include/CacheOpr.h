//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_CACHE_OPR_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_CACHE_OPR_H__

// JYSHIN 20080723
#if __cplusplus
extern "C" {
#endif

void CleanInvalidateCacheRange(PBYTE StartAddress, PBYTE EndAddress);
void CleanCacheRange(PBYTE StartAddress, PBYTE EndAddress);
void InvalidateCacheRange(PBYTE StartAddress, PBYTE EndAddress);

#if __cplusplus
}
#endif

#endif
