//------------------------------------------------------------------------------
// File: MFCDecFilter.cpp
//
// Desc: MFCDecFilter DirectShow filter
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "fMFCDecFilter.h"

#include <initguid.h>
#include "MFCDecFilter.h"


//
// Setup data for filter registration
//
#define TRANSFORM_NAME L"S3C6400 MFC Decoder Filter"

const AMOVIESETUP_MEDIATYPE sudInputPinTypes[] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_m4v},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_h264raw},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_rcv},
    {&MEDIATYPE_Video, &WMMEDIASUBTYPE_WMV3},
    {&MEDIATYPE_Video, &WMMEDIASUBTYPE_wmv3}
};

const AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
    { &MEDIATYPE_Video, &MEDIASUBTYPE_NULL }
};

const AMOVIESETUP_PIN psudPins[] =
{
    {
        L"Input",        // Pin's string name
        FALSE,            // Is it rendered
        FALSE,            // Is it an output
        FALSE,            // Allowed none
        FALSE,            // Allowed many
        &CLSID_NULL,    // Connects to filter
        L"Output",        // Connects to pin
        sizeof(sudInputPinTypes) / sizeof(AMOVIESETUP_MEDIATYPE),// Number of types
        sudInputPinTypes    // Pin type information
    },
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

const AMOVIESETUP_FILTER sudMFCDecFilter =
{
    &CLSID_MFCDecFilter,        // CLSID of filter
    TRANSFORM_NAME,                // Filter's name
    0x00800004,                // Filter merit
    2,                            // Number of pins
    psudPins                    // Pin information
};


//
//  Object creation template
//
CFactoryTemplate g_Templates[] = 
{
    {
        TRANSFORM_NAME,
        &CLSID_MFCDecFilter,
        CMFCDecFilter::CreateInstance,
        NULL,
        &sudMFCDecFilter
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
CUnknown *CMFCDecFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    RETAILMSG(1, (L"\n{ MFCDecFilter } Instance created\n"));


    CMFCDecFilter *pNewObject = new CMFCDecFilter(pUnk, phr);
    if (pNewObject == NULL)
    {
        RETAILMSG(1,(L"\n[MFC Filter]Filter:E_OUTOFMEMORY\n"));
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
}
