/******************** (c) Marvell Semiconductor, Inc., ************************
 *
 * $Header:
 *
 * Purpose:
 *    This file contains the function prototypes and definitions for the
 *    sdio.c.
 *
 * Public Procedures:
 *
 * Notes:
 *    None.
 *
 *****************************************************************************/


#ifndef _SDIO_H_
#define _SDIO_H_

//=============================================================================
//                                DEFINITIONS
//=============================================================================


#define MRVL_8100_SDIO_VER_ID           0x02 // TODO: what should this be?

// amount of time to wait in ms to check FW again if it is not ready
#define SDIO_FW_NOT_READY_WAIT_TIME     5

#define CF_READ_CMD_BUF_SIZE 128
#define CF_WRITE_CMD_BUF_SIZE 128

#define SDIO_EXTENDED_IO_BLOCK_SIZE  32//64 //32  //256  //128  //512  //32

#define SDIO_IO_READY               0x08
#define SDIO_CIS_CARD_READY         0x04
#define SDIO_UPLOAD_CARD_READY      0x02
#define SDIO_DOWNLOAD_CARD_READY    0x01
#define SDIO_IO_PORT                0x10000

#define SDIO_DISABLE_INTERRUPT      0x0
#define SDIO_ENABLE_INTERRUPT       0x1

#define UPLOAD_HOST_INT_STATUS_RDY      0x01

#define DOWNLOAD_HOST_INT_STATUS_RDY       0x02
#define UPLOADDOWNLOAD_HOST_INT_STATUS_RDY 0x03 

//define for internal driver trigger event
#define MRVL_SOFT_INT_ChipReset             (1<<0)
#define MRVL_SOFT_INT_ExitThread            (1<<1)
#define MRVL_SOFT_INT_AutoDeepSleep         (1<<2)
#define MRVL_SOFT_INT_TxRequest         0x08



// host control register
#define HCR_HOST_INT_STATUS_REGISTER        0x5
#define HCR_HOST_INT_STATUS_RSR_REGISTER    0x6
#define HCR_HOST_INT_MASK_REGISTER          0x4
#define HCR_HOST_CONFIGURATION_REGISTER     0x3
//TT
#define HCR_SDIO_BUS_INTERFACE_CONTROL  0x7
#define HCR_HOST_CARD_STATUS_REGISTER   0x20

#define CARD_REVISION_REG            0x3c

// card control register
#define CCR_CARD_STATUS_REGISTER        0x20

#define LENGTH_SCRATCH_REGISTER                         0x34
#define PACKET_TYPE_MAC_EVENT_SCRATCH_REGISTER          0x36
// code which will be in the scratch register after FW is initialized
#define SDIO_FW_INIT_CODE               0xFEDC

// 50 ms wait time
#define PKT_WAIT_TIME               50000   // number of 10 us units

// block size of FW download
#define SDIO_FW_DOWNLOAD_BLOCK_SIZE 512
#define NOT !



//=============================================================================
//                          PUBLIC TYPE DEFINITIONS
//=============================================================================
//
//

//=============================================================================
//                          FUNCTION HEADER
//=============================================================================

NDIS_STATUS SDIOInitialization(PMRVDRV_ADAPTER pAdapter);
NDIS_STATUS sdio_Initialization( PMRVDRV_ADAPTER Adapter, NDIS_HANDLE WrapperConfigurationContext );
IF_API_STATUS sdio_GetLengthOfDataBlock( PMRVDRV_ADAPTER Adapter, USHORT *pLength );
IF_API_STATUS sdio_GetCardStatusAndMacEvent(  PMRVDRV_ADAPTER Adapter, UCHAR *pCardStatus, UCHAR *pMacEvent );
IF_API_STATUS sdio_GetDataBlock( PMRVDRV_ADAPTER Adapter, USHORT usLength, IF_PKT_TYPE ucCardStatus, UCHAR *p_pkt );
IF_API_STATUS sdio_ReInitCard(PMRVDRV_ADAPTER Adapter); 
IF_API_STATUS sdio_PowerUpDevice(PMRVDRV_ADAPTER Adapter); 
IF_API_STATUS sdio_ClearPowerUpBit(PMRVDRV_ADAPTER Adapter); //071107
IF_API_STATUS sdio_WaitFWPowerUp(PMRVDRV_ADAPTER Adapter);   //081007

 
VOID
sdio_DisableInterrupt(
    IN PMRVDRV_ADAPTER Adapter
    );

VOID
sdio_EnableInterrupt(
    IN PMRVDRV_ADAPTER Adapter
    );

#endif // _SDIO_H_

