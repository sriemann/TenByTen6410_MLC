#ifndef __S3C6410_DISPLAY_CON_MACRO_H__
#define __S3C6410_DISPLAY_CON_MACRO_H__

#if __cplusplus
extern "C"
{
#endif

////////////////////////////////////////
//
//    Display Controller SFR Bit Field Macro
//
////////////////////////////////////////

// VIDCON0
#define PROGRESSIVE            (0<<29)
#define INTERLACE            (1<<29)

#define VIDOUT_RGBIF        (0<<26)
#define VIDOUT_TVENC        (1<<26)
#define VIDOUT_I80IF0        (2<<26)
#define VIDOUT_I80IF1        (3<<26)
#define VIDOUT_TV_RGBIF    (4<<26)
#define VIDOUT_TV_I80IF0    (6<<26)
#define VIDOUT_TV_I80IF1    (7<<26)

#define LDI1_16_MODE        (0<<23)
#define LDI1_16_2_MODE        (1<<23)
#define LDI1_9_9_MODE        (2<<23)
#define LDI1_16_8_MODE        (3<<23)
#define LDI1_18_MODE        (4<<23)
#define LDI1_8_8_MODE        (5<<23)

#define LDI0_16_MODE        (0<<20)
#define LDI0_16_2_MODE        (1<<20)
#define LDI0_9_9_MODE        (2<<20)
#define LDI0_16_8_MODE        (3<<20)
#define LDI0_18_MODE        (4<<20)
#define LDI0_8_8_MODE        (5<<20)

#define PNRMODE_RGB_P        (0<<17)
#define PNRMODE_BGR_P        (1<<17)
#define PNRMODE_RGB_S        (2<<17)
#define PNRMODE_BGR_S        (3<<17)
#define PNRMODE_MASK        (3<<17)

#define CLKVALUP_ALWAYS    (0<<16)
#define CLKVALUP_ST_FRM    (1<<16)

#define CLKVAL_F(n)            (((n-1)&0xff)<<6)

#define VCLK_NORMAL            (0<<5)
#define VCLK_FREERUN        (1<<5)

#define CLKDIR_DIRECT        (0<<4)
#define CLKDIR_DIVIDED        (1<<4)

#define CLKSEL_F_HCLK        (0<<2)
#define CLKSEL_F_LCDCLK        (1<<2)
#define CLKSEL_F_EXT27M    (3<<2)
#define CLKSEL_F_MASK        (3<<2)

#define ENVID_DISABLE        (0<<1)
#define ENVID_ENABLE        (1<<1)
#define ENVID_F_DISABLE        (0<<0)
#define ENVID_F_ENABLE        (1<<0)

// VIDCON1
#define IVCLK_FALL_EDGE        (0<<7)
#define IVCLK_RISE_EDGE        (1<<7)
#define IHSYNC_HIGH_ACTIVE    (0<<6)
#define IHSYNC_LOW_ACTIVE    (1<<6)
#define IVSYNC_HIGH_ACTIVE    (0<<5)
#define IVSYNC_LOW_ACTIVE    (1<<5)
#define IVDEN_HIGH_ACTIVE    (0<<4)
#define IVDEN_LOW_ACTIVE    (1<<4)

// VIDCON2
#define EN601               (0x1<<23)
#define TVIF_FMT_YUV444     (0x7<<7) // ????
#define TVIF_FMT_YUV422     (0x5<<12)   // Software Format Setting with YUV422 Format

// VIDTCON0
#define VBPDE(n)                (((n-1)&0xff)<<24)
#define VBPD(n)                (((n-1)&0xff)<<16)
#define VFPD(n)                (((n-1)&0xff)<<8)
#define VSPW(n)                (((n-1)&0xff)<<0)

// VIDTCON1
#define VFPDE(n)                (((n-1)&0xff)<<24)
#define HBPD(n)                (((n-1)&0xff)<<16)
#define HFPD(n)                (((n-1)&0xff)<<8)
#define HSPW(n)                (((n-1)&0xff)<<0)

// VIDTCON2
#define LINEVAL(n)            (((n-1)&0x7ff)<<11)
#define HOZVAL(n)            (((n-1)&0x7ff)<<0)

// VIDTCON3

// WINCON0
#define CSC_WIDE_RANGE        (0<<26)
#define CSC_NARROW_RANGE    (3<<26)

#define LOCALSEL_TVSCALER    (0<<23)
#define LOCALSEL_CIPREVIEW    (1<<23)
#define LOCALSEL_CICODEC    (1<<23)

#define LOCAL_PATH_DISABLE    (0<<22)
#define LOCAL_PATH_ENABLE    (1<<22)

#define BUFSEL_BUF0            (0<<20)
#define BUFSEL_BUF1            (1<<20)

#define BUFAUTO_DISABLE    (0<<19)
#define BUFAUTO_ENABLE        (1<<19)

#define BITSWP_DISABLE        (0<<18)
#define BITSWP_ENABLE        (1<<18)
#define BYTSWP_DISABLE        (0<<17)
#define BYTSWP_ENABLE        (1<<17)
#define HAWSWP_DISABLE    (0<<16)
#define HAWSWP_ENABLE        (1<<16)

#define LOCAL_IN_RGB888    (0<<13)
#define LOCAL_IN_YUV444    (1<<13)

#define BURSTLEN_16WORD    (0<<9)
#define BURSTLEN_8WORD        (1<<9)
#define BURSTLEN_4WORD        (2<<9)
#define BURSTLEN_MASK        (3<<9)

#define BLEND_PER_PLANE    (0<<6)
#define BLEND_PER_PIXEL        (1<<6)

#define BPPMODE_F_1BPP            (0<<2)
#define BPPMODE_F_2BPP            (1<<2)
#define BPPMODE_F_4BPP            (2<<2)
#define BPPMODE_F_8BPP_PAL    (3<<2)
#define BPPMODE_F_8BPP_NOPAL    (4<<2)
#define BPPMODE_F_16BPP_565    (5<<2)
#define BPPMODE_F_16BPP_A555    (6<<2)
#define BPPMODE_F_16BPP_I555    (7<<2)
#define BPPMODE_F_18BPP_666    (8<<2)
#define BPPMODE_F_18BPP_A665    (9<<2)
#define BPPMODE_F_19BPP_A666    (0xa<<2)
#define BPPMODE_F_24BPP_888    (0xb<<2)
#define BPPMODE_F_24BPP_A887    (0xc<<2)
#define BPPMODE_F_25BPP_A888    (0xd<<2)
#define BPPMODE_F_MASK            (0xf<<2)
#define BPPMODE_F(n)            (((n)&0xf)<<2)

#define ALPHASEL_ALPHA0    (0<<1)    // Per Plane
#define ALPHASEL_ALPHA1    (1<<1)
#define ALPHASEL_AEN        (0<<1)    // Per Pixel
#define ALPHASEL_DATA        (1<<1)

#define ENWIN_F_DISABLE    (0<<0)
#define ENWIN_F_ENABLE        (1<<0)

// VIDOSDxA
#define OSD_LEFTTOPX_F(n)    (((n)&0x7ff)<<11)
#define OSD_LEFTTOPY_F(n)    ((n)&0x7ff)

// VIDOSDxB
#define OSD_RIGHTBOTX_F(n)    (((n)&0x7ff)<<11)
#define OSD_RIGHTBOTY_F(n)    ((n)&0x7ff)

// VIDOSDxC
#define ALPHA0_R(n)            (((n)&0xf)<<20)
#define ALPHA0_G(n)            (((n)&0xf)<<16)
#define ALPHA0_B(n)            (((n)&0xf)<<12)
#define ALPHA1_R(n)            (((n)&0xf)<<8)
#define ALPHA1_G(n)            (((n)&0xf)<<4)
#define ALPHA1_B(n)            ((n)&0xf)

// VIDOSD0C, VIDOSDxD
#define OSD_SIZE(n)            ((n)&0xffffff)

// VIDW0xADD0
#define VBANK_F(n)            (((n)&0xff)<<24)
#define VBASEU_F(n)            ((n)&0xffffff)

// VIDW0xADD1
#define VBASEL_F(n)            ((n)&0xffffff)

// VIDW0xADD2
#define OFFSIZE_F(n)            (((n)&0x1fff)<<13)
#define PAGEWIDTH_F(n)        ((n)&0x1fff)

// VIDINTCON0
#define FIFOINTERVAL(n)        (((n)&3f)<<20)

#define SYSMAINCON_DISABLE    (0<<19)
#define SYSMAINCON_ENABLE    (1<<19)
#define SYSSUBCON_DISABLE    (0<<18)
#define SYSSUBCON_ENABLE    (1<<18)
#define SYSIFDONE_DISABLE    (0<<17)
#define SYSIFDONE_ENABLE    (1<<17)

#define FRAMESEL0_BACK        (0<<15)
#define FRAMESEL0_VSYNC    (1<<15)
#define FRAMESEL0_ACTIVE    (2<<15)
#define FRAMESEL0_FRONT    (3<<15)

#define FRAMESEL1_NONE        (0<<13)
#define FRAMESEL1_BACK        (1<<13)
#define FRAMESEL1_VSYNC    (2<<13)
#define FRAMESEL1_FRONT    (3<<13)

#define INTFRMEN_DISABLE    (0<<12)
#define INTFRMEN_ENABLE    (1<<12)
#define FRAMEINT_MASK        (0x1f<<12)

#define FIFOSEL_WIN4        (1<<11)
#define FIFOSEL_WIN3        (1<<10)
#define FIFOSEL_WIN2        (1<<9)
#define FIFOSEL_WIN1        (1<<6)
#define FIFOSEL_WIN0        (1<<5)
#define FIFOSEL_ALL            (FIFOSEL_WIN0 | FIFOSEL_WIN1 | FIFOSEL_WIN2 | FIFOSEL_WIN3 | FIFOSEL_WIN4)

#define FIFOLEVEL_25        (0<<2)
#define FIFOLEVEL_50        (1<<2)
#define FIFOLEVEL_75        (2<<2)
#define FIFOLEVEL_EMPTY        (3<<2)
#define FIFOLEVEL_FULL        (4<<2)

#define INTFIFOEN_DISABLE    (0<<1)
#define INTFIFOEN_ENABLE    (1<<1)
#define FIFOINT_MASK        (0x7ff<<1)

#define INTEN_DISABLE        (0<<0)
#define INTEN_ENABLE        (1<<0)

// VIDINTCON1
#define INTSYSIF_PEND        (1<<2)
#define INTFRM_PEND            (1<<1)
#define INTFIFO_PEND        (1<<0)
#define INTPEND_MASK        (0x7)

// WxKEYCON0
#define KEYBLEN_DISABLE        (0<<26)
#define KEYBLEN_ENABLE        (1<<26)
#define KEYEN_F_DISABLE    (0<<25)
#define KEYEN_F_ENABLE        (1<<25)
#define DIRCON_FG_MATCH_BG_DISPLAY (0<<24)
#define DIRCON_BG_MATCH_FG_DISPLAY (1<<24)
#define COMPKEY(n)            ((n)&0xffffff)

// WxKEYCON1
#define COLVAL(n)            ((n)&0xffffff)

// DITHMODE
#define RDITHPOS_8BIT        (0<<5)
#define RDITHPOS_6BIT        (1<<5)
#define RDITHPOS_5BIT        (2<<5)
#define GDITHPOS_8BIT        (0<<3)
#define GDITHPOS_6BIT        (1<<3)
#define GDITHPOS_5BIT        (2<<3)
#define BDITHPOS_8BIT        (0<<1)
#define BDITHPOS_6BIT        (1<<1)
#define BDITHPOS_5BIT        (2<<1)
#define RGB_DITHPOS_MASK    (0x3f<<1)
#define DITHEN_F_DISABLE    (0<<0)
#define DITHEN_F_ENABLE    (1<<0)

// WINxMAP
#define MAPCOLEN_F_DISABLE    (0<<24)
#define MAPCOLEN_F_ENABLE    (1<<24)
#define MAPCOLOR(n)            ((n)&0xffffff)

// WPALCON
#define PALUPDATE_NORMAL    (0<<9)
#define PALUPDATE_ENABLE    (1<<9)

#define W4PAL_16BIT_565    (0<<8)
#define W4PAL_16BIT_A555    (1<<8)

#define W3PAL_16BIT_565    (0<<7)
#define W3PAL_16BIT_A555    (1<<7)

#define W2PAL_16BIT_565    (0<<6)
#define W2PAL_16BIT_A555    (1<<6)

#define W1PAL_25BIT_A888    (0<<3)
#define W1PAL_24BIT_888    (1<<3)
#define W1PAL_19BIT_A666    (2<<3)
#define W1PAL_18BIT_A665    (3<<3)
#define W1PAL_18BIT_666    (4<<3)
#define W1PAL_16BIT_A555    (5<<3)
#define W1PAL_16BIT_565    (6<<3)
#define W1PAL_MASK            (0x7<<3)

#define W0PAL_25BIT_A888    (0<<0)
#define W0PAL_24BIT_888    (1<<0)
#define W0PAL_19BIT_A666    (2<<0)
#define W0PAL_18BIT_A665    (3<<0)
#define W0PAL_18BIT_666    (4<<0)
#define W0PAL_16BIT_A555    (5<<0)
#define W0PAL_16BIT_565    (6<<0)
#define W0PAL_MASK            (0x7<<0)

typedef struct _tDispBlendingConfig
{
    unsigned int BlendingMethod;
    union
    {
        struct
        {
            unsigned int AlphaSelect;
            unsigned int Alpha0_Red;
            unsigned int Alpha0_Green;
            unsigned int Alpha0_Blue;
            unsigned int Alpha1_Red;
            unsigned int Alpha1_Green;
            unsigned int Alpha1_Blue;
        };
        struct
        {
            unsigned int ColorKeyValue;
            unsigned int CompareKeyValue;
            unsigned int Direction;
        };
    };
} tDispAlphaBlendingConfig;

typedef struct _tDispWindow0Config
{
    DISP_BPP_MODE BPP_Mode;
    unsigned int LocalPathEnable;
    unsigned int LocaPathSourceFormat;
    unsigned int BufferSelect;
    unsigned int BufferAutoControl;
    unsigned int BitSwapping;
    unsigned int ByteSwapping;
    unsigned int HalfWordSwapping;
    unsigned int BurstLength;
    unsigned int uiWidth;
    unsigned int uiHeight;
    unsigned int uiOffsetX;
    unsigned int uiOffsetY;
    unsigned int uiPageWidth;
} tDispWindow0Config;

typedef struct _tDispWindow12Config
{
    DISP_BPP_MODE BPP_Mode;
    unsigned int LocalPathSelect;
    unsigned int LocalPathEnable;
    unsigned int LocaPathSourceFormat;
    unsigned int BufferSelect;
    unsigned int BufferAutoControl;
    unsigned int BitSwapping;
    unsigned int ByteSwapping;
    unsigned int HalfWordSwapping;
    unsigned int BurstLength;
    unsigned int uiWidth;
    unsigned int uiHeight;
    unsigned int uiOffsetX;
    unsigned int uiOffsetY;
    unsigned int uiPageWidth;
    tDispAlphaBlendingConfig BlendConfig;
} tDispWindow12Config;

typedef struct _tDispWindow34Config
{
    DISP_BPP_MODE BPP_Mode;
    unsigned int BitSwapping;
    unsigned int ByteSwapping;
    unsigned int HalfWordSwapping;
    unsigned int BurstLength;
    unsigned int uiWidth;
    unsigned int uiHeight;
    unsigned int uiOffsetX;
    unsigned int uiOffsetY;
    unsigned int uiPageWidth;
    tDispAlphaBlendingConfig BlendConfig;
} tDispWindow34Config;

#if __cplusplus
}
#endif

#endif    // __S3C6410_DISPLAY_CON_MACRO_H__
