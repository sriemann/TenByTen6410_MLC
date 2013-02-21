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
#include <EGL\egl.h>
#include <EGL\eglext.h>
#include <GLES2\gl2.h>
#include <GLES2\gl2ext.h>
#include "debug.hpp"
#include "config.hpp"
#include "macros.hpp"
#include <ehm.h>
#include "OpenGLDevice.hpp"
#include "OpenGLSurface.hpp"

class FrameBufferObject
{
public:
    static 
    HRESULT     Create          ( FrameBufferObject** ppFrameBufferObject );


    HRESULT     Initialize      ( COpenGLDevice* pOGLDevice, unsigned int width, unsigned int height );
    void        Deinitialize    ( );

    UINT AddRef();
    UINT Release();

    HRESULT     UseAsRenderTarget( bool bVal );
    BOOL        IsRenderTarget   ( );
    GLuint      GetTexture       ( );

protected:
    FrameBufferObject();
    virtual ~FrameBufferObject();


protected:
    UINT        m_RefCount;

    COpenGLDevice*  m_pOGLDevice;

    bool        m_fEnabledForRendering;

    GLuint      m_uiFramebuffer;
    GLuint      m_uiDepthRenderbuffer;
    GLuint      m_uiTextureID;
    GLint       m_TexWidth;
    GLint       m_TexHeight;
    GLint       m_MaxRenderbufferSize;
};
