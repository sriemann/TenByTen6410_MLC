#include "stdafx.h"

#include "glHelpers.h"


#include "Pawn_Data.h"
#include "Fsq_Data.h"

#include "AniPolka.frag.h"
#include "AniPolka.vert.h"

#include "PostProcess.vert.h"
#include "PPGray.frag.h"

#include <math.h>

//static GLuint geomVbo=0;


static const int numTris = nNumTrisPawn;
static const float* geomData = Pawn_Data;
static const int vertexSize = 4*nNumAttributesPawn;


static float mv[16];
static float mvp[16];
//static float nm[16];
static float proj[16];
static float tmvp[16];

//Load shaders from disk... will only work on Windows!
//ghProgram polkaProg("SH/AniPolka.vert","SH/AniPolka.frag");

//Use the shader included in headerfile
static ghProgram polkaProg(AniPolkaVertShader, AniPolkaFragShader);
static ghProgram ppGrayProg(PostProcessVertShader, PPGrayFragShader);

static GLuint renderTex;
//static GLuint renderBuffer;
static GLuint fbo;
static GLuint rbo;
const int RTwidth = 800;//240;
const int RTheight = 480;//320;

static void demoInit()
{
    glViewport(0,0, GLH_WIDTH, GLH_HEIGHT);
    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClearDepthf(1.0f);

    glEnable(GL_DEPTH_TEST);

    matIdentity(proj);
    matPerspective(proj, 45, GLH_WIDTH/float(GLH_HEIGHT), 0.1f, 10000.0f);

    matIdentity(mv);
    matTranslate(mv, 0.0, 0.0,-30.0);

    matMult(mvp, proj, mv);

    //VBO
    /*
    glGenBuffers(1, &geomVbo);

    glBindBuffer(GL_ARRAY_BUFFER, geomVbo);
    glBufferData(GL_ARRAY_BUFFER, vertexSize*numTris*3,geomData ,GL_STATIC_DRAW);
    */

    checkGLerror("Post array buffer data");

    //Program
    polkaProg.bindAttribLoc(0, "vertexPos");
    polkaProg.bindAttribLoc(1, "iNormal");

    polkaProg.load();

    ppGrayProg.bindAttribLoc(0, "vertexPos");
    ppGrayProg.bindAttribLoc(1, "iTexcoord");

    ppGrayProg.load();

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &renderTex);
    glBindTexture(GL_TEXTURE_2D, renderTex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RTwidth, RTheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    checkGLerror("Post tex create");
    printf("\nTex id: %d\n",renderTex);

    
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);


    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, RTwidth, RTheight);

    checkGLerror("Post RBO create");
    printf("\nRBO: %d\n", rbo);

    glGenFramebuffers(1, &fbo);
    printf("\nFBO: %d\n", fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    checkGLerror("Post FBO create");

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);
    checkGLerror("Post FBO tex attach");
//    glDisable(GL_DEPTH_TEST);
//    glDepthMask(GL_FALSE);
    checkFBStatus("Post FBO tex attach");

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    checkGLerror("Post FBO rbo attach");
    checkFBStatus("Post FBO rbo attach");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


//    glEnable(GL_DEPTH_TEST);
//    glDepthMask(GL_TRUE);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE,GL_ONE);

}


static UINT32 elapsedTime=0;

void PostProcessFBORender()
{
    glUseProgram(polkaProg.prog);

    static float a = 0.0;
    a = 5.0f * (elapsedTime*0.1f);
    if(++elapsedTime > 100)
    {
        elapsedTime = 0;
    }
    float scale = (float) abs(int(sin(a)));
    float lpY = 0.75f;
    float lpX = 30.0f * sin(a);
    float lpZ = -10.0f + 30.0f * cos(a);
    bool nrmMatcalc = true;

//-------------Render to FBO-------------
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0,0,RTwidth, RTheight);
    //Clear frame
    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    checkGLerror("Post FBO clear");

    glUseProgram(polkaProg.prog);

    if(nrmMatcalc)
    {
        //Normal matrix calculation
        matInverse(tmvp, mv);
        matTranspose(tmvp);
        nrmMatcalc = false;
    }
    //Specify uniforms
    glUniformMatrix4fv(polkaProg.loc("mv"), 1, false, mv);
    glUniformMatrix4fv(polkaProg.loc("proj"),1, false,proj);
    glUniformMatrix4fv(polkaProg.loc("mvp"),1, false, mvp);
    glUniformMatrix4fv(polkaProg.loc("normalMat"),1, false, tmvp);

    glUniform4f(polkaProg.loc("lightPos"), lpX, lpY, lpZ,0.0);
    glUniform1f(polkaProg.loc("time"), 0.2f*a);
    glUniform4f(polkaProg.loc("color"), 1.0f * scale, 1.0f * scale,0.0f,0.0f);

    //Geometry
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, geomData);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexSize, (char*)(geomData)+4*sizeof(float));

    //Draw!!!
//    printf("draw1\n");
    glDrawArrays(GL_TRIANGLES, 0,numTris*3);
//    printf("draw1 done\n");

//-------------Render to back display buffer-------------
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0, GLH_WIDTH, GLH_HEIGHT);

    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(ppGrayProg.prog);
    glUniform1f(ppGrayProg.loc("time"), 0.2f*a);
    glUniform1i(ppGrayProg.loc("tex"), 0);
    glUniform1f(ppGrayProg.loc("width"),(float) RTwidth);
    glUniform1f(ppGrayProg.loc("height"), (float)RTheight);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, nNumAttributesFsq*4, Fsq_Data);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, nNumAttributesFsq*4, (char*)(Fsq_Data)+4*sizeof(float));

//    printf("draw2\n");
    glDrawArrays(GL_TRIANGLES, 0, nNumTrisFsq*3);
//    printf("draw1 done\n");

}



int PostProcessFBODemo()
{
    demoInit();
/*
    DWORD diff = 0;
    DWORD start = GetTickCount();

    for(int demo_loop = 0; demo_loop<200;demo_loop++)
    {DWORD start_render = GetTickCount();
        //Render Scene
        render();

    DWORD end_render = GetTickCount();
    diff += (end_render - start_render);
        //Swap Buffers
        eglSwapBuffers ( dpy, surface);
        _DEGLSwapBuffers();
    }
        
    DWORD end = GetTickCount();
    float fps = (float)(200.0*1000)/(float)(end - start);

    printf("start %d \n",start);
    printf("end %d \n",end);
    printf("fps %f \n",fps);
    printf("fps for only render %f \n",(200.0*1000)/(float)diff);
*/
    return 1;
}

int PostProcessFBODeinit()
{
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
    polkaProg.reset();
    ppGrayProg.reset();
    return 1;
}