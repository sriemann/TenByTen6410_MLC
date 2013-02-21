//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/



#include "MfcDrvParams.h"
#include "MFC_Instance.h"
#include "LogMsg.h"
#include "MfcSfr.h"
#include "CacheOpr.h"



int MFC_GetConfigParams(
                         MFCInstCtx  *pMfcInst,
                         MFC_ARGS     *args            // Input arguments for IOCTL_MFC_SET_CONFIG
                        )
{
    int             ret;

    switch (args->get_config.in_config_param) {

    case MFC_GET_CONFIG_DEC_FRAME_NEED_COUNT:
        args->get_config.out_config_value[0] = pMfcInst->frambufCnt;
        ret = MFCINST_RET_OK;

        break;

    case MFC_GET_CONFIG_DEC_MP4ASP_MV:
    case MFC_GET_CONFIG_DEC_MP4ASP_MBTYPE:

        // "MFC_GET_CONFIG_DEC_MP4ASP_MV" and "MFC_GET_CONFIG_DEC_MP4ASP_MBTYPE" are processed in the upper function.
        ret = MFCINST_RET_OK;

        break;

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
    case MFC_GET_CONFIG_DEC_BYTE_CONSUMED:
        args->get_config.out_config_value[0] = (int)pMfcInst->RET_DEC_PIC_RUN_BAK_BYTE_CONSUMED; 
        ret = MFCINST_RET_OK;
        break;

    case MFC_GET_CONFIG_DEC_MP4ASP_FCODE:
        args->get_config.out_config_value[0] = (int)pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_FCODE;
        ret = MFCINST_RET_OK;
        break;
        
    case MFC_GET_CONFIG_DEC_MP4ASP_VOP_TIME_RES:
        args->get_config.out_config_value[0] = (int)pMfcInst->RET_DEC_SEQ_INIT_BAK_MP4ASP_VOP_TIME_RES;
        ret = MFCINST_RET_OK;
        break;
        
    case MFC_GET_CONFIG_DEC_MP4ASP_TIME_BASE_LAST:
        args->get_config.out_config_value[0] = (int)pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_TIME_BASE_LAST;
        ret = MFCINST_RET_OK;
        break;
        
    case MFC_GET_CONFIG_DEC_MP4ASP_NONB_TIME_LAST:
        args->get_config.out_config_value[0] = (int)pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_NONB_TIME_LAST;
        ret = MFCINST_RET_OK;
        break;
        
    case MFC_GET_CONFIG_DEC_MP4ASP_TRD:
        args->get_config.out_config_value[0] = (int)pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_MP4ASP_TRD;
        ret = MFCINST_RET_OK;
        break;
#endif    

    case MFC_GET_CONFIG_DEC_H264_CROP_LEFT_RIGHT :
        args->get_config.out_config_value[0] = (int)pMfcInst->crop_left;
        args->get_config.out_config_value[1] = (int)pMfcInst->crop_right;
        ret = MFCINST_RET_OK;
        break;
        
    case MFC_GET_CONFIG_DEC_H264_CROP_TOP_BOTTOM :
        args->get_config.out_config_value[0] = (int)pMfcInst->crop_top;
        args->get_config.out_config_value[1] = (int)pMfcInst->crop_bottom;
        ret = MFCINST_RET_OK;
        break;   
        
    case MFC_GET_CONFIG_ENC_PIC_TYPE:
        args->get_config.out_config_value[0] = (int)pMfcInst->RET_ENC_PIC_RUN_BAK_PIC_TYPE;
        ret = MFCINST_RET_OK;
        break;

    default:
        ret = -1;
    }


    // Output arguments for IOCTL_MFC_SET_CONFIG
    args->get_config.ret_code = ret;

    return MFCINST_RET_OK;
}

int MFC_SetConfigParams(
                         MFCInstCtx  *pMfcInst,
                         MFC_ARGS     *args            // Input arguments for IOCTL_MFC_SET_CONFIG
                        )
{
    int             ret;
    unsigned int    param_change_enable, param_change_val;

    param_change_enable = 0;
    switch (args->set_config.in_config_param) {

    case MFC_SET_CONFIG_DEC_ROTATE:
#if (MFC_ROTATE_ENABLE == 1)
        args->set_config.out_config_value_old[0]
                = MFCInst_Set_PostRotate(pMfcInst, args->set_config.in_config_value[0]);
#else
        LOG_MSG(LOG_ERROR, "MFC_SetConfigParams", "IOCTL_MFC_SET_CONFIG with MFC_SET_CONFIG_DEC_ROTATE is not supported.\r\n");
        LOG_MSG(LOG_ERROR, "MFC_SetConfigParams", "Please check if MFC_ROTATE_ENABLE is defined as 1 in MfcConfig.h file.\r\n");
#endif
        ret = MFCINST_RET_OK;
        break;

    case MFC_SET_CONFIG_DEC_OPTION:

        ret = MFCINST_RET_OK;
        break;

    case MFC_SET_CONFIG_ENC_H263_PARAM:
        args->set_config.out_config_value_old[0] = pMfcInst->h263_annex;
        pMfcInst->h263_annex = args->set_config.in_config_value[0];
        ret = MFCINST_RET_OK;
        break;


    case MFC_SET_CONFIG_ENC_SLICE_MODE:

        if (pMfcInst->enc_num_slices) {
            args->set_config.out_config_value_old[0] = 1;
            args->set_config.out_config_value_old[1] = pMfcInst->enc_num_slices;
        }
        else {
            args->set_config.out_config_value_old[0] = 0;
            args->set_config.out_config_value_old[1] = 0;
        }

        if (args->set_config.in_config_value[0])
            pMfcInst->enc_num_slices = args->set_config.in_config_value[1];
        else
            pMfcInst->enc_num_slices = 0;

        ret = MFCINST_RET_OK;
        break;

    case MFC_SET_CONFIG_ENC_PARAM_CHANGE:

        switch (args->set_config.in_config_value[0]) {
        case ENC_PARAM_GOP_NUM:
            param_change_enable = (1 << 0);
            break;

        case ENC_PARAM_INTRA_QP:
            param_change_enable = (1 << 1);
            break;

        case ENC_PARAM_BITRATE:
            param_change_enable = (1 << 2);
            break;

        case ENC_PARAM_F_RATE:
            param_change_enable = (1 << 3);
            break;

        case ENC_PARAM_INTRA_REF:
            param_change_enable = (1 << 4);
            break;

        case ENC_PARAM_SLICE_MODE:
            param_change_enable = (1 << 5);
            break;

        default:
            break;
        }

        param_change_val  = args->set_config.in_config_value[1];
        ret = MFCInst_EncParamChange(pMfcInst, param_change_enable, param_change_val);

        break;

    case MFC_SET_CONFIG_ENC_CUR_PIC_OPT:

        switch (args->set_config.in_config_value[0]) {
        case ENC_PIC_OPT_IDR:
            pMfcInst->enc_pic_option ^= (args->set_config.in_config_value[1] << 1);
            break;

        case ENC_PIC_OPT_SKIP:
            pMfcInst->enc_pic_option ^= (args->set_config.in_config_value[1] << 0);
            break;

        case ENC_PIC_OPT_RECOVERY:
            pMfcInst->enc_pic_option ^= (args->set_config.in_config_value[1] << 24);
            break;

        default:
            break;
        }

        ret = MFCINST_RET_OK;
        break;

    case MFC_SET_CACHE_CLEAN:
        //printf("MFC_SET_CACHE_CLEAN : start(0x%08x) size(0x%x)\n",  args->set_config.in_config_value[0], args->set_config.in_config_value[1]);
        CleanCacheRange((PBYTE )args->set_config.in_config_value[0], (PBYTE )(args->set_config.in_config_value[0] + args->set_config.in_config_value[1] ) );
        ret = MFCINST_RET_OK;
        break;

    case MFC_SET_CACHE_INVALIDATE:
        //printf("MFC_SET_CACHE_INVALIDATE : start(0x%08x) size(0x%x)\n",  args->set_config.in_config_value[0], args->set_config.in_config_value[1]);
        InvalidateCacheRange((PBYTE )args->set_config.in_config_value[0], (PBYTE )(args->set_config.in_config_value[0] + args->set_config.in_config_value[1] ) );
        ret = MFCINST_RET_OK;
        break;

    case MFC_SET_CACHE_CLEAN_INVALIDATE:
        //printf("MFC_SET_CACHE_CLEAN_INVALIDATE : start(0x%08x) size(0x%x)\n",  args->set_config.in_config_value[0], args->set_config.in_config_value[1]);
        CleanInvalidateCacheRange((PBYTE )args->set_config.in_config_value[0], (PBYTE )(args->set_config.in_config_value[0] + args->set_config.in_config_value[1] ) );
        ret = MFCINST_RET_OK;
        break;

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
    case MFC_SET_PADDING_SIZE:
//        printf("MFC_SET_PADDING_SIZE : paddingsize(%d)\n",  args->set_config.in_config_value[0]);
        pMfcInst->paddingSize = args->set_config.in_config_value[0];
        ret = MFCINST_RET_OK;
        break;
#endif

    default:
        ret = -1;
    }

    // Output arguments for IOCTL_MFC_SET_CONFIG
    args->set_config.ret_code = ret;

    return MFCINST_RET_OK;
}

