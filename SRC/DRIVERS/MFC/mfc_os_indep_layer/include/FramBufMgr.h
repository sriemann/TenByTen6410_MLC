//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __SAMSUNG_SYSLSI_APDEV_FRAM_BUF_MGR_H__
#define __SAMSUNG_SYSLSI_APDEV_FRAM_BUF_MGR_H__


#include "MfcTypes.h"


#ifdef __cplusplus
extern "C" {
#endif


BOOL            FramBufMgrInit(unsigned char *pBufBase, int nBufSize);
void            FramBufMgrFinal(void);

unsigned char  *FramBufMgrCommit(int idx_commit, int commit_size);
void            FramBufMgrFree(int idx_commit);

unsigned char  *FramBufMgrGetBuf(int idx_commit);
int             FramBufMgrGetBufSize(int idx_commit);

void            FramBufMgrPrintCommitInfo(void);


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_FRAM_BUF_MGR_H__ */
