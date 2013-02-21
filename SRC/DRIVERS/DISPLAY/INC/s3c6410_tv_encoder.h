#ifndef __S3C6410_TV_ENCODER_H__
#define __S3C6410_TV_ENCODER_H__

#if __cplusplus
extern "C"
{
#endif

typedef enum
{
    TVENC_COMPOSITE = 0,
    TVENC_S_VIDEO
} TVENC_OUTPUT_TYPE;

typedef enum
{
    TVENC_NTSC_M = 0,
    TVENC_NTSC_J,
    TVENC_PAL_BDGHI,
    TVENC_PAL_M,
    TVENC_PAL_NC
} TVENC_OUTPUT_STANDARD;

typedef enum
{
    TVENC_ENCODER_OFF = 0,
    TVENC_ENCODER_ON
} TVENC_ENCODER_ONOFF;

typedef enum
{
    TVENC_MUTE_OFF = 0,
    TVENC_MUTE_ON
} TVENC_MUTE_ONOFF;

typedef enum
{
    TVENC_MACROVISION_AGC4L = 0,
    TVENC_MACROVISION_AGC2L,
    TVENC_MACROVISION_N01,
    TVENC_MACROVISION_N02,
    TVENC_MACROVISION_P01,
    TVENC_MACROVISION_P02,
    TVENC_MACROVISION_OFF
} TVENC_MACROVISION_PATTERN;

typedef enum
{
    TVENC_FIFO_OPERATION = 0,
    TVENC_FIFO_IDLE
} TVENC_FIFO_STATUS;

typedef enum
{
    TVENC_SYSTEM_OFF = 0,
    TVENC_SYSTEM_READY,
    TVENC_SYSTEM_OPERATION,
    TVENC_SYSTEM_UNDER_RUN,
} TVENC_SYSTEM_STATUS;

typedef enum
{
    TVENC_BG_SOFTMIX_OFF = 0,
    TVENC_BG_SOFTMIX_ON
} TVENC_BG_SOFTMIX_ONOFF;

typedef enum
{
    TVENC_BG_BLACK = 0,
    TVENC_BG_BLUE,
    TVENC_BG_RED,
    TVENC_BG_MAGENTA,
    TVENC_BG_GREEN,
    TVENC_BG_CYAN,
    TVENC_BG_YELLOW,
    TVENC_BG_WHITE
} TVENC_BG_COLOR;

typedef enum
{
    TVENC_SUCCESS,
    TVENC_ERROR_NULL_PARAMETER,
    TVENC_ERROR_ILLEGAL_PARAMETER,
    TVENC_ERROR_NOT_INITIALIZED,
    TVENC_ERROR_NOT_IMPLEMENTED,
    TVENC_ERROR_XXX
} TVENC_ERROR;

typedef struct _tTVEncConfig
{
    TVENC_OUTPUT_TYPE OutputType;
    TVENC_OUTPUT_STANDARD OutputStandard;
    TVENC_MACROVISION_PATTERN MacrovisionPattern;
    unsigned int Width;
    unsigned int Height;
} tTVEncConfig;

TVENC_ERROR TVEnc_initialize_register_address(void *pTVEncReg, void *pGPIOReg);
TVENC_ERROR TVEnc_initialize(TVENC_OUTPUT_TYPE OutputType, TVENC_OUTPUT_STANDARD OutputStandard, TVENC_MACROVISION_PATTERN Macrovision, unsigned int Width, unsigned int Height);

void TVEnc_output_onoff(TVENC_ENCODER_ONOFF OnOff);
TVENC_ENCODER_ONOFF TVEnc_get_output_state(void);

void TVEnc_enable_interrupt(void);
void TVEnc_disable_interrupt(void);
BOOL TVEnc_clear_interrupt_pending(void);

static void TVEnc_initialize_port(TVENC_OUTPUT_TYPE Type);
static TVENC_ERROR TVEnc_set_output_mode(TVENC_OUTPUT_TYPE Type, TVENC_OUTPUT_STANDARD Standard);
static TVENC_ERROR TVEnc_set_video_size(unsigned int uiWidth, unsigned int uiHeight);

#if    0    
void TVEnc_mute_onoff(TVENC_MUTE_ONOFF OnOff);

TVENC_FIFO_STATUS TVEnc_get_fifo_status(void);
TVENC_SYSTEM_STATUS TVEnc_get_system_status(void);

static TVENC_ERROR TVEnc_set_contrast(unsigned int uiContrast);
static TVENC_ERROR TVEnc_set_brightness(unsigned int uiBrightness);
static TVENC_ERROR TVEnc_set_chroma_gain(unsigned int uiCbGain, unsigned int uiCrGain);
static TVENC_ERROR TVEnc_set_background_control(TVENC_BG_SOFTMIX_ONOFF SMOnOff, TVENC_BG_COLOR Color, unsigned int uiLumaOffset);
static TVENC_ERROR TVEnc_set_background_area(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
static TVENC_ERROR TVEnc_set_demo_window_area(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiOffsetX, unsigned int uiOffsetY);
// Demo Control???
// Pedestal Control
// HUE Phase Control
// Black Stretch
// White Strech
//
#endif

#if __cplusplus
}
#endif

#endif    // __S3C6410_TV_ENCODER_H__