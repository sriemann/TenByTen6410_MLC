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

Module Name:    blt_alpha.cpp

Abstract:       alphablended accelerated bitblt/rectangle for S3C6410 FIMGSE-2D

Functions:


Notes:


--*/

#include "precomp.h"

#define ALPHABIT_WORKAROUND0 (TRUE)
// This will blend alphabit of destination surface that has non-blended alpha value(source value transferred)
// This workaround is applied on S3C6410, S5PC100
#define ALPHABIT_WORKAROUND1 (FALSE)

extern "C"
{
    void MultiplyAlphaBit_ASM(DWORD *pdwStartAddress, DWORD dwBufferLength, DWORD AlphaConstant);
    void PerPixelAlpha_ASM(DWORD dwSrcAddr, DWORD dwDstAddr, DWORD MinWidth);
}

/// From Frame Buffer to Frame Buffer Directly
/// Constraints
/// Source Surface's width is same with Destination Surface's width.
/// Source and Dest must be in Video FrameBuffer Region
/// In Surface Format
/// ScreenHeight and ScreenWidth means logical looking aspect for application
/// Height and Width means real data format.
/// This function will copy the source and destination surface into scratch buffer
/**
*   @fn SCODE S3C6410Disp::AcceleratedAlphaSrcCopyBlt(GPEBltParms *pBltParms)
*   @brief  Do Blit with SRCCOPY, SRCAND, SRCPAINT, SRCINVERT
*   @param  pBltParms    Blit Parameter Information Structure
*   @sa     GPEBltParms
*   @note   ROP : 0xCCCC(SRCCOPY), 0x8888(SRCAND), 0x6666(SRCINVERT), 0XEEEE(SRCPAINT)
*   @note   Using Information : DstSurface, ROP, Solidcolor
*/
SCODE S3C6410Disp::AcceleratedAlphaSrcCopyBlt(GPEBltParms *pBltParms)
{
    PRECTL prclSrc, prclDst;
    RECT rectlSrcBackup;
    RECT rectlDstBackup;
    RECT rectlClipBackup;
    BOOL bHWSuccess = FALSE;
    BOOL bSrcInScratch = FALSE;
    BOOL bDstInScratch = FALSE;

    /// Allocation Scratch Framebuffer to process Bottom-Up Image
    DDGPESurf *SrcScratchSurf = NULL;
    DDGPESurf *DstScratchSurf = NULL;
    DDGPESurf *pDstClone = NULL;
    GPESurf *OldSrcSurf = NULL;
    GPESurf *OldDstSurf = NULL;

    prclSrc = pBltParms->prclSrc;
    prclDst = pBltParms->prclDst;

    /**
    *   Prepare Source & Destination Surface Information
    */
    SURFACE_DESCRIPTOR descSrcSurface, descDstSurface;
    DWORD   dwTopStrideStartAddr = 0;

    descSrcSurface.dwColorMode = GetHWColorFormat(pBltParms->pSrc);
    descDstSurface.dwColorMode = GetHWColorFormat(pBltParms->pDst);
    /// !!!!Surface Width may not match to Real Data format!!!!
    /// !!!!Set Width by Scan Stride Size!!!!
    descSrcSurface.dwHoriRes = ABS(SURFACE_WIDTH(pBltParms->pSrc));
    descDstSurface.dwHoriRes = ABS(SURFACE_WIDTH(pBltParms->pDst));

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
        m_oG2D->SetRopEtype(ROP_SRC_XOR_DST);
        break;
    case    0x8888: // SRCAND
        m_oG2D->SetRopEtype(ROP_SRC_AND_DST);
        break;
    case    0xCCCC: // SRCCOPY
        m_oG2D->SetRopEtype(ROP_SRC_ONLY);
        break;
    case    0xEEEE: // SRCPAINT
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
    CopyRect(&rectlClipBackup, (LPRECT)pBltParms->prclClip);

    /// Destination's Region can have negative coordinate, especially for left, top point
    /// In this case, For both destination, source's rectangle must be clipped again to use HW.
    ClipDestDrawRect(pBltParms);

    /// Set Source Surface Information
    if((pBltParms->pSrc->Stride() > 0) && (pBltParms->pSrc == pBltParms->pDst)) // OnScreen BitBlt
    {
        if(pBltParms->pSrc->InVideoMemory())
        {
            dwTopStrideStartAddr = (m_VideoMemoryPhysicalBase + (( DDGPESurf *)(pBltParms->pSrc))->OffsetInVideoMemory());
        }
        else if(!m_dwPhyAddrOfSurface[0])
        {
            dwTopStrideStartAddr = m_dwPhyAddrOfSurface[0];
        }
        else{
            bHWSuccess = FALSE;
            goto PostHWBitBlt;
        }
        descSrcSurface.dwBaseaddr = dwTopStrideStartAddr;
        descSrcSurface.dwVertRes = (pBltParms->pSrc->ScreenHeight() != 0 ) ? pBltParms->pSrc->ScreenHeight() : pBltParms->pSrc->Height();
        descDstSurface.dwBaseaddr = descSrcSurface.dwBaseaddr;
        descDstSurface.dwVertRes = descSrcSurface.dwVertRes;
        RETAILMSG(DISP_ZONE_2D,(TEXT("Onscreen CachedBitBlt:Addr:0x%x, Height:%d\r\n"), descSrcSurface.dwBaseaddr, descSrcSurface.dwVertRes));
    }
    else        // OffScreen BitBlt
    {
        if((( DDGPESurf *)(pBltParms->pSrc))->InVideoMemory() )
        {
            descSrcSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + (( DDGPESurf *)(pBltParms->pSrc))->OffsetInVideoMemory());
            /// If surface is created by user temporary, that has no screen width and height.
            descSrcSurface.dwVertRes = (pBltParms->pSrc->ScreenHeight() != 0 ) ? pBltParms->pSrc->ScreenHeight() : pBltParms->pSrc->Height();
            RETAILMSG(DISP_ZONE_2D,(TEXT("Offscreen BitBlt Src in VideoMem:Addr:0x%x, Height:%d\r\n"), descSrcSurface.dwBaseaddr, descSrcSurface.dwVertRes));
        }
        else
        {
            if((pBltParms->pSrc->Stride() >= 0) && (m_dwPhyAddrOfSurface[0] != NULL))
            {
                dwTopStrideStartAddr = m_dwPhyAddrOfSurface[0] + pBltParms->prclSrc->top * pBltParms->pSrc->Stride();
                descSrcSurface.dwBaseaddr = dwTopStrideStartAddr;
                descSrcSurface.dwVertRes = RECT_HEIGHT(pBltParms->prclSrc);

                pBltParms->prclSrc->top = 0;
                pBltParms->prclSrc->bottom = descSrcSurface.dwVertRes;
                RETAILMSG(DISP_ZONE_2D,(TEXT("STVBase:0x%x, Vert:%d, Hori:%d, CM:%d\r\n"), descSrcSurface.dwBaseaddr, descSrcSurface.dwVertRes,
                descSrcSurface.dwHoriRes, descSrcSurface.dwColorMode));
            }
            else    // In this case, WE must copy source area. This case includes Stride < 0, or Physically non-contigious surface
            {
                bHWSuccess = CreateScratchSurface(pBltParms->pSrc, &SrcScratchSurf, pBltParms->prclSrc, &descSrcSurface, pBltParms->pSrc->Format(), TRUE);

                if(!bHWSuccess)
                {
                    goto PostHWBitBlt;
                }

                OldSrcSurf = pBltParms->pSrc;
                pBltParms->pSrc = SrcScratchSurf;
                bSrcInScratch = TRUE;
                RETAILMSG(DISP_ZONE_2D,(TEXT("SBBase:0x%x, Vert:%d, Hori:%d, CM:%d\r\n"), descSrcSurface.dwBaseaddr, descSrcSurface.dwVertRes,
                descSrcSurface.dwHoriRes, descSrcSurface.dwColorMode));
            }
        }

        /// Set Destination Surface Information
        if(((DDGPESurf *)(pBltParms->pDst))->InVideoMemory() )
        {
            descDstSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + (( DDGPESurf *)(pBltParms->pDst))->OffsetInVideoMemory());
            /// If surface is created by user temporary, that has no screen width and height.
            descDstSurface.dwVertRes = (pBltParms->pDst->ScreenHeight() != 0 ) ? pBltParms->pDst->ScreenHeight() : pBltParms->pDst->Height();
            RETAILMSG(DISP_ZONE_2D,(TEXT("Offscreen BitBlt Dst in VideoMem:Addr:0x%x, Height:%d\r\n"), descDstSurface.dwBaseaddr, descDstSurface.dwVertRes));
        }
        else
        {
            if((pBltParms->pDst->Stride() >= 0) && (m_dwPhyAddrOfSurface[1] != NULL))
            {
                dwTopStrideStartAddr = m_dwPhyAddrOfSurface[1] + pBltParms->prclDst->top * pBltParms->pDst->Stride();
                descDstSurface.dwBaseaddr = dwTopStrideStartAddr;
                descDstSurface.dwVertRes = RECT_HEIGHT(pBltParms->prclDst);
                
                pBltParms->prclDst->top = 0;
                pBltParms->prclDst->bottom = descDstSurface.dwVertRes;
                RETAILMSG(DISP_ZONE_2D,(TEXT("DTVBase:0x%x, Vert:%d, Hori:%d, CM:%d\r\n"), descDstSurface.dwBaseaddr, descDstSurface.dwVertRes,
                descDstSurface.dwHoriRes, descDstSurface.dwColorMode));
            }
            else    // This case includes Stride < 0, or Physically non-contigious surface
            {
                bHWSuccess = CreateScratchSurface(pBltParms->pDst, &DstScratchSurf, pBltParms->prclDst, &descDstSurface, pBltParms->pDst->Format(), TRUE);

                if(!bHWSuccess)
                {
                    goto PostHWBitBlt;
                }

                OldDstSurf = pBltParms->pDst;
                pBltParms->pDst = DstScratchSurf;
                if (pBltParms->prclClip != NULL)
                {
                    pBltParms->prclClip->top = 0;
                    pBltParms->prclClip->bottom = RECT_HEIGHT(pBltParms->prclClip);
                }
                bDstInScratch = TRUE;
                RETAILMSG(DISP_ZONE_2D,(TEXT("DTBase:0x%x, Vert:%d, Hori:%d, CM:%d\r\n"), descDstSurface.dwBaseaddr, descDstSurface.dwVertRes,
                descDstSurface.dwHoriRes, descDstSurface.dwColorMode));
            }
        }
    }

    m_oG2D->Set3rdOperand(G2D_OPERAND3_PAT);

    // AlphaBlending Equation, Perpixel and Perplane same.
    // Data = (Source * (ALPHA+1) + destination *(256-ALPHA)) >> 8
    // Fading
    // Data = ((Source * (ALPHA+1) ) >>8) + fading offset
    //

    // Check if Constant Alpha or Per-Pixel Alpha
    if(pBltParms->blendFunction.AlphaFormat != 0 && pBltParms->blendFunction.SourceConstantAlpha != 0xFF)
    {
#ifdef ALPHABIT_WORKAROUND0
        //
        // ALPHABIT_WORKAROUND0: If both PP and SCA are specified at the same time, just pick one as a workaround.
        //                       This will validate the performance of the driver (rather than correctness)
        //                       Use other Workarounds for correctness
        //
        m_oG2D->SetAlphaMode(G2D_ALPHA_MODE);   //< Constant Alpha
        m_oG2D->SetAlphaValue(pBltParms->blendFunction.SourceConstantAlpha);
#else   // ALPHABIT_WORKAROUND0
        SURFACE_DESCRIPTOR descRealSrcSurface;
        GPESurf *pDstBackup=0;
        RECT rectDstBackup;
        RECT rectSrcBackup;
#if ALPHABIT_WORKAROUND2        
        SURFACE_DESCRIPTOR descRealDstSurface;        
        RECT rectBackupCloneDst;
#endif  // ALPHABIT_WORKAROUND2        

        // Backup original source region, This can be Scratch Surface's region
        CopyRect(&rectSrcBackup, (LPRECT)pBltParms->prclSrc);
        // Backup original destination region
        CopyRect(&rectDstBackup, (LPRECT)pBltParms->prclDst);
        // Bakcup original destination surface
        pDstBackup = pBltParms->pDst;

        // backup real source Surface.
        memcpy(&descRealSrcSurface, &descSrcSurface, sizeof(SURFACE_DESCRIPTOR));

        // if Per-Pixel Alpha with SCA then we just run 2 alphablend call
        // set destination descriptor as source descriptor.
        // prepare scratch for fading.
        if(!bSrcInScratch)
        {
            bHWSuccess = CreateScratchSurface(pBltParms->pSrc, &SrcScratchSurf, pBltParms->prclSrc, &descSrcSurface, pBltParms->pSrc->Format(), FALSE);

            OldSrcSurf = pBltParms->pSrc;
            pBltParms->pSrc = SrcScratchSurf;
            bSrcInScratch = TRUE;
        }

        // Replace Target region as source itself
        pBltParms->prclDst = pBltParms->prclSrc;
        // Replace Target Surface as Source Surface
        pBltParms->pDst = pBltParms->pSrc;

        m_oG2D->SetAlphaMode(G2D_FADING_MODE); //< Self Fading AlphaBlend
        m_oG2D->SetAlphaValue(pBltParms->blendFunction.SourceConstantAlpha);
        // Do SCA to self(fading mode).
        bHWSuccess = HWBitBlt(pBltParms, &descRealSrcSurface, &descSrcSurface);
        m_oG2D->WaitForIdle();

        ///////
        // restore destination region
        pBltParms->prclDst = (LPRECTL)&rectDstBackup;
        // restore destination surface
        pBltParms->pDst = pDstBackup;

        if(!bHWSuccess)
        {
            goto PostHWBitBlt;
        }

#if ALPHABIT_WORKAROUND1
        // SW Solution, just fix the alphabit
        // Not good, need to implement as assembler.
        MultiplyAlphaBit_ASM((DWORD *)pBltParms->pSrc->Buffer(),
                        ABS(pBltParms->pSrc->Stride()*pBltParms->pSrc->Height()),
                        pBltParms->blendFunction.SourceConstantAlpha);
#endif  // ALPHABIT_WORKAROUND1
#if ALPHABIT_WORKAROUND2
        // Workaround2
        // The only possible same scenario to MS SW alphablend is
        // 1. Fading Source Surface
        // 2. Create Destination Scratch Surface and Clone destination surface.
        // 3. Do Constant Alphablend with Fading Source Surface and Destination Scratch Surface.
        // 4. Do Per-Pixel Alphablend with Destination Scratch Surface and Destination Surface.
        // Through this scenario, but destination surface's alphabit has source surface's one.
        // Only RGB pixel has almost same result to MS SW alphablend.
        // This requires 4 memory copies

        // Do SCA between Source and Destination Scratch
        // We should make another Destination Scratch surface, this will have some blended pixel

        // 0. backup real destination Surface.
        memcpy(&descRealDstSurface, &descDstSurface, sizeof(SURFACE_DESCRIPTOR));

        CopyRect((LPRECT)&rectBackupCloneDst, (LPRECT)&rectDstBackup);

        // 1.Create Destination Scratch Surface for Source Constant AlphaBlending with Source Scratch Surface
        // This Cloned Destination surface should have 32bpp alphaformat
        // The trick is change the color format of pDst
        bHWSuccess = CreateScratchSurface(pBltParms->pDst, &pDstClone, (LPRECTL)&rectBackupCloneDst, &descDstSurface, gpe32Bpp, FALSE);

        descDstSurface.dwColorMode = GetHWColorFormat(pDstClone);
        pBltParms->prclDst = (LPRECTL)&rectBackupCloneDst;

        if ( DMDO_90 == pBltParms->pDst->Rotate() || DMDO_270 == pBltParms->pDst->Rotate() )
        {
            pDstClone->SetRotation( pDstClone->Height(), pDstClone->Width(), pBltParms->pDst->Rotate() );

            // Adjust the source rectangle for this case.
            RotateRectl(pBltParms->prclDst);
        }

        if(!bHWSuccess)
        {
            memcpy(&descDstSurface, &descRealDstSurface, sizeof(SURFACE_DESCRIPTOR));
            goto PostHWBitBlt;
        }

        // 2.Copy Destination Surface's contents
        // This will be processed by 2DHW with color converting
        // 2-1 Set Source as Real Destination Surface
        pBltParms->pSrc = pDstBackup;
        pBltParms->prclSrc = (LPRECTL)&rectDstBackup;
        // 2-2 Set Destination as Cloned desitnation surface
        pBltParms->pDst = pDstClone;
        m_oG2D->SetAlphaMode(G2D_NO_ALPHA_MODE); //< Source Constant AlphaBlend
        // Alpha = Alpha*(Alpha/256)
        m_oG2D->SetAlphaValue((pBltParms->blendFunction.SourceConstantAlpha*pBltParms->blendFunction.SourceConstantAlpha)>>8);

        bHWSuccess = HWBitBlt(pBltParms, &descRealDstSurface, &descDstSurface);
        m_oG2D->WaitForIdle();

        if(!bHWSuccess)
        {
            goto PostHWBitBlt;
        }

        m_oG2D->SetAlphaMode(G2D_ALPHA_MODE); //< Source Constant AlphaBlend

        // Set Source as Source Scratch Surface
        pBltParms->pSrc = SrcScratchSurf;
        pBltParms->prclSrc = (LPRECTL)&rectSrcBackup;

        // 3.Do SCA from faded source surface to cloned destination surface.
        bHWSuccess = HWBitBlt(pBltParms, &descSrcSurface, &descDstSurface);
        m_oG2D->WaitForIdle();
        if(bHWSuccess)
        {
            // Keep Going
            bHWSuccess = FALSE;
        }
        else
        {
            goto PostHWBitBlt;
        }

        // 4. assign cloned destination surface as source surface.
        pBltParms->pSrc = pDstClone;
        pBltParms->prclSrc = pBltParms->prclDst;
        memcpy(&descSrcSurface, &descDstSurface, sizeof(SURFACE_DESCRIPTOR));
        // 5. restore original destination surface.
        pBltParms->pDst = pDstBackup;
        pBltParms->prclDst = (LPRECTL)&rectDstBackup;
        memcpy(&descDstSurface, &descRealDstSurface, sizeof(SURFACE_DESCRIPTOR));

        // 6. do PPA with cloned destination surface that is assigned to source scratch surface.
        ///////////
#endif  // ALPHABIT_WORKAROUND2

        // Do PPA
        m_oG2D->SetAlphaMode(G2D_PP_ALPHA_SOURCE_MODE); //< Per-Pixel AlphaBlend
        m_oG2D->SetAlphaValue(pBltParms->blendFunction.SourceConstantAlpha/*0xff*/);                    //< Per-Pixel Opaque, dummy value

        if ((descSrcSurface.dwColorMode == descDstSurface.dwColorMode) && !(pBltParms->bltFlags & BLT_STRETCH))
        {
            AcceleratedPerPixelAlpha(pBltParms);
            bHWSuccess = TRUE;
            goto PostHWBitBlt;
        }
#endif  // !ALPHABIT_WORKAROUND0
    }
    else if(pBltParms->blendFunction.SourceConstantAlpha != 0xFF)
    {
        if(descDstSurface.dwBaseaddr == descSrcSurface.dwBaseaddr)
        {
            RETAILMSG(DISP_ZONE_2D,(TEXT("FadingMode\r\n")));
            m_oG2D->SetAlphaMode(G2D_FADING_MODE); //< Self Fading AlphaBlend
        }
        else{
            RETAILMSG(DISP_ZONE_2D,(TEXT("Constant\r\n")));
            m_oG2D->SetAlphaMode(G2D_ALPHA_MODE);   //< Constant Alpha
        }
        m_oG2D->SetAlphaValue(pBltParms->blendFunction.SourceConstantAlpha);
    }
    else if(pBltParms->blendFunction.AlphaFormat != 0)
    {
        RETAILMSG(DISP_ZONE_2D,(TEXT("PerPixelMode\r\n")));
        m_oG2D->SetAlphaMode(G2D_PP_ALPHA_SOURCE_MODE); //< Per-Pixel AlphaBlend
        m_oG2D->SetAlphaValue(pBltParms->blendFunction.SourceConstantAlpha/*0xff*/);                    //< Per-Pixel Opaque, dummy value

        if ((descSrcSurface.dwColorMode == descDstSurface.dwColorMode) && !(pBltParms->bltFlags & BLT_STRETCH))
        {
            AcceleratedPerPixelAlpha(pBltParms);
            bHWSuccess = TRUE;
            goto PostHWBitBlt;
        }
    }
    else    // No AlphaBlend
    {
        /// Transparency does not relate with alpha blending
        m_oG2D->SetAlphaMode(G2D_NO_ALPHA_MODE);
        /// No transparecy with alphablend
        m_oG2D->SetAlphaValue(0xff);
    }

    RETAILMSG(FALSE,(TEXT("PixelDst:0x%x\n"), *((DWORD*)pBltParms->pDst->Buffer())));
    /// Real Register Surface Description setting will be done in HWBitBlt
    bHWSuccess = HWBitBlt(pBltParms, &descSrcSurface, &descDstSurface);

    RETAILMSG(FALSE,(TEXT("PixelDstAfter:0x%x\n"), *((DWORD*)pBltParms->pDst->Buffer())));

PostHWBitBlt:
    if(!bHWSuccess)
    {
        RETAILMSG(DISP_ZONE_WARNING,(TEXT("HWBitBlt Failed\r\n")));
    }
    else    // TODO: Post mortem, Alphabit multiplying
    {

    }

    if(pDstClone != NULL)
    {
        delete pDstClone;
        pDstClone = NULL;
    }
    if(OldSrcSurf != NULL)
    {
        RETAILMSG(DISP_ZONE_2D,(TEXT("Release Source Scratch Surface\r\n")));
        pBltParms->pSrc = OldSrcSurf;
        if(SrcScratchSurf)
        {
            RETAILMSG(DISP_ZONE_2D,(TEXT("Delete Source Scratch Surface\r\n")));
            delete SrcScratchSurf;
            SrcScratchSurf = NULL;
        }
    }
    RETAILMSG(DISP_ZONE_2D,(TEXT("Source Scratch Surface is destroyed\r\n")));
    if(OldDstSurf != NULL)
    {
        if(m_dwPhyAddrOfSurface[1] == NULL) // copy
        {
            DWORD dwTopOffset;
            m_oG2D->WaitForIdle();
            dwTopOffset = rectlDstBackup.top * pBltParms->pDst->Stride();
            RETAILMSG(DISP_ZONE_2D,(TEXT("Release Destination Scratch Surface\r\n")));
            pBltParms->pDst = OldDstSurf;
            // Copy ScratchBuffer to Original Image Buffer
            RETAILMSG(DISP_ZONE_2D,(TEXT("DstScratch:0x%x, OriDstBuffer:0x%x, Offset:(%d), Dst:0x%x  Size:%d\r\n"),
            DstScratchSurf->Buffer(), pBltParms->pDst->Buffer(),
                dwTopOffset,
//                (BYTE *)((DWORD)pBltParms->pDst->Buffer() + ABS(pBltParms->pDst->Stride()) + RECT_HEIGHT(pBltParms->prclDst) * pBltParms->pDst->Stride()),
                (BYTE *)((DWORD)pBltParms->pDst->Buffer() + dwTopOffset),
                ABS(DstScratchSurf->Height() * DstScratchSurf->Stride())));

            memcpy((BYTE *)((DWORD)pBltParms->pDst->Buffer() + dwTopOffset),
                (BYTE *)DstScratchSurf->Buffer(),
                ABS(DstScratchSurf->Stride()* RECT_HEIGHT(pBltParms->prclDst))
                );
        }
        else
        {
            RETAILMSG(DISP_ZONE_2D,(TEXT("Release Destination Scratch Surface\r\n")));
            m_oG2D->WaitForIdle();
            pBltParms->pDst = OldDstSurf;
            // Copy ScratchBuffer to Original Image Buffer
            RETAILMSG(DISP_ZONE_2D,(TEXT("DstScratch:0x%x, OriDstBuffer:0x%x, Offset:(%d), Dst:0x%x  Size:%d\r\n"),
            DstScratchSurf->Buffer(), pBltParms->pDst->Buffer(),
                RECT_HEIGHT(pBltParms->prclDst) * pBltParms->pDst->Stride(),
                (BYTE *)((DWORD)pBltParms->pDst->Buffer() + ABS(pBltParms->pDst->Stride()) + RECT_HEIGHT(pBltParms->prclDst) * pBltParms->pDst->Stride()),
                ABS(DstScratchSurf->Height() * DstScratchSurf->Stride())));

            for(DWORD i = 0; i < (DWORD)DstScratchSurf->Height(); i++)
            {
                memcpy((BYTE *)((DWORD)pBltParms->pDst->Buffer() + pBltParms->pDst->Stride() * (i + pBltParms->prclDst->top)),
                    (BYTE *)DstScratchSurf->Buffer()+ i * ABS(DstScratchSurf->Stride()),
                    ABS(DstScratchSurf->Stride())
                    );
            }
        }
        if(DstScratchSurf)
        {
            delete DstScratchSurf;
            DstScratchSurf = NULL;
        }

    }

    RETAILMSG(DISP_ZONE_2D,(TEXT("Recover Rect\r\n")));
    CopyRect((LPRECT)pBltParms->prclSrc, &rectlSrcBackup);
    CopyRect((LPRECT)pBltParms->prclDst, &rectlDstBackup);
    CopyRect((LPRECT)pBltParms->prclClip, &rectlClipBackup);

    RETAILMSG(DISP_ZONE_2D,(TEXT("Rerotate Rect\r\n")));
    if(pBltParms->pDst->IsRotate())
    {
        RotateRectlBack(prclDst);
        if(pBltParms->prclClip)
        {
        RotateRectlBack(pBltParms->prclClip);
        }
    }
    if(pBltParms->pSrc && pBltParms->pSrc->IsRotate())
    {
        RotateRectlBack(prclSrc);
    }
    if (pBltParms->bltFlags & BLT_TRANSPARENT)
    {
        m_oG2D->SetTransparentMode(0, pBltParms->solidColor);// turn off Transparency
    }
    RETAILMSG(DISP_ZONE_2D,(TEXT("Check HW Success:%d\r\n"),bHWSuccess));
    if(!bHWSuccess)
    {
        return EmulatedBlt(pBltParms);
    }

    return  S_OK;
}

void S3C6410Disp::MultiplyAlphaBit(DWORD *pdwStartAddress, DWORD dwBufferLength, DWORD AlphaConstant)
{
    // Premultiplying Alphabit for Target Area that already processed by fading.
    // We assume the target surface's color depth is 32bpp.
    DWORD orgPixel;
    DWORD rgb;
    DWORD alpha;
    DWORD *pdwEndAddress;
    pdwEndAddress = pdwStartAddress + dwBufferLength;
    while(pdwEndAddress != pdwStartAddress)
    {
        orgPixel = *(pdwStartAddress);
        rgb = (orgPixel & 0x00FFFFFF);
        alpha = (((orgPixel>>24)*AlphaConstant)>>8)<<24;
        *(pdwStartAddress) = rgb | alpha;
        pdwStartAddress++;
    };
}

// There are lots of difference between H/W and S/W perpixel alpha blend algorithm.
// In case of ARGB(8888) to ARGB(8888) perpixel alpha blend, H/W output could be completely different from S/W(GPE).
// So this function is only called, when the source and destination surfaces are ARGB(8888) format.
void S3C6410Disp::AcceleratedPerPixelAlpha(GPEBltParms * pBltParms)
{
    DWORD SrcStride = pBltParms->pSrc->Stride();
    DWORD DstStride = pBltParms->pDst->Stride();
    DWORD SrcAddr = (DWORD)pBltParms->pSrc->Buffer() + ((pBltParms->prclSrc->top * SrcStride) + pBltParms->prclSrc->left * BYTES_PER_PIXEL(pBltParms->pSrc));
    DWORD DstAddr = (DWORD)pBltParms->pDst->Buffer() + ((pBltParms->prclDst->top * DstStride) + pBltParms->prclDst->left * BYTES_PER_PIXEL(pBltParms->pDst));
    DWORD SrcWidth = RECT_WIDTH(pBltParms->prclSrc);
    DWORD DstWidth = RECT_WIDTH(pBltParms->prclDst);
    DWORD SrcHeight = RECT_HEIGHT(pBltParms->prclSrc);
    DWORD DstHeight = RECT_HEIGHT(pBltParms->prclDst);
    DWORD MinWidth = MIN(SrcWidth, DstWidth);
    DWORD MinHeight = MIN(SrcHeight, DstHeight);

    for (DWORD i=0; i<MinHeight; i++)
    {
        PerPixelAlpha_ASM(SrcAddr, DstAddr, MinWidth);
        SrcAddr += SrcStride;
        DstAddr += DstStride;
    }
}


// This function will change the surface descriptor directly,
// So, caller should backup the surface descriptor or clone.
BOOL S3C6410Disp::CreateScratchSurface(GPESurf* OriginalSurface, DDGPESurf** ScratchSurface, PRECTL NewSurfaceSize, SURFACE_DESCRIPTOR *NewSurfaceDescriptor, EGPEFormat NewColorFormat, BOOL bCopy)
{
    DWORD dwTopOffset = 0;
    DWORD OriginalBuffer = (DWORD)OriginalSurface->Buffer();
    int OriginalStride = OriginalSurface->Stride();

    DWORD ScratchBuffer = 0;
    int ScratchStride = 0;
    int ScratchHeight = 0;

    // DirectDrawSurface has Align limitation
    AllocSurface((DDGPESurf**)ScratchSurface, ABS(SURFACE_WIDTH(OriginalSurface)),
                                RECT_HEIGHT(NewSurfaceSize),
                                NewColorFormat,
                                EGPEFormatToEDDGPEPixelFormat[NewColorFormat],
                                GPE_REQUIRE_VIDEO_MEMORY);
    if(ScratchSurface == NULL || *ScratchSurface == NULL)
    {
        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed for AlphaBlend %d\n"), __LINE__));
        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
        RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
        return FALSE;
    }

    ScratchBuffer = (DWORD)((*ScratchSurface)->Buffer());
    ScratchStride = (*ScratchSurface)->Stride();
    ScratchHeight = (*ScratchSurface)->Height();

    if(bCopy)
    {
        // Copy Original Image to ScratchBuffer for top stride of original rect
        // Scratch Surface's Stride can not be same to Original's one because of Alignment
        for(DWORD i = 0; i < (DWORD)ScratchHeight; i++)
        {
            memcpy((BYTE *)ScratchBuffer+ i * ABS(ScratchStride),
                (BYTE *)(OriginalBuffer + OriginalStride * (i + NewSurfaceSize->top)),
                ABS(OriginalStride)
                );
        }
    }

    // Assign Scratch Surface as Source Surface
    NewSurfaceDescriptor->dwBaseaddr = (m_VideoMemoryPhysicalBase + (*ScratchSurface)->OffsetInVideoMemory());
    NewSurfaceDescriptor->dwVertRes = RECT_HEIGHT(NewSurfaceSize);
    NewSurfaceSize->top = 0;
    NewSurfaceSize->bottom = NewSurfaceDescriptor->dwVertRes;

    return TRUE;
}
