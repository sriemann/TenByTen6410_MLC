/******************** (c) Marvell Semiconductor, Inc., *************************
 *
 *  Purpose:
 *
 *      This file contains the function prototypes, data structure and defines 
 *      for all the host/station commands. Please check the MrvDrv 802.11 
 *      GUI/Driver/Station Interface Specification for detailed command 
 *      information
 *
 *  Notes:
 *
 *
 *  $Author: schiu $
 *
 *  $Date: 2004/12/09 $
 *
 *  $Revision: #9 $
 *
 *****************************************************************************/

#ifndef __HOSTCMD__H
#define __HOSTCMD__H

#include "wlan_ccx.h"

//=============================================================================
//          PUBLIC DEFINITIONS
//=============================================================================
#define DEFAULT_WEP_STATUS          1
#define DEFAULT_POWERSAVE_MODE      0
#define DEFAULT_DATA_RATE           0xff
#define DEFAULT_CHANNEL             1
#define DEFAULT_AUTHENTICATE_MODE   0   
#define DEFAULT_NETWORK_MODE        1   
#define DEFAULT_RTS_THRESHOLD       2346    
#define DEFAULT_FRAG_THRESHOLD      2346
#define DEFAULT_RX_ANTENNA          0x0000ffff          
#define DEFAULT_TX_ANTENNA          2
#define DEFAULT_PREAMBLE            1


//
//   For diagnostic test purposes
//
#define HostCmd_CMD_DUTY_CYCLE_TEST                 0x002A
#define HostCmd_RET_DUTY_CYCLE_TEST                 0x802A

/*  Define data structure for HostCmd_CMD_DUTY_CYCLE_TEST */
typedef struct _HostCmd_DS_DUTY_CYCLE_TEST {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    ULONG  BeaconOffsetInSQ;
    ULONG  RFParam;  //Replace beaconFrame[2] with RFParam 
    USHORT Reserved;
    
} HostCmd_DS_DUTY_CYCLE_TEST, *PHostCmd_DS_DUTY_CYCLE_TEST;


//
//   Define Command Processing States and Options
//
#define HostCmd_STATE_IDLE                          0x0000
#define HostCmd_STATE_IN_USE_BY_HOST                0x0001
#define HostCmd_STATE_IN_USE_BY_MINIPORT            0x0002
#define HostCmd_STATE_FINISHED                      0x000f

#define HostCmd_Q_NONE                              0x0000
#define HostCmd_Q_INIT                              0x0001
#define HostCmd_Q_RESET                             0x0002
#define HostCmd_Q_STAT                              0x0003

//
//          Command pending states
//
#define HostCmd_PENDING_ON_NONE                     0x0000
#define HostCmd_PENDING_ON_MISC_OP                  0x0001
#define HostCmd_PENDING_ON_INIT                     0x0002
#define HostCmd_PENDING_ON_RESET                    0x0003
#define HostCmd_PENDING_ON_SET_OID                  0x0004
#define HostCmd_PENDING_ON_GET_OID                  0x0005
#define HostCmd_PENDING_ON_CMD                      0x0006
#define HostCmd_PENDING_ON_STAT                     0x0007

#define HostCmd_OPTION_USE_INT                      0x0000
#define HostCmd_OPTION_NO_INT                       0x0001

#define HostCmd_DELAY_NORMAL                        0x00000200  //  512 micro sec
#define HostCmd_DELAY_MIN                           0x00000100  //  256 micro sec
#define HostCmd_DELAY_MAX                           0x00000400  // 1024 micro sec

//***************************************************************************
//
//          16 bit host command code - HHH updated on 110201
//
#define HostCmd_CMD_NONE                            0x0000
#define HostCmd_CMD_CODE_DNLD                       0x0001
#define HostCmd_CMD_OP_PARAM_DNLD                   0x0002
#define HostCmd_CMD_GET_HW_SPEC                     0x0003
#define HostCmd_CMD_EEPROM_ACCESS                   0x0004
#define HostCmd_CMD_802_11_RESET                    0x0005
#define HostCmd_CMD_802_11_SCAN                     0x0006
#define HostCmd_CMD_802_11_QUERY_SCAN_RESULT        0x0029
#define HostCmd_CMD_802_11_QUERY_TRAFFIC            0x0009
#define HostCmd_CMD_802_11_QUERY_STATUS             0x000a
#define HostCmd_CMD_802_11_GET_LOG                  0x000b
#define HostCmd_CMD_MAC_CONTROL                     0x0028
#define HostCmd_CMD_MAC_MULTICAST_ADR               0x0010
#define HostCmd_CMD_802_11_AUTHENTICATE             0x0011
#define HostCmd_CMD_802_11_DEAUTHENTICATE           0x0024
#define HostCmd_CMD_802_11_ASSOCIATE                0x0012
#define HostCmd_CMD_802_11_REASSOCIATE              0x0025
#define HostCmd_CMD_802_11_DISASSOCIATE             0x0026
#define HostCmd_CMD_802_11_SET_WEP                  0x0013
#define HostCmd_CMD_802_11_GET_STAT                 0x0014
#define HostCmd_CMD_802_3_GET_STAT                  0x0015
#define HostCmd_CMD_802_11_SNMP_MIB                 0x0016
#define HostCmd_CMD_MAC_REG_MAP                     0x0017
#define HostCmd_CMD_BBP_REG_MAP                     0x0018
#define HostCmd_CMD_RF_REG_MAP                      0x0023
#define HostCmd_CMD_MAC_REG_ACCESS                  0x0019
#define HostCmd_CMD_BBP_REG_ACCESS                  0x001a
#define HostCmd_CMD_RF_REG_ACCESS                   0x001b
#define HostCmd_CMD_802_11_RADIO_CONTROL            0x001c
#define HostCmd_CMD_802_11_RF_CHANNEL               0x001d
#define HostCmd_CMD_802_11_RF_TX_POWER              0x001e
#define HostCmd_CMD_802_11_RSSI                     0x001f
#define HostCmd_CMD_802_11_RF_ANTENNA               0x0020
#define HostCmd_CMD_802_11_PS_MODE                  0x0021
#define HostCmd_CMD_802_11_DATA_RATE                0x0022
#define HostCmd_CMD_802_11_AD_HOC_START             0x002B
#define HostCmd_CMD_802_11_AD_HOC_JOIN              0x002C
//#define HostCmd_CMD_SET_ACTIVE_SCAN_SSID          0x002D

#define HostCmd_CMD_802_11_QUERY_RSN_OPTION         0x002d
#define HostCmd_CMD_802_11_QUERY_TKIP_REPLY_CNTRS   0x002e

#define HostCmd_CMD_MFG_COMMAND                     0x0040

#define HostCmd_CMD_802_11_AD_HOC_STOP              0x0040

#define HostCmd_CMD_802_11_DEEP_SLEEP               0x003E

#define HostCmd_CMD_802_11_HOST_SLEEP_CFG           0x0043
#define HostCmd_CMD_802_11_HOST_SLEEP_AWAKE_CONFIRM 0x0044
#define HostCmd_CMD_802_11_HOST_SLEEP_ACTIVATE      0x0045

// test mode
#define HostCmd_CMD_TEST_TX_MODE                    0x004A
#define HostCmd_CMD_TEST_RX_MODE                    0x004B //It's value is the same as TX_CONTROL_MODE (ref. linux driver)

// read or write region code
#define HostCmd_CMD_REGION_CODE                     0x004C  

// read or write MAC address
#define HostCmd_CMD_MAC_ADDRESS                     0x004D

// set LED Control
#define HostCmd_CMD_802_11_LED_CONTROL              0x004E

// Extended Associate CMD
#define HostCmd_CMD_802_11_ASSOCIATE_EXT            0x0050


#define HostCmd_CMD_802_11D_DOMAIN_INFO             0x005b

#define HostCmd_CMD_802_11_KEY_MATERIAL             0x005e

// BT mode configuration (value could change)
#define HostCmd_CMD_BCA_CONFIG                      0x0065

#define HostCmd_CMD_802_11_SLEEP_PARAMS             0x0066

#define HostCmd_CMD_802_11_INACTIVITY_TIMEOUT       0x0067

#define HostCmd_CMD_802_11_SLEEP_PERIOD             0x0068

#define HostCmd_CMD_802_11_BCA_CONFIG_TIMESHARE     0x0069

#define HostCmd_CMD_802_11_BG_SCAN_CONFIG           0x006b
#define HostCmd_CMD_802_11_BG_SCAN_QUERY            0x006c

#define HostCmd_CMD_802_11_CAL_DATA_EXT             0x006d

#define HostCmd_CMD_802_11_TPC_CFG                  0x0072
#define HostCmd_CMD_802_11_PWR_CFG                  0x0073

#define HostCmd_CMD_802_11_FW_WAKE_METHOD           0x0074  

#define HostCmd_CMD_802_11_RATE_ADAPT_RATESET       0x0076
#define HostCmd_CMD_802_11_SUBSCRIBE_EVENT          0x0075

#define HostCmd_CMD_802_11_WMM_ACK_POLICY           0x005C
#define HostCmd_CMD_802_11_WMM_GET_STATUS           0x0071

#define HostCmd_CMD_802_11_KEY_ENCRYPT              0x0054
#define HostCmd_CMD_802_11_CRYPTO                   0x0078

//dralee_20060601
#define HostCmd_CMD_802_11_IBSS_COALESING_STATUS    0x0083                    

//dralee 20061016

#define HostCmd_CMD_802_11_TX_RATE_QUERY            0x007f

#define HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT      0x007e

#define HostCmd_CMD_802_11_GET_TSF                  0x0080
#define HostCmd_CMD_802_11_WMM_ADDTS_REQ            0x006e
#define HostCmd_CMD_802_11_WMM_DELTS_REQ            0x006f
#define HostCmd_CMD_802_11_WMM_QUEUE_CONFIG         0x0070
#define HostCmd_CMD_802_11_WMM_QUEUE_STATS          0x0081






//***************************************************************************
//
//          16 bit RET code, MSB is set to 1
//
#define HostCmd_RET_NONE                            0x8000
#define HostCmd_RET_HW_SPEC_INFO                    0x8003
#define HostCmd_RET_EEPROM_ACCESS                   0x8004
#define HostCmd_RET_802_11_RESET                    0x8005
#define HostCmd_RET_802_11_SCAN                     0x8006
#define HostCmd_RET_802_11_QUERY_SCAN_RESULT        0x8029
#define HostCmd_RET_802_11_QUERY_TRAFFIC            0x8009
#define HostCmd_RET_802_11_STATUS_INFO              0x800a
#define HostCmd_RET_802_11_GET_LOG                  0x800b
#define HostCmd_RET_MAC_CONTROL                     0x8028
#define HostCmd_RET_MAC_MULTICAST_ADR               0x8010
#define HostCmd_RET_802_11_AUTHENTICATE             0x8011
#define HostCmd_RET_802_11_DEAUTHENTICATE           0x8024
#define HostCmd_RET_802_11_ASSOCIATE                0x8012
#define HostCmd_RET_802_11_REASSOCIATE              0x8025
#define HostCmd_RET_802_11_DISASSOCIATE             0x8026
#define HostCmd_RET_802_11_SET_WEP                  0x8013
#define HostCmd_RET_802_11_GET_STAT                 0x8014
#define HostCmd_RET_802_3_STAT                      0x8015
#define HostCmd_RET_802_11_SNMP_MIB                 0x8016
#define HostCmd_RET_MAC_REG_MAP                     0x8017
#define HostCmd_RET_BBP_REG_MAP                     0x8018
#define HostCmd_RET_RF_REG_MAP                      0x8023
#define HostCmd_RET_MAC_REG_ACCESS                  0x8019
#define HostCmd_RET_BBP_REG_ACCESS                  0x801a
#define HostCmd_RET_RF_REG_ACCESS                   0x801b
#define HostCmd_RET_802_11_RADIO_CONTROL            0x801c
#define HostCmd_RET_802_11_RF_CHANNEL               0x801d
#define HostCmd_RET_802_11_RSSI                     0x801f
#define HostCmd_RET_802_11_RF_TX_POWER              0x801e
#define HostCmd_RET_802_11_RF_ANTENNA               0x8020
#define HostCmd_RET_802_11_PS_MODE                  0x8021
#define HostCmd_RET_802_11_DATA_RATE                0x8022
#define HostCmd_RET_802_11_AD_HOC_START             0x802B
#define HostCmd_RET_802_11_AD_HOC_JOIN              0x802C
#define HostCmd_RET_802_11_QUERY_RSN_OPTION         0x802d
#define HostCmd_RET_802_11_QUERY_TKIP_REPLY_CNTRS   0x802e
#define HostCmd_RET_802_11_UNICAST_CIPHER           0x8031
#define HostCmd_RET_802_11_RSN_AUTH_SUITES          0x8032
#define HostCmd_RET_802_11_RSN_STATS                0x8033
#define HostCmd_RET_802_11_PWK_KEY                  0x8034
#define HostCmd_RET_802_11_GRP_KEY                  0x8035
#define HostCmd_RET_802_11_PAIRWISE_TSC             0x8036
#define HostCmd_RET_802_11_GROUP_TSC                0x8037

#define HostCmd_RET_802_11_ENABLE_QOS_WME           0x8038
#define HostCmd_RET_802_11_WME_AC_PARAMS            0x8039

#define HostCmd_RET_MFG_COMMAND                     0x8040

#define HostCmd_RET_802_11_AD_HOC_STOP              0x8040
#define HostCmd_RET_802_11_MCAST_CIPHER             0x803A

#define HostCmd_RET_802_11_DEEP_SLEEP               0x803E


#define HostCmd_RET_802_11_HOST_SLEEP_CFG               0x8043
#define HostCmd_RET_802_11_HOST_SLEEP_AWAKE_CONFIRM     0x8044
#define HostCmd_RET_CMD_802_11_HOST_SLEEP_ACTIVATE      0x8045

// test mode
#define HostCmd_RET_TEST_TX_MODE                    0x804A
#define HostCmd_RET_TEST_RX_MODE                    0x804B

#define HostCmd_RET_REGION_CODE                     0x804C
#define HostCmd_RET_MAC_ADDRESS                     0x804D
#define HostCmd_RET_802_11_LED_CONTROL              0x804E

// Extended Associate CMD
#define HostCmd_RET_802_11_ASSOCIATE_EXT            0x8012 
#define HostCmd_RET_BCA_CONFIG                      0x8065 

#define HostCmd_RET_802_11_CAL_DATA                 0x8057

#define HostCmd_RET_802_11_KEY_MATERIAL             0x805e

#define HostCmd_RET_802_11_SLEEP_PARAMS             0x8066

#define HostCmd_RET_802_11_INACTIVITY_TIMEOUT       0x8067
 
#define HostCmd_RET_802_11_BG_SCAN_CONFIG           0x806b
#define HostCmd_RET_802_11_BG_SCAN_QUERY            0x806c

#define HostCmd_RET_802_11_FW_WAKE_METHOD           0x8074

#define HostCmd_RET_802_11_WMM_ACK_POLICY           0x805C
#define HostCmd_RET_802_11_WMM_GET_STATUS           0x8071

#define HostCmd_RET_802_11_SLEEP_PERIOD             0x8068

#define HostCmd_RET_802_11_KEY_ENCRYPT              0x8054
#define HostCmd_RET_802_11_CRYPTO                   0x8078

#define HostCmd_RET_802_11D_DOMAIN_INFO             0x805b
 
#define HostCmd_RET_802_11_IBSS_COALESING_STATUS    0x8083                    

#define HostCmd_RET_802_11_TX_RATE_QUERY            0x807f

#define HostCmd_RET_802_11_POWER_ADAPT_CFG_EXT      0x807e
#define HostCmd_RET_802_11_GET_TSF                  0x8080
#define HostCmd_RET_802_11_WMM_ADDTS_REQ            0x806e
#define HostCmd_RET_802_11_WMM_DELTS_REQ            0x806f
#define HostCmd_RET_802_11_WMM_QUEUE_CONFIG         0x8070
#define HostCmd_RET_802_11_WMM_QUEUE_STATS          0x8081
                                                          




//***************************************************************************
// 
// Define general result code for each command
//
#define HostCmd_RESULT_OK                           0x0000 // OK
#define HostCmd_RESULT_ERROR                        0x0001 // Genenral error
#define HostCmd_RESULT_NOT_SUPPORT                  0x0002 // Command is not valid
#define HostCmd_RESULT_PENDING                      0x0003 // Command is pending (will be processed)
#define HostCmd_RESULT_BUSY                         0x0004 // System is busy (command ignored)
#define HostCmd_RESULT_PARTIAL_DATA                 0x0005 // Data buffer is not big enough


//***************************************************************************
// 
// Definition of action or option for each command
//
// Define general purpose action
//
#define HostCmd_ACT_GEN_READ                        0x0000
#define HostCmd_ACT_GEN_WRITE                       0x0001
#define HostCmd_ACT_GEN_GET                         0x0000
#define HostCmd_ACT_GEN_SET                         0x0001
#define HostCmd_ACT_GEN_OFF                         0x0000
#define HostCmd_ACT_GEN_ON                          0x0001

// Define action or option for HostCmd_CMD_802_11_AUTHENTICATE
#define HostCmd_ACT_AUTHENTICATE                    0x0001
#define HostCmd_ACT_DEAUTHENTICATE                  0x0002

// Define action or option for HostCmd_CMD_802_11_ASSOCIATE
#define HostCmd_ACT_ASSOCIATE                       0x0001
#define HostCmd_ACT_DISASSOCIATE                    0x0002
#define HostCmd_ACT_REASSOCIATE                     0x0003

#define HostCmd_CAPINFO_ESS                         0x0001
#define HostCmd_CAPINFO_IBSS                        0x0002
#define HostCmd_CAPINFO_CF_POLLABLE                 0x0004
#define HostCmd_CAPINFO_CF_REQUEST                  0x0008
#define HostCmd_CAPINFO_PRIVACY                     0x0010
#define HostCmd_CAPINFO_SHORT_PREAMBLE              0x0020
#define HostCmd_CAPINFO_SHORT_SLOT_TIME             0x0400

#define HostCmd_ACT_ADD                             0x0002
#define HostCmd_ACT_REMOVE                          0x0004
#define HostCmd_ACT_USE_DEFAULT                     0x0008

#define HostCmd_TYPE_WEP_40_BIT                     0x0001 // 40 bit
#define HostCmd_TYPE_WEP_104_BIT                    0x0002 // 104 bit
#define HostCmd_TYPE_WEP_128_BIT                    0x0003 // 128 bit
#define HostCmd_TYPE_WEP_TX_KEY                     0x0004 // TX WEP

#define HostCmd_NUM_OF_WEP_KEYS                     4

#define HostCmd_WEP_KEY_INDEX_MASK                  0x3fffffff
#define HostCmd_CCKM_KEY_INDEX_MASK                 0x1fffffff

#define WEP_KEY_40_BIT_LEN                          5
#define WEP_KEY_104_BIT_LEN                         13


// Define action or option for HostCmd_CMD_802_11_RESET
#define HostCmd_ACT_NOT_REVERT_MIB                  0x0001
#define HostCmd_ACT_REVERT_MIB                      0x0002
#define HostCmd_ACT_HALT                            0x0003

//dralee 20060905 FW implementation is different with FW spec.
//modify back to old setting to fit FW implementation.
/*
// Define action or option for HostCmd_CMD_802_11_SCAN
///crlo: According to firmware specification v8, 
///     0x01=BSS_IDPENDENT
///     0x02=BSS_INFRASTRUCTURE
///     0x03=BSS_ANY
///     Our old implementation seems to be incorrect.
#define HostCmd_BSS_TYPE_IBSS                       0x0001
#define HostCmd_BSS_TYPE_BSS                        0x0002
#define HostCmd_BSS_TYPE_ANY                        0x0003
*/
#define HostCmd_BSS_TYPE_BSS                        0x0001
#define HostCmd_BSS_TYPE_IBSS                       0x0002
#define HostCmd_BSS_TYPE_ANY                        0x0003

// Define action or option for HostCmd_CMD_802_11_SCAN
#define HostCmd_SCAN_TYPE_ACTIVE                    0x0000
#define HostCmd_SCAN_TYPE_PASSIVE                   0x0001

#define HostCmd_SCAN_MIN_CH_TIME                    100  //60
#define HostCmd_SCAN_MAX_CH_TIME                    100  //60

#define HostCmd_SCAN_PROBE_DELAY_TIME               0

// Define action or option for HostCmd_CMD_802_11_QUERY_STATUS
#define HostCmd_STATUS_FW_INIT                      0x0000
#define HostCmd_STATUS_FW_IDLE                      0x0001
#define HostCmd_STATUS_FW_WORKING                   0x0002
#define HostCmd_STATUS_FW_ERROR                     0x0003
#define HostCmd_STATUS_FW_POWER_SAVE                0x0004

#define HostCmd_STATUS_MAC_RX_ON                    0x0001
#define HostCmd_STATUS_MAC_TX_ON                    0x0002
#define HostCmd_STATUS_MAC_LOOP_BACK_ON             0x0004
#define HostCmd_STATUS_MAC_WEP_ENABLE               0x0008
#define HostCmd_STATUS_MAC_INT_ENABLE               0x0010

// Define action or option for HostCmd_CMD_MAC_CONTROL 
#define HostCmd_ACT_MAC_RX_ON                       0x0001
#define HostCmd_ACT_MAC_TX_ON                       0x0002
#define HostCmd_ACT_MAC_LOOPBACK_ON                 0x0004
#define HostCmd_ACT_MAC_WEP_ENABLE                  0x0008
#define HostCmd_ACT_MAC_FRAME_TYPE                  0x0010
// #define HostCmd_ACT_MAC_INT_ENABLE               0x0010 // v8, v9 firmware doesn't need this anymore.
#define HostCmd_ACT_MAC_MULTICAST_ENABLE            0x0020
#define HostCmd_ACT_MAC_BROADCAST_ENABLE            0x0040
#define HostCmd_ACT_MAC_PROMISCUOUS_ENABLE          0x0080
#define HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE        0x0100
#define HostCmd_ACT_MAC_WMM_ENABLE                  0x0800
#define HostCmd_ACT_MAC_STRICT_PROTECTION_ENABLE    0x0400
#define HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON       0x2000

//BugFix
#define HostCmd_ACT_MAC_WEP_TYPE                    0x8000
// Define action or option or constant for HostCmd_CMD_MAC_MULTICAST_ADR
#define HostCmd_SIZE_MAC_ADR                        6
#define HostCmd_MAX_MCAST_ADRS                      32

// Define action or option for HostCmd_CMD_802_11_SNMP_MIB 
#define HostCmd_TYPE_MIB_FLD_BOOLEAN                0x0001 // Boolean
#define HostCmd_TYPE_MIB_FLD_INTEGER                0x0002 // 32 UCHAR unsigned integer
#define HostCmd_TYPE_MIB_FLD_COUNTER                0x0003 // Counter
#define HostCmd_TYPE_MIB_FLD_OCT_STR                0x0004 // Octet string
#define HostCmd_TYPE_MIB_FLD_DISPLAY_STR            0x0005 // String
#define HostCmd_TYPE_MIB_FLD_MAC_ADR                0x0006 // MAC address
#define HostCmd_TYPE_MIB_FLD_IP_ADR                 0x0007 // IP address
#define HostCmd_TYPE_MIB_FLD_WEP                    0x0008 // WEP


// Define action or option for HostCmd_CMD_802_11_RADIO_CONTROL 
#define HostCmd_TYPE_AUTO_PREAMBLE                  (0x0002 << 1)
#define HostCmd_TYPE_SHORT_PREAMBLE                 (0x0001 << 1)
#define HostCmd_TYPE_LONG_PREAMBLE                  (0x0000 << 1)

#define TURN_ON_RF                                  0x01

// Specific for CF
#define SET_AUTO_PREAMBLE                           0x05
#define SET_SHORT_PREAMBLE                          0x03
#define SET_LONG_PREAMBLE                           0x01

// Define action or option for CMD_802_11_RF_CHANNEL
#define HostCmd_TYPE_802_11A                        0x0001
#define HostCmd_TYPE_802_11B                        0x0002

// Define action or option for HostCmd_CMD_802_11_RF_TX_POWER
#define HostCmd_ACT_TX_POWER_OPT_GET                0x0000
#define HostCmd_ACT_TX_POWER_OPT_SET                0x0001
#define HostCmd_ACT_TX_POWER_OPT_SET_HIGH           0x8007
#define HostCmd_ACT_TX_POWER_OPT_SET_MID            0x8004
#define HostCmd_ACT_TX_POWER_OPT_SET_LOW            0x8000
#define HostCmd_ACT_TX_POWER_OPT_SET_TPC            0x8fff //Auto Power

#define HostCmd_ACT_TX_POWER_INDEX_TPC              0xffff

#define HostCmd_ACT_TX_POWER_INDEX_HIGH             0x0007
#define HostCmd_ACT_TX_POWER_INDEX_MID              0x0004
#define HostCmd_ACT_TX_POWER_INDEX_LOW              0x0000

#define HostCmd_ACT_TX_POWER_LEVEL_MIN              0x000e // in dbm
#define HostCmd_ACT_TX_POWER_LEVEL_GAP              0x0001 // in dbm

// Define action or option for HostCmd_CMD_802_11_DATA_RATE 
#define HostCmd_ACT_SET_TX_AUTO                     0x0000
#define HostCmd_ACT_SET_TX_FIX_RATE                 0x0001
#define HostCmd_ACT_GET_TX_RATE                     0x0002

#define HostCmd_ACT_SET_RX                          0x0001
#define HostCmd_ACT_SET_TX                          0x0002
#define HostCmd_ACT_SET_BOTH                        0x0003
#define HostCmd_ACT_GET_RX                          0x0004
#define HostCmd_ACT_GET_TX                          0x0008
#define HostCmd_ACT_GET_BOTH                        0x000c

// TODO: Need to verify these values
// command option for QOS commands
#define HostCmd_ACT_SET                             0x0001
#define HostCmd_ACT_GET                             0x0000

#define HostCmd_ACT_GET_AES                         0x0002
#define HostCmd_ACT_SET_AES                         0x0003
#define HostCmd_ACT_REMOVE_AES                      0x0004


#define TYPE_ANTENNA_DIVERSITY                      0xffff

// Define action or option for HostCmd_CMD_802_11_PS_MODE 
#define HostCmd_TYPE_CAM                            0x0000
#define HostCmd_TYPE_MAX_PSP                        0x0001
#define HostCmd_TYPE_FAST_PSP                       0x0002

#define HostCmd_SubCmd_Enter_PS                     0x0030
#define HostCmd_SubCmd_Exit_PS                      0x0031
#define HostCmd_SubCmd_TxPend_PS                    0x0032
#define HostCmd_SubCmd_ChangeSet_PS                 0x0033
#define HostCmd_SubCmd_Sleep_Confirmed              0x0034
#define HostCmd_SubCmd_Full_PowerDown               0x0035
#define HostCmd_SubCmd_Full_PowerUp                 0x0036

// notify FW that there's no pending TX pkt
#define HostCmd_SubCmd_No_Tx_Pkt                    0x0037  

// action for CMD_EEPROM_ACCESS
#define HostCmd_ACT_READ                            0x0000
#define HostCmd_ACT_WRITE                           0x0001


#define HostCmd_SCAN_RADIO_TYPE_BG                  0
#define HostCmd_SCAN_RADIO_TYPE_A                   1
#define HostCmd_SCAN_PASSIVE_MAX_CH_TIME            100


#define TLV_TYPE_SSID                               0x0000
#define TLV_TYPE_RATES                              0x0001
#define TLV_TYPE_PHY_FH                             0x0002
#define TLV_TYPE_PHY_DS                             0x0003
#define TLV_TYPE_CF                                 0x0004
#define TLV_TYPE_IBSS                               0x0006
#define TLV_TYPE_DOMAIN                             0x0007
#define TLV_TYPE_POWER_CAPABILITY                   0x0021


/** TLV  type ID definition */

#define PROPRIETARY_TLV_BASE_ID     0x0100
#define TLV_TYPE_KEY_MATERIAL       (PROPRIETARY_TLV_BASE_ID + 0)
#define TLV_TYPE_CHANLIST           (PROPRIETARY_TLV_BASE_ID + 1)
#define TLV_TYPE_NUMPROBES          (PROPRIETARY_TLV_BASE_ID + 2)
#define TLV_TYPE_RSSI               (PROPRIETARY_TLV_BASE_ID + 4)
#define TLV_TYPE_SNR                (PROPRIETARY_TLV_BASE_ID + 5)
#define TLV_TYPE_FAILCOUNT          (PROPRIETARY_TLV_BASE_ID + 6)
#define TLV_TYPE_BCNMISS            (PROPRIETARY_TLV_BASE_ID + 7)
#define TLV_TYPE_LED_GPIO           (PROPRIETARY_TLV_BASE_ID + 8)
#define TLV_TYPE_LEDBEHAVIOR        (PROPRIETARY_TLV_BASE_ID + 9)
#define TLV_TYPE_PASSTHROUGH        (PROPRIETARY_TLV_BASE_ID + 10)
#define TLV_TYPE_REASSOCAP          (PROPRIETARY_TLV_BASE_ID + 11)
#define TLV_TYPE_POWER_TBL_2_4GHZ   (PROPRIETARY_TLV_BASE_ID + 12)
#define TLV_TYPE_POWER_TBL_5GHZ     (PROPRIETARY_TLV_BASE_ID + 13)
#define TLV_TYPE_BCASTPROBE         (PROPRIETARY_TLV_BASE_ID + 14)
#define TLV_TYPE_NUMSSID_PROBE      (PROPRIETARY_TLV_BASE_ID + 15) 
#define TLV_TYPE_CRYPTO_DATA        (PROPRIETARY_TLV_BASE_ID + 17)

#define TLV_TYPE_RSSI_HIGH          (PROPRIETARY_TLV_BASE_ID + 22) 
#define TLV_TYPE_SNR_HIGH           (PROPRIETARY_TLV_BASE_ID + 23) 
#define TLV_TYPE_TSFTIMESTAMP       (PROPRIETARY_TLV_BASE_ID + 19)
#define TLV_TYPE_AUTH_TYPE          (PROPRIETARY_TLV_BASE_ID + 31)



#define  RSN_IE_AES         1
#define  RSN_IE_TKIP        2


//////////////////////////////////////////////////////////////////
//
//  IEEE scan.confirm data structure borrowed from station FW code
//  Reference: IEEE 802.11 1999 Spec 10.3.2.2.2
//
//////////////////////////////////////////////////////////////////

#pragma pack(1)

// In station FW code, enum type is 1 byte UCHAR, 
typedef enum
{
	SSID = 0,
	SUPPORTED_RATES,
	FH_PARAM_SET,
	DS_PARAM_SET,
	CF_PARAM_SET,
	TIM,
	IBSS_PARAM_SET,

	COUNTRY_INFO = 7,
	CHALLENGE_TEXT = 16,
	POWER_CONSTRAINT = 32,
	POWER_CAPABILITY = 33,
	TPC_REQUEST  = 34,
	TPC_REPORT   = 35,
	SUPPORTED_CHANNELS = 36,
	CHANNEL_SWITCH_ANN = 37,
	QUIET              = 40,
	IBSS_DFS           = 41,
	ERP_INFO = 42,
	EXTENDED_SUPPORTED_RATES = 50,
	VENDOR_SPECIFIC_221 = 221,
	RM_CAPABILITY = 221,            ///ref: S36.6 of CCX specification v2.8
	WPA_IE = 221,
	WPA_OUI_TYPE=2,
	WPA2_IE = 48,
	EXTRA_IE = 133,
	WMM_OUI_TYPE=2
} IEEEtypes_ElementId_e;

typedef struct IEEEtypes_CapInfo_t
{
   USHORT Ess:1;
   USHORT Ibss:1;
   USHORT CfPollable:1;
   USHORT CfPollRqst:1;
   USHORT Privacy:1;
   USHORT ShortPreamble:1;
   USHORT Pbcc:1;
   USHORT ChanAgility:1 ;
    USHORT  SpectrumMgmt:1;
    USHORT  QoS:1;
   USHORT ShortSlotTime:1;
   USHORT Rsrvd1112:2;
   USHORT DSSSOFDM:1;
   USHORT Rsrvd1415:2;
} IEEEtypes_CapInfo_t;

#pragma pack(1)
typedef struct _MrvlIEtypesHeader {
    USHORT  Type;
    USHORT  Len;
} MrvlIEtypesHeader_t;

//
// additional TLV data structures for background scan
//
typedef struct _MrvlIEtypes_BcastProbe_t 
{
    MrvlIEtypesHeader_t Header;
    USHORT              BcastProbe;
} MrvlIEtypes_BcastProbe_t;

typedef struct _MrvlIEtypes_NumSSIDProbe_t 
{
    MrvlIEtypesHeader_t Header;
    USHORT          NumSSIDProbe;
} MrvlIEtypes_NumSSIDProbe_t;




typedef enum {
    DISABLE_11D = 0,
    ENABLE_11D  = 1,
} state_11d_t;

typedef enum {
  Wlan802_11AuthModeOpen = 0x00,
  Wlan802_11AuthModeShared = 0x01,
  Wlan802_11AuthModeNetworkEAP = 0x80
} WLAN_802_11_AUTHENTICATION_MODE;  

// Data structure for Country IE
typedef struct _IEEEtypes_SubbandSet 
{
    UCHAR   FirstChan;
    UCHAR   NoOfChan;
    UCHAR   MaxTxPwr;   

} IEEEtypes_SubbandSet_t;

typedef struct _IEEEtypes_CountryInfoSet 
{
    UCHAR   ElementId;
    UCHAR   Len;
    UCHAR   CountryCode[COUNTRY_CODE_LEN];
    IEEEtypes_SubbandSet_t  Subband[1];

} IEEEtypes_CountryInfoSet_t;

typedef struct _IEEEtypes_CountryInfoFullSet 
{
    UCHAR   ElementId;
    UCHAR   Len;
    UCHAR   CountryCode[COUNTRY_CODE_LEN];
    IEEEtypes_SubbandSet_t  Subband[MRVDRV_MAX_SUBBAND_802_11D];
}  IEEEtypes_CountryInfoFullSet_t;

typedef struct _IEEEtypes_ERPInfo_t {
    UCHAR   ElementId;
    UCHAR   Len;
    UCHAR   ERPFlags;
} IEEEtypes_ERPInfo_t;

typedef struct _MrvlIEtypes_DomainParamSet 
{
    MrvlIEtypesHeader_t Header;
    UCHAR               CountryCode[COUNTRY_CODE_LEN];
    //dralee_20060502
    IEEEtypes_SubbandSet_t  Subband[MRVDRV_MAX_SUBBAND_802_11D];
    //IEEEtypes_SubbandSet_t    Subband[1];
} MrvlIEtypes_DomainParamSet_t;

// Define data structure for HostCmd_CMD_802_11D_DOMAIN_INFO 
typedef struct _HostCmd_DS_802_11D_DOMAIN_INFO 
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;

    USHORT  Action;
    MrvlIEtypes_DomainParamSet_t Domain;

}  HostCmd_DS_802_11D_DOMAIN_INFO, *PHostCmd_DS_802_11D_DOMAIN_INFO;

// Define data structure for HostCmd_RET_802_11D_DOMAIN_INFO 
typedef struct _HostCmd_DS_802_11D_DOMAIN_INFO_RSP 
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result; 

    USHORT  Action;
    MrvlIEtypes_DomainParamSet_t Domain;

}  HostCmd_DS_802_11D_DOMAIN_INFO_RSP, *PHostCmd_DS_802_11D_DOMIAN_INFO_RSP;




typedef union _KeyInfo_WEP_t 
{
    UCHAR   Reserved;           // bits 5-15: Reserved 
    UCHAR   WepKeyIndex;        // bits 1-4: Specifies the index of key 
    UCHAR   isWepDefaultKey;    // bit 0: Specifies that this key is to be used as the default for TX data packets 
}  KeyInfo_WEP_t;

typedef union _KeyInfo_TKIP_t 
{
    UCHAR   Reserved;           // bits 3-15: Reserved 
    UCHAR   isKeyEnabled;       // bit 2: Specifies that this key is enabled and valid to use 
    UCHAR   isUnicastKey;       // bit 1: Specifies that this key is to be used as the unicast key 
    UCHAR   isMulticastKey;     // bit 0: Specifies that this key is to be used as the multicast key 
}  KeyInfo_TKIP_t;

typedef union _KeyInfo_AES_t
{
    UCHAR   Reserved;           // bits 3-15: Reserved */
    UCHAR   isKeyEnabled;       // bit 2: Specifies that this key is enabled and valid to use 
    UCHAR   isUnicastKey;       // bit 1: Specifies that this key is to be used as the unicast key 
    UCHAR   isMulticastKey;     // bit 0: Specifies that this key is to be used as the multicast key 
}  KeyInfo_AES_t;

typedef struct _KeyMaterial_TKIP_t 
{
    UCHAR   TkipKey[16];        // TKIP encryption/decryption key 
    UCHAR   TkipTxMicKey[8];    // TKIP TX MIC Key 
    UCHAR   TkipRxMicKey[8];    // TKIP RX MIC Key 
}  KeyMaterial_TKIP_t, *PKeyMaterial_TKIP_t;

typedef struct _KeyMaterial_AES_t 
{
    UCHAR   AesKey[16];         // AES encryption/decryption key 
}  KeyMaterial_AES_t, *PKeyMaterial_AES_t;

typedef struct _MrvlIEtype_KeyParamSet_t 
{
    USHORT  Type;       // Type ID 
    USHORT  Length;     // Length of Payload 
    USHORT  KeyTypeId;  // Type of Key: WEP=0, TKIP=1, AES=2 
    USHORT  KeyInfo;    // Key Control Info specific to a KeyTypeId 
    USHORT  KeyLen;     // Length of key 
    UCHAR   Key[32];    // Key material of size KeyLen 
}  MrvlIEtype_KeyParamSet_t, *PMrvlIEtype_KeyParamSet_t;

typedef struct _HostCmd_DS_802_11_KEY_MATERIAL 
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     

    MrvlIEtype_KeyParamSet_t    KeyParamSet;    
    
}  HostCmd_DS_802_11_KEY_MATERIAL, *PHostCmd_DS_802_11_KEY_MATERIAL;


typedef struct IEEEtypes_CfParamSet_t
{
   UCHAR  ElementId;
   UCHAR  Len;
   UCHAR  CfpCnt;
   UCHAR  CfpPeriod;
   USHORT CfpMaxDuration;
   USHORT CfpDurationRemaining;
} IEEEtypes_CfParamSet_t;

typedef struct IEEEtypes_IbssParamSet_t
{
   UCHAR  ElementId;
   UCHAR  Len;
   USHORT AtimWindow;
} IEEEtypes_IbssParamSet_t;

typedef union IEEEtypes_SsParamSet_t
{
   IEEEtypes_CfParamSet_t    CfParamSet;
   IEEEtypes_IbssParamSet_t  IbssParamSet;
} IEEEtypes_SsParamSet_t;

typedef struct IEEEtypes_FhParamSet_t
{
   UCHAR  ElementId;
   UCHAR  Len;
   USHORT DwellTime;
   UCHAR  HopSet;
   UCHAR  HopPattern;
   UCHAR  HopIndex;
} IEEEtypes_FhParamSet_t;

typedef struct IEEEtypes_DsParamSet_t
{
   UCHAR ElementId;
   UCHAR Len;
   UCHAR CurrentChan;
} IEEEtypes_DsParamSet_t;

typedef union IEEEtypes_PhyParamSet_t
{
   IEEEtypes_FhParamSet_t  FhParamSet;
   IEEEtypes_DsParamSet_t  DsParamSet;
} IEEEtypes_PhyParamSet_t;


/* Data structure for WPS information */
typedef struct _WPS_IE_INFO {
    UCHAR      Wps_ie[256];
    UCHAR      Wps_ie_len;  
} WPS_IE_INFO, *PWPS_IE_INFO;

typedef struct _BSS_DESCRIPTION_SET_ALL_FIELDS
{
    UCHAR                     BSSID[MRVDRV_ETH_ADDR_LEN];
    UCHAR                     SSID[MRVDRV_MAX_SSID_LENGTH];
    UCHAR                     BSSType;
    USHORT                    BeaconPeriod;
    UCHAR                     DTIMPeriod;
    UCHAR                     TimeStamp[8];
    UCHAR                     LocalTime[8];
    IEEEtypes_PhyParamSet_t   PhyParamSet;
    IEEEtypes_SsParamSet_t    SsParamSet;
    IEEEtypes_CapInfo_t       Cap;
    UCHAR                     DataRates[NDIS_SUPPORTED_RATES];
    USHORT                   bss_band;  
    UCHAR                    bHaveCountryIE;
    IEEEtypes_CountryInfoFullSet_t CountryInfo;
    WPA_SUPPLICANT           wpa_supplicant;
    ULONGLONG                     networkTSF;
    WPA_SUPPLICANT           wpa2_supplicant;
    UCHAR Wmm_IE[WMM_PARA_IE_LENGTH + 2];
    UCHAR Wmm_ie_len;
    CCX_BSS_Info_t		ccx_bss_info;
    WPS_IE_INFO     wps;
    
    UCHAR       ERPFlags;
    
} BSS_DESCRIPTION_SET_ALL_FIELDS, *PBSS_DESCRIPTION_SET_ALL_FIELDS;


typedef struct _BSS_DESCRIPTION_SET_FIXED_FIELDS {
    UCHAR BSSID[MRVDRV_ETH_ADDR_LEN];
    UCHAR SSID[MRVDRV_MAX_SSID_LENGTH];
    UCHAR BSSType;
    USHORT BeaconPeriod;
    UCHAR DTIMPeriod;
    UCHAR TimeStamp[8];
    UCHAR LocalTime[8];
} BSS_DESCRIPTION_SET_FIXED_FIELDS, *PBSS_DESCRIPTION_SET_FIXED_FIELDS;

#pragma pack()

//=============================================================================
//          HOST COMMAND DEFINITIONS
//=============================================================================

#pragma pack(1)
//
// Definition of data structure for each command
//
// Define general data structure
typedef struct _HostCmd_DS_GEN {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
} HostCmd_DS_GEN, *PHostCmd_DS_GEN;


#define HostCmd_DS_802_11_DEEP_SLEEP        HostCmd_DS_GEN

  

//dralee_20060605
#define MAX_HOST_SLEEP_WAKEUP_FILETER_ENTRY  20
#define Host_Sleep_Filter_Type 0x0115
 
typedef struct _WakeUpFilterEntry 
{
    USHORT     AddressType; 
    USHORT     EthType; 
    ULONG      Ipv4Addr;
} WakeUpFilterEntry, *PWakeUpFilterEntry;

typedef struct _HostCmd_DS_HOST_WAKEUP_FILTER
{
      MrvlIEtypesHeader_t   Header;  
      WakeUpFilterEntry     entry[MAX_HOST_SLEEP_WAKEUP_FILETER_ENTRY];
} HostCmd_DS_HOST_WAKEUP_FILTER, *PHostCmd_DS_HOST_WAKEUP_FILTER;

typedef struct _HostCmd_DS_HOST_SLEEP {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    ULONG  Criteria;
    UCHAR  GPIO;
    UCHAR  Gap;
    HostCmd_DS_HOST_WAKEUP_FILTER Filter;
} HostCmd_DS_HOST_SLEEP, *PHostCmd_DS_HOST_SLEEP;

#define HostCmd_DS_WAKEUP_CONFIRM       HostCmd_DS_GEN

#define HostCmd_DS_WAKEUP_CONFIRM       HostCmd_DS_GEN


// Define data structure for HostCmd_CMD_CODE_DNLD
typedef struct _HostCmd_DS_CODE_DNLD {
    USHORT  Len;
    USHORT  Type;
  UCHAR Code[4096];
} HostCmd_DS_CODE_DNLD, *PHostCmd_DS_CODE_DNLD;


// Define data structure for HostCmd_CMD_OP_PARAM_DNLD
typedef struct _HostCmd_DS_OP_PARAM_DNLD {
    UCHAR OpParam[4096];
} HostCmd_DS_OP_PARAM_DNLD, *PHostCmd_DS_OP_PARAM_DNLD;

#define MRVDRV_FW_CAPINFO_WPA  (1 << 0)
#define MRVDRV_FW_CAPINFO_PS   (1 << 1)
#define MRVDRV_FW_CAPINFO_GM   (1 << 2)



// Define data structure for HostCmd_CMD_GET_HW_SPEC
typedef struct _HostCmd_DS_GET_HW_SPEC {
    USHORT Command;                 // Command number
    USHORT Size;                    // Size of the data structure
    USHORT SeqNum;                  // Command sequence number
    USHORT Result;                  // Result code

    USHORT  HWIfVersion;            // version of the hardware interface

    USHORT  Version;                 // HW version
    USHORT  NumOfWCB;                // Max. number of WCB FW can handle
    USHORT  NumOfMCastAdr;           // Max. number of Multicast address FW can handle
    UCHAR   PermanentAddr[6];        // MAC address
    USHORT  RegionCode;              // Region Code
    USHORT  NumberOfAntenna;         // Number of antenna used
    ULONG   FWReleaseNumber;         // 4 byte of FW release number, example 0x1234=1.2.3.4
    ULONG   WcbBase;
    ULONG   RxPdRdPtr;
    ULONG   RxPdWrPtr;
    ULONG   fwCapInfo;              /* Firmware Capability Info */
} HostCmd_DS_GET_HW_SPEC, *PHostCmd_DS_GET_HW_SPEC;

// Define data structure for HostCmd_CMD_EEPROM_UPDATE
typedef struct _HostCmd_DS_EEPROM_ACCESS {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;                  // Detailed action or option
    USHORT  DataLength;
    ULONG   Address;                 // target address
    UCHAR   Data[1];
} HostCmd_DS_EEPROM_ACCESS, *PHostCmd_DS_EEPROM_ACCESS;

// Define data structure for HostCmd_CMD_802_11_RESET
typedef struct _HostCmd_DS_802_11_RESET {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
} HostCmd_DS_802_11_RESET, *PHostCmd_DS_802_11_RESET;

typedef struct _HostCmd_DS_802_11_BG_SCAN_CONFIG 
{
        USHORT      Command;
        USHORT      Size;
        USHORT      SeqNum;
        USHORT      Result;
    
        USHORT      Action;
    
    // Enable 
    // 0 - Disable 1 - Enable 
    UCHAR       Enable; 
    
    // bssType 
    //  1 - Infrastructure
    //  2 - IBSS
    //  3 - any 
    UCHAR       BssType;
    
    // ChannelsPerScan 
    // No of channels to scan at one scan 
    UCHAR       ChannelsPerScan;
    
    // 0 - Discard old scan results
    // 1 - Discard new scan results 
    UCHAR       DiscardWhenFull;
    
    USHORT      Reserved;
    
    // ScanInterval 
    ULONG       ScanInterval;
    
    // StoreCondition 
    // - SSID Match
    // - Exceed RSSI threshold
    // - SSID Match & Exceed RSSI Threshold 
    // - Always 
    ULONG       StoreCondition; 
    
    // ReportConditions 
    // - SSID Match
    // - Exceed RSSI threshold
    // - SSID Match & Exceed RSSIThreshold
    // - Exceed MaxScanResults
    // - Entire channel list scanned once 
    // - Domain Mismatch in country IE 
    //
    ULONG       ReportConditions;   

    // MaxScanResults 
    // Max scan results that will trigger 
    // a scn completion event 
    USHORT      MaxScanResults;
    
    //  attach TLV based parameters as needed, e.g.
    //  MrvlIEtypes_SsIdParamSet_t  SsIdParamSet;
    //  MrvlIEtypes_ChanListParamSet_t  ChanListParamSet;
    //  MrvlIEtypes_NumProbes_t     NumProbes;
    //
    UCHAR           TlvData[1]; 
}  HostCmd_DS_802_11_BG_SCAN_CONFIG, *PHostCmd_DS_802_11_BG_SCAN_CONFIG;

/** HostCmd_DS_802_11_BG_SCAN_QUERY */
typedef struct _HostCmd_DS_802_11_BG_SCAN_QUERY 
{
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    UCHAR           Flush;
} HostCmd_DS_802_11_BG_SCAN_QUERY, *PHostCmd_DS_802_11_BG_SCAN_QUERY;

/** HostCmd_DS_802_11_BG_SCAN_QUERY_RSP */
typedef struct _HostCmd_DS_802_11_BG_SCAN_QUERY_RSP 
{
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    ULONG           ReportCondition;
    USHORT      BSSDescriptSize;
    UCHAR       NumberOfSets;
    //UCHAR           BssDescAndTlvBuffer[1]; 022607 dralee
} HostCmd_DS_802_11_BG_SCAN_QUERY_RSP, *PHostCmd_DS_802_11_BG_SCAN_QUERY_RSP;

typedef struct  _MrvlIEtypes_RatesParamSet_t {
    MrvlIEtypesHeader_t Header;
    UCHAR   Rates[1];
} MrvlIEtypes_RatesParamSet_t;

typedef struct  _MrvlIEtypes_SsIdParamSet_t {
    MrvlIEtypesHeader_t Header;
    UCHAR   SsId[1];
} MrvlIEtypes_SsIdParamSet_t;

typedef struct _ChanScanParamSet_t {
    UCHAR   RadioType;
    UCHAR   ChanNumber;
    UCHAR   ScanType;
    USHORT  MinScanTime;
    USHORT  ScanTime;
} ChanScanParamSet_t;

typedef struct  _MrvlIEtypes_ChanListParamSet_t {
    MrvlIEtypesHeader_t Header;
    ChanScanParamSet_t  *ChanScanParam;
} MrvlIEtypes_ChanListParamSet_t; 

//ahan [2005-12-09], TLV for Number of Probes
typedef struct  _MrvlIEtypes_NumProbes_t {
        MrvlIEtypesHeader_t Header;
        USHORT              NumProbes;
} MrvlIEtypes_NumProbes_t;
//ahan [2005-12-09]


typedef struct _HostCmd_DS_802_11_SCAN {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    UCHAR   BSSType;
    UCHAR   BSSID[MRVDRV_ETH_ADDR_LEN];
    MrvlIEtypes_SsIdParamSet_t      SsIdParamSet;
    MrvlIEtypes_ChanListParamSet_t  ChanListParamSet;
    MrvlIEtypes_RatesParamSet_t     OpRateSet;
} HostCmd_DS_802_11_SCAN, *PHostCmd_DS_802_11_SCAN;

typedef struct _HostCmd_DS_802_11_SCAN_RSP {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT BSSDescriptSize;
    UCHAR  NumberOfSets;
} HostCmd_DS_802_11_SCAN_RSP, *PHostCmd_DS_802_11_SCAN_RSP;


// Define data structure for HostCmd_CMD_802_11_QUERY_TRAFFIC
typedef struct _HostCmd_DS_802_11_QUERY_TRAFFIC {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    ULONG Traffic;                  // Traffic in bps
} HostCmd_DS_802_11_QUERY_TRAFFIC, *PHostCmd_DS_802_11_QUERY_TRAFFIC;

// Define data structure for HostCmd_CMD_802_11_QUERY_STATUS
typedef struct _HostCmd_DS_802_11_QUERY_STATUS {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT FWStatus;
    USHORT MACStatus;
    USHORT RFStatus;
    USHORT CurentChannel;           // 1..99
    UCHAR APMACAdr[6];              // Associated AP MAC address
    USHORT Reserved;
    ULONG MaxLinkSpeed;             // Allowable max.link speed in unit of 100bps
} HostCmd_DS_802_11_QUERY_STATUS, *PHostCmd_DS_802_11_QUERY_STATUS;

//dralee_20060509
typedef struct _Mrvl_802_11GetLog_t
{
    ULONG   mcasttxframe;
    ULONG   failed;
    ULONG   retry;
    ULONG   multiretry;
    ULONG   framedup;
    ULONG   rtssuccess;
    ULONG   rtsfailure;
    ULONG   ackfailure;
    ULONG   rxfrag;
    ULONG   mcastrxframe;
    ULONG   fcserror;
    ULONG   txframe;
    ULONG   wepundecryptable;
} Mrvl_802_11GetLog_t;

// Define data structure for HostCmd_CMD_802_11_GET_LOG
typedef struct _HostCmd_DS_802_11_GET_LOG {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    //dralee_20060509
    //UCHAR Log[512];
    Mrvl_802_11GetLog_t  Log;
} HostCmd_DS_802_11_GET_LOG, *PHostCmd_DS_802_11_GET_LOG;

// Define data structure for HostCmd_CMD_MAC_CONTROL
typedef struct _HostCmd_DS_MAC_CONTROL {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;                  // RX, TX, INT, WEP, LOOPBACK on/off
    USHORT Reserved;
} HostCmd_DS_MAC_CONTROL, *PHostCmd_DS_MAC_CONTROL;

// Define data structure for HostCmd_CMD_MAC_MULTICAST_ADR
typedef struct _HostCmd_DS_MAC_MULTICAST_ADR {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    USHORT NumOfAdrs;
    UCHAR MACList[HostCmd_SIZE_MAC_ADR*HostCmd_MAX_MCAST_ADRS];
} HostCmd_DS_MAC_MULTICAST_ADR, *PHostCmd_DS_MAC_MULTICAST_ADR;

// Define data structure for HostCmd_CMD_802_11_AUTHENTICATE
typedef struct _HostCmd_DS_802_11_AUTHENTICATE {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    UCHAR MacAddr[6];
    UCHAR AuthType;
    USHORT TimeOut;
    UCHAR  Reserved[3];
} HostCmd_DS_802_11_AUTHENTICATE, *PHostCmd_DS_802_11_AUTHENTICATE;

// Define data structure for HostCmd_RET_802_11_AUTHENTICATE
typedef struct _HostCmd_DS_802_11_AUTHENTICATE_RESULT {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    UCHAR MacAddr[6];
    UCHAR AuthType;
    UCHAR AuthStatus;
} HostCmd_DS_802_11_AUTHENTICATE_RESULT, *PHostCmd_DS_802_11_AUTHENTICATE_RESULT;

// Define data structure for HostCmd_CMD_802_11_DEAUTHENTICATE
typedef struct _HostCmd_DS_802_11_DEAUTHENTICATE {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    UCHAR MacAddr[6];
    ULONG ReasonCode;
} HostCmd_DS_802_11_DEAUTHENTICATE, *PHostCmd_DS_802_11_DEAUTHENTICATE;

// Define data structure for HostCmd_RET_802_11_DEAUTHENTICATE
typedef struct _HostCmd_DS_802_11_DEAUTHENTICATE_RESULT {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    UCHAR MacAddr[6];
    UCHAR AuthStatus;
    UCHAR Reserved;
} HostCmd_DS_802_11_DEAUTHENTICATE_RESULT, *PHostCmd_DS_802_11_DEAUTHENTICATE_RESULT;

// Define data structure for HostCmd_CMD_802_11_ASSOCIATE and  
//  HostCmd_CMD_802_11_REASSOCIATE

#define WLAN_SUPPORTED_RATES        14

typedef struct _CfParamSet_t {
    UCHAR   CfpCnt;
    UCHAR   CfpPeriod;
    USHORT  CfpMaxDuration;
    USHORT  CfpDurationRemaining;
} CfParamSet_t;

typedef struct _IbssParamSet_t {
    USHORT  AtimWindow;
} IbssParamSet_t;

typedef struct _MrvlIEtypes_SsParamSet_t {
    MrvlIEtypesHeader_t Header;
    union {
        CfParamSet_t    CfParamSet[1];
        IbssParamSet_t  IbssParamSet[1];
    } cf_ibss;
} MrvlIEtypes_SsParamSet_t;

typedef struct _FhParamSet_t {
    USHORT  DwellTime;
    UCHAR   HopSet;
    UCHAR   HopPattern;
    UCHAR   HopIndex;
}  FhParamSet_t;

typedef struct _DsParamSet_t {
    UCHAR   CurrentChan;
}  DsParamSet_t;

typedef struct _MrvlIEtypes_PhyParamSet_t {
    MrvlIEtypesHeader_t Header;
    union {
        FhParamSet_t    FhParamSet[1];
        DsParamSet_t    DsParamSet[1];
    } fh_ds;
}  MrvlIEtypes_PhyParamSet_t;

typedef struct  _MrvlIEtypes_AuthType_t {
    MrvlIEtypesHeader_t Header;
    USHORT  AuthType;
}  MrvlIEtypes_AuthType_t;

typedef struct  _MrvlIEtypes_RsnParamSet_t {
    MrvlIEtypesHeader_t Header;
    UCHAR   RsnIE[1];
}  MrvlIEtypes_RsnParamSet_t;

typedef struct  _MrvlIEtypes_WmmParamSet_t {
    MrvlIEtypesHeader_t Header;
    UCHAR   WmmIE[1];
}  MrvlIEtypes_WmmParamSet_t;

typedef struct  _MrvlIEtypes_WmmUapsd_t {
    MrvlIEtypesHeader_t Header;
    UCHAR   WmmIE[1];
}  MrvlIEtypes_WmmUapsd_t; 

typedef struct  _MrvlIEtypes_TsfTimestamp_t {
    MrvlIEtypesHeader_t Header;
    ULONGLONG   tsfTable[1];
}  MrvlIEtypes_TsfTimestamp_t; 

typedef struct  _MrvlIEtypes_Data_t {
    MrvlIEtypesHeader_t Header;
    UCHAR   Data[1];
}  MrvlIEtypes_Data_t; 

typedef struct  _MrvlIEtypes_Passthrough_t {
    MrvlIEtypesHeader_t Header;
    ULONGLONG   Data[1];
}  MrvlIEtypes_Passthrough_t; 

typedef struct _HostCmd_DS_802_11_ASSOCIATE_EXT 
{
    USHORT                      Command;
    USHORT                      Size;
    USHORT                      SeqNum;
    USHORT                      Result;
    UCHAR                       PeerStaAddr[6];
    IEEEtypes_CapInfo_t         CapInfo;
    USHORT                      ListenInterval;
    USHORT                      BcnPeriod;
    UCHAR                       DtimPeriod;
    
    MrvlIEtypes_SsIdParamSet_t  SsIdParamSet;
    MrvlIEtypes_PhyParamSet_t   PhyParamSet;
    MrvlIEtypes_SsParamSet_t    SsParamSet;
    MrvlIEtypes_RatesParamSet_t RatesParamSet;

    MrvlIEtypes_RsnParamSet_t   RsnParamSet;

    MrvlIEtypes_WmmParamSet_t   WmmParamSet;
  MrvlIEtypes_WmmUapsd_t    WmmUapsd;

} HostCmd_DS_802_11_ASSOCIATE_EXT, 
  *PHostCmd_DS_802_11_ASSOCIATE_EXT;

// Define data structure for HostCmd_RET_802_11_ASSOCIATE
typedef struct _HostCmd_DS_802_11_ASSOCIATE_RESULT {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;    
    USHORT  CapInfo;
    USHORT  StatusCode;
    USHORT  AssociationID;
    USHORT  IELength;
    UCHAR   IE[1];
}HostCmd_DS_802_11_ASSOCIATE_RESULT, *PHostCmd_DS_802_11_ASSOCIATE_RESULT;

// Define data structure for HostCmd_RET_802_11_AD_HOC_JOIN
typedef struct _HostCmd_DS_802_11_AD_HOC_RESULT {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    UCHAR  ResultCode;
    // changed due to FW mismatch
    //UCHAR  Reserved[3];
    UCHAR   Reserved[2];
    UCHAR  BSSID[MRVDRV_ETH_ADDR_LEN];  
}HostCmd_DS_802_11_AD_HOC_RESULT, *PHostCmd_DS_802_11_AD_HOC_RESULT;

// Define data structure for HostCmd_CMD_802_11_SET_WEP
typedef struct _HostCmd_DS_802_11_SET_WEP {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;                      // ACT_ADD, ACT_REMOVE or ACT_ENABLE 
    USHORT KeyIndex;                    // Key Index selected for Tx
    UCHAR WEPTypeForKey1;               // 40, 128bit or TXWEP 
    UCHAR WEPTypeForKey2;
    UCHAR WEPTypeForKey3;
    UCHAR WEPTypeForKey4;
    UCHAR WEP1[16];                     // WEP Key itself
    UCHAR WEP2[16];
    UCHAR WEP3[16];
    UCHAR WEP4[16];
} HostCmd_DS_802_11_SET_WEP, *PHostCmd_DS_802_11_SET_WEP;



typedef enum {
    DesiredBssType_i = 0,
    OpRateSet_i,
    BcnPeriod_i,
    DtimPeriod_i,
    AssocRspTimeOut_i,
    RtsThresh_i,
    ShortRetryLim_i,
    LongRetryLim_i,
    FragThresh_i,
    //MaxTxMsduLife_i,
    //MaxRxLife_i,
    Dot11D_i,
    Dot11HTPC_i,
    ManufId_i,
    ProdId_i,
    ManufOui_i,
    ManufName_i,
    ManufProdName_i,
    ManufProdVer_i      
} SNMP_MIB_INDEX_e;

// Define data structure for HostCmd_CMD_802_11_SNMP_MIB
typedef struct _HostCmd_DS_802_11_SNMP_MIB {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT QueryType;
    USHORT OID;
    USHORT BufSize;
    UCHAR Value[128];
} HostCmd_DS_802_11_SNMP_MIB, *PHostCmd_DS_802_11_SNMP_MIB;

// Define data structure for HostCmd_CMD_MAC_REG_MAP
typedef struct _HostCmd_DS_MAC_REG_MAP {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT BufferSize;              // 128 UCHARs
    UCHAR RegMap[128];
    USHORT Reserved;
} HostCmd_DS_MAC_REG_MAP, *PHostCmd_DS_MAC_REG_MAP;

// Define data structure for HostCmd_CMD_BBP_REG_MAP
typedef struct _HostCmd_DS_BBP_REG_MAP {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT BufferSize;              // 128 UCHARs
    UCHAR RegMap[128];
    USHORT Reserved;
} HostCmd_DS_BBP_REG_MAP, *PHostCmd_DS_BBP_REG_MAP;

// Define data structure for HostCmd_CMD_RF_REG_MAP
typedef struct _HostCmd_DS_RF_REG_MAP {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT BufferSize;              // 64 UCHARs
    UCHAR RegMap[64];
    USHORT Reserved;
} HostCmd_DS_RF_REG_MAP, *PHostCmd_DS_RF_REG_MAP;

// Define data structure for HostCmd_CMD_MAC_REG_ACCESS
typedef struct _HostCmd_DS_MAC_REG_ACCESS {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    USHORT Offset;
    ULONG Value;
    USHORT Reserved;
} HostCmd_DS_MAC_REG_ACCESS, *PHostCmd_DS_MAC_REG_ACCESS;

// Define data structure for HostCmd_CMD_BBP_REG_ACCESS
typedef struct _HostCmd_DS_BBP_REG_ACCESS {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    USHORT Offset;
    UCHAR Value;
    UCHAR Reserverd[3];
} HostCmd_DS_BBP_REG_ACCESS, *PHostCmd_DS_BBP_REG_ACCESS;

// Define data structure for HostCmd_CMD_RF_REG_ACCESS
typedef struct _HostCmd_DS_RF_REG_ACCESS {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    USHORT Offset;
    UCHAR Value;
    UCHAR Reserverd[3];
} HostCmd_DS_RF_REG_ACCESS, *PHostCmd_DS_RF_REG_ACCESS;

// Define data structure for HostCmd_CMD_802_11_RADIO_CONTROL
typedef struct _HostCmd_DS_802_11_RADIO_CONTROL {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;  
    //dralee_20060613                 
    //UCHAR  Control; // @bit0: 1/0,on/off, @bit1: 1/0, long/short @bit2: 1/0,auto/fix
    //UCHAR  Reserved; 
    USHORT   Control;
} HostCmd_DS_802_11_RADIO_CONTROL, *PHostCmd_DS_802_11_RADIO_CONTROL;

// Define data structure for HostCmd_CMD_802_11_RF_CHANNEL
typedef struct _HostCmd_DS_802_11_RF_CHANNEL {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    USHORT CurentChannel;
    // next 3 fields are present on v3 API document, but marked not used
    // commented out to avoid misuse
    /*    
    USHORT RFType;                  // HostCmd_TYPE_802_11A or HostCmd_TYPE_802_11A 
    USHORT Reserved;
    UCHAR ChannelList[MRVDRV_MAX_CHANNEL_NUMBER];
    */
    
} HostCmd_DS_802_11_RF_CHANNEL, *PHostCmd_DS_802_11_RF_CHANNEL;

/**  HostCmd_CMD_802_11_RSSI */
typedef struct _HostCmd_DS_802_11_RSSI {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    /* weighting factor */
     SHORT  N;  

     SHORT Reserved_0;
     SHORT Reserved_1;
     SHORT Reserved_2;  
} HostCmd_DS_802_11_RSSI, *PHostCmd_DS_802_11_RSSI;

typedef struct _HostCmd_DS_802_11_RSSI_RSP {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    SHORT  SNR;                      
    SHORT  NoiseFloor;
    SHORT  AvgSNR;
    SHORT  AvgNoiseFloor;
} HostCmd_DS_802_11_RSSI_RSP, *PHostCmd_DS_802_11_RSSI_RSP;



// Define data structure for HostCmd_CMD_802_11_RF_TX_POWER
typedef struct _HostCmd_DS_802_11_RF_TX_POWER {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    // 35.p6 +++ : for new command RF_TX_POWER
    USHORT CurrentLevel;     // 1..8
    UCHAR   MaxPower;
    UCHAR   MinPower;
    // 35.p6 --- : for new command RF_TX_POWER
} HostCmd_DS_802_11_RF_TX_POWER, *PHostCmd_DS_802_11_RF_TX_POWER;

// Define data structure for HostCmd_CMD_802_11_RF_ANTENNA
typedef struct _HostCmd_DS_802_11_RF_ANTENNA {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    USHORT AntennaMode;             // Number of antennas or 0xffff(diversity)
} HostCmd_DS_802_11_RF_ANTENNA, *PHostCmd_DS_802_11_RF_ANTENNA;

// Define data structure for HostCmd_CMD_802_11_PS_MODE
typedef struct _HostCmd_DS_802_11_PS_MODE {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    //Dralee_20061115
    USHORT SubCommand;  //Action. 

    //Dralee_20061115
    USHORT NullPktInterval; //USHORT reserved1;  //PowerMode; dralee_20060220      /*CAM, Max.PSP or Fast PSP */
    USHORT PSNumDtims;
    USHORT reserved2;
    USHORT LocalListenInterval;  
    USHORT AdhocAwakePeriod;

} HostCmd_DS_802_11_PS_MODE, *PHostCmd_DS_802_11_PS_MODE;

// Define data structure for HostCmd_CMD_802_11_DATA_RATE
typedef struct _HostCmd_DS_802_11_DATA_RATE {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    USHORT Reserverd;    
    UCHAR DataRate[NDIS_SUPPORTED_RATES];               // Supported data reate list

} HostCmd_DS_802_11_DATA_RATE, *PHostCmd_DS_802_11_DATA_RATE;

// Define data structure for start Command in Ad Hoc mode  
typedef struct _HostCmd_DS_802_11_AD_HOC_START {
    USHORT                    Command;
    USHORT                    Size;
    USHORT                    SeqNum;
    USHORT                    Result;
    UCHAR                     SSID[MRVDRV_MAX_SSID_LENGTH];
    UCHAR                     BSSType;
    USHORT                    BeaconPeriod;
    UCHAR                     DTIMPeriod;
    IEEEtypes_SsParamSet_t    SsParamSet;
    IEEEtypes_PhyParamSet_t   PhyParamSet;
    USHORT                    ProbeDelay;
    IEEEtypes_CapInfo_t       Cap;
    UCHAR                     BasicDataRates[14];
    // UCHAR                     OpDataRates[8];
} HostCmd_DS_802_11_AD_HOC_START, *PHostCmd_DS_802_11_AD_HOC_START;

// Define data structure for Join Command in Ad Hoc mode
// v9 adhoc join
typedef struct _IEEEtypes_AdHoc_BssDesc_t
{
    UCHAR                     BSSID[MRVDRV_ETH_ADDR_LEN];
    UCHAR                     SSID[MRVDRV_MAX_SSID_LENGTH];
    UCHAR                     BSSType; // v9 linux driver uses this size.
    USHORT                   BeaconPeriod;
    UCHAR                     DTIMPeriod;
    UCHAR                     TimeStamp[8];
    UCHAR                     LocalTime[8];
    IEEEtypes_DsParamSet_t      DsParamSet;
    UCHAR                                Reserved1[4];
    IEEEtypes_IbssParamSet_t    IbssParamSet;
    UCHAR                                Reserved2[4];
    IEEEtypes_CapInfo_t            Cap;
    UCHAR                                DataRates[NDIS_SUPPORTED_RATES];
} IEEEtypes_AdHoc_BssDesc_t, *PIEEEtypes_AdHoc_BssDesc_t;

typedef struct _HostCmd_DS_802_11_AD_HOC_JOIN {
    USHORT                          Command;
    USHORT                          Size;
    USHORT                          SeqNum;
    USHORT                          Result;
    IEEEtypes_AdHoc_BssDesc_t  BssDescriptor;
    USHORT                          FailTimeOut;
    USHORT                          ProbeDelay;
    /*
        TODO: There are 2 more members here for DFS. v9 linux driver doesn't use it. Let me skip them first.
    */
} HostCmd_DS_802_11_AD_HOC_JOIN, *PHostCmd_DS_802_11_AD_HOC_JOIN;

// Define data structure for HostCmd_CMD_SET_ACTIVE_SCAN_SSID 
typedef struct _HostCmd_DS_SET_ACTIVE_SCAN_SSID {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;
    NDIS_802_11_SSID ActiveScanSSID;
} HostCmd_DS_SET_ACTIVE_SCAN_SSID, *PHostCmd_DS_SET_ACTIVE_SCAN_SSID;

// Define data structure for HostCmd_CMD_QOS_WME_ENABLE_STATE 
typedef struct _HostCmd_CMD_QOS_WME_ENABLE_STATE {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;  // ACTION_GET or ACTION_SET
    USHORT Enabled; // 0 = disabled, 1 = enabled
} HostCmd_CMD_QOS_WME_ENABLE_STATE, *PHostCmd_CMD_QOS_WME_ENABLE_STATE;

// Define data structure for HostCmd_CMD_QOS_WME_ENABLE_STATE 
typedef struct _HostCmd_CMD_QOS_WME_ACCESS_CATEGORY_PARAMETERS {
    USHORT Command;
    USHORT Size;
    USHORT SeqNum;
    USHORT Result;
    USHORT Action;      // ACTION_GET or ACTION_SET
    ULONG  AC_BE;       // Best Effort AC parameters
    ULONG  AC_BE_XTRA;  // Extra AC_BE parameters
    ULONG  AC_BK;       // Background AC parameters
    ULONG  AC_BK_XTRA;  // Extra AC_BK parameters
    ULONG  AC_VI;       // Video AC parameters
    ULONG  AC_VI_XTRA;  // Extra Video AC parameters
    ULONG  AC_VO;       // Voice AC parameters
    ULONG  AC_VO_XTRA;  // Extra Voice AC parameters
} HostCmd_CMD_QOS_WME_ACCESS_CATEGORY_PARAMETERS,  
*PHostCmd_CMD_QOS_WME_ACCESS_CATEGORY_PARAMETERS;


typedef struct _HostCmd_DS_802_11_UNICAST_CIPHER
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    UCHAR   UnicastCipher[4];
    USHORT  Enabled;     
} HostCmd_DS_802_11_UNICAST_CIPHER, *PHostCmd_DS_802_11_UNICAST_CIPHER;

typedef struct _HostCmd_DS_802_11_MCAST_CIPHER
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    UCHAR   McastCipher[4];
    USHORT  Enabled;     
} HostCmd_DS_802_11_MCAST_CIPHER, *PHostCmd_DS_802_11_MCAST_CIPHER;
typedef struct _HostCmd_DS_802_11_RSN_AUTH_SUITES
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    UCHAR   AuthSuites[4];
    USHORT  Enabled;     
} HostCmd_DS_802_11_RSN_AUTH_SUITES, *PHostCmd_DS_802_11_RSN_AUTH_SUITES;

typedef struct _HostCmd_DS_802_11_PWK_KEY
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    UCHAR   TkipEncryptKey[16];
    UCHAR   TkipTxMicKey[8];
    UCHAR   TkipRxMicKey[8];
} HostCmd_DS_802_11_PWK_KEY, *PHostCmd_DS_802_11_PWK_KEY;

/*Define data structure for CMD_802_11_GRP_KEY*/
typedef struct _HostCmd_DS_802_11_GRP_KEY
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    UCHAR   TkipEncryptKey[16];
    UCHAR   TkipTxMicKey[8];
    UCHAR   TkipRxMicKey[8];
} HostCmd_DS_802_11_GRP_KEY, *PHostCmd_DS_802_11_GRP_KEY;

/*Define data structure for CMD_802_11_PAIRWISE_TSC*/
typedef struct _HostCmd_DS_802_11_PAIRWISE_TSC
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    ULONG   TxIv32;
    USHORT  TxIv16;
} HostCmd_DS_802_11_PAIRWISE_TSC, *PHostCmd_DS_802_11_PAIRWISE_TSC;

/*Define data structure for CMD_802_11_GROUP_TSC*/
typedef struct _HostCmd_DS_802_11_GROUP_TSC
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    ULONG   TxIv32;
    USHORT  TxIv16;
} HostCmd_DS_802_11_GROUP_TSC, *PHostCmd_DS_802_11_GROUP_TSC;

/*Define data structure for CMD_802_11_QUERY_TKIP_REPLY_CNTRS*/
typedef struct _HostCmd_DS_802_11_QUERY_TKIP_REPLY_CNTRS
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    ULONG   NumTkipCntrs;
} HostCmd_DS_802_11_QUERY_TKIP_REPLY_CNTRS, *PHostCmd_DS_802_11_QUERY_TKIP_REPLY_CNTRS;
typedef struct _HostCmd_DS_CMD_MAC_REG_ACCESS {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     // ACTION_GET or ACTION_SET
    USHORT  Offset;
    ULONG   Value;
} HostCmd_DS_CMD_MAC_REG_ACCESS, *PHostCmd_DS_CMD_MAC_REG_ACCESS;

typedef struct _HostCmd_DS_CMD_RF_REG_ACCESS {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     // ACTION_GET or ACTION_SET
    USHORT  Offset;
    UCHAR   Value;
} HostCmd_DS_CMD_RF_REG_ACCESS, *PHostCmd_DS_CMD_RF_REG_ACCESS;

typedef struct _HostCmd_DS_CMD_BBP_REG_ACCESS {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     // ACTION_GET or ACTION_SET
    USHORT  Offset;
    UCHAR   Value;
} HostCmd_DS_CMD_BBP_REG_ACCESS, *PHostCmd_DS_CMD_BBP_REG_ACCESS;

typedef struct _HostCmd_DS_CMD_REGION_CODE {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     // ACTION_GET or ACTION_SET
    USHORT  RegionCode;
} HostCmd_DS_CMD_REGION_CODE, *PHostCmd_DS_CMD_REGION_CODE;

typedef struct _HostCmd_DS_CMD_MAC_ADDRESS {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     // ACTION_GET or ACTION_SET
    UCHAR   MacAddress[MRVDRV_ETH_ADDR_LEN];
} HostCmd_DS_CMD_MAC_ADDRESS, *PHostCmd_DS_CMD_MAC_ADDRESS;

// TX and RX mode command uses the same type of buffer structure
typedef struct _HostCmd_DS_CMD_TXRX_MODE {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    UCHAR   Mode;
} HostCmd_DS_CMD_TXRX_MODE, *PHostCmd_DS_CMD_TXRX_MODE;

// BT configuration
typedef struct _HostCmd_DS_CMD_BCA_CONFIGURATION {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;
    USHORT  Mode;
    USHORT  AntennaConfig;
    USHORT  FreqConfig;
    ULONG   TxPriorityLo;
    ULONG   TxPriorityHi;
    ULONG   RxPriorityLo;
    ULONG   RxPriorityHi;
} HostCmd_DS_CMD_BCA_CONFIGURATION, *PHostCmd_DS_CMD_BCA_CONFIGURATION;


typedef struct _HostCmd_DS_802_11_SLEEP_PARAMS{
  USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     /* ACT_GET/ACT_SET */   
    USHORT  Error;      /* Sleep clock error in ppm */
    USHORT  Offset;     /* Wakeup offset in usec */
    USHORT  StableTime;     /* Clock stabilization time in usec */
    UCHAR   CalControl;     /* Control periodic calibration */
    UCHAR   ExternalSleepClk; /* Control the use of external sleep clock */ 
    USHORT  Reserved;   /* Reserved field, should be set to zero */
}HostCmd_DS_802_11_SLEEP_PARAMS,*PHostCmd_DS_802_11_SLEEP_PARAMS;

typedef struct _HostCmd_DS_802_11_FW_WAKE_METHOD{
  USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     /* ACT_GET/ACT_SET */   
    USHORT  Method;     /* awake method */
}HostCmd_DS_802_11_FW_WAKE_METHOD,*PHostCmd_DS_802_11_FW_WAKE_METHOD;

typedef struct _HostCmd_CMD_802_11_INACTIVITY_TIMEOUT{
  USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     /* ACT_GET/ACT_SET */   
    USHORT  timeout;        /* awake method */
} HostCmd_DS_802_11_INACTIVITY_TIMEOUT,*PHostCmd_DS_802_11_INACTIVITY_TIMEOUT; 

typedef struct _HostCmd_CMD_802_11_SLEEP_PERIOD{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action;     /* ACT_GET/ACT_SET */   
    USHORT  period;     /* sleep period */
} HostCmd_DS_802_11_SLEEP_PERIOD,*PHostCmd_DS_802_11_SLEEP_PERIOD;

typedef struct _HostCmd_DS_802_11_WMM_ACK_POLICY {
        USHORT  Command;
        USHORT  Size;
        USHORT  SeqNum;
        USHORT  Result;
    USHORT  Action;         /* 0 - ACT_GET
                       1 - ACT_SET */
    UCHAR   AC;         /* 0 - AC_BE
                       1 - AC_BK
                       2 - AC_VI
                       3 - AC_VO */
    UCHAR   AckPolicy;      /* 0 - WMM_ACK_POLICY_IMM_ACK
                       1 - WMM_ACK_POLICY_NO_ACK */
}HostCmd_DS_802_11_WMM_ACK_POLICY, *PHostCmd_DS_802_11_WMM_ACK_POLICY;

typedef struct _HostCmd_DS_802_11_WMM_GET_STATUS {
        USHORT  Command;
        USHORT  Size;
        USHORT  SeqNum;
        USHORT  Result;
    WMM_AC_STATUS Status[4];
}HostCmd_DS_802_11_WMM_GET_STATUS,*PHostCmd_DS_802_11_WMM_GET_STATUS;







#define SIZEOF_OID_DS_LEADING       sizeof(NDIS_OID)

/*
    the type of oid_ds's data_member is different from cmd_ds's
*/
#define TT_CMDPARSE_STD_QUERY_BACKUP_SINGLE_MEMBER( oid_ds, cmd_ds, oid_data_member, cmd_data_member, oid_data_type )\
{\
    P##cmd_ds   pCmd;\
    P##oid_ds   pUserBuffer;\
\
    pUserBuffer = (P##oid_ds) Adapter->OidCmdRespBuf;\
    pCmd = (P##cmd_ds) pRetPtr;\
\
    pUserBuffer->##oid_data_member = (##oid_data_type) pCmd->##cmd_data_member;\
    Adapter->nOidCmdResult = pCmd->Result;\
    Adapter->nSizeOfOidCmdResp = sizeof(##oid_ds);\
\
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\n", pUserBuffer->##oid_data_member) );\
}

#define TT_CMDPARSE_STD_QUERY_BACKUP_SINGLE_VALUE( dest_ptr, dest_ds, src_ptr, src_ds, src_member )\
{\
    (dest_ptr) = (##dest_ds *) Adapter->OidCmdRespBuf;\
    (src_ptr) = (P##src_ds) pRetPtr;\
\
    *(dest_ptr) = (##dest_ds)((src_ptr)->##src_member);\
    Adapter->nOidCmdResult = (src_ptr)->Result;\
    Adapter->nSizeOfOidCmdResp = sizeof(##dest_ds);\
\
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD resp value: %d\n", *(dest_ptr)) );\
}


#define TT_CMDPARSE_SYNC_STD_OID_QUERY_AND_RETURN( needed_size, cmd_id, cmd_op )\
{\
        DWORD       dwWaitStatus;\
\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- STD OID sync query - " L#cmd_id L" (OID: 0x%x)\n", Oid) );\
\
        if( InformationBufferLength < (needed_size) )\
        {\
            *BytesNeeded = (needed_size);\
            DBGPRINT(DBG_OID|DBG_HELP, (L"     Invalid length [In:%d, Needed:%d]\n", InformationBufferLength, (needed_size)) );\
            return NDIS_STATUS_INVALID_LENGTH;\
        }\
\
        Adapter->nOidCmdResult = HostCmd_RESULT_ERROR;\
\
        Status=PrepareAndSendCommand(\
                    Adapter,\
                    (cmd_id),\
                    (cmd_op),\
                    HostCmd_OPTION_USE_INT,\
                    Oid,\
                    HostCmd_PENDING_ON_GET_OID,\
                    0,\
                    FALSE,\
                    BytesWritten,\
                    NULL,\
                    BytesNeeded,\
                    InformationBuffer);\
\
        if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)\
            return Status;\
\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Wait for complete...\n") );\
		dwWaitStatus = WaitOIDAccessRespond( Adapter );\
		if ( dwWaitStatus == NDIS_STATUS_SUCCESS)\
        {\
            if ( Adapter->nOidCmdResult != HostCmd_RESULT_OK )\
            {\
                DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [CommandResult is not OK]\n") );\
                return NDIS_STATUS_NOT_ACCEPTED;\
            }\
\
            if ( Adapter->nSizeOfOidCmdResp > (needed_size) )\
            {\
                DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Wrong length [oid: 0x%x, size: %d]\n", Oid, Adapter->nSizeOfOidCmdResp) );\
                return NDIS_STATUS_NOT_ACCEPTED;\
            }\
\
            NdisMoveMemory(\
                (PUCHAR) InformationBuffer,\
                (PUCHAR) (Adapter->OidCmdRespBuf),\
                Adapter->nSizeOfOidCmdResp );\
\
            *BytesWritten = Adapter->nSizeOfOidCmdResp;\
\
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Success...\n") );\
            return NDIS_STATUS_SUCCESS;\
        }\
        else\
        {\
            *BytesWritten = 0;\
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [0x%x]\n", dwWaitStatus) );\
            return NDIS_STATUS_NOT_ACCEPTED;\
        }\
}\

#define TT_CMDPARSE_SYNC_STD_OID_QUERY_ADHOCAES_RETURN( needed_size, cmd_id, cmd_op )\
{\
        DWORD       dwWaitStatus;\
\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- STD OID sync query - " L#cmd_id L" (OID: 0x%x)\n", Oid) );\
\
        if( InformationBufferLength < (needed_size) )\
        {\
            *BytesNeeded = (needed_size);\
            DBGPRINT(DBG_OID|DBG_HELP, (L"     Invalid length [In:%d, Needed:%d]\n", InformationBufferLength, (needed_size)) );\
            return NDIS_STATUS_INVALID_LENGTH;\
        }\
\
        Adapter->nOidCmdResult = HostCmd_RESULT_ERROR;\
\
        Status=PrepareAndSendCommand(\
                    Adapter,\
                    (cmd_id),\
                    (cmd_op),\
                    KEY_INFO_ENABLED,\
                    Oid,\
                    HostCmd_PENDING_ON_GET_OID,\
                    0,\
                    FALSE,\
                    BytesWritten,\
                    NULL,\
                    BytesNeeded,\
                    InformationBuffer);\
\
        if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)\
            return Status;\
\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Wait for complete...\n") );\
		dwWaitStatus = WaitOIDAccessRespond( Adapter );\
		if ( dwWaitStatus == NDIS_STATUS_SUCCESS)\
        {\
            if ( Adapter->nOidCmdResult != HostCmd_RESULT_OK )\
            {\
                DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [CommandResult is not OK]\n") );\
                return NDIS_STATUS_NOT_ACCEPTED;\
            }\
\
            if ( Adapter->nSizeOfOidCmdResp > (needed_size) )\
            {\
                DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Wrong length [oid: 0x%x, size: %d]\n", Oid, Adapter->nSizeOfOidCmdResp) );\
                return NDIS_STATUS_NOT_ACCEPTED;\
            }\
\
            NdisMoveMemory(\
                (PUCHAR) InformationBuffer,\
                (PUCHAR) (Adapter->OidCmdRespBuf),\
                Adapter->nSizeOfOidCmdResp );\
\
            *BytesWritten = Adapter->nSizeOfOidCmdResp;\
\
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Success...\n") );\
            return NDIS_STATUS_SUCCESS;\
        }\
        else\
        {\
            *BytesWritten = 0;\
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [0x%x]\n", dwWaitStatus) );\
            return NDIS_STATUS_NOT_ACCEPTED;\
        }\
}\

//ahan [2006-04-14]
#define TT_CMDPARSE_SEND_CMD_NO_ACTION(cmd_ds, oid_ds, start_member) \
{ \
    P##cmd_ds   pThisCmd; \
    P##oid_ds       pOid; \
 \
    pThisCmd = (P##cmd_ds) pCmdPtr; \
    Size = sizeof(##cmd_ds); \
 \
    if ( CmdOption == HostCmd_ACT_GEN_SET ) \
    { \
        pOid = (P##oid_ds) InformationBuffer; \
 \
        NdisMoveMemory( \
                (PUCHAR)&(pThisCmd->##start_member), \
                (PUCHAR)&(pOid->##start_member), \
                sizeof(##oid_ds) - SIZEOF_OID_DS_LEADING); \
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- GET_OID " L#cmd_ds L"." L#start_member L"=0x%x\n",  pThisCmd->##start_member) ); \
        DBGPRINT(DBG_OID|DBG_HELP, (L"     [0x%x] [0x%x] [%d] [TotalLen=%d]\n",  pThisCmd, &(pThisCmd->##start_member), FIELD_OFFSET(##cmd_ds, ##start_member), sizeof(##oid_ds)-SIZEOF_OID_DS_LEADING) ); \
    } \
    else \
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- GET_OID " L#cmd_ds L"...\n") ); \
}
//ahan [2006-04-14]


#define TT_CMDPARSE_SEND_CMD(cmd_ds, oid_ds, start_member) \
{ \
    P##cmd_ds   pThisCmd; \
    P##oid_ds       pOid; \
 \
    pThisCmd = (P##cmd_ds) pCmdPtr; \
    Size = sizeof(##cmd_ds); \
 \
    pThisCmd->Action = CmdOption; \
 \
    if ( CmdOption == HostCmd_ACT_GEN_SET ) \
    { \
        pOid = (P##oid_ds) InformationBuffer; \
 \
        NdisMoveMemory( \
                (PUCHAR)&(pThisCmd->##start_member), \
                (PUCHAR)&(pOid->##start_member), \
                sizeof(##oid_ds) - SIZEOF_OID_DS_LEADING); \
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- SET_OID " L#cmd_ds L"." L#start_member L"=0x%x\n",  pThisCmd->##start_member) ); \
        DBGPRINT(DBG_OID|DBG_HELP, (L"     [0x%x] [0x%x] [%d] [TotalLen=%d]\n",  pThisCmd, &(pThisCmd->##start_member), FIELD_OFFSET(##cmd_ds, ##start_member), sizeof(##oid_ds)-SIZEOF_OID_DS_LEADING) ); \
    } \
    else \
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- GET_OID " L#cmd_ds L"...\n") ); \
}

/* // tt wled
    change pCmdResult->Action to 99, since some cmd doesn't have such data member.
*/
#define TT_CMDPARSE_CMD_RESP2(cmd_ds, member_1, member_2) \
{ \
    P##cmd_ds   pCmdResult; \
 \
    pCmdResult = (P##cmd_ds) pRetPtr; \
 \
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- CMD_RSP: " L#cmd_ds L", " L#member_1 L"=0x%x, " L#member_2 L"=0x%x [Rlt=%d, Act=%d]\n", \
        pCmdResult->##member_1, pCmdResult->##member_2, \
        pCmdResult->Result, 99) ); \
}

#define TT_CMDPARSE_QUERY_BACKUP_CMD_VAR_RESP(adapter_obj, cmd_ds, cmd_start_member, lead_size)\
{\
    P##cmd_ds   pCmdResult;\
\
    pCmdResult = (P##cmd_ds) Adapter->CurCmd->BufVirtualAddr;\
\
    if ( (lead_size) > 0 )\
        NdisZeroMemory( (adapter_obj)->OidCmdRespBuf, (lead_size) );\
\
    NdisMoveMemory(\
        (adapter_obj)->OidCmdRespBuf + (lead_size),\
        (PUCHAR) &(pCmdResult->##cmd_start_member),\
        pCmdResult->Size-FIELD_OFFSET(##cmd_ds, ##cmd_start_member) );\
\
    (adapter_obj)->nSizeOfOidCmdResp = pCmdResult->Size-FIELD_OFFSET(##cmd_ds, ##cmd_start_member);\
    (adapter_obj)->nOidCmdResult = pCmdResult->Result;\
}

#define TT_CMDPARSE_QUERY_BACKUP_CMD_RESP(adapter_obj, cmd_ds, cmd_start_member, lead_size)\
{\
    P##cmd_ds   pCmdResult;\
\
    pCmdResult = (P##cmd_ds) Adapter->CurCmd->BufVirtualAddr;\
\
    if ( (lead_size) > 0 )\
        NdisZeroMemory( (adapter_obj)->OidCmdRespBuf, (lead_size) );\
\
    NdisMoveMemory(\
        (adapter_obj)->OidCmdRespBuf + (lead_size),\
        (PUCHAR) &(pCmdResult->##cmd_start_member),\
        sizeof(##cmd_ds)-FIELD_OFFSET(##cmd_ds, ##cmd_start_member) );\
\
    (adapter_obj)->nSizeOfOidCmdResp = sizeof(##cmd_ds)-FIELD_OFFSET(##cmd_ds, ##cmd_start_member);\
    (adapter_obj)->nOidCmdResult = pCmdResult->Result;\
}

#define TT_CMDPARSE_SET_BACKUP_CMD_RESP(adapter_obj)\
{\
    PHostCmd_DS_GEN pCmdResult;\
\
    pCmdResult = (PHostCmd_DS_GEN) Adapter->CurCmd->BufVirtualAddr;\
\
    (adapter_obj)->nSizeOfOidCmdResp = 0;\
    (adapter_obj)->nOidCmdResult = pCmdResult->Result;\
}



/*
    needed_size can be zero if you don't want this macro to compare it with InformationBufferLength.
*/
#define TT_CMDPARSE_SYNC_OID( hostcmd_id, oid_id, needed_size, cmd_op, pend_info, bytes_rw) \
{ \
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- OID - " L#oid_id L" (" L#pend_info L")\n") ); \
 \
    if ( ((needed_size)!=0) && InformationBufferLength < (needed_size) ) \
    { \
        *BytesNeeded = (needed_size); \
        DBGPRINT(DBG_OID|DBG_HELP, (L"     Invalid length [In:%d, Needed:%d]\n", InformationBufferLength, (needed_size)) ); \
        return NDIS_STATUS_INVALID_LENGTH; \
    } \
 \
    Adapter->nOidCmdResult = HostCmd_RESULT_ERROR;\
 \
    Status=PrepareAndSendCommand( \
            Adapter, \
            (hostcmd_id), \
            (cmd_op), \
            HostCmd_OPTION_USE_INT, \
            0, \
            (pend_info), \
            0, \
            FALSE, \
            (bytes_rw), \
            0, \
            BytesNeeded, \
            InformationBuffer ); \
 \
    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES) \
        return Status; \
}


#define ASYNC_OID_QUERY_TIMEOUT     12000

#define TT_CMDPARSE_QUERY_WAIT_COMPLETE_AND_RETURN(adapter_obj, oid_ds, start_member, lead_size)\
{\
    DWORD       dwWaitStatus;\
    P##oid_ds   pInfoBuf;\
\
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Wait for complete... [" L#oid_ds L"]\n") );\
	dwWaitStatus = WaitOIDAccessRespond( (adapter_obj) );\
	if ( dwWaitStatus == NDIS_STATUS_SUCCESS)\
    {\
        if ( (adapter_obj)->nOidCmdResult != HostCmd_RESULT_OK )\
        {\
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [CommandResult is not OK]\n") );\
            return NDIS_STATUS_NOT_ACCEPTED;\
        }\
\
        pInfoBuf = (P##oid_ds) InformationBuffer; \
        NdisMoveMemory(\
            (PUCHAR) pInfoBuf, \
            (PUCHAR) ((adapter_obj)->OidCmdRespBuf),\
            (adapter_obj)->nSizeOfOidCmdResp+(lead_size) );\
\
        *BytesWritten = sizeof(##oid_ds);\
\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- [0x%x] BakSize=%d, RetSize=%d\r\n", Oid, (adapter_obj)->nSizeOfOidCmdResp+(lead_size), sizeof(##oid_ds)) );\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Success...\r\n") );\
\
        return NDIS_STATUS_SUCCESS;\
    }\
    else\
    {\
        *BytesWritten = 0;\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [0x%x]\n", dwWaitStatus) );\
        return NDIS_STATUS_NOT_ACCEPTED;\
    }\
}

#define TT_CMDPARSE_SET_WAIT_COMPLETE_THEN_GO_OR_RETURN( adapter_obj, cmd_id )\
{\
    DWORD       dwWaitStatus;\
\
    *BytesRead = 0;\
\
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Wait for complete... [cmd_id=0x%x]\n", (cmd_id)) );\
	dwWaitStatus = WaitOIDAccessRespond( (adapter_obj) );\
	if ( dwWaitStatus == NDIS_STATUS_SUCCESS)\
    {\
        if ( (adapter_obj)->nOidCmdResult != HostCmd_RESULT_OK )\
        {\
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [CommandResult is not OK]\n") );\
            return NDIS_STATUS_NOT_ACCEPTED;\
        }\
\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Success then go to next...\n") );\
    }\
    else\
    {\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [0x%x]\n", dwWaitStatus) );\
        return NDIS_STATUS_NOT_ACCEPTED;\
    }\
}

#define TT_CMDPARSE_SET_WAIT_COMPLETE_AND_RETURN( adapter_obj, oid_id, bytes_read )\
{\
    DWORD       dwWaitStatus;\
\
    *BytesRead = 0;\
\
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- Wait for complete... [" L#oid_id L"]\n") );\
	dwWaitStatus = WaitOIDAccessRespond( (adapter_obj) );\
	if ( dwWaitStatus == NDIS_STATUS_SUCCESS)\
    {\
        if ( (adapter_obj)->nOidCmdResult != HostCmd_RESULT_OK )\
        {\
            DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [CommandResult is not OK]\n") );\
            return NDIS_STATUS_NOT_ACCEPTED;\
        }\
\
        *BytesRead = (bytes_read);\
\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Success...\n") );\
\
        return NDIS_STATUS_SUCCESS;\
    }\
    else\
    {\
        DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- >> Fail... [0x%x]\n", dwWaitStatus) );\
        return NDIS_STATUS_NOT_ACCEPTED;\
    }\
}


#define TT_CMDPARSE_DUMP_DATA3(pvar, ds, m1, m2, m3) \
{ \
    P##ds   pDs; \
 \
    pDs = (P##ds) (pvar); \
 \
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- dump " L#ds L"...\n") ); \
    DBGPRINT(DBG_OID|DBG_HELP, (L"     " L#m1 L"=0x%x, " L#m2 L"=0x%x, " L#m3 L"=0x%x\n", \
        pDs->##m1, pDs->##m2, pDs->##m3) ); \
}

#define TT_CMDPARSE_DUMP_DATA5(pvar, ds, m1, m2, m3, m4, m5) \
{ \
    P##ds   pDs; \
 \
    pDs = (P##ds) (pvar); \
 \
    DBGPRINT(DBG_OID|DBG_HELP, (L"-v5- dump " L#ds L"...   (" L#pvar L")\n") ); \
    DBGPRINT(DBG_OID|DBG_HELP, (L"     " L#m1 L"=0x%x, " L#m2 L"=0x%x, " L#m3 L"=0x%x, " L#m4 L"=0x%x, " L#m5 L"=0x%x\n", \
        pDs->##m1, pDs->##m2, pDs->##m3, pDs->##m4, pDs->##m5) ); \
}


#define HostCmd_RET_802_11_RATE_ADAPT_RATESET       (HostCmd_CMD_802_11_RATE_ADAPT_RATESET|0x8000)
#define HostCmd_RET_802_11_TPC_CFG                  (HostCmd_CMD_802_11_TPC_CFG|0x8000)
#define HostCmd_RET_802_11_PWR_CFG                  (HostCmd_CMD_802_11_PWR_CFG|0x8000)
#define HostCmd_RET_802_11_BCA_CONFIG_TIMESHARE (HostCmd_CMD_802_11_BCA_CONFIG_TIMESHARE|0x8000)
#define HostCmd_RET_802_11_SUBSCRIBE_EVENT          (HostCmd_CMD_802_11_SUBSCRIBE_EVENT|0x8000)


typedef struct _HostCmd_DS_802_11_BCA_TIMESHARE
{
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;

    USHORT  Action;     /* ACT_GET/ACT_SET */   
    USHORT  TrafficType;    /* Type: WLAN, BT */
    ULONG   TimeShareInterval; /* 20msec - 60000msec */
    ULONG   BTTime;     /* PTA arbiter time in msec */  
} HostCmd_DS_802_11_BCA_TIMESHARE, *PHostCmd_DS_802_11_BCA_TIMESHARE;

typedef struct _HostCmd_DS_802_11_LED_CONTROL {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;

    USHORT  Action;
    USHORT  NumLed;
    UCHAR   data[256]; //TLV format
} HostCmd_DS_802_11_LED_CONTROL, *PHostCmd_DS_802_11_LED_CONTROL;

typedef struct _HostCmd_DS_MFG_CMD {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;

    UCHAR   MfgCmd[MRVDRV_MFG_CMD_LEN];
} HostCmd_DS_Mfg_Cmd, *PHostCmd_DS_Mfg_Cmd;

#define HostCmd_CMD_802_11_CAL_DATA                 0x0057
#define HostCmd_RET_802_11_CAL_DATA_EXT             (HostCmd_CMD_802_11_CAL_DATA_EXT|0X8000)

typedef struct _HostCmd_DS_802_11_CAL_DATA
{
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    USHORT  Action;
    UCHAR   Reserved1[9];
    UCHAR   PAOption;       /* PA calibration options */
    UCHAR   ExtPA;          /* type of external PA */
    UCHAR   Ant;            /* Antenna selection */
    USHORT  IntPA[14];      /* channel calibration */
    UCHAR   PAConfig[4];        /* RF register calibration */
    UCHAR   Reserved2[4];
    USHORT  Domain;         /* Regulatory Domain */
    UCHAR   ECO;            /* ECO present or not */
    UCHAR   LCT_cal;        /* VGA capacitor calibration */
    UCHAR   Reserved3[12];      
} HostCmd_DS_802_11_CAL_DATA, *PHostCmd_DS_802_11_CAL_DATA;

typedef struct _HostCmd_DS_802_11_CAL_DATA_EXT
{
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    USHORT  Action;
    USHORT  Revision;
    USHORT  CalDataLen;
    UCHAR   CalData[2000]; 
} HostCmd_DS_802_11_CAL_DATA_EXT, *PHostCmd_DS_802_11_CAL_DATA_EXT;

typedef struct _HostCmd_DS_802_11_PWR_CFG {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    USHORT  Action;
    UCHAR   Enable;
    UCHAR   P0;
    UCHAR   P1;
    UCHAR   P2;
} HostCmd_DS_802_11_PWR_CFG, *PHostCmd_DS_802_11_PWR_CFG;

typedef struct _HostCmd_DS_802_11_TPC_CFG {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    USHORT  Action;
    UCHAR   Enable;
    UCHAR   P0;
    UCHAR   P1;
    UCHAR   P2;
    UCHAR   UseSNR;
} HostCmd_DS_802_11_TPC_CFG, *PHostCmd_DS_802_11_TPC_CFG;

typedef struct _HostCmd_RATE_ADAPT_RATESET {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    USHORT      Action;         // 0: GET; 1: SET 

    USHORT      EnableHwAuto;   //0327
    USHORT      Bitmap;
    USHORT      Threshold;
    USHORT      FinalRate;
} HostCmd_RATE_ADAPT_RATESET , *PHostCmd_RATE_ADAPT_RATESET;


typedef struct _HostCmd_DS_802_11_GET_STAT {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    ULONG       Length;          

    ULONG       TransmittedFragmentCount;    
    ULONG       MulticastTransmittedFrameCount;   
    ULONG       FailedCount;   
    ULONG       RetryCount;   
    ULONG       MultipleRetryCount;   
    ULONG       RTSSuccessCount;    	
    ULONG       RTSFailureCount;   	
    ULONG       ACKFailureCount;   
    ULONG       FrameDuplicateCount;   
    ULONG       ReceivedFragmentCount;   
    ULONG       MulticastReceivedFrameCount;   
    ULONG       FCSErrorCount;   
    ULONG       TKIPLocalMICFailures;   
    ULONG       TKIPICVErrorCount;   
    ULONG       TKIPCounterMeasuresInvoked;   
    ULONG       TKIPReplays;   
    ULONG       CCMPFormatErrors;   
    ULONG       CCMPReplays;   
    ULONG       CCMPDecryptErrors;   
    ULONG       FourWayHandshakeFailures;   
    ULONG       WEPUndecryptableCount;   
    ULONG       WEPICVErrorCount;   
    ULONG       DecryptSuccessCount;   	
    ULONG       DecryptFailureCount;   	
} HostCmd_DS_802_11_GET_STAT , *PHostCmd_DS_802_11_GET_STAT;


#define MAX_POWER_ADAPT_GROUP       5

typedef struct _PA_Group_t {
    USHORT      PowerAdaptLevel;
    USHORT      RateBitmap;
    ULONG       Reserved;
} PA_Group_t;

/** MrvlIEtypes_PA_Group_t */
typedef struct _MrvlIEtypes_PowerAdapt_Group_t {
    MrvlIEtypesHeader_t Header;
    PA_Group_t      PA_Group[MAX_POWER_ADAPT_GROUP];
} MrvlIEtypes_PowerAdapt_Group_t;


/** HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT */
typedef struct _HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT {
       USHORT       Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;
    
    USHORT      Action;     /* 0 = ACT_GET; 1 = ACT_SET; */
    USHORT      EnablePA;   /* 0 = disable; 1 = enable; */
    MrvlIEtypes_PowerAdapt_Group_t  PowerAdaptGroup;
} HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT,*PHostCmd_DS_802_11_POWER_ADAPT_CFG_EXT;
/** HostCmd_DS_802_11_AFC */

//ahan [2005-12-13], Subscribe Event  ++dralee_20060217
#define SNR_HIGH            0x0020 
#define RSSI_HIGH           0x0010 
#define LINK_LOSS           0x0008
#define MAX_FAIL            0x0004
#define SNR_LOW             0x0002
#define RSSI_LOW            0x0001

#define LINK_LOST_BEACONCNT 60


#define TLV_PAYLOAD_SIZE    2

typedef struct _MrvlIEtypes_RssiThreshold_t {
    MrvlIEtypesHeader_t Header;
    UCHAR           RSSIValue;
    UCHAR           RSSIFreq;
} MrvlIEtypes_RssiParamSet_t;

typedef struct _MrvlIEtypes_SnrThreshold_t {
    MrvlIEtypesHeader_t Header;
    UCHAR           SNRValue;
    UCHAR           SNRFreq;
} MrvlIEtypes_SnrThreshold_t;

typedef struct _MrvlIEtypes_FailureCount_t {
    MrvlIEtypesHeader_t Header;
    UCHAR           FailValue;
    UCHAR           FailFreq;
} MrvlIEtypes_FailureCount_t;

typedef struct _MrvlIEtypes_BeaconsMissed_t {
    MrvlIEtypesHeader_t Header;
    UCHAR           BeaconMissed;
    UCHAR           Reserved;
} MrvlIEtypes_BeaconsMissed_t;

typedef struct _MrvlIEtypes_EventTLV_t {
    MrvlIEtypesHeader_t Header;
    UCHAR           data1;
    UCHAR           data2;
} MrvlIEtypes_EventTLV_t;

typedef struct _SubscibeEvent_t {
    USHORT          EventMap;
    UCHAR           RSSILowValue;
    UCHAR           RSSILowFreq;
    UCHAR           SNRLowValue;
    UCHAR           SNRLowFreq;
    UCHAR           FailValue;
    UCHAR           FailFreq;
    UCHAR           BeaconMissed;
    UCHAR           Reserved;
    UCHAR           RSSIHighValue;
    UCHAR           RSSIHighFreq;
    UCHAR           SNRHighValue;
    UCHAR           SNRHighFreq;
} SubscibeEvent_t;


typedef struct _HostCmd_DS_802_11_SUBSCRIBE_EVENT 
{
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;
    USHORT      Action;
    USHORT      Events;
    MrvlIEtypes_RssiParamSet_t      RssiLow;
    MrvlIEtypes_SnrThreshold_t      SnrLow;
    MrvlIEtypes_FailureCount_t      FailCnt;
    MrvlIEtypes_BeaconsMissed_t     BcnMiss;
    MrvlIEtypes_RssiParamSet_t      RssiHigh;
    MrvlIEtypes_SnrThreshold_t      SnrHigh;    
} HostCmd_DS_802_11_SUBSCRIBE_EVENT, *PHostCmd_DS_802_11_SUBSCRIBE_EVENT;

     

//++dralee_20060421
typedef struct MrvlIEAesCrypt_t
{
    MrvlIEtypesHeader_t  Header;
    UINT8                payload[1024];
} MrvlIEAesCrypt_t;



/** HostCmd_DS_802_11_KEY_ENCRYPT */
typedef struct _HostCmd_DS_802_11_KEY_ENCRYPT {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;

    USHORT  Action;
    USHORT  EncType;
    UCHAR   KeyIV[16];
    UCHAR   KeyEncKey[16];
    USHORT  KeyDataLen;
    UCHAR   KeyData[512];
} HostCmd_DS_802_11_KEY_ENCRYPT, *PHostCmd_DS_802_11_KEY_ENCRYPT;

#define CIPHER_TEST_RC4 (1)
#define CIPHER_TEST_AES (2)
#define CIPHER_TEST_AES_KEY_WRAP (3)

/** HostCmd_DS_802_11_CRYPTO */
typedef struct _HostCmd_DS_802_11_CRYPTO {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;

    USHORT  EncDec;         //Decrypt=0, Encrypt=1
    USHORT  Algorithm;      //RC4=1, AES=2, AES_KEY_WRAP=3
    USHORT  KeyIVLength;    //Length of Key IV (bytes)      
    UCHAR   KeyIV[32];      //Key IV
    USHORT  KeyLength;      //Length of Key (bytes)
    UCHAR   Key[32];        //Key
    //UCHAR   DataBuf[1024];
    MrvlIEAesCrypt_t TLVBuf; //++dralee_20060421
} HostCmd_DS_802_11_CRYPTO, *PHostCmd_DS_802_11_CRYPTO;

typedef struct _HostCmd_TX_RATE_QUERY 
{
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;
    USHORT      TxRate;
}HostCmd_TX_RATE_QUERY, *PHostCmd_TX_RATE_QUERY;

//dralee_20060601
typedef struct _HostCmd_DS_802_11_IBSS_Coalesing_Status {
    USHORT  Command;
    USHORT  Size;
    USHORT  SeqNum;
    USHORT  Result;
    USHORT  Action; 
    
    USHORT  Enable;
    UCHAR   bssid[6];
    USHORT  BeanconInterval;
    USHORT  AtimWindow; 
    USHORT  UseGRateProtection;
} HostCmd_802_11_IBSS_Coalesing_Status, *PHostCmd_802_11_IBSS_Coalesing_Status;

typedef struct _HostCmd_GET_TSF {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    ULONGLONG       TSF;
} HostCmd_DS_802_11_GET_TSF, *PHostCmd_DS_802_11_GET_TSF;

typedef struct _HostCmd_DS_802_11_WMM_ADDTS {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    UCHAR       TspecResult;
    ULONG       TimeOut;
    UCHAR       DialogToken;
    UCHAR       IEEEStatus;
    UCHAR       TspecData[63];
    UCHAR       ExtraIeData[256];

} HostCmd_DS_802_11_WMM_ADDTS, *PHostCmd_DS_802_11_WMM_ADDTS;

typedef struct _HostCmd_DS_802_11_WMM_DELTS {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    UCHAR       TspecResult;
    UCHAR       DialogToken;
    UCHAR       IEEEReasonCode;
    UCHAR       TspecData[63];
} HostCmd_DS_802_11_WMM_DELTS, *PHostCmd_DS_802_11_WMM_DELTS;

typedef struct _HostCmd_DS_802_11_WMM_QUEUE_CONFIG {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    UCHAR       Action;
    UCHAR       AC;
    USHORT      MsduLifeExpiry;
} HostCmd_DS_802_11_WMM_QUEUE_CONFIG, *PHostCmd_DS_802_11_WMM_QUEUE_CONFIG;

typedef struct _DS_WMM_QUEUE_STATS {
    UCHAR       Action;
    UCHAR       AC;

    USHORT      PktCount;
    USHORT      PktLoss;
    ULONG       AvgQueueDelay;
    ULONG       AvgTxDelay;
    ULONG       Resv;
    USHORT      DelayHistogram[7];
    UCHAR		RoamingCount;
    USHORT		RoamingDelay;
} DS_802_11_WMM_QUEUE_STATS, *PDS_802_11_WMM_QUEUE_STATS;
typedef struct _HostCmd_DS_802_11_WMM_QUEUE_STATS {
    USHORT      Command;
    USHORT      Size;
    USHORT      SeqNum;
    USHORT      Result;

    UCHAR       Action;
    UCHAR       AC;

    USHORT      PktCount;
    USHORT      PktLoss;
    ULONG       AvgQueueDelay;
    ULONG       AvgTxDelay;
    ULONG       Resv;
    USHORT      DelayHistogram[7];
} HostCmd_DS_802_11_WMM_QUEUE_STATS, *PHostCmd_DS_802_11_WMM_QUEUE_STATS;







#pragma pack()
#define ABS(x)    (((x) >= 0) ? (x): (-x))
#endif /* __HOSTCMD__H */

