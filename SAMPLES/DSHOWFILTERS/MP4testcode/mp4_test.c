#include <stdio.h>

#include "mp4.h"


char *mp4FileName = "\\Storage Card\\TestVectors\\Downloaded\\[Ani]Conan-01.mp4";

int WINAPI WinMain(    HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow)
{
    int            b;

    MP4FileHandle  mp4File;

    MP4SampleId    sampleId;

    MP4TrackId     video_trId, audio_trId;
    char          *video_name, *audio_name;

    /////////   VIDEO variables
    u_int32_t      video_num_samples;
    u_int32_t      video_width, video_height;
    double         video_frame_rate;
    u_int32_t      video_timescale;
    MP4Duration    video_duration;
    u_int8_t      *p_video_config;
    u_int32_t      n_video_config_size;
    u_int8_t     **pp_sps, **pp_pps;
    u_int32_t     *pn_sps, *pn_pps;
    u_int8_t      *p_video_sample;
    u_int32_t      n_video_sample;
    u_int32_t      video_sample_max_size;

    /////////   AUDIO variables
    u_int32_t      audio_num_samples;
    int            audio_num_channels;
    u_int32_t      audio_timescale;
    MP4Duration    audio_duration;
    u_int8_t      *p_audio_config;
    u_int32_t      n_audio_config_size;
    u_int32_t      audio_sample_max_size;

    unsigned long  tick;


    ///////////////////////////////////////////////
    /////                                     /////
    /////  1. Open the mp4 file for reading.  /////
    /////                                     /////
    ///////////////////////////////////////////////
    mp4File = MP4Read(mp4FileName, 0);
    if (!mp4File) {
        exit(1);
    }

printf("\n1. Open the mp4 file for reading.");

    ////////////////////////////////////////////
    /////                                  /////
    /////  2. Identify the video & audio.  /////
    /////                                  /////
    ////////////////////////////////////////////
    video_trId = MP4FindTrackId(mp4File, 0, MP4_VIDEO_TRACK_TYPE, 0);
    if (video_trId == MP4_INVALID_TRACK_ID) {
        printf("\nNo video track");
        video_name = NULL;
    }
    else {
        video_name = MP4GetTrackMediaDataName(mp4File, video_trId);
        if (strcmp(video_name, "mp4v") == 0)
            printf("\nVideo = MPEG4");
        else if (strcmp(video_name, "h263") == 0)
            printf("\nVideo = H.263");
        else if (strcmp(video_name, "avc1") == 0)
            printf("\nVideo = H.264");
        else
            printf("\nVideo = Unknown");
    }
printf("\n2-1. Identify the video.");
    audio_trId = MP4FindTrackId(mp4File, 0, MP4_AUDIO_TRACK_TYPE, 0);
    if (audio_trId == MP4_INVALID_TRACK_ID) {
        printf("\nNo audio track");
        audio_name = NULL;
    }
    else {
        audio_name = MP4GetTrackMediaDataName(mp4File, audio_trId);
        if (strcmp(audio_name, "mp4a") == 0)
            printf("\nAudio = MPEG4 AAC");
        else
            printf("\nAudio = Unknown");
    }
printf("\n2-2. Identify the audio.");

    //////////////////////////////////
    /////                        /////
    /////  3. Video Properties.  /////
    /////                        /////
    //////////////////////////////////
    if (video_trId != MP4_INVALID_TRACK_ID) {
        video_num_samples     = MP4GetTrackNumberOfSamples(mp4File, video_trId);
        video_width           = MP4GetTrackVideoWidth(mp4File, video_trId);
        video_height          = MP4GetTrackVideoHeight(mp4File, video_trId);
        video_frame_rate      = MP4GetTrackVideoFrameRate(mp4File, video_trId);

        video_timescale       = MP4GetTrackTimeScale(mp4File, video_trId);
        video_duration        = MP4GetTrackDuration(mp4File, video_trId);

        video_sample_max_size = MP4GetTrackMaxSampleSize(mp4File, video_trId);
    }
printf("\n3. Video Properties.");

    //////////////////////////////////
    /////                        /////
    /////  4. Audio Properties.  /////
    /////                        /////
    //////////////////////////////////
    if (audio_trId != MP4_INVALID_TRACK_ID) {
        audio_num_samples     = MP4GetTrackNumberOfSamples(mp4File, audio_trId);
        audio_num_channels    = MP4GetTrackAudioChannels(mp4File, audio_trId);

        audio_timescale       = MP4GetTrackTimeScale(mp4File, audio_trId);
        audio_duration        = MP4GetTrackDuration(mp4File, audio_trId);

        audio_sample_max_size = MP4GetTrackMaxSampleSize(mp4File, audio_trId);
    }

printf("\n4. Audio Properties.");

    /////////////////////////////////////
    /////                           /////
    /////  4. Video Stream Header.  /////
    /////                           /////
    /////////////////////////////////////
    if (video_trId != MP4_INVALID_TRACK_ID) {
        if (strcmp(video_name, "mp4v") == 0) {
            p_video_config       = NULL;
            n_video_config_size  = 0;

            b = MP4GetTrackESConfiguration(mp4File, video_trId, &p_video_config, &n_video_config_size);
        }
        else if (strcmp(video_name, "h263") == 0) {

        }
        else if (strcmp(video_name, "avc1") == 0) {
            pp_sps = pn_sps = pp_pps = pn_pps = NULL;

            b = MP4GetTrackH264SeqPictHeaders(mp4File, video_trId, &pp_sps, &pn_sps, &pp_pps, &pn_pps);
        }
        else
            printf("\nVideo = Unknown");
    }


    /////////////////////////////////////
    /////                           /////
    /////  5. Audio Stream Header.  /////
    /////                           /////
    /////////////////////////////////////
    if (audio_trId != MP4_INVALID_TRACK_ID) {
        if (strcmp(audio_name, "mp4a") == 0) {
            p_audio_config       = NULL;
            n_audio_config_size  = 0;

            b = MP4GetTrackESConfiguration(mp4File, audio_trId, &p_audio_config, &n_audio_config_size);
        }
        else
            printf("\nVideo = Unknown");
    }



    tick = GetTickCount();


    ////////////////////////////////////
    /////                          /////
    /////  6. Read Video Samples.  /////
    /////                          /////
    ////////////////////////////////////
    if (video_trId != MP4_INVALID_TRACK_ID) {
        p_video_sample = (u_int8_t *) malloc(video_sample_max_size);

        for (sampleId=1; sampleId<=video_num_samples; sampleId++) {
            n_video_sample = video_sample_max_size;

            /////////////////////////////////
            /////     MP4ReadSample     /////
            /////////////////////////////////
            b = MP4ReadSample(mp4File, video_trId, sampleId,
                              &p_video_sample, &n_video_sample,
                              NULL, NULL, NULL, NULL);
            if (!b) {
                printf("\n ERROR [%d] \n", sampleId);
                break;
            }

//            printf("\n [%d] = %d", sampleId, n_video_sample);
        }

        free(p_video_sample); p_video_sample = NULL; n_video_sample = 0;
    }


    tick = GetTickCount() - tick;
    printf("\n\n TOTAL TIME = %u  (%d frames)\n", tick, video_num_samples);


    ///////////////////////////////////////////
    /////                                 /////
    /////  7. Close the mp4 file handle.  /////
    /////                                 /////
    ///////////////////////////////////////////
    MP4Close(mp4File); 

    return 0;
}

