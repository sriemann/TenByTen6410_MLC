//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

#ifndef __GLCONTEXT_H__
#define __GLCONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GLES11_BUILD
#include <GLES/gl.h>
#else
#include <GLES2/gl2.h>
#endif


#ifndef GLES_NO_CONTEXT 
#define GLES_NO_CONTEXT 0
#endif

#include <COMMON/framebuffer.h>

typedef struct
{
    AddressBase     colorAddr;
    AddressBase     depthStencilAddr; //Address of depth buffer
    GLuint         width; //Width of Framebuffer surface
    GLuint        height; //Height of Framebuffer surface
    PxFmt         nativeColorFormat; //Pixel format of Framebuffer surface
    PxFmt        nativeDepthStencilFormat; //Depth buffer format
    GLuint        flipped; // 0 is rendering origin is top left, 1 if its bottom left    
}GLESSurfaceData;

//Initializes the 3D HW and the pool memory
//returns    GL_TRUE/GL_FALSE on success/failure
//GLboolean GLES2Initdriver(void);    //depreciated

//Attaches the framebuffer (surface) to the current GL context
GLboolean GLES11SetSurfaceData(GLESSurfaceData * surfdata);
GLboolean GLES2SetSurfaceData(GLESSurfaceData * surfdata);

//Notifies the GL library that a swap buffer is performed. 
//returns  GL_FALSE is no valid surface is bound to the context.
//GLboolean GLES2SwapBuffer(void);    //depreciated

//Creates the GLES 2.0 graphics context
GLESContext GLES11CreateContext(GLESContext sharedctx, GLuint hDriver);
GLESContext GLES2CreateContext(GLESContext sharedctx, GLuint hDriver);

//Sets the context pointed to by ctx as the current context for
//  the current thread
GLESContext GLES11SetContext(GLESContext ctx);
GLESContext GLES2SetContext(GLESContext ctx);

//Destroys the graphics context
GLboolean GLES11DestroyContext(GLESContext ctx);
GLboolean GLES2DestroyContext(GLESContext ctx);

//De initializes the driver. This frees up the pool memory. 
//Should be done when no more GL contexts are active.
//GLboolean GLES2DeInitdriver(void);   //depreciated



//used for defining  externally managed texture. right now only level 0 is supported.
void GLES11BindTexImage( const GLESSurfaceData* pSurfData , GLenum target, GLint level , GLuint isMipmapped);
void GLES2BindTexImage( const GLESSurfaceData* pSurfData , GLenum target, GLint level , GLuint isMipmapped);


//used to release the externally managed texture.
void GLES11ReleaseTexImage(const GLESSurfaceData* pSurfData );
void GLES2ReleaseTexImage(const GLESSurfaceData* pSurfData );




#ifdef __cplusplus
}
#endif

#endif //__GLCONTEXT_H__
