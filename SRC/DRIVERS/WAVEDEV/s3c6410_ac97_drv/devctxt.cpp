//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include "wavemain.h"


BOOL DeviceContext::IsSupportedFormat(LPWAVEFORMATEX lpFormat)
{
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM)
        return FALSE;

    if (  (lpFormat->nChannels!=1) && (lpFormat->nChannels!=2) )
        return FALSE;

    if (  (lpFormat->wBitsPerSample!=8) && (lpFormat->wBitsPerSample!=16) )
        return FALSE;

    if (lpFormat->nSamplesPerSec < 100 || lpFormat->nSamplesPerSec > 192000)
        return FALSE;

    return TRUE;
}


// We also support MIDI on output
BOOL OutputDeviceContext::IsSupportedFormat(LPWAVEFORMATEX lpFormat)
{
    if (lpFormat->wFormatTag == WAVE_FORMAT_MIDI)
    {
        return TRUE;
    }

    return DeviceContext::IsSupportedFormat(lpFormat);
}


// Assumes lock is taken
void DeviceContext::NewStream(StreamContext *pStreamContext)
{
    InsertTailList(&m_StreamList,&pStreamContext->m_Link);
}


// Assumes lock is taken
void DeviceContext::DeleteStream(StreamContext *pStreamContext)
{
    RemoveEntryList(&pStreamContext->m_Link);
}


// Returns # of samples of output buffer filled
// Assumes that g_pHWContext->Lock already held.
PBYTE DeviceContext::TransferBuffer(PBYTE pBuffer, PBYTE pBufferEnd, DWORD *pNumStreams)
{
    PLIST_ENTRY pListEntry;
    StreamContext *pStreamContext;
    PBYTE pBufferLastThis;
    PBYTE pBufferLast=pBuffer;
    DWORD NumStreams=0;

    pListEntry = m_StreamList.Flink;
    while (pListEntry != &m_StreamList)
    {
        // Get a pointer to the stream context
        pStreamContext = CONTAINING_RECORD(pListEntry,StreamContext,m_Link);

        // Note: The stream context may be closed and removed from the list inside
        // of Render, and the context may be freed as soon as we call Release.
        // Therefore we need to grab the next Flink first in case the
        // entry disappears out from under us.
        pListEntry = pListEntry->Flink;

        // Render buffers
        pStreamContext->AddRef();
        pBufferLastThis = pStreamContext->Render(pBuffer, pBufferEnd, pBufferLast);
        pStreamContext->Release();
        if (pBufferLastThis>pBuffer)
        {
            NumStreams++;
        }
        if (pBufferLast < pBufferLastThis)
        {
            pBufferLast = pBufferLastThis;
        }
    }

    if (pNumStreams)
    {
        *pNumStreams=NumStreams;
    }
    return pBufferLast;
}


void DeviceContext::RecalcAllGains()
{
    PLIST_ENTRY pListEntry;
    StreamContext *pStreamContext;

    for (pListEntry = m_StreamList.Flink;
        pListEntry != &m_StreamList;
        pListEntry = pListEntry->Flink)
    {
        pStreamContext = CONTAINING_RECORD(pListEntry,StreamContext,m_Link);
        pStreamContext->GainChange();
    }
    return;
}


void OutputDeviceContext::StreamReadyToRender(StreamContext *pStreamContext)
{
    g_pHWContext->StartOutputDMA();
    return;
}


void InputDeviceContext::StreamReadyToRender(StreamContext *pStreamContext)
{
    g_pHWContext->StartInputDMA();
    return;
}


DWORD OutputDeviceContext::GetDevCaps(LPVOID pCaps, DWORD dwSize)
{
    static const WAVEOUTCAPS wc =
    {
        MM_MICROSOFT,
        24,
        0x0001,
        TEXT("Audio Output"),
        WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_4M08 |
        WAVE_FORMAT_1S08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4S08 |
        WAVE_FORMAT_1M16 | WAVE_FORMAT_2M16 | WAVE_FORMAT_4M16 |
        WAVE_FORMAT_1S16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S16,
        1,
        0,
        WAVECAPS_VOLUME | WAVECAPS_PLAYBACKRATE
    };

    memcpy( pCaps, &wc, min(dwSize,sizeof(wc)));
    return MMSYSERR_NOERROR;
}


DWORD InputDeviceContext::GetDevCaps(LPVOID pCaps, DWORD dwSize)
{
    static const WAVEINCAPS wc =
    {
        MM_MICROSOFT,
        23,
        0x0001,
        TEXT("Audio Input"),
        WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_4M08 |
        WAVE_FORMAT_1S08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4S08 |
        WAVE_FORMAT_1M16 | WAVE_FORMAT_2M16 | WAVE_FORMAT_4M16 |
        WAVE_FORMAT_1S16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S16,
        1,
        0
    };

    memcpy( pCaps, &wc, min(dwSize,sizeof(wc)));
    return MMSYSERR_NOERROR;
}


DWORD OutputDeviceContext::GetExtDevCaps(LPVOID pCaps, DWORD dwSize)
{
    static const WAVEOUTEXTCAPS wec =
    {
        0x0000FFFF,                         // max number of hw-mixed streams
        0x0000FFFF,                         // available HW streams
        0,                                  // preferred sample rate for software mixer (0 indicates no preference)
        0,                                  // preferred buffer size for software mixer (0 indicates no preference)
        0,                                  // preferred number of buffers for software mixer (0 indicates no preference)
        8000,                               // minimum sample rate for a hw-mixed stream
        48000                               // maximum sample rate for a hw-mixed stream
    };

    memcpy( pCaps, &wec, min(dwSize,sizeof(wec)));
    return MMSYSERR_NOERROR;
}


DWORD InputDeviceContext::GetExtDevCaps(LPVOID pCaps, DWORD dwSize)
{
    return MMSYSERR_NOTSUPPORTED;
}


StreamContext *InputDeviceContext::CreateStream(LPWAVEOPENDESC lpWOD)
{
    return new InputStreamContext;
}


StreamContext *OutputDeviceContext::CreateStream(LPWAVEOPENDESC lpWOD)
{
    LPWAVEFORMATEX lpFormat=lpWOD->lpFormat;
    if (lpWOD->lpFormat->wFormatTag == WAVE_FORMAT_MIDI)
    {
        return new CMidiStream;
    }

    if (lpFormat->nChannels==1)
    {
        if (lpFormat->wBitsPerSample==8)
        {
            return new OutputStreamContextM8;
        }
        else
        {
            return new OutputStreamContextM16;
        }
    }
    else
    {
        if (lpFormat->wBitsPerSample==8)
        {
            return new OutputStreamContextS8;
        }
        else
        {
            return new OutputStreamContextS16;
        }
    }
}


DWORD DeviceContext::OpenStream(LPWAVEOPENDESC lpWOD, DWORD dwFlags, StreamContext **ppStreamContext)
{
    HRESULT Result;
    StreamContext *pStreamContext;

    if (lpWOD->lpFormat==NULL)
    {
        return WAVERR_BADFORMAT;
    }

    if (!IsSupportedFormat(lpWOD->lpFormat))
    {
        return WAVERR_BADFORMAT;
    }

    // Query format support only - don't actually open device?
    if (dwFlags & WAVE_FORMAT_QUERY)
    {
        return MMSYSERR_NOERROR;
    }

    pStreamContext = CreateStream(lpWOD);
    if (!pStreamContext)
    {
        return MMSYSERR_NOMEM;
    }

    Result = pStreamContext->Open(this,lpWOD,dwFlags);
    if (FAILED(Result))
    {
        delete pStreamContext;
        return MMSYSERR_ERROR;
    }

    *ppStreamContext=pStreamContext;
    return MMSYSERR_NOERROR;
}

