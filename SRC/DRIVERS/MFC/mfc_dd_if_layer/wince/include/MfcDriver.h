//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__

DWORD MFC_Init(DWORD dwContext);
BOOL  MFC_Deinit(DWORD InitHandle);

DWORD MFC_Open(DWORD InitHandle, DWORD dwAccess, DWORD dwShareMode);
BOOL  MFC_Close(DWORD OpenHandle);

BOOL  MFC_IOControl(DWORD OpenHandle, DWORD dwIoControlCode,
                    PBYTE pInBuf, DWORD nInBufSize, PBYTE pOutBuf,
                    DWORD nOutBufSize,
                    PDWORD pBytesReturned);


#define MFC_THREAD_PRIORITY_DEFAULT    100

#define MFC_INIT_SUCCESS   0x9224033

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__ */
