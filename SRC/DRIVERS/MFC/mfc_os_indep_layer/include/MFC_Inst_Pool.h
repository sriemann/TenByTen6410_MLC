//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_INST_POOL_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_INST_POOL_H__


#ifdef __cplusplus
extern "C" {
#endif


int MfcInstPool_NumAvail(void);

int MfcInstPool_Occupy(void);
int MfcInstPool_Release(int instance_no);

void MfcInstPool_OccupyAll(void);
void MfcInstPool_ReleaseAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_INST_POOL_H__ */
