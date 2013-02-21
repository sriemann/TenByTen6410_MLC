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
#include <XamlRuntimeGraphics.h>
#include <TileEnginerenderPlugin.h>
#include "config.hpp"


class FrameBufferObject;


// forward declaration
//
class COpenGLSurface;

//------------------------------------------------------------------------
// class COpenGLDevice
// 
// Abstraction representing an object capable of managing, creating and composing
// OpenGL Surfaces (textured quads). 
// Used by the Renderer abstraction for CachedComposition
//
//------------------------------------------------------------------------
class COpenGLDevice : public ITileEngineGraphicsDevice // subclasses ICustomGraphicsDevice
{
public:
    static HRESULT InitializeGlobals();
    static void    CleanupGlobals();
    
    static HRESULT Create(
        HWND hWindow, 
        ICustomGraphicsDevice **ppOGLDevice
        );

    // ICustomGraphicsDevice functions
    //
    virtual UINT AddRef();
    virtual UINT Release();

    virtual __checkReturn HRESULT Resize(UINT uWidth, UINT uHeight);

    virtual __checkReturn HRESULT CreateTexture(
        __in int fRenderTarget,
        __in UINT nWidth,
        __in UINT nHeight,
        __in int fKeepSystemMemory,
        __out ICustomSurface **ppSurface
        );

    virtual __checkReturn HRESULT SetTexture(
        __in UINT uSampler,
        __in ICustomSurface *pTexture
        );

    virtual __checkReturn HRESULT SetRenderTarget(
        __in ICustomSurface *pRenderTarget
        );


    virtual __checkReturn HRESULT DrawTriangleStrip(
        __in_ecount(cVertices) XRVertex *pVertices,  
        UINT cVertices
        );
        
    virtual __checkReturn HRESULT Clear(
        __in UINT uColor
        );

    virtual __checkReturn HRESULT Present();
    
    virtual __checkReturn HRESULT Present(
        __in XRRECT_WH bounds, 
        __in XRRECT_WH clip, 
        __in UINT bAlphaBlend);

    virtual __checkReturn HRESULT GetTextureMemoryUsage(
        UINT *puTextureMemoryUsage,
        UINT *puTextureMemoryUsageNPOT
        );

    virtual __checkReturn bool IsHardwareComposited()
    {
        return true;
    }


    // public ITileEngineGraphicsDevice function
    // This is called ONLY by the IE Tiling Engine, and never by XamlRuntime
    virtual __checkReturn HRESULT Create16BppTexture(
        __in int fRenderTarget, 
        __in UINT nWidth, 
        __in UINT nHeight, 
        __in int fKeepSystemMemory, 
        __out ICustomSurface** ppSurface
        );

    virtual HRESULT Lock();
    virtual HRESULT Unlock();


    // public non-ICustomGraphics functions
    //
    EGLDisplay GetEGLDisplay() const
    {
        return s_eglDisplay;
    }

    BOOL IsSupportedGLExt (__in_z const char * cszName);
    BOOL IsSupportedEGLExt(__in_z const char * cszName);

    BOOL IsGlBGRAExtSupported()   const    { return m_fGlBGRAExt;            }
    BOOL Use16BitTextures()       const    { return m_bUse16BitTextures;     }
    BOOL Use16BitWindow()         const    { return m_bUse16BitWindow;       }
    BOOL UseCheckerboard()        const    { return m_bUseCheckerboard;      }
    BOOL UseWORDAligned565()      const    { return m_bUseWORDAligned565;    }


private:
    // Internal functions
    // 
    COpenGLDevice();
    virtual ~COpenGLDevice();

    void GetRegistryOptions();

    virtual HRESULT Initialize(HWND hWindow);
    virtual void    Deinitialize();

    virtual __checkReturn HRESULT CreateDisplaySurface();


    HRESULT InitializeState();

    HRESULT PrintDisplayConfig(EGLConfig config);

    void    PrintPerfStats();
    
    BOOL FindExtString(__in_z const char * cszString, __in_z const char * cszName) const;


    HRESULT InitializeShader(
                        GLenum  eShaderType,
                        LPCWSTR pShaderName,
                        const char* pShaderSource,
                        GLuint* puiHandle
                        );

    HRESULT LoadShaderBinary(
                        GLenum  eShaderType,
                        LPCWSTR pShaderName,
                        GLuint* puiHandle
                        );

    HRESULT CompileShader(
                        GLenum      eShaderType,
                        const char* pShaderSource,
                        GLuint*     puiHandle
                        );


private:
    static CRITICAL_SECTION s_OpenGLCS;
    static BOOL             s_csCreated;
    static signed long      s_LockCount;

    static UINT             s_NumInstances;

    static EGLDisplay       s_eglDisplay;
    static EGLContext       s_eglContext;
    static EGLConfig        s_eglActiveConfig;
    static EGLConfig        s_eglConfig32bpp;
    static EGLConfig        s_eglConfig16bpp;

    static GLuint           s_uiProgramObject;
    static GLuint           s_uiVertShader;
    static GLuint           s_uiFragShader;
    static GLint            s_matrixProjectionLoc;
    static GLint            s_checkerBoardLoc;
    static GLint            s_colorConversionLoc;
    
    UINT            m_RefCount;
    UINT            m_Instance;
    UINT32          m_uThreadID;

    HWND            m_hwnd;
    RECT            m_rcWindow;
    UINT            m_uRenderWidth;
    UINT            m_uRenderHeight;
    EGLSurface      m_eglSurface;
    GLfloat         m_matProj[16];

    COpenGLSurface* m_pMostRecentTexture;

    UINT32          m_fGlBGRAExt;
    BOOL            m_bUse16BitTextures;
    BOOL            m_bUseCheckerboard;
    BOOL            m_bUseWORDAligned565;   // Only required to work around buggy drivers
    BOOL            m_bUse16BitWindow;

    DWORD           m_dwFrameCount;
    DWORD           m_dwLastTick;
};  
