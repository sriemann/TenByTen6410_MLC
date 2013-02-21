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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//

#include <windows.h>
#include <pm.h>
#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include "CameraDriver.h"

#include "camera.h"
#include "PinDriver.h"

#include "CameraPDD.h"


DWORD MDD_HandleIO( LPVOID ModeContext, ULONG ulModeType )
{
    PPINDEVICE pPinDevice = (PPINDEVICE)ModeContext;
    if( NULL == pPinDevice )
    {
        return ERROR_INVALID_PARAMETER;
    }
    return pPinDevice->HandlePinIO() ;
}

CCameraDevice::CCameraDevice( )
{
    m_hStream        = NULL;
    m_hCallerProcess = NULL;

    m_dwVersion = 0;
    m_PowerState = D0;
    m_PDDContext = NULL;

    memset(&m_PDDFuncTbl, 0, sizeof(PDDFUNCTBL) );
    memset( &m_AdapterInfo, 0, sizeof(ADAPTERINFO ));
    InitializeCriticalSection( &m_csDevice );
    m_pStrmInstances = NULL;

}


CCameraDevice::~CCameraDevice( )
{
    DeleteCriticalSection( &m_csDevice );

    m_PDDFuncTbl.PDD_DeInit( m_PDDContext );

    if( NULL != m_pStrmInstances )
    {
        delete [] m_pStrmInstances;
        m_pStrmInstances = NULL;
    }

    if ( NULL != m_hStream )
    {
        DeactivateDevice( m_hStream );
        m_hStream = NULL;
    }
}


bool
CCameraDevice::Initialize(
    PVOID context
    )
{
    DWORD dwRet = ERROR_SUCCESS;
    m_hStream = ActivateDeviceEx( PIN_REG_PATH, NULL, 0, reinterpret_cast<LPVOID>( this ) );

    if ( NULL == m_hStream )
    {
        DEBUGMSG( ZONE_INIT|ZONE_ERROR, ( _T("CAM_Init: ActivateDevice on Pin failed\r\n") ) );
        return false;
    }

    m_PDDFuncTbl.dwSize = sizeof( PDDFUNCTBL );
    m_PDDContext = PDD_Init( context, &m_PDDFuncTbl );

    if( m_PDDContext == NULL )
    {
        return false;
    }       

    if( m_PDDFuncTbl.dwSize < sizeof( PDDFUNCTBL ) ||
        NULL == m_PDDFuncTbl.PDD_Init || 
        NULL == m_PDDFuncTbl.PDD_DeInit ||
        NULL == m_PDDFuncTbl.PDD_GetAdapterInfo ||
        NULL == m_PDDFuncTbl.PDD_HandleVidProcAmpChanges ||
        NULL == m_PDDFuncTbl.PDD_HandleCamControlChanges ||
        NULL == m_PDDFuncTbl.PDD_HandleVideoControlCapsChanges ||
        NULL == m_PDDFuncTbl.PDD_SetPowerState ||
        NULL == m_PDDFuncTbl.PDD_HandleAdapterCustomProperties ||
        NULL == m_PDDFuncTbl.PDD_InitSensorMode ||
        NULL == m_PDDFuncTbl.PDD_DeInitSensorMode ||
        NULL == m_PDDFuncTbl.PDD_SetSensorState ||
        NULL == m_PDDFuncTbl.PDD_TakeStillPicture ||
        NULL == m_PDDFuncTbl.PDD_GetSensorModeInfo ||
        NULL == m_PDDFuncTbl.PDD_SetSensorModeFormat ||
        NULL == m_PDDFuncTbl.PDD_FillBuffer ||
        NULL == m_PDDFuncTbl.PDD_HandleModeCustomProperties )
    {
        return false;
    }

    dwRet = m_PDDFuncTbl.PDD_GetAdapterInfo( m_PDDContext, &m_AdapterInfo );

    if( ERROR_SUCCESS != dwRet || DRIVER_VERSION > m_AdapterInfo.ulVersionID || DRIVER_VERSION_2 < m_AdapterInfo.ulVersionID )
    {
        return false;
    }

    m_dwVersion = m_AdapterInfo.ulVersionID;

    m_pStrmInstances = new STREAM_INSTANCES[m_AdapterInfo.ulCTypes];
    if( NULL == m_pStrmInstances )
    {
        return false;
    }
    memset( m_pStrmInstances, 0x0, sizeof ( STREAM_INSTANCES ) * m_AdapterInfo.ulCTypes );

    
    if( false == GetPDDPinInfo() )
    {
        delete m_pStrmInstances;
        m_pStrmInstances = NULL;
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Failed to retrieve sub-device information\r\n"), this)) ;
        return false ;
    }

    return true;
}

bool
CCameraDevice::GetPDDPinInfo()
{
    SENSORMODEINFO SensorModeInfo;
    if( NULL == m_pStrmInstances )
    {
        return false;
    }

    for( UINT i=0; i < m_AdapterInfo.ulCTypes; i++ )
    {
        if( ERROR_SUCCESS != PDDGetPinInfo( i, &SensorModeInfo ) )
        {
            return false ;
        }
        m_pStrmInstances[i].ulPossibleCount = SensorModeInfo.PossibleCount ;
        m_pStrmInstances[i].VideoCaps.DefaultVideoControlCaps = SensorModeInfo.VideoCaps.DefaultVideoControlCaps;
        m_pStrmInstances[i].VideoCaps.CurrentVideoControlCaps = SensorModeInfo.VideoCaps.CurrentVideoControlCaps;
        m_pStrmInstances[i].pVideoFormat = SensorModeInfo.pVideoFormat;
        
        if( SensorModeInfo.MemoryModel == CSPROPERTY_BUFFER_DRIVER &&
            ( NULL == m_PDDFuncTbl.PDD_AllocateBuffer || NULL == m_PDDFuncTbl.PDD_DeAllocateBuffer ) )
        {
            return false;
        }

        if( SensorModeInfo.MemoryModel != CSPROPERTY_BUFFER_DRIVER &&
            ( NULL == m_PDDFuncTbl.PDD_RegisterClientBuffer || NULL == m_PDDFuncTbl.PDD_UnRegisterClientBuffer ) )
        {
            return false;
        }
    }

    return true;

}

bool
CCameraDevice::BindApplicationProc(
    HANDLE hCurrentProc
    )
{
    if ( NULL != m_hCallerProcess )
    {
        return false;
    }

    m_hCallerProcess = hCurrentProc;

    return true;
}


bool
CCameraDevice::UnBindApplicationProc( )
{
    DEBUGMSG( ZONE_FUNCTION, ( _T("CAM_Close: Unbind application from camera device\r\n") ) );

    m_hCallerProcess = NULL;

    return true;
}

bool
CCameraDevice::IsValidPin(
    ULONG ulPinId
    )
{
    if ( ulPinId >= m_AdapterInfo.ulCTypes )
    {
        return false;
    }

    return true;
}

bool
CCameraDevice::IncrCInstances(
    ULONG        ulPinId,
    CPinDevice * pPinDev
    )
{
    bool bRet = false;

    if( IsValidPin( ulPinId ) )
    {
        PSTREAM_INSTANCES pStreamInst = &m_pStrmInstances[ulPinId];
        if( pStreamInst->ulCInstances < pStreamInst->ulPossibleCount )
        {
            pStreamInst->ulCInstances++;
// TODO This is memory leak
            pStreamInst->pPinDev = pPinDev;
            bRet = true;
        }

    }

    return bRet;
}

bool
CCameraDevice::DecrCInstances(
    ULONG ulPinId
    )
{
    bool bRet = false;

    if( IsValidPin( ulPinId ) )
    {
        PSTREAM_INSTANCES pStreamInst = &m_pStrmInstances[ulPinId];
        if( 0 < pStreamInst->ulCInstances )
        {
            pStreamInst->ulCInstances--;
// TODO This is memory leak
            pStreamInst->pPinDev = NULL;
            bRet = true;
        }
    }
    return bRet;
}

bool
CCameraDevice::PauseCaptureAndPreview( void )
{
    if ( PREVIEW < m_AdapterInfo.ulCTypes )
    {
        if ( NULL != m_pStrmInstances[PREVIEW].pPinDev )
        {
            m_pStrmInstances[PREVIEW].pPinDev->SetState( CSSTATE_PAUSE, &m_pStrmInstances[PREVIEW].CsPrevState );
        }
    }

    if ( NULL != m_pStrmInstances[CAPTURE].pPinDev )
    {
        m_pStrmInstances[CAPTURE].pPinDev->SetState( CSSTATE_PAUSE, &m_pStrmInstances[CAPTURE].CsPrevState );
    }

    return true;
}

bool
CCameraDevice::RevertCaptureAndPreviewState( void )
{
    if ( PREVIEW < m_AdapterInfo.ulCTypes )
    {
        m_pStrmInstances[PREVIEW].pPinDev->SetState( m_pStrmInstances[PREVIEW].CsPrevState, NULL );
    }

    m_pStrmInstances[CAPTURE].pPinDev->SetState( m_pStrmInstances[CAPTURE].CsPrevState, NULL );

    return true;
}


bool
CCameraDevice::GetPinFormat(
    ULONG                 ulPinId,
    ULONG                 ulIndex,
    PCS_DATARANGE_VIDEO * ppCsDataRangeVid
    )
{
    if( false == IsValidPin( ulPinId ) )
    {
        return false;
    }

    if ( 0 >= ulIndex || ulIndex > m_pStrmInstances[ulPinId].pVideoFormat->ulAvailFormats )
    {
        return false;
    }

    *ppCsDataRangeVid = m_pStrmInstances[ulPinId].pVideoFormat->pCsDataRangeVideo[ulIndex-1];

    return true;
}


bool
CCameraDevice::AdapterCompareFormat(
    ULONG                       ulPinId,
    const PCS_DATARANGE_VIDEO   pCsDataRangeVideoToCompare,
    PCS_DATARANGE_VIDEO       * ppCsDataRangeVideoMatched,
    bool                        fDetailedComparison
    )
{
    for ( ULONG ulCount = 0 ; ulCount < m_pStrmInstances[ulPinId].pVideoFormat->ulAvailFormats ; ulCount++ )
    {
        PCS_DATARANGE_VIDEO pCsDataRangeVideo = m_pStrmInstances[ulPinId].pVideoFormat->pCsDataRangeVideo[ulCount];

        if ( false == AdapterCompareGUIDsAndFormatSize( reinterpret_cast<PCSDATARANGE>( pCsDataRangeVideo ),
                                                        reinterpret_cast<PCSDATARANGE>( pCsDataRangeVideoToCompare ) ) )
        {
            continue;
        }

        if ( true == fDetailedComparison )
        {
            if ( ( pCsDataRangeVideoToCompare->bFixedSizeSamples != pCsDataRangeVideo->bFixedSizeSamples )
                  || ( pCsDataRangeVideoToCompare->bTemporalCompression   != pCsDataRangeVideo->bTemporalCompression )
                  || ( pCsDataRangeVideoToCompare->StreamDescriptionFlags != pCsDataRangeVideo->StreamDescriptionFlags )
                  || ( pCsDataRangeVideoToCompare->MemoryAllocationFlags  != pCsDataRangeVideo->MemoryAllocationFlags ) )
            {
                continue;
            }
            if( pCsDataRangeVideoToCompare->VideoInfoHeader.AvgTimePerFrame < pCsDataRangeVideo->ConfigCaps.MinFrameInterval  ||
                pCsDataRangeVideoToCompare->VideoInfoHeader.AvgTimePerFrame > pCsDataRangeVideo->ConfigCaps.MaxFrameInterval  )
            {
                continue;
            }

            if ( 0 != memcmp(&pCsDataRangeVideoToCompare->VideoInfoHeader.bmiHeader, &pCsDataRangeVideo->VideoInfoHeader.bmiHeader, sizeof (pCsDataRangeVideo->VideoInfoHeader.bmiHeader) ) )
            {
                continue;
            }

        }

        // You can now perform more granular comparison involving ConfigCaps and VIDOINFOHEADER etc.

        /////////////////////////////////////////
        if ( NULL != ppCsDataRangeVideoMatched )
        {
            *ppCsDataRangeVideoMatched = pCsDataRangeVideo;
        }

        return true;
    }

    return false;
}


bool
CCameraDevice::AdapterCompareFormat(
    ULONG                                  ulPinId,
    const PCS_DATAFORMAT_VIDEOINFOHEADER   pCsDataVIHToCompare,
    PCS_DATARANGE_VIDEO                  * ppCsDataRangeVideoMatched,
    bool                                   fDetailedComparison
    )
{
    for ( ULONG ulCount = 0 ; ulCount < m_pStrmInstances[ulPinId].pVideoFormat->ulAvailFormats ; ulCount++ )
    {
        PCS_DATARANGE_VIDEO pCsDataRangeVideo = m_pStrmInstances[ulPinId].pVideoFormat->pCsDataRangeVideo[ulCount];

        if ( false == AdapterCompareGUIDsAndFormatSize( reinterpret_cast<PCSDATARANGE>( pCsDataRangeVideo ),
                                                        reinterpret_cast<PCSDATARANGE>( pCsDataVIHToCompare ) ) )
        {
            continue;
        }

        if ( true == fDetailedComparison )
        {
            if( pCsDataVIHToCompare->VideoInfoHeader.AvgTimePerFrame < pCsDataRangeVideo->ConfigCaps.MinFrameInterval  ||
                pCsDataVIHToCompare->VideoInfoHeader.AvgTimePerFrame > pCsDataRangeVideo->ConfigCaps.MaxFrameInterval  )
            {
                continue;
            }

            if ( 0 != memcmp(&pCsDataVIHToCompare->VideoInfoHeader.bmiHeader, &pCsDataRangeVideo->VideoInfoHeader.bmiHeader, sizeof (pCsDataRangeVideo->VideoInfoHeader.bmiHeader) ) )
            {
                continue;
            }
        }

        // You can now perform more granular comparison involving ConfigCaps and VIDOINFOHEADER etc.

        /////////////////////////////////////////
        if ( NULL != ppCsDataRangeVideoMatched )
        {
            *ppCsDataRangeVideoMatched = pCsDataRangeVideo;
        }

        return true;
    }

    return false;
}


bool
CCameraDevice::AdapterCompareGUIDsAndFormatSize(
    const PCSDATARANGE DataRange1,
    const PCSDATARANGE DataRange2
    )
{
    return ( IsEqualGUID( DataRange1->MajorFormat, DataRange2->MajorFormat )
             && IsEqualGUID( DataRange1->SubFormat, DataRange2->SubFormat )
             && IsEqualGUID( DataRange1->Specifier, DataRange2->Specifier ) );
}


LPVOID
CCameraDevice::ValidateBuffer(
    LPVOID   lpBuff,
    ULONG    ulActualBufLen,
    ULONG    ulExpectedBuffLen,
    DWORD  * dwError
    )
{
    if ( NULL == lpBuff )
    {
        *dwError = ERROR_INSUFFICIENT_BUFFER;

        return NULL;
    }

    if ( ulActualBufLen < ulExpectedBuffLen )
    {
        DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, (_T("IOControl(%08x): buffer is not large enough\r\n"), this ) );
        *dwError = ERROR_INSUFFICIENT_BUFFER;

        return NULL;
    }


    //If the size is good, just return the buffer passed in
    return lpBuff;
}


DWORD
CCameraDevice::AdapterHandleVersion( 
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    )
{
    if( OutBufLen < sizeof( DWORD ))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pdwBytesTransferred = sizeof( DWORD );

    if( !CeSafeCopyMemory( pOutBuf, &m_dwVersion, sizeof( DWORD )))
    {
        return ERROR_INVALID_USER_BUFFER;
    }

    return ERROR_SUCCESS;
}


DWORD
CCameraDevice::AdapterHandlePinRequests(
    PUCHAR pInBuf,
    DWORD  InBufLen,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    )
{
    DEBUGMSG( ZONE_IOCTL, ( _T("CAM_IOControl(%08x): HandlePinRequests\r\n"), this ) );

    DWORD                          dwError           = ERROR_INVALID_PARAMETER;
    PCSP_PIN                       pCsPin            = NULL;
    PCS_DATARANGE_VIDEO            pCsDataRangeVideo = NULL;
    PCSMULTIPLE_ITEM               pCsMultipleItem   = NULL;
    PCS_DATAFORMAT_VIDEOINFOHEADER pCsDataFormatVih  = NULL;
    PCSPIN_CINSTANCES              pCsPinCinstances  = NULL;

    CSPROPERTY                      csProp = {0};
    LONG                            lPinId = 0;

    if( !CeSafeCopyMemory( &csProp, pInBuf, sizeof( CSPROPERTY )))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if ( CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags )
    {
        return ERROR_SUCCESS;
    }

    // we are here so the request is not SETSUPPORT and after this only GET requests will be entertained since
    // PROPSETID_Pin contains READ-ONLY properties
    if ( CSPROPERTY_TYPE_GET != csProp.Flags )
    {
        return dwError;
    }
   
    switch( csProp.Id )
    {
        case CSPROPERTY_PIN_CTYPES:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_CTYPES\r\n"), this ) );

            *pdwBytesTransferred = sizeof(ULONG);

            EnterCriticalSection( &m_csDevice );

            if( OutBufLen < sizeof( ULONG ))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if( !CeSafeCopyMemory( pOutBuf, &(m_AdapterInfo.ulCTypes), sizeof (ULONG) ))
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            LeaveCriticalSection( &m_csDevice );

            dwError = ERROR_SUCCESS;
            break;
        }

        case CSPROPERTY_PIN_CINSTANCES:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_CINSTANCES\r\n"), this ) );

            if ( NULL == ( pCsPin = reinterpret_cast<PCSP_PIN>( ValidateBuffer( pInBuf, InBufLen, sizeof (CSP_PIN), &dwError ) ) ) )
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if ( false == IsValidPin( lPinId ))
            {
                DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
                break;
            }

            *pdwBytesTransferred = sizeof (CSPIN_CINSTANCES);

            if ( NULL == ( pCsPinCinstances = reinterpret_cast<PCSPIN_CINSTANCES>( ValidateBuffer( pOutBuf, OutBufLen, sizeof (CSPIN_CINSTANCES), &dwError ) ) ) )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            pCsPinCinstances->PossibleCount = m_pStrmInstances[lPinId].ulPossibleCount;
            pCsPinCinstances->CurrentCount  = m_pStrmInstances[lPinId].ulCInstances;

            dwError = ERROR_SUCCESS;
            break;
        }

        case CSPROPERTY_PIN_DATARANGES:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_DATARANGES\r\n"), this ) );

            if( NULL == ( pCsPin = reinterpret_cast<PCSP_PIN>( ValidateBuffer( pInBuf, InBufLen, sizeof (CSP_PIN), &dwError ) ) ) )
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if ( false == IsValidPin( lPinId ) )
            {
                DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
                break;
            }

            *pdwBytesTransferred = sizeof(CSMULTIPLE_ITEM) + ( m_pStrmInstances[lPinId].pVideoFormat->ulAvailFormats * sizeof (CS_DATARANGE_VIDEO) );

            if ( NULL == ( pCsMultipleItem = reinterpret_cast<PCSMULTIPLE_ITEM>( ValidateBuffer( pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError ) ) ) )
            {
                dwError = ERROR_MORE_DATA ;
                break ;
            }

            pCsMultipleItem->Count = m_pStrmInstances[lPinId].pVideoFormat->ulAvailFormats;
            pCsMultipleItem->Size  = *pdwBytesTransferred;

            pCsDataRangeVideo = NULL;
            pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>( pCsMultipleItem + 1 );

            for ( int iCount = 0 ; iCount < static_cast<int>( m_pStrmInstances[lPinId].pVideoFormat->ulAvailFormats ); iCount++ )
            {
                memcpy( pCsDataRangeVideo, m_pStrmInstances[lPinId].pVideoFormat->pCsDataRangeVideo[iCount], sizeof (CS_DATARANGE_VIDEO) );
                pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>( pCsDataRangeVideo + 1 );
            }

            dwError = ERROR_SUCCESS;

            break;
        }

        case CSPROPERTY_PIN_DATAINTERSECTION:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_DATAINTERSECTION\r\n"), this ) );
            if ( NULL == ( pCsPin = reinterpret_cast<PCSP_PIN>( ValidateBuffer( pInBuf, InBufLen, sizeof (CSP_PIN), &dwError ) ) ) )
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if ( false == IsValidPin( lPinId ) )
            {
                DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
                break;
            }

            pCsMultipleItem = NULL;
            pCsMultipleItem = reinterpret_cast<PCSMULTIPLE_ITEM>( pCsPin + 1 );

            if ( NULL == ValidateBuffer( pInBuf, InBufLen, ( sizeof (CSP_PIN) + (pCsMultipleItem->Count * pCsMultipleItem->Size ) ), &dwError ) )
            {
                break;
            }

            {
                *pdwBytesTransferred = sizeof(CS_DATAFORMAT_VIDEOINFOHEADER);
            }

            if( NULL == ( pCsDataFormatVih = reinterpret_cast<PCS_DATAFORMAT_VIDEOINFOHEADER>( ValidateBuffer( pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError ) ) ) )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            pCsDataRangeVideo = NULL;
            pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>( pCsMultipleItem + 1 );


            {
                for ( int iCount = 0 ; iCount < (int)pCsMultipleItem->Count ; iCount++ )
                {
                    // First check whether the GUIDs match or not. This driver also tests other high level attributes of KS_DATARANGE_VIDEO
                    if ( true == AdapterCompareFormat( lPinId, pCsDataRangeVideo, NULL, true) )
                    {
                        // We found our format
                        memcpy(&pCsDataFormatVih->DataFormat,&pCsDataRangeVideo->DataRange, sizeof(CSDATAFORMAT) ) ;
                        memcpy(&pCsDataFormatVih->VideoInfoHeader,&pCsDataRangeVideo->VideoInfoHeader, sizeof(CS_VIDEOINFOHEADER) ) ;
                        dwError = ERROR_SUCCESS ;
                        break ;
                    }
                    pCsDataRangeVideo = reinterpret_cast<PCS_DATARANGE_VIDEO>(pCsDataRangeVideo + 1) ;
                }
            }
            break;
        }

        case CSPROPERTY_PIN_CATEGORY:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): CSPROPERTY_PIN_CATEGORY\r\n"), this ) );

            if ( NULL == ( pCsPin = reinterpret_cast<PCSP_PIN>( ValidateBuffer( pInBuf, InBufLen, sizeof (CSP_PIN), &dwError ) ) ) )
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if ( false == IsValidPin( lPinId ) )
            {
                DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
                break;
            }

            *pdwBytesTransferred = sizeof( GUID );

            if( OutBufLen < *pdwBytesTransferred )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if ( CeSafeCopyMemory( pOutBuf, &(m_pStrmInstances[lPinId].pVideoFormat->categoryGUID), sizeof( GUID ) ))
            {
                dwError = ERROR_SUCCESS;
            }

            break;
        }

        case CSPROPERTY_PIN_NAME:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Property CSPROPERTY_PIN_NAME\r\n"), this ) );

            if ( NULL == ( pCsPin = reinterpret_cast<PCSP_PIN>( ValidateBuffer( pInBuf, InBufLen, sizeof(CSP_PIN), &dwError ) ) ) )
            {
                break;
            }

            lPinId = pCsPin->PinId;

            if ( false == IsValidPin( lPinId ) )
            {
                DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
                break;
            }

            *pdwBytesTransferred = ( wcslen(g_wszPinNames[lPinId] ) + 1 ) * sizeof( g_wszPinNames[0][0] );

            if( OutBufLen < *pdwBytesTransferred )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if( CeSafeCopyMemory( pOutBuf, g_wszPinNames[lPinId], *pdwBytesTransferred ))
            {
                dwError = ERROR_SUCCESS;
            }

            break;
        }

        case CSPROPERTY_PIN_DEVICENAME:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Property CSPROPERTY_PIN_NAME\r\n"), this ) );

            if ( NULL == ( pCsPin = reinterpret_cast<PCSP_PIN>( ValidateBuffer( pInBuf, InBufLen, sizeof(CSP_PIN), &dwError ) ) ) )
            {
                break;
            }

            lPinId = pCsPin->PinId;
            if ( false == IsValidPin( lPinId ) )
            {
                DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
                break;
            }

            *pdwBytesTransferred = ( wcslen( g_wszPinDeviceNames[lPinId] ) + 1 ) * sizeof( g_wszPinDeviceNames[0][0] ) ;

            if( OutBufLen < *pdwBytesTransferred )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if( CeSafeCopyMemory( pOutBuf, g_wszPinDeviceNames[lPinId], *pdwBytesTransferred ))
            {
                dwError = ERROR_SUCCESS;
            }

            break;
        }

        default:
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this ) );

            break;
        }
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleVidProcAmpRequests(
    PUCHAR pInBuf,                  // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,                 // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred      // Warning: This is an unsafe buffer, access with care
    )
{
    // Note: This whole function is wrapped in a __try/__except block

    DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandleVidProcAmpRequests\r\n"), this ) );

    DWORD                      dwError                 = ERROR_INVALID_PARAMETER;
    LONG                       lValue                  = 0;
    LONG                       lFlags                  = 0;
    PSENSOR_PROPERTY           pSensorProp             = NULL;
    PCSPROPERTY_VIDEOPROCAMP_S pCsPropVidProcAmpOutput = NULL;
    PCSPROPERTY_VIDEOPROCAMP_S pCsPropVidProcAmpInput  = NULL;
    PCSPROPERTY_DESCRIPTION    pCsPropDesc             = NULL;
    PCSPROPERTY_VALUES         pCsPropValues           = NULL;

    CSPROPERTY                 csProp = {0};

    if( !CeSafeCopyMemory( &csProp, pInBuf, sizeof( CSPROPERTY )))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if ( CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags )
    {
        return ERROR_SUCCESS;
    }

    if ( csProp.Id < CSPROPERTY_VIDEOPROCAMP_BRIGHTNESS || csProp.Id > ENUM_GAIN )
    {
        DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this ) );

        return dwError;
    }

    if ( ERROR_SUCCESS != m_PDDFuncTbl.PDD_GetAdapterInfo( m_PDDContext, &m_AdapterInfo ) )
    {
        return dwError;
    }

    pSensorProp = m_AdapterInfo.SensorProps + csProp.Id;

    switch( csProp.Flags )
    {
    case CSPROPERTY_TYPE_GET:


        if ( FALSE == pSensorProp->fGetSupported )
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        *pdwBytesTransferred = sizeof(CSPROPERTY_VIDEOPROCAMP_S);
        if ( NULL == ( pCsPropVidProcAmpInput = reinterpret_cast<PCSPROPERTY_VIDEOPROCAMP_S>( ValidateBuffer( pInBuf, InBufLen, sizeof (CSPROPERTY_VIDEOPROCAMP_S), &dwError ) ) ) )
        {
            return dwError;
        }

        if( NULL == ( pCsPropVidProcAmpOutput = reinterpret_cast<PCSPROPERTY_VIDEOPROCAMP_S>( ValidateBuffer( pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError ) ) ) )
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        //Copy the CSPROPERTY structure to the output buffer just in case!
        memcpy( pCsPropVidProcAmpOutput, pCsPropVidProcAmpInput, sizeof(CSPROPERTY) );

        pCsPropVidProcAmpOutput->Value        = pSensorProp->ulCurrentValue;
        pCsPropVidProcAmpOutput->Flags        = pSensorProp->ulFlags;
        pCsPropVidProcAmpOutput->Capabilities = pSensorProp->ulCapabilities;

        dwError = ERROR_SUCCESS;
        break;

    case CSPROPERTY_TYPE_SET:

        if ( NULL == ( pCsPropVidProcAmpInput = reinterpret_cast<PCSPROPERTY_VIDEOPROCAMP_S>( ValidateBuffer( pInBuf, InBufLen, sizeof (CSPROPERTY_VIDEOPROCAMP_S), &dwError ) ) ) )
        {
            return dwError;
        }

        if ( FALSE == pSensorProp->fSetSupported )
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        // CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL and CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO are mutually exclusive
        if( pCsPropVidProcAmpInput->Flags != CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL && 
            pCsPropVidProcAmpInput->Flags != CSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO )
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        lValue = pCsPropVidProcAmpInput->Value;
        lFlags = pCsPropVidProcAmpInput->Flags;

        if( pCsPropVidProcAmpInput->Flags & ~pSensorProp->ulCapabilities )
        {
            break;
        }

        if( CSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL == pCsPropVidProcAmpInput->Flags )
        {
            if ( lValue < pSensorProp->pRangeNStep->Bounds.SignedMinimum ||
                lValue > pSensorProp->pRangeNStep->Bounds.SignedMaximum ||
                0 != ((lValue - pSensorProp->pRangeNStep->Bounds.SignedMinimum) % pSensorProp->pRangeNStep->SteppingDelta) )
            {
                break;
            }
        }

        dwError = m_PDDFuncTbl.PDD_HandleVidProcAmpChanges( m_PDDContext, csProp.Id, lFlags, lValue );
        break;

    case CSPROPERTY_TYPE_DEFAULTVALUES:

        GetDefaultValues( pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError );
        break;

    case CSPROPERTY_TYPE_BASICSUPPORT:

        GetBasicSupportInfo( pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError );
        break;

    default :
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this));
        return dwError;
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleCamControlRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
    )
{
    // Note: This whole function is wrapped in a __try/__except block

    DEBUGMSG( ZONE_IOCTL, ( _T("CAM_IOControl(%08x): HandleCamControlRequests\r\n"), this ) );
    DWORD dwError = ERROR_INVALID_PARAMETER;
    LONG  lValue  = 0;
    LONG  lFlags  = 0;
    PSENSOR_PROPERTY pSensorProp = NULL;
    PCSPROPERTY_CAMERACONTROL_S pCsPropCamControlOutput = NULL;
    PCSPROPERTY_CAMERACONTROL_S pCsPropCamControlInput  = NULL;

    CSPROPERTY   csProp = {0};
    if( !CeSafeCopyMemory( &csProp, pInBuf, sizeof( CSPROPERTY )))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if ( CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags )
    {
        return ERROR_SUCCESS;
    }

    if ( csProp.Id < CSPROPERTY_CAMERACONTROL_PAN || csProp.Id > CSPROPERTY_CAMERACONTROL_FLASH )
    {
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this));
        return dwError;
    }

    if ( ERROR_SUCCESS != m_PDDFuncTbl.PDD_GetAdapterInfo( m_PDDContext, &m_AdapterInfo ) )
    {
        return dwError;
    }

    pSensorProp = m_AdapterInfo.SensorProps + (csProp.Id + (NUM_VIDEOPROCAMP_ITEMS));

    switch( csProp.Flags )
    {
    case CSPROPERTY_TYPE_GET:

        if ( NULL == ( pCsPropCamControlInput = reinterpret_cast<PCSPROPERTY_CAMERACONTROL_S>( ValidateBuffer( pInBuf, InBufLen, sizeof(CSPROPERTY_CAMERACONTROL_S), &dwError ) ) ) )
        {
            return dwError;
        }

        if ( FALSE == pSensorProp->fGetSupported )
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        *pdwBytesTransferred = sizeof(CSPROPERTY_CAMERACONTROL_S);
        if( NULL == ( pCsPropCamControlOutput = reinterpret_cast<PCSPROPERTY_CAMERACONTROL_S>(ValidateBuffer( pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError ) ) ) )
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        //Copy the CSPROPERTY structure to the output buffer just in case!
        memcpy( pCsPropCamControlOutput, pCsPropCamControlInput, sizeof(CSPROPERTY) );

        pCsPropCamControlOutput->Value         = pSensorProp->ulCurrentValue;
        pCsPropCamControlOutput->Flags         = pSensorProp->ulFlags;
        pCsPropCamControlOutput->Capabilities  = pSensorProp->ulCapabilities;

        dwError = ERROR_SUCCESS;
        break;

    case CSPROPERTY_TYPE_SET:

        if ( NULL == ( pCsPropCamControlInput = reinterpret_cast<PCSPROPERTY_CAMERACONTROL_S>( ValidateBuffer( pInBuf, InBufLen, sizeof(CSPROPERTY_CAMERACONTROL_S), &dwError ) ) ) )
        {
            return dwError;
        }

        if ( FALSE == pSensorProp->fSetSupported )
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        // CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL and CSPROPERTY_CAMERACONTROL_FLAGS_AUTO are mutually exclusive
        if( pCsPropCamControlInput->Flags != CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL && 
            pCsPropCamControlInput->Flags != CSPROPERTY_CAMERACONTROL_FLAGS_AUTO )
        {
            SetLastError(ERROR_INVALID_OPERATION);
            break;
        }

        lValue = pCsPropCamControlInput->Value;
        lFlags = pCsPropCamControlInput->Flags;

        if( lFlags & ~pSensorProp->ulCapabilities )
        {
            break;
        }
        
        if( CSPROPERTY_CAMERACONTROL_FLAGS_MANUAL == lFlags )
        {
            if ( lValue < pSensorProp->pRangeNStep->Bounds.SignedMinimum ||
                lValue > pSensorProp->pRangeNStep->Bounds.SignedMaximum ||
                0 != ((lValue - pSensorProp->pRangeNStep->Bounds.SignedMinimum) % pSensorProp->pRangeNStep->SteppingDelta) )
            {
                break;
            }

        }

        dwError = m_PDDFuncTbl.PDD_HandleCamControlChanges( m_PDDContext, csProp.Id + (NUM_VIDEOPROCAMP_ITEMS), lFlags, lValue );                                                                                             
        
        break;

    case CSPROPERTY_TYPE_DEFAULTVALUES:

        GetDefaultValues( pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError );
        break;

    case CSPROPERTY_TYPE_BASICSUPPORT:

        GetBasicSupportInfo( pOutBuf, OutBufLen, pdwBytesTransferred, pSensorProp, &dwError );
        break;

    default :
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this));
        return dwError;
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleVideoControlRequests(
    PUCHAR pInBuf,                  // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,                 // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred      // Warning: This is an unsafe buffer, access with care
    )
{
    // Note: This whole function is wrapped in a __try/__except block
    DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandleVideoControlRequests\r\n"), this ) );

    DWORD dwError = ERROR_INVALID_PARAMETER;
    LONG  lValue  = 0;
    ULONG ulCaps  = 0;
    LONG lIndex = 0;

    PCSPROPERTY_VIDEOCONTROL_CAPS_S pCsPropVideoControlCapsOutput = NULL;
    PCSPROPERTY_VIDEOCONTROL_CAPS_S pCsPropVideoControlCapsInput  = NULL;
    PCSPROPERTY_VIDEOCONTROL_MODE_S pCsPropVideoControlModeInput  = NULL;

    CSPROPERTY csProp = {0};

    if( !CeSafeCopyMemory( &csProp, pInBuf, sizeof( CSPROPERTY )))
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    // we support PROPSETID_Pin, so just return success
    if ( CSPROPERTY_TYPE_SETSUPPORT == csProp.Flags )
    {
        return ERROR_SUCCESS;
    }

    switch( csProp.Id )
    {
    case CSPROPERTY_VIDEOCONTROL_CAPS:
        if( NULL == ( pCsPropVideoControlCapsInput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_CAPS_S>(ValidateBuffer( pInBuf, InBufLen, sizeof(CSPROPERTY_VIDEOCONTROL_CAPS_S), &dwError ) ) ) )
        {
            break;
        }

        lIndex = pCsPropVideoControlCapsInput->StreamIndex;
        if ( false == IsValidPin( lIndex ) )
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
            break;
        }

        switch ( csProp.Flags )
        {
        case CSPROPERTY_TYPE_GET:
            *pdwBytesTransferred = sizeof(CSPROPERTY_VIDEOCONTROL_CAPS_S);
            if ( NULL == ( pCsPropVideoControlCapsOutput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_CAPS_S>( ValidateBuffer( pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError ) ) ) )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            //Copy the CSPROPERTY structure to the output buffer just in case!
            memcpy( pCsPropVideoControlCapsOutput, pCsPropVideoControlCapsInput, sizeof( CSPROPERTY ) );

            pCsPropVideoControlCapsOutput->StreamIndex      = lIndex;

            if( true == GetPDDPinInfo() )
            {
                pCsPropVideoControlCapsOutput->VideoControlCaps = m_pStrmInstances[lIndex].VideoCaps.CurrentVideoControlCaps;

                dwError = ERROR_SUCCESS;
            }
            break;

        case CSPROPERTY_TYPE_SET:
            lIndex = pCsPropVideoControlCapsInput->StreamIndex;

            ulCaps = pCsPropVideoControlCapsInput->VideoControlCaps;
            if ( ulCaps & ~( CS_VideoControlFlag_FlipHorizontal        |
                             CS_VideoControlFlag_FlipVertical          |
                             CS_VideoControlFlag_ExternalTriggerEnable |
                             CS_VideoControlFlag_Trigger ) )
            {
                DEBUGMSG( ZONE_IOCTL, ( _T("CAM_IOControl(%08x): Invalid flag specified as Video Control Caps.\r\n"), this ) );
                break;
            }

            if( false == IsValidPin( lIndex ))
            {
                DEBUGMSG( ZONE_IOCTL, ( _T("CAM_IOControl(%08x): Invalid pin index.\r\n"), this ) );
                break;
            }

            dwError = m_PDDFuncTbl.PDD_HandleVideoControlCapsChanges( m_PDDContext, lIndex ,ulCaps );            
            break;

        case CSPROPERTY_TYPE_DEFAULTVALUES:
            *pdwBytesTransferred = sizeof(CSPROPERTY_VIDEOCONTROL_CAPS_S);

            if ( NULL == ( pCsPropVideoControlCapsOutput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_CAPS_S>( ValidateBuffer( pOutBuf, OutBufLen, *pdwBytesTransferred, &dwError ) ) ) )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            lIndex = pCsPropVideoControlCapsInput->StreamIndex;
            if( false == IsValidPin( lIndex ))
            {
                DEBUGMSG( ZONE_IOCTL, ( _T("CAM_IOControl(%08x): Invalid pin index.\r\n"), this ) );
                break;
            }

            //Copy the CSPROPERTY structure to the output buffer just in case!
            memcpy( pCsPropVideoControlCapsOutput, pCsPropVideoControlCapsInput, sizeof ( CSPROPERTY ) );

            pCsPropVideoControlCapsOutput->StreamIndex      = lIndex;

            pCsPropVideoControlCapsOutput->VideoControlCaps =  m_pStrmInstances[lIndex].VideoCaps.DefaultVideoControlCaps;

            dwError = ERROR_SUCCESS;

            break;

        default:
            DEBUGMSG( ZONE_IOCTL, ( _T("CAM_IOControl(%08x): Invalid Request\r\n"), this ) );
            break;
        }

        break;

    case CSPROPERTY_VIDEOCONTROL_MODE:
        if( NULL == ( pCsPropVideoControlModeInput = reinterpret_cast<PCSPROPERTY_VIDEOCONTROL_MODE_S>(ValidateBuffer( pInBuf, InBufLen, sizeof(CSPROPERTY_VIDEOCONTROL_MODE_S), &dwError ) ) ) )
        {
            break;
        }

        lIndex = pCsPropVideoControlModeInput->StreamIndex;
        if ( false == IsValidPin( lIndex ) )
        {
            DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid PinId\r\n"), this ) );
            break;
        }

        switch( csProp.Flags )
        {
        case CSPROPERTY_TYPE_SET:
            ulCaps = pCsPropVideoControlModeInput->Mode;
            if ( ulCaps != CS_VideoControlFlag_Trigger )
            {
                DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid flag specified as Video Control Mode.\r\n"), this ) );
                break;
            }

            //Only STILL pin supports CS_VideoControlFlag_Trigger for the current CAMERA MDD
            if ( m_pStrmInstances[lIndex].pPinDev &&  (  lIndex == STILL ) )
            {
                if( CSSTATE_STOP == m_pStrmInstances[lIndex].pPinDev->GetState() )
                {
                    dwError = ERROR_INVALID_STATE;
                }
                else
                {
                    m_pStrmInstances[lIndex].pPinDev->IncrementStillCount();
                    m_pStrmInstances[lIndex].pPinDev->SetState( CSSTATE_RUN, NULL );
                    dwError = m_PDDFuncTbl.PDD_TakeStillPicture( m_PDDContext, NULL );
                }
            }
            else
            {
                DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid pin for Trigger.\r\n"), this ) );
            }

            break ;

        default :
            DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Request\r\n"), this)) ;
            break ;
        }

    default:
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this)) ;
        return dwError ;
    }

    return dwError;
}

DWORD
CCameraDevice::AdapterHandleDroppedFramesRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
    )
{
    // Note: This whole function is wrapped in a __try/__except block
    DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): HandleDroppedFramesRequests\r\n"), this ) );

    DWORD dwError = ERROR_INVALID_PARAMETER;
    LONG  lValue  = 0;
    ULONG ulCaps  = 0;

    CSPROPERTY csProp;
    CSPROPERTY_DROPPEDFRAMES_CURRENT_S csPropDroppedFramesOutput = {0};

    //if( !CeSafeCopyMemory( &csProp, pInBuf, InBufLen ))
    if( !CeSafeCopyMemory( &csProp, pInBuf, sizeof(CSPROPERTY)) )
    {
        return dwError;
    }

    *pdwBytesTransferred = 0;

    switch( csProp.Id )
    {
    case CSPROPERTY_DROPPEDFRAMES_CURRENT:

        switch ( csProp.Flags )
        {
        case CSPROPERTY_TYPE_GET:
            *pdwBytesTransferred = sizeof(CSPROPERTY_DROPPEDFRAMES_CURRENT_S);
            if( OutBufLen < *pdwBytesTransferred )
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            if( NULL == pOutBuf )
            {
                break;
            }

            //Copy the CSPROPERTY structure to the output buffer just in case!
            memcpy( &csPropDroppedFramesOutput, &csProp, sizeof( CSPROPERTY ) );

            if ( m_pStrmInstances[CAPTURE].pPinDev )
            {
                csPropDroppedFramesOutput.PictureNumber    = m_pStrmInstances[CAPTURE].pPinDev->PictureNumber( );
                csPropDroppedFramesOutput.DropCount        = m_pStrmInstances[CAPTURE].pPinDev->FramesDropped( );
                csPropDroppedFramesOutput.AverageFrameSize = m_pStrmInstances[CAPTURE].pPinDev->FrameSize( );

                if( CeSafeCopyMemory( pOutBuf, &csPropDroppedFramesOutput, sizeof( CSPROPERTY_DROPPEDFRAMES_CURRENT_S )))
                {
                    dwError = ERROR_SUCCESS;
                }
            }

            break;
        }

    default:
        DEBUGMSG(ZONE_IOCTL, (_T("CAM_IOControl(%08x): Invalid Property\r\n"), this)) ;
        return dwError;
    }

    return dwError;
}

DWORD 
CCameraDevice::AdapterHandlePowerRequests(
    DWORD  dwCode,
    PUCHAR pInBuf,
    DWORD  InBufLen,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    )
{
    DWORD dwErr = ERROR_INVALID_PARAMETER;

    DEBUGMSG( ZONE_IOCTL, ( _T("CAM_IOControl(%08x): HandlePowerRequests\r\n"), this ) );

    switch (dwCode)
    {
    //
    // Power Management Support.
    //
    case IOCTL_POWER_CAPABILITIES:

        DEBUGMSG(ZONE_IOCTL, (TEXT("CAM: IOCTL_POWER_CAPABILITIES\r\n")));
        if ( pOutBuf && OutBufLen >= sizeof(POWER_CAPABILITIES) )
        {
             memcpy(pOutBuf, &m_AdapterInfo.PowerCaps, sizeof(POWER_CAPABILITIES));
             if(pdwBytesTransferred)
             {
                 *pdwBytesTransferred = sizeof(POWER_CAPABILITIES);
             }
             dwErr = ERROR_SUCCESS;
        }
        break;

    case IOCTL_POWER_SET:

        DEBUGMSG(ZONE_IOCTL, (TEXT("CAM: IOCTL_POWER_SET\r\n")));
        if ( pOutBuf && OutBufLen >= sizeof(CEDEVICE_POWER_STATE) )
        {
            PCEDEVICE_POWER_STATE pState = (PCEDEVICE_POWER_STATE) pOutBuf;

            switch (*pState)
            {
            case D0:
                RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D0\r\n")));

                m_PowerState = D0;
                PowerUp();

                dwErr = ERROR_SUCCESS;
                break;
            
            case D1:
                RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D0\r\n")));

                m_PowerState = D1;
                PowerUp();                

                dwErr = ERROR_SUCCESS;
                break;


            case D2:
                RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D2\r\n")));

                m_PowerState = D2;
                PowerUp();                

                dwErr = ERROR_SUCCESS;
                break;
        
            case D3:
                RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D3\r\n")));

                m_PowerState = D3;
                PowerDown();

                dwErr = ERROR_SUCCESS;
                break;
                
            case D4:
                RETAILMSG(ZONE_IOCTL, (TEXT("CAM: D4\r\n")));

                m_PowerState = D4;
                PowerDown();

                dwErr = ERROR_SUCCESS;
                break;

            default:
                RETAILMSG(ZONE_IOCTL|ZONE_WARN, (TEXT("CAM: Unrecognized Valid Power state. Powering Up\r\n")));
                
                m_PowerState = *pState;
                PowerUp();                
                
                dwErr = ERROR_SUCCESS;
                break;
            }

            if ( dwErr == ERROR_SUCCESS )
            {
                *pState = m_PowerState;
                if(pdwBytesTransferred)
                {
                    *pdwBytesTransferred = sizeof(CEDEVICE_POWER_STATE);
                }
            }
        }
        break;

    case IOCTL_POWER_GET:

        DEBUGMSG(ZONE_IOCTL, (TEXT("CAM: IOCTL_POWER_GET\r\n")));
        if ( pOutBuf && OutBufLen >= sizeof(CEDEVICE_POWER_STATE) )
        {
            *((PCEDEVICE_POWER_STATE) pOutBuf) = m_PowerState;
            if(pdwBytesTransferred)
            {
                *pdwBytesTransferred = sizeof(CEDEVICE_POWER_STATE);
            }
            dwErr = ERROR_SUCCESS;
        }
        break;

    default:
        break;
    }

    return dwErr;
}

DWORD
CCameraDevice::AdapterHandleCustomRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
    )
{
    // Note: This whole function is wrapped in a __try/__except block
    DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): AdapterHandleCustomRequests\r\n"), this ) );
    return m_PDDFuncTbl.PDD_HandleAdapterCustomProperties( m_PDDContext, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred );
}

void 
CCameraDevice::PowerDown()
{
    if ( m_hCallerProcess )
    {
        for( UINT i = 0; i < m_AdapterInfo.ulCTypes; i++ )
        {
            if ( NULL != m_pStrmInstances[i].pPinDev ) 
            {
                m_pStrmInstances[i].pPinDev->SetState( CSSTATE_STOP, &m_pStrmInstances[i].CsPrevState );
            }
        }
        m_PDDFuncTbl.PDD_SetPowerState( m_PDDContext, m_PowerState);
    }
}

void 
CCameraDevice::PowerUp()
{
    if ( m_hCallerProcess )
    {
        m_PDDFuncTbl.PDD_SetPowerState( m_PDDContext, m_PowerState );
        for( UINT i = 0; i < m_AdapterInfo.ulCTypes; i++ )
        {
            if ( NULL != m_pStrmInstances[i].pPinDev ) 
            {
                m_pStrmInstances[i].pPinDev->SetState( m_pStrmInstances[i].CsPrevState, NULL );
            }
        }
    }
}

void
CCameraDevice::GetBasicSupportInfo(
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred,
    PSENSOR_PROPERTY pSensorProp,
    PDWORD pdwError
    )
{
    PCSPROPERTY_DESCRIPTION   pCsPropDesc          = NULL;
    PCSPROPERTY_MEMBERSHEADER pCsPropMembersHeader = NULL;

    if ( OutBufLen >= sizeof(CSPROPERTY_DESCRIPTION) )
    {
        if( OutBufLen >= StandardSizeOfBasicValues )
        {
            *pdwBytesTransferred = StandardSizeOfBasicValues;
        }
        else
        {
            *pdwBytesTransferred = sizeof(CSPROPERTY_DESCRIPTION);
        }

        pCsPropDesc = reinterpret_cast<PCSPROPERTY_DESCRIPTION>( ValidateBuffer( pOutBuf, OutBufLen, sizeof (CSPROPERTY_DESCRIPTION), pdwError ) );

        if ( NULL == pCsPropDesc )
        {
            *pdwError = ERROR_MORE_DATA;

            return;
        }

        pCsPropDesc->AccessFlags      = (CSPROPERTY_TYPE_GET | CSPROPERTY_TYPE_SET | CSPROPERTY_TYPE_DEFAULTVALUES);
        pCsPropDesc->MembersListCount = 1;
        pCsPropDesc->Reserved         = 0;
        pCsPropDesc->DescriptionSize  = StandardSizeOfBasicValues;

        memcpy( &pCsPropDesc->PropTypeSet, &pSensorProp->pCsPropValues->PropTypeSet, sizeof (CSIDENTIFIER) );

        if( OutBufLen >= StandardSizeOfBasicValues )
        {
            pCsPropMembersHeader = reinterpret_cast<PCSPROPERTY_MEMBERSHEADER>( pCsPropDesc + 1 );

            // Copy the CSPROPERTY_MEMBERSHEADER
            memcpy( pCsPropMembersHeader, pSensorProp->pCsPropValues->MembersList, sizeof (CSPROPERTY_MEMBERSHEADER) );

            // Copy the CSPROPERTY_STEPPING_LONG
            memcpy( pCsPropMembersHeader + 1, pSensorProp->pCsPropValues->MembersList->Members, sizeof (CSPROPERTY_STEPPING_LONG) );

            *pdwError = ERROR_SUCCESS;
        }
        else
        {
            DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Not all available data could be written\r\n"), this ) );
            *pdwError = ERROR_MORE_DATA;
        }
    }
    else if ( OutBufLen >= sizeof(ULONG) )
    {
        // We just need to return AccessFlags.
        DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Returning AccessFlags\r\n"), this ) );

        *pdwBytesTransferred = sizeof(ULONG);

        if ( NULL == ValidateBuffer( pOutBuf, OutBufLen, sizeof (ULONG), pdwError ) )
        {
            *pdwError = ERROR_MORE_DATA;
            return;
        }

        *((PULONG)pOutBuf) = static_cast<ULONG>( CSPROPERTY_TYPE_GET | CSPROPERTY_TYPE_SET | CSPROPERTY_TYPE_DEFAULTVALUES );
        *pdwError = ERROR_SUCCESS;
    }
    else
    {
        DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Output buffer is not large enough\r\n"), this ) );
        *pdwError = ERROR_INSUFFICIENT_BUFFER;
    }

    return;
}

void
CCameraDevice::GetDefaultValues(
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred,
    PSENSOR_PROPERTY pSensorProp,
    PDWORD pdwError
    )
{
    PCSPROPERTY_DESCRIPTION   pCsPropDesc          = NULL;
    PCSPROPERTY_MEMBERSHEADER pCsPropMembersHeader = NULL;

    if ( OutBufLen >= sizeof(CSPROPERTY_DESCRIPTION) )
    {
        if( OutBufLen >= StandardSizeOfDefaultValues )
        {
            *pdwBytesTransferred = StandardSizeOfDefaultValues;
        }
        else
        {
            *pdwBytesTransferred = sizeof(CSPROPERTY_DESCRIPTION);
        }

        pCsPropDesc = reinterpret_cast<PCSPROPERTY_DESCRIPTION>( ValidateBuffer( pOutBuf, OutBufLen, sizeof (CSPROPERTY_DESCRIPTION), pdwError ) );

        if ( NULL == pCsPropDesc )
        {
            *pdwError = ERROR_MORE_DATA;

            return;
        }

        pCsPropDesc->AccessFlags      = (CSPROPERTY_TYPE_GET | CSPROPERTY_TYPE_SET | CSPROPERTY_TYPE_DEFAULTVALUES);
        pCsPropDesc->MembersListCount = 1;
        pCsPropDesc->Reserved         = 0;
        pCsPropDesc->DescriptionSize  = StandardSizeOfDefaultValues;

        memcpy( &pCsPropDesc->PropTypeSet, &pSensorProp->pCsPropValues->PropTypeSet, sizeof (CSIDENTIFIER) );

        if( OutBufLen >= StandardSizeOfDefaultValues )
        {
            pCsPropMembersHeader = reinterpret_cast<PCSPROPERTY_MEMBERSHEADER>( pCsPropDesc + 1 );

            // Copy the CSPROPERTY_MEMBERSHEADER
            memcpy( pCsPropMembersHeader, &(pSensorProp->pCsPropValues->MembersList[1]), sizeof (CSPROPERTY_MEMBERSHEADER) );

            // Copy the LONG (default value)
            memcpy( pCsPropMembersHeader + 1, pSensorProp->pCsPropValues->MembersList[1].Members, sizeof (LONG) );

            *pdwError = ERROR_SUCCESS;
        }
        else
        {
            DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, (_T("CAM_IOControl(%08x): Not all available data could be written\r\n"), this ) );
            *pdwError = ERROR_MORE_DATA;
        }
    }
    else if ( OutBufLen >= sizeof(ULONG) )
    {
        // We just need to return AccessFlags.
        DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Returning AccessFlags\r\n"), this ) );

        *pdwBytesTransferred = sizeof(ULONG);

        if ( NULL == ValidateBuffer( pOutBuf, OutBufLen, sizeof (ULONG), pdwError ) )
        {
            *pdwError = ERROR_MORE_DATA;
            return;
        }

        *((PULONG)pOutBuf) = static_cast<ULONG>( CSPROPERTY_TYPE_GET | CSPROPERTY_TYPE_SET | CSPROPERTY_TYPE_DEFAULTVALUES );
        *pdwError = ERROR_SUCCESS;
    }
    else
    {
        DEBUGMSG( ZONE_IOCTL, (_T("CAM_IOControl(%08x): Output buffer is not large enough\r\n"), this ) );
        *pdwError = ERROR_INSUFFICIENT_BUFFER;
    }

    return;
}

DWORD CCameraDevice::PDDClosePin( ULONG ulPinId )
{   
    return m_PDDFuncTbl.PDD_DeInitSensorMode( m_PDDContext, ulPinId );
}

DWORD CCameraDevice::PDDGetPinInfo( ULONG ulPinId, PSENSORMODEINFO pSensorModeInfo )
{    
    return m_PDDFuncTbl.PDD_GetSensorModeInfo( m_PDDContext, ulPinId, pSensorModeInfo );
}


DWORD CCameraDevice::PDDSetPinState( ULONG ulPinId, CSSTATE State )
{   
    return m_PDDFuncTbl.PDD_SetSensorState( m_PDDContext, ulPinId, State );
}


DWORD CCameraDevice::PDDFillPinBuffer( ULONG ulPinId, PUCHAR pImage )
{  
    return m_PDDFuncTbl.PDD_FillBuffer( m_PDDContext, ulPinId, pImage );
}


DWORD CCameraDevice::PDDInitPin( ULONG ulPinId, CPinDevice *pPin )
{   
    return m_PDDFuncTbl.PDD_InitSensorMode( m_PDDContext, ulPinId, pPin );
}

DWORD CCameraDevice::PDDSetPinFormat( ULONG ulPinId, PCS_DATARANGE_VIDEO pCsDataRangeVideo )
{
    if( NULL == pCsDataRangeVideo )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    return m_PDDFuncTbl.PDD_SetSensorModeFormat( m_PDDContext, ulPinId, pCsDataRangeVideo );
}


PVOID CCameraDevice::PDDAllocatePinBuffer( ULONG ulPinId )
{   
    return m_PDDFuncTbl.PDD_AllocateBuffer( m_PDDContext, ulPinId );
}


DWORD CCameraDevice::PDDDeAllocatePinBuffer( ULONG ulPinId, PVOID pBuffer )
{
    return m_PDDFuncTbl.PDD_DeAllocateBuffer( m_PDDContext, ulPinId, pBuffer );
}

DWORD CCameraDevice::PDDRegisterClientBuffer( ULONG ulPinId, PVOID pBuffer )
{
    return m_PDDFuncTbl.PDD_RegisterClientBuffer( m_PDDContext, ulPinId, pBuffer );
}

DWORD CCameraDevice::PDDUnRegisterClientBuffer( ULONG ulPinId, PVOID pBuffer )
{   
    return m_PDDFuncTbl.PDD_UnRegisterClientBuffer( m_PDDContext, ulPinId, pBuffer );
}

DWORD 
CCameraDevice::PDDHandlePinCustomProperties(
    ULONG ulPinId,
    PUCHAR pInBuf,
    DWORD  InBufLen,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    )
{   
    return m_PDDFuncTbl.PDD_HandleModeCustomProperties( m_PDDContext, ulPinId, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred );
}
