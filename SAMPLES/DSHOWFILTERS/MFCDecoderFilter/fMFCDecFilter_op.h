//------------------------------------------------------------------------------
// File: fMFCDecFilter_op.h
//
// Desc: define CMFCDecFilterOutputPin classss
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#if !defined(_SAMSUNG_SYSLSI_FMFCDECFILTER_OP_H_)
#define _SAMSUNG_SYSLSI_FMFCDECFILTER_OP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CMFCDecFilterOutputPin : public CTransformOutputPin
{
    friend class CMFCDecFilter;

public:
    CMFCDecFilterOutputPin(HRESULT *phr, CMFCDecFilter *pFilter, LPCWSTR pPinName);
    virtual ~CMFCDecFilterOutputPin(void);

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


#endif // !defined(_SAMSUNG_SYSLSI_FMFCDECFILTER_OP_H_)
