// AACSBREnhanced.cpp: implementation of CAACSBREnhanced class.
// 
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <streams.h>     // DirectShow
#include <initguid.h>    // DEFINE_GUID to declare an EXTERN_C const.
#include <stdio.h>
#include <mmreg.h>
#include "iAACSBREnhanced.h"
#include "AACSBREnhanced.h"

#define err_AAC 1
#include "SsbTypeDefs.h"
#include "SsbSsipAudioAacDecoder.h"

#define ENABLE        1
#define DISABLE       0

#ifdef AAC_ENABLE_SBR
#define SBR_MUL 2
#ifdef PS_DEC
#define PS_MUL 2
#endif
#else
#define SBR_MUL 1
#define PS_MUL 1  
#endif

#define AAC_MIN_BUF_SIZE    (768*4)
#define WAVE_HEADER            ENABLE
#define OUTPUT_FORMAT        FMT_16BIT
#define MAXFRAMESIZE        500
#define MIN_STREAMSIZE      768*8

#define AAC_MAX_NCHANS        2        
#define AAC_MAX_NSAMPS        1024
#define OUTBUFSIZE        (AAC_MAX_NCHANS * AAC_MAX_NSAMPS * SBR_MUL)
#define AAC_MAINBUF_SIZE    (768 * AAC_MAX_NCHANS * 2)
#define READBUF_SIZE    (2 * AAC_MAINBUF_SIZE * AAC_MAX_NCHANS)    


int first_time=1;
unsigned char * InBuf=NULL;
void* OutBuf=NULL;
SAACDecoder *AACDecoder=NULL;
int bytes_left=0;
 int uiSampleRate=48000, uiChannels=2;
unsigned int bitsPerSample=16;
int stopped_once=0;
SsbSipAudioAacDecoderConfig_t config;
BYTE* param=NULL;
int param_size;

#define TRANSFORM_NAME L"AACSBREnhanced Filter"

// setup data - allows the self-registration to work.
const AMOVIESETUP_MEDIATYPE sudInputPinTypes =
{ &MEDIATYPE_Audio        // clsMajorType
, &MEDIASUBTYPE_EAACPLUS };  // clsMinorType

const AMOVIESETUP_MEDIATYPE sudOutputPinTypes =
{ &MEDIATYPE_Audio        // clsMajorType
, &MEDIASUBTYPE_PCM };  // clsMinorType

const AMOVIESETUP_PIN psudPins[] =
{ { L"Input"            // strName
  , FALSE               // bRendered
  , FALSE               // bOutput
  , FALSE               // bZero
  , FALSE               // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L"Output"                 // strConnectsToPin
  , 1                   // nTypes
  , &sudInputPinTypes       // lpTypes
  }
, { L"Output"           // strName
  , FALSE               // bRendered
  , TRUE                // bOutput
  , FALSE               // bZero
  , FALSE               // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L"Input"                 // strConnectsToPin
  , 1                   // nTypes
  , &sudOutputPinTypes        // lpTypes
  }
};

const AMOVIESETUP_FILTER sudAACSBREnhanced =
{ &CLSID_AACSBREnhanced              // clsID
, TRANSFORM_NAME                    // strName
, MERIT_NORMAL                        // dwMerit
, 2                                 // nPins
, psudPins };                       // lpPin

// Needed for the CreateInstance mechanism
CFactoryTemplate g_Templates[]=
    {   { TRANSFORM_NAME
        , &CLSID_AACSBREnhanced
        , CAACSBREnhanced::CreateInstance
        , NULL
        , &sudAACSBREnhanced }
    };
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);

///////////////////////////////////////////////////////////////////////
// CreateInstance: for COM to create a CAACSBREnhanced object
///////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CAACSBREnhanced::CreateInstance(LPUNKNOWN punk, HRESULT *phr) 
{
        CAACSBREnhanced *pNewObject = new CAACSBREnhanced(NAME("AACSBREnhanced"), punk, phr );
        if (pNewObject == NULL) 
        {
            *phr = E_OUTOFMEMORY;
        }

        return pNewObject;
}

///////////////////////////////////////////////////////////////////////
// CAACSBREnhanced: Constructor
///////////////////////////////////////////////////////////////////////
CAACSBREnhanced::CAACSBREnhanced(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
    : CTransformFilter (tszName, punk, CLSID_AACSBREnhanced)
{
   stopped_once=0;
}

///////////////////////////////////////////////////////////////////////
// ~CAACSBREnhanced: Destructor
///////////////////////////////////////////////////////////////////////
CAACSBREnhanced::~CAACSBREnhanced()
{
    if(param)
        free(param);
    param=NULL;
}



///////////////////////////////////////////////////////////////////////
// CheckInputType: Check if the input type can be done
///////////////////////////////////////////////////////////////////////
HRESULT CAACSBREnhanced::CheckInputType(const CMediaType *mtIn)
{
    if (*mtIn->Type() != MEDIATYPE_Audio)
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }


    if (mtIn->subtype != MEDIASUBTYPE_EAACPLUS)
    {
        RETAILMSG(1,(L"\n[EAAC PLUS]Check MediaSubType again..\n"));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }


    WAVEFORMATEX* wfe = (WAVEFORMATEX*) mtIn->Format();


    if(wfe!=NULL)
    {
        uiSampleRate = wfe->nSamplesPerSec;
        uiChannels = wfe->nChannels;
        bitsPerSample = wfe->wBitsPerSample;

        

        if(uiChannels>2 || uiChannels<1)
        {
            param_size=0;
            uiChannels=2;
            uiSampleRate=48000;
            bitsPerSample=16;
            wfe=NULL;
        }

        if(wfe!=NULL)
        {
            if(param==NULL)
            {
                param = (BYTE*)malloc(wfe->cbSize + 1);
            }
            memcpy(param, wfe+1, wfe->cbSize);
            param_size = wfe->cbSize;

            Initialize_Decoder(2048,param, param_size, &bytes_left);
        }
        
    
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// Checktransform: Check a transform can be done between these formats
///////////////////////////////////////////////////////////////////////
HRESULT CAACSBREnhanced::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////
// DecideBufferSize: Tell the output pin's allocator what size buffers 
// we require. Can only do this when the input is connected
///////////////////////////////////////////////////////////////////////
HRESULT CAACSBREnhanced::DecideBufferSize(IMemAllocator *pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties)
{
    // Is the input pin connected
    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;


    pProperties->cBuffers = 1;

    // Get input pin's allocator size and use that
    ALLOCATOR_PROPERTIES InProps;
    IMemAllocator * pInAlloc = NULL;
    hr = m_pInput->GetAllocator(&pInAlloc);
    if (SUCCEEDED (hr))
    {
        hr = pInAlloc->GetProperties (&InProps);
        if (SUCCEEDED (hr))
        {
            pProperties->cbBuffer = 8*1024;
        }
        pInAlloc->Release();
    }

    if (FAILED(hr))
        return hr;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    ASSERT( Actual.cBuffers == 1 );

    if ( pProperties->cBuffers > Actual.cBuffers ||
         pProperties->cbBuffer > Actual.cbBuffer) {
                return E_FAIL;
    }

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////
// GetMediaType: I support one type, namely the type of the input pin
// This type is only available if my input is connected
///////////////////////////////////////////////////////////////////////
HRESULT CAACSBREnhanced::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    // Is the input pin connected
    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }
    // This should never happen
    if (iPosition < 0) {
        return E_INVALIDARG;
    }
    // Do we have more items to offer
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    WAVEFORMATEXTENSIBLE fmt;
    //WAVEFORMATEX fmt;
    ZeroMemory(&fmt, sizeof(WAVEFORMATEXTENSIBLE));

    fmt.Format.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.Format.nChannels       = uiChannels;
    fmt.Format.nSamplesPerSec  = uiSampleRate;
    fmt.Format.wBitsPerSample  = bitsPerSample;
    fmt.Format.nBlockAlign     = fmt.Format.wBitsPerSample / 8 * fmt.Format.nChannels;
    fmt.Format.nAvgBytesPerSec = fmt.Format.nSamplesPerSec *fmt.Format.nBlockAlign;
    fmt.Format.cbSize          = 0;

    //guru
    fmt.SubFormat = MEDIASUBTYPE_PCM;//KSDATAFORMAT_SUBTYPE_PCM;
    fmt.Samples.wValidBitsPerSample = 16;
    fmt.dwChannelMask = 0xff;

    *pMediaType = m_pInput->CurrentMediaType();
    //guru
    pMediaType->SetType(&MEDIATYPE_Audio);
    pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
    pMediaType->SetFormat((BYTE*)&fmt, sizeof(WAVEFORMATEX) + fmt.Format.cbSize);

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////
// Transform (for CTransformFitler)
//
// Copy the input sample into the output sample - then transform the 
// output sample 'in place'. If we have all keyframes, then we should
// not do a copy. If we have cinepak or indeo and are decompressing 
// frame N it needs frame decompressed frame N-1 available to calculate
// it, unless we are at a keyframe. So with keyframed codecs, you can't 
// get away with applying the transform to change the frames in place, 
// because you'll mess up the next frames decompression. The runtime 
// MPEG decoder does not have keyframes in the same way so it can be 
// done in place. We know if a sample is key frame as we transform 
// because the sync point property will be set on the sample
///////////////////////////////////////////////////////////////////////
HRESULT CAACSBREnhanced::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    unsigned char *pBufferIn;
    unsigned char *pBufferOut;
    SsbSipAudioAacDecoderFrameInfo_t frameInfo;
    AM_MEDIA_TYPE *mtout;
     int iBytesConsumed=0;
    unsigned int ActualOutLength=0;
    short End;
    unsigned int decoded_bytes=0;
    static int once=1;
    frameInfo.sbrEnabled=0;


    HRESULT hr;
#if 0
if (first_time == 1)
RETAILMSG(1,(L"\n[EAAC PLUS] Transform....\n"));

{
LONGLONG  tStart, tEnd;
pIn->GetMediaTime(&tStart, &tEnd);
printf("\n\t*** Time = %u, %u", (unsigned int) tStart, (unsigned int) tEnd);
}
#endif
    memset(&frameInfo,0,sizeof(SsbSipAudioAacDecoderFrameInfo_t));
    hr= pIn->GetPointer(&pBufferIn);
    if(FAILED(hr))
    {
        return E_FAIL;
    }

    hr= pOut->GetPointer(&pBufferOut);
    if(FAILED(hr))
    {
        return E_FAIL;
    }

     int srcBufLength= pIn->GetActualDataLength();

    if(param_size!=0)
    {

        if(first_time==1)
        {
            if(stopped_once==1)
            {
                Initialize_Decoder(2048,param, param_size,&bytes_left);
            }
            first_time=0;
            once=1;
        }
        memcpy(InBuf, pBufferIn, srcBufLength);
        End=AACDecode_Ittiam(AACDecoder,InBuf,OutBuf,&frameInfo);

        if(frameInfo.error)
            {
                RETAILMSG(1,(L"\nError in decoding : %d\n", frameInfo.error));
                pOut->SetActualDataLength(ActualOutLength);
                Stop();
            }
        if(frameInfo.sbrEnabled && once)
        {
            uiSampleRate *=2;
            once=0;
            CMediaType temp;

            GetMediaType(0,&temp);

            mtout=(AM_MEDIA_TYPE*)&temp;
        

            WAVEFORMATEXTENSIBLE *fmt = (WAVEFORMATEXTENSIBLE*)mtout->pbFormat;

            fmt->Format.nChannels = uiChannels;
            fmt->Format.nSamplesPerSec =uiSampleRate ;
            fmt->Format.nBlockAlign = fmt->Format.wBitsPerSample / 8 * fmt->Format.nChannels;
            fmt->Format.nAvgBytesPerSec = fmt->Format.nSamplesPerSec * fmt->Format.nBlockAlign;

             hr = m_pOutput->QueryAccept((AM_MEDIA_TYPE*)&temp);

            if(hr==S_OK)
                hr=pOut->SetMediaType(mtout);
            
        }
        iBytesConsumed =frameInfo.bytesconsumed;

        memcpy(pBufferOut, (void*)OutBuf, frameInfo.samples*2);
        ActualOutLength=frameInfo.samples*2;
        pOut->SetActualDataLength(ActualOutLength);


RETAILMSG(0,(L"\n[EAAC PLUS] Decode  (in=%d)(out=%d)....\n", srcBufLength, ActualOutLength));
    }

    else
    {

        iBytesConsumed=0;
        if(first_time==1)
        {
            hr=Initialize_Decoder(srcBufLength,pBufferIn,srcBufLength,&iBytesConsumed);
            if(hr!= S_OK)
            {
                Stop();
                return E_FAIL;
            }

            first_time=0;
            once=1;
            
        }
        else
        {
            memcpy(InBuf+bytes_left, pBufferIn, srcBufLength);
        }

        bytes_left+=srcBufLength;
        ActualOutLength=0;
        
        while(1)
            {

                if(iBytesConsumed >0)
                {

                    bytes_left-=iBytesConsumed;
                    memmove(InBuf,InBuf+iBytesConsumed,bytes_left);
                    
                    if(bytes_left<500)
                    {
                        break;
                    }
                }

                End=AACDecode_Ittiam(AACDecoder,InBuf,OutBuf,&frameInfo);

                if(frameInfo.sbrEnabled && once)
                {
                    uiSampleRate *=2;
                    once=0;
                    CMediaType temp;
            
                    GetMediaType(0,&temp);

                    mtout=(AM_MEDIA_TYPE*)&temp;
                

                    WAVEFORMATEXTENSIBLE *fmt = (WAVEFORMATEXTENSIBLE*)mtout->pbFormat;

                    fmt->Format.nChannels = uiChannels;
                    fmt->Format.nSamplesPerSec =uiSampleRate ;
                    fmt->Format.nBlockAlign = fmt->Format.wBitsPerSample / 8 * fmt->Format.nChannels;
                    fmt->Format.nAvgBytesPerSec = fmt->Format.nSamplesPerSec * fmt->Format.nBlockAlign;

                     hr = m_pOutput->QueryAccept((AM_MEDIA_TYPE*)&temp);

                    if(hr==S_OK)
                        hr=pOut->SetMediaType(mtout);
                    
                }

                if(frameInfo.error)
                {
                    RETAILMSG(1,(L"\nError in decoding : %d\n", frameInfo.error));
                    pOut->SetActualDataLength(ActualOutLength);
                    Stop();
                }



                iBytesConsumed =frameInfo.bytesconsumed;

                memcpy(pBufferOut, (void*)OutBuf, frameInfo.samples*2);
                ActualOutLength+=frameInfo.samples*2;
                pBufferOut+=frameInfo.samples*2;

            }

            pOut->SetActualDataLength(ActualOutLength);
        
        }

    return S_OK;

}



///////////////////////////////////////////////////////////////////////
// DllRegisterServer
///////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

///////////////////////////////////////////////////////////////////////
// DllUnregisterServer
///////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

HRESULT CAACSBREnhanced::AACExit()
{
    if(InBuf) 
        free(InBuf);
    if(OutBuf) 
        free(OutBuf);
    if(AACDecoder)
        SAACFreeDecoder(AACDecoder);
    InBuf=NULL;
    OutBuf=NULL;
    AACDecoder=NULL;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////
// Stop
///////////////////////////////////////////////////////////////////////
HRESULT CAACSBREnhanced::Stop()
{
    
    AACExit();

    first_time=1;
    stopped_once=1;
    bytes_left=0;

    CTransformFilter::Stop();

     return S_OK;
}

STDMETHODIMP CAACSBREnhanced::Initialize_Decoder(int inpBufSize, BYTE* buffer, int buffer_size, int *bytes_consumed)
{
        int hdrDecResult;
        if(InBuf==NULL)
        {
            InBuf = (unsigned char*)malloc(inpBufSize*2);
            if(!InBuf) 
            {
                RETAILMSG(err_AAC,(TEXT("memory could not be allocated for input buffer\n")));
                return E_FAIL;
            }
        }

        if(OutBuf==NULL)
        {
            OutBuf = malloc(OUTBUFSIZE * sizeof(short));
            memset(OutBuf, 0, OUTBUFSIZE*2);
            if(!OutBuf) 
            {
                AACExit();
                RETAILMSG(err_AAC,(TEXT("memory could not be allocated for output buffer\n")));
                return E_FAIL;
            }
        }

        // Intialize the Decoder 
        if(AACDecoder==NULL)
        {
            AACDecoder = (SAACDecoder*)SAACInitDecoder();
            if(!AACDecoder)
            {
                AACExit();

                RETAILMSG(err_AAC,(TEXT("decoder Initialization failed\n")));
                return 0;
            }
        }
        

        memcpy(InBuf,buffer,buffer_size);
    
        hdrDecResult = AACHeaderDecode_Ittiam(AACDecoder,InBuf, &config);
    
        *bytes_consumed = hdrDecResult;
        uiSampleRate = config.defSampleRate;
        uiChannels = config.defNumChannels;

        if(*bytes_consumed < 0)
        {
            RETAILMSG(err_AAC,(TEXT("header decode error\n")));
            if(*bytes_consumed == -5)
            {
                RETAILMSG(err_AAC,(TEXT("ADTS Sync-Word Not Found\n")));
            }

            return E_FAIL;
        }

        return S_OK;
}
