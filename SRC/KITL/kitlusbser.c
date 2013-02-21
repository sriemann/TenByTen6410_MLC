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

#include <windows.h>
#include <bsp.h>
#include <kitl_cfg.h>
#include "kitlusbser.h"

// start // from otg_dev.c

static DWORD KitlIoPortBase;

extern USBSerKitl_POLL;

/*----------------------------------------------------------------------
 * Variables for EP0 resend control. Keep track of last packet sent.
 */
S3CUSB_INFO  g_Info; // record keeping
SetupPKG dReq;
USBSERKITL_INFO USBSerInfo;

static char * sendPacket;
static int sendPacketLength;
static int sendTotalLength;
static char * savSendPacket;
static int savSendPacketLength;

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

volatile UINT32                 readPtIndex;
volatile UINT8                    *g_pDownPt;

void SetEndpoint(void);
static void delayLoop(int count);

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
    volatile UINT32 uTemp;

    uTemp = Inp32SYSC(0x900);
    Outp32SYSC(0x900, uTemp|(1<<16)); // unmask usb signal
    
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
        OTGDEV_SetSoftDisconnect();
        delayLoop(10);
        OTGDEV_ClearSoftDisconnect();
        OTGDEV_InitDevice();
    }
    else
    {
        EdbgOutputDebugString("Error : Current Mode is Host\n");
        return;
    }
    
    if(oOtgDev.m_eSpeed == USB_HIGH)
    {
        oOtgDev.m_uControlEPMaxPktSize = HIGH_SPEED_CONTROL_PKT_SIZE;
        oOtgDev.m_uBulkInEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;
        oOtgDev.m_uBulkOutEPMaxPktSize = HIGH_SPEED_BULK_PKT_SIZE;
    }
    else
    {
        oOtgDev.m_uControlEPMaxPktSize = FULL_SPEED_CONTROL_PKT_SIZE;
        oOtgDev.m_uBulkInEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;
        oOtgDev.m_uBulkOutEPMaxPktSize = FULL_SPEED_BULK_PKT_SIZE;
    }
    
    oOtgDev.m_uEp0State = EP0_STATE_INIT;
    OTGDEV_SetOpMode(USB_CPU);
    OTGDEV_SetDescriptorTable();
    OTGDEV_SetEndpoint();
}



//////////
// Function Name : OTGDEV_HandleEvent
// Function Desctiption : This function handles various OTG interrupts of Device mode.
// Input : NONE
// Output : NONE
// Version :
UINT16 OTGDEV_HandleEvent(UINT8 *pch, UINT16 length)
{
    volatile UINT32 uGIntStatus, uDStatus;
    UINT32 ep_int_status, ep_int;

    UINT32 uPcgctl;
    UINT32 uGrstctl;


    length = 0;
    Inp32(GINTSTS, uGIntStatus); // System status read
    Outp32(GINTSTS, uGIntStatus); // Interrupt Clear

    if (uGIntStatus & INT_RESET) // Reset interrupt
    {        
        Outp32(GINTSTS, INT_RESET); // Interrupt Clear
        
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
        Outp32(GINTSTS, INT_ENUMDONE); // Interrupt Clear
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
        Outp32(GINTSTS, INT_RESUME); // Host software send ClearPortFeature. Interrupt Clear

        Inp32(PCGCCTL,uPcgctl);
        Outp32(PCGCCTL, uPcgctl &~ (1<<0));    //start pclk

        DBGUSB("\n [USB_Diag_Log]  : Resume Mode \n");
    }

    if (uGIntStatus & INT_SUSPEND)
    {
        Outp32(GINTSTS, INT_SUSPEND); // Interrupt Clear

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

               g_bTransferEp0=true;
        }
        else if ((GrxStatus & OUT_PKT_RECEIVED) == OUT_PKT_RECEIVED)
        {

            fifoCntByte = (GrxStatus & 0x7ff0)>>4;
            if(((GrxStatus & 0xF)==BULK_OUT_EP)&&(fifoCntByte))
            {                
                DBGUSB("Bulk Out : OUT_PKT_RECEIVED\n");
                OTGDEV_HandleEvent_BulkOut(pch, fifoCntByte);
                if( oOtgDev.m_eOpMode == USB_CPU )
                    Outp32(GINTMSK, INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY);
                length = fifoCntByte;
            }
        }
        Outp32(GINTMSK, INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY); //gint unmask
    }

    if ((uGIntStatus & INT_IN_EP) || (uGIntStatus & INT_OUT_EP))
    {
        
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
            
            OTGDEV_SetOutEpXferSize(EP_TYPE_CONTROL, 1, 8);
            Outp32(DOEPCTL0, 0u<<31|1<<26);        //ep0 enable, clear nak
            
            Outp32(DOEPINT0, ep_int_status);         // Interrupt Clear
        }
    }
    return length;
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

        if (oOtgDev.m_oDeviceRequest.bmRequestType & 0x60 )
        { 
            switch ( oOtgDev.m_oDeviceRequest.bRequest )
            {
                case ( SET_CONTROL_LINE_STATE ) :
                {

                    /* Host is notifying us of control line state.
                    * wValue contains bitmask
                    * 0 - DTR
                    * 1 - RTS
                    */
                    //KITLOutputDebugString("SET_CONTROL_LINE_STATE,  H:%X,L:%X \r\n",g_oDeviceRequest.wValue_H, g_oDeviceRequest.wValue_L);

                    if ( oOtgDev.m_oDeviceRequest.wValue_L & 0x01 )
                    {
                        USBSerInfo.dwModemStatus |= (MS_DSR_ON|MS_RLSD_ON); // DTR active, set DSR/RLSD
                    }
                    else
                    {
                        USBSerInfo.dwModemStatus &= ~(MS_DSR_ON|MS_RLSD_ON); // DTR clear, clr DSR/RLSD
                    }
                    if ( oOtgDev.m_oDeviceRequest.wValue_L & 0x02 )
                    {
                        USBSerInfo.dwModemStatus |= MS_CTS_ON;   // RTS active, set CTS
                    }
                    else
                    {
                        USBSerInfo.dwModemStatus &= ~MS_CTS_ON;  // RTS clear, clear CTS
                    }
                    if ( USBSerInfo.dwModemStatus != 0 )
                    {
                        USBSerInfo.dwState = KITLUSBSER_STATE_CONNECTED;
                        KITLOutputDebugString("\n KITLUSBSER_STATE_CONNECTED \n");                             
                    }
                    else
                    {
                        USBSerInfo.dwState = KITLUSBSER_STATE_CONFIGURED;
                        KITLOutputDebugString("\n KITLUSBSER_STATE_CONFIGURED \n");                             
                    }

                /* Command is complete */
                //  sendDone(0);
                }
                break;

                default:
                {
                    //KITLOutputDebugString("Unknown vendor/class request %X\r\n",g_oDeviceRequest.bRequest);
                }
            }
            return;
        }

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
                g_usConfig = oOtgDev.m_oDeviceRequest.wValue_L; // Configuration value in configuration descriptor
                oOtgDev.m_uIsUsbOtgSetConfiguration = 1;
                USBSerInfo.dwState = KITLUSBSER_STATE_CONFIGURED;        
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
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, sizeof(aDescStr1));
            if ((oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize+oOtgDev.m_uControlEPMaxPktSize)<sizeof(aDescStr1))
            {
                if (oOtgDev.m_eSpeed == USB_HIGH)
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
                else
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)aDescStr1+(oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize), oOtgDev.m_uControlEPMaxPktSize);
                oOtgDev.m_uEp0State = EP0_STATE_GD_STR_I1;
                oOtgDev.m_uEp0SubState++;
            }
            else
            {
                if (oOtgDev.m_eSpeed == USB_HIGH)
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
                else
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)aDescStr1+(oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize), sizeof(aDescStr1)-(oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize));
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                oOtgDev.m_uEp0SubState = 0;
            }
            break;

        case EP0_STATE_GD_STR_I2:
            OTGDEV_SetInEpXferSize(EP_TYPE_CONTROL, 1, sizeof(aDescStr2));
            if ((oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize+oOtgDev.m_uControlEPMaxPktSize)<sizeof(aDescStr2))
            {
                if (oOtgDev.m_eSpeed == USB_HIGH)
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(0<<0));    //ep0 enable, clear nak, next ep0, 64byte
                else
                    Outp32(DIEPCTL0, (1u<<31)|(1<<26)|(BULK_IN_EP<<11)|(3<<0));    //ep0 enable, clear nak, next ep0, 8byte
                OTGDEV_WrPktEp0((UINT8 *)aDescStr2+(oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize), oOtgDev.m_uControlEPMaxPktSize);
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
                OTGDEV_WrPktEp0((UINT8 *)aDescStr2+(oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize), sizeof(aDescStr2)-(oOtgDev.m_uEp0SubState*oOtgDev.m_uControlEPMaxPktSize));
                oOtgDev.m_uEp0State = EP0_STATE_INIT;
                oOtgDev.m_uEp0SubState = 0;
            }
            break;

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
UINT16 OTGDEV_HandleEvent_BulkIn(UINT8 *pch, UINT32 fifoCntByte)
{
    UINT32 ucLen;

    volatile UINT32 uGIntStatus,uDaint;

    if (!pch ) 
    {
        fifoCntByte = 0;
        return fifoCntByte;
    }
                      
    // Don't try to send more than EP1 can handle.
    if( fifoCntByte > oOtgDev.m_uBulkInEPMaxPktSize )
    {
            ucLen = oOtgDev.m_uBulkInEPMaxPktSize;
    }
    else 
    {
        // If we end exactly on a packet boundary, the host doesn't
        // realize there is no more data.  So if we exactly fill the final 
        // packet, truncate it so we can send a short packet next frame
        // indicating end of the transmission
        if( fifoCntByte == oOtgDev.m_uBulkInEPMaxPktSize ) 
        {
            ucLen = oOtgDev.m_uBulkInEPMaxPktSize - 4;
        }
        else
        {
            ucLen = fifoCntByte;
        }
    }   
    while(1)
    {    
        Inp32(GINTSTS, uGIntStatus); // System status read
        
        if (uGIntStatus & INT_IN_EP)
        {
            Inp32(DAINT, uDaint);
            if (uDaint & (1<<BULK_IN_EP))
            {
                break;
            }
        }
    }   
    
    OTGDEV_SetInEpXferSize(EP_TYPE_BULK, 1, ucLen);
    Outp32(bulkIn_DIEPCTL, 1<<31|1<<26|2<<18|1<<15|oOtgDev.m_uBulkInEPMaxPktSize<<0);        //ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64
    OTGDEV_WrPktBulkInEp(pch, ucLen);
    return ucLen;
}

//////////
// Function Name : OTGDEV_HandleEvent_BulkOut
// Function Desctiption : This function handles bulk out transfer.
// Input : NONE
// Output : NONE
// Version :
void OTGDEV_HandleEvent_BulkOut(UINT8 *pch, UINT32 fifoCntByte)
{
    if (oOtgDev.m_eOpMode == USB_CPU)
    {
        OTGDEV_RdPktBulkOutEp((UINT8 *)pch, fifoCntByte);

        OTGDEV_SetOutEpXferSize(EP_TYPE_BULK, 1, oOtgDev.m_uBulkOutEPMaxPktSize);
        
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
    delayLoop(10);
    Outp32(RSTCON, 0x0);
    delayLoop(10);
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
    oOtgDev.m_oDesc.oDescDevice.bDeviceSubClass=0xFF;
    oOtgDev.m_oDesc.oDescDevice.bDeviceProtocol=0xFF;
    oOtgDev.m_oDesc.oDescDevice.bMaxPacketSize0=oOtgDev.m_uControlEPMaxPktSize;
    oOtgDev.m_oDesc.oDescDevice.idVendorL=0x5E;    //0x45;
    oOtgDev.m_oDesc.oDescDevice.idVendorH=0x04;    //0x53;
    oOtgDev.m_oDesc.oDescDevice.idProductL=0xCE;    //0x34;
    oOtgDev.m_oDesc.oDescDevice.idProductH=0x00;    //0x12;
    oOtgDev.m_oDesc.oDescDevice.bcdDeviceL=0x00;
    oOtgDev.m_oDesc.oDescDevice.bcdDeviceH=0x00;
    oOtgDev.m_oDesc.oDescDevice.iManufacturer=0x0; // index of string descriptor
    oOtgDev.m_oDesc.oDescDevice.iProduct=0x0;    // index of string descriptor
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
    oOtgDev.m_oDesc.oDescInterface.bInterfaceSubClass=0xFF;
    oOtgDev.m_oDesc.oDescInterface.bInterfaceProtocol=0xFF;
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
    
    Inp32(DSTS, uDStatus); // System status read

    *eSpeed = (USB_SPEED)((uDStatus&0x6) >>1);
    /// 0 : High Speed (phy 30mhz or 60mhz
    /// 1 : Full Speed (phy 30mhz or 60mhz)
    /// 2 : low Speed (phy 6Mhz
    /// 3 : Full Speed (phy 48Mhz)
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


/* S3C6410USBSER_Init
 *
 *  Called by PQOAL KITL framework to initialize the serial port
 *
 *  Return Value:
 */
BOOL S3C6410USBSER_Init (KITL_SERIAL_INFO *pSerInfo)
{
    volatile S3C6410_SYSCON_REG *g_pSysConReg;

    g_pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

    // Enable Clock Source
    g_pSysConReg->HCLK_GATE |= (1<<20);        // HCLK_USB

    memset(&g_Info, 0, sizeof(g_Info));
    memset(&USBSerInfo, 0, sizeof(USBSERKITL_INFO));

    KITLOutputDebugString ("Wait for connecting\n");

    KitlIoPortBase = (DWORD)pSerInfo->pAddress; 
//    delayLoop(100);
    if (!KitlIoPortBase)
    {    
        return FALSE;
    }
    else
    {
        OTGDEV_InitOtg(USB_HIGH);        
//        OTGDEV_InitOtg(USB_FULL);
    }
    pSerInfo->bestSize = 512;
//    pSerInfo->bestSize = 64;

    return TRUE;
}

/* S3C6410USBSER_WriteData
 *
 *  Block until the byte is sent
 *
 *  Return Value: TRUE on success, FALSE otherwise
 */
 #define NPTxFEmpty (0x1<<5)
 
UINT16 S3C6410USBSER_WriteData (UINT8 *pch, UINT16 length)
{

    volatile UINT32 uGIntStatus;
    if (USBSerInfo.dwState != KITLUSBSER_STATE_CONNECTED)
    {
        return length;
    }

    while(1)
    {
        Inp32(GINTSTS, uGIntStatus); // Wait Until TX FIFO is availabled.
        if (uGIntStatus & NPTxFEmpty)
        {
            break;
        }
    }
    
    length = OTGDEV_HandleEvent_BulkIn(pch, length);
    return length;    
}


void S3C6410USBSER_SendComplete (UINT16 length)
{

    volatile UINT32 dwDiepint;
    Inp32(bulkIn_DIEPINT, dwDiepint);
    Outp32(bulkIn_DIEPINT, dwDiepint);
}



/* S3C6410USBSER_ReadData
 *
 *  Called from PQOAL KITL to read a byte from serial port
 *
 *  Return Value: TRUE on success, FALSE otherwise
 */
UINT16 S3C6410USBSER_ReadData (UINT8 *pch, UINT16 length)
{
    length = OTGDEV_HandleEvent(pch, length);
    return length;
}


/* S3C6410USBSER_EnableInt
 *
 *  Enable Recv data interrupt
 *
 *  Return Value:
 */

VOID S3C6410USBSER_EnableInt (void)
{
    volatile S3C6410_VIC_REG *g_pVIC1Reg;
    g_pVIC1Reg = (volatile S3C6410_VIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_VIC1, FALSE);

    if (!USBSerKitl_POLL)
    {
        g_pVIC1Reg->VICINTENABLE = (1<<(PHYIRQ_OTG-32));
    }
}

/* S3C6410USBSER_DisableInt
 *
 *  Disable Recv data interrupt
 *
 *  Return Value:
 */
VOID S3C6410USBSER_DisableInt (void)
{
    volatile S3C6410_VIC_REG *g_pVIC1Reg;
    g_pVIC1Reg = (volatile S3C6410_VIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_VIC1, FALSE);

    if (!USBSerKitl_POLL)
    {
        g_pVIC1Reg->VICINTENCLEAR = (1<<(PHYIRQ_OTG-32));
    }
}


VOID S3C6410USBSER_PowerOff (void)
{
    //KITLOutputDebugString ("+S3C6410USBSER_PowerOff\n");    
}

VOID S3C6410USBSER_PowerOn (void)
{
    //KITLOutputDebugString ("+S3C6410USBSER_PowerOn\n");
}

// serial driver
OAL_KITL_SERIAL_DRIVER DrvUSBSerial = {
    S3C6410USBSER_Init,
    NULL,
    S3C6410USBSER_WriteData,//TX
    S3C6410USBSER_SendComplete,//Clear EP1 Interrupt pending
    S3C6410USBSER_ReadData,//RX
    S3C6410USBSER_EnableInt,
    S3C6410USBSER_DisableInt,
    NULL,    //S3C6410USBSER_PowerOff,
    NULL,    //S3C6410USBSER_PowerOn,
    NULL, //flow control
};

const OAL_KITL_SERIAL_DRIVER *GetKitlUSBSerialDriver (void)
{
    return &DrvUSBSerial;
}

static void delayLoop(int count)
{
    volatile int j,k;
    for(j = 0; j < count; j++)
    {
        for(k=0;k<100000;k++);
    }
}

