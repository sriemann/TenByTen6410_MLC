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
#include "OpenGLRenderer.hpp"
#include "OpenGLSurface.hpp"
#include "debug.hpp"
#include "macros.hpp"
#include <ehm.h>


COpenGLRenderer::COpenGLRenderer( ) :
    m_RefCount(1),
    m_pBackSurface(NULL),
    m_pOpenGLDevice(NULL),
    m_bHasDeviceLock(FALSE),
    m_fNeedsFullRedraw(FALSE)
{
}



COpenGLRenderer::~COpenGLRenderer()
{
    FreeResources();
}


HRESULT
COpenGLRenderer::SetGraphicsDevice( COpenGLDevice* pDevice )
{
    HRESULT hr = S_OK;
    CPREx(pDevice, E_POINTER);

    m_pOpenGLDevice = pDevice;
    m_pOpenGLDevice->AddRef();

Error:
    return hr;
}


UINT COpenGLRenderer::AddRef()
{
    return InterlockedIncrement((LONG *)&m_RefCount);
}



UINT COpenGLRenderer::Release()
{
    UINT cRef = InterlockedDecrement((LONG *)&m_RefCount);
    
    if (cRef == 0)
    {
        RETAILMSG(ZONE_INFO, (TEXT("COpenGLRenderer: no more refs; cleaning up\n")));
        delete this;
    }

    return cRef;
}




HRESULT
COpenGLRenderer::Create(
    __deref_out IRenderer** ppRenderer
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer::Create()")));

    HRESULT hr = S_OK;
    COpenGLRenderer* pOpenGLRenderer = NULL;


    pOpenGLRenderer = new COpenGLRenderer();
    CPR(pOpenGLRenderer);

    *ppRenderer = pOpenGLRenderer;
    pOpenGLRenderer = NULL;

Error:
    SAFE_DELETE(pOpenGLRenderer);
    return hr;
}



void COpenGLRenderer::FreeResources()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer::FreeResources()")));

    SAFE_RELEASE(m_pBackSurface);
    SAFE_RELEASE(m_pOpenGLDevice);

    if (m_RefCount)
        RETAILMSG(ZONE_WARNING, (TEXT("COpenGLRenderer::FreeResources: %d dangling ref(s)!"), m_RefCount));
}



void COpenGLRenderer::Reset()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer::Reset()")));

    return;
}



HRESULT
COpenGLRenderer::PreRender(
    HWND hwndRender, 
    HDC hdcRender,
    __in const SIZE* pSurfaceSize, 
    __out BOOL* pNeedsFullRedraw,
    __out ICustomSurface** ppSurface
    )
{
    UNREFERENCED_PARAMETER(hwndRender);
    UNREFERENCED_PARAMETER(hdcRender);
    UNREFERENCED_PARAMETER(ppSurface);

    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer::PreRender() Thread [0x%x]"), GetCurrentThreadId() ));


    HRESULT hr = S_OK;
    bool    fFullRedraw = FALSE;


    // Gain exclusive access to the OpenGL driver
    // until end-of-frame [ ::PostRender() ]
    // This ensures windows render in lock-step (OpenGL ES is not thread-safe).
    // It's also generally the most efficient use of the GPU.
    CHR( m_pOpenGLDevice->Lock() );
    m_bHasDeviceLock = TRUE;

    // Allocate a texture and return it to HgCore.
    // Every scene will have at least one textured quad for the background.
    // This quad is later presented to the screen.
    //
    // We don't actually allocate the pixel buffer here; it's deferred until HgCore
    // calls COpenGLSurface::Lock() to update the bits.
    //
    if (NULL == m_pBackSurface)
    {
        CPREx(pSurfaceSize, E_INVALIDARG);

        hr =  m_pOpenGLDevice->CreateTexture(TRUE, pSurfaceSize->cx, pSurfaceSize->cy, TRUE, (ICustomSurface**)&m_pBackSurface);
        if (FAILED(hr))
        {
            RETAILMSG(ZONE_ERROR, (TEXT("COpenGLRenderer: Failure to create surface, hr=0x%08X\r\n"), hr));
            CHR(hr);
        }

        fFullRedraw = TRUE;
    }

    *pNeedsFullRedraw = fFullRedraw;
    *ppSurface = m_pBackSurface;
    (*ppSurface)->AddRef();

Error:
    return hr;
}



HRESULT
COpenGLRenderer::PostRender(
    __in HWND hwndRender, 
    __in HDC hdcRender, 
    __in const SIZE * pSurfaceSize, 
    __in XRRECT_WH * prcUpdate
    )
{
    HRESULT hr = S_OK;

    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer::PostRender()")));

    UNREFERENCED_PARAMETER(hwndRender);
    UNREFERENCED_PARAMETER(hdcRender);
    UNREFERENCED_PARAMETER(pSurfaceSize);
    UNREFERENCED_PARAMETER(prcUpdate);

    CHR( m_pOpenGLDevice->Present() );

Error:
    if (m_bHasDeviceLock)
    {
        m_pOpenGLDevice->Unlock();
        m_bHasDeviceLock = FALSE;
    }

    return hr;
}



// ----------------------------------------------------------
//
// RenderPluginInitialize/Cleanup
//
// Initialize/Cleanup OpenGL and cache supported capabilities
//
//--------------------------------------------------
HRESULT RenderPluginInitialize()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer: - RenderPluginInitialize()")));

    return COpenGLDevice::InitializeGlobals();
}



void RenderPluginCleanup()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer - RenderPluginCleanup()")));

    COpenGLDevice::CleanupGlobals();
}



// ----------------------------------------------------------
//
// CreateRenderer : Required API to implement to link into XamlRuntime.dll
//
// Creates a COpenGLRenderer object for XamlRuntime to use when rendering content
// to the display. 
//
//  The IRenderer parameter is MANDATORY to provide and will always be used.
//
//  The ICustomGraphicsDevice is OPTIONAL and should only be filled in if the device
//  supports cached composition. If the device does not support cached composition
//  all CacheMode flags and settings will be ignored in XAML elements
//
//--------------------------------------------------
HRESULT CreateRenderer(
    __in HWND HostWindow ,
    __out IRenderer **ppRenderer, 
    __out_opt ICustomGraphicsDevice** ppDevice
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLRenderer - CreateRenderer()")));

    HRESULT hr = S_OK;
    COpenGLDevice*           pGraphicsDevice = NULL;
    COpenGLRenderer*         pRenderer       = NULL;
    
    CHR(COpenGLRenderer::Create(  (IRenderer**) (&pRenderer)) );

    CHR(COpenGLDevice::Create(HostWindow, (ICustomGraphicsDevice**) &pGraphicsDevice));
    CHR(pRenderer->SetGraphicsDevice( pGraphicsDevice ));

    // Pass ownership to the caller
    // Caller must release these objects when done
    *ppDevice = pGraphicsDevice;
    pGraphicsDevice = NULL;

    *ppRenderer = pRenderer;
    pRenderer = NULL;

Error:
    // cleanup
    //
    SAFE_RELEASE(pGraphicsDevice);
    SAFE_RELEASE(pRenderer);

    return hr;
}

