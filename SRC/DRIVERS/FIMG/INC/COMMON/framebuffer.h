//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include <COMMON/pixelFmts.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef void* GLESContext;

typedef struct {
    void*            paddr;
    void*            vaddr;
    void*           vaddrCP; // for Current Process   
    void*            ChunkHandle;
} AddressBase;

typedef void* BufferHandle;
typedef AddressBase  BufferAddress;

//This structure is deprecated and is left here for backward compatibility and internal use. Use GLES2SurfaceData instead.
typedef struct
{
    AddressBase colorAddr;
    AddressBase depthStencilAddr;

    unsigned int width;
    unsigned int height;
    /*
    int       colorFormat;      //must be set to a gl enum
    int       colorType;
    int       depthStencilFormat;   
    */
    PxFmt      nativeColorFormat;
    PxFmt      nativeDepthStencilFormat;
    int       flipped;  //0 is rendering origin is top left, 1 if its bottom left.
} FramebufferData;



#ifdef __cplusplus
}
#endif

#endif //__FRAMEBUFFER_H__
