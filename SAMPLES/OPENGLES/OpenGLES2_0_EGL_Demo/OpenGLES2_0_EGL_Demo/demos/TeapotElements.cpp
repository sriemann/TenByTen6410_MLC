#include "stdafx.h"


#include "glHelpers.h"


#include "TeapotElements_Data.h"

#include "Sample.frag.h"
#include "Sample.vert.h"

#include <math.h>


static float mv[16];
static float mvp[16];
static float proj[16];
static float tmvp[16];

//Load shaders from disk... will only work on Windows!
//ghProgram polkaProg("SH/AniPolka.vert","SH/AniPolka.frag");

static ghProgram teapotElementsProg(SampleVertShader, SampleFragShader);

GLuint indexVbo;

static void demoInit()
{
    glViewport(0,0, GLH_WIDTH, GLH_HEIGHT);
    glClearColor(0.6f,0.6f,1.0f,1.0f);
    glClearDepthf(1.0);

    glEnable(GL_DEPTH_TEST);

    matIdentity(proj);
    matPerspective(proj, 45, GLH_WIDTH/float(GLH_HEIGHT), 0.1f, 10000.0f);

    matIdentity(mv);
    matTranslate(mv, -0.1f, 0.0f, -3.0f);
    matRotate(mv, -90, 1.0f, 0.0f, 0.0f);
    
    matMult(mvp, proj, mv);
    
    //Normal matrix calculation
    matInverse(tmvp, mv);
    matTranspose(tmvp);

#if 1
    //VBO element array
    
    glGenBuffers(1, &indexVbo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*nTeapotElements_Elements*sizeof(GLushort),TeapotElements_Elements ,GL_STATIC_DRAW);
#endif    

    checkGLerror("Post array buffer data");

    //Program
    teapotElementsProg.bindAttribLoc(0, "vertexPos");
    teapotElementsProg.bindAttribLoc(1, "iNormal");

    teapotElementsProg.load();
    
    glUseProgram(teapotElementsProg.prog);
    
    //Specify uniforms
    glUniformMatrix4fv(teapotElementsProg.loc("mv"), 1, false, mv);
    glUniformMatrix4fv(teapotElementsProg.loc("proj"),1, false,proj);
    glUniformMatrix4fv(teapotElementsProg.loc("mvp"),1, false, mvp);
    glUniformMatrix4fv(teapotElementsProg.loc("normalMat"),1, false, tmvp);

    glUniform4f(teapotElementsProg.loc("color"), 1.0, 1.0,0.0,0.0);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE,GL_ONE);

}


void TeapotElementsRender()
{
    glUseProgram(teapotElementsProg.prog);

    //Clear frame
    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    //Geometry
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), TeapotElements_Data);

    //Draw!!!
#if 0    
    glDrawElements(GL_TRIANGLES, 3*nTeapotElements_Elements, GL_UNSIGNED_SHORT, TeapotElements_Elements);
#else
    glDrawElements(GL_TRIANGLES, 3*nTeapotElements_Elements, GL_UNSIGNED_SHORT, 0);
#endif
}



int TeapotElementsDemo()
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


int TeapotElementsDeinit()
{
    teapotElementsProg.reset();
    return 1;
}