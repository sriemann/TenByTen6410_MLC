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

Module Name:    

    line.cpp

Abstract:        

    draws accelerated lines

Functions:

Notes: 

--*/

#include "precomp.h"
#include <dispperf.h>

#define SIMPLE_LINEACCEL        //< This option enable Only Horizontal/Vertical Acceleration with 2D HW

SCODE S3C6410Disp::WrappedEmulatedLine(GPELineParms *pLineParms)
{
    SCODE retval;
    RECT  bounds;
    int   N_plus_1;                // Minor length of bounding rect + 1

    DEBUGMSG(GPE_ZONE_LINE, (TEXT("++%s()\r\n"), _T(__FUNCTION__)));
    // calculate the bounding-rect to determine overlap with cursor
    if (pLineParms->dN)            // The line has a diagonal component (we'll refresh the bounding rect)
    {
        N_plus_1 = 2 + ((pLineParms->cPels * pLineParms->dN) / pLineParms->dM);
    }
    else
    {
        N_plus_1 = 1;
    }

    switch(pLineParms->iDir)
    {
    case 0:
        bounds.left = pLineParms->xStart;
        bounds.top = pLineParms->yStart;
        bounds.right = pLineParms->xStart + pLineParms->cPels + 1;
        bounds.bottom = bounds.top + N_plus_1;
        break;

    case 1:
        bounds.left = pLineParms->xStart;
        bounds.top = pLineParms->yStart;
        bounds.bottom = pLineParms->yStart + pLineParms->cPels + 1;
        bounds.right = bounds.left + N_plus_1;
        break;

    case 2:
        bounds.right = pLineParms->xStart + 1;
        bounds.top = pLineParms->yStart;
        bounds.bottom = pLineParms->yStart + pLineParms->cPels + 1;
        bounds.left = bounds.right - N_plus_1;
        break;

    case 3:
        bounds.right = pLineParms->xStart + 1;
        bounds.top = pLineParms->yStart;
        bounds.left = pLineParms->xStart - pLineParms->cPels;
        bounds.bottom = bounds.top + N_plus_1;
        break;

    case 4:
        bounds.right = pLineParms->xStart + 1;
        bounds.bottom = pLineParms->yStart + 1;
        bounds.left = pLineParms->xStart - pLineParms->cPels;
        bounds.top = bounds.bottom - N_plus_1;
        break;

    case 5:
        bounds.right = pLineParms->xStart + 1;
        bounds.bottom = pLineParms->yStart + 1;
        bounds.top = pLineParms->yStart - pLineParms->cPels;
        bounds.left = bounds.right - N_plus_1;
        break;

    case 6:
        bounds.left = pLineParms->xStart;
        bounds.bottom = pLineParms->yStart + 1;
        bounds.top = pLineParms->yStart - pLineParms->cPels;
        bounds.right = bounds.left + N_plus_1;
        break;

    case 7:
        bounds.left = pLineParms->xStart;
        bounds.bottom = pLineParms->yStart + 1;
        bounds.right = pLineParms->xStart + pLineParms->cPels + 1;
        bounds.top = bounds.bottom - N_plus_1;
        break;

    default:
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] WrappedEmulatedLine() : Invalid Direction %d\n\r"), pLineParms->iDir));
        return E_INVALIDARG;
    }
    // check for line overlap with cursor and turn off cursor if overlaps
    if (m_CursorVisible && !m_CursorDisabled)
    {
        RotateRectl(&m_CursorRect);
        if (m_CursorRect.top < bounds.bottom && m_CursorRect.bottom > bounds.top &&
            m_CursorRect.left < bounds.right && m_CursorRect.right > bounds.left)
        {
            RotateRectlBack(&m_CursorRect);
            CursorOff();
            m_CursorForcedOff = TRUE;
        }
        else
            RotateRectlBack(&m_CursorRect);
    }

    if(m_VideoPowerState != VideoPowerOff && m_G2DControlArgs.HWOnOff && m_G2DControlArgs.UseLineDraw)
    {
        m_descDstSurface.dwColorMode = GetHWColorFormat(pLineParms->pDst);
        if( pLineParms->pDst->InVideoMemory() &&
            m_descDstSurface.dwColorMode != G2D_COLOR_UNUSED &&
           (pLineParms->pDst->Rotate()==DMDO_0) &&
           (pLineParms->mix == 0x0d0d) &&
           (pLineParms->style == 0) 
#ifdef SIMPLE_LINEACCEL
           && (pLineParms->dN == 0) &&
           (pLineParms->dM == pLineParms->cPels*16) 
#endif
             )
        {
            DispPerfType(DISPPERF_ACCEL_HARDWARE);
            m_oG2D->WaitForIdle();    //< Wait for Fully Empty Command Fifo for all HW Line Drawing Request    
            RETAILMSG(DISP_ZONE_LINE, (TEXT("ALine:(%d,%d) + m(%d), n(%d), cPels(%d), dir(%d)\n"), pLineParms->xStart, pLineParms->yStart, pLineParms->dM, pLineParms->dN, pLineParms->cPels, pLineParms->iDir));
            retval = AcceleratedSolidLine(pLineParms);
        }
        else
        {
            DEBUGMSG(GPE_ZONE_LINE, (TEXT("ELine:(%d,%d) + m(%d), n(%d), cPels(%d), dir(%d)\n"), pLineParms->xStart, pLineParms->yStart, pLineParms->dM, pLineParms->dN, pLineParms->cPels, pLineParms->iDir));
            retval = EmulatedLine (pLineParms);
        }
    }
    else
    {
        // do emulated line
        retval = EmulatedLine (pLineParms);
    }

    // se if cursor was forced off because of overlap with line bouneds and turn back on
        if (m_CursorForcedOff)
    {
        m_CursorForcedOff = FALSE;
        CursorOn();
    }

    DEBUGMSG(GPE_ZONE_LINE, (TEXT("--%s()\r\n"), _T(__FUNCTION__)));

    return retval;
}

/**
*    @fn    SCODE S3C6410DISP::AcceleratedSolidLine(GPELineParms *pLineParms)
*    @brief Draw Solid line with 1 width.
*    @param    pLineParms    Line Drawing Information Structure
*    @
*/

SCODE S3C6410Disp::AcceleratedSolidLine(GPELineParms *pLineParms)
{
    SCODE retval;

    retval = S_OK;

    int nMajorLength = pLineParms->dM / 16;
    int nMinorLength = pLineParms->dN / 16;
    int xStart = pLineParms->xStart;
    int yStart = pLineParms->yStart;
    int minorlength = 0;                        // default 0

    int xEnd = 1;
    int yEnd = 1;    

    /**
    *    Prepare DestinationSurface Information
    *
    */
    m_descDstSurface.dwBaseaddr = (m_VideoMemoryPhysicalBase + pLineParms->pDst->OffsetInVideoMemory());
    m_descDstSurface.dwHoriRes = pLineParms->pDst->Width();
    m_descDstSurface.dwVertRes = pLineParms->pDst->Height();

    m_oG2D->SetDstSurface(&m_descDstSurface);
    m_oG2D->SetClipWindow(pLineParms->prclClip);
    RETAILMSG(DISP_ZONE_LINE, (TEXT("ClipForLine:(%d,%d)~(%d,%d)\n"), pLineParms->prclClip->left, pLineParms->prclClip->top,
                        pLineParms->prclClip->right, pLineParms->prclClip->bottom));
    RETAILMSG(DISP_ZONE_LINE, (TEXT("dwBase:0x%x, m_iRotate:%d, DstRot:%d\r\n"), m_descDstSurface.dwBaseaddr, m_iRotate, pLineParms->pDst->Rotate()));
    m_oG2D->SetTransparentMode(0, 0x00000000L);
//    pLineParms->
    m_oG2D->SetRopEtype(ROP_SRC_ONLY);

    if(pLineParms->cPels == 1)
    {
        EnterCriticalSection(&m_cs2D);
        m_oG2D->PutPixel(xStart, yStart, pLineParms->solidColor);
        LeaveCriticalSection(&m_cs2D);
    }
    else
    {
#ifdef SIMPLE_LINEACCEL
        // Line is vertical or horizontal
        // Use fill-blt to draw a line starting at
        // pLineParms->xStart, pLineParms->yStart
        // in the direction specified by pLineParms->iDir
        // for a total of pLineParms->cPels pixels
        //           -y
        //           ^
        //     ¢Ø  5 | 6  ¢Ö
        //       ¢Ø  |  ¢Ö
        //      4  ¢Ø|¢Ö  7
        // -x<-------+-------> +x
        //      3  ¢×|¢Ù  0
        //       ¢×  |  ¢Ù
        //     ¢×  2 | 1  ¢Ù
        //             +y

        switch (pLineParms->iDir & 0x07)
        {
            // major axis is X-axis, dM = X-axis, dN = Y-axis
            case    0:                // +x +1/2y
                yEnd = yStart ;
                xEnd = xStart + pLineParms->cPels;
                break;
            case    1:                // +1/2x + y
                yEnd = yStart + pLineParms->cPels;
                xEnd = xStart;
                break;
            case    2:                // -1/2x + y
                yEnd = yStart + pLineParms->cPels;
                xEnd = xStart;
                break;
            case    3:                // -x + 1/2y
                yEnd = yStart;
                xEnd = xStart - pLineParms->cPels;
                break;
            case    4:                // -x - 1/2y
                yEnd = yStart;
                xEnd = xStart - pLineParms->cPels;
                break;
            case    5:                // -1/2x - y
                yEnd = yStart - pLineParms->cPels;
                xEnd = xStart;
                break;
            case    6:                // +1/2x - y
                yEnd = yStart - pLineParms->cPels;
                xEnd = xStart;
                break;
            case    7:                // +x -1/2y
                yEnd = yStart;
                xEnd = xStart + pLineParms->cPels;
                break;
        }


        bool IsLastDraw = false;

        if(xEnd < 0) xEnd = 0;        // Cut negative coordinate
        if(yEnd < 0) yEnd = 0;        // negative coordinate is not supported on H/W IP
        EnterCriticalSection(&m_cs2D);
        m_oG2D->PutLine(xStart, yStart, xEnd, yEnd, pLineParms->solidColor, IsLastDraw);
        LeaveCriticalSection(&m_cs2D);        
#else   // In Development
        if (pLineParms->dN)            // The line has a diagonal component (we'll refresh the bounding rect)
        {
            minorlength = ((pLineParms->cPels * pLineParms->dN) / pLineParms->dM);
        }

        // Line is vertical or horizontal
        // Use fill-blt to draw a line starting at
        // pLineParms->xStart, pLineParms->yStart
        // in the direction specified by pLineParms->iDir
        // for a total of pLineParms->cPels pixels
        //           -y
        //           ^
        //     ¢Ø  5 | 6  ¢Ö
        //       ¢Ø  |  ¢Ö
        //      4  ¢Ø|¢Ö  7
        // -x<-------+-------> +x
        //      3  ¢×|¢Ù  0
        //       ¢×  |  ¢Ù
        //     ¢×  2 | 1  ¢Ù
        //             +y

        switch (pLineParms->iDir & 0x07)
        {
            // major axis is X-axis, dM = X-axis, dN = Y-axis
            case    0:                // +x +1/2y
                yEnd = yStart + nMinorLength;
                xEnd = xStart + nMajorLength;
                break;
            case    1:                // +1/2x + y
                yEnd = yStart + nMajorLength;
                xEnd = xStart + nMinorLength;
                break;
            case    2:                // -1/2x + y
                yEnd = yStart + nMajorLength;
                xEnd = xStart - nMinorLength;
                break;
            case    3:                // -x + 1/2y
                yEnd = yStart + nMinorLength;
                xEnd = xStart - nMajorLength;
                break;
            case    4:                // -x - 1/2y
                yEnd = yStart - nMinorLength;
                xEnd = xStart - nMajorLength;
                break;
            case    5:                // -1/2x - y
                yEnd = yStart - nMajorLength;
                xEnd = xStart - nMinorLength;
                break;
            case    6:                // +1/2x - y
                yEnd = yStart - nMajorLength;
                xEnd = xStart + nMinorLength;
                break;
            case    7:                // +x -1/2y
                yEnd = yStart - nMinorLength;
                xEnd = xStart + nMajorLength;
                break;
        }


        bool IsLastDraw = false;
        int y_intercept = 0;
        int x_intercept = 0;
        int x1 = xStart * 16;        // interpolation
        int x2 = xEnd * 16;
        int y1 = yStart * 16;
        int y2 = yEnd * 16;

        ASSERT(xStart >=0);
        ASSERT(yStart >=0);
        // y=(y2-y1)/(x2-x1)x + b
        // b = (y1x2-x1y2)/(x2-x1)
        // 1. line is out over y-axis
        // y value when x=0 is y=b=(y1x2-x1y2)/(x2-x1)
        if(x2 < 0)
        {
            if(x2 == x1)    // do not draw. it is clipped.
            {
                return retval;
            }
            if(y2 == y1)    // Horizontal Line.
            {
                if(yEnd < 0) yEnd = 0;
            RETAILMSG(DISP_ZONE_LINE,(_T("ACCDRAW1 : LT(%d,%d)~RB(%d,%d)\n "), xStart, yStart, xEnd, yEnd));                
                m_oG2D->PutLine(xStart, yStart, 0, yEnd, pLineParms->solidColor, IsLastDraw);
                return retval;
            }

            y_intercept = ((y1*x2)-(x1*y2))/(x2-x1);

            if(y_intercept < 0 )    // Recalc for x
            {
                x_intercept = ((x1*y2)-(y1*x2))/(y2-y1);
                if(x_intercept < 0)
                {
                    RETAILMSG(DISP_ZONE_LINE,(_T("Line Draw error\n")));
                }
                else    // (xStart, yStart) ~ (x_intercept/16, 0)
                {
            RETAILMSG(DISP_ZONE_LINE,(_T("ACCDRAW2 : LT(%d,%d)~RB(%d,%d)\n "), xStart, yStart, xEnd, yEnd));                                    
                    m_oG2D->PutLine(xStart, yStart, x_intercept/16, 0, pLineParms->solidColor, IsLastDraw);
                    return retval;
                }
            }
            else    // (xStart, yStart) ~ (0, y_intercept/16)
            {
        RETAILMSG(DISP_ZONE_LINE,(_T("ACCDRAW3 : LT(%d,%d)~RB(%d,%d)\n "), xStart, yStart, xEnd, yEnd));                                
                    m_oG2D->PutLine(xStart, yStart, 0, y_intercept/16, pLineParms->solidColor, IsLastDraw);
                    return retval;
            }
        }
        else if(y2 < 0)
        {
            if(y1 == y2)        // do not draw. it is clipped
            {
                return retval;
            }
            if(x1 == x2)    // Vertical Line
            {
                if(xEnd < 0) xEnd = 0;
        RETAILMSG(DISP_ZONE_LINE,(_T("ACCDRAW4 : LT(%d,%d)~RB(%d,%d)\n "), xStart, yStart, xEnd, yEnd));                                
                m_oG2D->PutLine(xStart, yStart, xEnd, 0, pLineParms->solidColor, IsLastDraw);
                return retval;
            }
            x_intercept = ((x1*y2)-(y1*x2))/(y2-y1);
            if(x_intercept < 0)    // Recalc for y
            {
                y_intercept = ((y1*x2)-(x1*y2))/(x2-x1);
                if(y_intercept < 0)
                {
                    RETAILMSG(DISP_ZONE_LINE,(_T("Line Draw error\n")));
                }
                else    // (xStart, yStart) ~ (0, y_intercept)
                {
        RETAILMSG(DISP_ZONE_LINE,(_T("ACCDRAW5 : LT(%d,%d)~RB(%d,%d)\n "), xStart, yStart, xEnd, yEnd));                                    
                    m_oG2D->PutLine(xStart, yStart, 0, y_intercept/16, pLineParms->solidColor, IsLastDraw);
                    return retval;
                }
            }
            else    // (xStart, yStart) ~ (x_intercept, 0)
            {
        RETAILMSG(DISP_ZONE_LINE,(_T("ACCDRAW6 : LT(%d,%d)~RB(%d,%d)\n "), xStart, yStart, xEnd, yEnd));                                
                    m_oG2D->PutLine(xStart, yStart, x_intercept/16, 0, pLineParms->solidColor, IsLastDraw);
                    return retval;
            }
        }
        else
        {
        RETAILMSG(DISP_ZONE_LINE,(_T("ACCDRAW7 : LT(%d,%d)~RB(%d,%d)\n "), xStart, yStart, xEnd, yEnd));                                            
            m_oG2D->PutLine(xStart, yStart, xEnd, yEnd, pLineParms->solidColor, IsLastDraw);
            return retval;
        }
#endif
    }


    return retval;
}

SCODE
S3C6410Disp::Line(GPELineParms *pLineParms, EGPEPhase phase)
{
    DEBUGMSG (GPE_ZONE_INIT, (TEXT("++%s()\r\n"), _T(__FUNCTION__)));

    if (phase == gpeSingle || phase == gpePrepare)
    {
        DispPerfStart(ROP_LINE);
        pLineParms->pLine = (SCODE (GPE::*)(struct GPELineParms *))&S3C6410Disp::WrappedEmulatedLine;
    }
    else if (phase == gpeComplete)
    {
        DispPerfEnd(0);
        if(m_VideoPowerState != VideoPowerOff && m_G2DControlArgs.HWOnOff)
        {
            m_oG2D->WaitForIdle();    //< Wait for Engine Idle(Fully Empty Command Fifo) for all HW Line Drawing Request
        }
    }
    DEBUGMSG(GPE_ZONE_INIT, (TEXT("--%s()\r\n"), _T(__FUNCTION__)));

    return S_OK;
}
