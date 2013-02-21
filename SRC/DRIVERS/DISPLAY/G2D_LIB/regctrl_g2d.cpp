//
// Copyright (c) Samsung Electronics CO., LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    regctrl_g2d.cpp

Abstract:        implementation of register controller for FIMGSE-2D

Functions:

Notes:

--*/

#include "precomp.h"
#include "regctrl_g2d.h"

RegCtrlG2D::RegCtrlG2D() : m_pG2DReg(NULL)
{
    PHYSICAL_ADDRESS    ioPhysicalAddress;
    ioPhysicalAddress.LowPart = S3C6410_BASE_REG_PA_2DGRAPHICS;
    m_pG2DReg = (G2D_REG *)MmMapIoSpace(ioPhysicalAddress, sizeof(G2D_REG), FALSE);
    if (m_pG2DReg == NULL)
    {
        RETAILMSG(DISP_ZONE_ERROR, (_T("[DISPDRV:ERR] RegCtrlG2D() : m_pG2DReg MmMapIoSpace() Failed \n\r")));
    }
}

RegCtrlG2D::~RegCtrlG2D()
{
    if (m_pG2DReg != NULL)
    {
        MmUnmapIoSpace((PVOID)m_pG2DReg, sizeof(G2D_REG));
        m_pG2DReg = NULL;
    }
}

/**
*    @fn    RegCtrlG2D::Reset(void)
*    @brief    This function reset 2D IP that clear register set of G2D
*
*/
void RegCtrlG2D::Reset(void)
{
    m_pG2DReg->CONTROL = 1;  //assert G2D reset, automatic clear
}

/**
*    @fn        void RegCtrlG2D::SetEndian(bool bEndian)
*    @brief    Set both Source and Destination data Endian
*    @param    bEndian    Endian setting 1: Big Endian, 0: Little Endian
*/
void RegCtrlG2D::SetEndian(bool bEndian)
{
    RequestEmptyFifo(1);
    m_pG2DReg->ENDIAN = ( (bEndian<<1) | (bEndian<<0) );
}

/**
*    Interrupt Block
*/
void RegCtrlG2D::IntEnable(void)
{
    IntPendingClear();
    m_pG2DReg->INTEN = (0x1<<9);// + (0x1<<8) + (0x1);                //Enable Interrupt
}

void RegCtrlG2D::IntDisable(void)
{
    m_pG2DReg->INTEN &= ~((1<<9) /*+(1<<8) + 1*/);//Disable Interrupt
}

void RegCtrlG2D::IntPendingClear(void)
{
    m_pG2DReg->INTC_PEND = 0x80000701;        // Level Interrupt (Interupt Clear Enable)    // 0x80000401;??
    m_pG2DReg->INTC_PEND = 0x80000000;        // Level Interrupt (Interupt Clear Enable)    
}

void RegCtrlG2D::IntEnableForDeEngineFinish(void)
{
    m_pG2DReg->INTEN = (m_pG2DReg->INTEN)&~(0x7<<8) | (1<<10);
    m_pG2DReg->INTC_PEND = (0x80000000|(7<<8));
    m_pG2DReg->INTC_PEND = 0x80000000;
}
void RegCtrlG2D::IntEnableForCmdFinish(void)
{
    m_pG2DReg->INTEN = ((m_pG2DReg->INTEN)&~(0x7<<8)) | (1<<9);
    m_pG2DReg->INTC_PEND = 0x80000000;
}
void RegCtrlG2D::IntEnableForOverflow(bool bFifo, BYTE ucFifoLevel)
{
    if(bFifo) {
//        m_pG2DReg->INTEN = ((m_pG2DReg->INTEN)&~(0x7<<8))|(1<<8)|1;
        m_pG2DReg->INTEN = ((m_pG2DReg->INTEN)&~(0x7<<8))|1;
        m_pG2DReg->FIFO_INTC = ucFifoLevel;
    }    
    else
    {
        m_pG2DReg->INTEN = ((m_pG2DReg->INTEN)&~(0x7<<8))|(1<<8);
    }
    m_pG2DReg->INTC_PEND = 0x80000000;
}
void RegCtrlG2D::InterruptDisAll(void)
{
    m_pG2DReg->INTEN = (m_pG2DReg->INTEN)&~((3<<8)|1);
}

bool RegCtrlG2D::IsIdle(void)
{
    return ((m_pG2DReg->FIFO_STATUS & G2D_DE_STATUS_FA_BIT) != 0);
}

void RegCtrlG2D::WaitForIdleStatus(void)
{
    while(1)
    {
        if((m_pG2DReg->FIFO_STATUS & G2D_DE_STATUS_FA_BIT))
        {
            break;
        }
    }
}


bool RegCtrlG2D::WaitForFinish(void)
{
    volatile unsigned uPendVal;

    uPendVal = m_pG2DReg->INTC_PEND;

    if( (uPendVal>>8) & 0x7){
        switch( (uPendVal>>8) & 0x7) {
            case 1:
                m_pG2DReg->INTC_PEND = ((1<<31)|(1<<8));        // Overflow
                RETAILMSG (DISP_ZONE_TEMP, (TEXT("OV\r\n"),uPendVal));                            
                break;
            case 2:
                m_pG2DReg->INTC_PEND = ((1<<31)|(1<<9));        // Command All Finish, Engine IDLE
                RETAILMSG (DISP_ZONE_TEMP, (TEXT("EI\r\n"),uPendVal));
                break;
            case 4:
                m_pG2DReg->INTC_PEND = ((1<<31)|(1<<10));    // Drawing Engine Finish
                RETAILMSG (DISP_ZONE_TEMP, (TEXT("EF\r\n"),uPendVal));                
                return false;
            default:
                m_pG2DReg->INTC_PEND = ((1<<31)|(0x7<<8));    // All Clear
                break;
        }
        m_pG2DReg->INTC_PEND = (DWORD)(1<<31); // to clear pending.        


        return true;
    }
    return false;
}

/**
*    Command Block
*/

/**
*    @fn    RegCtrlG2D::RequestEmptyFifo(DWORD uEmptyFifo)
*    @param    uEmptyFifo    Requested Empty FIFO size
*    @return    int        Return Available FIFO Size
*    @note This function will do busy-waiting until requested fifo is empty.
*/
int RegCtrlG2D::RequestEmptyFifo(DWORD uEmptyFifo)
{
    if(uEmptyFifo > G2D_COMMANDFIFO_SIZE)
    {
        RETAILMSG(DISP_ZONE_WARNING, (TEXT("[G2D] Requested Empty fifo is exceeded over maxium value, this will be set maxium value")));
        uEmptyFifo = G2D_COMMANDFIFO_SIZE;
    }
    while( (DWORD)CheckFifo() > (G2D_COMMANDFIFO_SIZE - uEmptyFifo) ); 
    
    DEBUGMSG(DISP_ZONE_2D,(TEXT("Occupied FIFO: %d\n"),CheckFifo() ));    
    return (G2D_COMMANDFIFO_SIZE - CheckFifo());

}

/**
*    @fn    RegCtrlG2D::CheckFifo()
*    @return    int        Return Used FIFO Size
*/
int RegCtrlG2D::CheckFifo()
{
    return (((m_pG2DReg->FIFO_STATUS)& (0x3f<<1))>>1);
}


/*
 *     @fn    void RegCtrlG2D::SetFirstBitBLTData(DWORD uFirstData)
 *    @brief    Set First Source Data to FIMG2D HW Register using CMD register 2, Next Data must be issued by CMD Register 3 continously
 *    @param    uFirstData    32bit surface Surface data
 */
void RegCtrlG2D::SetFirstBitBLTData(DWORD uFirstData)
{
    RequestEmptyFifo(1);
    m_pG2DReg->CMDR2 = uFirstData;
}

/*
 *     @fn    void RegCtrlG2D::SetNextBitBLTData(DWORD uFirstData)
 *    @brief    Set Next Source Data to FIMG2D HW Register using CMD register 2, Next Data must be issued by CMD Register 3
 *    @param    uNextData    32bit surface Surface data
 */
void RegCtrlG2D::SetNextBitBLTData(DWORD uNextData)
{
    RequestEmptyFifo(1);
    m_pG2DReg->CMDR3 = uNextData;
}

/**
*    @fn    void RegCtrlG2D::SetRotationMode(DWORD uRotationType)
*    @param    uRotationType    This is register value for each rotation. It can handle 0, 90, 180, 270, Xflip, Yflip
*    @note G2D's Rotation Register has only 1 bit as enabling.
*/
void RegCtrlG2D::SetRotationMode(ROT_TYPE uRotationType)
{
    RequestEmptyFifo(1);
    m_pG2DReg->ROT_MODE = ((m_pG2DReg->ROT_MODE) & ~0x3f)|(uRotationType);    
}

/**
*    @fn    void RegCtrlG2D::SetRotationOrg(WORD usRotOrg, WORD usRotOrgY)
*    @param    usRotOrgX    X value of Rotation Origin.
*    @param    usRotOrgY    Y value of Rotation Origin.
*    @sa    SetRotationOrgX()
*    @sa SetRotationOrgY()
*    @brief    this function sets rotation origin.
*/
void RegCtrlG2D::SetRotationOrg(WORD usRotOrgX, WORD usRotOrgY)
{
    SetRotationOrgX(usRotOrgX);
    SetRotationOrgY(usRotOrgY);
}

void RegCtrlG2D::SetRotationOrgX(WORD usRotOrgX)
{
    RequestEmptyFifo(1);
    m_pG2DReg->ROT_OC_X = (DWORD) (usRotOrgX & 0x000007FF);
}

void RegCtrlG2D::SetRotationOrgY(WORD usRotOrgY)
{
    RequestEmptyFifo(1);
    m_pG2DReg->ROT_OC_Y = (DWORD) (usRotOrgY & 0x000007FF);
}

/**
*    @fn        void RegCtrlG2D::SetCoordinateSrcBlockEndian(DWORD uStartX, DWORD uStartY, DWORD uEndX, DWORD uEndY)
*    @brief    Set Source Data Area that will be read
*    @param    uStartX    left X 
*            uStartY    top Y
*            uEndX    right X
*            uEndY    bottom Y
*/
void RegCtrlG2D::SetCoordinateSrcBlock(DWORD uStartX, DWORD uStartY, DWORD uEndX, DWORD uEndY)
{
    RequestEmptyFifo(4);
    m_pG2DReg->COORD0_X = uStartX;
    m_pG2DReg->COORD0_Y = uStartY;
    m_pG2DReg->COORD1_X = uEndX;
    m_pG2DReg->COORD1_Y = uEndY;    
}

/**
*    @fn        void RegCtrlG2D::SetCoordinateDstBlockEndian(DWORD uStartX, DWORD uStartY, DWORD uEndX, DWORD uEndY)
*    @brief    Set Destination Data Area that will be written
*    @param    uStartX    left X 
*            uStartY    top Y
*            uEndX    right X
*            uEndY    bottom Y
*/
void RegCtrlG2D::SetCoordinateDstBlock(DWORD uStartX, DWORD uStartY, DWORD uEndX, DWORD uEndY)
{
    RequestEmptyFifo(4);
    m_pG2DReg->COORD2_X = uStartX;
    m_pG2DReg->COORD2_Y = uStartY;
    m_pG2DReg->COORD3_X = uEndX;
    m_pG2DReg->COORD3_Y = uEndY;    
}

/**
*    @fn        void RegCtrlG2D::SetRoptype(DWORD uRopVal)
*    @note    Set Ternary Raster Operation into G2D register directly
*/
void RegCtrlG2D::SetRopValue(DWORD uRopVal)
{
    RequestEmptyFifo(1);
    m_pG2DReg->ROP = ((m_pG2DReg->ROP)&(~0xff)) | uRopVal;
    DEBUGMSG(DISP_ZONE_TEMP,(TEXT("ROPRegAddr : 0x%x, ROP : 0x%x\r\n"),&(m_pG2DReg->ROP), uRopVal));    
}

/**
*    @fn        void RegCtrlG2D::Set3rdOperand(G2D_OPERAND3 e3rdOp)
*    @brief    Set thrid operand as Pattern or Foreground color
*    @param    e3rdOp can be pattern or foreground color
*/
void RegCtrlG2D::Set3rdOperand(G2D_OPERAND3 e3rdOp)
{
    RequestEmptyFifo(1);
    DWORD u3rdOpSel =
        (e3rdOp == G2D_OPERAND3_PAT) ? G2D_OPERAND3_PAT_BIT :
        (e3rdOp == G2D_OPERAND3_FG) ? G2D_OPERAND3_FG_BIT :     0xffffffff;

    if (u3rdOpSel == 0xffffffff)
    {
        RETAILMSG(DISP_ZONE_ERROR, (TEXT("UnSupported Third Operand!\r\n")));
    }

    m_pG2DReg->ROP = ((m_pG2DReg->ROP) & ~(0x1<<13)) | u3rdOpSel;
}

/**
*    @fn        void RegCtrlG2D::SetClipWindow(PRECTL prtClipWindow)
*    @brief    Set Clipping Data Area that will be not cropped.
*    @param    uStartX    left X 
*            uStartY    top Y
*            uEndX    right X
*            uEndY    bottom Y
*/
void RegCtrlG2D::SetClipWindow(PRECTL prtClipWindow)
{
    RequestEmptyFifo(4);
    DEBUGMSG(DISP_ZONE_TEMP,(TEXT("cl:%d, ct:%d, cr:%d, cb:%d\r\n"), prtClipWindow->left, prtClipWindow->top, prtClipWindow->right, prtClipWindow->bottom));
    m_pG2DReg->CW_LEFT_TOP_X = prtClipWindow->left;
    m_pG2DReg->CW_LEFT_TOP_Y =  prtClipWindow->top;
    m_pG2DReg->CW_RIGHT_BOTTOM_X = prtClipWindow->right;
    m_pG2DReg->CW_RIGHT_BOTTOM_Y = prtClipWindow->bottom;
}
