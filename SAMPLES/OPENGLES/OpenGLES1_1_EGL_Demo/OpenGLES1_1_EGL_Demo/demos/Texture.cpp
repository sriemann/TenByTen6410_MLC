#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include <gles/gl.h>
#include <math.h>


#define PI 3.141592f

extern int ReadBmp (const char *filename, unsigned char  *buffer, int *width, int *height);
extern void WriteBmp(const char *pFilename, const unsigned int *pBuffer, int nWidth, int nHeight);

static GLint TexWidth, TexHeight;
GLubyte CheckerData[64][64][4];
GLint *ImageData1;
GLint *SubImageData1;
GLfloat texangx, texangy, texangz;

GLvoid MakeCheckerImage (GLvoid)
{
    GLint i, j, c;

    for (i = 0; i < TexHeight; i ++)
    {
        for (j = 0; j < TexWidth; j++)
        {
            c = ((((i & 0x8) == 0) ^ ((j & 0x8)) == 0)) * 255;
            CheckerData[i][j][0] = (GLubyte) c;
            CheckerData[i][j][1] = (GLubyte) c;
            CheckerData[i][j][2] = (GLubyte) c;
            CheckerData[i][j][3] = (GLubyte) 255;
        }
    }
}

GLvoid CreateImage (GLint ImageId)
{
    GLint width, height;

    switch (ImageId)
    {
    case 1:
        width = 128;
        height = 128;
        ImageData1 = (GLint*) malloc (width * height * (sizeof (GLint)));
        ReadBmp ("kat_128_128.bmp",(unsigned char*) ImageData1, &TexWidth, &TexHeight);
        break;
    case 2:
        width = 74;
        height = 94;
        SubImageData1 = (GLint*) malloc (width * height * (sizeof (GLint)));
        ReadBmp ("Amoeba_74_94.bmp",(unsigned char*) SubImageData1, &TexWidth, &TexHeight);
    }
}

GLvoid FreeImage (GLint ImageId)
{
    switch (ImageId)
    {
    case 1:
        free (ImageData1);
        break;
    case 2:
        free (SubImageData1);
        break;
    }
}


void ReSizeGLScene_Texture (int w, int h)
{
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    //glFrustumf (-15.0f, 15.0f, -20.0f, 20.0f, 0.30f, 200.0f);
    glOrthof(-1.5, 1.5, -1.5*(GLfloat)h/(GLfloat)w,
                 1.5*(GLfloat)h/(GLfloat)w, -10.0, 10.0);
    glMatrixMode (GL_MODELVIEW);
    //glTranslatef (10.0, 10.0, -10.0f);

    glLoadIdentity ();

    glViewport (0, 0, w, h);
}

int InitGL_Texture (void)
{
    GLuint texName;
    GLint CurrentTexWidth, CurrentTexHeight;

    glClearColor (0.0, 0.0, 0.0, 0.0);
    glEnable (GL_DEPTH_TEST);

    glGenTextures (1, &texName);
    glBindTexture (GL_TEXTURE_2D, texName);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    CreateImage ( 1);
    CurrentTexWidth = TexWidth;
    CurrentTexHeight = TexHeight;

    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, CurrentTexWidth, CurrentTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, ImageData1);
    FreeImage (1);

    return GL_TRUE;
}

#define LEVELS 20
#define ARCS 30 



static void DrawSphere () 
{
    GLfloat vertex[(ARCS + 1) * LEVELS * 3 * 2] = {0};
    GLfloat texture[(ARCS + 1) * LEVELS * 2 * 2] = {0};
    GLfloat x1, y1, z1, x2, y2, z2;
    GLfloat r;
    GLfloat levelstep, arcsstep;
    GLfloat cphi1, sphi1, cphi2, sphi2, ctheta, stheta;
    int index = 0; 
    int nindex = 0; 
    int tindex = 0;
    int i = 0, j = 0;
    
    glEnable (GL_TEXTURE_2D);
    glVertexPointer (3, GL_FLOAT, 0, vertex); 
//    glNormalPointer (GL_FLOAT, 0, normal);
    glTexCoordPointer (2, GL_FLOAT, 0, texture);

    r = 1;
    
    levelstep = PI / LEVELS ;
    arcsstep = (2.0f * PI) / ARCS;
    
    for (i = 0; i < LEVELS; i++) 
    {
        cphi1 = cos (i*levelstep);
        sphi1 = sin (i*levelstep);

        cphi2 = cos ((i + 1) * levelstep);
        sphi2 = sin ((i + 1) * levelstep);

        for (j = 0; j <= ARCS; j++) 
        {
            ctheta = cos (j * arcsstep);
            stheta = sin (j * arcsstep);
            x1 = r * ctheta * sphi1;
            y1 = r * stheta * sphi1; 
            z1 = r * cphi1;

            x2 = r * ctheta * sphi2;
            y2 = r * stheta * sphi2; 
            z2 = r * cphi2;


            vertex[index++] = x1;
            vertex[index++] = y1;
            vertex[index++] = z1;

            vertex[index++] = x2;
            vertex[index++] = y2;
            vertex[index++] = z2;

        /*    normal[nindex++] = ctheta * sphi1;
            normal[nindex++] = stheta * sphi1;
            normal[nindex++] = cphi1;

            normal[nindex++] = ctheta * sphi2;
            normal[nindex++] = stheta * sphi2;
            normal[nindex++] = cphi2;*/

            texture [tindex++] = j * (1.0f / ARCS);
            texture [tindex++] = i * (1.0f / LEVELS);
            
            texture [tindex++] = j * (1.0f / ARCS);
            texture [tindex++] = (i + 1) *(1.0f / LEVELS);

        }
    }
    
    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    //glEnableClientState (GL_NORMAL_ARRAY);
    

//    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

//    glColor4f (1.0f, 0.0f, 0.0f, 1.0f);
    
    for (i = 0; i < LEVELS; i++ ) 
    {
        glDrawArrays (GL_TRIANGLE_STRIP, i * 2.0f * (ARCS + 1) , (2.0f *(ARCS + 1)));
    }

    glDisableClientState (GL_VERTEX_ARRAY);
    //glDisableClientState (GL_NORMAL_ARRAY);
    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glDisable (GL_TEXTURE_2D);
}



static void DrawAxis ()
{
    GLfloat axis[]  = {0.0, 0.0, 0.0, 
                       10.0 ,0.0, 0.0, 
                       0.0, 10.0, 0.0, 
                       0.0, 0.0, 10.0};
    GLubyte index[] = {0,1, 0, 2, 0,3};
    glVertexPointer (3, GL_FLOAT, 0, axis);

    glEnableClientState (GL_VERTEX_ARRAY);

    glColor4f (1.0, 1.0, 1.0, 1.0);
    glDrawElements (GL_LINES, 6, GL_UNSIGNED_BYTE, index);

    glDisableClientState (GL_VERTEX_ARRAY);
}

void TextureRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);    
    glLoadIdentity();
    glRotatef (30, 1.0f, 0.0f, 0.0f);
    glRotatef (-45, 0.0f, 1.0f, 0.0f);

    DrawAxis ();
    //DrawTexRect ();
    glRotatef (texangx, 1.0f, 0.0f, 0.0f);
    glRotatef (texangy, 0.0f, 1.0f, 0.0f);
    glRotatef (texangz, 0.0f, 0.0f, 1.0f);
    DrawSphere ();

    if (texangx >= 360.0f)
    {
        texangx = 1.0f;
    }
    else
    {
        texangx += 1.0f;
    }

    if (texangy >= 360.0f)
    {
        texangy = 2.0f;
    }
    else
    {
        texangy += 2.0f;
    }

    if (texangz >= 360.0f)
    {
        texangz = 3.0f;
    }
    else
    {
        texangz += 3.0f;
    }
}


int TextureDemo()
{    
    InitGL_Texture ();
    ReSizeGLScene_Texture(WINDOW_WIDTH, WINDOW_HEIGHT);

    texangx = 0;
    texangy = 0;
    texangz = 0;

    return 1;
}

int TextureDeinit()
{
    return 1;
}