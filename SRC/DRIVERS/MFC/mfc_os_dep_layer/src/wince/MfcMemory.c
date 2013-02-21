//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include <bsp.h>
#include "LogMsg.h"

void *Phy2Vir_AddrMapping(unsigned int phy_addr, int mem_size, BOOL CacheEnable)
{
    void *mappedAddr = NULL;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    ioPhysicalBase.LowPart = phy_addr;
    mappedAddr = MmMapIoSpace(ioPhysicalBase, mem_size, CacheEnable);
    if (mappedAddr == NULL)
    {
        RETAILMSG(1, (L"[MFC:ERR] %s : Mapping Failed [PA:0x%08x]\n\r", __FUNCTION__, phy_addr));
    }

    return mappedAddr;
}

void *Mem_Alloc(unsigned int size)
{
    void    *alloc_mem;

    alloc_mem = (void *) malloc(size);
    if (alloc_mem == NULL) {
        LOG_MSG(LOG_ERROR, "Mem_Alloc", "memory allocation failed!\r\n");
        return NULL;
    }

    return alloc_mem;
}

void Mem_Free(void *addr)
{
    free(addr);
}

void *Mem_Cpy(void *dst, const void *src, int size)
{
    return memcpy(dst, src, size);
}

int   Mem_Set(void *dst, int value, int size)
{
    memset(dst, value, size);

    return value;
}

int Copy_From_User(void *to, const void *from, unsigned long n)
{
    return (int)memcpy(to, from, n);
}

int Copy_To_User(void *to, const void *from, unsigned long n)
{
    return (int)memcpy(to, from, n);
}
