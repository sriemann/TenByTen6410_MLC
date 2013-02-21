/*******************  Marvell Semiconductor, Inc., ******************************
 *
 *  Purpose:
 *
 *      This module has the implementation of SDIO related utilities
 *
 *  Notes:
 *
 *  $Author: yyi $
 *
 *  $Date: 2004/12/09 18:35:17 $
 *
 *  $Revision: 1.6 $
 *
 *****************************************************************************/

#include "precomp.h"
#include "Firmware.h"
#include "sdio.h"
#include "SDCardDDK.h"
#include "igxBug.h"
#include <celog.h>

#define     SCRATCHREGFUNCNUM     1

// Define this to non-zero to add CELOG info for debugging
#define WLANDRV_CELOG_TRACE         FALSE

// structure used for fw download is a little different
// from regular packet download
struct _SDIO_FW_DWLD_PKT
{    
    ULONG  Len;    
    UCHAR  Buf[SDIO_FW_DOWNLOAD_BLOCK_SIZE];
};


// marked by ice.zhang  20080827
/*
typedef enum {
  SD_CARD_SELECT_REQUEST=9,
  SD_CARD_DESELECT_REQUEST,
  SD_CARD_FORCE_RESET,
  SD_IS_FAST_PATH_AVAILABLE =12,
  SD_FAST_PATH_DISABLE,
  SD_FAST_PATH_ENABLE,
  SD_IO_FUNCTION_HIGH_POWER,
  SD_IO_FUNCTION_LOW_POWER,
  SD_INFO_POWER_CONTROL_STATE,
  SD_SET_CARD_INTERFACE_EX,
  SD_SET_SWITCH_FUNCTION,
  SD_DMA_ALLOC_PHYS_MEM,
  SD_DMA_FREE_PHYS_MEM
} SD_SET_FEATURE_TYPE, *PSD_SET_FEATURE_TYPE;
*/

///////////////////////////////////////////////////////////////////
//  sdio_IsFirmwareLoaded - check if FW has been loaded/initialized
//  Input:  Adapter - Adapter context
//       
//  Output: 
//  Returns:  return SDIO_FW_STATUS
//  Notes:  
///////////////////////////////////////////////////////////////////
IF_FW_STATUS sdio_IsFirmwareLoaded( 
    IN PMRVDRV_ADAPTER pAdapter
)
{

    USHORT          usLength;
    SD_API_STATUS   status;          // intermediate status

    // read scratch register for FW init code, function number is 0
    status = SDReadWriteRegistersDirect(pAdapter->hDevice,
                                        SD_IO_READ ,
                                        SCRATCHREGFUNCNUM,
                                        LENGTH_SCRATCH_REGISTER, 
                                        FALSE,
                                        (UCHAR *)&usLength,
                                        sizeof(usLength));
    if (!SD_API_SUCCESS(status))
    {
        DBGPRINT(DBG_LOAD|DBG_OID|DBG_WARNING, (L"sdio_IsFirmwareLoaded: Unable to read FW init code\n"));
        return FW_STATUS_READ_FAILED;
    }

    
    if ( usLength == SDIO_FW_INIT_CODE )
    {
        // read the expected code
    
        DBGPRINT(DBG_LOAD, 
            (L"sdio_IsFirmwareLoaded Returning code SUCCESS (FW_STATUS_INITIALIZED)\n"));
        return FW_STATUS_INITIALIZED;
    }

    DBGPRINT(DBG_LOAD, (L"sdio_IsFirmwareLoaded Returning code FW_STATUS_UNINITIALIZED\n"));

    return FW_STATUS_UNINITIALIZED;
}


///////////////////////////////////////////////////////////////////
//  sdio_IsFirmwareLoaded - check if FW has been loaded/initialized
//  Input:  Adapter - Adapter context
//       
//  Output: 
//  Returns:  return SDIO_FW_STATUS
//  Notes:  
///////////////////////////////////////////////////////////////////
IF_FW_STATUS sdio_FirmwareDownload(
    IN PMRVDRV_ADAPTER Adapter
)
{
    UINT    blockCount = 0;
    UINT    sizeOfFirmware,sizeOfHelper;
    UINT    sizeSend = 0;
    USHORT  sizeBlock; 
    ULONG   firmwareDelay = 0;
#define FWDELAY 200             // cycles should give us about 10 seconds   
#define FWBLOCK_DELAY  5000     // cycles for fw block delay.
    struct _SDIO_FW_DWLD_PKT DownloadPkt;
    SD_API_STATUS           status;          // intermediate status
    SD_TRANSFER_CLASS       transferClass;   // general transfer class
    DWORD                   argument;        // argument
    ULONG                   numBlocks;       // number of blocks
    SD_COMMAND_RESPONSE     response;        // IO response status
    UCHAR                   ucCardStatus;
    UCHAR                  ucLen[2];
    ULONG                   loopCount;
    BOOLEAN                 startFirmware;   // send down firmware start
                                             // pkt in the next round
    BOOLEAN                 exitLoop;
        UCHAR    regValue;
    
  //
  // Set up NIC HW (include firmwware download)
  //
        regValue = 3;
        // enable Client UpLdCardRdy interrup
        status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                            SD_IO_WRITE,          
                                            1,     
                                            HCR_HOST_INT_MASK_REGISTER ,   // address of Card Status register
                                            FALSE,
                                            &regValue,   // reg
                                            1);   
        if (!SD_API_SUCCESS(status))
        {
             DBGPRINT(DBG_ERROR,(L"SDIO Samp: Failed to enable UpLdCardRdy interrupt: 0x%08X  \n",status));
             return FW_STATUS_UNINITIALIZED;
        } 

    //Download helper file
    sizeOfHelper = sizeof(helperimage);

    DBGPRINT(DBG_LOAD|DBG_HELP,
                      (L"INIT - Helper, Helper size = %d bytes, block size=%d bytes\n", 
                       sizeOfHelper, SDIO_FW_DOWNLOAD_BLOCK_SIZE));

    RETAILCELOGMSG(WLANDRV_CELOG_TRACE, (L"SDIO8686: Download Helper image\n"));

    startFirmware=FALSE;
    exitLoop = FALSE;
    while ( ! exitLoop )
    {
        blockCount++;

        if ( TRUE != startFirmware )
        {
            // regular download
            sizeBlock = SDIO_FW_DOWNLOAD_BLOCK_SIZE;

            // last pkt is not necessarily full pkt
            if ( (sizeSend + sizeBlock) >= sizeOfHelper )
            {
                sizeBlock = sizeOfHelper - sizeSend;
                startFirmware = TRUE;
            }
            // Move the data from firmware buf to the command header
            NdisMoveMemory(DownloadPkt.Buf, &helperimage[sizeSend], sizeBlock);
        }
        else
        {
            // download pkt of size 0 to kick start firmware
            sizeBlock = 0;

            // exit loop at the complete of the current loop
            exitLoop = TRUE;
        }

        DownloadPkt.Len = sizeBlock;
        
        // this is almost like downloading regular pkt

        loopCount = PKT_WAIT_TIME;

        RETAILCELOGMSG(WLANDRV_CELOG_TRACE, (L"SDIO8686: Wait for device to be ready (%d) Block=%d\n", 1, blockCount));

        while ( loopCount != 0 )
        {
            // need to check if FW is ready for download

            // read card status register (function 1,addr 20)
            status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                                SD_IO_READ ,
                                                1, // function 1
                                                HCR_HOST_CARD_STATUS_REGISTER, // reg 0x20
                                                FALSE,
                                                &ucCardStatus,
                                                sizeof(ucCardStatus));
            if (!SD_API_SUCCESS(status))
            {       
                return FW_STATUS_READ_FAILED; 
            }

            if ( ( NOT (ucCardStatus & SDIO_IO_READY) )  ||  
                 ( NOT (ucCardStatus & SDIO_DOWNLOAD_CARD_READY) ) )
            {
                // Not ready for packet download
        
                loopCount--;
                //NdisStallExecution(10); // stall for 10 us
                NdisMSleep(10);
            }
            else
            {
                break;
            }
        }

        if ( loopCount == 0 )
        {
            DBGPRINT(DBG_LOAD|DBG_ERROR,(L"Downloading FW died on block %d\n",blockCount));
            return FW_STATUS_UNINITIALIZED; 
        }

        // Len field ONLY accounts for the buffer size
        numBlocks = (DownloadPkt.Len + sizeof(ULONG)) / 
                            SDIO_EXTENDED_IO_BLOCK_SIZE;

        if ( ((DownloadPkt.Len + sizeof(ULONG)) % 
                    SDIO_EXTENDED_IO_BLOCK_SIZE) != 0 )
        {
            numBlocks++;
        }

        // write, block mode, address starts at 0, fixed address
        argument =  BUILD_IO_RW_EXTENDED_ARG(SD_IO_OP_WRITE, 
                                             SD_IO_BLOCK_MODE, 
                                             1, // function number is 1 
                                             SDIO_IO_PORT , 
                                             SD_IO_FIXED_ADDRESS, 
                                             numBlocks);
        transferClass = SD_WRITE;
        status = SDSynchronousBusRequest(Adapter->hDevice, 
                                         SD_CMD_IO_RW_EXTENDED,
                                         argument,
                                         transferClass, 
                                         ResponseR5,
                                         &response, 
                                         numBlocks,
                                         SDIO_EXTENDED_IO_BLOCK_SIZE, 
                                         (PUCHAR)&DownloadPkt,
                                         0); 
        if (!SD_API_SUCCESS(status))
        {
            DBGPRINT(DBG_LOAD,(L"Downloading FW died on block %d\n",blockCount));
            return FW_STATUS_UNINITIALIZED; 
        }
        DBGPRINT(DBG_LOAD,(L"FW Download block #%d\n",blockCount));
        sizeSend += sizeBlock;
    } 
 
    while(1)
    {
        //    MessageBox(NULL, TEXT("Download Helper Module"), TEXT("SD8385"), MB_OK);
        // read card status register (function 1,addr 20)
        status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                                SD_IO_READ ,
                                                1, // function 1
                                                0x10, // reg 0x20
                                                FALSE,
                                                ucLen,
                                                2);

        if (!SD_API_SUCCESS(status))
        {           
           return FW_STATUS_READ_FAILED; 
        }

        if(*((PUSHORT)(ucLen)) == 0x10)
        {
            DBGPRINT(DBG_LOAD,(L"Download helper size == %x !!\n",*((PUSHORT)(ucLen))));
            break;
        }
        DBGPRINT(DBG_LOAD,(L"Download helper size == %x !!\n",*((PUSHORT)(ucLen))));
    }
    DBGPRINT(DBG_LOAD,(L"Finished to download Helper !!\n"));
    //Download helper file-- 

    sizeOfFirmware = sizeof(fmimage);

    blockCount = 0;
    sizeSend = 0;
    
    DBGPRINT(DBG_LOAD|DBG_WARNING,
        (L"INIT - Firmware download start, FW size = %d bytes, block size=%d bytes\n", 
            sizeOfFirmware, SDIO_FW_DOWNLOAD_BLOCK_SIZE));
    //MessageBox(NULL, TEXT("Download Firmware Now"), TEXT("SD25"), MB_OK);
   

    RETAILCELOGMSG(WLANDRV_CELOG_TRACE, (L"SDIO8686: Download Firmware image\n"));

    startFirmware=FALSE;
    exitLoop = FALSE;
    while ( ! exitLoop )
    {
        blockCount++;

        if ( TRUE != startFirmware )
        {
            //MessageBox(NULL, TEXT("Download Read len size"), TEXT("SD25"), MB_OK);

            status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                                SD_IO_READ ,
                                                1, // function 1
                                                0x10, // reg 0x20
                                                FALSE,
                                                ucLen,
                                                2);
            DBGPRINT(DBG_LOAD|DBG_HELP,(L"Download firmare module size == %x !!\n",*((PUSHORT)(ucLen))));

            if (!SD_API_SUCCESS(status))
            {        
                return FW_STATUS_READ_FAILED; 
            }

            if(*((PUSHORT)(ucLen)) == 0)
            {
                 DBGPRINT(DBG_LOAD|DBG_HELP,(L"End of download firmware!!\n"));
                 break;
            }

            if((*((PUSHORT)(ucLen)) & 1) == 1)
            {
                 DBGPRINT(DBG_LOAD|DBG_ERROR,(L"download firmware with CRC error!!\n"));
                 //MessageBox(NULL, TEXT("Download with CRC error"), TEXT("SD25"), MB_OK);
            }
            else
            {
                 DBGPRINT(DBG_LOAD|DBG_HELP,(L"download firmware module success!!\n"));
                 // regular download
                 sizeBlock = *((PUSHORT)(ucLen));
            }

            // last pkt is not necessarily full pkt
            if ( (sizeSend + sizeBlock) >= sizeOfFirmware )
            {
                sizeBlock = sizeOfFirmware - sizeSend;
                startFirmware = TRUE;
                exitLoop = TRUE;
                //MessageBox(NULL, TEXT("Download last one"), TEXT("SD25"), MB_OK);
            }
             
            // Move the data from firmware buf to the command header
            NdisMoveMemory(DownloadPkt.Buf, &fmimage[sizeSend], sizeBlock);
        } 

        DownloadPkt.Len = sizeBlock;
        // Len field ONLY accounts for the buffer size
        numBlocks = (((DownloadPkt.Len + 127)/128)*128) / 
                            SDIO_EXTENDED_IO_BLOCK_SIZE;

        if ( ((((DownloadPkt.Len + 127)/128)*128) % 
                    SDIO_EXTENDED_IO_BLOCK_SIZE) != 0 )
        {
            numBlocks++;
        }

        DBGPRINT(DBG_LOAD|DBG_HELP, (L"NUmberblock = %d\r\n",numBlocks));

        // write, block mode, address starts at 0, fixed address
        argument =  BUILD_IO_RW_EXTENDED_ARG(SD_IO_OP_WRITE, 
                                             SD_IO_BLOCK_MODE, 
                                             1, // function number is 1 
                                             SDIO_IO_PORT , 
                                             SD_IO_FIXED_ADDRESS, 
                                             numBlocks);
        transferClass = SD_WRITE;

        status = SDSynchronousBusRequest(Adapter->hDevice, 
                                         SD_CMD_IO_RW_EXTENDED,
                                         argument,
                                         transferClass, 
                                         ResponseR5,
                                         &response, 
                                         numBlocks,
                                         SDIO_EXTENDED_IO_BLOCK_SIZE, 
                                         (PUCHAR)&DownloadPkt.Buf,
                                         0); 

        if (!SD_API_SUCCESS(status))
        {
            DBGPRINT(DBG_LOAD|DBG_ERROR,(L"Downloading FW died on block %d\n",blockCount));
            return FW_STATUS_UNINITIALIZED; 
        }

        DBGPRINT(DBG_LOAD|DBG_HELP,(L"FW Download block #%d\n",blockCount));
            
        sizeSend += sizeBlock;

        loopCount = PKT_WAIT_TIME;

        RETAILCELOGMSG(WLANDRV_CELOG_TRACE, (L"SDIO8686: Wait for device to be ready (%d) Block=%d\n", 2, blockCount));

        while ( loopCount != 0 )
        {
            // need to check if FW is ready for download

            // read card status register (function 1,addr 20)
            status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                                SD_IO_READ ,
                                                1, // function 1
                                                HCR_HOST_CARD_STATUS_REGISTER, // reg 0x20
                                                FALSE,
                                                &ucCardStatus,
                                                sizeof(ucCardStatus));

            if (!SD_API_SUCCESS(status))
            {
                DBGPRINT(DBG_LOAD,(L"card status is fail!!\n")); 
                return FW_STATUS_READ_FAILED; 
            }

            if ( ( NOT (ucCardStatus & SDIO_IO_READY) )  ||  
                 ( NOT (ucCardStatus & SDIO_DOWNLOAD_CARD_READY) ) )
            {
                // Not ready for packet download
        
                loopCount--;
                //NdisStallExecution(10); // stall for 10 us
                NdisMSleep(10);
            }
            else
            {
                break;
            }
        }

        if ( loopCount == 0 )
        {
            
            DBGPRINT(DBG_LOAD,(L"Downloading FW died on block %d\n",blockCount));

            return FW_STATUS_UNINITIALIZED; 
        }
        DBGPRINT(DBG_LOAD,(L"Downloading FW loop-back \n"));

    }
//    DBGPRINT(DBG_LOAD,(L"End End !!!! \n"));
//    MessageBox(NULL, TEXT("End End!!!!!"), TEXT("SD25"), MB_OK);

    loopCount = PKT_WAIT_TIME;

    RETAILCELOGMSG(WLANDRV_CELOG_TRACE, (L"SDIO8686: Wait for device to be ready (%d)\n", 3));
    
    while ( loopCount != 0 )
    {
        if ( sdio_IsFirmwareLoaded(Adapter) != FW_STATUS_INITIALIZED )
        {        
            // FW not ready    
            loopCount--;
            //NdisStallExecution(10); // stall for 10 us
            NdisMSleep(10);
        }
        else
        {
            // FW ready!
            DBGPRINT(DBG_LOAD,(L"FW started SUCCESSFULLY\n"));

            return FW_STATUS_INITIALIZED;

            break;
        }
    } 

    DBGPRINT(DBG_LOAD|DBG_HELP,(L"FW DIDNOT start successfully\r\n"));
    return FW_STATUS_UNINITIALIZED;
}

///////////////////////////////////////////////////////////////////
//  SDIODownloadPkt - download a SDIO paket to FW
//  Input:  Adapter - Adapter context
//          DownloadPkt - Pkt to be downloaded
//  Output: 
//  Returns:  SD_API_STATUS code
//  Notes:  
///////////////////////////////////////////////////////////////////
SD_API_STATUS SDIODownloadPkt(
    IN PMRVDRV_ADAPTER   Adapter,
    IN PSDIO_TX_PKT      pDownloadPkt
)
{

    SD_API_STATUS           status;          // intermediate status
    SD_TRANSFER_CLASS       transferClass;   // general transfer class
    DWORD                   argument;        // argument
    ULONG                   numBlocks;       // number of blocks
    SD_COMMAND_RESPONSE     response;        // IO response status

    // command buffer and the header required for SDIO
    numBlocks = pDownloadPkt->Len / SDIO_EXTENDED_IO_BLOCK_SIZE;

    if ( (pDownloadPkt->Len % SDIO_EXTENDED_IO_BLOCK_SIZE) != 0 )
    {
        numBlocks++;
    } 


    if ( pDownloadPkt->Len > 1520 )
    {
        DBGPRINT(DBG_ISR, (L"Download PKT has length greater than 1520! (%d)\n", pDownloadPkt->Len));
    }

    // write, block mode, address starts at 0, fixed address
    argument =  BUILD_IO_RW_EXTENDED_ARG(SD_IO_OP_WRITE, 
                                         SD_IO_BLOCK_MODE, 
                                         1, // function number is 1 
                                         SDIO_IO_PORT , 
                                         SD_IO_FIXED_ADDRESS, 
                                         numBlocks);
    transferClass = SD_WRITE;

    status = SDSynchronousBusRequest(Adapter->hDevice, 
                                     SD_CMD_IO_RW_EXTENDED,
                                     argument,
                                     transferClass, 
                                     ResponseR5,
                                     &response, 
                                     numBlocks,
                                     SDIO_EXTENDED_IO_BLOCK_SIZE, 
                                     (PUCHAR)pDownloadPkt,
                                     0); 

   
    // return the status, no need to check whether the download
    // was successful or not

    // check if it's packet or command response
    if (!SD_API_SUCCESS(status))
    {
        //EnableInterrupt(pAdapter);
        DBGPRINT(DBG_ERROR, (L"SDIODownloadPkt():Unable to download packet to FW!\n"));
        return status;
    }

    // record the first 32 bytes in case if the buffer needs to be resent
    NdisMoveMemory(Adapter->LastFWBuffer, pDownloadPkt, 32);

    return status;
}

/******************************************************************************
 *
 * Name: EnableInterrupt()
 *
 *
 * Description: 
 *    This routine enables interrupts
 *
 *  Arguments:
 *    
 *  Return Value:
 * 
 *  Notes:                
 *
 *****************************************************************************/

VOID
sdio_EnableInterrupt(PMRVDRV_ADAPTER Adapter)
{
    
    UCHAR   ucInterrupt;

    SD_API_STATUS status;
    ucInterrupt =0x3;
    
    return; 

    status=SDReadWriteRegistersDirect(Adapter->hDevice,
                                    SD_IO_WRITE,          
                                    1,     
                                    HCR_HOST_INT_MASK_REGISTER,
                                    FALSE,
                                    &ucInterrupt,   // reg
                                    sizeof(ucInterrupt)); 
                                    
    if (!SD_API_SUCCESS(status))
    {
        DBGPRINT(DBG_ERROR,(L"Error: *** Enable Interrupt ***\n\n"));
         
    }
     return;
    
}


/******************************************************************************
 *
 * Name: DisableInterrupt()
 *
 *
 * Description: 
 *    This routine disables interrupts 
 *
 *  Arguments:
 *      DriverObject - Pointer to driver object created by the system.
 *    
 *  Return Value:
 * 
 *  Notes:                
 *
 *****************************************************************************/

VOID
sdio_DisableInterrupt(
     IN PMRVDRV_ADAPTER Adapter
)
{
    UCHAR   ucInterrupt;



      SD_API_STATUS status;
      ucInterrupt = 0x0;
      
      return;
    
    status=SDReadWriteRegistersDirect(Adapter->hDevice,
                                    SD_IO_WRITE,          
                                    1,     
                                    HCR_HOST_INT_MASK_REGISTER,
                                    FALSE,
                                    &ucInterrupt,   // reg
                                    sizeof(ucInterrupt)); 
                                      

    if (!SD_API_SUCCESS(status))
    {
        DBGPRINT(DBG_ERROR,(L"Error: *** Disable Interrupt ***\n\n"));
         
    }
    return;

}



/******************************************************************************
 *
 *  Name: SDIO_ReadCommandReponse()
 *
 *  Description: Read command response from firmware
 *
 *  Arguments:  PMRVDRV_ADAPTER  Adapter, 
 *              PVOID           BufVirtualAddr
 *    
 *  Return Value:        
 * 
 *  Notes: 
 *
 *****************************************************************************/
BOOL SDIO_ReadCommandReponse(
        IN  PMRVDRV_ADAPTER Adapter, 
        OUT PVOID BufVirtualAddr)
{

    SD_API_STATUS          status;          // intermediate status
    SD_TRANSFER_CLASS      transferClass;   // general transfer class
    DWORD                  argument;        // argument
    ULONG                  numBlocks;       // number of blocks
    SD_COMMAND_RESPONSE    response;        // IO response status
    HostCmd_DS_CODE_DNLD   cmdDownload;

    
    
    
    // read, block mode, address at 0, incrementing address
    // first read 1 block
    argument =  BUILD_IO_RW_EXTENDED_ARG(SD_IO_OP_READ, 
                                         SD_IO_BLOCK_MODE, 
                                         0, 
                                         0, 
                                         SD_IO_FIXED_ADDRESS, 
                                         1);
    transferClass = SD_READ;

    status = SDSynchronousBusRequest(Adapter->hDevice, 
                                     SD_CMD_IO_RW_EXTENDED,
                                     argument,
                                     transferClass, 
                                     ResponseR5,
                                     &response, 
                                     1,
                                     SDIO_EXTENDED_IO_BLOCK_SIZE, 
                                     (PUCHAR)&cmdDownload,
                                     0); 
    
      
    // TODO: may need to check for pkt type to see if it's command

    if ( cmdDownload.Len > SDIO_EXTENDED_IO_BLOCK_SIZE )
    {
        // more blocks
        numBlocks = (cmdDownload.Len + (sizeof(USHORT) * 2)) / 
                        SDIO_EXTENDED_IO_BLOCK_SIZE;

        if ( ((cmdDownload.Len + (sizeof(USHORT) * 2)) % 
                        SDIO_EXTENDED_IO_BLOCK_SIZE) != 0 )
        {
            // one last block
            numBlocks++;
        }

        // already read the first block
        numBlocks--;

        argument =  BUILD_IO_RW_EXTENDED_ARG(SD_IO_OP_READ, 
                                         SD_IO_BLOCK_MODE, 
                                         0, 
                                         0, 
                                         SD_IO_FIXED_ADDRESS, 
                                         numBlocks);

        status = SDSynchronousBusRequest(Adapter->hDevice, 
                                     SD_CMD_IO_RW_EXTENDED,
                                     argument,
                                     transferClass, 
                                     ResponseR5,
                                     &response, 
                                     numBlocks,
                                     SDIO_EXTENDED_IO_BLOCK_SIZE, 
                                     ((PUCHAR)&cmdDownload + 
                                       SDIO_EXTENDED_IO_BLOCK_SIZE),
                                     0); 

    }

    // copy back to the command buffer
    NdisMoveMemory(BufVirtualAddr, cmdDownload.Code, cmdDownload.Len);

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// SDNdisGetSDDeviceHandle - Get the SD Device handle
// Inputs:  
// Outputs:
// returns: NDIS_SUCCESS if successful
// Notes:
//          The bus driver loads NDIS and stores the SD Device Handle context
//          in NDIS's device active key.  This function scans the NDIS 
//          configuration for the ActivePath to feed into the
//          SDGetDeviceHandle API.
/////////////////////////////////////////////////////////////////////////////
NDIS_STATUS SDNdisGetSDDeviceHandle(PMRVDRV_ADAPTER pAdapter)
{

    NDIS_STATUS status;                       // intermediate status
    NDIS_HANDLE configHandle;
    NDIS_STRING activePathKey = NDIS_STRING_CONST("ActivePath");
    PNDIS_CONFIGURATION_PARAMETER pConfigParm;  

       // open a handle to our configuration in the registry
    NdisOpenConfiguration(&status,
                          &configHandle,
                          pAdapter->ConfigurationHandle);

    if (!NDIS_SUCCESS(status)) {
        DBGPRINT(DBG_LOAD | DBG_ERROR, 
            (L"SDNdis: NdisOpenConfiguration failed (0x%08X)\n",
                   status));
        return status;
    }

        // read the ActivePath key set by the NDIS loader driver
    NdisReadConfiguration(&status,
                          &pConfigParm,
                          configHandle,
                          &activePathKey,
                          NdisParameterString);

    if (!NDIS_SUCCESS(status)) {
        DBGPRINT(DBG_LOAD | DBG_ERROR, 
            (L"SDNdis: Failed to get active path key (0x%08X)\n",
                   status));
             // close our registry configuration
			RETAILMSG(1, (L"***!!!!!!NdisReadConfiguration  failed\r\n"));
        NdisCloseConfiguration(configHandle);
        return status;
    }

    if (NdisParameterString != pConfigParm->ParameterType) {
        DBGPRINT(DBG_LOAD | DBG_ERROR, 
            (L"SDNdis: PARAMETER TYPE NOT STRING!!!\n"));
             // close our registry configuration
        NdisCloseConfiguration(configHandle);
        return status;
    }

    if (pConfigParm->ParameterData.StringData.Length > sizeof(pAdapter->ActivePath)) {
        DBGPRINT(DBG_LOAD | DBG_ERROR, 
            (L"SDNdis: Active path too long!\n"));
        NdisCloseConfiguration(configHandle);
        return NDIS_STATUS_FAILURE;
    }

        // copy the counted string over
    memcpy(pAdapter->ActivePath, 
           pConfigParm->ParameterData.StringData.Buffer, 
           pConfigParm->ParameterData.StringData.Length);
  
  if ( pConfigParm->ParameterData.StringData.Length == 0 )
    {
        DBGPRINT(DBG_LOAD | DBG_WARNING, 
            (L"SDNdis: Active path str length is 0, perhaps no card!\n"));
        NdisCloseConfiguration(configHandle);
        return NDIS_STATUS_FAILURE;
    }else{
        DBGPRINT(DBG_LOAD | DBG_WARNING, 
            (L"SDNdis: Active path str == %s\n",pConfigParm->ParameterData.StringData.Buffer));
    }
    
                          


    NdisCloseConfiguration(configHandle);

    DBGPRINT(DBG_LOAD | DBG_LOAD, 
            (L"SDNdis: Active Path Retrieved: %s \n",pAdapter->ActivePath));

        // now get the device handle
    pAdapter->hDevice = SDGetDeviceHandle((DWORD)pAdapter->ActivePath, NULL);
    return NDIS_STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
// SDIOInitialization - SDIO specific initialization
// Inputs:   pAdapter - pointer to freshly allocated adapter
// Outputs:
// Returns: NDIS_STATUS
// Notes: 
/////////////////////////////////////////////////////////////////////////////
NDIS_STATUS SDIOInitialization(PMRVDRV_ADAPTER pAdapter)
{
    NDIS_STATUS                     NdisStatus;     // intermediate status
    SDCARD_CLIENT_REGISTRATION_INFO clientInfo;     // client registration
    SD_API_STATUS                   sdStatus;       // SD Status
    SD_IO_FUNCTION_ENABLE_INFO      functionEnable; // enable sd card function
    SD_CARD_INTERFACE           cardInterface;      // card interface information
    SDIO_CARD_INFO              sdioInfo;           // sdio info
    DWORD                       blockLength;        // block length
    UCHAR                   regValue;           // reg value
    
    
    NdisStatus = NDIS_STATUS_FAILURE;

    if (pAdapter->hDevice != NULL) 
    {
        memset(&clientInfo, 0, sizeof(clientInfo));

        // set client options and register as a client device
        _tcscpy(clientInfo.ClientName, TEXT("MRVL WiFi"));

        // set the event callback
        clientInfo.pSlotEventCallBack = SDNdisSlotEventCallBack;

        sdStatus = SDRegisterClient(pAdapter->hDevice, pAdapter, &clientInfo);
    
        if (!SD_API_SUCCESS(sdStatus))
        {
            DBGPRINT(DBG_ERROR, 
                    (L"SDNDIS: Failed to register client : 0x%08X \n", sdStatus));
            return NdisStatus;
        }

        // set up the function enable struct
        // TODO use the appropriate retry and interval count for the function
        functionEnable.Interval = 500;
        functionEnable.ReadyRetryCount = 3;
    
        DBGPRINT(DBG_LOAD|DBG_HELP, (L"SDNDIS : Enabling Card ... \n"));

        // turn on our function
        sdStatus = SDSetCardFeature(pAdapter->hDevice,
                                    SD_IO_FUNCTION_DISABLE,
                                    &functionEnable,
                                    sizeof(functionEnable));   

        //MessageBox(NULL, TEXT("Enable IO FUN"), (TEXT(IFSTRN) TEXT(CHIPSTRN) TEXT("_") TEXT(RFSTRN)), MB_OK); 
                      
        //031407 Need to do delay here. Don't try to save the time....
        DBGPRINT(DBG_LOAD|DBG_ERROR, (L"%S() - Sleeping for %d ms\n", __FUNCTION__, 100));
        NdisMSleep(100000);

        functionEnable.Interval = 500;
        functionEnable.ReadyRetryCount = 3;

        // turn on our function
        sdStatus = SDSetCardFeature(pAdapter->hDevice,
                                    SD_IO_FUNCTION_ENABLE,
                                    &functionEnable,
                                    sizeof(functionEnable));


        if (!SD_API_SUCCESS(sdStatus))
        {
             DBGPRINT(DBG_ERROR, (L"SDNDIS: Failed to enable Function:0x%08X\r\n",sdStatus));
             return NdisStatus;
        }

        // query the card interface
        sdStatus = SDCardInfoQuery(pAdapter->hDevice,
                                 SD_INFO_CARD_INTERFACE,
                                 &cardInterface,
                                 sizeof(cardInterface));

        if (!SD_API_SUCCESS(sdStatus))
        {
            DBGPRINT(DBG_ERROR, (L"SDIO Samp: Failed to query interface ! 0x%08X  \r\n",sdStatus));
            return sdStatus;
        }


        if (cardInterface.ClockRate == 0)
        {
            DBGPRINT(DBG_ERROR, (L"SDIO Samp: Device interface rate is zero! \n"));
            return SD_API_STATUS_UNSUCCESSFUL;
        }

        DBGPRINT(DBG_LOAD, (L"1 SDIO Samp: Interface Clock : %d Hz \n",
            cardInterface.ClockRate));
    
        if (cardInterface.InterfaceMode == SD_INTERFACE_SD_MMC_1BIT) 
        {
            DBGPRINT(DBG_LOAD, (L"SDIO Samp: 1 Bit interface mode \n"));
            DBGPRINT(DBG_ALLEN, (L"1SDIO Samp: 1 Bit interface mode \n"));
        } 
        else if (cardInterface.InterfaceMode == SD_INTERFACE_SD_4BIT)
        {
            DBGPRINT(DBG_LOAD, (L"SDIO Samp: 4 bit interface mode \n"));
      DBGPRINT(DBG_ALLEN, (L"1SDIO Samp: 4 Bit interface mode \n"));
        } else 
        {
            DBGPRINT(DBG_ERROR, (L"SDIO Samp: Unknown interface mode! %d \n",
                cardInterface.InterfaceMode));
            return SD_API_STATUS_UNSUCCESSFUL;
        }

       if(pAdapter->SetSD4BIT==1)
       {
            cardInterface.InterfaceMode = SD_INTERFACE_SD_4BIT;
          //DBGPRINT(DBG_ALLEN, ("2SDIO Samp: 4 Bit interface mode \n"));
       }else
       {
              cardInterface.InterfaceMode = SD_INTERFACE_SD_MMC_1BIT;
           DBGPRINT(DBG_ALLEN, (L"2SDIO Samp: 1 Bit interface mode \n"));
       }
       
       // set clock rate
       // This is been commented by soujanya on 2/17/2004 in order to improve 
       // the through put.
     cardInterface.ClockRate = 25000000;//25MHz
     sdStatus = SDSetCardFeature(pAdapter->hDevice,
                  SD_SET_CARD_INTERFACE,
                  &cardInterface,
                  sizeof(cardInterface));

    if (!SD_API_SUCCESS(sdStatus))
    {
            //DBGPRINT(DBG_ERROR, ("SDIO Samp: SetClock rate failed! 0x%08X  \n",sdStatus));
      if(cardInterface.InterfaceMode == SD_INTERFACE_SD_MMC_1BIT)
        {
                     DBGPRINT(DBG_ERROR, (L"SDIO Samp: Set Clock rate and set 1bit mode failed! 0x%08X  \n",sdStatus));
        }else if(cardInterface.InterfaceMode == SD_INTERFACE_SD_4BIT)
          {
                       DBGPRINT(DBG_ERROR, (L"SDIO Samp:Set Clock rate and set 4bit mode failed! 0x%08X  \n",sdStatus));
          }
            return sdStatus;
        } 
    
    sdStatus = SDCardInfoQuery(pAdapter->hDevice,
                                 SD_INFO_CARD_INTERFACE,
                                 &cardInterface,
                                 sizeof(cardInterface));

        if (!SD_API_SUCCESS(sdStatus))
    {
            DBGPRINT(DBG_ERROR, (L"SDIO Samp: Failed to query interface ! 0x%08X  \n",sdStatus));
            return sdStatus;
        }

        if (cardInterface.ClockRate == 0)
    {
            DBGPRINT(DBG_ERROR, (L"SDIO Samp: Device interface rate is zero! \n"));
            return SD_API_STATUS_UNSUCCESSFUL;
        }

        DBGPRINT(DBG_LOAD|DBG_WARNING, (L"2 SDIO Samp: Interface Clock : %d Hz \n",
            cardInterface.ClockRate));

     DBGPRINT(DBG_ALLEN, (L"SDIO Samp: Interface Clock : %d Hz \n",
            cardInterface.ClockRate));
    
        if (cardInterface.InterfaceMode == SD_INTERFACE_SD_MMC_1BIT) 
        {
            DBGPRINT(DBG_LOAD|DBG_WARNING, (L"SDIO Samp: 1 Bit interface mode \n"));
        } 
        else if (cardInterface.InterfaceMode == SD_INTERFACE_SD_4BIT) 
        {
            DBGPRINT(DBG_LOAD|DBG_WARNING, (L"SDIO Samp: 4 bit interface mode \n"));
        } 
        else 
        {
            DBGPRINT(DBG_ERROR, (L"SDIO Samp: Unknown interface mode! %d \n",
                cardInterface.InterfaceMode));
            return SD_API_STATUS_UNSUCCESSFUL;
        }


           // query the SDIO information
        sdStatus = SDCardInfoQuery(pAdapter->hDevice,
                                 SD_INFO_SDIO,
                                 &sdioInfo,
                                 sizeof(sdioInfo));
    
    
        if (!SD_API_SUCCESS(sdStatus)) 
    {
            DBGPRINT(DBG_ERROR, 
                (L"SDIO Samp: Failed to query SDIO info ! 0x%08X  \n",sdStatus));
            return sdStatus;
        }

           // this card only has one function 
        if (sdioInfo.FunctionNumber != 1) 
    {
            DBGPRINT(DBG_ERROR, 
                (L"SDIO Samp: Function number %d is incorrect! \n",
                sdioInfo.FunctionNumber));
            return SD_API_STATUS_UNSUCCESSFUL;
        }

            // save off function number
        //pAdapter->Function = sdioInfo.FunctionNumber;
    
        DBGPRINT(DBG_LOAD|DBG_HELP, 
            (L"SDIO Samp: Function: %d \n", sdioInfo.FunctionNumber)); 
        DBGPRINT(DBG_LOAD|DBG_HELP, 
            (L"SDIO Samp: Device Code: %d \n", sdioInfo.DeviceCode)); 
        DBGPRINT(DBG_LOAD|DBG_HELP, 
            (L"SDIO Samp: CISPointer: 0x%08X \n", sdioInfo.CISPointer)); 
        DBGPRINT(DBG_LOAD|DBG_HELP, 
            (L"SDIO Samp: CSAPointer: 0x%08X \n", sdioInfo.CSAPointer)); 
        DBGPRINT(DBG_LOAD|DBG_HELP, 
            (L"SDIO Samp: CardCaps: 0x%02X \n", sdioInfo.CardCapability)); 

            // none of these should be zero
        if ((sdioInfo.DeviceCode == 0) ||
            (sdioInfo.CISPointer == 0) ||
            (sdioInfo.CardCapability == 0)) 
           {
               DBGPRINT(DBG_ERROR, (L"SDIO Samp: SDIO information is incorrect \n"));
               return SD_API_STATUS_UNSUCCESSFUL;
           }


        blockLength = SDIO_EXTENDED_IO_BLOCK_SIZE;

        // set the block length for this function
        sdStatus = SDSetCardFeature(pAdapter->hDevice,
                                  SD_IO_FUNCTION_SET_BLOCK_SIZE,
                                  &blockLength,
                                  sizeof(blockLength));

        if (!SD_API_SUCCESS(sdStatus)) 
    {
             DBGPRINT(DBG_ERROR, 
                 (L"SDIO Samp: Failed to set Block Length ! 0x%08X  \n",sdStatus));
             return sdStatus;
        }

        DBGPRINT(DBG_LOAD|DBG_HELP, 
            (L"SDIO Samp: Block Size set to %d bytes \n", blockLength));

    {
        SD_SET_FEATURE_TYPE     nSdFeature;

        DBGPRINT( DBG_LOAD|DBG_HELP,( L"[MRVL] - SdioFastPath = %d\n", pAdapter->SdioFastPath ));
        
        nSdFeature = ( pAdapter->SdioFastPath == 1 ? SD_FAST_PATH_ENABLE : SD_FAST_PATH_DISABLE );
        
        sdStatus = SDSetCardFeature( pAdapter->hDevice,
                                nSdFeature,
                                NULL,
                                0 );
        if (!SD_API_SUCCESS(sdStatus)) 
        {
            DBGPRINT( DBG_LOAD|DBG_HELP, ( L"MRVL -error when %s SDIO FAST PATH\n", (nSdFeature == SD_FAST_PATH_ENABLE ? L"enabling" : L"disabling") ) );
            return sdStatus;
        }
        else
        {
            DBGPRINT( DBG_LOAD|DBG_HELP, ( L"MRVL - SDIO FAST PATH is %s\n", (nSdFeature == SD_FAST_PATH_ENABLE ? L"enabled" : L"disabled")) );
        }
    }


       sdStatus = If_ReadRegister(pAdapter,
                            //SD_IO_READ ,
                            0,
                            HCR_SDIO_BUS_INTERFACE_CONTROL , 
                            FALSE,
                            &regValue,
                            sizeof(regValue));

         if (!SD_API_SUCCESS(sdStatus))
       {
                DBGPRINT(DBG_ERROR, (L"SDIO Samp: Failed to read Bus Interface Control 0x07: 0x%08X  \n",sdStatus));
                 return sdStatus;
            }
          else
            {

              DBGPRINT(DBG_ALLEN, (L"4 SDIO read Bus Interface Control 0x07=0x%X \n",regValue));
            }

        if(cardInterface.InterfaceMode == SD_INTERFACE_SD_4BIT)
          {
                //regValue = 0x2;    // 0xA0 for 1 bit mode and 0x02 for 4 bit mode
                //regValue = 0xA2; 
                regValue=regValue|0xA2;
        DBGPRINT(DBG_ALLEN, (L"5 SDIO 4bit A2 write Bus Interface Control 0x07=0x%X \n",regValue));
            }
          else
            {   //cardInterface.InterfaceMode == SD_INTERFACE_SD_MMC_1BIT
                //regValue = 0xA0;
         regValue=regValue|0xA0;
        DBGPRINT(DBG_ALLEN, (L"5 SDIO 1bit A0 write Bus Interface Control 0x07=0x%X \n",regValue));
            }

       sdStatus = If_WriteRegister(pAdapter,
                                            //SD_IO_WRITE,          
                                            0,     
                                            HCR_SDIO_BUS_INTERFACE_CONTROL ,   // address of Bus interface control register
                                            FALSE,
                                            &regValue,   // reg
                                            1);   
    
           if (!SD_API_SUCCESS(sdStatus))
       {
                DBGPRINT(DBG_ERROR, 
                 (L"SDIO Samp: Failed to enable Bus Interface Control 0x07: 0x%08X  \n",sdStatus));
                 return sdStatus;
            }

    // set up for write to clear
    // Set to read to clear mode for interrupt
        regValue = 0;
        
        sdStatus = If_WriteRegister(pAdapter,
                                            //SD_IO_WRITE,          
                                            1,     
                                            HCR_HOST_INT_STATUS_RSR_REGISTER ,   // Int RSR address
                                            FALSE,
                                            &regValue,   // reg
                                            1);   
    
        if (!SD_API_SUCCESS(sdStatus))
      {
                DBGPRINT(DBG_ERROR, (L"SDIO Samp: Failed to enable Interrupt write To Clear: 0x%08X  \n",sdStatus));
                return sdStatus;
            }

        // connect the interrupt callback
        sdStatus = SDIOConnectInterrupt(pAdapter->hDevice, (PSD_INTERRUPT_CALLBACK)SDNdisInterruptCallback);    
    

        if (!SD_API_SUCCESS(sdStatus))
       {
             DBGPRINT(DBG_ERROR, (L"SDNDIS: Failed to connect interrupt: 0x%08X  \n",sdStatus));
             return NdisStatus;
           }
        DBGPRINT(DBG_LOAD|DBG_HELP, (L"SDNDIS : Card ready \n"));
    }else //if (pAdapter->hDevice != NULL) 
    {
        DEBUG_ASSERT(FALSE);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS sdio_Initialization( PMRVDRV_ADAPTER Adapter, NDIS_HANDLE WrapperConfigurationContext )
{
    NDIS_STATUS     Status;
    
    Adapter->ConfigurationHandle = WrapperConfigurationContext;
    Adapter->ShutDown = FALSE;
    Status = SDNdisGetSDDeviceHandle(Adapter);
// tt wled  if( Status != NDIS_STATUS_SUCCESS )
        /*
            Need to check hDevice for avoiding a data abort if driver could not get a valid
            device handle.
        */
        if( Status != NDIS_STATUS_SUCCESS || Adapter->hDevice == NULL ) // tt wled
    {
        // Only adapter object itself has been created up to
        // this point
        //MRVDRV_FREE_MEM((PVOID)Adapter, sizeof(MRVDRV_ADAPTER));
        DBGPRINT(DBG_LOAD | DBG_ERROR, (L"*** SDNdisGetSDDeviveHandleFailed %d\n", Status));

        return NDIS_STATUS_FAILURE;
    }
    if ( SDIOInitialization(Adapter) != NDIS_STATUS_SUCCESS )
    {
        // Only adapter object itself has been created up to
        // this point
        //MRVDRV_FREE_MEM((PVOID)Adapter, sizeof(MRVDRV_ADAPTER));
        DBGPRINT(DBG_LOAD | DBG_ERROR, (L"*** SDIOInitialization FAILED! ***\n"));
        return NDIS_STATUS_FAILURE;
    }
    return Status;
}


IF_API_STATUS sdio_GetLengthOfDataBlock( PMRVDRV_ADAPTER Adapter, USHORT *pLength )
{
    USHORT   usThisLen;
    SD_API_STATUS   status;

    usThisLen = 0;
        status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                               SD_IO_READ ,
                                               SCRATCHREGFUNCNUM,
                                               LENGTH_SCRATCH_REGISTER, 
                                               FALSE,
                                               (UCHAR *)&usThisLen,
                                               sizeof(usThisLen));
    if (!SD_API_SUCCESS(status))
    {
        return IF_FAIL;
    }
    *pLength = usThisLen;

    return IF_SUCCESS;
}

IF_API_STATUS sdio_GetCardStatusAndMacEvent(  PMRVDRV_ADAPTER Adapter, UCHAR *pCardStatus, UCHAR *pMacEvent )
{
    const int    nMaxReadCount = 5;
    int      nReadCount;
    SD_API_STATUS   status;
    UCHAR   ucCardStatus, macEvent;

    for( nReadCount=0; nReadCount<nMaxReadCount; nReadCount++ )
    {
        status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                               SD_IO_READ ,
                                               SCRATCHREGFUNCNUM, // function x for scratch register
                                               PACKET_TYPE_MAC_EVENT_SCRATCH_REGISTER, 
                                               FALSE,
                                               &ucCardStatus,
                                               sizeof(ucCardStatus));
        if (!SD_API_SUCCESS(status))
        {
            DBGPRINT(DBG_ISR,(L"Unable to read PACKET_TYPE_MAC_EVENT_SCRATCH_REGISTER\n"));
            return IF_FAIL;
        }

        // bits 2-0 are the interrupt cause, bits 3-7 are the MAC_EVENT if the 
        // cause is MAC_EVENT
        macEvent = ucCardStatus >> 3;           // take the top 5 bits
        ucCardStatus = ucCardStatus & 0x07;     // take the lower 3 bits
        if( ucCardStatus < IF_INVALID_PKT_TYPE )
        {
            *pCardStatus = ucCardStatus;
            *pMacEvent = macEvent;
            return IF_SUCCESS;
        }
        else
            DBGPRINT(DBG_ERROR,(L"get invalid pkt type: , read again\r\n", ucCardStatus));
    }

    return IF_FAIL;
}

IF_API_STATUS sdio_GetDataBlock( PMRVDRV_ADAPTER Adapter, USHORT usLength, UCHAR ucCardStatus, UCHAR *p_pkt )
///IF_API_STATUS sdio_GetDataBlock( PMRVDRV_ADAPTER Adapter, USHORT usLength, IF_PKT_TYPE ucCardStatus, UCHAR *p_pkt )
{
    ULONG                   numBlocks;
    DWORD                   argument;
    SD_TRANSFER_CLASS       transferClass;
    SD_API_STATUS           status;
    SD_COMMAND_RESPONSE response;
    
         // calculate the number of blocks
         numBlocks = usLength / SDIO_EXTENDED_IO_BLOCK_SIZE;
      
         if ( (usLength % SDIO_EXTENDED_IO_BLOCK_SIZE) != 0 )
         {
           numBlocks++;
         }                      
           
/* dralee 082707 remove dummy code         
    //tt IMPORTANT: WinCE 4.2 needs this block        
#ifndef MRVL_WINCE50
         if((numBlocks%2) != 0)
         {
           numBlocks++;
         } 
#endif
*/
                        
         // read, block mode, address at 0, incrementing address
         // first read 1 block
         argument =  BUILD_IO_RW_EXTENDED_ARG(
                                              SD_IO_OP_READ, 
                                              SD_IO_BLOCK_MODE, 
                                              1,   // functon number
                                              SDIO_IO_PORT, 
                                              SD_IO_FIXED_ADDRESS, 
                                              numBlocks);
         transferClass = SD_READ;
      
         if ( ucCardStatus == IF_MAC_EVENT )
         {
             status = SDSynchronousBusRequest(
                                              Adapter->hDevice, 
                                              SD_CMD_IO_RW_EXTENDED,
                                              argument,
                                              transferClass, 
                                              ResponseR5,
                                              &response, 
                                              numBlocks,
                                              SDIO_EXTENDED_IO_BLOCK_SIZE, 
                                              (PUCHAR)p_pkt,
                                              0); 

             if (!SD_API_SUCCESS(status))
             {                       
                 DBGPRINT(DBG_ERROR,(L"[Marvell]MacEventread error\r\n") );
                 return IF_FAIL;
             }       
           
         }
         else
         {                                 
           status = SDSynchronousBusRequest(
                                           Adapter->hDevice, 
                                           SD_CMD_IO_RW_EXTENDED,
                                           argument,
                                           transferClass, 
                                           ResponseR5,
                                           &response, 
                                           numBlocks,
                                           SDIO_EXTENDED_IO_BLOCK_SIZE, 
                                           (PUCHAR)p_pkt,
                                           0); 
           // check if it's packet or command response
           if (!SD_API_SUCCESS(status))
           {
              DBGPRINT(DBG_ERROR,(L"MrvDrvSdioIstThread: Unable to read packet! 0x%x, %d, %d\r\n",status,numBlocks,usLength));
              return IF_FAIL;
           }
         } 

    return IF_SUCCESS;
}

IF_API_STATUS sdio_ReadRegistersDirect( PMRVDRV_ADAPTER Adapter, UCHAR nFunc, DWORD dwAddr, UINT bReadAfterWrite, UCHAR *pBuf, ULONG nBufLen )
{
    SD_API_STATUS status;

    status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                               SD_IO_READ ,
                                               nFunc, // function 0 for scratch register
                                               dwAddr, 
                                               bReadAfterWrite,
                                               pBuf,
                                               nBufLen);

    if ( !SD_API_SUCCESS( status ) )
        return IF_FAIL;

    return IF_SUCCESS;
}

IF_API_STATUS sdio_WriteRegistersDirect( PMRVDRV_ADAPTER Adapter, UCHAR nFunc, DWORD dwAddr, UINT bReadAfterWrite, UCHAR *pBuf, ULONG nBufLen )
{
    SD_API_STATUS status;

    status = SDReadWriteRegistersDirect(Adapter->hDevice,
                                               SD_IO_WRITE ,
                                               nFunc, // function 0 for scratch register
                                               dwAddr, 
                                               bReadAfterWrite,
                                               pBuf,
                                               nBufLen);

    if ( !SD_API_SUCCESS( status ) )
        return IF_FAIL;

    return IF_SUCCESS;
} 

IF_API_STATUS sdio_ReInitCard(PMRVDRV_ADAPTER Adapter)
{
   NDIS_STATUS sdStatus;


   sdStatus = SDSetCardFeature(Adapter->hDevice,
                               SD_CARD_FORCE_RESET,
                               NULL,
                               0);

   if(!SD_API_SUCCESS(sdStatus))   
   {  
      DBGPRINT(DBG_ERROR, (L"SD: Fail to force card reset\r\n",sdStatus));
      return IF_FAIL;
   }

   sdStatus = SDSetCardFeature(Adapter->hDevice,
                               SD_CARD_DESELECT_REQUEST,
                               NULL,
                               0);

   if(!SD_API_SUCCESS(sdStatus))   
   {  
      DBGPRINT(DBG_ERROR, (L"SD: Fail to deselect card\r\n",sdStatus));
      return IF_FAIL;
   }

   sdStatus = SDSetCardFeature(Adapter->hDevice,
                               SD_CARD_SELECT_REQUEST,
                               NULL,
                               0);

   if(!SD_API_SUCCESS(sdStatus))   
   {  
      DBGPRINT(DBG_ERROR, (L"SD: Fail to select card\r\n",sdStatus)); 
      return IF_FAIL;
   }

   
   return IF_SUCCESS;
}
      

//071107
IF_API_STATUS sdio_PowerUpDevice(PMRVDRV_ADAPTER Adapter)
{
   UCHAR   regValue;
   IF_API_STATUS   sdStatus; 

   DBGPRINT(DBG_DEEPSLEEP|DBG_WARNING, (L"SetUp PowerUP bit\r\n"));


   regValue = 0x2;  //Configuration: Host Offset: 0x03 Write Bit1=1
   sdStatus = If_WriteRegister(Adapter,
                               //SD_IO_WRITE,          
                               1,     
                               HCR_HOST_CONFIGURATION_REGISTER,  //0x03 
                               FALSE,
                               &regValue,   // reg
                               1);   
    
   if (!SD_API_SUCCESS(sdStatus))
   {
        DBGPRINT(DBG_ERROR, (L"Set PowerUp bit Fail: %x  \r\n",sdStatus));
        return sdStatus;
   }  
    
   return  sdStatus;
}

//071107
IF_API_STATUS sdio_ClearPowerUpBit(PMRVDRV_ADAPTER Adapter)
{
   UCHAR   regValue;
   IF_API_STATUS   sdStatus=IF_SUCCESS; 

  
   {                              

   regValue = 0x0;  //Configuration: Host Offset: 0x03
   sdStatus = If_WriteRegister(Adapter,
                               //SD_IO_WRITE,          
                               1,     
                               HCR_HOST_CONFIGURATION_REGISTER,  //0x03 
                               FALSE,
                               &regValue,   // reg
                               1);   
    
   if (!SD_API_SUCCESS(sdStatus))
   {
        DBGPRINT(DBG_ERROR, (L"Clear PowerUp bit Fail: %x  \r\n",sdStatus));
        return sdStatus;
   }  

      DBGPRINT(DBG_DEEPSLEEP|DBG_WARNING, (L"Clear PowerUP bit\r\n"));
   }
   return  sdStatus;
}






IF_API_STATUS SDIOGetPktTypeAndLength(PMRVDRV_ADAPTER pAdapter, 
                                      PUCHAR type, 
                                      PUCHAR mEvent, 
                                      PUSHORT usLength,
                                      PPVOID p_pkt)
{
    IF_API_STATUS   ifStatus;
    UCHAR macEvent;
    USHORT pktLength;
    MRVDRV_GET_PACKET_STATUS    qStatus; 
    PNDIS_PACKET                pPacket;  
    UINT                        bufferLength;
    PNDIS_BUFFER                pBuffer;
    HostCmd_DS_CODE_DNLD       *pkt=NULL;
    PVOID                       pRxBufVM=NULL;

	///#ifdef MAC_EVENT_INFO_FROM_PACKET 
    ifStatus = If_GetLengthOfDataBlock( pAdapter, &pktLength );
    //041807 
    *usLength = pktLength;
     
    if ( !IF_IS_SUCCESS(ifStatus) )
    {
        DBGPRINT(DBG_ERROR, (L"Read SDIO data block length is Fail: %x  \r\n", ifStatus));
        return ifStatus;
    }
    
	//101807 Drop packet with abnormal length ++
    if( pktLength >= (MRVDRV_ETH_RX_PACKET_BUFFER_SIZE)*2 )
    {   
       UCHAR temp[SDIO_EXTENDED_IO_BLOCK_SIZE*3];    
          
       RETAILMSG(1, (L"Read abnormal SDIO data block length:%d, Drop this Packet\r\n", pktLength));

       If_GetDataBlock( pAdapter, SDIO_EXTENDED_IO_BLOCK_SIZE*2, IF_INVALID_PKT_TYPE, (UCHAR*)temp);    
       return IF_FAIL;
    }
	//101807 Drop packet with abnormal length --

    qStatus=GetRxPacketDesc(pAdapter, &pPacket);
    if (qStatus == GPS_FAILED)
    {
        DBGPRINT(DBG_ERROR, (L"\r\n *** ERROR: GetRxPacketDesc() fail...\r\n")); 
        return IF_FAIL;
    }
    
    pAdapter->pRxCurPkt = pPacket; 
    NdisQueryPacket(pPacket, NULL, NULL, &pBuffer, NULL);
    NdisQueryBufferSafe(pBuffer, &pRxBufVM, &bufferLength, HighPagePriority);

    //dralee 082707
    NdisAdjustBufferLength(pBuffer, (pktLength-MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE));  
    *p_pkt=pkt=(PHostCmd_DS_CODE_DNLD)((ULONG)pRxBufVM - MRVDRV_ETH_RX_HIDDEN_HEADER_SIZE);

    pAdapter->pRxPD1 = (PRxPD)pkt->Code;

	///Note: 3rd parameter is not used for SDIO interface since v12
    ifStatus = If_GetDataBlock( pAdapter, pktLength, IF_INVALID_PKT_TYPE, (UCHAR*)pkt);    

    if ( !IF_IS_SUCCESS(ifStatus) )
    {
        DBGPRINT(DBG_ERROR, (L"Read SDIO data block Fail: %x  \r\n", ifStatus));
        return ifStatus;
    }


    if(pkt->Type == IF_MAC_EVENT)
    {
       macEvent = (UCHAR)(pkt->Code[0]);
       DBGPRINT(DBG_ERROR, (L"MAC Event is %x \r\n", macEvent));
    }
    else
       macEvent = 0;

    *type = (UCHAR)pkt->Type;
    *mEvent = (UCHAR)macEvent;

	///#endif	///MAC_EVENT_INFO_FROM_PACKET

     *usLength = *usLength - sizeof(pkt->Type) - sizeof(pkt->Len);

     return IF_SUCCESS;	
}

