//------------------------------------------------------------------------------
// File: fFrameExtractFilter.h
//
// Desc: define CFrameExtractFilter class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI. All rights reserved.
//------------------------------------------------------------------------------

#if !defined(_SAMSUNG_SYSLSI_FFRAMEEXTRACTFILTER_H_)
#define _SAMSUNG_SYSLSI_FFRAMEEXTRACTFILTER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include "FrameExtractor.h"

class CFrameExtractFilterStream;


class CFrameExtractFilter : public CSource, public IFileSourceFilter
{
    friend class CFrameExtractFilterStream;

public:
    CFrameExtractFilter(LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CFrameExtractFilter(void);

// static
public:
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// Implementations
protected:
    // IUnknown interface
    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IFileSourceFilter){
            return GetInterface((IFileSourceFilter *)this, ppv);
        }

        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }

    // IFileSourceFilter interface
    STDMETHODIMP Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetCurFile(LPOLESTR *ppwszFileName, AM_MEDIA_TYPE *pmt);

private:
    HRESULT CheckFile(LPCTSTR pszFileName, const AM_MEDIA_TYPE *pmt);
    HRESULT MakeMediaType(CMediaType *pMediaType, int width, int height);

// member variables
private:
    CMediaType      m_mt;
    LPWSTR          m_pwFileName;

    void           *m_File;    // FILE pointer
    void           *m_pFramexCtx;

#define RAW_STRM_TYPE_M4V        100
#define RAW_STRM_TYPE_H264RAW    101
#define RAW_STRM_TYPE_RCV        102
    int             m_StrmType;    // 0: m4v, 1: h264raw, 2: rcv

    //ULONG            frameSize;

    int             m_first;

public:
    int GetNextFrame(unsigned char *pFrameBuf, unsigned int *pSize);
    int CloseFile();
};


#endif // !defined(_SAMSUNG_SYSLSI_FFRAMEEXTRACTFILTER_H_)
