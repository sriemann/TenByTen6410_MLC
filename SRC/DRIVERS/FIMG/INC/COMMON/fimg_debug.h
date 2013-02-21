//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

#ifndef _FIMG_DEBUG_H_
#define _FIMG_DEBUG_H_


#ifdef __cplusplus
extern "C" {
#endif

// Debug zones
// Even on Release Mode, We don't care about GPE message
#define ZONEID_FATAL                0
#define ZONEID_ERROR                1
#define ZONEID_WARNING              2
#define ZONEID_MESSAGE              3
#define ZONEID_VERBOSE              4
#define ZONEID_CALLTRACE            5
#define ZONEID_ALLOC                6
#define ZONEID_FLIP                 7

#define ZONEMASK_FATAL         (1 << ZONEID_FATAL)
#define ZONEMASK_ERROR         (1 << ZONEID_ERROR)
#define ZONEMASK_WARNING       (1 << ZONEID_WARNING)
#define ZONEMASK_MESSAGE       (1 << ZONEID_MESSAGE)
#define ZONEMASK_VERBOSE       (1 << ZONEID_VERBOSE)
#define ZONEMASK_CALLTRACE     (1 << ZONEID_CALLTRACE)
#define ZONEMASK_ALLOC         (1 << ZONEID_ALLOC)
#define ZONEMASK_FLIP          (1 << ZONEID_FLIP)

#define FIMG_DBG_FATAL          ZONEID_FATAL,__FILE__, __LINE__
#define FIMG_DBG_ERROR          ZONEID_ERROR,__FILE__, __LINE__
#define FIMG_DBG_WARNING        ZONEID_WARNING,__FILE__, __LINE__
#define FIMG_DBG_MESSAGE        ZONEID_MESSAGE,__FILE__, __LINE__
#define FIMG_DBG_VERBOSE        ZONEID_VERBOSE,__FILE__, __LINE__
#define FIMG_DBG_CALLTRACE      ZONEID_CALLTRACE,__FILE__, __LINE__
#define FIMG_DBG_ALLOC          ZONEID_ALLOC,__FILE__, __LINE__
#define FIMG_DBG_FLIP           ZONEID_FLIP,__FILE__, __LINE__


void GPIDebugPrintf(unsigned int ui32DebugLevel,
                                    const char *pszFileName,
                                    unsigned int ui32Line,
                                    const char *pszFormat,
                                    ...);


#define DEFAULT_MSG_LEVEL        (ZONEMASK_FATAL|ZONEMASK_ERROR|ZONEMASK_WARNING)//|ZONEMASK_MESSAGE|ZONEMASK_VERBOSE)

#define FIMG_MAX_DEBUG_MESSAGE_LEN    (512)
#define FIMG_PRINT(X)        GPIDebugPrintf X


#ifdef _WIN32_WCE
#include "windows.h"

extern DBGPARAM dpCurSettings;

#endif //_WIN32_WCE


#ifdef __cplusplus
}
#endif

#endif //_FIMG_DEBUG_H_

