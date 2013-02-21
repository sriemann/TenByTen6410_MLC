//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_MUTEX_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_MUTEX_H__


#include "MfcTypes.h"


#ifdef __cplusplus
extern "C" {
#endif


BOOL MFC_Mutex_Create(void);
void MFC_Mutex_Delete(void);
BOOL MFC_Mutex_Lock(void);
BOOL MFC_Mutex_Release(void);


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_MUTEX_H__ */
