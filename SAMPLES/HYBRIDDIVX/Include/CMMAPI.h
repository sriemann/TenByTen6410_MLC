#ifndef __CMM_API_H__
#define __CMM_API_H__

#if __cplusplus
extern "C" {
#endif

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define IOCTL_CODEC_MEM_ALLOC                CTL_CODE( 0, 0x820, 0, 0 )
#define IOCTL_CODEC_MEM_FREE                CTL_CODE( 0, 0x821, 0, 0 )
#define IOCTL_CODEC_CACHE_FLUSH            CTL_CODE( 0, 0x822, 0, 0 )
#define IOCTL_CODEC_GET_PHY_ADDR            CTL_CODE( 0, 0x823, 0, 0 )
#define CODEC_MEM_DRIVER_NAME        L"CMM1:"

typedef struct tagCMM_ALLOC_PRAM_T{
    char                    cacheFlag;
    unsigned int            size;       // memory size
}CMM_ALLOC_PRAM_T;

#if __cplusplus
}
#endif

#endif //__CMM_API_H__