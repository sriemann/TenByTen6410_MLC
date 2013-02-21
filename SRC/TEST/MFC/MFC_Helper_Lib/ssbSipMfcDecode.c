//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#include <windows.h>

#include "MfcDriver.h"
#include "MfcDrvParams.h"


#include "SsbSipMfcDecode.h"
#include "SsbSipLogMsg.h"

#define _MFCLIB_DEC_MAGIC_NUMBER        0x92241000

typedef struct
{
    DWORD   magic;
    HANDLE  hOpen;

    int     decoder;
    unsigned int     width, height;

    void   *p_buf;
    int     size;

//    DWORD   proc_id;    // process ID

    int     fInit;
} _MFCLIB_DEC;



void *SsbSipMfcDecodeInit(int dec_type)
{
    _MFCLIB_DEC   *pCTX;
    HANDLE         hOpen;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (   (dec_type != SSBSIPMFCDEC_MPEG4)
        && (dec_type != SSBSIPMFCDEC_H263)
        && (dec_type != SSBSIPMFCDEC_H264)
        && (dec_type != SSBSIPMFCDEC_VC1)) {

        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeInit", "Undefined codec type.\n");
        return NULL;
    }


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
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeInit", "MFC Open failure\n");
        return NULL;
    }

    pCTX = (_MFCLIB_DEC *) malloc(sizeof(_MFCLIB_DEC));
    if (pCTX == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeInit", "malloc failed.\n");
        CloseHandle(hOpen);
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB_DEC));

    pCTX->magic   = _MFCLIB_DEC_MAGIC_NUMBER;
    pCTX->hOpen   = hOpen;
    pCTX->decoder = dec_type;
//    pCTX->proc_id = GetCurrentProcessId();
    pCTX->fInit   = 0;


    return (void *) pCTX;
}


int SsbSipMfcDecodeExe(void *openHandle, long lengthBufFill)
{
    _MFCLIB_DEC    *pCTX;
    MFC_ARGS        mfc_args;
    BOOL            r;

    int  ioctl_cmd;

    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeExe", "openHandle is NULL\n");
        return SSBSIP_MFC_DEC_RET_ERR_INVALID_HANDLE;
    }
    if ((lengthBufFill < 0) || (lengthBufFill > 0x100000)) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeExe", "lengthBufFill is invalid. (lengthBufFill=%d)\n", lengthBufFill);
        return SSBSIP_MFC_DEC_RET_ERR_INVALID_PARAM;
    }


    pCTX  = (_MFCLIB_DEC *) openHandle;


    switch (pCTX->decoder) {
    case SSBSIPMFCDEC_MPEG4:
    case SSBSIPMFCDEC_H263:
        ioctl_cmd = IOCTL_MFC_MPEG4_DEC_EXE;
        break;

    case SSBSIPMFCDEC_H264:
        ioctl_cmd = IOCTL_MFC_H264_DEC_EXE;
        break;

    case SSBSIPMFCDEC_VC1:
        ioctl_cmd = IOCTL_MFC_VC1_DEC_EXE;
        break;

    default:
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeExe", "Undefined codec type.\n");
        return SSBSIP_MFC_DEC_RET_ERR_UNDEF_CODEC;
    }


    if (!pCTX->fInit) {

        switch (pCTX->decoder) {
        case SSBSIPMFCDEC_MPEG4:
        case SSBSIPMFCDEC_H263:
            ioctl_cmd = IOCTL_MFC_MPEG4_DEC_INIT;
            break;

        case SSBSIPMFCDEC_H264:
            ioctl_cmd = IOCTL_MFC_H264_DEC_INIT;
            break;

        case SSBSIPMFCDEC_VC1:
            ioctl_cmd = IOCTL_MFC_VC1_DEC_INIT;
            break;
        }

        /////////////////////////////////////////////////
        /////           (DeviceIoControl)           /////
        /////       IOCTL_MFC_H264_DEC_EXE         /////
        /////////////////////////////////////////////////
        mfc_args.dec_init.in_strmSize = lengthBufFill;
        r = DeviceIoControl(pCTX->hOpen, ioctl_cmd,
                            &mfc_args, sizeof(MFC_ARGS),
                            NULL, 0,
                            NULL,
                            NULL);
        if ((r == FALSE) || (mfc_args.get_buf_addr.ret_code != 0)) {
            return SSBSIP_MFC_DEC_RET_ERR_CONFIG_FAIL;
        }

        // Output argument (width , height)
        pCTX->width  = mfc_args.dec_init.out_width;
        pCTX->height = mfc_args.dec_init.out_height;

        pCTX->fInit = 1;

        return SSBSIP_MFC_DEC_RET_OK;
    }

    /////////////////////////////////////////////////
    /////           (DeviceIoControl)           /////
    /////       IOCTL_MFC_H264_DEC_EXE         /////
    /////////////////////////////////////////////////
    mfc_args.dec_exe.in_strmSize = lengthBufFill;
    r = DeviceIoControl(pCTX->hOpen, ioctl_cmd,
                        &mfc_args, sizeof(MFC_ARGS),
                        NULL, 0,
                        NULL,
                        NULL);
    if ((r == FALSE) || (mfc_args.dec_exe.ret_code != 0)) {
        return SSBSIP_MFC_DEC_RET_ERR_DECODE_FAIL;
    }

    return SSBSIP_MFC_DEC_RET_OK;
}


int SsbSipMfcDecodeDeInit(void *openHandle)
{
    _MFCLIB_DEC  *pCTX;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeExe", "openHandle is NULL\n");
        return SSBSIP_MFC_DEC_RET_ERR_INVALID_HANDLE;
    }


    pCTX  = (_MFCLIB_DEC *) openHandle;


    CloseHandle(pCTX->hOpen);
    free(pCTX);

    return SSBSIP_MFC_DEC_RET_OK;
}


void *SsbSipMfcDecodeGetInBuf(void *openHandle, long *size)
{
    _MFCLIB_DEC     *pCTX;
    MFC_ARGS         mfc_args;
    BOOL             r;

    void       *pStrmBuf;
    DWORD       nStrmBufSize; 

    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetInBuf", "openHandle is NULL\n");
        return NULL;
    }
    if (size == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetInBuf", "size is NULL.\n");
        return NULL;
    }

    pCTX  = (_MFCLIB_DEC *) openHandle;



    /////////////////////////////////////////////////
    /////           (DeviceIoControl)           /////
    /////      IOCTL_MFC_GET_STRM_BUF_ADDR      /////
    /////////////////////////////////////////////////
//    mfc_args.get_buf_addr.in_usr_data = pCTX->proc_id;
    r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_GET_LINE_BUF_ADDR,
                        &mfc_args, sizeof(MFC_ARGS),
                        NULL, 0,
                        NULL,
                        NULL);
    if ((r == FALSE) || (mfc_args.get_buf_addr.ret_code != 0)) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetInBuf", "Failed in get RING_BUF address.\n");
        return NULL;
    }


    // Output arguments
    pStrmBuf     = (void *) mfc_args.get_buf_addr.out_buf_addr;    
    nStrmBufSize = mfc_args.get_buf_addr.out_buf_size;


    *size = nStrmBufSize;

    return pStrmBuf;
}


void *SsbSipMfcDecodeGetOutBuf(void *openHandle, long *size)
{
    _MFCLIB_DEC     *pCTX;
    MFC_ARGS         mfc_args;
    BOOL             r;

    void       *pFramBuf;
    DWORD       nFramBufSize;

    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetOutBuf", "openHandle is NULL\n");
        return NULL;
    }
    if (size == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetOutBuf", "size is NULL.\n");
        return NULL;
    }

    pCTX  = (_MFCLIB_DEC *) openHandle;


    /////////////////////////////////////////////////
    /////           (DeviceIoControl)           /////
    /////      IOCTL_MFC_GET_STRM_BUF_ADDR      /////
    /////////////////////////////////////////////////
//    mfc_args.get_buf_addr.in_usr_data = pCTX->proc_id;
    r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_GET_FRAM_BUF_ADDR,
                        &mfc_args, sizeof(MFC_ARGS),
                        NULL, 0,
                        NULL,
                        NULL);
    if ((r == FALSE) || (mfc_args.get_buf_addr.ret_code != 0)) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetOutBuf", "Failed in get FRAM_BUF address.\n");
        return NULL;
    }

    // Output arguments
    pFramBuf     = (void *) mfc_args.get_buf_addr.out_buf_addr;
    nFramBufSize = mfc_args.get_buf_addr.out_buf_size;

    *size = nFramBufSize;

    return pFramBuf;
}



int SsbSipMfcDecodeSetConfig(void *openHandle, MFC_DEC_CONF conf_type, void *value)
{
    _MFCLIB_DEC  *pCTX;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeSetConfig", "openHandle is NULL\n");
        return SSBSIP_MFC_DEC_RET_ERR_INVALID_HANDLE;
    }

    pCTX  = (_MFCLIB_DEC *) openHandle;


    return SSBSIP_MFC_DEC_RET_OK;
}


int SsbSipMfcDecodeGetConfig(void *openHandle, MFC_DEC_CONF conf_type, void *value)
{
    _MFCLIB_DEC  *pCTX;
    MFC_ARGS            mfc_args;
    BOOL                r;


    ////////////////////////////////
    //  Input Parameter Checking  //
    ////////////////////////////////
    if (openHandle == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetConfig", "openHandle is NULL\n");
        return SSBSIP_MFC_DEC_RET_ERR_INVALID_HANDLE;
    }
    if (value == NULL) {
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetConfig", "value is NULL\n");
        return SSBSIP_MFC_DEC_RET_ERR_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB_DEC *) openHandle;


    switch (conf_type) {

    case MFC_DEC_GETCONF_STREAMINFO:
        ((SSBSIP_MFC_STREAM_INFO *)value)->width  = pCTX->width;
        ((SSBSIP_MFC_STREAM_INFO *)value)->height = pCTX->height;
        break;

    case MFC_DEC_GETCONF_PHYADDR_FRAM_BUF:

        /////////////////////////////////////////////////
        /////           (DeviceIoControl)           /////
        /////      IOCTL_MFC_GET_FRAM_BUF_ADDR      /////
        /////////////////////////////////////////////////
//        mfc_args.get_buf_addr.in_usr_data = pCTX->proc_id;
        r = DeviceIoControl(pCTX->hOpen, IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR,
                            &mfc_args, sizeof(MFC_ARGS),
                            NULL, 0,
                            NULL,
                            NULL);
        if ((r == FALSE) || (mfc_args.get_buf_addr.ret_code != 0)) {
            LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetConfig", "Error in MFC_DEC_GETCONF_PHYADDR_FRAM_BUF.\n");
            return SSBSIP_MFC_DEC_RET_ERR_GETCONF_FAIL;
        }

        // Output arguments
        ((unsigned int*) value)[0] = mfc_args.get_buf_addr.out_buf_addr;
        ((unsigned int*) value)[1] = mfc_args.get_buf_addr.out_buf_size;

        break;

    default:
        LOG_MSG(LOG_ERROR, "SsbSipMfcDecodeGetConfig", "No such conf_type is supported.\n");
        return SSBSIP_MFC_DEC_RET_ERR_GETCONF_FAIL;
    }


    return SSBSIP_MFC_DEC_RET_OK;
}


