//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __SAMSUNG_SYSLSI_APDEV_DATA_BUF_H__
#define __SAMSUNG_SYSLSI_APDEV_DATA_BUF_H__

#include "MfcTypes.h"

#ifdef __cplusplus
extern "C" {
#endif


BOOL MfcDataBufMemMapping(void);

volatile unsigned char *GetDataBufVirAddr(void);
volatile unsigned char *GetFramBufVirAddr(void);
unsigned int GetDataBufPhyAddr(void);
unsigned int GetFramBufPhyAddr(void);


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_DATA_BUF_H__ */
