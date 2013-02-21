//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

/**
 * @file    fgl.h
 * @brief    This is the header file for fgl library.
 * @version    1.5
 */

#if !defined(__FIMG_H__)
#define __FIMG_H__

#include "fglconfig.h"

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************
 *  Typedefs
 ****************************************************************************/
typedef unsigned int         FGL_BOOL;
typedef unsigned int         FG_BOOL;

/****************************************************************************
 *  Defines
 ****************************************************************************/
#define FGL_TRUE     (1==1)
#define FGL_FALSE     (!(FGL_TRUE))
#define FGL_NULL     ((void*)0)

#define FGL_ZERO         0
#define FGL_ONE          1
#if 0
#define NO_ERROR          0
#define ERROR             -1
#define NOT_SUPPORTED     -2    /* test not supported by this config */
#endif
/* Vertex/Pixel Shader */
#define BUILD_SHADER_VERSION(major, minor)    (0xFFFF0000 | (((minor)&0xFF)<<8) | ((major) & 0xFF))
#define VERTEX_SHADER_MAGIC                 (((('V')&0xFF)<<0)|((('S')&0xFF)<<8)|(((' ')&0xFF)<<16)|(((' ')&0xFF)<<24))
#define PIXEL_SHADER_MAGIC                     (((('P')&0xFF)<<0)|((('S')&0xFF)<<8)|(((' ')&0xFF)<<16)|(((' ')&0xFF)<<24))
#define SHADER_VERSION                         BUILD_SHADER_VERSION(3,0)

#define SET_BIT(n)        (1<<n)

#define FIMG_3DSE_V121                  0x01020100
#define FIMG_3DSE_V150                  0x01050000
#define FIMG_3DSE_V200                  0x02000000
/****************************************************************************
 *  Macros
 ****************************************************************************/

/** GLOBAL REGISTER PIPELINE STATE BIT POSITION */
#define FGL_PIPESTATE_HOSTFIFO_BIT        (0)
#define FGL_PIPESTATE_HI_BIT            (1)
#define FGL_PIPESTATE_HI2VS_BIT            (2)
#define FGL_PIPESTATE_VC_BIT            (3)
#define FGL_PIPESTATE_VS_BIT            (4)
#define FGL_PIPESTATE_PE_BIT            (8)
#define FGL_PIPESTATE_TSE_BIT            (9)
#define FGL_PIPESTATE_RA_BIT            (10)
#define FGL_PIPESTATE_PS0_BIT            (12)
#define FGL_PIPESTATE_PS1_BIT            (13)
#define FGL_PIPESTATE_PF0_BIT            (16)
#define FGL_PIPESTATE_PF1_BIT            (17)
#define FGL_PIPESTATE_CCACHE0_BIT        (18)
#define FGL_PIPESTATE_CCACHE1_BIT        (19)

#define FGL_PIPESTATE_HOSTFIFO        (1)
#define FGL_PIPESTATE_HI            (1 << FGL_PIPESTATE_HI_BIT)
#define FGL_PIPESTATE_HI2VS            (1 << FGL_PIPESTATE_HI2VS_BIT)
#define FGL_PIPESTATE_VC            (1 << FGL_PIPESTATE_VC_BIT)
#define FGL_PIPESTATE_VS            (1 << FGL_PIPESTATE_VS_BIT)
#define FGL_PIPESTATE_PE            (1 << FGL_PIPESTATE_PE_BIT)
#define FGL_PIPESTATE_TSE            (1 << FGL_PIPESTATE_TSE_BIT)
#define FGL_PIPESTATE_RA            (1 << FGL_PIPESTATE_RA_BIT)
#define FGL_PIPESTATE_PS0            (1 << FGL_PIPESTATE_PS0_BIT)
#define FGL_PIPESTATE_PS1            (1 << FGL_PIPESTATE_PS1_BIT)
#define FGL_PIPESTATE_PF0            (1 << FGL_PIPESTATE_PF0_BIT)
#define FGL_PIPESTATE_PF1            (1 << FGL_PIPESTATE_PF1_BIT)
#define FGL_PIPESTATE_CCACHE0        (1 << FGL_PIPESTATE_CCACHE0_BIT)
#define FGL_PIPESTATE_CCACHE1        (1 << FGL_PIPESTATE_CCACHE1_BIT)

//FGL_PIPESTATE_X_TILL_Y = or of bits in [X,Y) including X, excluding Y

#define FGL_PIPESTATE_HI_TILL_PE        (FGL_PIPESTATE_HOSTFIFO \
                                        | FGL_PIPESTATE_HI      \
                                        | FGL_PIPESTATE_HI2VS   \
                                        | FGL_PIPESTATE_VC      \
                                        | FGL_PIPESTATE_VS )


#define FGL_PIPESTATE_HI_TILL_PS        (FGL_PIPESTATE_HOSTFIFO \
                                        | FGL_PIPESTATE_HI      \
                                        | FGL_PIPESTATE_HI2VS   \
                                        | FGL_PIPESTATE_VC      \
                                        | FGL_PIPESTATE_VS      \
                                        | FGL_PIPESTATE_PE      \
                                        | FGL_PIPESTATE_TSE     \
                                        | FGL_PIPESTATE_RA  )
                
#ifndef _FIMG_PIPELINE_SINGLE
    #define FGL_PIPESTATE_PS            (FGL_PIPESTATE_PS0 \
                                        | FGL_PIPESTATE_PS1)
    #define FGL_PIPESTATE_PF            (FGL_PIPESTATE_PF0 \
                                        | FGL_PIPESTATE_PF1)
    #define FGL_PIPESTATE_CCACHE        (FGL_PIPESTATE_CCACHE0 \
                                        | FGL_PIPESTATE_CCACHE1)
#else // _FIMG_PIPELINE_SINGLE
    #define FGL_PIPESTATE_CCACHE        FGL_PIPESTATE_CCACHE0
    #define FGL_PIPESTATE_PS            FGL_PIPESTATE_PS0
    #define FGL_PIPESTATE_PF            FGL_PIPESTATE_PF0
#endif // _FIMG_PIPELINE_SINGLE

#define FGL_PIPESTATE_ALL                (FGL_PIPESTATE_HOSTFIFO \
                                        | FGL_PIPESTATE_HI \
                                        | FGL_PIPESTATE_HI2VS \
                                        | FGL_PIPESTATE_VC \
                                        | FGL_PIPESTATE_VS \
                                        | FGL_PIPESTATE_PE \
                                        | FGL_PIPESTATE_TSE \
                                        | FGL_PIPESTATE_RA \
                                        | FGL_PIPESTATE_PS \
                                        | FGL_PIPESTATE_PF)

// for Blend
#define FGL_PIPESTATE_ALL_WITH_CCACHE    (FGL_PIPESTATE_HOSTFIFO \
                                        | FGL_PIPESTATE_HI \
                                        | FGL_PIPESTATE_HI2VS \
                                        | FGL_PIPESTATE_VC \
                                        | FGL_PIPESTATE_VS \
                                        | FGL_PIPESTATE_PE \
                                        | FGL_PIPESTATE_TSE \
                                        | FGL_PIPESTATE_RA \
                                        | FGL_PIPESTATE_PS \
                                        | FGL_PIPESTATE_PF \
                                        | FGL_PIPESTATE_CCACHE)

/** GLOBAL REGISTER CACHE STATE BIT POSITION */
#define FGL_CACHECTL_FLUSH_ZCACHE0    (1)
#define FGL_CACHECTL_FLUSH_ZCACHE1    (1 << 1)
#define FGL_CACHECTL_FLUSH_CCACHE0    (1 << 4)
#define FGL_CACHECTL_FLUSH_CCACHE1    (1 << 5)
#define FGL_CACHECTL_CLEAR_TCACHE0    (1 << 8)
#define FGL_CACHECTL_CLEAR_TCACHE1    (1 << 9)
#define FGL_CACHECTL_CLEAR_VTCACHE    (1 << 12)

#ifndef _FIMG_PIPELINE_SINGLE
    #define FGL_CACHECTL_FLUSH_ZCACHE    (FGL_CACHECTL_FLUSH_ZCACHE0 \
                                        | FGL_CACHECTL_FLUSH_ZCACHE1)
    #define FGL_CACHECTL_FLUSH_CCACHE    (FGL_CACHECTL_FLUSH_CCACHE0 \
                                        | FGL_CACHECTL_FLUSH_CCACHE1)
    #define FGL_CACHECTL_CLEAR_TCACHE    (FGL_CACHECTL_CLEAR_TCACHE0 \
                                        | FGL_CACHECTL_CLEAR_TCACHE1)
#else // _FIMG_PIPELINE_SINGLE
    #define FGL_CACHECTL_FLUSH_ZCACHE    FGL_CACHECTL_FLUSH_ZCACHE0
    #define FGL_CACHECTL_FLUSH_CCACHE    FGL_CACHECTL_FLUSH_CCACHE0
//    #define FGL_CACHECTL_CLEAR_TCACHE    FGL_CACHECTL_CLEAR_TCACHE0
//    Texture cache always 2 bit-checked
    #define FGL_CACHECTL_CLEAR_TCACHE    (FGL_CACHECTL_CLEAR_TCACHE0 \
                                        | FGL_CACHECTL_CLEAR_TCACHE1)
#endif // _FIMG_PIPELINE_SINGLE

#define FGL_CACHECTL_INIT_ALL        (FGL_CACHECTL_FLUSH_ZCACHE \
                                    | FGL_CACHECTL_FLUSH_CCACHE \
                                    | FGL_CACHECTL_CLEAR_TCACHE \
                                    | FGL_CACHECTL_CLEAR_VTCACHE)


/****************************************************************************
 *  Enumerated types
 ****************************************************************************/

typedef enum FGL_ErrorTag
{
    FGL_ERR_UNKNOWN = 0,
    FGL_ERR_NO_ERROR,
    FGL_ERR_INVALID_PARAMETER,
    FGL_ERR_INVALID_PRIMITIVE,
    FGL_ERR_INVALID_WIDTH,
    FGL_ERR_INVALID_HEIGHT,
    FGL_ERR_INVALID_SIZE,
    FGL_ERR_INVALID_VALUE,
    FGL_ERR_INVALID_SHADER_CODE,
    FGL_ERR_STATUS_BUSY,
    FGL_ERR_NO_SUPPORT
} FGL_Error;

typedef enum FGL_CacheClearFlagsTag
{
    FGL_CLEAR_DEPTH_CACHE   = 3,
    FGL_CLEAR_COLOR_CACHE   = (3 << 4),
    FGL_CLEAR_TEX_CACHE     = (3 << 8),
    FGL_CLEAR_VTXTEX_CACHE  = (1 << 12)
} FGL_CacheClearFlags;

typedef enum FGL_ResetFlagsTag
{
    FGL_RESET_TEX_UNIT            = SET_BIT(0),
    FGL_RESET_PERFRAG_UNIT        = SET_BIT(1),
    FGL_RESET_PIXEL_SHADER        = SET_BIT(1),
    FGL_RESET_RASTER_UNIT        = SET_BIT(2),
    FGL_RESET_SETUP_ENGINE        = SET_BIT(3),
    FGL_RESET_GEOM_ENGINE        = SET_BIT(4),
    FGL_RESET_VERTEX_SHADER        = SET_BIT(5),
    FGL_RESET_HOST_INTERFACE    = SET_BIT(7)
} FGL_ResetFlags;

#if TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0
typedef enum FGL_ExeStateTag
{
    FGL_EXESTATE_IDLE    = 0,
    FGL_EXESTATE_SFRAD   = (1),
    FGL_EXESTATE_SFRAI   = (1 << 2),
    FGL_EXESTATE_VBWR0   = (1 << 3),
    FGL_EXESTATE_INDEX   = (1 << 6),
    FGL_EXESTATE_INDEXAI = (1 << 7),
    FGL_EXESTATE_INIT    = (1 << 24),
    FGL_EXESTATE_LOAD    = (1 << 25),
    FGL_EXESTATE_DECD    = (1 << 26),
    FGL_EXESTATE_INTR    = (1 << 27)
} FGL_ExeState;
#endif

// Host Interface
typedef enum FGL_IndexDataTypeTag
{
    FGL_INDEX_DATA_UINT = 0x80,
    FGL_INDEX_DATA_USHORT = 0x81,
    FGL_INDEX_DATA_UBYTE = 0x83
} FGL_IndexDataType;

typedef enum FGL_AttribDataTypeTag
{
    FGL_ATTRIB_DATA_BYTE = 0,
    FGL_ATTRIB_DATA_SHORT,
    FGL_ATTRIB_DATA_INT,
    FGL_ATTRIB_DATA_FIXED,
    FGL_ATTRIB_DATA_UBYTE,
    FGL_ATTRIB_DATA_USHORT,
    FGL_ATTRIB_DATA_UINT,
    FGL_ATTRIB_DATA_FLOAT,
    FGL_ATTRIB_DATA_NBYTE,
    FGL_ATTRIB_DATA_NSHORT,
    FGL_ATTRIB_DATA_NINT,
    FGL_ATTRIB_DATA_NFIXED,
    FGL_ATTRIB_DATA_NUBYTE,
    FGL_ATTRIB_DATA_NUSHORT,
    FGL_ATTRIB_DATA_NUINT
} FGL_AttribDataType;

typedef enum FGL_AttribCompOrderTag
{
    FGL_ATTRIB_ORDER_1ST = 0,
    FGL_ATTRIB_ORDER_2ND,
    FGL_ATTRIB_ORDER_3RD,
    FGL_ATTRIB_ORDER_4TH
} FGL_AttribCompOrder;

#if TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0
typedef enum FGL_AddrModeTag
{
    FGL_ADDR_MODE_MEMORY_CACHEABLE = 0,
    FGL_ADDR_MODE_VB,    
    FGL_ADDR_MODE_MEMORY_NON_CACHEABLE
} FGL_AddrMode;

typedef enum FGL_VBSizeModeTag
{
    FGL_NO_VB_8K_PREVC = 0,
    FGL_2K_VB_6K_PREVC,
    FGL_4K_VB_4K_PREVC,
    FGL_6K_VB_2K_PREVC
} FGL_VBSizeMode;
#endif

// Vertex Shader 

typedef enum FGL_AttribTableIdxTag
{
    FGL_INPUT_ATTRIB_IDX0,
    FGL_INPUT_ATTRIB_IDX1,
    FGL_INPUT_ATTRIB_IDX2,
    FGL_OUTPUT_ATTRIB_IDX0,
    FGL_OUTPUT_ATTRIB_IDX1,
    FGL_OUTPUT_ATTRIB_IDX2
} FGL_AttribTableIdx;


// Primitive Engine
typedef enum FGL_PrimitiveTag
{
    FGL_PRIM_TRIANGLES,
    FGL_PRIM_TRIANGLE_FAN,
    FGL_PRIM_TRIANGLE_STRIP,
    FGL_PRIM_LINES,
    FGL_PRIM_LINE_LOOP,
    FGL_PRIM_LINE_STRIP,
    FGL_PRIM_POINTS,
    FGL_PRIM_POINT_SPRITE
} FGL_Primitive;

typedef enum FGL_ShadingTag
{
    FGL_SHADING_FLAT,
    FGL_SHADING_SMOOTH
} FGL_Shading;


// Raster Engine
typedef enum FGL_SampleTag
{
    FGL_SAMPLE_CENTER,
    FGL_SAMPLE_LEFTTOP
} FGL_Sample;

typedef enum FGL_DepthOffsetParamTag {

    FGL_DEPTH_OFFSET_FACTOR,
    FGL_DEPTH_OFFSET_UNITS,
    FGL_DEPTH_OFFSET_R

} FGL_DepthOffsetParam;

typedef enum FGL_FaceTag
{
    FGL_FACE_BACK,
    FGL_FACE_FRONT,
    FGL_FACE_RESERVED,
    FGL_FACE_FRONT_AND_BACK = 3
} FGL_Face;

typedef enum FGL_LodCoeffTag
{
    FGL_LODCOEFF_DISABLE,
    FGL_LODCOEFF_ENABLE_LOD,
    FGL_LODCOEFF_ENABLE_DDX,
    FGL_LODCOEFF_ENABLE_DDX_LOD,
    FGL_LODCOEFF_ENABLE_DDY,
    FGL_LODCOEFF_ENABLE_DDY_LOD,
    FGL_LODCOEFF_ENABLE_DDY_DDX,
    FGL_LODCOEFF_ENABLE_ALL
} FGL_LodCoeff;

// Texture Units
typedef enum FGL_TexTypeTag
{
    FGL_TEX_2D = 1,
    FGL_TEX_CUBE,
    FGL_TEX_3D

} FGL_TexType;


typedef enum FGL_TexelFormatTag
{
    FGL_TEXEL_ARGB1555,
    FGL_TEXEL_RGB565,
    FGL_TEXEL_ARGB4444,
    FGL_TEXEL_DEPTH24,
    FGL_TEXEL_IA88,
    FGL_TEXEL_I8,
    FGL_TEXEL_ARGB8888,
    FGL_TEXEL_1BPP,
    FGL_TEXEL_2BPP,
    FGL_TEXEL_4BPP,
    FGL_TEXEL_8BPP,
    FGL_TEXEL_S3TC,
    FGL_TEXEL_Y1VY0U,
    FGL_TEXEL_VY1UY0,
    FGL_TEXEL_Y1UY0V,
    FGL_TEXEL_UY1VY0
} FGL_TexelFormat;

typedef enum FGL_PaletteFormatTag
{
    FGL_PALETTE_ARGB1555,
    FGL_PALETTE_RGB565,
    FGL_PALETTE_ARGB4444,
    FGL_PALETTE_ARGB8888
} FGL_PaletteFormat;

typedef enum FGL_TexWrapModeTag
{
    FGL_TEX_WRAP_REPEAT,
    FGL_TEX_WRAP_FLIP,
    FGL_TEX_WRAP_CLAMP

} FGL_TexWrapMode;

typedef enum FGL_CKeySelTag
{
    FGL_CKEY_DISABLE,
    FGL_CKEY_SEL1,
    FGL_CKEY_DISABLE2,
    FGL_CKEY_SEL2 = 3
} FGL_CKeySel;

typedef enum FGL_MipMapFilterTag
{
    FGL_FILTER_DISABLE,
    FGL_FILTER_NEAREST,
    FGL_FILTER_LINEAR
} FGL_MipMapFilter;

typedef enum FGL_TexSizeTag {
    FGL_TEX_SIZE_1P,
    FGL_TEX_SIZE_2P,
    FGL_TEX_SIZE_4P,
    FGL_TEX_SIZE_8P,
    FGL_TEX_SIZE_16P,
    FGL_TEX_SIZE_32P,
    FGL_TEX_SIZE_64P,
    FGL_TEX_SIZE_128P,
    FGL_TEX_SIZE_256P,
    FGL_TEX_SIZE_512P,
    FGL_TEX_SIZE_1024P,
    FGL_TEX_SIZE_2048P
} FGL_TexSize;

typedef enum FGL_MipmapLevelTag {
    FGL_MIPMAP_MIN_LEVEL,
    FGL_MIPMAP_MAX_LEVEL
} FGL_MipmapLevel;

// Per-fragment Unit
typedef enum FGL_PerFragUnitTag
{
    FGL_PF_SCISSOR        = SET_BIT(0),
    FGL_PF_ALPHA        = SET_BIT(1),
    FGL_PF_STENCIL        = SET_BIT(2),
    FGL_PF_DEPTH        = SET_BIT(3),
    FGL_PF_BLENDING        = SET_BIT(4),
    FGL_PF_LOGICALOP    = SET_BIT(5)
} FGL_PerFragUnit;

typedef enum FGL_CompareFuncTag
{
    FGL_COMP_NEVER,
    FGL_COMP_ALWAYS,
    FGL_COMP_LESS,
    FGL_COMP_LEQUAL,
    FGL_COMP_EQUAL,
    FGL_COMP_GREATER,
    FGL_COMP_GEQUAL,
    FGL_COMP_NOTEQUAL
} FGL_CompareFunc;

typedef enum FGL_StencilActTag
{
    FGL_ACT_KEEP,
    FGL_ACT_ZERO,
    FGL_ACT_REPLACE,
    FGL_ACT_INCR,
    FGL_ACT_DECR,
    FGL_ACT_INVERT,
    FGL_ACT_INCRWRAP,
    FGL_ACT_DECRWRAP

} FGL_StencilAct;

typedef enum FGL_BlendFuncTag
{                                       /*     source      destination  */
    FGL_BLEND_ZERO,                     /*     ok          ok(default) */
    FGL_BLEND_ONE,                      /*     ok(default) ok */
    FGL_BLEND_SRC_COLOR,                /*     ok          ok */
    FGL_BLEND_ONE_MINUS_SRC_COLOR,      /*     ok          ok */
    FGL_BLEND_DST_COLOR,                /*     ok          ok */
    FGL_BLEND_ONE_MINUS_DST_COLOR,      /*     ok          ok */
    FGL_BLEND_SRC_ALPHA,                /*     ok          ok */
    FGL_BLEND_ONE_MINUS_SRC_ALPHA,      /*     ok          ok */
    FGL_BLEND_DST_ALPHA,                /*     ok          ok */
    FGL_BLEND_ONE_MINUS_DST_ALPHA,      /*     ok          ok */
    FGL_BLEND_CONSTANT_COLOR,           /*     ok          ok */
    FGL_BLEND_ONE_MINUS_CONSTANT_COLOR, /*     ok          ok */
    FGL_BLEND_CONSTANT_ALPHA,           /*     ok          ok */
    FGL_BLEND_ONE_MINUS_CONSTANT_ALPHA, /*     ok          ok */
    FGL_BLEND_SRC_ALPHA_SATURATE        /*     ok          n/a */
} FGL_BlendFunc;

typedef enum FGL_BlendEquaTag
{
    FGL_EQ_ADD,
    FGL_EQ_SUBTRACT,
    FGL_EQ_REV_SUBTRACT,
    FGL_EQ_MIN,
    FGL_EQ_MAX
} FGL_BlendEqua;

typedef enum FGL_LogicalOpTag
{
    FGL_OP_CLEAR,
    FGL_OP_AND,
    FGL_OP_AND_REVERSE,
    FGL_OP_COPY,
    FGL_OP_AND_INVERTED,
    FGL_OP_NOOP,
    FGL_OP_XOR,
    FGL_OP_OR,
    FGL_OP_NOR,
    FGL_OP_EQUIV,
    FGL_OP_INVERT,
    FGL_OP_OR_REVERSE,
    FGL_OP_COPY_INVERTED,
    FGL_OP_OR_INVERTED,
    FGL_OP_NAND,
    FGL_OP_SET
} FGL_LogicalOp;

typedef enum FGL_PixelFormatTag
{
    FGL_PIXEL_RGB555,
    FGL_PIXEL_RGB565,
    FGL_PIXEL_ARGB4444,
    FGL_PIXEL_ARGB1555,
    FGL_PIXEL_ARGB0888,
    FGL_PIXEL_ARGB8888
} FGL_PixelFormat;

// Memory Access Arbiter
typedef enum FGL_DMAPriorityTag
{
    FGL_DMA_FIXED,
    FGL_DMA_ROTATING,
    FGL_DMA_PARTIALLY_FIXED
} FGL_DMAPriority;

typedef enum FGL_DMAAccessOrderTag
{
    FGL_ORDER_0123,
    FGL_ORDER_1230,
    FGL_ORDER_2301,
    FGL_ORDER_3012
} FGL_DMAAccessOrder;
 
/****************************************************************************
 *  Data structures
 ****************************************************************************/

// global registers (Chapter 0)

#if 0
typedef struct FGL_PipelineStatusTag
{
    FG_BOOL isEmptyPF1;            // per-fragment #2
    FG_BOOL isEmptyPF0;            // per-fragment #1
    FG_BOOL isEmptyPS1;            // pixel shader #2
    FG_BOOL isEmptyPS0;            // pixel shader #1
    FG_BOOL isEmptyRA;            // raster engine
    FG_BOOL isEmptyTSE;            // triangle setup engine
    FG_BOOL isEmptyPE;            // primitive engine
    FG_BOOL isEmptyVS;            // vertex shader
    FG_BOOL isEmptyVC;            // vertex cache
    FG_BOOL isEmptyHI2VS;        // FIFO between host interface and vertex shader
    FG_BOOL isEmptyHI;             // host interface
    FG_BOOL isEmptyHOSTFIFO;     // host-fifo
} FGL_PipelineStatus;

typedef struct FGL_PipelineTargetStateTag
{
    FG_BOOL isNotEmptyPF1;        // per-fragment #2
    FG_BOOL isNotEmptyPF0;        // per-fragment #1
    FG_BOOL isNotEmptyPS1;        // pixel shader #2
    FG_BOOL isNotEmptyPS0;        // pixel shader #1
    FG_BOOL isNotEmptyRA;        // raster engine
    FG_BOOL isNotEmptyTSE;        // triangle setup engine
    FG_BOOL isNotEmptyPE;        // primitive engine
    FG_BOOL isNotEmptyVS;        // vertex shader
    FG_BOOL isNotEmptyVC;        // vertex cache
    FG_BOOL isNotEmptyHI2VS;    // FIFO between host interface and vertex shader
    FG_BOOL isNotEmptyHI;         // host interface
    FG_BOOL isNotEmptyHOSTFIFO;    // host-fifo
} FGL_PipelineTargetState;
#else
typedef struct FGL_PipelineStatusTag
{
    FG_BOOL isNotEmptyPF;            // per-fragment
    FG_BOOL isNotEmptyPS;        // pixel shader
    FG_BOOL isNotEmptyRA;        // raster engine
    FG_BOOL isNotEmptyTSE;        // triangle setup engine
    FG_BOOL isNotEmptyPE;        // primitive engine
    FG_BOOL isNotEmptyVS;        // vertex shader
    FG_BOOL isNotEmptyVC;        // vertex cache
    FG_BOOL isNotEmptyHI2VS;    // FIFO between host interface and vertex shader
    FG_BOOL isNotEmptyHI;         // host interface
    FG_BOOL isNotEmptyHOSTFIFO;    // host-fifo
} FGL_PipelineStatus;

//typedef struct FGL_PipelineTargetStateTag
//{
//    FG_BOOL isNotEmptyPF;        // per-fragment
//    FG_BOOL isNotEmptyPS;        // pixel shader
//    FG_BOOL isNotEmptyRA;        // raster engine
//    FG_BOOL isNotEmptyTSE;        // triangle setup engine
//    FG_BOOL isNotEmptyPE;        // primitive engine
//    FG_BOOL isNotEmptyVS;        // vertex shader
//    FG_BOOL isNotEmptyVC;        // vertex cache
//    FG_BOOL isNotEmptyHI2VS;    // FIFO between host interface and vertex shader
//    FG_BOOL isNotEmptyHI;         // host interface
//    FG_BOOL isNotEmptyHOSTFIFO;    // host-fifo
//} FGL_PipelineTargetState;
#endif

// Host Interface Data Structure

typedef struct FGL_HInterfaceTag
{
    FGL_BOOL             enableVtxBuffer;
    FGL_IndexDataType     idxType;
    FGL_BOOL             enableAutoInc; // auto increment
    FGL_BOOL             enableVtxCache;
    unsigned char         numVSOut;
} FGL_HInterface;

typedef struct FGL_AttributeTag
{
    unsigned char         numComp;
    FGL_AttribCompOrder srcX;
    FGL_AttribCompOrder srcY;
    FGL_AttribCompOrder srcZ;
    FGL_AttribCompOrder srcW;
    FGL_AttribDataType     type;
    FGL_BOOL             bEndFlag;
#if TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0
    FGL_AddrMode        addrMode;
#endif
    
} FGL_Attribute;

typedef struct FGL_VtxBufAttribTag
{
    unsigned char     stride;
    unsigned int     num;
    unsigned int     addr;

} FGL_VtxBufAttrib;

#if TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0
typedef struct FGL_HIInstOptionTag
{
    unsigned int pipeMask;
    FG_BOOL wait;
    FG_BOOL interrupt;    
} FGL_InstOption;
#endif

// Primitive Engine Data Structure

typedef struct FGL_VertexTag
{
    FGL_Primitive     prim;
    FGL_BOOL         enablePointSize;        
    unsigned char     numVSOut;        // excluding position, the number of output of VS
    FGL_Shading     shadeModel;
    unsigned char    colorAttribIdx;    // this value is meaningful only when shadeModel is FGL_SHADING_FLAT
} FGL_Vertex;

typedef struct FGL_VertexCtxTag
{
    FGL_Primitive     prim;
    FGL_BOOL         enablePointSize;        

    // excluding position, the number of output of VS
    unsigned char     numVSOut;
    //ith lsb is set if ith varying needs flat interpolation
    unsigned int    varyingInterpolation;
} FGL_VertexCtx;


// Vertex/Pixel shader data structure
typedef enum FGL_ExecuteModeTag
{
    FGL_HOST_ACCESS_MODE = 0,
    FGL_PS_EXECUTE_MODE
} FGL_ExecuteMode;

// Shader Program Header Structure
typedef struct FGL_ShaderHeaderTag
{
    unsigned int    Magic;
    unsigned int    Version;
    unsigned int    HeaderSize;
    unsigned int    InTableSize;
    unsigned int    OutTableSize;
    unsigned int    SamTableSize;
    unsigned int    InstructSize;
    unsigned int    ConstFloatSize;
    unsigned int    ConstIntSize;
    unsigned int    ConstBoolSize;
    unsigned int    reserved[6]; 
} FGL_ShaderHeader, *pFGL_ShaderHeader;


typedef struct FGL_ShaderAttribTable_TAG
{
    FGL_BOOL        validTableInfo;
    unsigned int    outAttribTableSize;
    unsigned int    inAttribTableSize;
    unsigned int    vsOutAttribTable[12];
    unsigned int    psInAttribTable[12];
} FGL_ShaderAttribTable, *pFGL_ShaderAttribTable;


// Texture Units Data Structure


typedef struct FGL_TexUnitParamsTag
{
    FGL_TexType         eType;
    FGL_CKeySel         eColorkey;
    FG_BOOL             bUseExpansion; // duplication or 0 expansion
    FGL_PaletteFormat   ePaletteFormat;
    FGL_TexelFormat     eFormat;
    FGL_TexWrapMode        eUMode;
    FGL_TexWrapMode        eVMode;
//    FGL_TexWrapMode        ePMode;
    FG_BOOL                bIsNonparametric;
    FG_BOOL                bUseMagFilter;
    FG_BOOL                bUseMinFilter;
    FGL_MipMapFilter     eMipMapFilter;
    unsigned int         uUSize;
    unsigned int         uVSize;
    unsigned int         uPSize;
} FGL_TexUnitParams;


typedef struct FGL_TexStatusParamsTag
{
    FGL_TexType        eType;
    FGL_CKeySel        eColorKey;
    FG_BOOL            bUseExpansion; // duplication or 0 expansion
    FGL_PaletteFormat   ePaletteFormat;
    FGL_TexelFormat     eFormat;
    FGL_TexWrapMode        eUMode;
    FGL_TexWrapMode        eVMode;
    FG_BOOL                bIsNonparametric;
    FG_BOOL                bUseMagFilter;
    FG_BOOL                bUseMinFilter;
    FGL_MipMapFilter     eMipMapFilter;
} FGL_TexStatusParams;


typedef struct FGL_TexCKeyTag    // color key
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} FGL_TexCKey;


typedef struct FGL_VtxTexStatusRegTag // vertex texture
{    
    FGL_TexWrapMode uMode;
    FGL_TexWrapMode vMode;
    FGL_TexSize     uSize;
    FGL_TexSize     vSize;
} FGL_VtxTexStatusReg;

// Per-fragemtn Unit Data Structure

typedef struct FGL_StencilParamTag 
{
    FGL_Face             face;
    FGL_StencilAct         zpass;
    FGL_StencilAct         zfail;
    FGL_StencilAct         sfail;
    unsigned int         mask; 
    unsigned int         ref;
    FGL_CompareFunc     mode;
} FGL_StencilParam, *pFGL_StencilParam;

typedef struct FGL_StencilTag 
{
    FGL_StencilAct         zpass;
    FGL_StencilAct         zfail;
    FGL_StencilAct         sfail;
    unsigned int         mask; 
    unsigned int         ref;
    FGL_CompareFunc     mode;
} FGL_Stencil, *pFGL_Stencil;

typedef struct FGL_BlendParamTag 
{
    unsigned int     constColor;
    FGL_BlendEqua     colorEqua;
    FGL_BlendEqua     alphaEqua;
    FGL_BlendFunc     dstAlpha; 
    FGL_BlendFunc     dstColor;
    FGL_BlendFunc     srcAlpha; 
    FGL_BlendFunc     srcColor;
} FGL_BlendParam, *pFGL_BlendParam;

typedef struct FGL_BlendTag 
{
    FGL_BlendEqua     colorEqua;
    FGL_BlendEqua     alphaEqua;
    FGL_BlendFunc     dstAlpha; 
    FGL_BlendFunc     dstColor;
    FGL_BlendFunc     srcAlpha; 
    FGL_BlendFunc     srcColor;
} FGL_Blend, *pFGL_Blend;


typedef struct FGL_FBCtrlParamTag 
{
    FGL_BOOL        opaqueAlpha; 
    unsigned int    thresholdAlpha;
    unsigned int    constAlpha; 
    FGL_BOOL        dither;
    FGL_PixelFormat format;
} FGL_FBCtrlParam, *pFGL_FBCtrlParam;


/****************************************************************************
 *  EXPORT FUNCTIONS
 ****************************************************************************/

/***************************************************************************** 
 * 
 *  Global SFRs Register-level API
 * 
 *****************************************************************************/
FGL_Error fglGetPipelineStatus(FGL_PipelineStatus *status );

FGL_Error fglFlush(unsigned int pipelineFlags);

FGL_Error fglClearCache(unsigned int clearFlags);

//FGL_Error fglReset(FGL_ResetFlags resetFlags);

//FGL_Error fglSetDMAParams( 
//                    FGL_DMAPriority     policy, 
//                    FGL_DMAAccessOrder     order
//                );

FGL_Error fglSoftReset(void);

FGL_Error fglGetVersion(unsigned int* ver);

FGL_Error fglGetInterrupt(FG_BOOL* isGenInterrupt);

FGL_Error fglPendInterrupt(void);

FGL_Error fglEnableInterrupt(void);

FGL_Error fglDisableInterrupt(void);

FGL_Error fglSetInterruptBlock(unsigned int pipeMask);

FGL_Error fglSetInterruptState(FGL_PipelineStatus status);

FGL_Error fglGetInterruptState(FGL_PipelineStatus* status);

#if TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0
FGL_Error fglGetExeState(FGL_ExeState status);

FGL_Error fglSetNextStep(void);
#endif

/***************************************************************************** 
 * 
 *  Host Interface Register-level API
 * 
 *****************************************************************************/
#if TARGET_FIMG_VERSION < _FIMG3DSE_VER_2_0
FGL_Error fglGetNumEmptyFIFOSlots ( unsigned int *pNumSlots );

FGL_Error fglSendToFIFO ( unsigned int bytes, void *buffer );
FGL_Error _fglSendToFIFO32 ( unsigned int bytes, void *buffer, unsigned int dest );
#define fglSendToFIFO32( x, y )    _fglSendToFIFO32(x, y, FGHI_FIFO_ENTRY)

FGL_Error fglDrawNonIndexArrays(
                int numAttribs,
                FGL_Attribute *pAttrib,
                int numVertices,
                void **ppData
            );
#endif

FGL_Error fglSetHInterface( FGL_HInterface *pHI );

FGL_Error fglSetIndexOffset( unsigned int offset );

#if TARGET_FIMG_VERSION < _FIMG3DSE_VER_2_0
FGL_Error fglSetVtxBufferAddr( unsigned int addr );

FGL_Error fglSendToVtxBuffer ( unsigned int data );
#endif

FGL_Error fglSetAttribute ( 
                    unsigned char     attribIdx, 
                    FGL_Attribute     *pAttribInfo
                );

FGL_Error fglSetVtxBufAttrib ( 
                        unsigned char         attribIdx, 
                         FGL_VtxBufAttrib     *pAttribInfo
                   );

#if TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0                   
FGL_Error fglSetHIInstructions(
                    unsigned int    baseAddr,
                    FGL_AddrMode    addrMode,
                    unsigned int    instCount                        
                        );
                        
FGL_Error fglGetNextInstAddr(unsigned int *pNextAddr);
                        
FGL_Error fglGetRemainInstCount(unsigned int *pInstCount);

FGL_Error fglInstSFRAD(
                    FGL_InstOption instOption, unsigned int numSFRPairs, 
                    unsigned int addrSrc, FGL_AddrMode addrMode    
                        );
                        
FGL_Error fglInstSFRIM(
                    FGL_InstOption instOption, unsigned int addrDst, unsigned int SFRVal
                        );
                        
FGL_Error fglInstSFRAI(
                    FGL_InstOption instOption, unsigned int addrDst, unsigned int numSFR, 
                    unsigned int addrSrc, FGL_AddrMode addrMode
                        );
            
FGL_Error fglInstVBWR(
                    FGL_InstOption instOption, unsigned int addrDst, 
                    unsigned int numWord, unsigned int addrSrc, FGL_AddrMode addrMode
                        );
                        
FGL_Error fglInstNOP(FGL_InstOption instOption);

FGL_Error fglInstINDEX(
                    FGL_InstOption instOption, FGL_IndexDataType indexType,
                    unsigned int numIndices, unsigned int addrIndex, FGL_AddrMode addrMode
                        );
                        
FGL_Error fglInstINDEXAI(
                    FGL_InstOption instOption, unsigned int numIndices, unsigned int startIndex
                        );
                        
FGL_Error fglEXEControl(FG_BOOL interrupt, FG_BOOL wait);

FGL_Error fglSetVBSize(FGL_VBSizeMode VBSizeMode);

FGL_Error fglClearPreVC(void);

FGL_Error fglClearPostVC(void);

#endif

/***************************************************************************** 
 * 
 *  Vertex Shader Register-level API
 * 
 *****************************************************************************/
FGL_Error fglVSSetIgnorePCEnd ( FGL_BOOL enable );

FGL_Error fglVSSetPCRange ( 
                    unsigned int start, 
                    unsigned int end
                );

FGL_Error fglVSSetPCRangeEXT ( 
                    unsigned int start, 
                    unsigned int end,
                    FGL_BOOL ignorePCEnd
                );

FGL_Error fglVSSetAttribNum ( 
                        //unsigned int outAttribNum, 
                        unsigned int inAttribNum
                  );

FGL_Error fglMakeShaderAttribTable (
                            const unsigned int *pVertexShader, 
                            const unsigned int *pPixelShader,
                            pFGL_ShaderAttribTable attribTable 
                         );

FGL_Error fglRemapVShaderOutAttrib (
                                        pFGL_ShaderAttribTable pShaderAttribTable
                                   );

FGL_Error fglSetVShaderAttribTable (
                                        FGL_AttribTableIdx idx,
                                        unsigned int value
                                   );
                                   
                                   
/***************************************************************************** 
 * 
 *  Primitive Engine Register-level API
 * 
 *****************************************************************************/
FGL_Error fglSetVertex ( FGL_Vertex *pVtx );

FGL_Error fglSetVertexCtx ( FGL_VertexCtx *pVtx );
FGL_Error fglSetViewportParams (
                        FGL_BOOL     bYFlip, 
                        float         x0, 
                        float         y0, 
                        float         px, 
                        float         py, 
                        float         H
                  );

FGL_Error fglSetDepthRange ( float n, float f );


/***************************************************************************** 
 * 
 *  Raster Engine Register-level API
 * 
 *****************************************************************************/
FGL_Error fglSetPixelSamplePos ( FGL_Sample samp );

FGL_Error fglEnableDepthOffset ( FG_BOOL enable );

FGL_Error fglSetDepthOffsetParamf ( 
                            FGL_DepthOffsetParam     param, 
                            float                   value
                       );

FGL_Error fglSetDepthOffsetParam ( 
                            FGL_DepthOffsetParam     param, 
                            unsigned int             value
                       );

FGL_Error fglSetFaceCullControl ( 
                            FG_BOOL         enable, 
                            FG_BOOL             bCW, 
                            FGL_Face         face
                      );

FGL_Error fglSetYClip ( unsigned int ymin, unsigned int ymax );

FGL_Error fglSetLODControl ( unsigned int ctl );

FGL_Error fglSetLODRegister (
                    FGL_LodCoeff lodCon0,
                    FGL_LodCoeff lodCon1,
                    FGL_LodCoeff lodCon2,
                    FGL_LodCoeff lodCon3,
                    FGL_LodCoeff lodCon4,
                    FGL_LodCoeff lodCon5,
                    FGL_LodCoeff lodCon6,
                    FGL_LodCoeff lodCon7
                );

FGL_Error fglSetXClip( unsigned int xmin, unsigned int xmax );

FGL_Error fglSetPointWidth(float pWidth);

FGL_Error fglSetMinimumPointWidth(float pWidthMin);

FGL_Error fglSetMaximumPointWidth(float pWidthMax);

FGL_Error fglSetCoordReplace(unsigned int coordReplaceNum);

FGL_Error fglSetLineWidth(float lWidth);

/***************************************************************************** 
 * 
 *  Pixel Shader Register-level API
 * 
 *****************************************************************************/
FGL_Error fglLoadVShader ( const unsigned int *pShaderCode );

FGL_Error fglLoadPShader ( const unsigned int *pShaderCode );

FGL_Error fglSetPSMode ( FGL_ExecuteMode mode );

FGL_Error fglPSSetPCRange ( unsigned int start, unsigned int end );

FGL_Error fglPSSetPCRangeEXT ( unsigned int start,
                                unsigned int end,
                                FGL_BOOL ignorePCEnd );

FGL_Error fglPSSetIgnorePCEnd ( FGL_BOOL enable );

FGL_Error fglPSSetAttributeNum ( unsigned int attributeNum );

FGL_Error fglPSSetAttribNum ( unsigned int attributeNum );

FGL_Error fglPSGetInBufferStatus ( unsigned int *ready );

FGL_Error fglSetPSParams( unsigned int attrubNum,
                            unsigned int startPC,
                            unsigned int endPC,
                            FGL_BOOL ignorePCEnd );

/***************************************************************************** 
 * 
 *  Texture Units Register-level API
 * 
 *****************************************************************************/
FGL_Error fglSetTexUnitParams ( unsigned int unit, FGL_TexUnitParams *params );

FGL_Error fglSetTexStatusParams(unsigned int unit, FGL_TexStatusParams *params);

FGL_Error fglSetTexUSize(unsigned int unit, unsigned int uSize);

FGL_Error fglSetTexVSize(unsigned int unit, unsigned int vSize);

FGL_Error fglSetTexPSize(unsigned int unit, unsigned int pSize);

FGL_Error fglCalculateMipmapOffset(
                    unsigned int unit,
                    unsigned int uSize,
                    unsigned int vSize,
                    unsigned int *maxLev
        );

FGL_Error fglCalculateMipmapOffsetS3TC(
                    unsigned int unit,
                    unsigned int uSize,
                    unsigned int vSize,
                    unsigned int *maxLev
        );

FGL_Error fglCalculateMipmapOffsetYUV(
                    unsigned int unit,
                    unsigned int uSize,
                    unsigned int vSize,
                    unsigned int *maxLev
        );

FGL_Error fglSetTexMipmapLevel (
                        unsigned int     unit, 
                        FGL_MipmapLevel level, 
                        unsigned int     value 
                     );

FGL_Error fglSetTexBaseAddr( unsigned int unit, unsigned int addr );

FGL_Error fglSetTexColorKey( unsigned int unit, FGL_TexCKey ckey ); // CK 0 or 1

FGL_Error fglSetTexColorKeyYUV( unsigned char g, unsigned char b );

FGL_Error fglSetTexColorKeyMask( unsigned char bitsToMask );

FGL_Error fglSetTexPaletteAddr( unsigned int addr );

FGL_Error fglSetTexPaletteEntry( unsigned int entry );

FGL_Error fglSetVtxTexUnitParams( unsigned int unit, FGL_VtxTexStatusReg *vts );

FGL_Error fglSetVtxTexBaseAddr ( unsigned int unit, unsigned int addr );


/***************************************************************************** 
 * 
 *  Per-fragment Unit Register-level API 
 * 
 *****************************************************************************/
FGL_Error fglEnablePerFragUnit ( FGL_PerFragUnit unit, FGL_BOOL enable );

FGL_Error fglSetScissorParams ( 
                        unsigned int xMax, 
                        unsigned int xMin,
                           unsigned int yMax, 
                           unsigned int yMin
                       );

FGL_Error fglSetScissor ( 
                        FGL_BOOL enable,
                        unsigned int xMax, 
                        unsigned int xMin,
                           unsigned int yMax, 
                           unsigned int yMin
                      );

FGL_Error fglSetAlphaParams ( unsigned int refAlpha, FGL_CompareFunc mode );

FGL_Error fglSetAlpha (
                FGL_BOOL enable,
                unsigned int refAlpha,
                FGL_CompareFunc mode
                );

FGL_Error fglSetStencilParams (pFGL_StencilParam stencilParam );

FGL_Error fglSetFrontStencil( FGL_BOOL enable, pFGL_Stencil stencilParam );

FGL_Error fglSetBackStencil( pFGL_Stencil stencilParam );

FGL_Error fglSetDepthParams ( FGL_CompareFunc mode );

FGL_Error fglSetDepth( FGL_BOOL enable,  FGL_CompareFunc mode );

FGL_Error fglSetBlendParams ( pFGL_BlendParam blendParam );

// Software workaround for bug ID 82 (v1.21), 32 (v1.51)
FGL_Error fglSetBlendParamsWorkAround ( pFGL_BlendParam blendParam );

FGL_Error fglSetBlend( FGL_BOOL enable, pFGL_Blend blendParam );

// Software workaround for bug ID 82 (v1.21), 32 (v1.51)
FGL_Error fglSetBlendWorkAround( FGL_BOOL enable, pFGL_Blend blendParam );

FGL_Error fglSetBlendColor ( unsigned int blendColor );

/* Just only use OpenGL|ES 1.1 */
FGL_Error fglSetLogicalOpParams ( FGL_LogicalOp colorOp );

FGL_Error fglSetLogicalOp ( FGL_BOOL enable,
                            FGL_LogicalOp alphaOp,
                            FGL_LogicalOp colorOp );

FGL_Error fglSetColorBufWriteMask(
                    FGL_BOOL r,
                    FGL_BOOL g,
                    FGL_BOOL b,
                    FGL_BOOL a
                );

#if TARGET_FIMG_VERSION != _FIMG3DSE_VER_1_2
    FGL_Error fglSetStencilBufWriteMask( FGL_Face face, unsigned int mask );
#endif

FGL_Error fglSetZBufWriteMask ( FGL_BOOL enable );

FGL_Error fglSetDepthStencilWriteMask ( 
                unsigned int     backStencilMask, 
                unsigned int     frontStencilMask,
                FGL_BOOL        bDepthBufferMask 
            );

FGL_Error fglSetFrameBufParams ( pFGL_FBCtrlParam fbctrlParam );

FGL_Error fglSetZBufBaseAddr ( unsigned int addr );

FGL_Error fglSetColorBufBaseAddr ( unsigned int addr );

FGL_Error fglSetFrameBufWidth ( unsigned int width );

#if TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0
/***************************************************************************** 
 * 
 *  Pre-Depth Test Unit Register-level API 
 * 
 *****************************************************************************/
FGL_Error fglSetPreStencilTest ( FGL_BOOL enable );
FGL_Error fglSetPreDepthTest ( FGL_BOOL enable );
FGL_Error fglSetPDTStencilParams (pFGL_StencilParam stencilParam );
FGL_Error fglSetPDTDepthParams ( FGL_CompareFunc mode );
FGL_Error fglSetPDTStencilBufWriteMask( FGL_Face face, unsigned int mask );
FGL_Error fglSetPDTZBufWriteMask ( FGL_BOOL enable );
FGL_Error fglSetPDTZBufBaseAddr ( unsigned int addr );
FGL_Error fglSetPDTFrameBufWidth ( unsigned int width );
#endif /* TARGET_FIMG_VERSION == _FIMG3DSE_VER_2_0 */

#if defined __cplusplus
}
#endif

#endif    /* __FIMG_H__ */

