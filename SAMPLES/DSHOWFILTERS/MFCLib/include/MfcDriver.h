#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_DRIVER_H__



DWORD MFC_Init(DWORD dwContext);
BOOL  MFC_Deinit(DWORD InitHandle);

DWORD MFC_Open(DWORD InitHandle, DWORD dwAccess, DWORD dwShareMode);
BOOL  MFC_Close(DWORD OpenHandle);

BOOL  MFC_IOControl(DWORD OpenHandle, DWORD dwIoControlCode,
                    PBYTE pInBuf, DWORD nInBufSize, PBYTE pOutBuf,
                    DWORD nOutBufSize,
                    PDWORD pBytesReturned);




#define IOCTL_MFC_MPEG4_DEC_INIT        (0x00800001)
#define IOCTL_MFC_MPEG4_ENC_INIT        (0x00800002)
#define IOCTL_MFC_MPEG4_DEC_EXE            (0x00800003)
#define IOCTL_MFC_MPEG4_ENC_EXE            (0x00800004)

#define IOCTL_MFC_H264_DEC_INIT            (0x00800011)
#define IOCTL_MFC_H264_ENC_INIT            (0x00800012)
#define IOCTL_MFC_H264_DEC_EXE            (0x00800013)
#define IOCTL_MFC_H264_ENC_EXE            (0x00800014)

#define IOCTL_MFC_H263_DEC_INIT            (0x00800021)
#define IOCTL_MFC_H263_ENC_INIT            (0x00800022)
#define IOCTL_MFC_H263_DEC_EXE            (0x00800023)
#define IOCTL_MFC_H263_ENC_EXE            (0x00800024)

#define IOCTL_MFC_VC1_DEC_INIT            (0x00800031)
#define IOCTL_MFC_VC1_DEC_EXE            (0x00800032)

#define IOCTL_MFC_GET_LINE_BUF_ADDR        (0x00800101)
#define IOCTL_MFC_GET_RING_BUF_ADDR        (0x00800102)
#define IOCTL_MFC_GET_FRAM_BUF_ADDR        (0x00800103)
#define IOCTL_MFC_GET_POST_BUF_ADDR        (0x00800104)
#define IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR    (0x00800105)
#define IOCTL_MFC_SET_CONFIG            (0x00800106)
#define IOCTL_MFC_GET_CONFIG            (0x00800107)

#define IOCTL_MFC_SET_DISP_CONFIG        (0x00800111)
#define IOCTL_MFC_GET_FRAME_SIZE        (0x00800112)
#define IOCTL_MFC_SET_PP_DISP_SIZE        (0x00800113)
#define IOCTL_MFC_SET_DEC_INBUF_TYPE    (0x00800114)




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
