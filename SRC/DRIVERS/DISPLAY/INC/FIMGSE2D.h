//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    fimgse2d.h

Abstract:       defines for FIMGSE-2D Graphics Accelerator
                Header to define the FIMGSE-2D class.
Functions:

Notes:          This version is made for FIMGSE-2D v2.0

--*/

#ifndef __FIMGSE2D_H__
#define __FIMGSE2D_H__

#include "regctrl_g2d.h"

/**
*    Define G2D Command processing and return type
*
**/
typedef enum
{
    G2D_FASTRETURN, //< Currently, this mode has some bugs, with GDI call
    G2D_INTERRUPT,
    G2D_BUSYWAITING
} G2D_CMDPROCESSING_TYPE;

/**
*    Hardware Limitation Macro
*
**/
/// For Coordinate Register
#define G2D_MAX_WIDTH           (1<<11)        //< 2048
#define G2D_MAX_HEIGHT          (1<<11)        //< 2048

#define MAX_2DHW_WIDTH  (2040)
#define MAX_2DHW_HEIGHT (2040)
#define MAX_2DHW_XCOORD (2040)
#define MAX_2DHW_YCOORD (2040)

// ROP_REG (0x410)
#define G2D_TRANSPARENT_BIT     (1<<9)
#define G2D_OPAQUE_BIT          (0<<9)

// Color_Mode_Reg (0x510[2:0])
#define G2D_COLOR_RGB_565       (0) 
#define G2D_COLOR_RGBA_5551     (1)
#define G2D_COLOR_ARGB_1555     (2)
#define G2D_COLOR_RGBA_8888     (3)
#define G2D_COLOR_ARGB_8888     (4)
#define G2D_COLOR_XRGB_8888     (5)
#define G2D_COLOR_RGBX_8888     (6)
#define G2D_COLOR_UNUSED        (7)

#define MS_NUM_SUPPORT_COLORMODE    (10)        //EGPEFormat


// CMD0_REG (Line) (0x100)
#define G2D_REND_POINT_BIT              (1<<0)
#define G2D_REND_LINE_BIT               (1<<1)
#define G2D_MAJOR_COORD_X_BIT           (1<<8)
#define G2D_MAJOR_COORD_Y_BIT           (0<<8)
#define G2D_NOT_DRAW_LAST_POINT_BIT     (1<<9)
#define G2D_DRAW_LAST_POINT_BIT         ~(1<<9)

// CMD1_REG (BitBlt) (0x104)
#define G2D_STRETCH_BITBLT_BIT          (1<<1)
#define G2D_NORMAL_BITBLT_BIT           (1<<0)

#define ABS(v)                          (((v)>=0) ? (v):(-(v)))
#define START_ASCII                     (0x20)
#define OPAQUE_ENABLE                   (0<<9)

// Set fading and alpha value
#define FADING_OFFSET_DISABLE           (0x0<<8)
#define ALPHA_VALUE_DISABLE             (0xff<<0)



#define HOST2SCREEN         (0)
#define SCREEN2SCREEN       (1)

// G2D Source           0xf0
// G2D Dest             0xcc
// G2D Pattern          0xaa
// MS ROP Pattern       0xf0
// MS ROP Source        0xcc
// MS ROP Dest          0xaa
//                       G2D     MS
// SRC_ONLY             0xf0    0xcc        // SRCCOPY : S
// DST_ONLY             0xcc    0xaa        // DSTCOPY : D
// PAT_ONLY             0xaa    0xf0        // PATCOPY : P
// SRC_OR_DST           0xfc    0xee        // SRCPAINT : S | D
// SRC_OR_PAT           0xfa    0xfc        // P | S --> 0xF0008A
// DST_OR_PAT           0xee    0xfa        // R2_MERGEPEN : P | D
// SRC_AND_DST          0xc0    0x88        // SRCAND : S & D
// SRC_AND_PAT          0xa0    0xc0        // MERGECOPY : S & P
// DST_AND_PAT          0x88    0xa0        // R2_MASKPEN : P & D
// SRC_XOR_DST          0x3c    0x66        // SRCINVERT : S ^ D
// SRC_XOR_PAT          0x5a    0x3c        //  X
// DST_XOR_PAT          0x66    0x5a        // PATINVERT : P ^ D
// NOTSRCCOPY           0x0f    0x33        // NOTSRCCOPY : ~S
// DSTINVERT            0x33    0x55        // DSTINVERT : ~D
// R2_NOTCOPYPEN        0x55    0x0f        // R2_NOTCOPYPEN : ~P
//
#define G2D_ROP_SRC_ONLY        (0xf0)
#define G2D_ROP_PAT_ONLY        (0xaa)
#define G2D_ROP_DST_ONLY        (0xcc)
#define G2D_ROP_SRC_OR_DST      (0xfc)
#define G2D_ROP_SRC_OR_PAT      (0xfa)
#define G2D_ROP_DST_OR_PAT      (0xee)
#define G2D_ROP_SRC_AND_DST     (0xc0) //(pat==1)? src:dst
#define G2D_ROP_SRC_AND_PAT     (0xa0)
#define G2D_ROP_DST_AND_PAT     (0x88)
#define G2D_ROP_SRC_XOR_DST     (0x3c)
#define G2D_ROP_SRC_XOR_PAT     (0x5a)
#define G2D_ROP_DST_XOR_PAT     (0x66)
#define G2D_ROP_NOTSRCCOPY      (0x0f)
#define G2D_ROP_DSTINVERT       (0x33)
#define G2D_ROP_R2_NOTCOPYPEN   (0x55)
#define G2D_NUM_SUPPORT_ROP     (15)

typedef struct
{
    DWORD    dwBaseaddr;
    DWORD    dwHoriRes;
    DWORD    dwVertRes;
    DWORD    dwColorMode;

} SURFACE_DESCRIPTOR, *PSURFACE_DESCRIPTOR;

typedef enum
{
    ROP_SRC_ONLY = 0,            //O
    ROP_PAT_ONLY = 1,            //O
    ROP_DST_ONLY = 2,            //O
    ROP_SRC_OR_DST = 3,        //O
    ROP_SRC_OR_PAT = 4,        //O
    ROP_DST_OR_PAT = 5,        //O
    ROP_SRC_AND_DST = 6,    //O
    ROP_SRC_AND_PAT = 7,    //O
    ROP_DST_AND_PAT = 8,    //N
    ROP_SRC_XOR_DST = 9,    //N
    ROP_SRC_XOR_PAT = 10,    //O
    ROP_DST_XOR_PAT = 11,    //N
    ROP_NOTSRCCOPY = 12,        //N
    ROP_DSTINVERT = 13,        //N
    ROP_R2_NOTCOPYPEN = 14        //N
} G2D_ROP_TYPE;


// This enumerate value can be used directly to set register
typedef enum
{
    G2D_NO_ALPHA_MODE =         (0<<10),
    G2D_PP_ALPHA_SOURCE_MODE =  (1<<10),
    G2D_ALPHA_MODE =            (2<<10),
    G2D_FADING_MODE =           (4<<10)
} G2D_ALPHA_BLENDING_MODE;
typedef enum
{
    QCIF, CIF/*352x288*/, 
    QQVGA, QVGA, VGA, SVGA/*800x600*/, SXGA/*1280x1024*/, UXGA/*1600x1200*/, QXGA/*2048x1536*/,
    WVGA/*854x480*/, HD720/*1280x720*/, HD1080/*1920x1080*/
} IMG_SIZE;

#define    HASBIT_COND(var,cond)        (((var&cond) == cond) ? TRUE : FALSE)

class FIMGSE2D : public RegCtrlG2D
{
    private:
        BYTE    m_iROPMapper[G2D_NUM_SUPPORT_ROP];
        LONG    m_iColorModeMapper[MS_NUM_SUPPORT_COLORMODE];

        /// Source Surface Descriptor
        SURFACE_DESCRIPTOR    m_descSrcSurface;
        /// Destination Surface Descriptor
        SURFACE_DESCRIPTOR    m_descDstSurface;
        //  Max Window Size of clipping window
        RECT    m_rtClipWindow;

        DWORD  m_uMaxDx;
        DWORD  m_uMaxDy;

        // Coordinate (X, Y) of clipping window
        DWORD  m_uCwX1, m_uCwY1;
        DWORD  m_uCwX2, m_uCwY2;

        DWORD  m_uFgColor;
        DWORD  m_uBgColor;
        DWORD  m_uBlueScreenColor;
        DWORD  m_uColorVal[8];

        // Reference to Raster operation
        DWORD  m_uRopVal; // Raster operation value
        DWORD  m_uAlphaBlendMode;
        DWORD  m_uTransparentMode;
        DWORD  m_u3rdOprndSel;

        // Reference to alpha value
        DWORD  m_uFadingOffsetVal;
        DWORD  m_uAlphaVal;

        // Reference to image rotation
        DWORD  m_uRotOrgX, m_uRotOrgY;
        DWORD  m_uRotAngle;

        // reference to pattern of bitblt
        DWORD  m_uPatternOffsetX, m_uPatternOffsetY;

        DWORD  m_uBytes;

        BYTE   m_ucAlphaVal;
        bool m_bIsAlphaCall;

        // true: BitBlt enable in Host-To-Screen Font Drawing
        // false: BitBlt disable in Host-To-Screen Font Drawing
        bool m_bIsBitBlt;

//        DWORD m_uFontAddr;
        bool m_bIsScr2Scr;

        // N_24X24, B_24X24, N_16X16, T_8X16, N_8X8, N_8X15
        BYTE*  m_upFontType;
        DWORD  m_uFontWidth, m_uFontHeight;
        // for Interrupt
        DWORD        m_dwSysIntr2D;                    // 2D SysIntr
        HANDLE    m_hInterrupt2D;                // handle for 2D interrupt handler

        /// Internal Functions. 
        
        /// For StretchBlt
        DWORD CalculateXYIncrFormat(DWORD uDividend, DWORD uDivisor);
        /// For StretchBlt Algorithm Compensation
        LONG GetCompensatedOffset(DWORD usSrcValue, DWORD usDstValue);
        /// Submit command and wait
        BOOL DoCmd(volatile UINT32 *CmdReg, DWORD CmdValue, G2D_CMDPROCESSING_TYPE eCmdType);
        
    public:
        FIMGSE2D();
        virtual ~FIMGSE2D();
        /// G2D Method
        /// For Initialization
        void Init();


        /// For Common Resource Setting
        void SetRopEtype(G2D_ROP_TYPE eRopType);
        /// For Rotation Setting
        ROT_TYPE GetRotType(int m_iRotate)        ;
        
        void SetTransparentMode(bool bIsTransparent, DWORD uBsColor);        

        void SetColorKeyOn(DWORD uColorKey);
        void SetColorKeyOff(void);

        void SetFgColor(DWORD uFgColor);
        void SetBgColor(DWORD uBgColor);
        void SetBsColor(DWORD uBsColor);

        void SetSrcSurface(PSURFACE_DESCRIPTOR desc_surface);
        void SetDstSurface(PSURFACE_DESCRIPTOR desc_surface);        
        void TranslateCoordinateToZero(PSURFACE_DESCRIPTOR pdescDstSurface, PRECTL prclDst, PRECTL prclClip);        

//        void SetRotate(WORD usSrcX1, WORD usSrcY1, WORD usSrcX2, WORD usSrcY2,    WORD usDestX1, WORD usDestY1, ROT_TYPE eRotDegree);

        /// For Bitblt
        void BitBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate);
        void StretchBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate);
        void StretchBlt_Bilinear(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate);
        BOOL FlipBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate);
        void FillRect(PRECTL prclDst, DWORD uColor);        //< FillRect doesn't care about Rotation, just look at Destination


        /// For Additional Effect Setting
        void EnablePlaneAlphaBlending(BYTE ucAlphaVal);
        void DisablePlaneAlphaBlending(void);

        void EnablePixelAlphaBlending(void); // Only Support 24bpp
        void DisablePixelAlphaBlending(void); // Only Support 24bpp

        void EnableFadding(BYTE ucFadingVal);
        void DisableFadding(void);

        void SetAlphaMode(G2D_ALPHA_BLENDING_MODE eMode);
        void SetAlphaValue(BYTE ucAlphaVal);
        void SetFadingValue(BYTE ucFadeVal);


        /// For Line Drawing
        void PutPixel(DWORD uPosX, DWORD uPosY, DWORD uColor);
        void PutLine(DWORD uPosX1, DWORD uPosY1, DWORD uPosX2, DWORD uPosY2, DWORD uColor, bool bIsDrawLastPoint);

        void WaitForIdle();

        //-- for NK interrupt process
        BOOL InitializeInterrupt(void);
        void DeinitInterrupt(void);
    protected:
        void GetRotationOrgXY(WORD usSrcX1, WORD usSrcY1, WORD usSrcX2, WORD usSrcY2,    WORD usDestX1, WORD usDestY1, ROT_TYPE eRotDegree, WORD* usOrgX, WORD* usOrgY);

        void DisableEffect(void);

        void SetStencilKey(DWORD uIsColorKeyOn, DWORD uIsInverseOn, DWORD uIsSwapOn);
        void SetStencilMinMax(DWORD uRedMin, DWORD uRedMax, DWORD uGreenMin, DWORD uGreenMax, DWORD uBlueMin, DWORD uBlueMax);

        void SetColorExpansionMethod(bool bIsScr2Scr);

        void BlendingOut(DWORD uSrcData, DWORD uDstData, BYTE ucAlphaVal, bool bFading, BYTE ucFadingOffset, DWORD *uBlendingOut);
        void Convert24bpp(DWORD uSrcData, EGPEFormat eBpp, bool bSwap, DWORD *uConvertedData);
        void GetRotateCoordinate(DWORD uDstX, DWORD uDstY, DWORD uOrgX, DWORD uOrgY, DWORD uRType, DWORD *uRsltX, DWORD *uRsltY);




};

#endif
