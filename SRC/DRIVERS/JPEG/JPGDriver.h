//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __JPEG_DRIVER_H__
#define __JPEG_DRIVER_H__


#define MAX_INSTANCE_NUM         1
#define MAX_PROCESSING_THRESHOLD 1000    // 1Sec

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define IOCTL_JPG_DECODE            CTL_CODE( 0, 0x810, 0, 0 )
#define IOCTL_JPG_ENCODE            CTL_CODE( 0, 0x811, 0, 0 )
#define IOCTL_JPG_GET_STRBUF        CTL_CODE( 0, 0x812, 0, 0 )
#define IOCTL_JPG_GET_FRMBUF        CTL_CODE( 0, 0x813, 0, 0 )
#define IOCTL_JPG_GET_PHY_FRMBUF    CTL_CODE( 0, 0x814, 0, 0 )
#define IOCTL_JPG_GET_THUMB_STRBUF  CTL_CODE( 0, 0x815, 0, 0 )
#define IOCTL_JPG_GET_THUMB_FRMBUF  CTL_CODE( 0, 0x816, 0, 0 )
#define IOCTL_JPG_GET_RGBBUF        CTL_CODE( 0, 0x817, 0, 0 )
#define IOCTL_JPG_GET_PHY_RGBBUF    CTL_CODE( 0, 0x818, 0, 0 )

#define JPG_CLOCK_DIVIDER_RATIO_QUARTER    4

#endif /*__JPEG_DRIVER_H__*/
