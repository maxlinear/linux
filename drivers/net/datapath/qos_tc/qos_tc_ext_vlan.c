// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 *
 * Copyright (c) 2020 - 2023 MaxLinear, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 *****************************************************************************/
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/netdevice.h>
#include <linux/version.h>
#include <net/pkt_cls.h>
#include <net/flow_dissector.h>
#include <net/tc_act/tc_vlan.h>
#include <net/tc_act/tc_gact.h>
#include <net/tc_act/tc_mirred.h>
#include <net/datapath_api.h>
#include <net/datapath_api_vlan.h>
#include <uapi/linux/tc_act/tc_vlan.h>
#include "qos_tc_compat.h"
#include "qos_tc_flower.h"
#include "qos_tc_vlan_prepare.h"
#include "qos_tc_vlan_storage.h"
#include "qos_tc_ext_vlan.h"
#include "qos_tc_trace.h"
#include "qos_tc_parser.h"

static LIST_HEAD(tc_ext_vlan_storage);

static bool has_cookie(const struct dp_act_vlan *act)
{
	return act->merge_key.hi != 0 && act->merge_key.lo != 0;
}

static bool dp_merge_key_eq(const struct dp_merge_key *a,
			    const struct dp_merge_key *b)
{
	return a->hi == b->hi && a->lo == b->lo;
}

static bool is_dscp(struct dp_act_vlan *act)
{
	unsigned int i;

	if (act->push_n == 0)
		return false;

	for (i = 0; i < act->push_n; i++)
		if (act->prio[i] != DERIVE_FROM_DSCP)
			return false;

	return true;
}

static void dp_pattern_vlan_from_cookie_us(struct dp_pattern_vlan *outer,
					   struct dp_pattern_vlan *inner,
					   struct dp_act_vlan *act)
{
	if (outer && inner) {
		if (FILTER_OUTER_TPID(act->merge_key.hi) == FILTER_TPID_ANY)
			outer->tpid = DP_VLAN_PATTERN_NOT_CARE;

		if (FILTER_INNER_TPID(act->merge_key.hi) == FILTER_TPID_ANY)
			inner->tpid = DP_VLAN_PATTERN_NOT_CARE;

		if (FILTER_ETHERTYPE(act->merge_key.hi) == FILTER_ETHERTYPE_ANY)
			inner->proto = DP_VLAN_PATTERN_NOT_CARE;

		return;
	}

	if (outer) {
		if (FILTER_INNER_TPID(act->merge_key.hi) == FILTER_TPID_ANY)
			outer->tpid = DP_VLAN_PATTERN_NOT_CARE;
		if (FILTER_ETHERTYPE(act->merge_key.hi) == FILTER_ETHERTYPE_ANY)
			outer->proto = DP_VLAN_PATTERN_NOT_CARE;
	}
}

static void dp_pattern_vlan_from_cookie_ds(struct dp_pattern_vlan *outer,
					   struct dp_pattern_vlan *inner,
					   struct dp_act_vlan *act)
{
	if (outer && inner) {
		if (FILTER_DS_OUTER_TPID(act->merge_key.hi) == FILTER_TPID_ANY)
			outer->tpid = DP_VLAN_PATTERN_NOT_CARE;

		if (FILTER_DS_INNER_TPID(act->merge_key.hi) == FILTER_TPID_ANY)
			inner->tpid = DP_VLAN_PATTERN_NOT_CARE;

		if (FILTER_ETHERTYPE(act->merge_key.hi) == FILTER_ETHERTYPE_ANY)
			inner->proto = DP_VLAN_PATTERN_NOT_CARE;

		return;
	}

	if (outer) {
		if (FILTER_DS_INNER_TPID(act->merge_key.hi) == FILTER_TPID_ANY)
			outer->tpid = DP_VLAN_PATTERN_NOT_CARE;
		if (FILTER_ETHERTYPE(act->merge_key.hi) == FILTER_ETHERTYPE_ANY)
			outer->proto = DP_VLAN_PATTERN_NOT_CARE;
	}
}

static void dp_pattern_vlan_from_cookie(struct dp_pattern_vlan *outer,
					struct dp_pattern_vlan *inner,
					struct dp_act_vlan *act)
{
	if (TREATMENT_DS_EN(act->merge_key.lo))
		dp_pattern_vlan_from_cookie_ds(outer, inner, act);
	else
		dp_pattern_vlan_from_cookie_us(outer, inner, act);
}

static int dp_pattern_vlan_eq(const struct dp_pattern_vlan *a,
			      const struct dp_pattern_vlan *b)
{
	return a->prio == b->prio &&
	       a->vid == b->vid &&
	       a->tpid == b->tpid &&
	       a->dei == b->dei &&
	       a->proto == b->proto;
}

static int merge(int old_def,
		 struct dp_pattern_vlan *old_outer,
		 struct dp_pattern_vlan *old_inner,
		 struct dp_act_vlan *old,
		 int new_def,
		 struct dp_pattern_vlan *new_outer,
		 struct dp_pattern_vlan *new_inner,
		 struct dp_act_vlan *new)
{
	int i;

	if (old_def != new_def)
		return -EINVAL;

	if (old_outer && new_outer)
		if (!dp_pattern_vlan_eq(old_outer, new_outer))
			return -EINVAL;

	if (old_inner && new_inner)
		if (!dp_pattern_vlan_eq(old_inner, new_inner))
			return -EINVAL;

	if (!dp_merge_key_eq(&old->merge_key, &new->merge_key))
		return -EINVAL;

	if (old->act != new->act)
		return -EINVAL;

	if (old->push_n != new->push_n)
		return -EINVAL;

	if (old->pop_n != new->pop_n)
		return -EINVAL;

	for (i = 0; i < old->push_n; i++) {
		if (old->prio[i] != new->prio[i])
			return -EINVAL;
		if (old->tpid[i] != new->tpid[i])
			return -EINVAL;
		if (old->vid[i] != new->vid[i])
			return -EINVAL;
	}

	if (is_dscp(new) && is_dscp(old)) {
		for (i = 0; i < DP_DSCP_MAP; i++) {
			if (new->dscp_pcp_map_valid[i]) {
				old->dscp_pcp_map[i] = new->dscp_pcp_map[i];
				old->dscp_pcp_map_valid[i] = true;
			}
		}
	}

	return 0;
}

static int untagged_add(struct net_device *dev,
			struct flow_cls_offload *f,
			struct qos_tc_vlan_storage_node *node,
			struct net_device *bp_dev)
{
	struct dp_vlan0 *rule, *p;
	int err = 0;

	rule = kzalloc(sizeof(*rule), GFP_KERNEL);
	if (!rule)
		return -ENOMEM;

	err = qos_tc_vlan_untagged_prepare(dev, f, bp_dev, rule);
	if (err) {
		kfree(rule);
		return err;
	}

	if (has_cookie(&rule->act))
		dp_pattern_vlan_from_cookie(&rule->outer, NULL, &rule->act);

	/* Attempt to merge new rule into existing one */
	list_for_each_entry(p, &node->rules, list) {
		if (!merge(p->def, &p->outer, NULL, &p->act, rule->def,
			   &rule->outer, NULL, &rule->act)) {
			kfree(rule);
			return 0;
		}
	}

	err = qos_tc_flower_storage_add(dev, f->cookie, TC_TYPE_EXT_VLAN,
					node, rule);
	if (err < 0) {
		kfree(rule);
		return err;
	}

	list_add_tail(&rule->list, &node->rules);
	list_sort(NULL, &node->rules, qos_tc_cookie_cmp0);

	node->rule_cnt++;

	return 0;
}

static int single_tagged_add(struct net_device *dev,
			     struct flow_cls_offload *f,
			     struct qos_tc_vlan_storage_node *node,
			     struct net_device *bp_dev)
{
	struct dp_vlan1 *rule, *p;
	int err;

	rule = kzalloc(sizeof(*rule), GFP_KERNEL);
	if (!rule)
		return -ENOMEM;

	err = qos_tc_vlan_single_tagged_prepare(dev, f, bp_dev, rule);
	if (err) {
		kfree(rule);
		return err;
	}

	if (has_cookie(&rule->act))
		dp_pattern_vlan_from_cookie(&rule->outer, NULL, &rule->act);

	/* Attempt to merge new rule into existing one */
	list_for_each_entry(p, &node->rules, list) {
		if (!merge(p->def, &p->outer, NULL, &p->act, rule->def,
			   &rule->outer, NULL, &rule->act)) {
			kfree(rule);
			return 0;
		}
	}

	err = qos_tc_flower_storage_add(dev, f->cookie, TC_TYPE_EXT_VLAN,
					node, rule);
	if (err < 0) {
		kfree(rule);
		return err;
	}

	list_add_tail(&rule->list, &node->rules);
	list_sort(NULL, &node->rules, qos_tc_cookie_cmp1);

	node->rule_cnt++;

	return 0;
}

static int double_tagged_add(struct net_device *dev,
			     struct flow_cls_offload *f,
			     struct qos_tc_vlan_storage_node *node,
			     struct net_device *bp_dev)
{
	struct dp_vlan2 *rule, *p;
	int err;

	rule = kzalloc(sizeof(*rule), GFP_KERNEL);
	if (!rule)
		return -ENOMEM;

	err = qos_tc_vlan_double_tagged_prepare(dev, f, bp_dev, rule);
	if (err) {
		kfree(rule);
		return err;
	}

	if (has_cookie(&rule->act))
		dp_pattern_vlan_from_cookie(&rule->outer, &rule->inner,
					    &rule->act);

	/* Attempt to merge new rule into existing one */
	list_for_each_entry(p, &node->rules, list) {
		if (!merge(p->def, &p->outer, &p->inner, &p->act, rule->def,
			   &rule->outer, &rule->inner, &rule->act)) {
			kfree(rule);
			return 0;
		}
	}

	err = qos_tc_flower_storage_add(dev, f->cookie, TC_TYPE_EXT_VLAN,
					node, rule);
	if (err < 0) {
		kfree(rule);
		return err;
	}

	list_add_tail(&rule->list, &node->rules);
	list_sort(NULL, &node->rules, qos_tc_cookie_cmp2);

	node->rule_cnt++;

	return 0;
}

static void dp_vlan_list_update(struct net_device *dev,
				struct dp_tc_vlan *dp_vlan,
				struct qos_tc_vlan_storage_node *node)
{
	switch (node->tag) {
	case TC_VLAN_UNTAGGED:
		dp_vlan->vlan0_head = &node->rules;
		dp_vlan->n_vlan0 = node->rule_cnt;
		break;
	case TC_VLAN_SINGLE_TAGGED:
		dp_vlan->vlan1_head = &node->rules;
		dp_vlan->n_vlan1 = node->rule_cnt;
		break;
	case TC_VLAN_DOUBLE_TAGGED:
		dp_vlan->vlan2_head = &node->rules;
		dp_vlan->n_vlan2 = node->rule_cnt;
		break;
	default:
		break;
	}
}

static int dp_update(struct net_device *dev,
		     struct dp_tc_vlan *dp_vlan)
{
	struct qos_tc_vlan_storage_node *p;
	bool ingress = dp_vlan->dir == DP_DIR_INGRESS;
	bool mcc = dp_vlan->mcast_flag & DP_MULTICAST_SESSION;

	list_for_each_entry(p, &tc_ext_vlan_storage, list) {
		if (dev == p->dev &&
		    ingress == p->ingress &&
		    mcc == p->mcc) {
			dp_vlan_list_update(dev, dp_vlan, p);
		}
	}

	if (dp_vlan_set(dp_vlan, 0) != DP_SUCCESS) {
		netdev_err(dev, "dp_vlan_set failed!\n");
		return -EPERM;
	}

	return 0;
}

static int qos_tc_get_flow_type(const struct qos_tc_vlan_storage_node *p)
{
	return p->mcc ? DP_MULTICAST_SESSION : DP_NON_MULTICAST_SESSION;
}

static int qos_tc_get_active_vlan_flows(const struct dp_tc_vlan *dp_vlan,
					const struct list_head *head)
{
	struct qos_tc_vlan_storage_node *p = NULL;
	bool ingress = dp_vlan->dir == DP_DIR_INGRESS;
	int flags = 0;

	list_for_each_entry(p, head, list) {
		if (p->dev == dp_vlan->dev && p->ingress == ingress)
			flags |= qos_tc_get_flow_type(p);

		if (flags == (DP_NON_MULTICAST_SESSION | DP_MULTICAST_SESSION))
			break;
	}

	return flags;
}

static int qos_tc_set_vlan_ops_flag(const struct qos_tc_vlan_storage_node *n,
				    const struct dp_tc_vlan *dp_vlan,
				    const struct list_head *head)
{
	int flags = qos_tc_get_active_vlan_flows(dp_vlan, head);

	if (n->mcc)
		return DP_MULTICAST_SESSION;

	if (!n->mcc && (flags & DP_MULTICAST_SESSION))
		return DP_NON_MULTICAST_SESSION;

	return 0;
}

static int qos_tc_ext_vlan_update(struct net_device *dev,
				  struct dp_tc_vlan *dp_vlan,
				  const struct list_head *head)
{
	int flags = qos_tc_get_active_vlan_flows(dp_vlan, head);
	int ret;

	ret = dp_update(dev, dp_vlan);
	if (ret)
		return ret;

	if (flags && !(flags & DP_MULTICAST_SESSION)) {
		dp_vlan->mcast_flag = 0;

		ret = dp_update(dev, dp_vlan);
		if (ret)
			return ret;
	}

	return 0;
}

int qos_tc_ext_vlan_add(struct net_device *dev,
			struct flow_cls_offload *f,
			bool ingress,
			struct net_device *bp_dev)
{
	struct dp_tc_vlan dp_vlan = {0};
	struct qos_tc_vlan_storage_node *node;
	enum tc_flower_vlan_tag tag;
	bool mcc = qos_tc_parse_is_mcc(dev, f);
	int err;

	netdev_dbg(dev, "Extended VLAN add start\n");

	tag = qos_tc_vlan_tag_get(dev, f);
	if (tag == TC_VLAN_UNKNOWN)
		return -EIO;

	trace_ev_add_enter(dev, NULL, -1, -1, ingress);

	node = qos_tc_vlan_storage_get(dev, ingress, bp_dev, tag, mcc,
				       &tc_ext_vlan_storage);
	if (!node) {
		err = qos_tc_vlan_storage_crt(dev, ingress, tag, mcc, &node);
		if (err)
			return err;
		list_add(&node->list, &tc_ext_vlan_storage);
	}

	qos_tc_dp_vlan_prepare(dev, &dp_vlan, ingress);

	dp_vlan.mcast_flag = qos_tc_set_vlan_ops_flag(node, &dp_vlan,
						      &tc_ext_vlan_storage);

	switch (tag) {
	case TC_VLAN_UNTAGGED:
		err = untagged_add(dev, f, node, bp_dev);
		if (err)
			goto err;
		break;
	case TC_VLAN_SINGLE_TAGGED:
		err = single_tagged_add(dev, f, node, bp_dev);
		if (err)
			goto err;
		break;
	case TC_VLAN_DOUBLE_TAGGED:
		err = double_tagged_add(dev, f, node, bp_dev);
		if (err)
			goto err;
		break;
	default:
		err = -EIO;
		goto err;
	}

	netdev_dbg(dev, "%s: node:%p, dir:%d, tags:%d, rules:%d m_f:%#x\n",
		   __func__, node, node->ingress, node->tag,
		   node->rule_cnt, dp_vlan.mcast_flag);

	err = dp_update(dev, &dp_vlan);

	trace_ev_add_exit(dev, node, node->tag, node->rule_cnt, ingress);

	return err;
err:
	qos_tc_vlan_storage_del(dev, node);
	return err;
}

static void qos_tc_ext_vlan_bp_dev_put(void *rule, enum tc_flower_vlan_tag tag)
{
#if (KERNEL_VERSION(4, 19, 0) > LINUX_VERSION_CODE)
	struct dp_vlan0 *rule0;
	struct dp_vlan1 *rule1;
	struct dp_vlan2 *rule2;

	switch (tag) {
	case TC_VLAN_UNTAGGED:
		rule0 = (struct dp_vlan0 *)rule;
		if (rule0->act.ract.bp_dev)
			dev_put(rule0->act.ract.bp_dev);
		break;
	case TC_VLAN_SINGLE_TAGGED:
		rule1 = (struct dp_vlan1 *)rule;
		if (rule1->act.ract.bp_dev)
			dev_put(rule1->act.ract.bp_dev);
		break;
	case TC_VLAN_DOUBLE_TAGGED:
		rule2 = (struct dp_vlan2 *)rule;
		if (rule2->act.ract.bp_dev)
			dev_put(rule2->act.ract.bp_dev);
		break;
	default:
		break;
	}
#endif
}

#define bp_ract_present(p, node)					\
	({								\
		typeof (p) _p = (p);					\
			list_for_each_entry(_p, &node->rules, list)	\
			if (_p->act.ract.act)				\
				return true;				\
			return false; 					\
	})

bool qos_tc_bp_ract_present(struct qos_tc_vlan_storage_node *node, void *rule)
{
	switch (node->tag) {
	case TC_VLAN_UNTAGGED:
		bp_ract_present((struct dp_vlan0 *)rule, node);
		break;
	case TC_VLAN_SINGLE_TAGGED:
		bp_ract_present((struct dp_vlan1 *)rule, node);
		break;
	case TC_VLAN_DOUBLE_TAGGED:
		bp_ract_present((struct dp_vlan2 *)rule, node);
		break;
	default:
		return false;
	}
}

int qos_tc_ext_vlan_del(struct net_device *dev, void *vlan_storage, void *rule)
{
	struct dp_tc_vlan dp_vlan = {0};
	struct qos_tc_vlan_storage_node *node;
	struct dp_vlan0 *rule0;
	struct dp_vlan1 *rule1;
	struct dp_vlan2 *rule2;
	bool ingress;
	int ret = 0;

	netdev_dbg(dev, "%s: node:%p, rule:%p\n", __func__, vlan_storage, rule);

	if (!vlan_storage || !rule)
		return -EIO;

	node = vlan_storage;
	ingress = node->ingress;

	trace_ev_del_enter(dev, node, node->tag, node->rule_cnt, ingress);

	qos_tc_dp_vlan_prepare(dev, &dp_vlan, ingress);

	switch (node->tag) {
	case TC_VLAN_UNTAGGED:
		rule0 = rule;
		qos_tc_ext_vlan_bp_dev_put(rule0, node->tag);
		list_del(&rule0->list);
		break;
	case TC_VLAN_SINGLE_TAGGED:
		rule1 = rule;
		qos_tc_ext_vlan_bp_dev_put(rule1, node->tag);
		list_del(&rule1->list);
		break;
	case TC_VLAN_DOUBLE_TAGGED:
		rule2 = rule;
		qos_tc_ext_vlan_bp_dev_put(rule2, node->tag);
		list_del(&rule2->list);
		break;
	default:
		return -EIO;
	}

	kfree(rule);
	node->rule_cnt--;

	dp_vlan.mcast_flag = qos_tc_set_vlan_ops_flag(node, &dp_vlan,
						      &tc_ext_vlan_storage);

	netdev_dbg(dev, "%s: node:%p, dir:%d, tags:%d, rules:%d m_f:%#x\n",
		   __func__, node, node->ingress, node->tag,
		   node->rule_cnt, dp_vlan.mcast_flag);

	qos_tc_vlan_storage_del(dev, node);

	ret = qos_tc_ext_vlan_update(dev, &dp_vlan, &tc_ext_vlan_storage);

	trace_ev_del_exit(dev, node, node->tag, node->rule_cnt, ingress);

	return ret;
}

struct list_head *qos_tc_get_ext_vlan_storage(void)
{
	return &tc_ext_vlan_storage;
}

static void print_rule(struct seq_file *file, struct dp_pattern_vlan *outer,
		       struct dp_pattern_vlan *inner, struct dp_act_vlan *act)
{
	unsigned int i = 0;

	seq_printf(file, "%016llx filter", act->sort_key);

	if (outer) {
		seq_printf(file, " outer prio %d tpid 0x%x vid %d dei %d proto %d",
			   outer->prio, outer->tpid, outer->vid, outer->dei,
			   outer->proto);
	}

	if (inner) {
		seq_printf(file, " inner prio %d tpid 0x%x vid %d dei %d proto %d",
			   inner->prio, inner->tpid, inner->vid, inner->dei,
			   inner->proto);
	}

	seq_puts(file, "\n");

	seq_printf(file, "treatment act %d pop_n %d push_n %d", act->act,
		   act->pop_n, act->push_n);
	for (i = 0; i < act->push_n; ++i) {
		seq_printf(file, " tpid[%d] 0x%x", i, act->tpid[i]);
		seq_printf(file, " vid[%d] %d", i, act->vid[i]);
		seq_printf(file, " dei[%d] %d", i, act->dei[i]);
		seq_printf(file, " prio[%d] %d", i, act->prio[i]);
	}

	seq_puts(file, "\n");
	if (is_dscp(act)) {
		seq_puts(file, "dscp");
		for (i = 0; i < ARRAY_SIZE(act->dscp_pcp_map); ++i) {
			if (act->dscp_pcp_map_valid[i])
				seq_printf(file, " %2x", act->dscp_pcp_map[i]);
			else
				seq_puts(file, " ..");
		}
		seq_puts(file, "\n");
	}
	seq_puts(file, "\n");
}

static void print_node(struct seq_file *file,
		       struct qos_tc_vlan_storage_node *p)
{
	seq_printf(file, "%s", p->dev->name);
	if (p->ingress)
		seq_puts(file, " ingress");
	else
		seq_puts(file, " egress");

	switch (p->tag) {
	case TC_VLAN_UNTAGGED:
		seq_puts(file, " untagged");
		break;
	case TC_VLAN_SINGLE_TAGGED:
		seq_puts(file, " single");
		break;
	case TC_VLAN_DOUBLE_TAGGED:
		seq_puts(file, " double");
		break;
	default:
		seq_puts(file, " unknown");
		break;
	}

	if (p->mcc)
		seq_puts(file, " mcc");

	seq_puts(file, "\n");

	if (p->tag == TC_VLAN_UNTAGGED) {
		struct dp_vlan0 *i;

		list_for_each_entry(i, &p->rules, list)
			print_rule(file, &i->outer, NULL, &i->act);
	}
	if (p->tag == TC_VLAN_SINGLE_TAGGED) {
		struct dp_vlan1 *i;

		list_for_each_entry(i, &p->rules, list)
			print_rule(file, &i->outer, NULL, &i->act);
	}
	if (p->tag == TC_VLAN_DOUBLE_TAGGED) {
		struct dp_vlan2 *i;

		list_for_each_entry(i, &p->rules, list)
			print_rule(file, &i->outer, &i->inner, &i->act);
	}
}

void qos_tc_ext_vlan_debugfs(struct seq_file *file, void *ctx)
{
	struct qos_tc_vlan_storage_node *p;
	unsigned int i = 0;

	static const struct {
		bool ingress;
		enum tc_flower_vlan_tag tag;
	} display_order[] = {
		/* ingress */
		{true, TC_VLAN_UNTAGGED},
		{true, TC_VLAN_SINGLE_TAGGED},
		{true, TC_VLAN_DOUBLE_TAGGED},
		/* egress */
		{false, TC_VLAN_UNTAGGED},
		{false, TC_VLAN_SINGLE_TAGGED},
		{false, TC_VLAN_DOUBLE_TAGGED},
	};

	if (!file) {
		WARN(1, "file is NULL\n");
		return;
	}

	rtnl_lock();
	for (i = 0; i < ARRAY_SIZE(display_order); ++i)
		list_for_each_entry(p, &tc_ext_vlan_storage, list)
			if (p->ingress == display_order[i].ingress &&
			    p->tag == display_order[i].tag)
				print_node(file, p);
	rtnl_unlock();
}
