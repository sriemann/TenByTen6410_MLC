/******************* Marvell Semiconductor, Inc., *****************
 *
 *  Purpose:    This module provides the implementaion of NDIS miniport 
 *              dirver initialization, reset and check for hang routines.
 *
 *  $Author: schiu $
 *
 *  $Date: 2005/01/19 $
 *
 *  $Revision: #10 $
 *
 *****************************************************************************/
/*
===============================================================================
            INCLUDE FILES
===============================================================================
*/
#include "precomp.h"

#include "wlan_roam.h"
#include "wlan_thread.h"

// cardservices support
#include <cardserv.h>
#include <cardapi.h>
#include <tuple.h>
#include "If.h"
#include <celog.h>
   
// tt ++ loader issue
#define DLS_DRIVER_NOT_LOAD         0
#define DLS_DRIVER_LOADED_OK       1

// Define this to non-zero to add CELOG info for debugging
#define WLANDRV_CELOG_TRACE         TRUE


unsigned char lptRegImage1 = 0;
unsigned char lptRegImage2 = 0;

ULONG DebugLevel = DBG_LVLDEFAULT;
unsigned long long DebugModule = DBG_MDLDEFAULT;
FILE     *MrvDbgfp = NULL;


BOOL g_fMrvDrvHaltCalled = FALSE;


HANDLE g_hLoaderThread = NULL;
CRITICAL_SECTION    LoaderCriticalSection;

///////////////////////////////////////////////////////////////////////////////
//  LoaderEntry - init loader
//  Input:  hInstance - the instance that is attaching
//          Reason - the reason for attaching
//          pReserved - not much
//  Output: 
//  Return: Always returns TRUE
//  Notes:  this is only used to initialize the zones
///////////////////////////////////////////////////////////////////////////////
BOOL LoaderEntry(HINSTANCE  hInstance,
                 ULONG      Reason,
                 LPVOID     pReserved)
{
    if ( Reason == DLL_PROCESS_ATTACH ) {
        DEBUGREGISTER(hInstance);
        InitializeCriticalSection(&LoaderCriticalSection);
        g_hLoaderThread = NULL;
    }

    if ( Reason == DLL_PROCESS_DETACH ) {
        DeleteCriticalSection(&LoaderCriticalSection);
    }

    return(TRUE);
}



/*
===============================================================================
            CODED PUBLIC VARIABLE
===============================================================================
*/

//
//          Registry table, works with INF file and ReadRegistryInfo() to provide a 
//          convenient way for device specific registry information reading. 
//
static REGINFOTAB MrvDrvRegInfoTab[] = { 
    {NDIS_STRING_CONST("DesiredSSID"),          "DesiredSSID",          1, FIELD_OFFSET(MRVDRV_ADAPTER, DesiredSSID)+4,             32},
    {NDIS_STRING_CONST("NetWorkMode"),          "NetworkMode",          0, FIELD_OFFSET(MRVDRV_ADAPTER, InfrastructureMode),        4},   
    {NDIS_STRING_CONST("AuthMode"),             "AuthMode",             0, FIELD_OFFSET(MRVDRV_ADAPTER, AuthenticationMode),        4},
    {NDIS_STRING_CONST("TxAntenna"),            "TxAntenna",            3, FIELD_OFFSET(MRVDRV_ADAPTER, TxAntenna),                 4},
    {NDIS_STRING_CONST("RxAntenna"),            "RxAntenna",            3, FIELD_OFFSET(MRVDRV_ADAPTER, RxAntenna),                 4},
    {NDIS_STRING_CONST("Channel"),              "Channel",              0, FIELD_OFFSET(MRVDRV_ADAPTER, Channel),                   4},
    {NDIS_STRING_CONST("DataRate"),             "DataRate",             3, FIELD_OFFSET(MRVDRV_ADAPTER, DataRate),                  4},
    {NDIS_STRING_CONST("FragThsd"),             "FragThsd",             0, FIELD_OFFSET(MRVDRV_ADAPTER, FragThsd),                  4},
    {NDIS_STRING_CONST("RTSThsd"),              "RTSThsd",              0, FIELD_OFFSET(MRVDRV_ADAPTER, RTSThsd),                   4},
    {NDIS_STRING_CONST("MaxChanTime"),          "MaxChanTime",          0, FIELD_OFFSET(MRVDRV_ADAPTER, DefaultPerChannelScanTime), 4},
    {NDIS_STRING_CONST("Preamble"),             "Preamble",             0, FIELD_OFFSET(MRVDRV_ADAPTER, Preamble),                  4},
    {NDIS_STRING_CONST("PowerMode"),            "PowerMode",            0, FIELD_OFFSET(MRVDRV_ADAPTER, PSMode),                    4},
    {NDIS_STRING_CONST("RoamingMode"),            "RoamingMode",          0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamingMode),               4},
    //{NDIS_STRING_CONST("RoamingRSSILevel"),       "RoamingRSSILevel",     0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamingLevel),              1},
    //{NDIS_STRING_CONST("RoamingPeriod"),      "RoamingPeriod",        0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamingPeriod),             4},
    {NDIS_STRING_CONST("BTMode"),               "BTMode",               0, FIELD_OFFSET(MRVDRV_ADAPTER, BTMode),                    4},
    {NDIS_STRING_CONST("BTTrafficType"),        "BTTrafficType",        0, FIELD_OFFSET(MRVDRV_ADAPTER, BTTrafficType),        4},
    {NDIS_STRING_CONST("BTTimeShareInterval"),  "BTTimeShareInterval",  0, FIELD_OFFSET(MRVDRV_ADAPTER, BTTimeShareInterval),        4},
    {NDIS_STRING_CONST("BTTime"),               "BTTime",               0, FIELD_OFFSET(MRVDRV_ADAPTER, BTTime),        4},
    {NDIS_STRING_CONST("QOS"),                  "QOS",                  0, FIELD_OFFSET(MRVDRV_ADAPTER, EnableQOS),                 4},
    {NDIS_STRING_CONST("WepStatus"),            "WepStatus",            0, FIELD_OFFSET(MRVDRV_ADAPTER, WEPStatus),                 4},
    {NDIS_STRING_CONST("TxWepKey"),             "TxWepKey",             0, FIELD_OFFSET(MRVDRV_ADAPTER, TxWepKey),                  4},
    {NDIS_STRING_CONST("TxPower"),              "TxPower",              0, FIELD_OFFSET(MRVDRV_ADAPTER, TxPower),                   4},
    {NDIS_STRING_CONST("WepKey1"),              "WepKey1",              1, FIELD_OFFSET(MRVDRV_ADAPTER, WepKey1)+12,                16},
    {NDIS_STRING_CONST("WepKey2"),              "WepKey2",              1, FIELD_OFFSET(MRVDRV_ADAPTER, WepKey2)+12,                16},
    {NDIS_STRING_CONST("WepKey3"),              "WepKey3",              1, FIELD_OFFSET(MRVDRV_ADAPTER, WepKey3)+12,                16},
    {NDIS_STRING_CONST("WepKey4"),              "WepKey4",              1, FIELD_OFFSET(MRVDRV_ADAPTER, WepKey4)+12,                16},
    {NDIS_STRING_CONST("AdhocDefaultBand"),     "AdhocDefaultBand",     0, FIELD_OFFSET(MRVDRV_ADAPTER, AdhocDefaultBand),          4},
    {NDIS_STRING_CONST("AdhocDefaultChannel"),  "AdhocDefaultChannel",  0, FIELD_OFFSET(MRVDRV_ADAPTER, AdhocDefaultChannel),       4},    
    {NDIS_STRING_CONST("AdhocWiFiDataRate"),    "AdhocWiFiDataRate",    0, FIELD_OFFSET(MRVDRV_ADAPTER, AdhocWiFiDataRate),         4},
    {NDIS_STRING_CONST("LocalListenInterval"),  "LocalListenInterval",  0, FIELD_OFFSET(MRVDRV_ADAPTER, LocalListenInterval),       4},
    {NDIS_STRING_CONST("SetSD4BIT"),            "SetSD4BIT",            0, FIELD_OFFSET(MRVDRV_ADAPTER, SetSD4BIT),                 4},  
    {NDIS_STRING_CONST("SdioIstThread"),        "SdioIstThread",        0, FIELD_OFFSET(MRVDRV_ADAPTER, SdioIstThread),             4},
    {NDIS_STRING_CONST("AvoidScanTime"),        "AvoidScanTime",        0, FIELD_OFFSET(MRVDRV_ADAPTER, AvoidScanTime),             4},
    {NDIS_STRING_CONST("ulRSSIThresholdTimer"), "ulRSSIThresholdTimer", 0, FIELD_OFFSET(MRVDRV_ADAPTER, ulRSSIThresholdTimer),      4},
    {NDIS_STRING_CONST("RSSI_Range"),           "RSSI_Range",           0, FIELD_OFFSET(MRVDRV_ADAPTER, RSSI_Range),                4},
    {NDIS_STRING_CONST("Enable80211D"),         "Enable80211D",         0, FIELD_OFFSET(MRVDRV_ADAPTER, Enable80211D),              4},
    {NDIS_STRING_CONST("DefaultRegion"),        "DefaultRegion",        0, FIELD_OFFSET(MRVDRV_ADAPTER, DefaultRegion),             4},
    {NDIS_STRING_CONST("DefaultBand"),          "DefaultBand",          0, FIELD_OFFSET(MRVDRV_ADAPTER, DefaultBand),               4},
    ///crlo:filter_32_ESSID ++
    {NDIS_STRING_CONST("ESSID_32"),             "ESSID_32",             0, FIELD_OFFSET(MRVDRV_ADAPTER, ESSID_32),                  4},
    ///crlo:filter_32_ESSID --
    {NDIS_STRING_CONST("NullPktInterval"),  "NullPktInterval",  0, FIELD_OFFSET(MRVDRV_ADAPTER, NullPktInterval),       4},  
    {NDIS_STRING_CONST("MultipleDTim"),  "MultipleDTim",  0, FIELD_OFFSET(MRVDRV_ADAPTER, MultipleDTim),       4},  
    {NDIS_STRING_CONST("AdhocAwakePeriod"),  "AdhocAwakePeriod",  0, FIELD_OFFSET(MRVDRV_ADAPTER, AdhocAwakePeriod),       4},  
    {NDIS_STRING_CONST("bActiveRoamingwithBGSCAN"),"bActiveRoamingwithBGSCAN",0, FIELD_OFFSET(MRVDRV_ADAPTER, bActiveRoamingwithBGSCAN),4},
    {NDIS_STRING_CONST("SdioFastPath"),"SdioFastPath",0, FIELD_OFFSET(MRVDRV_ADAPTER, SdioFastPath),4}, 
    {NDIS_STRING_CONST("RoamSignalStrengthThreshold"), "RoamSignalStrengthThreshold", 0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamSignalStrengthThreshold), 4},
/// {NDIS_STRING_CONST("DesiredSSID"),          "DesiredSSID",          1, FIELD_OFFSET(MRVDRV_ADAPTER, DesiredSSID)+4,             32},
    {NDIS_STRING_CONST("RoamChannelScanList"),"RoamChannelScanList",             0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamChannelScanList), 4},
    {NDIS_STRING_CONST("RoamMaxScanInterval"),"RoamMaxScanInterval",             0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamMaxScanInterval), 4},
    {NDIS_STRING_CONST("RoamMinScanInterval"),"RoamMinScanInterval",             0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamMinScanInterval), 4},
    {NDIS_STRING_CONST("RoamDiffRSSIThreshold"),"RoamDiffRSSIThreshold",             0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamDiffRSSIThreshold), 4},
    {NDIS_STRING_CONST("RoamScanAlgorithm"),"RoamScanAlgorithm",             0, FIELD_OFFSET(MRVDRV_ADAPTER, RoamScanAlgorithm), 4},
    {NDIS_STRING_CONST("UseMfgFw"),"UseMfgFw",0, FIELD_OFFSET(MRVDRV_ADAPTER, UseMfgFw),4},
    {NDIS_STRING_CONST("GPIOIntPinNumber"),"GPIOIntPinNumber",0, FIELD_OFFSET(MRVDRV_ADAPTER, GPIOIntPinNumber),4},
    {NDIS_STRING_CONST("GPIOIntTriggerEdge"),"GPIOIntTriggerEdge",0, FIELD_OFFSET(MRVDRV_ADAPTER, GPIOIntTriggerEdge),4},
    {NDIS_STRING_CONST("GPIOIntPulsewidth"),"GPIOIntPulsewidth",0, FIELD_OFFSET(MRVDRV_ADAPTER, GPIOIntPulsewidth),4},
    {NDIS_STRING_CONST("MacFrameType"),"MacFrameType",0, FIELD_OFFSET(MRVDRV_ADAPTER, MacFrameType),4},
    {NDIS_STRING_CONST("AutoDeepSleepTime"),"AutoDeepSleepTime",0, FIELD_OFFSET(MRVDRV_ADAPTER, AutoDeepSleepTime),4},
    {NDIS_STRING_CONST("AssoRetryTimes"),"AssoRetryTimes",0, FIELD_OFFSET(MRVDRV_ADAPTER, AssoRetryTimes),4},
    {NDIS_STRING_CONST("BusPowerInD3"), "BusPowerInD3", 0, FIELD_OFFSET(MRVDRV_ADAPTER, BusPowerInD3), 4}
    };
   
#define SIZE_OF_REG_TAB (sizeof(MrvDrvRegInfoTab)/sizeof(REGINFOTAB))


/*
===============================================================================
            CODED PUBLIC PROCEDURES
===============================================================================
*/
/* Add the coded public routines here, each with their own prologue. */

// jeff.spurgat 2003-04-02
// DllEntry is required for WinCE driver
//-----------------------------------------------------------------------------
// Function   : DllEntry
// This function provides the DLL entry point required for the WinCE 
// driver. Since all network drivers are implemented as DLL's in WinCE, 
// this entry point is needed.
//
// Inputs     : HANDLE hDLL, handle identifying this DLL.
//
//              DWORD dwReason, reason of invocation of this entry point.
//
//              LPVOID lpReserved, reserved param
//
// Outputs    : None.
//
// Returns    : TRUE - don't really know what this function is used
//                     for, it has basically been reproduced here from 
//                     the WinCE sample driver. Just safer to return TRUE
//                     than FALSE :-)
//
//-----------------------------------------------------------------------------
BOOL __stdcall
DllEntry( HANDLE hDLL,
          DWORD dwReason,
          LPVOID lpReserved )
{
    switch (dwReason) 
    {
        case DLL_PROCESS_ATTACH:
            DEBUGMSG(1, (TEXT("MRVDRVND5: DLL_PROCESS_ATTACH\n")));

            DisableThreadLibraryCalls ((HMODULE)hDLL);
            break;
        case DLL_PROCESS_DETACH:
            DEBUGMSG(1, (TEXT("MRVDRVND5: DLL_PROCESS_DETACH\n")));
            break;
    }

    return LoaderEntry(hDLL,dwReason,lpReserved);
}


// jeff.spurgat 2003-04-02
// Added AddKeyValues() function, which is used by Install_Driver() to set
// registry key entries. This function was taken from samples in Platform
// Builder.
/****************************************************************************
 * Function   : AddKeyValues
 * This function adds the specified key and its value into the
 * registry under HKEY_LOCAL_MACHINE.
 *
 * Inputs     : LPWSTR KeyName, name of the key to be added to the registry.
 *
 *              PREG_VALUE_DESCR Vals, value associated with this key.
 *
 * Outputs    : None.
 *
 * NOTE       : 1. This function only supports REG_MULTI_SZ strings with 
 *                 one item.
 *              2. Assumes that input Value for the specified Key is NULL
 *                 terminated.
 *
 * Returns    : TRUE  - if successfully added Key & Value
 *              FALSE - if failed to add Key & Value
 *
 ***************************************************************************/
BOOL
AddKeyValues (LPWSTR KeyName, PREG_VALUE_DESCR Vals)
{
    DWORD Status;
    DWORD dwDisp;
    HKEY hKey;
    PREG_VALUE_DESCR pValue;
    DWORD ValLen;
    PBYTE pVal;
    DWORD dwVal;
    LPWSTR pStr;

    Status = RegCreateKeyEx (HKEY_LOCAL_MACHINE, KeyName, 0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, &dwDisp);

    if (Status != ERROR_SUCCESS)
        return (FALSE);

    pValue = Vals;
    while (pValue->val_name)
    {
        switch (pValue->val_type)
        {
          case REG_DWORD:
            pVal = (PBYTE)&dwVal;
            dwVal = (DWORD)pValue->val_data;
            ValLen = sizeof (DWORD);
            break;
          case REG_SZ:
            pVal = (PBYTE)pValue->val_data;
            ValLen = (wcslen ((LPWSTR)pVal) + 1) * sizeof (WCHAR);
            break;
          case REG_MULTI_SZ:
            dwVal = wcslen ((LPWSTR)pValue->val_data);
            ValLen = (dwVal + 2) * sizeof (WCHAR);
            pVal = LocalAlloc (LPTR, ValLen);
            if (pVal == NULL)
            {
              RegCloseKey (hKey);
              return (FALSE);
            }
            wcscpy ((LPWSTR)pVal, (LPWSTR)pValue->val_data);
            pStr = (LPWSTR)pVal + dwVal;
            pStr[1] = 0;
            break;
          default: //Coverity Error id:8,9 (UNINIT)
            ValLen = 0;
            pVal = NULL; 
            return (FALSE);
        } 

        Status = RegSetValueEx (hKey, pValue->val_name, 0, pValue->val_type, pVal, ValLen);
        if (pValue->val_type == REG_MULTI_SZ)
            LocalFree (pVal);
        if (Status != ERROR_SUCCESS)
        {
            RegCloseKey (hKey);
            return (FALSE);
        }
        pValue++;
    }
    RegCloseKey (hKey);
    return (TRUE);
}

/******************************************************************************
 *
 *  Name: DriverEntry()
 *
 *  Description:  
 *    NDIS miniport driver primary driver entry. 
 *
 *  Arguments:
 *      DriverObject - Pointer to driver object created by the system.
 *      RegistryPath - The registry path of this driver
 *    
 *  Return Value:
 *
 *    NDIS_STATUS_SUCCESS 
 *    NDIS_STATUS_BAD_CHARACTERISTICS 
 *    NDIS_STATUS_BAD_VERSION 
 *    NDIS_STATUS_RESOURCES 
 *    NDIS_STATUS_FAILURE
 * 
 *  Notes:                
 *
 *****************************************************************************/
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
  )
{
    NDIS_STATUS Status;
    NDIS_HANDLE NdisWrapperHandle;

    NDIS_MINIPORT_CHARACTERISTICS MrvDrvNMC;

    //INITDEBUG();

    DBGPRINT(DBG_ERROR,(L"INIT - Enter DriverEntry\n"));


    DBGPRINT(DBG_ERROR, (L"*** Driver version %s ***\n",
                        TEXT(MRVDRV_DRIVER_BUILD_VERSION)));
    DBGPRINT(DBG_ERROR, (L"*** Built on %s %s ***\n",
                         TEXT(__DATE__), 
                         TEXT(__TIME__)));

    // Now we must initialize the wrapper, and then register the Miniport
    NdisMInitializeWrapper(&NdisWrapperHandle, DriverObject, RegistryPath, NULL);


    NdisZeroMemory(&MrvDrvNMC, sizeof(MrvDrvNMC));

    // Initialize the Miniport characteristics for NdisMRegisterMiniport.
    MrvDrvNMC.MajorNdisVersion         = MRVDRV_NDIS_MAJOR_VERSION;
    MrvDrvNMC.MinorNdisVersion         = MRVDRV_NDIS_MINOR_VERSION;
    MrvDrvNMC.InitializeHandler        = MrvDrvInitialize;
    MrvDrvNMC.ResetHandler             = MrvDrvReset;
    MrvDrvNMC.CheckForHangHandler      = MrvDrvCheckForHang;
  
    MrvDrvNMC.HandleInterruptHandler   = NULL;
    MrvDrvNMC.DisableInterruptHandler  = NULL;  //MrvDrvDisableInterrupt;
    MrvDrvNMC.EnableInterruptHandler   = NULL;   //MrvDrvEnableInterrupt;
    MrvDrvNMC.ISRHandler               = NULL;

    MrvDrvNMC.QueryInformationHandler  = MrvDrvQueryInformation;
    MrvDrvNMC.SetInformationHandler    = MrvDrvSetInformation;  

    MrvDrvNMC.AllocateCompleteHandler  = MrvDrvAllocateComplete;
    MrvDrvNMC.ReconfigureHandler       = NULL;
    MrvDrvNMC.PnPEventNotifyHandler    = MrvDrvPnPEventNotify;
    MrvDrvNMC.CancelSendPacketsHandler = NULL;
    MrvDrvNMC.AdapterShutdownHandler   = MrvDrvShutdownHandler;
    //++dralee_20060418 don't remove this call back. unplug card need this to release resource. 
    MrvDrvNMC.HaltHandler              = MrvDrvHalt;
    //MrvDrvNMC.HaltHandler              = NULL;

    MrvDrvNMC.TransferDataHandler      = NULL;
    MrvDrvNMC.ReturnPacketHandler      = MrvDrvReturnPacket;   

    MrvDrvNMC.SendHandler              = MrvDrvSend;
    MrvDrvNMC.SendPacketsHandler       = NULL;    

    //  Register this driver with the NDIS wrapper
    //  This will cause MrvDrvInitialize to be called before returning
  Status = NdisMRegisterMiniport(
        NdisWrapperHandle,
        &MrvDrvNMC,
        sizeof(NDIS_MINIPORT_CHARACTERISTICS));

  if (Status == NDIS_STATUS_SUCCESS)
  { 
    DBGPRINT(DBG_LOAD,(L"<== DriverEntry: register miniport success\n"));
    //RETAILMSG(1,(TEXT("<== DriverEntry: register miniport success")));
    return NDIS_STATUS_SUCCESS; 
  }
  
  NdisTerminateWrapper(NdisWrapperHandle, NULL);
  DBGPRINT(DBG_LOAD,(L"<== INIT - DriverEntry: register miniport failed!!\n"));
  //RETAILMSG(1,(TEXT("<== INIT - DriverEntry: register miniport failed!!")));
    return Status;
}


///Fix Bug#17113 ++
VOID ResetFastRoamParam(PFASTROAMPARAM      pfrParam)
{
	pfrParam->is80211x = FALSE;
	return;
}
///Fix Bug#17113 --
static VOID InitFastRoamParam(PMRVDRV_ADAPTER pAdapter, PFASTROAMPARAM  pfrParam)
{
	pfrParam->is80211x = FALSE;
	return;
}


///Initalize the roaming Mode
NDIS_STATUS InitRoamingMode(PMRVDRV_ADAPTER pAdapter)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    ResetFastRoamParam(&pAdapter->frParam);
    switch (pAdapter->RoamingMode) {
        case FAST_ROAMING_MODE:
        ///#ifdef CCX		///CCX_FASTROAM
	///	InitFastRoamParam(pAdapter, &pAdapter->frParam);
        ///#endif			///CCX_FASTROAM
		break;
        case SMLS_ROAMING_MODE:
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"RoamSignalStrengthThreshold: %d\n", pAdapter->RoamSignalStrengthThreshold));
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"RoamChannelScanList: %x\n", pAdapter->RoamChannelScanList));
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"RoamMaxScanInterval: %d ms\n", pAdapter->RoamMaxScanInterval));
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"RoamMinScanInterval: %d ms\n", pAdapter->RoamMinScanInterval));
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"RoamDiffRSSIThreshold: %d \n", pAdapter->RoamDiffRSSIThreshold));
            DBGPRINT(DBG_ROAM|DBG_HELP, (L"RoamScanAlgorithm: %d \n", pAdapter->RoamScanAlgorithm));
            //InitFastRoamParam(pAdapter, &pAdapter->frParam);
            ///===========================================================  
            NdisZeroMemory(&pAdapter->wlanRoamExtParam, sizeof(WROAMEXTPARAM));
            pAdapter->wlanRoamExtParam.RoamSST = pAdapter->RoamSignalStrengthThreshold;
            ///Supported channel list: 0x0000 ~ 0x3ffff (14 channels)
            pAdapter->wlanRoamExtParam.ChannelList = (WORD)(pAdapter->RoamChannelScanList & 0x03fff);
            pAdapter->wlanRoamExtParam.MaxScanInterval = pAdapter->RoamMaxScanInterval;
            pAdapter->wlanRoamExtParam.MinScanInterval = pAdapter->RoamMinScanInterval;
            pAdapter->wlanRoamExtParam.diffRSSI = pAdapter->RoamDiffRSSIThreshold;
            pAdapter->wlanRoamExtParam.ScanAlgorithm = pAdapter->RoamScanAlgorithm;
            pAdapter->pwlanRoamParam = wlan_roam_init(pAdapter, &pAdapter->wlanRoamExtParam);
            pAdapter->pReconectArg = wlan_roam_GetReconnectArg(pAdapter->pwlanRoamParam);
            break;
    }
    return Status;
}

/******************************************************************************
 *
 *  Name: MrvDrvInitialize()
 *
 *  Description: 
 *      NDIS miniport initialization routine, upper layer interface to the NDIS wrapper.
 *      This routine will check medium support and call rsource allocation and HW 
 *      initialization routines to set up the staion.
 *
 *  Conditions for Use: 
 *      Will be called by NDIS wrapper to initialize the device
 *
 *  Arguments: 
 *      OUT PNDIS_STATUS OpenErrorStatus,
 *      OUT PUINT SelectedMediumIndex,
 *      IN PNDIS_MEDIUM MediumArray,
 *      IN UINT MediumArraySize,
 *      IN NDIS_HANDLE MiniportAdapterHandle,
 *      IN NDIS_HANDLE WrapperConfigurationContext
 *    
 *  Return Value:         
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_FAILURE
 * 
 *  Notes:                
 *
 *****************************************************************************/
NDIS_STATUS
MrvDrvInitialize(
  OUT PNDIS_STATUS OpenErrorStatus,
  OUT PUINT SelectedMediumIndex,
  IN PNDIS_MEDIUM MediumArray,
  IN UINT MediumArraySize,
  IN NDIS_HANDLE MiniportAdapterHandle,
  IN NDIS_HANDLE WrapperConfigurationContext
  )
{
	ULONG i;
	NDIS_STATUS Status;
	PMRVDRV_ADAPTER Adapter;
	NDIS_HANDLE RegHdl;   

	IF_FW_STATUS      fwStatus; 
	i =0;
	DBGPRINT(DBG_LOAD|DBG_CUSTOM,(L"INIT - Enter MrvDrvInitialize \n"));

	///Display the driver version to message
	DBGPRINT(DBG_LOAD, (L"[WiFi]: Driver Version: %s\n", TEXT(MRVDRV_DRIVER_BUILD_VERSION) ) );


    // unblocks eject notification
    g_fMrvDrvHaltCalled = FALSE;

  //RETAILMSG(1,(TEXT("INIT - Enter MrvDrvInitialize(%xh, %xh)"), (int)MiniportAdapterHandle, (int)WrapperConfigurationContext));
//???dralee-------------------------------------------------------- 

//???---------------------------------------------------------------------
	// Only support medium type 802.3
	for( i = 0; i < MediumArraySize; i++ )
	{
		if( MediumArray[i] == NdisMedium802_3 ) 
			break;
	}

	// If 802.3 is not found, return error
	if (i == MediumArraySize)
	{
            DBGPRINT(DBG_LOAD,(L"INIT - Leave MrvDrvInitialize NDIS_STATUS_UNSUPPORTED_MEDIA\n"));
		return NDIS_STATUS_UNSUPPORTED_MEDIA;
	}

	// Select medium type 802.3
	*SelectedMediumIndex = i;

	// Allocate adapter handler
	Status = MRVDRV_ALLOC_MEM(&Adapter, sizeof(MRVDRV_ADAPTER));
	if( Status != NDIS_STATUS_SUCCESS )
	{
		DBGPRINT(DBG_ERROR | DBG_LOAD, (L"Unable to allocate adapter memory, Status = 0x%x\n", Status));
            DBGPRINT(DBG_LOAD,(L"INIT - Leave MrvDrvInitialize alloc\n"));
		return Status;
	}

    // Zero out the adapter object space
    NdisZeroMemory(Adapter, sizeof(MRVDRV_ADAPTER));
    Adapter->MrvDrvAdapterHdl = MiniportAdapterHandle;
    Adapter->MrvDrvWrapperConfigurationContext = WrapperConfigurationContext;
	Adapter->ConfigurationHandle = WrapperConfigurationContext;
	Adapter->ShutDown = FALSE;
	//************************************************************
	//[1]  Assign default adapter object value and allocate buffer
	//************************************************************     
	InitAdapterObject(Adapter);  
	//************************************************************
	//[2] Read configuration from registry
	//************************************************************
    NdisOpenConfiguration(
	    &Status,
        &RegHdl,
        WrapperConfigurationContext);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		FreeAdapterObject(Adapter);
                DBGPRINT(DBG_LOAD,(L"INIT - Leave MrvDrvInitialize fail1\n"));
		return NDIS_STATUS_FAILURE;
	}
  
    Status = ReadRegistryInfo(Adapter, RegHdl);
    if(Status != NDIS_STATUS_SUCCESS )
    {
        FreeAdapterObject(Adapter);
        NdisCloseConfiguration(RegHdl);
        DBGPRINT(DBG_LOAD,(L"INIT - Leave MrvDrvInitialize fail2\n"));
        return Status;
    }
    NdisCloseConfiguration(RegHdl);
	///Give up "Fast_Roaming" mode, and use "seamless_roaming" instead
	if (Adapter->RoamingMode == FAST_ROAMING_MODE) {
		Adapter->RoamingMode =SMLS_ROAMING_MODE;
	}

    if (Adapter->UseMfgFw != 1)
    {
        //Initialize the roaming mode
        InitRoamingMode(Adapter);
    }

	////////////////////////////////////////////////////////////////
	//[3]
	if ( If_Initialize( Adapter, WrapperConfigurationContext ) != NDIS_STATUS_SUCCESS )
	{
		// Only adapter object itself has been created up to this point
		MRVDRV_FREE_MEM((PVOID)Adapter, sizeof(MRVDRV_ADAPTER));
		DBGPRINT(DBG_LOAD | DBG_ERROR, (L"*** SDIOInitialization FAILED! ***\n"));
		// tt ++ test loader
		DBGPRINT(DBG_LOAD | DBG_ERROR,(L"[TT] SDIO initialization is failed! Quit initialization process\r\n" ));
		// tt --
		return NDIS_STATUS_FAILURE;      
    }
      
	// Call NdisMSetAttributesEx to inform NDIS NIC features
	// Set the time-out for unreturned OID to be 10 seconds 
	NdisMSetAttributesEx(Adapter->MrvDrvAdapterHdl,
							(NDIS_HANDLE) Adapter,
							MRVL_CHECK_FOR_HANG_TIME,  
							(ULONG)NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK,
							NdisInterfaceInternal); 
	//  Assign default adapter object value and allocate buffer
    InitializeCriticalSection(&Adapter->CmdQueueExeSection);
    // command
	NdisAllocateSpinLock(&Adapter->FreeQSpinLock); 
	NdisAllocateSpinLock(&Adapter->PendQSpinLock); 
	NdisAllocateSpinLock(&Adapter->RxQueueSpinLock);
	Status = AllocateAdapterBuffer(Adapter);

	// Initialize sync object
	InitSyncObjects(Adapter);

	wlan_ccx_init(Adapter);
	///CCX_FASTROAM
	Adapter->EventRecord.RSSILowValue  =  (UCHAR)Adapter->RoamSignalStrengthThreshold;
	///CCX_FASTROAM

	// SDIO FW Download
    RETAILCELOGMSG(WLANDRV_CELOG_TRACE, (L"SDIO8686: FW Download: Start\n"));
	fwStatus = If_FirmwareDownload(Adapter);
    RETAILCELOGMSG(WLANDRV_CELOG_TRACE, (L"SDIO8686: FW Download: End\n"));
    
	if ( fwStatus != FW_STATUS_INITIALIZED  )
	{
        DBGPRINT(DBG_LOAD, (L"Failed to download the firmware\n"));
        MrvDrvHalt(Adapter);
        DBGPRINT(DBG_LOAD,(L"INIT - Leave MrvDrvInitialize fail4\n"));
        return NDIS_STATUS_FAILURE;
    }
	IF_BUS_CONFIG;
	NdisMSleep(IF_WAITING_FW_BOOTUP);


	/// Power-Save
	if ( Adapter->PSMode != 0)
       Adapter->PSMode = Ndis802_11PowerModeMAX_PSP;

	DBGPRINT(DBG_PS|DBG_WARNING, (L"Adapter PSMODE = %d\n", Adapter->PSMode));
	IF_RELEASE_CPU_TIME;

    
    if(!InitializeWirelessConfig(Adapter))
    {
        MrvDrvHalt(Adapter);
        DBGPRINT(DBG_LOAD,(L"INIT - Leave MrvDrvInitialize fail5\n"));
        return NDIS_STATUS_FAILURE;      
    }

	/// Add the FW version and Driver Version to Registry   
	{   // Add driver version to registry
        REG_VALUE_DESCR Vals[2];

        Vals[0].val_data = (PBYTE)TEXT(MRVDRV_DRIVER_BUILD_VERSION);
        Vals[0].val_name = TEXT("DriverVersion");
        Vals[0].val_type = REG_SZ;
        Vals[1].val_name = NULL;
        AddKeyValues ((TEXT("Comm\\") TEXT(IFSTRN) TEXT(CHIPSTRN)), Vals);
	}


    DBGPRINT(DBG_TMP, (L"->Init: Register RSSI_LOW: %d\r\n", Adapter->EventRecord.RSSILowValue ));
    Adapter->NeedDisconnectNotification = FALSE;
    Enable_11d(Adapter, (UCHAR)Adapter->Enable80211D);   
    ResetAllScanTypeAndPower(Adapter);

    if ( Adapter->MacFrameType == 0 )
    {
        DBGPRINT( DBG_LOAD, ( L"[Mrvl] MAC frame type is 802.3+LLC\n" ));
        Adapter->CurrentMacControl &= ~HostCmd_ACT_MAC_FRAME_TYPE; // it is used to enable "802.3+LLC" packet!
        SetMacControl( Adapter );
    }
    else
    {
        DBGPRINT( DBG_LOAD, ( L"[Mrvl] MAC frame type is Ethernet II\n" ));
        Adapter->CurrentMacControl |= HostCmd_ACT_MAC_FRAME_TYPE; // it is used to enable Ethernet II packet!
        SetMacControl( Adapter );
    }



    DBGPRINT(DBG_LOAD,(L"INIT - Leave MrvDrvInitialize \n"));
    return NDIS_STATUS_SUCCESS; 
}

/******************************************************************************
 *
 *  Name: MrvDrvAllocateComplete()
 *
 *  Description: 
 *      NDIS miniport memory allocation complete event handler
 *
 *  Conditions for Use:
 *      NDIS wrapper will call this function when an async memory allocation is completed.
 *
 *  Arguments:   
 *    
 *      NDIS_HANDLE MiniportAdapterContext
 *      IN PVOID VirtualAddress
 *      IN PNDIS_PHYSICAL_ADDRESS PhysicalAddress
 *      IN ULONG Length
 *      IN PVOID Context
 *
 *  Return Value: None
 * 
 *  Notes: This routine is not implemented in this version of driver.
 *
 *****************************************************************************/
VOID
MrvDrvAllocateComplete(
  NDIS_HANDLE MiniportAdapterContext,
  IN PVOID VirtualAddress,
  IN PNDIS_PHYSICAL_ADDRESS PhysicalAddress,
  IN ULONG Length,
  IN PVOID Context
  )
{
  DBGPRINT(DBG_LOAD,(L"INIT - Enter MrvDrvAllocateComplete \n"));
  return;
}

/*
===============================================================================
                           CODED PRIVATE PROCEDURES
===============================================================================
*/

/******************************************************************************
 *
 *  Name: InitAdapterObject()
 *
 *  Description: Device object initialization function
 *
 *  Arguments:  
 *      PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: 
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_FAILURE
 * 
 *  Notes:
 *
 *****************************************************************************/
VOID InitAdapterObject(
    IN PMRVDRV_ADAPTER Adapter
  )
{
    USHORT i;

    Adapter->UseMfgFw = 0;

    //pmkcache: bug#16956 ++
    Adapter->isPktPending = FALSE;
    Adapter->bIsReconnectAssociation = FALSE;
    //pmkcache: bug#16956 --
    // Device information
    Adapter->VendorID = 0xffffffff; // Will be filled with real data
    Adapter->nAssocRspCount = 0; //tt ra fail

    ResetPmkidCache( Adapter );
    Adapter->RSNStatusIndicated.StatusType =  Ndis802_11StatusTypeMax;

    // NDIS Timer variables
    Adapter->TimerInterval = MRVDRV_DEFAULT_TIMER_INTERVAL;
    Adapter->isCommandTimerExpired = FALSE;
 
    // Operation characteristics
    Adapter->CurrentLookAhead = (ULONG)MRVDRV_MAXIMUM_ETH_PACKET_SIZE - MRVDRV_ETH_HEADER_SIZE;
    Adapter->ProtocolOptions = 0;
    Adapter->MACOptions  = (ULONG)NDIS_MAC_OPTION_NO_LOOPBACK;
    Adapter->MediaConnectStatus = NdisMediaStateDisconnected;
    Adapter->LinkSpeed = MRVDRV_LINK_SPEED_1mbps;
    Adapter->MediaInUse = NdisMedium802_3;
    NdisFillMemory(Adapter->CurrentAddr, MRVDRV_ETH_ADDR_LEN, 0xff);

    // Status variables
    Adapter->HardwareStatus = NdisHardwareStatusInitializing;

    // 802.11 specific
    Adapter->WEPStatus = Ndis802_11WEPDisabled;
    Adapter->AuthenticationMode = Ndis802_11AuthModeOpen;
    Adapter->InfrastructureMode = Ndis802_11Infrastructure;
    Adapter->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
    Adapter->ulNumOfBSSIDs =0;
    Adapter->ulCurrentBSSIDIndex =0;
    Adapter->ulAttemptedBSSIDIndex =0;
    Adapter->LastRSSI =  MRVDRV_RSSI_DEFAULT_NOISE_VALUE;
    Adapter->ulLastScanRequestTime =0;
  
    NdisZeroMemory(&(Adapter->CurrentSSID), sizeof(NDIS_802_11_SSID));
    NdisZeroMemory(&(Adapter->NullSSID), sizeof(NDIS_802_11_SSID));
    NdisZeroMemory(Adapter->NullBSSID, MRVDRV_ETH_ADDR_LEN);

    // Initialize RSSI-related variables
    Adapter->RSSITriggerValue = MRVDRV_RSSI_TRIGGER_DEFAULT;
    Adapter->RSSITriggerCounter = 0;

    // Tx related varables
    //  Adapter->QueuedPacket = NULL; //by dralee
    Adapter->SentPacket = NULL;

    // Power management state
    Adapter->CurPowerState = NdisDeviceStateD0;

    // PnP and power profile
    Adapter->SurpriseRemoved = FALSE;
    for( i=0; i<MRVDRV_TX_POWER_LEVEL_TOTAL ; i++){
        Adapter->PowerLevelList[i] = 
            HostCmd_ACT_TX_POWER_LEVEL_MIN + i * HostCmd_ACT_TX_POWER_LEVEL_GAP;
    }
    Adapter->SupportTxPowerLevel = MRVDRV_TX_POWER_LEVEL_TOTAL;
    Adapter->CurrentTxPowerLevel = HostCmd_ACT_TX_POWER_INDEX_MID;

    Adapter->TxPowerLevel = 0;
    Adapter->TxPowerLevelIsSetByOS = FALSE;

    // Set number of WEP keys
    Adapter->NoOfWEPEntry = 0;

    // Use system up time (in milli sec) for IV
    NdisGetSystemUpTime(&Adapter->IV);

    Adapter->SoftwareFilterOn = FALSE;
    Adapter->CurrentPacketFilter = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_BROADCAST;
    Adapter->CurrentMacControl = HostCmd_ACT_MAC_RX_ON | 
                                 HostCmd_ACT_MAC_TX_ON |
// v8,v9 doesn't this            HostCmd_ACT_MAC_INT_ENABLE|
                                 HostCmd_ACT_MAC_MULTICAST_ENABLE|
                                 HostCmd_ACT_MAC_WMM_ENABLE|
                                 HostCmd_ACT_MAC_BROADCAST_ENABLE ;
    Adapter->AcceptAnySSID=0x1;
    Adapter->AutoConfig=0x1;
    Adapter->SetActiveScanSSID=0x0;

    Adapter->RadioOn = TRUE;
    Adapter->TxAntenna = 2;
    Adapter->RxAntenna = 0x0000ffff;
    Adapter->DataRate = 0; 
    Adapter->FragThsd = 2346; 
    Adapter->RTSThsd = 2346;
    Adapter->Preamble = HostCmd_TYPE_LONG_PREAMBLE; 
    Adapter->TxPowerLevel = 100;    // 100 mW

    Adapter->Channel = 1;//default

    // Power Save 
    Adapter->PSMode = Ndis802_11PowerModeCAM;
    Adapter->psState = PS_STATE_FULL_POWER;
    Adapter->MultipleDTim = 1;  
    Adapter->ulAwakeTimeStamp      = 0;   

    Adapter->EncryptionStatus = Ndis802_11EncryptionDisabled;
    // setup association information buffer
    {
        PNDIS_802_11_ASSOCIATION_INFORMATION pAssoInfo;
        //ULONG   ulLength;
        
        pAssoInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION) 
                        Adapter->AssocInfoBuffer;
       // ulLength = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION);
        //ulLength = (ulLength) + 3
        DBGPRINT(DBG_LOAD, (L"pAssoInfo = 0x%x, pAssoInfo->Length = 0x%x\n", pAssoInfo, 
                            &(pAssoInfo->Length)));
        // assume the buffer has already been zero-ed
        // no variable IE, so both request and response IE are
        // pointed to the end of the buffer, 4 byte aligned
        pAssoInfo->OffsetRequestIEs = 
        pAssoInfo->OffsetResponseIEs = 
        pAssoInfo->Length = ((sizeof(NDIS_802_11_ASSOCIATION_INFORMATION) + 3) 
                                /4) * 4 ;
    }

   //012207
   //Adapter->IsDeepSleep = FALSE;
   //Adapter->IsDeepSleepRequired = FALSE;
   /*
        Cannot (also don't need to) use critical section here.
   */
   Adapter->DSState = DS_STATE_NONE;

	Adapter->AdhocAESEnabled = FALSE;


    Adapter->bPSConfirm=FALSE;
    Adapter->bIsPendingReset = FALSE;
    Adapter->bIsAssociationBlockedByScan = FALSE;
    Adapter->bIsScanWhileConnect = FALSE;
    Adapter->bIsConnectToAny = FALSE;
    Adapter->bIsScanInProgress = FALSE;
    Adapter->bIsAssociateInProgress = FALSE;


    Adapter->region_channel[0].Valid  = TRUE;
    Adapter->region_channel[0].Band   = MRVDRV_802_11_BAND_BG;  
    Adapter->region_channel[0].Region   = 0x40; //JPN
    Adapter->region_channel[0].CFP      = channel_freq_power_JPN_BG;
    Adapter->region_channel[0].NrCFP  = 14;




  Adapter->cur_region_channel = &(Adapter->region_channel[0]);
  Adapter->connected_band  = MRVDRV_802_11_BAND_BG;
  Adapter->connected_channel = 1;

  //lykao, 053005, set Adhoc Initial value

    Adapter->AdhocDefaultChannel      = 6;

  Adapter->AdhocWiFiDataRate          = 0; //set adhoc Band=MRVDRV_802_11_BAND_B

   
    Adapter->SetSD4BIT                  =0;  //Default setSD1BIT =4 bit mode
      Adapter->RegionTableIndex = 0;
      Adapter->RegionCode = 0x10;

    Adapter->SdioIstThread= 101; //150; dralee_20060604. to fix platform reset hang bug. 

        Adapter->bIsSystemConnectNow = FALSE;
        //35.p6++
       Adapter->bIsReconnectEnable = FALSE; 
       Adapter->bIsBeaconLoseEvent = FALSE;
       Adapter->bIsDeauthenticationEvent = FALSE;
       //35.p6--   
       Adapter->CurCmd = NULL;
    
    
    Adapter->bcn_avg_factor  = DEFAULT_BCN_AVG_FACTOR;
    Adapter->data_avg_factor = DEFAULT_DATA_AVG_FACTOR;  
    Adapter->ulRSSITickCount = 0;  
    Adapter->ulRSSIThresholdTimer=5000;
    Adapter->RSSI_Range=10;

    /*
     * ahan [2005-12-09]
     * 0 - No TLV,  [1,2] is valid range
     */ //++dralee_20060220
    Adapter->ScanProbes = 2;           

    /*
     * ahan [2005-12-12]
     * vlaid range is [0,20]
     */
    Adapter->LocalListenInterval = 0;  //0 - remain firmware listen interval unchanged

    /*
     * ahan [2005-12-13]
     * default event is LINK_LOSS
     * default beacon missed value is 60
     */  //++dralee_20060217
    Adapter->SubscribeEvents = LINK_LOSS; 
    Adapter->EventRecord.EventMap = LINK_LOSS;
    Adapter->EventRecord.BeaconMissed = LINK_LOST_BEACONCNT;
    /* ahan [2005-12-13] */

	Adapter->AvoidScanTime=10000;

    // tt mic error ++
    Adapter->bMicErrorIsHappened = FALSE;
    // tt mic error --

     Adapter->RxPDRate = 0; 
     Adapter->SNR[TYPE_RXPD][TYPE_NOAVG]=0 ;

        Adapter->bBgScanEnabled = 0;
        Adapter->bActiveRoamingwithBGSCAN=FALSE;
    Adapter->bPSEnableBGScan=FALSE;
    //NdisZeroMemory(Adapter->BgScanCfg, LENGTH_BGSCAN_CFG);
    NdisZeroMemory(Adapter->BgScanCfgInfo, sizeof(Adapter->BgScanCfgInfo));
    Adapter->nBgScanCfg=0;
    Adapter->ulScanInterval=20000; //20sec
    Adapter->bBgDeepSleep=0;
    Adapter->isPassiveScan = FALSE;
    Adapter->MeasureDurtion = 0;
    Adapter->ChannelNum = 0;


        Adapter->WmmDesc.enabled = 0;
        Adapter->WmmDesc.required = 1;
        Adapter->WmmDesc.qosinfo = 0x0f;

        Adapter->IbssCoalsecingEnable = 1;

    Adapter->NeedSetWepKey = FALSE;
    Adapter->ConnectedAPAuthMode = Ndis802_11AuthModeOpen;
    Adapter->RoamingMode = NOT_ROAMING_MODE;

      Adapter->TCloseWZCFlag=WZC_Default;
    
    //032007 fix adhoc indication
    Adapter->bIsMoreThanOneStaInAdHocBSS = FALSE;

    Adapter->bHasTspec = FALSE;
    Adapter->RoamAccount = 0;
    Adapter->RoamDelay = 0;

    Adapter->WaitCCKMIEEvent = CreateEvent(NULL, FALSE, FALSE,NULL);


    ///Reconnect_DiffSetting ++
    NdisZeroMemory(&Adapter->ReIEBuffer, sizeof(MRV_BSSID_IE_LIST));
    ///Reconnect_DiffSetting --

    return;
}

/******************************************************************************
 *
 *  Name: AllocateAdapterBuffer()
 *
 *  Description: Device object buffer initialization function
 *
 *  Arguments:  
 *      PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE
 * 
 *  Notes:       
 *
 *****************************************************************************/
NDIS_STATUS AllocateAdapterBuffer(
    IN PMRVDRV_ADAPTER Adapter
  )
{
    NDIS_STATUS Status;

	//  We are going to use non-cached shared memory for Rx buffers, no need to 
	//  check cache fill size offset
	ULONG Offset;

	Offset = 0;

	Adapter->InitializationStatus |= MRVDRV_INIT_STATUS_MAP_REGISTER_ALLOCATED;

  
	// Build up TxQ
	if(AllocateSingleTx(Adapter) != NDIS_STATUS_SUCCESS)
	{
		FreeAdapterObject(Adapter);
		return NDIS_STATUS_RESOURCES; 
	}

	// Build up RxQ
	Status = AllocateRxQ(Adapter);
	if( Status != NDIS_STATUS_SUCCESS ){
		FreeAdapterObject(Adapter);
		return NDIS_STATUS_RESOURCES;
	}

	// Build up command buffers
	Status = AllocateCmdBuffer(Adapter);
	if( Status != NDIS_STATUS_SUCCESS ){
		FreeAdapterObject(Adapter);
		return NDIS_STATUS_RESOURCES;
	}

	return NDIS_STATUS_SUCCESS;
}

/******************************************************************************
 *
 *  Name: ReadRegistryInfo()
 *
 *  Description: Device registry read function
 *
 *  Arguments: 
 *      PMRVDRV_ADAPTER Adapter
 *      NDIS_HANDLE RegHdl
 *
 *    
 *  Return Value: 
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_FAILURE
 * 
 *  Notes:        
 *
 *****************************************************************************/

NDIS_STATUS ReadRegistryInfo(
    IN PMRVDRV_ADAPTER Adapter,
    IN NDIS_HANDLE RegHdl
  )
{
    NDIS_STATUS Status;
    REGINFOTAB *RegInfoTab;
    UINT i;
    ULONG j;
    UINT Value;
    PUCHAR pPtr;
    PUCHAR pTmp;
    NDIS_CONFIGURATION_PARAMETER NdisConfigParam,*ReturnedValue;
    ANSI_STRING  DestinationANSIString;
    char UnicodeBuffer[512];//to save the UNICODE string. 
    char AnsiBuffer[128];//to save the ANSI string. 
    PUCHAR NetworkAddress;

    // Read registry network address, if there is a good one, put it itno Adapter->CurrentAddr array
    NdisReadNetworkAddress(&Status, &NetworkAddress, &i, RegHdl);
    if( Status == NDIS_STATUS_SUCCESS )
	{
        if( i == MRVDRV_ETH_ADDR_LEN )
		{
            NdisMoveMemory(Adapter->CurrentAddr, NetworkAddress, MRVDRV_ETH_ADDR_LEN);
			DBGPRINT(DBG_LOAD,(L"*** Got registry soft addr: %2x %2x %2x %2x %2x %2x ***\n", 
			Adapter->CurrentAddr[0],
			Adapter->CurrentAddr[1],
			Adapter->CurrentAddr[2],
			Adapter->CurrentAddr[3],
			Adapter->CurrentAddr[4],
			Adapter->CurrentAddr[5]));
        }
    }

	RegInfoTab = MrvDrvRegInfoTab;
	ReturnedValue = &NdisConfigParam;

	DestinationANSIString.Buffer=AnsiBuffer; 
	DestinationANSIString.MaximumLength=0x21;
	ReturnedValue->ParameterData.StringData.Buffer=(PWSTR)UnicodeBuffer;
    for(i=0; i<SIZE_OF_REG_TAB; i++, RegInfoTab++)
    {
        pPtr = ((PUCHAR)Adapter) + RegInfoTab->Offset;
        pTmp = pPtr;
        if (RegInfoTab->Type == 0) {
            NdisReadConfiguration(
                                &Status,
                                &ReturnedValue,
                                RegHdl,
                                &RegInfoTab->ObjNSName,
                                NdisParameterInteger);
            if (Status == NDIS_STATUS_SUCCESS)
            {             
                Value = ReturnedValue->ParameterData.IntegerData;
                *((PULONG)pPtr) = (ULONG)Value;
            }
        }
        else if(RegInfoTab->Type == 1)
        {
          
            NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                RegHdl,
                &RegInfoTab->ObjNSName,
                NdisParameterString);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                NdisUnicodeStringToAnsiString(
                        &DestinationANSIString, 
                        (PUNICODE_STRING)(&(ReturnedValue->ParameterData.StringData))
                        );   

                for(j=0;DestinationANSIString.Buffer[j] != '\0';j++)
                {
                    *(pTmp)=DestinationANSIString.Buffer[j];
                    pTmp++;
                } 

                *((PULONG)(pPtr-4))=j;
            }                                     
        }  
        else if (RegInfoTab->Type == 3) {
            NdisReadConfiguration(
                            &Status,
                            &ReturnedValue,
                            RegHdl,
                            &RegInfoTab->ObjNSName,
                            NdisParameterHexInteger);

            if (Status == NDIS_STATUS_SUCCESS) {
                Value = ReturnedValue->ParameterData.IntegerData;
                *((PINT)pPtr) = Value;
            }
        }
        else if (RegInfoTab->Type == 4) {
            NdisReadConfiguration(
                            &Status,
                            &ReturnedValue,
                            RegHdl,
                            &RegInfoTab->ObjNSName,
                            NdisParameterMultiString);
            if (Status == NDIS_STATUS_SUCCESS)
            {             
                Value = ReturnedValue->ParameterData.IntegerData;
                *((PULONG)pPtr) = (ULONG)Value;
            }
        }
    }

	Adapter->WepKey1.KeyIndex=0x0;
	Adapter->WepKey2.KeyIndex=0x1;
	Adapter->WepKey3.KeyIndex=0x2;
	Adapter->WepKey4.KeyIndex=0x3;      
	switch(Adapter->TxWepKey){

    case 0:
      Adapter->WepKey1.KeyIndex |= 0x80000000;
      NdisMoveMemory(&(Adapter->CurrentWEPKey),&(Adapter->WepKey1) , 12 + Adapter->WepKey1.KeyLength);
      break;
    case 1:
      Adapter->WepKey2.KeyIndex |= 0x80000000;
      NdisMoveMemory(&(Adapter->CurrentWEPKey),&(Adapter->WepKey2) , 12 + Adapter->WepKey2.KeyLength);
      break;  
    case 2:
      Adapter->WepKey3.KeyIndex |= 0x80000000;
      NdisMoveMemory(&(Adapter->CurrentWEPKey),&(Adapter->WepKey3) , 12 + Adapter->WepKey3.KeyLength);
      break;
    case 3:
      Adapter->WepKey4.KeyIndex |= 0x80000000;
      NdisMoveMemory(&(Adapter->CurrentWEPKey),&(Adapter->WepKey4) , 12 + Adapter->WepKey4.KeyLength);
      break;                
    default:
      break;  
  }        

 
    return NDIS_STATUS_SUCCESS;
}

/******************************************************************************
 *
 *  Name: InitSyncObjects()
 *
 *  Description: Syncronization object initialization function
 *
 *  Arguments:  
 *      PMRVDRV_ADAPTER Adapter
 *    
 *  Return Value: 
 *      NDIS_STATUS_SUCCESS
 *      NDIS_STATUS_FAILURE
 * 
 *  Notes:        
 *
 *****************************************************************************/
NDIS_STATUS
InitSyncObjects(
  IN PMRVDRV_ADAPTER Adapter
  )
{
	DWORD         threadID;  
	DWORD         waitStatus;

	InitializeCriticalSection(&Adapter->TxCriticalSection);
	InitializeCriticalSection(&Adapter->IntCriticalSection);
	Adapter->TxResourceControlEvent = CreateEvent(NULL, FALSE, FALSE,NULL);

       InitializeCriticalSection( &Adapter->CsForDSState );
       InitializeCriticalSection( &Adapter->CsForHostPowerState );

//tt ++ v5 firmware
    Adapter->hOidQueryEvent = CreateEvent(NULL, FALSE, FALSE,NULL);
//tt
    Adapter->hPendOnCmdEvent = CreateEvent(NULL, FALSE, FALSE,NULL);

	Adapter->hControllerInterruptEvent = CreateEvent(NULL, FALSE, FALSE,NULL);
	if (NULL == Adapter->hControllerInterruptEvent)
	{
		return NDIS_STATUS_FAILURE;
	} 
	Adapter->hThreadReadyEvent = CreateEvent(NULL, FALSE, FALSE,NULL);
	if (NULL == Adapter->hThreadReadyEvent)
	{
		return NDIS_STATUS_FAILURE;
	} 

    // create event to detect ejection
    Adapter->hEjectEvent = CreateEvent(NULL, TRUE, FALSE,NULL);
    if (NULL == Adapter->hEjectEvent)
    {
        return NDIS_STATUS_FAILURE;
    } 

	Adapter->hHwSpecEvent = CreateEvent(NULL, FALSE, FALSE,NULL);
	if (NULL == Adapter->hHwSpecEvent)
	{
		RETAILMSG(1,(TEXT("Can't Create HwSpec EVENT \n")));
		return NDIS_STATUS_FAILURE;
	}




//This Timer now is used to work around for interrupt missing issue. 
	NdisMInitializeTimer(&Adapter->MrvDrvSdioCheckFWReadyTimer,
							Adapter->MrvDrvAdapterHdl,
							(PNDIS_TIMER_FUNCTION)MrvDrvSdioCheckFWReadyTimerFunction,
							(PVOID)Adapter
							);
	Adapter->MrvDrvSdioCheckFWReadyTimerIsSet = FALSE;   
	NdisMInitializeTimer(&Adapter->MrvDrvIndicateConnectStatusTimer,
							Adapter->MrvDrvAdapterHdl,
							(PNDIS_TIMER_FUNCTION)MrvDrvIndicateConnectStatusTimer,
							(PVOID)Adapter
							);
	Adapter->DisconnectTimerSet = FALSE;
	// Initialize the timer for command
	NdisMInitializeTimer(&Adapter->MrvDrvCommandTimer,
							  Adapter->MrvDrvAdapterHdl,
							  (PNDIS_TIMER_FUNCTION) MrvDrvCommandTimerFunction,
							  (PVOID)Adapter
							  );
	Adapter->CommandTimerSet = FALSE;
  
	// Initialize the timer for Tx
	NdisMInitializeTimer(&Adapter->MrvDrvTxPktTimer,
								Adapter->MrvDrvAdapterHdl,
								(PNDIS_TIMER_FUNCTION) MrvDrvTxPktTimerFunction,
								(PVOID)Adapter
								);
	Adapter->TxPktTimerIsSet = FALSE;
	InitializeTxNodeQ(Adapter);
	// Initialize the timer for avoid scan when 802.1x authentication be not completed 
	NdisMInitializeTimer(&Adapter->MrvDrvAvoidScanAfterConnectedTimer,
							Adapter->MrvDrvAdapterHdl,
							(PNDIS_TIMER_FUNCTION)MrvDrvAvoidScanAfterConnectedTimer,
							(PVOID)Adapter
							);
	Adapter->bAvoidScanAfterConnectedforMSecs = FALSE;    
	// Initialize the timer for Reconnect 
    NdisMInitializeTimer(
            &Adapter->MrvReConnectTimer,
            Adapter->MrvDrvAdapterHdl,
            (PNDIS_TIMER_FUNCTION) MrvDrvReConnectTimerFunction,
            (PVOID)Adapter
            );
    Adapter->ReConnectTimerIsSet = FALSE;
    DBGPRINT(DBG_LOAD, (L" MrvReConnectTimer success \n"));
 
    NdisMInitializeTimer(
            &Adapter->MrvMicDisconnectTimer,
            Adapter->MrvDrvAdapterHdl,
            (PNDIS_TIMER_FUNCTION) MrvMicDisconnectTimerFunction,
            (PVOID)Adapter
            );

    Adapter->DisconnectTimerIsSet = FALSE;

    if ( (Adapter->DefaultPerChannelScanTime < 20) || (Adapter->DefaultPerChannelScanTime > 1000) )
    {
        DBGPRINT(DBG_WARNING, (L"Default channel time out of range: %d, set to default(%d)!\n",
               Adapter->DefaultPerChannelScanTime, HostCmd_SCAN_MAX_CH_TIME));

        Adapter->DefaultPerChannelScanTime = HostCmd_SCAN_MAX_CH_TIME;
    }

    Adapter->hControllerInterruptThread = CreateThread(NULL,
                                                      0,
                                                      (LPTHREAD_START_ROUTINE)MrvIstThread,
                                                      Adapter,
                                                      0,
                                                      &threadID);

    if (NULL == Adapter->hControllerInterruptThread)
    {
        return NDIS_STATUS_FAILURE;
    } 

    waitStatus = WaitForSingleObject(Adapter->hThreadReadyEvent, INFINITE);
    if (WAIT_OBJECT_0 != waitStatus)
    {
        return NDIS_STATUS_FAILURE;
    }

  return NDIS_STATUS_SUCCESS;
}

//tt ++ v5 firmware
/*
    Does the code need to delay for 10000 ms!!??
*/



VOID
DelaySeconds(
  ULONG interval
  )
{
  UINT i;

  for(i=0;i<(100 *interval);i++){           
    NdisStallExecution(10000);  //0.01 sec  
  }         
}

/******************************************************************************
 *
 *  Name: InitializeWirelessConfig()
 *
 *  Description: 
 *      Initialization routine for wireless configuration.
 *
 *
 *  Arguments: 
 *      IN NDIS_HANDLE MiniportAdapterHandle
 *    
 *  Return Value:         
 *
 *****************************************************************************/
BOOL
InitializeWirelessConfig(
  PMRVDRV_ADAPTER Adapter
  )
{
  PVOID InfoBuffer = NULL;
  NDIS_STATUS Status;
  DWORD   dwWaitStatus;




  NdisFillMemory(Adapter->CurrentAddr, MRVDRV_ETH_ADDR_LEN, 0xff);
  NdisFillMemory(Adapter->PermanentAddr, MRVDRV_ETH_ADDR_LEN, 0xff);

  PrepareAndSendCommand(
                        Adapter, 
                        HostCmd_CMD_GET_HW_SPEC, 
                        0, 
                        HostCmd_OPTION_USE_INT,
                        (NDIS_OID)0,
                        HostCmd_PENDING_ON_NONE,
                        0,
                        FALSE,
                        NULL,
                        NULL,
                        NULL,
                        NULL);                                 

        dwWaitStatus = WaitForSingleObjectWithCancel( Adapter, Adapter->hHwSpecEvent, ASYNC_OID_QUERY_TIMEOUT );

        if ( dwWaitStatus != WAIT_OBJECT_0 )

        {

            DBGPRINT( DBG_CUSTOM, (L"[MRVL] * ERROR, timeout (%d sec) for waiting HW Spec!\r\n", ASYNC_OID_QUERY_TIMEOUT) );
            return FALSE;

        }

        else

        {

            DBGPRINT( DBG_CUSTOM, (L"[MRVL] * Got HW Spec!\r\n") );

            DBGPRINT(DBG_CUSTOM,(L"[MRVL] * PermanentAddr %02x.%02x.%02x.%02x.%02x.%02x ***\r\n", 

                          Adapter->PermanentAddr[0],

                          Adapter->PermanentAddr[1],

                          Adapter->PermanentAddr[2],

                          Adapter->PermanentAddr[3],

                          Adapter->PermanentAddr[4],

                          Adapter->PermanentAddr[5]

                             ));

        }


  DBGPRINT(DBG_LOAD ,(L"Init: Setting fragmentation threshold to %d\n", Adapter->FragThsd ));
  Status =PrepareAndSendCommand(
      Adapter,
      HostCmd_CMD_802_11_SNMP_MIB,
      0,
      HostCmd_OPTION_USE_INT,
      (NDIS_OID)OID_802_11_FRAGMENTATION_THRESHOLD,
      HostCmd_PENDING_ON_NONE,
      0,
      FALSE,
      NULL,
      NULL,
      NULL,
      NULL);
  
    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
    {
       DBGPRINT(DBG_ERROR,(L"***1 InitializeWirelessConfig: HostCmd_CMD_802_11_SNMP_MIB Fail ****\n"));
       return FALSE;
    }  
    else
    {
       DBGPRINT(DBG_LOAD,(L"***1 InitializeWirelessConfig: HostCmd_CMD_802_11_SNMP_MIB Success ****\n"));
    }

  
    DBGPRINT(DBG_LOAD ,(L"Init: Setting RTS threshold to %d\n", Adapter->RTSThsd ));
  Status =PrepareAndSendCommand(
      Adapter,
      HostCmd_CMD_802_11_SNMP_MIB,
      0,
      HostCmd_OPTION_USE_INT,
      (NDIS_OID)OID_802_11_RTS_THRESHOLD,
      HostCmd_PENDING_ON_NONE,
      0,
      FALSE,
      NULL,
      NULL,
      NULL,
      NULL);

    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
    {
       DBGPRINT(DBG_ERROR,(L"***2 InitializeWirelessConfig: HostCmd_CMD_802_11_SNMP_MIB Fail ****\n"));
       return FALSE;
    }  
    else
    {
       DBGPRINT(DBG_LOAD,(L"***2 InitializeWirelessConfig: HostCmd_CMD_802_11_SNMP_MIB Success ****\n"));
    }

    Status = SetMacControl(Adapter);
  
    if (Status == NDIS_STATUS_FAILURE || Status == NDIS_STATUS_RESOURCES)
    {
       DBGPRINT(DBG_ERROR,(L"***3 InitializeWirelessConfig: HostCmd_CMD_MAC_CONTROL Fail ****\n"));
       return FALSE;
    }  
    else
    {
       DBGPRINT(DBG_LOAD,(L"***3 InitializeWirelessConfig: HostCmd_CMD_MAC_CONTROL Success ****\n"));
    }
  

    return TRUE;
} // InitializeWirelessConfig()


