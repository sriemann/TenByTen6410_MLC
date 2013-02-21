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
* \file        platform.cpp
* \brief    Platform specific functionality
*
*//*---------------------------------------------------------------------------
* NOTES:
*/

/*
*******************************************************************************
* Includes
*******************************************************************************
*/

#include "platform.h"
#include "string.h"
#include "stdlib.h"
#include "register.h"

/*
*******************************************************************************
* Macro definitions and enumerations
*******************************************************************************
*/

/*
*******************************************************************************
* Type, Structure & Class Definitions
*******************************************************************************
*/

/*
*******************************************************************************
* Global Variables
*******************************************************************************
*/
CRITICAL_SECTION    gles20_fimg_mutex;
CRITICAL_SECTION    gles20_chunkalloc_mutex;
CRITICAL_SECTION    gles20_open_mutex;
/*
*******************************************************************************
* Local Function Declarations
*******************************************************************************
*/

/*
*******************************************************************************
* Function Definitions
*******************************************************************************
*/
extern "C" void _gl_asm_memset(unsigned int *_dst, unsigned int c, size_t num);

void Plat::initMutex(  CRITICAL_SECTION*  mutex  ,  const char * func_name)
{
#ifdef MULTI_CONTEXT
        //printf("trying to lock fimg mutex in func: %s\n", func_name);
        InitializeCriticalSection(mutex);
#endif    
}

void  Plat::DeinitMutex(  CRITICAL_SECTION* mutex  ,  const char * func_name)
{
#ifdef MULTI_CONTEXT
        //printf("trying to lock fimg mutex in func: %s\n", func_name);
        DeleteCriticalSection(mutex);
#endif        
}


void Plat::lock( CRITICAL_SECTION*  mutex , const char * func_name)
{
#ifdef MULTI_CONTEXT
        EnterCriticalSection(mutex);        
#endif    

}

void Plat::unlock( CRITICAL_SECTION*  mutex , const char * func_name)
{
#ifdef MULTI_CONTEXT
        LeaveCriticalSection(mutex);
#endif    
}

void
Plat::memset_long(void* _dst, unsigned int c, size_t num)
{
    unsigned int *dst = (unsigned int*)(_dst);

    // Put dst on 8word (32byte) alignment
    while(((unsigned int)dst) % (4*8) != 0) {
        *dst = c;
        dst++; num--;
    }

    int temp = num % 8;
    num = num - temp;
    
    //calling asm function for memset
    _gl_asm_memset(dst, c, num);

    // Copy rest of them
    if(temp != 0)
    {//printf("after _testcpy %d %x %x \n",num,dst,c);
        dst = dst+num;
        while(temp > 0){
            *dst = c;
            dst++; temp--;
        }
    }

}


void
Plat::memset_long(void* _dst, unsigned int _c, size_t num, unsigned int mask)
{
    if(mask == 0)
        return;

    if(mask == 0xFFFFFFFF)
    {
        memset_long(_dst, _c, num);
        return;
    }
    
    unsigned int *dst = (unsigned int*)(_dst);
    unsigned int c = _c & mask;

    while(num > 0)
    {
        *dst = c | (*dst & ~mask);
        dst++; num--;
    }


}

void *
Plat::memcpy_RTLSIM(void* dst, const void* src, size_t num)
{
    for(unsigned int j = 0; j < (num >> 2) ; j++)
    //for(unsigned int j = 0; j < num / 4 ; j++)
    {
        WRITEREG(((unsigned int *)dst + j),
                    ((unsigned int *)src + j));
    }

    return dst;
}

