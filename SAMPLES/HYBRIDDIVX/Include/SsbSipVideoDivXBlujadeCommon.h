#ifndef __BJ_COMMON__
#define __BJ_COMMON__
typedef struct {
    /* The structure will evolve depending on the needs for particular codec*/
    int srcRate;     /* sample rate or frame rate depends on media */
    int bitRate;
    int height;
    int width;
    int timeStamp;
    int precision;      /* Generally 16 for speech and audio */
    int mediaType;   /* audio, video, speech, image */
    int noComp;      /* number of color components or number of audio channels */
    int codingType;  /* type of coding YUV,RGB or sterio, joint sterio etc */
    int codecType;   /* different codecs MPEG, JPEG, MP3 etc*/
    //void *codec_param;/* may be needed only for encoders, to give encoding parameters */ 
}Media;

typedef struct {
    char inputFile[20];
    char outputFile[20];
    unsigned char *input;
    unsigned int   ipLength;
    unsigned char *output;
    unsigned int   opLength;
    int isFile; /* Flag telss whether the operations are file based or stream based */
    //Media media_param;
}InputParam;

typedef enum 
{
    SUCCESS,
    FAILURE,
    NO_OUTPUT  /* sometimes there may not be any o/p like decoding only headers */
}ReturnCode;



typedef enum ACode
{
    INIT_CODEC,
    DEINIT_CODEC,
    ENCODE,
    DECODE
    /* Add any needed */
}ActionCode;


/* There is only one API for all purposes */
ReturnCode CodeMedia (InputParam *pInput, 
                       Media *pMediaParam, 
                       void *pCodecParam,
                       ActionCode action);
ReturnCode CodeMediaDivx (InputParam *pInput, 
                       Media *pMediaParam, 
                       void *pCodecParam,
                       ActionCode action);

#endif


