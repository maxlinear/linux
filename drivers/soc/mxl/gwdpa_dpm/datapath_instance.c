// SPDX-License-Identifier: GPL-2.0
/*****************************************************************************
 * Copyright (c) 2024, MaxLinear, Inc.
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.

*******************************************************************************/

#include <linux/types.h>
#include <linux/soc/mxl/datapath_api.h>
#include <linux/soc/mxl/datapath_proc_api.h>
#include <linux/kallsyms.h>
#if IS_ENABLED(CONFIG_QOS_TC)
#include <linux/soc/mxl/qos_tc.h>
#endif
#include "datapath.h"
#include "datapath_instance.h"
#include "hal/datapath_misc.h"

#if IS_ENABLED(CONFIG_MXL_VPN)
#include <linux/soc/mxl/atapath_api_vpn.h>
#include <net/xfrm.h>
#endif
#if IS_ENABLED(CONFIG_QOS_TC) || IS_ENABLED(CONFIG_QOS_MGR)
#define DP_ENABLE_TC_OFFLOADS
#endif

/* disable optimization in debug mode: push */
DP_NO_OPTIMIZE_PUSH

int dp_cap_num;
struct dp_hw_cap hw_cap_list[DP_MAX_HW_CAP];

/*Module hash list */
struct hlist_head dp_mod_list[DP_MOD_HASH_SIZE];

static struct kmem_cache *cache_mod_list;

int (*qos_tc_setup_fn)(struct net_device *dev,
		       enum tc_setup_type type,
		       void *type_data,
		       int port_id,
		       int deq_idx);
EXPORT_SYMBOL(qos_tc_setup_fn);

char *dp_get_sym_name_by_addr(void *symaddr, char *symname,
		const char *fail_str)
{
	int ret;
	ret = sprint_symbol_no_offset(symname, (unsigned long)symaddr);
	if (ret == 0 || symname[0] == '\0' ||
		(symname[0] == '0' && symname[1] == 'x'))
		if (fail_str)
			dp_strlcpy(symname, fail_str, strlen(fail_str)+1);

	return symname;
}

int register_dp_hw_cap(struct dp_hw_cap *info, u32 flag)
{
	int i;

	if (!info) {
		pr_err("DPM: %s: NULL info\n", __func__);
		return -1;
	}
	for (i = 0; i < DP_MAX_HW_CAP; i++) {
		if (hw_cap_list[i].valid)
			continue;
		hw_cap_list[i].valid = 1;
		hw_cap_list[i].info = info->info;
		dp_cap_num++;
#if IS_ENABLED(CONFIG_DPM_DATAPATH_EXTRA_DEBUG)
		pr_err("DPM: Succeed to %s HAL[%d]: dp_cap_num=%d\n",
		       "Register",
		       i,
		       dp_cap_num);
#endif
		return 0;
	}
	pr_err("DPM: Failed to %s HAL\n",
	       "Register");
	return -1;
}

static struct dp_hw_cap *dp_get_hw_cap(void)
{
	int k;

	for (k = 0; k < DP_MAX_HW_CAP; k++) {
		if (!hw_cap_list[k].valid)
			continue;
		/*Verify some other hw version*/
		break;
	}

	if (k == DP_MAX_HW_CAP)
		return NULL;

	return &hw_cap_list[k];
}

/*return value:
 *succeed: return 0 with info->inst updated
 *fail: -1
 */
int dp_request_inst(struct dp_inst_info *info, u32 flag)
{
	int i, j;
	struct inst_property *dp_prop;
	struct dp_hw_cap *hwc;
	struct hal_priv *hal;

	if (!info)
		return -1;

	if (flag & DP_F_DEREGISTER) {
		/*do de-register */
		return 0;
	}
	/*register a dp instance */

	/*to check whether any such matched HW cap */
	hwc = dp_get_hw_cap();
	if (!hwc) {
		pr_err("DPM: %s fail to match cap\n",
		       __func__);
		return -1;
	}

	/* to find a free instance */
	for (i = 0; i < DP_MAX_INST; i++) {
		if (!dp_port_prop[i].valid)
			break;
	}
	if (i == DP_MAX_INST) {
		pr_err("DPM: %s fail for dp inst full already\n", __func__);
		return -1;
	}

	dp_prop = get_dp_port_prop(i);
#if 0 //GSW_ENABLE
	dp_prop->ops[0] = info->ops[0];
	dp_prop->ops[1] = info->ops[1];
	for (j = 0; j < DP_MAX_MAC_HANDLE; j++) {
		if (info->mac_ops[j])
			dp_prop->mac_ops[j] = info->mac_ops[j];
	}
#endif

	dp_prop->info = hwc->info;
	dp_prop->cqm_inst = info->cqm_inst;
	dp_prop->qos_inst = info->qos_inst;
	dp_prop->valid = 1;
#ifdef CONFIG_LTQ_DATAPATH_CPUFREQ
	dp_cpufreq_notify_init(i);
	DP_DEBUG(DP_DBG_FLAG_COC, "DP registered CPUFREQ notifier\n");
#endif
	/*Allocate dp_port_info table and subifs for this instance*/
	if (alloc_dp_port_subif_info(i)) {
		pr_err("DPM: alloc_dp_port_subif_info fail..\n");
		return DP_FAILURE;
	}
	/* TODO: VBOLLA: assing ring_index by iterating over dp_deq/enq/ret/req_ring_tbl once
	 * for all
	 */
	for (j = 0; j < DP_MAX_CQM_DEQ; j++) {
		get_dp_deqring_info(i, j)->ring_id = j;
		/*DEQ qid also init to -1*/
		get_dp_deqring_info(i, j)->qid = -1;
	}
	for (j = 0; j < DP_MAX_CQM_ENQ; j++)
		get_dp_enqring_info(i, j)->ring_id = j;
	for (j = 0; j < DP_MAX_CQM_REQ; j++)
		get_dp_reqring_info(i, j)->ring_id = j;
	for (j = 0; j < DP_MAX_CQM_RET; j++)
		get_dp_retring_info(i, j)->ring_id = j;

	if (dp_prop->info.dp_platform_set(i, 0) < 0) {
		dp_prop->valid = 0;
		pr_err("DPM: %s failed for inst=%d\n", __func__, i);
		return -1;
	}
	info->inst = i;
	dp_inst_num++;

	hal = dp_prop->priv_hal;
	/*Link hal->spl[x].user_data.mem_port to hold hal->spl[x].mem_port*/
	for (j = 0; j < DP_MAX_SPL_CONN; j++)
		hal->spl[j].user_data.mem_port = &hal->spl[j].mem_port;

	DP_DEBUG(DP_DBG_FLAG_INST,
		 "%s ok: inst=%d, dp_inst_num=%d\n", __func__, i, dp_inst_num);
	return 0;
}

#if defined(DP_ENABLE_TC_OFFLOADS)
int dp_ndo_setup_tc(struct net_device *dev,
		    enum tc_setup_type type,
		    void *type_data)
{
#if IS_ENABLED(CONFIG_QOS_TC)
	if (qos_tc_setup_fn)
		return qos_tc_setup_fn(dev, type, type_data, -1, -1);
#endif

	pr_info_once("%s is triggered\n", __func__);
	return 0;
}
#else
int dp_ndo_setup_tc(struct net_device *dev,
		     enum tc_setup_type type,
		     void *type_data)
{
	pr_info_once("%s is triggered\n", __func__);
	return 0;
}
#endif /* DP_ENABLE_TC_OFFLOADS */
EXPORT_SYMBOL(dp_ndo_setup_tc);

int dp_dev_update_xfrm(struct net_device *dev)
{
#if IS_ENABLED(CONFIG_MXL_VPN)
	dev->features |= NETIF_F_HW_ESP;
	dev->hw_enc_features |= NETIF_F_HW_ESP;
	dev->vlan_features |= NETIF_F_HW_ESP;
#endif
	return 0;
}
EXPORT_SYMBOL(dp_dev_update_xfrm);

int dp_xdo_dev_state_add(struct xfrm_state *x)
{
#if IS_ENABLED(CONFIG_MXL_VPN)
	struct mxl_vpn_ops *vpn;

	vpn = dp_get_vpn_ops(0);
	if (!vpn) {
		pr_err("Invalid vpn ops\n");
		return -1;
	}
	return vpn->add_xfrm_sa(x);
#endif
	return -1;
}
EXPORT_SYMBOL(dp_xdo_dev_state_add);

void dp_xdo_dev_state_delete(struct xfrm_state *x)
{
#if IS_ENABLED(CONFIG_MXL_VPN)
	struct mxl_vpn_ops *vpn;

	vpn = dp_get_vpn_ops(0);
	if (!vpn) {
		pr_err("Invalid vpn ops\n");
		return;
	}
	vpn->delete_xfrm_sa(x);
#endif
}
EXPORT_SYMBOL(dp_xdo_dev_state_delete);

bool dp_xdo_dev_offload_ok(struct sk_buff *skb, struct xfrm_state *x)
{
#if IS_ENABLED(CONFIG_MXL_VPN)
	struct mxl_vpn_ops *vpn;

	vpn = dp_get_vpn_ops(0);
	if (!vpn) {
		pr_err("Invalid vpn ops\n");
		return false;
	}
	return vpn->xfrm_offload_ok(skb, x);
#endif
	return false;
}
EXPORT_SYMBOL(dp_xdo_dev_offload_ok);

int dp_dev_update_toe(struct net_device *dev)
{
/*TODO: VBOLLA CONFIG_LGM_TOE -> CONFIG_MXL_TOE, CONFIG_SOC_LGM -> CONFIG_SOC_TPZ*/
#if IS_ENABLED(CONFIG_LGM_TOE) || IS_ENABLED(CONFIG_SOC_LGM)
	struct lro_ops *ops;

	ops = dp_get_lro_ops();
	if (!ops)
		return -1;

	netif_set_gso_max_size(dev, ops->get_gso_max_size(ops->toe));
	ops->cfg_netdev_feature(ops->toe, dev, true);
#endif
	//dev->features |= NETIF_F_HW_CSUM;
	dev->hw_features |= NETIF_F_HW_CSUM;
	return 0;
}
EXPORT_SYMBOL(dp_dev_update_toe);

int dp_dev_update_tc(struct net_device *dev)
{
#if defined(DP_ENABLE_TC_OFFLOADS)
	dev->features |= NETIF_F_HW_TC;
#endif
	return 0;
}
EXPORT_SYMBOL(dp_dev_update_tc);

u32 dp_mod_hash(struct module *owner, u16 ep)
{
	return hash_ptr(owner, DP_MOD_HASH_BIT_LENGTH);
}

struct dp_mod *dp_mod_lookup(struct hlist_head *head, struct module *owner,
			     u16 ep, u32 flag)
{
	struct dp_mod *item;

	hlist_for_each_entry(item, head, hlist) {
		if (item->mod == owner && item->ep == ep)
			return item;
	}
	return NULL;
}

/* tuple: owner + ep
 * act: inst
 */
int dp_inst_insert_mod(struct module *owner, u16 ep, u32 inst, u32 flag)
{
	struct dp_mod *dp_mod;
	u8 new_f = 0;
	u32 idx;

	if (!owner) {
		pr_err("DPM: %s owner: NULL?\n", __func__);
		return -1;
	}
	idx = dp_mod_hash(owner, ep);
	DP_DEBUG(DP_DBG_FLAG_INST, "dp_mod_list idx=%u\n", idx);
	dp_mod = dp_mod_lookup(&dp_mod_list[idx], owner, ep, flag);
	if (!dp_mod) { /*alloc new */
		dp_mod = kmem_cache_zalloc(cache_mod_list, GFP_ATOMIC);
		if (dp_mod) {
			dp_mod->mod = owner;
			dp_mod->ep = ep;
			dp_mod->inst = inst;
			new_f = 1;
		}
	}
	if (!dp_mod)
		return -1;
	if (new_f)
		hlist_add_head(&dp_mod->hlist, &dp_mod_list[idx]);
	DP_DEBUG(DP_DBG_FLAG_INST, "owner: %s\n", owner->name);
	return 0;
}

int dp_inst_del_mod(struct module *owner, u16 ep, u32 flag)
{
	struct dp_mod *dp_mod;
	u32 idx;

	if (!owner) {
		pr_err("DPM: %s owner: NULL?\n", __func__);
		return -1;
	}
	idx = dp_mod_hash(owner, ep);
	dp_mod = dp_mod_lookup(&dp_mod_list[idx], owner, ep, flag);
	if (!dp_mod) {
		pr_err("DPM: Failed to dp_mod_lookup: %s\n",
		       owner->name);
		return -1;
	}
	hlist_del(&dp_mod->hlist);
	kmem_cache_free(cache_mod_list, dp_mod);

	DP_DEBUG(DP_DBG_FLAG_INST, "ok: %s:\n", owner->name);
	return 0;
}

int dp_get_inst_via_module(struct module *owner,  u16 ep, u32 flag)
{
	struct dp_mod *dp_mod;
	u32 idx;

	if (!owner) {
		pr_err("DPM: owner NULL?\n");
		return -1;
	}
	idx = dp_mod_hash(owner, ep);
	dp_mod = dp_mod_lookup(&dp_mod_list[idx], owner, ep, flag);
	if (!dp_mod) {
		pr_err("DPM: Failed to dp_mod_lookup: %s\n",
		       owner->name);
		return -1;
	}

	return dp_mod->inst;
}

static void dump_cap(struct seq_file *s, struct dp_cap *cap)
{
	if (!s)
		return;
	dp_sprintf(s, "	HW TX checksum offloading: %s\n",
		   cap->tx_hw_chksum ? "Yes" : "No");
	dp_sprintf(s, "	HW RX checksum verification: %s\n",
		   cap->rx_hw_chksum ? "Yes" : "No");
	dp_sprintf(s, "	HW TSO: %s\n",
		   cap->hw_tso ? "Yes" : "No");
	dp_sprintf(s, "	HW GSO: %s\n",
		   cap->hw_gso ? "Yes" : "No");
	dp_sprintf(s, "	QOS Engine: %s\n",
		   cap->qos_eng_name);
	dp_sprintf(s, "	Pkt Engine: %s\n",
		   cap->pkt_eng_name);
	dp_sprintf(s, "	max_num_queues: %d\n",
		   cap->max_num_queues);
	dp_sprintf(s, "	max_num_scheds: %d\n",
		   cap->max_num_scheds);
	dp_sprintf(s, "	max_num_deq_rings: %d\n",
		   cap->max_num_deq_rings);
	dp_sprintf(s, "	max_num_qos_ports: %d\n",
		   cap->max_num_qos_ports);
	dp_sprintf(s, "	max_num_dp_ports: %d\n",
		   cap->max_num_dp_ports);
	dp_sprintf(s, "	max_num_subif_per_port: %d\n",
		   cap->max_num_subif_per_port);
	dp_sprintf(s, "	max_num_subif: %d\n",
		   cap->max_num_subif);
}

static u32 mod_hash_index;
static struct dp_mod *dp_mod_proc;
int proc_inst_mod_dump(struct seq_file *s, int pos)
{
	if (!capable(CAP_SYS_PACCT))
		return -1;
	while (!dp_mod_proc) {
		mod_hash_index++;
		if (mod_hash_index == DP_MOD_HASH_SIZE)
			return -1;

		dp_mod_proc =
			hlist_entry_safe((&dp_mod_list[mod_hash_index])->first,
					 struct dp_mod, hlist);
	}
	dp_sprintf(s, "Hash=%u pos=%d owner=%s(@0x%px) ep=%d inst=%d\n",
		   mod_hash_index,
		   pos,
		   dp_mod_proc->mod->name,
		   dp_mod_proc->mod,
		   dp_mod_proc->ep,
		   dp_mod_proc->inst);

	dp_mod_proc = hlist_entry_safe((dp_mod_proc)->hlist.next,
				       struct dp_mod, hlist);
	pos++;
	return pos;
}

int proc_inst_dump(struct seq_file *s, int pos)
{
	struct dp_cap *cap;

	if (!capable(CAP_SYS_PACCT))
		return -1;
	if (!dp_port_prop[pos].valid)
		goto NEXT;
	dp_sprintf(s, "Inst[%d]\n",
		   pos);
	/*dump_cap(s, &dp_port_prop[pos].info.cap);*/
	cap = dp_kzalloc(sizeof(*cap), GFP_ATOMIC);
	if (!cap)
		return DP_FAILURE;
	cap->inst = pos;
	dp_get_cap(cap, 0);
	dump_cap(s, cap);
	kfree(cap);
NEXT:
	pos++;
	if (pos == DP_MAX_INST)
		return -1;
	return pos;
}

int proc_inst_hal_dump(struct seq_file *s, int pos)
{
	if (!capable(CAP_SYS_PACCT))
		return -1;
	if (!hw_cap_list[pos].valid) {
		if (pos == 0) {
		/* For spl_conn automation test case purpose
		 * before DPM instance is ready
		 */
		}
		goto NEXT;
	}

	dp_sprintf(s, "HAL[%d] dp_cap_num=%d\n",
		   pos,
		   dp_cap_num);
	dump_cap(s, &hw_cap_list[pos].info.cap);

NEXT:
	pos++;
	if (pos == DP_MAX_HW_CAP)
		return -1;
	return pos;
}

int proc_inst_mod_start(void  *param)
{
	mod_hash_index = 0;
	dp_mod_proc = hlist_entry_safe((&dp_mod_list[mod_hash_index])->first,
				       struct dp_mod, hlist);
	return 0;
}

int dp_inst_init(u32 flag)
{
	dp_cap_num = 0;
	dp_memset(hw_cap_list, 0, sizeof(hw_cap_list));

	cache_mod_list = kmem_cache_create("dp_mod_list", sizeof(struct dp_mod),
					   0, SLAB_HWCACHE_ALIGN, NULL);
	if (!cache_mod_list)
		return -ENOMEM;
	return 0;
}

void dp_inst_free(void)
{
	kmem_cache_destroy(cache_mod_list);
}

/* disable optimization in debug mode: pop */
DP_NO_OPTIMIZE_POP
