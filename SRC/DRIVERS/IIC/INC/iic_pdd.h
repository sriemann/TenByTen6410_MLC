/**************************************************************************************
* 
*    Project Name : IIC Driver 
*
*    Copyright 2006 by Samsung Electronics, Inc.
*    All rights reserved.
*
*    Project Description :
*        This software is PDD layer for IIC Samsung driver. 
*  
*--------------------------------------------------------------------------------------
* 
*    File Name : iic_pdd.h
*  
*    File Description : This file declare PDD functions for IIC driver.
*
*    Author : JeGeon.Jung
*    Dept. : AP Development Team
*    Created Date : 2007/06/12
*    Version : 0.1 
* 
*    History
*    - Created(JeGeon.Jung 2007/06/12)
*  
*    Todo
*
*
*    Note
*
**************************************************************************************/
#ifndef __IIC_PDD_H__
#define __IIC_PDD_H__

#if __cplusplus
extern "C"
{
#endif

#include <iic.h>

#define DEFAULT_SLAVE_ADDRESS        0xc0
#define DEFAULT_INTERRUPT_ENABLE    1

// I2C Master Commands (IICSTAT)
#define M_IDLE          0x00    // Disable Rx/Tx
#define M_ACTIVE      0x10    // Enable Rx/Tx
#define MTX_START    0xF0    // Master Tx Start
#define MTX_STOP      0xD0    // Master Tx Stop
#define MRX_START    0xB0    // Master Rx Start
#define MRX_STOP      0x90    // Master Rx Stop

// I2C State (IICSTAT)
#define ARBITRATION_FAILED              0x08
#define SLAVE_ADDRESS_MATCHED      0x04
#define SLAVE_ADDRESS_ZERO             0x02
#define ACK_NOT_RECEIVED                  0x01

//        declare functions
BOOL         HW_Init         (PHW_INIT_INFO pInitContext);
VOID         HW_Deinit         (PHW_INIT_INFO pInitContext);
BOOL         HW_OpenFirst     (PHW_OPEN_INFO pOpenContext);
BOOL         HW_CloseLast     (PHW_OPEN_INFO pOpenContext);

BOOL         HW_Open             (PHW_OPEN_INFO pOpenContext);
BOOL         HW_Close        (PHW_OPEN_INFO pOpenContext);

BOOL        HW_PowerUp        (PHW_INIT_INFO pInitContext);
BOOL        HW_PowerDown    (PHW_INIT_INFO pInitContext);

VOID         HW_SetRegister     (PHW_OPEN_INFO pOpenContext);
VOID         HW_SetClock        (PHW_OPEN_INFO pOpenContext);

BOOL        HW_Read             (PHW_OPEN_INFO pOpenContext, PIIC_IO_DESC pInData ,PIIC_IO_DESC pOutData);
BOOL        HW_Write        (PHW_OPEN_INFO pOpenContext, PIIC_IO_DESC pInData);

#if __cplusplus
}
#endif

#endif //    __IIC_PDD_H__
