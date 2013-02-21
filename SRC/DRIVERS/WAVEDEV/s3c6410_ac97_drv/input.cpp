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

// Init input m_DeltaT with (HWSampleRate/SampleRate) calculated in 24.8 fixed point form
// Note that we need to hold the result in a 64-bit value until we're done shifting,
// since the result of the multiply will overflow 32 bits for sample rates greater than
// or equal to the hardware's sample rate.
DWORD InputStreamContext::SetRate(DWORD dwMultiplier)
{
    m_dwMultiplier = dwMultiplier;

    UINT64 Delta = (m_WaveFormat.nSamplesPerSec * m_dwMultiplier) >> 16;
    Delta = ((UINT32)(((1i64<<32)/Delta)+1));
    Delta = (Delta * SAMPLERATE) >> 24;
    m_DeltaT = (DWORD)Delta;
    return MMSYSERR_NOERROR;
}


DWORD InputStreamContext::Stop()
{
    // Stop the stream
    WaveStreamContext::Stop();

    // Return any partially filled buffers to the client
    if ((m_lpWaveHdrCurrent) && (m_lpWaveHdrCurrent->dwBytesRecorded>0))
    {
        GetNextBuffer();
    }

    return MMSYSERR_NOERROR;
}


//#if (OUTCHANNELS==2)
#if (INCHANNELS==2)
PBYTE InputStreamContext::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast)
{
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain = m_fxpGain;

    LONG CurrT = m_CurrT;
    LONG DeltaT = m_DeltaT;
    PCM_TYPE SampleType = m_SampleType;

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    LONG CurrSamp1 = m_CurrSamp[1];
    LONG PrevSamp1 = m_PrevSamp[1];
    LONG InSamp0;
    LONG InSamp1;

    for (;;)
    {
        // Make sure we have a place to put the data
        if (pCurrData>=pCurrDataEnd)
        {
            goto Exit;
        }

        // Get the next sample
        while (CurrT >= 0x100)
        {
            if (pBuffer>=pBufferEnd)
            {
                goto Exit;
            }

            PrevSamp0 = CurrSamp0;
            PrevSamp1 = CurrSamp1;

            CurrSamp0 = ((HWSAMPLE *)pBuffer)[0];
            CurrSamp1 = ((HWSAMPLE *)pBuffer)[1];
            pBuffer += 2*sizeof(HWSAMPLE);

            // Apply input gain
            CurrSamp0 = (CurrSamp0 * fxpGain) >> 16;
            CurrSamp1 = (CurrSamp1 * fxpGain) >> 16;

            CurrT -= 0x100;
        }

        InSamp0 = (PrevSamp0 + ((CurrT * (CurrSamp0 - PrevSamp0)) >> 8));
        InSamp1 = (PrevSamp1 + ((CurrT * (CurrSamp1 - PrevSamp1)) >> 8));
        CurrT += DeltaT;

        PPCM_SAMPLE pSampleDest = (PPCM_SAMPLE)pCurrData;
        switch (m_SampleType)
        {
        case PCM_TYPE_M8:
        default:
            pSampleDest->m8.sample = (UINT8)( ((InSamp0+InSamp1) >> 9) + 128);
            pCurrData  += 1;
            break;

        case PCM_TYPE_S8:
            pSampleDest->s8.sample_left  = (UINT8)((InSamp0 >> 8) + 128);
            pSampleDest->s8.sample_right = (UINT8)((InSamp1 >> 8) + 128);
            pCurrData  += 2;
            break;

        case PCM_TYPE_M16:
            pSampleDest->m16.sample = (INT16)((InSamp0+InSamp1)>>1);
            pCurrData  += 2;
            break;

        case PCM_TYPE_S16:
            pSampleDest->s16.sample_left  = (INT16)InSamp0;
            pSampleDest->s16.sample_right = (INT16)InSamp1;
            pCurrData  += 4;
            break;
        }
    }

Exit:
    m_lpWaveHdrCurrent->dwBytesRecorded += (pCurrData-m_lpCurrData);
    m_dwByteCount += (pCurrData-m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrT = CurrT;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    m_PrevSamp[1] = PrevSamp1;
    m_CurrSamp[1] = CurrSamp1;
    return pBuffer;
}


#else
PBYTE InputStreamContext::Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast)
{
    PBYTE pCurrData = m_lpCurrData;
    PBYTE pCurrDataEnd = m_lpCurrDataEnd;
    LONG fxpGain = m_fxpGain;

    LONG CurrT = m_CurrT;
    LONG DeltaT = m_DeltaT;

    LONG CurrSamp0 = m_CurrSamp[0];
    LONG PrevSamp0 = m_PrevSamp[0];
    LONG InSamp0;

    PCM_TYPE SampleType = m_SampleType;

    for (;;)
    {
        // Make sure we have a place to put the data
        if (pCurrData>=pCurrDataEnd)
        {
            goto Exit;
        }

        // Get the next sample
        while (CurrT >= 0x100)
        {
            if (pBuffer>=pBufferEnd)
            {
                goto Exit;
            }

            PrevSamp0 = CurrSamp0;
            CurrSamp0 = *(HWSAMPLE *)pBuffer;
            pBuffer += sizeof(HWSAMPLE);

            // Apply input gain?
            CurrSamp0 = (CurrSamp0 * fxpGain) >> 16;

            CurrT -= 0x100;
        }

        InSamp0 = (PrevSamp0 + ((CurrT * (CurrSamp0 - PrevSamp0)) >> 8));
        CurrT += DeltaT;

        PPCM_SAMPLE pSampleDest = (PPCM_SAMPLE)pCurrData;
        switch (m_SampleType)
        {
        case PCM_TYPE_M8:
        default:
            pSampleDest->m8.sample = (UINT8)((InSamp0 >> 8) + 128);
            pCurrData  += 1;
            break;

        case PCM_TYPE_S8:
            pSampleDest->s8.sample_left  = (UINT8)((InSamp0 >> 8) + 128);
            pSampleDest->s8.sample_right = (UINT8)((InSamp0 >> 8) + 128);
            pCurrData  += 2;
            break;

        case PCM_TYPE_M16:
            pSampleDest->m16.sample = (INT16)InSamp0;
            pCurrData  += 2;
            break;

        case PCM_TYPE_S16:
            pSampleDest->s16.sample_left  = (INT16)InSamp0;
            pSampleDest->s16.sample_right = (INT16)InSamp0;
            pCurrData  += 4;
            break;
        }
    }

Exit:
    m_lpWaveHdrCurrent->dwBytesRecorded += (pCurrData-m_lpCurrData);
    m_dwByteCount += (pCurrData-m_lpCurrData);
    m_lpCurrData = pCurrData;
    m_CurrT = CurrT;
    m_PrevSamp[0] = PrevSamp0;
    m_CurrSamp[0] = CurrSamp0;
    return pBuffer;
}
#endif
