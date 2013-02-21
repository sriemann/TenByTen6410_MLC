//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------

//
// s3c6410_ac97_interface_macro.h

#ifndef _S3C6410_AC97_INTERFACE_MACRO_H_
#define _S3C6410_AC97_INTERFACE_MACRO_H_

#if __cplusplus
extern "C"
{
#endif

// AC_GLBCTRL
#define CODEC_READY_INT_CLEAR                (1<<30)
#define PCMOUT_UR_INT_CLEAR                (1<<29)
#define PCMIN_OR_INT_CLEAR                    (1<<28)
#define MICIN_OR_INT_CLEAR                    (1<<27)
#define PCMOUT_THOLD_INT_CLEAR            (1<<26)
#define PCMIN_THOLD_INT_CLEAR                (1<<25)
#define MICIN_THOLD_INT_CLEAR                (1<<24)
#define ALL_INT_CLEAR_MASK                    (0x3f<<24)
#define CODEC_READY_INT_ENABLE            (1<<22)
#define PCMOUT_UR_INT_ENABLE                (1<<21)
#define PCMIN_OR_INT_ENABLE                (1<<20)
#define MICIN_OR_INT_ENABLE                (1<<19)
#define PCMOUT_THOLD_INT_ENABLE            (1<<18)
#define PCMIN_THOLD_INT_ENABLE            (1<<17)
#define MICIN_THOLD_INT_ENABLE            (1<<16)
#define ALL_INT_ENABLE_MASK                (0x3f<<16)
#define PCMOUT_TRANSFER_OFF                (0<<12)
#define PCMOUT_TRANSFER_PIO                (1<<12)
#define PCMOUT_TRANSFER_DMA                (2<<12)
#define PCMOUT_TRANSFER_RESERVED            (3<<12)
#define PCMOUT_TRANSFER_MODE_MASK        (3<<12)
#define PCMIN_TRANSFER_OFF                (0<<10)
#define PCMIN_TRANSFER_PIO                    (1<<10)
#define PCMIN_TRANSFER_DMA                (2<<10)
#define PCMIN_TRANSFER_RESERVED            (3<<10)
#define PCMIN_TRANSFER_MODE_MASK            (3<<10)
#define MICIN_TRANSFER_OFF                    (0<<9)
#define MICIN_TRANSFER_PIO                    (1<<9)
#define MICIN_TRANSFER_DMA                (2<<9)
#define MICIN_TRANSFER_RESERVED            (3<<9)
#define MICIN_TRANSFER_MODE_MASK            (3<<9)
#define ACLINK_DATA_TRANSFER_DISABLE        (0<<3)
#define ACLINK_DATA_TRANSFER_ENABLE        (1<<3)
#define ACLINK_OFF                            (0<<2)
#define ACLINK_ON                            (1<<2)
#define WARM_RESET                            (1<<1)
#define COLD_RESET                            (1<<0)



// AC_CODEC_CMD
#define CMD_WRITE                            (0<<23)
#define CMD_READ                            (1<<23)
#define CMD_ADDRESS(n)                        (((n)&0x7f)<<16)
#define CMD_DATA(n)                            ((n)&0xffff)

// AC_CODEC_STAT
#define STATUS_ADDRESS(n)                    (((n)>>16)&0x7f)
#define STATUS_DATA(n)                        ((n)&0xffff)

// AC_PCMADDR
#define PCMOUT_FIFO_READ_ADDR(n)            (((n)>>24)&0xf)
#define PCMIN_FIFO_READ_ADDR(n)            (((n)>>16)&0xf)
#define PCMOUT_FIFO_WRITE_ADDR(n)            (((n)>>8)&0xf)
#define PCMIN_FIFO_WRITE_ADDR(n)            ((n)&0xf)

// AC_MICADDR
#define MICIN_FIFO_READ_ADDR(n)            (((n)>>16)&0xf)
#define MICIN_FIFO_WRITE_ADDR(n)            ((n)&0xf)

// AC_PCMDATA
#define RIGHT_DATA_W(n)                    (((n)&0xffff)<<16)
#define LEFT_DATA_W(n)                        ((n)&0xffff)
#define RIGHT_DATA_R(n)                        (((n)>>16)&0xffff)
#define LEFT_DATA_R(n)                        ((n)&0xffff)

// AC_MICDATA
#define MONO_DATA(n)                        ((n)&0xffff)

#if __cplusplus
    }
#endif

#endif    // _S3C6410_AC97_INTERFACE_MACRO_H_
