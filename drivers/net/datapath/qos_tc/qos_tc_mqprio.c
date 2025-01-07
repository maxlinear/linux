// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 *
 * Copyright (c) 2021 - 2023 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/
#include <net/pkt_cls.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <net/netlink.h>
#include <net/pkt_sched.h>
#include <net/sch_generic.h>
#include <net/datapath_api.h>
#include <net/datapath_api_qos.h>
#include <net/qos_tc.h>
#include <linux/version.h>
#include "qos_tc_compat.h"
#include "qos_tc_qos.h"

/* create qdisc tree */

static int qos_tc_sched_update(struct net_device *dev,
			       struct qos_tc_port *port,
			       u32 handle,
			       const struct qos_tc_params *tc_params)
{
	int ret;
	struct qos_tc_qdisc *sch = &port->root_qdisc;

	if (sch->use_cnt)
		return 0;

	sch->type = QOS_TC_QDISC_PRIO;
	sch->dev = dev;
	sch->handle = handle;
	/* mqprio can be only root */
	sch->parent = TC_H_ROOT;
	sch->use_cnt = 1;

	ret = qos_tc_fill_port_data(sch, tc_params);
	if (ret < 0) {
		netdev_err(dev, "failed getting port hw config %d\n", ret);
		return ret;
	}

	/* add to radix tree here */
	ret = radix_tree_insert(&port->qdiscs, TC_H_MAJ(handle), sch);
	if (ret) {
		netdev_err(dev, "qdisc insertion to radix tree failed: %d\n",
			   ret);
		return ret;
	}

	return qos_tc_add_sched(sch, 0, tc_params);
}

static int qos_tc_queues_update(struct net_device *dev,
				struct qos_tc_qdisc *sch,
				u32 handle,
				struct tc_mqprio_qopt *parms,
				const struct qos_tc_params *tc_params)
{
	u8 *priomap = parms->prio_tc_map;
	u8 bands = parms->num_tc;
	int ret, i;

	if (!sch->use_cnt || handle != sch->handle)
		return -EINVAL;

	if (bands == sch->prio.bands)
		/* TODO: nothing to do? */
		return 0;

	if (bands < sch->prio.bands) {
		/* TODO delete queue here */
		return -EINVAL;
	}

	sch->prio.bands = bands;
	memcpy(sch->prio.priomap, priomap,
	       sizeof(sch->prio.priomap));

	for (i = 0; i < bands; i++) {
		ret = qos_tc_queue_add(sch, QOS_TC_QDISC_PRIO, priomap[i], i,
				tc_params);
		if (ret < 0)
			netdev_err(dev, "queue add failed\n");
	}

	return 0;
}

static int qos_tc_mqprio_replace(struct net_device *dev,
				 u32 handle,
				 struct tc_mqprio_qopt *opt,
				 const struct qos_tc_params *tc_params)
{
	int ret = 0;
	struct qos_tc_port *port;
	bool newp = false;

	port = qos_tc_port_get(dev);
	if (!port) {
		port = qos_tc_port_alloc(dev);
		if (!port) {
			netdev_err(dev, "tc-mqprio port alloc failed\n");
			return -ENOMEM;
		}
		newp = true;
	}

	port->root_qdisc.port = tc_params->port_id;
	port->root_qdisc.deq_idx = tc_params->deq_idx;
	port->root_qdisc.dev = dev;
	ret = qos_tc_get_port_info(&port->root_qdisc, tc_params);
	if (ret < 0)
		goto err_free_port;

	netdev_dbg(dev, "Config sched\n");
	ret = qos_tc_sched_update(dev, port, handle, tc_params);
	if (ret < 0) {
		netdev_err(dev, "tc-mqpio sched config failed\n");
		goto err_free_port;
	}

	netdev_dbg(dev, "Config queues\n");
	ret = qos_tc_queues_update(dev, &port->root_qdisc, handle, opt, tc_params);
	if (ret < 0) {
		netdev_err(dev, "tc-mqprio queues config failed\n");
		goto err_free_qdisc;
	}

	return 0;

err_free_qdisc:
	ret = qos_tc_sched_del(&port->root_qdisc, tc_params);
	if (ret < 0)
		netdev_err(dev, "%s: sched del failed\n", __func__);
err_free_port:
	if (newp)
		qos_tc_port_delete(port);
	return ret;
}

static int qos_tc_mqprio_destroy(struct net_device *dev,
				 u32 handle,
				 struct tc_mqprio_qopt *opt,
				 const struct qos_tc_params *tc_params)
{
	struct qos_tc_port *port = NULL;
	struct qos_tc_qdisc *sch = NULL;

	port = qos_tc_port_get(dev);
	if (!port)
		return -ENODEV;

	sch = &port->root_qdisc;

	if (handle != sch->handle)
		return -EINVAL;

	qos_tc_qdisc_tree_del(port, sch, tc_params);

	return 0;
}

int qos_tc_mqprio_offload(struct net_device *dev,
			  u32 handle,
			  void *type_data,
			  const struct qos_tc_params *tc_params)
{
	int err = 0;
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	struct tc_to_netdev *tc_to_netdev = type_data;
	struct tc_mqprio_qopt *opt = tc_to_netdev->mqprio;
#else
	struct tc_mqprio_qopt_offload *qopt_offload = type_data;
	struct tc_mqprio_qopt *opt = &qopt_offload->qopt;
#endif

	ASSERT_RTNL();
	netdev_dbg(dev, "MQPRIO: offload starting\n");
	if (opt->num_tc > QOS_TC_MAX_Q) {
		netdev_err(dev, "tc mqprio offload - out of range bands\n");
		return -EINVAL;
	}

	if (opt->num_tc) {
		err = qos_tc_mqprio_replace(dev, handle, opt, tc_params);
		if (err < 0) {
			netdev_err(dev, "tc-mqprio replace failed\n");
			return err;
		}
	} else {
		err = qos_tc_mqprio_destroy(dev, handle, opt, tc_params);
		if (err < 0) {
			netdev_err(dev, "tc-mqprio destroy failed\n");
			return err;
		}
	}

	return 0;
}
