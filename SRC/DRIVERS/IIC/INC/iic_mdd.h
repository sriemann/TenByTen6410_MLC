/**************************************************************************************
* 
*    Project Name : IIC Driver 
*
*    Copyright 2006 by Samsung Electronics, Inc.
*    All rights reserved.
*
*    Project Description :
*        This software is MDD layer for IIC Samsung driver. 
*  
*--------------------------------------------------------------------------------------
* 
*    File Name : iic_mdd.h
*  
*    File Description : This file implements common structures and macros for IIC driver.
*
*    Author : JeGeon.Jung
*    Dept. : AP Development Team
*    Created Date : 2007/06/11
*    Version : 0.1 
* 
*    History
*    - Created(JeGeon.Jung 2007/06/11)
*  
*    Todo
*
*
*    Note
*
**************************************************************************************/
#ifndef __IIC_MDD_H__
#define __IIC_MDD_H__

#include <iic.h>


enum    IIC_STATE
{
    IIC_FINISH = 0,
    IIC_RUN = 1
};


typedef struct __HW_OPEN_INFO HW_OPEN_INFO, *PHW_OPEN_INFO;

typedef struct __PDD_COMMON_SET {
    BYTE                SlaveAddress;
    BYTE                InterruptEnable;        // Tx/Rx interrupt enable
} PDD_COMMON_SET, *PPDD_COMMON_SET;

typedef struct __PDD_CONTEXT_SET {
    UINT32                Clock;
    IIC_MODE            ModeSel;        // Master/Slave, Tx/Rx
    BYTE                ClockSel;           // Clock prescaler selection 
    BYTE                ClockDiv;           // Clock Divider value
    BYTE                FilterEnable;     // IIC bus filter enable bit
    IIC_DELAY            Delay;           // IIC bus line delay lengh selection
} PDD_CONTEXT_SET, *PPDD_CONTEXT_SET;


typedef struct __HW_INIT_INFO {
    CRITICAL_SECTION    CritSec;            // Protects tx/rx action
    PHW_OPEN_INFO       pAccessOwner;       // Points to whichever open has acess permissions
    LIST_ENTRY          OpenList;           // Head of linked list of OPEN_INFOs 
    CRITICAL_SECTION     OpenCS;                // Protects Open Linked List + ref counts    
    DWORD                OpenCnt;            // Protects use of this port     
    ULONG                Priority256;        // CeThreadPriority of transmit Thread.
    PDD_COMMON_SET        PDDCommonVal;        // common setting value for all contexts.
    IIC_STATE            State;                // state (IIC_RUN, IIC_FINISH)    
} HW_INIT_INFO, *PHW_INIT_INFO;


typedef struct __HW_OPEN_INFO {
    PHW_INIT_INFO       pInitContext;         // Pointer back to our HW_INIT_INFO
    DWORD                AccessCode;          // What permissions was this opened with
    DWORD                ShareMode;           // What Share Mode was this opened with
    DWORD               StructUsers;         // Count of threads currently using struct.
    LIST_ENTRY              llist;               // Linked list of OPEN_INFOs
    PDD_CONTEXT_SET        PDDContextVal;        // setting value for each context.
    BOOL                DirtyBit;            // This means PDDContextVal is updated.
} HW_OPEN_INFO, *PHW_OPEN_INFO;


#endif //    __IIC_MDD_H__
