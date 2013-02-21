//------------------------------------------------------------------------------
// File: MFCDecFilter.h
//
// Desc: MFCDecFilter DirectShow filter (CLSIDs)
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI, All rights reserved.
//------------------------------------------------------------------------------
#if !defined(_SAMSUNG_SYSLSI_MFCDECFILTER_H_)
#define _SAMSUNG_SYSLSI_MFCDECFILTER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STATUS                        0

// {CE12BD3B-710F-4b71-95B4-FCE1ABDF2EC1}
DEFINE_GUID(CLSID_MFCDecFilter, 
0xce12bd3b, 0x710f, 0x4b71, 0x95, 0xb4, 0xfc, 0xe1, 0xab, 0xdf, 0x2e, 0xc1);

// {59fcbec7-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_m4v, 
0x59fcbec7, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// {7807c3af-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_h264raw, 
0x7807c3af, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// {6e1e6367-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_rcv, 
0x6e1e6367, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

DEFINE_GUID(WMMEDIASUBTYPE_WMV3, 
0x33564D57, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(WMMEDIASUBTYPE_wmv3, 
0x33766D77, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);


DEFINE_GUID(WMMEDIATYPE_test, 
0x18056900, 0xC406, 0x11D0, 0xA5, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

#endif // !defined(_SAMSUNG_SYSLSI_MFCDECFILTER_H_)
