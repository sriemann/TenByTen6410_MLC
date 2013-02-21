#ifdef	_SHOW_
#define	_VERBOSE_	1
#elif !defined(_VERBOSE_)
#define	_VERBOSE_	0
#endif

#if   defined(__BORLANDC__)	/* show what compiler we used */
#define	COMPILER_ID "Borland"
#define	LITTLE_ENDIAN	1
#elif defined(_MSC_VER)
#define	COMPILER_ID "Microsoft"
#define	LITTLE_ENDIAN	1
#elif defined(__GNUC__)
#define	COMPILER_ID "GNU"
#ifndef LITTLE_ENDIAN		/* assume gcc = little-endian, unless told otherwise */
#define	BIG_ENDIAN	1
#endif
#else				/* assume big endian, if compiler is unknown */
#define	COMPILER_ID "Unknown"
#endif

/* 1. Standard types for AES cryptography source code               */

typedef unsigned char   u08b; /* an 8 bit unsigned character type */
typedef unsigned short  u16b; /* a 16 bit unsigned integer type   */
typedef unsigned long   u32b; /* a 32 bit unsigned integer type   */

/* 2. Standard interface for AES cryptographic routines             */

/* These are all based on 32-bit unsigned values and will therefore */
/* require endian conversions for big-endian architectures          */

#ifdef  __cplusplus
extern "C"
{
#endif
    u32b *  AES_SetKey (const u32b in_key[ ], const u32b key_len);
#if 1
    void    AES_Encrypt(const u32b in_blk[4], u32b *out_blk);
    void    AES_Decrypt(const u32b in_blk[4], u32b *out_blk);
#else
    void    AES_Encrypt(const u32b in_blk[4], u32b out_blk[4]);
    void    AES_Decrypt(const u32b in_blk[4], u32b out_blk[4]);
#endif
#ifdef  __cplusplus
};
#endif

/* 3. Basic macros for speeding up generic operations               */

/* Circular rotate of 32 bit values                                 */

#ifdef _MSC_VER

#include <stdlib.h>
#pragma intrinsic(_lrotr,_lrotl)
#define rotr(x,n) _lrotr(x,n)
#define rotl(x,n) _lrotl(x,n)

#else

#define rotr(x,n)   (((x) >> ((int)(n))) | ((x) << (32 - (int)(n))))
#define rotl(x,n)   (((x) << ((int)(n))) | ((x) >> (32 - (int)(n))))

#endif

/* Extract byte from a 32 bit quantity (little endian notation)     */ 

#define byte(x,n)   ((u08b)((x) >> (8 * n)))

/* For inverting byte order in input/output 32 bit words, if needed  */

#ifdef  LITTLE_ENDIAN
#define bswap(x)    (x)
#else
#define bswap(x)    (rotl((x), 8) & 0x00ff00ff | rotr((x), 8) & 0xff00ff00)
#endif
