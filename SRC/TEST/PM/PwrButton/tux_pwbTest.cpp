//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
////////////////////////////////////////////////////////////////////////////////
//
//  tux_pwb TUX DLL
//
//  Module: tux_pwbTest.cpp
//          Contains the shell processing function.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"
#include <pm.h>
#include "resource.h"
#include "DbgApi.h"


#define DEFAULT_ZONES 0x0007

#define ZONE_INIT		DEBUGZONE(0)
#define ZONE_FUNCTION	DEBUGZONE(1)
#define ZONE_ERROR		DEBUGZONE(2)

DBGPARAM dpCurSettings = 
{ 
	TEXT("PWBTUX"), 
	{ 
		TEXT("Init"), TEXT("Function"), 
		TEXT("Error"), TEXT("Undefined"), 
		TEXT("Undefined"), TEXT("Undefined"), 
		TEXT("Undefined"), TEXT("Undefined"), 
		TEXT("Undefined"), TEXT("Undefined"), 
		TEXT("Undefined"), TEXT("Undefined"), 
		TEXT("Undefined"), TEXT("Undefined"), 
		TEXT("Undefined"), TEXT("Undefined") 
	},
	DEFAULT_ZONES 
};

#define QUEUE_ENTRIES   16
#define MAX_NAMELEN     64
#define QUEUE_SIZE      (QUEUE_ENTRIES*(sizeof(POWER_BROADCAST)+MAX_NAMELEN))

HANDLE g_PwrMonThread = NULL;
BOOL   g_bFlagExitThrd = FALSE;
HANDLE  g_hevWaitPowerStateChange=NULL;
BOOL g_bEnterSuspend=FALSE;
BOOL g_bEnterResume=FALSE;
HANDLE  g_hWaitResume=NULL;
HINSTANCE g_hInst = NULL;


////////////////////////////////////////////////////////////////////////////////
// PwrMonThread
//  Power state monitor thread
//
// Parameters:
//  none
//
// Return value:
//  The return value indicates the success or failure of this function.
//
INT WINAPI PwrMonThread(void)
{
    HANDLE hNotifications;
    HANDLE hReadMsgQ;
    MSGQUEUEOPTIONS msgOptions = {0}; 

    // Create a message queue for Power Manager notifications.
    msgOptions.dwSize        = sizeof(MSGQUEUEOPTIONS);
    msgOptions.dwFlags       = 0;
    msgOptions.dwMaxMessages = QUEUE_ENTRIES;
    msgOptions.cbMaxMessage  = sizeof(POWER_BROADCAST) + MAX_NAMELEN;
    msgOptions.bReadAccess   = TRUE;

    hReadMsgQ = CreateMsgQueue(NULL, &msgOptions);
    if ( hReadMsgQ == NULL )
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("[TUXPWB]: CreateMsgQueue(Read): error %d\r\n"), GetLastError()));
        g_bFlagExitThrd = TRUE;
    }

    // Request Power notifications
    hNotifications = RequestPowerNotifications(hReadMsgQ, POWER_NOTIFY_ALL);
    if ( ! hNotifications ) 
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("[TUXPWB]: RequestPowerNotifications: error %d\r\n"), GetLastError()));
        g_bFlagExitThrd = TRUE;
    }
	
    while(!g_bFlagExitThrd)
    {
        DWORD   iBytesInQueue = 0;
        DWORD dwFlags;
        UCHAR buf[QUEUE_SIZE];
        PPOWER_BROADCAST pB = (PPOWER_BROADCAST) buf;

        memset(buf, 0, QUEUE_SIZE);
        DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: Waiting for PM state transition notification\r\n")));
 

        // Read message from queue.
        if ( ! ReadMsgQueue(hReadMsgQ, buf, QUEUE_SIZE, &iBytesInQueue, INFINITE, &dwFlags) )
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("[TUXPWB]: ReadMsgQueue: ERROR:%d\r\n"), GetLastError()));
        } 
        else if ( iBytesInQueue < sizeof(POWER_BROADCAST) )
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("[TUXPWB]: Received short message: %d bytes, expected: %d\r\n"),
                                  iBytesInQueue, sizeof(POWER_BROADCAST)));
        }
        else 
        {
            switch ( pB->Message )
            {
                case PBT_TRANSITION:
    
                    RETAILMSG(TRUE, (TEXT("[TUXPWB]: PBT_TRANSITION to system power state [Flags: 0x%x]: '%s'\r\n"),
                                  pB->Flags, pB->SystemPowerState));
    
                    switch ( POWER_STATE(pB->Flags) )
                    {
                        case POWER_STATE_ON:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_ON\r\n")));
                            break;
    
                        case POWER_STATE_OFF:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_OFF\r\n")));
                            break;
    
                        case POWER_STATE_CRITICAL:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_CRITICAL\r\n")));
                            break;
    
                        case POWER_STATE_BOOT:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_BOOT\r\n")));
                            break;
    
                        case POWER_STATE_IDLE:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_IDLE\r\n")));
                            break;
    
                        case POWER_STATE_SUSPEND:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_SUSPEND\r\n")));
							g_bEnterSuspend=TRUE;
							SetEvent(g_hevWaitPowerStateChange);
                            break;
    
                        case POWER_STATE_RESET:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_RESET\r\n")));
                            break;
    
                        case POWER_STATE_PASSWORD:
                            DEBUGMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: POWER_STATE_PASSWORD\r\n")));
                            break;
    
                        case 0:
                            DEBUGMSG(ZONE_FUNCTION,(TEXT("[TUXPWB]: Power State Flags:0x%x\r\n"),pB->Flags));
                            break;
    
                        default:
                            DEBUGMSG(ZONE_FUNCTION,(TEXT("[TUXPWB]: Unknown Power State Flags:0x%x\r\n"),pB->Flags));
                            break;
                    }
                    break;

                case PBT_RESUME:
                {
                    
					RETAILMSG(TRUE, (TEXT("[TUXPWB]: PBT_RESUME\r\n")));
					g_bEnterResume=TRUE;
					SetEvent(g_hWaitResume);

                }
                    break; 
                case PBT_POWERSTATUSCHANGE:
                    RETAILMSG(TRUE, (TEXT("[TUXPWB]: PBT_POWERSTATUSCHANGE\r\n")));
                    break;
                        
                case PBT_POWERINFOCHANGE:
                {
                    PPOWER_BROADCAST_POWER_INFO ppbpi = (PPOWER_BROADCAST_POWER_INFO) pB->SystemPowerState;
    
                    RETAILMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: PBT_POWERINFOCHANGE\r\n")));
                    RETAILMSG(ZONE_FUNCTION, (TEXT("[TUXPWB]: \tAC line status %u, battery flag %u, backup flag %u, %d levels\r\n"),
                            ppbpi->bACLineStatus, ppbpi->bBatteryFlag, 
                            ppbpi->bBackupBatteryFlag, ppbpi->dwNumLevels));
                    break;
                }
                        
                default:
                    DEBUGMSG(ZONE_ERROR, (TEXT("[TUXPWB]: Unknown Message:%d\r\n"), pB->Message));
                    break;
            }
        }
    }

    if ( hNotifications )
    {
        StopPowerNotifications(hNotifications);
        hNotifications = NULL;
    }

    if ( hReadMsgQ )
    {
        CloseMsgQueue(hReadMsgQ);
        hReadMsgQ = NULL;
    }

    return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// DllMain
//  Main entry point of the DLL. Called when the DLL is loaded or unloaded.
//
// Parameters:
//  hInstance       Module instance of the DLL.
//  dwReason        Reason for the function call.
//  lpReserved      Reserved for future use.
//
// Return value:
//  TRUE if successful, FALSE to indicate an error condition.

BOOL WINAPI DllMain(HANDLE hInstance, ULONG dwReason, LPVOID lpReserved)
{
	switch (dwReason)    {
	  case DLL_PROCESS_ATTACH:
		  g_hInst = (HINSTANCE)hInstance;
		  DEBUGREGISTER(g_hInst);
		  break;
	  case DLL_PROCESS_DETACH:
	  default:
		  break;
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
// Debug
//  Printf-like wrapping around OutputDebugString.
//
// Parameters:
//  szFormat        Formatting string (as in printf).
//  ...             Additional arguments.
//
// Return value:
//  None.

void Debug(LPCTSTR szFormat, ...)
{
    static  TCHAR   szHeader[] = TEXT("tux_pwb: ");
    TCHAR   szBuffer[1024];

    va_list pArgs;
    va_start(pArgs, szFormat);
    lstrcpy(szBuffer, szHeader);
    wvsprintf(
        szBuffer + countof(szHeader) - 1,
        szFormat,
        pArgs);
    va_end(pArgs);

    _tcscat(szBuffer, TEXT("\r\n"));

    OutputDebugString(szBuffer);
}


////////////////////////////////////////////////////////////////////////////////
// ShellProc
//  Processes messages from the TUX shell.
//
// Parameters:
//  uMsg            Message code.
//  spParam         Additional message-dependent data.
//
// Return value:
//  Depends on the message.

SHELLPROCAPI ShellProc(UINT uMsg, SPPARAM spParam)
{
    LPSPS_BEGIN_TEST    pBT;
    LPSPS_END_TEST      pET;

    switch (uMsg)
    {
    case SPM_LOAD_DLL:
        // Sent once to the DLL immediately after it is loaded. The spParam
        // parameter will contain a pointer to a SPS_LOAD_DLL structure. The
        // DLL should set the fUnicode member of this structure to TRUE if the
        // DLL is built with the UNICODE flag set. If you set this flag, Tux
        // will ensure that all strings passed to your DLL will be in UNICODE
        // format, and all strings within your function table will be processed
        // by Tux as UNICODE. The DLL may return SPR_FAIL to prevent the DLL
        // from continuing to load.
        Debug(TEXT("ShellProc(SPM_LOAD_DLL, ...) called"));

        // If we are UNICODE, then tell Tux this by setting the following flag.
#ifdef UNICODE
        ((LPSPS_LOAD_DLL)spParam)->fUnicode = TRUE;
#endif // UNICODE
        g_pKato = (CKato*)KatoGetDefaultObject();
	
		//Create thread for monitor power state changed
		if ((g_PwrMonThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)PwrMonThread,NULL,0,NULL)) == NULL)
		{
			DEBUGMSG(ZONE_FUNCTION,(_T("[TUXPWB]: Failed to create Power Monitor Thread\r\n")));
		}
		
		//Create event for power state changed
		g_hevWaitPowerStateChange = CreateEvent(NULL,FALSE,FALSE,NULL);
		if (g_hevWaitPowerStateChange == NULL) 
		{
			RETAILMSG(TRUE, (TEXT("[TUXPWB]: failed to Create g_hevWaitPowerStateChange Event\r\n")));
		}
		
		//Create event for resume waiting
		g_hWaitResume = CreateEvent(NULL,FALSE,FALSE,NULL);
		if (g_hWaitResume == NULL) 
		{
			RETAILMSG(TRUE, (TEXT("[TUXPWB]: failed to Create g_hWaitResume Event\r\n")));
		}
			break;

    case SPM_UNLOAD_DLL:
        // Sent once to the DLL immediately before it is unloaded.
        Debug(TEXT("ShellProc(SPM_UNLOAD_DLL, ...) called"));

		g_bFlagExitThrd = TRUE;
		if (g_PwrMonThread)
		{    
		   CloseHandle(g_PwrMonThread);
		   g_PwrMonThread=NULL;
		}    

		if(g_hevWaitPowerStateChange)
		{
			CloseHandle(g_hevWaitPowerStateChange);
			g_hevWaitPowerStateChange=NULL;
		}

		if(g_hWaitResume)
		{
			CloseHandle(g_hWaitResume);
			g_hWaitResume=NULL;
		}

        break;

    case SPM_SHELL_INFO:
        // Sent once to the DLL immediately after SPM_LOAD_DLL to give the DLL
        // some useful information about its parent shell and environment. The
        // spParam parameter will contain a pointer to a SPS_SHELL_INFO
        // structure. The pointer to the structure may be stored for later use
        // as it will remain valid for the life of this Tux Dll. The DLL may
        // return SPR_FAIL to prevent the DLL from continuing to load.
        Debug(TEXT("ShellProc(SPM_SHELL_INFO, ...) called"));

        // Store a pointer to our shell info for later use.
        g_pShellInfo = (LPSPS_SHELL_INFO)spParam;
        break;

    case SPM_REGISTER:
        // This is the only ShellProc() message that a DLL is required to
        // handle (except for SPM_LOAD_DLL if you are UNICODE). This message is
        // sent once to the DLL immediately after the SPM_SHELL_INFO message to
        // query the DLL for its function table. The spParam will contain a
        // pointer to a SPS_REGISTER structure. The DLL should store its
        // function table in the lpFunctionTable member of the SPS_REGISTER
        // structure. The DLL may return SPR_FAIL to prevent the DLL from
        // continuing to load.
        Debug(TEXT("ShellProc(SPM_REGISTER, ...) called"));
        ((LPSPS_REGISTER)spParam)->lpFunctionTable = g_lpFTE;
#ifdef UNICODE
        return SPR_HANDLED | SPF_UNICODE;
#else // UNICODE
        return SPR_HANDLED;
#endif // UNICODE

    case SPM_START_SCRIPT:
        // Sent to the DLL immediately before a script is started. It is sent
        // to all Tux DLLs, including loaded Tux DLLs that are not in the
        // script. All DLLs will receive this message before the first
        // TestProc() in the script is called.
        Debug(TEXT("ShellProc(SPM_START_SCRIPT, ...) called"));
        break;

    case SPM_STOP_SCRIPT:
        // Sent to the DLL when the script has stopped. This message is sent
        // when the script reaches its end, or because the user pressed
        // stopped prior to the end of the script. This message is sent to
        // all Tux DLLs, including loaded Tux DLLs that are not in the script.
        Debug(TEXT("ShellProc(SPM_STOP_SCRIPT, ...) called"));
        break;

    case SPM_BEGIN_GROUP:
        // Sent to the DLL before a group of tests from that DLL is about to
        // be executed. This gives the DLL a time to initialize or allocate
        // data for the tests to follow. Only the DLL that is next to run
        // receives this message. The prior DLL, if any, will first receive
        // a SPM_END_GROUP message. For global initialization and
        // de-initialization, the DLL should probably use SPM_START_SCRIPT
        // and SPM_STOP_SCRIPT, or even SPM_LOAD_DLL and SPM_UNLOAD_DLL.
        Debug(TEXT("ShellProc(SPM_BEGIN_GROUP, ...) called"));
        g_pKato->BeginLevel(0, TEXT("BEGIN GROUP: tux_pwb.DLL"));
        break;

    case SPM_END_GROUP:
        // Sent to the DLL after a group of tests from that DLL has completed
        // running. This gives the DLL a time to cleanup after it has been
        // run. This message does not mean that the DLL will not be called
        // again; it just means that the next test to run belongs to a
        // different DLL. SPM_BEGIN_GROUP and SPM_END_GROUP allow the DLL
        // to track when it is active and when it is not active.
        Debug(TEXT("ShellProc(SPM_END_GROUP, ...) called"));
        g_pKato->EndLevel(TEXT("END GROUP: tux_pwb.DLL"));
        break;

    case SPM_BEGIN_TEST:
        // Sent to the DLL immediately before a test executes. This gives
        // the DLL a chance to perform any common action that occurs at the
        // beginning of each test, such as entering a new logging level.
        // The spParam parameter will contain a pointer to a SPS_BEGIN_TEST
        // structure, which contains the function table entry and some other
        // useful information for the next test to execute. If the ShellProc
        // function returns SPR_SKIP, then the test case will not execute.
        Debug(TEXT("ShellProc(SPM_BEGIN_TEST, ...) called"));
        // Start our logging level.
        pBT = (LPSPS_BEGIN_TEST)spParam;
        g_pKato->BeginLevel(
            pBT->lpFTE->dwUniqueID,
            TEXT("BEGIN TEST: \"%s\", Threads=%u, Seed=%u"),
            pBT->lpFTE->lpDescription,
            pBT->dwThreadCount,
            pBT->dwRandomSeed);
        break;

    case SPM_END_TEST:
        // Sent to the DLL after a single test executes from the DLL.
        // This gives the DLL a time perform any common action that occurs at
        // the completion of each test, such as exiting the current logging
        // level. The spParam parameter will contain a pointer to a
        // SPS_END_TEST structure, which contains the function table entry and
        // some other useful information for the test that just completed. If
        // the ShellProc returned SPR_SKIP for a given test case, then the
        // ShellProc() will not receive a SPM_END_TEST for that given test case.
        Debug(TEXT("ShellProc(SPM_END_TEST, ...) called"));
        // End our logging level.
        pET = (LPSPS_END_TEST)spParam;
        g_pKato->EndLevel(
            TEXT("END TEST: \"%s\", %s, Time=%u.%03u"),
            pET->lpFTE->lpDescription,
            pET->dwResult == TPR_SKIP ? TEXT("SKIPPED") :
            pET->dwResult == TPR_PASS ? TEXT("PASSED") :
            pET->dwResult == TPR_FAIL ? TEXT("FAILED") : TEXT("ABORTED"),
            pET->dwExecutionTime / 1000, pET->dwExecutionTime % 1000);
        break;

    case SPM_EXCEPTION:
        // Sent to the DLL whenever code execution in the DLL causes and
        // exception fault. TUX traps all exceptions that occur while
        // executing code inside a test DLL.
        Debug(TEXT("ShellProc(SPM_EXCEPTION, ...) called"));
        g_pKato->Log(LOG_EXCEPTION, TEXT("Exception occurred!"));
        break;

    default:
        // Any messages that we haven't processed must, by default, cause us
        // to return SPR_NOT_HANDLED. This preserves compatibility with future
        // versions of the TUX shell protocol, even if new messages are added.
        return SPR_NOT_HANDLED;
    }

    return SPR_HANDLED;
}

////////////////////////////////////////////////////////////////////////////////
// ShowDialogBox
//  Open dialog box to show information to user
//
// Parameters:
//  szPromptString	string to display
//
// Return value:
//		Windows handle of opened dialog
//
HWND ShowDialogBox(LPCTSTR szPromptString)
{

	if(szPromptString == NULL)
		return NULL;

	//create a dialog box 
	HWND hDiagWnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SHOWBOX), NULL, NULL);
	if(NULL == hDiagWnd)
	{
		ErrorExit(TEXT("ShowDialogBox"));
	}

	//show dialog   		
	ShowWindow(hDiagWnd, SW_SHOW);

	//show the prompt info
	SetDlgItemText(hDiagWnd, IDC_TEXT1, szPromptString);
	UpdateWindow(hDiagWnd);

	//debug output msg for headless tests.
	Debug(szPromptString);

	return hDiagWnd;

}


////////////////////////////////////////////////////////////////////////////////
// DeleteDialogBox
//  Close opened dialog box
//
// Parameters:
//  hDiagWnd	Handle of opened window
//
// Return value:
//		none
//
VOID DeleteDialogBox(HWND hDiagWnd)
{

	//destroy the dialogbox
	if(NULL != hDiagWnd)
		DestroyWindow(hDiagWnd);
	hDiagWnd = NULL;
}


////////////////////////////////////////////////////////////////////////////////
// ErrorExit
//  Display the error message and exit the process
//
// Parameters:
//  pszFunction		error function name
//
// Return value:
//		none
//
void ErrorExit(LPTSTR pszFunction) 
{ 
	// Retrieve the system error message for the last-error code
	LPTSTR pszMessage;
	DWORD dwLastError = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&pszMessage,
		0, NULL );

	wprintf(L"%s failed with error %d: %s\n", pszFunction, dwLastError, pszMessage);
	LocalFree(pszMessage);
	ExitProcess(dwLastError); 
}


////////////////////////////////////////////////////////////////////////////////
// CheckStopPowerNotification
//  check if StopPowerNotifications API work correctly
//
// Parameters:
//  none
//
// Return value:
//		TRUE success and FALSE fail.
//
BOOL CheckStopPowerNotification()
{
	HANDLE hNotifications;
	HANDLE hReadMsgQ;
	MSGQUEUEOPTIONS msgOptions = {0}; 
	DWORD   iBytesInQueue = 0;
	DWORD dwFlags;
	UCHAR buf[QUEUE_SIZE];
	PPOWER_BROADCAST pB = (PPOWER_BROADCAST) buf;
	BOOL bRet=FALSE;

	// Create a message queue for Power Manager notifications.
	msgOptions.dwSize        = sizeof(MSGQUEUEOPTIONS);
	msgOptions.dwFlags       = 0;
	msgOptions.dwMaxMessages = QUEUE_ENTRIES;
	msgOptions.cbMaxMessage  = sizeof(POWER_BROADCAST) + MAX_NAMELEN;
	msgOptions.bReadAccess   = TRUE;

	hReadMsgQ = CreateMsgQueue(NULL, &msgOptions);
	if ( hReadMsgQ == NULL )
	{
		g_pKato->Log(LOG_FAIL, TEXT("[TUXPWB]: CreateMsgQueue(Read): error %d\r\n"), GetLastError() );
		goto LEAVE;
	}

	// Request Power notifications
	hNotifications = RequestPowerNotifications(hReadMsgQ, POWER_NOTIFY_ALL);
	if ( ! hNotifications ) 
	{
		g_pKato->Log(LOG_FAIL, TEXT("[TUXPWB]: RequestPowerNotifications: error %d\r\n"), GetLastError() );
		goto LEAVE;
	}
	else
	{
		StopPowerNotifications(hNotifications);
		hNotifications = NULL;
	}

	memset(buf, 0, QUEUE_SIZE);
	// Read message from queue.
	if ( ! ReadMsgQueue(hReadMsgQ, buf, QUEUE_SIZE, &iBytesInQueue, INFINITE, &dwFlags) )
	{
		g_pKato->Log(LOG_PASS, TEXT("StopPowerNotifications Successfully!"));
		bRet = TRUE;
	} 
	else
	{
		bRet = FALSE;
	}

LEAVE:
	if (hReadMsgQ )
	{
		CloseMsgQueue(hReadMsgQ);
		hReadMsgQ = NULL;
	}

	return bRet;
}