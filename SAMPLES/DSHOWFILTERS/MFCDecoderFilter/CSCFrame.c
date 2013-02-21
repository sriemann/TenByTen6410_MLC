#include <stdio.h>
#include <string.h>
#include <math.h>
/*
#include "common.h"
#include "frame.h"

#include "assert_exp.h"
*/
long int crv_tab[256];
long int cbu_tab[256];
long int cgu_tab[256];
long int cgv_tab[256];
long int tab_76309[256];
unsigned char clp[1024];

extern void _initConvTab();

#define CRGBY    65535
#define CRV        90111
#define CBU        114686
#define CGU        21504
#define CGV        45055

/*
 R = Y + (                + 90111*(V-128) ) >> 16
 G = Y - (  21504*(U-128) + 45055*(V-128) ) >> 16
 B = Y + ( 114686*(U-128)                 ) >> 16
*/

void _initConvTab()
{
    static int finitTab = 0;

    long int crv, cbu, cgu, cgv;
    int i, ind;

    if (finitTab != 0)
        return;


    crv = CRV; cbu = CBU;
    cgu = CGU; cgv = CGV;

    for (i = 0; i < 256; i++) {
        crv_tab[i] = (i-128) * crv;
        cbu_tab[i] = (i-128) * cbu;
        cgu_tab[i] = (i-128) * cgu;
        cgv_tab[i] = (i-128) * cgv;
        tab_76309[i] = CRGBY * i;
    }

    for (i=0; i<384; i++)
        clp[i] =0;
    ind=384;
    for (i=0;i<256; i++)
        clp[ind++]=i;
    ind=640;
    for (i=0;i<384;i++)
        clp[ind++]=255;

    finitTab = 1;
}

void _yuv420ToRgb565(unsigned char *p_lum, unsigned char *p_cb, unsigned char *p_cr, int w_src, int h_src,
                            unsigned char *dest,  int w_dst, int h_dst,
                            int topdown)
{
    int i, j;
    int y1, y2, u, v;
    unsigned char *py1, *py2, *pu, *pv;
    int c1, c2, c3, c4;
    unsigned short *d1, *d2;
    unsigned short *ptrarr[640];

    unsigned char r, g, b;

    // s_dw, s_dh: 소스 이미지에서 skip할 width와 height양
    // d_dw, d_dh: 결과 이미지에서 skip할 width와 height양
    // 소스 이미지의 width가 결과 이미지의 width보다 큰 경우, width clipping이 필요하다.
    // 이 때, skip을 통해서 clipping을 한다. s_dw=(w_src-w_dst)이 되고, d_dw=0이 된다.
    int s_dw, s_dh, d_dw, d_dh;

    // yuv-to-rgb를 수행하는 실제 width와 height.
    int width, height;

    width =  ((w_src < w_dst) ? w_src : w_dst);
    height = ((h_src < h_dst) ? h_src : h_dst);

    if (w_src > w_dst) {
        s_dw = w_src - w_dst;
        d_dw = 0;
    }
    else {
        s_dw = 0;
        d_dw = w_dst - w_src;
    }
    if (h_src > h_dst) {
        s_dh = h_src - h_dst;
        d_dh = 0;
    }
    else {
        s_dh = 0;
        d_dh = h_dst - h_src;
    }

    //Initialization
    py1 = p_lum;
    py2 = py1 + w_src;
    pu  = p_cb;
    pv  = p_cr;

    switch (topdown) {
    case 0:    // Top -> Down
        ptrarr[0] = (unsigned short *) dest;
        for (j = 1; j < height; j++) {
            ptrarr[j] = ptrarr[j-1] + w_dst;
        }
        break;

    default: // Down -> Top
        ptrarr[0] = ((unsigned short *) dest) + (w_dst * (h_dst-1));        // 좌상단
        for (j = 1; j < height; j++) {
            ptrarr[j] = ptrarr[j-1] - w_dst;
        }
        break;
    }

    for (j = 0; j < height; j += 2) {

        d1 = ptrarr[j];
        d2 = ptrarr[j + 1];

        for (i = 0; i < width; i += 2) {

            u = *pu++;
            v = *pv++;

            c1 = crv_tab[v];
            c2 = cgu_tab[u];
            c3 = cgv_tab[v];
            c4 = cbu_tab[u];

            //up-left
            y1 = tab_76309[*py1++];
            r = clp[384+((y1 + c1)>>16)];        // R
            g = clp[384+((y1 - c2 - c3)>>16)];    // G
            b = clp[384+((y1 + c4)>>16)];        // B
            *d1++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

            //up-right
            y1 = tab_76309[*py1++];
            r = clp[384+((y1 + c1)>>16)];        // R
            g = clp[384+((y1 - c2 - c3)>>16)];    // G
            b = clp[384+((y1 + c4)>>16)];        // B
            *d1++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

            //down-left
            y2 = tab_76309[*py2++];
            r = clp[384+((y2 + c1)>>16)];        // R
            g = clp[384+((y2 - c2 - c3)>>16)];    // G
            b = clp[384+((y2 + c4)>>16)];        // B
            *d2++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

            //down-right
            y2 = tab_76309[*py2++];
            r = clp[384+((y2 + c1)>>16)];        // R
            g = clp[384+((y2 - c2 - c3)>>16)];    // G
            b = clp[384+((y2 + c4)>>16)];        // B
            *d2++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }
        py1 += (w_src + s_dw);
        py2 += (w_src + s_dw);
        pu  += (s_dw>>1);
        pv  += (s_dw>>1);
    }

}
