#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FrameExtractor.h"
#include "MPEG4Frames.h"
#include "H263Frames.h"
#include "H264Frames.h"
#include "VC1Frames.h"


#include "FileRead.h"
#ifndef DATAREAD_TYPE
#error "DATAREAD_TYPE is not defined."
#endif
#if (DATAREAD_TYPE == DATA_FILE)
#define FRAMEX_IN_TYPE_SEL        (FRAMEX_IN_TYPE_FILE)
#else
#define FRAMEX_IN_TYPE_SEL        (FRAMEX_IN_TYPE_MEM)
#endif


//unsigned char delimiter[4] = {0x00, 0x00, 0x00, 0x01};
unsigned char delimiter_mpg4[] = {0x00, 0x00, 0x01};
unsigned char delimiter_h263[] = {0x00, 0x00, 0x80};
unsigned char delimiter_h264[] = {0x00, 0x00, 0x00, 0x01};


//#define MAX_FRAME_SIZE    (16384 * 7)
//#define MAX_FRAME_SIZE    (409600)
#define MAX_FRAME_SIZE    (429600)
#define DELTA_INCREASE    2048

void foo(char *filename)
{
    int   ret;
    int   num_frame, num_filled, max_size = 0;
    FILE *fp;

    FRAMEX_CTX *pCTX;
    unsigned char *outbuf, *tmpbuf;
    int            outbuf_size;

    unsigned char  *delimiter = delimiter_h264;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }

    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;


    //////////////////////////////////////
    /////     FrameExtractorInit     /////
    //////////////////////////////////////
    pCTX = FrameExtractorInit(FRAMEX_IN_TYPE_FILE, delimiter, 4, 1);


    ///////////////////////////////////////
    /////     FrameExtractorFirst     /////
    ///////////////////////////////////////
    ret = FrameExtractorFirst(pCTX, fp);

    num_frame = 0;
    do {
        //////////////////////////////////////
        /////     FrameExtractorNext     /////
        //////////////////////////////////////
        ret = FrameExtractorNext(pCTX, fp, outbuf, outbuf_size, &num_filled);
        while (ret == FRAMEX_CONTINUE) {
            tmpbuf = (unsigned char *) malloc(outbuf_size + DELTA_INCREASE);
            memcpy(tmpbuf, outbuf, outbuf_size);
            outbuf_size += DELTA_INCREASE;

            free(outbuf);
            outbuf = tmpbuf;
            tmpbuf = NULL;
            ret = FrameExtractorNext(pCTX, fp, outbuf, outbuf_size, &num_filled);
        }

        if (ret != FRAMEX_OK)
            break;

        // 한 frame의 최대 길이를 측정한다.
        if (max_size < num_filled)
            max_size = num_filled;

        num_frame++;
    } while(1);

    switch (ret) {
    case FRAMEX_ERR_EOS:
        printf("End of Stream. (maxsize=%d)", max_size);
        break;

    case FRAMEX_ERR_NOTFOUND:
        printf("No such frame delimiter.");
        break;

    case FRAMEX_ERR_BUFSIZE_TOO_SMALL:
        printf("Buffer size is too small.");
        break;

    }

    printf("\n### Number of frames = %d\n", num_frame);


    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////
    FrameExtractorFinal(pCTX);

    free(outbuf);

    fclose(fp);
}



void main01()
{
    LPCTSTR  filename = TEXT("C:\\Utils\\resin-3.0.18\\webapps\\ROOT\\Vectors\\H.264\\ultraviolet-640x480.264");
//    LPCTSTR  filename = TEXT("C:\\Utils\\resin-3.0.18\\webapps\\ROOT\\Vectors\\MPEG4\\spider-man_3-720x480.m4v");
//    char *filename = "balls_of_fury_640_480.m4v";

    int   ret;
    int   num_frame, num_filled, max_size = 0;

    FILE   *fp2;

    FILE   *strm_ptr;

    FRAMEX_CTX      *pCTX;
//    FRAMEX_STRM_PTR *strm_ptr;
    unsigned char   *outbuf, *tmpbuf;
    int              outbuf_size;

    unsigned char   *delimiter = delimiter_h264;

    DWORD tick;


    strm_ptr = fopen(filename, "rb");
    if (strm_ptr == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }


    fp2 = fopen("output1", "wb");
    if (fp2 == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }


    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;

    tick = GetTickCount();

    //////////////////////////////////////
    /////     FrameExtractorInit     /////
    //////////////////////////////////////
    pCTX = FrameExtractorInit(FRAMEX_IN_TYPE_FILE, delimiter, 4, 1);


    ///////////////////////////////////////
    /////     FrameExtractorFirst     /////
    ///////////////////////////////////////
    ret = FrameExtractorFirst(pCTX, strm_ptr);

    num_frame = 0;
    do {
        //////////////////////////////////////
        /////     FrameExtractorNext     /////
        //////////////////////////////////////
        ret = FrameExtractorNext(pCTX, strm_ptr, outbuf, outbuf_size, &num_filled);
        while (ret == FRAMEX_CONTINUE) {
            tmpbuf = (unsigned char *) malloc(outbuf_size + DELTA_INCREASE);
            memcpy(tmpbuf, outbuf, outbuf_size);
            outbuf_size += DELTA_INCREASE;

            free(outbuf);
            outbuf = tmpbuf;
            tmpbuf = NULL;
            ret = FrameExtractorNext(pCTX, strm_ptr, outbuf, outbuf_size, &num_filled);
        }

        if (ret != FRAMEX_OK) {
//            strm_ptr->p_cur = strm_ptr->p_start;
            break;
        }

        // 한 frame의 최대 길이를 측정한다.
        if (max_size < num_filled)
            max_size = num_filled;

//    printf("\n[%003d] = %d\n", num_frame, num_filled);


/*
        fprintf(fp2, "\tmpeg4_%d[] = {", num_frame);
        for (z=0; z<num_filled; z++) {
            if (z % 25 == 24) {
                fprintf(fp2, "\n\t             ");
            }
            fprintf(fp2, "0x%02X, ", outbuf[z]);
        }

        fprintf(fp2, "};\n");
*/

        fwrite(outbuf, 1, num_filled, fp2);

        num_frame++;
        if (num_frame == 1000)
            break;
    } while(1);

    switch (ret) {
    case FRAMEX_ERR_EOS:
        printf("End of Stream. (maxsize=%d)", max_size);
        break;

    case FRAMEX_ERR_NOTFOUND:
        printf("No such frame delimiter.");
        break;

    case FRAMEX_ERR_BUFSIZE_TOO_SMALL:
        printf("Buffer size is too small.");
        break;

    }

    tick = GetTickCount() - tick;
    printf("\nTime = %d\n", tick);

    ret = FrameExtractorPeek(pCTX, strm_ptr, outbuf+10, 8, &num_filled);
    ret = FrameExtractorPeek(pCTX, strm_ptr, outbuf, 8, &num_filled);

    printf("\n### Number of frames = %d\n", num_frame);


    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////
    FrameExtractorFinal(pCTX);

    free(outbuf);

    fclose(fp2);
}



void mpeg4()
{
    LPCTSTR  filename = TEXT("ghost_rider-320x240.m4v");


    int    num_frames_convert;

    int    i, ret;
    int    num_filled, max_size = 0;
    FILE  *fp;

    FRAMEX_CTX *pCTX;
    unsigned char *outbuf;
    int            outbuf_size;

    MPEG4_CONFIG_DATA   mpeg4_conf;
    unsigned int        coding_type;
    unsigned char      *delimiter = delimiter_mpg4;


    num_frames_convert = 100;



    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }

    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;


    //////////////////////////////////////
    /////     FrameExtractorInit     /////
    //////////////////////////////////////
    pCTX = FrameExtractorInit(FRAMEX_IN_TYPE_FILE, delimiter, 3, 1);


    ///////////////////////////////////////
    /////     FrameExtractorFirst     /////
    ///////////////////////////////////////
    ret = FrameExtractorFirst(pCTX, fp);


    num_filled = ExtractConfigStreamMpeg4(pCTX, fp, outbuf, outbuf_size, &mpeg4_conf);

    printf("\nWidth  = [%d]\nHeight = [%d]\n", mpeg4_conf.width, mpeg4_conf.height);
    printf("\n{%02d}Num = %d", 0, num_filled);

    for (i=1; i<=num_frames_convert; i++) {
        num_filled = NextFrameMpeg4(pCTX, fp, outbuf, outbuf_size, &coding_type);
        if (num_filled == 0)
            break;


//        fwrite(outbuf, 1, num_filled, fp2);
        printf("\n{%02d}Num = %d,\t%d", i, num_filled, coding_type);

        if (max_size < num_filled)
            max_size = num_filled;
    }

    printf("\ni = %d, max_size = %d\n", i, max_size);

    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////

    free(outbuf);

    fclose(fp);
}




void h263()
{
    LPCTSTR  filename = TEXT("car16.263");

    int   num_frames_convert;

    int   i, ret;
    int   num_filled, max_size = 0;
    FILE *fp, *fp2;

    FRAMEX_CTX *pCTX;
    unsigned char *outbuf;
    int            outbuf_size;

    H263_CONFIG_DATA    h263_conf;
    unsigned int        coding_type;
    unsigned char      *delimiter = delimiter_h263;


    num_frames_convert = 100;



    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }

    fp2 = fopen("out.h", "wb");
    if (fp2 == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }


    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;


    //////////////////////////////////////
    /////     FrameExtractorInit     /////
    //////////////////////////////////////
    pCTX = FrameExtractorInit(FRAMEX_IN_TYPE_FILE, delimiter, 3, 1);


    ///////////////////////////////////////
    /////     FrameExtractorFirst     /////
    ///////////////////////////////////////
    ret = FrameExtractorFirst(pCTX, fp);


    num_filled = ExtractConfigStreamH263(pCTX, fp, outbuf, outbuf_size, &h263_conf);

    for (i=0; i<num_frames_convert; i++) {
        num_filled = NextFrameH263(pCTX, fp, outbuf, outbuf_size, &coding_type);
        if (num_filled == 0)
            break;

        printf("\n{%02d}Num = %d,\t%d", i, num_filled, coding_type);

        if (max_size < num_filled)
            max_size = num_filled;
    }

    printf("\ni = %d, max_size = %d\n", i, max_size);

    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////

    free(outbuf);

    fclose(fp);
    fclose(fp2);
}

void h264test()
{
//    LPCTSTR  filename = TEXT("C:\\Program Files\\War-ftpd\\FTPRoot\\pub\\TestVectors\\h264_vectors\\Football(VGA)\\fb_3000k.264");
//    LPCTSTR  filename = TEXT("walk_vga_vbr_1951k_25fps_daci264.avc");

    LPCTSTR  filename = TEXT("D:\\FTPRoot\\anonymous\\pub\\TestVectors\\H.264_BP\\Stream\\250_Allegro_BDWIDTH_CAVLC_B01_L30_SD480_5.7.26l");



    int     i, ret;
    int     num_filled, max_size = 0;
    void   *inp;
    FILE   *fp2;

    FRAMEX_CTX *pCTX;
    unsigned char *outbuf;
    int            outbuf_size;

    H264_CONFIG_DATA    h264_conf;
    unsigned int        coding_type;
    unsigned char      *delimiter = delimiter_h264;

    inp = SSB_FILE_OPEN(filename);
    if (inp == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }

    fp2 = fopen("out.264", "wb");
    if (fp2 == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }


    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;


    //////////////////////////////////////
    /////     FrameExtractorInit     /////
    //////////////////////////////////////
    pCTX = FrameExtractorInit(FRAMEX_IN_TYPE_SEL, delimiter, 4, 1);


    ///////////////////////////////////////
    /////     FrameExtractorFirst     /////
    ///////////////////////////////////////
    ret = FrameExtractorFirst(pCTX, inp);


    num_filled = ExtractConfigStreamH264(pCTX, inp, outbuf, outbuf_size, &h264_conf);
    fwrite(outbuf, 1, num_filled, fp2);

    printf("\n[[ width, height ]] = (%d, %d)\n", h264_conf.width, h264_conf.height);

    for (i=1; i<1000; i++) {
        num_filled = NextFrameH264(pCTX, inp, outbuf, outbuf_size, &coding_type);
        if (num_filled == 0) {
            break;

            SSB_FILE_REWIND(inp);
            FrameExtractorFirst(pCTX, inp);
            num_filled = NextFrameH264(pCTX, inp, outbuf, outbuf_size, &coding_type);
        }
        fwrite(outbuf, 1, num_filled, fp2);
        printf("\n{%02d}Num = %d,\t%d", i, num_filled, coding_type);

        if (max_size < num_filled)
            max_size = num_filled;

    }

    printf("\ni = %d, max_size = %d\n", i, max_size);

    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////

    free(outbuf);

    SSB_FILE_CLOSE(inp);
    fclose(fp2);
}


void vc1test()
{
    LPCTSTR  filename = TEXT("D:\\Temp\\VC1테스트벡터\\003_comdex_wmv9_cbr_0_one-pass_simple_progressive_0_0_0_3_0_0_58000bps_176x144_10fps_64kbps_44khz_s.rcv");

    int   i;
    int   num_filled, acc_size, max_size = 0;
    void *fp;

    unsigned char *outbuf;
    int            outbuf_size;

    VC1_CONFIG_DATA   vc1_conf_data;


    fp = SSB_FILE_OPEN(filename);
    if (fp == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }

    acc_size = 0;

    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;


    num_filled = ExtractConfigStreamVC1(fp, outbuf, outbuf_size, &vc1_conf_data);
    if (max_size < num_filled)
        max_size = num_filled;
    acc_size += num_filled;

    for (i=0; i<4000; i++) {

        num_filled = NextFrameVC1(fp, outbuf, outbuf_size, NULL);
        if (num_filled == 0)
            break;
        acc_size += num_filled;
        printf("\n[%d] Num = %d,\tAcc = %d", i+1, num_filled, acc_size);

        if (max_size < num_filled)
            max_size = num_filled;
    }

    printf("\ni = %d, max_size = %d\n", i, max_size);

    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////

    free(outbuf);

    SSB_FILE_CLOSE(fp);
}



static int h264_parsing_test(LPCTSTR filename)
{
    int     i;    // Loop counter
    int     nFrames, nLoop;
    int     ret, iRet;
    int     num_filled, max_size = 0;
    void   *inp;

    FRAMEX_CTX *pCTX;
    unsigned char *outbuf;
    int            outbuf_size;

    H264_CONFIG_DATA    h264_conf;
    unsigned int        coding_type;
    unsigned char      *delimiter = delimiter_h264;

    inp = SSB_FILE_OPEN(filename);
    if (inp == NULL) {
        printf("[Error] %s file not found\n", filename);
        return -1;
    }


    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;


    //////////////////////////////////////
    /////     FrameExtractorInit     /////
    //////////////////////////////////////
    pCTX = FrameExtractorInit(FRAMEX_IN_TYPE_SEL, delimiter, 4, 1);


    ///////////////////////////////////////
    /////     FrameExtractorFirst     /////
    ///////////////////////////////////////
    ret = FrameExtractorFirst(pCTX, inp);


    num_filled = ExtractConfigStreamH264(pCTX, inp, outbuf, outbuf_size, &h264_conf);

    printf("\n----------------------------");
    printf("\n- Filename : %s", filename);
    printf("\n[[ width, height ]] = (%d, %d)\n", h264_conf.width, h264_conf.height);

    for (nFrames=0, nLoop=0;  nLoop<2  ; nFrames++) {
        num_filled = NextFrameH264(pCTX, inp, outbuf, outbuf_size, &coding_type);
        if (num_filled == 0) {
            nLoop++;
            nFrames++; break;
            SSB_FILE_REWIND(inp);

            FrameExtractorFirst(pCTX, inp);
            num_filled = NextFrameH264(pCTX, inp, outbuf, outbuf_size, &coding_type);
        }
        if (max_size < num_filled)
            max_size = num_filled;

    }

    printf("\nframes = %d, max_size = %d", nFrames / nLoop, max_size);


bail_out:
    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////
    FrameExtractorFinal(pCTX);

    free(outbuf);
    SSB_FILE_CLOSE(inp);

    return iRet;
}


void h264_parser_batch_test()
{
    int              iRet;

    HANDLE           hFindFile;
    WIN32_FIND_DATA  find_data;

    LPCTSTR  folderpath = "D:\\FTPRoot\\anonymous\\pub\\TestVectors\\H.264_BP\\Stream\\";
    CHAR     filepath[512];


    strcpy(filepath, folderpath);
//    strcat(filepath, "*.26?");
    strcat(filepath, "222_Allegro_PROCESSING_CAVLC_B00_L30_5x6_5.7.26l");

    hFindFile = FindFirstFile(filepath, &find_data);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return;

    do {
        strcpy(filepath, folderpath);
        strcat(filepath, find_data.cFileName);
        iRet = h264_parsing_test(filepath);

        if (FindNextFile(hFindFile, &find_data) == FALSE)
            break;

    } while (1);

    FindClose(hFindFile);

    printf("\n");
}


void main(char *argv[], int argc)
{
//    mpeg4();

//    h263();

//    h264test();

    vc1test();


//    h264_parser_batch_test();

}


int vc1maxtest(LPCTSTR  filename)
{
    int   i, ret;
    int   num_filled, max_size = 0;
    FILE *fp;

    unsigned char *outbuf;
    int            outbuf_size;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("[Error] %s file not found\n", filename);
        return;
    }


    outbuf = (unsigned char *) malloc(MAX_FRAME_SIZE);
    outbuf_size = MAX_FRAME_SIZE;


    num_filled = ExtractConfigStreamVC1(fp, outbuf, outbuf_size, NULL);
    if (max_size < num_filled)
        max_size = num_filled;

    for (i=0; i<3600; i++) {

        num_filled = NextFrameVC1(fp, outbuf, outbuf_size, NULL);
        if (num_filled == 0)
            break;

        if (max_size < num_filled)
            max_size = num_filled;
    }


    ///////////////////////////////////////
    /////     FrameExtractorFinal     /////
    ///////////////////////////////////////

    free(outbuf);

    fclose(fp);

    return max_size;
}


void main011()
{
    int max_size=0;

    LPCTSTR  folderpath = "D:\\Temp\\VC1테스트벡터\\";
    CHAR     filepath[512];

    FILE     *fp;

    HANDLE           hFindFile;
    WIN32_FIND_DATA  find_data;

    strcpy(filepath, folderpath);
    strcat(filepath, "*.rcv");
    hFindFile = FindFirstFile(filepath, &find_data);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return;

    fp = fopen("out.txt", "wt");

    do {
        strcpy(filepath, folderpath);
        strcat(filepath, find_data.cFileName);
        max_size = vc1maxtest(filepath);

        fprintf(fp, "\n[%s]\t\t max_size = %d\n", find_data.cFileName, max_size);

        if (FindNextFile(hFindFile, &find_data) == FALSE)
            break;

    } while (1);


    FindClose(hFindFile);

    fclose(fp);

}

