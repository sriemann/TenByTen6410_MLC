#ifndef __SAMSUNG_SYSLSI_APDEV_MFCLIB_SSBSIPMPEG4DECODE_H__
#define __SAMSUNG_SYSLSI_APDEV_MFCLIB_SSBSIPMPEG4DECODE_H__



typedef struct
{
    int width;
    int height;
} SSBSIP_MPEG4_STREAM_INFO;



typedef unsigned int    MPEG4_DEC_CONF;

#define MPEG4_DEC_GETCONF_STREAMINFO        0x00001001
#define MPEG4_DEC_GETCONF_PHYADDR_FRAM_BUF    0x00001002
#define MPEG4_DEC_GETCONF_FRAM_NEED_COUNT    0x00001003

#define MPEG4_DEC_SETCONF_POST_ROTATE        0x00002001


#ifdef __cplusplus
extern "C" {
#endif


void *SsbSipMPEG4DecodeInit();
int   SsbSipMPEG4DecodeExe(void *openHandle, long lengthBufFill);
int   SsbSipMPEG4DecodeDeInit(void *openHandle);

int   SsbSipMPEG4DecodeSetConfig(void *openHandle, MPEG4_DEC_CONF conf_type, void *value);
int   SsbSipMPEG4DecodeGetConfig(void *openHandle, MPEG4_DEC_CONF conf_type, void *value);


void *SsbSipMPEG4DecodeGetInBuf(void *openHandle, long size);
void *SsbSipMPEG4DecodeGetOutBuf(void *openHandle, long *size);



#ifdef __cplusplus
}
#endif


// Error codes
#define SSBSIP_MPEG4_DEC_RET_OK                        (0)
#define SSBSIP_MPEG4_DEC_RET_ERR_INVALID_HANDLE        (-1)
#define SSBSIP_MPEG4_DEC_RET_ERR_INVALID_PARAM        (-2)

#define SSBSIP_MPEG4_DEC_RET_ERR_CONFIG_FAIL        (-100)
#define SSBSIP_MPEG4_DEC_RET_ERR_DECODE_FAIL        (-101)
#define SSBSIP_MPEG4_DEC_RET_ERR_GETCONF_FAIL        (-102)
#define SSBSIP_MPEG4_DEC_RET_ERR_SETCONF_FAIL        (-103)


#endif /* __SAMSUNG_SYSLSI_APDEV_MFCLIB_SSBSIPMPEG4DECODE_H__ */

