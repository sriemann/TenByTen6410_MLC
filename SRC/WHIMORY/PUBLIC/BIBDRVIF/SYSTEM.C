/*****************************************************************************/
/*                                                                           */
/* PROJECT : PocketStoreII v1.0.0_build001                                   */
/* FILE    : SYSTEM.c                                                        */
/* PURPOSE : This file implements Windows CE Block device driver interface   */
/*          for supporting BIN file system.                                  */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                COPYRIGHT 2003 SAMSUNG ELECTRONICS CO., LTD.               */
/*                      ALL RIGHTS RESERVED                                  */
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
/* - 20-JAN-2003 [SongHo Yoon]: first writing                                */
/* - 14-APR-2004 [Janghwan Kim]: PocketStoreII migration                     */
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

//#include <WMRTypes.h>
typedef		UINT32              	BOOL32;

#include <MkMBR.h>
//#include <PSII.h>
#include <VFLBuffer.h>
#include <VFL.h>
#include <FIL.h>
#include <HALWrapper.h>

#include "CacheBuf.h"
#include <BIBDRVINFO.h>

#if (CE_MAJOR_VER > 0x0003)
#include <storemgr.h>
#include <pm.h>
#endif  //(CE_MAJOR_VER > 0x0003)

extern VOID GetNandInfo(NAND_INFO *pNandInfo);

NAND_INFO stNandInfo;

// by HMSEO,,, this definitions are have to matched with defined in WMRConfig.h ... Waring....
#define		FALSE32				(BOOL32) 0
#define		TRUE32					(BOOL32) 1
#define		BYTES_PER_SECTOR		512
#define		WMR_RETURN_VALUE(err, maj, min)	(INT32)(((UINT32)((err) & 0x00000001) << 31) | \
											        ((UINT32)((maj) & 0x00007FFF) << 16) | \
											         (UINT32)((min) & 0x0000FFFF))
// by HMSEO,,, because,,, driver can't know this settings....


/*****************************************************************************/
/* Debug Definitions                                                         */
/*****************************************************************************/
#define BIBDRV_ERR_MSG_ON		1
#define BIBDRV_LOG_MSG_ON		0
#define BIBDRV_INF_MSG_ON		0

#define BIBDRV_RTL_PRINT(x)        RETAILMSG(1, x)

#if BIBDRV_ERR_MSG_ON
#define BIBDRV_ERR_PRINT(x)        RETAILMSG(1, x)
#else
#define BIBDRV_ERR_PRINT(x)
#endif /* #if BIBDRV_ERR_MSG_ON */

#if BIBDRV_LOG_MSG_ON
#define BIBDRV_LOG_PRINT(x)        RETAILMSG(1, x)
#else
#define BIBDRV_LOG_PRINT(x)
#endif  /* #if BIBDRV_LOG_MSG_ON */

#if BIBDRV_INF_MSG_ON
#define BIBDRV_INF_PRINT(x)        RETAILMSG(1, x)
#else
#define BIBDRV_INF_PRINT(x)
#endif  /* #if BIBDRV_INF_MSG_ON */


/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/* Debug Zones.
 */
#ifdef DEBUG

    #define DBG_INIT        0x0001
    #define DBG_OPEN        0x0002
    #define DBG_READ        0x0004
    #define DBG_WRITE       0x0008
    #define DBG_CLOSE       0x0010
    #define DBG_IOCTL       0x0020
    #define DBG_THREAD      0x0040
    #define DBG_EVENTS      0x0080
    #define DBG_CRITSEC     0x0100
    #define DBG_FLOW        0x0200
    #define DBG_IR          0x0400
    #define DBG_NOTHING     0x0800
    #define DBG_ALLOC       0x1000
    #define DBG_FUNCTION    0x2000
    #define DBG_WARNING     0x4000
    #define DBG_ERROR       0x8000

DBGPARAM dpCurSettings = {
    TEXT("Serial"), { TEXT("Init"),
                      TEXT("Open"),
                      TEXT("Read"),
                      TEXT("Write"),
                      TEXT("Close"),
                      TEXT("Ioctl"),
                      TEXT("Error")},
    0
}; 
#endif


/*****************************************************************************/
/* Imported variable declarations                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Imported function declarations                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local #define                                                             */
/*****************************************************************************/
//#define _SUPPORT_HAL_WRAPPER_
#undef  _BIBDRV_CACHING_SECTORS_
#undef  _READ_PERFORMANCE_MEASUREMENT_

#undef  _BIBDRV_SECTOR_ACCESS_STATISTIC_
//#undef  _BIBDRV_MBR_DEBUG_
#define  _BIBDRV_MBR_DEBUG_


/*****************************************************************************/
// Local constant definitions
/*****************************************************************************/

/*****************************************************************************/
// Local typedefs
/*****************************************************************************/
#if defined(_BIBDRV_SECTOR_ACCESS_STATISTIC_)

typedef struct _SEC_ACCESS_STATISTIC
{
    UINT nSecNum;
    UINT nSecHit;
} SEC_ACCESS_STATISTIC;

#endif  //_BIBDRV_SECTOR_ACCESS_STATISTIC_

typedef struct _DISK
{
    struct _DISK       *pd_next;
    CRITICAL_SECTION    d_DiskCardCrit; // guard access to global state and card
    HANDLE              hDevice;        // activate Handle
    DISK_INFO           d_DiskInfo;     // for DISK_IOCTL_GET/SETINFO
    DWORD               d_OpenCount;    // open ref count
    LPWSTR              d_ActivePath;   // registry path to active key for this device
    UINT                nVol;           // Volume Number
    BOOL                bIsBMLOpen;     // if BML_Open operation successes,
                                        // it's TRUE otherwize it's FALSE
    UINT                nBaseVsn;       // start Virtual Sector Number of area,
                                        // which is reserved for OS regions
#if defined(_BIBDRV_CACHING_SECTORS_)
    UINT                nNumOfSecCache;
    BIBDRV_CACHEBUF     *pCacheBuf;     // pointer of CacheBuf for caching sectors.
#endif  //(_BIBDRV_CACHING_SECTORS_)

#if defined(_BIBDRV_SECTOR_ACCESS_STATISTIC_)
    SEC_ACCESS_STATISTIC *pSecHitStat;
#endif  //(_BIBDRV_SECTOR_ACCESS_STATISTIC_)

} DISK, *PDISK;

/*****************************************************************************/
// Local variables
/*****************************************************************************/

static CRITICAL_SECTION gDiskCrit;
static PDISK gDiskList;             // initialized to 0 in bss

/*****************************************************************************/
// Local function prototypes
/*****************************************************************************/
static HKEY OpenDriverKey(LPTSTR ActiveKey);
static BOOL GetFolderName(PDISK pDisk, LPWSTR FolderName, DWORD cBytes, DWORD *pcBytes);
static BOOL GetFSDName(PDISK pDisk, LPWSTR FSDName, DWORD cBytes, DWORD *pcBytes);

#if (CE_MAJOR_VER > 0x0003)
static BOOL GetDeviceInfo(PDISK pDisk, PSTORAGEDEVICEINFO psdi);
#endif  //(CE_MAJOR_VER > 0x0003)

static VOID  CloseDisk           (PDISK pDisk);
static BOOL  RequestReadSecToHAL (PDISK pDisk, UINT nSecNum, UCHAR *pBuf);
static DWORD DoDiskRead          (PDISK pDisk, PVOID pData);
static DWORD GetDiskInfo         (PDISK pDisk, PDISK_INFO pInfo);
static DWORD SetDiskInfo         (PDISK pDisk, PDISK_INFO pInfo);
static PDISK CreateDiskObject    (VOID);
static BOOL  IsValidDisk         (PDISK pDisk);
static BOOL  InitializeNAND      (PDISK pDisk);
static BOOL  InitDisk            (PDISK pDisk, LPTSTR ActiveKey);

/*****************************************************************************/
// Function definitions
/*****************************************************************************/

#if defined(_READ_PERFORMANCE_MEASUREMENT_)

#define WT_PRESCALER    (0x00)
#define WT_CLK_DIVISION (0x01)

UINT nTotalBootTime = 0;

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      WatchTimerStart                                                      */
/* DESCRIPTION                                                               */
/*      This function starts watchdog timer                                  */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*      This function should be called WatchTimerStop() with a pair          */
/*****************************************************************************/
static void
WatchTimerStart(void)
{
    WATCHreg *pstWDog = (WATCHreg *) WATCH_BASE;
    
    pstWDog->rWTCON = (WT_PRESCALER << 8) |     /* The prescaler value => 63 + 1    */
                      (0x00 << 6) |             /* Reserved                         */
                      (0x00 << 5) |             /* Timer enable or disable          */
                      (WT_CLK_DIVISION << 3) |  /* The clock division factor => 32  */
                      (0x00 << 2) |             /* Disable bit of interrupt         */
                      (0x00 << 1) |             /* Reserved                         */
                      (0x00 << 0);              /* Disable the reset function of timer */
    pstWDog->rWTDAT = 0xFFFF;
    pstWDog->rWTCNT = 0xFFFF;
    pstWDog->rWTCON = (WT_PRESCALER << 8) |     /* The prescaler value => 63 + 1    */
                      (0x00 << 6) |             /* Reserved                         */
                      (0x01 << 5) |             /* Timer enable or disable          */
                      (WT_CLK_DIVISION << 3) |  /* The clock division factor => 32  */
                      (0x00 << 2) |             /* Disable bit of interrupt         */
                      (0x00 << 1) |             /* Reserved                         */
                      (0x00 << 0);              /* Disable the reset function of timer */
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      WatchTimerStop                                                       */
/* DESCRIPTION                                                               */
/*      This function stops watchdog timer                                   */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTES                                                                     */
/*      This function should be called WatchTimerStart() with a pair         */
/*****************************************************************************/
static void
WatchTimerStop(void)
{
    UINT Cnt, Dat;
    UINT dwUnitCnt;
    UINT dwPrescaler;
    UINT dwWGPref;
    WATCHreg *pstWDog = (WATCHreg *) WATCH_BASE;
    UINT nMTime;


    pstWDog->rWTCON = (WT_PRESCALER << 8) |     /* The prescaler value => 63 + 1    */
                      (0x00 << 6) |             /* Reserved                         */
                      (0x00 << 5) |             /* Timer enable or disable          */
                      (WT_CLK_DIVISION << 3) |  /* The clock division factor => 32*/
                      (0x00 << 2) |             /* Disable bit of interrupt         */
                      (0x00 << 1) |             /* Reserved                         */
                      (0x00 << 0);              /* Disable the reset function of timer */

    Cnt = pstWDog->rWTCNT;
    Dat = pstWDog->rWTDAT;

    dwPrescaler = (WT_PRESCALER + 1) * (16 << WT_CLK_DIVISION);
    dwWGPref    = S2410PCLK / dwPrescaler;
    dwUnitCnt   = 1000000000 / dwWGPref;        // The count unit is NANO second.
    nMTime      = dwUnitCnt * (Dat - Cnt);
    
    nTotalBootTime = nTotalBootTime + nMTime / 1000;
    
    RETAILMSG(1, (TEXT("   Diff = %4d, %8d nsec, Total = %9d usec\r\n"), 
                        Dat - Cnt, nMTime, nTotalBootTime));        
}

#endif  //(_READ_PERFORMANCE_MEASUREMENT_)

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      OpenDriverKey                                                        */
/* DESCRIPTION                                                               */
/*      This function opens the driver key specified by the active key       */
/* PARAMETERS                                                                */
/*      ActiveKey       Handle to a currently open key or any of the         */
/*                      following predefined reserved handle values          */
/* RETURN VALUES                                                             */
/*      Return values is HKEY value of "[ActiveKey]\[Key]", The caller is    */
/*      responsible for closing the returned HKEY                            */
/*                                                                           */
/*****************************************************************************/
static HKEY
OpenDriverKey(LPTSTR ActiveKey)
{
    TCHAR   DevKey[256];
    HKEY    hDevKey;
    HKEY    hActive;
    DWORD   ValType;
    DWORD   ValLen;
    DWORD   status;

    //
    // Get the device key from active device registry key
    //
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,   /* Handle to a currently open key       */
                          ActiveKey,            /* Pointer to subkey                    */
                          0,                    /* Option : Reserved - set to 0         */
                          0,                    /* samDesired : Not supported - set to 0 */
                          &hActive);            /* Pointer for receved handele          */

    if (ERROR_SUCCESS != status)
    {
        BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:OpenDriverKey RegOpenKeyEx(HLM\\%s) returned %d!!!\r\n"),
                        ActiveKey, status));
        return NULL;
    }

    hDevKey = NULL;
    ValLen  = sizeof(DevKey);

    status = RegQueryValueEx(hActive,               /* Handle to a currently open key   */
                             DEVLOAD_DEVKEY_VALNAME,/* Pointer to quary                 */
                             NULL,                  /* Reserved - set to NULL           */
                             &ValType,              /* Pointer to type of data          */
                             (PUCHAR)DevKey,        /* Pointer to data                  */
                             &ValLen);              /* the Length of data               */

    if (ERROR_SUCCESS != status)
    {
        BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:OpenDriverKey - RegQueryValueEx(%s) returned %d\r\n"),
                        DEVLOAD_DEVKEY_VALNAME, status));

        RegCloseKey(hActive);
        return hDevKey;
    }

    //
    // Get the geometry values from the device key
    //
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,   /* Handle to a currently open key       */ 
                          DevKey,               /* Pointer to subkey                    */ 
                          0,                    /* Option : Reserved - set to 0         */ 
                          0,                    /* samDesired : Not supported - set to 0 */
                          &hDevKey);            /* Pointer for receved handele          */ 

    if (ERROR_SUCCESS != status)
    {
        hDevKey = NULL;
        BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:OpenDriverKey RegOpenKeyEx - DevKey(HLM\\%s) returned %d!!!\r\n"),
                        DevKey, status));
    }

    RegCloseKey(hActive);

    return hDevKey;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      GetFolderName                                                        */
/* DESCRIPTION                                                               */
/*      Function to retrieve the folder name value from the driver key       */
/*      The folder name is used by File System Driver to name this disk volume*/
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/*      FolderName  
/*      cBytes      
/*      pcBytes     
/* RETURN VALUES                                                             */
/*      If it successes, it returns TRUE. otherwize it returns FALSE         */
/*                                                                           */
/*****************************************************************************/
static BOOL
GetFolderName(PDISK     pDisk,
              LPWSTR    FolderName,
              DWORD     cBytes,
              DWORD    *pcBytes)
{
    HKEY    DriverKey;
    DWORD   ValType;
    DWORD   status;

    DriverKey = OpenDriverKey(pDisk->d_ActivePath);

    if (NULL != DriverKey)
    {
        *pcBytes = cBytes;
        status = RegQueryValueEx(DriverKey,         /* Handle to a currently open key   */
                                 TEXT("Folder"),    /* Pointer to quary                 */
                                 NULL,              /* Reserved - set to NULL           */
                                 &ValType,          /* Pointer to type of data          */
                                 (PUCHAR)FolderName,/* Pointer to data                  */
                                 pcBytes);          /* the Length of data               */

        if (ERROR_SUCCESS != status)
        {
            BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:GetFolderName - RegQueryValueEx(Folder) returned %d\r\n"),
                            status));

            *pcBytes = 0;
        }
        else
        {
            BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:GetFolderName - FolderName = %s, length = %d\r\n"),
                            FolderName, *pcBytes));

            *pcBytes += sizeof(WCHAR); // account for terminating 0.
        }

        RegCloseKey(DriverKey);

        if ((ERROR_SUCCESS != status) || (0 == *pcBytes))
        {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      GetFSDName                                                           */
/* DESCRIPTION                                                               */
/*      Function to retrieve the FSD(file system driver) value from the      */
/*      driver key. The FSD is used to load file system driver               */
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/*      FSDName
/*      cBytes      
/*      pcBytes     
/* RETURN VALUES                                                             */
/*      If it successes, it returns TRUE. otherwize it returns FALSE         */
/*                                                                           */
/*****************************************************************************/
static BOOL
GetFSDName(PDISK    pDisk,
           LPWSTR   FSDName,
           DWORD    cBytes,
           DWORD   *pcBytes)
{
    HKEY DriverKey;
    DWORD ValType;
    DWORD status;

    DriverKey = OpenDriverKey(pDisk->d_ActivePath);

    if (NULL != DriverKey)
    {
        *pcBytes = cBytes;
        status = RegQueryValueEx(DriverKey,         /* Handle to a currently open key   */
                                 TEXT("FSD"),       /* Pointer to quary                 */
                                 NULL,              /* Reserved - set to NULL           */
                                 &ValType,          /* Pointer to type of data          */
                                 (PUCHAR)FSDName,   /* Pointer to data                  */
                                 pcBytes);          /* the Length of data               */

        if (ERROR_SUCCESS != status)
        {
            BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:GetFSDName - RegQueryValueEx(FSD) returned %d\r\n"),
                            status));
            *pcBytes = 0;
        }
        else
        {
            BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:GetFSDName - FSDName = %s, length = %d\r\n"),
                            FSDName, *pcBytes));

            *pcBytes += sizeof(WCHAR); // account for terminating 0.
        }

        RegCloseKey(DriverKey);

        if ((ERROR_SUCCESS != status) || (0 == *pcBytes))
        {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

#if (CE_MAJOR_VER > 0x0003)

static BOOL
GetDeviceInfo(PDISK                 pDisk,
              PSTORAGEDEVICEINFO    psdi)
{
    HKEY DriverKey;
    DWORD ValType;
    DWORD status;
    DWORD dwSize;

    DriverKey = OpenDriverKey(pDisk->d_ActivePath);

    if (DriverKey)
    {
        dwSize = sizeof(psdi->szProfile);
        status = RegQueryValueEx(DriverKey,                 /* Handle to a currently open key   */
                                 TEXT("Profile"),           /* Pointer to quary                 */
                                 NULL,                      /* Reserved - set to NULL           */
                                 &ValType,                  /* Pointer to type of data          */
                                 (LPBYTE)psdi->szProfile,   /* Pointer to data                  */
                                 &dwSize);                  /* the Length of data               */

        if ((status != ERROR_SUCCESS) || (dwSize > sizeof(psdi->szProfile)))
        {
            BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:GetFolderName - RegQueryValueEx(Profile) returned %d\r\n"),
                            status));
            wcscpy( psdi->szProfile, L"Default");
        }
        else
        {
            BIBDRV_RTL_PRINT((TEXT("BIBDRV_PS:GetProfileName - Profile = %s, length = %d\r\n"),
                            psdi->szProfile, dwSize));
        }
        RegCloseKey(DriverKey);
    }

    psdi->cbSize        = sizeof(STORAGEDEVICEINFO);
    psdi->dwDeviceClass = STORAGE_DEVICE_CLASS_BLOCK;
    psdi->dwDeviceType  = STORAGE_DEVICE_TYPE_ATA;      //STORAGE_DEVICE_TYPE_FLASH;
    psdi->dwDeviceFlags = STORAGE_DEVICE_FLAG_READONLY; //STORAGE_DEVICE_FLAG_READWRITE;

    return TRUE;
}

#endif  //(CE_MAJOR_VER > 0x0003)


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      CloseDisk                                                            */
/* DESCRIPTION                                                               */
/*      free all resources associated with the specified disk                */
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/*                                                                           */
/*****************************************************************************/
static VOID
CloseDisk(PDISK pDisk)
{
    PDISK pd;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++CloseDisk() pDisk=0x%x\r\n"), pDisk));

    //
    // Remove it from the global list of disks
    //
    EnterCriticalSection(&gDiskCrit);
    
#if defined(_BIBDRV_SECTOR_ACCESS_STATISTIC_)
    LocalFree(pDisk->pSecHitStat);
#endif  //(_BIBDRV_SECTOR_ACCESS_STATISTIC_)

#if defined(_BIBDRV_CACHING_SECTORS_)
    BIBDRVCache_Deinit(pDisk->pCacheBuf);
#endif  //(_BIBDRV_CACHING_SECTORS_)

    if (pDisk == gDiskList)
    {
        gDiskList = pDisk->pd_next;
    }
    else
    {
        pd = gDiskList;
        while (pd->pd_next != NULL)
        {
            if (pd->pd_next == pDisk)
            {
                pd->pd_next = pDisk->pd_next;
                break;
            }
            pd = pd->pd_next;
        }
    }

    LeaveCriticalSection(&gDiskCrit);

    //
    // Try to ensure this is the only thread holding the disk crit sec
    //
    Sleep(50);
    EnterCriticalSection    (&(pDisk->d_DiskCardCrit));
    LeaveCriticalSection    (&(pDisk->d_DiskCardCrit));
    DeleteCriticalSection   (&(pDisk->d_DiskCardCrit));
    LocalFree(pDisk);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --CloseDisk() pDisk=0x%x\r\n"), pDisk));
}   





/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      RequestReadSecToHAL                                                  */
/* DESCRIPTION                                                               */
/*      free all resources associated with the specified disk                */
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/*      nSecNum     Number of Sectors                                        */
/*      pBuf        Pointer to Read buffer                                   */
/* RETURN VALUES                                                             */
/*      If it successes, it returns TRUE. otherwize it returns FALSE         */
/*                                                                           */
/*****************************************************************************/
static BOOL
RequestReadSecToHAL(PDISK   pDisk,
                    UINT    nSecNum,
                    UCHAR  *pBuf)
{
#if defined(_SUPPORT_HAL_WRAPPER_)
    VFLPacket   stPacket = {0,};
    INT         nResult = 0;
#endif
    BOOL        bRet = FALSE;
    Buffer      stBuf = {0,};
    UINT32      nStartPageAddr = 0;
    UINT32      nVsnAlign = 0;
    UINT        startSectorAddr = 0;
    UINT32      nVsnCnt = 0;
    UINT32      nSctToRead = 0;
    UINT8       *pTmpBuf = NULL;   // maximum value for 2 plane & internal interleaving & 2CE
    UINT32      uFullSectorBitmapPage = 0;
    DWORD       nCnt = 0, nBit1Cnt = 0;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++RequestReadSecToHAL()\r\n")));

    pTmpBuf = (UINT8 *)malloc(8192 * 2 * 2);

    if (pTmpBuf == NULL)
    {
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR] RequestReadSecToHAL - Memory allocation is failed!!\r\n")));
        goto RequestReadSecToHALError;
    }

	uFullSectorBitmapPage = ((1 << (stNandInfo.dwSectorsPerSuPage)) - 1);

#if defined(_SUPPORT_HAL_WRAPPER_)
	stBuf.eStatus	= BUF_AUX;
	stBuf.nBank	= 0;
	stBuf.pData	= (UINT8 *)pBuf;
	stBuf.pSpare	= NULL;
                
	stPacket.nVol		= pDisk->nVol;
	stPacket.nCtrlCode	= PM_HAL_VFL_READ;
	stPacket.nVbn		= 0;            // Not used
	stPacket.nVpn		= nSecNum + pDisk->nBaseVsn;
	stPacket.pBuf		= &stBuf;
	stPacket.nSrcVpn	= 0;
	stPacket.nDesVpn	= 0;
	stPacket.bCleanCheck	= FALSE32;

	BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  stPacket.nVol       = %d\r\n"),   stPacket.nVol));
	BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  stPacket.nCtrlCode  = 0x%x\r\n"), stPacket.nCtrlCode));
	BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  stPacket.nVpn       = %d\r\n"),   stPacket.nVpn));
	BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  stPacket.pBuf       = 0x%x\r\n"), stPacket.pBuf));

	KernelIoControl(IOCTL_POCKETSTOREII_CMD, /* IO Control Code                      */
				&stPacket,             /* Pointer to additional IO Control Code*/
				sizeof(stPacket),      /* Size of Input buffer                 */
				NULL,                  /* Pointer to out buffer                */
				0,                     /* Size of Output buffer                */
				&nResult);             /* Number of bytes returned             */

	if (VFL_SUCCESS != nResult)
	{
		BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  nResult    = %d\r\n"),   nResult));
		goto RequestReadSecToHALError;
	}

#else   //(_SUPPORT_HAL_WRAPPER_)
	startSectorAddr = nSecNum + pDisk->nBaseVsn;
	nVsnCnt     = 1;
	
	nVsnAlign = startSectorAddr % stNandInfo.dwSectorsPerSuPage;
	nStartPageAddr= startSectorAddr/stNandInfo.dwSectorsPerSuPage;

	if (nVsnAlign != 0)
	{
		if (nVsnCnt >= (stNandInfo.dwSectorsPerSuPage - nVsnAlign))
		{
			nSctToRead = stNandInfo.dwSectorsPerSuPage - nVsnAlign;
		}
		else
		{
			nSctToRead = nVsnCnt;
		}
	}
	else
	{
		if (nVsnCnt >= stNandInfo.dwSectorsPerSuPage)
		{
			nSctToRead = stNandInfo.dwSectorsPerSuPage;
		}
		else
		{
			nSctToRead = nVsnCnt;
		}
	}

	stBuf.eStatus	= BUF_AUX;
	stBuf.nBank	= 0;
	stBuf.pSpare	= NULL;
	stBuf.nBitmap = ((((uFullSectorBitmapPage >> (stNandInfo.dwSectorsPerSuPage - nSctToRead))&uFullSectorBitmapPage)<<nVsnAlign)&uFullSectorBitmapPage);

	if (((stBuf.nBitmap & 0x1) == 0x1) || (stBuf.nBitmap == 0x0))
	{
		stBuf.pData = (UINT8 *)pBuf;
	}
	else
	{
		stBuf.pData = (UINT8 *)pTmpBuf;
	}

#define MULTI_SECTOR_READ 1  // only 1 sector reading from above layer
#if MULTI_SECTOR_READ  // find the number of bit '1' at stBuf.nBitmap,
		// But i cannot find the nBit1Cnt value except 1.
		// so you must copy just 1 sector to pBuf from pTmpBuf after VFL_Read() in case stBuf.nBitmap is more 2.
	for (nCnt = 0; nCnt < stNandInfo.dwSectorsPerSuPage; nCnt++)
	{
		if ((stBuf.nBitmap & (1 << nCnt)) != 0)
			nBit1Cnt++;
	}
#endif

//	BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  nStartPageAddr       = 0x%x\r\n"),   nStartPageAddr));
//	BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  stBuf.nBitmap        = 0x%x\r\n"),   stBuf.nBitmap));
//	BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  stBuf.pData          = 0x%x\r\n"),   stBuf.pData));
	
	if (VFL_SUCCESS != VFL_Read(nStartPageAddr,
								&stBuf,
								FALSE32))
	{
		goto RequestReadSecToHALError;
	}

	for (nCnt = 0; nCnt < stNandInfo.dwSectorsPerSuPage; nCnt++)
	{
		if (nCnt == 0) continue;
		else if ((stBuf.nBitmap & (1 << nCnt)) != 0)
		{
#if MULTI_SECTOR_READ
			memcpy(pBuf, pTmpBuf+(BYTES_PER_SECTOR*nCnt), BYTES_PER_SECTOR*nBit1Cnt);  // the value of nBit1Cnt is only 1, so execute next line.
#else
			memcpy(pBuf, pTmpBuf+(BYTES_PER_SECTOR*nCnt), BYTES_PER_SECTOR);
#endif
			break;
		}
	}

#if 0
	{
		DWORD i;
		for (i = 0; i < 512; i++) {
			if (i % 16 == 0)
				RETAILMSG(1, (L"0x%x: ", pBuf+i));
			RETAILMSG(1, (L"%x%x ", (pBuf[i] >> 4) & 0x0f, pBuf[i] & 0x0f));
			if ((i + 1) % 16 == 0)
				RETAILMSG(1, (L"\r\n"));
		}
	}
#endif
#endif  //(_SUPPORT_HAL_WRAPPER_)

	bRet = TRUE;
    
RequestReadSecToHALError:

    if (pTmpBuf != NULL)
	{
        free(pTmpBuf);
        pTmpBuf = NULL;
    }

	BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --RequestReadSecToHAL()\r\n")));

	return bRet;
}

static void
ViewPage(UCHAR *pBuf)
{
    UINT nIdx1, nIdx2;
    
    RETAILMSG(1, (TEXT("=======================================================================\r\n")));
    for (nIdx1 = 0; nIdx1 < (512/16); nIdx1 ++)
    {
        RETAILMSG(1, (TEXT("%02X : "), nIdx1));
        for (nIdx2 = 0; nIdx2 < 16; nIdx2 ++)
        {
            RETAILMSG(1, (TEXT("%02X "), pBuf[nIdx1 * 16 + nIdx2]));
        }
        RETAILMSG(1, (TEXT("\r\n"), nIdx1));
    }
    RETAILMSG(1, (TEXT("=======================================================================\r\n")));
}

#if defined(_BIBDRV_CACHING_SECTORS_)

static BOOL
RequestReadSecToHALwithCaching(PDISK    pDisk,
                               UINT     nSecNum,
                               UCHAR   *pBuf)
{
#if defined(_SUPPORT_HAL_WRAPPER_)
//	FM_PACKET           stPacket;
	VFLPacket           stPacket;
	INT                 nResult;
	BIBDRV_SEC_CACHE    *pSecCache;

	pSecCache = BIBDRVCache_GetSecCache (pDisk->pCacheBuf, nSecNum);
	if (NULL != pSecCache)
	{
		memcpy(pBuf, pSecCache->aBuf, MAINPAGE_SIZE);
		return TRUE;
	}

	stPacket.nVol       = pDisk->nVol;
	stPacket.nCtrlCode  = PM_HAL_VFL_READ;
	stPacket.nVpn       = nSecNum + pDisk->nBaseVsn;
//	stPacket.nNumOfScts = 1;

	KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */                       
					&stPacket,              /* Input buffer (Additional Control Code) */
					sizeof(stPacket),       /* Size of Input buffer */                  
					pBuf,                   /* Output buffer */                         
					WMR_SECTOR_SIZE,        /* Size of Output buffer */                 
					&nResult);              /* Error Return */                          
                    
	if (BML_SUCCESS != nResult)
	{
		return FALSE;
	}

	BIBDRVCache_AddSecCache (pDisk->pCacheBuf, nSecNum, pBuf);

	return TRUE;
#else   //(_SUPPORT_HAL_WRAPPER_)
	VFLPacket	stPacket;
	INT			nResult;
	BOOL		bRet = FALSE;
	Buffer		stBuf;
	UINT32		nStartPageAddr;
	UINT32          nVsnAlign;
	UINT		startSectorAddr;
	UINT32          nVsnCnt;
	UINT32          nSctToRead;
	UINT32		uFullSectorBitmapPage;

	uFullSectorBitmapPage = ((1 << (stNandInfo.dwSectorsPerSuPage)) - 1);
	pSecCache = BIBDRVCache_GetSecCache (pDisk->pCacheBuf, nSecNum);
	if (NULL != pSecCache)
	{
		memcpy(pBuf, pSecCache->aBuf, MAINPAGE_SIZE);
		return TRUE;
	}

	startSectorAddr = nSecNum + pDisk->nBaseVsn;
	nVsnCnt     = 1;
	
	nVsnAlign = startSectorAddr % stNandInfo.dwSectorsPerSuPage;
	nStartPageAddr= startSectorAddr/stNandInfo.dwSectorsPerSuPage;

	if (nVsnAlign != 0)
	{
		if (nVsnCnt >= (stNandInfo.dwSectorsPerSuPage - nVsnAlign))
		{
			nSctToRead = stNandInfo.dwSectorsPerSuPage - nVsnAlign;
		}
		else
		{
			nSctToRead = nVsnCnt;
		}
	}
	else
	{
		if (nVsnCnt >= stNandInfo.dwSectorsPerSuPage)
		{
			nSctToRead = stNandInfo.dwSectorsPerSuPage;
		}
		else
		{
			nSctToRead = nVsnCnt;
		}
	}

	stBuf.eStatus	= BUF_AUX;
	stBuf.nBank	= 0;
	stBuf.pData	= (UINT8 *)pBuf;
	stBuf.pSpare	= NULL;
	stBuf.nBitmap = ((((uFullSectorBitmapPage >> (stNandInfo.dwSectorsPerSuPage - nSctToRead))&uFullSectorBitmapPage)<<nVsnAlign)&uFullSectorBitmapPage);

	if (VFL_SUCCESS != VFL_Read(nStartPageAddr,
								&stBuf,
								FALSE32))
	{
		return TRUE;
	}

	return FALSE;

#endif  //(_SUPPORT_HAL_WRAPPER_)
}

#endif  //(_BIBDRV_CACHING_SECTORS_)

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      DoDiskRead                                                           */
/* DESCRIPTION                                                               */
/*      Do read operation from NAND flash memory                             */
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/*      pData       PSQ_REQ structure pointer,it contains request information*/
/*                  for read operations                                      */
/* RETURN VALUES                                                             */
/*      If it successes, it returns TRUE. otherwize it returns FALSE         */
/*                                                                           */
/*****************************************************************************/
static DWORD
DoDiskRead(PDISK pDisk,
           PVOID pData)
{
    DWORD   status = ERROR_SUCCESS;
    DWORD   num_sg;
    DWORD   bytes_this_sg;
    PSG_REQ pSgr;
    PSG_BUF pSg;
    PUCHAR  pBuf;
    BOOL    nRes;
    UINT    nSecCount, nSecIdx;
    static UINT nAccessCnt = 0;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++DoDiskRead()\r\n")));

/*
typedef struct _SG_REQ {
    DWORD sr_start;     // starting sector number
	DWORD sr_num_sec;   // number of sectors
	DWORD sr_num_sg;    // number of scatter/gather buffers
    DWORD sr_status;    // request status
    PFN_REQDONE sr_callback;  // request completion callback function
	SG_BUF sr_sglist[1];   // first scatter/gather buffer
} SG_REQ, * PSG_REQ;
*/

    pSgr = (PSG_REQ) pData;

    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  ========= DoDiskRead Request Info ========= \r\n")));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tpSgr->sr_start    = %d(0x%x)\r\n"), pSgr->sr_start, pSgr->sr_start));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tpSgr->sr_num_sec  = %d\r\n"),       pSgr->sr_num_sec));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tpSgr->sr_num_sg   = %d\r\n"),       pSgr->sr_num_sg));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tlast sector num   = %d\r\n"),       pSgr->sr_start + pSgr->sr_num_sec - 1));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tpSgr->sr_status   = 0x%x\r\n"),     pSgr->sr_status));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  =========================================== \r\n")));
    
    /* BML Open Check */
    if (pDisk->bIsBMLOpen == FALSE)
    {
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  pDisk BML Open Check Fail\r\n")));
        status = ERROR_INVALID_PARAMETER;
        goto ddi_exit;
    }

    /* Scatter/Gather buffer Bound Check */
    if (pSgr->sr_num_sg > MAX_SG_BUF)
    {
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  Scatter/Gather buffer Bound Check Fail (Too many buffers)\r\n")));
        status = ERROR_INVALID_PARAMETER;
        goto ddi_exit;
    }

    pSgr->sr_status = ERROR_IO_PENDING;

    /*----------------------------------------------*/
    // Make sure request doesn't exceed the disk    */
    /*----------------------------------------------*/
    if ((pSgr->sr_start + pSgr->sr_num_sec - 1) > pDisk->d_DiskInfo.di_total_sectors)
    {
        status = ERROR_SECTOR_NOT_FOUND;
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  Request Sector OOB Check Fail(sector exceeded)\r\n")));
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  - Disk Totol Sectors        = %d\r\n"), pDisk->d_DiskInfo.di_total_sectors));
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  - Requested pSgr->sr_start  = %d\r\n"), pSgr->sr_start));
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  - Requested pSgr->sr_num_sec= %d\r\n"), pSgr->sr_num_sec));
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  - Requested last sector num = %d\r\n"), pSgr->sr_start + pSgr->sr_num_sec - 1));

        goto ddi_exit;
    }

    status          = ERROR_SUCCESS;
    num_sg          = pSgr->sr_num_sg;
    pSg             = &(pSgr->sr_sglist[0]);
    bytes_this_sg   = pSg->sb_len;
    pBuf            = MapPtrToProcess((LPVOID)pSg->sb_buf, GetCallerProcess());

    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  ----------------------------------- \r\n")));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tnum_sg        = %d\r\n"),     num_sg));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tpSg           = 0x%08x\r\n"), pSg));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tbytes_this_sg = %d\r\n"),     bytes_this_sg));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tpSg->sb_buf   = 0x%08x\r\n"), pSg->sb_buf));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  \tpBuf          = 0x%08x\r\n"), pBuf));
    BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  ----------------------------------- \r\n")));

    EnterCriticalSection(&(pDisk->d_DiskCardCrit));

    /*--------------------------*/
    /* Read sectors from disk.  */
    /*--------------------------*/
    while (num_sg)
    {
        nSecCount = ((bytes_this_sg - 1) / pDisk->d_DiskInfo.di_bytes_per_sect) + 1;
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF] DoDiskRead StartSector=%d, nNumOfScts=%d, nAccessCnt=%d\r\n"),
                        pSgr->sr_start, nSecCount, nAccessCnt++));

#if defined(_READ_PERFORMANCE_MEASUREMENT_)
        WatchTimerStart();
#endif  //(_READ_PERFORMANCE_MEASUREMENT_)

        for (nSecIdx = 0; nSecIdx < nSecCount; nSecIdx ++)
        {
            BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF] DoDiskRead %d bytes at sector %d\r\n"),
                            bytes_this_sg, pSgr->sr_start + nSecIdx));

            /*----------------------*/
            /* Sector(512byte) Read */
            /*----------------------*/
            nRes = RequestReadSecToHAL(pDisk, (UINT)pSgr->sr_start + nSecIdx, pBuf);

            if (nRes != TRUE)
            {
                BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR][DoDiskRead] %5d bytes at sector %5d, num_sg = %d\r\n"),
                                bytes_this_sg, pSgr->sr_start, num_sg));

                status = ERROR_SECTOR_NOT_FOUND;
                goto ddi_req_done;
            }

            pBuf += pDisk->d_DiskInfo.di_bytes_per_sect;
        }
        
#if defined(_READ_PERFORMANCE_MEASUREMENT_)
        WatchTimerStop();
#endif  //(_READ_PERFORMANCE_MEASUREMENT_)      

        //
        // Use the next scatter/gather buffer
        //
        num_sg --;
        if (num_sg == 0)
        {
            break;
        }

        pSg ++;

        pBuf = MapPtrToProcess((LPVOID)pSg->sb_buf, GetCallerProcess());
        bytes_this_sg = pSg->sb_len;
    }   // while sg

ddi_req_done:
    LeaveCriticalSection(&(pDisk->d_DiskCardCrit));

ddi_exit:

    pSgr->sr_status = status;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --DoDiskRead()\r\n")));

    return status;
}

#if defined(_BIBDRV_CACHING_SECTORS_)

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      DoDiskReadWithCaching                                                */
/* DESCRIPTION                                                               */
/*      Do read operation from NAND flash memory                             */
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/*      pData       PSQ_REQ structure pointer,it contains request information*/
/*                  for read operations                                      */
/* RETURN VALUES                                                             */
/*      If it successes, it returns TRUE. otherwize it returns FALSE         */
/*                                                                           */
/*****************************************************************************/
static DWORD
DoDiskReadWithCaching(PDISK pDisk,
                      PVOID pData)
{
    DWORD   status = ERROR_SUCCESS;
    DWORD   num_sg;
    DWORD   bytes_this_sg;
    PSG_REQ pSgr;
    PSG_BUF pSg;
    PUCHAR  pBuf;
    BOOL    nRes;
    UINT    nSecCount, nSecIdx;
    static UINT nAccessCnt = 0;


    /* BML Open Check */
    if (pDisk->bIsFDMOpen == FALSE)
    {
        status = ERROR_INVALID_PARAMETER;
        goto ddi_exit;
    }

    /* Scatter/Gather buffer Bound Check */
    pSgr = (PSG_REQ)pData;
    if (pSgr->sr_num_sg > MAX_SG_BUF)
    {
        status = ERROR_INVALID_PARAMETER;
        goto ddi_exit;
    }

    pSgr->sr_status = ERROR_IO_PENDING;

    //
    // Make sure request doesn't exceed the disk
    //
    if ((pSgr->sr_start + pSgr->sr_num_sec - 1) > pDisk->d_DiskInfo.di_total_sectors)
    {
        status = ERROR_SECTOR_NOT_FOUND;
        
        BIBDRV_ERR_PRINT((TEXT("[DoDiskRead] not found at sector = %d sec_num = %d\r\n"),
                        pSgr->sr_start, pSgr->sr_num_sec));

        BIBDRV_ERR_PRINT((TEXT("[DoDiskRead] di_total_sectors = %d\r\n"), pDisk->d_DiskInfo.di_total_sectors));

        goto ddi_exit;
    }

    status          = ERROR_SUCCESS;
    num_sg          = pSgr->sr_num_sg;
    pSg             = &(pSgr->sr_sglist[0]);
    bytes_this_sg   = pSg->sb_len;
    pBuf            = MapPtrToProcess((LPVOID)pSg->sb_buf, GetCallerProcess());

    BIBDRV_INF_PRINT((TEXT("[DoDiskRead] %5d bytes at sector %5d, num_sg = %d\r\n"),
                    bytes_this_sg, pSgr->sr_start, num_sg));

    EnterCriticalSection(&(pDisk->d_DiskCardCrit));
    //
    // Read sectors from disk.
    //
    while (num_sg)
    {
        nSecCount = ((bytes_this_sg - 1) / pDisk->d_DiskInfo.di_bytes_per_sect) + 1;
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF] Read StartSector=%d, nNumOfScts=%d, nAccessCnt=%d\r\n"),
                        pSgr->sr_start, nSecCount, nAccessCnt++));
                            
#if defined(_READ_PERFORMANCE_MEASUREMENT_)
        WatchTimerStart();
#endif  //(_READ_PERFORMANCE_MEASUREMENT_)

        for (nSecIdx = 0; nSecIdx < nSecCount; nSecIdx ++)
        {
            BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF] DoDiskRead %d bytes at sector %d\r\n"),
                            bytes_this_sg, pSgr->sr_start + nSecIdx));

            /*----------------------*/
            /* Sector(512byte) Read */
            /*----------------------*/
            nRes = RequestReadSecToHALwithCaching(pDisk, (UINT) pSgr->sr_start + nSecIdx, pBuf);

            if (nRes != TRUE)
            {
                BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR][DoDiskReadWithCaching] %5d bytes at sector %5d, num_sg = %d\r\n"),
                                bytes_this_sg, pSgr->sr_start, num_sg));

                status = ERROR_SECTOR_NOT_FOUND;
                goto ddi_req_done;
            }

            pBuf += pDisk->d_DiskInfo.di_bytes_per_sect;
        }
        
#if defined(_READ_PERFORMANCE_MEASUREMENT_)
        WatchTimerStop();
#endif  //(_READ_PERFORMANCE_MEASUREMENT_)      

        //
        // Use the next scatter/gather buffer
        //
        num_sg --;
        if (num_sg == 0)
        {
            break;
        }

        pSg ++;

        pBuf = MapPtrToProcess((LPVOID)pSg->sb_buf, GetCallerProcess());
        bytes_this_sg = pSg->sb_len;
    }   // while sg

ddi_req_done:
    LeaveCriticalSection(&(pDisk->d_DiskCardCrit));

ddi_exit:
    pSgr->sr_status = status;
    return status;
}

#endif //(_BIBDRV_CACHING_SECTORS_)

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      GetDiskInfo                                                          */
/* DESCRIPTION                                                               */
/*      Get disk information from pDisk structure                            */
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/*      pInfo       DISK Information structure pointer                       */
/* RETURN VALUES                                                             */
/*      it always returns ERROR_SUCCESS                                      */
/*                                                                           */
/*****************************************************************************/
static DWORD
GetDiskInfo(PDISK       pDisk,
            PDISK_INFO  pInfo)
{    
    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++GetDiskInfo()\r\n")));

    memcpy(pInfo, &(pDisk->d_DiskInfo), sizeof(DISK_INFO));
    pInfo->di_flags &= ~DISK_INFO_FLAG_UNFORMATTED;
    

    BIBDRV_INF_PRINT((TEXT("[NFALT:INF] \tpInfo->di_total_sectors    =%d\r\n"), pInfo->di_total_sectors));
    BIBDRV_INF_PRINT((TEXT("[NFALT:INF] \tpInfo->di_bytes_per_sect   =%d\r\n"), pInfo->di_bytes_per_sect));
    BIBDRV_INF_PRINT((TEXT("[NFALT:INF] \tpInfo->di_cylinders        =%d\r\n"), pInfo->di_cylinders));
    BIBDRV_INF_PRINT((TEXT("[NFALT:INF] \tpInfo->di_heads            =%d\r\n"), pInfo->di_heads));
    BIBDRV_INF_PRINT((TEXT("[NFALT:INF] \tpInfo->di_sectors          =%d\r\n"), pInfo->di_sectors));
    BIBDRV_INF_PRINT((TEXT("[NFALT:INF] \tpInfo->di_flags            =%X\r\n"), pInfo->di_flags));

    /*
    The device supports demand paging.
    Read and write requests are synchronous and do not involve
    memory manager calls, loader operations, or thread switches.
    pInfo->di_flags |= DISK_INFO_FLAG_PAGEABLE;
    */

    /*
    The device does not support CHS addressing;
    values for di_cylinders, di_heads, and di_sectors may be simulations,
    estimations, or not provided.

    pInfo->di_flags |= DISK_INFO_FLAG_CHS_UNCERTAIN;
    */

    /*
    The device requires a low-level format with the IOCTL_DISK_FORMAT_MEDIA.
    The FAT file system currently ignores this flag.
    */

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --GetDiskInfo()\r\n")));

    return ERROR_SUCCESS;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      SetDiskInfo                                                          */
/* DESCRIPTION                                                               */
/*      Set disk information to pDisk structure                              */
/* PARAMETERS                                                                */
/*      pDisk       BIBDRV_PS driver own structure pointer                    */
/*      pInfo       DISK Information structure pointer                       */
/* RETURN VALUES                                                             */
/*      it always returns ERROR_SUCCESS                                      */
/*                                                                           */
/*****************************************************************************/
static DWORD
SetDiskInfo(PDISK       pDisk,
            PDISK_INFO  pInfo)
{
    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++SetDiskInfo()\r\n")));

    pDisk->d_DiskInfo = *pInfo;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --SetDiskInfo()\r\n")));

    return ERROR_SUCCESS;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      CreateDiskObject                                                     */
/* DESCRIPTION                                                               */
/*      Create a DISK structure, init some fields and link it.               */
/* PARAMETERS                                                                */
/*      none                                                                 */
/* RETURN VALUES                                                             */
/*      new DISK structure pointer                                           */
/*                                                                           */
/*****************************************************************************/
static PDISK
CreateDiskObject(VOID)
{
    PDISK pDisk;
    
    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++CreateDiskObject()\r\n")));
    
    pDisk = LocalAlloc(LPTR, sizeof(DISK));

    if (pDisk != NULL)
    {
        pDisk->hDevice      = NULL;
        pDisk->d_OpenCount  = 0;
        pDisk->nBaseVsn     = 0;
        pDisk->d_ActivePath = NULL;
        pDisk->bIsBMLOpen   = FALSE;
        
        /* Initialize CiriticalSection for Disk Object(NAND flash) */
        InitializeCriticalSection(&(pDisk->d_DiskCardCrit));

        EnterCriticalSection(&gDiskCrit);
        pDisk->pd_next      = gDiskList;
        gDiskList           = pDisk;
        LeaveCriticalSection(&gDiskCrit);
    }
    
    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --CreateDiskObject()\r\n")));

    return pDisk;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      IsValidDisk                                                          */
/* DESCRIPTION                                                               */
/*      This function checks Disk validation                                 */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Pointer to disk handle                                           */
/* RETURN VALUES                                                             */
/*      Return TRUE if pDisk is valid, FALSE if not.                         */
/*                                                                           */
/*****************************************************************************/
static BOOL
IsValidDisk(PDISK pDisk)
{
    PDISK   pd;
    BOOL    bRet = FALSE;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++IsValidDisk()\r\n")));
 
    EnterCriticalSection(&gDiskCrit);

    pd = gDiskList;
    while (pd) 
    {
        if (pd == pDisk)
        {
            bRet = TRUE;
            break;
        }
        pd = pd->pd_next;
    }

    LeaveCriticalSection(&gDiskCrit);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --IsValidDisk()\r\n")));

    return bRet;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      BIBDrv_Entry                                                         */
/* DESCRIPTION                                                               */
/*      This function is BIBDRV_PS.dll Entry Point                            */
/* PARAMETERS                                                                */
/*      DllInstance                                                          */
/*      Reason                                                               */
/*      Reserved                                                             */
/* RETURN VALUES                                                             */
/*      it always returns TRUE                                               */
/*                                                                           */
/*****************************************************************************/
BOOL WINAPI
BIBDrv_Entry(HINSTANCE    DllInstance, 
             INT          Reason, 
             LPVOID       Reserved)
{
    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++BIBDRVEntry()\r\n")));

    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  DLL_PROCESS_ATTACH\r\n")));
            DEBUGREGISTER(DllInstance);
            break;
    
        case DLL_PROCESS_DETACH:
            BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  DLL_PROCESS_DETACH\r\n")));
            DeleteCriticalSection(&gDiskCrit);
            break;
    }

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --BIBDRVEntry()\r\n")));
    return TRUE;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      InitializeNAND                                                       */
/* DESCRIPTION                                                               */
/*      This function initializes NAND Disk Handle                           */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Disk handle                                                      */
/* RETURN VALUES                                                             */
/*      Return TRUE if pDisk is valid, FALSE if not.                         */
/* NOTE                                                                      */
/*      call from InitDisk from DSK_Init                                     */
/*                                                                           */
/*****************************************************************************/
static BOOL 
InitializeNAND(PDISK pDisk)
{
#if defined(_SUPPORT_HAL_WRAPPER_)
    VFLPacket   stPacket;
    INT         nResult;
#endif    
    BOOL        bRet = FALSE;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++InitializeNAND()\r\n")));

#if defined(_SUPPORT_HAL_WRAPPER_)
    do {
        /*--------------------*/
        /* BML Volume Setting */
        /*--------------------*/
        stPacket.nVol      = pDisk->nVol;  

        /* BML Init */
        stPacket.nCtrlCode = PM_HAL_VFL_INIT;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,              /* Input buffer (Additional Control Code) */
                        sizeof(stPacket),       /* Size of Input buffer */
                        NULL,                   /* Output buffer */
                        0,                      /* Size of Output buffer */
                        &nResult);              /* Error Return */
    
        if (nResult != VFL_SUCCESS)
        {
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  BML_Init() failure. ERR Code=%x\r\n"), nResult));
    
            break;
        }

        /* BML Open */
        stPacket.nCtrlCode = PM_HAL_VFL_OPEN;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* Io Control Code */
                        &stPacket,              /* Input buffer (Additional Control Code */
                        sizeof(stPacket),       /* Size of Input buffer */
                        NULL,                   /* Output buffer */
                        0,                      /* Size of Output buffer */
                        &nResult);              /* Error Return */
    
        if (nResult != VFL_SUCCESS)
        {
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  BML_Open() failure. ERR Code=%x\r\n"), nResult));
    
            break;
        }

        bRet = TRUE;
        
    } while(0);

#else   //_SUPPORT_HAL_WRAPPER_

	do {
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] not _SUPPORT_HAL_WRAPPER_\r\n")));

		if (FIL_Init() != VFL_SUCCESS)
		{
			BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  FIL_Init() failure.\r\n")));
			break;
		}

		if (VFL_Init() != VFL_SUCCESS)
		{
			BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  VFL_Init() failure.\r\n")));
			break;
		}

		if (VFL_Open() != VFL_SUCCESS)
		{
			BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  VFL_Open() failure.\r\n")));
			break;
		}

		GetNandInfo(&stNandInfo);

		BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  stNandInfo.dwPagesPerSuBlk      = 0x%X\r\n"), stNandInfo.dwPagesPerSuBlk));
		BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  stNandInfo.dwSectorsPerSuPage   = 0x%X\r\n"), stNandInfo.dwSectorsPerSuPage));
		BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  stNandInfo.dwSpecialAreaSize    = 0x%X\r\n"), stNandInfo.dwSpecialAreaSize));
		BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  stNandInfo.dwSpecialAreaStart   = 0x%X\r\n"), stNandInfo.dwSpecialAreaStart));

		bRet = TRUE;
	} while(0);

#endif  //_SUPPORT_HAL_WRAPPER_

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --InitializeNAND()\r\n")));

    return bRet;
}

#if 0
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      CheckSamsungNANDFlashMemory                                          */
/* DESCRIPTION                                                               */
/*      This function checks if Samsung NAND flash memory is                 */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Disk handle                                                      */
/* RETURN VALUES                                                             */
/*      Return TRUE if device id is samsung code, FALSE if not.              */
/* NOTE                                                                      */
/*      call from InitDisk from DSK_Init                                     */
/*                                                                           */
/*****************************************************************************/
#define SAMSUNG_DEVICE_ID           0xEC

static BOOL 
CheckSamsungNANDFlashMemory(PDISK pDisk)
{
    UCHAR aXID[512];    
    BOOL  bRet = FALSE;

#if defined(_SUPPORT_HAL_WRAPPER_)
    VFLPacket   stPacket;
    INT         nResult;
#else   //_SUPPORT_HAL_WRAPPER_
    BMLVolSpec  stVolSpec;
#endif  //_SUPPORT_HAL_WRAPPER_

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++CheckSamsungNANDFlashMemory()\r\n")));

    do {

#if defined(_SUPPORT_HAL_WRAPPER_)

        /* BML Volume Setting */
        stPacket.nVol = pDisk->nVol;
        
        /* BML Read XID */
        stPacket.nCtrlCode = PS_HAL_BML_READXID;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD, 
                        &stPacket, 
                        sizeof(stPacket), 
                        aXID, 
                        512, 
                        &nResult);
                        
        if (nResult != BML_SUCCESS)
        {
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR] READXID failure. ERRNO=%d\r\n"), nResult));
            break;
        }
    
#else   //_SUPPORT_HAL_WRAPPER_

        if (BML_GetVolInfo(pDisk->nVol, &stVolSpec) != BML_SUCCESS)
        {
            BDRV_ERR_PRINT((TEXT("[BDRV:ERR] BML_GetVolInfo failure. \r\n")));
            break;
        }

        memcpy(aXID, stVolSpec.aUID, XSR_UID_SIZE);

#endif  //_SUPPORT_HAL_WRAPPER_

        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF] MANUFACTURE ID=0x%X\r\n"), aXID[0]));
    
        if (SAMSUNG_DEVICE_ID != aXID[0])
        {
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR] SAMSUNG_DEVICE_ID Error ID=0x%X\r\n"), aXID[0]));
            break;
        }

        bRet = TRUE;

    } while(0);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --CheckSamsungNANDFlashMemory()\r\n")));

    return bRet;
}
#endif

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      ViewMBR                                                              */
/* DESCRIPTION                                                               */
/*      This function prints MBR contents                                    */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Disk handle                                                      */
/* RETURN VALUES                                                             */
/*      none                                                                 */
/* NOTE                                                                      */
/*                                                                           */
/*****************************************************************************/
#if defined(_BIBDRV_MBR_DEBUG_)
#include <MkMBR.h>

void 
ViewMBR(PDISK pDisk)
{
    UCHAR       aBuf[528];
    UCHAR      *pBuf;
    PARTENTRY  *pPartEntry;
    
    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] ++ViewMBR()\r\n")));
    
    pBuf       = &aBuf[0];
    pPartEntry = (PARTENTRY *) (pBuf + PARTTABLE_OFFSET);
    
    do {
        if (RequestReadSecToHAL(pDisk, 0, aBuf) == FALSE)
        {
            RETAILMSG(1, (TEXT("[BIBDRV:INF]  BIBDRV_PS:Read Sector 0 failure.\r\n")));
            break;
        }

        /*------------------------------------------*/
        /*    g_pbMBRSector[0] = 0xE9;              */
        /*    g_pbMBRSector[1] = 0xfd;              */
        /*    g_pbMBRSector[2] = 0xff;              */
        /*    g_pbMBRSector[SECTOR_SIZE-2] = 0x55;  */
        /*    g_pbMBRSector[SECTOR_SIZE-1] = 0xAA;  */
        /*------------------------------------------*/
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  MBR MAGIC CODE (First): 0x%x,0x%x,0x%x\r\n"), aBuf[0], aBuf[1], aBuf[2]));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  MBR MAGIC CODE (Last) : 0x%x,0x%x\r\n"),      aBuf[512 - 2], aBuf[512 - 1]));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_BootInd      = 0x%X\r\n"), pPartEntry[0].Part_BootInd));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_FirstHead    = 0x%X\r\n"), pPartEntry[0].Part_FirstHead));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_FirstSector  = 0x%X\r\n"), pPartEntry[0].Part_FirstSector));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_FirstTrack   = 0x%X\r\n"), pPartEntry[0].Part_FirstTrack));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_FileSystem   = 0x%X\r\n"), pPartEntry[0].Part_FileSystem));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_LastHead     = 0x%X\r\n"), pPartEntry[0].Part_LastHead));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_LastSector   = 0x%X\r\n"), pPartEntry[0].Part_LastSector));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_LastTrack    = 0x%X\r\n"), pPartEntry[0].Part_LastTrack));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_StartSector  = %d\r\n"),   pPartEntry[0].Part_StartSector));
        BIBDRV_RTL_PRINT((TEXT("[BIBDRV:INF]  Part_TotalSectors = %d\r\n"),   pPartEntry[0].Part_TotalSectors));
    } while(0);
    
    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --ViewMBR()\r\n")));
}

#endif  //(_BIBDRV_MBR_DEBUG_)

#if 0 // for Whimory by hmseo 061126
/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      RequestGetXSRPartInfoToHAL                                           */
/* DESCRIPTION                                                               */
/*      This function gets XSR Partition Information                         */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Disk handle                                                      */
/*      pstPartI                                                             */
/*          Pointer to Partition Information                                 */
/* RETURN VALUES                                                             */
/*      Return TRUE if Operation success, FALSE if not.                      */
/* NOTE                                                                      */
/*                                                                           */
/*****************************************************************************/
static BOOL
RequestGetXSRPartInfoToHAL(PDISK pDisk, XSRPartI *pstPartI)
{
    VFLPacket       stPacket;
    BMLIOCtlPacket  stBMLIOCtl;
    INT             nResult;
    UINT            nByteRet;
    BOOL            bRet = FALSE;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++RequestGetXSRPartInfoToHAL()\r\n")));

    do {
        stPacket.nVol       = pDisk->nVol;
        stPacket.nCtrlCode  = PS_HAL_BML_IOCTL;
        stPacket.nVpn       = 0;    // Not used
//        stPacket.nNumOfScts = 0;    // Not used
    
        /* IOControl Code Setting */
        stBMLIOCtl.nCode    = BML_IOCTL_GET_FULL_PI;
        stBMLIOCtl.pBufIn   = NULL;
        stBMLIOCtl.nLenIn   = 0;
        stBMLIOCtl.pBufOut  = (UINT8*)pstPartI;
        stBMLIOCtl.nLenOut  = sizeof(XSRPartI);
        stBMLIOCtl.pByteRet = &nByteRet;
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD, 
                        &stPacket, 
                        sizeof(stPacket),
                        (LPVOID)&stBMLIOCtl, 
                        sizeof(BMLIOCtlPacket), 
                        &nResult);
                        
        if (nResult != BML_SUCCESS)
        {
            break;
        }

        bRet = TRUE;
        
    } while(0);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --RequestGetXSRPartInfoToHAL()\r\n")));

    return bRet;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      RequestGetXSRPartEntryInfoToHAL                                      */
/* DESCRIPTION                                                               */
/*      This function gets XSR Partition Entry Information                   */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Disk handle                                                      */
/*      pstPartE                                                             */
/*          Pointer to Partition Entry Information                           */
/* RETURN VALUES                                                             */
/*      Return TRUE if Operation success, FALSE if not.                      */
/* NOTE                                                                      */
/*                                                                           */
/*****************************************************************************/
static BOOL
RequestGetXSRPartEntryInfoToHAL(PDISK pDisk, XSRPartEntry *pstPartE)
{
    VFLPacket   stPacket; 
    INT         nResult;
    BOOL        bRet = FALSE;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++RequestGetXSRPartEntryInfoToHAL()\r\n")));

    do {
        /* Get Partition Entry */
        stPacket.nVol       = pDisk->nVol;
        stPacket.nCtrlCode  = PS_HAL_BML_GET_PARTITIONENTRYINFO;
        stPacket.nVpn       = 0;    // Not used
//        stPacket.nNumOfScts = 0;    // Not used

        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  XSRPartition ID to find = 0x%x\r\n"), pstPartE->nID));
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,    /* IO Control Code  */
                        &stPacket,                  /* Input buffer     */
                        sizeof(stPacket),
                        pstPartE,                   /* Output buffer    */
                        sizeof(XSRPartEntry), 
                        &nResult);
                        
        if (nResult != BML_SUCCESS)
        {
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  PS_HAL_BML_GET_PARTITIONENTRYINFO Error\r\n")));
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  Cannot find Partition Entry Info\r\n")));
            break;
        }

        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  XSRPartition ID(0x%x) is found\r\n"), pstPartE->nID));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  +------ XSR Partition Entry Info ------+\r\n")));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstPartE->nAttr       = %d\r\n"), pstPartE->nAttr));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstPartE->n1stVbn     = %d\r\n"), pstPartE->n1stVbn));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstPartE->nNumOfBlks  = %d\r\n"), pstPartE->nNumOfBlks));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  +--------------------------------------+\r\n")));
        
        bRet = TRUE;
        
    } while(0);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --RequestGetXSRPartEntryInfoToHAL()\r\n")));

    return bRet;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      RequestGetXSRVolumeInfoToHAL                                         */
/* DESCRIPTION                                                               */
/*      This function gets XSR Volume Information                            */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Disk handle                                                      */
/*      pstVolSpec                                                           */
/*          Pointer to Volume Spec                                           */
/* RETURN VALUES                                                             */
/*      Return TRUE if Operation success, FALSE if not.                      */
/* NOTE                                                                      */
/*                                                                           */
/*****************************************************************************/
static BOOL
RequestGetXSRVolumeInfoToHAL(PDISK pDisk, BMLVolSpec *pstVolSpec)
{
    VFLPacket   stPacket; 
    INT         nResult;
    BOOL        bRet = FALSE;
    INT         nCnt;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++RequestGetXSRVolumeInfoToHAL()\r\n")));

    do {

        /* Get Partition Entry */
        stPacket.nVol       = pDisk->nVol;
        stPacket.nCtrlCode  = PS_HAL_BML_GET_VOLUMEINFO;
        stPacket.nVpn       = 0;    // Not used
//        stPacket.nNumOfScts = 0;    // Not used
    
        KernelIoControl(IOCTL_POCKETSTOREII_CMD,    /* IO Control Code  */
                        &stPacket,                  /* Input buffer     */
                        sizeof(stPacket),
                        pstVolSpec,                 /* Output buffer    */
                        sizeof(BMLVolSpec), 
                        &nResult);
                        
        if (nResult != BML_SUCCESS)
        {
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  PS_HAL_BML_GET_VOLUMEINFO Error\r\n")));
            break;
        }

        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  +------ BML Volume Spec Info ------+\r\n")));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->nPgsPerBlk    = %d\r\n"), pstVolSpec->nPgsPerBlk));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->nSctsPerPg    = %d\r\n"), pstVolSpec->nSctsPerPg));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->nLsnPos       = %d\r\n"), pstVolSpec->nLsnPos));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->nTrTime       = %d\r\n"), pstVolSpec->nTrTime));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->nTwTime       = %d\r\n"), pstVolSpec->nTwTime));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->nTeTime       = %d\r\n"), pstVolSpec->nTeTime));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->nNumOfUsBlks  = %d\r\n"), pstVolSpec->nNumOfUsBlks));
        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  | pstVolSpec->aUID          = ")));
        
        for (nCnt = 0; nCnt < XSR_UID_SIZE; nCnt++)
        {
            BIBDRV_INF_PRINT((TEXT("%02x "),  pstVolSpec->aUID[nCnt]));
        }
        BIBDRV_INF_PRINT((TEXT("\r\n[BIBDRV:INF]  +----------------------------------+\r\n")));
        
        bRet = TRUE;
        
    } while(0);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --RequestGetXSRVolumeInfoToHAL()\r\n")));

    return bRet;
}
#endif


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      InitDisk                                                             */
/* DESCRIPTION                                                               */
/*      This function initializes Disk handle                                */
/* PARAMETERS                                                                */
/*      pDisk                                                                */
/*          Disk handle                                                      */
/*      ActiveKey                                                            */
/*          Pointer to active key                                            */
/* RETURN VALUES                                                             */
/*      Return TRUE if Operation success, FALSE if not.                      */
/* NOTE                                                                      */
/*      call from DSK_Init                                                   */
/*                                                                           */
/*****************************************************************************/
static BOOL
InitDisk(PDISK  pDisk,
         LPTSTR ActiveKey)
{
	BOOL            bRet = FALSE;
//	XSRPartEntry    stXSRPartE;
//	BMLVolSpec      stVolSpec;

	BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++InitDisk() Entered\r\n")));

	do {
#if defined(_BIBDRV_CACHING_SECTORS_)

		BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  BIBDRVCache_Init\r\n")));

		/*--------------------------------------*/
		/* Note                                 */
		/*                                      */
		/* NUM_OF_SEC_CACHE_ROW             128 */
		/* NUM_OF_SEC_CACHE_COLUMN          8   */
		/*--------------------------------------*/
		pDisk->nNumOfSecCache   = NUM_OF_SEC_CACHE_ROW * NUM_OF_SEC_CACHE_COLUMN;
		pDisk->pCacheBuf        = BIBDRVCache_Init(pDisk->nNumOfSecCache);

		if (pDisk->pCacheBuf == NULL)
		{
			BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  BIBDRVCache_Init Error\r\n")));
			break;
		}

#endif  //(_BIBDRV_CACHING_SECTORS_)

    
		if (InitializeNAND(pDisk) == FALSE)
		{
			pDisk->bIsBMLOpen = FALSE;
			BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  NAND Initializion Error\r\n")));
			break;
		}
		else
		{
			pDisk->bIsBMLOpen = TRUE;
			BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  NAND Initializion Success\r\n")));
		}
    
		{
			UINT32  nPgsPerBlk;
			UINT32  nSctsPerPg;
			UINT32  nSctsPerBlk;

			nPgsPerBlk  = (UINT32)stNandInfo.dwPagesPerSuBlk;
			nSctsPerPg  = (UINT32)stNandInfo.dwSectorsPerSuPage;
			nSctsPerBlk = nSctsPerPg * nPgsPerBlk;

			pDisk->nBaseVsn = (stNandInfo.dwSpecialAreaStart)* nSctsPerBlk;

			// MBR sector is always located in PARTITION_ID_DEMANDONOS partition area.
			// MBR is the first sector of PARTITION_ID_DEMANDONOS partition area.
			pDisk->d_DiskInfo.di_total_sectors  = (stNandInfo.dwSpecialAreaSize) * nSctsPerBlk;	// 3 is for TOC and Eboot.
			pDisk->d_DiskInfo.di_bytes_per_sect = WMR_SECTOR_SIZE;
			pDisk->d_DiskInfo.di_cylinders      = 1;
			pDisk->d_DiskInfo.di_heads          = 1;
			pDisk->d_DiskInfo.di_sectors        = pDisk->d_DiskInfo.di_total_sectors;
			pDisk->d_DiskInfo.di_flags          = DISK_INFO_FLAG_PAGEABLE;  // | DISK_INFO_FLAG_CHS_UNCERTAIN;
		}    
    
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV:INF] nBaseSctorNum    = %d\r\n"), pDisk->nBaseVsn));
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV:INF] total_sectors    = %d\r\n"), pDisk->d_DiskInfo.di_total_sectors));
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV:INF] bytes_per_sect   = %d\r\n"), pDisk->d_DiskInfo.di_bytes_per_sect));
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV:INF] di_cylinders     = %d\r\n"), pDisk->d_DiskInfo.di_cylinders));
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV:INF] di_heads         = %d\r\n"), pDisk->d_DiskInfo.di_heads));
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV:INF] di_sectors       = %d\r\n"), pDisk->d_DiskInfo.di_sectors));
		BIBDRV_LOG_PRINT((TEXT("[BIBDRV:INF] storage size     = %d bytes\r\n"), 
					pDisk->d_DiskInfo.di_total_sectors * pDisk->d_DiskInfo.di_bytes_per_sect));


#if defined(_BIBDRV_MBR_DEBUG_)
		ViewMBR(pDisk);
#endif  //(_BIBDRV_MBR_DEBUG_)

		bRet = TRUE;

	} while(0);
    
	return bRet;
}

/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      DSK_Init                                                             */
/* DESCRIPTION                                                               */
/*      Create Disk Object, Initialize Disk Object                           */
/* PARAMETERS                                                                */
/*      dwContext   BIBDRV_PS driver own structure pointer                    */
/* RETURN VALUES                                                             */
/*      Returns context data for this Init instance                          */
/*                                                                           */
/*****************************************************************************/
DWORD
DSK_Init(DWORD dwContext)
{
    PDISK   pDisk;
    LPWSTR  ActivePath  = (LPWSTR)dwContext;
    LPWSTR  ActivePath2 = (LPWSTR)dwContext;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++DSK_Init() Entered\r\n")));

    if (gDiskList == NULL)
    {
        InitializeCriticalSection(&gDiskCrit);
    }

    pDisk = CreateDiskObject();
    
    if (pDisk == NULL)
    {
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  LocalAlloc(PDISK) failed %d\r\n"), GetLastError()));
       
        return 0;
    }

    if (ActivePath)
    {
        // for using loader
        ActivePath2 = (LPWSTR)MapPtrToProcess((LPVOID)ActivePath, GetCallerProcess());

        if (NULL != ActivePath2)
        {
            ActivePath = ActivePath2;
        }

        if (pDisk->d_ActivePath = LocalAlloc(LPTR, wcslen(ActivePath) * sizeof(WCHAR) + sizeof(WCHAR)))
        {
            /* Copy Active Path to Dist structure */
            wcscpy(pDisk->d_ActivePath, ActivePath);
        }

        BIBDRV_INF_PRINT((TEXT("[BIBDRV:INF]  ActiveKey (copy) = %s (@ 0x%08X)\r\n"),
                        pDisk->d_ActivePath, pDisk->d_ActivePath));
    }

    if (InitDisk(pDisk, ActivePath) == FALSE)
    {
        /* If pDisk already memory allocated, it should be deallocated */
        if (pDisk)
        {
            if (pDisk->d_ActivePath)
            {
                /* Counter function of LocalAlloc */
                LocalFree(pDisk->d_ActivePath);
                pDisk->d_ActivePath = NULL;
            }
            CloseDisk(pDisk);
        }    
        return 0;
    }

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --DSK_Init() returning pDisk=0x%x\r\n"), pDisk));

    return (DWORD)pDisk;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      DSK_Close                                                            */
/* DESCRIPTION                                                               */
/*      This function Close Disk                                             */
/* PARAMETERS                                                                */
/*      Handle                                                               */
/*          Disk handle                                                      */
/* RETURN VALUES                                                             */
/*      Return TRUE if Operation success, FALSE if not.                      */
/* NOTE                                                                      */
/*      call from DSK_Deinit                                                 */
/*                                                                           */
/*****************************************************************************/
BOOL
DSK_Close(DWORD Handle)
{
    PDISK pDisk = (PDISK)Handle;
    BOOL bClose = FALSE;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++DSK_Close()\r\n")));

    do {
        if (!IsValidDisk(pDisk))
        {
            break;
        }
        
        EnterCriticalSection(&(pDisk->d_DiskCardCrit));
    
        pDisk->d_OpenCount--;

        if (pDisk->d_OpenCount <= 0)
        {
            pDisk->d_OpenCount = 0;
        }

        LeaveCriticalSection(&(pDisk->d_DiskCardCrit));

        bClose = TRUE;

    } while(0);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --DSK_Close\r\n")));

    return bClose;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      DSK_Deinit                                                           */
/* DESCRIPTION                                                               */
/*      This function deinitializes Disk                                     */
/* PARAMETERS                                                                */
/*      dwContext                                                            */
/*          Disk handle                                                      */
/* RETURN VALUES                                                             */
/*      Return TRUE if Operation success, FALSE if not.                      */
/* NOTE                                                                      */
/*      Device deinit - devices are expected to close down.                  */
/*      The device manager does not check the return code.                   */
/*                                                                           */
/*****************************************************************************/
BOOL
DSK_Deinit(DWORD dwContext) // future: pointer to the per disk structure
{
    PDISK pDisk = (PDISK)dwContext;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++DSK_Deinit()\r\n")));

    DSK_Close(dwContext);
    CloseDisk((PDISK)dwContext);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --DSK_Deinit()\r\n")));

    return TRUE;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      DSK_Open                                                             */
/* DESCRIPTION                                                               */
/*      This function opens Disk                                             */
/* PARAMETERS                                                                */
/*      dwData                                                               */
/*          Disk handle                                                      */
/*      dwAccess                                                             */
/*          Not used                                                         */
/*      dwShareMode                                                          */
/*          Not used                                                         */
/* RETURN VALUES                                                             */
/*      Return address of pDisk(disk handle)                                 */
/* NOTE                                                                      */
/*                                                                           */
/*****************************************************************************/
DWORD
DSK_Open(DWORD dwData,
         DWORD dwAccess,
         DWORD dwShareMode)
{
    PDISK pDisk = (PDISK)dwData;
    DWORD dwRet = 0;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++DSK_Open(0x%x)\r\n"),dwData));

    do {
        if (IsValidDisk(pDisk) == FALSE)
        {
            BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  DSK_Open - Passed invalid disk handle\r\n")));
            break;
        }
    
        EnterCriticalSection(&(pDisk->d_DiskCardCrit));
        {
            pDisk->d_OpenCount++;
        }
        LeaveCriticalSection(&(pDisk->d_DiskCardCrit));
    
        dwRet = (DWORD)pDisk;

    } while(0);

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --DSK_Open(0x%x) returning %d\r\n"),dwData, dwRet));

    return dwRet;
}


/*****************************************************************************/
/*                                                                           */
/* NAME                                                                      */
/*      DSK_IOControl                                                        */
/* DESCRIPTION                                                               */
/*      This function is Disk IO Control                                     */
/* PARAMETERS                                                                */
/*      Handle                                                               */
/*          Disk handle                                                      */
/*      dwIoControlCode                                                      */
/*          IO Control Code                                                  */
/*      pInBuf                                                               */
/*          Pointer to input buffer                                          */
/*      nInBufSize                                                           */
/*          Size of input buffer                                             */
/*      pOutBuf                                                              */
/*          Pointer to output buffer                                         */
/*      nOutBufSize                                                          */
/*          Size of output buffer                                            */
/*      pBytesReturned                                                       */
/*          Pointer to byte returned                                         */
/* RETURN VALUES                                                             */
/*      Return TRUE if Operation success, FALSE if not.                      */
/* NOTE                                                                      */
/*      I/O Control function - responds to info, read and write control codes*/
/*      The read and write take a scatter/gather list in pInBuf              */
/*                                                                           */
/*****************************************************************************/
BOOL
DSK_IOControl(DWORD  Handle,
              DWORD  dwIoControlCode,
              PBYTE  pInBuf,
              DWORD  nInBufSize,
              PBYTE  pOutBuf,
              DWORD  nOutBufSize,
              PDWORD pBytesReturned)
{
    PSG_REQ pSG;
    PDISK   pDisk = (PDISK)Handle;
    BOOL    bRes  = TRUE;
    BOOL    bLastMode;
    BOOL    bPrintFlg = TRUE;

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV: IN] ++DSK_IOControl() (IO code=%x)\r\n"), dwIoControlCode));

    /* Disk Handle(pDisk) validation check */
    if (IsValidDisk(pDisk) == FALSE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:ERR]  Invalid Disk\r\n")));
        bRes = FALSE;

        goto IOControlError;
    }


    /* dwIoControlCode Print */
    switch (dwIoControlCode)
    {
    case DISK_IOCTL_READ:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = DISK_IOCTL_READ\r\n")));
        break;
    case IOCTL_DISK_READ:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = IOCTL_DISK_READ\r\n")));
        break;
    case DISK_IOCTL_GETINFO:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = DISK_IOCTL_GETINFO\r\n")));
        break;
    case DISK_IOCTL_SETINFO:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = DISK_IOCTL_SETINFO\r\n")));
        break;
    case DISK_IOCTL_INITIALIZED:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = DISK_IOCTL_INITIALIZED\r\n")));
        break;
    case DISK_IOCTL_GETNAME:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = DISK_IOCTL_GETNAME\r\n")));
        break;
    case IOCTL_DISK_GETINFO:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = IOCTL_DISK_GETINFO\r\n")));
        break;

#if (CE_MAJOR_VER > 0x0003)
    case IOCTL_DISK_DEVICE_INFO:
        BIBDRV_LOG_PRINT((TEXT("[BIBDRV:LOG]  dwIoControlCode = IOCTL_DISK_DEVICE_INFO\r\n")));
        break;
#endif  //(CE_MAJOR_VER > 0x0003)        
    }

    /*------------------*/
    /* Check parameters */
    /*------------------*/
    switch (dwIoControlCode)
    {
    case DISK_IOCTL_READ:
    case IOCTL_DISK_READ:
    case DISK_IOCTL_GETINFO:
    case DISK_IOCTL_SETINFO:
    case DISK_IOCTL_INITIALIZED:
        if (pInBuf == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            bRes = FALSE;
            goto IOControlError;
        }
        break;
    case DISK_IOCTL_GETNAME:
    case IOCTL_DISK_GETINFO:
        if (pOutBuf == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            bRes = FALSE;
            goto IOControlError;
        }
        break;

#if (CE_MAJOR_VER > 0x0003)
    case IOCTL_DISK_DEVICE_INFO:
        if(!pInBuf || nInBufSize != sizeof(STORAGEDEVICEINFO)) 
        {
            SetLastError(ERROR_INVALID_PARAMETER);   
            bRes = FALSE;
            goto IOControlError;
        }
        break;
#endif  //(CE_MAJOR_VER > 0x0003)        

    default:
        BIBDRV_ERR_PRINT((TEXT("[BIBDRV:MSG]  DSK_IOControl unkonwn code(%04x)\r\n"), dwIoControlCode));
        SetLastError(ERROR_INVALID_PARAMETER);
        bRes = FALSE;
        goto IOControlError;
    }

    EnterCriticalSection(&gDiskCrit);

    //
    // Execute dwIoControlCode
    //
    switch (dwIoControlCode)
    {
    case DISK_IOCTL_READ:
    case IOCTL_DISK_READ:
    
        bLastMode = SetKMode(TRUE); 

        pSG = (PSG_REQ)pInBuf;
        
#if defined(_BIBDRV_CACHING_SECTORS_)
        DoDiskReadWithCaching(pDisk, (PVOID) pSG);
#else   //_BIBDRV_CACHING_SECTORS_
        DoDiskRead(pDisk, (PVOID) pSG);
#endif  //_BIBDRV_CACHING_SECTORS_

        if (ERROR_SUCCESS != pSG->sr_status) 
        {
            SetLastError(pSG->sr_status);
            bRes = FALSE;
        }
        else
        {
            bRes = TRUE;
            if (pBytesReturned)
                *(pBytesReturned) = pDisk->d_DiskInfo.di_bytes_per_sect;
        }

        SetKMode (bLastMode);

        break;

    case DISK_IOCTL_GETINFO:
        GetDiskInfo(pDisk, (PDISK_INFO) pInBuf);
        if (pBytesReturned)
            *(pBytesReturned) = sizeof(DISK_INFO);
        bRes = TRUE;
        break;
        
    case IOCTL_DISK_GETINFO:
        GetDiskInfo(pDisk, (PDISK_INFO) pOutBuf);
        if (pBytesReturned)
            *(pBytesReturned) = sizeof(DISK_INFO);
        bRes = TRUE;
        break;
        
    case DISK_IOCTL_SETINFO:
        RETAILMSG(1, (TEXT("-DISK_IOCTL_SETINFO\r\n")));
        SetDiskInfo(pDisk, (PDISK_INFO)pInBuf);
        if (pBytesReturned)
            *(pBytesReturned) = sizeof(DISK_INFO);
        bRes = TRUE;
        break;

    case DISK_IOCTL_GETNAME:
        RETAILMSG(1, (TEXT("-DISK_IOCTL_GETNAME\r\n")));
        bRes = GetFolderName(pDisk, (LPWSTR)pOutBuf, nOutBufSize, pBytesReturned);
        break;
        
#if (CE_MAJOR_VER > 0x0003)
    case IOCTL_DISK_DEVICE_INFO: // new ioctl for disk info
        bRes = GetDeviceInfo(pDisk, (PSTORAGEDEVICEINFO)pInBuf);
        if (pBytesReturned)
            *(pBytesReturned) = sizeof(STORAGEDEVICEINFO);
        break;
#endif  //(CE_MAJOR_VER > 0x0003)

    case DISK_IOCTL_INITIALIZED:
        bRes = TRUE;
        break;
        
    default:
        RETAILMSG(1, (TEXT("-DSK_IOControl (default:0x%x) \r\n"), dwIoControlCode));
        SetLastError(ERROR_INVALID_PARAMETER);
        bRes = FALSE;
        break;
    }

    LeaveCriticalSection(&gDiskCrit);

IOControlError:

    BIBDRV_LOG_PRINT((TEXT("[BIBDRV:OUT] --DSK_IOControl()\r\n")));

    return bRes;
}


DWORD
DSK_Read(DWORD Handle, LPVOID pBuffer, DWORD dwNumBytes)
{
    BIBDRV_LOG_PRINT((TEXT("DSK_Read\r\n")));
    return 0;
}

DWORD
DSK_Write(DWORD Handle, LPCVOID pBuffer, DWORD dwNumBytes)
{
    BIBDRV_LOG_PRINT((TEXT("DSK_Write\r\n")));
    return 0;
}

DWORD
DSK_Seek(DWORD Handle, long lDistance, DWORD dwMoveMethod)
{
    BIBDRV_LOG_PRINT((TEXT("DSK_Seek\r\n")));
    return 0;
}

void
DSK_PowerUp(void)
{
}

void
DSK_PowerDown(void)
{
}
