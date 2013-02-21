#include <windows.h>
#include <ddraw.h>
#include "resource.h"

#include "mfc_render.h"

#include "FrameExtractor.h"



#define APPNAME                TEXT("ddrawtest")
#define APPTITLE            TEXT("DDRAWTEST")

LPDIRECTDRAW          g_pDD;
LPDIRECTDRAWSURFACE   g_pDDSPrimary = NULL;        // Primary Surface.


static DDPIXELFORMAT ddpfOverlayFormats[] = {
    {sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y', 'V', '1', '2'), 0, 0, 0, 0, 0},
    {sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','U','Y','V'),0,0,0,0,0},  // YUYV
    {sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('U','Y','V','Y'),0,0,0,0,0},  // UYVY
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16,  0x7C00, 0x03e0, 0x001F, 0},         // 16-bit RGB 5:5:5
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16,  0xF800, 0x07e0, 0x001F, 0},         // 16-bit RGB 5:6:5
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0}
};



typedef struct tagMFC_RENDER_OVERLAY
{
    LPDIRECTDRAWSURFACE   pDD;
    LPDIRECTDRAWSURFACE   pDD_back;
    int   which;
    int   show;

    int   x, y;
    int   src_wd, src_hi;
    int   dst_wd, dst_hi;

    int   surface_type;
} _MFC_RENDER_OVERLAY;



extern "C" _initConvTab();
extern "C"
void _yuv420ToRgb565(unsigned char *p_lum, unsigned char *p_cb, unsigned char *p_cr, int w_src, int h_src,
                     unsigned char *dest,  int w_dst, int h_dst,
                    int topdown);



static HRESULT WINAPI 
EnumSurfacesCallback(LPDIRECTDRAWSURFACE lpDDSurface,  
                     LPDDSURFACEDESC lpDDSurfaceDesc,  
                     LPVOID lpContext)
{
    HRESULT               hr;
    LPDIRECTDRAWSURFACE  *ppDDSurface;

    ppDDSurface = (LPDIRECTDRAWSURFACE *) lpContext;
    hr = (HRESULT)DDENUMRET_OK;

    *ppDDSurface = lpDDSurface;

    RETAILMSG(1, (L"\nEnumSurface Callback function."));

//    lpDDSurface->Release();
    return hr;
}


//-----------------------------------------------------------------------------
// Name: ReleaseAllObjects()
// Desc: Finished with all objects we use; release them
//-----------------------------------------------------------------------------
static void ReleaseAllObjects(void)
{
    if (g_pDDSPrimary != NULL)
    {
        g_pDDSPrimary->Release();
        g_pDDSPrimary = NULL;
    }

    if (g_pDD != NULL)
    {
        g_pDD->Release();
        g_pDD = NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: InitFail()
// Desc: This function is called if an initialization function fails
//-----------------------------------------------------------------------------
#define PREFIX      TEXT("\n{MFC DDRAW TEST}: ")
#define PREFIX_LEN  12

BOOL PRINT_ERRMSG(LPCTSTR szError,...)
{
    static TCHAR   szBuff[128] = PREFIX;
    va_list        vl;
    
    va_start(vl, szError);
    StringCchVPrintf(szBuff + PREFIX_LEN, (128-PREFIX_LEN), szError, vl);
    OutputDebugString(szBuff);
    va_end(vl);

    return FALSE;
}





BOOL mfc_render_init(HWND hWnd)
{
    DDSURFACEDESC    ddsd;
    DDCAPS           ddcaps;

    HRESULT          hRet;

    ///////////////////////////////////////////////////////////////////////////
    // Create the main DirectDraw object
    ///////////////////////////////////////////////////////////////////////////
    hRet = DirectDrawCreate(NULL, &g_pDD, NULL);
    if (hRet != DD_OK)
        return PRINT_ERRMSG(TEXT("DirectDrawCreate FAILED"));

    // Get exclusive mode
    hRet = g_pDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
    if (hRet != DD_OK)
        return PRINT_ERRMSG(TEXT("SetCooperativeLevel FAILED"));


    // Get a primary surface interface pointer (only needed for init.)

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hRet = g_pDD->CreateSurface(&ddsd, &g_pDDSPrimary, NULL);
    if (hRet != DD_OK)
        return PRINT_ERRMSG(TEXT("CreateSurface FAILED"));

    // See if we can support overlays.
    memset(&ddcaps, 0, sizeof(ddcaps));
    ddcaps.dwSize = sizeof(ddcaps);
    hRet = g_pDD->GetCaps(&ddcaps, NULL);
    if (hRet != DD_OK)
        return PRINT_ERRMSG(TEXT("GetCaps FAILED"));

    if (ddcaps.dwOverlayCaps == 0)
        return PRINT_ERRMSG(TEXT("Overlays are not supported in hardware!"));


    // SW YUV->RGB conversion table initialization
    _initConvTab();

    return TRUE;
}



void mfc_render_final()
{
    ReleaseAllObjects();
}


void *mfc_render_create_overlay(int surface_type,
                                int x, int y,
                                int src_wd, int src_hi,
                                int dst_wd, int dst_hi)
{
    _MFC_RENDER_OVERLAY   *pMFC_RENDER_OVERLAY;
    LPDIRECTDRAWSURFACE    pDDSOverlay = NULL;    // Overlay Surface.
    DDSURFACEDESC          ddsd;

    HRESULT                hRet;


    switch (surface_type) {
    case MFC_RENDER_SURFACE_TYPE_YV12:
    case MFC_RENDER_SURFACE_TYPE_RGB565:
        break;

    default:
        PRINT_ERRMSG(TEXT("surface_type is not supported."));
        return NULL;
    }


    pMFC_RENDER_OVERLAY = (_MFC_RENDER_OVERLAY *) malloc(sizeof(_MFC_RENDER_OVERLAY));
    if (pMFC_RENDER_OVERLAY == NULL) {
        PRINT_ERRMSG(TEXT("malloc failed."));
        return NULL;
    }

    // Create the overlay flipping surface. We will attempt the pixel formats
    // in our table one at a time until we find one that jives.

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize            = sizeof(ddsd);
    ddsd.ddsCaps.dwCaps    = DDSCAPS_OVERLAY | DDSCAPS_FLIP;
    ddsd.dwFlags           = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_BACKBUFFERCOUNT | DDSD_PIXELFORMAT;
    ddsd.dwWidth           = src_wd;
    ddsd.dwHeight          = src_hi;
    ddsd.dwBackBufferCount = 1;
    ddsd.ddpfPixelFormat   = ddpfOverlayFormats[surface_type];

    hRet = g_pDD->CreateSurface(&ddsd, &pDDSOverlay, NULL);
    if (hRet != DD_OK) {
        PRINT_ERRMSG(TEXT("Unable to create overlay surface!"));
        free(pMFC_RENDER_OVERLAY);
        return NULL;
    }

RETAILMSG(1,(L"\n\t  OVERLAY SURFACE TYPE %d\n", surface_type));

    pMFC_RENDER_OVERLAY->pDD       = pDDSOverlay;
    pMFC_RENDER_OVERLAY->pDD_back  = NULL;    // 아래 EnumAttachedSurface을 통하여 세팅됨
    pMFC_RENDER_OVERLAY->which     = 0;
    pMFC_RENDER_OVERLAY->show      = 0;

    pMFC_RENDER_OVERLAY->x             = x;
    pMFC_RENDER_OVERLAY->y             = y;
    pMFC_RENDER_OVERLAY->src_wd        = src_wd;
    pMFC_RENDER_OVERLAY->src_hi        = src_hi;
    pMFC_RENDER_OVERLAY->dst_wd        = dst_wd;
    pMFC_RENDER_OVERLAY->dst_hi        = dst_hi;

    pMFC_RENDER_OVERLAY->surface_type  = surface_type;


    hRet = pDDSOverlay->EnumAttachedSurfaces((LPVOID)(&pMFC_RENDER_OVERLAY->pDD_back), EnumSurfacesCallback);
    if (hRet != DD_OK) {
        PRINT_ERRMSG(TEXT("Unable to EnumAttachedSurfaces!"));
        pDDSOverlay->Release();
        free(pMFC_RENDER_OVERLAY);
        return NULL;
    }


    return (void *) pMFC_RENDER_OVERLAY;
}


void mfc_render_do(void *mfc_render_handle, unsigned char *pImg, int width, int height, int img_type)
{
    _MFC_RENDER_OVERLAY   *pMFC_RENDER_OVERLAY;

    DDSURFACEDESC          ddsd;
    HRESULT                hRet;

    LPDIRECTDRAWSURFACE    pDDSOverlay;    // Overlay Surface.

    unsigned char *pSurf;
    int            y_size, u_size;


    switch (img_type) {
    case MFC_RENDER_IMAGE_TYPE_YUV420:
    case MFC_RENDER_IMAGE_TYPE_RGB565:
    case MFC_RENDER_IMAGE_TYPE_YV12:
        break;

    default:
        PRINT_ERRMSG(TEXT("img_type is not supported."));
        return;
    }


    if (mfc_render_handle == NULL) {
        PRINT_ERRMSG(TEXT("\nmfc_render_handle is NULL."));
        return;
    }
    if (pImg == NULL) {
        PRINT_ERRMSG(TEXT("Input image is NULL."));
        return;
    }

    pMFC_RENDER_OVERLAY = (_MFC_RENDER_OVERLAY *) mfc_render_handle;


    if (pMFC_RENDER_OVERLAY->which == 1) {
        pDDSOverlay = pMFC_RENDER_OVERLAY->pDD;
    }
    else if (pMFC_RENDER_OVERLAY->which == 0) {
        pDDSOverlay = pMFC_RENDER_OVERLAY->pDD_back;
    }


    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    // Lock down the surface so we can modify it's contents.
    hRet = pDDSOverlay->Lock( NULL, &ddsd, DDLOCK_WAITNOTBUSY, NULL);
    if (hRet != DD_OK) {
        PRINT_ERRMSG(TEXT("Unable to show overlay surface!"));
        return;
    }

    pSurf = (LPBYTE)ddsd.lpSurface;

    y_size = width * height;
    u_size = y_size >> 2;


    switch (pMFC_RENDER_OVERLAY->surface_type) {
    case MFC_RENDER_SURFACE_TYPE_YV12:    // YV12의 경우는 그대로 복사한다.
        if (img_type == MFC_RENDER_IMAGE_TYPE_YUV420) {
            memcpy(pSurf, pImg, y_size);
            memcpy(pSurf + y_size, pImg + y_size + u_size, u_size);
            memcpy(pSurf + y_size + u_size, pImg + y_size, u_size);
        }
        else if (img_type == MFC_RENDER_IMAGE_TYPE_YV12) {
            memcpy(pSurf, pImg, y_size + (u_size << 1));
        }
        else if (img_type == MFC_RENDER_IMAGE_TYPE_RGB565) {
            PRINT_ERRMSG(TEXT("This case is not supported. : (Surface type is YV12 and Image type is RGB565)"));
        }
        break;

    case MFC_RENDER_SURFACE_TYPE_RGB565:    // RGB565의 경우는 SW YUV->RGB 변환 수행
        if (img_type == MFC_RENDER_IMAGE_TYPE_YUV420) {
            _yuv420ToRgb565(pImg, pImg + y_size, pImg + y_size + u_size, width, height,
                            pSurf, width, height,
                            0);
        }
        else if (img_type == MFC_RENDER_IMAGE_TYPE_YV12) {
            _yuv420ToRgb565(pImg, pImg + y_size + u_size, pImg + y_size, width, height,
                            pSurf, width, height,
                            0);
        }
        else if (img_type == MFC_RENDER_IMAGE_TYPE_RGB565) {
            memcpy(pSurf, pImg, y_size << 1);
        }
        break;

    default:
        break;
    }

    pDDSOverlay->Unlock(NULL);

}

void mfc_render_flip(void *mfc_render_handle)
{
    HRESULT                hRet;

    _MFC_RENDER_OVERLAY   *pMFC_RENDER_OVERLAY;
    LPDIRECTDRAWSURFACE    pDDSOverlay;

    if (mfc_render_handle == NULL) {
        PRINT_ERRMSG(TEXT("\nmfc_render_handle is NULL."));
        return;
    }

    pMFC_RENDER_OVERLAY = (_MFC_RENDER_OVERLAY *) mfc_render_handle;


    if (pMFC_RENDER_OVERLAY->show == 0) {
        RECT  rs, rd;

        rs.left   = 0; 
        rs.top    = 0;
        rs.right  = pMFC_RENDER_OVERLAY->src_wd;
        rs.bottom = pMFC_RENDER_OVERLAY->src_hi;
        rd.left   = pMFC_RENDER_OVERLAY->x; 
        rd.top    = pMFC_RENDER_OVERLAY->y;
        rd.right  = pMFC_RENDER_OVERLAY->dst_wd;
        rd.bottom = pMFC_RENDER_OVERLAY->dst_hi;


        pDDSOverlay = pMFC_RENDER_OVERLAY->pDD;

        // Update the overlay parameters.
        hRet = pDDSOverlay->UpdateOverlay(&rs, g_pDDSPrimary, &rd, DDOVER_SHOW, NULL);
        if (hRet != DD_OK) {
            PRINT_ERRMSG(TEXT("Unable to show overlay surface!"));
            return;
        }
    
    
        hRet = pDDSOverlay->SetOverlayPosition(0, 0);
        if (hRet != DD_OK) {
            PRINT_ERRMSG(TEXT("Unable to SetOverlayPosition!"));
            return;
        }

        pMFC_RENDER_OVERLAY->show = 1;
    }

    if (pMFC_RENDER_OVERLAY->which == 0) {
        pMFC_RENDER_OVERLAY->pDD->Flip(NULL, 0);
        pMFC_RENDER_OVERLAY->which = 1;
    }
    else if (pMFC_RENDER_OVERLAY->which == 1) {
        pMFC_RENDER_OVERLAY->pDD_back->Flip(NULL, 0);
        pMFC_RENDER_OVERLAY->which = 0;
    }
}


void mfc_render_delete_overlay(void *mfc_render_handle)
{
    _MFC_RENDER_OVERLAY   *pMFC_RENDER_OVERLAY;
    LPDIRECTDRAWSURFACE    pDDSOverlay;


    if (mfc_render_handle == NULL) {
        PRINT_ERRMSG(TEXT("\nmfc_render_handle is NULL."));
        return;
    }

    pMFC_RENDER_OVERLAY = (_MFC_RENDER_OVERLAY *) mfc_render_handle;

    pDDSOverlay = pMFC_RENDER_OVERLAY->pDD;
    if (pDDSOverlay != NULL) {
        // Use UpdateOverlay() with the DDOVER_HIDE flag to remove an overlay 
        // from the display.
        pDDSOverlay->UpdateOverlay(NULL, g_pDDSPrimary, NULL, DDOVER_HIDE, NULL);
        pDDSOverlay->Release();
        pDDSOverlay = NULL;
    }

    pDDSOverlay = (LPDIRECTDRAWSURFACE) pMFC_RENDER_OVERLAY->pDD_back;
    if (pDDSOverlay != NULL) {
        // Use UpdateOverlay() with the DDOVER_HIDE flag to remove an overlay 
        // from the display.
        pDDSOverlay->UpdateOverlay(NULL, g_pDDSPrimary, NULL, DDOVER_HIDE, NULL);
        pDDSOverlay->Release();
        pDDSOverlay = NULL;
    }
}
