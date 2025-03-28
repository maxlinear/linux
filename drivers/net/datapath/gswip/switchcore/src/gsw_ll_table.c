/******************************************************************************
                Copyright (c) 2016, 2017 Intel Corporation

******************************************************************************/
/*****************************************************************************
                Copyright (c) 2012, 2014, 2015
                    Lantiq Deutschland GmbH
    For licensing information, see the file 'LICENSE' in the root folder of
    this software module.
******************************************************************************/


#include <gsw_init.h>

#ifdef GSW_SW_FKT
#undef GSW_SW_FKT
#endif /* GSW_SW_FKT */

#define GSW_SW_FKT(x, y) x ? (LTQ_ll_fkt)y : NULL

#ifdef CONFIG_MXL_8021X
#undef CONFIG_MXL_8021X
#define CONFIG_MXL_8021X		1
#else
#define CONFIG_MXL_8021X		0
#endif
#ifdef CONFIG_MXL_MULTICAST
#undef CONFIG_MXL_MULTICAST
#define CONFIG_MXL_MULTICAST	1
#else
#define CONFIG_MXL_MULTICAST	0
#endif
#ifdef CONFIG_MXL_QOS
#undef CONFIG_MXL_QOS
#define CONFIG_MXL_QOS			1
#else
#define CONFIG_MXL_QOS			0
#endif
#ifdef CONFIG_MXL_STP
#undef CONFIG_MXL_STP
#define CONFIG_MXL_STP			1
#else
#define CONFIG_MXL_STP			0
#endif
#ifdef CONFIG_MXL_VLAN
#undef CONFIG_MXL_VLAN
#define CONFIG_MXL_VLAN			1
#else
#define CONFIG_MXL_VLAN			0
#endif
#ifdef CONFIG_MXL_WOL
#undef CONFIG_MXL_WOL
#define CONFIG_MXL_WOL			1
#else
#define CONFIG_MXL_WOL			0
#endif

#ifdef CONFIG_MXL_PMAC
#undef CONFIG_MXL_PMAC
#define CONFIG_MXL_PMAC			1
#else
#define CONFIG_MXL_PMAC			0
#endif
//#define CONFIG_MXL_PMAC			1
#ifdef CONFIG_MXL_RMON
#undef CONFIG_MXL_RMON
#define CONFIG_MXL_RMON			1
#else
#define CONFIG_MXL_RMON			0
#endif
//#define CONFIG_MXL_RMON			1
LTQ_ll_fkt ltq_fkt_ptr_tbl[] = {
	/* 0x00 */
	(LTQ_ll_fkt) NULL,
	/* Command: GSW_MAC_TABLE_ENTRY_READ ; Index: 0x01 */
	(LTQ_ll_fkt) GSW_MAC_TableEntryRead,
	/* Command: GSW_MAC_TABLE_ENTRY_QUERY ; Index: 0x02 */
	(LTQ_ll_fkt) GSW_MAC_TableEntryQuery,
	/* Command: GSW_MAC_TABLE_ENTRY_ADD ; Index: 0x03 */
	(LTQ_ll_fkt) GSW_MAC_TableEntryAdd,
	/* Command: GSW_MAC_TABLE_ENTRY_REMOVE ; Index: 0x04 */
	(LTQ_ll_fkt) GSW_MAC_TableEntryRemove,
	/* Command: GSW_MAC_TABLE_CLEAR ; Index: 0x05 */
	(LTQ_ll_fkt) GSW_MAC_TableClear,
	/* Command: GSW_STP_PORT_CFG_SET ; Index: 0x06 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_STP, GSW_STP_PortCfgSet),
	/* Command: GSW_STP_PORT_CFG_GET ; Index: 0x07 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_STP, GSW_STP_PortCfgGet),
	/* Command: GSW_STP_BPDU_RULE_SET ; Index: 0x08 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_STP, GSW_STP_BPDU_RuleSet),
	/* Command: GSW_STP_BPDU_RULE_GET ; Index: 0x09 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_STP, GSW_STP_BPDU_RuleGet),
	/* Command: GSW_8021X_EAPOL_RULE_GET ; Index: 0x0A */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_8021X, GSW_8021X_EAPOL_RuleGet),
	/* Command: GSW_8021X_EAPOL_RULE_SET ; Index: 0x0B */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_8021X, GSW_8021X_EAPOL_RuleSet),
	/* Command: GSW_8021X_PORT_CFG_GET ; Index: 0x0C */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_8021X, GSW_8021X_PortCfgGet),
	/* Command: GSW_8021X_PORT_CFG_SET ; Index: 0x0D */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_8021X, GSW_8021X_PortCfgSet),
	/* Command: GSW_VLAN_RESERVED_ADD ; Index: 0x0E */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_ReservedAdd),
	/* Command: GSW_VLAN_RESERVED_REMOVE ; Index: 0x0F */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_ReservedRemove),
	/* Command: GSW_VLAN_PORT_CFG_GET ; Index: 0x10 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_PortCfgGet),
	/* Command: GSW_VLAN_PORT_CFG_SET ; Index: 0x11 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_PortCfgSet),
	/* Command: GSW_VLAN_ID_CREATE ; Index: 0x12 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_IdCreate),
	/* Command: GSW_VLAN_ID_DELETE ; Index: 0x13 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_IdDelete),
	/* Command: GSW_VLAN_PORT_MEMBER_ADD ; Index: 0x14 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_PortMemberAdd),
	/* Command: GSW_VLAN_PORT_MEMBER_REMOVE ; Index: 0x15 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_PortMemberRemove),
	/* Command: GSW_VLAN_PORT_MEMBER_READ ; Index: 0x16 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_PortMemberRead),
	/* Command: GSW_VLAN_ID_GET ; Index: 0x17 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_IdGet),
	/* Command: GSW_QOS_PORT_CFG_SET ; Index: 0x18 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_PortCfgSet),
	/* Command: GSW_QOS_PORT_CFG_GET ; Index: 0x19 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_PortCfgGet),
	/* Command: GSW_QOS_DSCP_CLASS_SET ; Index: 0x1A */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_DSCP_ClassSet),
	/* Command: GSW_QOS_DSCP_CLASS_GET ; Index: 0x1B */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_DSCP_ClassGet),
	/* Command: GSW_QOS_PCP_CLASS_SET ; Index: 0x1C */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_PCP_ClassSet),
	/* Command: GSW_QOS_PCP_CLASS_GET ; Index: 0x1D */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_PCP_ClassGet),
	/* Command: GSW_QOS_DSCP_DROP_PRECEDENCE_CFG_SET ; Index: 0x1E */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_DSCP_DropPrecedenceCfgSet),
	/* Command: GSW_QOS_DSCP_DROP_PRECEDENCE_CFG_GET ; Index: 0x1F */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_DSCP_DropPrecedenceCfgGet),
	/* Command: GSW_QOS_PORT_REMARKING_CFG_SET ; Index: 0x20 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_PortRemarkingCfgSet),
	/* Command: GSW_QOS_PORT_REMARKING_CFG_GET ; Index: 0x21 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_PortRemarkingCfgGet),
	/* Command: GSW_QOS_CLASS_DSCP_SET ; Index: 0x22 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ClassDSCP_Set),
	/* Command: GSW_QOS_CLASS_DSCP_GET ; Index: 0x23 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ClassDSCP_Get),
	/* Command: GSW_QOS_CLASS_PCP_SET ; Index: 0x24 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ClassPCP_Set),
	/* Command: GSW_QOS_CLASS_PCP_GET ; Index: 0x25 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ClassPCP_Get),
	/* Command: GSW_QOS_SHAPER_CFG_SET ; Index: 0x26 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ShaperCfgSet),
	/* Command: GSW_QOS_SHAPER_CFG_GET ; Index: 0x27 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ShaperCfgGet),
	/* Command: GSW_QOS_SHAPER_QUEUE_ASSIGN ; Index: 0x28 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ShaperQueueAssign),
	/* Command: GSW_QOS_SHAPER_QUEUE_DEASSIGN ; Index: 0x29 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ShaperQueueDeassign),
	/* Command: GSW_QOS_SHAPER_QUEUE_GET ; Index: 0x2A */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_ShaperQueueGet),
	/* Command: GSW_QOS_WRED_CFG_SET ; Index: 0x2B */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_WredCfgSet),
	/* Command: GSW_QOS_WRED_CFG_GET ; Index: 0x2C */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_WredCfgGet),
	/* Command: GSW_QOS_WRED_QUEUE_CFG_SET ; Index: 0x2D */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_WredQueueCfgSet),
	/* Command: GSW_QOS_WRED_QUEUE_CFG_GET ; Index: 0x2E */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_WredQueueCfgGet),
	/* Command: GSW_QOS_METER_CFG_SET ; Index: 0x2F */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_MeterCfgSet),
	/* Command: GSW_QOS_METER_CFG_GET ; Index: 0x30 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_MeterCfgGet),
	/* Command: GSW_QOS_METER_PORT_ASSIGN ; Index: 0x31 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_MeterPortAssign),
	/* Command: GSW_QOS_METER_PORT_DEASSIGN ; Index: 0x32 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_MeterPortDeassign),
	/* Command: GSW_QOS_METER_PORT_GET ; Index: 0x33 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_MeterPortGet),
	/* Command: GSW_QOS_STORM_CFG_SET ; Index: 0x34 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_StormCfgSet),
	/* Command: GSW_QOS_STORM_CFG_GET ; Index: 0x35 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_StormCfgGet),
	/* Command: GSW_QOS_SCHEDULER_CFG_SET ; Index: 0x36 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_SchedulerCfgSet),
	/* Command: GSW_QOS_SCHEDULER_CFG_GET ; Index: 0x37 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_SchedulerCfgGet),
	/* Command: GSW_QOS_QUEUE_PORT_SET ; Index: 0x38 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_QueuePortSet),
	/* Command: GSW_QOS_QUEUE_PORT_GET ; Index: 0x39 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_QueuePortGet),
	/* Command: GSW_MULTICAST_SNOOP_CFG_SET ; Index: 0x3A */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastSnoopCfgSet),
	/* Command: GSW_MULTICAST_SNOOP_CFG_GET ; Index: 0x3B */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastSnoopCfgGet),
	/* Command: GSW_MULTICAST_ROUTER_PORT_ADD ; Index: 0x3C */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastRouterPortAdd),
	/* Command: GSW_MULTICAST_ROUTER_PORT_REMOVE ; Index: 0x3D */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastRouterPortRemove),
	/* Command: GSW_MULTICAST_ROUTER_PORT_READ ; Index: 0x3E */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastRouterPortRead),
	/* Command: GSW_MULTICAST_TABLE_ENTRY_ADD ; Index: 0x3F */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastTableEntryAdd),
	/* Command: GSW_MULTICAST_TABLE_ENTRY_REMOVE ; Index: 0x40 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastTableEntryRemove),
	/* Command: GSW_MULTICAST_TABLE_ENTRY_READ ; Index: 0x41 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_MULTICAST, GSW_MulticastTableEntryRead),
	/* Command: GSW_HW_INIT ; Index: 0x42 */
	(LTQ_ll_fkt) GSW_HW_Init,
	/* Command: GSW_VERSION_GET ; Index: 0x43 */
	(LTQ_ll_fkt) GSW_VersionGet,
	/* Command: GSW_CAP_GET ; Index: 0x44 */
	(LTQ_ll_fkt) GSW_CapGet,
	/* Command: GSW_CFG_SET ; Index: 0x45 */
	(LTQ_ll_fkt) GSW_CfgSet,
	/* Command: GSW_CFG_GET ; Index: 0x46 */
	(LTQ_ll_fkt) GSW_CfgGet,
	/* Command: GSW_ENABLE ; Index: 0x47 */
	(LTQ_ll_fkt) GSW_Enable,
	/* Command: GSW_DISABLE ; Index: 0x48 */
	(LTQ_ll_fkt) GSW_Disable,
	/* Command: GSW_PORT_CFG_GET ; Index: 0x49 */
	(LTQ_ll_fkt) GSW_PortCfgGet,
	/* Command: GSW_PORT_CFG_SET ; Index: 0x4A */
	(LTQ_ll_fkt) GSW_PortCfgSet,
	/* Command: GSW_CPU_PORT_CFG_SET ; Index: 0x4B */
	(LTQ_ll_fkt) GSW_CPU_PortCfgSet,
	/* Command: GSW_CPU_PORT_CFG_GET ; Index: 0x4C */
	(LTQ_ll_fkt) GSW_CPU_PortCfgGet,
	/* Command: GSW_CPU_PORT_EXTEND_CFG_SET ; Index: 0x4D */
	(LTQ_ll_fkt) GSW_CPU_PortExtendCfgSet,
	/* Command: GSW_CPU_PORT_EXTEND_CFG_GET ; Index: 0x4E */
	(LTQ_ll_fkt) GSW_CPU_PortExtendCfgGet,
	/* Command: GSW_PORT_LINK_CFG_GET ; Index: 0x4F */
	(LTQ_ll_fkt) GSW_PortLinkCfgGet,
	/* Command: GSW_PORT_LINK_CFG_SET ; Index: 0x50 */
	(LTQ_ll_fkt) GSW_PortLinkCfgSet,
	/* Command: GSW_PORT_RGMII_CLK_CFG_SET ; Index: 0x51 */
	(LTQ_ll_fkt) GSW_PortRGMII_ClkCfgSet,
	/* Command: GSW_PORT_RGMII_CLK_CFG_GET ; Index: 0x52 */
	(LTQ_ll_fkt) GSW_PortRGMII_ClkCfgGet,
	/* Command: GSW_PORT_PHY_QUERY ; Index: 0x53 */
	(LTQ_ll_fkt) GSW_PortPHY_Query,
	/* Command: GSW_PORT_PHY_ADDR_GET ; Index: 0x54 */
	(LTQ_ll_fkt) GSW_PortPHY_AddrGet,
	/* Command: GSW_PORT_REDIRECT_GET ; Index: 0x55 */
	(LTQ_ll_fkt) GSW_PortRedirectGet,
	/* Command: GSW_PORT_REDIRECT_SET ; Index: 0x56 */
	(LTQ_ll_fkt) GSW_PortRedirectSet,
	/* Command: GSW_MONITOR_PORT_CFG_GET ; Index: 0x57 */
	(LTQ_ll_fkt) GSW_MonitorPortCfgGet,
	/* Command: GSW_MONITOR_PORT_CFG_SET ; Index: 0x58 */
	(LTQ_ll_fkt) GSW_MonitorPortCfgSet,
	/* Command: GSW_RMON_PORT_GET ; Index: 0x59 */
	(LTQ_ll_fkt) GSW_RMON_Port_Get,
	/* Command: GSW_RMON_CLEAR ; Index: 0x5A */
	(LTQ_ll_fkt) GSW_RMON_Clear,
	/* Command: GSW_MDIO_CFG_GET ; Index: 0x5B */
	(LTQ_ll_fkt) GSW_MDIO_CfgGet,
	/* Command: GSW_MDIO_CFG_SET ; Index: 0x5C */
	(LTQ_ll_fkt) GSW_MDIO_CfgSet,
	/* Command: GSW_MDIO_DATA_READ ; Index: 0x5D */
	(LTQ_ll_fkt) GSW_MDIO_DataRead,
	/* Command: GSW_MDIO_DATA_WRITE ; Index: 0x5E */
	(LTQ_ll_fkt) GSW_MDIO_DataWrite,
	/* Command: GSW_MMD_DATA_READ ; Index: 0x5F */
	(LTQ_ll_fkt) GSW_MmdDataRead,
	/* Command: GSW_MMD_DATA_WRITE ; Index: 0x60 */
	(LTQ_ll_fkt) GSW_MmdDataWrite,
	/* Command: GSW_WOL_CFG_SET ; Index: 0x61 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_WOL, GSW_WoL_CfgSet),
	/* Command: GSW_WOL_CFG_GET ; Index: 0x62 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_WOL, GSW_WoL_CfgGet),
	/* Command: GSW_WOL_PORT_CFG_SET ; Index: 0x63 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_WOL, GSW_WoL_PortCfgSet),
	/* Command: GSW_WOL_PORT_CFG_GET ; Index: 0x64 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_WOL, GSW_WoL_PortCfgGet),
	/* Command: GSW_TRUNKING_CFG_GET ; Index: 0x65 */
	(LTQ_ll_fkt) GSW_TrunkingCfgGet,
	/* Command: GSW_TRUNKING_CFG_SET ; Index: 0x66 */
	(LTQ_ll_fkt) GSW_TrunkingCfgSet,
	/* Command: GSW_TRUNKING_PORT_CFG_GET ; Index: 0x67 */
	(LTQ_ll_fkt) GSW_TrunkingPortCfgGet,
	/* Command: GSW_TRUNKING_PORT_CFG_SET ; Index: 0x68 */
	(LTQ_ll_fkt) GSW_TrunkingPortCfgSet,
	/* Command: GSW_QOS_WRED_PORT_CFG_SET ; Index: 0x69 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_WredPortCfgSet),
	/* Command: GSW_QOS_WRED_PORT_CFG_GET ; Index: 0x6a */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_WredPortCfgGet),
	/* Command: GSW_QOS_FLOWCTRL_CFG_SET ; Index: 0x6b */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_FlowctrlCfgSet),
	/* Command: GSW_QOS_FLOWCTRL_CFG_GET ; Index: 0x6c */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_FlowctrlCfgGet),
	/* Command: GSW_QOS_FLOWCTRL_PORT_CFG_SET ; Index: 0x6d */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_FlowctrlPortCfgSet),
	/* Command: GSW_QOS_FLOWCTRL_PORT_CFG_GET ; Index: 0x6e */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_FlowctrlPortCfgGet),
	/* Command: GSW_QOS_QUEUE_BUFFER_RESERVE_CFG_SET ; Index: 0x6f */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_QueueBufferReserveCfgSet),
	/* Command: GSW_QOS_QUEUE_BUFFER_RESERVE_CFG_GET ; Index: 0x70 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_QueueBufferReserveCfgGet),
	/* Command: GSW_SVLAN_CFG_GET ; Index: 0x71 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_SVLAN_CfgGet),
	/* Command: GSW_SVLAN_CFG_SET ; Index: 0x72 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_SVLAN_CfgSet),
	/* Command: GSW_SVLAN_PORT_CFG_GET ; Index: 0x73 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_SVLAN_PortCfgGet),
	/* Command: GSW_SVLAN_PORT_CFG_SET ; Index: 0x74 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_SVLAN_PortCfgSet),
	/* Command: GSW_QOS_SVLAN_CLASS_PCP_PORT_SET ; Index: 0x75 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_QoS_SVLAN_ClassPCP_PortSet),
	/* Command: GSW_QOS_SVLAN_CLASS_PCP_PORT_GET ; Index: 0x76 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_QoS_SVLAN_ClassPCP_PortGet),
	/* Command: GSW_QOS_SVLAN_PCP_CLASS_SET ; Index: 0x77 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_QoS_SVLAN_PCP_ClassSet),
	/* Command: GSW_QOS_SVLAN_PCP_CLASS_GET ; Index: 0x78 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_QoS_SVLAN_PCP_ClassGet),
	/* Command: GSW_VLAN_MEMBER_INIT ; Index: 0x79 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_VLAN_Member_Init),
	/* Command: GSW_PCE_EG_VLAN_CFG_SET ; Index: 0x7A */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_PCE_EG_VLAN_CfgSet),
	/* Command: GSW_PCE_EG_VLAN_CFG_GET ; Index: 0x7B */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_PCE_EG_VLAN_CfgGet),
	/* Command: GSW_PCE_EG_VLAN_ENTRY_WRITE ; Index: 0x7C */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_PCE_EG_VLAN_EntryWrite),
	/* Command: GSW_PCE_EG_VLAN_ENTRY_READ ; Index: 0x7D */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, GSW_PCE_EG_VLAN_EntryRead),
	/* Command: GSW_QOS_METER_ACT ; Index: 0x7E */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_QOS, GSW_QoS_Meter_Act),
	(LTQ_ll_fkt) NULL,  /* Index: 0x7F */
	(LTQ_ll_fkt) NULL, /* Index: 0x80 */
	(LTQ_ll_fkt) NULL, /* Index: 0x81 */
	(LTQ_ll_fkt) NULL, /* Index: 0x82 */
	(LTQ_ll_fkt) NULL, /* Index: 0x83 */
	(LTQ_ll_fkt) NULL, /* Index: 0x84 */
	(LTQ_ll_fkt) NULL, /* Index: 0x85 */
	(LTQ_ll_fkt) NULL, /* Index: 0x86 */
	(LTQ_ll_fkt) NULL, /* Index: 0x87 */
	(LTQ_ll_fkt) NULL, /* Index: 0x88 */
	(LTQ_ll_fkt) NULL, /* Index: 0x89 */
	(LTQ_ll_fkt) NULL, /* Index: 0x8a */
	(LTQ_ll_fkt) NULL, /* Index: 0x8b */
	(LTQ_ll_fkt) NULL, /* Index: 0x8c */
	(LTQ_ll_fkt) NULL, /* Index: 0x8d */
	(LTQ_ll_fkt) NULL, /* Index: 0x8e */
	(LTQ_ll_fkt) NULL, /* Index: 0x8f */
	/* Command: GSW_RMON_MODE_SET ; Index: 0x90 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_RMON, GSW_RMON_Mode_Set),
	/* Command: GSW_RMON_IF_GET ; Index: 0x91 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_RMON, GSW_RMON_If_Get),
	/* Command: GSW_RMON_REDIRECT_GET ; Index: 0x92 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_RMON, GSW_RMON_Redirect_Get),
	/* Command: GSW_RMON_ROUTE_GET ; Index: 0x93 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_RMON, GSW_RMON_Route_Get),
	/* Command: GSW_RMON_METER_GET ; Index: 0x94 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_RMON, GSW_RMON_Meter_Get),
	/* Command: GSW_PMAC_BM_CFG_SET ; Index: 0x95 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_BM_CfgSet),
	/* Command: GSW_PMAC_BM_CFG_GET ; Index: 0x96 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_BM_CfgGet),
	/* Command: GSW_PMAC_IG_CFG_SET ; Index: 0x97 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_IG_CfgSet),
	/* Command: GSW_PMAC_IG_CFG_GET ; Index: 0x98 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_IG_CfgGet),
	/* Command: GSW_PMAC_EG_CFG_SET ; Index: 0x99 */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_EG_CfgSet),
	/* Command: GSW_PMAC_EG_CFG_GET ; Index: 0x9a */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_EG_CfgGet),
	/* Command: GSW_PMAC_COUNT_GET ; Index: 0x9b */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_CountGet),
	/* Command: GSW_PMAC_GLBL_CFG_SET ; Index: 0x9c */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_GLBL_CfgSet),
	/* Command: GSW_PMAC_GLBL_CFG_GET ; Index: 0x9d */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_PMAC, GSW_PMAC_GLBL_CfgGet),
	(LTQ_ll_fkt) NULL,  /* Index: 0x9E */
	(LTQ_ll_fkt) NULL, /* Index: 0x9F */
	/*Applicable only for GSWIP 3.1*/
	/* Command: GSW_CTP_PORT_ASSIGNMENT_SET ; Index: 0xA0 */
	(LTQ_ll_fkt) GSW_CTP_PortAssignmentSet,
	/* Command: GSW_CTP_PORT_ASSIGNMENT_GET ; Index: 0xA1 */
	(LTQ_ll_fkt) GSW_CTP_PortAssignmentGet,
	/* Command: GSW_BRIDGE_CONFIG_SET ; Index: 0xA2 */
	(LTQ_ll_fkt) GSW_BridgeConfigSet,
	/* Command: GSW_BRIDGE_FREE ; Index: 0xA3 */
	(LTQ_ll_fkt) GSW_BridgeFree,
	/* Command: GSW_BRIDGE_CONFIG_GET ; Index: 0xA4 */
	(LTQ_ll_fkt) GSW_BridgeConfigGet,
	/* Command: GSW_BRIDGE_PORT_CONFIG_SET ; Index: 0xA5 */
	(LTQ_ll_fkt) GSW_BridgePortConfigSet,
	/* Command: GSW_BRIDGE_PORT_FREE ; Index: 0xA6 */
	(LTQ_ll_fkt) GSW_BridgePortFree,
	/* Command: GSW_BRIDGE_PORT_CONFIG_GET ; Index: 0xA7 */
	(LTQ_ll_fkt) GSW_BridgePortConfigGet,
	/* Command: GSW_CTP_PORT_CONFIG_SET ; Index: 0xA8 */
	(LTQ_ll_fkt) GSW_CtpPortConfigSet,
	/* Command: GSW_CTP_PORT_CONFIG_GET ; Index: 0xA9 */
	(LTQ_ll_fkt) GSW_CtpPortConfigGet,
	/* Command: GSW_EXTENDEDVLAN_SET ; Index: 0xAA */
	(LTQ_ll_fkt) GSW_ExtendedVlanSet,
	/* Command: GSW_EXTENDEDVLAN_FREE ; Index: 0xAB */
	(LTQ_ll_fkt) GSW_ExtendedVlanFree,
	/* Command: GSW_EXTENDEDVLAN_GET ; Index: 0xAC */
	(LTQ_ll_fkt) GSW_ExtendedVlanGet,
	/* Command: GSW_VLANFILTER_SET ; Index: 0xAD */
	(LTQ_ll_fkt) GSW_VlanFilterSet,
	/* Command: GSW_VLANFILTER_FREE ; Index: 0xAE */
	(LTQ_ll_fkt) GSW_VlanFilterFree,
	/* Command: GSW_VLANFILTER_GET ; Index: 0xAF */
	(LTQ_ll_fkt) GSW_VlanFilterGet,
	/* Command: GSW_QOS_METER_ALLOC ; Index: 0xB0 */
	(LTQ_ll_fkt) GSW_QOS_MeterAlloc,
	/* Command: GSW_QOS_METER_FREE ; Index: 0xB1 */
	(LTQ_ll_fkt) GSW_QOS_MeterFree,
	/* Command: GSW_QOS_COLOR_MARKING_TABLE_SET ; Index: 0xB2 */
	(LTQ_ll_fkt) GSW_QOS_ColorMarkingTableSet,
	/* Command: GSW_QOS_COLOR_MARKING_TABLE_GET ; Index: 0xB3 */
	(LTQ_ll_fkt) GSW_QOS_ColorMarkingTableGet,
	/* Command: GSW_QOS_COLOR_REMARKING_TABLE_SET ; Index: 0xB4 */
	(LTQ_ll_fkt) GSW_QOS_ColorReMarkingTableSet,
	/* Command: GSW_QOS_COLOR_REMARKING_TABLE_GET ; Index: 0xB5 */
	(LTQ_ll_fkt) GSW_QOS_ColorReMarkingTableGet,
	/* Command: GSW_DSCP2PCP_MAP_SET ; Index: 0xB6 */
	(LTQ_ll_fkt) GSW_QOS_Dscp2PcpTableSet,
	/* Command: GSW_DSCP2PCP_MAP_GET ; Index: 0xB7 */
	(LTQ_ll_fkt) GSW_QOS_Dscp2PcpTableGet,
	/* Command: GSW_PMAPPER_SET ; Index: 0xB8 */
	(LTQ_ll_fkt) GSW_QOS_PmapperTableSet,
	/* Command: GSW_PMAPPER_GET ; Index: 0xB9 */
	(LTQ_ll_fkt) GSW_QOS_PmapperTableGet,
	/* Command: GSW_DEFAUL_MAC_FILTER_SET ; Index: 0xBA */
	(LTQ_ll_fkt) GSW_DefaultMacFilterSet,
	/* Command: GSW_DEFAUL_MAC_FILTER_GET ; Index: 0xBB */
	(LTQ_ll_fkt) GSW_DefaultMacFilterGet,
	/* Command: GSW_CTP_PORT_CONFIG_RESET ; Index: 0xBC */
	(LTQ_ll_fkt) GSW_CtpPortConfigReset,
	/* Command: GSW_RMON_TFLOW_CLEAR ; Index: 0xBD */
	(LTQ_ll_fkt) GSW_RmonTflowClear,
	/* Command: GSW_TFLOW_COUNT_MODE_SET ; Index: 0xBE */
	(LTQ_ll_fkt) GSW_TflowCountModeSet,
	/* Command: GSW_TFLOW_COUNT_MODE_GET ; Index: 0xBF */
	(LTQ_ll_fkt) GSW_TflowCountModeGet,
	/* Command: GSW_TFLOW_COUNT_MODE_GET ; Index: 0xC0 */
	(LTQ_ll_fkt) GSW_CTP_PortAssignmentAlloc,
	/* Command: GSW_TFLOW_COUNT_MODE_GET ; Index: 0xC1 */
	(LTQ_ll_fkt) GSW_CTP_PortAssignmentFree,
	/* Command: GSW_EXTENDEDVLAN_ALLOC ; Index: 0xC2 */
	(LTQ_ll_fkt) GSW_ExtendedVlanAlloc,
	/* Command: GSW_VLANFILTER_ALLOC ; Index: 0xC3 */
	(LTQ_ll_fkt) GSW_VlanFilterAlloc,
	/* Command: GSW_BRIDGE_ALLOC ; Index: 0xC4 */
	(LTQ_ll_fkt) GSW_BridgeAlloc,
	/* Command: GSW_BRIDGE_PORT_ALLOC ; Index: 0xC5 */
	(LTQ_ll_fkt) GSW_BridgePortAlloc,
	/* Command: GSW_XGMAC_CFG ; Index: 0xC6 */
	(LTQ_ll_fkt) GSW_XgmacCfg,
	/* Command: GSW_GSWSS_CFG ; Index: 0xC7 */
	(LTQ_ll_fkt) GSW_GswssCfg,
	/* Command: GSW_LMAC_CFG ; Index: 0xC8 */
	(LTQ_ll_fkt) GSW_LmacCfg,
#if defined(WIN_PC_MODE) && WIN_PC_MODE
	/* Command: GSW_MACSEC_CFG ; Index: 0xC9 */
	(LTQ_ll_fkt) GSW_MacsecCfg,
#endif
	/* Command: GSW_DUMP_MEM ; Index: 0xCA */
	(LTQ_ll_fkt) GSW_DumpTable,
#if defined(WIN_PC_MODE) && WIN_PC_MODE
	/* Command: GSW_PMACBR_CFG ; Index: 0xCB */
	(LTQ_ll_fkt) GSW_PmacbrCfg,
#endif
	/* Command: GSW_VXLAN_CFG_GET ; Index: 0xCC */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, gsw_vxlan_cfg_get),
	/* Command: GSW_SVLAN_CFG_SET ; Index: 0xCD */
	(LTQ_ll_fkt) GSW_SW_FKT(CONFIG_MXL_VLAN, gsw_vxlan_cfg_set),
};

ltq_lowlevel_fkts_t GSW_FLOW_fkt_tbl = {
	NULL, /* pNext */
	(u16) GSW_MAGIC, /* nType */
	198, /* nNumFkts */
	ltq_fkt_ptr_tbl /* pFkts */
};

LTQ_ll_fkt gsw_flow_fkt_ptr_tbl[] = {
	/* 0x00 */
	(LTQ_ll_fkt) NULL,
	/* Command: GSW_REGISTER_SET ; Index: 0x01 */
	(LTQ_ll_fkt) GSW_RegisterSet,
	/* Command: GSW_REGISTER_GET ; Index: 0x02 */
	(LTQ_ll_fkt) GSW_RegisterGet,
	/* Command: GSW_IRQ_MASK_GET ; Index: 0x03 */
	(LTQ_ll_fkt) GSW_IrqMaskGet,
	/* Command: GSW_IRQ_MASK_SET ; Index: 0x04 */
	(LTQ_ll_fkt) GSW_IrqMaskSet,
	/* Command: GSW_IRQ_GET ; Index: 0x05 */
	(LTQ_ll_fkt) GSW_IrqGet,
	/* Command: GSW_IRQ_STATUS_CLEAR ; Index: 0x06 */
	(LTQ_ll_fkt) GSW_IrqStatusClear,
	/* Command: GSW_PCE_RULE_WRITE ; Index: 0x07 */
	(LTQ_ll_fkt) GSW_PceRuleWrite,
	/* Command: GSW_PCE_RULE_READ ; Index: 0x08 */
	(LTQ_ll_fkt) GSW_PceRuleRead,
	/* Command: GSW_PCE_RULE_DELETE ; Index: 0x09 */
	(LTQ_ll_fkt) GSW_PceRuleDelete,
	/* Command: GSW_RESET ; Index: 0x0A */
	(LTQ_ll_fkt) GSW_Reset,
	/* Command: GSW_RMON_EXTEND_GET ; Index: 0x0B */
	(LTQ_ll_fkt) GSW_RMON_ExtendGet,
	/* Command: GSW_TIMESTAMP_TIMER_SET ; Index: 0x0C */
	(LTQ_ll_fkt) GSW_TimestampTimerSet,
	/* Command: GSW_TIMESTAMP_TIMER_GET ; Index: 0x0D */
	(LTQ_ll_fkt) GSW_TimestampTimerGet,
	/* Command: GSW_TIMESTAMP_PORT_READ ; Index: 0x0E */
	(LTQ_ll_fkt) GSW_TimestampPortRead,
	/* Command: GSW_RMON_FLOW_GET ; Index: 0x0F */
	(LTQ_ll_fkt) GSW_RMON_FlowGet,
};

ltq_lowlevel_fkts_t ltq_flow_fkt_tbl = {
	&GSW_FLOW_fkt_tbl, /* pNext */
	(u16) GSW_FLOW_MAGIC, /* nType */
	16, /* nNumFkts */
	gsw_flow_fkt_ptr_tbl /* pFkts */
};

#ifdef __KERNEL__
LTQ_ll_fkt gsw_rt_fkt_ptr_tbl[] = {
	/* 0x00 */
	(LTQ_ll_fkt) NULL,
	/* Command: GSW_ROUTE_ENTRY_ADD ; Index: 0x01 */
	(LTQ_ll_fkt) GSW_ROUTE_SessionEntryAdd,
	/* Command: GSW_ROUTE_ENTRY_DELETE ; Index: 0x02 */
	(LTQ_ll_fkt) GSW_ROUTE_SessionEntryDel,
	/* Command: GSW_ROUTE_ENTRY_READ ; Index: 0x03 */
	(LTQ_ll_fkt) GSW_ROUTE_SessionEntryRead,
	/* Command: GSW_ROUTE_TUNNEL_ENTRY_ADD ; Index: 0x04 */
	(LTQ_ll_fkt) GSW_ROUTE_TunnelEntryAdd,
	/* Command: GSW_ROUTE_TUNNEL_ENTRY_DELETE ; Index: 0x05 */
	(LTQ_ll_fkt) GSW_ROUTE_TunnelEntryDel,
	/* Command: GSW_ROUTE_TUNNEL_ENTRY_READ ; Index: 0x06 */
	(LTQ_ll_fkt) GSW_ROUTE_TunnelEntryRead,
	/* Command: GSW_ROUTE_L2NAT_CFG_WRITE ; Index: 0x07 */
	(LTQ_ll_fkt) GSW_ROUTE_L2NATCfgWrite,
	/* Command: GSW_ROUTE_L2NAT_CFG_READ ; Index: 0x08 */
	(LTQ_ll_fkt) GSW_ROUTE_L2NATCfgRead,
	/* Command: GSW_ROUTE_SESSION_HIT_OP ; Index: 0x09 */
	(LTQ_ll_fkt) GSW_ROUTE_SessHitOp,
	/* Command: GSW_ROUTE_SESSION_DEST_MOD ; Index: 0x0A */
	(LTQ_ll_fkt) GSW_ROUTE_SessDestModify,
};

ltq_lowlevel_fkts_t ltq_rt_fkt_tbl = {
	&ltq_flow_fkt_tbl, /* pNext */
	(u16) GSW_ROUTE_MAGIC, /* nType */
	11, /* nNumFkts */
	gsw_rt_fkt_ptr_tbl /* pFkts */
};

#endif
