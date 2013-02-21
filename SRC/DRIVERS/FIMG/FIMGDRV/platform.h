//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/
/*
***************************************************************************//*!
*
* \file        platform.h
* \brief    Platform specific functionality
*
*//*---------------------------------------------------------------------------
* NOTES:
*
*//*---------------------------------------------------------------------------
* HISTORY:
*
*******************************************************************************
*/

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

/*
*******************************************************************************
* Includes
*******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>
//#include <stdarg.h>

/*
*******************************************************************************
* Macro definitions and enumerations
*******************************************************************************
*/

//#define SHARED_CONTEXT_DEBUG

#define thread_debug 0
    

/*
*******************************************************************************
* Type, Structure & Class Definitions
*******************************************************************************
*/

typedef unsigned int size_t;

#if 0 //Bhushan
#define gAssert assert
#else
#define gAssert( _expr ) (void)( (_expr) ? 1 : (printf(">>ASSERTION FAILED: %s \n",#_expr)))
#endif

extern "C" void _gl_asm_memcpy_3l(unsigned int *dst, unsigned int *src, size_t num);
extern "C" void _gl_asm_memcpy_4l(unsigned int *dst, unsigned int *src, size_t num);


//! Platform class encapsulating all platform specific functions
class Plat {

public:
    template<typename T>
    static T min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }

    template<typename T>
    static T max(const T& a, const T&b)
    {
        return a > b ? a : b;
    }
    template <typename T>
    static T clamp(const T& x, const T& a, const T& b)
    {
        return Plat::min(Plat::max(a,x), b);
    }

    inline static void* malloc (size_t size )
    {
        return ::malloc(size);
    }

    template <typename T>
    inline static void safe_free (T& ptr)
    {
        if(ptr) {
            ::free((void*)(ptr));
            ptr = 0;
        }
    }

    inline static void* memset (void* dst, int c, size_t num)
    {
        return ::memset(dst, c, num);
    }


    inline static void* memcpy (void* dst, const void* src, size_t num)
    {
        return ::memcpy(dst, src, num);
    }

    inline static void memcpy_3l (void* dst, const void* src, size_t num)
    {
        _gl_asm_memcpy_3l((unsigned int *)dst,(unsigned int *)src,num);
    }

    inline static void memcpy_4l (void* dst, const void* src, size_t num)
    {
        _gl_asm_memcpy_4l((unsigned int *)dst,(unsigned int *)src,num);
    }

    inline static void* memcpy_FIMG (void* dst, const void* src, size_t num)
    {
#ifndef FIMG_VERA_SIM
        return ::memcpy(dst, src, num);
#else
        return memcpy_RTLSIM(dst, src, num);
#endif
    }

    inline static int memcmp(const void* buf1, const void* buf2, size_t size)
    {
        return ::memcmp(buf1, buf2, size);
    }

    inline static int printf(const char* format, ...)
    {
/*        va_list args;
        va_start(args, format);
        int res = 0;
        res = vprintf(format, args);
        va_end(args);
*/
        return TRUE;
    }

    static void memset_long(void* dst, unsigned int c, size_t num);

    static void *memcpy_RTLSIM(void* dst, const void* src, size_t num);

    inline static void memset_short(void* dst, unsigned short c, size_t num)
    {
        unsigned int val = (c << 16) | c;
    
        if((num%2)==0)
        {
            memset_long(dst, val, num / 2);
        }
        else
        {
            *(unsigned short*)(dst) = c;
            memset_long(((unsigned short*)dst)+1, val, num/2);
        }
        
    }
        
        inline static void memset_short(void* dst, unsigned int c, size_t num, unsigned int mask)
        {
                unsigned int c32 = (c <<16) | c;
                unsigned int mask32 = (mask<<16)|mask;
                
                if((num%2)==0)
                {
                        memset_long(dst, c32, num/2, mask32);
                }
                else
                {
                        *(unsigned short*)(dst) = (c & mask) | ((*(unsigned short*)(dst)) & ~mask);
                        memset_long(((unsigned short*)dst)+1, c32, num/2,mask32);
                }
        }

    static void memset_long(void* dst, unsigned int c, size_t num, unsigned int mask);

    static void lock( CRITICAL_SECTION* mutex , const char * func_name);

    static void unlock( CRITICAL_SECTION* mutex , const char * func_name);

    static void  initMutex(  CRITICAL_SECTION* mutex  ,  const char * func_name);
    
    static void  DeinitMutex(  CRITICAL_SECTION* mutex  ,  const char * func_name);
};


/*
*******************************************************************************
* Global Variable Declarations
*******************************************************************************
*/
extern CRITICAL_SECTION gles20_fimg_mutex;
extern CRITICAL_SECTION gles20_chunkalloc_mutex;
extern CRITICAL_SECTION gles20_open_mutex;
/*
*******************************************************************************
* Function Declarations
*******************************************************************************
*/

#endif /*__PLATFORM_H__*/

