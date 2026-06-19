// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 *
 * Copyright (c) 2020 - 2026 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/
#include <linux/netdevice.h>
#include <net/datapath_api.h>
#include <net/datapath_api_qos.h>
#include "../qos_tc_qos.h"
#include <net/qos_tc.h>

/* These implementations require switch queue map hardware. When
 * CONFIG_QOS_TC_SWITCH_OFFLOAD is disabled (Makefile guard), static inline
 * stubs in qos_tc_qmap_ops.h are used instead.
 */

bool qos_tc_is_netdev_reinsert_port(struct net_device *dev)
{
	dp_subif_t *subif __free(kfree) = NULL;
	int ret;

	subif = kzalloc(sizeof(*subif), GFP_KERNEL);
	if (!subif) {
		netdev_err(dev, "%s: failed to allocate subif\n", __func__);
		return false;
	}
	ret = qos_tc_get_netif_subifid(dev, subif);
	if (ret == DP_FAILURE) {
		/* negative return value is no error for non
		 * reinsertion port
		 */
		return false;
	}
	netdev_dbg(dev, "%s: returned %d\n", __func__,
		   QOS_TC_SUBIF_DATA_FLAGS(subif) & DP_SUBIF_REINSERT ? 1 : 0);

	return !!(QOS_TC_SUBIF_DATA_FLAGS(subif) & DP_SUBIF_REINSERT);
}

int qos_tc_set_qmap(int qid, struct qos_tc_qdisc *sch, int subif, int tc_cookie,
		    bool en, const struct qos_tc_params *tc_params)
{
	struct dp_queue_map_set qmap_set = { 0 };
	int ret;

	if (WARN(qid < 0, "change invalid queue qid (%i) for subif: %i, en: %i",
		 qid, subif, en))
		return -EINVAL;

	qmap_set.inst = 0;
	qmap_set.mask.flowid = 1;

	/* If tc cookie is not set, assume class is same as subif */
	if (tc_cookie == QOS_TC_COOKIE_EMPTY)
		qmap_set.map.class = subif;
	else
		qmap_set.map.class = tc_cookie;

	if (tc_params && (tc_params->flags & QOS_TC_Q_MAP)) {
		qmap_set.q_id = en ? qid : tc_params->def_q;
		qmap_set.map.dp_port = tc_params->qmap_port;
		qmap_set.map.mpe1 = 1;
		qmap_set.map.mpe2 = 0;
		qmap_set.mask.enc = 1;
		qmap_set.mask.dec = 1;
		qmap_set.mask.subif = 1;
	} else {
		qmap_set.q_id = en ? qid : 0;
		qmap_set.map.dp_port = sch->port;
		qmap_set.map.subif = subif;

		if (!qos_tc_is_cpu_port(sch->port)) {
			/* Do not set the flags below for the CPU port otherwise the
			 * lookup table entry for the re-insert queue is overwritten.
			 * The re-inserted packets carry mpe1 = 0 and
			 * mpe = 0 in the DMA descriptor and they have to be kept.
			 * Not used on URX, only relevant on PRX. URX uses dedicated
			 * logical port.
			 */
			qmap_set.mask.mpe1 = 1;
			qmap_set.mask.mpe2 = 1;
		}

		/* egflag should be set only for egress ports,
		 * and skipped for non-egress ports like CPU and VUNI.
		 */
		if (!qos_tc_is_cpu_port(sch->port) &&
		    !qos_tc_is_vuni_dev(sch->dev)) {
			qmap_set.map.egflag = 1;
			qmap_set.mask.egflag = 0;
		}

		if (qos_tc_is_netdev_reinsert_port(sch->dev)) {
			/* subif to be ignored for reinsertion port */
			qmap_set.mask.subif = 1;
			/* enc is relevant for PRX only */
			qmap_set.map.enc = 1;
		}
	}

	ret = dp_queue_map_set(&qmap_set, 0);
	if (ret == DP_FAILURE) {
		pr_err("%s: queue map set failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

int qos_tc_set_qmap_extraction(struct qos_tc_q_data *qid, int port, bool en)
{
	int ret;
	struct dp_queue_map_set qmap_set = {
		.inst = 0,
		.q_id = en ? qid->qid : 0,
		.map = {
			.dp_port = port,
			.mpe1 = 0,
			.mpe2 = 1,
		},
		.mask = {
			.subif = 1,
			.class = 1,
			.flowid = 1,
		}
	};

	ret = dp_queue_map_set(&qmap_set, 0);
	if (ret == DP_FAILURE) {
		pr_err("%s: queue map set failed\n", __func__);
		return -EINVAL;
	}

	if (!en)
		atomic_dec(&qid->ref_cnt);
	else
		atomic_inc(&qid->ref_cnt);

	return 0;
}

int qos_tc_set_qmap_cpu_from_indev(struct qos_tc_q_data *qid, int port,
				   int tc_cookie, bool en)
{
	int ret;
	struct dp_queue_map_set qmap_set = {
		.inst = 0,
		.q_id = en ? qid->qid : 0,
		.map = {
			.dp_port = port,
			.mpe1 = 0,
			.class = tc_cookie,
		},
		.mask = {
			.mpe2 = 1,
			.subif = 1,
			.flowid = 1,
		}
	};

	ret = dp_queue_map_set(&qmap_set, 0);
	if (ret == DP_FAILURE) {
		pr_err("%s: queue map set failed\n", __func__);
		return -EINVAL;
	}

	if (!en)
		atomic_dec(&qid->ref_cnt);
	else
		atomic_inc(&qid->ref_cnt);

	return 0;
}

void q_parms_enable(struct qos_tc_qdisc *sch, struct dp_queue_conf *q)
{
	struct dp_qos_q_parms p = { 0 };
	int ret;

	ret = dp_qos_get_q_global_parms(sch->inst, sch->port, sch->alloc_flag,
					0, &p);
	if (!ret) {
		/* Apply global DPM queue parameters here. The parameters are
		 * currently set over dts.
		 */
		netdev_dbg(sch->dev, "codel for qid: %u enabled\n", q->q_id);
		q->codel = p.codel_en;
	}
}
