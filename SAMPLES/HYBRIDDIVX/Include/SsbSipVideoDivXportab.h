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

/** $Id: SsbSipVideoDivXportab.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  
 * Copyright (C) 2001 - DivXNetworks
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// SsbSipVideoDivXportab.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPPORTAB_H__
#define ___SSBSIPPORTAB_H__
#define EXPORT      extern
#define STATIC      static

#ifdef WIN32

/* basic types
*/
#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#define int8_t char
#define uint8_t unsigned char
#define int16_t short
#define uint16_t unsigned short
#define int32_t int
#define uint32_t unsigned int 
#define int64_t __int64
#define uint64_t unsigned __int64

#define SU_MALLOC_ALIGN_SIZE    32

#elif defined (TARGET_PS2)

#define int8_t char
#define uint8_t unsigned char
#define int16_t short
#define uint16_t unsigned short
#define int32_t int
#define uint32_t unsigned int 
#define    int64_t signed long        
#define    uint64_t unsigned long

#define SU_MALLOC_ALIGN_SIZE    32

#elif defined (ARCH_BSP_V60)

#include <inttypes.h>
#define memcpy unaligned_memcpy
#define SU_MALLOC_ALIGN_SIZE    128

#elif defined (ARCH_TI_C6000)

#define __inline
#define SU_MALLOC_ALIGN_SIZE    64

#define int8_t char
#define __int8_t_defined
#define uint8_t unsigned char
#define int16_t short
#define uint16_t unsigned short
#define int32_t int
#define uint32_t unsigned int 
#define    int64_t double
#define    uint64_t double

#elif defined (__CYGWIN__)
#include "sys/types.h"
#define SU_MALLOC_ALIGN_SIZE    128
#elif WINCE
#define int8_t char
#define uint8_t unsigned char
#define int16_t short
#define uint16_t unsigned short
#define int32_t int
#define uint32_t unsigned int 
#define int64_t __int64
#define uint64_t unsigned __int64

#define SU_MALLOC_ALIGN_SIZE    32
#else // WIN32

#include <inttypes.h>
#define SU_MALLOC_ALIGN_SIZE    128
#endif 

#endif // ___SSBSIPPORTAB_H__
