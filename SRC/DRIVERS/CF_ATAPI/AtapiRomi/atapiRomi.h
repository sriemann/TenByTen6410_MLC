//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

/*++

Module Name:
    atapipci.h

Abstract:
    ATA/ATAPI for 2440 Ref Board.

Revision History:

--*/

#ifndef _ATAPIROMI_H_
#define _ATAPIROMI_H_

#include <atamain.h>
#include <diskmain.h>

#include <bsp_cfg.h>
#include "s3c6410_gpio.h"
#include "s3c6410_base_regs.h"
#include "s3c6410_syscon.h"

// Config port undo flags
#define CP_CLNUP_IRQEVENT    1
#define CP_CLNUP_HEVENT      2
#define CP_CLNUP_INTCHNHDNLR 4
#define CP_CLNUP_IST         8
#define ISTFLG_EXIT          1





class CRomiDisk : public CDisk {
  public:

    // member variables
    static LONG  m_lDeviceCount;
    HANDLE m_hIsr; // handle to ISR



    // (DMA support)
    LPBYTE       m_pStartMemory;
    PDMATable    m_pPRD;
    PPhysTable   m_pPhysList;
    PSGCopyTable m_pSGCopy;
    PDWORD       m_pPFNs;
    DWORD        m_dwSGCount;
    DWORD        m_pPRDPhys;
    DWORD        m_dwPhysCount;
    DWORD        m_dwPRDSize;
    LPBYTE       m_pBMCommand;
    DWORD        m_dwIndirectMode;

    volatile S3C6410_SYSCON_REG *m_vpSYSCONRegs;
    volatile S3C6410_GPIO_REG * m_vpIOPORTRegs;
    DWORD        m_dwOPMode;

//    volatile S3C2443_CFCARD_REG *m_vpATAPIRegs;

    // constructors/destructors
    CRomiDisk(HKEY hKey);
    virtual ~CRomiDisk();

    // member functions
    virtual VOID ConfigureRegisterBlock(DWORD dwStride);
    virtual BOOL Init(HKEY hActiveKey);
    virtual DWORD MainIoctl(PIOREQ pIOReq);
    // virtual void TakeCS();
    // virtual void ReleaseCS();
    virtual BOOL WaitForInterrupt(DWORD dwTimeOut);
    virtual void EnableInterrupt();
    virtual BOOL InitializePort();
    virtual void ConfigPort();
    virtual BOOL WakeUp();
    
    void FreeDMABuffers();
    void CopyDiskInfoFromPort();
    BOOL TranslateAddress(PDWORD pdwAddr);
    virtual CDiskPower *GetDiskPowerInterface(void);


    inline virtual void CRomiDisk::TakeCS() {
        m_pPort->TakeCS();
    }
    inline virtual void CRomiDisk::ReleaseCS() {
        m_pPort->ReleaseCS();
    }
    inline void CRomiDisk::WriteBMCommand(BYTE bCommand) {
        ATA_WRITE_BYTE(m_pBMCommand, bCommand);
    }
    inline BYTE CRomiDisk::ReadBMStatus() {
        return ATA_READ_BYTE(m_pBMCommand + 2);
    }
    inline void CRomiDisk::WriteBMTable(DWORD dwPhys) {
        ATA_WRITE_DWORD((LPDWORD)(m_pBMCommand + 4), dwPhys);
    }
    inline void CRomiDisk::WriteBMStatus(BYTE bStatus) {
        ATA_WRITE_BYTE(m_pBMCommand + 2, bStatus);
    }
    inline BOOL CRomiDisk::DoesDeviceAlreadyExist() {
        return FALSE;
    }

    virtual void SetPioMode(UCHAR pmode);
    virtual void SetUdmaMode(void) ;    

};

#endif //_ATAPIPCI_H_

