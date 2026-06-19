/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2026 MaxLinear, Inc.
 *
 *****************************************************************************/

/* Switch offload API stubs.
 *
 * When CONFIG_QOS_TC_SWITCH_OFFLOAD is enabled the real implementations
 * live in the switch/ subdirectory and the extern declarations below
 * are resolved at link time.
 *
 * When it is *not* enabled (e.g. on Topaz, which has no integrated
 * switch) every function is provided as a static inline no-op so that
 * the rest of the driver compiles and operates correctly for non-switch
 * offloads.  Offload requests return -EOPNOTSUPP; unoffload / cleanup /
 * debugfs helpers return 0 or do nothing.
 */

#ifndef _QOS_TC_SWITCH_STUBS_H_
#define _QOS_TC_SWITCH_STUBS_H_

#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <net/pkt_cls.h>
#include <net/qos_tc.h>

struct cfm_rule;

#if IS_ENABLED(CONFIG_QOS_TC_SWITCH_OFFLOAD)

/* --- ext_vlan ----------------------------------------------------------- */
int qos_tc_ext_vlan_add(struct net_device *dev, struct flow_cls_offload *f,
			bool ingress, struct net_device *bp_dev);
int qos_tc_ext_vlan_del(struct net_device *dev, void *vlan_storage,
			void *rule);
void qos_tc_ext_vlan_debugfs(struct seq_file *file, void *ctx);

/* --- vlan_filter -------------------------------------------------------- */
int qos_tc_vlan_filter_add(struct net_device *dev,
			   struct flow_cls_offload *f, bool ingress);
int qos_tc_vlan_filter_del(struct net_device *dev, void *vlan_storage,
			   void *rule);
void qos_tc_vlan_filter_storage_debugfs(struct seq_file *file, void *ctx);

/* --- qmap --------------------------------------------------------------- */
int qos_tc_map(struct net_device *dev, struct flow_cls_offload *f,
	       bool ingress, const struct qos_tc_params *tc_params);
int qos_tc_unmap(struct net_device *dev, void *list_node,
		 const struct qos_tc_params *tc_params);
int qos_tc_classid_unmap(u32 classid);
void qos_tc_class_list_debugfs(struct seq_file *file, void *ctx);

/* --- police ------------------------------------------------------------- */
int qos_tc_police_offload(struct net_device *dev,
			  struct flow_cls_offload *f, bool ingress);
int qos_tc_police_unoffload(struct net_device *dev,
			    void *meter_cfg_ptr, void *flags_ptr);
void qos_tc_policer_list_debugfs(struct seq_file *file, void *ctx);

/* --- pce ---------------------------------------------------------------- */
void qos_tc_pce_list_debugfs(struct seq_file *file, void *ctx);

/* --- mirred ------------------------------------------------------------- */
int qos_tc_mirred_offload(struct net_device *dev,
			  struct flow_cls_offload *f, unsigned long cookie);
int qos_tc_mirred_unoffload(struct net_device *dev,
			    struct flow_cls_offload *f, unsigned long cookie);

/* --- trap --------------------------------------------------------------- */
int qos_tc_trap_offload(struct net_device *dev,
			struct flow_cls_offload *f, unsigned long cookie);
int qos_tc_trap_unoffload(struct net_device *dev,
			  struct flow_cls_offload *f, unsigned long cookie);
void qos_tc_trap_storage_debugfs(struct seq_file *file, void *ctx);

/* --- ip_drop ------------------------------------------------------------ */
int qos_tc_ip_drop_offload(struct net_device *dev,
			   struct flow_cls_offload *f, unsigned long cookie);
int qos_tc_ip_drop_unoffload(struct net_device *dev,
			     struct flow_cls_offload *f, unsigned long cookie);

/* --- skbedit ------------------------------------------------------------ */
int qos_tc_skbedit_offload(struct net_device *dev,
			   struct flow_cls_offload *f, bool ingress);
int qos_tc_skbedit_unoffload(struct net_device *dev, void *skbedit,
			     void *rule);
int qos_tc_skbedit_action_check(struct flow_cls_offload *f, int *tc);
void qos_tc_skbedit_debugfs(struct seq_file *file, void *ctx);

/* --- extract_cfm -------------------------------------------------------- */
int qos_tc_cfm_offload(struct net_device *dev, struct flow_cls_offload *f,
		       bool ingress);
int qos_tc_cfm_unoffload(struct net_device *dev, struct flow_cls_offload *f,
			 unsigned long cookie, struct cfm_rule *rule,
			 unsigned long *p_cookie);
void qos_tc_cfm_debugfs(struct seq_file *file, void *ctx);

/* --- ds_tc_remap -------------------------------------------------------- */
int qos_tc_mappings_init(void);

#else /* !CONFIG_QOS_TC_SWITCH_OFFLOAD */

/* --- ext_vlan stubs ----------------------------------------------------- */
static inline int qos_tc_ext_vlan_add(struct net_device *dev,
				      struct flow_cls_offload *f,
				      bool ingress, struct net_device *bp_dev)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_ext_vlan_del(struct net_device *dev,
				      void *vlan_storage, void *rule)
{
	return 0;
}

static inline void qos_tc_ext_vlan_debugfs(struct seq_file *file, void *ctx)
{
}

/* --- vlan_filter stubs -------------------------------------------------- */
static inline int qos_tc_vlan_filter_add(struct net_device *dev,
					 struct flow_cls_offload *f,
					 bool ingress)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_vlan_filter_del(struct net_device *dev,
					 void *vlan_storage, void *rule)
{
	return 0;
}

static inline void qos_tc_vlan_filter_storage_debugfs(struct seq_file *file,
						      void *ctx)
{
}

/* --- qmap stubs --------------------------------------------------------- */
static inline int qos_tc_map(struct net_device *dev,
			     struct flow_cls_offload *f, bool ingress,
			     const struct qos_tc_params *tc_params)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_unmap(struct net_device *dev, void *list_node,
			       const struct qos_tc_params *tc_params)
{
	return 0;
}

static inline int qos_tc_classid_unmap(u32 classid)
{
	return 0;
}

static inline void qos_tc_class_list_debugfs(struct seq_file *file, void *ctx)
{
}

/* --- police stubs ------------------------------------------------------- */
static inline int qos_tc_police_offload(struct net_device *dev,
					struct flow_cls_offload *f,
					bool ingress)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_police_unoffload(struct net_device *dev,
					  void *meter_cfg_ptr,
					  void *flags_ptr)
{
	return 0;
}

static inline void qos_tc_policer_list_debugfs(struct seq_file *file,
					       void *ctx)
{
}

/* --- pce stubs ---------------------------------------------------------- */
static inline void qos_tc_pce_list_debugfs(struct seq_file *file, void *ctx)
{
}

/* --- mirred stubs ------------------------------------------------------- */
static inline int qos_tc_mirred_offload(struct net_device *dev,
					struct flow_cls_offload *f,
					unsigned long cookie)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_mirred_unoffload(struct net_device *dev,
					  struct flow_cls_offload *f,
					  unsigned long cookie)
{
	return 0;
}

/* --- trap stubs --------------------------------------------------------- */
static inline int qos_tc_trap_offload(struct net_device *dev,
				      struct flow_cls_offload *f,
				      unsigned long cookie)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_trap_unoffload(struct net_device *dev,
					struct flow_cls_offload *f,
					unsigned long cookie)
{
	return 0;
}

static inline void qos_tc_trap_storage_debugfs(struct seq_file *file,
					       void *ctx)
{
}

/* --- ip_drop stubs ------------------------------------------------------ */
static inline int qos_tc_ip_drop_offload(struct net_device *dev,
					 struct flow_cls_offload *f,
					 unsigned long cookie)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_ip_drop_unoffload(struct net_device *dev,
					   struct flow_cls_offload *f,
					   unsigned long cookie)
{
	return 0;
}

/* --- skbedit stubs ------------------------------------------------------ */
static inline int qos_tc_skbedit_offload(struct net_device *dev,
					 struct flow_cls_offload *f,
					 bool ingress)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_skbedit_unoffload(struct net_device *dev,
					   void *skbedit, void *rule)
{
	return 0;
}

static inline int qos_tc_skbedit_action_check(struct flow_cls_offload *f,
					      int *tc)
{
	return -EOPNOTSUPP;
}

static inline void qos_tc_skbedit_debugfs(struct seq_file *file, void *ctx)
{
}

/* --- extract_cfm stubs -------------------------------------------------- */
static inline int qos_tc_cfm_offload(struct net_device *dev,
				     struct flow_cls_offload *f, bool ingress)
{
	return -EOPNOTSUPP;
}

static inline int qos_tc_cfm_unoffload(struct net_device *dev,
				       struct flow_cls_offload *f,
				       unsigned long cookie,
				       struct cfm_rule *rule,
				       unsigned long *p_cookie)
{
	return 0;
}

static inline void qos_tc_cfm_debugfs(struct seq_file *file, void *ctx)
{
}

/* --- ds_tc_remap stubs -------------------------------------------------- */
static inline int qos_tc_mappings_init(void)
{
	return 0;
}

#endif /* CONFIG_QOS_TC_SWITCH_OFFLOAD */

#endif /* _QOS_TC_SWITCH_STUBS_H_ */
