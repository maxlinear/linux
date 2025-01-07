/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2022 MaxLinear, Inc.
 *
 *****************************************************************************/
#ifndef _QOS_TC_COMPAT_
#define _QOS_TC_COMPAT_

#include <linux/version.h>
#include <net/pkt_cls.h>

#if (KERNEL_VERSION(5, 3, 0) >= LINUX_VERSION_CODE)
#define flow_cls_offload tc_cls_flower_offload

#define FLOW_CLS_REPLACE TC_CLSFLOWER_REPLACE
#define FLOW_CLS_DESTROY TC_CLSFLOWER_DESTROY
#define FLOW_CLS_STATS TC_CLSFLOWER_STATS

#endif

static inline struct flow_dissector *
qos_tc_get_dissector(struct flow_cls_offload *f)
{
#if (KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE)
	struct flow_rule *rule = flow_cls_offload_flow_rule(f);

	return rule->match.dissector;
#else
	return f->dissector;
#endif
}

static inline void *
qos_tc_get_mask(struct flow_cls_offload *f)
{
#if (KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE)
	struct flow_rule *rule = flow_cls_offload_flow_rule(f);

	return rule->match.mask;
#else
	return f->mask;
#endif
}

static inline void *
qos_tc_get_key(struct flow_cls_offload *f)
{
#if (KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE)
	struct flow_rule *rule = flow_cls_offload_flow_rule(f);

	return rule->match.key;
#else
	return f->key;
#endif
}

#endif
