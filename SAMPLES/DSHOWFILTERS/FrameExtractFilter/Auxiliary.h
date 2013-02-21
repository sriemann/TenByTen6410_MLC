//------------------------------------------------------------------------------
// File: Auxiliary.h
//
// Desc: Useful auxiliaries for the Win32 Application.
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------

#if !defined(_SAMSUNG_SYSLSI_AUXILIARY_H_)
#define _SAMSUNG_SYSLSI_AUXILIARY_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//
// Function prototypes
//

//#ifdef _DEBUG

void TRACE(LPCTSTR lpszFormat, ...) ;

//#endif


//
// Macros
//

#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

#define JIF(x) { if (FAILED(hr=(x))) \
    { TRACE(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n"), hr); return hr; }}

#define LIF(x) { if (FAILED(hr=(x))) \
    { TRACE(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n"), hr); }}

#endif // !defined(_SAMSUNG_SYSLSI_AUXILIARY_H_)
