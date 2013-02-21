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

    surf.cpp

Abstract:

    surface allocation/manipulation/free routines

Functions:


Notes:


--*/

#include "precomp.h"

#define ALIGN(x, align)        (((x) + ((align) - 1)) & ~((align) - 1))

static DWORD dwSurfaceCount = 0;

SCODE
S3C6410Disp::AllocSurfacePACS(
                        GPESurf **ppSurf,
                        int width,
                        int height,
                        EGPEFormat format,
                        int stride, 
                        EDDGPEPixelFormat pixelFormat
                        )
{
    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s( %dx%d FMT:%d stride:%d PFMT: %d )\r\n"), _T(__FUNCTION__), width, height,  format, stride, pixelFormat));

    // try to allocate physically linear address
    *ppSurf = new PACSurf(width, height, format, stride, pixelFormat);
    
    if (*ppSurf == NULL)
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : PACSurface allocate Failed -> Try to allocate Normal GPE Surface\r\n")));
        return E_OUTOFMEMORY;
    }
    else if ((*ppSurf)->Buffer() == NULL)
    {
        delete *ppSurf;
        *ppSurf = NULL;

        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : PACSurface Buffer is NULL -> Try to allocate Normal GPE Surface\r\n")));
    }
    else        /// PAC Allocation succeeded.
    {
        return S_OK;
    }
    return E_FAIL;
}


//  This method is called for all normal surface allocations from ddgpe and gpe
SCODE
S3C6410Disp::AllocSurface(
                        GPESurf **ppSurf,
                        int width,
                        int height,
                        EGPEFormat format,
                        int surfaceFlags)
{
    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s( %dx%d FMT:%d flags:%08x )\r\n"), _T(__FUNCTION__), width, height,  format, surfaceFlags));

    // This method is only for surface in system memory
    if (surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : Can not allocate GPE_REQUIRE_VIDEO_MEMORY Surface in system memory\r\n")));
        return E_OUTOFMEMORY;
    }

    if(m_G2DControlArgs.HWOnOff && m_G2DControlArgs.UsePACSurf)
    {
        /// Only Support 16bpp, 24bpp, 32bpp
        if( format == gpe16Bpp || format == gpe24Bpp || format == gpe32Bpp || format == gpeDeviceCompatible)
        {
            if(width*height*(EGPEFormatToBpp[format] >> 3) > PAC_ALLOCATION_BOUNDARY)
            {
                if(S_OK == AllocSurfacePACS(ppSurf, width, height, format))
                {
                    return S_OK;
                }
            }
        }
    }
    /// if allocation is failed or boundary condition is not met, just create GPESurf in normal system memory that can be non-linear physically.

    // Allocate surface from system memory
    *ppSurf = new GPESurf(width, height, format);

    if (*ppSurf != NULL)
    {
        if (((*ppSurf)->Buffer()) == NULL)
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : OUT OF MEMORY\r\n"), _T(__FUNCTION__)));
            delete *ppSurf;
            return E_OUTOFMEMORY;
        }
        else
        {
            return S_OK;
        }
    }

    RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] AllocSurface() : Surface allocate Failed\r\n")));
    return E_OUTOFMEMORY;
}


//  This method is used for DirectDraw enabled surfaces
SCODE
S3C6410Disp::AllocSurface(
                        DDGPESurf **ppSurf,
                        int width,
                        int height,
                        EGPEFormat format,
                        EDDGPEPixelFormat pixelFormat,
                        int surfaceFlags)
{
    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s( %dx%d FMT:%d PFMT: %d, flags:%08x )\r\n"), _T(__FUNCTION__), width, height,  format, pixelFormat, surfaceFlags));

    unsigned int bpp;
    unsigned int stride;
    unsigned int align_width;

    if (NULL == ppSurf)
    {
        return E_POINTER;
    }

    if (pixelFormat == ddgpePixelFormat_I420 || pixelFormat == ddgpePixelFormat_YV12)
    {
        // in this case, stride can't be calculated. because of planar format (non-interleaved...)
        bpp = 12;
        align_width = ALIGN(width, 16);
    }
    else if (pixelFormat == ddgpePixelFormat_YVYU || pixelFormat == ddgpePixelFormat_VYUY)
    {
        bpp = 16;
        align_width = width;
    }
    else
    {
        bpp = EGPEFormatToBpp[format];
        align_width = width;
    }

    //DISPDRV_ERR((_T("[AS] %dx%d %dbpp FMT:%d F:%08x\r\n"), width, height, bpp, format, surfaceFlags));

    //--------------------------------------
    // Try to allocate surface from video memory
    //--------------------------------------

    // stride are all 32bit aligned for Video Memory
    stride = ((bpp * align_width + 31) >> 5) << 2;

    if ((surfaceFlags & GPE_REQUIRE_VIDEO_MEMORY)
        || (surfaceFlags & GPE_BACK_BUFFER)
        //|| ((surfaceFlags & GPE_PREFER_VIDEO_MEMORY) && (format == m_pMode->format)))
        || (surfaceFlags & GPE_PREFER_VIDEO_MEMORY) && (format == m_pMode->format))
    {
        SCODE rv = AllocSurfaceVideo(ppSurf, width, height, stride, format, pixelFormat);
        if (rv == S_OK)
        {
            return S_OK;
        }
        else
        {
            if (surfaceFlags & (GPE_REQUIRE_VIDEO_MEMORY|GPE_BACK_BUFFER))
            {
                RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : AllocSurfaceVideo() failed\r\n"), _T(__FUNCTION__)));
                return E_OUTOFMEMORY;
            }
        }
    }

    //--------------------------------------
    // Try to allocate surface from system memory
    //--------------------------------------

    // stride and surface size for system memory surfaces
    stride = ((bpp * width + 31) >> 5) << 2;
    unsigned int surface_size = stride*height;

    if(m_G2DControlArgs.HWOnOff && m_G2DControlArgs.UsePACSurf)
    {
        if(surface_size > PAC_ALLOCATION_BOUNDARY)
        {
            if(S_OK == AllocSurfacePACS((GPESurf**)ppSurf, width, height, format))
            {
                return S_OK;
            }
        }
        else
        {
            RETAILMSG(DISP_ZONE_WARNING, (_T("[DISPDRV:WARN] %s() : surface size %d < PAC_ALLOCATION_BOUNDARY (%d); HW acceleration disabled.\r\n"),  
                _T(__FUNCTION__), surface_size, PAC_ALLOCATION_BOUNDARY));
        }
    }

    // if allocation is failed or boundary condition is not met, just create DDGPESurf in normal system memory that can be non-linear physically.    
    // Hardware acceleration will be disabled for this surface.
    *ppSurf = new DDGPESurf(width, height, format);

    if (*ppSurf != NULL)
    {
        if (((*ppSurf)->Buffer()) == NULL)
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : OUT OF MEMORY\r\n"), _T(__FUNCTION__)));
            delete *ppSurf;
            return E_OUTOFMEMORY;
        }
        else
        {
            RETAILMSG(DISP_ZONE_WARNING,(_T("[DISPDRV] %s() : Allocated SYSTEM surface in discontiguous memory.\r\n"), _T(__FUNCTION__)));
            RETAILMSG(DISP_ZONE_WARNING,(_T("[DISPDRV] %s() : HW acceleration disabled for this surface.\r\n"), _T(__FUNCTION__)));

            return S_OK;
        }
    }

    return S_OK;
}


SCODE
S3C6410Disp::AllocSurfaceVideo(
                        DDGPESurf **ppSurf,
                        int width,
                        int height,
                        int stride,
                        EGPEFormat format,
                        EDDGPEPixelFormat pixelFormat)
{
    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s( %dx%d stride:%d FMT:%d PFMT: %d )\r\n"), _T(__FUNCTION__), width, height, stride, format, pixelFormat));

    // align frame buffer size with 4-word unit
    DWORD dwSize = ALIGN(stride * height, 16);

    // Try to allocate surface from video memory
    SurfaceHeap *pHeap = m_pVideoMemoryHeap->Alloc(dwSize);
    if (pHeap != NULL)
    {
        DWORD dwVideoMemoryOffset = pHeap->Address() - (DWORD)m_VideoMemoryVirtualBase;
        RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s() : Allocated PA = 0x%08x\r\n"), _T(__FUNCTION__), dwVideoMemoryOffset+m_VideoMemoryPhysicalBase));

        *ppSurf = new S3C6410Surf(width, height, dwVideoMemoryOffset, (PVOID)pHeap->Address(), stride, format, pixelFormat, pHeap);
        if (*ppSurf  == NULL)
        {
            RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : Create S3C6410Surf() Failed\r\n"), _T(__FUNCTION__)));

            pHeap->Free();

            return E_OUTOFMEMORY;
        }

        return S_OK;
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR,(_T("[DISPDRV:ERR] %s() : SurfaceHeap Alloc() Failed\r\n"), _T(__FUNCTION__)));

        *ppSurf = (DDGPESurf *)NULL;

        return E_OUTOFMEMORY;
    }
}


void S3C6410Disp::SetVisibleSurface(GPESurf *pSurf, BOOL bWaitForVBlank)
{
    S3C6410Surf *pDDSurf = (S3C6410Surf *)pSurf;

    if(pDDSurf->IsOverlay() == TRUE)
    {
        m_OverlayCtxt.pPrevSurface = m_OverlayCtxt.pSurface;        // Being Flipped Surface
        m_OverlayCtxt.pSurface = pDDSurf;
    }
    else
    {
        m_pVisibleSurface = pDDSurf;
    }

    EnterCriticalSection(&m_csDevice);

    DevSetVisibleSurface(pDDSurf, bWaitForVBlank);

    LeaveCriticalSection(&m_csDevice);
}

//-----------------------------------------------------------------------------

S3C6410Surf::S3C6410Surf(int width, int height, DWORD offset, VOID *pBits, int stride,
            EGPEFormat format, EDDGPEPixelFormat pixelFormat, SurfaceHeap *pHeap)
            : DDGPESurf(width, height, pBits, stride, format, pixelFormat)
{
    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s( %dx%d stride:%d FMT:%d PFMT: %d )\r\n"), _T(__FUNCTION__), width, height, stride, format, pixelFormat));

    dwSurfaceCount++;

    m_fInVideoMemory = TRUE;
    m_nOffsetInVideoMemory = offset;
    m_pSurfHeap = pHeap;

    if (pixelFormat == ddgpePixelFormat_I420)       // 3Plane
    {
        m_uiOffsetCb = width*height;
        m_uiOffsetCr = m_uiOffsetCb+width*height/4;
    }
    else if (pixelFormat == ddgpePixelFormat_YV12)  // 3Plane
    {
        m_uiOffsetCr = width*height;
        m_uiOffsetCb = m_uiOffsetCr+width*height/4;
    }
    else
    {
        m_uiOffsetCr = 0;
        m_uiOffsetCb = 0;
    }

    RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s() : offset in video memory 0x%08x, total surfaces %d\r\n"), _T(__FUNCTION__), m_nOffsetInVideoMemory, dwSurfaceCount));
}


S3C6410Surf::~S3C6410Surf()
{
    S3C6410Disp    *pDDGPE;
    pDDGPE = (S3C6410Disp *)GetDDGPE();
    if (pDDGPE)
    {        
        // If this surface was being used in a H/W blit, let's make sure it is done
        pDDGPE->CheckAndWaitForHWIdle((GPESurf*) this);
    }
    
    dwSurfaceCount--;
    
    if(m_pSurfHeap)
    {
        RETAILMSG(DISP_ZONE_CREATE,(_T("\r\n[DISPDRV] %s() : Heap 0x%08x Addr:0x%x, Avail:%d, Size:%d\r\n"), 
        _T(__FUNCTION__), m_pSurfHeap, m_pSurfHeap->Address(), m_pSurfHeap->Available(), m_pSurfHeap->Size()));
        
        m_pSurfHeap->Free();
        RETAILMSG(DISP_ZONE_CREATE,(_T("[DISPDRV] %s() : offset in video memory 0x%08x, total surfaces %d\r\n"), _T(__FUNCTION__), m_nOffsetInVideoMemory, dwSurfaceCount));
    }
    else
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("ERROR, Invalid SurfaceHeap Address")));
    }
}



/**
*    @class    PACSurf
*    @desc    This Surface will try to allocate physically linear address
*
**/
/**
*    @fn       PACSurf::PACSurf
*    @brief    try to allocate memory region that is physically linear
*    @param    GPESurf **ppSurf, INT width, INT height, EGPEFormat format, int surfaceFlags
*    @sa       GPESurf
*    @note     This Surface format is compatible to GPESurf
**/
PACSurf::PACSurf(int width, int height, EGPEFormat format, int stride, EDDGPEPixelFormat pixelFormat)
    : DDGPESurf( width, height, NULL, stride, format, pixelFormat )
{
    RETAILMSG(DISP_ZONE_CREATE, (_T("[DISPDRV]  %s(%dx%d, %d, FMT:%d, PFMT:%08x)\r\n"), _T(__FUNCTION__), width, height, stride, format, pixelFormat));    

    // Even though "width" and "height" are int's, they must be positive.
    ASSERT(width > 0);
    ASSERT(height > 0);

    memset( &m_Format, 0, sizeof ( m_Format ) );

    m_pPhysAddr            = NULL;
    m_pVirtAddr            = NULL;
    m_pKernelVirtAddr      = NULL;
    m_hUserProcess         = NULL;
    m_nStrideBytes         = 0;
    m_eFormat              = gpeUndefined;
    m_fInVideoMemory       = 0;
    m_fInUserMemory        = FALSE;
    m_fOwnsBuffer          = 0;
    m_nWidth               = 0;
    m_nHeight              = 0;
    m_nOffsetInVideoMemory = 0;
    m_iRotate              = DMDO_0;
    m_ScreenWidth          = 0;
    m_ScreenHeight         = 0;
    m_BytesPixel           = 0;
    m_nHandle              = NULL;

    if (width > 0 && height > 0)
    {
        m_nWidth            = width;
        m_nHeight           = height;
        m_eFormat           = format;
        m_nStrideBytes      = ( (EGPEFormatToBpp[ format ] * width + 7 )/ 8 + 3 ) & ~3L;
        m_dwSurfaceSize     = m_nStrideBytes * height;

        // try to allocate physically linear address
        m_pKernelVirtAddr   = (ADDRESS) AllocPhysMem( m_dwSurfaceSize, PAGE_READWRITE, 0, 0, &m_pPhysAddr );

        if(m_pKernelVirtAddr != NULL)
        {
            if (GetCurrentProcessId() != GetDirectCallerProcessId())
            {
                // Map it into the caller's process so they can access the pixel data.   
                m_hUserProcess = (HANDLE)GetDirectCallerProcessId();

                // This is safe because m_pVirtAddr is page-aligned and a multiple of pages
                // So we're mapping no more and no less than we intend into the caller's process
                m_pVirtAddr = (ADDRESS)VirtualAllocCopyEx(
                    (HANDLE)GetCurrentProcessId(),
                    m_hUserProcess,
                    (LPVOID)m_pKernelVirtAddr,
                    m_dwSurfaceSize,
                    PAGE_READWRITE);

                if (NULL == m_pVirtAddr)
                {
                    RETAILMSG(DISP_ZONE_ERROR,(TEXT("[DISPDRV:ERR] Mapping PAC Surf to caller process failed: 0x%x\r\n"),
                        GetLastError()));

                    FreePhysMem( (LPVOID)m_pKernelVirtAddr );
                }
            }
            else
            {
                m_pVirtAddr = m_pKernelVirtAddr;
            }

            m_fOwnsBuffer = 1;
            m_BytesPixel  = EGPEFormatToBpp[m_eFormat] >> 3;

            RETAILMSG(DISP_ZONE_CREATE,(TEXT("\n%s(): size : %d, PAC Surf PA Base : 0x%x KVA Base : 0x%x UVA Base : 0x%x STRIDE : %d"), 
                _T(__FUNCTION__), m_dwSurfaceSize, m_pPhysAddr, m_pKernelVirtAddr, m_pVirtAddr, m_nStrideBytes));
        }
        else
        {
            // Ensure the PACSurf object will be cleaned up, and a 2nd allocation from system memory attempted.
            m_pVirtAddr = m_pKernelVirtAddr = m_pPhysAddr = NULL;

            RETAILMSG(DISP_ZONE_WARNING,(TEXT("[DISPDRV:ERR] PAC Surf: AllocPhysMem() failed.  Will alloc from (potentially discontiguous) RAM.")));        
            return;
        }
    }
}

PACSurf::~PACSurf()
{
    S3C6410Disp    *pDDGPE;
    pDDGPE = (S3C6410Disp *)GetDDGPE();
    if (pDDGPE)
    {        
        // If this surface was being used in a H/W blit, let's make sure it is done
        pDDGPE->CheckAndWaitForHWIdle((GPESurf*) this);
    }

    // Unmap the pixel buffer from caller's address space, if it was mapped
    if ( m_hUserProcess     == (HPROCESS)GetDirectCallerProcessId() )
    {
        if (!VirtualFreeEx( m_hUserProcess, (LPVOID)m_pVirtAddr, 0, MEM_RELEASE ))
        {
            RETAILMSG(DISP_ZONE_ERROR,(TEXT("\nPACSurface unmap from caller process failed\r\n")));
            ASSERT( 0 );
        }
        m_pVirtAddr = NULL;
    }

    // Free the physical memory
    if ( !FreePhysMem((LPVOID)m_pKernelVirtAddr) )
    {
        RETAILMSG(DISP_ZONE_ERROR,(TEXT("\nPACSurface deallocation is failed\r\n")));
    }
    else
    {
        RETAILMSG(DISP_ZONE_CREATE,(TEXT("\n%s(): size: %d, VA: 0x%08x, PA: 0x%08x\r\n"), 
            _T(__FUNCTION__), m_dwSurfaceSize, m_pKernelVirtAddr, m_pPhysAddr));
        m_pVirtAddr = m_pKernelVirtAddr = NULL;
        m_fOwnsBuffer = 0;
    }
}


//-----------------------------------------------------------------------------

