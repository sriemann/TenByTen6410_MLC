//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#include <windows.h>
#include <usb200.h>
#include <usbtypes.h>
#include <oal.h>
#include <bsp.h>
#include <s3c6410_syscon.h>
#include <usbdbgddsi.h>
#include <usbdbgutils.h>
#include "Otgdev.h"

//=====================================================================
// global varibles used in several functions

OTGDEV oOtgDev;
ENDPT_INFO EndPointsInfo[MAX_ENDPTS];
DWORD g_NumEPsUsed = 0;

// Local copy of the device descriptor
USBDBG_DEVICE_DESCRIPTOR g_usbDeviceDescriptor;
USBDBG_DEVICE_DESCRIPTOR *g_pusbDeviceDescriptor = NULL;

DWORD EP_FIFO[MAX_ENDPTS];

DWORD DIEPCTL[MAX_ENDPTS];
DWORD DIEPINT[MAX_ENDPTS];
DWORD DIEPTSIZ[MAX_ENDPTS];
DWORD DIEPDMA[MAX_ENDPTS];

DWORD DOEPCTL[MAX_ENDPTS];
DWORD DOEPINT[MAX_ENDPTS];
DWORD DOEPTSIZ[MAX_ENDPTS];
DWORD DOEPDMA[MAX_ENDPTS];


//////////
// Function Name : OTGDevice_StallHelper
// Function Desctiption : This function stalls for a specified period of time
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_StallHelper(int count)
{
    volatile int j,i;
    for(j = 0; j < count; j++)
        for(i=0;i<1000;i++);
}

//////////
// Function Name : OTGDevice_InitOtg
// Function Desctiption : This function initializes OTG PHY and LINK.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_Init()
{
    UINT8 ucMode;
    volatile S3C6410_SYSCON_REG *g_pSysConReg;

    USBDBGMSG(USBDBG_ZONE_FUNC, (L"usbpdd: +OTGDevice_Init\r\n"));

    g_pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    // Enable Clock Source
    g_pSysConReg->HCLK_GATE |= (1<<20);     // HCLK_USB

#ifdef OTGDEVICE_HIGH_SPEED
    USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Init: Running as High Speed device by default\r\n"));
    oOtgDev.m_eSpeed = USB_HIGH;
    oOtgDev.m_uControlEPMaxPktSize = HIGH_SPEED_CONTROL_PKT_SIZE;
    oOtgDev.m_uBulkInEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;
    oOtgDev.m_uBulkOutEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;    
#else
    USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Init: Running as Full Speed device by default\r\n"));
    oOtgDev.m_eSpeed = USB_FULL;
    oOtgDev.m_uControlEPMaxPktSize = FULL_SPEED_CONTROL_PKT_SIZE;
    oOtgDev.m_uBulkInEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;
    oOtgDev.m_uBulkOutEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;    
#endif
    oOtgDev.m_uIsUsbOtgSetConfiguration = 0;

    OTGDevice_SetSoftDisconnect();
    OTGDevice_SoftResetCore();
    OTGDevice_InitCore();
    OTGDevice_CheckCurrentMode(&ucMode);
    if (ucMode == INT_DEV_MODE)
    {
        OTGDevice_StallHelper(4000);
        OTGDevice_InitDevice();
    }
    else
    {
        USBDBGMSG(USBDBG_ZONE_ERROR, (L"usbpdd: Error! OTGDevice_Init: Current Mode is Host\r\n"));
        return;
    }
}


//////////
// Function Name : OTGDevice_InitPhyCon
// Function Desctiption : This function initializes OTG Phy.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_InitPhyCon(void)
{
    OUTREG32(PHYPWR, 0x0);
    OUTREG32(PHYCTRL, 0x20);
    OUTREG32(RSTCON, 0x1);
    OTGDevice_StallHelper(100);
    OUTREG32(RSTCON, 0x0);
    OTGDevice_StallHelper(100);
}


//////////
// Function Name : OTGDevice_SoftResetCore
// Function Desctiption : This function soft-resets OTG Core and then unresets again.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_SoftResetCore(void)
{
    volatile UINT32 uTemp;
    OUTREG32(GRSTCTL, CORE_SOFT_RESET);
    OTGDevice_StallHelper(100);
    do
    {
        uTemp = INREG32(GRSTCTL);
    } while(!(uTemp & AHB_MASTER_IDLE));   
}



//////////
// Function Name : OTGDevice_InitCore
// Function Desctiption : This function initializes OTG Link Core.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_InitCore(void)
{
    OUTREG32(GAHBCFG, MODE_SLAVE|BURST_SINGLE|GBL_INT_UNMASK);

    OUTREG32(GUSBCFG, 0<<15           // PHY Low Power Clock sel
                    |1<<14          // Non-Periodic TxFIFO Rewind Enable
                    |0x5<<10        // Turnaround time
                    |0<<9|0<<8      // [0:HNP disable, 1:HNP enable][ 0:SRP disable, 1:SRP enable] H1= 1,1
                    |0<<7           // Ulpi DDR sel
                    |0<<6           // 0: high speed utmi+, 1: full speed serial
                    |0<<4           // 0: utmi+, 1:ulpi
                    |1<<3           // phy i/f  0:8bit, 1:16bit
                    |0x7<<0         // HS/FS Timeout*
                    );
}


//////////
// Function Name : OTGDevice_CheckCurrentMode
// Function Desctiption : This function checks the current mode.
// Input : pucMode, current mode(device or host)
// Output : NONE
// Version :
void OTGDevice_CheckCurrentMode(UINT8 *pucMode)
{
    *pucMode = (UINT8)(INREG32(GINTSTS)) & 0x1;
}


//////////
// Function Name : OTGDevice_SetSoftDisconnect
// Function Desctiption : This function puts the OTG device core in the disconnected state.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_SetSoftDisconnect(void)
{
    SETREG32(DCTL, SOFT_DISCONNECT);
}


//////////
// Function Name : OTGDevice_ClearSoftDisconnect
// Function Desctiption : This function makes the OTG device core to exit from the disconnected state.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_ClearSoftDisconnect(void)
{
    CLRREG32(DCTL, SOFT_DISCONNECT);
}


//////////
// Function Name : OTGDevice_InitDevice
// Function Desctiption : This function configures OTG Core to initial settings of device mode.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_InitDevice(void)
{
    OUTREG32(DCFG, 1<<18|oOtgDev.m_eSpeed<<0);  // [][1: full speed(30Mhz) 0:high speed]
    OUTREG32(GINTMSK, OTGINTMASK);              //gint unmask
}


//////////
// Function Name : OTGDevice_SetAddress
// Function Desctiption : This function sets the USB address of the device
// Input : address - sent by the USB host through the Set_Address command
// Output : NONE
// Version :
void OTGDevice_SetAddress(USHORT address)
{
    USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"UsbDbg: Setting device address to %d.\r\n", address));
    OUTREG32(DCFG, 1<<18 | ((UINT8)address)<<4 | oOtgDev.m_eSpeed<<0);
}


//////////
// Function Name : OTGDevice_InitEndPts
// Function Desctiption : This function configures the endpoints based on the Device Descriptor.
// Input : USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc
// Output : NONE
//
BOOL OTGDevice_InitEndPts(USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc)
{
    UINT32 i,j,k;
    DWORD EpIndex = 0;
    DWORD epNum;
    USBDBG_CONFIG_DESCRIPTOR* pConfigDesc;
    USBDBG_INTERFACE_DESCRIPTOR* pUsbDbg_IntrfcDesc;
    USB_ENDPOINT_DESCRIPTOR* pEndPtDesc;

    // Save a local copy of the device descriptor passed by MDD
    if (g_pusbDeviceDescriptor == NULL)
    {
        g_pusbDeviceDescriptor = &g_usbDeviceDescriptor;
        memcpy(g_pusbDeviceDescriptor, pDeviceDesc, sizeof(USBDBG_DEVICE_DESCRIPTOR));
    }
    
    // Configure Endpoint register addresses
    for (i=0; i<MAX_ENDPTS; i++)
    {
        EP_FIFO[i] =  (EP0_FIFO + (0x1000*i));

        DIEPCTL[i] =  (DIEPCTL0 + (0x20*i));
        DIEPINT[i] =  (DIEPINT0 + (0x20*i));
        DIEPTSIZ[i] =  (DIEPTSIZ0 + (0x20*i));
        DIEPDMA[i] =  (DIEPDMA0 + (0x20*i));

        DOEPCTL[i] =  (DOEPCTL0 + (0x20*i));
        DOEPINT[i] =  (DOEPINT0 + (0x20*i)); 
        DOEPTSIZ[i] =  (DOEPTSIZ0 + (0x20*i));
        DOEPDMA[i] =  (DOEPDMA0 + (0x20*i)); 
    }

    // Configure EP0
    OUTREG32(DIEPINT0, 0xff);
    OUTREG32(DOEPINT0, 0xff);

    OUTREG32(DIEPTSIZ0, (1<<19)|oOtgDev.m_uControlEPMaxPktSize); // packet count = 1, xfr size
    OUTREG32(DOEPTSIZ0, (1<<29)|(1<<19)|oOtgDev.m_uControlEPMaxPktSize); // setup packet count = 1, packet count = 1, xfr size

    OUTREG32(DIEPCTL0, DEPCTL_EPENA |DEPCTL_CNAK |DEPCTL0_MPS_64);   //MPS:64bytes
    
    OUTREG32(DAINTMSK, ((1<<0)<<16) | (1<<0));
    OUTREG32(DOEPCTL0, DEPCTL_EPENA |DEPCTL_CNAK);     //ep0 enable, clear nak
    g_NumEPsUsed = 1;

    memset(EndPointsInfo, 0, sizeof(EndPointsInfo));
        
    // iterate all configurations and configure rest of the endpoints
    for (i=0;
        i < (UINT32)pDeviceDesc->pUsbDeviceDescriptor->bNumConfigurations;
        i++)
    {
        pConfigDesc = pDeviceDesc->ppUsbConfigDescriptors[i];
        
        //iterate all interfaces
        for (j=0;
            j < (UINT32)pConfigDesc->pUsbConfigDescriptor->bNumInterfaces;
            j++)
        {
            pUsbDbg_IntrfcDesc = pConfigDesc->ppUsbInterfaceDescriptors[j];

            // iterate all endpoints and save the endpoint data
            for (k=0;
                k < (UINT32)pUsbDbg_IntrfcDesc->pUsbInterfaceDescriptor->bNumEndpoints;
                k++)
            {
                pEndPtDesc = pUsbDbg_IntrfcDesc->ppUsbEndPtDescriptors[k];
                
                epNum = ENDPTNUM(pEndPtDesc->bEndpointAddress);
                if (epNum >= MAX_ENDPTS)
                {
                    USBDBGMSG(USBDBG_ZONE_ERROR, (L"usbpdd: InitEndPts EpNum (%d) > Max \r\n", epNum));
                }
                else
                {
                    EndPointsInfo[epNum].EndPtAddress = pEndPtDesc->bEndpointAddress;
                    EndPointsInfo[epNum].EndPtAttributes = pEndPtDesc->bmAttributes;
                    g_NumEPsUsed++;
                }
            }
        }
    }

    // Configure the endpoints using saved data
    OTGDevice_InitNonControlEndPts();
    
    return TRUE;
}


//////////
// Function Name : OTGDevice_InitNonControlEndPts
// Function Desctiption : This function configures the non-control endpoints 
//                      based on the data saved in OTGDevice_InitEndPts.
// Input : USBDBG_DEVICE_DESCRIPTOR* pDeviceDesc
// Output : NONE
//
BOOL OTGDevice_InitNonControlEndPts()
{
    UINT32 i;
    DWORD epNum, cfg;
    BYTE bEndPtAddress;
    BYTE bEndPtAttributes;
    
    // iterate all endpoints and initialize
    for (i=0; i < MAX_ENDPTS; i++)
    {
        if (EndPointsInfo[i].EndPtAddress != 0)
        {
            bEndPtAddress = EndPointsInfo[i].EndPtAddress;
            bEndPtAttributes = EndPointsInfo[i].EndPtAttributes;

            // Create EP config
            if (bEndPtAttributes == 2)
            {
                cfg  = DEPCTL_CNAK | DEPCTL_BULK_TYPE | DEPCTL_USBACTEP;
            }
            else if (bEndPtAttributes == 3)
            {
                cfg  = DEPCTL_CNAK | DEPCTL_INTR_TYPE | DEPCTL_USBACTEP;
            }

            // Get EP address
            epNum = ENDPTNUM(bEndPtAddress);

            // RX or TX?
            if (IS_ENDPT_RX(bEndPtAddress))
            {
                UINT32 Reg = DOEPCTL[epNum];

                cfg |= oOtgDev.m_uBulkOutEPMaxPktSize;
                OUTREG32(Reg,cfg);

                SETREG32(DAINTMSK,(1<<epNum)<<16);                    
                
                // set RX config (OUT Endpoint)
                OUTREG32(DOEPTSIZ[epNum],(1<<19)|oOtgDev.m_uBulkOutEPMaxPktSize);

                USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                    L"usbpdd: InitEndPoints: setting EP-%d as OUT Ep, Attr = %d\r\n", 
                    epNum, bEndPtAttributes));
            }
            else
            {
                UINT32 Reg = DIEPCTL[epNum];
                
                cfg |= oOtgDev.m_uBulkInEPMaxPktSize;
                OUTREG32(Reg,cfg);    

                SETREG32(DAINTMSK,(1<<epNum));

                // set TX config (IN Endpoint)
                OUTREG32(DIEPTSIZ[epNum],(1<<19)|oOtgDev.m_uBulkInEPMaxPktSize);

                USBDBGMSG(USBDBG_ZONE_VERBOSE, (
                    L"usbpdd: InitEndPoints: setting EP-%d as IN Ep, Attr = %d\r\n", 
                    epNum, bEndPtAttributes));
            }
        }
    }
    
    return TRUE;
}



//////////
// Function Name : OTGDEV_SetOutEpXferSize
// Function Desctiption : This function sets DOEPTSIZn CSR according to input parameters.
// Input : eType, transfer type
//          uPktCnt, packet count to transfer
// Output : NONE
// Version :
void OTGDevice_SetOutEpXferSize(UINT32 epNum, UINT32 uPktCnt)
{
    if(epNum == 0)
    {
        OUTREG32(DOEPTSIZ0, (1<<29)|(uPktCnt<<19)|(oOtgDev.m_uControlEPMaxPktSize<<0));
    }
    else
    {
        OUTREG32(DOEPTSIZ[epNum], (uPktCnt<<19)|(oOtgDev.m_uBulkOutEPMaxPktSize<<0));
    }
}


//////////
// Function Name : OTGDevice_DeInit
// Function Desctiption : This function de-initializes OTG PHY and LINK.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_DeInit()
{
    DWORD epNum;

    // Clear USB Interrupt enable registers
    OUTREG32(GINTMSK, 0); 

    // Disable all RX, TX EPs
    if (INREG32(DIEPCTL0) & DEPCTL_EPENA)
        OUTREG32(DIEPCTL0, DEPCTL_EPDIS);

    if (INREG32(DOEPCTL0) & DEPCTL_EPENA)
        OUTREG32(DOEPCTL0, DEPCTL_EPDIS);
    
    for (epNum = 1; epNum < MAX_ENDPTS; epNum++)
    {
        if (INREG32(DIEPCTL[epNum]) & DEPCTL_EPENA)
            OUTREG32(DIEPCTL[epNum], DEPCTL_EPDIS);

        if (INREG32(DOEPCTL[epNum]) & DEPCTL_EPENA)
            OUTREG32(DOEPCTL[epNum], DEPCTL_EPDIS);
    }

    SETREG32(PCGCCTL, (1<<0));   //stop pclk
}


//////////
// Function Name : OTGDevice_HandleReset
// Function Desctiption : This function handles a USB RESET.
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_HandleReset()
{
    OUTREG32(GAHBCFG, MODE_SLAVE|BURST_SINGLE|GBL_INT_UNMASK);
    
    OUTREG32(GINTMSK, OTGINTMASK); // Global Interrupt Mask
    OUTREG32(DOEPMSK, CTRL_OUT_EP_SETUP_PHASE_DONE|AHB_ERROR|TRANSFER_DONE); // Out EP Int Mask
    OUTREG32(DIEPMSK, INTKN_TXFEMP|NON_ISO_IN_EP_TIMEOUT|AHB_ERROR|TRANSFER_DONE); // In EP Int Mask


    OUTREG32(GRXFSIZ, RX_FIFO_SIZE);      // Rx FIFO Size
    OUTREG32(GNPTXFSIZ, NPTX_FIFO_SIZE<<16| NPTX_FIFO_START_ADDR<<0); // Non Periodic Tx FIFO Size
    OUTREG32(GRSTCTL, (1<<5)|(1<<4));     // TX and RX FIFO Flush
    
    CLRREG32(PCGCCTL, (1<<0));   //start PHY clock
}


//////////
// Function Name : OTGDevice_HandleEnumDone
// Function Desctiption : This function handles the enumeration done interrupt from the core
// Input : NONE
// Output : NONE
// Version :
void OTGDevice_HandleEnumDone()
{
    oOtgDev.m_eSpeed = (INREG32(DSTS) & 0x6) >> 1;
    if (oOtgDev.m_eSpeed == USB_HIGH)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Enumerated at High Speed\r\n"));
        oOtgDev.m_uControlEPMaxPktSize = HIGH_SPEED_CONTROL_PKT_SIZE;
        oOtgDev.m_uBulkInEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;
        oOtgDev.m_uBulkOutEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;  
    }
    else if (oOtgDev.m_eSpeed == USB_FULL)
    {
        USBDBGMSG(USBDBG_ZONE_VERBOSE, (L"usbpdd: Enumerated at Full Speed\r\n"));
        oOtgDev.m_uControlEPMaxPktSize = FULL_SPEED_CONTROL_PKT_SIZE;
        oOtgDev.m_uBulkInEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;
        oOtgDev.m_uBulkOutEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;  
    }
    else
    {
        USBDBGMSG(USBDBG_ZONE_WARN, (L"usbpdd: Enumerated Speed not high or full. Speed = %d\r\n", oOtgDev.m_eSpeed));
    }

    //Re-init non control end points
    OTGDevice_InitNonControlEndPts();
}


