/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2020 - 2024 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/

#ifndef _QOS_TC_FLOWER_
#define _QOS_TC_FLOWER_

enum qos_tc_flower_type {
	TC_TYPE_UNKNOWN = 0,
	TC_TYPE_EXT_VLAN = 1,
	TC_TYPE_VLAN_FILTER = 2,
	TC_TYPE_QUEUE = 3,
	TC_TYPE_MIRRED = 4,
	TC_TYPE_COLMARK = 5,
	TC_TYPE_TRAP = 6,
	TC_TYPE_POLICE = 7,
	TC_TYPE_IP_DROP = 8,
	TC_TYPE_SKBEDIT = 9,
	TC_TYPE_EXTRACT_CFM = 10,
};

enum cfm_act_type {
	CFM_ACT_EXT1 = BIT(0),
	CFM_ACT_EXT1_SAMPLE = BIT(1),
	CFM_ACT_EXT2 = BIT(2),
	CFM_ACT_EXT2_SAMPLE = BIT(3),
	CFM_ACT_TX_TS = BIT(4),
	CFM_ACT_TX_SAMPLE = BIT(5),
	CFM_ACT_DROP = BIT(6),
};

int qos_tc_flower_storage_add(struct net_device *dev,
			      unsigned long cookie,
			      enum qos_tc_flower_type type,
			      void *arg1, void *arg2);

#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
bool has_action_id(struct flow_cls_offload *f,
		   bool (*check)(const struct tc_action *a));
#elif (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE)
bool has_action_id(struct flow_cls_offload *f,
		   bool (*check)(const struct tc_action *a));
#elif (KERNEL_VERSION(5, 1, 0) > LINUX_VERSION_CODE)
bool has_action_id(struct flow_cls_offload *f,
		   bool (*check)(const struct tc_action *a));
#else
bool has_action_id(struct flow_cls_offload *f, enum flow_action_id id);
#endif

struct net_device *qos_tc_get_indev(struct net_device *dev,
				    struct flow_cls_offload *f);

void qos_tc_storage_debugfs(struct seq_file *file, void *ctx);

int qos_tc_get_cfm_act(struct net_device *dev, struct flow_cls_offload *f);

#endif
