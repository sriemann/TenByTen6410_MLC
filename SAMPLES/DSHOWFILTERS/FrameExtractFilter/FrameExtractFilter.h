//------------------------------------------------------------------------------
// File: FrameExtractFilter.h
//
// Desc: FrameExtractFilter DirectShow filter (CLSIDs)
//
// Author : JiyoungShin(idon.shin@samsung.com)
//
// Copyright 2007 Samsung System LSI.  All rights reserved.
//------------------------------------------------------------------------------

#if !defined(_SAMSUNG_SYSLSI_FRAMEEXTRACTFILTER_H_)
#define _SAMSUNG_SYSLSI_FRAMEEXTRACTFILTER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// {1A821B27-D2F4-4e22-9496-3C7D0F612EC2}
DEFINE_GUID(CLSID_FrameExtractFilter, 
0x1a821b27, 0xd2f4, 0x4e22, 0x94, 0x96, 0x3c, 0x7d, 0xf, 0x61, 0x2e, 0xc2);

// {59fcbec7-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_m4v, 
0x59fcbec7, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// {7807c3af-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_h264raw, 
0x7807c3af, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// {6e1e6367-524f-11ce-9f53-0020af0ba770}
DEFINE_GUID(MEDIASUBTYPE_rcv, 
0x6e1e6367, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

#endif // !defined(_SAMSUNG_SYSLSI_FRAMEEXTRACTFILTER_H_)
