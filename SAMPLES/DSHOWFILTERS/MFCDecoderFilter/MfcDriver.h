//------------------------------------------------------------------------------
// File: MfcDriver.h
//
// Desc: Header file from MFC Device Driver
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

DWORD MFC_Init(DWORD dwContext);
BOOL  MFC_Deinit(DWORD InitHandle);

DWORD MFC_Open(DWORD InitHandle, DWORD dwAccess, DWORD dwShareMode);
BOOL  MFC_Close(DWORD OpenHandle);

BOOL  MFC_IOControl(DWORD OpenHandle, DWORD dwIoControlCode,
                    PBYTE pInBuf, DWORD nInBufSize, PBYTE pOutBuf,
                    DWORD nOutBufSize,
                    PDWORD pBytesReturned);


#ifdef __cplusplus
}
#endif



#define IOCTL_MFC_MPEG4_DEC_INIT        (0x00800001)
#define IOCTL_MFC_MPEG4_ENC_INIT        (0x00800002)
#define IOCTL_MFC_MPEG4_DEC_EXE            (0x00800003)
#define IOCTL_MFC_MPEG4_ENC_EXE            (0x00800004)
#define IOCTL_MFC_MPEG4_DEC_PP_EXE        (0x00800005)
#define IOCTL_MFC_MPEG4_DEC_PP_DISP_EXE    (0x00800006)

#define IOCTL_MFC_H264_DEC_INIT            (0x00800007)
#define IOCTL_MFC_H264_ENC_INIT            (0x00800008)
#define IOCTL_MFC_H264_DEC_EXE            (0x00800009)
#define IOCTL_MFC_H264_ENC_EXE            (0x0080000A)
#define IOCTL_MFC_H264_DEC_PP_EXE        (0x0080000B)
#define IOCTL_MFC_H264_DEC_PP_DISP_EXE    (0x0080000C)

#define IOCTL_MFC_H263_DEC_INIT            (0x0080000D)
#define IOCTL_MFC_H263_ENC_INIT            (0x0080000E)
#define IOCTL_MFC_H263_DEC_EXE            (0x0080000F)
#define IOCTL_MFC_H263_ENC_EXE            (0x00800010)
#define IOCTL_MFC_H263_DEC_PP_EXE        (0x00800011)
#define IOCTL_MFC_H263_DEC_PP_DISP_EXE    (0x00800012)

#define IOCTL_MFC_VC1_DEC_INIT            (0x00800013)
#define IOCTL_MFC_VC1_DEC_EXE            (0x00800014)
#define IOCTL_MFC_VC1_DEC_PP_EXE        (0x00800015)
#define IOCTL_MFC_VC1_DEC_PP_DISP_EXE    (0x00800016)

#define IOCTL_MFC_PP_DISP_EXE            (0x00800017)

#define IOCTL_MFC_GET_STRM_BUF_ADDR        (0x00800018)
#define IOCTL_MFC_GET_FRAM_BUF_ADDR        (0x00800019)

#define IOCTL_MFC_SET_DISP_CONFIG        (0x0080001A)
#define IOCTL_MFC_GET_FRAME_SIZE        (0x0080001B)
#define IOCTL_MFC_SET_PP_DISP_SIZE        (0x0080001C)
#define IOCTL_MFC_GET_POST_BUF_ADDR        (0x0080001D)


/*
// 이 설정이 되면,
// Driver는 output buffer파라미터 값이
// Physical Address의 주소값로 인식하고, HW PostProcessor가
// 직접 이 buffer에 결과를 출력한다.
// 
#define IOCTL_MFC_SET_PHYSICAL_OUTBUF    (0x00800101)
#define IOCTL_MFC_SET_OUT_POSITION        (0x00800102)

#define IOCTL_MFC_GET_INPUT_BUF_ADDR    (0x0080000E)
#define IOCTL_MFC_GET_OUTPUT_BUF_ADDR    (0x0080000F)
*/

typedef struct
{
    int  width;
    int  height;
} MFC_DECODED_FRAME_INFO;

typedef struct
{
    int  rotate;
    int  deblockenable;
} MFC_DECODE_OPTIONS;


typedef struct
{
    unsigned char *outbuf;

    int  buf_width,  buf_height;
    int  img_width,  img_height;
    int  x_offset,   y_offset;
} MFC_OUTBUF;


#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__ */
