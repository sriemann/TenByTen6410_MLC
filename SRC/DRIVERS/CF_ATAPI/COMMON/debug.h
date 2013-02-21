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
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <atapi2.h>

// Debug zones

#define ZONEID_INIT       0
#define ZONEID_DEINIT     1
#define ZONEID_MAIN       2
#define ZONEID_IO         3
#define ZONEID_PCMCIA     4
#define ZONEID_PCI        5
#define ZONEID_IOCTL      6
#define ZONEID_CDROM      7
#define ZONEID_DMA        8
#define ZONEID_POWER      9
#define ZONEID_WARNING    12
#define ZONEID_ERROR      13
#define ZONEID_HELPER     14
#define ZONEID_CELOG      15

#define ZONE_INIT         DEBUGZONE(ZONEID_INIT)
#define ZONE_DEINIT       DEBUGZONE(ZONEID_DEINIT)
#define ZONE_MAIN         DEBUGZONE(ZONEID_MAIN)
#define ZONE_IO           DEBUGZONE(ZONEID_IO)
#define ZONE_PCMCIA       DEBUGZONE(ZONEID_PCMCIA)
#define ZONE_PCI          DEBUGZONE(ZONEID_PCI)
#define ZONE_IOCTL        DEBUGZONE(ZONEID_IOCTL)
#define ZONE_CDROM        DEBUGZONE(ZONEID_CDROM)
#define ZONE_DMA          DEBUGZONE(ZONEID_DMA)
#define ZONE_POWER        DEBUGZONE(ZONEID_POWER)
#define ZONE_WARNING      DEBUGZONE(ZONEID_WARNING)
#define ZONE_ERROR        DEBUGZONE(ZONEID_ERROR)
#define ZONE_HELPER       DEBUGZONE(ZONEID_HELPER)
#define ZONE_CELOG        DEBUGZONE(ZONEID_CELOG)

#define ZONEMASK_INIT     (1 << ZONEID_INIT)
#define ZONEMASK_DEINIT   (1 << ZONEID_DEINIT)
#define ZONEMASK_MAIN     (1 << ZONEID_MAIN)
#define ZONEMASK_IO       (1 << ZONEID_IO)
#define ZONEMASK_PCMCIA   (1 << ZONEID_PCMCIA)
#define ZONEMASK_PCI      (1 << ZONEID_PCI)
#define ZONEMASK_IOCTL    (1 << ZONEID_IOCTL)
#define ZONEMASK_CDROM    (1 << ZONEID_CDROM)
#define ZONEMASK_DMA      (1 << ZONEID_DMA)
#define ZONEMASK_WARNING  (1 << ZONEID_WARNING)
#define ZONEMASK_ERROR    (1 << ZONEID_ERROR)
#define ZONEMASK_HELPER   (1 << ZONEID_HELPER)
#define ZONEMASK_CELOG    (1<<ZONEID_CELOG)

#define CELID_ATAPI_BASE                (CELID_USER + 0x180)
#define CELID_ATAPI_STARTIOCTL          (CELID_ATAPI_BASE + 0)
#define CELID_ATAPI_COMPLETEIOCTL       (CELID_ATAPI_BASE + 1)
#define CELID_ATAPI_BASESTATUS          (CELID_ATAPI_BASE + 2)
#define CELID_ATAPI_IOCOMMAND           (CELID_ATAPI_BASE + 3)
#define CELID_ATAPI_WAITINTERRUPT       (CELID_ATAPI_BASE + 4)
#define CELID_ATAPI_INTERRUPTDONE       (CELID_ATAPI_BASE + 5)
#define CELID_ATAPI_ENABLEINTERRUPT     (CELID_ATAPI_BASE + 6)
#define CELID_ATAPI_WAITDRQ             (CELID_ATAPI_BASE + 7)
#define CELID_ATAPI_STATUSWAITDRQ       (CELID_ATAPI_BASE + 8)
#define CELID_ATAPI_STARTREADBUFFER     (CELID_ATAPI_BASE + 9)
#define CELID_ATAPI_COMPLETEREADBUFFER  (CELID_ATAPI_BASE + 10)
#define CELID_ATAPI_STARTWRITEBUFFER    (CELID_ATAPI_BASE + 11)
#define CELID_ATAPI_COMPLETEWRITEBUFFER (CELID_ATAPI_BASE + 12)
#define CELID_ATAPI_SETUPDMA_COPY       (CELID_ATAPI_BASE + 13)
#define CELID_ATAPI_WAITDMAINTERRUPT    (CELID_ATAPI_BASE + 14)
#define CELID_ATAPI_DMAINTERRUPTDONE    (CELID_ATAPI_BASE + 15)
#define CELID_ATAPI_SETUPDMA_ALIGNED    (CELID_ATAPI_BASE + 16)
#define CELID_ATAPI_SETDEVICEPOWER      (CELID_ATAPI_BASE + 17)
#define CELID_ATAPI_POWERCOMMAND        (CELID_ATAPI_BASE + 18)

#ifdef DEBUG
    void DumpRegKey(DWORD dwZone, PTSTR szKey, HKEY hKey);
    #define DUMPREGKEY(dwZone, szKey, hKey) DumpRegKey(dwZone, szKey, hKey)
    VOID DumpIdentify(PIDENTIFY_DATA pId);
    #define DUMPIDENTIFY(pId) DumpIdentify(pId)
    VOID DumpSupportedTransferModes(PIDENTIFY_DATA pId);
    #define DUMPSUPPORTEDTRANSFERMODES(pId) DumpSupportedTransferModes(pId)
#else
    #define DUMPREGKEY(dwZone, szKey, hKey)
    #define DUMPIDENTIFY(pId)
    #define DUMPSUPPORTEDTRANSFERMODES(pId)
#endif // DEBUG

extern DBGPARAM dpCurSettings;

#endif //_DEBUG_H_

