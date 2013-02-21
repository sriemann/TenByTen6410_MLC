//------------------------------------------------------------------------------
// File: fMFCDecFilter_ip.cpp
//
// Desc: implement CMFCDecFilterInputPin class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "fMFCDecFilter.h"
#include "fMFCDecFilter_ip.h"


//
// Constructor
//
CMFCDecFilterInputPin::CMFCDecFilterInputPin(HRESULT *phr, CMFCDecFilter *pFilter, LPCWSTR pPinName)
    : CTransformInputPin(NAME("CMFCDecFilter"), pFilter, phr, pPinName)
{
    CAutoLock cAutoLock(m_pLock);
}

//
// Destructor
//
CMFCDecFilterInputPin::~CMFCDecFilterInputPin()
{
}
