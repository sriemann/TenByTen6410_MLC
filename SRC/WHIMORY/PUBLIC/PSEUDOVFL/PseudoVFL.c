/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* MODULE  : Pseudo VFL                                                      */
/* NAME    : Pseudo VFL                                                      */
/* FILE    : PseudoVFL.c                                                     */
/* PURPOSE : This file contains the exported routine for interfacing with    */
/*           the upper layer of VFL.                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2004 SAMSUNG ELECTRONICS CO., LTD.                */
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
/*   17-OCT-2006 [Seungkyu Kim]: first writing                               */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/
/* Headerfile Include                                                        */
/*****************************************************************************/
#include <windows.h>
#include <bldver.h>
#include <windev.h>
#include <types.h>
#include <excpt.h>
#include <tchar.h>
#include <devload.h>
#include <diskio.h>

#include <VFLBuffer.h>
#include <WMRTypes.h>
#include <VFL.h>
#include <HALWrapper.h>

#include <storemgr.h>
#include <pm.h>
#include <config.h>
#include <WMRConfig.h>
#include <FIL.h>
#include <bibdrvinfo.h>

/*****************************************************************************/
/* Debug Definitions                                                         */
/*****************************************************************************/

#define VFLP_RTL_PRINT(x)        PSII_RTL_PRINT(x)

#if VFLP_ERR_MSG_ON
#define VFLP_ERR_PRINT(x)        PSII_RTL_PRINT(x)
#else
#define VFLP_ERR_PRINT(x)        
#endif /* #if VFLP_ERR_MSG_ON */

#if VFLP_LOG_MSG_ON
#define VFLP_LOG_PRINT(x)        PSII_RTL_PRINT(x)
#else
#define VFLP_LOG_PRINT(x)        
#endif  /* #if VFLP_LOG_MSG_ON */

#if VFLP_INF_MSG_ON
#define VFLP_INF_PRINT(x)        PSII_RTL_PRINT(x)
#else
#define VFLP_INF_PRINT(x)        
#endif  /* #if VFLP_INF_MSG_ON */


/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/


/*****************************************************************************/
/* Imported variable declarations                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Imported function declarations                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local #define                                                             */
/*****************************************************************************/


/*****************************************************************************/
// Local constant definitions
/*****************************************************************************/


/*****************************************************************************/
// Local typedefs
/*****************************************************************************/


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		VFL_Init                                                             */
/* DESCRIPTION                                                               */
/*      This function initializes VFL layer.                                 */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      VFL_SUCCESS                                                          */
/*            VFL_Init is completed.                                         */
/*      VFL_CRITICAL_ERROR                                                   */
/*            VFL_Init is failed.                                            */
/* NOTES                                                                     */
/*      Before all of other functions of VFL is called, VFL_Init() should be */
/*      called.                                                              */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Init(VOID)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Init()\r\n")));

    do {
        /* VFL Init */
        stPacket.nCtrlCode  = PM_HAL_VFL_INIT;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
    
        if ((nResult != VFL_SUCCESS) && (nResult != VFL_CRITICAL_ERROR))
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Init() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Init()\r\n")));

    return (INT32)nResult;
    
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		VFL_Open                                                             */
/* DESCRIPTION                                                               */
/*      This function opens VFL layer.                                       */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      VFL_SUCCESS                                                          */
/*            VFL_Open is completed.                                         */
/*		VFL_CRITICAL_ERROR                                                   */
/*			  VFL_Open is failed.                                            */
/* NOTES                                                                     */
/*      Before VFL_Open() is called, VFL_Init() should be called.            */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Open(VOID)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Open()\r\n")));

    do {
        /* VFL_Open */
        stPacket.nCtrlCode  = PM_HAL_VFL_OPEN;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
    
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Open() failure. ERR Code=%x\r\n"), nResult));
    
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Open()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		VFL_Format                                                           */
/* DESCRIPTION                                                               */
/*      This function formats VFL.                                           */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*		VFL_SUCCESS                                                          */
/*            VFL_Format is completed.                                       */
/*		VFL_CRITICAL_ERROR                                                   */
/*            VFL_Format is failed.    		                                 */
/* NOTES                                                                     */
/*      Before VFL_Format() is called, VFL_Init() should be called.          */
/*      When this function is called, AC power must be connected.            */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Format(VOID)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Format()\r\n")));

    do {
        /* VFL_Format */
        stPacket.nCtrlCode  = PM_HAL_VFL_FORMAT;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
    
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Format() failure. ERR Code=%x\r\n"), nResult));
    
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Format()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_Read                                                             */
/* DESCRIPTION                                                               */
/*      This function reads virtual page.                                    */
/* PARAMETERS                                                                */
/*      nVpn		[IN]	virtual page number                              */
/*      pBuf		[IN]	Buffer pointer                                   */
/*      bCleanCheck	[IN]	clean check or not                               */
/* RETURN VALUES                                                             */
/*      VFL_SUCCESS                                                          */
/*            VFL_Read is completed.                                         */
/*      VFL_CRITICAL_ERROR                                                   */
/*            critical error                                                 */
/*		VFL_U_ECC_ERROR                                                      */
/*            ECC uncorrectable error occurs from FIL read function.         */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Read(UINT32 nVpn, Buffer *pBuf, BOOL32 bCleanCheck)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Read()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Read()  nVpn        = %d\r\n"), nVpn));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Read()  pBuf        = 0x%x\r\n"), pBuf));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Read()  bCleanCheck = %d\r\n"), bCleanCheck));

    do {
        /* VFL_Read */
        stPacket.nCtrlCode  = PM_HAL_VFL_READ;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = nVpn;
        stPacket.pBuf       = pBuf;
        stPacket.nSrcVpn    = 0;
        stPacket.nDesVpn    = 0;
        stPacket.bCleanCheck= bCleanCheck;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Read() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Read()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_Write                                                            */
/* DESCRIPTION                                                               */
/*      This function writes virtual page.                                   */
/* PARAMETERS                                                                */
/*      nVpn		[IN]	virtual page number                              */
/*      pBuf		[IN]	Buffer pointer                                   */
/* RETURN VALUES                                                             */
/*      VFL_SUCCESS                                                          */
/*            VFL_Write is completed.                                        */
/*      VFL_CRITICAL_ERROR                                                   */
/*            VFL_Write is failed.                                           */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Write(UINT32 nVpn, Buffer *pBuf)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Write()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Wrhte()  nVpn = %d\r\n"), nVpn));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Wrhte()  pBuf = 0x%x\r\n"), pBuf));

    do {
        /* VFL_Write */
        stPacket.nCtrlCode  = PM_HAL_VFL_WRITE;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = nVpn;
        stPacket.pBuf       = pBuf;
        stPacket.nSrcVpn    = 0;
        stPacket.nDesVpn    = 0;
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Write() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Write()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_Erase                                                            */
/* DESCRIPTION                                                               */
/*      This function erases virtual block(super block).				     */
/* PARAMETERS                                                                */
/*      nVbn	[IN]	virtual block number 								 */
/* RETURN VALUES                                                             */
/* 		VFL_SUCCESS                                                          */
/*            VFL_Erase is completed.  	                                     */
/*      VFL_CRITICAL_ERROR                                                   */
/*            VFL_Erase is failed.    		                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Erase(UINT32 nVbn)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Erase()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Erase()  nVbn = %d(0x%x)\r\n"), nVbn, nVbn));

    do {
        /* VFL_Erase */
        stPacket.nCtrlCode  = PM_HAL_VFL_ERASE;
        stPacket.nVbn       = nVbn;
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
   
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Erase() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Erase()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_Copyback                                                         */
/* DESCRIPTION                                                               */
/*      This function copies page data from source to destination.		     */
/* PARAMETERS                                                                */
/*      nSrcVpn		[IN]	virtual block number (source)					 */ 
/*      nDesVpn		[IN]	virtual block number (destination)				 */
/*      pBuf		[IN]	buffer pointer		 							 */
/* RETURN VALUES                                                             */
/* 		VFL_SUCCESS                                                          */
/*            VFL_Copyback is completed.                                     */
/*      VFL_CRITICAL_ERROR                                                   */
/*            VFL_Copyback is failed.    	                                 */
/*		VFL_U_ECC_ERROR														 */
/*			  ECC uncorrectable error occurs from FIL read function.		 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Copyback(UINT32 nSrcVpn, UINT32 nDesVpn, Buffer *pBuf)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Copyback()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Copyback()  nSrcVpn = %d\r\n"), nSrcVpn));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Copyback()  nDesVpn = %d\r\n"), nDesVpn));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Copyback()  pBuf    = 0x%x\r\n"), pBuf));

    do {
        /* VFL_Copyback */
        stPacket.nCtrlCode  = PM_HAL_VFL_CPBACK;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = pBuf;
        stPacket.nVpn       = nSrcVpn;            // Not used
        stPacket.nVpn       = nDesVpn;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Copyback() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Copyback()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_Sync                                                             */
/* DESCRIPTION                                                               */
/*      This function checks all bank's status.							     */
/* PARAMETERS                                                                */
/*      none										                         */
/* RETURN VALUES                                                             */
/* 		VFL_SUCCESS                                                          */
/*            VFL_Sync is completed.  	                                     */
/*      VFL_CRITICAL_ERROR                                                   */
/*            VFL_Sync is failed.    		                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Sync(VOID)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Sync()\r\n")));

    do {
        /* VFL_Sync */
        stPacket.nCtrlCode  = PM_HAL_VFL_SYNC;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Sync() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Sync()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_Close                                                            */
/* DESCRIPTION                                                               */
/*      This function releases VFL layer.								     */
/* PARAMETERS                                                                */
/*      none			                                                     */
/* RETURN VALUES                                                             */
/*      none			                                                     */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_Close(VOID)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_Close()\r\n")));

    do {
        /* VFL_Close */
        stPacket.nCtrlCode  = PM_HAL_VFL_CLOSE;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_Close() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_Close()\r\n")));

    return (INT32)nResult;
}    


#if (WMR_SUPPORT_META_WEAR_LEVEL)
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_GetFTLCxtVbn                                                     */
/* DESCRIPTION                                                               */
/*      This function is returning the recent FTL context block position     */
/* PARAMETERS                                                                */
/*		aFTLCxtVbn  [OUT]   Recent FTL context block list					 */
/* RETURN VALUES                                                             */
/* 		none                                                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
void
VFL_GetFTLCxtVbn(UINT16      *pFTLCxtVbn)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_GetFTLCxtVbn()\r\n")));

    do {
        /* VFL_GetFTLCxtVbn */
        stPacket.nCtrlCode  = PM_HAL_VFL_GETFTLCXT;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        pFTLCxtVbn,       /* Output buffer */
                        sizeof(UINT16*),                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        VFLP_LOG_PRINT((TEXT("[VFLP:INF] VFL_GetFTLCxtVbn()  nResult = 0x%x\r\n"), nResult));
        VFLP_LOG_PRINT((TEXT("[VFLP:INF] VFL_GetFTLCxtVbn()  pFTLCxtVbn = 0x%x\r\n"), pFTLCxtVbn));
//        VFLP_LOG_PRINT((TEXT("[VFLP:INF] VFL_GetFTLCxtVbn()  *pFTLCxtVbn = 0x%x\r\n"), *pFTLCxtVbn));
        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_GetFTLCxtVbn() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_GetFTLCxtVbn()\r\n")));

//    return (UINT16*)pFTLCxtVbn;
}    
#endif


#if (WMR_SUPPORT_META_WEAR_LEVEL)
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      VFL_ChangeFTLCxtVbn	                                                 */
/* DESCRIPTION                                                               */
/*      This function change the virtual block number of FTL context block   */
/* PARAMETERS                                                                */
/*      aFTLCxtVbn  [IN]    FTL context block list                           */
/*                          that replace old FTL context positon             */
/* RETURN VALUES                                                             */
/* 		VFL_SUCCESS                                                          */
/*            VFL_Format is completed.                                       */
/*      VFL_CRITICAL_ERROR                                                   */
/*            VFL_Format is failed.    		                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
VFL_ChangeFTLCxtVbn(UINT16 *pFTLCxtVbn)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_ChangeFTLCxtVbn()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++VFL_ChangeFTLCxtVbn()  *pFTLCxtVbn = %d\r\n"), *pFTLCxtVbn));

    do {
        /* VFL_ChangeFTLCxtVbn */
        stPacket.nCtrlCode  = PM_HAL_VFL_CHANGEFTLCXT;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        (LPVOID)pFTLCxtVbn,       /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  VFL_ChangeFTLCxtVbn() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --VFL_ChangeFTLCxtVbn()\r\n")));

    return (INT32)nResult;
}    
#endif


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BUF_Get	                                                             */
/* DESCRIPTION                                                               */
/*      This function returns a new free buffer. 						     */
/*		if there is not a free buffer, this function calls the sync function */
/*		and generates a new free buffer.									 */
/* PARAMETERS                                                                */
/*		eType			[IN]												 */
/*				buffer type													 */
/* RETURN VALUES                                                             */
/* 		Buffer	                                                             */
/*            BUF_Get is completed.                                          */
/*      NULL			                                                     */
/*            BUF_Get is failed.    		                                 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
Buffer *
BUF_Get(BUFType eType)
{
    VFLPacket   stPacket;
    Buffer      *pBuf = NULL;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++BUF_Get()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] BUF_Get()  eType = %d\r\n"), eType));

    do {
        /* BUF_Get */
        stPacket.nCtrlCode  = PM_HAL_VFL_BUF_GET;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
        stPacket.eType      = eType;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  BUF_Get() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

        pBuf = stPacket.pBuf;
        VFLP_LOG_PRINT((TEXT("[VFLP: IN] BUF_Get()  pBuf = 0x%x\r\n"), pBuf));

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --BUF_Get()\r\n")));

    return pBuf;
}    


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BUF_Reget                                                            */
/* DESCRIPTION                                                               */
/*      This function changes the type of the buffer.					     */
/* PARAMETERS                                                                */
/*      pBuf			[IN/OUT]                                             */
/*				buffer pointer												 */
/*		eType			[IN]												 */
/*				buffer type													 */
/* RETURN VALUES                                                             */
/*		none																 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
BUF_Reget(Buffer *pBuf, BUFType eType)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++BUF_Reget()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++BUF_Reget()  pBuf  = 0x%x\r\n"), pBuf));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++BUF_Reget()  eType = %d\r\n"), eType));

    do {
        /* BUF_Reget */
        stPacket.nCtrlCode  = PM_HAL_VFL_BUF_REGET;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = pBuf;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
        stPacket.eType      = eType;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  BUF_Reget() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --BUF_Reget()\r\n")));

    return;
}    


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BUF_Release                                                          */
/* DESCRIPTION                                                               */
/*      This function releases the buffer to the buffer pool.			     */
/* PARAMETERS                                                                */
/*      pBuf			[IN]	                                             */
/*				buffer pointer												 */
/* RETURN VALUES                                                             */
/*		none																 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
VOID
BUF_Release(Buffer *pBuf)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++BUF_Release()\r\n")));
    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++BUF_Release()  pBuf = 0x%x\r\n"), pBuf));

    do {
        /* BUF_Release */
        stPacket.nCtrlCode  = PM_HAL_VFL_BUF_RELEASE;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = pBuf;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  BUF_Release() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --BUF_Release()\r\n")));

    return;
}    


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		FIL_Init                                                             */
/* DESCRIPTION                                                               */
/*      This function initializes FIL layer.                                 */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      FIL_SUCCESS                                                          */
/*            VFL_Init is completed.                                         */
/*      FIL_CRITICAL_ERROR                                                   */
/*            VFL_Init is failed.                                            */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
FIL_Init(VOID)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[FILP: IN] ++FIL_Init()\r\n")));

    do {
        /* FIL Init */
        stPacket.nCtrlCode  = PM_HAL_FIL_INIT;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
    
        if (nResult != FIL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[FILP:ERR]  FIL_Init() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[FILP:OUT] --FIL_Init()\r\n")));

    return (INT32)nResult;
    
}

VOID
GetNandInfo(NAND_INFO * pNandInfo)
{
    VFLPacket   stPacket;
    UINT32      nResult;

    VFLP_LOG_PRINT((TEXT("[VFLP: IN] ++GetNandInfo()\r\n")));

    do {
        /* BUF_Release */
        stPacket.nCtrlCode  = PM_HAL_FIL_GET_NANDINFO;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = 0;            // Not used
        stPacket.pBuf       = NULL;
        stPacket.nSrcVpn    = 0;            // Not used
        stPacket.nDesVpn    = 0;            // Not used
        stPacket.bCleanCheck= 0;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(VFLPacket),        /* Size of Input buffer */
                        pNandInfo,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */
                        
        if (nResult != VFL_SUCCESS)
        {
            VFLP_ERR_PRINT((TEXT("[VFLP:ERR]  BUF_Release() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    VFLP_LOG_PRINT((TEXT("[VFLP:OUT] --BUF_Release()\r\n")));

    return;
}

