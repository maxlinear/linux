// SPDX-License-Identifier: GPL-2.0
/*****************************************************************************
 * Copyright (c) 2024, MaxLinear, Inc.
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.

*******************************************************************************/

#include <linux/soc/mxl/datapath_api.h>
#include "../datapath.h"
#include "datapath_misc.h"
#include "datapath_ppv4_session.h"

/* disable optimization in debug mode: push */
DP_NO_OPTIMIZE_PUSH

static void __mark_alloc_gpid(int inst, int base, int end, int dpid)
{
	int tmp;
	struct hal_priv *priv = HAL(inst);

	for (tmp = base; (tmp < end) && (tmp < MAX_GPID); tmp++) {
		priv->gp_dp_map[tmp].alloc_flags = 1;
		priv->gp_dp_map[tmp].dpid = dpid;
		priv->gp_dp_map[tmp].subif = -1;
		priv->gp_dp_map[tmp].ref_cnt = 0;
	}
}

/*Get n continuous free GPIDs*/
int can_get_ncont_free_gpid(int inst, u32 gpid_base, int num, int end_limit)
{
	int ret = 0, nfree = 0, i;
	struct hal_priv *priv = HAL(inst);

	if (end_limit < gpid_base)
		return ret;
	if (gpid_base + num - 1 > end_limit)
		return ret;

	for (i = 0; i < num; i++) {
		if ((gpid_base + i) > end_limit)
			break;
		if ((gpid_base + i) >= MAX_GPID)
			return ret;
		if (priv->gp_dp_map[gpid_base + i].alloc_flags)
			break;
		nfree++;
	}

	if (nfree == num)
		ret = 1;

	return ret;
}

/* This API will be used during DP_alloc_port/ext only */
int alloc_gpid(int inst, enum GPID_TYPE type, int gpid_num, int dpid)
{
	u32 base;
	struct hal_priv *priv = HAL(inst);

	if (type < DP_DYN_GPID) {
		DP_DEBUG(DP_DBG_FLAG_DBG, "dpid: %d, can't alloc overlaps with LPID\n", dpid);
		return DP_FAILURE;
	}

	if (type == DP_DYN_GPID) {
		for (base = DP_DYN_GPID_START; base <= DP_DYN_GPID_END; base++) {
			if (can_get_ncont_free_gpid(inst, base, gpid_num, DP_DYN_GPID_END)) {
				__mark_alloc_gpid(inst, base, base + gpid_num, dpid);
				return base;
			}
		}
	} else if (type == DP_SPL_GPID) {
		base = SPL_GPID_VIA_DPID(dpid);
		if (gpid_num != 1) {
			pr_err("DPM: %s %d, why dpid: %d, gpid_num(%d) != 1 for SPL_GPID\n",
					__func__, __LINE__, dpid, gpid_num);
			return DP_FAILURE;
		}
		if (priv->gp_dp_map[base].alloc_flags) {
			pr_err("DPM: %s %d, why this dpid: %d, SPL_GPID: %d already allocated\n",
					__func__, __LINE__, dpid, base);
			return DP_FAILURE;
		}
		__mark_alloc_gpid(inst, base, base + 1, dpid);

		return base;
	}

	pr_err("DPM: %s %d, failed: dpid: %d, type: %d, nGpid: %d\n",
			__func__, __LINE__, dpid,
			type, gpid_num);
	return DP_FAILURE; /* Alloc GPID Failure */
}

int free_gpid(int inst, int gpid_base, int gpid_num, int gpid_spl)
{
	struct dp_gpid_map_table *map = HAL(inst)->gp_dp_map;

	if (gpid_num > 0) {
		/* Expecting base always greater than DP_DYN_GPID_START */
		if (gpid_base >= DP_DYN_GPID_START)
			dp_memset(map + gpid_base, 0, gpid_num * sizeof(*map));
	}
	if (gpid_spl > 0)
		dp_memset(map + gpid_spl, 0, sizeof(*map));

	return DP_SUCCESS;
}

int get_gpid_refcnt(int inst, int gpid)
{
	struct hal_priv *priv = HAL(inst);

	return priv->gp_dp_map[gpid].ref_cnt;

}

int get_gpid_subif(int inst, int gpid)
{
	struct hal_priv *priv = HAL(inst);

	return priv->gp_dp_map[gpid].subif;
}

int get_dpid_from_gpid(int inst, int gpid)
{
	struct hal_priv *priv = HAL(inst);

	if (likely(gpid < MAX_GPID))
		return priv->gp_dp_map[gpid].dpid;
	else
		return -1;
}

int get_subif_size(u32 vap_mask)
{
	u8 i;

	for (i = 0; i < sizeof(vap_mask) * 8; i++)
		if (!(vap_mask & (1 << i)))
			break;

	return i;
}

static void dp_dump_hif_datapath(const char *f, struct pp_hif_datapath *dp)
{
#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	int i;

	if (!(dp_dbg_flag & DP_DBG_FLAG_REG))
		return;
	if (f)
		DP_DUMP("DPM: %s -> %s pp_hif_datapath:\n", f, __func__);
	DP_DUMP("   hif.dp.color          = %d\n", dp->color);
	for (i = 0; i < ARRAY_SIZE(dp->eg); i++) {
		DP_DUMP("   hif.dp.eg[%d].pid     = %u\n", i, dp->eg[i].pid);
		DP_DUMP("   hif.dp.eg[%d].qos_q   = %u\n", i, dp->eg[i].qos_q);
	}
#endif
}

static void dp_dump_hostif_cfg(const char *f, struct pp_hostif_cfg *hif)
{
#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (!(dp_dbg_flag & DP_DBG_FLAG_REG))
		return;
	DP_DUMP("DPM: %s -> %s pp_hostif_cfg:\n", f, __func__);
	DP_DUMP("   hif.cls.port          = %u\n", hif->cls.port);
	DP_DUMP("   hif.cls.tc_bitmap     = 0x%x\n", hif->cls.tc_bitmap);
	dp_dump_hif_datapath(NULL, &hif->dp);
#endif
}

static void dp_dump_port_cfg(struct pp_port_cfg *p)
{
#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (!(dp_dbg_flag & DP_DBG_FLAG_REG))
		return;
	DP_DUMP("   pp_port_cfg:\n");
	DP_DUMP("      cfg.rx.cls.n_flds     = %d\n", p->rx.cls.n_flds);
	DP_DUMP("      cfg.rx.mem_port_en    = %d\n", p->rx.mem_port_en);
	DP_DUMP("      cfg.rx.flow_ctrl_en   = %d\n", p->rx.flow_ctrl_en);
	DP_DUMP("      cfg.rx.policies_map   = 0x%X\n", p->rx.policies_map);
	DP_DUMP("      cfg.rx.parse_type     = %d\n", p->rx.parse_type);

	DP_DUMP("      cfg.tx.max_pkt_size   = %d\n", p->tx.max_pkt_size);
	DP_DUMP("      cfg.tx.headroom_size  = %d\n", p->tx.headroom_size);
	DP_DUMP("      cfg.tx.tailroom_size  = %d\n", p->tx.tailroom_size);
	DP_DUMP("      cfg.tx.min_pkt_len    = %d\n", p->tx.min_pkt_len);
	DP_DUMP("      cfg.tx.base_policy    = %d\n", p->tx.base_policy);
	DP_DUMP("      cfg.tx.policy_map     = 0x%X\n", p->tx.policies_map);
	DP_DUMP("      cfg.tx.pkt_only_en    = %d\n", p->tx.pkt_only_en);
	DP_DUMP("      cfg.tx.seg_en         = %d\n", p->tx.seg_en);
	DP_DUMP("      cfg.tx.pre_l2_en      = %d\n", p->tx.prel2_en);
	DP_DUMP("      cfg.tx.wr_desc        = %d\n", p->tx.wr_desc);
#endif
}

static void dp_init_hif_datapath(struct pp_hif_datapath *dp)
{
	int i;

	dp->color = PP_COLOR_INVALID;

	for (i = 0; i < ARRAY_SIZE(dp->sgc); i++)
		dp->sgc[i] = PP_SGC_INVALID;

	for (i = 0; i < ARRAY_SIZE(dp->tbm); i++)
		dp->tbm[i] = PP_TBM_INVALID;

	for (i = 0; i < ARRAY_SIZE(dp->eg); i++) {
		dp->eg[i].qos_q = PP_QOS_INVALID_ID;
		dp->eg[i].pid = PP_PORT_INVALID;
	}
}

/*TODO: VBOLLA: have to check this api thoroughly*/
static void dp_fill_hif_datapath(int inst, int priority, int dpid, int vap,
				 struct pp_hif_datapath *dp, /* old_dp */
				 struct pp_hif_datapath *new_dp /* new_dp */)
{
	struct dp_port_info *cpu_info = get_dp_port_info(inst, CPU_PORT);
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	struct dp_subif_info *port_sif = get_dp_port_subif(port_info, vap);
	struct dp_subif_info *sif;
	int i, n;

	dp_init_hif_datapath(dp);
	dp->color = PP_COLOR_GREEN;

	/* cpu0: 0(high) 1(low)
	 * cpu1: 2(high) 3(low)
	 * cpu2: 4(high) 5(low)
	 * cpu3: 6(high) 7(low)
	 */
	n = !(priority / MAX_PORTS_PER_CPU); /* 0/1 - 2nd (Low Prio) dequeue port, priorirty 2/3 - 1st (High Prio) dequeue port */
	for (i = 0; i < DP_MAX_CPU; i++) { /* each CPU */
		/* high priority : deq ring @[0]
		 *  low priority : deq ring @[1]
		 */
		sif = get_dp_port_subif(cpu_info, DP_DPID0_CPU_SUBIF_START + i);
		if (sif->flags) {
			if (!port_sif->last_hosif_qid[i][priority]) {
				dp->eg[i].qos_q = dp_get_q_logic(inst, sif->def_qid_list[n]);
				port_sif->last_hosif_qid[i][priority] = dp->eg[i].qos_q;
			} else {
				dp->eg[i].qos_q = port_sif->last_hosif_qid[i][priority];
				if (new_dp)
					port_sif->last_hosif_qid[i][priority] = new_dp->eg[i].qos_q;
			}
			dp->eg[i].pid = sif->gpid;
		}
	}
	dp_dump_hif_datapath(__func__, dp);
}

static void dp_fill_hostif_cfg(int inst, int dpid, int vap, int priority,
			       struct pp_hostif_cfg *hif,
			       struct pp_hif_datapath *new_dp)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);

	hif->cls.port = get_dp_port_subif(port_info, vap)->gpid;
	hif->cls.tc_bitmap = 1 << priority;
	dp_fill_hif_datapath(inst, priority, dpid, vap, &hif->dp, new_dp);
	dp_dump_hostif_cfg(__func__, hif);
}

static int dp_set_policy(int inst, int dpid, int vap,
			 int spl_gpid, struct pp_port_cfg *p)
{
	struct dp_port_info *port = get_dp_port_info(inst, dpid);
	int tx_base = 0, tx_range = 0, rx_base = 0, rx_range = 0;
	int flags = 0;
	struct dp_subif_info *sif;

	if (spl_gpid) {
		flags |= SP_GPID_POLICYMAP;
		vap = 0;
	}

	sif = get_dp_port_subif(port, vap);
	tx_base = (int)sif->tx_policy_base;
	tx_range = (int)sif->tx_policy_num;
	rx_base = (int)sif->rx_policy_base;
	rx_range = (int)sif->rx_policy_num;

	p->tx.base_policy = tx_base;
	p->tx.policies_map = dp_cqm_get_policy_map(inst, tx_base,
				     tx_range, port->alloc_flags,
				     flags | TX_POLICYMAP);
	p->rx.policies_map = dp_cqm_get_policy_map(inst, rx_base,
				     rx_range, port->alloc_flags,
				     flags | RX_POLICYMAP);

	sif->tx_policy_map = p->tx.policies_map;
	sif->rx_policy_map = p->rx.policies_map;

	return DP_SUCCESS;
}

static int dp_set_head_tail_room(int inst, int dpid, int vap,
				 int spl_gpid, struct pp_port_cfg *p)
{
	struct dp_port_info *port = get_dp_port_info(inst, dpid);
	struct dp_subif_info *sif = get_dp_port_subif(port, vap);
	struct cqm_dflt_resv *resv;
	struct dp_cap *cap;

	cap = &get_dp_prop_info(inst)->cap;
	resv = dp_kzalloc(sizeof(*resv), GFP_ATOMIC);
	if (!resv)
		return DP_FAILURE;
	resv->dp_inst = inst;
	resv->cqm_inst = dp_port_prop[inst].cqm_inst;
	resv->alloc_flags = port->alloc_flags;
	resv->dp_port = dpid;

	/* CQM will fill headroom and tailroom based on spl conn type */
	resv->spl_conn_type = sif->spl_conn_type;
	if (dpid == CPU_PORT) {
		if (is_cpu_vap(dpid, vap))
			resv->f_cpu = 1;
	}
	if (spl_gpid)
		resv->f_spl_gpid = 1;
	if (p->tx.seg_en)
		resv->f_segment = 1;
	if (dp_cqm_get_dflt_resv(inst, resv, 0)) {
		kfree(resv);
		pr_err("DPM: cqm_get_dflt_resv failed\n");
		return DP_FAILURE;
	}
	p->tx.headroom_size = resv->headroom;
	p->tx.tailroom_size = resv->tailroom;

	if (spl_gpid) {
		sif->spl_gpid_headroom_size = p->tx.headroom_size;
		sif->spl_gpid_tailroom_size = p->tx.tailroom_size;
	}
	sif->headroom_size = p->tx.headroom_size;
	sif->tailroom_size = p->tx.tailroom_size;

	kfree(resv);
	return DP_SUCCESS;
}

static int dp_set_max_pkt_size(int inst, int dpid, int vap,
			       int spl_gpid, struct pp_port_cfg *p)
{
	struct dp_port_info *port = get_dp_port_info(inst, dpid);
	struct dp_subif_info *sif = get_dp_port_subif(port, vap);
	struct cqm_mtu mtu = {0};
	u32 size;
	struct dp_cap *cap;

	cap = &get_dp_prop_info(inst)->cap;
	mtu.cqm_inst = dp_port_prop[inst].cqm_inst;
	mtu.dp_port = dpid;
	mtu.policy_map = p->tx.policies_map;
	mtu.alloc_flag = port->alloc_flags;
	mtu.subif_flag = sif->flags;
	mtu.spl_conn_type = sif->spl_conn_type;
	if (dpid == CPU_PORT) {
		if (is_cpu_vap(dpid, vap))
			mtu.f_cpu = 1;
	}
	if (spl_gpid)
		mtu.f_spl_gpid = 1;
	if (p->tx.seg_en)
		mtu.f_segment = 1;

	if (unlikely(dp_cqm_get_mtu_size(inst, &mtu)))
		return DP_FAILURE;

	if (spl_gpid) {
		p->tx.max_pkt_size = mtu.mtu;
		return DP_SUCCESS;
	}

	/* for CPU path, we can set size to CQM provided value */
	if (dpid == CPU_PORT && !is_spl_conn(dpid, vap)) {
		size = mtu.mtu;
	} else {
		size = (sif->netif ? sif->netif->mtu : ETH_DATA_LEN) + ETH_HLEN;
		if (size > mtu.mtu) {
			size = mtu.mtu;
			if (sif->netif)
				sif->netif->mtu = mtu.mtu - ETH_HLEN;
		}
	}
	p->tx.max_pkt_size = size;
	sif->max_pkt_size = size;
	sif->cqm_mtu_size = mtu.mtu;
	return DP_SUCCESS;
}

static int dp_map_pp_gpid(int inst, int portid, int vap)
{
	struct dp_port_info *port = get_dp_port_info(inst, portid);

	/* no GPID mapped, esp for some spl_conn */
	if (vap < port->vap_offset_gpid)
		return 0;
	if (vap - port->vap_offset_gpid < port->num_gpid)
		return port->gpid[vap - port->vap_offset_gpid];

	return port->gpid[port->num_gpid - 1];
}

/* dp_subif_seg_en: to configure seg_en for a GPID to use FSQM buffer
 * inst - DP instance
 * gpid - GPID to configure seg_en
 * flag - subif specifc flags
 * if flag is DP_SUBIF_SEG_EN, set seg_en = 1
 * else if flag is DP_SUBIF_SEG_DIS set seg_en = 0
 * else do nothing
 * If success, return DP_SUCCESS.
 * else return -1 /DP_FAILURE
 */
static
int dp_subif_seg_cfg(int inst, int gpid, struct pp_port_cfg *cfg, u32 flag)
{
	struct dp_gpid_map_table *m = &HAL(inst)->gp_dp_map[gpid];
	struct pp_port_cfg *pp_cfg;

	if (!(flag & DP_SUBIF_SEG_EN) && !(flag & DP_SUBIF_SEG_DIS))
		return DP_SUCCESS;

	/* For First GPID add */
	if (!m->ref_cnt) {
		if (flag & DP_SUBIF_SEG_EN)
			cfg->tx.seg_en = 1;
		else
			cfg->tx.seg_en = 0;
		return DP_SUCCESS;
	}

	/* if multiple subif sharing same GPID, Get the settings and update only
	 * seg_en if flag is set
	 */
	pp_cfg = dp_kzalloc(sizeof(*pp_cfg), GFP_ATOMIC);
	if (!pp_cfg)
		return DP_FAILURE;
	if (unlikely(pp_port_get(gpid, pp_cfg))) {
		kfree(pp_cfg);
		pr_err("DPM: %s: failed to get cfg of gpid: %d\n", __func__, gpid);
		return DP_FAILURE;
	}

	if ((flag & DP_SUBIF_SEG_EN) && !pp_cfg->tx.seg_en) {
		pp_cfg->tx.headroom_size = 0;
		pp_cfg->tx.tailroom_size = 0;
		pp_cfg->tx.seg_en = 1;
	} else if ((flag & DP_SUBIF_SEG_DIS) && pp_cfg->tx.seg_en) {
		pp_cfg->tx.seg_en = 0;
	} else {
		kfree(pp_cfg);
		return DP_SUCCESS;
	}

	if (unlikely(pp_port_update(gpid, pp_cfg))) {
		kfree(pp_cfg);
		pr_err("DPM: %s: failed to update gpid: %d\n", __func__, gpid);
		return DP_FAILURE;
	}

	kfree(pp_cfg);
	return DP_SUCCESS;
}

/* dp_add_pp_gpid: to configure normal GPID or special GPID
 * Note: try to get all GPID related configuration via dpid/vap
 *       if spl_gpid is 1, vap is not valid
 * flag - subif specifc flags
 * If success, return DP_SUCCESS.
 * else return -1 /DP_FAILURE
 */
int dp_add_pp_gpid(int inst, int dpid, int vap, int gpid, int spl_gpid,
		   u32 flag)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	struct dp_subif_info *sif = get_dp_port_subif(port_info, vap);
	struct dp_gpid_map_table *m = &HAL(inst)->gp_dp_map[gpid];
	struct pp_port_cfg *cfg;
	const struct dp_dflt_cfg_assign *ctp_info;
	struct dp_cap *cap;

	cap = &get_dp_prop_info(inst)->cap;
	cfg = dp_kzalloc(sizeof(*cfg), GFP_ATOMIC);
	if (!cfg)
		return DP_FAILURE;
	DP_DEBUG(DP_DBG_FLAG_REG, "inst: %d, dpid: %d, vap: %d, gpid: %d, "
			"spl_gpid: %d, flag: 0x%x\n", inst, dpid, vap, gpid,
			spl_gpid, flag);
	if (spl_gpid) {
		ctp_info = dp_get_dflt_cfg_info(inst, port_info->alloc_flags);
		if (unlikely(!ctp_info)) {
			kfree(cfg);
			pr_err("DPM: get_dp_dflt_cfg_assign fail:0x%x for dpid=%d\n",
			       port_info->alloc_flags, dpid);
			return DP_FAILURE;
		}
		cfg->rx.mem_port_en = 1;
		cfg->rx.parse_type = NO_PARSE;
		cfg->rx.cls.n_flds = 2;
		/* convert traffic class to PP classification ID */
		cfg->rx.cls.n_flds = 2;
		cfg->rx.cls.cp[0].stw_off = DP_CLASS_OFFSET;
		cfg->rx.cls.cp[0].copy_size = DP_CLASS_SIZE;
		cfg->rx.cls.cp[1].stw_off = port_info->vap_offset;
		cfg->rx.cls.cp[1].copy_size =
			get_subif_size(port_info->vap_mask);
	} else {
		if (is_stream_port(port_info->alloc_flags)) {
			if (unlikely(dp_subif_seg_cfg(inst, gpid, cfg, flag))) {
				kfree(cfg);
				return DP_FAILURE;
			}
		} else {
			cfg->rx.mem_port_en = 1;
		}
		cfg->rx.parse_type = L2_PARSE;
		/* Note: for subif based GPID, no need to stw operation since
		 * it will be based on packet information, not stw data
		 */
	}

	/* only for CPU port : later need to remove once PPv4 driver update pp_rx_hook */
	if ((dpid == CPU_PORT) && vap > (DP_DPID0_SUBIF_END - cap->max_cpu))
		cfg->tx.wr_desc = true;

	cfg->rx.flow_ctrl_en = 1;
	cfg->tx.min_pkt_len = sif->min_pkt_len_cfg;

	/* Note: following functions shall be called in order */
	if (unlikely(dp_set_policy(inst, dpid, vap, spl_gpid, cfg))) {
		kfree(cfg);
		pr_err("DPM: dp_set_policy fail:dpid=%d vap=%d spl_gpid=%d\n",
			dpid, vap, spl_gpid);
		return DP_FAILURE;
	}
	if (unlikely(dp_set_head_tail_room(inst, dpid, vap, spl_gpid, cfg))) {
		kfree(cfg);
		pr_err("DPM: dp_set_policy fail:dpid=%d vap=%d spl_gpid=%d\n",
			dpid, vap, spl_gpid);
		return DP_FAILURE;
	}
	if (unlikely(dp_set_max_pkt_size(inst, dpid, vap, spl_gpid, cfg))) {
		kfree(cfg);
		pr_err("DPM: dp_set_max_pkt_size fail:dpid=%d vap=%d spl_gpid=%d\n",
			dpid, vap, spl_gpid);
		return DP_FAILURE;
	}

#ifdef TOPAZ_CODE_ENABLE
	//cfg->tx.prel2_en = !!sif->prel2_len;
#endif
	/* Note: steps above are always required to fill dp port and subif info
	 *       regardless of ref counter
	 */
	if (!m->ref_cnt) {
		dp_dump_port_cfg(cfg);
		if (unlikely(pp_port_add(gpid, cfg))) {
			kfree(cfg);
			pr_err("DPM: %s failed to create gpid: %d\n", __func__,
					gpid);
			return DP_FAILURE;
		}
	}

	if (!spl_gpid) {
		sif->gpid = gpid;
		m->subif = sif->subif;
	}

	m->ref_cnt++;
	kfree(cfg);
	DP_DEBUG(DP_DBG_FLAG_REG, "GPID[%d] added ok\n", gpid);
	return DP_SUCCESS;
}

int dp_del_pp_gpid(int inst, int dpid, int vap)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	int gpid = 0;
	struct dp_gpid_map_table *m;

	if (vap == -1) { /* free spl gpid during dp_dev_de-register */
		/* Need to check this for voice */
		gpid = get_dp_port_info(inst, dpid)->gpid_spl;
	} else {
		gpid = get_dp_port_subif(port_info, vap)->gpid;
	}
	m = &HAL(inst)->gp_dp_map[gpid];

	if (!m->ref_cnt) {
		pr_err("DPM: cannot delete gpid[%d] since ref counter is 0\n", gpid);
		return DP_FAILURE;
	}
	if (m->ref_cnt > 1)
		goto EXIT;

	if (unlikely(pp_port_del(gpid))) {
		pr_err("DPM: failed to delete gpid: %d\n", gpid);
		return DP_FAILURE;
	}
EXIT:
	m->ref_cnt--;
	return DP_SUCCESS;
}

/* dp_del_default_egress_sess: Del default egress session based on session id
 */
int dp_del_default_egress_sess(struct dp_subif_info *p_subif, int flag)
{
	int ret, j;
	u32 sess_id;
	/* ?? not sure whether need consider oob_class_size per port or not */
	for (j = 0; j < DP_DFL_SESS_NUM; j++) {
		if (p_subif->dfl_eg_sess[0][j] == INV_RESV_IDX)
			continue;

		sess_id = p_subif->dfl_eg_sess[0][j];

		ret = pp_session_delete(sess_id, NULL);
		DP_DEBUG(DP_DBG_FLAG_REG,
			 "%s %s %d\n",
			 ret ? "Fail" : "Succeed",
			 "to del dft egress sess", sess_id);
		if (ret)
			return DP_FAILURE;
	}
	return DP_SUCCESS;
}

/* dp_add_default_egress_sess: Add default egress session based on
 *                             special GPID, class/subif only
 * This API will be used only for CPU TX path to memory port for
 * policy/pool conversion
 */
int dp_add_default_egress_sess(struct dp_session *sess, int flag)
{
	int i;
	struct pp_sess_create_args *args;
	u32 sess_id = -1;
	int ret;
	struct dp_port_info *port_info;

	args = dp_kzalloc(sizeof(*args), GFP_ATOMIC);
	if (!args)
		return DP_FAILURE;
	args->in_port = sess->in_port;
	args->eg_port = sess->eg_port;
	args->fsqm_prio = 0;
	args->color = PP_COLOR_GREEN;
	args->flags = 0;
	args->dst_q = dp_get_q_logic(sess->inst, sess->qid);
	for (i = 0; i < ARRAY_SIZE(args->sgc); i++)
		args->sgc[i] = PP_SGC_INVALID;
	for (i = 0; i < ARRAY_SIZE(args->tbm); i++)
		args->tbm[i] = PP_TBM_INVALID;
	args->tmp_ud_sz = 0; /* 1 means 1 template of UD, ie, 16 bytes */
	args->cls.n_flds = 2;
	args->cls.fld_data[0] = sess->class;
	args->cls.fld_data[1] = sess->vap;
	args->rx = NULL;
	args->tx = NULL;
	args->hash.h1 = sess->h1;
	args->hash.h2 = sess->h2;
	args->hash.sig = sess->sig;
	ret = pp_session_create(args, &sess_id, NULL);
	DP_DEBUG(DP_DBG_FLAG_REG,
		 "%s %s=%u %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%x %s=%x %s=%x\n",
		 ret ? "Fail" : "Succeed",
		 "to create dft egress sess", sess_id,
		 "in_gpid(spl_id)", args->in_port,
		 "eg_gpid", args->eg_port,
		 "class", sess->class,
		 "dpid", get_dpid_from_gpid(0, sess->eg_port),
		 "vap", sess->vap,
		 "phy_dst_qid", sess->qid,
		 "logic_qid", args->dst_q,
		 "h1", args->hash.h1,
		 "h2", args->hash.h2,
		 "sig", args->hash.sig);

	if (ret) {
		/* IF failure, call PPA API to add this session... */
		kfree(args);
		pr_err("DPM: session create failed. Call PPA to continue");
		return DP_FAILURE;
	}
	port_info = get_dp_port_info(sess->inst,
				     get_dpid_from_gpid(0, sess->eg_port));
	get_dp_port_subif(port_info, sess->vap)->dfl_eg_sess[0][sess->class] =
		sess_id;
	DP_DEBUG(DP_DBG_FLAG_REG, "session id = %u\n", sess_id);
	kfree(args);
	return DP_SUCCESS;
}

/* dp_add_default_egress_sess_sw_hash: Add default egress session based on
 *                                     special GPID, class/subif only.
 *                                     PP will calculate hash value.
 * This API will be used only for CPU TX path to memory port for
 * policy/pool conversion
 */
static int dp_add_default_egress_sess_sw_hash(struct dp_session *sess)
{
	struct pp_sess_create_args *args;
	int ret;

	args = dp_kzalloc(sizeof(*args), GFP_ATOMIC);
	if (!args)
		return DP_FAILURE;
	args->in_port = sess->in_port;
	args->eg_port = sess->eg_port;
	args->color = PP_COLOR_GREEN;
	args->flags = BIT(PP_SESS_FLAG_INTERNAL_HASH_CALC_BIT);
	args->dst_q = dp_get_q_logic(sess->inst, sess->qid);
	dp_memset(&args->sgc, U8_MAX, sizeof(args->sgc));
	dp_memset(&args->tbm, U8_MAX, sizeof(args->tbm));

	args->cls.n_flds = 2;
	args->cls.fld_data[0] = sess->class;
	args->cls.fld_data[1] = sess->vap;

	ret = pp_session_create(args, &sess->sess_id, NULL);
	DP_DEBUG(DP_DBG_FLAG_REG,
		 "%s %s=%u %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%x %s=%x %s=%x\n",
		 ret ? "Fail" : "Succeed",
		 "to create default egress sess", sess->sess_id,
		 "in_gpid(spl_id)", args->in_port,
		 "eg_gpid", args->eg_port,
		 "class", sess->class,
		 "dpid", get_dpid_from_gpid(0, sess->eg_port),
		 "vap", sess->vap,
		 "phy_dst_qid", sess->qid,
		 "logic_qid", args->dst_q,
		 "h1", args->hash.h1,
		 "h2", args->hash.h2,
		 "sig", args->hash.sig);

	if (ret) {
		if (ret != -EOPNOTSUPP)
			pr_err("DPM: Failed to create default egress session\n");
		kfree(args);
		return DP_FAILURE;
	}

	kfree(args);
	return DP_SUCCESS;
}

/* dp_add_hostif: create hostif for CPU RX path
 * This API is for normal CPU RX path traffic per GPID
 * If exception session table full, dp_add_hostif will fail to add.
 * But this API itseld still can return success.
 */
int dp_add_hostif(int inst, int dpid, int vap)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	int gpid = get_dp_port_subif(port_info, vap)->gpid;
	struct dp_gpid_map_table *m = &HAL(inst)->gp_dp_map[gpid];
	struct pp_hostif_cfg hif;
	int i;

	for_each_clear_bit(i, &m->f_hif, DP_MAX_HOSTIF) {
		dp_fill_hostif_cfg(inst, dpid, vap, i, &hif, NULL);
		if (pp_hostif_add(&hif))
			pr_warn("DPM: hostif_add fail:dpid/gpid=%u/%u vap/tc=%d/%u\n",
				 dpid, hif.cls.port, vap, hif.cls.tc_bitmap);
		else
			set_bit(i, &m->f_hif);
	}
	return DP_SUCCESS;
}

int dp_update_hostif(int inst, int dpid, int vap, struct dp_hif_datapath *hif_dp_new)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	int gpid = get_dp_port_subif(port_info, vap)->gpid;
	struct dp_gpid_map_table *m = &HAL(inst)->gp_dp_map[gpid];
	struct pp_hostif_cfg *hif;
	struct pp_hif_datapath *new_dp;
	int i,j;
	bool need_update = 0;

	hif = dp_kzalloc(sizeof(*hif), GFP_ATOMIC);
	if (!hif)
		return DP_FAILURE;
	new_dp = dp_kzalloc(sizeof(*new_dp), GFP_ATOMIC);
	if (!new_dp) {
		kfree(hif);
		return DP_FAILURE;
	}

	for_each_set_bit(i, &m->f_hif, DP_MAX_HOSTIF) {
		need_update = 0;
		dp_memset(new_dp, 0, sizeof(*new_dp));
		dp_init_hif_datapath(new_dp);
		new_dp->color = hif_dp_new[i].color;
		for (j = 0; j < DP_MAX_CPU; j++) { /* each CPU */
			if (hif_dp_new[i].eg[j].cpu_gpid) {
				need_update = 1;
				new_dp->eg[j].pid = hif_dp_new[i].eg[j].cpu_gpid;
				new_dp->eg[j].qos_q = dp_get_q_logic(inst, hif_dp_new[i].eg[j].q_id);
			}
		}
		if (need_update) {
			dp_fill_hostif_cfg(inst, dpid, vap, i, hif, new_dp);

			if (unlikely(pp_hostif_update(hif, new_dp))) {
				pr_err("DPM: hostif_update fail:dpid/gpid=%u/%u vap/tc=%d/%u\n",
						dpid, hif->cls.port, vap, hif->cls.tc_bitmap);
				kfree(hif);
				kfree(new_dp);
				return DP_FAILURE;
			}
		}
	}
	kfree(hif);
	kfree(new_dp);
	return DP_SUCCESS;
}
EXPORT_SYMBOL(dp_update_hostif);

int __dp_update_hostif(int inst, int dpid, int vap, int type)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	int gpid = get_dp_port_subif(port_info, vap)->gpid;
	struct dp_gpid_map_table *m = &HAL(inst)->gp_dp_map[gpid];
	struct pp_hostif_cfg *hif;
	struct pp_hif_datapath *dp_new;
	int i;

	hif = dp_kzalloc(sizeof(*hif), GFP_ATOMIC);
	if (!hif)
		return DP_FAILURE;

	dp_new = dp_kzalloc(sizeof(*dp_new), GFP_ATOMIC);
	if (!dp_new) {
		kfree(hif);
		return DP_FAILURE;
	}
	for_each_set_bit(i, &m->f_hif, DP_MAX_HOSTIF) {
		dp_fill_hostif_cfg(inst, dpid, vap, i, hif, NULL);
		if (unlikely(pp_hostif_update(hif, dp_new))) {
			pr_err("DPM: hostif_update fail:dpid/gpid=%u/%u vap/tc=%d/%u\n",
			       dpid, hif->cls.port, vap, hif->cls.tc_bitmap);
			kfree(hif);
			kfree(dp_new);
			return DP_FAILURE;
		}
	}
	kfree(hif);
	kfree(dp_new);
	return DP_SUCCESS;
}

int dp_del_hostif(int inst, int dpid, int vap)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	int gpid = get_dp_port_subif(port_info, vap)->gpid;
	struct dp_gpid_map_table *m = &HAL(inst)->gp_dp_map[gpid];
	struct pp_hostif_cfg *hif;
	int i;

	if (m->ref_cnt > 1)
		return DP_SUCCESS;

	hif = dp_kzalloc(sizeof(*hif), GFP_ATOMIC);
	if (!hif)
		return DP_FAILURE;
	for_each_set_bit(i, &m->f_hif, DP_MAX_HOSTIF) {
		dp_fill_hostif_cfg(inst, dpid, vap, i, hif, NULL);
		if (unlikely(pp_hostif_del(hif))) {
			pr_err("DPM: hostif_del fail:dpid/gpid=%u/%u vap/tc=%d/%u\n",
			       dpid, hif->cls.port, vap, hif->cls.tc_bitmap);
			kfree(hif);
			return DP_FAILURE;
		}
		clear_bit(i, &m->f_hif);
	}
	kfree(hif);
	return DP_SUCCESS;
}

/* dp_add_dflt_hostif: create default hostif
 * This API is for default setting in case not match any exception sessions
 */
int dp_add_dflt_hostif(struct dp_dflt_hostif *hostif, int flag)
{
	struct pp_hif_datapath *dp;
	int ret;

	if (!hostif) {
		pr_err("DPM: hostif NULL\n");
		return DP_FAILURE;
	}
	dp = dp_kzalloc(sizeof(*dp), GFP_ATOMIC);
	if (!dp)
		return DP_FAILURE;
	dp_init_hif_datapath(dp);
	/* only allowed one queue for pp_hostif_dflt_set */
	dp->eg[0].qos_q = dp_get_q_logic(hostif->inst, hostif->qid);
	dp->eg[0].pid = hostif->gpid;
	dp->color = PP_COLOR_GREEN;

	ret = pp_hostif_dflt_set(dp);
	kfree(dp);
	return ret;
}

/* flag shall come from enum DP_SUBIF_DATA_FLAG */
int dp_subif_pp_set(int inst, int portid, int vap, u32 flag)
{
	int gpid = dp_map_pp_gpid(inst, portid, vap);
	struct dp_port_info *port_info;
	struct dp_subif_info *sif;
	struct dp_session *sess;
	int i, rc;

	rc = dp_add_pp_gpid(inst, portid, vap, gpid, 0, flag);
	if (unlikely(rc != DP_SUCCESS)) {
		pr_err("DPM: %s failed for dport/vap=%d/%d\n",
		       __func__, portid, vap);
		return rc;
	}
	if (!(flag & DP_SUBIF_NO_HOSTIF))
		dp_add_hostif(inst, portid, vap);

	sess = dp_kzalloc(sizeof(*sess), GFP_ATOMIC);
	if (!sess)
		return DP_FAILURE;
	port_info = get_dp_port_info(inst, portid);
	if (port_info->gpid_spl > 0) {
		sif = get_dp_port_subif(port_info, vap);

		sess->inst = inst;
		sess->in_port = port_info->gpid_spl;
		sess->eg_port = gpid;
		sess->qid = sif->def_qid;
		sess->vap = vap;

		for (i = 0; i < DP_DFL_SESS_NUM; i++) {
			sif->dfl_eg_sess[0][i] = INV_RESV_IDX;
			sess->class = i;
			if (dp_add_default_egress_sess_sw_hash(sess)) {
				break;
			} else {
				sif->dfl_eg_sess[0][i] = sess->sess_id;
			}
		}
	}

	kfree(sess);
	return DP_SUCCESS;
}

/* flag shall come from enum DP_SUBIF_DATA_FLAG */
int dp_subif_pp_reset(int inst, int portid, int vap, u32 flag)
{
	int rc = DP_SUCCESS;
	struct dp_port_info *port_info = get_dp_port_info(inst, portid);
	struct dp_subif_info *sif = get_dp_port_subif(port_info, vap);

	if (!(flag & DP_SUBIF_NO_HOSTIF))
		rc = dp_del_hostif(inst, portid, vap);
	if (likely(rc == DP_SUCCESS))
		rc = dp_del_pp_gpid(inst, portid, vap);

	if (unlikely(rc != DP_SUCCESS))
		pr_err("DPM: %s failed for dport/vap=%d/%d\n",
		       __func__, portid, vap);

	if (port_info->gpid_spl > 0)
		rc = dp_del_default_egress_sess(sif, 0);

	return rc;
}

int dp_subif_pp_change_mtu(int inst, int dpid, int vap, u32 mtu)
{
	struct dp_port_info *port = get_dp_port_info(inst, dpid);
	struct dp_subif_info *sif = get_dp_port_subif(port, vap);
	int gpid = sif->gpid;
	struct dp_gpid_map_table *m = &HAL(inst)->gp_dp_map[gpid];
	struct pp_port_cfg cfg;

	if (!m->ref_cnt)
		return DP_FAILURE;
	if (unlikely(pp_port_get(gpid, &cfg))) {
		pr_err("DPM: %s: failed to get cfg of gpid: %d\n", __func__, gpid);
		return DP_FAILURE;
	}
	if (mtu > sif->cqm_mtu_size) {
		pr_err("DPM: %s: MTU requested (%u) exceeds limit (%u)\n",
		       __func__, mtu, sif->cqm_mtu_size);
		return DP_FAILURE;
	}
	cfg.tx.max_pkt_size = mtu;
	if (unlikely(pp_port_update(gpid, &cfg))) {
		pr_err("DPM: %s: failed to update gpid: %d\n", __func__, gpid);
		return DP_FAILURE;
	}
	return DP_SUCCESS;
}

/* disable optimization in debug mode: pop */
DP_NO_OPTIMIZE_POP
