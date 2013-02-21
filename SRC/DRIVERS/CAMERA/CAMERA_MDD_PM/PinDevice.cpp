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

#define PININTERFACE
#include <windows.h>
#include <pm.h>
#include <Msgqueue.h>
#include <pwinbase.h>

#include "Cs.h"
#include "Csmedia.h"

#include "CameraPDDProps.h"
#include "dstruct.h"
#include "dbgsettings.h"
#include <camera.h>
#include "CameraDriver.h"
#include "PinDriver.h"
#include "wchar.h"


CPinDevice :: CPinDevice( )
{
    m_dwMemoryModel             = CSPROPERTY_BUFFER_CLIENT_LIMITED;
    m_fClientInitialized        = false;
    m_fDiscontinuity            = true;
    m_ulPinId                   = -1; // Invalid Pin Id
    m_ulMaxNumOfBuffers         = 0;
    m_ulFrameSize               = 0;
    m_ulFramesDropped           = 0;
    m_ulPictureNumber           = 0;
    m_RtAveTimePerFrame         = 0;
    m_hMsgQ                     = NULL;
    m_CsState                   = CSSTATE_STOP;
    m_msStart                   = 0xFFFFFFFF;
    m_msLastPT                  = 0;
    m_pStreamDescriptorList     = NULL;
    m_dwBufferCount             = 0;
    m_lStillCount               = 0;    

    InitializeCriticalSection( &m_csStreamBuffer );    
    InitializeCriticalSection( &m_csStreamIO );    
}

CPinDevice :: ~CPinDevice( )
{
    ResetBufferList( );
    
    if ( NULL != m_hMsgQ )
    {
        CloseMsgQueue( m_hMsgQ );
    }

    if ( NULL != m_pStreamDescriptorList )
    {
        LocalFree( m_pStreamDescriptorList );
        m_pStreamDescriptorList = NULL;
    }

    m_CsState = CSSTATE_STOP;
    DeleteCriticalSection( &m_csStreamBuffer );
    DeleteCriticalSection( &m_csStreamIO );
}

bool CPinDevice :: InitializeSubDevice( PCAMERADEVICE pCamDevice )
{
    TCHAR *tszLibraryName = NULL;
    TCHAR *tszFunctionName = NULL;
    DWORD dwDataSize = 0;
    DWORD dwDataType = 0;

    m_pCamAdapter = pCamDevice ;
    if (NULL == m_pCamAdapter)
    {
        return false ;   
    }

    return true ;
}

DWORD CPinDevice :: CloseSubDevice()
{
    DWORD dwRet = FALSE;
    dwRet = m_pCamAdapter->DecrCInstances( m_ulPinId ) ;
    if( dwRet )
    {
        dwRet = m_pCamAdapter->PDDClosePin( m_ulPinId );
    }

    return dwRet;
}


DWORD CPinDevice :: StreamInstantiate( PCSPROPERTY_STREAMEX_S pCsPropStreamEx, PUCHAR pOutBuf, DWORD  OutBufLen, PDWORD pdwBytesTransferred )
{
    DWORD   dwError  = ERROR_INVALID_PARAMETER;
    HANDLE  hProcess = NULL;
    PCS_DATARANGE_VIDEO pCsDataRangeVid = NULL;

    
    if ( -1 != m_ulPinId )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Pin %d is already instantiated.\r\n"), this, m_ulPinId )) ;
        return dwError ;
    }

    if( NULL == m_pCamAdapter )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Initialization incomplete.\r\n"), this, m_ulPinId )) ;
        return dwError;
    }

    if ( false == m_pCamAdapter->IsValidPin( pCsPropStreamEx->CsPin.PinId ) )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Invalid Pin Id\r\n"), this)) ;
        return dwError ;
    }
    
    m_ulPinId = pCsPropStreamEx->CsPin.PinId ;
    
    SENSORMODEINFO SensorModeInfo;
    if( ERROR_SUCCESS != m_pCamAdapter->PDDGetPinInfo( m_ulPinId, &SensorModeInfo ) )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Failed to retrieve sub-device information\r\n"), this)) ;
        return dwError ;
    }

    m_dwMemoryModel     = SensorModeInfo.MemoryModel;
    m_ulMaxNumOfBuffers = SensorModeInfo.MaxNumOfBuffers;
    
    // Let us set a default format for this pin

    if ( false == m_pCamAdapter->GetPinFormat( m_ulPinId, 1, &pCsDataRangeVid ) )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): No Pin Format provided for pin\r\n"), this)) ;
        return dwError ;
    }
    
    memcpy(&m_CsDataRangeVideo,pCsDataRangeVid, sizeof(CS_DATARANGE_VIDEO) ) ;

    if ( NULL == pCsPropStreamEx->hMsgQueue )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): NULL Handle provided for msgqueue\r\n"), this)) ;
        return dwError ;
    }    

    //TODO : Check whether the client created msgqueue with enough buffersize and number of buffers.

    MSGQUEUEOPTIONS msgQueueOptions;
    msgQueueOptions.bReadAccess = FALSE; // we need write-access to msgqueue
    msgQueueOptions.dwSize      = sizeof(MSGQUEUEOPTIONS);

    hProcess = OpenProcess(NULL, FALSE, GetCallerVMProcessId());
    if(NULL == hProcess)
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Failed to open Process\r\n"), this)) ;
        return dwError ;
    }

    ASSERT( m_hMsgQ == NULL );

    if ( NULL == (m_hMsgQ = OpenMsgQueue(hProcess, pCsPropStreamEx->hMsgQueue, &msgQueueOptions ) ) )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Failed to open MsgQueue\r\n"), this)) ;
        CloseHandle(hProcess);
        return dwError ;
    }

    CloseHandle(hProcess);

    if ( false == m_pCamAdapter->IncrCInstances( m_ulPinId, this ) )
    {
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Pin %d is already instantiated.\r\n"), this, m_ulPinId)) ;
        return dwError ;
    }

    return ERROR_SUCCESS ;
}

void CPinDevice :: SetState( CSSTATE CsState, CSSTATE *CsPrevState )
{
    EnterCriticalSection(&m_csStreamIO);
    if ( NULL != CsPrevState )
    {
        *CsPrevState = m_CsState ;
    }

     //Check if we are not already in the target state
    if(m_CsState != CsState)
    {
        m_CsState = CsState ;

        if ( STILL != m_ulPinId || CSSTATE_RUN != CsState )
        {
            m_pCamAdapter->PDDSetPinState( m_ulPinId, CsState );
        }

        if( STILL == m_ulPinId && CSSTATE_RUN == CsState )
        {
            m_fDiscontinuity = true;
        }
        
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("Pin: %d Setting State to 0x%X\r\n"), m_ulPinId, CsState)) ;
    }
    LeaveCriticalSection(&m_csStreamIO);
    return ;
}

BOOL CPinDevice::InitMsgQueueDescriptor (PCS_MSGQUEUE_BUFFER pCsMsgQBuff, PCS_STREAM_DESCRIPTOR pCsStreamDesc, PVOID pMappedData, PVOID pUnmappedData, BOOL bBufferFill)
{
    PCSSTREAM_HEADER pCsStreamHeader = reinterpret_cast<PCSSTREAM_HEADER>(pCsStreamDesc);
    PCS_FRAME_INFO   pCsFrameInfo    = reinterpret_cast<PCS_FRAME_INFO>(pCsStreamHeader + 1);
    //    RETAILMSG(1,(TEXT("InitMsgQueueDescriptor\n")));
    if(( pCsStreamHeader == NULL ) || ( pCsFrameInfo == NULL ))
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("InitMsgQueueDescriptor(%08x): Invalid Stream Descriptor\r\n"), this));
        return false;
    }

    if( bBufferFill )
    {
        // The buffer fill function must use the pointer that's been mapped into this process.
        //RETAILMSG(1,(TEXT("bBufferFill\n")));
        pCsStreamHeader->Data = pMappedData;

        EnterCriticalSection(&m_csStreamBuffer) ;
        pCsStreamHeader->DataUsed = m_pCamAdapter->PDDFillPinBuffer( m_ulPinId, (PUCHAR) pMappedData ) ;
        LeaveCriticalSection(&m_csStreamBuffer) ;

        pCsFrameInfo->PictureNumber = (LONGLONG)++m_ulPictureNumber;
        pCsFrameInfo->DropCount     = (LONGLONG)m_ulFramesDropped;
    }

    // The message queue requires the original pointer value.
    pCsStreamHeader->Data = pUnmappedData;

    // Init the flags to zero
    pCsStreamHeader->OptionsFlags = 0;

    // Set the discontinuity flag if frames have been previously
    // dropped, and then reset our internal flag

    if ( true == m_fDiscontinuity ) 
    {
        pCsStreamHeader->OptionsFlags |= CSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
        m_fDiscontinuity = false;
    }

    DWORD msNow = GetTickCount();

    if (m_msStart == 0xFFFFFFFF)
    {
        m_msStart = msNow;
    }

    //
    // Return the timestamp for the frame
    //
    pCsStreamHeader->PresentationTime.Numerator   = 1;
    pCsStreamHeader->PresentationTime.Denominator = 1;
    pCsStreamHeader->Duration                     = m_RtAveTimePerFrame;
    DWORD prevPT = m_msLastPT;

    m_msLastPT = msNow - m_msStart;
    pCsStreamHeader->PresentationTime.Time        = (LONGLONG) m_msLastPT * 10000;  // presentation time stamp in 100s of ns


    DEBUGMSG(ZONE_FUNCTION, (_T("InitMsgQueueDescriptor: LastPT = %d, elapsed = %d\n"), m_msLastPT, m_msLastPT - prevPT));

    // clear the timestamp valid flags
    pCsStreamHeader->OptionsFlags &= ~( CSSTREAM_HEADER_OPTIONSF_TIMEVALID | CSSTREAM_HEADER_OPTIONSF_DURATIONVALID );

    // Every frame we generate is a key frame (aka SplicePoint)
    // Delta frames (B or P) should not set this flag

    pCsStreamHeader->OptionsFlags |= CSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

    pCsMsgQBuff->CsMsgQueueHeader.Size    = sizeof(CS_MSGQUEUE_HEADER);
    pCsMsgQBuff->CsMsgQueueHeader.Flags   = FLAG_MSGQ_FRAME_BUFFER;
    pCsMsgQBuff->CsMsgQueueHeader.Context = NULL;

    //Get the unmarshalled StreamDescriptor in order to send the message to the application
    pCsMsgQBuff->pStreamDescriptor = NULL;
    for (UINT ii = 0; ii < m_dwBufferCount; ii++)
    {
        if(m_pStreamDescriptorList[ii].csStreamDescriptorShadow.CsStreamHeader.Data == pUnmappedData)
        {
            pCsMsgQBuff->pStreamDescriptor = m_pStreamDescriptorList[ii].m_pUnMarshalledStreamDesc;
            break;
        }
    }

    if(NULL == pCsMsgQBuff->pStreamDescriptor)
    {
        DEBUGMSG(ZONE_FUNCTION, (_T("InitMsgQueueDescriptor(%08x): Unable to find Unmarshalled Stream Desc\n"), this));
        return FALSE;
    }
    
    DEBUGMSG(ZONE_FUNCTION, (_T("InitMsgQueueDescriptor(%08x): Frame buf queued: %d (dropped %d), start %d, time %d\n"), 
        this,
        (LONG)pCsFrameInfo->PictureNumber, 
        (LONG)pCsFrameInfo->DropCount, 
        (LONG)m_msStart,
        (LONG)(pCsStreamHeader->PresentationTime.Time / 10000)));

    return TRUE;
   
}

void  CPinDevice::FlushBufferQueue()
{
    PCS_STREAM_DESCRIPTOR pCsStreamDesc = NULL;
    PVOID                 pMappedData   = NULL;
    PVOID                 pUnmappedData = NULL;
    CS_MSGQUEUE_BUFFER    CsMsgQBuff ;    

    while (( true == RemoveBufferFromList( &pCsStreamDesc, &pMappedData, &pUnmappedData )) && ( NULL != pCsStreamDesc ) && ( m_hMsgQ != NULL ))
    {
        if (!InitMsgQueueDescriptor (&CsMsgQBuff, pCsStreamDesc, pMappedData, pUnmappedData, FALSE))
        {
            continue;
        }

        if ( false == WriteMsgQueue( m_hMsgQ, reinterpret_cast<LPVOID>(&CsMsgQBuff),  sizeof(CS_MSGQUEUE_BUFFER), PIN_TIMEOUT, 0 ) )
        {
            DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): WriteMsgQueue returned false\r\n"), this));
        }
    }    

    return;
}

DWORD CPinDevice :: HandlePinIO()
{    
    DWORD dwRet = ERROR_SUCCESS;
    BOOL bOK = TRUE;

    EnterCriticalSection(&m_csStreamIO);
    if ( CSSTATE_RUN != m_CsState ) 
    {
        LeaveCriticalSection(&m_csStreamIO);
        return ERROR_INVALID_STATE;
    }
       //RETAILMSG(1,(TEXT("HandlePinIO\n")));
    PCS_STREAM_DESCRIPTOR pCsStreamDesc = NULL;
    PVOID                 pMappedData   = NULL;
    PVOID                 pUnmappedData = NULL;
    CS_MSGQUEUE_BUFFER    CsMsgQBuff ;

    if ( false == RemoveBufferFromList( &pCsStreamDesc, &pMappedData, &pUnmappedData ) || NULL == pCsStreamDesc )
    {
        // We dropped a frame
        m_ulFramesDropped++;
        m_fDiscontinuity = true;
        bOK = FALSE;
        dwRet = ERROR_OUTOFMEMORY;
        goto exit;
    }

    if (!InitMsgQueueDescriptor (&CsMsgQBuff, pCsStreamDesc, pMappedData, pUnmappedData, TRUE))
    {        
        bOK = FALSE;
        dwRet = ERROR_INVALID_DATA;
        goto exit;
    }

    if ( NULL == m_hMsgQ )
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): MsgQueue is not opened\r\n"), this)) ;         
        bOK = FALSE;
        goto exit;
    }

    if ( false == WriteMsgQueue( m_hMsgQ, reinterpret_cast<LPVOID>(&CsMsgQBuff),  sizeof(CS_MSGQUEUE_BUFFER), PIN_TIMEOUT, 0 ) )
    {
        DEBUGMSG(ZONE_FUNCTION|ZONE_ERROR, (_T("PIN_Function(%08x): WriteMsgQueue returned false\r\n"), this));
        dwRet = ERROR_WRITE_FAULT;
    }

exit:
    
    if ( STILL == m_ulPinId )
    {
        if((bOK) && (0 == DecrementStillCount()))
        {
            m_CsState = CSSTATE_PAUSE;
        }
    }
    LeaveCriticalSection(&m_csStreamIO);
    return dwRet;
}

DWORD CPinDevice ::PauseStream( )
{

    if( m_CsState == CSSTATE_STOP )
    {
        // Let's allocate our resources
        if( m_pStreamDescriptorList == NULL )
        {
            m_pStreamDescriptorList = (PCS_STREAM_DESCRIPTOR_SHADOW) LocalAlloc( LMEM_ZEROINIT, sizeof( CS_STREAM_DESCRIPTOR_SHADOW ) * m_ulMaxNumOfBuffers );
            if( NULL == m_pStreamDescriptorList )
                return ERROR_OUTOFMEMORY;
        }

//        m_dwBufferCount = 0;
    }

    if ( false == m_fClientInitialized )
    {
        // By this time the buffers must be allocated
        if( ERROR_SUCCESS == m_pCamAdapter->PDDInitPin( m_ulPinId, this ) )
        {
            m_fClientInitialized = true;
        }
    }

    if( m_fClientInitialized == false )
    {
        return ERROR_INTERNAL_ERROR;
    }

    m_CsState    = CSSTATE_PAUSE ;
    m_pCamAdapter->PDDSetPinState( m_ulPinId, m_CsState );

    return ERROR_SUCCESS ;
}


DWORD CPinDevice :: PinHandleConnectionRequests( 
        PCSPROPERTY pCsProp, 
        PUCHAR pOutBuf,                 // Unsafe, use with caution
        DWORD  OutBufLen, 
        PDWORD pdwBytesTransferred      // Unsafe, use with caution
        )
{
    DEBUGMSG( ZONE_IOCTL, (_T("PIN_IOControl(%08x): PinHandleConnectionRequests\r\n"), this));
    
    DWORD                           dwError                 = ERROR_INVALID_PARAMETER; 
    PCSALLOCATOR_FRAMING            pCsAllocatorFraming     = NULL;
    PCS_DATAFORMAT_VIDEOINFOHEADER  pCsDataFormatVidInfoHdr = NULL;
    PCS_DATAFORMAT_VIDEOINFOHEADER  pCsDataFormatVidInfoHdrCopy = NULL;
    
    if ( NULL == pCsProp )
    {
        return dwError;
    }
    
    __try
    {
        *pdwBytesTransferred = 0 ;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // we support PROPSETID_Pin, so just return success
    if ( CSPROPERTY_TYPE_SETSUPPORT == pCsProp->Flags )
    {
        return ERROR_SUCCESS;
    }

    switch ( pCsProp->Id )
    {
    case CSPROPERTY_CONNECTION_STATE:
        dwError = PinHandleConnStateSubReqs( pCsProp->Flags, pOutBuf, OutBufLen, pdwBytesTransferred ) ;
        break ;

    case CSPROPERTY_CONNECTION_DATAFORMAT:
        
        pCsDataFormatVidInfoHdr = (PCS_DATAFORMAT_VIDEOINFOHEADER) pOutBuf;  
        DWORD dwStructSize;
        if(( OutBufLen < sizeof( PCS_DATAFORMAT_VIDEOINFOHEADER )) || ( pOutBuf == NULL ))
        {
            return dwError;
        }
            
        // The video info header can be modified by the caller while it's being accessed in the subroutine.
        // The Subroutine needs to make a copy of the video info header before accessing it.

        __try
        {
            dwStructSize = sizeof( CS_DATAFORMAT_VIDEOINFOHEADER ) + pCsDataFormatVidInfoHdr->VideoInfoHeader.bmiHeader.biSize - sizeof( CS_BITMAPINFOHEADER );
            pCsDataFormatVidInfoHdrCopy = (PCS_DATAFORMAT_VIDEOINFOHEADER) LocalAlloc( LMEM_ZEROINIT, dwStructSize );
            if( pCsDataFormatVidInfoHdrCopy == NULL )
            {
                return ERROR_INVALID_PARAMETER;
            }

            if( CeSafeCopyMemory( pCsDataFormatVidInfoHdrCopy, pCsDataFormatVidInfoHdr, dwStructSize ))
            {  
                dwError = PinHandleConnDataFormatSubReqs( pCsProp->Flags, pCsDataFormatVidInfoHdrCopy, pdwBytesTransferred ) ;
            }
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            dwError = ERROR_INVALID_PARAMETER;
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): CSPROPERTY_CONNECTION_DATAFORMAT: Exception occured\r\n"), this)) ;
        }

        LocalFree( pCsDataFormatVidInfoHdrCopy );
        break ;
    
    case CSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        switch ( pCsProp->Flags )
        {
        case CSPROPERTY_TYPE_GET:
        case CSPROPERTY_TYPE_BASICSUPPORT:
            CSALLOCATOR_FRAMING csAllocatorFraming;
            
            if(( OutBufLen < sizeof( CSALLOCATOR_FRAMING )) || ( pOutBuf == NULL ))
            {
                dwError = ERROR_MORE_DATA;
                break;
            }

            {
                csAllocatorFraming.RequirementsFlags = m_dwMemoryModel;
                csAllocatorFraming.PoolType          = PagedPool;
                csAllocatorFraming.Frames            = m_ulMaxNumOfBuffers;
                csAllocatorFraming.FrameSize         = m_CsDataRangeVideo.VideoInfoHeader.bmiHeader.biSizeImage;
                csAllocatorFraming.FileAlignment     = FILE_BYTE_ALIGNMENT;
                csAllocatorFraming.Reserved          = 0;
            }
            
            __try
            {
                memcpy( pOutBuf, &csAllocatorFraming, sizeof( CSALLOCATOR_FRAMING ));
                *pdwBytesTransferred = sizeof( CSALLOCATOR_FRAMING );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            dwError = ERROR_SUCCESS;
            break ;
        
        case CSPROPERTY_TYPE_SET:
            
            if( OutBufLen < sizeof( CSALLOCATOR_FRAMING ))
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }

            pCsAllocatorFraming = (PCSALLOCATOR_FRAMING) pOutBuf;
            if(( m_CsState != CSSTATE_STOP ) || ( m_dwMemoryModel != pCsAllocatorFraming->RequirementsFlags ))
            {
                dwError = ERROR_INVALID_PARAMETER;
                break;
            }
            m_ulMaxNumOfBuffers = pCsAllocatorFraming->Frames ;
            dwError = ERROR_SUCCESS;
            break ;
        
        default :
            DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): CSPROPERTY_CONNECTION_ALLOCATORFRAMING Invalid Request\r\n"), this)) ;
        }
        
        break ;
    
    case CSPROPERTY_CONNECTION_PROPOSEDATAFORMAT :
        // I don't want to support dynamic format changes for this test driver
        break ;
    
    default :
        DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl(%08x): Invalid Request\r\n"), this)) ;

    }
    
    return dwError ;
}

DWORD
CPinDevice::PinHandleBufferRequest(
    CSBUFFER_INFO  csBufferInfo,
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    DWORD *pdwBytesTransferred
    )
{
    DWORD   dwError = ERROR_INVALID_PARAMETER;
    DWORD dwCommand = csBufferInfo.dwCommand;
    PCS_STREAM_DESCRIPTOR pCsDescriptor = (PCS_STREAM_DESCRIPTOR) csBufferInfo.pStreamDescriptor;

    if ( pdwBytesTransferred )
    {
        *pdwBytesTransferred = 0;
    }

    // The pOutBuf argument has already been probed with MapCallerPointer, and the upper layer 
    // has already checked for the size of the buffer to be at least sizeof( CS_STREAM_DESCRIPTOR )

    switch( dwCommand )
    {
        case CS_ALLOCATE:
            // Let's allocate our resources
            if( m_pStreamDescriptorList == NULL )
            {
                m_pStreamDescriptorList = (PCS_STREAM_DESCRIPTOR_SHADOW) LocalAlloc( LMEM_ZEROINIT, sizeof( CS_STREAM_DESCRIPTOR_SHADOW ) * m_ulMaxNumOfBuffers );
                if( NULL == m_pStreamDescriptorList )
                    return ERROR_OUTOFMEMORY;
            }


            dwError = AllocateBuffer( csBufferInfo.pStreamDescriptor, pOutBuf, OutBufLen, pdwBytesTransferred );
            break;

        case CS_ENQUEUE:
            dwError = EnqueueDescriptor( csBufferInfo.pStreamDescriptor );
            break;

        case CS_DEALLOCATE:
            dwError = DeallocateBuffer( csBufferInfo.pStreamDescriptor );

        default:
            break;
    }

    return dwError;
}


DWORD
CPinDevice::PinHandleConnStateSubReqs(
    ULONG  ulReqFlags,
    PUCHAR pOutBuf,                 // Unsafe, use with caution
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    )
{
    DWORD    dwError  = ERROR_INVALID_PARAMETER;
    PCSSTATE pCsState = NULL;
    
    switch( ulReqFlags )
    {
    case CSPROPERTY_TYPE_GET:
        if( OutBufLen < sizeof ( CSSTATE ) )
        {
            dwError = ERROR_MORE_DATA;
            break;
        }
        
        if( NULL == pOutBuf )
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }

        __try
        {
            memcpy( pOutBuf, &m_CsState, sizeof ( CSSTATE ) );
            *pdwBytesTransferred = sizeof ( CSSTATE );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )        
        {
            dwError = ERROR_MORE_DATA;
            break;
        }
        
        dwError = ERROR_SUCCESS;
        break;
    
    case CSPROPERTY_TYPE_SET:

        CSSTATE csState;
        if( OutBufLen < sizeof( CSSTATE ))
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        if( NULL == pOutBuf )
        {
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }

        if( !CeSafeCopyMemory( &csState, pOutBuf, sizeof( CSSTATE )))
        {
            dwError = ERROR_MORE_DATA;
            break;
        }

        if( csState == m_CsState )
        { 
            dwError = ERROR_SUCCESS;
            break;
        }

        EnterCriticalSection(&m_csStreamIO);
        switch ( csState )
        {
        case CSSTATE_STOP:

            m_ulPictureNumber = 0;
            m_ulFramesDropped = 0;
            m_msLastPT        = 0;

            // We can get to the CSSTATE_STOP state from any other state.
            if ( CSSTATE_STOP == m_CsState ) 
            {
                DEBUGMSG( ZONE_IOCTL, ( _T("PIN_IOControl(%08x): State to set = CSSTATE_STOP but we are already Stopped.\r\n"), this ) );
                dwError = ERROR_SUCCESS;
                break;
            }
        
            m_CsState = CSSTATE_STOP;
            m_pCamAdapter->PDDSetPinState( m_ulPinId, m_CsState );

            // The buffer queue needs to be emptied if the driver is not allocating the buffers
            FlushBufferQueue();
            dwError = ERROR_SUCCESS;

            break;

        case CSSTATE_PAUSE:
            
            if ( CSSTATE_PAUSE == m_CsState ) 
            {
                DEBUGMSG( ZONE_IOCTL, ( _T("PIN_IOControl(%08x): State to set = CSSTATE_PAUSE but we are already Paused.\r\n"), this ) );
                dwError = ERROR_SUCCESS;
                break;
            }

            dwError = PauseStream();
            break;

        case CSSTATE_RUN:
            
            if ( CSSTATE_STOP == m_CsState ) 
            {
                DEBUGMSG( ZONE_IOCTL, ( _T("PIN_IOControl(%08x): CSSTATE_STOP to CSSTATE_RUN is not a supported transition .\r\n"), this ) );
                dwError = ERROR_INVALID_STATE;
                break;
            }

            // We only allow Still Pin to goto Run state through PROPSETID_VIDCAP_VIDEOCONTROL
            if ( STILL == m_ulPinId )
            {
                dwError = ERROR_SUCCESS;
                break;
            }

            m_CsState = CSSTATE_RUN;          
            m_msStart = 0xFFFFFFFF;
            m_pCamAdapter->PDDSetPinState( m_ulPinId, m_CsState );


            dwError = ERROR_SUCCESS;
            
            break;

        default :
            DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("PIN_IOControl(%08x): Incorrect State\r\n"), this ) );
            dwError = ERROR_INVALID_PARAMETER;
        }
        LeaveCriticalSection(&m_csStreamIO);
        break;

    default:
        DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("PIN_IOControl(%08x): Invalid Request\r\n"), this ) );

        break;
    }

    return dwError;
}


DWORD
CPinDevice::PinHandleCustomRequests(
    PUCHAR pInBuf,              // Warning: This is an unsafe buffer, access with care
    DWORD  InBufLen,
    PUCHAR pOutBuf,             // Warning: This is an unsafe buffer, access with care
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred  // Warning: This is an unsafe buffer, access with care
    )
{
    return m_pCamAdapter->PDDHandlePinCustomProperties( m_ulPinId, pInBuf, InBufLen, pOutBuf, OutBufLen, pdwBytesTransferred );
}

DWORD
CPinDevice::PinHandleConnDataFormatSubReqs(
    ULONG                          ulReqFlags,
    PCS_DATAFORMAT_VIDEOINFOHEADER pCsDataFormatVidInfoHdr,  // Warning: this buffer is unsafe, use with caution
    PDWORD                         pdwBytesTransferred
    )
{
    DWORD    dwError  = ERROR_INVALID_PARAMETER;
    PCSSTATE pCsState = NULL;

    PCS_DATARANGE_VIDEO pCsDataRangeVideoMatched = NULL;

    // We must have called IOCTL_STREAM_INSTANTIATE before setting format
    if ( -1 == m_ulPinId )
    {
        return dwError;
    }
    
    // The incoming video info header is unsafe. The data might change on a separate thread
    // while it's being accessed. For security purposes, let's make a copy of the data
    // before any attempt to access them is done, and then work off the copy

    switch( ulReqFlags )
    {
    case CSPROPERTY_TYPE_SET:
        if ( true == m_pCamAdapter->AdapterCompareFormat( m_ulPinId, pCsDataFormatVidInfoHdr, &pCsDataRangeVideoMatched, true ) )
        {
            // We found our format
            memcpy( &m_CsDataRangeVideo, pCsDataRangeVideoMatched, sizeof ( CS_DATARANGE_VIDEO ) );
            memcpy( &m_CsDataRangeVideo, &pCsDataFormatVidInfoHdr->DataFormat, sizeof ( CSDATARANGE ) );
            memcpy( &m_CsDataRangeVideo.VideoInfoHeader, &pCsDataFormatVidInfoHdr->VideoInfoHeader, sizeof ( CS_VIDEOINFOHEADER ) );

            m_RtAveTimePerFrame = m_CsDataRangeVideo.VideoInfoHeader.AvgTimePerFrame;
            
            dwError = m_pCamAdapter->PDDSetPinFormat( m_ulPinId, &m_CsDataRangeVideo );
            *pdwBytesTransferred = 0;
        }

        break;

    default:
        DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("PIN_IOControl(%08x): Invalid Request\r\n"), this ) );
    }

    return dwError;
}


DWORD 
CPinDevice::AllocateBuffer( PCS_STREAM_DESCRIPTOR pCsDescriptor, LPVOID pOutBuf, DWORD  OutBufLen, DWORD *pdwBytesTransferred )
{
    DWORD                   dwError = ERROR_INVALID_PARAMETER;
    DWORD                   dwResult = -1;
    PCS_STREAM_DESCRIPTOR   pCsDescriptorOut = (PCS_STREAM_DESCRIPTOR) pOutBuf;

    MarshalledBuffer_t MarshalledStreamDesc(pCsDescriptor, sizeof(CS_STREAM_DESCRIPTOR), ARG_O_PTR, FALSE, TRUE);
    pCsDescriptor = reinterpret_cast<PCS_STREAM_DESCRIPTOR>( MarshalledStreamDesc.ptr() );

    if( NULL == pCsDescriptor )
    {
        return dwError;
    }

    // There are 2 cases here: the buffer comes from the hardware or from the software. 
    // If the buffer comes from the software, we generate a new entry in the table up to the maximum allowed.
    // If the buffer comes from the hardware, we setup the application stream descriptor

    EnterCriticalSection( &m_csStreamBuffer );

    //Check the BufferCount after the critical section is entered.
    //This prevents synch issues with validating the BufferCount
    if( m_dwBufferCount >= m_ulMaxNumOfBuffers )
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if( m_dwMemoryModel == CSPROPERTY_BUFFER_DRIVER )
    {
        if( NULL == pOutBuf || OutBufLen < sizeof(CS_STREAM_DESCRIPTOR) )
        {
            goto Cleanup;
        }
        // pOutBuf has already been validated through MapCallerPtr in the IOCTL function
        if( pCsDescriptorOut == NULL )
        {
            goto Cleanup;
        }

        // Get one of the hardware buffers, and setup the descriptor
        ASSERT( m_pStreamDescriptorList[ m_dwBufferCount ].m_fBusy == FALSE );

        dwError = HwSetupStreamDescriptor( m_dwBufferCount );
        if( dwError != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
            
        if( !CeSafeCopyMemory( pCsDescriptorOut, &(m_pStreamDescriptorList[ m_dwBufferCount ].csStreamDescriptorShadow), sizeof( CS_STREAM_DESCRIPTOR )))
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        m_pStreamDescriptorList[ m_dwBufferCount ].pCsStreamDescriptorExternal = NULL;
        m_pStreamDescriptorList[ m_dwBufferCount ].m_fBusy = TRUE;
        m_dwBufferCount++;
        if( pdwBytesTransferred )
        {
            *pdwBytesTransferred = sizeof(CS_STREAM_DESCRIPTOR);
        }
    }
    else if( m_dwMemoryModel == CSPROPERTY_BUFFER_CLIENT_LIMITED )
    {
        // The software is allocated by the software, let's copy the descriptor and generate a handle
        ASSERT( m_pStreamDescriptorList[ m_dwBufferCount ].m_fBusy == FALSE );
        //RETAILMSG(1,(TEXT("AllocBuffer:CeSafeCopyMemory\n")));
        if( !CeSafeCopyMemory( &( m_pStreamDescriptorList[ m_dwBufferCount ].csStreamDescriptorShadow ), pCsDescriptor, sizeof( CS_STREAM_DESCRIPTOR )))
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        //RETAILMSG(1,(TEXT("AllocBuffer:SwSetupStreamDescriptor\n")));
        // Let's populate the handle and the buffer field.
        dwError = SwSetupStreamDescriptor( m_dwBufferCount, &( m_pStreamDescriptorList[ m_dwBufferCount ].csStreamDescriptorShadow ), pCsDescriptor );
        if( dwError != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
        //RETAILMSG(1,(TEXT("AllocBuffer:OKay!!!\n")));
        m_pStreamDescriptorList[ m_dwBufferCount ].pCsStreamDescriptorExternal = NULL;
        m_pStreamDescriptorList[ m_dwBufferCount ].m_fBusy = TRUE;
        m_dwBufferCount++;
    }
    else if( m_dwMemoryModel == CSPROPERTY_BUFFER_CLIENT_UNLIMITED )
    {
        // let's find a slot available
        DWORD dwAvailableRow = -1;
        for( DWORD i = 0; ( i < m_ulMaxNumOfBuffers ) && ( dwAvailableRow == -1 ); i++ )
        {
            if( m_pStreamDescriptorList[ i ].m_fBusy == FALSE )
            {
                dwAvailableRow = i;
            }
        }
        if( dwAvailableRow == -1 )
        {
            goto Cleanup;
        }

        if( !CeSafeCopyMemory( &( m_pStreamDescriptorList[ dwAvailableRow ].csStreamDescriptorShadow ), pCsDescriptor, sizeof( CS_STREAM_DESCRIPTOR )))
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        dwError = SwSetupStreamDescriptor( dwAvailableRow, &( m_pStreamDescriptorList[ dwAvailableRow ].csStreamDescriptorShadow ), pCsDescriptor );
        if( dwError != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        m_pStreamDescriptorList[ dwAvailableRow ].pCsStreamDescriptorExternal = NULL;
        m_pStreamDescriptorList[ dwAvailableRow ].m_fBusy = TRUE;
        m_dwBufferCount++;
    }

Cleanup:
    LeaveCriticalSection( &m_csStreamBuffer );
    return dwError;
}


DWORD
CPinDevice::DeallocateBuffer( PCS_STREAM_DESCRIPTOR pCsDescriptor )
{
    LPVOID  lpBuffer;
    DWORD   dwHandle;
    LONG    lIndex;
    DWORD   dwError = ERROR_SUCCESS;    

    MarshalledBuffer_t MarshalledStreamDesc(pCsDescriptor, sizeof(CS_STREAM_DESCRIPTOR), ARG_O_PTR, FALSE, TRUE);
    pCsDescriptor = reinterpret_cast<PCS_STREAM_DESCRIPTOR>( MarshalledStreamDesc.ptr() );

    if( NULL == pCsDescriptor )
    {
        return dwError;
    }

    lpBuffer = pCsDescriptor->CsStreamHeader.Data;
    dwHandle = pCsDescriptor->CsStreamHeader.Handle;

    // Get the entry for this buffer in the internal list
    EnterCriticalSection( &m_csStreamBuffer );
    
    //Check if there are any buffers to deallocate    
    if(0 == m_dwBufferCount)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    lIndex = GetIndexFromHandle( dwHandle, lpBuffer );
    if( lIndex == -1 )
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // If the row is not in use, let's make it available
    if( m_pStreamDescriptorList[ lIndex ].pCsStreamDescriptorExternal != NULL )
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    ASSERT( m_pStreamDescriptorList[ lIndex ].m_fBusy == TRUE );
    m_pStreamDescriptorList[ lIndex ].m_fBusy = FALSE;
    m_dwBufferCount--;

    if( m_dwMemoryModel == CSPROPERTY_BUFFER_DRIVER )
    {
        // We release the buffer. 
        dwError = m_pCamAdapter->PDDDeAllocatePinBuffer( m_ulPinId, m_pStreamDescriptorList[ lIndex ].csStreamDescriptorShadow.CsStreamHeader.Data );
        m_pStreamDescriptorList[ lIndex ].csStreamDescriptorShadow.CsStreamHeader.Data = NULL;
        pCsDescriptor->CsStreamHeader.Data = NULL;
    }
    else
    {
        dwError = m_pCamAdapter->PDDUnRegisterClientBuffer( m_ulPinId, m_pStreamDescriptorList[ lIndex ].csStreamDescriptorShadow.CsStreamHeader.Data );

        m_pStreamDescriptorList[ lIndex ].csStreamDescriptorShadow.CsStreamHeader.Data = NULL;

        pCsDescriptor->CsStreamHeader.Data = NULL;
    }

Cleanup:
    LeaveCriticalSection( &m_csStreamBuffer );
    return dwError;
}


DWORD
CPinDevice::SwSetupStreamDescriptor( 
    DWORD                   dwIndex,
    PCS_STREAM_DESCRIPTOR   pCsStreamDesc, 
    LPVOID                  pBuffer             // Warning: This is an unsafe buffer, use with caution
)
{
    DWORD dwHandle;
    PCS_STREAM_DESCRIPTOR pCsStreamDescExt = ( PCS_STREAM_DESCRIPTOR ) pBuffer;

    if(( pCsStreamDesc == NULL ) || ( pBuffer == NULL ))
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwHandle = CreateHandle( dwIndex, pBuffer );

    __try
    {
        pCsStreamDescExt->CsStreamHeader.Handle = dwHandle; 
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Note: This is the place to setup DMA for the buffer. 

    pCsStreamDesc->CsStreamHeader.Handle = dwHandle; 
    
    return m_pCamAdapter->PDDRegisterClientBuffer( m_ulPinId, pCsStreamDesc->CsStreamHeader.Data );    
}


DWORD
CPinDevice::HwSetupStreamDescriptor(
    DWORD   dwIndex
)
{
    PCSSTREAM_HEADER      pCsStreamHeader; 
    PCS_FRAME_INFO        pCsFrameInfo;

    if( dwIndex > m_dwBufferCount )
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_ulFrameSize = CS__DIBSIZE (m_CsDataRangeVideo.VideoInfoHeader.bmiHeader);

    pCsStreamHeader = &( m_pStreamDescriptorList[ dwIndex ].csStreamDescriptorShadow.CsStreamHeader );
    pCsFrameInfo = &( m_pStreamDescriptorList[ dwIndex ].csStreamDescriptorShadow.CsFrameInfo );

    pCsStreamHeader->Size                         = sizeof(CSSTREAM_HEADER);
    pCsStreamHeader->TypeSpecificFlags            = 0;
    pCsStreamHeader->PresentationTime.Time        = 0;
    pCsStreamHeader->PresentationTime.Numerator   = 1;
    pCsStreamHeader->PresentationTime.Denominator = 1;
    pCsStreamHeader->Duration                     = 0;
    pCsStreamHeader->FrameExtent                  = m_ulFrameSize;
    pCsStreamHeader->DataUsed                     = m_ulFrameSize;
    pCsStreamHeader->OptionsFlags                 = CSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;

    pCsFrameInfo->ExtendedHeaderSize = sizeof(CS_FRAME_INFO);
    pCsFrameInfo->dwFrameFlags       = CS_VIDEO_FLAG_FRAME;
    pCsFrameInfo->PictureNumber      = 0; 
    pCsFrameInfo->DropCount          = 0;

    // Note: RemoteLocalAlloc can't really trigger an exception, the __try/__except block is here
    // to highlight the fact that this call has to be protected in the case of a hardware access.
    __try
    {
        pCsStreamHeader->Data = m_pCamAdapter->PDDAllocatePinBuffer( m_ulPinId );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return ERROR_INTERNAL_ERROR;
    }
    
    if( NULL == pCsStreamHeader->Data )
    {
        return ERROR_OUTOFMEMORY;
    }

    // And setup the handle
    pCsStreamHeader->Handle = CreateHandle( dwIndex, pCsStreamHeader->Data );

    return ERROR_SUCCESS;
}


DWORD 
CPinDevice::CreateHandle( 
    DWORD  dwIndex, 
    LPVOID pBuffer 
)
{
    DWORD  dwHandle;
    DWORD dwProcessId = GetCallerVMProcessId();

    dwHandle = dwProcessId ^ (( dwIndex << 16 ) + ( (DWORD)pBuffer & 0xFFFF ));

    return dwHandle;
}


LONG
CPinDevice::GetIndexFromHandle( 
    DWORD  dwHandle, 
    LPVOID pBuffer      // Warning: This is an unsafe buffer, use with caution
)
{
    LONG   lIndex = -1;    
    DWORD dwProcessId = GetCallerVMProcessId();

    // let's retrieve the index from the handle table and make sure we have a match    
    lIndex = ( dwHandle ^ dwProcessId ) >> 16;
    if( lIndex >= (LONG)m_ulMaxNumOfBuffers || lIndex < 0 )
    {
        // Invalid index, bail out
        return -1;
    }

    if(   ( m_pStreamDescriptorList[ lIndex ].csStreamDescriptorShadow.CsStreamHeader.Data   != pBuffer )
        ||( m_pStreamDescriptorList[ lIndex ].csStreamDescriptorShadow.CsStreamHeader.Handle != dwHandle ))
    {
        // Something's wrong, bail out
        return -1;
    }

    return lIndex;
}


DWORD
CPinDevice::EnqueueDescriptor( PCS_STREAM_DESCRIPTOR pUnMarshalCsDescriptor )
{
    LPVOID  lpBuffer, lpMappedBuffer;
    DWORD   dwHandle;
    LONG   lIndex;
    DWORD   dwSize;
    DWORD   dwError = ERROR_INVALID_PARAMETER;
    PCS_VIDEOINFOHEADER   pCsVideoInfoHdr;
    PCS_STREAM_DESCRIPTOR pCsDescriptor = NULL;
    MarshalledBuffer_t  *pMarshalledStreamDesc = NULL;
    
    DEBUGMSG( ZONE_IOCTL, ( _T("PIN_IOControl(%08x): EnqueueDescriptor\r\n"), this ) );
       // RETAILMSG( 1, ( _T("PIN_IOControl(%08x): EnqueueDescriptor\r\n"), this ) );
    if( m_CsState == CSSTATE_STOP )
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }
       // RETAILMSG( 1, ( _T("EnqueueDescriptor:Go On 0\r\n")) );
    {
        MarshalledBuffer_t MarshalledStreamDesc(pUnMarshalCsDescriptor, sizeof(CS_STREAM_DESCRIPTOR), ARG_O_PTR, FALSE, TRUE);
        pCsDescriptor = reinterpret_cast<PCS_STREAM_DESCRIPTOR>( MarshalledStreamDesc.ptr() );

        if( NULL == pCsDescriptor )
        {
            return dwError;
        }

        // First, let's use the handle and the buffer to retrieve the shadow copy
        // If an exception happens during the following 2 lines, it will be trapped by the upper level
        lpBuffer = pCsDescriptor->CsStreamHeader.Data;
        dwHandle = pCsDescriptor->CsStreamHeader.Handle;
    }
        //    RETAILMSG( 1, ( _T("EnqueueDescriptor:EnterCriticalSection( &m_csStreamBuffer )\r\n")) );
    EnterCriticalSection( &m_csStreamBuffer );

    // Get the entry for this buffer in the internal list
    lIndex = GetIndexFromHandle( dwHandle, lpBuffer );
    if( lIndex == -1 )
    {
        goto Cleanup;
    }

    // Is the row in use?
    if( m_pStreamDescriptorList[ lIndex ].m_fBusy == FALSE )
    {
        DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("PIN_IOControl(%08x): The buffer has not be prepared. Call CS_ALLOCATE first.\r\n"), this ) );
        goto Cleanup;
    }

    if( m_pStreamDescriptorList[ lIndex ].pCsStreamDescriptorExternal != NULL )
    {
        DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("PIN_IOControl(%08x): This buffer has already be enqueued.\r\n"), this ) );
        goto Cleanup;
    }
          //  RETAILMSG( 1, ( _T("EnqueueDescriptor:Go On 1\r\n")) );
    // Now, let's probe the incoming buffer
    pCsVideoInfoHdr = reinterpret_cast<PCS_VIDEOINFOHEADER>( &m_CsDataRangeVideo.VideoInfoHeader );
    dwSize = m_CsDataRangeVideo.VideoInfoHeader.bmiHeader.biSizeImage;

    if( m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer != NULL &&
        NULL != m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer->ptr() )
    {
        m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer->Unmarshal();
    }
    if( m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc != NULL &&
        NULL != m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc->ptr() )
    {
        m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc->Unmarshal();
    }

    if( NULL == m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc )
    {
        m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc = new MarshalledBuffer_t();
        if( NULL == m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc )
        {
            dwError = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
    }

         //   RETAILMSG( 1, ( _T("EnqueueDescriptor:Go On 2\r\n")) );
    m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc->Marshal(pUnMarshalCsDescriptor, sizeof(CS_STREAM_DESCRIPTOR), ARG_O_PTR|MARSHAL_FORCE_ALIAS, FALSE, TRUE);
    pCsDescriptor = reinterpret_cast<PCS_STREAM_DESCRIPTOR>( m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc->ptr() );

    if( NULL == pCsDescriptor )
    {
        goto Cleanup;
    }

    //Marshal the data buffer
    if( NULL == m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer )
    {
        m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer = new MarshalledBuffer_t();
        if( NULL == m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer )
        {
            dwError = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }
    }
        //    RETAILMSG( 1, ( _T("EnqueueDescriptor:Go On 3\r\n")) );
    if(FAILED(m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer->Marshal(lpBuffer, //Unmarshalled Src
                                                                                  dwSize, //Size of buffer
                                                                                  ARG_O_PTR|MARSHAL_FORCE_ALIAS, //Pointer is output
                                                                                  FALSE, //Don't force Duplicate
                                                                                  TRUE) //Enable asynch access
      )) 
    {
        DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("PIN_IOControl(%08x): Unable to marshal data buffer for asynch access.\r\n"), this ) );
        dwError = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    //        RETAILMSG( 1, ( _T("EnqueueDescriptor:Go On 4\r\n")) );
    lpMappedBuffer = m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer->ptr();

    m_pStreamDescriptorList[ lIndex ].csStreamDescriptorShadow.CsStreamHeader.Data = lpMappedBuffer;
    m_pStreamDescriptorList[ lIndex ].pCsStreamDescriptorExternal = pCsDescriptor;

    m_pStreamDescriptorList[ lIndex ].m_pUnMarshalledStreamDesc = (PCS_STREAM_DESCRIPTOR)pUnMarshalCsDescriptor;

    if ( false == m_fClientInitialized && CSSTATE_PAUSE == m_CsState )
    {
        m_fClientInitialized = true;
    }

    dwError = ERROR_SUCCESS;
    
Cleanup:

    if( ERROR_SUCCESS != dwError && lIndex >= 0 )
    {
        if( NULL != m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer &&
            NULL != m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer->ptr() )
        {
            m_pStreamDescriptorList[ lIndex ].m_pMarshalledDataBuffer->Unmarshal();
        }
        if( NULL != m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc &&
            NULL != m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc->ptr() )
        {
            m_pStreamDescriptorList[ lIndex ].m_pMarshalledStreamDesc->Unmarshal();
        }
    }

    LeaveCriticalSection( &m_csStreamBuffer );
    return dwError;
}


bool
CPinDevice::RemoveBufferFromList(
    PCS_STREAM_DESCRIPTOR * ppCsStreamDesc,
    PVOID                 * ppMappedData,
    PVOID                 * ppUnmappedData
    )
{
    DWORD dwCounter = 0;
    bool  RetVal = true;

    // Let's look in the list of buffers for the first buffer that has a non null external stream descriptor
    DEBUGMSG( ZONE_IOCTL, ( _T("PIN_IOControl(%08x): RemoveBufferFromList\r\n"), this ) );
   // RETAILMSG( 1, ( _T("PIN_IOControl(%08x): RemoveBufferFromList\r\n"), this ) );
    if(( ppCsStreamDesc == NULL ) || ( ppMappedData == NULL ) || ( ppUnmappedData == NULL ))
    {
        DEBUGMSG( ZONE_IOCTL|ZONE_ERROR, ( _T("PIN_IOControl(%08x): RemoveBufferFromList - Null pointer has been passed in.\r\n"), this ) );
        return RetVal;
    }
    //Initialize arguments
    *ppCsStreamDesc = NULL;
    *ppMappedData = NULL;
    *ppUnmappedData = NULL;
 //   RETAILMSG( 1, ( _T("EnterCriticalSection\r\n")) );
    EnterCriticalSection( &m_csStreamBuffer );
    while(( dwCounter < m_dwBufferCount ) && ( *ppCsStreamDesc == NULL ))
    {
        if( m_pStreamDescriptorList[ dwCounter ].pCsStreamDescriptorExternal != NULL )
        {
            //REVIEW: All buffers accessed here should have been marshalled, check if try/except is needed
            __try
            {
            // We found one registered buffer. Let's return it.
            *ppCsStreamDesc = m_pStreamDescriptorList[ dwCounter ].pCsStreamDescriptorExternal;            
            *ppMappedData   = m_pStreamDescriptorList[ dwCounter ].csStreamDescriptorShadow.CsStreamHeader.Data;
            *ppUnmappedData = m_pStreamDescriptorList[ dwCounter ].pCsStreamDescriptorExternal->CsStreamHeader.Data;
            m_pStreamDescriptorList[ dwCounter ].pCsStreamDescriptorExternal = NULL;
            m_pStreamDescriptorList[ dwCounter ].csStreamDescriptorShadow.CsStreamHeader.Data = *ppUnmappedData;
            break;
            }
            __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
            //RETAILMSG( 1, ( _T("Exception\r\n")) );            
                DEBUGMSG(ZONE_IOCTL|ZONE_ERROR, (_T("PIN_IOControl: RemoveBufferFromList - Access Violation.\r\n"))) ;                
                RetVal = false;
            }         
        }

        dwCounter++;
    }
    LeaveCriticalSection( &m_csStreamBuffer );
    if(NULL == *ppMappedData)
    {
     //   RETAILMSG( 1, ( _T("NULL == *ppMappedData\r\n")) );                
        RetVal = false;
    }
    
    return RetVal;
}


bool
CPinDevice::ResetBufferList()
{
    EnterCriticalSection( &m_csStreamBuffer );
    if( m_pStreamDescriptorList )
    {
        for( DWORD i = 0; i < m_ulMaxNumOfBuffers; i++ )
        {
            if(m_pStreamDescriptorList[ i ].m_pMarshalledDataBuffer != NULL)
            {
                delete m_pStreamDescriptorList[ i ].m_pMarshalledDataBuffer;
                m_pStreamDescriptorList[ i ].m_pMarshalledDataBuffer = NULL;
            }

            if(m_pStreamDescriptorList[ i ].m_pMarshalledStreamDesc != NULL)
            {
                delete m_pStreamDescriptorList[ i ].m_pMarshalledStreamDesc;
                m_pStreamDescriptorList[ i ].m_pMarshalledStreamDesc = NULL;
            }

            m_pStreamDescriptorList[ i ].pCsStreamDescriptorExternal = NULL;
            m_pStreamDescriptorList[ i ].m_fBusy = FALSE;
        }
    }
    
    LeaveCriticalSection( &m_csStreamBuffer );

    return true;
}



ULONG CPinDevice :: PictureNumber( ) const
{
    return m_ulPictureNumber;
}

ULONG CPinDevice :: FramesDropped( ) const
{
    return m_ulFramesDropped;
}

ULONG CPinDevice :: FrameSize( ) const
{
    return m_ulFrameSize;
}


