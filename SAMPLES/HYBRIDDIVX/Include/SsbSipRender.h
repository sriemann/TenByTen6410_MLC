#ifndef __SAMSUNG_SYSLSI_APDEV_SAMPLE_IMG_RENDER_H__
#define __SAMSUNG_SYSLSI_APDEV_SAMPLE_IMG_RENDER_H__
/*********************************************************************************
*                                                                               *
* Copyright (c) 2008 Samsung System LSI                                            *
* All rights reserved.                                                          *
*                                                                               *
* This software is test sample code for Hybrid Divx Decoder                        *
*                                                                                *
* Author : Jiyoung Shin                                                            *
* Last Changed : 2008.06.10                                                     *
*********************************************************************************/

#include "CMMAPI.h"
#include <windows.h>

#define LCD_X    800
#define LCD_Y    480

#define DISPLAY_X    800
#define DISPLAY_Y    480


#ifdef __cplusplus
extern "C" {    
#endif    /* __cplusplus */

// Codec Mem Test
int SsbSipDisplayInit();
int SsbSipDisplayDeInit();
void SsbSipDisplayEventSet();
void SsbSipDisplayEventCreate();
int SsbSipDisplayThreadCreate();
void SsbSipDisplayThreadClose();
void SsbSipDisplayParamSet(UINT    phy_addr, unsigned int width, unsigned int height, unsigned int buf_width, unsigned int buf_height, unsigned int offset_x, unsigned int offset_y);
static DWORD WINAPI SsbSipDisplayThread();

#ifdef __cplusplus
}        
#endif    /* __cplusplus */

#endif /* __SAMSUNG_SYSLSI_APDEV_SAMPLE_IMG_RENDER_H__ */
