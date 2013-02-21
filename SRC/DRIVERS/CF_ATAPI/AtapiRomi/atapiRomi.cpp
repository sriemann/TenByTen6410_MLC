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
    atapiRomi.cpp
        based on atapipci.cpp

Abstract:
    ATA/ATAPI for 6410 Ref Board.

Revision History:

--*/

#include <windows.h>
#include <types.h>
#include <ceddk.h>
#include <bsp_cfg.h>
#include "atapiRomi.h"

#define DirectMode 1
#define IndirectMode 0

LONG CRomiDisk::m_lDeviceCount = 0;

static TCHAR *g_szPCICDRomDisk = TEXT("CD-ROM");
static TCHAR *g_szPCIHardDisk = TEXT("Hard Disk");

// ----------------------------------------------------------------------------
// Function: CreateRomi
//     Spawn function called by IDE/ATA controller enumerator
//
// Parameters:
//     hDevKey -
// ----------------------------------------------------------------------------


EXTERN_C
CDisk *
CreateRomi(
    HKEY hDevKey
    )
{
    return new CRomiDisk(hDevKey);
}

// ----------------------------------------------------------------------------
// Function: CRomiDisk
//     Constructor
//
// Parameters:
//     hKey -
// ----------------------------------------------------------------------------

CRomiDisk::CRomiDisk(
    HKEY hKey
    ) : CDisk(hKey)
{
    m_pStartMemory = NULL;
    m_pPort = NULL;
    m_pPhysList = NULL;
    m_pSGCopy = NULL;
    m_pPFNs = NULL;
    m_pPRDPhys = 0;
    m_pPRD = NULL;
    m_dwPhysCount = 0;
    m_dwSGCount = 0;
    m_dwPRDSize = 0;
    m_pBMCommand = NULL;

    InterlockedIncrement(&m_lDeviceCount);

    DEBUGMSG(ZONE_INIT|ZONE_PCI, (_T(
        "Atapi!CRomiDisk::CRomiDisk> device count(%d)\r\n"
        ), m_lDeviceCount));
}

// ----------------------------------------------------------------------------
// Function: ~CRomiDisk
//     Destructor
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

CRomiDisk::~CRomiDisk(
    )
{
    FreeDMABuffers();

    if ( m_vpSYSCONRegs)
    {
        MmUnmapIoSpace((PVOID)m_vpSYSCONRegs, sizeof(S3C6410_SYSCON_REG));
        m_vpSYSCONRegs= NULL;
    }

    if ( m_vpIOPORTRegs)
    {
        MmUnmapIoSpace((PVOID)m_vpIOPORTRegs, sizeof(S3C6410_GPIO_REG));
        m_vpIOPORTRegs= NULL;
    }

    if ( m_pDMAVirtualAddress )
    {
        PHYSICAL_ADDRESS PhysicalAddress;

        PhysicalAddress.LowPart = m_DMAPhyaddress.LowPart;
        HalFreeCommonBuffer(0, 0, PhysicalAddress, (PVOID)m_pDMAVirtualAddress, FALSE);
        m_pDMAVirtualAddress = NULL;
    }

    InterlockedDecrement(&m_lDeviceCount);

    DEBUGMSG(ZONE_INIT|ZONE_PCI, (_T(
        "Atapi!CRomiDisk::~CRomiDisk> device count(%d)\r\n"
        ), m_lDeviceCount));
}

// ----------------------------------------------------------------------------
// Function: FreeDMABuffers
//     Deallocate DMA buffers
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

void
CRomiDisk::FreeDMABuffers(
    )
{
    if (m_pPRD) {
        FreePhysMem(m_pPRD);
        m_pPRDPhys = NULL;
        m_pPRD = NULL;
    }

    if (m_pPhysList) {
        // free the fixed pages; the variable pages should already be free
        for (DWORD i = 0; i < MIN_PHYS_PAGES; i++) {
            FreePhysMem(m_pPhysList[i].pVirtualAddress);
        }
        VirtualFree(m_pPhysList, UserKInfo[KINX_PAGESIZE], MEM_DECOMMIT);
        m_pPhysList = NULL;
    }

    if (m_pSGCopy) {
        VirtualFree(m_pSGCopy, UserKInfo[KINX_PAGESIZE], MEM_DECOMMIT);
        m_pSGCopy = NULL;
    }

    if (m_pPFNs) {
        VirtualFree(m_pPFNs, UserKInfo[KINX_PAGESIZE], MEM_DECOMMIT);
        m_pSGCopy = NULL;
    }

    if (m_pStartMemory) {
        VirtualFree(m_pStartMemory, 0, MEM_RELEASE);
        m_pStartMemory = NULL;
    }
    
    m_dwPhysCount = 0;
    m_dwSGCount = 0;
}

// ----------------------------------------------------------------------------
// Function: CopyDiskInfoFromPort
//     This function is not used
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

void
CRomiDisk::CopyDiskInfoFromPort(
    )
{
    ASSERT(m_pPort->m_dwRegBase != 0);
    m_pATAReg = (PBYTE)m_pPort->m_dwRegBase;
    m_pATARegAlt = (PBYTE)m_pPort->m_dwRegAlt;

    ASSERT(m_pPort->m_dwBMR != 0);
    m_pBMCommand = (LPBYTE)m_pPort->m_dwBMR;
}

// ----------------------------------------------------------------------------
// Function: WaitForInterrupt
//     Wait for interrupt
//
// Parameters:
//     dwTimeOut -
// ----------------------------------------------------------------------------

BOOL
CRomiDisk::WaitForInterrupt(
    DWORD dwTimeOut
    )
{
    BYTE bStatus;
    BOOL fRet = TRUE;
    DWORD dwRet;

    // wait for interrupt
    dwRet = WaitForSingleObject(m_pPort->m_hIRQEvent, dwTimeOut);
    if (dwRet == WAIT_TIMEOUT) {
        fRet = FALSE;
    }
    else {
        if (dwRet != WAIT_OBJECT_0) {
            if (!WaitForDisc(WAIT_TYPE_DRQ, dwTimeOut, 10)) {
                fRet = FALSE;
            }
        }
    }

    // read status; acknowledge interrupt
    bStatus = GetBaseStatus();
    if (bStatus & ATA_STATUS_ERROR) {
        bStatus = GetError();
        fRet = FALSE;
    }

    // signal interrupt done
    InterruptDone(m_pPort->m_dwSysIntr);

    return fRet;
}

// ----------------------------------------------------------------------------
// Function: EnableInterrupt
//     Enable channel interrupt
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

void
CRomiDisk::EnableInterrupt(
    )
{
    GetBaseStatus(); // acknowledge interrupt, if pending

    // signal interrupt done
    InterruptDone(m_pPort->m_dwSysIntr);
}

// ----------------------------------------------------------------------------
// Function: ConfigureRegisterBlock
//     This function is called by DSK_Init before any other CDisk function to
//     set up the register block.
//
// Parameters:
//     dwStride -
// ----------------------------------------------------------------------------

VOID
CRomiDisk::ConfigureRegisterBlock(
    DWORD dwStride
    )
{

    m_dwStride = dwStride;
    m_dwDataDrvCtrlOffset = ATA_PIO_DTR;//ATA_REG_DATA * dwStride;
    m_dwFeatureErrorOffset = ATA_PIO_FED;//ATA_REG_FEATURE * dwStride;
    m_dwSectCntReasonOffset = ATA_PIO_SCR;//ATA_REG_SECT_CNT * dwStride;
    m_dwSectNumOffset = ATA_PIO_LLR;//ATA_REG_SECT_NUM * dwStride;
    m_dwByteCountLowOffset = ATA_PIO_LMR;//ATA_REG_BYTECOUNTLOW * dwStride;
    m_dwByteCountHighOffset = ATA_PIO_LHR;//ATA_REG_BYTECOUNTHIGH * dwStride;
    m_dwDrvHeadOffset = ATA_PIO_DVR;//ATA_REG_DRV_HEAD * dwStride;
    m_dwCommandStatusOffset =ATA_PIO_CSD;// ATA_REG_COMMAND * dwStride;

    // PCI ATA implementations don't assign I/O resources for the first four
    // bytes, as they are unused

    m_dwAltStatusOffset = ATA_PIO_DAD;//8 * dwStride;//ATA_REG_ALT_STATUS * dwStride;
    m_dwAltDrvCtrl = ATA_PIO_DAD;//8 * dwStride;//ATA_REG_DRV_CTRL * dwStride;
}

// ----------------------------------------------------------------------------
// Function: Init
//     Initialize channel
//
// Parameters:
//     hActiveKey -
// ----------------------------------------------------------------------------

BOOL
CRomiDisk::Init(
    HKEY hActiveKey
    )
{
    BOOL bRet = FALSE;

    m_f16Bit = TRUE; // PCI is 16-bit

    // configure port
    if (!InitializePort()) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!CRomiDisk::Init> Failed to configure port; device(%u)\r\n"
            ), m_dwDeviceId));

        goto exit;
    }

    // assign the appropriate folder name
    m_szDiskName = (IsCDRomDevice() ? g_szPCICDRomDisk : g_szPCIHardDisk);

    // reserve memory for DMA buffers
    m_pStartMemory = (LPBYTE)VirtualAlloc(NULL, 0x10000, MEM_RESERVE, PAGE_READWRITE);
    if (!m_pStartMemory) {
        bRet = FALSE;
    }

    WriteReg(ATA_CONTROL, ReadReg(ATA_CONTROL) | 0x1);
    // finish intialization; i.e., initialize device
    bRet = CDisk::Init(hActiveKey);

    if ((m_pPort->m_pDskReg[m_dwDeviceId]->dwEnablePDMA || m_pPort->m_pDskReg[m_dwDeviceId]->dwEnableUDMA) && m_pDMAVirtualAddress == NULL)
    {
        DMA_ADAPTER_OBJECT dmaAdapter;
        dmaAdapter.ObjectSize = sizeof(dmaAdapter);
        dmaAdapter.InterfaceType = Internal;
        dmaAdapter.BusNumber = 0;
        m_pDMAVirtualAddress= (PBYTE)HalAllocateCommonBuffer( &dmaAdapter, m_pPort->m_pDskReg[m_dwDeviceId]->dwDoubleBufferSize, &m_DMAPhyaddress, FALSE );
    }

    if (m_pPort->m_pDskReg[m_dwDeviceId]->dwEnableUDMA && ( m_Id.UltraDMASupport & 0x3f))
    {    
        for(int i=5;i>=0;i--) {
            if(m_Id.UltraDMASupport & (0x01<<i)) {
                m_dwCurrentUDMAMode = (i > 4) ? 4 : i;
                break; 
            }
        }     
        SetPioMode(PIO0);        
        SetUdmaMode();
        DEBUGMSG(ZONE_INIT, (_T("### ATA-Disk supports UDMA to 0x%x 0x%x\r\n"), m_Id.UltraDMASupport,m_dwCurrentUDMAMode));  
        m_pPort->m_pDskReg[m_dwDeviceId]->dwEnablePDMA = FALSE;

    }
    else if (m_pPort->m_pDskReg[m_dwDeviceId]->dwEnablePDMA)
    {
        SetPioMode(m_Id.AdvancedPIOxferreserved);
        m_pPort->m_pDskReg[m_dwDeviceId]->dwEnableUDMA = FALSE;;    
        DEBUGMSG(ZONE_INIT, (_T("### ATA-Disk dose not support UDMA. It is running on PDMA\r\n")));  
    }
    else {
        SetPioMode(m_Id.AdvancedPIOxferreserved);
        m_pPort->m_pDskReg[m_dwDeviceId]->dwEnableUDMA = FALSE;;        
        m_pPort->m_pDskReg[m_dwDeviceId]->dwEnablePDMA = FALSE;;        
        DEBUGMSG(ZONE_INIT, (_T("### ATA-Disk is running on PIO MODE\r\n")));          
        DEBUGMSG(ZONE_INIT, (_T("Atapi!CDisk::Init> Disabled DMA\r\n")));
    }

    // associate interrupt event with IRQ
    if (!InterruptInitialize(
        m_pPort->m_dwSysIntr,
        m_pPort->m_hIRQEvent,
        NULL,
        0)
    ) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
            "Atapi!CRomiDisk::ConfigPort> Failed to initialize interrupt(SysIntr(%d)) for device(%d)\r\n"
            ), m_pPort->m_dwSysIntr, m_dwDeviceId));
        bRet = FALSE;
    }

    if (!bRet) {
        goto exit;
    }

exit:;
    return bRet;
}

// ----------------------------------------------------------------------------
// Function: MainIoctl
//     This is redundant
//
// Parameters:
//     pIOReq -
// ----------------------------------------------------------------------------

DWORD
CRomiDisk::MainIoctl(
    PIOREQ pIOReq
    )
{
    DEBUGMSG(ZONE_IOCTL, (_T(
        "Atapi!CRomiDisk::MainIoctl> IOCTL(%d), device(%d) \r\n"
        ), pIOReq->dwCode, m_dwDeviceId));

    return CDisk::MainIoctl(pIOReq);
}

// ----------------------------------------------------------------------------
// Function: InitializePort
//     Initialize IST/ISR
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

BOOL 
CRomiDisk::InitializePort(
    )
{
    BOOL RetValue=TRUE;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
    
    m_pATAReg = (PBYTE)m_pPort->m_dwRegBase;

    m_pATARegAlt = (PBYTE)m_pPort->m_dwRegAlt;
    m_pBMCommand = (PBYTE)m_pPort->m_dwBMR;

    m_dwIndirectMode = m_pPort->m_pDskReg[m_dwDeviceId]->dwIndirectMode;

    // Map it if it is Memeory Mapped IO.
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;    
    m_vpIOPORTRegs = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase , sizeof(S3C6410_GPIO_REG),FALSE);
    if (m_vpIOPORTRegs == NULL)
    {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,(TEXT("For m_vpIOPORTRegs: MmMapIoSpace() failed IOPORT!\r\n")));
        RetValue = FALSE;
        goto init_done;
    }
    
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;    
    m_vpSYSCONRegs = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (m_vpSYSCONRegs == NULL)
    {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,(TEXT("For m_vpSYSCONRegs: MmMapIoSpace() failed SYSCON!\r\n")));
        RetValue = FALSE;
        goto init_done;        
    }

    ConfigPort();

    if (m_pPort->m_pDskReg[m_dwDeviceId]->dwInterruptDriven || m_pPort->m_pDskReg[m_dwDeviceId]->dwEnableUDMA)
    {
        if (m_pPort->m_hIRQEvent) {
            m_dwDeviceFlags |= DFLAGS_DEVICE_INITIALIZED;
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T("atapiRomi already initialized\n")));
            return TRUE;
        }
        // create interrupt event
        if (NULL == (m_pPort->m_hIRQEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR, (_T(
                "Atapi!CRomiDisk::ConfigPort> Failed to create interrupt event for device(%d)\r\n"
                ), m_dwDeviceId));
            return FALSE;
        }
    }
    
init_done:
    if ( RetValue != TRUE )
    {
        if ( m_vpSYSCONRegs)
        {
            MmUnmapIoSpace((PVOID)m_vpSYSCONRegs, sizeof(S3C6410_SYSCON_REG));
            m_vpSYSCONRegs= NULL;
        }
    
        if ( m_vpIOPORTRegs)
        {
            MmUnmapIoSpace((PVOID)m_vpIOPORTRegs, sizeof(S3C6410_GPIO_REG));
            m_vpIOPORTRegs= NULL;
        }        
    }

    return RetValue;

    //WriteDriveHeadReg(0x40);
}

// ----------------------------------------------------------------------------
// Function: ConfigPort
//     Initialize DATA/ADDR/Control port
//
// Parameters:
//     None
// ----------------------------------------------------------------------------

void
CRomiDisk::ConfigPort(
    )
{
    m_vpIOPORTRegs->GPPPUD |= (m_vpIOPORTRegs->GPPPUD & (~(0x3) << 28)) | (0x2 << 28);

    // Chip Selection signal in S3C6410 is controlled using "MEM_SYS_CFG" register in SYSCON.
    // MEM_SYS_CFG[5:4] = 2'b11 -> CFCON CS0, CFCON CS1
    m_vpSYSCONRegs->MEM_SYS_CFG |= ((m_vpSYSCONRegs->MEM_SYS_CFG & (~(0x3F<<0)))|(0x1<<5)|(0x1<<4));
    m_vpIOPORTRegs->GPBCON &= ~(0xF<<16);
    m_vpIOPORTRegs->GPBCON |= (4<<16);        // CF Data DIR

    if ( m_dwIndirectMode == 1 )
    {
        m_vpSYSCONRegs->MEM_SYS_CFG &= ~(0x1<<14); // InDirectMode
        m_dwOPMode = IndirectMode;
        DEBUGMSG(ZONE_INIT,(TEXT("[CF-ATA] Mode : InDirect Mode\n")));
    }
    else
    {
        m_vpSYSCONRegs->MEM_SYS_CFG |= (0x1<<14); // Use independent CF interface (DirectMode)
        m_dwOPMode = DirectMode;
        DEBUGMSG(ZONE_INIT,(TEXT("[CF-ATA] Mode : Direct Mode\n")));
    }

    // Set the GPIO for CF_Data, CF_Addr, IORDY, IOWR, IORD, CE[0], and CE[1]
    // Details.
    // 1. GPKCON0[31:0]  - DATA_CF[7:0] -> 4'b0101
    // 2. GPKCON1[31:0]  - DATA_CF[15:8] -> 4'b0101
    // 3. GPLCON0[11:0]  - ADDR_CF[2:0] -> 4'b0110
    // 4. GPMCON[19:16] - IORDY_CF -> 4'b0110
    // 5. GPMCON[15:12] - IOWR_CF -> 4'b0110
    // 6. GPMCON[11:8]   - IORD_CF -> 4'b0110
    // 7. GPMCON[7:4]      - CE_CF[1] -> 4'b0110
    // 8. GPMCON[3:0]     - CE_CF[0] -> 4'b0110


    if(m_dwOPMode == DirectMode)
    {
        m_vpIOPORTRegs->GPKCON0 = (m_vpIOPORTRegs->GPKCON0 & ~(0xFFFFFFFF)) | (0x55555555); // D: Set XhiDATA[7:0] pins as CF Data[7:0] 

        m_vpIOPORTRegs->GPKCON1 = (m_vpIOPORTRegs->GPKCON1 & ~(0xFFFFFFFF)) | (0x55555555); // D: Set XhiDATA[15:8] pins as CF Data[15:8]

        m_vpIOPORTRegs->GPLCON0 = (m_vpIOPORTRegs->GPLCON0 & ~(0xFFF)) | (0x666);           // A: Set XhiADDR[2:0] pins as CF ADDR[2:0]

        m_vpIOPORTRegs->GPMCON  = (m_vpIOPORTRegs->GPMCON & ~(0xFFFFF)) | (0x66666);        // IORDY_CF, IOWR_CF, IORD_CF, CE_CF[1:0]
    }

    m_vpIOPORTRegs->GPMCON = (m_vpIOPORTRegs->GPMCON &  ~(0xF<<20)) | 0x1<<20;              // set XhINTR/CF Data Dir./GPM5 as output
    m_vpIOPORTRegs->GPMDAT = (m_vpIOPORTRegs->GPMDAT | 0x1 << 5);                           // GPM[5] -> High

    WriteReg(MUX_REG, 0x07);
    WriteReg(MUX_REG, 0x03);
    WriteReg(MUX_REG, 0x01);
    Sleep(100);


    WriteReg(ATA_PIO_TIME, 0x1c238);
    WriteReg(ATA_UDMA_TIME, 0x20B1362);

    WriteReg(ATA_IRQ, ReadReg(ATA_IRQ) | 0x1f);
    WriteReg(ATA_IRQ_MASK, ReadReg(ATA_IRQ_MASK) | 0x1f);

    WriteReg(ATA_CFG, ReadReg(ATA_CFG) & ~(0x40));

    WriteReg(ATA_CONTROL, ReadReg(ATA_CONTROL) | 0x1);    // ATA is enabled, When this value is set to 1, delay of 200ms will be required(UserManual).
    Sleep(200);

    // Make the ADDR_CF0 port high for CF card boot up
    m_vpIOPORTRegs->GPNCON = (m_vpIOPORTRegs->GPNCON & ~(0x3<<16)) | (0x1<<16);     // set XEINT8/ADDR_CF0/GPN8 as output
    m_vpIOPORTRegs->GPNDAT |= (0x1<<8);                                             // GPN[8] -> High
    Sleep(100);
    m_vpIOPORTRegs->GPNDAT &= ~(0x1<<8);                                            // GPN[8] -> Low

}

// ----------------------------------------------------------------------------
// Function: TranslateAddress
//     Translate a system address to a bus address for the DMA controller
//
// Parameters:
//     pdwAddr -
// ----------------------------------------------------------------------------

BOOL
CRomiDisk::TranslateAddress(
    PDWORD pdwAddr
    )
{
    // translate a system address to a bus address for the DMA bus controller

    PHYSICAL_ADDRESS SystemLogicalAddress, TransLogicalAddress;
    DWORD dwBus;

    dwBus = m_pPort->m_pController->m_dwi.dwBusNumber;

    // translate address
    SystemLogicalAddress.HighPart = 0;
    SystemLogicalAddress.LowPart = *pdwAddr;
    if (!HalTranslateSystemAddress(PCIBus, dwBus, SystemLogicalAddress, &TransLogicalAddress)) {
        return FALSE;
    }

    *pdwAddr = TransLogicalAddress.LowPart;

    return TRUE;
}

 BOOL
CRomiDisk::WakeUp(
    )
{
    ConfigPort();
    return CDisk::Init(NULL);
}

CDiskPower *
CRomiDisk::GetDiskPowerInterface(
    void
    )
{
    CDiskPower *pDiskPower = new CDiskPower;
    return pDiskPower;
}

void
CRomiDisk::SetPioMode(UCHAR  pmode)
{
    UINT8 nMode;
    UINT32 uT1;
    UINT32 uT2;
    UINT32 uTeoc;
    UINT32 i;

    UINT32 uPioTime[5];
    UINT32 m_uPioT1[5] = {70,50,30,30,25};
    UINT32 m_uPioT2[5] = {290,290,290,80,70};
    UINT32 m_uPioTeoc[5] = {20,15,10,10,10};

    UINT32 uCycleTime = (UINT32)(1000000000/S3C6410_HCLK);

    UINT32 uTemp = ReadReg(ATA_CFG);

    if ((pmode & (UCHAR)0x3) == 1)
        nMode = PIO3;
    else if ((pmode & (UCHAR)0x3) == 3)
        nMode = PIO4;
    else
        nMode = PIO2 ;


    for (i=0; i<5; i++)
    {
        uT1   = (m_uPioT1[i]  /uCycleTime + 1)&0xff;
        uT2   = (m_uPioT2[i]  /uCycleTime + 1)&0xff;
        uTeoc = (m_uPioTeoc[i]/uCycleTime + 1)&0x0f;
        uPioTime[i] = (uTeoc<<12)|(uT2<<4)|uT1;
    }

    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_DAD, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_FED, 0x3);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_SCR, (0x8 |(nMode&0x7)));
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_LLR, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_LMR, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_LHR, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_DVR, 0x40);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_CSD, ATAPI_CMD_SET_FEATURES);
    WaitForNoBusyStatus();
    switch(pmode) {
        case PIO1:
            uTemp &= (~0x2); //IORDY disable
            WriteReg(ATA_PIO_TIME, uPioTime[1]);
            break;
        case PIO2:
            uTemp &= (~0x2); //IORDY disable
            WriteReg(ATA_PIO_TIME, uPioTime[2]);
            break;
        case PIO3:
            uTemp |= 0x2; //IORDY enable
            WriteReg(ATA_PIO_TIME, uPioTime[3]);
            break;
        case PIO4:
            uTemp |= 0x2; //IORDY enable
            WriteReg(ATA_PIO_TIME, uPioTime[4]);
            break;
        default:
            uTemp &= (~0x2); //IORDY disable
            WriteReg(ATA_PIO_TIME, uPioTime[0]);
            break;
    }

    WriteReg(ATA_CFG, uTemp);
}


void
CRomiDisk::SetUdmaMode()
{

    UINT32 uTdvh1;
    UINT32 uTdvs;
    UINT32 uTrp;
    UINT32 uTss;
    UINT32 uTackenv;
    UINT32 i;

    UINT32 uUdmaTime[5] = {0};
    UINT32 uUdmaTdvh[5] = {20,20,10,10,10};
    UINT32 uUdmaTdvs[5] = {100,60,50,35,20};
    UINT32 uUdmaTrp[5] = {160,125,100,100,100};
    UINT32 uUdmaTss[5] = {50,50,50,50,50};
    UINT32 uUdmaTackenvMin[5] = {20,20,20,20,20};
    UINT32 uUdmaTackenvMax[5] = {70,70,70,55,55};
    UINT32 uCycleTime = (UINT32)(1000000000/S3C6410_HCLK);


    for (i=0; i<5; i++) 
    {
        uTdvh1  = (uUdmaTdvh[i] / uCycleTime /*+ 1*/)&0x0f;
        uTdvs   = (uUdmaTdvs[i] / uCycleTime /*+ 1*/)&0xff;
        uTrp    = (uUdmaTrp[i]  / uCycleTime /*+ 1*/)&0xff;
        uTss    = (uUdmaTss[i]  / uCycleTime /*+ 1*/)&0x0f;
        uTackenv= (uUdmaTackenvMin[i]/uCycleTime /*+ 1*/)&0x0f;
        uUdmaTime[i] = (uTdvh1<<24)|(uTdvs<<16)|(uTrp<<8)|(uTss<<4)|uTackenv;
    }

    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_DAD, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_FED, 0x3);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_SCR, (0x40 |((BYTE)m_dwCurrentUDMAMode&0x7)));
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_LLR, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_LMR, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_LHR, 0x0);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_DVR, 0x40);
    ATA_WRITE_BYTE(m_pATAReg + ATA_PIO_CSD, ATAPI_CMD_SET_FEATURES);

    WaitForNoBusyStatus();


    switch(m_dwCurrentUDMAMode)
    {
        case UDMA0:
            //*(UINT32 *)(m_pATAReg + ATA_UDMA_TIME) = uUdmaTime[0];
            WriteReg(ATA_UDMA_TIME, uUdmaTime[0]);
            break;
        case UDMA1:
            WriteReg(ATA_UDMA_TIME, uUdmaTime[1]);
            break;
        case UDMA2:
            WriteReg(ATA_UDMA_TIME, uUdmaTime[2]);
            break;
        case UDMA3:
            WriteReg(ATA_UDMA_TIME, uUdmaTime[3]);
            break;
        case UDMA4:
            WriteReg(ATA_UDMA_TIME, uUdmaTime[4]);
            break;
        default:
            DEBUGMSG(ZONE_INIT|ZONE_ERROR,(TEXT("UDMA mode is supported between 0 to 4 !!!! . %d\r\n"),m_dwCurrentUDMAMode));
            break;
    }
}


