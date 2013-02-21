



#ifndef _SSBSIP_DECODER_H_
#define _SSBSIP_DECODER_H_


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<malloc.h>




#define sInt8   char
#define sInt16  short
#define sInt32  int
#define sInt64  __int64

#define uInt8  unsigned char
#define uInt16 unsigned short
#define uInt32 unsigned int
#define uInt64 unsigned __int64

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++ 
#endif    // __cplusplus *


#define __IO_ERROR__    -1





//ISO/IEC 14496-2:2004(E) Page:39 Table 6.3
#define VIDEO_OBJECT_START_CODE            0x00000100    // 0x00000100 --- 0x0000011F
#define VIDEO_OBJECT_LAYER_START_CODE    0x00000120    // 0x00000120 --- 0x0000012F
#define VISUAL_OBJECT_SEQ_START_CODE    0x000001B0
#define VISUAL_OBJECT_SEQ_END_CODE        0x000001B1    
#define USER_DATA_START_CODE            0x000001B2
#define GROUP_OF_VOP_START_CODE            0x000001B3
#define VIDEO_SESSION_ERROR_CODE        0x000001B4 
#define VISUAL_OBJECT_START_CODE        0x000001B5
#define VOP_START_CODE                    0x000001B6
#define STUFFING_START_CODE                0x000001C3
#define SHORT_VIDEO_HEADER                0x00000009 // defined by me


//Table6.16 video object layer shape type page 145
#define RECTANGULAR        0
#define BINARY            1
#define BINARY_ONLY        2
#define GRAY_SCALE        3


//Table B-1 page 22
#define NOT_CODED_MB        -1    //by me

#define INTER_MB            0
#define INTER_Q_MB            1
#define INTER_4V_MB            2
#define INTRA_MB            3
#define INTRA_Q_MB            4

#define GMC_MB                5    //by me
#define NOT_CODED_GMC_MB    6    //by me
#define STUFFING            7    //by me




#define MODB_1                0
#define MODB_00                1
#define MODB_01                2

#define DIRECT                0
#define INTERPOLATE_MC_Q    1
#define BACKWARD_MC_Q        2
#define FORWARD_MC_Q        3

//#define    NOT_CODED_MB        6
//#define    NOT_CODED_MB_GMC    7
#define    DIRECT_WITH_ZERO_DELTA    8


//Table 6.6 Meaning of visual object type, page 137
#define VIDEO_ID            1
#define STILL_TEXTURE_ID    2
#define MESH_ID                3
#define FBA_ID                4
#define THREED_MESH_ID        5

#define START_CODE_24BIT    0x000001
#define SHORT_VIDEO_START_MARKER 0x20    //ITS 22 BIT VALUE PAGE 160

#define FEATURE_NOT_SUPPORTED    -64

#define EXTENDED_PAR    15

#define STATIC    1
#define GMC        2

#define I_FRAME    0
#define P_FRAME    1
#define B_FRAME    2
#define S_FRAME    3
#define NOT_CODED_FRAME    4

#define I    0
#define P    1
#define B    2
#define S    3
#define N    4

#define    ESCAPE 3

#define    FORWARD        1
#define    BACKWARD    2



#define    ZIG_ZAG_SCANING        0
#define    HORIZONTAL_SCANING    1
#define    VERTICAL_SCANING    2

#define NO_FRAME_DISPLAY -1

#define DC 0
#define AC 1
#define VL 2
#define HL 3
#define SQ 4



#define SUCCESS                            0
#define INPUT_STREAM_NOT_SUFFICIENT        -1
#define VISUAL_OBJECT_IS_NOT_VIDEO        -2
#define NO_DATA_TO_DECODE_VOP            -3    
#define VIDEO_IS_NOT_RECTANGULAR_SHAPE    -4
#define VIDEO_IS_INTERLACED                -5
#define PIXEL_SIZE_IS_NOT_8_BIT            -6
#define COMPLEX_HEADER_PRESENT            -7
#define DATA_PARTION_ENABLED            -8
#define REDUCED_RESOLUTION_VOP_PRESENT    -9
#define VIDEO_SCALABILITY_ENABLED        -10
#define SHORT_VIDEO_HEADER_PRESENT        -11


extern char ERROR_MESSAGES[16][255];



//acdc prediction
#define nBLKS 6
#define ACDC_BLK_PRED_BUFFER_SIZE    16
#define ACDC_MB_PRED_BUFFER_SIZE    (nBLKS*ACDC_BLK_PRED_BUFFER_SIZE)

#define ACDC_PRED_BUFFER_OFFSET_1BLK (ACDC_BLK_PRED_BUFFER_SIZE)
#define ACDC_PRED_BUFFER_OFFSET_2BLK (2*ACDC_BLK_PRED_BUFFER_SIZE)
#define ACDC_PRED_BUFFER_OFFSET_3BLK (3*ACDC_BLK_PRED_BUFFER_SIZE)
#define ACDC_PRED_BUFFER_OFFSET_2CB_BLK (4*ACDC_BLK_PRED_BUFFER_SIZE)
#define ACDC_PRED_BUFFER_OFFSET_2CR_BLK (5*ACDC_BLK_PRED_BUFFER_SIZE)


#define BSWAP(a) \
    ((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
     (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

extern unsigned char Lut_intra_quant_default_matrix[64];
extern unsigned char Lut_nonintra_quant_defualt_matrix[64];
extern unsigned char Lut_ZigZag_Scan[64];
extern unsigned int Lut_LOG2[16] ;


#define PAD_SIZE  32
#define PAD_SIZEc  (PAD_SIZE/2)

typedef struct
{
    unsigned int word1;
    unsigned int word2;
    unsigned int pos;
    unsigned int length;
    unsigned int *pBase;
    unsigned int *pCur;
    int prev_offset;
    
}BitStream;


typedef struct MP4_GMC {

    int num_warping_points;
    int warping_accuracy;
    int real_warping_points;

    int du[3];
    int dv[3];

    int offset[2][2];
    int delta[2][2];
    int shift[2];
    
}MP4_GMC;

#define __XVID_GMC__

typedef struct _XVID_GMC_DATA
{
    int num_warping_points;
    int warping_accuracy;
    int real_warping_points;

    int du[3];
    int dv[3];


   /*  0=none, 1=translation, 2,3 = warping
    *  a value of -1 means: "structure not initialized!" */
    int num_wp;

    /* {0,1,2,3}  =>   {1/2,1/4,1/8,1/16} pel */
    int accuracy;

    /* sprite size * 16 */
    int sW, sH;

    /* gradient, calculated from warp points */
    int dU[2], dV[2], Uo, Vo, Uco, Vco;

    void (*predict_16x16)( struct _XVID_GMC_DATA *This, unsigned char *dst, unsigned char *src,  int srcstride, int x, int y, int rounding);
    void (*predict_8x8)  ( struct _XVID_GMC_DATA *This, unsigned char *uDst, unsigned char *uSrc,  unsigned char *vDst, unsigned char *vSrc,  int srcstride, int x, int y, int rounding);
    void (*get_average_mv)( struct _XVID_GMC_DATA *Dsp, short *pmv,  int x, int y, int qpel, int fcode);

} XVID_GMC_DATA;

typedef struct {
    
    int mb_type;                             // vlc(1-4);
    unsigned int cbp;                         // vlc(3-6);

}MPSASP_MB;


typedef struct
{
    unsigned char *pImg_Y;
    unsigned char *pImg_U;
    unsigned char *pImg_V;

}MPASP_FRAME;

typedef struct
{
    //VISUAL OBJECT
    unsigned int visual_obiect_verid;                // ugetbits(4);


    //VideoObjectLayer() 
    unsigned int low_delay;                            // ugetbits(1);
    unsigned int vop_time_increment_resolution;        // ugetbits(16);
    unsigned int bits_for_vop_time_increment_resolution; //defined by me
    unsigned int interlaced;                        // getbits(1);
    unsigned int sprite_enable;                        // getbits(1-2);
    unsigned int no_of_sprite_warping_points;        // ugetbits(6);
    unsigned int sprite_warping_accuracy;            // ugetbits(2);
    unsigned int sprite_brightness_change;            // getbits(1);
    unsigned int low_latency_sprite_enable;            // getbits(1);
    unsigned int quant_type;                        // getbits(1);
    unsigned char *intra_quant_mat;                    // ugetbits(8(2-64));
    unsigned char *nonintra_quant_mat;                // ugetbits(8(2-64));
    unsigned int quarter_sample;                    // getbits(1);
    unsigned int newpred_enable;                    // getbits(1);
    unsigned int short_video_header;


    // VideoObjectPlane() 
    unsigned int vop_coding_type;                    // ugetbits(2);
    unsigned int vop_coded;                            // getbits(1);
    unsigned int vop_rounding_type;                    // getbits(1);
    unsigned int vop_quant;                            // ugetbits(3-9);
    unsigned int use_intra_dc_vlc;                    // derived value
    unsigned int vop_fcode_forward;                    // ugetbits(3);
    unsigned int vop_fcode_backward;                // ugetbits(3);
    unsigned int preceding_vop_coding_type;        

    __int64 time;                
    __int64 non_Bvop_cum_time_base;
    __int64 prev_non_Bvop_cum_time_base;
    __int64 non_Bvop_time;
    int TRB;
    int TRD;


    //void macroblock_I()
    unsigned int ac_pred_flag;                         // get_bits(1);
    int *pinter_idct_type;
    int rag_idct_type;

    
    //void block();
    unsigned int intra_dc_coefficient;                                // get_bits(8);

    MPASP_FRAME frame;
    MPASP_FRAME ref_frame[2];    
    short *pResdues;
            

    int mb_width;
    int mb_height;
    int width;
    int height;
    int pad_width;
    int pad_height;



    unsigned char *quant;
    short *pMV;
    short *pMV_bck;
    short *pMV_for;

    MPSASP_MB *pMB;
    MPSASP_MB *pPrv_MB;

    unsigned int first_time_buff_init;
    short *pAcDcPredBuff;
    unsigned char *pintratype;

    //MP4_GMC *gmc;
    XVID_GMC_DATA *gmc;

    int is_paded[2];

    int low_delay_default;
    int packed_mode;
    int last_frame_type;
    int frames;
    int type;

    unsigned char *pYUVptr;



}ASP_DEC;


extern BitStream *stream;
BitStream *BitstreamInit(unsigned char *pbitstream,  unsigned int length);
unsigned int getbits(int req_bits);

unsigned int read_bits(BitStream *stream, int req_bits);

void stream_forward(BitStream *stream,  int req_bits);
int next_bits(BitStream *stream, int nbit);

int bytealign(BitStream *stream);
void next_start_code(BitStream *stream);










int vlc_decod_mcbpc(BitStream *stream, int *cbpc);










int Decode_I_Frame(BitStream *stream, ASP_DEC *AspDec);
int Decode_P_Frame(BitStream *stream, ASP_DEC *AspDec);
int Decode_S_Frame(BitStream *stream, ASP_DEC *AspDec);
int Decode_B_Frame(BitStream *stream, ASP_DEC *AspDec);
int Decode_NotCoded_Frame(BitStream *stream, ASP_DEC *AspDec);


void vop_padding(MPASP_FRAME *frame, int pad_width, int pad_height, int width, int height);



void get_residues(ASP_DEC *AspDec, BitStream *stream, int iQuant, int cbp, short *pResdues);

void idct (short *block);

short *idct_inter1(int stride, short *pRes,  int cbp, unsigned char *pImg_Y,  unsigned char *pImg_U, unsigned char *pImg_V, int rag_idct_type);
void do_QPEL_8(unsigned char *pDst, unsigned char *pRef, int x, int y, short *pMV, int strideS, int strideD,int rnd);
void do_QPEL_16(unsigned char *pDst, unsigned char *pRef, int flag, int strideS, int strideD, int rnd);


//void do_hpel_8(unsigned char *pDst,  unsigned char *pRef,  int strideS, int strideD, int rnd, int flag)
void do_hpel_8(unsigned char *pDst,  unsigned char *pRef,  int stride, int strided, int rnd, int flag);
void do_hpel_16(unsigned char *pDst,  unsigned char *pRef, int stride, int strided, int rnd, int flag);


void copy_img2mb(unsigned char *pDst, unsigned char *pSrc, int strideS, int strideD);


// all

int get_coeff_vlc(BitStream *stream, int *last1, int *run1, int *level1, int intra_flag);


//void set_blk_zero(short *blk1);

//i

void idct_intra (short *block, unsigned char *pCurImg, int stride);

//p

int vlc_decod_mcbpc_p(BitStream *stream);

//s

void gmc_frame_init(ASP_DEC *AspDec);

void get_amv(ASP_DEC *AspDec, int mb_x, int mb_y);

//i and p
int vlc_decod_cbpy(BitStream *stream, int is_inter);

//intra
int get_dct_dc_size_luma_vlc(BitStream *stream);
int get_dct_dc_differential_vlc(BitStream *stream, int dct_dc_size);
int get_dc_scaler(int Qp, int luma_flag);
int get_dct_dc_size_croma_vlc(BitStream *stream);

//void acdc_pred(ASP_DEC *AspDec, int dc_scaler, int mb_x, int mb_index, int *direction, int block_num, short *cur_blk_preds);
//void acdc_add(ASP_DEC *AspDec, short *block, int dc_scaler, int mb_index,int acpred_direction, int block_num, short *cur_blk_preds);

void acdc_pred(ASP_DEC *AspDec, int dc_scaler, int mb_x, int mb_index, int *direction, int block_num, short *cur_blk_preds);
void acdc_add(ASP_DEC *AspDec, short *block, int dc_scaler, int mb_index,int acpred_direction, int block_num,  short *cur_blk_preds);


void block(BitStream *stream, ASP_DEC *AspDec, int mb_x, int mb_y, int block_num, unsigned char *pImgblk);

//b frame
int get_modb_vlc(BitStream *stream);
int get_mb_type_vlc(BitStream *stream);
int get_dbqauant_vlc(BitStream *stream);

// p and b
int get_mv_data(BitStream *stream);




void Assain_3D_short_memory(short ****base, int Z_DIM, int Y_DIM, int X_DIM);
void free_3D_short_memory(short ***array3D, int nmbs);


int Read_Header(BitStream *stream, ASP_DEC *AspDec);
int SsbSipMP4ASPDecExe(BitStream *stream, ASP_DEC *AspDec);

typedef void (*get_predictors_ptr)(    ASP_DEC *AspDec, int mb_type, int mb_x, int mb_y, int rounding, int ref_flag, int bvop, unsigned char *pCurImg_Y, unsigned char *pCurImg_U, unsigned char *pCurImg_V);




#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif    /* __cplusplus */


#endif //_SSBSIP_DECODER_H_