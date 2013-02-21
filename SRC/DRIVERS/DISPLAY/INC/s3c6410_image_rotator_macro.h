#ifndef __S3C6410_IMAGE_ROTATOR_MACRO_H__
#define __S3C6410_IMAGE_ROTATOR_MACRO_H__

#if __cplusplus
extern "C"
{
#endif

////////////////////////////////////////
//
//    Image Rotator SFR Bit Field Macro
//
////////////////////////////////////////

// CTRLCFG
#define INT_DISABLE            (0<<24)
#define INT_ENABLE            (1<<24)
#define IMG_FORMAT_YUV420    (0<<13)
#define IMG_FORMAT_YUV422    (3<<13)
#define IMG_FORMAT_RGB565    (4<<13)
#define IMG_FORMAT_RGB888    (5<<13)
#define ROTATE_DEGREE_00    (0<<6)
#define ROTATE_DEGREE_90    (1<<6)    // Clockwise
#define ROTATE_DEGREE_180    (2<<6)
#define ROTATE_DEGREE_270    (3<<6)    // Clockwise
#define FLIP_NONE            (0<<4)
#define FLIP_VERTICAL        (2<<4)
#define FLIP_HORIZONTAL        (3<<4)
#define ROTATOR_START        (1<<0)

// SRCADDRREG0
// SRCADDRREG1
// SRCADDRREG2
// DESTADDRREG0
// DESTADDRREG1
// DESTADDRREG2
#define ADDRESS_RGB_Y(n)    ((n)&0x7FFFFFFF)
#define ADDRESS_CB(n)        ((n)&0x7FFFFFFF)
#define ADDRESS_CR(n)        ((n)&0x7FFFFFFF)

// SRCSIZEREG
#define VERTICAL_SIZE(n)        (((n)&0xFFFF)<<16)
#define HORIZONTAL_SIZE(n)    ((n)&0xFFFF)

// STATCFG
#define CUR_LINE_MASK(n)    (((n)>>16)&0xFFFF)
#define INT_PENDING            (1<<8)
#define STAT_IDLE            (0<<0)
#define STAT_BUSY            (2<<0)
#define STAT_BUSY2            (3<<0)    // One more jop pended..

#if __cplusplus
}
#endif

#endif    // __S3C6410_IMAGE_ROTATOR_MACRO_H__
