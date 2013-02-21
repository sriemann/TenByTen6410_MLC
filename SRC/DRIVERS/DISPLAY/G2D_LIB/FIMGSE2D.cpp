//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
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

Module Name:    fimgse2d.cpp

Abstract:       hardware control implementation for FIMGSE-2D v2.0, This class is a kind of adapter

Functions:

Notes:          This version is made for S3C6410.

--*/

#include "precomp.h"        // Share Display Driver's
#include <assert.h>
#include <dispperf.h>
#include "regctrl_g2d.h"

#undef SWAP
#define SWAP(a,b,type) { type tmp=a; a=b; b=tmp; }

//Define the default return type for G2D commands. Following are two common options:
//    G2D_INTERRUPT:- This waits for the G2D idle interrupt before returning from DoCmd function
//    G2D_FASTRETURN:- This returns immediately after issuing the command to G2D
//
// Using G2D_FASTRETURN enables BLTs to happen in parallel while the calling application
// prepares the next BLT. Synchronization is required before preparing the bext BLT, in 
// WaitForNotBusy() and in DDGPE::Lock(). These have been implemented and so DoCmd() can
// use G2D_FASTRETURN
//
#define G2D_DEFAULT_RETURN_TYPE G2D_FASTRETURN

#define G2D_INTERRUPT_TIMEOUT_PER_TRY   10  // 10ms timeout per try
#define G2D_INTERRUPT_TIMEOUT_RETRIES   10  // Max 10 retries

FIMGSE2D::FIMGSE2D() : RegCtrlG2D()
{
//    Reset();
        m_iROPMapper[ROP_SRC_ONLY] = G2D_ROP_SRC_ONLY;
        m_iROPMapper[ROP_PAT_ONLY] = G2D_ROP_PAT_ONLY;
        m_iROPMapper[ROP_DST_ONLY] = G2D_ROP_DST_ONLY;
        m_iROPMapper[ROP_SRC_OR_DST] = G2D_ROP_SRC_OR_DST;
        m_iROPMapper[ROP_SRC_OR_PAT] = G2D_ROP_SRC_OR_PAT;
        m_iROPMapper[ROP_DST_OR_PAT] = G2D_ROP_DST_OR_PAT;
        m_iROPMapper[ROP_SRC_AND_DST] = G2D_ROP_SRC_AND_DST;
        m_iROPMapper[ROP_SRC_AND_PAT] = G2D_ROP_SRC_AND_PAT;
        m_iROPMapper[ROP_DST_AND_PAT] = G2D_ROP_DST_AND_PAT;
        m_iROPMapper[ROP_SRC_XOR_DST] = G2D_ROP_SRC_XOR_DST;
        m_iROPMapper[ROP_SRC_XOR_PAT] = G2D_ROP_SRC_XOR_PAT;
        m_iROPMapper[ROP_DST_XOR_PAT] = G2D_ROP_DST_XOR_PAT;
        m_iROPMapper[ROP_NOTSRCCOPY] = G2D_ROP_NOTSRCCOPY;
        m_iROPMapper[ROP_DSTINVERT] = G2D_ROP_DSTINVERT;
        m_iROPMapper[ROP_R2_NOTCOPYPEN] = G2D_ROP_R2_NOTCOPYPEN;
}

FIMGSE2D::~FIMGSE2D()
{
}

// Set Ternary raster operation
// Support 256 raster operation
// Refer to ternary raster operation table if you know 256 ROP

// Set Alpha Value
void FIMGSE2D::SetAlphaValue(BYTE ucAlphaVal)
{
    ucAlphaVal &= 0xff;
    m_pG2DReg->ALPHA = (m_pG2DReg->ALPHA&(~0xff)) | ucAlphaVal;
}

// Set alpha blending mode
void FIMGSE2D::SetAlphaMode(G2D_ALPHA_BLENDING_MODE eMode)
{
    RequestEmptyFifo(1);
/*    DWORD uAlphaBlend;

    uAlphaBlend =
        (eMode == G2D_NO_ALPHA_MODE) ? G2D_NO_ALPHA_BIT :
        (eMode == G2D_PP_ALPHA_SOURCE_MODE) ? G2D_PP_ALPHA_SOURCE_BIT :
        (eMode == G2D_ALPHA_MODE) ? G2D_ALPHA_BIT : 
        (eMode == G2D_FADING_MODE) ? G2D_FADING_BIT : G2D_NO_ALPHA_BIT;
*/
    m_pG2DReg->ROP = (m_pG2DReg->ROP & ~(0x7<<10)) | eMode;//AlphaBlend;
}

// Set fade value
void FIMGSE2D::SetFadingValue(BYTE ucFadeVal)
{
    ucFadeVal &= 0xff;
    m_pG2DReg->ALPHA = (m_pG2DReg->ALPHA & ~(0xff<<8)) | (ucFadeVal<<8);
}

void FIMGSE2D::DisableEffect(void)
{
    m_pG2DReg->ROP &= ~(0x7<<10);
}

void FIMGSE2D::EnablePlaneAlphaBlending(BYTE ucAlphaVal)
{
    ucAlphaVal &= 0xff;

    // Set Alpha Blending Mode
    m_pG2DReg->ROP = ((m_pG2DReg->ROP) & ~(0x7<<10)) | G2D_ALPHA_MODE;


    // Set Alpha Value
    m_pG2DReg->ALPHA = ((m_pG2DReg->ALPHA) & ~(0xff)) | ucAlphaVal;

    m_ucAlphaVal = ucAlphaVal;
    m_bIsAlphaCall = true;
}

void FIMGSE2D::DisablePlaneAlphaBlending(void)
{
    DisableEffect();
}

void FIMGSE2D::EnablePixelAlphaBlending(void) // Only Support 24bpp and Only used in BitBlt
{
    m_pG2DReg->ROP = ((m_pG2DReg->ROP) & ~(0x7<<10)) | G2D_PP_ALPHA_SOURCE_MODE;
}

void FIMGSE2D::DisablePixelAlphaBlending(void) // Only Support 24bpp and only used in BitBlt
{
    DisableEffect();
}

void FIMGSE2D::EnableFadding(BYTE ucFadingVal)
{
    BYTE ucAlphaVal;

    ucAlphaVal = (m_bIsAlphaCall == true) ? m_ucAlphaVal : 255;

    ucFadingVal &= 0xff;

    // Set Fadding Mode    
    m_pG2DReg->ROP = ((m_pG2DReg->ROP) & ~(0x7<<10)) | G2D_FADING_MODE;

    // Set Fadding Value    
    m_pG2DReg->ALPHA = ((m_pG2DReg->ALPHA) & ~(0xff<<8)) | (ucFadingVal<<8) | (ucAlphaVal<<0);
}

void FIMGSE2D::DisableFadding(void)
{
    DisableEffect();
}



/**
*    @fn    FIMGSE2D::GetRotType(int m_iRotate)
*    @brief    This function convert rotation degree value to ROT_TYPE
*
*/
ROT_TYPE FIMGSE2D::GetRotType(int m_iRotate)
{
    switch(m_iRotate)
    {
        case DMDO_0:
            return    ROT_0;
        case DMDO_90:
            return    ROT_270;
        case DMDO_180:
            return    ROT_180;
        case DMDO_270:
            return    ROT_90;
        default:
            return    ROT_0;
    }
    return ROT_0;
}

/**
*    @fn    DWORD FIMGSE2D::CalculateXYIncrFormat(DWORD uDividend, DWORD uDivisor)
*    @brief    This function returns x_incr or y_incr vaule in register format
*    @input    this function accept real pixel coordinate, ex) (0,0)~(9,9) means that 10pixel by pixel image
*    @return    Result value
*/
DWORD FIMGSE2D::CalculateXYIncrFormat(DWORD uDividend, DWORD uDivisor)
{
    int i;
    DWORD uQuotient;
    DWORD uUnderPoint=0;

    if(uDivisor == 0)
    {
        uDivisor = 1;    //< this will prevent data abort. but result is incorrect.
    }

    uQuotient = (DWORD)(uDividend/uDivisor);
    // Quotient should be less than MAX_XCOORD or MAX_YCOORD.
    if(uQuotient > MAX_2DHW_XCOORD) 
    {
        RETAILMSG(DISP_ZONE_WARNING, (TEXT("Increment value to stretch can not exceed %d, Value will be set as 1.0\n"), MAX_2DHW_XCOORD));
        return ((1<<11) | 0 );
    }

    uDividend-=(uQuotient*uDivisor);

    /// Now under point is calculated.
    for (i=0; i<12; i++)
    {
        uDividend <<= 1;
        uUnderPoint <<= 1;

        if (uDividend >= uDivisor)
        {
            uUnderPoint = uUnderPoint | 1;
            uDividend -= uDivisor;
        }
        DEBUGMSG(DISP_ZONE_2D, (TEXT("uDivend:%x(%d), uDivisor:%x(%d), uUnderPoint:%x(%d)\n"), uDividend, uDividend, uDivisor, uDivisor,uUnderPoint, uUnderPoint));
    }

    uUnderPoint = (uUnderPoint + 1) >> 1;

    return ( uUnderPoint|(uQuotient<<11) );
}

/**
*    @fn    FIMGSE2D::BitBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
*    @param    prclSrc    Source Rectangle
*    @param    prclDst    Destination Rectangle
*    @param    m_iRotate    Rotatation Degree. See also ROT_TYPE type
*    @note This funciton performs real Bit blit using 2D HW. this functio can handle rotation case.
*            There's predefine macro type for presenting rotation register's setting value
*            G2D_ROTATION
@    @sa    ROT_TYPE    this can be set mixed value.
*/
void FIMGSE2D::BitBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate )
{
    DWORD uCmdRegVal=0;
    RECT    rectDst;            //< If rotation case this value must be corrected.
    
    RETAILMSG(DISP_ZONE_ENTER,(TEXT("[2DHW] BitBlt Entry\r\n")));    

    /// Always LeftTop Coordinate is less than RightBottom for Source and Destination Region
    assert( (prclSrc->left < prclSrc->right) && (prclSrc->top < prclSrc->bottom) );
    assert( (prclDst->left < prclDst->right) && (prclDst->top < prclDst->bottom) );    

    /// Set Destination's Rotation mode
    SetRotationMode(m_iRotate);
    SetCoordinateSrcBlock(prclSrc->left, prclSrc->top, prclSrc->right - 1, prclSrc->bottom - 1);

    if(m_iRotate == ROT_180)        //< origin set to (x2,y2)
    {
        rectDst.left = prclDst->right - 1;                        //< x2
        rectDst.top = prclDst->bottom - 1;                        //< y2
        rectDst.right = 2 * (prclDst->right - 1) - prclDst->left ;        //< x2 + (x2 - x1)
        rectDst.bottom = 2 * (prclDst->bottom -1) - prclDst->top;    //< y2 + (y2 - y1)
    }
    else     if(m_iRotate == ROT_90)        //<In this time, Height and Width are swapped.
    {
        rectDst.left = prclDst->right - 1;                        //< x2
        rectDst.right = prclDst->right - 1 + prclDst->bottom - 1 - prclDst->top;    //< x2 + (y2 - y1)
        rectDst.top = prclDst->top;                                        //< y1
        rectDst.bottom = prclDst->top + prclDst->right - 1 - prclDst->left;        //< y1 + (x2 - x1)
    }
    else     if(m_iRotate == ROT_270)        //<In this time, Height and Width are swapped.
    {
        rectDst.left = prclDst->left;                            //< x1
        rectDst.right = prclDst->left + prclDst->bottom - 1- prclDst->top;        //< x1 + (y2 - y1)
        rectDst.top = prclDst->bottom - 1;                                    //< y2
        rectDst.bottom = prclDst->bottom - 1 + prclDst->right - 1- prclDst->left;    //< y2 + (x2 - x1)
    }
    else        //< ROT_0
    {
        rectDst.left = prclDst->left;
        rectDst.top = prclDst->top;
        rectDst.right = prclDst->right - 1;
        rectDst.bottom = prclDst->bottom - 1;
    }

    SetRotationOrg((WORD)rectDst.left, (WORD)rectDst.top);
    SetCoordinateDstBlock(rectDst.left, rectDst.top, rectDst.right, rectDst.bottom);

    RETAILMSG(DISP_ZONE_2D,(TEXT("ROT:%d, Src:(%d,%d)~(%d,%d), Dst:(%d,%d)~(%d,%d)\r\n"), 
        m_iRotate, prclSrc->left, prclSrc->top, prclSrc->right, prclSrc->bottom, 
        rectDst.left, rectDst.top, rectDst.right, rectDst.bottom));
        
    uCmdRegVal = G2D_NORMAL_BITBLT_BIT;
    
    DoCmd(&(m_pG2DReg->CMDR1), uCmdRegVal, G2D_DEFAULT_RETURN_TYPE);

    RETAILMSG(DISP_ZONE_ENTER,(TEXT("[2DHW] BitBlt Exit\r\n")));            
    /// TODO: Resource Register clearing can be needed.

}

/**
*    @fn    FIMGSE2D::GetCompensatedOffset(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
*    @param    prclSrc    Source Rectangle
*    @param    prclDst    Destination Rectangle
*    @param    m_iRotate    Rotatation Degree. See also ROT_TYPE type
*    @note This funciton performs getting offset for stretchblt algorithm compensation
*    @sa    ROT_TYPE
*
**/
LONG FIMGSE2D::GetCompensatedOffset(DWORD usSrcValue, DWORD usDstValue)
{
    /// Calculate X,Y Offset
    float fIncrement;
    float fStretchRatio;
    float fReferPoint = 0.5;
    LONG i =0;
    
    fIncrement = (float)usSrcValue / (float)usDstValue;
    fStretchRatio = (float)usDstValue / (float)usSrcValue;
    
    do
    {
        if(fReferPoint > 1) break;    
        fReferPoint += fIncrement;
        i++;
    } while(1);

    RETAILMSG(DISP_ZONE_2D,(TEXT("\n fIncr : %5.6f, fSR: %5.6f, i : %d, Offset : %d"), fIncrement, fStretchRatio, i, (LONG)(fStretchRatio - i)));
    
    return (fStretchRatio < 1) ? 0 : (LONG)(fStretchRatio - i);
}

/**
*    @fn    FIMGSE2D::StretchBlt_Bilinear(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
*    @param    prclSrc    Source Rectangle
*    @param    prclDst    Destination Rectangle
*    @param    m_iRotate    Rotatation Degree. See also ROT_TYPE type
*    @note This funciton performs real Stretched Bit blit using 2D HW. this functio can handle rotation case.
*            There's predefine macro type for presenting rotation register's setting value
*            G2D_ROTATION
*    @note This function can not support Multiple Operation ex) mirrored + rotation because of HW
*    @sa    ROT_TYPE
**/
void FIMGSE2D::StretchBlt_Bilinear(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
{
    POINT ptCompensatedOffset;
    WORD usSrcWidth = 0;
    WORD usSrcHeight = 0;
    WORD usDstWidth = 0;
    WORD usDstHeight = 0;
    
    DWORD uXIncr = 0;
    DWORD uYIncr = 0;
    DWORD uCmdRegVal=0;

    RECTL    rectDst;
    RECTL    rectDstRT;            //< If rotation case this value must be corrected.
    RECTL    rectDstLT;
    RECTL    rectDstRB;
    RECTL    rectDstLB;

    RETAILMSG(DISP_ZONE_ENTER,(TEXT("\n[2DHW] StretchBlt Entry")));    

    /// Always LeftTop Coordinate is less than RightBottom for Source and Destination Region
    assert( (prclSrc->left < prclSrc->right) && (prclSrc->top < prclSrc->bottom) );
    assert( (prclDst->left < prclDst->right) && (prclDst->top < prclDst->bottom) );    

    /// Set Stretch parameter
    /// most right and bottom line does not be drawn.
    usSrcWidth=(WORD) ABS( prclSrc->right  - prclSrc->left);
    usDstWidth=(WORD) ABS( prclDst->right  - prclDst->left);
    usSrcHeight=(WORD) ABS( prclSrc->bottom  - prclSrc->top);
    usDstHeight=(WORD) ABS( prclDst->bottom  - prclDst->top);

    if((m_iRotate == ROT_90) ||(m_iRotate == ROT_270) )
    {
        SWAP(usDstHeight, usDstWidth, WORD);
    }

    /// Stretch ratio calculation, width and height is include last line
    /// ex) 10x10 to 30x30
    /// Given Coordinate parameter
    /// SrcCoord. (0,0)~(10,10) :  srcwidth = 10-0 = 10, srcheight = 10-0 = 10
    /// DstCoord. (30,30)~(60,60) : dstwidth = 60-30 = 30, dstheight = 60-30 = 30
    /// Actual using coordinate
    /// src (0,0)~(9,9)
    /// Dst (30,30)~(59,59)
    /// Increment calculation : srcwidth/dstwidth = 10/30 = 0.33333...
    
    if(usSrcWidth == usDstWidth && usSrcHeight == usDstHeight)
    {
        RETAILMSG(DISP_ZONE_2D, (TEXT("\nThis is not stretch or shrink BLT, redirect to BitBlt, R:%d"), m_iRotate));
        BitBlt(prclSrc, prclDst, m_iRotate);
        return;
    }
    
    /// calculate horizontal length        
    uXIncr = CalculateXYIncrFormat(usSrcWidth , usDstWidth );    
    SetXIncr(uXIncr);
    RETAILMSG(DISP_ZONE_2D,(TEXT("\nXIncr : %d.%d"), (uXIncr&0x003ff800)>>11, (uXIncr & 0x000007ff)));        
    
    /// calculate vertical length
    uYIncr = CalculateXYIncrFormat(usSrcHeight  , usDstHeight );
    SetYIncr(uYIncr);
    RETAILMSG(DISP_ZONE_2D,(TEXT("\nYIncr : %d.%d"), (uYIncr&0x003ff800)>>11, (uYIncr & 0x000007ff)));            

    /// Set Source Region Coordinate
    SetCoordinateSrcBlock(prclSrc->left, prclSrc->top, prclSrc->right - 1, prclSrc->bottom - 1);    

    /// Now We divide destination region by 4 logically.
    /// 1. LeftTop Vertical Bar, 2. RightTop Block, 3. LeftBottom Small Block, 4.RightBottom Horizontal Bar
    
    /// 1. LeftTop Vertical Bar
    /// This region has destination's height - RightBottom Horizontal bar's height
    /// and has width value can be gotten by this fomula : 
    ///     Refered source surface's pixel coordinate = 0.5 + Increment ratio(Src/Dst) * destination's coordinate
    /// In stretchBlt, Destination's coordinate always starts from (0,0)
    /// Here, Bar's width is (max desitnation's coordinate+1) until refered source surface's pixel coordinate is not over 1.0.
    /// ex) 10x10 to 30x30
    ///    Increment ratio = 10/30 = 0.33333
    ///    Refered source surface's pixel coordinate = 0.5+0.333*0, 0.5+0.333*1, 0.5+0.333*2
    ///    0.5, 0.833  meets this condition. so max destination's coordnate is 1. width is 2
    /// then each block of destination's has this region
    /// LT = (0,0)~(1,27), LB(0,28)~(1,29), RT(2,0)~(29,27), RB(2,28)~(29,29)
    ///  real stretch ratio is 30/10 = 3. so 2 is less than 3. we need add offset 1
    /// ex) 10x10 to 50x50
    ///    Increment ratio = 10/50 = 0.2
    ///    Refered source surface's pixel coordinate = 0.5+0.2*0, 0.5+0.2*1, 0.5+0.2*2, 0.5+0.2*3
    ///   0.5, 0.7, 0.9 meets this condition. so max destination's coordinate is 2. width is 3
    /// then each block of desitnation's has this region
    /// LT = (0,0)~(2,46), LB(0,47)~(2,49), RT(3,0)~(49,47), RB(3,47)~(49,49)
    ///  real stretch ratio is 50/10 = 5. so 3 is less than 5, we need add offset 2
    ptCompensatedOffset.x = GetCompensatedOffset(usSrcWidth, usDstWidth);
    ptCompensatedOffset.y = GetCompensatedOffset(usSrcHeight, usDstHeight);

    ///    

    /// Calculate Destination Region Coordinate for each rotation degree
    if(m_iRotate == ROT_180)        //< origin set to (x2,y2)
    {
        rectDst.left = prclDst->right - 1;                        //< x2
        rectDst.top = prclDst->bottom - 1;                        //< y2
        rectDst.right = 2 * (prclDst->right - 1) - prclDst->left ;        //< x2 + (x2 - x1)
        rectDst.bottom = 2 * (prclDst->bottom -1) - prclDst->top;    //< y2 + (y2 - y1)
    }
    else     if(m_iRotate == ROT_90)        //<In this time, Height and Width are swapped.    
    {
        rectDst.left = prclDst->right - 1;                        //< x2
        rectDst.right = prclDst->right - 1 + prclDst->bottom - 1 - prclDst->top;    //< x2 + (y2 - y1)
        rectDst.top = prclDst->top;                                        //< y1
        rectDst.bottom = prclDst->top + prclDst->right - 1 - prclDst->left;        //< y1 + (x2 - x1)
    }
    else     if(m_iRotate == ROT_270)        //<In this time, Height and Width are swapped.    
    {
        rectDst.left = prclDst->left;                            //< x1
        rectDst.right = prclDst->left + prclDst->bottom - 1- prclDst->top;        //< x1 + (y2 - y1)
        rectDst.top = prclDst->bottom - 1;                                    //< y2
        rectDst.bottom = prclDst->bottom - 1 + prclDst->right - 1- prclDst->left;    //< y2 + (x2 - x1)
    }
    else        //< ROT_0
    {
        rectDst.left = prclDst->left;
        rectDst.top = prclDst->top;        
        rectDst.right = prclDst->right - 1;
        rectDst.bottom = prclDst->bottom - 1;
    }

    RETAILMSG(DISP_ZONE_2D,(TEXT("\nROT:%d, Src:(%d,%d)~(%d,%d), Dst:(%d,%d)~(%d,%d), OC:(%d,%d)"), 
        m_iRotate, prclSrc->left, prclSrc->top, prclSrc->right, prclSrc->bottom, 
        rectDst.left, rectDst.top, rectDst.right, rectDst.bottom, rectDst.left, rectDst.top));

    /// Set Destination's Rotation mode
    SetRotationMode(m_iRotate);        
    SetRotationOrg((WORD)rectDst.left, (WORD)rectDst.top);    
    uCmdRegVal = G2D_STRETCH_BITBLT_BIT;
    
    if(ptCompensatedOffset.x != 0 || ptCompensatedOffset.y != 0)
    {
        rectDstRB.left = rectDst.left + ptCompensatedOffset.x;
        rectDstRB.right = rectDst.right;
        rectDstRB.top = rectDst.top + ptCompensatedOffset.y;
        rectDstRB.bottom = rectDst.bottom;
        SetClipWindow(&rectDstRB);    //< Reconfigure clip region as Each Block's region    
        SetCoordinateDstBlock(rectDstRB.left, rectDstRB.top, rectDstRB.right + ptCompensatedOffset.x, rectDstRB.bottom + ptCompensatedOffset.y);

        /// First Issuing for Right Bottom Big Region.
        RequestEmptyFifo(1);
        
        RETAILMSG(DISP_ZONE_2D,(TEXT("\nRight Bottom Block : Dst:(%d,%d)~(%d,%d)"), 
            rectDstRB.left, rectDstRB.top, rectDstRB.right, rectDstRB.bottom));    
        
        m_pG2DReg->CMDR1 = uCmdRegVal;

        rectDstRT.left = rectDstRB.left;
        rectDstRT.right = rectDst.right;
        rectDstRT.top = rectDst.top;
        rectDstRT.bottom = rectDstRB.top - 1;
        SetClipWindow(&rectDstRT);
        SetCoordinateDstBlock(rectDstRT.left, rectDst.top, rectDst.right + ptCompensatedOffset.x, rectDst.bottom);

        /// Second Issuing for Right Top Horizontal Bar Region.(in 0, 180 degree)
        RequestEmptyFifo(1);
        RETAILMSG(DISP_ZONE_2D,(TEXT("\nRight Top Block : Dst:(%d,%d)~(%d,%d)"), 
            rectDstRT.left, rectDstRT.top, rectDstRT.right, rectDstRT.bottom));    
        
        m_pG2DReg->CMDR1 = uCmdRegVal;

        
        rectDstLB.left = rectDst.left;
        rectDstLB.right = rectDstRB.left - 1;
        rectDstLB.top = rectDstRB.top;
        rectDstLB.bottom = rectDst.bottom;
        SetClipWindow(&rectDstLB);
        SetCoordinateDstBlock(rectDst.left, rectDstLB.top, rectDst.right, rectDstLB.bottom + ptCompensatedOffset.y);    
        /// Third Issuing for Left Bottom Vertical Bar(in 0,180 degree)
        RequestEmptyFifo(1);
        RETAILMSG(DISP_ZONE_2D,(TEXT("\nLeft Bottom Block : Dst:(%d,%d)~(%d,%d)"), 
            rectDstLB.left, rectDstLB.top, rectDstLB.right, rectDstLB.bottom));    
        
        m_pG2DReg->CMDR1 = uCmdRegVal;

        
        rectDstLT.left = rectDst.left;
        rectDstLT.right = rectDstLB.right;
        rectDstLT.top = rectDst.top;
        rectDstLT.bottom = rectDstRT.bottom;
        SetClipWindow(&rectDstLT);
    RETAILMSG(DISP_ZONE_2D,(TEXT("\nLeft Top Block : Dst:(%d,%d)~(%d,%d)"), 
        rectDstLT.left, rectDstLT.top, rectDstLT.right, rectDstLT.bottom));    
        
    }
    
    SetCoordinateDstBlock(rectDst.left, rectDst.top, rectDst.right, rectDst.bottom);    
    /// Last Issuing for Left Top Small Region(in 0,180 degree)

    DoCmd(&(m_pG2DReg->CMDR1), uCmdRegVal, G2D_DEFAULT_RETURN_TYPE);    

    RETAILMSG(DISP_ZONE_ENTER,(TEXT("\n[2DHW] StretchBlt Exit")));
    /// TODO: Resource Register clearing can be needed.

}
/// This implementation has distorted stretch result.
/**
*   @fn FIMGSE2D::StretchBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
*   @param  prclSrc Source Rectangle
*   @param  prclDst Destination Rectangle
*   @param  m_iRotate   Rotatation Degree. See also ROT_TYPE type
*   @note This funciton performs real Stretched Bit blit using 2D HW. this functio can handle rotation case.
*       There's predefine macro type for presenting rotation register's setting value
*       G2D_ROTATION
*   @note This function can not support Multiple Operation ex) mirrored + rotation because of HW
*   @sa ROT_TYPE
**/
void FIMGSE2D::StretchBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
{
    WORD usSrcWidth = 0;
    WORD usSrcHeight = 0;
    WORD usDstWidth = 0;
    WORD usDstHeight = 0;
    DWORD uXYIncr = 0;
    DWORD uCmdRegVal=0;

    RECTL    rectDst;            //< If rotation case this value must be corrected.

    RETAILMSG(DISP_ZONE_ENTER,(TEXT("[2DHW] StretchBlt Entry\r\n")));    

    /// Always LeftTop Coordinate is less than RightBottom for Source and Destination Region
    assert( (prclSrc->left < prclSrc->right) && (prclSrc->top < prclSrc->bottom) );
    assert( (prclDst->left < prclDst->right) && (prclDst->top < prclDst->bottom) );    

    /// Set Stretch parameter
    /// Stretch ratio calculation, width and height is not include last line
    usSrcWidth=(WORD) ABS( prclSrc->right  - prclSrc->left);
    usDstWidth=(WORD) ABS( prclDst->right  - prclDst->left);
    usSrcHeight=(WORD) ABS( prclSrc->bottom  - prclSrc->top);
    usDstHeight=(WORD) ABS( prclDst->bottom  - prclDst->top);

    if((m_iRotate == ROT_90) ||(m_iRotate == ROT_270) )
    {
        SWAP(usDstHeight, usDstWidth, WORD);
    }    

    /// When Orthogonally Rotated operation is conducted,     
    if(usSrcWidth == usDstWidth && usSrcHeight == usDstHeight)
    {
        RETAILMSG(DISP_ZONE_2D, (TEXT("This is not stretch or shrink BLT, redirect to BitBlt, R:%d\n"), m_iRotate));
        BitBlt(prclSrc, prclDst, m_iRotate);
        return;
    }
    
    uXYIncr = CalculateXYIncrFormat(usSrcWidth -1, usDstWidth -1);
    SetXIncr(uXYIncr);    
    RETAILMSG(DISP_ZONE_2D,(TEXT("\nXIncr : %d.%x"), (uXYIncr&0x003ff800)>>11, (uXYIncr & 0x000007ff)));    

    uXYIncr = CalculateXYIncrFormat(usSrcHeight -1, usDstHeight -1);
    SetYIncr(uXYIncr);
    RETAILMSG(DISP_ZONE_2D,(TEXT("\nYIncr : %d.%x"), (uXYIncr&0x003ff800)>>11, (uXYIncr & 0x000007ff)));        
    
    SetCoordinateSrcBlock(prclSrc->left, prclSrc->top, prclSrc->right - 1, prclSrc->bottom - 1);    

    if(m_iRotate == ROT_180)        //< origin set to (x2,y2)
    {
        rectDst.left = prclDst->right - 1;                        //< x2
        rectDst.top = prclDst->bottom - 1;                        //< y2
        rectDst.right = 2 * (prclDst->right - 1) - prclDst->left ;        //< x2 + (x2 - x1)
        rectDst.bottom = 2 * (prclDst->bottom -1) - prclDst->top;    //< y2 + (y2 - y1)
    }
    else     if(m_iRotate == ROT_90)        //<In this time, Height and Width are swapped.
    {
        rectDst.left = prclDst->right - 1;                        //< x2
        rectDst.right = prclDst->right - 1 + prclDst->bottom - 1 - prclDst->top;    //< x2 + (y2 - y1)
        rectDst.top = prclDst->top;                                        //< y1
        rectDst.bottom = prclDst->top + prclDst->right - 1 - prclDst->left;        //< y1 + (x2 - x1)
    }
    else     if(m_iRotate == ROT_270)        //<In this time, Height and Width are swapped.
    {
        rectDst.left = prclDst->left;                            //< x1
        rectDst.right = prclDst->left + prclDst->bottom - 1- prclDst->top;        //< x1 + (y2 - y1)
        rectDst.top = prclDst->bottom - 1;                                    //< y2
        rectDst.bottom = prclDst->bottom - 1 + prclDst->right - 1- prclDst->left;    //< y2 + (x2 - x1)
    }
    else        //< ROT_0
    {
        rectDst.left = prclDst->left;
        rectDst.top = prclDst->top;
        rectDst.right = prclDst->right - 1;
        rectDst.bottom = prclDst->bottom - 1;
    }

    /// Set Destination's Rotation mode
    SetRotationMode(m_iRotate);
    SetRotationOrg((WORD)rectDst.left, (WORD)rectDst.top);
    SetCoordinateDstBlock(rectDst.left, rectDst.top, rectDst.right, rectDst.bottom);

    RETAILMSG(DISP_ZONE_2D,(TEXT("ROT:%d, Src:(%d,%d)~(%d,%d), Dst:(%d,%d)~(%d,%d), OC:(%d,%d)\r\n"), 
        m_iRotate, prclSrc->left, prclSrc->top, prclSrc->right, prclSrc->bottom, 
        rectDst.left, rectDst.top, rectDst.right, rectDst.bottom, rectDst.left, rectDst.top));

    uCmdRegVal = G2D_STRETCH_BITBLT_BIT;

    DoCmd(&(m_pG2DReg->CMDR1), uCmdRegVal, G2D_DEFAULT_RETURN_TYPE);

    RETAILMSG(DISP_ZONE_ENTER,(TEXT("[2DHW] StretchBlt Exit\r\n")));            
    /// TODO: Resource Register clearing can be needed.
}

/**
*    @fn    FIMGSE2D::FlipBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
*    @param    prclSrc    Source Rectangle
*    @param    prclDst    Destination Rectangle
*    @param    m_iRotate    Flip Setting. See also ROT_TYPE type
*    @note This funciton performs ONLY FLIP Bit blit using 2D HW. this function cannot handle rotation case.
*            There's predefine macro type for presenting rotation register's setting value
*            This function requires Scratch Memory for Destination.
*            This function don't support X&Y flipping
*    @sa    ROT_TYPE
**/
BOOL FIMGSE2D::FlipBlt(PRECTL prclSrc, PRECTL prclDst, ROT_TYPE m_iRotate)
{
    DWORD uCmdRegVal=0;
    BOOL bRetVal = FALSE;
    RECTL    rectDst;            //< If rotation case this value must be corrected.    

    RETAILMSG(DISP_ZONE_ENTER,(TEXT("[2DHW] FlipBlt Entry\r\n")));                

    /// Always LeftTop Coordinate is less than RightBottom for Source and Destination Region
    assert( (prclSrc->left < prclSrc->right) && (prclSrc->top < prclSrc->bottom) );
    assert( (prclDst->left < prclDst->right) && (prclDst->top < prclDst->bottom) );

    /// Check Flip Option, we only do care about only flip, don't care about rotation option although it set.
    if(HASBIT_COND(m_iRotate, FLIP_X))
    {
        SetRotationMode(FLIP_X);
        /// Set rotation origin on destination's bottom line.
        rectDst.left = prclDst->left;                    //< x1
        rectDst.right = prclDst->right - 1;                //< x2
        rectDst.top = prclDst->bottom - 1;            //< y2
        rectDst.bottom = prclDst->bottom - 1 + prclDst->bottom - 1 - prclDst->top;    //< y2 + (y2-y1)
    }
    else if(HASBIT_COND(m_iRotate, FLIP_Y))
    {
        SetRotationMode(FLIP_Y);
        /// Set rotation origin on destination's right line.
        rectDst.left = prclDst->right - 1;                //< x2
        rectDst.right = prclDst->right - 1 + prclDst->right - 1 - prclDst->left;        //< x2 + (x2 - x1)
        rectDst.top = prclDst->top;                    //< y1
        rectDst.bottom = prclDst->bottom - 1;            //< y2
    }
    else
    {
        /// Do not need to do Flip operation.
        return FALSE;
    }

    SetCoordinateSrcBlock(prclSrc->left, prclSrc->top, prclSrc->right - 1, prclSrc->bottom - 1);
    SetRotationOrg((WORD)rectDst.left, (WORD)rectDst.top);
    SetCoordinateDstBlock(rectDst.left, rectDst.top, rectDst.right, rectDst.bottom);

    uCmdRegVal = G2D_NORMAL_BITBLT_BIT;

    bRetVal = DoCmd(&(m_pG2DReg->CMDR1), uCmdRegVal, G2D_DEFAULT_RETURN_TYPE);
    
    RETAILMSG(DISP_ZONE_ENTER,(TEXT("[2DHW] FlipBlt Exit\r\n")));            
    /// TODO: Resource Register clearing can be needed.

    return bRetVal;

}

// if ucTransMode is '1', Transparent Mode
// else '0', Opaque Mode
void FIMGSE2D::SetTransparentMode(bool bIsTransparent, COLOR uBsColor)
{
    DWORD uRopRegVal;

    RequestEmptyFifo(2);

    uRopRegVal = m_pG2DReg->ROP;

    uRopRegVal =
        (bIsTransparent == 1) ? (uRopRegVal | G2D_TRANSPARENT_BIT) : (uRopRegVal & ~(G2D_TRANSPARENT_BIT));

    m_pG2DReg->ROP = uRopRegVal;

    // register Blue Screen Color
    m_pG2DReg->BS_COLOR = uBsColor;
}

// if ucTransMode is '1', Transparent Mode
// else '0', Opaque Mode
void FIMGSE2D::SetColorKeyOn(DWORD uBsColor)
{
    RequestEmptyFifo(2);

    m_pG2DReg->ROP = m_pG2DReg->ROP | G2D_TRANSPARENT_BIT;

    // register Blue Screen Color
    m_pG2DReg->BS_COLOR = uBsColor;
}

void FIMGSE2D::SetColorKeyOff(void)
{
    RequestEmptyFifo(2);

    // Blue screen off
    m_pG2DReg->ROP =  m_pG2DReg->ROP & ~(G2D_TRANSPARENT_BIT);

    // color key off
    m_pG2DReg->COLORKEY_CNTL = (m_pG2DReg->COLORKEY_CNTL & ~(0x1U<<31));
}

void FIMGSE2D::SetFgColor(DWORD uFgColor)
{
    RequestEmptyFifo(1);
    uFgColor &= 0x00ffffff;        //< Remove Alpha value
    m_pG2DReg->FG_COLOR = uFgColor;
}

void FIMGSE2D::SetBgColor(DWORD uBgColor)
{
    RequestEmptyFifo(1);
    uBgColor &= 0x00ffffff;        //< Remove Alpha value
    m_pG2DReg->BG_COLOR = uBgColor;
}

void FIMGSE2D::SetBsColor(DWORD uBsColor)
{
    RequestEmptyFifo(1);
    uBsColor &= 0x00ffffff;        //< Remove Alpha value
    m_pG2DReg->BS_COLOR = uBsColor;
}


/**
*    @fn    void FIMGSE2D::FillRect(PRECT prtDst, DWORD uColor)
*    @param    prtDst    Destination Rectangle
*    @param    uColor    Filled Color
*    @attention    prtDst must have positive value.
*    @brief    prclDst must be rotated when screen is rotated.
*/
void FIMGSE2D::FillRect(PRECTL prclDst, COLOR uColor)
{
    SetFgColor(uColor);
    Set3rdOperand(G2D_OPERAND3_FG);
    SetRopEtype(ROP_PAT_ONLY);
    BitBlt(prclDst, prclDst, ROT_0);        // Fill Rect doesn't care about screen rotation,
}

/*
 *     @fn    void FIMGSE2D::SetSrcSurface(PSURFACE_DESCRIPTOR desc_surface)
 *    @brief    Set Source Surface Information to FIMG2D HW Register
 *    @param    desc_surface    Surface Information : Base Address, Horizontal&Vertical Resolution, Color mode
 */
void FIMGSE2D::SetSrcSurface(PSURFACE_DESCRIPTOR desc_surface)
{
    RequestEmptyFifo(4);
    
    m_pG2DReg->SRC_BASE_ADDR = desc_surface->dwBaseaddr;
    
    m_pG2DReg->SRC_COLOR_MODE = desc_surface->dwColorMode;
    
    m_pG2DReg->SRC_HORI_RES = desc_surface->dwHoriRes;
    m_pG2DReg->SRC_VERT_RES = desc_surface->dwVertRes;
}

/*
 *     @fn    void FIMGSE2D::SetDstSurface(PSURFACE_DESCRIPTOR desc_surface)
 *    @brief    Set Destination Surface Information to FIMG2D HW Register
 *    @param    desc_surface    Surface Information : Base Address, Horizontal&Vertical Resolution, Color mode
 */
void FIMGSE2D::SetDstSurface(PSURFACE_DESCRIPTOR desc_surface)
{
    RequestEmptyFifo(4);
    
    m_pG2DReg->DST_BASE_ADDR = desc_surface->dwBaseaddr;
    
    m_pG2DReg->DST_COLOR_MODE = desc_surface->dwColorMode;
    
    m_pG2DReg->SC_HORI_RES = desc_surface->dwHoriRes;
    m_pG2DReg->SC_VERT_RES = desc_surface->dwVertRes;
}

/*
 *  @fn     void FIMGSE2D::TranslateCoordinateToZero(PSURFACE_DESCRIPTOR pdesc_surface, LPRECT prclDst)
 *  @brief  Adjust Coordinate from (x1,y1)~(x2,y2) to (0,0)~(x2-x1,y2-y1)
 *          Recalculate BaseAddress
 *  @param  desc_surface    Surface Information : Base Address, Horizontal&Vertical Resolution, Color mode
 *          prclDst         Surface Target Region coordinate
 *          prclCLip        Surface Clipping region coordinate
 */
void FIMGSE2D::TranslateCoordinateToZero(PSURFACE_DESCRIPTOR pdesc_surface, PRECTL prclDst, PRECTL prclClip)
{
    RECTL   rtNew;
    RECTL   rtNewClip;
    /// NewAddress = OriginalAddress + (Y1 * Hori.Res + X1) * BPP
    /// NewRect : NewRect.Left & top = (0,0), Right & bottom = (X2-X1, Y2-Y1)
    switch(pdesc_surface->dwColorMode)
    {
        case G2D_COLOR_RGB_565:
        case G2D_COLOR_RGBA_5551:
        case G2D_COLOR_ARGB_1555:
            /// 2Bytes per pixel
            pdesc_surface->dwBaseaddr = pdesc_surface->dwBaseaddr + (prclDst->top * pdesc_surface->dwHoriRes + prclDst->left) * 2;
        break;
        case G2D_COLOR_RGBA_8888:
        case G2D_COLOR_ARGB_8888:
        case G2D_COLOR_XRGB_8888:
        case G2D_COLOR_RGBX_8888:
            /// 4Bytes per pixel
            pdesc_surface->dwBaseaddr = pdesc_surface->dwBaseaddr + (prclDst->top * pdesc_surface->dwHoriRes + prclDst->left) * 4;
        break;
        default:
            RETAILMSG(DISP_ZONE_ERROR,(TEXT("[TCTZ] Unsupported Color Format\r\n")));
        break;
    }
    rtNew.left = 0;
    rtNew.top = 0;
    rtNew.right = prclDst->right - prclDst->left;
    rtNew.bottom = prclDst->bottom - prclDst->top;

    if(prclClip)
    {
        rtNewClip.left = 0;
        rtNewClip.top = 0;
        rtNewClip.right = prclClip->right - prclDst->left;
        rtNewClip.bottom = prclClip->bottom - prclDst->top;
        CopyRect((LPRECT)prclClip, (LPRECT) &rtNewClip);
    }
    
    CopyRect((LPRECT)prclDst, (LPRECT)&rtNew);
    pdesc_surface->dwVertRes -= prclDst->bottom - prclDst->top;
}


/**
*    Initialize 2D HW
*/
void FIMGSE2D::Init() 
{
    RequestEmptyFifo(4);
    
    /// Font Operation Related
    m_bIsBitBlt = true;
    m_bIsScr2Scr = false;
    DisableEffect(); // Disable per-pixel/per-plane alpha blending and fading
    SetColorKeyOff();

    m_pG2DReg->ALPHA = (FADING_OFFSET_DISABLE | ALPHA_VALUE_DISABLE);
    m_pG2DReg->ROP = (G2D_OPERAND3_FG_BIT | G2D_NO_ALPHA_MODE | OPAQUE_ENABLE | G2D_ROP_SRC_ONLY);
    SetRotationOrg(0, 0);
    m_pG2DReg->ROT_MODE = ROT_0;
    m_pG2DReg->ALPHA = 0;
}

void FIMGSE2D::PutPixel(DWORD uPosX, DWORD uPosY, DWORD uColor) //modification
{
    SetRotationMode(ROT_0);    
    
    RequestEmptyFifo(4);
    
    m_pG2DReg->COORD0_X = uPosX;
    m_pG2DReg->COORD0_Y = uPosY;
    m_pG2DReg->FG_COLOR = uColor;

    DoCmd(&(m_pG2DReg->CMDR0), G2D_REND_POINT_BIT, G2D_FASTRETURN);
}

/**
 * Draw Line
 * (usPosX1, usPosY1) ~ (usPosX2, usPosY2)
 * Do not draw last point
 *   0 < usPosX, usPosY1, usPosX2, usPosY2 < 2040
 * X-INCR is calculated by (End_X - Start_X)/ ABS(End_Y - Start Y), when Y-axis is the major axis(Y length>X length)
 * Y-INCR is calculated by (End_Y - Start_Y)/ ABS(End_X - Start X), When X-axis is the major axis
 */
#define INCR_INTEGER_PART_LENGTH    (11)
#define INCR_FRACTION_PART_LENGTH   (11)
void FIMGSE2D::PutLine(DWORD usPosX1, DWORD usPosY1, DWORD usPosX2, DWORD usPosY2, DWORD uColor, bool bIsDrawLastPoint) //modification
{
    int nMajorCoordX;
    DWORD uHSz, uVSz;
    int i;
    int nIncr=0;
    DWORD uCmdRegVal;

    SetRotationMode(ROT_0);        

    RequestEmptyFifo(7);

    RETAILMSG(DISP_ZONE_LINE,(TEXT("(%d,%d)~(%d,%d):Color:0x%x, LasT:%d\n"),
        usPosX1, usPosY1, usPosX2, usPosY2, uColor, bIsDrawLastPoint));

    m_pG2DReg->COORD0_X = usPosX1;
    m_pG2DReg->COORD0_Y = usPosY1;
    m_pG2DReg->COORD2_X = usPosX2;
    m_pG2DReg->COORD2_Y = usPosY2;

    /// Vertical Length
    uVSz = ABS((WORD)usPosY1 - (WORD)usPosY2);
    /// Horizontal Length
    uHSz = ABS((WORD)usPosX1 - (WORD)usPosX2);

    nMajorCoordX = (uHSz>=uVSz);

    if(nMajorCoordX)
    {
        // X length is longer than Y length
        // YINCR = (EY-SY)/ABS(EX-SX)
        for (i=0; i<INCR_FRACTION_PART_LENGTH+1; i++)
        {
            uVSz <<= 1;
            nIncr <<= 1;
            if (uVSz >= uHSz)
            {
                nIncr = nIncr | 1;
                uVSz -= uHSz;
            }
        }
        nIncr = (nIncr + 1) >> 1;
        if (usPosY1 > usPosY2)
        {
            nIncr = (~nIncr) + 1; // 2's complement
        }
        RETAILMSG(DISP_ZONE_LINE, (TEXT("YINCR: %x  "), nIncr ));
  }
    else
    {
        // Y length is longer than Y length    
        // XINCR = (EX-SX)/ABS(EY-SY)
        for (i=0; i<INCR_FRACTION_PART_LENGTH+1; i++)
        {
            uHSz <<= 1;
            nIncr <<= 1;
            if (uHSz >= uVSz)
            {
                nIncr = nIncr | 1;
                uHSz -= uVSz;
            }
        }
        nIncr = (nIncr + 1) >> 1;
        if (usPosX1 > usPosX2)
        {
            nIncr = (~nIncr) + 1; // 2's complement
        }
        RETAILMSG(DISP_ZONE_LINE, (TEXT("XINCR: %x  "), nIncr ));
    }

    m_pG2DReg->FG_COLOR = uColor;

    uCmdRegVal = 0;

    SetAlphaMode(G2D_NO_ALPHA_MODE);   //< Constant Alpha
    SetAlphaValue(0xff);
    

    if(nMajorCoordX)
    {
        SetYIncr(nIncr);

        uCmdRegVal =
            (bIsDrawLastPoint == true) ? (G2D_REND_LINE_BIT | G2D_MAJOR_COORD_X_BIT & G2D_DRAW_LAST_POINT_BIT) :
            (G2D_REND_LINE_BIT | G2D_MAJOR_COORD_X_BIT | G2D_NOT_DRAW_LAST_POINT_BIT);
        
        RETAILMSG(DISP_ZONE_LINE,(TEXT("m_pG2DReg:0x%x, CMD: %x, XINCR: %x, YINCR: %x\n"), m_pG2DReg, uCmdRegVal, m_pG2DReg->X_INCR, m_pG2DReg->Y_INCR ));
    }
    else
    {
        SetXIncr(nIncr);

        uCmdRegVal =
            (bIsDrawLastPoint == true) ? (G2D_REND_LINE_BIT | G2D_MAJOR_COORD_Y_BIT & G2D_DRAW_LAST_POINT_BIT) :
            (G2D_REND_LINE_BIT | G2D_MAJOR_COORD_Y_BIT | G2D_NOT_DRAW_LAST_POINT_BIT);

        RETAILMSG(DISP_ZONE_LINE,(TEXT("CMD: %x, XINCR: %x, YINCR: %x\n"), uCmdRegVal, m_pG2DReg->X_INCR, m_pG2DReg->Y_INCR ));
    }
    
    DoCmd(&(m_pG2DReg->CMDR0), uCmdRegVal, G2D_FASTRETURN);
}

/**
*    @fn    FIMGSE2D::DoCmd(DWORD *CmdRegister, DWORD CmdValue, DWORD CmdType)
*    @note  Submit the 2D Processing Command
*    @note  3 processing return Style is supported 
*           Asynchornous Return -> Not Recommended for all GDI call, very small transfer or just with DDraw
*           Synchornous Wait for interrupt -> Recommended for most case, Big Transfer
*           Synchrnous polling -> Recommended for very short process time, ex) LineDrawing, under tick(1 millisecond)
*/
BOOL FIMGSE2D::DoCmd(volatile UINT32 *CmdReg, DWORD CmdValue, G2D_CMDPROCESSING_TYPE eCmdType)
{
    DWORD bRetVal = 0;
    switch(eCmdType)
    {
    case G2D_FASTRETURN:
        RequestEmptyFifo(1);        
        *CmdReg = CmdValue;
        break;
        
    case G2D_INTERRUPT:
        RequestEmptyFifo(1);
        IntEnable();    
        
        *CmdReg = CmdValue;
        
        DispPerfBeginWait();        
        bRetVal = WaitForSingleObject(m_hInterrupt2D, 10000L);    //< Set Timeout as 10seconds.
        DispPerfEndWait();        
        
        if(bRetVal == WAIT_TIMEOUT)
        {
            RETAILMSG(DISP_ZONE_ERROR, (TEXT("2D Command take too long time. This command cannot be processed properly\r\n")));
            RETAILMSG(DISP_ZONE_ERROR, (TEXT("Reset 2D HW\r\n")));
            Reset();
        }

        IntDisable();    
        IntPendingClear();    

        InterruptDone(m_dwSysIntr2D);    
        break;
        
    case G2D_BUSYWAITING:
        RequestEmptyFifo(1);
        IntDisable();    

        *CmdReg = CmdValue;
        
        DispPerfBeginWait();
        WaitForIdleStatus();                        // Polling Style    
        DispPerfEndWait();
       
        break;
    default:
        RETAILMSG(DISP_ZONE_ERROR,(TEXT("CMDPROCESSING TYPE is invalid : %d\n"), eCmdType));
        return FALSE;
    }
    return TRUE;
}


/**
*    @fn    FIMGSE2D::SetRopEtype(G2D_ROP_TYPE eRopType)
*    @note    Set Ternary Raster Operation
*    @note    Only support 7 raster operation (most used Rop)
*/
void FIMGSE2D::SetRopEtype(G2D_ROP_TYPE eRopType)
{
    DWORD uRopVal;

    uRopVal =
        (eRopType == ROP_SRC_ONLY) ? G2D_ROP_SRC_ONLY :
        (eRopType == ROP_DST_ONLY) ? G2D_ROP_DST_ONLY :
        (eRopType == ROP_PAT_ONLY) ? G2D_ROP_PAT_ONLY :
        (eRopType == ROP_SRC_AND_DST) ? G2D_ROP_SRC_AND_DST:
        (eRopType == ROP_SRC_AND_PAT) ? G2D_ROP_SRC_AND_PAT :
        (eRopType == ROP_DST_AND_PAT) ? G2D_ROP_DST_AND_PAT :
        (eRopType == ROP_SRC_OR_DST) ? G2D_ROP_SRC_OR_DST :
        (eRopType == ROP_SRC_OR_PAT) ? G2D_ROP_SRC_OR_PAT :
        (eRopType == ROP_DST_OR_PAT) ? G2D_ROP_DST_OR_PAT :
        (eRopType == ROP_SRC_XOR_DST) ? G2D_ROP_SRC_XOR_DST :
        (eRopType == ROP_SRC_XOR_PAT) ? G2D_ROP_SRC_XOR_PAT :
        (eRopType == ROP_DST_XOR_PAT) ? G2D_ROP_DST_XOR_PAT :
         G2D_ROP_SRC_ONLY;

    SetRopValue(uRopVal);

}



void FIMGSE2D::SetStencilKey(DWORD uIsColorKeyOn, DWORD uIsInverseOn, DWORD uIsSwapOn)
{
    RequestEmptyFifo(1);
    m_pG2DReg->COLORKEY_CNTL = ((uIsColorKeyOn&1)<<31)|((uIsInverseOn&1)<<23)|(uIsSwapOn&1);
}

void FIMGSE2D::SetStencilMinMax(DWORD uRedMin, DWORD uRedMax, DWORD uGreenMin, DWORD uGreenMax, DWORD uBlueMin, DWORD uBlueMax)
{
    RequestEmptyFifo(2);
    m_pG2DReg->COLORKEY_DR_MIN = ((uRedMin&0xff)<<16)|((uGreenMin&0xff)<<8)|(uBlueMin&0xff);
    m_pG2DReg->COLORKEY_DR_MAX = ((0xffU<<24)|(uRedMax&0xff)<<16)|((uGreenMax&0xff)<<8)|(uBlueMax&0xff);
}

void FIMGSE2D::SetColorExpansionMethod(bool bIsScr2Scr)
{
    m_bIsScr2Scr  = bIsScr2Scr;
}

void FIMGSE2D::BlendingOut(DWORD uSrcData, DWORD uDstData, BYTE ucAlphaVal, bool bFading, BYTE ucFadingOffset, DWORD *uBlendingOut)
{

    DWORD uSrcRed, uSrcGreen, uSrcBlue;
    DWORD uDstRed, uDstGreen, uDstBlue;
    DWORD uBldRed, uBldGreen, uBldBlue;

    uSrcRed= (uSrcData & 0x00ff0000)>>16;  // Mask R
    uSrcGreen = (uSrcData & 0x0000ff00)>>8;     // Mask G
    uSrcBlue = uSrcData & 0x000000ff;         // Mask B

    uDstRed = (uDstData & 0x00ff0000)>>16; // Mask R
    uDstGreen = (uDstData & 0x0000ff00)>>8;  // Mask G
    uDstBlue = uDstData & 0x000000ff;         // Mask B

    if(bFading) {
        uBldRed= ((uSrcRed*(ucAlphaVal+1))>>8) + ucFadingOffset; // R output
        uBldGreen= ((uSrcGreen*(ucAlphaVal+1))>>8) + ucFadingOffset; // G output
        uBldBlue= ((uSrcBlue*(ucAlphaVal+1)>>8)) + ucFadingOffset; // B output
        if(uBldRed>=256) uBldRed=255;
        if(uBldGreen>=256) uBldGreen=255;
        if(uBldBlue>=256) uBldBlue=255;
    }
    else {
        uBldRed= ((uSrcRed*(ucAlphaVal+1)) + (uDstRed*(256-ucAlphaVal)))>>8; // R output
        uBldGreen= ((uSrcGreen*(ucAlphaVal+1)) + (uDstGreen*(256-ucAlphaVal)))>>8; // G output
        uBldBlue= ((uSrcBlue*(ucAlphaVal+1)) + (uDstBlue*(256-ucAlphaVal)))>>8; // B output
    }

    *uBlendingOut = (uBldRed<<16) | (uBldGreen<<8) | uBldBlue;
}


void FIMGSE2D::Convert24bpp(DWORD uSrcData, EGPEFormat eBpp, bool bSwap, DWORD *uConvertedData)
{

    DWORD uRed, uGreen, uBlue;

    switch(eBpp) {
        case  gpe8Bpp: // 15 bit color mode(ARGB:1555)
            if(bSwap == 1) {  // pde_state == 2(BitBlt)
                uRed = uSrcData & 0x00007c00;  // R
                uGreen = uSrcData & 0x000003e0;  // G
                uBlue = uSrcData & 0x0000001f;  // B
            
                *uConvertedData = uRed<<9 | uGreen<<6 | uBlue<<3; // SUM
            }
            else { //hsel = 0
                uRed = uSrcData & 0x7c000000;
                uGreen = uSrcData & 0x03e00000;
                uBlue = uSrcData & 0x001f0000;

                *uConvertedData = uRed>>7 | uGreen>>10 | uBlue>>13;
            } 
            break;
        case gpe16Bpp : // 16 bit color mode
            if(bSwap == 1) {
                uRed = uSrcData & 0x0000f800;
                uGreen = uSrcData & 0x000007e0;
                uBlue = uSrcData & 0x0000001f;

                *uConvertedData = uRed<<8 | uGreen<<5 | uBlue<<3;
            }
            else {
                uRed = uSrcData & 0xf8000000;
                uGreen = uSrcData & 0x07e00000;
                uBlue = uSrcData & 0x001f0000;

                *uConvertedData = uRed>>8 | uGreen>>11 | uBlue>>13;
            }
            break;    
        case gpe32Bpp : // 24 bit color mode
            *uConvertedData = uSrcData;
            break;
    } // End of switch
} // End of g2d_cvt24bpp function


void FIMGSE2D::GetRotateCoordinate(DWORD uDstX, DWORD uDstY, DWORD uOrgX, DWORD uOrgY, DWORD uRType, DWORD *uRsltX, DWORD *uRsltY)
{

    switch(uRType) {
        case  1 : // No Rotate. bypass.
            *uRsltX = uDstX;
            *uRsltY = uDstY;
            break;
        case  2 : // 90 degree Rotation
            *uRsltX = uOrgX + uOrgY - uDstY;
            *uRsltY = uDstX - uOrgX + uOrgY;
            break;
        case  4 : // 180 degree Rotation
            *uRsltX = 2*uOrgX - uDstX;
            *uRsltY = 2*uOrgY - uDstY;
            break;
        case  8 : // 270 degree Rotation
            *uRsltX = uDstY + uOrgX - uOrgY;
            *uRsltY = uOrgX + uOrgY - uDstX;
            break;
        case 16 : // X-flip
            *uRsltX = uDstX;
            *uRsltY = 2*uOrgY - uDstY;
            break;
        case 32 : // Y-flip
            *uRsltX = 2*uOrgX - uDstX;
            *uRsltY = uDstY;
            break;
        default :
            RETAILMSG(DISP_ZONE_ERROR, (TEXT("Invalid Rotation Type : %d"), uRType));
            break;
    }
}


void FIMGSE2D::WaitForIdle(void)
{
    DWORD dwRetries = 0;

    if(!(m_pG2DReg->FIFO_STATUS & G2D_DE_STATUS_FA_BIT))
    {
        IntEnable();    

        // In some cases, the interrupt does not fire on G2D idle even though it is enabled
        // So a loop with small enough timeout and a retry logic is used to catch such rare cases
        while(!(m_pG2DReg->FIFO_STATUS & G2D_DE_STATUS_FA_BIT) && dwRetries < G2D_INTERRUPT_TIMEOUT_RETRIES)
        {
            if(WAIT_TIMEOUT == WaitForSingleObject(m_hInterrupt2D, G2D_INTERRUPT_TIMEOUT_PER_TRY))
            {
                dwRetries++;
            }
        }

        if (G2D_INTERRUPT_TIMEOUT_RETRIES == dwRetries)
        {
            RETAILMSG(DISP_ZONE_ERROR, (TEXT("FIMGSE2D: 2D Command is taking more than %d ms. Bailing out.\r\n"), 
                G2D_INTERRUPT_TIMEOUT_RETRIES * G2D_INTERRUPT_TIMEOUT_PER_TRY));
        }

        IntDisable();    
        IntPendingClear();    
        
        InterruptDone(m_dwSysIntr2D); 
    }
}


BOOL FIMGSE2D::InitializeInterrupt(void)
{
    DWORD dwIRQ;

    dwIRQ = IRQ_2D;                    // 2D Accelerator IRQ
    m_dwSysIntr2D = SYSINTR_UNDEFINED;
    m_hInterrupt2D = NULL;

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dwIRQ, sizeof(DWORD), &m_dwSysIntr2D, sizeof(DWORD), NULL))
    {
        m_dwSysIntr2D = SYSINTR_UNDEFINED;
        return FALSE;
    }
    RETAILMSG(DISP_ZONE_INIT, (TEXT("2D Sysintr : %d\r\n"),m_dwSysIntr2D));

    m_hInterrupt2D = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == m_hInterrupt2D)
    {
        return FALSE;
    }

    if (!(InterruptInitialize(m_dwSysIntr2D, m_hInterrupt2D, 0, 0)))
    {
        return FALSE;
    }
    return TRUE;
}

void FIMGSE2D::DeinitInterrupt(void)
{
    if (m_dwSysIntr2D != SYSINTR_UNDEFINED)
    {
        InterruptDisable(m_dwSysIntr2D);
    }

    if (m_hInterrupt2D != NULL)
    {
        CloseHandle(m_hInterrupt2D);
    }

    if (m_dwSysIntr2D != SYSINTR_UNDEFINED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &m_dwSysIntr2D, sizeof(DWORD), NULL, 0, NULL);
    }
}
