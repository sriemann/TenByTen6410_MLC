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

/** $Id: debug.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Andrea Graziani
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// debug.h //

/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/
#ifndef ___SSBSIPDEBUG_H__
#define ___SSBSIPDEBUG_H__
#include "SsbSipVideoDivXportab.h"
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _DEBUG

EXPORT void _SetPrintCond(
    int picnum_start, int picnum_end, 
    int mba_start, int mba_end);
EXPORT void _OpenTrace();
EXPORT void _CloseTrace();
EXPORT void _Print(const char * format, ...);
EXPORT void _Break(int picnum, int mba);
EXPORT void _Error(const char * format, ...);

#else

#define _SetPrintCond(a, b, c, d) 
#define _OpenTrace()
#define _CloseTrace()
#define _Break(a, b) 
#define _Print  
#define _Error 

#endif // _DEBUG
/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/

enum mc_mode
{
    null,
    hor_hpel,
    ver_hpel,
    horver_hpel,
    hor_qpel,
    ver_qpel,
    horver_qpel
};

typedef struct 
{
    char * mc_block_mode;
    int width;
    int height;
    int stride;
} DEBUG_INFO;


#ifdef __cplusplus
extern "C"
}
#endif

#endif 
