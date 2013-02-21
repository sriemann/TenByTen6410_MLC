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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    SDHCDMA.hpp
Abstract:
    Standard SDHC Driver DMA object.

Notes: 
--*/


#pragma once

#include "SDCardDDK.h"
#include "SDHCD.h"
#include <ceddk.h>

class CSDHCSlotBase;

typedef enum _DMAEVENT {
    NO_DMA = 0,
    DMA_COMPLETE,
    DMA_ERROR_OCCOR,
    TRANSFER_COMPLETED
} DMAEVENT;
#define INITIAL_DMA_LIST 0x400 // 1K
class CSDHCSlotBaseDMA {
public: 
    CSDHCSlotBaseDMA(CSDHCSlotBase& SDHCSloteBase);
    virtual ~CSDHCSlotBaseDMA();
    virtual BOOL Init() ;
    virtual BYTE DmaSelectBit() = 0; // 2.2.10 Bit 4-3.
    virtual BOOL ArmDMA(SD_BUS_REQUEST& Request, BOOL fToDevice);
    virtual BOOL CancelDMA(); 
    virtual BOOL DMANotifyEvent(SD_BUS_REQUEST& Request,DMAEVENT dmaEvent) = 0;
    virtual BOOL IsDMACompleted() { return !m_fDMAProcessing; };
    DWORD   ReAllocateDMABufferList(DWORD dwRequired);
    DWORD DMAComletionCode();
protected:
    DMA_BUFFER_HANDLE m_hDma;
    DWORD           m_dwNumOfList;
    DWORD           m_dwNumOfAvailabe;
    PCE_DMA_BUFFER_BLOCK 
                    m_pDmaBufferList;
    DWORD           m_dwCurDMAListPos;
    CE_DMA_ADAPTER  m_dmaAdapter;
    
    BOOL            m_fDMAProcessing;
    DWORD           m_dwDMACompletionCode;
    CSDHCSlotBase&  m_SDHCSloteBase;
};

typedef struct {
    DWORD   pSrcSize;
    PVOID   pSrcVirtualAddr;
    
    DWORD   dwBufferSize;
    PVOID   pBufferedVirtualAddr;
    DWORD   dwBufferOffset;
    PHYSICAL_ADDRESS physicalAddress;    
} DMA_BUFFERED_BUFFER, *PDMA_BUFFERED_BUFFER;

class CSDHCSlotBaseSDMA : public  CSDHCSlotBaseDMA {
public:
    CSDHCSlotBaseSDMA(CSDHCSlotBase& SDHCSloteBase) : CSDHCSlotBaseDMA(SDHCSloteBase) {
        DEBUGMSG(SDCARD_ZONE_INIT, (_T("CSDHCSlotBaseSDMA:Create DMA Object for SDMA\r\n")));    
        m_dwNextOffset = 0;
        m_fLocked = FALSE;
    };
    ~CSDHCSlotBaseSDMA();
    virtual BOOL Init() ;
    virtual BYTE DmaSelectBit() { return 0; }; // SDMA 2.2.10.
    virtual BOOL ArmDMA(SD_BUS_REQUEST& Request, BOOL fToDevice ) ;
    virtual BOOL DMANotifyEvent(SD_BUS_REQUEST& Request, DMAEVENT dmaEvent);
protected:
    BOOL    GetDMABuffer(SD_BUS_REQUEST& Request,BOOL fToDevice);
    DWORD    GetDMALengthBit(DWORD dwPhysAdddr, DWORD dwDataLength) {
        DWORD dwOffset = (dwPhysAdddr & (PAGE_SIZE-1));    
        return min(dwDataLength, (PAGE_SIZE - dwOffset));
    }
    DWORD   m_dwNextOffset;

    // We handle DMA buffer directly because of SDMA LIMITATION.
    BOOL    m_fLocked;
    LPVOID  m_lpvLockedAddress;
    DWORD   m_dwLockedSize;
    
    DMA_BUFFERED_BUFFER m_StartBuffer;
    DMA_BUFFERED_BUFFER m_EndBuffer;
    DWORD GetDataCacheSize() const {
        return (m_ceCacheInfo.dwL1DCacheLineSize!=0? m_ceCacheInfo.dwL1DCacheLineSize: 0x10); // Default to 16 if it not avaiable.
    }
    CacheInfo       m_ceCacheInfo ;
};

// For 32 bit ADMA2.
typedef struct __ADMA2_32_DESC {
    DWORD   Valid:1;
    DWORD   End:1;
    DWORD   Int:1;
    DWORD   :1;
    DWORD   Act:2;
    DWORD   :10;
    DWORD   Length:16;
    DWORD   Address:32;
} ADMA2_32_DESC,*PADMA2_32_DESC;

#define DESC_ENTRY_PER_TABLE (PAGE_SIZE/sizeof(ADMA2_32_DESC))
#define MAXIMUM_DESC_TABLES 0x10

class CSDHCSlotBase32BitADMA2: public CSDHCSlotBaseDMA {
public:
    CSDHCSlotBase32BitADMA2(CSDHCSlotBase& SDHCSloteBase);
    virtual ~CSDHCSlotBase32BitADMA2();
    virtual BOOL Init() ;
    virtual BYTE DmaSelectBit() { return 2; }; // 32 bit ADMA2 2.2.10.
    virtual BOOL ArmDMA(SD_BUS_REQUEST& Request, BOOL fToDevice ) ;
    virtual BOOL DMANotifyEvent(SD_BUS_REQUEST& Request, DMAEVENT dmaEvent);
protected:
    DWORD           m_dwNumOfTables;
    PADMA2_32_DESC  m_pDmaDescTables[MAXIMUM_DESC_TABLES];
    DWORD           m_dwDescTablePhysAddr[MAXIMUM_DESC_TABLES];

    BOOL    IsEnoughDescTable(DWORD dwNumOfBlock);
};
