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

#include <windows.h>
#include <wavedbg.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "s3c6410_ac97_interface_macro.h"
#include "s3c6410_ac97_interface.h"

#define AC97_MSG(x)
#define AC97_INF(x)    DEBUGMSG(ZONE_FUNCTION, x)
#define AC97_ERR(x)    DEBUGMSG(ZONE_ERROR, x)

#define DELAY_LOOP_COUNT    (S3C6410_ACLK/100000)
#define CONTROL_DELAY        (10)
#define CMD_DELAY            (3)

#define PCMOUT_BUFFER_PHYSICAL_ADDRESS   0x7F001018
#define PCMIN_BUFFER_PHYSICAL_ADDRESS    0x7F001018
#define MICIN_BUFFER_PHYSICAL_ADDRESS    0x7F00101C

static volatile S3C6410_AC97_REG *g_pAC97Reg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;


AC97_ERROR AC97_initialize_register_address(void *pAC97Reg, void *pGPIOReg)
{
    AC97_ERROR error = AC97_SUCCESS;

    AC97_MSG((_T("[AC97]++AC97_initialize_register_address(0x%08x, 0x%08x)\n\r"), pAC97Reg, pGPIOReg));

    if (pAC97Reg == NULL || pGPIOReg == NULL)
    {
        AC97_ERR((_T("[AC97:ERR] AC97_initialize_register_address() : NULL pointer parameter\n\r")));
        error = AC97_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pAC97Reg = (S3C6410_AC97_REG *)pAC97Reg;
        g_pGPIOReg = (S3C6410_GPIO_REG *)pGPIOReg;
        AC97_INF((_T("[AC97:INF] g_pAC97Reg = 0x%08x\n\r"), g_pAC97Reg));
        AC97_INF((_T("[AC97:INF] g_pGPIOReg = 0x%08x\n\r"), g_pGPIOReg));
    }

    AC97_MSG((_T("[AC97]--AC97_initialize_register_address() : %d\n\r"), error));

    return error;
}


void AC97_initialize_ACLink(void)
{
    AC97_MSG((_T("[AC97] ++AC97_initialize_ACLink()\n\r")));

    AC97_port_initialize();

    // Codec Chip Need Cold - Cold- Warm Reset Sequence !!!

    // Cold Reset
    AC97_cold_reset();

    // Cold Reset Again
    AC97_cold_reset();

    // Warm Reset
    AC97_warm_reset();

    // Enable Codec Ready Interrupt
    //AC97_enable_codec_ready_interrupt();

    // AC-Link On
    AC97_set_ACLink_On();

    AC97_MSG((_T("[AC97] --AC97_initialize_ACLink()\n\r")));
}


void AC97_enable_ACLink_data_transfer(void)
{
    AC97_MSG((_T("[AC97] AC97_enable_ACLink_data_transfer()\n\r")));

    g_pAC97Reg->AC_GLBCTRL |= ACLINK_DATA_TRANSFER_ENABLE;
    DelayLoop(CONTROL_DELAY);
}


void AC97_disable_ACLink_data_transfer(void)
{
    AC97_MSG((_T("[AC97] AC97_disable_ACLink_data_transfer()\n\r")));

    g_pAC97Reg->AC_GLBCTRL &= ~ACLINK_DATA_TRANSFER_ENABLE;
    DelayLoop(CONTROL_DELAY);
}


BOOL AC97_wait_for_codec_ready(void)
{
    int iCnt = 0;

    AC97_MSG((_T("[AC97] AC97_wait_for_codec_ready()\n\r")));

    // Waiting for Codec Ready
    while(!(AC97_get_status()&CODEC_READY_INT_PEND))
    {
        if (iCnt > 100)
        {
            AC97_ERR((_T("[AC97:ERR] AC97_wait_for_codec_ready() : Codec can't be initialized\n\r")));
            return FALSE;
        }

        iCnt++;
        Sleep(10);
    }

    AC97_INF((_T("[AC97:INF] AC97_wait_for_codec_ready() : Status = 0x%08x\n\r"), AC97_get_status()));

    return TRUE;
}


AC97_ERROR AC97_set_pcmout_transfer_mode(AC97_CHANNEL_MODE eMode)
{
    AC97_ERROR error = AC97_SUCCESS;

    AC97_MSG((_T("[AC97] AC97_set_pcmout_transfer_mode(%d)\n\r"), eMode));

    switch(eMode)
    {
    case AC97_CH_OFF:
        g_pAC97Reg->AC_GLBCTRL &= ~PCMOUT_TRANSFER_MODE_MASK;
        break;
    case AC97_CH_PIO:
        g_pAC97Reg->AC_GLBCTRL = (g_pAC97Reg->AC_GLBCTRL & ~(PCMOUT_TRANSFER_MODE_MASK)) | PCMOUT_TRANSFER_PIO;
        break;
    case AC97_CH_DMA:
        g_pAC97Reg->AC_GLBCTRL = (g_pAC97Reg->AC_GLBCTRL & ~(PCMOUT_TRANSFER_MODE_MASK)) | PCMOUT_TRANSFER_DMA;
        break;
    default:
        AC97_ERR((_T("[AC97:ERR] AC97_set_pcmout_transfer_mode() : Unknown Mode [%d]\n\r"), eMode));
        error = AC97_ERROR_ILLEGAL_PARAMETER;
        g_pAC97Reg->AC_GLBCTRL &= ~PCMOUT_TRANSFER_MODE_MASK;
        break;
    }

    return error;
}


AC97_ERROR AC97_set_pcmin_transfer_mode(AC97_CHANNEL_MODE eMode)
{
    AC97_ERROR error = AC97_SUCCESS;

    AC97_MSG((_T("[AC97] AC97_set_pcmin_transfer_mode(%d)\n\r"), eMode));

    switch(eMode)
    {
    case AC97_CH_OFF:
        g_pAC97Reg->AC_GLBCTRL &= ~PCMIN_TRANSFER_MODE_MASK;
        break;
    case AC97_CH_PIO:
        g_pAC97Reg->AC_GLBCTRL = (g_pAC97Reg->AC_GLBCTRL & ~(PCMIN_TRANSFER_MODE_MASK)) | PCMIN_TRANSFER_PIO;
        break;
    case AC97_CH_DMA:
        g_pAC97Reg->AC_GLBCTRL = (g_pAC97Reg->AC_GLBCTRL & ~(PCMIN_TRANSFER_MODE_MASK)) | PCMIN_TRANSFER_DMA;
        break;
    default:
        AC97_ERR((_T("[AC97:ERR] AC97_set_pcmin_transfer_mode() : Unknown Mode [%d]\n\r"), eMode));
        error = AC97_ERROR_ILLEGAL_PARAMETER;
        g_pAC97Reg->AC_GLBCTRL &= ~PCMIN_TRANSFER_MODE_MASK;
        break;
    }

    return error;
}


AC97_ERROR AC97_set_micin_transfer_mode(AC97_CHANNEL_MODE eMode)
{
    AC97_ERROR error = AC97_SUCCESS;

    AC97_MSG((_T("[AC97] AC97_set_micin_transfer_mode(%d)\n\r"), eMode));

    switch(eMode)
    {
    case AC97_CH_OFF:
        g_pAC97Reg->AC_GLBCTRL &= ~MICIN_TRANSFER_MODE_MASK;
        break;
    case AC97_CH_PIO:
        g_pAC97Reg->AC_GLBCTRL = (g_pAC97Reg->AC_GLBCTRL & ~(MICIN_TRANSFER_MODE_MASK)) | MICIN_TRANSFER_PIO;
        break;
    case AC97_CH_DMA:
        g_pAC97Reg->AC_GLBCTRL = (g_pAC97Reg->AC_GLBCTRL & ~(MICIN_TRANSFER_MODE_MASK)) | MICIN_TRANSFER_DMA;
        break;
    default:
        AC97_ERR((_T("[AC97:ERR] AC97_set_micin_transfer_mode() : Unknown Mode [%d]\n\r"), eMode));
        error = AC97_ERROR_ILLEGAL_PARAMETER;
        g_pAC97Reg->AC_GLBCTRL &= ~MICIN_TRANSFER_MODE_MASK;
        break;
    }

    return error;
}


void AC97_write_codec(unsigned char ucReg, unsigned short usData)
{
    g_pAC97Reg->AC_CODEC_CMD = CMD_WRITE | CMD_ADDRESS(ucReg) | CMD_DATA(usData);
    DelayLoop(CMD_DELAY);

    g_pAC97Reg->AC_CODEC_CMD |= CMD_READ;    //To receive SLOTREQ bits when VRA is '1'
    DelayLoop(CMD_DELAY);
}


unsigned short AC97_read_codec(unsigned char ucReg)
{
    unsigned short usVal;

    // Read Dummy
    usVal = STATUS_DATA(g_pAC97Reg->AC_CODEC_STAT);
    DelayLoop(CMD_DELAY);

    g_pAC97Reg->AC_CODEC_CMD = CMD_READ | CMD_ADDRESS(ucReg) | CMD_DATA(0x0);
    DelayLoop(CMD_DELAY);

    // Read Value
    usVal = STATUS_DATA(g_pAC97Reg->AC_CODEC_STAT);
    DelayLoop(CMD_DELAY);

    g_pAC97Reg->AC_CODEC_CMD |= CMD_READ;    //To receive SLOTREQ bits when VRA is '1'
    DelayLoop(CMD_DELAY);

    return usVal;
}


void AC97_enable_all_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= ALL_INT_ENABLE_MASK;
}


void AC97_enable_codec_ready_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= CODEC_READY_INT_ENABLE;
}


void AC97_enable_pcmout_underrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMOUT_UR_INT_ENABLE;
}


void AC97_enable_pcmin_overrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMIN_OR_INT_ENABLE;
}


void AC97_enable_micin_overrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= MICIN_OR_INT_ENABLE;
}


void AC97_enable_pcmout_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMOUT_THOLD_INT_ENABLE;
}


void AC97_enable_pcmin_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMIN_THOLD_INT_ENABLE;
}


void AC97_enable_micin_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= MICIN_THOLD_INT_ENABLE;
}


void AC97_disable_all_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~ALL_INT_ENABLE_MASK;
}


void AC97_disable_codec_ready_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~CODEC_READY_INT_ENABLE;
}


void AC97_disable_pcmout_underrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~PCMOUT_UR_INT_ENABLE;
}


void AC97_disable_pcmin_overrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~PCMIN_OR_INT_ENABLE;
}


void AC97_disable_micin_overrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~MICIN_OR_INT_ENABLE;
}


void AC97_disable_pcmout_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~PCMOUT_THOLD_INT_ENABLE;
}


void AC97_disable_pcmin_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~PCMIN_THOLD_INT_ENABLE;
}


void AC97_disable_micin_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL &= ~MICIN_THOLD_INT_ENABLE;
}


void AC97_clear_all_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= ALL_INT_CLEAR_MASK;
}


void AC97_clear_codec_ready_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= CODEC_READY_INT_PEND;
}


void AC97_clear_pcmout_underrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMOUT_UR_INT_PEND;
}


void AC97_clear_pcmin_overrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMIN_OR_INT_PEND;
}


void AC97_clear_micin_overrun_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= MICIN_OR_INT_PEND;
}


void AC97_clear_pcmout_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMOUT_THOLD_INT_PEND;
}


void AC97_clear_pcmin_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= PCMIN_THOLD_INT_PEND;
}


void AC97_clear_micin_threshold_interrupt(void)
{
    g_pAC97Reg->AC_GLBCTRL |= MICIN_THOLD_INT_PEND;
}


unsigned int AC97_get_pcmout_physical_buffer_address(void)
{
    return (unsigned int)(PCMOUT_BUFFER_PHYSICAL_ADDRESS);
}


unsigned int AC97_get_pcmin_physical_buffer_address(void)
{
    return (unsigned int)(PCMIN_BUFFER_PHYSICAL_ADDRESS);
}


unsigned int AC97_get_micin_physical_buffer_address(void)
{
    return (unsigned int)(MICIN_BUFFER_PHYSICAL_ADDRESS);
}


static void AC97_port_initialize(void)
{
    AC97_MSG((_T("[AC97] AC97_port_initialize()\n\r")));

    // AC97 BITCLK    : GPD[0], GPE[0]
    // AC97 RESETn    : GPD[1], GPE[1]
    // AC97 SYNC    : GPD[2], GPE[2]
    // AC97 SDI        : GPD[3], GPE[3]
    // AC97 SDO        : GPD[4], GPE[4]

#if    1    // AC97 Use GPD Port
    g_pGPIOReg->GPDPUD &= ~(0x3ff);    // Pull-Up/Down Disable
    g_pGPIOReg->GPDCON = 0x44444;         // GPD -> AC97
#else    // AC97 Use GPE Port
    g_pGPIOReg->GPEPUD &= ~(0x3ff);    // Pull-Up/Down Disable
    g_pGPIOReg->GPECON = 0x44444;         // GPD -> AC97
#endif
}


static void AC97_cold_reset(void)
{
    AC97_MSG((_T("[AC97] AC97_cold_reset()\n\r")));

    g_pAC97Reg->AC_GLBCTRL |= COLD_RESET;
    DelayLoop(CONTROL_DELAY);
    g_pAC97Reg->AC_GLBCTRL &= ~COLD_RESET;
    DelayLoop(CONTROL_DELAY);
}


static void AC97_warm_reset(void)
{
    AC97_MSG((_T("[AC97] AC97_warm_reset()\n\r")));

    g_pAC97Reg->AC_GLBCTRL |= WARM_RESET;
    DelayLoop(CONTROL_DELAY);
    g_pAC97Reg->AC_GLBCTRL &= ~WARM_RESET;
    DelayLoop(CONTROL_DELAY);
}


static void AC97_set_ACLink_On(void)
{
    AC97_MSG((_T("[AC97] AC97_set_ACLink_On()\n\r")));

    g_pAC97Reg->AC_GLBCTRL |= ACLINK_ON;
    DelayLoop(CONTROL_DELAY);
}


static void AC97_set_ACLink_Off(void)
{
    AC97_MSG((_T("[AC97] AC97_set_ACLink_Off()\n\r")));

    g_pAC97Reg->AC_GLBCTRL &= ~ACLINK_ON;
    DelayLoop(CONTROL_DELAY);
}


static AC97_STATUS AC97_get_status(void)
{
    return g_pAC97Reg->AC_GLBSTAT;
}


static void DelayLoop(unsigned int count)
{
    volatile int i, j=0;

    for(;count > 0;count--)
        for(i=0;i < DELAY_LOOP_COUNT; i++) { j++; }
}

