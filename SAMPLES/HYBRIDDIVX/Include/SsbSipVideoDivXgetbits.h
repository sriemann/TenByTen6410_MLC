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

/** $Id: StaDivXGetBits.h,v 1.1.1.1 2003/04/23 23:24:24 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Eugene Kuznetsov
 * Andrea Graziani
 * Lioner Ulmer
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPGETBITS_H__
#define ___SSBSIPGETBITS_H__

#ifdef WIN32
#include <stdio.h>
#endif
/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/

void SsbSipVideoDivXinitbits (MP4_STREAM * _ld, const unsigned char * stream, int length);
STATIC unsigned int StaDivXBitPos (MP4_STREAM * _ld);
STATIC void StaDivXFlushBits (MP4_STREAM * ld, int n);
STATIC unsigned int StaDivXGetBits (MP4_STREAM * ld, int n);
STATIC unsigned int StaDivXGetBits1(MP4_STREAM * ld);
void SsbSipVideoDivXnext_start_code(MP4_STREAM * _ld, MP4_STATE * _mp4_state);
#ifdef _ARM_ASSEMBLY1_
void SsbLibVideoDivXFlushbits_asm(MP4_STREAM * ld, int n);
unsigned int SsbLibVideoDivXShowbits_asm(MP4_STREAM * ld, int n);
#endif
/***/
#ifndef WIN32
#define __forceinline __inline
#endif

#ifdef WIN32
#pragma warning ( disable : 4514 ) // unreferenced inline function has been removed
#pragma warning ( disable : 4127 ) // conditional expression is constant
#endif

#ifndef _DEBUG
#define printbits(a, b) 
#endif // _DEBUG 




#if (defined(LINUX) && defined(X86))

// Intel 486+ specific instruction
#define _SWAP(a,b)    \
    __asm__ ( "bswapl %0\n" : "=r" (b) : "0"(*(int*)a) )

#elif (defined(LINUX) && defined(ARM__))

// StrongARM assembly
#define _SWAP(a,b) { register unsigned int temp; \
    b = *(int*)(a); \
    __asm__ ( " EOR %0, %1, %2, ROR #16" : "=r" (temp) : "r" (b), "r" (b)); \
    __asm__ ( " BIC %0, %1, #0x00FF0000" : "=r" (temp) : "r" (temp)); \
    __asm__ ( " MOV %0, %1, ROR #8"      : "=r" (b) : "r" (b)); \
    __asm__ ( " EOR %0, %1, %2, LSR #8"  : "=r" (b) : "r" (b), "r" (temp)); \
    }

#elif (defined(LINUX) && defined(INTEL_C))

#define _SWAP(a,b) \
    {    \
    register unsigned int * c = &b;    \
    b=*(int*)a; __asm mov ecx, c __asm mov eax, [ecx] __asm bswap eax __asm mov [ecx], eax    \
    }

#elif defined(_CUSTOM_PROC)

#define _SWAP(a,b) (b=((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3]))

#elif defined(WIN32)

#define _SWAP(a,b) \
    {    \
    register unsigned int * c = &b;    \
    b=*(int*)a; __asm mov ecx, c __asm mov eax, [ecx] __asm bswap eax __asm mov [ecx], eax    \
    }
    
#elif defined(_ARM_ASSEMBLY_)
    #define _SWAP(a,b) (b=((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])) // DEEP CHECK ENDIAN CONVERSION
#else
    #define _SWAP(a,b) (b=((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])) // DEEP CHECK ENDIAN CONVERSION
#endif





#define BUF_SIZE 2048
#define BUF_LIMIT (BUF_SIZE - 8)

/***/


/* advance by n bits */
//STATIC __forceinline void StaDivXFlushBits (MP4_STREAM * ld, int n)
#ifdef _ARM_ASSEMBLY1_
STATIC __forceinline void StaDivXFlushBits (MP4_STREAM * ld, int n)
    {
        SsbLibVideoDivXFlushbits_asm(ld, n);
    }
#else
STATIC __forceinline void StaDivXFlushBits (MP4_STREAM * ld, int n)
    {
#ifndef _DECORE
        printbits(ld, n);
#endif // _DECORE


        ld->bitcnt += n;
        if (ld->bitcnt >= 32) 
        {
            ld->bit_a = ld->bit_b;
            
            _SWAP(ld->rdptr, ld->bit_b);
#ifdef _PLD_OPT_1            
             __asm
             {
                 PLD    [ld->rdptr,#32]    
             }
#endif
            ld->rdptr += 4;
        ld->bitcnt -= 32;
        }
#ifndef _DECORE
        // check if there's still valid stream in the buffer
        if (ld->rdptr >= ld->rdbfr + BUF_LIMIT) 
        {
            StaDivXFilllBfr (ld);
        }
#endif // _DECORE
    }
#endif //_ARM_ASSEMBLY_




/* read n bits */
#ifdef _ARM_ASSEMBLY1_
    STATIC __forceinline unsigned int StaDivXShowBits (MP4_STREAM * ld, int n)
    {
        int bit_a;
        bit_a = SsbLibVideoDivXShowbits_asm(ld, n);
        return bit_a;
    }
#else
    STATIC __forceinline unsigned int StaDivXShowBits (MP4_STREAM * ld, int n)
    // unsigned int StaDivXShowBits (MP4_STREAM * ld, int n)
    {
        int toshift = 32-n;
        int nbit = toshift - ld->bitcnt;
        int bit_a = (ld->bit_a << (ld->bitcnt)) >> (toshift);
        if (nbit >= 0) 
            return bit_a;
        return (bit_a) | (ld->bit_b >> (32 + nbit));
    }
#endif //_ARM_ASSEMBLY_


STATIC __forceinline unsigned int StaDivXShowBits1 (MP4_STREAM * ld)
//unsigned int StaDivXShowBits1 (MP4_STREAM * ld)
{
  if(ld->bit_a & (0x80000000U >> ld->bitcnt))
      return 1;
  else 
      return 0;
}

// returns absolute bis position inside the stream
STATIC __forceinline unsigned int StaDivXBitPos(MP4_STREAM * ld)
{
#ifdef _DECORE
    return 8*(ld->rdptr - ld->startptr) + ld->bitcnt - 64;
#else 
    return 0;
#endif    
}

STATIC __forceinline unsigned int StaDivXGetBits (MP4_STREAM * ld, int n)
//unsigned int StaDivXGetBits (MP4_STREAM * ld, int n)
{
    unsigned int l = StaDivXShowBits (ld, n);
    StaDivXFlushBits (ld, n);
    return l;
}

STATIC __forceinline unsigned int StaDivXGetBits1(MP4_STREAM * ld)
//unsigned int StaDivXGetBits1(MP4_STREAM * ld)
{
  unsigned int l = StaDivXShowBits1 (ld);
  StaDivXFlushBits (ld, 1);
  return l;
}

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/



#define SsbSipVideoDivXflushbits_vld StaDivXFlushBits
#define StaDivXGetBits_vld   StaDivXGetBits
#define StaDivXGetBits1_vld  StaDivXGetBits1


/*
 *
 *
 */

#endif /* _DECORE_GETBITS_H */
