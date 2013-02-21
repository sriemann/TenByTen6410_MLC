//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

#ifndef _PROFILER_H_
#define _PROFILER_H_
#include <time.h>

clock_t startTime;

typedef struct 
{
    clock_t startTime;                    // Start time of the timer
    float timeDiffinSec;                // Time difference between start and end time
}Timer;

void getStartTime (Timer *t);
void getTimeDiff (Timer *t);


#endif //_PROFILER_H_