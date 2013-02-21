/*****************************************************************************/
/*                                                                           */
/* COMPONENT   : Rainbow				                                     */
/* MODULE      : Example			                                         */
/* NAME    	   : Example					                                 */
/* FILE        : WMRExam.h		                                             */
/* PURPOSE 	   : the example for whimory initialization						 */
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
/*   12-SEP-2005 [Jaesung Jung] : first writing                              */
/*                                                                           */
/*****************************************************************************/
#ifndef _WMR_EXAM_H_
#define _WMR_EXAM_H_

/*****************************************************************************/
/* Return value of WMR_XXX()                                                 */
/*****************************************************************************/
#define     WMR_SUCCESS                     WMR_RETURN_VALUE(0, 0x0000, 0x0000)
#define     WMR_CRITICAL_ERROR              WMR_RETURN_VALUE(1, 0x0001, 0x0000)

/*****************************************************************************/
/* exported function prototype of WMR example                                */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32	 WMR_Init				(UINT32 *pTotalScts);
VOID	 WMR_Close				(VOID);
VOID	 WMR_EraseAll			(VOID);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FTL_H_ */
