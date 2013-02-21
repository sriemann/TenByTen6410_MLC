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
#include "OpenGLSurface.hpp"
#include "OpenGLDevice.hpp"
#include "debug.hpp"
#include "macros.hpp"
#include <ehm.h>
#include <intsafe.h>
#include "config.hpp"


//
// Initialize static variables
UINT32 COpenGLSurface::s_uTextureMemoryUsage = 0;
UINT32 COpenGLSurface::s_uSurfaceCount       = 0;

COpenGLSurface::COpenGLSurface() :
    m_RefCount(1),
    m_pOpenGLDevice(NULL),
    m_pPixelBuffer(NULL),
    m_uTextureID(0),
    m_fKeepSystemMemory(0),
    m_bIsOpaque(FALSE),
    m_bIsTransparent(FALSE),
    m_uWidth(0),
    m_uHeight(0),
    m_uStride(0),
    m_uByteCount(0),
    m_pixelFormat(XRPixelFormat_pixelColor32bpp_A8R8G8B8),
    m_uTexelSize(4)
{
    RETAILMSG(ZONE_FUNCTION|ZONE_SURFACE, (TEXT("COpenGLSurface::COpenGLSurface()")));

    s_uSurfaceCount++;

    memset(&m_rcDirty,0,sizeof(m_rcDirty));
}
    


COpenGLSurface::~COpenGLSurface()
{
    RETAILMSG(ZONE_MEMORY, (TEXT("~COpenGLSurface[%3d]()"), m_uTextureID));

    s_uSurfaceCount--;

    Deinitialize();
}


//-------------------------------------------------------------------------
//
//  Function:   COpenGLSurface::Create
//
//  Synopsis:
//     Create a COpenGLSurface object
//
//-------------------------------------------------------------------------
HRESULT 
COpenGLSurface::Create(
    __in COpenGLDevice *pOGLDevice, 
    __in INT32          fRenderTarget,
    __in UINT32         uWidth,
    __in UINT32         uHeight,
    __in INT32          fKeepSystemMemory,
    __in XRPixelFormat  pixelFormat,
    __out COpenGLSurface **ppOGLSurface
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLSurface::Create()")));

    HRESULT hr = S_OK;
    COpenGLSurface *pOGLSurface = NULL;
    
    if (!uWidth || !uHeight)
    {
        RETAILMSG(ZONE_ERROR, (TEXT("ERROR: COpenGLSurface::Create(): cannot create %d x %d surface."),
            uWidth, uHeight));
        CHR(E_INVALIDARG);
    }

    pOGLSurface = new COpenGLSurface();
    CPREx(pOGLSurface, E_OUTOFMEMORY);

    CHR(pOGLSurface->Initialize(pOGLDevice, fRenderTarget, uWidth, uHeight, pixelFormat, fKeepSystemMemory));

    // Pass ownership to the caller; caller must free
    *ppOGLSurface = pOGLSurface;
    pOGLSurface = NULL;

Error:
    SAFE_RELEASE(pOGLSurface);

    return(hr);
}



//-------------------------------------------------------------------------
//
//  Function:   COpenGLSurface::Initialize
//
//  Synopsis:
//     Create the underlying GL texture.
//     The pixel storage isn't allocated until COpenGLSurface::Lock().
//
//-------------------------------------------------------------------------
HRESULT 
COpenGLSurface::Initialize(    
    __in COpenGLDevice *pOGLDevice, 
    __in INT32          fRenderTarget,
    __in UINT32         uWidth,
    __in UINT32         uHeight,
    __in XRPixelFormat  pixelFormat,
    __in INT32          fKeepSystemMemory
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLSurface::Initialize(fRenderTarget = %d)"), fRenderTarget));

    UNREFERENCED_PARAMETER(fRenderTarget);

    HRESULT hr = S_OK;


    CPREx( pOGLDevice, E_POINTER );

    m_pixelFormat = pixelFormat;
    switch (pixelFormat)
    {
        case XRPixelFormat_pixelColor16bpp_R5G6B5:
            m_uTexelSize = 2;
            break;
        case XRPixelFormat_pixelColor32bpp_A8R8G8B8:
            m_uTexelSize = 4;
            break;
        default:
            RETAILMSG(ZONE_ERROR, (TEXT("ERROR: COpenGLSurface::Initialize(): XRPixelFormat format 0x%x is not supported."),
                pixelFormat));
            DEBUGCHK(0);
            CHR(E_INVALIDARG);
    }

    m_uWidth  = uWidth;
    m_uHeight = uHeight;
    m_uStride = uWidth * m_uTexelSize;

    // Bug: a certain platform doesn't support DWORD-aligned 16-bit textures.
    // So report stride as (width * texel size).  
    // For well-behaved drivers, report stride as (width * texel size), rounded up to DWORD boundary.
    if ( !pOGLDevice->UseWORDAligned565() )
    {
        // Ensure stride is DWORD-aligned, since we put the driver in that mode with 
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 4)
        if (m_uStride & 0x3)
        {
            m_uStride += 2;
        }
    }

    CHR(UIntMult(uHeight, m_uStride, &m_uByteCount));
    
    // Do we keep a CPU copy of the pixel data, or delete it after
    // uploading to GPU?
    m_fKeepSystemMemory = fKeepSystemMemory;
        
    m_pOpenGLDevice = pOGLDevice;
    m_pOpenGLDevice->AddRef();
    
    m_pOpenGLDevice->Lock();

    VERIFYGL(glGenTextures(1, &m_uTextureID));
    VERIFYGL(glActiveTexture(GL_TEXTURE0));
    VERIFYGL(glBindTexture(GL_TEXTURE_2D, m_uTextureID));
    VERIFYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    VERIFYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    VERIFYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
#ifdef USE_MIPMAPS
    VERIFYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
#else
    VERIFYGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
#endif


    // NOTE: we don't pass m_pPixelBuffer to glTexImage2D() because the memory
    // is not yet allocated (and would not hold meaningful pixel data in any case)
    //
    // The texture will be written to by XamlRuntime after ::Lock(), so we allocate there
    // The texture upload to GPU will occur in ::AddDirty()
    if ( XRPixelFormat_pixelColor16bpp_R5G6B5 == m_pixelFormat )
    {
        RETAILMSG(ZONE_SURFACE, (TEXT("COpenGLSurface[%3d]: 16bit RGB 565"), m_uTextureID));
        VERIFYGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, uWidth, uHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL));
    }
    else if ( XRPixelFormat_pixelColor32bpp_A8R8G8B8 == m_pixelFormat )
    {                
        if ( m_pOpenGLDevice->IsGlBGRAExtSupported() )
        {
            RETAILMSG(ZONE_SURFACE, (TEXT("COpenGLSurface[%3d]: 32bit BGRA"), m_uTextureID));
            VERIFYGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, uWidth, uHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL));
        }
        else
        {
            RETAILMSG(ZONE_SURFACE, (TEXT("COpenGLSurface[%3d]: 32bit RGBA"), m_uTextureID));
            VERIFYGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, uWidth, uHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
        }
    }

#ifdef USE_MIPMAPS
    RETAILMSG(ZONE_SURFACE, (TEXT("COpenGLSurface[%d]: created mipmap"), m_uTextureID));
    VERIFYGL(glGenerateMipmap(GL_TEXTURE_2D));
#endif

    RETAILMSG(ZONE_SURFACE, (TEXT("COpenGLSurface[%3d]: created %d x %d x %d"),
        m_uTextureID, m_uWidth, m_uHeight, m_uTexelSize));

Error:
    if (m_pOpenGLDevice)
        m_pOpenGLDevice->Unlock();

    return(hr);
}



//-------------------------------------------------------------------------
//
//  Function:   COpenGLSurface::Deinitialize
//
//  Synopsis:
//     Free the underlying GL texture and system memory buffer
//
//-------------------------------------------------------------------------
void
COpenGLSurface::Deinitialize()
{
    RETAILMSG(ZONE_VERBOSE, (TEXT("COpenGLSurface[%3d]::Deinitialize()"), m_uTextureID));

    m_pOpenGLDevice->Lock();

    if (m_uTextureID)
    {
        glDeleteTextures(1, &m_uTextureID);
        m_uTextureID = 0;
    }

    m_pOpenGLDevice->Unlock();

    if (m_pPixelBuffer)
    {
        DecrementTextureMemoryUse(&s_uTextureMemoryUsage, m_uByteCount);
        delete[] m_pPixelBuffer;
    }

    SAFE_RELEASE(m_pOpenGLDevice);

    if (m_RefCount)
        RETAILMSG(ZONE_WARNING, (TEXT("COpenGLSurface[%3d]::Deinitialize(): %d dangling ref(s)!"), m_uTextureID, m_RefCount));

}


UINT 
COpenGLSurface::AddRef()
{
    RETAILMSG(ZONE_VERBOSE, (TEXT("COpenGLSurface[%3d]::AddRef()"), m_uTextureID));

    return InterlockedIncrement((LONG *)&m_RefCount);
}



UINT
COpenGLSurface::Release()
{
    RETAILMSG(ZONE_VERBOSE, (TEXT("COpenGLSurface[%3d]::Release()"), m_uTextureID));

    UINT cRef = InterlockedDecrement((LONG *)&m_RefCount);
    if (cRef == 0)
    {
        RETAILMSG(ZONE_SURFACE|ZONE_VERBOSE, (TEXT("COpenGLSurface: no more refs; cleaning up\n")));
        delete this;
    }

    return cRef;
}



HRESULT
COpenGLSurface::Lock(
        __out void **ppAddress,
        __out int *pnStride,
        __out UINT *puWidth,
        __out UINT *puHeight
        )
{
    HRESULT hr = S_OK;
    *pnStride = 0;
    *ppAddress = NULL;
    *puWidth = 0;
    *puHeight = 0;

    if ( !m_pPixelBuffer )
    {
        ASSERT(m_uStride);
        ASSERT(m_uHeight);
        ASSERT(m_uByteCount);

        m_pPixelBuffer = new BYTE[m_uByteCount];
        CPREx(m_pPixelBuffer, E_OUTOFMEMORY);

        IncrementTextureMemoryUse(&s_uTextureMemoryUsage, m_uByteCount);

        RETAILMSG(ZONE_MEMORY, (TEXT("COpenGLSurface[%3d] %3d x %3d being used for first time; allocated %7d bytes @ 0x%08x - 0x%08x"), 
            m_uTextureID, m_uWidth, m_uHeight, m_uByteCount, m_pPixelBuffer, (BYTE*)m_pPixelBuffer + m_uByteCount));
    }

    RETAILMSG(ZONE_FUNCTION|ZONE_VERBOSE, (TEXT("COpenGLSurface[%3d]::Lock( 0x%08x, STR: %d, %d x %d )"), 
        m_uTextureID, m_pPixelBuffer, m_uStride, m_uWidth, m_uHeight));

    *pnStride   = m_uStride;
    *ppAddress  = m_pPixelBuffer;
    *puWidth    = m_uWidth;
    *puHeight   = m_uHeight;

Error:

    return(hr);
}
    


HRESULT 
COpenGLSurface::Unlock()
{
    RETAILMSG(ZONE_FUNCTION|ZONE_VERBOSE, (TEXT("COpenGLSurface[%3d]::Unlock()"), m_uTextureID));

    return S_OK;
}



HRESULT 
COpenGLSurface::Present(
    __in HWND hTarget, 
    __in_opt XRPOINT *pOffset, 
    __in UINT bAlphaBlt)
{
    RETAILMSG(ZONE_FUNCTION|ZONE_SURFACE, (TEXT("COpenGLSurface[%3d]::Present()"), m_uTextureID));

    UNREFERENCED_PARAMETER(hTarget);
    UNREFERENCED_PARAMETER(pOffset);
    UNREFERENCED_PARAMETER(bAlphaBlt);

    HRESULT hr = S_OK;

    CHR(m_pOpenGLDevice->Present());

Error:
    return(hr);
}



HRESULT 
COpenGLSurface::Present(
    __in HWND hTarget, 
    __in XRRECT_WH bounds, 
    __in XRRECT_WH clip, 
    __in UINT bAlphaBlt)
{
    RETAILMSG(ZONE_FUNCTION|ZONE_SURFACE, (TEXT("COpenGLSurface[%3d]::Present( NOTIMPL )"), m_uTextureID));

    UNREFERENCED_PARAMETER(hTarget);
    UNREFERENCED_PARAMETER(bounds);
    UNREFERENCED_PARAMETER(clip);
    UNREFERENCED_PARAMETER(bAlphaBlt);

    ASSERT(0);
    return E_NOTIMPL;
}



HRESULT 
COpenGLSurface::AddDirty(__in const XRRECT_WH &rcDirty)
{
    if (!rcDirty.Height && !rcDirty.Width)
        return S_OK;

    RETAILMSG(ZONE_FUNCTION|ZONE_SURFACE, (TEXT("COpenGLSurface[%3d]::AddDirty( %d, %d, %d, %d )"),
        m_uTextureID, rcDirty.X, rcDirty.Y, rcDirty.Width, rcDirty.Height));

    HRESULT hr = S_OK;

    m_pOpenGLDevice->Lock();

    if (m_pPixelBuffer)
    {

        // Clear any prior errors
        glGetError();

        VERIFYGL(glActiveTexture(GL_TEXTURE0));
        VERIFYGL(glBindTexture(GL_TEXTURE_2D, m_uTextureID));
        if (rcDirty.Width && rcDirty.Height)
        {
            if ( XRPixelFormat_pixelColor16bpp_R5G6B5 == m_pixelFormat )
            {
                VERIFYGL(glTexSubImage2D(
                    GL_TEXTURE_2D, 
                    0, 
                    0, 
                    rcDirty.Y, 
                    m_uWidth, 
                    rcDirty.Height, 
                    GL_RGB,
                    GL_UNSIGNED_SHORT_5_6_5,
                    (BYTE *)(m_pPixelBuffer) + rcDirty.Y * m_uStride
                    ));
            }
            else if ( XRPixelFormat_pixelColor32bpp_A8R8G8B8 == m_pixelFormat )
            {                
                if ( m_pOpenGLDevice->IsGlBGRAExtSupported() )
                {
                    VERIFYGL(glTexSubImage2D(
                        GL_TEXTURE_2D, 
                        0, 
                        0, 
                        rcDirty.Y, 
                        m_uWidth, 
                        rcDirty.Height, 
                        GL_BGRA_EXT, 
                        GL_UNSIGNED_BYTE,
                        (BYTE *)m_pPixelBuffer + rcDirty.Y * m_uStride
                        ));
                }
                else
                {
                    VERIFYGL(glTexSubImage2D(
                        GL_TEXTURE_2D, 
                        0, 
                        0, 
                        rcDirty.Y, 
                        m_uWidth, 
                        rcDirty.Height, 
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        (BYTE *)m_pPixelBuffer + rcDirty.Y * m_uStride
                        ));
                }
            }
        }

        // We are free to delete the CPU copy of the texture
        // after uploading it to the GPU.
        //
        // HgCore may set fKeepSystemMemory if the texture is likely
        // to be modified and reuploaded, in which case we don't want to 
        // delete our copy just yet.
        if (!m_fKeepSystemMemory)
        {
            SAFE_ARRAYDELETE(m_pPixelBuffer);
            DecrementTextureMemoryUse(&s_uTextureMemoryUsage, m_uByteCount);   
        }
    }

Error:
    m_pOpenGLDevice->Unlock();

    return(hr);
}




