#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "glHelpers.h"
//#include "buffers.h"
//#include "glContext.h"

#include <vector>
#include <list>

#define GLES2LIB
#define EN_REG_DUMP 1


ghShader::ghShader(const char* text, const unsigned int* bin,  int binLen, int binCS)
{
    textShader = text;
    binShader = bin;
    binShaderLen = sizeof(int) * binLen; //added shariq for converting the size in byte
    binShaderCheckSum = binCS;
}

ghShader::ghShader()
{
    textShader = 0;
    binShader = 0;
    binShaderLen = 0;
    binShaderCheckSum = 0;
}



ghProgram::ghProgram(ghShader vertShaderObject, ghShader fragShaderObject)
{
    vertShader = vertShaderObject;
    fragShader = fragShaderObject;

    loadShadersFromFile = false;

    prog = 0;
    vs = 0;
    fs = 0;

    progUsed = false;

    //_registerProgram(this);
}
    

//Loading from files is only supported on WIN32

ghProgram::ghProgram(const char* vsFileName, const char* fsFileName)
{

    vsFile = vsFileName;
    fsFile = fsFileName;
    loadShadersFromFile = true;
    
    prog = 0;
    vs = 0;
    fs = 0;
}


ghProgram::~ghProgram()
{
    reset();

}

GLint ghProgram::loc(const char* varName)
{
    //TODO: implement a cache? (not for performance but will it help in isolating bugs in gl lib?)
    return glGetUniformLocation(prog, varName);
}

void ghProgram::bindAttribLoc(int index, const char* attribName)
{

    std::string s(attribName);
    
    unsigned int i=0;
    
    for(i=0; i<attribBindings.size(); i++)
    {
        if(attribBindings[i].attribName == s)
        {
            attribBindings[i].index = index;
            break;
        }
    }
    if(i==attribBindings.size())
    {
        attribBindings.push_back(AttribBinding(s,index));
    }
}

void ghProgram::reset()
{
    if(prog)
    {
        glDeleteProgram(prog);
    }

    if(vs)
    {
        glDeleteShader(vs);
    }

    if(fs)
    {
        glDeleteShader(fs);
    }
    
    prog = 0;
    vs = 0;
    fs = 0;
}

int ghProgram::reload()
{
    if(progUsed)
    {
    return    load();
    }
    
    return 0;
}



void checkGLerror(const char* location)
{
    GLenum err = glGetError();
    
    const char* errString = 0;
    switch(err)
    {
        case GL_NO_ERROR:           errString = "GL_NO_ERROR"; break;
        case GL_INVALID_ENUM:       errString = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:      errString = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:  errString = "GL_INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:     errString = "GL_STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:    errString = "GL_STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:      errString = "GL_OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: errString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        default:                    errString = "UNKNOWN ERROR!!"; break;
    }

    if(err != GL_NO_ERROR)
    {
        printf("%s: %s\n", location,errString);
       // __asm int 3;
    }
    
}

void checkFBStatus(const char* location)
{
    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    const char* statusString = 0;
    switch(fbStatus)
    {
    case GL_FRAMEBUFFER_COMPLETE: 
        statusString = "GL_FRAMEBUFFER_COMPLETE"; break;   
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: 
        statusString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: 
        statusString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
//    case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT: 
//        statusString = "GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: 
        statusString = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS: 
        statusString = "GL_FRAMEBUFFER_INCOMPLETE_FORMATS"; break;
    case GL_FRAMEBUFFER_UNSUPPORTED: 
        statusString = "GL_FRAMEBUFFER_UNSUPPORTED"; break;
//    case GL_FRAMEBUFFER_STATUS_ERROR: 
//        statusString = "GL_FRAMEBUFFER_STATUS_ERROR"; break;
    default:
        statusString = "UNKNOWN!"; break;
    }

    printf("@%s FBstatus :%s\n", location, statusString);
}

extern "C" unsigned char* binFileRead(const char*fn, unsigned int* len){

    FILE* fp;
    unsigned char* content = NULL;
    if(fn!=NULL) 
    {
        fp = fopen(fn,"rb");
        if(fp!=NULL) {
            fseek(fp,0,SEEK_END);
            *len = ftell(fp);
            fseek(fp,0,SEEK_SET);

            if(*len > 0) {
                content = (unsigned char*)malloc(sizeof(unsigned char)* (*len));
                *len = fread(content,sizeof(unsigned char),*len, fp);
            }
            fclose(fp);
        }
        else{
            printf("\nERROR: unable to open file %s for reading\n", fn);
        }
    }

    return content;
}

//Taken from lighthouse3d's tut..
extern "C" char *textFileRead(const char *fn) {


    FILE *fp;
    char *content = NULL;

    int count=0;

    if (fn != NULL) {
        fp = fopen(fn,"rt");

        if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      fseek(fp,0,SEEK_SET);

            if (count > 0) {
                content = (char *)malloc(sizeof(char) * (count+1));
                count = fread(content,sizeof(char),count,fp);
                content[count] = '\0';
            }
            //printf("SOURCE:%s\n",content);
            fclose(fp);
        }
        else
        {
            printf("\nERROR: unable to open file %s for reading\n", fn);
        }
    }
   
    return content;
}

int ghProgram::load()
{
    progUsed = true;
    //if(!this->vs)
    {
        this->reset();//to be safe
        this->vs = glCreateShader(GL_VERTEX_SHADER);
        this->fs = glCreateShader(GL_FRAGMENT_SHADER);
        this->prog = glCreateProgram();

            
        glAttachShader(this->prog, this->fs);
        checkGLerror();

        glAttachShader(this->prog, this->vs);
        checkGLerror();
    }



    if(this->loadShadersFromFile)
    {
#ifndef GLES2LIB
        const char* vsText = textFileRead(this->vsFile);
        const char* fsText = textFileRead(this->fsFile);
        if(vsText && fsText)
        {
            printf("\nReloading: %s, %s\n",this->vsFile, this->fsFile);
            glShaderSource(this->fs, 1, &(fsText) , NULL);
            checkGLerror("Post fs text shader load");

            glShaderSource(this->vs, 1, &(vsText) , NULL);
            checkGLerror("Post fs text shader load");

            free((void*)vsText);
            free((void*)fsText);
        }
        else
        {
            printf("Program load failed, one or more shader files not found\n");
            return 0;
        }
#else
        unsigned int vsBinLen=0;
        unsigned int fsBinLen=0;
        char fname[512];
        sprintf(fname,"%s.bin",vsFile);
        const unsigned char* vsBin = binFileRead(fname, &vsBinLen);
        sprintf(fname,"%s.bin",fsFile);
        const unsigned char* fsBin = binFileRead(fname, &fsBinLen);
        glShaderBinary(1, &this->fs, (GLenum)0, fsBin, fsBinLen);
        checkGLerror("Post fs bin load (from filesystem)");
        glShaderBinary(1, &this->vs, (GLenum)0, vsBin, vsBinLen);
        checkGLerror("Post vs bin load (from filesystem)");
#endif

    }
    else
    {
#ifndef GLES2LIB
        glShaderSource(this->fs, 1, &(this->fragShader.textShader) , NULL);
        checkGLerror("Post fs text shader load");

        glShaderSource(this->vs, 1, &(this->vertShader.textShader) , NULL);
        checkGLerror("Post fs text shader load");
#else

    isCheckSumValid(this->fragShader.binShader, this->fragShader.binShaderLen,
        this->fragShader.binShaderCheckSum,"Fragment Shader");
    isCheckSumValid(this->vertShader.binShader, this->vertShader.binShaderLen,
        this->vertShader.binShaderCheckSum,"Vertex Shader");

    glShaderBinary(1, &this->fs, (GLenum)0, this->fragShader.binShader, this->fragShader.binShaderLen);
    checkGLerror("Post fs bin load");

    glShaderBinary(1, &this->vs, (GLenum)0, this->vertShader.binShader, this->vertShader.binShaderLen);
    checkGLerror("Post vs bin load");
#endif
    }


    for(unsigned int i=0; i<this->attribBindings.size(); i++)
    {
        glBindAttribLocation(this->prog, this->attribBindings[i].index, this->attribBindings[i].attribName.c_str());
    }

    glLinkProgram(this->prog);
    checkGLerror();

    static char buff[4000];
    int len;
    glGetProgramInfoLog(this->prog, sizeof(buff), &len, buff);

    printf("\nProgram info log:\n%s\n", buff);
    fflush(stdout);
    
    glUseProgram(this->prog);
    checkGLerror("set current program");
    
    return 1;
}

/*****************************************************************************************/

//TODO: remove this and use c++ swap
void tswap(float& a, float& b)
{
    
    float temp = a;
    a = b;
    b = temp;    
}

/* element order reference:
    m[0]  m[4]  m[8]   m[12]
    m[1]  m[5]  m[9]   m[13]
    m[2]  m[6]  m[10]  m[14]
    m[3]  m[7]  m[11]  m[15]
*/


extern "C" void matTranspose(float* m)
{
    tswap(m[1],m[4]);     tswap(m[2],m[8]);     tswap(m[3],m[12]);
                        tswap(m[6],m[9]);      tswap(m[7],m[13]);
                                            tswap(m[11],m[14]);
}


extern "C" void matIdentity(float m[16])
{
    for(int i=0; i<16; i++)
    {
        m[i] = (i%5==0)? 1.0f : 0.0f;  //The first and every fifth element after that is 1.0 other are 0.0
    }
}

/*
#ifndef fabs
float fabs(float f)
{
    return (f >0)? f : -f;
}
#endif
*/

//TEMP: ripped from nate robins : http://www.xmission.com/~nate/tutors.html
extern "C" bool matInverse(float inverse[16], const float src[16])
{
double t;
    int i, j, k, swap;
    float tmp[4][4];
    
    matIdentity(inverse);
    
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            tmp[i][j] = src[i*4+j];
        }
    }
    
    for (i = 0; i < 4; i++) {
        /* look for largest element in column. */
        swap = i;
        for (j = i + 1; j < 4; j++) {
            if (fabs(tmp[j][i]) > fabs(tmp[i][i])) {
                swap = j;
            }
        }
        
        if (swap != i) {
            /* swap rows. */
            for (k = 0; k < 4; k++) {
                t = tmp[i][k];
                tmp[i][k] = tmp[swap][k];
                tmp[swap][k] = t;
                
                t = inverse[i*4+k];
                inverse[i*4+k] = inverse[swap*4+k];
                inverse[swap*4+k] = t;
            }
        }
        
        if (tmp[i][i] == 0) {
        /* no non-zero pivot.  the matrix is singular, which
           shouldn't happen.  This means the user gave us a bad
            matrix. */
            return false;
        }
        
        t = tmp[i][i];
        for (k = 0; k < 4; k++) {
            tmp[i][k] /= t;
            inverse[i*4+k] /= t;
        }
        for (j = 0; j < 4; j++) {
            if (j != i) {
                t = tmp[j][i];
                for (k = 0; k < 4; k++) {
                    tmp[j][k] -= tmp[i][k]*t;
                    inverse[j*4+k] -= inverse[i*4+k]*t;
                }
            }
        }
    }
    return true;
}
/***************** Virdi added this set of code***********/
// m3=m1*m2
extern "C" void matMultl(float* m3, const float* m1, const float* m2) 
{
        
       m3[0] = m2[0]*m1[0] + m2[4]*m1[1] + m2[8]*m1[2] + m2[12]*m1[3];
       m3[1] = m2[1]*m1[0] + m2[5]*m1[1] + m2[9]*m1[2] + m2[13]*m1[3];
       m3[2] = m2[2]*m1[0] + m2[6]*m1[1] + m2[10]*m1[2] + m2[14]*m1[3];
       m3[3] = m2[3]*m1[0] + m2[7]*m1[1] + m2[11]*m1[2] + m2[15]*m1[3];
       
       m3[4] = m2[0]*m1[4] + m2[4]*m1[5] + m2[8]*m1[6] + m2[12]*m1[7];
       m3[5] = m2[1]*m1[4] + m2[5]*m1[5] + m2[9]*m1[6] + m2[13]*m1[7];
       m3[6] = m2[2]*m1[4] + m2[6]*m1[5] + m2[10]*m1[6] + m2[14]*m1[7];
       m3[7] = m2[3]*m1[4] + m2[7]*m1[5] + m2[11]*m1[6] + m2[15]*m1[7];
       
       m3[8] = m2[0]*m1[8] + m2[4]*m1[9] + m2[8]*m1[10] + m2[12]*m1[11];
       m3[9] = m2[1]*m1[8] + m2[5]*m1[9] + m2[9]*m1[10] + m2[13]*m1[11];
       m3[10] = m2[2]*m1[8] + m2[6]*m1[9] + m2[10]*m1[10] + m2[14]*m1[11];
       m3[11] = m2[3]*m1[8] + m2[7]*m1[9] + m2[11]*m1[10] + m2[15]*m1[11];
       
       m3[12] = m2[0]*m1[12] + m2[4]*m1[13] + m2[8]*m1[14] + m2[12]*m1[15];
       m3[13] = m2[1]*m1[12] + m2[5]*m1[13] + m2[9]*m1[14] + m2[13]*m1[15];
       m3[14] = m2[2]*m1[12] + m2[6]*m1[13] + m2[10]*m1[14] + m2[14]*m1[15];
       m3[15] = m2[3]*m1[12] + m2[7]*m1[13] + m2[11]*m1[14] + m2[15]*m1[15];

}
extern "C" void matMult(float* m3, const float* m1, const float* m2) 
{
        
       m3[0] = m1[0]*m2[0] + m1[4]*m2[1] + m1[8]*m2[2] + m1[12]*m2[3];
       m3[1] = m1[1]*m2[0] + m1[5]*m2[1] + m1[9]*m2[2] + m1[13]*m2[3];
       m3[2] = m1[2]*m2[0] + m1[6]*m2[1] + m1[10]*m2[2] + m1[14]*m2[3];
       m3[3] = m1[3]*m2[0] + m1[7]*m2[1] + m1[11]*m2[2] + m1[15]*m2[3];
       
       m3[4] = m1[0]*m2[4] + m1[4]*m2[5] + m1[8]*m2[6] + m1[12]*m2[7];
       m3[5] = m1[1]*m2[4] + m1[5]*m2[5] + m1[9]*m2[6] + m1[13]*m2[7];
       m3[6] = m1[2]*m2[4] + m1[6]*m2[5] + m1[10]*m2[6] + m1[14]*m2[7];
       m3[7] = m1[3]*m2[4] + m1[7]*m2[5] + m1[11]*m2[6] + m1[15]*m2[7];
       
       m3[8] = m1[0]*m2[8] + m1[4]*m2[9] + m1[8]*m2[10] + m1[12]*m2[11];
       m3[9] = m1[1]*m2[8] + m1[5]*m2[9] + m1[9]*m2[10] + m1[13]*m2[11];
       m3[10] = m1[2]*m2[8] + m1[6]*m2[9] + m1[10]*m2[10] + m1[14]*m2[11];
       m3[11] = m1[3]*m2[8] + m1[7]*m2[9] + m1[11]*m2[10] + m1[15]*m2[11];
       
       m3[12] = m1[0]*m2[12] + m1[4]*m2[13] + m1[8]*m2[14] + m1[12]*m2[15];
       m3[13] = m1[1]*m2[12] + m1[5]*m2[13] + m1[9]*m2[14] + m1[13]*m2[15];
       m3[14] = m1[2]*m2[12] + m1[6]*m2[13] + m1[10]*m2[14] + m1[14]*m2[15];
       m3[15] = m1[3]*m2[12] + m1[7]*m2[13] + m1[11]*m2[14] + m1[15]*m2[15];

}
//transform the vector x,y,z  with the transformation matrix m
extern "C" void vecMult(const float* m, float x, float y, float z)
{    
    float xo,yo,zo;
       xo = m[0] * x + m[4] * y + m[8] * z + m[12];
       yo = m[1] * x + m[5] * y + m[9] * z + m[13];
       zo = m[2] * x + m[6] * y + m[10] * z + m[14];
       
       x=xo;
       y=yo;
       z=zo;
}

//will update the current matrix' m' with the tranalation vector x,y,z
extern "C" void matTranslate(float* m, const float x, const float y, const float z  )
{
       m[12] = m[0] * x + m[4] *y + m[8]* z + m[12];
       m[13] = m[1] * x + m[5] *y + m[9]* z + m[13];
       m[14] = m[2] * x + m[6] *y + m[10]* z + m[14];
       m[15] = m[3] * x + m[7] *y + m[11]* z + m[15];
}  

//rotate about arbit axis
extern "C" bool matRotate(float* m, float DEGAngle, float x, float y, float z)
{
        
       float Sq = sqrt(x*x + y*y + z*z);
       float inv;
       if(Sq > -FLT_EPSILON && Sq < FLT_EPSILON) //chk for divide by zero......
           return false;
       inv = 1.0f/Sq;    
       x = x * inv;   
       y = y * inv;
       z = z * inv;
       float radian = PI_OVER_180 * DEGAngle;
       float f32c = (float)cos(radian); //TODO!! Optimize me
       float f32s = (float)sin(radian);
       float f32OneMinC = 1 - f32c;
        
        float RotMat[16];
       RotMat[0] = f32c + f32OneMinC * x * x;
       RotMat[1] = (f32OneMinC * x * y) + (z * f32s);
       RotMat[2] = (f32OneMinC * x * z) - (y * f32s);
       RotMat[3] = 0.0;
       RotMat[4] = (f32OneMinC * x * y) - (z * f32s);
       RotMat[5] = f32c + f32OneMinC * y * y;
       RotMat[6] = (f32OneMinC * y * z) + (x * f32s);
       RotMat[7] = 0.0;
       RotMat[8] = (f32OneMinC * x * z) + (y * f32s);
       RotMat[9] = (f32OneMinC * y * z) - (x * f32s);
       RotMat[10] = f32c + f32OneMinC * z * z;
       RotMat[11] = RotMat[12] =RotMat[13] = RotMat[14] = 0.0;RotMat[15] =1.0f;
       float t[16];
       matMult( &t[0] ,  m , &RotMat[0]);
       for(int i = 0; i < 16; ++i)
        m[i] = t[i];
        
      return true;
}

//will update the current matrix' m' with the tranalation vector x,y,z
extern "C" void matTranslatel(float* m, const float x, const float y, const float z  )
{
       m[3] = m[0] * x + m[1] *y + m[2]* z + m[3];
       m[7] = m[4] * x + m[5] *y + m[6]* z + m[7];
       m[10] = m[8] * x + m[9] *y + m[10]* z + m[11];
       m[15] = m[12] * x + m[13] *y + m[14]* z + m[15];
}  

//rotate about arbit axis
extern "C" bool matRotatel(float* m, float DEGAngle, float x, float y, float z)
{
        
       float Sq = sqrt(x*x + y*y + z*z);
       float inv;
       if(Sq > -FLT_EPSILON && Sq < FLT_EPSILON) //chk for divide by zero......
           return false;
       inv = 1.0f/Sq;    
       x = x * inv;   
       y = y * inv;
       z = z * inv;
       float radian = PI_OVER_180 * DEGAngle;
       float f32c = (float)cos(radian); //TODO!! Optimize me
       float f32s = (float)sin(radian);
       float f32OneMinC = 1 - f32c;
        
        float RotMat[16];
       RotMat[0] = f32c + f32OneMinC * x * x;
       RotMat[4] = (f32OneMinC * x * y) + (z * f32s);
       RotMat[8] = (f32OneMinC * x * z) - (y * f32s);
       RotMat[12] = 0.0;
       RotMat[1] = (f32OneMinC * x * y) - (z * f32s);
       RotMat[5] = f32c + f32OneMinC * y * y;
       RotMat[9] = (f32OneMinC * y * z) + (x * f32s);
       RotMat[13] = 0.0;
       RotMat[2] = (f32OneMinC * x * z) + (y * f32s);
       RotMat[6] = (f32OneMinC * y * z) - (x * f32s);
       RotMat[10] = f32c + f32OneMinC * z * z;
       RotMat[14] = RotMat[3] = RotMat[7] = RotMat[11] = 0.0;RotMat[15] =1.0f;
       float t[16];
       matMult( &t[0] ,  m , &RotMat[0]);
       for(int i = 0; i < 16; ++i)
        m[i] = t[i];
        
      return true;
}

extern "C" void matScale(float*m, float x, float y, float z)
{
    m[0] *= x ; m[4] *= y ; m[8] *= z ;// m[12] *= x ;  
    m[1] *= x ; m[5] *= y ; m[9] *= z ; //m[13] *= y ;  
    m[2] *= x ; m[6] *= y ; m[10] *= z ; //m[14] *= z ;  
}

//like glfrustum
extern "C" bool matFrustum(float* m,float f32Left, float f32Right,float f32Bottom, float f32Top, float f32ZNear, float f32ZFar)
{
    float diff = f32Right - f32Left;
    if(diff > -FLT_EPSILON && diff < FLT_EPSILON) //chk for divide by zero......
        return false;
    
    diff = f32Top - f32Bottom;
    if(diff > -FLT_EPSILON && diff < FLT_EPSILON) //chk for divide by zero......
        return false;
        
    diff = f32ZFar - f32ZNear;
    if(diff > -FLT_EPSILON && diff < FLT_EPSILON) //chk for divide by zero......
        return false;       
        
       m[0] = float(2.0*f32ZNear/(f32Right-f32Left));
       m[1] = m[2] = m[3] = 0;

       m[4] = 0;
       m[5] = float(2.0*f32ZNear/(f32Top-f32Bottom));
       m[6] = m[7] = 0;

       m[8] = (f32Right + f32Left) / (f32Right - f32Left);
       m[9] = (f32Top + f32Bottom) / (f32Top - f32Bottom);
       m[10] = -( (f32ZNear + f32ZFar) / (f32ZFar - f32ZNear)  );
       m[11] = -1;
       
       m[12] = m[13] =0;
       m[14] = -( (2*f32ZNear*f32ZFar) / (f32ZFar-f32ZNear));
       m[15] = 0;
       return true;
}

//like gluperspective matrix         
extern "C" bool matPerspective(float* m ,float fieldOfViewDegree, float aspectRatio, float zNear, float zFar)
{
   if(fieldOfViewDegree <= 0.0f || fieldOfViewDegree >=180.0f)
      //fieldOfViewDegree = 45.0f;   //assign FOV to 45 deg if value passed is not in proper range 
       return false;

   float FOVrad = float(PI_OVER_180*fieldOfViewDegree*0.5f);//angle divided by 2 !!!

   float f32top = float( zNear*tan(FOVrad) );
   float f32Right = aspectRatio*f32top;
   return matFrustum(m,-f32Right,f32Right,-f32top,f32top,zNear,zFar);

}  

//like glortho
extern "C" bool matOrtho(float* m,float f32Left, float f32Right,float f32Top, float f32Bottom, float f32ZNear, float f32ZFar)
{
    float diff = f32Right - f32Left;
    if(diff > -FLT_EPSILON && diff < FLT_EPSILON) //chk for divide by zero......
        return false;
    
    diff = f32Top - f32Bottom;
    if(diff > -FLT_EPSILON && diff < FLT_EPSILON) //chk for divide by zero......
        return false;
        
    diff = f32ZFar - f32ZNear;
    if(diff > -FLT_EPSILON && diff < FLT_EPSILON) //chk for divide by zero......
        return false;
        
    m[0] = float(2.0/(f32Right-f32Left));
   m[1] = m[2] = m[3] = 0;

   m[4] = 0;
   m[5] = float(2.0/(f32Top-f32Bottom));
   m[6] = m[7] = 0;

   m[8] = m[9] = 0;
   m[10] = -float(2.0/(f32ZFar - f32ZNear));
   m[11] = 0;
   
   m[12] = -((f32Right+f32Left)/(f32Right-f32Left));
   m[13] = -((f32Top+f32Bottom)/(f32Top-f32Bottom));
   m[14] = -((f32ZNear+f32ZFar)/(f32ZFar-f32ZNear));
   m[15] = 1;
   return true;
}
extern "C" bool vecNormalize(float& x, float& y, float& z)
{
       float Sq = sqrt(x*x + y*y + z*z);
       float inv;
       if(Sq > -FLT_EPSILON && Sq < FLT_EPSILON) //chk for divide by zero......
           return false;
       inv = 1.0f/Sq;    
       x = x * inv;   
       y = y * inv;
       z = z * inv;
       return true;
}

//vector x = x1 cross y1
extern "C" void vecCrossProduct(float& x,float& y, float& z , float x1,float y1, float z1 ,float x2,float y2, float z2)
{
    //x= y1*z2 - y1*z2;
    x= y1*z2 - y2*z1;
    y= z1*x2 - z2*x1;
    z= x1*y2 - x2*y1;
}         
//like gluLookAt
extern "C" void matLookAt(float* m, float px, float py, float pz,float tx, float ty, float tz, float ux, float uy, float uz)
 {
    float sx,sy,sz;
    float uux,uuy,uuz;
    float M[16];

    float zx = tx-px;
    float zy = ty-py;
    float zz = tz-pz;

    vecNormalize(zx,zy,zz);
    vecNormalize(ux,uy,uz);

    vecCrossProduct(sx,sy,sz,zx,zy,zz,ux,uy,uz);

    vecCrossProduct(uux,uuy,uuz,sx,sy,sz,zx,zy,zz);

    vecNormalize(sx,sy,sz);
    vecNormalize(uux,uuy,uuz);

    M[0]=sx; M[4]=sy; M[8]=sz; M[12]=0;
    M[1]=uux; M[5]=uuy; M[9]=uuz; M[13]=0;
    M[2]=-zx; M[6]=-zy; M[10]=-zz; M[14]=0;
    M[3]=0; M[7]=0; M[11]=0; M[15]=1.0;

    matTranslate(M,-px,-py,-pz);

 }

extern "C" void matPrint(float* m)
{
    for(int i=0; i<16; i++)
    {
        if(i%4 == 0) printf("\n");
        printf("%f  ",m[i]);
    }
    printf("\n");
}


extern "C" unsigned int simpleCheckSum(const unsigned int* bin, int length)
{
    int i;
    unsigned int cs = 0;
    unsigned char* cbin =(unsigned char *) bin;        //added shariq
    for(i=0; i< length ; i++)
    {
        cs ^= cbin[i];
    }

    return cs;
}

extern "C" void isCheckSumValid(const unsigned int* bin, int len, int checkSum, const char* name)
{
    int cs = simpleCheckSum(bin,len);
    
    if(cs != checkSum)
    {
        
        //printf("Checksum MISMATCH! for %s\n", name);
        return;
    }
    
    //printf("Checksum PASS! for %s\n",name);
    
}
