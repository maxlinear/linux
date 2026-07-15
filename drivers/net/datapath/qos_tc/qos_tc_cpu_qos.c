// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 *
 * Copyright (c) 2026 MaxLinear, Inc.
 *
 *****************************************************************************/

#include <net/datapath_api.h>
#include <net/datapath_api_qos.h>
#include "qos_tc_trace.h"
#include "qos_tc_cpu_qos.h"
#include "switch/qos_tc_qmap.h"

#define MAX_CPU			4
#define MAX_HP_QUEUE	7 /* High priority queues */
#define MAX_LP_QUEUE	6 /* Low priority queues */
#define MAX_CPU_INGG_GRPS	6 /* Max CPU ingress groups (group1 to 6) */

/* cpu groups information */
static struct qos_inggrp_info g_qos_inggrp_info = {
    .grp_list_info = LIST_HEAD_INIT(g_qos_inggrp_info.grp_list_info),
};

/**
 * This function initialize default CPU queue information.
 * Fetches the CPU queue resources and initializes the global CPU queue info
 * structure with default queue IDs and GPIDs.
 * Each CPU has two ports: high (even) for priority traffic, low (odd) for
 * regular.
 * Return: 0 on success, negative error code on failure
 */
static int qos_tc_update_default_queue_info(void)
{
    /* Track lowest queue IDs and corresponding GPIDs per CPU */
    int low_qid[MAX_CPU] = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
    int high_qid[MAX_CPU] = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
    int low_gpid[MAX_CPU] = {0};
    int high_gpid[MAX_CPU] = {0};
    struct dp_dequeue_res *dq_res __free(kfree) =
		kzalloc(sizeof(*dq_res), GFP_KERNEL);
    struct dp_queue_res *q_res __free(kfree) = NULL;
    int i, j;

    if (!dq_res) {
        pr_err("%s: Failed to allocate memory for dp_dequeue_res\n",
	       __func__);
        return -ENOMEM;
    }

    /* dp port zero is used to retrieve all CPU queue details */
    dq_res->dp_port = 0;
    dq_res->cqm_deq_idx = DEQ_PORT_OFFSET_ALL;

    /* First call to get number of queues */
    if (dp_deq_port_res_get(dq_res, 0) != 0) {
        pr_err("%s: Failed to get number of queues\n", __func__);
        return -EINVAL;
    }

    q_res = kzalloc(sizeof(struct dp_queue_res) * dq_res->num_q,
		    GFP_KERNEL);
    if (!q_res) {
        pr_err("%s: Failed to allocate memory for queue resources\n",
	       __func__);
        return -ENOMEM;
    }

    dq_res->q_res = q_res;
    dq_res->q_res_size = dq_res->num_q;

    if (dp_deq_port_res_get(dq_res, 0) != 0) {
        pr_err("%s: Failed to get queue resources\n", __func__);
        return -EINVAL;
    }


    /* Iterate over all queue entries to find the lowest q_id per port
     * type per CPU
     */
    for (i = 0; i < dq_res->num_q; i++) {
        struct dp_queue_res *q = &dq_res->q_res[i];
        int cpu_idx = q->cpu_id;
        int port = q->cqm_deq_port;

        /* Validate CPU index */
        if (cpu_idx >= MAX_CPU)
            continue;

        if (port % 2 == 0) {
            /* High port (even-numbered) */
            if (q->q_id < high_qid[cpu_idx]) {
                high_qid[cpu_idx] = q->q_id;
                high_gpid[cpu_idx] = q->cpu_gpid;
            }
        } else {
            /* Low port (odd-numbered) */
            if (q->q_id < low_qid[cpu_idx]) {
                low_qid[cpu_idx] = q->q_id;
                low_gpid[cpu_idx] = q->cpu_gpid;
            }
        }
    }

    /* Update cpu_queue_info structure with default queue IDs and GPIDs */
    for (i = 0; i < MAX_CPU; i++) {
        /* Set GPIDs for high and low ports */
        g_qos_inggrp_info.cpu_q_info[i].cpu_high_gpid = high_gpid[i];
        g_qos_inggrp_info.cpu_q_info[i].cpu_low_gpid = low_gpid[i];

        /* Fill all high queue slots with the lowest high q_id */
        for (j = 0; j < MAX_HP_QUEUE; j++) {
            g_qos_inggrp_info.cpu_q_info[i].cpu_high_q[j] = high_qid[i];
        }

        /* Fill all low queue slots with the lowest low q_id */
        for (j = 0; j < MAX_LP_QUEUE; j++) {
            g_qos_inggrp_info.cpu_q_info[i].cpu_low_q[j] = low_qid[i];
        }
    }

    /* Mark update as complete */
    g_qos_inggrp_info.q_update_done = true;

    return 0;
}

/**
 * Update CPU high priority queue info from PRIO scheduler
 * Updates the global CPU queue info with queue IDs from the PRIO scheduler.
 * Only processes high priority ports (even-numbered EPNs 0,2,4,6).
 */
int qos_tc_prio_update_cpu_info(struct qos_tc_qdisc *sch)
{
    int i, idx = 0, ret, cpu_id, max_bands;

	if (!(sch->data_flag & DP_SUBIF_CPU_HIGH_PRI))
		return 0;

    if (!g_qos_inggrp_info.q_update_done) {
        ret = qos_tc_update_default_queue_info();
        if (ret != 0) {
            netdev_err(sch->dev, "Failed to update default queue information\n");
            return -EINVAL;
        }
    }

	/* Map even-numbered EPNs(CPU high port - 0,2,4,6) to CPU IDs (0,1,2,3) */
	cpu_id = sch->epn / 2;
	max_bands = sch->prio.bands ? sch->prio.bands : (QOS_TC_MAX_Q - 1);
	for (i = 0; i < max_bands; i++) {
		if (sch->qids[i].qid) {
			idx = sch->prio.priomap[i];
			g_qos_inggrp_info.cpu_q_info[cpu_id].cpu_high_q[idx] =
				sch->qids[idx].qid;
		} else {
			g_qos_inggrp_info.cpu_q_info[cpu_id].cpu_high_q[i] =
				sch->def_q;
		}
	}

    return 0;
}

/**
 * Update CPU low priority queue info from DRR scheduler
 * Updates the global CPU queue info with queue IDs from the DRR scheduler.
 * Only processes low priority ports (odd-numbered EPNs 1,3,5,7).
 */
int qos_tc_drr_update_cpu_info(struct qos_tc_qdisc *sch, int idx)
{
    int ret, cpu_id;

	if (!(sch->data_flag & DP_SUBIF_CPU_LOW_PRI))
		return 0;

    if (!g_qos_inggrp_info.q_update_done) {
        ret = qos_tc_update_default_queue_info();
        if (ret != 0) {
            netdev_err(sch->dev, "Failed to update default queue information\n");
            return -EINVAL;
        }
    }

	/* Map odd-numbered EPNs (1,3,5,7) to CPU IDs (0,1,2,3)
	 * Subtract 1 then divide by 2: EPN 1->CPU 0, EPN 3->CPU 1, EPN 5->CPU 2, EPN 7->CPU 3 */
	cpu_id = (sch->epn - 1) / 2;
	if (sch->qids[idx].qid)
		g_qos_inggrp_info.cpu_q_info[cpu_id].cpu_low_q[idx] = sch->qids[idx].qid;
	else
		g_qos_inggrp_info.cpu_q_info[cpu_id].cpu_low_q[idx] = sch->def_q;

    return 0;
}

static struct grp_info *qos_tc_get_group_by_cookie(uint8_t cookie)
{
    char group_name[IFNAMSIZ];
    struct grp_info *grp;

    snprintf(group_name, sizeof(group_name), "group%hhd", cookie);

    list_for_each_entry(grp, &g_qos_inggrp_info.grp_list_info, list) {
        if (strcmp(grp->grp_name, group_name) == 0)
            return grp;
    }
    return NULL;
}

static bool qos_tc_is_group_exist(uint8_t cookie)
{
    return qos_tc_get_group_by_cookie(cookie) != NULL;
}

/**
 * Fill PP CPU info structure for group creation/update
 * Fills CPU info with 7 high priority queues + 1 low priority queue per CPU.
 * The low priority queue is selected based on cookie value (cookie-1 as
 * index).
 */
static unsigned int qos_tc_fill_cpu_info(struct pp_cpu_info *cpu_info_ptr,
                                         uint8_t cookie)
{
    int i, j;
    unsigned int num_cpus = 0;
    struct dp_qos_q_logic q_cfg;

    /* Validate cookie before using as array index */
    if (cookie < 1 || cookie > MAX_CPU_INGG_GRPS) {
        pr_err("%s: Invalid cookie %u, valid range is 1-%d\n",
               __func__, cookie, MAX_CPU_INGG_GRPS);
        return 0;
    }

    for (i = 0; i < MAX_CPU; i++) {
        struct pp_cpu_info *cpu_info = &cpu_info_ptr[i];
        struct cpu_queue_info *src = &g_qos_inggrp_info.cpu_q_info[i];

        if (!cpu_online(i))
            continue;

        num_cpus++;
        cpu_info->num_q = PP_MAX_CPU_QUEUES;

        /* Fill 7 high priority queues */
        for (j = 0; j < MAX_HP_QUEUE; j++) {
            q_cfg.inst = 0;
            q_cfg.q_id = src->cpu_high_q[j];
            if (dp_qos_get_q_logic(&q_cfg, 0) != DP_SUCCESS) {
                pr_err("%s: Failed to get logical queue ID for CPU %d queue %d\n",
                       __func__, i, j);
                return 0;
            }
            cpu_info->queue[j].id = q_cfg.q_logic_id;
            cpu_info->queue[j].gpid = src->cpu_high_gpid;
        }

        /* One low priority queue for unclassified traffic */
        q_cfg.inst = 0;
        q_cfg.q_id = src->cpu_low_q[cookie - 1];
        if (dp_qos_get_q_logic(&q_cfg, 0) != DP_SUCCESS) {
            pr_err("%s: Failed to get logical queue ID for CPU %d low queue\n",
                   __func__, i);
            return 0;
        }
        cpu_info->queue[j].id = q_cfg.q_logic_id;
        cpu_info->queue[j].gpid = src->cpu_low_gpid;
    }

    return num_cpus;
}

/**
 * Creates a new PP ingress group with CPU queue configuration and adds
 * it to the global group list.
 */
static int qos_tc_create_pp_ing_group(struct net_device *dev, uint8_t cookie)
{
    int pp_grp_id;
    unsigned int num_cpus = 0;
    char group_name[IFNAMSIZ];
    struct grp_info *pp_grp;
    struct pp_cpu_info *cpu_info_ptr __free(kfree) = NULL;

    /* Validate cookie range: Valid range is group1-6 */
    if (cookie < 1 || cookie > MAX_CPU_INGG_GRPS) {
        pr_err("%s: Invalid cookie %u, valid range is 1-%d\n", __func__, cookie,
				MAX_CPU_INGG_GRPS);
        return -EINVAL;
    }

    snprintf(group_name, sizeof(group_name), "group%hhd", cookie);

    cpu_info_ptr = kzalloc(MAX_CPU * sizeof(struct pp_cpu_info),
			   GFP_KERNEL);
    if (!cpu_info_ptr) {
        pr_err("%s: Failed to allocate memory for CPU info\n", __func__);
        return -ENOMEM;
    }

    /*
     * Fill cpu_info_ptr with the CPU high and low priority queues
     * - 7 queues from each CPU high port for protected high priority traffic.
     * - 1 queue from each CPU low port for unclassified traffic.
     */
    num_cpus = qos_tc_fill_cpu_info(cpu_info_ptr, cookie);
    if (num_cpus == 0) {
        pr_err("%s: Failed to fill CPU info\n", __func__);
        return -EINVAL;
    }

    pp_grp_id = dp_gpid_group_create(0, group_name, cpu_info_ptr, num_cpus);

    if (pp_grp_id < 0) {
        pr_err("%s: Failed to create PP group\n", __func__);
        return -EINVAL;
    }

    pp_grp = kzalloc(sizeof(struct grp_info), GFP_KERNEL);
    if (!pp_grp) {
        pr_err("%s: Failed to allocate memory for new group\n", __func__);
        dp_gpid_group_delete(0, pp_grp_id);
        return -ENOMEM;
    }

    snprintf(pp_grp->grp_name, sizeof(pp_grp->grp_name), "group%hhd", cookie);
    pp_grp->pp_grp_id = pp_grp_id;
    INIT_LIST_HEAD(&pp_grp->list);
    INIT_LIST_HEAD(&pp_grp->rules_list);
    list_add(&pp_grp->list, &g_qos_inggrp_info.grp_list_info);

    return pp_grp_id;
}

static bool qos_tc_is_port_added_to_group(struct net_device *dev,
					   uint8_t cookie)
{
    struct grp_info *grp;
    int i;

    grp = qos_tc_get_group_by_cookie(cookie);
    if (!grp) {
        return false;
	}

    for (i = 0; i < ARRAY_SIZE(grp->iface_info); i++) {
        if (strncmp(grp->iface_info[i].ifname, dev->name,
		    IFNAMSIZ) == 0) {
            return true;
        }
    }

    return false;
}

static bool qos_tc_is_port_in_any_group(struct net_device *dev)
{
    struct grp_info *grp;
    int i;

    list_for_each_entry(grp, &g_qos_inggrp_info.grp_list_info, list) {
        for (i = 0; i < ARRAY_SIZE(grp->iface_info); i++) {
            if (strncmp(grp->iface_info[i].ifname, dev->name,
			IFNAMSIZ) == 0) {
                return true;
            }
        }
    }

    return false;
}

static int qos_tc_group_add_port(struct net_device *dev,
				  struct flow_cls_offload *f)
{
    int i, ret;
    u32 indev_gpid;
    struct grp_info *grp;
    struct net_device *indev;
	dp_subif_t *subif __free(kfree) = NULL;
	char cookie = QOS_TC_COOKIE_EMPTY;

	ret = qos_tc_get_flower_cookie(f, &cookie);
	if (ret < 0) {
		pr_debug("%s: Failed to get cookie\n", __func__);
		return ret;
	}

    indev = qos_tc_get_indev(dev, f);
    if (!indev) {
        return -EINVAL;
	}

    if (qos_tc_is_port_added_to_group(indev, cookie))
		return 0;

    /* Check if port is already in a different group */
    if (qos_tc_is_port_in_any_group(indev)) {
		pr_err("%s: %s is already part of another group\n",
		       __func__, indev->name);
		return -EEXIST;
    }

    grp = qos_tc_get_group_by_cookie(cookie);
    if (!grp) {
        return -EINVAL;
	}

	subif = kzalloc(sizeof(*subif), GFP_KERNEL);
	if (!subif) {
		pr_err("%s: failed to allocate memory for subif\n",
		       __func__);
		return -EINVAL;
	}

	ret = dp_get_netif_subifid(indev, NULL, NULL, 0, subif, 0);
	if (ret == DP_FAILURE) {
		pr_err("%s: subif get failed for %s\n",
		       __func__, dev->name);
		return -EINVAL;
	}
	indev_gpid = subif->gpid;

	ret = dp_gpid_group_add_port(0, grp->pp_grp_id, indev_gpid, 7);
	if (ret < 0) {
		pr_err("%s: failed to add port to group\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(grp->iface_info); i++) {
		if (grp->iface_info[i].ifname[0] == '\0') {
			strlcpy(grp->iface_info[i].ifname, indev->name,
				IFNAMSIZ);
			grp->iface_info[i].gpid = indev_gpid;
			break;
		}
	}

	return ret;
}

int qos_tc_get_queue_prio(struct net_device *dev, unsigned int qid)
{
	int i;
	struct qos_tc_port *port;
	struct qos_tc_qdisc *sch;

	port = qos_tc_port_get(dev);
	if (!port) {
		pr_err("%s: qos port does not exist\n", __func__);
		return -ENODEV;
	}
	sch = &port->root_qdisc;

	for (i = 0; i < QOS_TC_MAX_Q; i++) {
		if (qid == sch->qids[i].qid)
			return i;
	}

	return -1;
}

/**
 * Converts TC flower match fields (VLAN, IP proto, ports, IPs) into
 * PP whitelist field format for classification rules.
 */
static int qos_tc_build_rule_fields(struct flower_cls_map *map,
				    struct pp_whitelist_field *fields)
{
	int cnt = 0;

	/* VLAN Ethertype */
	if (map->key.vlan_tpid) {
		fields[cnt].type = PP_FLD_ETH_TYPE;
		fields[cnt].eth_type = map->key.vlan_tpid;
		cnt++;
		pr_info("%s: VLAN Ethertype: 0x%04x\n", __func__,
			ntohs(map->key.vlan_tpid));
	}

	/* IP Protocol */
	if (map->basic_key.ip_proto) {
		fields[cnt].type = PP_FLD_IP_PROTO;
		fields[cnt].ip_proto = map->basic_key.ip_proto;
		cnt++;
		pr_info("%s: IP Proto: %u\n", __func__, map->basic_key.ip_proto);
	}

	/* Source Port */
	if (map->ports_key.src) {
		fields[cnt].type = PP_FLD_L4_SRC_PORT;
		fields[cnt].src_port = map->ports_key.src;
		cnt++;
		pr_info("%s: Src Port: %u\n", __func__, ntohs(map->ports_key.src));
	}

	/* Destination Port */
	if (map->ports_key.dst) {
		fields[cnt].type = PP_FLD_L4_DST_PORT;
		fields[cnt].dst_port = map->ports_key.dst;
		cnt++;
		pr_info("%s: Dst Port: %u\n", __func__, ntohs(map->ports_key.dst));
	}

	/* Source IP */
	if (map->ipv4_key.src) {
		fields[cnt].type = PP_FLD_IPV4_SRC_IP;
		fields[cnt].src_ip.v4 = map->ipv4_key.src;
		cnt++;
		pr_info("%s: Src IP: %pI4\n", __func__, &map->ipv4_key.src);
	}

	/* Destination IP */
	if (map->ipv4_key.dst) {
		fields[cnt].type = PP_FLD_IPV4_DST_IP;
		fields[cnt].dst_ip.v4 = map->ipv4_key.dst;
		cnt++;
		pr_info("%s: Dst IP: %pI4\n", __func__, &map->ipv4_key.dst);
	}

	return cnt;
}

/**
 * Find a rule in the group's rules list
 * Returns pointer to rule_info if found, NULL otherwise
 */
static struct rule_info *qos_tc_find_rule(struct grp_info *grp,
					  struct flower_cls_map *map, u32 prio)
{
	struct rule_info *rule;

	list_for_each_entry(rule, &grp->rules_list, list) {
		if (rule->prio == prio &&
		    rule->ip_proto == map->basic_key.ip_proto &&
		    rule->src_port == map->ports_key.src &&
		    rule->dst_port == map->ports_key.dst &&
		    rule->src_ip == map->ipv4_key.src &&
		    rule->dst_ip == map->ipv4_key.dst) {
			return rule;
		}
	}
	return NULL;
}

/**
 * Add or increment reference for a rule
 * Returns 0 if rule already exists (ref incremented), 1 if new rule created,
 * negative on error
 */
static int qos_tc_add_rule_ref(struct grp_info *grp,
			       struct flower_cls_map *map, u32 prio)
{
	struct rule_info *rule;

	rule = qos_tc_find_rule(grp, map, prio);
	if (rule) {
		rule->ref_count++;
		pr_debug("%s: Rule exists, incremented ref_count to %d\n",
			 __func__, rule->ref_count);
		return 0; /* Rule already exists */
	}

	/* Create new rule */
	rule = kzalloc(sizeof(*rule), GFP_KERNEL);
	if (!rule)
		return -ENOMEM;

	rule->prio = prio;
	rule->ip_proto = map->basic_key.ip_proto;
	rule->src_port = map->ports_key.src;
	rule->dst_port = map->ports_key.dst;
	rule->src_ip = map->ipv4_key.src;
	rule->dst_ip = map->ipv4_key.dst;
	rule->ref_count = 1;
	INIT_LIST_HEAD(&rule->list);
	list_add(&rule->list, &grp->rules_list);

	return 1;
}

/**
 * Decrement reference for a rule
 * Returns true if rule should be deleted from PP (ref_count reached 0),
 * false otherwise
 */
static bool qos_tc_remove_rule_ref(struct grp_info *grp,
				   struct flower_cls_map *map, u32 prio)
{
	struct rule_info *rule;

	rule = qos_tc_find_rule(grp, map, prio);
	if (!rule) {
		pr_debug("%s: Rule not found in tracking list\n", __func__);
		return false; /* Rule not found, shouldn't happen */
	}

	rule->ref_count--;
	pr_debug("%s: Decremented ref_count to %d\n", __func__, rule->ref_count);

	if (rule->ref_count <= 0) {
		/* Remove from list and free */
		list_del(&rule->list);
		kfree(rule);
		pr_debug("%s: Rule removed from tracking list, "
			 "delete from PP\n", __func__);
		return true; /* Delete from PP */
	}

	pr_debug("%s: Rule still has %d references, keeping in PP\n",
		 __func__, rule->ref_count);
	return false;
}

/**
 * Builds classification rule fields from TC flower match and adds the rule
 * to the specified PP group with priority derived from queue ID.
 * Uses reference counting to avoid duplicate rule addition to PP.
 */
int qos_tc_gpid_group_rule_add(struct net_device *dev,
			       struct flower_cls_map *map,
			       int grp_id, struct grp_info *grp)
{
	struct pp_whitelist_field fields[PP_FLD_COUNT];
	int cnt, prio, ret, rule_ref_ret;

	cnt = qos_tc_build_rule_fields(map, fields);
	if (cnt == 0) {
		pr_debug("%s: No valid fields to add\n", __func__);
		return -EINVAL;
	}

	prio = qos_tc_get_queue_prio(dev, map->qid);
	if (prio == -1) {
		pr_debug("%s: Invalid prio: %d\n", __func__, prio);
		return -EINVAL;
	}
	pr_debug("%s: prio: %d map->tc: %d pref: %d\n",
		 __func__, prio, map->tc, map->pref);

	/* Check if rule already exists and manage reference count */
	rule_ref_ret = qos_tc_add_rule_ref(grp, map, prio);
	if (rule_ref_ret < 0)
		return rule_ref_ret;

	if (rule_ref_ret == 0) {
		/* Rule already exists in PP, just incremented ref count */
		pr_debug("%s: Rule already exists, skipping PP add\n",
			 __func__);
		return 0;
	}

	/* New rule, add to PP */
	ret = dp_gpid_group_rule_add(0, grp_id, prio, fields, cnt);
	if (ret < 0) {
		/* Failed to add to PP, remove the reference we just added */
		qos_tc_remove_rule_ref(grp, map, prio);
		return ret;
	}

	return 0;
}

/**
 * Updates an existing PP group with current CPU queue configuration.
 * Called when adding subsequent rules to an existing group.
 */
static int qos_tc_update_pp_ing_group(struct net_device *dev, uint8_t cookie)
{
    struct grp_info *grp;
    struct pp_cpu_info *cpu_info_ptr __free(kfree) = NULL;
    unsigned int num_cpus;
    int ret;

    grp = qos_tc_get_group_by_cookie(cookie);
    if (!grp) {
        pr_err("%s: Group not found for cookie %hhd\n", __func__, cookie);
        return -EINVAL;
    }

    cpu_info_ptr = kzalloc(MAX_CPU * sizeof(struct pp_cpu_info), GFP_KERNEL);
    if (!cpu_info_ptr) {
        pr_err("%s: Failed to allocate memory for CPU info\n", __func__);
        return -ENOMEM;
    }

    num_cpus = qos_tc_fill_cpu_info(cpu_info_ptr, cookie);

    ret = dp_gpid_group_update(0, grp->pp_grp_id, cpu_info_ptr, num_cpus);
    if (ret < 0) {
        pr_err("%s: Failed to update PP group %d\n", __func__, grp->pp_grp_id);
    }

    return ret;
}

int qos_tc_cpu_ingg_qos_add(struct net_device *dev, struct flower_cls_map *map,
	struct flow_cls_offload *f)
{
	int pp_grp_id, ret;
	char cookie = QOS_TC_COOKIE_EMPTY;
	struct grp_info *grp;

	ret = qos_tc_get_flower_cookie(f, &cookie);
	if (ret < 0) {
		pr_debug("%s: Failed to get cookie\n", __func__);
		return ret;
	}

	/* Create or get existing group */
	if (!qos_tc_is_group_exist(cookie)) {
		pp_grp_id = qos_tc_create_pp_ing_group(dev, cookie);
		if (pp_grp_id < 0) {
			pr_err("%s: failed to create group%hhd\n", __func__, cookie);
			return -EINVAL;
		}

		/* Retrieve the newly created group */
		grp = qos_tc_get_group_by_cookie(cookie);
		if (!grp) {
			pr_err("%s: failed to retrieve newly created group%hhd\n",
			       __func__, cookie);
			return -EINVAL;
		}
	} else {
		pr_debug("%s: Group already exists for cookie %d, "
			 "updating...\n", __func__, cookie);
		ret = qos_tc_update_pp_ing_group(dev, cookie);
		if (ret < 0) {
			pr_err("%s: failed to update group%hhd\n", __func__, cookie);
			return -EINVAL;
		}
		grp = qos_tc_get_group_by_cookie(cookie);
		if (!grp) {
			pr_err("%s: failed to get group%hhd\n", __func__, cookie);
			return -EINVAL;
		}
		pp_grp_id = grp->pp_grp_id;
	}

	/* Add port to the group */
	ret = qos_tc_group_add_port(dev, f);
	if (ret < 0) {
		pr_err("%s: failed to add port %s to the group\n",
		       __func__, dev->name);
		return ret;
	}

	/* Add rule with reference counting - grp is guaranteed to be set by now */
	ret = qos_tc_gpid_group_rule_add(dev, map, pp_grp_id, grp);
	if (ret < 0) {
		pr_debug("%s: failed to add rule\n", __func__);
		return -EINVAL;
	}

	return 0;
}

int qos_tc_gpid_group_rule_delete(struct net_device *dev,
				  struct flower_cls_map *map,
				  int grp_id, struct grp_info *grp)
{
	struct pp_whitelist_field fields[PP_FLD_COUNT];
	int cnt, prio;
	bool should_delete;

	cnt = qos_tc_build_rule_fields(map, fields);
	if (cnt == 0) {
		pr_debug("%s: No valid fields to delete\n", __func__);
		return -EINVAL;
	}

	prio = qos_tc_get_queue_prio(dev, map->qid);
	if (prio == -1) {
		pr_debug("%s: Invalid prio: %d\n", __func__, prio);
		return -EINVAL;
	}

	/* Check if we should delete from PP based on reference count */
	should_delete = qos_tc_remove_rule_ref(grp, map, prio);
	if (!should_delete) {
		pr_debug("%s: Rule still in use, not deleting from PP\n",
			 __func__);
		return 0;
	}

	/* Last reference, delete from PP */
	pr_debug("%s: Last reference, deleting rule from PP\n", __func__);
	return dp_gpid_group_rule_del(0, grp_id, prio, fields, cnt);
}

int qos_tc_cpu_ingg_qos_delete(struct net_device *dev,
				  struct net_device *indev,
				  struct flower_cls_map *map)
{
	struct grp_info *grp;
	u32 indev_gpid;
	int i, ret;
	bool port_found = false;

	if (!indev) {
		pr_debug("%s: Invalid indev\n", __func__);
		return 0;
	}

	grp = qos_tc_get_group_by_cookie(map->tc_cookie);
	if (!grp) {
		pr_debug("%s: No group available with cookie %hhd (may not "
			 "exist if add failed early)\n", __func__, map->tc_cookie);
		return 0;
	}

	/* Check if port is actually in this group
	 * If not, it means add failed before port was added - nothing to do
	 */
	for (i = 0; i < ARRAY_SIZE(grp->iface_info); i++) {
		if (strcmp(grp->iface_info[i].ifname, indev->name) == 0) {
			port_found = true;
			break;
		}
	}
	if (!port_found) {
		/* No port to delete, but return success to allow
		 * normal cleanup of queue mappings.
		 */
		pr_debug("%s: Port %s not in group%hhd, nothing to delete "
			 "(add must have failed early)\n",
			 __func__, indev->name, map->tc_cookie);
		return 0;
	}

	/* Delete Rule (with reference counting) */
	ret = qos_tc_gpid_group_rule_delete(dev, map, grp->pp_grp_id, grp);
	if (ret < 0) {
		pr_debug("%s: Failed to delete rule from group %d\n",
			 __func__, grp->pp_grp_id);
		return -EINVAL;
	}

	/* Find and delete port from group */
	for (i = 0; i < ARRAY_SIZE(grp->iface_info); i++) {
		if (strcmp(grp->iface_info[i].ifname, indev->name) == 0) {
			indev_gpid = grp->iface_info[i].gpid;
			ret = dp_gpid_group_del_port(0, grp->pp_grp_id, indev_gpid);
			if (ret != 0) {
				pr_err("%s: failed to delete port: %d from "
				       "group%hhd\n", __func__, indev_gpid,
				       map->tc_cookie);
				return -EINVAL;
			}
			memset(&grp->iface_info[i], 0, sizeof(grp->iface_info[i]));
			break;
		}
	}

	/* Don't delete group if any ports are remains in the group */
	for (i = 0; i < ARRAY_SIZE(grp->iface_info); i++) {
		if (grp->iface_info[i].ifname[0] != '\0') {
			return 0;
		}
	}

	/* Delete the group if no ports present in the group */
	ret = dp_gpid_group_delete(0, grp->pp_grp_id);
	if (ret != 0) {
		pr_err("%s: failed to delete group%hhd\n", __func__, map->tc_cookie);
		return -EINVAL;
	}

	/* Clean up rules list */
	{
		struct rule_info *rule, *tmp;
		list_for_each_entry_safe(rule, tmp, &grp->rules_list, list) {
			list_del(&rule->list);
			kfree(rule);
		}
	}

	list_del(&grp->list);
	kfree(grp);

	return 0;
}

/* Helper functions for CPU port handling */

int qos_tc_validate_cpu_port_scheduler(const struct qos_tc_qdisc *sch,
				       bool is_wrr_type)
{
	if (is_wrr_type && (sch->data_flag & DP_SUBIF_CPU_HIGH_PRI)) {
		netdev_err(sch->dev,
			  "Creating WRR scheduler on high cpu port %s is not allowed\n",
			  sch->dev->name);
		return -EINVAL;
	}

	if (!is_wrr_type && (sch->data_flag & DP_SUBIF_CPU_LOW_PRI)) {
		netdev_err(sch->dev,
			  "Creating SP scheduler on low cpu port %s is not allowed\n",
			  sch->dev->name);
		return -EINVAL;
	}

	return 0;
}
