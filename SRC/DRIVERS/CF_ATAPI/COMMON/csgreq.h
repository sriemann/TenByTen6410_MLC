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
/*++

Module Name:

    CSgReq.h

Abstract:

    Scatter/Gather Request (SG_REQ) wrapper.

--*/

#ifndef __CSGREQ_H__
#define __CSGREQ_H__

#include <windows.h>
#include <diskio.h>

class CSgReq {
    // Attached Scatter/Gather request.  This is the structure that is wrapped
    // by this object.
    PSG_REQ      m_pSgReq;
    // A mapped rendition of the Scatter/Gather buffer list.  This is a copy
    // of the attached Scatter/Gather buffer list. In pre-Yamazaki CE every buffer pointer
    // had been mapped through MapCallerPtr. This mapping call has been removed now because
    // ValidateSg does the mapping through CeOpenCallerBuffer.
    SG_BUF       m_rgMappedSgBufList[MAX_SG_BUF];
    // The sector size of the device targeted by the Scatter/Gather request.
    DWORD        m_dwSectorSize;
    // The buffer in the Scatter/Gather buffer list currently being accessed by
    // the user of the wrapper.
    DWORD        m_dwCurrentBuffer;
    // The current position in the above buffer (m_dwCurrentBuffer).  That is,
    // the byte currently being accessed by the user of the wrapper.
    DWORD        m_dwCurrentBufferPosition;
    // It is added for the case that can not use DMA mode.
    static DWORD GetSgReqLengthBySgBufList(PSG_REQ pSgReq);
    static VOID  DoResetMappedSgBufList(PSG_BUF rgMappedSgBufList, DWORD dwMappedSgBufListLength);
    static BOOL  DoMapSgBufList(PSG_BUF rgMappedSgBufList, DWORD dwMappedSgBufListLength, PSG_BUF rgSourceSgBufList, DWORD dwSourceSgBufListLength);
  public:
                 CSgReq();
                 ~CSgReq();
    DWORD        GetStartingSector();
    DWORD        GetNumberOfSectors();
    DWORD        GetNumberOfBuffers();
    DWORD        GetCurrentBufferLength();
    DWORD        GetCurrentBufferNumber();
    PBYTE        GetCurrentBufferAsPointer();
    DWORD        GetCurrentBufferPosition();
    PBYTE        GetCurrentBufferPositionAsPointer();
    DWORD        GetAbsoluteBufferLength();
    DWORD        GetAbsoluteBufferPosition();
    VOID         SetCurrentBuffer(DWORD dwCurrentBuffer);
    VOID         SetCurrentBufferPosition(DWORD dwCurrentBufferPosition);
    BOOL         DoAttach(PSG_REQ pSgReq, DWORD dwSectorSize);
    BOOL         DoAdvanceBuffer();
    DWORD        DoSeek(DWORD dwBytes);
    BOOL         DoReadMultiple(PBYTE pbBuf, DWORD dwBytes, PDWORD pdwBytesRead);
    BOOL         DoWriteMultiple(PBYTE pbBuf, DWORD dwBytes, PDWORD pdwBytesWritten);
    inline BOOL  DoReadByte(PBYTE pb);
    inline BOOL  DoWriteByte(PBYTE pb);
    inline BOOL  DoReadWord(PWORD pw);
    inline BOOL  DoWriteWord(PWORD pw);
};

// ----------------------------------------------------------------------------
// This function reads and returns one byte from the current position in the
// active buffer in the Scatter/Gather buffer list and advances the current
// position in the active buffer in the Scatter/Gather bufer list by 1 byte,
// updating the active buffer and current position within the active buffer
// accordingly.  If successful, return TRUE.  If unsuccessful, return FALSE.
//
//   pb
//     [out] The destination buffer.
// ----------------------------------------------------------------------------
inline BOOL CSgReq::DoReadByte(PBYTE pb)
{
    DWORD dwBytesRead = 0;
    if (FALSE == this->DoReadMultiple(pb, 1, &dwBytesRead)) {
        return FALSE;
    }
    if (1 != dwBytesRead) {
        return FALSE;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------
// This function writes one byte to the current position in the active buffer
// in the Scatter/Gather buffer list and seeks 1 byte forward, updating the
// active buffer and current position within the active buffer accordingly.  If
// If successful, return TRUE.  If unsuccessful, return FALSE.
//
//   pb
//     [in] The source buffer.
// ----------------------------------------------------------------------------
inline BOOL CSgReq::DoWriteByte(PBYTE pb)
{
    DWORD dwBytesWritten = 0;
    if (FALSE == this->DoWriteMultiple(pb, 1, &dwBytesWritten)) {
        return FALSE;
    }
    if (1 != dwBytesWritten) {
        return FALSE;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------
// This function reads and returns one word from the current position in the
// active buffer in the Scatter/Gather buffer list and advances the current
// position in the active buffer in the Scatter/Gather buffer list by 2 bytes,
// updating the active buffer and current position within the active buffer
// accordingly.  If successful, return TRUE.  If unsuccessful, return FALSE.
//
//   pb
//     [out] The destination buffer.
// ----------------------------------------------------------------------------
inline BOOL CSgReq::DoReadWord(PWORD pw)
{
    DWORD dwBytesRead = 0;
    if (FALSE == this->DoReadMultiple((PBYTE)pw, 2, &dwBytesRead)) {
        return FALSE;
    }
    if (2 != dwBytesRead) {
        return FALSE;
    }
    return TRUE;
}

// ----------------------------------------------------------------------------
// This function writes one word to the current position in the active buffer
// in the Scatter/Gather buffer list and seeks 1 byte forward, updating the
// active buffer and current position within the active buffer accordingly.  If
// If successful, return TRUE.  If unsuccessful, return FALSE.
//
//   pb
//     [in] The source buffer.
// ----------------------------------------------------------------------------
inline BOOL CSgReq::DoWriteWord(PWORD pw)
{
    DWORD dwBytesWritten = 0;
    if (FALSE == this->DoWriteMultiple((PBYTE)pw, 2, &dwBytesWritten)) {
        return FALSE;
    }
    if (2 != dwBytesWritten) {
        return FALSE;
    }
    return TRUE;
}

#endif // __CSGREQ_H__
