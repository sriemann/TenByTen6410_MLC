//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

extern "C"
{
#include <windows.h>
#include <bsp.h>
#include "loader.h"
#include <fmd.h>

#include <VFLBuffer.h>
#include <WMRTypes.h>
#include <VFL.h>
#include <FIL.h>



extern DWORD        g_ImageType;
extern UCHAR        g_TOC[SECTOR_SIZE];
extern const PTOC   g_pTOC;
extern DWORD        g_dwTocEntry;
extern PBOOT_CFG    g_pBootCfg;
extern BOOL         g_bBootMediaExist;
extern MultiBINInfo g_BINRegionInfo;
extern DWORD        g_dwImageStartBlock;
extern BOOL         g_bWaitForConnect;      // Is there a SmartMedia card on this device?
extern char*        BootDeviceString[];
extern DWORD        g_DefaultBootDevice;
//----------------------------------------------

//-----------------------------------------------------------
extern DWORD             g_DefaultLcdDevice;
//----------------------------------------------

// MLC low level test function
// by dodan2 061102
extern UINT32 MLC_Read_RAW(UINT32 nBank, UINT32 nPpn, UINT8 *pMBuf, UINT8 *pSBuf);
extern UINT32 MLC_Write_RAW(UINT32 nBank, UINT32 nPpn, UINT8 *pMBuf, UINT8 *pSBuf);
extern UINT32 MLC_Erase_RAW(UINT32 nBank, UINT32 nBlock);

extern void _Read_512Byte(UINT8* pBuf);
extern void _Read_512Byte_Unaligned(UINT8* pBuf);
extern void _Write_512Byte(UINT8* pBuf);
extern void _Write_512Byte_Unaligned(UINT8* pBuf);
}

BOOL WriteBlock(DWORD dwBlock, LPBYTE pbBlock, PSectorInfo pSectorInfoTable);
BOOL ReadBlock(DWORD dwBlock, LPBYTE pbBlock, PSectorInfo pSectorInfoTable);

#define         WMRBUFSIZE                        (8192 + 256)  // the maximum size of 2-plane data for 4KByte/Page NAND flash Device

extern DWORD        g_dwLastWrittenLoc;                     //  Defined in bootpart.lib
extern PSectorInfo  g_pSectorInfoBuf;
extern FlashInfo    g_FlashInfo;
static UCHAR         toc[SECTOR_SIZE];

extern BOOL IsValidMBRSector();
extern BOOL CreateMBR();
extern UCHAR WMRBuf[];

void MLC_Print_Page_Data(unsigned char *pMBuf, unsigned char *pSBuf);

// Define a dummy SetKMode function to satisfy the NAND FMD.
//
DWORD SetKMode (DWORD fMode)
{
    return(1);
}


void BootConfigPrint(void)
{
    EdbgOutputDebugString( "BootCfg { \r\n");
    EdbgOutputDebugString( "  ConfigFlags: 0x%x\r\n", g_pBootCfg->ConfigFlags);
    EdbgOutputDebugString( "  BootDelay: 0x%x\r\n", g_pBootCfg->BootDelay);
    EdbgOutputDebugString( "  ImageIndex: %d \r\n", g_pBootCfg->ImageIndex);
    EdbgOutputDebugString ("  Boot Device: %s\r\n", BootDeviceString[g_pBootCfg->BootDevice]);
    EdbgOutputDebugString( "  IP: %s\r\n", inet_ntoa(g_pBootCfg->EdbgAddr.dwIP));
    EdbgOutputDebugString( "  MAC Address: %B:%B:%B:%B:%B:%B\r\n",
                           g_pBootCfg->EdbgAddr.wMAC[0] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[0] >> 8,
                           g_pBootCfg->EdbgAddr.wMAC[1] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[1] >> 8,
                           g_pBootCfg->EdbgAddr.wMAC[2] & 0x00FF, g_pBootCfg->EdbgAddr.wMAC[2] >> 8);
    EdbgOutputDebugString( "  Port: %s\r\n", inet_ntoa(g_pBootCfg->EdbgAddr.wPort));

    EdbgOutputDebugString( "  SubnetMask: %s\r\n", inet_ntoa(g_pBootCfg->SubnetMask));
    EdbgOutputDebugString( "}\r\n");
}


// Set default boot configuration values
static void BootConfigInit(DWORD dwIndex)
{

    EdbgOutputDebugString("+BootConfigInit\r\n");

    g_pBootCfg = &g_pTOC->BootCfg;

    memset(g_pBootCfg, 0, sizeof(BOOT_CFG));

    g_pBootCfg->ImageIndex   = dwIndex;

    g_pBootCfg->BootDevice = g_DefaultBootDevice;	  
	//----------------------------------------------
    g_pBootCfg->powerCTL = pBSPArgs->powerCTL= PWRCTL_HITEG_FACTORY_DEFAULTS;		// Hiteg Ltd. default config;
    g_pBootCfg->displayType = pBSPArgs->displayType= SMDK6410_LCD_MODULE;				// undefined value results to attached display==NONE	
    g_pBootCfg->framebufferDepth =pBSPArgs->framebufferDepth= LCD_BPP;						// we set 16bpp as default, can be changed to 24/32 bit
    g_pBootCfg->backgroundColor = pBSPArgs->backgroundColor=DEFAULT_FRAMEBUFFER_COLOR;	// a nice classy grey ... 
	g_pBootCfg->logoHW=pBSPArgs->logoHW=DEFAULT_LOGO_POSITION;					// if set to ~0 no logo uploaded...		
	g_pBootCfg->debugUART=pBSPArgs->debugUART=0;
	//----------------------------------------------------

    g_pBootCfg->ConfigFlags  = BOOT_TYPE_MULTISTAGE|BOOT_OPTION_CLEAN;

    g_pBootCfg->BootDelay    = CONFIG_BOOTDELAY_DEFAULT;

    // Setup some defaults so that KITL will work without having to change anything
    g_pBootCfg->ConfigFlags |= CONFIG_FLAGS_KITL | CONFIG_FLAGS_DHCP;
    g_pBootCfg->EdbgAddr.dwIP = inet_addr("192.198.1.101");
    g_pBootCfg->SubnetMask = inet_addr("255.255.255.0");
    g_pBootCfg->EdbgAddr.wMAC[0] = 0x1100;
    g_pBootCfg->EdbgAddr.wMAC[1] = 0x3322;
    g_pBootCfg->EdbgAddr.wMAC[2] = 0x5544;

    EdbgOutputDebugString("-BootConfigInit\r\n");
    return;
}

void ID_Print(DWORD i) {
    DWORD j;
    EdbgOutputDebugString("ID[%u] {\r\n", i);
    EdbgOutputDebugString("  dwVersion: 0x%x\r\n",  g_pTOC->id[i].dwVersion);
    EdbgOutputDebugString("  dwSignature: 0x%x\r\n", g_pTOC->id[i].dwSignature);
    EdbgOutputDebugString("  String: '%s'\r\n", g_pTOC->id[i].ucString);
    EdbgOutputDebugString("  dwImageType: 0x%x\r\n", g_pTOC->id[i].dwImageType);
    EdbgOutputDebugString("  dwTtlSectors: 0x%x\r\n", g_pTOC->id[i].dwTtlSectors);
    EdbgOutputDebugString("  dwLoadAddress: 0x%x\r\n", g_pTOC->id[i].dwLoadAddress);
    EdbgOutputDebugString("  dwJumpAddress: 0x%x\r\n", g_pTOC->id[i].dwJumpAddress);
    EdbgOutputDebugString("  dwStoreOffset: 0x%x\r\n", g_pTOC->id[i].dwStoreOffset);
    for (j = 0; j < MAX_SG_SECTORS; j++) {
        if ( !g_pTOC->id[i].sgList[j].dwLength )
            break;
        EdbgOutputDebugString("  sgList[%u].dwSector: 0x%x\r\n", j, g_pTOC->id[i].sgList[j].dwSector);
        EdbgOutputDebugString("  sgList[%u].dwLength: 0x%x\r\n", j, g_pTOC->id[i].sgList[j].dwLength);
    }

    EdbgOutputDebugString("}\r\n");
}

void TOC_Print(void)
{
    int i;

    EdbgOutputDebugString("g_dwTocEntry : %d\r\n", g_dwTocEntry);
    EdbgOutputDebugString("TOC {\r\n");
    EdbgOutputDebugString("dwSignature: 0x%x\r\n", g_pTOC->dwSignature);


    BootConfigPrint( );

    for (i = 0; i < MAX_TOC_DESCRIPTORS; i++) {
        if ( !VALID_IMAGE_DESCRIPTOR(&g_pTOC->id[i]) )
        {
            EdbgOutputDebugString("!!!Invalid Image Descriptor: id[%d]=0x%x\r\n", i, g_pTOC->id[i]);
            break;
        }
        ID_Print(i);
    }

    //  Print out Chain Information
    EdbgOutputDebugString("chainInfo.dwLoadAddress: 0X%X\r\n", g_pTOC->chainInfo.dwLoadAddress);
    EdbgOutputDebugString("chainInfo.dwFlashAddress: 0X%X\r\n", g_pTOC->chainInfo.dwFlashAddress);
    EdbgOutputDebugString("chainInfo.dwLength: 0X%X\r\n", g_pTOC->chainInfo.dwLength);

    EdbgOutputDebugString("}\r\n");
}


// init the EBOOT TOC to defaults
BOOL TOC_Init(DWORD dwEntry, DWORD dwImageType, DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr)
{
    DWORD dwSig = 0;

    EdbgOutputDebugString("TOC_Init: dwEntry:%u, dwImageType: 0x%x, dwImageStart: 0x%x, dwImageLength: 0x%x, dwLaunchAddr: 0x%x\r\n",
        dwEntry, dwImageType, dwImageStart, dwImageLength, dwLaunchAddr);

    if (0 == dwEntry) {
        EdbgOutputDebugString("\r\n*** WARNING: TOC_Init blasting Eboot ***\r\n");
        return FALSE;
    }

    switch (dwImageType) {
        case IMAGE_TYPE_LOADER:
            dwSig = IMAGE_EBOOT_SIG;
            break;
        case IMAGE_TYPE_RAMIMAGE:
            dwSig = IMAGE_RAM_SIG;
            break;
        default:
            EdbgOutputDebugString("ERROR: OEMLaunch: unknown image type: 0x%x \r\n", dwImageType);
            return FALSE;
    }

    memset(g_pTOC, 0, sizeof(g_TOC));

    // init boof cfg
    BootConfigInit(dwEntry);

    // update our index
    g_dwTocEntry = dwEntry;

    // debugger enabled?
    g_bWaitForConnect = (g_pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL) ? TRUE : FALSE;

    // init TOC...
    //
    g_pTOC->dwSignature = TOC_SIGNATURE;

    //  init TOC entry for Eboot
    //  Those are hard coded numbers from boot.bib
    g_pTOC->id[0].dwVersion     = (EBOOT_VERSION_MAJOR << 16) | EBOOT_VERSION_MINOR;
    g_pTOC->id[0].dwSignature   = IMAGE_EBOOT_SIG;
    memcpy(g_pTOC->id[0].ucString, "eboot.nb0", sizeof("eboot.nb0")+1);   //  NUll terminate
    g_pTOC->id[0].dwImageType   = IMAGE_TYPE_RAMIMAGE;
    g_pTOC->id[0].dwLoadAddress = EBOOT_RAM_IMAGE_BASE;
    g_pTOC->id[0].dwJumpAddress = EBOOT_RAM_IMAGE_BASE;
    g_pTOC->id[0].dwTtlSectors  = FILE_TO_SECTOR_SIZE(EBOOT_RAM_IMAGE_SIZE);
    // 1 contigious segment
    g_pTOC->id[0].sgList[0].dwSector = BLOCK_TO_SECTOR(EBOOT_BLOCK);
    g_pTOC->id[0].sgList[0].dwLength = g_pTOC->id[0].dwTtlSectors;

    // init the TOC entry
    g_pTOC->id[dwEntry].dwVersion     = 0x001;
    g_pTOC->id[dwEntry].dwSignature   = dwSig;
    memset(g_pTOC->id[dwEntry].ucString, 0, IMAGE_STRING_LEN);
    g_pTOC->id[dwEntry].dwImageType   = dwImageType;
    g_pTOC->id[dwEntry].dwLoadAddress = dwImageStart;
    g_pTOC->id[dwEntry].dwJumpAddress = dwLaunchAddr;
    g_pTOC->id[dwEntry].dwStoreOffset = 0;
    g_pTOC->id[dwEntry].dwTtlSectors  = FILE_TO_SECTOR_SIZE(dwImageLength);
    // 1 contigious segment
    g_pTOC->id[dwEntry].sgList[0].dwSector = BLOCK_TO_SECTOR(IMAGE_START_BLOCK);
    g_pTOC->id[dwEntry].sgList[0].dwLength = g_pTOC->id[dwEntry].dwTtlSectors;

    TOC_Print();

    return TRUE;
}

//
// Retrieve EBOOT TOC from Nand.
//
BOOL TOC_Read(void)
{
    LowFuncTbl *pLowFuncTbl;
    DWORD dwBlock;
    INT32 nRet;
    UINT8 *pSBuf;

    EdbgOutputDebugString("TOC_Read\r\n");

    if ( !g_bBootMediaExist )
    {
        EdbgOutputDebugString("TOC_Read ERROR: no boot media\r\n");
        return FALSE;
    }

    pLowFuncTbl = FIL_GetFuncTbl();

    pSBuf = WMRBuf + BYTES_PER_MAIN_SUPAGE;

    dwBlock = TOC_BLOCK;

    while(1)
    {
        if (dwBlock == (TOC_BLOCK+TOC_BLOCK_SIZE+TOC_BLOCK_RESERVED))
        {
            OALMSG(TRUE, (TEXT("TOC_Read Failed !!!\r\n")));
            OALMSG(TRUE, (TEXT("Too many Bad Block\r\n")));
            return FALSE;
        }

        IS_CHECK_SPARE_ECC = FALSE32;
        pLowFuncTbl->Read(0, dwBlock*PAGES_PER_BLOCK+PAGES_PER_BLOCK-1, 0x0, enuLEFT_PLANE_BITMAP, NULL, pSBuf, TRUE32, FALSE32);
        IS_CHECK_SPARE_ECC = TRUE32;
        if (pSBuf[0] == 0xff)
        {
            nRet = pLowFuncTbl->Read(0, dwBlock*PAGES_PER_BLOCK, 0x01, enuLEFT_PLANE_BITMAP, (UINT8 *)g_pTOC, NULL, FALSE32, FALSE32);
            if (nRet != FIL_SUCCESS)
            {
                OALMSG(TRUE, (TEXT("[ERR] FIL Read Error @ %d Block %d Page, Skipped\r\n"), dwBlock, 0));
                dwBlock++;
                continue;
            }
            else
            {
                break;
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT("Bad Block %d Skipped\r\n"), dwBlock));
            dwBlock++;
            continue;
        }
    }

    // is it a valid TOC?
    if ( !VALID_TOC(g_pTOC) )
    {
        EdbgOutputDebugString("TOC_Read ERROR: INVALID_TOC Signature: 0x%x\r\n", g_pTOC->dwSignature);
        return FALSE;
    }

    // invalidate TOC if Eboot version has been updated
    if (g_pTOC->id[0].dwVersion != ((EBOOT_VERSION_MAJOR << 16) | EBOOT_VERSION_MINOR))    
    {
        EdbgOutputDebugString("TOC_Read WARN: Eboot version mismatch: TOC->dwVersion: 0x%x , Expected: 0x%x\r\n", 
            g_pTOC->id[0].dwVersion, ((EBOOT_VERSION_MAJOR << 16) | EBOOT_VERSION_MINOR));
        EdbgOutputDebugString("Invalidating TOC for re-initializing.\r\n");
        return FALSE;
    }

    // update our boot config
    g_pBootCfg = &g_pTOC->BootCfg;

    // update our index
    g_dwTocEntry = g_pBootCfg->ImageIndex;

    // KITL enabled?
    g_bWaitForConnect = (g_pBootCfg->ConfigFlags & CONFIG_FLAGS_KITL) ? TRUE : FALSE;

    // cache image type
    g_ImageType = g_pTOC->id[g_dwTocEntry].dwImageType;

    EdbgOutputDebugString("-TOC_Read\r\n");

    return TRUE;

}
//
// Store EBOOT TOC to Nand
// BUGBUG: only uses 1 sector for now.
//
BOOL TOC_Write(void)
{
    LowFuncTbl *pLowFuncTbl;
    DWORD dwBlock;
    INT32 nRet;
    UINT8 *pSBuf;
    UINT32 nSyncRet;

    EdbgOutputDebugString("+TOC_Write\r\n");

    if ( !g_bBootMediaExist )
    {
        EdbgOutputDebugString("TOC_Write WARN: no boot media\r\n");
        return FALSE;
    }

    // is it a valid TOC?
    if ( !VALID_TOC(g_pTOC))
    {
        EdbgOutputDebugString("TOC_Write ERROR: INVALID_TOC Signature: 0x%x\r\n", g_pTOC->dwSignature);
        return FALSE;
    }

    // is it a valid image descriptor?
    if ( !VALID_IMAGE_DESCRIPTOR(&g_pTOC->id[g_dwTocEntry]) )
    {
        EdbgOutputDebugString("TOC_Write ERROR: INVALID_IMAGE[%u] Signature: 0x%x\r\n",
            g_dwTocEntry, g_pTOC->id[g_dwTocEntry].dwSignature);
        return FALSE;
    }

    pLowFuncTbl = FIL_GetFuncTbl();

    memset(WMRBuf, '\0', WMRBUFSIZE);

    pSBuf = WMRBuf + BYTES_PER_MAIN_SUPAGE;

    dwBlock = TOC_BLOCK;

    while(1)
    {
        if (dwBlock == (TOC_BLOCK+TOC_BLOCK_SIZE+TOC_BLOCK_RESERVED))
        {
            OALMSG(TRUE, (TEXT("TOC_Write Failed !!!\r\n")));
            OALMSG(TRUE, (TEXT("Too many Bad Block\r\n")));
            return FALSE;
        }

        IS_CHECK_SPARE_ECC = FALSE32;
        pLowFuncTbl->Read(0, dwBlock*PAGES_PER_BLOCK+PAGES_PER_BLOCK-1, 0x0, enuLEFT_PLANE_BITMAP, NULL, pSBuf, TRUE32, FALSE32);
        IS_CHECK_SPARE_ECC = TRUE32;
        if (pSBuf[0] == 0xff)
        {
            pLowFuncTbl->Erase(0, dwBlock, enuLEFT_PLANE_BITMAP);
            nRet = pLowFuncTbl->Sync(0, &nSyncRet);
            if ( nRet != FIL_SUCCESS)
            {
                OALMSG(TRUE, (TEXT("[ERR] FIL Erase Error @ %d block, Skipped\r\n"), dwBlock));
                goto MarkAndSkipBadBlock;
            }

            pLowFuncTbl->Write(0, dwBlock*PAGES_PER_BLOCK, 0x01, enuLEFT_PLANE_BITMAP, (UINT8 *)g_pTOC, NULL);
            nRet = pLowFuncTbl->Sync(0, &nSyncRet);
            if (nRet != FIL_SUCCESS)
            {
                OALMSG(TRUE, (TEXT("[ERR] FIL Write Error @ %d Block %d Page, Skipped\r\n"), dwBlock, 0));
                goto MarkAndSkipBadBlock;
            }

            nRet = pLowFuncTbl->Read(0, dwBlock*PAGES_PER_BLOCK, 0x01, enuLEFT_PLANE_BITMAP, toc, NULL, FALSE32, FALSE32);
            if (nRet != FIL_SUCCESS)
            {
                OALMSG(TRUE, (TEXT("[ERR] FIL Read Error @ %d Block %d Page, Skipped\r\n"), dwBlock, 0));
                goto MarkAndSkipBadBlock;
            }

            if (0 != memcmp(toc, (UINT8 *)g_pTOC, SECTOR_SIZE))
            {
                OALMSG(TRUE, (TEXT("[ERR] Verify Error @ %d Block %d Page, Skipped\r\n"), dwBlock, 0));
                goto MarkAndSkipBadBlock;
            }

            OALMSG(TRUE, (TEXT("[OK] Write %d th Block Success\r\n"), dwBlock));
			
            break;

MarkAndSkipBadBlock:

            pLowFuncTbl->Erase(0, dwBlock, enuLEFT_PLANE_BITMAP);
            memset(pSBuf, 0x0, BYTES_PER_SPARE_PAGE);
            IS_CHECK_SPARE_ECC = FALSE32;
            pLowFuncTbl->Write(0, dwBlock*PAGES_PER_BLOCK+PAGES_PER_BLOCK-1, 0x0, enuLEFT_PLANE_BITMAP, NULL, pSBuf);
            IS_CHECK_SPARE_ECC = TRUE32;
            dwBlock++;
            continue;
        }
        else
        {
            OALMSG(TRUE, (TEXT("Bad Block %d Skipped\r\n"), dwBlock));
            dwBlock++;
            continue;
        }
    }

    EdbgOutputDebugString("-TOC_Write\r\n");
    return TRUE;
}

extern DWORD g_dwMBRSectorNum;

/*
    @func   void | DumpXIPChainSummary | Dump XIPCHAIN_SUMMARY structure
    @rdesc  none
    @comm
    @xref
*/
void DumpXIPChainSummary(PXIPCHAIN_SUMMARY pChainInfo)
{    
    EdbgOutputDebugString("[Dump XIP Chain Summary]\r\n");
    EdbgOutputDebugString(" - pvAddr: 0x%x\r\n", pChainInfo->pvAddr);
    EdbgOutputDebugString(" - dwMaxLength : %d\r\n", pChainInfo->dwMaxLength);
    EdbgOutputDebugString(" - usOrder : 0x%x\r\n", pChainInfo->usOrder);
    EdbgOutputDebugString(" - usFlags : 0x%x\r\n", pChainInfo->usFlags);
    EdbgOutputDebugString(" - reserved : 0x%x\r\n", pChainInfo->reserved);    
}

/*
    @func   BOOL | WriteOSImageToBootMedia | Stores the image cached in RAM to the Boot Media.
    The image may be comprised of one or more BIN regions.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL WriteOSImageToBootMedia(DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr)
{
    BYTE nCount;
    DWORD dwNumExts;
    PXIPCHAIN_SUMMARY pChainInfo = NULL;
    EXTENSION *pExt = NULL;
    DWORD dwBINFSPartLength = 0;
    HANDLE hPart;
    DWORD dwStoreOffset;
    DWORD dwMaxRegionLength[BL_MAX_BIN_REGIONS] = {0};
    DWORD dwChainStart, dwChainLength;
    DWORD dwBlock;
    DWORD nBlockNum;

    //  Initialize the variables
    dwChainStart = dwChainLength = 0;

    OALMSG(OAL_FUNC, (TEXT("+WriteOSImageToBootMedia\r\n")));
    OALMSG(OAL_INFO, (TEXT("+WriteOSImageToBootMedia: g_dwTocEntry =%d, ImageStart: 0x%x, ImageLength: 0x%x, LaunchAddr:0x%x\r\n"),
                            g_dwTocEntry, dwImageStart, dwImageLength, dwLaunchAddr));

    if ( !g_bBootMediaExist )
    {
        OALMSG(OAL_ERROR, (TEXT("ERROR: WriteOSImageToBootMedia: device doesn't exist.\r\n")));
        return(FALSE);
    }

    if (g_ImageType == IMAGE_TYPE_RAMIMAGE)
    {
        g_dwMBRSectorNum = BLOCK_TO_SECTOR(IMAGE_START_BLOCK); 
        OALMSG(TRUE, (TEXT("g_dwMBRSectorNum = 0x%x\r\n"), g_dwMBRSectorNum));
        
         dwBlock = IMAGE_START_BLOCK; //by hmseo - 20061124 This block is to MBR
//      dwImageStart += dwLaunchAddr;
//      dwImageLength = dwImageLength; //step loader can support 4k bytes only. NO SuperIPL case is 16K....... 3 Block
    }

    RETAILMSG(1,(TEXT("Erase Block from 0x%x, to 0x%x \n"), dwBlock, SPECIAL_AREA_START + SPECIAL_AREA_SIZE - 1));
    for (nBlockNum = dwBlock ; nBlockNum < SPECIAL_AREA_START + SPECIAL_AREA_SIZE; nBlockNum++)
    {
        if (!FMD_EraseBlock(nBlockNum))
        {
            return(FALSE);
        }
    }

    if ( !VALID_TOC(g_pTOC) )
    {
        OALMSG(OAL_WARN, (TEXT("WARN: WriteOSImageToBootMedia: INVALID_TOC\r\n")));
        if ( !TOC_Init(g_dwTocEntry, g_ImageType, dwImageStart, dwImageLength, dwLaunchAddr) )
        {
            OALMSG(OAL_ERROR, (TEXT("ERROR: INVALID_TOC\r\n")));
            return(FALSE);
        }
    }

    // Look in the kernel region's extension area for a multi-BIN extension descriptor.
    // This region, if found, details the number, start, and size of each BIN region.
    //
    for (nCount = 0, dwNumExts = 0 ; (nCount < g_BINRegionInfo.dwNumRegions); nCount++)
    {
        // Does this region contain nk.exe and an extension pointer?
        //
        pExt = (EXTENSION *)GetKernelExtPointer(g_BINRegionInfo.Region[nCount].dwRegionStart,
                                                g_BINRegionInfo.Region[nCount].dwRegionLength );
        EdbgOutputDebugString("INFO: %s: [%d] RegionStart:0x%x, RegionLength:0x%x, pExt:0x%x\r\n",
                                                __FUNCTION__, nCount,
                                                g_BINRegionInfo.Region[nCount].dwRegionStart,
                                                g_BINRegionInfo.Region[nCount].dwRegionLength,
                                                pExt);
        if ( pExt != NULL)
        {
            // If there is an extension pointer region, walk it until the end.
            //
            while (pExt)
            {
                DWORD dwBaseAddr = g_BINRegionInfo.Region[nCount].dwRegionStart;
                pExt = (EXTENSION *)OEMMapMemAddr(dwBaseAddr, (DWORD)pExt);
                EdbgOutputDebugString("INFO: %s: [%d] Found chain extension: '%s' @ 0x%x\r\n", __FUNCTION__, nCount, pExt->name, dwBaseAddr);
                if ((pExt->type == 0) && !strcmp(pExt->name, "chain information"))
                {
                    pChainInfo = (PXIPCHAIN_SUMMARY) OEMMapMemAddr(dwBaseAddr, (DWORD)pExt->pdata);
                    dwNumExts = (pExt->length / sizeof(XIPCHAIN_SUMMARY));
                    EdbgOutputDebugString("INFO: %s: [%d] Found 'chain information' (pChainInfo=0x%x  Extensions=0x%x).\r\n", __FUNCTION__, nCount, (DWORD)pChainInfo, dwNumExts);
                    DumpXIPChainSummary(pChainInfo);
                    dwChainStart = (DWORD)pChainInfo->pvAddr;
                    dwChainLength = pChainInfo->dwMaxLength;
                    goto FoundChainAddress;
                }
                pExt = (EXTENSION *)pExt->pNextExt;
            }
        }
        else
        {
            //  Search for Chain region. Chain region doesn't have the ROMSIGNATURE set        
            EdbgOutputDebugString("Not Found pExtPointer, Check if this is Chain region.\n");        

            DWORD   dwRegionStart = g_BINRegionInfo.Region[nCount].dwRegionStart;
            DWORD   dwSig = *(LPDWORD) OEMMapMemAddr(dwRegionStart, dwRegionStart + ROM_SIGNATURE_OFFSET);

            if ( dwSig != ROM_SIGNATURE)
            {
                //  It is the chain
                dwChainStart = dwRegionStart;
                dwChainLength = g_BINRegionInfo.Region[nCount].dwRegionLength;
                OALMSG(TRUE, (TEXT("Found the Chain region: StartAddress: 0x%X; Length: 0x%X\n"), dwChainStart, dwChainLength));
            }
        }
    }
FoundChainAddress:    

    // Determine how big the Total BINFS partition needs to be to store all of this.
    //
    if (pChainInfo && dwNumExts == g_BINRegionInfo.dwNumRegions)    // We're downloading all the regions in a multi-region image...
    {
        DWORD i;
        OALMSG(TRUE, (TEXT("Writing multi-regions : Total %d regions\r\n"), dwNumExts));

        for (nCount = 0, dwBINFSPartLength = 0 ; nCount < dwNumExts ; nCount++)
        {
            dwBINFSPartLength += (pChainInfo + nCount)->dwMaxLength;
            OALMSG(OAL_ERROR, (TEXT("BINFSPartMaxLength[%u]: 0x%x, TtlBINFSPartLength: 0x%x \r\n"),
                nCount, (pChainInfo + nCount)->dwMaxLength, dwBINFSPartLength));

            // MultiBINInfo does not store each Regions MAX length, and pChainInfo is not in any particular order.
            // So, walk our MultiBINInfo matching up pChainInfo to find each regions MAX Length
            for (i = 0; i < dwNumExts; i++)
            {
                if ( g_BINRegionInfo.Region[i].dwRegionStart == (DWORD)((pChainInfo + nCount)->pvAddr) )
                {
                    dwMaxRegionLength[i] = (pChainInfo + nCount)->dwMaxLength;
                    OALMSG(TRUE, (TEXT("dwMaxRegionLength[%u]: 0x%x \r\n"), i, dwMaxRegionLength[i]));
                    break;
                }
            }
        }

    }
    else    // A single BIN file or potentially a multi-region update (but the partition's already been created in this latter case).
    {
        dwBINFSPartLength = g_BINRegionInfo.Region[0].dwRegionLength;
        OALMSG(TRUE, (TEXT("Writing single region/multi-region update, dwBINFSPartLength: %u \r\n"), dwBINFSPartLength));
    }

    OALMSG(TRUE, (TEXT("dwBlock = %d \n"), dwBlock));
    // Open/Create the BINFS partition where images are stored.  This partition starts immediately after the MBR on the Boot Media and its length is
    // determined by the maximum image size (or sum of all maximum sizes in a multi-region design).
    // Parameters are LOGICAL sectors.
    //
    hPart = BP_OpenPartition( (dwBlock)*(PAGES_PER_SUBLK)*(SECTORS_PER_SUPAGE),
                            //SECTOR_TO_BLOCK_SIZE(FILE_TO_SECTOR_SIZE(dwBINFSPartLength))*PAGES_PER_BLOCK, // align to block
                            //(dwBINFSPartLength/SECTOR_SIZE)+1,
                            SECTOR_TO_BLOCK_SIZE(FILE_TO_SECTOR_SIZE(dwBINFSPartLength))*SECTORS_PER_SUBLK + (IMAGE_START_BLOCK+1)*SECTORS_PER_SUBLK, // hmseo for whimory... +1 is for MBR
                            PART_BINFS,
                            TRUE,
                            PART_OPEN_ALWAYS);

    if (hPart == INVALID_HANDLE_VALUE )
    {
        OALMSG(OAL_ERROR, (TEXT("ERROR: WriteOSImageToBootMedia: Failed to open/create partition.\r\n")));
        return(FALSE);
    }

    // Are there multiple BIN files in RAM (we may just be updating one in a multi-BIN solution)?
    //
    for (nCount = 0, dwStoreOffset = 0; nCount < g_BINRegionInfo.dwNumRegions ; nCount++)
    {
        EdbgOutputDebugString("BIN Region Number : %d\r\n", nCount);
        DWORD dwRegionStart  = (DWORD)OEMMapMemAddr(0, g_BINRegionInfo.Region[nCount].dwRegionStart);
        DWORD dwRegionLength = g_BINRegionInfo.Region[nCount].dwRegionLength;

        // No concern about Multiple XIP
        OALMSG(TRUE, (TEXT("nCount = %d \n"), nCount));

        // Media byte offset where image region is stored.
        dwStoreOffset += nCount ? dwMaxRegionLength[nCount-1] : 0;

        // Set the file pointer (byte indexing) to the correct offset for this particular region.
        //
        if ( !BP_SetDataPointer(hPart, dwStoreOffset + (dwBlock+1)*BYTES_PER_SECTOR*SECTORS_PER_SUBLK) )
        {
            OALMSG(OAL_ERROR, (TEXT("ERROR: StoreImageToBootMedia: Failed to set data pointer in partition (offset=0x%x).\r\n"), dwStoreOffset));
            return(FALSE);
        }

        // Write the region to the BINFS partition.
        if ( !BP_WriteData(hPart, (LPBYTE)dwRegionStart, dwRegionLength) )
        {
            EdbgOutputDebugString("ERROR: StoreImageToBootMedia: Failed to write region to BINFS partition (start=0x%x, length=0x%x).\r\n", dwRegionStart, dwRegionLength);
            return(FALSE);
        }

        // update our TOC?
        //
        if ((g_pTOC->id[g_dwTocEntry].dwLoadAddress == g_BINRegionInfo.Region[nCount].dwRegionStart) &&
             g_pTOC->id[g_dwTocEntry].dwTtlSectors == FILE_TO_SECTOR_SIZE(dwRegionLength) )
        {
            g_pTOC->id[g_dwTocEntry].dwStoreOffset = dwStoreOffset;
            g_pTOC->id[g_dwTocEntry].dwJumpAddress = 0; // Filled upon return to OEMLaunch

            g_pTOC->id[g_dwTocEntry].dwImageType = g_ImageType;

            g_pTOC->id[g_dwTocEntry].sgList[0].dwSector = FILE_TO_SECTOR_SIZE(g_dwLastWrittenLoc);
            g_pTOC->id[g_dwTocEntry].sgList[0].dwLength = g_pTOC->id[g_dwTocEntry].dwTtlSectors;

            // copy Kernel Region to SDRAM for jump
//            memcpy((void*)g_pTOC->id[g_dwTocEntry].dwLoadAddress, (void*)dwRegionStart, dwRegionLength);

            OALMSG(TRUE, (TEXT("Update Last TOC[%d] Entry to TOC cache in memory!\r\n"), g_pTOC->id[g_dwTocEntry]));
        } 
        else if( (dwChainStart == g_BINRegionInfo.Region[nCount].dwRegionStart) &&
                (dwChainLength == g_BINRegionInfo.Region[nCount].dwRegionLength)) 
        {
            //  Update our TOC for Chain region
            g_pTOC->chainInfo.dwLoadAddress = dwChainStart;
            g_pTOC->chainInfo.dwFlashAddress = FILE_TO_SECTOR_SIZE(g_dwLastWrittenLoc);
            g_pTOC->chainInfo.dwLength = FILE_TO_SECTOR_SIZE(dwMaxRegionLength[nCount]);

            OALMSG(TRUE, (TEXT("Written Chain Region to the Flash\n")));
            OALMSG(TRUE, (TEXT("LoadAddress = 0x%X; FlashAddress = 0x%X; Length = 0x%X\n"), 
                        g_pTOC->chainInfo.dwLoadAddress, 
                        g_pTOC->chainInfo.dwFlashAddress,
                        g_pTOC->chainInfo.dwLength));
            OALMSG(TRUE, (TEXT(" memcpy : g_pTOC->chainInfo.dwLoadAddress = 0x%X; dwRegionStart = 0x%X; dwRegionLength = 0x%X\n"), 
                        g_pTOC->chainInfo.dwLoadAddress, 
                        dwRegionStart,
                        dwRegionLength));
            // Now copy it to the SDRAM
            // by hmseo.... ???? 061125
            //memcpy((void *)g_pTOC->chainInfo.dwLoadAddress, (void *)dwRegionStart, dwRegionLength);
        }
    }

    TOC_Print();

    OALMSG(TRUE, (TEXT("-WriteOSImageToBootMedia\r\n")));

    return(TRUE);
}


/*
    @func   BOOL | ReadKernelRegionFromBootMedia |
            BinFS support. Reads the kernel region from Boot Media into RAM.  The kernel region is fixed up
            to run from RAM and this is done just before jumping to the kernel entry point.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm
    @xref
*/
BOOL ReadOSImageFromBootMedia()
{
    OALMSG(OAL_FUNC, (TEXT("+ReadOSImageFromBootMedia\r\n")));

    if (!g_bBootMediaExist)
    {
        OALMSG(OAL_ERROR, (TEXT("ERROR: ReadOSImageFromBootMedia: device doesn't exist.\r\n")));
        return(FALSE);
    }

    if ( !VALID_TOC(g_pTOC) )
    {
        OALMSG(OAL_ERROR, (TEXT("ERROR: ReadOSImageFromBootMedia: INVALID_TOC\r\n")));
        return(FALSE);
    }

    if ( !VALID_IMAGE_DESCRIPTOR(&g_pTOC->id[g_dwTocEntry]) )
    {
        OALMSG(OAL_ERROR, (TEXT("ReadOSImageFromBootMedia: ERROR_INVALID_IMAGE_DESCRIPTOR: 0x%x\r\n"),
            g_pTOC->id[g_dwTocEntry].dwSignature));
        return FALSE;
    }

    if ( !OEMVerifyMemory(g_pTOC->id[g_dwTocEntry].dwLoadAddress, sizeof(DWORD)) ||
         !OEMVerifyMemory(g_pTOC->id[g_dwTocEntry].dwJumpAddress, sizeof(DWORD)) ||
         !g_pTOC->id[g_dwTocEntry].dwTtlSectors )
    {
        OALMSG(OAL_ERROR, (TEXT("ReadOSImageFromBootMedia: ERROR_INVALID_ADDRESS: (address=0x%x, sectors=0x%x, launch address=0x%x)...\r\n"),
            g_pTOC->id[g_dwTocEntry].dwLoadAddress, g_pTOC->id[g_dwTocEntry].dwTtlSectors, g_pTOC->id[g_dwTocEntry].dwJumpAddress));
        return FALSE;
    }

    {
        DWORD dwStartPage, dwNumPage, dwPage;
        Buffer InBuf;
        DWORD dwRegionStart = (DWORD)((g_pTOC->id[g_dwTocEntry].dwLoadAddress) | CACHED_TO_UNCACHED_OFFSET);
        DWORD dwRegionLength = SECTOR_TO_FILE_SIZE(g_pTOC->id[g_dwTocEntry].dwTtlSectors);

         dwStartPage = (IMAGE_START_BLOCK+1)*(PAGES_PER_SUBLK);
        dwNumPage = (dwRegionLength-1)/BYTES_PER_MAIN_SUPAGE+1;

        OALMSG(TRUE, (TEXT("Read OS image to BootMedia \r\n")));
        OALMSG(TRUE, (TEXT("ImageLength = %d Byte \r\n"), dwRegionLength));
        OALMSG(TRUE, (TEXT("Start Page = %d, End Page = %d, Page Count = %d\r\n"), dwStartPage, dwStartPage+dwNumPage-1, dwNumPage));

        InBuf.pData = (unsigned char *)dwRegionStart;
        InBuf.pSpare = NULL;
        InBuf.nBank = 0;
        if ( TWO_PLANE_PROGRAM == TRUE32 )
        {
             InBuf.nBitmap = FULL_SECTOR_BITMAP_PAGE; 
         }
        else
        {
             InBuf.nBitmap = LEFT_SECTOR_BITMAP_PAGE; 
         }
        InBuf.eStatus = BUF_AUX;    // No need to sync

        for (dwPage = dwStartPage; dwPage < dwStartPage+dwNumPage; dwPage++)
        {
            if (VFL_Read(dwPage, &InBuf, FALSE32) != FIL_SUCCESS)
            {
                OALMSG(TRUE, (TEXT("[ERR] VFL Read Error @ %d page\r\n"), dwPage));
                OALMSG(TRUE, (TEXT("Read OS image to BootMedia Failed !!!\r\n")));
                while(1);
            }
//        OALMSG(TRUE, (TEXT("dwPage = %d, End Page = %d, Buf = 0x%x\r\n"), dwPage, dwStartPage+dwNumPage, *((UINT32 *)InBuf.pData)));  // ksk dbg

            InBuf.pData += BYTES_PER_MAIN_SUPAGE;
            
            if (dwPage%PAGES_PER_BLOCK == (PAGES_PER_BLOCK-1)) OALMSG(TRUE, (TEXT(".")));
        }

        OALMSG(TRUE, (TEXT("\r\nRead OS image to BootMedia Success \r\n")));
    }

    OALMSG(OAL_FUNC, (TEXT("_ReadOSImageFromBootMedia\r\n")));
    return(TRUE);
}

BOOL ReadBlock(DWORD dwBlock, LPBYTE pbBlock, PSectorInfo pSectorInfoTable)
{
#if 0
    for (int iSector = 0; iSector < g_FlashInfo.wSectorsPerBlock; iSector++) {
        if (!FMD_ReadSector(dwBlock * g_FlashInfo.wSectorsPerBlock + iSector, pbBlock, pSectorInfoTable, 1))
            return FALSE;
        if (pbBlock)
            pbBlock += g_FlashInfo.wDataBytesPerSector;
        if (pSectorInfoTable)
            pSectorInfoTable++;
    }
#else
    if (!FMD_ReadSector(dwBlock * g_FlashInfo.wSectorsPerBlock, pbBlock, /*pSectorInfoTable*/NULL, g_FlashInfo.wSectorsPerBlock))        // hmseo-061028
        return FALSE;
#endif
    return TRUE;
}

BOOL WriteBlock(DWORD dwBlock, LPBYTE pbBlock, PSectorInfo pSectorInfoTable)
{
#if 0
    for (int iSector = 0; iSector < g_FlashInfo.wSectorsPerBlock; iSector++) {
        if (!FMD_WriteSector(dwBlock * g_FlashInfo.wSectorsPerBlock + iSector, pbBlock, /*pSectorInfoTable*/NULL, 1))        // hmseo-061028
            return FALSE;
        if (pbBlock)
            pbBlock += g_FlashInfo.wDataBytesPerSector;
        if (pSectorInfoTable)
            pSectorInfoTable++;
    }
#else
    if (!FMD_WriteSector(dwBlock * g_FlashInfo.wSectorsPerBlock, pbBlock, /*pSectorInfoTable*/NULL, g_FlashInfo.wSectorsPerBlock))        // hmseo-061028
        return FALSE;
#endif
    return TRUE;
}

static UCHAR WMRBuf[WMRBUFSIZE];
static UCHAR BadInfoBuf[(4096+256)*2];

BOOL WriteRawImageToBootMedia(DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr)
{
    LowFuncTbl *pLowFuncTbl;

    UINT32 dwStartPage, dwNumPage, dwPage;
    UINT32 dwStartBlock, dwNumBlock, dwBlock;
    UINT32 dwPageOffset;
    INT32 nRet;
    BOOL32 bIsBadBlock = FALSE32;
    UINT32 nSyncRet;

    LPBYTE pbBuffer;

    OALMSG(OAL_FUNC, (TEXT("+WriteRawImageToBootMedia\r\n")));

    if ( !g_bBootMediaExist )
    {
        OALMSG(OAL_ERROR, (TEXT("ERROR: WriteRawImageToBootMedia: device doesn't exist.\r\n")));
        return(FALSE);
    }

    if (g_ImageType == IMAGE_TYPE_LOADER)
    {
        UINT8 *pMBuf;
        UINT8 *pSBuf;

        pbBuffer = OEMMapMemAddr(dwImageStart, dwImageStart);

        dwStartBlock = EBOOT_BLOCK;
        dwNumBlock = (dwImageLength-1)/(BYTES_PER_MAIN_SUPAGE*PAGES_PER_BLOCK)+1;

        if ( !VALID_TOC(g_pTOC) )
        {
            OALMSG(OAL_WARN, (TEXT("WARN: WriteRawImageToBootMedia: INVALID_TOC\r\n")));
            if ( !TOC_Init(g_dwTocEntry, g_ImageType, dwImageStart, dwImageLength, dwLaunchAddr) )
            {
                OALMSG(OAL_ERROR, (TEXT("ERROR: INVALID_TOC\r\n")));
                return(FALSE);
            }
        }

        OALMSG(TRUE, (TEXT("Write Eboot image to BootMedia \r\n")));
        OALMSG(TRUE, (TEXT("ImageLength = %d Byte \r\n"), dwImageLength));
        OALMSG(TRUE, (TEXT("Start Block = %d, End Block = %d, Block Count = %d\r\n"), dwStartBlock, dwStartBlock+dwNumBlock-1, dwNumBlock));

        pLowFuncTbl = FIL_GetFuncTbl();

        pMBuf = WMRBuf;
        pSBuf = WMRBuf+BYTES_PER_MAIN_SUPAGE;

        dwBlock = dwStartBlock;

        while(dwNumBlock > 0)
        {
            if (dwBlock == (EBOOT_BLOCK+EBOOT_BLOCK_SIZE+EBOOT_BLOCK_RESERVED))
            {
                OALMSG(TRUE, (TEXT("Write RAW image to BootMedia Failed !!!\r\n")));
                OALMSG(TRUE, (TEXT("Too many Bad Block\r\n")));
                return(FALSE);
            }

            IS_CHECK_SPARE_ECC = FALSE32;
            pLowFuncTbl->Read(0, dwBlock*PAGES_PER_BLOCK+PAGES_PER_BLOCK-1, 0x0, enuBOTH_PLANE_BITMAP, NULL, pSBuf, TRUE32, FALSE32);
            IS_CHECK_SPARE_ECC = TRUE32;

            if (TWO_PLANE_PROGRAM == TRUE32)
            {
                if (pSBuf[0] == 0xff && pSBuf[BYTES_PER_SPARE_PAGE] == 0xff)
                    bIsBadBlock = TRUE32;
            }
            else
            {
                if (pSBuf[0] == 0xff)
                    bIsBadBlock = TRUE32;
            }

            if (bIsBadBlock)
            {
                pLowFuncTbl->Erase(0, dwBlock, enuBOTH_PLANE_BITMAP);
                nRet = pLowFuncTbl->Sync(0, &nSyncRet);
                if ( nRet != FIL_SUCCESS)
                {
                    OALMSG(TRUE, (TEXT("[ERR] FIL Erase Error @ %d block, Skipped\r\n"), dwBlock));
                    goto MarkAndSkipBadBlock;
                }

                for (dwPageOffset=0; dwPageOffset<PAGES_PER_BLOCK; dwPageOffset++)
                {
                    pLowFuncTbl->Write(0, dwBlock*PAGES_PER_BLOCK+dwPageOffset, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, pbBuffer+BYTES_PER_MAIN_SUPAGE*dwPageOffset, NULL);
                    nRet = pLowFuncTbl->Sync(0, &nSyncRet);
                    if (nRet != FIL_SUCCESS)
                    {
                        OALMSG(TRUE, (TEXT("[ERR] FIL Write Error @ %d Block %d Page, Skipped\r\n"), dwBlock, dwPageOffset));
                        goto MarkAndSkipBadBlock;
                    }

                    nRet = pLowFuncTbl->Read(0, dwBlock*PAGES_PER_BLOCK+dwPageOffset, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, pMBuf, NULL, FALSE32, FALSE32);
                    if (nRet != FIL_SUCCESS)
                    {
                        OALMSG(TRUE, (TEXT("[ERR] FIL Read Error @ %d Block %d Page, Skipped\r\n"), dwBlock, dwPageOffset));
                        goto MarkAndSkipBadBlock;
                    }

                    if (0 != memcmp(pbBuffer+BYTES_PER_MAIN_SUPAGE*dwPageOffset, pMBuf, BYTES_PER_MAIN_SUPAGE))
                    {
                        OALMSG(TRUE, (TEXT("[ERR] Verify Error @ %d Block %d Page, Skipped\r\n"), dwBlock, dwPageOffset));
                        goto MarkAndSkipBadBlock;
                    }
                }

                OALMSG(TRUE, (TEXT("[OK] Write %d th Block Success\r\n"), dwBlock));
                dwBlock++;
                dwNumBlock--;
                pbBuffer += BYTES_PER_MAIN_SUPAGE*PAGES_PER_BLOCK;
                continue;

MarkAndSkipBadBlock:

                pLowFuncTbl->Erase(0, dwBlock, enuBOTH_PLANE_BITMAP);
                memset(pSBuf, 0x0, BYTES_PER_SPARE_SUPAGE);
                IS_CHECK_SPARE_ECC = FALSE32;
                pLowFuncTbl->Write(0, dwBlock*PAGES_PER_BLOCK+PAGES_PER_BLOCK-1, 0x0, enuBOTH_PLANE_BITMAP, NULL, pSBuf);
                IS_CHECK_SPARE_ECC = TRUE32;
                dwBlock++;
                continue;
            }
            else
            {
                OALMSG(TRUE, (TEXT("Bad Block %d Skipped\r\n"), dwBlock));
                dwBlock++;
                continue;
            }
        }

        OALMSG(TRUE, (TEXT("Write Eboot image to BootMedia Success\r\n")));
    }
    else if (g_ImageType == IMAGE_TYPE_STEPLDR)
    {
        BOOL bFormatted = FALSE;
        UINT32 nSctBitmap;
        UINT32 nBufCnt;
        UINT32 nCnt;

        pLowFuncTbl = FIL_GetFuncTbl();
        nRet = pLowFuncTbl->Read(0, PAGES_PER_BLOCK - 1, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, WMRBuf, WMRBuf+BYTES_PER_MAIN_SUPAGE, TRUE32, TRUE32);
        if (nRet == FIL_SUCCESS_CLEAN || nRet == FIL_U_ECC_ERROR)    // WMR Not formatted
        {
            bFormatted = FALSE;
        }
        else
        {
            OALMSG(TRUE, (TEXT("VFL Already Formatted\r\n")));
            nRet = pLowFuncTbl->Read(0, PAGES_PER_BLOCK-2, LEFT_SECTOR_BITMAP_PAGE, enuLEFT_PLANE_BITMAP, BadInfoBuf, NULL, FALSE32, FALSE32);
            if (nRet == FIL_SUCCESS)
            {
                bFormatted = TRUE;
            }
            else
            {
                OALMSG(TRUE, (TEXT("VFL Formatted... But There is no Initial Bad Block Infomation -> Clear VFL Signiture\r\n")));
                bFormatted = FALSE;
            }
        }

        pbBuffer = OEMMapMemAddr(dwImageStart, dwImageStart);

        dwStartPage = 0;

        dwNumPage = (dwImageLength)/(BYTES_PER_MAIN_PAGE)+((dwImageLength%BYTES_PER_MAIN_PAGE)? 1 : 0);
        if (SECTORS_PER_PAGE == 8) dwNumPage++;  // page No. 0 and 1 use only 2KByte/Page, so add 1 page.

        OALMSG(TRUE, (TEXT("Write Steploader (NBL1+NBL2) image to BootMedia dwNumPage : %d \r\n"),dwNumPage));
        OALMSG(TRUE, (TEXT("ImageLength = %d Byte \r\n"), dwImageLength));
        OALMSG(TRUE, (TEXT("Start Page = %d, End Page = %d, Page Count = %d\r\n"), dwStartPage, dwStartPage+dwNumPage-1, dwNumPage));

        for (dwPage = dwStartPage, nCnt = 0; nCnt < dwNumPage; nCnt++)
        {
            if (dwPage < 2)
            {
                if(BYTES_PER_MAIN_PAGE == 2048)//for 2Kpage
                {
                    nSctBitmap = 0xf;
                    nBufCnt = BYTES_PER_SECTOR*4;
                }
                else if(BYTES_PER_MAIN_PAGE == 4096)//for 4Kpage
                {
                    nSctBitmap = 0xff;
                    nBufCnt = BYTES_PER_SECTOR*8;
                }
            }
            else
            {
                nSctBitmap = LEFT_SECTOR_BITMAP_PAGE;
                nBufCnt = BYTES_PER_MAIN_PAGE;
            }
            
            if (dwPage%PAGES_PER_BLOCK == 0)
            {
                pLowFuncTbl->Erase(0, dwPage/PAGES_PER_BLOCK, enuBOTH_PLANE_BITMAP);
                if (pLowFuncTbl->Sync(0, &nSyncRet) != FIL_SUCCESS)
                {
                    OALMSG(TRUE, (TEXT("[ERR] FIL Erase Error @ %d block\r\n"), dwPage/PAGES_PER_BLOCK));
                    OALMSG(TRUE, (TEXT("Write Steploader image to BootMedia Failed !!!\r\n")));
                    return FALSE;
                }
            }

            OALMSG(OAL_FUNC, (TEXT(" dwPage = 0x%x, pbBuffer = 0x%x \r\n"), dwPage, pbBuffer));

            if (dwPage < PAGES_PER_BLOCK-2)
            {
                #ifdef _IROMBOOT_
                pLowFuncTbl->Steploader_Write(0, dwPage, nSctBitmap, enuLEFT_PLANE_BITMAP, pbBuffer, NULL);
                #else
                pLowFuncTbl->Write(0, dwPage, nSctBitmap, enuLEFT_PLANE_BITMAP, pbBuffer, NULL);
                #endif
            }
            else
            {
                OALMSG(TRUE, (TEXT("Cannot Write image on page %d (Reserved area) !!!\r\n"), dwPage));
                return FALSE;
            }
            
            if (pLowFuncTbl->Sync(0, &nSyncRet) != FIL_SUCCESS)
            {
                OALMSG(TRUE, (TEXT("[ERR] FIL Write Error @ %d page\r\n"), dwPage));
                OALMSG(TRUE, (TEXT("Write Steploader image to BootMedia Failed !!!\r\n")));
                return FALSE;
            }

            // write pages with 0, 1 and 6 to PAGES_PER_BLOCK-3
            dwPage++;
            if(BYTES_PER_MAIN_PAGE == 2048)//for 2Kpage
            {
                if (IS_MLC && dwPage >= 4 && dwPage < 10) dwPage = 10; //for 8K Stepping stone
            }
            else if(BYTES_PER_MAIN_PAGE == 4096)//for 4Kpage
            {
                if (IS_MLC && dwPage >= 2 && dwPage < 10) dwPage = 10; //for 8K Stepping stone
            }
            pbBuffer += nBufCnt;
        }

        // Write WMR Data (last page of block 0)
        if (bFormatted)
        {
            pLowFuncTbl->Write(0, PAGES_PER_BLOCK-2, LEFT_SECTOR_BITMAP_PAGE, enuLEFT_PLANE_BITMAP, BadInfoBuf, NULL);
            if (pLowFuncTbl->Sync(0, &nSyncRet) != FIL_SUCCESS)
            {
                OALMSG(TRUE, (TEXT("[ERR] FIL Write Error @ %d page\r\n"), PAGES_PER_BLOCK-2));
                OALMSG(TRUE, (TEXT("Write Steploader image to BootMedia Failed !!!\r\n")));
                return FALSE;
            }
            else
            {
                pLowFuncTbl->Write(0, PAGES_PER_BLOCK-1, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, WMRBuf, WMRBuf+BYTES_PER_MAIN_SUPAGE);
                if (pLowFuncTbl->Sync(0, &nSyncRet) != FIL_SUCCESS)
                {
                    OALMSG(TRUE, (TEXT("[ERR] FIL Write Error @ %d page\r\n"), PAGES_PER_BLOCK-1));
                    OALMSG(TRUE, (TEXT("Write Steploader image to BootMedia Failed !!!\r\n")));
                    return FALSE;
                }
            }
        }

        OALMSG(TRUE, (TEXT("Write Steploader image to BootMedia Success\r\n")));
    }

    if (g_ImageType == IMAGE_TYPE_LOADER)
    {
        g_pTOC->id[0].dwLoadAddress = dwImageStart;
        g_pTOC->id[0].dwJumpAddress = 0;
        g_pTOC->id[0].dwTtlSectors  = FILE_TO_SECTOR_SIZE(dwImageLength);
        g_pTOC->id[0].sgList[0].dwSector = BLOCK_TO_SECTOR(EBOOT_BLOCK);
        g_pTOC->id[0].sgList[0].dwLength = g_pTOC->id[0].dwTtlSectors;
    }

    OALMSG(OAL_FUNC, (TEXT("_WriteRawImageToBootMedia\r\n")));

    return TRUE;
}

static DWORD GetDecimalNumber(void)
{
    DWORD dwNumber = 0;
    int InChar = 0;

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            if ((InChar >= '0' && InChar <= '9'))
            {
                dwNumber = dwNumber*10;
                dwNumber = dwNumber+(InChar-'0');
                OEMWriteDebugByte((BYTE)InChar);
            }
            else if (InChar == 8)        // If it's a backspace, back up.
            {
                dwNumber = dwNumber/10;
                OEMWriteDebugByte((BYTE)InChar);
            }
        }
    }

    OEMWriteDebugByte('\n');
    OEMWriteDebugByte('\r');

    return dwNumber;
}

static void MLC_Print_Page_Data_VFL(unsigned char *pMBuf, unsigned char *pSBuf, unsigned int OSIndexAddr)
{
    unsigned int i, j;

    if (pMBuf != NULL)
    {
        OALMSG(TRUE, (TEXT("Main Data\r\n")));
        for (j = 0; j < SECTORS_PER_PAGE; j++)
        {
            OALMSG(TRUE, (TEXT("=========================================================== Sector %d\n"), j));
            for (i = (BYTES_PER_SECTOR * j); i < (BYTES_PER_SECTOR * (j+1)); i++)
            {
                if (i%BYTES_PER_SPARE == 0) 
                    OALMSG(TRUE, (TEXT("%08xh: "), OSIndexAddr + i));
                OALMSG(TRUE, (TEXT("%02x "), pMBuf[i]));
                if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
            }
        }
#if 1
        if (TWO_PLANE_PROGRAM == TRUE32)
        {
            for (j = SECTORS_PER_PAGE; j < (SECTORS_PER_PAGE * 2); j++)
            {
                OALMSG(TRUE, (TEXT("=========================================================== Sector %d\n"), j));
                for (i = (BYTES_PER_SECTOR * j); i < (BYTES_PER_SECTOR * (j+1)); i++)
                {
                    if (i%BYTES_PER_SPARE == 0) 
                        OALMSG(TRUE, (TEXT("%08xh: "), OSIndexAddr + i));
                    OALMSG(TRUE, (TEXT("%02x "), pMBuf[i]));
                    if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
                }
            }
        }
#endif
        OALMSG(TRUE, (TEXT("=================================================\n")));
    }

    if (pSBuf != NULL)
    {
        OALMSG(TRUE, (TEXT("Spare Data\n")));
        for (i = 0; i < BYTES_PER_SPARE_PAGE; i++)
        {
            OALMSG(TRUE, (TEXT("%02x "), pSBuf[i]));
            if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
        }
#if 1
        if (TWO_PLANE_PROGRAM == TRUE32)
        {
            for (i = BYTES_PER_SPARE_PAGE; i < BYTES_PER_SPARE_SUPAGE; i++)
            {
                OALMSG(TRUE, (TEXT("%02x "), pSBuf[i]));
                if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
            }
        }
#endif
        OALMSG(TRUE, (TEXT("=================================================\n")));
    }
}


static void MLC_Print_Page_Data(unsigned char *pMBuf, unsigned char *pSBuf)
{
    unsigned int i, j;

    if (pMBuf != NULL)
    {
        OALMSG(TRUE, (TEXT("Main Data\r\n")));
        for (j = 0; j < SECTORS_PER_PAGE; j++)
        {
            OALMSG(TRUE, (TEXT("================================================= Sector %d\n"), j));
            for (i = (BYTES_PER_SECTOR * j); i < (BYTES_PER_SECTOR * (j+1)); i++)
            {
                OALMSG(TRUE, (TEXT("%02x "), pMBuf[i]));
                if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
            }
        }
#if 1
        if (TWO_PLANE_PROGRAM == TRUE32)
        {
            for (j = SECTORS_PER_PAGE; j < (SECTORS_PER_PAGE * 2); j++)
            {
                OALMSG(TRUE, (TEXT("================================================= Sector %d\n"), j));
                for (i = (BYTES_PER_SECTOR * j); i < (BYTES_PER_SECTOR * (j+1)); i++)
                {
                    OALMSG(TRUE, (TEXT("%02x "), pMBuf[i]));
                    if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
                }
            }
        }
#endif
        OALMSG(TRUE, (TEXT("=================================================\n")));
    }

    if (pSBuf != NULL)
    {
        OALMSG(TRUE, (TEXT("Spare Data\n")));
        for (i = 0; i < BYTES_PER_SPARE_PAGE; i++)
        {
            OALMSG(TRUE, (TEXT("%02x "), pSBuf[i]));
            if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
        }
#if 1
        if (TWO_PLANE_PROGRAM == TRUE32)
        {
            for (i = BYTES_PER_SPARE_PAGE; i < BYTES_PER_SPARE_SUPAGE; i++)
            {
                OALMSG(TRUE, (TEXT("%02x "), pSBuf[i]));
                if (i%BYTES_PER_SPARE == 15) OALMSG(TRUE, (TEXT("\n")));
            }
        }
#endif
        OALMSG(TRUE, (TEXT("=================================================\n")));
    }
}

static void MLC_FIL_Erase_Test(void)
{
    DWORD dwNum;
    LowFuncTbl *pLowFuncTbl;
    UINT32 nSyncRet;

    pLowFuncTbl = FIL_GetFuncTbl();

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n FIL Erase Test \r\n")));
        OALMSG(TRUE, (TEXT(" Block Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum > 0 && dwNum < SUBLKS_TOTAL)
        {
            pLowFuncTbl->Erase(0, dwNum, enuBOTH_PLANE_BITMAP);
            if (pLowFuncTbl->Sync(0, &nSyncRet))
            {
                OALMSG(TRUE, (TEXT(" FIL Erase Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" FIL Erase Success \r\n")));
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Block Number ! You can erase 1~%d block \r\n"), SUBLKS_TOTAL-1));
        }
    }
}

static void MLC_FIL_Write_Test(void)
{
    DWORD dwNum;
    UINT32 iCnt;
    unsigned char *pMBuf;
    unsigned char *pSBuf;
    LowFuncTbl *pLowFuncTbl;
    UINT32 nSyncRet;

    pLowFuncTbl = FIL_GetFuncTbl();

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_SUPAGE;
//    memcpy((void *)pMBuf, (void *)InitialImage_rgb16_320x240, BYTES_PER_MAIN_SUPAGE);
    memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_SUPAGE);

    for (iCnt=0; iCnt<BYTES_PER_MAIN_SUPAGE; iCnt++)
    {
        pMBuf[iCnt] = iCnt;
    }

    for (iCnt=0; iCnt<BYTES_PER_SPARE_SUPAGE; iCnt++)
    {
        pSBuf[iCnt] = iCnt;
    }

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n FIL Write Test \r\n")));
        OALMSG(TRUE, (TEXT(" Page Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum >= PAGES_PER_BLOCK && dwNum < (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1))
        {
            pLowFuncTbl->Write(0, dwNum, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, pMBuf, pSBuf);
            if (pLowFuncTbl->Sync(0, &nSyncRet))
            {
                OALMSG(TRUE, (TEXT(" FIL Write Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" FIL Write Success \r\n")));
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Page Number ! You can write %d~%d page \r\n"), PAGES_PER_BLOCK, (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1)));
        }
    }
}

static void MLC_FIL_Read_Test(void)
{
    DWORD dwNum;
    unsigned char *pMBuf;
    unsigned char *pSBuf;
    LowFuncTbl *pLowFuncTbl;
    INT32 nRet;

    pLowFuncTbl = FIL_GetFuncTbl();

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_SUPAGE;

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n FIL Read Test \r\n")));
        OALMSG(TRUE, (TEXT(" Page Number [99999999 to exit] = ")));
        memset((void *)pMBuf, 0xff, BYTES_PER_MAIN_SUPAGE);
        memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_SUPAGE);

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum >= 0 && dwNum < (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1))
        {
            nRet = pLowFuncTbl->Read(0, dwNum, FULL_SECTOR_BITMAP_PAGE, enuBOTH_PLANE_BITMAP, pMBuf, pSBuf, 1, 0);
            if (nRet)
            {
                OALMSG(TRUE, (TEXT(" FIL Read Fail \r\n")));
                MLC_Print_Page_Data(pMBuf, pSBuf);
            }
            else
            {
                OALMSG(TRUE, (TEXT(" FIL Read Success \r\n")));
                MLC_Print_Page_Data(pMBuf, pSBuf);
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Page Number ! You can read 0~%d page \r\n"), (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1)));
        }
    }
}

static void MLC_VFL_Erase_Test(void)
{
    DWORD dwNum;

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n VFL Erase Test \r\n")));
        OALMSG(TRUE, (TEXT(" Block Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum > 0 && dwNum < BLOCKS_PER_BANK)
        {
            VFL_Erase(dwNum);
            if (VFL_Sync())
            {
                OALMSG(TRUE, (TEXT(" VFL Erase Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" VFL Erase Success \r\n")));
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Block Number ! You can erase 1~%d block \r\n"), BLOCKS_PER_BANK-1));
        }
    }
}

static void MLC_VFL_Write_Test(void)
{
    DWORD dwNum;
    UINT32 iCnt;
    unsigned char *pMBuf;
    unsigned char *pSBuf;
    Buffer InBuf;

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_SUPAGE;
//    memcpy((void *)pMBuf, (void *)InitialImage_rgb16_320x240, BYTES_PER_MAIN_SUPAGE);
    memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_SUPAGE);

    for (iCnt=0; iCnt<BYTES_PER_MAIN_SUPAGE; iCnt++)
    {
        pMBuf[iCnt] = iCnt;
    }


    for (iCnt=0; iCnt<BYTES_PER_SPARE_SUPAGE; iCnt++)
    {
        pSBuf[iCnt] = iCnt;
    }

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n VFL Write Test \r\n")));
        OALMSG(TRUE, (TEXT(" Page Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum >= PAGES_PER_BLOCK && dwNum < (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1))
        {
            InBuf.nBitmap = FULL_SECTOR_BITMAP_PAGE;
            InBuf.eStatus = BUF_AUX;
            InBuf.pData = pMBuf;
            InBuf.pSpare = pSBuf;

            VFL_Write(dwNum, &InBuf);
            if (VFL_Sync())
            {
                OALMSG(TRUE, (TEXT(" VFL Write Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" VFL Write Success \r\n")));
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Page Number ! You can write %d~%d page \r\n"), PAGES_PER_BLOCK, (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1)));
        }
    }
}

static void MLC_VFL_Read_Test(void)
{
    DWORD dwNum;
    unsigned char *pMBuf;
    unsigned char *pSBuf;
    LowFuncTbl *pLowFuncTbl;
    Buffer InBuf;
    INT32 nRet;

    pLowFuncTbl = FIL_GetFuncTbl();

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_SUPAGE;

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n VFL Read Test \r\n")));
        OALMSG(TRUE, (TEXT(" Page Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum >= PAGES_PER_BLOCK && dwNum < (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1))
        {
            memset((void *)pMBuf, 0xff, BYTES_PER_MAIN_SUPAGE);
            memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_SUPAGE);

            InBuf.nBitmap = FULL_SECTOR_BITMAP_PAGE;
            InBuf.eStatus = BUF_AUX;
            InBuf.pData = pMBuf;
            InBuf.pSpare = pSBuf;

            nRet = VFL_Read(dwNum, &InBuf, 0);
            if (nRet)
            {
                OALMSG(TRUE, (TEXT(" VFL Read Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" VFL Read Success \r\n")));
                MLC_Print_Page_Data_VFL(pMBuf, pSBuf, (dwNum-1408)*512*8 );
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Page Number ! You can read %d~%d page \r\n"), PAGES_PER_BLOCK-1, (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1)));
        }
    }
}

#define FTL_TEST 0

#if FTL_TEST
#include <FTL.h>

static void MLC_FTL_Write_Test(void)
{
    DWORD dwNum;
    UINT32 iCnt;
    unsigned char *pMBuf;
    unsigned char *pSBuf;

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_SUPAGE;
    memcpy((void *)pMBuf, (void *)prayer16bpp, BYTES_PER_MAIN_SUPAGE);
    memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_SUPAGE);

    for (iCnt=0; iCnt<BYTES_PER_SPARE_SUPAGE; iCnt++)
    {
        pSBuf[iCnt] = iCnt;
    }

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n FTL Write Test \r\n")));
        OALMSG(TRUE, (TEXT(" Sector Number [0 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 0) break;
        else if (dwNum >= 0 && dwNum < (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1))
        {
            FTL_Write(dwNum, 16, pMBuf);
            if (VFL_Sync())
            {
                OALMSG(TRUE, (TEXT(" FTL Write Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" FTL Write Success \r\n")));
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Sector Number ! You can write 0~%d sector \r\n"), (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1)));
        }
    }
}

static void MLC_FTL_Read_Test(void)
{
    DWORD dwNum;
    unsigned char *pMBuf;
    unsigned char *pSBuf;
    LowFuncTbl *pLowFuncTbl;
    UINT32 nRet;
    volatile S3C2443_IOPORT_REG *s2443IOP = (S3C2443_IOPORT_REG *)OALPAtoVA(S3C2443_BASE_REG_PA_IOPORT, FALSE);

    pLowFuncTbl = FIL_GetFuncTbl();

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_SUPAGE;

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n FTL Read Test \r\n")));
        OALMSG(TRUE, (TEXT(" Sector Number [0 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 0) break;
        else if (dwNum >= 0 && dwNum < (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1))
        {
            memset((void *)pMBuf, 0xff, BYTES_PER_MAIN_SUPAGE);
            memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_SUPAGE);

            nRet = FTL_Read(dwNum, 16, pMBuf);
            if (nRet)
            {
                OALMSG(TRUE, (TEXT(" FTL Read Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" FTL Read Success \r\n")));
                MLC_Print_Page_Data(pMBuf, pSBuf);
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Sector Number ! You can read 0~%d sector \r\n"), (BLOCKS_PER_BANK*PAGES_PER_SUBLK-1)));
        }
    }
}
#endif

static void MLC_RAW_Erase_Test(void)
{
    DWORD dwNum;
    DWORD endblock = BANKS_TOTAL*BLOCKS_PER_BANK*(TWO_PLANE_PROGRAM+1);

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n RAW Erase Test \r\n")));
        OALMSG(TRUE, (TEXT(" Block Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum > 0 && dwNum < endblock)
        {
            if (MLC_Erase_RAW(0, dwNum))
            {
                OALMSG(TRUE, (TEXT(" RAW Erase Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" RAW Erase Success \r\n")));
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Block Number ! You can erase 1~%d block \r\n"), endblock-1));
        }
    }
}

static void MLC_RAW_Write_Test(void)
{
    DWORD dwNum;
    UINT32 iCnt;
    DWORD endblock = BANKS_TOTAL*BLOCKS_PER_BANK*(TWO_PLANE_PROGRAM+1);
    unsigned char *pMBuf;
    unsigned char *pSBuf;

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_PAGE;
//    memcpy((void *)pMBuf, (void *)InitialImage_rgb16_320x240, BYTES_PER_MAIN_PAGE);
    memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_PAGE);

//    RETAILMSG(1,(TEXT("~~~~~~~chadolp MLC %d %d\r\n"),BYTES_PER_MAIN_PAGE,BYTES_PER_SPARE_PAGE));

    for (iCnt=0; iCnt<BYTES_PER_MAIN_SUPAGE; iCnt++)
    {
        pMBuf[iCnt] = iCnt;
    }

    for (iCnt=0; iCnt<BYTES_PER_SPARE_PAGE; iCnt++)
    {
        pSBuf[iCnt] = iCnt;
    }

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n RAW Write Test \r\n")));
        OALMSG(TRUE, (TEXT(" Page Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum >= (PAGES_PER_BLOCK*2) && dwNum < (endblock*PAGES_PER_BLOCK))
        {
            if (MLC_Write_RAW(0, dwNum, pMBuf, pSBuf))
            {
                OALMSG(TRUE, (TEXT(" RAW Write Fail \r\n")));
            }
            else
            {
                OALMSG(TRUE, (TEXT(" RAW Write Success \r\n")));
            }
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Page Number ! You can write %d~%d page \r\n"), PAGES_PER_BLOCK*2, endblock*PAGES_PER_BLOCK-1));
        }
    }
}

static void MLC_RAW_Read_Test(void)
{
    DWORD dwNum;
    DWORD endblock = BANKS_TOTAL*BLOCKS_PER_BANK*(TWO_PLANE_PROGRAM+1);
    unsigned char *pMBuf;
    unsigned char *pSBuf;

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_PAGE;

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n RAW Read Test \r\n")));
        OALMSG(TRUE, (TEXT(" Page Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum >= 0 && dwNum < (endblock*PAGES_PER_BLOCK))
        {
            memset((void *)pMBuf, 0xff, BYTES_PER_MAIN_PAGE);
            memset((void *)pSBuf, 0xff, BYTES_PER_SPARE);

            MLC_Read_RAW(0, dwNum, pMBuf, pSBuf);
            MLC_Print_Page_Data(pMBuf, pSBuf);
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Page Number ! You can read 0~%d page \r\n"), endblock*PAGES_PER_BLOCK-1));
        }
    }
}

static void MLC_RAW_Find_MBR_Test(void)
{
    DWORD dwNum;
    DWORD endblock = BANKS_TOTAL*BLOCKS_PER_BANK*(TWO_PLANE_PROGRAM+1);
    unsigned char *pMBuf;
    unsigned char *pSBuf;
    unsigned char uLoc;

    pMBuf = WMRBuf;
    pSBuf = WMRBuf+BYTES_PER_MAIN_PAGE;

    for (dwNum=0; dwNum<(endblock*PAGES_PER_BLOCK-1); dwNum++)
    {
        uLoc = 0x0;
        
        if (dwNum%PAGES_PER_BLOCK == 0) OALMSG(TRUE, (TEXT("."), dwNum));

        memset((void *)pMBuf, 0xff, BYTES_PER_MAIN_PAGE);
        memset((void *)pSBuf, 0xff, BYTES_PER_SPARE_PAGE);

        MLC_Read_RAW(0, dwNum, pMBuf, pSBuf);

        if ((WMRBuf[0] == 0xe9) &&
             (WMRBuf[1] == 0xfd) &&
             (WMRBuf[2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR-1] == 0xaa)) uLoc = 0x1;
        if ((WMRBuf[BYTES_PER_SECTOR+0] == 0xe9) &&
             (WMRBuf[BYTES_PER_SECTOR+1] == 0xfd) &&
             (WMRBuf[BYTES_PER_SECTOR+2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR*2-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR*2-1] == 0xaa)) uLoc = 0x2;
        if ((WMRBuf[BYTES_PER_SECTOR*2+0] == 0xe9) &&
             (WMRBuf[BYTES_PER_SECTOR*2+1] == 0xfd) &&
             (WMRBuf[BYTES_PER_SECTOR*2+2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR*3-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR*3-1] == 0xaa)) uLoc = 0x4;
        if ((WMRBuf[BYTES_PER_SECTOR*3+0] == 0xe9) &&
             (WMRBuf[BYTES_PER_SECTOR*3+1] == 0xfd) &&
             (WMRBuf[BYTES_PER_SECTOR*3+2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR*4-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR*4-1] == 0xaa)) uLoc = 0x8;
        if ((WMRBuf[BYTES_PER_SECTOR*4+0] == 0xe9) &&
             (WMRBuf[BYTES_PER_SECTOR*4+1] == 0xfd) &&
             (WMRBuf[BYTES_PER_SECTOR*4+2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR*5-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR*5-1] == 0xaa)) uLoc = 0x10;
        if ((WMRBuf[BYTES_PER_SECTOR*5+0] == 0xe9) &&
             (WMRBuf[BYTES_PER_SECTOR*5+1] == 0xfd) &&
             (WMRBuf[BYTES_PER_SECTOR*5+2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR*6-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR*6-1] == 0xaa)) uLoc = 0x20;
        if ((WMRBuf[BYTES_PER_SECTOR*6+0] == 0xe9) &&
             (WMRBuf[BYTES_PER_SECTOR*6+1] == 0xfd) &&
             (WMRBuf[BYTES_PER_SECTOR*6+2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR*7-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR*7-1] == 0xaa)) uLoc = 0x40;
        if ((WMRBuf[BYTES_PER_SECTOR*7+0] == 0xe9) &&
             (WMRBuf[BYTES_PER_SECTOR*7+1] == 0xfd) &&
             (WMRBuf[BYTES_PER_SECTOR*7+2] == 0xff) &&
             (WMRBuf[BYTES_PER_SECTOR*8-2] == 0x55) &&
             (WMRBuf[BYTES_PER_SECTOR*8-1] == 0xaa)) uLoc = 0x80;

        if (uLoc & 0xff)
        {
            OALMSG(TRUE, (TEXT("\r\nMBR is found on Page %d (%x)\r\n"), dwNum, uLoc));
            MLC_Print_Page_Data(pMBuf, pSBuf);
			break;
        }
    }
}

static void MLC_Mark_BadBlock_Test(void)
{
    DWORD dwNum;
    LowFuncTbl *pLowFuncTbl;
    UINT32 nSyncRet;

    pLowFuncTbl = FIL_GetFuncTbl();

    while(1)
    {
        OALMSG(TRUE, (TEXT("\r\n Mark Bad Block \r\n")));
        OALMSG(TRUE, (TEXT(" Block Number [99999999 to exit] = ")));

        dwNum = GetDecimalNumber();

        if (dwNum == 99999999) break;
        else if (dwNum > 0 && dwNum < (SUBLKS_TOTAL*BANKS_TOTAL*(TWO_PLANE_PROGRAM+1)))
        {
            pLowFuncTbl->Erase(0, dwNum, enuBOTH_PLANE_BITMAP);
            if (pLowFuncTbl->Sync(0, &nSyncRet))
            {
                OALMSG(TRUE, (TEXT(" FIL Erase Fail \r\n")));
                return;
            }

            memset(WMRBuf, 0x00, BYTES_PER_SPARE_SUPAGE);
            IS_CHECK_SPARE_ECC = FALSE32;
            pLowFuncTbl->Write(0, dwNum*PAGES_PER_BLOCK+PAGES_PER_BLOCK-1, 0x00, enuBOTH_PLANE_BITMAP, NULL, WMRBuf);
            IS_CHECK_SPARE_ECC = TRUE32;

            OALMSG(TRUE, (TEXT(" Mark Bad Block Success \r\n")));
        }
        else
        {
            OALMSG(TRUE, (TEXT(" Wrong Block Number ! You can mark 1~%d block \r\n"), SUBLKS_TOTAL-1));
        }
    }
}

void MLC_LowLevelTest(void)
{
    int iNum;

    while (1)
    {
        OALMSG(TRUE, (TEXT("\r\n====================\r\n")));
        OALMSG(TRUE, (TEXT(" MLC Low Level Test \r\n")));
        OALMSG(TRUE, (TEXT("  1. FIL Erase Test \r\n")));
        OALMSG(TRUE, (TEXT("  2. FIL Write Test \r\n")));
        OALMSG(TRUE, (TEXT("  3. FIL Read Test \r\n")));
        OALMSG(TRUE, (TEXT("  4. VFL Erase Test \r\n")));
        OALMSG(TRUE, (TEXT("  5. VFL Write Test \r\n")));
        OALMSG(TRUE, (TEXT("  6. VFL Read Test \r\n")));
        OALMSG(TRUE, (TEXT("  7. RAW Erase Test \r\n")));
        OALMSG(TRUE, (TEXT("  8. RAW Write Test \r\n")));
        OALMSG(TRUE, (TEXT("  9. RAW Read Test \r\n")));
        OALMSG(TRUE, (TEXT(" 10. Mark Bad Block \r\n")));
        OALMSG(TRUE, (TEXT(" 11. Find MBR Page \r\n")));
#if FTL_TEST
        OALMSG(TRUE, (TEXT(" 12. FTL Write Test \r\n")));
        OALMSG(TRUE, (TEXT(" 13. FTL Read Test \r\n")));
#endif
        OALMSG(TRUE, (TEXT("  0. Exit Test \r\n")));
        OALMSG(TRUE, (TEXT("====================\r\n")));

        OALMSG(TRUE, (TEXT("Select : ")));
        iNum = GetDecimalNumber();

        if (iNum == 0) break;
        else if (iNum == 1) MLC_FIL_Erase_Test();
        else if (iNum == 2) MLC_FIL_Write_Test();
        else if (iNum == 3) MLC_FIL_Read_Test();
        else if (iNum == 4) MLC_VFL_Erase_Test();
        else if (iNum == 5) MLC_VFL_Write_Test();
        else if (iNum == 6) MLC_VFL_Read_Test();
        else if (iNum == 7) MLC_RAW_Erase_Test();
        else if (iNum == 8) MLC_RAW_Write_Test();
        else if (iNum == 9) MLC_RAW_Read_Test();
        else if (iNum == 10) MLC_Mark_BadBlock_Test();
        else if (iNum == 11) MLC_RAW_Find_MBR_Test();
#if FTL_TEST
        else if (iNum == 12) MLC_FTL_Write_Test();
        else if (iNum == 13) MLC_FTL_Read_Test();
#endif
        else OALMSG(TRUE, (TEXT("Wrong selection\r\n")));
    }
}
