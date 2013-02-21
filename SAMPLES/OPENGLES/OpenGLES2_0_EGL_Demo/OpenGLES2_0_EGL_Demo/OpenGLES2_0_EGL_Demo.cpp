// OpenGLES2_0_EGL_Demo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "OpenGLES2_0_EGL_Demo.h"
#include <windows.h>
#include <commctrl.h>

#include <gles2/gl2.h>
#include <egl/egl.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE            g_hInst;            // current instance
#ifdef SHELL_AYGSHELL
HWND                g_hWndMenuBar;        // menu bar handle
#else // SHELL_AYGSHELL
HWND                g_hWndCommandBar;    // command bar handle
#endif // SHELL_AYGSHELL

EGLDisplay        display;
EGLSurface        surface;
EGLContext        context;
EGLConfig         config;

NativeDisplayType g_dpy;

HWND                gHWND;

extern void AniPolkaRender();
extern int AniPolkaDemo();
extern int AniPolkaDeinit();
extern int PostProcessFBODemo();
extern void PostProcessFBORender();
extern int PostProcessFBODeinit();
extern int TeapotElementsDemo();
extern void TeapotElementsRender();
extern int TeapotElementsDeinit();


typedef int (*pfnDemoInit)(void);
typedef void (*pfnDemoRender)(void);
typedef int (*pfnDemoDeinit)(void);

typedef struct _DemoFunction {
    pfnDemoInit        pfnInit;    
    pfnDemoRender        pfnRender;
    pfnDemoDeinit    pfnDeinit;
} DemoFunction;

DemoFunction    DemoList[3] = 
{
    {AniPolkaDemo, AniPolkaRender, AniPolkaDeinit},
    {PostProcessFBODemo, PostProcessFBORender, PostProcessFBODeinit},
    {TeapotElementsDemo, TeapotElementsRender, TeapotElementsDeinit}
};

int selectedDemo=0;

// Forward declarations of functions included in this code module:
ATOM            MyRegisterClass(HINSTANCE, LPTSTR);
BOOL            InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void            initializeEGL();
void            deinitializeEGL();
void            render();

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
    MSG msg;  

    DWORD dwFrames = 0;
    DWORD dwPrevTicks = GetTickCount();
    int stats_time = 0; // Assume no statistics

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) 
    {
        return FALSE;
    }

    initializeEGL();
    printf("EGL Init Done!!\n");

    HACCEL hAccelTable;
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OPENGLES2_0_EGL_DEMO));

    // Main message loop:
    /*
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }*/

    DemoList[selectedDemo].pfnInit();
    for (;;)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            render();
            dwFrames++;
        }
        const DWORD dwTicks = GetTickCount();
        if (stats_time > 0 && dwTicks - dwPrevTicks >= (DWORD)stats_time*1000)
        {
            TCHAR szMsg[16];
            wsprintf(szMsg, TEXT("%d FPS\n"), (1000 * dwFrames) / (dwTicks - dwPrevTicks));
            OutputDebugString(szMsg);
            dwPrevTicks = GetTickCount();
            dwFrames = 0;
        }
    }

    DemoList[selectedDemo].pfnDeinit();
    deinitializeEGL();

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
    WNDCLASS wc;

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OPENGLES2_0_EGL_DEMO));
    wc.hCursor       = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = 0;
    wc.lpszClassName = szWindowClass;

    return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];        // title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];    // main window class name

    g_hInst = hInstance; // Store instance handle in our global variable

#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
    // SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the device specific controls such as CAPEDIT and SIPPREF.
    SHInitExtraControls();
#endif // WIN32_PLATFORM_PSPC || WIN32_PLATFORM_WFSP

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_OPENGLES2_0_EGL_DEMO, szWindowClass, MAX_LOADSTRING);

#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
    //If it is already running, then focus on the window, and exit
    hWnd = FindWindow(szWindowClass, szTitle);    
    if (hWnd) 
    {
        // set focus to foremost child window
        // The "| 0x00000001" is used to bring any owned windows to the foreground and
        // activate them.
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        return 0;
    } 
#endif // WIN32_PLATFORM_PSPC || WIN32_PLATFORM_WFSP

    if (!MyRegisterClass(hInstance, szWindowClass))
    {
        return FALSE;
    }

    gHWND = hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    /*gHWND = hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
        300, 200, 400, 240, NULL, NULL, hInstance, NULL);*/

    if (!hWnd)
    {
        return FALSE;
    }

#ifdef WIN32_PLATFORM_PSPC
    // When the main window is created using CW_USEDEFAULT the height of the menubar (if one
    // is created is not taken into account). So we resize the window after creating it
    // if a menubar is present
    if (g_hWndMenuBar)
    {
        RECT rc;
        RECT rcMenuBar;

        GetWindowRect(hWnd, &rc);
        GetWindowRect(g_hWndMenuBar, &rcMenuBar);
        rc.bottom -= (rcMenuBar.bottom - rcMenuBar.top);
        
        MoveWindow(hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
    }
#endif // WIN32_PLATFORM_PSPC

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

#ifndef SHELL_AYGSHELL
    if (g_hWndCommandBar)
    {
        CommandBar_Show(g_hWndCommandBar, TRUE);
    }
#endif // !SHELL_AYGSHELL

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

#if defined(SHELL_AYGSHELL) && !defined(WIN32_PLATFORM_WFSP)
    static SHACTIVATEINFO s_sai;
#endif // SHELL_AYGSHELL && !WIN32_PLATFORM_WFSP
    
    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
                    break;
#ifndef SHELL_AYGSHELL
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;
#endif // !SHELL_AYGSHELL
#ifdef WIN32_PLATFORM_PSPC
                case IDM_OK:
                    SendMessage (hWnd, WM_CLOSE, 0, 0);                
                    break;
#endif // WIN32_PLATFORM_PSPC
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
#ifndef SHELL_AYGSHELL
            g_hWndCommandBar = CommandBar_Create(g_hInst, hWnd, 1);
            CommandBar_InsertMenubar(g_hWndCommandBar, g_hInst, IDR_MENU, 0);
            CommandBar_AddAdornments(g_hWndCommandBar, 0, 0);
#endif // !SHELL_AYGSHELL
#ifdef SHELL_AYGSHELL
            SHMENUBARINFO mbi;

            memset(&mbi, 0, sizeof(SHMENUBARINFO));
            mbi.cbSize     = sizeof(SHMENUBARINFO);
            mbi.hwndParent = hWnd;
            mbi.nToolBarId = IDR_MENU;
            mbi.hInstRes   = g_hInst;

            if (!SHCreateMenuBar(&mbi)) 
            {
                g_hWndMenuBar = NULL;
            }
            else
            {
                g_hWndMenuBar = mbi.hwndMB;
            }

            // Initialize the shell activate info structure
            memset(&s_sai, 0, sizeof (s_sai));
            s_sai.cbSize = sizeof (s_sai);
#endif // SHELL_AYGSHELL
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            
            // TODO: Add any drawing code here...
            
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
#ifndef SHELL_AYGSHELL
            CommandBar_Destroy(g_hWndCommandBar);
#endif // !SHELL_AYGSHELL
#ifdef SHELL_AYGSHELL
            CommandBar_Destroy(g_hWndMenuBar);
#endif // SHELL_AYGSHELL
            PostQuitMessage(0);
            break;

#if defined(SHELL_AYGSHELL) && !defined(WIN32_PLATFORM_WFSP)
        case WM_ACTIVATE:
            // Notify shell of our activate message
            SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
            break;
        case WM_SETTINGCHANGE:
            SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
            break;
#endif // SHELL_AYGSHELL && !WIN32_PLATFORM_WFSP

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
#ifndef SHELL_AYGSHELL
            RECT rectChild, rectParent;
            int DlgWidth, DlgHeight;    // dialog width and height in pixel units
            int NewPosX, NewPosY;

            // trying to center the About dialog
            if (GetWindowRect(hDlg, &rectChild)) 
            {
                GetClientRect(GetParent(hDlg), &rectParent);
                DlgWidth    = rectChild.right - rectChild.left;
                DlgHeight    = rectChild.bottom - rectChild.top ;
                NewPosX        = (rectParent.right - rectParent.left - DlgWidth) / 2;
                NewPosY        = (rectParent.bottom - rectParent.top - DlgHeight) / 2;
                
                // if the About box is larger than the physical screen 
                if (NewPosX < 0) NewPosX = 0;
                if (NewPosY < 0) NewPosY = 0;
                SetWindowPos(hDlg, 0, NewPosX, NewPosY,
                    0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
#endif // !SHELL_AYGSHELL
#ifdef SHELL_AYGSHELL
            {
                // Create a Done button and size it.  
                SHINITDLGINFO shidi;
                shidi.dwMask = SHIDIM_FLAGS;
                shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
                shidi.hDlg = hDlg;
                SHInitDialog(&shidi);
            }
#endif // SHELL_AYGSHELL

            return (INT_PTR)TRUE;

        case WM_COMMAND:
#ifndef SHELL_AYGSHELL
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
#endif // !SHELL_AYGSHELL
#ifdef SHELL_AYGSHELL
            if (LOWORD(wParam) == IDOK)
#endif
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return (INT_PTR)TRUE;

    }
    return (INT_PTR)FALSE;
}


void            initializeEGL()
{
    EGLint config_list[] = {
        EGL_RED_SIZE,       5,
        EGL_GREEN_SIZE,     6,
        EGL_BLUE_SIZE,      5,
#if defined(OPENGLES_TEST)        
        EGL_DEPTH_SIZE,     24,
#endif        
        EGL_NONE
    };

    EGLint num_config;

    
    g_dpy = GetDC(gHWND);

    display = eglGetDisplay(g_dpy);

    if (eglInitialize(display, NULL, NULL) == EGL_FALSE || eglGetError() != EGL_SUCCESS)
    {
        printf("eglInitialize Error!\r\n");
        return;
    }

    eglChooseConfig(display, config_list, &config, 1, &num_config);

    eglBindAPI(EGL_OPENGL_ES_API);

    surface = eglCreateWindowSurface( display, config, (NativeWindowType)(gHWND) , NULL);
    if ( surface == EGL_NO_SURFACE || eglGetError() != EGL_SUCCESS )
    {
        printf("eglCreateWindowSurface Error!\r\n");
        return ;
    }

    context = eglCreateContext( display, config, NULL,  NULL );
    if ( context == EGL_NO_CONTEXT || eglGetError() != EGL_SUCCESS )
    {
        printf("eglCreateContext Error!\r\n");
        return ;
    }

    if ( eglMakeCurrent( display, surface, surface, context ) == EGL_FALSE || eglGetError() != EGL_SUCCESS )
    {
        printf("eglMakeCurrent Error!\r\n");
        return;
    }

}


void    deinitializeEGL()
{
    eglMakeCurrent   (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    ReleaseDC(gHWND, (HDC)g_dpy);
}

void render()
{
    HDC         memdc;
    HBITMAP     bitmap;    
    RECT          rect;
    int width, height;
#if 0

    GetClientRect( gHWND, &rect );
    
    width  = rect.right - rect.left;
    height = rect.bottom - rect.top;

    bitmap = CreateCompatibleBitmap((HDC)display, width, height);
#endif 

    DemoList[selectedDemo].pfnRender();

//    glFlush ();

    eglSwapBuffers(display, surface);

#if 0

    glClearColor( 1.0f, 0.0f, 0.0f, 1.0f );        // Red
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    eglSwapBuffers(display, surface);
     
    glClearColor( 0.0f, 1.0f, 0.0f, 1.0f );        // Green
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    eglSwapBuffers(display, surface);

     glFinish();
     eglWaitGL();
     eglCopyBuffers( display, surface, bitmap ); // Get Red Buffer

     /*
    glClearColor( 0.0f, 1.0f, 0.0f, 1.0f );      // Green
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
     glFinish();
     eglWaitGL();
     eglCopyBuffers( display, surface, bitmap );  // Still Get Red Buffer, not Green Buffer     
     */




    //eglCopyBuffers(display, surface, bitmap);

    memdc  = CreateCompatibleDC( (HDC)display );
    SelectObject( memdc, bitmap );
    
    BitBlt( (HDC)display, 0, 0, width, height, memdc, 0, 0, SRCCOPY );
#endif
//    printf("[%d] demo eglSwapBuffers Done\n", selectedDemo);
    if (EGL_SUCCESS != eglGetError())
    {
        RETAILMSG(1, (TEXT("eglSwapBuffers error: %x\r\n"), eglGetError()));
    }

    DeleteDC( memdc );       
    DeleteObject( bitmap );
}