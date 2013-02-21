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

    halcaps.cpp

Abstract:

    The implementation HAL capability properties to support DirectDraw

Functions:

    buildDDHALInfo, ...

Notes:

--*/

#include "precomp.h"

//----------------------------------
// callbacks from the DIRECTDRAW object
//----------------------------------
DDHAL_DDCALLBACKS cbDDCallbacks =
{
    sizeof( DDHAL_DDCALLBACKS ),        // dwSize
    DDHAL_CB32_CREATESURFACE |
    DDHAL_CB32_WAITFORVERTICALBLANK |
    DDHAL_CB32_CANCREATESURFACE |
    //DDHAL_CB32_CREATEPALETTE |
    DDHAL_CB32_GETSCANLINE |
    0,
    HalCreateSurface,        // CreateSurface
    HalWaitForVerticalBlank,    // WaitForVerticalBlank
    HalCanCreateSurface,    // CanCreateSurface
    NULL,                    // CreatePalette          // WinCE6.0 WM6.0 Unsupport. Must be set to NULL
    HalGetScanLine            // GetScanLine
};

//------------------------------------------
// callbacks from the DIRECTDRAWSURFACE object
//------------------------------------------
DDHAL_DDSURFACECALLBACKS cbDDSurfaceCallbacks =
{
    sizeof( DDHAL_DDSURFACECALLBACKS ),    // dwSize
    DDHAL_SURFCB32_DESTROYSURFACE |    // dwFlags
    DDHAL_SURFCB32_FLIP |
    DDHAL_SURFCB32_LOCK |
    DDHAL_SURFCB32_UNLOCK |
    DDHAL_SURFCB32_SETCOLORKEY |
    DDHAL_SURFCB32_GETBLTSTATUS |
    DDHAL_SURFCB32_GETFLIPSTATUS |
    DDHAL_SURFCB32_UPDATEOVERLAY |
    DDHAL_SURFCB32_SETOVERLAYPOSITION |
    //DDHAL_SURFCB32_SETPALETTE |
    0,
    DDGPEDestroySurface,            // DestroySurface
    HalFlip,                        // Flip
    HalLock,                        // Lock
    DDGPEUnlock,                    // Unlock
    HalSetColorKey,                 // SetColorKey
    HalGetBltStatus,                // GetBltStatus
    HalGetFlipStatus,               // GetFlipStatus
    HalUpdateOverlay,               // UpdateOverlay
    HalSetOverlayPosition,          // SetOverlayPosition
    NULL                            // SetPalette              // WinCE6.0 WM6.0 Unsupport. Must be set to NULL
};

//------------------------------------------------
// callbacks from the DIRECTDRAWMISCELLANEOUS object
//------------------------------------------------
DDHAL_DDMISCELLANEOUSCALLBACKS MiscellaneousCallbacks =
{
    sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS),    // dwSize
    //DDHAL_MISCCB32_GETAVAILDRIVERMEMORY |    // dwFlags
    //DDHAL_MISCCB32_GETDEVICEIDENTIFIER |
    0,
    NULL,        // GetAvailDriverMemory
    NULL        // GetDeviceIdentifier
};

//-------------------------------------------------------
// callbacks from the DIRECTDRAWCOLORCONTROL pseudo object
//-------------------------------------------------------
DDHAL_DDCOLORCONTROLCALLBACKS ColorControlCallbacks =
{
    sizeof(DDHAL_DDCOLORCONTROLCALLBACKS),        // dwSize
    //DDHAL_COLORCB32_COLORCONTROL |            // dwFlags
    //DDHAL_COLORCB32_GAMMACONTROL |
    0,
    NULL,                                        // ColorControl
    NULL                                        // GammaControl
};

// This global pointer is to be recorded in the DirectDraw structure
// Initialized by HalInit()
DDGPE* g_pGPE = (DDGPE*)NULL;
DDGPESurf* g_pDDrawPrimarySurface = NULL;

/* 
DWORD lpdwFourCC[] = {
    FOURCC_I420,    // YUV420
    FOURCC_YV12,    // YVU420
    FOURCC_YUYV,    // 422 (YCbYCr)
    FOURCC_YUY2,    // 422 (YCbYCr)
    FOURCC_UYVY,    // 422 (CbYCrY)
    FOURCC_YVYU,    // 422 (YCrYCb)
    FOURCC_VYUY    // 422 (CrYCbY)
};
#define MAX_FOURCC        (sizeof(lpdwFourCC)/sizeof(DWORD))
*/

FourCCDescription SupportedFourCCs[] = 
{
    //   CODE          BPP  YBITMASK    UBITMASK    VBITMASK
    // ======================================================    
    { FOURCC_I420,      12, (DWORD) -1, (DWORD) -1, (DWORD) -1 },
    { FOURCC_YV12,      12, (DWORD) -1, (DWORD) -1, (DWORD) -1 },
    { FOURCC_YUYV,      16, 0x00FF00FF, 0x0000FF00, 0xFF000000 },
    { FOURCC_YUY2,      16, 0x00FF00FF, 0x0000FF00, 0xFF000000 },
    { FOURCC_UYVY,      16, 0xFF00FF00, 0x000000FF, 0x00FF0000 },
    { FOURCC_YVYU,      16, 0x00FF00FF, 0xFF000000, 0x0000FF00 },
    { FOURCC_VYUY,      16, 0xFF00FF00, 0x00FF0000, 0x000000FF },
};

#define MAX_FOURCC  sizeof(SupportedFourCCs)/sizeof(SupportedFourCCs[0])

// InitDDHALInfo must set up this information
unsigned char    *g_pVideoMemory        = NULL;        // virtual address of video memory from client's side
unsigned long     g_nVideoMemorySize     = 0;

EXTERN_C void buildDDHALInfo(LPDDHALINFO lpddhi, DWORD modeidx)
{
    RETAILMSG(DISP_ZONE_ENTER, (_T("[DDHAL] ++buildDDHALInfo()\r\n")));

    S3C6410Disp *pS3C6410Disp = ((S3C6410Disp *)GetDDGPE());

    if(g_pVideoMemory == NULL)    // to avoid confilct in case of called more than once...
    {
        unsigned long VideoMemoryStart;
        unsigned long VideoMemorySize;

        pS3C6410Disp->GetVirtualVideoMemory(&VideoMemoryStart, &VideoMemorySize);
        g_pVideoMemory = (unsigned char *)VideoMemoryStart;
        g_nVideoMemorySize = (DWORD)VideoMemorySize;
        RETAILMSG(DISP_ZONE_INIT, (_T("[DDHAL:INF] buildDDHALInfo() :  VideoMemory = 0x%08x\r\n"), g_pVideoMemory));
        RETAILMSG(DISP_ZONE_INIT, (_T("[DDHAL:INF] buildDDHALInfo() :  VideoMemorySize = 0x%08x\r\n"), g_nVideoMemorySize));
    }

    // Clear the DDHALINFO structure
    memset(lpddhi, 0, sizeof(DDHALINFO));

    //----------------------
    // Fill DDHALINFO Structure
    //----------------------
    lpddhi->dwSize = sizeof(DDHALINFO);
//    lpddhi->dwFlags = 0;                        // Reserved. Set to 0 for future compatibility.
    lpddhi->dwFlags = DDHALINFO_FOURCCINFORMATION;

    // Callbacks
    lpddhi->lpDDCallbacks = &cbDDCallbacks;
    lpddhi->lpDDSurfaceCallbacks = &cbDDSurfaceCallbacks;
    lpddhi->lpDDPaletteCallbacks = NULL;        // WinCE6.0 WM6.0 Unsupport. Must be set to NULL
    lpddhi->GetDriverInfo = HalGetDriverInfo;

//    lpddhi->lpdwFourCC = lpdwFourCC;            // fourcc codes supported
    lpddhi->lpdwFourCC = (DWORD *)SupportedFourCCs;

    //--------------------------------------------------
    // Capabilities that are supported in the display hardware.
    //--------------------------------------------------

    lpddhi->ddCaps.dwSize = sizeof(DDCAPS);    // size of the DDDRIVERCAPS structure

    // Surface capabilities
    lpddhi->ddCaps.dwVidMemTotal = g_nVideoMemorySize;    // total amount of video memory
    lpddhi->ddCaps.dwVidMemFree = g_nVideoMemorySize;    // amount of free video memory
    lpddhi->ddCaps.dwVidMemStride = 0;                    // This value is 0 if the stride is linear.

    // Capabilities of the surface.
    lpddhi->ddCaps.ddsCaps.dwCaps =
        //DDSCAPS_ALPHA |                // Indicates that this surface contains alpha-only information.
        DDSCAPS_BACKBUFFER |            // Indicates that this surface is the back buffer of a surface flipping structure.
        // DDSCAPS_DYNAMIC |            // Unsupported.
        DDSCAPS_FLIP |                    // Indicates that this surface is a part of a surface flipping structure.
        DDSCAPS_FRONTBUFFER |            // Indicates that this surface is the front buffer of a surface flipping structure.
        //DDSCAPS_NOTUSERLOCKABLE |    // Unsupported.
        DDSCAPS_OVERLAY |                // Indicates that this surface is an overlay.
        //DDSCAPS_PALETTE |                // Not supported.
        DDSCAPS_PRIMARYSURFACE |        // Indicates the surface is the primary surface.
        //DDSCAPS_READONLY |            // Indicates that only read access is permitted to the surface. When locking the surface with IDirectDrawSurface::Lock, the DDLOCK_READONLY flag must be specified.
        DDSCAPS_SYSTEMMEMORY |        // Indicates that this surface memory was allocated in system memory.
        DDSCAPS_VIDEOMEMORY |            // Indicates that this surface exists in display memory.
             DDSCAPS_OWNDC |                // Surfaces that own their own DCs, CE6.0QFE080121_KB946657
        //DDSCAPS_WRITEONLY |            // Indicates that only write access is permitted to the surface.
        0;

    lpddhi->ddCaps.dwNumFourCCCodes = MAX_FOURCC;    // number of four cc codes

    // Palette capabilities.
    lpddhi->ddCaps.dwPalCaps =
        //DDPCAPS_ALPHA |                // Supports palettes that include an alpha component. For alpha-capable palettes, the peFlags member of for each PALETTEENTRY structure the palette contains is to be interpreted as a single 8-bit alpha value (in addition to the color data in the peRed, peGreen, and peBlue members). A palette created with this flag can only be attached to a texture surface.
        //DDPCAPS_PRIMARYSURFACE |        // Indicates that the palette is attached to the primary surface. Changing the palette has an immediate effect on the display unless the DDPCAPS_VSYNC capability is specified and supported.
        0;

    // Hardware blitting capabilities
    //

    // Driver specific blitting capabilities.
    lpddhi->ddCaps.dwBltCaps =
        DDBLTCAPS_READSYSMEM |        // Supports blitting from system memory.
        DDBLTCAPS_WRITESYSMEM |        // Supports blitting to system memory.
        //DDBLTCAPS_FOURCCTORGB |        // Supports blitting from a surface with a FOURCC pixel format to a surface with an RGB pixel format.
        //DDBLTCAPS_COPYFOURCC |        // Supports blitting from a surface with a FOURCC pixel format to another surface with the same pixel format, or to the same surface. DDBLTCAPS_FILLFOURCC Supports color-fill blitting to a surface with a FOURCC pixel format.
        //DDBLTCAPS_FILLFOURCC |        // Supports color-fill blitting to a surface with a FOURCC pixel format.
        0;

    // Color key capabilities
    lpddhi->ddCaps.dwCKeyCaps =
        //DDCKEYCAPS_BOTHBLT |                    // Supports transparent blitting with for both source and destination surfaces.
        //DDCKEYCAPS_DESTBLT |                    // Supports transparent blitting with a color key that identifies the replaceable bits of the destination surface for RGB colors.
        //DDCKEYCAPS_DESTBLTCLRSPACE |        // Supports transparent blitting with a color space that identifies the replaceable bits of the destination surface for RGB colors.
        //DDCKEYCAPS_DESTBLTCLRSPACEYUv |        // Supports transparent blitting with a color space that identifies the replaceable bits of the destination surface for YUV colors.
        //DDCKEYCAPS_SRCBLT |                    // Supports transparent blitting using the color key for the source with this surface for RGB colors.
        //DDCKEYCAPS_SRCBLTCLRSPACE |            // Supports transparent blitting using a color space for the source with this surface for RGB colors.
        //DDCKEYCAPS_SRCBLTCLRSPACEYUV |        // Supports transparent blitting using a color space for the source with this surface for YUV colors.
         0;

    // Alpha blitting capabilities.
    lpddhi->ddCaps.dwAlphaCaps =
        DDALPHACAPS_ALPHAPIXELS |    // Supports per-pixel alpha values specified alongside with the RGB values in the pixel structure.
        //DDALPHACAPS_ALPHASURFACE |    // Unsupported.
        //DDALPHACAPS_ALPHAPALETTE |    // Unsupported.
        DDALPHACAPS_ALPHACONSTANT |
        //DDALPHACAPS_ARGBSCALE |        // Unsupported.
        //DDALPHACAPS_SATURATE |        // Unsupported.
        //DDALPHACAPS_PREMULT |        // Supports pixel formats with premultiplied alpha values.
        //DDALPHACAPS_NONPREMULT |    // Supports pixel formats with non-premultiplied alpha values.
        //DDALPHACAPS_ALPHAFILL |        // Supports color-fill blitting using an alpha value.
        //DDALPHACAPS_ALPHANEG |        // Supports inverted-alpha pixel formats, where 0 indicates fully opaque and 255 indicates fully transparent.
        0;

    SETROPBIT(lpddhi->ddCaps.dwRops, SRCCOPY);                   // Set bits for ROPS supported
    SETROPBIT(lpddhi->ddCaps.dwRops, PATCOPY);
    SETROPBIT(lpddhi->ddCaps.dwRops, BLACKNESS);
    SETROPBIT(lpddhi->ddCaps.dwRops, WHITENESS);

    // General overlay capabilities.
    lpddhi->ddCaps.dwOverlayCaps =
        DDOVERLAYCAPS_FLIP |                    // Supports surface flipping with overlays.
        DDOVERLAYCAPS_FOURCC |                // Supports FOURCC pixel formats with overlays. Use IDirectDraw::GetFourCCCodes to determine which FOURCC formats are supported.
        //DDOVERLAYCAPS_ZORDER |                // Supports changing Z order of overlays.
        //DDOVERLAYCAPS_MIRRORLEFTRIGHT |        // Supports surface mirroring in the left-to-right direction for overlays.
        //DDOVERLAYCAPS_MIRRORUPDOWN |        // Supports surface mirroring in the up-to-down direction for overlays.
        DDOVERLAYCAPS_CKEYSRC |                // Supports source color keying for overlays.
        //DDOVERLAYCAPS_CKEYSRCCLRSPACE |    // Supports source color-space keying for overlays.
        //DDOVERLAYCAPS_CKEYSRCCLRSPACEYUV |    // Supports source color-space keying for overlays with FOURCC pixel formats.
        DDOVERLAYCAPS_CKEYDEST |                // Supports destination color keying for overlays.
        //DDOVERLAYCAPS_CKEYDESTCLRSPACE |    // Supports destination colo-space keying for overlays.
        //DDOVERLAYCAPS_CKEYDESTCLRSPACEYUV |// Supports destination color-space keying for overlays with FOURCC pixel formats.
        //DDOVERLAYCAPS_CKEYBOTH |            // Supports simultaneous source and destination color keying for overlays.
        //DDOVERLAYCAPS_ALPHADEST |            // Supports destination alpha blending for overlays.
        DDOVERLAYCAPS_ALPHASRC |                // Supports source alpha blending for overlays.
        //DDOVERLAYCAPS_ALPHADESTNEG |        // Supports inverted destination alpha blending for overlays.
        //DDOVERLAYCAPS_ALPHASRCNEG |            // Supports inverted source alpha blending for overlays.
        DDOVERLAYCAPS_ALPHACONSTANT |            // Supports constant alpha blending for overlays (specified in the DDOVERLAYFX structure).
        //DDOVERLAYCAPS_ALPHAPREMULT |        // Supports premultiplied alpha pixel formats for overlay alpha blending.
        //DDOVERLAYCAPS_ALPHANONPREMULT |        // Supports non-premultiplied alpha pixel formats for overlay alpha blending.
        //DDOVERLAYCAPS_ALPHAANDKEYDEST |        // Supports simultaneous source alpha blending with a destination color key for overlays.
        DDOVERLAYCAPS_OVERLAYSUPPORT |        // Supports overlay surfaces.
         0;

    lpddhi->ddCaps.dwMaxVisibleOverlays = 1;            // Maximum number of visible overlays or overlay sprites.
    lpddhi->ddCaps.dwCurrVisibleOverlays = 0;            // Current number of visible overlays or overlay sprites.

    lpddhi->ddCaps.dwAlignBoundarySrc = 8;                // Source rectangle alignment, in pixels, for an overlay surface
    lpddhi->ddCaps.dwAlignSizeSrc = 8;                    // Source rectangle size alignment, in pixels, for an overlay surface.
    lpddhi->ddCaps.dwAlignBoundaryDest = 2;            // Destination rectangle alignment, in pixels, for an overlay surface.
    lpddhi->ddCaps.dwAlignSizeDest = 8;                // Destination rectangle size alignment, in pixels, for an overlay surface.

    lpddhi->ddCaps.dwMinOverlayStretch = 250;    // 1  // 1x 1000
    lpddhi->ddCaps.dwMaxOverlayStretch = 9999;    // 1x 1000

    // Miscellaneous video capabilities.
    lpddhi->ddCaps.dwMiscCaps =
        DDMISCCAPS_READSCANLINE |                // Supports reading the current scanline being drawn.
        //DDMISCCAPS_READMONITORFREQ |        // Unsupported.
        DDMISCCAPS_READVBLANKSTATUS |            // Supports reading the current V-Blank status of the hardware.
        //DDMISCCAPS_FLIPINTERVAL |                // Supports interval flipping.
        //DDMISCCAPS_FLIPODDEVEN |                // Supports Even/Odd flipping.
        DDMISCCAPS_FLIPVSYNCWITHVBI |            // Supports V-Sync-coordinated flipping.
        //DDMISCCAPS_COLORCONTROLOVERLAY |    // Supports color controls on overlay surfaces.
        //DDMISCCAPS_COLORCONTROLPRIMARY |    // Supports color controls on primary surfaces.
        //DDMISCCAPS_GAMMACONTROLOVERLAY |    // Supports gamma controls on overlay surfaces.
        //DDMISCCAPS_GAMMACONTROLPRIMARY |    // Supports gamma controls on primary surfaces.
         0;

    // Video port capabilities.
    lpddhi->ddCaps.dwMinVideoStretch = 1000;    // minimum video port stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    lpddhi->ddCaps.dwMaxVideoStretch = 1000;    // maximum video port stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    lpddhi->ddCaps.dwMaxVideoPorts = 0;        // maximum number of usable video ports
    lpddhi->ddCaps.dwCurrVideoPorts = 0;        // current number of video ports used

    RETAILMSG(DISP_ZONE_ENTER,(_T("[DDHAL] --buildDDHALInfo()\r\n")));
}

