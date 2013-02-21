///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Marvell Corporation.  All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT 
// WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
//
// Module Name:
//
//    NDISLoader.cpp
//
// Abstract:
//
//    Streams portion of the NDIS sample
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////


#include "SDCardDDK.h"
#include "sdndis.h"

#define MAX_MINIPORT_NAME_PATH  256
#define MAX_NUMBER_OF_ADAPTERS  8
#define LOADER_INSTANCE_KEY TEXT("Instance")
#include "precomp.h"

// this was in driver entry file of the NDISSample
// initialize debug zones
SD_DEBUG_INSTANTIATE_ZONES(
     TEXT("Marvell SDIO"), // module name
     ZONE_ENABLE_INIT | ZONE_ENABLE_ERROR | ZONE_ENABLE_WARN | ENABLE_ZONE_SEND,    // initial settings
     TEXT(""),
     TEXT(""), 
     TEXT(""), 
     TEXT(""),                  
     TEXT(""), 
     TEXT(""), 
     TEXT(""), 
     TEXT(""),
     TEXT(""),
     TEXT(""),
     TEXT(""));

    // miniport instance information
typedef struct _MINIPORT_INSTANCE_INFO {
    WCHAR  MiniportName[MAX_MINIPORT_NAME_PATH];
    WCHAR  MiniportInstance[MAX_MINIPORT_NAME_PATH];
    WCHAR  RegPath[MAX_MINIPORT_NAME_PATH];
    WCHAR  ActiveKeyPath[MAX_MINIPORT_NAME_PATH];
    ULONG  InstanceNumber;
}MINIPORT_INSTANCE_INFO, *PMINIPORT_INSTANCE_INFO;

extern BOOL LoaderEntry(HINSTANCE  hInstance,
                     ULONG      Reason,
                     LPVOID     pReserved);
extern "C" {
extern HANDLE g_hLoaderThread;
extern CRITICAL_SECTION    LoaderCriticalSection;
}

///////////////////////////////////////////////////////////////////////////////
//  LoadMiniport - load the miniport for this instance
//  Input:  pInstance - information for this instance
//  Output: 
//  Return: TRUE if successful
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
BOOL LoadMiniport(PMINIPORT_INSTANCE_INFO pInstance)
{
    HKEY     hKey;              // registry key
    DWORD    win32Status;       // status  
    DWORD    dataSize;          // data size for query
    WCHAR    stringBuff[128];   // string buffer
    WCHAR    instanceKey[32];   // instance name
    WCHAR    instanceNumber[10];    // instance number
    WCHAR    *token;                // tokenizer
    NDIS_STATUS NdisStatus;         // ndis status
    
    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("+LoadMiniport\r\n")));

 //  MessageBox(NULL, TEXT("SDIO init"), TEXT("LoadMiniport"), MB_OK);

        // open the registry path for this instance
    if ((win32Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    pInstance->RegPath,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKey)) != ERROR_SUCCESS) {
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR:Failed to open path %s; %d \n"), 
                   pInstance->RegPath, win32Status));

        
        return FALSE;
    }

//    MessageBox(NULL, TEXT("SDIO init"), TEXT("1"), MB_OK);
    dataSize = sizeof(stringBuff);

        // build up the instance key
    wcscpy(instanceKey,LOADER_INSTANCE_KEY);
    _ultow(pInstance->InstanceNumber, instanceNumber, 10);
    wcscat(instanceKey, instanceNumber);

        // retrieve the real reg path to the device parameters
    if (RegQueryValueEx(hKey, 
                        instanceKey, 
                        0, 
                        NULL, 
                        (PUCHAR)stringBuff, 
                        &dataSize) != ERROR_SUCCESS) {
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR: Failed to get the instance key : %d \n"), instanceKey));
        RegCloseKey(hKey);
        return FALSE;
    }

//    MessageBox(NULL, TEXT("SDIO init"), TEXT("2"), MB_OK);
        // don't need this anymore
    RegCloseKey(hKey);

    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: Tokenizing instance information: %s \n"), stringBuff));
      
        // extract the miniport name and instance name, in the form of "<Miniport Name>:<Miniport Instance>
    token = wcstok(stringBuff, TEXT(":"));

    if (token != NULL) {

        wcscpy(pInstance->MiniportName, token);
        
            // search for the next one
        token = wcstok( NULL, TEXT(":"));

        if (token != NULL) {
            wcscpy(pInstance->MiniportInstance, token);
        } else {
             DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR: Failed to get miniport instance \n")));
             return FALSE;
        }

    } else {
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR: Failed to get miniport name \n")));
        return FALSE;
    }
   
//    MessageBox(NULL, TEXT("SDIO init"), TEXT("3"), MB_OK);
        // build up the miniport instance path in order to stick in the "ActivePath" key
    wcscpy(stringBuff,TEXT("\\Comm\\"));
    wcscat(stringBuff,pInstance->MiniportInstance);
    wcscat(stringBuff,TEXT("\\Parms"));

    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: Miniport instance path %s \n"), stringBuff));
   
    if ((win32Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    stringBuff,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKey)) != ERROR_SUCCESS) {
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR:Failed to open path %s; %d \n"), 
                   stringBuff, win32Status));
        return FALSE;
    }

//    MessageBox(NULL, TEXT("SDIO init"), TEXT("4"), MB_OK);
        // make sure the key is deleted first
    RegDeleteKey(hKey, TEXT("ActivePath")); 
//    RegDeleteValue(hKey, TEXT("ActivePath")); 
//    MessageBox(NULL, TEXT("SDIO init"), TEXT("4"), MB_OK);

    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: Storing ActiveKey Path %s \n"), pInstance->ActiveKeyPath));

        // save the active ActivePath in the registry path for the miniport.  The miniport
        // portion will look up this key
    if (RegSetValueEx(hKey, 
                      TEXT("ActivePath"), 
                      0, 
                      REG_SZ, 
                      (PUCHAR)pInstance->ActiveKeyPath, 
                      ((sizeof(WCHAR))*(wcslen(pInstance->ActiveKeyPath) + 1))) != ERROR_SUCCESS) { 
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR: Failed to set ActiveKey path \n")));
        RegCloseKey(hKey);
        return FALSE;
    }

//    MessageBox(NULL, TEXT("SDIO init"), TEXT("5"), MB_OK);
        // close the key
    RegCloseKey(hKey);

//    MessageBox(NULL, pInstance->MiniportName, pInstance->MiniportInstance, MB_OK);

        // register the adapter
    NdisRegisterAdapter(&NdisStatus,
                        pInstance->MiniportName,
                        pInstance->MiniportInstance);

    if (!NDIS_SUCCESS(NdisStatus)) {
        return FALSE;
    }

	DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("-LoadMiniport\r\n")));

//    MessageBox(NULL, TEXT("SDIO init"), TEXT("6"), MB_OK);
    return TRUE;
   
}

///////////////////////////////////////////////////////////////////////////////
//  UnloadMiniport - unload the miniport
//  Input:   pInstance - the instance to unload
//  Output: 
//  Return: 
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
VOID UnloadMiniport(PMINIPORT_INSTANCE_INFO pInstance)
{
    NDIS_STATUS NdisStatus;
	HKEY     hKey;              // registry key
    DWORD    win32Status;       // status  
    WCHAR    stringBuff[128];   // string buffer

    DbgPrintZo(SDCARD_ZONE_INIT, 
        (TEXT("SDNDISLDR: Unloading Miniport Instance %s \n"), 
        pInstance->MiniportInstance));
   
    NdisDeregisterAdapter(&NdisStatus, pInstance->MiniportInstance);

    DbgPrintZo(SDCARD_ZONE_INIT, 
        (TEXT("SDNDISLDR: Miniport Unloaded 0x%08X \n"), 
        NdisStatus));

	    wcscpy(stringBuff,TEXT("\\Comm\\"));
    wcscat(stringBuff,pInstance->MiniportInstance);
    wcscat(stringBuff,TEXT("\\Parms"));

    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: Miniport instance path %s \n"), stringBuff));
   
    if ((win32Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    stringBuff,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKey)) != ERROR_SUCCESS) {
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR:Failed to open path %s; %d \n"), 
                   stringBuff, win32Status));
        return;
    }

//    MessageBox(NULL, TEXT("SDIO init"), TEXT("4"), MB_OK);
        // make sure the key is deleted first
    RegDeleteValue(hKey, TEXT("ActivePath")); 

    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: Storing ActiveKey Path %s \n"), pInstance->ActiveKeyPath));
	
	RegCloseKey(hKey);
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_Deinit - the deinit entry point for this driver
//  Input:  hDeviceContext - the context returned from NDL_Init
//  Output: 
//  Returns: always returns TRUE.  
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
BOOL NDL_Deinit(DWORD hDeviceContext)
{
    PMINIPORT_INSTANCE_INFO pInstance;     // memory card instance

    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: +NDL_Deinit\n")));
    //DBGPRINT(DBG_UNLOAD, ("SDNDISLDR: +NDL_Deinit\n"));

    pInstance = (PMINIPORT_INSTANCE_INFO)hDeviceContext;

    DEBUGCHK(g_hLoaderThread);

    // wait for loader thread to exit before trying to unload anything
    WaitForSingleObject(g_hLoaderThread,INFINITE);

    EnterCriticalSection(&LoaderCriticalSection);

        // unload the miniport
    UnloadMiniport(pInstance);

    CloseHandle(g_hLoaderThread);
    g_hLoaderThread = NULL;  
    LocalFree(pInstance);

    LeaveCriticalSection(&LoaderCriticalSection);


    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: -NDL_Deinit\n")));

    //DBGPRINT(DBG_UNLOAD, ("SDNDISLDR: -NDL_Deinit\n"));
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_Init - the init entry point
//  Input:  dwContext - the context for this init
//  Output: 
//  Return: returns a non-zero context
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
DWORD NDL_Init(DWORD dwContext)
{
    PMINIPORT_INSTANCE_INFO         pInstance;      // this instance of the device
    DWORD                           dwThreadID;
    BOOL                            bWaitForUnload = TRUE;

    //
    // Initialize the output log file here
    //
    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: +NDL_Init\n")));
    pInstance = (PMINIPORT_INSTANCE_INFO)
    LocalAlloc(LPTR, sizeof(MINIPORT_INSTANCE_INFO));

    if (pInstance == NULL) {
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR: Failed to allocate device info \n")));
        DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: -NDL_Init\n")));
        return 0;
    }
    // on CE, the dwContext is a pointer to a string to the "Active" registry path
    // we pass this to the NDIS driver
    wcscpy(pInstance->ActiveKeyPath, (PWCHAR)dwContext);

    if (SDGetRegPathFromInitContext((PWCHAR)dwContext, 
                                    pInstance->RegPath, 
                                    sizeof(pInstance->RegPath)) != ERROR_SUCCESS) {
        DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR: Failed to get reg path \n")));
        LocalFree(pInstance);
        return 0;
    }

    // only allow one instance to be loaded
    while(bWaitForUnload)
    {
        EnterCriticalSection(&LoaderCriticalSection);
        if(g_hLoaderThread == NULL)
        {
            // per BSquare's suggestion, use a seperate thread to prevent the interrupt replies from
            // blocking
            g_hLoaderThread = CreateThread(NULL, NULL, (unsigned long (__cdecl *)(void *))LoadMiniport, pInstance, 0, &dwThreadID);      
            pInstance->InstanceNumber = 0;
            bWaitForUnload = FALSE;
            LeaveCriticalSection(&LoaderCriticalSection);
            break;
        }
        else
        {
            DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("SDNDISLDR: Previous instance not unloaded yet \n")));
        }
        
        LeaveCriticalSection(&LoaderCriticalSection);

        Sleep(10);
    }

    DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("SDNDISLDR: -NDL_Init\n")));

    return (DWORD)pInstance;
}


///////////////////////////////////////////////////////////////////////////////
//  NDL_IOControl - the I/O control entry point 
//  Input:  Handle - the context returned from NDL_Open
//          IoctlCode - the ioctl code
//          pInBuf - the input buffer from the user
//          InBufSize - the length of the input buffer
//          pOutBuf - the output buffer from the user
//          InBufSize - the length of the output buffer
//          pBytesReturned - the size of the transfer
//  Output: 
//  Return:  TRUE if Ioctl was handled
//  Notes:   Not used
///////////////////////////////////////////////////////////////////////////////
BOOL NDL_IOControl(DWORD   Handle,
                   DWORD   IoctlCode,
                   PBYTE   pInBuf,
                   DWORD   InBufSize,
                   PBYTE   pOutBuf,
                   DWORD   OutBufSize,
                   PDWORD  pBytesReturned)
{
        // not used
    return FALSE;
    
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_Open - the open entry point 
//  Input:  hDeviceContext - the device context from NDL_Init
//          AccessCode - the desired access
//          ShareMode - the desired share mode
//  Output: 
//  Return: returns an open context 
//  Notes:  Not used
///////////////////////////////////////////////////////////////////////////////
DWORD NDL_Open(DWORD    hDeviceContext,
               DWORD    AccessCode,
               DWORD    ShareMode)
{
        // just return the instance
    return hDeviceContext;
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_PowerDown - the power down entry point for the bus driver
//  Input:  hDeviceContext - the device context from NDL_Init
//  Output:
//  Return: 
//  Notes:  performs no actions
///////////////////////////////////////////////////////////////////////////////
void NDL_PowerDown(DWORD    hDeviceContext)
{
    return;
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_PowerUp - the power up entry point for the CE file system wrapper
//  Input:  hDeviceContext - the device context from NDL_Init
//  Output: 
//  Return:
//  Notes:  performs no actions
///////////////////////////////////////////////////////////////////////////////
void NDL_PowerUp(DWORD  hDeviceContext)
{
    return;
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_Read - the read entry point 
//  Input:  hOpenContext - the context from NDL_Open
//          pBuffer - the user's buffer
//          Count - the size of the transfer
//  Output: 
//  Return: zero (not implemented)
//  Notes:  Not used
///////////////////////////////////////////////////////////////////////////////
DWORD NDL_Read(DWORD    hOpenContext,
               LPVOID   pBuffer,
               DWORD    Count)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_Seek - the seek entry point 
//  Input:  hOpenContext - the context from NDL_Open
//          Amount - the amount to seek
//          Type - the type of seek
//  Output: 
//  Return: zero (not implemented)
//  Notes:  Not used
///////////////////////////////////////////////////////////////////////////////
DWORD NDL_Seek(DWORD    hOpenContext,
               long     Amount,
               DWORD    Type)
{    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_Write - the write entry point 
//  Input:  hOpenContext - the context from NDL_Open
//          pBuffer - the user's buffer
//          Count - the size of the transfer
//  Output: 
//  Return: zero (not implemented)
//  Notes:  Not used
///////////////////////////////////////////////////////////////////////////////
DWORD NDL_Write(DWORD   hOpenContext,
                LPCVOID pBuffer,
                DWORD   Count)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//  NDL_Close - the close entry point 
//  Input:  hOpenContext - the context returned from NDL_Open
//  Output: 
//  Return: always returns TRUE
//  Notes:  Not used
///////////////////////////////////////////////////////////////////////////////
BOOL NDL_Close(DWORD hOpenContext)
{
    return TRUE;
}

