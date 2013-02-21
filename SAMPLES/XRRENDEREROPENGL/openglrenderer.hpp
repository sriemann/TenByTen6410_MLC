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

#pragma once

#include <XamlRuntimeGraphics.h>
#include "OpenGLDevice.hpp"
#include "OpenGLSurface.hpp"

//------------------------------------------------------------------------
//
//  Class:  COpenGLRenderer
//
//  Synopsis:
//      Render XamlRuntime content using OpenGL to a window.
//
//------------------------------------------------------------------------

class COpenGLRenderer :
    public IRenderer
{
protected:
    COpenGLSurface *m_pBackSurface;
    BOOL            m_fNeedsFullRedraw;

protected:
    COpenGLRenderer();
    virtual ~COpenGLRenderer();

public:
    static
    HRESULT
    Create(
        __deref_out IRenderer** ppRenderer
        );

public:
    virtual UINT AddRef();
    virtual UINT Release();
    
    virtual void FreeResources();
    virtual void Reset();

    virtual HRESULT SetGraphicsDevice( __in COpenGLDevice* pDevice );

    virtual HRESULT PreRender(
        HWND hwndRender, 
        HDC hdcRender, 
        __in const SIZE * pSurfaceSize, 
        __out BOOL * pfNeedsFullRedraw, 
        __out ICustomSurface * * ppSurface
        );

    virtual HRESULT PostRender(
        HWND hwndRender, 
        HDC hdcRender,
        __in const SIZE* pSurfaceSize,
        __in XRRECT_WH* prcUpdate
        );
    
private:
    COpenGLDevice          *m_pOpenGLDevice;
    UINT                    m_RefCount;
    BOOL                    m_bHasDeviceLock;
};
    
