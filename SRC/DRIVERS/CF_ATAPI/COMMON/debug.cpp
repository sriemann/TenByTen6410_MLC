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
#include <windows.h>
#include <debug.h>

#ifdef DEBUG

void DumpRegKey(DWORD dwZone, PTSTR szKey, HKEY hKey)
{
    DWORD dwIndex = 0;
    WCHAR szValueName[MAX_PATH];
    DWORD dwValueNameSize = MAX_PATH;
    BYTE  pValueData[256];
    DWORD dwType;
    DWORD dwValueDataSize = sizeof(pValueData);
    DEBUGMSG(dwZone, (TEXT("Atapi!DumpRegKey> %s \r\n"), szKey));
    while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szValueName, &dwValueNameSize, NULL, &dwType, pValueData, &dwValueDataSize)) {
        if (REG_SZ == dwType) {
            DEBUGMSG(dwZone, (TEXT("\t\t%s = %s\r\n"), szValueName, (LPWSTR)pValueData));
        }
        else if (REG_DWORD == dwType) {
            DEBUGMSG(dwZone, (TEXT("\t\t%s = %08X\r\n"), szValueName, *(PDWORD)pValueData));
        }
        else if (REG_MULTI_SZ == dwType) {
            PWSTR pValueTemp = (PWSTR)pValueData;
            DEBUGMSG(dwZone, (TEXT("\t\t%s :\r\n"), szValueName));
            while (*pValueTemp) {
                DEBUGMSG(dwZone, (TEXT("\t\t\t%s\r\n"), (LPWSTR)pValueTemp));
                pValueTemp += (wcslen(pValueTemp) + 1);
            }
        }
        dwIndex++;
        dwValueDataSize = sizeof(pValueData);
        dwValueNameSize = MAX_PATH;
    }
}

VOID DumpIdentify(PIDENTIFY_DATA pId)
{
    if (pId->GeneralConfiguration & 0x0400) {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> Disk transfer rate > 10Mbs\r\n")));
    }
    if (pId->GeneralConfiguration & 0x0200) {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> Disk transfer rate > 5Mbs and <= 10Mbs\r\n")));
    }
    if (pId->GeneralConfiguration & 0x0100) {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> Disk transfer rate <= 5Mbs\r\n")));
    }
    if (pId->Capabilities & 0x0100) {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> DMA supported\r\n")));
    }
    else {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> DMA not supported\r\n")));
    }
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> (The following data may be read from an obsolete INDENTIFY DEVICE address)\r\n")));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d cylinders\r\n"), pId->NumberOfCylinders));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d heads\r\n"), pId->NumberOfHeads));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d unformatted bytes per track\r\n"), pId->UnformattedBytesPerTrack));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d unformatted bytes per sector\r\n"), pId->UnformattedBytesPerSector));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d sectors per track\r\n"), pId->SectorsPerTrack));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> (The following data may be read from an obsolete INDENTIFY DEVICE address)\r\n")));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d cylinders\r\n"), pId->NumberOfCurrentCylinders));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d heads\r\n"), pId->NumberOfCurrentHeads));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d sectors per track\r\n"), pId->CurrentSectorsPerTrack));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d total sectors\r\n"), pId->CurrentSectorCapacity));
    DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d total user-addressable sectors\r\n"), pId->TotalUserAddressableSectors));
    if (pId->MaximumBlockTransfer == 0) {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> READ/WRITE MULTIPLE not supported\r\n")));
    }
    else {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d sectors in maximum DRQ data block (READ/WRITE MULTIPLE sectors per interrupt)\r\n"), pId->MaximumBlockTransfer));
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d sectors in current DRQ data block (READ/WRITE MULTIPLE sectors per interrupt) [%s] \r\n"), pId->MultiSectorCount, (pId->MultiSectorSettingValid & 1) ? (_T("valid")) : (_T("invalid"))));
    }
    if (pId->BufferSectorSize == 0) {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> Unspecified on-disk cache size\r\n")));
    }
    else {
        DEBUGMSG(ZONE_INIT, (TEXT("Atapi!CDisk::Identify> %d sector on-disk cache\r\n"), pId->BufferSectorSize));
    }
}

#define TM_PIO           0x08
#define TM_MULTIWORD_DMA 0x20
#define TM_ULTRA_DMA     0x40

void
DumpSupportedTransferModes(
    PIDENTIFY_DATA pId
    )
{
    BOOL fDMASupported = TRUE; // whether UDMA or Multiword DMA is supported
    // TranslationFieldsValid <=> Word 53
    if (pId->TranslationFieldsValid & 0x0002) {
        // Word 64; determine "best" supported PIO mode
        if (pId->AdvancedPIOxferreserved & 0x01) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> PIO mode 0 supported\r\n")));
        }
        if (pId->AdvancedPIOxferreserved & 0x02) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> PIO mode 1 supported\r\n")));
        }
        if (pId->AdvancedPIOxferreserved & 0x04) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> PIO mode 2 supported\r\n")));
        }
        if (pId->AdvancedPIOxferreserved & 0x08) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> PIO mode 3 supported\r\n")));
        }
        if (pId->AdvancedPIOxferreserved & 0x10) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> PIO mode 4 supported\r\n")));
        }
        if (!(pId->AdvancedPIOxferreserved & 0x1F)) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> PIO mode not supported\r\n")));
        }
    }

    // MultiDMAModesSupported <=> Word 63, low-byte
    // determine "best" supported Multiword DMA mode
    if (pId->MultiDmaModesSupported & 0x04) {
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Multiword DMA mode <= 2 are supported\r\n")));
    }
    else if (pId->MultiDmaModesSupported & 0x02) {
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Multiword DMA mode <= 1 are supported\r\n")));
    }
    else if (pId->MultiDmaModesSupported & 0x01) {
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Multiword DMA mode 0 is supported\r\n")));
    }
    else {
        fDMASupported = FALSE;
    }
    if (fDMASupported) {
        // MultiDmaTransferActive <=> Word 64, high-byte; dump selected Multiword DMA modes
        if (pId->MultiDmaTransferActive & 0x04) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Multiword DMA mode 2 is active\r\n")));
        }
        if (pId->MultiDmaTransferActive & 0x02) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> multiword DMA mode 1 is active\r\n")));
        }
        if (pId->MultiDmaTransferActive & 0x01) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> multiword DMA mode 0 is active\r\n")));
        }
    }

    // TranslationFieldsValid <=> Word 53; UltraDMASupport <=> Word 88, low-byte
    // dump supported Ultra DMA modes
    if (pId->TranslationFieldsValid & 0x0004) {

        if (pId->UltraDMASupport & 0x20) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode <= 5 are supported\r\n")));
        }
        else if (pId->UltraDMASupport & 0x10) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode <= 4 are supported\r\n")));
        }
        else if (pId->UltraDMASupport & 0x08) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode <= 3 are supported\r\n")));
        }
        else if (pId->UltraDMASupport & 0x04) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode <= 2 are supported\r\n")));
        }
        else if (pId->UltraDMASupport & 0x02) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode <= 1 are supported\r\n")));
        }
        else if (pId->UltraDMASupport & 0x01) {
            DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode 0 is supported\r\n")));
        }
        else {
            fDMASupported = FALSE;
        }
        // UltraDMAActive <=> Word 88, high-byte; dump selected Ultra DMA modes
        if (fDMASupported) {
            if (pId->UltraDMAActive & 0x20) {
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode 5 is active\r\n")));
            }
            if (pId->UltraDMAActive & 0x10) {
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode 4 is active\r\n")));
            }
            if (pId->UltraDMAActive & 0x08) {
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode 3 is active\r\n")));
            }
            if (pId->UltraDMAActive & 0x04) {
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode 2 is active\r\n")));
            }
            if (pId->UltraDMAActive & 0x02) {
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode 1 is active\r\n")));
            }
            if (pId->UltraDMAActive & 0x01) {
                DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Ultra DMA mode 0 is active\r\n")));
            }
        }
    }

    // CommandSetSupported1 <=> Word 82 <=> whether look-ahead + write cache are supported
    if (pId->CommandSetSupported1 & (1 << 6)) {
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Look-ahead supported\r\n")));
    }
    if (pId->CommandSetSupported1 & (1 << 5)) {
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> Write cache supported\r\n")));
    }
    if (pId->CommandSetSupported1 & 0x01) {
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Identify> SMART feature set supported\r\n")));
    }
}

#endif
