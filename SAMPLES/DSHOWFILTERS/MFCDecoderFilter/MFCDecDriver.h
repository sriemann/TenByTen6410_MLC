//------------------------------------------------------------------------------
// File: MFCDecDriver.h
//
// Desc: define CMFCDecFilterOutputPin classss
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#ifndef _SAMSUNG_SYSLSI_MFC_DEC_DRIVER_H_
#define _SAMSUNG_SYSLSI_MFC_DEC_DRIVER_H_


typedef int SVCM_ERRORTYPE;

typedef unsigned int  SVCM_U32;
typedef signed   int  SVCM_Int;
typedef unsigned char SVCM_U8;
typedef signed   char SVCM_S8;

typedef void          SVCM_Void;

typedef enum
{
    CODEC_TYPE_MPEG4,
    CODEC_TYPE_H263,
    CODEC_TYPE_H264,
} CODEC_TYPE;

typedef enum
{
    CODEC_ID_MPEG4,
    CODEC_ID_H263,
    CODEC_ID_H264,
} CODEC_ID;



typedef struct MemoryStructure
{
    SVCM_U8* pucAddr;
    SVCM_U32 uSize;
} SVCM_Memory;

typedef struct VFrame 
{
    SVCM_U8 *     pucY;
    SVCM_U8 *     pucCb;
    SVCM_U8 *     pucCr;
    SVCM_U8 *     pOutputMemory;
    SVCM_U32    u32Size;
} VFrame;


typedef struct
{
    SVCM_S8       sCodecName[32];
    CODEC_TYPE    CodecType;
    CODEC_ID      CodecId;

    SVCM_U32      uCodecInitFlag;

    SVCM_Memory  *pOperationalMemory;

    SVCM_U32      uWidth, uHeight;

    SVCM_Void    *pvPrivData;
} SVCMCodecContext;

typedef struct
{
    HANDLE         hOpen;            //Win32 HANDLE for the MFC device

    int            fInstInit;        // MFC Instance init flag (0: not initialized, 1: initialized)
    unsigned char *pConfStrmBuf;    // Temporary buffer for the config stream. After 'SEQ_INIT' command, it is released.
    int            nConfStrmSize;    // Size of config stream

    unsigned char *pStrmBuf;        // 입력 버퍼 (STRM_BUF). MFC 내부 버퍼 값을 얻어내어 이 변수에 저장한다.
    int            width, height;    // Width & height of the frame

} PRIV_DATA;


/***********************************************************************
// Status type
***********************************************************************/

typedef enum {
  H264_DEC_SUCCESS=0,            
  H264_DEC_OK_BUT_NOT_FRAME,
  H264_DEC_OK_BUT_ENDOFSEQUENCE,
  H264_DEC_FAILURE,            
  H264_DEC_ERROR,
  H264_DEC_ERROR_OVER_MAX_INSTANCE,
  H264_DEC_ERROR_INIT_NOT_SEQ,
  H264_DEC_ERROR_PARSERTOP_TIMEOUT,
  H264_DEC_ERROR_CODECTOP_TIMEOUT  
} H264_DEC_STATUS;


/***********************************************************************
// structure
***********************************************************************/

typedef struct _TH264_DEC_OUT_{
    unsigned char *pY;
    unsigned char *pCb;
    unsigned char *pCr;
    unsigned int uiWidth;
    unsigned int uiHeight;
    unsigned int uiNalUnitType;
} TH264DecOut, *pTH264DecOut;

#endif /* _SAMSUNG_SYSLSI_MFC_DEC_DRIVER_H_ */
