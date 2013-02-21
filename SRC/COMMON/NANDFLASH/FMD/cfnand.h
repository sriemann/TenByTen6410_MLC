/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2001  Microsoft Corporation

Module Name:    S3C6410.H

Abstract:        FLASH Media Driver Interface Samsung S3C6410 CPU with NAND Flash
                controller.

Environment:    As noted, this media driver works on behalf of the FAL to directly
                access the underlying FLASH hardware.  Consquently, this module
                needs to be linked with FLASHFAL.LIB to produce the device driver
                named FLASHDRV.DLL.

-----------------------------------------------------------------------------*/
#ifndef _S3C6410_CFNAND_H
#define _S3C6410_CFNAND_H

#include "FMD_LB.h"
#include "FMD_SB.h"
#include "nand.h"

#define BW_X08    (0)
#define BW_X16    (1)
#define BW_X32    (2)

#define NAND_MID_SAMSUNG  (0xEC)
#define NAND_MID_TOSHIBA  (0x98)
#define NAND_MID_FUJITSU  (0x04)
#define NAND_MID_NATIONAL (0x8F)
#define NAND_MID_RENESAS  (0x07)
#define NAND_MID_STMICRO  (0x20)
#define NAND_MID_HYNIX    (0xAD)

#define MAX_SECTORS_PER_PAGE    (8)


/*****************************************************************************/
/* S3C6410 Nand Flash Internal Data Structure Definition                                    */
/*****************************************************************************/
typedef struct
{
    UINT16 nMID;            /* Manufacturer ID               */
    UINT16 nDID;                /* Device ID                     */

    UINT16 nNumOfBlks;        /* Number of Blocks              */
    UINT16 nPgsPerBlk;        /* Number of Pages per block     */
    UINT16 nSctsPerPg;        /* Number of Sectors per page    */
    UINT16 nNumOfPlanes;    /* Number of Planes              */
    UINT16 nBlksInRsv;        /* The Number of Blocks in Reservior for Bad Blocks   */
    UINT8 nBadPos;            /* BadBlock Information Poisition*/
    UINT8 nLsnPos;            /* LSN Position                  */
    UINT8 nECCPos;            /* ECC Policy : HW_ECC, SW_ECC   */
    UINT16 nBWidth;            /* Nand Organization X8 or X16   */

    UINT16 nTrTime;            /* Typical Read Op Time          */
    UINT16 nTwTime;            /* Typical Write Op Time         */
    UINT16 nTeTime;            /* Typical Erase Op Time         */
    UINT16 nTfTime;            /* Typical Transfer Op Time      */
} FlashDevSpec;

static FlashDevSpec astNandSpec[] = {
    /*************************************************************************/
    /* nMID, nDID,                                                           */
    /*            nNumOfBlks                                                 */
    /*                  nPgsPerBlk                                           */
    /*                      nSctsPerPg                                       */
    /*                         nNumOfPlanes                                  */
    /*                            nBlksInRsv                                 */
    /*                                nBadPos                                */
    /*                                   nLsnPos                             */
    /*                                      nECCPos                          */
    /*                                         nBWidth                       */
    /*                                                nTrTime                */
    /*                                                    nTwTime            */
    /*                                                         nTeTime       */
    /*                                                                nTfTime*/
    /*************************************************************************/
    /* 8Gbit DDP NAND Flash */
    { NAND_MID_SAMSUNG, 0xD3, 8192, 64, 4, 2,160, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    /* 4Gbit DDP NAND Flash */
    { NAND_MID_SAMSUNG, 0xAC, 4096, 64, 4, 2, 80, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_SAMSUNG, 0xDC, 4096, 64, 4, 2, 80, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0xBC, 4096, 64, 4, 2, 80, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0xCC, 4096, 64, 4, 2, 80, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    /* 2Gbit NAND Flash */
    { NAND_MID_SAMSUNG, 0xAA, 2048, 64, 4, 1, 40, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_SAMSUNG, 0xDA, 2048, 64, 4, 1, 40, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0xBA, 2048, 64, 4, 1, 40, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0xCA, 2048, 64, 4, 1, 40, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    /* 2Gbit DDP NAND Flash */
    //{ NAND_MID_SAMSUNG, 0xDA, 2048, 64, 4, 2, 40, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0xAA, 2048, 64, 4, 2, 40, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    //{ 0xEC, 0xBA, 2048, 64, 4, 2, 40, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    //{ 0xEC, 0xCA, 2048, 64, 4, 2, 40, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    /*1Gbit NAND Flash */
    { NAND_MID_SAMSUNG, 0xA1, 1024, 64, 4, 1, 20, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_SAMSUNG, 0xF1, 1024, 64, 4, 1, 20, 0, 2, 8, BW_X08, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0xB1, 1024, 64, 4, 1, 20, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0xC1, 1024, 64, 4, 1, 20, 0, 2, 8, BW_X16, 50, 350, 2000, 50},
    /* 1Gbit NAND Flash */
    { NAND_MID_SAMSUNG, 0x79, 8192, 32, 1, 4,120, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_SAMSUNG, 0x78, 8192, 32, 1, 4,120, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0x74, 8192, 32, 1, 4,120,11, 0, 6, BW_X16, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0x72, 8192, 32, 1, 4,120,11, 0, 6, BW_X16, 50, 350, 2000, 50},
    /* 512Mbit NAND Flash */
    { NAND_MID_SAMSUNG, 0x76, 4096, 32, 1, 4, 70, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_SAMSUNG, 0x36, 4096, 32, 1, 4, 70, 5, 0, 6, BW_X08, 50, 350, 2000, 50},

    /* 512Mbit XP Card */
    { NAND_MID_TOSHIBA, 0x76, 4096, 32, 1, 4, 70, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_TOSHIBA, 0x79, 4096, 32, 1, 4, 70, 5, 0, 6, BW_X08, 50, 350, 2000, 50},

    //{ NAND_MID_SAMSUNG, 0x56, 4096, 32, 1, 4, 70,11, 0, 6, BW_X16, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0x46, 4096, 32, 1, 4, 70,11, 0, 6, BW_X16, 50, 350, 2000, 50},
    /* 256Mbit NAND Flash */
    { NAND_MID_SAMSUNG, 0x75, 2048, 32, 1, 1, 35, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_SAMSUNG, 0x35, 2048, 32, 1, 1, 35, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0x55, 2048, 32, 1, 1, 35,11, 0, 6, BW_X16, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0x45, 2048, 32, 1, 1, 35,11, 0, 6, BW_X16, 50, 350, 2000, 50},
    /* 128Mbit NAND Flash */
    { NAND_MID_SAMSUNG, 0x73, 1024, 32, 1, 1, 20, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    { NAND_MID_SAMSUNG, 0x33, 1024, 32, 1, 1, 20, 5, 0, 6, BW_X08, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0x53, 1024, 32, 1, 1, 20,11, 0, 6, BW_X16, 50, 350, 2000, 50},
    //{ NAND_MID_SAMSUNG, 0x43, 1024, 32, 1, 1, 20,11, 0, 6, BW_X16, 50, 350, 2000, 50},

    { 0x00, 0x00,    0,  0, 0, 0,  0, 0, 0, 0,      0,  0,   0,    0,  0}
};

#endif _S3C6410_CFNAND_H

