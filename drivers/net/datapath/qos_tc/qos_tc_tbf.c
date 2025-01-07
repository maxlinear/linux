// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 *
 * Copyright (c) 2020 - 2022 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/
#include <net/pkt_cls.h>
#include <linux/version.h>
#include "qos_tc_compat.h"
#include "qos_tc_qos.h"
#include "qos_tc_trace.h"

static int qos_tc_tbf_remove(struct net_device *dev, u32 handle, u32 parent)
{
	struct qos_tc_qdisc *qdisc = NULL;
	struct qos_tc_q_data *qid = NULL;
	struct qos_tc_port *port = NULL;
	int ret;

	port = qos_tc_port_get(dev);
	if (!port) {
		netdev_err(dev, "tc-tbf port get failed\n");
		return -ENODEV;
	}

	qdisc = qos_tc_qdisc_find(port, parent);
	if (!qdisc) {
		netdev_err(dev, "tc-tbf qdisc get failed\n");
		return -ENODEV;
	}

	qid = qos_tc_qdata_qid_get(dev, qdisc, parent);
	if (!qid) {
		ret = qos_tc_get_queue_by_handle(dev, parent, &qid);
		if (ret < 0 || !qid) {
			netdev_err(dev, "%s: handle not found err %d\n",
				   __func__, ret);
			return -EINVAL;
		}
	}

	ret = qos_tc_shaper_remove(qdisc, qid);
	if (ret < 0)
		return ret;

	radix_tree_delete(&port->qdiscs, TC_H_MAJ(handle));

	return 0;
}

static int qos_tc_tbf_replace(struct net_device *dev,
			      struct tc_tbf_qopt_offload *opt)
{
	struct qos_tc_qdisc *qdisc = NULL;
	struct qos_tc_q_data *qid = NULL;
	struct qos_tc_port *port = NULL;
	int ret;

	port = qos_tc_port_get(dev);
	if (!port) {
		netdev_err(dev, "tc-tbf port get failed\n");
		return -ENODEV;
	}

	qdisc = qos_tc_qdisc_find(port, opt->parent);
	if (!qdisc) {
		netdev_err(dev, "tc-tbf qdisc get failed\n");
		return -ENODEV;
	}

	qid = qos_tc_qdata_qid_get(dev, qdisc, opt->parent);
	if (!qid) {
		ret = qos_tc_get_queue_by_handle(dev, opt->parent, &qid);
		if (ret < 0 || !qid) {
			netdev_err(dev, "%s: handle not found err %d\n",
				   __func__, ret);
			return -EINVAL;
		}
	}

	ret = qos_tc_shaper_add(qdisc, qid, &opt->replace_params);
	if (ret < 0)
		return ret;

	ret = qos_tc_qdata_add(dev, qid, opt->handle, opt->parent,
			       QOS_TC_QDATA_TBF, qos_tc_tbf_remove);
	if (ret < 0)
		return ret;

	ret = radix_tree_insert(&port->qdiscs, TC_H_MAJ(opt->handle), qdisc);
	if (ret < 0) {
		netdev_err(dev, "%s: qdisc insertion to radix tree failed: %d\n",
			   __func__, ret);
		return ret;
	}

	return 0;
}

static int qos_tc_tbf_destroy(struct net_device *dev,
			      struct tc_tbf_qopt_offload *opt)
{
	struct qos_tc_qdisc *qdisc = NULL;
	struct qos_tc_q_data *qid = NULL;
	struct qos_tc_port *port = NULL;
	int ret;

	port = qos_tc_port_get(dev);
	if (!port) {
		/* Linux deletes some qdiscs from root to the leaf node
		 * which may cause that the tbf offload is already removed
		 * by its parent i.e. deleted by qos_tc_qdisc_tree_del.
		 */
		netdev_dbg(dev, "tc-tbf port get failed\n");
		return -ENODEV;
	}

	qdisc = qos_tc_qdisc_find(port, opt->parent);
	if (!qdisc) {
		netdev_err(dev, "tc-tbf qdisc get failed\n");
		return -ENODEV;
	}

	qid = qos_tc_qdata_qid_get(dev, qdisc, opt->parent);
	if (!qid) {
		ret = qos_tc_get_queue_by_handle(dev, opt->parent, &qid);
		if (ret < 0 || !qid) {
			netdev_err(dev, "%s: handle not found err %d\n",
				   __func__, ret);
			return -EINVAL;
		}
	}

	return qos_tc_qdata_remove(dev, qid, opt->handle, opt->parent);
}

int qos_tc_tbf_offload(struct net_device *dev,
		       void *type_data)
{
	int err = 0;
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	struct tc_to_netdev *tc_to_netdev = type_data;
	struct tc_tbf_qopt_offload *opt = tc_to_netdev->sch_tbf;
#else
	struct tc_tbf_qopt_offload *opt = type_data;
#endif

	trace_qos_tc_tbf_enter(dev, opt);

	switch (opt->command) {
	case TC_TBF_REPLACE:
		err = qos_tc_tbf_replace(dev, opt);
		if (err < 0) {
			netdev_err(dev, "tc-tbf replace failed\n");
			return err;
		}
		break;
	case TC_TBF_DESTROY:
		err = qos_tc_tbf_destroy(dev, opt);
		if (err < 0) {
			/* Expected to fail if root was previously removed */
			netdev_dbg(dev, "tc-tbf destroy failed\n");
			return err;
		}
		break;
	case TC_TBF_STATS:
		return -EOPNOTSUPP;
	default:
		break;
	}

	trace_qos_tc_tbf_exit(dev, opt);

	return 0;
}
