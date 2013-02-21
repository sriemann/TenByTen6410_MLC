//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    fmd.cpp

Abstract:

    This module implements main functions of FMD common PDD

Functions:

    FMD_Init,  

Notes:

--*/

#include "precomp.h"

volatile S3C6410_NAND_REG *g_pNFConReg = NULL;
volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;

// This FMD library can be included into StorageDeviceDriver and EBOOT
// There are different debugzones
extern DBGPARAM dpCurSettings;

BOOL g_bNeedExtAddr;
NANDDeviceInfo stDeviceInfo;
NANDDeviceInfo GetNandInfo(void) { return stDeviceInfo; }

/*
    @func   DWORD | ReadFlashID | Reads the flash manufacturer and device codes.
    @rdesc  Manufacturer and device codes.
    @comm
    @xref
*/
static DWORD ReadFlashID(void)
{
    BYTE Mfg, Dev;
    int i;

    NF_nFCE_L();                // Deselect the flash chip.
    NF_CMD(CMD_READID);        // Send flash ID read command.

    NF_ADDR(0);

    for (i=0; i<NAND_READ_TIMEOUT; i++)
    {
        Mfg    = NF_RDDATA_BYTE();
        if (Mfg == NAND_MID_SAMSUNG || Mfg == NAND_MID_TOSHIBA) break;
    }

    Dev    = NF_RDDATA_BYTE();

    NF_nFCE_H();

    return ((DWORD)MAKEWORD(Dev, Mfg));
}

/*
    @func   PVOID | FMD_Init | Initializes the Smart Media NAND flash controller.
    @rdesc  Pointer to S3C6410 NAND controller registers.
    @comm
    @xref
*/
PVOID FMD_Init(LPCTSTR lpActiveReg, PPCI_REG_INFO pRegIn, PPCI_REG_INFO pRegOut)
{
    volatile DWORD nNandID;
    UINT8 nMID, nDID;
    UINT32 nCnt;
    BOOL bNandExt = FALSE;
    UINT32 nTotPages;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] ++FMD_Init()\r\n")));
    PHYSICAL_ADDRESS    ioPhysicalBase ={0,0};

    if (pRegIn && pRegIn->MemBase.Num && pRegIn->MemBase.Reg[0])
    {
        g_pNFConReg = (S3C6410_NAND_REG *)(pRegIn->MemBase.Reg[0]);
    }
    else
    {
#ifndef BUILDING_BOOTLOADER // Not building boot loader
        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_NFCON;
        g_pNFConReg = (S3C6410_NAND_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_NAND_REG), FALSE);
        if (g_pNFConReg == NULL)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("Error: Io Mapping failed for NFCON Register\n")));
        }
#endif
    }

    if (pRegIn && pRegIn->MemBase.Num==2 && pRegIn->MemBase.Reg[1])
    {
        g_pSysConReg = (S3C6410_SYSCON_REG *)(pRegIn->MemBase.Reg[1]);
    }
    else
    {
#ifndef BUILDING_BOOTLOADER // Not building boot loader
        ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
        g_pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
        if (g_pSysConReg == NULL)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("Error: Io Mapping failed for SYSCON Register\n")));
        }
#endif
    }
    
    // Configure BUS Width and Chip Select for NAND Flash
    CLEARBIT32(g_pSysConReg->MEM_SYS_CFG, BIT_ROMBUS_WIDTH);   // NAND Flash BUS Width -> 8 bit 
    // Select NFCON CS0 as Xm0CSn[2]
    g_pSysConReg->MEM_SYS_CFG &= ~(0x1<<1);

    Init_NandController();
    
    nNandID = ReadFlashID();

    nMID = HIBYTE(nNandID);
    nDID = LOBYTE(nNandID);

    RETAILMSG(FMD_ZONE_STATUS, (TEXT("[FMD:INF] FMD_Init() : Read ID = 0x%08x, MID:0x%02x, DID:0x%02x\n\r"), nNandID, nMID, nDID));

    // Search Nand ID Table
    for (nCnt = 0; astNandSpec[nCnt].nMID != 0; nCnt++)
    {
        if (nDID == astNandSpec[nCnt].nDID)
        {
            bNandExt = TRUE;
            break;
        }
    }

    if (!bNandExt)
    {
        RETAILMSG(FMD_ZONE_STATUS, (TEXT("[FMD:ERR] FMD_Init() : Unknown ID = 0x%08x\n\r"), nNandID));
        return NULL;
    }

    NUM_OF_BLOCKS = astNandSpec[nCnt].nNumOfBlks;
    PAGES_PER_BLOCK = astNandSpec[nCnt].nPgsPerBlk;
    SECTORS_PER_PAGE = astNandSpec[nCnt].nSctsPerPg;

    nTotPages = NUM_OF_BLOCKS * PAGES_PER_BLOCK;
    if(((nTotPages-1)>>16) != 0)
    {
        g_bNeedExtAddr = TRUE;
    }
    else
    {
        g_bNeedExtAddr = FALSE;
    }

    RETAILMSG(FMD_ZONE_STATUS, (TEXT("[FMD] FMD_Init() : NUM_OF_BLOCKS = %d\n\r"), NUM_OF_BLOCKS));
    RETAILMSG(FMD_ZONE_STATUS, (TEXT("[FMD] FMD_Init() : PAGES_PER_BLOCK = %d\n\r"), PAGES_PER_BLOCK));
    RETAILMSG(FMD_ZONE_STATUS, (TEXT("[FMD] FMD_Init() : SECTORS_PER_PAGE = %d\n\r"), SECTORS_PER_PAGE));
    RETAILMSG(FMD_ZONE_STATUS, (TEXT("[FMD] FMD_Init() : Addr Cycle = %d\n\r"), g_bNeedExtAddr ? 5 : 4));    

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] --FMD_Init()\r\n")));

    return((PVOID)g_pNFConReg);
}


/*
    @func   BOOL | FMD_ReadSector | Reads the specified sector(s) from NAND flash.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL FMD_ReadSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    BOOL bRet;

    RETAILMSG(FMD_ZONE_READ, (TEXT("[FMD] ++FMD_ReadSector(0x%08x) \r\n"), startSectorAddr));

    if ( IS_LB )
    {
        bRet = FMD_LB_ReadSector(startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors);
    }
    else
    {
        bRet = FMD_SB_ReadSector(startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors);
    }

    RETAILMSG(FMD_ZONE_READ, (TEXT("[FMD] --FMD_ReadSector()\r\n")));

    return bRet;
}


/*
    @func   BOOL | FMD_EraseBlock | Erases the specified flash block.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL FMD_EraseBlock(BLOCK_ID blockID)
{
    BOOL    bRet = TRUE;

    RETAILMSG(FMD_ZONE_ERASE, (TEXT("[FMD] ++FMD_EraseBlock(0x%08x) \r\n"), blockID));

    if ( IS_LB )
    {
        bRet = FMD_LB_EraseBlock(blockID);
    }
    else
    {
        bRet = FMD_SB_EraseBlock(blockID);
    }

    RETAILMSG(FMD_ZONE_ERASE, (TEXT("[FMD] --FMD_EraseBlock()\r\n")));

    return bRet;
}


/*
    @func   BOOL | FMD_WriteSector | Writes the specified data to the specified NAND flash sector/page.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL FMD_WriteSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    BOOL    bRet = TRUE;

    RETAILMSG(FMD_ZONE_WRITE, (TEXT("[FMD] ++FMD_WriteSector(0x%08x) \r\n"), startSectorAddr));

    if ( IS_LB )
    {
        bRet = FMD_LB_WriteSector(startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors);
    }
    else
    {
        bRet = FMD_SB_WriteSector(startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors);
    }

    RETAILMSG(FMD_ZONE_WRITE, (TEXT("[FMD] --FMD_WriteSector()\r\n")));

    return bRet;
}


VOID FMD_PowerUp(VOID)
{
    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] FMD_PowerUp() \r\n")));
    // Set up initial flash controller configuration.
    Init_NandController();
}


VOID FMD_PowerDown(VOID)
{
    DEBUGMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] FMD_PowerDown() \r\n")));
    // Using PMIC Power off is preferable for saving the power of NANDFlash attached to PMIC
    // See the board's Electrical Circuit
}


BOOL FMD_OEMIoControl(DWORD dwIoControlCode, PBYTE pInBuf, DWORD nInBufSize, PBYTE pOutBuf, DWORD nOutBufSize, PDWORD pBytesReturned)
{
    switch(dwIoControlCode)
    {
        case IOCTL_FMD_GET_INTERFACE:
        {
            RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] FMD_OEMIoControl() : IOCTL_FMD_GET_INTERFACE\r\n")));

            if (!pOutBuf || nOutBufSize < sizeof(FMDInterface))
            {
                DEBUGMSG(FMD_ZONE_ERROR, (TEXT("FMD_OEMIoControl: IOCTL_FMD_GET_INTERFACE bad parameter(s).\r\n")));
                return(FALSE);
            }

            PFMDInterface pInterface = (PFMDInterface)pOutBuf;

            pInterface->cbSize = sizeof(FMDInterface);
            pInterface->pInit = FMD_Init;
            pInterface->pDeInit = FMD_Deinit;
            pInterface->pGetInfo = FMD_GetInfo;
            pInterface->pGetInfoEx = NULL;        //FMD_GetInfoEx;
            pInterface->pGetBlockStatus = FMD_GetBlockStatus;
            pInterface->pSetBlockStatus = FMD_SetBlockStatus;
            pInterface->pReadSector = FMD_ReadSector;
            pInterface->pWriteSector = FMD_WriteSector;
            pInterface->pEraseBlock = FMD_EraseBlock;
            pInterface->pPowerUp = FMD_PowerUp;
            pInterface->pPowerDown = FMD_PowerDown;
            pInterface->pGetPhysSectorAddr = NULL;

            break;
        }

    case IOCTL_FMD_LOCK_BLOCKS:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_LOCK_BLOCKS Not Supported\r\n")));
        return FALSE;

    case IOCTL_FMD_UNLOCK_BLOCKS:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_UNLOCK_BLOCKS Not Supported\r\n")));
        return FALSE;

    case IOCTL_FMD_READ_RESERVED:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_READ_RESERVED\r\n")));
        return FALSE;

    case IOCTL_FMD_WRITE_RESERVED:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_WRITE_RESERVED\r\n")));
        return FALSE;

    case IOCTL_FMD_GET_RESERVED_TABLE:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_GET_RESERVED_TABLE\r\n")));
        return FALSE;

    case IOCTL_FMD_SET_REGION_TABLE:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_SET_REGION_TABLE\r\n")));
        return FALSE;

    case IOCTL_FMD_SET_SECTORSIZE:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_SET_SECTORSIZE\r\n")));
        return FALSE;

    case IOCTL_FMD_RAW_WRITE_BLOCKS:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_RAW_WRITE_BLOCKS\r\n")));
        return FALSE;

    case IOCTL_FMD_GET_RAW_BLOCK_SIZE:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_GET_RAW_BLOCK_SIZE\r\n")));
        return FALSE;

    case IOCTL_FMD_GET_INFO:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_GET_INFO is unsupported\r\n")));
        return FALSE;

    case  IOCTL_FMD_SET_XIPMODE    :
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_SET_XIPMODE is unsupported\r\n")));
        return FALSE;

    case  IOCTL_FMD_GET_XIPMODE:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_FMD_GET_XIPMODE is unsupported\r\n")));
        return FALSE;

    case  IOCTL_DISK_FLUSH_CACHE:
        RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] FMD_OEMIoControl() : IOCTL_DISK_FLUSH_CACHE\r\n")));
        return TRUE;

    case IOCTL_DISK_GET_STORAGEID:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : IOCTL_DISK_GET_STORAGEID is unsupported\r\n")));
        return TRUE;

    default:
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_OEMIoControl() : Unknown IOCTL (0x%08x)\r\n"), dwIoControlCode));
        return FALSE;
    }

    return TRUE;
}

BOOL FMD_Deinit(PVOID hFMD)
{
    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] FMD_Deinit() \r\n")));
    
#ifndef BUILDING_BOOTLOADER // Not building boot loader
    if(g_pNFConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pNFConReg, sizeof(S3C6410_NAND_REG));
        g_pNFConReg = NULL;
    }
    if(g_pSysConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pSysConReg, sizeof(S3C6410_SYSCON_REG));
        g_pSysConReg = NULL;
    }    
#endif

    return(TRUE);
}


/*
    @func   BOOL | FMD_GetInfo | Provides information on the NAND flash.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL FMD_GetInfo(PFlashInfo pFlashInfo)
{
    UINT32 nCnt;
    UINT32 nNandID;
    UINT8 nMID, nDID;

    if (pFlashInfo == NULL)
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("[FMD:ERR] FMD_GetInfo() : Invalid Parameter\r\n")));
        return(FALSE);
    }

    pFlashInfo->flashType = NAND;

    nNandID = ReadFlashID();

    nMID = nNandID >> 8;
    nDID = nNandID & 0xff;

    for (nCnt = 0; astNandSpec[nCnt].nMID != 0; nCnt++)
    {
        if (nDID == astNandSpec[nCnt].nDID)
        {
            break;
        }
    }    

    pFlashInfo->flashType = NAND;

    //  OK, instead of reading it from the chip, we use the hardcoded numbers here.
    //  These information is filled on FMD_Init() into stDeviceInfo
    pFlashInfo->dwNumBlocks         = NUM_OF_BLOCKS;
    pFlashInfo->wSectorsPerBlock    = PAGES_PER_BLOCK;
    pFlashInfo->wDataBytesPerSector = NAND_PAGE_SIZE;
    pFlashInfo->dwBytesPerBlock     = (PAGES_PER_BLOCK * NAND_PAGE_SIZE);

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] FMD_GetInfo() : NUMBLOCKS = %d(0x%x), SECTORSPERBLOCK = %d(0x%x), BYTESPERSECTOR = %d(0x%x) \r\n"), pFlashInfo->dwNumBlocks, pFlashInfo->dwNumBlocks, pFlashInfo->wSectorsPerBlock, pFlashInfo->wSectorsPerBlock, pFlashInfo->wDataBytesPerSector, pFlashInfo->wDataBytesPerSector));

    return TRUE;
}


/*
    @func   DWORD | FMD_GetBlockStatus | Returns the status of the specified block.
    @rdesc  Block status (see fmd.h).
    @comm
    @xref
*/
DWORD FMD_GetBlockStatus(BLOCK_ID blockID)
{
    DWORD dwResult = 0;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] ++FMD_GetBlockStatus(0x%08x) \r\n"), blockID));

    if ( IS_LB )
    {
        dwResult = FMD_LB_GetBlockStatus(blockID);
    }
    else
    {
        dwResult = FMD_SB_GetBlockStatus(blockID);
    }

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] --FMD_GetBlockStatus()\r\n")));

    return dwResult;
}


/*
    @func   BOOL | FMD_SetBlockStatus | Marks the block with the specified block status.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL FMD_SetBlockStatus(BLOCK_ID blockID, DWORD dwStatus)
{
    BOOL    bRet = TRUE;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] ++FMD_SetBlockStatus(0x%08x, 0x%08x) \r\n"), blockID, dwStatus));

    if ( IS_LB )
    {
        bRet = FMD_LB_SetBlockStatus(blockID, dwStatus);
    }
    else
    {
        bRet = FMD_SB_SetBlockStatus(blockID, dwStatus);
    }

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("[FMD] --FMD_SetBlockStatus()\r\n")));

    return bRet;
}

BOOL ECC_CorrectData(SECTOR_ADDR sectoraddr, LPBYTE pData, UINT32 nRetEcc, ECC_CORRECT_TYPE nType)
{
    DWORD  nErrStatus;
    DWORD  nErrDataNo;
    DWORD  nErrBitNo;
    UINT32 nErrDataMask;
    UINT32 nErrBitMask = 0x7;
    BOOL bRet = TRUE;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("#### FMD_DRIVER::ECC_CorrectData %x, %x, %x\r\n"), sectoraddr, nRetEcc, nType));

    switch(nType)
    {
    case ECC_CORRECT_MAIN:
    case ECC_CORRECT_SPARE1:
    case ECC_CORRECT_SPARE2:
        nErrStatus   = 0;
        nErrDataNo   = 7;
        nErrBitNo    = 4;
        nErrDataMask = 0x7ff;
        break;
    case ECC_CORRECT_SPARE:
        nErrStatus   = 2;
        nErrDataNo   = 21;
        nErrBitNo    = 18;
        nErrDataMask = 0xf;
        break;
    default:
        return FALSE;
    }

    switch((nRetEcc>>nErrStatus) & 0x3)
    {
        case 0:    // No Error
            bRet = TRUE;
            break;
        case 1:    // 1-bit Error(Correctable)
            RETAILMSG(FMD_ZONE_STATUS,(TEXT("%cECC correctable error(0x%x). Byte:%d, bit:%d\r\n"), ((nType==ECC_CORRECT_MAIN)?'M':'S'), sectoraddr, (nRetEcc>>nErrDataNo)&nErrDataMask, (nRetEcc>>nErrBitNo)&nErrBitMask));
            (pData)[(nRetEcc>>nErrDataNo)&nErrDataMask] ^= (1<<((nRetEcc>>nErrBitNo)&nErrBitMask));
            bRet = TRUE;
            break;
        case 2:    // Multiple Error
            RETAILMSG(FMD_ZONE_STATUS,(TEXT("%cECC Uncorrectable error(0x%x)\r\n"), ((nType==ECC_CORRECT_MAIN)?'M':'S'), sectoraddr));
            bRet = FALSE;
            break;
        case 3:    // ECC area Error
            RETAILMSG(FMD_ZONE_STATUS,(TEXT("%cECC area error\r\n"), ((nType==ECC_CORRECT_MAIN)?'M':'S')));
            // Intentional fall through to default case
        default:
            bRet = FALSE;
            break;
    }

    return bRet;
}

void Init_NandController()
{
    // Set up initial flash controller configuration.
    // TACLS, TWRPH0, TWRPH1 is defined in bsp_cfg.h
    g_pNFConReg->NFCONF = (TACLS<<BIT_TACLS) | (TWRPH0<<BIT_TWRPH0) | (TWRPH1<<BIT_TWRPH1);
    g_pNFConReg->NFCONT = NFCONT_DISABLE_LOCK_TIGHT |
                            NFCONT_DISABLE_SOFTLOCK |
                            NFCONT_DISABLE_ILLEGAL_ACCESS_INT |
                            NFCONT_DISABLE_RNB_INT |
                            NFCONT_DETECT_RNB_RISING |
                            NFCONT_LOCK_MECC |
                            NFCONT_LOCK_SECC |
                            NFCONT_INIT_MECC |
                            NFCONT_INIT_SECC |
                            NFCONT_REG_NCE1_HIGH |
                            NFCONT_REG_NCE0_HIGH |
                            NFCONT_ENABLE ;
    SETBIT32(g_pNFConReg->NFSTAT, 4);   //< Clear RnB Transition low to high interrupt status, to clear write '1'
}

#ifdef _IROMBOOT_
BOOL FMD_WriteSector_Stepldr(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    BOOL    bRet = TRUE;

    if(IS_LB)
        bRet = FMD_LB_WriteSector_Steploader(startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors);
    else
        bRet = FMD_SB_WriteSector_Steploader(startSectorAddr, pSectorBuff, pSectorInfoBuff, dwNumSectors);

    return bRet;
}
#endif
