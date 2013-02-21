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
    SDHCDMA.cpp
Abstract:
    Standard SDHC Driver DMA object implementation.

Notes: 
--*/

#include "SDHC.h"
#include "sdhcdma.hpp"
#include "SDHCSlot.h"

CSDHCSlotBaseDMA::CSDHCSlotBaseDMA (CSDHCSlotBase& SDHCSloteBase)
:   m_SDHCSloteBase (SDHCSloteBase)
{
    m_dwNumOfList = 0;
    m_dwNumOfAvailabe = 0;
    m_pDmaBufferList = NULL;
    m_dwCurDMAListPos = 0;
    m_fDMAProcessing = FALSE;
    m_dwDMACompletionCode = ERROR_SUCCESS;
    
    m_hDma = NULL;
    memset(&m_dmaAdapter,0,sizeof(m_dmaAdapter));
    m_dmaAdapter.Size = sizeof(m_dmaAdapter);
    m_dmaAdapter.BusMaster = TRUE;
    m_dmaAdapter.BusNumber = m_SDHCSloteBase.m_dwBusNumber;
    m_dmaAdapter.InterfaceType = m_SDHCSloteBase.m_interfaceType ;
};
BOOL CSDHCSlotBaseDMA::Init()
{
    m_pDmaBufferList = new CE_DMA_BUFFER_BLOCK[INITIAL_DMA_LIST];
    if (m_pDmaBufferList) {
        m_dwNumOfList = INITIAL_DMA_LIST ;
    }
    return (m_pDmaBufferList!=NULL && m_dwNumOfList != 0);
}

CSDHCSlotBaseDMA::~CSDHCSlotBaseDMA()
{
    if (m_pDmaBufferList)
        delete [] m_pDmaBufferList;
    if (m_hDma) {
        DMACloseBuffer(m_hDma);
    }
    
}

DWORD   CSDHCSlotBaseDMA::ReAllocateDMABufferList(DWORD dwRequired)
{
    if (m_pDmaBufferList && m_dwNumOfList < dwRequired) {
        delete [] m_pDmaBufferList;
        m_dwNumOfList = 0;
    }
    if (m_pDmaBufferList == NULL && dwRequired!= 0) {
        m_pDmaBufferList = new CE_DMA_BUFFER_BLOCK[dwRequired];
        if (m_pDmaBufferList) {
            m_dwNumOfList = dwRequired;
        }
    }
    return m_dwNumOfList;
}
BOOL CSDHCSlotBaseDMA::CancelDMA()
{
    if (!m_fDMAProcessing) {
        m_fDMAProcessing = FALSE;
        m_dwCurDMAListPos = m_dwNumOfAvailabe;
        m_dwDMACompletionCode = ERROR_CANCELLED ;
    }
    if (!m_fDMAProcessing && m_hDma) {
        DMACloseBuffer(m_hDma);
        m_hDma = NULL;
    }
    return TRUE;
}

BOOL CSDHCSlotBaseDMA::ArmDMA(SD_BUS_REQUEST& Request,BOOL fToDevice) 
{
    ASSERT(!m_fDMAProcessing);
    ASSERT(m_hDma==NULL);
    BOOL fReturn = FALSE;

    // Check for the limitaion.
    if (Request.NumBlocks > 1 && (Request.BlockSize & (sizeof(DWORD)-1)) == 0 
            && ((DWORD)Request.pBlockBuffer & (sizeof(DWORD)-1)) == 0
            && m_hDma == NULL) {
        // We are going to transfer this block as DMA.
        if ( (Request.Flags & SD_BUS_REQUEST_PHYS_BUFFER)!=0 && 
            Request.cbSizeOfPhysList!=0 && Request.pPhysBuffList!=NULL) {
            // We have user passed in Physical Buffer List.
            if (ReAllocateDMABufferList(Request.cbSizeOfPhysList) >= Request.cbSizeOfPhysList) {
                //Copy the Buffer.
                ASSERT(Request.cbSizeOfPhysList <= m_dwNumOfList);
                m_dwNumOfAvailabe = Request.cbSizeOfPhysList ;
                PBYTE pVirtualAddr = Request.pBlockBuffer ;
                for (DWORD dwIndex = 0; dwIndex < Request.cbSizeOfPhysList && dwIndex< m_dwNumOfList; dwIndex ++) {
                    m_pDmaBufferList[dwIndex].dwLength = Request.pPhysBuffList[dwIndex].PhysLen;
                    m_pDmaBufferList[dwIndex].physicalAddress = Request.pPhysBuffList[dwIndex].PhysAddr;
                    m_pDmaBufferList[dwIndex].virtualAddress = pVirtualAddr + m_pDmaBufferList[dwIndex].dwLength;
                }
                fReturn = TRUE;
                
            }
        }
        else { // We need figure out the physical address by using CEDDK DMA function.
            DWORD dwLength = Request.BlockSize * Request.NumBlocks;
            m_dmaAdapter.dwFlags =(fToDevice? DMA_FLAGS_WRITE_TO_DEVICE: 0);
//            if (!fToDevice) {
//                memset(Request.pBlockBuffer,0xc5,dwLength);
//            }
            m_hDma = DMAOpenBuffer(&m_dmaAdapter,1, (PVOID *)&Request.pBlockBuffer,&dwLength);
            if (m_hDma) {
                m_dwNumOfAvailabe = DMAGetBufferPhysAddr(m_hDma, m_dwNumOfList, m_pDmaBufferList);
                if (m_dwNumOfAvailabe>m_dwNumOfList && ReAllocateDMABufferList(m_dwNumOfAvailabe)>=m_dwNumOfAvailabe) {
                    m_dwNumOfAvailabe = DMAGetBufferPhysAddr(m_hDma, m_dwNumOfList, m_pDmaBufferList);
                }
                if (m_dwNumOfAvailabe<= m_dwNumOfList) {
                    fReturn = TRUE;
                }
                else { // FAILED.
                    m_dwNumOfAvailabe = 0;
                    DMACloseBuffer(m_hDma);
                    m_hDma = NULL;
                }
            }
        }
        ASSERT(fReturn);        
    }
    return fReturn;
}

CSDHCSlotBaseSDMA::~CSDHCSlotBaseSDMA()
{
   if (m_StartBuffer.pBufferedVirtualAddr!=NULL) {
        OALDMAFreeBuffer(&m_dmaAdapter, PAGE_SIZE,m_StartBuffer.physicalAddress,m_StartBuffer.pBufferedVirtualAddr,FALSE);
        //OALDMAFreeBuffer(&m_dmaAdapter, PAGE_SIZE*128,m_StartBuffer.physicalAddress,m_StartBuffer.pBufferedVirtualAddr,FALSE);
    }
    if (m_EndBuffer.pBufferedVirtualAddr!=NULL) {
        OALDMAFreeBuffer(&m_dmaAdapter, PAGE_SIZE,m_EndBuffer.physicalAddress,m_EndBuffer.pBufferedVirtualAddr,FALSE);
        //OALDMAFreeBuffer(&m_dmaAdapter, PAGE_SIZE*128,m_EndBuffer.physicalAddress,m_EndBuffer.pBufferedVirtualAddr,FALSE);
    }
    if (m_fLocked )
        UnlockPages( m_lpvLockedAddress, m_dwLockedSize);
    m_fLocked = FALSE;
}

BOOL CSDHCSlotBaseSDMA::Init()  
{
    m_fLocked= FALSE;
    if (!CeGetCacheInfo(sizeof(m_ceCacheInfo), &m_ceCacheInfo)) {
        ASSERT(FALSE);
        return FALSE;
    }
    else  {
        m_StartBuffer.pBufferedVirtualAddr = OALDMAAllocBuffer(&m_dmaAdapter, PAGE_SIZE , &m_StartBuffer.physicalAddress, FALSE );
        m_StartBuffer.dwBufferOffset = 0;
        m_StartBuffer.dwBufferSize = PAGE_SIZE;
        m_StartBuffer.pSrcVirtualAddr = NULL;
        m_StartBuffer.pSrcSize = 0;
        
        m_EndBuffer.pBufferedVirtualAddr = OALDMAAllocBuffer(&m_dmaAdapter, PAGE_SIZE , &m_EndBuffer.physicalAddress, FALSE );
        m_EndBuffer.dwBufferOffset = 0;
        m_EndBuffer.dwBufferSize = PAGE_SIZE;
        m_EndBuffer.pSrcVirtualAddr = NULL;
        m_EndBuffer.pSrcSize = 0;
        return (m_StartBuffer.pBufferedVirtualAddr!=NULL &&  m_EndBuffer.pBufferedVirtualAddr!=NULL && CSDHCSlotBaseDMA::Init());
    }
};


BOOL CSDHCSlotBaseSDMA::GetDMABuffer(SD_BUS_REQUEST& Request,BOOL fToDevice)
{
  // We shouldn't use PAGE_SHIFT in CEDDK because it is too confusing and too dangerouse.
#define PFN_SHIEFT UserKInfo[KINX_PFN_SHIFT]
#define MAX_SUPPORTED_PFN (MAXDWORD>>PFN_SHIEFT)
#define PAGE_MASK (PAGE_SIZE-1)
  ASSERT(!m_fDMAProcessing);
  ASSERT(m_hDma==NULL);
  ASSERT(m_fLocked == FALSE);
  m_StartBuffer.pSrcVirtualAddr = NULL;
  m_StartBuffer.pSrcSize = 0;
  m_EndBuffer.pSrcVirtualAddr = NULL;
  m_EndBuffer.pSrcSize = 0;
  BOOL fReturn = FALSE;
  // Check for the limitaion.
  if (Request.NumBlocks > 1 && (Request.BlockSize & (sizeof(DWORD)-1)) == 0 
      && ((DWORD)Request.pBlockBuffer & (sizeof(DWORD)-1)) == 0
      && m_hDma == NULL) {
    // We are going to transfer this block as DMA.
    if ( (Request.Flags & SD_BUS_REQUEST_PHYS_BUFFER)!=0 && 
        Request.cbSizeOfPhysList!=0 && Request.pPhysBuffList!=NULL) {
      // We have user passed in Physical Buffer List.
      if (ReAllocateDMABufferList(Request.cbSizeOfPhysList) >= Request.cbSizeOfPhysList) {
        //Copy the Buffer.
        ASSERT(Request.cbSizeOfPhysList <= m_dwNumOfList);
        m_dwNumOfAvailabe = Request.cbSizeOfPhysList ;
        PBYTE pVirtualAddr = Request.pBlockBuffer ;
        for (DWORD dwIndex = 0; dwIndex < Request.cbSizeOfPhysList && dwIndex< m_dwNumOfList; dwIndex ++) {
          m_pDmaBufferList[dwIndex].dwLength = Request.pPhysBuffList[dwIndex].PhysLen;
          m_pDmaBufferList[dwIndex].physicalAddress = Request.pPhysBuffList[dwIndex].PhysAddr;
          m_pDmaBufferList[dwIndex].virtualAddress = pVirtualAddr + m_pDmaBufferList[dwIndex].dwLength;
        }
        fReturn = TRUE;

      }
    }
    else { // We need figure out the physical address by using CEDDK DMA function.
      DWORD dwLength = Request.BlockSize * Request.NumBlocks;
      PVOID pUseBufferPtr = Request.pBlockBuffer;
      m_dmaAdapter.dwFlags =(fToDevice? DMA_FLAGS_WRITE_TO_DEVICE: 0);
      //            if (!fToDevice) {
      //                memset(Request.pBlockBuffer,0xc5,dwLength);
      //            }

      m_dwNumOfAvailabe = ADDRESS_AND_SIZE_TO_SPAN_PAGES( pUseBufferPtr, dwLength );
      PDWORD pdwPhysAddress = new DWORD[m_dwNumOfAvailabe];
      if (pdwPhysAddress) {
        if (m_dwNumOfAvailabe> m_dwNumOfList) {
          ReAllocateDMABufferList(m_dwNumOfAvailabe);
        }
        if (m_dwNumOfAvailabe<= m_dwNumOfList ) {
          m_fLocked = LockPages( pUseBufferPtr,dwLength,pdwPhysAddress, // m_pdwPhysAddress to stall PFN temperory.
              fToDevice? LOCKFLAG_READ: LOCKFLAG_WRITE);
          if (m_fLocked) { // Create table for Physical Address and length.
            m_lpvLockedAddress = pUseBufferPtr;
            m_dwLockedSize = dwLength;

            fReturn = TRUE;
            for (DWORD dwIndex = 0; dwIndex< m_dwNumOfAvailabe; dwIndex++) {
              if (pdwPhysAddress[dwIndex] > MAX_SUPPORTED_PFN) {
                ASSERT(FALSE);
                fReturn = FALSE;
                break;
              }
              else {
                m_pDmaBufferList[dwIndex].dwLength = min((PAGE_SIZE - ((DWORD)pUseBufferPtr & PAGE_MASK)),dwLength);
                m_pDmaBufferList[dwIndex].physicalAddress.LowPart = (pdwPhysAddress[dwIndex]<<PFN_SHIEFT) + ((DWORD)pUseBufferPtr & PAGE_MASK);
                m_pDmaBufferList[dwIndex].physicalAddress.HighPart = 0;
                m_pDmaBufferList[dwIndex].virtualAddress = (PBYTE)pUseBufferPtr;
                dwLength -= m_pDmaBufferList[dwIndex].dwLength;
                pUseBufferPtr = (PBYTE)pUseBufferPtr+m_pDmaBufferList[dwIndex].dwLength;
              }
            };
          }

        }
        delete pdwPhysAddress;
      }
      if (fReturn && m_dwNumOfAvailabe) { // Check for Cache aligh begin and end.
        if (!fToDevice) {
          DWORD dwDCacheLineSize = GetDataCacheSize() ;
          ASSERT ((dwDCacheLineSize & dwDCacheLineSize-1) == 0 ); // I has to be nature align.
          // First Block
          if ((m_pDmaBufferList[0].physicalAddress.LowPart & (dwDCacheLineSize-1)) != 0) { // Not cache align. we have to do something.
            m_StartBuffer.dwBufferOffset = (m_pDmaBufferList[0].physicalAddress.LowPart & PAGE_MASK);
            m_StartBuffer.pSrcSize = m_pDmaBufferList[0].dwLength;
            m_StartBuffer.pSrcVirtualAddr = m_pDmaBufferList[0].virtualAddress;
            m_pDmaBufferList[0].physicalAddress.QuadPart = m_StartBuffer.physicalAddress.QuadPart + m_StartBuffer.dwBufferOffset;
            //m_pDmaBufferList[0].virtualAddress = (PBYTE)m_StartBuffer.pBufferedVirtualAddr + m_StartBuffer.dwBufferOffset;
          }
          if(m_dwNumOfAvailabe!=0 && (m_pDmaBufferList[m_dwNumOfAvailabe-1].dwLength & (dwDCacheLineSize-1))!=0) {
            m_EndBuffer.dwBufferOffset = 0 ;
            m_EndBuffer.pSrcSize = m_pDmaBufferList[m_dwNumOfAvailabe-1].dwLength;
            m_EndBuffer.pSrcVirtualAddr = m_pDmaBufferList[m_dwNumOfAvailabe-1].virtualAddress;
            m_pDmaBufferList[m_dwNumOfAvailabe-1].physicalAddress = m_EndBuffer.physicalAddress;
            //m_pDmaBufferList[m_dwNumOfAvailabe-1].virtualAddress = m_EndBuffer.pBufferedVirtualAddr;
          }
        }
      }
      else {
        fReturn = FALSE;
        if (m_fLocked) {
          UnlockPages(m_lpvLockedAddress, m_dwLockedSize);
        };
        m_fLocked = FALSE;
      }
      if (fReturn && fToDevice) {
        for (DWORD dwIndex = 0 ; dwIndex<m_dwNumOfAvailabe ; dwIndex++) {
          CacheRangeFlush(m_pDmaBufferList[dwIndex].virtualAddress,m_pDmaBufferList[dwIndex].dwLength, CACHE_SYNC_WRITEBACK );
        }
      }
    }
    ASSERT(fReturn);        
  }
  return fReturn;
}
BOOL CSDHCSlotBaseSDMA::ArmDMA(SD_BUS_REQUEST& Request,BOOL fToDevice )
{
  BOOL fResult = GetDMABuffer(Request,fToDevice);//CSDHCSlotBaseDMA::ArmDMA(Request,fToDevice);
  if (fResult) {
    m_fDMAProcessing = TRUE;
    m_dwDMACompletionCode = ERROR_IO_PENDING ;
    m_dwCurDMAListPos = 0;
    ASSERT(m_dwNumOfAvailabe!=0);
    // Arm Buffer Bound
    DWORD dwArmBit= 0;
    m_dwNextOffset = GetDMALengthBit(m_pDmaBufferList[m_dwCurDMAListPos].physicalAddress.LowPart, m_pDmaBufferList[m_dwCurDMAListPos].dwLength);
    ASSERT(m_dwNextOffset<= m_pDmaBufferList[m_dwCurDMAListPos].dwLength);

    // Arm the first buffer.
    m_SDHCSloteBase.WriteDword(SDHC_SYSTEMADDRESS_LO, m_pDmaBufferList[m_dwCurDMAListPos].physicalAddress.LowPart );
  }
  return fResult;
}
BOOL CSDHCSlotBaseSDMA::DMANotifyEvent(SD_BUS_REQUEST& Request, DMAEVENT dmaEvent)
{
  switch (dmaEvent) {
    case DMA_COMPLETE:
      if (m_fDMAProcessing && m_pDmaBufferList && m_dwCurDMAListPos<m_dwNumOfAvailabe) {
        if (m_dwNextOffset < m_pDmaBufferList[m_dwCurDMAListPos].dwLength) { // re-arm the same.
          DWORD dwNewAddr = m_pDmaBufferList[m_dwCurDMAListPos].physicalAddress.LowPart + m_dwNextOffset;
          DWORD dwLength = GetDMALengthBit(dwNewAddr, m_pDmaBufferList[m_dwCurDMAListPos].dwLength-m_dwNextOffset);
          ASSERT(dwLength + m_dwNextOffset <= m_pDmaBufferList[m_dwCurDMAListPos].dwLength);
          m_dwNextOffset +=  dwLength;
          m_SDHCSloteBase.WriteDword(SDHC_SYSTEMADDRESS_LO,dwNewAddr);
        }
        else { // next entry,
          // Update HCParam
          Request.HCParam += m_pDmaBufferList[m_dwCurDMAListPos].dwLength;
          m_dwNextOffset = 0;

          m_dwCurDMAListPos++;
          if (m_dwCurDMAListPos < m_dwNumOfAvailabe) { // Continue for next
            m_dwNextOffset = GetDMALengthBit(m_pDmaBufferList[m_dwCurDMAListPos].physicalAddress.LowPart, m_pDmaBufferList[m_dwCurDMAListPos].dwLength);
            ASSERT(m_dwNextOffset<= m_pDmaBufferList[m_dwCurDMAListPos].dwLength);
            m_SDHCSloteBase.WriteDword(SDHC_SYSTEMADDRESS_LO, m_pDmaBufferList[m_dwCurDMAListPos].physicalAddress.LowPart);
          }
          else {
            m_fDMAProcessing = FALSE;
            ASSERT(FALSE); // DMA has been completed.
          }
        }
      }
      else {
        ASSERT(m_dwNumOfAvailabe == m_dwCurDMAListPos);
        m_fDMAProcessing = FALSE;
      }
      break;
    case TRANSFER_COMPLETED:
      ASSERT(m_dwCurDMAListPos <= m_dwNumOfAvailabe);
      if (m_fDMAProcessing && m_pDmaBufferList && m_dwCurDMAListPos<m_dwNumOfAvailabe) {
        Request.HCParam += m_pDmaBufferList[m_dwCurDMAListPos].dwLength ;
        m_dwCurDMAListPos ++;
      }
      m_fDMAProcessing = FALSE;
      break;
    case DMA_ERROR_OCCOR:
    default:
      ASSERT(FALSE);
      m_dwDMACompletionCode = ERROR_NOT_READY;
      m_fDMAProcessing = FALSE;
      break;
  }
  if (!m_fDMAProcessing && m_hDma) {
    DMACloseBuffer(m_hDma);
    m_hDma = NULL;
  }
  else if (!m_fDMAProcessing && m_fLocked) {
    if (!TRANSFER_IS_WRITE(&Request)) {
      for (DWORD dwIndex = 0 ; dwIndex<m_dwNumOfAvailabe ; dwIndex++) 
      {
        CacheRangeFlush(m_pDmaBufferList[dwIndex].virtualAddress,m_pDmaBufferList[dwIndex].dwLength, CACHE_SYNC_DISCARD );
      }
      if (m_StartBuffer.pSrcVirtualAddr!=NULL && m_StartBuffer.pSrcSize!=0) {
        CeSafeCopyMemory (m_StartBuffer.pSrcVirtualAddr,
            (PBYTE)m_StartBuffer.pBufferedVirtualAddr+ m_StartBuffer.dwBufferOffset,
            m_StartBuffer.pSrcSize);
      }
      if (m_EndBuffer.pSrcVirtualAddr!=NULL && m_EndBuffer.pSrcSize!=0) {
        CeSafeCopyMemory (m_EndBuffer.pSrcVirtualAddr, m_EndBuffer.pBufferedVirtualAddr,m_EndBuffer.pSrcSize);
      }
    }
    if (m_fLocked )
      UnlockPages( m_lpvLockedAddress, m_dwLockedSize);
    m_fLocked = FALSE;
  }
  return TRUE;
}

CSDHCSlotBase32BitADMA2::CSDHCSlotBase32BitADMA2(CSDHCSlotBase& SDHCSloteBase)
:    CSDHCSlotBaseDMA(SDHCSloteBase)
{
    DEBUGMSG(SDCARD_ZONE_INIT, (_T("CSDHCSlotBase32BitADMA2:Create DMA Object for SDMA\r\n")));        
    m_dwNumOfTables =0;
    for (DWORD dwIndex=0; dwIndex < MAXIMUM_DESC_TABLES; dwIndex++) {
        m_pDmaDescTables[dwIndex] = NULL;
        m_dwDescTablePhysAddr[dwIndex] = 0 ;
    }
        
};
CSDHCSlotBase32BitADMA2::~CSDHCSlotBase32BitADMA2()
{
    for (DWORD dwIndex=0; dwIndex< m_dwNumOfTables; dwIndex++) {
        ASSERT(m_pDmaDescTables[dwIndex]);
        ASSERT(m_dwDescTablePhysAddr[dwIndex]);
        PHYSICAL_ADDRESS LogicalAddress = {m_dwDescTablePhysAddr[dwIndex],0};
        OALDMAFreeBuffer(&m_dmaAdapter,PAGE_SIZE,LogicalAddress,m_pDmaDescTables[dwIndex],FALSE);
    }
}
BOOL CSDHCSlotBase32BitADMA2::Init()
{
    if (CSDHCSlotBaseDMA::Init()) {
        PHYSICAL_ADDRESS LogicalAddress;
        ASSERT(m_dwNumOfTables==0);
        if (m_dwNumOfTables<MAXIMUM_DESC_TABLES) {
            m_pDmaDescTables[m_dwNumOfTables] = (PADMA2_32_DESC) OALDMAAllocBuffer(&m_dmaAdapter, PAGE_SIZE , &LogicalAddress, FALSE );
            if (m_pDmaDescTables[m_dwNumOfTables]) {
                m_dwDescTablePhysAddr[m_dwNumOfTables] = LogicalAddress.LowPart; // We are using 32 bit address.
                m_dwNumOfTables++;
            }
        }
        ASSERT(m_dwNumOfTables!=0);
        return (m_dwNumOfTables!=0);
    }
    return FALSE;
}
BOOL CSDHCSlotBase32BitADMA2::ArmDMA(SD_BUS_REQUEST& Request,BOOL fToDevice )
{
    BOOL fResult = FALSE ;
    if (CSDHCSlotBaseDMA::ArmDMA(Request,fToDevice) 
            && m_dwNumOfAvailabe 
            && IsEnoughDescTable(m_dwNumOfAvailabe) ) {

        DWORD dwCurTable = 0 ;
        DWORD dwCurEntry = 0 ;
        DWORD dwCurPhysicalPage = 0;
        while (dwCurPhysicalPage < m_dwNumOfAvailabe) {
            PADMA2_32_DESC pCurTable = m_pDmaDescTables[dwCurTable];
            PADMA2_32_DESC pCurEntry = pCurTable + dwCurEntry;
            // Setup Descriptor
            pCurEntry->Valid = 1 ;
            pCurEntry->End = 0 ;
            pCurEntry->Int = 0 ;
            pCurEntry->Act = 2 ; // Transfer.
            pCurEntry->Length = m_pDmaBufferList[dwCurPhysicalPage].dwLength;
            pCurEntry->Address = m_pDmaBufferList[dwCurPhysicalPage].physicalAddress.LowPart;

            dwCurPhysicalPage++;
            
            if (dwCurPhysicalPage < m_dwNumOfAvailabe) { // We have more
                dwCurEntry++;
                if (dwCurEntry>= DESC_ENTRY_PER_TABLE -1 ) { // We reserv last one for Link Descriptor.
                    pCurEntry = pCurTable+dwCurEntry;
                    // Setup link.
                    pCurEntry->Valid = 1 ;
                    pCurEntry->End = 0 ;
                    pCurEntry->Int = 0 ;
                    pCurEntry->Act = 3 ; // Link
                    pCurEntry->Length = 0;
                    pCurEntry->Address = m_dwDescTablePhysAddr[dwCurTable+1];
                    dwCurTable ++; 
                    dwCurEntry = 0;
                    
                    if (dwCurTable>=m_dwNumOfTables) { // For some reason we exceed.
                        ASSERT(FALSE);
                        break;
                    }
                }
            }
            else { // We finished here.
                // Change this link to end
                pCurEntry->End = 1;
                fResult = TRUE;
                break;
            }

        }
        // Arm the first buffer.
        if (fResult) {
            m_fDMAProcessing = TRUE;
            m_SDHCSloteBase.WriteDword(SDHC_ADMA_SYSTEMADDRESS_LO, m_dwDescTablePhysAddr[0] ); // 32-bit address.
            m_SDHCSloteBase.WriteDword(SDHC_ADMA_SYSTEMADDRESS_HI, 0 );
        }
        else {
            ASSERT(FALSE);
        }

    }
    return fResult;
}
BOOL CSDHCSlotBase32BitADMA2::IsEnoughDescTable(DWORD dwNumOfBlock)
{
    DWORD dwNumOfEntryPerTable = DESC_ENTRY_PER_TABLE -1; // we reserv one for the link.
    DWORD dwNumOfTable = (dwNumOfBlock+dwNumOfEntryPerTable-1)/dwNumOfEntryPerTable;
    if (dwNumOfTable> MAXIMUM_DESC_TABLES){
        return FALSE;
        }
    if (dwNumOfTable> m_dwNumOfTables) { // we need allocate more
        for (DWORD dwIndex = m_dwNumOfTables; dwIndex< dwNumOfTable; dwIndex++) {
            PHYSICAL_ADDRESS LogicalAddress;
            m_pDmaDescTables[m_dwNumOfTables] = (PADMA2_32_DESC) OALDMAAllocBuffer(&m_dmaAdapter, PAGE_SIZE , &LogicalAddress, FALSE );
            if (m_pDmaDescTables[m_dwNumOfTables]) {
                m_dwDescTablePhysAddr[m_dwNumOfTables] = LogicalAddress.LowPart; // We are using 32 bit address.
                m_dwNumOfTables++;
            }
            else
                break;
        }
        if (dwNumOfTable!=m_dwNumOfTables) {
            ASSERT(FALSE);
            return FALSE;
        }
    }
    return TRUE;
    
}

BOOL CSDHCSlotBase32BitADMA2::DMANotifyEvent(SD_BUS_REQUEST& Request, DMAEVENT dmaEvent)
{
    switch (dmaEvent) {
        ASSERT(FALSE);
        break;
      case TRANSFER_COMPLETED:
        ASSERT(m_dwCurDMAListPos <= m_dwNumOfAvailabe);
        if (m_fDMAProcessing && m_pDmaBufferList && m_dwCurDMAListPos<m_dwNumOfAvailabe) {
            Request.HCParam = Request.BlockSize*Request.NumBlocks;
        }
        m_fDMAProcessing = FALSE;
        break;
      case DMA_ERROR_OCCOR:
        m_fDMAProcessing = FALSE; {
        m_dwDMACompletionCode = ERROR_NOT_READY;
        
        BYTE ADMAErrorStatus = m_SDHCSloteBase.ReadByte(SDHC_ADMA_ERROR_STATUS);
        DEBUGMSG(SDCARD_ZONE_ERROR,(TEXT("ADMA Erorr Status 0x%x: Refer to 2.2.30"), ADMAErrorStatus));
        }
        break;
      default:
      case DMA_COMPLETE:
        ASSERT(FALSE);
        m_dwDMACompletionCode = ERROR_NOT_READY;
        m_fDMAProcessing = FALSE;
        break;
    }
    if (!m_fDMAProcessing && m_hDma) {
        DMACloseBuffer(m_hDma);
        m_hDma = NULL;
    }
    return TRUE;
}

