//------------------------------------------------------------------------------
// File: fFrameExtractFilter_op.h
//
// Desc: define CFrameExtractFilterStream class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI.  All rights reserved.
//------------------------------------------------------------------------------

#if !defined(_SAMSUNG_SYSLSI_FFRAMEEXTRACTFILTER_OP_H_)
#define _SAMSUNG_SYSLSI_FFRAMEEXTRACTFILTER_OP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CFrameExtractFilterStream : public CSourceStream
{
public:
    CFrameExtractFilterStream(HRESULT *phr, CFrameExtractFilter *pParent, LPCWSTR pPinName);
    virtual ~CFrameExtractFilterStream(void);

// Attributes
public:

// Operations
public:

// Overrides
protected:
    ///////////////////////////
    // Media Type support

    // media types filter have
    virtual HRESULT GetMediaType(CMediaType *pMediaType);

    ///////////////////////////
    // Buffer Negotiation support

    // buffer size
    virtual HRESULT DecideBufferSize(IMemAllocator *pMemAlloc, ALLOCATOR_PROPERTIES *pProperties);

    ///////////////////////////
    // Data Source support

    // media sample
    virtual HRESULT FillBuffer(IMediaSample *pSample);

    virtual HRESULT Stop();

    // signal
    virtual HRESULT OnThreadCreate(void);
    virtual HRESULT OnThreadDestroy(void);
    virtual HRESULT OnThreadStartPlay(void);

// Overrides
protected:

// Implementations
protected:

// member variables
private:

};


#endif // !defined(_SAMSUNG_SYSLSI_FFRAMEEXTRACTFILTER_OP_H_)
