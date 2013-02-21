//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_DRV_PARAMS_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_DRV_PARAMS_H__

//#include "MfcTypes.h"

/* MFC IOCTL Code Definition */
#define IOCTL_MFC_MPEG4_DEC_INIT               (0x00800001)
#define IOCTL_MFC_MPEG4_ENC_INIT               (0x00800002)
#define IOCTL_MFC_MPEG4_DEC_EXE                (0x00800003)
#define IOCTL_MFC_MPEG4_ENC_EXE                (0x00800004)

#define IOCTL_MFC_H264_DEC_INIT                (0x00800011)
#define IOCTL_MFC_H264_ENC_INIT                (0x00800012)
#define IOCTL_MFC_H264_DEC_EXE                 (0x00800013)
#define IOCTL_MFC_H264_ENC_EXE                 (0x00800014)

#define IOCTL_MFC_H263_DEC_INIT                (0x00800021)
#define IOCTL_MFC_H263_ENC_INIT                (0x00800022)
#define IOCTL_MFC_H263_DEC_EXE                 (0x00800023)
#define IOCTL_MFC_H263_ENC_EXE                 (0x00800024)

#define IOCTL_MFC_VC1_DEC_INIT                 (0x00800031)
#define IOCTL_MFC_VC1_DEC_EXE                  (0x00800032)

#define IOCTL_MFC_GET_LINE_BUF_ADDR            (0x00800101)
#define IOCTL_MFC_GET_FRAM_BUF_ADDR            (0x00800103)
#define IOCTL_MFC_GET_POST_BUF_ADDR            (0x00800104)
#define IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR        (0x00800105)
#define IOCTL_MFC_SET_CONFIG                   (0x00800106)
#define IOCTL_MFC_GET_CONFIG                   (0x00800107)
#define IOCTL_MFC_GET_MPEG4_ASP_PARAM    (0x00800108)

#define IOCTL_MFC_SET_DISP_CONFIG              (0x00800111)
#define IOCTL_MFC_GET_FRAME_SIZE               (0x00800112)
#define IOCTL_MFC_SET_PP_DISP_SIZE             (0x00800113)
#define IOCTL_MFC_SET_DEC_INBUF_TYPE           (0x00800114)


/* MFC Parameter Definition */
typedef struct {
    int ret_code;                   // [OUT] Return code
    int in_width;                   // [IN]  width  of YUV420 frame to be encoded
    int in_height;                  // [IN]  height of YUV420 frame to be encoded
    int in_bitrate;                 // [IN]  Encoding parameter: Bitrate (kbps)
    int in_gopNum;                  // [IN]  Encoding parameter: GOP Number (interval of I-frame)
    int in_frameRateRes;            // [IN]  Encoding parameter: Frame rate (Res)
    int in_frameRateDiv;            // [IN]  Encoding parameter: Frame rate (Divider)
    int in_intraqp;            // [IN] Encoding Parameter: Intra Quantization Parameter
    int in_qpmax;            // [IN] Encoding Paramter: Maximum Quantization Paramter
    float in_gamma;            // [IN] Encoding Paramter: Gamma Factor for Motion Estimation
} MFC_ENC_INIT_ARG;

typedef struct {
    int ret_code;                   // [OUT] Return code
    int out_encoded_size;           // [OUT] Length of Encoded video stream
    int out_header_size;            // [OUT] Length of video stream header
} MFC_ENC_EXE_ARG;

typedef struct {
    int ret_code;                   // [OUT] Return code
    int in_strmSize;                // [IN]  Size of video stream filled in STRM_BUF
    int out_width;                  // [OUT] width  of YUV420 frame
    int out_height;                 // [OUT] height of YUV420 frame
} MFC_DEC_INIT_ARG;

typedef struct {
    int ret_code;                   // [OUT] Return code
    int in_strmSize;                // [IN]  Size of video stream filled in STRM_BUF
} MFC_DEC_EXE_ARG;

typedef struct {
    int ret_code;                   // [OUT] Return code
    int out_buf_addr;               // [OUT] Buffer address
    int out_buf_size;               // [OUT] Size of buffer address
} MFC_GET_BUF_ADDR_ARG;

typedef struct {
    int ret_code;                   // [OUT] Return code
    int in_config_param;            // [IN]  Configurable parameter type
    int out_config_value[2];        // [IN]  Values to get for the configurable parameter.
                                    //       Maximum two integer values can be obtained;
} MFC_GET_CONFIG_ARG;

typedef struct {
    int ret_code;                   // [OUT] Return code
    int in_config_param;            // [IN]  Configurable parameter type
    int in_config_value[2];         // [IN]  Values to be set for the configurable parameter.
                                    //       Maximum two integer values can be set.
    int out_config_value_old[2];    // [OUT] Old values of the configurable parameters
} MFC_SET_CONFIG_ARG;


typedef struct {
    int   ret_code;            // [OUT] Return code
    int   mv_addr;
    int   mb_type_addr;
    UINT  mv_size;
    UINT  mb_type_size;
    UINT  mp4asp_vop_time_res;
    UINT  byte_consumed;
    UINT  mp4asp_fcode;
    UINT  mp4asp_time_base_last;
    UINT  mp4asp_nonb_time_last;
    UINT  mp4asp_trd;
} MFC_GET_MPEG4ASP_ARG;

typedef union {
    MFC_ENC_INIT_ARG        enc_init;
    MFC_ENC_EXE_ARG         enc_exe;
    MFC_DEC_INIT_ARG        dec_init;
    MFC_DEC_EXE_ARG         dec_exe;
    MFC_GET_BUF_ADDR_ARG    get_buf_addr;
    MFC_GET_CONFIG_ARG      get_config;
    MFC_SET_CONFIG_ARG      set_config;
    MFC_GET_MPEG4ASP_ARG    mpeg4_asp_param;
} MFC_ARGS;


#define MFC_GET_CONFIG_DEC_FRAME_NEED_COUNT         (0x0AA0C001)
#define MFC_GET_CONFIG_DEC_MP4ASP_MV                (0x0AA0C002)
#define MFC_GET_CONFIG_DEC_MP4ASP_MBTYPE            (0x0AA0C003)
#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
#define MFC_GET_CONFIG_DEC_MP4ASP_FCODE             (0x0AA0C011)
#define MFC_GET_CONFIG_DEC_MP4ASP_VOP_TIME_RES      (0x0AA0C012)
#define MFC_GET_CONFIG_DEC_MP4ASP_TIME_BASE_LAST    (0x0AA0C013)
#define MFC_GET_CONFIG_DEC_MP4ASP_NONB_TIME_LAST    (0x0AA0C014)
#define MFC_GET_CONFIG_DEC_MP4ASP_TRD               (0x0AA0C015)
#define MFC_GET_CONFIG_DEC_BYTE_CONSUMED            (0x0AA0C016)
#endif

#define MFC_SET_CONFIG_DEC_ROTATE                   (0x0ABDE001)
#define MFC_SET_CONFIG_DEC_OPTION                   (0x0ABDE002)

#define MFC_SET_CONFIG_ENC_H263_PARAM               (0x0ABDC001)
#define MFC_SET_CONFIG_ENC_SLICE_MODE               (0x0ABDC002)
#define MFC_SET_CONFIG_ENC_PARAM_CHANGE             (0x0ABDC003)
#define MFC_SET_CONFIG_ENC_CUR_PIC_OPT              (0x0ABDC004)

#define MFC_SET_CACHE_CLEAN                         (0x0ABDD001)
#define MFC_SET_CACHE_INVALIDATE                    (0x0ABDD002)
#define MFC_SET_CACHE_CLEAN_INVALIDATE              (0x0ABDD003)

#define MFC_SET_PADDING_SIZE                        (0x0ABDE003)

#define ENC_PARAM_GOP_NUM                           (0x7000A001)
#define ENC_PARAM_INTRA_QP                          (0x7000A002)
#define ENC_PARAM_BITRATE                           (0x7000A003)
#define ENC_PARAM_F_RATE                            (0x7000A004)
#define ENC_PARAM_INTRA_REF                         (0x7000A005)
#define ENC_PARAM_SLICE_MODE                        (0x7000A006)

#define ENC_PIC_OPT_IDR                             (0x7000B001)
#define ENC_PIC_OPT_SKIP                            (0x7000B002)
#define ENC_PIC_OPT_RECOVERY                        (0x7000B003)


#define DEC_PIC_OPT_MP4ASP                          (0x7000C001)


#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_DRV_PARAMS_H__ */


