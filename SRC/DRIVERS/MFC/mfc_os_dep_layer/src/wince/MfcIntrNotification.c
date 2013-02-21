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

#include "MfcIntrNotification.h"


static HANDLE gMfcDoneEvent = NULL;
static unsigned int  gIntrType = 0;

BOOL CreateInterruptNotification(void)
{
    if (gMfcDoneEvent != NULL)
        return TRUE;

    gMfcDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (gMfcDoneEvent == NULL)
        return FALSE;

    return TRUE;
}

void DeleteInterruptNotification(void)
{
    if (gMfcDoneEvent != NULL)
        CloseHandle(gMfcDoneEvent);

    gMfcDoneEvent = NULL;
}

int SendInterruptNotification(int intr_type)
{
    if (gMfcDoneEvent == NULL)
        return -1;

    SetEvent(gMfcDoneEvent);
    gIntrType = intr_type;

    return 0;
}

int WaitInterruptNotification(void)
{
    DWORD ret;

    if (gMfcDoneEvent == NULL)
        return -1;

    ret = WaitForSingleObject(gMfcDoneEvent, MFC_INTR_NOTI_TIMEOUT);
    if (ret == WAIT_TIMEOUT)
        return MFC_INTR_REASON_INTRNOTI_TIMEOUT;

    return gIntrType;
}
