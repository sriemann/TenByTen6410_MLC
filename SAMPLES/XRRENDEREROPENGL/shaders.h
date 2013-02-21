//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//


// Order of fields passed to the Vertex and Fragment Shaders
#define VERTEX_POSITION 0
#define VERTEX_DIFFUSE  1
#define TEXTURE_COORD   2

//------------------------------------------------------------------------
//
//  Struct:  PAL vertex definition
// 
//  Synopsis:
//      DirectX/OpenGL abstraction
//
//------------------------------------------------------------------------
struct CPALVertex
{
    float x, y, z;            // z allows perspective texturing
    unsigned long dwDiffuse;  // Multiplies with texture and can be used for opacity
                              // effects and solid color draws
    float u0, v0;             // Texture coordinates stage 0
    float u1, v1;             // Texture coordinates stage 1
};


///////////////////////////
//
// Default Shaders: handle transform, opacity, color conversion
//

#define GL_FRAGMENT_SHADER_RT                  0x8B30
#define GL_VERTEX_SHADER_RT                    0x8B31
