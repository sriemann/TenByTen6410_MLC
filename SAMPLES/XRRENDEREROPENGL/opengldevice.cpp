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
#include "OpenGLDevice.hpp"
#include "OpenGLSurface.hpp"
#include "FrameBufferObject.hpp"
#include "debug.hpp"
#include "macros.hpp"
#include <ehm.h>
#include "shaders.h"


CRITICAL_SECTION COpenGLDevice::s_OpenGLCS;
BOOL             COpenGLDevice::s_csCreated     = FALSE;
signed long      COpenGLDevice::s_LockCount     = 0;


UINT       COpenGLDevice::s_NumInstances        = 0;
EGLDisplay COpenGLDevice::s_eglDisplay          = EGL_NO_DISPLAY;
EGLContext COpenGLDevice::s_eglContext          = EGL_NO_CONTEXT;
EGLConfig  COpenGLDevice::s_eglActiveConfig     = 0;
EGLConfig  COpenGLDevice::s_eglConfig32bpp      = 0;
EGLConfig  COpenGLDevice::s_eglConfig16bpp      = 0;

GLuint     COpenGLDevice::s_uiProgramObject     = 0;
GLuint     COpenGLDevice::s_uiVertShader        = 0;
GLuint     COpenGLDevice::s_uiFragShader        = 0;
GLint      COpenGLDevice::s_matrixProjectionLoc = 0;
GLint      COpenGLDevice::s_checkerBoardLoc     = 0;
GLint      COpenGLDevice::s_colorConversionLoc  = 0;


#ifdef USE_DEPTH_BUFFER
static GLfloat  s_polygonOffset = 0.0f;
#endif


HINSTANCE g_hInstance    = NULL;


// ----------------------------------------------------
// InitializeGlobals: 
//
// ----------------------------------------------------
HRESULT
COpenGLDevice::InitializeGlobals()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice::InitializeGlobals()")));

    if (!s_csCreated)
    {
        InitializeCriticalSection( &s_OpenGLCS );
        s_csCreated = TRUE;
        RETAILMSG(ZONE_SERIALIZE, (TEXT("Created OpenGL CS")));
    }

    return S_OK;
}



// ----------------------------------------------------
// CleanupGlobals: 
//
// Cleanup any global data we allocated in this plugin
// ----------------------------------------------------
void 
COpenGLDevice::CleanupGlobals()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice::CleanupGlobals()")));

    if ( 0 == s_NumInstances && s_csCreated )
    {
        ASSERT(0 == s_LockCount);
        DeleteCriticalSection( &s_OpenGLCS );
        s_csCreated = FALSE;
    }
}



// ----------------------------------------------------
// OpenGLDevice::Create
//
// ----------------------------------------------------
HRESULT
COpenGLDevice::Create(
    HWND hWindow,
    ICustomGraphicsDevice **ppOpenGLDevice
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice::Create()")));

    HRESULT hr = S_OK;
    COpenGLDevice *pOGLDevice = NULL;

    CPREx(ppOpenGLDevice, E_POINTER);
    *ppOpenGLDevice = NULL;

    pOGLDevice = new COpenGLDevice();
    CPREx(pOGLDevice, E_OUTOFMEMORY);

    CHR(pOGLDevice->Initialize(hWindow));

    // Don't AddRef(): the caller will do it
    *ppOpenGLDevice = pOGLDevice;
    pOGLDevice = NULL;

Error:
    SAFE_RELEASE(pOGLDevice);

    return(hr);
}



// ----------------------------------------------------
// COpenGLDevice::Constructor
//
// ----------------------------------------------------
COpenGLDevice::COpenGLDevice() :
    m_RefCount(1),
    m_hwnd(NULL),
    m_uRenderWidth(0),
    m_uRenderHeight(0),
    m_eglSurface(EGL_NO_SURFACE),
    m_fGlBGRAExt(false),
    m_pMostRecentTexture(NULL),

    // Configuration options, may be overridden by [HKLM\Software\Microsoft\XamlRenderOpenGL]
    m_bUse16BitTextures(false),
    m_bUseCheckerboard(false),
    m_bUseWORDAligned565(false),
    m_bUse16BitWindow(false)
{
    InterlockedIncrement( (LONG*)&s_NumInstances );
    m_Instance = s_NumInstances;

    memset(&m_rcWindow, 0, sizeof(m_rcWindow));

    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]()"), m_Instance));
}



// ----------------------------------------------------
// COpenGLDevice::Destructor
//
// ----------------------------------------------------
COpenGLDevice::~COpenGLDevice()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("~COpenGLDevice()")));

    Deinitialize();
}



// ----------------------------------------------------
// COpenGLDevice::Initialize
//
// Create the primary surface which will display the end result
// of a render operation
// ----------------------------------------------------
HRESULT
COpenGLDevice::Initialize(HWND hWindow)
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::Initialize()"), m_Instance));

    HRESULT hr = S_OK;

    GetRegistryOptions();

    EGLint attribs_32bpp[] =
    {
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         8,

        EGL_LEVEL,              0,
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
        EGL_NATIVE_RENDERABLE,  EGL_FALSE,
#ifdef USE_ANTIALIASING
        EGL_SAMPLE_BUFFERS,     1,
#endif
#ifdef USE_DEPTH_BUFFER
        EGL_DEPTH_SIZE,         16,
#else
        EGL_DEPTH_SIZE,         EGL_DONT_CARE,
#endif
        EGL_NONE
    };

    EGLint attribs_16bpp[] =
    {
        EGL_RED_SIZE,           5,
        EGL_GREEN_SIZE,         6,
        EGL_BLUE_SIZE,          5,
        EGL_ALPHA_SIZE,         EGL_DONT_CARE,

        EGL_LEVEL,              0,
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
        EGL_NATIVE_RENDERABLE,  EGL_FALSE,
#ifdef USE_ANTIALIASING
        EGL_SAMPLE_BUFFERS,     1,
#endif
#ifdef USE_DEPTH_BUFFER
        EGL_DEPTH_SIZE,         16,
#else
        EGL_DEPTH_SIZE,         EGL_DONT_CARE,
#endif
        EGL_NONE
    };


    m_hwnd = (HWND)hWindow; 
    GetWindowRect(m_hwnd, &m_rcWindow);
    m_uRenderWidth  = m_rcWindow.right  - m_rcWindow.left;
    m_uRenderHeight = m_rcWindow.bottom - m_rcWindow.top;
    m_uThreadID = ::GetCurrentThreadId();

    Lock();

    // Get a handle to the display.
    // Only need do this once per process.
    // It will be shared between all threads and window surfaces.
    if (EGL_NO_DISPLAY == s_eglDisplay)
    {
        s_eglDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
        if (EGL_NO_DISPLAY == s_eglDisplay)
        {
            // Some drivers are buggy and require the Display Context
            NativeDisplayType display = GetDC(hWindow);
            s_eglDisplay = eglGetDisplay(display);
            ReleaseDC(hWindow, display);
            if ( EGL_NO_DISPLAY == s_eglDisplay )
            {
                RETAILMSG(ZONE_ERROR, (TEXT("ERROR: eglGetDisplay() failed.")));
                CHR(E_FAIL);
            }
        }
        RETAILMSG(ZONE_INFO, (TEXT("Thread [0x%x] created eglDisplay 0x%x"), GetCurrentThreadId(), s_eglDisplay));

        EGLint nEGLMajor = 0;
        EGLint nEGLMinor = 0;
        VERIFYEGL(eglInitialize(s_eglDisplay, &nEGLMajor, &nEGLMinor));

        if ( dpCurSettings.ulZoneMask & (ZONE_ID_CONFIGS) )
        {
            //
            // Dump all OpenGL ES configurations reported by the driver
            //
            EGLint totalConfigs = 0;
            EGLConfig configs[32];
            eglGetConfigs(s_eglDisplay, configs, 32, &totalConfigs);
            RETAILMSG(ZONE_INFO, (TEXT("Configurations supported by this device:")));
            for (EGLint configId = 0; configId < totalConfigs; configId++)
            {
                PrintDisplayConfig(configs[configId]);
            }
        }
    }

    // Enable OpenGL ES 2.0 for the current thread
    VERIFYEGL(eglBindAPI(EGL_OPENGL_ES_API));

    //
    // Select our framebuffer configuration.
    // Only need do this once per process.
    // It will be shared between all threads and window surfaces.
    //
    if ( !s_eglConfig32bpp && !s_eglConfig16bpp )
    {
        EGLint matchingConfigs = 0;


        if ( Use16BitWindow() )  
        {
            s_eglConfig32bpp = 0;
        }
        else
        {
            // Query 32-bit configs
            VERIFYEGL(eglChooseConfig(s_eglDisplay, attribs_32bpp, &s_eglConfig32bpp, 1, &matchingConfigs));
        }


        // Query 16-bit configs
        matchingConfigs = 0;
        VERIFYEGL(eglChooseConfig(s_eglDisplay, attribs_16bpp, &s_eglConfig16bpp, 1, &matchingConfigs));


        // Select best config
        if (s_eglConfig32bpp != 0)
        {
            s_eglActiveConfig = s_eglConfig32bpp;
            RETAILMSG(ZONE_INFO, (TEXT("Using 32bpp config:")));
            PrintDisplayConfig(s_eglActiveConfig);
        } 
        else if (s_eglConfig16bpp != 0)
        {
            s_eglActiveConfig = s_eglConfig16bpp;
            RETAILMSG(ZONE_INFO, (TEXT("Using 16bpp config:")));
            PrintDisplayConfig(s_eglActiveConfig);
        }
        else
        {
            RETAILMSG(ZONE_ERROR, (TEXT("ERROR: COpenGLDevice[%d]::Initialize(): no matching configuration found"), m_Instance));
            CHR(E_FAIL);
        }
    }

    // Create a rendering context.
    // Only need to do this once per process.
    // It will be shared between all threads and surfaces.
    if (EGL_NO_CONTEXT == s_eglContext)
    {
        static const EGLint nContextAttribs[] = 
        {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        VERIFYEGL(s_eglContext = eglCreateContext(s_eglDisplay, s_eglActiveConfig, EGL_NO_CONTEXT, nContextAttribs));
        if (s_eglContext == EGL_NO_CONTEXT)
        {
            RETAILMSG(ZONE_ERROR, (TEXT("ERROR: eglCreateContext(), err = 0x%x"), eglGetError()));
            CHR(E_FAIL);
        }
        else
        {
            RETAILMSG(ZONE_INFO, (TEXT("Thread [0x%x] created eglContext 0x%x"), GetCurrentThreadId(), s_eglContext));
        }
    }

    CHR(CreateDisplaySurface());
    CHR(InitializeState());
    CHR(Resize( m_rcWindow.right - m_rcWindow.left, m_rcWindow.bottom - m_rcWindow.top ));

    // Does the device support BGRA textures?
    // If so, we can skip BGRA -> RGBA conversion in the fragment shader
    m_fGlBGRAExt = IsSupportedGLExt("GL_EXT_bgra");

Error:
    Unlock();

    if (SUCCEEDED(hr))
    {
        RETAILMSG(ZONE_INFO, (TEXT("Thread [0x%x] created eglSurface 0x%x"), GetCurrentThreadId(), m_eglSurface));
    }
    else
    {
        RETAILMSG(ZONE_ERROR, (TEXT("ERROR: COpenGLDevice::Initialize(), hr = 0x%x"), hr));
    }

    return(hr);
}



void
COpenGLDevice::Deinitialize()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::Deinitialize()"), m_Instance));

    Lock();
    InterlockedDecrement( (LONG*)&s_NumInstances );

    if (s_eglDisplay != EGL_NO_DISPLAY)
        eglMakeCurrent(s_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (m_eglSurface != EGL_NO_SURFACE)
    {
        RETAILMSG(ZONE_INFO, (TEXT("Thread [0x%x] freeing eglSurface: 0x%x"), 
            GetCurrentThreadId(), m_eglSurface));
        eglDestroySurface(s_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    if ( 0 == s_NumInstances )
    {
        if (s_eglContext != EGL_NO_CONTEXT)
        {
            RETAILMSG(ZONE_INFO, (TEXT("Thread [0x%x] freeing eglContext: 0x%x"), 
                GetCurrentThreadId(), s_eglContext));

            eglDestroyContext(s_eglDisplay, s_eglContext);
            s_eglContext = EGL_NO_CONTEXT;
        }

        RETAILMSG(ZONE_INFO, (TEXT("Thread [0x%x] freeing eglDisplay: 0x%x"), 
            GetCurrentThreadId(), s_eglDisplay));

        s_eglConfig32bpp  = 0;
        s_eglConfig16bpp  = 0;
        s_eglActiveConfig = 0;

        // This should only be done ONCE per process as it invalidates 
        // all open handles, even those created on other windows or threads.
        eglTerminate(s_eglDisplay);
        s_eglDisplay = EGL_NO_DISPLAY;
    }

    // Release thread-specific data
    eglReleaseThread();

    Unlock();

    if (m_RefCount)
        RETAILMSG(ZONE_WARNING, (TEXT("COpenGLDevice::Deinitialize(): %d dangling ref(s)!"), m_RefCount));
}



UINT 
COpenGLDevice::AddRef()
{
    UINT cRef = InterlockedIncrement((LONG *)&m_RefCount);
    RETAILMSG(ZONE_FUNCTION|ZONE_VERBOSE, (TEXT("COpenGLDevice[%d]::AddRef( %d )\n"), m_Instance, cRef));

    return cRef;
}



UINT 
COpenGLDevice::Release()
{   
    UINT  cRef = InterlockedDecrement((LONG *)&m_RefCount);
    RETAILMSG(ZONE_FUNCTION|ZONE_VERBOSE, (TEXT("COpenGLDevice[%d]::Release( %d )\n"), m_Instance, cRef));
    
    if (cRef == 0)
    {
        RETAILMSG(ZONE_INFO, (TEXT("COpenGLDevice[%d]: no more refs; cleaning up\n"), m_Instance));
        delete this;
    }

    return cRef;
}



// ----------------------------------------------------
// COpenGLDevice::Resize
//
// Create a new renderering target to receive content
// ----------------------------------------------------
HRESULT 
COpenGLDevice::Resize(UINT uWidth, UINT uHeight)
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::Resize( %dx%d )"), m_Instance, uWidth, uHeight));

    HRESULT hr = S_OK;

    if (!uWidth || !uHeight)
        return hr;

    m_uRenderWidth  = uWidth;
    m_uRenderHeight = uHeight;

    //
    // Convert from the renderer's coordinate system to OpenGL's coordinate system...
    //
    // The underlying one is this (with integer pixel center at edges):
    //
    //             +1.0
    //              ^ +y
    //              |
    //              |
    //              |
    //       <------O------> +x  +1.0
    //              |
    //              |
    //              |
    //              v
    //
    // Ours is this with integers at upper-left of pixel (half-integer pixel
    //  center):
    //
    //     O------------> +x  +Width
    //     |
    //     |
    //     |
    //     |
    //     |
    //     |
    //     v +y
    //   +Height

    //
    // The following creates an orthographic projection; everything will
    // be flattened onto the 2D plane of the screen with no perspective.
    // This means that everything drawn should be parallel to the screen, 
    // or it will look warped.  For XamlRuntime, this is OK.
    // 
    // To support true 3D or 2.5D, the matrix needs to change to a perspective projection.
    //

    GLfloat rRecipWidth = static_cast<GLfloat>(1.0f / uWidth);
    GLfloat rRecipHeight = static_cast<GLfloat>(1.0f / uHeight);

    GLfloat sinRot = 0.0f;
    GLfloat cosRot = 1.0f;

    // Compute the projection matrix
    m_matProj[0]  = static_cast<GLfloat>( 2.0f * cosRot * rRecipWidth);
    m_matProj[4]  = static_cast<GLfloat>( 2.0f * sinRot * rRecipHeight);
    m_matProj[8]  = static_cast<GLfloat>( 0.0f);
    m_matProj[12] = static_cast<GLfloat>( 0.0f - cosRot - sinRot);

    m_matProj[1]  = static_cast<GLfloat>( 2.0f * sinRot * rRecipWidth);
    m_matProj[5]  = static_cast<GLfloat>(-2.0f * cosRot * rRecipHeight);
    m_matProj[9]  = static_cast<GLfloat>( 0.0f);
    m_matProj[13] = static_cast<GLfloat>( 0.0f + cosRot - sinRot);

    m_matProj[2]  = static_cast<GLfloat>( 0);
    m_matProj[6]  = static_cast<GLfloat>( 0);
    m_matProj[10] = static_cast<GLfloat>(-1.0f);
    m_matProj[14] = static_cast<GLfloat>( 0);

    m_matProj[3]  =  0;
    m_matProj[7]  =  0;
    m_matProj[11] =  0;
    m_matProj[15] =  1.0f;

    Lock();

    VERIFYGL(glUseProgram(s_uiProgramObject));
    VERIFYGL(glUniformMatrix4fv(s_matrixProjectionLoc, 1, GL_FALSE, (GLfloat*)&m_matProj[0]));
    VERIFYGL(glViewport(0, 0, uWidth, uHeight));

    if (S_OK != hr) {
        RETAILMSG(ZONE_VERBOSE, (TEXT("COpenGLDevice[%d]::Resize() failed, err = 0x%x"), m_Instance, hr));
    }

Error:
    Unlock();

    return hr;
}


// ----------------------------------------------------
// COpenGLDevice::CreateTexture
//
// Create a new texture for the caller
//
// ----------------------------------------------------
HRESULT 
COpenGLDevice::CreateTexture(
        __in int fRenderTarget,
        __in UINT nWidth,
        __in UINT nHeight,
        __in int fKeepSystemMemory,
        __out ICustomSurface **ppSurface
        )
{
    RETAILMSG(ZONE_FUNCTION|ZONE_SURFACE, (TEXT("COpenGLDevice[%d]::CreateTexture( %s ) %d x %d"), 
        m_Instance, fKeepSystemMemory ? L"KEEP ON CPU" : L"DISCARDABLE", nWidth, nHeight ));

    HRESULT hr = S_OK;
    COpenGLSurface *pOGLSurface;

    if ( Use16BitTextures() )
    {
        CHR(COpenGLSurface::Create(this, fRenderTarget, nWidth, nHeight, fKeepSystemMemory, XRPixelFormat_pixelColor16bpp_R5G6B5, &pOGLSurface));
    }
    else
    {
        CHR(COpenGLSurface::Create(this, fRenderTarget, nWidth, nHeight, fKeepSystemMemory, XRPixelFormat_pixelColor32bpp_A8R8G8B8, &pOGLSurface));
    }

    *ppSurface = pOGLSurface;
    pOGLSurface = NULL;

Error:
    return(hr);
}



// ----------------------------------------------------
// CWriteableBitmapDevice::Create16bppTexture
//
// Create a new texture for the caller.  Use R5G6B5
// regardless of the plugin's default texture type.
//
// This method is part of ITileEngineGraphicsDevice,
// and is called ONLY by the IE Tiling Engine, never by
// XamlRuntime.
//
// ----------------------------------------------------
HRESULT 
COpenGLDevice::Create16BppTexture(
    __in int fRenderTarget, 
    __in UINT nWidth, 
    __in UINT nHeight, 
    __in int fKeepSystemMemory, 
    __out ICustomSurface** ppSurface
    )
{
    RETAILMSG(ZONE_FUNCTION|ZONE_SURFACE, (TEXT("COpenGLDevice[%d]::Create16BppTexture( %s ) %d x %d"),
        m_Instance, fKeepSystemMemory ? L"KEEP ON CPU" : L"DISCARDABLE", nWidth, nHeight ));

    HRESULT hr = S_OK;
    COpenGLSurface *pOGLSurface;

    CHR(COpenGLSurface::Create(this, fRenderTarget, nWidth, nHeight, fKeepSystemMemory, XRPixelFormat_pixelColor16bpp_R5G6B5, &pOGLSurface));

    *ppSurface = pOGLSurface;
    pOGLSurface = NULL;

Error:
    return(hr);
}

// ----------------------------------------------------
// COpenGLDevice::SetTexture
//
// Set the texture as active for the next call to DrawTriangleStrip()
// ----------------------------------------------------
HRESULT 
COpenGLDevice::SetTexture(
    __in UINT uSampler,
    __in ICustomSurface *pTexture
    )
{
    UNREFERENCED_PARAMETER(uSampler);
    HRESULT hr = S_OK;

    m_pMostRecentTexture = static_cast<COpenGLSurface*>(pTexture);

    RETAILMSG(ZONE_FUNCTION|ZONE_VERBOSE, (TEXT("COpenGLDevice[%d]::SetTexture( %3d )"),
        m_Instance, m_pMostRecentTexture->GetTextureID()));

    return hr;
}



// ----------------------------------------------------
// COpenGLDevice::SetRenderTarget
//
// Set the IN surface as our target for composition or end rendering
// ----------------------------------------------------
HRESULT 
COpenGLDevice::SetRenderTarget(
    __in ICustomSurface *pRenderTarget
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::SetRenderTarget( NOTIMPL )"), m_Instance));

    UNREFERENCED_PARAMETER(pRenderTarget);

    return S_OK;
}



//------------------------------------------------------------------------
// COpenGLDevice::DrawTriangleStrip
//
//------------------------------------------------------------------------
HRESULT COpenGLDevice::DrawTriangleStrip(
        __in_ecount(cVertices) XRVertex *pVertices,  
        UINT cVertices
        )
{
    RETAILMSG(ZONE_RENDER, (TEXT("COpenGLDevice[%d]::DrawTriangleStrip(cVerts: %d, tex: %d)"), 
        m_Instance, cVertices, m_pMostRecentTexture->GetTextureID()));


    HRESULT hr = S_OK;


    Lock();

    VERIFYGL(glVertexAttribPointer(VERTEX_POSITION, 3, GL_FLOAT,         GL_FALSE, sizeof(XRVertex), &pVertices->x));
    VERIFYGL(glVertexAttribPointer(VERTEX_DIFFUSE,  4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(XRVertex), &pVertices->dwDiffuse));
    VERIFYGL(glVertexAttribPointer(TEXTURE_COORD,   2, GL_FLOAT,         GL_FALSE, sizeof(XRVertex), &pVertices->u0));

    if ( dpCurSettings.ulZoneMask & ZONE_ID_VERBOSE )
    {
        for (UINT i = 0; i < cVertices; i++)
        {
            wchar_t buffer[128];

            wsprintf(buffer, L"vPos (%4.4f, %4.4f, %4.4f), vDiffuse 0x%08x, vTexCoords (%1.3f, %1.3f)", 
                pVertices[i].x, pVertices[i].y, pVertices[i].z,
                pVertices[i].dwDiffuse,
                pVertices[i].u0, pVertices[i].v0 );

            RETAILMSG(ZONE_RENDER|ZONE_VERBOSE, (TEXT("%s"), buffer));
        }
    }
#ifdef USE_DEPTH_BUFFER
    VERIFYGL(glPolygonOffset(1.0f, s_polygonOffset));
    s_polygonOffset += 1.0f;
#endif


    if ( m_pMostRecentTexture->Is32Bit() && !IsGlBGRAExtSupported() )
    {
        // XamlRuntime uses BGRA; if the hardware is in RGBA mode the texture colors will look wrong.  
        // So ask the fragment shader to do color conversion.
        glUniform1i(s_colorConversionLoc, 1);
    }
    else
    {
        // Color conversion not needed for RGB or BGRA
        glUniform1i(s_colorConversionLoc, 0);
    }

    VERIFYGL(glActiveTexture(GL_TEXTURE0));
    VERIFYGL(glBindTexture(GL_TEXTURE_2D, m_pMostRecentTexture->GetTextureID()));
    VERIFYGL(glDrawArrays( GL_TRIANGLE_STRIP, 0, cVertices ));

Error:
    Unlock();

    return(hr);
}
        


HRESULT COpenGLDevice::Clear(
        __in UINT uColor
        )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::Clear( 0x%08x )"), m_Instance, uColor));

    HRESULT hr = S_OK;

    GLfloat a, r, g, b;
    GLfloat scale = GLfloat(1.0f / 255.0f); // This is per section 2.1.2 of the OpenGL ES 2.0 spec.

    a = ((uColor & 0xff000000) >> 24) * scale;
    r = ((uColor & 0x00ff0000) >> 16) * scale;
    g = ((uColor & 0x0000ff00) >> 8) * scale;
    b = ((uColor & 0x000000ff) >> 0) * scale;

    Lock();

    VERIFYGL(glClearColor(r, g, b, a));

#ifdef USE_DEPTH_BUFFER
    VERIFYGL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
#else
    VERIFYGL(glClear(GL_COLOR_BUFFER_BIT));
#endif

Error:
    Unlock();

    return(hr);
}



HRESULT COpenGLDevice::Present()
{
    RETAILMSG(ZONE_FUNCTION|ZONE_RENDER, (TEXT("COpenGLDevice[%d]::Present()"), m_Instance));

    HRESULT hr = S_OK;

    if ( dpCurSettings.ulZoneMask & ZONE_ID_PERF )
    {
        PrintPerfStats();
    }

#ifdef USE_DEPTH_BUFFER
    // reset the offset for coplanar polygons on each frame, so it doesn't increment into infinity
    s_polygonOffset = 0.0f;
#endif

    Lock();

    VERIFYEGL(eglSwapBuffers(s_eglDisplay, m_eglSurface));

Error:
    Unlock();

    return hr;
}



HRESULT 
COpenGLDevice::Present(
        __in XRRECT_WH bounds, 
        __in XRRECT_WH clip, 
        __in UINT bAlphaBlend)
{
    RETAILMSG(ZONE_FUNCTION|ZONE_RENDER, (TEXT("COpenGLDevice[%d]::Present( NOTIMPL )"), m_Instance));

    UNREFERENCED_PARAMETER(bounds);
    UNREFERENCED_PARAMETER(clip);
    UNREFERENCED_PARAMETER(bAlphaBlend);

    return S_OK;
}



HRESULT 
COpenGLDevice::GetTextureMemoryUsage(
        UINT *puTextureMemoryUsage,
        UINT *puTextureMemoryUsageNPOT
        )
{    
    RETAILMSG(ZONE_FUNCTION|ZONE_MEMORY, (TEXT("COpenGLDevice[%d]::GetTextureMemoryUsage()"), m_Instance));

    // For OpenGL 2.0, all our textures are non-power-of-two by default
    // So we don't track separate byte counts

    if (puTextureMemoryUsage)
    {
        *puTextureMemoryUsage = COpenGLSurface::GetTextureMemoryUsage() / 1024;
    }

    if (puTextureMemoryUsageNPOT)
    {
        *puTextureMemoryUsageNPOT = COpenGLSurface::GetTextureMemoryUsage() / 1024;
    }

    return S_OK;
}



//-------------------------------------------------------------------------
//
//  Function:   COpenGLDevice::CreateDisplaySurface
//
//  Synopsis:
//     Creates an EGL surface corresponding to our HWND, and
//     makes it current.
//
//-------------------------------------------------------------------------
HRESULT
COpenGLDevice::CreateDisplaySurface()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::CreateDisplaySurface()"), m_Instance));

    HRESULT hr = S_OK;

    if (EGL_NO_SURFACE != m_eglSurface)
        return hr;


    EGLint attribs[] =
    {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    Lock();

    RETAILMSG(ZONE_INFO, (TEXT("COpenGLDevice[%d]: Parent window: %dx%d @ (%d,%d)"),
        m_Instance,
        m_rcWindow.right  - m_rcWindow.left,
        m_rcWindow.bottom - m_rcWindow.top,
        m_rcWindow.left, m_rcWindow.top ));



    m_eglSurface = eglCreateWindowSurface(s_eglDisplay, s_eglActiveConfig, m_hwnd, attribs);
    if (m_eglSurface == EGL_NO_SURFACE)
    {
        RETAILMSG(ZONE_ERROR, (TEXT("ERROR: COpenGLDevice[%d]::CreateDisplaySurface(): eglCreateWindowSurface() = 0x%x"),
            m_Instance, eglGetError()));
        CHR(E_FAIL);
    }

    RETAILMSG(ZONE_INFO, (TEXT("COpenGLDevice[%d] created EGL window 0x%08x, hWnd = 0x%08x"), 
        m_Instance, m_eglSurface, m_hwnd));

    RETAILMSG(ZONE_SERIALIZE, (TEXT("eglMakeCurrent( disp 0x%x, surf 0x%x, surf 0x%x, ctx 0x%x  )"),
        s_eglDisplay, m_eglSurface, m_eglSurface, s_eglContext));

    VERIFYEGL(eglMakeCurrent(s_eglDisplay, m_eglSurface, m_eglSurface, s_eglContext));

Error:
    Unlock();

    return(hr);
}



//*************************************************************************
//
//                      PRIVATE CLASS METHODS
//
//*************************************************************************



//-------------------------------------------------------------------------
//
//  Function:   COpenGLDevice::InitializeState
//
//  Synopsis:
//     Init our device state
//
//-------------------------------------------------------------------------
HRESULT
COpenGLDevice::InitializeState()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::InitializeState()"), m_Instance));

    HRESULT hr = S_OK;
    BOOL    bStatus;

    // Only need do this once per process.
    // The context and shaders will be shared between all threads and window surfaces.
    if (s_NumInstances > 1)
        return hr;

    Lock();

    {
    RETAILMSG(ZONE_INFO, (TEXT("GL_VENDOR       = %S\n"),   glGetString(GL_VENDOR)));
    RETAILMSG(ZONE_INFO, (TEXT("GL_RENDERER     = %S\n"),   glGetString(GL_RENDERER)));
    RETAILMSG(ZONE_INFO, (TEXT("GL_VERSION      = %S\n"),   glGetString(GL_VERSION)));
    RETAILMSG(ZONE_INFO, (TEXT("GLSL_VERSION    = %S\n"),   glGetString(GL_SHADING_LANGUAGE_VERSION)));
    RETAILMSG(ZONE_INFO, (TEXT("GL_EXTENSIONS   = %S\n"),   glGetString(GL_EXTENSIONS)));

    GLint       iParam;
    VERIFYGL(glGetIntegerv(GL_SUBPIXEL_BITS,  &iParam));
    RETAILMSG(ZONE_INFO, (TEXT("GL_SUBPIXEL_BITS = %d\n"), iParam));
    VERIFYGL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,  &iParam));
    RETAILMSG(ZONE_INFO, (TEXT("GL_MAX_TUs       = %d\n"), iParam));
    RETAILMSG(ZONE_INFO, (TEXT("BGRA textures    = %S\n\n"), IsSupportedGLExt("GL_EXT_bgra") ? "YES" : "NO" ));
    }

    VERIFYGL(glEnableVertexAttribArray(VERTEX_POSITION));
    VERIFYGL(glEnableVertexAttribArray(VERTEX_DIFFUSE));
    VERIFYGL(glEnableVertexAttribArray(TEXTURE_COORD));

    // Note: Nvidia Tegra GPUs ignore this!
    // You have to set the blend mode directly in the fragment shader, or when it's compiled.
    VERIFYGL(glEnable(GL_BLEND));
    VERIFYGL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // Enable back-face culling for performance
    VERIFYGL(glFrontFace(GL_CW));
    VERIFYGL(glCullFace(GL_BACK));
#ifdef USE_BACKFACE_CULLING
    VERIFYGL(glEnable(GL_CULL_FACE));
#else
    VERIFYGL(glDisable(GL_CULL_FACE));
#endif

#ifdef USE_DEPTH_BUFFER
    VERIFYGL(glEnable(GL_DEPTH_TEST));
    VERIFYGL(glDepthFunc(GL_LEQUAL));  // would prefer GL_LESS, if we can make all surfaces not coplanar
    //VERIFYGL(glDepthFunc(GL_LESS));
    VERIFYGL(glEnable(GL_POLYGON_OFFSET_FILL));
#else
    VERIFYGL(glDisable(GL_DEPTH_TEST));
#endif

#ifdef USE_ANTIALIASING
    VERIFYGL(glSampleCoverage(0.25f, GL_FALSE));
    VERIFYGL(glEnable(GL_SAMPLE_COVERAGE));
#endif

    // Tell the driver all texture scanlines are DWORD-aligned,
    // even when using 16-bit pixels.  
    VERIFYGL(glPixelStorei(GL_PACK_ALIGNMENT, 4));
    VERIFYGL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));


    // Initialize the shaders
    // If we can't load binary shaders that match this hardware,
    // the following source will be compiled instead.
    // But you should really provide optimized binary shaders.

    CHR(InitializeShader(GL_VERTEX_SHADER, L"DefaultEffect.vs", 
        "precision highp float;\r\n"
        "attribute vec4 inPosition;\r\n"
        "attribute vec4 inDiffuse;\r\n"
        "attribute vec2 inTextureCoordinate;\r\n"
        "\r\n"
        "uniform mat4 matViewProjection;\r\n"
        "varying vec2 textureCoordinate;\r\n"
        "varying vec4 diffuse;\r\n"
        "\r\n"
        "void main()\r\n"
        "{\r\n"
        "   // Transform the vertex into screen space\r\n"
        "   gl_Position = matViewProjection * vec4(inPosition.xyz, 1.0);\r\n"
        "\r\n"
        "   // XamlRuntime arbitrarily sets Z = 0.5\r\n" 
        "   // which is in front of the screen/near clip plane for OpenGL\r\n"
        "   // Zero it.\r\n"
        "   gl_Position.z = 0.0;\r\n"
        "\r\n"
        "   // Pass to fragment shader\r\n"
        "   diffuse             = inDiffuse;\r\n"
        "   textureCoordinate   = inTextureCoordinate;\r\n"
        "}\r\n"
        , &s_uiVertShader));

    if (s_uiVertShader == 0)
    {
        CHR(E_FAIL);
    }

    CHR(InitializeShader(GL_FRAGMENT_SHADER, L"DefaultEffect.fs", 
        "precision highp float;\r\n"
        "uniform    sampler2D   inTexture1;\r\n"
        "varying    vec2        textureCoordinate;\r\n"
        "varying    vec4        diffuse;\r\n"
        "\r\n"
        "uniform    bool        colorConversion;\r\n"
        "\r\n"
        "void main(void)\r\n"
        "{\r\n"
        "   vec4 color = diffuse * texture2D(inTexture1, textureCoordinate);\r\n"
        "\r\n"
        "   // This is worth removing if you know your hardware does BGRA;\r\n"
        "   // even the if() test can be expensive.\r\n"
        "   if (colorConversion)\r\n"
        "   {\r\n"
        "       // Do BGRA -> RGBA color conversion\r\n"
        "       float temp  = color.r;\r\n"
        "       color.r     = color.b;\r\n"
        "       color.b     = temp;\r\n"
        "   }\r\n"
        "\r\n"
        "   gl_FragColor = color;\r\n"
        "}\r\n"
        , &s_uiFragShader));

    if (s_uiFragShader == 0)
    {
        CHR(E_FAIL);
    }

    s_uiProgramObject = glCreateProgram();

    VERIFYGL(glAttachShader(s_uiProgramObject, s_uiFragShader));
    VERIFYGL(glAttachShader(s_uiProgramObject, s_uiVertShader));

    VERIFYGL(glBindAttribLocation(s_uiProgramObject, VERTEX_POSITION, "inPosition"));
    VERIFYGL(glBindAttribLocation(s_uiProgramObject, VERTEX_DIFFUSE,  "inDiffuse"));
    VERIFYGL(glBindAttribLocation(s_uiProgramObject, TEXTURE_COORD,   "inTextureCoordinate"));

    VERIFYGL(glLinkProgram(s_uiProgramObject));
    VERIFYGL(glGetProgramiv(s_uiProgramObject, GL_LINK_STATUS, &bStatus));
    if (!bStatus)
    {
        CHR(E_FAIL);
    }

    VERIFYGL(glUseProgram(s_uiProgramObject));

    // Cache some shader parameter locations
    s_matrixProjectionLoc = glGetUniformLocation(s_uiProgramObject, "matViewProjection");
    s_checkerBoardLoc     = glGetUniformLocation(s_uiProgramObject, "useCheckerboard");
    s_colorConversionLoc  = glGetUniformLocation(s_uiProgramObject, "colorConversion");

    if ( UseCheckerboard() )
    {
        glUniform1i(s_checkerBoardLoc, 1);
        RETAILMSG(ZONE_INFO, (TEXT("COpenGLDevice: Enable checkerboard in fragment shader")));
    }
    else
    {
        glUniform1i(s_checkerBoardLoc, 0);
    }

    //glReleaseShaderCompiler();

    VERIFYEGL(eglSwapInterval(s_eglDisplay, 0));

Error:
    Unlock();

    return(hr);
}



//*************************************************************************
//
//                      STATIC CLASS METHODS
//
//*************************************************************************


//-------------------------------------------------------------------------
//
//  Function:   LoadShaderBinary
//
//  Synopsis:
//     Load a binary shader from a resource (if one exists). This returns
//     S_OK if successful, S_FALSE if there is no compatible binary shader,
//     and an error otherwise.
//
//-------------------------------------------------------------------------
HRESULT 
COpenGLDevice::LoadShaderBinary(
    GLenum  eShaderType,
    LPCWSTR pShaderName,
    GLuint* puiHandle
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::LoadShaderBinary()"), m_Instance));


    HRESULT   hr = S_FALSE;
    HINSTANCE hShaderDLL = NULL;
    GLint     nBinaryFormats = -1;
    GLint*    pBinaryFormats = NULL;
    HRSRC     hSrc;
    HGLOBAL   hGlobal;
    LPVOID    pBytes;
    DWORD     nSize;

    CPREx(puiHandle, E_POINTER);
    *puiHandle = 0;


    Lock();

    // Try to load a shader resource DLL.
    // The BSP should have provided this.
    hShaderDLL = LoadLibrary( L"shaders.dll" );
    if (NULL == hShaderDLL)
    {
        RETAILMSG(ZONE_WARNING, (TEXT("Failed to load shaders.dll")));
        CHR(E_FAIL);
    }

    // Determine binary format
    VERIFYGL(glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &nBinaryFormats));
    if (nBinaryFormats < 0)
    {
        CHR(E_INVALIDARG);
    }
    if (nBinaryFormats > 0)
    {
        pBinaryFormats = new GLint[nBinaryFormats];
        CPREx(pBinaryFormats, E_OUTOFMEMORY);
        pBinaryFormats[0] = 0xDEADBEEF;

        glGetIntegerv(GL_SHADER_BINARY_FORMATS, pBinaryFormats);
        if (0xDEADBEEF == pBinaryFormats[0])
        {
            // Driver bug: a certain platform doesn't support querying the ID
            *pBinaryFormats = 0x1;
            // clear the error code
            glGetError();
        }

        // Try each format
        for (int i=0; i<nBinaryFormats; i++)
        {
            RETAILMSG(ZONE_INFO, (TEXT("Loading shader [%s]"), pShaderName));

            hSrc = FindResource(hShaderDLL, pShaderName, MAKEINTRESOURCE(eShaderType));
            if (hSrc == NULL)
            {
                continue;
            }

            nSize = SizeofResource(hShaderDLL, hSrc);
            if (nSize == 0)
            {
                CHR(E_FAIL);
            }

            hGlobal = LoadResource(hShaderDLL, hSrc);
            if (hGlobal == NULL)
            {
                CHR(E_FAIL);
            }

            pBytes = LockResource(hGlobal);
            if (pBytes == NULL)
            {
                CHR(E_FAIL);
            }

            VERIFYGL(*puiHandle = glCreateShader(eShaderType));

            glShaderBinary(1, puiHandle, pBinaryFormats[i], pBytes, nSize);
            GLint error = glGetError();
            if (error != GL_NO_ERROR)
                RETAILMSG(ZONE_VERBOSE, (TEXT("glShaderBinary(): error = 0x%x"), error));
                   

            hr = S_OK; // We found one
            break;
        }
    }

Error:
    Unlock();

    if (hShaderDLL)
        FreeLibrary( hShaderDLL );

    if (S_OK != hr)
        RETAILMSG(ZONE_VERBOSE, (TEXT("Failed to load binary shader")));

    SAFE_ARRAYDELETE(pBinaryFormats);
    return(hr);
}



//-------------------------------------------------------------------------
//
//  Function:   CompileShader
//
//  Synopsis:
//     Compile a shader program
//
//-------------------------------------------------------------------------
HRESULT 
COpenGLDevice::CompileShader(
    GLenum      eShaderType,
    const char* pShaderSource,
    GLuint*     puiHandle
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::CompileShader()"), m_Instance));

    HRESULT hr = S_OK;

    CPREx(puiHandle, E_POINTER);

    Lock();

    *puiHandle = glCreateShader(eShaderType);

    // Create and compile the shader object
    VERIFYGL(glShaderSource(*puiHandle, 1, &pShaderSource, NULL));
    VERIFYGL(glCompileShader(*puiHandle));

    // Test if compilation succeeded
    GLint ShaderCompiled;
    glGetShaderiv(*puiHandle, GL_COMPILE_STATUS, &ShaderCompiled);
    if (!ShaderCompiled)
    {
        int i32InfoLogLength, i32CharsWritten;
        glGetShaderiv(*puiHandle, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
        char* pszInfoLog = new char[i32InfoLogLength];

        glGetShaderInfoLog(*puiHandle, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
        RETAILMSG(ZONE_ERROR, (TEXT("CompileShader: Error: %S\r\n"), pszInfoLog));
        SAFE_ARRAYDELETE(pszInfoLog);

        glDeleteShader(*puiHandle);
        *puiHandle = 0;
        CHR(E_FAIL);

        DEBUGCHK(0);
    }
    else
    {
        RETAILMSG(ZONE_INFO, (TEXT("Shader compiled OK\r\n")));
    }

Error:
    Unlock();

    return(hr);
}



//-------------------------------------------------------------------------
//
//  Function:   InitializeShader
//
//  Synopsis:
//     Load a binary shader if one is found that matches the current hardware. 
//     Otherwise we try to compile generic source code at runtime.
//
//-------------------------------------------------------------------------
HRESULT 
COpenGLDevice::InitializeShader(
    GLenum  eShaderType,
    LPCWSTR pShaderName,
    const char* pShaderSource,
    GLuint* puiHandle
    )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::InitializeShader()"), m_Instance));

    HRESULT hr = S_OK;

    CPREx(puiHandle, E_POINTER);
    *puiHandle = NULL;

    // Try to load a binary shader
    hr = LoadShaderBinary(eShaderType, pShaderName, puiHandle);

    Lock();

    // If that didn't work, try to compile it
    if (S_OK != hr)
    {
        GLboolean bHasShaderCompiler = false;
        VERIFYGL(glGetBooleanv(GL_SHADER_COMPILER, &bHasShaderCompiler));
        RETAILMSG(ZONE_VERBOSE, (TEXT("Hardware %s runtime shader compilation."), bHasShaderCompiler ? L"supports" : L"does not support"));
        
        if (bHasShaderCompiler)
        {
            CHR(CompileShader(eShaderType, pShaderSource, puiHandle));
        }
        else
        {
            RETAILMSG(ZONE_ERROR, (TEXT("Found no binary shader and unable to compile; fatal errors may occur.")));
            hr = E_FAIL;
            DEBUGCHK(0);
        }
    }

Error:
    Unlock();

    return(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   COpenGLDevice::FindExtString
//
//  Synopsis:
//     Searches for an OGLES/EGL extension function name with a string
//     of function names separated by whitespaces.
//-------------------------------------------------------------------------
BOOL
COpenGLDevice::FindExtString(__in_z const char * cszString, __in_z const char * cszName) const
{
    if (!cszString)
        return FALSE;

    while(*cszString)
    {
        unsigned int len = strcspn(cszString, " ");
        if ((strlen(cszName) == len) && (strncmp(cszName, cszString, len) == 0))
        {
            return TRUE;
        }
        cszString += (len + 1);
    }
    return FALSE;
}

//-------------------------------------------------------------------------
//
//  Function:   COpenGLDevice::IsSupportedGLExt
//
//  Synopsis:
//     Returns TRUE if the provided GL extension name is supported by the
//     OGLES implementation.
//-------------------------------------------------------------------------
BOOL 
COpenGLDevice::IsSupportedGLExt(__in_z const char * cszName)
{
    Lock();

    const char * cszGLString = (const char *)glGetString(GL_EXTENSIONS);

    Unlock();


    if (!cszGLString)
        return FALSE;

    BOOL rval = FindExtString(cszGLString, cszName);    

    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::IsSupportedGLExt( %S ) = %s"), 
        m_Instance,
        cszName,
        rval ? L"true" : L"false" ));

    return rval;
}

//-------------------------------------------------------------------------
//
//  Function:   COpenGLDevice::IsSupportedEGLExt
//
//  Synopsis:
//     Returns TRUE if the provided EGL extension name is supported by the
//     OGLES implementation.
//-------------------------------------------------------------------------
BOOL 
COpenGLDevice::IsSupportedEGLExt(__in_z const char * cszName)
{
    Lock();

    const char * cszEGLString = eglQueryString(s_eglDisplay, EGL_EXTENSIONS);

    Unlock();


    if (!cszEGLString)
        return FALSE;

    BOOL rval = FindExtString(cszEGLString, cszName);    

    RETAILMSG(ZONE_FUNCTION, (TEXT("COpenGLDevice[%d]::IsSupportedEGLExt( %S ) = %s"), 
        m_Instance,
        cszName,
        rval ? L"true" : L"false" ));

    return rval;
}

//-------------------------------------------------------------------------
//
//  Function:   PrintDisplayConfig(EGLConfig config)
//
//-------------------------------------------------------------------------
HRESULT
COpenGLDevice::PrintDisplayConfig(EGLConfig config)
{
    HRESULT hr = S_OK;

    EGLint totalConfigs = 0;
    EGLConfig configs[32];

    Lock();

    eglGetConfigs(s_eglDisplay, configs, 32, &totalConfigs);

    bool bFoundConfig = false;
    for (int i = 0; i < totalConfigs; i++)
    {
        if (configs[i] == config)
        {
            bFoundConfig = true;
            break;
        }
    }

    if (!bFoundConfig)
        CHR(E_FAIL);


    EGLConfig configId;
    eglGetConfigAttrib(s_eglDisplay, config, EGL_CONFIG_ID, (EGLint*)&configId);

    EGLint red=0, blue=0, green=0, alpha=0;
    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_RED_SIZE,  &red));
    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_GREEN_SIZE,  &green));
    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_BLUE_SIZE,  &blue));
    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_ALPHA_SIZE,  &alpha));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: r%dg%db%da%d\n"), configId, red, green, blue, alpha));

    EGLint value;
    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_BUFFER_SIZE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_BUFFER_SIZE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_LUMINANCE_SIZE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_LUMINANCE_SIZE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_ALPHA_MASK_SIZE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_ALPHA_MASK_SIZE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_BIND_TO_TEXTURE_RGB,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_BIND_TO_TEXTURE_RGB = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_BIND_TO_TEXTURE_RGBA,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_BIND_TO_TEXTURE_RGBA = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_COLOR_BUFFER_TYPE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_COLOR_BUFFER_TYPE = 0x%x\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_CONFIG_CAVEAT,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_CONFIG_CAVEAT = 0x%x\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_CONFORMANT,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_CONFORMANT = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_DEPTH_SIZE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_DEPTH_SIZE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_LEVEL,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_LEVEL = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_MAX_PBUFFER_WIDTH,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_MAX_PBUFFER_WIDTH = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_MAX_PBUFFER_HEIGHT,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_MAX_PBUFFER_HEIGHT = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_MAX_PBUFFER_PIXELS,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_MAX_PBUFFER_PIXELS = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_MAX_SWAP_INTERVAL,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_MAX_SWAP_INTERVAL = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_MIN_SWAP_INTERVAL,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_MIN_SWAP_INTERVAL = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_NATIVE_RENDERABLE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_NATIVE_RENDERABLE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_NATIVE_VISUAL_ID,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_NATIVE_VISUAL_ID = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_NATIVE_VISUAL_TYPE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_NATIVE_VISUAL_TYPE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_RENDERABLE_TYPE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_RENDERABLE_TYPE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_SURFACE_TYPE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_SURFACE_TYPE = %d\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_TRANSPARENT_TYPE,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_TRANSPARENT_TYPE = 0x%x\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_SAMPLE_BUFFERS,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_SAMPLE_BUFFERS = 0x%x\n"), configId, value));

    VERIFYEGL(eglGetConfigAttrib(s_eglDisplay, config, EGL_SAMPLES,  &value));
    RETAILMSG(ZONE_INFO, (TEXT("config[%d]: EGL_SAMPLES = 0x%x\n\n"), configId, value));

Error:
    Unlock();

    return hr;
}


void
COpenGLDevice::GetRegistryOptions()
{
    HKEY  hKey    = 0;
    DWORD dwType  = 0;
    DWORD dwSize  = sizeof ( DWORD );
    DWORD dwValue = 0;

    if( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\XamlRenderOpenGL", 0, 0, &hKey )) {
        goto Exit;
    }

    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"16BitTextures", 0, &dwType, (BYTE *)&dwValue, &dwSize ) && dwValue ) {
        m_bUse16BitTextures = true;
    }

    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"16BitWindow", 0, &dwType, (BYTE *)&dwValue, &dwSize ) && dwValue ) {
        m_bUse16BitWindow = true;
    }

    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"Checkerboard", 0, &dwType, (BYTE *)&dwValue, &dwSize ) && dwValue ) {
        m_bUseCheckerboard = true;
    }

    // By default all textures are DWORD-aligned at the start of each scanline; we also request that mode
    // by calling glPixelStorei(GL_UNPACK_ALIGNMENT, 4).
    // This is only required for buggy drivers that expect WORD-aligned RGB565 textures and don't honor glPixelStorei.
    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"WORDAlignedTextures", 0, &dwType, (BYTE *)&dwValue, &dwSize ) && dwValue ) {
        m_bUseWORDAligned565 = true;
    }

Exit:
    if ( hKey )
        RegCloseKey( hKey );
}



HRESULT 
COpenGLDevice::Lock()
{
    HRESULT hr = S_OK;

    // 
    // Acquire the OpenGL CRITICAL_SECTION.
    // This prevents multiple threads in the same process
    // from entering the OpenGL driver simultaneously
    // (the driver must ensure protection between processes).
    // Threads should lock for the duration of rendering their window: 
    //      PreRender() - PostRender()
    //

    EnterCriticalSection( &s_OpenGLCS );

    ++s_LockCount;
    RETAILMSG(ZONE_SERIALIZE, (TEXT("COpenGLDevice[%d] thread [0x%x] took OpenGL CS, lock = %d"), m_Instance, GetCurrentThreadId(), s_LockCount ));

    if (EGL_NO_CONTEXT != s_eglContext && EGL_NO_DISPLAY != s_eglDisplay && EGL_NO_SURFACE != m_eglSurface)
    {
        EGLContext currentContext = eglGetCurrentContext();
        EGLDisplay currentDisplay = eglGetCurrentDisplay();
        EGLSurface currentSurface = eglGetCurrentSurface(EGL_DRAW);

        // Perf warning if current context/surface != my context/surface.
        // Ideally we do this once per window per frame, in a sequential fashion.
        // Otherwise we're thrashing the GPU.
        if ((currentContext != s_eglContext) || 
            (currentDisplay != s_eglDisplay) || 
            (currentSurface != m_eglSurface))
        {
            RETAILMSG(ZONE_PERF, (TEXT("COpenGLDevice[%d]: EGL CONTEXT SWITCH"), m_Instance));
            RETAILMSG(ZONE_PERF, (TEXT("Context 0x%08x -> 0x%08x"), eglGetCurrentContext(),         s_eglContext));
            RETAILMSG(ZONE_PERF, (TEXT("Display 0x%08x -> 0x%08x"), eglGetCurrentDisplay(),         s_eglDisplay));
            RETAILMSG(ZONE_PERF, (TEXT("Surface 0x%08x -> 0x%08x"), eglGetCurrentSurface(EGL_DRAW), m_eglSurface));

            VERIFYEGL(eglMakeCurrent(s_eglDisplay, m_eglSurface, m_eglSurface, s_eglContext));

            RETAILMSG(ZONE_SERIALIZE, (TEXT("COpenGLDevice[%d] thread [0x%x] eglMakeCurrent( eglDisplay: 0x%x eglContext: 0x%x eglSurface: 0x%x )"),
                m_Instance,
                GetCurrentThreadId(),
                eglGetCurrentDisplay(),
                eglGetCurrentContext(),
                eglGetCurrentSurface(EGL_DRAW)));


            //
            // Reset the viewport any time we change windows
            //
            if (currentSurface != m_eglSurface && m_eglSurface != EGL_NO_SURFACE)
            {
                VERIFYGL(glUniformMatrix4fv(s_matrixProjectionLoc, 1, GL_FALSE, (GLfloat*)&m_matProj[0]));
                VERIFYGL(glViewport(0, 0, m_uRenderWidth, m_uRenderHeight));
            }
        }
    }

Error:
    return hr;
}


HRESULT 
COpenGLDevice::Unlock()
{
    RETAILMSG(ZONE_SERIALIZE, (TEXT("Thread [0x%x] released OpenGL CS, lock = %d"), GetCurrentThreadId(), s_LockCount-1 ));

    --s_LockCount;
    LeaveCriticalSection( &s_OpenGLCS );

    return S_OK;
}



void
COpenGLDevice::PrintPerfStats()
{
    static DWORD s_dwLowWaterMark  = 0xFFFFFFFF;
    static DWORD s_dwHighWaterMark = 0;
    static DWORD s_dwMaxSurfaces   = 0;

    m_dwFrameCount++;

    DWORD dwCurrTick = GetTickCount();
    DWORD dwElapsed = dwCurrTick - m_dwLastTick;
    if (dwElapsed >= 1000)
    {
        TCHAR szMsg[256];

        MEMORYSTATUS      memstat;
        memstat.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus( &memstat );
        
        DWORD freeRAM     =  memstat.dwAvailPhys;
        DWORD texmem      =  COpenGLSurface::GetTextureMemoryUsage();
        DWORD surfaces    =  COpenGLSurface::GetSurfaceCount();

        s_dwLowWaterMark  = min( s_dwLowWaterMark,  memstat.dwAvailPhys );
        s_dwHighWaterMark = max( s_dwHighWaterMark, memstat.dwAvailPhys );
        s_dwMaxSurfaces   = max( s_dwMaxSurfaces,   surfaces            );


        wsprintf(szMsg, TEXT("COpenGLDevice[%d]: FPS: %2.2f, free RAM: %d, CPU texture cache: %d, surface count: %d\r\n"), 
            m_Instance,
            (double)m_dwFrameCount / dwElapsed * 1000,
            freeRAM,
            texmem,
            surfaces
             );
        RETAILMSG(ZONE_PERF, (szMsg));


        wsprintf(szMsg, TEXT("COpenGLDevice[%d]: min RAM: %d, max RAM: %d, max surfaces: %d\r\n"), 
            m_Instance, s_dwLowWaterMark, s_dwHighWaterMark, s_dwMaxSurfaces );
        RETAILMSG(ZONE_PERF, (szMsg));


        m_dwLastTick = dwCurrTick;
        m_dwFrameCount = 0;
    }

    return;
}