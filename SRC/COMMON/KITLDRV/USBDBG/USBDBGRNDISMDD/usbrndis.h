//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft
// premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license
// agreement, you are not authorized to use this source code.
// For the terms of the license, please see the license agreement
// signed by you and Microsoft.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#ifndef __USBRNDIS_H_
#define __USBRNDIS_H_

//
//  Interface: USB RNDIS interface to RNDISMin
//
BOOL UsbRndis_Init(UINT16 mac[3]);
VOID UsbRndis_Deinit(void);
UINT32 UsbRndis_EventHandler();
void UsbRndis_SendMessage(PBYTE rndisMsg, UINT32 rndisMsgLen);
void RndisPdd_RecvdMsg(PBYTE pbData, UINT32 cbData);
DWORD RndisPdd_ProcessRecvdData(PBYTE pbSourceBuf, DWORD cbSourceBufLen,
                             PBYTE pbTargetBuf, DWORD cbTargetBufLen);
void RndisPdd_SetRndisState(BOOL notInit);
DWORD UsbRndis_RecvData(PBYTE pbBuffer, DWORD cbRecvdDataLen);
DWORD UsbRndis_SendData(UINT8 *pData, UINT32 cbDataLen);
void UsbRndis_SetPower(BOOL fPowerOff);
BOOL UsbRndis_RndisMsgSent();

#define MANUFACTURER    L"Microsoft"
#define PRODUCT         L"Microsoft USBDBGRNDIS for WindowsCE Devices"

#endif
