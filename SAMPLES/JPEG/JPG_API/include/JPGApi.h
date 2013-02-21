//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    JPGApi.h

Abstract:

    This file implements JPEG driver.

Functions:


Notes:

--*/
#define MAX_JPG_WIDTH                2048
#define MAX_JPG_HEIGHT                1536
#define MAX_YUV_SIZE                (MAX_JPG_WIDTH * MAX_JPG_HEIGHT * 2)
#define    MAX_FILE_SIZE                (MAX_JPG_WIDTH * MAX_JPG_HEIGHT)
#define MAX_JPG_THUMBNAIL_WIDTH        160
#define MAX_JPG_THUMBNAIL_HEIGHT    120
#define MAX_YUV_THUMB_SIZE            (MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT * 2)
#define    MAX_FILE_THUMB_SIZE            (MAX_JPG_THUMBNAIL_WIDTH * MAX_JPG_THUMBNAIL_HEIGHT)
#define MAX_RGB_WIDTH                800
#define MAX_RGB_HEIGHT                480
#define EXIF_FILE_SIZE                28800


typedef enum tagSAMPLE_MODE_T{
    JPG_444,
    JPG_422,
    JPG_420, 
    JPG_411,
    JPG_400,
    JPG_SAMPLE_UNKNOWN
}SAMPLE_MODE_T;

typedef enum tagIMAGE_QUALITY_TYPE_T{
    JPG_QUALITY_LEVEL_1 = 0, /*high quality*/
    JPG_QUALITY_LEVEL_2,
    JPG_QUALITY_LEVEL_3,
    JPG_QUALITY_LEVEL_4     /*low quality*/
}IMAGE_QUALITY_TYPE_T;

typedef enum tagJPEGConf{
    JPEG_GET_DECODE_WIDTH,
    JPEG_GET_DECODE_HEIGHT,
    JPEG_GET_SAMPING_MODE,
    JPEG_SET_ENCODE_WIDTH,
    JPEG_SET_ENCODE_HEIGHT,
    JPEG_SET_ENCODE_QUALITY,
    JPEG_SET_ENCODE_THUMBNAIL,
    JPEG_SET_SAMPING_MODE,
    JPEG_SET_THUMBNAIL_WIDTH,
    JPEG_SET_THUMBNAIL_HEIGHT
}JPEGConf;

typedef enum tagJPEG_ERRORTYPE{
    JPEG_FAIL,
    JPEG_OK,
    JPEG_ENCODE_FAIL,
    JPEG_ENCODE_OK,
    JPEG_DECODE_FAIL,
    JPEG_DECODE_OK,
    JPEG_HEADER_PARSE_FAIL,
    JPEG_HEADER_PARSE_OK,
    JPEG_DISPLAY_FAIL,
    JPEG_UNKNOWN_ERROR
}JPEG_ERRORTYPE;

typedef struct tagExifFileInfo{
    char    Make[32]; 
    char    Model[32]; 
    char    Version[32]; 
    char    DateTime[32]; 
    char    CopyRight[32]; 

    UINT    Height; 
    UINT    Width;
    UINT    Orientation; 
    UINT    ColorSpace; 
    UINT    Process;
    UINT    Flash; 

    UINT    FocalLengthNum; 
    UINT    FocalLengthDen; 

    UINT    ExposureTimeNum; 
    UINT    ExposureTimeDen; 

    UINT    FNumberNum; 
    UINT    FNumberDen; 

    UINT    ApertureFNumber; 

    int        SubjectDistanceNum; 
    int        SubjectDistanceDen; 

    UINT    CCDWidth;

    int        ExposureBiasNum; 
    int        ExposureBiasDen; 


    int        WhiteBalance; 

    UINT    MeteringMode; 

    int        ExposureProgram;

    UINT    ISOSpeedRatings[2]; 
    
    UINT    FocalPlaneXResolutionNum;
    UINT    FocalPlaneXResolutionDen;

    UINT    FocalPlaneYResolutionNum;
    UINT    FocalPlaneYResolutionDen;

    UINT    FocalPlaneResolutionUnit;

    UINT    XResolutionNum;
    UINT    XResolutionDen;
    UINT    YResolutionNum;
    UINT    YResolutionDen;
    UINT    RUnit; 

    int        BrightnessNum; 
    int        BrightnessDen; 

    char    UserComments[150];
}ExifFileInfo;

void *SsbSipJPEGDecodeInit(void);
void *SsbSipJPEGEncodeInit(void);
JPEG_ERRORTYPE SsbSipJPEGDecodeExe(void *openHandle);
JPEG_ERRORTYPE SsbSipJPEGEncodeExe(void *openHandle, ExifFileInfo *Exif);
void *SsbSipJPEGGetDecodeInBuf(void *openHandle, long size);
void *SsbSipJPEGGetDecodeOutBuf (void *openHandle, long *size);
void *SsbSipJPEGGetEncodeInBuf(void *openHandle, long size);
void *SsbSipJPEGGetEncodeOutBuf (void *openHandle, long *size);
JPEG_ERRORTYPE SsbSipJPEGSetConfig (JPEGConf type, INT32 value);
JPEG_ERRORTYPE SsbSipJPEGGetConfig (JPEGConf type, INT32 *value);
JPEG_ERRORTYPE SsbSipJPEGDecodeDeInit (void *openHandle);
JPEG_ERRORTYPE SsbSipJPEGEncodeDeInit (void *openHandle);
void *SsbSipJPEGGetDecodeOutPhyBuf(void *openHandle);
void *SsbSipJPEGGetRGBBuf(void *openHandle, INT32 width, INT32 height);
void *SsbSipJPEGGetRGBPhyBuf(void *openHandle, INT32 width, INT32 height);

