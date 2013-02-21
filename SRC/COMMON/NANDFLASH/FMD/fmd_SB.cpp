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

    fmd_sb.cpp

Abstract:

    This module implements main functions of FMD PDD for Small Block(512Kbyte Block size)

Functions:

    FMD_Init,  

Notes:

--*/

#include "precomp.h"

extern volatile S3C6410_NAND_REG *g_pNFConReg;

BOOL FMD_SB_ReadSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    ULONG SectorAddr = (ULONG)startSectorAddr;
    ULONG MECC;
    UINT32 nRet = TRUE;
    UINT32 nRetEcc = 0;

    if (!pSectorBuff && !pSectorInfoBuff)
    {
        RETAILMSG(FMD_ZONE_ERROR,(TEXT("[FMD:ERR] FMD_SB_ReadSector(0x%08x, 0x%08x) : Invalid Parameter\n\r"), pSectorBuff, pSectorInfoBuff));
        return(FALSE);
    }

    while (dwNumSectors--)
    {
        NF_RSTECC();
        NF_MECC_UnLock();
        NF_nFCE_L();

        if (!pSectorBuff)
        {
            NF_CLEAR_RB();
            NF_CMD(CMD_READ2);            // Send read confirm command.

            NF_ADDR(0);                                    // Ignored.
            NF_ADDR(SectorAddr         & 0xff);            // Page address.
            NF_ADDR((SectorAddr >>  8) & 0xff);
            if(g_bNeedExtAddr)
            {
                NF_ADDR((SectorAddr >> 16) & 0xff);
            }

            NF_DETECT_RB();

            RdPageInfo((PBYTE)pSectorInfoBuff);    // Read page/sector information.

            pSectorInfoBuff++;
        }
        else
        {
            NF_CLEAR_RB();

            NF_CMD(CMD_READ);                    // Send read command.

            NF_ADDR(0);                                    // Column = 0.
            NF_ADDR(SectorAddr         & 0xff);            // Page address.
            NF_ADDR((SectorAddr >>  8) & 0xff);
            if(g_bNeedExtAddr)
            {
                NF_ADDR((SectorAddr >> 16) & 0xff);
            }

            NF_DETECT_RB();                    // Wait for command to complete.

            if( ((DWORD) pSectorBuff) & 0x3)
            {
                RdPage512Unalign (pSectorBuff);
            }
            else
            {
                RdPage512(pSectorBuff);                    // Read page/sector data.
            }

            NF_MECC_Lock();

            if (pSectorInfoBuff)
            {
                RdPageInfo((PBYTE)pSectorInfoBuff);        // Read page/sector information.
                pSectorInfoBuff ++;
            }
            else
            {
                BYTE TempInfo[8];
                RdPageInfo(TempInfo);                       // Read page/sector information.
            }

            MECC  = NF_RDDATA_BYTE() << 0;
            MECC |= NF_RDDATA_BYTE() << 8;
            MECC |= NF_RDDATA_BYTE() << 16;
            MECC |= NF_RDDATA_BYTE() << 24;

            NF_WRMECCD0( ((MECC&0xff00)<<8)|(MECC&0xff) );
            NF_WRMECCD1( ((MECC&0xff000000)>>8)|((MECC&0xff0000)>>16) );

            nRetEcc = NF_ECC_ERR0;
            
            if (!ECC_CorrectData(SectorAddr, pSectorBuff, nRetEcc, ECC_CORRECT_MAIN)) // Use specific byte
            {
                RETAILMSG(FMD_ZONE_ERROR, (TEXT("#### SECC1 ECC correction failed\n")));   
                NF_nFCE_H();
                return FALSE;
            }            

            pSectorBuff += NAND_PAGE_SIZE;
        }
        NF_nFCE_H();
        ++SectorAddr;
    }

    return(nRet);
}

BOOL FMD_SB_WriteSector(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    BYTE Status;
    ULONG SectorAddr = (ULONG)startSectorAddr;
    ULONG MECC;

    if (!pSectorBuff && !pSectorInfoBuff)
        return(FALSE);

    if (dwNumSectors <= 0)
    {
        RETAILMSG(FMD_ZONE_ERROR,(TEXT("[FMD:ERR] FMD_SB_ReadSector(0x%08x, 0x%08x) : Invalid Parameter dwNumSectors:%d\n\r"), pSectorBuff, pSectorInfoBuff, dwNumSectors));
        return(FALSE);
    }

    RETAILMSG(FMD_ZONE_FUNCTION,(TEXT("#### FMD_DRIVER:::FMD_sbwrite \r\n")));

    NF_nFCE_L();                        // Select the flash chip.

    while (dwNumSectors--)
    {
        if (!pSectorBuff)    // Only spare area
        {
            // If we are asked just to write the SectorInfo, we will do that separately
            NF_CMD(CMD_READ2);                             // Send read command.

            NF_CMD(CMD_WRITE);                            // Send write command.
            NF_ADDR(0);                                    // Column = 0.
            NF_ADDR(SectorAddr         & 0xff);            // Page address.
            NF_ADDR((SectorAddr >>  8) & 0xff);
            if(g_bNeedExtAddr)
            {
                NF_ADDR((SectorAddr >> 16) & 0xff);
            }

            // Write the SectorInfo data to the media.
            // Spare area[7:0]
            WrPageInfo((PBYTE)pSectorInfoBuff);

            NF_CLEAR_RB();
            NF_CMD(CMD_WRITE2);                // Send write confirm command.
            NF_DETECT_RB();

            NF_CMD(CMD_STATUS);
            Status = NF_RDDATA_BYTE();                    // Read command status.

            if (Status & STATUS_ERROR)
            {
                NF_nFCE_H();                            // Deselect the flash chip.

                return(FALSE);
            }
            pSectorInfoBuff++;
        }
        else         // Main area+Spare area.
        {
            NF_CMD(CMD_READ);                             // Send read command.
            NF_CMD(CMD_WRITE);                            // Send write command.
            NF_ADDR(0);                                    // Column = 0.
            NF_ADDR(SectorAddr         & 0xff);            // Page address.
            NF_ADDR((SectorAddr >>  8) & 0xff);
            if(g_bNeedExtAddr)
            {
                NF_ADDR((SectorAddr >> 16) & 0xff);
            }

            //  Special case to handle un-aligned buffer pointer.
            NF_RSTECC();
            NF_MECC_UnLock();
            if( ((DWORD) pSectorBuff) & 0x3)
            {
                WrPage512Unalign (pSectorBuff);
            }
            else
            {
                WrPage512(pSectorBuff);                    // Write page/sector data.
            }
            NF_MECC_Lock();

            // Write the SectorInfo data to the media.
            // Spare area[7:0]
            if(pSectorInfoBuff)
            {
                WrPageInfo((PBYTE)pSectorInfoBuff);
                pSectorInfoBuff++;
            }
            else    // Make sure we advance the Flash's write pointer (even though we aren't writing the SectorInfo data)
            {
                BYTE TempInfo[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                WrPageInfo(TempInfo);
            }

            // Write the SectorInfo data to the media.
            // Spare area[11:8]
            // Get the ECC data from status register.
            MECC = NF_RDMECC0();
            // Now, Write the ECC data to Spare area[11:8]
            NF_WRDATA_BYTE((UCHAR)((MECC      ) & 0xff));        // Spare area offset 8
            NF_WRDATA_BYTE((UCHAR)((MECC >>  8) & 0xff));    // Spare area offset 9
            NF_WRDATA_BYTE((UCHAR)((MECC >> 16) & 0xff));    // Spare area offset 10
            NF_WRDATA_BYTE((UCHAR)((MECC >> 24) & 0xff));    // Spare area offset 11

            NF_CLEAR_RB();
            NF_CMD(CMD_WRITE2);                            // Send write confirm command.
            NF_DETECT_RB();

            do
            {
                NF_CMD(CMD_STATUS);
                Status = NF_RDDATA_BYTE();                    // Read command status.
            }while(!(Status & STATUS_READY));

            if (Status & STATUS_ERROR)
            {
                NF_nFCE_H();                            // Deselect the flash chip.
                return(FALSE);
            }
            pSectorBuff += NAND_PAGE_SIZE;
        }
        ++SectorAddr;
    }
    NF_nFCE_H();                                        // Deselect the flash chip.

    return(TRUE);
}

BOOL FMD_SB_EraseBlock(BLOCK_ID blockID)
{
    BOOL    bRet = TRUE;
    DWORD   dwPageID = blockID << SB_NAND_LOG_2_PAGES_PER_BLOCK;

    //  Enable the chip
    NF_nFCE_L();                        // Select the flash chip.

    //  Issue command
    NF_CMD(CMD_ERASE);

    //  Set up address
    NF_ADDR((dwPageID) & 0xff);
    NF_ADDR((dwPageID >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwPageID >> 16) & 0xff);
    }

    NF_CLEAR_RB();

    //  Complete erase operation
    NF_CMD(CMD_ERASE2);

    //  Wait for ready bit
    NF_DETECT_RB();     // Wait tR(max 12us)

    if ( NF_RDSTAT & STATUS_ILLACC )
    {
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("SB######## Error Erasing block (Illigar Access) %d!\n"), blockID));
        g_pNFConReg->NFSTAT =  STATUS_ILLACC;    // Write 1 to clear.
        bRet = FALSE;
    }
    else
    {
        //  Check the status
        NF_CMD(CMD_STATUS);

        if( NF_RDDATA_BYTE() & STATUS_ERROR)
        {
            RETAILMSG(FMD_ZONE_ERROR, (TEXT("SB######## Error Erasing block %d!\n"), blockID));
            bRet = FALSE;
        }
    }

    NF_nFCE_H();                        // Select the flash chip.

    return bRet;
}

DWORD FMD_SB_GetBlockStatus(BLOCK_ID blockID)
{
    SECTOR_ADDR sectorAddr = blockID << SB_NAND_LOG_2_PAGES_PER_BLOCK;
    SectorInfo SI;
    DWORD dwResult = 0;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("FMD_SB_GetBlockStatus (0x%x)0x%x \r\n"), blockID, sectorAddr));

    if(!FMD_SB_ReadSector(sectorAddr, NULL, &SI, 1))
    {
        return BLOCK_STATUS_UNKNOWN;
    }

    if(!(SI.bOEMReserved & OEM_BLOCK_READONLY))
    {
        dwResult |= BLOCK_STATUS_READONLY;
    }

    if(SI.bBadBlock != 0xFF)
    {
        dwResult |= BLOCK_STATUS_BAD;
    }

    return dwResult;
}

BOOL FMD_SB_SetBlockStatus(BLOCK_ID blockID, DWORD dwStatus)
{
    SECTOR_ADDR sectorAddr = blockID << SB_NAND_LOG_2_PAGES_PER_BLOCK;
    BYTE bStatus = 0;

    RETAILMSG(FMD_ZONE_FUNCTION,(TEXT("#### FMD_DRIVER:::FMD_sbsetblock \r\n")));

    if(dwStatus & BLOCK_STATUS_BAD)
    {
        if(!NAND_SB_MarkBlockBad (blockID))
        {
            return FALSE;
        }
    }

    // We don't currently support setting a block to read-only, so fail if request is
    // for read-only and block is not currently read-only.
    if(dwStatus & BLOCK_STATUS_READONLY)
    {
        if(!(FMD_SB_GetBlockStatus(blockID) & BLOCK_STATUS_READONLY))
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL NAND_SB_MarkBlockBad(BLOCK_ID blockID)
{
    DWORD   dwStartPage = blockID << SB_NAND_LOG_2_PAGES_PER_BLOCK;
    BOOL    bRet = TRUE;

    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("NAND_SB_MarkBlockBad 0x%x \r\n"), dwStartPage));

    //  Enable chip
    NF_nFCE_L();
    NF_CLEAR_RB();

    //  Issue command
    //  We are dealing with spare area
    NF_CMD(CMD_READ2);
    NF_CMD(CMD_WRITE);

    //  Set up address
    NF_ADDR(SB_POS_BADBLOCK);
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
        RETAILMSG(FMD_ZONE_ERROR, (TEXT("NAND_LB_WriteSectorInfo() ######## Error Programming page (Illigar Access) %d!\n")));
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

BOOL NAND_SB_IsBlockBad(BLOCK_ID blockID)
{
    DWORD   dwPageID = blockID << SB_NAND_LOG_2_PAGES_PER_BLOCK;
    BOOL    bRet = FALSE;
    BYTE    bFlag;

    //  Enable the chip
    NF_nFCE_L();
    NF_CLEAR_RB();
    //  Issue the command
    NF_CMD(CMD_READ2);

    //  Set up address
    NF_ADDR(SB_POS_BADBLOCK);
    NF_ADDR((dwPageID) & 0xff);
    NF_ADDR((dwPageID >> 8) & 0xff);
    if(g_bNeedExtAddr)
    {
        NF_ADDR((dwPageID >> 16) & 0xff);
    }

    //  Wait for Ready bit
    NF_DETECT_RB();     // Wait tR(max 12us)

    //  Now get the byte we want
    bFlag = (BYTE) NF_RDDATA_BYTE();

    if(bFlag != 0xff)
    {
        RETAILMSG(FMD_ZONE_STATUS, (TEXT("FMDSB: IsBlockBad - Page #: 0x%x \r\n"), dwPageID));
        bRet = TRUE;
    }

    //  Disable the chip
    NF_nFCE_H();

    return bRet;
}

#ifdef _IROMBOOT_
BOOL FMD_SB_WriteSector_Steploader(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors)
{
    DWORD   i;
    int NewSpareAddr = 0x0;
    int NewDataAddr = 0;
    int NewSectorAddr = startSectorAddr;
    MECC8 t8MECC;

    ULONG blockPage = (((NewSectorAddr / PAGES_PER_BLOCK) * PAGES_PER_BLOCK) | (NewSectorAddr % PAGES_PER_BLOCK));
    
    RETAILMSG(FMD_ZONE_FUNCTION, (TEXT("FMD::FMD_SB_WriteSector_Steploader 0x%x \r\n"), startSectorAddr));
    
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

    NF_CMD(CMD_READ);                             // Send read command.

    //  Issue command
    NF_CMD(CMD_WRITE);    //0x80

    //  Setup address to write Main data
    NF_ADDR(0x0);    // 1bytes for column address
    NF_ADDR((blockPage)&0xff);    // 3bytes for row address
    NF_ADDR((blockPage>>8)&0xff);
    NF_ADDR((blockPage>>16)&0xff);    
    //NF_ADDR((NewSectorAddr)&0xff);    // 3bytes for row address
    //NF_ADDR((NewSectorAddr>>8)&0xff);
    //NF_ADDR((NewSectorAddr>>16)&0xff);

    // initialize variable.

    t8MECC.n8MECC0 = 0x0;
    t8MECC.n8MECC1 = 0x0;
    t8MECC.n8MECC2 = 0x0;
    t8MECC.n8MECC3 = 0x0;


    // Write each Sector in the Page.
    //  Initialize ECC register
    NF_MECC_UnLock();
    NF_RSTECC();

    // Special case to handle un-aligned buffer pointer.
    if( ((DWORD) pSectorBuff) & 0x3) 
    {
        //  Write the data
        WrPage512Unalign(pSectorBuff);
    }
    else
    {
        WrPage512(pSectorBuff);
    }

    NF_MECC_Lock();

    while(!(g_pNFConReg->NFSTAT&(1<<7))) ;
    g_pNFConReg->NFSTAT|=(1<<7);

    //  Read out the ECC value generated by HW
    t8MECC.n8MECC0 = NF_8MECC0();
    t8MECC.n8MECC1 = NF_8MECC1();
    t8MECC.n8MECC2 = NF_8MECC2();
    t8MECC.n8MECC3 = (NF_8MECC3() & 0xff);

    NF_WRDATA_WORD(t8MECC.n8MECC0); // 4 byte n8MECC0
    NF_WRDATA_WORD(t8MECC.n8MECC1); // 4 byte n8MECC1
    NF_WRDATA_WORD(t8MECC.n8MECC2); // 4 byte n8MECC2
    NF_WRDATA_BYTE((t8MECC.n8MECC3) & 0xff); // 1 byte n8MECC3

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

#endif  // !_IROMBOOT_

BOOL RAW_SB_ReadSector(UINT32 startSectorAddr, LPBYTE pSectorBuff, LPBYTE pSectorInfoBuff)
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
    NF_ADDR((nPageAddr)&0xff);
    NF_ADDR((nPageAddr>>8)&0xff);
    if (g_bNeedExtAddr)
    {
        NF_ADDR((nPageAddr>>16)&0xff);  
    }

    NF_CMD(CMD_READ3);    // 2nd command
    NF_DETECT_RB();                                // Wait for command to complete.

    for (nSectorLoop = 0; nSectorLoop < 1; nSectorLoop++)
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

    for( i = 0; i < 16; i++)
    {
        *pSectorInfoBuff = NF_RDDATA_BYTE();        // read and trash the data
        pSectorInfoBuff+=1;
    }

    NF_nFCE_H();

    return(TRUE);
}
