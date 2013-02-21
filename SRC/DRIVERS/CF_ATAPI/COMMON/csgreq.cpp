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

#include "csgreq.h"

// ----------------------------------------------------------------------------
// This function returns the length (number of bytes) of a Scatter/Gather
// request by summing the number of bytes in each buffer in the Scatter/
// Gather buffer list.  If successful, return the length of the supplied
// Scatter/Gather request.  If unsuccessful, return -1.
//
//   pSgReq
//     [in] Pointer to target Scatter/Gather request.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetSgReqLengthBySgBufList(PSG_REQ pSgReq)
{
    DWORD dwSgReqLengthBySgBufList = 0;

    if (NULL == pSgReq) {
        ASSERT(NULL != pSgReq);
        return -1;
    }
    for (DWORD i = 0; i < pSgReq->sr_num_sg; i += 1) {
        dwSgReqLengthBySgBufList += pSgReq->sr_sglist[i].sb_len;
    }
    return dwSgReqLengthBySgBufList;
}

// ----------------------------------------------------------------------------
// This function initializes a Scatter/Gather buffer list.
//
//   rgMappedSgBufList
//     [in] Pointer to target Scatter/Gather buffer list.
//   dwMappedSgBufListLength
//     [in] The number of buffers in the target Scatter/Gather buffer list.
// ----------------------------------------------------------------------------
VOID CSgReq::DoResetMappedSgBufList(PSG_BUF rgMappedSgBufList, DWORD dwMappedSgBufListLength)
{
    if (NULL == rgMappedSgBufList) {
        ASSERT(NULL != rgMappedSgBufList);
        return;
    }
    if (MAX_SG_BUF < dwMappedSgBufListLength) {
        ASSERT(MAX_SG_BUF >= dwMappedSgBufListLength);
        return;
    }
    for (DWORD i = 0; i < dwMappedSgBufListLength; i += 1) {
        rgMappedSgBufList[i].sb_len = 0;
        rgMappedSgBufList[i].sb_buf = NULL;
    }
    return;
}

// ----------------------------------------------------------------------------
// This function maps the buffer pointers of a Scatter/Gather buffer list.
// The mapping is redundant as ValidateSg has already performed the mapping.
// So this function just copies values and does a sanity check.
// If successful, return TRUE.  If unsuccessful, return FALSE.
//
//   rgMappedSgBufList
//     [out] Pointer to destination Scatter/Gather buffer list.  The mapped
//     buffer pointers will reside in this buffer list.
//   dwMappedSgBufListLength
//     [in] The number of buffers in the destination Scatter/Gather buffer
//     list.
//   rgSourceSgBufList
//     [in] Pointer to source Scatter/Gather buffer list.  The buffer pointers
//     in this buffer list was previously mapped via CeOpenCallerBuffer. Hence
//     in ValidateSg. So we just do a plain copy.
//   dwSourceSgBufListLength
//     [in] The number of buffers in the source Scatter/Gather buffer list.
// ----------------------------------------------------------------------------
BOOL CSgReq::DoMapSgBufList(PSG_BUF rgMappedSgBufList, DWORD dwMappedSgBufListLength, PSG_BUF rgSourceSgBufList, DWORD dwSourceSgBufListLength)
{
    DWORD dwLength = 0;

    if (NULL == rgMappedSgBufList) {
        ASSERT(NULL != rgMappedSgBufList);
        return FALSE;
    }
    if (MAX_SG_BUF < dwMappedSgBufListLength) {
        ASSERT(MAX_SG_BUF >= dwMappedSgBufListLength);
        return FALSE;
    }
    if (NULL == rgSourceSgBufList) {
        ASSERT(NULL != rgSourceSgBufList);
        return FALSE;
    }
    if (MAX_SG_BUF < dwSourceSgBufListLength) {
        ASSERT(MAX_SG_BUF >= dwSourceSgBufListLength);
        return FALSE;
    }
    if (dwMappedSgBufListLength < dwSourceSgBufListLength) {
        ASSERT(dwMappedSgBufListLength >= dwSourceSgBufListLength);
        return FALSE;
    }

    for (DWORD i = 0; i < dwSourceSgBufListLength; i += 1) {
        rgMappedSgBufList[i].sb_buf = rgSourceSgBufList[i].sb_buf;
        if ((NULL != rgSourceSgBufList[i].sb_buf) && (NULL == rgMappedSgBufList[i].sb_buf)) {
            return FALSE;
        }
        rgMappedSgBufList[i].sb_len = rgSourceSgBufList[i].sb_len;
    }

    return TRUE;
}

// ----------------------------------------------------------------------------
// This constructor initializes the object's member variables to invalid
// values.
// ----------------------------------------------------------------------------
CSgReq::CSgReq()
{
    m_pSgReq = NULL;
    m_dwSectorSize = (DWORD)-1;
    m_dwCurrentBuffer = (DWORD)-1;
    m_dwCurrentBufferPosition = (DWORD)-1;
}

// ----------------------------------------------------------------------------
// This destructor does nothing.
// ----------------------------------------------------------------------------
CSgReq::~CSgReq()
{

}

// ----------------------------------------------------------------------------
// This function returns the starting sector of the Scatter/Gather request.
// If successful, return the starting sector of the Scatter/Gather request.
// If unsuccessful, return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetStartingSector()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    return m_pSgReq->sr_start;
}

// ----------------------------------------------------------------------------
// This function returns the number of sequential sectors accessed by the
// Scatter/Gather request.  If successful, return the starting sector of the
// Scatter/Gather request.  If sunsuccessful, return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetNumberOfSectors()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    return m_pSgReq->sr_num_sec;
}

// ----------------------------------------------------------------------------
// This function returns the number of buffers in the Scatter/Gather buffer
// list.  If successful, return the number of buffers in the Scatter/Gather
// buffer list.  If unsuccessful, return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetNumberOfBuffers()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    return m_pSgReq->sr_num_sg;
}

// ----------------------------------------------------------------------------
// This function returns the length (total bytes) of the active buffer in the
// Scatter/Gather buffer list.  If successful, return the length of the active
// buffer in the Scatter/Gather buffer list.  If unsuccessful, return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetCurrentBufferLength()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    return m_pSgReq->sr_sglist[m_dwCurrentBuffer].sb_len;
}

// ----------------------------------------------------------------------------
// This function returns the 0-based index number of the active buffer in the
// Scatter/Gather buffer list.  If successful, return the 0-based index number
// of the active buffer in the Scatter/Gather buffer list.  If unsuccessful,
// return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetCurrentBufferNumber()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    return m_dwCurrentBuffer;
}

// ----------------------------------------------------------------------------
// This function returns a pointer to the active buffer in the Scatter/Gather
// buffer list.  If successful, return a pointer to the active Scatter/Gather
// buffer in the Scatter/Gather buffer list.  If unsuccessful, return NULL.
// ----------------------------------------------------------------------------
PBYTE CSgReq::GetCurrentBufferAsPointer()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return NULL;
    }
    return m_rgMappedSgBufList[m_dwCurrentBuffer].sb_buf;
}

// ----------------------------------------------------------------------------
// This function returns the current position in the active Scatter/Gather
// buffer in the Scatter/Gather buffer list.  If successful, return the 0-based
// index number of the current byte in the active Scatter/Gather buffer.  If
// unsuccessful, return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetCurrentBufferPosition()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    return m_dwCurrentBufferPosition;
}

// ----------------------------------------------------------------------------
// This function returns a pointer to the current byte in the active Scatter/
// Gather buffer in the Scatter/Gather buffer list.  If successful, return a
// pointer to the current byte in the active Scatter/Gather buffer in the
// Scatter/Gather buffer list.  If unsuccessful, return NULL.
// ----------------------------------------------------------------------------
PBYTE CSgReq::GetCurrentBufferPositionAsPointer()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return NULL;
    }
    return &m_rgMappedSgBufList[m_dwCurrentBuffer].sb_buf[m_dwCurrentBufferPosition];
}

// ----------------------------------------------------------------------------
// Since a Scatter/Gather buffer list is a single buffer split over a set of
// sub-buffers, it is possible to calculate the absolute buffer length as the
// sum of the sub-buffer lengths.  This function returns the absolute buffer
// length.  If successful, return the absolute buffer length.  If unsuccessful,
// return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetAbsoluteBufferLength()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    return GetSgReqLengthBySgBufList(m_pSgReq);
}

// ----------------------------------------------------------------------------
// Since a Scatter/Gather buffer list is a single buffer split over a set of
// sub-buffers, it is possible to calculate the absolute buffer position.  This
// function returns the absolute buffer position.  If successful, return the
// absolute buffer position.  If unsuccessful, return -1.
// ----------------------------------------------------------------------------
DWORD CSgReq::GetAbsoluteBufferPosition()
{
    DWORD dwRet = 0;

    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return (DWORD)-1;
    }
    for (DWORD i = 0; i < m_dwCurrentBuffer; i += 1) {
        dwRet += m_pSgReq->sr_sglist[i].sb_len;
    }
    // m_dwCurrentBufferPosition is 0-based; add 1 for absolute position.
    dwRet += (m_dwCurrentBufferPosition + 1);
    return dwRet;
}

// ----------------------------------------------------------------------------
// This function sets the active buffer in the Scatter/Gather buffer list.
//
//   dwCurrentBuffer
//     [in] The 0-based index of the new active buffer in the Scatter/Gather
//     buffer list.
// ----------------------------------------------------------------------------
VOID CSgReq::SetCurrentBuffer(DWORD dwCurrentBuffer)
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return;
    }
    m_dwCurrentBuffer = dwCurrentBuffer;
    return;
}

// ----------------------------------------------------------------------------
// This function sets the current position in the active buffer in the Scatter/
// Gather buffer list.
//
//   dwCurrentBufferPosition
//     [in] The 0-based index of the new position in the active buffer in the
//     Scatter/Gather buffer list.
// ----------------------------------------------------------------------------
VOID CSgReq::SetCurrentBufferPosition(DWORD dwCurrentBufferPosition)
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return;
    }
    m_dwCurrentBufferPosition = dwCurrentBufferPosition;
    return;
}

// ----------------------------------------------------------------------------
// This function associates a Scatter/Gather structure with the wrapper.  This
// function also validates the Scatter/Gather structure and copies it into 
// m_rgMappedSgBufList.  If successful, return TRUE.  If
// unsuccessful, return FALSE.
//
//   pSgReq
//     [in] Pointer to Scatter/Gather structure to wrap.
// ----------------------------------------------------------------------------
BOOL CSgReq::DoAttach(PSG_REQ pSgReq, DWORD dwSectorSize)
{
    if (NULL == pSgReq) {
        ASSERT(NULL != pSgReq);
        return FALSE;
    }
    if (0 == pSgReq->sr_num_sec) {
        ASSERT(0 < pSgReq->sr_num_sec);
        return FALSE;
    }
    if ((0 == pSgReq->sr_num_sg) || (pSgReq->sr_num_sg > MAX_SG_BUF)) {
        ASSERT(0 < pSgReq->sr_num_sg);
        ASSERT(MAX_SG_BUF >= pSgReq->sr_num_sg);
        return FALSE;
    }
    if ((pSgReq->sr_num_sec * dwSectorSize) != GetSgReqLengthBySgBufList(pSgReq)) {
        ASSERT(FALSE);
        return FALSE;
    }

    // Reset mapped Scatter/Gather buffer list.
    DoResetMappedSgBufList(m_rgMappedSgBufList, MAX_SG_BUF);

    // Map pointers in caller's buffer list to mapped Scatter/Gather buffer list.
    if(FALSE == DoMapSgBufList(m_rgMappedSgBufList, MAX_SG_BUF, pSgReq->sr_sglist, pSgReq->sr_num_sg)) {
        return FALSE;
    }

    // Initialize position.
    m_pSgReq = pSgReq;
    m_dwSectorSize = dwSectorSize;
    m_dwCurrentBuffer = 0;
    m_dwCurrentBufferPosition = 0;

    return TRUE;
}

// ----------------------------------------------------------------------------
// This function activates the next buffer in the Scatter/Gather buffer list.
// That is, if buffer 0 is currently active, then after calling this function,
// buffer 1 will be active.  It is not possible to advance past the last buffer
// in the buffer list.  If successful, return TRUE.  If unsuccessful, return
// FALSE.
// ----------------------------------------------------------------------------
BOOL CSgReq::DoAdvanceBuffer()
{
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return FALSE;
    }
    // Don't allow the caller to advance past the last buffer (i.e., we do not
    // wrap around to the first buffer).
    if ((m_dwCurrentBuffer + 1) > (m_pSgReq->sr_num_sg - 1)) {
        return FALSE;
    }
    m_dwCurrentBuffer += 1;
    m_dwCurrentBufferPosition = 0;
    return TRUE;
}

// ----------------------------------------------------------------------------
// This function advances the current buffer position by the specified number of
// bytes and updates the active buffer and current position within the active
// buffer accordingly.  It is not possible to seek past the last buffer in the
// Scatter/Gather buffer list.  Return the number of bytes the current buffer
// position advanced forward.  This value may differ from dwBytes if the caller
// attempted to seek past the last buffer in the Scatter/Gather buffer list.
//
//      dwBytes
//        [in] The number of bytes to advance.
// ----------------------------------------------------------------------------
DWORD CSgReq::DoSeek(DWORD dwBytes)
{
    DWORD dwRet = dwBytes;
    DWORD dwBytesLeftInCurrentBuffer = 0;

    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return FALSE;
    }

seek_in_current_buffer:;

    // Is the seek finished?
    if (0 == dwBytes) {
        goto exit;
    }

    // Calculate the number of bytes left in the current buffer.  We'll advance
    // one buffer at a time.
    dwBytesLeftInCurrentBuffer = GetCurrentBufferLength() - m_dwCurrentBufferPosition;

    // Given our current position and the number of bytes left to seek, will we
    // seek past the current buffer?
    if (dwBytes > dwBytesLeftInCurrentBuffer) {
        // We'll seek past the current buffer.  Advance to the next buffer.  If
        // this fails, then we'll have reached the end of the buffer list.
        dwBytes -= dwBytesLeftInCurrentBuffer;
        if (FALSE == DoAdvanceBuffer()) {
            goto exit;
        }
        // Repeat with next buffer.
        goto seek_in_current_buffer;
    }
    else {
        // We won't seek past the current buffer.  Adjust our current position
        // within the current buffer.
        m_dwCurrentBufferPosition += dwBytes;
        dwBytes = 0;
    }

exit:;

    // Return the difference between the requested number of bytes to seek and
    // the number of bytes yet to seek.  If the caller tried seeking past the
    // end of the buffer list, then the actual number of bytes we seeked past
    // will be less than the requested number of bytes to seek past.
    return (dwRet - dwBytes);
}

// ----------------------------------------------------------------------------
// This function reads the specified number of bytes from the current
// position in the active buffer in the Scatter/Gather buffer list to the
// supplied buffer.  If the read spans Scatter/Gather buffers, this function
// updates the active buffer and current position within the active buffer
// accordingly.  If successful, return TRUE.  If unsuccessful, return FALSE.
//
//   pbBuf
//     [out] The destination buffer.
//   dwBytes
//     [in] The number of bytes to read (from the current position in the
//     active buffer in the Scatter/Gather buffer list).
//   pdwBytesRead
//     [out] Pointer to DWORD in which to store the actual number of bytes
//     read.
// ----------------------------------------------------------------------------
BOOL CSgReq::DoReadMultiple(PBYTE pbBuf, DWORD dwBytes, PDWORD pdwBytesRead)
{
    DWORD dwBytesLeftInCurrentBuffer = 0;

    if (NULL == pbBuf) {
        ASSERT(NULL != pbBuf);
        return FALSE;
    }
    if (NULL == pdwBytesRead) {
        ASSERT(NULL != pdwBytesRead);
        return FALSE;
    }
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return FALSE;
    }

    *pdwBytesRead = dwBytes;

read_from_current_buffer:;

    // Is the read finished?
    if (0 == dwBytes) {
        goto exit;
    }

    // Calculate the number of bytes left in the current buffer.  We'll advance
    // one buffer at a time.
    dwBytesLeftInCurrentBuffer = GetCurrentBufferLength() - m_dwCurrentBufferPosition;

    // Given our current position and the number of bytes left to read, will we
    // read past the current buffer?
    if (dwBytes > dwBytesLeftInCurrentBuffer) {
        // We'll read past the current buffer.  Read the contents of the current
        // buffer.
        memcpy((LPVOID)pbBuf, (LPVOID)GetCurrentBufferPositionAsPointer(), dwBytesLeftInCurrentBuffer);
        pbBuf += dwBytesLeftInCurrentBuffer;
        dwBytes -= dwBytesLeftInCurrentBuffer;
        // Advance to the next buffer.  If this fails, then we'll have reached
        // the end of the buffer list.
        if (FALSE == DoAdvanceBuffer()) {
            goto exit;
        }
        // Repeat with the next buffer.
        goto read_from_current_buffer;
    }
    else {
        // We won't read past the current buffer.  Read the required contents of
        // the current buffer.
        memcpy((LPVOID)pbBuf, (LPVOID)GetCurrentBufferPositionAsPointer(), dwBytes);
        m_dwCurrentBufferPosition += dwBytes;
        dwBytes = 0;
    }

exit:;

    // Return the difference between the requested number of bytes to read and
    // the number of bytes remaining to be read.  If the caller tried reading
    // past the end of the buffer list, then the actual number of bytes we read
    // will be less than the requested number of bytes to read.
    *pdwBytesRead -= dwBytes;
    return TRUE;
}

// ----------------------------------------------------------------------------
// This function writes the specified number of bytes from the supplied
// buffer to the current position in the active buffer in the Scatter/Gather
// buffer list.  If the write spans Scatter/Gather buffers, this function
// updates the active buffer and current position within the active buffer
// accordingly.  If successful, return TRUE.  If unsuccessful, return FALSE.
//
//   pbBuf
//     [out] The source buffer.
//   dwBytes
//     [in] The number of bytes to write (to the current position in the
//     active buffer in the Scatter/Gather buffer list).
//   pdwBytesRead
//     [out] Pointer to DWORD in which to store the actual number of bytes
//     written.
// ----------------------------------------------------------------------------
BOOL CSgReq::DoWriteMultiple(PBYTE pbBuf, DWORD dwBytes, PDWORD pdwBytesWritten)
{
    DWORD dwBytesLeftInCurrentBuffer = 0;

    if (NULL == pbBuf) {
        ASSERT(NULL != pbBuf);
        return FALSE;
    }
    if (NULL == pdwBytesWritten) {
        ASSERT(NULL != pdwBytesWritten);
        return FALSE;
    }
    if (NULL == m_pSgReq) {
        ASSERT(NULL != m_pSgReq);
        return FALSE;
    }

    *pdwBytesWritten = dwBytes;

write_to_current_buffer:;

    // Is the write finished?
    if (0 == dwBytes) {
        goto exit;
    }

    // Calculate the number of bytes left in the current buffer.  We'll advance
    // one buffer at a time.
    dwBytesLeftInCurrentBuffer = GetCurrentBufferLength() - m_dwCurrentBufferPosition;

    // Given our current position and the number of bytes left to write, will we
    // write past the current buffer?
    if (dwBytes > dwBytesLeftInCurrentBuffer) {
        // We'll write past the current buffer.  Write the contents of the
        // current buffer.
        memcpy((LPVOID)GetCurrentBufferPositionAsPointer(), (LPVOID)pbBuf, dwBytesLeftInCurrentBuffer);
        pbBuf += dwBytesLeftInCurrentBuffer;
        dwBytes -= dwBytesLeftInCurrentBuffer;
        // Advance to the next buffer.  If this fails, then we'll have reached
        // the end of the buffer list.
        if (FALSE == DoAdvanceBuffer()) {
            goto exit;
        }
        // Repeat with the next buffer.
        goto write_to_current_buffer;
    }
    else {
        // We won't write past the current buffer.  Write the required contents
        // of the current buffer.
        memcpy((LPVOID)GetCurrentBufferPositionAsPointer(), (LPVOID)pbBuf, dwBytes);
        m_dwCurrentBufferPosition += dwBytes;
        dwBytes = 0;
    }

exit:;

    // Return the difference between the requested number of bytes to write and
    // the number of bytes remaining to be written.  If the caller tried writing
    // past the end of the buffer list, then the actual number of bytes we wrote
    // will be less than the requested number of bytes to write.
    *pdwBytesWritten -= dwBytes;
    return TRUE;
}
