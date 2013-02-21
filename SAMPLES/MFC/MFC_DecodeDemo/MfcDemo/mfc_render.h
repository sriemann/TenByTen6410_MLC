#ifndef __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_RENDER_H__
#define __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_RENDER_H__



#define MFC_RENDER_SURFACE_TYPE_YV12        (0)
#define MFC_RENDER_SURFACE_TYPE_RGB565        (4)


#define MFC_RENDER_IMAGE_TYPE_YV12            (0)
#define MFC_RENDER_IMAGE_TYPE_YUV420        (9)
#define MFC_RENDER_IMAGE_TYPE_RGB565        (4)


#include <windows.h>    // Because of HWND

BOOL mfc_render_init(HWND hWnd);
void mfc_render_final();

void *mfc_render_create_overlay(int surface_type,
                                int x, int y,
                                int src_wd, int src_hi,
                                int dst_wd, int dst_hi);
void mfc_render_do(void *mfc_render_handle, unsigned char *pImg, int width, int height, int img_type);
void mfc_render_flip(void *mfc_render_handle);
void mfc_render_delete_overlay(void *mfc_render_handle);

#endif /* __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_RENDER_H__ */
