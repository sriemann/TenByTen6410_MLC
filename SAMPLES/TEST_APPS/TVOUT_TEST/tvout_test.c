// testapp.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <nkintr.h>
#include <stdio.h>

// defined in priv_context.h in display driver of SMDK6410
#define DRVESC_OUTPUT_BASE              (0x00020100)
#define DRVESC_OUTPUT_RGB               (DRVESC_OUTPUT_BASE+0)
#define DRVESC_OUTPUT_TV                (DRVESC_OUTPUT_BASE+1)
#define DRVESC_OUTPUT_SWITCH            (DRVESC_OUTPUT_BASE+2)
#define DRVESC_TV_DMA_DISABLE           (DRVESC_OUTPUT_BASE+10)
#define DRVESC_TV_DMA_PRIMARY           (DRVESC_OUTPUT_BASE+11)
#define DRVESC_TV_DMA_OVERLAY           (DRVESC_OUTPUT_BASE+12)


int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{
	HDC hdc = NULL;

	// Get the Null DC = Primary Surface's window DC
	hdc = GetDC(NULL);

	if (hdc != NULL)
	{
		// Call the Display Driver Escape
		// The user can call above 6 escape code.
		ExtEscape(hdc, DRVESC_OUTPUT_SWITCH, 0, NULL , 0, NULL);
		RETAILMSG(1,(_T("[INF] Display output Switched\r\n")));

		ReleaseDC(NULL, hdc);
	}
	else
	{
		RETAILMSG(1,(_T("[ERR] hdc is NULL\r\n")));
	}

	return 0;
}

