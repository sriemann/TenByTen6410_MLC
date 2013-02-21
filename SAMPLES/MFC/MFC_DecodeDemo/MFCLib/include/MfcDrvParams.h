#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_DRV_PARAMS_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_DRV_PARAMS_H__

typedef struct {
    int ret_code;            // [OUT] Return code
    int in_width;            // [IN]  width  of YUV420 frame to be encoded
    int in_height;            // [IN]  height of YUV420 frame to be encoded
    int in_bitrate;            // [IN]  Encoding parameter: Bitrate (kbps)
    int in_gopNum;            // [IN]  Encoding parameter: GOP Number (interval of I-frame)
    int in_frameRateRes;    // [IN]  Encoding parameter: Frame rate (Res)
    int in_frameRateDiv;    // [IN]  Encoding parameter: Frame rate (Divider)
} MFC_ENC_INIT_ARG;

typedef struct {
    int ret_code;            // [OUT] Return code
    int out_encoded_size;    // [OUT] Length of Encoded video stream
    int out_header_size;    // [OUT] Length of video stream header
} MFC_ENC_EXE_ARG;

typedef struct {
    int ret_code;            // [OUT] Return code
    int in_strmSize;        // [IN]  Size of video stream filled in STRM_BUF
    int out_width;            // [OUT] width  of YUV420 frame
    int out_height;            // [OUT] height of YUV420 frame
} MFC_DEC_INIT_ARG;

typedef struct {
    int ret_code;            // [OUT] Return code
    int in_strmSize;        // [IN]  Size of video stream filled in STRM_BUF
} MFC_DEC_EXE_ARG;

typedef struct {
    int ret_code;            // [OUT] Return code
    int in_usr_data;        // [IN]  User data for translating Kernel-mode address to User-mode address
    int out_buf_addr;        // [OUT] Buffer address
    int out_buf_size;        // [OUT] Size of buffer address
} MFC_GET_BUF_ADDR_ARG;

typedef struct {
    int ret_code;                    // [OUT] Return code
    int in_config_param;            // [IN]  Configurable parameter type
    int out_config_value[2];        // [IN]  Values to get for the configurable parameter.
                                    //       Maximum two integer values can be obtained;
} MFC_GET_CONFIG_ARG;

typedef struct {
    int ret_code;                    // [OUT] Return code
    int in_config_param;            // [IN]  Configurable parameter type
    int in_config_value[2];            // [IN]  Values to be set for the configurable parameter.
                                    //       Maximum two integer values can be set.
    int out_config_value_old[2];    // [OUT] Old values of the configurable parameters
} MFC_SET_CONFIG_ARG;

typedef union {
    MFC_ENC_INIT_ARG        enc_init;
    MFC_ENC_EXE_ARG            enc_exe;
    MFC_DEC_INIT_ARG        dec_init;
    MFC_DEC_EXE_ARG            dec_exe;
    MFC_GET_BUF_ADDR_ARG    get_buf_addr;
    MFC_GET_CONFIG_ARG      get_config;
    MFC_SET_CONFIG_ARG      set_config;
} MFC_ARGS;


#define MFC_GET_CONFIG_DEC_FRAME_NEED_COUNT        (0x0AA0C001)



#define MFC_SET_CONFIG_DEC_ROTATE                (0x0ABDE001)

#define MFC_SET_CONFIG_ENC_H263_PARAM            (0x0ABDC001)
#define MFC_SET_CONFIG_ENC_SLICE_MODE            (0x0ABDC002)
#define MFC_SET_CONFIG_ENC_PARAM_CHANGE            (0x0ABDC003)
#define MFC_SET_CONFIG_ENC_CUR_PIC_OPT            (0x0ABDC004)



#define ENC_PARAM_GOP_NUM            (0x7000A001)
#define ENC_PARAM_INTRA_QP            (0x7000A002)
#define ENC_PARAM_BITRATE            (0x7000A003)
#define ENC_PARAM_F_RATE            (0x7000A004)
#define ENC_PARAM_INTRA_REF            (0x7000A005)
#define ENC_PARAM_SLICE_MODE        (0x7000A006)

#define ENC_PIC_OPT_IDR                (0x7000B001)
#define ENC_PIC_OPT_SKIP            (0x7000B002)
#define ENC_PIC_OPT_RECOVERY        (0x7000B003)


#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_DRV_PARAMS_H__ */


