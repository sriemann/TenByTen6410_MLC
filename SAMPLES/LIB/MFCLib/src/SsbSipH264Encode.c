#include <windows.h>

#include "MfcDriver.h"
#include "MfcDrvParams.h"

#include "SsbSipH264Encode.h"
#include "SsbSipLogMsg.h"


#define _MFCLIB_H264_ENC_MAGIC_NUMBER        0x92242002

typedef struct
{
    DWORD   magic;
    HANDLE  hOpen;

    unsigned int width, height;
    unsigned int framerate, bitrate;
    unsigned int gop_num;

    unsigned int intraqp;
    unsigned int qpmax;
    float gamma;

    int     enc_strm_size;

//    DWORD   proc_id;    // process ID

    int     fInit;
} _MFCLIB_H264_ENC;



void *SsbSipH264EncodeInit(unsigned int uiWidth,     unsigned int uiHeight,
                           unsigned int uiFramerate, unsigned int uiBitrate_kbps,
                           unsigned int uiGOPNum)
{
    _MFCLIB_H264_ENC   *pCTX;
    HANDLE              hOpen;


    //////////////////////////////
    /////     CreateFile     /////
    //////////////////////////////
    hOpen = CreateFile(L"MFC1:",
                       GENERIC_READ|GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hOpen == INVALID_HANDLE_VALUE) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeInit", "MFC Open failure.\n");
        return NULL;
    }

    pCTX = (_MFCLIB_H264_ENC *) malloc(sizeof(_MFCLIB_H264_ENC));
    if (pCTX == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeInit", "malloc failed.\n");
        CloseHandle(hOpen);
        return NULL;
    }
    memset(pCTX, 0x00, sizeof(_MFCLIB_H264_ENC));

    pCTX->magic = _MFCLIB_H264_ENC_MAGIC_NUMBER;
    pCTX->hOpen = hOpen;
    pCTX->fInit = 0;

    pCTX->width     = uiWidth;
    pCTX->height    = uiHeight;
    pCTX->framerate = uiFramerate;
    pCTX->bitrate   = uiBitrate_kbps;
    pCTX->gop_num   = uiGOPNum;

    pCTX->intraqp = 10;
    pCTX->qpmax = 10;
    pCTX->gamma = 0.75;

    pCTX->enc_strm_size = 0;

    return (void *) pCTX;
}


int SsbSipH264EncodeExe(void *openHandle)
{
    _MFCLIB_H264_ENC  *pCTX;

    BOOL                r;
    MFC_ARGS            mfc_args;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeExe", "openHandle is NULL\n");
        return SSBSIP_H264_ENC_RET_ERR_INVALID_HANDLE;
    }


    pCTX  = (_MFCLIB_H264_ENC *) openHandle;

    if (!pCTX->fInit) {
        mfc_args.enc_init.in_width        = pCTX->width;
        mfc_args.enc_init.in_height        = pCTX->height;
        mfc_args.enc_init.in_bitrate    = pCTX->bitrate;
        mfc_args.enc_init.in_gopNum        = pCTX->gop_num;
        mfc_args.enc_init.in_frameRateRes    = pCTX->framerate;
        mfc_args.enc_init.in_frameRateDiv    = 0;

        mfc_args.enc_init.in_intraqp = pCTX->intraqp;
        mfc_args.enc_init.in_qpmax = pCTX->qpmax;
        mfc_args.enc_init.in_gamma = pCTX->gamma;

        ////////////////////////////////////////////////
        /////          (DeviceIoControl)           /////
        /////       IOCTL_MFC_H264_ENC_INIT        /////
        ////////////////////////////////////////////////
        r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_H264_ENC_INIT,
                            &mfc_args, sizeof(MFC_ENC_INIT_ARG),
                            NULL, 0,
                            NULL,
                            NULL);
        if ((r < 0) || (mfc_args.enc_init.ret_code < 0)) {
            LOG_MSG(LOG_ERROR, "SsbSipH264EncodeExe", "IOCTL_MFC_MPEG4_ENC_INIT failed.\n");
            return SSBSIP_H264_ENC_RET_ERR_ENCODE_FAIL;
        }

        pCTX->fInit = 1;

        return SSBSIP_H264_ENC_RET_OK;
    }

    /////////////////////////////////////////////////
    /////           (DeviceIoControl)           /////
    /////       IOCTL_MFC_MPEG4_DEC_EXE         /////
    /////////////////////////////////////////////////
    r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_H264_ENC_EXE,
                        &mfc_args, sizeof(MFC_ENC_EXE_ARG),
                        NULL, 0,
                        NULL,
                        NULL);
    if ((r < 0) || (mfc_args.enc_exe.ret_code < 0)) {
        return SSBSIP_H264_ENC_RET_ERR_ENCODE_FAIL;
    }

    // Encoded stream size is saved. (This value is returned in SsbSipH264EncodeGetOutBuf)
    pCTX->enc_strm_size = mfc_args.enc_exe.out_encoded_size;

    return SSBSIP_H264_ENC_RET_OK;
}


int SsbSipH264EncodeDeInit(void *openHandle)
{
    _MFCLIB_H264_ENC  *pCTX;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeDeInit", "openHandle is NULL\n");
        return SSBSIP_H264_ENC_RET_ERR_INVALID_HANDLE;
    }


    pCTX  = (_MFCLIB_H264_ENC *) openHandle;


    CloseHandle(pCTX->hOpen);
    free(pCTX);

    return SSBSIP_H264_ENC_RET_OK;
}


void *SsbSipH264EncodeGetInBuf(void *openHandle, long size)
{
    _MFCLIB_H264_ENC   *pCTX;

    BOOL                r;
    MFC_ARGS            mfc_args;

    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeGetInBuf", "openHandle is NULL\n");
        return NULL;
    }
    if ((size < 0) || (size > 0x100000)) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeGetInBuf", "size is invalid. (size=%d)\n", size);
        return NULL;
    }

    pCTX  = (_MFCLIB_H264_ENC *) openHandle;


    /////////////////////////////////////////////////
    /////           (DeviceIoControl)           /////
    /////     IOCTL_MFC_GET_INPUT_BUF_ADDR      /////
    /////////////////////////////////////////////////
//    mfc_args.get_buf_addr.in_usr_data = GetCurrentProcessId();        // Input argument
    r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_GET_FRAM_BUF_ADDR,
                        &mfc_args, sizeof(MFC_GET_BUF_ADDR_ARG),
                        NULL, 0,
                        NULL,
                        NULL);
    if ((r < 0) || (mfc_args.get_buf_addr.ret_code < 0)) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeGetInBuf", "Failed in get FRAM_BUF address\n");
        return NULL;
    }


    return (void *)mfc_args.get_buf_addr.out_buf_addr;
}


void *SsbSipH264EncodeGetOutBuf(void *openHandle, long *size)
{
    _MFCLIB_H264_ENC  *pCTX;

    BOOL                r;
    MFC_ARGS            mfc_args;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeGetOutBuf", "openHandle is NULL\n");
        return NULL;
    }
    if (size == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeGetOutBuf", "size is NULL.\n");
        return NULL;
    }

    pCTX  = (_MFCLIB_H264_ENC *) openHandle;


    /////////////////////////////////////////////////
    /////           (DeviceIoControl)           /////
    /////      IOCTL_MFC_GET_STRM_BUF_ADDR      /////
    /////////////////////////////////////////////////
//    mfc_args.get_buf_addr.in_usr_data = GetCurrentProcessId();        // Input argument
    r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_GET_LINE_BUF_ADDR,
                        &mfc_args, sizeof(MFC_GET_BUF_ADDR_ARG),
                        NULL, 0,
                        NULL,
                        NULL);
    if ((r < 0) || (mfc_args.get_buf_addr.ret_code < 0)) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeGetOutBuf", "Failed in get LINE_BUF address.\n");
        return NULL;
    }

    *size = pCTX->enc_strm_size;

    return (void *)mfc_args.get_buf_addr.out_buf_addr;
}


int SsbSipH264EncodeSetConfig(void *openHandle, H264_ENC_CONF conf_type, void *value)
{
    _MFCLIB_H264_ENC  *pCTX;
    MFC_ARGS           mfc_args;
    BOOL               r;

    unsigned int       num_slices;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeSetConfig", "openHandle is NULL\n");
        return SSBSIP_H264_ENC_RET_ERR_INVALID_HANDLE;
    }
    if (value == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeSetConfig", "value is NULL\n");
        return SSBSIP_H264_ENC_RET_ERR_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB_H264_ENC *) openHandle;

    switch (conf_type) {
    case H264_ENC_SETCONF_NUM_SLICES:

        num_slices = *((unsigned int *) value);

        mfc_args.set_config.in_config_param     = MFC_SET_CONFIG_ENC_SLICE_MODE;
        mfc_args.set_config.in_config_value[0]  = (num_slices ? 1 : 0);
        mfc_args.set_config.in_config_value[1]  = num_slices;
        r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_SET_CONFIG,
                            &mfc_args, sizeof(MFC_SET_CONFIG_ARG),
                            NULL, 0,
                            NULL,
                            NULL);
        if ((r == FALSE) || (mfc_args.set_config.ret_code != 0)) {
            LOG_MSG(LOG_ERROR, "SsbSipH264EncodeSetConfig", "Error in H264_ENC_SETCONF_NUM_SLICES.\n");
            return SSBSIP_H264_ENC_RET_ERR_SETCONF_FAIL;
        }
        break;

    case H264_ENC_SETCONF_PARAM_CHANGE:

        mfc_args.set_config.in_config_param     = MFC_SET_CONFIG_ENC_PARAM_CHANGE;
        mfc_args.set_config.in_config_value[0]  = ((unsigned int *) value)[0];
        mfc_args.set_config.in_config_value[1]  = ((unsigned int *) value)[1];
        r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_SET_CONFIG,
                            &mfc_args, sizeof(MFC_SET_CONFIG_ARG),
                            NULL, 0,
                            NULL,
                            NULL);
        if ((r == FALSE) || (mfc_args.set_config.ret_code != 0)) {
            LOG_MSG(LOG_ERROR, "SsbSipH264EncodeSetConfig", "Error in H264_ENC_SETCONF_PARAM_CHANGE.\n");
            return SSBSIP_H264_ENC_RET_ERR_SETCONF_FAIL;
        }
        break;


    case H264_ENC_SETCONF_CUR_PIC_OPT:

        mfc_args.set_config.in_config_param     = MFC_SET_CONFIG_ENC_CUR_PIC_OPT;
        mfc_args.set_config.in_config_value[0]  = ((unsigned int *) value)[0];
        mfc_args.set_config.in_config_value[1]  = ((unsigned int *) value)[1];
        r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_SET_CONFIG,
                            &mfc_args, sizeof(MFC_SET_CONFIG_ARG),
                            NULL, 0,
                            NULL,
                            NULL);
        if ((r == FALSE) || (mfc_args.set_config.ret_code != 0)) {
            LOG_MSG(LOG_ERROR, "SsbSipH264EncodeSetConfig", "Error in H264_ENC_SETCONF_CUR_PIC_OPT.\n");
            return SSBSIP_H264_ENC_RET_ERR_SETCONF_FAIL;
        }
        break;

    default:
        LOG_MSG(LOG_ERROR, "SsbSipH264EncodeSetConfig", "No such conf_type is supported.\n");
        return SSBSIP_H264_ENC_RET_ERR_SETCONF_FAIL;
    }

    return SSBSIP_H264_ENC_RET_OK;
}


#if 0


int SsbSipH264DecodeGetConfig(void *openHandle, MPEG4_DEC_CONF conf_type, void *value)
{
    _MFCLIB_H264_ENC  *pCTX;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        return NULL;
    }

    pCTX  = (_MFCLIB_H264_ENC *) openHandle;


    switch (conf_type) {
    case MPEG4_DEC_GETCONF_STREAMINFO:
        ((SSBSIP_MPEG4_STREAM_INFO *)value)->width  = pCTX->width;
        ((SSBSIP_MPEG4_STREAM_INFO *)value)->height = pCTX->height;
        break;

    default:
        break;
    }


    return 0;
}


#endif
