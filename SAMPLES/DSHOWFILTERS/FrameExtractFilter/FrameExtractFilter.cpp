//------------------------------------------------------------------------------
// File: FrameExtractFilter.cpp
//
// Desc: FrameExtractFilter DirectShow filter
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "fFrameExtractFilter.h"

#include <initguid.h>
#include "FrameExtractFilter.h"


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
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_h264raw},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_rcv}
};

const AMOVIESETUP_PIN psudPins[] =
{
    {
        L"Output",            // Pin's string name
        FALSE,                // Is it rendered
        TRUE,                // Is it an output
        FALSE,                // Allowed none
        FALSE,                // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Input",            // Connects to pin
        sizeof(sudOutputPinTypes) / sizeof(AMOVIESETUP_MEDIATYPE),  // Number of media types
        sudOutputPinTypes    // Pin type information
    },
};

const AMOVIESETUP_FILTER sudFrameExtractFilter =
{
    &CLSID_FrameExtractFilter,    // CLSID of filter
    L"FrameExtract Filter",        // Filter's name
    MERIT_DO_NOT_USE,            // Filter merit
    1,                            // Number of pins
    psudPins                    // Pin information
};


//
//  Object creation template
//
CFactoryTemplate g_Templates[] = 
{
    {
        L"FrameExtract Filter",
        &CLSID_FrameExtractFilter,
        CFrameExtractFilter::CreateInstance,
        NULL,
        &sudFrameExtractFilter
    },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// Filter registration functions
//
HRESULT DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

HRESULT DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}


//
// Create a new instance of this class
//
CUnknown *CFrameExtractFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    RETAILMSG(1, (L"\n{ CFrameExtractFilter } Instance created\n"));



    //    DLLEntry does the right thing with the return code and
    //    returned value on failure
    CUnknown *pUnknown = new CFrameExtractFilter(pUnk, phr);
    if (pUnknown == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }

    return pUnknown;
}
