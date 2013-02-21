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
#ifndef _S3C6410_HSMMC_H_
#define _S3C6410_HSMMC_H_

#include "../s3c6410_hsmmc_lib/SDHC.h"
#include <oal_intr.h>

// for card detect pin of HSMMC ch0 on SMDK6410.
#define SD_CD1_IRQ    IRQ_EINT10
typedef class CSDHControllerCh1 : public CSDHCBase {
    public:
        // Constructor
#ifndef _SMDK6410_CH1_EXTCD_
        CSDHControllerCh1() : CSDHCBase() {}
#else
        CSDHControllerCh1();
#endif

        // Destructor
        virtual ~CSDHControllerCh1() {}

        // Perform basic initialization including initializing the hardware
        // so that the capabilities register can be read.
        virtual BOOL Init(LPCTSTR pszActiveKey);

        virtual VOID PowerUp();

        virtual LPSDHC_DESTRUCTION_PROC GetDestructionProc() {
            return &DestroyHSMMCHCCh1Object;
        }

        static VOID DestroyHSMMCHCCh1Object(PCSDHCBase pSDHC);

#ifdef _SMDK6410_CH1_EXTCD_
        // Below functions and variables are newly implemented for card detect of HSMMC ch1 on SMDK6410.
        virtual SD_API_STATUS Start();
#endif
    protected:
        BOOL InitClkPwr();
        BOOL InitGPIO();
        BOOL InitHSMMC();

        BOOL InitCh();
#ifdef _SMDK6410_CH1_EXTCD_
        static DWORD SD_CardDetectThread(CSDHControllerCh1 *pController) {
            return pController->CardDetectThread();
        }

        virtual DWORD CardDetectThread();

        virtual BOOL InitializeHardware();

        BOOL EnableCardDetectInterrupt();

        HANDLE        m_hevCardDetectEvent;
        HANDLE        m_htCardDetectThread;
        DWORD            m_dwSDDetectSysIntr;
        DWORD            m_dwSDDetectIrq;
#endif
} *PCSDHControllerCh1;

#endif // _S3C6410_HSMMC_H_

