#ifndef _CUSTOM_INTERFACE_H_
#define _CUSTOM_INTERFACE_H_

#include <ntddndis.h>
#ifdef __cplusplus
extern "C" {	
#endif

typedef struct _MRVDRV_ADAPTER *PMRVDRV_ADAPTER;

#define AES_KEY_STORE   0x0001
#define AES_KEY_SET     0x0002
#define AES_KEY_GET     0x0004

BOOLEAN CustomDsAwakeEventProcess(PMRVDRV_ADAPTER Adapter);
VOID CustomMacEventProcess(PMRVDRV_ADAPTER Adapter, UINT IntCode);

BOOLEAN CustomLinkSensePorcess(PMRVDRV_ADAPTER Adapter);

VOID CustomInitialize(PMRVDRV_ADAPTER Adapter);

VOID AddCustomDefineIE(PMRVDRV_ADAPTER Adapter,
                       PUCHAR pCustomIE, 
                       PUSHORT pSize);
BOOLEAN CustomIeNeedKeep(PUCHAR Ie);
BOOLEAN CustomIeVerification(PMRVDRV_ADAPTER Adapter,
                             PUCHAR Ie);
VOID CustomIeSaveDestinateMac(PVOID CustomInfoPt, PVOID Ie);

NDIS_STATUS
CustomOidSetInformation(PMRVDRV_ADAPTER Adapter, NDIS_OID Oid, 
                              PVOID InformationBuffer, ULONG InformationBufferLength,
                              OUT PULONG BytesRead, OUT PULONG BytesNeeded);
NDIS_STATUS
CustomOidGetInformation(PMRVDRV_ADAPTER Adapter, NDIS_OID Oid, 
                              PVOID InformationBuffer, ULONG InformationBufferLength,
                              OUT PULONG BytesWritten,  OUT PULONG BytesNeeded);

BOOLEAN PrepareCustomCommand(USHORT Cmd,
                          USHORT CmdOption,
                          PHostCmd_DS_GEN pCmdPtr,
                          PUSHORT Size,
                          PVOID InformationBuffer);


VOID HandleCustomPendOid(USHORT Ret,
                         PMRVDRV_ADAPTER Adapter,
                         PHostCmd_DS_GEN pRetPtr,
                         USHORT PendingInfo );

NDIS_STATUS 
CustomAdhocStartJoinResponseProcess(
	 PMRVDRV_ADAPTER Adapter,
	 USHORT Cmd,
         NDIS_STATUS Result);   

NDIS_STATUS CustomSetMode(PMRVDRV_ADAPTER Adapter, USHORT PendInfo);

NDIS_STATUS CustomRequestFromOIDInfrastructureMode(PMRVDRV_ADAPTER Adapter);
BOOLEAN IsNormalWiFiMode(PMRVDRV_ADAPTER Adapter);



BOOLEAN CustomScanNeedPending(PMRVDRV_ADAPTER Adapter);


NDIS_STATUS
HandleMsPowerState(
            PMRVDRV_ADAPTER Adapter, PUCHAR PowerState,
            BOOLEAN bIsSet);

BOOLEAN CustomOIDAllowInDeepSleep(NDIS_OID Oid);

BOOLEAN CustomOIDAllowInPassiveMode( PMRVDRV_ADAPTER Adapter, NDIS_OID Oid);

VOID CustomSetScanTime(PMRVDRV_ADAPTER Adapter,  
                      ChanScanParamSet_t  *chaninfo); 
          
BOOLEAN CustomIgnoreThisOid(PMRVDRV_ADAPTER Adapter,
                            NDIS_OID Oid);
          
NDIS_STATUS
CustomAesKeyProcess(PMRVDRV_ADAPTER Adapter,
                    USHORT Command,
                    PVOID InformationBuffer,
                    ULONG InformationBufferLength);

ULONG CustomScanIterations(PMRVDRV_ADAPTER Adapter);

BOOLEAN CustomOIDQueryAllowInPS(NDIS_OID Oid);

BOOLEAN CustomOIDAllowInPowerState(PMRVDRV_ADAPTER Adapter, 
                                  NDIS_OID Oid);

BOOLEAN CustomStaInD0PowerState(PMRVDRV_ADAPTER Adapter);

NDIS_STATUS
MrvDrvCustomSetInformation(
  IN NDIS_HANDLE MiniportAdapterContext,
  IN NDIS_OID Oid,
  IN PVOID InformationBuffer,
  IN ULONG InformationBufferLength,
  OUT PULONG BytesWritten,
  OUT PULONG BytesNeeded
  );

NDIS_STATUS
MrvDrvCustomQueryInformation(
  IN NDIS_HANDLE MiniportAdapterContext,
  IN NDIS_OID Oid,
  IN PVOID InformationBuffer,
  IN ULONG InformationBufferLength,
  OUT PULONG BytesWritten,
  OUT PULONG BytesNeeded
  );

NDIS_STATUS
MrvDrvCustomOIDProcess(
  IN BOOLEAN bIsSet,
  IN NDIS_HANDLE MiniportAdapterContext,
  IN NDIS_OID Oid,
  IN PVOID InformationBuffer,
  IN ULONG InformationBufferLength,
  OUT PULONG BytesWritten,
  OUT PULONG BytesNeeded
  );

VOID CustomDeInitialize(PMRVDRV_ADAPTER Adapter);

VOID CustomSignalModeSwitchEvent(PMRVDRV_ADAPTER Adapter);

void CustomProcessPnpSetPower( PMRVDRV_ADAPTER Adapter, NDIS_DEVICE_POWER_STATE NewPowerState );

#ifdef __cplusplus
}
#endif

#endif	// _CUSTOM_INTERFACE_H_
