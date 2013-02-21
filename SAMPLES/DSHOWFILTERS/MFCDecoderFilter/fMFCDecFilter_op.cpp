//------------------------------------------------------------------------------
// File: fCopyTransform_op.cpp
//
// Desc: implement CMFCDecFilterOutputPin class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "fMFCDecFilter.h"
#include "fMFCDecFilter_op.h"

//
// Constructor
//
CMFCDecFilterOutputPin::CMFCDecFilterOutputPin(HRESULT *phr, CMFCDecFilter *pFilter, LPCWSTR pPinName)
    : CTransformOutputPin(NAME("CMFCDecFilterOutputPin"), pFilter, phr, pPinName)
{
    CAutoLock cAutoLock(m_pLock);
}


//
// Destructor
//
CMFCDecFilterOutputPin::~CMFCDecFilterOutputPin()
{
}

