#ifndef _INC_IF_H
#define _INC_IF_H

/*---------------------------------------------------*\
   General part
\*---------------------------------------------------*/

typedef enum _IF_FW_STATUS {
    FW_STATUS_READ_FAILED,
    FW_STATUS_INITIALIZED,
    FW_STATUS_UNINITIALIZED
} IF_FW_STATUS;

typedef enum _IF_API_STATUS {
	IF_SUCCESS = 0,
	IF_FAIL = 1,
} IF_API_STATUS;

#define IF_IS_SUCCESS(x)		((x) == IF_SUCCESS)

typedef enum if_pkt_type {
   IF_DATA_PKT = 0,
   IF_CMD_PKT,
   IF_TX_DONE_EVENT,
   IF_MAC_EVENT,
   IF_INVALID_PKT_TYPE   //dralee, last entry of sdio_pkt_type
} IF_PKT_TYPE;


/*---------------------------------------------------*/
/*---------------------------------------------------*/

#include "sdio.h"
#include "SDCardDDK.h"
#include "if_sdio.h"

#define If_Initialize					sdio_Initialization
#define If_FirmwareDownload				sdio_FirmwareDownload
#define If_IsFirmwareLoaded				sdio_IsFirmwareLoaded
#define If_DownloadPkt					SDIODownloadPkt
#define If_GetLengthOfDataBlock			sdio_GetLengthOfDataBlock
#define If_GetCardStatusAndMacEvent		sdio_GetCardStatusAndMacEvent
#define If_GetDataBlock					sdio_GetDataBlock
#define If_ReadRegister( Ada, Function, Address, ReadAfterWrite, pBuffer, BufferLength) \
	(SDReadWriteRegistersDirect( (Ada)->hDevice, SD_IO_READ, (Function), (Address), (ReadAfterWrite), (pBuffer), (BufferLength) ))

#define If_WriteRegister( Ada, Function, Address, ReadAfterWrite, pBuffer, BufferLength) \
	(SDReadWriteRegistersDirect( (Ada)->hDevice, SD_IO_WRITE, (Function), (Address), (ReadAfterWrite), (pBuffer), (BufferLength) ))
#define If_EnableInterrupt(x)			sdio_EnableInterrupt(x)
#define If_DisableInterrupt(x)			sdio_DisableInterrupt(x) 
#define If_ReInitialize                 SDIOInitialization
#define If_ReInitCard                   sdio_ReInitCard  
//#define If_PowerUpDevice                sdio_PowerUpDevice
//071107
//#define If_ClearPowerUpBit              sdio_ClearPowerUpBit
//081007
//#define If_WaitFWPowerUp                sdio_WaitFWPowerUp

/*---------------------------------------------------*/

#endif
