//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#ifndef __SAMSUNG_SYSLSI_APDEV_SAMPLE_NUMBER_IMG_H__
#define __SAMSUNG_SYSLSI_APDEV_SAMPLE_NUMBER_IMG_H__


#include <windows.h>	// Because of HWND


// character image
#define NUM_CHAR_IMG			12

#define CHAR_IMG_WIDTH			15
#define CHAR_IMG_HEIGHT			20

#define CHAR_IMG_WIDTH_HALF			7
#define CHAR_IMG_WIDTH_ONE_HALF		23



// size image
#define NUM_SIZE_IMG			3
#define SIZE_IMG_WIDTH			70
#define SIZE_IMG_HEIGHT			20


typedef enum
{
	SIZE_IMG_QVGA   = 0,
	SIZE_IMG_VGA    = 1,
	SIZE_IMG_SD     = 2,
	SIZE_IMG_UNDEF  = 9
} SIZE_IMG_IDX;



#ifdef __cplusplus
extern "C" {
#endif

BOOL NumImg_MemLoad(unsigned char *mem_ptr);
BOOL NumImg_Write(int number, unsigned char *pImg, int width, int height, int x_pos, int y_pos);
void NumImg_Write_FPS(float fps, unsigned char *pImg, int width, int height, int x_pos, int y_pos);


BOOL SizeImg_MemLoad(unsigned char *mem_ptr);
BOOL SizeImg_Write(SIZE_IMG_IDX size_idx, unsigned char *pImg, int width, int height, int x_pos, int y_pos);

#ifdef __cplusplus
}
#endif






#endif /* __SAMSUNG_SYSLSI_APDEV_SAMPLE_NUMBER_IMG_H__ */
