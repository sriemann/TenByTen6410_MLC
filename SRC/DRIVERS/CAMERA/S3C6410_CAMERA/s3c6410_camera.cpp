//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s3c6410_camera.cpp

Abstract:       Handle Camera device in Low level abstract

Functions:


Notes:


--*/

#include <bsp.h>
#include <types.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <pm.h>
#include "pmplatform.h"
#include <pmplatform.h>
#include "s3c6410_camera.h"
#include "Module.h"

// For Debug
#define CAM_MSG                0
#define CAM_INOUT            0
#define CAM_ERR                1
// Macros

// Definitions
//#define CAM_CLK_DIV            6
#define CAM_CLK_SOURCE            (S3C6410_HCLKx2)


// Register Control bit definition
// For CISRCFMT
#define CAM_MODE_ITU601_BIT             (1<<31)
// For CIGCTRL
#define CAM_EXTCAMERA_SWRESET_BIT       (1<<31)
#define CAM_EXTCAMERA_PROC_RESET_BIT    (1<<30)
#define CAM_CIGCTRL_RESERVED1           (1<<29)
#define CAM_CIGCTRL_EXTCAMERA_INPUT     (0<<27)
#define CAM_CIGCTRL_COLORBAR_TEST_PATTERN   (1<<27)
#define CAM_CIGCTRL_HORI_INCR_TEST_PATTERN  (2<<27)
#define CAM_CIGCTRL_VERT_INCR_TEST_PATTERN  (3<<27)

#define CAM_CODEC_SACLER_START_BIT            (1<<15)
#define CAM_PVIEW_SACLER_START_BIT            (1<<15)

#define CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT        (1<<31)
#define CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT        (1<<30)
#define CAM_PVIEW_SCALER_CAPTURE_ENABLE_BIT        (1<<29)

#define CAM_OFFSET_STEP                        (4)

// structures
typedef struct {
    UINT32    Width;
    UINT32    Height;
    int        Format;
    int     Size;
    int        FrameSize;
} BUFFER_DESC;

// Variables

// Pointer to camera driver instance which we will send back with callback functions
DWORD dwCameraDriverContext;

// Signals the application that the video or still image frame is available
PFNCAMHANDLEFRAME pfnCameraHandleVideoFrame = NULL;
PFNCAMHANDLEFRAME pfnCameraHandleStillFrame = NULL;
PFNCAMHANDLEFRAME pfnCameraHandlePreviewFrame = NULL;

volatile S3C6410_GPIO_REG *s6410IOP = NULL;
volatile S3C6410_CAMIF_REG    *s6410CAM = NULL;
//volatile S3C6410_INTR_REG    *s6410INT = NULL;
volatile S3C6410_SYSCON_REG *s6410PWR = NULL;

//mio
static BUFFER_DESC    Video_Buffer;
static BUFFER_DESC    Still_Buffer;
static BUFFER_DESC    Preview_Buffer;

static PHYSICAL_ADDRESS PhysPreviewAddr;
static PHYSICAL_ADDRESS PhysCodecAddr;

static PBYTE    pPreviewVirtAddr;
static PBYTE    pCodecVirtAddr;

static BYTE     PreviewOn=0;
static BYTE     CodecOn=0;
static BYTE     VideoOn=0;
static BYTE     StillOn=0;

static UINT32   PreviewFrameCnt=0;            // this is for skipping first 3 frames. Because first 3 frames are useless.
static UINT32   CodecFrameCnt=0;            // this is for skipping first 3 frames. Because first 3 frames are useless.

static INT32    g_iHorOffset1=0;
static INT32    g_iVerOffset1=0;
static INT32    g_iHorOffset2=0;
static INT32    g_iVerOffset2=0;
static UINT32   g_uCamIrqForCapture = IRQ_CAMIF_C;
static UINT32   g_uCamSysIntrForCapture = SYSINTR_UNDEFINED;
static UINT32   g_uCamIrqForPreview = IRQ_CAMIF_P;
static UINT32   g_uCamSysIntrForPreview = SYSINTR_UNDEFINED;

static HANDLE   hCaptureThread;
static BOOL     bCaptureThreadExit = FALSE;
static HANDLE   hCaptureEvent;
static HANDLE   hPreviewThread;
static BOOL     bPreviewThreadExit = FALSE;
static HANDLE   hPreviewEvent;

static HANDLE   hPwrControl;
// Functions

static BOOL     bPowerOn=TRUE;

static void CameraGpioInit();               
static void CameraInterfaceReset();     
static void CameraModuleReset();        
static void CameraSetClockDiv();        
static void CameraCaptureSourceSet();        // Set source registers

static void CameraSetCodecRegister(UINT32 width, UINT32 height, int Format);
static void CameraSetPreviewRegister(UINT32 width, UINT32 height, int Format);

static void CameraSetScaler(UINT32 width, UINT32 height, int path);
static void CalculateBurstSize(unsigned int hSize,unsigned int *mainBurstSize,unsigned int *remainedBurstSize);
static void CalculatePrescalerRatioShift(unsigned int SrcSize, unsigned int DstSize, unsigned int *ratio,unsigned int *shift);

static UINT32 CalculateBufferSize(UINT32 width, UINT32 height, int format);

static BOOL InitializeBuffer();
static BOOL DeinitializeBuffer();

static BOOL InterruptInitialize();

static DWORD WINAPI CameraCaptureThread(void);
static DWORD WINAPI CameraPreviewThread(void);

static void Delay(UINT32 count)
{
    volatile int i, j = 0;
    volatile static int loop = S3C6410_ACLK/100000;

    for(;count > 0;count--)
        for(i=0;i < loop; i++) { j++; }
}

int CameraInit(void *pData)
{
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
    RETAILMSG(CAM_INOUT,(TEXT("++%s\n"), __FUNCTION__));
    // 0. Map to Virtual Address

    // GPIO Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    s6410IOP = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (s6410IOP == NULL)
    {
        goto CleanUp;
    }

    // Camera Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_CAMIF;
    s6410CAM = (S3C6410_CAMIF_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_CAMIF_REG), FALSE);
    if (s6410CAM == NULL)
    {
        goto CleanUp;
    }

    // PWM clock Virtual alloc
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
    s6410PWR = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (s6410PWR == NULL)
    {
        goto CleanUp;
    }

    hPwrControl = CreateFile( L"PWC0:", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == hPwrControl )
    {
        RETAILMSG(CAM_ERR, (TEXT("[CAM] CameraInit() : PWC0 Open Device Failed\r\n")));
        return FALSE;
    }

    ModuleInit();

    CameraSetClockDiv();

    // 1. Camera IO setup
    CameraGpioInit();    
    
    // 2. Camera Clock setup
    CameraClockOn(TRUE);

    // 3. camera module reset
    CameraModuleReset();    
    /*
    // Reserved Step
    // 4. Write Setting for Module using I2C
    if(!ModuleWriteBlock())
    {
        return FALSE;
    }    
    
    // 5. Camera i/f reset
    CameraInterfaceReset();

    // 6. Initialize I/F source register
    CameraCaptureSourceSet();
    */
    // 7. Camera Clock Off
    CameraClockOn(FALSE);  
        
    // 8. Allocation Buffer();
    if(!InitializeBuffer())
    {
        return FALSE;
    }

    // 9. Interrupt Initlaize();
    if(!InterruptInitialize())
    {
        return FALSE;
    }

    RETAILMSG(CAM_INOUT,(TEXT("--%s Succeeded\n"), __FUNCTION__));

    return TRUE;

CleanUp:
    RETAILMSG(1,(TEXT("%s : Failed, ioPhysicalBase(0x%x,0x%x)\r\n"), __FUNCTION__, ioPhysicalBase.LowPart, ioPhysicalBase.HighPart));
    return FALSE;
}

void CameraDeinit()
{
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraDeInit\n")));

    if(s6410IOP != NULL)
    {
        MmUnmapIoSpace((PVOID)s6410IOP, sizeof(S3C6410_GPIO_REG));
        s6410IOP = NULL;
    }
    if(s6410CAM != NULL)
    {
        MmUnmapIoSpace((PVOID)s6410CAM, sizeof(S3C6410_CAMIF_REG));
        s6410CAM = NULL;
    }
    if(s6410PWR != NULL)
    {
        MmUnmapIoSpace((PVOID)s6410PWR, sizeof(S3C6410_SYSCON_REG));
        s6410PWR = NULL;
    }

    ModuleDeinit();
    DeinitializeBuffer();

    
//    CloseHandle(hCaptureThread);
//    CloseHandle(hPreviewThread);
    bCaptureThreadExit = TRUE;
    SetEvent(hCaptureEvent);
    bPreviewThreadExit = TRUE;
    SetEvent(hPreviewEvent);
    CloseHandle(hCaptureEvent);
    CloseHandle(hPreviewEvent);
    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraDeInit\n")));
}

void CameraGpioInit()        //    Initialize GPIO setting for Camera Interface
{
    s6410IOP->GPFPUD = (s6410IOP->GPFPUD & ~(0x3ffffff));         // CAM IO PullUpDown Disable setup except CAMRESET
    s6410IOP->GPFCON = (s6410IOP->GPFCON & ~(0x3ffffff)) | 0x2aaaaaa;
}

void CameraInterfaceReset()    // Reset Camera Inteface IP
{
    // This functin is used on power handler operation.
    // So, you should not use Kernel API functions as like as "Sleep()".

    MODULE_DESCRIPTOR value;

    ModuleGetFormat(value);
    //
    // Camera (FIMC2.0) I/F Reset
    //
    // Recommended Camera Interface reset sequence in S3C6410 User manual
    s6410CAM->CISRCFMT |= (CAM_MODE_ITU601_BIT);
    s6410CAM->CIGCTRL |= (CAM_EXTCAMERA_SWRESET_BIT);
    s6410CAM->CIGCTRL &= ~(CAM_EXTCAMERA_SWRESET_BIT);

    if(value.ITUXXX == CAM_ITU656)
    {
        s6410CAM->CISRCFMT &= ~(CAM_MODE_ITU601_BIT);
    }
}

void CameraModuleReset()        // Reset Camera Module
{
    MODULE_DESCRIPTOR value;

    ModuleGetFormat(value);

    if(value.HighRst)
    {
        s6410CAM->CIGCTRL |= (CAM_EXTCAMERA_PROC_RESET_BIT);
        // Don't modify this delay time
        Delay(100);
        s6410CAM->CIGCTRL &= ~(CAM_EXTCAMERA_PROC_RESET_BIT);
        // Wait for Camera module initialization
        Delay(1000);
    }
    else
    {
        s6410CAM->CIGCTRL &= ~(CAM_EXTCAMERA_PROC_RESET_BIT);
        // Wait for Camera module initialization
        Delay(100);

        s6410CAM->CIGCTRL |= (CAM_EXTCAMERA_PROC_RESET_BIT);
        // Don't modify this delay time
        Delay(1000);
    }
}

void CameraClockOn(BOOL bOnOff)
{
    DWORD dwIPIndex = PWR_IP_CAMIF;
    DWORD dwBytes;    
    static int isOn = 0;
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++[CAM] CameraClockOn %d\n"),bOnOff));
    // Camera clock
    if (!bOnOff)
    {
        if(isOn == 1)
        {
            isOn = 0;
            s6410PWR->HCLK_GATE &= ~(1<<10); // Camera clock disable
            s6410PWR->SCLK_GATE &= ~(1<<2); // Camera clock disable        
            if ( !DeviceIoControl(hPwrControl, IOCTL_PWRCON_SET_POWER_OFF, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(CAM_ERR,(TEXT("[CAM:ERR] CameraClockOn(%d) : IOCTL_PWRCON_SET_POWER_OFF Failed\r\n")));
            }            
            bPowerOn=FALSE;
        }
    }
    else 
    {
        if(isOn == 0)
        {
            isOn = 1;
            
            if ( !DeviceIoControl(hPwrControl, IOCTL_PWRCON_SET_POWER_ON, &dwIPIndex, sizeof(DWORD), NULL, 0, &dwBytes, NULL) )
            {
                RETAILMSG(CAM_ERR,(TEXT("[CAM:ERR] CameraClockOn(%d) : IOCTL_PWRCON_SET_POWER_OFF Failed\r\n")));
            }        

            bPowerOn=TRUE;

            s6410PWR->HCLK_GATE |= (1<<10); // Camera clock enable
            s6410PWR->SCLK_GATE |= (1<<2); // Camera clock enable        
            Delay(1000);
        }
    }
    RETAILMSG(CAM_INOUT,(TEXT("------------------[CAM] CameraClockOn\n")));
}

void CameraCaptureSourceSet()        // Set source registers
{
    UINT32 WinOfsEn=0;
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraCaptureSourceSet\n")));
    MODULE_DESCRIPTOR value;

    ModuleGetFormat(value);

    s6410CAM->CIGCTRL = ((value.HighRst ? (0<<30) : (CAM_EXTCAMERA_PROC_RESET_BIT))) |
                        (CAM_CIGCTRL_RESERVED1) | (CAM_CIGCTRL_EXTCAMERA_INPUT) | 
                        (value.InvPCLK<<26)|
                        (value.InvVSYNC<<25)|(value.InvHREF<<24)|(0<<22)|(1<<21)|(1<<20); // inverse PCLK
    s6410CAM->CIWDOFST = (1<<30)|(0xf<<27)|(0xf<<12); // clear overflow
    s6410CAM->CIWDOFST = 0;

    if((value.SourceHOffset > 0) || (value.SourceVOffset > 0))
    {
        WinOfsEn=1;
    }
    s6410CAM->CIWDOFST = (WinOfsEn<<31)|(value.SourceHOffset <<16)|(value.SourceVOffset);
    s6410CAM->CIDOWSFT2 = (value.SourceHOffset <<16)|(value.SourceVOffset);

    //TODO:: Set Offset for scaler
    s6410CAM->CISRCFMT = (value.ITUXXX<<31)|(value.UVOffset<<30)|(0<<29)|
                            (value.SourceHSize<<16)|(value.Order422<<14)|(value.SourceVSize);

    RETAILMSG(CAM_MSG,(TEXT("s6410CAM->CISRCFMT=0x%08X\n"),s6410CAM->CISRCFMT));
    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraCaptureSourceSet\n")));
}

int CameraPrepareBuffer(P_CAMERA_DMA_BUFFER_INFO pBufInfo, int BufferType)        // allocate DMA buffer
{
    int i,size;
    int sizeY,sizeC;

    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraPrepareBuffer\n")));


    if(BufferType == VIDEO_CAPTURE_BUFFER)
    {
        size = CalculateBufferSize(Video_Buffer.Width, Video_Buffer.Height, Video_Buffer.Format);
        Video_Buffer.FrameSize = size;
        Video_Buffer.Size = size * MAX_HW_FRAMES;
        RETAILMSG(CAM_MSG,(TEXT("Video_Buffer.Width=%d Video_Buffer.Height=%d Video_Buffer.Size=%d  Video_Buffer.Format=%d\n"),Video_Buffer.Width, Video_Buffer.Height, Video_Buffer.Size, Video_Buffer.Format));

        if(Video_Buffer.Size > CAPTURE_BUFFER_SIZE)
        {
            RETAILMSG(CAM_ERR,(TEXT("Video size is larger than buffer size\n")));
            return FALSE;
        }

        for(i=0;i<MAX_HW_FRAMES;i++)
        {
            pBufInfo[i].VirtAddr = (DWORD)pCodecVirtAddr + size*i;
            pBufInfo[i].size = size;
            pBufInfo[i].pY = (DWORD*)((DWORD)(PhysCodecAddr.LowPart) + size*i);
        }

        if(OUTPUT_CODEC_YCBCR420 == Video_Buffer.Format)
        {
            sizeY = Video_Buffer.Width*Video_Buffer.Height;
            sizeC = Video_Buffer.Width*Video_Buffer.Height/4;
            for(i=0;i<MAX_HW_FRAMES;i++)
            {
                pBufInfo[i].pCb = (DWORD*)((DWORD)pBufInfo[i].pY + sizeY);
                pBufInfo[i].pCr = (DWORD*)((DWORD)pBufInfo[i].pCb + sizeC);
            }
        }

    }
    else if(BufferType == STILL_CAPTURE_BUFFER)
    {
        size = CalculateBufferSize(Still_Buffer.Width, Still_Buffer.Height, Still_Buffer.Format);
        Still_Buffer.FrameSize = size;
        Still_Buffer.Size = size;
        RETAILMSG(CAM_MSG,(TEXT("Still_Buffer.Width=%d Still_Buffer.Height=%d Still_Buffer.Size=%d  Still_Buffer.Format=%d\n"),Still_Buffer.Width, Still_Buffer.Height, Still_Buffer.Size, Still_Buffer.Format));


        if(Still_Buffer.Size > CAPTURE_BUFFER_SIZE)
        {
            RETAILMSG(CAM_ERR,(TEXT("Still size is larger than buffer size\n")));
            return FALSE;
        }

        pBufInfo[0].VirtAddr = (DWORD)pCodecVirtAddr;
        pBufInfo[0].size = size;
        pBufInfo[0].pY = (DWORD*)(PhysCodecAddr.LowPart);


        if(OUTPUT_CODEC_YCBCR420 == Still_Buffer.Format)
        {
            sizeY = Still_Buffer.Width*Still_Buffer.Height;
            sizeC = Still_Buffer.Width*Still_Buffer.Height/4;
            pBufInfo[0].pCb = (DWORD*)((DWORD)pBufInfo[0].pY + sizeY);
            pBufInfo[0].pCr = (DWORD*)((DWORD)pBufInfo[0].pCb + sizeC);

        }
    }
    else if(BufferType == PREVIEW_CAPTURE_BUFFER)
    {
        size = CalculateBufferSize(Preview_Buffer.Width, Preview_Buffer.Height, Preview_Buffer.Format);
        Preview_Buffer.FrameSize = size;
        Preview_Buffer.Size = size * MAX_HW_FRAMES;
        RETAILMSG(CAM_MSG,(TEXT("Preview_Buffer.Width=%d Preview_Buffer.Height=%d Preview_Buffer.Size=%d Preview_Buffer.Format=%d\n"),Preview_Buffer.Width, Preview_Buffer.Height, Preview_Buffer.Size, Preview_Buffer.Format));

        if(Preview_Buffer.Size > PREVIEW_BUFFER_SIZE)
        {
            RETAILMSG(CAM_ERR,(TEXT("Preview size is larger than buffer size\n")));
            return FALSE;
        }

        for(i=0;i<MAX_HW_FRAMES;i++)
        {
            pBufInfo[i].VirtAddr = (DWORD)pPreviewVirtAddr + size*i;
            pBufInfo[i].size = size;
            pBufInfo[i].pY = (DWORD*)((DWORD)(PhysPreviewAddr.LowPart) + size*i);
        }

        if(OUTPUT_CODEC_YCBCR420 == Preview_Buffer.Format)
        {
            sizeY = Preview_Buffer.Width*Preview_Buffer.Height;
            sizeC = Preview_Buffer.Width*Preview_Buffer.Height/4;
            for(i=0;i<MAX_HW_FRAMES;i++)
            {
                pBufInfo[i].pCb = (DWORD*)((DWORD)pBufInfo[i].pY + sizeY);
                pBufInfo[i].pCr = (DWORD*)((DWORD)pBufInfo[i].pCb + sizeC);
            }
        }
    }
    else
    {
        return FALSE;
    }

    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraPrepareBuffer\n")));
    return TRUE;
}


int     CameraDeallocateBuffer(int BufferType)
{
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraDeallocateBuffer\n")));

    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraDeallocateBuffer\n")));
    return TRUE;
}

int     CameraSetFormat(UINT32 width, UINT32 height, int format, int BufferType)
{
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraSetFormat\n")));
    if(BufferType == VIDEO_CAPTURE_BUFFER)
    {
        Video_Buffer.Width = width;
        Video_Buffer.Height = height;
        Video_Buffer.Format = format;
    }
    else if(BufferType == STILL_CAPTURE_BUFFER)
    {
        Still_Buffer.Width = width;
        Still_Buffer.Height = height;
        Still_Buffer.Format = format;
    }
    else if(BufferType == PREVIEW_CAPTURE_BUFFER)
    {
        Preview_Buffer.Width = width;
        Preview_Buffer.Height = height;
        Preview_Buffer.Format = format;
    }
    else
    {
        return FALSE;
    }
    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraSetFormat\n")));
    return TRUE;
}


void CameraSetCodecRegister(UINT32 width, UINT32 height, int Format)        // set codec register
{    
    MODULE_DESCRIPTOR value;
    BYTE    Out422_Co, Cpt_CoDMA_RGBFMT;
    UINT32 MainBurstSizeY, RemainedBurstSizeY, MainBurstSizeC, RemainedBurstSizeC;    
    
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++SetCodecRegister  Width=%d  Height=%d  Format=%d\n"),width,height,Format));
    ModuleGetFormat(value);
    
    switch(Format)
    {
    case OUTPUT_CODEC_YCBCR422:
        Out422_Co = 2;        // we don't support Non-interleave 4:2:2
        Cpt_CoDMA_RGBFMT = 0;
        CalculateBurstSize(width*2, &MainBurstSizeY, &RemainedBurstSizeY);
        MainBurstSizeY /= 2;
        RemainedBurstSizeY /= 2;
        MainBurstSizeC = MainBurstSizeY/2;
        RemainedBurstSizeC = RemainedBurstSizeY/2;        
        //CalculateBurstSize(width/2, &MainBurstSizeC, &RemainedBurstSizeC);        
        break;
    case OUTPUT_CODEC_RGB16:
        Out422_Co = 3;
        Cpt_CoDMA_RGBFMT = 0;    
        CalculateBurstSize(width*2, &MainBurstSizeY, &RemainedBurstSizeY);        
        MainBurstSizeC = 0;
        RemainedBurstSizeC = 0;
        break;
    case OUTPUT_CODEC_RGB24:
        Out422_Co = 3;
        Cpt_CoDMA_RGBFMT = 2;        
        CalculateBurstSize(width*4, &MainBurstSizeY, &RemainedBurstSizeY);    
        MainBurstSizeC = 0;
        RemainedBurstSizeC = 0;            
        break;
    case OUTPUT_CODEC_YCBCR420:
    default:
        Out422_Co = 0;
        Cpt_CoDMA_RGBFMT = 0;
        CalculateBurstSize(width, &MainBurstSizeY, &RemainedBurstSizeY);
        CalculateBurstSize(width/2, &MainBurstSizeC, &RemainedBurstSizeC);            
        break;
    }    
    
    s6410CAM->CICOTRGFMT= (Out422_Co<<29)|(width<<16)|(height);
    s6410CAM->CICOCTRL=(MainBurstSizeY<<19)|(RemainedBurstSizeY<<14)|(MainBurstSizeC<<9)|(RemainedBurstSizeC<<4)|(0);
    s6410CAM->CICOSCCTRL=(1<<28)|(1<<27)|(0<<26)|(0<<25)|(3<<13)|(Cpt_CoDMA_RGBFMT<<11)|(0<<10);    
    
    CameraSetScaler(width,height,CODEC_PATH);

    s6410CAM->CICOTAREA=width*height;
    
    //s6410CAM->CIIMGCPT ;
    RETAILMSG(CAM_INOUT,(TEXT("------------------SetCodecRegister\n")));
}

void CameraSetPreviewRegister(UINT32 width, UINT32 height, int Format)     // set preview register
{
    MODULE_DESCRIPTOR value;
    BYTE   Out422_Pr, Cpt_PrDMA_RGBFMT;
    UINT32 MainBurstSizeY, RemainedBurstSizeY, MainBurstSizeC, RemainedBurstSizeC;        
        
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++SetPreviewRegister\n")));
    ModuleGetFormat(value);
    
    switch(Format)
    {
    case OUTPUT_CODEC_YCBCR422:
        Out422_Pr = 2;        // we don't support Non-interleave 4:2:2
        Cpt_PrDMA_RGBFMT = 0;
        CalculateBurstSize(width*2, &MainBurstSizeY, &RemainedBurstSizeY);
        CalculateBurstSize(width/2, &MainBurstSizeC, &RemainedBurstSizeC);        
        break;
    case OUTPUT_CODEC_RGB16:
        Out422_Pr = 3;
        Cpt_PrDMA_RGBFMT = 0;    
        CalculateBurstSize(width*2, &MainBurstSizeY, &RemainedBurstSizeY);        
        MainBurstSizeC = 0;
        RemainedBurstSizeC = 0;
        break;
    case OUTPUT_CODEC_RGB24:
        Out422_Pr = 3;
        Cpt_PrDMA_RGBFMT = 2;        
        CalculateBurstSize(width*4, &MainBurstSizeY, &RemainedBurstSizeY);    
        MainBurstSizeC = 0;
        RemainedBurstSizeC = 0;            
        break;
    case OUTPUT_CODEC_YCBCR420:
    default:
        Out422_Pr = 0;
        Cpt_PrDMA_RGBFMT = 0;
        CalculateBurstSize(width, &MainBurstSizeY, &RemainedBurstSizeY);
        CalculateBurstSize(width/2, &MainBurstSizeC, &RemainedBurstSizeC);            
        break;
    }    
    
    s6410CAM->CIPRTRGFMT= (Out422_Pr<<29)|(width<<16)|(height);
    s6410CAM->CIPRCTRL=(MainBurstSizeY<<19)|(RemainedBurstSizeY<<14)|(MainBurstSizeC<<9)|(RemainedBurstSizeC<<4);
    s6410CAM->CIPRSCCTRL=(1<<28)|(1<<27)|(0<<26)|(0<<25)|(3<<13)|(Cpt_PrDMA_RGBFMT<<11)|(0<<10);    
    
    CameraSetScaler(width,height,PREVIEW_PATH);
    
    s6410CAM->CIPRTAREA= width*height;    
    
    s6410CAM->CICPTSEQ = 0xFFFFFFFF;
    s6410CAM->CIMSCTRL = 0;    
    s6410CAM->CIPRSCOSY = 0;
    s6410CAM->CIPRSCOSCB = 0;
    s6410CAM->CIPRSCOSCR = 0;
    
    RETAILMSG(CAM_INOUT,(TEXT("------------------SetPreviewRegister\n")));
}

/********************************************************
 CalculateBurstSize - Calculate the busrt lengths

 Description:
 - dstHSize: the number of the byte of H Size.

*/
void CalculateBurstSize(unsigned int hSize,unsigned int *mainBurstSize,unsigned int *remainedBurstSize)
{
    unsigned int tmp;
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CalculateBurstSize\n")));
    tmp=(hSize/4)%16;
    switch(tmp) {
        case 0:
            *mainBurstSize=16;
            *remainedBurstSize=16;
            break;
        case 4:
            *mainBurstSize=16;
            *remainedBurstSize=4;
            break;
        case 8:
            *mainBurstSize=16;
            *remainedBurstSize=8;
            break;
        default:
            tmp=(hSize/4)%8;
            switch(tmp) {
                case 0:
                    *mainBurstSize=8;
                    *remainedBurstSize=8;
                    break;
                case 4:
                    *mainBurstSize=8;
                    *remainedBurstSize=4;
                    break;
                default:
                    *mainBurstSize=4;
                    tmp=(hSize/4)%4;
                    *remainedBurstSize= (tmp) ? tmp: 4;
                    break;
            }
            break;
    }
    RETAILMSG(CAM_INOUT,(TEXT("------------------CalculateBurstSize\n")));
}

/********************************************************
 CalculatePrescalerRatioShift - none

 Description:
 - none

*/
void CalculatePrescalerRatioShift(unsigned int SrcSize, unsigned int DstSize, unsigned int *ratio,unsigned int *shift)
{
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CalculatePrescalerRatioShift\n")));
    if(SrcSize>=64*DstSize) {
//        Uart_Printf("ERROR: out of the prescaler range: SrcSize/DstSize = %d(< 64)\r\n",SrcSize/DstSize);
        while(1);
    }
    else if(SrcSize>=32*DstSize) {
        *ratio=32;
        *shift=5;
    }
    else if(SrcSize>=16*DstSize) {
        *ratio=16;
        *shift=4;
    }
    else if(SrcSize>=8*DstSize) {
        *ratio=8;
        *shift=3;
    }
    else if(SrcSize>=4*DstSize) {
        *ratio=4;
        *shift=2;
    }
    else if(SrcSize>=2*DstSize) {
        *ratio=2;
        *shift=1;
    }
    else {
        *ratio=1;
        *shift=0;
    }
    RETAILMSG(CAM_INOUT,(TEXT("------------------CalculatePrescalerRatioShift\n")));
}

UINT32 CalculateBufferSize(UINT32 width, UINT32 height, int format)
{
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CalculateBufferSize\n")));
    UINT32 size=0;
    switch(format)
    {
    case OUTPUT_CODEC_YCBCR422:
        size = width*height + width*height/2*2;
        break;
    case OUTPUT_CODEC_YCBCR420:
        size = width*height + width*height/4*2;
        break;
    case OUTPUT_CODEC_RGB24:
        size = width*height*4;
        break;
    case OUTPUT_CODEC_RGB16:
        size = width*height*2;
        break;
    }

    RETAILMSG(CAM_INOUT,(TEXT("------------------CalculateBufferSize\n")));
    return size;
}

int        CameraGetCurrentFrameNum(int BufferType)
{
    int temp;
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraGetCurrentFrameNum\n")));
    if(VIDEO_CAPTURE_BUFFER == BufferType)
    {
        temp = (s6410CAM->CICOSTATUS>>26)&3;
        temp = (temp + 2) % 4;
    }
    else if(STILL_CAPTURE_BUFFER == BufferType)
    {
        temp = (s6410CAM->CICOSTATUS>>26)&3;
        temp = (temp + 2) % 4;
    }
    else if(PREVIEW_CAPTURE_BUFFER == BufferType)
    {
        temp = (s6410CAM->CIPRSTATUS>>26)&3;
        temp = (temp + 2) % 4;
    }
    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraGetCurrentFrameNum\n")));
    return temp;
}

int     CameraCaptureControl(int Format, BOOL on)
{
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraCaptureControl\n")));

    if(!bPowerOn)
    {
        RETAILMSG(CAM_MSG,(TEXT("[CAM] Camera is Power Off state\n")));
        return TRUE; 
    }

    s6410CAM->CIIMGCPT &= ~(CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT);
    if(PREVIEW_CAPTURE_BUFFER == Format)
    {
        if(on)
        {
            RETAILMSG(CAM_MSG,(TEXT("Preview ON\n")));
            PreviewOn = TRUE;
            s6410CAM->CIPRSCCTRL |=(CAM_PVIEW_SACLER_START_BIT);
            s6410CAM->CIIMGCPT |= (CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT)|(CAM_PVIEW_SCALER_CAPTURE_ENABLE_BIT);
        }
        else
        {
            RETAILMSG(CAM_MSG,(TEXT("Preview OFF\n")));
            PreviewFrameCnt = 0;
            PreviewOn = FALSE;
            s6410CAM->CIPRSCCTRL &= ~(CAM_PVIEW_SACLER_START_BIT);
            s6410CAM->CIIMGCPT &= ~(CAM_PVIEW_SCALER_CAPTURE_ENABLE_BIT);
        }
    }
    else    // STILL, VIDEO
    {
        if(on)
        {
            RETAILMSG(CAM_MSG,(TEXT("Capture ON\n")));
            CodecOn = TRUE;
            if(STILL_CAPTURE_BUFFER == Format)
            {
                StillOn = TRUE;
                VideoOn = FALSE;
            }
            else
            {
                StillOn = FALSE;
                VideoOn = TRUE;
            }
            s6410CAM->CICOSCCTRL |=(CAM_CODEC_SACLER_START_BIT);
            s6410CAM->CIIMGCPT |=(CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT)|(CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT);
        }
        else
        {
            RETAILMSG(CAM_MSG,(TEXT("Capture OFF\n")));
            CodecOn = FALSE;
            StillOn = FALSE;
            VideoOn = FALSE;
            CodecFrameCnt = 0;
            s6410CAM->CICOSCCTRL &= ~(CAM_CODEC_SACLER_START_BIT);
            s6410CAM->CIIMGCPT &= ~((CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT));
        }
    }

    if(PreviewOn == FALSE && CodecOn == FALSE)    // All Off
    {
        s6410CAM->CIGCTRL &= ~(1<<20);
    }
    else        // On
    {
        s6410CAM->CIGCTRL |= (1<<20);
        s6410CAM->CIIMGCPT |= (CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT);

        RETAILMSG(CAM_MSG,(TEXT("s6410CAM->CIPRSCCTRL=0x%08X\n"),s6410CAM->CIPRSCCTRL));
        RETAILMSG(CAM_MSG,(TEXT("s6410CAM->CIIMGCPT=0x%08X\n"),s6410CAM->CIIMGCPT));
        RETAILMSG(CAM_MSG,(TEXT("s6410CAM->CIPRSTATUS=0x%08X\n"),s6410CAM->CIPRSTATUS));
    }

    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraCaptureControl\n")));
    return TRUE;
}

int     CameraSetRegisters(int format)
{
    int size;

    int sizeY,sizeC;

    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++CameraSetRegisters\n")));

    if(format == VIDEO_CAPTURE_BUFFER)
    {
        size = Video_Buffer.FrameSize;
        s6410CAM->CICOYSA1 = (UINT32)(PhysCodecAddr.LowPart);
        s6410CAM->CICOYSA2 = s6410CAM->CICOYSA1 + size;
        s6410CAM->CICOYSA3 = s6410CAM->CICOYSA2 + size;
        s6410CAM->CICOYSA4 = s6410CAM->CICOYSA3 + size;

        if(OUTPUT_CODEC_YCBCR420 == Video_Buffer.Format)
        {
            sizeY = Video_Buffer.Width*Video_Buffer.Height;
            sizeC = Video_Buffer.Width*Video_Buffer.Height/4;

            // In DSHOW  CB and Cr are swapped.(YCrCb)
            s6410CAM->CICOCRSA1=s6410CAM->CICOYSA1+sizeY;
            s6410CAM->CICOCRSA2=s6410CAM->CICOYSA2+sizeY;
            s6410CAM->CICOCRSA3=s6410CAM->CICOYSA3+sizeY;
            s6410CAM->CICOCRSA4=s6410CAM->CICOYSA4+sizeY;

            s6410CAM->CICOCBSA1=s6410CAM->CICOCRSA1+sizeC;
            s6410CAM->CICOCBSA2=s6410CAM->CICOCRSA2+sizeC;
            s6410CAM->CICOCBSA3=s6410CAM->CICOCRSA3+sizeC;
            s6410CAM->CICOCBSA4=s6410CAM->CICOCRSA4+sizeC;
        }
        CameraSetCodecRegister(Video_Buffer.Width, Video_Buffer.Height, Video_Buffer.Format);

    }
    else if(format == STILL_CAPTURE_BUFFER)
    {

        size = Still_Buffer.FrameSize;
        s6410CAM->CICOYSA1 = (UINT32)(PhysCodecAddr.LowPart);
        s6410CAM->CICOYSA2 = s6410CAM->CICOYSA1;
        s6410CAM->CICOYSA3 = s6410CAM->CICOYSA1;
        s6410CAM->CICOYSA4 = s6410CAM->CICOYSA1;

        if(OUTPUT_CODEC_YCBCR420 == Still_Buffer.Format)
        {
            sizeY = Still_Buffer.Width*Still_Buffer.Height;
            sizeC = Still_Buffer.Width*Still_Buffer.Height/4;

            // In DSHOW  Cb and Cr are swappedd.
            s6410CAM->CICOCRSA1=s6410CAM->CICOYSA1+sizeY;
            s6410CAM->CICOCRSA2=s6410CAM->CICOCRSA1;
            s6410CAM->CICOCRSA3=s6410CAM->CICOCRSA1;
            s6410CAM->CICOCRSA4=s6410CAM->CICOCRSA1;

            s6410CAM->CICOCBSA1=s6410CAM->CICOCRSA1+sizeC;
            s6410CAM->CICOCBSA2=s6410CAM->CICOCBSA1;
            s6410CAM->CICOCBSA3=s6410CAM->CICOCBSA1;
            s6410CAM->CICOCBSA4=s6410CAM->CICOCBSA1;
        }
        CameraSetCodecRegister(Still_Buffer.Width, Still_Buffer.Height, Still_Buffer.Format);
    }
    else if(format == PREVIEW_CAPTURE_BUFFER)
    {
        size = Preview_Buffer.FrameSize;
        s6410CAM->CIPRYSA1=(UINT32)(PhysPreviewAddr.LowPart);
        s6410CAM->CIPRYSA2=s6410CAM->CIPRYSA1+size;
        s6410CAM->CIPRYSA3=s6410CAM->CIPRYSA2+size;
        s6410CAM->CIPRYSA4=s6410CAM->CIPRYSA3+size;
        if(OUTPUT_CODEC_YCBCR420 == Preview_Buffer.Format)
        {
            sizeY = Preview_Buffer.Width*Preview_Buffer.Height;
            sizeC = Preview_Buffer.Width*Preview_Buffer.Height/4;

            // In DSHOW  CB and Cr are swapped.
            s6410CAM->CIPRCRSA1=s6410CAM->CIPRYSA1+sizeY;
            s6410CAM->CIPRCRSA2=s6410CAM->CIPRYSA2+sizeY;
            s6410CAM->CIPRCRSA3=s6410CAM->CIPRYSA3+sizeY;
            s6410CAM->CIPRCRSA4=s6410CAM->CIPRYSA4+sizeY;

            s6410CAM->CIPRCBSA1=s6410CAM->CIPRCRSA1+sizeC;
            s6410CAM->CIPRCBSA2=s6410CAM->CIPRCRSA2+sizeC;
            s6410CAM->CIPRCBSA3=s6410CAM->CIPRCRSA3+sizeC;
            s6410CAM->CIPRCBSA4=s6410CAM->CIPRCRSA4+sizeC;
        }
        CameraSetPreviewRegister(Preview_Buffer.Width, Preview_Buffer.Height, Preview_Buffer.Format);
    }
    else
    {
        return FALSE;
    }

    RETAILMSG(CAM_INOUT,(TEXT("------------------CameraSetRegisters\n")));
    return TRUE;
}

BOOL InitializeBuffer()
{
    DMA_ADAPTER_OBJECT Adapter1, Adapter2;

    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++InitializeBuffer\n")));
    memset(&Adapter1, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter1.InterfaceType = Internal;
    Adapter1.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    memset(&Adapter2, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter2.InterfaceType = Internal;
    Adapter2.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    pCodecVirtAddr  = (PBYTE)HalAllocateCommonBuffer(&Adapter1, CAPTURE_BUFFER_SIZE, &PhysCodecAddr, FALSE);
    if (pCodecVirtAddr  == NULL)
    {
        RETAILMSG(CAM_ERR, (TEXT("CameraPrepareBuffer() - Failed to allocate DMA buffer for CODEC Path.\r\n")));
        return FALSE;
    }

    pPreviewVirtAddr  = (PBYTE)HalAllocateCommonBuffer(&Adapter2, PREVIEW_BUFFER_SIZE, &PhysPreviewAddr, FALSE);
    if (pPreviewVirtAddr  == NULL)
    {
        RETAILMSG(CAM_ERR, (TEXT("CameraPrepareBuffer() - Failed to allocate DMA buffer for Preview Path.\r\n")));
        return FALSE;
    }

    RETAILMSG(CAM_INOUT,(TEXT("------------------InitializeBuffer\n")));
    return TRUE;
}

BOOL DeinitializeBuffer()
{
    DMA_ADAPTER_OBJECT Adapter1,Adapter2;
    RETAILMSG(CAM_INOUT,(TEXT("++++++++++++++++++DeinitializeBuffer\n")));
    memset(&Adapter1, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter1.InterfaceType = Internal;
    Adapter1.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    memset(&Adapter2, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter2.InterfaceType = Internal;
    Adapter2.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);

    if(pCodecVirtAddr != NULL)
    {
        HalFreeCommonBuffer(&Adapter1, 0, PhysCodecAddr, (PVOID)pCodecVirtAddr, FALSE);
        pCodecVirtAddr = NULL;
    }

    if(pPreviewVirtAddr != NULL)
    {
        HalFreeCommonBuffer(&Adapter2, 0, PhysPreviewAddr, (PVOID)pPreviewVirtAddr, FALSE);
        pPreviewVirtAddr = NULL;
    }
    RETAILMSG(CAM_INOUT,(TEXT("-------------------DeinitializeBuffer\n")));
    return TRUE;
}

BOOL InterruptInitialize()
{
    DWORD   threadID;                         // thread ID
    BOOL    bSuccess;

    RETAILMSG(CAM_INOUT,(TEXT("+++++++++++++++++++InterruptInitialize\n")));

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &g_uCamIrqForCapture, sizeof(UINT32), &g_uCamSysIntrForCapture, sizeof(UINT32), NULL))
    {
        RETAILMSG(CAM_ERR, (TEXT("ERROR: Failed to request sysintr value for Camera interrupt.\r\n")));
        return FALSE;
    }

    hCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!hCaptureEvent)
    {
        RETAILMSG(CAM_ERR,(TEXT("Fail to create camera interrupt event\r\n")));
        return FALSE;
    }

    bSuccess = InterruptInitialize(g_uCamSysIntrForCapture, hCaptureEvent, NULL, 0);
    if (!bSuccess)
    {
        RETAILMSG(CAM_ERR,(TEXT("Fail to initialize camera interrupt event\r\n")));
        return FALSE;
    }

    hCaptureThread = CreateThread(NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE)CameraCaptureThread,
                                 0,
                                 0,
                                 &threadID);

    if (NULL == hCaptureThread ) {
        RETAILMSG(CAM_ERR,(TEXT("Create Camera Thread Fail\r\n")));
        return FALSE;
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////

    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &g_uCamIrqForPreview, sizeof(UINT32), &g_uCamSysIntrForPreview, sizeof(UINT32), NULL))
    {
        RETAILMSG(CAM_ERR, (TEXT("ERROR: Failed to request sysintr value for Camera interrupt.\r\n")));
        return FALSE;
    }

    hPreviewEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!hPreviewEvent)
    {
        RETAILMSG(CAM_ERR,(TEXT("Fail to create camera interrupt event\r\n")));
        return FALSE;
    }

    bSuccess = InterruptInitialize(g_uCamSysIntrForPreview, hPreviewEvent, NULL, 0);
    if (!bSuccess)
    {
        RETAILMSG(CAM_ERR,(TEXT("Fail to initialize camera interrupt event\r\n")));
        return FALSE;
    }

    hPreviewThread = CreateThread(NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE)CameraPreviewThread,
                                 0,
                                 0,
                                 &threadID);

    if (NULL == hPreviewThread ) {
        RETAILMSG(CAM_ERR,(TEXT("Create Camera Thread Fail\r\n")));
        return FALSE;
    }

    RETAILMSG(CAM_INOUT,(TEXT("-------------------InterruptInitialize  %d  %d\n"),g_uCamSysIntrForCapture, g_uCamSysIntrForPreview));
    return TRUE;
}

DWORD WINAPI CameraCaptureThread(void)
{
    DWORD    dwCause;

    while(!bCaptureThreadExit)
    {
        dwCause = WaitForSingleObject(hCaptureEvent, INFINITE);
        if(bCaptureThreadExit)
        {
            break;
        }

        if (dwCause == WAIT_OBJECT_0)
        {
            if(CodecFrameCnt >= 2)
            {
                CodecFrameCnt = 3;
                if(StillOn)
                {
                    // Stop Capture
                    s6410CAM->CICOSCCTRL &= ~(CAM_CODEC_SACLER_START_BIT);
                    s6410CAM->CIIMGCPT &= ~((CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT));

                    while((s6410CAM->CICOSTATUS & (1<<21)) == 1) // check Codec path disable
                    {
                        ;
                    }


                    pfnCameraHandleStillFrame(dwCameraDriverContext);

                    // Restart Capture
                    s6410CAM->CICOSCCTRL |=(CAM_CODEC_SACLER_START_BIT);
                    s6410CAM->CIIMGCPT |=(CAM_CAMIF_GLOBAL_CAPTURE_ENABLE_BIT)|(CAM_CODEC_SCALER_CAPTURE_ENABLE_BIT);
                }
                else if(VideoOn)
                {
                    pfnCameraHandleVideoFrame(dwCameraDriverContext);
                }
            }
            else
            {
                CodecFrameCnt++;
            }
            s6410CAM->CIGCTRL |= (1<<19);
            InterruptDone(g_uCamSysIntrForCapture);
        }
        else
        {
            RETAILMSG(CAM_ERR, (TEXT("[CAM_HW] InterruptThread : Exit %d, Cause %d\r\n"), GetLastError(), dwCause));
        }
    }

    return 0;
}

DWORD WINAPI CameraPreviewThread(void)
{
    DWORD    dwCause;

    while(!bPreviewThreadExit)
    {

        dwCause = WaitForSingleObject(hPreviewEvent, INFINITE);
        if(bPreviewThreadExit)
        {
            break;
        }

        if (dwCause == WAIT_OBJECT_0)
        {

            if(PreviewFrameCnt >= 2)
            {
                PreviewFrameCnt = 3;
                pfnCameraHandlePreviewFrame(dwCameraDriverContext);
            }
            else
            {
                PreviewFrameCnt ++;
            }
            s6410CAM->CIGCTRL |= (1<<18);
            InterruptDone(g_uCamSysIntrForPreview);
        }
        else
        {
            RETAILMSG(CAM_ERR, (TEXT("[CAM_HW] InterruptThread : Exit %d, Cause %d\r\n"), GetLastError(), dwCause));
        }
    }

    return 0;
}

void CameraSetClockDiv()
{
    MODULE_DESCRIPTOR value;
    int    div;
    
    ModuleGetFormat(value);
    div = (int)(CAM_CLK_SOURCE / (float)value.Clock + 0.5) - 1;
    
    RETAILMSG(CAM_MSG,(TEXT("[CAM]  CameraSetClockDiv = %d\n"),div));
    s6410PWR->CLK_DIV0 = (s6410PWR->CLK_DIV0 & ~(0xf<<20))  | ((div & 0xf)<< 20); // CAMCLK is divided..    
}

void    CameraSleep()
{
}

void    CameraResume()
{
    RETAILMSG(CAM_INOUT,(TEXT("+++++++++++++++ [CAM] CameraResume\n")));

    // 1. Camera IO setup
    CameraGpioInit();

    // 2. Camera Clock setup
    // Reserved Step

    // 3. camera module reset
    CameraModuleReset();

    // 4. Write Setting for Module using I2C
    ModuleWriteBlock();

    // 5. Camera i/f reset
    CameraInterfaceReset();

    // 6. Initialize I/F source register
    CameraCaptureSourceSet();

    // 7. Camera Clock Off
    // Reserved Step
    
    RETAILMSG(CAM_INOUT,(TEXT("---------------- [CAM] CameraResume\n")));

}


int        CameraZoom(int value)
{
    MODULE_DESCRIPTOR moduleValue;
    UINT32 offsetValueWidth,offsetValueHeight;
    
    ModuleGetFormat(moduleValue);
    offsetValueWidth = value * CAM_OFFSET_STEP;
    offsetValueHeight = (int)(offsetValueWidth * (moduleValue.SourceVSize/(float)moduleValue.SourceHSize));
    offsetValueHeight = (offsetValueHeight<<1)>>1;
    
    if( offsetValueWidth*2 > (moduleValue.SourceHSize-16) || offsetValueHeight*2 > (moduleValue.SourceVSize-8) )
    {
        return FALSE;
    }
    
    g_iHorOffset1 = offsetValueWidth;
    g_iVerOffset1 = offsetValueHeight;
    g_iHorOffset2 = offsetValueWidth;
    g_iVerOffset2 = offsetValueHeight;    
    RETAILMSG(CAM_MSG,(TEXT("[CAM] offsetValueWidth=%d   offsetValueHeight=%d\n"),offsetValueWidth,offsetValueHeight));
    
    CameraSetScaler(Preview_Buffer.Width, Preview_Buffer.Height, PREVIEW_PATH);
    CameraSetScaler(Video_Buffer.Width, Video_Buffer.Height, CODEC_PATH);
    
    return TRUE;
}


void     CameraSetScaler(UINT32 width, UINT32 height, int path)
{
    UINT32 H_Shift, V_Shift, PreHorRatio, PreVerRatio, MainHorRatio, MainVerRatio;
    UINT32 ScaleUp_H, ScaleUp_V, SrcWidth, SrcHeight, WinOfsEn;    
    MODULE_DESCRIPTOR moduleValue;
    
    RETAILMSG(CAM_INOUT,(TEXT("+++++++++++++++++++CameraSetScaler\n")));
    
    if(width == 0 || height == 0) return;
    
    ModuleGetFormat(moduleValue);
            
    SrcWidth=moduleValue.SourceHSize-moduleValue.SourceHOffset*2-g_iHorOffset1-g_iHorOffset2;
    SrcHeight=moduleValue.SourceVSize-moduleValue.SourceVOffset*2-g_iVerOffset1-g_iVerOffset2;    
    
    RETAILMSG(CAM_MSG,(TEXT("[CAM] width=%d   height=%d   SrcWidth=%d  SrcHeight=%d\n"),width,height,SrcWidth,SrcHeight));
    
    if((moduleValue.SourceHSize > SrcWidth) || (moduleValue.SourceVSize > SrcHeight))
    {
        WinOfsEn = 1;
    }
    else
    {
        WinOfsEn = 0;
    }
    s6410CAM->CIWDOFST = (WinOfsEn<<31)|((moduleValue.SourceHOffset+g_iHorOffset1) <<16)|(moduleValue.SourceVOffset+g_iVerOffset1);         
    s6410CAM->CIDOWSFT2 = ((moduleValue.SourceHOffset+g_iHorOffset2) <<16)|(moduleValue.SourceVOffset+g_iVerOffset2);            
    
    CalculatePrescalerRatioShift(SrcWidth, width, &PreHorRatio, &H_Shift);
    CalculatePrescalerRatioShift(SrcHeight, height, &PreVerRatio, &V_Shift);
    MainHorRatio=(SrcWidth<<8)/(width<<H_Shift);
    MainVerRatio=(SrcHeight<<8)/(height<<V_Shift);
        
    if(SrcWidth>=width) ScaleUp_H=0; //down
    else ScaleUp_H=1;        //up

    if(SrcHeight>=height) ScaleUp_V=0;
    else ScaleUp_V=1;
            
    switch(path)
    {
    case PREVIEW_PATH:
        s6410CAM->CIPRSCPRERATIO=((10-H_Shift-V_Shift)<<28)|(PreHorRatio<<16)|(PreVerRatio);         
        s6410CAM->CIPRSCPREDST=((SrcWidth/PreHorRatio)<<16)|(SrcHeight/PreVerRatio); 
        s6410CAM->CIPRSCCTRL = (s6410CAM->CIPRSCCTRL & ~((0x1<<31)|(0x1<<30)|(0x1<<29)|(0x1ff<<16)|(0x1ff<<0)))
                                |(0<<31)|(ScaleUp_H<<30)|(ScaleUp_V<<29)|(MainHorRatio<<16)|(MainVerRatio);
        break;
    case CODEC_PATH:
        s6410CAM->CICOSCPRERATIO=((10-H_Shift-V_Shift)<<28)|(PreHorRatio<<16)|(PreVerRatio);
        s6410CAM->CICOSCPREDST=((SrcWidth/PreHorRatio)<<16)|(SrcHeight/PreVerRatio); 
        s6410CAM->CICOSCCTRL=(s6410CAM->CICOSCCTRL & ~((0x1<<31)|(0x1<<30)|(0x1<<29)|(0x1ff<<16)|(0x1ff<<0)))
                                |(0<<31)|(ScaleUp_H<<30)|(ScaleUp_V<<29)|(MainHorRatio<<16)|(MainVerRatio);    
        break;        
    }
    
    RETAILMSG(CAM_INOUT,(TEXT("-------------------CameraSetScaler\n")));
}
