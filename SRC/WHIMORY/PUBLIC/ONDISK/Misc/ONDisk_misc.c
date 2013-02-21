/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketMory                                                      */
/* MODULE  : Block Driver for supporting FAT File system                     */
/* FILE    : ONDisk_misc.c                                                   */
/* PURPOSE : This file implements Windows CE Block device driver interface   */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*          COPYRIGHT 2003-2009 SAMSUNG ELECTRONICS CO., LTD.                */
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
/*   30-JAN-2009 [HMSEO]    : first writing                                  */
/*   05-NOV-2009 [YONG]     : Running condition(CPU idle ratio) is changed.  */
/*                                                                           */
/*****************************************************************************/


#include <windows.h>
#include <bldver.h>
#include <windev.h>
#include <types.h>
#include <excpt.h>
#include <tchar.h>
#include <devload.h>
#include <diskio.h>
#include <storemgr.h>

#include <WMRConfig.h>
#include <WMRTypes.h>
#include <VFLBuffer.h>
#include <HALWrapper.h>
#include <config.h>
#include <WMR.h>
#include <WMR_Utils.h>


/**
 *  Imported variable declarations
 */
HANDLE g_hThread = NULL;
BOOL g_bNandScan = FALSE;


/**
 *  Imported function declarations
 */


/**
 *  Local #define
 */
#define SCAN_OS (1<<1)
#define SCAN_FS (1<<2)


/**
 *  Local constant definitions
 */


/**
 *  Local typedefs
 */


/**
 *  Local function prototypes
 */
#if (WMR_READ_RECLAIM)
DWORD WINAPI CallFTL_Reclaim(LPVOID lpParameter);
DWORD WINAPI ForceFTL_Reclaim(LPVOID lpParameter);
DWORD WINAPI CallFTL_Scan(LPVOID lpParameter);
DWORD WINAPI ForceFTL_Scan(LPVOID lpParameter);
#endif


/**
 *  Function definitions
 */

#if (WMR_READ_RECLAIM)
/**
 *  @anchor NAND_Scan
 *
 *  @brief  This function is Main function to create thread
 *
 *  @return TRUE : Success to create threads
 *  @return FALSE : Fail to create threads
 */
BOOL NAND_Scan(void)
{
    if ( g_bNandScan == TRUE )
        return TRUE;

    RETAILMSG(TRUE, (TEXT("NAND_Scan:Create thread!!!\r\n")));

    g_bNandScan = TRUE;

    g_hThread = CreateThread(NULL, 0, CallFTL_Reclaim, NULL, 0, NULL);
    if ( g_hThread == NULL )
    {
        RETAILMSG(TRUE, (TEXT("CallFTL_Reclaim Thread Creat Fail!!!(LastError : 0x%x)\r\n"), GetLastError()));
        return FALSE;
    }

#if 0
	g_hThread = CreateThread(NULL,NULL,ForceFTL_Reclaim,NULL,NULL,NULL);
	if ( g_hThread == NULL )
	{
	    RETAILMSG(1,(TEXT("ForceFTL_Reclaim Thread Creat Fail!!!(LastError : 0x%x)\r\n"), GetLastError()));
	    return FALSE;
	}
#endif

    g_hThread = CreateThread(NULL, 0, CallFTL_Scan, NULL, 0, NULL);
    if ( g_hThread == NULL )
    {
        RETAILMSG(TRUE, (TEXT("CallFTL_Scan Thread Creat Fail!!!(LastError : 0x%x)\r\n"), GetLastError()));
        return FALSE;
    }

    return TRUE;
}


/**
 *  @anchor CallFTL_Reclaim
 *
 *  @brief  This function calls FTL_ReadReclaim function every 1 seconds.
 *
 *  @note   Adjust the running condition like a CPU IDLE ratio to fit OEM's target.
 */
DWORD WINAPI CallFTL_Reclaim(LPVOID lpParameter)
{
    unsigned long dwStartTick = 0, dwIdleSt = 0, dwStopTick = 0, dwIdleEd = 0, PercentIdle = 0;

    Sleep(1*60*1000);   // Wait 1 Minutes

    RETAILMSG(TRUE, (TEXT("CallFTL_Reclaim thread start!!!\r\n")));

    while(TRUE)
    {
        Sleep(3*60*1000);   // Wait 3 Minutes

        while(TRUE)
        {
            dwStartTick = GetTickCount();
            dwIdleSt = GetIdleTime();
            Sleep(1000);    // 1 Seconds
            dwStopTick = GetTickCount();
            dwIdleEd = GetIdleTime();
            PercentIdle = ((100*(dwIdleEd - dwIdleSt)) / (dwStopTick - dwStartTick));

            if ( PercentIdle > 40 )
            {
                // RETAILMSG(TRUE, (_T(" Idle Time : %3d%% :"),PercentIdle));
                RETAILMSG(TRUE, (TEXT("Call FTL_ReadReclaim!!\r\n")));

                FTL_ReadReclaim();

                break;
            }
        }
    }
}


/**
 *  @anchor CallFTL_Scan
 *
 *  @brief  This function calls FTL_Scan function every 1 seconds.
 *
 *  @note   Adjust the running condition like a CPU IDLE ratio to fit OEM's target.
 */
DWORD WINAPI CallFTL_Scan(LPVOID lpParameter)
{
    unsigned long dwStartTick = 0, dwIdleSt = 0, dwStopTick = 0, dwIdleEd = 0, PercentIdle = 0;
    UINT32 mode = 0;

    RETAILMSG(TRUE, (TEXT("CallFTL_Scan thread start!!!\r\n")));

    while(TRUE)
    {
        Sleep(5*60*1000);

        while(TRUE)
        {
            dwStartTick = GetTickCount();
            dwIdleSt = GetIdleTime();
            Sleep(1000);    // 1 Seconds
            dwStopTick = GetTickCount();
            dwIdleEd = GetIdleTime();
            PercentIdle = ((100*(dwIdleEd - dwIdleSt)) / (dwStopTick - dwStartTick));

            if ( PercentIdle > 50 )
            {
                // RETAILMSG(TRUE, (_T(" Idle Time : %3d%% :"),PercentIdle));
                RETAILMSG(TRUE, (TEXT("Call FTL_Scan!!!\r\n")));

                mode = (SCAN_FS|SCAN_OS);
                FTL_Scan(mode);

                break;
            }
        }
    }
}


/**
 *  @anchor ForceFTL_Reclaim
 *
 *  @brief  This function forces it to call FTL_ReadReclaim function every 30 minutes.
 */
DWORD WINAPI ForceFTL_Reclaim(LPVOID lpParameter)
{
    RETAILMSG(TRUE, (TEXT("ForceFTL_ReadReclaim thread start!!!\r\n")));

    while(TRUE)
    {
        Sleep(30*60*1000);  // 30 minutes...
        // RETAILMSG(TRUE, (TEXT("Force FTL_ReadReclaim!!!\r\n")));

        FTL_ReadReclaim();
    }
}


/**
 *  @anchor ForceFTL_Scan
 *
 *  @brief  This function forces it to call FTL_Scan function every 5 minutes.
 */
DWORD WINAPI ForceFTL_Scan(LPVOID lpParameter)
{
    UINT32 mode = 0;
    RETAILMSG(TRUE, (TEXT("ForceFTL_Scan thread start!!!\r\n")));
    while(TRUE)
    {
        Sleep(5*60*1000);   // 5 minutes...
        // RETAILMSG(TRUE, (TEXT("Force FTL_Scan!!!\r\n")));

        mode = (SCAN_FS|SCAN_OS);
        FTL_Scan(mode);
    }
}

#endif

