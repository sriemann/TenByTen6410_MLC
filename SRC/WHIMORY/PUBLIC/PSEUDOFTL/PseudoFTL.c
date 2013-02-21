/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* MODULE  : Pseudo FTL                                                      */
/* NAME    : Pseudo FTL                                                      */
/* FILE    : PseudoFTL.c                                                     */
/* PURPOSE : This file contains the exported routine for interfacing with    */
/*           the upper layer of FTL.                                         */
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
#include <WinCEWMROAM.h>
#include <FTL.h>

/*****************************************************************************/
/* Debug Definitions                                                         */
/*****************************************************************************/

#define FTLP_RTL_PRINT(x)        PSII_RTL_PRINT(x)

#if FTLP_ERR_MSG_ON
#define FTLP_ERR_PRINT(x)        PSII_RTL_PRINT(x)
#else
#define FTLP_ERR_PRINT(x)
#endif /* #if FTLP_ERR_MSG_ON */

#if FTLP_LOG_MSG_ON
#define FTLP_LOG_PRINT(x)        PSII_DBG_PRINT(x)
#else
#define FTLP_LOG_PRINT(x)
#endif  /* #if FTLP_LOG_MSG_ON */

#if FTLP_INF_MSG_ON
#define FTLP_INF_PRINT(x)        PSII_DBG_PRINT(x)
#else
#define FTLP_INF_PRINT(x)
#endif  /* #if FTLP_INF_MSG_ON */


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
/*		FTL_Init                                                             */
/* DESCRIPTION                                                               */
/*      This function initializes FTL layer.                                 */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      FTL_SUCCESS                                                          */
/*            FTL_Init is completed.                                         */
/*      FTL_CRITICAL_ERROR                                                   */
/*            FTL_Init is failed.                                            */
/* NOTES                                                                     */
/*      Before all of other functions of FTL is called, FTL_Init() should be */
/*      called.                                                              */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_Init(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Init()\r\n")));

    do {
        /* FTL Init */
        stPacket.nCtrlCode  = PM_HAL_FTL_INIT;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if ((nResult != FTL_SUCCESS) && (nResult != FTL_CRITICAL_ERROR))
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Init() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Init()\r\n")));

    return (INT32)nResult;

}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		FTL_Open                                                             */
/* DESCRIPTION                                                               */
/*      This function opens FTL layer.                                       */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      FTL_SUCCESS                                                          */
/*            FTL_Open is completed.                                         */
/*		FTL_CRITICAL_ERROR                                                   */
/*			  FTL_Open is failed.                                            */
/* NOTES                                                                     */
/*      Before FTL_Open() is called, FTL_Init() should be called.            */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_Open(UINT32 *pTotalScts)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Open()\r\n")));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Open()  pTotalScts = 0x%x\r\n"), pTotalScts));

    do {
        /* FTL_Open */
        stPacket.nCtrlCode  = PM_HAL_FTL_OPEN;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = pTotalScts;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Open() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Open()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*		FTL_Format                                                           */
/* DESCRIPTION                                                               */
/*      This function formats FTL.                                           */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*		FTL_SUCCESS                                                          */
/*            FTL_Format is completed.                                       */
/*		FTL_CRITICAL_ERROR                                                   */
/*            FTL_Format is failed.    		                                 */
/* NOTES                                                                     */
/*      Before FTL_Format() is called, FTL_Init() should be called.          */
/*      When this function is called, AC power must be connected.            */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_Format(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Format()\r\n")));

    do {
        /* FTL_Format */
        stPacket.nCtrlCode  = PM_HAL_FTL_FORMAT;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Format() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Format()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      FTL_Read                                                             */
/* DESCRIPTION                                                               */
/*      This function reads virtual page.                                    */
/* PARAMETERS                                                                */
/*      nVpn		[IN]	virtual page number                              */
/*      pBuf		[IN]	Buffer pointer                                   */
/*      bCleanCheck	[IN]	clean check or not                               */
/* RETURN VALUES                                                             */
/*		FTL_SUCCESS															 */
/*				FTL_Read is completed										 */
/*		FTL_CRITICAL_ERROR													 */
/*				FTL_Read is failed											 */
/*		FTL_USERDATA_ERROR													 */
/*				there is an uncorrectable read error on user data			 */
/*		FTL_OUT_OF_RANGE													 */
/*				input parameters are invalid								 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_Read(UINT32 nLsn, UINT32 nNumOfScts, UINT8 *pBuf)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Read()\r\n")));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Read()  nLsn        = %d\r\n"), nLsn));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Read()  nNumOfScts  = %d\r\n"), nNumOfScts));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Read()  pBuf        = 0x%x\r\n"), pBuf));

    do {
        /* FTL_Read */
        stPacket.nCtrlCode  = PM_HAL_FTL_READ;
        stPacket.nLsn       = nLsn;
        stPacket.nNumOfScts = nNumOfScts;
        stPacket.pBuf       = pBuf;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Read() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Read()\r\n")));

    return (INT32)nResult;
}


INT32
FTL_Scan(UINT32 mode)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Scan()\r\n")));

    do {
        /* FTL_Read */
        stPacket.nCtrlCode  = PM_HAL_FTL_SCAN;
        stPacket.nLsn       = mode;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Read() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Read()\r\n")));

    return (INT32)nResult;
}


#if (WMR_READ_RECLAIM)
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      FTL_ReadReclaim		                                                 */
/* DESCRIPTION                                                               */
/*      This function checks read reclaim map & reclaims read error blks.	 */
/* PARAMETERS                                                                */
/*		none																 */
/* RETURN VALUES                                                             */
/*		FTL_SUCCESS															 */
/*				FTL_ReadReclaim is completed								 */
/*		FTL_CRITICAL_ERROR													 */
/*				FTL_ReadReclaim is failed									 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_ReadReclaim(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_ReadReclaim()\r\n")));

    do {
        /* FTL_ReadReclaim */
        stPacket.nCtrlCode  = PM_HAL_FTL_READRECLAIM;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_ReadReclaim() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_ReadReclaim()\r\n")));

    return (INT32)nResult;
}
#endif


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      FTL_Write                                                            */
/* DESCRIPTION                                                               */
/*      This function writes virtual page.                                   */
/* PARAMETERS                                                                */
/*      nVpn		[IN]	virtual page number                              */
/*      pBuf		[IN]	Buffer pointer                                   */
/* RETURN VALUES                                                             */
/*		FTL_SUCCESS															 */
/*				FTL_Write is completed										 */
/*		FTL_CRITICAL_ERROR													 */
/*				FTL_Write is failed											 */
/*		FTL_OUT_OF_RANGE													 */
/*				input parameters are invalid								 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_Write(UINT32 nLsn, UINT32 nNumOfScts, UINT8 *pBuf)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Write()\r\n")));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Wrhte()  nVpn        = %d\r\n"), nLsn));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Wrhte()  nNumOfScts  = %d\r\n"), nNumOfScts));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Wrhte()  pBuf        = 0x%x\r\n"), pBuf));

    do {
        /* FTL_Write */
        stPacket.nCtrlCode  = PM_HAL_FTL_WRITE;
        stPacket.nLsn       = nLsn;
        stPacket.nNumOfScts = nNumOfScts;
        stPacket.pBuf       = pBuf;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Write() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Write()\r\n")));

    return (INT32)nResult;
}

INT32
FTL_Delete(UINT32 nLsn, UINT32 nNumOfScts)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Delete()\r\n")));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Delete()  nVpn        = %d\r\n"), nLsn));
    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Delete()  nNumOfScts  = %d\r\n"), nNumOfScts));

    do {
        /* FTL_Write */
        stPacket.nCtrlCode  = PM_HAL_FTL_DELETE;
        stPacket.nLsn       = nLsn;
        stPacket.nNumOfScts = nNumOfScts;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Delete() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Delete()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      FTL_GarbageCollect	                                                 */
/* DESCRIPTION                                                               */
/*      This function call _Merge(NULL) function.							 */
/* PARAMETERS                                                                */
/*		none																 */
/* RETURN VALUES                                                             */
/*		TRUE32																 */
/*				FTL_GarbageCollect is success.								 */
/*		FALSE32																 */
/*				FTL_GarbageCollect is failed.								 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
BOOL32
FTL_GarbageCollect(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_GarbageCollect()\r\n")));

    do {
        /* FTL_ReadReclaim */
        stPacket.nCtrlCode  = PM_HAL_FTL_GARBAGECOLLECT;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if ((BOOL32)nResult != TRUE32)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_GarbageCollect() failure.x\r\n")));
            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_GarbageCollect()\r\n")));

    return (BOOL32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      FTL_Close                                                            */
/* DESCRIPTION                                                               */
/*      This function releases FTL layer.								     */
/* PARAMETERS                                                                */
/*      none			                                                     */
/* RETURN VALUES                                                             */
/*		FTL_SUCCESS															 */
/*				FTL_Close is completed										 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_Close(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_Close()\r\n")));

    do {
        /* FTL_Close */
        stPacket.nCtrlCode  = PM_HAL_FTL_CLOSE;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_Close() failure. ERR Code=%x\r\n"), nResult));
            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_Close()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      FTL_FormatProc		                                                 */
/* DESCRIPTION                                                               */
/*      This function formats ftl in line process.							 */
/* PARAMETERS                                                                */
/*		none																 */
/* RETURN VALUES                                                             */
/*		FTL_SUCCESS															 */
/*				FTL_FormatProc is completed									 */
/*		FTL_CRITICAL_ERROR													 */
/*				FTL_FormatProc is failed									 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
INT32
FTL_FormatProc(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++FTL_FormatProc()\r\n")));

    do {
        /* FTL_FormatProc */
        stPacket.nCtrlCode  = PM_HAL_FTL_FORMATPROC;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult != FTL_SUCCESS)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  FTL_FormatProc() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --FTL_FormatProc()\r\n")));

    return (INT32)nResult;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      WMR_Format_FIL		                                                 */
/* DESCRIPTION                                                               */
/*      This function formats fil in line process.							 */
/* PARAMETERS                                                                */
/*		none																 */
/* RETURN VALUES                                                             */
/*		TRUE32  															 */
/*				WMR_Format_FIL is completed									 */
/*		FALSE32																 */
/*				WMR_Format_FIL is failed									 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
BOOL32
WMR_Format_FIL(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++WMR_Format_FIL()\r\n")));

    do {
        /* WMR_Format_FTL */
        stPacket.nCtrlCode  = PM_HAL_WMR_FORMAT_FIL;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult == FALSE32)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  WMR_Format_FIL() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --WMR_Format_FIL()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      WMR_Format_VFL		                                                 */
/* DESCRIPTION                                                               */
/*      This function formats vfl in line process.							 */
/* PARAMETERS                                                                */
/*		none																 */
/* RETURN VALUES                                                             */
/*		TRUE32  															 */
/*				WMR_Format_VFL is completed									 */
/*		FALSE32																 */
/*				WMR_Format_VFL is failed									 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
BOOL32
WMR_Format_VFL(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++WMR_Format_VFL()\r\n")));

    do {
        /* WMR_Format_FTL */
        stPacket.nCtrlCode  = PM_HAL_WMR_FORMAT_VFL;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult == FALSE32)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  WMR_Format_VFL() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --WMR_Format_VFL()\r\n")));

    return (INT32)nResult;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      WMR_Format_FTL		                                                 */
/* DESCRIPTION                                                               */
/*      This function formats ftl in line process.							 */
/* PARAMETERS                                                                */
/*		none																 */
/* RETURN VALUES                                                             */
/*		TRUE32  															 */
/*				WMR_Format_FTL is completed									 */
/*		FALSE32																 */
/*				WMR_Format_FTL is failed									 */
/* NOTES                                                                     */
/*                                                                           */
/*****************************************************************************/
BOOL32
WMR_Format_FTL(VOID)
{
    FTLPacket   stPacket;
    UINT32      nResult;

    FTLP_LOG_PRINT((TEXT("[FTLP: IN] ++WMR_Format_FTL()\r\n")));

    do {
        /* WMR_Format_FTL */
        stPacket.nCtrlCode  = PM_HAL_WMR_FORMAT_FTL;
        stPacket.nLsn       = 0;
        stPacket.nNumOfScts = 0;
        stPacket.pBuf       = NULL;
        stPacket.pTotalScts = NULL;

        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,                /* Input buffer (Additional Control Code) */
                        sizeof(FTLPacket),        /* Size of Input buffer */
                        NULL,                     /* Output buffer */
                        0,                        /* Size of Output buffer */
                        &nResult);                /* Error Return */

        if (nResult == FALSE32)
        {
            FTLP_ERR_PRINT((TEXT("[FTLP:ERR]  WMR_Format_FTL() failure. ERR Code=%x\r\n"), nResult));

            break;
        }

    } while(0);

    FTLP_LOG_PRINT((TEXT("[FTLP:OUT] --WMR_Format_FTL()\r\n")));

    return (INT32)nResult;
}

