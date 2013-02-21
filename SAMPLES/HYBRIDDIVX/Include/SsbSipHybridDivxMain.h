/*********************************************************************************
*                                                                               *
* Copyright (c) 2008 Samsung System LSI                                            *
* All rights reserved.                                                          *
*                                                                               *
* This software is test sample code for Hybrid Divx Decoder                        *
*                                                                                *
* Author : Jiyoung Shin                                                            *
* Last Changed : 2008.06.10                                                     *
*********************************************************************************/

#ifndef __HYBRID_DIVX_MAIN_H__
#define __HYBRID_DIVX_MAIN_H__

#ifdef __cplusplus
extern "C" {    
#endif
void SsbSipHybridDivxResourceRelease(void);
void SsbSipHybridDivxMain(void);
void SsbSipHybridDivxDecode(char *filename);
#ifdef __cplusplus
}        
#endif    /* __cplusplus */

#endif

