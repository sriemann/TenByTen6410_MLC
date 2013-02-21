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
#include <windows.h>


//
// This class is included as a debugging convenience for developers.
// It is not #included by the shipping OpenGL plugin.
//
// If the BSP supports a high-resolution timer, PerfTimer has sub-millisecond accuracy.
// If the BSP does not support a high-resolution timer, PerfTimer has millisecond accuracy +/- 1 millisecond
// [ the same as GetTickCount() ].
//
//
// #include "PerfTimer.hpp"
//
// void COpenGLDevice::SomeMethod()
// {
//     PerfTimer timer;
//     timer.Start();
//     ... perform an expensive operation ...
//     timer.Stop();
//     printf( "duration = %2.2f ms\n", timer.GetDurationInMilliSecs() );
// }

class PerfTimer 
{    
public:

    void Start(void) 
    {        
        QueryPerformanceCounter(&mTimeStart);    
    };    
    
    void Stop(void) {        
        QueryPerformanceCounter(&mTimeStop);    
    };    
    
    double GetDurationInSecs(void)    
    {        
        LARGE_INTEGER freq;        
        QueryPerformanceFrequency(&freq);        
        double duration = (double)(mTimeStop.QuadPart-mTimeStart.QuadPart)/(double)freq.QuadPart;        
        return duration;    
    }    
    
    double GetDurationInMilliSecs(void)    
    {        
        LARGE_INTEGER freq;        
        QueryPerformanceFrequency(&freq);        
        double duration = (double)(mTimeStop.QuadPart-mTimeStart.QuadPart)/((double)freq.QuadPart/1000);
        return duration;    
    }    

    LARGE_INTEGER mTimeStart;    
    LARGE_INTEGER mTimeStop;
};
