//------------------------------------------------------------------------------
// File: fFrameExtractFilter.cpp
//
// Desc: implement CMP4ParserFilter class
//
// Copyright (c) 2008, Samsung Electronics S.LSI.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>

#include "SsapMp4Parser.h"
#include "fSsapMp4Parser.h"
#include "fSsapMp4Parser_AudioOp.h"
#include "fSsapMp4Parser_VideoOp.h"

#include "mp4.h"
#include "get_videoinfo.h"



static unsigned char  h264_delimiter[4] = {0x00, 0x00, 0x00, 0x01};

static int GetVideoStreamHeader(MP4FileHandle mp4File, MP4TrackId video_trId, int video_codec,
                                unsigned char *strm_hdr_buf, int *strm_hdr_leng);
static int GetAudioStreamHeader(MP4FileHandle mp4File, MP4TrackId audio_trId, int audio_codec,
                                unsigned char *strm_hdr_buf, int *strm_hdr_leng);
static int GetVideoProfileAndSize(MP4FileHandle mp4File, MP4TrackId video_trId, int video_codec,
                                  int  *profile, int *width, int *height);

#define NAL_UNIT_TYPE_TYPE(n)    ((0x1F) & (n))


//
// Constructor
//
CMP4ParserFilter::CMP4ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
    : CSource(NAME("CMP4ParserFilter"), pUnk, CLSID_SSAP_MP4Parser_Filter),
    m_pwFileName(NULL)
{
    CAutoLock cAutoLock(&m_cStateLock);


    // Initializing Member variables
    m_hMP4File            = NULL;

    m_video_codec         = 0;
    m_video_trId          = MP4_INVALID_TRACK_ID;
    m_video_num_samples   = 0;

    m_video_timescale     = 0;
    m_video_duration      = 0;
    m_video_profile       = 0;

    m_audio_codec         = 0;
    m_audio_trId          = MP4_INVALID_TRACK_ID;
    m_audio_num_samples   = 0;

    m_audio_timescale     = 0;
    m_audio_duration      = 0;


    m_h264_bufleng_size   = 4;
    m_h264_tmp_buf        = NULL;

    //
    // CSource::AddPin() is automatically called
    // by creating a source stream (CSourceStream)
    //

    // Add one source stream (output pin)!

    // TODO: add source stream here
}


//
// Destructor
//
CMP4ParserFilter::~CMP4ParserFilter()
{
    if (m_pwFileName)
        delete[] m_pwFileName;

    if (m_h264_tmp_buf) {
        free(m_h264_tmp_buf);
        m_h264_tmp_buf = NULL;
    }

    if (m_hMP4File)
        MP4Close(m_hMP4File);
}


//
// Load
//
STDMETHODIMP CMP4ParserFilter::Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt)
{
    // IFileSourceFilter::Load()
    // Any calls to this method after the first call will fail!
    if (m_pwFileName)
        return E_FAIL;


    CheckPointer(pwszFileName, E_POINTER);

    // lstrlenW is one of the few Unicode functions that works on win95
    DWORD dwSize = lstrlenW(pwszFileName) + 1;

#ifndef UNICODE
    TCHAR *pszFileName = 0;
    pszFileName = new char[dwSize * 2];
    if (!pszFileName) 
        return E_OUTOFMEMORY;

    WideCharToMultiByte(GetACP(), 0, pwszFileName, -1,
        pszFileName, dwSize * 2, NULL, NULL);
#else
    TCHAR pszFileName[MAX_PATH] = {0};
    lstrcpy(pszFileName, pwszFileName);
#endif

    CAutoLock lck(&m_cStateLock);

    // file checking and identify the video & audio codec
    HRESULT  hr = CheckFile(pszFileName);
    
#ifndef UNICODE
    delete[] pszFileName;
#endif

    if (FAILED(hr))
        return E_FAIL;

    ///////////////////////////////////////////
    // Add Output Pin for Video Stream
    int        b;
    switch (m_video_codec) {
    case RAW_STRM_TYPE_M4V:            // MPEG4
    case RAW_STRM_TYPE_H263:
// 왜 MPEG4에서 profile을 가져오면 에러가 나느냐... (throw Exception 때문에...)
//        video_profile = MP4GetVideoProfileLevel(m_hMP4File, m_video_trId);
printf("\n[VIDEO] MPEG4, profile = 0x%02X", m_video_profile);
        MakeVideoMediaType(&m_video_mt);
        new CMP4ParserVideoOp(&hr, this, L"Video Out");
        break;

    case RAW_STRM_TYPE_H264RAW:        // H264
printf("\n[VIDEO] H.264, profile = 0x%02X", m_video_profile);
        if (m_video_profile == 66) {    // It supports H.264 Baseline only..
            MakeVideoMediaType(&m_video_mt);
            new CMP4ParserVideoOp(&hr, this, L"Video Out");
        }
        else
            m_video_codec = MP4_INVALID_TRACK_ID;
        break;

    default:
        break;
    }

    ///////////////////////////////////////////
    // Add Output Pin for Audio Stream
    switch (m_audio_codec) {
    case RAW_STRM_TYPE_M4A:            // MPEG4-AAC
        MakeAudioMediaType(&m_audio_mt);
        new CMP4ParserAudioOp(&hr, this, L"Audio Out");
        break;

    case RAW_STRM_TYPE_AMR_NB:        // AMR-NB
        MakeAudioMediaType(&m_audio_mt);
        new CMP4ParserAudioOp(&hr, this, L"Audio Out");
        break;

    default:
        break;
    }

    // Restore the current file name
    m_pwFileName = new WCHAR[dwSize];

    if (m_pwFileName != NULL)
        CopyMemory(m_pwFileName, pwszFileName, dwSize * sizeof(WCHAR));

    return S_OK;
}


//
// GetCurFile
//
STDMETHODIMP CMP4ParserFilter::GetCurFile(LPOLESTR *ppwszFileName, AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppwszFileName, E_POINTER);
    *ppwszFileName = NULL;

    if (m_pwFileName != NULL) 
    {
        DWORD dwSize = sizeof(WCHAR) * (1 + lstrlenW(m_pwFileName));
        
        *ppwszFileName = (LPOLESTR)CoTaskMemAlloc(dwSize);
        if (*ppwszFileName != NULL) 
        {
            CopyMemory(*ppwszFileName, m_pwFileName, dwSize);
        }
    }

    return S_OK;
}


int CMP4ParserFilter::GetVideoFrame(unsigned char *pFrameBuf, unsigned int *pSize, unsigned int *isIframe, int sample_id)
{
    int nRead;

    int            n_video_hdr;

    u_int8_t      *p_video_sample;
    u_int32_t      n_video_sample;

    int            b, ret;


    if ((m_video_codec == RAW_STRM_TYPE_H264RAW) && (m_h264_bufleng_size != 4))
        p_video_sample = (u_int8_t *) m_h264_tmp_buf;
    else
        p_video_sample = (u_int8_t *) pFrameBuf;
    n_video_sample = *pSize;


    if ((sample_id <= 0) || (sample_id > m_video_num_samples)) {
        RETAILMSG(1, (L"\n[CMP4ParserFilter::GetVideoFrame] \'sample_id\' is invalid. (sample_id = %d", sample_id));
        *pSize = 0;
        return -1;
    }

    // if (sampleId == 1),
    // then "video stream header" is extracted and it is put in the buffer.
    if (sample_id == 1) {
        ret = GetVideoStreamHeader(m_hMP4File, m_video_trId, m_video_codec,
                                   pFrameBuf, &n_video_hdr);

        pFrameBuf      += n_video_hdr;
        p_video_sample += n_video_hdr;
        n_video_sample -= n_video_hdr;
    }
    else
        n_video_hdr = 0;

    /////////////////////
    //  MP4ReadSample  //
    /////////////////////
    b = MP4ReadSample(m_hMP4File, m_video_trId, sample_id,
                      &p_video_sample, &n_video_sample,
                      NULL, NULL, NULL, NULL);
    if (!b) {
        *pSize = 0;
        return -1;
    }

    // if (codec == h.264), the first 4 bytes are the length octets.
    // They need to be changed to H.264 delimiter (00 00 00 01).
    if (m_video_codec == RAW_STRM_TYPE_H264RAW) {
        int h264_nal_leng;
        int nal_leng_acc = 0;

        if (m_h264_bufleng_size == 4) {
            do {
                h264_nal_leng = p_video_sample[0];
                h264_nal_leng = (h264_nal_leng << 8) | p_video_sample[1];
                h264_nal_leng = (h264_nal_leng << 8) | p_video_sample[2];
                h264_nal_leng = (h264_nal_leng << 8) | p_video_sample[3];
                memcpy(p_video_sample, h264_delimiter, 4);
                nal_leng_acc   += (h264_nal_leng + 4);
                p_video_sample += (h264_nal_leng + 4);
            } while (nal_leng_acc < n_video_sample);
        }
        else {

            do {
                if (m_h264_bufleng_size == 1)
                    h264_nal_leng = p_video_sample[0];
                else {  //  m_h264_bufleng_size == 2
                    h264_nal_leng = p_video_sample[0];
                    h264_nal_leng = (h264_nal_leng << 8) | p_video_sample[1];
                }

                memcpy(pFrameBuf,     h264_delimiter, 4);
                memcpy(pFrameBuf + 4, p_video_sample + m_h264_bufleng_size, h264_nal_leng);

                nal_leng_acc   += (h264_nal_leng + m_h264_bufleng_size);
                p_video_sample += (h264_nal_leng + m_h264_bufleng_size);
                pFrameBuf      += (h264_nal_leng + 4);

            } while (nal_leng_acc < n_video_sample);

        }
    }


    *pSize = nRead = (n_video_hdr + n_video_sample);
    *isIframe  = MP4GetSampleSync(m_hMP4File, m_video_trId, sample_id);


    return nRead;
}

int CMP4ParserFilter::GetAudioFrame(unsigned char *pFrameBuf, unsigned int *pSize, int sample_id)
{
    int nRead;

    u_int8_t      *p_audio_sample;
    u_int32_t      n_audio_sample;

    int            b, ret;


    p_audio_sample = (u_int8_t *) pFrameBuf;
    n_audio_sample = *pSize;


    if ((sample_id <= 0) || (sample_id > m_audio_num_samples)) {
        RETAILMSG(1, (L"\n[CMP4ParserFilter::GetAudioFrame] \'sample_id\' is invalid. (sample_id = %d", sample_id));
        *pSize = 0;
        return -1;
    }

    /////////////////////
    //  MP4ReadSample  //
    /////////////////////
    b = MP4ReadSample(m_hMP4File, m_audio_trId, sample_id,
                      &p_audio_sample, &n_audio_sample,
                      NULL, NULL, NULL, NULL);
    if (!b) {
        *pSize = 0;
        return -1;
    }

//printf("\n  --- MP4ReadSample=%d", n_audio_sample);

    *pSize = nRead = n_audio_sample;

    return nRead;
}

double  CMP4ParserFilter::GetVideoFrameTime()
{
    double   frame_time;

    frame_time = 1.0 / GetVideoFrameRate();    // in seconds

    return frame_time;
}

double  CMP4ParserFilter::GetAudioFrameTime()
{
    double   frame_time=0.0;


    switch (m_audio_codec) {
    case RAW_STRM_TYPE_M4A:        // AAC-LC
        frame_time = 1024.0 / GetAudioTimescale();    // in seconds
        break;

    case RAW_STRM_TYPE_AMR_NB:    // AMR-NB
        frame_time = 160.0 / GetAudioTimescale();    // in seconds
        break;

    }

    return frame_time;
}

unsigned int  CMP4ParserFilter::IsSyncSample(int sample_id)
{
    return MP4GetSampleSync(m_hMP4File, m_video_trId, sample_id);
}

static void wchar_t2char(const wchar_t *str_wchar, char *str_char)
{
    int        i, j;
    int        leng;
    wchar_t    c;

    leng = wcslen(str_wchar);
    for (i=j=0; i<leng; i++, j++)
    {
        c = str_wchar[i];
        if (c == 0) {
            str_char[j] = '\0';
            break;
        }
        else if (c < 0x80) {
            str_char[j] = (char)str_wchar[i];
        }
        else {
            // Currently the non-alphanumeric characters are not supported.
            str_char[j] = '\0';
            break;
        }
    }

    str_char[j] = '\0';
}

//
// ReadTheFile
//
HRESULT CMP4ParserFilter::CheckFile(LPCTSTR pszFileName)
{
    char      *video_name, *audio_name;
    char       filename[256];


    ///////////////////////////////////////////////
    /////                                     /////
    /////  1. Open the mp4 file for reading.  /////
    /////                                     /////
    ///////////////////////////////////////////////
    wchar_t2char(pszFileName, filename);
//printf("\n(CMP4ParserFilter::CheckFile) filename=%s.", filename);
//    m_hMP4File = MP4Read(filename, 0);
RETAILMSG(1, (L"\n(CMP4ParserFilter::CheckFile) filename=%s.", pszFileName));
    m_hMP4File = MP4ReadWchar(pszFileName, 0);
    if (!m_hMP4File)
        return E_FAIL;

    RETAILMSG(1, (L"\n MP4Read succeed.."));

    /////////////////////////////////////////////////////////
    /////                                               /////
    /////  2. Identify the video & get its properties.  /////
    /////                                               /////
    /////////////////////////////////////////////////////////
    m_video_trId = MP4FindTrackId(m_hMP4File, 0, MP4_VIDEO_TRACK_TYPE, 0);
    if (m_video_trId != MP4_INVALID_TRACK_ID) {

        video_name = (char *) MP4GetTrackMediaDataName(m_hMP4File, m_video_trId);
        if (strcmp(video_name, "mp4v") == 0)
            m_video_codec = RAW_STRM_TYPE_M4V;        // MPEG4
        else if (strcmp(video_name, "s263") == 0)
            m_video_codec = RAW_STRM_TYPE_H263;        // H.263
        else if (strcmp(video_name, "avc1") == 0)
            m_video_codec = RAW_STRM_TYPE_H264RAW;    // H.264
        else
            m_video_codec = 4;    // Unknown

        // Timescale (ticks per second) and duration of the Video track
        m_video_timescale       = MP4GetTrackTimeScale(m_hMP4File, m_video_trId);
        m_video_duration        = MP4GetTrackDuration(m_hMP4File, m_video_trId);

        // Number of video samples, video frame rate
        m_video_num_samples     = MP4GetTrackNumberOfSamples(m_hMP4File, m_video_trId);
        m_video_framerate       = MP4GetTrackVideoFrameRate(m_hMP4File, m_video_trId);
        m_video_sample_max_size = MP4GetTrackMaxSampleSize(m_hMP4File, m_video_trId);

        // video width and height information is extracted from 'tkhd' header.
        // But this information is often wrong.
        // Therefore, the size information is obtained from "Video Header" directly.
//        m_video_width           = MP4GetTrackVideoWidth(m_hMP4File, m_video_trId);
//        m_video_height          = MP4GetTrackVideoHeight(m_hMP4File, m_video_trId);
        GetVideoProfileAndSize(m_hMP4File, m_video_trId, m_video_codec,
                               &m_video_profile, &m_video_width, &m_video_height);

        if (m_video_codec == RAW_STRM_TYPE_H264RAW) {
            if (MP4GetTrackH264LengthSize(m_hMP4File, m_video_trId, &m_h264_bufleng_size)) {
                m_h264_tmp_buf = (unsigned char *) malloc(m_video_sample_max_size + 128);    // 128 : for SPS+PPS
            }
            else {
                m_h264_bufleng_size = 4;
                if (m_h264_tmp_buf) {
                    free(m_h264_tmp_buf);
                    m_h264_tmp_buf = NULL;
                }
            }

        }
    }

    /////////////////////////////////////////////////////////
    /////                                               /////
    /////  3. Identify the audio & get its properties.  /////
    /////                                               /////
    /////////////////////////////////////////////////////////
    m_audio_trId = MP4FindTrackId(m_hMP4File, 0, MP4_AUDIO_TRACK_TYPE, 0);
    if (m_audio_trId != MP4_INVALID_TRACK_ID) {
        audio_name = (char *) MP4GetTrackMediaDataName(m_hMP4File, m_audio_trId);
        if (strcmp(audio_name, "mp4a") == 0)
            m_audio_codec = RAW_STRM_TYPE_M4A;            // MPEG4 AAC
        else if (strcmp(audio_name, "samr") == 0)
            m_audio_codec = RAW_STRM_TYPE_AMR_NB;        // AMR-NB
        else {
            printf("\n !! Unknown Audio (%s) !!\n", audio_name);
            m_audio_codec = 4;    // Unknown
        }

        // Timescale (ticks per second) and duration of the Video track
        m_audio_timescale       = MP4GetTrackTimeScale(m_hMP4File, m_audio_trId);
        m_audio_duration        = MP4GetTrackDuration(m_hMP4File, m_audio_trId);

        // Number of video samples, video width, video height, video frame rate
        m_audio_num_samples     = MP4GetTrackNumberOfSamples(m_hMP4File, m_audio_trId);
        m_audio_num_channels    = 2;//MP4GetTrackAudioChannels(m_hMP4File, m_audio_trId);
        m_audio_sample_max_size = MP4GetTrackMaxSampleSize(m_hMP4File, m_audio_trId);

printf("\n m_audio_trId = %d, m_audio_duration = (%d), m_audio_timescale = (%d) !!\n", m_audio_trId, m_audio_duration, m_audio_timescale);
    }

    return S_OK;
}


//
// MakeVideoMediaType
//
HRESULT CMP4ParserFilter::MakeVideoMediaType(CMediaType *pMediaType)
{
    unsigned int  *size;

    pMediaType->InitMediaType();
    pMediaType->SetType(&MEDIATYPE_Stream);

    switch (m_video_codec) {
    case RAW_STRM_TYPE_M4V:
    case RAW_STRM_TYPE_H263:
        pMediaType->SetSubtype(&MEDIASUBTYPE_m4v);
        break;

    case RAW_STRM_TYPE_H264RAW:
        pMediaType->SetSubtype(&MEDIASUBTYPE_h264raw);
        break;

    default:
        RETAILMSG(1, (L"[SSAP MP4Parser]unsupported file type\n"));
        return E_FAIL;
    }

    size = (unsigned int *) pMediaType->ReallocFormatBuffer(sizeof(unsigned int) * 2);
    size[0] = m_video_width;
    size[1] = m_video_height;

    pMediaType->SetFormat((BYTE *)size, sizeof(unsigned int) * 2);

    return S_OK;
}


//
// MakeAudioMediaType
//
HRESULT CMP4ParserFilter::MakeAudioMediaType(CMediaType *pMediaType)
{
    WAVEFORMATEX  *wfe;
    unsigned char  strm_hdr_buf[128];
    int            strm_hdr_leng;

    pMediaType->InitMediaType();
//    pMediaType->SetType(&MEDIATYPE_Stream);
    pMediaType->SetType(&MEDIATYPE_Audio);
    

    switch (m_audio_codec) {
    case RAW_STRM_TYPE_M4A:
//        pMediaType->SetSubtype(&MEDIASUBTYPE_m4a);
        pMediaType->SetSubtype(&MEDIASUBTYPE_EAACPLUS);
        break;

    case RAW_STRM_TYPE_AMR_NB:
        pMediaType->SetSubtype(&MEDIASUBTYPE_AmrDec);
        break;

    default:
        RETAILMSG(1, (L"[SSAP MP4Parser]unsupported file type\n"));
        return E_FAIL;
    }

    // Audio stream header
    strm_hdr_leng = sizeof(strm_hdr_buf);
    if (GetAudioStreamHeader(m_hMP4File, m_audio_trId, m_audio_codec, strm_hdr_buf, &strm_hdr_leng) != 0)
        return E_FAIL;


    wfe = (WAVEFORMATEX *) pMediaType->ReallocFormatBuffer(sizeof(WAVEFORMATEX) + strm_hdr_leng);
    memset(wfe, 0, sizeof(WAVEFORMATEX) + strm_hdr_leng);
//    wfe->wFormatTag = ;

    wfe->nSamplesPerSec  = m_audio_timescale;
    wfe->nChannels       = m_audio_num_channels;
    wfe->cbSize          = strm_hdr_leng;
    wfe->wBitsPerSample  = 16;
    memcpy(wfe + 1, strm_hdr_buf, strm_hdr_leng);

    pMediaType->SetFormat((BYTE *)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

    return S_OK;
}



static int GetVideoStreamHeader(MP4FileHandle mp4File, MP4TrackId video_trId, int video_codec,
                                unsigned char *strm_hdr_buf, int *strm_hdr_leng)
{
    int            b;

    // for MPEG4
    u_int8_t      *p_video_conf;
    u_int32_t      n_video_conf;

    // for H.264
    u_int8_t     **pp_sps, **pp_pps;
    u_int32_t     *pn_sps, *pn_pps;
    u_int32_t      n_strm_size;
    int            i;


    switch (video_codec) {
    case RAW_STRM_TYPE_M4V:        // MPEG4
        p_video_conf = NULL;
        n_video_conf = 0;
        b = MP4GetTrackESConfiguration(mp4File, video_trId,
                                       &p_video_conf, &n_video_conf);
        if (!b)
            return -1;

        memcpy(strm_hdr_buf, p_video_conf, n_video_conf);
        free(p_video_conf);

        *strm_hdr_leng = n_video_conf;
        break;

    case RAW_STRM_TYPE_H263:        // H.263
        *strm_hdr_leng = 0;
        break;

    case RAW_STRM_TYPE_H264RAW:        // H.264
        pp_sps = pp_pps = NULL;
        pn_sps = pn_pps = NULL;
        n_strm_size     = 0;

        b = MP4GetTrackH264SeqPictHeaders(mp4File, video_trId, &pp_sps, &pn_sps, &pp_pps, &pn_pps);
        if (!b)
            return -1;

        // SPS memcpy
        if (pp_sps) {
            for (i=0; *(pp_sps + i); i++) {
                memcpy(strm_hdr_buf + n_strm_size, h264_delimiter, sizeof(h264_delimiter));
                n_strm_size += sizeof(h264_delimiter);
                memcpy(strm_hdr_buf + n_strm_size, *(pp_sps + i), *(pn_sps + i));
/*
                if (NAL_UNIT_TYPE_TYPE(strm_hdr_buf[n_strm_size]) == 7) {
                    strm_hdr_buf[n_strm_size + 1] = 66;
                }
*/
                n_strm_size += *(pn_sps + i);
                free(*(pp_sps + i));
            }
            free(pp_sps);
        }
        // PPS memcpy
        if (pp_pps) {
            for (i=0; *(pp_pps + i); i++) {
                memcpy(strm_hdr_buf + n_strm_size, h264_delimiter, sizeof(h264_delimiter));
                n_strm_size += sizeof(h264_delimiter);
                memcpy(strm_hdr_buf + n_strm_size, *(pp_pps + i), *(pn_pps + i));
                n_strm_size += *(pn_pps + i);

                free(*(pp_pps + i));
            }
            free(pp_pps);
        }

        *strm_hdr_leng = n_strm_size;
        break;

    default:    // Unknown
        *strm_hdr_leng = 0;
        break;
    }

    return 0;
}

static int GetAudioStreamHeader(MP4FileHandle mp4File, MP4TrackId audio_trId, int audio_codec,
                                unsigned char *strm_hdr_buf, int *strm_hdr_leng)
{
    int            b;

    // for MPEG4
    u_int32_t      n_audio_conf;
    u_int8_t      *p_audio_conf;

    // for AMR-NB
    u_int16_t      amr_mode_set;


    switch (audio_codec) {
    case RAW_STRM_TYPE_M4A:
        p_audio_conf = NULL;
        n_audio_conf = 0;
        b = MP4GetTrackESConfiguration(mp4File, audio_trId, &p_audio_conf, &n_audio_conf);
        if (!b)
            return -1;
        memcpy(strm_hdr_buf, p_audio_conf, n_audio_conf);
        *strm_hdr_leng = n_audio_conf;
        free(p_audio_conf);
        break;

    case RAW_STRM_TYPE_AMR_NB:
        amr_mode_set = MP4GetAmrModeSet(mp4File, audio_trId);
        strm_hdr_buf[0] = (unsigned char) amr_mode_set;
        strm_hdr_buf[1] = (unsigned char) (amr_mode_set >> 8);
        *strm_hdr_leng = 2;
        break;

    default:
        RETAILMSG(1, (L"[SSAP MP4Parser]unsupported file type\n"));
        return E_FAIL;
    }


    return 0;
}


static int GetVideoProfileAndSize(MP4FileHandle mp4File, MP4TrackId video_trId, int video_codec,
                                  int  *profile, int *width, int *height)
{
    int             b, ret;

    unsigned char   strm_hdr_buf[512];
    int             strm_hdr_leng;
    u_int8_t       *p_video_sample=NULL;
    u_int32_t       n_video_sample=0;


    if (RAW_STRM_TYPE_H263 == video_codec) {

        b = MP4ReadSample(mp4File, video_trId, 1/*sample_id=1*/,
                          &p_video_sample, &n_video_sample,
                          NULL, NULL, NULL, NULL);

        ret = get_h263_info(p_video_sample, n_video_sample, profile, width, height);
        free(p_video_sample);
    }
    else if (RAW_STRM_TYPE_M4V == video_codec) {
        strm_hdr_leng = sizeof(strm_hdr_buf);
        ret = GetVideoStreamHeader(mp4File, video_trId, video_codec,
                                   strm_hdr_buf, &strm_hdr_leng);

        // Profile and size information are directly obtained from the video stream header.
        ret = get_mpeg4_info(strm_hdr_buf, strm_hdr_leng, profile, width, height);
    }
    else if (RAW_STRM_TYPE_H264RAW == video_codec) {
        strm_hdr_leng = sizeof(strm_hdr_buf);
        ret = GetVideoStreamHeader(mp4File, video_trId, video_codec,
                                   strm_hdr_buf, &strm_hdr_leng);
        // Profile and size information are directly obtained from the video stream header.
        ret = get_h264_info(strm_hdr_buf, strm_hdr_leng, profile, width, height);
    }


    return 0;
}

