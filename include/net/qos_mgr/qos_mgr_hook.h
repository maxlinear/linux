// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 MaxLinear, Inc.
   Copyright (C) 2019~2020 Intel Corporation.
 */
#ifndef __QOS_MGR_HOOK_H
#define __QOS_MGR_HOOK_H
/*! \file qos_mgr_hook.h
  \brief This file contains: QOS Manager Hooks.
 */

#include <net/qos_mgr_common.h>

struct qos_hal_qos_stats {
	uint64_t enqPkts; /* Enqueued packets Count */
	uint64_t enqBytes; /* Enqueued Bytes Count */
	uint64_t deqPkts; /* Dequeued packets Count */
	uint64_t deqBytes; /* Dequeued Bytes Count */
	uint64_t dropPkts; /* Dropped Packets Count */
	uint64_t dropBytes; /* Dropped Bytes Count - UNUSED for now */
	uint32_t qOccPkts; /* Queue Occupancy Packets Count -
				Only at Queue level */
};

/* PPA class2prio notifier chain. */
#define QOS_MGR_CLASS2PRIO_DEFAULT	0x0001 /* Notify class2prio default */
#define QOS_MGR_CLASS2PRIO_CHANGE	0x0002 /* Notify class2prio change */

struct qos_mgr_class2prio_notifier_info {
	s32 port_id;
	struct net_device *dev;
	u8 class2prio[MAX_TC_NUM];
};
extern int qos_mgr_register_event_notifier(QOS_MGR_NOTIFIER_BLOCK *nb);
extern int qos_mgr_unregister_event_notifier(QOS_MGR_NOTIFIER_BLOCK *nb);
extern int qos_mgr_call_class2prio_notifiers(unsigned long val,
					 s32 port_id, struct net_device *dev,
					 u8 class2prio[MAX_TC_NUM]);

int qos_hal_setup_dp_ingress_connectivity(
		struct net_device *netdev,
		uint32_t pmac_port);
int qos_hal_setup_dp_egress_connectivity(
		struct net_device *netdev,
		uint32_t pmac_port);
int qos_hal_remove_dp_egress_connectivity(
		struct net_device *netdev,
		uint32_t pmac_port);
int qos_hal_remove_dp_ingress_connectivity(
		struct net_device *netdev,
		uint32_t pmac_port);
extern int32_t (*qos_hal_get_csum_ol_mib_hook_fn)(
		struct qos_hal_qos_stats *csum_mib,
		uint32_t flag);
extern int32_t (*qos_hal_clear_csum_ol_mib_hook_fn)(
		uint32_t flag);
extern int32_t(*qos_hal_get_qos_mib_hook_fn)(
		struct net_device *netdev,
		dp_subif_t *subif_id,
		int32_t queueid,
		struct qos_hal_qos_stats *qos_mib,
		uint32_t flag);
extern int32_t (*qos_hal_clear_qos_mib_hook_fn)(
		struct net_device *netdev,
		dp_subif_t *subif_id,
		int32_t queueid,
		uint32_t flag);

extern int32_t (*qos_mgr_br2684_get_vcc_fn)(
		struct net_device *netdev,
		struct atm_vcc **pvcc);

#endif  /*end of __QOS_MGR_HOOK_H */
