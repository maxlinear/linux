/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2020 - 2026 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/
#ifndef _QOS_TC_QMAP_OPS_H_
#define _QOS_TC_QMAP_OPS_H_

/* Platform stubs for qos_tc_qos.c functions that require switch queue map
 * hardware (CONFIG_QOS_TC_SWITCH_OFFLOAD). When switch offload is disabled,
 * static inline no-ops are provided so qos_tc_qos.c compiles unchanged.
 *
 * This header must only be included from qos_tc_qos.c, after qos_tc_qos.h
 * and the datapath API headers, so that all struct definitions are visible.
 */

#include "qos_tc_qos.h"

#if !IS_ENABLED(CONFIG_QOS_TC_SWITCH_OFFLOAD)

static inline bool
qos_tc_is_netdev_reinsert_port(struct net_device *dev)
{
	return false;
}

static inline int
qos_tc_set_qmap(int qid, struct qos_tc_qdisc *sch, int subif,
		int tc_cookie, bool en, const struct qos_tc_params *tc_params)
{
	return 0;
}

static inline int
qos_tc_set_qmap_extraction(struct qos_tc_q_data *qid, int port, bool en)
{
	return 0;
}

static inline int
qos_tc_set_qmap_cpu_from_indev(struct qos_tc_q_data *qid, int port,
			       int tc_cookie, bool en)
{
	return 0;
}

static inline void q_parms_enable(struct qos_tc_qdisc *sch,
				  struct dp_queue_conf *q)
{
}

#else /* IS_ENABLED(CONFIG_QOS_TC_SWITCH_OFFLOAD) */

void q_parms_enable(struct qos_tc_qdisc *sch, struct dp_queue_conf *q);
bool qos_tc_is_netdev_reinsert_port(struct net_device *dev);
int qos_tc_set_qmap(int qid, struct qos_tc_qdisc *sch, int subif,
		    int tc_cookie, bool en,
		    const struct qos_tc_params *tc_params);
int qos_tc_set_qmap_extraction(struct qos_tc_q_data *qid, int port, bool en);
int qos_tc_set_qmap_cpu_from_indev(struct qos_tc_q_data *qid, int port,
				   int tc_cookie, bool en);

#endif /* !IS_ENABLED(CONFIG_QOS_TC_SWITCH_OFFLOAD) */

#endif /* _QOS_TC_QMAP_OPS_H_ */
