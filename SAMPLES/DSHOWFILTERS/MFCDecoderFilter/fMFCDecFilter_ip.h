//------------------------------------------------------------------------------
// File: fMFCDecFilter_ip.h
//
// Desc: define CMFCDecFilterInputPin class
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#if !defined(_SAMSUNG_SYSLSI_FMFCDECFILTER_IP_H_)
#define _SAMSUNG_SYSLSI_FMFCDECFILTER_IP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CMFCDecFilterInputPin : public CTransformInputPin
{
    friend class CMFCDecFilter;

public:
    CMFCDecFilterInputPin(HRESULT *phr, CMFCDecFilter *pFilter, LPCWSTR pPinName);
    virtual ~CMFCDecFilterInputPin(void);

// Attributes
public:

// Operations
public:

// Overrides
protected:

// Implementations
protected:

// member variables
private:

};

#endif // !defined(_SAMSUNG_SYSLSI_FMFCDECFILTER_IP_H_)
