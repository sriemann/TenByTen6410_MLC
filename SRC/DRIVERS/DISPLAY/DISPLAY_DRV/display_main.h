//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    display_main.h

Abstract:

    This module present class definition of S3C6410Disp, S3C6410Surf, PACSurf 
    S3C6410Disp class is derived from DDGPE
    S3C6410Surf class is derived from DDGPESurf
    PACSurf class is derived from GPESurf

Functions:


Notes:

--*/

#ifndef __DISPLAY_MAIN_H__
#define __DISPLAY_MAIN_H__

class S3C6410Surf;

class FIMGSE2D;

#include "precomp.h"

class S3C6410Disp : public DDGPE
{

protected:
    CRegistryEdit *m_ActiveReg;
    volatile S3C6410_DISPLAY_REG *  m_pDispConReg;    // Display Controller SFR
    volatile S3C6410_GPIO_REG *     m_pGPIOReg;        // GPIO SFR
    volatile S3C6410_SYSCON_REG *      m_pSYSReg;        // SYSCON Controller 

    S3C6410Surf *   m_pVisibleSurface;
    GPEModeEx       m_ModeInfoEx;
    FIMGSE2D *      m_oG2D;
    SURFACE_DESCRIPTOR m_descSrcSurface;            // 2D HW Source Surface Information
    SURFACE_DESCRIPTOR m_descDstSurface;            // 2D HW Destination Surface Information

    G2D_ACCEL_CONTROL_ARGS m_G2DControlArgs;

    /// for Cache Region Clean
    DWORD       m_dwSourceSurfacePA;
    DWORD       m_dwPhyAddrOfSurface[2];  //< [0] is for Source, [1] is for Destination
    HANDLE      m_hVideoDrv;
    DWORD       m_dwPrimaryWinNum;
    DWORD       m_dwPrimaryWinMode;
    DWORD       m_dwDDOverlayWinNum;
    DWORD       m_dwDDOverlayWinMode;
    CRITICAL_SECTION    m_csDevice;                // Critical Section for Display Device Control (FIMD/POST/TVSc/TVEnc)
    CRITICAL_SECTION    m_cs2D;                // Ciritcal Section for 2D

private:

    DWORD       m_dwDeviceScreenWidth;        // LCD Pannel Horizontal Resolution (Pixel Count)
    DWORD       m_dwDeviceScreenHeight;        // LCD Pannel Vertical Resolution (Pixel Count)

    DWORD       m_VideoMemoryPhysicalBase;
    DWORD       m_VideoMemoryVirtualBase;
    DWORD       m_VideoMemorySize;
    SurfaceHeap *m_pVideoMemoryHeap;            // Video Memory Surface Heap

    UCHAR       m_CursorBackingStore[64*64*4];
    UCHAR       m_CursorXorShape[64*64];
    UCHAR       m_CursorAndShape[64*64];
    BOOL        m_CursorDisabled;
    BOOL        m_CursorVisible;
    BOOL        m_CursorForcedOff;
    RECTL       m_CursorRect;
    POINTL      m_CursorSize;
    POINTL      m_CursorHotspot;

    VIDEO_POWER_STATE       m_VideoPowerState;

    OUTPUT_INTERFACE        m_eOutputInterface;
    TV_DMA_MODE             m_eTVDMAMode;
    BOOL                    m_bTVDMARunning;

    OverlayContext          m_OverlayCtxt;
    TVDMAContext            m_TVDMACtxt;

    GPESurf     *m_pLastSrcSurfUsingHW;
    GPESurf     *m_pLastDstSurfUsingHW;

public:

    BOOL        m_InDDraw;
    ULONG_PTR    m_fpCurrentOverlay;
    ULONG_PTR    m_fpPreviousOverlay;

    S3C6410Disp();

    virtual
    ~S3C6410Disp();

    virtual
    int
    NumModes();

    virtual
    SCODE
    SetMode(
        int            modeId,
        HPALETTE    * palette
        );

    virtual
    int
    InDisplay(void);

    virtual
    int
    InVBlank();

    virtual
    SCODE
    SetPalette(
        const PALETTEENTRY *source,
        USHORT        firstEntry,
        USHORT        numEntries
        );

    virtual
    SCODE
    GetModeInfo(
        GPEMode * pMode,
        int       modeNo
        );

    virtual
    SCODE
    GetModeInfoEx(
        GPEModeEx *pModeEx,
        int       modeNo
        );

    virtual
    SCODE
    SetPointerShape(
        GPESurf * mask,
        GPESurf * colorSurface,
        int       xHot,
        int       yHot,
        int       cX,
        int       cY
        );

    virtual
    SCODE
    MovePointer(
        int xPosition,
        int yPosition
        );

    virtual
    void
    WaitForNotBusy();

    virtual
    int
    IsBusy();

    virtual
    bool
    SurfaceBusyBlitting(
        DDGPESurf *pSurf
        );

    virtual
    void
    GetPhysicalVideoMemory(
        unsigned long * physicalMemoryBase,
        unsigned long * videoMemorySize
        );

    void
    GetVirtualVideoMemory(
        unsigned long * virtualMemoryBase,
        unsigned long * videoMemorySize
        );

    virtual
    SCODE
    Line(
        GPELineParms * lineParameters,
        EGPEPhase      phase
        );

    // blt.cpp
    virtual
    SCODE
        BltPrepare(
        GPEBltParms * blitParameters
        );

    virtual
    SCODE
        BltComplete(
        GPEBltParms * blitParameters
        );

    // For 2D Raster Graphic Acceleration
    void InitAcceleration(void);
    void CheckAndWaitForHWIdle(GPESurf* pSurf);
    virtual SCODE NullBlt(GPEBltParms *pBltParms);
    virtual SCODE AcceleratedSolidFill(GPEBltParms *pParms);
    virtual SCODE AcceleratedPatFill(GPEBltParms *pParms);      // TBD
    virtual SCODE AcceleratedSrcCopyBlt(GPEBltParms *pBltParms);
    virtual SCODE AcceleratedAlphaSrcCopyBlt(GPEBltParms *pBltParms);    
    virtual void MultiplyAlphaBit(DWORD *pdwStartAddress, DWORD dwBufferLength, DWORD AlphaConstant);
    virtual void AcceleratedPerPixelAlpha(GPEBltParms *pBltParms);
    virtual BOOL CreateScratchSurface(GPESurf* OriginalSurface, DDGPESurf** ScratchSurface, PRECTL NewSurfaceSize, SURFACE_DESCRIPTOR *NewSurfaceDescriptor, EGPEFormat NewColorFormat, BOOL bCopy);
    virtual SCODE AcceleratedBltSelect(GPEBltParms *pBltParms);
    virtual SCODE AcceleratedDestInvert(GPEBltParms *pBltParms);
    virtual SCODE AcceleratedSolidLine(GPELineParms *pLineParms);
#if USE_SECEMUL_LIBRARY
    virtual SCODE SECEmulatedBltSelect2416(GPEBltParms *pBltParms);
    virtual SCODE SECEmulatedBltSelect1624(GPEBltParms *pBltParms);
    virtual SCODE SECEmulatedBltSelect16(GPEBltParms *pBltParms);    
#endif
    virtual DWORD ValidatePAContinuityOfSurf(GPESurf *pTargetSurf);
    virtual void FreePhysAddress(DWORD *m_pdwPhysAddress);
    virtual BOOL HWBitBlt(GPEBltParms *pBltParms, PSURFACE_DESCRIPTOR Src, PSURFACE_DESCRIPTOR Dst);
    virtual int GetRelativeDegree(int SrcDegree, int DstDegree);
    virtual void ClipDestDrawRect(GPEBltParms *pBltParms);
    virtual DWORD GetHWColorFormat(GPESurf *pSurf);
    virtual void DumpBltParms(GPEBltParms *pBltParms);

    virtual
    ULONG
    DrvEscape(
        SURFOBJ * pso,
        ULONG     iEsc,
        ULONG     cjIn,
        void    * pvIn,
        ULONG     cjOut,
        void    * pvOut
        );

    int
    GetGameXInfo(
        ULONG   iEsc,
        ULONG   cjIn,
        void  * pvIn,
        ULONG   cjOut,
        void  * pvOut
        );

    int
    GetRawFrameBuffer(
        ULONG   iEsc,
        ULONG   cjIn,
        void  * pvIn,
        ULONG   cjOut,
        void  * pvOut
        );

    SCODE
    WrappedEmulatedLine(
        GPELineParms * lineParameters
        );

    void
    CursorOn();

    void
    CursorOff();

    // surf.cpp
    SCODE
    AllocSurface(
        GPESurf    ** ppSurf,
        int           width,
        int           height,
        EGPEFormat    format,
        int           surfaceFlags
        );

    SCODE
    AllocSurface(
        DDGPESurf         ** ppSurf,
        int                  width,
        int                  height,
        EGPEFormat           format,
        EDDGPEPixelFormat    pixelFormat,
        int                  surfaceFlags
        );

    SCODE
    AllocSurfaceVideo(
        DDGPESurf        ** ppSurf,
        int                width,
        int                height,
        int                stride,
        EGPEFormat           format,
        EDDGPEPixelFormat    pixelFormat
        );
    SCODE
    AllocSurfacePACS(
        GPESurf **ppSurf,
        int width,
        int height,
        EGPEFormat format,
        int stride = 0, 
        EDDGPEPixelFormat pixelFormat = ddgpePixelFormat_UnknownFormat
        );

    virtual
    void
    SetVisibleSurface(
        GPESurf * pSurf,
        BOOL      bWaitForVBlank
        );

    int
    GetRotateModeFromReg();

    void
    SetRotateParams();

    long
    DynRotate(
        int angle
        );

    virtual
    SCODE DetectPixelFormat(
        DWORD                dwCaps,            // in: DDSCAPS_xxx flags
        DDPIXELFORMAT*        pDDPF,            // in: Explicit pixel format or current mode
        EGPEFormat*            pFormat,
        EDDGPEPixelFormat*    pPixelFormat
        );

    //---------------------------
    // Resource Alloc/Release Method
    //---------------------------
    BOOL AllocResource(void);
    void ReleaseResource(void);
    BOOL TVOutAllocResource(void);
    BOOL TVOutReleaseResource(void);

    //------------------------
    // Power Management Handler
    //------------------------
    void SetDisplayPowerState(VIDEO_POWER_STATE PowerState);
    VIDEO_POWER_STATE GetDisplayPowerState(void);

    //----------------------------------------
    // S3C6410 H/W Device Control Wrapper Method
    //----------------------------------------

    // Frame Interrupt / Status
    BOOL WaitForVerticalBlank(VB_STATUS Status);
    DWORD GetScanLine(void);

    // Overlay Control
    BOOL OverlayAllocResource(BOOL bLocalPath);
    BOOL OverlayReleaseResource(BOOL bLocalPath);
    BOOL OverlayInitialize(S3C6410Surf* pOverlaySurface, RECT *pSrc, RECT *pDest);
    void OverlaySetPosition(UINT32 uiOffsetX, UINT32 uiOffsetY);
    void OverlayEnable(void);
    void OverlayDisable(void);
    void OverlayBlendDisable(void);
    void OverlaySetColorKey(BOOL bSrcCKey, EDDGPEPixelFormat Format, DWORD ColorKey);
    void OverlaySetAlpha(BOOL bUsePixelBlend, DWORD Alpha);

    void InitalizeOverlayContext(void);
    S3C6410Surf * GetCurrentOverlaySurf(void);
    S3C6410Surf * GetPreviousOverlaySurf(void);

    //--------------------------------
    // S3C6410 H/W Device Control Method
    //--------------------------------

    // Initialize / Power Management
    BOOL DevInitialize(void);
    BOOL DevPowerOn(void);
    BOOL DevPowerOff(void);

    // Change Interface
    BOOL DevChangeOutputInterface(OUTPUT_INTERFACE eNewOutputIF);
    void DevOutputEnableRGBIF(void);
    void DevOutputDisableRGBIF(void);
    void DevOutputEnableTV(void);
    void DevOutputDisableTV(void);

    BOOL DevSetTVDMAMode(TV_DMA_MODE eType);
    void DevUpdateTVDMAContext(void);
    void DevOutputEnableTVDMA(void);
    void DevOutputDisableTVDMA(void);
    void DevSetTVDMABuffer(unsigned int uiAddrRGBY, unsigned int uiAddrCb, unsigned int uiAddrCr, BOOL bWaitForVBlank);

    // Frame Interrupt / Status
    BOOL DevWaitForVerticalBlank(void);
    int DevGetVerticalStatus(void);
    DWORD DevGetScanLine(void);

    // Change Frame Buffer
    void DevSetVisibleSurface(S3C6410Surf *pSurf, BOOL bWaitForVBlank);

    // Overlay Control
    void DevOverlayInitialize(void);
    void DevOverlaySetPosition(void);
    void DevOverlayEnable(void);
    void DevOverlayDisable(void);
    void DevOverlayBlendDisable(void);
    void DevOverlaySetColorKey(void);
    void DevOverlaySetAlpha(void);
    void DevRecoverOverlay(void);
    void InitializeDisplayMode(void);

    //Enable VSync Interrupt
    BOOL DevEnableVsyncInterrupt(void);
    //To read the Flipstatus whether Flip is completed or in progress.
    BOOL DevGetFlipStatus(void);


friend
    void
    buildDDHALInfo(
        LPDDHALINFO lpddhi,
        DWORD       modeidx
        );
};

class S3C6410Surf : public DDGPESurf
{
private:
    SurfaceHeap*    m_pSurfHeap;

public:
    UINT32    m_uiOffsetCb;
    UINT32    m_uiOffsetCr;

    S3C6410Surf(int, int, DWORD, VOID*, int, EGPEFormat, EDDGPEPixelFormat pixelFormat, SurfaceHeap*);

    virtual
    ~S3C6410Surf();

};

class PACSurf : public DDGPESurf
{
public:
    ADDRESS         m_pPhysAddr;        // Physical address of memory (contiguous and 64KB-aligned)
    ADDRESS         m_pKernelVirtAddr;  // Virtual address of memory, before mapping to caller (m_pVirtAddr)
    HANDLE          m_hUserProcess;

    PACSurf(int width, int height, EGPEFormat format, int stride = 0, EDDGPEPixelFormat pixelFormat = ddgpePixelFormat_UnknownFormat);
    virtual ~PACSurf();
};

#endif __DISPLAY_MAIN_H__
