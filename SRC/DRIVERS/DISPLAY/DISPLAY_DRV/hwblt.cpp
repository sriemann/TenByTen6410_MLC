//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    hwblt.cpp

Abstract:   accelerated bitblt/rectangle for S3C6410 FIMGSE-2D

Functions:  HWBitBlt


Notes:


--*/

#include "precomp.h"

/**
*   @fn     void S3C6410Disp::HWBitBlt(GpeBltParms *pBltParms)
*   @brief  Check Further Optional processing. This is intemediate layer between display driver and 2D driver
*   @param  pBltParms Blit Parameter Information Structure
*   @sa     GPEBltParms
*   @note   currently support only rotation
*   @note   DMDO_0 = 0, DMDO_90 = 1, DMDO_180 = 2, DMDO_270 = 4
*/
BOOL S3C6410Disp::HWBitBlt(GPEBltParms *pBltParms, PSURFACE_DESCRIPTOR pdescSrcSurface, PSURFACE_DESCRIPTOR pdescDstSurface)
{
    PRECTL  prclSrc;     //< Src is already rotated.
    PRECTL  prclDst;     //< Dst is already rotated.
    RECT    rectlPhySrc;       //< If screen is rotated, this value has RotateRectl(prclSrc), physically addressed coordinate.
    RECT    rectlPhyDst;        //< If screen is rotated, this value has RotateRectl(prclDst), physically addressed coordinate.
    UINT    uiSrcWidth_0;       //< Source Rectangle's width as non-rotated logically.
    UINT    uiDstWidth_0;       //< Destination Rectangle's width as non-rotated logically.
    UINT    uiSrcHeight_0;      //< Source Rectangle's height as non-rotated logically.
    UINT    uiDstHeight_0;      //< Destination Rectangle's height as non-rotated logically.

    prclSrc = pBltParms->prclSrc;
    prclDst = pBltParms->prclDst;

    CopyRect(&rectlPhySrc, (LPRECT)prclSrc);
    CopyRect(&rectlPhyDst, (LPRECT)prclDst);

    int     iRotate = DMDO_0;

    RotateRectlBack(prclSrc);
    RotateRectlBack(prclDst);

    /// Get Logical Aspect.
    uiSrcWidth_0 = ABS(prclSrc->right - prclSrc->left);
    uiSrcHeight_0 = ABS(prclSrc->bottom - prclSrc->top);
    uiDstWidth_0 = ABS(prclDst->right - prclDst->left);
    uiDstHeight_0 = ABS(prclDst->bottom - prclDst->top);

    RotateRectl(prclSrc);
    RotateRectl(prclDst);
    

    /// Set Relative Rotation Degree
    iRotate = GetRelativeDegree(pBltParms->pSrc->Rotate(), pBltParms->pDst->Rotate());

#define SWAP(x,y, _type)  { _type i; i = x; x = y; y = i; }
    RETAILMSG(DISP_ZONE_TEMP,(TEXT("HWBitBlt Src(%d,%d)~(%d,%d):%d, Dst(%d,%d)~(%d,%d):%d, RelativeDegree:%d\n"),
        prclSrc->left, prclSrc->top, prclSrc->right, prclSrc->bottom, pBltParms->pSrc->Rotate(),
        prclDst->left, prclDst->top, prclDst->right, prclDst->bottom, pBltParms->pDst->Rotate(), iRotate));
        
    /// if Stretch option is set, run StretchBlt
    /// if StretchBlt is on, but source region and destination region are same. use BitBlt
    if( ((pBltParms->bltFlags & BLT_STRETCH) == BLT_STRETCH) 
        //< If it does not need to stretch or shrink. just using BitBlt is faster.
        && !(( uiSrcWidth_0 == uiDstWidth_0) && (uiSrcHeight_0 == uiDstHeight_0))
        )
    {
        if( (prclSrc->left < 0) || (prclSrc->right < 0) || (prclSrc->top < 0) || (prclSrc->bottom < 0)
            || (prclDst->left < 0) || (prclDst->right < 0) || (prclDst->top < 0) || (prclDst->bottom <0))
        {
            /// If rotated coodinate has negative value. we can't care about when stretching is need.
            /// So return false, then process it by SW
            return FALSE;
        }

        RECTL   t_rect;
        DWORD   dwSrcWidth;
        DWORD   dwSrcHeight;
        SURFACE_DESCRIPTOR descScratch;

        /// This is physical value == physically 0 degree;
        dwSrcWidth = ABS(prclSrc->right - prclSrc->left);
        dwSrcHeight  = ABS(prclSrc->bottom - prclSrc->top);

        /// Set Scratch Destination Region
        t_rect.left = 0;
        t_rect.top = 0;
        t_rect.right = dwSrcWidth;
        t_rect.bottom = dwSrcHeight;

        RETAILMSG(DISP_ZONE_TEMP,(TEXT("t_rect,realstretch: (%d,%d)~(%d,%d), R:%d\r\n"), 
            t_rect.left,t_rect.top,t_rect.right,t_rect.bottom, iRotate));
        DumpBltParms(pBltParms);           

        /// Set Source Surface Descriptor 
        m_oG2D->SetSrcSurface(pdescSrcSurface);

        /// Check whether XY flip or not, 
        ///if XY flip is requested, just Rotation 180 degree
        RotateRectlBack(prclDst);
        if( (prclDst->right < prclDst->left)  && (prclDst->bottom < prclDst->top) )
        {
            RotateRectl(prclDst);
            switch(iRotate)
            {
            case DMDO_0:
                iRotate = DMDO_180;
                break;
            case DMDO_90:
                iRotate = DMDO_270;
                break;
            case DMDO_180:
                iRotate = DMDO_0;
                break;
            case DMDO_270:
                iRotate = DMDO_90;
                break;
            }

            /// SWAP rect
            SWAP(prclDst->top, prclDst->bottom, LONG);
            SWAP(prclDst->left, prclDst->right, LONG);

#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
            /// Adjust Coordinate from (x1,y1)~(x2,y2) to (0,0)~(x2-x1,y2-y1)
            /// and Recalculate BaseAddress
            m_oG2D->TranslateCoordinateToZero(pdescDstSurface, prclDst, pBltParms->prclClip);
#endif
            /// Set Destination Surface to real Framebuffer Surface
            m_oG2D->SetDstSurface(pdescDstSurface);

            /// Set Destination Clipping window Rect
            if(pBltParms->prclClip)
            {
                m_oG2D->SetClipWindow(pBltParms->prclClip);
            }
            else
            {
                m_oG2D->SetClipWindow(prclDst);
            }

            EnterCriticalSection(&m_cs2D);
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
            m_oG2D->StretchBlt_Bilinear( prclSrc, prclDst, m_oG2D->GetRotType(iRotate));
#else
            m_oG2D->StretchBlt( prclSrc, prclDst, m_oG2D->GetRotType(iRotate));
#endif
            LeaveCriticalSection(&m_cs2D);
            /// Recover rect
            SWAP(prclDst->top, prclDst->bottom, LONG);
            SWAP(prclDst->left, prclDst->right, LONG);

            //RotateRectl(prclDst);

            return TRUE;

        }
        RotateRectl(prclDst);


        /// Reconfigure HW to set destination framebuffer address as Scratch Framebuffer

        /// Check mirror case, and reset region rectangle
        /// Doing FlipBlt from Source to Sratch
        /// In mirror case, source region does not change.
        /// only destination's regions has reverse coordinate, this cannot be negative.
        if(iRotate == DMDO_90 || iRotate == DMDO_270)
        {
            /// back to logical value
            RotateRectlBack(prclDst);
            /// if left-right mirror case
            if(prclDst->right < prclDst->left)
            {
                RotateRectl(prclDst);
                /// Allocation Scratch Framebuffer for Flip Operation.
                DDGPESurf *ScratchSurf;

                AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                if(ScratchSurf == NULL)
                {
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                    RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                    PACSurf *ScratchSurf;
#endif
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                    return FALSE;
                }
                /// Set Scratch Surface Information
                descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                RETAILMSG(DISP_ZONE_TEMP,(TEXT("ScratchBaseAddr : 0x%x\n"), descScratch.dwBaseaddr));
                descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                descScratch.dwHoriRes = dwSrcWidth;
                descScratch.dwVertRes = dwSrcHeight;

                /// Set Destination Surface to Scratch Surface
                m_oG2D->SetDstSurface(&descScratch);
                /// Set Destination Clipping window Rect
                m_oG2D->SetClipWindow(&t_rect);
                /// Set Y-axis flip flag
                EnterCriticalSection(&m_cs2D);
                m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_Y );
                m_oG2D->WaitForIdle();
                LeaveCriticalSection(&m_cs2D);
                /// Y-axis mirror case. left-right inversion
                /// Set Source Address to Scratch Memory
                m_oG2D->SetSrcSurface(&descScratch);

                /// Swap top, left coordinate when 90 and 270
                RETAILMSG(DISP_ZONE_TEMP,(TEXT("S TBSWAP:%d,%d,%d,%d,%d,%d\n"),prclDst->left, prclDst->top, prclDst->right, prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
                SWAP(prclDst->top, prclDst->bottom, LONG);

#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
                /// Adjust Coordinate from (x1,y1)~(x2,y2) to (0,0)~(x2-x1,y2-y1)
                /// and Recalculate BaseAddress
                m_oG2D->TranslateCoordinateToZero(pdescDstSurface, prclDst, pBltParms->prclClip);
#endif
                /// Set Destination Surface to real Framebuffer Surface
                m_oG2D->SetDstSurface(pdescDstSurface);

                /// Set Destination Clipping window Rect
                if(pBltParms->prclClip)
                {
                    m_oG2D->SetClipWindow(pBltParms->prclClip);
                }
                else
                {
                    m_oG2D->SetClipWindow(prclDst);
                }

                EnterCriticalSection(&m_cs2D);
                RETAILMSG(DISP_ZONE_TEMP,(TEXT("S TASWAP:%d,%d,%d,%d,%d,%d\n"),prclDst->left, prclDst->top, prclDst->right, prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
                m_oG2D->StretchBlt_Bilinear( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#else
                m_oG2D->StretchBlt( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#endif
                LeaveCriticalSection(&m_cs2D);
                /// recover left, right coordinate
                SWAP(prclDst->top, prclDst->bottom, LONG);

                /// Disallocate Scratch Surface
                delete ScratchSurf;

                RETAILMSG(DISP_ZONE_TEMP, (TEXT("Stretch Y-axis flip: R:%d\r\n"), pBltParms->pDst->Rotate()));

                return TRUE;
            }
            /// if bottom-up mirror case
            if(prclDst->bottom < prclDst->top)
            {
                RotateRectl(prclDst);
                /// Allocation Scratch Framebuffer for Flip Operation.
                DDGPESurf *ScratchSurf;
                AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                if(ScratchSurf == NULL)
                {
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                    RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                    PACSurf *ScratchSurf;
#endif
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                    return FALSE;
                }

                /// Set Scratch Surface Information
                descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                RETAILMSG(DISP_ZONE_TEMP,(TEXT("ScratchBaseAddr : 0x%x, Offset:%d,SrcW:%d, SrcH:%d, Stride:%d, R:%d\n"), descScratch.dwBaseaddr, ScratchSurf->OffsetInVideoMemory(), dwSrcWidth, dwSrcHeight, ScratchSurf->Stride(), ScratchSurf->Rotate()));
                descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                descScratch.dwHoriRes = dwSrcWidth;
                descScratch.dwVertRes = dwSrcHeight;

                /// Set Destination Surface to Scratch Surface
                m_oG2D->SetDstSurface(&descScratch);
                /// Set Destination Clipping window Rect
                m_oG2D->SetClipWindow(&t_rect);
                /// Set X-axis flip flag
                EnterCriticalSection(&m_cs2D);
                m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_X );
                m_oG2D->WaitForIdle();
                LeaveCriticalSection(&m_cs2D);

                /// Set Source Address to Scratch Memory
                m_oG2D->SetSrcSurface(&descScratch);

                /// X-axis mirror case. up-down inversion
                /// Swap left, right coordinate when 90 and 270 degree
                RETAILMSG(DISP_ZONE_TEMP,(TEXT("S LRSWAP:%d,%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
                SWAP(prclDst->left, prclDst->right, LONG);


#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
                /// Adjust Coordinate from (x1,y1)~(x2,y2) to (0,0)~(x2-x1,y2-y1)
                /// and Recalculate BaseAddress
                m_oG2D->TranslateCoordinateToZero(pdescDstSurface, prclDst, pBltParms->prclClip);
#endif
                /// Set Destination Surface to real Framebuffer Surface
                m_oG2D->SetDstSurface(pdescDstSurface);

                /// Set Destination Clipping window Rect
                if(pBltParms->prclClip)
                {
                    m_oG2D->SetClipWindow(pBltParms->prclClip);
                }
                else
                {
                    m_oG2D->SetClipWindow(prclDst);
                }

                EnterCriticalSection(&m_cs2D);
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)    
                m_oG2D->StretchBlt_Bilinear( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#else
                m_oG2D->StretchBlt( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#endif
                LeaveCriticalSection(&m_cs2D);
                /// recover top, bottom coordinate
                SWAP(prclDst->left, prclDst->right, LONG);

                /// Disallocate Scratch Surface
                delete ScratchSurf;

                RETAILMSG(DISP_ZONE_TEMP, (TEXT("Stretch X-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));

                return TRUE;
            }
            RotateRectl(prclDst);
            /// Non flip but rotated.
        }
        else        //< DMDO_0 and DMDO_180 does not need to modify prclDst region
        {
            RotateRectlBack(prclDst);
            /// Check Left-Right flip, when stretching is needed.
            if(prclDst->right < prclDst->left)
            {
                RotateRectl(prclDst);
                /// Allocation Scratch Framebuffer for Flip Operation.
                DDGPESurf *ScratchSurf;

                AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                if(ScratchSurf == NULL)
                {
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                    RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                    PACSurf *ScratchSurf;
#endif
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                    return FALSE;
                }
                /// Set Scratch Surface Information
                descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                descScratch.dwHoriRes = dwSrcWidth;
                descScratch.dwVertRes = dwSrcHeight;

                /// Set Destination Surface to Scratch Surface
                m_oG2D->SetDstSurface(&descScratch);
                /// Set Destination Clipping window Rect
                m_oG2D->SetClipWindow(&t_rect);

                /// Set Y-axis flip flag
                EnterCriticalSection(&m_cs2D);
                m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_Y );
                m_oG2D->WaitForIdle();
                LeaveCriticalSection(&m_cs2D);
                /// Y-axis mirror case. left-right inversion
                /// Set Source Address to Scratch Memory
                m_oG2D->SetSrcSurface(&descScratch);

                /// Swap left, right coordinate
                RETAILMSG(DISP_ZONE_TEMP,(TEXT("S 0,180 BSWAP : %d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom));
                SWAP(prclDst->right, prclDst->left, LONG);
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)    
                /// Adjust Coordinate from (x1,y1)~(x2,y2) to (0,0)~(x2-x1,y2-y1)
                /// and Recalculate BaseAddress
                m_oG2D->TranslateCoordinateToZero(pdescDstSurface, prclDst, pBltParms->prclClip);
#endif
                /// Set Destination Surface to real Framebuffer Surface
                m_oG2D->SetDstSurface(pdescDstSurface);

                /// Set Destination Clipping window Rect
                if(pBltParms->prclClip)
                {
                    m_oG2D->SetClipWindow(pBltParms->prclClip);
                }
                else
                {
                    m_oG2D->SetClipWindow(prclDst);
                }

                RETAILMSG(DISP_ZONE_TEMP,(TEXT("S 0,180 ASWAP : %d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom));
                EnterCriticalSection(&m_cs2D);
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
                m_oG2D->StretchBlt( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#else
                m_oG2D->StretchBlt( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#endif
                LeaveCriticalSection(&m_cs2D);
                /// recover left, right coordinate
                SWAP(prclDst->right, prclDst->left, LONG);

                /// Disallocate Scratch Surface
                delete ScratchSurf;

                RETAILMSG(DISP_ZONE_TEMP, (TEXT("Stretch Y-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));

                return TRUE;
            }
            /// Check Up-Down flip, when stretching is needed.
            if(prclDst->bottom < prclDst->top)
            {
                RotateRectl(prclDst);
                /// Allocation Scratch Framebuffer for Flip Operation.
                DDGPESurf *ScratchSurf;
                AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                if(ScratchSurf == NULL)
                {
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                    RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                    PACSurf *ScratchSurf;
#endif
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                    RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                    return FALSE;
                }
                /// Set Scratch Surface Information
                descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                descScratch.dwHoriRes = dwSrcWidth;
                descScratch.dwVertRes = dwSrcHeight;

                /// Set Destination Surface to Scratch Surface
                m_oG2D->SetDstSurface(&descScratch);
                /// Set Destination Clipping window Rect
                m_oG2D->SetClipWindow(&t_rect);

                /// Set X-axis flip flag
                EnterCriticalSection(&m_cs2D);
                m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_X );
                m_oG2D->WaitForIdle();
                LeaveCriticalSection(&m_cs2D);

                /// Set Source Address to Scratch Memory
                m_oG2D->SetSrcSurface(&descScratch);

                /// X-axis mirror case. up-down inversion
                /// Swap top, bottom coordinate
                SWAP(prclDst->top, prclDst->bottom, LONG);

#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
                /// Adjust Coordinate from (x1,y1)~(x2,y2) to (0,0)~(x2-x1,y2-y1)
                /// and Recalculate BaseAddress
                m_oG2D->TranslateCoordinateToZero(pdescDstSurface, prclDst, pBltParms->prclClip);
#endif
                /// Set Destination Surface to real Framebuffer Surface
                m_oG2D->SetDstSurface(pdescDstSurface);

                /// Set Destination Clipping window Rect
                if(pBltParms->prclClip)
                {
                    m_oG2D->SetClipWindow(pBltParms->prclClip);
                }
                else
                {
                    m_oG2D->SetClipWindow(prclDst);
                }

                EnterCriticalSection(&m_cs2D);
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
                m_oG2D->StretchBlt_Bilinear( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#else
                m_oG2D->StretchBlt( &t_rect, prclDst, m_oG2D->GetRotType(iRotate));
#endif
                LeaveCriticalSection(&m_cs2D);
                /// recover top, bottom coordinate
                SWAP(prclDst->top, prclDst->bottom, LONG);

                /// Disallocate Scratch Surface
                delete ScratchSurf;

                RETAILMSG(DISP_ZONE_TEMP, (TEXT("Stretch X-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));

                return TRUE;
            }
            RotateRectl(prclDst);
        }
        /// cover 0, 90, 180, 270 but no flip
        RETAILMSG(DISP_ZONE_TEMP,(TEXT("S BRNONSWAP:%d,%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));

 //       if(pBltParms->pDst->Rotate() == DMDO_90 || pBltParms->pDst->Rotate() == DMDO_270)
//        {
//            RotateRectl(prclDst);
//            RotateRectl(prclDst);
//        }
        RETAILMSG(DISP_ZONE_TEMP,(TEXT("S BZNONSWAP:%d,%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
        /// This case does not need to flip.
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
        /// Adjust Coordinate from (x1,y1)~(x2,y2) to (0,0)~(x2-x1,y2-y1)
        /// and Recalculate BaseAddress
        m_oG2D->TranslateCoordinateToZero(pdescDstSurface, prclDst, pBltParms->prclClip);
#endif
        RETAILMSG(DISP_ZONE_TEMP,(TEXT("S NONSWAP:%d,%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
        /// Set Destination Surface to real Framebuffer Surface
        m_oG2D->SetDstSurface(pdescDstSurface);
        /// Set Destination Clipping window Rect
        if(pBltParms->prclClip)
        {
            m_oG2D->SetClipWindow(pBltParms->prclClip);
        }
        else
        {
            m_oG2D->SetClipWindow(prclDst);
        }

        EnterCriticalSection(&m_cs2D);
#if (G2D_STRETCH_ALGORITHM==G2D_QUARTERED_ADJUSTING)
        m_oG2D->StretchBlt_Bilinear( prclSrc, prclDst, m_oG2D->GetRotType(iRotate));
#else
        m_oG2D->StretchBlt( prclSrc, prclDst, m_oG2D->GetRotType(iRotate));
#endif
        LeaveCriticalSection(&m_cs2D);

        RETAILMSG(DISP_ZONE_TEMP, (TEXT("\nStretch no flip: R:%d\r\n"),pBltParms->pDst->Rotate()));
    }
    else    // Do not stretch.
    {
        /// Set Source Surface Descriptor
        m_oG2D->SetSrcSurface(pdescSrcSurface);

        /// Check whether XY flip or not, 
        ///if XY flip is requested, just Rotation 180 degree
        RotateRectlBack(prclDst);
        if( ((pBltParms->bltFlags & BLT_STRETCH) == BLT_STRETCH)
            && (prclDst->right < prclDst->left)  && (prclDst->bottom < prclDst->top) )
        {
            RotateRectl(prclDst);
            switch(iRotate)
            {
            case DMDO_0:
                iRotate = DMDO_180;
                break;
            case DMDO_90:
                iRotate = DMDO_270;
                break;
            case DMDO_180:
                iRotate = DMDO_0;
                break;
            case DMDO_270:
                iRotate = DMDO_90;
                break;
            }

            /// Set Destination Surface to real Framebuffer Surface
            m_oG2D->SetDstSurface(pdescDstSurface);
            /// Set Destination Clipping window Rect
            if(pBltParms->prclClip)
            {
                m_oG2D->SetClipWindow(pBltParms->prclClip);
            }
            else
            {
                m_oG2D->SetClipWindow(prclDst);
            }

            /// SWAP rect
            SWAP(prclDst->top, prclDst->bottom, LONG);
            SWAP(prclDst->left, prclDst->right, LONG);

            EnterCriticalSection(&m_cs2D);
            m_oG2D->BitBlt( prclSrc, prclDst, m_oG2D->GetRotType(iRotate));
            LeaveCriticalSection(&m_cs2D);
            /// Recover rect
            SWAP(prclDst->top, prclDst->bottom, LONG);
            SWAP(prclDst->left, prclDst->right, LONG);

            RETAILMSG(DISP_ZONE_TEMP,(TEXT("XY Flip R:%d\n"), iRotate));

            return TRUE;
        }
        //RotateRectl(prclDst);
        //RotateRectlBack(prclDst);

        /// Mirroring is needed.
        if( ((pBltParms->bltFlags & BLT_STRETCH) == BLT_STRETCH) 
            &&  ((prclDst->left > prclDst->right) || (prclDst->top > prclDst->bottom) )
            )
        {
            RotateRectl(prclDst);

            RECTL   t_rect;
            DWORD   dwSrcWidth;
            DWORD   dwSrcHeight;
            SURFACE_DESCRIPTOR descScratch;

            dwSrcWidth = ABS(prclSrc->right - prclSrc->left);
            dwSrcHeight  = ABS(prclSrc->bottom - prclSrc->top);

            /// Set Scratch Destination Region
            t_rect.left = 0;
            t_rect.top = 0;
            t_rect.right = dwSrcWidth;
            t_rect.bottom = dwSrcHeight;

            RETAILMSG(DISP_ZONE_TEMP,(TEXT("t_rect,justbitbltflip(%d,%d)~(%d,%d), R:%d\r\n"), 
                t_rect.left,t_rect.top,t_rect.right,t_rect.bottom,iRotate));
            DumpBltParms(pBltParms);

            /// In mirror case, source region does not change.
            /// only destination's regions has reverse coordinate, this cannot be negative.

            /// This is Difference between source and destination.
            if(iRotate == DMDO_0)   
            {
                //  RotateRectlBack(prclDst);
                if(prclDst->right < prclDst->left)
                {
                //  RotateRectl(prclDst);
                    RETAILMSG(DISP_ZONE_TEMP, (TEXT("BitBlt Y-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));
                    /// Set Destination Surface to real Framebuffer Surface
                    m_oG2D->SetDstSurface(pdescDstSurface);

                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("BY TBSWAP:%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate));
                    /// Y-axis mirror case. left-right inversion
                    /// Swap left, right coordinate
                    SWAP(prclDst->right, prclDst->left, LONG);

                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("BY TASWAP:%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate));
                    /// Set Destination Clipping window Rect
                    if(pBltParms->prclClip)
                    {
                        m_oG2D->SetClipWindow(pBltParms->prclClip);
                    }
                    else
                    {
                        m_oG2D->SetClipWindow(prclDst);
                    }

                    /// Set Y-axis flip flag
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->FlipBlt( prclSrc, prclDst,  FLIP_Y );
                    LeaveCriticalSection(&m_cs2D);
                    /// recover left, right coordinate
                    SWAP(prclDst->right, prclDst->left, LONG);

                    return TRUE;
                }
                else if(prclDst->bottom < prclDst->top)
                {
                    //  RotateRectl(prclDst);
                    RETAILMSG(DISP_ZONE_TEMP, (TEXT("BitBlt X-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));
                    /// Set Destination Surface to real Framebuffer Surface
                    m_oG2D->SetDstSurface(pdescDstSurface);

                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("BX TBSWAP:%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate));
                    /// X-axis mirror case. up-down inversion
                    /// Swap top, bottom coordinate
                    SWAP(prclDst->top, prclDst->bottom, LONG);
                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("BX TASWAP:%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate));
                    /// Set Destination Clipping window Rect
                    if(pBltParms->prclClip)
                    {
                        m_oG2D->SetClipWindow(pBltParms->prclClip);
                    }
                    else
                    {
                        m_oG2D->SetClipWindow(prclDst);
                    }

                    /// Set X-axis flip flag
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->FlipBlt( prclSrc, prclDst,  FLIP_X );
                    LeaveCriticalSection(&m_cs2D);
                    /// recover top, bottom coordinate
                    SWAP(prclDst->top, prclDst->bottom, LONG);

                    return TRUE;
                   }
                //  RotateRectl(prclDst);
            }
            else if(iRotate == DMDO_90 || iRotate == DMDO_270)
            {
                RotateRectlBack(prclDst);
                /// Original Coordinate
                RETAILMSG(DISP_ZONE_TEMP, (TEXT("R:%d, DR:%d, DST(%d,%d,%d,%d)\r\n"),pBltParms->pDst->Rotate(), iRotate, 
                    pBltParms->prclDst->left,
                    pBltParms->prclDst->top,
                    pBltParms->prclDst->right,
                    pBltParms->prclDst->bottom));
                /// if screen rotation is not DMDO_0. we need to bitblt once more. and use scratch memory
                if(prclDst->right < prclDst->left)
                {
                    RotateRectl(prclDst);
                    /// Screen rotated 
                    /// if 90, 270 degree, that is T,B Swaped and rotate 
                    ///  +-----+ (L,T)
                    ///  |     |
                    ///  |     |       
                    ///  |     |
                    ///  +-----+
                    /// (R,B)

                    /// Allocation Scratch Framebuffer for Flip Operation.
                    DDGPESurf *ScratchSurf;

                    AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                    if(ScratchSurf == NULL)
                    {
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                        RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                        PACSurf *ScratchSurf;
#endif
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                        return FALSE;
                    }

                    /// Set Scratch Surface Information
                    descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                    descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                    descScratch.dwHoriRes = dwSrcWidth;
                    descScratch.dwVertRes = dwSrcHeight;

                    /// Set Destination Surface as Scratch Surface
                    m_oG2D->SetDstSurface(&descScratch);
                    /// Set Destination Clipping window Rect
                    m_oG2D->SetClipWindow(&t_rect);

                    /// Y-axis mirror case. left-right inversion

                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_Y );
                    m_oG2D->WaitForIdle();
                    LeaveCriticalSection(&m_cs2D);

                    /// Set Source Address to Scratch Memory
                    m_oG2D->SetSrcSurface(&descScratch);
                    /// Set Destination Surface to real Framebuffer Surface
                    m_oG2D->SetDstSurface(pdescDstSurface);

                    /// Swap left, right coordinate
                    /// Y-axis mirror case. left-right inversion
                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("B TBSWAP:%d,%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
                    SWAP(prclDst->top, prclDst->bottom, LONG);
                    /// Set Destination Clipping window Rect
                    if(pBltParms->prclClip)
                    {
                        m_oG2D->SetClipWindow(pBltParms->prclClip);
                    }
                    else
                    {
                        m_oG2D->SetClipWindow(prclDst);
                    }

                    /// Set Y-axis flip flag
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->BitBlt( &t_rect, prclDst,  m_oG2D->GetRotType(iRotate) );
                    LeaveCriticalSection(&m_cs2D);
                    /// recover left, right coordinate
                    SWAP(prclDst->top, prclDst->bottom, LONG);

                    /// Disallocate Scratch Surface
                    delete ScratchSurf;

                    RETAILMSG(DISP_ZONE_TEMP, (TEXT("BitBlt Y-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));

                    //  RotateRectl(prclDst);

                    return TRUE;

                }
                else if(prclDst->bottom < prclDst->top)
                {
                    RotateRectl(prclDst);
                    /// Allocation Scratch Framebuffer for Flip Operation.
                    DDGPESurf *ScratchSurf;

                    AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                    if(ScratchSurf == NULL)
                    {
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                        RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                        PACSurf *ScratchSurf;
#endif
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                        return FALSE;
                    }

                    /// Set Scratch Surface Information
                    descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                    descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                    descScratch.dwHoriRes = dwSrcWidth;
                    descScratch.dwVertRes = dwSrcHeight;

                    /// Set Destination Surface to Scratch Surface
                    m_oG2D->SetDstSurface(&descScratch);
                    /// Set Destination Clipping window Rect
                    m_oG2D->SetClipWindow(&t_rect);

                    /// X-axis mirror case. top-bottom inversion
                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("B LRSWAP:%d,%d,%d,%d,%d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_X );
                    m_oG2D->WaitForIdle();
                    LeaveCriticalSection(&m_cs2D);

                    /// Set Source Address to Scratch Memory
                    m_oG2D->SetSrcSurface(&descScratch);
                    /// Set Destination Surface to real Framebuffer Surface
                    m_oG2D->SetDstSurface(pdescDstSurface);

                    /// Swap left, right coordinate    
                    /// LT <-> RB
                    SWAP(prclDst->left, prclDst->right, LONG);
                    /// Set Destination Clipping window Rect
                    if(pBltParms->prclClip)
                    {
                        m_oG2D->SetClipWindow(pBltParms->prclClip);
                    }
                    else
                    {
                        m_oG2D->SetClipWindow(prclDst);
                    }

                    /// Set Y-axis flip flag
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->BitBlt( &t_rect, prclDst,  m_oG2D->GetRotType(iRotate) );
                    LeaveCriticalSection(&m_cs2D);
                    /// recover left, right coordinate
                    SWAP(prclDst->left, prclDst->right, LONG);

                    /// Disallocate Scratch Surface
                    delete ScratchSurf;

                    RETAILMSG(DISP_ZONE_TEMP, (TEXT("BitBlt X-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));

                    return TRUE;
                }
                RotateRectl(pBltParms->prclDst);
            }
            else    //< DMDO_180
            {
                RotateRectlBack(prclDst);
                /// if screen rotation is not DMDO_0. we need to bitblt once more. and use scratch memory
                if(prclDst->right < prclDst->left)
                {
                    RotateRectl(prclDst);
                    RETAILMSG(DISP_ZONE_TEMP, (TEXT("BitBlt Y-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));

                    /// Allocation Scratch Framebuffer for Flip Operation.
                    DDGPESurf *ScratchSurf;

                    AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                    if(ScratchSurf == NULL)
                    {
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                        RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                        PACSurf *ScratchSurf;
#endif
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                        return FALSE;
                    }

                    /// Set Scratch Surface Information
                    descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                    descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                    descScratch.dwHoriRes = dwSrcWidth;
                    descScratch.dwVertRes = dwSrcHeight;

                    /// Set Destination Surface to Scratch Surface
                    m_oG2D->SetDstSurface(&descScratch);
                    /// Set Destination Clipping window Rect
                    m_oG2D->SetClipWindow(&t_rect);

                    /// Y-axis mirror case. left-right inversion
                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("(%d,%d)~(%d,%d) %d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_Y );
                    m_oG2D->WaitForIdle();
                    LeaveCriticalSection(&m_cs2D);

                    /// Set Source Address to Scratch Memory
                    m_oG2D->SetSrcSurface(&descScratch);
                    /// Set Destination Surface to real Framebuffer Surface
                    m_oG2D->SetDstSurface(pdescDstSurface);

                    /// Swap left, right coordinate
                    SWAP(prclDst->right, prclDst->left, LONG);
                    /// Set Destination Clipping window Rect
                    if(pBltParms->prclClip)
                    {
                        m_oG2D->SetClipWindow(pBltParms->prclClip);
                    }
                    else
                    {
                        m_oG2D->SetClipWindow(prclDst);
                    }

                    /// Set Y-axis flip flag
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->BitBlt( &t_rect, prclDst,  m_oG2D->GetRotType(iRotate) );
                    LeaveCriticalSection(&m_cs2D);
                    /// recover left, right coordinate
                    SWAP(prclDst->right, prclDst->left, LONG);

                    /// Disallocate Scratch Surface
                    delete ScratchSurf;

                    return TRUE;

                }
                else if(prclDst->bottom < prclDst->top)
                {
                    RotateRectl(prclDst);
                    RETAILMSG(DISP_ZONE_TEMP, (TEXT("BitBlt X-axis flip: R:%d\r\n"),pBltParms->pDst->Rotate()));
                    /// Allocation Scratch Framebuffer for Flip Operation.
                    DDGPESurf *ScratchSurf;

                    AllocSurface(&ScratchSurf, dwSrcWidth, dwSrcHeight, pBltParms->pDst->Format(), EGPEFormatToEDDGPEPixelFormat[pBltParms->pDst->Format()], GPE_REQUIRE_VIDEO_MEMORY);
                    if(ScratchSurf == NULL)
                    {
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Scratch Surface Allocation is failed. %d\n"), __LINE__));
#if 0//USE_PACSURF, To increase video memory is better than to use system memory as Video Surface
                        RETAILMSG(TRUE,(TEXT("try to allocate surface usign PA Surface\r\n")));
                        PACSurf *ScratchSurf;
#endif
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("Maybe There's no sufficient video memory. please increase video memory\r\n")));
                        RETAILMSG(DISP_ZONE_WARNING,(TEXT("try to redirect to SW Emulated Bitblt\r\n")));
                        return FALSE;
                    }

                    /// Set Scratch Surface Information
                    descScratch.dwBaseaddr = (m_VideoMemoryPhysicalBase + ScratchSurf->OffsetInVideoMemory());
                    descScratch.dwColorMode = GetHWColorFormat(pBltParms->pDst);
                    descScratch.dwHoriRes = dwSrcWidth;
                    descScratch.dwVertRes = dwSrcHeight;

                    /// Set Destination Surface to Scratch Surface
                    m_oG2D->SetDstSurface(&descScratch);
                    /// Set Destination Clipping window Rect
                    m_oG2D->SetClipWindow(&t_rect);

                    /// X-axis mirror case. top-down inversion
                    RETAILMSG(DISP_ZONE_TEMP,(TEXT("(%d,%d)~(%d,%d) %d,%d\n"),pBltParms->prclDst->left,pBltParms->prclDst->top,pBltParms->prclDst->right,pBltParms->prclDst->bottom, iRotate, m_oG2D->GetRotType(iRotate)));
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->FlipBlt( prclSrc, &t_rect,  FLIP_X );
                    m_oG2D->WaitForIdle();
                    LeaveCriticalSection(&m_cs2D);

                    /// Set Source Address to Scratch Memory
                    m_oG2D->SetSrcSurface(&descScratch);
                    /// Set Destination Surface to real Framebuffer Surface
                    m_oG2D->SetDstSurface(pdescDstSurface);

                    /// Swap left, right coordinate
                    SWAP(prclDst->top, prclDst->bottom, LONG);
                    /// Set Destination Clipping window Rect
                    if(pBltParms->prclClip)
                    {
                        m_oG2D->SetClipWindow(pBltParms->prclClip);
                    }
                    else
                    {
                        m_oG2D->SetClipWindow(prclDst);
                    }
                    /// Set Y-axis flip flag
                    EnterCriticalSection(&m_cs2D);
                    m_oG2D->BitBlt( &t_rect, prclDst,  m_oG2D->GetRotType(iRotate) );
                    LeaveCriticalSection(&m_cs2D);
                    /// recover left, right coordinate
                    SWAP(prclDst->top, prclDst->bottom, LONG);

                    /// Disallocate Scratch Surface
                    delete ScratchSurf;

                    return TRUE;
                }
                RotateRectl(prclDst);
            }
        }
        RETAILMSG(DISP_ZONE_TEMP,(TEXT("SimpleBitBlt\n")));
        RotateRectl(prclDst);
        /// Set Destination Surface to real Framebuffer Surface
        m_oG2D->SetDstSurface(pdescDstSurface);
        /// Set Destination Clipping window Rect
        if(pBltParms->prclClip)
        {
            m_oG2D->SetClipWindow(pBltParms->prclClip);
        }
        else
        {
            m_oG2D->SetClipWindow(prclDst);
        }

        if(pBltParms->bltFlags & BLT_ALPHABLEND)
            DumpBltParms(pBltParms);

        EnterCriticalSection(&m_cs2D);
        m_oG2D->BitBlt( prclSrc, prclDst,  m_oG2D->GetRotType(iRotate) );
        LeaveCriticalSection(&m_cs2D);

    }
    return TRUE;
}

/**
*   @fn     int S3C6410Disp::GetRelativeDegree(int SourceDegree, int DestinationDegree)
*   @brief  Get Reletive Rotation Degree From Source Degree and Destination Degreee
*   @param  pBltParms Blit Parameter Information Structure
*   @note   DMDO_0 = 0, DMDO_90 = 1, DMDO_180 = 2, DMDO_270 = 4
*/
int S3C6410Disp::GetRelativeDegree(int SrcDegree, int DstDegree)
{
    switch(SrcDegree)    
    {
    default:
    case DMDO_0:
        switch(DstDegree)
        {
        default:
        case DMDO_0:    return DMDO_0;
        case DMDO_90:   return DMDO_90;
        case DMDO_180:  return DMDO_180;
        case DMDO_270:  return DMDO_270;
        }
    case DMDO_90:
        switch(DstDegree)
        {
        default:
        case DMDO_0:    return DMDO_270;
        case DMDO_90:   return DMDO_0;
        case DMDO_180:  return DMDO_90;
        case DMDO_270:  return DMDO_180;
        }
    case DMDO_180:
        switch(DstDegree)
        {
        default:
        case DMDO_0:    return DMDO_180;
        case DMDO_90:   return DMDO_270;
        case DMDO_180:  return DMDO_0;
        case DMDO_270:  return DMDO_90;
        }
    case DMDO_270:
        switch(DstDegree)
        {
        default:
        case DMDO_0:    return DMDO_90;
        case DMDO_90:   return DMDO_180;
        case DMDO_180:  return DMDO_270;
        case DMDO_270:  return DMDO_0;
        }
    }
}

void S3C6410Disp::DumpBltParms(GPEBltParms *pBltParms)
{
    if(pBltParms->pSrc)
    {
    RETAILMSG(DISP_ZONE_BLT_LO,(TEXT("Src:0x%x SrcB 0x%x, Surf(W:%d,H:%d,BPP:%d,STRIDE:%d), Screen(W:%d,H:%d), rect: (%d,%d)~(%d,%d), R:%d\r\n"), 
        pBltParms->pSrc,
        pBltParms->pSrc->Buffer(),
        pBltParms->pSrc->Width(),
        pBltParms->pSrc->Height(),
        EGPEFormatToBpp[pBltParms->pSrc->Format()],
        pBltParms->pSrc->Stride(),
        pBltParms->pSrc->ScreenWidth(),
        pBltParms->pSrc->ScreenHeight(),
        pBltParms->prclSrc->left,
        pBltParms->prclSrc->top,
        pBltParms->prclSrc->right,
        pBltParms->prclSrc->bottom,
        pBltParms->pSrc->Rotate()
        ));
    }
    if(pBltParms->pDst)
    {
    RETAILMSG(DISP_ZONE_BLT_LO,(TEXT("Dst:0x%x DstB 0x%x,  Surf(W:%d,H:%d,BPP:%d,STRIDE:%d), Screen(W:%d,H:%d), rect: (%d,%d)~(%d,%d), R:%d\r\n"), 
        pBltParms->pDst,
        pBltParms->pDst->Buffer(),
        pBltParms->pDst->Width(),
        pBltParms->pDst->Height(),
        EGPEFormatToBpp[pBltParms->pDst->Format()],
        pBltParms->pDst->Stride(),
        pBltParms->pDst->ScreenWidth(),
        pBltParms->pDst->ScreenHeight(),
        pBltParms->prclDst->left,
        pBltParms->prclDst->top,
        pBltParms->prclDst->right,
        pBltParms->prclDst->bottom,
        pBltParms->pDst->Rotate()
        ));
    }
    if(pBltParms->prclClip)
    {
    RETAILMSG(DISP_ZONE_BLT_LO,(TEXT("ClipRegion (%d,%d)~(%d,%d)\r\n"), 
        pBltParms->prclClip->left,
        pBltParms->prclClip->top,
        pBltParms->prclClip->right,
        pBltParms->prclClip->bottom
        ));                                    
    }    
    RETAILMSG(DISP_ZONE_BLT_LO, (TEXT("ROP : 0x%0x, iMode:%d\r\n"), pBltParms->rop4, pBltParms->iMode));
    RETAILMSG(DISP_ZONE_BLT_LO, (TEXT("BlendFunction : Op(%d), Flag(%d),SourceConstantAlpha(%d),AlphaFormat(%d)\r\n"),
        pBltParms->blendFunction.BlendOp,
        pBltParms->blendFunction.BlendFlags, 
        pBltParms->blendFunction.SourceConstantAlpha, pBltParms->blendFunction.AlphaFormat));    
    RETAILMSG(DISP_ZONE_BLT_LO, (TEXT("xPositive : %d, yPositive : %d\r\n"),pBltParms->xPositive, pBltParms->yPositive));
}
