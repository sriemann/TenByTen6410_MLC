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
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"


extern HANDLE g_hevWaitPowerStateChange;
extern HANDLE g_hWaitResume;
extern BOOL g_bEnterSuspend;
extern BOOL g_bEnterResume;

#define TIMEOUT_WAITSUSPEND 20000
////////////////////////////////////////////////////////////////////////////////
// PWBTUX_SuspendCheck
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI PWBTUX_SuspendCheck(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

	DWORD dwResult=0;
	HWND hWndBox=NULL;
	BOOL bRet=FALSE;

	hWndBox= ShowDialogBox(TEXT("[TUXPWB]Please press power button to enter suspend within 20 secs"));
	dwResult = WaitForSingleObject(g_hevWaitPowerStateChange, TIMEOUT_WAITSUSPEND);
	if(hWndBox)
		DeleteDialogBox(hWndBox);

	if(WAIT_OBJECT_0 != dwResult)
	{
		ERRFAIL("Wait Suspend timeout !!");
		goto LEAVE;
	}

	if(g_bEnterSuspend)
	{
		SUCCESS("Success to enter SUSPEND");
		g_bEnterSuspend=FALSE;
		return TPR_PASS;
		bRet=TRUE;
	}
	else
		ERRFAIL("Fail to enter SUSPEND ");

LEAVE:
	return bRet ? TPR_PASS : TPR_FAIL;
}

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// PWBTUX_ResumeCheck
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI PWBTUX_ResumeCheck(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// The shell doesn't necessarily want us to execute the test. Make sure
	// first.
	if(uMsg != TPM_EXECUTE)
	{
		return TPR_NOT_HANDLED;
	}

	DWORD dwResult;
	BOOL bRet=FALSE;

	dwResult = WaitForSingleObject(g_hWaitResume, TIMEOUT_WAITSUSPEND);
	if(WAIT_OBJECT_0 != dwResult)
	{
		ERRFAIL("Wait Resume timeout !");
		goto LEAVE;

	}

	if(g_bEnterResume)
	{
		HWND hWndBox=NULL;
		SUCCESS("Success to enter RESUME");
		g_bEnterResume=FALSE;


		hWndBox= ShowDialogBox(TEXT("[TUXPWB] Success to enter RESUME"));
		Sleep(2000);
		if(hWndBox)
			DeleteDialogBox(hWndBox);

		bRet=TRUE;
	}
	else
		ERRFAIL("Fail to enter RESUME");

LEAVE:
	return bRet ? TPR_PASS : TPR_FAIL;
}

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// PWBTUX_CheckStopPowerNotification
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI PWBTUX_CheckStopPowerNotification(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// The shell doesn't necessarily want us to execute the test. Make sure
	// first.
	BOOL bRet=FALSE;

	if(uMsg != TPM_EXECUTE)
	{
		return TPR_NOT_HANDLED;
	}
	
	bRet = CheckStopPowerNotification();

	return bRet ? TPR_PASS : TPR_FAIL;

}

////////////////////////////////////////////////////////////////////////////////
