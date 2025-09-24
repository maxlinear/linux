// SPDX-License-Identifier: GPL-2.0
/*****************************************************************************
 * Copyright (c) 2024, MaxLinear, Inc.
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.

*******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/debugfs.h>
#include <linux/kgdb.h>
#include <linux/soc/mxl/datapath_api.h>
#include "datapath.h"
#include "datapath_tx.h"
#include "datapath_rx.h"
#include "datapath_instance.h"
#include "datapath_trace.h"
#include "datapath_ver.h"

/* disable optimization in debug mode: push */
DP_NO_OPTIMIZE_PUSH

#define DP_FAST_LATE_INIT 0
u32 dp_drop_all_tcp_err;
u32 dp_pkt_size_check;
int dp_dbg_mode = 0; /* 0-pr_info, 1-trace_printk */
u8 g_toe_disable = 0; /*Gloabal toe disable default setting: enable toe*/
u64 dp_dbg_flag;
u64 dp_dbgfs_flag;
EXPORT_SYMBOL(dp_dbgfs_flag);
#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
module_param(dp_dbg_flag, ullong, 0660);
MODULE_PARM_DESC(dp_dbg_flag, " set the dp_dbg_flag value runtime");
module_param(dp_dbgfs_flag, ullong, 0660);
MODULE_PARM_DESC(dp_dbgfs_flag, " set the dp_dbgfs_flag value runtime");
#endif
char *log_buf;
int log_buf_len = 1000;
EXPORT_SYMBOL(dp_dbg_flag);

#undef DP_DBGFS_ENUM_OR_STRING
#define DP_DBGFS_ENUM_OR_STRING(name, short_name) short_name
char *dp_dbgfs_flag_str[] = DP_DBGFS_FLAG_LIST;

#undef DP_DBGFS_ENUM_OR_STRING
#define DP_DBGFS_ENUM_OR_STRING(name, short_name) name
u64 dp_dbgfs_flag_list[] = DP_DBGFS_FLAG_LIST;

static char *dp_dbg; /* for module_param */
bool dp_mod_exiting = false;
#undef DP_DBG_ENUM_OR_STRING
#define DP_DBG_ENUM_OR_STRING(name, short_name) short_name
char *dp_dbg_flag_str[] = DP_DBG_FLAG_LIST;

#undef DP_DBG_ENUM_OR_STRING
#define DP_DBG_ENUM_OR_STRING(name, short_name) name
u64 dp_dbg_flag_list[] = DP_DBG_FLAG_LIST;

#undef DP_F_ENUM_OR_STRING
#define DP_F_ENUM_OR_STRING(name, short_name) short_name
char *dp_port_type_str[] = DP_F_FLAG_LIST;

#undef DP_F_ENUM_OR_STRING
#define DP_F_ENUM_OR_STRING(name, short_name) name
u32 dp_port_flag[] = DP_F_FLAG_LIST;

char *dp_port_status_str[] = {
	"PORT_FREE",
	"PORT_ALLOCATED",
	"PORT_DEV_REGISTERED",
	"PORT_SUBIF_REGISTERED",
	"Invalid"
};

int dp_init_ok = 0;
u32 dp_init_state = 0;
int dp_cpu_init_ok;
atomic_t dp_status = ATOMIC_INIT(0);
DP_DEFINE_LOCK(dp_lock);
u32 dp_dbg_err = 1; /*print error */
EXPORT_SYMBOL(dp_dbg_err);

struct platform_device *g_dp_dev;
/*port 0 is reserved and never assigned to any one */
int dp_inst_num;
/* Keep per DP instance information here */
struct inst_property dp_port_prop[DP_MAX_INST];
/* Keep all subif information per instance/LPID/subif */
struct dp_port_info *dp_port_info[DP_MAX_INST];

/* q_tbl[] is mainly for the queue created/used during dp_register_subif_ext
 */
struct q_info dp_q_tbl[DP_MAX_INST][DP_MAX_QUEUE_NUM];

/* sched_tbl[] is mainly for the sched created/used during dp_register_subif_ext
 * Note: dp_sched_tbl is used for all logical node, not just for scheduler itself
 */
struct dp_sched_info dp_sched_tbl[DP_MAX_INST][DP_MAX_NODES];

/* dp_deq_port_tbl[] is to record cqm dequeue port info
 */
struct cqm_deq_ring_info dp_deq_ring_tbl[DP_MAX_INST][DP_MAX_PPV4_PORT];
/* dp_enq_port_tbl[] is to record cqm enqueue port info for DC device
 */
struct cqm_enq_ring_info dp_enq_ring_tbl[DP_MAX_INST][DP_MAX_CQM_ENQ];
struct cqm_ret_ring_info dp_ret_ring_tbl[DP_MAX_INST][DP_MAX_CQM_RET];
struct cqm_req_ring_info dp_req_ring_tbl[DP_MAX_INST][DP_MAX_CQM_REQ];

struct parser_info pinfo[4];
int dp_print_len;

static void *dp_ops[DP_MAX_INST][DP_OPS_CNT];
struct cqm_ops *dp_cqm_ops[DP_MAX_INST];

/* Per CPU gobal Rx and Tx counters for DPM */
DEFINE_PER_CPU_SHARED_ALIGNED(struct mib_global_stats, mib_g_stats);

void (*dp_dev_get_ethtool_stats_fn)(struct net_device *dev,
		struct ethtool_stats *stats,
		u64 *data) = NULL;
EXPORT_SYMBOL(dp_dev_get_ethtool_stats_fn);

void dp_set_ethtool_stats_fn(int inst, void (*cb)(struct net_device *dev,
			struct ethtool_stats *stats, u64 *data))
{
	dp_dev_get_ethtool_stats_fn = cb;
}
EXPORT_SYMBOL(dp_set_ethtool_stats_fn);

int (*dp_get_dev_stat_strings_count_fn)(struct net_device *dev) = NULL;
EXPORT_SYMBOL(dp_get_dev_stat_strings_count_fn);

void dp_set_ethtool_stats_strings_cnt_fn(int inst,
		int (*cb)(struct net_device *dev))
{
	dp_get_dev_stat_strings_count_fn = cb;
}
EXPORT_SYMBOL(dp_set_ethtool_stats_strings_cnt_fn);

void (*dp_get_dev_ss_stat_strings_fn)(struct net_device *dev,
		u8 *data) = NULL;
EXPORT_SYMBOL(dp_get_dev_ss_stat_strings_fn);

void dp_set_ethtool_stats_strings_fn(int inst, void (*cb)(struct net_device *dev,
			u8 *data))
{
	dp_get_dev_ss_stat_strings_fn = cb;
}
EXPORT_SYMBOL(dp_set_ethtool_stats_strings_fn);

int dp_register_ops(int inst, enum DP_OPS_TYPE type, void *ops)
{
	if (is_invalid_inst(inst) || type >= DP_OPS_CNT) {
		DP_DEBUG(DP_DBG_FLAG_REG, "wrong index\n");
		return DP_FAILURE;
	}
	if (!ops)
		return DP_FAILURE;

	dp_ops[inst][type] = ops;
	dp_init_state |= BIT(type);

	/* Get registered ops from CQM */
	if (type == DP_OPS_CQM)
		dp_cqm_ops[inst] = (struct cqm_ops *)ops;

	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_register_ops);

void *dp_get_ops(int inst, enum DP_OPS_TYPE type)
{
	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return NULL;
	}

	if (is_invalid_inst(inst) || type >= DP_OPS_CNT) {
		DP_DEBUG(DP_DBG_FLAG_REG, "wrong index\n");
		return NULL;
	}
	return dp_ops[inst][type];
}

EXPORT_SYMBOL(dp_get_ops);

char *get_dp_port_type_str(int k)
{
	return dp_port_type_str[k];
}
EXPORT_SYMBOL(get_dp_port_type_str);

u32 get_dp_port_flag(int k)
{
	return dp_port_flag[k];
}
EXPORT_SYMBOL(get_dp_port_flag);

int get_dp_port_type_str_size(void)
{
	return ARRAY_SIZE(dp_port_type_str);
}
EXPORT_SYMBOL(get_dp_port_type_str_size);

int get_dp_dbg_flag_str_size(void)
{
	return ARRAY_SIZE(dp_dbg_flag_str);
}

int get_dp_dbgfs_flag_str_size(void)
{
	return ARRAY_SIZE(dp_dbgfs_flag_str);
}

int get_dp_port_status_str_size(void)
{
	return ARRAY_SIZE(dp_port_status_str);
}

void dp_print_err_info(int res)
{
	switch (res) {
		case DP_ERR_SUBIF_NOT_FOUND:
			pr_err("DPM: subif not found\n");
			break;
		case DP_ERR_INIT_FAIL:
			pr_err("DPM: init not done\n");
			break;
		case DP_ERR_INVALID_PORT_ID:
			pr_err("DPM: invalid port id\n");
			break;
		case DP_ERR_MEM:
			pr_err("DPM: memory allocation failure\n");
			break;
		case DP_ERR_NULL_DATA:
			pr_err("DPM: exp data info is NULL\n");
			break;
		case DP_ERR_INVALID_SUBIF:
			pr_err("DPM: invalid subif\n");
			break;
		case DP_ERR_DEFAULT:
			pr_err("DPM: other generic error\n");
			break;
		default:
			pr_err("DPM: why come to here??(%s)\n", __func__);
			break;
	}
}

u32 *get_port_flag(int inst, int index)
{
	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return NULL;
	}

	if (is_invalid_inst(inst))
		return NULL;

	if (index < dp_port_prop[inst].info.cap.max_num_dp_ports)
		return &get_dp_port_info(inst, index)->alloc_flags;

	return NULL;
}
EXPORT_SYMBOL(get_port_flag);

struct dp_port_info *get_port_info_via_dp_port(int inst, int dp_port)
{
	int i;

	for (i = 0; i < dp_port_prop[inst].info.cap.max_num_dp_ports; i++) {
		struct dp_port_info *port = get_dp_port_info(inst, i);

		if ((port->status & PORT_DEV_REGISTERED) &&
				port->port_id == dp_port)
			return port;
	}

	return NULL;
}

void dp_dump_umt_attr(struct seq_file *s, int indent, struct dp_umt_attr *u)
{
	if (!u)
		return;
	dp_sprintf(s, "%*s   attr.umt.msg_mode: %d (%s)\n", indent, "", u->msg_mode,
			u->msg_mode ? "UMT MSG MODE" : "UMT INTR MODE");
	dp_sprintf(s, "%*s   attr.umt.dst     : 0x%llx\n", indent, "", u->dst);
	if (u->msg_mode) {
		dp_sprintf(s, "%*s   attr.umt.msg_attr.enable_send : %d\n",
				indent, "",
				u->msg_attr.enable_send);
		dp_sprintf(s, "%*s   attr.umt.msg_attr.sup_zero_msg: %d\n",
				indent, "",
				u->msg_attr.supress_zero_msg);
	} else {
		dp_sprintf(s, "%*s   attr.umt.irq_attr.p_cnt_regs[0:UMT_ACC]: 0x%llx\n",
				indent, "",
				u->irq_attr.p_cnt_regs[0]);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.p_cnt_regs[1:UMT_ADD]: 0x%llx\n",
				indent, "",
				u->irq_attr.p_cnt_regs[1]);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.p_cnt_regs[2:UMT_SUB]: 0x%llx\n",
				indent, "",
				u->irq_attr.p_cnt_regs[2]);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.v_cnt_regs[0:UMT_ACC]: %px\n",
				indent, "",
				u->irq_attr.v_cnt_regs[0]);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.v_cnt_regs[1:UMT_ADD]: %px\n",
				indent, "",
				u->irq_attr.v_cnt_regs[1]);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.v_cnt_regs[2:UMT_SUB]: %px\n",
				indent, "",
				u->irq_attr.v_cnt_regs[2]);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.threshold: %d\n",
				indent, "",
				u->irq_attr.threshold);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.umt_poll: %px\n",
				indent, "",
				u->irq_attr.umt_poll);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.ring_poll_data: %px\n",
				indent, "",
				u->irq_attr.ring_poll_data);
		dp_sprintf(s, "%*s   attr.umt.irq_attr.err_cb: %px\n",
				indent, "",
				u->irq_attr.err_cb);
	}
	return;
}

void dp_dump_ring_attr(struct seq_file *s, int indent, struct dp_ring_attr *a, bool umt_en)
{
	if (!a)
		return;

	dp_sprintf(s, "%*s   attr.ring.index  : %d\n", indent, "", a->ring.index);
	dp_sprintf(s, "%*s   attr.ring.port_id: %u\n", indent, "", a->ring.port_id);
	dp_sprintf(s, "%*s   attr.size        : %d\n", indent, "", a->size);
	dp_sprintf(s, "%*s   attr.paddr       : %px\n", indent, "", a->paddr);
	dp_sprintf(s, "%*s   attr.vaddr       : %px\n", indent, "", a->vaddr);
	if (umt_en)
		dp_dump_umt_attr(s, indent, &a->umt);
}

/*It can be used to dump pkt_deq, tx ippu and rx ippu rings*/
void __dp_dump_pktdeq_ring(int indent, struct dp_ring_pktdeq *r, int n,
		bool umt_en, bool ippu, bool tx)
{
	int i;

	for(i = 0; i < n; i++) {
		pr_cont("%*sdp_ring_pktdeq%s[%d]:\n", indent, "", ippu ? tx ? " tx_ippu":" rx_ippu":"", i);
		dp_dump_ring_attr(NULL, indent, &r[i].attr, umt_en);
		pr_cont("%*s   txpush_addr_qos  : %px\n", indent, "", r[i].txpush_addr_qos);
		pr_cont("%*s   dma_ch_id        : 0x%x\n", indent, "", r[i].dma_ch_id);
		pr_cont("%*s   pkt_credit       : %u\n", indent, "", r[i].pkt_credit);
		pr_cont("%*s   b_credit         : %u\n", indent, "", r[i].b_credit);
		pr_cont("%*s   num_policy       : %hhu\n", indent, "", r[i].num_policy);
		pr_cont("%*s   policy_base      : %d\n", indent, "", r[i].policy_base);
		pr_cont("%*s   pool_id(base)    : %hu\n", indent, "", r[i].pool_id);
		pr_cont("%*s   policy_map(Tx)   : %hhu\n", indent, "", r[i].policy_map);
	}
	return;
}

void dp_dump_pktenq_ring(int indent, struct dp_ring_pktenq *r, int n)
{
	int i;

	for(i = 0; i < n; i++) {
		pr_cont("%*sdp_ring_pktenq[%d]:\n", indent, "", i);
		dp_dump_ring_attr(NULL, indent, &r[i].attr, 0);
	}
	return;
}

void dp_dump_bufret_ring(int indent, struct dp_ring_bufret *r, int n)
{
	int i;

	for(i = 0; i < n; i++) {
		pr_cont("%*sdp_ring_bufret[%d]:\n", indent, "", i);
		dp_dump_ring_attr(NULL, indent, &r[i].attr, 0);
	}
	return;
}

void dp_dump_bufreq_ring(int indent, struct dp_ring_bufreq *r, int n)
{
	int i, j;

	for(i = 0; i < n; i++) {
		pr_cont("%*sdp_ring_bufreq[%d]:\n", indent, "", i);
		dp_dump_ring_attr(NULL, indent, &r[i].attr, 0);
		pr_cont("%*s   num_policy: %d (max: %d)\n", indent, "", r[i].num_policy, DP_MAX_POLICY_PER_DEV);
		for (j = 0; j < r[i].num_policy; j++)
			pr_cont("%*s   policy: %d, min_guarantee: %u, buf_size: %d\n",
					indent, "", j, r[i].min_guarantee[j],
					r[i].buf_size[j]);
		pr_cont("%*s   policy_base: %d\n", indent, "", r[i].policy_base);
		pr_cont("%*s   policy_map: %hhu\n", indent, "",  r[i].policy_map);
		pr_cont("%*s   pool_id: %hu\n", indent, "", r[i].pool_id);
		pr_cont("%*s   min_burst: %hu\n", indent, "", r[i].min_burst);
	}
	return;
}

void dp_dump_lookup_sel_attr(int indent, const struct lookup_sel_attr *sel)
{
	int i;
	pr_cont("%*slookup_sel_attr:\n", indent, "");
	for(i = 0; i < CQM_LOOKUP_SEL_NUM; i++) {
		pr_info("%*s   sel[%d].size   : %hd\n", indent, "", i, sel[i].size);
		pr_info("%*s   sel[%d].hd_off : %hd\n", indent, "", i, sel[i].hd_offset);
		pr_info("%*s   sel[%d].dst_off: %hd\n", indent, "", i, sel[i].dst_offset);
	}
}

void dp_dump_dp_port_data(int indent, struct dp_port_data *d)
{
	if (!d)
		return;

	pr_cont("%*s   dp_port_data: 0x%px\n", indent, "", d);
	pr_cont("%*s      flag_ops         : 0x%x\n", indent, "", d->flag_ops);
	pr_cont("%*s      resv_num_port    : %u\n", indent, "", d->resv_num_port);
	pr_cont("%*s      start_port_no    : %u\n", indent, "", d->start_port_no);
	pr_cont("%*s      num_deq          : %d (max: %d)\n",
			indent, "", d->num_deq, DP_NUM_PKTDEQ_RING);
	pr_cont("%*s      deq_ring_base    : %px\n",
			indent, "", d->deq_ring_base);
	pr_cont("%*s      link_speed_cap   : %d\n", indent, "", d->link_speed_cap);
}

void dp_dump_pmac_cfg(dp_pmac_cfg_t *pmac_cfg)
{
#ifdef TOPAZ_CODE_ENABLE
	int i;

	if (!pmac_cfg)
		return;
	pr_cont("   pmac_cfg: 0x%px\n", pmac_cfg);
	pr_cont("      ig_pmac_flags: 0x%x\n", pmac_cfg->ig_pmac_flags);
	pr_cont("      eg_pmac_flags: 0x%x\n", pmac_cfg->eg_pmac_flags);
	pr_cont("      ig_pmac:\n");
	pr_cont("         tx_dma_chan: %d, err_disc: %d, pmac: %d, "
			"def_pmac: %d, def_pmac_pmap: %d\n"
			"         def_pmac_en_pmap: %d, def_pmac_tc: %d, "
			"def_pmac_en_tc: %d, def_pmac_subifid: %d\n"
			"         def_pmac_src_port: %d, res_ing: 0x%hx\n"
			"         def_pmac_hdr(len=%d): ",
			pmac_cfg->ig_pmac.tx_dma_chan,
			pmac_cfg->ig_pmac.err_disc,
			pmac_cfg->ig_pmac.pmac,
			pmac_cfg->ig_pmac.def_pmac,
			pmac_cfg->ig_pmac.def_pmac_pmap,
			pmac_cfg->ig_pmac.def_pmac_en_pmap,
			pmac_cfg->ig_pmac.def_pmac_tc,
			pmac_cfg->ig_pmac.def_pmac_en_tc,
			pmac_cfg->ig_pmac.def_pmac_subifid,
			pmac_cfg->ig_pmac.def_pmac_src_port,
			pmac_cfg->ig_pmac.res_ing, DP_MAX_PMAC_LEN);
	for (i = 0; i < DP_MAX_PMAC_LEN; i++)
		pr_cont("0x%02x ", pmac_cfg->ig_pmac.def_pmac_hdr[i]);
	pr_cont("\n");
	pr_cont("      eg_pmac:\n");
	pr_cont("         rx_dma_chan: %d, rm_l2hdr: %d, "
			"num_l2hdr_bytes_rm: %d, "
			"fcs: %d, pmac: %d, redir: %d, bsl_seg: %d, "
			"dst_port: %d\n"
			"         res_endw1: %d, res_dw1: %d, res1_endw0: %d, "
			"res1_dw0: %d, res2_endw0: %d, res2_dw0: %d"
			"tc_enable: %d, traffic_class: %d\n"
			"         flow_id: %d, dec_flag: %d, enc_flag: %d, "
			"mpe1_flag: %d, mpe2_flag: %d, res_eg: 0x%x\n",
			pmac_cfg->eg_pmac.rx_dma_chan,
			pmac_cfg->eg_pmac.rm_l2hdr,
			pmac_cfg->eg_pmac.num_l2hdr_bytes_rm,
			pmac_cfg->eg_pmac.fcs,
			pmac_cfg->eg_pmac.pmac,
			pmac_cfg->eg_pmac.redir,
			pmac_cfg->eg_pmac.bsl_seg,
			pmac_cfg->eg_pmac.dst_port,
			pmac_cfg->eg_pmac.res_endw1,
			pmac_cfg->eg_pmac.res_dw1,
			pmac_cfg->eg_pmac.res1_endw0,
			pmac_cfg->eg_pmac.res1_dw0,
			pmac_cfg->eg_pmac.res2_endw0,
			pmac_cfg->eg_pmac.res2_dw0,
			pmac_cfg->eg_pmac.tc_enable,
			pmac_cfg->eg_pmac.traffic_class,
			pmac_cfg->eg_pmac.flow_id,
			pmac_cfg->eg_pmac.dec_flag,
			pmac_cfg->eg_pmac.enc_flag,
			pmac_cfg->eg_pmac.mpe1_flag,
			pmac_cfg->eg_pmac.mpe2_flag,
			pmac_cfg->eg_pmac.res_eg);
#endif
}

static struct kmem_cache *subif_cache;
static int dp_subif_init(void)
{
	subif_cache = kmem_cache_create("dp_subif",
					sizeof(struct dp_subif),
					0, SLAB_HWCACHE_ALIGN, NULL);
	if (!subif_cache)
		return -ENOMEM;
	return 0;
}

static void dp_subif_free(void)
{
	kmem_cache_destroy(subif_cache);
}

/*note: dev can be NULL */
static int32_t dp_alloc_port_private(int inst,
		struct module *owner,
		struct net_device *dev,
		u32 dev_port, s32 port_id,
		dp_pmac_cfg_t *pmac_cfg,
		struct dp_port_data *data,
		u32 flags)
{
	int i, ring_idx;
	struct cqm_dp_alloc_data cqm_data = {0};
	struct dp_port_info *port;
	struct inst_info *dp_info;
	struct cqm_deq_ring_info *rinfo;
	struct dp_port_data_priv *priv_data;

	if (!owner) {
		pr_err("DPM: Allocate port failed for owner NULL\n");
		return DP_FAILURE;
	}
	if (is_invalid_port(port_id) || is_invalid_inst(inst)) {
		pr_err("DPM: %s: wrong port_id=%d or inst=%d\n", __func__, port_id,
		       inst);
		return DP_FAILURE;
	}

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (unlikely(dp_dbg_flag & DP_DBG_FLAG_REG)) {
		pr_cont("\n=====> DPM: %s input arguments:\n"
			"   inst: %d, owner: 0x%px(%s), dev: 0x%px, dev_port: %u, port_id: %d\n"
			"   pmac_cfg: 0x%px, data: 0x%px, Flags: ",
			__func__, inst, owner, owner->name, dev, dev_port, port_id, pmac_cfg,
			data);
		for (i = 0; i < ARRAY_SIZE(dp_port_type_str); i++)
			if (flags & dp_port_flag[i])
				pr_cont("%s| ", dp_port_type_str[i]);
		pr_cont("\n");
		/*Print dp_pmac_cfg*/
		dp_dump_pmac_cfg(pmac_cfg);
		dp_dump_dp_port_data(0, data);
	}
#endif

	dp_info = get_dp_prop_info(inst);
	cqm_data.dp_inst = inst;
	cqm_data.cqm_inst = dp_port_prop[inst].cqm_inst;

	if (flags & DP_F_DEREGISTER) {	/*De-register */
		port = get_dp_port_info(inst, port_id);
		if (port->status != PORT_ALLOCATED) {
			pr_err("DPM: %s is failed, inst: %d, port_id: %d, "
			       "module: %s\n", __func__, inst, port_id,
			       owner->name);
			return DP_FAILURE;
		}

		dp_notifier_invoke(inst, dev, port_id, 0, NULL,
				DP_EVENT_DE_ALLOC_PORT);

		cqm_data.data = NULL;
		cqm_data.dp_port = port_id;
		dp_dealloc_cqm_port(owner, dev_port, port, &cqm_data,
				port->alloc_flags | flags);
		dp_inst_del_mod(owner, port_id, 0);
		DP_CB(inst, port_platform_set)(inst, port_id, data, flags);
		/* Only clear those fields we need to clear */
		dp_memset(port, 0, offsetof(struct dp_port_info, tail));
		return DP_SUCCESS;
	}
	if (port_id) { /*with specified port_id */
		port = get_dp_port_info(inst, port_id);
		if (port->status != PORT_FREE) {
			pr_err("DPM: %s %s(%s %d) fail: port %d used by %s %d\n",
					"module", owner->name,
					"dev_port", dev_port, port_id,
					port->owner->name,
					port->dev_port);
			return DP_FAILURE;
		}
	}

	priv_data = dp_kzalloc(sizeof(*priv_data), GFP_ATOMIC);
	if (!priv_data) {
		pr_err("DPM: %s, Failed to allocate memory!\n", __func__);
		return DP_FAILURE;
	}
	priv_data->udata = data;
	cqm_data.data = priv_data;
	if (dp_alloc_cqm_port(owner, dev, dev_port, port_id, &cqm_data, flags))
		goto free;

	port_id = cqm_data.dp_port;
	if (port_id >= dp_info->cap.max_num_dp_ports) {
		pr_err("DPM: wrong dp_port: %d. Max Number of dp_port=%d\n",
				port_id, dp_info->cap.max_num_dp_ports);
		goto free;
	}
	/* Only clear those fields we need to clear */
	port = get_dp_port_info(inst, port_id);
	dp_memset(port, 0, offsetof(struct dp_port_info, tail));

	/*save info from caller */
	port->owner = owner;
	port->dev = dev;
	port->dev_port = dev_port;
	port->alloc_flags = flags;
	port->status = PORT_ALLOCATED;

	/*save info from cqm_dp_port_alloc to pmac_port DB*/
	port->flag_other = cqm_data.flags;
	port->port_id = cqm_data.dp_port;
	port->inst_prop = get_dp_port_prop(inst);
	port->inst = inst;
	port->data_flag_ops = data->flag_ops;

	if (is_xpon(flags)) {
		/*TODO: its pon device, allocate qos port*/
	}

	/*TX deq ring: Sanity check */
	if (data->num_deq > DP_MAX_PKTDEQ_RING) {
		pr_err("DPM: %s, wrong deq_port_num: %d, max allowed: %d\n",
				__func__,
				data->num_deq, DP_MAX_PKTDEQ_RING);
		goto cqm_free;
	}

	/*TX deq ring: save full info into deq_ring tbl and store ref dp_port_info tbl */
	for (i = 0; i < data->num_deq; i++) {
		ring_idx = priv_data->deq[i].attr.ring.index;
		rinfo = get_dp_deqring_info(inst, ring_idx);

		rinfo->deq = priv_data->deq[i];
		rinfo->dp_port[cqm_data.dp_port] = 1;
		rinfo->ring_id = ring_idx;
		rinfo->type = CQM_DEQ_RING_NORM;

		port->deq[i] = &rinfo->deq;
		port->num_deq++;
	}

	/*TX ippu ring: Sanity check */
	if (priv_data->num_ippu > DP_MAX_INTERM) {
		pr_err("DPM: %s, wrong tx ippu: %d, max allowed: %d\n",
				__func__,
				priv_data->num_ippu, DP_MAX_INTERM);
		goto cqm_free;
	}

	/*TX ippu ring: save full info into deq_ring tbl and store ref dp_port_info tbl */
	for (i = 0; i < priv_data->num_ippu; i++) {
		ring_idx = priv_data->ippu[i].attr.ring.index;
		rinfo = get_dp_deqring_info(inst, ring_idx);

		rinfo->deq = priv_data->ippu[i];
		rinfo->dp_port[cqm_data.dp_port] = 1;
		rinfo->ring_id = ring_idx;
		/*bitwise | cuz a ring id can be tx and rx ippu both*/
		rinfo->type |= CQM_DEQ_RING_TX_IPPU;

		port->tx_ippu[i] = &rinfo->deq;
		port->num_tx_ippu++;
	}

	if (dp_info->port_platform_set(inst, port_id, data, flags)) {
		pr_err("DPM: Failed port_platform_set for port_id=%d(%s)\n",
				port_id, owner ? owner->name : "");
		goto cqm_free;
	}
	//if (pmac_cfg)
	//	dp_pmac_set(inst, port_id, pmac_cfg); VBOLLA: this is calling
	//	gsw_pmac_set() function internally. So need to check, if pmac
	//	config to do in IPPU.

	/*only 1st dp instance support real CPU path traffic */
	if (!inst && dp_info->init_adp_template) //TODO: VBOLLA, had to do when ADP desc changes
		dp_info->init_adp_template(port_id, flags, false);

	dp_inst_insert_mod(owner, port_id, inst, 0);
#ifdef CONFIG_RFS_ACCEL //VBOLLA: is this needed for TOPAZ? valid for TPZ
	if (is_soc_tpz(inst)) {
		struct dp_port_info *cpu_port = NULL;
		/* Setup the Rx_cpu_map for the device, if present */
		cpu_port = get_dp_port_info(inst, CPU_PORT);
		if (cpu_port && dev && cpu_port->rx_cpu_rmap) {
			dev->rx_cpu_rmap = cpu_port->rx_cpu_rmap;
			if (unlikely(!dev->rx_cpu_rmap))
				DP_DEBUG(DP_DBG_FLAG_REG, "[%s]: dev->rx_cpu_rmap nul for [%s]\n", __func__, dev->name);
		}
	}
#endif /* CONFIG_RFS_ACCEL */
	DP_DEBUG(DP_DBG_FLAG_REG,
			"Port %d allocation succeed for module %s with dev_port %d\n",
			port_id, owner->name, dev_port);

	dp_notifier_invoke(inst, dev, port_id, 0, NULL, DP_EVENT_ALLOC_PORT);

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (unlikely(dp_dbg_flag & DP_DBG_FLAG_REG)) {
		pr_cont("\n<===== DPM: %s output arguments:\n"
			"   inst: %d, owner: 0x%px(%s), dev: 0x%px, dev_port: %u, port_id: %d\n"
			"   pmac_cfg: 0x%px, data: 0x%px, Flags: ",
			__func__, inst, owner, owner->name, dev, dev_port, port_id, pmac_cfg,
			data);
		for (i = 0; i < ARRAY_SIZE(dp_port_type_str); i++)
			if (flags & dp_port_flag[i])
				pr_cont("%s| ", dp_port_type_str[i]);
		pr_cont("\n");
		/*Print dp_pmac_cfg*/
		dp_dump_pmac_cfg(pmac_cfg);
		dp_dump_dp_port_data(0, data);
	}
#endif
	kfree(priv_data);
	return port_id;

cqm_free:
	dp_dealloc_cqm_port(owner, dev_port, port, &cqm_data,
			flags);
	/* Only clear those fields we need to clear */
	dp_memset(port, 0, offsetof(struct dp_port_info, tail));
free:
	kfree(priv_data);

	return DP_FAILURE;

}

int32_t dp_register_subif_private(int inst, struct module *owner,
		struct net_device *dev,
		char *subif_name, dp_subif_t *subif_in,
		struct dp_subif_data *data, u32 flags)
{
	int i, port_id, start, end, j;
	struct dp_port_info *port_info;
	struct cqm_dp_en_data *cqm_data;
	struct subif_platform_data platfrm_data = {0};
	struct dp_subif_info *sif;
	struct inst_info *dp_info = get_dp_prop_info(inst);
	struct cqm_deq_ring_info *deq;

	port_id = subif_in->port_id;
	port_info = get_dp_port_info(inst, port_id);
	subif_in->inst = inst;
	subif_in->subif_num = 1;
	platfrm_data.subif_data = data;
	platfrm_data.dev = dev;
	/*Sanity Check*/
	if (port_info->status < PORT_DEV_REGISTERED) {
		DP_DEBUG(DP_DBG_FLAG_REG,
				"register subif failed:%s is not a registered dev!\n",
				subif_name);
		return DP_FAILURE;
	}
	if (subif_in->subif < 0) {/*dynamic mode */
		start = 0;
		end = port_info->max_subif;
	} else {
		/*caller provided subif. Try to get its vap value as start */
		start = GET_VAP(subif_in->subif, port_info->vap_offset,
				port_info->vap_mask);
		end = start + 1;
	}

	/*allocate a free subif */
	for (i = start; i < end; i++) {
		sif = get_dp_port_subif(port_info, i);
		if (!sif->flags)
			break;
		if ((subif_in->subif > 0) && (start == i))
			pr_err("DPM: subifid(%d) is duplicated! vap(%d)\n",
					subif_in->subif, start);
	}
	if (i >= end) {
		pr_err("DPM: register subif failed for no matched vap(%s)\n",
				dev ? dev->name : "NULL");
		return DP_FAILURE;
	}
	if (data->num_deq_ring == 0)
		data->num_deq_ring = 1;

	sif->dp_port = port_id;
	cqm_data = dp_kzalloc(sizeof(*cqm_data), GFP_ATOMIC);
	if (!cqm_data)
		return DP_FAILURE;
	cqm_data->dp_inst = inst;
	cqm_data->cqm_inst = dp_port_prop[inst].cqm_inst;
	cqm_data->f_policy = data->f_policy;
	cqm_data->bm_policy_res_id = data->bm_policy_res_id;
	cqm_data->data = data;

	if ((data->num_deq_ring + data->deq_ring_idx) > port_info->num_deq) {
		pr_err("DPM: %s, num_deq_ring: %d exceeds the max: %d\n",
				__func__, data->num_deq_ring,
				port_info->num_deq);
		kfree(cqm_data);
		return DP_FAILURE;
	}
	for (j = 0; j < data->num_deq_ring; j++) {
		cqm_data->deq_port_ring[j] = port_info->deq[data->deq_ring_idx + j]->attr.ring.index;
		cqm_data->num_deq_ring++;
#ifdef TOPAZ_CODE_ENABLE
		/*TODO: VBOLLA: CQM has to internally handle it., if refcnt is > 1*/
		if (get_dp_deqring_info(inst, cqm_data->deq_port_ring[j])->ref_cnt)
			continue;
#endif
	}

	/* always call dp_enable_cqm_port even without US traffic.
	 * The reason is to get the unique policy for this subif.
	 */
	if (dp_enable_cqm_rings(owner, port_info, cqm_data, 0)) {
		kfree(cqm_data);
		return DP_FAILURE;
	}

	sif->port_info = port_info;
	if (data->f_policy) {
		sif->tx_policy_num = cqm_data->tx_policy_num;
		sif->tx_policy_base = cqm_data->tx_policy_base;
		sif->rx_policy_num = cqm_data->rx_policy_num;
		sif->rx_policy_base = cqm_data->rx_policy_base;

		/* copy back the policy information to caller */
		data->tx_policy_num = cqm_data->tx_policy_num;
		data->tx_policy_base = cqm_data->tx_policy_base;
		data->rx_policy_num = cqm_data->rx_policy_num;
		data->rx_policy_base = cqm_data->rx_policy_base;
	} else {
		/* If the policy details in the deq[] or req[] differ in each array index then
		 * user has to pass which one to select. As of now hardcoding
		 * 0th index.
		 */
		sif->tx_policy_num = port_info->deq[data->deq_ring_idx]->num_policy;
		sif->tx_policy_base = port_info->deq[data->deq_ring_idx]->policy_base;
		if (port_info->req[data->deq_ring_idx] && port_info->num_req) {
			/*only valid for ACA, so for steaming case port_info->req[x] is NULL*/
			sif->rx_policy_num = port_info->req[data->deq_ring_idx]->num_policy;
			sif->rx_policy_base = port_info->req[data->deq_ring_idx]->policy_base;
		}
	}

	if (subif_in->subif < 0) {
		sif->subif = SET_VAP(i, port_info->vap_offset,
				port_info->vap_mask);
		subif_in->subif = sif->subif;
	} else /* provided by caller since it is alerady shifted properly */
		sif->subif = subif_in->subif;

	if (dp_info->subif_platform_set(inst, port_id, i, &platfrm_data,
				flags)) {
		pr_err("DPM: subif_platform_set fail\n");
		kfree(cqm_data);
		return DP_FAILURE;
	}
	DP_DEBUG(DP_DBG_FLAG_REG, "subif_platform_set succeed\n");

	sif->flags = 1;
	sif->netif = dev;
	sif->num_deq_ring = data->num_deq_ring;
	/* currently this field is used for EPON case. Later can enhance */
	sif->num_qid = data->num_deq_ring;
	sif->deq_ring_idx = data->deq_ring_idx;

	dp_strlcpy(sif->device_name, subif_name, sizeof(sif->device_name));
	sif->subif_flag = flags;
	sif->spl_conn_type = DP_SPL_INVAL;
	sif->data_flag_ops = data->flag_ops;
	if (data->flag_ops & DP_SUBIF_RX_FLAG)
		STATS_SET(sif->rx_flag, !!data->rx_en_flag);
	else
		STATS_SET(sif->rx_flag, 1);

	if (!data->rx_fn)
		sif->rx_fn = port_info->cb.rx_fn;
	else
		sif->rx_fn = data->rx_fn;

	sif->get_subifid_fn = data->get_subifid_fn;
	if (!sif->get_subifid_fn)
		sif->get_subifid_fn = port_info->cb.get_subifid_fn;
	port_info->status = PORT_SUBIF_REGISTERED;
	subif_in->port_id = port_id;
	subif_in->subif = sif->subif;
	subif_in->def_qid = sif->def_qid;
	subif_in->gpid = sif->gpid;
	for (j = 0; j < sif->num_deq_ring; j++) {
		deq = get_dp_deqring_info(inst, sif->cqm_deq_ring[j]);
		subif_in->deq[j] = deq->deq;
	}
	subif_in->num_deq = sif->num_deq_ring;

	for (j = 0; j < sif->num_tx_ippu; j++) {
		deq = get_dp_deqring_info(inst, sif->tx_ippu_ring[j]);
		subif_in->tx_ippu[j] = deq->deq;
	}
	subif_in->num_tx_ippu = sif->num_tx_ippu;

	for (j = 0; j < sif->num_rx_ippu; j++) {
		deq = get_dp_deqring_info(inst, sif->rx_ippu_ring[j]);
		subif_in->rx_ippu[j] = deq->deq;
	}
	subif_in->num_rx_ippu = sif->num_rx_ippu;

	sif->subif_groupid = i; /*save vap/subif_grp*/
	subif_in->subif_groupid = i;
	port_info->num_subif++;

	_dp_init_subif_q_map_rules(sif, 0);
	/*default egress, TX ippu and RX ippu queue map lookup table set*/
	//TODO: VBOLLA: for pon, pon driver calls only one time
	//subif_register(), how to handle this for PON case??
	for (i = 0; i < sif->num_deq_ring; i++)
		_dp_set_subif_q_lookup_tbl(sif, i, 0);

	kfree(cqm_data);
	return DP_SUCCESS;
}

int32_t dp_deregister_subif_private(
	int inst, struct module *owner, struct net_device *dev,
	char *subif_name, dp_subif_t *subif_id,
	struct dp_subif_data *data, u32 flags)
{
	int res = DP_FAILURE;
	int i, j, port_id;
	u8 find = 0;
	struct dp_port_info *port_info;
	struct cqm_dp_en_data *cqm_data;
	struct subif_platform_data platfrm_data = {0};
	struct dp_subif_info *sif;
	struct inst_info *dp_info = get_dp_prop_info(inst);

	port_id = subif_id->port_id;
	port_info = get_dp_port_info(inst, port_id);
	platfrm_data.subif_data = data;
	platfrm_data.dev = dev;

	if (port_info->status != PORT_SUBIF_REGISTERED) {
		pr_err("DPM: %s: Unregister failed:%s not registered subif!\n",
				__func__, dev->name);
		return res;
	}

	for (i = 0; i < port_info->max_subif; i++) {
		sif = get_dp_port_subif(port_info, i);
		if (sif->subif == subif_id->subif) {
			find = 1;
			break;
		}
	}
	if (!find)
		return res;

	DP_DEBUG(DP_DBG_FLAG_REG,
			"Found matched subif: port_id=%d subif=%x vap=%d\n",
			subif_id->port_id, subif_id->subif, i);

	/* device not match */
	if (sif->netif != dev) {
		pr_err("DPM: %s: dev=%s not match %s\n", __func__, dev->name,
				sif->netif->name);
		return DP_FAILURE;
	}

	/* disable dp_rx path for this device */
	STATS_SET(sif->rx_flag, 0);

	if (sif->ctp_dev) {
		dp_notifier_invoke(inst, sif->ctp_dev, port_id, subif_id->subif,
				NULL, DP_EVENT_DE_REGISTER_SUBIF);
	}

	for (j = 0; j < sif->num_deq_ring; j++)
		_dp_reset_subif_q_lookup_tbl(sif, j);

	/* subif_hw_reset */
	if (dp_info->subif_platform_set(inst, port_id, i, &platfrm_data,
				flags)) {
		pr_err("DPM: subif_hw_reset fail\n");
	}
	/* reset mib, flag, and others */
	dp_memset(&sif->mib, 0, sizeof(sif->mib));
	sif->flags = 0;
	sif->netif = NULL;
	port_info->num_subif--;

	if (!port_info->num_subif)
		port_info->status = PORT_DEV_REGISTERED;

	cqm_data = dp_kzalloc(sizeof(*cqm_data), GFP_ATOMIC);
	if (!cqm_data)
		return DP_FAILURE;

	cqm_data->dp_inst = inst;
	cqm_data->cqm_inst = dp_port_prop[inst].cqm_inst;
	for (j = 0; j < sif->num_qid; j++) {
		cqm_data->deq_port_ring[j] = sif->cqm_deq_ring[j];
		cqm_data->num_deq_ring++;
		if (get_dp_deqring_info(inst, cqm_data->deq_port_ring[j])->ref_cnt)
			continue;
	}

	/* Note: for deregistration, caller no need to fill in policy
	 * information even if this subif once requested the unique
	 * policy during registration stage. Instead CQM driver will
	 * handle it
	 */
	if (dp_enable_cqm_rings(owner, port_info, cqm_data,
				CQM_PORT_F_DISABLE)) {
		kfree(cqm_data);
		return DP_FAILURE;
	}

	DP_DEBUG(DP_DBG_FLAG_REG, "  dp_port=%d subif=%d cqm_ring[0]=%d\n",
			subif_id->port_id, subif_id->subif, cqm_data->deq_port_ring[0]);
	res = DP_SUCCESS;

	kfree(cqm_data);
	return res;
}

/*Note: For same owner, it should be in the same HW instance
 *          since dp_register_dev/subif no dev_port information at all,
 *          at the same time, dev is optional and can be NULL
 */

int32_t dp_alloc_port(struct module *owner, struct net_device *dev,
		u32 dev_port, int32_t port_id,
		dp_pmac_cfg_t *pmac_cfg, u32 flags)
{
	struct dp_port_data *data = NULL;
	int ret;

	data = dp_kzalloc(sizeof(*data), GFP_ATOMIC);
	if (!data)
		return DP_FAILURE;

	ret = dp_alloc_port_ext(0, owner, dev, dev_port, port_id, pmac_cfg,
			data, flags);
	kfree(data);

	return ret;
}
EXPORT_SYMBOL(dp_alloc_port);

static int dp_sanity_check(void)
{
	int size1, size2;
	int res = 0;

	size1 = offsetof(struct dp_subif_info, subif_cmn_end) -
		offsetof(struct dp_subif_info, subif_cmn_start);
	size2 = offsetof(dp_subif_t, subif_cmn_end) -
		offsetof(dp_subif_t, subif_cmn_start);
	if (size1 != size2) {
		pr_err("dpm: subif_info defined in dp_subif_info and dp_subif_t not match: %ul_%ul\n",
		       size1, size2);
		res = -1;
	}

	size1 = offsetof(struct dp_port_info, port_cmn_end) -
		offsetof(struct dp_port_info, port_cmn_start);
	size2 = offsetof(dp_subif_t, port_cmn_end) -
		offsetof(dp_subif_t, port_cmn_start);
	if (size1 != size2) {
		pr_err("dpm: port_info defined in dp_port_info and dp_subif_t not match:%ul_%ul\n",
		       size1, size2);
		res = -1;
	}
	return res;
}

static bool dp_late_init(void)
{
	if (dp_sanity_check())
		return false;

	if (atomic_cmpxchg(&dp_status, 0, 1) == 0)
		dp_late_init_module();
	if (!dp_init_ok)
		pr_err("DPM: dp_late_init fail: datapath can't init\n");
	return dp_init_ok;
}

#if IS_ENABLED(CONFIG_DPM_DATAPATH_EXTRA_DEBUG)
int dp_late_exit_module(void)
{
	int i;

	DP_DUMP("start late cleanup dp module\n");
	DP_LIB_LOCK(&dp_lock);
	if (dp_init_ok) {  /* map to dp_late_exit_module */

		for (i = 0; i < dp_inst_num; i++) {
			DP_CB(i, dp_platform_set)(i, DP_PLATFORM_DE_INIT);
			free_dp_port_subif_info(i);
			dp_port_prop[i].valid = 0;
		}
		dp_inst_num = 0;
		dp_init_ok = 0;
	}

	DP_LIB_UNLOCK(&dp_lock);

	return 0;
}

static bool dp_late_exit(void)
{
	if (atomic_cmpxchg(&dp_status, 1, 0) == 1)
		dp_late_exit_module();
	if (dp_init_ok)
		pr_err("DPM: dp_late_exit fail: datapath can't exit\n");
	return dp_init_ok;
}

int dp_manual_init_exit_trigger(bool exit_flag)
{
	return exit_flag ? dp_late_exit() : dp_late_init();
}
#endif

int32_t dp_alloc_port_ext(int inst, struct module *owner,
		struct net_device *dev,
		u32 dev_port, int32_t port_id,
		dp_pmac_cfg_t *pmac_cfg,
		struct dp_port_data *data, u32 flags)
{
	int res;
	struct dp_port_data *tmp_data;

	tmp_data = dp_kzalloc(sizeof(*tmp_data), GFP_ATOMIC);
	if (!tmp_data)
		return DP_FAILURE;
	if (!dp_late_init()) {
		kfree(tmp_data);
		return DP_FAILURE;
	}
	if (!dp_port_prop[0].valid) {
		pr_err("DPM: No Valid datapath instance yet?\n");
		kfree(tmp_data);
		return DP_FAILURE;
	}
	if (!data)
		data = tmp_data;
	DP_LIB_LOCK(&dp_lock);
	res = dp_alloc_port_private(inst, owner, dev, dev_port,
			port_id, pmac_cfg, data, flags);
	DP_LIB_UNLOCK(&dp_lock);
	kfree(tmp_data);
	if (inst) /* only inst zero need ACA workaround */
		return res;

	return res;
}
EXPORT_SYMBOL(dp_alloc_port_ext);

int32_t dp_register_dev(struct module *owner, u32 port_id,
		dp_cb_t *dp_cb, u32 flags)
{
	int inst;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	if (!owner || is_invalid_port(port_id))
		return DP_FAILURE;

	inst = dp_get_inst_via_module(owner, port_id, 0);

	if (inst < 0) {
		pr_err("DPM: %s not valid module %s\n", __func__, owner->name);
		return -1;
	}

	return dp_register_dev_ext(inst, owner, port_id, dp_cb, NULL, flags);
}
EXPORT_SYMBOL(dp_register_dev);
#ifdef TOPAZ_CODE_ENABLE
static int remove_umt(int inst, const struct dp_umt_port *umt)
{
	struct umt_ops *ops = dp_get_umt_ops(inst);

	if (!ops)
		return -ENODEV;

	/* Disable UMT port */
	if (ops->umt_enable(ops->umt_dev, umt->ctl.id, 0))
		return DP_FAILURE;

	/* Release UMT port */
	return ops->umt_release(ops->umt_dev, umt->ctl.id);
}
#endif

static int32_t dp_deregister_dev(int inst, struct module *owner,
		u32 port_id, struct dp_dev_data *data,
		u32 flags)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, port_id);
	struct cqm_dp_alloc_complete_data cqm_data = {0};
	struct cqm_deq_ring_info *deq;
	int ret, i;

	if (port_info->status != PORT_DEV_REGISTERED) {
		DP_DEBUG(DP_DBG_FLAG_REG,
				"%s de-register dev failed due to wrong state %d\n",
				owner->name, port_info->status);
		return DP_FAILURE;
	}
	if (port_info->num_subif) {
		DP_DEBUG(DP_DBG_FLAG_REG,
				"%s de-register dev failed with %d subif registered\n",
				owner->name, port_info->num_subif);
		return DP_FAILURE;
	}

	dp_notifier_invoke(inst, port_info->dev, port_id, 0, NULL,
			DP_EVENT_DE_REGISTER_DEV);
	port_info->status = PORT_ALLOCATED;

	cqm_data.num_qid = port_info->num_resv_q;
	cqm_data.qid_base = port_info->qid_base;
	cqm_data.data = NULL;

	ret = dp_cqm_port_alloc_complete(owner, port_info, port_id,
			&cqm_data, DP_F_DEREGISTER);
	if (ret == DP_FAILURE)
		return ret;

	/* Clear RXIPPU FLAG, but if shared across multi dpid, check
	 * ref count
	 */
	for (i = 0; i < port_info->num_rx_ippu; i++) {
		deq = get_dp_deqring_info(inst, port_info->rx_ippu[i]->attr.ring.index);
		if (!deq->ref_cnt)
			deq->type &= ~CQM_DEQ_RING_RX_IPPU;
	}
	DP_CB(inst, dev_platform_set)(inst, port_id, data, flags);

	return DP_SUCCESS;
}

void dp_dump_dp_cb(dp_cb_t *dp_cb)
{
	if (!dp_cb)
		return;
	pr_cont("   dp_cb dump: 0x%px\n", dp_cb);
	pr_cont("      rx_fn                  : 0x%px\n", dp_cb->rx_fn);
	pr_cont("      stop_fn                : 0x%px\n", dp_cb->stop_fn);
	pr_cont("      restart_fn             : 0x%px\n", dp_cb->restart_fn);
	pr_cont("      get_subifid_fn         : 0x%px\n", dp_cb->get_subifid_fn);
	pr_cont("      reset_mib_fn           : 0x%px\n", dp_cb->reset_mib_fn);
	pr_cont("      get_mib_fn             : 0x%px\n", dp_cb->get_mib_fn);
	pr_cont("      dma_rx_irq             : 0x%px\n", dp_cb->dma_rx_irq);
	pr_cont("      aca_fw_stop            : 0x%px\n", dp_cb->aca_fw_stop);
#if IS_ENABLED(CONFIG_DPM_DATAPATH_CPUFREQ)
	pr_cont("      dp_coc_confirm_stat_fn : 0x%px\n", dp_cb->dp_coc_confirm_start_fn);
#endif
}

void dp_dump_mem_port_info(int indent, struct dp_mem_port_info *m)
{
	if (!m)
		return;
	pr_cont("      num_enq          : %d (max: %d)\n", m->num_enq, DP_NUM_DC_PKTENQ_RING);
	pr_cont("      num_req          : %d (max: %d)\n", m->num_req, DP_NUM_DC_BUFREQ_RING);
	pr_cont("      num_deq          : %d (max: %d)\n", m->num_deq, DP_NUM_DC_PKTDEQ_RING);
	pr_cont("      num_ret          : %d (max: %d)\n", m->num_ret, DP_NUM_DC_BUFRET_RING);
	pr_cont("      umt_enable       : %d\n", m->umt_enable);
	pr_cont("      umt_igp_interval : %d\n", m->umt_igp_interval);
	pr_cont("      umt_egp_interval : %d\n", m->umt_egp_interval);
	pr_cont("      enable_cqm_meta  : %d\n", m->enable_cqm_meta);

	dp_dump_pktenq_ring(7, m->enq, m->num_enq);
	dp_dump_bufreq_ring(7, m->req, m->num_req);
	dp_dump_pktdeq_ring(7, m->deq, m->num_deq, m->umt_enable);
	dp_dump_bufret_ring(7, m->ret, m->num_ret);
}

void dp_dump_dp_dev_data(int indent, struct dp_dev_data *data)
{
	if (!data)
		return;
	pr_cont("%*s   dp_dev_data: 0x%px\n", indent, "", data);
	pr_cont("%*s      flag_ops         : 0x%x\n", indent, "", data->flag_ops);
	pr_cont("%*s      max_subif        : %d\n", indent, "", data->max_subif);
	pr_cont("%*s      max_gpid         : %d\n", indent, "", data->max_gpid);
	pr_cont("%*s      num_resv_q       : %d\n", indent, "", data->num_resv_q);
	pr_cont("%*s      num_resv_sched   : %d\n", indent, "", data->num_resv_sched);
	pr_cont("%*s      qos_resv_q_base  : %d\n", indent, "", data->qos_resv_q_base);
	pr_cont("%*s      gpid_base        : %d\n", indent, "", data->gpid_base);
	pr_cont("%*s      bm_policy_res_id : %d\n", indent, "", data->bm_policy_res_id);
	pr_cont("%*s      tx_policy_base   : %d\n", indent, "", data->tx_policy_base);
	pr_cont("%*s      tx_policy_num    : %d\n", indent, "", data->tx_policy_num);
	pr_cont("%*s      opt_param.pcidata: 0x%px\n", indent, "", data->opt_param.pcidata);
	pr_cont("%*s      gpid_info dump:\n", indent, "");
	pr_cont("%*s         f_min_pkt_len: %d\n", indent, "", data->gpid_info.f_min_pkt_len);
	pr_cont("%*s         seg_en       : %d\n", indent, "", data->gpid_info.seg_en);
	pr_cont("%*s         min_pkt_len  : %d\n", indent, "", data->gpid_info.min_pkt_len);

	dp_dump_mem_port_info(7, data->mem_port);

	pr_cont("\n");
}

static int dp_set_dts_qos(int inst, int dp_port, int qos_id)
{
	struct cqm_deq_ring_info *dqinfo;
	struct dp_port_info *port_info;
	int i, index;

	port_info = get_dp_port_info(inst, dp_port);

	for (i = 0; i < port_info->num_deq; i++) {
		index = port_info->deq[i]->attr.ring.index;
		dqinfo = get_dp_deqring_info(inst, index);
		dqinfo->dts_qos = dp_get_qos_cfg(inst, dp_port,
						 port_info->alloc_flags, qos_id);
	}

	for (i = 0; i < port_info->num_tx_ippu; i++) {
		index = port_info->tx_ippu[i]->attr.ring.index;
		dqinfo = get_dp_deqring_info(inst, index);
		dqinfo->dts_qos = dp_get_inter_qos_cfg(inst, index);
	}

	for (i = 0; i < port_info->num_rx_ippu; i++) {
		index = port_info->rx_ippu[i]->attr.ring.index;
		dqinfo = get_dp_deqring_info(inst, index);
		dqinfo->dts_qos = dp_get_inter_qos_cfg(inst, index);
	}
	return 0;

}

static int dp_save_dc_info(int inst, int dpid, struct dp_dev_data_priv *p, u32 flags)
{
	struct dp_mem_port_info *m = p->udata->mem_port;
	struct dp_port_info *port_info = NULL;
	struct cqm_deq_ring_info *drinfo;
	struct cqm_enq_ring_info *erinfo;
	struct cqm_req_ring_info *rqinfo;
	struct cqm_ret_ring_info *rtinfo;
	int i, ring_idx;

	if (!(flags & DP_F_ACA))
		return DP_SUCCESS;

	if (!m) {
		pr_err("DPM: %s, DP_F_ACA flag is given but mem_port ptr is NULL\n", __func__);
		return DP_FAILURE;
	}

	port_info = get_dp_port_info(inst, dpid);
	/*store umt enable flag given by user, its at cqm deq port level*/
	port_info->umt_enable = m->umt_enable;

	if (port_info->num_deq) {
		/* As dp_alloc_cqm_port() does not return any deq rings
		 * for ACA
		 */
		pr_err("DPM: %s, for ACA device num_deq: %d (must be 0 at this point)\n",
				__func__, port_info->num_deq);
		return DP_FAILURE;
	}

	if (m->num_deq > DP_NUM_DC_PKTDEQ_RING) {
		pr_err("DPM: %s, wrong pktdeq ring: %d, max allowed: %d\n",
				__func__,
				m->num_deq, DP_NUM_DC_PKTDEQ_RING);
		return DP_FAILURE;
	}

	for (i = 0; i < m->num_deq; i++) {
		ring_idx = m->deq[i].attr.ring.index;
		drinfo = get_dp_deqring_info(inst, ring_idx);

		drinfo->deq = m->deq[i];
		drinfo->dp_port[dpid] = 1;
		drinfo->ring_id = ring_idx;
		drinfo->type = CQM_DEQ_RING_NORM;

		port_info->deq[i] = &drinfo->deq;
		port_info->num_deq++;
	}

	if (m->num_enq > DP_NUM_DC_PKTENQ_RING) {
		pr_err("DPM: %s, wrong pktenq ring: %d, max allowed: %d\n",
				__func__,
				m->num_enq, DP_NUM_DC_PKTENQ_RING);
		return DP_FAILURE;
	}
	for (i = 0; i < m->num_enq; i++) {
		ring_idx = m->enq[i].attr.ring.index;
		erinfo = get_dp_enqring_info(inst, ring_idx);

		erinfo->enq = m->enq[i];
		erinfo->dp_port[dpid] = 1;
		erinfo->ring_id = ring_idx;
		erinfo->type = CQM_ENQ_RING;

		port_info->enq[i] = &erinfo->enq;
		port_info->num_enq++;
	}

	if (p->num_ippu > DP_MAX_INTERM) {
		pr_err("DPM: %s, wrong num rx ippu ring: %d, max allowed: %d\n",
				__func__,
				p->num_ippu, DP_MAX_INTERM);
		return DP_FAILURE;
	}
	for (i = 0; i < p->num_ippu; i++) {
		ring_idx = p->ippu[i].attr.ring.index;
		drinfo = get_dp_deqring_info(inst, ring_idx);

		drinfo->deq = p->ippu[i];
		drinfo->dp_port[dpid] = 1;
		drinfo->ring_id = ring_idx;
		/*bitwise | cuz a ring id can be tx and rx ippu both*/
		drinfo->type |= CQM_DEQ_RING_RX_IPPU;

		port_info->rx_ippu[i] = &drinfo->deq;
		port_info->num_rx_ippu++;
	}

	if (m->num_req > DP_NUM_DC_BUFREQ_RING) {
		pr_err("DPM: %s, wrong bufreq ring: %d, max allowed: %d\n",
				__func__,
				m->num_req, DP_NUM_DC_BUFREQ_RING);
		return DP_FAILURE;
	}
	for (i = 0; i < m->num_req; i++) {
		ring_idx = m->req[i].attr.ring.index;
		rqinfo = get_dp_reqring_info(inst, ring_idx);

		rqinfo->req = m->req[i];
		rqinfo->dp_port[dpid] = 1;
		rqinfo->ring_id = ring_idx;
		rqinfo->type = CQM_REQ_RING;

		port_info->req[i] = &rqinfo->req;
		port_info->num_req++;
	}

	if (m->num_ret > DP_NUM_DC_BUFRET_RING) {
		pr_err("DPM: %s, wrong bufret ring: %d, max allowed: %d\n",
				__func__,
				m->num_ret, DP_NUM_DC_BUFRET_RING);
		return DP_FAILURE;
	}
	for (i = 0; i < m->num_ret; i++) {
		ring_idx = m->ret[i].attr.ring.index;
		rtinfo = get_dp_retring_info(inst, ring_idx);

		rtinfo->ret = m->ret[i];
		rtinfo->dp_port[dpid] = 1;
		rtinfo->ring_id = ring_idx;
		rtinfo->type = CQM_RET_RING;

		port_info->ret[i] = &rtinfo->ret;
		port_info->num_ret++;
	}

	return DP_SUCCESS;
}

int32_t dp_register_dev_ext(int inst, struct module *owner, u32 port_id,
		dp_cb_t *dp_cb, struct dp_dev_data *data,
		u32 flags)
{
	struct cqm_dp_alloc_complete_data cqm_data = {0};
	struct dp_dev_data_priv *priv_data;
	struct dp_port_info *port_info;
	struct dp_dev_data *tmp_data;
	int res = DP_FAILURE;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "dp_register_dev failed for datapath not init yet\n");
		return res;
	}
	tmp_data = dp_kzalloc(sizeof(*tmp_data), GFP_ATOMIC);
	if (!tmp_data)
		return res;
	if (!data)
		data = tmp_data;

	if (is_invalid_port(port_id) || is_invalid_inst(inst)) {
		kfree(tmp_data);
		return res;
	}

	if (!owner) {
		pr_err("DPM: owner NULL\n");
		kfree(tmp_data);
		return res;
	}

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (unlikely(dp_dbg_flag & DP_DBG_FLAG_REG)) {
		pr_cont("\n=====> DPM: %s input arguments:\n"
			"   inst: %d, owner: 0x%px(%s), port_id: %d, dp_cb: 0x%px, "
			"data: 0x%px, flags: 0x%x\n",
			__func__, inst, owner, owner->name, port_id, dp_cb,
			data, flags);
		dp_dump_dp_cb(dp_cb);
		dp_dump_dp_dev_data(0, data);
	}
#endif
	port_info = get_dp_port_info(inst, port_id);

	DP_LIB_LOCK(&dp_lock);
	if (flags & DP_F_DEREGISTER) {	/*de-register */
		res = dp_deregister_dev(inst, owner, port_id, data,
				flags);
		goto exit;
	}

	/*register a device */
	if (port_info->status != PORT_ALLOCATED) {
		pr_err("DPM: register dev fail for %s for unknown status:%d\n",
				owner->name, port_info->status);
		goto exit;
	}

	if (port_info->owner != owner) {
		pr_err("DPM: No matched owner(%s):0x%px->0x%px\n",
				owner->name, owner, port_info->owner);
		goto exit;
	}

	port_info->res_qid_base = data->qos_resv_q_base;
	port_info->num_resv_q = data->num_resv_q;
	port_info->dts_qos = dp_get_qos_cfg(inst, port_id,
					    port_info->alloc_flags,
					    data->qos_id);

	priv_data = dp_kzalloc(sizeof(*priv_data), GFP_ATOMIC);
	if (!priv_data) {
		pr_err("DPM: %s, failed to allocate memory\n", __func__);
		goto exit;
	}

	cqm_data.num_qid = data->num_resv_q;
	cqm_data.qid_base = data->qos_resv_q_base;
	cqm_data.data = priv_data;
	priv_data->udata = data;

	/* Register device to CQM */
	if (dp_cqm_port_alloc_complete(owner, port_info, port_id, &cqm_data,
				flags))
		goto free;

	port_info->qid_base = cqm_data.qid_base;

	if (dp_save_dc_info(inst, port_id, priv_data, port_info->alloc_flags)) {
		pr_err("DPM: failed dp_save_dc_info()\n");
		goto cqm_free;
	}

	/*Sanity check for port_info->num_deq*/
	if (!port_info->num_deq) {
		pr_err("DPM: %s %d Sanity check failed as num_deq is still 0\n",
				__func__, __LINE__);
		res = DP_FAILURE;
		goto cqm_free;
	}
	dp_set_dts_qos(inst, port_id, data->qos_id);

	DP_CB(inst, dev_platform_set)(inst, port_id, data, flags);

	port_info->status = PORT_DEV_REGISTERED;
	if (dp_cb)
		port_info->cb = *dp_cb;

	dp_notifier_invoke(inst, port_info->dev, port_id, 0, data,
			DP_EVENT_REGISTER_DEV);

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (unlikely(dp_dbg_flag & DP_DBG_FLAG_REG)) {
		pr_cont("\n<===== DPM: %s output arguments:\n"
			"   inst: %d, owner: 0x%px(%s), port_id: %d, dp_cb: 0x%px, "
			"data: 0x%px, flags: 0x%x\n",
			__func__, inst, owner, owner->name, port_id, dp_cb,
			data, flags);
		dp_dump_dp_cb(dp_cb);
		dp_dump_dp_dev_data(0, data);
	}
#endif
	res = DP_SUCCESS;
	goto free;

cqm_free:
	dp_cqm_port_alloc_complete(owner, port_info, port_id, &cqm_data,
				flags | DP_F_DEREGISTER);
free:
	kfree(priv_data);
exit:
	kfree(tmp_data);
	DP_LIB_UNLOCK(&dp_lock);
	return res;
}
EXPORT_SYMBOL(dp_register_dev_ext);

void dp_dump_dp_subif_data(struct dp_subif_data *data)
{
	if (!data)
		return;

	DP_DUMP("   dp_subif_data dump: 0x%px\n", data);
	DP_DUMP("   ------------------------\n");
	DP_DUMP("      deq_ring_idx        : %hhd\n", data->deq_ring_idx);
	DP_DUMP("      flag_ops            : 0x%x\n", (u32)data->flag_ops);
	DP_DUMP("      q_id                : %d\n", data->q_id);
	DP_DUMP("      ctp_dev             : 0x%px(%s)\n", data->ctp_dev,
		data->ctp_dev ? data->ctp_dev->name : "null");
	DP_DUMP("      rx_fn               : 0x%px\n", data->rx_fn);
	DP_DUMP("      get_subifid_fn      : 0x%px\n", data->get_subifid_fn);
	DP_DUMP("      f_policy            : %hhu\n", data->f_policy);
	DP_DUMP("      tx_pkt_size         : %hu\n", data->tx_pkt_size);
	DP_DUMP("      gpid_tx_info:\n"
		"         f_min_pkt_len: %d\n"
		"         seg_en       : %d\n"
		"         min_pkt_len  : %d\n",
		data->gpid_tx_info.f_min_pkt_len,
		data->gpid_tx_info.seg_en,
		data->gpid_tx_info.min_pkt_len);
	DP_DUMP("      tx_policy_base      : %hu\n", data->tx_policy_base);
	DP_DUMP("      tx_policy_num       : %hhu\n", data->tx_policy_num);
	DP_DUMP("      rx_policy_base      : %hu\n", data->rx_policy_base);
	DP_DUMP("      rx_policy_num       : %hhu\n", data->rx_policy_num);
	DP_DUMP("      txin_ring_size      : %d\n", data->txin_ring_size);
	DP_DUMP("      txin_ring_phy_addr  : 0x%px\n",
		data->txin_ring_phy_addr);
	DP_DUMP("      credit_add_phy_addr : 0x%px\n",
		data->credit_add_phy_addr);
	DP_DUMP("      credit_left_phy_addr: 0x%px\n",
		data->credit_left_phy_addr);
	DP_DUMP("      num_deq_ring        : %hu\n", data->num_deq_ring);
	DP_DUMP("      rx_en_flag          : %u\n", data->rx_en_flag);
	DP_DUMP("      bm_policy_res_id    : %u\n", data->bm_policy_res_id);
}

void dp_dump_dp_subif(dp_subif_single_t *subif_id)
{
	if (!subif_id)
		return;

	DP_DUMP("   dp_subif_t dump: 0x%px\n", subif_id);
	DP_DUMP("   -----------------------\n");
	DP_DUMP("      dp_subif_port_cmn:\n");
	DP_DUMP("         port_id      : %d\n", subif_id->port_id);
	DP_DUMP("         subif        : %d\n", subif_id->subif);
	DP_DUMP("         gpid         : %d\n", subif_id->gpid);
}

/* if subif_id->subif < 0: Dynamic mode
 * else subif is provided by caller itself
 * Note: for IPOA/PPPOA, dev is NULL and subif_name is dummy string.
 *       in this case, dev->name may not be subif_name
 */
int32_t dp_register_subif_ext(int inst, struct module *owner,
		struct net_device *dev,
		char *subif_name, dp_subif_single_t *subif_id_one,
		/*device related info*/
		struct dp_subif_data *data, u32 flags)
{
	int res = DP_FAILURE;
	int n, port_id, old_subif = -1;
	struct dp_port_info *port_info;
	struct dp_subif_data tmp_data = {0};
	dp_subif_t *subif_id_sync;
	dp_get_netif_subifid_fn_t subifid_fn = NULL;
	/* it seems only use first entry here although allocate 2
	 * Here alloct 2 is just for lower level API dp_sync_subifid requirement
	 */
	subif_id_sync = dp_kzalloc(sizeof(*subif_id_sync) * 2, GFP_ATOMIC);
	if (!subif_id_sync) {
		DP_LIB_UNLOCK(&dp_lock);
		res = DP_ERR_MEM;
		goto EXIT;
	}

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_REG,
				"dp_register_subif fail for datapath not init yet\n");
		res = DP_ERR_INIT_FAIL;
		dp_print_err_info(res);
		return DP_FAILURE;
	}

	if (!subif_id_one || !owner) {
		DP_DEBUG(DP_DBG_FLAG_REG, "Failed subif_id 0x%px owner 0x%px\n",
				subif_id_one, owner);
		res = DP_ERR_INVALID_SUBIF;
		dp_print_err_info(res);
		return DP_FAILURE;
	}

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (unlikely(dp_dbg_flag & DP_DBG_FLAG_REG)) {
		DP_DUMP("\n=====> DPM: %s input arguments:\n", __func__);
		DP_DUMP("   %s: %d, %s: 0x%px(%s), %s: 0x%px(%s), %s: %s,",
			"inst", inst,
			"owner", owner, owner->name,
			"dev", dev, dev ? dev->name : "NULL",
			"subif_name", subif_name ? subif_name : "NULL");
		DP_DUMP("   %s: 0x%px, %s: 0x%px, %s: 0x%x\n",
			"subif_id", subif_id_one,
			"data", data,
			"flags", flags);

		dp_dump_dp_subif(subif_id_one);
		dp_dump_dp_subif_data(data);
	}
#endif
	old_subif = subif_id_one->subif;
	port_id = subif_id_one->port_id;

	subif_id_sync->subif = subif_id_one->subif;
	subif_id_sync->port_id = subif_id_one->port_id;
	subif_id_sync->inst = inst;
	if (is_invalid_port(port_id) || is_invalid_inst(inst)) {
		res = DP_ERR_INVALID_PORT_ID;
		DP_DEBUG(DP_DBG_FLAG_REG,
				"dp_register_subif fail, either inst: %d or port_id: %d is invalid\n"
				, inst, port_id);
		return DP_FAILURE;
	}
	port_info = get_dp_port_info(inst, port_id);

	if ((!dev && !is_dsl(port_info)) || !subif_name) {
		DP_DEBUG(DP_DBG_FLAG_REG, "Wrong dev=0x%px, subif_name=0x%px\n",
				dev, subif_name);
		return DP_FAILURE;
	}
	if (!data)
		data = &tmp_data;
	DP_LIB_LOCK(&dp_lock);
	if (port_info->owner != owner) {
		DP_DEBUG(DP_DBG_FLAG_REG,
				"Unregister subif fail:Not matching:0x%px(%s)->0x%px(%s)\n",
				owner, owner->name, port_info->owner,
				port_info->owner->name);
		DP_LIB_UNLOCK(&dp_lock);
		return res;
	}

	if (dev)
		DP_DEBUG(DP_DBG_FLAG_OPS, "%s before %s%s netdev_ops=0x%px\n",
				dev->name,
				flags & DP_F_DEREGISTER ? "de-register" : "register",
				"_subif",
				dev->netdev_ops);

	if (flags & DP_F_DEREGISTER) /*de-register */
		res = dp_deregister_subif_private(inst, owner, dev, subif_name,
				subif_id_sync, data, flags);
	else { /*register */
		res = dp_register_subif_private(inst, owner, dev, subif_name,
				subif_id_sync, data, flags);
		/* subif value may be updated and need return to caller */
		subif_id_one->subif = subif_id_sync->subif;
		subif_id_one->gpid = subif_id_sync->gpid;
	}
	if (res) {
		DP_LIB_UNLOCK(&dp_lock);
		goto EXIT;
	}

	n = GET_VAP(subif_id_sync->subif, port_info->vap_offset,
			port_info->vap_mask);
	subifid_fn = get_dp_port_subif(port_info, n)->get_subifid_fn;

	res = dp_sync_subifid(dev, subif_name, subif_id_sync, data, flags);
	if (res) {
		DP_LIB_UNLOCK(&dp_lock);
		kfree(subif_id_sync);
		goto EXIT;
	}
	DP_LIB_UNLOCK(&dp_lock);
	if (!res)
		res = dp_sync_subifid_priv(dev, subif_name, subif_id_sync, data,
				flags, subifid_fn, 1);
EXIT:
	kfree(subif_id_sync);
	if (unlikely(res)) {
		dp_print_err_info(res);
		res = DP_FAILURE;
	}
	if (dev)
		DP_DEBUG(DP_DBG_FLAG_OPS, "%s after %s%s netdev_ops=0x%px\n",
				dev->name,
				flags & DP_F_DEREGISTER ? "de-register" : "register",
				"_subif",
				dev->netdev_ops);
#ifdef TOPAZ_CODE_ENABLE
	trace_dp_register_subif(res, inst, owner, dev, subif_name, subif_id,
			data, flags, old_subif);

	dp_dump_debugfs_all(port_id,
			GET_VAP(subif_id->subif, port_info->vap_offset, port_info->vap_mask));
#endif
#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (unlikely(dp_dbg_flag & DP_DBG_FLAG_REG)) {
		DP_DUMP("\n<===== DPM: %s output arguments:\n", __func__);
		DP_DUMP("   %s: %d, %s: 0x%px(%s), %s: 0x%px(%s), %s: %s,",
			"inst", inst,
			"owner", owner, owner->name,
			"dev", dev, dev ? dev->name : "NULL",
			"subif_name", subif_name ? subif_name : "NULL");
		DP_DUMP("   %s: 0x%px, %s: 0x%px, %s: 0x%x\n",
			"subif_id", subif_id_one,
			"data", data,
			"flags", flags);

		dp_dump_dp_subif(subif_id_one);
		dp_dump_dp_subif_data(data);
	}
#endif
	return res;
}
EXPORT_SYMBOL(dp_register_subif_ext);

int32_t dp_register_subif(struct module *owner, struct net_device *dev,
		char *subif_name, dp_subif_single_t *subif_id,
		u32 flags)
{
	int inst;
	struct dp_subif_data data = {0};

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT,
				"%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	if (!subif_id || !owner || is_invalid_port(subif_id->port_id)) {
		DP_DEBUG(DP_DBG_FLAG_REG, "fail owner 0x%px subif_id 0x%px\n",
				subif_id, owner);
		return DP_FAILURE;
	}
	inst = dp_get_inst_via_module(owner, subif_id->port_id, 0);
	if (inst < 0) {
		pr_err("DPM: wrong inst for owner=%s with ep=%d\n", owner->name,
				subif_id->port_id);
		return DP_FAILURE;
	}
	return dp_register_subif_ext(inst, owner, dev, subif_name,
			subif_id, &data, flags);
}
EXPORT_SYMBOL(dp_register_subif);

#ifdef TOPAZ_CODE_ENABLE //Check if it is used by anyother driver/code, as reinsertion is not valid in topaz, may need to remove.
/* Register sub interface for special devices (CPU or reinsertion port)
 * Corresponding flags have to be set in order to be registered inside this
 * function.
 */
int32_t dp_register_subif_spl_dev(int inst, struct net_device *dev,
		char *subif_name, dp_subif_t *subif_id,
		struct dp_subif_data *data, u32 flags)
{
	int res = DP_FAILURE;

	if (!dp_late_init())
		return DP_FAILURE;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT,
				"%s datapath is not yet initialized\n",
				__func__);
		dp_print_err_info(DP_ERR_INIT_FAIL);
		return DP_FAILURE;
	}

	if (!subif_id) {
		DP_DEBUG(DP_DBG_FLAG_REG, "%s invalid subif_id\n",
				__func__);
		dp_print_err_info(DP_ERR_INVALID_SUBIF);
		return DP_FAILURE;
	}

	if (!data) {
		DP_DEBUG(DP_DBG_FLAG_REG, "%s invalid subif data\n",
				__func__);
		dp_print_err_info(DP_ERR_NULL_DATA);
		return DP_FAILURE;
	}

	DP_DEBUG(DP_DBG_FLAG_REG,
			"%s:dev=%s(p=0x%px) %s=%s port_id=%d subif=%d(%s)\n",
			(flags & DP_F_DEREGISTER) ?
			"unregister subif:" : "register subif",
			dev ? dev->name : "NULL",
			dev, "subif_name", subif_name,
			subif_id->port_id, subif_id->subif,
			(subif_id->subif < 0) ? "dynamic" : "fixed");

	if (!(data->flag_ops & (DP_SUBIF_CPU | DP_SUBIF_REINSERT))) {
		pr_err("DPM: %s unsupported flags dev=%s, flag_ops=0x%x\n",
				__func__, dev ? dev->name : "NULL", data->flag_ops);
		return DP_FAILURE;
	}

	DP_LIB_LOCK(&dp_lock);
	if (flags & DP_F_DEREGISTER)
		res = dp_del_subif_spl_dev(inst, dev, subif_name,
				subif_id, data, flags);
	else
		res = dp_add_subif_spl_dev(inst, dev, subif_name,
				subif_id, data, flags);
	DP_LIB_UNLOCK(&dp_lock);

	return res;
}
EXPORT_SYMBOL(dp_register_subif_spl_dev);
#endif

dp_subif_t *dp_get_netif_subifid(struct net_device *netif, struct sk_buff *skb,
		void *subif_data, u8 dst_mac[DP_MAX_ETH_ALEN], u32 flags)
{
	dp_get_netif_subifid_fn_t subifid_fn_t;
	struct dp_subif_cache *dp_subif;
	dp_subif_t *subif;
	int res;
	u32 idx;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT,
				"failed: datapath not initialized yet\n");
		return NULL;
	}
	idx = dp_subif_hash(netif);
	//TODO handle DSL case in future
	rcu_read_lock_bh();
	dp_subif = dp_subif_lookup_safe(&dp_subif_list[idx], netif, subif_data);
	if (!dp_subif) {
		DP_DEBUG(DP_DBG_FLAG_DBG, "Failed dp_subif_lookup: %s\n",
				netif ? netif->name : "NULL");
		rcu_read_unlock_bh();
		return NULL;
	}
	subif = kmem_cache_alloc(subif_cache, GFP_ATOMIC);
	if (!subif)
	{
		DP_DEBUG(DP_DBG_FLAG_DBG, "Failed to kmem_cache_alloc dp_subif\n");
		rcu_read_unlock_bh();
		return NULL;
	}
	dp_memcpy(subif, &dp_subif->subif, sizeof(*subif));
	subifid_fn_t = dp_subif->subif_fn;
	rcu_read_unlock_bh();
	if (subifid_fn_t) {
		/* subif->subif will be set by callback api itself */
		res = subifid_fn_t(netif, skb, subif_data, dst_mac, subif,
				   flags);
		if (res != 0) {
			DP_DEBUG(DP_DBG_FLAG_DBG,
				 "dp_subif->subif_fn return fail\n");
			dp_free_netif_subifid(subif);
			return NULL;
		}
	}
	/* Note: subif buffer should be freed by caller */
	return subif;
}
EXPORT_SYMBOL(dp_get_netif_subifid);

void dp_free_netif_subifid(dp_subif_t *subif)
{
	if (subif)
		kmem_cache_free(subif_cache, subif);
}
EXPORT_SYMBOL(dp_free_netif_subifid);

/* check whether this netif is registered DPM device or not */
bool dp_valid_netif(const struct net_device *netif)
{
	u32 idx;
	int res = false;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT,
				"%s failed: datapath not initialized yet\n",
				__func__);
		return res;
	}
	if (!netif)
		return false;

	idx = dp_subif_hash((struct net_device *)netif);
	rcu_read_lock_bh();
	if (dp_subif_lookup_safe(&dp_subif_list[idx], netif, NULL)) {
		rcu_read_unlock_bh();
		return true;
	}
	rcu_read_unlock_bh();
	return res;
}

bool dp_is_pmapper_check(struct net_device *dev)
{
	dp_subif_t *subif;

	subif = dp_get_netif_subifid(dev, NULL, NULL, NULL, 0);
	if (!subif) {
		netdev_err(dev, "can not get subif\n");
		return false;
	}
	if (subif->flag_pmapper) {
		dp_free_netif_subifid(subif);
		return true;
	}
	dp_free_netif_subifid(subif);
	return false;
}
EXPORT_SYMBOL(dp_is_pmapper_check);

/*Note:
 * get subif according to netif.
 * Use subif->port_id passed by caller to get port_info
 */
int32_t dp_get_subifid_for_update(int inst, struct net_device *netif,
		dp_subif_t *subif, u32 flags)
{
	int res = DP_FAILURE;
	int i;
	int port_id;
	u8 num = 0;
	struct dp_subif_info **sif_ptr_arr = NULL;
	struct dp_port_info *p_info;
	bool max_subif_over = false;

	if (!subif) {
		DP_DEBUG(DP_DBG_FLAG_REG,
				"%s failed:subif is NULL\n", __func__);
		return DP_ERR_NULL_DATA;
	}
	port_id = subif->port_id;
	if (port_id < 0) {
		DP_DEBUG(DP_DBG_FLAG_DBG,
				"failed: %s\n" ,
				netif ? netif->name : "NULL");
		res = DP_ERR_SUBIF_NOT_FOUND;
		goto EXIT;
	}

	p_info = get_dp_port_info(inst, port_id);

	/* For DSL ATM case netif will be NULL with valid port id */
	if ((!netif) && (!is_dsl(p_info))) {
		DP_DEBUG(DP_DBG_FLAG_REG,
				"failed: netif null\n");
		return DP_ERR_NULL_DATA;
	}

	sif_ptr_arr = dp_kzalloc(sizeof(struct dp_subif_info *) * DP_MAX_SUBIF_PER_DEV,
				 GFP_ATOMIC);
	if (!sif_ptr_arr)
		return DP_ERR_MEM;

	subif->flag_pmapper = 0;

	/*search sub-interfaces/VAP */
	for (i = 0; i < p_info->max_subif; i++) {
		struct dp_subif_info *sif = get_dp_port_subif(p_info, i);

		/* FOR DSL ATM case when netif is NULL, no need subif info */
		if (is_dsl(p_info) && (!netif))
			break;

		if (!sif->flags)
			continue;

		if (sif->ctp_dev == netif) { /*for PON pmapper case*/
			if (num > 0) {
				pr_err("DPM: Multiple same ctp_dev exist\n");
				goto EXIT;
			}
			sif_ptr_arr[num] = sif;
			sif_ptr_arr[num]->subif_flag = sif->subif_flag;
			subif->num_qid = sif->num_qid;
			dp_memcpy(subif->def_qid_list, sif->def_qid_list,
				  sizeof(subif->def_qid_list));
			dp_memcpy(&subif->subif_cmn_start, &sif->subif_cmn_start,
				  &subif->subif_cmn_end - &subif->subif_cmn_start);
			res = DP_SUCCESS;
			num++;
			break;
		}
		if (sif->netif == netif) {
			if (num >= DP_MAX_SUBIF_PER_DEV) {
				pr_err("DPM: %s: Why CTP over %d\n",
						netif->name,
						DP_MAX_SUBIF_PER_DEV);
				max_subif_over = true;
				goto UPDATE;
			}
			/* some dev may have multiple
			 * subif,like pon
			 */
			sif_ptr_arr[num] = sif;
			dp_memcpy(&subif->subif_cmn_start, &sif->subif_cmn_start,
				  &subif->subif_cmn_end - &subif->subif_cmn_start);
			sif_ptr_arr[num]->subif_flag = sif->subif_flag;
			subif->num_qid = sif->num_qid;
			dp_memcpy(subif->def_qid_list, sif->def_qid_list,
				  sizeof(subif->def_qid_list));
			if (sif->ctp_dev)
				subif->flag_pmapper = 1;
			res = DP_SUCCESS;
			num++;
		}
		if (num != 0)
			continue;
	}

UPDATE:
	subif->inst = inst;
	dp_memcpy(&subif->port_cmn_start, &p_info->port_cmn_start,
		  &subif->port_cmn_end - &subif->port_cmn_start);
	subif->subif_num = num;
	for (i = 0; i < num; i++) {
		subif->subif_list[i] = sif_ptr_arr[i]->subif;
		subif->subif_flag[i] = sif_ptr_arr[i]->subif_flag;
		subif->gpid_list[i] = sif_ptr_arr[i]->gpid;
		dp_memcpy(subif->dfl_eg_sess[i], sif_ptr_arr[i]->dfl_eg_sess,
			  sizeof(sif_ptr_arr[i]->dfl_eg_sess[0]));
	}
	if (!max_subif_over)
		res = DP_SUCCESS;
EXIT:
	kfree(sif_ptr_arr);
	return res;
}

dp_subif_t *dp_get_port_subitf_via_dev(struct net_device *dev)
{
	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return NULL;
	}

	return dp_get_netif_subifid(dev, NULL, NULL, NULL, 0);
}
EXPORT_SYMBOL(dp_get_port_subitf_via_dev);

dp_subif_t *dp_get_port_subitf_via_ifname(char *ifname)
{
	dp_subif_t *subif;
	struct net_device *dev;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return NULL;
	}

	if (!ifname)
		return NULL;
	dev = dev_get_by_name(&init_net, ifname);
	if (!dev)
		return NULL;
	subif = dp_get_port_subitf_via_dev(dev);
	dev_put(dev);
	return subif;
}
EXPORT_SYMBOL(dp_get_port_subitf_via_ifname);

struct module *dp_get_module_owner(int ep)
{
	int inst = 0; /*here hardcode for PPA only */

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed for dp not init yet\n", __func__);
		return NULL;
	}

	if (ep >= 0 && ep < get_dp_prop_info(inst)->cap.max_num_dp_ports)
		return get_dp_port_info(inst, ep)->owner;

	return NULL;
}
EXPORT_SYMBOL(dp_get_module_owner);

/*if subif->vap == -1, it means all vap */
void dp_clear_mib(dp_subif_t *subif, u32 flag)
{
	int i, j, start_vap, end_vap;
	dp_reset_mib_fn_t reset_mib_fn;
	struct dp_port_info *port_info;

	if (!subif || is_invalid_port(subif->port_id)) {
		DP_DEBUG(DP_DBG_FLAG_DBG, "Wrong subif\n");
		return;
	}

	i = subif->port_id;
	port_info = get_dp_port_info(subif->inst, i);

	if (subif->subif == -1) {
		start_vap = 0;
		end_vap = port_info->num_subif;
	} else {
		start_vap = GET_VAP(subif->subif, port_info->vap_offset,
				    port_info->vap_mask);
		end_vap = start_vap + 1;
	}

	for (j = start_vap; j < end_vap; j++) {
		struct dp_subif_info *sif = get_dp_port_subif(port_info, i);
		struct dev_mib *mib = get_dp_port_subif_mib(sif);

		STATS_SET(port_info->tx_err_drop, 0);
		STATS_SET(port_info->rx_err_drop, 0);
		dp_memset(mib, 0, sizeof(struct dev_mib));
		reset_mib_fn = port_info->cb.reset_mib_fn;

		if (reset_mib_fn)
			reset_mib_fn(subif, 0);
	}
}

void dp_clear_all_mib_inside(u32 flag)
{
	dp_subif_t *subif;
	int i;

	subif = dp_kzalloc(sizeof(*subif), GFP_ATOMIC);
	if (!subif)
		return;

	for (i = 0; i < MAX_DP_PORTS; i++) {
		subif->port_id = i;
		subif->subif = -1;
		dp_clear_mib(subif, flag);
	}
	kfree(subif);
}

int dp_get_drv_mib(dp_subif_t *subif, dp_drv_mib_t *mib, u32 flag)
{
	dp_get_mib_fn_t get_mib_fn;
	dp_drv_mib_t tmp;
	int i, vap;
	struct dp_port_info *port_info;
	struct dp_subif_info *sif;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT,
			 "failed for datapath not init yet\n");
		return DP_FAILURE;
	}

	if (!subif || !mib)
		return -1;
	dp_memset(mib, 0, sizeof(*mib));
	port_info = get_dp_port_info(subif->inst, subif->port_id);
	vap = GET_VAP(subif->subif, port_info->vap_offset,
		      port_info->vap_mask);
	get_mib_fn = port_info->cb.get_mib_fn;

	if (!get_mib_fn)
		return -1;

	if (!(flag & DP_F_STATS_SUBIF)) {
		/*get all VAP's  mib counters if it is -1 */
		for (i = 0; i < port_info->num_subif; i++) {
			sif = get_dp_port_subif(port_info, i);
			if (!sif->flags)
				continue;

			subif->subif = sif->subif;
			dp_memset(&tmp, 0, sizeof(tmp));
			get_mib_fn(subif, &tmp, flag);
			mib->rx_drop_pkts += tmp.rx_drop_pkts;
			mib->rx_error_pkts += tmp.rx_error_pkts;
			mib->tx_drop_pkts += tmp.tx_drop_pkts;
			mib->tx_error_pkts += tmp.tx_error_pkts;
		}
	} else {
		sif = get_dp_port_subif(port_info, vap);
		if (sif->flags)
			get_mib_fn(subif, mib, flag);
	}

	return 0;
}

int dp_get_netif_stats(struct net_device *dev, dp_subif_single_t *subif_id,
		       struct rtnl_link_stats64 *stats, u32 flags)
{
	int res = 0;
#ifdef CONFIG_DPM_MIB
	dp_subif_t *subif;
	int (*get_mib)(dp_subif_single_t *subif_id, void *priv,
		       struct rtnl_link_stats64 * stats,
		       u32 flags);

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	subif = dp_kzalloc(sizeof(*subif), GFP_ATOMIC);
	if (!subif)
		return DP_FAILURE;
	if (subif_id) {
		*subif = *subif_id;
	} else if (dev) {
		res = dp_get_port_subitf_via_dev(dev, subif);
		if (res) {
			DP_DEBUG(DP_DBG_FLAG_MIB,
				 "fail:%s not registered yet to datapath\n",
				 dev->name);
			kfree(subif);
			return DP_FAILURE;
		}
	} else {
		DP_DEBUG(DP_DBG_FLAG_MIB,
			 "dev/subif_id both NULL\n");
		kfree(subif);
		return DP_FAILURE;
	}
	get_mib = get_dp_prop_info(subif->inst)->dp_get_port_vap_mib;
	if (!get_mib) {
		kfree(subif);
		return DP_FAILURE;
	}

	res = get_mib(subif, NULL, stats, flags);
	kfree(subif);
#endif
	return res;
}
EXPORT_SYMBOL(dp_get_netif_stats);

int dp_clear_netif_stats(struct net_device *dev, dp_subif_single_t *subif_id,
			 u32 flag)
{
#ifdef CONFIG_DPM_MIB
	dp_subif_t *subif;
	int (*clear_netif_mib_fn)(dp_subif_t *subif, void *priv, u32 flag);
	int i, res;
	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	if (subif_id) {
		clear_netif_mib_fn =
			get_dp_prop_info(subif_id->inst)->dp_clear_netif_mib;
		if (!clear_netif_mib_fn)
			return -1;
		return clear_netif_mib_fn(subif_id, NULL, flag);
	}
	subif = dp_kzalloc(sizeof(*subif), GFP_ATOMIC);
	if (!subif)
		return DP_FAILURE;
	if (dev) {
		if (dp_get_port_subitf_via_dev(dev, subif)) {
			DP_DEBUG(DP_DBG_FLAG_MIB, "not register to %s\n",
				 dev->name);
			kfree(subif);
			return -1;
		}
		clear_netif_mib_fn =
			get_dp_prop_info(subif->inst)->dp_clear_netif_mib;
		if (!clear_netif_mib_fn) {
			kfree(subif);
			return -1;
		}
		res = clear_netif_mib_fn(subif, NULL, flag);
		kfree(subif);
		return res;
	}
	/*clear all */
	for (i = 0; i < DP_MAX_INST; i++) {
		clear_netif_mib_fn = get_dp_prop_info(i)->dp_clear_netif_mib;
		if (!clear_netif_mib_fn)
			continue;
		clear_netif_mib_fn(NULL, NULL, flag);
	}
	kfree(subif);
#endif
	return 0;
}
EXPORT_SYMBOL(dp_clear_netif_stats);

/*\brief Datapath Manager Pmapper Configuration Set
 *\param[in] dev: network device point to set pmapper
 *\param[in] mapper: buffer to get pmapper configuration
 *\param[in] flag: reserve for future
 *\return Returns 0 on succeed and -1 on failure
 *\note  for pcp mapper case, all 8 mapping must be configured properly
 *       for dscp mapper case, all 64 mapping must be configured properly
 *       def ctp will match non-vlan and non-ip case
 *	For drop case, assign CTP value == DP_PMAPPER_DISCARD_CTP
 */
int dp_set_pmapper(struct net_device *dev, struct dp_pmapper *mapper, u32 flag)
{
	int inst, ret, i;
	dp_subif_t *subif;
	struct dp_pmapper *map = NULL;
	int res = DP_FAILURE;
	struct inst_info *dp_info;

	if (!dev || !mapper) {
		pr_err("DPM: dev or mapper is NULL\n");
		return DP_FAILURE;
	}

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "Failed for datapath not init yet\n");
		return DP_FAILURE;
	}
	if (mapper->mode >= DP_PMAP_MAX) {
		pr_err("DPM: mapper->mode(%d) out of range %d\n",
		       mapper->mode, DP_PMAP_MAX);
		return DP_FAILURE;
	}
	/* get the subif from the dev */
	subif = dp_get_netif_subifid(dev, NULL, NULL, NULL, 0);
	if (!subif) {
		pr_err("DPM: Fail to get subif:dev=%s ret=%d flag_pmap=%d\n",
		       dev->name, ret, subif->flag_pmapper);
		return DP_FAILURE;
	}
	inst = subif->inst;

	dp_info = get_dp_prop_info(inst);

	if (!dp_info->dp_set_gsw_pmapper) {
		pr_err("DPM: Set pmapper is not supported\n");
		dp_free_netif_subifid(subif);
		return DP_FAILURE;
	}

	map = dp_kzalloc(sizeof(*map), GFP_ATOMIC);
	if (!map) {
		dp_free_netif_subifid(subif);
		return DP_FAILURE;
	}
	dp_memcpy(map, mapper, sizeof(*map));
	switch (mapper->mode) {
	case DP_PMAP_PCP:
	case DP_PMAP_DSCP:
#ifdef TOPAZ_CODE_ENABLE
		map->mode = GSW_PMAPPER_MAPPING_PCP;
		break;
	case DP_PMAP_DSCP_ONLY:
		map->mode = GSW_PMAPPER_MAPPING_DSCP;
#endif
		break;
	default:
		pr_err("DPM: Unknown mapper mode: %d\n", map->mode);
		goto EXIT;
	}
	/* workaround in case caller forget to set to default ctp */
	if (mapper->mode == DP_PMAP_PCP)
		for (i = 0; i < DP_PMAP_DSCP_NUM; i++)
			map->dscp_map[i] = mapper->def_ctp;

	//ret = dp_info->dp_set_gsw_pmapper(inst, subif->port_id, map,
	//				  flag);
	//if (ret == DP_FAILURE) {
	//	pr_err("DPM: Failed to set mapper\n");
	//	goto EXIT;
	//}

	res = DP_SUCCESS;
EXIT:
	kfree(map);
	dp_free_netif_subifid(subif);
	return res;
}
EXPORT_SYMBOL(dp_set_pmapper);

/*\brief Datapath Manager Pmapper Configuration Get
 *\param[in] dev: network device point to set pmapper
 *\param[out] mapper: buffer to get pmapper configuration
 *\param[in] flag: reserve for future
 *\return Returns 0 on succeed and -1 on failure
 *\note  for pcp mapper case, all 8 mapping must be configured properly
 *       for dscp mapper case, all 64 mapping must be configured properly
 *       def ctp will match non-vlan and non-ip case
 *	 For drop case, assign CTP value == DP_PMAPPER_DISCARD_CTP
 */
//int dp_get_pmapper(struct net_device *dev, struct dp_pmapper *mapper, u32 flag)
//{
//	int inst, ret;
//	dp_subif_t *subif;
//	struct inst_info *dp_info;
//
//	if (!dev || !mapper) {
//		pr_err("DPM: The parameter dev or mapper can not be NULL\n");
//		return DP_FAILURE;
//	}
//
//	if (unlikely(!dp_init_ok)) {
//		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "Failed for datapath not init yet\n");
//		return DP_FAILURE;
//	}
//
//	subif = dp_kzalloc(sizeof(*subif), GFP_ATOMIC);
//	if (!subif)
//		return DP_FAILURE;
//
//	/*get the subif from the dev*/
//	ret = dp_get_netif_subifid(dev, NULL, NULL, NULL, subif, 0);
//	if (ret == DP_FAILURE) {
//		kfree(subif);
//		pr_err("DPM: Can not get the subif from the dev\n");
//		return DP_FAILURE;
//	}
//	inst = subif->inst;
//
//	dp_info = get_dp_prop_info(inst);
//
//	if (!dp_info->dp_get_gsw_pmapper) {
//		kfree(subif);
//		pr_err("DPM: Get pmapper is not supported\n");
//		return DP_FAILURE;
//	}
//
//	/* init the subif into the dp_port_info*/
//	/* call the switch api to get the HW*/
//	ret = dp_info->dp_get_gsw_pmapper(inst, bport, subif->port_id, mapper,
//					  flag);
//	if (ret == DP_FAILURE) {
//		kfree(subif);
//		pr_err("DPM: Failed to get mapper\n");
//		return DP_FAILURE;
//	}
//	kfree(subif);
//	return ret;
//}
//EXPORT_SYMBOL(dp_get_pmapper);

int32_t dp_rx(struct sk_buff *skb, u32 flags)
{
	struct sk_buff *next;
	int res = -1;
	int inst = 0;
	struct inst_info *dp_info = get_dp_prop_info(inst);

	if (unlikely(!dp_init_ok)) {
		while (skb) {
			next = skb->next;
			skb->next = 0;
			dev_kfree_skb_any(skb);
			skb = next;
		}
	}

	while (skb) {
		next = skb->next;
		skb->next = 0;
		res = dp_info->dp_rx(skb, flags);
		skb = next;
	}

	return res;
}
EXPORT_SYMBOL(dp_rx);

int dp_lan_wan_bridging(int port_id, struct sk_buff *skb)
{
	dp_subif_single_t *subif;
	struct net_device *dev;
	static int lan_port = 4;
	int inst = 0, ret = DP_SUCCESS;
	struct dp_subif_info *sif;

	if (!skb)
		return DP_FAILURE;

	skb_pull(skb, 8);	/*remove pmac */

	subif = dp_kzalloc(sizeof(*subif), GFP_ATOMIC);
	if (!subif)
		return DP_FAILURE;

	if (port_id == 15) {
		/*recv from WAN and forward to LAN via lan_port */
		subif->port_id = lan_port;	/*send to last lan port */
		subif->subif = 0;
	} else if (port_id <= 6) { /*recv from LAN and forward to WAN */
		subif->port_id = 15;
		subif->subif = 0;
		lan_port = port_id;	/*save lan port id */
	} else {
		dev_kfree_skb_any(skb);
		kfree(subif);
		return DP_FAILURE;
	}

	sif = get_dp_port_subif(get_dp_port_info(inst, subif->port_id), 0);
	dev = sif->netif;

	if (!sif->flags || !dev) {
		dev_kfree_skb_any(skb);
		kfree(subif);
		return DP_FAILURE;
	}

	//((struct adp_tx_desc_3 *)&skb->DW3)->port = subif->port_id; //TODO: VBOLLA: No need to fill DPM port_id in ADP
	((struct adp_tx_desc_0 *)&skb->DW0)->all |= ADP_SET_SUBIF(subif->subif,
		sif->port_info->subif_offset, sif->port_info->subif_mask);
	ret = dp_xmit(dev, subif, skb, skb->len, 0);
	kfree(subif);
	return ret;
}

int32_t dp_xmit(struct net_device *rx_if, dp_subif_single_t *rx_subif,
		struct sk_buff *skb, int32_t len, u32 flags)
{
	int inst = 0;
	struct dp_tx_common_ex ex = {
		.cmn = {
			.inst = 0,
			.flags = flags,
			.subif = rx_subif->subif,
			.tx_portid = rx_subif->port_id,
		},
	};
	struct dp_port_info *port;
	u32 vap;
	struct inst_info *dp_info = get_dp_prop_info(inst);
	enum DP_TX_FN_RET ret = DP_TX_FN_DROPPED;

#if IS_ENABLED(CONFIG_DPM_DATAPATH_EXTRA_DEBUG)
	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed for dp no init yet\n", __func__);
		dev_kfree_skb_any(skb);
		goto exit;
	}

	if (unlikely(!rx_subif)) {
		DP_DEBUG(DP_DBG_FLAG_DUMP_TX, "%s failed for rx_subif null\n", __func__);
		dev_kfree_skb_any(skb);
		goto exit;
	}

	if (unlikely(!skb)) {
		DP_DEBUG(DP_DBG_FLAG_DUMP_TX, "%s skb NULL\n", __func__);
		dev_kfree_skb_any(skb);
		goto exit;
	}

	if (unlikely(in_irq())) {
		DP_DEBUG(DP_DBG_FLAG_DUMP_TX, "%s not allowed in interrupt context\n",
				   __func__);
		dev_kfree_skb_any(skb);
		goto exit;
	}
#endif

	if (unlikely(rx_subif->port_id >=
		     dp_info->cap.max_num_dp_ports)) {
		DP_DEBUG(DP_DBG_FLAG_DUMP_TX, "rx_subif->port_id >= max_ports");
		UP_STATS(get_dp_port_info(inst, 0)->tx_err_drop);
		MIB_G_STATS_INC(tx_drop);
		dev_kfree_skb_any(skb);
		goto exit;
	}

	port = get_dp_port_info(inst, rx_subif->port_id);
	if (unlikely(!rx_if && !is_dsl(port))) {
		DP_DEBUG(DP_DBG_FLAG_DUMP_TX, "null dev but not DSL\n");
		dev_kfree_skb_any(skb);
		goto exit;
	}

	ex.dev = rx_if;
	ex.rx_subif = rx_subif;
	ex.port = port;
	vap = GET_VAP(ex.cmn.subif, port->vap_offset, port->vap_mask);
	ex.sif = get_dp_port_subif(port, vap);

	ex.mib = get_dp_port_subif_mib(ex.sif);
	ex.cmn.alloc_flags = port->alloc_flags;
	ret = dp_info->dp_tx(skb, &ex.cmn);
	if (likely(ret == DP_TX_FN_CONSUMED)) {
		MIB_G_STATS_INC(tx_pkts);
	} else if (ret != DP_TX_FN_BUSY) {
		if (ret == DP_TX_FN_CONTINUE)
			ret = DP_TX_FN_DROPPED;

		UP_STATS(ex.mib->tx_pkt_dropped);
		MIB_G_STATS_INC(tx_drop);

		/* In LGM Fail case DPM free skb, Success case CQM Free
		 * In PRX Fail and Success case CQM free
		 */
		if (is_soc_tpz(ex.cmn.inst))
			dev_kfree_skb_any(skb);
	}

	if (flags & DP_TX_NEWRET)
		return ret;

	if (ret == DP_TX_FN_BUSY) {
		UP_STATS(ex.mib->tx_pkt_dropped);
		MIB_G_STATS_INC(tx_drop);
		dev_kfree_skb_any(skb);
	}
exit:
	return ret ? DP_FAILURE : DP_SUCCESS;
}
EXPORT_SYMBOL(dp_xmit);

void set_dp_dbg_flag(u64 flags)
{
	dp_dbg_flag = flags;
}

u64 get_dp_dbg_flag(void)
{
	return dp_dbg_flag;
}

u64 get_dp_dbgfs_flag(void)
{
	return dp_dbgfs_flag;
}

void set_dp_dbgfs_flag(u64 flags)
{
	dp_dbgfs_flag = flags;
}

/*!
 *@brief  The API is for dp_get_cap
 *@param[in,out] cap dp_cap pointer, caller must provide the buffer
 *@param[in] flag for future
 *@return 0 if OK / -1 if error
 */
int dp_get_cap(struct dp_cap *cap, int flag)
{
	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	if (!cap || is_invalid_inst(cap->inst))
		return DP_FAILURE;
	if (!hw_cap_list[cap->inst].valid)
		return DP_FAILURE;
	*cap = hw_cap_list[cap->inst].info.cap;

	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_get_cap);

int dp_rx_enable(struct net_device *netif, char *ifname, u32 flags)
{
	dp_subif_t *subif;
	struct dp_port_info *port_info;
	int vap, i;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	subif = dp_get_netif_subifid(netif, NULL, NULL, NULL, 0);
	if (!subif) {
		DP_DEBUG(DP_DBG_FLAG_DBG, "get subifid fail(%s)\n",
			 netif ? netif->name : "NULL");
		return DP_FAILURE;
	}
	port_info = get_dp_port_info(subif->inst, subif->port_id);
	for (i = 0; i < subif->subif_num; i++) {
		vap = GET_VAP(subif->subif_list[i], port_info->vap_offset,
				port_info->vap_mask);
		STATS_SET(get_dp_port_subif(port_info, vap)->rx_flag,
			  flags ? 1 : 0);
	}

	dp_free_netif_subifid(subif);
	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_rx_enable);

int dp_lookup_mode_cfg(int inst, struct cqm_lookup_sel *entry,
		       u32 flag)
{
	int ret = DP_SUCCESS;
	struct dp_port_info *port_info;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	if (!entry)
		return DP_FAILURE;

	if (is_invalid_port(entry->dpid))
		return DP_FAILURE;

	port_info = get_dp_port_info(inst, entry->dpid);
	if (!port_info)
		return DP_FAILURE;

	DP_LIB_LOCK(&dp_lock);
	if (flag & DP_CQM_LU_MODE_GET) {
		ret = dp_cqm_qsv_get(inst,
			      entry, 0);
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "GET: inst: %d, dp_port: %d\n",
			 inst, entry->dpid);
	} else {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "SET: Not supported!! inst: %d, dp_port: %d\n",
			 inst, entry->dpid);
	}
	DP_LIB_UNLOCK(&dp_lock);

	return ret;
}
EXPORT_SYMBOL(dp_lookup_mode_cfg);

/* ethtool statistics support */
void dp_net_dev_get_ss_stat_strings(struct net_device *dev, u8 *data)
{
	/* This is for the Mib counter wraparound module */
	if (dp_get_dev_ss_stat_strings_fn) {
		DP_DEBUG(DP_DBG_FLAG_MIB,
			 "MIB call dp_get_dev_ss_stat_strings_fn callback\n");
		dp_get_dev_ss_stat_strings_fn(dev, data);
	}

	return;
}
EXPORT_SYMBOL(dp_net_dev_get_ss_stat_strings);

int dp_net_dev_get_ss_stat_strings_count(struct net_device *dev)
{
	/* This is for the Mib counter wraparound module */
	if (dp_get_dev_stat_strings_count_fn) {
		DP_DEBUG(DP_DBG_FLAG_MIB,
			 "MIB call dp_get_dev_stat_strings_count_fn callback\n");
		return dp_get_dev_stat_strings_count_fn(dev);
	}

	return 0;
}
EXPORT_SYMBOL(dp_net_dev_get_ss_stat_strings_count);

void dp_net_dev_get_ethtool_stats(struct net_device *dev,
				  struct ethtool_stats *stats, u64 *data)
{
	/* This is for the Mib counter wraparound module */
	if (dp_dev_get_ethtool_stats_fn) {
		DP_DEBUG(DP_DBG_FLAG_MIB,
			 "MIB call dp_dev_get_ethtool_stats_fn callback\n");
		return dp_dev_get_ethtool_stats_fn(dev, stats, data);
	}

	return;
}
EXPORT_SYMBOL(dp_net_dev_get_ethtool_stats);

int dp_spl_conn(int inst, struct dp_spl_cfg *conn)
{
	struct inst_info *dp_info = get_dp_prop_info(inst);
	int res;

	if (is_invalid_inst(inst))
		return DP_FAILURE;

	if (!dp_late_init())
		return DP_FAILURE;

	if (!dp_info->dp_spl_conn)
		return DP_FAILURE;

	res = dp_info->dp_spl_conn(inst, conn);

	return res;
}
EXPORT_SYMBOL(dp_spl_conn);

int dp_spl_conn_get(int inst, enum DP_SPL_TYPE type,
		    struct dp_spl_cfg *conns, u8 cnt)
{
	struct inst_info *dp_info;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	if (is_invalid_inst(inst))
		return DP_FAILURE;

	if (!cnt || !conns) {
		pr_err("DPM: Wrong parameter\n");
		return DP_FAILURE;
	}

	dp_info = get_dp_prop_info(inst);

	if (!dp_info->dp_spl_conn_get)
		return DP_FAILURE;

	return dp_info->dp_spl_conn_get(inst, type, conns, cnt);
}
EXPORT_SYMBOL(dp_spl_conn_get);

int dp_register_event_cb(struct dp_event *info, u32 flag)
{
	int ret;

	if (!info || is_invalid_inst(info->inst))
		return DP_FAILURE;

	if (flag & DP_F_DEREGISTER)
		ret = unregister_dp_event_notifier(info);
	else
		ret = register_dp_event_notifier(info);

	return ret;
}
EXPORT_SYMBOL(dp_register_event_cb);

int dp_free_buffer_by_policy(struct dp_buffer_info *info, u32 flag)
{
	struct cqm_bm_free data = {0};
	int ret;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s failed: datapath not initialized yet\n", __func__);
		return DP_FAILURE;
	}

	if (!info)
		return DP_FAILURE;

	data.flag = flag;
	data.buf = (void *)info->addr;
	data.policy_base = info->policy_base;
	data.policy_num = info->policy_num;

	ret = dp_cqm_buffer_free_by_policy(info->inst, &data);
	if (ret != CQM_OK) {
		pr_err("DPM: cqm_buffer_free_by_policy failed with %d\n", ret);
		return DP_FAILURE;
	}
	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_free_buffer_by_policy);

/* support dp_dbg string instead of value only.
 * the supported format: tx:rx:dbg:dbgfs_xxx...
 * dbgfs_xxx can be enabled only via 'dp_dbg' as string
 */
static int set_dbg_flag(char *str)
{
	struct {
		char *param_list[64];
		char *p1, str[120];
	} *p;
	int i, j, num, cmd_total, dbgfs_cmds;

	p = dp_kzalloc(sizeof(*p), GFP_ATOMIC);
	if (!p)
		return -ENOMEM;

	dp_strlcpy(p->str, str, sizeof(p->str));
	p->p1 = p->str;
	for (num = 0; num < ARRAY_SIZE(p->param_list); num++) {
		if (!strlen(p->p1))
			break;
		p->param_list[num] = dp_strsep(&p->p1, ":");
		if (!p->p1) {
			num++;
			break;
		}
	}
	cmd_total = get_dp_dbg_flag_str_size() - 1;
	dbgfs_cmds = get_dp_dbgfs_flag_str_size() - 1;
	for (i = 0; i < num; i++) {
		for (j = 0; j < cmd_total; j++) {
			if (!strcasecmp(p->param_list[i],
					dp_dbg_flag_str[j])) {
				dp_dbg_flag |= dp_dbg_flag_list[j];
				break;
			}
		}
		if (j < cmd_total) /*cmd found, so continue*/
		       continue;
		/*cmd  not found, so check the debugfs_flag list*/
		for (j = 0; j < dbgfs_cmds; j++) {
			if (!strcasecmp(p->param_list[i],
					dp_dbgfs_flag_str[j])) {
				dp_dbgfs_flag |= dp_dbgfs_flag_list[j];
				break;
			}
		}
	}

	kfree(p);
	return 0;
}

#if IS_ENABLED(CONFIG_KALLSYMS) && IS_ENABLED(CONFIG_KGDB)
/* copy below two struct from kernel/module.c */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
struct module_sect_attr {
	struct bin_attribute battr;
	unsigned long address;
};

struct module_sect_attrs {
	struct attribute_group grp;
	unsigned int nsections;
	struct module_sect_attr attrs[];
};
#else
struct module_sect_attr {
	/* purpose change mattr to battr to share same code of dp_get_addr */
	struct module_attribute battr;
	char *name;
	unsigned long address;
};

struct module_sect_attrs {
	struct attribute_group grp;
	unsigned int nsections;
	struct module_sect_attr attrs[0];
};
#endif

static unsigned long text, bss, data;
#endif

void dp_get_addr(void)
{
#if IS_ENABLED(CONFIG_KALLSYMS) && IS_ENABLED(CONFIG_KGDB)
	int i;
	struct module *module = THIS_MODULE;

	if (!module->sect_attrs) {
		pr_err("DPM: why THIS_MODULE->sect_attrs NULL ?\n");
		return;
	}
	for (i = 0; i < module->sect_attrs->nsections; i++) {
		if (strcmp(module->sect_attrs->attrs[i].battr.attr.name,
			".text") == 0) {
			text = module->sect_attrs->attrs[i].address;
			continue;
		}
		if (strcmp(module->sect_attrs->attrs[i].battr.attr.name,
			".data") == 0) {
			data = module->sect_attrs->attrs[i].address;
			continue;
		}
		if (strcmp(module->sect_attrs->attrs[i].battr.attr.name,
			".bss") == 0) {
			bss = module->sect_attrs->attrs[i].address;
			continue;
		}
	}
#endif
}

void dp_dump_addr(struct seq_file *s)
{
#if IS_ENABLED(CONFIG_KALLSYMS) && IS_ENABLED(CONFIG_KGDB)
	dp_sprintf(s, "dp add-symbol-file:0x%lx -s .data 0x%lx -s .bss 0x%lx\n",
			   text, data, bss);
#endif
}

void dp_gdb_break(void)
{
#if IS_ENABLED(CONFIG_KALLSYMS) && IS_ENABLED(CONFIG_KGDB)
	kgdb_breakpoint();
#endif
}

int dp_pre_init(void)
{
	if (dp_sanity_check())
		return -1;
	/*mask to reset some field as SWAS required  all others try to keep */
	dp_memset(dp_port_prop, 0, sizeof(dp_port_prop));
	dp_memset(dp_port_info, 0, sizeof(dp_port_info));
	if (dp_dbg_flag) /* high priority(1st) with local setting */
		goto SKIP_MODULE_DP_DBG_CHECK;
	/* 2nd priority: get dp_dbg flag from module_param. */
	if (dp_dbg && strlen(dp_dbg)) {
		set_dbg_flag(dp_dbg);
		if (!dp_dbg_flag) /* check number if no string match */
			dp_dbg_flag = dp_atoull(dp_dbg);
	}
SKIP_MODULE_DP_DBG_CHECK:
	g_dp_dev = platform_device_register_simple("dp_plat_dev", 0, NULL, 0);
	if (IS_ERR(g_dp_dev)) {
		pr_err("DPM: dp_pre_init register platform device fail\n");
		return -1;
	}
	//dp_get_addr(); TODO: VBOLLA: in builtin module it causes crash
	if (dp_dbg_flag & DP_DBG_FLAG_GDB) {
		dp_dump_addr(NULL);
		dp_gdb_break();
	}
	log_buf = devm_kzalloc(&g_dp_dev->dev, log_buf_len, GFP_ATOMIC);
	dp_proc_install();
	dp_inst_init(0);
	dp_subif_init();
	dp_subif_list_init();

	if (dp_tx_ctx_init(0)) {
		pr_err("DPM: dp_pre_init tx_ctx_init fail\n");
		goto error;
	}
	if (dp_rx_ctx_init(0)) {
		pr_err("DPM: dp_pre_init rx_ctx_init fail\n");
		goto error;
	}
	register_dp_cap(0);
	init_qos_setting();
	DP_DUMP("DPM: dp_pre_init done: dp_dbg=%s dp_dbg_flag=0x%llx\n",
		dp_dbg, dp_dbg_flag);

#if DP_FAST_LATE_INIT
	dp_late_init();
#endif
	return 0;
error:
	platform_device_unregister(g_dp_dev);
	return -1;
}

int dp_late_init_module(void)
{
	int res = 0;

	if (dp_init_ok) /* alredy fully init */
		return 0;
	if (!dp_is_ready(NULL)) /* dependency not ready and re-check again */
		return 0;
	if (request_dp(0)) { /*register 1st dp instance */
		pr_err("DPM: register_dp instance fail\n");
		atomic_sub(1, &dp_status);
		return -1;
	}
#if IS_ENABLED(CONFIG_DPM_DATAPATH_EXTRA_DEBUG)
	DP_DUMP("\n%s: Context ==> preempt_count=0x%x\n", __func__,
		preempt_count());
	DP_DUMP("   irq: %d, softirq: %d, interrupt: %d, serv_softirq: %d, "
		"nmi: %d, task: %d\n", !!in_irq(), !!in_softirq(),
		!!in_interrupt(), !!in_serving_softirq(), !!in_nmi(),
		!!in_task());
#endif
	dp_init_ok = 1;

	return res;
}

void dp_cleanup_module(void)
{
	int i;

	dp_mod_exiting = true;

	DP_DUMP("start cleanup dp module\n");
	DP_LIB_LOCK(&dp_lock);
	if (dp_init_ok) {  /* map to dp_late_init_module */

		for (i = 0; i < dp_inst_num; i++) {
			DP_CB(i, dp_platform_set)(i, DP_PLATFORM_DE_INIT);
			free_dp_port_subif_info(i);
		}
		dp_init_ok = 0;
	}
	dp_init_state = 0;
	/* dp_subif_list_free */
	dp_subif_list_free();
	dp_subif_free();
	/* dp_inst_free */
	dp_inst_free();

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DEBUGFS)
	/* dp_proc_install */
	debugfs_remove_recursive(dp_proc_node);
#endif
	platform_device_unregister(g_dp_dev);
	DP_LIB_UNLOCK(&dp_lock);
}

/*!
 * @brief get network device's MTU
 * @param[in] dev: network device pointer
 * @param[out] mtu_size: return the maximum MTU can be supported
 *                       for this device based on current HW configuration
 * @return DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_get_mtu_size(struct net_device *dev, u32 *mtu_size)
{
	struct dp_port_info *port;
	struct dp_subif_info *sif;
	dp_subif_t *subif;
	struct cqm_mtu mtu;

	if (unlikely(!dp_init_ok)) {
		DP_DEBUG(DP_DBG_FLAG_INIT_STAT, "%s dpm not ready\n", __func__);
		return DP_FAILURE;
	}

	subif = dp_get_netif_subifid(dev, NULL, NULL, NULL, 0);
	if (unlikely(!subif))
		return DP_FAILURE;

	port = get_dp_port_info(subif->inst, subif->port_id);
	sif = get_dp_port_subif(port, GET_VAP(subif->subif, port->vap_offset,
					      port->vap_mask));

	mtu.cqm_inst = dp_port_prop[subif->inst].cqm_inst;
	mtu.dp_port = subif->port_id;
	mtu.policy_map = dp_cqm_get_policy_map(subif->inst,
				 sif->tx_policy_base, sif->tx_policy_num,
				 port->alloc_flags, TX_POLICYMAP);
	mtu.alloc_flag = port->alloc_flags;
	mtu.subif_flag = sif->flags;

	if (unlikely(dp_cqm_get_mtu_size(subif->inst, &mtu))) {
		dp_free_netif_subifid(subif);
		return DP_FAILURE;
	}
	mtu.mtu -= ETH_HLEN;
#ifdef TOPAZ_CODE_ENABLE
	if (is_stream_port(port->alloc_flags))
		mtu.mtu -= sizeof(struct pmac_tx_hdr);
#endif
	if (likely(mtu_size))
		*mtu_size = mtu.mtu;
	dp_free_netif_subifid(subif);
	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_get_mtu_size);

int dp_set_mtu_size(struct net_device *dev, u32 mtu_size)
{
	dp_subif_t *subif;
	struct inst_info *dp_info;
	struct dp_port_info *port;
	int vap, rc;

	subif = dp_get_netif_subifid(dev, NULL, NULL, NULL, 0);
	if (unlikely(!subif))
		return -1;
	DP_LIB_LOCK(&dp_lock);
	dp_info = get_dp_prop_info(subif->inst);
	port = get_dp_port_info(subif->inst, subif->port_id);
	vap = GET_VAP(subif->subif, port->vap_offset, port->vap_mask);
	if (unlikely(!dp_info->subif_platform_change_mtu))
		goto EXIT;
	rc = dp_info->subif_platform_change_mtu(subif->inst, subif->port_id,
						vap, mtu_size + ETH_HLEN);
EXIT:
	DP_LIB_UNLOCK(&dp_lock);
	dp_free_netif_subifid(subif);
	return rc;
}
EXPORT_SYMBOL(dp_set_mtu_size);

int dp_get_dc_config(struct dp_dc_res *res, int flag)
{
	struct cqm_dc_res r;

	if (!res)
		return DP_FAILURE;

	r.cqm_inst = res->inst;
	r.dp_port = res->dp_port;
	r.res_id = res->res_id;
	r.alloc_flags = get_dp_port_info(res->inst, res->dp_port)->alloc_flags;

	if (dp_cqm_get_dc_config(res->inst, &r, flag)) {
		pr_err("DPM: %s: %s failed, inst=%d, dp_port=%d, res_id=%d\n, alloc_flags=0x%x",
		       __func__, "cqm_dp_get_dc_config",
		       r.cqm_inst, r.dp_port, r.res_id, r.alloc_flags);
		return DP_FAILURE;
	}
	res->rx_res = r.rx_res;
	res->tx_res = r.tx_res;
	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_get_dc_config);

int dp_get_port_prop(int inst, int port_id,
		      struct dp_port_prop *prop)
{
	struct dp_port_info *port_info;
	if (!dp_init_ok)
		return DP_FAILURE;

	if (!prop)
		return DP_FAILURE;
	port_info = get_dp_port_info(inst, port_id);
	prop->vap_offset = port_info->vap_offset;
	prop->vap_mask = port_info->vap_mask;
	prop->alloc_flags = port_info->alloc_flags;
	prop->owner = port_info->owner;
	prop->num_subif = port_info->num_subif;
	prop->max_subif = port_info->max_subif;
	prop->status = port_info->status;
	prop->port_id = port_info->port_id;
	prop->deq_ring_num = port_info->num_deq;

	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_get_port_prop);

int dp_get_subif_prop(int inst, int port_id, int vap,
		       struct dp_subif_prop *prop)
{
	struct dp_port_info *port_info;
	struct dp_subif_info *subif_info;

	if (!dp_init_ok)
		return DP_FAILURE;
	if (!prop)
		return DP_FAILURE;
	port_info = get_dp_port_info(inst, port_id);
	subif_info = get_dp_port_subif(port_info, vap);
	prop->flags = subif_info->flags;
	prop->netif = subif_info->netif;
	prop->ctp_dev = subif_info->ctp_dev;
	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_get_subif_prop);

bool dp_is_ready(u32 *state)
{
	if (!(dp_init_state & BIT(DP_OPS_QOS))) {
		struct pp_qos_dev *qdev;
		u32 major, minor, build;

		/* workaround required since PP not register ops to DPM */
		qdev = dp_qos_dev_open(dp_port_prop[0].qos_inst);
		if (qdev &&
		    !pp_qos_get_fw_version(qdev, &major, &minor, &build)) {
			dp_init_state |= BIT(DP_OPS_QOS);
		}
	}
	if (state)
		*state = dp_init_state;
	if ((dp_init_state & DP_DEPENDENCY_BITS) == DP_DEPENDENCY_BITS)
		return true;
	return false;
}
EXPORT_SYMBOL(dp_is_ready);

/* Note, we can use kgdb or dp_dbg memroy write tool to set dp_dbg_flag to zero
 * and jump back to caller's context
 */
void dp_die(const char *func_name, int curr_v, int ref_v)
{
	pr_err("DPM: %s curr_v=%d ref_v=%d dp_dbg_flag=0x%px\n",
	       func_name ? func_name : "NULL",
	       curr_v, ref_v, &dp_dbg_flag);
	DPM_BUG_ON(1);
}

static int __init dp_init(void)
{
	printk("========> DPM: (DATAPATH MANAGER) Module, Version: %s <========\n",
			__stringify(DP_VER_MAJ.DP_VER_MID.DP_VER_MIN.DP_VER_TAG));
	return dp_pre_init();
}

static void __exit dp_exit(void)
{
	dp_cleanup_module();
}

/* parameter dp_dbg=-1, or dp_dbg=0x10, or
 *           dp_dbg=dbg:qos:tx
 */
static int __init dp_dbg_lvl_set(char *str)
{
	DP_DUMP("dp_dbg=%s\n", str);
	dp_dbg = str;

	/* check string first if it is not set yet */
	if (!dp_dbg_flag || !dp_dbgfs_flag)
		set_dbg_flag(dp_dbg);
	/* check number if no string match */
	if (!dp_dbg_flag)
		dp_dbg_flag = dp_atoull(dp_dbg);

	return 0;
}

/* uboot pass dp_dbg= to linux in built-in kernel */
early_param("dp_dbg", dp_dbg_lvl_set);

module_param(dp_dbg, charp, S_IRUGO);
arch_initcall(dp_init);
MODULE_LICENSE("GPL");
MODULE_VERSION(__stringify(DP_VER_MAJ.DP_VER_MID.DP_VER_MIN.DP_VER_TAG));

/* disable optimization in debug mode: pop */
DP_NO_OPTIMIZE_POP
