//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/
/*
***************************************************************************//*!
*
* \file        Osalioctl.h
* \brief File contains the input and output data structure and defines for Osal functions for OpenGl2.0 Lib.    
*
*//*---------------------------------------------------------------------------
* NOTES:
*
*/

#ifndef __gl_OSAL_IOCTL_h_
#define __gl_OSAL_IOCTL_h_ 1

#include <windows.h>
/*
#ifdef WINCE_DLL
    #include "gl.h"
#else
    #include "gles.h"
#endif
*/
// GLenums #defines made into real enums for easier debugging while development
typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef signed char     GLbyte;
typedef short           GLshort;
typedef int             GLint;
typedef int             GLsizei;
typedef unsigned char   GLubyte;
typedef unsigned short  GLushort;
typedef unsigned int    GLuint;
typedef float           GLfloat;
typedef float           GLclampf;
typedef int             GLfixed;
typedef int             GLclampx;

typedef int             GLintptr;
typedef int             GLsizeiptr;

typedef struct _GL_ioctlBuffer {
//    unsigned int lock;
    unsigned int length; //Queue Buffer length, inital value is zero
    unsigned int bufPointer;
    unsigned int data[2000];
} GL_ioctlBuffer;

struct GlAPIPARAMS
{
GLenum     VarGLenum1;
GLenum     VarGLenum2;
GLenum     VarGLenum3;
GLenum     VarGLenum4;

GLboolean VarGLboolean1;
GLboolean VarGLboolean2;
GLboolean VarGLboolean3;
GLboolean VarGLboolean4;

GLbitfield  VarGLbitfield1;

GLint  VarGLint1;
GLint  VarGLint2;
GLint  VarGLint3;
GLint  VarGLint4;
GLint  VarGLint5;
GLint  VarGLint6;

GLsizei VarGLsizei1;
GLsizei VarGLsizei2;
GLsizei VarGLsizei3;
GLsizei VarGLsizei4;

GLuint VarGLuint1;
GLuint VarGLuint2;

GLfloat VarGLfloat1;
GLfloat VarGLfloat2;
GLfloat VarGLfloat3;
GLfloat VarGLfloat4;

GLclampf VarGLclampf1;
GLclampf VarGLclampf2;
GLclampf VarGLclampf3;
GLclampf VarGLclampf4;

GLintptr VarGLintptr1;

GLsizeiptr VarGLsizeiptr1;

GLsizei *ptrGLsizei1;

GLint *ptrGLint1;
GLint *ptrGLint2;

GLboolean *ptrGLboolean1;

char *ptrchar1;

void *ptrvoid1;
void *ptrvoid2;

GLuint *ptrGLuint1;

GLfloat * ptrGLfloat1;

void **ptrdvoid1;

const GLint *ptrconstGLint1;

const GLfloat *ptrconstGLfloat1;

const void *ptrconstvoid1;

const GLuint *ptrconstGLuint1;

const char **ptrdconstchar1; 

const char *ptrconstchar1;

GLenum *ptrGLenum1;

int varint1;

GL_ioctlBuffer *pioctlBuffer;
};

struct GLRetVal
{
    GLuint RetValGLuint;
    int RetValint;
    GLenum RetValGLenum;
    const GLubyte * ptrRetconstGLubyte;
    GLboolean RetValGLboolean;
    void *ptrRetvoid;
};

struct ChunkAllocArg
{
    void *handle;
    void *phyaddr;
    void *viraddr;
};

struct GlfFlushArg
{
    void *pState;
    FramebufferData fbData;
};

struct GlfSetDrawModeArg
{
    void *pState;
    GLenum mode;
};

struct GlfDrawArg
{
    void *pState;
    GLenum primitive;
    GLint first;
    GLsizei count;
};

struct GlfDrawElementsArg
{
    void *pState;
    GLenum primitive;
    GLsizei count;
    GLenum type;
    void* indices;
};

struct GetDMACODEArg
{
    UINT32 offset;
    int cacheEnable;
};

struct GetPhysMemStatusArg
{
    int usedMemory;
    int totalMemory;
};

#define IOCTL_FUNCODE(X)    (2048 + X )
#define FIMG_6410_v1_5 FILE_DEVICE_CONTROLLER
#define FIMG_6410_v1_5DEVICETYPE_CTL_CODE(_Function, _Method, _Access)  \
            CTL_CODE(FIMG_6410_v1_5, _Function, _Method, _Access)

#define IOCTL_GetPhysicalAddress                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(169), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_getSFRAddress                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(177), METHOD_BUFFERED, FILE_ANY_ACCESS)                        
            
#define IOCTL_freeSFRAddress                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(178), METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_waitForPipelineStatus                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(179), METHOD_BUFFERED, FILE_ANY_ACCESS)      
            
#define IOCTL_AllocPhysMem                      \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(180), METHOD_BUFFERED, FILE_ANY_ACCESS)   
            
#define IOCTL_FreePhysMem                       \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(181), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_LockDrawCall                       \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(182), METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_UnlockDrawCall                       \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(183), METHOD_BUFFERED, FILE_ANY_ACCESS)                                                    
            
#define IOCTL_getDMASFRAddress                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(184), METHOD_BUFFERED, FILE_ANY_ACCESS)                        
            
#define IOCTL_freeDMASFRAddress                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(185), METHOD_BUFFERED, FILE_ANY_ACCESS)            
            
#define IOCTL_getDMACODEAddress                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(186), METHOD_BUFFERED, FILE_ANY_ACCESS)                        
            
#define IOCTL_freeDMACODEAddress                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(187), METHOD_BUFFERED, FILE_ANY_ACCESS)                   
                        
#define IOCTL_GetPhysMemStatus                       \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(188), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_RequestDepthBuffer                  \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(189), METHOD_BUFFERED, FILE_ANY_ACCESS)                   

#define IOCTL_ReleaseDepthBuffer                       \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(190), METHOD_BUFFERED, FILE_ANY_ACCESS)                       

#define IOCTL_ReleaseAllMemoryBlock                       \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(191), METHOD_BUFFERED, FILE_ANY_ACCESS)  

#define IOCTL_ReAllocAllMemoryBlock                       \
            FIMG_6410_v1_5DEVICETYPE_CTL_CODE(IOCTL_FUNCODE(192), METHOD_BUFFERED, FILE_ANY_ACCESS)  

                        
#endif /* __gl_OSAL_IOCTL_h_ */
