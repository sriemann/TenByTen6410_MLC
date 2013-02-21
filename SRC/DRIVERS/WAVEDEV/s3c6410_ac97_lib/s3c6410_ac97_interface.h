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

#ifndef __S3C6410_AC97_INTERFACE_H__
#define __S3C6410_AC97_INTERFACE_H__

#if __cplusplus
extern "C"
{
#endif


// AC_GLBSTAT
typedef enum
{
    CODEC_READY_INT_PEND = (1<<22),
    PCMOUT_UR_INT_PEND = (1<<21),
    PCMIN_OR_INT_PEND     = (1<<20),
    MICIN_OR_INT_PEND = (1<<19),
    PCMOUT_THOLD_INT_PEND = (1<<18),
    PCMIN_THOLD_INT_PEND     = (1<<17),
    MICIN_THOLD_INT_PEND = (1<<16),
    CONTROLLER_STAT_IDLE = (0<<0),
    CONTROLLER_STAT_INIT = (1<<0),
    CONTROLLER_STAT_READY = (2<<0),
    CONTROLLER_STAT_ACTIVE = (3<<0),
    CONTROLLER_STAT_LP = (4<<0),
    CONTROLLER_STAT_WARM = (5<<0),
    CONTROLLER_STAT_MASK = (7<<0)
} AC97_STATUS;


typedef enum
{
    AC97_CH_OFF,
    AC97_CH_PIO,
    AC97_CH_DMA
} AC97_CHANNEL_MODE;


typedef enum
{
    AC97_SUCCESS,
    AC97_ERROR_NULL_PARAMETER,
    AC97_ERROR_ILLEGAL_PARAMETER,
    AC97_ERROR_NOT_INITIALIZED,
    AC97_ERROR_NOT_IMPLEMENTED,
    AC97_ERROR_XXX
} AC97_ERROR;


AC97_ERROR AC97_initialize_register_address(void *pAC97Reg, void *pGPIOReg);
void AC97_initialize_ACLink(void);
void AC97_enable_ACLink_data_transfer(void);
void AC97_disable_ACLink_data_transfer(void);
BOOL AC97_wait_for_codec_ready(void);
AC97_ERROR AC97_set_pcmout_transfer_mode(AC97_CHANNEL_MODE eMode);
AC97_ERROR AC97_set_pcmin_transfer_mode(AC97_CHANNEL_MODE eMode);
AC97_ERROR AC97_set_micin_transfer_mode(AC97_CHANNEL_MODE eMode);
void AC97_write_codec(unsigned char ucReg, unsigned short usData);
unsigned short AC97_read_codec(unsigned char ucReg);

void AC97_enable_all_interrupt(void);
void AC97_enable_codec_ready_interrupt(void);
void AC97_enable_pcmout_underrun_interrupt(void);
void AC97_enable_pcmin_overrun_interrupt(void);
void AC97_enable_micin_overrun_interrupt(void);
void AC97_enable_pcmout_threshold_interrupt(void);
void AC97_enable_pcmin_threshold_interrupt(void);
void AC97_enable_micin_threshold_interrupt(void);
void AC97_disable_all_interrupt(void);
void AC97_disable_codec_ready_interrupt(void);
void AC97_disable_pcmout_underrun_interrupt(void);
void AC97_disable_pcmin_overrun_interrupt(void);
void AC97_disable_micin_overrun_interrupt(void);
void AC97_disable_pcmout_threshold_interrupt(void);
void AC97_disable_pcmin_threshold_interrupt(void);
void AC97_disable_micin_threshold_interrupt(void);
void AC97_clear_all_interrupt(void);
void AC97_clear_codec_ready_interrupt(void);
void AC97_clear_pcmout_underrun_interrupt(void);
void AC97_clear_pcmin_overrun_interrupt(void);
void AC97_clear_micin_overrun_interrupt(void);
void AC97_clear_pcmout_threshold_interrupt(void);
void AC97_clear_pcmin_threshold_interrupt(void);
void AC97_clear_micin_threshold_interrupt(void);

unsigned int AC97_get_pcmout_physical_buffer_address(void);
unsigned int AC97_get_pcmin_physical_buffer_address(void);
unsigned int AC97_get_micin_physical_buffer_address(void);

static void AC97_port_initialize(void);
static void AC97_cold_reset(void);
static void AC97_warm_reset(void);
static void AC97_set_ACLink_On(void);
static void AC97_set_ACLink_Off(void);
static AC97_STATUS AC97_get_status(void);

static void DelayLoop(unsigned int count);

#if __cplusplus
}
#endif

#endif    // __S3C6410_AC97_INTERFACE_H__
