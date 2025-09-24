// SPDX-License-Identifier: GPL-2.0
/*****************************************************************************
 * Copyright (c) 2024, MaxLinear, Inc.
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.

*******************************************************************************/
#include <linux/device.h>
#include <linux/soc/mxl/datapath_api.h>
#include "../datapath.h"
#include "datapath_misc.h"
#include "datapath_ppv4.h"
#include "datapath_ppv4_session.h"

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"

/* disable optimization in debug mode: push */
DP_NO_OPTIMIZE_PUSH

struct spl_conn_name {
	enum DP_SPL_TYPE type;
	char *name;
};
static const struct spl_conn_name spl_conn_name[] = {
	{DP_SPL_TOE, "TOE"},
	{DP_SPL_VOICE, "VOICE"},
	{DP_SPL_VPNA, "VPN_ADAPTER"},
	{DP_SPL_APP_LITEPATH, "APP_LITEPATH"},
	{DP_SPL_PP_NF, "PP_NF"},
	{DP_SPL_PP_DUT, "PP_DUT"}
};

char *get_spl_name(enum DP_SPL_TYPE type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(spl_conn_name); i++) {
		if (spl_conn_name[i].type == type)
			return spl_conn_name[i].name;
	}
	return "unknown";
}

static int deq_ring_pre_update(int inst, const struct dp_spl_cfg *conn,
			  const struct dp_ring_pktdeq *d)
{
	struct cqm_deq_ring_info *deq = get_dp_deqring_info(inst, d->attr.ring.index);

	/* Perform update if ref_cnt is 0 */
	if (!deq->ref_cnt) {
		deq->deq = *d;
		deq->dp_port[conn->dp_port] = 1;
	}

	return deq->ref_cnt;
}

static void deq_ring_post_update(int inst, struct dp_ring_pktdeq *d,
			    const struct ppv4_q_sch_port *pp)
{
	struct cqm_deq_ring_info *deq = get_dp_deqring_info(inst, d->attr.ring.index);

	//egp->qid equals deq->qid;
	deq->qid = pp->qid;
	deq->q_node = pp->q_node;
	DP_DEBUG(DP_DBG_FLAG_SPL, "deq_ring:%d QID:%d QNODE:%d\n",
		 deq->ring_id, deq->qid, deq->q_node);
}

static int get_deq_q_node(int inst, const struct dp_ring_pktdeq *d)
{
	struct cqm_deq_ring_info *deq = get_dp_deqring_info(inst, d->attr.ring.index);

	return deq->q_node;
}

static int release_deq_ring(int inst, struct dp_spl_cfg *conn,
			    struct dp_ring_pktdeq *d)
{
	struct cqm_deq_ring_info *deq = get_dp_deqring_info(inst, d->attr.ring.index);
	struct dp_node_alloc alloc = {
		.inst = inst,
		.dp_port = conn->dp_port,
		.type = DP_NODE_QUEUE,
		.id.q_id = deq->qid,
	};

	deq->ref_cnt--;
	DP_DEBUG(DP_DBG_FLAG_SPL, "%s ref_cnt left=%u for deq=%d qid=%d\n",
		 __func__, deq->ref_cnt, deq->ring_id, alloc.id.q_id);
	if (deq->ref_cnt)
		return DP_SUCCESS;

	return dp_node_free(&alloc, 0);
}

static int setup_deq_ring(int inst, struct dp_spl_cfg *conn,
				 struct dp_ring_pktdeq *deq)
{
	struct ppv4_q_sch_port pp = {
		.inst    = inst,
		.dp_port = conn->dp_port,
		.vap     = conn->spl_id,
		.cqe_deq = deq->attr.ring.index,
	};
	struct cqm_deq_ring_info *deq_info = get_dp_deqring_info(inst, pp.cqe_deq);

	if (!deq_ring_pre_update(inst, conn, deq)) {
		/* first time for this deq port */
		if (alloc_q_to_port(&pp, conn->flag) == DP_FAILURE)
			return DP_FAILURE;
		deq_ring_post_update(inst, deq, &pp);
	} else {
		DP_DEBUG(DP_DBG_FLAG_SPL,
			 "id:%d deq_ring:%d already allocated/shared",
			 conn->spl_id, deq_info->ring_id);
	}
	deq_info->ref_cnt++;

	return DP_SUCCESS;
}

static int remove_deq_ring(int inst, struct dp_spl_cfg *conn)
{
	int ret = DP_SUCCESS;
	int i;

	if (!conn->mem_port)
		return DP_FAILURE;

	for (i = 0; i < conn->mem_port->num_deq; i++) {
		if (release_deq_ring(inst, conn, &conn->mem_port->deq[i]))
			ret = DP_FAILURE;
	}
	return ret;
}

static int add_deq_ring(int inst, struct dp_spl_cfg *conn)
{
	int i;

	if (!conn->mem_port)
		return DP_FAILURE;
	for (i = 0; i < conn->mem_port->num_deq; i++) {
		int ret = setup_deq_ring(inst, conn, &conn->mem_port->deq[i]);

		if (ret) {
			pr_err("DPM: failed to setup deq_ring:%d(%d)\n",
			       conn->mem_port->deq[i].attr.ring.index, i);
			return ret;
		}
	}
	return DP_SUCCESS;
}

static int add_enq_ring(int inst, struct dp_spl_cfg *conn)
{
	int i;
	struct dp_port_info *port_info;
	struct dp_subif_info *sif;

	if (!conn->mem_port)
		return DP_FAILURE;
	port_info = get_dp_port_info(inst, conn->dp_port);
	sif = get_dp_port_subif(port_info, conn->spl_id);
	for (i = 0; i < conn->mem_port->num_enq; i++) {
		struct dp_ring_pktenq *e = &conn->mem_port->enq[i];

		if (!e->attr.ring.index)
			continue;
#ifdef TOPAZ_CODE_ENABLE // notvalid anymore
		if (conn->type == DP_SPL_TOE) {
			if (i == 0)
				dflt_q_toe[inst] = igp->egp[i].qid;
		}
#endif
	}
	return DP_SUCCESS;
}

#ifdef TOPAZ_CODE_ENABLE
/* Note: so far only support one UMT for spl connection */
static int add_umt(int inst, struct dp_spl_cfg *conn,
		   const struct dp_port_info *port)
{
	struct umt_ops *ops = dp_get_umt_ops(inst);
	struct umt_port_res *res = &conn->umt->res;
	struct cqm_deq_ring_info *deq;
	int ret = DP_SUCCESS;

	res->rx_src = UMT_RX_SRC_CQEM;
	res->cqm_enq_pid = conn->igp[0].igp_id;
	res->cqm_dq_pid = conn->egp[0].egp_id;
	if (conn->num_umt_port > 1) {
		pr_err("DPM: so far not support more than 2 umt yet: %d\n",
		       conn->num_umt_port);
		return DP_FAILURE;
	}
	deq = get_dp_deqring_info(inst, conn->egp[0].egp_id);

	if (!ops) {
		pr_err("DPM: No UMT driver registered\n");
		return -ENODEV;
	}
	if (!deq->umt_info[0].ref_cnt_umt) {
		if (ops->umt_alloc(ops->umt_dev, conn->umt) < 0) {
			pr_err("DPM: Failed to allocate umt for spl type=%s\n",
			       get_spl_name(conn->type));
			return -ENODEV;
		}
		ret = ops->umt_request(ops->umt_dev, conn->umt);
		if (ret) {
			pr_err("DPM: Failed to umt_request for spl_type: %s\n",
			       get_spl_name(conn->type));
			ops->umt_release(ops->umt_dev, conn->umt->ctl.id);
			return DP_FAILURE;
		}
		/* save umt info */
		deq->umt_info[0].umt = conn->umt[0];
	} else {
		/* get saved umt info */
		conn->umt[0] = deq->umt_info[0].umt;
	}
	deq->umt_info[0].ref_cnt_umt++;

	return DP_SUCCESS;
}

static int remove_umt(int inst, const struct dp_spl_cfg *conn)
{
	struct umt_ops *ops = dp_get_umt_ops(inst);
	struct cqm_deq_ring_info *deq;

	if (!ops)
		return -ENODEV;
	deq = get_dp_deqring_info(inst, conn->deq[0].attr.ring.index);
	deq->umt_info[0].ref_cnt_umt--;

	if (deq->umt_info[0].ref_cnt_umt)
		return DP_SUCCESS;

	/* Disable UMT port */
	if (ops->umt_enable(ops->umt_dev, conn->umt.ctl.id, 0))
		return DP_FAILURE;

	return ops->umt_release(ops->umt_dev, conn->umt.ctl.id);
}
#endif

static int sync_sif(int inst, struct dp_spl_cfg *conn,
			const struct dp_port_info *port,
			struct dp_subif_info *sif)
{
	dp_subif_t *subif = NULL;
	const struct dp_dflt_cfg_assign *ca;
	int ret = DP_FAILURE;

	if (!conn->dev) {
		DP_DEBUG(DP_DBG_FLAG_SPL, "%s dev NULL for spl_conn type=%d\n",
			 __func__, conn->type);
		return DP_SUCCESS;
	}
	if (conn->flag & DP_F_DEREGISTER) {
		if (dp_del_subif(conn->dev, NULL, NULL, NULL, 0)) {
			pr_err("DPM: dp_del_subif fail for %s\n", conn->dev->name);
			return DP_FAILURE;
		}
		return DP_SUCCESS;
	}
	ca = dp_get_dflt_cfg_info(inst, DP_F_CPU);
	if (!ca)
		return DP_FAILURE;
	subif = dp_kzalloc(sizeof(struct dp_subif), GFP_ATOMIC);
	if (!subif)
		return DP_FAILURE;
	dp_memcpy(&subif->subif_cmn_start,
		  &sif->subif_cmn_start,
		  &subif->subif_cmn_end - &subif->subif_cmn_start);
	subif->subif = sif->subif;
	subif->gpid = sif->gpid;
	dp_memcpy(subif->dfl_eg_sess, sif->dfl_eg_sess,
		  sizeof(sif->dfl_eg_sess));
	dp_memcpy(&subif->port_cmn_start, &port->port_cmn_start,
		  &subif->port_cmn_end - &subif->port_cmn_start);
	subif->inst = inst;
	subif->subif_num = 1;
	if (dp_update_subif(conn->dev, NULL, subif, conn->dev->name, 0, NULL)) {
		pr_err("DPM: dp_update_subif failed: %s\n", conn->dev->name);
		goto exit;
	}
	ret = DP_SUCCESS;
exit:
	kfree(subif);
	return ret;
}

/* note: just found there is no remove_igp to remove the queue yet
	 later need to add
 */
#ifdef TOPAZ_CODE_ENABLE
static int remove_spl_conn(int inst, struct dp_spl_cfg *conn,
			   const struct dp_port_info *port)
{
	int ret = DP_SUCCESS;
	struct dp_subif_info *sif = get_dp_port_subif(port, conn->spl_id);

	if (sif->flags & PORT_FREE) {
		pr_err("DPM: dpm:Failed to remove spl_conn(%d): not allocted yet\n",
		       conn->spl_id);
		return DP_FAILURE;
	}

	if (conn->f_gpid && dp_del_pp_gpid(inst, CPU_PORT, conn->spl_id)) {
		pr_err("DPM: failed to delete GPID:%d\n", conn->gpid);
		ret = DP_FAILURE;
	}

	if (conn->f_hostif && dp_del_hostif(inst, CPU_PORT, conn->spl_id)) {
		pr_err("DPM: failed to delete hostif for GPID:%d\n", conn->gpid);
		return DP_FAILURE;
	}

	if (dp_del_default_egress_sess(sif, 0)) {
		pr_err("DPM: Failed to delete def egress session\n");
		return DP_FAILURE;
	}

	/* remove exported subif */
	if (conn->dev)
		sync_sif(inst, conn, port, sif);

	/* remove internal subif */
	sif->flags = PORT_FREE;

	/* remove UMT */
	if (conn->num_umt_port) {
		if (remove_umt(inst, conn)) {
			pr_err("DPM: failed to release umt for spl type:%d\n",
			       conn->type);
		}
	}
	_dp_reset_subif_q_lookup_tbl(sif, 0, -1);
	/* later need to add remove_igp */

	/* remove PPv4 */
	if (remove_deq_ring(inst, conn)) {
		pr_err("DPM: failed to release egp for spl type:%d\n", conn->type);
		ret = DP_FAILURE;
	}
	/* remove CQM */
	ret = dp_cqm_spl_conn(inst, conn);

	dp_memset(conn, 0, sizeof(struct dp_spl_cfg));
	return ret;
}
#endif

static int spl_get_free_idx(int inst, struct dp_spl_cfg *conn)
{
	struct hal_priv *hal = HAL(inst);
	int cnt = 0;
	int i, start = 0, end = DP_MAX_SPL_CONN, free_idx = -1;

	end = DP_DPID0_CPU_SUBIF_START - 1;
	start = DP_DPID0_SUBIF_START;

	if (!conn->f_gpid) {
		/*spl conn w/o GPID then assign 11 to 0 subif_grp/vap*/
		/*iterate from end to start: reverse iteration*/
		for (i = end; i >= start; i--) {
			struct dp_spl_cfg *cfg = &hal->spl[i].user_data;

			if (!cfg->spl_id ) { /* free one */
				if (free_idx < 0) /* save first free entry */
					free_idx = i;
				continue;
			} else if (cfg->type != conn->type) {/* not match type */
				continue;
			}
			cnt++;
		}
	} else {
		/*spl conn with GPID assign from 0 to 11*/
		/*iterate from start to end: forward iteration*/
		/* found out all matched number of type and get the first free entry */
		for (i = start; i < end; i++) {
			struct dp_spl_cfg *cfg = &hal->spl[i].user_data;

			if (!cfg->spl_id ) { /* free one */
				if (free_idx < 0) /* save first free entry */
					free_idx = i;
				continue;
			} else if (cfg->type != conn->type) {/* not match type */
				continue;
			}
			cnt++;
		}
	}
	if (conn->type == DP_SPL_PP_NF) {
		if (cnt == MAX_PP_NF_CNT) {
			pr_err("DPM: reach maximum PPNF port count %d\n",
			       MAX_PP_NF_CNT);
			return DP_MAX_SPL_CONN;
		}
	} else {
		if (cnt) {
			pr_err("DPM: %s CPU special path created already\n",
			       get_spl_name(conn->type));
			return DP_MAX_SPL_CONN;
		}
	}
	DP_DEBUG(DP_DBG_FLAG_SPL, "%s:cnt=%d free_idx=%d spl_id=%d\n",
		 get_spl_name(conn->type),
		 cnt, free_idx, free_idx);

	if ((free_idx >= 0) && (free_idx < DP_MAX_SPL_CONN))
		return free_idx;
	pr_err("DPM: No enough room to create CPU special path for %s\n",
	       get_spl_name(conn->type));

	return DP_MAX_SPL_CONN;
}

#ifdef TOPAZ_CODE_ENABLE
static int alloc_gpid_for_spl_conn(int inst, struct dp_port_info *port,
		struct dp_spl_cfg *conn)
{
	struct hal_priv *priv = HAL(inst);

	conn->gpid = alloc_gpid(inst, DP_DYN_GPID, 1, CPU_PORT);
	if (conn->gpid == DP_FAILURE) {
		pr_err("DPM: %s %d, spl_conn(%d), type: %s, gpid alloc failed\n",
				__func__, __LINE__, conn->spl_id,
				get_spl_name(conn->type));
		return DP_FAILURE;
	}
	if (set_gsw_gpid_map(inst, CPU_PORT, conn->gpid, 1, -1, port->alloc_flags))
		return DP_FAILURE;

	if (set_cqm_gpid_map(inst, CPU_PORT, conn->gpid, 1, -1, port->alloc_flags))
		return DP_FAILURE;

	/* Update table */
	priv->gp_dp_map[conn->gpid].dpid = CPU_PORT;

	return DP_SUCCESS;
}

static int add_spl_conn(int inst, struct dp_spl_cfg *conn,
			      const struct dp_port_info *port)
{
	struct cqm_deq_ring_stat *m = NULL;
	struct dp_spl_cfg_priv *priv_data;
	struct hal_priv *hal = HAL(inst);
	struct dp_subif_info *sif;
	int idx, i;

	if (is_invalid_inst(inst))
		return DP_FAILURE;
	if (conn->type >= DP_SPL_MAX) {
		pr_err("DPM: invalid spl conn type: %d\n", conn->type);
		return DP_FAILURE;
	}

	if (conn->dp_cb && conn->dp_cb->rx_fn && !conn->dev) {
		pr_err("DPM: cannot attach rx_fn without dev for %s\n",
		       get_spl_name(conn->type));
		return DP_FAILURE;
	}
	if ((conn->type == DP_SPL_TOE) && (conn->f_subif || conn->f_gpid)) {
		pr_err("DPM: invalid flag for %s\n", get_spl_name(conn->type));
		return DP_FAILURE;
	}
	if (!conn->f_subif && conn->f_gpid) {
		pr_err("DPM: dpm:f_subif must be set when f_gpid is set\n");
		return DP_FAILURE;
	}

	/* For CQM driver purpose to set DP_F_CPU */
	conn->flag |= DP_F_CPU;
	idx = spl_get_free_idx(inst, conn);
	if (idx == DP_MAX_SPL_CONN)
		return DP_FAILURE;
	/* set spl_conn spl_id = vap/subif_grp */
	conn->spl_id = idx;
	if (conn->spl_id >= DP_DPID0_CPU_SUBIF_START) {
		pr_err("DPM: %s spl conn id: %d should be less than %d\n",
				__func__,
				conn->spl_id, DP_DPID0_CPU_SUBIF_START);
		return DP_FAILURE;
	}
	sif = get_dp_port_subif(port, conn->spl_id);
	if (sif->flags != PORT_FREE) {
		pr_err("DPM: %s allocated already\n", get_spl_name(conn->type));
		return DP_FAILURE;
	}
	DP_DEBUG(DP_DBG_FLAG_SPL, "spl_conn type=%s(%d) idx=%d\n",
		 get_spl_name(conn->type),
		 conn->type, conn->spl_id);
	conn->dp_port = port->port_id;
	sif->port_info = port;
	sif->subif_groupid = conn->spl_id;
	BUG_ON(port->port_id != CPU_PORT);
	conn->gpid = -1;
	if (conn->f_gpid) {
		/*TODO: allocate new gpid */
		if (alloc_gpid_for_spl_conn(inst, port, conn))
			return DP_FAILURE;
#ifdef TOPAZ_CODE_ENABLE
		if ((conn->gpid > DP_DYN_GPID_START) &&
				(conn->gpid < DP_DYN_GPID_END))
			pr_err("DPM: %s: %d, spl conn gpid: %d is not matched the concept[%d, %d]\n",
					__func__, __LINE__, conn->gpid,
					DP_DYN_GPID_START,
					DP_DYN_GPID_END);
#endif
		conn->subif = SET_VAP(conn->spl_id, port->vap_offset, port->vap_mask);
	} else
		conn->subif = SET_VAP(conn->spl_id, port->vap_offset, 0xffffffff);
	priv_data = dp_kzalloc(sizeof(*priv_data), GFP_ATOMTIC);
	if (!priv_data) {
		pr_err("DPM: %s, failed to alloc memory!\n", __func__);
		return DP_FAILURE;
	}
	priv_data->udata = conn;
	if (dp_cqm_spl_conn(inst, priv_data))
		goto free;
	if (add_deq_ring(inst, conn))
		goto free;
	if (add_enq_ring(inst, conn))
		goto free;
	if (add_ret_ring(inst, conn))
		goto free;
	if (add_req_ring(inst, conn))
		goto free;
	if (add_ippu_ring(inst, conn))
		goto free;
	if (conn->num_umt) {
		if (add_umt(inst, conn, port)) {
			pr_err("DPM: failed to setup umt for %s\n",
			       get_spl_name(conn->type));
			goto free;
		}
	}

	sif = get_dp_port_subif(port, conn->spl_id);
	sif->subif_flag = conn->flag;
	sif->netif = conn->dev;
	sif->subif = conn->subif;
	sif->subif_groupid = conn->spl_id;
	sif->gpid = conn->gpid;
	sif->netif = conn->dev;
	sif->tx_policy_base = conn->policy[0].tx_policy_base;
	sif->tx_policy_num = conn->policy[0].tx_policy_num;
	sif->rx_policy_base = conn->policy[0].rx_policy_base;
	sif->rx_policy_num = conn->policy[0].rx_policy_num;
	sif->num_qid = 0;
	for (i = 0; i < conn->num_egp; i++) {
		/* here return all queue whether
		 * conn->egp[i].type == DP_EGP_TO_DEV or not
		 * For PP APP lite, in fact, no queue to device at all
		 */
		sif->def_qid_list[i] = conn->egp[i].qid;
		sif->q_node[i] = get_deq_q_node(inst, conn->egp);
		sif->num_qid++;
		m = &HAL(inst)->deq_ring_stat[conn->egp[i].egp_id];
		sif->qos_deq_port[i] = m->node_id;
	}
	sif->num_igp = conn->num_igp;
	sif->num_egp = conn->num_egp;
	sif->prel2_len = conn->prel2_len;
	sif->spl_conn_type = conn->type;
	if (conn->dp_cb) {
		atomic_set(&sif->rx_flag, 1);
		sif->rx_fn = conn->dp_cb->rx_fn;
	}

	if (conn->dev) {
		dp_memcpy(sif->device_name, conn->dev->name, IFNAMSIZ);
		if (sync_sif(inst, conn, port, sif)) {
			pr_err("DPM: failed to sync subif for %s\n",
			       get_spl_name(conn->type));
			goto free;
		}
	} else {
		dp_strlcpy(sif->device_name, get_spl_name(conn->type),
			   sizeof(sif->device_name));
	}

	if (conn->f_gpid) {
		if (dp_add_pp_gpid(inst, CPU_PORT, conn->spl_id, conn->gpid, 0, 0))
			goto free;
		if (conn->type == DP_SPL_VOICE) {
			BUG_ON(!port->gpid_spl);
			if (dp_add_pp_gpid(inst, CPU_PORT, 0, port->gpid_spl, 1, 0))
				goto free;
			conn->spl_gpid = port->gpid_spl;
		}
	}
	if (conn->f_hostif)
		dp_add_hostif(inst, CPU_PORT, conn->spl_id);
	if (conn->num_igp) {
		/* at present, DPM hardcoded here before DRVLIB_SW-4341 done
		 * once DRVLIB_SW-4341 is done, DPM should check out_qos_mode
		 */
		if (conn->type == DP_SPL_VPNA) {
			_dp_init_subif_q_map_rules(sif, 1);
			_dp_set_subif_q_lookup_tbl(sif, 0,
						   conn->igp->egp[0].qid, -1);
		}
	}

	/*
	 * Increase the cpu allocated subif cnt: port->num_subif++ (even if user
	 * give no subif allocation flag, we allocate interanally to satisfy DPM
	 * state machine)
	 */
	port->num_subif++;

	/*default tc value for toe*/
	sif->toe_tc = TOE_TC_DEF_VALUE;
	sif->flags = PORT_SUBIF_REGISTERED;

	/*Store priv_data into local DB hal->spl*/
	hal->spl[conn->spl_id - DP_DPID0_SPL_CONN_SUBIF_START].priv_data = *priv_data;
	/*hal spl array index: conn->spl_id - DP_DPID0_SPL_CONN_SUBIF_START*/
	hal->spl[conn->spl_id - DP_DPID0_SPL_CONN_SUBIF_START].user_data = *conn;
	if (conn->mem_port)
		hal->spl[conn->spl_id - DP_DPID0_SPL_CONN_SUBIF_START].mem_port = *conn->mem_port;

	/* Keep a reference of spl_cfg from hal_priv in subif */
	sif->spl_cfg = &hal->spl[conn->spl_id - DP_DPID0_SPL_CONN_SUBIF_START];

	DP_DEBUG(DP_DBG_FLAG_SPL, "%s id:%d gpid:%d %s",
		 sif->device_name, conn->spl_id, sif->gpid,
		 conn->dev ? conn->dev->name : "");
	kfree(priv_data);
	return DP_SUCCESS;
free:
	kfree(priv_data);
	return DP_FAILURE;
}
#endif

static void dump_spl_parameters(int inst, struct dp_spl_cfg *conn,
				   bool insert_line, char *s)
{
#ifdef TOPAZ_CODE_ENABLE
	int i;
	int size = 300;
	int idx;
	char *buf = dp_kmalloc(size, GFP_ATOMIC);

	if (!buf)
		return;

	DP_DUMP("DPM: %s%s\n", insert_line ? "\n" : "", s);
	DP_DUMP("   type:%s, spl_id:%d, f_subif:%d, f_gpid:%d, f_policy:%d, "
		"f_hostif:%d, dp_cb:%s, dev:%s\n",
		get_spl_name(conn->type), conn->spl_id,
		conn->f_subif, conn->f_gpid, conn->f_policy, conn->f_hostif,
		conn->dp_cb ? "set" : "NULL",
		conn->dev ? conn->dev->name : NULL);
	if (conn->flag) {
		idx = scnprintf(buf, size, "   flags(%x): ", conn->flag);
		for (i = 0; i < get_dp_port_type_str_size(); i++) {
			if (conn->flag & dp_port_flag[i])
				idx += scnprintf(buf + idx, size - idx - 1,
						"%s ", dp_port_type_str[i]);
		}
		idx += scnprintf(buf + idx, size - idx - 1, "\n");
		DP_DUMP("%s", buf);
	}
	for (i = 0; i < conn->num_enq; i++) {
		struct dp_ring_pktenq *igp = &conn->enq[i];

		DP_DUMP("   [%02d]: IGP%d --> igp_ring_size:%d out_msg_mode:%d out_qos_mode:%d egp:%d\n",
			i, igp->igp_id, igp->igp_ring_size, igp->out_msg_mode,
			igp->out_qos_mode, igp->egp ? igp->egp->egp_id : -1);
	}
	for (i = 0; i < conn->num_deq; i++) {
		struct dp_ring_pktdeq *egp = &conn->deq[i];

		DP_DUMP("   [%02d]: EGP%d --> pp_ring_size:%d tx_push_paddr:0x%px tx_push_paddr_qos:0x%px\n",
			i, egp->egp_id, egp->pp_ring_size, egp->tx_push_paddr,
			egp->tx_push_paddr_qos);
	}

	if (conn->num_umt_port) {
		struct umt_port_ctl *c = &conn->umt->ctl;

		DP_DUMP("  %s:%d %s:%d %s:%d %s:%d %s:%d %s:%d %s:%d %s:%ld\n",
			"UMT id", c->id,
			"msg_interval", c->msg_interval,
			"msg_mode", c->msg_mode,
			"cnt_mode", c->cnt_mode,
			"sw_msg", c->sw_msg,
			"rx_msg_mode", c->rx_msg_mode,
			"enable", c->enable,
			"fflag", c->fflag);
		DP_DUMP("  dst_addr_cnt: %u\n", c->dst_addr_cnt);
		if (unlikely(c->dst_addr_cnt >= UMT_DST_ADDR_MAX)) {
			pr_err("  DPM: %s, "
			       "umt dst_addr_cnt(%u) >= UMT_DST_ADDR_MAX(%d)",
			       __func__, c->dst_addr_cnt, UMT_DST_ADDR_MAX);
		} else {
			if (c->dst_addr_cnt) {
				int i;
				DP_DUMP("  dst[]: ");
				for (i = 0; i < c->dst_addr_cnt; i++)
					DP_DUMP("%pad ", &c->dst[i]);
				DP_DUMP("\n");
			} else {
				DP_DUMP("  daddr: %pad\n", &c->daddr);
			}
		}
	}

	if (conn->f_policy)
		DP_DUMP("policy rx_pkt_size:%d\n", conn->policy->rx_pkt_size);
	kfree(buf);
#endif
}
#define EGP_SHIFT(x, offset) (struct dp_spl_egp *)((unsigned long) (x) + offset)
int _dp_spl_conn(int inst, struct dp_spl_cfg *conn)
{
#ifdef TOPAZ_CODE_ENABLE
	struct hal_priv *hal = HAL(inst);
	struct dp_port_info *port;
	int ret = DP_FAILURE;

	if (!hal) {
		pr_err("DPM: DP HAL not initialized yet\n");
		return DP_FAILURE;
	}

	if (!dp_init_ok) {
		pr_err("DPM: DP not initialized yet\n");
		return DP_FAILURE;
	}
	port = get_dp_port_info(inst, CPU_PORT);
	if (!port) {
		pr_err("DPM: %s why port NULL\n", __func__);
		return DP_FAILURE;
	}
	if (IS_ENABLED(CONFIG_DPM_DATAPATH_DBG) &&
	    unlikely(dp_dbg_flag & DP_DBG_FLAG_SPL))
		dump_spl_parameters(inst, conn, 0,
			(conn->flag & DP_F_DEREGISTER) ? "de-register caller" :
			"register caller");
	DP_LIB_LOCK(&dp_lock);
	if ((conn->flag & DP_F_DEREGISTER)) { /* de_register spl_conn */
		int i;

		DP_DEBUG(DP_DBG_FLAG_SPL,
			 "de_register_spl: type=%s(%d) spid=%d %s\n",
			 get_spl_name(conn->type), conn->type,
			 conn->spl_id,
			 conn->dev ? conn->dev->name: "");

		for (i = 0; i < MAX_SPL_CONN_CNT; i++) {
			struct dp_spl_cfg *cfg = &hal->spl[i].user_data;
			struct dp_subif_info *sif;

			if (cfg->spl_id != conn->spl_id)
				continue;
			cfg->flag = conn->flag;
			if (IS_ENABLED(CONFIG_DPM_DATAPATH_DBG) &&
			    unlikely(dp_dbg_flag & DP_DBG_FLAG_SPL))
				dump_spl_parameters(inst, conn, 0, "saved-conn");
			ret = remove_spl_conn(inst, cfg, port);
			sif = get_dp_port_subif(port, conn->spl_id);
			sif->spl_cfg = NULL;
			break;
		}
		if (i == MAX_SPL_CONN_CNT)
			pr_err("DPM: No matching spl_id=%d %s\n",
			       conn->spl_id,
			       conn->dev ? conn->dev->name : "");
		else {
			ret = DP_SUCCESS;
			DP_DEBUG(DP_DBG_FLAG_SPL, "de-register-spl done\n\n");
		}

		goto exit;
	}

	/* register spl_conn */
	ret = add_spl_conn(inst, conn, port);
	if (ret == DP_SUCCESS) {
		struct dp_spl_cfg *tmp_cfg;
		struct dp_subif_info *sif;
		int i;
		unsigned long offset;

		sif = get_dp_port_subif(port, conn->spl_id);
		dp_memcpy(tmp_cfg, conn, sizeof(struct dp_spl_cfg));
		/* update internal igp->egp pointers correctly */
		for (i = 0; i < conn->num_igp; i++) {
			if (conn->igp[i].egp) {
				offset = (unsigned long) conn->igp[i].egp - (unsigned long) conn->egp;
				tmp_cfg->igp[i].egp = EGP_SHIFT(tmp_cfg->egp,
								offset);
			} else {
				pr_err("DPM: %s: conn->igp[%d].egp NULL\n", __func__, i);
			}
		}
#ifdef CONFIG_RFS_ACCEL
		if (conn->dev && port->rx_cpu_rmap) {
			conn->dev->rx_cpu_rmap = port->rx_cpu_rmap;
			if (unlikely(!conn->dev->rx_cpu_rmap))
				DP_DEBUG(DP_DBG_FLAG_SPL, "[%s]:rx_cpu_rmap NULL for [%s]\n", __func__ , conn->dev->name);
		}
#endif /* CONFIG_RFS_ACCEL */

		if (IS_ENABLED(CONFIG_DPM_DATAPATH_DBG) &&
		    unlikely(dp_dbg_flag & DP_DBG_FLAG_SPL)) {
			DP_DEBUG(DP_DBG_FLAG_SPL,
				 "register_spl: type=%s(%d) spid=%d %s\n",
				 get_spl_name(conn->type), conn->type,
				 conn->spl_id,
				 conn->dev ? conn->dev->name: "");
			dump_spl_parameters(inst, conn, 0, "spl_conn created");
		}
	} else {
		pr_err("DPM: %s failed: register_spl: spid=%d %s\n", __func__,
		       conn->spl_id,
		       conn->dev ? conn->dev->name: "");
	}
exit:
	DP_LIB_UNLOCK(&dp_lock);
	return ret;
#else
	(void)inst;
	(void)conn;
	return 0;
#endif
}

int spl_conn_get(int inst, enum DP_SPL_TYPE type,
		    struct dp_spl_cfg *conns, u8 total)
{
	struct hal_priv *hal = HAL(inst);
	u8 cnt = 0;
	int i;

	if (!dp_init_ok) {
		pr_err("DPM: DP not initialized yet\n");
		return 0;
	}
	if (!hal) {
		pr_err_once("%s hal[%d] is NULL\n", __func__, inst);
		return 0;
	}
	if (type >= DP_SPL_MAX) {
		pr_err("DPM: wrong type: %d\n", type);
		return 0;
	}

	if (type != DP_SPL_PP_NF)
		total = 1;
	DP_DEBUG(DP_DBG_FLAG_SPL, "type:%s cnt:%d", get_spl_name(type), total);
	for (i = 0; i < DP_MAX_SPL_CONN; i++) {
		struct dp_spl_cfg *cfg = &hal->spl[i].user_data;
		/* for PP NF case it will return first PP NF */
		if (!cfg->spl_id || cfg->type != type)
			continue;
		dp_memcpy(conns + cnt, cfg, sizeof(struct dp_spl_cfg));
		if (++cnt >= total)
			break;
	}
	return cnt;
}

int _dp_spl_conn_get(int inst, enum DP_SPL_TYPE type,
		       struct dp_spl_cfg *conns, u8 total)
{
	int ret;

	DP_LIB_LOCK(&dp_lock);
	ret = spl_conn_get(inst, type, conns, total);
	DP_LIB_UNLOCK(&dp_lock);
	return ret;
}

int dp_spl_conn_get_num_of_egps(int inst, int egps[16])
{
#ifdef TOPAZ_CODE_ENABLE
	struct hal_priv *hal = HAL(inst);
	int cnt = 0;
	int i;

	if (!dp_init_ok) {
		pr_err("DPM: DP not initialized yet\n");
		return 0;
	}
	if (!hal) {
		pr_err_once("%s hal[%d] is NULL\n", __func__, inst);
		return 0;
	}

	DP_LIB_LOCK(&dp_lock);
	for (i = 0; i < MAX_SPL_CONN_CNT; i++) {
		struct dp_spl_cfg *cfg = &hal->spl[i];
		int c;

		if (!cfg->spl_id)
			continue;
		for (c = 0; c < cfg->num_egp; c++)
			egps[cnt++] = cfg->egp[c].egp_id;
	}
	DP_LIB_UNLOCK(&dp_lock);
	return cnt;
#else
	return 0;
#endif
}

static const char *get_dev_name(int inst, const struct dp_spl_cfg *c)
{
	return get_dp_port_subif(get_dp_port_info(inst, CPU_PORT),
				 c->spl_id)->device_name;
}

const char *dp_spl_conn_get_name_from_egp(int inst, int egp_id)
{
#ifdef TOPAZ_CODE_ENABLE
	struct hal_priv *hal = HAL(inst);
	const char *dev_name = NULL;
	int i;

	if (!dp_init_ok) {
		pr_err("DPM: DP not initialized yet\n");
		return 0;
	}
	if (!hal) {
		pr_err_once("%s hal[%d] is NULL\n", __func__, inst);
		return 0;
	}
	DP_LIB_LOCK(&dp_lock);
	for (i = 0; i < MAX_SPL_CONN_CNT; i++) {
		struct dp_spl_cfg *cfg = &hal->spl[i];
		int c;

		if (!cfg->spl_id)
			continue;
		for (c = 0; c < cfg->num_egp; c++) {
			if (cfg->egp[c].egp_id == egp_id) {
				dev_name = get_dev_name(inst, cfg);
				goto exit;
			}
		}
	}
exit:
	DP_LIB_UNLOCK(&dp_lock);
	return dev_name;
#else
	return NULL;
#endif
}

/*Just to tell if this egp is belongs to spl device*/
int dp_is_spl_conn(int inst, int ring_id)
{
	int found = 0;
	struct hal_priv *hal = HAL(inst);
	int i;

	if (!dp_init_ok) {
		pr_err("DPM: DP not initialized yet\n");
		return 0;
	}
	if (!hal) {
		pr_err_once("%s hal[%d] is NULL\n", __func__, inst);
		return 0;
	}

	DP_LIB_LOCK(&dp_lock);
	for (i = 0; i < DP_MAX_SPL_CONN; i++) {
		struct dp_spl_cfg *cfg = &hal->spl[i].user_data;
		int c;

		if (!cfg->spl_id)
			continue;
		for (c = 0; c < cfg->mem_port->num_deq; c++) {
			if (cfg->mem_port->deq[c].attr.ring.index == ring_id) {
				found = 1;
				goto exit;
			}
		}
	}
exit:
	DP_LIB_UNLOCK(&dp_lock);
	return found;
}

/* disable optimization in debug mode: pop */
DP_NO_OPTIMIZE_POP
