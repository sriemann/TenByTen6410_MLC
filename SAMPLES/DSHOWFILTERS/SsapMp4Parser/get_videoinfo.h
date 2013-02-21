//------------------------------------------------------------------------------
// File: get_videoinfo.h
//
// Desc: FrameExtractFilter DirectShow filter (CLSIDs)
//
//------------------------------------------------------------------------------

#if !defined(_SAMSUNG_SYSLSI_GET_VIDEOINFO_H_)
#define _SAMSUNG_SYSLSI_GET_VIDEOINFO_H_

#ifdef __cplusplus
extern "C" {
#endif

int get_mpeg4_info(unsigned char *vos, int vos_leng, int *profile, int *width, int *height);
int get_h263_info(unsigned char *frame, int frame_leng, int *profile, int *width, int *height);
int get_h264_info(unsigned char *sps, int sps_leng, int *profile_idc, int *width, int *height);


#ifdef __cplusplus
}
#endif

#endif // !defined(_SAMSUNG_SYSLSI_GET_VIDEOINFO_H_)
