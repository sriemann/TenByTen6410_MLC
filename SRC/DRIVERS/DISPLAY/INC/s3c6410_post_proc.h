#ifndef __S3C6410_POST_PROC_H__
#define __S3C6410_POST_PROC_H__

#if __cplusplus
extern "C"
{
#endif

typedef enum
{
    POST_SRC_RGB16 = 0,
    POST_SRC_RGB24,
    POST_SRC_YUV420,
    POST_SRC_YUV422_YCBYCR,
    POST_SRC_YUV422_CBYCRY,
    POST_SRC_YUV422_YCRYCB,
    POST_SRC_YUV422_CRYCBY
} POST_SRC_TYPE;

typedef enum
{
    POST_DST_RGB16 = 0,
    POST_DST_RGB24,
    POST_DST_YUV420,
    POST_DST_YUV422_YCBYCR,
    POST_DST_YUV422_CBYCRY,
    POST_DST_YUV422_YCRYCB,
    POST_DST_YUV422_CRYCBY,
    POST_DST_FIFO_YUV444,
    POST_DST_FIFO_RGB888
} POST_DST_TYPE;

typedef enum
{
    POST_PER_FRAME_MODE,
    POST_FREE_RUN_MODE
} POST_OP_MODE;

typedef enum
{
    POST_PROGRESSIVE,
    POST_INTERLACE
} POST_SCAN_MODE;

typedef enum
{
    POST_SRC_ADDRESS,
    POST_NEXT_SRC_ADDRESS,
    POST_DST_ADDRESS,
    POST_NEXT_DST_ADDRESS
} POST_DMA_ADDRESS;

typedef enum
{
    POST_IDLE = 0,
    POST_BUSY
} POST_STATE;

typedef enum
{
    POST_SUCCESS,
    POST_ERROR_NULL_PARAMETER,
    POST_ERROR_ILLEGAL_PARAMETER,
    POST_ERROR_NOT_INITIALIZED,
    POST_ERROR_NOT_IMPLEMENTED,
    POST_ERROR_PRESCALER_OUT_OF_SCALE_RANGE,
    POST_ERROR_XXX
} POST_ERROR;

POST_ERROR Post_initialize_register_address(void *pPostReg);
POST_ERROR Post_initialize(POST_OP_MODE Mode, POST_SCAN_MODE Scan,
                    POST_SRC_TYPE SrcType, unsigned int SrcBaseWidth, unsigned int SrcBaseHeight,
                    unsigned int SrcWidth, unsigned int SrcHeight, unsigned int SrcOffsetX, unsigned int SrcOffsetY,
                    POST_DST_TYPE DstType, unsigned int DstBaseWidth, unsigned int DstBaseHeight,
                    unsigned int DstWidth, unsigned int DstHeight, unsigned int DstOffsetX, unsigned int DstOffsetY);
POST_ERROR Post_set_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);
POST_ERROR Post_set_next_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);
POST_ERROR Post_set_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);
POST_ERROR Post_set_next_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);

void Post_processing_start(void);
void Post_processing_stop(void);
void Post_autoload_disable(void);
POST_STATE Post_get_processing_state(void);

void Post_enable_interrupt(void);
void Post_disable_interrupt(void);
BOOL Post_clear_interrupt_pending(void);

static POST_ERROR Post_set_mode(POST_OP_MODE Mode, POST_SCAN_MODE Scan, POST_SRC_TYPE SrcType, POST_DST_TYPE DstType);
static void Post_set_source_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY);
static void Post_set_destination_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY);
static POST_ERROR Post_get_prescaler_shiftvalue(unsigned int *MainShift, unsigned int SrcValue, unsigned int DstValue);
static POST_ERROR Post_update_condition(void);
static POST_ERROR Post_set_dma_address(POST_DMA_ADDRESS DMA, unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr);


#if __cplusplus
}
#endif

#endif    // __S3C6410_POST_PROC_H__
