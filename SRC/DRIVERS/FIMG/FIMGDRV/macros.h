//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

#if !defined(__FIMG_MACROS_H__)
#define __FIMG_MACROS_H__


/*
 * Standard max/min macros.
 * Beware of using arguments with side-effects.
 */
#define FIMG_MAX(A,B)    ((B)>(A)?(B):(A))
#define FIMG_MIN(A,B)    ((B)<(A)?(B):(A))

/*
 * Enumerated type value check.
 * Checks that the enumerated type has one of a set of values.
 * valueset should be of the form: (1 << VAL1) | (1 << VAL2) | ...
 * (Max 32 values.  Enums must be zero-based.)
 */
#define FGL_ENUM_IN_SET(value, valueset)            \
        ( ((1 << (unsigned int)(value)) & (valueset)) != 0 )


/* Binary play to detect powers of two in four operations */
#define FGL_IS_POWER_OF_TWO(X)    ( ( (~(X)) & ((X)-1) ) == ((X)-1) )

/* Detect path separator character */
#define FGL_IS_FILE_SEPARATOR(character)           \
        ( ((character) == '\\') || ((character) == '/') )



/*
 * Bitfield validator.
 * Checks that the set bitfield flags are acceptable.
 * Validator should have all acceptable bits set.
 */
#define FGL_VALID_BITFIELD(value, validator)        \
        ( ((value) & (validator)) == (value) )

#define FGL_BITFIELD_MASK(fields) ((0xFFFFFFFF>>(31-(1?fields))) & (0xFFFFFFFF<<(0?fields)))

/* Extract a bitfield */
/* Use with 2nd arg as colon separated bitfield defn - no brackets round 2nd arg */
/* e.g. FGL_EXTRACT_BITFIELD(0x00003c00, 11:8) == c */
#define FGL_GET_BITFIELD(flags, fields) (((flags)&FGL_BITFIELD_MASK(fields))>>(0?fields))


/* Set a bitfield */
/* E.g. FGL_SET_BITFIELD(flags[=0 initially], 13:8, 0xf) gives 0x00000f00  */
#define FGL_SET_BITFIELD(flags, fields, value)                            \
        (flags) &= ~FGL_BITFIELD_MASK(fields);                            \
        (flags) |= (((value)<<(0?fields)) & FGL_BITFIELD_MASK(fields));

/* Use this for object tracking */
#define FGL_UNREFERENCED_PARAMETER(param)    if(param);

#endif    /* __FIMG_MACROS_H__ */
