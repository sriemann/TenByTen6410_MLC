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


precision highp float;
uniform    sampler2D   inTexture1;
varying    vec2        textureCoordinate;
varying    vec4        diffuse;

void main(void)
{
   // XamlRuntime uses the diffuse color's Alpha channel to alter the opacity of textures
   gl_FragColor = diffuse * texture2D(inTexture1, textureCoordinate);
}
