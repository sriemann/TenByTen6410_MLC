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

#include <windows.h>
// Platform Dependent
#include <SVEDriverAPI.h>
#include "SsbSipVideoDivXdecore.h"
#include "SsbSipVideoDivXlibAVI.h"
#include "SsbSipVideoDivXmp4_vars.h"
#include "SsbSipRender.h"
#include "SsbSipParser.h"
#include "SsbSipMpeg4Decode.h"
#include "SsbSipMfcDecode.h"
#include "SsbSipDecoder.h"
#include "SsbSipHybridDivxInterface.h"
#include "SsbSipHybridDivxMain.h"


#define MAX_RESOLUTION_WIDTH    720
#define MAX_RESOLUTION_HEIGHT    576
#define MAX_STRM_SIZE             512000   // This is maximum value by MFC H/W. don't change to bigger value
#define TEST_LOOP_COUNT            1
#define MAX_FILENAME_LEN        256
#define CONFIG_FILE_NAME        "\\DivxFileList.txt"
//#define FILE_OUT

void SsbSipHybridDivxDecode(char *filename);
void SsbSipHybridDivxResourceRelease(void);

void SsbSipHybridDivxMain(void)
{

    int loop_cnt = TEST_LOOP_COUNT;
    char filename[MAX_FILENAME_LEN];
    FILE *fp = NULL;


    printf("\n#### Divx Test Start \n\n");
    
    while(loop_cnt--)
    {
        printf("-----------------------------------------------------------------\n");
        printf("\tLoop Count = %3d\n", TEST_LOOP_COUNT - loop_cnt);
        printf("-----------------------------------------------------------------\n");
        fp = fopen(CONFIG_FILE_NAME, "rb");
        if(!fp){
            printf("Can not open Divx File List (%s)\n", CONFIG_FILE_NAME);
            return;        
        }        
        
        do{
            memset(&filename, 0x00, MAX_FILENAME_LEN);
            fgets(filename, MAX_FILENAME_LEN - 1, fp);
            if('\0' != filename[0]){
                
                int len = strlen(filename);
                
                while(!isprint(filename[len-1]))
                {
                    len--;
                }
                filename[len] = '\0';
                SsbSipHybridDivxDecode(filename);
            }
        }while(!feof(fp));

        if(fp){
            fclose(fp);
            fp = NULL;
        }
    }
    printf("\n\n#### Divx Test Ends\n");    
    printf("\nEXITING......\n");
    exit(-1);
}



void SsbSipHybridDivxDecode(char *filename)
{

    DivxDecInParam        DivxInParam;
    DivxDecOutParam        DivxOutParam;
    DivxParserInParam    ParserInParam;
    DivxParserOutParam    ParserOutParam;
    unsigned int            frame_num;
    CHUNK                 *StreamChunk;
    int                     clock_value_BLOCK ;
    long int                 total_cycles_BLOCK =0;
    int                    err;
    unsigned int            i;
    unsigned int            buf_width, buf_height;
#ifdef PERF_RENDER
    int cyclecountstart,cyclecountend,cyclecount = 0;
#endif    
#ifdef FILE_OUT
    char                    *YUVBuf;
    FILE                    *fp;
    char                    outFilename[MAX_FILENAME_LEN];
    char *prevVirbuf = NULL;
#endif



    memset(&DivxInParam, 0x00, sizeof(DivxInParam));
    memset(&DivxOutParam, 0x00, sizeof(DivxOutParam));
    memset(&ParserInParam, 0x00, sizeof(ParserInParam));
    memset(&ParserOutParam, 0x00, sizeof(ParserOutParam));


    //////////////////////////////////////////////////////
    //// 1. Initialize Paser                                    //
    //////////////////////////////////////////////////////
    memcpy(ParserInParam.filename , filename, strlen(filename));
    ParserInParam.stream_size =MAX_STRM_SIZE;
    if(!SsbSipParserInit(&ParserInParam, &ParserOutParam)){
        SsbSipParserThreadClose();
        goto PRINT_PERF;
    }


    printf("filename : %s\n", filename);
    printf("Video dimension : %d x %d\n", ParserOutParam.width, ParserOutParam.height);
    printf("Video frame rate : %.3f fps\n", (ParserOutParam.dwScale ? (float)ParserOutParam.dwRate / ParserOutParam.dwScale : 0));

    if((ParserOutParam.width > MAX_RESOLUTION_WIDTH) || ( ParserOutParam.height > MAX_RESOLUTION_HEIGHT)){
        printf("Width and Height is too big....Maximum size is 720*480\n\n");
        SsbSipParserThreadClose();
        goto PRINT_PERF;
    }

    
#ifdef FILE_OUT
    sprintf(outFilename, "%s.yuv", filename);
    printf("output file name : %s\n", outFilename);
    fp = fopen(outFilename, "wb");
    if(fp == NULL){
        printf("fopen error : %s\n", outFilename);
        return;
    }
#endif

    //////////////////////////////////////////////////////
    //// 2. Initialize Divx Decoder                        //
    //////////////////////////////////////////////////////
    DivxInParam.width = ParserOutParam.width;
    DivxInParam.height = ParserOutParam.height;
    DivxInParam.codec_version = ParserOutParam.codec_version;
    DivxInParam.size = MAX_STRM_SIZE;
    DivxInParam.file_format = ParserOutParam.file_format;


    err = SsbSipHybridDivxDecInit(&DivxInParam, &DivxOutParam);
    if (err != DEC_OK)
    {
        printf("SsbSipHybridDivxDecInit failure: %d\n", err);
        SsbSipHybridDivxDecDeInit();
        goto PRINT_PERF;
    }

    buf_width = DivxOutParam.buf_width;
    buf_height = DivxOutParam.buf_height;
    printf("\nbuf_width:%d buf_height:%d\n",   buf_width, buf_height);
    


    //////////////////////////////////////////////////////
    //// 3. Create Parser & Display Thread                //
    //////////////////////////////////////////////////////
    SsbSipDisplayEventCreate();
    if(!SsbSipDisplayThreadCreate()){
        SsbSipHybridDivxDecDeInit();
        SsbSipDisplayThreadClose();
        goto PRINT_PERF;
    }
    if(!SsbSipParserThreadCreate()){
        SsbSipHybridDivxResourceRelease();
        goto PRINT_PERF;
    }

#ifdef PERF_RENDER
    cyclecountstart = GetTickCount();
#endif

    //////////////////////////////////////////////////////
    //// 4. Divx Decoding                                //
    //////////////////////////////////////////////////////
    frame_num = 0;
    while (1)
    {
        //get a frame from Parser
        StreamChunk = SsbSipParserFrameGet(frame_num);
        
        if(StreamChunk == NULL){
        //    printf("End of Stream\n");
            break;
        }

        frame_num ++;
        if(StreamChunk->length <= 0 ){
            printf("stream size is not greater than 0...skip stream\n");
            continue;
        }
        
        DivxInParam.strmbuf = (char *)StreamChunk->buffer;
        DivxInParam.size = StreamChunk->length;
        DivxInParam.frame_num = frame_num;

        //Sleep(15); // for normal play speed

        //start  Divx decoding
        err = SsbSipHybridDivxDecExe(&DivxInParam, &DivxOutParam);

        //search for first I frame
        if(err == FRSTKEYFRMCRPTED){
            continue;
        }

        if(err != DEC_OK){
            printf("err = %d\n", err);
            break;
        }

        //display YUV
        if (DivxOutParam.phyYUVBuff != NULL){        
            SsbSipDisplayParamSet(DivxOutParam.phyYUVBuff, DivxInParam.width, DivxInParam.height, buf_width, buf_height, DIVX_BUFF_PAD_SIZE, DIVX_BUFF_PAD_SIZE);
            SsbSipDisplayEventSet();
        }

#ifdef FILE_OUT
    
        if (DivxOutParam.virYUVBuff != NULL)
        {
            YUVBuf = DivxOutParam.virYUVBuff + buf_width*DIVX_BUFF_PAD_SIZE + DIVX_BUFF_PAD_SIZE;
            for(i = 0; i < DivxInParam.height; i++){
                fwrite(YUVBuf, 1, DivxInParam.width, fp);
                YUVBuf += buf_width;
            }

            YUVBuf = DivxOutParam.virYUVBuff + buf_width*buf_height + buf_width/2*DIVX_BUFF_PAD_SIZE/2 + DIVX_BUFF_PAD_SIZE/2;
            for(i = 0; i < DivxInParam.height/2; i++){
                fwrite(YUVBuf, 1, DivxInParam.width/2, fp);
                YUVBuf += buf_width/2;
            }

            YUVBuf = DivxOutParam.virYUVBuff + buf_width*buf_height +buf_width*buf_height/4+ buf_width/2*DIVX_BUFF_PAD_SIZE/2 + DIVX_BUFF_PAD_SIZE/2;
            for(i = 0; i < DivxInParam.height/2; i++){
                fwrite(YUVBuf, 1, DivxInParam.width/2, fp);
                YUVBuf += buf_width/2;
            }
        }
#endif    
    }

    // for the last frame display if stream include unpacked PB frame
    DivxInParam.strmbuf = NULL;
    DivxInParam.size = 0;

    err = SsbSipHybridDivxDecExe(&DivxInParam, &DivxOutParam);
    if(err != DEC_OK)
        printf("err = %d\n", err);
    
    if (DivxOutParam.phyYUVBuff != NULL){    
        SsbSipDisplayParamSet(DivxOutParam.phyYUVBuff, DivxInParam.width, DivxInParam.height, buf_width, buf_height, DIVX_BUFF_PAD_SIZE, DIVX_BUFF_PAD_SIZE);
        SsbSipDisplayEventSet();
    }
    
#ifdef FILE_OUT
    
        if (DivxOutParam.virYUVBuff != NULL)
        {
            YUVBuf = DivxOutParam.virYUVBuff + buf_width*DIVX_BUFF_PAD_SIZE + DIVX_BUFF_PAD_SIZE;
            for(i = 0; i < DivxInParam.height; i++){
                fwrite(YUVBuf, 1, DivxInParam.width, fp);
                YUVBuf += buf_width;
            }

            YUVBuf = DivxOutParam.virYUVBuff + buf_width*buf_height + buf_width/2*DIVX_BUFF_PAD_SIZE/2 + DIVX_BUFF_PAD_SIZE/2;
            for(i = 0; i < DivxInParam.height/2; i++){
                fwrite(YUVBuf, 1, DivxInParam.width/2, fp);
                YUVBuf += buf_width/2;
            }

            YUVBuf = DivxOutParam.virYUVBuff + buf_width*buf_height +buf_width*buf_height/4+ buf_width/2*DIVX_BUFF_PAD_SIZE/2 + DIVX_BUFF_PAD_SIZE/2;
            for(i = 0; i < DivxInParam.height/2; i++){
                fwrite(YUVBuf, 1, DivxInParam.width/2, fp);
                YUVBuf += buf_width/2;
            }
        }
#endif    

    //////////////////////////////////////////////////////
    //// 5. Release Divx Deocoder Resources                //
    //////////////////////////////////////////////////////
#ifdef FILE_OUT
    fclose(fp);
#endif
    SsbSipHybridDivxResourceRelease();

        
PRINT_PERF:
#ifdef PERF_RENDER
    cyclecountend = GetTickCount();
    cyclecount += cyclecountend - cyclecountstart;

    if(frame_num && cyclecount){
        printf("\nTime Taken in Milliseconds=%d",cyclecount);
        printf("\nTotal No of frames=%d",frame_num);
        printf("\n----------------------------------------------------------------------\nrendering FPS : %f(%d msec)\n\n", frame_num*1000.0/cyclecount, cyclecount/frame_num);
        printf("------------------------------------------------------------------------\n");
    }
#endif
    return;
}

void SsbSipHybridDivxResourceRelease(void)
{
    SsbSipHybridDivxDecDeInit();
    SsbSipDisplayThreadClose();
    SsbSipParserThreadClose();
}

