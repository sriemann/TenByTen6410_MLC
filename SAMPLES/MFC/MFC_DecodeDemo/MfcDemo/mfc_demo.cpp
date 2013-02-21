#include <windows.h>
#include "resource.h"


#define APPNAME                TEXT("S3C6410 MFC DECODE DEMO")
#define APPTITLE            TEXT("S3C6410 MFC DECODE DEMO")


extern BOOL mfc_render_init(HWND hWnd);
extern void mfc_render_final();


static HBITMAP  hBmpBkg=NULL;            // HBITMAP object for image on the main window
static HBITMAP  hBmpExitButton=NULL;    // HBITMAP object for exit button


#include "mfc_decode.h"
#include "mfc_render.h"


#define NUM_VECTORS_MAX            6
#define MEDIA_FILES_FOLDER        TEXT("\\Storage Card")


LPCTSTR pszFileNames0[]   = {L"\\My Documents\\shrek_640_480_768bit.m4v"};
CODEC_MODE codec_modes0[] = {CODEC_MPEG4};

LPCTSTR pszFileNames1[]   = {L"\\My Documents\\shrek3_320_240.m4v"};
CODEC_MODE codec_modes1[] = {CODEC_MPEG4};

LPCTSTR pszFileNames2[]   = {L"\\My Documents\\ultraviolet-640x480.264"};
CODEC_MODE codec_modes2[] = {CODEC_H264};

LPCTSTR pszFileNames3[]   = {L"\\My Documents\\harryp_320x240.264"};
CODEC_MODE codec_modes3[] = {CODEC_H264};

LPCTSTR pszFileNames4[]   = {L"\\My Documents\\veggie_tales_pirates_720x576.264"};
CODEC_MODE codec_modes4[] = {CODEC_H264};

LPCTSTR pszFileNames5[]   = {L"\\My Documents\\veggie_tales_pirates_320x240.264"};
CODEC_MODE codec_modes5[] = {CODEC_H264};


LPCTSTR *array_pszFileNames[NUM_VECTORS_MAX] = {
    pszFileNames0,
    pszFileNames1,
    pszFileNames2,
    pszFileNames3,
    pszFileNames4,
    pszFileNames5,
};

CODEC_MODE *array_codec_modes[NUM_VECTORS_MAX] = {
    codec_modes0,
    codec_modes1,
    codec_modes2,
    codec_modes3,
    codec_modes4,
    codec_modes5
};

/*
static void release_media_files_array()
{
    int              num_vectors;

    for (num_vectors=0; num_vectors < NUM_VECTORS_MAX; num_vectors++) {
        if (array_pszFileNames[num_vectors] == NULL)
            break;
    }
}
*/

static int find_media_files_fill_array(LPCTSTR folder_name)
{
    int              num_vectors;
    int              not_media_file;

    HANDLE           hFindFile;
    WIN32_FIND_DATA  find_data;

    TCHAR            filepath[128];
    TCHAR            file_ext[4];

    CODEC_MODE       codec_mode;

    wsprintf(filepath, L"%s\\*.*", folder_name);

    hFindFile = FindFirstFile(filepath, &find_data);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return 0;

#define FILE_EXT_M4V    L".m4v"
#define FILE_EXT_H263    L".263"
#define FILE_EXT_H264    L".264"
#define FILE_EXT_RCV    L".rcv"

    for (num_vectors=0; num_vectors < NUM_VECTORS_MAX; ) {
        if (find_data.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
            not_media_file = 0;

            if (wcslen(find_data.cFileName) > 4)
                wcsncpy(file_ext, find_data.cFileName + (wcslen(find_data.cFileName) - 4), 4);
            else
                file_ext[0] = 0;

            // Identify the codec type according to the filename extention
            if(_wcsnicmp(file_ext, FILE_EXT_M4V, 4) == 0)
                codec_mode = CODEC_MPEG4;
            else if(_wcsnicmp(file_ext, FILE_EXT_H263, 4) == 0)
                codec_mode = CODEC_H263;
            else if(_wcsnicmp(file_ext, FILE_EXT_H264, 4) == 0)
                codec_mode = CODEC_H264;
            else if(_wcsnicmp(file_ext, FILE_EXT_RCV, 4) == 0)
                codec_mode = CODEC_VC1;
            else
                not_media_file = 1;
#if 0
            // Identify the codec type according to the filename extention
            if(wcsstr(find_data.cFileName, FILE_EXT_M4V) != NULL)
                codec_mode = CODEC_MPEG4;
            else if(wcsstr(find_data.cFileName, FILE_EXT_H263) != NULL)
                codec_mode = CODEC_H263;
            else if(wcsstr(find_data.cFileName, FILE_EXT_H264) != NULL)
                codec_mode = CODEC_H264;
            else if(wcsstr(find_data.cFileName, FILE_EXT_RCV) != NULL)
                codec_mode = CODEC_VC1;
            else
                not_media_file = 1;
#endif

            if (not_media_file == 0) {
                wsprintf(filepath, L"%s\\%s", folder_name, find_data.cFileName);

                array_pszFileNames[num_vectors][0] = _wcsdup(filepath);
                array_codec_modes[num_vectors][0]  = codec_mode;

                num_vectors++;
            }
        }

        if (FindNextFile(hFindFile, &find_data) == FALSE)
            break;
    }

    FindClose(hFindFile);

    return num_vectors;
}



typedef struct
{
    HWND  hWnd;
    BOOL  fForceExit;

    int   demofile_idx;
} MFCDEC_DEMO_THREAD_PARAM;



static DWORD WINAPI mfc_demo_thread(LPVOID pPARAM)
{
    MFCDEC_DEMO_THREAD_PARAM    *p_mfc_demo_param;
    int  idx;

    if (pPARAM == NULL) {
        return 0x0FFFF;
    }

    p_mfc_demo_param = (MFCDEC_DEMO_THREAD_PARAM *) pPARAM;
    idx = p_mfc_demo_param->demofile_idx;

    RETAILMSG(1, (L"\n[MFC_DEMO] Playing \'%s\' file", array_pszFileNames[idx][0]));

#define NUM_PIP_VIDEOS        1
    mfcdec_demo(p_mfc_demo_param->hWnd,
                array_pszFileNames[idx],
                array_codec_modes[idx],
                NUM_PIP_VIDEOS,
                &(p_mfc_demo_param->fForceExit));

    return 0;
}


static void mfc_demo_thread_control(HWND hWnd, int demofile_idx)
{
    static MFCDEC_DEMO_THREAD_PARAM    mfc_demo_param;
    static HANDLE  hThread = NULL;

    DWORD  exit_code=0;


    if (hThread != NULL) {

        // Check if the thread exited already.
        GetExitCodeThread(hThread, &exit_code);
        if (exit_code == STILL_ACTIVE) {    // Thread is running -> Terminate thread.
            mfc_demo_param.fForceExit = TRUE;
            WaitForSingleObject(hThread, 2000);
            hThread = NULL;
            return;
        }
    }

    if ((demofile_idx >= 0) && (demofile_idx < NUM_VECTORS_MAX)) {
        // Start demo thread
        mfc_demo_param.hWnd          = hWnd;
        mfc_demo_param.fForceExit    = FALSE;
        mfc_demo_param.demofile_idx  = demofile_idx;

        hThread = CreateThread(NULL, 0,
                               mfc_demo_thread,
                               &mfc_demo_param,
                               0,
                               NULL);
    }
}



#define DRAW_TEXT_X_POS        100
#define DRAW_TEXT_Y_POS        150
#define DRAW_TEXT_X_LENG    280
#define DRAW_TEXT_Y_LENG    30

// return value = -1  : not selected
static int GetDemofileIndex(DWORD xPos, DWORD yPos)
{
    DWORD i;


    if ((xPos >= DRAW_TEXT_X_POS) && (xPos <= (DRAW_TEXT_X_POS + DRAW_TEXT_X_LENG))) {

        if (yPos < DRAW_TEXT_Y_POS)
            return -1;

        for (i=1; i<=NUM_VECTORS_MAX; i++) {
            if (  yPos < (DRAW_TEXT_Y_POS + (DRAW_TEXT_Y_LENG * i))  )
                return (i-1);
        }
    }

    return -1;
}


//-----------------------------------------------------------------------------
// Name: WindowProc()
// Desc: The Main Window Procedure
//-----------------------------------------------------------------------------
long FAR PASCAL
WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC         hdc;
    DWORD       xPos, yPos;
    RECT        draw_rect;
    POINT       pt;

    int         i;
    int         demofile_idx;

    static int         num_vectors = 0;
    static HDC         hdc_mem_bkg, hdc_mem_exit_btn;
    static BITMAP      bm_bkg, bm_exit_btn;

    TCHAR       msg_filename_to_play[128];

    switch (message)
    {
        case WM_CREATE:
            // Rendering Init
            mfc_render_init(hWnd);
            // Check the number of media files in specific folder
            // and fill the filename list array.
            num_vectors = find_media_files_fill_array(MEDIA_FILES_FOLDER);


            hdc_mem_bkg = CreateCompatibleDC(NULL);
            GetObject(hBmpBkg, sizeof(BITMAP), &bm_bkg);
            SelectObject(hdc_mem_bkg, (HGDIOBJ)hBmpBkg);

            hdc_mem_exit_btn = CreateCompatibleDC(NULL);
            GetObject(hBmpExitButton, sizeof(BITMAP), &bm_exit_btn);
            SelectObject(hdc_mem_exit_btn, (HGDIOBJ)hBmpExitButton);
            break;

        case WM_DESTROY:
            // Clean up and close the app
//            SelectObject(hdc_mem, (HGDIOBJ)hBitmapOld);
            DeleteDC(hdc_mem_bkg);
            if (hBmpBkg)
                DeleteObject(hBmpBkg);
            DeleteDC(hdc_mem_exit_btn);
            if (hBmpExitButton)
                DeleteObject(hBmpExitButton);
            mfc_render_final();
            PostQuitMessage(0);
            return 0L;

        case WM_LBUTTONDOWN:
            xPos = LOWORD(lParam); 
            yPos = HIWORD(lParam);
            pt.x = xPos; pt.y = yPos;

            // exit 버튼을 누르면 WM_DESTROY 메시지를 날려서 종료시킨다.
            draw_rect.left    = 30+bm_bkg.bmWidth;
            draw_rect.top     = 25;
            draw_rect.right   = draw_rect.left + bm_exit_btn.bmWidth;
            draw_rect.bottom  = draw_rect.top  + bm_exit_btn.bmHeight;
            if (PtInRect(&draw_rect, pt) == TRUE) {
                SendMessage(hWnd, WM_DESTROY, 0, 0);
                break;
            }

            demofile_idx = GetDemofileIndex(xPos, yPos);
            mfc_demo_thread_control(hWnd, demofile_idx);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);

            // Background bitmap image
            BitBlt(hdc, 20, 20, bm_bkg.bmWidth, bm_bkg.bmHeight, hdc_mem_bkg, 0, 0, SRCCOPY);
            BitBlt(hdc, 30+bm_bkg.bmWidth, 25, bm_exit_btn.bmWidth, bm_exit_btn.bmHeight, hdc_mem_exit_btn, 0, 0, SRCCOPY);

            // Rectangle for the "DrawText" function below.
            draw_rect.left    = DRAW_TEXT_X_POS;
            draw_rect.top     = DRAW_TEXT_Y_POS - 60;
            draw_rect.right   = draw_rect.left + DRAW_TEXT_X_LENG;
            draw_rect.bottom  = draw_rect.top  + DRAW_TEXT_Y_LENG;


            wsprintf(msg_filename_to_play, L"Press the file name to play.\n(In %s folder)", MEDIA_FILES_FOLDER);
            FillRect(hdc, &draw_rect, (HBRUSH) GetStockObject(WHITE_BRUSH));
            DrawText(hdc, msg_filename_to_play, -1, &draw_rect, DT_CENTER);

            // Move the rectangle for the media filenames list
            draw_rect.left    = DRAW_TEXT_X_POS;
            draw_rect.top     = DRAW_TEXT_Y_POS;
            draw_rect.right   = draw_rect.left + DRAW_TEXT_X_LENG;
            draw_rect.bottom  = draw_rect.top  + DRAW_TEXT_Y_LENG;
            for (i=0; i<num_vectors; i++) {
                if (GetFileAttributes(*(array_pszFileNames[i])) != 0xFFFFFFFF)
                    DrawText(hdc, *(array_pszFileNames[i]), -1, &draw_rect, DT_LEFT);
                draw_rect.top     += DRAW_TEXT_Y_LENG;
                draw_rect.bottom  += DRAW_TEXT_Y_LENG;
            }

            EndPaint(hWnd, &ps);
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


    // Load the bitmap image which will be on the main window.
    hBmpBkg = (HBITMAP) LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDB_BMP_SAMSUNG_DEMO),
                                  IMAGE_BITMAP,
                                  0, 0,
                                  0);
/*
    hBmpExitButton  = (HBITMAP) LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDB_BMP_EXIT_BUTTON),
                                  IMAGE_BITMAP,
                                  0, 0,
                                  0);
*/
    hBmpExitButton = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BMP_EXIT_BUTTON));


    hWnd = InitApp(hInstance, nCmdShow);
    if (hWnd == NULL)
        return FALSE;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

