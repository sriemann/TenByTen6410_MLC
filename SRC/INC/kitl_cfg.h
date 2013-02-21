//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#ifndef __KITL_CFG_H
#define __KITL_CFG_H

//------------------------------------------------------------------------------
#include <bsp.h>
#include <oal_kitl.h>
#include <oal_ethdrv.h>
#include <s3c6410_base_regs.h>
#include <usbdbgser.h>
#include <usbdbgrndis.h>


//------------------------------------------------------------------------------
// KITL Devices
//------------------------------------------------------------------------------

OAL_KITL_ETH_DRIVER g_kitlEthDM9000A    = OAL_ETHDRV_CS8900A;
OAL_KITL_ETH_DRIVER g_kitlEthUsbRndis   = OAL_KITLDRV_USBRNDIS;
OAL_KITL_SERIAL_DRIVER g_kitlUsbSerial  = OAL_KITLDRV_USBSERIAL;


OAL_KITL_DEVICE g_kitlDevices[] = {
    { 
        L"6410Ethernet", Internal, BSP_BASE_REG_PA_DM9000A_IOBASE, 0, OAL_KITL_TYPE_ETH, 
        &g_kitlEthDM9000A
    },
    { 
        L"6410USBSerial", Internal, S3C6410_BASE_REG_PA_USBOTG_LINK, 0, OAL_KITL_TYPE_SERIAL, 
        &g_kitlUsbSerial
    },
    {
        L"6410USBRndis", InterfaceTypeUndefined, S3C6410_BASE_REG_PA_USBOTG_LINK, 0, OAL_KITL_TYPE_ETH,
        &g_kitlEthUsbRndis
    },
    {
        L"6410USBDNW", Internal, S3C6410_BASE_REG_PA_USBOTG_LINK, 0, OAL_KITL_TYPE_SERIAL,
        &g_kitlUsbSerial
    },
    {
        NULL, 0, 0, 0, 0, NULL
    }
};  

#endif

