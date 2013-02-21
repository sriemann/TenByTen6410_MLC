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

#define VERIFYGL(x)               (x);  {if (glGetError() != GL_NO_ERROR){ hr = E_FAIL; goto Error;}}
#define VERIFYEGL(x)              (x);  {if (eglGetError()!= EGL_SUCCESS){ hr = E_FAIL; goto Error;}}


#define SAFE_RELEASE(p)           if ((p)) {(p)->Release(); (p) = NULL;}
#define SAFE_DELETE(p)            if( NULL != p ) { delete p; p = NULL; }
#define SAFE_ARRAYDELETE(p)       if( NULL != p ) { delete [] p; p = NULL; }

#define ARRAY_COUNT(x)            (sizeof(x)/sizeof((x)[0]))
