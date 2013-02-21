/*******************************************************************************
                        Samsung India Software Operations Pvt. Ltd. (SISO)
                                    Copyright 2006
;*******************************************************************************/

typedef        char            CHAR;
typedef        unsigned char    BYTE;
typedef        unsigned short    WORD;
typedef        unsigned int    DWORD_t;
typedef        unsigned int    LONG_t;

#ifndef _INC_AVIFMT
#define _INC_AVIFMT    100    /* version number * 100 + revision */

#ifdef __cplusplus
EXPORT "C" {            /* Assume C declarations for C++ */
#endif    /* __cplusplus */

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                \
        ( (DWORD_t)(BYTE)(ch0) | ( (DWORD_t)(BYTE)(ch1) << 8 ) |    \
        ( (DWORD_t)(BYTE)(ch2) << 16 ) | ( (DWORD_t)(BYTE)(ch3) << 24 ) )
#endif

/* Macro to make a TWOCC out of two characters */
#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

/* form types, list types, and chunk types */
#define formheaderRIFF            mmioFOURCC('R', 'I', 'F', 'F')
#define formtypeAVI             mmioFOURCC('A', 'V', 'I', ' ')
#define listheaderLIST          mmioFOURCC('L', 'I', 'S', 'T')
#define listtypeAVIHEADER       mmioFOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          mmioFOURCC('a', 'v', 'i', 'h')
#define listtypeSTREAMHEADER    mmioFOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        mmioFOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        mmioFOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   mmioFOURCC('s', 't', 'r', 'd')
#define ckidSTREAMNAME            mmioFOURCC('s', 't', 'r', 'n')
#define listtypeAVIMOVIE        mmioFOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       mmioFOURCC('r', 'e', 'c', ' ')
#define ckidAVINEWINDEX         mmioFOURCC('i', 'd', 'x', '1')

/*
** Stream types for the <fccType> field of the stream header.
*/
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI            mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')

/* Basic chunk types */
#define cktypeDIBbits           aviTWOCC('d', 'b')
#define cktypeDIBcompressed     aviTWOCC('d', 'c')
#define cktypeDIBdrm            aviTWOCC('d', 'd')
#define cktypePALchange         aviTWOCC('p', 'c')
#define cktypeWAVEbytes         aviTWOCC('w', 'b')
#define cktypeSUBtext           aviTWOCC('s', 't')
#define cktypeSUBbmp            aviTWOCC('s', 'b')
#define cktypeCHAP              aviTWOCC('c', 'h')

/* Chunk id to use for extra chunks for padding. */
#define ckidAVIPADDING          mmioFOURCC('J', 'U', 'N', 'K')

/*
** Useful macros
*/

/* Macro to get stream number out of a FOURCC ckid */
#define FromHex(n)    (((n) >= 'A') ? ((n) + 10 - 'A') : ((n) - '0'))
#define StreamFromFOURCC(fcc) ((FromHex((fcc) & 0xff)) << 4) + (FromHex((fcc >> 8) & 0xff))

/* Macro to get TWOCC chunk type out of a FOURCC ckid */
#define TWOCCFromFOURCC(fcc)    (fcc >> 16)

/* Macro to make a ckid for a chunk out of a TWOCC and a stream number
** from 0-255.
*/
#define ToHex(n)    ((BYTE) (((n) > 9) ? ((n) - 10 + 'A') : ((n) + '0')))
#define MAKEAVICKID(tcc, stream) \
        ((DWORD_t) ((ToHex((stream) & 0xf0)) | (ToHex((stream) & 0x0f) << 8) | (tcc << 16)))

/*
** Main AVI File Header
*/    
        
/* flags for use in <dwFlags> in AVIFileHdr */
#define AVIF_HASINDEX        0x00000010    // Index at end of file
#define AVIF_MUSTUSEINDEX    0x00000020
#define AVIF_ISINTERLEAVED    0x00000100
#define AVIF_TRUSTCKTYPE    0x00000800    // Use CKType to find key frames
#define AVIF_WASCAPTUREFILE    0x00010000
#define AVIF_COPYRIGHTED    0x00020000

/* The AVI File Header LIST chunk should be padded to this size */
#define AVI_HEADERSIZE  2048                    // size of AVI header list

typedef struct
{
    DWORD_t        dwMicroSecPerFrame;    // frame display rate (or 0L)
    DWORD_t        dwMaxBytesPerSec;    // max. transfer rate
    DWORD_t        dwPaddingGranularity;    // pad to multiples of this
                                                // size; normally 2K.
    DWORD_t        dwFlags;        // the ever-present flags
    DWORD_t        dwTotalFrames;        // # frames in file
    DWORD_t        dwInitialFrames;
    DWORD_t        dwStreams;
    DWORD_t        dwSuggestedBufferSize;

    DWORD_t        dwWidth;
    DWORD_t        dwHeight;

    DWORD_t        dwReserved[4];
} MainAVIHeader;

/*
** Stream header
*/

#define AVISF_DISABLED            0x00000001
#define AVISF_VIDEO_PALCHANGES        0x00010000

typedef    DWORD_t    FOURCC_t;

typedef struct tagRECT_t {
    short int left;
    short int top;
    short int right;
    short int bottom;
} RECT_t;

typedef struct {
    FOURCC_t        fccType;
    FOURCC_t        fccHandler;
    DWORD_t        dwFlags;    /* Contains AVITF_* flags */
    WORD        wPriority;
    WORD        wLanguage;
    DWORD_t        dwInitialFrames;
    DWORD_t        dwScale;    
    DWORD_t        dwRate;    /* dwRate / dwScale == samples/second */
    DWORD_t        dwStaDivXrt;
    DWORD_t        dwLength; /* In units above... */
    DWORD_t        dwSuggestedBufferSize;
    DWORD_t        dwQuality;
    DWORD_t        dwSampleSize;
    RECT_t        rcFrame;
} AVIStreamHeader;

/* Flags for index */
#define AVIIF_LIST          0x00000001L // chunk is a 'LIST'
#define AVIIF_KEYFRAME      0x00000010L // this frame is a key frame.
#define AVIIF_NOTIME        0x00000100L // this frame doesn't take any time
#define AVIIF_COMPUSE       0x0FFF0000L // these bits are for compressor use

typedef struct
{
    DWORD_t        ckid;
    DWORD_t        dwFlags;
    DWORD_t        dwChunkOffset;        // Position of chunk
    DWORD_t        dwChunkLength;        // Length of chunk
} AVIINDEXENTRY;

/*
** Palette change chunk
**
** Used in video streams.
*/

typedef struct tagPALETTEENTRY_t { // pe 
    BYTE peRed; 
    BYTE peGreen; 
    BYTE peBlue; 
    BYTE peFlags; 
} PALETTEENTRY_t; 

typedef struct
{
    BYTE        bFirstEntry;    /* first entry to change */
    BYTE        bNumEntries;    /* # entries to change (0 if 256) */
    WORD        wFlags;        /* Mostly to preserve alignment... */
    PALETTEENTRY_t    peNew[];    /* New color specifications */
} AVIPALCHANGE;

typedef struct tagBITMAPINFOHEADER_t{ // bmih 
    DWORD_t  biSize; 
    LONG_t   biWidth; 
    LONG_t   biHeight; 
    WORD   biPlanes; 
    WORD   biBitCount;
    DWORD_t  biCompression; 
    DWORD_t  biSizeImage; 
    LONG_t   biXPelsPerMeter; 
    LONG_t   biYPelsPerMeter; 
    DWORD_t  biClrUsed; 
    DWORD_t  biClrImportant; 
} BITMAPINFOHEADER_t; 

typedef struct tagWAVEFORMATEX_t{ 
    WORD  wFormatTag; 
    WORD  nChannels; 
    DWORD_t nSamplesPerSec; 
    DWORD_t nAvgBytesPerSec; 
    WORD  nBlockAlign; 
    WORD  wBitsPerSample; 
    WORD  cbSize; 
} WAVEFORMATEX_t; 

typedef struct tagTEXTINFO{
    WORD wCodePage;
    WORD wCountryCode;
    WORD wLanguageCode;
    WORD wDialect;
} TEXTINFO;

// end_vfw32

#ifdef __cplusplus
}                       /* End of EXPORT "C" { */
#endif    /* __cplusplus */

#endif /* _INC_AVIFMT */
