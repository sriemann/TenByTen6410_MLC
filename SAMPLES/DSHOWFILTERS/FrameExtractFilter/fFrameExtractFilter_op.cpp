//------------------------------------------------------------------------------
// File: fFrameExtractFilter_op.cpp
//
// Desc: implement CFrameExtractFilterStream class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "fFrameExtractFilter.h"
#include "fFrameExtractFilter_op.h"

//
// Constructor
//
CFrameExtractFilterStream::CFrameExtractFilterStream(HRESULT *phr, CFrameExtractFilter *pParent, LPCWSTR pPinName)
    : CSourceStream(NAME("CFrameExtractFilterStream"), phr, pParent, pPinName)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
}


//
// Destructor
//
CFrameExtractFilterStream::~CFrameExtractFilterStream()
{
}


//
// GetMediaType
//
HRESULT CFrameExtractFilterStream::GetMediaType(CMediaType *pMediaType)
{
    CAutoLock lock(m_pFilter->pStateLock());

    ZeroMemory(pMediaType, sizeof(CMediaType));

    *pMediaType = ((CFrameExtractFilter *)m_pFilter)->m_mt;

    return S_OK;
}

//
// DecideBufferSize
//
HRESULT CFrameExtractFilterStream::DecideBufferSize(IMemAllocator *pMemAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    ASSERT(pMemAlloc);
    ASSERT(pProperties);

    HRESULT hr;

    pProperties->cbBuffer = 256*1024;    // 256 KB
    pProperties->cBuffers = 3;
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


//
// FillBuffer
//
HRESULT CFrameExtractFilterStream::FillBuffer(IMediaSample *pSample)
{
    static INT32    m_FrameTime, m_FrameTimePrev, m_FrameTimeTotal;
    static INT32    m_FrameCounter = 0;
    static INT32    m_FrameSyncCounter = 0;
    INT32            fps;

    CAutoLock lock(m_pFilter->pStateLock());

    HRESULT hr;
    BYTE *pBuffer;
    long lSize;
    int nRead = 0;

    // Get the pointer for output buffer
    hr = pSample->GetPointer(&pBuffer);
    if (FAILED(hr))
        return S_FALSE;

    // Get the size of input sample
    lSize = pSample->GetSize();

    CFrameExtractFilter *pFilter = (CFrameExtractFilter *)m_pFilter;
    if ((nRead = pFilter->GetNextFrame(pBuffer, (unsigned int *) &lSize)) <= 0)
        return S_FALSE;
    
    pSample->SetActualDataLength(nRead);

//printf("FrameExtractor Time : %ld\n", GetTickCount());

    if(m_FrameCounter == 0){
        m_FrameTimeTotal = 0;
        m_FrameTimePrev = GetTickCount();
    }
    else{
        m_FrameTime = GetTickCount();
        m_FrameTimeTotal += m_FrameTime - m_FrameTimePrev;
        m_FrameTimePrev = m_FrameTime;
    }



    if(m_FrameCounter >= 100){
        fps = m_FrameTimeTotal/m_FrameCounter;
        printf("AvgFps per 100 frames : %d FPS\n",1000/fps);
        m_FrameTimeTotal = 0;
        m_FrameCounter = 0;
    }

    REFERENCE_TIME m_rtFrameLength = UNITS/5;
    REFERENCE_TIME rtStart = m_FrameSyncCounter*m_rtFrameLength;
    REFERENCE_TIME rtStop = rtStart + m_rtFrameLength;

    //pSample->SetTime(&rtStart, &rtStop);
    //pSample->SetSyncPoint(TRUE);

    m_FrameCounter++;
    m_FrameSyncCounter++;
    
    return S_OK;
}



HRESULT CFrameExtractFilterStream::Stop(void)
{
    CFrameExtractFilter *pFilter = (CFrameExtractFilter *)m_pFilter;

//    pFilter->CloseFile();

    return CSourceStream::Stop();
}

//
// OnThreadCreate
//
HRESULT CFrameExtractFilterStream::OnThreadCreate(void)
{
    RETAILMSG(1, (L"[FrameExtractFilter] OnThreadCreate\n"));

    return CSourceStream::OnThreadCreate();
}


//
// OnThreadDestroy
//
HRESULT CFrameExtractFilterStream::OnThreadDestroy(void)
{
    RETAILMSG(1, (L"[FrameExtractFilter] OnThreadDestroy\n"));

    return CSourceStream::OnThreadDestroy();
}


//
// OnThreadStartPlay
//
HRESULT CFrameExtractFilterStream::OnThreadStartPlay(void)
{
    RETAILMSG(1, (L"[FrameExtractFilter] OnThreadStartPlay\n"));

    return CSourceStream::OnThreadStartPlay();
}
