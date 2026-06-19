/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2020 - 2026 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/
#ifndef _QOS_TC_COMPAT_H_
#define _QOS_TC_COMPAT_H_

#include <linux/version.h>

/* This header handles two orthogonal compatibility concerns:
 *
 * 1. KERNEL VERSION differences (bstats API):
 *    - Kernel < 5.18:  gnet_stats_basic_packed with plain __u64 fields
 *    - Kernel >= 5.18: gnet_stats_basic_sync with u64_stats_t fields
 *    Uses LINUX_VERSION_CODE checks.
 *
 * 2. PLATFORM differences :
 *    - dp_get_netif_subifid() API and dp_subif_t field names
 *    - DP flags (DP_SUBIF_REINSERT, DP_CQE_LU_MODE2, QOS_EVENT_MAP_*)
 *    - QOS_TC_NOTIFY_PRIO implementation
 *    Uses CONFIG_QOS_TC_SWITCH_OFFLOAD checks.
 *
 * These are kept separate to allow any combination of kernel version
 * and platform configuration.
 */

/* =========================================================================
 * KERNEL VERSION COMPAT: gnet_stats_basic abstraction
 * =========================================================================
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
/* Kernel 5.18+: gnet_stats_basic_sync with u64_stats_t fields */
#define qos_tc_bstats_t struct gnet_stats_basic_sync

#define qos_tc_bstats_init(b)			gnet_stats_basic_sync_init(b)
#define qos_tc_bstats_set_packets(b, v)		u64_stats_set(&(b)->packets, (v))
#define qos_tc_bstats_set_bytes(b, v)		u64_stats_set(&(b)->bytes, (v))
#define qos_tc_bstats_get_packets(b)		u64_stats_read(&(b)->packets)
#define qos_tc_bstats_get_bytes(b)		u64_stats_read(&(b)->bytes)
#else
/* Kernel < 5.18: gnet_stats_basic_packed with plain __u64 fields */
#define qos_tc_bstats_t struct gnet_stats_basic_packed

#define qos_tc_bstats_init(b)			memset((b), 0, sizeof(*(b)))
#define qos_tc_bstats_set_packets(b, v)		((b)->packets = (v))
#define qos_tc_bstats_set_bytes(b, v)		((b)->bytes = (v))
#define qos_tc_bstats_get_packets(b)		((b)->packets)
#define qos_tc_bstats_get_bytes(b)		((b)->bytes)
#endif

/* =========================================================================
 * PLATFORM COMPAT: dp_get_netif_subifid() and dp_subif_t field access
 * =========================================================================
 */

/* Compatibility layer for dp_get_netif_subifid() and dp_subif_t field
 * access between switch offload and non-switch offload platforms.
 *
 * Switch offload (CONFIG_QOS_TC_SWITCH_OFFLOAD):
 *   int dp_get_netif_subifid(dev, skb, data, mac, subif, flags)
 *   Fills caller-provided dp_subif_t, returns DP_SUCCESS/DP_FAILURE.
 *
 * Non-switch offload (e.g., Topaz):
 *   dp_subif_t *dp_get_netif_subifid(dev, skb, data, mac, flags)
 *   Returns dynamically allocated dp_subif_t (must dp_free_netif_subifid).
 *
 * dp_subif_t field names differ between platforms:
 *   Switch offload          Non-switch offload
 *   -----------             ------------------
 *   data_flag               data_flag_ops
 *   lookup_mode             cqe_lu_mode
 *   alloc_flag              alloc_flags
 *   subif_common.num_q      num_qid
 *   subif_common.def_qlist  def_qid_list
 */
#if IS_ENABLED(CONFIG_QOS_TC_SWITCH_OFFLOAD)

static inline int qos_tc_get_netif_subifid(struct net_device *dev,
					   dp_subif_t *subif)
{
	return dp_get_netif_subifid(dev, NULL, NULL, NULL, subif, 0);
}

#define QOS_TC_SUBIF_DATA_FLAGS(_subif)		((_subif)->data_flag)
#define QOS_TC_SUBIF_LOOKUP_MODE(_subif)	((_subif)->lookup_mode)
#define QOS_TC_SUBIF_ALLOC_FLAGS(_subif)	((_subif)->alloc_flag)
#define QOS_TC_SUBIF_NUM_Q(_subif)		((_subif)->subif_common.num_q)
#define QOS_TC_SUBIF_DEF_Q(_subif, _idx)	((_subif)->subif_common.def_qlist[_idx])

#else /* !CONFIG_QOS_TC_SWITCH_OFFLOAD */

static inline int qos_tc_get_netif_subifid(struct net_device *dev,
					   dp_subif_t *subif)
{
	dp_subif_t *tmp;

	tmp = dp_get_netif_subifid(dev, NULL, NULL, NULL, 0);
	if (!tmp)
		return DP_FAILURE;

	memcpy(subif, tmp, sizeof(*subif));
	dp_free_netif_subifid(tmp);
	return DP_SUCCESS;
}

#define QOS_TC_SUBIF_DATA_FLAGS(_subif)		((_subif)->data_flag_ops)
#define QOS_TC_SUBIF_LOOKUP_MODE(_subif)	((_subif)->cqe_lu_mode)
#define QOS_TC_SUBIF_ALLOC_FLAGS(_subif)	((_subif)->alloc_flags)
#define QOS_TC_SUBIF_NUM_Q(_subif)		((_subif)->num_qid)
#define QOS_TC_SUBIF_DEF_Q(_subif, _idx) \
	(((_idx) < (_subif)->num_qid) ? (_subif)->def_qid_list[_idx] : (_subif)->def_qid)

#ifndef DP_SUBIF_REINSERT
#define DP_SUBIF_REINSERT 0
#endif
#ifndef DP_CQE_LU_MODE2
#define DP_CQE_LU_MODE2 0xff
#endif
#if IS_ENABLED(CONFIG_QOS_NOTIFY)
#ifndef QOS_EVENT_MAP_ADD
#define QOS_EVENT_MAP_ADD QOS_EVENT_Q_ADD
#endif
#ifndef QOS_EVENT_MAP_DELETE
#define QOS_EVENT_MAP_DELETE QOS_EVENT_Q_DELETE
#endif
#endif /* CONFIG_QOS_NOTIFY */

#endif /* IS_ENABLED(CONFIG_QOS_TC_SWITCH_OFFLOAD) */

/* =========================================================================
 * PLATFORM COMPAT: QOS_TC_NOTIFY_PRIO (switch offload vs no switch offload)
 * =========================================================================
 */

/* TODO: Investigate unifying QOS_TC_NOTIFY_PRIO implementations.
 *
 * Currently two different implementations exist:
 * - With switch offload: returns sch->offset + tc_cookie (dynamic
 *   scheduler offset in hardware queue map plus traffic class cookie)
 * - No switch offload: returns qid->p_w (pre-computed priority weight)
 *
 * These differ because:
 * - With switch offload: sch->offset tracks dynamic positioning in the switch
 *   hardware queue map (q_map bitmap), requiring on-the-fly computation
 * - Without switch offload: no hardware queue map exists, p_w is pre-stored
 *
 * Unification would require ensuring p_w is always populated with the
 * equivalent of (offset + tc_cookie) at queue creation time on switch
 * platforms, or abstracting the priority source behind a common accessor.
 */
#if IS_ENABLED(CONFIG_QOS_NOTIFY)
#if IS_ENABLED(CONFIG_QOS_TC_SWITCH_OFFLOAD)
#define QOS_TC_NOTIFY_PRIO(qid, sch, tc_cookie) \
	((void)(qid), (sch)->offset + (tc_cookie))
#else
#define QOS_TC_NOTIFY_PRIO(qid, sch, tc_cookie) \
	((void)(sch), (void)(tc_cookie), (qid)->p_w)
#endif
#endif /* CONFIG_QOS_NOTIFY */

#endif /* _QOS_TC_COMPAT_H_ */
