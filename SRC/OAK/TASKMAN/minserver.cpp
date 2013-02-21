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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
/*---------------------------------------------------------------------------*\
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 * 
 *
 *  file:  MinServer.cpp
 *  purpose: 
 *    This file shows how to use the SHELLCB library to implement the 
 *    Shell_NotifyIcon, SHAddToRecentDocs and SHCreateExplorerInstance
 *    shell APIs. This code will run as part of the sample shell and will use
 *    the SHELLCB library to register callback functions to handle these APIs. 
 *    The SHELLCB lib registers these callbacks with the OS. Now when any 
 *    applications call these APIs the callback functions will be called.
 * 
 *    The callback functions are executed on the thread of the calling 
 *    application (the thread is sort of "loaned" to the shell temporarily).
 *    This imposes severe restrictions on what the callback functions can and
 *    cannot do. The reccomended implem is for the callback to make a copy of 
 *    the parameters, alert some other shell thread to process this data and
 *    return immediately, i.e. the callback does not actually do any serious 
 *    work itself. 
 *    NOTE: The callback must make a copy of the actual parameter data, not of 
 *    the data pointer! When the callback returns the pointer will be invalid 
 *    and the data it pointed to will be invalid!
 * 
\*---------------------------------------------------------------------------*/

#include "windows.h"
#include "minshell.h"
#include "shellcb.h"

// local prototypes
BOOL  MyShell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA lpData);
void  MySHAddToRecentDocs(UINT uFlags, LPCVOID pv);

// Register the callback functions. This is called during startup
BOOL MinServer_Init(void)
{

	SHELLCALLBACKS shcb;
	memset(&shcb, 0, sizeof(shcb));

	shcb.dwSize = sizeof(shcb);
	shcb.pfnShell_NotifyIcon = MyShell_NotifyIcon;
	shcb.pfnSHAddToRecentDocs = MySHAddToRecentDocs;
	
	return ShellRegisterCallbacks(&shcb);
}

BOOL  MyShell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA pData)
{
	// we are on the calling app's thread. 
	// make a copy of the NOTIFYICONDATA and forward this to our taskbar thread
	PNOTIFYICONDATA pCopy = MyAlloc(NOTIFYICONDATA);
	if(!pCopy)
		return FALSE;
	memcpy(pCopy, pData, sizeof(NOTIFYICONDATA));
	// foward this to taskbar thread via PostMessage. Taskbar will also free the allocated copy
	PostMessage(g_hwndTaskBar, WM_HANDLESHELLNOTIFYICON, dwMessage, (LPARAM)pCopy);
	return TRUE;
}

void  MySHAddToRecentDocs(UINT uFlags, LPCVOID pv)
{
	// we are on the calling app's thread.
	// make a copy of the path and forward this to our taskbar thread
	// Note we don't handle PIDLs yet in this sample
	if(uFlags != SHARD_PATH)
	{
		RETAILMSG(1, (L"Sample shell does not handle SHARD_PIDL in SHAddToRecentDocs\r\n"));
		ASSERT(FALSE);
		return;
	}

	// MySzDup handle the NULL ptr & empty string cases correctly
	PTSTR pszCopy = MySzDup((PTSTR)pv);

	// foward this to taskbar thread via PostMessage. Taskbar will also free the allocated copy
	PostMessage(g_hwndTaskBar, WM_HANDLESHADDTORECENTDOCS, uFlags, (LPARAM)pszCopy);
}

// see MINTASK.CPP for how the WM_HANDLESHELLNOTIFYICON and WM_HANDLESHADDTORECENTDOCS 
// messages are handled by the Taskbar thread.
 
 

