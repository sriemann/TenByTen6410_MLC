//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/
/**************************************************************************************
* 
*    Project Name : IIC Driver 
*    Project Description :
*        This software is MDD layer for IIC Samsung driver. 
*  
*--------------------------------------------------------------------------------------
* 
*    File Name : iic_mdd.cpp
*  
*    File Description : This file implements MDD layer functions which is stream driver.
*
**************************************************************************************/

#include <windows.h>
#include <types.h>
#include <linklist.h>
#include <nkintr.h>
#include <devload.h>
#include <pm.h>
#include <pmplatform.h>

#include <iic_mdd.h>
#include <iic_pdd.h>


#define DEFAULT_CE_THREAD_PRIORITY 103


CEDEVICE_POWER_STATE    g_Dx;

// Define some internally used functions
BOOL IIC_Close(PHW_OPEN_INFO    pOpenContext);
BOOL IIC_Deinit(PHW_INIT_INFO   pInitContext);
void IIC_PowerUp(PHW_INIT_INFO    pInitContext);
void IIC_PowerDown(PHW_INIT_INFO    pInitContext);

#if DEBUG
#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INIT           DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)

DBGPARAM dpCurSettings =
{
    TEXT("IIC"),
    {
         TEXT("Errors"),TEXT("Warnings"),TEXT("Function"),TEXT("Init"),
         TEXT("Info"),TEXT("Ist"),TEXT("Undefined"),TEXT("Undefined"),
         TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),
         TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined")
    },
        (1 << 0)   // Errors
    |   (1 << 1)   // Warnings
};
#endif

static const POWER_CAPABILITIES  g_PowerCaps =
{
    // DeviceDx:    Supported power states
    DX_MASK(D0 ) | DX_MASK(D4),

    0,              // WakeFromDx:
    0,              // InrushDx:    No inrush of power

    {               // Power: Maximum milliwatts in each state
        0x00000001, //        D0 = 0
        0x00000001, //        D1 = 0
        0x00000001, //        D2 = 0
        0x00000001, //        D3 = 0
        0x00000001  //        D4 = 0 (off)
    },

    {               // Latency
        0x00000000, //        D0 = 0
        0x00000000, //        D1 = 0
        0x00000000, //        D2 = 0
        0x00000000, //        D3 = 0
        0x00000000  //        D4 = 0
    },

    POWER_CAP_PREFIX_MICRO | POWER_CAP_UNIT_AMPS,       // Flags
};

//////////
// Function Name : DllEntry
// Function Description : Process attach/detach api.
// Input : HINSTANCE   hinstDll, DWORD   dwReason, LPVOID  lpReserved
// Output : The return is a BOOL, representing success (TRUE) or failure (FALSE).
// Version : v1.0
BOOL
DllEntry(
              HINSTANCE   hinstDll,             /*Instance pointer. */
              DWORD   dwReason,                 /*Reason routine is called. */
              LPVOID  lpReserved                /*system parameter. */
              )
{
    if ( dwReason == DLL_PROCESS_ATTACH ) {
        DEBUGREGISTER(hinstDll);
        DEBUGMSG (ZONE_INIT, (TEXT("process attach\r\n")));
        DisableThreadLibraryCalls((HMODULE) hinstDll);
    }

    if ( dwReason == DLL_PROCESS_DETACH ) {
        DEBUGMSG (ZONE_INIT, (TEXT("process detach called\r\n")));
    }

    return(TRUE);
}

//////////
// Function Name : IIC_Init
// Function Description : IIC device initialization.
// Input : LPCTSTR pContext
// Output : Returns a pointer to the head which is passed into
//                              the IIC_OPEN and IIC_DEINIT entry points as a device handle.
// Version : v0.5
HANDLE
IIC_Init(
         LPCTSTR pContext                        /* Pointer to a string containing the registry path.*/
        )
{
    PHW_INIT_INFO    pInitContext = NULL;
    HKEY            hKey;
    ULONG           datasize = sizeof(ULONG);    
    ULONG           kvaluetype;    

    DEBUGCHK(pContext != NULL); 

    DEBUGMSG(ZONE_FUNCTION,(TEXT("+IIC_Init\r\n")));

    // Allocate our control structure.
    pInitContext  =  (PHW_INIT_INFO)LocalAlloc(LPTR, sizeof(HW_INIT_INFO));

    // Check that LocalAlloc did stuff ok too.
    if ( !pInitContext ) {
        DEBUGMSG(ZONE_ERROR,
                 (TEXT("Error allocating memory for pInitContext, IIC_Init failed\n\r")));
        return(NULL);
    }


    // Initially, open list is empty.
    InitializeListHead( &pInitContext->OpenList );
    InitializeCriticalSection(&(pInitContext->OpenCS));


    /* Initialize the critical sections that will guard the parts of
     * the receive and transmit action.
     */
    InitializeCriticalSection(&(pInitContext->CritSec));
    
    pInitContext->pAccessOwner = NULL;
    
    

    /* Want to use the Identifier to do RegOpenKey and RegQueryValue (?)
     * to get the index to be passed to GetHWObj.
     * The HWObj will also have a flag denoting whether to start the
     * listening thread or provide the callback.
     */
    DEBUGMSG(ZONE_INFO, (TEXT("Try to open %s\r\n"), pContext));
    hKey = OpenDeviceKey(pContext);
    if ( !hKey ) {
        DEBUGMSG (ZONE_ERROR,
                  (TEXT("Failed to open devkeypath, IIC_Init failed\r\n")));
        IIC_Deinit(pInitContext);
        return(NULL);
    }

    datasize = sizeof(DWORD);
    if ( RegQueryValueEx(hKey, L"Priority256", NULL, &kvaluetype,
                         (LPBYTE)&pInitContext->Priority256, &datasize) ) {
        pInitContext->Priority256 = DEFAULT_CE_THREAD_PRIORITY;
        DEBUGMSG (ZONE_WARN,
                  (TEXT("Failed to get Priority256 value, defaulting to %d\r\n"), pInitContext->Priority256));
    }
    
    if ( RegQueryValueEx(hKey, L"SlaveAddress", NULL, &kvaluetype,
                         (LPBYTE)&pInitContext->PDDCommonVal.SlaveAddress, &datasize) ) {
        pInitContext->PDDCommonVal.SlaveAddress = DEFAULT_SLAVE_ADDRESS;
        DEBUGMSG (ZONE_WARN,
                  (TEXT("Failed to get SlaveAddress value, defaulting to %d\r\n"), pInitContext->PDDCommonVal.SlaveAddress));
    }
    
    if ( RegQueryValueEx(hKey, L"Mode", NULL, &kvaluetype,
                         (LPBYTE)&pInitContext->PDDCommonVal.InterruptEnable, &datasize) ) {
        pInitContext->PDDCommonVal.InterruptEnable = DEFAULT_INTERRUPT_ENABLE;
        DEBUGMSG (ZONE_WARN,
                  (TEXT("Failed to get InterruptEnable value, defaulting to %d\r\n"), pInitContext->PDDCommonVal.InterruptEnable));
    }        

    RegCloseKey (hKey);



    pInitContext->State = IIC_RUN;
    /* Check that HW_Init did stuff ok.  From here on out, call Deinit function
     * when things fail.
     */
    if ( !HW_Init(pInitContext) ) {
        DEBUGMSG (ZONE_ERROR,
                  (TEXT("Hardware doesn't init correctly, IIC_Init failed\r\n")));
        IIC_Deinit(pInitContext);
        return(NULL);
    } 

    g_Dx = D0;
    
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-IIC_Init\r\n")));
    return(pInitContext);
}

//////////
// Function Name : IIC_Deinit
// Function Description : IIC device De-initialization.
// Input : PHW_INIT_INFO    pInitContext
// Output : The return is a BOOL, representing success (TRUE) or failure (FALSE).
// Version : v0.5
BOOL
IIC_Deinit(
         PHW_INIT_INFO    pInitContext                        /* Context pointer returned from IIC_Init*/
        )
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+IIC_Deinit\r\n")));    
    
    if ( !pInitContext ) {
        /* Can't do much without this */
        DEBUGMSG (ZONE_ERROR,
                  (TEXT("IIC_Deinit can't find pInitContext\r\n")));
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }    
    
    /*
    ** Call close, if we have a user.  Note that this call will ensure that
    ** all users are out of the routines before it returns, so we can
    ** go ahead and free our internal memory.
    */
    EnterCriticalSection(&(pInitContext->OpenCS));
    if ( pInitContext->OpenCnt ) {
        PLIST_ENTRY     pEntry;
        PHW_OPEN_INFO   pOpenContext;

        pEntry = pInitContext->OpenList.Flink;
        while ( pEntry != &pInitContext->OpenList ) {
            pOpenContext = CONTAINING_RECORD( pEntry, HW_OPEN_INFO, llist);
            pEntry = pEntry->Flink;  // advance to next

            DEBUGMSG (ZONE_INFO, (TEXT(" Deinit - Closing Handle 0x%X\r\n"),
                                               pOpenContext ));
            IIC_Close(pOpenContext);
        }
    }
    LeaveCriticalSection(&(pInitContext->OpenCS));    
    
    /* Free our resources */


    DeleteCriticalSection(&(pInitContext->CritSec));
    DeleteCriticalSection(&(pInitContext->OpenCS));
        
    pInitContext->State = IIC_FINISH;        
    /* Now, call HW specific deinit function */        
    HW_Deinit(pInitContext);
            
    LocalFree(pInitContext);    
    
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-IIC_Deinit\r\n")));
    return(TRUE);    
}

//////////
// Function Name : IIC_Open
// Function Description : IIC device initialization.
/*        This routine must be called by the user to open the
 *      IIC device. The HANDLE returned must be used by the application in
 *      all subsequent calls to the IIC driver.
 *      Exported to users.
 */
// Input : HANDLE  pHead, DWORD   AccessCode, DWORD   ShareMode
// Output : This routine returns a HANDLE representing the device.
// Version : v0.1
HANDLE
IIC_Open(
        HANDLE  pHead,          // Handle returned by IIC_Init.
        DWORD   AccessCode,     // access code.
        DWORD   ShareMode       // share mode - Not used in this driver.
        )
{
    PHW_INIT_INFO      pInitContext = (PHW_INIT_INFO)pHead;
    PHW_OPEN_INFO   pOpenContext;    
    
    DEBUGMSG (ZONE_FUNCTION, (TEXT("+IIC_Open handle x%X, access x%X, share x%X\r\n"),
                                        pHead, AccessCode, ShareMode));   
                                        
    // Return NULL if pInitContext failed.
    if ( !pInitContext )
    {
        DEBUGMSG (ZONE_ERROR,
                  (TEXT("Open attempted on uninited device!\r\n")));
        SetLastError(ERROR_INVALID_HANDLE);
        return(NULL);
    }   
    
    if (AccessCode & DEVACCESS_BUSNAMESPACE )
    {
        AccessCode &=~(GENERIC_READ |GENERIC_WRITE|GENERIC_EXECUTE|GENERIC_ALL);
    }
    
    // OK, lets allocate an open structure
    pOpenContext    =  (PHW_OPEN_INFO)LocalAlloc(LPTR, sizeof(HW_OPEN_INFO));
    if ( !pOpenContext )
    {
        DEBUGMSG (ZONE_ERROR,
                 (TEXT("Error allocating memory for pOpenContext, IIC_Open failed\n\r")));
        return(NULL);
    }           
    
    // Init the structure
    pOpenContext->pInitContext = pInitContext;  // pointer back to our parent
    pOpenContext->StructUsers = 0;
    pOpenContext->AccessCode = AccessCode;
    pOpenContext->ShareMode = ShareMode;

    // if we have access permissions, note it in pInitContext
    if ( AccessCode & (GENERIC_READ | GENERIC_WRITE) )
    {
        DEBUGMSG(ZONE_INFO,
                 (TEXT("IIC_Open: Access permission handle granted x%X\n\r"),
                  pOpenContext));
        pInitContext->pAccessOwner = pOpenContext;
    }
    
    // add this open entry to list of open entries.
    // Note that we hold the open CS for the duration of the routine since
    // all of our state info is in flux during this time.  In particular,
    // without the CS is would be possible for an open & close to be going on
    // simultaneously and have bad things happen like spinning a new event
    // thread before the old one was gone, etc.
    EnterCriticalSection(&(pInitContext->OpenCS));
    InsertHeadList(&pInitContext->OpenList,
                   &pOpenContext->llist);

    // We do special for Power Manger and Device Manager.
    if (pOpenContext->AccessCode &  DEVACCESS_BUSNAMESPACE )
    {
        // OK, We do not need initialize pOpenContext and start any thread. return the handle now.
        LeaveCriticalSection(&(pInitContext->OpenCS));
        DEBUGMSG(ZONE_FUNCTION, (TEXT("-IIC_Open handle x%X, x%X, Ref x%X\r\n"),
                                        pOpenContext, pOpenContext->pInitContext, pInitContext->OpenCnt));
        return(pOpenContext);
    }
    
    if ( ! pInitContext->OpenCnt )
    {
        DEBUGMSG(ZONE_INFO,
                 (TEXT("IIC_Open: First open : Do Init x%X\n\r"),
                  pOpenContext));
                  
        if ( !HW_OpenFirst(pOpenContext) )
        {
            DEBUGMSG (ZONE_ERROR, (TEXT("HW Open First failed.\r\n")));
            goto OpenFail;
        }                  
        
        HW_PowerUp(pInitContext);
    }   
    
    if ( !HW_Open(pOpenContext) )
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("HW Open failed.\r\n")));
        goto OpenFail;
    }     

    ++(pInitContext->OpenCnt);

    // OK, we are finally back in a stable state.  Release the CS.
    LeaveCriticalSection(&(pInitContext->OpenCS));    
    
    
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-IIC_Open handle x%X, x%X, Ref x%X\r\n"),
                                        pOpenContext, pOpenContext->pInitContext, pInitContext->OpenCnt));

    return(pOpenContext); 
    

OpenFail :
    DEBUGMSG(ZONE_FUNCTION, (TEXT("-IIC_Open handle x%X, x%X, Ref x%X\r\n"),
                                        NULL, pOpenContext->pInitContext, pInitContext->OpenCnt));

    SetLastError(ERROR_OPEN_FAILED);

    // If this was the handle with access permission, remove pointer
    if ( pOpenContext == pInitContext->pAccessOwner )
        pInitContext->pAccessOwner = NULL;

    // Remove the Open entry from the linked list
    RemoveEntryList(&pOpenContext->llist);

    // OK, everything is stable so release the critical section
    LeaveCriticalSection(&(pInitContext->OpenCS));

    // Free all data allocated in open
    LocalFree( pOpenContext );

    return(NULL);                       
}


//////////
// Function Name : IIC_Close
// Function Description : close the IIC device.
// Input : PHW_OPEN_INFO  pOpenContext
// Output : TRUE if success; FALSE if failure.
// Note : This routine is called by the device manager to close the device.
// Version : v0.5
BOOL
IIC_Close(PHW_OPEN_INFO pOpenContext)    //Context pointer returned from IIC_Open
{
    PHW_INIT_INFO  pInitContext = pOpenContext->pInitContext;

    BOOL            RetCode = TRUE;

    DEBUGMSG (ZONE_FUNCTION, (TEXT("+IIC_Close\r\n")));

    if ( !pInitContext )
    {
        DEBUGMSG (ZONE_ERROR, (TEXT("!!IIC_Close: pInitContext == NULL!!\r\n")));
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    // Use the OpenCS to make sure we don't collide with an in-progress open.
    EnterCriticalSection(&(pInitContext->OpenCS));
    // We do special for Power Manger and Device Manager.
    if (pOpenContext->AccessCode & DEVACCESS_BUSNAMESPACE)
    {
        // Remove the entry from the linked list
        RemoveEntryList(&pOpenContext->llist);

        LocalFree( pOpenContext );
    }
    else
    {
        if ( pInitContext->OpenCnt )
        {
            --(pInitContext->OpenCnt);

            if ( !HW_Close(pOpenContext) )
            {
                DEBUGMSG (ZONE_ERROR, (TEXT("HW Close failed.\r\n")));
                goto CloseFail;
            }     

            if ( ! pInitContext->OpenCnt )
            {
                DEBUGMSG(ZONE_INFO,
                         (TEXT("IIC_Close: Last Close : Do Clost x%X\n\r"),
                          pOpenContext));
                      
                if ( !HW_CloseLast(pOpenContext) )
                {
                    DEBUGMSG (ZONE_ERROR, (TEXT("HW_CloseLast failed.\r\n")));
                }        
            
                HW_PowerDown(pInitContext);          
            }   

            // If this was the handle with access permission, remove pointer
            if ( pOpenContext == pInitContext->pAccessOwner )
            {
                DEBUGMSG(ZONE_INFO,
                         (TEXT("IIC_Close: Closed access owner handle\n\r"),
                          pOpenContext));

                pInitContext->pAccessOwner = NULL;
            }

            // Remove the entry from the linked list
            RemoveEntryList(&pOpenContext->llist);

            // Free all data allocated in open
            LocalFree( pOpenContext );
        }
        else
        {
            DEBUGMSG (ZONE_ERROR, (TEXT("!!Close of non-open port\r\n")));
            SetLastError(ERROR_INVALID_HANDLE);
            RetCode = FALSE;
        }
    }

    // OK, other inits/opens can go ahead.
    LeaveCriticalSection(&(pInitContext->OpenCS));

CloseFail:
    DEBUGMSG (ZONE_FUNCTION, (TEXT("-IIC_Close\r\n")));
    return(RetCode);
}


//////////
// Function Name : IIC_IOControl
// Function Description : Device IO control routine.
// Input : //          DWORD | pOpenContext | value returned from IIC_Open call
//                     DWORD | dwCode | io control code to be performed
//                  PBYTE | pBufIn | input data to the device
//                  DWORD | dwLenIn | number of bytes being passed in
//                  PBYTE | pBufOut | output data from the device
//                  DWORD | dwLenOut |maximum number of bytes to receive from device
//                  PDWORD | pdwActualOut | actual number of bytes received from device
// Output : TRUE if success; FALSE if failure.
// Note : 
// Version : v0.1


BOOL
IIC_IOControl(PHW_OPEN_INFO pOpenContext,
              DWORD dwCode, PBYTE pBufIn,
              DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut,
              PDWORD pdwActualOut)
{
    BOOL            RetVal           = TRUE;        // Initialize to success
    PHW_INIT_INFO   pInitContext;
    IIC_IO_DESC     IoDescIn;
    IIC_IO_DESC     IoDescOut;
    PVOID           pUnMarshalledInBuf = NULL;
    PVOID           pUnMarshalledOutBuf = NULL;
	BOOL			securedFuncs = FALSE;
	
	securedFuncs = 	((dwCode == IOCTL_POWER_CAPABILITIES) | 
					(dwCode == IOCTL_POWER_QUERY) |
					(dwCode == IOCTL_POWER_SET));
    //if caller is not kernel mode, do not allow setting power state
    if ( securedFuncs && (GetDirectCallerProcessId() != GetCurrentProcessId()) )
	{
        return ERROR_ACCESS_DENIED;
    }

    if (pOpenContext==NULL) {
        SetLastError (ERROR_INVALID_HANDLE);
        return(FALSE);
    }
    pInitContext = pOpenContext->pInitContext;
    if ( pInitContext == NULL )
    {
        SetLastError (ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    DEBUGMSG (ZONE_FUNCTION,
              (TEXT("+IIC_IOControl(0x%X, %d, 0x%X, %d, 0x%X, %d, 0x%X)\r\n"),
               pOpenContext, dwCode, pBufIn, dwLenIn, pBufOut,
               dwLenOut, pdwActualOut));
   
    if ( !pInitContext->OpenCnt ) {
        DEBUGMSG (ZONE_ERROR,
                  (TEXT(" IIC_IOControl - device was closed\r\n")));
        SetLastError (ERROR_INVALID_HANDLE);
        return(FALSE);
    }
   
    switch ( dwCode ) {  
    case IOCTL_POWER_CAPABILITIES:
        if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(POWER_CAPABILITIES)) )
        {
            RetVal = FALSE;
            SetLastError (ERROR_INVALID_PARAMETER);                
            break;
        }

        __try
        {
            memcpy(pBufOut, &g_PowerCaps, sizeof(POWER_CAPABILITIES));
            *pdwActualOut = sizeof(POWER_CAPABILITIES);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RetVal = FALSE;
            SetLastError (ERROR_INVALID_PARAMETER);                
            break;
        }
        break;    
        
    case IOCTL_POWER_QUERY:
        break;
        
    case IOCTL_POWER_SET:
        break;
        
    case IOCTL_IIC_WRITE:
        if ( (dwLenIn < sizeof(IIC_IO_DESC)) || (NULL == pBufIn) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }        

        EnterCriticalSection(&(pInitContext->CritSec));
        
        __try
        {
            memcpy(&IoDescIn, pBufIn, sizeof(IIC_IO_DESC));
            pUnMarshalledInBuf = IoDescIn.Data;
            if(FAILED(CeOpenCallerBuffer((PVOID *)&IoDescIn.Data, pUnMarshalledInBuf, IoDescIn.Count, ARG_I_PTR, FALSE)))
            {
                RETAILMSG(1, (TEXT("IIC_IOControl: CeOpenCallerBuffer failed in IOCTL_IIC_WRITE for IN buf.\r\n")));
                RetVal = FALSE;
                break;
            }

            if((HW_Write(pOpenContext, &IoDescIn)==NULL))
            {
                SetLastError(ERROR_TIMEOUT);
                RetVal = FALSE;                
            }

            if(FAILED(CeCloseCallerBuffer(IoDescIn.Data, pUnMarshalledInBuf, IoDescIn.Count, ARG_I_PTR)))
            {
                RETAILMSG(1, (TEXT("IIC_IOControl: CeCloseCallerBuffer failed in IOCTL_IIC_WRITE for IN buf.\r\n")));
                return FALSE;
            }

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RetVal = FALSE;
            SetLastError (ERROR_INVALID_PARAMETER);                
        }        
        
        LeaveCriticalSection(&(pInitContext->CritSec));    
        break;
        
    case IOCTL_IIC_READ:    
        if ( (dwLenIn < sizeof(IIC_IO_DESC)) || (NULL == pBufIn) || (dwLenOut < sizeof(IIC_IO_DESC)) || (NULL == pBufOut) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }            

        EnterCriticalSection(&(pInitContext->CritSec));    
        __try
        {
            memcpy(&IoDescIn, pBufIn, sizeof(IIC_IO_DESC));
            pUnMarshalledInBuf = IoDescIn.Data;
            if(FAILED(CeOpenCallerBuffer((PVOID *)&IoDescIn.Data, pUnMarshalledInBuf, IoDescIn.Count, ARG_I_PTR, FALSE)))
            {
                RETAILMSG(1, (TEXT("IIC_IOControl: CeOpenCallerBuffer failed in IOCTL_IIC_READ for IN buf.\r\n")));
                RetVal = FALSE;
                break;
            }

            memcpy(&IoDescOut, pBufIn, sizeof(IIC_IO_DESC));
            pUnMarshalledOutBuf = IoDescOut.Data;
            if(FAILED(CeOpenCallerBuffer((PVOID *)&IoDescOut.Data, pUnMarshalledOutBuf, IoDescOut.Count, ARG_O_PTR, FALSE)))
            {
                RETAILMSG(1, (TEXT("IIC_IOControl: CeOpenCallerBuffer failed in IOCTL_IIC_READ for OUT buf.\r\n")));
                RetVal = FALSE;
                break;
            }
            
            if(HW_Read(pOpenContext, &IoDescIn, &IoDescOut))
            {
                // success
                *pdwActualOut = sizeof(IIC_IO_DESC);                        
            }
            else
            {
                SetLastError(ERROR_TIMEOUT);
                *pdwActualOut = 0;
                RetVal = FALSE;            
            }
            
            if(FAILED(CeCloseCallerBuffer(IoDescIn.Data, pUnMarshalledInBuf, IoDescIn.Count, ARG_I_PTR)))
            {
                RETAILMSG(1, (TEXT("IIC_IOControl: CeCloseCallerBuffer failed in IOCTL_IIC_READ for IN buf.\r\n")));
                RetVal = FALSE;
            }
            if(FAILED(CeCloseCallerBuffer(IoDescOut.Data, pUnMarshalledOutBuf, IoDescOut.Count, ARG_O_PTR)))
            {
                RETAILMSG(1, (TEXT("IIC_IOControl: CeCloseCallerBuffer failed in IOCTL_IIC_READ for OUT buf.\r\n")));
                RetVal = FALSE;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RetVal = FALSE;
            SetLastError (ERROR_INVALID_PARAMETER);                
        }  

        LeaveCriticalSection(&(pInitContext->CritSec));    
        break;
                
    case IOCTL_IIC_SET_CLOCK:    
        if ( (dwLenIn < sizeof(UINT32)) || (NULL == pBufIn) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }

        EnterCriticalSection(&(pInitContext->CritSec));
        __try
        {
            pOpenContext->PDDContextVal.Clock = *(UINT32*)pBufIn;
            HW_SetClock(pOpenContext);
            pOpenContext->DirtyBit = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_SET_CLOCK] exception...\n"));
            RetVal = FALSE;
        }

        LeaveCriticalSection(&(pInitContext->CritSec));
        break;
        
    case IOCTL_IIC_GET_CLOCK:    
        if ( (dwLenOut < sizeof(UINT32)) || (NULL == pBufOut) ||
             (NULL == pdwActualOut) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }

        __try
        {
            *(UINT32*)pBufOut = pOpenContext->PDDContextVal.Clock;
            // Return the size
            *pdwActualOut = sizeof(UINT32);    
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_GET_CLOCK] exception...\n"));
            RetVal = FALSE;
        }
        break;
        
    case IOCTL_IIC_SET_MODE:
        if ( (dwLenIn < sizeof(UINT32)) || (NULL == pBufIn) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }    

        __try
        {
            pOpenContext->PDDContextVal.ModeSel = (IIC_MODE)*(UINT32*)pBufIn;
            pOpenContext->DirtyBit = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_SET_MODE] exception...\n"));
            RetVal = FALSE;
        }
        break;
                
    case IOCTL_IIC_GET_MODE:        
        if ( (dwLenOut < sizeof(UINT32)) || (NULL == pBufOut) ||
             (NULL == pdwActualOut) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }

        __try
        {
            *(UINT32*)pBufOut = pOpenContext->PDDContextVal.ModeSel;
            // Return the size
            *pdwActualOut = sizeof(UINT32);        
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_GET_MODE] exception...\n"));
            RetVal = FALSE;
        }
        break;
        
    case IOCTL_IIC_SET_FILTER:        
        if ( (dwLenIn < sizeof(UINT32)) || (NULL == pBufIn) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }    

        __try
        {
            pOpenContext->PDDContextVal.FilterEnable = (*(UINT32*)pBufIn) ? 1 : 0; // Filter Enable is a bit  a non-zero input value will turn this bit on
            pOpenContext->DirtyBit = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_SET_FILTER] exception...\n"));
            RetVal = FALSE;
        }
        break;
        
    case IOCTL_IIC_GET_FILTER:    
        if ( (dwLenOut < sizeof(UINT32)) || (NULL == pBufOut) ||
             (NULL == pdwActualOut) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }

        __try
        {
            *(UINT32*)pBufOut = pOpenContext->PDDContextVal.FilterEnable;
            // Return the size
            *pdwActualOut = sizeof(UINT32);        
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_GET_FILTER] exception...\n"));
            RetVal = FALSE;
        }
        break;
        
    case IOCTL_IIC_SET_DELAY:        
        if ( (dwLenIn < sizeof(UINT32)) || (NULL == pBufIn) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }    

        __try
        {
            pOpenContext->PDDContextVal.Delay = (IIC_DELAY)*(UINT32*)pBufIn;
            pOpenContext->DirtyBit = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_SET_DELAY] exception...\n"));
            RetVal = FALSE;
        }
        break;
                                                                
    case IOCTL_IIC_GET_DELAY:        
        if ( (dwLenOut < sizeof(UINT32)) || (NULL == pBufOut) ||
             (NULL == pdwActualOut) ) {
            SetLastError (ERROR_INVALID_PARAMETER);
            RetVal = FALSE;
            DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Invalid parameter\r\n")));
            break;
        }

        __try
        {
            *(UINT32*)pBufOut = pOpenContext->PDDContextVal.Delay;
            // Return the size
            *pdwActualOut = sizeof(UINT32);        
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[IOCTL_IIC_GET_FILTER] exception...\n"));
            RetVal = FALSE;
        }
        break;

    default:
        DEBUGMSG (ZONE_ERROR, (TEXT("IIC_IOControl: Unknown IOCTL code (%d)\r\n"), dwCode));
        SetLastError (ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
    }
    
    DEBUGMSG (ZONE_FUNCTION|(RetVal == FALSE?ZONE_ERROR:0),
              (TEXT("-IIC_IOControl %s Ecode=%d (len=%d)\r\n"),
               (RetVal == TRUE) ? TEXT("Success") : TEXT("Error"),
               GetLastError(), (NULL == pdwActualOut) ? 0 : *pdwActualOut));

    return(RetVal);        
}

void IIC_PowerUp(
            PHW_INIT_INFO    pInitContext                        /* Context pointer returned from IIC_Init*/
)
{
    HW_PowerUp(pInitContext);
}

void IIC_PowerDown(
            PHW_INIT_INFO    pInitContext                        /* Context pointer returned from IIC_Init*/
)
{
    HW_PowerDown(pInitContext);
}