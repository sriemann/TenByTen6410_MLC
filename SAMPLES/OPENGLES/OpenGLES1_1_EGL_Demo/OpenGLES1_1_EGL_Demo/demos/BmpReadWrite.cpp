#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include <gles/gl.h>
#include <math.h>

int ReadBmp (const char *filename, unsigned char  *buffer, int *width, int *height)
{
       FILE *f;
       BITMAPFILEHEADER bmfh;
       BITMAPINFOHEADER bmih;
       int i;
       unsigned char r, g, b;

       f = fopen(filename, "rb");
       if(f)
       {
               // read file header
               fread(&bmfh, 1, sizeof(bmfh), f);

               // write bitmap header
               fread(&bmih, 1, sizeof(bmih), f);
               *width = bmih.biWidth;
               *height = bmih.biHeight;
               for(i = 0;i < (*width)*(*height);++i)
               {
                       b = fgetc(f); /* write blue */
                       g = fgetc(f); /* write green */
                       r = fgetc(f);   /* write red */

                       *(buffer+4*i) = r;                    //r;
                       *(buffer+4*i+1) = g;                    //g;
                       *(buffer+4*i+2) = b;                    //b;
                       *(buffer+4*i+3) = 0x000000ff;        //a
               }
               fclose(f);
               return 1;
       }
       else
       {
               *width = 0;
               *height = 0;
               return 0;
       }
}

void WriteBmp(const char *pFilename, const unsigned int *pBuffer, int nWidth, int nHeight)
{
       FILE *pf;
       BITMAPFILEHEADER sBmfh;
       BITMAPINFOHEADER sBmih;
       int nIndex;
       unsigned char nR, nG, nB;
       unsigned int nColor_RGB8888;

       pf = fopen(pFilename, "wb");
       if (pf==NULL)
       {
               exit(-1);
       }

       // write file header
       sBmfh.bfType = *((unsigned short*)"BM");
       sBmfh.bfSize =  sizeof(BITMAPFILEHEADER) +
                                       sizeof(BITMAPINFOHEADER) +
                                       (nWidth * 3 * nHeight);

       sBmfh.bfReserved1 = 0;
       sBmfh.bfReserved2 = 0;
       sBmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

       fwrite(&sBmfh, 1, sizeof(sBmfh), pf);

       // write bitmap header
       sBmih.biSize = sizeof(sBmih);
       sBmih.biWidth = nWidth;
       sBmih.biHeight = nHeight;
       sBmih.biPlanes = 1;
       sBmih.biBitCount = 24;
       sBmih.biCompression = 0;
       sBmih.biSizeImage = 0;
       sBmih.biXPelsPerMeter = 10076;
       sBmih.biYPelsPerMeter = 10076;
       sBmih.biClrUsed = 0;
       sBmih.biClrImportant = 0;

       fwrite(&sBmih, 1, sizeof(sBmih), pf);

       //for(nIndex = nWidth*nHeight; nIndex--; )
       for (nIndex = 0; nIndex < nWidth * nHeight; nIndex++)
       {
               nColor_RGB8888 = pBuffer[nIndex];
               nR = (unsigned char) (((nColor_RGB8888) & 0xFF000000) >> 24);
               nG = (unsigned char) (((nColor_RGB8888) & 0x00FF0000) >> 16);
               nB = (unsigned char) (((nColor_RGB8888) & 0x0000FF00) >> 8);

               fputc(nB, pf); /* write blue */
               fputc(nG, pf); /* write green */
               fputc(nR, pf);   /* write red */
       }
       fclose(pf);
}
