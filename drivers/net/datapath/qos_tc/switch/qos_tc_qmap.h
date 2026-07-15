/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2021 - 2026 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/
#ifndef _QOS_TC_QMAP_
#define _QOS_TC_QMAP_

#include <net/qos_tc.h>

struct flower_cls_map {
	__be16 proto;
	u32 pref;
	u32 classid;
	u32 qid;
	char tc_cookie;
	bool ingress;
	struct flow_dissector_key_vlan key;
	struct flow_dissector_key_vlan mask;
	struct flow_dissector_key_basic basic_key;
	struct flow_dissector_key_basic basic_mask;
	struct flow_dissector_key_ports ports_key;
	struct flow_dissector_key_ports ports_mask;
	struct flow_dissector_key_ipv4_addrs ipv4_key;
	struct flow_dissector_key_ipv4_addrs ipv4_mask;
	struct net_device *dev;
	int in_ifi;
	int tc;
	int subif;
	struct list_head list;
};

int qos_tc_map(struct net_device *dev, struct flow_cls_offload *f,
	       bool ingress, const struct qos_tc_params *tc_params);
int qos_tc_unmap(struct net_device *dev, void *list_node,
		const struct qos_tc_params *tc_params);
int qos_tc_classid_unmap(u32 classid);

void qos_tc_class_list_debugfs(struct seq_file *file, void *ctx);
int qos_tc_get_flower_cookie(struct flow_cls_offload *f, char *tc_cookie);

#endif
