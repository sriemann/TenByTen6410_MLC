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

#include "SsbSipParser.h"

HANDLE                pSema;
HANDLE                cSema;
HANDLE                parsingThread;
CHUNK                *AviChunk[MAX_CHUNK_BUFF_NUM];
AVI_FILE_IN             AVIFile;
M4V_FILE_IN                 M4VFile;
FILE_EXTENTION        file_format;
FRAMEX_CTX          *FrameExCtx;
unsigned int             strm_index;

/**************************************************
 
    SsbSipVideoDivXAVIParsing() 

    Functionality : Get the next data block
    Return value  :
        0   allright
        1    end of movi reached
        2    bad format

 **************************************************/
static DWORD WINAPI SsbSipParserThread()
{
    BOOL            end_of_stream = FALSE;
    unsigned int        frame_num = 0 ;
    int                ret;


    if(file_format == FORMAT_AVI)
        frame_num = 0;

    if(file_format == FORMAT_M4V)
        frame_num = 1;
        
    while(1){

        WaitForSingleObject(pSema, INFINITE);

        
        memset(AviChunk[frame_num%MAX_CHUNK_BUFF_NUM]->buffer, 0x00, AVIFile.inputbuffersize);

        if(file_format == FORMAT_AVI)
        {
            while(1){
    
                if((ret = SsbSipVideoDivXAVI_reader_get_data_block_temp(&AVIFile, AviChunk[frame_num%MAX_CHUNK_BUFF_NUM])) != 0){
                    end_of_stream = TRUE;
                    break;
                }
                if (StreamFromFOURCC(AviChunk[frame_num%MAX_CHUNK_BUFF_NUM]->fcc) == strm_index)
                    break;

            }

            if(end_of_stream) break;
            

        }
        else if(file_format == FORMAT_M4V)
        {
            AviChunk[frame_num%MAX_CHUNK_BUFF_NUM]->length = NextFrameMpeg4(FrameExCtx, M4VFile.fp, (unsigned char *)AviChunk[frame_num%MAX_CHUNK_BUFF_NUM]->buffer, M4VFile.inputbuffersize, NULL);
            if (AviChunk[frame_num%MAX_CHUNK_BUFF_NUM]->length < 4){

                ReleaseSemaphore(cSema, 1, NULL);
                break;
            }


        }

        frame_num++;
        ReleaseSemaphore(cSema, 1, NULL);
    }
    ExitThread(0);
    return 1;
}

CHUNK    *SsbSipParserFrameGet(unsigned int index)
{
    int        i;
    CHUNK    *chunk = NULL;


    i = index % MAX_CHUNK_BUFF_NUM;

    if(WaitForSingleObject(cSema, SEMAPHORE_PARSER_TIMEOUT) == WAIT_TIMEOUT){
        printf("End of Stream\n");
        return NULL;
    }

    chunk = AviChunk[i];

    ReleaseSemaphore(pSema, 1, NULL);

    return chunk;

}


BOOL SsbSipParserThreadCreate()
{
    


    parsingThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)SsbSipParserThread,
                                    NULL,
                                    0,
                                    NULL);
    if (!parsingThread) {
        printf("Unable to create parsing thread");
        return FALSE;
    }

    return TRUE;

}

void SsbSipParserThreadClose()
{
    int        i;


    for(i = 0; i < MAX_CHUNK_BUFF_NUM; i++){
        if(AviChunk[i]->buffer)
            free(AviChunk[i]->buffer);

        if(AviChunk[i])
            free(AviChunk[i]);
    }

    if(pSema)
        CloseHandle(pSema);

    if(cSema)
        CloseHandle(cSema);

    if(parsingThread)
        CloseHandle(parsingThread);

    if(AVIFile.idx1){
        free(AVIFile.idx1);
        AVIFile.idx1 = NULL;
    }

    if(file_format == FORMAT_AVI)
        SsbSipVideoDivXAVI_reader_close_file(&AVIFile);
    if(file_format == FORMAT_M4V)
        fclose(M4VFile.fp);
}

BOOL SsbSipParserInit(DivxParserInParam *inParam, DivxParserOutParam *outParam)
{

    STREAM_INFO            AviStreamInfo;
    MPEG4_CONFIG_DATA    M4vStreamInfo;
    unsigned int             i;
    AVIStreamHeader         *strm_hdr;
    BITMAPINFOHEADER_t     *bmp_info_hdr;
    static unsigned char     delimiter_mpeg4[3] = {0x00, 0x00, 0x01};


    // StreamBuffer init
    for(i = 0; i < MAX_CHUNK_BUFF_NUM; i++){
        AviChunk[i] = (CHUNK *)malloc(sizeof(CHUNK));
        memset(AviChunk[i], 0x00, sizeof(CHUNK));

        AviChunk[i]->buffer = (char *)malloc(inParam->stream_size);
        if(!AviChunk[i]->buffer)
            printf("AviChunk[%d].buffer creation failed\n", i);

        memset(AviChunk[i]->buffer, 0x00, inParam->stream_size);
    }

    pSema = CreateSemaphore(NULL, MAX_CHUNK_BUFF_NUM-1, MAX_CHUNK_BUFF_NUM-1, NULL);

    if (!pSema) {
        printf("Unable to create producer semaphore");
        return FALSE;
    }

    cSema = CreateSemaphore(NULL, 0, MAX_CHUNK_BUFF_NUM - 1, NULL);

    if (!cSema) {
        printf("Unable to create consumer semaphore");
        return FALSE;
    }

    outParam->file_format = file_format = SsbSipParserFileFormatGet(inParam->filename);

    if(file_format == FORMAT_AVI)
    {
        memset(&AVIFile, 0x00, sizeof(AVIFile));

        AVIFile.inputbuffersize = inParam->stream_size;
        AVIFile.filename = inParam->filename;

        if (SsbSipVideoDivXAVI_reader_open_file(&AVIFile, 10000) != 0)
        {
            printf( "Error opening input file.\n");
            return FALSE;
        }

        for (strm_index = 0; strm_index < AVIFile.nStream; strm_index ++) {
            SsbSipVideoDivXAVI_reader_get_stream_info(&AVIFile, &AviStreamInfo, strm_index);
            if (AviStreamInfo.media_type == 'v') break;
        }

        if (strm_index == AVIFile.nStream) {
            printf("The input AVI file does not contain a video stream.\n");
            return FALSE;
        }

        strm_hdr = (AVIStreamHeader *)AviStreamInfo.strh->buffer;
        bmp_info_hdr = (BITMAPINFOHEADER_t *)AviStreamInfo.strf->buffer;

        switch (strm_hdr->fccHandler) {
            case mmioFOURCC('d','i','v','3'):
            case mmioFOURCC('D','I','V','3'):
            case mmioFOURCC('d','i','v','4'):
            case mmioFOURCC('D','I','V','4'):
                printf("DivX Video Version : v3.11\n");
                outParam->codec_version = 311;
                break;
            case mmioFOURCC('d','i','v','x'):
            case mmioFOURCC('D','I','V','X'):
            case mmioFOURCC('X','V','I','D'):
            case mmioFOURCC('x','v','i','d'):
            case mmioFOURCC('d','x','5','0'):
            case mmioFOURCC('D','X','5','0'):
                switch (bmp_info_hdr->biCompression) {
                    case mmioFOURCC('d','i','v','x'):
                    case mmioFOURCC('D','I','V','X'):
                        printf("DivX Video Version : v4.x\n");
                        outParam->codec_version = 412;
                        break;
                    case mmioFOURCC('X','V','I','D'):
                    case mmioFOURCC('x','v','i','d'):
                        outParam->codec_version = 412;
                            break;
                            
                    case mmioFOURCC('d','x','5','0'):
                    case mmioFOURCC('D','X','5','0'):
                        printf("DivX Video Version : v5.x\n");
                        outParam->codec_version = 500;
                        break;
                }
                break;
        }
        
        if (outParam->codec_version == 0) {
            printf("The bitstream in the AVI file is not in DivX format.\n");
            return FALSE;
        }

        AVIFile.idx1 = (AVIINDEXENTRY *)malloc(AVIFile.inputbuffersize);
                
        outParam->width = bmp_info_hdr->biWidth;
        outParam->height = bmp_info_hdr->biHeight;
        outParam->dwRate = strm_hdr->dwRate;
        outParam->dwScale = strm_hdr->dwScale;

    }
    else if(file_format == FORMAT_M4V)
    {
        M4VFile.fp = fopen(inParam->filename, "rb");
        if (M4VFile.fp == NULL) {
            printf("File not found(%s)\n", inParam->filename);
            return FALSE;

        }
        M4VFile.inputbuffersize = inParam->stream_size;;
        FrameExCtx = FrameExtractorInit(FRAMEX_IN_TYPE_FILE, delimiter_mpeg4, sizeof(delimiter_mpeg4), 1);
        FrameExtractorFirst(FrameExCtx, M4VFile.fp);
        
        WaitForSingleObject(pSema, INFINITE);
        AviChunk[0]->length = ExtractConfigStreamMpeg4(FrameExCtx, M4VFile.fp, (unsigned char *)AviChunk[0]->buffer, inParam->stream_size, &M4vStreamInfo);
        outParam->width = M4vStreamInfo.width;
        outParam->height = M4vStreamInfo.height;
        outParam->dwRate = 0;
        outParam->dwScale = 0;
        outParam->codec_version = 500;
        
        ReleaseSemaphore(cSema, 1, NULL);
    }
    else
    {
        printf("not supported file extention(%s)\n", inParam->filename);
        return FALSE;
    }

    return TRUE;
        
}


static FILE_EXTENTION SsbSipParserFileFormatGet(char *filename)
{
    unsigned int    filelength;

    filelength = strlen(filename);

    if(!strncmp(&filename[filelength - 4], ".AVI", 4) || !strncmp(&filename[filelength - 4], ".avi", 4))
        return FORMAT_AVI;

    if(!strncmp(&filename[filelength - 5], ".DIVX", 5) || !strncmp(&filename[filelength - 5], ".divx", 5))
        return FORMAT_AVI;

    if(!strncmp(&filename[filelength - 4], ".M4V", 4) || !strncmp(&filename[filelength - 4], ".m4v", 4))
        return FORMAT_M4V;


    return FORMAT_NOT_SUPPORT;

}


