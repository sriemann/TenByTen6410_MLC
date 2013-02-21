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


#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_INST_INIT_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_INST_INIT_H__


#include "Mfc.h"
#include "MfcTypes.h"

typedef unsigned int    PHYADDR_VAL;

typedef enum
{
    MFC_CONFIG_PHY_OUTBUF,
    MFC_CONFIG_OUT_POSITION
} MFC_CONFIG;

typedef struct
{
    int  width,  height;
    int  x_offset, y_offset;
} MFC_OUTBUF_POSITION;


typedef enum
{
    MFCINST_STATE_DELETED = 0,            // Instance is deleted

    MFCINST_STATE_CREATED         = 10,    // Instance is created but not initialized
    MFCINST_STATE_DEC_INITIALIZED = 20,    // Instance is initialized for decoding
    MFCINST_STATE_DEC_PIC_RUN_LINE_BUF,

    MFCINST_STATE_ENC_INITIALIZED = 30,    // Instance is initialized for encoding
    MFCINST_STATE_ENC_PIC_RUN_LINE_BUF,
} MFCINST_STATE;


#define MFCINST_STATE_PWR_OFF_FLAG        0x40000000        // MFC
#define MFCINST_STATE_BUF_FILL_REQ        0x80000000

#define MFCINST_STATE_TRANSITION(inst_ctx, state)    ((inst_ctx)->state_var =  (state))
#define MFCINST_STATE(inst_ctx)                        ((inst_ctx)->state_var & 0x0FFFFFFF)
#define MFCINST_STATE_CHECK(inst_ctx, state)        (((inst_ctx)->state_var & 0x0FFFFFFF) == (state))

#define MFCINST_STATE_PWR_OFF_FLAG_SET(inst_ctx)    ((inst_ctx)->state_var |= MFCINST_STATE_PWR_OFF_FLAG)
#define MFCINST_STATE_PWR_OFF_FLAG_CLEAR(inst_ctx)    ((inst_ctx)->state_var &= ~MFCINST_STATE_PWR_OFF_FLAG)
#define MFCINST_STATE_PWR_OFF_FLAG_CHECK(inst_ctx)    ((inst_ctx)->state_var & MFCINST_STATE_PWR_OFF_FLAG)
#define MFCINST_STATE_BUF_FILL_REQ_SET(inst_ctx)    ((inst_ctx)->state_var |= MFCINST_STATE_BUF_FILL_REQ)
#define MFCINST_STATE_BUF_FILL_REQ_CLEAR(inst_ctx)    ((inst_ctx)->state_var &= ~MFCINST_STATE_BUF_FILL_REQ)
#define MFCINST_STATE_BUF_FILL_REQ_CHECK(inst_ctx)    ((inst_ctx)->state_var & MFCINST_STATE_BUF_FILL_REQ)

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
typedef struct{
    DWORD                p_addr;  // physical address
    UINT8                *v_addr;  // virtual address of cached area
    UINT8                *u_addr;  // copyed virtual address for user mode process
    UINT32                size;       // memory size    
}MFC_DIVX_ADDR_TABLE;
#endif


typedef struct
{
    int             inst_no;

    MFC_CODECMODE   codec_mode;

    unsigned char  *pStrmBuf;            // STRM_BUF pointer (virtual address)
    PHYADDR_VAL     phyadrStrmBuf;       // STRM_BUF physical address
    unsigned int    nStrmBufSize;        // STRM_BUF size

    unsigned char  *pFramBuf;            // FRAM_BUF pointer (virtual address)
    PHYADDR_VAL     phyadrFramBuf;       // FRAM_BUF physical address
    unsigned int    nFramBufSize;        // FRAM_BUF size
    PHYADDR_VAL     mv_mbyte_addr;       // Phyaical address of MV and MByte tables

    int             framBufAllocated;
    int             cnf_PhyOutBuf;

    MFC_OUTBUF_POSITION  cnf_OutPos;

    int             width,     height;
    int             buf_width, buf_height;    // buf_width is stride.

    int             crop_left, crop_right, crop_top, crop_bottom;   // crop

    int             frambufCnt;   // Decoding case: RET_DEC_SEQ_FRAME_NEED_COUNT
                                  // Encoding case: fixed at 2 (at lease 2 frame buffers)
    

    // decoding configuration info
    unsigned int    PostRotMode;

    // decoding picture option
    unsigned int    dec_pic_option;    // 0-th bit : MP4ASP FLAG,
                                       // 1-st bit : MV REPORT ENABLE,
                                       // 2-nd bit : MBTYPE REPORT ENABLE

    // encoding configuration info
    int             frameRateRes;
    int             frameRateDiv;
    int             gopNum;
    int             bitrate;
// Add For VideoEditor
    int                vopTimeIncResol;
    int                level;

    int             intraqp;
    int             qpmax;
    float          gamma;

    // encoding configuration info (misc.)
    int             h263_annex;
    int             enc_num_slices;

    // encoding picture option
    unsigned int    enc_pic_option;    // 0-th bit : ENC_PIC_OPT_SKIP,
                                       // 1-st bit : ENC_PIC_OPT_IDR,
                                       // 24-th bit: ENC_PIC_OPT_RECOVERY


    int             frame_num;         // DEC_PIC_FRAME_NUM
    int             run_index;         // DEC_PIC_RUN_IDX


    MFCINST_STATE   state_var;         // State Variable

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
    HANDLE          callerProcess;
    UINT            paddingSize;

    /* RET_DEC_PIC_RUN_BAK_XXXXX : MP4ASP(DivX) related values that is returned        */
    /*                         on DEC_PIC_RUN command.                                 */
    /* RET_DEC_SEQ_INIT_BAK_XXXXX : MP4ASP(DivX) related values that is returned       */
    /*                         on DEC_SEQ_INIT command.                                */
    /* They are maintained in the context structure variable.                          */
    UINT            RET_DEC_SEQ_INIT_BAK_MP4ASP_VOP_TIME_RES;
    UINT            RET_DEC_PIC_RUN_BAK_BYTE_CONSUMED;
    UINT            RET_DEC_PIC_RUN_BAK_MP4ASP_FCODE;
    UINT            RET_DEC_PIC_RUN_BAK_MP4ASP_TIME_BASE_LAST;
    UINT            RET_DEC_PIC_RUN_BAK_MP4ASP_NONB_TIME_LAST;
    UINT            RET_DEC_PIC_RUN_BAK_MP4ASP_MP4ASP_TRD;
#endif

    UINT            RET_DEC_PIC_RUN_BAK_H264_CROP_LEFT;
    UINT            RET_DEC_PIC_RUN_BAK_H264_CROP_RIGHT;
    UINT            RET_DEC_PIC_RUN_BAK_H264_CROP_TOP;
    UINT            RET_DEC_PIC_RUN_BAK_H264_CROP_BOTTOM;
    
    UINT            RET_ENC_PIC_RUN_BAK_PIC_TYPE;

} MFCInstCtx;

typedef struct {
    int disp_width;
    int disp_height;
} disp_info_t;

typedef struct {
    int width;
    int height;
    int frameRateRes;
    int frameRateDiv;
    int gopNum;
    int bitrate;
    
    int intraqp;
    int qpmax;
    float gamma;
} enc_info_t;

#define MFCINST_MP4_QPMAX  31
#define MFCINST_MP4_QPMIN  1
#define MFCINST_H264_QPMAX  51
#define MFCINST_H264_QPMIN  0

#define MFCINST_GAMMA_FACTOR 0.75
#define MFCINST_GAMMA_FACTEE 32768

typedef struct
{
    int  width;
    int  height;
} MFC_DECODED_FRAME_INFO;


#ifdef __cplusplus
extern "C" {
#endif


MFCInstCtx *MFCInst_GetCtx(int inst_no);


void MFCInst_RingBufAddrCorrection(MFCInstCtx *ctx);
int  MFCInst_GetInstNo(MFCInstCtx *ctx);
BOOL MFCInst_GetStreamRWPtrs(MFCInstCtx *ctx, unsigned char **ppRD_PTR, unsigned char **ppWR_PTR);

MFCInstCtx *MFCInst_Create(void);
void MFCInst_Delete(MFCInstCtx *ctx);

void MFCInst_PowerOffState(MFCInstCtx *ctx);
void MFCInst_PowerOnState(MFCInstCtx *ctx);

int  MFCInst_Dec_Init(MFCInstCtx *ctx, MFC_CODECMODE codec_mode, unsigned long strm_leng);
int  MFCInst_Decode(MFCInstCtx *ctx, unsigned long arg);
int  MFCInst_Decode_Stream(MFCInstCtx *ctx, unsigned long strm_leng);

int  MFCInst_Enc_Init(MFCInstCtx *ctx, MFC_CODECMODE codec_mode, enc_info_t *enc_info);
int  MFCInst_Encode(MFCInstCtx *ctx, int *enc_data_size, int *header_size);
int  MFCInst_EncHeader(MFCInstCtx *ctx, int hdr_code, int hdr_num, unsigned int outbuf_physical_addr, int outbuf_size, int *hdr_size);
int  MFCInst_EncParamChange(MFCInstCtx *ctx, unsigned int param_change_enable, unsigned int param_change_val);

void MFCInst_Set_Disp_Config(MFCInstCtx *ctx, int arg);
void MFCInst_Get_Frame_size(MFCInstCtx *ctx, int arg);

unsigned int MFCInst_Set_PostRotate(MFCInstCtx *ctx, unsigned int post_rotmode);

int  MFCInst_GetLineBuf(MFCInstCtx *ctx, unsigned char **ppBuf, int *size);
int  MFCInst_GetRingBuf(MFCInstCtx *ctx, unsigned char **ppBuf, int *size);
int  MFCInst_GetFramBuf(MFCInstCtx *ctx, unsigned char **ppBuf, int *size);
int  MFCInst_GetFramBufPhysical(MFCInstCtx *ctx, unsigned char **ppBuf, int *size);


#ifdef __cplusplus
}
#endif


#define MFCINST_RET_OK                                 (0)
#define MFCINST_ERR_INVALID_PARAM                      (-1001)
#define MFCINST_ERR_STATE_CHK                          (-1002)
#define MFCINST_ERR_STATE_DELETED                      (-1003)
//#define MFCINST_ERR_STATE_INVALIDATED                (-1004)
#define MFCINST_ERR_STATE_POWER_OFF                    (-1004)
#define MFCINST_ERR_WRONG_CODEC_MODE                   (-1005)

#define MFCINST_ERR_DEC_INIT_CMD_FAIL                  (-2001)
#define MFCINST_ERR_DEC_PIC_RUN_CMD_FAIL               (-2002)
#define MFCINST_ERR_DEC_DECODE_FAIL_ETC                (-2011)
#define MFCINST_ERR_DEC_INVALID_STRM                   (-2012)
#define MFCINST_ERR_DEC_EOS                            (-2013)
//#define MFCINST_ERR_DEC_DECODE_NO_DISP               (-2015)
#define MFCINST_ERR_DEC_BUF_FILL_SIZE_WRONG            (-2014)

#define MFCINST_ERR_ENC_INIT_CMD_FAIL                  (-3001)
#define MFCINST_ERR_ENC_PIC_RUN_CMD_FAIL               (-3002)
#define MFCINST_ERR_ENC_HEADER_CMD_FAIL                (-3003)
#define MFCINST_ERR_ENC_PARAM_CHANGE_INVALID_VALUE     (-3011)

#define MFCINST_ERR_MEMORY_ALLOCATION_FAIL             (-4001)

#define MFCINST_ERR_ETC                                (-9001)
#define MFCINST_ERR_INTEGER_OVERFLOW                   (-9002)
#define MFCINST_ERR_INTEGER_UNDERFLOW                  (-9003)



#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_INST_INIT_H__ */
