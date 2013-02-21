/**
 ** Copyright (C) 2002 DivXNetworks, all rights reserved.
 **
 ** DivXNetworks, Inc. Proprietary & Confidential
 **
 ** This source code and the algorithms implemented therein constitute
 ** confidential information and may comprise trade secrets of DivXNetworks
 ** or its associates, and any use thereof is subject to the terms and
 ** conditions of the Non-Disclosure Agreement pursuant to which this
 ** source code was originally received.
 **
 **/

/** $Id: SsbSipVideoDivXtypes_311.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
 *
 * Utility functions for parsing bitstream DivX ;-) 3.11 format
 *
 *  Copyright (C) 2001 - DivXNetworks
 *
 * Eugene Kuznetsov
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// SsbSipVideoDivXtypes_311.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPTYPES_311_H__
#define ___SSBSIPTYPES_311_H__
/*
// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
*/

#include "SsbSipVideoDivXdecore.h"
#include "SsbSipVideoDivXmp4_vld.h"
#include "SsbSipVideoDivXmp4_vars.h"

#include "SsbSipVideoDivXgetbits.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/
#define VLD_TABLE_DIM 6
struct item;
struct mv_item;

#ifdef WIN32
#pragma warning ( disable : 4514 ) // unreferenced inline function has been removed
#pragma warning ( disable : 4127 ) // conditional expression is constant
#endif

#ifndef WIN32
#define __forceinline __inline
#endif

typedef struct item item_t;
typedef struct mv_item mv_item_t;

struct ShortVector
{
    short x;
    short y;
};

struct item
{
    event_t value;
    char length;
};

struct mv_item
{
    struct ShortVector value;
    short length;
};

STATIC __forceinline event_t StaDivXGetEvent311(MP4_STREAM * ld, const item_t *items, const int* indices)
{
    short dimension = VLD_TABLE_DIM;
    while(1)
    {
    unsigned int val = indices[StaDivXShowBits(ld, dimension)];
    if(!(val & 0xFFFF0000))
    {
        StaDivXFlushBits(ld, items[val].length);
        return items[val].value;
    }
    StaDivXFlushBits(ld, dimension);
    dimension = val >> 16;
    indices += (val & 0xFFFF);
   }
}

STATIC __forceinline short StaDivXGetShort311(MP4_STREAM * ld, const int* indices)
{
    short dimension = VLD_TABLE_DIM;
    while(1)
    {
    unsigned int val = indices[StaDivXShowBits(ld, dimension)];
    if(!(val & 0xFFFF0000))
    {
        StaDivXFlushBits(ld, val >> 10);
        return val & 0x3FF;
    }
    StaDivXFlushBits(ld, dimension);
    dimension = val >> 16;
    indices += (val & 0xFFFF);
    }
}

STATIC __forceinline const struct ShortVector* StaDivXGetMotionVector311(MP4_STREAM * ld, const mv_item_t* items, const int* indices)
{
    short dimension = VLD_TABLE_DIM;
    while(1)
    {
    unsigned int val = indices[StaDivXShowBits(ld, dimension)];
    if(!(val & 0xFFFF0000))
    {
        StaDivXFlushBits(ld, items[val].length);
        return &items[val].value;
    }
    StaDivXFlushBits(ld, dimension);
    dimension = val >> 16;
    indices += (val & 0xFFFF);
    }
}

#ifdef __cplusplus
extern "C"
}
#endif


#endif
