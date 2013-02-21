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

Module Name:    camera_pdd.cpp

Abstract:       Handle Camera device in High level abstract

Functions:


Notes:


--*/


#include <windows.h>
#include <pm.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"

#include "s3c6410_camera.h"

#include <camera.h>
#include "CameraDriver.h"
#include "SensorFormats.h"
#include "SensorProperties.h"
#include "PinDriver.h"

#include "CameraPDD.h"
#include "camera_pdd.h"
#include "wchar.h"



#define MSG_ERROR       1
#define MSG_INFO        0
#define MSG_INOUT       0
#define MSG_ON          0

//mio
#define CAPTURE_FORMATS_COUNT   5
#define STILLSHOT_FORMATS_COUNT 2
#define PREVIEW_FORMATS_COUNT   4

// Pointer to camera driver instance which we will send back with callback functions
extern DWORD dwCameraDriverContext;

// Signals the application that the video or still image frame is available
extern PFNCAMHANDLEFRAME pfnCameraHandleVideoFrame;
extern PFNCAMHANDLEFRAME pfnCameraHandleStillFrame;
extern PFNCAMHANDLEFRAME pfnCameraHandlePreviewFrame;

PDDFUNCTBL FuncTbl = {
    sizeof(PDDFUNCTBL),
    PDD_Init,
    PDD_DeInit,
    PDD_GetAdapterInfo,
    PDD_HandleVidProcAmpChanges,
    PDD_HandleCamControlChanges,
    PDD_HandleVideoControlCapsChanges,
    PDD_SetPowerState,
    PDD_HandleAdapterCustomProperties,
    PDD_InitSensorMode,
    PDD_DeInitSensorMode,
    PDD_SetSensorState,
    PDD_TakeStillPicture,
    PDD_GetSensorModeInfo,
    PDD_SetSensorModeFormat,
    PDD_AllocateBuffer,
    PDD_DeAllocateBuffer,
    PDD_RegisterClientBuffer,
    PDD_UnRegisterClientBuffer,
    PDD_FillBuffer,
    PDD_HandleModeCustomProperties
};

const POWER_CAPABILITIES s_PowerCaps = 
{
    // DeviceDx:    Supported power states
    DX_MASK(D0 ) | DX_MASK(D4),

    0,              // WakeFromDx:
    0,              // InrushDx:    No inrush of power

    {               // Power: Maximum milliwatts in each state
        0x00000001, //        D0 = 0
        0x00000001, //        D1 = 0
        0x00000001, //        D2 = 0
        0x00000001, //        D3 = 0
        0x00000001  //        D4 = 0 (off)
    },

    {               // Latency
        0x00000000, //        D0 = 0
        0x00000000, //        D1 = 0
        0x00000000, //        D2 = 0
        0x00000000, //        D3 = 0
        0x00000000  //        D4 = 0
    },

    0,                    // Flags: None
};

CCameraPdd::CCameraPdd()
{
    m_ulCTypes = 2;
    m_bStillCapInProgress = false;
    m_hContext = NULL;
    m_pModeVideoFormat = NULL;
    m_pModeVideoCaps = NULL;
    m_ppModeContext = NULL;
    m_pCurrentFormat = NULL;
    m_iPinUseCount     = 0;
    m_PowerState = D0;    
    m_bCameraPreviewRunning = FALSE;
    m_bCameraVideoRunning = FALSE;
    m_bCameraPreviewWasRunning = FALSE;
    m_bCameraVideoWasRunning = FALSE;    
    
    InitializeCriticalSection( &m_csPddDevice );

    memset( &m_CsState, 0x0, sizeof(m_CsState));
    memset( &m_SensorModeInfo, 0x0, sizeof(m_SensorModeInfo));
    memset( &m_SensorProps, 0x0, sizeof(m_SensorProps));
    memset( &PowerCaps, 0x0, sizeof(PowerCaps));

}

CCameraPdd::~CCameraPdd()
{

    DeleteCriticalSection( &m_csPddDevice );

    if( NULL != m_pModeVideoCaps )
    {
        delete [] m_pModeVideoCaps;
        m_pModeVideoCaps = NULL;
    }

    if( NULL != m_pCurrentFormat )
    {
        delete [] m_pCurrentFormat;
        m_pCurrentFormat = NULL;
    }

    if( NULL != m_pModeVideoFormat )
    {
        delete [] m_pModeVideoFormat;
        m_pModeVideoFormat = NULL;
    }

    if( NULL != m_ppModeContext )
    {
        delete [] m_ppModeContext;
        m_ppModeContext = NULL;
    }

}

DWORD CCameraPdd::PDDInit( PVOID MDDContext, PPDDFUNCTBL pPDDFuncTbl )
{
    m_hContext = (HANDLE)MDDContext;
    // Real drivers may want to create their context

        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++PDDInit\n")));
    if (!CameraInit(MDDContext))
    {
        DEBUGMSG( ZONE_ERROR, ( _T("PddInit:FAILED to Initialize camera\r\n") ) );
        return ERROR_GEN_FAILURE;
    }

    dwCameraDriverContext = reinterpret_cast<DWORD>( this );
    // This callback function is used to signal the application that the video frame is now available.
    //mio
    //pfnCameraHandleVideoFrame = CameraVideoFrameCallback;
    pfnCameraHandleVideoFrame = CCameraPdd::CameraVideoFrameCallback;
    // This callback function is used to signal the application that the still image is now available.
    //mio
    //pfnCameraHandleStillFrame = CameraStillFrameCallback;
    pfnCameraHandleStillFrame = CCameraPdd::CameraStillFrameCallback;
    // This callback function is used to signal the application that the Preview image is now available.
    //mio
    //pfnCameraHandlePreviewFrame = CameraPreviewFrameCallback;
    pfnCameraHandlePreviewFrame = CCameraPdd::CameraPreviewFrameCallback;


    m_ulCTypes = 3; // Default number of Sensor Modes is 2

    // Read registry to override the default number of Sensor Modes.
    ReadMemoryModelFromRegistry();

    if( pPDDFuncTbl->dwSize  > sizeof( PDDFUNCTBL ) )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy( pPDDFuncTbl, &FuncTbl, sizeof( PDDFUNCTBL ) );

    memset( m_SensorProps, 0x0, sizeof(m_SensorProps) );

    memcpy( &PowerCaps, &s_PowerCaps, sizeof( POWER_CAPABILITIES ) );

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Set all VideoProcAmp and CameraControl properties.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //VideoProcAmp
    m_SensorProps[ENUM_BRIGHTNESS].ulCurrentValue     = BrightnessDefault;
    m_SensorProps[ENUM_BRIGHTNESS].ulDefaultValue     = BrightnessDefault;
    m_SensorProps[ENUM_BRIGHTNESS].pRangeNStep        = &BrightnessRangeAndStep[0];
    m_SensorProps[ENUM_BRIGHTNESS].ulFlags            = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    m_SensorProps[ENUM_BRIGHTNESS].ulCapabilities     = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_BRIGHTNESS].fSetSupported      = VideoProcAmpProperties[ENUM_BRIGHTNESS].SetSupported;
    m_SensorProps[ENUM_BRIGHTNESS].fGetSupported      = VideoProcAmpProperties[ENUM_BRIGHTNESS].GetSupported;
    m_SensorProps[ENUM_BRIGHTNESS].pCsPropValues      = &BrightnessValues;

    m_SensorProps[ENUM_CONTRAST].ulCurrentValue       = ContrastDefault;
    m_SensorProps[ENUM_CONTRAST].ulDefaultValue       = ContrastDefault;
    m_SensorProps[ENUM_CONTRAST].pRangeNStep          = &ContrastRangeAndStep[0];
    m_SensorProps[ENUM_CONTRAST].ulFlags              = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    m_SensorProps[ENUM_CONTRAST].ulCapabilities       = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_CONTRAST].fSetSupported        = VideoProcAmpProperties[ENUM_CONTRAST].SetSupported;
    m_SensorProps[ENUM_CONTRAST].fGetSupported        = VideoProcAmpProperties[ENUM_CONTRAST].GetSupported;
    m_SensorProps[ENUM_CONTRAST].pCsPropValues        = &ContrastValues;

    m_SensorProps[ENUM_HUE].ulCurrentValue            = HueDefault;
    m_SensorProps[ENUM_HUE].ulDefaultValue            = HueDefault;
    m_SensorProps[ENUM_HUE].pRangeNStep               = &HueRangeAndStep[0];
    m_SensorProps[ENUM_HUE].ulFlags                   = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_HUE].ulCapabilities            = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_HUE].fSetSupported             = VideoProcAmpProperties[ENUM_HUE].SetSupported;
    m_SensorProps[ENUM_HUE].fGetSupported             = VideoProcAmpProperties[ENUM_HUE].GetSupported;
    m_SensorProps[ENUM_HUE].pCsPropValues             = &HueValues;

    m_SensorProps[ENUM_SATURATION].ulCurrentValue     = SaturationDefault;
    m_SensorProps[ENUM_SATURATION].ulDefaultValue     = SaturationDefault;
    m_SensorProps[ENUM_SATURATION].pRangeNStep        = &SaturationRangeAndStep[0];
    m_SensorProps[ENUM_SATURATION].ulFlags            = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SATURATION].ulCapabilities     = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SATURATION].fSetSupported      = VideoProcAmpProperties[ENUM_SATURATION].SetSupported;
    m_SensorProps[ENUM_SATURATION].fGetSupported      = VideoProcAmpProperties[ENUM_SATURATION].GetSupported;
    m_SensorProps[ENUM_SATURATION].pCsPropValues      = &SaturationValues;

    m_SensorProps[ENUM_SHARPNESS].ulCurrentValue      = SharpnessDefault;
    m_SensorProps[ENUM_SHARPNESS].ulDefaultValue      = SharpnessDefault;
    m_SensorProps[ENUM_SHARPNESS].pRangeNStep         = &SharpnessRangeAndStep[0];
    m_SensorProps[ENUM_SHARPNESS].ulFlags             = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SHARPNESS].ulCapabilities      = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_SHARPNESS].fSetSupported       = VideoProcAmpProperties[ENUM_SHARPNESS].SetSupported;
    m_SensorProps[ENUM_SHARPNESS].fGetSupported       = VideoProcAmpProperties[ENUM_SHARPNESS].GetSupported;
    m_SensorProps[ENUM_SHARPNESS].pCsPropValues       = &SharpnessValues;

    m_SensorProps[ENUM_GAMMA].ulCurrentValue          = GammaDefault;
    m_SensorProps[ENUM_GAMMA].ulDefaultValue          = GammaDefault;
    m_SensorProps[ENUM_GAMMA].pRangeNStep             = &GammaRangeAndStep[0];
    m_SensorProps[ENUM_GAMMA].ulFlags                 = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAMMA].ulCapabilities          = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAMMA].fSetSupported           = VideoProcAmpProperties[ENUM_GAMMA].SetSupported;
    m_SensorProps[ENUM_GAMMA].fGetSupported           = VideoProcAmpProperties[ENUM_GAMMA].GetSupported;
    m_SensorProps[ENUM_GAMMA].pCsPropValues           = &GammaValues;

    m_SensorProps[ENUM_COLORENABLE].ulCurrentValue    = ColorEnableDefault;
    m_SensorProps[ENUM_COLORENABLE].ulDefaultValue    = ColorEnableDefault;
    m_SensorProps[ENUM_COLORENABLE].pRangeNStep       = &ColorEnableRangeAndStep[0];
    m_SensorProps[ENUM_COLORENABLE].ulFlags           = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_COLORENABLE].ulCapabilities    = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_COLORENABLE].fSetSupported     = VideoProcAmpProperties[ENUM_COLORENABLE].SetSupported;
    m_SensorProps[ENUM_COLORENABLE].fGetSupported     = VideoProcAmpProperties[ENUM_COLORENABLE].GetSupported;
    m_SensorProps[ENUM_COLORENABLE].pCsPropValues     = &ColorEnableValues;

    m_SensorProps[ENUM_WHITEBALANCE].ulCurrentValue   = WhiteBalanceDefault;
    m_SensorProps[ENUM_WHITEBALANCE].ulDefaultValue   = WhiteBalanceDefault;
    m_SensorProps[ENUM_WHITEBALANCE].pRangeNStep      = &WhiteBalanceRangeAndStep[0];
    m_SensorProps[ENUM_WHITEBALANCE].ulFlags          = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_WHITEBALANCE].ulCapabilities   = CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL|CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_WHITEBALANCE].fSetSupported    = VideoProcAmpProperties[ENUM_WHITEBALANCE].SetSupported;
    m_SensorProps[ENUM_WHITEBALANCE].fGetSupported    = VideoProcAmpProperties[ENUM_WHITEBALANCE].GetSupported;
    m_SensorProps[ENUM_WHITEBALANCE].pCsPropValues    = &WhiteBalanceValues;

    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulCurrentValue = BackLightCompensationDefault;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulDefaultValue = BackLightCompensationDefault;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].pRangeNStep    = &BackLightCompensationRangeAndStep[0];
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulFlags        = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].ulCapabilities = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].fSetSupported  = VideoProcAmpProperties[ENUM_BACKLIGHT_COMPENSATION].SetSupported;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].fGetSupported  = VideoProcAmpProperties[ENUM_BACKLIGHT_COMPENSATION].GetSupported;
    m_SensorProps[ENUM_BACKLIGHT_COMPENSATION].pCsPropValues  = &BackLightCompensationValues;

    m_SensorProps[ENUM_GAIN].ulCurrentValue           = GainDefault;
    m_SensorProps[ENUM_GAIN].ulDefaultValue           = GainDefault;
    m_SensorProps[ENUM_GAIN].pRangeNStep              = &GainRangeAndStep[0];
    m_SensorProps[ENUM_GAIN].ulFlags                  = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAIN].ulCapabilities           = CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
    m_SensorProps[ENUM_GAIN].fSetSupported            = VideoProcAmpProperties[ENUM_GAIN].SetSupported;
    m_SensorProps[ENUM_GAIN].fGetSupported            = VideoProcAmpProperties[ENUM_GAIN].GetSupported;
    m_SensorProps[ENUM_GAIN].pCsPropValues            = &GainValues;

    //CameraControl
    m_SensorProps[ENUM_PAN].ulCurrentValue            = PanDefault;
    m_SensorProps[ENUM_PAN].ulDefaultValue            = PanDefault;
    m_SensorProps[ENUM_PAN].pRangeNStep               = &PanRangeAndStep[0];
    m_SensorProps[ENUM_PAN].ulFlags                   = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_PAN].ulCapabilities            = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_PAN].fSetSupported             = VideoProcAmpProperties[ENUM_PAN-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_PAN].fGetSupported             = VideoProcAmpProperties[ENUM_PAN-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_PAN].pCsPropValues             = &PanValues;

    m_SensorProps[ENUM_TILT].ulCurrentValue           = TiltDefault;
    m_SensorProps[ENUM_TILT].ulDefaultValue           = TiltDefault;
    m_SensorProps[ENUM_TILT].pRangeNStep              = &TiltRangeAndStep[0];
    m_SensorProps[ENUM_TILT].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_TILT].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_TILT].fSetSupported            = VideoProcAmpProperties[ENUM_TILT-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_TILT].fGetSupported            = VideoProcAmpProperties[ENUM_TILT-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_TILT].pCsPropValues            = &TiltValues;

    m_SensorProps[ENUM_ROLL].ulCurrentValue           = RollDefault;
    m_SensorProps[ENUM_ROLL].ulDefaultValue           = RollDefault;
    m_SensorProps[ENUM_ROLL].pRangeNStep              = &RollRangeAndStep[0];
    m_SensorProps[ENUM_ROLL].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ROLL].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ROLL].fSetSupported            = VideoProcAmpProperties[ENUM_ROLL-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_ROLL].fGetSupported            = VideoProcAmpProperties[ENUM_ROLL-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_ROLL].pCsPropValues            = &RollValues;

    m_SensorProps[ENUM_ZOOM].ulCurrentValue           = ZoomDefault;
    m_SensorProps[ENUM_ZOOM].ulDefaultValue           = ZoomDefault;
    m_SensorProps[ENUM_ZOOM].pRangeNStep              = &ZoomRangeAndStep[0];
    m_SensorProps[ENUM_ZOOM].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ZOOM].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_ZOOM].fSetSupported            = VideoProcAmpProperties[ENUM_ZOOM-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_ZOOM].fGetSupported            = VideoProcAmpProperties[ENUM_ZOOM-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_ZOOM].pCsPropValues            = &ZoomValues;

    m_SensorProps[ENUM_IRIS].ulCurrentValue           = IrisDefault;
    m_SensorProps[ENUM_IRIS].ulDefaultValue           = IrisDefault;
    m_SensorProps[ENUM_IRIS].pRangeNStep              = &IrisRangeAndStep[0];
    m_SensorProps[ENUM_IRIS].ulFlags                  = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_IRIS].ulCapabilities           = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_IRIS].fSetSupported            = VideoProcAmpProperties[ENUM_IRIS-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_IRIS].fGetSupported            = VideoProcAmpProperties[ENUM_IRIS-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_IRIS].pCsPropValues            = &IrisValues;

    m_SensorProps[ENUM_EXPOSURE].ulCurrentValue       = ExposureDefault;
    m_SensorProps[ENUM_EXPOSURE].ulDefaultValue       = ExposureDefault;
    m_SensorProps[ENUM_EXPOSURE].pRangeNStep          = &ExposureRangeAndStep[0];
    m_SensorProps[ENUM_EXPOSURE].ulFlags              = CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_EXPOSURE].ulCapabilities       = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL|CSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    m_SensorProps[ENUM_EXPOSURE].fSetSupported        = VideoProcAmpProperties[ENUM_EXPOSURE-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_EXPOSURE].fGetSupported        = VideoProcAmpProperties[ENUM_EXPOSURE-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_EXPOSURE].pCsPropValues        = &ExposureValues;

    m_SensorProps[ENUM_FOCUS].ulCurrentValue          = FocusDefault;
    m_SensorProps[ENUM_FOCUS].ulDefaultValue          = FocusDefault;
    m_SensorProps[ENUM_FOCUS].pRangeNStep             = &FocusRangeAndStep[0];
    m_SensorProps[ENUM_FOCUS].ulFlags                 = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FOCUS].ulCapabilities          = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FOCUS].fSetSupported           = VideoProcAmpProperties[ENUM_FOCUS-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_FOCUS].fGetSupported           = VideoProcAmpProperties[ENUM_FOCUS-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_FOCUS].pCsPropValues           = &FocusValues;

    m_SensorProps[ENUM_FLASH].ulCurrentValue          = FlashDefault;
    m_SensorProps[ENUM_FLASH].ulDefaultValue          = FlashDefault;
    m_SensorProps[ENUM_FLASH].pRangeNStep             = &FlashRangeAndStep[0];
    m_SensorProps[ENUM_FLASH].ulFlags                 = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FLASH].ulCapabilities          = CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    m_SensorProps[ENUM_FLASH].fSetSupported           = VideoProcAmpProperties[ENUM_FLASH-NUM_VIDEOPROCAMP_ITEMS].SetSupported;
    m_SensorProps[ENUM_FLASH].fGetSupported           = VideoProcAmpProperties[ENUM_FLASH-NUM_VIDEOPROCAMP_ITEMS].GetSupported;
    m_SensorProps[ENUM_FLASH].pCsPropValues           = &FlashValues;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    m_pModeVideoFormat = NULL;
    // Allocate Video Format specific array.
    m_pModeVideoFormat = new PINVIDEOFORMAT[m_ulCTypes];
    if( NULL == m_pModeVideoFormat )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // Video Format initialization
    m_pModeVideoFormat[CAPTURE].categoryGUID         = PINNAME_VIDEO_CAPTURE;
    //mio
    m_pModeVideoFormat[CAPTURE].ulAvailFormats       = CAPTURE_FORMATS_COUNT;
    m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo = new PCS_DATARANGE_VIDEO[m_pModeVideoFormat[CAPTURE].ulAvailFormats];

    if( NULL == m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo[0] = &DCAM_StreamMode_5;
    m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo[1] = &DCAM_StreamMode_6;
    m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo[2] = &DCAM_StreamMode_7;
    m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo[3] = &DCAM_StreamMode_8;
    m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo[4] = &DCAM_StreamMode_9;

    m_pModeVideoFormat[STILL].categoryGUID           = PINNAME_VIDEO_STILL;
    //mio
    m_pModeVideoFormat[STILL].ulAvailFormats         = STILLSHOT_FORMATS_COUNT;
    m_pModeVideoFormat[STILL].pCsDataRangeVideo = new PCS_DATARANGE_VIDEO[m_pModeVideoFormat[STILL].ulAvailFormats];

    if( NULL == m_pModeVideoFormat[STILL].pCsDataRangeVideo )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    m_pModeVideoFormat[STILL].pCsDataRangeVideo[0]   = &DCAM_StreamMode_6;
    m_pModeVideoFormat[STILL].pCsDataRangeVideo[1]   = &DCAM_StreamMode_8;

    if( 3 == m_ulCTypes )
    {
        m_pModeVideoFormat[PREVIEW].categoryGUID         = PINNAME_VIDEO_PREVIEW;
        //mio
        m_pModeVideoFormat[PREVIEW].ulAvailFormats       = PREVIEW_FORMATS_COUNT;
        m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo = new PCS_DATARANGE_VIDEO[m_pModeVideoFormat[PREVIEW].ulAvailFormats];

        if( NULL == m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo )
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo[0] = &DCAM_StreamMode_0;
        m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo[1] = &DCAM_StreamMode_1;
        m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo[2] = &DCAM_StreamMode_2;
        m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo[3] = &DCAM_StreamMode_3;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    m_pModeVideoCaps = NULL;
    // Allocate Video Control Caps specific array.
    m_pModeVideoCaps = new VIDCONTROLCAPS[m_ulCTypes];
    if( NULL == m_pModeVideoCaps )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    // Video Control Caps

    m_pModeVideoCaps[CAPTURE].DefaultVideoControlCaps     = DefaultVideoControlCaps[CAPTURE];
    m_pModeVideoCaps[CAPTURE].CurrentVideoControlCaps     = DefaultVideoControlCaps[CAPTURE];;
    m_pModeVideoCaps[STILL].DefaultVideoControlCaps       = DefaultVideoControlCaps[STILL];
    m_pModeVideoCaps[STILL].CurrentVideoControlCaps       = DefaultVideoControlCaps[STILL];;
    if( 3 == m_ulCTypes )
    {
        // Note PREVIEW control caps are the same, so we don't differentiate
        m_pModeVideoCaps[PREVIEW].DefaultVideoControlCaps     = DefaultVideoControlCaps[PREVIEW];
        m_pModeVideoCaps[PREVIEW].CurrentVideoControlCaps     = DefaultVideoControlCaps[PREVIEW];;
    }

   // m_SensorModeInfo[CAPTURE].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
    m_SensorModeInfo[CAPTURE].MaxNumOfBuffers = 1;
    m_SensorModeInfo[CAPTURE].PossibleCount = 1;
  //  m_SensorModeInfo[STILL].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
    m_SensorModeInfo[STILL].MaxNumOfBuffers = 1;
    m_SensorModeInfo[STILL].PossibleCount = 1;
    if( 3 == m_ulCTypes )
    {
   //     m_SensorModeInfo[PREVIEW].MemoryModel = CSPROPERTY_BUFFER_CLIENT_UNLIMITED;
        m_SensorModeInfo[PREVIEW].MaxNumOfBuffers = 1;
        m_SensorModeInfo[PREVIEW].PossibleCount = 1;
    }

    m_ppModeContext = new LPVOID[m_ulCTypes];
    if ( NULL == m_ppModeContext )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    m_pCurrentFormat = new CS_DATARANGE_VIDEO[m_ulCTypes];
    if( NULL == m_pCurrentFormat )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    return ERROR_SUCCESS;
}


void CCameraPdd::PDD_DeInit()
{
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++PDD_DeInit\n")));
    CameraDeinit();

    if( NULL != m_ppModeContext )
    {
        delete [] m_ppModeContext;
        m_ppModeContext = NULL;
    }

    if( NULL != m_pModeVideoCaps )
    {
        delete [] m_pModeVideoCaps;
        m_pModeVideoCaps = NULL;
    }

    if( NULL != m_pModeVideoFormat )
    {
        if (NULL != m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo)
        {
            delete [] m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo;
            m_pModeVideoFormat[CAPTURE].pCsDataRangeVideo = NULL;
        }

        if (NULL != m_pModeVideoFormat[STILL].pCsDataRangeVideo)
        {
            delete [] m_pModeVideoFormat[STILL].pCsDataRangeVideo;
            m_pModeVideoFormat[STILL].pCsDataRangeVideo = NULL;
        }

        if (NULL != m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo)
        {
            delete [] m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo;
            m_pModeVideoFormat[PREVIEW].pCsDataRangeVideo = NULL;
        }

        delete [] m_pModeVideoFormat;
        m_pModeVideoFormat = NULL;
    }
    RETAILMSG(MSG_INOUT,(TEXT("---------------------PDD_DeInit\n")));
}


DWORD CCameraPdd::GetAdapterInfo( PADAPTERINFO pAdapterInfo )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++GetAdapterInfo\n")));
    pAdapterInfo->ulCTypes = m_ulCTypes;
    pAdapterInfo->PowerCaps = PowerCaps;
    pAdapterInfo->ulVersionID = DRIVER_VERSION_2; //Camera MDD and DShow support DRIVER_VERSION and DRIVER_VERSION_2. Defined in camera.h
    memcpy( &pAdapterInfo->SensorProps, &m_SensorProps, sizeof(m_SensorProps));

    return ERROR_SUCCESS;

}

DWORD CCameraPdd::HandleVidProcAmpChanges( DWORD dwPropId, LONG lFlags, LONG lValue )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++HandleVidProcAmpChanges\n")));
    PSENSOR_PROPERTY pDevProp = NULL;

    pDevProp = m_SensorProps + dwPropId;

    if( CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL == lFlags )
    {
        pDevProp->ulCurrentValue = lValue;
    }

    pDevProp->ulFlags = lFlags;
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::HandleCamControlChanges( DWORD dwPropId, LONG lFlags, LONG lValue )
{
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++HandleCamControlChanges\n")));
    PSENSOR_PROPERTY pDevProp = NULL;
    int    value;
    pDevProp = m_SensorProps + dwPropId;
    
    if( CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL == lFlags )
    {
        pDevProp->ulCurrentValue = lValue;
        switch(dwPropId)
        {
        case ENUM_ZOOM:
            RETAILMSG(MSG_INFO,(TEXT("[CAM] Set Zoom value = %d!!!\n"), pDevProp->ulCurrentValue));
            value = (int)((pDevProp->ulCurrentValue-ZoomRangeAndStep[0].Bounds.UnsignedMinimum)/ZoomRangeAndStep[0].SteppingDelta);
            RETAILMSG(MSG_INFO,(TEXT("[CAM] Zoom value=%ld\n"),value));
            if(!CameraZoom(value))
            {
                RETAILMSG(MSG_ERROR,(TEXT("[CAM_ERROR] Zoom value %ld is not supported\n"),value));
            }
            break;
        default:
            break;
        }        
    }

    pDevProp->ulFlags = lFlags;
    
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::HandleVideoControlCapsChanges( LONG lModeType ,ULONG ulCaps )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++HandleVideoControlCapsChanges\n")));
    m_pModeVideoCaps[lModeType].CurrentVideoControlCaps = ulCaps;
    return ERROR_SUCCESS;
}

DWORD CCameraPdd :: SetPowerState( CEDEVICE_POWER_STATE PowerState )
{
    DWORD dwErr = ERROR_SUCCESS;    
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++SetPowerState\n")));
    switch (PowerState)
    {
    case D0:  
        RETAILMSG(MSG_ON, (TEXT("[CAM]: D0\r\n")));

        if (m_PowerState != D0) 
            ResumeCamera();

        m_PowerState = D0;

        dwErr = ERROR_SUCCESS;
        break;

    case D4:
        RETAILMSG(MSG_ON, (TEXT("[CAM]: D4\r\n")));
        if (m_PowerState != D4) 
            SuspendCamera();
           m_PowerState = D4;
        dwErr = ERROR_SUCCESS;
        break;

    default:
        break;
    }
    return dwErr;
}

DWORD CCameraPdd::HandleAdapterCustomProperties( PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++HandleAdapterCustomProperties\n")));
     DEBUGMSG( ZONE_IOCTL, ( _T("IOControl Adapter PDD: Unsupported PropertySet Request\r\n")) );
    return ERROR_NOT_SUPPORTED;
}

DWORD CCameraPdd::InitSensorMode( ULONG ulModeType, LPVOID ModeContext )
{
    DWORD hr = ERROR_SUCCESS;
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++InitSensorMode\n")));
    ASSERT( ModeContext );

    EnterCriticalSection( &m_csPddDevice );
    m_iPinUseCount += 1;
    if(m_iPinUseCount > MAX_SUPPORTED_PINS)
        m_iPinUseCount = MAX_SUPPORTED_PINS;
    //RETAILMSG(1,(TEXT("m_iPinUseCount=%d\r\n"),m_iPinUseCount));
    if(m_iPinUseCount == 1)
    {
        CameraClockOn(TRUE);
        CameraResume();
    }
    LeaveCriticalSection( &m_csPddDevice );
    m_ppModeContext[ulModeType] = ModeContext;

    return hr;
}

DWORD CCameraPdd::DeInitSensorMode( ULONG ulModeType )
{
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++DeInitSensorMode\n")));
    EnterCriticalSection( &m_csPddDevice );
    m_iPinUseCount -= 1;
    if(m_iPinUseCount < 0)
        m_iPinUseCount = 0;
    //RETAILMSG(1,(TEXT("m_iPinUseCount=%d\r\n"),m_iPinUseCount));
    if(m_iPinUseCount == 0)
    {
        CameraSleep();
        CameraClockOn(FALSE);
    }
    LeaveCriticalSection( &m_csPddDevice );
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::SetSensorState( ULONG lModeType, CSSTATE csState )
{
    DWORD dwError = ERROR_SUCCESS;
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++SetSensorState\n")));
    int format = (lModeType==CAPTURE)?VIDEO_CAPTURE_BUFFER:((lModeType==STILL)?STILL_CAPTURE_BUFFER:PREVIEW_CAPTURE_BUFFER);

    switch ( csState )
    {
        case CSSTATE_STOP:
            RETAILMSG(MSG_ON,(TEXT("[CAM] %d STOP\n"),format));
            m_CsState[lModeType] = CSSTATE_STOP;

            if( STILL == lModeType )
            {
                m_bStillCapInProgress = false;
            } 
            else if(CAPTURE == lModeType)
            {
                m_bCameraVideoRunning = false;
            } 
            else
            {
                m_bCameraPreviewRunning = false;
            }
            CameraCaptureControl(format, FALSE);
            break;

        case CSSTATE_PAUSE:
            RETAILMSG(MSG_ON,(TEXT("[CAM] %d PAUSE\n"),format));
            if(CAPTURE == lModeType)
            {
                m_bCameraVideoRunning = false;
            } 
            else
            {
                m_bCameraPreviewRunning = false;
            }            
            m_CsState[lModeType] = CSSTATE_PAUSE;
            //mio
            CameraCaptureControl(format, FALSE);
            break;

        case CSSTATE_RUN:
            RETAILMSG(MSG_ON,(TEXT("[CAM] %d RUN\n"),format));
            m_CsState[lModeType] = CSSTATE_RUN;

            if(CAPTURE == lModeType)
            {
                m_bCameraVideoRunning = true;
            } 
            else
            {
                m_bCameraPreviewRunning = true;
            }
            SetSensorFormat(lModeType);
            CameraSetRegisters(format);
                        //mio
            CameraCaptureControl(format,TRUE);
            break;

        default:
            DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("IOControl(%08x): Incorrect State\r\n"), this ) );
            dwError = ERROR_INVALID_PARAMETER;
    }

    return dwError;
}

DWORD CCameraPdd::TakeStillPicture( LPVOID pBurstModeInfo )
{
    DWORD dwError = ERROR_SUCCESS;
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++TakeStillPicture\n")));
    m_bStillCapInProgress = true;
    //Ignore pBurstModeInfo
    m_CsState[STILL] = CSSTATE_RUN;

    SetSensorFormat(STILL);
    CameraSetRegisters(STILL_CAPTURE_BUFFER);
    //mio
    CameraCaptureControl(STILL_CAPTURE_BUFFER,TRUE);
    return dwError;
}


DWORD CCameraPdd::GetSensorModeInfo( ULONG ulModeType, PSENSORMODEINFO pSensorModeInfo )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++GetSensorModeInfo\n")));
    pSensorModeInfo->MemoryModel = m_SensorModeInfo[ulModeType].MemoryModel;
    pSensorModeInfo->MaxNumOfBuffers = m_SensorModeInfo[ulModeType].MaxNumOfBuffers;
    pSensorModeInfo->PossibleCount = m_SensorModeInfo[ulModeType].PossibleCount;
    pSensorModeInfo->VideoCaps.DefaultVideoControlCaps = DefaultVideoControlCaps[ulModeType];
    pSensorModeInfo->VideoCaps.CurrentVideoControlCaps = m_pModeVideoCaps[ulModeType].CurrentVideoControlCaps;
    pSensorModeInfo->pVideoFormat = &m_pModeVideoFormat[ulModeType];
        RETAILMSG(MSG_INOUT,(TEXT("--------------------GetSensorModeInfo\n")));
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::SetSensorModeFormat( ULONG ulModeType, PCS_DATARANGE_VIDEO pCsDataRangeVideo )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++SetSensorModeFormat     %d\n"),ulModeType));
    // MDD will take care of ulModeType. It will never be out of range. MDD will also ask for
    // the format we support. We need not to check it here.

    memcpy( &m_pCurrentFormat[ulModeType], pCsDataRangeVideo, sizeof ( CS_DATARANGE_VIDEO ) );
    return ERROR_SUCCESS;
}

PVOID CCameraPdd::AllocateBuffer( ULONG ulModeType )
{
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++AllocateBuffer\n")));
    // Real PDD may want to save off this allocated pointer in an array.
    // In this PDD, we don't need the buffer for copy.
    return NULL;
}

DWORD CCameraPdd::DeAllocateBuffer( ULONG ulModeType, PVOID pBuffer )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++DeAllocateBuffer\n")));

    return ERROR_SUCCESS;
}

DWORD CCameraPdd::RegisterClientBuffer( ULONG ulModeType, PVOID pBuffer )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++RegisterClientBuffer\n")));
    // Real PDD may want to save pBuffer which is a pointer to buffer that DShow created.
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::UnRegisterClientBuffer( ULONG ulModeType, PVOID pBuffer )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++UnRegisterClientBuffer\n")));
    // DShow is not going to use pBuffer (which was originally allocated by DShow) anymore. If the PDD
    // is keeping a cached pBuffer pointer (in RegisterClientBuffer()) then this is the right place to
    // stop using it and maybe set the cached pointer to NULL.
    // Note: PDD must not delete this pointer as it will be deleted by DShow itself
    return ERROR_SUCCESS;
}

DWORD CCameraPdd::HandleSensorModeCustomProperties( ULONG ulModeType, PUCHAR pInBuf, DWORD  InBufLen, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++HandleSensorModeCustomProperties\n")));
    DEBUGMSG( ZONE_IOCTL, ( _T("IOControl: Unsupported PropertySet Request\r\n")) );
    return ERROR_NOT_SUPPORTED;
}

extern "C" { WINGDIAPI HBITMAP WINAPI CreateBitmapFromPointer( CONST BITMAPINFO *pbmi, int iStride, PVOID pvBits); }

DWORD CCameraPdd::FillBuffer( ULONG ulModeType, PUCHAR pImage )
{
    DWORD dwRet = 0;
    INT CurrentFrame;
    PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &m_pCurrentFormat[ulModeType].VideoInfoHeader;

    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++FillBuffer\n")));
        ASSERT(pCsVideoInfoHdr->bmiHeader.biSizeImage != 0);
    if(pImage == NULL)
    {
        RETAILMSG(MSG_ERROR,(TEXT("pImage is null!\n")));
        return 0;
    }
    // MDD will make sure that the buffer is sufficient for the image.

    if (ulModeType == CAPTURE)
    {
        CurrentFrame = CameraGetCurrentFrameNum(VIDEO_CAPTURE_BUFFER);
        RETAILMSG(MSG_ON,(TEXT("m_CameraHWVideoBuffers[%d].VirtAddr=0x%x\n"),CurrentFrame,m_CameraHWVideoBuffers[CurrentFrame].VirtAddr));
        dwRet = pCsVideoInfoHdr->bmiHeader.biSizeImage;
        memcpy(pImage, (void *)m_CameraHWVideoBuffers[CurrentFrame].VirtAddr, dwRet);
    }
    else if (ulModeType == STILL)
    {
        CurrentFrame = CameraGetCurrentFrameNum(STILL_CAPTURE_BUFFER);
        RETAILMSG(MSG_ON,(TEXT("m_CameraHWStillBuffer.VirtAddr=0x%x\n"),m_CameraHWStillBuffer.VirtAddr));
        dwRet = pCsVideoInfoHdr->bmiHeader.biSizeImage;
        memcpy(pImage, (void *)m_CameraHWStillBuffer.VirtAddr, dwRet);
    }
    else if(ulModeType == PREVIEW)
    {
        CurrentFrame = CameraGetCurrentFrameNum(PREVIEW_CAPTURE_BUFFER);
        dwRet = pCsVideoInfoHdr->bmiHeader.biSizeImage;
        RETAILMSG(MSG_ON,(TEXT("m_CameraHWPreviewBuffers[%d].VirtAddr=0x%x\n"),CurrentFrame,m_CameraHWPreviewBuffers[CurrentFrame].VirtAddr));
        memcpy(pImage, (void *)m_CameraHWPreviewBuffers[CurrentFrame].VirtAddr, dwRet);
    }
    RETAILMSG(MSG_INOUT,(TEXT("--------------------FillBuffer\n")));
    // return the size of the image filled
        return dwRet;
}


void CCameraPdd :: HandleCaptureInterrupt( ULONG ulModeTypeIn )
{
    ULONG ulModeType;
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++HandleCaptureInterrupt\n")));
    if( m_bStillCapInProgress )
    {
        return;
    }

    if( ulModeTypeIn == CAPTURE)
    {
        ulModeType = CAPTURE;
    }
    else if ( m_ulCTypes == 3 && ulModeTypeIn == PREVIEW )
    {
        ulModeType = PREVIEW;
    }
    else
    {
        ASSERT(false);
        return;
    }

    MDD_HandleIO( m_ppModeContext[ulModeType], ulModeType );
     RETAILMSG(MSG_INOUT,(TEXT("------------------------HandleCaptureInterrupt  %d\n"),ulModeType));

}


void CCameraPdd :: HandleStillInterrupt( )
{
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++HandleStillInterrupt\n")));
    MDD_HandleIO( m_ppModeContext[STILL], STILL );
    m_bStillCapInProgress = false;
    RETAILMSG(MSG_INOUT,(TEXT("--------------------HandleStillInterrupt\n")));
}

bool CCameraPdd::ReadMemoryModelFromRegistry()
{
    HKEY  hKey = 0;
    DWORD dwType  = 0;
    DWORD dwSize  = sizeof ( DWORD );
    DWORD dwValue = -1;
        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++ReadMemoryModelFromRegistry\n")));

    if( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Drivers\\Capture\\Camera", 0, 0, &hKey ))
    {
        //mio
        return false;
    }

    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"MemoryModel", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(   ( REG_DWORD == dwType )
           && ( sizeof( DWORD ) == dwSize )
           && (( dwValue == CSPROPERTY_BUFFER_DRIVER ) || ( dwValue == CSPROPERTY_BUFFER_CLIENT_LIMITED ) || ( dwValue == CSPROPERTY_BUFFER_CLIENT_UNLIMITED )))
        {
            for( int i=0; i<MAX_SUPPORTED_PINS ; i++ )
            {
                m_SensorModeInfo[i].MemoryModel = (CSPROPERTY_BUFFER_MODE) dwValue;
            }
        }
    }

    // Find out if we should be using some other number of supported modes. The only
    // valid options are 2 or 3. Default to 2.
    if ( ERROR_SUCCESS == RegQueryValueEx( hKey, L"PinCount", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if ( REG_DWORD == dwType
             && sizeof ( DWORD ) == dwSize
             && 3 == dwValue )
        {
            m_ulCTypes = 3;
        }
    }

    RegCloseKey( hKey );
    return true;
}

//mio
//bool CCameraPdd::AllocateHWBuffers(P_CAMERA_DMA_BUFFER_INFO pCamDmaBuf, ULONG ucBufferType)
//{
//        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++AllocateHWBuffers\n")));
//
//    return TRUE;
//}
//
//bool CCameraPdd::DeAllocateHWBuffers(ULONG ucBufferType)
//{
//        RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++DeAllocateHWBuffers\n")));
//
//    return TRUE;
//}

bool CCameraPdd::SetSensorFormat( ULONG ulModeType)
{
    RETAILMSG(MSG_INOUT,(TEXT("++++++++++++++++++++SetSensorFormat\n")));
    INT format;
    PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &m_pCurrentFormat[ulModeType].VideoInfoHeader;

    UINT biWidth        = pCsVideoInfoHdr->bmiHeader.biWidth;
    UINT biHeight       = abs(pCsVideoInfoHdr->bmiHeader.biHeight);
    DWORD biBitCount = pCsVideoInfoHdr->bmiHeader.biBitCount;
    DWORD biCompression = pCsVideoInfoHdr->bmiHeader.biCompression;


    // Prepare buffers here for Preview and Still mode.
    // Set Register for output

    if ( (FOURCC_YUY2 == (biCompression & ~BI_SRCPREROTATE)))
    {
        format = OUTPUT_CODEC_YCBCR422;
    }
    else if((FOURCC_YV12 == (biCompression & ~BI_SRCPREROTATE)))
    {
        format = OUTPUT_CODEC_YCBCR420;
    }
    else
    {
        if(biBitCount == 24)
        {
            format = OUTPUT_CODEC_RGB24;
        }
        else
        {
            format = OUTPUT_CODEC_RGB16;
        }
    }


    if (ulModeType == CAPTURE)
    {
        CameraSetFormat(biWidth, biHeight, format, VIDEO_CAPTURE_BUFFER);
        if (!CameraPrepareBuffer(m_CameraHWVideoBuffers, VIDEO_CAPTURE_BUFFER) )
        {
            RETAILMSG(MSG_ERROR, ( _T("InitSensorMode: CameraPrepareBuffer() failed\r\n") ) );
            return FALSE;
        }

    }
    else if (ulModeType == STILL)
    {
        CameraSetFormat(biWidth, biHeight, format, STILL_CAPTURE_BUFFER);
        if (!CameraPrepareBuffer(&m_CameraHWStillBuffer, STILL_CAPTURE_BUFFER))
        {
            RETAILMSG(MSG_ERROR, ( _T("InitSensorMode: CameraPrepareBuffer() failed\r\n") ) );
            return FALSE;
        }
    }
    else if(ulModeType == PREVIEW)
    {
        CameraSetFormat(biWidth, biHeight, format, PREVIEW_CAPTURE_BUFFER);
        if (!CameraPrepareBuffer(m_CameraHWPreviewBuffers, PREVIEW_CAPTURE_BUFFER))
        {
            RETAILMSG(MSG_ERROR, ( _T("InitSensorMode: CameraPrepareBuffer() failed\r\n") ) );
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

void CCameraPdd::SuspendCamera( )
{
    if(m_bCameraVideoRunning)
    {
        m_bCameraVideoWasRunning = TRUE;
        if (m_CsState[CAPTURE]== CSSTATE_RUN)
        {
            SetSensorState(CAPTURE, CSSTATE_PAUSE);
        }        
    }
    
    if(m_bCameraPreviewRunning)
    {
        m_bCameraPreviewWasRunning = TRUE;
        if (m_CsState[PREVIEW]== CSSTATE_RUN)
        {
            SetSensorState(PREVIEW, CSSTATE_PAUSE);
        }           
    }
    
    if(m_iPinUseCount > 0)
    {
        CameraSleep();        
        CameraClockOn(FALSE);
    }
}

void CCameraPdd::ResumeCamera( )
{
    // Restart camera sensor if it was running before
    
    if(m_iPinUseCount > 0)
    {
        CameraClockOn(TRUE);    
        CameraResume();        
    }      
    
    if(m_bCameraVideoWasRunning)
    {
        m_bCameraVideoWasRunning = FALSE;
        SetSensorState(CAPTURE, CSSTATE_RUN);        
    }
    if ( m_bCameraPreviewWasRunning )
    {
        m_bCameraPreviewWasRunning = FALSE;
        SetSensorState(PREVIEW, CSSTATE_RUN);
    }    
}

void CCameraPdd::CameraVideoFrameCallback( DWORD dwContext )
{
    CCameraPdd * pCamDevice = reinterpret_cast<CCameraPdd *>( dwContext );

    // Video frame is ready - put it into stream

    if (NULL != pCamDevice)
    {
        pCamDevice->HandleCaptureInterrupt(CAPTURE);
    }
}

void CCameraPdd::CameraStillFrameCallback( DWORD dwContext )
{
    CCameraPdd * pCamDevice = reinterpret_cast<CCameraPdd *>( dwContext );

    // Still image frame is ready - put it into stream

    if (NULL != pCamDevice)
    {
        pCamDevice->HandleStillInterrupt();
    }
}

void CCameraPdd::CameraPreviewFrameCallback( DWORD dwContext )
{
    CCameraPdd * pCamDevice = reinterpret_cast<CCameraPdd *>( dwContext );

    // Video frame is ready - put it into stream

    if (NULL != pCamDevice)
    {
        pCamDevice->HandleCaptureInterrupt(PREVIEW);
    }
}
