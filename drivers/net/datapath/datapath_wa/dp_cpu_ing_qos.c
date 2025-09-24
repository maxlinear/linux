// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 * Copyright (c) 2025 MaxLinear, Inc.
 *
 * TODO: This file will be removed once the permanent solution for CPU ingress
 * QoS is implemented.
 *
 *****************************************************************************/
#include <linux/pp_api.h>
#include <linux/cleanup.h>
#include <net/datapath_api.h>
#include <net/dp_cpu_ing_qos.h>

#define MAX_CPU_COUNT			4
#define LGM_INGRESS_MAX_PRIO	4

/*
 * The following CPU port information is fixed across the platform and values
 * will remain unchanged after reboot.
 * - cqm_deq_ports of CPU high ports: 0, 2, 4, 6 (from cpu0 to cpu3)
 * - Corresponding GPIDs: 16, 18, 20, 22
 * - Queue IDs initialized to -1 (invalid) and will be updated after queue creation.
 */
static struct cpu_port_info {
	const int port;		/* High CPU port number */
	const int gpid;		/* GPID of high CPU port */
	int queue_id;	/* Queue ID for the high CPU port */
} cpu_ports_info[MAX_CPU_COUNT] = {
	{0, 16, -1},
	{2, 18, -1},
	{4, 20, -1},
	{6, 22, -1}
};

/* Flag to track if queues were successfully created and hostif updated */
static bool queues_update_done = false;

/**
 * Removes high-priority queues from each high CPU port.
 */
static int dp_omci_cleanup_allocated_hp_queues(struct net_device *dev)
{
	int i, ret;
	struct dp_node_link node = {0};
	struct dp_node_alloc anode = {0};

	if (!queues_update_done)
		return 0;

	for (i = 0; i < MAX_CPU_COUNT; i++) {
		if (!cpu_online(i))
			continue;

		node.node_type = DP_NODE_QUEUE;
		node.node_id.q_id = cpu_ports_info[i].queue_id;

		ret = dp_node_unlink(&node, 0);
		if (ret == DP_FAILURE)
			netdev_err(dev, "qid %d unlink failed\n", node.node_id.q_id);

		anode.type = DP_NODE_QUEUE;
		anode.id.q_id = cpu_ports_info[i].queue_id;

		ret = dp_node_free(&anode, DP_NODE_AUTO_FREE_RES);
		if (ret == DP_FAILURE)
			netdev_err(dev, "qid %d free failed\n", anode.id.q_id);

		cpu_ports_info[i].queue_id = -1;
	}

	queues_update_done = false;
	return 0;
}

/**
 * Creates high-priority queues for each high CPU port and updates hostif configuration.
 */
static int dp_omci_create_hp_queue_and_hostif_update(struct net_device *dev)
{
	int i, ret;
	struct dp_node_alloc anode = {0};
	struct dp_node_link node = {0};
	struct dp_hif_datapath *dp __free(kfree) = kzalloc(sizeof(*dp) * LGM_INGRESS_MAX_PRIO, GFP_KERNEL);
	dp_subif_t *subif __free(kfree) = kzalloc(sizeof(*subif), GFP_KERNEL);

	if (!dp || !subif) {
		netdev_err(dev, "%s: failed to allocate memory for dp or subif\n", __func__);
		return -ENOMEM;
	}

	ret = dp_get_netif_subifid(dev, NULL, NULL, 0, subif, 0);
	if (ret == DP_FAILURE) {
		netdev_err(dev, "%s: dp_get_netif_subifid failed\n", __func__);
		return -EINVAL;
	}

	/* Create one queue for each high CPU port and link it at highest priority (SP0) */
	for (i = 0; i < MAX_CPU_COUNT; i++) {
		if (!cpu_online(i))
			continue;

		anode.inst = 0;
		anode.dp_port = 0;
		anode.type = DP_NODE_QUEUE;
		anode.id.q_id = DP_NODE_AUTO_ID;

		ret = dp_node_alloc(&anode, 0);
		if (ret == DP_FAILURE) {
			netdev_err(dev, "Failed to allocate queue for cqm_deq_port=%d\n", cpu_ports_info[i].port);
			return -ENOMEM;
		}
		cpu_ports_info[i].queue_id = anode.id.q_id;

		node.arbi = ARBITRATION_WSP;
		node.prio_wfq = 0;
		node.node_type = DP_NODE_QUEUE;
		node.node_id.q_id = anode.id.q_id;
		node.p_node_type = DP_NODE_PORT;
		node.cqm_deq_port.cqm_deq_port = cpu_ports_info[i].port;
		node.p_node_id.cqm_deq_port = cpu_ports_info[i].port;

		ret = dp_node_link_add(&node, 0);
		if (ret == DP_FAILURE) {
			netdev_err(dev, "Failed to link queue to cqm_deq_port=%d\n", cpu_ports_info[i].port);

			ret = dp_node_free(&anode, DP_NODE_AUTO_FREE_RES);
			if (ret == DP_FAILURE)
				netdev_err(dev, "qid %d free failed\n", anode.id.q_id);

			cpu_ports_info[i].queue_id = -1;
			return -ENODEV;
		}
	}

	/* Update hostif data for OMCI traffic (classified to TC3) */
	dp[3].color = PP_COLOR_GREEN;
	for (i = 0; i < MAX_CPU_COUNT; i++) {
		if (!cpu_online(i))
			continue;

		dp[3].eg[i].q_id = cpu_ports_info[i].queue_id;
		dp[3].eg[i].cpu_gpid = cpu_ports_info[i].gpid;
	}

	ret = dp_hostif_update(0, subif->port_id, 0, dp);
	if (ret == DP_FAILURE) {
		netdev_err(dev, "dp_hostif_update failed: dpid=%d\n", subif->port_id);

		/* Unlink and free all HP queues from all high CPU ports */
		dp_omci_cleanup_allocated_hp_queues(dev);

		return -EINVAL;
	}

	queues_update_done = true;
	return 0;
}

/**
 * Updates hostif based on the specified action.
 */
int dp_omci_update_hostif(struct net_device *dev, int action)
{
	switch (action) {
	case OMCI_HOSTIF_ADD:
		return dp_omci_create_hp_queue_and_hostif_update(dev);

	case OMCI_HOSTIF_DEL:
		return dp_omci_cleanup_allocated_hp_queues(dev);

	default:
		netdev_err(dev, "Invalid action for omci_update_hostif: %d\n", action);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(dp_omci_update_hostif);
