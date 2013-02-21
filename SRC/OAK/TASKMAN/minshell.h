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
 *  file:    MinShell.h
 *  purpose: sample shell common header
 *
\*---------------------------------------------------------------------------*/

// Debug zones
  #define ZONE_TRACE		DEBUGZONE(0)
  #define ZONE_TRACE2		DEBUGZONE(1)
  #define ZONE_TRACEMSG		DEBUGZONE(2)
  #define ZONE_TRACEMSGHF	DEBUGZONE(3)
  #define ZONE_TRACETASKMAN	DEBUGZONE(4)
  #define ZONE_TRACEDESKT   DEBUGZONE(5)
  #define ZONE_WARN			DEBUGZONE(14)
  #define ZONE_ERROR		DEBUGZONE(15)

// extern const defns
extern TCHAR const c_szDesktopWndClass[];

// extern global Data defns
extern HINSTANCE g_hInst;
extern HWND g_hwndDesktop, g_hwndTaskBar, g_hwndTaskMan;
extern HWND g_hwndMBVL, g_hwndBBL, g_hwndBBVL;
extern int  g_iSignalStartedID;

// extern Function defns
extern BOOL Desktop_Register(HINSTANCE hInst);
extern HRESULT Desktop_InitInstance(int nCmdShow);
extern TCHAR const c_szDesktopWndClass[];
extern HWND  g_hwndDesktop;
extern BOOL TaskBar_ThreadProc(int nCmdShow);
extern BOOL TaskMan_Create();
extern void TaskMan_Destroy();
extern void Show_TaskMan(void);
extern void DoPowerCheck(HWND);
extern void DoHibernate(void);
extern void WINAPI PathRemoveBlanks(LPTSTR lpszString);
extern LPTSTR WINAPI PathGetArgs(LPCTSTR pszPath);
extern void WINAPI PathRemoveArgs(LPTSTR pszPath);
extern BOOL DoBrowse(HWND hwndParent, LPTSTR pszBuf, int iBufLen);
extern BOOL DoExec(HWND hwndParent, LPCTSTR lpsz, LPCTSTR lpszArgs);
extern BOOL MinServer_Init(void);

BOOL WINAPI CenterWindowSIPAware(HWND hwnd, BOOL fInitial);

// private messages
#define WM_HANDLESHELLNOTIFYICON	(WM_USER + 0xBAD)
#define WM_HANDLESHADDTORECENTDOCS	(WM_USER + 0xBAE)

// Useful macros
#ifdef DEBUG
	#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save = (HCURSOR)0xDEADBEEF
	#define SetWaitCursor()   { DEBUGCHK(hcursor_wait_cursor_save == (HCURSOR)0xDEADBEEF); hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT)); }
	#define ResetWaitCursor() { DEBUGCHK(hcursor_wait_cursor_save != (HCURSOR)0xDEADBEEF); SetCursor(hcursor_wait_cursor_save); hcursor_wait_cursor_save = (HCURSOR)0xDEADBEEF; }
#else
	#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save = NULL
	#define SetWaitCursor()   { hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT)); }
	#define ResetWaitCursor() { SetCursor(hcursor_wait_cursor_save);  hcursor_wait_cursor_save = NULL; }
#endif

/////////////////////////////////////////////////////////////////////////////
// Misc string & memory handling helpers
/////////////////////////////////////////////////////////////////////////////

#define MyAlloc(typ)		((typ*)LocalAlloc(LPTR, sizeof(typ)))
#define MyRgAlloc(typ, n)	((typ*)LocalAlloc(LPTR, (n)*sizeof(typ)))
#define MyRgReAlloc(typ, p, n)	((typ*)LocalReAlloc(p, (n)*sizeof(typ), LMEM_MOVEABLE | LMEM_ZEROINIT))
#define MySzAlloc(n)		((LPTSTR)LocalAlloc(LPTR, (1+(n))*sizeof(TCHAR)))
#define MyFree(p)			{ if(p) LocalFree(p); }

inline LPTSTR MySzDup(LPCTSTR pszIn) 
{ 
	if(!pszIn) return NULL;
	LPTSTR pszOut=MySzAlloc(lstrlen(pszIn)); 
	if(pszOut) lstrcpy(pszOut, pszIn); 
	return pszOut; 
}


