/******************* (c) Marvell Semiconductor, Inc., 2001 ********************
 *
 *  $HEADER&
 *
 *  Purpose:
 *     This file contains CF MAC registers definition
 *
 *  Notes:
 *
 *****************************************************************************/

#ifndef __MACREG_H_    /* filename in CAPS */
#define __MACREG_H_


/*
===============================================================================
                              PUBLIC DEFINITIONS
===============================================================================
*/
//
// NDIS version
//
#define MRVDRV_NDIS_MAJOR_VERSION 0x5

#define MRVDRV_NDIS_MINOR_VERSION 0x1

#define MRVDRV_DRIVER_VERSION ((MRVDRV_NDIS_MAJOR_VERSION*0x100) + MRVDRV_NDIS_MINOR_VERSION)

//
// Product name
//
#define	VENDORDESCRIPTOR "Marvell W8100 802.11 SDIO CARD NIC"


//
//          Define vendor ID and device ID
//
#define MRVL_PCI_VENDOR_ID                  0x11AB // VID
#define MRVL_8100_PCI_DEVICE_ID             0x1FA4 // DID
#define MRVL_8100_CARDBUS_DEVICE_ID         0x8101

#define MRVL_8100_PCI_REV_0                 0x00
#define MRVL_8100_PCI_REV_1                 0x01
#define MRVL_8100_PCI_REV_2                 0x02
#define MRVL_8100_PCI_REV_3                 0x03
#define MRVL_8100_PCI_REV_4                 0x04
#define MRVL_8100_PCI_REV_5                 0x05
#define MRVL_8100_PCI_REV_6                 0x06
#define MRVL_8100_PCI_REV_7                 0x07
#define MRVL_8100_PCI_REV_8                 0x08
#define MRVL_8100_PCI_REV_9                 0x09
#define MRVL_8100_PCI_REV_a                 0x0a
#define MRVL_8100_PCI_REV_b                 0x0b
#define MRVL_8100_PCI_REV_c                 0x0c
#define MRVL_8100_PCI_REV_d                 0x0d
#define MRVL_8100_PCI_REV_e                 0x0e
#define MRVL_8100_PCI_REV_f                 0x0f

//          The following version information is used bysed by OID_GEN_VENDOR_ID
#define MRVL_8100_PCI_VER_ID               0x00
#define MRVL_8100_CARDBUS_VER_ID           0x01

//
//          Define staiton register offset
//

//          Map to 0x80000000 (Bus control) on BAR4
#define MACREG_REG_H2A_INTERRUPT_CAUSE      0x00000C18 // (From host to ARM)
#define MACREG_REG_H2A_INTERRUPT_MASK       0x00000C1C // (From host to ARM)

#define MACREG_REG_A2H_INTERRUPT_CAUSE      0x00000C88 // (From ARM to host)
#define MACREG_REG_A2H_INTERRUPT_MASK       0x00000C8C // (From ARM to host)

//			Modification on 10/25/02
#define MACREG_REG_A2H_INTERRUPT_MASK_DISABLE 0x00000001  // Bit 0 indicates INT Enable/Disable
                                                          // Bit 0 =1 to Disable

//			Modification on 10/25/02
#define MACREG_REG_A2H_INTERRUPT_CLEAR      0x00000C90 // Write 0 to clear/re-enable interrupt

//          Map to 0x80000000 on BAR4
//#define MACREG_REG_OP_CODE                0x00000000  // Obsolete
#define MACREG_REG_GEN_PTR                  0x00000C68
#define MACREG_REG_INT_CODE                 0x00000C6C
#define MACREG_REG_PPA_BASE                 0x00000C70
#define MACREG_REG_DPA_BASE                 0x00000C74
#define MACREG_REG_RXPDQ_BASE               0x00000C78
#define MACREG_REG_RXPD_RD                  0x00000C7C
#define MACREG_REG_RXPD_WR                  0x00000C80
#define MACREG_REG_WCB_BASE                 0x00000C84
#define MACREG_REG_RSR						0x00000C94

//          (PCI control registers)
#define GT64115_PCI_TIMEOUT                 0x00000C04
#define GT64115_BAR0_REMAP                  0x00000C48
#define GT64115_BAR1_REMAP                  0x00000C4C
#define GT64115_BAR2_REMAP                  0x00000C50
#define GT64115_BAR3_REMAP                  0x00000C54

//          Bit definitio for MACREG_REG_A2H_INTERRUPT_CAUSE (A2HRIC)
#define MACREG_A2HRIC_BIT_MF_INT            0x00000200 // bit 9
#define MACREG_A2HRIC_BIT_MC_INT            0x00000400 // bit 10
#define MACREG_A2HRIC_BIT_SI_INT            0x00000800 // bit 11
#define MACREG_A2HRIC_BIT_GP_INT            0x00001000 // bit 12
#define MACREG_A2HRIC_BIT_CS_INT            0x00002000 // bit 13

#define MACREG_A2HRIC_BIT_DMA0_COMPLETE     0x00008000 // bit 15
#define MACREG_A2HRIC_BIT_DMA1_COMPLETE     0x00010000 // bit 16

#define MACREG_A2HRIC_BIT_TX_DONE           0x01000000 // bit 24
#define MACREG_A2HRIC_BIT_RX_RDY            0x02000000 // bit 25
#define MACREG_A2HRIC_BIT_OPC_DONE          0x04000000 // bit 26
#define MACREG_A2HRIC_BIT_MAC_EVENT         0x08000000 // bit 27
#define MACREG_A2HRIC_BIT_MASK              0x0F000000

//          Bit definitio for MACREG_REG_H2A_INTERRUPT_CAUSE (H2ARIC)
#define MACREG_H2ARIC_BIT_PPA_READY         0x10000000 // bit 28
#define MACREG_H2ARIC_BIT_DOOR_BELL         0x20000000 // bit 29

//          Bit definitio for MACREG_REG_CPU_INTERRUPT_MASK (CIM)
#define MACREG_CIM_BIT_DMA0_COMPLETE_MASK   0x00000010 // bit 4
#define MACREG_CIM_BIT_DMA1_COMPLETE_MASK   0x00000020 // bit 5
#define MACREG_CIM_BIT_DMA2_COMPLETE_MASK   0x00000040 // bit 6
#define MACREG_CIM_BIT_PPA_READY_MASK       0x10000000 // bit 28
#define MACREG_CIM_BIT_DOOR_BELL_MASK       0x20000000 // bit 29

//          Bit definitio for MACREG_REG_PCI_INTERRUPT_MASK (PIM) 
#define MACREG_PIM_BIT_TX_DONE_MASK         0x00400000 // bit 22
#define MACREG_PIM_BIT_RX_RDY_MASK          0x00800000 // bit 23
#define MACREG_PIM_BIT_OPC_DONE_MASK        0x01000000 // bit 24
#define MACREG_PIM_BIT_MAC_EVENT_MASK       0x02000000 // bit 25

//          INT code register event definition
#define MACREG_INT_CODE_TX_PPA_FREE         0x00000000 // Added 05/28/02
#define MACREG_INT_CODE_TX_DMA_DONE         0x00000001
#define MACREG_INT_CODE_LINK_LOSE_NO_SCAN   0x00000003
#define MACREG_INT_CODE_LINK_SENSED         0x00000004
#define MACREG_INT_CODE_CMD_FINISHED        0x00000005
#define MACREG_INT_CODE_MIB_CHANGED         0x00000006 // Added 01/22/02
#define MACREG_INT_CODE_INIT_DONE           0x00000007 // Added 01/30/02
#define MACREG_INT_CODE_DEAUTHENTICATED     0x00000008 // Added 11/08/02
#define MACREG_INT_CODE_DISASSOCIATED       0x00000009 // Added 11/08/02
#define MACREG_INT_CODE_PS_AWAKE			0x0000000a
#define MACREG_INT_CODE_PS_SLEEP			0x0000000b
#define MACREG_INT_CODE_TX_DONE				0x0000000c
// tt mic error++
#define MACREG_INT_CODE_WPA_MIC_ERR_UNICAST     0x0000000e
#define MACREG_INT_CODE_WPA_MIC_ERR_MULTICAST   0x0000000d       
// tt mic error--
#define MACREG_INT_CODE_HOST_AWAKE              0x0000000f
#define MACREG_INT_CODE_DS_AWAKE                0x00000010 //Deep Sleep Awake
#define MACREG_INT_CODE_ADHOC_BCN_LOST          0x00000011

#define MACREG_INT_CODE_HOST_SLEEP_AWAKE		0x00000012 


#define MACREG_INT_CODE_BG_SCAN_REPORT		0x00000018 

//dralee_20060601
#define MACREG_INT_CODE_IBSS_COALESCED      0x0000001e

//ahan [2005-12-01], Subscribe Event  ++dralee_20060217
#define MACREG_INT_CODE_RSSI_LOW                    0x00000019
#define MACREG_INT_CODE_SNR_LOW                     0x0000001a
#define MACREG_INT_CODE_MAX_FAIL                    0x0000001b
#define MACREG_INT_CODE_RSSI_HIGH                   0x0000001c  
#define MACREG_INT_CODE_SNR_HIGH                    0x0000001d  
//ahan [2005-12-01]   ++dralee_20060217


/*
===============================================================================
                            PUBLIC TYPE DEFINITIONS
===============================================================================
*/

#else
#endif // __MACREG_H_
