/*$Id: SsbTypeDefs.h, v.3.0 2005/12/09 Exp $*/
/*
//----------------------------------------------------------------------------
//                    Samsung Multi-Format Video Codec
//                      Samsung Electronics Co., LTD.
//                          COPYRIGHT (C) 2005
//
// This  program  is  proprietary  to  Samsung Electronics Co., LTD.,and is 
// protected under International Copyright Act as an unpublished work.  Its 
// use and disclosure  is  limited by the terms and conditions of a license 
// agreement. It may not be copied or otherwise  reproduced or disclosed to 
// persons  outside  the  licensee's organization except in accordance with 
// the  terms  and  conditions  of  such  an  agreement.   All  copies  and 
// reproductions shall be the property of Samsung Electronics Co., LTD. and
// must bear this  notice in its entirety.
//----------------------------------------------------------------------------
*/

/*
//----------------------------------------------------------------------------
//
// File Name   :   SsbTypeDefs.h
//
// Description :   The data type defined here will be used for all 
//                 SW code development for MultiFormat Video Codec Project.
//
// Reference Docment : System LSI System Software Lab. Coding Guideline
//                     (TN_SWL_GDL_CodingGuideline_030509)
//
// Revision History :
//        Date               Author             Detail description
//  ----------------    ----------------   ------------------------------
//   March 01, 2005         Mickey Kim          Created
//   July  26, 2005         Mickey Kim          Modified for MFC project 
//   Dec   09, 2005         Muckey Kim          Add 64bit float and integer type
//
//----------------------------------------------------------------------------
*/

#ifndef  SSBTYPEDEFS_H
#define  SSBTYPEDEFS_H

/*
// ---------------------------------------------------------------------------
// Include files
// ---------------------------------------------------------------------------
*/
// ---------------------------- NONE -----------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

/*
// ---------------------------------------------------------------------------
// Struct/Union Types and define
// ---------------------------------------------------------------------------
*/

//
// The Endian should be adjusted after the H/W configuration is fixed.
//
#define  __Ssb_STARCORE_BigEndian__
#undef   __Ssb_STARCORE_LittleEndian__

#define  __Ssb_CALM_BigEndian__
#undef   __Ssb_CALM_LittleEndian__

#define  __Ssb_ARM_LittleEndian__
#undef   __Ssb_ARM_BigEndian__


/*************************************************************************/
/*             Multim-Format Video Codec Typdef Standard                 */
/*************************************************************************/

/*----------------------------- ---------  -------- ------------ -----   */
/*          Types                NewType    Prefix    Examples   Bytes   */
/*----------------------------- ---------  -------- ------------ -----   */
#ifdef _STARCORE_
typedef   signed       char        Int8; /*    b       bName       1     */
typedef   signed       char *     pInt8; /*   pb       pbName      1     */
#else
typedef                char        Int8; /*    b       bName       1     */
typedef                char *     pInt8; /*   pb       pbName      1     */
#endif

typedef   signed       char       SInt8; /*    b       bName       1     */

typedef unsigned       char       UInt8; /*   ub       ubCnt       1     */
typedef unsigned       char *    pUInt8; /*  pub       pubCnt      1     */

typedef          short int        Int16; /*    s       sCnt        2     */
typedef          short int *     pInt16; /*   ps       psCnt       2     */
typedef unsigned short int       UInt16; /*   us       usCnt       2     */
typedef unsigned short int *    pUInt16; /*  pus       pusCnt      2     */

typedef                int        Int32; /*    i       iCnt        4     */
typedef                int *     pInt32; /*   pi       piCnt       4     */
typedef unsigned       int       UInt32; /*   ui       uiCnt       4     */
typedef unsigned       int *    pUInt32; /*  pui       puiCnt      4     */

typedef              float        Float; /*    f       fCnt        4     */
typedef              float *     pFloat; /*   pf       pfCnt       4     */

typedef             double      Float64; /*    d       dCnt        8     */
typedef             double *   pFloat64; /*   pd       pdCnt       8     */

#ifdef _STARCORE_
typedef          long long        Int64; /*    l       lCnt        8     */
typedef          long long *     pInt64; /*   pl       plCnt       8     */
#else
typedef            __int64        Int64; /*    l       lCnt        8     */
typedef            __int64 *     pInt64; /*   pl       plCnt       8     */
#endif


typedef                char        Bool; /* cond       condIsTrue  1     */
typedef                void        Void; /*    v       vFlag       4     */
typedef                void *     pVoid; /*   pv       pvFlag      4     */


/*************************************************************************/
/*                   System Specific Standard                            */
/*************************************************************************/
typedef UInt32      ERRORCODE;  /* Error Code Define */

#define EXPORT const       extern
#define STATIC      static

#ifdef __cplusplus
}
#endif

#endif  /* SSBTYPEDEFS_H */
