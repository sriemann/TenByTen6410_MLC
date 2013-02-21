/*********************************************************************************
*                                                                               *
* Copyright (c) 2008 Samsung System LSI                                            *
* All rights reserved.                                                          *
*                                                                               *
* This software is test sample code for Hybrid Divx Decoder                        *
*                                                                                *
* Author : Jiyoung Shin                                                            *
* Last Changed : 2008.06.10                                                     *
*********************************************************************************/

#include <windows.h>
#include <commctrl.h>
#include "DivxTest.h"
#include "UAOAPI.h"
#include "SsbSipRender.h"
#include "SsbSipHybridDivxMain.h"
//Included for Unaligned Memory Access
//#include "pkfuncs.h"

#define PERF_RENDER


#define APPNAME                    TEXT("S3C6400 Divx DEMO")
#define APPTITLE                    TEXT("S3C6400 Divx DEMO")
#define IDI_MAIN_ICON                   110

BOOL        isActive = FALSE;
void            *UAOHandle;
//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

long FAR PASCAL
WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC         hdc;
    RECT        draw_rect;
    DWORD       xPos, yPos;

    int         i;
    int         demofile_idx;
    static HANDLE  hThread = NULL;
    int            ret;


    switch (message)
    {
        case WM_CREATE:

            break;

        case WM_DESTROY:
            // Clean up and close the app
            CloseHandle(hThread);
            SsbSipDisplayDeInit();
            ret = DeviceIoControl(UAOHandle, IOCTL_UAO_DISABLE, 
                        NULL,0, NULL, 0, NULL, NULL);
            if(ret == 0){
                RETAILMSG(1, (TEXT("IOCTL_UAO_DISABLE failed\r\n")));
                return 0;
            }
            CloseHandle(UAOHandle);
            PostQuitMessage(0);
            return 0L;

        case WM_LBUTTONDOWN:

            if(isActive){
//                printf("on Playing...........stop!!\n");
                //SsbSipHybridDivxResourceRelease();
                SsbSipDisplayDeInit();
                CloseHandle(hThread);
                ret = DeviceIoControl(UAOHandle, IOCTL_UAO_DISABLE, 
                                    NULL,0, NULL, 0, NULL, NULL);
                if(ret == 0){
                    RETAILMSG(1, (TEXT("IOCTL_UAO_DISABLE failed\r\n")));
                    return 0;
                }
                CloseHandle(UAOHandle);
                PostQuitMessage(0);
                return 0L;
            }
            isActive = TRUE;
            
            //Enabling Unaligned Mempro Access
            //KLibUnalignedAccessEnable(TRUE);
            UAOHandle = CreateFile(UAO_DRIVER_NAME,
                            GENERIC_READ|GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,NULL);

            if(UAOHandle == INVALID_HANDLE_VALUE){
                RETAILMSG(1, (TEXT("CreateFile(UAO_DRIVER) failed\r\n")));
                return 0;
            }

            ret = DeviceIoControl(UAOHandle, IOCTL_UAO_ENABLE, 
                                NULL,0, NULL, 0, NULL, NULL);
            if(ret == 0){
                RETAILMSG(1, (TEXT("IOCTL_UAO_ENABLE failed\r\n")));
                return 0;
            }

            hThread = CreateThread(NULL, 0,
                               (LPTHREAD_START_ROUTINE)SsbSipHybridDivxMain,
                               0,
                               0,
                               NULL);
            break;

        case WM_PAINT:
            break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Name: InitApp()
// Desc: Do work required for every instance of the application:
//          Create the window, initialize data
//-----------------------------------------------------------------------------
static HWND
InitApp(HINSTANCE hInstance, int nCmdShow)
{
    HWND                        hWnd;
    WNDCLASS                    wc;


    // Set up and register window class
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH )GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = APPNAME;
    RegisterClass(&wc);

    // Create a window
    hWnd = CreateWindow(APPNAME,
                        APPTITLE,
                        WS_POPUP|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME,
                        0,
                        0,
                        GetSystemMetrics(SM_CXSCREEN),
                        GetSystemMetrics(SM_CYSCREEN),
                        NULL,
                        NULL,
                        hInstance,
                        NULL);
    if (!hWnd)
        return NULL;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    SetFocus(hWnd);

    return hWnd;
}


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Initialization, message loop
//-----------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{

    HWND     hWnd;
    MSG      msg;

    hWnd = InitApp(hInstance, nCmdShow);
    if (hWnd == NULL)
        return FALSE;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

