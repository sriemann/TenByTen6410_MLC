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

    cursor.cpp

Abstract:

    This module implement the main class that derived from DDGPE of display driver to support DirectDraw
    In this part, there are codes related with Cursoe

Functions:

    SetPointerShape, MovePointer, CursorOn, CursorOff

Notes:

--*/

#include "precomp.h"

void
S3C6410Disp::CursorOn (void)
{
    UCHAR *ptrScreen = (UCHAR*)m_pPrimarySurface->Buffer();
    UCHAR *ptrLine;
    UCHAR *cbsLine;
    int x, y;

    if (!m_CursorForcedOff && !m_CursorDisabled && !m_CursorVisible)
    {
        RECTL cursorRectSave = m_CursorRect;
        int   iRotate;

        RotateRectl(&m_CursorRect);
        for (y = m_CursorRect.top; y < m_CursorRect.bottom; y++)
        {
            if (y < 0)
            {
                continue;
            }
            if (y >= m_nScreenHeightSave)
            {
                break;
            }

            ptrLine = &ptrScreen[y * m_pPrimarySurface->Stride()];
            cbsLine = &m_CursorBackingStore[(y - m_CursorRect.top) * (m_CursorSize.x * (m_pMode->Bpp >> 3))];

            for (x = m_CursorRect.left; x < m_CursorRect.right; x++)
            {
                if (x < 0)
                {
                    continue;
                }

                if (x >= m_nScreenWidthSave)
                {
                    break;
                }

                // x' = x - m_CursorRect.left; y' = y - m_CursorRect.top;
                // Width = m_CursorSize.x;   Height = m_CursorSize.y;
                switch (m_iRotate)
                {
                case DMDO_0:
                    iRotate = (y - m_CursorRect.top)*m_CursorSize.x + x - m_CursorRect.left;
                    break;

                case DMDO_90:
                    iRotate = (x - m_CursorRect.left)*m_CursorSize.x + m_CursorSize.y - 1 - (y - m_CursorRect.top);
                    break;

                case DMDO_180:
                    iRotate = (m_CursorSize.y - 1 - (y - m_CursorRect.top))*m_CursorSize.x + m_CursorSize.x - 1 - (x - m_CursorRect.left);
                    break;

                case DMDO_270:
                    iRotate = (m_CursorSize.x -1 - (x - m_CursorRect.left))*m_CursorSize.x + y - m_CursorRect.top;
                    break;

                default:
                    iRotate = (y - m_CursorRect.top)*m_CursorSize.x + x - m_CursorRect.left;
                    break;
                }

                cbsLine[(x - m_CursorRect.left) * (m_pMode->Bpp >> 3)] = ptrLine[x * (m_pMode->Bpp >> 3)];
                ptrLine[x * (m_pMode->Bpp >> 3)] &= m_CursorAndShape[iRotate];
                ptrLine[x * (m_pMode->Bpp >> 3)] ^= m_CursorXorShape[iRotate];

                if (m_pMode->Bpp > 8)
                {
                    cbsLine[(x - m_CursorRect.left) * (m_pMode->Bpp >> 3) + 1] = ptrLine[x * (m_pMode->Bpp >> 3) + 1];
                    ptrLine[x * (m_pMode->Bpp >> 3) + 1] &= m_CursorAndShape[iRotate];
                    ptrLine[x * (m_pMode->Bpp >> 3) + 1] ^= m_CursorXorShape[iRotate];

                    if (m_pMode->Bpp > 16)
                    {
                        cbsLine[(x - m_CursorRect.left) * (m_pMode->Bpp >> 3) + 2] = ptrLine[x * (m_pMode->Bpp >> 3) + 2];
                        ptrLine[x * (m_pMode->Bpp >> 3) + 2] &= m_CursorAndShape[iRotate];
                        ptrLine[x * (m_pMode->Bpp >> 3) + 2] ^= m_CursorXorShape[iRotate];
                    }
                }
            }
        }

        m_CursorRect = cursorRectSave;
        m_CursorVisible = TRUE;
    }
}

void
S3C6410Disp::CursorOff (void)
{
    UCHAR *ptrScreen = (UCHAR*)m_pPrimarySurface->Buffer();
    UCHAR *ptrLine;
    UCHAR *cbsLine;
    int x, y;

    if (!m_CursorForcedOff && !m_CursorDisabled && m_CursorVisible)
    {
        RECTL rSave = m_CursorRect;
        RotateRectl(&m_CursorRect);
        for (y = m_CursorRect.top; y < m_CursorRect.bottom; y++)
        {
            // clip to displayable screen area (top/bottom)
            if (y < 0)
            {
                continue;
            }

            if (y >= m_nScreenHeightSave)
            {
                break;
            }

            ptrLine = &ptrScreen[y * m_pPrimarySurface->Stride()];
            cbsLine = &m_CursorBackingStore[(y - m_CursorRect.top) * (m_CursorSize.x * (m_pMode->Bpp >> 3))];

            for (x = m_CursorRect.left; x < m_CursorRect.right; x++)
            {
                // clip to displayable screen area (left/right)
                if (x < 0)
                {
                    continue;
                }

                if (x >= m_nScreenWidthSave)
                {
                    break;
                }

                ptrLine[x * (m_pMode->Bpp >> 3)] = cbsLine[(x - m_CursorRect.left) * (m_pMode->Bpp >> 3)];

                if (m_pMode->Bpp > 8)
                {
                    ptrLine[x * (m_pMode->Bpp >> 3) + 1] = cbsLine[(x - m_CursorRect.left) * (m_pMode->Bpp >> 3) + 1];

                    if (m_pMode->Bpp > 16)
                    {
                        ptrLine[x * (m_pMode->Bpp >> 3) + 2] = cbsLine[(x - m_CursorRect.left) * (m_pMode->Bpp >> 3) + 2];
                    }
                }
            }
        }

        m_CursorRect = rSave;
        m_CursorVisible = FALSE;
    }
}

SCODE
S3C6410Disp::SetPointerShape(GPESurf *pMask, GPESurf *pColorSurf, INT xHot, INT yHot, INT cX, INT cY)
{
    UCHAR    *andPtr;        // input pointer
    UCHAR    *xorPtr;        // input pointer
    UCHAR    *andLine;        // output pointer
    UCHAR    *xorLine;        // output pointer
    char    bAnd;
    char    bXor;
    int        row;
    int        col;
    int        i;
    int        bitMask;

    DEBUGMSG(GPE_ZONE_CURSOR,(TEXT("%s(0x%X, 0x%X, %d, %d, %d, %d)\r\n"), _T(__FUNCTION__), pMask, pColorSurf, xHot, yHot, cX, cY));

    // turn current cursor off
    CursorOff();

    // release memory associated with old cursor
    if (!pMask)                            // do we have a new cursor shape
    {
        m_CursorDisabled = TRUE;        // no, so tag as disabled
    }
    else
    {
        m_CursorDisabled = FALSE;        // yes, so tag as not disabled

        // store size and hotspot for new cursor
        m_CursorSize.x = cX;
        m_CursorSize.y = cY;
        m_CursorHotspot.x = xHot;
        m_CursorHotspot.y = yHot;

        andPtr = (UCHAR*)pMask->Buffer();
        xorPtr = (UCHAR*)pMask->Buffer() + (cY * pMask->Stride());

        // store OR and AND mask for new cursor
        for (row = 0; row < cY; row++)
        {
            andLine = &m_CursorAndShape[cX * row];
            xorLine = &m_CursorXorShape[cX * row];

            for (col = 0; col < cX / 8; col++)
            {
                bAnd = andPtr[row * pMask->Stride() + col];
                bXor = xorPtr[row * pMask->Stride() + col];

                for (bitMask = 0x0080, i = 0; i < 8; bitMask >>= 1, i++)
                {
                    andLine[(col * 8) + i] = bAnd & bitMask ? 0xFF : 0x00;
                    xorLine[(col * 8) + i] = bXor & bitMask ? 0xFF : 0x00;
                }
            }
        }
    }

    return    S_OK;
}

SCODE
S3C6410Disp::MovePointer(INT xPosition, INT yPosition)
{
    DEBUGMSG(GPE_ZONE_CURSOR, (TEXT("%s(%d, %d)\r\n"), _T(__FUNCTION__), xPosition, yPosition));

    CursorOff();

    if (xPosition != -1 || yPosition != -1)
    {
        // compute new cursor rect
        m_CursorRect.left = xPosition - m_CursorHotspot.x;
        m_CursorRect.right = m_CursorRect.left + m_CursorSize.x;
        m_CursorRect.top = yPosition - m_CursorHotspot.y;
        m_CursorRect.bottom = m_CursorRect.top + m_CursorSize.y;

        CursorOn();
    }

    return    S_OK;
}