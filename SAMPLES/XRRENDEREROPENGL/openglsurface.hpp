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
#include "config.hpp"


// Does this device support the BGRA surface extension?
// OpenGL ES default is RGBA
#define GL_BGRA_EXT     0x80e1


// forward declarations
//
class COpenGLDevice;


//------------------------------------------------------------------------
// class COpenGLSurface
// 
// Abstraction representing an OpenGLSurface (textured quad). 
//
//------------------------------------------------------------------------
class COpenGLSurface : public ICustomSurface
{
public:
    // ICustomSurface methods
    //
    UINT AddRef();
    UINT Release();

    HRESULT Lock(
        __out void **ppAddress,
        __out int *pnStride,
        __out UINT *puWidth,
        __out UINT *puHeight
        );

    HRESULT Unlock();

    HRESULT Present(
        __in HWND hTarget, 
        __in_opt XRPOINT *pOffset, 
        __in UINT bAlphaBlt);

    virtual __checkReturn HRESULT Present(
        __in HWND hTarget, 
        __in XRRECT_WH bounds, 
        __in XRRECT_WH clip, 
        __in UINT bAlphaBlt
        );


    inline UINT          GetWidth()         { return m_uWidth;       }
    inline UINT          GetHeight()        { return m_uHeight;      }
    inline UINT          GetByteCount()     { return m_uByteCount;   }
    inline XRPixelFormat GetPixelFormat()   { return m_pixelFormat;  }

    inline int IsVideoSurface()
    {
        return FALSE;
    }

    inline int IsOpaque() 
    {
        return m_bIsOpaque;
    }

    inline int IsTransparent()
    {
        return m_bIsTransparent;
    }

    inline void SetIsOpaque(__in int bIsOpaque) 
    {
        m_bIsOpaque = bIsOpaque;
    }

    inline void SetIsTransparent(__in int bIsTransparent)
    {
        m_bIsTransparent = bIsTransparent;
    }

    HRESULT AddDirty(__in const XRRECT_WH &rcDirty);
    

    // Public COpenGLSurface specific methods
    //

    static HRESULT COpenGLSurface::Create(
        __in COpenGLDevice *pOGLDevice, 
        __in INT32          fRenderTarget,
        __in UINT32         uWidth,
        __in UINT32         uHeight,
        __in INT32          fKeepSystemMemory,
        __in XRPixelFormat  pixelFormat,
        __out COpenGLSurface **ppOGLSurface
        );

    static inline UINT32 GetTextureMemoryUsage()
    {
        return s_uTextureMemoryUsage;
    }

    static inline UINT32 GetSurfaceCount()
    {
        return s_uSurfaceCount;
    }

    inline UINT32  GetTextureID()  { return m_uTextureID; }

    inline BOOL    Is16Bit()       { return m_pixelFormat == XRPixelFormat_pixelColor16bpp_R5G6B5   ? TRUE : FALSE; }
    inline BOOL    Is32Bit()       { return m_pixelFormat == XRPixelFormat_pixelColor32bpp_A8R8G8B8 ? TRUE : FALSE; }
   
private:    
    // internal functions
    //
    COpenGLSurface();
    virtual ~COpenGLSurface();

    HRESULT Initialize(
        __in COpenGLDevice *pOGLDevice, 
        __in INT32          fRenderTarget,
        __in UINT32         uWidth,
        __in UINT32         uHeight,
        __in XRPixelFormat  pixelFormat,
        __in INT32          fKeepSystemMemory
        );

    void Deinitialize();

    static inline void IncrementTextureMemoryUse(UINT32 * puCounter, UINT32 uAddend)
    {
        InterlockedExchangeAdd((LONG *) puCounter, uAddend);
    }



    static inline void DecrementTextureMemoryUse(UINT32 * puCounter, UINT32 uSubtrahend)
    {
        InterlockedExchangeAdd((LONG *) puCounter, -(INT32) uSubtrahend);
    }

    
protected:
    static UINT32           s_uTextureMemoryUsage;
    static UINT32           s_uSurfaceCount;

    int                     m_fKeepSystemMemory;    // Do not discard pixel data after
                                                    // uploading to GPU, because this
                                                    // texture is likely to be modified 
                                                    // and re-uploaded later

    UINT32                  m_RefCount;
    RECT                    m_rcDirty;       

    int                     m_bIsOpaque;
    int                     m_bIsTransparent;

    UINT                    m_uWidth;
    UINT                    m_uHeight;
    UINT                    m_uStride;
    UINT                    m_uByteCount;
    
    COpenGLDevice          *m_pOpenGLDevice;
    
    void                   *m_pPixelBuffer;
    GLuint                  m_uTextureID;
    UINT32                  m_uTexelSize;
    XRPixelFormat           m_pixelFormat;
};

