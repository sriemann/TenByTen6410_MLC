/*
*******************************************************************************
*
*                        SAMSUNG INDIA SOFTWARE OPERATIONS
*                               Copyright(C) 2006
*                              ALL RIGHTS RESERVED
*
* This program is proprietary  to Samsung India  Software Operations Pvt. Ltd.,
* and is protected under International Copyright Act as an unpublished work.Its
* use and disclosure is limited by the terms and conditions of a license agree-
* -ment. It may not be  copied or otherwise  reproduced or disclosed to persons
* outside the licensee's  organization except in accordance  with the terms and
* conditions of  such an agreement. All copies and  reproductions  shall be the
* property of  Samsung  India Software Operations Pvt. Ltd.  and must bear this
* notice in its entirety.
*
*******************************************************************************
*/

/*
***************************************************************************//*!
*
* \file        ghHelpers.h
* \author    Sandeep Kakarlapudi (s.kakarla@samsung.com)
*           Sandeep Virdi (san.virdi@samsung.com)
* \brief    GL framework
*
*//*---------------------------------------------------------------------------
*/

#ifndef __GLHELPERS_H__
#define __GLHELPERS_H__


//GL_HELPERS_INCLUDED is used by the shader .h files!
#define GL_HELPERS_INCLUDED 1


#include <gles2/gl2.h>

enum {GLH_WIDTH = 800,GLH_HEIGHT=480};

//---------- Dummy EGL framework ---------- 
void _DEGLInitializeContext(int maxSBC);

bool _DEGLProcessEvents();

void _DEGLSwapBuffers(); //Note: on windows this sleeps for 1ms to prevent things from rendering too fast

float _DEGLGetElapsedTime();

//------------------------------------------ 


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define PI (3.141592)
#define PI_OVER_180 (PI/180.0)
#define ERR -1
#define NOERR 0

#define TGA_RGB         2        // This tells us it's a normal RGB (really BGR) file
#define TGA_A         3        // This tells us it's a ALPHA file
#define TGA_RLE        10        // 

struct tImageTGA
{
public:
    int channels;            // The channels in the image (3 = RGB : 4 = RGBA)
    int sizeX;            // The width of the image in pixels
    int sizeY;            // The height of the image in pixels
    unsigned char *data;        // The image pixel data
};

struct ghShader
{
    const char* textShader;
    const unsigned int* binShader;
    int binShaderLen;
    int binShaderCheckSum;

    ghShader(const char* text, const unsigned int* bin,  int binLen, int binCS);

    ghShader();
};

class ghProgram;

int ghLoadShaders(ghProgram& prog);

void reloadAllShaders();

struct AttribBinding
{
    AttribBinding(const char* aName, int indx)
        :attribName(aName), index(indx)
    {}
    AttribBinding(const std::string& s, int indx)
        :attribName(s), index(indx)
    {}

    std::string attribName;
    int         index;
};

class ghProgram 
{
public:
    GLuint fs;      //GL fragment shader id
    GLuint vs;      //GL vertex shader id
    GLuint prog;    //GL program shader id

    ghShader vertShader;
    ghShader fragShader;
    
    bool loadShadersFromFile;
    
    const char* fsFile;
    const char* vsFile;

    ghProgram(ghShader vertShaderObject, ghShader fragShaderObject);

//#ifdef WIN32
//#ifndef GLES2LIB    
    //Loading from files is only supported on WIN32
    ghProgram(const char* vsFileName, const char* fsFileName);
//#endif
//#endif

    ~ghProgram();

    GLint loc(const char* varName);

    void bindAttribLoc(int index, const char* attribName);

    int load();

    int reload();

    void reset();

private:
    std::vector<AttribBinding> attribBindings;
    bool progUsed;

    ghProgram& operator= (ghProgram&);
    
};


void checkGLerror(const char* location = "");
void checkFBStatus(const char* location = "");

extern "C"
{
void matTranspose(float* m);
void matIdentity(float m[16]);
bool matInverse(float inverse[16], const float src[16]);
void matMult(float* m3, const float* m1, const float* m2) ;
void matTranslate(float* m, const float x, const float y, const float z  );
bool matRotate(float* m, float DEGAngle, float x, float y, float z);
void matScale(float*m, float x, float y, float z);
bool matFrustum(float* m,float f32Left, float f32Right,float f32Bottom, float f32Top, float f32ZNear, float f32ZFar);
bool matPerspective(float* m ,float fieldOfViewDegree, float aspectRatio, float zNear, float zFar);
bool matOrtho(float* m,float f32Left, float f32Right,float f32Top, float f32Bottom, float f32ZNear, float f32ZFar);
bool vecNormalize(float& x, float& y, float& z);
void vecCrossProduct(float& x,float& y, float& z , float x1,float y1, float z1 ,float x2,float y2, float z2);
void vecMult(const float* m, float x, float y, float z);

void matLookAt(float* m, float px, float py, float pz,float tx, float ty, float tz, float ux, float uy, float uz);
void matPrint(float* m);

//No need to expose this perhaps since loadShaders calls these internally
unsigned int simpleCheckSum(const unsigned int* bin, int length);
void isCheckSumValid(const unsigned int* bin, int len, int checkSum, const char* name);
}


#endif //__GLHELPERS_H__
