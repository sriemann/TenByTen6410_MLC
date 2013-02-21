//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//------------------------------------------------------------------------------
//
//  Header: s3c6410_spi.h
//
//  Defines the Serial Peripheral Interface (SPI) controller CPU register layout and
//  definitions.
//
#ifndef __S3C6410_SPI_H
#define __S3C6410_SPI_H

#if __cplusplus
    extern "C"
    {
#endif

#include <winioctl.h>


// IOCTL Commands
#define    SPI_IOCTL_SET_CONFIG         CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    SPI_IOCTL_START              CTL_CODE(FILE_DEVICE_SERIAL_PORT, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define    SPI_MASTER_MODE      1
#define    SPI_SLAVE_MODE       0


typedef struct {
    PVOID                VirtualAddress;
    PVOID               PhysicalAddress;
} DMA_BUFFER, *PDMA_BUFFER;

typedef struct {
    DWORD                   dwMode;

    BOOL                    bUseFullDuflex;

    DWORD                   dwRxBurstDataLen;
    BOOL                    bUseRxDMA;
    BOOL                    bUseRxIntr;

    DWORD                   dwTxBurstDataLen;
    BOOL                    bUseTxDMA;
    BOOL                    bUseTxIntr;

    DWORD                   dwPrescaler;
    DWORD                   dwTimeOutVal;
} SET_CONFIG, *PSET_CONFIG;



//------------------------------------------------------------------------------

#if __cplusplus
       }
#endif

#endif    // __S3C6410_SPI_H
