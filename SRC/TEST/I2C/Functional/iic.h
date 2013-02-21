//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/**************************************************************************************
* 
*    Project Name : IIC Driver 
*
*    Copyright 2006 by Samsung Electronics, Inc.
*    All rights reserved.
*
*    Project Description :
*        This heder file is for interfacing IIC driver and the other drivers. 
*  
*--------------------------------------------------------------------------------------
* 
*    File Name : iic.h
*  
*    File Description : This heder file is for interfacing IIC driver and the other drivers.
*
*    Author : JeGeon.Jung
*    Dept. : AP Development Team
*    Created Date : 2007/06/12
*    Version : 0.1 
* 
*    History
*    - Created(JeGeon.Jung 2007/06/12)
*  
*    Todo
*
*
*    Note
*
**************************************************************************************/
#ifndef __IIC_H__
#define __IIC_H__



enum    IIC_MODE
{
    Slave_receive = 0,
    Slave_transmit = 1,
    Master_receive = 2,
    Master_transmit = 3
};

enum    IIC_DELAY
{
    Clk_0 = 0,
    Clk_5 = 1,
    Clk_10 = 2,
    Clk_15 = 3
};


//
// I/O DESCRIPTOR for Read/Write
//
typedef struct _IIC_IO_DESC {
    UCHAR      SlaveAddress;               // Slave Address
    PUCHAR  Data;               // pBuffer
    DWORD   Count;              // nBytes to read/write
} IIC_IO_DESC, *PIIC_IO_DESC;

//
// I2C Bus Driver IOCTLS
//
#define FILE_DEVICE_IIC     FILE_DEVICE_CONTROLLER

// IN:  PIIC_IO_DESC
#define IOCTL_IIC_READ \
    CTL_CODE(FILE_DEVICE_IIC, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  PIIC_IO_DESC
#define IOCTL_IIC_WRITE \
    CTL_CODE(FILE_DEVICE_IIC, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  UINT32* 
#define IOCTL_IIC_SET_CLOCK \
    CTL_CODE(FILE_DEVICE_IIC, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  UINT32* 
#define IOCTL_IIC_GET_CLOCK \
    CTL_CODE(FILE_DEVICE_IIC, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  UINT32* 
#define IOCTL_IIC_SET_MODE \
    CTL_CODE(FILE_DEVICE_IIC, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  UINT32* 
#define IOCTL_IIC_GET_MODE \
    CTL_CODE(FILE_DEVICE_IIC, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  UINT32* 
#define IOCTL_IIC_SET_FILTER \
    CTL_CODE(FILE_DEVICE_IIC, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  UINT32* 
#define IOCTL_IIC_GET_FILTER \
    CTL_CODE(FILE_DEVICE_IIC, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
    
// IN:  UINT32* 
#define IOCTL_IIC_SET_DELAY \
    CTL_CODE(FILE_DEVICE_IIC, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IN:  UINT32* 
#define IOCTL_IIC_GET_DELAY \
    CTL_CODE(FILE_DEVICE_IIC, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif //    __IIC_H__
