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

#ifndef    _WM9713_H_
#define    _WM9713_H_

typedef enum
{
    WM9713_RESET                        = 0x00,
    WM9713_SPEAKER_VOL                = 0x02,
    WM9713_HEADPHONE_VOL            = 0x04,
    WM9713_OUT34_VOL                    = 0x06,
    WM9713_MONO_VOL_ROUTING        = 0x08,
    WM9713_LINEIN_VOL_ROUTING        = 0x0A,
    WM9713_DAC_VOL_ROUTING            = 0x0C,
    WM9713_MIC_VOL                    = 0x0E,
    WM9713_MIC_ROUTING                = 0x10,
    WM9713_RECORD_VOL                = 0x12,
    WM9713_RECORD_ROUTING_MUX        = 0x14,
    WM9713_PCBEEP_VOL_ROUTING        = 0x16,
    WM9713_VxDAC_VOL_ROUTING        = 0x18,
    WM9713_AUXDAC_VOL_ROUTING        = 0x1A,
    WM9713_OUTPUT_MUX                = 0x1C,
    WM9713_DAC_3D_INV_MUX            = 0x1E,
    WM9713_DAC_TONE                    = 0x20,
    WM9713_MIC_INPUT_SEL                = 0x22,
    WM9713_OUTPUT_VOL_MAP            = 0x24,
    WM9713_POWER_CONTROL            = 0x26,
    WM9713_EXTED_AUDIOID            = 0x28,
    WM9713_EXTED_AUDIOCTRL            = 0x2A,
    WM9713_AUDIO_DAC_RATE            = 0x2C,
    WM9713_AUX_DAC_RATE                = 0x2E,
    WM9713_AUDIO_ADC_RATE            = 0x32,
    WM9713_PCM_CODEC_CTRL            = 0x36,
    WM9713_SPDIF_CONTROL                = 0x3A,
    WM9713_POWERDOWN1                = 0x3C,
    WM9713_POWERDOWN2                = 0x3E,
    WM9713_GENERAL                    = 0x40,
    WM9713_FAST_POWERUP_CTRL        = 0x42,
    WM9713_MCLK_PLL_CTRL0            = 0x44,
    WM9713_MCLK_PLL_CTRL1            = 0x46,
    WM9713_GPIO_CONFIG                = 0x4C,
    WM9713_GPIO_POLARITY                = 0x4E,
    WM9713_GPIO_STICKY                = 0x50,
    WM9713_GPIO_WAKEUP                = 0x52,
    WM9713_GPIO_STATUS                = 0x54,
    WM9713_GPIO_SAHRING                = 0x56,
    WM9713_GPIO_PULLUPDN            = 0x58,
    WM9713_ADDITIONAL_FUNC1            = 0x5A,
    WM9713_ADDITIONAL_FUNC2            = 0x5C,
    WM9713_ALC_CONTROL                = 0x60,
    WM9713_ALC_NOISE_CONTROL        = 0x62,
    WM9713_AUXDAC_INPUT_CTRL        = 0x64,
    WM9713_DIGITISER_REG1            = 0x74,
    WM9713_DIGITISER_REG2            = 0x76,
    WM9713_DIGITISER_REG3            = 0x78,
    WM9713_DIGITISER_READBACK        = 0x7A,
    WM9713_VENDOR_ID1                = 0x7C,
    WM9713_VENDOR_ID2                = 0x7E,
}  WM9713_RegAddr;


// control
#define        AC97_PWR_PR0            0x0100      // ADC and Mux powerdown
#define        AC97_PWR_PR1            0x0200      // DAC powerdown
#define        AC97_PWR_PR2            0x0400      // Analog Mixer Powerdown (Vref on)
#define        AC97_PWR_PR3            0x0800      // Output Mixer Powerdown (Vref off)
#define        AC97_PWR_PR4            0x1000      // AC-link Powerdown
#define        AC97_PWR_PR5            0x2000      // Internal Clk Disable
#define        AC97_PWR_PR6            0x4000      // Aux Out Powerdown


// useful power states
#define        AC97_PWR_D0             0x0000      // everything on
#define        AC97_PWR_D1             AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR4
#define        AC97_PWR_D2             AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR2|AC97_PWR_PR3|AC97_PWR_PR4
#define        AC97_PWR_D3             AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR2|AC97_PWR_PR3|AC97_PWR_PR4
#define        AC97_PWR_ANLOFF         AC97_PWR_PR2|AC97_PWR_PR3  //analog section off


#endif    // _WM9713_H_
