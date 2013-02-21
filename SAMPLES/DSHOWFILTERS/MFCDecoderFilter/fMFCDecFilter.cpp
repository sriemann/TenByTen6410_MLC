//------------------------------------------------------------------------------
// File: fMFCDecFilter.cpp
//
// Desc: implement CMFCDecFilter class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "MFCDecFilter.h"
#include "fMFCDecFilter.h"
#include "fMFCDecFilter_ip.h"
#include "fMFCDecFilter_op.h"

#include <windows.h>
#include <stdio.h>


#include "SsbSipMpeg4Decode.h"
#include "SsbSipH264Decode.h"
#include "SsbSipVC1Decode.h"

#define  YV12             1
#define  RGB565           2
#define  OUTPIN_TYPE      (YV12)


extern "C" void _initConvTab();
extern "C" void _yuv420ToRgb565(unsigned char *p_lum, unsigned char *p_cb, unsigned char *p_cr, int w_src, int h_src,
                                unsigned char *dest,  int w_dst, int h_dst,
                                int topdown);


//
// Constructor
//
CMFCDecFilter::CMFCDecFilter(LPUNKNOWN pUnk, HRESULT *phr)
    : CTransformFilter(NAME("MFC Decoder Filter"), pUnk, CLSID_MFCDecFilter)
{
    CAutoLock cAutoLock(&m_csFilter);

    if (SUCCEEDED(*phr))
    {
        // Create an output pin
        CMFCDecFilterOutputPin *pOut = new CMFCDecFilterOutputPin(phr, this, L"Out");
        if (pOut)
        {
            if (SUCCEEDED(*phr))
                m_pOutput = pOut;
            else
                delete pOut;
        }
        else
            *phr = E_OUTOFMEMORY;

        //
        // NOTE!: If we've created our own output pin we must also create
        // the input pin ourselves because the CTransformFilter base class 
        // will create an extra output pin if the input pin wasn't created.        
        //
        CMFCDecFilterInputPin *pIn = new CMFCDecFilterInputPin(phr, this, L"In");
        if (pIn)
        {
            if (SUCCEEDED(*phr))
                m_pInput = pIn;
            else
                delete pIn;
        }
        else
            *phr = E_OUTOFMEMORY;
    }

    this->m_MP4IsFirstTransform  = TRUE;
    this->m_H264IsFirstTransform = TRUE;
    this->m_VC1IsFirstTransform  = TRUE;
    this->m_WMV9sFirstTransform  = TRUE;

    this->decode_ctx             = NULL;


#if (OUTPIN_TYPE != YV12)
    _initConvTab();
#endif
}


//
// Destructor
//
CMFCDecFilter::~CMFCDecFilter()
{
    DecoderReset();
}


//
// Transform
//
HRESULT CMFCDecFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    HRESULT     hr;
    long        y_size, u_size;
    long        y_buf_size, u_buf_size;
    BYTE        *pDataOut, *p_FrmBuf, *p_OutBuf;
    int         buf_height, buf_width, i;

    // Copy IMediaSample data except for data
    hr = CopySample(pIn, pOut);
    if (FAILED(hr))
        return hr;

    hr = pOut->GetPointer(&pDataOut);
    if (FAILED(hr))
        return hr;

    if(m_Height % 16 != 0)
        buf_height = (m_Height/16 + 1) *16;
    else
        buf_height = m_Height;

    if(m_Width % 16 != 0)
        buf_width = (m_Width/16 + 1) *16;
    else
        buf_width = m_Width;

    y_size = m_Width * m_Height;
    u_size = y_size >> 2;

    y_buf_size = buf_width * buf_height;
    u_buf_size = y_buf_size >> 2;
    
#if (OUTPIN_TYPE == YV12)
    p_FrmBuf = m_pFrmBuf;
    p_OutBuf = pDataOut;
    for(i = 0; i < m_Height; i++){
        memcpy(p_OutBuf, p_FrmBuf, m_Width);
        p_OutBuf += m_Width;
        p_FrmBuf += buf_width;
    }

    p_FrmBuf = m_pFrmBuf + y_buf_size + u_buf_size;
    p_OutBuf = pDataOut + y_size;
    for(i = 0; i < m_Height/2; i++){
        memcpy(p_OutBuf, p_FrmBuf, m_Width/2);
        p_OutBuf += m_Width/2;
        p_FrmBuf += buf_width/2;
    }

    p_FrmBuf = m_pFrmBuf + y_buf_size;
    p_OutBuf = pDataOut + y_size + u_size;
    for(i = 0; i < m_Height/2; i++){
        memcpy(p_OutBuf, p_FrmBuf, m_Width/2);
        p_OutBuf += m_Width/2;
        p_FrmBuf += buf_width/2;
    }
    pOut->SetActualDataLength((y_size*3)>>1);
#else
    _yuv420ToRgb565(m_pFrmBuf, m_pFrmBuf + y_size, m_pFrmBuf + y_size + u_size, m_Width, m_Height,
                    pDataOut, m_Width, m_Height,
                    1);
    pOut->SetActualDataLength(y_size << 1);
#endif


    return hr;
}


//
// CheckInputType
//
HRESULT CMFCDecFilter::CheckInputType(const CMediaType *mtIn)
{
    unsigned int    *size;
    long            length =0;

    
    if((*mtIn->Type() != MEDIATYPE_Stream) && (*mtIn->Type() != MEDIATYPE_Video)&& (*mtIn->Type() != WMMEDIATYPE_test)) 
    {
        RETAILMSG(1,(L"[MFC Filter]CheckInputType fail \n"));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    
    if(mtIn->subtype == MEDIASUBTYPE_m4v){
        m_StrmType = RAW_STRM_TYPE_M4V;
    }else if(mtIn->subtype == MEDIASUBTYPE_h264raw){
        m_StrmType = RAW_STRM_TYPE_H264RAW;
    }else if(mtIn->subtype == MEDIASUBTYPE_rcv){
        m_StrmType = RAW_STRM_TYPE_RCV;
    }else if(mtIn->subtype ==WMMEDIASUBTYPE_WMV3){
        m_StrmType = VIDEO_STRM_TYPE_WMV9;
    }else if(mtIn->subtype ==WMMEDIASUBTYPE_wmv3){
        m_StrmType = VIDEO_STRM_TYPE_WMV9;
    }else{
        RETAILMSG(1,(L"\n[MFC Filter] Check MediaSubType again..\n"));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    size = (unsigned int *) mtIn->Format();
    length = mtIn->FormatLength();
    
    if(m_StrmType == VIDEO_STRM_TYPE_WMV9)
    {
        m_Width  = size[13];
        m_Height = size[14];
        m_Wmvdata = size[22];
    }
    else
    {
        m_Width = size[0];
        m_Height =size[1];
    }
    
    RETAILMSG(1, (L"\n{ MFC DECODER FILTER } CheckInputType OK, filetype = %d, size=(%d,%d)\n", m_StrmType, size[0], size[1]));

    return S_OK;
}


//
// CheckTransform
//
HRESULT CMFCDecFilter::CheckTransform(const CMediaType *pmtIn, const CMediaType *pmtOut)
{
    return S_OK;
}


//
// DecideBufferSize
//
HRESULT CMFCDecFilter::DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties)
{
    // First, the input pin has to be connected
    if (m_pInput->IsConnected() == FALSE)
        return E_UNEXPECTED;

    ASSERT(pAllocator);
    ASSERT(pProperties);

    HRESULT hr;

    // Pass the allocator requirements to our output side
    // but do a little sanity checking first
    pProperties->cBuffers  = 1;
    pProperties->cbPrefix  = 0;
#if (OUTPIN_TYPE == YV12)
    pProperties->cbBuffer  = ((m_Width*m_Height*3)>>1);
#else
    pProperties->cbBuffer  = ((m_Width*m_Height)<<1)+1;
#endif

    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties, &Actual);
    if (FAILED(hr)) 
    {
        RETAILMSG(1,(L"[MFC Filter]wmv9 DecideBufferSize  end fail 1.\n"));
        return hr;
    }

    // Make sure we got the right alignment and at least the minimum required
    if (pProperties->cBuffers > Actual.cBuffers ||
        pProperties->cbBuffer > Actual.cbBuffer ||
        pProperties->cbAlign > Actual.cbAlign) 
    {
        RETAILMSG(1,(L"[MFC Filter]wmv9 DecideBufferSize  end fail 2.\n"));
        return E_FAIL;
    }

    return S_OK;
}


//
// GetMediaType
//
HRESULT CMFCDecFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    DWORD    *pDwBitMask;

    if (m_pInput->IsConnected() == FALSE)
    {
        return E_UNEXPECTED;
    }
    if (iPosition < 0) 
    {
        return E_INVALIDARG;
    }

    VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER) + sizeof(DWORD)*3);
    if (vih == NULL) 
    {
        RETAILMSG(STATUS, (TEXT("\n[MFC Filter]VIDEOINFOHEADER:E_OUTOFMEMORY\n")));
        return E_OUTOFMEMORY;
    }
    pDwBitMask = (DWORD *)((BYTE *)vih + sizeof(VIDEOINFOHEADER));
    ZeroMemory(vih, sizeof (VIDEOINFOHEADER) + sizeof(DWORD)*3);

    pMediaType->SetFormatType(&FORMAT_VideoInfo);
    vih->bmiHeader.biSize    = sizeof(BITMAPINFOHEADER);

    vih->bmiHeader.biWidth    = m_Width;    //Width of the image
    vih->bmiHeader.biHeight = m_Height;    //Height of image
    vih->bmiHeader.biPlanes = 1;

    switch(iPosition)
    {
#if (OUTPIN_TYPE == YV12)
    case 0:
        vih->bmiHeader.biCompression= MEDIASUBTYPE_YV12.Data1;
        vih->bmiHeader.biBitCount    = 12;        //Bits/Pixel
        pMediaType->SetSubtype(&MEDIASUBTYPE_YV12);
        vih->bmiHeader.biSizeImage = GetBitmapSize(&vih->bmiHeader);
        break;
#else
    case 0:
        vih->bmiHeader.biCompression= BI_BITFIELDS;
        vih->bmiHeader.biBitCount    = 16;        //Bits/Pixel
        pMediaType->SetSubtype(&MEDIASUBTYPE_RGB565);
        pDwBitMask[0] = 0x0000f800;
        pDwBitMask[1] = 0x000007e0;
        pDwBitMask[2] = 0x0000001f;
        vih->bmiHeader.biSizeImage = GetBitmapSize(&vih->bmiHeader);
        break;
#endif
    default:
        RETAILMSG(STATUS, (TEXT("\n[MFC Filter]GetMediaType:Position\n")));
        return VFW_S_NO_MORE_ITEMS;
    }

    pMediaType->SetType(&MEDIATYPE_Video);
    pMediaType->SetTemporalCompression(FALSE);
    pMediaType->SetSampleSize(vih->bmiHeader.biSizeImage);
    pMediaType->SetFormat((BYTE *)vih, sizeof(VIDEOINFOHEADER) + sizeof(DWORD)*3);//cc
    return S_OK;
}


//
// When Stream ends
//
HRESULT CMFCDecFilter::StopStreaming()
{
//    DecoderReset();

    return CTransformFilter::StopStreaming();
}


//
// Copy
//
HRESULT CMFCDecFilter::CopySample(IMediaSample *pSource, IMediaSample *pDest)
{
    // Copy the sample times
    REFERENCE_TIME TimeStart, TimeEnd;
    if (NOERROR == pSource->GetTime(&TimeStart, &TimeEnd))
        pDest->SetTime(&TimeStart, &TimeEnd);

    LONGLONG MediaStart, MediaEnd;
    if (pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
        pDest->SetMediaTime(&MediaStart,&MediaEnd);

    // Copy the Sync point property
    HRESULT hr = pSource->IsSyncPoint();
    if (hr == S_OK)
        pDest->SetSyncPoint(TRUE);
    else if (hr == S_FALSE)
        pDest->SetSyncPoint(FALSE);
    else    // an unexpected error has occured...
        return E_UNEXPECTED;

    // Copy the media type
    AM_MEDIA_TYPE *pMediaType;
    pSource->GetMediaType(&pMediaType);
    pDest->SetMediaType(pMediaType);
    DeleteMediaType(pMediaType);

    // Copy the preroll property
    hr = pSource->IsPreroll();
    if (hr == S_OK)
        pDest->SetPreroll(TRUE);
    else if (hr == S_FALSE)
        pDest->SetPreroll(FALSE);
    else    // an unexpected error has occured...
        return E_UNEXPECTED;

    // Copy the discontinuity property
    hr = pSource->IsDiscontinuity();
    if (hr == S_OK)
        pDest->SetDiscontinuity(TRUE);
    else if (hr == S_FALSE)
        pDest->SetDiscontinuity(FALSE);
    else    // an unexpected error has occured...
        return E_UNEXPECTED;

    return S_OK;
}


//
// H264Transform
//
HRESULT CMFCDecFilter::MP4Decode(IMediaSample* pSample)
{
    BYTE                        *pDataIn;
    HRESULT                        hr;
    unsigned int                    dwBytesToRead;
    BYTE                        *pStrmBuf;
    SSBSIP_MPEG4_STREAM_INFO    stream_info;


    hr = pSample->GetPointer(&pDataIn);
    if (FAILED(hr))
        return hr;

    dwBytesToRead = pSample->GetActualDataLength();

    ////////////////////////////////////////////////
    // 1. Decoder init                              //
    ////////////////////////////////////////////////
    if(m_MP4IsFirstTransform) {
        decode_ctx = SsbSipMPEG4DecodeInit();
        if (decode_ctx == NULL) {
            RETAILMSG(1,(L"[MFC Filter]SsbSipMPEG4DecodeInit Failed.\n"));
            return S_FALSE;

        }
        
        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipMPEG4DecodeGetInBuf(decode_ctx, dwBytesToRead);
        memcpy(pStrmBuf, pDataIn, dwBytesToRead);

        ////////////////////////////////////////////////
        // 3. Decoder Init                              //
        ////////////////////////////////////////////////
        if (SsbSipMPEG4DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_MPEG4_DEC_RET_OK) {
            RETAILMSG(1,(L"MPEG4 Decoder Configuration Failed.\n"));
            return S_FALSE;
        }

        /////////////////////////////////////
        ///   4. Get stream information   ///
        /////////////////////////////////////
        SsbSipMPEG4DecodeGetConfig(decode_ctx, MPEG4_DEC_GETCONF_STREAMINFO, &stream_info);
        m_Width  = stream_info.width;
        m_Height = stream_info.height;

        m_MP4IsFirstTransform = FALSE;
    }
    else {
        if (decode_ctx == NULL) {
            return S_FALSE;
        }

        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipMPEG4DecodeGetInBuf(decode_ctx, dwBytesToRead);
        memcpy(pStrmBuf, pDataIn, dwBytesToRead);
    }

    ////////////////////////////////////////////////
    // 3. Decode Frames                              //
    ////////////////////////////////////////////////
    if (SsbSipMPEG4DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_MPEG4_DEC_RET_OK) {
        RETAILMSG(1,(L"MPEG4 Decoding Failed.\n"));
        return S_FALSE;
    }


    /////////////////////////////////////
    ///   4. Get YUV data   ///
    /////////////////////////////////////
    m_pFrmBuf = (BYTE*) SsbSipMPEG4DecodeGetOutBuf(decode_ctx, &m_FrmSize);

    return S_OK;
}


//
// H264Transform
//
HRESULT CMFCDecFilter::H264Decode(IMediaSample* pSample)
{
    BYTE                        *pDataIn;
    HRESULT                        hr;
    unsigned int                    dwBytesToRead;
    BYTE                        *pStrmBuf;
    SSBSIP_H264_STREAM_INFO        stream_info;


    hr = pSample->GetPointer(&pDataIn);
    if (FAILED(hr))
        return hr;

    dwBytesToRead = pSample->GetActualDataLength();


    ////////////////////////////////////////////////
    // 1. Decoder init                              //
    ////////////////////////////////////////////////
    if(m_H264IsFirstTransform) {

        decode_ctx = SsbSipH264DecodeInit();
        if (decode_ctx == NULL) {
            RETAILMSG(1,(L"[MFC Filter] SsbSipH264DecodeInit Failed.\n"));
            return S_FALSE;

        }

        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipH264DecodeGetInBuf(decode_ctx, dwBytesToRead);
        memcpy(pStrmBuf, pDataIn, dwBytesToRead);
        pStrmBuf += dwBytesToRead;
        pStrmBuf[0] = 0x00;
        pStrmBuf[1] = 0x00;
        pStrmBuf[2] = 0x00;
        pStrmBuf[3] = 0x01;

        ////////////////////////////////////////////////
        // 3. Decoder Init                              //
        ////////////////////////////////////////////////
        if (SsbSipH264DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_H264_DEC_RET_OK) {
            RETAILMSG(1,(L"H.264 Decoder Configuration Failed.\n"));
            return S_FALSE;
        }

        /////////////////////////////////////
        ///   4. Get stream information   ///
        /////////////////////////////////////
        SsbSipH264DecodeGetConfig(decode_ctx, H264_DEC_GETCONF_STREAMINFO, &stream_info);
        m_Width  = stream_info.width;
        m_Height = stream_info.height;
    
        m_H264IsFirstTransform = FALSE;
    }
    else {
        if (decode_ctx == NULL) {
            return S_FALSE;
        }

        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipH264DecodeGetInBuf(decode_ctx, dwBytesToRead);
        memcpy(pStrmBuf, pDataIn, dwBytesToRead);
        pStrmBuf += dwBytesToRead;
        pStrmBuf[0] = 0x00;
        pStrmBuf[1] = 0x00;
        pStrmBuf[2] = 0x00;
        pStrmBuf[3] = 0x01;
    }

    ////////////////////////////////////////////////
    // 3. Decode Frames                              //
    ////////////////////////////////////////////////
    if (SsbSipH264DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_H264_DEC_RET_OK) {
        RETAILMSG(1,(L"H.264 Decoding Failed.\n"));
        return S_FALSE;
    }

    /////////////////////////////////////
    ///   5. Get YUV data   ///
    /////////////////////////////////////
    m_pFrmBuf = (BYTE*) SsbSipH264DecodeGetOutBuf(decode_ctx, &m_FrmSize);

    return S_OK;
}


//
// VC1Decode
//
HRESULT CMFCDecFilter::VC1Decode(IMediaSample* pSample)
{
    BYTE                        *pDataIn;
    HRESULT                        hr;
    unsigned int                    dwBytesToRead;
    BYTE                        *pStrmBuf;
    SSBSIP_H264_STREAM_INFO        stream_info;


    hr = pSample->GetPointer(&pDataIn);
    if (FAILED(hr))
        return hr;

    dwBytesToRead = pSample->GetActualDataLength();

    ////////////////////////////////////////////////
    // 1. Decoder init                              //
    ////////////////////////////////////////////////
    if(m_VC1IsFirstTransform) {

        decode_ctx = SsbSipVC1DecodeInit();
        if (decode_ctx == NULL) {
            RETAILMSG(1,(L"[MFC Filter] SsbSipVC1DecodeInit Failed.\n"));
            return S_FALSE;

        }

        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipVC1DecodeGetInBuf(decode_ctx, dwBytesToRead);
        memcpy(pStrmBuf, pDataIn, dwBytesToRead);
        pStrmBuf += dwBytesToRead;
        pStrmBuf[0] = 0x00;
        pStrmBuf[1] = 0x00;
        pStrmBuf[2] = 0x00;
        pStrmBuf[3] = 0x01;

        ////////////////////////////////////////////////
        // 3. Decoder Init                              //
        ////////////////////////////////////////////////
        if (SsbSipVC1DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_H264_DEC_RET_OK) {
            RETAILMSG(1,(L"VC-1Decoder Configuration Failed.\n"));
            return S_FALSE;
        }


        /////////////////////////////////////
        ///   4. Get stream information   ///
        /////////////////////////////////////
        SsbSipVC1DecodeGetConfig(decode_ctx, H264_DEC_GETCONF_STREAMINFO, &stream_info);
        m_Width = stream_info.width;
        m_Height = stream_info.height;

        m_VC1IsFirstTransform = FALSE;
    }
    else {
        if (decode_ctx == NULL) {
            return S_FALSE;
        }

        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipVC1DecodeGetInBuf(decode_ctx, dwBytesToRead);
        memcpy(pStrmBuf, pDataIn, dwBytesToRead);
        pStrmBuf += dwBytesToRead;
        pStrmBuf[0] = 0x00;
        pStrmBuf[1] = 0x00;
        pStrmBuf[2] = 0x00;
        pStrmBuf[3] = 0x01;
    }

    ////////////////////////////////////////////////
    // 3. Decode Frames                              //
    ////////////////////////////////////////////////
    if (SsbSipVC1DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_H264_DEC_RET_OK) {
        RETAILMSG(1,(L"VC-1 Decoding Failed.\n"));
        return S_FALSE;
    }

    /////////////////////////////////////
    ///   5. Get YUV data   ///
    /////////////////////////////////////
    m_pFrmBuf = (BYTE*) SsbSipVC1DecodeGetOutBuf(decode_ctx, &m_FrmSize);
    
    return S_OK;
}

// WMV9 Transform
//
HRESULT CMFCDecFilter::WMV9Decode(IMediaSample* pSample)
{
    
    BYTE                        *pDataIn;
    HRESULT                      hr;
    unsigned int                 dwBytesToRead, index=0;
    BYTE                        *pStrmBuf;

    SSBSIP_H264_STREAM_INFO      stream_info;
        

    hr = pSample->GetPointer(&pDataIn);
    if (FAILED(hr))
        return hr;


    dwBytesToRead = pSample->GetActualDataLength();

    ////////////////////////////////////////////////
    // 1. Decoder init                              //
    ////////////////////////////////////////////////
    if(m_WMV9sFirstTransform) {
        decode_ctx = SsbSipVC1DecodeInit();
        if (decode_ctx == NULL) {
            RETAILMSG(0,(L"[MFC Filter] wmvDecodeInit Failed.\n"));
            return 0;

        }
        
        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipVC1DecodeGetInBuf(decode_ctx, dwBytesToRead);

        
        index =0;
        
        //indefnite stream
        pStrmBuf[index]   = 0xFF;
        pStrmBuf[index+1] = 0xFF ;
        pStrmBuf[index+2] = 0xFF ;
        pStrmBuf[index+3] = 0xc5;
        index+=4;

        pStrmBuf[index]   = 0x04;
        pStrmBuf[index+1] = 0x00;
        pStrmBuf[index+2] = 0x00;
        pStrmBuf[index+3] = 0x00;
        index+=4;
        
        //struct_c

        pStrmBuf[index]   = m_Wmvdata & 0xFF ;
        pStrmBuf[index+1] = (m_Wmvdata >>8)& 0xFF ;
        pStrmBuf[index+2] = (m_Wmvdata>>16) & 0xFF;
        pStrmBuf[index+3] = (m_Wmvdata>>24) & 0xFF;
        index+=4;
        
        //struct_a_vert
        
        pStrmBuf[index]      = m_Height & 0xFF ;
        pStrmBuf[index+1] = (m_Height>>8) & 0xFF ;
        pStrmBuf[index+2] = (m_Height>>16) & 0xFF;
        pStrmBuf[index+3] = (m_Height>>24) & 0xFF;
        index+=4;
        
        //struct_a_horz
        
        pStrmBuf[index]   = m_Width & 0xFF ;
        pStrmBuf[index+1] = (m_Width>>8) & 0xFF ;
        pStrmBuf[index+2] = (m_Width>>16) & 0xFF;
        pStrmBuf[index+3] = (m_Width>>24) & 0xFF;
        index+=4;
        
        //const_0C
        pStrmBuf[index]   = 0x0C;
        pStrmBuf[index+1] = 0x00;
        pStrmBuf[index+2] = 0x00;
        pStrmBuf[index+3] = 0x00;
        index+=4;
        
        //struct_b_1
        pStrmBuf[index]   = 0x00;
        pStrmBuf[index+1] = 0x00;
        pStrmBuf[index+2] = 0x00;
        pStrmBuf[index+3] = 0x00;
        index+=4;
        
        //struct_b_2
        pStrmBuf[index]   = 0x00;
        pStrmBuf[index+1] = 0x00;
        pStrmBuf[index+2] = 0x00;
        pStrmBuf[index+3] = 0x00;
        index+=4;
        
        //struct_b_3
        pStrmBuf[index]   = 0x00;
        pStrmBuf[index+1] = 0x00;
        pStrmBuf[index+2] = 0x00;
        pStrmBuf[index+3] = 0x00;
        index+=4;
        
        //pStrmBuf +=index;
        
        //frame data
        //index;
        pStrmBuf[index]   = dwBytesToRead & 0xFF;
        pStrmBuf[index+1] = (dwBytesToRead>>8) & 0xFF;
        pStrmBuf[index+2] = (dwBytesToRead>>16) & 0xFF;
        pStrmBuf[index+3] = 0x80;
        index+=4;

        pStrmBuf[index]   = 0x00;
        pStrmBuf[index+1] = 0x00;
        pStrmBuf[index+2] = 0x00;
        pStrmBuf[index+3] = 0x00;
        index+=4;
        

        pStrmBuf +=index;

        memcpy(pStrmBuf, pDataIn, dwBytesToRead);
        pStrmBuf += dwBytesToRead;


        
        ////////////////////////////////////////////////
        // 3. Decoder Init                              //
        ////////////////////////////////////////////////
        if (SsbSipVC1DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_H264_DEC_RET_OK) {
            RETAILMSG(1,(L"WMV9 Decoder decoding 1 Failed.\n"));
            return 0;
        }

        /////////////////////////////////////
        ///   4. Get stream information   ///
        /////////////////////////////////////
        SsbSipVC1DecodeGetConfig(decode_ctx, H264_DEC_GETCONF_STREAMINFO, &stream_info);
        m_Width = stream_info.width;
        m_Height = stream_info.height;
        
        m_WMV9sFirstTransform = FALSE;
    }
    else {
        if (decode_ctx == NULL) {
            return S_FALSE;
        }

        ////////////////////////////////////////////////
        // 2. Data copy                                  //
        ////////////////////////////////////////////////
        pStrmBuf = (BYTE*) SsbSipVC1DecodeGetInBuf(decode_ctx, dwBytesToRead);

        pStrmBuf[index]   = dwBytesToRead & 0xFF;
        pStrmBuf[index+1] = (dwBytesToRead>>8) & 0xFF;
        pStrmBuf[index+2] = (dwBytesToRead>>16) & 0xFF;
        pStrmBuf[index+3] = 0x00;
        index+=4;

        pStrmBuf[index]   = 0x00;
        pStrmBuf[index+1] = 0x00;
        pStrmBuf[index+2] = 0x00;
        pStrmBuf[index+3] = 0x00;
        index+=4;

        pStrmBuf +=8;
        memcpy(pStrmBuf, pDataIn, dwBytesToRead);
    
        pStrmBuf += dwBytesToRead;
    }

    ////////////////////////////////////////////////
    // 3. Decode Frames                              //
    ////////////////////////////////////////////////
    if (SsbSipVC1DecodeExe(decode_ctx, dwBytesToRead) != SSBSIP_H264_DEC_RET_OK) {
        RETAILMSG(1,(L"WMV9 Decoder decoding  Failed.\n"));
        return 0;
    }

    /////////////////////////////////////
    ///   5. Get YUV data   ///
    /////////////////////////////////////
    m_pFrmBuf = (BYTE*) SsbSipVC1DecodeGetOutBuf(decode_ctx, &m_FrmSize);
    
    return S_OK;
}




HRESULT CMFCDecFilter::Receive(IMediaSample* pSample)
{
    HRESULT                hr;

    switch (m_StrmType) {
        case RAW_STRM_TYPE_M4V:
            hr = MP4Decode(pSample);
            break;

        case RAW_STRM_TYPE_H264RAW:
            hr = H264Decode(pSample);
            break;

        case RAW_STRM_TYPE_RCV:
            hr = VC1Decode(pSample);
            break;
            
        case VIDEO_STRM_TYPE_WMV9:
            hr = WMV9Decode(pSample);
            break;

        default:
            RETAILMSG(1, (L"[MFC Filter]unsupported file type\n"));
            return E_FAIL;
    }

    if (FAILED(hr))
        return hr;

    return CTransformFilter::Receive(pSample);
}


void CMFCDecFilter::DecoderReset()
{
    if((m_StrmType == RAW_STRM_TYPE_M4V) && (decode_ctx != NULL))
    {
        SsbSipMPEG4DecodeDeInit(decode_ctx);
        this->decode_ctx = NULL;
        this->m_MP4IsFirstTransform = TRUE;
    }
    else if((m_StrmType == RAW_STRM_TYPE_H264RAW) && (decode_ctx != NULL))
    {
        SsbSipH264DecodeDeInit(decode_ctx);
        this->decode_ctx = NULL;
        this->m_H264IsFirstTransform = TRUE;
    }
    else if((m_StrmType == RAW_STRM_TYPE_RCV) && (decode_ctx != NULL))
    {
        SsbSipVC1DecodeDeInit(decode_ctx);
        this->decode_ctx = NULL;
        this->m_VC1IsFirstTransform = TRUE;
    }
    else if((m_StrmType == VIDEO_STRM_TYPE_WMV9) && (decode_ctx != NULL))
    {
        SsbSipVC1DecodeDeInit(decode_ctx);
        this->decode_ctx = NULL;
        this->m_WMV9sFirstTransform = TRUE;
    }
}

