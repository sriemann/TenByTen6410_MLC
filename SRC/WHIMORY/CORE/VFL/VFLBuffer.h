/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : Virtual Flash Layer                                         */
/* NAME    	   : VFL Buffer Management                                       */
/* FILE        : VFLBuffer.h                                                 */
/* PURPOSE     : This file contains routines for managing buffers which      */
/*              whimory uses. 						                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2005 SAMSUNG ELECTRONICS CO., LTD.                */
/*                          ALL RIGHTS RESERVED                              */
/*                                                                           */
/*   Permission is hereby granted to licensees of Samsung Electronics        */
/*   Co., Ltd. products to use or abstract this computer program for the     */
/*   sole purpose of implementing a product based on Samsung                 */
/*   Electronics Co., Ltd. products. No other rights to reproduce, use,      */
/*   or disseminate this computer program, whether in part or in whole,      */
/*   are granted.                                                            */
/*                                                                           */
/*   Samsung Electronics Co., Ltd. makes no representation or warranties     */
/*   with respect to the performance of this computer program, and           */
/*   specifically disclaims any responsibility for any damages,              */
/*   special or consequential, connected with the use of this program.       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* REVISION HISTORY                                                          */
/*                                                                           */
/*   13-JUL-2005 [Jaesung Jung] : separate from vfl.c & reorganize code      */
/*                                                                           */
/*****************************************************************************/

#ifndef _VFL_BUFFER_H_
#define _VFL_BUFFER_H_


/**
 *  Return value of BUF_XXX()
 */
#define BUF_SUCCESS         WMR_RETURN_VALUE(0, 0x0000, 0x0000)
#define BUF_CRITICAL_ERROR  WMR_RETURN_VALUE(1, 0x0001, 0x0000)
#define BUF_NO_FREE_BUFFER  WMR_RETURN_VALUE(1, 0x0002, 0x0000)


/**
 *  Buffer state enum definition
 */
typedef enum {
    BUF_FREE        = 0x10000000,   // the buffer is free
    BUF_ALLOCATED   = 0x10000001,   // the buffer is allocated to some layer
    BUF_WRITTEN     = 0x10000002,   // data in the buffer is being written to flash
    BUF_AUX         = 0x10000003    // this buffer is not managed by VFL
} BUFStat;


/**
 *  Buffer type enum & macro definition
 */
typedef enum {
    BUF_MAIN_AND_SPARE  = 0x10000000,   // to get a buffer which has main & spare
    BUF_MAIN_ONLY       = 0x10000001,   // to get a buffer which has main only
    BUF_SPARE_ONLY      = 0x10000002    // to get a buffer which has spare only
} BUFType;


/**
 *  Buffer structure definition
 */
typedef struct Buf {
    UINT8 *pData;       // buffer area for main area
    UINT8 *pSpare;      // buffer area for spare area
    UINT32 nBitmap;     // valid sector bitmap for pData
                        // +---------------+
                        // + x + x + x + o +    --> 0x8 (1000)
                        // +---------------+
    BUFStat eStatus;    // the status of buffer
    UINT32 nBank;       // the allocated bank of buffer
    UINT8 *pDataBak;    // the original pointer of data buffer
    struct Buf *pNext;  // linked list for pointing the next buffer
    struct Buf *pPrev;  // linked list for pointing the prev buffer
} Buffer;


/**
 *  exported function prototype of VFL buffer manager
 */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32   BUF_Init            (VOID);
Buffer* BUF_Get             (BUFType eType);
VOID    BUF_Reget           (Buffer *pBuf, BUFType eType);
VOID    BUF_Release         (Buffer *pBuf);
VOID    BUF_MarkAsWritten   (Buffer *pBuf);
VOID    BUF_Close           (VOID);


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // !_VFL_BUFFER_H_

