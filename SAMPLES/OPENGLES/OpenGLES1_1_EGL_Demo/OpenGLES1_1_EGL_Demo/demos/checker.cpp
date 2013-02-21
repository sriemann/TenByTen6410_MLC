/*
 * License Applicability. Except to the extent portions of this file are
 * made subject to an alternative license as permitted in the SGI Free
 * Software License B, Version 1.1 (the "License"), the contents of this
 * file are subject only to the provisions of the License. You may not use
 * this file except in compliance with the License. You may obtain a copy
 * of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
 * Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
 * 
 * http://oss.sgi.com/projects/FreeB
 * 
 * Note that, as provided in the License, the Software is distributed on an
 * "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
 * DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
 * CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 * 
 * Original Code. The Original Code is: OpenGL Sample Implementation,
 * Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
 * Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
 * Copyright in any portions created by third parties is as indicated
 * elsewhere herein. All Rights Reserved.
 * 
 * Additional Notice Provisions: The application programming interfaces
 * established by SGI in conjunction with the Original Code are The
 * OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
 * April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
 * 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
 * Window System(R) (Version 1.3), released October 19, 1998. This software
 * was created using the OpenGL(R) version 1.2.1 Sample Implementation
 * published by SGI, but has not been independently verified as being
 * compliant with the OpenGL(R) version 1.2.1 Specification.
 *
 */

/*  checker.c
 *  This program texture maps a checkerboard image onto
 *  two rectangles.
 *
 *  If running this program on OpenGL 1.0, texture objects are
 *  not used.
 */
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include <gles/gl.h>

#include <math.h>
#include <Winbase.h>

static void display(void);
static void reshape(int w,int h);
static void init(void);

static GLuint texName;

/*    Create checkerboard texture    */
#define    checkImageWidth 64
#define    checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];

void makeCheckImage(void)
{
   int i, j, c;
    
   for (i = 0; i < checkImageHeight; i++) {
      for (j = 0; j < checkImageWidth; j++) {
         c = ((((i&0x8)==0)^((j&0x8))==0))*255;
         checkImage[i][j][0] = (GLubyte) c;
         checkImage[i][j][1] = (GLubyte) c;
         checkImage[i][j][2] = (GLubyte) c;
         checkImage[i][j][3] = (GLubyte) 255;
      }
   }
}

static void init(void)
{   
   static const GLfloat v[] = {
       -2.0, -1.0, 0.0,
       -2.0, 1.0, 0.0,
       0.0, -1.0, 0.0,
       0.0, 1.0, 0.0,

       1.0, -1.0, 0.0,
       1.0, 1.0, 0.0,
       2.41421f, -1.0, -1.41421f,
       2.41421f, 1.0, -1.41421f,
   };
   static const GLbyte t[] = {
       0, 0,
       0, 1,
       1, 0,
       1, 1,

       0, 0,
       0, 1,
       1, 0,
       1, 1,
   };
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_FLAT);
   glEnable(GL_DEPTH_TEST);

   makeCheckImage();
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glGenTextures(1, &texName);
   glBindTexture(GL_TEXTURE_2D, texName);

   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth, checkImageHeight, 
                0, GL_RGBA, GL_UNSIGNED_BYTE, checkImage);

   glVertexPointer(3, GL_FLOAT, 0, v);
   glTexCoordPointer(2, GL_BYTE, 0, t);
      
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void CheckerRender()
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(1.0,0.0,0.0,0.0);
   glEnable(GL_TEXTURE_2D);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
   
   glBindTexture(GL_TEXTURE_2D, texName);

   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);

   glFlush();
   glDisable(GL_TEXTURE_2D);
}

static void reshape(int w,int h)
{
       glViewport (0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glFrustumf(-1.5f, 1.5f, -1.5f,1.5f, 1.0f, 10.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
       glTranslatef(0.0, 0.0, -3.6f);
}


int CheckerDemo()
{
    init ();
    reshape(WINDOW_WIDTH, WINDOW_HEIGHT);

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

int CheckerDeinit()
{
    return 1;
}
