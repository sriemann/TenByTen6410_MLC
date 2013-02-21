//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

 
#ifndef __CMM_MISC_H__
#define __CMM_MISC_H__

typedef    unsigned char    UCHAR;
typedef unsigned long    ULONG;
typedef unsigned long    u32;
typedef    unsigned int    UINT;


HANDLE CreateCMMmutex(void);
DWORD LockCMMMutex(void);
DWORD UnlockCMMMutex(void);
void DeleteCMMMutex(void);
void printD(char* fmt, ...);

#endif