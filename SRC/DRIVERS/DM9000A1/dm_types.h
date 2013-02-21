/**************************************************************************
 * 
 * $Id: dm_types.h,v 1.1.1.1 2007/04/16 03:45:52 bill Exp $
 *
 * File: Types.h
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 *************************************************************************/
 
//#include	"dm_ndis.h"


typedef	unsigned long	U32;
typedef	unsigned long	*PU32;
typedef	unsigned short	U16;
typedef	unsigned short	*PU16;
typedef	unsigned char	U8;
typedef	unsigned char	*PU8;

#ifdef __cplusplus
extern "C" 
{
#include	<ndis.h>
//#include	".\inc\dm9_ndis.h"
}
#endif

#ifndef	__MY_TEYPES_H__
#define	__MY_TEYPES_H__


#if 0
typedef	int		BOOL;
#endif

#define	DIM(a)	(sizeof(a) / sizeof(a[0]))
#define	MINI(a,b)	(((a)<(b)) ? (a):(b))
#define	MAXI(a,b)	(((a)>(b)) ? (a):(b))

#define	MAKE_MASK(a)		MAKE_MASK1(a)
#define	MAKE_MASK1(a)		(1<<(a))
#define	MAKE_MASK2(a,b)		(MAKE_MASK1(a)|MAKE_MASK1(b))
#define	MAKE_MASK3(a,b,c)	(MAKE_MASK2(a,b)|MAKE_MASK1(c))
#define	MAKE_MASK4(a,b,c,d)	(MAKE_MASK2(a,b)|MAKE_MASK2(c,d))
#define	MAKE_MASK6(a,b,c,d,e,f)	\
	(MAKE_MASK4(a,b,c,d)|MAKE_MASK2(e,f))
#define	MAKE_MASK7(a,b,c,d,e,f,g)	\
	(MAKE_MASK4(a,b,c,d)|MAKE_MASK3(e,f,g))
#define	MAKE_MASK8(a,b,c,d,e,f,g,h)	\
	(MAKE_MASK4(a,b,c,d)|MAKE_MASK4(e,f,g,h))

#define	HIGH_BYTE(n)	(((n)&0xFF00)>>8)
#define	LOW_BYTE(n)		((n)&0x00FF)
#define	HIGH_WORD(n)	(((n)&0xFFFF0000)>>16)
#define	LOW_WORD(n)		((n)&0x0000FFFF)

#define	MAKE_WORD(l,h)	( ((h)<<8) | ((l)&0xff) )
#define	MAKE_DWORD(ll,lh,hl,hh)	\
	( (MAKE_WORD(hl,hh) << 16 ) | MAKE_WORD(ll,lh) )
#endif	// of __MY_TEYPES_H__
