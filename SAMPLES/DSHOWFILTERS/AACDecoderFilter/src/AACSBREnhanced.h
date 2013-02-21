// AACSBREnhanced.h: interface for the CAACSBREnhanced class.
// 
//////////////////////////////////////////////////////////////////////
//  CAACSBREnhanced: (A DirectShow (DirectX 8.0) filter)
//
//  Purpose: This filter is a basic Transform Filter to process video
//        
//  Usage: 
//  1.  Register the filter. 
//      regsvr32 AACSBREnhanced.ax
//
//  2.  Insert the filter into the graph to process video data (live 
//      capture or video files)
//      a.  Use the GraphEdt.exe (in DirectX SDK) to build a graph 
//      b.  To use this filter in App, include iAACSBREnhanced.h for
//          definition of interfaces and build the graph in the App.
///////////////////////////////////////////////////////////////////////
#pragma once

class CAACSBREnhanced
    : public CTransformFilter     // Derive from Transform Filter
     
{
public:

    DECLARE_IUNKNOWN;

// Constructor && Deconstructor
    static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    CAACSBREnhanced(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CAACSBREnhanced();

// Overrriden from CTransformFilter base class
    // Check if the transform type can be accomplished
    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                 ALLOCATOR_PROPERTIES *pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    // This function is called to process each new frame in the video.
    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);

    HRESULT Stop();
    HRESULT AACExit();
    STDMETHODIMP Initialize_Decoder(int inpBufSize, BYTE* buffer, int buffer_size, int *bytes_consumed);
 

protected:

    // Critical Section (used for multi-thread share)
    CCritSec m_AACSBREnhancedLock; 
};
