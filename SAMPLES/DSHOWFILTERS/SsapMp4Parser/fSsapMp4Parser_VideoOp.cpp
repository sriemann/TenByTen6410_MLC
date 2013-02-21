//------------------------------------------------------------------------------
// File: fFrameExtractFilter_op.cpp
//
// Desc: implement CMP4ParserVideoOp class
//
// Copyright (c) 2008, Samsung Electronics S.LSI.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "fSsapMp4Parser.h"
#include "fSsapMp4Parser_VideoOp.h"

#include <stdio.h>

//
// Constructor
//
CMP4ParserVideoOp::CMP4ParserVideoOp(HRESULT *phr, CMP4ParserFilter *pParent, LPCWSTR pPinName)
    : CSourceStream(NAME("CMP4ParserVideoOp"), phr, pParent, pPinName),
      CSourceSeeking(NAME("PinSeek"), (IPin*) this, phr, &m_cSharedState)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

}


//
// Destructor
//
CMP4ParserVideoOp::~CMP4ParserVideoOp()
{
}


//
// GetMediaType
//
HRESULT CMP4ParserVideoOp::GetMediaType(CMediaType *pMediaType)
{
    CAutoLock lock(m_pFilter->pStateLock());

    ZeroMemory(pMediaType, sizeof(CMediaType));

    // TODO: modify this option
    *pMediaType = ((CMP4ParserFilter *)m_pFilter)->m_video_mt;

    // m_rtDuration & m_rtStop are used by WMP
    // for displaying the total playback time and progress bar.
    // This function is the most proper place to set.
    m_rtDuration = (REFERENCE_TIME) ((CMP4ParserFilter *)m_pFilter)->GetVideoMSecDuration();
    m_rtDuration = m_rtDuration * 10000i64;
    m_rtStop     = m_rtDuration;

    return S_OK;
}

//
// DecideBufferSize
//
HRESULT CMP4ParserVideoOp::DecideBufferSize(IMemAllocator *pMemAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    ASSERT(pMemAlloc);
    ASSERT(pProperties);

    HRESULT hr;


    pProperties->cbBuffer = (((CMP4ParserFilter *)m_pFilter)->GetVideoSampleMaxSize()   +   512) & 0xFFFFFE00;    // 512 is for SPS+PPS...
    pProperties->cBuffers = 5;
    pProperties->cbAlign  = 1;
    pProperties->cbPrefix = 0;


    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pMemAlloc->SetProperties(pProperties, &Actual);
    if (FAILED(hr)) 
    {
        return hr;
    }

    // Is this allocator unsuitable
    if (Actual.cbBuffer < pProperties->cbBuffer) 
    {
        return E_FAIL;
    }

    return S_OK;
}

static void SaveData(unsigned char *pBuffer, int lSize)
{
    FILE *fp;

    fp = fopen("\\Storage Card\\hdr.tmp", "wb");
    if (fp == NULL)
        return;

    fwrite(pBuffer, 1, lSize, fp);
    fclose(fp);
}

//
// FillBuffer
//
HRESULT CMP4ParserVideoOp::FillBuffer(IMediaSample *pSample)
{
    CMP4ParserFilter *pFilter;

    unsigned int   isIFrame;

unsigned long tt;

    CAutoLock lock(m_pFilter->pStateLock());

    // End of Stream?
    if (m_rtMediaTime >= m_rtStop)
        return S_FALSE;

    HRESULT hr;
    BYTE *pBuffer;
    long lSize;

    // Get the pointer of MP4 Parser Filter
    pFilter = (CMP4ParserFilter *)m_pFilter;


    // If the current position was moved,
    // the 'm_bDiscontinuity' flag was set.
    // When the discontinuity happens, the I-frame (Sync sample) should be searched.
    // The output pin does not send the P-frame.
    // Once the I-frame is found, change the sample time to the I-frame's value.
    if (m_bDiscontinuity) {
        isIFrame = pFilter->IsSyncSample(m_nSampleId);
        if (!isIFrame) {

            do {
                m_nSampleId++;
                m_rtSampleTime += m_rtRepeatTime;
                m_rtMediaTime   += m_rtRepeatTime;
                isIFrame = pFilter->IsSyncSample(m_nSampleId);
            } while (!isIFrame && (m_nSampleId < pFilter->GetVideoNumSamples()));
        }

        tt = GetTickCount();
    }

    // Get the pointer for output buffer
    hr = pSample->GetPointer(&pBuffer);
    if (FAILED(hr))
        return S_FALSE;

    // Get the Video frame of 'm_nSampleId'.
    if (pFilter->GetVideoFrame(pBuffer, (unsigned int *) &lSize, &isIFrame, m_nSampleId) <= 0)
        return S_FALSE;

/*
    if (m_nSampleId == 1) {
        SaveData(pBuffer, lSize);
    }
*/

    pSample->SetActualDataLength(lSize);

    // Put the timestamp on the sample
    REFERENCE_TIME    rtStart, rtStop;
    rtStart = (REFERENCE_TIME) (m_rtSampleTime / m_dRateSeeking);
    rtStop  = rtStart + (REFERENCE_TIME) (m_rtRepeatTime / m_dRateSeeking);
    pSample->SetTime(&rtStart, &rtStop);

    if (isIFrame) {
        RETAILMSG(0, (L"\n Sync Sample (id = %d)", m_nSampleId));
    }
    pSample->SetSyncPoint(isIFrame);


    m_rtSampleTime  += m_rtRepeatTime;
    m_rtMediaTime   += m_rtRepeatTime;

    m_nSampleId++;


    // If the current position was moved,
    // the 'm_bDiscontinuity' flag was set
    // and therefore the sample needs to be set as discontinuity.
    if (m_bDiscontinuity) {
        pSample->SetDiscontinuity(TRUE);
        m_bDiscontinuity = FALSE;

        tt = GetTickCount() - tt;
RETAILMSG(0, (L"\n    ++++   time = %u ms", tt));
    }


    return S_OK;
}


//
// OnThreadCreate
//
HRESULT CMP4ParserVideoOp::OnThreadCreate(void)
{
    double  frame_time = ((CMP4ParserFilter *)m_pFilter)->GetVideoFrameTime();    // in seconds


    // SEEKING: Reset the sample time to zero
    CAutoLock lock(&m_cSharedState);

    m_rtSampleTime = 0;
    m_rtMediaTime  = m_rtStart;
    m_nSampleId    = 1;

    // we need to also reset the repeat time in case the system
    // clock is turned off after m_iRepeatTime gets very big
    m_rtRepeatTime = (REFERENCE_TIME) (UNITS * frame_time);

    return CSourceStream::OnThreadCreate();
}


//
// OnThreadDestroy
//
HRESULT CMP4ParserVideoOp::OnThreadDestroy(void)
{
    return CSourceStream::OnThreadDestroy();
}


//
// OnThreadStartPlay
//
HRESULT CMP4ParserVideoOp::OnThreadStartPlay(void)
{
    {
        CAutoLock lock(CSourceSeeking::m_pLock);
        m_bDiscontinuity = TRUE;
    }

    return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}


//
// ChangeStart
//
HRESULT CMP4ParserVideoOp::ChangeStart(void)
{
    // Reset stream time to zero and the source time to m_rtStart.
    {
        CAutoLock lock(CSourceSeeking::m_pLock);
        m_rtSampleTime = 0;
        m_rtMediaTime  = m_rtStart;
        m_nSampleId    = ((int) (m_rtStart / m_rtRepeatTime)) + 1;

        m_bDiscontinuity = TRUE;
    }

    // Change the start time then flush the graph.
    UpdateFromSeek();

    return S_OK;
}


//
// OnThreadDestroy
//
HRESULT CMP4ParserVideoOp::ChangeStop(void)
{
    CAutoLock lock(CSourceSeeking::m_pLock);
    if (m_rtMediaTime < m_rtStop)
        return S_OK;


    // We've already past the new stop time. Flush the graph.
    UpdateFromSeek();

    return S_OK;
}


//
// OnThreadStartPlay
//
HRESULT CMP4ParserVideoOp::ChangeRate(void)
{
    // Now ChangeRate won't ever be called,
    // but it's pure virtual, so it needs a dummy implementaion
    return S_OK;
}

//
// SetRate
//
STDMETHODIMP CMP4ParserVideoOp::SetRate(double dRate)
{
    if (dRate <= 1.0)
        return E_INVALIDARG;

    {
        CAutoLock lock(CSourceSeeking::m_pLock);
        m_dRateSeeking = dRate;
    }

    // Change rate then flush the graph
    UpdateFromSeek();

    // Now ChangeRate won't ever be called,
    // but it's pure virtual, so it nneds a dummy implementaion
    return S_OK;
}


//
// UpdateFromSeek
//
void CMP4ParserVideoOp::UpdateFromSeek(void)
{
    if (ThreadExists()) 
    {
        // next time around the loop, the worker thread will
        // pick up the position change.
        // We need to flush all the existing data - we must do that here
        // as our thread will probably be blocked in GetBuffer otherwise
        DeliverBeginFlush();

        // make sure we have stopped pushing
        Stop();

        // complete the flush
        DeliverEndFlush();

        // restart
        Run();
    }
}




STDMETHODIMP CMP4ParserVideoOp::SetTimeFormat(const GUID* pFormat)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    CheckPointer(pFormat, E_POINTER);

    if (m_pFilter->IsActive())
    {
        // Cannot switch formats while running
        return VFW_E_WRONG_STATE;
    }

    if (S_OK != IsFormatSupported(pFormat))
        return E_INVALIDARG;    // We don't support this time format.

    return S_OK;
}

STDMETHODIMP CMP4ParserVideoOp::GetTimeFormat(GUID* pFormat)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    CheckPointer(pFormat, E_POINTER);

    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP CMP4ParserVideoOp::IsUsingTimeFormat(const GUID* pFormat)
{
    return IsFormatSupported(pFormat);
}


STDMETHODIMP CMP4ParserVideoOp::IsFormatSupported(const GUID* pFormat)
{
    CheckPointer(pFormat, E_POINTER);

    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
        return S_OK;
//    else if (*pFormat == TIME_FORMAT_FRAME)
//        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP CMP4ParserVideoOp::QueryPreferredFormat(GUID* pFormat)
{
    CheckPointer(pFormat, E_POINTER);

    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP CMP4ParserVideoOp::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMP4ParserVideoOp::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
    return CSourceSeeking::SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CMP4ParserVideoOp::GetDuration(LONGLONG* pDuration)
{
    CheckPointer(pDuration, E_POINTER);
    *pDuration = m_rtDuration;

    return S_OK;
}

STDMETHODIMP CMP4ParserVideoOp::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{

    if (pCurrent)    *pCurrent = m_rtMediaTime;
    if (pStop)        *pStop    = m_rtStop;
    return S_OK;
}


STDMETHODIMP CMP4ParserVideoOp::GetCapabilities(DWORD* pCapabilities)
{
    return pCapabilities ? *pCapabilities = 
        AM_SEEKING_CanGetStopPos|
        AM_SEEKING_CanGetDuration|
        AM_SEEKING_CanSeekAbsolute|
        AM_SEEKING_CanSeekForwards|
        AM_SEEKING_CanSeekBackwards, S_OK : E_POINTER;
}
