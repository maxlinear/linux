/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2026 MaxLinear, Inc.
 *
 *****************************************************************************/
#ifndef _QOS_TC_CPU_QOS_
#define _QOS_TC_CPU_QOS_

#include <linux/pp_api.h>
#include "switch/qos_tc_qmap.h"

/*!
  \brief QoS Ingress Group list item structure
 */
struct iface_info {
	char		ifname[IFNAMSIZ]; /* Ingress Interface name */
	uint32_t	gpid; /* Corresponding GPID */
};

/*!
  \brief Rule tracking structure for reference counting
 */
struct rule_info {
	u32 prio;               /* Rule priority */
	u8 ip_proto;            /* IP protocol */
	__be16 src_port;        /* Source port */
	__be16 dst_port;        /* Destination port */
	__be32 src_ip;          /* Source IP */
	__be32 dst_ip;          /* Destination IP */
	int ref_count;          /* Number of filters using this rule */
	struct list_head list;  /* List node */
};

/*!
  \brief QoS Ingress Group list item structure
 */
struct grp_info {
    char                grp_name[IFNAMSIZ];
    int32_t             pp_grp_id;
    /* Combined structure for ifname and gpid */
    struct iface_info   iface_info[5];
    struct pp_cpu_info  cpu_q_info; /* cpu queue information */
    struct list_head    rules_list; /* List of rules with ref counts */
    struct list_head    list;
};

/*!
  \brief CPU Queue Information Structure
 */
struct cpu_queue_info {
	int32_t cpu_high_q[7]; /* CPU high queue information */
	int32_t cpu_high_gpid; /* High CPU GPIDs */
	int32_t cpu_low_q[6]; /* CPU low queue information */
	int32_t cpu_low_gpid; /* Low CPU GPIDs */
};

struct qos_inggrp_info {
	struct cpu_queue_info cpu_q_info[4];
	bool q_update_done; /* Flag to check whether CPU queue info is updated */
	struct list_head grp_list_info;
};

int qos_tc_cpu_qos_offload(struct net_device *dev, struct flow_cls_offload *f);
int qos_tc_prio_update_cpu_info(struct qos_tc_qdisc *sch);
int qos_tc_drr_update_cpu_info(struct qos_tc_qdisc *sch, int idx);
int qos_tc_cpu_ingg_qos_add(struct net_device *dev,
			    struct flower_cls_map *map,
			    struct flow_cls_offload *f);
int qos_tc_cpu_ingg_qos_delete(struct net_device *dev,
			       struct net_device *indev,
			       struct flower_cls_map *map);

/* Helper functions for CPU port handling */
int qos_tc_validate_cpu_port_scheduler(const struct qos_tc_qdisc *sch,
				       bool is_wrr_type);

#endif
