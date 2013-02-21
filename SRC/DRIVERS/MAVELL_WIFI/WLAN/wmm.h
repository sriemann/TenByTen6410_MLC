#ifndef __WMM__H
#define __WMM__H

#ifdef _DEBUG
#define DbgWmmMsg(str)      
#else
#define DbgWmmMsg(str)
#endif

enum AC_QUEUES {
	AC_PRIO_BK, //tt
	AC_PRIO_BE, //tt
	AC_PRIO_VI,
	AC_PRIO_VO,
	MAX_AC_QUEUES
};

#define WMM_GET_PACKET_MR(_p)         (&(_p)->MiniportReserved[0]) 

#define CMD_ACT_GET     0
#define CMD_ACT_SET     1
#define	CMD_DISABLED	0
#define	CMD_ENABLED	1

#define WMM_AC_NUMBER                           4
#define WMM_TSPEC				2
#define WMM_ACK_POLICY				3
#define WMM_PARA_IE				4
#define WMM_ACK_POLICY_PRIO			4

#define WMM_IE_LENGTH				0x0009
#define WMM_PARA_IE_LENGTH			0x0018
#define WMM_QOS_INFO_OFFSET			(0x08)
#define WMM_QOS_INFO_UAPSD_BIT			(0x80)

#define HostCmd_CMD_802_11_WMM_GET_TSPEC	0x006E
#define HostCmd_CMD_802_11_WMM_ADD_TSPEC	0x006F
#define HostCmd_CMD_802_11_WMM_REMOVE_TSPEC	0x0070
#define HostCmd_CMD_802_11_WMM_ACK_POLICY	0x005C
#define HostCmd_CMD_802_11_WMM_PRIO_PKT_AVAIL	0x005D
#define HostCmd_CMD_802_11_WMM_GET_STATUS	0x0071
#define HostCmd_RET_802_11_WMM_GET_TSPEC	0x806E
#define HostCmd_RET_802_11_WMM_ADD_TSPEC	0x806F
#define HostCmd_RET_802_11_WMM_REMOVE_TSPEC	0x8070
#define HostCmd_RET_802_11_WMM_ACK_POLICY	0x805C
#define HostCmd_RET_803_11_WMM_PRIO_PKT_AVAIL	0x805D
#define HostCmd_RET_802_11_WMM_GET_STATUS	0x8071
#define HostCmd_ACT_MAC_WMM_ENABLE		0x0800
#define MACREG_INT_CODE_WMM_STATUS_CHANGE	0x00000017 // tt wled

#define IPTOS_OFFSET           5
#define MIN_IP_PKT_LEN         34   // 14 + 20

#define WMM_ACK_POLICY_NO_ACK        1
#define WMM_ACK_POLICY_IMM_ACK      0

#define WMM_UAPSD_DEFAULT_SLEEP_PERIOD      60

#pragma pack(1)

typedef struct _WMM_AC_STATUS {
	USHORT Type;
	USHORT Length;
        UCHAR   QueueIndex;
	UCHAR	Disabled;
	UCHAR	TriggeredPS;
	UCHAR	FlowDirection;
	UCHAR	FlowRequired;
	UCHAR	FlowCreated;
	ULONG	MediumTime;
}WMM_AC_STATUS, *PWMM_AC_STATUS;

#define WMM_AC_STATUS_SIZE     WMM_AC_NUMBER * sizeof(WMM_AC_STATUS)                      

/** data structure of WMM QoS information */
typedef struct
{
    UCHAR ParaSetCount:4;
    UCHAR Reserved:3;
    UCHAR QosUAPSD:1;
}WMM_QoS_INFO;

typedef struct
{
    UCHAR AIFSN:4;
    UCHAR ACM:1;
    UCHAR ACI:2;
    UCHAR Reserved:1;
}WMM_ACI_AIFSN;

/**  data structure of WMM ECW */
typedef struct
{
    UCHAR ECW_Min:4;
    UCHAR ECW_Max:4;
}WMM_ECW;

/** data structure of WMM AC parameters  */
typedef struct
{
    WMM_ACI_AIFSN ACI_AIFSN;
    WMM_ECW ECW;
    USHORT Txop_Limit;
}WMM_AC_PARAS;

/** data structure of WMM Info IE  */
typedef struct
{
    /** 221 */
    UCHAR ElementId;
    /** 7 */
    UCHAR Length;
    /** 00:50:f2 (hex) */
    UCHAR Oui[3];
    /** 2 */
    UCHAR OuiType;
    /** 0 */
    UCHAR OuiSubtype;
    /** 1 */
    UCHAR Version;

    WMM_QoS_INFO QoSInfo;

}WMM_INFO_IE;

/** data structure of WMM parameter IE  */
typedef struct
{
    /** 221 */
    UCHAR ElementId;
    /** 24 */
    UCHAR Length;
    /** 00:50:f2 (hex) */
    UCHAR Oui[3];
    /** 2 */
    UCHAR OuiType;
    /** 1 */
    UCHAR OuiSubtype;
    /** 1 */
    UCHAR Version;

    WMM_QoS_INFO QoSInfo;
    UCHAR Reserved;

    /** AC Parameters Record AC_BE */
    WMM_AC_PARAS AC_Paras_BE;
    /** AC Parameters Record AC_BK */
    WMM_AC_PARAS AC_Paras_BK;
    /** AC Parameters Record AC_VI */
    WMM_AC_PARAS AC_Paras_VI;
    /** AC Parameters Record AC_VO */
    WMM_AC_PARAS AC_Paras_VO;
}WMM_PARAMETER_IE;

typedef struct _WMM_DESC
{
	UCHAR		required;
	UCHAR		enabled;
	UCHAR		fw_notify;
	UCHAR		acstatus;	/* x x x x O I K E --- 1=disabled 0=enabled */
	//Q_KEEPER	TxPktQ[MAX_AC_QUEUES];   move to Adapter , dralee
	UCHAR		Para_IE[WMM_PARA_IE_LENGTH];
	UCHAR		qosinfo;
        UCHAR           WmmUapsdEnabled;
}WMM_DESC,*PWMM_DESC;

typedef struct _WMM_ADDTS_RESP
{
    UCHAR       TspecResult;
    ULONG       TimeOut;
    UCHAR       DialogToken;
    UCHAR       IEEEStatus;
    UCHAR       TspecData[63];
    UCHAR       ExtraIeData[256];
} WMM_ADDTS_RESP, *PWMM_ADDTS_RESP;
 
#pragma pack()

typedef struct _MRVDRV_ADAPTER *PMRVDRV_ADAPTER;

void wmm_cleanup_queues(PMRVDRV_ADAPTER);
int wlan_wmm_enable_ioctl(PMRVDRV_ADAPTER, PVOID);
VOID wmm_update_status_from_cmd_resp( PMRVDRV_ADAPTER Adapter );
VOID wmm_set_ack_policy( PMRVDRV_ADAPTER Adapter, UCHAR nAC, UCHAR nPolicy );
UCHAR wmm_get_tos(PNDIS_PACKET pPacket);
NDIS_STATUS wmm_set_sleep_period( PMRVDRV_ADAPTER Adapter, USHORT nPeriod );

NDIS_STATUS SendNullPacket( IN PMRVDRV_ADAPTER Adapter, UCHAR pm );

UCHAR wmm_get_pkt_priority( PNDIS_PACKET Packet );

#endif
