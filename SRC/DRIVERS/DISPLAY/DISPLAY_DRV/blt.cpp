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

Module Name:    blt.cpp

Abstract:        accelerated bitblt/rectangle for S3C6410 FIMGSE-2D

Functions:


Notes:


--*/


#include "precomp.h"
#include <dispperf.h>

/// For Support Stretched Blt using HW
DDGPESurf *gpScratchSurf;
GPESurf *oldSrcSurf;

SCODE
S3C6410Disp::BltPrepare(GPEBltParms *pBltParms)
{
    RECTL rectl;
    int   iSwapTmp;
    BOOL  bRotate = FALSE;
    static bool bIsG2DReady = false;

    DEBUGMSG (GPE_ZONE_INIT, (TEXT("%s\r\n"), _T(__FUNCTION__)));

    DispPerfStart(pBltParms->rop4); //< This will be '0' when not defined DO_DISPPERF
    DispPerfParam(pBltParms);

    // default to base EmulatedBlt routine
    pBltParms->pBlt = &GPE::EmulatedBlt;        // catch all

    // see if we need to deal with cursor

    // check for destination overlap with cursor and turn off cursor if overlaps
    if (pBltParms->pDst == m_pPrimarySurface)    // only care if dest is main display surface
    {
        if (m_CursorVisible && !m_CursorDisabled)
        {
            if (pBltParms->prclDst != NULL)        // make sure there is a valid prclDst
            {
                rectl = *pBltParms->prclDst;        // if so, use it

                // There is no guarantee of a well ordered rect in blitParamters
                // due to flipping and mirroring.
                if(rectl.top > rectl.bottom)
                {
                    iSwapTmp     = rectl.top;
                    rectl.top    = rectl.bottom;
                    rectl.bottom = iSwapTmp;
                }
                if(rectl.left > rectl.right)
                {
                    iSwapTmp    = rectl.left;
                    rectl.left  = rectl.right;
                    rectl.right = iSwapTmp;
                }
            }
            else
            {
                rectl = m_CursorRect;                    // if not, use the Cursor rect - this forces the cursor to be turned off in this case
            }

            if (m_CursorRect.top <= rectl.bottom && m_CursorRect.bottom >= rectl.top &&
                m_CursorRect.left <= rectl.right && m_CursorRect.right >= rectl.left)
            {
                CursorOff();
                m_CursorForcedOff = TRUE;
            }
        }
    }

    // check for source overlap with cursor and turn off cursor if overlaps
    if (pBltParms->pSrc == m_pPrimarySurface)    // only care if source is main display surface
    {
        if (m_CursorVisible && !m_CursorDisabled)
        {
            if (pBltParms->prclSrc != NULL)        // make sure there is a valid prclSrc
            {
                rectl = *pBltParms->prclSrc;        // if so, use it
            }
            else
            {
                rectl = m_CursorRect;                    // if not, use the CUrsor rect - this forces the cursor to be turned off in this case
            }

            if (m_CursorRect.top < rectl.bottom && m_CursorRect.bottom > rectl.top &&
                m_CursorRect.left < rectl.right && m_CursorRect.right > rectl.left)
            {
                CursorOff();
                m_CursorForcedOff = TRUE;
            }
        }
    }

    // Fast Code entrance with Font Bitblt that not supported by other emulation code and acceleration hw
    // 
    if(pBltParms->rop4 == 0xAAF0)
    {
        return S_OK;
    }        
    if(pBltParms->pSrc && pBltParms->pSrc->IsRotate() ||
        pBltParms->pDst && pBltParms->pDst->IsRotate() )
    {
        bRotate = TRUE;
        pBltParms->pBlt = &GPE::EmulatedBltRotate;        // catch all
    }
    
    if( m_VideoPowerState != VideoPowerOff && // to avoid hanging while bring up display H/W
        m_G2DControlArgs.HWOnOff)
    {
        AcceleratedBltSelect(pBltParms);
        
        // Wait for the previous BLT operation to finish
        m_oG2D->WaitForIdle();  
    }

    if (pBltParms->pBlt != &GPE::EmulatedBlt &&
        pBltParms->pBlt != &GPE::EmulatedBltRotate)
    {
        // Some H/W Accelerated Function is assigned.
        // Save off the Src and Dst surface pointers that will be involved in the H/W BLT (for tracking during Lock and Delete)
        m_pLastSrcSurfUsingHW = pBltParms->pSrc;
        m_pLastDstSurfUsingHW = pBltParms->pDst;
    }
    else
    {
#if USE_SECEMUL_LIBRARY // To extract 2d_accel_lib.lib totally, using preprocess statement
        if( m_G2DControlArgs.UseSWAccel && !bRotate )
        {
            SECEmulatedBltSelect16(pBltParms);
            SECEmulatedBltSelect2416(pBltParms);
            SECEmulatedBltSelect1624(pBltParms);
       
            if(pBltParms->pBlt != (SCODE (GPE::*)(GPEBltParms *))&GPE::EmulatedBlt)
            {
                DispPerfType(DISPPERF_ACCEL_EMUL);
            }
        }
#endif
    }
    
    DEBUGMSG(GPE_ZONE_BLT_HI, (TEXT("--%s\r\n"), _T(__FUNCTION__)));
    return S_OK;
}



// This function would be used to undo the setting of clip registers etc
SCODE
S3C6410Disp::BltComplete(GPEBltParms *pBltParms)
{
    DEBUGMSG (GPE_ZONE_BLT_HI, (TEXT("++%s()\r\n"), _T(__FUNCTION__)));

    // see if cursor was forced off because of overlap with source or destination and turn back on
    if (m_CursorForcedOff)
    {
        m_CursorForcedOff = FALSE;

        if( m_VideoPowerState != VideoPowerOff && // to avoid hanging while bring up display H/W
            m_G2DControlArgs.HWOnOff)
        {        
            // For curser off scenario, wait for the BLT operation to finish before turning the cursor back on
            m_oG2D->WaitForIdle(); 

            // Since the previous H/W blit completed, we can reset the Last surface pointers
            m_pLastSrcSurfUsingHW = NULL;
            m_pLastDstSurfUsingHW = NULL;
        }

        CursorOn();
    }

    if(gpScratchSurf)
    {
        pBltParms->pSrc = oldSrcSurf;
        delete gpScratchSurf;
        gpScratchSurf=NULL;
    }

    DispPerfEnd(0);

    DEBUGMSG(GPE_ZONE_BLT_HI, (TEXT("--%s()\r\n"), _T(__FUNCTION__)));
    return S_OK;
}


/**
*   @fn SCODE S3C6410Disp::AcceleratedSolidFIll(GPEBltParms *pBltParms)
*   @brief  Rectangle Solid Filling Function. Solid Fill has no Source Rectangle
*   @param  pBltParms    Blit Parameter Information Structure
*   @sa     GPEBltParms
*   @note   ROP : 0xF0F0
*   @note   Using Information : DstSurface, ROP, Solidcolor
*   @note   SW ColorFill faster than HW ColorFill x2 times for 16Bpp
*/
SCODE S3C6410Disp::AcceleratedSolidFill(GPEBltParms *pBltParms)
{
    PRECTL    prclDst         = pBltParms->prclDst;
    DEBUGMSG(GPE_ZONE_BLT_LO, (TEXT("++%s()\r\n"), _T(__FUNCTION__)));

    /**
    *    Prepare Source & DestinationSurface Information
    */

    DWORD   dwTopStrideStartAddr = 0;    

    // When Screen is rotated, ScreenHeight and ScreenWidth always has initial surface property.
    m_descDstSurface.dwHoriRes = SURFACE_WIDTH(pBltParms->pDst);

    /// Set Destination Surface Information
    if(pBltParms->pDst->InVideoMemory() )
    {
        m_descDstSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + pBltParms->pDst->OffsetInVideoMemory());
        /// If surface is created by user temporary, that has no screen width and height.
        m_descDstSurface.dwVertRes = (pBltParms->pDst->ScreenHeight() != 0 ) ? pBltParms->pDst->ScreenHeight() : pBltParms->pDst->Height();    
    }
    else
    {
        dwTopStrideStartAddr = m_dwPhyAddrOfSurface[1] + pBltParms->prclDst->top * ABS(pBltParms->pDst->Stride());
        m_descDstSurface.dwBaseaddr = dwTopStrideStartAddr;
        m_descDstSurface.dwVertRes = RECT_HEIGHT(pBltParms->prclDst);
        pBltParms->prclDst->top = 0;
        pBltParms->prclDst->bottom = m_descDstSurface.dwVertRes;
    }

    m_oG2D->SetSrcSurface(&m_descDstSurface);     // Fill dummy value
    m_oG2D->SetDstSurface(&m_descDstSurface);

    if(pBltParms->prclClip)
    {
        m_oG2D->SetClipWindow(pBltParms->prclClip);
    }
    else
    {
        if(pBltParms->pDst->IsRotate())
        {
            RotateRectl(prclDst);
        }
        m_oG2D->SetClipWindow(prclDst);
    }
    m_oG2D->SetFgColor(pBltParms->solidColor);
    m_oG2D->Set3rdOperand(G2D_OPERAND3_FG);        
    switch(pBltParms->rop4 & 0xFF)
    {
        // Pat Copy
        case 0xF0:
            m_oG2D->SetRopEtype(ROP_PAT_ONLY);            
            break;
        // Pat Invert
        case 0x5A:
            m_oG2D->SetRopEtype(ROP_DST_XOR_PAT);            
            break;
    }
    EnterCriticalSection(&m_cs2D);
    m_oG2D->BitBlt(prclDst, prclDst, ROT_0);
    LeaveCriticalSection(&m_cs2D);

    if(pBltParms->pDst->IsRotate())
    {
        RotateRectlBack(prclDst);
    }

    DEBUGMSG(GPE_ZONE_BLT_LO, (TEXT("--%s()\r\n"), _T(__FUNCTION__)));

    return    S_OK;
}

/**
*   @fn SCODE S3C6410Disp::AcceleratedPatFIll(GPEBltParms *pBltParms)
*   @brief  Rectangle Pattern Filling Function. only 8x8x16bpp pattern
*   @param  pBltParms    Blit Parameter Information Structure
*   @sa     GPEBltParms
*   @note   ROP : 0xF0F0, 0x5A5A
*   @note   Using Information : DstSurface, ROP, Pattern Brush
*/
// TODO: BitBlt with Pattern memory when used with small size pattern
SCODE S3C6410Disp::AcceleratedPatFill(GPEBltParms *pBltParms)
{
    PRECTL    prclDst         = pBltParms->prclDst;
    DEBUGMSG(GPE_ZONE_BLT_LO, (TEXT("++%s()\r\n"), _T(__FUNCTION__)));

    /**
    *   Prepare Source & DestinationSurface Information
    */

    DWORD   dwTopStrideStartAddr = 0;    

    // When Screen is rotated, ScreenHeight and ScreenWidth always has initial surface property.
    m_descDstSurface.dwHoriRes = SURFACE_WIDTH(pBltParms->pDst);
    /// Set Destination Surface Information
    if(pBltParms->pDst->InVideoMemory() )
    {
        m_descDstSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + pBltParms->pDst->OffsetInVideoMemory());
        /// If surface is created by user temporary, that has no screen width and height.
        m_descDstSurface.dwVertRes = (pBltParms->pDst->ScreenHeight() != 0 ) ? pBltParms->pDst->ScreenHeight() : pBltParms->pDst->Height();    
    }
    else
    {
        dwTopStrideStartAddr = m_dwPhyAddrOfSurface[1] + pBltParms->prclDst->top * ABS(pBltParms->pDst->Stride());
        m_descDstSurface.dwBaseaddr = dwTopStrideStartAddr;
        m_descDstSurface.dwVertRes = RECT_HEIGHT(pBltParms->prclDst);
        pBltParms->prclDst->top = 0;
        pBltParms->prclDst->bottom = m_descDstSurface.dwVertRes;
    }

    m_oG2D->SetSrcSurface(&m_descDstSurface);     // Fill dummy value
    m_oG2D->SetDstSurface(&m_descDstSurface);

    if(pBltParms->prclClip)
    {
        m_oG2D->SetClipWindow(pBltParms->prclClip);
    }
    else
    {
        if(pBltParms->pDst->IsRotate())
        {
            RotateRectl(prclDst);
        }
        m_oG2D->SetClipWindow(prclDst);
    }
    
    m_oG2D->SetFgColor(pBltParms->solidColor);
    m_oG2D->Set3rdOperand(G2D_OPERAND3_FG);        
    switch(pBltParms->rop4 & 0xFF)
    {
        // Pat Copy
        case 0xF0:
            m_oG2D->SetRopEtype(ROP_PAT_ONLY);            
            break;
        // Pat Invert
        case 0x5A:
            m_oG2D->SetRopEtype(ROP_DST_XOR_PAT);            
            break;
    }
    EnterCriticalSection(&m_cs2D);
    m_oG2D->BitBlt(prclDst, prclDst, ROT_0);
    LeaveCriticalSection(&m_cs2D);

    if(pBltParms->pDst->IsRotate())
    {
        RotateRectlBack(prclDst);
    }

    DEBUGMSG(GPE_ZONE_BLT_LO, (TEXT("--%s()\r\n"), _T(__FUNCTION__)));

    return  S_OK;
}

// This function will be supported in the future
SCODE S3C6410Disp::AcceleratedDestInvert(GPEBltParms *pBltParms)
{
    int dstX   = pBltParms->prclDst->left;
    int dstY   = pBltParms->prclDst->top;
    int width  = RECT_WIDTH(pBltParms->prclDst);
    int height = RECT_HEIGHT(pBltParms->prclDst);

    return  S_OK;
}

// Do NOTHING.
SCODE S3C6410Disp::NullBlt(GPEBltParms *pBltParms)
{
    return S_OK;
}

/// From Frame Buffer to Frame Buffer Directly
/// Constraints
/// Source Surface's width is same with Destination Surface's width.
/// Source and Dest must be in Video FrameBuffer Region
/// In Surface Format
/// ScreenHeight and ScreenWidth means logical looking aspect for application
/// Height and Width means real data format.
/**
*   @fn     SCODE S3C6410Disp::AcceleratedSrcCopyBlt(GPEBltParms *pBltParms)
*   @brief  Do Blit with SRCCOPY, SRCAND, SRCPAINT, SRCINVERT
*   @param  pBltParms    Blit Parameter Information Structure
*   @sa     GPEBltParms
*   @note   ROP : 0xCCCC(SRCCOPY), 0x8888(SRCAND), 0x6666(SRCINVERT), 0XEEEE(SRCPAINT)
*   @note   Using Information : DstSurface, ROP, Solidcolor
*/
SCODE S3C6410Disp::AcceleratedSrcCopyBlt (GPEBltParms *pBltParms)
{
    PRECTL prclSrc, prclDst;
    RECT rectlSrcBackup;
    RECT rectlDstBackup;
    BOOL bHWSuccess = FALSE;
    prclSrc = pBltParms->prclSrc;
    prclDst = pBltParms->prclDst;

    // Set Destination Offset In Video Memory, this point is Dest lefttop point
    /**
    *   Prepare Source & Destination Surface Information
    */
    DWORD   dwTopStrideStartAddr = 0;

    /// !!!!Surface Width may not match to Real Data format!!!!
    /// !!!!Set Width by Scan Stride Size!!!!
    m_descSrcSurface.dwHoriRes = SURFACE_WIDTH(pBltParms->pSrc);
    m_descDstSurface.dwHoriRes = SURFACE_WIDTH(pBltParms->pDst);

    if(pBltParms->pDst->IsRotate())
    {
        RotateRectl(prclDst);   //< RotateRectl rotate rectangle with screen rotation information
        if(pBltParms->prclClip)
        {
            RotateRectl(pBltParms->prclClip);
        }
    }
    if(pBltParms->pSrc->IsRotate())
    {
        RotateRectl(prclSrc);
    }

    if (pBltParms->bltFlags & BLT_TRANSPARENT)
    {
        RETAILMSG(0,(TEXT("TransparentMode Color : %d\n"), pBltParms->solidColor));
        // turn on transparency & set comparison color
        m_oG2D->SetTransparentMode(1, pBltParms->solidColor);
    }

    switch (pBltParms->rop4)
    {
    case    0x6666: // SRCINVERT
        RETAILMSG(DISP_ZONE_2D, (TEXT("SRCINVERT\r\n")));
        m_oG2D->SetRopEtype(ROP_SRC_XOR_DST);
        break;

    case    0x8888: // SRCAND
        RETAILMSG(DISP_ZONE_2D, (TEXT("SRCAND\r\n")));
        m_oG2D->SetRopEtype(ROP_SRC_AND_DST);
        break;

    case    0xCCCC: // SRCCOPY
        RETAILMSG(DISP_ZONE_2D, (TEXT("SRCCOPY\r\n")));
        m_oG2D->SetRopEtype(ROP_SRC_ONLY);
        break;

    case    0xEEEE: // SRCPAINT
        RETAILMSG(DISP_ZONE_2D, (TEXT("SRCPAINT\r\n")));
        m_oG2D->SetRopEtype(ROP_SRC_OR_DST);
        break;
    }


    /// Check Source Rectangle Address
    /// HW Coordinate limitation is 2040
    /// 1. Get the Top line Start Address
    /// 2. Set the base offset to Top line Start Address
    /// 3. Recalulate top,bottom rectangle
    /// 4. Do HW Bitblt

    CopyRect(&rectlSrcBackup, (LPRECT)pBltParms->prclSrc);
    CopyRect(&rectlDstBackup, (LPRECT)pBltParms->prclDst);
    /// Destination's Region can have negative coordinate, especially for left, top point
    /// In this case, For both destination, source's rectangle must be clipped again to use HW.
    ClipDestDrawRect(pBltParms);
    /// Set Source Surface Information
    if(pBltParms->pSrc == pBltParms->pDst)  // OnScreen BitBlt
    {
        if((pBltParms->pSrc)->InVideoMemory() )
        {
            m_descSrcSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + pBltParms->pDst->OffsetInVideoMemory());
            /// If surface is created by user temporary, that has no screen width and height.
            m_descSrcSurface.dwVertRes = (pBltParms->pSrc->ScreenHeight() != 0 ) ? pBltParms->pSrc->ScreenHeight() : pBltParms->pSrc->Height();
            m_descDstSurface.dwBaseaddr = m_descSrcSurface.dwBaseaddr;
            m_descDstSurface.dwVertRes = m_descSrcSurface.dwVertRes;
        } 
        else
        {
            dwTopStrideStartAddr = m_dwPhyAddrOfSurface[0];
            m_descSrcSurface.dwBaseaddr = dwTopStrideStartAddr;
            m_descSrcSurface.dwVertRes = (pBltParms->pSrc->ScreenHeight() != 0 ) ? pBltParms->pSrc->ScreenHeight() : pBltParms->pSrc->Height();
            m_descDstSurface.dwBaseaddr = m_descSrcSurface.dwBaseaddr;
            m_descDstSurface.dwVertRes = m_descSrcSurface.dwVertRes;
        }
    }
    else        // OffScreen BitBlt
    {
        if(pBltParms->pSrc->InVideoMemory() )
        {
            m_descSrcSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + pBltParms->pSrc->OffsetInVideoMemory());
            /// If surface is created by user temporary, that has no screen width and height.
            m_descSrcSurface.dwVertRes = (pBltParms->pSrc->ScreenHeight() != 0 ) ? pBltParms->pSrc->ScreenHeight() : pBltParms->pSrc->Height();            
        } 
        else
        {
            dwTopStrideStartAddr = m_dwPhyAddrOfSurface[0] + pBltParms->prclSrc->top * ABS(pBltParms->pSrc->Stride());
            m_descSrcSurface.dwBaseaddr = dwTopStrideStartAddr;
            m_descSrcSurface.dwVertRes = RECT_HEIGHT(pBltParms->prclSrc);

            pBltParms->prclSrc->top = 0;
            pBltParms->prclSrc->bottom = m_descSrcSurface.dwVertRes;
        }

        /// Set Destination Surface Information
        if(pBltParms->pDst->InVideoMemory() )
        {
            m_descDstSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + pBltParms->pDst->OffsetInVideoMemory());
            /// If surface is created by user temporary, that has no screen width and height.
            m_descDstSurface.dwVertRes = (pBltParms->pDst->ScreenHeight() != 0 ) ? pBltParms->pDst->ScreenHeight() : pBltParms->pDst->Height();    
        }
        else
        {
            dwTopStrideStartAddr = m_dwPhyAddrOfSurface[1] + pBltParms->prclDst->top * ABS(pBltParms->pDst->Stride());
            m_descDstSurface.dwBaseaddr = dwTopStrideStartAddr;
            m_descDstSurface.dwVertRes = RECT_HEIGHT(pBltParms->prclDst);
            pBltParms->prclDst->top = 0;
            pBltParms->prclDst->bottom = m_descDstSurface.dwVertRes;
        }
    }

    /// Transparency does not relate with alpha blending
    m_oG2D->SetAlphaMode(G2D_NO_ALPHA_MODE);
    /// No transparecy with alphablend
    m_oG2D->SetAlphaValue(0xff);
    m_oG2D->Set3rdOperand(G2D_OPERAND3_PAT);

    /// Real Register Surface Description setting will be done in HWBitBlt
    bHWSuccess = HWBitBlt(pBltParms, &m_descSrcSurface, &m_descDstSurface);

    CopyRect((LPRECT)pBltParms->prclSrc, &rectlSrcBackup);
    CopyRect((LPRECT)pBltParms->prclDst, &rectlDstBackup);

    if(pBltParms->pDst->IsRotate())
    {
        RotateRectlBack(prclDst);
        if(pBltParms->prclClip)
        {
            RotateRectlBack(pBltParms->prclClip);
        }
    }
    if(pBltParms->pSrc->IsRotate())
    {
        RotateRectlBack(prclSrc);
    }
    if (pBltParms->bltFlags & BLT_TRANSPARENT)
    {
        m_oG2D->SetTransparentMode(0, pBltParms->solidColor);   // turn off Transparency
    }
    if(!bHWSuccess)
    {
        return EmulatedBlt(pBltParms);
    }

    return    S_OK;
}


/**
*   @fn     void S3C6410Disp::AcceleratedBltSelect(GpeBltParms *pBltParms)
*   @brief  Select appropriate hardware acceleration function or software emulation function. 
*           if there's no appropriate accelerated function,
*           Leave Blit funciton to intial setting, EmulatedBlt(generic Bit blit emulator)
*   @param  pBltParms   Blit Parameter Information Structure
*   @sa     GPEBltParms
*/
SCODE S3C6410Disp::AcceleratedBltSelect(GPEBltParms *pBltParms)
{

    if ((pBltParms->pBlt != (SCODE (GPE::*)(GPEBltParms *))&GPE::EmulatedBlt) && !m_G2DControlArgs.OverrideEmulFunc)    
    {
        if(pBltParms->pSrc && pBltParms->pDst)// && (pBltParms->pSrc->Rotate() != pBltParms->pDst->Rotate())) // Do not use in ColorFill
        {
            // In this case we will use HW, the rotation case
            RETAILMSG(FALSE,(TEXT("AlreadySelected But Rotation:0x%x\n"),pBltParms->pBlt));            
        }
        else
        {
            // Already Some Accelerated or Emulated Function is assigned.
            RETAILMSG(FALSE,(TEXT("AlreadySelected:0x%x\n"),pBltParms->pBlt));
            return S_OK;    
        }
    }

    if((pBltParms->rop4 != 0xCCCC)
        && (pBltParms->rop4 != 0xEEEE)
        && (pBltParms->rop4 != 0x6666)
        && (pBltParms->rop4 != 0x8888)
        && (pBltParms->rop4 != 0xF0F0)
        && (pBltParms->rop4 != 0x5A5A)
        )
    {
        return S_OK;
    }

    if(pBltParms->rop4 == 0xF0F0 && !m_G2DControlArgs.UseFillRect)
    {
        return S_OK;
    }

    if(!m_G2DControlArgs.UseStretchBlt && (pBltParms->bltFlags & BLT_STRETCH))       //< can support Stretch Blit
    {
        return S_OK;
    }

    if(pBltParms->pLookup)
    {
        return S_OK;
    }

    if(!m_G2DControlArgs.UseAlphaBlend && (pBltParms->bltFlags & BLT_ALPHABLEND))
    {
        return S_OK;
    }

    if(pBltParms->pDst)
    {
        m_descDstSurface.dwColorMode = GetHWColorFormat(pBltParms->pDst);
        if(m_descDstSurface.dwColorMode == G2D_COLOR_UNUSED)
        {
            RETAILMSG(DISP_ZONE_2D, (TEXT("2D HW does not support this color format\r\n")));
            return S_OK;
        }
    }

    if(pBltParms->pSrc)
    {
        m_descSrcSurface.dwColorMode = GetHWColorFormat(pBltParms->pSrc);
        if(m_descSrcSurface.dwColorMode == G2D_COLOR_UNUSED)
        {   
            RETAILMSG(DISP_ZONE_2D, (TEXT("2D HW does not support this color format\r\n")));
            return S_OK;
        }
    }


    if(pBltParms->bltFlags & BLT_ALPHABLEND)    //< Our HW can support AlphaBlend blit for 16bpp, 32bpp
    {
        //DumpBltParms(pBltParms);
        
        if(!pBltParms->pSrc)
        {
            return S_OK;
        }
        if(!(( &pBltParms->blendFunction != 0) && ( pBltParms->blendFunction.BlendFlags == 0) ) )
        {
            RETAILMSG(DISP_ZONE_TEMP, (TEXT("AlphaBlend BitBlt with HW request is rejected.\r\n")));
            RETAILMSG(DISP_ZONE_TEMP, (TEXT("SrcFormat:%d, BlendFunction : 0x%x Op(%d), Flag(%d),SourceConstantAlpha(%d),AlphaFormat(%d)\r\n"),
                pBltParms->pSrc->Format(),
                pBltParms->blendFunction, 
                pBltParms->blendFunction.BlendOp,
                pBltParms->blendFunction.BlendFlags, 
                pBltParms->blendFunction.SourceConstantAlpha, pBltParms->blendFunction.AlphaFormat));    
            
            return S_OK;
        }
#if G2D_BYPASS_2STEP_PROCESS_PPA_AFTER_SCA
        if(pBltParms->blendFunction.SourceConstantAlpha != 0xFF && pBltParms->blendFunction.AlphaFormat == 1)
        {
            return S_OK;
        }
#endif
#if G2D_BYPASS_SOURCECONSTANT_ALPHABLEND        // 
        if(pBltParms->blendFunction.SourceConstantAlpha != 0xFF)
        {
            return S_OK;
        }
#endif
#if G2D_BYPASS_PERPIXEL_ALPHABLEND
        if(pBltParms->blendFunction.AlphaFormat == 1)
        {
            return S_OK;
        }
#endif

    }

#if G2D_BYPASS_ALPHADIBTEST
    if(!(pBltParms->bltFlags & BLT_ALPHABLEND))
    {
        if(m_descSrcSurface.dwColorMode == G2D_COLOR_ARGB_8888) // We cannot do this
        {
            return S_OK;
#if 0
            m_descSrcSurface.dwColorMode = G2D_COLOR_XRGB_8888; // Force to change
            RETAILMSG(DISP_ZONE_2D, (TEXT("Src change from ARGB to XRGB.\r\n")));            
#endif
        }
        if(m_descDstSurface.dwColorMode == G2D_COLOR_ARGB_8888) // We cannot do this
        {
            return S_OK;
#if 0
            m_descDstSurface.dwColorMode = G2D_COLOR_XRGB_8888; // Force to change
            RETAILMSG(DISP_ZONE_2D, (TEXT("Dst change from ARGB to XRGB.\r\n")));
#endif
        }
#if 0
        if(m_descSrcSurface.dwColorMode == G2D_COLOR_ARGB_1555) // We cannot do this
        {
            return S_OK;
        }
        if(m_descDstSurface.dwColorMode == G2D_COLOR_ARGB_1555) // We cannot do this
        {
            return S_OK;
        }
#endif
    }
#endif

    if(pBltParms->pConvert)     //< Emulate if color conversion required
    {
        if(!pBltParms->pSrc)
        {
            return S_OK;
        }
    }

    if(pBltParms->prclClip && (pBltParms->prclClip->left == pBltParms->prclClip->right) && (pBltParms->prclClip->top == pBltParms->prclClip->bottom))
    {
        // Just skip, there is no image flushing to screen
        // SW bitblt takes this case, and it can skip more efficiently.
        return S_OK;
    }

    /// Odd case do nothing    
    if(pBltParms->pSrc && pBltParms->prclSrc)
    {
        if((pBltParms->prclSrc->right == pBltParms->prclSrc->left) || (pBltParms->prclSrc->bottom == pBltParms->prclSrc->top))
        {
            return S_OK;
        }
    }

    if(pBltParms->pDst && pBltParms->prclDst)
    {
        /// Odd case do nothing
        if ((pBltParms->prclDst->right == pBltParms->prclDst->left) || (pBltParms->prclDst->bottom == pBltParms->prclDst->top))
        {
            return S_OK;
        }    
    }

    /**
    *    Check if source and destination regions' coordinates has positive value.
    *
    *
    **/
    if ((pBltParms->bltFlags & BLT_STRETCH))            // Stretch Bllitting with X or Y axis mirroring
    {
        if(!pBltParms->prclDst || !pBltParms->prclSrc)
        {
            return S_OK;
        }
        else
        {
            if ((pBltParms->prclDst->left < 0) || (pBltParms->prclDst->right <0 ) || (pBltParms->prclDst->top <0 ) || (pBltParms->prclDst->bottom <0))
            {
                return S_OK;
            }
            if ((pBltParms->prclSrc->left < 0) || (pBltParms->prclSrc->right <0 ) || (pBltParms->prclSrc->top <0 ) || (pBltParms->prclSrc->bottom <0))
            {
                return S_OK;
            }
        }
        
        if(pBltParms->pDst && ((RECT_HEIGHT(pBltParms->prclDst) > pBltParms->pDst->Height()) || (RECT_WIDTH(pBltParms->prclDst) > pBltParms->pDst->Width())))
        {
            return S_OK;
        }
    }
    
    /// Prevent Condition Check Fail Case
    m_dwPhyAddrOfSurface[0] = NULL;
    m_dwPhyAddrOfSurface[1] = NULL;

    // select accelerated function based on rop value
    switch (pBltParms->rop4)
    {
    case 0xCCCC:    // SRCCOPY
        /// There are no image to draw.
        if( pBltParms->prclDst &&
            pBltParms->pDst &&
            (pBltParms->prclDst->right < 0 ||
             pBltParms->prclDst->bottom < 0 ||
             pBltParms->prclDst->top > pBltParms->pDst->Height() ||
             pBltParms->prclDst->left > SURFACE_WIDTH(pBltParms->pDst))
             )
        {
            return S_OK;
        }
      
        if( pBltParms->prclDst && 
            pBltParms->pSrc &&
            pBltParms->pDst &&
             (SURFACE_WIDTH(pBltParms->pDst) < MAX_2DHW_WIDTH) &&
             (SURFACE_WIDTH(pBltParms->pSrc) < MAX_2DHW_WIDTH) 
#if G2D_BLT_OPTIMIZE    
            &&    (ABS(RECT_WIDTH(pBltParms->prclSrc))*ABS(RECT_HEIGHT(pBltParms->prclSrc)*BYTES_PER_PIXEL(pBltParms->pSrc)) > G2D_COMPROMISE_LIMIT)
#endif
        )
        {
            if( ( (pBltParms->pSrc)->InVideoMemory()
                    || (m_G2DControlArgs.CachedBlt && (m_dwPhyAddrOfSurface[0] = ValidatePAContinuityOfSurf(pBltParms->pSrc))) )
                    &&
                    ( pBltParms->pDst->InVideoMemory()
                    || (m_G2DControlArgs.CachedBlt && (m_dwPhyAddrOfSurface[1] = ValidatePAContinuityOfSurf(pBltParms->pDst))) )
                    && (ABS(RECT_HEIGHT(pBltParms->prclSrc)) < MAX_2DHW_HEIGHT)
                    && (ABS(RECT_WIDTH(pBltParms->prclSrc)) < MAX_2DHW_WIDTH)                    
                )
                {
                // negative Stride for Alaphblend will be treated on HWBitBlt as Flip after Copy                
                if(pBltParms->bltFlags & BLT_ALPHABLEND)
                {
                    DispPerfType(DISPPERF_ACCEL_HARDWARE);
                    pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                }
                else if(pBltParms->pSrc->Stride() > 0)
                {
                    DispPerfType(DISPPERF_ACCEL_HARDWARE);
                    pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedSrcCopyBlt;
                }
                else if(pBltParms->pDst->Rotate() != DMDO_0 &&
                        pBltParms->pSrc->Rotate() != pBltParms->pDst->Rotate())  // DMDO_0=0, DMDO_90=1, DMDO_180=2, DMDO_270=4
                {
                    // Copy&Flip Whole Source Image into Temporary area
                    // BitBlt from temporary area to Screen, This can treat Negative Stride Case.
                    DispPerfType(DISPPERF_ACCEL_HARDWARE);
                    pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                }                
                return S_OK;
            }
            else
            {
#ifdef HWBLIT_CLIPPING_ISSUE_WORKAROUND
                // Surfaces are not in video memory, and not contiguous, so no hardware acceleration.
                // Implementing hardware acceleration requires creating a scratch surface and memcpy'ing the contents
                // for either or both surfaces, which negates the benefit of any HW acceleration in most cases.
                // However, for Alpha and Stretch BLTs, it is still faster to do it in H/W incurring the memcpy cost.
                // There is a problem with in the H/W code path for scratch surface creation regarding calculation of 
                // clipping regions and so disabling the H/W path even for Alpha and Stretch BLTs
#else
                if( (pBltParms->pSrc)->InVideoMemory() ){
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("INVideo:SRCCOPY!!!!!!\r\n")));
                }
                else    // Source is not in video memory, and not contiguous
                {
                    // negative Stride for Alaphblend will be treated on HWBitBlt as Flip after Copy                
                    if(pBltParms->bltFlags & BLT_ALPHABLEND)
                    {
                        //DumpBltParms(pBltParms);
                        DispPerfType(DISPPERF_ACCEL_HARDWARE);
                        pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                    }
                    /// If Stretch is needed, HW is more efficient. 
                    /// so in this case, copy va to pa's scratched buffer, then use HW
                    else if(pBltParms->bltFlags & BLT_STRETCH)
                    {
                        DispPerfType(DISPPERF_ACCEL_HARDWARE);
                        pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                    }
                    else if(pBltParms->pDst->Rotate() != DMDO_0 &&
                        pBltParms->pSrc->Rotate() != pBltParms->pDst->Rotate())  // DMDO_0=0, DMDO_90=1, DMDO_180=2, DMDO_270=4
                    {
                        // Copy&Flip Whole Source Image into Temporary area
                        // BitBlt from temporary area to Screen, This can treat Negative Stride Case.
                        DispPerfType(DISPPERF_ACCEL_HARDWARE);
                        pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                    }                
                    // Other case we cannot treat, maybe that's just SRCCOPY without any rotation, alphablend, stretch
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("NotInVideo:SRCCOPY\r\n")));                
                }
#endif
            }
        }
        return S_OK;
    case 0x6666:    // SRCINVERT        
    case 0x8888:    // SRCAND
    case 0xEEEE:    // SRCPAINT
        if( pBltParms->prclDst &&
            ((pBltParms->prclDst->left >= 0) &&
            (pBltParms->prclDst->top >= 0) &&
            (pBltParms->prclDst->right >= 0 ) &&
            (pBltParms->prclDst->bottom >= 0 )) &&
            pBltParms->pSrc &&
            pBltParms->pDst &&
             (SURFACE_WIDTH(pBltParms->pDst) < MAX_2DHW_WIDTH) &&
             (SURFACE_WIDTH(pBltParms->pSrc) < MAX_2DHW_WIDTH) 
#if G2D_BLT_OPTIMIZE    
            &&    (ABS(RECT_WIDTH(pBltParms->prclSrc))*ABS(RECT_HEIGHT(pBltParms->prclSrc)*BYTES_PER_PIXEL(pBltParms->pSrc)) > G2D_COMPROMISE_LIMIT)
#endif
        )    
        {
            if( ( (pBltParms->pSrc)->InVideoMemory()
                    || (m_G2DControlArgs.CachedBlt && (m_dwPhyAddrOfSurface[0] = ValidatePAContinuityOfSurf(pBltParms->pSrc))) )
                    &&
                    ( pBltParms->pDst->InVideoMemory()
                    || (m_G2DControlArgs.CachedBlt && (m_dwPhyAddrOfSurface[1] = ValidatePAContinuityOfSurf(pBltParms->pDst))) )
                    && (ABS(RECT_HEIGHT(pBltParms->prclSrc)) < MAX_2DHW_HEIGHT)
                    && (ABS(RECT_WIDTH(pBltParms->prclSrc)) < MAX_2DHW_WIDTH)                
                )
                {
                // negative Stride for Alaphblend will be treated on HWBitBlt as Flip after Copy                
                if(pBltParms->bltFlags & BLT_ALPHABLEND)
                {
                    DispPerfType(DISPPERF_ACCEL_HARDWARE);
                    pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                }
                else if(pBltParms->pSrc->Stride() > 0)
                {
                    DispPerfType(DISPPERF_ACCEL_HARDWARE);
                    pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedSrcCopyBlt;
                }
                else if(pBltParms->pDst->Rotate() != DMDO_0 &&
                        pBltParms->pSrc->Rotate() != pBltParms->pDst->Rotate())  // DMDO_0=0, DMDO_90=1, DMDO_180=2, DMDO_270=4
                {
                    // Copy&Flip Whole Source Image into Temporary area
                    // BitBlt from temporary area to Screen
                    DispPerfType(DISPPERF_ACCEL_HARDWARE);
                    pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                }
                return S_OK;
            }
            else
            {
#ifdef HWBLIT_CLIPPING_ISSUE_WORKAROUND
                // Surfaces are not in video memory, and not contiguous, so no hardware acceleration.
                // Implementing hardware acceleration requires creating a scratch surface and memcpy'ing the contents
                // for either or both surfaces, which negates the benefit of any HW acceleration in most cases.
                // However, for Alpha and Stretch BLTs, it is still faster to do it in H/W incurring the memcpy cost.
                // There is a problem with in the H/W code path for scratch surface creation regarding calculation of 
                // clipping regions and so disabling the H/W path even for Alpha and Stretch BLTs
#else
                if( (pBltParms->pSrc)->InVideoMemory() ){
                    RETAILMSG(FALSE,(TEXT("INVideo:ROP:0x%x\r\n"), pBltParms->rop4));
                }
                else
                {
                    // negative Stride for Alaphblend will be treated on HWBitBlt as Flip after Copy                
                    if(pBltParms->bltFlags & BLT_ALPHABLEND)
                    {
                        DispPerfType(DISPPERF_ACCEL_HARDWARE);
                        pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedAlphaSrcCopyBlt;
                    }
                    RETAILMSG(FALSE,(TEXT("NotInVideo:ROP:0x%x\r\n"), pBltParms->rop4));
                }
#endif
            }
        }
        return S_OK;
    case 0xF0F0:    // PATCOPY
    case 0x5A5A:    // PATINVERT    
        if( pBltParms->solidColor != -1)    // must be a solid colored brush
        {
            if(pBltParms->prclDst &&
                pBltParms->pDst &&               
                ( (pBltParms->prclDst->left >= 0) && (pBltParms->prclDst->left < MAX_2DHW_XCOORD)  &&
                  (pBltParms->prclDst->top >= 0) && (pBltParms->prclDst->top < MAX_2DHW_YCOORD) &&
                  (pBltParms->prclDst->right >= 0 ) && (pBltParms->prclDst->right < MAX_2DHW_YCOORD) &&
                  (pBltParms->prclDst->bottom >= 0 ) && (pBltParms->prclDst->bottom < MAX_2DHW_YCOORD)  ) &&
                 (SURFACE_WIDTH(pBltParms->pDst) < MAX_2DHW_WIDTH) 
#if G2D_BLT_OPTIMIZE                     
            &&    (ABS(RECT_WIDTH(pBltParms->prclDst))*ABS(RECT_HEIGHT(pBltParms->prclDst)*BYTES_PER_PIXEL(pBltParms->pDst)) > G2D_COMPROMISE_LIMIT)
#endif
             )
            {
                if( ( pBltParms->pDst->InVideoMemory()
                        || (m_G2DControlArgs.CachedBlt && (m_dwPhyAddrOfSurface[1] = ValidatePAContinuityOfSurf(pBltParms->pDst))) )
                        && (ABS(RECT_HEIGHT(pBltParms->prclDst)) < MAX_2DHW_HEIGHT)
                        && (ABS(RECT_WIDTH(pBltParms->prclDst)) < MAX_2DHW_WIDTH)                    
                   )
                {
                        DumpBltParms(pBltParms);
                        DispPerfType(DISPPERF_ACCEL_HARDWARE);        
                        pBltParms->pBlt = (SCODE (GPE::*)(struct GPEBltParms *)) &S3C6410Disp::AcceleratedSolidFill;
                        return S_OK;
                }
            }
        }
        return S_OK;
    default:        // unsupported ROP4 to accelerate
        return S_OK;
    }
    return S_OK;
}


#define PFN_SHIEFT UserKInfo[KINX_PFN_SHIFT]
#define MAX_SUPPORTED_PFN (MAXDWORD>>PFN_SHIEFT)
#define PAGE_MASK (PAGE_SIZE-1)
#define DBGMSG_SRCC    (FALSE)

/**
*    @fn    SCODE S3C6410DISP::ValidatePAContinuityOfSurf(GPESurf *pTargetSurf)
*    @brief    If possible, clean dcahce for Source Rectangle
*    @param    pTargetSurf       Surface pointer of Target Surface
*    @sa        GPESurf
*    @note    for support cb to ncnb bitblt
*    @note    Bitmap image has top-down or bottom-up style memory region,
*            we can determine that by stride as positive(top-down) or as negative(bottom-up)
*            bottom-up bitmap mean buffer's start address is last addres of image buffer
*            image's start address is calculated as (Buffer address + Stride(negative) * height)
*    @return DWORD PhysAddressofSurface
*/
DWORD S3C6410Disp::ValidatePAContinuityOfSurf(    GPESurf *pTargetSurf    )
{
    BOOL        m_fLocked;
    LPVOID      m_lpvLockedAddress;
    DWORD       m_dwLockedSize;
    DWORD       dwLength = 0;
    DWORD       dwSrcSurfaceSize = 0;
    PDWORD      m_pdwPhysAddress = 0;
    PDWORD      m_pdwPhysLength;
    PBYTE *     m_ppVirtAddress;
    PVOID       pUseBufferPtr = 0;
    PVOID       pVirtualStartPtr = 0;

    // Normally negative stride can be found on system memory bitmap image. or DIB
    if(pTargetSurf->Stride() < 0)
    {
        RETAILMSG(DBGMSG_SRCC, (TEXT("Currently we cannot handle bottom-up bitmap using HW\r\n")));
        return NULL;
    }

    // Check the start address
    // Check if data is allocated across different pages
    // usable macros
    //     VM_PAGE_OFST_MASK
    //    VM_PAGE_SHIFT
    //    VM_PAGE_SIZE

//    uCount++;

    ASSERT(m_pdwPhysAddress==0);
    ASSERT((PAGE_MASK & PAGE_SIZE) == 0 );

    dwLength = pTargetSurf->Height() * ABS(pTargetSurf->Stride());
    RETAILMSG(DBGMSG_SRCC,(TEXT("TargetSurf 0x%x, Height : %d, Stride : %d\r\n"),pTargetSurf->Buffer(), pTargetSurf->Height(), pTargetSurf->Stride())); //< XPositive, yPositive
    // Todo: Check the Data Size
    // if size is not big to get efficiency. return false
#if (_WIN32_WCE < 600)    
    if(pTargetSurf->Stride() < 0)
    {
        pUseBufferPtr = MapPtrToProcess((PDWORD)((DWORD)pTargetSurf->Buffer() - dwLength + ABS(pTargetSurf->Stride())), (HANDLE)GetCurrentProcessId());
        pVirtualStartPtr = MapPtrToProcess(pUseBufferPtr, (HANDLE)GetCurrentProcessId());
    }
    else
    {
        pUseBufferPtr = MapPtrToProcess((PDWORD)pTargetSurf->Buffer(), (HANDLE)GetCurrentProcessId());    
        pVirtualStartPtr = MapPtrToProcess(pUseBufferPtr, (HANDLE)GetCurrentProcessId());;        
    }
#else
    if(pTargetSurf->Stride() < 0)
    {
        pUseBufferPtr = (PDWORD)((DWORD)pTargetSurf->Buffer() - dwLength + ABS(pTargetSurf->Stride()));
        pVirtualStartPtr = pUseBufferPtr;
    }
    else
    {
        pUseBufferPtr = (PDWORD)pTargetSurf->Buffer();    
        pVirtualStartPtr = pUseBufferPtr;
    }
#endif
    
    UINT nPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES( pUseBufferPtr, dwLength );

    m_pdwPhysAddress = new DWORD[3*nPages];        // It's really sturcture {m_pdwPhysAddress, m_pdwPhysLength, m_ppVirtAddress}
        
    if (m_pdwPhysAddress) {
        m_pdwPhysLength = m_pdwPhysAddress + nPages;
        m_ppVirtAddress = (PBYTE *)(m_pdwPhysAddress + 2*nPages);
        RETAILMSG(DBGMSG_SRCC,(TEXT("pUseBufferPtr:0x%x, dwLength:%d, m_pdwPhysAddress:0x%x.\r\n"), pUseBufferPtr, dwLength, m_pdwPhysAddress));
        m_fLocked = LockPages( pUseBufferPtr, dwLength, m_pdwPhysAddress, LOCKFLAG_QUERY_ONLY);        // Src to Dst
            
        if (!m_fLocked) { // Create table for Physical Address and length.
            RETAILMSG(DBGMSG_SRCC,(TEXT("LockPages is Failed : %d\r\n"), GetLastError() ));
            
            FreePhysAddress(m_pdwPhysAddress);            
            return NULL;
        }

        m_lpvLockedAddress = pUseBufferPtr;
        m_dwLockedSize = dwLength;

        RETAILMSG(DBGMSG_SRCC,(TEXT("pVirtualStartPtr:0x%x.\r\n"), pVirtualStartPtr));

        /// Get each Physical address pages from pagelocked information
        for (DWORD dwIndex = 0; dwIndex< nPages; dwIndex++) {
            if (m_pdwPhysAddress[dwIndex] > MAX_SUPPORTED_PFN) {
                ASSERT(FALSE);
                FreePhysAddress(m_pdwPhysAddress);                
                return NULL;  //< NULL is FAIL
            }
                
            DWORD dwSize = min((PAGE_SIZE - ((DWORD)pUseBufferPtr & PAGE_MASK)),dwLength) ;
        
            m_pdwPhysAddress[dwIndex] = (m_pdwPhysAddress[dwIndex]<<PFN_SHIEFT) + ((DWORD)pUseBufferPtr & PAGE_MASK);
            m_pdwPhysLength[dwIndex] = dwSize;
            m_ppVirtAddress[dwIndex] = (PBYTE)pUseBufferPtr;
            dwLength -= dwSize;
            pUseBufferPtr = (PBYTE)pUseBufferPtr+dwSize;
            RETAILMSG(DBGMSG_SRCC,(TEXT("dwIndex : %d, m_pdwPhysAddress[%d]:0x%x, m_pdwPhysLength[%d]:%d, dwSize:%d, pUseBufferPtr(PageEnd):0x%x.\r\n"),
                dwIndex,
                dwIndex, m_pdwPhysAddress[dwIndex],
                dwIndex, m_pdwPhysLength[dwIndex],
                dwSize,
                pUseBufferPtr
            ));            
        }
                
        /// Check if Source Pages is contiguous in Physical memory address.
        DWORD dwRead = 1;
        while (dwRead < nPages) {
            if (m_pdwPhysAddress[dwRead - 1] + m_pdwPhysLength[dwRead - 1] == m_pdwPhysAddress[dwRead]) {
                // m_dwBlocks and m_dwBlocks+1 is contiguous.
                dwRead++;
            }
            else { // No match, We cannot use HW
                RETAILMSG(DBGMSG_SRCC,(TEXT("Source Memory Blocks is not congiuous : Go Emul path\r\n")));
                FreePhysAddress(m_pdwPhysAddress);                
                return NULL;    //< NULL is Fail 
            }
        }
        // Merge to one big contiguous memory block
        if(nPages > 1)
        {
            for(dwRead = 1 ; dwRead < nPages; dwRead++) {
                m_pdwPhysLength[0] += m_pdwPhysLength[dwRead];
            }
        }
    }
    else
    {
        RETAILMSG(DBGMSG_SRCC,(TEXT("Not Enough Memory for m_pdwPhysAddress\r\n")));
        FreePhysAddress(m_pdwPhysAddress);
        return NULL;        //< NULL is Fail
    }
       
//     uPossibleCount ++;                
//    RETAILMSG(TRUE,(TEXT("DCache Clean , Available Flow Count :  %d avail. / %d Total \r\n"),uPossibleCount, uCount));                
    RETAILMSG(DBGMSG_SRCC,(TEXT("CacheFlush Start : 0x%x length : %d\r\n"),
        (PBYTE)pVirtualStartPtr, 
        pTargetSurf->Height() * ABS(pTargetSurf->Stride())));
    // Just Contiguous Source Surface can use HW // Cache flush for all surface region
    CacheRangeFlush( (PBYTE)pVirtualStartPtr, 
        pTargetSurf->Height() * ABS(pTargetSurf->Stride()),
        CACHE_SYNC_WRITEBACK);        

    RETAILMSG(DBGMSG_SRCC,(TEXT("m_pdwPhysAddress[0] : 0x%x\r\n"), m_pdwPhysAddress[0]));
    m_dwSourceSurfacePA = m_pdwPhysAddress[0];  //< Just leave for compatiblilty 

    FreePhysAddress(m_pdwPhysAddress);
        
    return m_dwSourceSurfacePA;    
}
void S3C6410Disp::FreePhysAddress(DWORD *m_pdwPhysAddress)
{
    if(m_pdwPhysAddress)
    {
        delete [] m_pdwPhysAddress;
        m_pdwPhysAddress=NULL;
    }
}

void S3C6410Disp::ClipDestDrawRect(GPEBltParms *pBltParms)
{
    if(pBltParms->pDst->InVideoMemory() )
    {
        if(pBltParms->prclDst->left < 0)
        {
            pBltParms->prclSrc->left -= pBltParms->prclDst->left;
            pBltParms->prclDst->left = 0;
        
        }
        if(pBltParms->prclDst->top < 0)
        {
            pBltParms->prclSrc->top -= pBltParms->prclDst->top;
            pBltParms->prclDst->top = 0;
        }
    }
}

DWORD S3C6410Disp::GetHWColorFormat(GPESurf *pSurf)
{
    DWORD dw2DHWColorFormat = G2D_COLOR_UNUSED;
    if(pSurf)
    {
        GPEFormat * pFormat;
        pFormat = pSurf->FormatPtr();
    
        if (pSurf->Format() == gpe16Bpp)
        {
            if (pFormat->m_pPalette)
            {
                if (pFormat->m_PaletteEntries == 3 &&
                    pFormat->m_pPalette[0] == 0x0000f800 && // R
                    pFormat->m_pPalette[1] == 0x000007e0 && // G
                    pFormat->m_pPalette[2] == 0x0000001f) // B
                {
                    dw2DHWColorFormat = G2D_COLOR_RGB_565;
                }
                else if (pFormat->m_PaletteEntries == 3 &&
                         pFormat->m_pPalette[0] == 0x00007c00 &&    // R
                         pFormat->m_pPalette[1] == 0x000003e0 &&    // G
                         pFormat->m_pPalette[2] == 0x0000001f)      // B
                {
                    dw2DHWColorFormat = G2D_COLOR_UNUSED;//G2D_COLOR_ARGB_1555;
                }
                else if (pFormat->m_PaletteEntries == 4 &&
                         pFormat->m_pPalette[3] == 0x00008000 &&    // A
                         pFormat->m_pPalette[0] == 0x00007c00 &&    // R
                         pFormat->m_pPalette[1] == 0x000003e0 &&    // G
                         pFormat->m_pPalette[2] == 0x0000001f)      // B
                {
                    dw2DHWColorFormat = G2D_COLOR_ARGB_1555;
                }
                else 
                {
                    DEBUGMSG(GPE_ZONE_HW, (TEXT("pFormat->m_PaletteEntries=%08x\r\n"), pFormat->m_PaletteEntries));
                    for (int i=0; i<pFormat->m_PaletteEntries; i++)
                    {
                        DEBUGMSG(GPE_ZONE_HW, (TEXT("%d : 0x%08x\r\n"), i, pFormat->m_pPalette[i]));
                    }
                    dw2DHWColorFormat = G2D_COLOR_UNUSED;
                }
            }
            else
            {
                DEBUGMSG(GPE_ZONE_HW, (TEXT("Surface Format HAS NO PALETTE, we assume it's RGBx555\r\n")));
                dw2DHWColorFormat = G2D_COLOR_UNUSED;//G2D_COLOR_ARGB_1555;
            }
        }
        else
        if (pSurf->Format() == gpe32Bpp)
        {
            if (pFormat->m_pPalette)
            {
                if (pFormat->m_PaletteEntries == 4 &&
                         pFormat->m_pPalette[3] == 0xff000000 &&    // A
                         pFormat->m_pPalette[0] == 0x00ff0000 &&    // R
                         pFormat->m_pPalette[1] == 0x0000ff00 &&    // G
                         pFormat->m_pPalette[2] == 0x000000ff)      // B
                {
                    dw2DHWColorFormat = G2D_COLOR_ARGB_8888;
                }
                else if (//pFormat->m_PaletteEntries == 3 &&
                    pFormat->m_pPalette[0] == 0x00ff0000 && // R
                    pFormat->m_pPalette[1] == 0x0000ff00 && // G
                    pFormat->m_pPalette[2] == 0x000000ff)   // B
                {
                    dw2DHWColorFormat = G2D_COLOR_XRGB_8888;
                }
                else if (pFormat->m_PaletteEntries == 4 &&
                         pFormat->m_pPalette[3] == 0x000000ff &&    // A
                         pFormat->m_pPalette[0] == 0xff000000 &&    // R
                         pFormat->m_pPalette[1] == 0x00ff0000 &&    // G
                         pFormat->m_pPalette[2] == 0x0000ff00)      // B
                {
                    dw2DHWColorFormat = G2D_COLOR_RGBA_8888;
                }
                else  if (//pFormat->m_PaletteEntries == 3 &&
                    pFormat->m_pPalette[0] == 0xff000000 && // R
                    pFormat->m_pPalette[1] == 0x00ff0000 && // G
                    pFormat->m_pPalette[2] == 0x0000ff00)   // B
                {
                    dw2DHWColorFormat = G2D_COLOR_RGBX_8888;
                }
                else 
                {
                    DEBUGMSG(GPE_ZONE_HW, (TEXT("pFormat->m_PaletteEntries=%08x\r\n"), pFormat->m_PaletteEntries));
                    for (int i = 0; i<pFormat->m_PaletteEntries; i++)
                    {
                        DEBUGMSG(GPE_ZONE_HW, (TEXT("%d : 0x%08x\r\n"), i, pFormat->m_pPalette[i]));
                    }
                    dw2DHWColorFormat = G2D_COLOR_UNUSED;
                }
            }
            else
            {
                DEBUGMSG(GPE_ZONE_HW, (TEXT("Surface Format HAS NO PALETTE, we assume it as ABGR8888 and use ARGB8888\r\n")));
                //dw2DHWColorFormat = G2D_COLOR_UNUSED;
                
                dw2DHWColorFormat = G2D_COLOR_ARGB_8888;
            }
        }
        DEBUGMSG(GPE_ZONE_HW, (TEXT("Surface:0x%x, GPEFormat:%d, HWColorMode:%d\n\r"), pSurf, pSurf->Format(), dw2DHWColorFormat));        
        return dw2DHWColorFormat;        
    }
    RETAILMSG(DISP_ZONE_ERROR, (TEXT("Illegal Surface\n\r")));
    return dw2DHWColorFormat;
}
