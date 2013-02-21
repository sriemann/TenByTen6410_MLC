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
#include <halether.h>
#include <bsp.h>
#include <specstrings.h>
#include "loader.h"
#include "usb.h"

// start // from otg_dev.c

static void VIC_InterruptEnable(UINT32 intNum);
static void VIC_InterruptDisable(UINT32 intNum);
static void VIC_ClearVectAddr(void);
static void VIC_InitializeVectTable(void);
static void delayLoop(int count);

static DWORD transferComplete;

//=====================================================================
// setting the device qualifier descriptor and a string descriptor
const UINT8 aDeviceQualifierDescriptor[] =
{
    0x0a,                   //  0 desc size
    0x06,                   //  1 desc type (DEVICE)
    0x00,                   //  2 USB release
    0x02,                   //  3 => 2.00
    0x00,                   //  4 class
    0x00,                   //  5 subclass
    0x00,                   //  6 protocol
    64,                      //  7 max pack size
    0x01,                   //  8 number of other-speed configuration
    0x00,                   //  9 reserved
};

const UINT8 aDescStr0[]=
{
    4, STRING_DESCRIPTOR, LANGID_US_L, LANGID_US_H, // codes representing languages
};

const UINT8 aDescStr1[]= // Manufacturer
{
    (0x14+2), STRING_DESCRIPTOR,
    'S', 0x0, 'y', 0x0, 's', 0x0, 't', 0x0, 'e', 0x0, 'm', 0x0, ' ', 0x0, 'M', 0x0,
    'C', 0x0, 'U', 0x0,
};

const UINT8 aDescStr2[]= // Product
{
    (0x2a+2), STRING_DESCRIPTOR,
    'S', 0x0, 'E', 0x0, 'C', 0x0, ' ', 0x0, 'S', 0x0, '3', 0x0, 'C', 0x0, '6', 0x0,
    '4', 0x0, '1', 0x0, '0', 0x0, 'X', 0x0, ' ', 0x0, 'T', 0x0, 'e', 0x0, 's', 0x0,
    't', 0x0, ' ', 0x0, 'B', 0x0, '/', 0x0, 'D', 0x0
};


//=====================================================================
// global varibles used in several functions
OTGDEV oOtgDev;
USB_GET_STATUS oStatusGet;
USB_INTERFACE_GET oInterfaceGet;

UINT16 g_usConfig;
bool g_bTransferEp0 = false;

// end // from otg_dev.c


//static UINT32                    g_uDownloadFileSize;
//static UINT32                      g_uDownloadAddress = DMABUFFER;
volatile UINT32                 readPtIndex;
volatile UINT8                    *g_pDownPt;

//volatile S3C6410_USBOTG_LINK_REG *pUSBOTG_LINK;
//volatile S3C6410_USBOTG_PHY_REG *pUSBOTG_PHY;

volatile S3C6410_VIC_REG *s6410VIC0;
volatile S3C6410_VIC_REG *s6410VIC1;


void Isr_Init(void);
void C_IsrHandler(unsigned int val);
extern void ASM_IsrHandler(void);
void SetEndpoint(void);


// start // from otg_dev.c


//////////
// Function Name : OTGDEV_InitOtg
// Function Desctiption : This function initializes OTG PHY and LINK.
// Input : eSpeed, USB Speed (high or full)
// Output : NONE
// Version :
void OTGDEV_InitOtg(USB_SPEED eSpeed)
{
    UINT8 ucMode;
    volatile S3C6410_SYSCON_REG *pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);    
    

    volatile UINT32 uTemp;
    uTemp = Inp32SYSC(0x900);
    Outp32SYSC(0x900, uTemp|(1<<16)); // unmask usb signal  // For NAND Boot

    pSysConReg->HCLK_GATE |= (1<<20);

    oOtgDev.m_eSpeed = eSpeed;
    oOtgDev.m_uIsUsbOtgSetConfiguration = 0;
    oOtgDev.m_uEp0State = EP0_STATE_INIT;
    oOtgDev.m_uEp0SubState = 0;
    
    OTGDEV_InitPhyCon();
    
    OTGDEV_SoftResetCore();
    OTGDEV_InitCore();
    OTGDEV_CheckCurrentMode(&ucMode);
    if (ucMode == INT_DEV_MODE)
    {
        delayLoop(10000);
        OTGDEV_ClearSoftDisconnect();
        OTGDEV_InitDevice();
    }
    else
    {
        EdbgOutputDebugString("Error : Current Mode is Host\n");
        return;
    }
    oOtgDev.m_eSpeed = USB_HIGH;
    oOtgDev.m_uControlEPMaxPktSize = HIGH_SPEED_CONTROL_PKT_SIZE;
    oOtgDev.m_uBulkInEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;
    oOtgDev.m_uBulkOutEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;

}



//////////
// Function Name : OTGDEV_HandleEvent
// Function Desctiption : This function handles various OTG interrupts of Device mode.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_HandleEvent(void)
{
    UINT32 uGIntStatus, uDStatus;
    UINT32 ep_int_status, ep_int;
    UINT32 uPcgctl;
    UINT32 uGrstctl;

    Inp32(GINTSTS, uGIntStatus); // System status read
    Outp32(GINTSTS, uGIntStatus); // Interrupt Clear

    if (uGIntStatus & INT_RESET) // Reset interrupt
    {
        OTGDEV_SetAllOutEpNak();
        oOtgDev.m_uEp0State = EP0_STATE_INIT;
        Outp32(DAINTMSK,((1<<BULK_OUT_EP)|(1<<CONTROL_EP))<<16|((1<<BULK_IN_EP)|(1<<CONTROL_EP)));
        Outp32(DOEPMSK, CTRL_OUT_EP_SETUP_PHASE_DONE|AHB_ERROR|TRANSFER_DONE);
        Outp32(DIEPMSK, INTKN_TXFEMP|NON_ISO_IN_EP_TIMEOUT|AHB_ERROR|TRANSFER_DONE);
        Outp32(GRXFSIZ, RX_FIFO_SIZE);                    // Rx FIFO Size
        Outp32(GNPTXFSIZ, NPTX_FIFO_SIZE<<16| NPTX_FIFO_START_ADDR<<0);    // Non Periodic Tx FIFO Size
        Outp32(GRSTCTL, (1<<5)|(1<<4));      // TX and RX FIFO Flush
        OTGDEV_SetOpMode(USB_CPU);

        OTGDEV_ClearAllOutEpNak();
        Inp32(PCGCCTL,uPcgctl);
        Outp32(PCGCCTL, uPcgctl&~(1<<0));    //start pclk

        DBGUSB("\n [USB_Diag_Log]  : Reset Mode\n");
    }

    if (uGIntStatus & INT_ENUMDONE) // Device Speed Detection interrupt
    {
        DBGUSB("\n [USB_Diag_Log]  : Speed Detection interrupt \n");

        Inp32(DSTS, uDStatus); // System status read

        if (((uDStatus&0x6) >>1) == USB_HIGH)             // Set if Device is High speed or Full speed
        {
            DBGUSB("\n [USB_Diag_Log]  : High Speed Detection\n");
            OTGDEV_SetMaxPktSizes(USB_HIGH);
        }
        else if(((uDStatus&0x6) >>1) == USB_FULL)
        {
            DBGUSB("\n [USB_Diag_Log]  : full Speed Detection\n");
            OTGDEV_SetMaxPktSizes(USB_FULL);
        }
        else
        {
            DBGUSB("\n [USB_Diag_Log]  : No specific speed option\n"); //Assert(0);
            while(1);
        }

        OTGDEV_SetEndpoint();
        OTGDEV_SetDescriptorTable();
    }

    if (uGIntStatus & INT_RESUME)
    {
        Inp32(PCGCCTL,uPcgctl);
        Outp32(PCGCCTL, uPcgctl &~ (1<<0));    //start pclk
        DBGUSB("\n [USB_Diag_Log]  : Resume Mode \n");
    }

    if (uGIntStatus & INT_SUSPEND)
    {
        Inp32(PCGCCTL,uPcgctl);
        Outp32(PCGCCTL, uPcgctl |(1<<0));    //stop pclk
        DBGUSB("\n [USB_Diag_Log]  : Suspend Mode\n");
    }

    if(uGIntStatus & INT_RX_FIFO_NOT_EMPTY)
    {
        UINT32 GrxStatus;
        UINT32 fifoCntByte;

        Outp32(GINTMSK,  INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|INT_RESET|INT_SUSPEND);

        Inp32(GRXSTSP, GrxStatus);

        if ((GrxStatus & SETUP_PKT_RECEIVED) == SETUP_PKT_RECEIVED)
        {
            DBGUSB("SETUP_PACKET_RECEIVED\n");
            OTGDEV_HandleEvent_EP0();
            Inp32(GRSTCTL,uGrstctl);
            Outp32(GRSTCTL, uGrstctl &~ (0x1f<<6)|(0x1<<5));    //tx fifo flush
            g_bTransferEp0 = true;
        }
        else if ((GrxStatus & OUT_PKT_RECEIVED) == OUT_PKT_RECEIVED)
        {

            fifoCntByte = (GrxStatus & 0x7ff0)>>4;
            if(((GrxStatus & 0xF)==BULK_OUT_EP)&&(fifoCntByte))
            {
                DBGUSB("Bulk Out : OUT_PKT_RECEIVED\n");
                OTGDEV_HandleEvent_BulkOut(fifoCntByte);
                if( oOtgDev.m_eOpMode == USB_CPU )
                    Outp32(GINTMSK, INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY);
                return;
            }
        }
        Outp32(GINTMSK, INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|INT_RESET
            |INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY); //gint unmask
    }

    if ((uGIntStatus & INT_IN_EP) || (uGIntStatus & INT_OUT_EP))
    {
        UINT32 uDmaEnCheck;
        UINT32 uRemainCnt;

        Inp32(DAINT, ep_int);

        if (ep_int & (1<<CONTROL_EP))
        {
            Inp32(DIEPINT0, ep_int_status);

            if (ep_int_status & INTKN_TXFEMP)
            {
                if (g_bTransferEp0==true)
                {
                    OTGDEV_TransferEp0();
                    g_bTransferEp0 = false;
                }
            }
            Outp32(DIEPINT0, ep_int_status); // Interrupt Clear
        }

        else if (ep_int & ((1<<CONTROL_EP)<<16))
        {
            Inp32(DOEPINT0, ep_int_status);
            OTGDEV_SetOutEpXferSize(EP_TYPE_CONTROL, 1, oOtgDev.m_uControlEPMaxPktSize);
            Outp32(DOEPCTL0, 0u<<31|1<<26);        //ep0 enable, clear nak

            Outp32(DOEPINT0, ep_int_status);         // Interrupt Clear
        }

        if(ep_int & (1<<BULK_IN_EP))
        {
            Inp32(bulkIn_DIEPINT, ep_int_status);
            Outp32(bulkIn_DIEPINT, ep_int_status); // Interrupt Clear

            if ( (ep_int_status&INTKN_TXFEMP) && oOtgDev.m_eOpMode == USB_CPU)
                OTGDEV_HandleEvent_BulkIn();

            Inp32(GAHBCFG, uDmaEnCheck);
            if ((uDmaEnCheck&MODE_DMA)&&(ep_int_status&TRANSFER_DONE))
            {
                UINT32 temp;
                DBGUSB("DMA IN : Transfer Done\n");

                Inp32(bulkIn_DIEPDMA, temp);
                oOtgDev.m_pUpPt = (UINT8 *)temp;
                uRemainCnt = oOtgDev.m_uUploadSize- ((UINT32)oOtgDev.m_pUpPt - oOtgDev.m_uUploadAddr);

                if (uRemainCnt>0)
                {
                    UINT32 uPktCnt, uRemainder;
                    uPktCnt = (UINT32)(uRemainCnt/oOtgDev.m_uBulkInEPMaxPktSize);
                    uRemainder = (UINT32)(uRemainCnt%oOtgDev.m_uBulkInEPMaxPktSize);
                    if(uRemainder != 0)
                    {
                        uPktCnt += 1;
                    }
                    if (uPktCnt> 1023)
                    {
                        OTGDEV_SetInEpXferSize(EP_TYPE_BULK, 1023, (oOtgDev.m_uBulkInEPMaxPktSize*1023));
                    }
                    else
                    {
                        OTGDEV_SetInEpXferSize(EP_TYPE_BULK, uPktCnt, uRemainCnt);
                    }
                    Outp32(bulkIn_DIEPCTL, 1u<<31|1<<26|2<<18|1<<15|BULK_IN_EP<<11|oOtgDev.m_uBulkInEPMaxPktSize<<0);        //ep1 enable, clear nak, bulk, usb active, next ep1, max pkt
                }
				else{
					DBGUSB("DMA IN : Transfer Complete\n");
					transferComplete=1;
				}
            }
        }

        if (ep_int & ((1<<BULK_OUT_EP)<<16))
        {
            Inp32(bulkOut_DOEPINT, ep_int_status);
            Outp32(bulkOut_DOEPINT, ep_int_status);         // Interrupt Clear
            Inp32(GAHBCFG, uDmaEnCheck);
            if ((uDmaEnCheck&MODE_DMA)&&(ep_int_status&TRANSFER_DONE))
            {
                UINT32 temp;
                DBGUSB("DMA OUT : Transfer Done\n");
				transferComplete=1;
                Inp32(bulkOut_DOEPDMA, temp);
                oOtgDev.m_pDownPt = (UINT8 *)temp;

                uRemainCnt = oOtgDev.m_uDownloadFileSize - ((UINT32)oOtgDev.m_pDownPt - oOtgDev.m_uDownloadAddress + 8);

                if (uRemainCnt>0)
                {
                    UINT32 uPktCnt, uRemainder;
                    uPktCnt = (UINT32)(uRemainCnt/oOtgDev.m_uBulkOutEPMaxPktSize);
                    uRemainder = (UINT32)(uRemainCnt%oOtgDev.m_uBulkOutEPMaxPktSize);
                    if(uRemainder != 0)
                    {
                        uPktCnt += 1;
                    }
                    if (uPktCnt> 1023)
                    {
                        OTGDEV_SetOutEpXferSize(EP_TYPE_BULK, 1023, (oOtgDev.m_uBulkOutEPMaxPktSize*1023));
                    }
                    else
                    {
                        OTGDEV_SetOutEpXferSize(EP_TYPE_BULK, uPktCnt, uRemainCnt);
                    }
                    Outp32(bulkOut_DOEPCTL, 1<<31|1<<26|2<<18|1<<15|oOtgDev.m_uBulkOutEPMaxPktSize<<0);        //ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64
                }
                else
                {
                    DBGUSB("DMA OUT : Transfer Complete\n");
					transferComplete=1;
                    delayLoop(500);        //for FPGA ???
                }
            }
        }
    }

}

//////////
// Function Name : OTGDEV_HandleEvent_EP0
// Function Desctiption : This function is called when Setup packet is received.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_HandleEvent_EP0(void)
{
    UINT32 DeviceRequestLength;
    UINT16 i;
    UINT32 ReadBuf[64]={0x0000, };
    UINT16 setaddress;
    UINT32 uRemoteWakeUp=0;

    if (oOtgDev.m_uEp0State == EP0_STATE_INIT)
    {

        for(i=0;i<2;i++)
        {
            //ReadBuf[i] = Inp32(EP0_FIFO);
            ReadBuf[i] = Input32(EP0_FIFO);
        }

        oOtgDev.m_oDeviceRequest.bmRequestType=ReadBuf[0];
        oOtgDev.m_oDeviceRequest.bRequest=ReadBuf[0]>>8;
        oOtgDev.m_oDeviceRequest.wValue_L=ReadBuf[0]>>16;
        oOtgDev.m_oDeviceRequest.wValue_H=ReadBuf[0]>>24;
        oOtgDev.m_oDeviceRequest.wIndex_L=ReadBuf[1];
        oOtgDev.m_oDeviceRequest.wIndex_H=ReadBuf[1]>>8;
        oOtgDev.m_oDeviceRequest.wLength_L=ReadBuf[1]>>16;
        oOtgDev.m_oDeviceRequest.wLength_H=ReadBuf[1]>>24;

//        OTGDEV_PrintEp0Pkt((UINT8 *)&oOtgDev.m_oDeviceRequest, 8);

        switch (oOtgDev.m_oDeviceRequest.bRequest)
        {
            case STANDARD_SET_ADDRESS:
                setaddress = (oOtgDev.m_oDeviceRequest.wValue_L); // Set Address Update bit
                Outp32(DCFG, 1<<18|setaddress<<4|oOtgDev.m_eSpeed<<0);
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                break;

            case STANDARD_SET_DESCRIPTOR:
                DBGUSB("\n MCU >> Set Descriptor \n");
                break;

            case STANDARD_SET_CONFIGURATION:
                DBGUSB("\n MCU >> Set Configuration \n");
                g_usConfig = oOtgDev.m_oDeviceRequest.wValue_L; // Configuration value in configuration descriptor
                oOtgDev.m_uIsUsbOtgSetConfiguration = 1;
                break;

            case STANDARD_GET_CONFIGURATION:
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 2);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                Outp32(EP0_FIFO, g_usConfig);
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                break;

            case STANDARD_GET_DESCRIPTOR:
                switch (oOtgDev.m_oDeviceRequest.wValue_H)
                {
                    case DEVICE_DESCRIPTOR:
                        DBGUSB("\n MCU >> Get Device Descriptor \n");
                        oOtgDev.m_uEp0State = EP0_STATE_GD_DEV_0;
                        break;

                    case CONFIGURATION_DESCRIPTOR:
                        DBGUSB("\n MCU >> Get Configuration Descriptor \n");

                        DeviceRequestLength = (UINT32)((oOtgDev.m_oDeviceRequest.wLength_H << 8) |
                            oOtgDev.m_oDeviceRequest.wLength_L);

                        if (DeviceRequestLength > CONFIG_DESC_SIZE){
                        // === GET_DESCRIPTOR:CONFIGURATION+INTERFACE+ENDPOINT0+ENDPOINT1 ===
                        // Windows98 gets these 4 descriptors all together by issuing only a request.
                        // Windows2000 gets each descriptor seperately.
                        // oOtgDev.m_uEpZeroTransferLength = CONFIG_DESC_TOTAL_SIZE;
                            oOtgDev.m_uEp0State = EP0_STATE_GD_CFG_0;
                        }
                        else
                            oOtgDev.m_uEp0State = EP0_STATE_GD_CFG_ONLY_0; // for win2k
                        break;

                    case STRING_DESCRIPTOR :
                        switch(oOtgDev.m_oDeviceRequest.wValue_L)
                        {
                            case 0:
                                oOtgDev.m_uEp0State = EP0_STATE_GD_STR_I0;
                                break;
                            case 1:
                                oOtgDev.m_uEp0State = EP0_STATE_GD_STR_I1;
                                break;
                            case 2:
                                oOtgDev.m_uEp0State = EP0_STATE_GD_STR_I2;
                                break;
                            default:
                                    break;
                        }
                        break;
                    case ENDPOINT_DESCRIPTOR:
                        switch(oOtgDev.m_oDeviceRequest.wValue_L&0xf)
                        {
                        case 0:
                            oOtgDev.m_uEp0State=EP0_STATE_GD_EP0_ONLY_0;
                            break;
                        case 1:
                            oOtgDev.m_uEp0State=EP0_STATE_GD_EP1_ONLY_0;
                            break;
                        default:
                            break;
                        }
                        break;

                    case DEVICE_QUALIFIER:
                        DBGUSB("\n MCU >> Get Device Qualifier Descriptor \n");
                        oOtgDev.m_uEp0State = EP0_STATE_GD_DEV_QUALIFIER;
                        break;
                }
                break;

            case STANDARD_CLEAR_FEATURE:
                DBGUSB("\n MCU >> Clear Feature \n");
                switch (oOtgDev.m_oDeviceRequest.bmRequestType)
                {
                    case DEVICE_RECIPIENT:
                        if (oOtgDev.m_oDeviceRequest.wValue_L == 1)
                            uRemoteWakeUp = false;
                        break;

                    case ENDPOINT_RECIPIENT:
                        if (oOtgDev.m_oDeviceRequest.wValue_L == 0)
                        {
                            if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x7f) == 0x00)
                                oStatusGet.Endpoint0= 0;

                            if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x8f) == 0x01) // IN  Endpoint 1
                                oStatusGet.Endpoint1= 0;

                            if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x8f) == 0x02) // OUT Endpoint 3
                                oStatusGet.Endpoint2= 0;
                        }
                        break;

                    default:
                        break;
                }
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                break;

            case STANDARD_SET_FEATURE:
                DBGUSB("\n MCU >> Set Feature \n");
                switch (oOtgDev.m_oDeviceRequest.bmRequestType)
                {
                    case DEVICE_RECIPIENT:
                        if (oOtgDev.m_oDeviceRequest.wValue_L == 1)
                            uRemoteWakeUp = true;
                            break;

                    case ENDPOINT_RECIPIENT:
                        if (oOtgDev.m_oDeviceRequest.wValue_L == 0)
                        {
                            if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x7f) == 0x00)
                                oStatusGet.Endpoint0= 1;

                            if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x8f) == 0x01)
                                oStatusGet.Endpoint1= 1;

                            if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x8f) == 0x02)
                                oStatusGet.Endpoint2= 1;
                        }
                        break;

                    default:
                        break;
                }
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                break;

            case STANDARD_GET_STATUS:
                switch(oOtgDev.m_oDeviceRequest.bmRequestType)
                {
                    case  (0x80):
                        oStatusGet.Device=((UINT8)uRemoteWakeUp<<1)|0x1;        // SelfPowered
                        oOtgDev.m_uEp0State = EP0_STATE_GET_STATUS0;
                        break;

                    case  (0x81):
                        oStatusGet.Interface=0;
                        oOtgDev.m_uEp0State = EP0_STATE_GET_STATUS1;
                        break;

                    case  (0x82):
                        if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x7f) == 0x00)
                            oOtgDev.m_uEp0State = EP0_STATE_GET_STATUS2;

                        if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x8f) == 0x01)
                            oOtgDev.m_uEp0State = EP0_STATE_GET_STATUS3;

                        if ((oOtgDev.m_oDeviceRequest.wIndex_L & 0x8f) == 0x02)
                            oOtgDev.m_uEp0State = EP0_STATE_GET_STATUS4;
                        break;

                    default:
                        break;
                }
                break;

            case STANDARD_GET_INTERFACE:
                oOtgDev.m_uEp0State = EP0_STATE_INTERFACE_GET;
                break;

            case STANDARD_SET_INTERFACE:
                oInterfaceGet.AlternateSetting= oOtgDev.m_oDeviceRequest.wValue_L;
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                break;

            case STANDARD_SYNCH_FRAME:
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                break;

            default:
                break;
        }
    }
    OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, oOtgDev.m_uControlEPMaxPktSize);
    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
}

//////////
// Function Name : OTGDEV_TransferEp0
// Function Desctiption : This function is called during data phase of control transfer.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_TransferEp0(void)
{

    switch (oOtgDev.m_uEp0State)
    {
        case EP0_STATE_INIT:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 0);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            break;

        // === GET_DESCRIPTOR:DEVICE ===
        case EP0_STATE_GD_DEV_0:
            if (oOtgDev.m_eSpeed == USB_HIGH)
            {
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 18);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, max 64byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescDevice+0, 18); // EP0_PKT_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                DBGUSB("EndpointZeroTransfer(EP0_STATE_GD_DEV)\n");
            }
            else
            {
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 18);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, max 8byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescDevice+0, 8); // EP0_PKT_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_GD_DEV_1;
                DBGUSB("EndpointZeroTransfer(FS:EP0_STATE_GD_DEV_0)\n");
            }
            break;

        case EP0_STATE_GD_DEV_1:
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescDevice+8, 8); // EP0_PKT_SIZE
            oOtgDev.m_uEp0State = EP0_STATE_GD_DEV_2;
            DBGUSB("EndpointZeroTransfer(EP0_STATE_GD_DEV_1)\n");
            break;

        case EP0_STATE_GD_DEV_2:
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescDevice+16, 2); // EP0_PKT_SIZE
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            DBGUSB("EndpointZeroTransfer(EP0_STATE_GD_DEV_2)\n");
            break;

        // === GET_DESCRIPTOR:CONFIGURATION+INTERFACE+ENDPOINT0+ENDPOINT1 ===
        // Windows98 gets these 4 descriptors all together by issuing only a request.
        // Windows2000 gets each descriptor seperately.
        // === GET_DESCRIPTOR:CONFIGURATION ONLY for WIN2K===

        case EP0_STATE_GD_CFG_0:
            if (oOtgDev.m_eSpeed == USB_HIGH)
            {
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 32);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, max 64byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+0, 32); // EP0_PKT_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                DBGUSB("EndpointZeroTransfer(EP0_STATE_GD_CFG)\n");
            }
            else
            {
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 32);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+0, 8); // EP0_PKT_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_GD_CFG_1;
                DBGUSB("EndpointZeroTransfer(EP0_STATE_GD_CFG_0)\n");
            }
            break;

        case EP0_STATE_GD_CFG_1:
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+8, 8); // EP0_PKT_SIZE    OTGDEV_WrPktEp0((UINT8 *)&descConf+8, 1); OTGDEV_WrPktEp0((UINT8 *)&descIf+0, 7);
            oOtgDev.m_uEp0State = EP0_STATE_GD_CFG_2;
            break;

        case EP0_STATE_GD_CFG_2:
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+16, 8); // EP0_PKT_SIZE    OTGDEV_WrPktEp0((UINT8 *)&descIf+7, 2); OTGDEV_WrPktEp0((UINT8 *)&descEndpt0+0, 6);
            oOtgDev.m_uEp0State = EP0_STATE_GD_CFG_3;
            break;

        case EP0_STATE_GD_CFG_3:
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+24, 8); // EP0_PKT_SIZE    OTGDEV_WrPktEp0((UINT8 *)&descEndpt0+6, 1); OTGDEV_WrPktEp0((UINT8 *)&descEndpt1+0, 7);
            oOtgDev.m_uEp0State = EP0_STATE_GD_CFG_4;
            break;

        case EP0_STATE_GD_CFG_4:
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        // === GET_DESCRIPTOR:CONFIGURATION ONLY===
        case EP0_STATE_GD_CFG_ONLY_0:
            if (oOtgDev.m_eSpeed == USB_HIGH)
            {
                DBGUSB("[DBG : EP0_STATE_GD_CFG_ONLY]\n");
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 9);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, max 64byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+0, 9); // EP0_PKT_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
            }
            else
            {
                DBGUSB("[DBG : EP0_STATE_GD_CFG_ONLY_0]\n");
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 9);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+0, 8); // EP0_PKT_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_GD_CFG_ONLY_1;
            }
            break;

        case EP0_STATE_GD_CFG_ONLY_1:
            DBGUSB("[DBG : EP0_STATE_GD_CFG_ONLY_1]\n");
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescConfig+8, 1); // EP0_PKT_SIZE
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        // === GET_DESCRIPTOR:INTERFACE ONLY===

        case EP0_STATE_GD_IF_ONLY_0:
            if (oOtgDev.m_eSpeed == USB_HIGH)
            {
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 9);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, max 64byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescInterface+0, 9);    // INTERFACE_DESC_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
            }
            else
            {
                OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 9);
                Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescInterface+0, 8);    // INTERFACE_DESC_SIZE
                oOtgDev.m_uEp0State = EP0_STATE_GD_IF_ONLY_1;
            }
            break;

        case EP0_STATE_GD_IF_ONLY_1:
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescInterface+8, 1);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;


        // === GET_DESCRIPTOR:ENDPOINT 1 ONLY===
        case EP0_STATE_GD_EP0_ONLY_0:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, ENDPOINT_DESC_SIZE);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescEndpt1+0, ENDPOINT_DESC_SIZE);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        // === GET_DESCRIPTOR:ENDPOINT 2 ONLY===
        case EP0_STATE_GD_EP1_ONLY_0:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, ENDPOINT_DESC_SIZE);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oOtgDev.m_oDesc.oDescEndpt2+0, ENDPOINT_DESC_SIZE);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

                // === GET_DESCRIPTOR:STRING ===
        case EP0_STATE_GD_STR_I0:
            DBGUSB("[GDS0_0]");
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 4);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)aDescStr0, 4);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        case EP0_STATE_GD_STR_I1:
        {
            // __in_bound is to denote the fact that we know oOtgDev.m_uControlEPMaxPktSize will 
            // always contain a bounded value - 8 for fullspeed and 64 for high speed
            __in_bound UINT32 MaxPktSize = oOtgDev.m_uControlEPMaxPktSize; 
            
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, sizeof(aDescStr1));
            if ((oOtgDev.m_uEp0SubState*MaxPktSize + MaxPktSize) < sizeof(aDescStr1))
            {
                if (oOtgDev.m_eSpeed == USB_HIGH)
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
                else
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)aDescStr1+(oOtgDev.m_uEp0SubState*MaxPktSize), MaxPktSize);
                oOtgDev.m_uEp0State = EP0_STATE_GD_STR_I1;
                oOtgDev.m_uEp0SubState++;
            }
            else
            {
                if (oOtgDev.m_eSpeed == USB_HIGH)
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
                else
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)aDescStr1+(oOtgDev.m_uEp0SubState*MaxPktSize), sizeof(aDescStr1)-(oOtgDev.m_uEp0SubState*MaxPktSize));
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                oOtgDev.m_uEp0SubState = 0;
            }
            break;
        }

        case EP0_STATE_GD_STR_I2:
        {
            // __in_bound is to denote the fact that we know oOtgDev.m_uControlEPMaxPktSize will 
            // always contain a bounded value - 8 for fullspeed and 64 for high speed
            __in_bound UINT32 MaxPktSize = oOtgDev.m_uControlEPMaxPktSize; 
            
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, sizeof(aDescStr2));
            if ((oOtgDev.m_uEp0SubState*MaxPktSize + MaxPktSize) < sizeof(aDescStr2))
            {
                if (oOtgDev.m_eSpeed == USB_HIGH)
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
                else
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)aDescStr2+(oOtgDev.m_uEp0SubState*MaxPktSize), MaxPktSize);
                oOtgDev.m_uEp0State = EP0_STATE_GD_STR_I2;
                oOtgDev.m_uEp0SubState++;
            }
            else
            {
                if (oOtgDev.m_eSpeed == USB_HIGH)
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
                else
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                DBGUSB("[E]");
                OTGDEV_WrPktEp0((UINT8 *)aDescStr2+(oOtgDev.m_uEp0SubState*MaxPktSize), sizeof(aDescStr2)-(oOtgDev.m_uEp0SubState*MaxPktSize));
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                oOtgDev.m_uEp0SubState = 0;
            }
            break;
        }
        
        case EP0_STATE_GD_DEV_QUALIFIER:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 10);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)aDeviceQualifierDescriptor+0, 10);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        case EP0_STATE_INTERFACE_GET:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 1);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oInterfaceGet+0, 1);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;


        case EP0_STATE_GET_STATUS0:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 1);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oStatusGet+0, 1);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        case EP0_STATE_GET_STATUS1:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 1);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oStatusGet+1, 1);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        case EP0_STATE_GET_STATUS2:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 1);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oStatusGet+2, 1);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        case EP0_STATE_GET_STATUS3:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 1);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oStatusGet+3, 1);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        case EP0_STATE_GET_STATUS4:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, 1);
            Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
            OTGDEV_WrPktEp0((UINT8 *)&oStatusGet+4, 1);
            oOtgDev.m_uEp0State = EP0_STATE_INIT;
            break;

        default:
            break;
    }
}


//////////
// Function Name : OTGDEV_HandleEvent_BulkIn
// Function Desctiption : This function handles bulk in transfer in CPU mode.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_HandleEvent_BulkIn(void)
{
    UINT8* BulkInBuf;
    UINT32 uRemainCnt;

    DBGUSB("CPU_MODE Bulk In Function\n");

    BulkInBuf = (UINT8*)oOtgDev.m_pUpPt;

    uRemainCnt = oOtgDev.m_uUploadSize- ((UINT32)oOtgDev.m_pUpPt - oOtgDev.m_uUploadAddr);

    if (uRemainCnt > oOtgDev.m_uBulkInEPMaxPktSize)
    {
        OTGDEV_SetInEpXferSize(EP_TYPE_BULK, 1, oOtgDev.m_uBulkInEPMaxPktSize);
        Outp32(bulkIn_DIEPCTL, 1u<<31|1<<26|2<<18|1<<15|oOtgDev.m_uBulkInEPMaxPktSize<<0);        //ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64
        OTGDEV_WrPktBulkInEp(BulkInBuf, oOtgDev.m_uBulkInEPMaxPktSize);

        oOtgDev.m_pUpPt += oOtgDev.m_uBulkInEPMaxPktSize;
    }
    else if(uRemainCnt > 0)
    {
        OTGDEV_SetInEpXferSize(EP_TYPE_BULK, 1, uRemainCnt);
        Outp32(bulkIn_DIEPCTL, 1u<<31|1<<26|2<<18|1<<15|oOtgDev.m_uBulkInEPMaxPktSize<<0);        //ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64
        OTGDEV_WrPktBulkInEp(BulkInBuf, uRemainCnt);

        oOtgDev.m_pUpPt += uRemainCnt;
    }
    else    //uRemainCnt = 0
    {
        Outp32(bulkIn_DIEPCTL, (DEPCTL_SNAK|DEPCTL_BULK_TYPE));
    }
}

//////////
// Function Name : OTGDEV_HandleEvent_BulkOut
// Function Desctiption : This function handles bulk out transfer.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_HandleEvent_BulkOut(UINT32 fifoCntByte)
{
    if (oOtgDev.m_eOpMode == USB_CPU)
    {
//        EdbgOutputDebugString("fifoCntByte : %x\n", fifoCntByte);g_pDownPt
        OTGDEV_RdPktBulkOutEp((UINT8 *)g_pDownPt, fifoCntByte);

        OTGDEV_SetOutEpXferSize(EP_TYPE_BULK, 1, fifoCntByte);

        Outp32(bulkOut_DOEPCTL, 1<<31|1<<26|2<<18|1<<15|oOtgDev.m_uBulkOutEPMaxPktSize<<0);        //ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64
    }
}

//#pragma optimize ("",off)

//////////
// Function Name : OTGDEV_InitPhyCon
// Function Desctiption : This function initializes OTG Phy.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_InitPhyCon(void)
{
    Outp32(PHYPWR, 0x0);
    Outp32(PHYCTRL, 0x20);
    Outp32(RSTCON, 0x1);
    delayLoop(100);
    Outp32(RSTCON, 0x0);
    delayLoop(100);
}

//#pragma optimize ("",on)


//////////
// Function Name : OTGDEV_SoftResetCore
// Function Desctiption : This function soft-resets OTG Core and then unresets again.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_SoftResetCore(void)
{
    volatile UINT32 uTemp;

    Outp32(GRSTCTL, CORE_SOFT_RESET);

    do
    {
        Inp32(GRSTCTL, uTemp);
    }while(!(uTemp & AHB_MASTER_IDLE));

}

//////////
// Function Name : OTGDEV_WaitCableInsertion
// Function Desctiption : This function checks if the cable is inserted.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_WaitCableInsertion(void)
{
    volatile UINT32 uTemp;
    bool ucFirst=1;

    do
    {
        //uTemp = Inp32(GOTGCTL);
        Inp32(GOTGCTL, uTemp);

        if (uTemp & (B_SESSION_VALID|A_SESSION_VALID))
        {
            EdbgOutputDebugString("\nA OTG cable inserted !\n");
            break;
        }
        else if(ucFirst == 1)
        {
            EdbgOutputDebugString("\nInsert a OTG cable into the connector!\n");
            ucFirst = 0;
        }
    }while(1);
}

//////////
// Function Name : OTGDEV_InitCore
// Function Desctiption : This function initializes OTG Link Core.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_InitCore(void)
{
    Outp32(GAHBCFG, PTXFE_HALF|NPTXFE_HALF|MODE_SLAVE|BURST_SINGLE|GBL_INT_UNMASK);

    Outp32(GUSBCFG, 0<<15            // PHY Low Power Clock sel
                    |1<<14            // Non-Periodic TxFIFO Rewind Enable
                    |0x5<<10        // Turnaround time
                    |0<<9|0<<8        // [0:HNP disable, 1:HNP enable][ 0:SRP disable, 1:SRP enable] H1= 1,1
                    |0<<7            // Ulpi DDR sel
                    |0<<6            // 0: high speed utmi+, 1: full speed serial
                    |0<<4            // 0: utmi+, 1:ulpi
                    |1<<3            // phy i/f  0:8bit, 1:16bit
                    |0x7<<0            // HS/FS Timeout*
                    );
}

//////////
// Function Name : OTGDEV_CheckCurrentMode
// Function Desctiption : This function checks the current mode.
// Input : pucMode, current mode(device or host)
// Output : NONE
// Version :
void OTGDEV_CheckCurrentMode(UINT8 *pucMode)
{
    volatile UINT32 uTemp;

    //uTemp = Inp32(GINTSTS);
    Inp32(GINTSTS, uTemp);
    *pucMode = uTemp & 0x1;
}

//////////
// Function Name : OTGDEV_SetSoftDisconnect
// Function Desctiption : This function puts the OTG device core in the disconnected state.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_SetSoftDisconnect(void)
{
    volatile UINT32 uTemp;

    //uTemp = Inp32(DCTL);
    Inp32(DCTL, uTemp);
    uTemp |= SOFT_DISCONNECT;
    Outp32(DCTL, uTemp);
}

//////////
// Function Name : OTGDEV_ClearSoftDisconnect
// Function Desctiption : This function makes the OTG device core to exit from the disconnected state.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_ClearSoftDisconnect(void)
{
    volatile UINT32 uTemp;

    //uTemp = Inp32(DCTL);
    Inp32(DCTL, uTemp);
    uTemp = uTemp & ~SOFT_DISCONNECT;
    Outp32(DCTL, uTemp);
}

//////////
// Function Name : OTGDEV_InitDevice
// Function Desctiption : This function configures OTG Core to initial settings of device mode.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_InitDevice(void)
{
    Outp32(DCFG, 1<<18|oOtgDev.m_eSpeed<<0);                // [][1: full speed(30Mhz) 0:high speed]

    Outp32(GINTMSK, INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|INT_RESET
                    |INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY);    //gint unmask
}



//////////
// Function Name : OTGDEV_SetAllOutEpNak
// Function Desctiption : This function sets NAK bit of all EPs.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_SetAllOutEpNak(void)
{
    UINT8 i;
    volatile UINT32 uTemp;

    for(i=0;i<16;i++)
    {
        //uTemp = Inp32(DOEPCTL0+0x20*i);
        Inp32(DOEPCTL0+0x20*i, uTemp);
        uTemp |= DEPCTL_SNAK;
        Outp32(DOEPCTL0+0x20*i, uTemp);
    }
}

//////////
// Function Name : OTGDEV_ClearAllOutEpNak
// Function Desctiption : This function clears NAK bit of all EPs.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_ClearAllOutEpNak(void)
{
    UINT8 i;
    volatile UINT32 uTemp;

    for(i=0;i<16;i++)
    {
        //uTemp = Inp32(DOEPCTL0+0x20*i);
        Inp32(DOEPCTL0+0x20*i, uTemp);
        uTemp |= DEPCTL_CNAK;
        Outp32(DOEPCTL0+0x20*i, uTemp);
    }
}

//////////
// Function Name : OTGDEV_SetMaxPktSizes
// Function Desctiption : This function sets the max packet sizes of USB transfer types according to the speed.
// Input : eSpeed, usb speed(high or full)
// Output : NONE
// Version :
void OTGDEV_SetMaxPktSizes(USB_SPEED eSpeed)
{
    if (eSpeed == USB_HIGH)
    {
        oOtgDev.m_eSpeed = USB_HIGH;
        oOtgDev.m_uControlEPMaxPktSize = HIGH_SPEED_CONTROL_PKT_SIZE;
        oOtgDev.m_uBulkInEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;
        oOtgDev.m_uBulkOutEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;
    }
    else
    {
        oOtgDev.m_eSpeed = USB_FULL;
        oOtgDev.m_uControlEPMaxPktSize = FULL_SPEED_CONTROL_PKT_SIZE;
        oOtgDev.m_uBulkInEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;
        oOtgDev.m_uBulkOutEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;
    }
}

//////////
// Function Name : OTGDEV_SetEndpoint
// Function Desctiption : This function sets the endpoint-specific CSRs.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_SetEndpoint(void)
{
    // Unmask DAINT source
    Outp32(DIEPINT0, 0xff);
    Outp32(DOEPINT0, 0xff);
    Outp32(bulkIn_DIEPINT, 0xff);
    Outp32(bulkOut_DOEPINT, 0xff);

    // Init For Ep0
    if(oOtgDev.m_eSpeed == USB_HIGH)
    {
        Outp32(DIEPCTL0, (1u<<31)|((1<<26)|(BULK_IN_EP<<11)|(0<<0)));    //MPS:64bytes
    }
    else
    {
        Outp32(DIEPCTL0, (1u<<31)|((1<<26)|(BULK_IN_EP<<11)|(3<<0)));    //MPS:8bytes
    }

    OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, oOtgDev.m_uControlEPMaxPktSize);
    OTGDEV_SetOutEpXferSize(EP_TYPE_CONTROL, 1, oOtgDev.m_uControlEPMaxPktSize);
    Outp32(DOEPCTL0, (0u<<31)|(1<<26));        //ep0 enable, clear nak
}

//////////
// Function Name : OTGDEV_SetDescriptorTable
// Function Desctiption : This function sets the standard descriptors.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_SetDescriptorTable(void)
{
    // Standard device descriptor
    oOtgDev.m_oDesc.oDescDevice.bLength=DEVICE_DESC_SIZE;    //0x12
    oOtgDev.m_oDesc.oDescDevice.bDescriptorType=DEVICE_DESCRIPTOR;
    oOtgDev.m_oDesc.oDescDevice.bDeviceClass=0xFF; // 0x0
    oOtgDev.m_oDesc.oDescDevice.bDeviceSubClass=0x0;
    oOtgDev.m_oDesc.oDescDevice.bDeviceProtocol=0x0;
    oOtgDev.m_oDesc.oDescDevice.bMaxPacketSize0=oOtgDev.m_uControlEPMaxPktSize;
    oOtgDev.m_oDesc.oDescDevice.idVendorL=0xE8;    //0x45;
    oOtgDev.m_oDesc.oDescDevice.idVendorH=0x04;    //0x53;
    oOtgDev.m_oDesc.oDescDevice.idProductL=0x00;    //0x34;
    oOtgDev.m_oDesc.oDescDevice.idProductH=0x64;    //0x12;
    oOtgDev.m_oDesc.oDescDevice.bcdDeviceL=0x00;
    oOtgDev.m_oDesc.oDescDevice.bcdDeviceH=0x01;
    oOtgDev.m_oDesc.oDescDevice.iManufacturer=0x1; // index of string descriptor
    oOtgDev.m_oDesc.oDescDevice.iProduct=0x2;    // index of string descriptor
    oOtgDev.m_oDesc.oDescDevice.iSerialNumber=0x0;
    oOtgDev.m_oDesc.oDescDevice.bNumConfigurations=0x1;
    if (oOtgDev.m_eSpeed == USB_FULL) {
        oOtgDev.m_oDesc.oDescDevice.bcdUSBL=0x10;
        oOtgDev.m_oDesc.oDescDevice.bcdUSBH=0x01;     // Ver 1.10
    }
    else {
        oOtgDev.m_oDesc.oDescDevice.bcdUSBL=0x00;
        oOtgDev.m_oDesc.oDescDevice.bcdUSBH=0x02;     // Ver 2.0
    }

    // Standard configuration descriptor
    oOtgDev.m_oDesc.oDescConfig.bLength=CONFIG_DESC_SIZE; // 0x9 bytes
    oOtgDev.m_oDesc.oDescConfig.bDescriptorType=CONFIGURATION_DESCRIPTOR;
    oOtgDev.m_oDesc.oDescConfig.wTotalLengthL=CONFIG_DESC_TOTAL_SIZE;
    oOtgDev.m_oDesc.oDescConfig.wTotalLengthH=0;
    oOtgDev.m_oDesc.oDescConfig.bNumInterfaces=1;
// dbg    descConf.bConfigurationValue=2; // why 2? There's no reason.
    oOtgDev.m_oDesc.oDescConfig.bConfigurationValue=1;
    oOtgDev.m_oDesc.oDescConfig.iConfiguration=0;
    oOtgDev.m_oDesc.oDescConfig.bmAttributes=CONF_ATTR_DEFAULT|CONF_ATTR_SELFPOWERED; // bus powered only.
    oOtgDev.m_oDesc.oDescConfig.maxPower=25; // draws 50mA current from the USB bus.

    // Standard interface descriptor
    oOtgDev.m_oDesc.oDescInterface.bLength=INTERFACE_DESC_SIZE; // 9
    oOtgDev.m_oDesc.oDescInterface.bDescriptorType=INTERFACE_DESCRIPTOR;
    oOtgDev.m_oDesc.oDescInterface.bInterfaceNumber=0x0;
    oOtgDev.m_oDesc.oDescInterface.bAlternateSetting=0x0; // ?
    oOtgDev.m_oDesc.oDescInterface.bNumEndpoints = 2;    // # of endpoints except EP0
    oOtgDev.m_oDesc.oDescInterface.bInterfaceClass=0xff; // 0x0 ?
    oOtgDev.m_oDesc.oDescInterface.bInterfaceSubClass=0x0;
    oOtgDev.m_oDesc.oDescInterface.bInterfaceProtocol=0x0;
    oOtgDev.m_oDesc.oDescInterface.iInterface=0x0;

    // Standard endpoint0 descriptor
    oOtgDev.m_oDesc.oDescEndpt1.bLength=ENDPOINT_DESC_SIZE;
    oOtgDev.m_oDesc.oDescEndpt1.bDescriptorType=ENDPOINT_DESCRIPTOR;
    oOtgDev.m_oDesc.oDescEndpt1.bEndpointAddress=BULK_IN_EP|EP_ADDR_IN; // 2400Xendpoint 1 is IN endpoint.
    oOtgDev.m_oDesc.oDescEndpt1.bmAttributes=EP_ATTR_BULK;
    oOtgDev.m_oDesc.oDescEndpt1.wMaxPacketSizeL=(UINT8)oOtgDev.m_uBulkInEPMaxPktSize; // 64
    oOtgDev.m_oDesc.oDescEndpt1.wMaxPacketSizeH=(UINT8)(oOtgDev.m_uBulkInEPMaxPktSize>>8);
    oOtgDev.m_oDesc.oDescEndpt1.bInterval=0x0; // not used

    // Standard endpoint1 descriptor
    oOtgDev.m_oDesc.oDescEndpt2.bLength=ENDPOINT_DESC_SIZE;
    oOtgDev.m_oDesc.oDescEndpt2.bDescriptorType=ENDPOINT_DESCRIPTOR;
    oOtgDev.m_oDesc.oDescEndpt2.bEndpointAddress=BULK_OUT_EP|EP_ADDR_OUT; // 2400X endpoint 3 is OUT endpoint.
    oOtgDev.m_oDesc.oDescEndpt2.bmAttributes=EP_ATTR_BULK;
    oOtgDev.m_oDesc.oDescEndpt2.wMaxPacketSizeL=(UINT8)oOtgDev.m_uBulkOutEPMaxPktSize; // 64
    oOtgDev.m_oDesc.oDescEndpt2.wMaxPacketSizeH=(UINT8)(oOtgDev.m_uBulkOutEPMaxPktSize>>8);
    oOtgDev.m_oDesc.oDescEndpt2.bInterval=0x0; // not used
}

//////////
// Function Name : OTGDEV_CheckEnumeratedSpeed
// Function Desctiption : This function checks the current usb speed.
// Input : eSpeed, usb speed(high or full)
// Output : NONE
// Version :
void OTGDEV_CheckEnumeratedSpeed(USB_SPEED *eSpeed)
{
    volatile UINT32 uDStatus;

    //uDStatus = Inp32(DSTS); // System status read
    Inp32(DSTS, uDStatus); // System status read

    *eSpeed = (uDStatus&0x6) >>1;
}




//////////
// Function Name : OTGDEV_SetInEpXferSize
// Function Desctiption : This function sets DIEPTSIZn CSR according to input parameters.
// Input : eType, transfer type
//            uPktCnt, packet count to transfer
//            uXferSize, transfer size
// Output : NONE
// Version :
void OTGDEV_SetInEpXferSize(EP_TYPE eType, UINT32 uPktCnt, UINT32 uXferSize)
{
    if(eType == EP_TYPE_CONTROL)
    {
        Outp32(DIEPTSIZ0, (uPktCnt<<19)|(uXferSize<<0));
    }
    else if(eType == EP_TYPE_BULK)
    {
        Outp32(bulkIn_DIEPTSIZ, (1<<29)|(uPktCnt<<19)|(uXferSize<<0));
    }
}

//////////
// Function Name : OTGDEV_SetOutEpXferSize
// Function Desctiption : This function sets DOEPTSIZn CSR according to input parameters.
// Input : eType, transfer type
//            uPktCnt, packet count to transfer
//            uXferSize, transfer size
// Output : NONE
// Version :
void OTGDEV_SetOutEpXferSize(EP_TYPE eType, UINT32 uPktCnt, UINT32 uXferSize)
{
    if(eType == EP_TYPE_CONTROL)
    {
        Outp32(DOEPTSIZ0, (1<<29)|(uPktCnt<<19)|(uXferSize<<0));
    }
    else if(eType == EP_TYPE_BULK)
    {
        Outp32(bulkOut_DOEPTSIZ, (uPktCnt<<19)|(uXferSize<<0));
    }
}

//////////
// Function Name : OTGDEV_WrPktEp0
// Function Desctiption : This function reads data from the buffer and writes the data on EP0 FIFO.
// Input : buf, address of the data buffer to write on Control EP FIFO
//            num, size of the data to write on Control EP FIFO(byte count)
// Output : NONE
// Version :
void OTGDEV_WrPktEp0(UINT8 *buf, int num)
{
    int i;
    volatile UINT32 Wr_Data=0;

    for(i=0;i<num;i+=4)
    {
        Wr_Data = ((*(buf+3))<<24)|((*(buf+2))<<16)|((*(buf+1))<<8)|*buf;
        Outp32(control_EP_FIFO, Wr_Data);
        buf += 4;
    }
}

//////////
// Function Name : OTGDEV_PrintEp0Pkt
// Function Desctiption : This function reads data from the buffer and displays the data.
// Input : pt, address of the data buffer to read
//            count, size of the data to read(byte count)
// Output : NONE
// Version :
void OTGDEV_PrintEp0Pkt(UINT8 *pt, UINT8 count)
{
    int i;
    EdbgOutputDebugString("[DBG:");
    for(i=0;i<count;i++)
        EdbgOutputDebugString("%x,", pt[i]);
    EdbgOutputDebugString("]\n");
}


//////////
// Function Name : OTGDEV_WrPktBulkInEp
// Function Desctiption : This function reads data from the buffer and writes the data on Bulk In EP FIFO.
// Input : buf, address of the data buffer to write on Bulk In EP FIFO
//            num, size of the data to write on Bulk In EP FIFO(byte count)
// Output : NONE
// Version :
void OTGDEV_WrPktBulkInEp(UINT8 *buf, int num)
{
    int i;
    volatile UINT32 Wr_Data=0;

    for(i=0;i<num;i+=4)
    {
        Wr_Data=((*(buf+3))<<24)|((*(buf+2))<<16)|((*(buf+1))<<8)|*buf;
        Outp32(bulkIn_EP_FIFO, Wr_Data);
        buf += 4;
    }
}

//////////
// Function Name : OTGDEV_RdPktBulkOutEp
// Function Desctiption : This function reads data from Bulk Out EP FIFO and writes the data on the buffer.
// Input : buf, address of the data buffer to write
//            num, size of the data to read from Bulk Out EP FIFO(byte count)
// Output : NONE
// Version :
void OTGDEV_RdPktBulkOutEp(UINT8 *buf, int num)
{
    int i;
    volatile UINT32 Rdata;

    for (i=0;i<num;i+=4)
    {
        //Rdata = Inp32(bulkOut_EP_FIFO);
        Inp32(bulkOut_EP_FIFO, Rdata);

        buf[i] = (UINT8)Rdata;
        buf[i+1] = (UINT8)(Rdata>>8);
        buf[i+2] = (UINT8)(Rdata>>16);
        buf[i+3] = (UINT8)(Rdata>>24);
    }

    // increase global down pointer for usb download function
    g_pDownPt += num;
}



//////////
// Function Name : OTGDEV_IsUsbOtgSetConfiguration
// Function Desctiption : This function checks if Set Configuration is received from the USB Host.
// Input : NONE
// Output : configuration result
// Version :
BOOL OTGDEV_IsUsbOtgSetConfiguration(void)
{
    if (oOtgDev.m_uIsUsbOtgSetConfiguration == 0)
        return false;
    else
        return true;
}

//////////
// Function Name : OTGDEV_SetOpMode
// Function Desctiption : This function sets CSRs related to the operation mode.
// Input : eMode, operation mode(cpu or dma)
// Output : NONE
// Version :
void OTGDEV_SetOpMode(USB_OPMODE eMode)
{
    oOtgDev.m_eOpMode = eMode;

    Outp32(GINTMSK, INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY); //gint unmask
    Outp32(GAHBCFG, MODE_SLAVE|BURST_SINGLE|GBL_INT_UNMASK);

    OTGDEV_SetOutEpXferSize(EP_TYPE_BULK, 1, oOtgDev.m_uBulkOutEPMaxPktSize);
    OTGDEV_SetInEpXferSize(EP_TYPE_BULK, 1, 0);

    Outp32(bulkOut_DOEPCTL, 1<<31|1<<26|2<<18|1<<15|oOtgDev.m_uBulkOutEPMaxPktSize<<0);        //bulk out ep enable, clear nak, bulk, usb active, next ep3, max pkt
    Outp32(bulkIn_DIEPCTL, 0<<31|1<<26|2<<18|1<<15|oOtgDev.m_uBulkInEPMaxPktSize<<0);        //bulk in ep enable, clear nak, bulk, usb active, next ep1, max pkt
}

//////////
// Function Name : OTGDEV_VerifyChecksum
// Function Desctiption : This function calculates the checksum by summing all downloaded data
//                        and then compares the result with the checksum value which DNW sent.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_VerifyChecksum(void)
{
    UINT8* CalcCSPt;
    UINT16 dnCS;
    UINT16 checkSum;

    EdbgOutputDebugString("\nChecksum is being calculated....\n");

    // checksum calculation
    CalcCSPt = (UINT8*)oOtgDev.m_uDownloadAddress;
    checkSum = 0;
    while((UINT32)CalcCSPt < (oOtgDev.m_uDownloadAddress+(oOtgDev.m_uDownloadFileSize-8)))
        checkSum += *CalcCSPt++;

    // checkSum was calculated including dnCS. So, dnCS should be subtracted.
    checkSum=checkSum - *((unsigned char *)(oOtgDev.m_uDownloadAddress+oOtgDev.m_uDownloadFileSize-8-2))
        - *( (unsigned char *)(oOtgDev.m_uDownloadAddress+oOtgDev.m_uDownloadFileSize-8-1) );

    dnCS=*((unsigned char *)(oOtgDev.m_uDownloadAddress+oOtgDev.m_uDownloadFileSize-8-2))+
        (*( (unsigned char *)(oOtgDev.m_uDownloadAddress+oOtgDev.m_uDownloadFileSize-8-1) )<<8);

    if (checkSum ==dnCS)
    {
        EdbgOutputDebugString("Checksum O.K.\n\n");
    }
    else
    {
        EdbgOutputDebugString("Checksum Value => MEM:%x DNW:%x\n",checkSum,dnCS);
        EdbgOutputDebugString("Checksum failed.\n\n");
    }

}

void PowerOnUSB(void)
{
    volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

	// -------------------------------------------------------
	// OTG POWER ON, GPF13-->POWER
    if (pGPIOReg)
    {
		pGPIOReg->GPFCON &= ~(0x3<<26);
		pGPIOReg->GPFCON |=  (0x1<<26);
		pGPIOReg->GPFDAT |=  (0x1<<13);

        // Release
		pGPIOReg = NULL;
    }
}
BOOL InitializeUSB()
{
    OTGDEV_InitOtg(USB_HIGH);

    g_pDownPt = (UINT8 *)DMABUFFER;
    readPtIndex = DMABUFFER;

    return TRUE;
}

void InitializeInterrupt(void)
{
    s6410VIC0 = (S3C6410_VIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_VIC0, FALSE);
    s6410VIC1 = (S3C6410_VIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_VIC1, FALSE);

    System_DisableVIC();
    System_DisableIRQ();
    System_DisableFIQ();

    // Disable All Interrupts
    s6410VIC0->VICINTENCLEAR = 0xFFFFFFFF;
    s6410VIC1->VICINTENCLEAR = 0xFFFFFFFF;
    s6410VIC0->VICSOFTINTCLEAR = 0xFFFFFFFF;
    s6410VIC1->VICSOFTINTCLEAR = 0xFFFFFFFF;

    // All Interrupt is IRQ Mode
    s6410VIC0->VICINTSELECT = 0x0;
    s6410VIC1->VICINTSELECT = 0x0;

    // Clear Current Active Vector Address
    s6410VIC0->VICADDRESS = 0x0;
    s6410VIC1->VICADDRESS = 0x0;

    // Initialize Vector Table
    VIC_InitializeVectTable();

    EdbgOutputDebugString("INFO: (unsigned)C_IsrHandler : 0x%x\r\n", (unsigned)C_IsrHandler);
    EdbgOutputDebugString("INFO: (unsigned)ASM_IsrHandler : 0x%x\r\n", (unsigned)ASM_IsrHandler);

    // make value to assemble code "b IsrHandler"
    //EdbgOutputDebugString("INFO: (unsigned)pISR : 0x%x\r\n", (unsigned)pISR);
    pISR = (unsigned)(0xEA000000)+(((unsigned)ASM_IsrHandler -(DRAM_BASE_CA_START + 0x18 + 0x8))>>2);

    System_EnableIRQ();

    EdbgOutputDebugString("INFO: (unsigned)pISR : 0x%x\r\n", (unsigned)pISR);

    VIC_InterruptEnable(PHYIRQ_OTG);
}

void C_IsrHandler(unsigned int val)
{
    UINT32 irq, irq2;

    irq = s6410VIC0->VICADDRESS;
    irq2 = s6410VIC1->VICADDRESS;

    if (irq == PHYIRQ_OTG)
    {
        VIC_InterruptDisable(PHYIRQ_OTG);
        OTGDEV_HandleEvent();
        VIC_InterruptEnable(PHYIRQ_OTG);
    }
    if (irq == PHYIRQ_I2C)
    {
		//EdbgOutputDebugString("isr iic\r\n");
        VIC_InterruptDisable(PHYIRQ_I2C);
        Isr_IIC();
        VIC_InterruptEnable(PHYIRQ_I2C);
    }
    VIC_ClearVectAddr();
}

// we read any number of bytes, place it into pbdata
// and return the amount of bytes read...easy as that.
#pragma optimize ("",off)
DWORD UbootReadData2(LPBYTE pbData)
{

	DWORD index=0;
    DWORD current_bytes=0;
	DWORD old_bytes=0;
	while(1)
	{
	  current_bytes=((DWORD)g_pDownPt)-readPtIndex;
	  if((current_bytes>0) && (current_bytes==old_bytes))
	  {

		index++;
		//EdbgOutputDebugString("info : Current index: %d\n", index);
	  }
	  else
	  {
		
		old_bytes=current_bytes;
		index=0;
	  }
	  //EdbgOutputDebugString(" ");
      if ( index>=50 )
        {
			EdbgOutputDebugString("info : Copy %d KB from 0x%x to 0x%x \n", current_bytes/1024,(DWORD ) readPtIndex, (DWORD )pbData);
            memcpy((PVOID)pbData, (PVOID)(readPtIndex+6), current_bytes-6); // -6 bytes fro some DNW informations ???

            // Clear Partial Download Memory to 0xFF, Unless Gabage data will be written to Boot Media
            memset((PVOID)readPtIndex, 0xFF, current_bytes);

            readPtIndex += current_bytes;
			return current_bytes-6; // -6 bytes fro some DNW informations ???
			break;
        }
	}
    return current_bytes-6; // -6 bytes fro some DNW informations ???
}
#pragma optimize ("",off)
BOOL UbootReadData(DWORD cbData, LPBYTE pbData)
{
    while(1)
    {
        if (readPtIndex + cbData < readPtIndex)
        {
            // integer overflow
            return FALSE;
        }
        
        if ( (DWORD)g_pDownPt >= readPtIndex + cbData )
        {
            memcpy((PVOID)pbData, (PVOID)readPtIndex, cbData);

            // Clear Partial Download Memory to 0xFF, Unless Gabage data will be written to Boot Media
            memset((PVOID)readPtIndex, 0xFF, cbData);

            readPtIndex += cbData;
            break;
        }
        else if((DWORD)g_pDownPt == DMABUFFER)
        {

        }
    }
    return TRUE;
}
#pragma optimize ("",on)

static void VIC_InterruptEnable(UINT32 intNum)
{
    if(intNum<32)
    {
        s6410VIC0->VICINTENABLE = (1<<intNum);
    }
    else
    {
        s6410VIC1->VICINTENABLE = (1<<(intNum-32));
    }

    return;
}

static void VIC_InterruptDisable(UINT32 intNum)
{
    if(intNum<32)
    {
        s6410VIC0->VICINTENCLEAR = (1<<intNum);
    }
    else
    {
        s6410VIC1->VICINTENCLEAR = (1<<(intNum-32));
    }

    return;
}

static void VIC_ClearVectAddr(void)
{
    s6410VIC0->VICADDRESS = 0x0;
    s6410VIC1->VICADDRESS = 0x0;

    return;
}

static void VIC_InitializeVectTable(void)
{
    s6410VIC0->VICVECTADDR0 = PHYIRQ_EINT0;
    s6410VIC0->VICVECTADDR1 = PHYIRQ_EINT1;
    s6410VIC0->VICVECTADDR2 = PHYIRQ_RTC_TIC;
    s6410VIC0->VICVECTADDR3 = PHYIRQ_CAMIF_C;
    s6410VIC0->VICVECTADDR4 = PHYIRQ_CAMIF_P;

    s6410VIC0->VICVECTADDR5 = PHYIRQ_I2C1;
    s6410VIC0->VICVECTADDR6 = PHYIRQ_I2S_V40;
    s6410VIC0->VICVECTADDR7 = PHYIRQ_SSS;
    s6410VIC0->VICVECTADDR8 = PHYIRQ_3D;

    s6410VIC0->VICVECTADDR9 = PHYIRQ_POST;
    s6410VIC0->VICVECTADDR10 = PHYIRQ_ROTATOR;
    s6410VIC0->VICVECTADDR11 = PHYIRQ_2D;
    s6410VIC0->VICVECTADDR12 = PHYIRQ_TVENC;
    s6410VIC0->VICVECTADDR13 = PHYIRQ_TVSCALER;
    s6410VIC0->VICVECTADDR14 = PHYIRQ_BATF;
    s6410VIC0->VICVECTADDR15 = PHYIRQ_JPEG;
    s6410VIC0->VICVECTADDR16 = PHYIRQ_MFC;
    s6410VIC0->VICVECTADDR17 = PHYIRQ_SDMA0;
    s6410VIC0->VICVECTADDR18 = PHYIRQ_SDMA1;
    s6410VIC0->VICVECTADDR19 = PHYIRQ_ARM_DMAERR;
    s6410VIC0->VICVECTADDR20 = PHYIRQ_ARM_DMA;
    s6410VIC0->VICVECTADDR21 = PHYIRQ_ARM_DMAS;
    s6410VIC0->VICVECTADDR22 = PHYIRQ_KEYPAD;
    s6410VIC0->VICVECTADDR23 = PHYIRQ_TIMER0;
    s6410VIC0->VICVECTADDR24 = PHYIRQ_TIMER1;
    s6410VIC0->VICVECTADDR25 = PHYIRQ_TIMER2;
    s6410VIC0->VICVECTADDR26 = PHYIRQ_WDT;
    s6410VIC0->VICVECTADDR27 = PHYIRQ_TIMER3;
    s6410VIC0->VICVECTADDR28 = PHYIRQ_TIMER4;
    s6410VIC0->VICVECTADDR29 = PHYIRQ_LCD0_FIFO;
    s6410VIC0->VICVECTADDR30 = PHYIRQ_LCD1_FRAME;
    s6410VIC0->VICVECTADDR31 = PHYIRQ_LCD2_SYSIF;

    s6410VIC1->VICVECTADDR0 = PHYIRQ_EINT2;
    s6410VIC1->VICVECTADDR1 = PHYIRQ_EINT3;
    s6410VIC1->VICVECTADDR2 = PHYIRQ_PCM0;
    s6410VIC1->VICVECTADDR3 = PHYIRQ_PCM1;
    s6410VIC1->VICVECTADDR4 = PHYIRQ_AC97;
    s6410VIC1->VICVECTADDR5 = PHYIRQ_UART0;
    s6410VIC1->VICVECTADDR6 = PHYIRQ_UART1;
    s6410VIC1->VICVECTADDR7 = PHYIRQ_UART2;
    s6410VIC1->VICVECTADDR8 = PHYIRQ_UART3;
    s6410VIC1->VICVECTADDR9 = PHYIRQ_DMA0;
    s6410VIC1->VICVECTADDR10 = PHYIRQ_DMA1;
    s6410VIC1->VICVECTADDR11 = PHYIRQ_ONENAND0;
    s6410VIC1->VICVECTADDR12 = PHYIRQ_ONENAND1;
    s6410VIC1->VICVECTADDR13 = PHYIRQ_NFC;
    s6410VIC1->VICVECTADDR14 = PHYIRQ_CFC;
    s6410VIC1->VICVECTADDR15 = PHYIRQ_UHOST;
    s6410VIC1->VICVECTADDR16 = PHYIRQ_SPI0;
    s6410VIC1->VICVECTADDR17 = PHYIRQ_SPI1;
    s6410VIC1->VICVECTADDR18 = PHYIRQ_I2C;
    s6410VIC1->VICVECTADDR19 = PHYIRQ_HSITX;
    s6410VIC1->VICVECTADDR20 = PHYIRQ_HSIRX;
    s6410VIC1->VICVECTADDR21 = PHYIRQ_RESERVED;
    s6410VIC1->VICVECTADDR22 = PHYIRQ_MSM;
    s6410VIC1->VICVECTADDR23 = PHYIRQ_HOSTIF;
    s6410VIC1->VICVECTADDR24 = PHYIRQ_HSMMC0;
    s6410VIC1->VICVECTADDR25 = PHYIRQ_HSMMC1;
    s6410VIC1->VICVECTADDR26 = PHYIRQ_OTG;
    s6410VIC1->VICVECTADDR27 = PHYIRQ_IRDA;
    s6410VIC1->VICVECTADDR28 = PHYIRQ_RTC_ALARM;
    s6410VIC1->VICVECTADDR29 = PHYIRQ_SEC;
    s6410VIC1->VICVECTADDR30 = PHYIRQ_PENDN;
    s6410VIC1->VICVECTADDR31 = PHYIRQ_ADC;
}

static void delayLoop(int count)
{
    volatile int j,i;
    for(j = 0; j < count; j++)
        for(i=0;i<1000;i++);
}


/*=================================IIC================================================*/
static volatile u8 *g_PcIIC_BUFFER;
static volatile u32 g_uIIC_PT;
static u32 g_uIIC_DATALEN;
static volatile u8 g_cIIC_STAT0;
static volatile u8 g_cIIC_SlaveRxDone;
static volatile u8 g_cIIC_SlaveTxDone;
static volatile u32 g_IIC_WRITE_TIME;
static volatile u32 g_IIC_WAIT_TIME;
int g_PCLK = 12000000;

//////////
// Function Name : Isr_IIC
// Function Description : This function is Interrupt Service Routine of IIC
//					  when interrupt occurs, check IICSTAT, find mode and operate it
// Input : NONE
// Output : NONE
// Version : v0.1
void Isr_IIC( void)
{
	u32 uTemp0 = 0;
	u8 cCnt;
	//EdbgOutputDebugString("[Eboot] IIC_ISR\r\n");
	g_cIIC_STAT0 = Input32(rIICSTAT0);
	switch( (g_cIIC_STAT0>>6)&0x3) 
	{
		case SlaveRX	:	
			//UART_Printf("IICSTAT0 = %x	",g_cIIC_STAT0);

			if(g_uIIC_PT<100) 
			{
				g_PcIIC_BUFFER[g_uIIC_PT++]=Input8(rIICDS0);
				Delay(100);
				
				uTemp0 = Input32(rIICCON0);
				uTemp0 &= ~(1<<4);			//	Clear pending bit to resume
				Outp32(rIICCON0,uTemp0);
				break;
			}

			Outp8(rIICSTAT0,0x0);			// Stop Int
			
			uTemp0 = Input32(rIICCON0);
			uTemp0 &= ~(1<<4);			//	Clear pending bit to resume
			Outp32(rIICCON0,uTemp0);

			g_cIIC_SlaveRxDone = 1;

			Delay(1);						//	wait until Stop Condition is in effect
			break;

		case SlaveTX	:		
			//UART_Printf("IICSTAT = %x	",g_cIIC_STAT0);

			if(g_uIIC_PT>100)
			{
				Outp32(rIICSTAT0,0xd0);			//	Stop Master Tx condition, ACK flag clear

				uTemp0 = Input32(rIICCON0);
				uTemp0 &= ~(1<<4);			//	Clear pending bit to resume
				Outp32(rIICCON0,uTemp0);

				g_cIIC_SlaveTxDone = 1;

				Delay(1);						//	wait until Stop Condition is in effect
				break;
			}

			Outp8(rIICDS0,g_PcIIC_BUFFER[g_uIIC_PT++]);
			for(cCnt=0;cCnt<10;cCnt++);		//	for setup time (rising edge of IICSCL)
			Delay(100);
			
			uTemp0 = Input32(rIICCON0);
			uTemp0 &= ~(1<<4);			//	Clear pending bit to resume
			Outp32(rIICCON0,uTemp0);				
			break;

		case MasterRX:	
			if (g_uIIC_PT>0)
				g_PcIIC_BUFFER[g_uIIC_PT-1] = Input32(rIICDS0);

			g_uIIC_PT++;

			if (g_uIIC_PT==g_uIIC_DATALEN)
			{
				uTemp0 = Input32(rIICCON0);
				uTemp0 &= ~(1<<7);			// Disable Ack generation
				Outp32(rIICCON0,uTemp0);		// EEPROM 에서는 stop 전에 ACK 안보냄
			}
			else if (g_uIIC_PT > g_uIIC_DATALEN)
				Outp32(rIICSTAT0,0x90);		//	Stop Master Rx condition

			uTemp0 = Input32(rIICCON0);
			uTemp0 &= ~(1<<4);			//	Clear pending bit to resume
			Outp32(rIICCON0,uTemp0);
			break;

		case MasterTX:		

			if (g_uIIC_PT<g_uIIC_DATALEN)
				Outp32(rIICDS0,g_PcIIC_BUFFER[g_uIIC_PT]);
			else	
				Outp32(rIICSTAT0,0xd0);			//	Stop Master Tx condition, ACK flag clear

			g_uIIC_PT++;
			uTemp0 = Input32(rIICCON0);
			uTemp0 &= ~(1<<4);			//	Clear pending bit to resume
			Outp32(rIICCON0,uTemp0);
			break;
	}

	g_cIIC_STAT0	&=0xf;
	VIC_ClearVectAddr();
	//INTC_ClearVectAddr();
}
//////////
// Function Name : IIC_Open
// Function Description : This function Set up VIC & IICCON with user's input frequency which determines uClkValue
// Input : ufreq	ufreq(Hz) = PCLK/16/uClkValue
// Output : NONE
// Version : v0.1
void IIC_Open( u32 ufreq)		//	Hz order. freq(Hz) = PCLK/16/clk_divider
{
	u32	uSelClkSrc;
	u32	uClkValue;

	
	//INTC_SetVectAddr(NUM_IIC,Isr_IIC);
	//INTC_Enable(NUM_IIC);
    volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

	pGPIOReg->GPBCON &= ~((0xf<<20)|(0xf<<24));
	pGPIOReg->GPBCON |= ((0x2<<20)|(0x2<<24));
	pGPIOReg->GPBPUDSLP &= ~((0x3<<10)|(0x3<<12));
	pGPIOReg->GPBPUDSLP |= ((0x2<<10)|(0x2<<12));

    VIC_InterruptEnable(PHYIRQ_I2C);
	
	//GPIO_SetFunctionEach(eGPIO_B,eGPIO_5,2);
	//GPIO_SetFunctionEach(eGPIO_B,eGPIO_6,2);	
	//GPIO_SetPullUpDownEach(eGPIO_B,eGPIO_5,2);
	//GPIO_SetPullUpDownEach(eGPIO_B,eGPIO_6,2);
	
	if ((((g_PCLK>>4)/ufreq)-1)>0xf) 
	{
		uSelClkSrc	=	1;
		uClkValue	=	((g_PCLK>>9)/ufreq) -1;		//	PCLK/512/freq
	} 
	else 
	{
		uSelClkSrc	=	0;
		uClkValue	=	((g_PCLK>>4)/ufreq) -1;		//	PCLK/16/freq
	}

	//Prescaler IICCLK=PCLK/16, Enable interrupt, Transmit clock value Tx clock=IICCLK/16


	EdbgOutputDebugString("g_PCLK = %d",g_PCLK);
	EdbgOutputDebugString("uSelClkSrc = %d",uSelClkSrc);
	EdbgOutputDebugString("uClkValue = %d",uClkValue);


	Outp32(rIICCON0,(uSelClkSrc<<6) | (1<<5) | (uClkValue&0xf));
	Outp32(rIICADD0,0xc0);		//	Slave address = [7:1]
	Outp32(rIICSTAT0,0x10);	//	IIC bus data output enable(Rx/Tx)
	Outp32(rIICLC0,0x0);			//	SDA Filter enable,delayed 15clks
}

//////////
// Function Name : IIC_Close
// Function Description : This function disable IIC
// Input : NONE
// Output : NONE
// Version : v0.1
void IIC_Close(void)
{
    volatile S3C6410_GPIO_REG *pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

	pGPIOReg->GPBCON &= ~((0xf<<20)|(0xf<<24));
	pGPIOReg->GPBPUDSLP &= ~((0x3<<10)|(0x3<<12));
	
	//INTC_Disable(NUM_IIC);
    VIC_InterruptDisable(PHYIRQ_I2C);
	
	Outp32(rIICSTAT0,0x0);                		//	IIC bus data output disable(Rx/Tx)
	//GPIO_SetFunctionEach(eGPIO_B,eGPIO_5,0);
	//GPIO_SetFunctionEach(eGPIO_B,eGPIO_6,0);	
	//GPIO_SetPullUpDownEach(eGPIO_B,eGPIO_5,0);
	//GPIO_SetPullUpDownEach(eGPIO_B,eGPIO_6,0);
}


//////////
// Function Name : IIC_SetWrite
// Function Description : This function sets up write mode with 7-bit addresses
// Input :  cSlaveAddr [8bit SlaveDeviceAddress], 
//		  pData[Data which you want to write], 
//		  uDataLen [Data Length]
// Output : NONE
// Version : v0.1

static void IIC_SetWrite( u8 cSlaveAddr,  u8 *pData, u32 uDataLen)
{
	u32 uTmp0;
	u32 uTmp1;
	
	uTmp0 = Input32(rIICSTAT0);
	while(uTmp0&(1<<5))		//	Wait until IIC bus is free.
		uTmp0 = Input32(rIICSTAT0);			

	g_PcIIC_BUFFER	=	pData;
	g_uIIC_PT		=	0;
	g_uIIC_DATALEN	=	uDataLen;

	uTmp1 = Input32(rIICCON0);
	uTmp1 |= (1<<7);
	Outp32(rIICCON0,uTmp1);				//	Ack generation Enable
	
	Outp32(rIICDS0,cSlaveAddr);
	Outp32(rIICSTAT0,0xf0);				//	Master Tx Start.
}



static bool IIC_WaitTimeOut()
{
	g_IIC_WAIT_TIME++;
	if(g_IIC_WAIT_TIME%50000)
		return false;
	return true;
}

static bool IIC_WriteTimeOut()
{
	g_IIC_WRITE_TIME++;
	if(g_IIC_WRITE_TIME%300)
		return false;
	return true;
}
//////////
// Function Name : IIC_Wait
// Function Description : This function waits until the command takes effect
//											But not for IIC bus free
// Input : NONE 
// Output : NONE
// Version : v0.1
static BOOL IIC_Wait( void)						//	Waiting for the command takes effect.
{
	g_IIC_WAIT_TIME=1;
	while(g_uIIC_PT<=g_uIIC_DATALEN){
		if(IIC_WaitTimeOut())
		{
			EdbgOutputDebugString("[Eboot] IIC_WaitTimeOut!\r\n");
			return false;
		}
	}
	return true;
}

//////////
// Function Name : IIC_Status
// Function Description : This function returns IIC Status Register value at last interrupt occur
// Input : NONE
// Output : NONE
// Version : v0.1
u8 IIC_Status( void)						//	Return IIC Status Register value at last interrupt occur.
{
	return	g_cIIC_STAT0;
}
//////////
// Function Name : IIC_Write
// Function Description : This function STARTs up write mode with 7-bit addresses
// Input : cSlaveAddr [8bit SlaveDeviceAddress], 
//		 cAddr[8bit Address where you want to write], 
//		 pData[Data which you want to write]
// Output : NONE
// Version : v0.1

BOOL IIC_Write(u8 cSlaveAddr, u8 cAddr, u8 cData)
{
	u8 cD[2];

	cD[0]=cAddr;
	cD[1]=cData;

	g_IIC_WRITE_TIME=1;
	
	IIC_SetWrite(cSlaveAddr, cD, 2);

	if(!IIC_Wait())
		return false;
	do
	{									//	Polling for an ACK signal from SerialEEPROM.
		IIC_SetWrite(cSlaveAddr, NULL, 0);
		if(!IIC_Wait())
			return false;
		if(IIC_WriteTimeOut())
		{
			EdbgOutputDebugString("[Eboot] IIC_WriteTimeOut!\r\n");
			return false;
		}
	} while(IIC_Status()&0x1);
	return true;
}

//////////
// Function Name : IIC_SetRead
// Function Description : This function sets up Read mode with 7-bit addresses
// Input : cSlaveAddr [8bit SlaveDeviceAddress], 
//		  pData[Data which you want to read], 
//		  uDataLen [Data Length]
// Output : NONE
// Version : v0.1
static void IIC_SetRead(  u8 cSlaveAddr,  u8 *pData, u32 uDataLen)
{
	u32 uTmp2;
	u32 uTmp3;
	
	uTmp2= Input32(rIICSTAT0);
	while(uTmp2&(1<<5))		//	Wait until IIC bus is free.
		uTmp2 = Input32(rIICSTAT0);			

	g_PcIIC_BUFFER	=	pData;
	g_uIIC_PT		=	0;
	g_uIIC_DATALEN	=	uDataLen;

	uTmp3 = Input32(rIICCON0);
	uTmp3 |= (1<<7);
	Outp32(rIICCON0,uTmp3);				//	Ack generation Enable
	Outp32(rIICDS0,cSlaveAddr);
	Outp32(rIICSTAT0,0xB0);				//	Master Rx Start.
}
//////////
// Function Name : IIC_Read
// Function Description : This function STARTs up read mode with 7-bit addresses
// Input : cSlaveAddr [8bit SlaveDeviceAddress], 
//		 cAddr [8bit Address where you want to read], 
//		 cData [pointer of Data which you want to read]
// Output : NONE
// Version : v0.1
void IIC_Read(u8 cSlaveAddr,u8 cAddr,u8 *cData)
{
	IIC_SetWrite( cSlaveAddr, &cAddr, 1);			// following EEPROM random address access procedure
	IIC_SetRead( cSlaveAddr, cData, 1);
	IIC_Wait();								//	Waiting for read complete.
}

