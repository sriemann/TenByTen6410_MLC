/*********************************************************************************
*                                                                               *
* Copyright (c) 2008 Samsung System LSI                                            *
* All rights reserved.                                                          *
*                                                                               *
* This software is test sample code for Hybrid Divx Decoder                        *
*                                                                                *
* Author : Jiyoung Shin                                                            *
* Last Changed : 2008.06.10                                                     *
*********************************************************************************/

#ifndef __SAMSUNG_SYSLSI_SSBSIPAVI_PARSING_H__
#define __SAMSUNG_SYSLSI_SSBSIPAVI_PARSING_H__

#include "SsbSipVideoDivXlibAVI.h"
#include "FrameExtractor.h"
#include "MPEG4Frames.h"
#include <windows.h>

#define MAX_CHUNK_BUFF_NUM    5
#define SEMAPHORE_PARSER_TIMEOUT    1000 // msec

typedef enum 
{
    FORMAT_NOT_SUPPORT = 0,
    FORMAT_AVI,
    FORMAT_M4V
}FILE_EXTENTION;

typedef struct M4V_FILE_IN_t{
    FILE            *fp;
    unsigned int    inputbuffersize;
}M4V_FILE_IN;

typedef struct DivxParserInParam_t{
    char         filename[256];
    unsigned int    stream_size;
}DivxParserInParam;

typedef struct DivxParserOutParam_t{
    unsigned int     width;
    unsigned int     height;
    unsigned int     codec_version;
    unsigned int    dwScale;    
    unsigned int    dwRate;
    FILE_EXTENTION        file_format;
}DivxParserOutParam;

#ifdef __cplusplus
extern "C" {    
#endif

static DWORD WINAPI SsbSipParserThread();
CHUNK    *SsbSipParserFrameGet(unsigned int index);
BOOL SsbSipParserThreadCreate();
void SsbSipParserThreadClose();
BOOL SsbSipParserInit(DivxParserInParam *inParam, DivxParserOutParam *outParam);
static FILE_EXTENTION SsbSipParserFileFormatGet(char *filename);

#ifdef __cplusplus
}        
#endif

#endif //__SAMSUNG_SYSLSI_SSBSIPAVI_PARSING_H__
