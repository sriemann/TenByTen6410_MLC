//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include "UAODriver.h"
#include "UAOMisc.h"
#include "UAOSet.h"
#include <bsp_cfg.h>

#if DEBUG
#define ZONE_INIT              DEBUGZONE(0)

DBGPARAM dpCurSettings =                \
{                                       \
    TEXT("UAO_Driver"),                 \
    {                                   \
        TEXT("Init"),       /* 0  */    \
    },                                  \
    (0x0001)                            \
};
#endif

/*----------------------------------------------------------------------------
*Function: UAO_Init

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize UAO Hardware 
-----------------------------------------------------------------------------*/
DWORD
UAO_Init(
    DWORD dwContext
    )
{
    HANDLE                h_Mutex;

    printD("DD::UAO_Init\n");

    // Mutex initialization
    h_Mutex = CreateUAOMutex();
    if (h_Mutex == NULL) 
    {
        RETAILMSG(1, (TEXT("DD::UAO Mutex Initialize error : %d \r\n"),GetLastError()));    
        return FALSE;
    }

    return 0x12345678;
}


/*----------------------------------------------------------------------------
*Function: UAO_DeInit

*Parameters:         InitHandle        :
*Return Value:        True/False
*Implementation Notes: Deinitialize UAO Hardware 
-----------------------------------------------------------------------------*/
BOOL
UAO_Deinit(
    DWORD InitHandle
    )
{
    printD("UAO_Deinit\n");
    DeleteUAOMutex();
    return TRUE;
}


/*----------------------------------------------------------------------------
*Function: UAO_Open

*Parameters:         InitHandle        :Handle to UAO  context
                    dwAccess        :
                    dwShareMode        :File share mode of UAO
*Return Value:        This function returns a handle that identifies the 
                    open context of UAO  to the calling application.
*Implementation Notes: Opens UAO CODEC device for reading, writing, or both 
-----------------------------------------------------------------------------*/
DWORD
UAO_Open(
    DWORD InitHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    printD("UAO_Open(0x%x)\n", InitHandle);
    return InitHandle;
}


/*----------------------------------------------------------------------------
*Function: UAO_Close

*Parameters:         OpenHandle        :
*Return Value:        True/False
*Implementation Notes: This function closes the device context identified by
                        OpenHandle 
-----------------------------------------------------------------------------*/
BOOL
UAO_Close(
    DWORD OpenHandle
    )
{
    return TRUE;
}


/*----------------------------------------------------------------------------
*Function: UAO_IOControl

*Parameters:         OpenHandle        :
                    dwIoControlCode    :
*Return Value:        True/False
*Implementation Notes: UAO_IOControl sends commands to initiate different
*                       operations like Init,Decode and Deinit.The test 
*                       application uses the DeviceIOControl function to 
*                       specify an operation to perform 
-----------------------------------------------------------------------------*/
BOOL
UAO_IOControl(
    DWORD OpenHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned
    )
{
    BOOL    result = TRUE;
    DWORD    ret;

    printD("DD::IOCTL\n");

    ret = LockUAOMutex();
    if(!ret){
        RETAILMSG(1, (TEXT("DD::UAO Mutex Lock Fail\r\n")));
        return FALSE;
    }

    switch ( dwIoControlCode ) {
    
    case IOCTL_UAO_ENABLE:
            printD("DD::IOCTL_UAO_ENABLE\n");
            UAOEnable();
            break;

    case IOCTL_UAO_DISABLE:
            printD("DD::IOCTL_UAO_DISABLE\n");
            UAODisable();
            break;

    default:
            RETAILMSG(1, (TEXT("DD::UAO Invalid IOControl(0x%x)\r\n"), dwIoControlCode));
            result = FALSE;
    }

    UnlockUAOMutex();
    return result;
}

/*----------------------------------------------------------------------------
*Function: UAO_Write

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize UAO Hardware 
-----------------------------------------------------------------------------*/
DWORD UAO_Write(
    DWORD OpenHandle, 
    LPCVOID pBuffer, 
    DWORD dwNumBytes
    )
{
    printD("DD::UAO_Write \n");
    return TRUE;
}

/*----------------------------------------------------------------------------
*Function: UAO_PowerUp

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize UAO Hardware 
-----------------------------------------------------------------------------*/
void UAO_PowerUp(
    DWORD InitHandle
    )
{
    printD("DD::UAO_PowerUp \n");
}

/*----------------------------------------------------------------------------
*Function: UAO_PowerDown

*Parameters:         dwContext        :
*Return Value:        True/False
*Implementation Notes: Initialize UAO Hardware 
-----------------------------------------------------------------------------*/
void UAO_PowerDown(
    DWORD InitHandle
    )
{
    printD("DD::UAO_PowerDown \n");
}


/*----------------------------------------------------------------------------
*Function: UAO_DllMain

*Parameters:         DllInstance        :
                    Reason            : 
                    Reserved        :
*Return Value:        True/False
*Implementation Notes: Entry point for UAO.dll 
-----------------------------------------------------------------------------*/
BOOL WINAPI
UAO_DllMain(HINSTANCE DllInstance, DWORD Reason, LPVOID Reserved)
{
    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            DEBUGREGISTER(DllInstance);
            break;
    }
    return TRUE;
}   
