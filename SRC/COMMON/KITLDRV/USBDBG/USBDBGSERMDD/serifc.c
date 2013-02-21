//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement,
// you are not authorized to use this sample source code.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#include <windows.h>
#include <oal.h>
#include <usbdbgddsi.h>
#include "usbser.h"
#include <usbdbgser.h>

///=============================================================================
/// Local defines
///=============================================================================
#define USBDBG_KITL_MTU             1520        // same as KITL_MTU

///=============================================================================
/// Static variables
///=============================================================================

// a receive buffer to temporary store data for the upper layer (KITL/DLE)
//
static struct
{
    UINT32 cbFilled;                // cb data stored in the buffer
    BYTE buffer[RECVBUF_MAXSIZE];   // storage space
    UINT32 bufOffset;               // current buffer[] offset
} m_RecvBuffer;

///=============================================================================
/// Local functions
///=============================================================================

#ifdef DEBUG

static
void
DumpPacket(
    DWORD zone,
    BYTE* pData,
    UINT32 cbDataSize
    )
{
    UINT32 i;

    if (g_UsbDbgZones & (zone))
    {
        for (i=0; i < cbDataSize; i++)
        {
            USBDBGMSG(zone, (L"%02X ", *((PUCHAR)(pData+i))));
            if ( (i+1) % 25 == 0)
                USBDBGMSG(USBDBG_ZONE_INFO, (L"\r\n"));
        }
        if ( i % 25 != 0)
            USBDBGMSG(zone, (L"\r\n"));
    }
}

#endif

//------------------------------------------------------------------------------
// Description: Maintains a local buffer. Get data from the buffer (removes data)
//
// Returns: Size of data copied into pbBuffer.
// 
static
UINT32
RecvBuffer_GetData(
    __out_ecount(cbBufSize) BYTE* pbBuffer,
    UINT32 cbBufSize
    )
{
    UINT32 cbToCopy;
    
    if (m_RecvBuffer.cbFilled == 0)
        return 0;

    if (cbBufSize <= m_RecvBuffer.cbFilled)
        cbToCopy = cbBufSize;
    else
        cbToCopy = m_RecvBuffer.cbFilled;
        
    memcpy(pbBuffer, m_RecvBuffer.buffer+m_RecvBuffer.bufOffset, cbToCopy);
    m_RecvBuffer.cbFilled -= cbToCopy;
    m_RecvBuffer.bufOffset += cbToCopy;

    return cbToCopy;        
}


#if 0  // currently not used
//------------------------------------------------------------------------------
// Description: Maintains a local buffer. Stores data into the buffer
//
// Returns: Size of data copied into pbBuffer.
// 
// Assumption: Buffer is empty when RecvBuffer_StoreData is called
//
static
void
RecvBuffer_StoreData(
    BYTE* pbBuffer,
    UINT32 cbStoreSize
    )
{
    if (m_RecvBuffer.cbFilled > 0)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"ERROR:: UsbDbgSer: RecvBuffer_StoreData: Buffer is not empty\r\n"));
        return;
    }
    if (cbStoreSize > RECVBUF_MAXSIZE)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"ERROR: UsbDbgSer: RecvBuffer_StoreData: Avail buffer is too small. BufSize=%d. cbStoreSize=%d\r\n", RECVBUF_MAXSIZE, cbStoreSize));
        return;
    }
    memcpy(m_RecvBuffer.buffer, pbBuffer, cbStoreSize);
    m_RecvBuffer.cbFilled = cbStoreSize;
    m_RecvBuffer.bufOffset = 0;

    USBDBGMSG(USBDBG_ZONE_WARN, (L"RecvBuffer_StoreData: stored %d bytes\r\n", cbStoreSize));
}
#endif 

static 
BOOL
WaitForInit(
    )
{
#define WAITFORINIT_SEC 10
    DWORD dwStartSec;

    if (UsbSerial_IsConnected())
        return TRUE;
        
    dwStartSec=OEMKitlGetSecs();

    // wait for initialization if not already initialized
    while  (!UsbSerial_IsConnected() &&  OEMKitlGetSecs()-dwStartSec<WAITFORINIT_SEC) 
        UsbSerial_EventHandler();

    // if not initialized return FALSE
    if (!UsbSerial_IsConnected())
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"ERROR: UsbDbgSer:: Not connected. Aborting send/recv frame\r\n"));
        return FALSE;
    }

    return TRUE;
}

/// Public Functions (interface implementation)
/// 
///
//---------------------------------------------------------------------------//
/// incoming OAL_KITL_SERIAL_DRIVER

///
BOOL 
Serial_Init(
    KITL_SERIAL_INFO *pInfo
    )
{
#define INITIALTIME_SEC 300
    DWORD dwStartSec;

    // The input fields in pInfo don't apply to this driver. So pInfo may optionally be NULL.
    // When non-NULL the bestSize field is filled in.  
    if(pInfo != NULL ){   
        // set transfer size
        pInfo->bestSize = USBDBG_KITL_MTU;
    }        
    
    // init lower layer - usbserial
    //
    if(!UsbSerial_Init())
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"UsbDbgSer:: initialization: UsbSerial_Init() Failed!\r\n"));                
        return FALSE;
    }

    // wait for connection
    //
    dwStartSec=OEMKitlGetSecs();
    while (OEMKitlGetSecs()-dwStartSec<=INITIALTIME_SEC) {
        UsbSerial_EventHandler();             // call message handler
        if (UsbSerial_IsConnected())
            break;
    }

    // print success/failure message
    //
    if (UsbSerial_IsConnected()) {
        USBDBGMSG(USBDBG_ZONE_INFO, (L"UsbDbgSer:: initialization: Success\r\n"));
        return TRUE;
    }
    else {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"UsbDbgSer:: initialization: Fail!\r\n"));
        return FALSE;
    }   
}

void 
Serial_EnableInts(
    )
{
    //empty
}

void
Serial_DisableInts(
    )
{
    //empty
}   

//------------------------------------------------------------------------------
// Description: Signature matches OAL_KITLSERIAL_SEND defined in oal_kitl.h
//     Called by KITL or DLE to send data over USBDBG Serial.
//
// Parameters:
//     (IN) pData: data to send
//     (IN) size:  number of bytes to send from pData
//
// Returns: Number of bytes sent
// 
UINT16
Serial_Send(
    UINT8 *pData,
    UINT16 size
    )
{
    UINT32 retVal;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"+UsbDbgSerial_Send: size=%d\r\n", size));

    if( pData == NULL ){
        USBDBGMSG(USBDBG_ZONE_ERROR, (L" UsbDbgSerial_Send: Error: null ptr\r\n"));
        retVal = 0;
        goto cleanUp;
    }        
    
    if (!WaitForInit())
    {
        retVal = 0;       
        goto cleanUp;
    }
    
    retVal = UsbSerial_SendData(pData, size);

    if (retVal != ERROR_SUCCESS)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L" UsbDbgSerial_Send: Error: SendData returned %d\r\n", retVal));
        retVal = 0;  // no bytes sent.
    }
    else
    {
        retVal = size;
    }

#ifdef DEBUG
    USBDBGMSG(USBDBG_ZONE_SEND, (L">>SEND_UsbDbg: %d bytes\r\n", size));
    DumpPacket(USBDBG_ZONE_SEND, pData, size);
#endif
    
cleanUp:
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"-UsbDbgSerial_Send: retVal=%d\r\n", retVal));
    return (UINT16)retVal;

}

//------------------------------------------------------------------------------
// Description: Signature matches OAL_KITLSERIAL_RECV defined in oal_kitl.h
//     Called by KITL or DLE to receive data over USBDBG Serial.
//
// Parameters:
//     (IN) pData: data to send
//     (IN) size:  number of bytes to send from pData
//
// Returns: Number of bytes sent
// 
UINT16
Serial_Recv(
    UINT8 *pData,
    UINT16 size
    )
{
    UINT32 cbRecvd = 0;
    
    USBDBGMSG(USBDBG_ZONE_FUNC, (L"+UsbDbgSerial_Recv: bufSize=%d\r\n", size));

    if( pData == NULL ){
        USBDBGMSG(USBDBG_ZONE_ERROR, (L" UsbDbgSerial_Recv: Error: null ptr\r\n"));
        cbRecvd = 0;
        goto cleanUp;
    }        
    
    if (!WaitForInit())
    {
        cbRecvd = 0;
        goto cleanUp;
    }

    // previously received data available in buffer?
    cbRecvd = RecvBuffer_GetData(pData, size);
    if (cbRecvd > 0)
        goto cleanUp;

    // do a usb bulk receive
    //
#ifdef DEBUG
    // buffer must be empty (buffer is fully emptied out before getting filled)
    if (m_RecvBuffer.cbFilled > 0)
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"ERROR:: UsbDbgSer: Serial_Recv: Buffer is not empty\r\n"));
        cbRecvd = 0;
        goto cleanUp;
    }
#endif

    cbRecvd = UsbSerial_RecvData(m_RecvBuffer.buffer, sizeof(m_RecvBuffer.buffer)); 

    //if (cbRecvd > 0) 
    //    USBDBGMSG(USBDBG_ZONE_INFO, (L"Serial_Recv: UsbSerial_RecvData=%d bytes\r\n", cbRecvd));
        
    if (cbRecvd > 0)
    {
        //if (cbRecvd > USBDBG_KITL_MTU)  //truncate data
        //    cbRecvd = USBDBG_KITL_MTU;

        m_RecvBuffer.cbFilled = cbRecvd;
        m_RecvBuffer.bufOffset = 0;

        cbRecvd = RecvBuffer_GetData(pData, size);
        goto cleanUp;
    }
    
cleanUp:

#ifdef DEBUG
    if (cbRecvd > 0)
    {
        USBDBGMSG(USBDBG_ZONE_RECV, (L"<<RECV_UsbDbg: %d bytes\r\n", cbRecvd));
        DumpPacket(USBDBG_ZONE_RECV, pData, cbRecvd);
    }
#endif

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"-UsbDbgSerial_Recv: cbRecvd=%d\r\n",cbRecvd));
    return (UINT16)cbRecvd;
}

void
Serial_PowerOff(
    )
{
    UsbSerial_SetPower(TRUE);
}

void
Serial_PowerOn(
    )
{
    UsbSerial_SetPower(FALSE);
    WaitForInit();
}

void
Serial_DeInit(
    )
{
    UsbSerial_Deinit();
}
