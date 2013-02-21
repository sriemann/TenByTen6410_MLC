#include <windows.h>
#include <stdio.h>

#include "number_img.h"

#define R_RGB565(x)        (unsigned char) (((x) >> 8) & 0xF8)
#define G_RGB565(x)        (unsigned char) (((x) >> 3) & 0xFC)
#define B_RGB565(x)        (unsigned char) ((x) << 3)

// Generating RGB565 pixel by composing r, g, b components
#define RGB565(r, g, b)        (unsigned short) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))


static int find_octet(unsigned char *buf, int buf_size, unsigned char octet_list[], int octet_list_leng)
{
    int            i, j;
    unsigned char  b;

    for (i=0; i<buf_size; i++) {
        b = buf[i];
        for (j=0; j<octet_list_leng; j++) {
            if (b == octet_list[j])
                return i;
        }
    }

    return 0;
}


static int IMG_Header(unsigned char *img_hdr, int *wd, int *hi)
{
    unsigned char  octet_list;
    int            idx;
    unsigned char *p_img_hdr;

    octet_list = 0x0A;
    p_img_hdr  = img_hdr;

    idx = find_octet(p_img_hdr, 100, &octet_list, 1);

    // Skipping comment line
    do {
        p_img_hdr += (idx + 1);
        idx = find_octet(p_img_hdr, 100, &octet_list, 1);
        if (idx == 0)
            return -1;
    } while (p_img_hdr[0] == '#');

    // At this point, the line buffer contains width and height
    sscanf(p_img_hdr, "%d %d", wd, hi);

    p_img_hdr += (idx + 1);
    idx = find_octet(p_img_hdr, 100, &octet_list, 1);
    if (idx == 0)
        return -1;

    p_img_hdr += (idx + 1);

    return (int) (p_img_hdr - img_hdr);
}


static unsigned short  *num_img[NUM_CHAR_IMG];
static unsigned short   num_img_data[CHAR_IMG_WIDTH * CHAR_IMG_HEIGHT * NUM_CHAR_IMG];


BOOL NumImg_MemLoad(unsigned char *mem_ptr)
{
    int     i, j, number;    // Loop counter

    unsigned short *p_tmp;

    unsigned short *p_pix;

    unsigned char  *p_img;
    unsigned int    offset;

    int             width, height;


    if (mem_ptr == NULL)
        return FALSE;

    if ((offset = IMG_Header(mem_ptr, &width, &height)) <= 0) {
        RETAILMSG(1,(L"\nnumber image file is not supported format.\n"));
        return FALSE;
    }
    p_img = mem_ptr + offset;

    p_tmp = num_img_data;
    for (number=0; number<NUM_CHAR_IMG; number++) {
        // 숫자
        if ((number >= 0) && (number <= 9)) {
            num_img[number] = p_tmp;
            p_tmp += (CHAR_IMG_WIDTH * CHAR_IMG_HEIGHT);
        }
        // 소수점(period)
        else if (number == 10) {
            num_img[number] = p_tmp;
            p_tmp += (CHAR_IMG_WIDTH_HALF * CHAR_IMG_HEIGHT);
        }
        // 'fps' 문자 쓰기
        else if (number == 11) {
            num_img[number] = p_tmp;
            p_tmp += (CHAR_IMG_WIDTH_ONE_HALF * CHAR_IMG_HEIGHT);
        }
    }


    for (i=0; i<CHAR_IMG_HEIGHT; i++) {

        for (number=0; number<NUM_CHAR_IMG; number++) {

            // 숫자
            if ((number >= 0) && (number <= 9)) {

                p_pix = num_img[number] + (CHAR_IMG_WIDTH * i);

                for (j=0; j<CHAR_IMG_WIDTH; j++) {
                    *p_pix = RGB565(p_img[0], p_img[1], p_img[2]);
                    p_img += 3;
                    p_pix++;
                }
            }
            // 소수점(period)
            else if (number == 10) {

                p_pix = num_img[number] + (CHAR_IMG_WIDTH_HALF * i);

                for (j=0; j<CHAR_IMG_WIDTH_HALF; j++) {
                    *p_pix = RGB565(p_img[0], p_img[1], p_img[2]);
                    p_img += 3;
                    p_pix++;
                }
            }
            // 'fps' 문자 쓰기
            else if (number == 11) {

                p_pix = num_img[number] + (CHAR_IMG_WIDTH_ONE_HALF * i);

                for (j=0; j<CHAR_IMG_WIDTH_ONE_HALF; j++) {
                    *p_pix = RGB565(p_img[0], p_img[1], p_img[2]);
                    p_img += 3;
                    p_pix++;
                }
            }

        }
    }



    return TRUE;
}


BOOL NumImg_Write(int number, unsigned char *pImg, int width, int height, int x_pos, int y_pos)
{
    int   i;    // Loop counter

    unsigned short *p_num_img;

    if ((height < CHAR_IMG_HEIGHT) || (width < CHAR_IMG_WIDTH))
        return FALSE;

    pImg += ((y_pos * width) << 1);

    // 숫자 쓰기
    if ((number >= 0) && (number <= 9)) {
        p_num_img = num_img[number];
        pImg += (x_pos << 1);
        for (i=0; i<CHAR_IMG_HEIGHT; i++) {
            memcpy(pImg, p_num_img, CHAR_IMG_WIDTH*2);
            p_num_img += CHAR_IMG_WIDTH;
            pImg += (width << 1);
        }
    }
    // 소수점(period) 쓰기
    else if (number == 10) {
        p_num_img = num_img[number];

        pImg += (x_pos << 1);
        for (i=0; i<CHAR_IMG_HEIGHT; i++) {
            memcpy(pImg, p_num_img, CHAR_IMG_WIDTH_HALF*2);
            p_num_img += CHAR_IMG_WIDTH_HALF;
            pImg += (width << 1);
        }
    }
    // 'fps' 문자 쓰기
    else if (number == 11) {
        p_num_img = num_img[number];

        pImg += (x_pos << 1);
        for (i=0; i<CHAR_IMG_HEIGHT; i++) {
            memcpy(pImg, p_num_img, CHAR_IMG_WIDTH_ONE_HALF*2);
            p_num_img += CHAR_IMG_WIDTH_ONE_HALF;
            pImg += (width << 1);
        }
    }

    return TRUE;
}


void NumImg_Write_FPS(float fps, unsigned char *pImg, int width, int height, int x_pos, int y_pos)
{
    int   i;    // Loop counter
    int   number;
    int   string_leng;
    char  fraction_string[16];

    sprintf(fraction_string, "%.1f", fps);
    string_leng = strlen(fraction_string);


    // Integer part
    for (i=0; i<(string_leng - 2); i++) {
        number = (int) (fraction_string[i] - '0');
        NumImg_Write(number, pImg, width, height, x_pos, y_pos);
        x_pos += CHAR_IMG_WIDTH;
    }
    // Put the period
    NumImg_Write(10, pImg, width, height, x_pos, y_pos);
    x_pos += CHAR_IMG_WIDTH_HALF;
    // Fractional part
    number = (int) (fraction_string[string_leng-1] - '0');
    NumImg_Write(number, pImg, width, height, x_pos, y_pos);
    x_pos += CHAR_IMG_WIDTH;
    // Write 'fps' string
    NumImg_Write(11, pImg, width, height, x_pos, y_pos);
    x_pos += CHAR_IMG_WIDTH_ONE_HALF;
}



static unsigned short  *size_img[NUM_SIZE_IMG];
static unsigned short   num_size_data[SIZE_IMG_WIDTH * SIZE_IMG_HEIGHT * NUM_SIZE_IMG];

BOOL SizeImg_MemLoad(unsigned char *mem_ptr)
{
    int     i, j, k;    // Loop counter

    unsigned short *p_pix;

    unsigned char  *p_img;
    unsigned int    offset;

    int             width, height;


    if (mem_ptr == NULL)
        return FALSE;

    if ((offset = IMG_Header(mem_ptr, &width, &height)) <= 0) {
        RETAILMSG(1,(L"\nSize image file is not supported format.\n"));
        return FALSE;
    }
    p_img = mem_ptr + offset;

    for (i=0; i<NUM_SIZE_IMG; i++) {
        size_img[i] = num_size_data + (i * SIZE_IMG_WIDTH * SIZE_IMG_HEIGHT);
    }


    for (i=0; i<SIZE_IMG_HEIGHT; i++) {

        for (j=0; j<NUM_SIZE_IMG; j++) {

            p_pix = size_img[j] + (SIZE_IMG_WIDTH * i);

            for (k=0; k<SIZE_IMG_WIDTH; k++) {
                *p_pix = RGB565(p_img[0], p_img[1], p_img[2]);
                p_img += 3;
                p_pix++;
            }
        }
    }


    return TRUE;
}

BOOL SizeImg_Write(SIZE_IMG_IDX size_idx, unsigned char *pImg, int width, int height, int x_pos, int y_pos)
{
    int   i;    // Loop counter

    unsigned short *p_size_img;

    if ((height < SIZE_IMG_HEIGHT) || (width < SIZE_IMG_WIDTH))
        return FALSE;

    pImg += ((y_pos * width) << 1);

    switch (size_idx) {
    case SIZE_IMG_QVGA:        // QVGA
    case SIZE_IMG_VGA:        // VGA
    case SIZE_IMG_SD:        // SD
        break;

    case SIZE_IMG_UNDEF:    // undef
        return FALSE;

    default:
        RETAILMSG(1,(L"\nInvalid size_val value, %d\n", size_idx));
        return FALSE;
    }

    p_size_img = size_img[size_idx];
    pImg += (x_pos << 1);
    for (i=0; i<SIZE_IMG_HEIGHT; i++) {
        memcpy(pImg, p_size_img, SIZE_IMG_WIDTH*2);
        p_size_img += SIZE_IMG_WIDTH;
        pImg += (width << 1);
    }


    return TRUE;
}

