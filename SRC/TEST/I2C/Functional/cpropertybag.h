//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#ifndef PROPERTYBAG_H 
#define PROPERTYBAG_H

#include <strmif.h>

struct VAR_LIST
{
    VARIANT var;
    VAR_LIST *pNext;
    BSTR pBSTRName;
};

class CPropertyBag : public IPropertyBag
{  
public:
    CPropertyBag();
    ~CPropertyBag();
    
    HRESULT STDMETHODCALLTYPE
    Read(
        LPCOLESTR pszPropName, 
        VARIANT *pVar, 
        IErrorLog *pErrorLog
        );
    
    
    HRESULT STDMETHODCALLTYPE
    Write(
        LPCOLESTR pszPropName, 
        VARIANT *pVar
        );
        
    ULONG STDMETHODCALLTYPE AddRef();        
    ULONG STDMETHODCALLTYPE Release();        
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);   

private:
     ULONG _refCount;
     VAR_LIST *pVar;
};

#endif
