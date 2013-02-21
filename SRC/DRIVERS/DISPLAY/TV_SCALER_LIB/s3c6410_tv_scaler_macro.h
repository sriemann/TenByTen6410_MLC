#ifndef __S3C6410_TV_SCALER_MACRO_H__
#define __S3C6410_TV_SCALER_MACRO_H__

#if __cplusplus
extern "C"
{
#endif

////////////////////////////////////////
//
//    Display Controller SFR Bit Field Macro
//
////////////////////////////////////////

// MODE
#define FIFO_IN_DISABLE        (0<<31)
#define FIFO_IN_ENABLE        (1<<31)
#define CLKVALUP_ALWAYS    (0<<30)
#define CLKVALUP_ST_FRM    (1<<30)
#define CLKVAL_F(n)            (((n-1)&0x3f)<<24)
#define CLKDIR_DIRECT        (0<<23)
#define CLKDIR_DIVIDED        (1<<23)
#define CLKSEL_F_HCLK        (0<<21)
#define CLKSEL_F_PLLCLK        (1<<21)
#define CLKSEL_F_EXT27M    (3<<21)
#define OUTYUV_YCBYCR        (0<<19)
#define OUTYUV_CBYCRY        (1<<19)
#define OUTYUV_YCRYCB        (2<<19)
#define OUTYUV_CRYCBY        (3<<19)
#define OUTFMT_YUV            (0<<18)
#define OUTFMT_RGB            (1<<18)
#define OUTYUV_YUV422        (0<<17)
#define OUTYUV_YUV420        (1<<17)
#define CSC_R2Y_NARROW        (0<<16)
#define CSC_R2Y_WIDE        (1<<16)
#define AUTOLOAD_DISABLE    (0<<14)
#define AUTOLOAD_ENABLE    (1<<14)
#define FIFO_OUT_DISABLE    (0<<13)
#define FIFO_OUT_ENABLE    (1<<13)
#define PROGRESSIVE            (0<<12)
#define INTERLACE            (1<<12)
#define CSC_Y2R_NARROW        (1<<10)
#define CSC_Y2R_WIDE        (2<<10)
#define SRCYUV_YUV422        (0<<8)
#define SRCYUV_YUV420        (1<<8)
#define TVSCINT_ENABLE        (1<<7)
#define TVSCINT_PEND        (1<<6)
#define IRQ_EDGE            (0<<5)
#define IRQ_LEVEL            (1<<5)
#define OUTRGB_RGB565        (0<<4)
#define OUTRGB_RGB24        (1<<4)
#define SRCFMT_YUV            (0<<3)
#define SRCFMT_RGB            (1<<3)
#define SRC_NOT_INTERLEAVE    (0<<2)
#define SRC_INTERLEAVE        (1<<2)
#define SRCRGB_RGB565        (0<<1)
#define SRCRGB_RGB24        (1<<1)
#define SRCYUV_YCBYCR        ((0<<15)|(0<<0))
#define SRCYUV_CBYCRY        ((0<<15)|(1<<0))
#define SRCYUV_YCRYCB        ((1<<15)|(0<<0))
#define SRCYUV_CRYCBY        ((1<<15)|(1<<0))
#define SRCYUV_MASK        ((1<<15)|(1<<0))

// PewScale_Ratio
#define PRESCALE_V_RATIO(n)        (((n)&0x7f)<<7)
#define PRESCALE_H_RATIO(n)        ((n)&0x7f)

//PreScaleImgSize
#define PRESCALE_HEIGHT(n)        (((n)&0xfff)<<12)
#define PRESCALE_WIDTH(n)        ((n)&0xfff)

// SRCImgSize
#define SRC_HEIGHT(n)             (((n)&0xfff)<<12)
#define SRC_WIDTH(n)            ((n)&0xfff)

// MainScale_H_Ratio
#define MAINSCALE_H_RATIO(n)    ((n)&0x1ff)

// MainScale_V_Ratio
#define MAINSCALE_V_RATIO(n)    ((n)&0x1ff)

// DSTImgSize
#define DST_HEIGHT(n)            (((n)&0xfff)<<12)
#define DST_WIDTH(n)            ((n)&0xfff)

// PreScale_SHFactor
#define PRESCALE_SHFACTOR(n)    ((n)&0xf)

// OffSetx
#define OFFSET(n)                ((n)&0xffffff)

// POSTENVID
#define TVSC_ENVID    (1<<31)

// MODE_2
#define ADDR_CHANGE_ENABLE    (0<<4)
#define ADDR_CHANGE_DISABLE    (1<<4)
#define CHANGE_SEL_FIELD        (0<<3)
#define CHANGE_SEL_FRAME        (1<<3)
#define SW_TRG_MODE            (0<<0)

typedef struct _tTVScalerConfig
{
    TVSC_OP_MODE Mode;
    TVSC_SCAN_MODE Scan;
    TVSC_SRC_TYPE SrcType;
    unsigned int SrcBaseWidth;
    unsigned int SrcBaseHeight;
    unsigned int SrcWidth;
    unsigned int SrcHeight;
    unsigned int SrcOffsetX;
    unsigned int SrcOffsetY;
    TVSC_DST_TYPE DstType;
    unsigned int DstBaseWidth;
    unsigned int DstBaseHeight;
    unsigned int DstWidth;
    unsigned int DstHeight;
    unsigned int DstOffsetX;
    unsigned int DstOffsetY;
    BOOL bIntEnable;
} tTVScalerConfig;

#if __cplusplus
}
#endif

#endif    // __S3C6410_TV_SCALER_MACRO_H__
