/********************************************************************************
*                                                                                *
* File Name            : SsbSsipAudioAacDecoder.h                                *
* Description          : API functions Header for AAC Decoder                    *
* Reference Document   :                                                        *
* Revision History     :                                                        *
*      Date            Author                  Detail Description                *
*  ------------    ----------------    ----------------------------------        *
*  Nov 4, 2003     Vikash Saraogi       Created                                    *
*  Dec 8, 2003     Vikash Saraogi          Added Error Codes #defines            *    
*                                                                                *
*********************************************************************************/
#ifndef SSB_SIP_AUDIO_AAC_DECODER_H
#define SSB_SIP_AUDIO_AAC_DECODER_H

/********************************************************************************
*                                                                                *
* Include Files                                                                    *
*                                                                                *
*********************************************************************************/
#include "SsbTypeDefs.h"



#ifdef __cplusplus
extern "C"
{
#endif

/* Struct/Union Types and Define */
#define FMT_16BIT 1
#define FMT_24BIT 2

#define END_CODING -1
#define FATAL_ERROR -100

#define FRAME_LENGTH        (1024)
#define SCRATCH_MEM_SIZE    (12 * 1024)
#define PCM_SAMPLE_BITS_16  (16)
#define PCM_SAMPLE_BITS_24  (24)
#define INP_BUF_IDX         (0)
#define OUT_BUF_IDX         (1)
#define MAX_CHANNELS 2

#define SIP_AUDIO_AAC_DECODER_LO_PRIORITY_MEM     0
#define SIP_AUDIO_AAC_DECODER_HI_PRIORITY_MEM     1
#define SIP_AUDIO_AAC_DECODER_PERSIST_MEM         2
#define SIP_AUDIO_AAC_DECODER_SCRATCH_MEM         3
typedef void *SAACDecoder;

SAACDecoder SAACInitDecoder(void);
void SAACFreeDecoder(SAACDecoder hAACDecoder);

/* Memory Table Descriptor */
typedef struct SsbSipAudioAacDecoderMemInfoStruct
{
        Int32 iSize;       /* size of the memory in bytes */
        Int32 iAlignment;  /* alignment in bytes */
        Int32 iPriority;   /* high or low priority */

} SsbSipAudioAacDecoderMemInfo_t;

/* AAC Decoder Configuration */
typedef struct SsbSipAudioAacDecoderConfigStruct
{
        Int16  defObjectType;  /* object type which can be AACLC, ERAACLC, etc */
        Uint32 defSampleRate;  /* sampling freq of the decoded stream */
        Uint32 outputFormat;   /* output quant. level (16 or 24 bit) */
        Uint32 defNumChannels; /* number of channels */

} SsbSipAudioAacDecoderConfig_t;

/* AAC Decoder Frame Information */
typedef struct SsbSipAudioAacDecoderFrameInfoStruct
{
        Int32 bytesconsumed;
        Int32 error;
        Int32 samples;
        Int32 sbrEnabled;

} SsbSipAudioAacDecoderFrameInfo_t;


typedef struct SsbSipAudioAacDecoderStruct
{
        /* Internal Project Property */
        /* Pointer to respective state structure */
        void *pSsbAudioAacDecoder;

        void *pvAudioAacDecoderPersist;
        void *pvAudioAacDecoderScratch;

        SsbSipAudioAacDecoderMemInfo_t   strtMemInfoPersist;
        SsbSipAudioAacDecoderMemInfo_t   strtMemInfoScratch;
        SsbSipAudioAacDecoderConfig_t    strtAacDecoderConfig;
        SsbSipAudioAacDecoderFrameInfo_t strtAacDecoderFrameInfo;

        /* Array index for input/output buffers */
        Int32 iInpIdx;
        Int32 iOutIdx;

        /* Virtual IO Pad Connecting (i.e., Virtual wiring for Virtual IP) */
        /* Input/Output Buffer Pointers */
        Int32 *piAudioAacDecoderBuffer[2];

} SsbSipAudioAacDecoder_t;

/* Commands define for *Config() ApI function */
#define SIP_AUDIO_AAC_DECODER_INPBUF_ADDR              0
#define SIP_AUDIO_AAC_DECODER_OUTBUF_ADDR              1
#define SIP_AUDIO_AAC_DECODER_PERSIST_MEM_PTR          2
#define SIP_AUDIO_AAC_DECODER_SCRATCH_MEM_PTR          3
#define SIP_AUDIO_AAC_DECODER_INIT                     4
#define SIP_AUDIO_AAC_DECODER_NUM_OF_PCM_OUTPUT_BITS   5
#define SIP_AUDIO_AAC_DECODER_PERSIST_MEM_INFO         6
#define SIP_AUDIO_AAC_DECODER_SCRATCH_MEM_INFO         7
#define SIP_AUDIO_AAC_DECODER_CONFIG                   8
#define SIP_AUDIO_AAC_DECODER_HEADER_DECODE            9


#define SIP_AUDIO_AAC_DECODER_INVALID_INDEX_ERROR       100
#define SIP_AUDIO_AAC_DECODER_INVALID_ADDR_ERROR       101
#define SIP_AUDIO_AAC_DECODER_INVALID_MEM_ALLOC_ERROR  102
#define SIP_AUDIO_AAC_DECODER_INVALID_FORMAT_ERROR     103
#define SIP_AUDIO_AAC_DECODER_INVALID_COMMAND_ERROR    104


/* Exported Functions Prototypes */

/* API for AAC Decoder Set Config */

ERRORCODE SsbSipAudioAacDecoderSetConfig(
    SsbSipAudioAacDecoder_t *pObjAacDec,
    Int32 iCmd,
    void * pvValue,
    Int32 iIdx);

/* API for AAC Decoder Get Config */

void *SsbSipAudioAacDecoderGetConfig(
    SsbSipAudioAacDecoder_t *pObjAacDec,
    Int32 iCmd,
    Int32 iIdx);

/* API for AAC Decoder Execute */
ERRORCODE SsbSipAudioAacDecoderExe(SsbSipAudioAacDecoder_t *pObjAacDec);
Int16 AACHeaderDecode_Ittiam(SAACDecoder *AACDecoder, Uint8 *buffer,SsbSipAudioAacDecoderConfig_t *config);

Int16 AACDecode_Ittiam(SAACDecoder *AACDecoder, Uint8 *buffer, void *sample_buffer, SsbSipAudioAacDecoderFrameInfo_t *hInfo);
#ifdef __cplusplus
}
#endif

#endif /* SsbSsipAudioAacDecoder.h */