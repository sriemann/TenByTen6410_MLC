#ifndef __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_DECODE_H__
#define __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_DECODE_H__


// Display config
#define DECODED_OUT_NOTHING        (0)
#define DECODED_OUT_FILE        (1)
#define DECODED_OUT_DISP        (2)
#define DECODED_OUTPUT    (DECODED_OUT_DISP)

#if (DECODED_OUTPUT == DECODED_OUT_FILE)
#define OUT_FILE_NAME    "\\Temp\\output.yuv"
#endif


typedef enum {
    CODEC_MPEG4,
    CODEC_H263,
    CODEC_H264,
    CODEC_VC1
} CODEC_MODE;


#include <windows.h>    // Because of HWND

int mfcdec_demo(HWND hWnd, LPCTSTR pszFileName[], CODEC_MODE codec_mode[], int num_pips, int *force_exit);


#endif /* __SAMSUNG_SYSLSI_APDEV_SAMPLE_MFC_DECODE_H__ */
