//
// Copyright (c) Samsung Electronics CO., LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    regctrl_g2d.h

Abstract:       defines for FIMGSE-2D Graphics Accelerator Register Controller

Notes:          Header to define the FIMGSE-2D Register Controller class

--*/

#ifndef __REGCTRL_G2D_H__
#define __REGCTRL_G2D_H__

#include <ceddk.h>
#include <g2d_reg.h>

/**
*    For Hardware Specific Macro
**/
#define G2D_DE_STATUS_FA_BIT            (1<<9)

/// Each command will be issued to 2D engine.
/// But if 2D engine is not idle, command will be pushed to COMMAND FIFO,
/// and Related Parameter registers also will be pushed to COMMAND FIFO
/// For each command, required FIFO size can be variable(?)
#define    G2D_COMMANDFIFO_SIZE        (32)            //< 32-word size

#define G2D_OPERAND3_PAT_BIT            (0<<13)
#define G2D_OPERAND3_FG_BIT             (1<<13)

/**
*    @brief    Rotation Degree and flip setting for register setting. FLIP_X means that up-down inversion, FLIP_Y means that left-right inversion
*/
typedef enum __G2D_ROT_TYPE
{
    ROT_0=(0x1<<0), ROT_90=(0x1<<1), ROT_180=(0x1<<2), ROT_270=(0x1<<3), FLIP_X=(0x1<<4), FLIP_Y=(0x1<<5)
} ROT_TYPE;

typedef enum
{
    G2D_OPERAND3_PAT,  G2D_OPERAND3_FG 
} G2D_OPERAND3;

class RegCtrlG2D
{
    public:
        RegCtrlG2D();
        virtual ~RegCtrlG2D();

        /// For HW Control
        void WaitForIdleStatus(void);
        bool IsIdle(void);
        bool WaitForFinish(void);
        void WaitForEmptyFifo() { this->RequestEmptyFifo(G2D_COMMANDFIFO_SIZE);}
        int CheckFifo();        
        void IntEnable(void);
        void IntDisable(void);
        void IntPendingClear(void);        

        void SetClipWindow(PRECTL prtClipWindow);        
        void Set3rdOperand(G2D_OPERAND3 e3rdOp);            
        
    protected:
        volatile G2D_REG * m_pG2DReg;
        
        void Reset(void);
        void SetEndian(bool bEndian);
        void SetRopValue(DWORD uRopVal);        


        /// For Interrupt Handling
        void IntEnableForDeEngineFinish(void);
        void IntEnableForCmdFinish(void);
        void IntEnableForOverflow(bool bFifo, BYTE ucFifoLevel);    
        void InterruptDisAll(void);        

        int RequestEmptyFifo(DWORD uEmptyFifo); 

        void SetFirstBitBLTData(DWORD uFirstData);
        void SetNextBitBLTData(DWORD uNextData);    

        /// For Rotation Setting
        void SetRotationMode(ROT_TYPE uRotationType);
        void SetRotationOrg(WORD usRotOrgX, WORD usRotOrgY);        
        void SetRotationOrgX(WORD usRotOrgX);
        void SetRotationOrgY(WORD usRotOrgY);    

        /// For Stretching
        inline void SetXIncr(DWORD uXIncr) {    m_pG2DReg->X_INCR = uXIncr;    }
        inline void SetYIncr(DWORD uYIncr) {    m_pG2DReg->Y_INCR = uYIncr;    }
        
        /// For Transfer data region setting
        void SetCoordinateSrcBlock(DWORD uStartX, DWORD uStartY, DWORD uEndX, DWORD uEndY);
        void SetCoordinateDstBlock(DWORD uStartX, DWORD uStartY, DWORD uEndX, DWORD uEndY);        

        
        
};

#endif //__REGCTRL_G2D_H__

