/********************************************************************************
*                                                                                *
* File Name   :   SsbTypeDefs.h                                                    *
* Description :   Coding Rule for System LSI Home Platform Team                    *
* Reference Docment : System LSI Home Platform Team Coding Guideline            *
* Revision History :                                                            *
*      Date             Author             Detail description                    *
*  ------------    ----------------   ------------------------------            *
*   May 16, 2003     Brian JH Kim         Created                                *
*                                                                                *
*********************************************************************************/

#ifndef  SSBTYPEDEFS_H
#define  SSBTYPEDEFS_H

/* Include files */

#ifdef __cplusplus
extern "C"
{
#endif

/* Struct/Union Types and define */

/*************************************************************************/
/*          System LSI Home Platform Team Typdef Standard                */
/*************************************************************************/
/*     Types               Type Define    Prefix Examples       Bytes    */
/*-----------------------  -----------    ------ --------------  ---     */
typedef char                   Int8   ;/*    b   Int8    bName    1      */
typedef char                *  pInt8  ;/*   pb   pInt8   pbNmae   1      */
typedef unsigned char          Uint8  ;/*   ub   Uint8   ubCount  1      */
typedef unsigned char       *  pUint8 ;/*  pub   pUint8  pubCount 1      */
typedef short int              Int16  ;/*    s   Int16   sCount   2      */
typedef short int           *  pInt16 ;/*   ps   pInt16  psCount  2      */
typedef unsigned short int     Uint16 ;/*   us   Uint16  usCount  2      */  
typedef unsigned short int  *  pUint16;/*  pus  pUint16 pusCount  2      */  
typedef int                    Int32  ;/*    i   Int32   iCount   4      */
typedef int                 *  pInt32 ;/*   pi   pInt32  piCount  4      */
typedef unsigned int           Uint32 ;/*   ui   Uint32  uiCount  4      */
typedef unsigned int        *  pUint32;/*  pui   pUnit32 puiCount 4      */
typedef float                  Float  ;/*    f   Float   fCount   4      */
typedef float               *  pFloat ;/*   pf   pFloat  pfCount  4      */
typedef double                 Uint64 ;/*    d   Uint64  dCount   8      */
typedef double              *  pUint64;/*   pd   pUint64 pdCount  8      */
typedef char                   Bool   ;/* cond   Bool    condTrue 8 T/F  */
typedef void                   Void   ;/*    v   Void    vFlag    4      */
typedef void                *  pVoid  ;/*   pv   pVoid   pvFlag   4      */

/*************************************************************************/
/*                   System Specific Standard                            */
/*************************************************************************/
#define EXPORT      extern
#define STATIC      static
#define ERRORCODE   Int32    

#ifdef __cplusplus
}
#endif

#endif  /* SSBTYPEDEFS_H */