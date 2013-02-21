#ifndef __S3C6410_TV_SCALER_H__
#define __S3C6410_TV_SCALER_H__

#if __cplusplus
extern "C"
{
#endif

typedef enum
{
    TVSC_SRC_RGB16 = 0,
    TVSC_SRC_RGB24,
    TVSC_SRC_YUV420,
    TVSC_SRC_YUV422_YCBYCR,
    TVSC_SRC_YUV422_CBYCRY,
    TVSC_SRC_YUV422_YCRYCB,
    TVSC_SRC_YUV422_CRYCBY,
    TVSC_SRC_FIFO
} TVSC_SRC_TYPE;

typedef enum
{
    TVSC_DST_RGB16 = 0,
    TVSC_DST_RGB24,
    TVSC_DST_YUV420,
    TVSC_DST_YUV422_YCBYCR,
    TVSC_DST_YUV422_CBYCRY,
    TVSC_DST_YUV422_YCRYCB,
    TVSC_DST_YUV422_CRYCBY,
    TVSC_DST_FIFO_YUV444,
    TVSC_DST_FIFO_RGB888
} TVSC_DST_TYPE;

typedef enum
{
    TVSC_PER_FRAME_MODE,
    TVSC_FREE_RUN_MODE
} TVSC_OP_MODE;

typedef enum
{
    TVSC_PROGRESSIVE,
    TVSC_INTERLACE
} TVSC_SCAN_MODE;

typedef enum
{
    TVSC_SRC_ADDRESS,
    TVSC_NEXT_SRC_ADDRESS,
    TVSC_DST_ADDRESS,
    TVSC_NEXT_DST_ADDRESS
} TVSC_DMA_ADDRESS;

typedef enum
{
    TVSC_IDLE = 0,
    TVSC_BUSY
} TVSC_STATE;

typedef enum
{
    TVSC_SUCCESS,
    TVSC_ERROR_NULL_PARAMETER,
    TVSC_ERROR_ILLEGAL_PARAMETER,
    TVSC_ERROR_NOT_INITIALIZED,
    TVSC_ERROR_NOT_IMPLEMENTED,
    TVSC_ERROR_PRESCALER_OUT_OF_SCALE_RANGE,
    TVSC_ERROR_XXX
} TVSC_ERROR;

TVSC_ERROR TVSC_initialize_register_address(void *pTVSCReg);
TVSC_ERROR TVSC_initialize(TVSC_OP_MODE Mode, TVSC_SCAN_MODE Scan,
                    TVSC_SRC_TYPE SrcType, unsigned int SrcBaseWidth, unsigned int SrcBaseHeight,
                    unsigned int SrcWidth, unsigned int SrcHeight, unsigned int SrcOffsetX, unsigned int SrcOffsetY,
                    TVSC_DST_TYPE DstType, unsigned int DstBaseWidth, unsigned int DstBaseHeight,
                    unsigned int DstWidth, unsigned int DstHeight, unsigned int DstOffsetX, unsigned int DstOffsetY);
TVSC_ERROR TVSC_set_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);
TVSC_ERROR TVSC_set_next_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);
TVSC_ERROR TVSC_set_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);
TVSC_ERROR TVSC_set_next_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);

void TVSC_processing_start(void);
TVSC_ERROR TVSC_processing_stop(void);
void TVSC_autoload_disable(void);
TVSC_STATE TVSC_get_processing_state(void);

void TVSC_enable_interrupt(void);
void TVSC_disable_interrupt(void);
BOOL TVSC_clear_interrupt_pending(void);

static TVSC_ERROR TVSC_set_mode(TVSC_OP_MODE Mode, TVSC_SCAN_MODE Scan, TVSC_SRC_TYPE SrcType, TVSC_DST_TYPE DstType);
static void TVSC_set_source_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY);
static void TVSC_set_destination_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY);
static TVSC_ERROR TVSC_get_prescaler_shiftvalue(unsigned int *MainShift, unsigned int SrcValue, unsigned int DstValue);
static TVSC_ERROR TVSC_update_condition(void);
static TVSC_ERROR TVSC_set_dma_address(TVSC_DMA_ADDRESS DMA, unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);


#if __cplusplus
}
#endif

#endif    // __S3C6410_TV_SCALER_H__
