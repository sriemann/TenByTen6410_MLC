//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/


#ifndef __FIMGDRV_H__
#define __FIMGDRV_H__

typedef struct _ALLOCMEM_ITEM {
    void*                   phyAddr;
    void*                   virAddr;
    void*                   virAddrCP;    // mapped for caller 
    void*            memPool;
    struct _ALLOCMEM_ITEM   *next;
} ALLOCMEM_ITEM;

typedef struct _FIMG_CONTEXT {
    ALLOCMEM_ITEM   *allocated_list;
    DWORD       reserved[3];
} FIMG_CONTEXT;

#endif /*__FIMGDRV_H__*/

