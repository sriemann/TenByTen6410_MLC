/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : PSIIMDDOAL.h                                                    */
/* PURPOSE : This file is the header file of PSIIMDDOAL.c                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003,2004 SAMSUNG ELECTRONICS CO., LTD.                */
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
/*   01-APR-2004 [Janghwan Kim]: first writing                               */
/*                                                                           */
/*****************************************************************************/
#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL MSMDDInit();
BOOL IsMSMDDIoControl(DWORD     dwIoControlCode, 
                      LPVOID    lpInBuf, 
                      DWORD     nInBufSize, 
                      LPVOID    lpOutBuf, 
                      DWORD     nOutBufSize, 
                      LPDWORD   lpBytesReturned, 
                      BOOL     *pRetVal);

#ifdef __cplusplus
}
#endif
