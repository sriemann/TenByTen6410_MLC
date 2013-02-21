//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __MEM_POST_H__
#define __MEM_POST_H__

#include "windows.h"
#include "MEMMisc.h"
#include "S3c6400_display.h"

#define MAX_FRM_BUF_NUM        30

#define S3C6400_BASE_REG_POST        0x77000000
#define S3C6400_BASE_FRAME_POST        0x57000000
#define TMP_POST_BUFFER                0x56800000
#define POST_FRAME_BUF_SIZE            8388608 // 8MB


//POSTENVID
#define POST_START              (0x1U<<31) // khlee
#define POST_DISABLE            (0x0U<<31)

//MODE Control register
#define AUTOLOAD_ENABLE         (0x1<<14)
#define FIFO_OUT_ENABLE       (0x1<<13)
#define PROGRESSIVE_SCAN        (0x1<<12)
#define POST_INT_ENABLE         (0x1<<7)
#define POST_PENDING            (0x1<<6)
#define IRQ_LEVEL               (0x1<<5)
#define H_CLK_INPUT              (0x0<<2)
#define EXT_CLK_0_INPUT          (0X1<<2)
#define EXT_CLK_1_INPUT          (0x3<<2)

//MODE Control register 2
#define ADDR_CHANGE_ENABLE      (0x0<<4)
#define ADDR_CHANGE_DISABLE     (0x1<<4)
#define CHANGE_AT_FIELD_END     (0x0<<3)
#define CHANGE_AT_FRAME_END     (0x1<<3)
#define SOFTWARE_TRIGGER        (0x0<<0)
#define HARDWARE_TRIGGER        (0x1<<0)


typedef struct  POST_SFR
{
    UINT32 POST_MODE_CTRL;           //= POST_BASE + 0x00,
    UINT32 POST_PRESCALE_RATIO;      //= POST_BASE + 0x04,
    UINT32 POST_PRESCALE_IMG_SZ;     //= POST_BASE + 0x08,
    UINT32 POST_SRC_IMG_SZ;          //= POST_BASE + 0x0C,
    UINT32 POST_MAIN_SCALE_HRATIO;   //= POST_BASE + 0x10, //dx
    UINT32 POST_MAIN_SCALE_VRATIO;   //= POST_BASE + 0x14, //dy
    UINT32 POST_DST_IMG_SZ;          //= POST_BASE + 0x18,
    UINT32 POST_PRESCALE_SHIFT;      //= POST_BASE + 0x1C,
    UINT32 POST_START_ADDR_Y;        //= POST_BASE + 0x20,
    UINT32 POST_START_ADDR_CB;       //= POST_BASE + 0x24,
    UINT32 POST_START_ADDR_CR;       //= POST_BASE + 0x28,
    UINT32 POST_START_ADDR_RGB;      //= POST_BASE + 0x2C,
    UINT32 POST_END_ADDR_Y;             //= POST_BASE + 0x30,
    UINT32 POST_END_ADDR_CB;         //= POST_BASE + 0x34,
    UINT32 POST_END_ADDR_CR;         //= POST_BASE + 0x38,
    UINT32 POST_END_ADDR_RGB;        //= POST_BASE + 0x3C,
    UINT32 POST_OFFSET_ADDR_Y;       //= POST_BASE + 0x40,
    UINT32 POST_OFFSET_ADDR_CB;      //= POST_BASE + 0x44,
    UINT32 POST_OFFSET_ADDR_CR;      //= POST_BASE + 0x48,
    UINT32 POST_OFFSET_ADDR_RGB;     //= POST_BASE + 0x4C,
    UINT32 POST_EXT_FB;              //= POST_BASE + 0x50

    // Added in v2.2
    UINT32 POST_NEXT_ADDR_START_Y;          // = 0x54,
    UINT32 POST_NEXT_ADDR_START_CB;         // = 0x58,
    UINT32 POST_NEXT_ADDR_START_CR;         // = 0x5C,
    UINT32 POST_NEXT_ADDR_START_RGB;        // = 0x60,
    UINT32 POST_NEXT_ADDR_END_Y;            // = 0x64,
    UINT32 POST_NEXT_ADDR_END_CB;           // = 0x68,
    UINT32 POST_NEXT_ADDR_END_CR;           // = 0x6C,
    UINT32 POST_NEXT_ADDR_END_RGB;          // = 0x70,

    // Added in v2.3/v2.4
    UINT32 POST_ADDR_START_OUT_CB;         // = 0x74,
    UINT32 POST_ADDR_START_OUT_CR;         // = 0x78,
    UINT32 POST_ADDR_END_OUT_CB;           // = 0x7c,
    UINT32 POST_ADDR_END_OUT_CR;           // = 0x80,
    UINT32 POST_OFFSET_OUT_CB;             // = 0x84,
    UINT32 POST_OFFSET_OUT_CR;             // = 0x88,
    UINT32 POST_NEXT_ADDR_START_OUT_CB;    // = 0x8C,
    UINT32 POST_NEXT_ADDR_START_OUT_CR;    // = 0x90,
    UINT32 POST_NEXT_ADDR_END_OUT_CB;      // = 0x94,
    UINT32 POST_NEXT_ADDR_END_OUT_CR;      // = 0x98,
    UINT32 POST_START_VIDEO;               // = 0x9c,    // khlee
    UINT32 POST_MODE_CTRL_2;               // = 0xA0

} S3C6400_POST_SFR;

typedef enum
{
    PAL1, PAL2, PAL4, PAL8,
    RGB8, ARGB8, RGB16, ARGB16, RGB18, RGB24, RGB30, ARGB24,
    YC420, YC422, // Non-interleave
    CRYCBY, CBYCRY, YCRYCB, YCBYCR, YUV444 // Interleave
}CSPACE;

typedef enum
{
    POST_DMA, POST_FIFO
} POST_PATH;

typedef enum
{
    FREE_RUN, ONE_SHOT
} POST_RUN_MODE;

typedef enum
{
    HCLK = 0, PLL_EXT = 1, EXT_27MHZ = 3
} POST_CLK_SRC;

typedef struct
{
    u32  m_uBaseAddr;
    u32  m_uModeRegValue;
    CSPACE m_eSrcCSpace, m_eDstCSpace;
    BOOL m_bFreeRunMode;

    u32  m_uLastFrmBufIdx;
    u32  m_uSrcFrmStAddr[MAX_FRM_BUF_NUM];
    u32  m_uSrcStY[MAX_FRM_BUF_NUM], m_uSrcStCb[MAX_FRM_BUF_NUM], m_uSrcStCr[MAX_FRM_BUF_NUM];
    u32  m_uSrcEndY[MAX_FRM_BUF_NUM], m_uSrcEndCb[MAX_FRM_BUF_NUM], m_uSrcEndCr[MAX_FRM_BUF_NUM];

    u32  m_uStPosY, m_uEndPosY;
    u32  m_uStPosCb, m_uStPosCr, m_uEndPosCb, m_uEndPosCr;
    u32  m_uStPosRgb, m_uEndPosRgb;
    u32  m_uOutStPosCb, m_uOutStPosCr, m_uOutEndPosCb, m_uOutEndPosCr;
} POST;

volatile S3C6400_POST_SFR   *v_pPOST_SFR;
volatile UINT8                *v_pPOST_BUF;
volatile S3C6400_DISPLAY_REG  *v_pDISP_SFR;


BOOL POST_WaitforPostDone( void );
void POST_DisableStartBit( void );
int  POST_SetDataFormat( UINT eSrcCSpace, UINT eDstCSpace );
void POST_SetScaler( UINT uSrcWidth, UINT uDstWidth, UINT uSrcHeight, UINT uDstHeight );

void POST_SetAddrRegAndOffsetReg(
    UINT uSrcFullWidth,        UINT uSrcFullHeight,
    UINT uSrcStartX,        UINT uSrcStartY,
    UINT uSrcWidth,            UINT uSrcHeight,
    UINT uSrcFrmSt,
    UINT eSrcCSpace,
    UINT uDstFullWidth,        UINT uDstFullHeight,
    UINT uDstStartX,        UINT uDstStartY,
    UINT uDstWidth,            UINT uDstHeight,
    UINT uDstFrmSt,
    UINT eDstCSpace
    );
void POST_Start();
void PostProcess(UINT uSrcFrmSt,     UINT uDstFrmSt,
                   UINT srcFrmFormat,  UINT dstFrmFormat,
                   UINT srcFrmWidth,   UINT srcFrmHeight,
                   UINT dstFrmWidth,   UINT dstFrmHeight,
                   UINT srcXOffset,    UINT srcYOffset,
                   UINT dstXOffset,    UINT dstYOffset,
                   UINT srcCropWidth,  UINT srcCropHeight,
                   UINT dstCropWidth,  UINT dstCropHeight
                   );





#endif

