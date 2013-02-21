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
//------------------------------------------------------------------------------
//
//  File:  cs8900a.c
//
#include <windows.h>
#include <ceddk.h>
#include <ethdbg.h>
#include <oal.h>

//------------------------------------------------------------------------------

#define CS8900A_EISA_NUMBER         0x630e
#define RETRY_COUNT                 0x00100000

//------------------------------------------------------------------------------

typedef struct {
    UINT16 DATA0;
    UINT16 DATA1;
    UINT16 TXCMD;
    UINT16 TXLENGTH;
    UINT16 ISQ;
    UINT16 PAGEIX;
    UINT16 PAGE0;
    UINT16 PAGE1;
} CS8900A_REGS;

static CS8900A_REGS *g_pCS8900;
static UINT16 g_hash[4];
static UINT32 g_filter;

//------------------------------------------------------------------------------

#define EISA_NUMBER                 0x0000
#define PRODUCT_ID_CODE             0x0002
#define IO_BASE_ADDRESS             0x0020
#define INTERRUPT_NUMBER            0x0022
#define DMA_CHANNEL_NUMBER          0x0024
#define DMA_START_OF_FRAME          0x0026
#define DMA_FRAME_COUNT             0x0028
#define RXDMA_BYTE_COUNT            0x002a
#define MEMORY_BASE_ADDR            0x002c
#define BOOT_PROM_BASE_ADDR         0x0030
#define BOOT_PROM_ADDR_MASK         0x0034
#define EEPROM_COMMAND              0x0040
#define EEPROM_DATA                 0x0042
#define RECEIVE_FRAME_BYTE_COUNT    0x0050

#define INT_SQ                      0x0120
#define RX_CFG                      0x0102
#define RX_EVENT                    0x0124
#define RX_CTL                      0x0104
#define TX_CFG                      0x0106
#define TX_EVENT                    0x0128
#define TX_CMD                      0x0108
#define BUF_CFG                     0x010A
#define BUF_EVENT                   0x012C
#define RX_MISS                     0x0130
#define TX_COL                      0x0132
#define LINE_CTL                    0x0112
#define LINE_ST                     0x0134
#define SELF_CTL                    0x0114
#define SELF_ST                     0x0136
#define BUS_CTL                     0x0116
#define BUS_ST                      0x0138
#define TEST_CTL                    0x0118
#define AUI_TIME_DOMAIN             0x013C
#define TX_CMD_REQUEST              0x0144
#define TX_CMD_LENGTH               0x0146

#define LOGICAL_ADDR_FILTER_BASE    0x0150
#define INDIVIDUAL_ADDRESS          0x0158

#define RX_STATUS                   0x0400
#define RX_LENGTH                   0x0402
#define RX_FRAME                    0x0404
#define TX_FRAME                    0x0a00

//------------------------------------------------------------------------------

#define ISQ_ID_MASK                 0x003F


#define SELF_CTL_RESET              (1 << 6)

#define SELF_ST_SIBUSY              (1 << 8)
#define SELF_ST_INITD               (1 << 7)

#define LINE_CTL_MOD_BACKOFF        (1 << 11)
#define LINE_CTL_AUI_ONLY           (1 << 8)
#define LINE_CTL_TX_ON              (1 << 7)
#define LINE_CTL_RX_ON              (1 << 6)

#define RX_CFG_RX_OK_IE             (1 << 8)
#define RX_CFG_SKIP_1               (1 << 6)

#define RX_CTL_BROADCAST            (1 << 11)
#define RX_CTL_INDIVIDUAL           (1 << 10)
#define RX_CTL_MULTICAST            (1 << 9)
#define RX_CTL_RX_OK                (1 << 8)
#define RX_CTL_PROMISCUOUS          (1 << 7)
#define RX_CTL_IAHASH               (1 << 6)

#define RX_EVENT_RX_OK              (1 << 8)
#define RX_EVENT_ID                 0x0004

#define TX_CMD_PAD_DIS              (1 << 13)
#define TX_CMD_INHIBIT_CRC          (1 << 12)
#define TX_CMD_ONECOLL              (1 << 9)
#define TX_CMD_FORCE                (1 << 8)
#define TX_CMD_START_5              (0 << 6)
#define TX_CMD_START_381            (1 << 6)
#define TX_CMD_START_1021           (2 << 6)
#define TX_CMD_START_ALL            (3 << 6)

#define BUS_ST_TX_RDY               (1 << 8)

#define BUS_CTL_ENABLE_IRQ          (1 << 15)

//------------------------------------------------------------------------------

static UINT16 ReadPacketPage(UINT16 address);
static VOID WritePacketPage(UINT16 address, UINT16 data);
static UINT32 ComputeCRC(UINT8 *pBuffer, UINT32 length);

//------------------------------------------------------------------------------
//
//  Function:  CS8900AInit
//
BOOL CS8900AInit(UINT8 *pAddress, UINT32 offset, UINT16 mac[3])
{
    BOOL rc = FALSE;
    UINT32 count;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+CS8900AInit(0x%08x, 0x%08x, %02x:%02x:%02x:%02x:%02x:%02x)\r\n",
        pAddress, offset, mac[0]&0xFF, mac[0]>>8, mac[1]&0xFF, mac[1]>>8,
        mac[2]&0xFF, mac[2]>>8
    ));

    // Save address
    g_pCS8900 = (CS8900A_REGS*)pAddress;

    // First check if there is chip
    if (ReadPacketPage(EISA_NUMBER) != CS8900A_EISA_NUMBER) {
        OALMSGS(OAL_ERROR, (L"ERROR: CS8900AInit: Failed detect chip\r\n"));
        goto cleanUp;
    }

    OALMSGS(OAL_INFO, (L"INFO: CS8900AInit chip detected\r\n"));

    // Reset chip
    WritePacketPage(SELF_CTL, SELF_CTL_RESET);
    count = RETRY_COUNT;
    while (count-- > 0) {
        if ((ReadPacketPage(SELF_ST) & SELF_ST_INITD) != 0) break;
    }
    if (count == 0) {
        OALMSGS(OAL_ERROR, (L"ERROR: CS8900AInit: Failed to reset card\r\n"));
        goto cleanUp;
    }
    count = RETRY_COUNT;
    while (count-- > 0) {
        if ((ReadPacketPage(SELF_ST) & SELF_ST_SIBUSY) != 0) break;
    }
    if (count == 0) {
        OALMSGS(OAL_ERROR, (L"ERROR: CS8900AInit: Failed to reset card\r\n"));
        goto cleanUp;
    }

    // Set MAC address
    WritePacketPage(INDIVIDUAL_ADDRESS + 0, mac[0]);
    WritePacketPage(INDIVIDUAL_ADDRESS + 2, mac[1]);
    WritePacketPage(INDIVIDUAL_ADDRESS + 4, mac[2]);

    // Enable receive
    WritePacketPage(RX_CTL, RX_CTL_RX_OK|RX_CTL_INDIVIDUAL|RX_CTL_BROADCAST);

    // Enable interrupt on receive
    WritePacketPage(RX_CFG, RX_CFG_RX_OK_IE);

    // Let assume that hardware is using INTRQ0
    WritePacketPage(INTERRUPT_NUMBER, 0);

    // Enable
    WritePacketPage(LINE_CTL, LINE_CTL_RX_ON|LINE_CTL_TX_ON);

    // Done
    rc = TRUE;

cleanUp:
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-CS8900AInit(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  CS8900ASendFrame
//
UINT16 CS8900ASendFrame(UINT8 *pData, UINT32 length)
{
    BOOL rc = FALSE;
    UINT32 count;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"+CS8900ASendFrame(0x%08x, %d)\r\n", pData, length
    ));

    // Send Command
    OUTPORT16(&g_pCS8900->TXCMD, TX_CMD_START_ALL);
    OUTPORT16(&g_pCS8900->TXLENGTH, length);

    count = RETRY_COUNT;
    while (count-- > 0) {
        if ((ReadPacketPage(BUS_ST) & BUS_ST_TX_RDY) != 0) break;
    }
    if (count == 0) goto cleanUp;

    length = (length + 1) >> 1;
    while (length-- > 0) {
        OUTPORT16(&g_pCS8900->DATA0, *(UINT16*)pData);
        pData += sizeof(UINT16);
    }

    rc = TRUE;

cleanUp:
    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (L"-CS8900ASendFrame(rc = %d)\r\n", !rc));
    return !rc;
}

//------------------------------------------------------------------------------
//
//  Function:  CS8900AGetFrame
//
UINT16 CS8900AGetFrame(UINT8 *pData, UINT16 *pLength)
{
    UINT16 isq, length, status, count, data;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"+CS8900AGetFrame(0x%08x, %d)\r\n", pData, *pLength
    ));

    length = 0;
    isq = INPORT16(&g_pCS8900->ISQ);
    if ((isq & ISQ_ID_MASK) == RX_EVENT_ID && (isq & RX_EVENT_RX_OK) != 0) {

        // Get RxStatus and length
        status = INPORT16(&g_pCS8900->DATA0);
        length = INPORT16(&g_pCS8900->DATA0);

        if (length > *pLength) {
            // If packet doesn't fit in buffer, skip it
            data = ReadPacketPage(RX_CFG);
            WritePacketPage(RX_CFG, data | RX_CFG_SKIP_1);
            length = 0;
        } else {
            // Read packet data
            count = length;
            while (count > 1) {
                data = INPORT16(&g_pCS8900->DATA0);
                *(UINT16*)pData = data;
                pData += sizeof(UINT16);
                count -= sizeof(UINT16);
            }

            // Read last one byte
            if (count > 0) {
                data = INPORT16(&g_pCS8900->DATA0);
                *pData = (UINT8)data;
            }
        }
    }

    // Return packet size
    *pLength = length;

    OALMSGS(OAL_ETHER&&OAL_VERBOSE, (
        L"-CS8900AGetFrame(length = %d)\r\n", length
    ));
    return length;
}

//------------------------------------------------------------------------------
//
//  Function:  CS8900AEnableInts
//
VOID CS8900AEnableInts()
{
    UINT16 data;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+CS8900AEnableInts\r\n"));
    data = ReadPacketPage(BUS_CTL);
    WritePacketPage(BUS_CTL, data | BUS_CTL_ENABLE_IRQ);
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-CS8900AEnableInts\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  CS8900ADisableInts
//
VOID CS8900ADisableInts()
{
    UINT16 data;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"+CS8900ADisableInts\r\n"));
    data = ReadPacketPage(BUS_CTL);
    WritePacketPage(BUS_CTL, data & ~BUS_CTL_ENABLE_IRQ);
    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-CS8900ADisableInts\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  CS8900ACurrentPacketFilter
//
VOID CS8900ACurrentPacketFilter(UINT32 filter)
{
    UINT16 rxCtl;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+CS8900ACurrentPacketFilter(0x%08x)\r\n", filter
    ));

    // Read current filter
    rxCtl = ReadPacketPage(RX_CTL);

    if ((filter & PACKET_TYPE_ALL_MULTICAST) != 0) {
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 0, 0xFFFF);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 2, 0xFFFF);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 4, 0xFFFF);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 6, 0xFFFF);
    } else {
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 0, g_hash[0]);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 2, g_hash[1]);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 4, g_hash[2]);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 6, g_hash[3]);
    }

    if (
        (filter & PACKET_TYPE_MULTICAST) != 0 ||
        (filter & PACKET_TYPE_ALL_MULTICAST) != 0
    ) {
        rxCtl |= RX_CTL_MULTICAST;
    } else {
        rxCtl &= ~RX_CTL_MULTICAST;
    }

    if ((filter & PACKET_TYPE_PROMISCUOUS) != 0) {
        rxCtl |= RX_CTL_PROMISCUOUS;
    } else {
        rxCtl &= ~RX_CTL_PROMISCUOUS;
    }

    WritePacketPage(RX_CTL, rxCtl);

    // Save actual filter
    g_filter = filter;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-CS8900ACurrentPacketFilter\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  CS8900AMulticastList
//
BOOL CS8900AMulticastList(UINT8 *pAddresses, UINT32 count)
{
    UINT32 i, j, crc, data, bit;

    OALMSGS(OAL_ETHER&&OAL_FUNC, (
        L"+RTL8139MulticastList(0x%08x, %d)\r\n", pAddresses, count
    ));

    g_hash[0] = g_hash[1] = g_hash[2] = g_hash[3] = 0;
    for (i = 0; i < count; i++) {
        data = crc = ComputeCRC(pAddresses, 6);
        for (j = 0, bit = 0; j < 6; j++) {
            bit <<= 1;
            bit |= (data & 1);
            data >>= 1;
        }
        g_hash[bit >> 4] |= 1 << (bit & 0x0f);
        pAddresses += 6;
    }

    // But update only if all multicast mode isn't active
    if ((g_filter & PACKET_TYPE_ALL_MULTICAST) == 0) {        
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 0, g_hash[0]);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 2, g_hash[1]);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 4, g_hash[2]);
        WritePacketPage(LOGICAL_ADDR_FILTER_BASE + 6, g_hash[3]);
    }

    OALMSGS(OAL_ETHER&&OAL_FUNC, (L"-CS8900AMulticastList(rc = 1)\r\n"));
    return TRUE;
}

//------------------------------------------------------------------------------

static UINT16 ReadPacketPage(UINT16 address)
{
    OUTREG16(&g_pCS8900->PAGEIX, address);
    return INREG16(&g_pCS8900->PAGE0);
}

//------------------------------------------------------------------------------

static VOID WritePacketPage(UINT16 address, UINT16 data)
{
    OUTREG16(&g_pCS8900->PAGEIX, address);
    OUTREG16(&g_pCS8900->PAGE0, data);
}

//------------------------------------------------------------------------------

static UINT32 ComputeCRC(UINT8 *pBuffer, UINT32 length)
{
    UINT32 crc, carry, i, j;
    UINT8 byte;

    crc = 0xffffffff;
    for (i = 0; i < length; i++) {
        byte = pBuffer[i];
        for (j = 0; j < 8; j++) {
            carry = ((crc & 0x80000000) ? 1 : 0) ^ (byte & 0x01);
            crc <<= 1;
            byte >>= 1;
            if (carry) crc = (crc ^ 0x04c11db6) | carry;
        }
    }
    return crc;
}

//------------------------------------------------------------------------------

