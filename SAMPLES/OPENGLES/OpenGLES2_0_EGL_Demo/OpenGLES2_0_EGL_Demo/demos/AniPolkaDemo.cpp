#include "stdafx.h"


#include "glHelpers.h"

#include "Pawn_Data.h"

#include "AniPolka.frag.h"
#include "AniPolka.vert.h"

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

//Use the shader included in headerfile
static ghProgram polkaProg(AniPolkaVertShader, AniPolkaFragShader);


static GLint mvLoc;
static GLint projLoc;
static GLint mvpLoc;
static GLint normalMatLoc;
static GLint lightPosLoc;
static GLint timeLoc;
static GLint colorLoc;

static UINT32 elapsedTime=0;


static void demoInit()
{
    glViewport(0,0, GLH_WIDTH, GLH_HEIGHT);

    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClearDepthf(1.0f);

    glEnable(GL_DEPTH_TEST);

    matIdentity(proj);
    matPerspective(proj, 45, GLH_WIDTH/float(GLH_HEIGHT), 0.1f, 10000.0f);

    matIdentity(mv);
    matTranslate(mv, 0.0f, 0.0f,-30.0f);

    matMult(mvp, proj, mv);

    //VBO
    /*
    glGenBuffers(1, &geomVbo);

    glBindBuffer(GL_ARRAY_BUFFER, geomVbo);
    glBufferData(GL_ARRAY_BUFFER, vertexSize*numTris*3,geomData ,GL_STATIC_DRAW);
    */

    checkGLerror("Post array buffer data");

    //Program
    //Swapped order to make this a more difficult test case
    polkaProg.bindAttribLoc(1, "vertexPos");
    polkaProg.bindAttribLoc(0, "iNormal");

    polkaProg.load();
    
      mvLoc = polkaProg.loc("mv");
      projLoc= polkaProg.loc("proj");
      mvpLoc= polkaProg.loc("mvp");
      normalMatLoc= polkaProg.loc("normalMat");
      lightPosLoc= polkaProg.loc("lightPos");
      timeLoc= polkaProg.loc("time");
      colorLoc= polkaProg.loc("color");

    //Normal matrix calculation
    matInverse(tmvp, mv);
    matTranspose(tmvp);
    
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE,GL_ONE);

    glClearColor(0.2f,0.2f,0.2f,1.0f);
    //glEnable(GL_DITHER);

    //elapsedTime = GetTickCount();
}


void AniPolkaRender()
{
    glUseProgram(polkaProg.prog);
 
    //Clear frame
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    static float a = 0.0;
    a = 5.0f*(elapsedTime*0.1f);
    if(++elapsedTime > 1000)
    {
        elapsedTime = 0;
    }
    float scale = (float) abs(int(sin(a)));
    float lpY = 0.75f;
    float lpX = 30.0f*float(sin(a));
    float lpZ = -10.0f + 30.0f*cos(a);

    //glDisable(GL_SCISSOR_TEST);
   
   

    //glEnable(GL_SCISSOR_TEST);
    //glScissor(GLH_WIDTH/3,GLH_HEIGHT/3,GLH_WIDTH, GLH_HEIGHT);



    //Specify uniforms
    glUniformMatrix4fv(mvLoc, 1, false, mv);
    glUniformMatrix4fv(projLoc,1, false,proj);
    glUniformMatrix4fv(mvpLoc,1, false, mvp);
    glUniformMatrix4fv(normalMatLoc,1, false, tmvp);

    glUniform4f(lightPosLoc, lpX, lpY, lpZ,0.0);
    glUniform1f(timeLoc, 0.2f*a);
    glUniform4f(colorLoc, 1.0f*scale, 1.0f*scale,0.0f,0.0f);

    //Geometry
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    //Swapped order to make this a more difficult test case
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, vertexSize , geomData);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, vertexSize, (char*)(geomData)+4*sizeof(float));

//For testing flat shading using glVaryingInterpolationEXP
#if 0  
#ifdef GLES2LIB
    glVaryingInterpolationEXP("normal", GL_FLAT_EXP);
    glVaryingInterpolationEXP("eyeVec", GL_SMOOTH_EXP);
#else
    glShadeModel(GL_FLAT);
#endif
#endif

    //Draw!!!
    glDrawArrays(GL_TRIANGLES, 0,numTris*3);
}



int AniPolkaDemo()
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

int AniPolkaDeinit()
{
    polkaProg.reset();
    return 1;
}