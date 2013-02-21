//------------------------------------------------------------------------------
// File: SsapMp4Parser.cpp
//
// Desc: SsapMp4ParserFilter DirectShow filter
//
// Copyright (c) 2008, Samsung Electronics S.LSI.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "fSsapMp4Parser.h"

#include <initguid.h>
#include "SsapMp4Parser.h"


//
// Setup data for filter registration
//
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{    &MEDIATYPE_NULL,    // Major CLSID
    &MEDIASUBTYPE_NULL    // Minor type
};

const AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_m4v},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_h264raw}
};

const AMOVIESETUP_PIN psudPins[] =
{
    {
        L"Output",        // Pin's string name
        FALSE,            // Is it rendered
        TRUE,            // Is it an output
        FALSE,            // Allowed none
        FALSE,            // Allowed many
        &CLSID_NULL,    // Connects to filter
        L"Input",        // Connects to pin
        sizeof(sudOutputPinTypes) / sizeof(AMOVIESETUP_MEDIATYPE),  // Number of media types
        sudOutputPinTypes    // Pin type information
    },
};

const AMOVIESETUP_FILTER sudSSAP_MP4ParserFilter_bak =
{
    &CLSID_SSAP_MP4Parser_Filter,    // CLSID of filter
    L"SSAP MP4 Parser Filter",        // Filter's name
    MERIT_NORMAL,                    // Filter merit
    1,                                // Number of pins
    psudPins                        // Pin information
};

const AMOVIESETUP_FILTER sudSSAP_MP4ParserFilter =
{
    &CLSID_SSAP_MP4Parser_Filter,    // CLSID of filter
    L"SSAP MP4 Parser Filter",        // Filter's name
    MERIT_NORMAL,                    // Filter merit
    0,                                // Number of pins
    NULL                        // Pin information
};


//
//  Object creation template
//
CFactoryTemplate g_Templates[] = 
{
    {
        L"SSAP MP4 Parser Filter",
        &CLSID_SSAP_MP4Parser_Filter,
        CMP4ParserFilter::CreateInstance,
        NULL,
        &sudSSAP_MP4ParserFilter
    },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// Filter registration functions
//
HRESULT WINAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

HRESULT WINAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}


//
// Create a new instance of this class
//
CUnknown *CMP4ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    //    DLLEntry does the right thing with the return code and
    //    returned value on failure
    CUnknown *pUnknown = new CMP4ParserFilter(pUnk, phr);
    if (pUnknown == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }

    return pUnknown;
}
