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
#ifndef __USBDBGSERIAL_H_
#define __USBDBGSERIAL_H_

//
//  Interface: USB Serial interface to SerialIfc
//
BOOL UsbSerial_Init();
VOID UsbSerial_Deinit(void);
UINT32 UsbSerial_EventHandler();
UINT32 UsbSerial_RecvData(PBYTE pbBuffer, DWORD cbBufSize);
DWORD UsbSerial_SendData(UINT8 *pData, UINT32 cbDataLen);
void UsbSerial_SetPower(BOOL fPowerOff);
BOOL UsbSerial_IsConnected();

#define MANUFACTURER    L"Microsoft"
#define PRODUCT         L"Microsoft USBDBGSER for WindowsCE Devices"

#define RECVBUF_MAXSIZE             1536  

#endif
