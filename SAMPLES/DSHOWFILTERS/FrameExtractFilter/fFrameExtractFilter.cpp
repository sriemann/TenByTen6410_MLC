//------------------------------------------------------------------------------
// File: fFrameExtractFilter.cpp
//
// Desc: implement CFrameExtractFilter class.
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "FrameExtractFilter.h"
#include "fFrameExtractFilter.h"
#include "fFrameExtractFilter_op.h"

#include <stdio.h>
#include "FrameExtractor.h"
#include "H264Frames.h"
#include "H263Frames.h"
#include "MPEG4Frames.h"
#include "VC1Frames.h"


static unsigned char delimiter_H264[]  = {0x00, 0x00, 0x00, 0x01}; // H.264
static unsigned char delimiter_MPEG4[] = {0x00, 0x00, 0x01}; //MPEG4


#include "FileRead.h"
#ifndef DATAREAD_TYPE
#error "DATAREAD_TYPE is not defined."
#endif
#if (DATAREAD_TYPE == DATA_FILE)
    #define FRAMEX_IN_TYPE_SEL    (FRAMEX_IN_TYPE_FILE)
#else
    #define FRAMEX_IN_TYPE_SEL    (FRAMEX_IN_TYPE_MEM)
#endif


//
// Constructor
//
CFrameExtractFilter::CFrameExtractFilter(LPUNKNOWN pUnk, HRESULT *phr)
    : CSource(NAME("CFrameExtractFilter"), pUnk, CLSID_FrameExtractFilter),
    m_pwFileName(NULL), m_File(NULL), m_pFramexCtx(NULL)
{
    CAutoLock cAutoLock(&m_cStateLock);

    //
    // CSource::AddPin() is automatically called
    // by creating a source stream (CSourceStream)
    //

    // Add one source stream (output pin)!
    new CFrameExtractFilterStream(phr, this, L"Out");

    m_first = 1;
}


//
// Destructor
//
CFrameExtractFilter::~CFrameExtractFilter()
{
    // Close file
    if (m_File)
        SSB_FILE_CLOSE(m_File);

    if (m_pwFileName)
        delete[] m_pwFileName;

    if (m_pFramexCtx)
        FrameExtractorFinal((FRAMEX_CTX *) m_pFramexCtx);
}


//
// Load
//
STDMETHODIMP CFrameExtractFilter::Load(LPCOLESTR pwszFileName, const AM_MEDIA_TYPE *pmt)
{
    // IFileSourceFilter::Load()
    // Any calls to this method after the first call will fail!

    if (m_pwFileName)
        return E_FAIL;

    CheckPointer(pwszFileName, E_POINTER);

    // lstrlenW is one of the few Unicode functions that works on win95
    DWORD dwSize = lstrlenW(pwszFileName) + 1;

    TCHAR pszFileName[MAX_PATH] = {0};
    lstrcpy(pszFileName, pwszFileName);

    CAutoLock lck(&m_cStateLock);

    // file checking
    HRESULT hr = CheckFile(pszFileName, pmt);
    
    if (FAILED(hr))
        return E_FAIL;

    // Restore the current file name
    m_pwFileName = new WCHAR[dwSize];

    if (m_pwFileName != NULL)
        CopyMemory(m_pwFileName, pwszFileName, dwSize * sizeof(WCHAR));

    return S_OK;
}


//
// GetCurFile
//
STDMETHODIMP CFrameExtractFilter::GetCurFile(LPOLESTR *ppwszFileName, AM_MEDIA_TYPE *pmt)
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

    if (pmt != NULL) 
    {
        CopyMediaType(pmt, &m_mt);
    }

    return S_OK;
}


int CFrameExtractFilter::GetNextFrame(unsigned char *pFrameBuf, unsigned int *pSize)
{
    int nRead;


    // 처음 호출되는 경우는 config stream을 리턴한다.
    if (m_first) {
        switch (m_StrmType) {
        case RAW_STRM_TYPE_M4V:
            m_pFramexCtx = FrameExtractorInit(FRAMEX_IN_TYPE_SEL, delimiter_MPEG4, 3, 1);
            FrameExtractorFirst((FRAMEX_CTX *) m_pFramexCtx, m_File);
            nRead = ExtractConfigStreamMpeg4((FRAMEX_CTX *) m_pFramexCtx, m_File, pFrameBuf, *pSize, NULL);
            break;

        case RAW_STRM_TYPE_H264RAW:
            m_pFramexCtx = FrameExtractorInit(FRAMEX_IN_TYPE_SEL, delimiter_H264, 4, 1);
            FrameExtractorFirst((FRAMEX_CTX *) m_pFramexCtx, m_File);
            nRead = ExtractConfigStreamH264((FRAMEX_CTX *) m_pFramexCtx, m_File, pFrameBuf, *pSize, NULL);
            break;

        case RAW_STRM_TYPE_RCV:
            nRead = ExtractConfigStreamVC1(m_File, pFrameBuf, *pSize, NULL);
            break;

        default:
            return -1;
        }

        m_first = 0;

        return nRead;
    }

    switch (m_StrmType) {
    case RAW_STRM_TYPE_M4V:
        nRead = NextFrameMpeg4((FRAMEX_CTX *) m_pFramexCtx, (FILE *) m_File, pFrameBuf, *pSize, NULL);
        break;

    case RAW_STRM_TYPE_H264RAW:
        nRead = NextFrameH264((FRAMEX_CTX *) m_pFramexCtx, (FILE *) m_File, pFrameBuf, *pSize, NULL);
        break;

    case RAW_STRM_TYPE_RCV:
        nRead = NextFrameVC1(m_File, pFrameBuf, *pSize, NULL);
        break;

    default:
        return -1;
    }
    
    if (nRead <= 5)
        return -1;

    return nRead;
}


//
// ReadTheFile
//
HRESULT CFrameExtractFilter::CheckFile(LPCTSTR pszFileName, const AM_MEDIA_TYPE *pmt)
{
    TCHAR         file_ext[4];

    int           nRead;
    unsigned int  width, height;
    union {
        MPEG4_CONFIG_DATA mpeg4_conf;
        H263_CONFIG_DATA  h263_conf;
        H264_CONFIG_DATA  h264_conf;
        VC1_CONFIG_DATA   vc1_conf;
    } u;

    unsigned char *p_confbuf;


#define FILE_EXT_M4V    L".m4v"
#define FILE_EXT_H264    L".264"
#define FILE_EXT_RCV    L".rcv"

    if (wcslen(pszFileName) > 4)
        wcsncpy(file_ext, pszFileName + (wcslen(pszFileName) - 4), 4);
    else
        file_ext[0] = 0;

    // Identify the codec type according to the filename extention
    if(_wcsnicmp(file_ext, FILE_EXT_M4V, 4) == 0)
        m_StrmType = RAW_STRM_TYPE_M4V;
    else if(_wcsnicmp(file_ext, FILE_EXT_H264, 4) == 0)
        m_StrmType = RAW_STRM_TYPE_H264RAW;
    else if(_wcsnicmp(file_ext, FILE_EXT_RCV, 4) == 0)
        m_StrmType = RAW_STRM_TYPE_RCV;
    else{
        RETAILMSG(1, (L"[FrameExtractFilter]unsupported file type(%s)\n", pszFileName));
        return E_FAIL;
    }

    // Open file
    m_File = SSB_FILE_OPEN(pszFileName);
    if (m_File == NULL){
        RETAILMSG(1, (L"[FrameExtractFilter]file(%s) doesn't exist\n", pszFileName));
        return E_FAIL;
    }

#define CONFIG_STRM_LENG_MAX    (128 * 1024)    // 128 KB
    p_confbuf = (unsigned char *) malloc(CONFIG_STRM_LENG_MAX);
    if (p_confbuf == NULL) {
        RETAILMSG(1, (L"[FrameExtractFilter] memory allocation for CONFIG_STRM failed.\n"));
        SSB_FILE_CLOSE(m_File);
        return E_FAIL;
    }
    // Obtaininig Width, Height
    memset(&u, 0, sizeof(u));
    switch (m_StrmType) {
    case RAW_STRM_TYPE_M4V:
        m_pFramexCtx = FrameExtractorInit(FRAMEX_IN_TYPE_SEL, delimiter_MPEG4, 3, 1);
        FrameExtractorFirst((FRAMEX_CTX *) m_pFramexCtx, m_File);
        nRead = ExtractConfigStreamMpeg4((FRAMEX_CTX *) m_pFramexCtx, m_File, p_confbuf, CONFIG_STRM_LENG_MAX, &(u.mpeg4_conf));
        FrameExtractorFinal((FRAMEX_CTX *) m_pFramexCtx);
        m_pFramexCtx = NULL;

        width  = u.mpeg4_conf.width;
        height = u.mpeg4_conf.height;
        break;

    case RAW_STRM_TYPE_H264RAW:
        m_pFramexCtx = FrameExtractorInit(FRAMEX_IN_TYPE_SEL, delimiter_H264, 4, 1);
        FrameExtractorFirst((FRAMEX_CTX *) m_pFramexCtx, m_File);
        nRead = ExtractConfigStreamH264((FRAMEX_CTX *) m_pFramexCtx, m_File, p_confbuf, CONFIG_STRM_LENG_MAX, &(u.h264_conf));
        FrameExtractorFinal((FRAMEX_CTX *) m_pFramexCtx);
        m_pFramexCtx = NULL;

        width  = u.h264_conf.width;
        height = u.h264_conf.height;
        break;

    case RAW_STRM_TYPE_RCV:
        nRead = ExtractConfigStreamVC1(m_File, p_confbuf, CONFIG_STRM_LENG_MAX, &(u.vc1_conf));

        width  = u.vc1_conf.width;
        height = u.vc1_conf.height;
        break;

    default:
        free(p_confbuf);
        SSB_FILE_CLOSE(m_File);
        return E_FAIL;
    }

    // Close file
    free(p_confbuf);
    SSB_FILE_CLOSE(m_File);


    // Validate the width & height value
    if ((width > 720) || (width < 16) || (height > 576) || (height < 16)) {
        RETAILMSG(1, (L"[FrameExtractFilter] video size is too big. (width=%d, height=%d)\n", width, height));
        return E_FAIL;
    }

    // Open file
    m_File = SSB_FILE_OPEN(pszFileName);
    if (m_File == NULL){
        RETAILMSG(1, (L"[FrameExtractFilter]file(%s) doesn't exist\n", pszFileName));
        return E_FAIL;
    }


    // Setting MediaType
    CMediaType mt;

    if (pmt == NULL)
    {
        if (FAILED(MakeMediaType(&mt, width, height)))
        {
            SSB_FILE_CLOSE(m_File);
            return E_FAIL;
        }
    }
    else
        mt = *pmt;

    // this is not a simple assignment... pointers and format
    // block (if any) are intelligently copied
    m_mt = mt;

    return S_OK;
}


//
// MakeMediaType
//
HRESULT CFrameExtractFilter::MakeMediaType(CMediaType *pMediaType, int width, int height)
{
    unsigned int  *size;

    pMediaType->InitMediaType();
    pMediaType->SetType(&MEDIATYPE_Stream);

    switch (m_StrmType) {
    case RAW_STRM_TYPE_M4V:
        pMediaType->SetSubtype(&MEDIASUBTYPE_m4v);
        break;

    case RAW_STRM_TYPE_H264RAW:
        pMediaType->SetSubtype(&MEDIASUBTYPE_h264raw);
        break;

    case RAW_STRM_TYPE_RCV:
        pMediaType->SetSubtype(&MEDIASUBTYPE_rcv);
        break;

    default:
        RETAILMSG(1, (L"[FrameExtractFilter]unsupported file type\n"));
        return E_FAIL;
    }

    size = (unsigned int *) pMediaType->ReallocFormatBuffer(sizeof(unsigned int) * 2);
    size[0] = width;
    size[1] = height;

    pMediaType->SetFormat((BYTE *)size, sizeof(unsigned int) * 2);

    return S_OK;
}

int CFrameExtractFilter::CloseFile()
{
    if (m_File) {
        SSB_FILE_CLOSE(m_File);
        m_File = NULL;
    }

    return 0;
}
