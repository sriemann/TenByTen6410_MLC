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

#include <diskmain.h>

// ----------------------------------------------------------------------------
// Function: SendIOCommand
//     Issue I/O command
//
// Parameters:
//     pId -
//     dwNumberOfSectors -
//     bCmd -
// ----------------------------------------------------------------------------

BOOL
CDisk::SendIOCommand(
    DWORD dwStartSector,
    DWORD dwNumberOfSectors,
    BYTE bCmd
    )
{
    DEBUGMSG(ZONE_IO, (TEXT(
        "Atapi!CDisk::SendIOCommand> sector(%d), sectors left(%x), command(%x)\r\n"
        ), dwStartSector,dwNumberOfSectors,bCmd));

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_IOCOMMAND, &bCmd, sizeof(bCmd), 0, CELZONE_ALWAYSON, 0, FALSE);

    SelectDevice();

    if (WaitOnBusy(FALSE)) {
        DEBUGMSG(ZONE_IO, (TEXT(
            "Atapi!CDisk::SendIOCommand> Failed to send command; status(%x), error(%x)\r\n"
            ), GetAltStatus(),GetError()));
        return FALSE;
    }

    if (m_fUseLBA48) {
        ASSERT(dwNumberOfSectors <= MAX_SECT_PER_EXT_COMMAND);
        // to transfer 65536 sectors, set number of sectors to 0
        if (dwNumberOfSectors == MAX_SECT_PER_EXT_COMMAND) {
            dwNumberOfSectors = 0;
        }

        WriteSectorCount((BYTE)(dwNumberOfSectors >> 8));   // Sector Count 15:8
        WriteSectorCount((BYTE)(dwNumberOfSectors));        // Sector Count 7:0

        // CE supports only 32-bit LBA as of now.  Therefore, clear the upper 16 bits of LBA.
        WriteHighCount(0);    // LBA 47:40
        WriteLowCount(0);     // LBA 39:32
        WriteSectorNumber((BYTE)(dwStartSector >> 24));   // LBA 31:24
        WriteHighCount((BYTE)(dwStartSector >> 16));  // LBA 23:16
        WriteLowCount((BYTE)(dwStartSector >> 8));    // LBA 15:8
        WriteSectorNumber((BYTE)dwStartSector);           // LBA 7:0
        WriteDriveHeadReg( ATA_HEAD_LBA_MODE | ((m_dwDevice == 0 ) ? ATA_HEAD_DRIVE_1 : ATA_HEAD_DRIVE_2));
    }
    else {
        ASSERT(dwNumberOfSectors <= MAX_SECT_PER_COMMAND);
        // to transfer 256 sectors, set number of sectors to 0
        if (dwNumberOfSectors == MAX_SECT_PER_COMMAND) {
            dwNumberOfSectors = 0;
        }

        WriteSectorCount((BYTE)dwNumberOfSectors);
        if (m_fLBAMode == TRUE) {
            ASSERT((dwStartSector >> 28) == 0);
            WriteSectorNumber( (BYTE)dwStartSector);
            WriteLowCount((BYTE)(dwStartSector >> 8));
            WriteHighCount((BYTE)(dwStartSector >> 16));
            WriteDriveHeadReg((BYTE)((dwStartSector >> 24) | ATA_HEAD_LBA_MODE) | ((m_dwDevice == 0 ) ? ATA_HEAD_DRIVE_1 : ATA_HEAD_DRIVE_2));
        }
        else {
            DWORD dwSectors = m_DiskInfo.di_sectors;
            DWORD dwHeads = m_DiskInfo.di_heads;
            WriteSectorNumber((BYTE)((dwStartSector % dwSectors) + 1));
            WriteLowCount((BYTE) (dwStartSector /(dwSectors*dwHeads)));
            WriteHighCount((BYTE)((dwStartSector /(dwSectors*dwHeads)) >> 8));
            WriteDriveHeadReg((BYTE)(((dwStartSector/dwSectors)% dwHeads) | ((m_dwDevice == 0 ) ? ATA_HEAD_DRIVE_1 : ATA_HEAD_DRIVE_2)));
        }
    }

    WriteCommand(bCmd);

    return (TRUE);
}

// ----------------------------------------------------------------------------
// Function: WaitOnBusy
//     Wait for BSY=0
//
// Parameters:
//     fBase -
// ----------------------------------------------------------------------------

BYTE
CDisk::WaitOnBusy(
    BOOL fBase
    )
{
    DWORD i, j;
    BYTE bStatus = 0;
    for (i = 0; i < m_dwWaitCheckIter; i++) {
        for (j = 0; j < m_dwWaitSampleTimes; j++)  {
            bStatus = fBase ? GetBaseStatus() : GetAltStatus();
            if (!(bStatus & ATA_STATUS_BUSY)) {
                return bStatus & (ATA_STATUS_ERROR |ATA_STATUS_BUSY);
            }
        }
       StallExecution(m_dwWaitStallTime);
    }
    return bStatus & (ATA_STATUS_ERROR |ATA_STATUS_BUSY);
}

// ----------------------------------------------------------------------------
// Function: WaitForDisc
//     Generic wait routine; wait for @bStatusType
//
// Parameters:
//     bStatusType -
//     dwTimeOut -
//     dwPeriod -
// ----------------------------------------------------------------------------

BOOL
CDisk::WaitForDisc(
    BYTE bStatusType,
    DWORD dwTimeOut,
    DWORD dwPeriod
    )
{
    BYTE bStatusRead = 0;

    if (dwPeriod == 0) {
        dwPeriod = dwTimeOut;
    }

    while( TRUE)  {
        bStatusRead = GetAltStatus();
        switch (bStatusType) {
            case WAIT_TYPE_BUSY:
                if (bStatusRead & ATA_STATUS_BUSY) {
                    DEBUGMSG(ZONE_IO, (TEXT("Atapi!CDisk::WaitForDisc> WAIT_TYPE_BUSY\r\n")));
                    goto ExitDone;
                }
                break;
            case WAIT_TYPE_NOT_BUSY:
                if (!(bStatusRead & ATA_STATUS_BUSY)) {
                    DEBUGMSG(ZONE_IO, (TEXT("Atapi!CDisk::WaitForDisc> WAIT_TYPE_NOT_BUSY\r\n")));
                    goto ExitDone;
                }
                break;
            case WAIT_TYPE_READY:
                if (bStatusRead & ATA_STATUS_READY) {
                    DEBUGMSG(ZONE_IO, (TEXT("Atapi!CDisk::WaitForDisc> WAIT_TYPE_READY\r\n")));
                    StallExecution(100);
                    goto ExitDone;
                }
                break;
            case WAIT_TYPE_DRQ:
                if (bStatusRead & ATA_STATUS_DATA_REQ) {
                    DEBUGMSG(ZONE_IO, (TEXT("Atapi!CDisk::WaitForDisc> WAIT_TYPE_DRQ\r\n")));
                    goto ExitDone;
                }
                break;
            case WAIT_TYPE_NOT_DRQ:
                if (!(bStatusRead & ATA_STATUS_DATA_REQ)) {
                    DEBUGMSG(ZONE_IO, (TEXT("Atapi!CDisk::WaitForDisc> WAIT_TYPE_NOT_DRQ\r\n")));
                    goto ExitDone;
                }
                break;
            case WAIT_TYPE_ERROR:
                if (bStatusRead & ATA_STATUS_ERROR) {
                    DEBUGMSG(ZONE_IO, (TEXT("Atapi!CDisk::WaitForDisc> WAIT_TYPE_ERROR\r\n")));
                    goto ExitDone;
                }
                break;
        }
        if ((int)dwTimeOut > 0) {
            StallExecution(dwPeriod);
            dwTimeOut -= dwPeriod;
        }
        else {
            DEBUGMSG(ZONE_ERROR, (TEXT(
                "Atapi!CDisk::WaitForDisc> timeout; bStatusType(%d); status(%02X)\r\n"
                ), bStatusType, bStatusRead));
            return ERROR_GEN_FAILURE;
        }
    }
ExitDone:
    return ERROR_SUCCESS;
}

// ----------------------------------------------------------------------------
// Function: WaitForDRQ
//     Wait for DRQ=1 (and device ready to transfer) or timeout
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL
CDisk::WaitForDRQ(
    )
{
    DWORD i,j;
    BYTE bStatus = 0;

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_WAITDRQ, NULL, 0, 0, CELZONE_ALWAYSON, 0, FALSE);

    for (i = 0; i < m_dwWaitCheckIter; i++) {
        for (j = 0; j < m_dwWaitSampleTimes; j++) {
            bStatus = GetAltStatus() & (ATA_STATUS_BUSY|ATA_STATUS_DATA_REQ);
            if ((bStatus & ATA_STATUS_BUSY) == 0) {
                if (bStatus & ATA_STATUS_DATA_REQ){
                    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_STATUSWAITDRQ, &bStatus, sizeof(bStatus), 0, CELZONE_ALWAYSON, 0, FALSE);
                    return TRUE;
                }
            }
            StallExecution(m_dwWaitStallTime);
        }
        DEBUGMSG(ZONE_WARNING, (TEXT(
            "Atapi!CDisk::WaitForDRQ> status(%02X), error(%02X), reason(%02X)\r\n"
            ), GetAltStatus(), GetError(), GetReason()));
    }

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_STATUSWAITDRQ, &bStatus, sizeof(bStatus), 0, CELZONE_ALWAYSON, 0, FALSE);

    return (bStatus == ATA_STATUS_DATA_REQ);
}

// ----------------------------------------------------------------------------
// Function: CheckIntrState
//     Return interrupt reason/status
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

WORD
CDisk::CheckIntrState(
    )
{
    BYTE bReason, bDRQ;

    WaitOnBusy(FALSE);

    bReason = GetReason() & (ATA_IR_CoD | ATA_IR_IO);
    bDRQ = GetAltStatus() & ATA_STATUS_DATA_REQ;

    if (bDRQ) {
        bReason |= 4;
    }
    if (bReason < 3) {
        return((WORD) ATA_INTR_READY);
    }

    return ((WORD)bReason);
}

// ----------------------------------------------------------------------------
// Function: ReadBuffer
//     Fill buffer from data register
//
// Parameters:
//     pBuffer -
//     dwCount -
// ----------------------------------------------------------------------------

void
CDisk::ReadBuffer(
    PBYTE pBuffer,
    DWORD dwCount
    )
{
    union {
        WORD us;
        BYTE  uc[2];
    } unisc;

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_STARTREADBUFFER, &dwCount, sizeof(dwCount), 0, CELZONE_ALWAYSON, 0, FALSE);

    if (dwCount == 0) {
        return;
    }

    // determine whether required byte was read in previous read;
    // m_wNextByte=(-1) implies byte not read in previous read

    if (m_wNextByte != 0xFFFF) {
        DEBUGMSG(ZONE_WARNING, (TEXT("Atapi!CDisk::ReadBuffer> Unaligned buffer on prevous read\r\n")));
        // update first byte
        *pBuffer++ = (BYTE)m_wNextByte;
        dwCount--;
        m_wNextByte = 0xFFFF;
    }

    // check alignment of pBuffer

    if ((DWORD)pBuffer & 1) {
        DEBUGMSG(ZONE_WARNING, (TEXT("Atapi!CDisk::ReadBuffer> Unaligned buffer\r\n")));
        while (dwCount> 1) {
            unisc.us = ReadWord();
            *pBuffer++= unisc.uc[0];
            *pBuffer++= unisc.uc[1];
            dwCount-=2;
        }
    }
    else {
        ReadWordBuffer((PWORD)pBuffer,(DWORD)(dwCount)/sizeof(SHORT));
        pBuffer += dwCount;
        dwCount &= 1;       // if 1, we need to read the next byte still
        pBuffer -= dwCount; // adjust pBuffer if its address is odd
    }

    // read one word even if we only need one byte; save the unused byte and
    // use it as the first byte of the next read (scatter/gather buffer)

    if (dwCount == 1) {
        DEBUGMSG(ZONE_WARNING, (TEXT("Atapi!CDisk::ReadBuffer> Reading word for one byte\r\n")));
        unisc.us = ReadWord();
        *pBuffer=   unisc.uc[0];
        m_wNextByte = (WORD)unisc.uc[1]; // save the second byte
    }

    BYTE bStatus = GetAltStatus();

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_COMPLETEREADBUFFER, &bStatus, sizeof(bStatus), 0, CELZONE_ALWAYSON, 0, FALSE);
}

// ----------------------------------------------------------------------------
// Function: WriteBuffer
//     Empty buffer to data register
//
// Parameters:
//     pBuffer -
//     dwCount -
// ----------------------------------------------------------------------------

void
CDisk::WriteBuffer(
    PBYTE pBuffer,
    DWORD dwCount
    )
{
    union {
        WORD us;
        BYTE  uc[2];
    } unisc;

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_STARTWRITEBUFFER, &dwCount, sizeof(dwCount), 0, CELZONE_ALWAYSON, 0, FALSE);

    if (dwCount == 0) {
        return;
    }

    // determine whether required byte was written in previous write;
    // m_wNextByte=(-1) implies byte not written in previous write

    if (m_wNextByte != 0xFFFF) {
        // update first byte
        DEBUGMSG(ZONE_WARNING, (TEXT("Atapi!CDisk::WriteBuffer> Unaligned buffer on previous write\r\n")));
        unisc.uc[0] = (BYTE) m_wNextByte;
        unisc.uc[1] = *pBuffer++;
        dwCount--; 
        WriteWord(unisc.us);
        m_wNextByte = 0xFFFF;
    }

    // check alignment of pBuffer

    if ((DWORD) pBuffer & 1) {
        DEBUGMSG(ZONE_WARNING, (TEXT("Atapi!CDisk::WriteBuffer> Unaligned buffer\r\n")));
        while (dwCount> 1) {
            unisc.uc[0] = *pBuffer++;
            unisc.uc[1] = *pBuffer++;
            WriteWord(unisc.us);
            dwCount-=2;
        }
    }
    else {
        WriteWordBuffer((PWORD)pBuffer,(DWORD)(dwCount)/sizeof(SHORT));
        pBuffer += dwCount;
        dwCount &= 1;       // if 1, we need to write the next byte still
        pBuffer -= dwCount; // adjust pBuffer if its address is odd

    }

    // write one word even if we only need one byte; save the unused byte and
    // use it as the first byte of the next read (scatter/gather buffer)

    if (dwCount == 1) {
        DEBUGMSG( ZONE_WARNING, (TEXT("Atapi!CDisk::WriteBuffer> Writing one word for one byte\r\n")));
        m_wNextByte = (WORD) *pBuffer; // save the second byte
    }

    BYTE bStatus = GetAltStatus();

    if (ZONE_CELOG) CeLogData(TRUE, CELID_ATAPI_COMPLETEWRITEBUFFER, &bStatus, sizeof(bStatus), 0, CELZONE_ALWAYSON, 0, FALSE);
}

// ----------------------------------------------------------------------------
// Function: SetTransferMode
//     Set a device's transfer mode through the SET FEATURES command
//
// Parameters:
//     bMode - desired transfer mode
//
// Notes:
//     See ATA/ATAPI-6 R3B 8.15 IDENTIFY DEVICE for more information regarding
//     IDENTIFY DEVICE data.
//     See ATA/ATAPI-6 R3B 8.46 SET FEATURES for more information regarding
//     transfer mode encodings.
// ----------------------------------------------------------------------------

BOOL
CDisk::SetTransferMode(
    BYTE bMode
    )
{
    BYTE bStatus; // Status register
    BYTE bError;  // Error register
    BOOL fOk = TRUE;     // result

    // HI:Check_Status (Host Idle); wait until BSY=0 and DRQ=0
    // read Status register
    while (1) {
        bStatus = GetAltStatus();
        if (!(bStatus & (0x80|0x08))) break; // BSY := Bit 7, DRQ := Bit 3
        Sleep(5);
    }

    // HI:Device_Select; select device
    SelectDevice();

    // HI:Check_Status (Host Idle); wait until BSY=0 and DRQ=0
    // read Status register
    while (1) {
        bStatus = GetAltStatus();
        if (!(bStatus & (0x80|0x08))) break; // BSY := Bit 7, DRQ := Bit 3
        Sleep(5);
    }

    // HI:Write_Parameters
    WriteFeature(0x03);      // set transfer mode based on value in Sector Count register (Table 44)
    WriteSectorCount(bMode); // (Table 45)

    // HI:Write_Command
    WriteCommand(0xEF); // SET FEATURES command code := EFh

    // transition to non-data command protocol

    // HND:INTRQ_Wait
    // transition to HND:Check_Status
    // read Status register
    while (1) { // BSY := Bit 7
        bStatus = GetAltStatus();
        bError = GetError();
        if (bError & 0x04) { // ABRT := Bit 2
            // command was aborted
            DEBUGMSG(ZONE_ERROR, (_T(
                "Atapi!CDisk::SetTransferMode> Failed to send SET FEATURES command, parameter 0x%x\r\n"
                ), bMode));
            fOk = FALSE;
            break;
        }
        if (!(bStatus & 0x80)) break; // BSY := Bit 7
        Sleep(5);
    }
    GetBaseStatus();

    // transition to host idle protocol

    return fOk;
}

