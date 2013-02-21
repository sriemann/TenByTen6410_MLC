#include <windows.h>
#include <stdio.h>



#define R_RGB565(x)        (unsigned char) (((x) >> 8) & 0xF8)
#define G_RGB565(x)        (unsigned char) (((x) >> 3) & 0xFC)
#define B_RGB565(x)        (unsigned char) ((x) << 3)

// Generating RGB565 pixel by composing r, g, b components
#define RGB565(r, g, b)        (unsigned short) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))


extern const unsigned short icon[];


#include "exp_frame.h"
#include "imgproc.h"


static int PPM_Header(FILE *fp, int *wd, int *hi)
{
    char line[256];

    if (fp == NULL)
        return -1;

    // P6
    if (fgets(line, sizeof(line), fp) == NULL)
        return -1;
    if (memcmp(line, "P6", 2) != 0)
        return 1;

    // Skipping comment line
    do {
        if (fgets(line, sizeof(line), fp) == NULL)
            return -1;
    } while (line[0] == '#');

    // At this point, the line buffer contains width and height
    sscanf(line, "%d %d", wd, hi);

    // Max value for each R, G, B component
    if (fgets(line, sizeof(line), fp) == NULL)
        return -1;

    return 0;
}

int Write_PPM(int wd, int hi, unsigned char *p_img, char *filename)
{
    int   i, size;
    FILE *fp_write;
    unsigned short rgb565;
    unsigned char  rgb[3];

    fp_write = fopen(filename, "wb");

    fprintf(fp_write, "P6\n");
    fprintf(fp_write, "# Samsung Electronics\n");
    fprintf(fp_write, "%d %d\n255\n", wd, hi);

//    fwrite(p_img, 3, wd * hi, fp_write);

    size = wd * hi;
    for (i=0; i<size; i++) {
        rgb[0] = *p_img; p_img++;
        rgb[1] = *p_img; p_img++;
//        rgb565 = (rgb[0]<<8) | rgb[1];
        rgb565 = (rgb[1]<<8) | rgb[0];

        rgb[0] = R_RGB565(rgb565);
        rgb[1] = G_RGB565(rgb565);
        rgb[2] = B_RGB565(rgb565);

        fwrite(rgb, 1, 3, fp_write);
    }

    fclose(fp_write);

    return 0;
}

int Write_PGM(int wd, int hi, unsigned char *p_img, char *filename)
{
    int   i, size;
    FILE *fp_write;
    unsigned short rgb565;
    unsigned char  rgb[3];

    fp_write = fopen(filename, "wb");

    fprintf(fp_write, "P5\n");
    fprintf(fp_write, "# Samsung Electronics\n");
    fprintf(fp_write, "%d %d\n255\n", wd, hi);

    fwrite(p_img, 1, wd * hi, fp_write);

    fclose(fp_write);

    return 0;
}

int RGB888_To_RGB565(int wd, int hi, unsigned char *p_rgb888, unsigned char *p_rgb565)
{
    int      i, j;
    int      idx;
    unsigned short rgb565, *p_rgb565_short;
    unsigned char  rgb[3];

    idx = 0;
    p_rgb565_short = p_rgb565;

    for (i=0; i<hi*wd; i++) {
        rgb[0] = p_rgb888[idx++];
        rgb[1] = p_rgb888[idx++];
        rgb[2] = p_rgb888[idx++];
        rgb565 = RGB565(rgb[0], rgb[1], rgb[2]);
        p_rgb565_short[i] = rgb565;
    }


    return 0;
}

int RGB565_To_RGB888(int wd, int hi, unsigned char *p_rgb565, unsigned char *p_rgb888)
{
    int      i;
    int      idx;
    unsigned short rgb565, *p_rgb565_short;

    idx = 0;
    p_rgb565_short = p_rgb565;

    for (i=0; i<hi*wd; i++) {
//        rgb565 = (*(p_rgb565+1) << 8) | *(p_rgb565);
        rgb565 = (*(p_rgb565) << 8) | *(p_rgb565 + 1);

        p_rgb888[idx++] = R_RGB565(rgb565);
        p_rgb888[idx++] = G_RGB565(rgb565);
        p_rgb888[idx++] = B_RGB565(rgb565);
        p_rgb565 += 2;
    }

    return 0;
}
