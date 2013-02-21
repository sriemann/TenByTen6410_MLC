//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s3c6410_tv_scaler.c

Abstract:       Implementation of TV Scaler Control Library
                This module implements Low Level HW control 

Functions:


Notes:


--*/

#include <windows.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "s3c6410_tv_scaler.h"
#include "s3c6410_tv_scaler_macro.h"

#define TVSC_MSG(x)
#define TVSC_INF(x)
#define TVSC_ERR(x)    RETAILMSG(TRUE, x)

static volatile S3C6410_TVSC_REG *g_pTVSCReg = NULL;
static tTVScalerConfig g_TVSCConfig;

TVSC_ERROR TVSC_initialize_register_address(void *pTVSCReg)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_initialize_register_address(0x%08x)\n\r"), pTVSCReg));

    if (pTVSCReg == NULL)
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_initialize_register_address() : NULL pointer parameter\n\r")));
        error = TVSC_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pTVSCReg = (S3C6410_TVSC_REG *)pTVSCReg;
        TVSC_INF((_T("[TVSC:INF] g_pTVSCReg = 0x%08x\n\r"), g_pTVSCReg));
    }

    memset((void *)(&g_TVSCConfig), 0x0, sizeof(tTVScalerConfig));

    TVSC_MSG((_T("[TVSC]--TVSC_initialize_register_address() : %d\n\r"), error));

    return error;
}

TVSC_ERROR TVSC_initialize(TVSC_OP_MODE Mode, TVSC_SCAN_MODE Scan,
                    TVSC_SRC_TYPE SrcType, unsigned int SrcBaseWidth, unsigned int SrcBaseHeight,
                    unsigned int SrcWidth, unsigned int SrcHeight, unsigned int SrcOffsetX, unsigned int SrcOffsetY,
                    TVSC_DST_TYPE DstType, unsigned int DstBaseWidth, unsigned int DstBaseHeight,
                    unsigned int DstWidth, unsigned int DstHeight, unsigned int DstOffsetX, unsigned int DstOffsetY)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_initialize(%d, %d, %d, [%d, %d, %d, %d, %d, %d], %d, [%d, %d, %d, %d, %d, %d])\n\r"),
                Mode, Scan, SrcType, SrcBaseWidth, SrcBaseHeight, SrcWidth, SrcHeight, SrcOffsetX, SrcOffsetY,
                DstType, DstBaseWidth, DstBaseHeight, DstWidth, DstHeight, DstOffsetX, DstOffsetY));

    error = TVSC_set_mode(Mode, Scan, SrcType, DstType);
    if (error == TVSC_SUCCESS)
    {
        TVSC_set_source_size(SrcBaseWidth, SrcBaseHeight, SrcWidth, SrcHeight, SrcOffsetX, SrcOffsetY);
        TVSC_set_destination_size(DstBaseWidth, DstBaseHeight, DstWidth, DstHeight, DstOffsetX, DstOffsetY);
        error = TVSC_update_condition();
    }

    TVSC_MSG((_T("[TVSC]--TVSC_initialize() : %d\n\r"), error));

    return error;
}

TVSC_ERROR TVSC_set_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_set_source_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    if (g_TVSCConfig.SrcType == TVSC_SRC_FIFO)
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_source_buffer() : FIFO Mode does Not use DMA\n\r")));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        error = TVSC_set_dma_address(TVSC_SRC_ADDRESS, AddrY, AddrCb, AddrCr);
    }

    TVSC_MSG((_T("[TVSC]--TVSC_set_source_buffer() : %d\n\r"), error));

    return error;
}

TVSC_ERROR TVSC_set_next_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_set_next_source_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    if (g_TVSCConfig.SrcType == TVSC_SRC_FIFO)
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_next_source_buffer() : FIFO Mode does Not use DMA\n\r")));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        error = TVSC_set_dma_address(TVSC_NEXT_SRC_ADDRESS, AddrY, AddrCb, AddrCr);
    }

    TVSC_MSG((_T("[TVSC]--TVSC_set_next_source_buffer() : %d\n\r"), error));

    return error;
}

TVSC_ERROR TVSC_set_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_set_destination_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    if (g_TVSCConfig.DstType == TVSC_DST_FIFO_RGB888 || g_TVSCConfig.DstType == TVSC_DST_FIFO_YUV444)
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_destination_buffer() : FIFO Mode does Not use DMA\n\r")));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        error = TVSC_set_dma_address(TVSC_DST_ADDRESS, AddrY, AddrCb, AddrCr);
    }

    TVSC_MSG((_T("[TVSC]--TVSC_set_destination_buffer() : %d\n\r"), error));

    return error;
}

TVSC_ERROR TVSC_set_next_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_set_next_destination_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    if (g_TVSCConfig.DstType == TVSC_DST_FIFO_RGB888 || g_TVSCConfig.DstType == TVSC_DST_FIFO_YUV444)
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_next_destination_buffer() : FIFO Mode does Not use DMA\n\r")));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        error = TVSC_set_dma_address(TVSC_NEXT_DST_ADDRESS, AddrY, AddrCb, AddrCr);
    }

    TVSC_MSG((_T("[TVSC]--TVSC_set_next_destination_buffer() : %d\n\r"), error));

    return error;
}

void TVSC_processing_start(void)
{
    TVSC_MSG((_T("[TVSC]++TVSC_processing_start()\n\r")));

    if (g_TVSCConfig.Mode == TVSC_FREE_RUN_MODE)    // for FIFO output mode
    {
        g_pTVSCReg->MODE |= AUTOLOAD_ENABLE;
    }

    g_pTVSCReg->POSTENVID |= TVSC_ENVID;

    TVSC_MSG((_T("[TVSC]--TVSC_processing_start()\n\r")));
}

TVSC_ERROR TVSC_processing_stop(void)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_processing_stop()\n\r")));

    if (g_pTVSCReg->MODE & AUTOLOAD_ENABLE)    // for FIFO output mode
    {
        // FIFO mode should be stopped by autoload disable
        g_pTVSCReg->MODE &= ~AUTOLOAD_ENABLE;
    }

    g_pTVSCReg->POSTENVID &= ~TVSC_ENVID;

    TVSC_MSG((_T("[TVSC]--TVSC_processing_stop() : %d\n\r"), error));

    return error;
}

void TVSC_autoload_disable(void)
{
    TVSC_MSG((_T("[TVSC]++TVSC_autoload_disable()\n\r")));

    g_pTVSCReg->MODE &= ~AUTOLOAD_ENABLE;

    TVSC_MSG((_T("[TVSC]--TVSC_autoload_disable()\n\r")));
}

TVSC_STATE TVSC_get_processing_state(void)
{
    TVSC_STATE state;

    TVSC_MSG((_T("[TVSC]++TVSC_get_processing_state()\n\r")));

    if (g_pTVSCReg->POSTENVID & TVSC_ENVID)
    {
        state = TVSC_BUSY;
    }
    else
    {
        state = TVSC_IDLE;
    }

    TVSC_MSG((_T("[TVSC]--TVSC_get_processing_state() = %d\n\r"), state));

    return state;
}

void TVSC_enable_interrupt(void)
{
    TVSC_MSG((_T("[TVSC]++TVSC_enable_interrupt()\n\r")));

    g_TVSCConfig.bIntEnable = TRUE;

    g_pTVSCReg->MODE &= ~TVSCINT_PEND;    // Pending Clear
    g_pTVSCReg->MODE |= TVSCINT_ENABLE;    // Interrupt Enable

    TVSC_MSG((_T("[TVSC]--TVSC_enable_interrupt()\n\r")));
}

void TVSC_disable_interrupt(void)
{
    TVSC_MSG((_T("[TVSC]++TVSC_disable_interrupt()\n\r")));

    g_TVSCConfig.bIntEnable = FALSE;

    g_pTVSCReg->MODE &= ~TVSCINT_ENABLE;    // Interrupt Disable
    g_pTVSCReg->MODE &= ~TVSCINT_PEND;    // Pending Clear

    TVSC_MSG((_T("[TVSC]--TVSC_disable_interrupt()\n\r")));
}

BOOL TVSC_clear_interrupt_pending(void)
{
    BOOL IntPend = FALSE;

    TVSC_MSG((_T("[TVSC]++TVSC_clear_interrupt_pending()\n\r")));

    if (g_pTVSCReg->MODE & TVSCINT_PEND)
    {
        g_pTVSCReg->MODE &= ~TVSCINT_PEND;    // Pending Clear
        IntPend = TRUE;
    }

    TVSC_MSG((_T("[TVSC]--TVSC_clear_interrupt_pending()\n\r")));

    return IntPend;
}

static TVSC_ERROR TVSC_set_mode(TVSC_OP_MODE Mode, TVSC_SCAN_MODE Scan, TVSC_SRC_TYPE SrcType, TVSC_DST_TYPE DstType)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    TVSC_MSG((_T("[TVSC]++TVSC_set_mode(%d, %d, %d, %d)\n\r"), Mode, Scan, SrcType, DstType));

    g_TVSCConfig.Mode = Mode;
    g_TVSCConfig.Scan = Scan;
    g_TVSCConfig.SrcType = SrcType;
    g_TVSCConfig.DstType = DstType;

    // TODO: May be need to optimization for CLK : CLKVAL_F(0) | CLKDIR_DIRECT -> CLKVAL_F(1) | CLKDIR_DIVIDED
    g_pTVSCReg->MODE = CLKVALUP_ALWAYS | CLKVAL_F(0) | CLKDIR_DIRECT | CLKSEL_F_HCLK | CSC_R2Y_WIDE | CSC_Y2R_WIDE | IRQ_LEVEL;
    //g_pTVSCReg->MODE = CLKVALUP_ALWAYS | CLKVAL_F(2) | CLKDIR_DIVIDED | CLKSEL_F_HCLK | CSC_R2Y_NARROW | CSC_Y2R_NARROW | IRQ_LEVEL;    // Clock = HCLK/2

    if (g_TVSCConfig.bIntEnable)
    {
        g_pTVSCReg->MODE |= TVSCINT_ENABLE;
    }

    if (Mode == TVSC_PER_FRAME_MODE)
    {
        g_pTVSCReg->MODE |= AUTOLOAD_DISABLE;
    }
    else if (Mode == TVSC_FREE_RUN_MODE)
    {
        g_pTVSCReg->MODE |= AUTOLOAD_ENABLE;
    }
    else
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_mode() : Unknown Operation Mode %d)\n\r"), Mode));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
    }

    if (Scan == TVSC_PROGRESSIVE)
    {
        g_pTVSCReg->MODE |= PROGRESSIVE;
    }
    else if (Mode == TVSC_INTERLACE)
    {
        g_pTVSCReg->MODE |= INTERLACE;
    }
    else
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_mode() : Unknown Scan Mode %d)\n\r"), Scan));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
    }

    switch(SrcType)
    {
    case TVSC_SRC_RGB16:
        g_pTVSCReg->MODE |= SRCFMT_RGB | SRCRGB_RGB565 | SRCYUV_YUV422 | SRC_INTERLEAVE;
        break;
    case TVSC_SRC_RGB24:
        g_pTVSCReg->MODE |= SRCFMT_RGB | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE;
        break;
    case TVSC_SRC_YUV420:
        g_pTVSCReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV420 | SRC_NOT_INTERLEAVE;
        break;
    case TVSC_SRC_YUV422_YCBYCR:
        g_pTVSCReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_YCBYCR;
        break;
    case TVSC_SRC_YUV422_CBYCRY:
        g_pTVSCReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_CBYCRY;
        break;
    case TVSC_SRC_YUV422_YCRYCB:
        g_pTVSCReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_YCRYCB;
        break;
    case TVSC_SRC_YUV422_CRYCBY:
        g_pTVSCReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_CRYCBY;
        break;
    case TVSC_SRC_FIFO:
        g_pTVSCReg->MODE |= FIFO_IN_ENABLE;
        break;
    default:
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_mode() : Unknown Source Type %d)\n\r"), SrcType));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    switch(DstType)
    {
    case TVSC_DST_RGB16:
        g_pTVSCReg->MODE |= OUTFMT_RGB | OUTRGB_RGB565;
        break;
    case TVSC_DST_RGB24:
        g_pTVSCReg->MODE |= OUTFMT_RGB | OUTRGB_RGB24;
        break;
    case TVSC_DST_YUV420:
        g_pTVSCReg->MODE |= OUTFMT_YUV | OUTYUV_YUV420;
        break;
    case TVSC_DST_YUV422_YCBYCR:
        g_pTVSCReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_YCBYCR;
        break;
    case TVSC_DST_YUV422_CBYCRY:
        g_pTVSCReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_CBYCRY;
        break;
    case TVSC_DST_YUV422_YCRYCB:
        g_pTVSCReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_YCRYCB;
        break;
    case TVSC_DST_YUV422_CRYCBY:
        g_pTVSCReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_CRYCBY;
        break;
    case TVSC_DST_FIFO_YUV444:
        g_pTVSCReg->MODE |= OUTFMT_YUV | FIFO_OUT_ENABLE;
        break;
    case TVSC_DST_FIFO_RGB888:
        g_pTVSCReg->MODE |= OUTFMT_RGB | FIFO_OUT_ENABLE;
        break;
    default:
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_mode() : Unknown Destination Type %d)\n\r"), DstType));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    TVSC_MSG((_T("[TVSC]--TVSC_set_mode() : %d\n\r"), error));

    return error;
}

static void TVSC_set_source_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY)
{
    // To use TV Scaler, Image Width is limited to WORD(4Byte) boundary, 32Bpp->1Pixel, 16Bpp->2Pixel
    // This must be considered from Caller Application    
    TVSC_MSG((_T("[TVSC]++TVSC_set_source_size(%d, %d, %d, %d, %d, %d)\n\r"), BaseWidth, BaseHeight, Width, Height, OffsetX, OffsetY));

    g_TVSCConfig.SrcBaseWidth = BaseWidth;
    g_TVSCConfig.SrcBaseHeight = BaseHeight;
    g_TVSCConfig.SrcWidth = Width;
    g_TVSCConfig.SrcHeight = Height;
    g_TVSCConfig.SrcOffsetX = OffsetX;
    g_TVSCConfig.SrcOffsetY = OffsetY;

    TVSC_MSG((_T("[TVSC]--TVSC_set_source_size() : %d\n\r"), error));
}

static void TVSC_set_destination_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY)
{
    // To use TV Scaler, Image Width is limited to WORD(4Byte) boundary, 32Bpp->1Pixel, 16Bpp->2Pixel
    // This must be considered from Caller Application    
    TVSC_MSG((_T("[TVSC]++TVSC_set_destination_size(%d, %d, %d, %d, %d, %d)\n\r"), BaseWidth, BaseHeight, Width, Height, OffsetX, OffsetY));

    g_TVSCConfig.DstBaseWidth = BaseWidth;
    g_TVSCConfig.DstBaseHeight = BaseHeight;
    g_TVSCConfig.DstWidth = Width;
    g_TVSCConfig.DstHeight = Height;
    g_TVSCConfig.DstOffsetX = OffsetX;
    g_TVSCConfig.DstOffsetY = OffsetY;

    TVSC_MSG((_T("[TVSC]--TVSC_set_destination_size() : %d\n\r"), error));
}

static TVSC_ERROR TVSC_get_prescaler_shiftvalue(unsigned int *MainShift, unsigned int SrcValue, unsigned int DstValue)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    if (SrcValue >= 64*DstValue)
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_get_prescaler_shiftvalue() : Out of Range\r\n")));
        error = TVSC_ERROR_PRESCALER_OUT_OF_SCALE_RANGE;
    }
    else if (SrcValue >= 32*DstValue)
    {
        *MainShift = 5;
    }
    else if (SrcValue >= 16*DstValue)
    {
        *MainShift = 4;
    }
    else if (SrcValue >= 8*DstValue)
    {
        *MainShift = 3;
    }
    else if (SrcValue >= 4*DstValue)
    {
        *MainShift = 2;
    }
    else if (SrcValue >= 2*DstValue)
    {
        *MainShift = 1;
    }
    else
    {
        *MainShift = 0;
    }

    return error;
}

static TVSC_ERROR TVSC_update_condition(void)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    unsigned int PreHozRatio, PreVerRatio;
    unsigned int MainHozShift, MainVerShift;

    TVSC_MSG((_T("[TVSC]++TVSC_update_condition()\n\r")));

    error = TVSC_get_prescaler_shiftvalue(&MainHozShift, g_TVSCConfig.SrcWidth, g_TVSCConfig.DstWidth);
    if (error == TVSC_SUCCESS)
    {
        PreHozRatio = (1<<MainHozShift);
        error = TVSC_get_prescaler_shiftvalue(&MainVerShift, g_TVSCConfig.SrcHeight, g_TVSCConfig.DstHeight);
        if (error == TVSC_SUCCESS)
        {
            PreVerRatio = (1<<MainVerShift);
            g_pTVSCReg->PreScale_Ratio = PRESCALE_V_RATIO(PreVerRatio) | PRESCALE_H_RATIO(PreHozRatio);
            g_pTVSCReg->PreScaleImgSize = PRESCALE_WIDTH(g_TVSCConfig.SrcWidth/PreHozRatio) | PRESCALE_HEIGHT(g_TVSCConfig.SrcHeight/PreVerRatio);
            g_pTVSCReg->SRCImgSize = SRC_WIDTH(g_TVSCConfig.SrcWidth) | SRC_HEIGHT(g_TVSCConfig.SrcHeight);
            g_pTVSCReg->MainScale_H_Ratio = MAINSCALE_H_RATIO((g_TVSCConfig.SrcWidth<<8)/(g_TVSCConfig.DstWidth<<MainHozShift));
            g_pTVSCReg->MainScale_V_Ratio = MAINSCALE_V_RATIO((g_TVSCConfig.SrcHeight<<8)/(g_TVSCConfig.DstHeight<<MainVerShift));
            g_pTVSCReg->DSTImgSize = DST_WIDTH(g_TVSCConfig.DstWidth) | DST_HEIGHT(g_TVSCConfig.DstHeight);
            g_pTVSCReg->PreScale_SHFactor = PRESCALE_SHFACTOR(10-(MainHozShift+MainVerShift));
        }
    }

    TVSC_MSG((_T("[TVSC]--TVSC_update_condition() : %d\n\r"), error));

    return error;
}

static TVSC_ERROR TVSC_set_dma_address(TVSC_DMA_ADDRESS DMA, unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    TVSC_ERROR error = TVSC_SUCCESS;

    unsigned int AddrStart_Y=0, AddrStart_Cb=0, AddrStart_Cr=0;
    unsigned int AddrEnd_Y=0, AddrEnd_Cb=0, AddrEnd_Cr=0;
    unsigned int Offset_Y=0, Offset_Cb=0, Offset_Cr=0;

    TVSC_MSG((_T("[TVSC]++TVSC_set_dma_address(%d, 0x%08x, 0x%08x, 0x%08x)\n\r"), DMA, AddrY, AddrCb, AddrCr));

    if (DMA == TVSC_SRC_ADDRESS || DMA == TVSC_NEXT_SRC_ADDRESS)
    {
        switch(g_TVSCConfig.SrcType)
        {
        case TVSC_SRC_RGB16:
        case TVSC_SRC_YUV422_YCBYCR:
        case TVSC_SRC_YUV422_CBYCRY:
        case TVSC_SRC_YUV422_YCRYCB:
        case TVSC_SRC_YUV422_CRYCBY:
            Offset_Y = (g_TVSCConfig.SrcBaseWidth - g_TVSCConfig.SrcWidth)*2;
            AddrStart_Y = AddrY+(g_TVSCConfig.SrcBaseWidth*g_TVSCConfig.SrcOffsetY)*2 + g_TVSCConfig.SrcOffsetX*2;
            AddrEnd_Y = AddrStart_Y+(g_TVSCConfig.SrcWidth*g_TVSCConfig.SrcHeight)*2 + (g_TVSCConfig.SrcHeight-1)*Offset_Y;
            break;
        case TVSC_SRC_RGB24:
            Offset_Y = (g_TVSCConfig.SrcBaseWidth - g_TVSCConfig.SrcWidth)*4;
            AddrStart_Y = AddrY+(g_TVSCConfig.SrcBaseWidth*g_TVSCConfig.SrcOffsetY)*4 + g_TVSCConfig.SrcOffsetX*4;
            AddrEnd_Y = AddrStart_Y+(g_TVSCConfig.SrcWidth*g_TVSCConfig.SrcHeight)*4 + (g_TVSCConfig.SrcHeight-1)*Offset_Y;
            break;
        case TVSC_SRC_YUV420:
            Offset_Y = (g_TVSCConfig.SrcBaseWidth - g_TVSCConfig.SrcWidth);
            Offset_Cb = Offset_Y/2;
            Offset_Cr = Offset_Y/2;
            AddrStart_Y = AddrY+(g_TVSCConfig.SrcBaseWidth*g_TVSCConfig.SrcOffsetY) + g_TVSCConfig.SrcOffsetX;
            AddrEnd_Y = AddrStart_Y+(g_TVSCConfig.SrcWidth*g_TVSCConfig.SrcHeight) + (g_TVSCConfig.SrcHeight-1)*Offset_Y;
            AddrStart_Cb = AddrCb+(g_TVSCConfig.SrcBaseWidth/2)*(g_TVSCConfig.SrcOffsetY/2) + g_TVSCConfig.SrcOffsetX/2;
            AddrEnd_Cb = AddrStart_Cb+(g_TVSCConfig.SrcWidth/2)*(g_TVSCConfig.SrcHeight/2) + (g_TVSCConfig.SrcHeight/2-1)*Offset_Cb;
            AddrStart_Cr = AddrCr+(g_TVSCConfig.SrcBaseWidth/2)*(g_TVSCConfig.SrcOffsetY/2) + g_TVSCConfig.SrcOffsetX/2;
            AddrEnd_Cr = AddrStart_Cr+(g_TVSCConfig.SrcWidth/2)*(g_TVSCConfig.SrcHeight/2) + (g_TVSCConfig.SrcHeight/2-1)*Offset_Cr;
            break;
        default:
            TVSC_ERR((_T("[TVSC:ERR] TVSC_set_dma_address() : Unknown Format %d\r\n"), g_TVSCConfig.SrcType));
            error = TVSC_ERROR_ILLEGAL_PARAMETER;
        }
    }
    else if (DMA == TVSC_DST_ADDRESS || DMA == TVSC_NEXT_DST_ADDRESS)
    {
        switch(g_TVSCConfig.DstType)
        {
        case TVSC_DST_RGB16:
        case TVSC_DST_YUV422_YCBYCR:
        case TVSC_DST_YUV422_CBYCRY:
        case TVSC_DST_YUV422_YCRYCB:
        case TVSC_DST_YUV422_CRYCBY:
            Offset_Y = (g_TVSCConfig.DstBaseWidth - g_TVSCConfig.DstWidth)*2;
            AddrStart_Y = AddrY+(g_TVSCConfig.DstBaseWidth*g_TVSCConfig.DstOffsetY)*2 + g_TVSCConfig.DstOffsetX*2;
            AddrEnd_Y = AddrStart_Y+(g_TVSCConfig.DstWidth*g_TVSCConfig.DstHeight)*2 + (g_TVSCConfig.DstHeight-1)*Offset_Y;
            break;
        case TVSC_DST_RGB24:
            Offset_Y = (g_TVSCConfig.DstBaseWidth - g_TVSCConfig.DstWidth)*4;
            AddrStart_Y = AddrY+(g_TVSCConfig.DstBaseWidth*g_TVSCConfig.DstOffsetY)*4 + g_TVSCConfig.DstOffsetX*4;
            AddrEnd_Y = AddrStart_Y+(g_TVSCConfig.DstWidth*g_TVSCConfig.DstHeight)*4 + (g_TVSCConfig.DstHeight-1)*Offset_Y;
            break;
        case TVSC_DST_YUV420:
            Offset_Y = (g_TVSCConfig.DstBaseWidth - g_TVSCConfig.DstWidth);
            Offset_Cb = Offset_Y/2;
            Offset_Cr = Offset_Y/2;
            AddrStart_Y = AddrY+(g_TVSCConfig.DstBaseWidth*g_TVSCConfig.DstOffsetY) + g_TVSCConfig.DstOffsetX;
            AddrEnd_Y = AddrStart_Y+(g_TVSCConfig.DstWidth*g_TVSCConfig.DstHeight) + (g_TVSCConfig.DstHeight-1)*Offset_Y;
            AddrStart_Cb = AddrCb+(g_TVSCConfig.DstBaseWidth/2)*(g_TVSCConfig.DstOffsetY/2) + g_TVSCConfig.DstOffsetX/2;
            AddrEnd_Cb = AddrStart_Cb+(g_TVSCConfig.DstWidth/2)*(g_TVSCConfig.DstHeight/2) + (g_TVSCConfig.DstHeight/2-1)*Offset_Cb;
            AddrStart_Cr = AddrCr+(g_TVSCConfig.DstBaseWidth/2)*(g_TVSCConfig.DstOffsetY/2) + g_TVSCConfig.DstOffsetX/2;
            AddrEnd_Cr = AddrStart_Cr+(g_TVSCConfig.DstWidth/2)*(g_TVSCConfig.DstHeight/2) + (g_TVSCConfig.DstHeight/2-1)*Offset_Cr;
            break;
        default:
            TVSC_ERR((_T("[TVSC:ERR] TVSC_set_dma_address() : Unknown Format %d\r\n"), g_TVSCConfig.DstType));
            error = TVSC_ERROR_ILLEGAL_PARAMETER;
        }
    }
    else
    {
        TVSC_ERR((_T("[TVSC:ERR] TVSC_set_dma_address() : Unknown DMA address %d\r\n"), DMA));
        error = TVSC_ERROR_ILLEGAL_PARAMETER;
    }

    switch(DMA)
    {
    case TVSC_SRC_ADDRESS:
        g_pTVSCReg->ADDRStart_Y = AddrStart_Y;
        g_pTVSCReg->ADDREnd_Y = AddrEnd_Y;
        g_pTVSCReg->ADDRStart_Cb = AddrStart_Cb;
        g_pTVSCReg->ADDREnd_Cb = AddrEnd_Cb;
        g_pTVSCReg->ADDRStart_Cr = AddrStart_Cr;
        g_pTVSCReg->ADDREnd_Cr = AddrEnd_Cr;
        g_pTVSCReg->Offset_Y= Offset_Y;
        g_pTVSCReg->Offset_Cb= Offset_Cb;
        g_pTVSCReg->Offset_Cr= Offset_Cr;
        break;
    case TVSC_NEXT_SRC_ADDRESS:
        g_pTVSCReg->MODE_2 |= ADDR_CHANGE_DISABLE;    // for Safe
        g_pTVSCReg->NxtADDRStart_Y = AddrStart_Y;
        g_pTVSCReg->NxtADDREnd_Y = AddrEnd_Y;
        g_pTVSCReg->NxtADDRStart_Cb = AddrStart_Cb;
        g_pTVSCReg->NxtADDREnd_Cb = AddrEnd_Cb;
        g_pTVSCReg->NxtADDRStart_Cr = AddrStart_Cr;
        g_pTVSCReg->NxtADDREnd_Cr = AddrEnd_Cr;
        g_pTVSCReg->MODE_2 &= ~ADDR_CHANGE_DISABLE;    // for Safe
        break;
    case TVSC_DST_ADDRESS:
        g_pTVSCReg->ADDRStart_RGB = AddrStart_Y;
        g_pTVSCReg->ADDREnd_RGB = AddrEnd_Y;
        g_pTVSCReg->ADDRStart_oCb = AddrStart_Cb;
        g_pTVSCReg->ADDREnd_oCb = AddrEnd_Cb;
        g_pTVSCReg->ADDRStart_oCr = AddrStart_Cr;
        g_pTVSCReg->ADDREnd_oCr = AddrEnd_Cr;
        g_pTVSCReg->Offset_RGB= Offset_Y;
        g_pTVSCReg->Offset_oCb= Offset_Cb;
        g_pTVSCReg->Offset_oCr= Offset_Cr;
        break;
    case TVSC_NEXT_DST_ADDRESS:
        g_pTVSCReg->MODE_2 |= ADDR_CHANGE_DISABLE;    // for Safe
        g_pTVSCReg->NxtADDRStart_RGB = AddrStart_Y;
        g_pTVSCReg->NxtADDREnd_RGB = AddrEnd_Y;
        g_pTVSCReg->NxtADDRStart_oCb = AddrStart_Cb;
        g_pTVSCReg->NxtADDREnd_oCb = AddrEnd_Cb;
        g_pTVSCReg->NxtADDRStart_oCr = AddrStart_Cr;
        g_pTVSCReg->NxtADDREnd_oCr = AddrEnd_Cr;
        g_pTVSCReg->MODE_2 &= ~ADDR_CHANGE_DISABLE;    // for Safe
        break;
    }

    TVSC_MSG((_T("[TVSC]--TVSC_set_dma_address() : %d\n\r"), error));

    return error;
}

