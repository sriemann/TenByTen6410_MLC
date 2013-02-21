// dering.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPDERING_H__
#define ___SSBSIPDERING_H__

#include "SsbSipVideoDivXportab.h"
#include "SsbSipVideoDivXmp4_vars.h"

void SsbSipVideoDivXdering_alloc(DERING_INFO *di, int width, int height);
void SsbSipVideoDivXdering_free(DERING_INFO *di);

typedef void (SsbSipVideoDivXdering_prepare_proc)( DERING_INFO *di, uint8_t *image, int width, int height, 
            int stride, int QP, int xpos, int ypos, int chroma);
typedef SsbSipVideoDivXdering_prepare_proc* SsbSipVideoDivXdering_prepare_proc_ptr;
typedef void (SsbSipVideoDivXderingProc)( DERING_INFO *di, uint8_t *image, int width, int height, int stride, 
        int QP, int xpos, int ypos, int chroma,
        int intra, MotionVector mv, int render); 

typedef SsbSipVideoDivXderingProc* SsbSipVideoDivXderingProcPtr;


EXPORT SsbSipVideoDivXderingProcPtr SsbSipVideoDivXdering;
EXPORT SsbSipVideoDivXderingProc SsbSipVideoDivXdering_generic;
EXPORT SsbSipVideoDivXdering_prepare_proc_ptr SsbSipVideoDivXdering_prepare;
EXPORT SsbSipVideoDivXdering_prepare_proc SsbSipVideoDivXdering_prepare_generic;

#endif // ___SSBSIPDERING_H__

