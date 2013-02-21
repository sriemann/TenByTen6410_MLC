//------------------------------------------------------------------------------
// File: fMFCDecFilter.h
//
// Desc: define CMFCDecFilter class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//          Simon Chun(simon.chun@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#if !defined(_SAMSUNG_SYSLSI_FMFCDECFILTER_H_)
#define _SAMSUNG_SYSLSI_FMFCDECFILTER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MFCDriver.h"
#include "MFCDecDriver.h"

#define    FPS
#define NAL_UNIT_TYPE_TYPE(n)    ((0x001F) & (n))


typedef struct
{
    UCHAR  *lum;       /* Luminance pointer */
    UCHAR  *cb;        /* Cb pointer */
    UCHAR  *cr;        /* Cr pointer */
    UINT    width;     /* Width of the frame */
    UINT    height;    /* Height of the frame */
    UINT32    mLength;
} tBaseVideoFrame;

class CMFCDecFilterInputPin;
class CMFCDecFilterOutputPin;


class CMFCDecFilter : public CTransformFilter
{
    friend class CMFCDecFilterInputPin;
    friend class CMFCDecFilterOutputPin;

public:
    CMFCDecFilter(LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CMFCDecFilter(void);
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);



// Attributes
public:
    void               *decode_ctx;


    BOOLEAN                m_MP4IsFirstTransform;
    BOOLEAN                m_MP4IsFirstFrameDecode;
    BOOLEAN                m_H264IsFirstTransform;
    BOOLEAN                m_VC1IsFirstTransform;
    BOOLEAN                m_WMV9sFirstTransform;

    int                 m_Width, m_Height,m_Wmvdata;
    BYTE                *m_pFrmBuf;
    long                    m_FrmSize;

// Overrides
protected:
    // Transform operation, which this filter does
    virtual HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    // check if you can support mtIn
    virtual HRESULT CheckInputType(const CMediaType *pmtIn);
    // check if you can support the transform from this input to this output
    virtual HRESULT CheckTransform(const CMediaType *pmtIn, const CMediaType *pmtOut);
    // call the SetProperties function with appropriate arguments
    virtual HRESULT DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties);
    // override to suggest OUTPUT pin media types
    virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    virtual HRESULT Receive(IMediaSample* pSample);

    virtual HRESULT StopStreaming();

// Implementations
protected:

private:
    HRESULT CopySample(IMediaSample *pSource, IMediaSample *pDest);
    HRESULT MP4Decode(IMediaSample* pSample);
    HRESULT H264Decode(IMediaSample* pSample);
    HRESULT VC1Decode(IMediaSample* pSample);
    HRESULT WMV9Decode(IMediaSample* pSample);

    void    DecoderReset();

// member variables
private:
#define RAW_STRM_TYPE_M4V        100
#define RAW_STRM_TYPE_H264RAW    101
#define RAW_STRM_TYPE_RCV        102
#define VIDEO_STRM_TYPE_WMV9    103
    int                m_StrmType;
//    tBaseVideoFrame        ScaledBuffer;
//    tBaseVideoFrame        DecBuffer;
};


#endif // !defined(_SAMSUNG_SYSLSI_FMFCDECFILTER_H_)
