//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_INTR_NOTIFICATION_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_INTR_NOTIFICATION_H__


#ifdef __cplusplus
extern "C" {
#endif

BOOL CreateInterruptNotification(void);
void DeleteInterruptNotification(void);

int  SendInterruptNotification(int intr_type);
int  WaitInterruptNotification(void);


#ifdef __cplusplus
}
#endif


#define MFC_INTR_NOTI_TIMEOUT    1000

/*
 * MFC Interrupt Reason Macro Definition
 */
#define MFC_INTR_REASON_NULL            0x0000
#define MFC_INTR_REASON_BUFFER_EMPTY        0xC000
#define MFC_INTR_REASON_INTRNOTI_TIMEOUT    (-99)

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_INTR_NOTIFICATION_H__ */
