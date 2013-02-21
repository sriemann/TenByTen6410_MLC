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
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    SDSlot.cpp
Abstract:
    SDSlot Implementation.

Notes: 
--*/

#include <windows.h>
#include <types.h>

#include "../HSMMCCh1/s3c6410_hsmmc_lib/sdhcd.h"

#include "sdbus.hpp"
#include "sdslot.hpp"
#include "sdbusreq.hpp"
#include "sddevice.hpp"


CSDBusReqAsyncQueue::CSDBusReqAsyncQueue()
{
    m_pQueueListHead = m_pQueueListLast = NULL;
    m_fAttached = FALSE;
}
CSDBusReqAsyncQueue::~CSDBusReqAsyncQueue()
{
    Lock();
    while (m_pQueueListHead) {
        CSDBusRequest * pNext = m_pQueueListHead->GetAsyncQueueNext();
        m_pQueueListHead->DeRef();
        m_pQueueListHead = pNext;
    }
    Unlock();
}
BOOL CSDBusReqAsyncQueue::Init()
{
    m_fAttached = TRUE;
    return TRUE;
}
BOOL CSDBusReqAsyncQueue::Detach()
{
    m_fAttached = FALSE;
    RemoveAllRequest();
    if (m_pQueueListHead!=NULL) { // I don't think HC are going to complete this one. So, let me do it here.
        Lock();
        CSDBusRequest *  pCur = m_pQueueListHead;
        while(pCur) {
            CSDBusRequest *pNext = pCur->GetAsyncQueueNext();
            pCur->SetAsyncQueueNext(NULL);
            pCur->CompleteBusRequest(SD_API_STATUS_CANCELED);
            pCur->DeRef();
            pCur = pNext;
        }
        m_pQueueListLast = m_pQueueListHead = NULL ;;
        Unlock();
        
    }
    return TRUE;
}

SD_API_STATUS CSDBusReqAsyncQueue::QueueBusRequest(CSDBusRequest * pRequest)
{
    SD_API_STATUS status = SD_API_STATUS_UNSUCCESSFUL;
    if (pRequest && m_fAttached) {
        BOOL fHeader = FALSE;
        Lock();
        pRequest->SetAsyncQueueNext(NULL);
        if (m_pQueueListHead!=NULL && m_pQueueListLast!=NULL) {
            m_pQueueListLast->SetAsyncQueueNext(pRequest);
            m_pQueueListLast = pRequest;
        }
        else {
            ASSERT(m_pQueueListHead==m_pQueueListLast);
            m_pQueueListHead = m_pQueueListLast = pRequest;
            fHeader = TRUE;
        }
        pRequest->AddRef();
        if (fHeader) {
            BOOL fSync= ((pRequest->SystemFlags & SD_FAST_PATH_AVAILABLE)!=0);
            status = SubmitRequestToHC(pRequest);
            if (!SD_API_SUCCESS(status)  || status == SD_API_STATUS_FAST_PATH_SUCCESS ) { 
                ASSERT(pRequest == m_pQueueListHead);
                ASSERT( pRequest->GetAsyncQueueNext()==NULL);
                // Failed or Fast pass completion,  CompleteRequest is never happen.
                if (fSync) {
                    m_pQueueListHead = pRequest->GetAsyncQueueNext();
                    if (m_pQueueListHead == NULL)
                        m_pQueueListLast = NULL;
                    pRequest->SetStatus(status);
                    pRequest->DeRef();                    
                }
                else
                  CompleteRequest(pRequest,fSync);
            }
        }
        else { // Async any always.
            pRequest->SystemFlags &= ~ SD_FAST_PATH_AVAILABLE;
            status = SD_API_STATUS_PENDING ;
        }
        Unlock();
    }
    else
        ASSERT(FALSE);
    return status;
};
CSDBusRequest * CSDBusReqAsyncQueue::CompleteRequest(CSDBusRequest * pRequest,SD_API_STATUS Status)
{
    CSDBusRequest * pReturn = NULL;
    Lock();
    if (m_pQueueListHead && m_pQueueListHead == pRequest) {
        pRequest->SetStatus(Status);
        if (!SD_API_SUCCESS(Status) && pRequest->IsRequestNeedRetry()) {
            pRequest->SetStatus(SD_API_STATUS_PENDING);
            pRequest->DecRetryCount();
            SD_API_STATUS status = SubmitRequestToHC(pRequest);
            if (!SD_API_SUCCESS(status)  || status == SD_API_STATUS_FAST_PATH_SUCCESS ) { 
                // Failed or Fast pass completion,  CompleteRequest is never happen.
                pReturn = CompleteRequest(pRequest,status ); // Yes recursive. It will stop either succeeded or try count to zero.
            }
        }
        else {
            pRequest->CompleteBusRequest(Status);
            pReturn = m_pQueueListHead;
            ASSERT((m_pQueueListHead == m_pQueueListLast) == (m_pQueueListHead->GetAsyncQueueNext()==NULL));
            CSDBusRequest * pNext = m_pQueueListHead->GetAsyncQueueNext();
            m_pQueueListHead->DeRef();
            m_pQueueListHead = pNext;
            if (m_pQueueListHead == NULL) {
                m_pQueueListLast = NULL;
            }
            else { // Submit Another one inside queue.
                SD_API_STATUS status = SubmitRequestToHC(m_pQueueListHead);
                if (!SD_API_SUCCESS(status)  || status == SD_API_STATUS_FAST_PATH_SUCCESS ) { 
                    // Failed or Fast pass completion,  CompleteRequest is never happen.
                    CSDBusRequest * pReturn1 = CompleteRequest(m_pQueueListHead,status ); // Yes recursive. It will stop either succeeded or try count to zero.
                }                
            }
        }
    }
    else { // Why this happens?
        ASSERT(FALSE);
    }
    
    Unlock();
    return pReturn;
}
CSDBusRequest * CSDBusReqAsyncQueue::RemoveRequest(CSDBusRequest * pRequest)
{
    CSDBusRequest * pReturn = NULL;
    Lock();
    CSDBusRequest * pPrev = NULL;
    CSDBusRequest * pCur = m_pQueueListHead;
    while (pCur!=NULL && pCur != pRequest) {
        pPrev = pCur;
        pCur = pCur->GetAsyncQueueNext();
    }
    if (pCur == pRequest) {
        pReturn = pCur ;
        if (pPrev!=NULL) {
            pPrev->SetAsyncQueueNext(pCur->GetAsyncQueueNext());
            if (pCur->GetAsyncQueueNext() == NULL) // Last one.
                m_pQueueListLast = pPrev;
            pCur->CompleteBusRequest(SD_API_STATUS_CANCELED);
            pCur->SetAsyncQueueNext(NULL);
            pCur->DeRef();
        }
        else { 
            // Delete from First?
            // Queue header can not remove from this function because it is processing by SDHC.
            CancelRequestFromHC(pRequest);
        }
    }
    else 
        ASSERT(FALSE);
    Unlock();
    return pReturn;
}
BOOL  CSDBusReqAsyncQueue::RemoveAllRequest()
{
    Lock();
    if (m_pQueueListHead) { 
        CSDBusRequest * pCur = m_pQueueListHead->GetAsyncQueueNext();
        while(pCur) {
            CSDBusRequest *pNext = pCur->GetAsyncQueueNext();
            pCur->CompleteBusRequest(SD_API_STATUS_CANCELED);
            pCur->DeRef();
            pCur = pNext;
        }
        m_pQueueListHead->SetAsyncQueueNext(NULL);
        m_pQueueListLast = m_pQueueListHead;
        CancelRequestFromHC(m_pQueueListHead);
    }
    Unlock();
    return TRUE;
}

CSDBusRequest * CSDBusReqAsyncQueue::GetRequestFromHead()
{
    Lock();
    CSDBusRequest * pReturn = m_pQueueListHead;
    if (pReturn)
        pReturn->AddRef();
    Unlock();
    return pReturn;
}




CSDSlot::CSDSlot(DWORD dwSlotIndex, CSDHost& sdHost)
:   m_SdHost(sdHost)
,   m_dwSlotIndex(dwSlotIndex)
,   CSDWorkItem()
{
    for (DWORD dwIndex = 0; dwIndex<SD_MAXIMUM_DEVICE_PER_SLOT; dwIndex++) 
        m_pFuncDevice[dwIndex] = NULL;
    m_SlotState = SlotInactive;
    m_curHCOwned = NULL;
    m_lcurHCLockCount = NULL;
    m_Flags = 0 ;
}
CSDSlot::~CSDSlot()
{
    ASSERT(!(m_SdHost.m_fHostAttached));
    RemoveAllDevice();
    ASSERT(m_curHCOwned==NULL);
}

BOOL CSDSlot::Init()
{
    // get the capabilities of this slot
    memset((SDCARD_HC_SLOT_INFO *)this, 0, sizeof (SDCARD_HC_SLOT_INFO));
    // ??Read Setting From Registry in future??
    //by default enable power control at the slot
    m_SlotState = SlotInactive; 
    m_fEnablePowerControl = TRUE;
    m_AllocatedPower = 0;
    DWORD dwThreadPrority = CSDHostContainer::RegValueDWORD(SDCARD_THREAD_PRIORITY_KEY,DEFAULT_THREAD_PRIORITY);
    if (!CSDWorkItem::Init(dwThreadPrority + 1) || !CSDBusReqAsyncQueue::Init()) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("CSDSlot::Init: CSDWorkItem::Init or CSDBusReqAsyncQueue::Init Failed  \n")));
        return FALSE;
    }
    m_FastPathThreshHold =  CSDHostContainer::RegValueDWORD(SDCARD_PASTPATH_THRESHOLD,DEFAULT_PASTPATH_THRESHOLD);
    return TRUE;
};

SD_API_STATUS CSDSlot::SubmitRequestToHC(CSDBusRequest * pRequest) 
{
  SD_API_STATUS status = SD_API_STATUS_UNSUCCESSFUL;
  m_SdHost.SDHCAccessLock();
  DEBUGMSG(SDCARD_ZONE_0,(TEXT("SubmitRequestToHC(%x)\n"),pRequest));
  if (m_fAttached && SD_API_SUCCESS(CheckSlotReady()) && m_SdHost.IsAttached() &&
      m_curHCOwned==NULL && pRequest!=NULL) {
    m_curHCOwned = pRequest;
    m_lcurHCLockCount = 0 ;
    status = m_SdHost.BusRequestHandler(m_dwSlotIndex,pRequest);
    // Retry if needed.
    while (!SD_API_SUCCESS(status)) {
      if (SD_API_SUCCESS(CheckSlotReady()) && pRequest->IsRequestNeedRetry()) {
        pRequest->DecRetryCount();
        status = m_SdHost.BusRequestHandler(m_dwSlotIndex,pRequest);        
      }
      else
        break;
    }
    if (status!=SD_API_STATUS_PENDING) { // This is has been completed.
      m_curHCOwned = NULL;
    }
  }
  else {
    ASSERT(FALSE);
  }
  m_SdHost.SDHCAccessUnlock();
  return status;
}

DWORD   CSDSlot::GetNumOfFunctionDevice()
{
    DWORD dwReturn = 0;
    m_SlotLock.Lock();
    for (DWORD dwIndex = 0; dwIndex< SD_MAXIMUM_DEVICE_PER_SLOT; dwIndex ++) {
        if (m_pFuncDevice[dwIndex]) {
            dwReturn++;
        }
    };
    m_SlotLock.Unlock();
    return dwReturn;
}

CSDDevice * CSDSlot::RemoveDevice(DWORD dwIndex )
{
    m_SlotLock.Lock();
    CSDDevice*  pReturn = NULL;
    if( dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT )
    {
        pReturn = m_pFuncDevice[dwIndex];
        m_pFuncDevice[dwIndex] = NULL;
    };
    m_SlotLock.Unlock();
    if( pReturn ) {   
        pReturn->Detach();
        pReturn->DeRef();
    }
    return pReturn;
}
CSDDevice * CSDSlot::InsertDevice(DWORD dwIndex,CSDDevice * pObject)
{
    CSDDevice*  pReturn = NULL;
    if( pObject )
    {
        m_SlotLock.Lock();
        if( dwIndex < SD_MAXIMUM_DEVICE_PER_SLOT  && m_pFuncDevice[dwIndex]==NULL  ) {
            pReturn = m_pFuncDevice[dwIndex] = pObject;
            pReturn->AddRef();
        }
        m_SlotLock.Unlock();
    }
    return pReturn;
}
BOOL  CSDSlot::RemoveAllDevice()
{
    for (LONG lIndex=SD_MAXIMUM_DEVICE_PER_SLOT-1; lIndex>=0 ; lIndex--) {
        if (m_pFuncDevice[lIndex]!=NULL)
            RemoveDevice(lIndex);
    }
    return TRUE;
}
BOOL CSDSlot::Attach()
{
    SD_API_STATUS status = m_SdHost.SlotOptionHandler( m_dwSlotIndex, SDHCDGetSlotInfo, (SDCARD_HC_SLOT_INFO *)this, sizeof(SDCARD_HC_SLOT_INFO));
    if (!SD_API_SUCCESS(status)) {
        DEBUGMSG(SDCARD_ZONE_ERROR, (TEXT("SDBusDriver: Failed to get slot info for slot %u\n"), m_dwSlotIndex));
        return FALSE;
    }
    return TRUE;
}
BOOL CSDSlot::Detach()
{
    return HandleRemoveDevice();
}

BOOL CSDSlot::HandleAddDevice()
{
   // Create Function Zero.
  ASSERT(m_pFuncDevice[0]==NULL) ;
  BOOL fResult = CSDBusReqAsyncQueue::Init();
  ASSERT(fResult);
  CSDDevice * psdDevice = new CSDDevice(0, *this);
  if (psdDevice && psdDevice->Init() && InsertDevice(0,psdDevice)) {
    CSDDevice * psdDevice = GetFunctionDevice(0);
    if (psdDevice ) {
      psdDevice->Attach();
      m_SlotState = SlotIdle ;
      DelayForPowerUp();
      // Default Value.
      memset(&m_SlotInterfaceEx,0,sizeof(m_SlotInterfaceEx));
      m_SlotInterfaceEx.ClockRate = SD_DEFAULT_CARD_ID_CLOCK_RATE;
      DWORD dwNumOfFunc = 0;
      if (SD_API_SUCCESS(SDSetCardInterfaceForSlot(&m_SlotInterfaceEx)) &&
          SD_API_SUCCESS(psdDevice->DetectSDCard(dwNumOfFunc))) {
        m_AllocatedPower = 0 ;
        if (SD_API_SUCCESS ( psdDevice->GetCardRegisters()) && 
            SD_API_SUCCESS(psdDevice->DeactivateCardDetect())&&
            SD_API_SUCCESS(psdDevice->SDGetSDIOPnpInformation(*psdDevice)) &&
            SD_API_SUCCESS(EnumMultiFunction(*psdDevice,dwNumOfFunc ))) {

          SD_API_STATUS status = SD_API_STATUS_SUCCESS; // status

          for (DWORD dwIndex = 0; dwIndex<dwNumOfFunc && SD_API_SUCCESS(status); dwIndex++) {
            CSDDevice * childDevice = GetFunctionDevice(dwIndex);
            if (childDevice) {
              status = childDevice->SelectCardInterface();
              ASSERT(SD_API_SUCCESS(status));
              childDevice->DeRef();
            }
            else 
              ASSERT(FALSE);
          }
          if (SD_API_SUCCESS(status))
            status = SelectSlotInterface();
              
          ASSERT(SD_API_SUCCESS(status));

          for (dwIndex = 0; dwIndex<dwNumOfFunc &&SD_API_SUCCESS(status); dwIndex++) {
            CSDDevice * childDevice = GetFunctionDevice(dwIndex);
            if (childDevice) {
              status = childDevice->SetCardInterface(&m_SlotInterfaceEx);
              ASSERT(SD_API_SUCCESS(status));
              childDevice->DeRef();
            }
            else 
              ASSERT(FALSE);
          }
          status = SDSetCardInterfaceForSlot(&m_SlotInterfaceEx);
          ASSERT(SD_API_SUCCESS(status));
#ifdef _MMC_SPEC_42_
          /**
           * Description : MMCplus support normaly DAT 8bit bus, but MMCmicro Card dose not Support 8Bit bus, so let bus width down
           */
          for (dwIndex = 0; dwIndex<dwNumOfFunc &&SD_API_SUCCESS(status); dwIndex++) {
            CSDDevice * childDevice = GetFunctionDevice(dwIndex);
            if (childDevice) {
              status = childDevice->SetMMCmicroInterface();
              ASSERT(SD_API_SUCCESS(status));
              childDevice->DeRef();
            }
            else 
              ASSERT(FALSE);
          }
#endif
          if (psdDevice->GetDeviceType() == Device_SD_IO) { // Remove First Function.
            psdDevice->SetDeviceType(Device_Unknown);
          }
          else if (psdDevice->GetDeviceType() == Device_SD_Combo) { // Remove First Function.
            psdDevice->SetDeviceType(Device_SD_Memory);
          }
          /* Start Testing Code.
             else if (psdDevice->GetDeviceType() == Device_SD_Memory) { // we do some test
             if (Capabilities & SD_SLOT_HIGH_SPEED_CAPABLE) {
             SD_CARD_INTERFACE_EX sdCardInterface = m_SlotInterfaceEx;
             sdCardInterface.InterfaceModeEx.bit.sdHighSpeed = 1;
             SD_API_STATUS status = psdDevice->SDSetCardFeature_I(SD_SET_CARD_INTERFACE_EX, &sdCardInterface, sizeof(sdCardInterface));
             ASSERT(SD_API_SUCCESS(status));
             }
             }
             */                     
          if (SD_API_SUCCESS(status)) {
            m_SlotState = SlotInitFailed;
            for (dwIndex = 0; dwIndex<dwNumOfFunc; dwIndex++) {
              CSDDevice * childDevice = GetFunctionDevice(dwIndex);
              DEBUGMSG(ZONE_ENABLE_INFO,(TEXT("HandleAddDevice: LoadDevice type = %d, slot %d"),
                    (childDevice!=NULL?childDevice->GetDeviceType():Device_Unknown),
                    m_dwSlotIndex));
              if (childDevice && childDevice->GetDeviceType()!=Device_Unknown ) {
                status = childDevice->SDLoadDevice();
                ASSERT(SD_API_SUCCESS(status));
                childDevice->DeRef();
              }
            }
          }
        }
      }
      psdDevice->DeRef();
    }
  }
  else if (psdDevice) {
    delete psdDevice;
  }
   return TRUE;
}

BOOL CSDSlot::HandleRemoveDevice()
{
    CSDBusReqAsyncQueue::Detach();
    RemoveAllDevice();
    ASSERT(m_curHCOwned==NULL);
    return TRUE;
}
BOOL CSDSlot::HandleDeviceInterrupting()
{
    SD_API_STATUS status = SD_API_STATUS_UNSUCCESSFUL;
    if ((m_Flags & SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED)!=0) {
        CSDDevice * psdDevice = GetFunctionDevice(0);
        UCHAR regValue = 0 ;
        if (psdDevice) {
            // fetch the interrupt pending register 
            status = psdDevice->SDReadWriteRegistersDirect_I(
                SD_IO_READ,          
                SD_IO_REG_INT_PENDING ,
                FALSE,
                &regValue,   // reg
                1);
            if (SD_API_SUCCESS(status) ) {
                regValue &= ~1; // Drop First Bit.
                while (regValue) {
                    UCHAR regValueLowestBit = ((regValue ^ (regValue -1)) & regValue) ;// Detect Lowest Bit.
                    DWORD dwIndexFunction = 0;
                    switch (regValueLowestBit) { // For optimize the speed.
                      default:
                        ASSERT(FALSE);
                      case (1<<1):
                        dwIndexFunction=1;
                        break;
                      case (1<<2):
                        dwIndexFunction=2;
                        break;
                      case (1<<3):
                        dwIndexFunction=3;
                        break;
                      case (1<<4):
                        dwIndexFunction=4;
                        break;
                      case (1<<5):
                        dwIndexFunction=5;
                        break;
                      case (1<<6):
                        dwIndexFunction=6;
                        break;
                      case (1<<7):
                        dwIndexFunction=7;
                        break;
                    }
                    regValue &= ~regValueLowestBit;
                    CSDDevice * psdSubDevice = GetFunctionDevice(dwIndexFunction);
                    if (psdSubDevice) {
                        psdSubDevice->HandleDeviceInterrupt();
                        psdSubDevice->DeRef();
                    }
                    else
                        ASSERT(FALSE);
                }
                
            }
            psdDevice->DeRef();
        }
    }
    // Ack Interupt
    GetHost().SlotOptionHandler(m_dwSlotIndex,SDHCDAckSDIOInterrupt,NULL,0);
    return TRUE;
}
// Enumerate multi function card.
SD_API_STATUS CSDSlot::EnumMultiFunction(CSDDevice& psdDevice0,DWORD dwNumOfFunc ) 
{
    SD_API_STATUS               status;             // status
    UCHAR                       currentFunction;    // current function
    BOOL                        isCombo = FALSE;    // is combo device flag


    // check the base type for multifunction or combo device
    if (Device_SD_IO ==psdDevice0.GetDeviceType() || Device_SD_Combo == psdDevice0.GetDeviceType()) {
        ASSERT(dwNumOfFunc >= 2);
        // set up functions 1..7, if present for the parent device
        for (currentFunction = 1;currentFunction < dwNumOfFunc; currentFunction++) {
            // create SDIO child devices for this parent
            status = CreateChildDevice( Device_SD_IO, currentFunction, psdDevice0);
            if (SD_API_SUCCESS(status)) {
                CSDDevice * pChildDevice = GetFunctionDevice(currentFunction);
                if (pChildDevice) {
                    status = pChildDevice->SDGetSDIOPnpInformation(psdDevice0);
                    if (SD_API_SUCCESS(status)) {
                        pChildDevice->SetupWakeupSupport();
                    }
                    pChildDevice->DeRef();
                }
                else {
                    status = SD_API_STATUS_UNSUCCESSFUL;
                }
            }
            if (!SD_API_SUCCESS(status)) {
                ASSERT(FALSE);
                return status;
            }
        }
        return SD_API_STATUS_SUCCESS;
    }
    else {
        return SD_API_STATUS_SUCCESS;
    }
}
SD_API_STATUS CSDSlot::CreateChildDevice(SDCARD_DEVICE_TYPE sdCard_DeviceType, DWORD dwFunctionIndex, CSDDevice& psdDevice0)
{
    SD_API_STATUS status = SD_API_STATUS_INVALID_PARAMETER;
    CSDDevice*  pNewDevice = new CSDDevice(dwFunctionIndex,*this);
    if (pNewDevice && pNewDevice->Init() && InsertDevice(dwFunctionIndex, pNewDevice)) {
        CSDDevice * psdDevice = GetFunctionDevice(dwFunctionIndex);
        if (psdDevice) {
            if (m_SdHost.IsAttached()) {
                psdDevice->Attach();
                psdDevice->CopyContentFromParent(psdDevice0);
                psdDevice->SetDeviceType(sdCard_DeviceType);
            }
            psdDevice->DeRef();
            status = SD_API_STATUS_SUCCESS ;
        }
        
    }
    else {
        ASSERT(FALSE);
        if (pNewDevice)
            delete pNewDevice;
        status = SD_API_STATUS_INSUFFICIENT_RESOURCES;
    }
    ASSERT(SD_API_SUCCESS(status));
    return status;
}
BOOL    CSDSlot::CancelRequestFromHC(CSDBusRequest * pRequest)
{
    BOOL fRetrun = FALSE;
    m_SdHost.SDHCAccessLock();
    DEBUGMSG(SDCARD_ZONE_0,(TEXT("CancelRequestFromHC(%x)"),pRequest));
    if (m_SdHost.IsAttached() && m_curHCOwned && m_lcurHCLockCount == 0 && pRequest == m_curHCOwned) {
        fRetrun = m_SdHost.CancelIOHandler(m_dwSlotIndex, pRequest );
    }
    m_SdHost.SDHCAccessUnlock();
    return fRetrun;
}

BOOL    CSDSlot::CompleteRequestFromHC(CSDBusRequest * pRequest,SD_API_STATUS status)
{
    BOOL fRetrun = FALSE;
    m_SdHost.SDHCAccessLock();
    DEBUGMSG(SDCARD_ZONE_0,(TEXT("CompleteRequestFromHC(%x,%x)\n"),pRequest,status));
    if ( m_curHCOwned && pRequest == m_curHCOwned) {
        m_curHCOwned = NULL;
        m_lcurHCLockCount = 0 ;
        CompleteRequest(pRequest, status );
        fRetrun = TRUE;
    }
    else {
        ASSERT(FALSE);
    }
    m_SdHost.SDHCAccessUnlock();
    return fRetrun;
}

CSDBusRequest * CSDSlot::SDHCGetAndLockCurrentRequest_I()
{
    CSDBusRequest * pReturn = NULL;
    m_SdHost.SDHCAccessLock();
    DEBUGMSG(SDCARD_ZONE_0,(TEXT("SDHCGetAndLockCurrentRequest_I,m_curHCOwned=%x\n"),m_curHCOwned));
    if (m_curHCOwned) {
        m_lcurHCLockCount++;
        pReturn = m_curHCOwned;
    }
    m_SdHost.SDHCAccessUnlock();
    return pReturn;
}
void  CSDSlot::SDHCDUnlockRequest_I(PSD_BUS_REQUEST  hReques) 
{
    m_SdHost.SDHCAccessLock();
    DEBUGMSG(SDCARD_ZONE_0,(TEXT("SDHCDUnlockRequest_I(%x), m_curHCOwned=%x\n"),hReques,m_curHCOwned));
    if (m_curHCOwned!=NULL && m_curHCOwned == hReques && m_lcurHCLockCount) {
        m_lcurHCLockCount--;
    }
    m_SdHost.SDHCAccessUnlock();
};


///////////////////////////////////////////////////////////////////////////////
// This routine is calling from seperate working thread.
VOID    CSDSlot::SlotStatusChangeProcessing(SD_SLOT_EVENT Event)
///////////////////////////////////////////////////////////////////////////////
{
    switch (Event) {
      case DeviceInterrupting:
        HandleDeviceInterrupting();
        break;
      case DeviceInserted:
        HandleAddDevice();
        break;
      case DeviceEjected:
        HandleRemoveDevice();
        break;
      case SlotDeselectRequest:
      case SlotSelectRequest:
      case SlotResetRequest:
        HandleSlotSelectDeselect(Event);
      break;
    default:
        DEBUG_CHECK(FALSE, (TEXT("SDBusDriver: SlotStatusChange Unknown Slot Event : %d \n"),Event));
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// SlotStateChange - indicate a change in the SD Slot 
//
// Input: pExternalHCContext - Host controller context that was previously registered
//        SlotNumber - Slot Number
//        Event      - new event
// Output:
// Return: SD_API_STATUS 
// Notes:       
//      A host controller driver calls this api when the slot changes state (i.e.
//      device insertion/deletion).
///////////////////////////////////////////////////////////////////////////////
VOID CSDSlot::SlotStateChange(SD_SLOT_EVENT Event)
{
    switch (Event) {
      case DeviceInterrupting: // We should do interrupt callback at SDHC IST.
      case DeviceInserted:
      case DeviceEjected:
      case SlotDeselectRequest:
      case SlotSelectRequest:
      case SlotResetRequest:
        // post this message to the work item for each slot
        PostEvent(Event);
        break;
      case BusRequestComplete:
        ASSERT(FALSE);
        break;
      default:
        DEBUGCHK(FALSE);
    }
}
///////////////////////////////////////////////////////////////////////////////
//  DelayForPowerUp - Delay for powerup 
//  Input:  pSlot - slot to delay on
//       
//  Output: 
//  Notes:  
//     
///////////////////////////////////////////////////////////////////////////////
VOID CSDSlot::DelayForPowerUp()
{
    // delay for power up
    if (PowerUpDelay != 0) {
        Sleep(PowerUpDelay);
    } else {
        // powerup delay should be set
        DEBUG_CHECK(FALSE, (TEXT("SDBusDriver: Slot Delay is 0!!!\n")));
        Sleep(500);
    }
}
///////////////////////////////////////////////////////////////////////////////
//  SDSetCardInterfaceForSlot - set Card interface for the slot
//  Input:  pSlot - the slot to set the card interface for
//          pSetting - card interface settings
//  Output:
//  Return: SD_API_STATUS
//  Notes:  
//         this function calls the slot option handler in the host controller
//         driver
//         
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDSlot::SDSetCardInterfaceForSlot(PSD_CARD_INTERFACE_EX pSetting)
{
    SD_API_STATUS   status;             // intermediate status
    DWORD           origClockSetting;   // original clock setting upon entry

    origClockSetting = pSetting->ClockRate;

    status = m_SdHost.SlotSetupInterface(m_dwSlotIndex,pSetting);

    if (SD_API_SUCCESS(status)) {
        if (origClockSetting != pSetting->ClockRate){
            DEBUGMSG(SDCARD_ZONE_WARN, (TEXT("SDSetCardInterfaceForSlot - HC ClockRate differs desired setting: desired: %d Hz, Actual : %d Hz \n"),
                origClockSetting, pSetting->ClockRate));
         }

    }
    return status;
}
///////////////////////////////////////////////////////////////////////////////
//  SDGetOperationalVoltageRange - Get the operation voltage range
//  Input:  OcrValue - OCR bit mask to determine the voltage range
//          pSlot - the slot
//          
//  Output:
//  Return: DWORD bit mask for optimal operational voltage
//  Notes:  
//         This function compares the OcrValue against the desired
//         voltage range of the slot and the capabilities of the slot
//         the operational value (also encoded as an OCR value) is returned
//         A value of zero indicates no usable voltage range.
///////////////////////////////////////////////////////////////////////////////
DWORD CSDSlot::SDGetOperationalVoltageRange(DWORD OcrValue)
{
    ULONG i;            // loop variable 

    // check to see if the voltages can be supported
    if (OcrValue & VoltageWindowMask) {
        // check to see if the voltage meets the desired voltage range
        if (OcrValue & DesiredVoltageMask) {
            DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Device Power Range:0x%08X matches HC desired power: 0x%08X \n"), 
                OcrValue, DesiredVoltageMask)); 
            // returned desired voltage range suggested by host controller 
            return DesiredVoltageMask;
        } else {
            // walk through the voltage mask starting at the low end looking for
            // a voltage that will work with the OCR value
            for (i = 0; i < 32; i++) {
                if (OcrValue & VoltageWindowMask & (1 << i)) {
                    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Device Power Range:0x%08X does not match HC desired power 0x%08X \n"),
                        OcrValue, DesiredVoltageMask));
                    DEBUGMSG(SDBUS_ZONE_DEVICE, (TEXT("Using negotiated power range: 0x%08X \n"), 
                        (1 << i))); 
                    // found a match
                    return (1 << i);
                }
            }
            // this should never happen, 
            DEBUGCHK(FALSE);
            return 0;
        }

    } else {
        return 0;
    }
}
///////////////////////////////////////////////////////////////////////////////
//  SlotCardSelectDeselect - Select/Deselect the card in teh slot
//  Input:  SlotIndexWanted - index of slot
//          Event           - SlotDeselectRequest/SlotSelectRequest/SlotResetRequest
//  Output: 
//  Notes:  
///////////////////////////////////////////////////////////////////////////////
BOOL CSDSlot::SlotCardSelectDeselect(SD_SLOT_EVENT Event) 
{
    SD_API_STATUS           status;             // intermediate SD status
    BOOL                    retStatus = FALSE;  // return status
    SD_SET_FEATURE_TYPE     CardFeature;        // card feature code

        // check for a ready device in the slot
    if (m_SlotState == Ready) {        
        CSDDevice *pDevice0 = GetFunctionDevice(0);
        if (NULL != pDevice0) { 

            switch ( Event ) {
              case SlotDeselectRequest:
                CardFeature = SD_CARD_DESELECT_REQUEST;
                break;
              case SlotSelectRequest:
                CardFeature = SD_CARD_SELECT_REQUEST;
                break;
              case SlotResetRequest:
                CardFeature = SD_CARD_FORCE_RESET;
                break;
              default:
                DEBUG_CHECK(FALSE, (TEXT("SDBusDriver: Unknown Slot Event : %d \n"),Event));
                return FALSE;
            }

            // Call API to request the card to be deselected
            status = pDevice0->SDSetCardFeature_I(CardFeature,NULL,0);
            pDevice0->DeRef();

            retStatus  = TRUE;

            if(!SD_API_SUCCESS(status)) {
                DbgPrintZo(SDCARD_ZONE_ERROR,(TEXT("SDBusDriver: Failed to request the card to be deselected\r\n")));
                retStatus = FALSE;
            }
        }
        else {
            retStatus = FALSE;
            DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("Driver not loaded")));
        }
    }
    else {
        retStatus = FALSE;
        DbgPrintZo(SDCARD_ZONE_INIT, (TEXT("Empty Slot")));
    }

    return retStatus;
}

///////////////////////////////////////////////////////////////////////////////
//  SelectSlotInterface - select slot interface based on default interface 
//                        information of all devices.
//  Input:  pSlot - the slot
//          
//  Output: 
//  Return: SD_API_STATUS code
//  Notes:  This function sets the slot's default interface in the slot structure,
//          the result interface will fit for all functions of the card.
///////////////////////////////////////////////////////////////////////////////
SD_API_STATUS CSDSlot::SelectSlotInterface()
{
  // Try slot interface at full speed and 4 bit mode (highest bus mode except High Speed)
  memset(&m_SlotInterfaceEx,0, sizeof(m_SlotInterfaceEx));
  m_SlotInterfaceEx.InterfaceModeEx.bit.sd4Bit = 1;
#ifdef _MMC_SPEC_42_
  /**
   * Description : HSMMC slot supports up to 52MHz
   */
  m_SlotInterfaceEx.ClockRate = HSMMC_FULL_SPEED_RATE;
#else
  m_SlotInterfaceEx.ClockRate = SD_FULL_SPEED_RATE;
#endif


  m_SlotLock.Lock();
  for (DWORD dwIndex=0; dwIndex<SD_MAXIMUM_DEVICE_PER_SLOT; dwIndex ++) {
    CSDDevice * psdDevice = m_pFuncDevice[dwIndex];
    // for each device set it's card interface
    if (psdDevice) {
#ifdef _MMC_SPEC_42_
      /**
       * Description : HSMMC Slot supports up to 52MHz
       */
      m_SlotInterfaceEx.InterfaceModeEx.bit.hsmmc8Bit = psdDevice->GetCardInterface().InterfaceModeEx.bit.hsmmc8Bit;

#endif                
      m_SlotInterfaceEx.InterfaceModeEx.bit.sd4Bit = psdDevice->GetCardInterface().InterfaceModeEx.bit.sd4Bit;
      m_SlotInterfaceEx.InterfaceModeEx.bit.sdWriteProtected = psdDevice->GetCardInterface().InterfaceModeEx.bit.sdWriteProtected;
      if ( psdDevice->GetCardInterface().ClockRate < m_SlotInterfaceEx.ClockRate ) {
        m_SlotInterfaceEx.ClockRate = psdDevice->GetCardInterface().ClockRate;
      }
    }
  }
  m_SlotLock.Unlock();
  return SD_API_STATUS_SUCCESS;
}

BOOL CSDSlot::SDSlotEnableSDIOInterrupts() 
{
    BOOL fRet = FALSE;
    m_SlotLock.Lock();
    if (!IsSlotInterruptOn()) {
        SD_API_STATUS status = m_SdHost.SlotOptionHandler(m_dwSlotIndex,SDHCDEnableSDIOInterrupts, NULL, 0);
        if (SD_API_SUCCESS(status)) {
            m_Flags|=SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED;
            fRet = TRUE;
        };
    }
    m_SlotLock.Unlock();
    return fRet;
}

BOOL CSDSlot::SDSlotDisableSDIOInterrupts()
{
    BOOL fRet = FALSE;
    m_SlotLock.Lock();
    if (IsSlotInterruptOn()) {
        SD_API_STATUS status = m_SdHost.SlotOptionHandler(m_dwSlotIndex,SDHCDDisableSDIOInterrupts, NULL, 0);
        if (SD_API_SUCCESS(status)) {
            m_Flags&=~SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED;
            fRet = TRUE;
        };
    }
    m_SlotLock.Unlock();
    return fRet;
}
BOOL CSDSlot::HandleSlotSelectDeselect(SD_SLOT_EVENT SlotEvent)
{
    SD_API_STATUS          status = SD_API_STATUS_SUCCESS;
    SD_CARD_INTERFACE_EX   CurrentSlotInterface;


    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("CSDSlot: HandleSlotSelectDeselect++: %d \n"),SlotEvent));            
    // Deselect the card
    if ((SlotEvent == SlotResetRequest) || (SlotEvent == SlotDeselectRequest)) {
        DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("CSDSlot: Deselect the card\n")));
        // Update RCA for all devices, start from parent device
        for (DWORD dwIndex=0; dwIndex <= SD_MAXIMUM_DEVICE_PER_SLOT && SD_API_SUCCESS(status); dwIndex++) {
            CSDDevice * pCurrentDevice = GetFunctionDevice(dwIndex);
            if (pCurrentDevice != NULL) {
                status = pCurrentDevice->HandleDeviceSelectDeselect(SlotEvent, FALSE);  
                if (SD_API_SUCCESS(status)) {
                    pCurrentDevice->NotifyClient(SDCardDeselected);
                }
                pCurrentDevice->DeRef();
            }
        }
        if (SD_API_SUCCESS(status)) {
            m_SlotState = SlotDeselected; // Mark the slot as SlotDeselected to prevent further requests
            m_AllocatedPower = 0;
        }
        else {
            DbgPrintZo(SDCARD_ZONE_ERROR, (TEXT("CSDSlot: Failed to deselect card status =%x\n"),status));  
        }
    }


    // Select the card
    if (SD_API_SUCCESS(status) &&
            ((SlotEvent == SlotResetRequest) || (SlotEvent == SlotSelectRequest))) {
        DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: Select the card\n")));  

        // Save the current slot interface
        CurrentSlotInterface.InterfaceModeEx = m_SlotInterfaceEx.InterfaceModeEx;
        CurrentSlotInterface.ClockRate = m_SlotInterfaceEx.ClockRate;

        // set the slot for 1 bit mode
        m_SlotInterfaceEx.InterfaceModeEx.bit.sd4Bit = 0 ;
        m_SlotInterfaceEx.ClockRate = SD_DEFAULT_CARD_ID_CLOCK_RATE;

        // set the card interface for the slot
        status = SDSetCardInterfaceForSlot(&m_SlotInterfaceEx);
        // Mark slot status as SlotReseting, only this thread can send bus request.
        m_SlotState = SlotResetting;

        for (DWORD dwIndex=0; dwIndex <= SD_MAXIMUM_DEVICE_PER_SLOT && SD_API_SUCCESS(status); dwIndex++) {
            CSDDevice * pCurrentDevice = GetFunctionDevice(dwIndex);
            if (pCurrentDevice != NULL) {
                status = pCurrentDevice->HandleDeviceSelectDeselect(SlotEvent, TRUE);  
                pCurrentDevice->DeRef();
            }
        }
        if (SD_API_SUCCESS(status)) {
            // Set Slot state as ready
            m_SlotState = Ready;
            m_SlotInterfaceEx.InterfaceModeEx = CurrentSlotInterface.InterfaceModeEx;
            m_SlotInterfaceEx.ClockRate = CurrentSlotInterface.ClockRate;
            for (DWORD dwIndex=0; dwIndex <SD_MAXIMUM_DEVICE_PER_SLOT  && SD_API_SUCCESS(status); dwIndex++) {
                CSDDevice * pDevice = GetFunctionDevice(dwIndex);
                if (pDevice) {
                    status  =  pDevice->SetCardInterface(&CurrentSlotInterface);                    
                    pDevice->DeRef();
                }
                
            }
            status = SDSetCardInterfaceForSlot(&m_SlotInterfaceEx);
                
            for (DWORD dwIndex = 0 ; dwIndex <SD_MAXIMUM_DEVICE_PER_SLOT && SD_API_SUCCESS(status) ; dwIndex++) {
                CSDDevice * pDevice = GetFunctionDevice(dwIndex);
                if (pDevice) {
                    pDevice->NotifyClient(SDCardSelected);
                    pDevice->DeRef();
                }
                
            }
        }
    }
    ASSERT(SD_API_SUCCESS(status));
    DbgPrintZo(SDBUS_ZONE_DEVICE, (TEXT("SDBusDriver: HandleSlotSelectDeselect--status =%X\n"),status));  
    return (SD_API_SUCCESS(status));
}
