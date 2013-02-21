#ifndef __S3C6410_POST_PROC_MACRO_H__
#define __S3C6410_POST_PROC_MACRO_H__

#if __cplusplus
extern "C"
{
#endif

////////////////////////////////////////
//
//    TV Encoder SFR Bit Field Macro
//
////////////////////////////////////////


// TVCTRL
#define INTFIFO_URUN_DIS        (0<<16)
#define INTFIFO_URUN_EN         (1<<16)
#define INTFIFO_PEND            (1<<12)        // Interrupt Pending
#define TVOUTTYPE_COMPOSITE     (0<<8)
#define TVOUTTYPE_S_VIDEO       (1<<8)
#define TVOUTFMT_NTSC_M         (0<<4)
#define TVOUTFMT_NTSC_J         (1<<4)
#define TVOUTFMT_PAL_BDGHI      (2<<4)
#define TVOUTFMT_PAL_M          (3<<4)
#define TVOUTFMT_PAL_NC         (4<<4)
#define TVOUTFMT_MASK           (7<<4)
#define TVOUT_ON                (1<<0)
#define TVOUT_OFF               (0<<0)
#define TV_MAXWIDTH             (1440)
#define TV_MAXHEIGHT            (576)

// VBPORCH
#define VEFBPD(n)        (((n)&0x1ff)<<16)
#define VEFBPD_NTSC        (0x11c<<16)
#define VEFBPD_PAL        (0x14f<<16)
#define VOFBPD(n)        ((n)&0xff)
#define VOFBPD_NTSC        (0x15<<0)
#define VOFBPD_PAL        (0x16<<0)

// HBPORCH
#define HSPW(n)            (((n)&0xff)<<16)
#define HSPW_NTSC        (0x80<<16)
#define HSPW_PAL        (0x80<<16)
#define HBPD(n)            ((n)&0x7ff)
#define HBPD_NTSC        (0xf4<<0)
#define HBPD_PAL        (0x108<<0)

// HEnhOffset
#define VACTWinCenCTRL(n)    (((n)&0x3f)<<24)
#define HACTWinCenCTRL(n)    (((n)&0xff)<<16)
#define DTOffset(n)            (((n)&0x7)<<8)
#define DTOffset_NTSC        (0x4<<8)
#define DTOffset_PAL            (0x4<<8)
#define HEOV(n)                ((n)&0x1f)
#define HEOV_NTSC            (0x1a<<0)
#define HEOV_PAL            (0x1a<<0)

// VDemoWinSize
#define VDWS(n)            (((n)&0x1ff)<<16)
#define VDWS_NTSC        (0xf0<<16)
#define VDWS_PAL        (0xf0<<16)
#define VDWSP(n)            ((n)0x1ff)
#define VDWSP_NTSC        (0x0<<0)
#define VDWSP_PAL        (0x0<<0)

// HDemoWinSize
#define HDWEP(n)        (((n)&0x7ff)<<16)
#define HDWEP_NTSC        (0x5a0<<16)
#define HDWEP_PAL        (0x5a0<<16)
#define HDWSP(n)        ((n)&0x7ff)
#define HDWSP_NTSC        (0x0<<0)
#define HDWSP_PAL        (0x0<<0)

// InImageSize
#define ImageHehgit(n)    (((n)&0x3ff)<<16)
#define ImageWidth(n)    ((n)&0x7ff)

// PEDCTRL
#define PEDOn            (0<<0)
#define PEDOff            (1<<0)

// YCFilterBW
#define YBW_6_0MHZ        (0<<4)
#define YBW_3_8MHZ        (1<<4)
#define YBW_3_1MHZ        (2<<4)
#define YBW_2_6MHZ        (3<<4)
#define YBW_2_1MHZ        (4<<4)
#define CBW_1_2MHZ        (0<<0)
#define CBW_1_0MHZ        (1<<0)
#define CBW_0_8MHZ        (2<<0)
#define CBW_0_6MHZ        (3<<0)

// HUECTRL
#define HUE_SHIFT(n)        ((n)&0xff)
#define HUE_SHIFT_0        (0<<0)
#define HUE_SHIFT_90    (0x40<<0)
#define HUE_SHIFT_180    (0x80)<<0)
#define HUE_SHIFT_270    (0xC0)<<0)

// FscCTRL
#define FscCtrl(n)            ((n)&0x7fff)

// FscDTOManCTRL
#define FscMEn_DIS        (0<<31)
#define FscMEn_EN        (1<<31)
#define FscDTOManual(n)    ((n)&0x7fffffff)

// CCore
#define C_COR(n)            ((n)&0xf)

// BGCTRL
#define SoftMixEn_DIS    (0<<8)
#define SoftMixEn_EN        (1<<8)
#define BGCS_BLACK        (0<<4)
#define BGCS_BLUE        (1<<4)
#define BGCS_RED        (2<<4)
#define BGCS_MAGENTA    (3<<4)
#define BGCS_GREEN        (4<<4)
#define BGCS_CYAN        (5<<4)
#define BGCS_YELLOW    (6<<4)
#define BGCS_WHITE        (7<<4)
#define BGYOFS(n)        ((n)&0xf)

// BGHVAVCTRL
#define BG_HL(n)            (((n)&0xff)<<24)
#define BG_HL_NTSC        (0xb4<<24)
#define BG_HL_PAL        (0xb4<<24)
#define BG_HS(n)            (((n)&0xff<<16)
#define BG_HS_NTSC        (0x0<<16)
#define BG_HS_PAL        (0x0<<16)
#define BG_VL(n)            (((n)&0xff)<<8)
#define BB_VL_NTSC        (0xf0<<8)
#define GB_VL_PAL        (0xf0<<8)
#define BG_VS(n)            ((n)&0xff)
#define BG_VS_NTSC        (0x0<<0)
#define BG_VS_PAL        (0x0<<0)

// BWStrVal
#define O_BWMAX(n)        (((n)0xff)<<16)
#define O_BWMIN(n)        ((n)&0xff)

// DCAPL
#define O_DCAPL(n)        ((n)&0xff)

// ContraBright
#define BRIGHT(n)        (((n)0xff)<<16)
#define BRIGHT_MAX        (0x7f<<16)
#define BRIGHT_MIN        (0x80<<16)
#define CONTRAST(n)        ((n)&0xff)

// CbCrGainCTRL
#define CR_GAIN(n)        (((n)&0xff)<<16)
#define CB_GAIN(n)        ((n)&0xff)

// DemoWinCTRL
#define MVDemo_NORMAL    (0<<24)
#define MVDemo_DEMO    (1<<24)
#define BSDW_NORMAL    (0<<20)
#define BSDW_WIDE        (1<<20)
#define FreshEn_DIS        (0<<16)
#define FreshEn_EN        (1<<16)
#define BStEn_DIS        (0<<12)
#define BStEn_EN            (1<<12)
#define WStEn_DIS        (0<<8)
#define WStEn_EN        (1<<8)
#define BSTP_170        (0<<4)
#define BSTP_192        (1<<4)
#define BSGn_OFF        (0<<0)
#define BSGn_MAX        (3<<0)

// FTCA
#define FTCAC(n)            (((n)&0xff)<<16)
#define FTCAS(n)            ((n)&0xff)

// BWTiltHDLY
#define BW_HDLY(n)        (((n)&0xff)<<16)
#define WTilt(n)            (((n)&0xf)<<4)
#define BTilt(n)            ((n)&0xf)

// BWGAIN
#define WGain(n)            (((n)&0xf)<<4)
#define BGain(n)            ((n)&0xf)

// BWStrCTRL
#define BWHF(n)                (((n)&0xf)<<28)
#define BWVF(n)                (((n)&0xf)<<24)
#define T_BW_NORMAL        (0<<22)
#define T_BW_TEST            (1<<22)
#define BlackMode_NORMAL    (0<<21)
#define BlackMode_ADAPTIVE    (1<<21)
#define WhiteMode_NORMAL    (0<<20)
#define WhiteMode_ADAPTIVE    (1<<20)
#define FTimeResp(n)            (((n)&0xf)<<16)
#define BW_WEL(n)            (((n)&0xff<<8)
#define BW_WSL(n)            ((n)&0xff)

// SharpCTRL
#define SHARP_T(n)            (((n)&0xff)<<20)
#define DSTG(n)                (((n)&0xf)<<16)
#define DSTG_MAX            (0x0<<16)
#define DSTG_NO_LIMIT        (0xf<<16)
#define SDhCor(n)            (((n)&0x7)<<12)
#define SDhCor_DIS            (0<<12)
#define SDhCor_MAX            (7<<12)
#define DShpF0_LOW            (0<<8)
#define DShpF0_MID            (1<<8)
#define DShpF0_HIGH            (2<<8)
#define DShpGn(n)            ((n)&0x3f)
#define DShpGn_REDUCE_HFQ    (0x0<<0)
#define DShpGn_NO_SHARP    (0xf<<0)
#define DShpGn_MAX            (0x3f<<0)

// GammaCTRL
#define GamEn_DIS        (0<<12)
#define GamEn_EN        (1<<12)
#define GamMode(n)        (((n)&0x3)<<8)
#define GamMode_MIN    (0<<8)
#define GamMode_MAX    (3<<8)
#define DCTC(n)            (((n)&0x3)<<4)
#define DCTC_SLOW        (0<<4)
#define DCTC_FAST        (3<<4)
#define DCTRAN(n)        ((n)&0x7)
#define DCTRAN_80        (0<<0)
#define DCTRAN_100        (5<<0)
#define DCTRAN_110        (7<<0)

// FscAuxCTRL
#define Phalt_DIS        (0<<4)
#define Phalt_EN            (1<<4)
#define Fdrst_FREERUN    (0<<0)
#define Fdrst_RESET        (1<<0)

// SyncSizeCTRL
#define Sy_Size(n)        ((n)&0x3ff)
#define Sy_Size_NTSC    (0x3d<<0)
#define Sy_Size_PAL        (0x3e<<0)

// BurstCTRL
#define Bu_End(n)        (((n)&0x3ff)<<16)
#define Bu_End_NTSC        (0x69<<16)
#define Bu_End_PAL        (0x6a<<16)
#define Bu_St(n)            ((n)&0x3ff)
#define Bu_St_NTSC        (0x49<<0)
#define Bu_St_PAL        (0x4a<<0)

// MacroBurstCTRL
#define Bumav_St(n)        ((n)&0x3ff)
#define Bumav_St_NTSC    (0x41<<0)
#define Bumav_St_PAL    (0x42<<0)

// ActVidPoCTRL
#define Avon_End(n)        (((n)&0x3ff)<<16)
#define Avon_End_NTSC    (0x348<<16)
#define Avon_End_PAL    (0x352<<16)
#define Avon_St(n)        ((n)&0x3ff)
#define Avon_St_NTSC    (0x78<<0)
#define Avon_St_PAL        (0x82<<0)

// EncCTRL
#define BstLpfSel_NARROW    (0<<4)
#define BstLpfSel_WIDE        (1<<4)
#define BGEn_DIS            (0<<0)
#define BGEn_EN                (1<<0)

// MuteCTRL
#define Mute_Cr(n)        (((n)&0xff)<<24)
#define Mute_Cb(n)        (((n)&0xff)<<16)
#define Mute_Y(n)        (((n)&0xff)<<8)
#define MuteEn_EN        (0<<0)
#define MuteEn_DIS        (1<<0)

// Macrovision0
// Macrovision1
// Macrovision2
// Macrovision3
// Macrovision4
// Macrovision5
// Macrovision6

// VBIOn
#define VBI_ON(n)        ((n)&0x3)

// TVConFSMState
#define WrFIFOFSM_MASK        (0x1<<2)
#define SystemState_MASK    (0x3<<0)

// IPInfo

#if __cplusplus
}
#endif

#endif    // __S3C6410_POST_PROC_MACRO_H__
