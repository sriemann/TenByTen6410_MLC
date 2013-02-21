#ifndef __S3C6410_IMAGE_ROTATOR_H__
#define __S3C6410_IMAGE_ROTATOR_H__

#if __cplusplus
extern "C"
{
#endif

typedef enum
{
    ROT_FORMAT_YUV420 = 0,
    ROT_FORMAT_YUV422,
    ROT_FORMAT_RGB565,
    ROT_FORMAT_RGB888
} ROTATOR_IMAGE_FORMAT;

typedef enum
{
    ROT_OP_ROTATE_90 = 0,
    ROT_OP_ROTATE_180,
    ROT_OP_ROTATE_270,
    ROT_OP_FLIP_VERTICAL,
    ROT_OP_FLIP_HORIZONTAL
} ROTATOR_OPERATION_TYPE;

typedef enum
{
    ROT_SUCCESS,
    ROT_ERROR_NULL_PARAMETER,
    ROT_ERROR_ILLEGAL_PARAMETER,
    ROT_ERROR_NOT_INITIALIZED,
    ROT_ERROR_NOT_IMPLEMENTED,
    ROT_ERROR_XXX
} ROT_ERROR;

typedef enum
{
    ROTATOR_IDLE = 0,
    ROTATOR_BUSY,
    ROTATOR_BUSY2
} ROTATOR_OP_STATUS;

typedef struct _tRotatorStatus
{
    unsigned int    uiCurLineNumber;
    BOOL        bIntPending;
    ROTATOR_OP_STATUS eOpStatus;
} tRotatorStatus;

ROT_ERROR Rotator_initialize_register_address(void *pRotatorReg);
ROT_ERROR Rotator_initialize(ROTATOR_IMAGE_FORMAT eFormat, ROTATOR_OPERATION_TYPE eOperation, unsigned int uiSrcWidth, unsigned int uiSrcHeight);
ROT_ERROR Rotator_set_source_buffer(unsigned int AddrRGBY, unsigned int AddrCb, unsigned int AddrCr);
ROT_ERROR Rotator_set_destination_buffer(unsigned int AddrRGBY, unsigned int AddrCb, unsigned int AddrCr);

void Rotator_start(void);
void Rotator_stop(void);

void Rotator_enable_interrupt(void);
void Rotator_disable_interrupt(void);
BOOL Rotator_clear_interrupt_pending(void);

ROT_ERROR Rotator_get_status(tRotatorStatus *pStatus);


#if __cplusplus
}
#endif

#endif    // __S3C6410_IMAGE_ROTATOR_H__