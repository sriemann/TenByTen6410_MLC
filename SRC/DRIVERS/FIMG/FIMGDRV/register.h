//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

/**
 *     @brief
 *        FIMGSE 3D Hardware Register Defines
 *        FG[GB]_xxxx : Global control/state
 *        FGHI_xxxx : Host Interface
 *        FGVS_xxxx : Vertex Shaders
 *        FGPE_xxxx : Primitive Engine (Clipping, Viewport-mapping, etc)
 *        FGRE_xxxx : Raster Engine (Setup, Rasterizer, etc)
 *        FGPS_xxxx : Pixel Shaders
 *        FGTU_xxxx : Texture Units
 *        FGPF_xxxx : Per-Fragment Units
 */
 
#if !defined(__FIMG_3DREGS_H__)
#define __FIMG_3DREGS_H__

#include "fglconfig.h"
#include <COMMON/buffers.h>

#ifdef __cplusplus
extern "C" {
#endif

#if TARGET_PLATFORM == WIN32_VIP
    #define FIMG_BASE           0x20000000
#elif TARGET_PLATFORM == FPGA_BOARD
    #define FIMG_BASE           ((unsigned int)(gFimgBase.vaddr))
    //#define FIMG_BASE           0x6FD00000
#endif

/*****************************************************************************************
 **     REGISTER NAME          OFFSET            R/W  DESCRIPTION              INITIAL VALUE
 *****************************************************************************************/
#define SFR_OFFSET(in)            ((in) - FIMG_BASE)


#define FGGB_PIPESTATE                (FIMG_BASE+0x0)
#define FGHI_HI_CTRL                (FIMG_BASE+0x8008)
#define FGHI_ATTR0                  (FIMG_BASE+0x8040)
#define FGVS_INSTMEM_SADDR            (FIMG_BASE+0x10000)
#define FGVS_CFLOAT_SADDR             (FIMG_BASE+0x14000)
#define FGVS_CINT_SADDR             (FIMG_BASE+0x18000)
#define FGVS_CBOOL_SADDR             (FIMG_BASE+0x18400)
#define FGVS_CONFIG                    (FIMG_BASE+0x1C800)
#define FGVS_PC_RANGE                (FIMG_BASE+0x20000)
#define FGPE_VTX_CONTEXT            (FIMG_BASE+0x30000)
#define FGRA_PIXEL_SAMPOS            (FIMG_BASE+0x38000)
#define FGRA_LOD_CTRL                (FIMG_BASE+0x3C000)
#define FGRA_POINT_WIDTH            (FIMG_BASE+0x3801C)
#define FGPS_INSTMEM_SADDR            (FIMG_BASE+0x40000)
#define FGPS_CFLOAT_SADDR            (FIMG_BASE+0x44000)
#define FGPS_CINT_SADDR                (FIMG_BASE+0x48000)
#define FGPS_CBOOL_SADDR            (FIMG_BASE+0x48400)
#define FGPS_EXE_MODE                (FIMG_BASE+0x4C800)
#define FGTU_TEX0_CTRL              (FIMG_BASE+0x60000) /* R/W */
#define FGTU_TEX1_CTRL              (FIMG_BASE+0x60050)
#define FGTU_TEX2_CTRL              (FIMG_BASE+0x600A0)
#define FGTU_TEX3_CTRL              (FIMG_BASE+0x600F0)
#define FGTU_TEX4_CTRL              (FIMG_BASE+0x60140)
#define FGTU_TEX5_CTRL              (FIMG_BASE+0x60190)
#define FGTU_TEX6_CTRL              (FIMG_BASE+0x601E0)
#define FGTU_TEX7_CTRL              (FIMG_BASE+0x60230)
#define FGRA_CLIP_XCORD                (FIMG_BASE+0x3C004)
#define FGTU_COLOR_KEY1             (FIMG_BASE+0x60280) /* R/W Color Key1 */
#define FGTU_VTXTEX0_CTRL             (FIMG_BASE+0x602C0)
#define FGPF_SCISSOR_XCORD            (FIMG_BASE+0x70000)
#define FGPF_STENCIL_DEPTH_MASK     (FIMG_BASE+0x70028)
#define FGGB_PIPEMASK                (FIMG_BASE+0x48)
#define FGGB_INTMASK                (FIMG_BASE+0x44)
#define FGGB_INTPENDING                (FIMG_BASE+0x40)
#define FGGB_RST                    (FIMG_BASE+0x8)
#define FGGB_VERSION                (FIMG_BASE+0x10)
#define FGGB_PIPEINTSTATE            (FIMG_BASE+0x50)


/***********************************************************************
    @USAGE
    WRITE
        if you want to write SFR by 32-bits, then
            outw(SFR_ADDRESS, (unsigned int)WDATA);
        if by 16-bits, then
            outs(SFR_ADDRESS, (unsigned short int)WDATA);
        if by 8-bits, then
            outb(SFR_ADDRESS, (unsigned char)WDATA);
    READ
        if you read SFR by 32-bits, then
            (unsigned int)RDATA = inw(SFR_ADDRESS);
        if by 16-bits, then
            (unsigned short int)RDATA = ins(SFR_ADDRESS);
        if by 8-bits, then
            (unsigned char)RDATA = inb(SFR_ADDRESS);
*************************************************************************/

typedef volatile unsigned char     *vbptr;
typedef volatile unsigned short *vsptr;
typedef volatile unsigned int     *vwptr;
typedef volatile float            *vfptr;

#define READREGB(Port)        (*((vbptr) (Port)))
#define READREGPB(Port, Y)    (Y =*((vbptr) (Port)))
#define WRITEREGB(Port, X)    (*((vbptr) (Port)) = (unsigned char) (X))

#define READREGS(Port)        (*((vsptr) (Port)))
#define READREGPS(Port, Y)    (Y =*((vsptr) (Port)))
#define WRITEREGS(Port, X)    (*((vsptr) (Port)) = (unsigned short) (X))

#define READREG(Port)        (*((vwptr) (Port)))
#define READREGP(Port, Y)    (Y =*((vwptr) (Port)))
#define WRITEREG(Port, X)    (*((vwptr) (Port)) = (unsigned int) (X))

#define READREGF(Port)        (*((vfptr) (Port)))
#define READREGPF(Port, Y)    (Y =*((vfptr) (Port)))
#define WRITEREGF(Port, X)    (*((vfptr) (Port)) = (float) (X))

#ifdef __cplusplus
}
#endif


#endif /* __FIMG_3DREGS_H__ */

/*----------------------------------------< End of file >---------------------------------------------*/
