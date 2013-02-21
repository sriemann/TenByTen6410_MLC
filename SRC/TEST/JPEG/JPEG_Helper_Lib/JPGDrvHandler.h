//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#ifndef __JPEGDRVHANDLER__

#define __JPEGDRVHANDLER__

#define TRUE	1
#define FALSE	0
#define JPG_TC_FAIL	0
#define JPG_TC_PASS	1
#define JPG_TC_SKIP	3
#define JPG_TC_WONT 4

#define JPG_DRIVER_NAME		L"JPG1:"

typedef struct ioctl_Result{
	unsigned char 	get_strbuf;
	unsigned char 	get_frmbuf;
	unsigned char 	get_rgbbuf;
	unsigned char 	get_phy_frmbuf;
	unsigned char 	get_phy_rgbbuf;
	unsigned char	get_thumb_strbuf;
	unsigned char	get_thumb_frmbuf;
}RESULT_IOCL;

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

typedef enum tagENCDEC_TYPE_T{
	JPG_MAIN,
	JPG_THUMBNAIL
}ENCDEC_TYPE_T;

typedef struct tagExifFileInfo{
	char	Make[32]; 
	char	Model[32]; 
	char	Version[32]; 
	char	DateTime[32]; 
	char	CopyRight[32]; 

	UINT	Height; 
	UINT	Width;
	UINT	Orientation; 
	UINT	ColorSpace; 
	UINT	Process;
	UINT	Flash; 

	UINT	FocalLengthNum; 
	UINT	FocalLengthDen; 

	UINT	ExposureTimeNum; 
	UINT	ExposureTimeDen; 

	UINT	FNumberNum; 
	UINT	FNumberDen; 

	UINT	ApertureFNumber; 

	int		SubjectDistanceNum; 
	int		SubjectDistanceDen; 

	UINT	CCDWidth;

	int		ExposureBiasNum; 
	int		ExposureBiasDen; 


	int		WhiteBalance; 

	UINT	MeteringMode; 

	int		ExposureProgram;

	UINT	ISOSpeedRatings[2]; 
	
	UINT	FocalPlaneXResolutionNum;
	UINT	FocalPlaneXResolutionDen;

	UINT	FocalPlaneYResolutionNum;
	UINT	FocalPlaneYResolutionDen;

	UINT	FocalPlaneResolutionUnit;

	UINT	XResolutionNum;
	UINT	XResolutionDen;
	UINT	YResolutionNum;
	UINT	YResolutionDen;
	UINT	RUnit; 

	int		BrightnessNum; 
	int		BrightnessDen; 

	char	UserComments[150];
}ExifFileInfo;

typedef struct tagJPG_DEC_PROC_PARAM{
	SAMPLE_MODE_T	sampleMode;
	ENCDEC_TYPE_T	decType;
	UINT32	width;
	UINT32	height;
	UINT32	dataSize;
	UINT32	fileSize;
} JPG_DEC_PROC_PARAM;

typedef struct tagJPG_ENC_PROC_PARAM{
	SAMPLE_MODE_T	sampleMode;
	ENCDEC_TYPE_T	encType;
	IMAGE_QUALITY_TYPE_T quality;
	UINT32	width;
	UINT32	height;
	UINT32	dataSize;
	UINT32	fileSize;
} JPG_ENC_PROC_PARAM;


typedef struct tagJPG_CTX{
	UINT	debugPrint;
	char	*InBuf;
	char	*OutBuf;
	char	*InThumbBuf;
	char	*OutThumbBuf;
	UINT8	thumbnailFlag;
	JPG_DEC_PROC_PARAM	*decParam;
	JPG_ENC_PROC_PARAM	*encParam;
	JPG_ENC_PROC_PARAM	*thumbEncParam;
	ExifFileInfo *ExifInfo;
}JPG_CTX;

void* GetDecInBuf(void *, long);
JPEG_ERRORTYPE ExeJPEGDec(void *);
void* GetDecOutBuf(void *, long *);
JPEG_ERRORTYPE GetJPEGInfo(JPEGConf , UINT32 *);
void * GetRGBPhyBuf(void *, INT32 , INT32 );
BOOL ConvertYCBYCRToRGB(int , INT32 , INT32 , unsigned long ,int , INT32 , INT32 ,unsigned long);
void *GetJPEGDecOutPhyBuf(void *);
void *GetJPEGRGBPhyBuf(void *, INT32, INT32);
void *GetJPEGRGBBuf(void *, INT32, INT32);
JPEG_ERRORTYPE DeinitJPEGDec(void *);
JPEG_ERRORTYPE DeInitJPEGEnc (void *);
JPEG_ERRORTYPE SetJPGConfig (JPEGConf, INT32);
void *GetEncInBuf(void *, long );
void *GetEncOutBuf (void *, long *);
JPEG_ERRORTYPE ExeJPEGEnc(void *, ExifFileInfo *);
void* GetJPGDrvPtrDec(void);
void *GetJPGDrvPtrEnc(void);
JPEG_ERRORTYPE ExeJPEGDec(void *);

#else
#endif