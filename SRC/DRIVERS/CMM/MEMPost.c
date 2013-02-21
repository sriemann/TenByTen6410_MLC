//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include "MEMPost.h"


UINT  m_uModeRegValue;
UINT  uPixelSzIn, uPixelSzOut;
UINT  m_uSrcFrmStAddr;
UINT  m_uSrcStY, m_uSrcStCb, m_uSrcStCr;
UINT  m_uSrcEndY, m_uSrcEndCb, m_uSrcEndCr;


BOOL POST_WaitforPostDone( void )
{
    UINT    output, i;

    for (i=0;; i++) 
    {
        output = v_pPOST_SFR->POST_START_VIDEO & (0x1<<31);
        if(output == 0)
            break;
        
        if(i == 10000)
        {
            RETAILMSG(1, (TEXT("DD::POST PROCESS FAILED \r\n")));
            return FALSE;
        }
        Sleep(1);
    }
    printD("DD::POST_WaitforPostDone(i : %d)\n", i);
    return TRUE;
}

// disable post processor
void POST_DisableStartBit( void )
{
    UINT uPostModeCtrl2;
    UINT m_uModeRegValue = 0;

    v_pPOST_SFR->POST_START_VIDEO = POST_DISABLE;

    uPostModeCtrl2 = ADDR_CHANGE_ENABLE| CHANGE_AT_FRAME_END| SOFTWARE_TRIGGER;
    v_pPOST_SFR->POST_MODE_CTRL_2 = uPostModeCtrl2;
    //set data path // POST_DMA
    m_uModeRegValue &= ~(0x1U<<31);


    v_pPOST_SFR->POST_MODE_CTRL = m_uModeRegValue; //disable POSTENVID 
}



//set scalar
void POST_SetScaler( UINT uSrcWidth, UINT uDstWidth, UINT uSrcHeight, UINT uDstHeight )
{
    UINT uPreHratio, uPreVratio, uHshift, uVshift, uShFactor;
    UINT uPreDstWidth, uPreDstHeight, uDx, uDy,final_size;

    //Assert( !(uSrcWidth >= (uDstWidth<<6)) );
    //Assert( !(uSrcHeight >= (uDstHeight<<5)) );
    
    if ( uSrcWidth >= (uDstWidth<<5) )
        uPreHratio = 32, uHshift= 5;
    else if ( uSrcWidth >= (uDstWidth<<4) )
        uPreHratio = 16, uHshift = 4;
    else if ( uSrcWidth >= (uDstWidth<<3) )
        uPreHratio = 8, uHshift = 3;
    else if ( uSrcWidth >= (uDstWidth<<2) )
        uPreHratio = 4, uHshift = 2;
    else if ( uSrcWidth >= (uDstWidth<<1) )
        uPreHratio = 2, uHshift = 1;
    else
        uPreHratio = 1, uHshift = 0;
    
    uPreDstWidth = uSrcWidth / uPreHratio;
    uDx = (uSrcWidth<<8) / (uDstWidth<<uHshift);

    if ( uSrcHeight >= (uDstHeight<<5) )
        uPreVratio = 32, uVshift= 5;
    else if ( uSrcHeight >= (uDstHeight<<4) )
        uPreVratio = 16, uVshift = 4;
    else if ( uSrcHeight >= (uDstHeight<<3) )
        uPreVratio = 8, uVshift = 3;
    else if ( uSrcHeight >= (uDstHeight<<2) )
        uPreVratio = 4, uVshift = 2;
    else if ( uSrcHeight >= (uDstHeight<<1) )
        uPreVratio = 2, uVshift = 1;
    else
        uPreVratio = 1, uVshift = 0;
    
    uPreDstHeight = uSrcHeight / uPreVratio;
    uDy = (uSrcHeight<<8) / (uDstHeight<<uVshift);
    uShFactor = 10 - (uHshift + uVshift);


    v_pPOST_SFR->POST_PRESCALE_RATIO = ((uPreVratio<<7)|(uPreHratio<<0)) ; //prescale
    v_pPOST_SFR->POST_PRESCALE_IMG_SZ = ((uPreDstHeight<<12)|(uPreDstWidth<<0)) ;
    v_pPOST_SFR->POST_PRESCALE_SHIFT = uShFactor ; //shift factor
    v_pPOST_SFR->POST_MAIN_SCALE_HRATIO = uDx ; //main scale
    v_pPOST_SFR->POST_MAIN_SCALE_VRATIO = uDy ;
    v_pPOST_SFR->POST_SRC_IMG_SZ = ((uSrcHeight<<12)|(uSrcWidth<<0)) ; //image size
    v_pPOST_SFR->POST_DST_IMG_SZ = ((uDstHeight<<12)|(uDstWidth<<0)) ;
    
    final_size = ((uDstWidth<<0) | (uDstHeight<<12));
    v_pPOST_SFR->POST_DST_IMG_SZ = final_size ;
}




void POST_SetAddrRegAndOffsetReg(
    UINT uSrcFullWidth,        UINT uSrcFullHeight,
    UINT uSrcStartX,        UINT uSrcStartY,
    UINT uSrcWidth,            UINT uSrcHeight,
    UINT uSrcFrmSt,
    UINT eSrcCSpace,
    UINT uDstFullWidth,        UINT uDstFullHeight,
    UINT uDstStartX,        UINT uDstStartY,
    UINT uDstWidth,            UINT uDstHeight,
    UINT uDstFrmSt,
    UINT eDstCSpace
    )
{
    UINT  uOffsetY;
    UINT uOffsetCb, uOffsetCr;
    UINT uDstStRgb, uOffsetRgb, uDstEndRgb;
    UINT uOutOffsetCb, uOutOffsetCr;
    UINT uOutSrcStCb,uOutSrcEndCb,uOutSrcStCr,uOutSrcEndCr;
    UINT eInPath=POST_DMA,eOutPath=POST_DMA;
    UINT  m_uOutStPosCb, m_uOutStPosCr, m_uOutEndPosCb, m_uOutEndPosCr;
    UINT  m_uStPosY, m_uEndPosY;
    UINT  m_uStPosCb, m_uStPosCr, m_uEndPosCb, m_uEndPosCr;
    UINT  m_uStPosRgb, m_uEndPosRgb;

    
    m_uSrcStY=0;
    m_uSrcEndY=0;
    m_uSrcStCb=0;
    m_uSrcEndCb=0;
    m_uSrcStCr=0;
    m_uSrcEndCr=0;

    // input
    if ( eInPath == POST_DMA )
    {    
        uOffsetY = (uSrcFullWidth - uSrcWidth)*uPixelSzIn;
        m_uStPosY = (uSrcFullWidth*uSrcStartY+uSrcStartX)*uPixelSzIn;
        m_uEndPosY = uSrcWidth*uSrcHeight*uPixelSzIn + uOffsetY*(uSrcHeight-1);
        
        m_uSrcFrmStAddr = uSrcFrmSt;
        m_uSrcStY = uSrcFrmSt + m_uStPosY;
        m_uSrcEndY= m_uSrcStY+ m_uEndPosY;
        v_pPOST_SFR->POST_START_ADDR_Y = m_uSrcStY ;
        v_pPOST_SFR->POST_OFFSET_ADDR_Y = uOffsetY ;
        v_pPOST_SFR->POST_END_ADDR_Y = m_uSrcEndY ;
        
        if ( eSrcCSpace == YC420 )
        {
            uOffsetCb, uOffsetCr;
            uOffsetCb = uOffsetCr = ((uSrcFullWidth-uSrcWidth)/2)*uPixelSzIn;
            m_uStPosCb = uSrcFullWidth*uSrcFullHeight*1
                + (uSrcFullWidth*uSrcStartY/2 + uSrcStartX)/2*1;
            m_uEndPosCb = uSrcWidth/2*uSrcHeight/2*uPixelSzIn + (uSrcHeight/2-1)*uOffsetCr;
            m_uStPosCr = uSrcFullWidth*uSrcFullHeight*1 + uSrcFullWidth*uSrcFullHeight/4*1
                + (uSrcFullWidth*uSrcStartY/2 + uSrcStartX)/2*1;
            m_uEndPosCr = uSrcWidth/2*uSrcHeight/2*uPixelSzIn + (uSrcHeight/2-1)*uOffsetCb;
            m_uSrcStCb = uSrcFrmSt + m_uStPosCb;
            m_uSrcEndCb = m_uSrcStCb + m_uEndPosCb;
            m_uSrcStCr = uSrcFrmSt + m_uStPosCr;
            m_uSrcEndCr = m_uSrcStCr + m_uEndPosCr;
            v_pPOST_SFR->POST_START_ADDR_CB = m_uSrcStCb ;
            v_pPOST_SFR->POST_OFFSET_ADDR_CB = uOffsetCr ;
            v_pPOST_SFR->POST_END_ADDR_CB = m_uSrcEndCb ;
            v_pPOST_SFR->POST_START_ADDR_CR = m_uSrcStCr ;
            v_pPOST_SFR->POST_OFFSET_ADDR_CR = uOffsetCb ;
            v_pPOST_SFR->POST_END_ADDR_CR = m_uSrcEndCr ;
        }
    }


    // output
    uOffsetRgb = (uDstFullWidth - uDstWidth)*uPixelSzOut;
    m_uStPosRgb = (uDstFullWidth*uDstStartY + uDstStartX)*uPixelSzOut;
    m_uEndPosRgb = uDstWidth*uDstHeight*uPixelSzOut + uOffsetRgb*(uDstHeight-1);
    uDstStRgb = uDstFrmSt + m_uStPosRgb;
    uDstEndRgb = uDstStRgb + m_uEndPosRgb;
        
    v_pPOST_SFR->POST_START_ADDR_RGB = uDstStRgb ;
    v_pPOST_SFR->POST_OFFSET_ADDR_RGB = uOffsetRgb ;
    v_pPOST_SFR->POST_END_ADDR_RGB = uDstEndRgb ;
    

    if ( eDstCSpace == YC420 )
    {
            uOutOffsetCb = uOutOffsetCr= ((uDstFullWidth-uDstWidth)/2)*uPixelSzOut;
            m_uOutStPosCb = uDstFullWidth*uDstFullHeight*1
                + (uDstFullWidth*uDstStartY/2 + uDstStartX)/2*1;
            m_uOutEndPosCb = uDstWidth/2*uDstHeight/2*uPixelSzOut + (uDstHeight/2-1)*uOutOffsetCr;
            m_uOutStPosCr = uDstFullWidth*uDstFullHeight*1 + uDstFullWidth*uDstFullHeight/4*1
                + (uDstFullWidth*uDstStartY/2 + uDstStartX)/2*1;
            m_uOutEndPosCr = uDstWidth/2*uDstHeight/2*uPixelSzOut + (uDstHeight/2-1)*uOutOffsetCb;
            uOutSrcStCb = uDstFrmSt + m_uOutStPosCb;
            uOutSrcEndCb = uOutSrcStCb + m_uOutEndPosCb;
            uOutSrcStCr = uDstFrmSt + m_uOutStPosCr;
            uOutSrcEndCr = uOutSrcStCr + m_uOutEndPosCr;
            v_pPOST_SFR->POST_ADDR_START_OUT_CB = uOutSrcStCb ;
            v_pPOST_SFR->POST_OFFSET_OUT_CB = uOutOffsetCb ;
            v_pPOST_SFR->POST_ADDR_END_OUT_CB = uOutSrcEndCb ;
            v_pPOST_SFR->POST_ADDR_START_OUT_CR = uOutSrcStCr ;
            v_pPOST_SFR->POST_OFFSET_OUT_CR = uOutOffsetCr ;
            v_pPOST_SFR->POST_ADDR_END_OUT_CR = uOutSrcEndCr ;
    }

    v_pPOST_SFR->POST_MODE_CTRL &=~(0x1<<12); // 0 : progressive 1:interace

    if(eOutPath==POST_DMA)
    {
        v_pPOST_SFR->POST_MODE_CTRL &= ~(0x1<<13);
    }
    else if(eOutPath==POST_FIFO)
    {
        v_pPOST_SFR->POST_MODE_CTRL |= (0x1<<13);
        v_pPOST_SFR->POST_MODE_CTRL |= (0x1<<14);
    }

}




// 현재는 in/out channel을 고정시키고 한다.
int POST_SetDataFormat( UINT eSrcCSpace, UINT eDstCSpace )
{

    m_uModeRegValue |= (0x1<<16); // R2YSel = 1;
    m_uModeRegValue |= (0x2<<10); // Wide = 0x10;


    switch (eSrcCSpace) {
    case YC420:
        m_uModeRegValue &= ~((0x1<<3)|(0x1<2));
        m_uModeRegValue |= (0x1<<8)|(0x1<<1);
        uPixelSzIn = 1;
        break;

    case CRYCBY:
        m_uModeRegValue &= ~((0x1<<15)|(0x1<<8)|(0x1<<3)|(0x1<<0));
        m_uModeRegValue |= (0x1<<2)|(0x1<<1);
        uPixelSzIn = 2;
        break;

    case CBYCRY:
        m_uModeRegValue &= ~((0x1<<8)|(0x1<<3)|(0x1<<0));
        m_uModeRegValue |= (0x1<<15)|(0x1<<2)|(0x1<<1);
        uPixelSzIn = 2;
        break;

    case YCRYCB:
        m_uModeRegValue &= ~((0x1<<15)|(0x1<<8)|(0x1<<3));
        m_uModeRegValue |= (0x1<<2)|(0x1<<1)|(0x1<<0);
        uPixelSzIn = 2;
        break;

    case YCBYCR:
        m_uModeRegValue &= ~((0x1<<8)|(0x1<<3));
        m_uModeRegValue |= (0x1<<15)|(0x1<<2)|(0x1<<1)|(0x1<<0);    
        uPixelSzIn = 2;
        break;

    case RGB24:
        m_uModeRegValue &= ~(0x1<<8);
        m_uModeRegValue |=  (0x1<<3)|(0x1<<2)|(0x1<<1);
        uPixelSzIn = 4;
        break;

    case RGB16:
        m_uModeRegValue &= ~((0x1<<8)|(0x1<<1));
        m_uModeRegValue |=  (0x1<<3)|(0x1<<2);
        uPixelSzIn = 2;
        break;

    default:
        return -1;

    }


    switch (eDstCSpace) {
    case YC420:
        m_uModeRegValue &= ~(0x1<<18);
        m_uModeRegValue |= (0x1<<17);
        uPixelSzOut = 1;
        break;

    case CRYCBY:
        m_uModeRegValue &= ~((0x1<<20)|(0x1<<19)|(0x1<<18)|(0x1<<17));
        uPixelSzOut = 2;
        break;

    case CBYCRY:
        m_uModeRegValue &= ~((0x1<<19)|(0x1<<18)|(0x1<<17));
        m_uModeRegValue |= (0x1<<20);
        uPixelSzOut = 2;
        break;

    case YCRYCB:
        m_uModeRegValue &= ~((0x1<<20)|(0x1<<18)|(0x1<<17));
        m_uModeRegValue |= (0x1<<19);
        uPixelSzOut = 2;
        break;

    case YCBYCR:
        m_uModeRegValue &= ~((0x1<<18)|(0x1<<17));
        m_uModeRegValue |= (0x1<<20)|(0x1<<19);    
        uPixelSzOut = 2;
        break;
        
    case RGB30:
        m_uModeRegValue |=  (0x1<<18)|(0x1<<13)|(0x1<<4);
        uPixelSzIn = 4;
        break;

    case RGB24:
        m_uModeRegValue |= (0x1<<18)|(0x1<<4);
        uPixelSzOut = 4;
        break;

    case RGB16:
        m_uModeRegValue &= ~(0x1<<4);
        m_uModeRegValue |= (0x1<<18);
        uPixelSzOut = 2;
        break;

    default:
        return -1;

    }

    
    v_pPOST_SFR->POST_MODE_CTRL = m_uModeRegValue ;

    return 0;
}


void POST_Start()
{

    v_pPOST_SFR->POST_START_VIDEO = POST_START; // khlee

}

void PostProcess(UINT uSrcFrmSt,     UINT uDstFrmSt,
                   UINT srcFrmFormat,  UINT dstFrmFormat,
                   UINT srcFrmWidth,   UINT srcFrmHeight,
                   UINT dstFrmWidth,   UINT dstFrmHeight,
                   UINT srcXOffset,    UINT srcYOffset,
                   UINT dstXOffset,    UINT dstYOffset,
                   UINT srcCropWidth,  UINT srcCropHeight,
                   UINT dstCropWidth,  UINT dstCropHeight
                   )
{

    ////////////////////////////
    // 1. Stop Post Processor //
    ////////////////////////////
    POST_DisableStartBit();


    //////////////////////////
    // 2. Set Color Format  //
    //////////////////////////
    POST_SetDataFormat(srcFrmFormat, dstFrmFormat);


    ////////////////////
    // 3. Set Scaler  //
    ////////////////////
    POST_SetScaler(srcCropWidth, dstCropWidth, srcCropHeight, dstCropHeight);

    ////////////////////////////////
    // 4. Set Addresses & Offsets //
    ////////////////////////////////
    POST_SetAddrRegAndOffsetReg(srcFrmWidth,  srcFrmHeight,
                                srcXOffset,   srcYOffset,
                                srcCropWidth, srcCropHeight,
                                uSrcFrmSt,
                                srcFrmFormat,
                                dstFrmWidth,  dstFrmHeight,
                                dstXOffset,   dstYOffset,
                                dstCropWidth, dstCropHeight,
                                uDstFrmSt,
                                dstFrmFormat);

    POST_Start();


}
