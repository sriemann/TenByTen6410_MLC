//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
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
