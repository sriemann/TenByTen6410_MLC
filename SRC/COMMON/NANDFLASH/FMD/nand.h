//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
#ifndef __NAND_H__
#define __NAND_H__

//-----------------------------------------------------------------------------

#ifdef _IROMBOOT_
#include <fmd.h>
#endif

typedef struct
{
    UINT16 nNumOfBlks;
    UINT16 nPagesPerBlk;
    UINT16 nSctsPerPage;
} NANDDeviceInfo;

#ifdef __cplusplus
extern "C"  {
#endif
extern NANDDeviceInfo stDeviceInfo;
extern NANDDeviceInfo GetNandInfo(void);
#ifdef _IROMBOOT_
BOOL FMD_WriteSector_Stepldr(SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors);
#endif

#ifdef __cplusplus
}
#endif

#define NUM_OF_BLOCKS       (stDeviceInfo.nNumOfBlks)
#define PAGES_PER_BLOCK     (stDeviceInfo.nPagesPerBlk)
#define SECTORS_PER_PAGE    (stDeviceInfo.nSctsPerPage)

// Match to MDD
#undef SECTOR_SIZE
#define SECTOR_SIZE         (512)       //< Logical legacy Sector size, compatible to HDD
#define NAND_PAGE_SIZE      (SECTOR_SIZE*SECTORS_PER_PAGE)  //< Physical Page Size

#define IS_LB               (SECTORS_PER_PAGE == 4)

//-----------------------------------------------------------------------------
#define TACLS               (NAND_TACLS)
#define TWRPH0              (NAND_TWRPH0)
#define TWRPH1              (NAND_TWRPH1)
//-----------------------------------------------------------------------------

#define CMD_READID          (0x90)    // ReadID
#define CMD_READ            (0x00)    // Read
#define CMD_READ2           (0x50)    // Read2
#define CMD_READ3           (0x30)    // Read3
#define CMD_RESET           (0xff)    // Reset
#define CMD_ERASE           (0x60)    // Erase phase 1
#define CMD_ERASE2          (0xd0)    // Erase phase 2
#define CMD_WRITE           (0x80)    // Write phase 1
#define CMD_WRITE2          (0x10)    // Write phase 2
#define CMD_STATUS          (0x70)    // STATUS
#define CMD_RDI             (0x85)    // Random Data Input
#define CMD_RDO             (0x05)    // Random Data Output
#define CMD_RDO2            (0xE0)    // Random Data Output

#define BADBLOCKMARK        (0x00)

//  Status bit pattern
#define STATUS_READY        (0x40)    // Ready
#define STATUS_ERROR        (0x01)    // Error
#define STATUS_ILLACC       (0x20)    // Illegal Access

//-----------------------------------------------------------------------------

#define NF_CMD(cmd)         {g_pNFConReg->NFCMD   =  (unsigned char)(cmd);}
#define NF_ADDR(addr)       {g_pNFConReg->NFADDR  =  (unsigned char)(addr);}

#define NF_nFCE_L()         {g_pNFConReg->NFCONT &= ~(1<<1);}
#define NF_nFCE_H()         {g_pNFConReg->NFCONT |=  (1<<1);}

#define NF_RSTECC()         {g_pNFConReg->NFCONT |=  ((1<<5) | (1<<4));}

#define NF_MECC_UnLock()    {g_pNFConReg->NFCONT &= ~(1<<7);}
#define NF_MECC_Lock()      {g_pNFConReg->NFCONT |= (1<<7);}
#define NF_SECC_UnLock()    {g_pNFConReg->NFCONT &= ~(1<<6);}
#define NF_SECC_Lock()      {g_pNFConReg->NFCONT |= (1<<6);}

#define NF_CLEAR_RB()       {g_pNFConReg->NFSTAT |=  (1<<4);}                // Have write '1' to clear this bit.

#define NF_DETECT_RB()      {while((g_pNFConReg->NFSTAT&0x11)!=0x11);}        // RnB_Transdetect & RnB
#define NF_WAITRB()         {while (!(g_pNFConReg->NFSTAT & (1<<0))) ; }

#define NF_RDDATA_BYTE()    (g_pNFConReg->NFDATA)
#define NF_RDDATA_WORD()    (*(volatile UINT32 *)(&(g_pNFConReg->NFDATA)))

#define NF_WRDATA_BYTE(data)    {g_pNFConReg->NFDATA = (UINT8)(data);}
#define NF_WRDATA_WORD(data)    {*(volatile UINT32 *)(&(g_pNFConReg->NFDATA)) = (UINT32)(data);}

#define NF_RDMECC0()            (g_pNFConReg->NFMECC0)
#define NF_RDMECC1()            (g_pNFConReg->NFMECC1)
#define NF_RDSECC()             (g_pNFConReg->NFSECC)

#define NF_RDMECCD0()           (g_pNFConReg->NFMECCD0)
#define NF_RDMECCD1()           (g_pNFConReg->NFMECCD1)
#define NF_RDSECCD()            (g_pNFConReg->NFSECCD)

#define NF_ECC_ERR0             (g_pNFConReg->NFECCERR0)
#define NF_ECC_ERR1             (g_pNFConReg->NFECCERR1)

#define NF_WRMECCD0(data)       {g_pNFConReg->NFMECCD0 = (data);}
#define NF_WRMECCD1(data)       {g_pNFConReg->NFMECCD1 = (data);}
#define NF_WRSECCD(data)        {g_pNFConReg->NFSECCD = (data);}

#define NF_8MECC0()             (g_pNFConReg->NFM8ECC0)
#define NF_8MECC1()             (g_pNFConReg->NFM8ECC1)
#define NF_8MECC2()             (g_pNFConReg->NFM8ECC2)
#define NF_8MECC3()             (g_pNFConReg->NFM8ECC3)

#define NF_RDSTAT               (g_pNFConReg->NFSTAT)

//-----------------------------------------------------------------------------

typedef enum
{
    ECC_CORRECT_MAIN = 0,       // correct Main ECC using Main ECC Result Area
    ECC_CORRECT_SPARE1 = 1,     // correct Spare for Sector Info using Main ECC Result Area
    ECC_CORRECT_SPARE2 = 2,     // correct Spare for MECC using Main ECC Result Area
    ECC_CORRECT_SPARE = 3       // correct Spare using Spare ECC Result Area
} ECC_CORRECT_TYPE;

//-----------------------------------------------------------------------------

#endif    // __NAND_H_.
