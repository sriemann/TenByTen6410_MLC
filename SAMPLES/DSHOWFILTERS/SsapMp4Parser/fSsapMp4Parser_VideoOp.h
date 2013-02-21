//------------------------------------------------------------------------------
// File: fSsapMp4Parser_op.h
//
// Desc: define CMP4ParserVideoOp class
//
// Copyright (c) 2008, Samsung Electronics S.LSI.  All rights reserved.
//------------------------------------------------------------------------------

#if !defined(_SYSLSI_FSSAP_MP4_PARSER_VIDEO_OP_H_)
#define _SYSLSI_FSSAP_MP4_PARSER_VIDEO_OP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CMP4ParserVideoOp : public CSourceStream, public CSourceSeeking
{
public:
    CMP4ParserVideoOp(HRESULT *phr, CMP4ParserFilter *pParent, LPCWSTR pPinName);
    virtual ~CMP4ParserVideoOp(void);

// Attributes
public:

// Operations
public:

// Overrides
protected:
    // Override CSourceStream methods.
    virtual HRESULT GetMediaType(CMediaType *pMediaType);
    virtual HRESULT DecideBufferSize(IMemAllocator *pMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
    virtual HRESULT FillBuffer(IMediaSample *pSample);

    // The following methods support seeking.
    virtual HRESULT OnThreadCreate(void);
    virtual HRESULT OnThreadDestroy(void);
    virtual HRESULT OnThreadStartPlay(void);
    virtual HRESULT ChangeStart();    // Inherited from CSourceSeeking
    virtual HRESULT ChangeStop();    // Inherited from CSourceSeeking
    virtual HRESULT ChangeRate();    // Inherited from CSourceSeeking


    STDMETHODIMP SetTimeFormat(const GUID* pFormat);
    STDMETHODIMP GetTimeFormat(GUID* pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
    STDMETHODIMP IsFormatSupported(const GUID* pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
    STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
    STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
    STDMETHODIMP GetDuration(LONGLONG* pDuration);
/*
    STDMETHODIMP GetStopPosition(LONGLONG* pStop);


    STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
*/
    STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
/*
    STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double* pdRate);
*/

    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetCapabilities(DWORD* pCapabilities);

    STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) { return E_FAIL; }


// Implementations
protected:
    // IUnknown interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IMediaSeeking) 
            return CSourceSeeking::NonDelegatingQueryInterface(riid, ppv);
        else
            return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
    }

    void UpdateFromSeek();

// member variables
private:
    CCritSec        m_cSharedState;

    bool            m_bDiscontinuity;

    REFERENCE_TIME    m_rtMediaTime;
    REFERENCE_TIME    m_rtSampleTime;
    REFERENCE_TIME    m_rtRepeatTime;

    int             m_nSampleId;
};


#endif // !defined(_SYSLSI_FSSAP_MP4_PARSER_VIDEO_OP_H_)
