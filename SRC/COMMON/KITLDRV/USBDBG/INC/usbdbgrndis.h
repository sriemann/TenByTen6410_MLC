//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement,
// you are not authorized to use this sample source code.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef __USBDBGRNDIS_H_
#define __USBDBGRNDIS_H_

//
//  Interface: OAL_KITL_ETH_DRIVER interface to UsbDbgRndisMdd
//
BOOL Rndis_Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
void Rndis_DeInit();
UINT16 Rndis_SendFrame(UINT8 *pData, UINT32 size);
UINT16 Rndis_RecvFrame(LPBYTE pbOutBuffer, PUSHORT pcbOutBufSize);
void Rndis_EnableInts();
void Rndis_DisableInts();
void Rndis_CurrentPktFilter(UINT32 filter);
void Rndis_PowerOff();
void Rndis_PowerOn();

#define OAL_KITLDRV_USBRNDIS  { \
    Rndis_Init, \
    NULL, \
    NULL, \
    Rndis_SendFrame, \
    Rndis_RecvFrame, \
    Rndis_EnableInts, \
    Rndis_DisableInts, \
    Rndis_PowerOff, \
    Rndis_PowerOn, \
    Rndis_CurrentPktFilter, \
    NULL \
}
#endif
