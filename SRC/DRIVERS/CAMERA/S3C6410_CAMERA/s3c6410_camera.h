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
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s3c6410_camera.h

Abstract:       module interface

Functions:


Notes:


--*/

#ifndef _SMDK6410CAMERA_H
#define _SMDK6410CAMERA_H


#define    VIDEO_CAPTURE_BUFFER 0x1
#define    STILL_CAPTURE_BUFFER 0x2
#define    PREVIEW_CAPTURE_BUFFER 0x3

#define OUTPUT_CODEC_YCBCR422    0x1
#define OUTPUT_CODEC_YCBCR420    0x2
#define OUTPUT_CODEC_RGB16        0x3
#define OUTPUT_CODEC_RGB24        0x4

#define PREVIEW_PATH        0x1
#define CODEC_PATH            0x2

#define MAX_HW_FRAMES     4

#define CAPTURE_BUFFER_SIZE        1966080        //  ( 1280*1024*3/2 )
#define PREVIEW_BUFFER_SIZE        614400        //  ( 320*240*2  )  * MAX_HW_FRAMES

typedef struct
{
        DWORD VirtAddr;         
        int   size;             // size of the buffer
        int   BufferID;         // a buffer ID used to identify this buffer to the driver
        union{
            DWORD *pY;
            DWORD *pRGB;
        };
        DWORD *pCb;
        DWORD *pCr;
//        DWORD *pBuf;            // Address of the DMA buffer returned from a call to malloc().
} CAMERA_DMA_BUFFER_INFO, *P_CAMERA_DMA_BUFFER_INFO;

typedef void (*PFNCAMHANDLEFRAME)( DWORD dwContext );

int     CameraInit(void *pData);
void     CameraDeinit();
int     CameraPrepareBuffer(P_CAMERA_DMA_BUFFER_INFO pBufInfo, int BufferType);
int     CameraSetFormat(UINT32 width, UINT32 height, int Format, int BufferType);
int     CameraDeallocateBuffer(int BufferType);
int        CameraGetCurrentFrameNum(int BufferType);
int     CameraCaptureControl(int Format, BOOL on); 
int     CameraSetRegisters(int Format); 
void    CameraClockOn(BOOL bOnOff);    // Clock on/off
void    CameraSleep();
void    CameraResume();
int        CameraZoom(int value);
#endif