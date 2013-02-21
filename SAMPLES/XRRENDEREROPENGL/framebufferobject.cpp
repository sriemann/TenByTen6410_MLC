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

#include "FrameBufferObject.hpp"


HRESULT
FrameBufferObject::Create( FrameBufferObject** ppFrameBufferObject )
{
    if (!ppFrameBufferObject)
        return E_POINTER;

    *ppFrameBufferObject = new FrameBufferObject();

    if (!*ppFrameBufferObject)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        return S_OK;
    }
}


FrameBufferObject::FrameBufferObject() :
    m_RefCount(1),
    m_pOGLDevice(NULL),
    m_fEnabledForRendering(false),
    m_uiFramebuffer(0),
    m_uiDepthRenderbuffer(0),
    m_uiTextureID(0),
    m_TexWidth(0),
    m_TexHeight(0),
    m_MaxRenderbufferSize(0)
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("FrameBufferObject::FrameBufferObject()")));
}



FrameBufferObject::~FrameBufferObject()
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("~FrameBufferObject()")));
    
    Deinitialize();
}



HRESULT
FrameBufferObject::Initialize( COpenGLDevice* pOGLDevice, unsigned int width, unsigned int height )
{
    RETAILMSG(ZONE_INFO, (TEXT("FrameBufferObject::Initialize( %d x %d )"), width, height ));

    HRESULT hr = S_OK;

    if (!pOGLDevice || !width || !height)
    {
        RETAILMSG(ZONE_ERROR, (TEXT("ERROR: FrameBufferObject::Initialize(): pOGLdevice 0x%x, width %d, height %d\n"),
            pOGLDevice, width, height));

        CHR(E_INVALIDARG);
    }

    if (m_pOGLDevice != NULL)
    {
        m_pOGLDevice->Lock();

        // We are being re-initialized (probably because the screen changed size)
        VERIFYGL( glDeleteRenderbuffers(1, &m_uiDepthRenderbuffer  ));
        VERIFYGL( glDeleteFramebuffers (1, &m_uiFramebuffer        ));
        VERIFYGL( glDeleteTextures     (1, &m_uiTextureID          ));

        m_pOGLDevice->Unlock();
        m_pOGLDevice->Release();

        RETAILMSG(ZONE_INFO, (TEXT("FrameBufferObject::Initialize(); discard old FBO because because of resize\n")));
    }

    m_pOGLDevice    = pOGLDevice;   m_pOGLDevice->AddRef();
    m_TexWidth      = width;
    m_TexHeight     = height;

    m_pOGLDevice->Lock();

#ifdef USE_DEPTH_BUFFER
    VERIFYGL(glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &m_MaxRenderbufferSize ));
    if ((m_MaxRenderbufferSize <= m_TexWidth) ||
        (m_MaxRenderbufferSize <= m_TexHeight))
    {
        // cannot use framebuffer objects as we need to create
        // a depth buffer as a renderbuffer object
        DEBUGCHK(0);
        return E_OUTOFMEMORY;
    }

    VERIFYGL(glGenRenderbuffers (1, &m_uiDepthRenderbuffer));
#endif


    // Create a texture and attach to the FBO as a render target
    VERIFYGL(glGenTextures     (1, &m_uiTextureID));
    VERIFYGL(glBindTexture     (GL_TEXTURE_2D, m_uiTextureID));
    VERIFYGL(glTexParameteri   (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    VERIFYGL(glTexParameteri   (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    VERIFYGL(glTexParameteri   (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    VERIFYGL(glTexParameteri   (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

    if ( m_pOGLDevice->Use16BitTextures() )
    {
        VERIFYGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_TexWidth, m_TexHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL));
    } 
    else if ( m_pOGLDevice->IsGlBGRAExtSupported() )
    {
        VERIFYGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, m_TexWidth, m_TexHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL));
    }
    else
    {
        VERIFYGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_TexWidth, m_TexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
    }

    // Generate an off-screen Framebuffer and make it active
    VERIFYGL(glGenFramebuffers  (1, &m_uiFramebuffer));
    VERIFYGL(glBindFramebuffer(GL_FRAMEBUFFER, m_uiFramebuffer));

    // Attach our texture to FBO
    VERIFYGL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_uiTextureID, 0));

#ifdef USE_DEPTH_BUFFER
    // Create a depth buffer and attach to the FBO as a render target
    VERIFYGL(glBindRenderbuffer    (GL_RENDERBUFFER, m_uiDepthRenderbuffer));
    VERIFYGL(glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_TexWidth, m_TexHeight));
    VERIFYGL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_uiDepthRenderbuffer));
    //VERIFYGL(glClear(GL_DEPTH_BUFFER_BIT));
#endif

    VERIFYGL(glClearColor(1.0f, 0.0f, 1.0f, 1.0f));
    VERIFYGL(glClear(GL_COLOR_BUFFER_BIT));

    GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        RETAILMSG(ZONE_ERROR, (TEXT("ERROR: FrameBufferObject::Initialize() failed.  Off-screen rendering disabled.\n")));
        DEBUGCHK(0);
        hr = E_FAIL;
    }

Error:
    // Restore the default OS-provided framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_pOGLDevice->Unlock();

    RETAILMSG(S_OK == hr && ZONE_INFO, (TEXT("FrameBufferObject::Initialize() OK\n")));

    return hr;
}


void
FrameBufferObject::Deinitialize()
{
    if (m_pOGLDevice)
        m_pOGLDevice->Lock();

    if (m_uiDepthRenderbuffer)
    {
        glDeleteRenderbuffers(1, &m_uiDepthRenderbuffer);
        m_uiDepthRenderbuffer = NULL;
    }

    if (m_uiFramebuffer)
    {
        glDeleteFramebuffers(1, &m_uiFramebuffer);
        m_uiFramebuffer = NULL;
    }

    if (m_uiTextureID)
    {
        glDeleteTextures(1, &m_uiTextureID);
        m_uiTextureID = NULL;
    }

    if (m_pOGLDevice)
        m_pOGLDevice->Unlock();
}


HRESULT 
FrameBufferObject::UseAsRenderTarget( bool bVal )
{
    RETAILMSG(ZONE_FUNCTION, (TEXT("FrameBufferObject::UseAsRenderTarget( %d )"), bVal));

    HRESULT hr = S_OK;

    m_fEnabledForRendering = bVal;

    if (m_pOGLDevice)
        m_pOGLDevice->Lock();

    if (m_fEnabledForRendering)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_uiFramebuffer);
    }
    else
    {
        // Restore the default OS-provided framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE  && status != GL_NO_ERROR)
    {
        RETAILMSG(ZONE_ERROR, (TEXT("ERROR: FrameBufferObject::UseAsRenderTarget() failed.\n")));
        DEBUGCHK(0);
        CHR(E_FAIL);
    }

Error:
    if (m_pOGLDevice)
        m_pOGLDevice->Unlock();

    return hr;
}


BOOL
FrameBufferObject::IsRenderTarget()
{
    return m_fEnabledForRendering;
}



GLuint
FrameBufferObject::GetTexture( )
{
    return m_uiTextureID;
}



UINT 
FrameBufferObject::AddRef()
{
    RETAILMSG(ZONE_VERBOSE, (TEXT("FrameBufferObject::AddRef()")));

    return InterlockedIncrement((LONG *)&m_RefCount);
}



UINT
FrameBufferObject::Release()
{
    RETAILMSG(ZONE_VERBOSE, (TEXT("FrameBufferObject::Release()")));

    UINT cRef = InterlockedDecrement((LONG *)&m_RefCount);
    if (cRef == 0)
    {
        RETAILMSG(ZONE_VERBOSE, (TEXT("FrameBufferObject: no more refs; cleaning up\n")));
        delete this;
    }

    return cRef;
}

