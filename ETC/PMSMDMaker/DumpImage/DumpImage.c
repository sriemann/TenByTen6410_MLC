// DumpImage.c : Dump the TOC, OS and FS from MLC NANDFlash.
//

#pragma optimize("", off)

#include <windows.h>
#include <nkintr.h>
#include <stdio.h>
#include <bsp.h>
#include "VFLBuffer.h"
#include "WMRTypes.h"
#include "VFL.h"
#include "HALWrapper.h"


#define DUMPTOC
//#define DUMPOS
//#define DUMPFS

//#define K9G8G08
//#define K9LAG08
//#define K9GAG08_4BITECC
//#define K9GAG08_8BITECC

#define K9GAG08_16BITECC
//#define K9GBG08_16BITECC

//#define K9HAG08
//#define K9LBG08_4BITECC
//#define K9LBG08_8BITECC


#define TOC_AREA_START_L        (1) /* TOC Start Block */
#define TOC_AREA_SIZE_L         (1)

#ifdef K9G8G08
#define PAGESIZE2K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (100)
#define FTL_AREA_START_L        (216)
#define FTL_AREA_SIZE_L         (1832)
#define PAGES_PER_SUBLK_L       (128)

#define SECTORS_PER_PAGE_L      (4)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (64)

#define USE2PLANE_L
#endif

#ifdef K9LAG08
#define PAGESIZE2K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (50)
#define FTL_AREA_START_L        (166)
#define FTL_AREA_SIZE_L         (1882)
#define PAGES_PER_SUBLK_L       (256)

#define SECTORS_PER_PAGE_L      (4)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (64)

#define USE2PLANE_L
#endif

#ifdef K9HAG08
#define PAGESIZE2K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (25)
#define FTL_AREA_START_L        (90)
#define FTL_AREA_SIZE_L         (934)
#define PAGES_PER_SUBLK_L       (512)

#define SECTORS_PER_PAGE_L      (4)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (128)

#define USE2PLANE_L
#endif

#ifdef K9GAG08_4BITECC
#define PAGESIZE4K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (100)
#define FTL_AREA_START_L        (216)
#define FTL_AREA_SIZE_L         (1832)
#define PAGES_PER_SUBLK_L       (128)

#define SECTORS_PER_PAGE_L      (8)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (128)

#define USE2PLANE_L
#endif

#ifdef K9GAG08_8BITECC
#define PAGESIZE4K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (100)
#define FTL_AREA_START_L        (216)
#define FTL_AREA_SIZE_L         (1832)
#define PAGES_PER_SUBLK_L       (128)

#define SECTORS_PER_PAGE_L      (8)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (256)

#define USE2PLANE_L
#endif

#ifdef K9GAG08_16BITECC
#define PAGESIZE8K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (200)
#define FTL_AREA_START_L        (317)
#define FTL_AREA_SIZE_L         (1759)
#define PAGES_PER_SUBLK_L       (128)

#define SECTORS_PER_PAGE_L      (16)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (436)
#endif

#ifdef K9GBG08_16BITECC
#define PAGESIZE8K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (100)
#define FTL_AREA_START_L        (217)
#define FTL_AREA_SIZE_L         (1859)
#define PAGES_PER_SUBLK_L       (128)

#define SECTORS_PER_PAGE_L      (16)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (436)

#define USE2PLANE_L
#endif

#ifdef K9LBG08_4BITECC
#define PAGESIZE4K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (100)
#define FTL_AREA_START_L        (216)
#define FTL_AREA_SIZE_L         (1822)
#define PAGES_PER_SUBLK_L       (256)

#define SECTORS_PER_PAGE_L      (8)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (128)

#define USE2PLANE_L
#endif

#ifdef K9LBG08_8BITECC
#define PAGESIZE4K
#define SPECIAL_AREA_START_L    (10)
#define SPECIAL_AREA_SIZE_L     (50)
#define FTL_AREA_START_L        (166)
#define FTL_AREA_SIZE_L         (1882)
#define PAGES_PER_SUBLK_L       (256)

#define SECTORS_PER_PAGE_L      (8)
#define BYTES_PER_SECTOR_L      (512)
#define BYTES_PER_SPARE_PAGE_L  (256)

#define USE2PLANE_L
#endif


void ReadPage(DWORD nVpn, unsigned char *pData, unsigned char *pSpare)
{
    Buffer      pBuf = {0,};
    VFLPacket   stPacket = {0,};
    UINT32      nResult = 1;
    BOOL        bRet = FALSE;
    DWORD       dwLastErr = 0;

#if defined(PAGESIZE8K)         // 8KB page
    #if defined(USE2PLANE_L)    // 2-plane
        pBuf.nBitmap = 0xFFFFFFFF;
    #else                       // 1-plane
        pBuf.nBitmap = 0xFFFF;
    #endif

#elif defined(PAGESIZE4K)       // 4KB page
    #if defined(USE2PLANE_L)    // 2-plane
        pBuf.nBitmap = 0xFFFF;
    #else                       // 1-plane
        pBuf.nBitmap = 0xFF;
    #endif

#elif defined(PAGESIZE2K)       // 2KB page
    #if defined(USE2PLANE_L)    // 2-plane
        pBuf.nBitmap = 0xFF;
    #else                       // 1-plane
        pBuf.nBitmap = 0xF;
    #endif

#endif

    pBuf.eStatus = BUF_AUX;
    pBuf.nBank = 0;

    // Read Main Data Area
    pBuf.pData = pData;
    pBuf.pSpare = NULL;

    do {
        /* VFL_Read */
        stPacket.nCtrlCode  = PM_HAL_VFL_READ;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = nVpn;
        stPacket.pBuf       = &pBuf;
        stPacket.nSrcVpn    = 0;
        stPacket.nDesVpn    = 0;
        stPacket.bCleanCheck= FALSE32;
        bRet = KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                                &stPacket,                /* Input buffer (Additional Control Code) */
                                sizeof(VFLPacket),        /* Size of Input buffer */
                                NULL,                     /* Output buffer */
                                0,                        /* Size of Output buffer */
                                &nResult);                /* Error Return */
        if ((!bRet) || (nResult != VFL_SUCCESS))
        {
            dwLastErr = GetLastError();
            RETAILMSG(1, (TEXT("[VFLP:ERR]  VFL_Read(Main) failure. ERR Code(0x%x), LastError=(0x%x)\r\n"), nResult, dwLastErr));
            break;
        }
    } while(0);

    // Read Spare Area
    pBuf.pData = NULL;
    pBuf.pSpare = pSpare;

    do {
        /* VFL_Read */
        stPacket.nCtrlCode  = PM_HAL_VFL_READ;
        stPacket.nVbn       = 0;            // Not used
        stPacket.nVpn       = nVpn;
        stPacket.pBuf       = &pBuf;
        stPacket.nSrcVpn    = 0;
        stPacket.nDesVpn    = 0;
        stPacket.bCleanCheck= FALSE32;

        bRet = KernelIoControl(IOCTL_POCKETSTOREII_CMD,  /* IO Control Code */
                                &stPacket,                /* Input buffer (Additional Control Code) */
                                sizeof(VFLPacket),        /* Size of Input buffer */
                                NULL,                     /* Output buffer */
                                0,                        /* Size of Output buffer */
                                &nResult);                /* Error Return */

        if ((!bRet) || (nResult != VFL_SUCCESS))
        {
            dwLastErr = GetLastError();
            RETAILMSG(1, (TEXT("[VFLP:ERR]  VFL_Read(Spare) failure. ERR Code(0x%x), LastError=(0x%x)\r\n"), nResult, dwLastErr));
            break;
        }
    } while(0);

}


BOOL32 CheckAllFF(unsigned char * pMBuf, DWORD size)
{
    DWORD i = 0;

    for (i = 0; i < size; i++)
    {
        if ((*(pMBuf + i)) == 0xFF)
        {
            continue;
        }
        else
        {
            return FALSE32;
        }
    }
    return TRUE32;
}


int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{
    DWORD nVpn = 0;

    FILE    *pFileOutPtr = NULL;

    int     nFileLength = 0;

    char    *cBuffer = NULL;
    DWORD   dwTmpPer=0;
    static  DWORD   dwPer=0xFF;

#if defined(USE2PLANE_L)
    static  UINT8   pData[(SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L) * 2] = {0,};
    static  UINT8   pSpare[BYTES_PER_SPARE_PAGE_L * 2] = {0,};
#else
    static  UINT8   pData[(SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L)] = {0,};
    static  UINT8   pSpare[BYTES_PER_SPARE_PAGE_L] = {0,};
#endif

#if (defined(DUMPOS) || defined(DUMPFS)) && (defined(K9GAG08_16BITECC) || defined(K9GBG08_16BITECC))
#if defined(USE2PLANE_L)
    static  UINT8   pTempSpare[520 * 2] = {0,};     // in case of 4KB page sized MLC NANDFlash, converting from 520B to 436B is needed.
#else
    static  UINT8   pTempSpare[520] = {0,};
#endif
    static UINT8 cBitAnd = 0;
    static UINT32 nTemp = 0xffffffff;
#endif

    DWORD   dwTOCStartPage = 0;
    DWORD   dwTOCEndPage = 0;
    DWORD   dwOSStartPage = 0;
    DWORD   dwOSEndPage = 0;
    DWORD   dwFTLStartPage = 0;
    DWORD   dwFTLEndPage = 0;

    int i = 0;


    RETAILMSG(1, (TEXT("\r\n\r\n========== [SMD dumpimage, build date:%s] ===========\r\n"), _T(__DATE__) ));

#ifdef DUMPTOC
    // Extract TOC Image routine...

    RETAILMSG(1, (TEXT("Dump TOC Image\r\n")));

    pFileOutPtr = fopen("Storage Card/TOC.bin", "wb");

    if (!pFileOutPtr)
    {
        RETAILMSG(1, (TEXT("PocketMory1/TOC.bin file is not opened.!!!\r\n")));
        goto Fail;
    }

    fseek(pFileOutPtr, 0, SEEK_SET);

    dwTOCStartPage = TOC_AREA_START_L * PAGES_PER_SUBLK_L;
    dwTOCEndPage = ((TOC_AREA_START_L + TOC_AREA_SIZE_L) * PAGES_PER_SUBLK_L) - 1;

    RETAILMSG(1, (TEXT("TOC Area is from %d to %d\r\n"), dwTOCStartPage, dwTOCEndPage));
    RETAILMSG(1, (TEXT("Scanning...\r\n")));

    // Scan All FF area
    for (nVpn = dwTOCEndPage; nVpn > dwTOCStartPage; nVpn--)
    {
        memset(pData, 0xFF, sizeof(pData));
        memset(pSpare, 0xFF, sizeof(pSpare));
        ReadPage(nVpn, pData, pSpare);
        // Check only 4 bytes for spare area.
        if ((CheckAllFF(pData, sizeof(pData)) == TRUE32) && (CheckAllFF(pSpare, 4) == TRUE32)) // TRUE32 means All FF
        {
            continue;
        }
        else
        {
            break;
        }
    }
    dwTOCEndPage = nVpn + 1;

    RETAILMSG(1, (TEXT("Read page from %d to %d\r\n"), dwTOCStartPage, dwTOCEndPage));

    dwPer = 0xFF; /* Percent Initial */
    for(nVpn = dwTOCStartPage; nVpn < dwTOCEndPage; )
    {
        dwTmpPer = ((nVpn - dwTOCStartPage) * 100) / (dwTOCEndPage - dwTOCStartPage);
        if(dwPer != dwTmpPer)
        {
            RETAILMSG(1, (TEXT(" %02d Percent Completed"), dwTmpPer ));
        }

        memset(pData, 0xFF, sizeof(pData));
        memset(pSpare, 0xFF, sizeof(pSpare));

        ReadPage(nVpn, pData, pSpare);

        if(fwrite(pData, sizeof(char), BYTES_PER_SECTOR_L, pFileOutPtr) == BYTES_PER_SECTOR_L)
            break;

        nVpn += 4;
        if((dwPer != dwTmpPer) && (99 != dwTmpPer))
        {
            RETAILMSG(1,(TEXT("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b")));
        }
        dwPer = dwTmpPer;
    }

    if (pFileOutPtr) fclose(pFileOutPtr);

    RETAILMSG(1,(TEXT("\r\nDump TOC Image Finished\r\n")));

#endif  // DUMPTOC


#ifdef DUMPOS
    // Extract OS Image routine...

    RETAILMSG(1, (TEXT("Dump OS Image\r\n")));

    pFileOutPtr = fopen("Storage Card/OSIMG.IMG", "wb");
    if (!pFileOutPtr)
    {
        RETAILMSG(1, (TEXT("Storage Card/OSIMG.IMG file is not opened.!!!\r\n")));
        goto Fail;
    }

    fseek(pFileOutPtr, 0, SEEK_SET);

    dwOSStartPage = SPECIAL_AREA_START_L * PAGES_PER_SUBLK_L;
    dwOSEndPage = ((SPECIAL_AREA_START_L + SPECIAL_AREA_SIZE_L) * PAGES_PER_SUBLK_L) - 1;

    RETAILMSG(1, (TEXT("OS Area is from %d to %d\r\n"), dwOSStartPage, dwOSEndPage));
    RETAILMSG(1, (TEXT("Scanning...\r\n")));

    // Scan All FF area
    for (nVpn = dwOSEndPage; nVpn > dwOSStartPage; nVpn--)
    {
        if ((nVpn % 128) == 0)
        {
            RETAILMSG(0, (TEXT("%d:\r\n"), nVpn));
            RETAILMSG(1, (TEXT(".")));
        }

        memset(pData, 0xFF, sizeof(pData));
        memset(pSpare, 0xFF, sizeof(pSpare));

        ReadPage(nVpn, pData, pSpare);

        // Check only 4 bytes for spare area.
        if ((CheckAllFF(pData, sizeof(pData)) == TRUE32) && (CheckAllFF(pSpare, 4) == TRUE32)) // TRUE32 means All FF
        {
            continue;
        }
        else
        {
            break;
        }
    }
    dwOSEndPage = nVpn + 1;

    RETAILMSG(1, (TEXT("Read Page from %d to %d\r\n"), dwOSStartPage, dwOSEndPage));

    dwPer = 0xFF; /* Percent Initial */
    for( nVpn = dwOSStartPage; nVpn < dwOSEndPage; nVpn++ )
    {
        dwTmpPer = ((nVpn - dwOSStartPage) * 100) / (dwOSEndPage - dwOSStartPage);
        if(dwPer != dwTmpPer)
        {
            RETAILMSG(1, (TEXT(" %02d Percent Completed"), dwTmpPer ));
        }

        memset(pData, 0xFF, sizeof(pData));
        memset(pSpare, 0xFF, sizeof(pSpare));

#if defined(K9GAG08_16BITECC) || defined(K9GBG08_16BITECC)
        memset(pTempSpare, 0xFF, sizeof(pTempSpare));
        ReadPage(nVpn, pData, pTempSpare);

        // Spare area conversion from 520B to 436B
        //
        // The left plane(or single plane)
        memcpy(pSpare, pTempSpare, sizeof(UINT8));  // 1 byte Bad Mark
        memcpy((pSpare + 1), (pTempSpare + 1), sizeof(UINT8));  // 1 byte Clean Mark (3B -> 1B)
        memcpy((pSpare + 1 + 1), (pTempSpare + 1 + 3), (sizeof(UINT8) * 4));  // 4 byte SOffset

        nTemp = 0xffffffff;

#ifdef USE2PLANE_L
        for (i = 0; i < ((SECTORS_PER_PAGE_L * 2) / sizeof(UINT32)); i++)
#else
        for (i = 0; i < (SECTORS_PER_PAGE_L / sizeof(UINT32)); i++)
#endif
        {
            nTemp &= *((UINT32 *)(pTempSpare + 8) + i);
        }
        cBitAnd = (UINT8)(((nTemp & 0xff) & ((nTemp>>8) & 0xff) & ((nTemp>>16) & 0xff) & ((nTemp>>24) & 0xff))) & 0xff;
        memcpy((pSpare + 1 + 1 + 4), &cBitAnd, sizeof(UINT8));  // 1 byte Run Mark(SECTORS_PER_PAGE_L(16B or 32B) -> 1B)

#ifdef USE2PLANE_L
        for (i = 0; i < SECTORS_PER_PAGE_L; i++)
        {
            memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * i)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * i)), 26);  // Sector ECC(26B x 16)
        }

        memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * SECTORS_PER_PAGE_L)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * SECTORS_PER_PAGE_L)), 13);  // Spare ECC(13B)
#else
        for (i = 0; i < SECTORS_PER_PAGE_L; i++)
        {
            memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * i)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L) + (28 * i)), 26);      // Sector ECC(26B x 16)
        }

        memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * SECTORS_PER_PAGE_L)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L) + (28 * SECTORS_PER_PAGE_L)), 13);    // Spare ECC(13B)
#endif

        // The right plane
#ifdef USE2PLANE_L
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L), (pTempSpare + 520), sizeof(UINT8));  // 1 byte Bad Mark
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1), (pTempSpare + 520 + 1), sizeof(UINT8));  // 1 byte Clean Mark (3B -> 1B)
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1), (pTempSpare + 520 + 1 + 3), (sizeof(UINT8) * 4));  // 4 byte SOffset

        nTemp = 0xffffffff;

        for (i = 0; i < ((SECTORS_PER_PAGE_L * 2) / sizeof(UINT32)); i++)
        {
            nTemp &= *((UINT32 *)(pTempSpare + 520 + 8) + i);
        }
        cBitAnd = (UINT8)(((nTemp & 0xff) & ((nTemp>>8) & 0xff) & ((nTemp>>16) & 0xff) & ((nTemp>>24) & 0xff))) & 0xff;
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1 + 4), &cBitAnd, sizeof(UINT8));  // 1 byte Run Mark(SECTORS_PER_PAGE_L(16B or 32B) -> 1B)

        for (i = 0; i < SECTORS_PER_PAGE_L; i++)
        {
            memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1 + 4 + 1 + (26 * i)), (pTempSpare + 520 + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * i)), 26);   // Sector ECC(26B x 16)
        }

        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1 + 4 + 1 + (26 * SECTORS_PER_PAGE_L)), (pTempSpare + 520 + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * SECTORS_PER_PAGE_L)), 13); // Spare ECC(13B)
#endif

#else
        ReadPage(nVpn, pData, pSpare);
#endif

#ifdef USE2PLANE_L
        // Write First plane
        //RETAILMSG(1,(TEXT("Write First plane : page number (%d) mem (0x%x)\r\n"), nVpn*2, (nVpn-dwOSStartPage)*(2048+64)));
        fwrite(pData, sizeof(char), (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), pFileOutPtr);
        fwrite(pSpare, sizeof(char), BYTES_PER_SPARE_PAGE_L, pFileOutPtr);

        // Write Second plane
        //RETAILMSG(1,(TEXT("Write Second plane : page number (%d) mem (0x%x)\r\n"), nVpn*2+128, (nVpn-dwOSStartPage+128)*(2048+64)));
        fwrite(pData + (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), sizeof(char), (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), pFileOutPtr);
        fwrite(pSpare + BYTES_PER_SPARE_PAGE_L, sizeof(char), BYTES_PER_SPARE_PAGE_L, pFileOutPtr);
#else
        // Write First plane
        //RETAILMSG(1,(TEXT("Write First plane : page number (%d) mem (0x%x)\r\n"), nVpn*2, (nVpn-dwOSStartPage)*(2048+64)));
        fwrite(pData, sizeof(char), (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), pFileOutPtr);
        fwrite(pSpare, sizeof(char), (BYTES_PER_SPARE_PAGE_L), pFileOutPtr);
#endif

        if((dwPer != dwTmpPer) && (99 != dwTmpPer))
        {
            RETAILMSG(1, (TEXT("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b")));
        }
        dwPer = dwTmpPer;
    }

    if (pFileOutPtr) fclose(pFileOutPtr);

    RETAILMSG(1,(TEXT("Dump OS Image Finished\r\n")));

#endif  // DUMPOS


#ifdef DUMPFS
    // Extract FileSystem Image routine...

    RETAILMSG(1, (TEXT("Dump FTL Image\r\n")));

    pFileOutPtr = fopen("Storage Card/FTLIMG.IMG", "wb");
    if (!pFileOutPtr)
    {
        RETAILMSG(1, (TEXT("Storage Card/FTLIMG.IMG file is not opened.!!!\r\n")));
        goto Fail;
    }

    fseek(pFileOutPtr, 0, SEEK_SET);

    dwFTLStartPage = FTL_AREA_START_L * PAGES_PER_SUBLK_L;
    dwFTLEndPage = ((FTL_AREA_START_L + FTL_AREA_SIZE_L) * PAGES_PER_SUBLK_L) - 1;

    RETAILMSG(1, (TEXT("FTL Area is from %d to %d\r\n"), dwFTLStartPage, dwFTLEndPage));
    RETAILMSG(1, (TEXT("Scanning...\r\n")));

    // Scan All FF area
    for (nVpn = dwFTLEndPage; nVpn > dwFTLStartPage; nVpn--)
    {
        if ((nVpn % 128) == 0)
        {
            RETAILMSG(0, (TEXT("%d:\r\n"), nVpn));
            RETAILMSG(1, (TEXT(".")));
        }

        memset(pData, 0xFF, sizeof(pData));
        memset(pSpare, 0xFF, sizeof(pSpare));

        ReadPage( nVpn, pData, pSpare );

        // Check only 4 bytes for spare area.
        if ((CheckAllFF(pData, sizeof(pData)) == TRUE32) && (CheckAllFF(pSpare, 4) == TRUE32) ) // TRUE32 means All FF
        {
            continue;
        }
        else
        {
            break;
        }
	}

    dwFTLEndPage = nVpn + 1;

    RETAILMSG(1, (TEXT("Read Page from %d to %d\r\n"), dwFTLStartPage, dwFTLEndPage));

    dwPer = 0xFF; /* Percent Initial */
    for(nVpn = dwFTLStartPage; nVpn < dwFTLEndPage; nVpn++)
    {
        dwTmpPer = ((nVpn - dwFTLStartPage) * 100) / (dwFTLEndPage - dwFTLStartPage);
        if( dwPer != dwTmpPer )
        {
        	RETAILMSG(1, (TEXT(" %02d Percent Completed"), dwTmpPer ));
        }

        memset(pData, 0xFF, sizeof(pData));
        memset(pSpare, 0xFF, sizeof(pSpare));

#if defined(K9GAG08_16BITECC) || defined(K9GBG08_16BITECC)
        memset(pTempSpare, 0xFF, sizeof(pTempSpare));
        ReadPage(nVpn, pData, pTempSpare);

        // Spare area conversion from 520B to 436B
        //
        // The left plane(or single plane)
        memcpy(pSpare, pTempSpare, sizeof(UINT8));  // 1 byte Bad Mark
        memcpy((pSpare + 1), (pTempSpare + 1), sizeof(UINT8));  // 1 byte Clean Mark (3B -> 1B)
        memcpy((pSpare + 1 + 1), (pTempSpare + 1 + 3), (sizeof(UINT8) * 4));  // 4 byte SOffset

        nTemp = 0xffffffff;

#ifdef USE2PLANE_L
        for (i = 0; i < ((SECTORS_PER_PAGE_L * 2) / sizeof(UINT32)); i++)
#else
        for (i = 0; i < (SECTORS_PER_PAGE_L / sizeof(UINT32)); i++)
#endif
        {
            nTemp &= *((UINT32 *)(pTempSpare + 8) + i);
        }
        cBitAnd = (UINT8)(((nTemp & 0xff) & ((nTemp>>8) & 0xff) & ((nTemp>>16) & 0xff) & ((nTemp>>24) & 0xff))) & 0xff;
        memcpy((pSpare + 1 + 1 + 4), &cBitAnd, sizeof(UINT8));  // 1 byte Run Mark(SECTORS_PER_PAGE_L(16B or 32B) -> 1B)

#ifdef USE2PLANE_L
        for (i = 0; i < SECTORS_PER_PAGE_L; i++)
        {
            memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * i)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * i)), 26);    // Sector ECC(26B x 16)
        }

        memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * SECTORS_PER_PAGE_L)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * SECTORS_PER_PAGE_L)), 13);  // Spare ECC(13B)
#else
        for (i = 0; i < SECTORS_PER_PAGE_L; i++)
        {
            memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * i)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L) + (28 * i)), 26);    // Sector ECC(26B x 16)
        }

        memcpy((pSpare + 1 + 1 + 4 + 1 + (26 * SECTORS_PER_PAGE_L)), (pTempSpare + 1 + 3 + 4 + (SECTORS_PER_PAGE_L) + (28 * SECTORS_PER_PAGE_L)), 13);    // Spare ECC(13B)
#endif

        // The right plane
#ifdef USE2PLANE_L
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L), (pTempSpare + 520), sizeof(UINT8));  // 1 byte Bad Mark
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1), (pTempSpare + 520 + 1), sizeof(UINT8));  // 1 byte Clean Mark (3B -> 1B)
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1), (pTempSpare + 520 + 1 + 3), (sizeof(UINT8) * 4));  // 4 byte SOffset

        nTemp = 0xffffffff;

        for (i = 0; i < ((SECTORS_PER_PAGE_L * 2) / sizeof(UINT32)); i++)
        {
            nTemp &= *((UINT32 *)(pTempSpare + 520 + 8) + i);
        }
        cBitAnd = (UINT8)(((nTemp & 0xff) & ((nTemp>>8) & 0xff) & ((nTemp>>16) & 0xff) & ((nTemp>>24) & 0xff))) & 0xff;
        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1 + 4), &cBitAnd, sizeof(UINT8));  // 1 byte Run Mark(SECTORS_PER_PAGE_L(16B or 32B) -> 1B)

        for (i = 0; i < SECTORS_PER_PAGE_L; i++)
        {
            memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1 + 4 + 1 + (26 * i)), (pTempSpare + 520 + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * i)), 26); // Sector ECC(26B x 16)
        }

        memcpy((pSpare + BYTES_PER_SPARE_PAGE_L + 1 + 1 + 4 + 1 + (26 * SECTORS_PER_PAGE_L)), (pTempSpare + 520 + 1 + 3 + 4 + (SECTORS_PER_PAGE_L * 2) + (28 * SECTORS_PER_PAGE_L)), 13); // Spare ECC(13B)
#endif

#else
        ReadPage(nVpn, pData, pSpare);
#endif

#ifdef USE2PLANE_L
        // Write First plane
        //RETAILMSG(1,(TEXT("Write First plane : page number (%d) mem (0x%x)\r\n"), nVpn*2, (nVpn-dwOSStartPage)*(2048+64)));
        fwrite(pData, sizeof(char), (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), pFileOutPtr);
        fwrite(pSpare, sizeof(char), BYTES_PER_SPARE_PAGE_L, pFileOutPtr);

        // Write Second plane
        //RETAILMSG(1,(TEXT("Write Second plane : page number (%d) mem (0x%x)\r\n"), nVpn*2+128, (nVpn-dwOSStartPage+128)*(2048+64)));
        fwrite(pData + (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), sizeof(char), (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), pFileOutPtr);
        fwrite(pSpare + (BYTES_PER_SPARE_PAGE_L), sizeof(char), (BYTES_PER_SPARE_PAGE_L), pFileOutPtr);
#else
        // Write First plane
        //RETAILMSG(1,(TEXT("Write First plane : page number (%d) mem (0x%x)\r\n"), nVpn*2, (nVpn-dwOSStartPage)*(2048+64)));
        fwrite(pData, sizeof(char), (SECTORS_PER_PAGE_L * BYTES_PER_SECTOR_L), pFileOutPtr);
        fwrite(pSpare, sizeof(char), BYTES_PER_SPARE_PAGE_L, pFileOutPtr);
#endif
        if((dwPer != dwTmpPer) && (99 != dwTmpPer))
        {
            RETAILMSG(1,(TEXT("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b")));
        }
        dwPer = dwTmpPer;
    }

    if (pFileOutPtr) fclose(pFileOutPtr);

    RETAILMSG(1, (TEXT("Dump FTL Image Finished\r\n")));

#endif  // DUMPFS


    return 0;

Fail:
    RETAILMSG(1, (TEXT("WMR_RW_test is failed.!!\r\n")));
    if (pFileOutPtr) fclose(pFileOutPtr);
    return 0;

}


#pragma optimize("", on)
