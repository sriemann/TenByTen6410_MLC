//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include <windows.h>

#include "MfcMutex.h"


#define  MUTEX_TIMEOUT                    5000

static CRITICAL_SECTION cs;

BOOL MFC_Mutex_Create(void)
{
    InitializeCriticalSection(&cs);

    return TRUE;
}

void MFC_Mutex_Delete(void)
{
    DeleteCriticalSection(&cs);
}

BOOL MFC_Mutex_Lock(void)
{
    EnterCriticalSection(&cs);

    return TRUE;
}

BOOL MFC_Mutex_Release(void)
{
    LeaveCriticalSection(&cs);

    return TRUE;
}
