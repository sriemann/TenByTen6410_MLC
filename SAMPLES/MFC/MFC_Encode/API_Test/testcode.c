// TestCode.cpp : Defines the entry point for the console application.
// S3C6410 MFC
// AP Development 
// kimoon.kim@samsung.com


#include <windows.h>
#include <stdio.h>


#include "SsbSipMpeg4Decode.h"
#include "SsbSipMpeg4Encode.h"

#include "SsbSipH264Decode.h"
#include "SsbSipH264Encode.h"

#include "SsbSipVC1Decode.h"

#include "SsbSipMfcDecode.h"

#include "FrameExtractor.h"
#include "MPEG4Frames.h"
#include "H263Frames.h"
#include "H264Frames.h"
#include "VC1Frames.h"

//
// Option
// 
#define FPS                    1
#define DEBUG                1    

#define DEC_LOOF             100
#define DEC_MAKE_LOOF     20
#define ENC_LOOF             100
#define ENC_MAKE_LOOF    100        // 0 : No Make File

#define MOON_NoSleepWakeupTest

#define MOON_MakeVC1DecOutput
#define MOON_MakeMPEG4DecOutput
#define MOON_MakeH264DecOutput
#define MOON_MakeH263DecOutput

//#define MOON_DecRotate
#define DecRotate_value         0x0010

static unsigned char delimiter_mpeg4[3] = {0x00, 0x00, 0x01};
static unsigned char delimiter_h263[3]  = {0x00, 0x00, 0x80};
static unsigned char delimiter_h264[4]  = {0x00, 0x00, 0x00, 0x01};


#define INPUT_BUFFER_SIZE        (1024 * 256)


void printD(char* fmt, ...) 
{
#if (DEBUG == 1)
    char str[512];

    vsprintf(str, fmt, (char *)(&fmt+1));
    printf(str);
#endif
}





////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  MPEG4 decoder 관련 테스트 코드
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int mpeg4dec_test(char *InputFile, char *OutputFile)
{
    void    *handle;
    SSBSIP_MPEG4_STREAM_INFO stream_info;
//    SSBSIP_MPEG4_PHYOUTBUFF_INFO phyoutbuff_info;
    FILE            *fp_in;
    int              nLoop, nFrames;
    void            *pStrmBuf;
    int               nFrameLeng=100000;
    unsigned char *pYUVBuf;
    int                    nYUVLeng;
    FRAMEX_CTX  *pFrameExCtx;
    int             ret;
#ifdef MOON_MakeMPEG4DecOutput
    FILE         *fp_out;
    int            nWriteSize;
#endif
#ifdef MOON_DecRotate
    unsigned int    post_rotate;
#endif 

    RETAILMSG(1,(L" ***** MPEG4 DEC START ***** \r\n\n"));

//
// 0. Open File
//
    fp_in = fopen(InputFile, "rb");
    if (fp_in == NULL) 
    {
        printD("[APP]File not found\n");
        return 0;
    }
#ifdef MOON_MakeMPEG4DecOutput
    fp_out = fopen(OutputFile,"wb");
    if (fp_out == NULL) 
    {
        printD("[APP]Cannot open the output file.\n");
        return 0;
    }
#endif

//
//    Analyze Stream ( Just Check Delimeter )
//    
    pFrameExCtx = FrameExtractorInit(FRAMEX_IN_TYPE_FILE, delimiter_mpeg4, sizeof(delimiter_mpeg4), 1);
    FrameExtractorFirst(pFrameExCtx, fp_in);

// 
// 1. Decode Create Instance
//
    handle = SsbSipMPEG4DecodeInit();
    if (handle == NULL)
    {
        printD("[APP]SsbSipMPEG4DecodeInit Failed.\n");
        return 0;

    }

//
// 2. Get Input(Stream) Buffer Address
//
    pStrmBuf = SsbSipMPEG4DecodeGetInBuf(handle, nFrameLeng);
    if (pStrmBuf == NULL) 
    {
        printD("[APP]SsbSipMPEG4DecodeGetInBuf Failed.\n");
        SsbSipMPEG4DecodeDeInit(handle);
        return 0;
    }
    printD("[APP]SsbSipMPEG4DecodeGetInBuf: pStrmBuf=0x%X.\n", pStrmBuf);

//
// 3. Copy Header+I frame
//
    nFrameLeng = ExtractConfigStreamMpeg4(pFrameExCtx, fp_in, pStrmBuf, INPUT_BUFFER_SIZE,0);

//
// Option : Rotate
//
#ifdef MOON_DecRotate
    post_rotate = DecRotate_value;
    if (SsbSipMPEG4DecodeSetConfig(handle, MPEG4_DEC_SETCONF_POST_ROTATE, &post_rotate) != SSBSIP_MPEG4_DEC_RET_OK) {
        RETAILMSG(1,(L"[APP]Set Config Rotate failed.\n"));
        return 0;
    }
    RETAILMSG(1,(L"[APP] POST_ROTATE  succeeds.\n"));
#endif

//
// 4. Decode(Parsing Header)
//
    if (SsbSipMPEG4DecodeExe(handle, nFrameLeng) != SSBSIP_MPEG4_DEC_RET_OK) 
    {
        printD("[APP]MPEG4 Decoder Configuration Failed.\n");
        return 0;
    }

//
// 5. Get Configuration Information
//
    if (SsbSipMPEG4DecodeGetConfig(handle, MPEG4_DEC_GETCONF_STREAMINFO, &stream_info) != SSBSIP_MPEG4_DEC_RET_OK)
        return 0;
    printD("[APP]\t<STREAMINFO> width=%d   height=%d.\n", stream_info.width, stream_info.height);


    nFrames = 0;
    for (nLoop=0; nLoop < DEC_LOOF; nLoop++)
    {
//
// 6. Decode
//
#ifdef MOON_NoSleepWakeupTest        // Default
        ret = SsbSipMPEG4DecodeExe(handle, nFrameLeng) ;
        if (ret != SSBSIP_MPEG4_DEC_RET_OK)
        {
            printD("[APP]MPEG4 Decoder Failed! :: ret=%d \r\n", ret);
            break;
        }
#else                                // SleepWakeupTest
        do
        {
            ret = SsbSipMPEG4DecodeExe(handle, nFrameLeng) ;
            if(ret == SSBSIP_MPEG4_DEC_RET_OK)
                break;
            else
            {
                if(ret == -1004)
                {
                    printD("[APP] MFC Sleeping -> Wait!!! .\r\n");
                    Sleep(1000);
                }
                else
                    break;
            }
        }while(1);
        if (ret != SSBSIP_MPEG4_DEC_RET_OK)
        {
            printD("[APP]MPEG4 Decoder Failed! :: ret=%d \r\n", ret);
            break;
        }
#endif
        
//
// 7. Get Output(YUV420) Buffer Address
//
    // Virtual
        pYUVBuf = SsbSipMPEG4DecodeGetOutBuf(handle, &nYUVLeng);
        if (pYUVBuf == NULL)
        {
            printD("[APP]SsbSipMPEG4DecodeGetOutBuf Failed! \r\n");
            break;
        }
    // Physical    
//        if (SsbSipMPEG4DecodeGetConfig(handle, MPEG4_DEC_GETCONF_PHYADDR_FRAM_BUF, &phyoutbuff_info))
//            return 0;
//        if(nLoop<5)
//        {
//           printD("[APP]<STREAMINFO> phyaddr=%x  size=%d.\n", phyoutbuff_info.phyaddr, phyoutbuff_info.size);
//        }
        
// 
// 8. Make File
//
#ifdef MOON_MakeMPEG4DecOutput
        if (nLoop < DEC_MAKE_LOOF)
        {
            nWriteSize=fwrite(pYUVBuf, 1, nYUVLeng, fp_out);
            if(nWriteSize!= nYUVLeng)
            {
                printD("[APP] Error fwrite :: size=%d \r\n", nWriteSize);
                break;
            }
        }        
#endif

        printD("[APP] %d DecOK \r\n", nLoop);

//
// 9. Copy Stream
//
        nFrameLeng = NextFrameMpeg4(pFrameExCtx, fp_in, pStrmBuf, INPUT_BUFFER_SIZE, NULL);
        if (nFrameLeng < 4)
            break;
    }

//
// 10. Delete Instance
//
    SsbSipMPEG4DecodeDeInit(handle);
    printD("[APP]  Deinit\r\n");

//
// 11. Close File
//
    fclose(fp_in);
#ifdef MOON_MakeMPEG4DecOutput
    fclose(fp_out);
#endif

    return 0;
}





////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  H264 Encoder 관련 테스트 코드
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int H264_enc_test(char *InputFile, char *OutputFile, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiframerate, unsigned int uibitrate)
{
    void              *handle;
    FILE              *fp_in, *fp_out;
    int                ret, i;
    unsigned int       width, height;
    unsigned int       frame_rate, bitrate, gop_num;
    unsigned int   num_slices;
    unsigned int   change_param[2], cur_pic_opt[2];
    long               size;
    unsigned char *p_inbuf;
    unsigned char *p_outbuf;
    int            nWriteSize;

    RETAILMSG(1,(L" ***** H264 ENC START ***** \r\n\n"));

//
// 0. Open File
//
    fp_in = fopen(InputFile,"rb");
    if (fp_in == NULL) {
        printD("[APP]File not found\n");
        return 0;
    }
    fp_out = fopen(OutputFile, "wb");
    if (fp_out == NULL) {
        printD("[APP]Cannot open the output file.\n");
        return 0;

    }

// 
// 1. Encode Create Instance
//
    width      = uiWidth;
    height     = uiHeight;
    frame_rate = uiframerate;
    bitrate    = uibitrate;
    gop_num    = frame_rate;

    handle = SsbSipH264EncodeInit(width,      height,
                                  frame_rate, bitrate,
                                  gop_num);
    printD("[APP] HANDLE = 0x%X.\n", (DWORD) handle);
    if (handle == NULL) {
        return 0;
    }

// 
// 2. Initialize Instance
//
//    num_slices = 1;
//    SsbSipH264EncodeSetConfig(handle, H264_ENC_SETCONF_NUM_SLICES, &num_slices);

    if (SsbSipH264EncodeExe(handle) != SSBSIP_MPEG4_ENC_RET_OK) {
        RETAILMSG(1,(L"MPEG4 Encoder Instance Initialization Failed.\r\n"));
        return 0;
    }    
    
//
// 3. Get Input(YUV420) Buffer Address
//
    p_inbuf = SsbSipH264EncodeGetInBuf(handle, 0);
    if (p_inbuf == NULL) {
        return 0;
    }

    for (i=0; i<ENC_LOOF; i++) 
    {
//
// 4. Copy YUV into Input Buffer
//
        if( (fread(p_inbuf, 1, (width * height * 3) >> 1, fp_in)) != (width * height * 3) >> 1)
            break;

// 
// 5. Encode
//    
        do
        {
            ret = SsbSipH264EncodeExe(handle) ;
            
            if(ret == SSBSIP_H264_ENC_RET_OK)
                break;
            else
        {
                if(ret == -1004)
                {
                    printD("[APP] MFC Sleeping -> Wait!!! .\r\n");
                    Sleep(1000);
                }
                else
                    break;
            }
        }while(1);

        if (ret != SSBSIP_H264_ENC_RET_OK)
        {
            printD("[APP]H264 Encoder Failed! :: ret=%d \r\n", ret);
            break;
        }

//
// 6. Get Output(Stream) Buffer Address
//
        p_outbuf = SsbSipH264EncodeGetOutBuf(handle, &size);
        printD("[APP] Output Buf = 0x%X,  size = %d\n", (DWORD) p_outbuf, size);

// 
// 7. Make File
//
        if(i<ENC_MAKE_LOOF)
        {
            nWriteSize=fwrite(p_outbuf, 1, size, fp_out);
            if(nWriteSize!= size)
            {
                printD("[APP] Error fwrite :: size=%d \r\n", nWriteSize);
                break;
            }
        }        
        printD("[APP] %d Encoded.\r\n", i);
    }

    SsbSipH264EncodeDeInit(handle);
    printD("[APP]  Deinit\r\n");

    fclose(fp_in);
    fclose(fp_out);

    return 0;
}






////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  H253 encoder 관련 테스트 코드
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int H263_enc_test(char *InputFile, char *OutputFile, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiframerate, unsigned int uibitrate)
{
    void               *handle;
    FILE            *fp_in, *fp_out;
    int                ret, i;
    unsigned int     width, height;
    unsigned int    frame_rate, bitrate, gop_num;
    unsigned int   num_slices, h263_annex;    
    unsigned int   change_param[2];
    long               size;
    unsigned char *p_inbuf;
    unsigned char *p_outbuf;
    int            nWriteSize;

#ifdef FPS
    INT32    encodeTime;
    INT32    sum_encodeTime=0;
    INT32    ave_encodeTime=0;
#endif

    RETAILMSG(1,(L" ***** H263 ENC START ***** \r\n\n"));

//
// 0. Open File
//
    fp_in = fopen(InputFile,"rb");
    if (fp_in == NULL) 
    {
        printD("[APP]File not found\n");
        return 0;
    }
    fp_out = fopen(OutputFile, "wb");
    if (fp_out == NULL) 
    {
        printD("[APP]Cannot open the output file.\n");
        return 0;

    }

// 
// 1. Encode Create Instance
//
    width      = uiWidth;
    height     = uiHeight;
    frame_rate = uiframerate;
    bitrate    = uibitrate;
    gop_num    = frame_rate;

    handle = SsbSipMPEG4EncodeInit(SSBSIPMFCENC_H263, width,      height,
                                   frame_rate, bitrate, gop_num);
    if (handle == NULL)
    {
        return 0;
    }
    printD("[APP] HANDLE = 0x%X.\n", (DWORD) handle);

// 
// 2. Initialize Instance
//
#define ANNEX_T_OFF                        (0<<0)
#define ANNEX_T_ON                        (1<<0)
#define ANNEX_K_OFF                        (0<<1)
#define ANNEX_K_ON                        (1<<1)
#define ANNEX_J_OFF                        (0<<2)
#define ANNEX_J_ON                        (1<<2)
#define ANNEX_I_OFF                        (0<<3)
#define ANNEX_I_ON                        (1<<3)
    num_slices = 1;
    SsbSipMPEG4EncodeSetConfig(handle, MPEG4_ENC_SETCONF_H263_NUM_SLICES, &num_slices);
    h263_annex = ANNEX_K_ON;
    SsbSipMPEG4EncodeSetConfig(handle, MPEG4_ENC_SETCONF_H263_ANNEX,      &h263_annex);

    if (SsbSipMPEG4EncodeExe(handle) != SSBSIP_MPEG4_ENC_RET_OK) {
        RETAILMSG(1,(L"MPEG4 Encoder Instance Initialization Failed.\r\n"));
        return 0;
    }

//
// 3. Get Input(YUV420) Buffer Address
//
    p_inbuf = SsbSipMPEG4EncodeGetInBuf(handle, 0);
    if (p_inbuf == NULL) {
        return 0;
    }
    
    for (i=0; i<ENC_LOOF; i++)
    {
        
//
// 4. Copy YUV into Input Buffer
//
        if( (fread(p_inbuf, 1, (width * height * 3) >> 1, fp_in)) != (width * height * 3) >> 1)
            break;

// 
// 5. Encode
//    
#ifdef FPS
        encodeTime = GetTickCount();
#endif
        do
        {
            ret = SsbSipMPEG4EncodeExe(handle) ;
            if(ret == SSBSIP_MPEG4_ENC_RET_OK)
                break;
            else
            {
//                if(ret == SSBSIP_MPEG4_ENC_RET_ERR_STATE_POWER_OFF)
                if(ret == -1004)
                               {
                    printD("[APP] MFC Sleeping -> Wait!!! .\r\n");
                    Sleep(1000);
                }
                else
                    break;
            }
        }while(1);

        if (ret != SSBSIP_MPEG4_ENC_RET_OK)
        {
            printD("[APP]H263 Encoder Failed! :: ret=%d \r\n", ret);
            break;
        }

#ifdef FPS
        encodeTime = GetTickCount() - encodeTime;
        sum_encodeTime+=encodeTime;
        ave_encodeTime=sum_encodeTime/(i+1);
        printf( "[Performance]encodeTime : %d  ave_encodeTime: %d \n", encodeTime,ave_encodeTime);
#endif

//        printD("[APP] Encode, ret=%d\n", ret);


//
// 6. Get Output(Stream) Buffer Address
//
        p_outbuf = SsbSipMPEG4EncodeGetOutBuf(handle, &size);
//        printD("[APP] Output Buf = 0x%X,  size = %d\n", (DWORD) p_outbuf, size);

// 
// 7. Make File
//
        if(i<ENC_MAKE_LOOF)
        {
            nWriteSize=fwrite(p_outbuf, 1, size, fp_out);
            if(nWriteSize!= size)
            {
                printD("[APP] Error fwrite :: size=%d \r\n", nWriteSize);
                break;
            }
        }        
        printD("[APP] %d Encoded.\r\n", i);
    }

    SsbSipMPEG4EncodeDeInit(handle);
    printD("[APP]  Deinit\r\n");

    fclose(fp_in);
    fclose(fp_out);

    return 0;
}





////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  MPEG4 encoder 관련 테스트 코드
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int mpeg4_enc_test(char *InputFile, char *OutputFile, unsigned int uiWidth, unsigned int uiHeight, unsigned int uiframerate, unsigned int uibitrate)
{
    void               *handle;
    FILE            *fp_in, *fp_out;
    int                ret, i;
    unsigned int     width, height;
    unsigned int    frame_rate, bitrate, gop_num;
    unsigned int   change_param[2];
    long               size;
    unsigned char *p_inbuf;
    unsigned char *p_outbuf;
    int            nWriteSize;

#ifdef FPS
    INT32    encodeTime;
    INT32    sum_encodeTime=0;
    INT32    ave_encodeTime=0;
#endif

    RETAILMSG(1,(L" ***** MPEG4 ENC START ***** \r\n\n"));
 //   printD(" ***** MPEG4 ENC START ***** \r\n\n");

//
// 0. Open File
//
//    fp_in = fopen("\\My Documents\\news_qvga.yuv","rb");
    fp_in = fopen(InputFile,"rb");
    if (fp_in == NULL) 
    {
        printD("[APP]File not found\n");
        return 0;
    }
    fp_out = fopen(OutputFile, "wb");
    if (fp_out == NULL) 
    {
        printD("[APP]Cannot open the output file.\n");
        return 0;

    }

// 
// 1. Encode Create Instance
//
    width      = uiWidth;
    height     = uiHeight;
    frame_rate = uiframerate;
    bitrate    = uibitrate;
    gop_num    = frame_rate;

    handle = SsbSipMPEG4EncodeInit(SSBSIPMFCENC_MPEG4, width,      height,
                                   frame_rate, bitrate, gop_num);
    if (handle == NULL)
    {
        return 0;
    }
    printD("[APP] HANDLE = 0x%X.\n", (DWORD) handle);

// 
// 2. Initialize Instance
//
    if (SsbSipMPEG4EncodeExe(handle) != SSBSIP_MPEG4_ENC_RET_OK) {
        RETAILMSG(1,(L"MPEG4 Encoder Instance Initialization Failed.\r\n"));
        return 0;
    }

//
// 3. Get Input(YUV420) Buffer Address
//
    p_inbuf = SsbSipMPEG4EncodeGetInBuf(handle, 0);
    if (p_inbuf == NULL) {
        return 0;
    }

    for (i=0; i<ENC_LOOF; i++)
    {

//
// 4. Copy YUV into Input Buffer
//
        if( (fread(p_inbuf, 1, (width * height * 3) >> 1, fp_in)) != (width * height * 3) >> 1)
            break;

// 
// 5. Encode
//        
#ifdef FPS
        encodeTime = GetTickCount();
#endif
        do
        {
            ret = SsbSipMPEG4EncodeExe(handle) ;
            if(ret == SSBSIP_MPEG4_ENC_RET_OK)
                break;
            else
            {
                if(ret == -1004)
                {
                    printD("[APP] MFC Sleeping -> Wait!!! .\r\n");
                    Sleep(1000);
                }
                else
                    break;
            }
        }while(1);

        if (ret != SSBSIP_MPEG4_ENC_RET_OK)
        {
            printD("[APP]MPEG4 Encoder Failed! :: ret=%d \r\n", ret);
            break;
        }

#ifdef FPS
        encodeTime = GetTickCount() - encodeTime;
        sum_encodeTime+=encodeTime;
        ave_encodeTime=sum_encodeTime/(i+1);
        printf( "[Performance]encodeTime : %d  ave_encodeTime: %d \n", encodeTime,ave_encodeTime);
#endif

//        printD("[APP] Encode, ret=%d\n", ret);


//
// 6. Get Output(Stream) Buffer Address
//
        p_outbuf = SsbSipMPEG4EncodeGetOutBuf(handle, &size);
//        printD("[APP] Output Buf = 0x%X,  size = %d\n", (DWORD) p_outbuf, size);

// 
// 7. Make File
//
        if(i<    ENC_MAKE_LOOF)
        {
            nWriteSize=fwrite(p_outbuf, 1, size, fp_out);
            if(nWriteSize!= size)
            {
                printD("[APP] Error fwrite :: size=%d \r\n", nWriteSize);
                break;
            }
        }        
        printD("[APP] %d Encoded.\r\n", i);
        
    }

    SsbSipMPEG4EncodeDeInit(handle);
    printD("[APP]  Deinit\r\n");

    fclose(fp_in);
    fclose(fp_out);

    return 0;
}


int _tmain()
{
    char *InputFile;

    //
    // Encoding Test
    //
    InputFile = "\\Storage Card\\input_qvga.yuv";
    {H264_enc_test  (InputFile,"\\output.264",320,240,15,384);                 printD("[APP]EncEnd output.264 \r\n\n\n");     }
    {H263_enc_test  (InputFile,"\\output.263",320,240,15,384);                 printD("[APP]EncEnd output.263 \r\n\n\n");     }
    {mpeg4_enc_test(InputFile,"\\output.m4v",320,240,15,384);                printD("[APP]EncEnd output.m4v \r\n\n\n");    } 

    //
    // Decoding Test
    //
    InputFile = "\\Storage Card\\Input.m4v";
    {mpeg4dec_test (InputFile, "\\output_m4v.yuv");                                printD("[APP]DecEnd input.m4v \r\n\n\n"); }
    return 0;
}
