/*
;******************************************************************************
; File Name             : SLD_DivX_Interface.h
;
; Description           : This file includes the interface for Divx soft codec to
;                            the TCPMP media player.                         
;                         
;                         
;                         
;                         
;                         
;
; Funtions              : SLD_DivX_Init(),SLD_DivX_Decode_Frame(),SLD_DivX_Close().
;
; Revision History (Reverse chronological order)
;------------------------------------------------------------------------------
;    Author(s)        Date               Updation Details
;------------------------------------------------------------------------------
;     Anand            26-02-2008             File created
;
;
;------------------------------------------------------------------------------
;
; 
;*****************************************************************************/

 
/*
*****************************************************************************
*                         INCLUDE FILES
*****************************************************************************
*/
#ifndef __DIVX_H
#define __DIVX_H


#define DIVX_BUFF_PAD_SIZE        16 // don't change it. s/w use padding size as 16

typedef struct DivxDecInParam_t
{
    unsigned int     width;
    unsigned int     height;
    unsigned int     frame_num;
    unsigned int     codec_version;
    unsigned int    size;
    char         *strmbuf;
    unsigned int        file_format;
} DivxDecInParam;

typedef struct DivxDecOutParam_t
{
    char            *virYUVBuff;
    unsigned int    phyYUVBuff;
    unsigned int     buf_width;
    unsigned int     buf_height;
} DivxDecOutParam;

#ifdef __cplusplus
extern "C" {
#endif

int SsbSipHybridDivxDecInit(DivxDecInParam *inParam, DivxDecOutParam *outParam);
int SsbSipHybridDivxDecExe(DivxDecInParam *inParam, DivxDecOutParam *outParam);
int SsbSipHybridDivxDecDeInit();
void set_last_frame_to_display(DivxDecOutParam *outParam);

#ifdef __cplusplus
}
#endif
#endif