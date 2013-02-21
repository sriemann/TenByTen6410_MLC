// SsbSipVideoDivXpostprocess.h //

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

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * John Funnell
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/

// Currently this contains only the deblocking filter.  The vertial   
// deblocking filter operates over eight pixel-wide columns at once.  
// The  horizontal deblocking filter works on four horizontals row at a time. 

/* Picture height must be multiple of 8, width a multiple of 16 */

/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPPOSTPROCESS_H__
#define ___SSBSIPPOSTPROCESS_H__



#include "SsbSipVideoDivXportab.h"


/**** Compile-time options ****/

/* the following parameters allow for some tuning of the postprocessor */
#define DEBLOCK_HORIZ_USEDC_THR    (28 -  8)
#define DEBLOCK_VERT_USEDC_THR     (56 - 16)

/* SHOWDECISIONS(_H/_V) enables you to see where the deblocking filter has used DC filtering (black) and default filtering (white) */
//#define SHOWDECISIONS_H
//#define SHOWDECISIONS_V

/* When defined, PP_SELF_CHECK causes the postfilter to double check every */
/* computation it makes.  For development use. */
//#define PP_SELF_CHECK

/* Type to use for QP. This may depend on the decoder's QP store implementation */
//#define TSINGHUA

#define QP_STORE_T char

#ifdef TSINGHUA
#define QP_STORE_T int16_t
#endif

/*** SsbSipVideoDivXdecore parameter mask ***/
#define PP_DEBLOCK_Y_H_MASK        0x00ff0000
#define PP_DEBLOCK_Y_V_MASK        0x0000ff00
#define PP_DERING_Y_MASK        0x000000ff



//    Function Pointer Prototypes
    
typedef void (SsbSipVideoDivXpostprocessProc)(
    unsigned char * src[], int src_stride,
    unsigned char * dst[], int dst_stride, 
    int horizontal_size,   int vertical_size, 
    QP_STORE_T *QP_store,  int QP_stride, int mode, 
    int xpos, int ypos);

typedef SsbSipVideoDivXpostprocessProc* SsbSipVideoDivXpostprocessProcPtr;

EXPORT SsbSipVideoDivXpostprocessProcPtr SsbSipVideoDivXpostprocess;    
EXPORT SsbSipVideoDivXpostprocessProc SsbSipVideoDivXpostprocess_generic;

/**** mode flags to control postprocessing actions ****/
#define PP_DEBLOCK_Y_H  0x00000001  /* Luma horizontal deblocking   */
#define PP_DEBLOCK_Y_V  0x00000002  /* Luma vertical deblocking     */
#define PP_DEBLOCK_C_H  0x00000004  /* Chroma horizontal deblocking */
#define PP_DEBLOCK_C_V  0x00000008  /* Chroma vertical deblocking   */
#define PP_DERING_Y     0x00000010  /* Luma deringing           */
#define PP_DERING_C     0x00000020  /* Chroma deringing         */
#define PP_DONT_COPY    0x10000000  /* Postprocessor will not copy src -> dst */
                    /* instead, it will operate on dst only   */

#endif
