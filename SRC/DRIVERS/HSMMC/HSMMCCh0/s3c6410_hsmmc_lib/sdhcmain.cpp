//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

// Copyright (c) 2002 BSQUARE Corporation.  All rights reserved.
// DO NOT REMOVE --- BEGIN EXTERNALLY DEVELOPED SOURCE CODE ID 40973--- DO NOT REMOVE

// Driver entry points

#include "SDCardDDK.h"
#include "SDHC.h"

// initialize debug zones
SD_DEBUG_INSTANTIATE_ZONES(
                           TEXT("HSMMC"), // module name
                           ZONE_ENABLE_INIT | ZONE_ENABLE_ERROR | ZONE_ENABLE_WARN,
                           TEXT("Interrupts"),
                           TEXT("Send Handler "), 
                           TEXT("Responses"), 
                           TEXT("Receive Data"),                   
                           TEXT("Clock Control"), 
                           TEXT("Transmit Data"), 
                           TEXT("Function State Poll"), 
                           TEXT(""),
                           TEXT(""),
                           TEXT(""),
                           TEXT(""));


///////////////////////////////////////////////////////////////////////////////
//  DllEntry - the main dll entry point
//  Input:  hInstance - the instance that is attaching
//          Reason - the reason for attaching
//          pReserved - not much
//  Output: 
//  Return: Always TRUE
//  Notes:  this is only used to initialize the zones
///////////////////////////////////////////////////////////////////////////////
STDAPI_(BOOL) DllEntry(HINSTANCE  hInstance,
              ULONG      Reason,
              LPVOID     pReserved)
{
    BOOL fRet = TRUE;

    if (Reason == DLL_PROCESS_ATTACH) {
        DEBUGREGISTER(hInstance);
        DisableThreadLibraryCalls((HMODULE) hInstance);

        if (!SDInitializeCardLib()) {
            fRet = FALSE;
        }
        else if (!SD_API_SUCCESS(SDHCDInitializeHCLib())) {
            SDDeinitializeCardLib();
            fRet = FALSE;
        }
    }
    else if (Reason == DLL_PROCESS_DETACH) {
        SDHCDDeinitializeHCLib();
        SDDeinitializeCardLib();
    }

    return fRet;
}


///////////////////////////////////////////////////////////////////////////////
//  PreDeinit - the predeinit entry point for the driver
//  Input:  hDeviceContext - the context returned from HSC_Init
//  Output: 
//  Return: TRUE
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL HSC_PreDeinit(DWORD hDeviceContext)
{    
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC +PreDeinit\n")));

    PCSDHCBase pController = (PCSDHCBase) hDeviceContext;
    pController->PreDeinit();

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC -PreDeinit\n")));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//  Deinit - the deinit entry point for the driver
//  Input:  hDeviceContext - the context returned from HSC_Init
//  Output: 
//  Return: TRUE
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL HSC_Deinit(DWORD hDeviceContext)
{    
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC +Deinit\n")));

    PCSDHCBase pController = (PCSDHCBase) hDeviceContext;
    LPSDHC_DESTRUCTION_PROC pfnDestruction = pController->GetDestructionProc();
    if (pfnDestruction) {
        (*pfnDestruction)(pController);
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC -Deinit\n")));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//  Init - the init entry point for the CE driver instance
//  Input:  dwContext - the context passed from device manager
//  Output: 
//  Return: return DWORD identifer for the instance
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD HSC_Init(DWORD dwContext)
{
    LPCTSTR pszActiveKey = (LPCTSTR) dwContext;
    DWORD   dwRet = 0;

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC +Init\n")));    
    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC Active RegPath: %s \n"), pszActiveKey));

    PCSDHCBase pController = CSDHCBase::CreateSDHCControllerObject(pszActiveKey);
    if (!pController) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDHC Failed to create controller object\n")));
        goto EXIT;
    }

    if (!pController->Init(pszActiveKey)) {
        goto EXIT;
    }

    // Return the controller instance
    dwRet = (DWORD) pController;

EXIT:
    if ( (dwRet == 0) && pController ) {
        HSC_Deinit((DWORD) pController);
    }

    DEBUGMSG(SDCARD_ZONE_INIT, (TEXT("SDHC -Init\n")));

    return dwRet;
}


///////////////////////////////////////////////////////////////////////////////
//  Open - the open entry point for the driver
//  Input:  hDeviceContext - the context returned from HSC_Init
//  Output: 
//  Return: pController context
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD HSC_Open(
               DWORD hDeviceContext,
               DWORD,
               DWORD
               )
{
    PCSDHCBase pController = (PCSDHCBase) hDeviceContext;
    return (DWORD) pController;
}


///////////////////////////////////////////////////////////////////////////////
//  Close - the close entry point for the driver
//  Input:  hOpenContext - the context returned from HSC_Open
//  Output: 
//  Return: TRUE
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL HSC_Close(
               DWORD hOpenContext
               )
{
    PCSDHCBase pController = (PCSDHCBase) hOpenContext;
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//  PowerDown - the power down entry point for the driver
//  Input:  hDeviceContext - the device context from HSC_Init
//  Output: 
//  Return: 
//  Notes:  
//      Indicates powerdown 
///////////////////////////////////////////////////////////////////////////////
extern "C"
void HSC_PowerDown(DWORD hDeviceContext)
{
    PCSDHCBase pController = (PCSDHCBase) hDeviceContext;
    pController->PowerDown();
}


///////////////////////////////////////////////////////////////////////////////
//  PowerUp - the power up entry point for the driver
//  Input:  hDeviceContext - the device context from HSC_Init
//  Output: 
//  Return:
//  Notes: 
//          On power up, the indication is made to the bus driver and the
//          IST is triggered in order to remove the current instance 
///////////////////////////////////////////////////////////////////////////////
extern "C"
void HSC_PowerUp(DWORD hDeviceContext)
{
    PCSDHCBase pController = (PCSDHCBase) hDeviceContext;
    pController->PowerUp();
}


///////////////////////////////////////////////////////////////////////////////
//  IOControl - the iocontrol entry point for the driver
//  Input:  hOpenContext - the context returned from HSC_Open
//  Output: 
//  Return: TRUE, if success
//  Notes:  handles power management IOCTLs
///////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL HSC_IOControl(
                   DWORD hOpenContext,
                   DWORD dwCode,
                   PBYTE pBufIn,
                   DWORD dwLenIn,
                   PBYTE pBufOut,
                   DWORD dwLenOut,
                   PDWORD pdwActualOut 
                   )
{
    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDHC +IOControl\n")));

    PCSDHCBase pController = (PCSDHCBase) hOpenContext;
    DWORD dwErr = pController->IOControl(dwCode, pBufIn, dwLenIn, pBufOut,
        dwLenOut, pdwActualOut);

    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
    }

    DEBUGMSG(SDCARD_ZONE_FUNC, (TEXT("SDHC -IOControl\n")));

    return (dwErr == ERROR_SUCCESS);
}


// DO NOT REMOVE --- END EXTERNALLY DEVELOPED SOURCE CODE ID --- DO NOT REMOVE

