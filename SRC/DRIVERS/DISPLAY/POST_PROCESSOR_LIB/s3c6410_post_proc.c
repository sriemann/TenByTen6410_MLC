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

Module Name:    s3c6410_post_proc.c

Abstract:       Implementation of Post Processor Control Library
                This module implements Low Level HW control 

Functions:


Notes:


--*/

#include <windows.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "s3c6410_post_proc.h"
#include "s3c6410_post_proc_macro.h"

#define POST_MSG(x)    
#define POST_INF(x)    
#define POST_ERR(x)    RETAILMSG(TRUE, x)

#define REDUCE_VCLK_STOP_ON_LOCALPATH

static volatile S3C6410_POST_REG *g_pPostReg = NULL;
static tPostConfig g_PostConfig;

POST_ERROR Post_initialize_register_address(void *pPostReg)
{
    POST_ERROR error = POST_SUCCESS;

    POST_MSG((_T("[POST]++Post_initialize_register_address(0x%08x)\n\r"), pPostReg));

    if (pPostReg == NULL)
    {
        POST_ERR((_T("[POST:ERR] Post_initialize_register_address() : NULL pointer parameter\n\r")));
        error = POST_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pPostReg = (S3C6410_POST_REG *)pPostReg;
        POST_INF((_T("[POST:INF] g_pPostReg = 0x%08x\n\r"), g_pPostReg));
    }

    memset((void *)(&g_PostConfig), 0x0, sizeof(tPostConfig));

    POST_MSG((_T("[POST]--Post_initialize_register_address() : %d\n\r"), error));

    return error;
}

POST_ERROR Post_initialize(POST_OP_MODE Mode, POST_SCAN_MODE Scan,
                    POST_SRC_TYPE SrcType, unsigned int SrcBaseWidth, unsigned int SrcBaseHeight,
                    unsigned int SrcWidth, unsigned int SrcHeight, unsigned int SrcOffsetX, unsigned int SrcOffsetY,
                    POST_DST_TYPE DstType, unsigned int DstBaseWidth, unsigned int DstBaseHeight,
                    unsigned int DstWidth, unsigned int DstHeight, unsigned int DstOffsetX, unsigned int DstOffsetY)
{
    POST_ERROR error = POST_SUCCESS;

    POST_MSG((_T("[POST]++Post_initialize(%d, %d, %d, [%d, %d, %d, %d, %d, %d], %d, [%d, %d, %d, %d, %d, %d])\n\r"),
                Mode, Scan, SrcType, SrcBaseWidth, SrcBaseHeight, SrcWidth, SrcHeight, SrcOffsetX, SrcOffsetY,
                DstType, DstBaseWidth, DstBaseHeight, DstWidth, DstHeight, DstOffsetX, DstOffsetY));

    error = Post_set_mode(Mode, Scan, SrcType, DstType);
    if (error == POST_SUCCESS)
    {
#ifdef REDUCE_VCLK_STOP_ON_LOCALPATH
        // work-around for VCLK STOP when scaling down on Local Path State
        /*====================================================================*/
        if(Mode == POST_FREE_RUN_MODE)
        {
            if(SrcHeight >= 2*DstHeight)
            {
                int i=2;
                for(i=2;(SrcHeight >= i*DstHeight) && (i<8);i++);
                SrcBaseWidth *= i;
                SrcBaseHeight /= i;
                SrcHeight /= i;
                SrcOffsetY /= i;
            }
        }
        /*====================================================================*/
#endif
        Post_set_source_size(SrcBaseWidth, SrcBaseHeight, SrcWidth, SrcHeight, SrcOffsetX, SrcOffsetY);
        Post_set_destination_size(DstBaseWidth, DstBaseHeight, DstWidth, DstHeight, DstOffsetX, DstOffsetY);
        
        error = Post_update_condition();
    }

    POST_MSG((_T("[POST]--Post_initialize() : %d\n\r"), error));

    return error;
}

POST_ERROR Post_set_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    POST_ERROR error = POST_SUCCESS;

    POST_MSG((_T("[POST]++Post_set_source_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    error = Post_set_dma_address(POST_SRC_ADDRESS, AddrY, AddrCb, AddrCr);

    POST_MSG((_T("[POST]--Post_set_source_buffer() : %d\n\r"), error));

    return error;
}

POST_ERROR Post_set_next_source_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    POST_ERROR error = POST_SUCCESS;

    POST_MSG((_T("[POST]++Post_set_next_source_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    error = Post_set_dma_address(POST_NEXT_SRC_ADDRESS, AddrY, AddrCb, AddrCr);

    POST_MSG((_T("[POST]--Post_set_next_source_buffer() : %d\n\r"), error));

    return error;
}

POST_ERROR Post_set_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    POST_ERROR error = POST_SUCCESS;

    POST_MSG((_T("[POST]++Post_set_destination_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    if (g_PostConfig.DstType == POST_DST_FIFO_RGB888 || g_PostConfig.DstType == POST_DST_FIFO_YUV444)
    {
        POST_ERR((_T("[POST:ERR] Post_set_destination_buffer() : FIFO Mode does Not use DMA\n\r")));
        error = POST_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        error = Post_set_dma_address(POST_DST_ADDRESS, AddrY, AddrCb, AddrCr);
    }

    POST_MSG((_T("[POST]--Post_set_destination_buffer() : %d\n\r"), error));

    return error;
}

POST_ERROR Post_set_next_destination_buffer(unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    POST_ERROR error = POST_SUCCESS;

    POST_MSG((_T("[POST]++Post_set_next_destination_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrY, AddrCb, AddrCr));

    if (g_PostConfig.DstType == POST_DST_FIFO_RGB888 || g_PostConfig.DstType == POST_DST_FIFO_YUV444)
    {
        POST_ERR((_T("[POST:ERR] Post_set_next_destination_buffer() : FIFO Mode does Not use DMA\n\r")));
        error = POST_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        error = Post_set_dma_address(POST_NEXT_DST_ADDRESS, AddrY, AddrCb, AddrCr);
    }

    POST_MSG((_T("[POST]--Post_set_next_destination_buffer() : %d\n\r"), error));

    return error;
}

void Post_processing_start(void)
{
    POST_MSG((_T("[POST]++Post_processing_start()\n\r")));

    if (g_PostConfig.Mode == POST_FREE_RUN_MODE)    // for FIFO output mode
    {
        g_pPostReg->MODE |= AUTOLOAD_ENABLE;
    }

    g_pPostReg->POSTENVID |= POST_ENVID;

    POST_MSG((_T("[POST]--Post_processing_start()\n\r")));
}

void Post_processing_stop(void)
{
    POST_MSG((_T("[POST]++Post_processing_stop()\n\r")));

    if (g_pPostReg->MODE & AUTOLOAD_ENABLE)    // for FIFO output mode
    {
        // FIFO mode should be stopped by autoload disable
        g_pPostReg->MODE &= ~AUTOLOAD_ENABLE;
    }

    g_pPostReg->POSTENVID &= ~POST_ENVID;

    POST_MSG((_T("[POST]--Post_processing_stop()\n\r")));
}

void Post_autoload_disable(void)
{
    POST_MSG((_T("[POST]++Post_autoload_disable()\n\r")));

    g_pPostReg->MODE &= ~AUTOLOAD_ENABLE;

    POST_MSG((_T("[POST]--Post_autoload_disable()\n\r")));
}

POST_STATE Post_get_processing_state(void)
{
    POST_STATE state;

    POST_MSG((_T("[POST]++Post_get_processing_state()\n\r")));

    if (g_pPostReg->POSTENVID & POST_ENVID)
    {
        state = POST_BUSY;
    }
    else
    {
        state = POST_IDLE;
    }

    POST_MSG((_T("[POST]--Post_get_processing_state() = %d\n\r"), state));

    return state;
}

void Post_enable_interrupt(void)
{
    POST_MSG((_T("[POST]++Post_enable_interrupt()\n\r")));

    g_PostConfig.bIntEnable = TRUE;

    g_pPostReg->MODE &= ~POSTINT_PEND;    // Pending Clear
    g_pPostReg->MODE |= POSTINT_ENABLE;    // Interrupt Enable

    POST_MSG((_T("[POST]--Post_enable_interrupt()\n\r")));
}

void Post_disable_interrupt(void)
{
    POST_MSG((_T("[POST]++Post_disable_interrupt()\n\r")));

    g_PostConfig.bIntEnable = FALSE;

    g_pPostReg->MODE &= ~POSTINT_ENABLE;    // Interrupt Disable
    g_pPostReg->MODE &= ~POSTINT_PEND;    // Pending Clear

    POST_MSG((_T("[POST]--Post_disable_interrupt()\n\r")));
}

BOOL Post_clear_interrupt_pending(void)
{
    BOOL IntPend = FALSE;

    POST_MSG((_T("[POST]++Post_clear_interrupt_pending()\n\r")));

    if (g_pPostReg->MODE & POSTINT_PEND)
    {
        g_pPostReg->MODE &= ~POSTINT_PEND;    // Pending Clear
        IntPend = TRUE;
    }

    POST_MSG((_T("[POST]--Post_clear_interrupt_pending()\n\r")));

    return IntPend;
}

static POST_ERROR Post_set_mode(POST_OP_MODE Mode, POST_SCAN_MODE Scan, POST_SRC_TYPE SrcType, POST_DST_TYPE DstType)
{
    POST_ERROR error = POST_SUCCESS;

    POST_MSG((_T("[POST]++Post_set_mode(%d, %d, %d, %d)\n\r"), Mode, Scan, SrcType, DstType));

    g_PostConfig.Mode = Mode;
    g_PostConfig.Scan = Scan;
    g_PostConfig.SrcType = SrcType;
    g_PostConfig.DstType = DstType;

    // For some application PostProcessor May be need to faster CLK
    // setting for faster CLK : CLKVALUP_ALWAYS | CLKVAL_F(0) | CLKDIR_DIRECT | CLKSEL_F_HCLK | CSC_R2Y_WIDE | CSC_Y2R_WIDE | IRQ_LEVEL
    g_pPostReg->MODE = CLKVALUP_ALWAYS | CLKVAL_F(2) | CLKDIR_DIVIDED | CLKSEL_F_HCLK | CSC_R2Y_NARROW | CSC_Y2R_NARROW | IRQ_LEVEL;    // Clock = HCLK/2

    if (g_PostConfig.bIntEnable)
    {
        g_pPostReg->MODE |= POSTINT_ENABLE;
    }

    if (Mode == POST_PER_FRAME_MODE)
    {
        g_pPostReg->MODE |= AUTOLOAD_DISABLE;
    }
    else if (Mode == POST_FREE_RUN_MODE)
    {
        g_pPostReg->MODE |= AUTOLOAD_ENABLE;
    }
    else
    {
        POST_ERR((_T("[POST:ERR] Post_set_mode() : Unknown Operation Mode %d)\n\r"), Mode));
        return POST_ERROR_ILLEGAL_PARAMETER;
    }

    if (Scan == POST_PROGRESSIVE)
    {
        g_pPostReg->MODE |= PROGRESSIVE;
    }
    else if (Mode == POST_INTERLACE)
    {
        g_pPostReg->MODE |= INTERLACE;
    }
    else
    {
        POST_ERR((_T("[POST:ERR] Post_set_mode() : Unknown Scan Mode %d)\n\r"), Scan));
        return POST_ERROR_ILLEGAL_PARAMETER;
    }

    switch(SrcType)
    {
    case POST_SRC_RGB16:
        g_pPostReg->MODE |= SRCFMT_RGB | SRCRGB_RGB565 | SRCYUV_YUV422 | SRC_INTERLEAVE;
        break;
    case POST_SRC_RGB24:
        g_pPostReg->MODE |= SRCFMT_RGB | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE;
        break;
    case POST_SRC_YUV420:
        g_pPostReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV420 | SRC_NOT_INTERLEAVE;
        break;
    case POST_SRC_YUV422_YCBYCR:
        g_pPostReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_YCBYCR;
        break;
    case POST_SRC_YUV422_CBYCRY:
        g_pPostReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_CBYCRY;
        break;
    case POST_SRC_YUV422_YCRYCB:
        g_pPostReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_YCRYCB;
        break;
    case POST_SRC_YUV422_CRYCBY:
        g_pPostReg->MODE |= SRCFMT_YUV | SRCRGB_RGB24 | SRCYUV_YUV422 | SRC_INTERLEAVE | SRCYUV_CRYCBY;
        break;
    default:
        POST_ERR((_T("[POST:ERR] Post_set_mode() : Unknown Source Type %d)\n\r"), SrcType));
        return POST_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    switch(DstType)
    {
    case POST_DST_RGB16:
        g_pPostReg->MODE |= OUTFMT_RGB | OUTRGB_RGB565;
        break;
    case POST_DST_RGB24:
        g_pPostReg->MODE |= OUTFMT_RGB | OUTRGB_RGB24;
        break;
    case POST_DST_YUV420:
        g_pPostReg->MODE |= OUTFMT_YUV | OUTYUV_YUV420;
        break;
    case POST_DST_YUV422_YCBYCR:
        g_pPostReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_YCBYCR;
        break;
    case POST_DST_YUV422_CBYCRY:
        g_pPostReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_CBYCRY;
        break;
    case POST_DST_YUV422_YCRYCB:
        g_pPostReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_YCRYCB;
        break;
    case POST_DST_YUV422_CRYCBY:
        g_pPostReg->MODE |= OUTFMT_YUV | OUTYUV_YUV422 | OUTYUV_CRYCBY;
        break;
    case POST_DST_FIFO_YUV444:
        g_pPostReg->MODE |= OUTFMT_YUV | FIFO_OUT_ENABLE;
        break;
    case POST_DST_FIFO_RGB888:
        g_pPostReg->MODE |= OUTFMT_RGB | FIFO_OUT_ENABLE;
        break;
    default:
        POST_ERR((_T("[POST:ERR] Post_set_mode() : Unknown Destination Type %d)\n\r"), DstType));
        return POST_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    POST_MSG((_T("[POST]--Post_set_mode() : %d\n\r"), error));

    return error;
}

static void Post_set_source_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY)
{
    // To use Post Processor, Image Width is limited to WORD(4Byte) boundary, 32Bpp->1Pixel, 16Bpp->2Pixel
    // This must be considered from Caller Application    
    POST_MSG((_T("[POST]++Post_set_source_size(%d, %d, %d, %d, %d, %d)\n\r"), BaseWidth, BaseHeight, Width, Height, OffsetX, OffsetY));

    g_PostConfig.SrcBaseWidth = BaseWidth;
    g_PostConfig.SrcBaseHeight = BaseHeight;
    g_PostConfig.SrcWidth = Width;
    g_PostConfig.SrcHeight = Height;
    g_PostConfig.SrcOffsetX = OffsetX;
    g_PostConfig.SrcOffsetY = OffsetY;

    POST_MSG((_T("[POST]--Post_set_source_size()\n\r")));
}

static void Post_set_destination_size(unsigned int BaseWidth, unsigned int BaseHeight, unsigned int Width, unsigned int Height, unsigned int OffsetX, unsigned int OffsetY)
{
    // To use Post Processor, Image Width is limited to WORD(4Byte) boundary, 32Bpp->1Pixel, 16Bpp->2Pixel
    // This must be considered from Caller Application
    POST_MSG((_T("[POST]++Post_set_destination_size(%d, %d, %d, %d, %d, %d)\n\r"), BaseWidth, BaseHeight, Width, Height, OffsetX, OffsetY));

    g_PostConfig.DstBaseWidth = BaseWidth;
    g_PostConfig.DstBaseHeight = BaseHeight;
    g_PostConfig.DstWidth = Width;
    g_PostConfig.DstHeight = Height;
    g_PostConfig.DstOffsetX = OffsetX;
    g_PostConfig.DstOffsetY = OffsetY;
    
    POST_MSG((_T("[POST]--Post_set_destination_size()\n\r")));
}

static POST_ERROR Post_get_prescaler_shiftvalue(unsigned int *MainShift, unsigned int SrcValue, unsigned int DstValue )
{
    POST_ERROR error = POST_SUCCESS;

    if (SrcValue >= 64*DstValue)
    {
        POST_ERR((_T("[POST:ERR] Post_get_prescaler_shiftvalue() : Out of Range\r\n")));
        error = POST_ERROR_PRESCALER_OUT_OF_SCALE_RANGE;
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

static POST_ERROR Post_update_condition(void)
{
    POST_ERROR error = POST_SUCCESS;

    unsigned int PreHozRatio, PreVerRatio;
    unsigned int MainHozShift, MainVerShift;

    POST_MSG((_T("[POST]++Post_update_condition()\n\r")));

    error = Post_get_prescaler_shiftvalue(&MainHozShift, g_PostConfig.SrcWidth, g_PostConfig.DstWidth);
    if (error == POST_SUCCESS)
    {
        PreHozRatio = (1<<MainHozShift);
        error = Post_get_prescaler_shiftvalue(&MainVerShift, g_PostConfig.SrcHeight, g_PostConfig.DstHeight);
        if (error == POST_SUCCESS)
        {
            PreVerRatio = (1<<MainVerShift);
            g_pPostReg->PreScale_Ratio = PRESCALE_V_RATIO(PreVerRatio) | PRESCALE_H_RATIO(PreHozRatio);
            g_pPostReg->PreScaleImgSize = PRESCALE_WIDTH(g_PostConfig.SrcWidth/PreHozRatio) | PRESCALE_HEIGHT(g_PostConfig.SrcHeight/PreVerRatio);
            g_pPostReg->SRCImgSize = SRC_WIDTH(g_PostConfig.SrcWidth) | SRC_HEIGHT(g_PostConfig.SrcHeight);
            g_pPostReg->MainScale_H_Ratio = MAINSCALE_H_RATIO((g_PostConfig.SrcWidth<<8)/(g_PostConfig.DstWidth<<MainHozShift));
            g_pPostReg->MainScale_V_Ratio = MAINSCALE_V_RATIO((g_PostConfig.SrcHeight<<8)/(g_PostConfig.DstHeight<<MainVerShift));
            g_pPostReg->DSTImgSize = DST_WIDTH(g_PostConfig.DstWidth) | DST_HEIGHT(g_PostConfig.DstHeight);
            g_pPostReg->PreScale_SHFactor = PRESCALE_SHFACTOR(10-(MainHozShift+MainVerShift));
        }
    }

    POST_MSG((_T("[POST]--Post_update_condition() : %d\n\r"), error));

    return error;
}

static POST_ERROR Post_set_dma_address(POST_DMA_ADDRESS DMA, unsigned int AddrY, unsigned int AddrCb, unsigned int AddrCr)
{
    POST_ERROR error = POST_SUCCESS;

    unsigned int AddrStart_Y=0, AddrStart_Cb=0, AddrStart_Cr=0;
    unsigned int AddrEnd_Y=0, AddrEnd_Cb=0, AddrEnd_Cr=0;
    unsigned int Offset_Y=0, Offset_Cb=0, Offset_Cr=0;

    POST_MSG((_T("[POST]++Post_set_dma_address(%d, 0x%08x, 0x%08x, 0x%08x)\n\r"), DMA, AddrY, AddrCb, AddrCr));

    if (DMA == POST_SRC_ADDRESS || DMA == POST_NEXT_SRC_ADDRESS)
    {
        switch(g_PostConfig.SrcType)
        {
        case POST_SRC_RGB16:
        case POST_SRC_YUV422_YCBYCR:
        case POST_SRC_YUV422_CBYCRY:
        case POST_SRC_YUV422_YCRYCB:
        case POST_SRC_YUV422_CRYCBY:
            Offset_Y = (g_PostConfig.SrcBaseWidth - g_PostConfig.SrcWidth)*2;
            AddrStart_Y = AddrY+(g_PostConfig.SrcBaseWidth*g_PostConfig.SrcOffsetY)*2 + g_PostConfig.SrcOffsetX*2;
            AddrEnd_Y = AddrStart_Y+(g_PostConfig.SrcWidth*g_PostConfig.SrcHeight)*2 + (g_PostConfig.SrcHeight-1)*Offset_Y;
            break;
        case POST_SRC_RGB24:
            Offset_Y = (g_PostConfig.SrcBaseWidth - g_PostConfig.SrcWidth)*4;
            AddrStart_Y = AddrY+(g_PostConfig.SrcBaseWidth*g_PostConfig.SrcOffsetY)*4 + g_PostConfig.SrcOffsetX*4;
            AddrEnd_Y = AddrStart_Y+(g_PostConfig.SrcWidth*g_PostConfig.SrcHeight)*4 + (g_PostConfig.SrcHeight-1)*Offset_Y;
            break;
        case POST_SRC_YUV420:
            Offset_Y = (g_PostConfig.SrcBaseWidth - g_PostConfig.SrcWidth);
            Offset_Cb = Offset_Y/2;
            Offset_Cr = Offset_Y/2;
            AddrStart_Y = AddrY+(g_PostConfig.SrcBaseWidth*g_PostConfig.SrcOffsetY) + g_PostConfig.SrcOffsetX;
            AddrEnd_Y = AddrStart_Y+(g_PostConfig.SrcWidth*g_PostConfig.SrcHeight) + (g_PostConfig.SrcHeight-1)*Offset_Y;
            AddrStart_Cb = AddrCb+(g_PostConfig.SrcBaseWidth/2)*(g_PostConfig.SrcOffsetY/2) + g_PostConfig.SrcOffsetX/2;
            AddrEnd_Cb = AddrStart_Cb+(g_PostConfig.SrcWidth/2)*(g_PostConfig.SrcHeight/2) + (g_PostConfig.SrcHeight/2-1)*Offset_Cb;
            AddrStart_Cr = AddrCr+(g_PostConfig.SrcBaseWidth/2)*(g_PostConfig.SrcOffsetY/2) + g_PostConfig.SrcOffsetX/2;
            AddrEnd_Cr = AddrStart_Cr+(g_PostConfig.SrcWidth/2)*(g_PostConfig.SrcHeight/2) + (g_PostConfig.SrcHeight/2-1)*Offset_Cr;
            break;
        default:
            POST_ERR((_T("[POST:ERR] Post_set_dma_address() : Unknown Format %d\r\n"), g_PostConfig.SrcType));
            return POST_ERROR_ILLEGAL_PARAMETER;
        }
    }
    else if (DMA == POST_DST_ADDRESS || DMA == POST_NEXT_DST_ADDRESS)
    {
        switch(g_PostConfig.DstType)
        {
        case POST_DST_RGB16:
        case POST_DST_YUV422_YCBYCR:
        case POST_DST_YUV422_CBYCRY:
        case POST_DST_YUV422_YCRYCB:
        case POST_DST_YUV422_CRYCBY:
            Offset_Y = (g_PostConfig.DstBaseWidth - g_PostConfig.DstWidth)*2;
            AddrStart_Y = AddrY+(g_PostConfig.DstBaseWidth*g_PostConfig.DstOffsetY)*2 + g_PostConfig.DstOffsetX*2;
            AddrEnd_Y = AddrStart_Y+(g_PostConfig.DstWidth*g_PostConfig.DstHeight)*2 + (g_PostConfig.DstHeight-1)*Offset_Y;
            break;
        case POST_DST_RGB24:
            Offset_Y = (g_PostConfig.DstBaseWidth - g_PostConfig.DstWidth)*4;
            AddrStart_Y = AddrY+(g_PostConfig.DstBaseWidth*g_PostConfig.DstOffsetY)*4 + g_PostConfig.DstOffsetX*4;
            AddrEnd_Y = AddrStart_Y+(g_PostConfig.DstWidth*g_PostConfig.DstHeight)*4 + (g_PostConfig.DstHeight-1)*Offset_Y;
            break;
        case POST_DST_YUV420:
            Offset_Y = (g_PostConfig.DstBaseWidth - g_PostConfig.DstWidth);
            Offset_Cb = Offset_Y/2;
            Offset_Cr = Offset_Y/2;
            AddrStart_Y = AddrY+(g_PostConfig.DstBaseWidth*g_PostConfig.DstOffsetY) + g_PostConfig.DstOffsetX;
            AddrEnd_Y = AddrStart_Y+(g_PostConfig.DstWidth*g_PostConfig.DstHeight) + (g_PostConfig.DstHeight-1)*Offset_Y;
            AddrStart_Cb = AddrCb+(g_PostConfig.DstBaseWidth/2)*(g_PostConfig.DstOffsetY/2) + g_PostConfig.DstOffsetX/2;
            AddrEnd_Cb = AddrStart_Cb+(g_PostConfig.DstWidth/2)*(g_PostConfig.DstHeight/2) + (g_PostConfig.DstHeight/2-1)*Offset_Cb;
            AddrStart_Cr = AddrCr+(g_PostConfig.DstBaseWidth/2)*(g_PostConfig.DstOffsetY/2) + g_PostConfig.DstOffsetX/2;
            AddrEnd_Cr = AddrStart_Cr+(g_PostConfig.DstWidth/2)*(g_PostConfig.DstHeight/2) + (g_PostConfig.DstHeight/2-1)*Offset_Cr;
            break;
        default:
            POST_ERR((_T("[POST:ERR] Post_set_dma_address() : Unknown Format %d\r\n"), g_PostConfig.DstType));
            return POST_ERROR_ILLEGAL_PARAMETER;
        }
    }
    else
    {
        POST_ERR((_T("[POST:ERR] Post_set_dma_address() : Unknown DMA address %d\r\n"), DMA));
        return POST_ERROR_ILLEGAL_PARAMETER;
    }

    switch(DMA)
    {
    case POST_SRC_ADDRESS:
        g_pPostReg->ADDRStart_Y = AddrStart_Y;
        g_pPostReg->ADDREnd_Y = AddrEnd_Y;
        g_pPostReg->ADDRStart_Cb = AddrStart_Cb;
        g_pPostReg->ADDREnd_Cb = AddrEnd_Cb;
        g_pPostReg->ADDRStart_Cr = AddrStart_Cr;
        g_pPostReg->ADDREnd_Cr = AddrEnd_Cr;
        g_pPostReg->Offset_Y= Offset_Y;
        g_pPostReg->Offset_Cb= Offset_Cb;
        g_pPostReg->Offset_Cr= Offset_Cr;
        break;
    case POST_NEXT_SRC_ADDRESS:
        g_pPostReg->MODE_2 |= ADDR_CHANGE_DISABLE;    // To prevent address from changing still in register setting
        g_pPostReg->NxtADDRStart_Y = AddrStart_Y;
        g_pPostReg->NxtADDREnd_Y = AddrEnd_Y;
        g_pPostReg->NxtADDRStart_Cb = AddrStart_Cb;
        g_pPostReg->NxtADDREnd_Cb = AddrEnd_Cb;
        g_pPostReg->NxtADDRStart_Cr = AddrStart_Cr;
        g_pPostReg->NxtADDREnd_Cr = AddrEnd_Cr;
        g_pPostReg->MODE_2 &= ~ADDR_CHANGE_DISABLE;    // Now, address can change
        break;
    case POST_DST_ADDRESS:
        g_pPostReg->ADDRStart_RGB = AddrStart_Y;
        g_pPostReg->ADDREnd_RGB = AddrEnd_Y;
        g_pPostReg->ADDRStart_oCb = AddrStart_Cb;
        g_pPostReg->ADDREnd_oCb = AddrEnd_Cb;
        g_pPostReg->ADDRStart_oCr = AddrStart_Cr;
        g_pPostReg->ADDREnd_oCr = AddrEnd_Cr;
        g_pPostReg->Offset_RGB= Offset_Y;
        g_pPostReg->Offset_oCb= Offset_Cb;
        g_pPostReg->Offset_oCr= Offset_Cr;
        break;
    case POST_NEXT_DST_ADDRESS:
        g_pPostReg->MODE_2 |= ADDR_CHANGE_DISABLE;    // To prevent address from changing still in register setting
        g_pPostReg->NxtADDRStart_RGB = AddrStart_Y;
        g_pPostReg->NxtADDREnd_RGB = AddrEnd_Y;
        g_pPostReg->NxtADDRStart_oCb = AddrStart_Cb;
        g_pPostReg->NxtADDREnd_oCb = AddrEnd_Cb;
        g_pPostReg->NxtADDRStart_oCr = AddrStart_Cr;
        g_pPostReg->NxtADDREnd_oCr = AddrEnd_Cr;
        g_pPostReg->MODE_2 &= ~ADDR_CHANGE_DISABLE;    // Now, address can change
        break;
    default:
        return POST_ERROR_ILLEGAL_PARAMETER;
    }

    POST_MSG((_T("[POST]--Post_set_dma_address() : %d\n\r"), error));

    return error;
}

