//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
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

*/


#include "MfcMemory.h"
#include "FramBufMgr.h"
#include "MfcTypes.h"
#include "LogMsg.h"


// The size in bytes of the BUF_SEGMENT.
// The buffers are fragemented into the segment unit of this size.
#define BUF_SEGMENT_SIZE    4096    // Ensures page alignment


typedef struct
{
    unsigned char *pBaseAddr;
    int            idx_commit;
} SEGMENT_INFO;


typedef struct
{
    int index_base_seg;
    int num_segs;
} COMMIT_INFO;


static SEGMENT_INFO  *_p_segment_info = NULL;
static COMMIT_INFO   *_p_commit_info  = NULL;


static unsigned char *_pBufferBase  = NULL;
static int            _nBufferSize  = 0;
static int            _nNumSegs        = 0;


//
// int FramBufMgrInit(unsigned char *pBufBase, int nBufSize)
//
// Description
//        This function initializes the MfcFramBufMgr(Buffer Segment Manager)
// Parameters
//        pBufBase [IN]: pointer to the buffer which will be managed by this MfcFramBufMgr functions.
//        nBufSize [IN]: buffer size in bytes
// Return Value
//        1 : Success
//        0 : Fail
//
BOOL FramBufMgrInit(unsigned char *pBufBase, int nBufSize)
{
    int   i;

    // check parameters
    if (pBufBase == NULL || nBufSize == 0)
        return FALSE;


    // If the frame buffer is already initialized
    // (1) and the result of initialization equals input parameters,
    //     You have to return 1 immediately because this function is called by misatake.
    // (2) and the result of initialization does not equal input parameters,
    //     You have to re-initialize after finalize.
    if ((_pBufferBase != NULL) && (_nBufferSize != 0)) {
        if ((pBufBase == _pBufferBase) && (nBufSize == _nBufferSize))
            return TRUE;

        FramBufMgrFinal();
    }


    _pBufferBase = pBufBase;
    _nBufferSize = nBufSize;
    _nNumSegs = nBufSize / BUF_SEGMENT_SIZE;

    _p_segment_info = (SEGMENT_INFO *) Mem_Alloc(_nNumSegs * sizeof(SEGMENT_INFO));
    for (i=0; i<_nNumSegs; i++) {
        _p_segment_info[i].pBaseAddr   = pBufBase  +  (i * BUF_SEGMENT_SIZE);
        _p_segment_info[i].idx_commit  = 0;
    }

    _p_commit_info  = (COMMIT_INFO *) Mem_Alloc(_nNumSegs * sizeof(COMMIT_INFO));
    for (i=0; i<_nNumSegs; i++) {
        _p_commit_info[i].index_base_seg  = -1;
        _p_commit_info[i].num_segs        = 0;
    }


    return TRUE;
}


//
// void FramBufMgrFinal()
//
// Description
//        This function finalizes the MfcFramBufMgr(Buffer Segment Manager)
// Parameters
//        None
// Return Value
//        None
//
void FramBufMgrFinal()
{
    if (_p_segment_info != NULL) {
        Mem_Free(_p_segment_info);
        _p_segment_info = NULL;
    }

    if (_p_commit_info != NULL) {
        Mem_Free(_p_commit_info);
        _p_commit_info = NULL;
    }


    _pBufferBase  = NULL;
    _nBufferSize  = 0;
    _nNumSegs       = 0;
}


//
// unsigned char *FramBufMgrCommit(int idx_commit, int commit_size)
//
// Description
//        This function requests the commit for commit_size buffer to be reserved.
// Parameters
//        idx_commit  [IN]: pointer to the buffer which will be managed by this MfcFramBufMgr functions.
//        commit_size [IN]: commit size in bytes
// Return Value
//        NULL : Failed to commit (Wrong parameters, commit_size too big, and so on.)
//        Otherwise it returns the pointer which was committed.
//
unsigned char *FramBufMgrCommit(int idx_commit, int commit_size)
{
    int  i, j;

    int  num_fram_buf_seg;        // The number of Segment for frame buffer


    // Check Initialization
    if (_p_segment_info == NULL || _p_commit_info == NULL) {
        return NULL;
    }

    // check parameters
    if (idx_commit < 0 || idx_commit >= _nNumSegs)
        return NULL;
    if (commit_size <= 0 || commit_size > _nBufferSize)
        return NULL;

    // Check idx_commit'th value in COMMIT_INFO array is free or did alreay commit
    if (_p_commit_info[idx_commit].index_base_seg != -1)
        return NULL;


    // Get the number of required FRAM_BUF_SEGMENT
    // If the required size of Instance FRAM_BUF excess the FRAM_BUF_SEGMENT_SIZE unit,
    // You do need one more FRAM_BUF_SEGMENT.
    if ((commit_size % BUF_SEGMENT_SIZE) == 0)
        num_fram_buf_seg = commit_size / BUF_SEGMENT_SIZE;
    else
        num_fram_buf_seg = (commit_size / BUF_SEGMENT_SIZE)  +  1;

    for (i=0; i<(_nNumSegs - num_fram_buf_seg); i++) {
        // Check until finding a item that it didn't commit in SEGMENT
        if (_p_segment_info[i].idx_commit != 0)
            continue;

        for (j=0; j<num_fram_buf_seg; j++) {
            if (_p_segment_info[i+j].idx_commit != 0)
                break;
        }

        // If it is no commited SEGMENT from j = 0 to j = num_fram_buf_seg,
        // Commit here.
        if (j == num_fram_buf_seg) {

            for (j=0; j<num_fram_buf_seg; j++) {
                _p_segment_info[i+j].idx_commit = 1;
            }

            _p_commit_info[idx_commit].index_base_seg  = i;
            _p_commit_info[idx_commit].num_segs        = num_fram_buf_seg;

            return _p_segment_info[i].pBaseAddr;
        }
        else
        {
            // When there is a empty space between instance buffers,
            // if available space is very small for allocating new instance,
            // You have to skip this empty space and find other empty space.
            i = i + j - 1;
        }
    }

    return NULL;
}


//
// void FramBufMgrFree(int idx_commit)
//
// Description
//        This function frees the committed region of buffer.
// Parameters
//        idx_commit  [IN]: pointer to the buffer which will be managed by this MfcFramBufMgr functions.
// Return Value
//        None
//
void FramBufMgrFree(int idx_commit)
{
    int  i;

    int  index_base_seg;        // The index of base segment committed
    int  num_fram_buf_seg;      // The number of Segment for frame buffer


    // Check Initialization
    if (_p_segment_info == NULL || _p_commit_info == NULL)
        return;

    // check parameters
    if (idx_commit < 0 || idx_commit >= _nNumSegs)
        return;

    // Check idx_commit'th value in COMMIT_INFO array is free or did alreay commit
    if (_p_commit_info[idx_commit].index_base_seg == -1)
        return;


    index_base_seg    =  _p_commit_info[idx_commit].index_base_seg;
    num_fram_buf_seg  =  _p_commit_info[idx_commit].num_segs;

    for (i=0; i<num_fram_buf_seg; i++) {
        _p_segment_info[index_base_seg + i].idx_commit = 0;
    }


    _p_commit_info[idx_commit].index_base_seg  =  -1;
    _p_commit_info[idx_commit].num_segs        =  0;

}




//
// unsigned char *FramBufMgrGetBuf(int idx_commit)
//
// Description
//        This function obtains the committed buffer of 'idx_commit'.
// Parameters
//        idx_commit  [IN]: commit index of the buffer which will be obtained
// Return Value
//        NULL : Failed to get the indicated buffer (Wrong parameters, not committed, and so on.)
//        Otherwise it returns the pointer which was committed.
//
unsigned char *FramBufMgrGetBuf(int idx_commit)
{
    int index_base_seg;

    // Check Initialization
    if (_p_segment_info == NULL || _p_commit_info == NULL)
        return NULL;

    // check parameters
    if (idx_commit < 0 || idx_commit >= _nNumSegs)
        return NULL;

    // Check idx_commit'th value in COMMIT_INFO array is free or did alreay commit
    if (_p_commit_info[idx_commit].index_base_seg == -1)
        return NULL;


    index_base_seg  =  _p_commit_info[idx_commit].index_base_seg;

    return _p_segment_info[index_base_seg].pBaseAddr;
}

//
// int FramBufMgrGetBufSize(int idx_commit)
//
// Description
//        This function obtains the size of the committed buffer of 'idx_commit'.
// Parameters
//        idx_commit  [IN]: commit index of the buffer which will be obtained
// Return Value
//        0 : Failed to get the size of indicated buffer (Wrong parameters, not committed, and so on.)
//        Otherwise it returns the size of the buffer.
//        Note that the size is multiples of the BUF_SEGMENT_SIZE.
//
int FramBufMgrGetBufSize(int idx_commit)
{
    // Check Initialization
    if (_p_segment_info == NULL || _p_commit_info == NULL)
        return 0;

    // check parameters
    if (idx_commit < 0 || idx_commit >= _nNumSegs)
        return 0;

    // Check idx_commit'th value in COMMIT_INFO array is free or did alreay commit 
    if (_p_commit_info[idx_commit].index_base_seg == -1)
        return 0;


    return (_p_commit_info[idx_commit].num_segs * BUF_SEGMENT_SIZE);
}


//
// void FramBufMgrPrintCommitInfo()
//
// Description
//        This function prints the commited information on the console screen.
// Parameters
//        None
// Return Value
//        None
//
void FramBufMgrPrintCommitInfo()
{
    int  i;

    // Check Initialization
    if (_p_segment_info == NULL || _p_commit_info == NULL) {
        LOG_MSG(LOG_TRACE, "FramBufMgrPrintCommitInfo", "\n The FramBufMgr is not initialized.\n");
        return;
    }


    for (i=0; i<_nNumSegs; i++) {
        if (_p_commit_info[i].index_base_seg != -1)  {
            LOG_MSG(LOG_TRACE, "FramBufMgrPrintCommitInfo", "\n\nCOMMIT INDEX = [%03d], BASE_SEG_IDX = %d", i, _p_commit_info[i].index_base_seg);
            LOG_MSG(LOG_TRACE, "FramBufMgrPrintCommitInfo", "\nCOMMIT INDEX = [%03d], NUM OF SEGS  = %d", i, _p_commit_info[i].num_segs);
        }
    }
}
