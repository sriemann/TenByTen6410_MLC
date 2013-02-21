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
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#include "s3c6410otgdevice.h"

#define USB_DD_VERSION 100

// Caution: Turning on more debug zones can cause STALLs due
// to corrupted setup packets.
UFN_GENERATE_DPCURSETTINGS(__MODULE__,
    _T("Power"), _T("Trace"), _T(""), _T(""),
    USBFNCTL_ZONES);


#define UDC_REG_PRIORITY_VAL _T("Priority256")

#define LOCK_ENDPOINT(peps)        EnterCriticalSection(&peps->cs)
#define UNLOCK_ENDPOINT(peps)      LeaveCriticalSection(&peps->cs)

// Return the irq bit for this endpoint.
#define EpToIrqStatBit(dwEndpoint)    (0x1<<(BYTE)dwEndpoint)


#define EP_0_PACKET_SIZE     0x40    // USB2.0

#define ENDPOINT_COUNT      16
#define EP_VALID(x)         ((x) < ENDPOINT_COUNT)

#define DEFAULT_PRIORITY     120

#define SET        TRUE
#define CLEAR     FALSE


// This function is for checking the current usb client driver.
extern BOOL USBCurrentDriver(PUFN_CLIENT_INFO pDriverList);

#define PIPELINED_FIFO_DEPTH 2
#define GET_NEXT_PF_IDX(i) (((i+1) >= PIPELINED_FIFO_DEPTH) ? 0 : (i+1))
#define GET_PREV_PF_IDX(i) (((i-1) < 0) ? (PIPELINED_FIFO_DEPTH-1) : (i-1))

typedef struct CTRL_PDD_CONTEXT {
    volatile S3C6410_SYSCON_REG *       pSYSCONregs;
    PVOID                               pvMddContext;
    DWORD                               dwSig;
    HANDLE                              hIST;
    HANDLE                              hevInterrupt;
    BOOL                                fRunning;
    CRITICAL_SECTION                    csRegisterAccess;
    BOOL                                fSpeedReported;
    BOOL                                fRestartIST;
    BOOL                                fExitIST;
    BOOL                                attachedState;
    BOOL                                sendDataEnd;
    BOOL                                IsFirstReset;
    EP0_STATE                           Ep0State;

    DWORD                               dwEp0MaxPktSize;
    DWORD                               dwDetectedSpeed;

    // registry
    DWORD                               dwSysIntr;
    DWORD                               dwIrq;
    DWORD                               dwISTPriority;

    USB_DEVICE_REQUEST                  UDR;
    EP_STATUS                           rgEpStatus[ENDPOINT_COUNT];

    PFN_UFN_MDD_NOTIFY                  pfnNotify;
    HANDLE                              hBusAccess;
    CEDEVICE_POWER_STATE                cpsCurrent;

    DWORD                               dwUSBClassInfo;

    PDWORD                              pVAddrEP[ENDPOINT_COUNT][PIPELINED_FIFO_DEPTH];
    DWORD                               pPAddrEP[ENDPOINT_COUNT][PIPELINED_FIFO_DEPTH];
    DWORD                               dwPipelinedXfered[ENDPOINT_COUNT][PIPELINED_FIFO_DEPTH];
    DWORD                               dwPipelinedStrIdx;
    DWORD                               dwPipelinedEndIdx;
    DWORD                               dwPipelinedXferSize;
    DWORD                               dwPipelinedPktCnt;
    DWORD                               dwPipelinedEP;
    BOOL                                bRingBufferFull;
    
    BOOL                                bRdySetupPkt;
    DWORD                               dwInEPRunning[ENDPOINT_COUNT];
    DWORD                               dwXmittingEP;
    DWORD                               dwXmitReadyCnt;
    DWORD                               dwUsingEPCnt;
    BOOL                                bOutEPDMAStartFlag;
    BOOL                                bMemoryAllocFlag;
} *PCTRLR_PDD_CONTEXT;

// Global variables 
volatile BYTE *g_pUDCBase;

#define SC6410_SIG '6410' // "S3C6410" signature

#define IS_VALID_SC6410_CONTEXT(ptr) \
    ( (ptr != NULL) && (ptr->dwSig == SC6410_SIG) )


#ifdef DEBUG
// Validate the context.
static
VOID
ValidateContext(
                PCTRLR_PDD_CONTEXT pContext
                )
{
    PREFAST_DEBUGCHK(pContext);
    DEBUGCHK(pContext->dwSig == SC6410_SIG);
    DEBUGCHK(!pContext->hevInterrupt || pContext->hIST);
    DEBUGCHK(VALID_DX(pContext->cpsCurrent));
    DEBUGCHK(pContext->pfnNotify);
}

#else
#define ValidateContext(ptr)
#endif

#define CTRLR_BASE_REG_ADDR(offset) ((volatile ULONG*) ( (g_pUDCBase) + (offset)))


BOOL SetOtgDevicePower (
    PCTRLR_PDD_CONTEXT      pContext,
    CEDEVICE_POWER_STATE    cpsNew);

static 
void 
Delay(
    DWORD count
    )
{
    volatile int i, j = 0;
    volatile static int loop = S3C6410_ACLK/100000;
    
    for(; count > 0; count--)
    {
        for(i=0; i < loop; i++) 
        { 
            j++; 
        }
    }
}


// Read a register.
inline
DWORD
ReadReg(
    DWORD dwOffset
    )
{
    volatile ULONG *pbReg = CTRLR_BASE_REG_ADDR(dwOffset);
    DWORD bValue = (DWORD) *pbReg;
    return bValue;
}


// Write a register.
inline
VOID
WriteReg(
     DWORD dwOffset,
     DWORD bValue
     )
{
    volatile ULONG *pbReg = CTRLR_BASE_REG_ADDR(dwOffset);
    *pbReg = (ULONG) bValue;
}


// Read from Endpoint Specific Register
inline
DWORD
ReadEPSpecificReg(
    DWORD dwEndpoint,
    DWORD regOffset
    )
{
    DWORD bValue = ReadReg(regOffset + (dwEndpoint*0x20));
    return bValue;
}


// Write to Endpoint Specific Register
inline
VOID
WriteEPSpecificReg(
    DWORD dwEndpoint,
    DWORD regOffset,
    DWORD bValue
    )
{
    WriteReg(regOffset + (dwEndpoint*0x20), bValue);
}


inline
DWORD
SetClearReg(
    DWORD dwOffset,
    DWORD dwMask,
    BOOL  bSet
    )
{
    volatile ULONG *pbReg = CTRLR_BASE_REG_ADDR(dwOffset);
    volatile DWORD bValue = (DWORD) *pbReg;

    if (bSet)
    {
        bValue |= dwMask;
    }
    else
    {
        bValue &= ~dwMask;
    }

    *pbReg = bValue;

    return bValue;
}


// Set or Clear for specific bit of register.
inline
DWORD
SetClearEPSpecificReg(
    DWORD dwEndpoint,
    DWORD dwOffset,
    DWORD dwMask,
    BOOL  bSet
    )
{
    DWORD bValue = 0;

    // Now Write the Register associated with this Endpoint for a given offset
    bValue = SetClearReg(dwOffset+ (dwEndpoint*0x20), dwMask, bSet);

    return bValue;
}


/*++
Routine Description:
Return the data register of an endpoint.

Arguments:
dwEndpoint - the target endpoint

Return Value:
The data register of the target endpoint.
--*/
static
volatile ULONG*
_GetDataRegister(
    DWORD        dwEndpoint
    )
{
    volatile ULONG *pulDataReg = NULL;

    //
    // find the data register (non-uniform offset)
    //
    switch (dwEndpoint)
    {
        case  0: pulDataReg = CTRLR_BASE_REG_ADDR(EP0_FIFO);  break;
        case  1: pulDataReg = CTRLR_BASE_REG_ADDR(EP1_FIFO);  break;
        case  2: pulDataReg = CTRLR_BASE_REG_ADDR(EP2_FIFO);  break;
        case  3: pulDataReg = CTRLR_BASE_REG_ADDR(EP3_FIFO);  break;
        case  4: pulDataReg = CTRLR_BASE_REG_ADDR(EP4_FIFO);  break;
        case  5: pulDataReg = CTRLR_BASE_REG_ADDR(EP5_FIFO);  break;
        case  6: pulDataReg = CTRLR_BASE_REG_ADDR(EP6_FIFO);  break;
        case  7: pulDataReg = CTRLR_BASE_REG_ADDR(EP7_FIFO);  break;
        case  8: pulDataReg = CTRLR_BASE_REG_ADDR(EP8_FIFO);  break;
        case  9: pulDataReg = CTRLR_BASE_REG_ADDR(EP9_FIFO);  break;
        case  10: pulDataReg = CTRLR_BASE_REG_ADDR(EP10_FIFO);  break;
        case  11: pulDataReg = CTRLR_BASE_REG_ADDR(EP11_FIFO);  break;
        case  12: pulDataReg = CTRLR_BASE_REG_ADDR(EP12_FIFO);  break;
        case  13: pulDataReg = CTRLR_BASE_REG_ADDR(EP13_FIFO);  break;
        case  14: pulDataReg = CTRLR_BASE_REG_ADDR(EP14_FIFO);  break;
        case  15: pulDataReg = CTRLR_BASE_REG_ADDR(EP15_FIFO);  break;

        default:
            DEBUGCHK(FALSE);
        break;
    }
    return pulDataReg;
} // _GetDataRegister


// Retrieve the endpoint status structure.
inline
static
PEP_STATUS
GetEpStatus(
    PCTRLR_PDD_CONTEXT pContext,
    DWORD dwEndpoint
    )
{
    PEP_STATUS peps = &pContext->rgEpStatus[dwEndpoint];
    return peps;
}


/*++
Routine Description:
Enable the interrupt of an endpoint.

Arguments:
dwEndpoint - the target endpoint

Return Value:
None.
--*/
static
VOID
EnableDisableEndpointInterrupt(
    PCTRLR_PDD_CONTEXT        pContext,
    DWORD                   dwEndpoint,
    DWORD                   dwDirection,
    BOOL                    fEnable
    )
{
    EnterCriticalSection(&pContext->csRegisterAccess);
    
    volatile DWORD bEpIntReg = ReadReg(DAINTMSK);
    DWORD bIrqEnBit = EpToIrqStatBit(dwEndpoint);

    if (dwEndpoint == 0)
    {
        if (dwDirection == USB_OUT_TRANSFER)
        {
            bIrqEnBit = bIrqEnBit << ENDPOINT_COUNT;
        }
        else if (dwDirection == USB_IN_TRANSFER)
        {
            bIrqEnBit = bIrqEnBit;
        }
    }
    else
    {
        if (dwDirection == USB_OUT_TRANSFER)
        {
            bIrqEnBit = bIrqEnBit << ENDPOINT_COUNT;
        }
        else if (dwDirection == USB_IN_TRANSFER)
        {
            bIrqEnBit = bIrqEnBit;
        }
    }
    if (fEnable)
    {
        bEpIntReg |= bIrqEnBit;
    }
    else
    {
        bEpIntReg &= ~bIrqEnBit;
    }
    WriteReg(DAINTMSK, bEpIntReg);
    LeaveCriticalSection(&pContext->csRegisterAccess);
}


static
inline
VOID
EnableEndpointInterrupt(
    PCTRLR_PDD_CONTEXT        pContext,
    DWORD                   dwEndpoint,
    DWORD                   dwDirection
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();
    EnableDisableEndpointInterrupt(pContext, dwEndpoint, dwDirection, TRUE);
    FUNCTION_LEAVE_MSG();
}


static
inline
VOID
DisableEndpointInterrupt(
    PCTRLR_PDD_CONTEXT      pContext,
    DWORD                     dwEndpoint,
    DWORD                   dwDirection
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();
    EnableDisableEndpointInterrupt(pContext, dwEndpoint, dwDirection, FALSE);
    FUNCTION_LEAVE_MSG();
}


// Reset an endpoint
static
VOID
ResetEndpoint(
    PCTRLR_PDD_CONTEXT         pContext,
    EP_STATUS                 *peps
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    ValidateContext(pContext);
    PREFAST_DEBUGCHK(peps);

    // Since Reset can be called before/after an Endpoint has been configured,
    // it is best to clear all IN and OUT bits associated with endpoint.
    DWORD dwEndpoint = peps->dwEndpointNumber;

    // All Endpoints interrupt register clear
    if(dwEndpoint == 0)
    {
        WriteEPSpecificReg(dwEndpoint, DIEPINT, 0x3f);
        WriteEPSpecificReg(dwEndpoint, DOEPINT, 0x3f);
    }
    else if (dwEndpoint < ENDPOINT_COUNT)
    {
        WriteEPSpecificReg(dwEndpoint, DIEPINT, 0x3f);
        WriteEPSpecificReg(dwEndpoint, DOEPINT, 0x3f);
        DisableEndpointInterrupt(pContext, peps->dwEndpointNumber, peps->dwDirectionAssigned);
    }

    FUNCTION_LEAVE_MSG();
}


// Initialize OTG device register
static
VOID
InitDevice(
    PCTRLR_PDD_CONTEXT     pContext
    )
{
    WriteReg(GUSBCFG, NP_TXFIFO_REWIND_EN | TURNAROUND_TIME | PHYIF_16BIT | 0x7<<HS_FS_TIMEOUT);
    WriteReg(DCFG, 0x1f<<IN_EP_MISS_CNT_IDX | DEVICE_SPEED_HIGH);               
    WriteReg(DAINTMSK, EP0_OUT_INT | EP0_IN_INT);        // EP0 IN,OUT interrupt Unmask

    WriteReg(GOTGCTL,SESSION_REQUEST);
    WriteReg(GRXFSIZ, RXFIFO_DEPTH);                        // Rx FIFO Size
    WriteReg(GNPTXFSIZ, NPTXFIFO_DEPTH<<NPTXFIFO_DEPTH_IDX | RXFIFO_DEPTH<<0);        // Non Periodic Tx FIFO Size
    WriteReg(DOEPMSK, SETUP_PHASE_DONE | XFER_COMPLETE);
    WriteReg(DIEPMSK, TIMEOUT_CONDITION | XFER_COMPLETE);

    WriteReg(GINTMSK, INT_RESUME | INT_OUT_EP | INT_IN_EP | INT_EPMIS | INT_SDE | INT_RESET | INT_SUSPEND | INT_OTG);    //gint unmask
    WriteReg(GAHBCFG, NPTXFEMPLVL_COMPLETE_EMPTY | MODE_DMA | BURST_SINGLE | GBL_INT_UNMASK);

    WriteReg(GRSTCTL, TXFFLSH | RXFFLSH);

    DWORD dwTimeOutCount;
    for(dwTimeOutCount=0; dwTimeOutCount<1000; dwTimeOutCount++)
    {
        if ((ReadReg(GRSTCTL)& (TXFFLSH | RXFFLSH))==0)
            break;
    }
    if(dwTimeOutCount > 999)
        RETAILMSG(UFN_ZONE_ERROR,(_T("[UFNPDD] TX & RX FIFO Flush TimeOut Error\r\n"))); 
}

static
VOID
InitPDDContext(
    PCTRLR_PDD_CONTEXT     pContext
    )
{
    pContext->dwPipelinedEP = 0;
    pContext->bRingBufferFull = FALSE;
    pContext->dwPipelinedStrIdx = pContext->dwPipelinedEndIdx = 0;
    pContext->dwPipelinedXferSize = 1024*64;

    pContext->dwXmittingEP = 0;
    pContext->dwXmitReadyCnt = 0;
    pContext->bOutEPDMAStartFlag = FALSE;
    
    for(DWORD i=0; i<ENDPOINT_COUNT; ++i) 
    {
        pContext->dwInEPRunning[i] = 0;
        for(DWORD depth=0; depth<PIPELINED_FIFO_DEPTH; depth++)
            pContext->dwPipelinedXfered[i][depth] = 0;
    }
}

static 
inline
VOID
SetSoftDisconnect()
{
    SetClearReg(DCTL, SOFT_DISCONNECT, SET);
}


// This function is used to dicconnect to host.
// As long as this function is called, the host will not see that the device is connected,
// and the device will not receive signals on the USB.
static
inline
VOID
ClearSoftDisconnect()
{
    SetClearReg(DCTL, SOFT_DISCONNECT, CLEAR);
}


// USB signal mask to prevent unwanted leakage
// This function should be called before USB PHY is used
void MaskUSBSignal(
    PCTRLR_PDD_CONTEXT     pContext
    )
{
    volatile DWORD dwRegValue;
    dwRegValue = pContext->pSYSCONregs->OTHERS;
    dwRegValue |= USB_SIG_MASK;
    pContext->pSYSCONregs->OTHERS = dwRegValue;
}


// All module state machines (except the AHB Slave Unit) are reset to the IDLE state,
// and all the transmit FIFOs and the receive FIFO are flushed.
static
void
SoftResetCore(
    PCTRLR_PDD_CONTEXT     pContext
    )
{
    WriteReg(GRSTCTL, CSFTRST);

    DWORD dwTimeOutCount;
    for(dwTimeOutCount=0; dwTimeOutCount<1000; dwTimeOutCount++)
    {
        if ((ReadReg(GRSTCTL) & AHBIDLE))
            break;
    }
    if (dwTimeOutCount > 999)
        RETAILMSG(UFN_ZONE_ERROR,(_T("[UFNPDD] SoftCoreReset TimeOut Error\r\n")));
}


// Reset the device and EP0.
static
VOID
ResetDevice(
    PCTRLR_PDD_CONTEXT     pContext
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    DEBUGCHK(IS_VALID_SC6410_CONTEXT(pContext));

    SoftResetCore(pContext);
    InitDevice(pContext);

    // Reset all endpoints
    for (DWORD dwEpIdx = 0; dwEpIdx < ENDPOINT_COUNT; ++dwEpIdx)
    {
        EP_STATUS *peps = GetEpStatus(pContext, dwEpIdx);
        ResetEndpoint(pContext, peps);
    }

    FUNCTION_LEAVE_MSG();
}


// After transmit a packet, this function should be called to let MDD know.
static
VOID
CompleteTransfer(
    PCTRLR_PDD_CONTEXT     pContext,
    PEP_STATUS             peps,
    DWORD                 dwUsbError
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PSTransfer pTransfer = peps->pTransfer;
    
    peps->pTransfer = NULL;

    pTransfer->dwUsbError = dwUsbError;
    pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_TRANSFER_COMPLETE, (DWORD) pTransfer);

    FUNCTION_LEAVE_MSG();
}


// Wating until TxFIFO Empty ++//
static
BOOL
WatiForTxFIFOEmpty(
    DWORD dwWaitCount
    )
{
    for(DWORD i = 0 ; i < dwWaitCount ; i++)
    {
        if(ReadReg(GINTSTS) & INT_TX_FIFO_EMPTY)
        {
            break;
        }
        Sleep(1);
        if (i >= (dwWaitCount-1))
        {
            RETAILMSG(UFN_ZONE_ERROR,(_T("[UFNPDD] TX FIFO Not Empty Error!!! Wait Coutnt %d - StartPreparedInTrasfer\r\n"),i));
            return FALSE;
        }
    }
    return TRUE;
}    
// Wating until TxFIFO Empty --//


// This function is only vaild in debug build.
#ifdef DEBUG
static
VOID
ValidateTransferDirection(
    PCTRLR_PDD_CONTEXT     pContext,
    PEP_STATUS             peps,
    PSTransfer             pTransfer
    )
{
    DEBUGCHK(pContext);
    PREFAST_DEBUGCHK(peps);
    PREFAST_DEBUGCHK(pTransfer);

    if (peps->dwEndpointNumber != 0)
    {
        DEBUGCHK(peps->dwDirectionAssigned == pTransfer->dwFlags);
    }
}
#else
#define ValidateTransferDirection(ptr1, ptr2, ptr3)
#endif


#if TEST_MODE_SUPPORT
static BOOL
CheckForUSBTestModeRequest(
    PCTRLR_PDD_CONTEXT pContext
    )
{
    bool fTest = FALSE;

    volatile DWORD dwDCTL;

    PEP_STATUS peps = GetEpStatus(pContext, 0);
    LOCK_ENDPOINT(peps);
    
    // is this a request to enter a test mode?
    if( pContext->UDR.bmRequestType     == (USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_STANDARD | USB_REQUEST_FOR_DEVICE)
            && pContext->UDR.bRequest     == USB_REQUEST_SET_FEATURE
            && pContext->UDR.wValue     == USB_FEATURE_TEST_MODE
            && (pContext->UDR.wIndex & 0xFF) == 0) 
    {
        pContext->sendDataEnd = TRUE;
        pContext->Ep0State = EP0_STATE_IDLE;       


        RETAILMSG(UFN_ZONE_USB_EVENTS, (_T("[UFNPDD] USB_FEATURE_TEST_MODE\r\n")));
        // Set TEST MODE

        RETAILMSG(UFN_ZONE_USB_EVENTS, (_T("[UFNPDD] TEST_FORCE_ENABLE\r\n")));
        //Set Test Force Enable
        WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
        WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 3<<0);    //ep0 enable, clear nak, 8byte

        dwDCTL = ReadReg(DCTL);
        TM_Enabled(dwDCTL);
        WriteReg(DCTL, dwDCTL);


        USHORT wTestMode = pContext->UDR.wIndex >> 8;

        switch( wTestMode)
        {
            case USB_TEST_J:

                RETAILMSG(UFN_ZONE_USB_EVENTS, (_T("[UFNPDD] USB_TEST_J\r\n")));
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 3<<0);    //ep0 enable, clear nak, 8byte
                dwDCTL = ReadReg( DCTL);
                TM_J_Selected(dwDCTL);                    
                WriteReg(DCTL, dwDCTL);                
            break;

            case USB_TEST_K:

                RETAILMSG(UFN_ZONE_USB_EVENTS, (_T("[UFNPDD] USB_TEST_K\r\n")));
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 3<<0);    //ep0 enable, clear nak, 8byte
                dwDCTL = ReadReg( DCTL);
                TM_K_Selected(dwDCTL);                    
                WriteReg(DCTL, dwDCTL);                
                
            break;

            case USB_TEST_SE0_NAK:

                RETAILMSG(UFN_ZONE_USB_EVENTS, (_T("[UFNPDD] USB_TEST_SE0_NAK\r\n")));
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 3<<0);    //ep0 enable, clear nak, 8byte
                dwDCTL = ReadReg( DCTL);
                TM_SN_Selected(dwDCTL);                    
                WriteReg(DCTL, dwDCTL);                
            break;

            case USB_TEST_PACKET:
            {
                RETAILMSG(UFN_ZONE_USB_EVENTS, (_T("[UFNPDD] USB_TEST_PACKET\r\n")));
                DWORD cbWritten = 0;
                WORD WriteData = 0;

                memcpy( pContext->pVAddrEP[0][IN_EP] ,ahwTestPkt, TEST_PKT_SIZE);
                WatiForTxFIFOEmpty(100);    
                WriteReg(DIEPDMA0, pContext->pPAddrEP[0][IN_EP]);
                
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 3<<0);    //ep0 enable, clear nak, 8byte
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | TEST_PKT_SIZE); 
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | EP0_MAX_PK_SIZ);    //ep0 enable, clear nak, 64byte

                WatiForTxFIFOEmpty(100);    

                dwDCTL = ReadReg(DCTL);
                TM_PKT_Selected(dwDCTL);        
                RETAILMSG(UFN_ZONE_USB_EVENTS,(_T("[UFNPDD] DCTL: 0x%x\r\n"),dwDCTL));                
                WriteReg(DCTL, dwDCTL);
            }
            break;


            case USB_TEST_FORCE_ENABLE:
                RETAILMSG(UFN_ZONE_USB_EVENTS, (_T("[UFNPDD] TEST_FORCE_ENABLE\r\n")));
                //Set Test Force Enable
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 3<<0);    //ep0 enable, clear nak, 8byte

                dwDCTL = ReadReg(DCTL);
                TM_Enabled(dwDCTL);
                WriteReg(DCTL, dwDCTL);
            break;                
        }
        
        fTest = TRUE;
    }

    UNLOCK_ENDPOINT(peps);

    return fTest;
}
#endif


// Process an endpoint0 interrupt.  
static
VOID
HandleEndpoint0Event(
    PCTRLR_PDD_CONTEXT    pContext
    )
{                    
    SETFNAME();
    FUNCTION_ENTER_MSG();
    ValidateContext(pContext);
    DEBUGCHK(pContext->fRunning);
    PEP_STATUS peps = GetEpStatus(pContext, 0);
    LOCK_ENDPOINT(peps);

    // Write 0 to SEND_STALL and SENT_STALL to clear them, so we need to
    // leave them unchanged by default.
    BOOL fSendUDR = FALSE;
    BOOL fCompleted = FALSE;
    DWORD dwStatus;

    if (pContext->Ep0State == EP0_STATE_IDLE)
    {
        if (pContext->fSpeedReported == FALSE)
        {
            // After Every Reset Notify MDD of Speed setting.
            // This device can support both FULL and HIGH speed.
            // It will be process in HandleUSBBusIrq INT_SDE.
            if (pContext->dwDetectedSpeed == USB_FULL)
            {
                pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_SPEED, BS_FULL_SPEED);
            }
            else if(pContext->dwDetectedSpeed == USB_HIGH)
            {
                pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_SPEED, BS_HIGH_SPEED);
            }
            else
            {
                DEBUGMSG(ZONE_ERROR,(_T("SPEED Exeption LowSpeed \r\n")));
            }
            pContext->fSpeedReported = TRUE;
        }
        
        if (pContext->bRdySetupPkt == TRUE)    
        {
            pContext->bRdySetupPkt = FALSE;

            // Initialize EP0 Out DMA to get a SetUp packet            
            WriteReg(DOEPDMA0, pContext->pPAddrEP[0][OUT_EP]);
            WriteReg(DOEPTSIZ0, 1<<SETUP_PKT_CNT_IDX | 8);            
            WriteReg(DOEPCTL0, EP_ENABLE | SET_NAK | EP0_MAX_PK_SIZ);   //WriteReg(DOEPCTL0, EP_ENABLE | CLEAR_NAK | EP0_MAX_PK_SIZ);
        }
        else
        {
            PDWORD pbUDR = (PDWORD) &pContext->UDR;
               memcpy(pbUDR, pContext->pVAddrEP[0][OUT_EP] ,sizeof(pContext->UDR));
    
#if TEST_MODE_SUPPORT         
            if(CheckForUSBTestModeRequest(pContext)) //for testmode
            {
                // DO NOTHING
            }
            else if (8 != sizeof(pContext->UDR))
#else
            if (8 != sizeof(pContext->UDR))
#endif
            {
                DEBUGMSG(ZONE_ERROR, (_T("%s Setup packet was only !!\r\n"), pszFname));
                // Ideally this should not hapen. This is a recovery mechanism if
                // we get out of sync somehow.
            }
            else
            {
                // Parse the Setup Command this is necessary to Configure the
                // SW State Machine and to set bits to enable the HW to
                // ACK/NAK correctly.

                // Determine if this is a NO Data Packet
                if (pContext->UDR.wLength > 0)
                {
                    // Determine transfer Direction
                    if (pContext->UDR.bmRequestType & USB_ENDPOINT_DIRECTION_MASK)
                    {
                        // Start the SW IN State Machine
                        pContext->Ep0State = EP0_STATE_IN_DATA_PHASE;
                    }
                    else
                    {
                        // Start the SW OUT State Machine
                        pContext->Ep0State = EP0_STATE_OUT_DATA_PHASE;
                    }
                    pContext->sendDataEnd = FALSE;
                }
                else
                {   // UDR.wLength == 0
                    // ClientDriver will issue a SendControlStatusHandshake to
                    // complete the transaction.
                    pContext->sendDataEnd = TRUE;
                    // Nothing left to do... stay in IDLE.
                    DEBUGCHK(pContext->Ep0State == EP0_STATE_IDLE);
                }
                fSendUDR = TRUE;
            }
        }
    }

    else if (pContext->Ep0State == EP0_STATE_OUT_DATA_PHASE)
    {
        DEBUGMSG(ZONE_TRANSFER, (_T("EP0_OUT_PHASE\r\n")));
    
           DWORD dwCountBytes = ReadReg(DOEPTSIZ0) & 0x7f;
        dwCountBytes = pContext->dwEp0MaxPktSize - dwCountBytes;
        peps->pTransfer->cbTransferred += dwCountBytes;

        DWORD dwRemainBytes = peps->pTransfer->cbBuffer - peps->pTransfer->cbTransferred;

        if (dwRemainBytes == 0)
        {                
            pContext->Ep0State = EP0_STATE_IDLE;
            pContext->sendDataEnd = TRUE;
               memcpy(peps->pTransfer->pvBuffer, pContext->pVAddrEP[0][OUT_EP] , peps->pTransfer->cbBuffer);
            dwStatus = UFN_NO_ERROR;
            fCompleted = TRUE;
            pContext->Ep0State = EP0_STATE_IDLE;
        }
        else
        {
            WriteReg(DOEPDMA0, pContext->pPAddrEP[0][OUT_EP] + peps->pTransfer->cbTransferred);
            WriteReg(DOEPTSIZ0, 1<<PACKET_COUTNT_IDX | pContext->dwEp0MaxPktSize);
            WriteReg(DOEPCTL0, EP_ENABLE | CLEAR_NAK | EP0_MAX_PK_SIZ);
        }
    }
    else
    {
        DEBUGMSG(ZONE_TRANSFER, (_T("EP0_IN_PHASE\r\n")));
        
        if(peps->pTransfer->cbBuffer >= (pContext->dwEp0MaxPktSize + peps->pTransfer->cbTransferred))
            peps->pTransfer->cbTransferred += pContext->dwEp0MaxPktSize;        
        else
            peps->pTransfer->cbTransferred = peps->pTransfer->cbBuffer;

        DWORD dwRemainBytes = peps->pTransfer->cbBuffer - peps->pTransfer->cbTransferred;

        if (dwRemainBytes == 0)
        {
            dwStatus = UFN_NO_ERROR;
            fCompleted = TRUE;
            pContext->Ep0State = EP0_STATE_IDLE;
        }
        else
        {
            WatiForTxFIFOEmpty(100);
            WriteReg(DIEPDMA0, pContext->pPAddrEP[0][IN_EP] + peps->pTransfer->cbTransferred);
            if (dwRemainBytes >= pContext->dwEp0MaxPktSize)
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | pContext->dwEp0MaxPktSize);
            else
                  WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | dwRemainBytes);      
            WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 1<<NEXT_EP_IDX | EP0_MAX_PK_SIZ);
        }
    }

    if (fCompleted)
    {
        CompleteTransfer(pContext, peps, dwStatus);
    }
    
    if (fSendUDR)
    {
        pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_SETUP_PACKET, (DWORD) &pContext->UDR);
    }

    FUNCTION_LEAVE_MSG();
    UNLOCK_ENDPOINT(peps);
}


// Process an endpoint interrupt except endpoint0  
static
VOID
HandleOutEvent(
    PCTRLR_PDD_CONTEXT      pContext,
    DWORD                        dwEndpoint
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    ValidateContext(pContext);
    DEBUGCHK(pContext->fRunning);
    DEBUGCHK(dwEndpoint != 0);

    EP_STATUS *peps = GetEpStatus(pContext, dwEndpoint);
    PREFAST_DEBUGCHK(peps);

    LOCK_ENDPOINT(peps);

    PSTransfer pTransfer = peps->pTransfer;
    
    DWORD dwCountBytes = ReadEPSpecificReg(dwEndpoint, DOEPTSIZ) & 0x7FFFF;
    pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedStrIdx] = pContext->dwPipelinedXferSize - dwCountBytes;
    if(pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedStrIdx] > pContext->dwPipelinedXferSize)
        pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedStrIdx] = pContext->dwPipelinedXferSize;
    //For MSC compliance test : one of cases, A host send the data which is bigger than a device expect to get
    
    pContext->dwPipelinedStrIdx = GET_NEXT_PF_IDX(pContext->dwPipelinedStrIdx);
    
    if(pContext->dwUSBClassInfo != USB_MSF)
    {    
        if(pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedStrIdx] == 0)
        {
            WriteEPSpecificReg(dwEndpoint, DOEPDMA, pContext->pPAddrEP[dwEndpoint][pContext->dwPipelinedStrIdx]);
            WriteEPSpecificReg(dwEndpoint, DOEPTSIZ, pContext->dwPipelinedPktCnt<<PACKET_COUTNT_IDX | pContext->dwPipelinedXferSize);
            DWORD dwTimeOutCount;
            for(dwTimeOutCount=0; dwTimeOutCount<1000; dwTimeOutCount++)        
            {
                WriteEPSpecificReg(dwEndpoint, DOEPCTL, EP_ENABLE | CLEAR_NAK | SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
                if ((ReadEPSpecificReg(dwEndpoint, DOEPCTL) & 0x20000) == 0x0)
                    break;
            }
            if (dwTimeOutCount>999)
                RETAILMSG(UFN_ZONE_ERROR,(TEXT("Clear NAK TimeOut Error %d [HandleOutEvent]\r\n"),dwTimeOutCount));        }
        else
        {
            RETAILMSG(UFN_ZONE_WARNING,(TEXT("RING BUFFER STATE : FULL\r\n")));
            DisableEndpointInterrupt(pContext, peps->dwEndpointNumber, peps->dwDirectionAssigned);
            pContext->bRingBufferFull = TRUE;
        }
    }

    if(pContext->dwPipelinedEP != 0)
    {
        memcpy(pTransfer->pvBuffer, \
                  pContext->pVAddrEP[dwEndpoint][pContext->dwPipelinedEndIdx] , \
                  pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedEndIdx]); 

        pTransfer->cbTransferred += pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedEndIdx];
        pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedEndIdx] = 0;
        pContext->dwPipelinedEndIdx = GET_NEXT_PF_IDX(pContext->dwPipelinedEndIdx);
        pContext->dwPipelinedEP = 0;
        DisableEndpointInterrupt(pContext, peps->dwEndpointNumber, peps->dwDirectionAssigned);    
        CompleteTransfer(pContext, peps, UFN_NO_ERROR);
    }

    UNLOCK_ENDPOINT(peps);
    FUNCTION_LEAVE_MSG();
}


static
VOID
HandleInEvent(
    PCTRLR_PDD_CONTEXT    pContext,
    DWORD                        dwEndpoint
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    ValidateContext(pContext);
    DEBUGCHK(pContext->fRunning);
    DEBUGCHK(dwEndpoint != 0);

    EP_STATUS *peps = GetEpStatus(pContext, dwEndpoint);
    PREFAST_DEBUGCHK(peps);

    LOCK_ENDPOINT(peps);

    if(pContext->dwInEPRunning[dwEndpoint])
    {
        pContext->dwInEPRunning[dwEndpoint] = 0;
        pContext->dwXmitReadyCnt--;
        pContext->dwXmittingEP |= (1<<dwEndpoint);

        if (pContext->dwXmitReadyCnt == 0)
        {
            WriteReg(DIEPMSK, TIMEOUT_CONDITION | XFER_COMPLETE);
        }

        PSTransfer pTransfer = peps->pTransfer;

        DWORD dwPktcnt, dwBytes;

        if (pTransfer->cbBuffer == 0)
        {
            dwPktcnt = 1;
        }
        else
        {
            dwPktcnt = pTransfer->cbBuffer / peps->dwPacketSizeAssigned;
            dwBytes = pTransfer->cbBuffer % peps->dwPacketSizeAssigned;
            if (dwBytes) dwPktcnt++;
        }

        WatiForTxFIFOEmpty(100);
        WriteEPSpecificReg(dwEndpoint, DIEPDMA, pContext->pPAddrEP[dwEndpoint][IN_EP]);
        WriteEPSpecificReg(dwEndpoint, DIEPTSIZ, dwPktcnt<<PACKET_COUTNT_IDX | pTransfer->cbBuffer);


        if (pContext->dwUSBClassInfo == USB_RNDIS)
        {
            if (dwEndpoint == 1)
                WriteEPSpecificReg(dwEndpoint, DIEPCTL, EP_ENABLE | CLEAR_NAK | SET_TYPE_BULK | USB_ACT_EP | 2<<NEXT_EP_IDX | peps->dwPacketSizeAssigned);     
            else
                WriteEPSpecificReg(dwEndpoint, DIEPCTL, EP_ENABLE | CLEAR_NAK | SET_TYPE_BULK | USB_ACT_EP | 0<<NEXT_EP_IDX | peps->dwPacketSizeAssigned);     
        }
        else
        {
            WriteEPSpecificReg(dwEndpoint, DIEPCTL, EP_ENABLE | CLEAR_NAK | SET_TYPE_BULK | USB_ACT_EP | 0<<NEXT_EP_IDX | peps->dwPacketSizeAssigned);     
        }
    }

    UNLOCK_ENDPOINT(peps);
    FUNCTION_LEAVE_MSG();
}


static
VOID
SetEndpoint(
    PCTRLR_PDD_CONTEXT    pContext
    )
{
    DWORD dwEndpoint;

     // Clear all device endpoint interrupt
    for(dwEndpoint=0; dwEndpoint<ENDPOINT_COUNT; dwEndpoint++)
    {
        WriteEPSpecificReg(dwEndpoint, DIEPINT, 0x3f);
        WriteEPSpecificReg(dwEndpoint, DOEPINT, 0x3f);
    }
}


inline
static
VOID
SetAllOutEpNak()
{
    DWORD dwEndpoint;

    for(dwEndpoint=0; dwEndpoint<ENDPOINT_COUNT; dwEndpoint++)
    {
        SetClearEPSpecificReg(dwEndpoint, DOEPCTL, SET_NAK, SET);
    }
}


inline
static
VOID
ClearAllOutEpNak()
{
    DWORD dwEndpoint;

    for(dwEndpoint=0; dwEndpoint<ENDPOINT_COUNT; dwEndpoint++)
    {
           SetClearEPSpecificReg(dwEndpoint, DOEPCTL, CLEAR_NAK, SET);
    }
}


static
VOID
HandleReset(
    PCTRLR_PDD_CONTEXT    pContext
    )
{
    DWORD dwGOTGCTL = ReadReg(GOTGCTL);

    if (pContext->attachedState == UFN_DETACH)
    {
        if(dwGOTGCTL & (B_SESSION_VALID|A_SESSION_VALID))
        {
            if (pContext->IsFirstReset == TRUE)
            {
                Sleep(2000);        // For Sleep Wake Up Connection issue.
            }
            pContext->IsFirstReset = FALSE;            

            SetAllOutEpNak();

            // Below function(ResetDevice) make USB compliance Test(chir timimg) fail. 
            // So If your usb H/W have usb plug detect signal, This function should be moved to Plug thread fucntion.
            ResetDevice(pContext);          
            
            ClearAllOutEpNak();                    

            pContext->fSpeedReported = FALSE;
            pContext->Ep0State = EP0_STATE_IDLE;
            pContext->attachedState = UFN_ATTACH;
            pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_ATTACH);

            RETAILMSG(UFN_ZONE_USB_EVENTS,(_T("[UFNPDD] OTG Cable Attached\r\n")));
        }
        else
        {
            pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_DETACH);
            pContext->attachedState = UFN_DETACH;                
            RETAILMSG(UFN_ZONE_USB_EVENTS,(_T("[UFNPDD] RESET Exeption \r\n")));
        }
    }
    else
    {
        if(!(dwGOTGCTL & (B_SESSION_VALID|A_SESSION_VALID)))
        {
            pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_DETACH);
            pContext->attachedState = UFN_DETACH;                
        }
        else
        {        
            // USB Device Address field should be cleared per every reset time.
            // If not, USB Compliance Frame work test(EnumerationTest) make a fail.
            volatile DWORD wDCFG = ReadReg(DCFG);
            WriteReg(DCFG, (wDCFG & ~(DEVICE_ADDRESS_MSK)));    
            
            InitDevice(pContext);
            InitPDDContext(pContext);

            // Initialize EP0 Out DMA to get a SetUp packet
            WriteReg(DOEPDMA0, pContext->pPAddrEP[0][OUT_EP]);
            WriteReg(DOEPTSIZ0, 1<<SETUP_PKT_CNT_IDX | 8);            
            WriteReg(DOEPCTL0, EP_ENABLE | SET_NAK | EP0_MAX_PK_SIZ);   //WriteReg(DOEPCTL0, EP_ENABLE | CLEAR_NAK | EP0_MAX_PK_SIZ);
        
            RETAILMSG(UFN_ZONE_USB_EVENTS,(_T("[UFNPDD] RESET Again \r\n")));
        }
    }
}


// Process USB Bus interrupt
static
VOID
HandleUSBBusIrq(
    PCTRLR_PDD_CONTEXT    pContext,
    DWORD               bUSBBusIrqStat
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    ValidateContext(pContext);

    if (bUSBBusIrqStat & INT_RESET)
    {
        HandleReset(pContext);
        WriteReg(GINTSTS, INT_RESET);
        DEBUGMSG(ZONE_USB_EVENTS, (_T("%s Reset\r\n"), pszFname));
        pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_RESET);
    }

    // for Transmit data holding issue in RNDIS ++ //
    if (bUSBBusIrqStat & INT_EPMIS)
    {
        WriteReg(GINTSTS, INT_EPMIS);
        ClearAllOutEpNak();
        SetClearReg(DCTL, CLEAR_GOUTNAK, SET);
    }
    // for Transmit data holding issue in RNDIS -- //

    if (bUSBBusIrqStat & INT_SDE)
    {
        WriteReg(GINTSTS, INT_SDE);
        DWORD dwDSTS = ReadReg(DSTS);
        if (((dwDSTS & ENUM_SPEED_MSK)>>1) == USB_HIGH)
        {
            pContext->dwDetectedSpeed = USB_HIGH;
            pContext->dwEp0MaxPktSize = EP_0_PACKET_SIZE;
        }
        else if(((dwDSTS & ENUM_SPEED_MSK)>>1) == USB_FULL)
        {
            pContext->dwDetectedSpeed = USB_FULL;
            pContext->dwEp0MaxPktSize = EP_0_PACKET_SIZE;
        }
        else
        {
            RETAILMSG(UFN_ZONE_ERROR,(_T("[UFNPDD] INT_SDE_EXEPTION Occured! \r\n")));
        }
        SetEndpoint(pContext);

        // Enable Endpoint0 interrupt
        EnableEndpointInterrupt(pContext, 0, USB_IN_TRANSFER);
        EnableEndpointInterrupt(pContext, 0, USB_OUT_TRANSFER);        
    }

    if (bUSBBusIrqStat & INT_RESUME)
    {
        WriteReg(GINTSTS, INT_RESUME);
        pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_RESUME);

        // Enable Endpoint0 interrupt
        EnableEndpointInterrupt(pContext, 0, USB_IN_TRANSFER);
        EnableEndpointInterrupt(pContext, 0, USB_OUT_TRANSFER);        
    }

    if (bUSBBusIrqStat & INT_SUSPEND)
    {
        WriteReg(GINTSTS, INT_SUSPEND);
        pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_SUSPEND);
    }

    if (bUSBBusIrqStat & INT_OTG)
    {
        DWORD dwGotgint = ReadReg(GOTGINT);
        if (dwGotgint & SesEndDet)
        {    
            RETAILMSG(UFN_ZONE_USB_EVENTS,(_T("[UFNPDD] OTG Cable Detached\r\n")));
            pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_DETACH);
            pContext->attachedState = UFN_DETACH;
            pContext->IsFirstReset = TRUE;
            pContext->bOutEPDMAStartFlag = FALSE;
        }
        else
        {
            DEBUGMSG(ZONE_ERROR,(_T("[UFNPDD] OTG Interrupt Exeption %x\r\n"),dwGotgint));
        }
        WriteReg(GOTGINT, dwGotgint);
    }
    FUNCTION_LEAVE_MSG();
}


static
VOID
HandleUSBEvent(
    PCTRLR_PDD_CONTEXT    pContext
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();
    ValidateContext(pContext);
#if TEST_MODE_SUPPORT
    volatile DWORD dwDCTL;
#endif

    DWORD dwGintsts = ReadReg(GINTSTS);
    DWORD dwDaint = ReadReg(DAINT);
    DWORD dwDaintMsk = ReadReg(DAINTMSK);

    if (dwGintsts & (INT_RESUME | INT_EPMIS | INT_SDE | INT_RESET | INT_SUSPEND | INT_OTG))
    {
        HandleUSBBusIrq(pContext, dwGintsts);
    }

    if ((pContext->attachedState != UFN_DETACH) && (dwGintsts & INT_OUT_EP))
    {
        if (dwDaint & EP0_OUT_INT)
        {            
            volatile DWORD dwDoepint0 = ReadReg(DOEPINT0);
            WriteReg(DOEPINT0, dwDoepint0);

            if (dwDoepint0 & XFER_COMPLETE)
            {
                SetClearReg(DOEPCTL0, SET_NAK, SET);
                
                HandleEndpoint0Event(pContext);
            }
            
            if (dwDoepint0 & SETUP_PHASE_DONE)
            {
                SetClearReg(DOEPCTL0, SET_NAK, SET);
                
                HandleEndpoint0Event(pContext);
            }
        }
        for(DWORD dwEndpoint = 1; dwEndpoint < ENDPOINT_COUNT; ++ dwEndpoint)
        {
            DWORD dwEpBit = EpToIrqStatBit(dwEndpoint) << ENDPOINT_COUNT;
            if (dwDaint & dwEpBit)
            {
                volatile DWORD dwDoepint = ReadEPSpecificReg(dwEndpoint, DOEPINT);
                WriteEPSpecificReg(dwEndpoint, DOEPINT, dwDoepint);

                if (dwDoepint & XFER_COMPLETE)
                {                        
                    HandleOutEvent(pContext, dwEndpoint);
                }
            }
        }        
        
    }

    if ((pContext->attachedState != UFN_DETACH) && (dwGintsts & INT_IN_EP))
    {            
        if (dwDaint & EP0_IN_INT)
        {
            volatile DWORD dwDiepint0 = ReadReg(DIEPINT0);
            WriteReg(DIEPINT0, dwDiepint0);

            if (dwDiepint0 & XFER_COMPLETE)
            {            
                HandleEndpoint0Event(pContext);
            }
            
            if (dwDiepint0 & TIMEOUT_CONDITION)
            {
                RETAILMSG(UFN_ZONE_WARNING,(_T("[UFNPDD] Time Out EP0\r\n")));
                SetClearReg(DCTL, CLEAR_GNPINNAK, SET);
            }
        }
        for(DWORD dwEndpoint = 1; dwEndpoint < ENDPOINT_COUNT; ++ dwEndpoint)
        {
            DWORD dwEpBit = EpToIrqStatBit(dwEndpoint);
            if ((dwDaint & dwEpBit) && (dwDaintMsk & (0x1<<dwEndpoint)))
            {
                volatile DWORD dwDiepint = ReadEPSpecificReg(dwEndpoint, DIEPINT);
                WriteEPSpecificReg(dwEndpoint, DIEPINT, dwDiepint);

                if (dwDiepint & IN_TKN_RECEIVED)
                {
                    HandleInEvent(pContext, dwEndpoint);
                }
                if (dwDiepint & XFER_COMPLETE)
                {
                    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
                    LOCK_ENDPOINT(peps);
                    pContext->dwXmittingEP &= ~(1<<dwEndpoint);
                    peps->pTransfer->cbTransferred += peps->pTransfer->cbBuffer;
                    DisableEndpointInterrupt(pContext, peps->dwEndpointNumber, peps->dwDirectionAssigned);
                    CompleteTransfer(pContext, peps, UFN_NO_ERROR);
                    UNLOCK_ENDPOINT(peps);
                }
                if (dwDiepint & TIMEOUT_CONDITION)
                {
                    RETAILMSG(UFN_ZONE_WARNING,(_T("[UFNPDD] Time Out EP%d\r\n"),dwEndpoint));
                    RETAILMSG(UFN_ZONE_WARNING,(_T("[UFNPDD] Transmit Ready Count : %d\r\n"), pContext->dwXmitReadyCnt));
                    RETAILMSG(UFN_ZONE_WARNING,(_T("[UFNPDD] Transmitting EP 0x%x : %d\r\n"), dwEndpoint, pContext->dwXmittingEP));
                    for(DWORD dwEndpoint = 0; dwEndpoint < ENDPOINT_COUNT; ++dwEndpoint)
                    {
                        if(pContext->dwInEPRunning[dwEndpoint])
                        {
                            RETAILMSG(UFN_ZONE_WARNING,(_T("[UFNPDD]EP%d is running\r\n"),dwEndpoint));
                        }
                    }
                    
                    SetClearReg(DCTL, CLEAR_GNPINNAK, SET);
                }
            }
        }
    }

#if TEST_MODE_SUPPORT
    dwDCTL = ReadReg( DCTL);
    dwDCTL = dwDCTL & (TEST_MODE_MASK) | CLEAR_GOUTNAK;
    WriteReg(DCTL, dwDCTL);
#endif    
    SetClearReg(DCTL, CLEAR_GNPINNAK, SET);
    FUNCTION_LEAVE_MSG();
}

static
DWORD MapRegisterSet(
    PCTRLR_PDD_CONTEXT     pContext
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();
    
    ValidateContext(pContext);
    DEBUGCHK(g_pUDCBase == NULL);
    
    PBYTE  pVMem = NULL;
    DWORD dwRet = ERROR_SUCCESS;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
    
    //System Controller registers allocation
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;
    pContext->pSYSCONregs = (volatile S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (pContext->pSYSCONregs == NULL)
    {
        dwRet = GetLastError();
        RETAILMSG(UFN_ZONE_ERROR, (_T("%s MmMapIoSpace: FAILED\r\n"), pszFname));
        goto CleanUp;
    }

    // OTG LINK registers.
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_USBOTG_LINK;   
    pVMem = (PBYTE)MmMapIoSpace(ioPhysicalBase, OTG_LINK_REG_SIZE, FALSE);
    if (pVMem == NULL)
    {
        dwRet = GetLastError();
        RETAILMSG(UFN_ZONE_ERROR, (_T("%s MmMapIoSpace: FAILED\r\n"), pszFname));
        goto CleanUp;
    }

    g_pUDCBase = pVMem + BASE_REGISTER_OFFSET;
    DEBUGMSG(UFN_ZONE_INIT, (_T("%s MmMapIoSpace, pVMem:%x\r\n"), pszFname, pVMem));

CleanUp:

    if (dwRet != ERROR_SUCCESS)
    {
        if (pContext->pSYSCONregs)
        {
            MmUnmapIoSpace((PVOID)pContext->pSYSCONregs, sizeof(S3C6410_SYSCON_REG));
            pContext->pSYSCONregs = NULL;
        }

        if (pVMem)
        {
            MmUnmapIoSpace((PVOID)pVMem, OTG_LINK_REG_SIZE);
            pVMem = NULL;
        }   
    }

    FUNCTION_LEAVE_MSG();
    return dwRet;
}


/*++
Routine Description:
Deallocate register space.

Arguments:
None.

Return Value:
None.
--*/
static
VOID
UnmapRegisterSet(
    PCTRLR_PDD_CONTEXT     pContext
    )
{
    // Unmap any memory areas that we may have mapped.

    if (pContext->pSYSCONregs)
    {
        MmUnmapIoSpace((PVOID)pContext->pSYSCONregs, sizeof(S3C6410_SYSCON_REG));
        pContext->pSYSCONregs = NULL;
    }

    if (g_pUDCBase)
    {
        MmUnmapIoSpace((PVOID)(g_pUDCBase - BASE_REGISTER_OFFSET), OTG_LINK_REG_SIZE);
        g_pUDCBase = NULL;
    }    
}


// Interrupt thread routine.
static
DWORD
WINAPI
ISTMain(
    LPVOID    lpParameter
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) lpParameter;
    ValidateContext(pContext);

    CeSetThreadPriority(pContext->hIST, pContext->dwISTPriority);

    while (!pContext->fExitIST)
    {
        pContext->fRestartIST = FALSE;

        // Disable All Endpoint interrupts
        WriteReg(DAINTMSK, 0); // Disable All Endpoint

        // Clear any outstanding device & endpoint interrupts
        // USB Device Interrupt Status - Write a '1' to Clear
        WriteReg(GINTSTS, INT_RESUME | INT_EPMIS | INT_SDE | INT_RESET | INT_SUSPEND); 

        // Enable Device General interrupts
        WriteReg(GINTMSK, INT_RESUME | INT_OUT_EP | INT_IN_EP | INT_EPMIS | INT_SDE | INT_RESET | INT_SUSPEND | INT_OTG);    

        // Enable Endpoint0 interrupt
        EnableEndpointInterrupt(pContext, 0, USB_IN_TRANSFER);
        EnableEndpointInterrupt(pContext, 0, USB_OUT_TRANSFER);

        while (TRUE)
        {
            DWORD dwWait = WaitForSingleObject(pContext->hevInterrupt, INFINITE);
            if (pContext->fExitIST || pContext->fRestartIST)
            {
                break;
            }

            if (dwWait == WAIT_OBJECT_0)
            {
                HandleUSBEvent(pContext);
                InterruptDone(pContext->dwSysIntr);
            }
            else
            {
                DEBUGMSG(UFN_ZONE_INIT, (_T("%s WaitForMultipleObjects failed. Exiting IST.\r\n"), pszFname));
                break;
            }
        }

        // Notify Detach Event to MDD
        pContext->pfnNotify(pContext->pvMddContext, UFN_MSG_BUS_EVENTS, UFN_DETACH);
        
        pContext->fSpeedReported = FALSE;
        pContext->attachedState = UFN_DETACH;

        // Disable Device  interrupts - write Zeros to Disable
        WriteReg(GINTMSK, 0);

        // Disable endpoint interrupts - write Zeros to Disable
        WriteReg(DAINTMSK, 0);

    }

    FUNCTION_LEAVE_MSG();

    return 0;
}

static
VOID
StartTransfer(
    PCTRLR_PDD_CONTEXT    pContext,
    PEP_STATUS             peps,
    PSTransfer             pTransfer
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    DEBUGCHK(pContext);
    PREFAST_DEBUGCHK(peps);

    DEBUGCHK(!peps->pTransfer);
    ValidateTransferDirection(pContext, peps, pTransfer);

    LOCK_ENDPOINT(peps);

    DEBUGMSG(ZONE_TRANSFER, (_T("%s Setting up %s transfer on ep %u for %u bytes\r\n"),
    pszFname, (pTransfer->dwFlags == USB_IN_TRANSFER) ? _T("in") : _T("out"),
    peps->dwEndpointNumber, pTransfer->cbBuffer));

    // Enable transfer interrupts.
    peps->pTransfer = pTransfer;
    DWORD dwEndpoint = peps->dwEndpointNumber;

    if (pTransfer->dwFlags == USB_IN_TRANSFER)
    {
        if (dwEndpoint == 0)
        {
            DWORD dwDIEPINT0 = ReadReg(DIEPINT0);
            if (dwDIEPINT0 & XFERCOPMPL)
            {
                WriteReg(DIEPINT0, XFERCOPMPL);
            }

            memcpy( pContext->pVAddrEP[0][IN_EP] ,pTransfer->pvBuffer, pTransfer->cbBuffer);
        
            WriteReg(DIEPDMA0, pContext->pPAddrEP[0][IN_EP]);
            if(pTransfer->cbBuffer >= pContext->dwEp0MaxPktSize)
            {
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | pContext->dwEp0MaxPktSize);
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 1<<NEXT_EP_IDX | EP0_MAX_PK_SIZ);
            }
            else
            {
                WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | pTransfer->cbBuffer);
                WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 1<<NEXT_EP_IDX | EP0_MAX_PK_SIZ);
            }
        }
        else
        {
            DWORD dwDIEPINT = ReadEPSpecificReg(dwEndpoint, DIEPINT);

            if (dwDIEPINT & XFERCOPMPL)
            {
                WriteEPSpecificReg(dwEndpoint, DIEPINT, XFERCOPMPL);
            }
            
            memcpy(pContext->pVAddrEP[dwEndpoint][IN_EP] ,pTransfer->pvBuffer, pTransfer->cbBuffer);

            WriteReg(DIEPMSK, IN_TKN_RECEIVED | TIMEOUT_CONDITION | XFER_COMPLETE);
            EnableEndpointInterrupt(pContext, dwEndpoint, peps->dwDirectionAssigned);

            pContext->dwXmitReadyCnt++;
            pContext->dwInEPRunning[dwEndpoint] = pContext->dwXmitReadyCnt;
        }
    }
    else //USB_OUT_TRANSFER
    {
        if (dwEndpoint == 0)
        {
            WriteReg(DOEPDMA0, pContext->pPAddrEP[0][OUT_EP]);
            WriteReg(DOEPTSIZ0, 1<<PACKET_COUTNT_IDX | pContext->dwEp0MaxPktSize);
            WriteReg(DOEPCTL0, EP_ENABLE | CLEAR_NAK | EP0_MAX_PK_SIZ);
        }
        else 
        {
            pContext->dwPipelinedEP = dwEndpoint;
            EnableEndpointInterrupt(pContext, dwEndpoint, peps->dwDirectionAssigned);

            if( pContext->dwUSBClassInfo != USB_MSF)
            {
                if(pContext->bOutEPDMAStartFlag == FALSE)
                {
                    pContext->dwPipelinedXferSize = pTransfer->cbBuffer;
                    pContext->dwPipelinedPktCnt = pContext->dwPipelinedXferSize / peps->dwPacketSizeAssigned;
                    
                    pContext->bOutEPDMAStartFlag = TRUE;

                    EnableEndpointInterrupt(pContext, dwEndpoint, peps->dwDirectionAssigned);
                    WriteEPSpecificReg(dwEndpoint, DOEPDMA, pContext->pPAddrEP[dwEndpoint][pContext->dwPipelinedStrIdx]);
                    WriteEPSpecificReg(dwEndpoint, DOEPTSIZ, pContext->dwPipelinedPktCnt<<PACKET_COUTNT_IDX | pContext->dwPipelinedXferSize);
                    WriteEPSpecificReg(dwEndpoint, DOEPCTL, EP_ENABLE | CLEAR_NAK | SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
                }

                if(pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedEndIdx])
                {
                    RETAILMSG(UFN_ZONE_WARNING,(TEXT("Ring buffer is buffering... Late processing!!!\r\n")));
                    
                    memcpy(pTransfer->pvBuffer, \
                              pContext->pVAddrEP[dwEndpoint][pContext->dwPipelinedEndIdx] , \
                              pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedEndIdx]); 
                    
                    pTransfer->cbTransferred += pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedEndIdx];
                    pContext->dwPipelinedXfered[dwEndpoint][pContext->dwPipelinedEndIdx] = 0;
                    pContext->dwPipelinedEndIdx = GET_NEXT_PF_IDX(pContext->dwPipelinedEndIdx);
                    pContext->dwPipelinedEP = 0;

                    if(pContext->bRingBufferFull)
                    {
                        if(pContext->dwPipelinedStrIdx != GET_PREV_PF_IDX(pContext->dwPipelinedEndIdx))
                        {
                            RETAILMSG(UFN_ZONE_WARNING,(TEXT("ERROR : Ring State is not valud(Str : %d, End : %d)\r\n"), pContext->dwPipelinedStrIdx, GET_PREV_PF_IDX(pContext->dwPipelinedEndIdx)));
                        }
                        
                        pContext->bRingBufferFull = FALSE;
                        EnableEndpointInterrupt(pContext, dwEndpoint, peps->dwDirectionAssigned);
                        WriteEPSpecificReg(dwEndpoint, DOEPDMA, pContext->pPAddrEP[dwEndpoint][pContext->dwPipelinedStrIdx]);
                        WriteEPSpecificReg(dwEndpoint, DOEPTSIZ, pContext->dwPipelinedPktCnt<<PACKET_COUTNT_IDX | pContext->dwPipelinedXferSize);
                        WriteEPSpecificReg(dwEndpoint, DOEPCTL, EP_ENABLE | CLEAR_NAK | SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
                    }
                    
                    CompleteTransfer(pContext, peps, UFN_NO_ERROR);
                }
            }
            else
            {
                DWORD dwPktcnt, dwBytes;
                dwPktcnt = pTransfer->cbBuffer / peps->dwPacketSizeAssigned;
                dwBytes = pTransfer->cbBuffer % peps->dwPacketSizeAssigned;
                if (dwBytes) dwPktcnt++;        

                pContext->dwPipelinedXferSize = pTransfer->cbBuffer;
                EnableEndpointInterrupt(pContext, dwEndpoint, peps->dwDirectionAssigned);
                WriteEPSpecificReg(dwEndpoint, DOEPDMA, pContext->pPAddrEP[dwEndpoint][pContext->dwPipelinedStrIdx]);
                WriteEPSpecificReg(dwEndpoint, DOEPTSIZ, dwPktcnt<<PACKET_COUTNT_IDX | pTransfer->cbBuffer);
                WriteEPSpecificReg(dwEndpoint, DOEPCTL, EP_ENABLE | CLEAR_NAK | SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
            }
        }        
    }

    UNLOCK_ENDPOINT(peps);
    FUNCTION_LEAVE_MSG();
}


DWORD
WINAPI
UfnPdd_IssueTransfer(
    PVOID          pvPddContext,
    DWORD          dwEndpoint,
    PSTransfer     pTransfer
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    DEBUGCHK(EP_VALID(dwEndpoint));
    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);
    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
    DEBUGCHK(peps->bInitialized);
    DEBUGCHK(pTransfer->cbTransferred == 0);

    DWORD dwRet = ERROR_SUCCESS;

    DEBUGCHK(peps->pTransfer == NULL);
    StartTransfer(pContext, peps, pTransfer);

    FUNCTION_LEAVE_MSG();

    return dwRet;
}


DWORD
WINAPI
UfnPdd_AbortTransfer(
    PVOID       pvPddContext,
    DWORD       dwEndpoint,
    PSTransfer    pTransfer
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PREFAST_DEBUGCHK(pTransfer);
    DEBUGCHK(EP_VALID(dwEndpoint));

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
    LOCK_ENDPOINT(peps);
    DEBUGCHK(peps->bInitialized);

    ValidateTransferDirection(pContext, peps, pTransfer);

    DEBUGCHK(pTransfer == peps->pTransfer);
    CompleteTransfer(pContext, peps, UFN_CANCELED_ERROR);

    if (dwEndpoint == 0)
    {
        pContext->Ep0State = EP0_STATE_IDLE;
    }

    ResetEndpoint(pContext,peps);
    
    DisableEndpointInterrupt(pContext, 0, USB_IN_TRANSFER);
    DisableEndpointInterrupt(pContext, 0, USB_OUT_TRANSFER);
    
    UNLOCK_ENDPOINT(peps);

    FUNCTION_LEAVE_MSG();

    return ERROR_SUCCESS;
}


BOOL 
SetOtgDevicePower (
    PCTRLR_PDD_CONTEXT         pContext,
    CEDEVICE_POWER_STATE    cpsNew
    )
{
    if (cpsNew == D0)
    {
        RETAILMSG(UFN_ZONE_POWER,(_T("[UFNPDD] USB_POWER : D0\r\n")));
        InterruptDone(pContext->dwSysIntr);

        pContext->pSYSCONregs->HCLK_GATE |= OTG_HCLK_EN;     //OTG HClk enable
            SetAllOutEpNak();
        ClearSoftDisconnect();
        ResetDevice(pContext);        
            ClearAllOutEpNak();      

        pContext->IsFirstReset = TRUE;
        DWORD dwGOTGCTL = ReadReg(GOTGCTL);
        if(!(dwGOTGCTL & (B_SESSION_VALID)))
        {
            Delay(100);      //for OTG cable detahced state.
        }
    }

    else if (cpsNew == D4)
    {
        RETAILMSG(UFN_ZONE_POWER,(_T("[UFNPDD] USB_POWER : D4\r\n")));
        SetSoftDisconnect();        
        
        pContext->pSYSCONregs->HCLK_GATE &= ~OTG_HCLK_EN;    //OTG HClk disable
    }
    return TRUE;
}


static
CEDEVICE_POWER_STATE
SetPowerState(
    PCTRLR_PDD_CONTEXT      pContext,
    CEDEVICE_POWER_STATE    cpsNew
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();
    
    PREFAST_DEBUGCHK(pContext);
    DEBUGCHK(VALID_DX(cpsNew));
    ValidateContext(pContext);

    // Adjust cpsNew.
    if (cpsNew != pContext->cpsCurrent)
    {
        if (cpsNew == D1 || cpsNew == D2)
        {
            // D1 and D2 are not supported.
            cpsNew = D0;
        }
        else if (pContext->cpsCurrent == D4)
        {
            // D4 can only go to D0.
            cpsNew = D0;
        }
    }

    if (cpsNew != pContext->cpsCurrent)
    {
        DEBUGMSG(UFN_ZONE_POWER, (_T("%s Going from D%u to D%u\r\n"), pszFname, pContext->cpsCurrent, cpsNew));

        if ( (cpsNew < pContext->cpsCurrent) && pContext->hBusAccess )
        {
            SetDevicePowerState(pContext->hBusAccess, cpsNew, NULL);
        }

        switch (cpsNew)
        {
            case D0:
                SetOtgDevicePower (pContext, D0);
                KernelIoControl(IOCTL_HAL_DISABLE_WAKE, &pContext->dwSysIntr, sizeof(pContext->dwSysIntr), NULL, 0, NULL);

                if (pContext->fRunning)
                {
                    // Cause the IST to restart.
                    pContext->fRestartIST = TRUE;
                    SetInterruptEvent(pContext->dwSysIntr);
                }
            break;

            case D3:
                KernelIoControl(IOCTL_HAL_ENABLE_WAKE, &pContext->dwSysIntr, sizeof(pContext->dwSysIntr), NULL, 0, NULL);
            break;

            case D4:
                SetOtgDevicePower (pContext, D4);
                KernelIoControl(IOCTL_HAL_DISABLE_WAKE, &pContext->dwSysIntr, sizeof(pContext->dwSysIntr), NULL, 0, NULL);
            break;
        }

        if ( (cpsNew > pContext->cpsCurrent) && pContext->hBusAccess )
        {
            SetDevicePowerState(pContext->hBusAccess, cpsNew, NULL);
        }
        pContext->cpsCurrent = cpsNew;
    }
    
    FUNCTION_LEAVE_MSG();

    return pContext->cpsCurrent;
}


static
VOID
FreeCtrlrContext(
    PCTRLR_PDD_CONTEXT    pContext
    )
{
    PREFAST_DEBUGCHK(pContext);
    DEBUGCHK(!pContext->hevInterrupt);
    DEBUGCHK(!pContext->hIST);
    DEBUGCHK(!pContext->fRunning);
    pContext->dwSig = GARBAGE_DWORD;

    UnmapRegisterSet(pContext);

    if (pContext->hBusAccess) CloseBusAccessHandle(pContext->hBusAccess);

    if (pContext->dwSysIntr)
    {
        KernelIoControl(IOCTL_HAL_DISABLE_WAKE, &pContext->dwSysIntr, sizeof(pContext->dwSysIntr), NULL, 0, NULL);
    }

    if (pContext->dwIrq != IRQ_UNSPECIFIED)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, &pContext->dwSysIntr, sizeof(DWORD), NULL, 0, NULL);
    }

    DeleteCriticalSection(&pContext->csRegisterAccess);
    LocalFree(pContext);
}


DWORD
WINAPI
UfnPdd_Deinit(
    PVOID    pvPddContext
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    FUNCTION_ENTER_MSG();

    FreeCtrlrContext(pContext);

    FUNCTION_LEAVE_MSG();

    return ERROR_SUCCESS;
}


DWORD
WINAPI
UfnPdd_Start(
    PVOID    pvPddContext
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    UFN_CLIENT_INFO currentDriver; 
    DWORD dwRet;
    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    DEBUGCHK(!pContext->fRunning);

    BOOL fIntInitialized = FALSE;

    RETAILMSG(UFN_ZONE_INIT,(_T("[UFNPDD] USB DRIVER VERSION : %d\r\n"), USB_DD_VERSION));
    // Some application can change the usbfn client dirver dynamically.
    // In this case, below codes are needed.
    if (USBCurrentDriver((PUFN_CLIENT_INFO)&currentDriver)) 
    {
        if(!wcscmp(currentDriver.szName, TEXT("RNDIS")) ||!wcscmp(currentDriver.szName, TEXT("rndis")))
        {
            pContext->dwUSBClassInfo = USB_RNDIS;
            pContext->dwUsingEPCnt = 4;
            RETAILMSG(UFN_ZONE_INIT,(_T("[UFNPDD] USB RNDIS Function Class Enabled\r\n")));
        }
        else if(!wcscmp(currentDriver.szName, TEXT("Serial_Class")) ||!wcscmp(currentDriver.szName, TEXT("serial_class")))
        {
            pContext->dwUSBClassInfo = USB_SERIAL;
            pContext->dwUsingEPCnt = 3;
            RETAILMSG(UFN_ZONE_INIT,(_T("[UFNPDD] USB Serial Function Class Enabled\r\n")));
        }
        else if(!wcscmp(currentDriver.szName, TEXT("Mass_Storage_Class")) ||!wcscmp(currentDriver.szName, TEXT("mass_storage_class")))
        {
            pContext->dwUSBClassInfo = USB_MSF;
            pContext->dwUsingEPCnt = 3;
            RETAILMSG(UFN_ZONE_INIT,(_T("[UFNPDD] USB Mass Storage Function Class Enabled\r\n")));
        }
    } 
    else 
    {
        RETAILMSG(UFN_ZONE_ERROR,(_T("[UFNPDD] USB Function Class Read Error!!!!!!!\r\n")));
    }

    do
    {
        // Create the interrupt event
        pContext->hevInterrupt = CreateEvent(0, FALSE, FALSE, NULL);
        if (pContext->hevInterrupt == NULL)
        {
            dwRet = GetLastError();
            DEBUGMSG(ZONE_ERROR, (_T("%s Error creating  interrupt event. Error = %d\r\n"), pszFname, dwRet));
            break;
        }

        fIntInitialized = InterruptInitialize(pContext->dwSysIntr, pContext->hevInterrupt, NULL, 0);
        if (fIntInitialized == FALSE)
        {
            dwRet = ERROR_GEN_FAILURE;
            DEBUGMSG(ZONE_ERROR, (_T("%s  interrupt initialization failed\r\n"), pszFname));
            break;
        }
        InterruptDone(pContext->dwSysIntr);

        pContext->fExitIST = FALSE;
        pContext->hIST = CreateThread(NULL, 0, ISTMain, pContext, 0, NULL);
        if (pContext->hIST == NULL)
        {
            DEBUGMSG(ZONE_ERROR, (_T("%s IST creation failed\r\n"), pszFname));
            dwRet = GetLastError();
            break;
        }

        InitPDDContext(pContext);
        if(pContext->bMemoryAllocFlag == FALSE)
        {
            for(DWORD i=0; i<pContext->dwUsingEPCnt; i++)
            {
                for(DWORD j=0; j<PIPELINED_FIFO_DEPTH; j++)
                {
                    pContext->pVAddrEP[i][j] = (PDWORD)AllocPhysMem(pContext->dwPipelinedXferSize, PAGE_READWRITE, 0, 0, &pContext->pPAddrEP[i][j]);
                    memset(pContext->pVAddrEP[i][j], 0, pContext->dwPipelinedXferSize);
                    pContext->dwPipelinedXfered[i][j] = 0;
                }
            }
            pContext->bMemoryAllocFlag  = TRUE;
        }
        
        pContext->fRunning = TRUE;
        dwRet = ERROR_SUCCESS;
    }while(FALSE);
    
    if (pContext->fRunning == FALSE)
    {
        DEBUGCHK(dwRet != ERROR_SUCCESS);
        if (fIntInitialized) InterruptDisable(pContext->dwSysIntr);
        if (pContext->hevInterrupt) CloseHandle(pContext->hevInterrupt);
        pContext->hevInterrupt = NULL;
    }

    FUNCTION_LEAVE_MSG();

    return dwRet;
}


// Stop the device.
DWORD
WINAPI
UfnPdd_Stop(
    PVOID    pvPddContext
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    DEBUGCHK(pContext->fRunning);

    // Stop the IST
    pContext->fExitIST = TRUE;
    InterruptDisable(pContext->dwSysIntr);
    SetEvent(pContext->hevInterrupt);
    WaitForSingleObject(pContext->hIST, INFINITE);
    CloseHandle(pContext->hevInterrupt);
    CloseHandle(pContext->hIST);
    pContext->hIST = NULL;
    pContext->hevInterrupt = NULL;

    WriteReg(DAINTMSK,0);                // IN, OUT EP ALL MASK

    WriteReg(DOEPMSK, 0);                // DOEP INT MASK
    WriteReg(DIEPMSK, 0);                // DIEP INT MASK

    WriteReg(GINTMSK, 0);                // GINT MASK
    WriteReg(GAHBCFG, 0);                // GLOBAL INT MASK

    WriteReg(GOTGINT, 0xE0304);            // OTG INT All CLEAR
    WriteReg(GINTSTS, INT_RESUME | INT_EPMIS | INT_SDE | INT_RESET | INT_SUSPEND); // GINT Clear

    for (DWORD dwEpIdx = 0; dwEpIdx < ENDPOINT_COUNT; ++dwEpIdx) 
    {
        EP_STATUS *peps = GetEpStatus(pContext, dwEpIdx);
        ResetEndpoint(pContext, peps);
    }

    if(pContext->bMemoryAllocFlag == TRUE)
    {
        for(DWORD i=0; i<pContext->dwUsingEPCnt; i++)
        {
            for(DWORD j=0; j<PIPELINED_FIFO_DEPTH; j++)
            {
                FreePhysMem(pContext->pVAddrEP[i][j]);
                pContext->pVAddrEP[i][j] = NULL;
                pContext->pPAddrEP[i][j] = 0;
            }
        }
        pContext->bMemoryAllocFlag = FALSE;
    }

    pContext->fRunning = FALSE;

    DEBUGMSG(ZONE_FUNCTION, (_T("%s Device has been stopped\r\n"), pszFname));

    FUNCTION_LEAVE_MSG();

    return ERROR_SUCCESS;
}


DWORD
WINAPI
UfnPdd_IsConfigurationSupportable(
    PVOID                       pvPddContext,
    UFN_BUS_SPEED               Speed,
    PUFN_CONFIGURATION          pConfiguration
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    // This PDD does not have any special requirements that cannot be
    // handled through IsEndpointSupportable.
    DWORD dwRet = ERROR_SUCCESS;

    FUNCTION_LEAVE_MSG();

    return dwRet;
}


// Is this endpoint supportable.
DWORD
WINAPI
UfnPdd_IsEndpointSupportable(
    PVOID                       pvPddContext,
    DWORD                       dwEndpoint,
    UFN_BUS_SPEED               Speed,
    PUSB_ENDPOINT_DESCRIPTOR    pEndpointDesc,
    BYTE                        bConfigurationValue,
    BYTE                        bInterfaceNumber,
    BYTE                        bAlternateSetting
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    DEBUGCHK(EP_VALID(dwEndpoint));
    
    DWORD dwRet = ERROR_SUCCESS;
    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    // Special case for endpoint 0
    if (dwEndpoint == 0)
    {
        DEBUGCHK(pEndpointDesc->bmAttributes == USB_ENDPOINT_TYPE_CONTROL);

        if (pEndpointDesc->wMaxPacketSize < EP_0_PACKET_SIZE)
        {
            DEBUGMSG(ZONE_ERROR, (_T("%s Endpoint 0 only supports %u byte packets\r\n"), pszFname, EP_0_PACKET_SIZE));
            dwRet = ERROR_INVALID_PARAMETER;
        }
        else
        {
            // Larger than EP 0 Max Packet Size - reduce to Max
            pEndpointDesc->wMaxPacketSize = EP_0_PACKET_SIZE;
        }
    }
    else if (dwEndpoint < ENDPOINT_COUNT)
    {
        BYTE bTransferType = pEndpointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
        DEBUGCHK(bTransferType != USB_ENDPOINT_TYPE_CONTROL);

        // Validate and adjust packet size
        DWORD wPacketSize = (pEndpointDesc->wMaxPacketSize & USB_ENDPOINT_MAX_PACKET_SIZE_MASK);

        switch(bTransferType)
        {
            // Isoch not currently supported by Samsung HW
            case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                DEBUGMSG(ZONE_ERROR, (_T("%s Isochronous endpoints are not supported\r\n"), pszFname));
                dwRet = ERROR_INVALID_PARAMETER;
            break;

            case USB_ENDPOINT_TYPE_BULK:
            case USB_ENDPOINT_TYPE_INTERRUPT:
                // HW Can only Support 8, 16, 32, 64, 512 byte packets
                if((wPacketSize >= 8) && (wPacketSize < 16))
                {
                    wPacketSize = 8;
                }
                else if ((wPacketSize >= 16) && (wPacketSize < 32))
                {
                    wPacketSize = 16;
                }
                else if((wPacketSize >= 32) && (wPacketSize < 64))
                {
                    wPacketSize = 32;
                }
                else if ((wPacketSize >= 64) && (wPacketSize < 128))
                {
                    wPacketSize = 64;
                }
                else if((wPacketSize >= 128) && (wPacketSize < 256))
                {
                    wPacketSize = 128;
                }
                else if ((wPacketSize >= 256) && (wPacketSize < 512))
                {
                    wPacketSize = 256;
                }
                else if (wPacketSize >= 512)
                {
                    wPacketSize = 512;
                }
                else        // wPacketSize < 8
                { 
                    dwRet = ERROR_INVALID_PARAMETER;
                }
            break;

            default:
                dwRet = ERROR_INVALID_PARAMETER;
            break;
        }

        // If Requested Size is larger than what is supported ... change it.
        // Note only try and change it if no errors so far... meaning Ep is
        // Supportable.
        if ( (wPacketSize != (pEndpointDesc->wMaxPacketSize & USB_ENDPOINT_MAX_PACKET_SIZE_MASK)) && (dwRet == ERROR_SUCCESS) )
        {
            pEndpointDesc->wMaxPacketSize &= ~USB_ENDPOINT_MAX_PACKET_SIZE_MASK;
            pEndpointDesc->wMaxPacketSize |= wPacketSize;
        }
    }

    FUNCTION_LEAVE_MSG();
    return dwRet;
}


// Clear an endpoint stall.
DWORD
WINAPI
UfnPdd_ClearEndpointStall(
    PVOID         pvPddContext,
    DWORD         dwEndpoint
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    DEBUGCHK(EP_VALID(dwEndpoint));

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
    LOCK_ENDPOINT(peps);
    
    DWORD dwRet = ERROR_SUCCESS;

    if (dwEndpoint == 0)
    {
        WriteReg(DIEPCTL0, EP0_MAX_PK_SIZ);
        WriteReg(DOEPCTL0, EP0_MAX_PK_SIZ);
    }
    else if (peps->dwDirectionAssigned == USB_IN_TRANSFER)
    {
        WriteEPSpecificReg(dwEndpoint, DIEPCTL, SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
    }
    else
    {     
        WriteEPSpecificReg(dwEndpoint, DOEPCTL, SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
    }
  
    DEBUGMSG(ZONE_PIPE,(TEXT("%d EP CLR_STALL\r\n"), dwEndpoint));

    UNLOCK_ENDPOINT(peps);
    FUNCTION_LEAVE_MSG();
    
    return dwRet;
}


// Initialize an endpoint.
DWORD
WINAPI
UfnPdd_InitEndpoint(
    PVOID                             pvPddContext,
    DWORD                           dwEndpoint,
    UFN_BUS_SPEED                   Speed,
    PUSB_ENDPOINT_DESCRIPTOR        pEndpointDesc,
    PVOID                           pvReserved,
    BYTE                            bConfigurationValue,
    BYTE                            bInterfaceNumber,
    BYTE                            bAlternateSetting
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();
    
    DWORD dwRet = ERROR_SUCCESS;

    DEBUGCHK(EP_VALID(dwEndpoint));
    PREFAST_DEBUGCHK(pEndpointDesc);

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    BYTE bEndpointAddress = 0;
    DWORD dwRegTemp = 0;

    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
    DEBUGCHK(!peps->bInitialized);

    InitializeCriticalSection(&peps->cs);

    DWORD wMaxPacketSize = pEndpointDesc->wMaxPacketSize & USB_ENDPOINT_MAX_PACKET_SIZE_MASK;
    DEBUGCHK(wMaxPacketSize);

    peps->bInitialized = TRUE;
    // If the target is endpoint 0, then only allow the function driver
    // to register a notification function.
    if (dwEndpoint == 0)
    {
        peps->dwPacketSizeAssigned = wMaxPacketSize;
        // Interrupts for endpoint 0 are enabled in ISTMain
    }
    else if (dwEndpoint < ENDPOINT_COUNT)
    {
        bEndpointAddress = pEndpointDesc->bEndpointAddress;
        BOOL fModeOut = USB_ENDPOINT_DIRECTION_OUT(bEndpointAddress);
        if (fModeOut)
        {
            peps->dwDirectionAssigned = USB_OUT_TRANSFER;
        }
        else
        {
            peps->dwDirectionAssigned = USB_IN_TRANSFER;
        }

        // Set Transfer Type
        BYTE bTransferType = pEndpointDesc->bmAttributes & USB_ENDPOINT_TYPE_MASK;
        DEBUGCHK(bTransferType != USB_ENDPOINT_TYPE_CONTROL);

        switch(bTransferType)
        {
            case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                dwRegTemp |= SET_TYPE_ISO;
                   // 6410 USBFN driver does not support ISOCHRONOUS Type.
                dwRet = ERROR_INVALID_PARAMETER;    
            break;

            case USB_ENDPOINT_TYPE_INTERRUPT:
                 dwRegTemp |= SET_TYPE_INTERRUPT;
            break;
            
            case USB_ENDPOINT_TYPE_BULK:
            default:
                 dwRegTemp |= SET_TYPE_BULK;
        }
        
        peps->dwEndpointType = bTransferType;
        peps->dwPacketSizeAssigned = wMaxPacketSize;

        if (peps->dwDirectionAssigned == USB_OUT_TRANSFER)
        {
            WriteEPSpecificReg(dwEndpoint, DOEPTSIZ, 1<<PACKET_COUTNT_IDX | peps->dwPacketSizeAssigned);
            WriteEPSpecificReg(dwEndpoint, DOEPCTL, dwRegTemp | EP_ENABLE | SET_D0_PID | CLEAR_NAK | USB_ACT_EP | peps->dwPacketSizeAssigned);
        }
        else
        {
            WriteEPSpecificReg(dwEndpoint, DIEPTSIZ, 1<<MULTI_CNT_IDX | 1<<PACKET_COUTNT_IDX | peps->dwPacketSizeAssigned);
            if (pContext->dwUSBClassInfo == USB_RNDIS)
            {
                if (dwEndpoint == 1)
                    WriteEPSpecificReg(dwEndpoint, DIEPCTL, dwRegTemp | SET_D0_PID | CLEAR_NAK | USB_ACT_EP | 2<<NEXT_EP_IDX | peps->dwPacketSizeAssigned);
                else
                    WriteEPSpecificReg(dwEndpoint, DIEPCTL, dwRegTemp | SET_D0_PID | CLEAR_NAK | USB_ACT_EP | 0<<NEXT_EP_IDX | peps->dwPacketSizeAssigned);
            }
            else
            {
                WriteEPSpecificReg(dwEndpoint, DIEPCTL, dwRegTemp | SET_D0_PID | CLEAR_NAK | USB_ACT_EP |  0<<NEXT_EP_IDX | peps->dwPacketSizeAssigned);
            }

        }
    }
    
    FUNCTION_LEAVE_MSG();
    return dwRet;
}


// Deinitialize an endpoint.
DWORD
WINAPI
UfnPdd_DeinitEndpoint(
    PVOID    pvPddContext,
    DWORD     dwEndpoint
    )
{

    SETFNAME();
    FUNCTION_ENTER_MSG();

    DEBUGCHK(EP_VALID(dwEndpoint));

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
    LOCK_ENDPOINT(peps);

    DEBUGCHK(peps->bInitialized);
    DEBUGCHK(peps->pTransfer == NULL);

    // Reset and disable the endpoint
    // Mask endpoint interrupts
    ResetEndpoint(pContext, peps);

#if 0
    for(DWORD depth=0;depth<PIPELINED_FIFO_DEPTH;depth++)
    {
        FreePhysMem(pContext->pVAddrEP[dwEndpoint][depth]);
        pContext->pVAddrEP[dwEndpoint][depth] = NULL;
        pContext->pPAddrEP[dwEndpoint][depth] = 0;
    }
#endif

    peps->bInitialized = FALSE;
    UNLOCK_ENDPOINT(peps);

    DeleteCriticalSection(&peps->cs);

    FUNCTION_LEAVE_MSG();

    return ERROR_SUCCESS;
}


// Stall an endpoint.
DWORD
WINAPI
UfnPdd_StallEndpoint(
    PVOID    pvPddContext,
    DWORD    dwEndpoint
    )
{
    DWORD dwRet = ERROR_SUCCESS;
    SETFNAME();
    FUNCTION_ENTER_MSG();

    DEBUGCHK(EP_VALID(dwEndpoint));

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
    DEBUGCHK(peps->bInitialized);
    LOCK_ENDPOINT(peps);

    if (dwEndpoint == 0)
    {
        WriteReg(DIEPCTL0, STALL |  EP0_MAX_PK_SIZ);
        WriteReg(DOEPCTL0, STALL | EP0_MAX_PK_SIZ);
        WriteReg(DIEPINT0, 0x3F);
        WriteReg(DOEPINT0, 0x3F);
        pContext->sendDataEnd = FALSE;
        pContext->Ep0State = EP0_STATE_IDLE;
        
    }
    else if (peps->dwDirectionAssigned == USB_IN_TRANSFER)
    {
        WriteEPSpecificReg(dwEndpoint, DIEPCTL, SET_D0_PID | STALL | SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
        WriteEPSpecificReg(dwEndpoint, DIEPINT, 0x3F);     
    }
    else
    {     
        WriteEPSpecificReg(dwEndpoint, DOEPCTL, STALL | SET_TYPE_BULK | USB_ACT_EP | peps->dwPacketSizeAssigned);
        WriteEPSpecificReg(dwEndpoint, DOEPINT, 0x3F);
    }

    DEBUGMSG(ZONE_PIPE,(TEXT("%d EP SET_STALL\r\n"), dwEndpoint));

    UNLOCK_ENDPOINT(peps);
    FUNCTION_LEAVE_MSG();

    return ERROR_SUCCESS;
}


// Send the control status handshake.
DWORD
WINAPI
UfnPdd_SendControlStatusHandshake(
    PVOID   pvPddContext,
    DWORD     dwEndpoint
    )
{

    SETFNAME();
    FUNCTION_ENTER_MSG();

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);    

    DEBUGCHK(dwEndpoint == 0);

    // This function is only valid for Endpoint 0
    EP_STATUS *peps = GetEpStatus(pContext, 0);
    DEBUGCHK(peps->bInitialized);
    LOCK_ENDPOINT(peps);

    pContext->bRdySetupPkt = TRUE;
    DEBUGMSG(ZONE_TRANSFER, (_T("Handshake stage : ")));

    // Enable Endpoint0 interrupt
    EnableEndpointInterrupt(pContext, 0, USB_IN_TRANSFER);
    EnableEndpointInterrupt(pContext, 0, USB_OUT_TRANSFER);        
    
    if(pContext->sendDataEnd)
    {
        //Send zero packet data for control EP handshake.
        WriteReg(DIEPDMA0, pContext->pPAddrEP[0][IN_EP]);
        // Packet Coutnt =>1 , Transfer Size => 0
        WriteReg(DIEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
        // EP0 enable, clear nak, MaxPacket Size => 64Byte
        WriteReg(DIEPCTL0, EP_ENABLE | CLEAR_NAK | 1<<NEXT_EP_IDX | EP0_MAX_PK_SIZ);
        pContext->sendDataEnd = FALSE;
        DEBUGMSG(ZONE_TRANSFER, (_T("Sending 0 packet \r\n")));
    }
    else
    {
        WriteReg(DOEPDMA0, pContext->pPAddrEP[0][OUT_EP]);
        WriteReg(DOEPTSIZ0, 1<<PACKET_COUTNT_IDX | 0);
        WriteReg(DOEPCTL0, EP_ENABLE | CLEAR_NAK | EP0_MAX_PK_SIZ);
        DEBUGMSG(ZONE_TRANSFER, (_T("Getting 0 packet \r\n")));
    }
    
    UNLOCK_ENDPOINT(peps);
    FUNCTION_LEAVE_MSG();
    return ERROR_SUCCESS;
}


// Set the address of the device on the USB.
DWORD
WINAPI
UfnPdd_SetAddress(
    PVOID     pvPddContext,
    BYTE      bAddress
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    if(bAddress > 127)
    {
        RETAILMSG(UFN_ZONE_ERROR,(_T("[UFNPDD] Invalide Address - bAddr:%x\r\n"),bAddress));
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        // Device Address setting to DCFG
        volatile DWORD wDCFG = ReadReg(DCFG);
        WriteReg(DCFG, (wDCFG & ~(DEVICE_ADDRESS_MSK)) | (bAddress<<4));
    }
    
    FUNCTION_LEAVE_MSG();

    return ERROR_SUCCESS;
}


// Check if the Endpoint is stalled.
DWORD
WINAPI
UfnPdd_IsEndpointHalted(
    PVOID     pvPddContext,
    DWORD     dwEndpoint,
    PBOOL     pfHalted
    )
{
SETFNAME();
    FUNCTION_ENTER_MSG();

    DWORD dwRet = ERROR_SUCCESS;

    DWORD dRegVal;
    BOOL fHalted = FALSE;

    DEBUGCHK(EP_VALID(dwEndpoint));
    PREFAST_DEBUGCHK(pfHalted);

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    PEP_STATUS peps = GetEpStatus(pContext, dwEndpoint);
    LOCK_ENDPOINT(peps);

    // Check the Appropriate Stall Bit
    if (dwEndpoint == 0)
    {
        if (peps->dwDirectionAssigned == USB_IN_TRANSFER)
        {
            dRegVal = ReadReg(DIEPCTL0);
            if (dRegVal & STALL)
            {
                fHalted = TRUE;
            }
        }
        else
        {
            dRegVal = ReadReg(DOEPCTL0);
            if (dRegVal & STALL)
            {
                fHalted = TRUE;
            }
        }
    }
    else 
    {
        if (peps->dwDirectionAssigned == USB_IN_TRANSFER)
        {
            dRegVal = ReadEPSpecificReg(dwEndpoint, DIEPCTL) ;
            if (dRegVal & STALL)
            {
                fHalted = TRUE;
            }
        }
        else
        {
            dRegVal = ReadEPSpecificReg(dwEndpoint, DOEPCTL);
            if (dRegVal & STALL)
            {
                fHalted = TRUE;
            }
        }
    }

    DEBUGMSG(ZONE_PIPE,(TEXT("%d EP STALL : %d\r\n"), dwEndpoint, fHalted));

    *pfHalted = fHalted;
    pContext->sendDataEnd  = FALSE;
    UNLOCK_ENDPOINT(peps);

    FUNCTION_LEAVE_MSG();

    return dwRet;
}


DWORD
WINAPI
UfnPdd_InitiateRemoteWakeup(
    PVOID     pvPddContext
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    SetClearReg(DCTL, RMTWKUPSIG, SET);
    Delay(100);    
    SetClearReg(DCTL, RMTWKUPSIG, CLEAR);

    FUNCTION_LEAVE_MSG();

    return ERROR_SUCCESS;
}


DWORD
WINAPI
UfnPdd_RegisterDevice(
    PVOID                           pvPddContext,
    PCUSB_DEVICE_DESCRIPTOR         pHighSpeedDeviceDesc,
    PCUFN_CONFIGURATION             pHighSpeedConfig,
    PCUSB_CONFIGURATION_DESCRIPTOR  pHighSpeedConfigDesc,
    PCUSB_DEVICE_DESCRIPTOR         pFullSpeedDeviceDesc,
    PCUFN_CONFIGURATION             pFullSpeedConfig,
    PCUSB_CONFIGURATION_DESCRIPTOR  pFullSpeedConfigDesc,
    PCUFN_STRING_SET                pStringSets,
    DWORD                           cStringSets
    )
{
    // Nothing to do.
    return ERROR_SUCCESS;
}


DWORD
WINAPI
UfnPdd_DeregisterDevice(
    PVOID   pvPddContext
    )
{
    // Nothing to do.
    return ERROR_SUCCESS;
}


VOID
WINAPI
UfnPdd_PowerDown(
    PVOID     pvPddContext
    )
{
    SETFNAME();
    DEBUGMSG(UFN_ZONE_POWER, (_T("%s\r\n"), pszFname));

    // Nothing to do.
    // UFN power function is implemented in UfnPdd_IOControl function.
}


VOID
WINAPI
UfnPdd_PowerUp(
    PVOID     pvPddContext
    )
{
    SETFNAME();
    DEBUGMSG(UFN_ZONE_POWER, (_T("%s\r\n"), pszFname));

    // Nothing to do.
    // UFN power function is implemented in UfnPdd_IOControl function.
}


DWORD
WINAPI
UfnPdd_IOControl(
    PVOID           pvPddContext,
    IOCTL_SOURCE    source,
    DWORD           dwCode,
    PBYTE           pbIn,
    DWORD           cbIn,
    PBYTE           pbOut,
    DWORD           cbOut,
    PDWORD          pcbActualOut
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();

    PCTRLR_PDD_CONTEXT pContext = (PCTRLR_PDD_CONTEXT) pvPddContext;
    ValidateContext(pContext);

    DWORD dwRet = ERROR_INVALID_PARAMETER;

    switch (dwCode)
    {
        case IOCTL_BUS_GET_POWER_STATE:
            if (source == MDD_IOCTL)
            {
                PREFAST_DEBUGCHK(pbIn);
                DEBUGCHK(cbIn == sizeof(CE_BUS_POWER_STATE));
                PCE_BUS_POWER_STATE pCePowerState = (PCE_BUS_POWER_STATE) pbIn;
                PREFAST_DEBUGCHK(pCePowerState->lpceDevicePowerState);
                DEBUGMSG(UFN_ZONE_POWER, (_T("%s IOCTL_BUS_GET_POWER_STATE\r\n"), pszFname));
                *pCePowerState->lpceDevicePowerState = pContext->cpsCurrent;
                dwRet = ERROR_SUCCESS;
            }
        break;

        case IOCTL_BUS_SET_POWER_STATE:
            if (source == MDD_IOCTL)
            {
                PREFAST_DEBUGCHK(pbIn);
                DEBUGCHK(cbIn == sizeof(CE_BUS_POWER_STATE));
                PCE_BUS_POWER_STATE pCePowerState = (PCE_BUS_POWER_STATE) pbIn;
                PREFAST_DEBUGCHK(pCePowerState->lpceDevicePowerState);
                DEBUGCHK(VALID_DX(*pCePowerState->lpceDevicePowerState));
                DEBUGMSG(UFN_ZONE_POWER, (_T("%s IOCTL_BUS_SET_POWER_STATE(D%u)\r\n"), pszFname, *pCePowerState->lpceDevicePowerState));
                SetPowerState(pContext, *pCePowerState->lpceDevicePowerState);
                dwRet = ERROR_SUCCESS;
            }
        break;
    }
    FUNCTION_LEAVE_MSG();
    return dwRet;
}


// Initialize the device.
DWORD
WINAPI
UfnPdd_Init(
    LPCTSTR                     pszActiveKey,
    PVOID                       pvMddContext,
    PUFN_MDD_INTERFACE_INFO     pMddInterfaceInfo,
    PUFN_PDD_INTERFACE_INFO     pPddInterfaceInfo
    )
{
    SETFNAME();
    FUNCTION_ENTER_MSG();
    
    static const UFN_PDD_INTERFACE_INFO sc_PddInterfaceInfo =
    {
        UFN_PDD_INTERFACE_VERSION,
        (UFN_PDD_CAPS_SUPPORTS_FULL_SPEED | UFN_PDD_CAPS_SUPPORTS_HIGH_SPEED),
        ENDPOINT_COUNT,
        NULL, // This gets filled in later

        &UfnPdd_Deinit,
        &UfnPdd_IsConfigurationSupportable,
        &UfnPdd_IsEndpointSupportable,
        &UfnPdd_InitEndpoint,
        &UfnPdd_RegisterDevice,
        &UfnPdd_DeregisterDevice,
        &UfnPdd_Start,
        &UfnPdd_Stop,
        &UfnPdd_IssueTransfer,
        &UfnPdd_AbortTransfer,
        &UfnPdd_DeinitEndpoint,
        &UfnPdd_StallEndpoint,
        &UfnPdd_ClearEndpointStall,
        &UfnPdd_SendControlStatusHandshake,
        &UfnPdd_SetAddress,
        &UfnPdd_IsEndpointHalted,
        &UfnPdd_InitiateRemoteWakeup,
        &UfnPdd_PowerDown,
        &UfnPdd_PowerUp,
        &UfnPdd_IOControl,
    };

    DWORD dwType;
    DWORD dwRet;

    HKEY hkDevice = NULL;
    HKEY hKey = NULL;
    PCTRLR_PDD_CONTEXT pContext = NULL;

    DEBUGCHK(pszActiveKey);
    DEBUGCHK(pMddInterfaceInfo);
    DEBUGCHK(pPddInterfaceInfo);
    hkDevice = OpenDeviceKey(pszActiveKey);

    if (!hkDevice)
    {
        dwRet = GetLastError();
        DEBUGMSG(ZONE_ERROR, (_T("%s Could not open device key. Error: %d\r\n"), pszFname, dwRet));
        goto EXIT;
    }

    pContext = (PCTRLR_PDD_CONTEXT) LocalAlloc(LPTR, sizeof(*pContext));
    if (pContext == NULL)
    {
        dwRet = GetLastError();
        DEBUGMSG(ZONE_ERROR, (_T("%s LocalAlloc failed. Error: %d\r\n"), pszFname, dwRet));
        goto EXIT;
    }

    g_pUDCBase = NULL;

    pContext->dwSig = SC6410_SIG;

    pContext->pvMddContext = pvMddContext;
    pContext->cpsCurrent = D4;
    pContext->dwIrq = IRQ_UNSPECIFIED;
    pContext->pfnNotify = pMddInterfaceInfo->pfnNotify;
    pContext->dwDetectedSpeed = USB_FULL;
    pContext->IsFirstReset = TRUE;
    pContext->dwUSBClassInfo = USB_RNDIS; // RNDIS is default class
    pContext->bMemoryAllocFlag = FALSE;
    
    InitializeCriticalSection(&pContext->csRegisterAccess);

    for (DWORD dwEp = 0; dwEp < ENDPOINT_COUNT ; ++dwEp)
    {
        pContext->rgEpStatus[dwEp].dwEndpointNumber = dwEp;
    }

    DWORD dwDataSize;
    DWORD dwPriority;

    DDKISRINFO dii;

    // get ISR configuration information
    dii.cbSize = sizeof(dii);
    dwRet = DDKReg_GetIsrInfo(hkDevice, &dii);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s DDKReg_GetIsrInfo() failed %d\r\n"), pszFname, dwRet));
        goto EXIT;
    }
    else if( (dii.dwSysintr == SYSINTR_NOP) && (dii.dwIrq == IRQ_UNSPECIFIED) )
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s no IRQ or SYSINTR value specified\r\n"), pszFname));
        dwRet = ERROR_INVALID_DATA;
        goto EXIT;
    }
    else
    {
        if (dii.dwSysintr == SYSINTR_NOP)
        {
            RETAILMSG(UFN_ZONE_INIT, (_T("[UFNPDD] dii.dwIrq = %d\n\r"), dii.dwIrq));
            BOOL fSuccess = KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &dii.dwIrq,
                sizeof(DWORD), &dii.dwSysintr, sizeof(DWORD), NULL);
            if (!fSuccess)
            {
                DEBUGMSG(ZONE_ERROR, (_T("%s IOCTL_HAL_REQUEST_SYSINTR failed!\r\n"), pszFname));
                goto EXIT;
            }
            pContext->dwIrq = dii.dwIrq;
            pContext->dwSysIntr = dii.dwSysintr;
        }
        else
        {
            pContext->dwSysIntr = dii.dwSysintr;
        }
    }

    // Read the IST priority
    dwDataSize = sizeof(dwPriority);
    dwRet = RegQueryValueEx(hkDevice, UDC_REG_PRIORITY_VAL, NULL, &dwType, (LPBYTE) &dwPriority, &dwDataSize);
    if (dwRet != ERROR_SUCCESS)
    {
        dwPriority = DEFAULT_PRIORITY;
    }
    DEBUGMSG(UFN_ZONE_INIT, (_T("%s Using IST priority %u\r\n"), pszFname, dwPriority));
    pContext->dwISTPriority = dwPriority;
    
    pContext->hBusAccess = CreateBusAccessHandle(pszActiveKey);
    if (pContext->hBusAccess == NULL)
    {
        // This is not a failure.
        DEBUGMSG(ZONE_WARNING, (_T("%s Could not create bus access handle\r\n"), pszFname));
    }

    // map register space to virtual memory
    dwRet = MapRegisterSet(pContext);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (_T("%s failed to map register space\r\n"), pszFname));
        goto EXIT;
    }

    //OTG HCLK enable
    pContext->pSYSCONregs->HCLK_GATE |= OTG_HCLK_EN;
    
    WriteReg(GRSTCTL, TXFFLSH | RXFFLSH | INTKNQFLSH | FRMCNTRRST | HSFTRST | CSFTRST);
    SetSoftDisconnect();
    
    pContext->attachedState = UFN_DETACH;

    memcpy(pPddInterfaceInfo, &sc_PddInterfaceInfo, sizeof(sc_PddInterfaceInfo));
    pPddInterfaceInfo->pvPddContext = pContext;

EXIT:
    if (hkDevice) RegCloseKey(hkDevice);

    if (dwRet != ERROR_SUCCESS && pContext)
    {
        FreeCtrlrContext(pContext);
    }
    FUNCTION_LEAVE_MSG();

    return dwRet;
}


// Called by MDD's DllEntry.
extern "C"
BOOL
UfnPdd_DllEntry(
    HANDLE     hDllHandle,
    DWORD      dwReason,
    LPVOID     lpReserved
    )
{
    SETFNAME();

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        break;
    }

    return TRUE;
}


