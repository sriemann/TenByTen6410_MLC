//------------------------------------------------------------------------------
// File: FrameExtractFilter.h
//
// Desc: FrameExtractFilter DirectShow filter (CLSIDs)
//
// Copyright (c) 2000 - 2005, L544вт Technology.  All rights reserved.
//------------------------------------------------------------------------------

#if !defined(_SAMSUNG_SYSLSI_MP4FILEPARSERFILTER_H_)
#define _SAMSUNG_SYSLSI_MP4FILEPARSERFILTER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// {1A821B27-D2F4-4e22-9496-3C7D0F612EC0}
DEFINE_GUID(CLSID_SSAP_MP4Parser_Filter, 
0x1a821b27, 0xd2f4, 0x4e22, 0x94, 0x96, 0x3c, 0x7d, 0xf, 0x61, 0x2e, 0xc0);

// {59fcbec7-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_m4v, 
0x59fcbec7, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// {7807c3af-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_h264raw, 
0x7807c3af, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);


// {59fcbec7-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_m4a, 
0x59fcbec7, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x79);

// {7f7aa702-7343-49d0-a550-8e0cd7cbde23}
DEFINE_GUID(MEDIASUBTYPE_EAACPLUS, 
0x7f7aa702, 0x7343, 0x49d0, 0xa5, 0x50, 0x8e, 0x0c, 0xd7, 0xcb, 0xde, 0x23);

// {4c7012d9-2e06-4306-8cb0-39996b99f09b)
DEFINE_GUID(MEDIASUBTYPE_AmrDec, 
0x4c7012d9, 0x2e06, 0x4306, 0x8c, 0xb0, 0x39, 0x99, 0x6b, 0x99, 0xf0, 0x9b);

#endif // !defined(_SAMSUNG_SYSLSI_MP4FILEPARSERFILTER_H_)
