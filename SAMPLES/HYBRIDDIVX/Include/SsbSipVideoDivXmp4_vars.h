/**
 ** Copyright (C) 2002 DivXNetworks, all rights reserved.
 **
 ** DivXNetworks, Inc. Proprietary & Confidential
 **
 ** This source code and the algorithms implemented therein constitute
 ** confidential information and may comprise trade secrets of DivXNetworks
 ** or its associates, and any use thereof is subject to the terms and
 ** conditions of the Non-Disclosure Agreement pursuant to which this
 ** source code was originally received.
 **
 **/

/** $Id: SsbSipVideoDivXmp4_vars.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

/**
*  Copyright (C) 2001 - DivXNetworks
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// SsbSipVideoDivXmp4_vars.h //
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPMP4_VARS_H__
#define ___SSBSIPMP4_VARS_H__

/*
// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
*/
#include <stdio.h>

#include "SsbSipVideoDivXdecore.h"

#include "SsbSipVideoDivXportab.h"
#include "SsbSipVideoDivXbasic_prediction.h" // mc proc definition
#include "SsbSipVideoDivXnoise_adder.h"
#include "SsbSipVideoDivXlogo_adder.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/


#define mmax(a, b)      (a)-(((a)-(b))&(((a)-(b))>>31))
#define mmin(a, b)      (b)+(((a)-(b))&(((a)-(b))>>31))
#define sign(a)            ( ((a)>>31) | 1)

#define mnint(a)    ((a) < 0 ? (int)(a - 0.5) : (int)(a + 0.5))
#define abs(a)            ((a)>0 ? (a) : -(a))

#ifdef WIN32
#pragma warning ( disable : 4244 ) // conversion from 'x' to 'y'
#pragma warning ( disable : 4100 ) // unreferenced formal parameter
#endif

/**
 *    definitions
**/

#define VO_START_CODE        0x8
#define VO_START_CODE_MIN    0x100
#define VO_START_CODE_MAX    0x11f

#define VOL_START_CODE        0x12
#define VOL_START_CODE_MIN    0x120
#define VOL_START_CODE_MAX    0x12f

#define VOS_START_CODE  0x1b0
#define USR_START_CODE  0x1b2
#define GOP_START_CODE  0x1b3
#define VSO_START_CODE  0x1b5
#define VOP_START_CODE    0x1b6
#define STF_START_CODE  0x1c3 // stuffing_start_code
#define SHV_START_CODE  0x020
#define SHV_END_MARKER  0x03f

#define I_VOP        0
#define P_VOP        1
#define B_VOP        2
#define S_VOP        3
#ifdef OPT_IDCT
#define DC 0
#define AC 1
#define VL 2
#define HL 3
#define SQ 4
#endif


#define RECTANGULAR                0
#define BINARY                    1
#define BINARY_SHAPE_ONLY        2 
#define GRAY_SCALE                3

#define NOTUSE_SPRITE            0
#define STATIC_SPRITE            1
#define GMC_SPRITE                2
#define RESERVED_SPRITE            3

#define RESYNC_MARKER            1
#define NOT_VALID        1<<31
#define NOT_CODED        -1
#define INTER            0
#define INTER_Q            1
#define INTER4V            2
#define INTRA            3
#define INTRA_Q             4
#define STUFFING        7
#define B_MBLOCK        8    // used as a shortcut (overload) in mp4_recon_qpel
#define B_MBLOCK_DEST    9    // in this case, the destination is an array (mb)
#define B_BLOCK_DEST_4V    10

// b frames 
#define MODB_1                    0
#define MODB_00                    1
#define MODB_01                    2

#define MB_TYPE_1                0 // direct mode
#define MB_TYPE_01            1 // bi-directional mode
#define MB_TYPE_001            2 // backward mode
#define MB_TYPE_0001        3    // forward mode

#define DIRECT_MODE        MB_TYPE_1
#define BIDIR_MODE        MB_TYPE_01
#define BACKWARD_MODE    MB_TYPE_001
#define FORWARD_MODE    MB_TYPE_0001

#define FOUND_P_VOP_RET2MFC_NC  0x8000
#define FOUND_I_VOP_RET2MFC  0x8001
#define FOUND_P_VOP_RET2MFC  0x8002
#define FOUND_B_VOP_SWDCD    0x8003
#define FOUND_PB_CHUNK       0x8004
#define FOUND_VOP_SWDCD      0x8005
#define RET_DIVX_SW_DECODE   0x8006  
#define FRSTKEYFRMCRPTED     0x8007

#ifdef _PLD_OPT_
#ifdef LINUX
#define        PLD_LINUX(pld_s)     __asm__ __volatile__(    "PLD [ %0 ,#0]" :  "=r"(pld_s) : "r"(pld_s) );
#endif
#endif


// video packet structure size
#define MAX_MB_PACKET    2560 // [Review] what is the maximum allowed?

#define DEC_MBC    120
#define DEC_MBR    80

/**
 *    decoder struct
**/

typedef struct {
    int val, len;
} tab_type;


typedef struct {
    int last;
    int run;
    int level;
} event_t;

/**
    ATTENTION
    The block[] MUST be aligned on 16byte boundaries.  Vector processors 
    will otherwise access improperly.  For example the PowerPC doesn't even use the 
    lower address lines!  So It'll actually load the wrong memory if not properly 
    aligned!
     ( J.Leiterman )
**/
typedef struct 
{
    // block data
#ifdef OPT_MEM
    short *block;
#else
    short block[64];
#endif
    // bit input
#ifndef _DECORE
    FILE * infile;
    unsigned char rdbfr[2051];
#endif    
    const unsigned char *startptr;
    const unsigned char *rdptr;
#ifndef _DECORE
    unsigned char inbfr[16];
#endif
    int incnt;
    int bitcnt;

    // this int are supposed to be always aligned, I'm going to use always these two 
    // mini-buffers to access to the stream. It's faster! (thanks to Lionel Ulmer)
    unsigned int bit_a, bit_b; 
    unsigned int length;


} 
MP4_STREAM;

typedef struct _ac_dc
{
    int *prev_ac_lum; 
    int prev_dc_lum[2];
    int *prev_ac_lum_left;
    int *prev_ac_chr[2];
    int *prev_ac_chr_left[2];
    int prev_dc_chr[2];
    int ac_top_lum_stride;
    int predict_dir;

} ac_dc;

typedef struct _mp4_header {



    int dc_scaler;
    int ac_pred_flag;
    int quant_type;
    int intra_dc_vlc_thr;
    int use_intra_dc_vlc;
    int quantizer;
    // svh
    int short_video_header;

    int mcsel;
    int quarter_pixel;
    int old_prediction_type;
    int rounding_type;
    int fcode_for;
    int fcode_back;


    // extra/derived
#ifndef OPT_MEM
    int mba_size;       //Lobo 31/01/2008 To be removed from MB loop
    int mba;            //Lobo 31/01/2008 To be removed from MB loop
#endif    
    // MPEG-2 quant matrices
       unsigned int SsbSipVideoDivXintra_quant_matrix[64];
    unsigned int SsbSipVideoDivXnonintra_quant_matrix[64];

    int shape;
    int time_increment_resolution;

    int sprite_usage;
        


    int complexity_estimation_disable;
    int resync_marker_disable;
    int data_partitioning;
    int reversible_vlc;

    int scalability;

    // complexity estimation
    int estimation_method;

    int opaque;
    int transparent;
    int intra_cae;
    int inter_cae;
    int no_update;
    int upsampling;

    int intra_blocks;
    int inter_blocks;
    int inter4v_blocks;
    int not_coded_blocks;

    int dct_coefs;
    int dct_lines;
    int vlc_symbols;
    int vlc_bits;
    int motion_compensation_complexity_disable;
    int apm;
    int npm;
    int interpolate_mc_q;
    int forw_back_mc_q;
    int halfpel2;
    int halfpel4;
    int version2_complexity_estimation_disable;
    int sadct;
    int quarterpel;
    int newpred_enable;


    // vop

    int last_coded_prediction_type;
// Lobo. 31/01/2008  prediction_type Used rarely although in svh it's used within MB. 
// SVH MB processing is not optimized like non SVH
    int prediction_type; 
    int old_time_base;
    int time_base;
    int time_inc;
    int vop_coded;
//    int rounding_type;


    int display_time_next;
    int display_time_prev;
    int tframe; // see paragraph 7.7.2.2

    // video packet
    int macroblock_number;



    // SsbSipVideoDivXmacroblock

    int picnum;
    int packetnum; // if ~0 indicates the presece of at least 1 video packet
    int gobnum; // needed for short header decoding



    int num_mb_in_gob; // shv
    int num_gobs_in_vop; // shv

    int mb_in_vop_length;
    int resync_length;


    int no_of_sprite_warping_points;
    int warping_points[4][2];
    int sprite_warping_accuracy;
    int sprite_brightness_change;
    int sprite_brightness_change_factor;
                                                     // the ac rescaling have been applied to avoid to repeat it
    // 3.11 specific values
    short (*dc_chrom_table) (MP4_STREAM*);
    short (*dc_lum_table) (MP4_STREAM*);
    event_t (*ac_inter_table) (MP4_STREAM*);
    event_t (*ac_intra_chrom_table) (MP4_STREAM*);
    event_t (*ac_intra_lum_table) (MP4_STREAM*);
    void (*mv_table) (MP4_STREAM*, int*, int*);
    short (*get_cbp) (MP4_STREAM*);
    int has_skips;
    int vol_mode; /* see comment in mp4_header_311.c */                                                     
    int switch_rounding;
        // the ac rescaling have been applied to avoid to repeat it

    // interlace
    int top_field_first;
    int alternate_vertical_scan_flag;
    int dct_type;
    int field_prediction;
    int forward_top_field_reference;
    int forward_bottom_field_reference;
    int backward_top_field_reference;
    int backward_bottom_field_reference;

} mp4_header;


typedef struct 
{
  char corners[4];
  char top[DEC_MBC];
  char bottom[DEC_MBC];
  char left[DEC_MBR];
  char right[DEC_MBR];

} mp4_edge_info;




typedef struct
{
    unsigned int* deviations[3];
    unsigned char* history1;
    unsigned char* history2;
} DERING_INFO;



typedef struct
{
    int64_t mmw_brightness;
    int64_t mmw_contrast;
    int64_t mmw_saturation;
    int brightness;
    int contrast;
    int saturation;
} GAMMA_ADJUSTMENT;


typedef struct
{
    int X0, Y0;
    short XX, YX, XY, YY;
    int rounder1, rounder2;
    int64_t shifter;
} AFFINE_TRANSFORM;

struct _REFERENCE;

typedef int (VldProc)(struct _REFERENCE * ref,unsigned int * zigzag,int * i,int * m);
typedef VldProc * VldProcPtr;

typedef int (VldProcIntra)(struct _REFERENCE * ref,unsigned int * zigzag,int * i,short * Quantblock);
typedef VldProcIntra * VldProcIntraPtr;
struct _MP4_STATE_;
typedef void (SsbSipVideoDivXReconFunc)(struct _MP4_STATE_* mp4_state, unsigned char *src, unsigned char *dst,
                          int lx, int lx_dst, int x, int y, int dx, int dy, int interlaced);
typedef SsbSipVideoDivXReconFunc* ReconFuncPtr;
typedef void (SsbSipVideoDivXReconBlockFunc)(struct _MP4_STATE_* mp4_state, unsigned char *src, unsigned char *dst,
                          int lx, int lx_dst, int x, int y, int dx, int dy, int chrom, int interlaced);
typedef SsbSipVideoDivXReconBlockFunc* ReconBlockFuncPtr;
typedef struct 
{
    int x, y;
}MotionVector;
#include "SsbSipVideoDivXyuv2rgb.h"

typedef struct _MP4_STATE_
{    
    int mb_xpos;
    int mb_ypos;
    int    coded_picture_width;
    int    coded_picture_height;
    int mb_xsize;
    int mb_ysize;
    int gmcFlag;
    int interlaced;
    int trd;
    int trb;
    int    horizontal_size;
    int    vertical_size;
    int edge_hor_start;
    int edge_ver_start;
    int    mb_width;
    int    mb_height;


    int    chrom_width;
    int    chrom_height;
    unsigned int * zigzag;
#ifndef OPT_IDCT            
    unsigned int * ptrMaxFilledColsArray;
#endif
    VldProcPtr vld_inter_fun;
    VldProcIntraPtr vld_intra_fun;

    mp4_header hdr;



    int *modemap; // modemap[DEC_MBR+1][DEC_MBC+2]
    int *codedmap; //codedmap[DEC_MBR][DEC_MBC]
    short *cbp_store; // cbp_store[DEC_MBR+1][DEC_MBC+1]
    char *quant_store; // quant_store[DEC_MBR+1][DEC_MBC+1]
    MotionVector *MV; // MV[DEC_MBR+1][DEC_MBC+2][6][2]
//#ifndef _MFB_OPT_  made dummy to avaoid segmenattion fault
    MotionVector *MV_fowbak;
//#endif

    int *fieldpredictedmap; 
    int fieldpredictedmap_stride;
    char *fieldrefmap; 
    int fieldrefmap_stride;

    MotionVector (*MV_field)[6];

    int modemap_stride;
    int codedmap_stride;
    int cbp_store_stride;
    int quant_store_stride;
    int MV_stride;


    // b-vop


    unsigned char** frame_to_decode; 
    unsigned char** frame_to_display; // must create a delay when encounter the first B-VOP
    unsigned char** output_frame;   // always matches the frame we use as input for color conversion
    int prefixed; // avoid delay caused by B-VOPs using prefixed information, no_delay_frame_flag is on
    int history_prefixed; // there was a prefixed I-VOP, b_vop_grouping is on
    int preceding_vop_coding_type; 

    mp4_edge_info edge_info;   // This is necessary for 311 version


    ac_dc* coeff_pred;

#ifdef YUV2RGB
    yuv2rgbProcPtr convert_yuv;
    yuv2rgbProcPtr convert_yuv_fast;
    yuv2rgbProcPtr convert_yuv_generic;
    int bpp;
    int flag_invert;
#endif


    int pp_options;
    int postproc_level;


    int flag_keyframe; // indicates that the current frame is a keyframe
    int flag_smooth_playback; // delay one frame, copy B frames into private buffer, decode one frame at a time
    int flag_seek_bframe;
    int flag_buffered_bframe;
    char* buffered_bframe;
    int buffered_bframe_length;
    int flag_warmth;
    int flag_logo;
    /**
    flag_skip_decoding: allows the decoder to ignore the data and stop decoding for a while

    When the decoder receives skip_decoding set this flag to 1 and returns 0, when the decoder
    has this flag set to 1, if the frame received is a keyframe set this flag to 0 and decodes, otherwise
    returns 0.
    **/
    int flag_skip_decoding; 
    int multiplier;

    // user data info
    int userdata_codec_version;
    int userdata_build_number;
    int bad_header; /* Contains error returned when parsing last received VOL header */ 
    unsigned char * bmp;
    int stride;


    SsbSipVideoDivXCopyAreaProcPtr block_pointers[4];
    SsbSipVideoDivXCopyAreaProcPtr mblock_pointers[4];

    SsbSipVideoDivXCopyAreaProcPtr block_pointers_field[4];
    SsbSipVideoDivXCopyAreaProcPtr mblock_pointers_field[4];
    int trbi;
    int trdi;

    void (*MacroblockDisplay)(struct _REFERENCE *, int, int); // the function that is called each SsbSipVideoDivXmacroblock. can do nothing
    void (*MacroblockDisplayFinish)(struct _REFERENCE *, unsigned char *, unsigned int); // the function that is called in the end of each frame. Does YUV->RGB and preprocessing. Can call MacroblockDisplay2 and MacroblockDisplayFinish2.


    void (*set_gmc_mv) (struct _MP4_STATE_*);
    void (*SsbSipVideoDivXreconstruct_skip) (struct _REFERENCE*);


    ReconFuncPtr recon_16x16;
    ReconBlockFuncPtr recon_8x8_lum;
    ReconBlockFuncPtr recon_8x8_chr;

    int memory_cheat;
    int render_flag;

    int test_timeinc;


#ifdef DERING
    DERING_INFO di;
#endif

#ifdef WIN32
    __declspec(align(8))
#endif

#ifdef YUV2RGB
    GAMMA_ADJUSTMENT ga;
#endif

    AFFINE_TRANSFORM at_lum;
    AFFINE_TRANSFORM at_chrom;


#ifdef _ROUNDING_TYPE_TO_FILE
    FILE* rounding_type_file;
#endif


    logo_adder_t logo_adder;

#ifndef _DECORE
    char outputname[256];
    char infilename[256];
    int output_flag;
#endif

   int TimeBaseLast;
   int NonBVopTimeLast;
   unsigned char *pMbType;
   unsigned int packed_mode;


} 
MP4_STATE;

#ifdef OPT_MEM

typedef struct _REFERENCE
{    
    MP4_STREAM  *ld;
    MP4_STATE    *mp4_state;
    unsigned char *edged_ref[3];
    unsigned char *edged_for[3];
    unsigned char *edged_back[3];
    unsigned char *frame_ref[3];
    unsigned char *frame_for[3];
    unsigned char *frame_back[3];
    unsigned char *display_frame[3];


    void* (*alloc_fun) (uint32_t);
    void (*free_fun) (void*);
} REFERENCE;
#else

typedef struct _REFERENCE
{    
    unsigned char *edged_ref[3];
    unsigned char *edged_for[3];
    unsigned char *edged_back[3];
    unsigned char *frame_ref[3];
    unsigned char *frame_for[3];
    unsigned char *frame_back[3];
    unsigned char *display_frame[3];

    MP4_STATE    *mp4_state;
    MP4_STREAM  *ld;

    void* (*alloc_fun) (uint32_t);
    void (*free_fun) (void*);
} REFERENCE;

#endif
/** 
 *    prototypes of main SsbSipVideoDivXdecore functions
**/

int SsbSipVideoDivXdecore_frame (REFERENCE * ref, 
    const unsigned char *stream, 
    int length, 
    unsigned char *bmp, 
    unsigned int stride, 
    int render_flag,
    int just_vol_init,
    int skip_decoding);

int SsbSipVideoDivXdecore_alloc(REFERENCE * ref, void *cmm_handle);
int SsbSipVideoDivXdecore_dealloc(REFERENCE * ref);
int SsbSipVideoDivXdecore_release (REFERENCE * ref);
int SsbSipVideoDivXdecore_checkoutput(const DivXBitmapInfoHeader* pHeader);
int SsbSipVideoDivXdecore_setoutput(MP4_STATE * mp4_state, const DivXBitmapInfoHeader* pHeader); 
int SsbSipVideoDivXget_mp4picture (REFERENCE * ref, unsigned char *bmp, unsigned int stride, int render_flag);
int SsbSipVideoDivXget_notcoded_mp4picture (REFERENCE * ref, unsigned char *bmp, unsigned int stride, int render_flag);

/*
// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------
*/

typedef void (SsbSipVideoDivXdecore_cleanup_proc) ();
typedef SsbSipVideoDivXdecore_cleanup_proc* SsbSipVideoDivXdecore_cleanup_proc_ptr;
EXPORT SsbSipVideoDivXdecore_cleanup_proc_ptr SsbSipVideoDivXdecore_cleanup;    

EXPORT SsbSipVideoDivXdecore_cleanup_proc SsbSipVideoDivXdecore_cleanup_generic;
EXPORT SsbSipVideoDivXdecore_cleanup_proc SsbSipVideoDivXdecore_cleanup_mmx;
#ifdef __cplusplus
}
#endif

#endif // ___SSBSIPMP4_VARS_H__

