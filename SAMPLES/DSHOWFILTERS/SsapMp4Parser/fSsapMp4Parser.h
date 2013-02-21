//------------------------------------------------------------------------------
// File: fSsapMp4Parser.h
//
// Desc: define CMP4ParserFilter class
//
// Copyright (c) 2008, Samsung Electronics S.LSI.  All rights reserved.
//------------------------------------------------------------------------------

#if !defined(_SYSLSI_FSSAP_MP4_PARSER_H_)
#define _SYSLSI_FSSAP_MP4_PARSER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMP4ParserAudioOp;
class CMP4ParserVideoOp;


class CMP4ParserFilter : public CSource, public IFileSourceFilter
{
    friend class CMP4ParserAudioOp;
    friend class CMP4ParserVideoOp;

public:
    CMP4ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CMP4ParserFilter(void);

// static
public:
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// Implementations
protected:
    // IUnknown interface
    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IFileSourceFilter) 
            return GetInterface((IFileSourceFilter *)this, ppv);

        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }

    // IFileSourceFilter interface
    STDMETHODIMP Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetCurFile(LPOLESTR *ppwszFileName, AM_MEDIA_TYPE *pmt);

private:
    HRESULT  CheckFile(LPCTSTR pszFileName);
    HRESULT  MakeVideoMediaType(CMediaType *pMediaType);
    HRESULT  MakeAudioMediaType(CMediaType *pMediaType);

// member variables
private:
    CMediaType      m_video_mt;
    CMediaType      m_audio_mt;
    LPWSTR          m_pwFileName;

#define RAW_STRM_TYPE_M4V        100
#define RAW_STRM_TYPE_H263        101
#define RAW_STRM_TYPE_H264RAW    102
#define RAW_STRM_TYPE_M4A        200
#define RAW_STRM_TYPE_AMR_NB    201
    int             m_video_codec;
    int             m_audio_codec;


    void           *m_hMP4File;        // MP4 file handle

    int             m_video_trId;
    int             m_video_num_samples;
    unsigned int    m_video_timescale;
    unsigned int    m_video_duration;
    unsigned int    m_video_sample_max_size;

    int             m_video_width, m_video_height;
    double          m_video_framerate;
    int             m_video_profile;


    int             m_audio_trId;
    int             m_audio_num_samples;
    unsigned int    m_audio_timescale;
    unsigned int    m_audio_duration;
    unsigned int    m_audio_sample_max_size;

    int             m_audio_num_channels;



    unsigned int    m_h264_bufleng_size;
    unsigned char  *m_h264_tmp_buf;

public:
    double         GetVideoFrameRate()      { return  m_video_framerate; };
    unsigned int   GetVideoMSecDuration()   { return  (unsigned int) ((float) m_video_duration / m_video_timescale * 1000.0f); };
    unsigned int   GetVideoSampleMaxSize()  { return  m_video_sample_max_size; };
    unsigned int   GetVideoNumSamples()     { return  m_video_num_samples; };
    unsigned int   IsSyncSample(int sample_id);

    double         GetAudioTimescale()      { return  m_audio_timescale; };
    unsigned int   GetAudioMSecDuration()   { return  (unsigned int) ((float) m_audio_duration / m_audio_timescale * 1000.0f); };
    unsigned int   GetAudioSampleMaxSize()  { return  m_audio_sample_max_size; };

    int     GetVideoFrame(unsigned char *pFrameBuf, unsigned int *pSize, unsigned int *isIframe, int  sample_id);
    int     GetAudioFrame(unsigned char *pFrameBuf, unsigned int *pSize, int  sample_id);
    double  GetVideoFrameTime();    // returns the time (in seconds) per one video frame
    double  GetAudioFrameTime();    // returns the time (in seconds) per one audio frame
};



//#define RETAILMSG(a, msg)    0

#endif // !defined(_SYSLSI_FSSAP_MP4_PARSER_H_)
