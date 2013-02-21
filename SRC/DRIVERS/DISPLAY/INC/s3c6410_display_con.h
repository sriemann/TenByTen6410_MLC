//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#ifndef __S3C6410_DISPLAY_CON_H__
#define __S3C6410_DISPLAY_CON_H__

#if __cplusplus
extern "C"
{
#endif

#define MINIMUM_VCLK_FOR_TV    6000000

typedef enum
{
    DISP_VIDOUT_RGBIF,
    DISP_VIDOUT_TVENCODER,
    DISP_VIDOUT_TVENCODER_ITU601,    
    DISP_VIDOUT_RGBIF_TVENCODER,
    DISP_VIDOUT_I80IF_LDI0,
    DISP_VIDOUT_I80IF_LDI1,
    DISP_VIDOUT_I80IF_LDI0_TVENCODER,
    DISP_VIDOUT_I80IF_LDI1_TVENCODER
} DISP_VIDOUT_MODE;

typedef enum
{
    DISP_16BIT_RGB565_P = 0,
    DISP_16BIT_RGB565_S,
    DISP_18BIT_RGB666_P,
    DISP_18BIT_RGB666_S,
    DISP_24BIT_RGB888_P,
    DISP_24BIT_RGB888_S,
} DISP_RGBIFOUT_MODE;

typedef enum
{
    DISP_WIN0_DMA,
    DISP_WIN0_POST_RGB,
    DISP_WIN0_POST_YUV,
    DISP_WIN1_DMA,
    DISP_WIN1_TVSCALER_RGB,
    DISP_WIN1_TVSCALER_YUV,
    DISP_WIN1_CIPREVIEW_RGB,
    DISP_WIN1_CIPREVIEW_YUV,
    DISP_WIN2_DMA,
    DISP_WIN2_TVSCALER_RGB,
    DISP_WIN2_TVSCALER_YUV,
    DISP_WIN2_CICODEC_RGB,
    DISP_WIN2_CICODEC_YUV,
    DISP_WIN3_DMA,
    DISP_WIN4_DMA
} DISP_WINDOW_MODE;

typedef enum
{
    DISP_WIN0 = 0,
    DISP_WIN1,
    DISP_WIN2,
    DISP_WIN3,
    DISP_WIN4,
    DISP_WIN_MAX
} DISP_WINDOW;

typedef enum
{
    DISP_1BPP = 0,
    DISP_2BPP,
    DISP_4BPP,
    DISP_8BPP_PAL,
    DISP_8BPP_NOPAL,
    DISP_16BPP_565    ,
    DISP_16BPP_A555,
    DISP_16BPP_I555,
    DISP_18BPP_666,
    DISP_18BPP_A665,
    DISP_19BPP_A666,
    DISP_24BPP_888,
    DISP_24BPP_A887,
    DISP_25BPP_A888
} DISP_BPP_MODE;

typedef enum
{
    DISP_ALPHA_PER_PLANE,
    DISP_ALPHA_PER_PIXEL
} DISP_ALPHA_BLEND_METHOD;

typedef enum
{
    DISP_FG_MATCH_BG_DISPLAY,
    DISP_BG_MATCH_FG_DISPLAY
} DISP_COLOR_KEY_DIRECTION;

typedef enum
{
    DISP_ENVID_OFF,
    DISP_ENVID_ON,
    DISP_ENVID_DIRECT_OFF
} DISP_ENVID_ONOFF;

typedef enum
{
    DISP_WINDOW_OFF,
    DISP_WINDOW_ON
} DISP_WINDOW_ONOFF;

typedef enum
{
    DISP_LOCALPATH_OFF,
    DISP_LOCALPATH_ON
} DISP_LOCALPATH_ONOFF;

typedef enum
{
    DISP_DITHER_OFF,
    DISP_DITHER_565,
    DISP_DITHER_666,
    DISP_DITHER_888
} DISP_DITHER_MODE;

typedef enum
{
    DISP_FRMINT_BACKPORCH = 0,
    DISP_FRMINT_VSYNC,
    DISP_FRMINT_ACTIVE,
    DISP_FRMINT_FRONTPORCH
} DISP_FRAME_INTERRUPT;

typedef enum
{
    DISP_FIFO_WIN0 = 1<<5,
    DISP_FIFO_WIN1 = 1<<6,
    DISP_FIFO_WIN2 = 1<<9,
    DISP_FIFO_WIN3 = 1<<10,
    DISP_FIFO_WIN4 = 1<<11
} DISP_FIFO_INTERRUPT_WINDOW;

typedef enum
{
    DISP_FIFO_0_25 = 0,
    DISP_FIFO_0_50,
    DISP_FIFO_0_75,
    DISP_FIFO_EMPTY,
    DISP_FIFO_FULL
} DISP_FIFO_INTERRUPT_LEVEL;

typedef enum
{
    DISP_INT_NONE = 0,
    DISP_INT_FRAME = 1<<0,
    DISP_INT_FIFO = 1<<1,
    DISP_INT_I80 = 1<<2
} DISP_INTERRUPT;

typedef enum
{
    DISP_V_VSYNC = 0,
    DISP_V_BACKPORCH,
    DISP_V_ACTIVE,
    DISP_V_FRONTPORCH
} DISP_VERTICAL_STATUS;

typedef enum
{
    DISP_SUCCESS,
    DISP_ERROR_NULL_PARAMETER,
    DISP_ERROR_ILLEGAL_PARAMETER,
    DISP_ERROR_NOT_INITIALIZED,
    DISP_ERROR_NOT_IMPLEMENTED,
    DISP_ERROR_XXX
} DISP_ERROR;

typedef struct _tDevInfo
{
    DISP_VIDOUT_MODE VideoOutMode;
    DISP_RGBIFOUT_MODE RGBOutMode;
    unsigned int uiWidth;
    unsigned int uiHeight;
    unsigned int VBPD_Value;
    unsigned int VFPD_Value;
    unsigned int VSPW_Value;
    unsigned int HBPD_Value;
    unsigned int HFPD_Value;
    unsigned int HSPW_Value;
    unsigned int VCLK_Polarity;
    unsigned int HSYNC_Polarity;
    unsigned int VSYNC_Polarity;
    unsigned int VDEN_Polarity;
    unsigned int PNR_Mode;
    unsigned int VCLK_Source;
    unsigned int Frame_Rate;
    unsigned int VCLK_Direction;
    //unsigned int VCLK_Value;
} tDevInfo;

DISP_ERROR Disp_initialize_register_address(volatile S3C6410_DISPLAY_REG *pDispConReg,volatile S3C6410_MSMIF_REG *pMSMIFReg,volatile S3C6410_GPIO_REG *pGPIOReg);
DISP_ERROR Disp_set_output_device_information(void *pInfo);
//void* Disp_get_output_device_information_buffer(void);
DISP_ERROR Disp_set_output_TV_information(unsigned int uiWidth, unsigned int uiHeight);
DISP_ERROR Disp_initialize_output_interface(DISP_VIDOUT_MODE VidoutMode);
DISP_ERROR Disp_set_window_mode(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
DISP_ERROR Disp_set_window_position(DISP_WINDOW Win, unsigned int uiOffsetX, unsigned int uiOffsetY);
DISP_ERROR Disp_set_framebuffer(DISP_WINDOW Win, unsigned int uiFrameBufferAddress);
DISP_ERROR Disp_envid_onoff(DISP_ENVID_ONOFF EnvidOnOff);
BOOL Disp_get_envid_status(void);
DISP_ERROR Disp_window_onfoff(DISP_WINDOW Win, DISP_WINDOW_ONOFF WinOnOff);
DISP_ERROR Disp_local_path_off(DISP_WINDOW Win);
DISP_WINDOW_ONOFF Disp_get_window_status(DISP_WINDOW Win);
DISP_ERROR Disp_set_dithering_mode(DISP_DITHER_MODE Mode);

DISP_ERROR Disp_set_window_color_map(DISP_WINDOW Win, BOOL bOnOff, unsigned int uiColorValue);
DISP_ERROR Disp_set_color_key(DISP_WINDOW Win, BOOL bOnOff, DISP_COLOR_KEY_DIRECTION Direction, unsigned int uiColorKey, unsigned int uiComparekey);
DISP_ERROR Disp_set_alpha_blending(DISP_WINDOW Win, DISP_ALPHA_BLEND_METHOD Method, unsigned int uiAlphaSet0, unsigned int uiAlphaSet1);


DISP_ERROR Disp_set_frame_interrupt(DISP_FRAME_INTERRUPT FrameInt);
DISP_ERROR Disp_enable_frame_interrupt(void);
DISP_ERROR Disp_disable_frame_interrupt(void);

DISP_ERROR Disp_set_fifo_interrupt(DISP_WINDOW Win, DISP_FIFO_INTERRUPT_LEVEL Level);
DISP_ERROR Disp_enable_fifo_interrupt(void);
DISP_ERROR Disp_disable_fifo_interrupt(void);

DISP_INTERRUPT Disp_clear_interrupt_pending(void);

unsigned int Disp_get_line_count(void);
DISP_VERTICAL_STATUS Disp_get_vertical_status(void);

DISP_WINDOW Disp_get_win_num_from_win_mode(DISP_WINDOW_MODE Mode);

static DISP_ERROR Disp_initialize_port_RGBIF(DISP_RGBIFOUT_MODE RGBOutMode);
static DISP_ERROR Disp_initialize_RGBIF(void);
static DISP_ERROR Disp_initialize_RGBIF_withTVEnc(void);
static DISP_ERROR Disp_initialize_TVEnc(void);
static DISP_ERROR Disp_window0_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
static DISP_ERROR Disp_window1_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
static DISP_ERROR Disp_window2_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
static DISP_ERROR Disp_window3_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
static DISP_ERROR Disp_window4_initialize(DISP_WINDOW_MODE Mode, DISP_BPP_MODE BPPMode, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
static BOOL Disp_get_vclk_direction_divider(unsigned int CLKSrc, unsigned int *ClkDir, unsigned int *ClkDiv);
static BOOL Disp_get_vclk_direction_divider_forTVEnc(unsigned int CLKSrc, unsigned int *ClkDir, unsigned int *ClkDiv);
void Disp_Check_Vsync_Value(void);
void Disp_VSync_Enable(void);
void Disp_VSync_Disable(void);
BOOL Disp_GetFlipStatus(void);


#if __cplusplus
}
#endif

#endif    // __S3C6410_DISPLAY_CON_H__
