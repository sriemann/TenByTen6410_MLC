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
// wavefile.cpp

#include <windows.h>
#include "wavefile.h"

// -----------------------------------------------------------------------------
//                              FileHeader
// -----------------------------------------------------------------------------
typedef struct
{
   DWORD   dwRiff;     // Type of file header.
   DWORD   dwSize;     // Size of file header.
   DWORD   dwWave;     // Type of wave.
} RIFF_FILEHEADER, *PRIFF_FILEHEADER;


// -----------------------------------------------------------------------------
//                              ChunkHeader
// -----------------------------------------------------------------------------
typedef struct
{
   DWORD   dwCKID;        // Type Identification for current chunk header.
   DWORD   dwSize;        // Size of current chunk header.
} RIFF_CHUNKHEADER, *PRIFF_CHUNKHEADER;

/*  Chunk Types  
*/
#define RIFF_FILE       mmioFOURCC('R','I','F','F')
#define RIFF_WAVE       mmioFOURCC('W','A','V','E')
#define RIFF_FORMAT     mmioFOURCC('f','m','t',' ')
#define RIFF_CHANNEL    mmioFOURCC('d','a','t','a')

BOOL
ReadChunk(HANDLE fh, DWORD dwChunkType, PVOID * ppBuffer, DWORD * pdwSize, PDWORD pdwBytesLeft)
{ DWORD dwBytesRead;
  PVOID pBuffer;
  RIFF_CHUNKHEADER Chunk;

    if ((!pdwBytesLeft) || (*pdwBytesLeft <= 0) || (!pdwSize) || (!ppBuffer)) {
        RETAILMSG(1, (TEXT("Invalid parameter to ReadChunk()\r\n")));
        return FALSE;
    }

    // now scan for the format chunk
    while (*pdwBytesLeft > 0) {
        // now read the wave header (or what we hope is the wave header)
        if (! ReadFile(fh, &Chunk, sizeof(Chunk), &dwBytesRead, NULL) || dwBytesRead < sizeof(Chunk)) {
            RETAILMSG(1, (TEXT("Error reading chunk header\n")));
            return FALSE;
        }
        *pdwBytesLeft -= dwBytesRead;
        RETAILMSG(1, (TEXT("Chunk: \"%c%c%c%c\" size=0x%08x\r\n"), 
            (Chunk.dwCKID >>  0) & 0xff, 
            (Chunk.dwCKID >>  8) & 0xff, 
            (Chunk.dwCKID >> 16) & 0xff, 
            (Chunk.dwCKID >> 24) & 0xff, 
            Chunk.dwSize));
        if (Chunk.dwCKID == dwChunkType) {
            // found the desired chunk
            break;
        }
        // skip the data we don't know or care about...
        if (0xFFFFFFFF == SetFilePointer (fh, Chunk.dwSize, NULL, FILE_CURRENT)) {
            RETAILMSG(1,  (TEXT("Error setting file pointer while scanning for chunk\n")));
            return FALSE;
        }
        *pdwBytesLeft -= Chunk.dwSize;
    }
    // found the desired chunk.
    // allocate a buffer and read in the data
    pBuffer = new BYTE[Chunk.dwSize];
    if (pBuffer == NULL) {
        RETAILMSG(1, (TEXT("Unable to allocate chunk buffer\r\n")));
        return FALSE;
    }
    if (! ReadFile(fh, pBuffer, Chunk.dwSize, &dwBytesRead, NULL) || dwBytesRead < Chunk.dwSize) {
        delete [] pBuffer;
        RETAILMSG(1, (TEXT("Unable to read chunk data\r\n")));
        return FALSE;
    }
    *pdwBytesLeft -= dwBytesRead;
    *ppBuffer = pBuffer;
    *pdwSize = Chunk.dwSize;
    return TRUE;
}

MMRESULT
ReadWaveFile(LPCTSTR pszFilename, PWAVEFORMATEX * ppWFX, PDWORD pdwBufferSize, PBYTE * ppBufferBits)
{ RIFF_FILEHEADER FileHeader;
  DWORD dwBytesRead;
  DWORD dwBufferSize;
  DWORD dwFormatSize;
  PBYTE pBufferBits = NULL;
  PWAVEFORMATEX pwfx = NULL;
  DWORD dwBytesInChunk;
  HANDLE fh;
  MMRESULT mmRet = MMSYSERR_ERROR;

    fh = CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if( fh == INVALID_HANDLE_VALUE ) {
        RETAILMSG(1, (TEXT("Error opening %s. Error code = 0x%08x\n "), pszFilename, GetLastError()));
        return mmRet;
    }

    // Read file and determine sound format
    // Start with RIFF header:

    if (! ReadFile(fh, &FileHeader, sizeof(FileHeader), &dwBytesRead, NULL) || dwBytesRead < sizeof(FileHeader)) {
        RETAILMSG(1, (TEXT("Error reading file header\n")));
        goto ERROR_EXIT;
    }

    if ( FileHeader.dwRiff != RIFF_FILE || FileHeader.dwWave != RIFF_WAVE) {
        RETAILMSG(1, (TEXT("Invalid wave file header\n")));
        goto ERROR_EXIT;
    }

    dwBytesInChunk = FileHeader.dwSize;

    // load the wave format
    if (! ReadChunk(fh, RIFF_FORMAT, (PVOID*) &pwfx, &dwFormatSize, &dwBytesInChunk)) {
        RETAILMSG(1, (TEXT("Unable to read format chunk\r\n")));
        goto ERROR_EXIT;
    }
    if (dwFormatSize < sizeof(PCMWAVEFORMAT)) {
        RETAILMSG(1, (TEXT("Format record too small\r\n")));
        goto ERROR_EXIT;
    }

    // load the wave data
    if (! ReadChunk(fh, RIFF_CHANNEL, (PVOID*) &pBufferBits, &dwBufferSize, &dwBytesInChunk)) {
        RETAILMSG(1, (TEXT("Unable to read format chunk\r\n")));
        goto ERROR_EXIT;
    }

    *ppWFX = pwfx;
    *pdwBufferSize = dwBufferSize;
    *ppBufferBits = pBufferBits;

    // Success
    mmRet = MMSYSERR_NOERROR;
    goto EXIT;

ERROR_EXIT:
    delete [] pBufferBits;
    delete [] pwfx;
EXIT:
    CloseHandle(fh);
    return mmRet;   
}



MMRESULT
WriteWaveFile (LPCTSTR pszFilename, PWAVEFORMATEX pWFX, DWORD dwBufferSize, PBYTE pBufferBits)
{ RIFF_FILEHEADER FileHeader;
  RIFF_CHUNKHEADER WaveHeader;
  RIFF_CHUNKHEADER DataHeader;
  DWORD dwBytesWritten;
  HANDLE fh;
  MMRESULT mmRet = MMSYSERR_ERROR;

    
    // Fill in the file, wave and data headers
    WaveHeader.dwCKID = RIFF_FORMAT;
    WaveHeader.dwSize = sizeof(WAVEFORMATEX) + pWFX->cbSize;

    // the DataHeader chunk contains the audio data
    DataHeader.dwCKID = RIFF_CHANNEL;
    DataHeader.dwSize = dwBufferSize;

    // The FileHeader
    FileHeader.dwRiff = RIFF_FILE;
    FileHeader.dwWave = RIFF_WAVE;
    FileHeader.dwSize = sizeof(FileHeader.dwWave)+sizeof(WaveHeader) + WaveHeader.dwSize + sizeof(DataHeader) + DataHeader.dwSize;

    // Open wave file
    fh = CreateFile(pszFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if( fh == INVALID_HANDLE_VALUE ) {
        RETAILMSG(1, (TEXT("Error opening %s. Error code = 0x%08x\n"), pszFilename, GetLastError()));
        return mmRet;
    }

    // write the riff file
    if (! WriteFile(fh, &FileHeader, sizeof(FileHeader), &dwBytesWritten, NULL)) {
        RETAILMSG(1, (TEXT("Error writing file header\r\n")));
        goto ERROR_EXIT;
    }

    // write the wave header
    if (! WriteFile(fh, &WaveHeader, sizeof(WaveHeader), &dwBytesWritten, NULL)) {
        RETAILMSG(1, (TEXT("Error writing wave header\r\n")));
        goto ERROR_EXIT;
    }

    // write the wave format
    if (! WriteFile(fh, pWFX, WaveHeader.dwSize, &dwBytesWritten, NULL)) {
        RETAILMSG(1, (TEXT("Error writing wave format\r\n")));
        goto ERROR_EXIT;
    }

    // write the data header
    if (! WriteFile(fh, &DataHeader, sizeof(DataHeader), &dwBytesWritten, NULL)) {
        RETAILMSG(1, (TEXT("Error writing PCM data header\r\n")));
        goto ERROR_EXIT;
    }

    // write the PCM data
    if (! WriteFile(fh, pBufferBits, DataHeader.dwSize, &dwBytesWritten, NULL)) {
        RETAILMSG(1, (TEXT("Error writing PCM data\r\n")));
        goto ERROR_EXIT;
    }

    // Success
    mmRet = MMSYSERR_NOERROR;

ERROR_EXIT:
    CloseHandle(fh);
    return mmRet;
}

