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
//  AudioE2E TUX DLL
//
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "main.h"
#include "globals.h"
#include "wavefile.h"

unsigned char fail_log = 0;

#define MRCHECK(r,str,label)\
    if ((r != MMSYSERR_NOERROR)) { g_pKato->Log(LOG_FAIL, TEXT(#str) TEXT(" failed. mr=%08x\r\n"), r); mr = r;fail_log++;; goto label;}

#define AUDIO_SD_PLAY_PATH       "\\Storage Card\\E2EAudioPlay.wav"
#define AUDIO_SD_REC_PATH        "\\Storage Card\\E2EAudioRec.wav"



MMRESULT
RecordWaveBuffer (PWAVEHDR pwh, DWORD dwDeviceId, PWAVEFORMATEX pwfx, DWORD dwDuration)
{ 
  const DWORD dwTolerance = 1000;
  DWORD dwWait;
  HANDLE hevDone;
  HWAVEIN hwi;

    // create an event so we know when capture is completed
    hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hevDone == NULL) {
        RETAILMSG (1, (TEXT("Unable to create completion event\r\n")));
        return MMSYSERR_NOMEM;
    }

    // open the wave capture device
    MMRESULT mr = waveInOpen(&hwi, dwDeviceId, pwfx, (DWORD) hevDone, NULL, CALLBACK_EVENT);
    if (mr != MMSYSERR_NOERROR) {
        RETAILMSG(1, (TEXT("waveInOpen failed. mr=%08x\r\n"), mr));
        return mr;
    }

    // set up the WAVEHDR structure that describes the capture buffer

    // prepare the buffer for capture
    mr = waveInPrepareHeader(hwi, pwh, sizeof(WAVEHDR));
    MRCHECK(mr, waveInPrepareHeader,ERROR_DONE);

    // submit the buffer for capture
    mr = waveInAddBuffer(hwi, pwh, sizeof(WAVEHDR));
    MRCHECK(mr, waveInAddBuffer,ERROR_DONE);

    // start capturing
    RETAILMSG(1, (TEXT("Starting capture...\r\n")));
    mr = waveInStart(hwi);
    MRCHECK(mr, waveInStart,ERROR_DONE);

    // wait for completion + 1 second tolerance
    dwWait= WaitForSingleObject(hevDone, dwDuration + dwTolerance);
    if (dwWait != WAIT_OBJECT_0) {
        RETAILMSG(1, (TEXT("Timeout waiting for capture to complete, writing partialy file.\r\n")));
        mr = waveInReset(hwi);
        if (mr != MMSYSERR_NOERROR) {
            RETAILMSG(1, (TEXT("warning: waveInReset failed. mr=%08x\r\n"), mr));
        }
    }

    // now clean up: unprepare the buffer
    mr = waveInUnprepareHeader(hwi, pwh, sizeof(WAVEHDR));
    MRCHECK(mr, waveInUnprepareHeader,ERROR_DONE);

ERROR_DONE:
    // close the capture device & free the event handle
    mr = waveInClose(hwi);
    if (mr != MMSYSERR_NOERROR) {
        RETAILMSG(1, (TEXT("warning: waveInClose failed. mr=%08x\r\n"), mr));
    }

    CloseHandle(hevDone);

    return mr;
}


BOOL  ThreadProc_AudioPlayThread(LPVOID lpParameter)
{

	MMRESULT mr;
    DWORD dwBufferSize;
    PBYTE pBufferBits = NULL;
    PWAVEFORMATEX pwfx = NULL;
    DWORD dwSlop;
    DWORD dwWait;
    DWORD dwDuration;
	
    HANDLE hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hevDone == NULL) {
		g_pKato->Log(LOG_COMMENT, TEXT("CreateEvent Error."));
		goto ERROR_READ;
    }

    mr = ReadWaveFile(TEXT(AUDIO_SD_PLAY_PATH),&pwfx,&dwBufferSize,&pBufferBits);
    MRCHECK(mr, ReadWaveFile, ERROR_READ);

    // Note: Cast to UINT64 below is to avoid potential DWORD overflow for large (>~4MB) files.
    dwDuration = (DWORD)(((UINT64)dwBufferSize) * 1000 / pwfx->nAvgBytesPerSec);

    RETAILMSG(1, (TEXT("\"%s\" %c%02d %5dHz %6d bytes %5d ms\r\n")
        , TEXT(AUDIO_SD_PLAY_PATH)
        , pwfx->nChannels == 2 ? L'S' : L'M'
        , pwfx->wBitsPerSample
        , pwfx->nSamplesPerSec
        , dwBufferSize
        , dwDuration
        ));

    HWAVEOUT hwo;
    mr = waveOutOpen(&hwo, WAVE_MAPPER, pwfx, (DWORD) hevDone, NULL, CALLBACK_EVENT);
    MRCHECK(mr, waveOutOpen, ERROR_OPEN);

    WAVEHDR hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.dwBufferLength = dwBufferSize;
    hdr.lpData = (char *) pBufferBits;

    mr = waveOutPrepareHeader(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutPrepareHeader, ERROR_PLAY);

    mr = waveOutWrite(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutWrite, ERROR_PLAY);

    // wait for play + 1 second slop
    dwSlop = 1000;
    dwWait = WaitForSingleObject(hevDone, dwDuration + dwSlop);
    if (dwWait != WAIT_OBJECT_0) {
        // not much to here, other than issue a warning        
		g_pKato->Log(LOG_COMMENT, TEXT("Timeout waiting for playback to complete."));
    }

    mr = waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutUnprepareHeader, ERROR_PLAY);
	

ERROR_PLAY:
    mr = waveOutClose(hwo);
    MRCHECK(mr, waveOutClose, ERROR_OPEN);

ERROR_OPEN:
    delete [] pBufferBits;
    delete [] pwfx;

ERROR_READ:
    if(hevDone !=NULL)
	{
      CloseHandle(hevDone);
	}    
   
    return 0;

}

////////////////////////////////////////////////////////////////////////////////
// AudioPlay_E2E
//    Audio playing and file system (SD) E2E test.
//
// Parameters:
//    uMsg            Message code.
//    tpParam         Additional message-dependent data.
//    lpFTE           Function table entry that generated this call.
//
// Return value:
//    TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//    special conditions.

TESTPROCAPI AudioPlay_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	MMRESULT mr;
    DWORD dwBufferSize;
    PBYTE pBufferBits = NULL;
    PWAVEFORMATEX pwfx = NULL;
    DWORD dwSlop;
    DWORD dwWait;
    DWORD dwDuration;
	int user_ret;
	HWND hWnd1;
	FILE *fp;
	

    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    
    fail_log = 0;
	hWnd1 = GetForegroundWindow();
	MessageBox(hWnd1,TEXT("Please plug in headphone or speaker to your device. And insert SD card with test wav file. "),TEXT("AudioPlay_E2E!"),MB_OK);
    
	fp = fopen(AUDIO_SD_PLAY_PATH, "rb");
	if(fp == NULL){
		MessageBox(hWnd1,TEXT("File not found! Please insert SD card with test wav file."),TEXT("MFC E2E test"),MB_OK);
	}
	else
	{
        fclose(fp);
	}

    HANDLE hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hevDone == NULL) {
        return MMSYSERR_NOMEM;
    }

    mr = ReadWaveFile(TEXT(AUDIO_SD_PLAY_PATH),&pwfx,&dwBufferSize,&pBufferBits);
    MRCHECK(mr, ReadWaveFile, ERROR_READ);

    // Note: Cast to UINT64 below is to avoid potential DWORD overflow for large (>~4MB) files.
    dwDuration = (DWORD)(((UINT64)dwBufferSize) * 1000 / pwfx->nAvgBytesPerSec);

    RETAILMSG(1, (TEXT("\"%s\" %c%02d %5dHz %6d bytes %5d ms\r\n")
        , TEXT(AUDIO_SD_PLAY_PATH)
        , pwfx->nChannels == 2 ? L'S' : L'M'
        , pwfx->wBitsPerSample
        , pwfx->nSamplesPerSec
        , dwBufferSize
        , dwDuration
        ));

    HWAVEOUT hwo;
    mr = waveOutOpen(&hwo, WAVE_MAPPER, pwfx, (DWORD) hevDone, NULL, CALLBACK_EVENT);
    MRCHECK(mr, waveOutOpen, ERROR_OPEN);

    WAVEHDR hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.dwBufferLength = dwBufferSize;
    hdr.lpData = (char *) pBufferBits;

    mr = waveOutPrepareHeader(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutPrepareHeader, ERROR_PLAY);

    mr = waveOutWrite(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutWrite, ERROR_PLAY);

    // wait for play + 1 second slop
    dwSlop = 1000;
    dwWait = WaitForSingleObject(hevDone, dwDuration + dwSlop);
    if (dwWait != WAIT_OBJECT_0) {
        // not much to here, other than issue a warning        
		g_pKato->Log(LOG_COMMENT, TEXT("Timeout waiting for playback to complete."));
    }

    mr = waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutUnprepareHeader, ERROR_PLAY);
	user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you hear the sound playing correctly?", 
					(LPCWSTR)L"AudioPlay_E2E", MB_YESNO);		
	if( user_ret != IDYES ){
		g_pKato->Log(LOG_COMMENT, TEXT("User reported the sound playing failed!."));
		fail_log ++;
	}

ERROR_PLAY:
    mr = waveOutClose(hwo);
    MRCHECK(mr, waveOutClose, ERROR_OPEN);

ERROR_OPEN:
    delete [] pBufferBits;
    delete [] pwfx;

ERROR_READ:
    if(hevDone !=NULL)
	{
      CloseHandle(hevDone);
	}
    if( hWnd1 != NULL  ) 
	{
		CloseHandle(hWnd1);
	}
   
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);	
}


////////////////////////////////////////////////////////////////////////////////
// AudioRec_E2E
//    Audio recording and file system (SD) E2E test.
//
// Parameters:
//    uMsg            Message code.
//    tpParam         Additional message-dependent data.
//    lpFTE           Function table entry that generated this call.
//
// Return value:
//    TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//    special conditions.

TESTPROCAPI AudioRec_E2E(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	MMRESULT mr;
    PBYTE pBufferBits;
    DWORD dwBufferSize;
	DWORD dwDuration = 5 * 1000;    // record for 5 seconds
    DWORD dwChannels = 2;           // default to mono
    DWORD dwBitsPerSample = 16;     // default to 16-bit samples
    DWORD dwSampleRate = 11025;     // default to 11.025KHz sample rate
    DWORD dwDeviceId = WAVE_MAPPER;           // capture from any available device
    WAVEFORMATEX wfx;
    PWAVEFORMATEX pwfx;
	DWORD dwSlop;
    DWORD dwWait;  
	int user_ret;
	HWND hWnd1;
	HANDLE hThread;
	DWORD IDThreadKey;

    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

	pwfx=(PWAVEFORMATEX)&wfx;
    pwfx->cbSize = 0;
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->wBitsPerSample = (WORD) dwBitsPerSample;
    pwfx->nSamplesPerSec = dwSampleRate;
    pwfx->nChannels = (WORD) dwChannels;
    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;


	 fail_log = 0;
    // compute the size of the capture buffer
    dwBufferSize = (DWORD)((((UINT64)dwDuration) * ((UINT64)pwfx->nAvgBytesPerSec) / 1000));

    // Pad out to a multiple of nBlockAlign
    dwBufferSize += dwBufferSize % pwfx->nBlockAlign;

	hWnd1 = GetForegroundWindow();
	MessageBox(hWnd1,TEXT("Please plug in loop back audio cable to [Headphone jack] and [Line in jack] then press ok button to record. "),TEXT("AudioRec_E2E"),MB_OK);

    // let user know what's going on
    RETAILMSG(1, (TEXT("Recording %5d ms to \"%s\": %c%02d %5dHz (%8d bytes)\r\n")
        , dwDuration
        , TEXT(AUDIO_SD_REC_PATH)
        , pwfx->nChannels == 2 ? L'S' : L'M'
        , pwfx->wBitsPerSample
        , pwfx->nSamplesPerSec
        , dwBufferSize
        ));


	hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_AudioPlayThread, NULL, 0,&IDThreadKey);
    // try to allocate the capture buffer
    pBufferBits = new BYTE [dwBufferSize];
    if (pBufferBits == NULL) {
        RETAILMSG (1, (TEXT("Unable to allocate %d bytes for %d ms of audio data\r\n"), dwBufferSize, dwDuration));
        return MMSYSERR_NOMEM;
    }
    WAVEHDR hdr;
    memset(&hdr, 0, sizeof(WAVEHDR));
    hdr.dwBufferLength = dwBufferSize;
    hdr.lpData = (char *) pBufferBits;

    mr = RecordWaveBuffer(&hdr, dwDeviceId, pwfx, dwDuration);
    MRCHECK(mr, RecordWaveBuffer,ERROR_OPEN);

    // finally, write the captured buffer to the file
    // note that we use hdr.dwBytesRecorded, not dwBuffersize.
    RETAILMSG(1, (TEXT("Capture completed. Writing %s\r\n"), TEXT(AUDIO_SD_REC_PATH)));
    mr = WriteWaveFile(TEXT(AUDIO_SD_REC_PATH), pwfx, hdr.dwBytesRecorded, pBufferBits);
    MRCHECK(mr, WriteWaveFile,ERROR_OPEN);


    // Play
	pBufferBits = NULL;
    pwfx = NULL;

	
	MessageBox(hWnd1,TEXT("Please remove loop back audio cable and plug in headphone or speaker to your device. "),TEXT("AudioRec_E2E"),MB_OK);

    HANDLE hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hevDone == NULL) {
        return MMSYSERR_NOMEM;
    }

    mr = ReadWaveFile(TEXT(AUDIO_SD_REC_PATH),&pwfx,&dwBufferSize,&pBufferBits);
    MRCHECK(mr, ReadWaveFile, ERROR_READ);

    // Note: Cast to UINT64 below is to avoid potential DWORD overflow for large (>~4MB) files.
    dwDuration = (DWORD)(((UINT64)dwBufferSize) * 1000 / pwfx->nAvgBytesPerSec);

    RETAILMSG(1, (TEXT("\"%s\" %c%02d %5dHz %6d bytes %5d ms\r\n")
        , TEXT(AUDIO_SD_REC_PATH)
        , pwfx->nChannels == 2 ? L'S' : L'M'
        , pwfx->wBitsPerSample
        , pwfx->nSamplesPerSec
        , dwBufferSize
        , dwDuration
        ));

    HWAVEOUT hwo;
    mr = waveOutOpen(&hwo, WAVE_MAPPER, pwfx, (DWORD) hevDone, NULL, CALLBACK_EVENT);
    MRCHECK(mr, waveOutOpen, ERROR_OPEN);

    
    memset(&hdr, 0, sizeof(hdr));
    hdr.dwBufferLength = dwBufferSize;
    hdr.lpData = (char *) pBufferBits;

    mr = waveOutPrepareHeader(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutPrepareHeader, ERROR_PLAY);

    mr = waveOutWrite(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutWrite, ERROR_PLAY);

    // wait for play + 1 second slop
    dwSlop = 1000;
    dwWait = WaitForSingleObject(hevDone, dwDuration + dwSlop);
    if (dwWait != WAIT_OBJECT_0) {
        // not much to here, other than issue a warning        
		g_pKato->Log(LOG_COMMENT, TEXT("Timeout waiting for playback to complete."));
    }

    mr = waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr));
    MRCHECK(mr, waveOutUnprepareHeader, ERROR_PLAY);
	user_ret = MessageBox(hWnd1, (LPCWSTR)L"Do you hear the sound playing correctly?", 
					(LPCWSTR)L"AudioRec_E2E", MB_YESNO);		
	if( user_ret != IDYES ){
		g_pKato->Log(LOG_COMMENT, TEXT("User reported the sound playing failed!."));
		fail_log ++;
	}

ERROR_PLAY:
    mr = waveOutClose(hwo);
    MRCHECK(mr, waveOutClose, ERROR_OPEN);

ERROR_OPEN:
    delete [] pBufferBits;
    delete [] pwfx;

ERROR_READ:
	if(hevDone !=NULL)
	{
      CloseHandle(hevDone);
	}
    if(hThread !=NULL)
	{
		CloseHandle(hThread);
	}
    if( hWnd1 != NULL  ) 
	{
		CloseHandle(hWnd1);
	}
    return ((fail_log == 0)? TPR_PASS:TPR_FAIL);	
	
}




////////////////////////////////////////////////////////////////////////////////
