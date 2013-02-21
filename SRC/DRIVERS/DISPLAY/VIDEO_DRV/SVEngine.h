
#ifndef _SVENGINE_H_
#define _SVENGINE_H_

#include "s3c6410_display_con.h"
#include "s3c6410_ldi.h"
#include "s3c6410_post_proc.h"
#include "s3c6410_image_rotator.h"
#include "s3c6410_tv_scaler.h"
#include "s3c6410_tv_encoder.h"
#include "SVE_API.h"

#define __MODULE__  "S3C6410 Video Driver"

#define ZONEID_ERROR                0
#define ZONEID_WARNING              1
#define ZONEID_PERF                 2
#define ZONEID_TEMP                 3
#define ZONEID_ENTER                4
#define ZONEID_INIT                 5
#define ZONEID_POST                 6
#define ZONEID_ROTATOR              7
#define ZONEID_TVENCODER            8
#define ZONEID_TVSCALER             9
#define ZONEID_IOCTL                10

#define VDE_ZONE_ERROR             DEBUGZONE(ZONEID_ERROR)
#define VDE_ZONE_WARNING           DEBUGZONE(ZONEID_WARNING)
#define VDE_ZONE_PERF              DEBUGZONE(ZONEID_PERF)
#define VDE_ZONE_TEMP              DEBUGZONE(ZONEID_TEMP)
#define VDE_ZONE_ENTER             DEBUGZONE(ZONEID_ENTER)
#define VDE_ZONE_INIT              DEBUGZONE(ZONEID_INIT)
#define VDE_ZONE_POST              DEBUGZONE(ZONEID_POST)
#define VDE_ZONE_ROTATOR           DEBUGZONE(ZONEID_ROTATOR)
#define VDE_ZONE_TVENCODER         DEBUGZONE(ZONEID_TVENCODER)
#define VDE_ZONE_TVSCALER          DEBUGZONE(ZONEID_TVSCALER)
#define VDE_ZONE_IOCTL             DEBUGZONE(ZONEID_IOCTL)

#define ZONEMASK_ERROR         (1 << ZONEID_ERROR)
#define ZONEMASK_WARNING       (1 << ZONEID_WARNING)
#define ZONEMASK_PERF          (1 << ZONEID_PERF)
#define ZONEMASK_TEMP          (1 << ZONEID_TEMP)
#define ZONEMASK_ENTER         (1 << ZONEID_ENTER)
#define ZONEMASK_INIT          (1 << ZONEID_INIT)
#define ZONEMASK_POST          (1 << ZONEID_POST)
#define ZONEMASK_ROTATOR       (1 << ZONEID_ROTATOR)
#define ZONEMASK_TVENCODER     (1 << ZONEID_TVENCODER)
#define ZONEMASK_TVSCALER      (1 << ZONEID_TVSCALER)
#define ZONEMASK_IOCTL         (1 << ZONEID_IOCTL)

#ifndef VDE_DEBUGZONES
#define VDE_DEBUGZONES          (ZONEMASK_ERROR | ZONEMASK_WARNING | ZONEID_POST | \
                                 ZONEID_ROTATOR )
#endif
#ifndef VDE_RETAILZONES
#define VDE_RETAILZONES          (ZONEMASK_ERROR)
#endif
#ifdef  DEBUG
#define VDE_ZONES VDE_DEBUGZONES
#else
#define VDE_ZONES VDE_RETAILZONES
#endif

#define VDE_MSG(x)	  RETAILMSG(FALSE, x)
#define VDE_INF(x)    RETAILMSG(FALSE, x)
#define VDE_ERR(x)    DEBUGMSG(VDE_ZONE_ERROR, x)


#define SVE_DRIVER_SIGNITURE            (0xD3EC6400)
#define SVE_ERROR_BASE                  (0x20000000)        // Non system error code

#define SVE_IST_PRIORITY                (100)

#define SVE_DISP_CMD_TIMEOUT            (100)    // Optimal Period of Vsync is 16.7ms (60Hz)
#define SVE_POST_CMD_TIMEOUT            (100)    //(35)    // Optimal Period of Vsync is 16.7ms (60Hz)
#define SVE_TVSC_CMD_TIMEOUT            (100)    // Optimal Period of Vsync is 16.7ms (60Hz)

#define SVE_ROTATOR_FINISH_TIMEOUT      (100)    // Optimal Period of Vsync is 16.7ms (60Hz)

typedef struct _DispConCommandContext
{
    CRITICAL_SECTION    csCmd;
    BOOL    bCmdSetBuffer;
    BOOL    bCmdSetPosition;
    DWORD    dwBuffer;
    DWORD    dwOffsetX;
    DWORD    dwOffsetY;
} DispConCommandContext;

typedef struct _PostCommandContext
{
    CRITICAL_SECTION    csCmd;
    BOOL    bCmdSetSrcBuffer;
    BOOL    bCmdSetDstBuffer;
} PostCommandContext;

typedef struct _LocalPathCommandContext
{
    CRITICAL_SECTION    csCmd;
    BOOL    bCmdSetWin0Enable;        // Depricated
    BOOL    bCmdSetWin0Disable;
    BOOL    bCmdSetWin1Enable;        // Depricated
    BOOL    bCmdSetWin1Disable;
    BOOL    bCmdSetWin2Enable;        // Depricated
    BOOL    bCmdSetWin2Disable;
} LocalPathCommandContext;

typedef struct _TVSCCommandContext
{
    CRITICAL_SECTION    csCmd;
    BOOL    bCmdSetSrcBuffer;
    BOOL    bCmdSetDstBuffer;
} TVSCCommandContext;

typedef struct _SVEngineContext
{
    // Driver Signature
    DWORD    dwSignature;

    // Video-related Hardware SFR
    volatile S3C6410_DISPLAY_REG    *pDispConReg;    // Display Controller SFR
    volatile S3C6410_POST_REG        *pPostReg;        // Post Processor SFR
    volatile S3C6410_ROTATOR_REG    *pRotatorReg;    // Post Processor SFR
    volatile S3C6410_TVSC_REG        *pTVSCReg;        // TV Scaler SFR
    volatile S3C6410_TVENC_REG    *pTVEncReg;        // TV Encoder SFR
    volatile S3C6410_MSMIF_REG    *pMSMIFReg;    // MSM I/F SFR
    volatile S3C6410_SYSCON_REG    *pSysConReg;    // Syscon SFR
    volatile S3C6410_GPIO_REG        *pGPIOReg;        // GPIO SFR
    volatile S3C6410_SPI_REG        *pSPIReg;        // SPI Controller SFR

    // Interrupt
    DWORD    dwSysIntrDisp;        // LCD VSYNC SysIntr
    DWORD    dwSysIntrPost;        // Post Processor SysIntr
    DWORD    dwSysIntrTVSC;        // TV Scaler SysIntr
    DWORD    dwSysIntrRotator;        // Rotator SysIntr
    HANDLE    hInterruptDisp;
    HANDLE    hInterruptPost;
    HANDLE    hInterruptTVSC;
    HANDLE    hInterruptRotator;

    // Interrupt Thread
    HANDLE    hDisplayIST;
    HANDLE    hPostIST;
    HANDLE    hTVScalerIST;
    HANDLE    hRotatorIST;

    // Handle for Power Control Driver
    HANDLE    hPowerControl;

    // Critical Section for IOCTL Proc
    CRITICAL_SECTION csProc;

    // HW status Flags
    volatile BOOL bVideoEnable;
    volatile BOOL bWindowEnable[DISP_WIN_MAX];
    volatile BOOL bRotatorBusy;

    // Command Event
    DispConCommandContext    DispCmdCtxt[DISP_WIN_MAX];
    PostCommandContext        PostCmdCtxt;
    LocalPathCommandContext    LocalPathCmdCtxt;
    TVSCCommandContext        TVSCCmdCtxt;
    HANDLE hDispCmdDone;        // Display Controller Cmd Event
    HANDLE hPostCmdDone;        // Post Processor Cmd Event
    HANDLE hTVSCCmdDone;        // TV Scaler Cmd Event

    // Operation Finish Event
    HANDLE hRotatorFinish;        // Image Rotator Finish Event

    // Thread Loop Termination Trigger
    BOOL bDispThrdExit;
    BOOL bPostThrdExit;
    BOOL bTVSCThrdExit;
    BOOL bRotatorThrdExit;
    

    // Open Context of Resource Occupant
    DWORD dwOccupantFIMD;
    DWORD dwOccupantFIMDWindow[DISP_WIN_MAX];
    DWORD dwOccupantPost;
    DWORD dwOccupantRotator;
    DWORD dwOccupantTVScalerTVEncoder;

    DWORD dwOpenCount;
    DWORD dwLastOpenContext;
} SVEngineContext, *pSVEngineContext;


typedef struct _SVEnginePowerContext
{
    // Power State
    BOOL bPowerOn;

    // SVE_FIMD_SET_INTERFACE_PARAM
    BOOL bFIMDOutputParam;
    SVEARG_FIMD_OUTPUT_IF tFIMDOutputParam;

    // SVE_FIMD_SET_OUTPUT_RGBIF/TV
    BOOL bFIMDOutputTV;

    // SVE_FIMD_SET_OUTPUT_ENABLE/DISABLE
    BOOL bFIMDOutputEnable;

    // SVE_FIMD_SET_WINDOW_MODE/POSITION
    BOOL bFIMDWinMode[DISP_WIN_MAX];
    SVEARG_FIMD_WIN_MODE tFIMDWinMode[DISP_WIN_MAX];

    // SVE_FIMD_SET_WINDOW_FRAMEBUFFER
    BOOL bFIMDWinFBuffer[DISP_WIN_MAX];
    SVEARG_FIMD_WIN_FRAMEBUFFER tFIMDWinFBuffer[DISP_WIN_MAX];    // Doesn't care about which window number, "dwWinNum"

    // SVE_FIMD_SET_WINDOW_BLEND_COLORKEY
    BOOL bFIMDColorKey[DISP_WIN_MAX];
    SVEARG_FIMD_WIN_COLORKEY tFIMDColorKey[DISP_WIN_MAX];    // Doesn't care about which window number, "dwWinNum"

    // SVE_FIMD_SET_WINDOW_BLEND_ALPHA
    BOOL bFIMDAlpha[DISP_WIN_MAX];
    SVEARG_FIMD_WIN_ALPHA tFIMDAlpha[DISP_WIN_MAX];    // Doesn't care about which window number, "dwWinNum"

    // SVE_FIMD_SET_WINDOW_ENABLE/DISABLE
    BOOL bFIMDWinEnable[DISP_WIN_MAX];

    // SVE_POST_SET_PROCESSING_PARAM
    BOOL bPostParam;
    SVEARG_POST_PARAMETER     tPostParam;

    // SVE_POST_SET_SOURCE_BUFFER/SVE_POST_SET_NEXT_SOURCE_BUFFER
    BOOL bPostSrcBuffer;
    SVEARG_POST_BUFFER    tPostSrcBuffer;

    // SVE_POST_SET_DESTINATION_BUFFER/SVE_POST_SET_NEXT_DESTINATION_BUFFER
    BOOL bPostDstBuffer;
    SVEARG_POST_BUFFER tPostDstBuffer;

    // SVE_POST_SET_PROCESSING_START/STOP
    BOOL bPostStart;            // Need to Trigger

    // SVE_LOCALPATH_SET_WINX_START/STOP
    BOOL bLocalPathWin0Enable;
    BOOL bLocalPathWin1Enable;
    BOOL bLocalPathWin2Enable;

    // SVE_ROTATOR_SET_OPERATION_PARAM
    BOOL bRotatorParam;
    SVEARG_ROTATOR_PARAMETER tRotatorParam;

    // SVE_ROTATOR_SET_SOURCE_BUFFER
    BOOL bRotatorSrcBuffer;
    SVEARG_ROTATOR_BUFFER tRotatorSrcBuffer;

    // SVE_ROTATOR_SET_DESTINATION_BUFFER
    BOOL bRotatorDstBuffer;
    SVEARG_ROTATOR_BUFFER tRotatorDstBuffer;

    // SVE_ROTATOR_SET_OPERATION_START/STOP
    BOOL bRotatorStart;        // Need to Trigger

    // SVE_TVSC_SET_PROCESSING_PARAM
    BOOL bTVSCParam;
    SVEARG_TVSC_PARAMETER tTVSCParam;

    // SVE_TVSC_SET_SOURCE_BUFFER/SVE_TVSC_SET_NEXT_SOURCE_BUFFER
    BOOL bTVSCSrcBuffer;
    SVEARG_TVSC_BUFFER tTVSCSrcBuffer;

    // SVE_TVSC_SET_DESTINATION_BUFFER/SVE_TVSC_SET_NEXT_DESTINATION_BUFFER
    BOOL bTVSCDstBuffer;
    SVEARG_TVSC_BUFFER tTVSCDstBuffer;

    // SVE_TVSC_SET_PROCESSING_START/STOP
    BOOL bTVSCStart;        // Need to Trigger

    // SVE_TVENC_SET_INTERFACE_PARAM
    BOOL bTVEncParam;
    SVEARG_TVENC_PARAMETER tTVEncParam;

    // SVE_TVENC_SET_ENCODER_ON/OFF
    BOOL bTVEncEnable;
} SVEnginePowerContext, *pSVEnginePowerContext;

typedef enum
{
    HWCLK_ALL_ON = 0,        // 2D Clock is not included
    HWCLK_ALL_OFF,        // 2D Clock is not included
    HWCLK_DISPLAY_ON,
    HWCLK_DISPLAY_OFF,
    HWCLK_MSMIF_ON,
    HWCLK_MSMIF_OFF,
    HWCLK_POST_ON,
    HWCLK_POST_OFF,
    HWCLK_ROTATOR_ON,
    HWCLK_ROTATOR_OFF,
    HWCLK_TV_ON,
    HWCLK_TV_OFF,
    HWCLK_2D_ON,
    HWCLK_2D_OFF
} HWCLK_GATING;

typedef enum
{
    HWPWR_ALL_ON = 0,        // 2D Power is not included
    HWPWR_ALL_OFF,        // 2D Power is not included
    HWPWR_DISPLAY_ON,
    HWPWR_DISPLAY_OFF,
    HWPWR_POST_ON,
    HWPWR_POST_OFF,
    HWPWR_ROTATOR_ON,
    HWPWR_ROTATOR_OFF,
    HWPWR_TV_ON,
    HWPWR_TV_OFF,
    HWPWR_2D_ON,
    HWPWR_2D_OFF
} HWPWR_GATING;

// SVEngine
BOOL SVE_initialize_video_engine(void);
void SVE_deinitialize_video_engine(void);
void SVE_initialize_context(void);
void SVE_deinitialize_context(void);
SVEngineContext* SVE_get_context(void);
SVEnginePowerContext* SVE_get_power_context(void);
BOOL SVE_map_device_address(void);
void SVE_unmap_device_address(void);
DWORD SVE_get_api_function_code(DWORD dwCode);
DWORD SVE_get_driver_signature(void);
DWORD SVE_add_open_context(void);
BOOL SVE_remove_open_context(DWORD dwOpenContext);
DWORD SVE_get_current_open_count(void);

// H/W Resource management
BOOL SVE_resource_request_FIMD_interface(DWORD dwOpenContext);
BOOL SVE_resource_release_FIMD_interface(DWORD dwOpenContext);
BOOL SVE_resource_compare_FIMD_interface(DWORD dwOpenContext);
BOOL SVE_resource_request_FIMD_window(DWORD dwWinNum, DWORD dwOpenContext);
BOOL SVE_resource_release_FIMD_window(DWORD dwWinNum, DWORD dwOpenContext);
BOOL SVE_resource_compare_FIMD_window(DWORD dwWinNum, DWORD dwOpenContext);
BOOL SVE_resource_request_Post(DWORD dwOpenContext);
BOOL SVE_resource_release_Post(DWORD dwOpenContext);
BOOL SVE_resource_compare_Post(DWORD dwOpenContext);
BOOL SVE_resource_request_Rotator(DWORD dwOpenContext);
BOOL SVE_resource_release_Rotator(DWORD dwOpenContext);
BOOL SVE_resource_compare_Rotator(DWORD dwOpenContext);
BOOL SVE_resource_request_TVScaler_TVEncoder(DWORD dwOpenContext);
BOOL SVE_resource_release_TVScaler_TVEncoder(DWORD dwOpenContext);
BOOL SVE_resource_compare_TVScaler_TVEncoder(DWORD dwOpenContext);

// Interrupt & Command
BOOL SVE_initialize_interrupt(void);
void SVE_deinitialize_interrupt(void);
BOOL SVE_initialize_thread(void);
void SVE_deinitialize_thread(void);
DWORD SVE_DisplayIntrThread(void);
DWORD SVE_PostIntrThread(void);
DWORD SVE_RotatorIntrThread(void);
DWORD SVE_TVScalerIntrThread(void);
DWORD SVE_wait_disp_cmd_done(void);
DWORD SVE_wait_post_cmd_done(void);
DWORD SVE_wait_tvsc_cmd_done(void);
DWORD SVE_wait_rotator_finish(void);

// Power Control
void SVE_initialize_power_context(void);
void SVE_deinitialize_power_context(void);
BOOL SVE_initialize_power_control(void);
void SVE_deinitialize_power_control(void);
BOOL SVE_hw_clock_control(HWCLK_GATING eParams);
BOOL SVE_hw_power_control(HWPWR_GATING eParams);
void SVE_video_engine_power_on(void);
void SVE_video_engine_power_off(void);

// Resource API Proc
BOOL SVE_Resource_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

// Display Controller API Proc
BOOL SVE_DispCon_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

// Post Processor API Proc
BOOL SVE_Post_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

// Post-FIMD Local Path API Proc
BOOL SVE_LocalPath_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

// Image Rotator API Proc
BOOL SVE_Rotator_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

// TV Scaler API Proc
BOOL SVE_TVScaler_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

// TV Encoder API Proc
BOOL SVE_TVEncoder_API_Proc(
    DWORD hOpenContext,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
    );

#endif    // _SVENGINE_H_

