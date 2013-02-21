/*******************************************************************************
                        Samsung India Software Operations Pvt. Ltd. (SISO)
                                    Copyright 2006
;*******************************************************************************/

#ifndef _LIB_AVI_
#define _LIB_AVI_
//#include <windows.h>
#ifndef MDIRAC_TEST
#include <stdio.h>
#endif
#include "SsbSipVideoDivXportab.h"
#include "SsbSipVideoDivXavi.h"


#define MAX_STREAM 20
#define INDEX_INC 1000

#ifdef __cplusplus
extern "C" {    
#endif    /* __cplusplus */

typedef struct _CHUNK_ {
    FOURCC_t    fcc;
    DWORD_t    length;
    DWORD_t    flag;
    int        isValid;
    char    *buffer;
} CHUNK;

typedef struct _STREAM_INFO_ {
    char  stream_type;
    /**********************
        type of the stream
        a - audio
        v - video
        s - subtitle
        c - chapter
        u - unknown     
     **********************/

    char media_type;
    /**********************
        type of the media
        a - audio
        v - video
        t - text
        u - unknown
     **********************/

    CHUNK *strh;
    CHUNK *strf;
    CHUNK *strd;
    CHUNK *strn;

} STREAM_INFO;

typedef struct _AVI_FILE_IN_ {
    char    *filename;
#ifndef MDIRAC_TEST    
    FILE    *pf;
#else
    char        *StreamPtr;
    int        *marker;
#endif    
    CHUNK    *avih;
    int        nStream;
    STREAM_INFO *SInfo[MAX_STREAM];
    int        movi_start;
    int        movi_end;
    AVIINDEXENTRY *idx1;
    int        idx1_index;
    int        idx1_total;
    unsigned int inputbuffersize;
    CHUNK    *data_chunk;
} AVI_FILE_IN;
#ifndef MDIRAC_TEST
typedef struct _AVI_FILE_OUT_ {
    char    *filename;
    FILE    *pf;
    int        nStream;
    STREAM_INFO *SInfo[MAX_STREAM];
    int     SData[MAX_STREAM];
    int        strh_length_pos[MAX_STREAM];
    int     avih_pos;
    int     hdrl_pos;
    int     hdrl_size;
    int     movi_pos;
    int     movi_size;
    AVIINDEXENTRY *idx1;
    int        idx1_index;
    int        idx1_total;
} AVI_FILE_OUT;
#endif
/* AVI reader functions */

int SsbSipVideoDivXAVI_reader_open_file(AVI_FILE_IN *avi_file,unsigned int InputBufferSize);
int SsbSipVideoDivXAVI_reader_get_stream_info(AVI_FILE_IN *avi_file, STREAM_INFO *SInfo, int index);
int SsbSipVideoDivXAVI_reader_get_data_block(AVI_FILE_IN *avi_file, CHUNK *data_chunk,unsigned int );
int SsbSipVideoDivXAVI_reader_close_file(AVI_FILE_IN *avi_file);
CHUNK *StaDivXGetNextChunk(FILE *pf,unsigned int InputBufferSize);
STREAM_INFO *StaDivXGetNextStreamInfo(FILE *pf,unsigned int InputBufferSize);
void StaDivXReleaseChunk(CHUNK *chunk);
void StaDivXReleaseStreamInfo(STREAM_INFO *SInfo);

int StaDivXGetNextChunk_temp(FILE *pf, CHUNK *chunk);
int SsbSipVideoDivXAVI_reader_get_data_block_temp(AVI_FILE_IN *avi_file, CHUNK *chunk);

/* AVI writer functions */
#ifndef MDIRAC_TEST
int SsbSipVideoDivXAVI_writer_open_file(AVI_FILE_OUT *avi_file);
int SsbSipVideoDivXAVI_writer_put_stream_info(AVI_FILE_OUT *avi_file, STREAM_INFO *SInfo);
int SsbSipVideoDivXAVI_writer_put_data_block(AVI_FILE_OUT *avi_file, CHUNK *data_chunk);
int SsbSipVideoDivXAVI_writer_close_file(AVI_FILE_OUT *avi_file);
#endif

#ifdef __cplusplus
}   
#endif    /* __cplusplus */

#endif
