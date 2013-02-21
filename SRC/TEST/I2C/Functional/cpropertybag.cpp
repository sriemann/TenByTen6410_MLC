//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

//#include "stdafx.h"
#include <windows.h>
#include <Ocidl.h>
#include <oleauto.h>

#include "cpropertybag.h"


CPropertyBag::CPropertyBag() : _refCount(1), pVar(0)
{       
}

CPropertyBag::~CPropertyBag()
{
    VAR_LIST *pTemp = pVar;
    HRESULT hr = S_OK;
    
    while(pTemp)
    {
        VAR_LIST *pDel = pTemp;
        VariantClear(&pTemp->var);
        SysFreeString(pTemp->pBSTRName);
        pTemp = pTemp->pNext;
        delete pDel;
    }

}

HRESULT STDMETHODCALLTYPE
CPropertyBag::Read(LPCOLESTR pszPropName, 
                       VARIANT *_pVar, 
                       IErrorLog *pErrorLog)
{
    VAR_LIST *pTemp = pVar;
    HRESULT hr = S_OK;
    
    while(pTemp)
    {
        if(0 == wcscmp(pszPropName, pTemp->pBSTRName))
        {
            hr = VariantCopy(_pVar, &pTemp->var);
            break;
        }
        pTemp = pTemp->pNext;
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
CPropertyBag::Write(LPCOLESTR pszPropName, 
                            VARIANT *_pVar)
{
    HRESULT hr = S_OK;
    if( NULL == pszPropName || NULL == _pVar )
    {
        return E_POINTER;
    }
    

    VAR_LIST *pTemp = new VAR_LIST();    
    if( NULL == pTemp )
    {
        return E_OUTOFMEMORY;
    }

    VariantInit(&pTemp->var);
    pTemp->pBSTRName = SysAllocString(pszPropName);
    pTemp->pNext = pVar;
    pVar = pTemp;
    return VariantCopy(&pTemp->var, _pVar);
}

ULONG STDMETHODCALLTYPE 
CPropertyBag::AddRef() 
{
    return InterlockedIncrement((LONG *)&_refCount);
}

ULONG STDMETHODCALLTYPE 
CPropertyBag::Release() 
{
    ASSERT(_refCount != 0xFFFFFFFF);
    ULONG ret = InterlockedDecrement((LONG *)&_refCount);    
    if(!ret) 
        delete this; 
    return ret;
}

HRESULT STDMETHODCALLTYPE 
CPropertyBag::QueryInterface(REFIID riid, void** ppv) 
{
    if(!ppv) 
        return E_POINTER;
    if(riid == IID_IPropertyBag) 
        *ppv = static_cast<IPropertyBag*>(this);	
    else 
        return *ppv = 0, E_NOINTERFACE;
    
    return AddRef(), S_OK;	
}
