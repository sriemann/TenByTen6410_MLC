//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "LogMsg.h"

static const LOG_LEVEL log_level = LOG_ERROR;

static const char *modulename = "MFC_DRV";

static const char *level_str[] = {"TRACE", "WARNING", "ERROR"};


void LOG_MSG(LOG_LEVEL level, const char *func_name, const char *msg, ...)
{
    char buf[256] = {0,};
    va_list argptr;
    LPWSTR wsStr;
    int alen;
    int wlen;

    if (level < log_level)
        return;

    sprintf(buf, "[%s: %s] %s: ", modulename, level_str[level], func_name);

    va_start(argptr, msg);
    vsprintf(buf + strlen(buf), msg, argptr);
    alen = strlen(buf);
    wlen = MultiByteToWideChar(CP_ACP, 0, buf, alen, 0, 0);
    wsStr = (LPWSTR)calloc(wlen+1, sizeof(TCHAR));
    MultiByteToWideChar(CP_ACP, 0, buf, alen, wsStr, wlen);
    wsStr[wlen] = 0;
    NKDbgPrintfW(wsStr);
    va_end(argptr);
    free(wsStr);
}
