//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Copyright (c) Samsung Electronics. Co. LTD. All rights reserved.

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    fmd_lb.cpp

Abstract:

    This module implements main functions of FMD PDD for Large Block(1024K, 2048KBytes Block Size)

Functions:

    FMD_LB_*

Notes:

--*/

#include "precomp.h"

// SpareArea Map(Byte Addressing)
// 0~7   : SectorInfo
// 8~23  : MECC data(128bit)
// 24~27 : SECC1 Data(32bit) 
// 28~31 : SECC2 Data[32bit]

void SubmitSpareAreaReadCmd(DWORD SectorStartAddr, DWORD SpareStartAddr)
{
    NF_nFCE_L();

    NF_CLEAR_RB();

    NF_CMD(CMD_READ);                            // Send read command.

    NF_ADDR((SpareStartAddr)&0xff);
    NF_ADDR((SpareStartAddr>>8)&0xff);
    NF_ADDR((SectorStartAddr) & 0xff);
    NF_ADDR((SectorStartAddr >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((SectorStartAddr >> 16) & 0xff);
    }

    NF_CMD(CMD_READ3);                        // 2nd command

    NF_DETECT_RB();                                // Wait for command to complete.
}

// the 1st byte through ECC engine is badblock byte for SectorInfo structure in large block NAND flash
// the relationship between input through ECC engine and SectorInfo structure is like this
//
//   ReadByte  -  ECC input byte  -----------  SectorInfoLocation(if pSectorInfo = (LPBYTE)pData) (Little Endian)
//     1byte   -  bBadBlock                 -  6th byte  (*pData[5])
//     4byte   -  dwReserved1 & 0xff        -  1st byte  (*pData[0])
//             -  (dwReserved1>>8) & 0xff   -  2nd byte  (*pData[1])
//             -  (dwReserved1>>16) & 0xff  -  3rd byte  (*pData[2])
//             -  (dwReserved1>>24) & 0xff  -  4th byte  (*pData[3])
//     1byte   -  bOEMReserved              -  5th byte  (*pData[4])
//
void RestructSectorInfo(PSectorInfo pSectorInfoBuff, PBYTE RawSectorInfo)
{
    pSectorInfoBuff->bBadBlock = RawSectorInfo[0];
    pSectorInfoBuff->dwReserved1 = RawSectorInfo[1];
    pSectorInfoBuff->dwReserved1 |= RawSectorInfo[2]<<8;
    pSectorInfoBuff->dwReserved1 |= RawSectorInfo[3]<<16;
    pSectorInfoBuff->dwReserved1 |= RawSectorInfo[4]<<24;
    pSectorInfoBuff->bOEMReserved = RawSectorInfo[5];
    pSectorInfoBuff->wReserved2 = RawSectorInfo[6];
    pSectorInfoBuff->wReserved2 |= (RawSectorInfo[7]<<8);
}
        
void ReadOneSectorInfo(PBYTE RawSectorInfoBuf)        ///< Read One SectorInfo by sizeof(SectorInfo)
{
    int i;
    // Initialize MECC Module
    NF_RSTECC();
    NF_MECC_UnLock();

    for(i =0 ; i< 6; i++){  // Except Reserved2. Reserved2 area can vary 
        RawSectorInfoBuf[i] = NF_RDDATA_BYTE();
    }

    NF_MECC_Lock();
    RawSectorInfoBuf[6] = NF_RDDATA_BYTE();
    RawSectorInfoBuf[7] = NF_RDDATA_BYTE();    
}

BOOL FMD_LB_ReadSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    ULONG SectorAddr = (ULONG)startSectorAddr;
    DWORD       i;
    volatile DWORD        rddata;
    UINT32 nRetEcc = 0;
    DWORD MECCBuf[MAX_SECTORS_PER_PAGE];
    
    UINT16 nSectorLoop;
    int NewSpareAddr = NAND_PAGE_SIZE;
    int NewDataAddr = 0;
    int NewSectorAddr = startSectorAddr;

    DWORD SECCBuf1;         // 1st Spare ECC
    DWORD SECCBuf2;         // 2nd Spare ECC(cloned from 1st)
    BYTE RawSectorInfo[8];

    RETAILMSG(FMD_ZONE_FUNCTION,(TEXT("#### FMD_DRIVER:::FMD_LB_READSECTOR %x %x\r\n"),startSectorAddr,NewDataAddr));

    if (!pSectorBuff && !pSectorInfoBuff)
    {
        return(FALSE);
    }

    if ( dwNumSectors > 1 )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("######## FATAL ERROR => FMD::FMD_ReadSector->dwNumsectors is bigger than 1. \r\n")));
        return FALSE;
    }

    if (!pSectorBuff)
    {
        return NAND_LB_ReadSectorInfo(startSectorAddr, pSectorInfoBuff);
    }

    SubmitSpareAreaReadCmd(NewSectorAddr, NewSpareAddr+NAND_SECC1_OFFSET);    //< Spare Area Start Address, SECTOR_SIZE(512)*SECTOS_PER_PAGE(4)=2048 -> Physical Page Size

    // Read SECC1, SECC2 from Spare area
    SECCBuf1 = NF_RDDATA_WORD();
    SECCBuf2 = NF_RDDATA_WORD();

    // Set address for random access to read SectorInfo and MECC code from Spare Area
    NF_CMD(CMD_RDO);    // Send read random data output command
    NF_ADDR((NewSpareAddr)&0xff);
    NF_ADDR(((NewSpareAddr)>>8)&0xff);
    NF_CMD(CMD_RDO2);   // Command done

    if (pSectorInfoBuff)            /// Read SectorInfo doesn't check this
    {
        ReadOneSectorInfo(RawSectorInfo);        ///< Read One SectorInfo by sizeof(SectorInfo)
        
        // Check ECC abour SectorInfo         
        NF_WRMECCD0( ((SECCBuf1&0xff00)<<8)|(SECCBuf1&0xff) );    
        NF_WRMECCD1( ((SECCBuf1&0xff000000)>>8)|((SECCBuf1&0xff0000)>>16) );

        nRetEcc = NF_ECC_ERR0;

        if (!ECC_CorrectData(startSectorAddr, RawSectorInfo, nRetEcc, ECC_CORRECT_SPARE1)) // Use specific byte
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("#### SECC1 ECC correction failed\n")));   
            NF_nFCE_H();
            return FALSE;
        }

        RestructSectorInfo(pSectorInfoBuff, RawSectorInfo);
    }
    else
    {
         for(i=0; i<sizeof(SectorInfo)/sizeof(DWORD); i++)
         {
            rddata = (DWORD) NF_RDDATA_WORD();        // read and trash the data
         }
    }

    // Initialize MECC module
    NF_RSTECC();
    NF_MECC_UnLock();

    // Read MECC parity code from Spare Area
    for (nSectorLoop = 0; nSectorLoop < SECTORS_PER_PAGE; nSectorLoop++)
    {
        MECCBuf[nSectorLoop] = NF_RDDATA_WORD();
    }
    NF_MECC_Lock();

    // Check ECC about MECC parity code
    NF_WRMECCD0( ((SECCBuf2&0xff00)<<8)|(SECCBuf2&0xff) );
    NF_WRMECCD1( ((SECCBuf2&0xff000000)>>8)|((SECCBuf2&0xff0000)>>16) );

    nRetEcc = NF_ECC_ERR0;

    if (!ECC_CorrectData(startSectorAddr, (LPBYTE)MECCBuf, nRetEcc, ECC_CORRECT_SPARE2))
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("#### SECC2 ECC correction failed\n")));
        NF_nFCE_H();
        return FALSE;
    }

    // Read each Sector in the Page. (using SECTORS_PER_PAGE)
    for (nSectorLoop = 0; nSectorLoop < SECTORS_PER_PAGE; nSectorLoop++)
    {
        NewDataAddr = nSectorLoop * SECTOR_SIZE;

        NF_CMD(CMD_RDO);                            // Send read command.
        NF_ADDR((NewDataAddr)&0xff);
        NF_ADDR((NewDataAddr>>8)&0xff);
        NF_CMD(CMD_RDO2);    // 2nd command

        NF_RSTECC();
        NF_MECC_UnLock();

        if( ((DWORD) (pSectorBuff+nSectorLoop*SECTOR_SIZE)) & 0x3)
        {
            RdPage512Unalign(pSectorBuff+nSectorLoop*SECTOR_SIZE);
        }
        else
        {
            // Usual case to handle 4byte aligned buffer pointer
            RdPage512(pSectorBuff+nSectorLoop*SECTOR_SIZE);                    // Read page/sector data.
        }

        NF_MECC_Lock();

        // Check ECC about Main data with MECC parity code
        NF_WRMECCD0( ((MECCBuf[nSectorLoop]&0xff00)<<8)|(MECCBuf[nSectorLoop]&0xff) );
        NF_WRMECCD1( ((MECCBuf[nSectorLoop]&0xff000000)>>8)|((MECCBuf[nSectorLoop]&0xff0000)>>16) );

        nRetEcc = NF_ECC_ERR0;

        if (!ECC_CorrectData(startSectorAddr, pSectorBuff+nSectorLoop*SECTOR_SIZE, nRetEcc, ECC_CORRECT_MAIN))
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("#### MECC ECC correction failed\n")));
            NF_nFCE_H();            
            return FALSE;
        }
    }

    NF_nFCE_H();
    
    return TRUE;
}

BOOL NAND_LB_ReadSectorInfo(SECTOR_ADDR sectorAddr, PSectorInfo pInfo)
{
    BOOL bRet = TRUE;
    int NewSectorAddr = sectorAddr;
    UINT32 nRetEcc = 0;
    DWORD MECCBuf[MAX_SECTORS_PER_PAGE];
    DWORD SECCBuf;    
    BYTE RawSectorInfo[8];

/*    if(sectorAddr == 0)
    {
        // Bad Block must be treated as valid for steploader and eboot
        pInfo->bOEMReserved = ~(OEM_BLOCK_RESERVED | OEM_BLOCK_READONLY);
        pInfo->bBadBlock    = BADBLOCKMARK;
        pInfo->dwReserved1  = 0xffffffff;
        pInfo->wReserved2   = 0xffff;
        return TRUE;
    }
*/
    SubmitSpareAreaReadCmd(NewSectorAddr, NAND_PAGE_SIZE);
    
    ReadOneSectorInfo(RawSectorInfo);

    // Read MECC parity code from Spare area, actually don't use these data..
    MECCBuf[0] = NF_RDDATA_WORD();
    MECCBuf[1] = NF_RDDATA_WORD();
    MECCBuf[2] = NF_RDDATA_WORD();
    MECCBuf[3] = NF_RDDATA_WORD();
    // Check ECC about SectorInfo with SECC1 parity code
    SECCBuf = NF_RDDATA_WORD();


    NF_WRMECCD0( ((SECCBuf&0xff00)<<8)|(SECCBuf&0xff) );
    NF_WRMECCD1( ((SECCBuf&0xff000000)>>8)|((SECCBuf&0xff0000)>>16) );
    
    nRetEcc = NF_ECC_ERR0;

    bRet = ECC_CorrectData(sectorAddr, (LPBYTE)&RawSectorInfo, nRetEcc, ECC_CORRECT_SPARE1);
    if(!bRet)
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("#### Warning:NAND_LB_ReadSectorInfo Spare ECC operation for SectorInfo failed\n")));
        RETAILMSG(FMD_ZONE_READ, (TEXT("\n#### RawSectorInfo:bOEMReserved:0x%x, bBadBlock:0x%x, dwReserved1:0x%x, wReserved2:0x%x"),
                                        ((PSectorInfo)&RawSectorInfo)->bOEMReserved, ((PSectorInfo)&RawSectorInfo)->bBadBlock,
                                        ((PSectorInfo)&RawSectorInfo)->dwReserved1, ((PSectorInfo)&RawSectorInfo)->wReserved2));        
        RETAILMSG(FMD_ZONE_READ, (TEXT("\n#### SECCBuf : 0x%x"), SECCBuf));
//        NF_nFCE_H();
/*      Implementation 2
        pInfo->bOEMReserved = ~(OEM_BLOCK_RESERVED | OEM_BLOCK_READONLY);
        pInfo->bBadBlock    = BADBLOCKMARK;
        pInfo->dwReserved1  = 0xffffffff;
        pInfo->wReserved2   = 0xffff;
*/
//       return TRUE;        // Bad Block must be treated as valid for steploader and eboot

//        return bRet;
    }

    RestructSectorInfo(pInfo, RawSectorInfo);

    NF_nFCE_H();

    return bRet;
}

BOOL FMD_LB_WriteSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    DWORD   i;
    BOOL    bRet = TRUE;
    BYTE Status;    
    DWORD MECCBuf[MAX_SECTORS_PER_PAGE];
    UINT16 nSectorLoop;
    
    int NewSpareAddr = NAND_PAGE_SIZE;
    int NewDataAddr = 0;
    int NewSectorAddr = startSectorAddr;

    DWORD SECCBuf1;
    DWORD SECCBuf2;


    RETAILMSG(FMD_ZONE_WRITE, (TEXT("FMD::FMD_LB_WriteSector 0x%x \r\n"), startSectorAddr));

    if (!pSectorBuff && !pSectorInfoBuff)
        return(FALSE);

    if ( dwNumSectors > 1 )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("######## FATAL ERROR => FMD::FMD_WriteSector->dwNumsectors is bigger than 1. \r\n")));
        return FALSE;
    }

    if (!pSectorBuff)
    {
        NAND_LB_WriteSectorInfo(startSectorAddr, pSectorInfoBuff);
        return TRUE;
    }

    //  Enable Chip
    NF_nFCE_L();

    NF_CLEAR_RB();

    //  Issue command
    NF_CMD(CMD_WRITE);

    //  Setup address
    NF_ADDR((NewDataAddr)&0xff);
    NF_ADDR((NewDataAddr>>8)&0xff);
    NF_ADDR((NewSectorAddr)&0xff);
    NF_ADDR((NewSectorAddr>>8)&0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((NewSectorAddr>>16)&0xff);
    }

    for (nSectorLoop = 0; nSectorLoop < SECTORS_PER_PAGE; nSectorLoop++)
    {
        //  Initialize ECC register
        NF_RSTECC();
        NF_MECC_UnLock();

        //  Special case to handle un-aligned buffer pointer.
        //
        if( ((DWORD) (pSectorBuff+nSectorLoop*SECTOR_SIZE)) & 0x3)
        {
            //  Write the data
            WrPage512Unalign(pSectorBuff+nSectorLoop*SECTOR_SIZE);            
        }
        else
        {
            WrPage512(pSectorBuff+nSectorLoop*SECTOR_SIZE);
        }

        //  Read out the ECC value generated by HW
        NF_MECC_Lock();

        MECCBuf[nSectorLoop] = NF_RDMECC0();
    }

    NF_CMD(CMD_RDI);
    NF_ADDR((NewSpareAddr)&0xff);
    NF_ADDR((NewSpareAddr>>8)&0xff);

    // Write the SectorInfo data to the media
    // NOTE: This hardware is odd: only a byte can be written at a time and it must reside in the
    //       upper byte of a USHORT.
    if(pSectorInfoBuff)
    {
        // Initialize MECC module
        NF_RSTECC();
        NF_MECC_UnLock();

        //  Write SectorInfo
        NF_WRDATA_BYTE(pSectorInfoBuff->bBadBlock);
        NF_WRDATA_WORD(pSectorInfoBuff->dwReserved1);
        NF_WRDATA_BYTE(pSectorInfoBuff->bOEMReserved);

        NF_MECC_Lock();

        // Store SECC parity code for SectorInfo
        SECCBuf1 = NF_RDMECC0();

        NF_WRDATA_BYTE(pSectorInfoBuff->wReserved2&0xff);
        NF_WRDATA_BYTE((pSectorInfoBuff->wReserved2>>8)&0xff);
    }
    else
    {
        // Make sure we advance the Flash's write pointer (even though we aren't writing the SectorInfo data)
        for(i=0; i<sizeof(SectorInfo)/sizeof(DWORD); i++)
        {
            NF_WRDATA_WORD(0xffffffff);
        }
    }


    //  Write the ECC value to the flash (16 btes for 4 sectors)
    // Initialize MECC module
    NF_RSTECC();
    NF_MECC_UnLock();

    // Store MECC parity code for main data
    NF_WRDATA_WORD(MECCBuf[0]);
    NF_WRDATA_WORD(MECCBuf[1]);
    NF_WRDATA_WORD(MECCBuf[2]);
    NF_WRDATA_WORD(MECCBuf[3]);

    NF_MECC_Lock();     // get SECC

    // Store MECC parity code for MECC parity code
    SECCBuf2 = NF_RDMECC0();

    if(pSectorInfoBuff)
    {
        // Store SECC1, SECC2
        NF_WRDATA_WORD(SECCBuf1);
        NF_WRDATA_WORD(SECCBuf2);
    }

    //  Finish up the write operation
    NF_CMD(CMD_WRITE2);

    //  Wait for RB
    NF_DETECT_RB();     // Wait tR(max 12us)

    if ( NF_RDSTAT & STATUS_ILLACC )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("FMD_WriteSector() ######## Error Programming page (Illegal Access) %d!\n"), startSectorAddr));
        g_pNFConReg->NFSTAT =  STATUS_ILLACC;    // Write 1 to clear.
        bRet = FALSE;
    }
    else
    {
        //  Check the status
        do
        {
            NF_CMD(CMD_STATUS);
            Status = NF_RDDATA_BYTE();                    // Read command status.
        }while(!(Status & STATUS_READY));

        if(Status & STATUS_ERROR)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("FMD_WriteSector() ######## Error Programming page %d!\n"), startSectorAddr));
            bRet = FALSE;
        }
    }

    //  Disable the chip
    NF_nFCE_H();

    return bRet;
}

BOOL NAND_LB_WriteSectorInfo(SECTOR_ADDR sectorAddr, PSectorInfo pInfo)
{
    BOOL    bRet = TRUE;
    int NewSpareAddr = NAND_PAGE_SIZE;
    int NewSectorAddr = sectorAddr;

    DWORD SECCBuf1;

    //  Chip enable
    NF_nFCE_L();

    NF_CLEAR_RB();

    //  Write the command
    //  First, let's point to the spare area
    NF_CMD(CMD_WRITE);

    //  Write the address
    NF_ADDR((NewSpareAddr)&0xff);
    NF_ADDR((NewSpareAddr>>8)&0xff);
    NF_ADDR((NewSectorAddr)&0xff);
    NF_ADDR((NewSectorAddr>>8)&0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((NewSectorAddr>>16)&0xff);
    }

    // Initialize MECC module
    NF_RSTECC();
    NF_MECC_UnLock();

    //  Now let's write the SectorInfo data
    //  Write the first reserved field (DWORD)
    NF_WRDATA_BYTE(pInfo->bBadBlock);
    NF_WRDATA_WORD(pInfo->dwReserved1);
    NF_WRDATA_BYTE(pInfo->bOEMReserved);

    NF_MECC_Lock();

    SECCBuf1 = NF_RDMECC0();

    NF_WRDATA_BYTE(pInfo->wReserved2&0xff);
    NF_WRDATA_BYTE((pInfo->wReserved2>>8)&0xff);

    // Issue the write complete command
    NF_CMD(CMD_RDI);
    // Set Address to access SECC1 in Spare area
    NF_ADDR((NewSpareAddr+NAND_SECC1_OFFSET)&0xff);
    NF_ADDR(((NewSpareAddr+NAND_SECC1_OFFSET)>>8)&0xff);

    // Write SECC1
    NF_WRDATA_WORD(SECCBuf1);

    //  Issue the write complete command
    NF_CMD(CMD_WRITE2);

    //  Check ready bit
    NF_DETECT_RB();     // Wait tR(max 12us)

    if ( NF_RDSTAT & STATUS_ILLACC )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("NAND_LB_WriteSectorInfo() ######## Error Programming page (Illegal Access) %d!\n"), sectorAddr));
        g_pNFConReg->NFSTAT =  STATUS_ILLACC;    // Write 1 to clear.
           bRet = FALSE;
    }
    else
    {
        //  Check the status of program
        NF_CMD(CMD_STATUS);

        if( NF_RDDATA_BYTE() & STATUS_ERROR) {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("NAND_LB_WriteSectorInfo() ######## Error Programming page %d!\n"), sectorAddr));
            bRet = FALSE;
        }
    }

    NF_nFCE_H();

    return bRet;
}

BOOL FMD_LB_EraseBlock(BLOCK_ID blockID)
{
    BOOL    bRet = TRUE;
    DWORD   dwPageID = blockID << LB_NAND_LOG_2_PAGES_PER_BLOCK;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("FMD_LB_EraseBlock 0x%x \r\n"), blockID));

    //  Enable the chip
    NF_nFCE_L();        // Select the flash chip.

    NF_CLEAR_RB();

    //  Issue command
    NF_CMD(CMD_ERASE);

    //  Set up address
    NF_ADDR((dwPageID) & 0xff);
    NF_ADDR((dwPageID >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwPageID >> 16) & 0xff);
    }

    //  Complete erase operation
    NF_CMD(CMD_ERASE2);

    //  Wait for ready bit
    NF_DETECT_RB();     // Wait tR(max 12us)

    if ( NF_RDSTAT & STATUS_ILLACC )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("LB######## Error Erasing block (Illegal Access) %d!\n"), blockID));
        g_pNFConReg->NFSTAT =  STATUS_ILLACC;    // Write 1 to clear.
        bRet = FALSE;
    }
    else
    {
        //  Check the status
        NF_CMD(CMD_STATUS);

        if( NF_RDDATA_BYTE() & STATUS_ERROR)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("LB######## Error Erasing block %d!\n"), blockID));
            bRet = FALSE;
        }
    }

    NF_nFCE_H();                        // Select the flash chip.

    return bRet;
}

BOOL FMD_LB_EraseSector(DWORD dwPageID)
{
    BOOL    bRet = TRUE;
   // DWORD   dwPageID = blockID << LB_NAND_LOG_2_PAGES_PER_BLOCK;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("FMD_LB_EraseSector 0x%x \r\n"), dwPageID));

    //  Enable the chip
    NF_nFCE_L();        // Select the flash chip.

    NF_CLEAR_RB();

    //  Issue command
    NF_CMD(CMD_ERASE);

    //  Set up address
    NF_ADDR((dwPageID) & 0xff);
    NF_ADDR((dwPageID >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwPageID >> 16) & 0xff);
    }

    //  Complete erase operation
    NF_CMD(CMD_ERASE2);

    //  Wait for ready bit
    NF_DETECT_RB();     // Wait tR(max 12us)

    if ( NF_RDSTAT & STATUS_ILLACC )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("LB######## Error Erasing Sector (Illegal Access) %d!\n"), dwPageID));
        g_pNFConReg->NFSTAT =  STATUS_ILLACC;    // Write 1 to clear.
        bRet = FALSE;
    }
    else
    {
        //  Check the status
        NF_CMD(CMD_STATUS);

        if( NF_RDDATA_BYTE() & STATUS_ERROR)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("LB######## Error Erasing Sector %d!\n"), dwPageID));
            bRet = FALSE;
        }
    }

    NF_nFCE_H();                        // Select the flash chip.

    return bRet;
}

DWORD FMD_LB_GetBlockStatus(BLOCK_ID blockID)
{
    SECTOR_ADDR sectorAddr = blockID << LB_NAND_LOG_2_PAGES_PER_BLOCK;
    SectorInfo SI;
    DWORD dwResult = 0;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("FMD_LB_GetBlockStatus (0x%x)0x%x \r\n"), blockID, sectorAddr));

    if(!FMD_LB_ReadSector(sectorAddr, NULL, &SI, 1))
    {
        if(NAND_LB_IsBlockBad(blockID))
        {
            return BLOCK_STATUS_BAD;
        }
        else
        {
            return BLOCK_STATUS_UNKNOWN;
        }
    }
    if(!(SI.bOEMReserved & OEM_BLOCK_READONLY))
    {
        dwResult |= BLOCK_STATUS_READONLY;
    }

    if (!(SI.bOEMReserved & OEM_BLOCK_RESERVED))
    {
        dwResult |= BLOCK_STATUS_RESERVED;
    }

    if(SI.bBadBlock != 0xFF)
    {
        dwResult |= BLOCK_STATUS_BAD;
    }

    return dwResult;
}
DWORD FMD_LB_GetSectorStatus(SECTOR_ADDR sectorAddr)
{
    //SECTOR_ADDR sectorAddr = blockID << LB_NAND_LOG_2_PAGES_PER_BLOCK;
    SectorInfo SI;
    DWORD dwResult = 0;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("FMD_LB_GetSectorStatus (0x%x)0x%x \r\n"), sectorAddr, sectorAddr));

    if(!FMD_LB_ReadSector(sectorAddr, NULL, &SI, 1))
    {
        if(NAND_LB_IsSectorBad(sectorAddr))
        {
            return BLOCK_STATUS_BAD;
        }
        else
        {
            return BLOCK_STATUS_UNKNOWN;
        }
    }
    if(!(SI.bOEMReserved & OEM_BLOCK_READONLY))
    {
        dwResult |= BLOCK_STATUS_READONLY;
    }

    if (!(SI.bOEMReserved & OEM_BLOCK_RESERVED))
    {
        dwResult |= BLOCK_STATUS_RESERVED;
    }

    if(SI.bBadBlock != 0xFF)
    {
        dwResult |= BLOCK_STATUS_BAD;
    }

    return dwResult;
}
BOOL FMD_LB_SetBlockStatus(BLOCK_ID blockID, DWORD dwStatus)
{
    BYTE bStatus = 0;

    if(dwStatus & BLOCK_STATUS_BAD)
    {
        if(!NAND_LB_MarkBlockBad (blockID))
        {
            return FALSE;
        }
    }

    // We don't currently support setting a block to read-only, so fail if request is
    // for read-only and block is not currently read-only.
    if(dwStatus & BLOCK_STATUS_READONLY)
    {
        if(!(FMD_LB_GetBlockStatus(blockID) & BLOCK_STATUS_READONLY))
        {
            return FALSE;
        }
    }

    return TRUE;
}
BOOL FMD_LB_SetSectorStatus(SECTOR_ADDR sectorID, DWORD dwStatus)
{
    BYTE bStatus = 0;

    if(dwStatus & BLOCK_STATUS_BAD)
    {
        if(!NAND_LB_MarkSectorBad (sectorID))
        {
            return FALSE;
        }
    }

    // We don't currently support setting a block to read-only, so fail if request is
    // for read-only and block is not currently read-only.
    if(dwStatus & BLOCK_STATUS_READONLY)
    {
        if(!(FMD_LB_GetSectorStatus(sectorID) & BLOCK_STATUS_READONLY))
        {
            return FALSE;
        }
    }

    return TRUE;
}
BOOL NAND_LB_MarkBlockBad(BLOCK_ID blockID)
{
    DWORD   dwStartPage = blockID << LB_NAND_LOG_2_PAGES_PER_BLOCK;
    BOOL    bRet = TRUE;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("NAND_LB_MarkBlockBad 0x%x \r\n"), dwStartPage));

    //  Enable chip
    NF_nFCE_L();
    NF_CLEAR_RB();

    //  Issue command
    //  We are dealing with spare area
    NF_CMD(CMD_WRITE);

    //  Set up address
    NF_ADDR((NAND_PAGE_SIZE+LB_POS_BADBLOCK)&0xff);
    NF_ADDR(((NAND_PAGE_SIZE+LB_POS_BADBLOCK)>>8)&0xff);
    NF_ADDR((dwStartPage) & 0xff);
    NF_ADDR((dwStartPage >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwStartPage >> 16) & 0xff);
    }

    NF_WRDATA_BYTE(BADBLOCKMARK);

    //  Copmlete the write
    NF_CMD(CMD_WRITE2);

    //  Wait for RB
    NF_DETECT_RB();     // Wait tR(max 12us)

    if ( NF_RDSTAT & STATUS_ILLACC )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("NAND_LB_WriteSectorInfo() ######## Error Programming page (Illegal Access) %d!\n")));
        g_pNFConReg->NFSTAT =  STATUS_ILLACC;    // Write 1 to clear.
        bRet = FALSE;
    }
    else
    {
        //  Check the status of program
        NF_CMD(CMD_STATUS);

        if( NF_RDDATA_BYTE() & STATUS_ERROR)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("NAND_LB_WriteSectorInfo() ######## Error Programming page %d!\n")));
            bRet = FALSE;
        }
    }

    //  Disable chip select
    NF_nFCE_H();

    return bRet;
}
BOOL NAND_LB_MarkSectorBad(SECTOR_ADDR sectorID)
{
    DWORD   dwStartPage = sectorID;
    BOOL    bRet = TRUE;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("NAND_LB_MarkSectorBad 0x%x \r\n"), dwStartPage));

    //  Enable chip
    NF_nFCE_L();
    NF_CLEAR_RB();

    //  Issue command
    //  We are dealing with spare area
    NF_CMD(CMD_WRITE);

    //  Set up address
    NF_ADDR((NAND_PAGE_SIZE+LB_POS_BADBLOCK)&0xff);
    NF_ADDR(((NAND_PAGE_SIZE+LB_POS_BADBLOCK)>>8)&0xff);
    NF_ADDR((dwStartPage) & 0xff);
    NF_ADDR((dwStartPage >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwStartPage >> 16) & 0xff);
    }

    NF_WRDATA_BYTE(BADBLOCKMARK);

    //  Copmlete the write
    NF_CMD(CMD_WRITE2);

    //  Wait for RB
    NF_DETECT_RB();     // Wait tR(max 12us)

    if ( NF_RDSTAT & STATUS_ILLACC )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("NAND_LB_WriteSectorInfo() ######## Error Programming page (Illegal Access) %d!\n")));
        g_pNFConReg->NFSTAT =  STATUS_ILLACC;    // Write 1 to clear.
        bRet = FALSE;
    }
    else
    {
        //  Check the status of program
        NF_CMD(CMD_STATUS);

        if( NF_RDDATA_BYTE() & STATUS_ERROR)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("NAND_LB_WriteSectorInfo() ######## Error Programming page %d!\n")));
            bRet = FALSE;
        }
    }

    //  Disable chip select
    NF_nFCE_H();

    return bRet;
}
BOOL NAND_LB_IsBlockBad(BLOCK_ID blockID)
{
    DWORD   dwPageID = blockID << LB_NAND_LOG_2_PAGES_PER_BLOCK;
    BOOL    bRet = FALSE;
    BYTE    bFlag;

    //  Enable the chip
    NF_nFCE_L();
    NF_CLEAR_RB();

    //  Issue the command
    NF_CMD(CMD_READ);

    //  Set up address
    NF_ADDR((NAND_PAGE_SIZE+LB_POS_BADBLOCK)&0xff);
    NF_ADDR(((NAND_PAGE_SIZE+LB_POS_BADBLOCK)>>8)&0xff);
    NF_ADDR((dwPageID) & 0xff);
    NF_ADDR((dwPageID >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwPageID >> 16) & 0xff);
    }

    NF_CMD(CMD_READ3);

    //  Wait for Ready bit
    NF_DETECT_RB();     // Wait tR(max 12us)

    //  Now get the byte we want
    bFlag = (BYTE)NF_RDDATA_BYTE();

    if(bFlag != 0xff)
    {
        RETAILMSG(FMD_ZONE_STATUS, (TEXT("FMDLB: IsBlockBad - Page #: 0x%x \r\n"), dwPageID));
        bRet = TRUE;
    }

    //  Disable the chip
    NF_nFCE_H();

    return bRet;
}
BOOL NAND_LB_IsSectorBad(SECTOR_ADDR sectorID)
{
    DWORD   dwPageID = sectorID;
    BOOL    bRet = FALSE;
    BYTE    bFlag;

    //  Enable the chip
    NF_nFCE_L();
    NF_CLEAR_RB();

    //  Issue the command
    NF_CMD(CMD_READ);

    //  Set up address
    NF_ADDR((NAND_PAGE_SIZE+LB_POS_BADBLOCK)&0xff);
    NF_ADDR(((NAND_PAGE_SIZE+LB_POS_BADBLOCK)>>8)&0xff);
    NF_ADDR((dwPageID) & 0xff);
    NF_ADDR((dwPageID >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwPageID >> 16) & 0xff);
    }

    NF_CMD(CMD_READ3);

    //  Wait for Ready bit
    NF_DETECT_RB();     // Wait tR(max 12us)

    //  Now get the byte we want
    bFlag = (BYTE)NF_RDDATA_BYTE();

    if(bFlag != 0xff)
    {
        RETAILMSG(FMD_ZONE_STATUS, (TEXT("FMDLB: IsBlockBad - Page #: 0x%x \r\n"), dwPageID));
        bRet = TRUE;
    }

    //  Disable the chip
    NF_nFCE_H();

    return bRet;
}
#ifdef _IROMBOOT_
BOOL FMD_LB_WriteSector_Steploader(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    DWORD   i;
    UINT16 nSectorLoop;
    int NewDataAddr = 0;
    int NewSectorAddr = startSectorAddr;
    MECC8 t8MECC[4];

    //Check Parameters.
    if (!pSectorBuff && !pSectorInfoBuff)
    {
        return(FALSE);
    }
    if ( dwNumSectors > 1 )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("######## FATAL ERROR => FMD::FMD_WriteSector->dwNumsectors is bigger than 1. \r\n")));
        return FALSE;
    }

    g_pNFConReg->NFCONF = (g_pNFConReg->NFCONF & ~(1<<30)) | (1<<23) | (NAND_TACLS <<12) | (NAND_TWRPH0 <<8) | (NAND_TWRPH1 <<4);
    g_pNFConReg->NFCONT |= (1<<18)|(1<<13)|(1<<12)|(1<<11)|(1<<10)|(1<<9); 
    g_pNFConReg->NFSTAT |= ((1<<6)|(1<<5)|(1<<4));

    //  Enable Chip
    NF_nFCE_L();

    //  Issue command
    NF_CMD(CMD_WRITE);    //0x80

    //  Setup address to write Main data
    NF_ADDR((NewDataAddr)&0xff);    // 2bytes for column address
    NF_ADDR((NewDataAddr>>8)&0xff);
    NF_ADDR((NewSectorAddr)&0xff);    // 3bytes for row address
    NF_ADDR((NewSectorAddr>>8)&0xff);
//    if(g_bNeedExtAddr)
    {
        NF_ADDR((NewSectorAddr>>16)&0xff);
    }

    // initialize variable.
    for(i = 0; i < 4; i++) {
        t8MECC[i].n8MECC0 = 0x0;
        t8MECC[i].n8MECC1 = 0x0;
        t8MECC[i].n8MECC2 = 0x0;
        t8MECC[i].n8MECC3 = 0x0;
    }

    // Write each Sector in the Page. (4 Sector per Page, Loop 4 times.)
    for (nSectorLoop = 0; nSectorLoop < SECTORS_PER_PAGE; nSectorLoop++)
    {
        //  Initialize ECC register
        NF_MECC_UnLock();
        NF_RSTECC();        

        // Special case to handle un-aligned buffer pointer.
        if( ((DWORD) (pSectorBuff+nSectorLoop*SECTOR_SIZE)) & 0x3) 
        {
            //  Write the data        
            WrPage512Unalign(pSectorBuff+nSectorLoop*SECTOR_SIZE);
        }
        else
        {
            WrPage512(pSectorBuff+nSectorLoop*SECTOR_SIZE);
        }

        NF_MECC_Lock();

        while(!(g_pNFConReg->NFSTAT&(1<<7)));
        g_pNFConReg->NFSTAT|=(1<<7);

        //  Read out the ECC value generated by HW
        t8MECC[nSectorLoop].n8MECC0 = NF_8MECC0();
        t8MECC[nSectorLoop].n8MECC1 = NF_8MECC1();
        t8MECC[nSectorLoop].n8MECC2 = NF_8MECC2();
        t8MECC[nSectorLoop].n8MECC3 = (NF_8MECC3() & 0xff);

    }

    for(nSectorLoop = 0; nSectorLoop < 4; nSectorLoop++)
    {
        NF_WRDATA_WORD(t8MECC[nSectorLoop].n8MECC0); // 4 byte n8MECC0
        NF_WRDATA_WORD(t8MECC[nSectorLoop].n8MECC1); // 4 byte n8MECC1
        NF_WRDATA_WORD(t8MECC[nSectorLoop].n8MECC2); // 4 byte n8MECC2
        NF_WRDATA_BYTE((t8MECC[nSectorLoop].n8MECC3) & 0xff); // 1 byte n8MECC3
    }

    g_pNFConReg->NFSTAT |=  (1<<4); //NF_CLEAR_RB

    //  Finish up the write operation
    NF_CMD(CMD_WRITE2);    // 0x10

    NF_DETECT_RB();     // Wait tR(max 12us)

    NF_CMD(CMD_STATUS);   // Read status command       

    for(i=0;i<3;i++);  //twhr=60ns

    if(NF_RDDATA_BYTE() & STATUS_ERROR)
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("FMD_WriteSector() ######## Error Programming page %d!\n"), startSectorAddr));
        //  Disable the chip
        NF_nFCE_H();
        g_pNFConReg->NFCONF = (0<<23) | (NAND_TACLS <<12) | (NAND_TWRPH0 <<8) | (NAND_TWRPH1 <<4);
        return FALSE;
    }
    else
    {
        NF_nFCE_H();
        g_pNFConReg->NFCONF = (0<<23) | (NAND_TACLS <<12) | (NAND_TWRPH0 <<8) | (NAND_TWRPH1 <<4);
        return TRUE;
    }
}
#endif

BOOL RAW_LB_ReadSector(UINT32 startSectorAddr, LPBYTE pSectorBuff, LPBYTE pSectorInfoBuff)
{
    UINT32 nPageAddr = startSectorAddr;
    DWORD  i;
    UINT32 nRetEcc = 0;
    UINT32 nSectorLoop;
    UINT32 nColAddr = 0;

    if (!pSectorBuff && !pSectorInfoBuff)
        return(FALSE);

    NF_nFCE_L();

    NF_CLEAR_RB();

    NF_CMD(CMD_READ);                            // Send read command.

    NF_ADDR((nColAddr)&0xff);
    NF_ADDR((nColAddr>>8)&0xff);
    NF_ADDR((nPageAddr)&0xff);
    NF_ADDR((nPageAddr>>8)&0xff);
    if (g_bNeedExtAddr)
    {
        NF_ADDR((nPageAddr>>16)&0xff);  
    }

    NF_CMD(CMD_READ3);    // 2nd command
    NF_DETECT_RB();                                // Wait for command to complete.

    for (nSectorLoop = 0; nSectorLoop < 4; nSectorLoop++)
    {
        if( ((DWORD) (pSectorBuff+nSectorLoop*SECTOR_SIZE)) & 0x3)
        {
            RdPage512Unalign(pSectorBuff+nSectorLoop*SECTOR_SIZE);
        }
        else
        {
            RdPage512(pSectorBuff+nSectorLoop*SECTOR_SIZE);                    // Read page/sector data.
        }
    }

    for(i=0; i<64; i++)
    {
        *pSectorInfoBuff = NF_RDDATA_BYTE();        // read and trash the data
        pSectorInfoBuff+=1;
    }

    NF_nFCE_H();

    return(TRUE);
}
